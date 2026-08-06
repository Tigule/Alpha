#include <Base/Base.h>

#include "Base/MsgBuffer.h"

#include <storm.h>
#include <string.h>

void CMsgBuffer::ReallocData(UINT count) {
  if (count & 0xFF) {
    count += 0x100 - (count & 0xFF);
  }
  m_alloc = count;
  if (m_freeData) {
    m_data = static_cast<BYTE *>(SMemReAlloc(m_data, count, __FILE__, __LINE__, 0));
  } else {
    BYTE *data = static_cast<BYTE *>(SMemAlloc(count, __FILE__, __LINE__, 0));
    memcpy(data, m_data, m_write);
    m_data = data;
  }
  m_freeData = 1;
}

#define DEFINE_ADD_SCALAR(functionName, valueType)          \
  void CMsgBuffer::functionName(valueType val) {            \
    Reserve(sizeof(val));                                   \
    *reinterpret_cast<valueType *>(m_data + m_write) = val; \
    m_write += sizeof(val);                                 \
  }

DEFINE_ADD_SCALAR(AddChar, char)
DEFINE_ADD_SCALAR(AddUchar, BYTE)
DEFINE_ADD_SCALAR(AddByte, BYTE)
DEFINE_ADD_SCALAR(AddTchar, char)
DEFINE_ADD_SCALAR(AddShort, short)
DEFINE_ADD_SCALAR(AddUshort, WORD)
DEFINE_ADD_SCALAR(AddWord, WORD)
DEFINE_ADD_SCALAR(AddInt, int)
DEFINE_ADD_SCALAR(AddUint, UINT)
DEFINE_ADD_SCALAR(AddLong, long)
DEFINE_ADD_SCALAR(AddUlong, DWORD)
DEFINE_ADD_SCALAR(AddDword, DWORD)
DEFINE_ADD_SCALAR(AddLongLong, LONGLONG)
DEFINE_ADD_SCALAR(AddUlongLong, DWORDLONG)
DEFINE_ADD_SCALAR(AddFloat, float)

#undef DEFINE_ADD_SCALAR

void CMsgBuffer::AddTcharArray(LPCSTR str, UINT count, int zeroExtra) {
  Reserve(count);
  while (count) {
    BYTE value = *str++;
    m_data[m_write++] = value;
    --count;
    if (zeroExtra && !value) {
      memset(m_data + m_write, 0, count);
      m_write += count;
      return;
    }
  }
}

void CMsgBuffer::AddTcharString(LPCSTR str, int compress) {
  UINT length = strlen(str);
  if (length > 0x3FFF) {
    ASSERT(length <= 0x3FFF);
    length = 0x3FFF;
  }

  BYTE prefix = static_cast<BYTE>(length & 0x3F);
  if (length > 0x3F) {
    prefix |= 0x40;
  }
  if (compress) {
    for (UINT i = 0; i < length; ++i) {
      if (str[i] > 0xFF) {
        prefix |= 0x80;
        break;
      }
    }
  } else {
    prefix |= 0x80;
  }
  AddByte(prefix);
  if (prefix & 0x40) {
    AddByte(static_cast<BYTE>(length >> 8));
  }
  if (prefix & 0x80) {
    AddTcharArray(str, length, 0);
  } else {
    for (UINT i = 0; i < length; ++i) {
      AddByte(str[i]);
    }
  }
}

void CMsgBuffer::AddData(BYTE *data, UINT count) {
  AddData(static_cast<LPCVOID>(data), count);
}

void CMsgBuffer::AddData(LPCVOID data, UINT count) {
  Reserve(count);
  memcpy(m_data + m_write, data, count);
  m_write += count;
}

#define DEFINE_ADD_ARRAY(functionName, valueType)                      \
  void CMsgBuffer::functionName(const valueType *buffer, UINT count) { \
    AddData(buffer, count * sizeof(valueType));                        \
  }

DEFINE_ADD_ARRAY(AddWordArray, WORD)
DEFINE_ADD_ARRAY(AddDwordArray, DWORD)
DEFINE_ADD_ARRAY(AddUintArray, UINT)
DEFINE_ADD_ARRAY(AddFloatArray, float)

#undef DEFINE_ADD_ARRAY

#define DEFINE_GET_SCALAR(functionName, valueType)                     \
  valueType CMsgBuffer::functionName() {                               \
    ASSERT(Bytes() >= sizeof(valueType));                              \
    valueType value = *reinterpret_cast<valueType *>(m_data + m_read); \
    m_read += sizeof(valueType);                                       \
    return value;                                                      \
  }

DEFINE_GET_SCALAR(GetChar, char)
DEFINE_GET_SCALAR(GetUchar, BYTE)
DEFINE_GET_SCALAR(GetByte, BYTE)
DEFINE_GET_SCALAR(GetTchar, char)
DEFINE_GET_SCALAR(GetShort, short)
DEFINE_GET_SCALAR(GetUshort, WORD)
DEFINE_GET_SCALAR(GetWord, WORD)
DEFINE_GET_SCALAR(GetInt, int)
DEFINE_GET_SCALAR(GetUint, UINT)
DEFINE_GET_SCALAR(GetLong, long)
DEFINE_GET_SCALAR(GetUlong, DWORD)
DEFINE_GET_SCALAR(GetDword, DWORD)
DEFINE_GET_SCALAR(GetLongLong, LONGLONG)
DEFINE_GET_SCALAR(GetUlongLong, DWORDLONG)
DEFINE_GET_SCALAR(GetFloat, float)

#undef DEFINE_GET_SCALAR

void CMsgBuffer::GetTcharArray(char *buffer, UINT count) {
  GetData(buffer, count);
}

UINT CMsgBuffer::GetTcharStringBufferLength(int *wide) {
  BYTE prefix = GetByte();
  UINT length = prefix & 0x3F;
  if (prefix & 0x40) {
    length |= static_cast<UINT>(GetByte()) << 8;
  }
  if (wide) {
    *wide = (prefix & 0x80) == 0x80;
  }
  return length + 1;
}

void CMsgBuffer::GetTcharString(char *buffer, UINT bufferLength, int wide) {
  if (wide) {
    GetTcharArray(buffer, bufferLength - 1);
    buffer[bufferLength - 1] = 0;
  } else {
    UINT count = bufferLength - 1;
    while (count--) {
      *buffer++ = GetByte();
    }
    *buffer = 0;
  }
}

LPCVOID CMsgBuffer::GetData(int count) {
  ASSERT(Bytes() >= count);
  LPCVOID data = m_data + m_read;
  m_read += count;
  return data;
}

void CMsgBuffer::GetData(LPVOID buffer, int count) {
  memcpy(buffer, GetData(count), count);
}

#define DEFINE_GET_ARRAY(functionName, valueType)                \
  void CMsgBuffer::functionName(valueType *buffer, UINT count) { \
    GetData(buffer, count * sizeof(valueType));                  \
  }

DEFINE_GET_ARRAY(GetWordArray, WORD)
DEFINE_GET_ARRAY(GetDwordArray, DWORD)
DEFINE_GET_ARRAY(GetUintArray, UINT)
DEFINE_GET_ARRAY(GetFloatArray, float)

#undef DEFINE_GET_ARRAY
