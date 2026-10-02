#include <Base/Base.h>

#include "Base/MsgBuffer.h"

#include <storm.h>
#include <string.h>

#define CMB_TCHARSTR_MAX_LONG_LEN 0x3FFF

void CMsgBuffer::ReallocData(UINT count) {
  BYTE *data = m_data;
  if (count & 0xFF) {
    count += 0x100 - (count & 0xFF);
  }
  m_alloc = count;
  if (m_freeData) {
    m_data = static_cast<BYTE *>(SMemReAlloc(data, count, __FILE__, __LINE__, 0));
  } else {
    m_data = static_cast<BYTE *>(SMemAlloc(count, __FILE__, __LINE__, 0));
    memcpy(m_data, data, m_write);
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
    --count;
    BYTE value = *str++;
    m_data[m_write++] = value;
    if (zeroExtra && !value) {
      memset(m_data + m_write, 0, count);
      m_write += count;
      return;
    }
  }
}

void CMsgBuffer::AddTcharString(LPCSTR str, int compress) {
  UINT len = strlen(str);
  if (len > CMB_TCHARSTR_MAX_LONG_LEN) {
    ASSERT(len <= CMB_TCHARSTR_MAX_LONG_LEN);
    len = CMB_TCHARSTR_MAX_LONG_LEN;
  }

  BYTE prefix = static_cast<BYTE>(len & 0x3F);
  if (len > 0x3F) {
    prefix |= 0x40;
  }
  if (compress) {
    for (UINT i = 0; i < len; ++i) {
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
    AddByte(static_cast<BYTE>(len >> 8));
  }
  if (prefix & 0x80) {
    AddTcharArray(str, len, 0);
  } else {
    for (UINT i = 0; i < len; ++i) {
      AddByte(str[i]);
    }
  }
}

void CMsgBuffer::AddData(BYTE *data, UINT count) {
  Reserve(count);
  memcpy(m_data + m_write, data, count);
  m_write += count;
}

void CMsgBuffer::AddData(LPCVOID data, UINT count) {
  Reserve(count);
  memcpy(m_data + m_write, data, count);
  m_write += count;
}

void CMsgBuffer::AddWordArray(const WORD *buffer, UINT count) {
  uint bytes = count * sizeof(WORD);
  Reserve(bytes);
  memcpy(m_data + m_write, buffer, bytes);
  m_write += bytes;
}

void CMsgBuffer::AddDwordArray(const DWORD *buffer, UINT count) {
  uint bytes = count * sizeof(DWORD);
  Reserve(bytes);
  memcpy(m_data + m_write, buffer, bytes);
  m_write += bytes;
}

void CMsgBuffer::AddUintArray(const UINT *buffer, UINT count) {
  uint bytes = count * sizeof(UINT);
  Reserve(bytes);
  memcpy(m_data + m_write, buffer, bytes);
  m_write += bytes;
}

void CMsgBuffer::AddFloatArray(const float *buffer, UINT count) {
  uint bytes = count * sizeof(float);
  Reserve(bytes);
  memcpy(m_data + m_write, buffer, bytes);
  m_write += bytes;
}

#define DEFINE_GET_SCALAR(functionName, valueType)                 \
  valueType CMsgBuffer::functionName() {                           \
    valueType val;                                                \
    ASSERT(Bytes() >= sizeof(val));                               \
    val = *reinterpret_cast<valueType *>(m_data + m_read);         \
    m_read += sizeof(val);                                        \
    return val;                                                   \
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
  while (count--) {
    *buffer++ = GetTchar();
  }
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
  UINT len = bufferLength - 1;
  if (wide) {
    GetTcharArray(buffer, len);
    buffer[len] = 0;
  } else {
    while (len--) {
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
  ASSERT(Bytes() >= count);
  memcpy(buffer, m_data + m_read, count);
  m_read += count;
}

void CMsgBuffer::GetWordArray(WORD *buffer, UINT count) {
  uint bytes = count * sizeof(WORD);
  ASSERT(bytes <= (uint)Bytes());
  memcpy(buffer, m_data + m_read, bytes);
  m_read += bytes;
}

void CMsgBuffer::GetDwordArray(DWORD *buffer, UINT count) {
  uint bytes = count * sizeof(DWORD);
  ASSERT(bytes <= (uint)Bytes());
  memcpy(buffer, m_data + m_read, bytes);
  m_read += bytes;
}

void CMsgBuffer::GetUintArray(UINT *buffer, UINT count) {
  uint bytes = count * sizeof(UINT);
  ASSERT(bytes <= (uint)Bytes());
  memcpy(buffer, m_data + m_read, bytes);
  m_read += bytes;
}

void CMsgBuffer::GetFloatArray(float *buffer, UINT count) {
  uint bytes = count * sizeof(float);
  ASSERT(bytes <= (uint)Bytes());
  memcpy(buffer, m_data + m_read, bytes);
  m_read += bytes;
}
