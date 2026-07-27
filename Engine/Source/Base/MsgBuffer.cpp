#include "Base/MsgBuffer.h"

#include <storm.h>
#include <string.h>

CMsgBuffer::CMsgBuffer(unsigned int count)
    : m_alloc(count), m_freeData(1), m_read(0), m_write(0),
      m_data(count ? static_cast<unsigned char *>(SMemAlloc(count, __FILE__, __LINE__, 0)) : 0) {
}

CMsgBuffer::~CMsgBuffer() {
  if (m_freeData && m_data) {
    SMemFree(m_data, __FILE__, __LINE__, 0);
  }
}

void CMsgBuffer::Reset() {
  m_read = 0;
  m_write = 0;
}

int CMsgBuffer::Bytes() const {
  return m_write - m_read;
}

unsigned int CMsgBuffer::GetReadPosition() {
  return m_read;
}

void CMsgBuffer::SetReadPosition(unsigned int position) {
  m_read = position;
}

unsigned int CMsgBuffer::GetWritePosition() {
  return m_write;
}

void CMsgBuffer::SetWritePosition(unsigned int position) {
  m_write = position;
}

unsigned char *CMsgBuffer::Data() {
  return m_data;
}

void CMsgBuffer::SetData(unsigned char *data, unsigned int count, int freeData) {
  if (m_freeData && m_data) {
    SMemFree(m_data, __FILE__, __LINE__, 0);
  }
  m_alloc = count;
  m_freeData = freeData;
  m_read = 0;
  m_write = count;
  m_data = data;
}

void CMsgBuffer::ReallocData(unsigned int count) {
  if (count & 0xFF) {
    count += 0x100 - (count & 0xFF);
  }
  m_alloc = count;
  if (m_freeData) {
    m_data = static_cast<unsigned char *>(SMemReAlloc(m_data, count, __FILE__, __LINE__, 0));
  } else {
    unsigned char *data = static_cast<unsigned char *>(SMemAlloc(count, __FILE__, __LINE__, 0));
    memcpy(data, m_data, m_write);
    m_data = data;
  }
  m_freeData = 1;
}

void CMsgBuffer::Reserve(unsigned int count) {
  if (m_write + count > m_alloc) {
    ReallocData(m_write + count);
  }
}

#define DEFINE_ADD_SCALAR(functionName, valueType) \
  void CMsgBuffer::functionName(valueType val) {   \
    Reserve(sizeof(val));                           \
    *reinterpret_cast<valueType *>(m_data + m_write) = val; \
    m_write += sizeof(val);                         \
  }

DEFINE_ADD_SCALAR(AddChar, char)
DEFINE_ADD_SCALAR(AddUchar, unsigned char)
DEFINE_ADD_SCALAR(AddByte, unsigned char)
DEFINE_ADD_SCALAR(AddTchar, char)
DEFINE_ADD_SCALAR(AddShort, short)
DEFINE_ADD_SCALAR(AddUshort, unsigned short)
DEFINE_ADD_SCALAR(AddWord, unsigned short)
DEFINE_ADD_SCALAR(AddInt, int)
DEFINE_ADD_SCALAR(AddUint, unsigned int)
DEFINE_ADD_SCALAR(AddLong, long)
DEFINE_ADD_SCALAR(AddUlong, unsigned long)
DEFINE_ADD_SCALAR(AddDword, unsigned long)
DEFINE_ADD_SCALAR(AddLongLong, __int64)
DEFINE_ADD_SCALAR(AddUlongLong, unsigned __int64)
DEFINE_ADD_SCALAR(AddFloat, float)

#undef DEFINE_ADD_SCALAR

void CMsgBuffer::AddTcharArray(const char *str, unsigned int count, int zeroExtra) {
  Reserve(count);
  while (count) {
    unsigned char value = *str++;
    m_data[m_write++] = value;
    --count;
    if (zeroExtra && !value) {
      memset(m_data + m_write, 0, count);
      m_write += count;
      return;
    }
  }
}

void CMsgBuffer::AddTcharString(const char *str, int compress) {
  unsigned int length = strlen(str);
  if (length > 0x3FFF) {
    ASSERT(length <= 0x3FFF);
    length = 0x3FFF;
  }

  unsigned char prefix = static_cast<unsigned char>(length & 0x3F);
  if (length > 0x3F) {
    prefix |= 0x40;
  }
  if (!compress) {
    prefix |= 0x80;
  }
  AddByte(prefix);
  if (prefix & 0x40) {
    AddByte(static_cast<unsigned char>(length >> 8));
  }
  if (prefix & 0x80) {
    AddTcharArray(str, length, 0);
  } else {
    for (unsigned int i = 0; i < length; ++i) {
      AddByte(str[i]);
    }
  }
}

void CMsgBuffer::AddData(unsigned char *data, unsigned int count) {
  AddData(static_cast<const void *>(data), count);
}

void CMsgBuffer::AddData(const void *data, unsigned int count) {
  Reserve(count);
  memcpy(m_data + m_write, data, count);
  m_write += count;
}

#define DEFINE_ADD_ARRAY(functionName, valueType)                         \
  void CMsgBuffer::functionName(const valueType *buffer, unsigned int count) { \
    AddData(buffer, count * sizeof(valueType));                           \
  }

DEFINE_ADD_ARRAY(AddWordArray, unsigned short)
DEFINE_ADD_ARRAY(AddDwordArray, unsigned long)
DEFINE_ADD_ARRAY(AddUintArray, unsigned int)
DEFINE_ADD_ARRAY(AddFloatArray, float)

#undef DEFINE_ADD_ARRAY

#define DEFINE_GET_SCALAR(functionName, valueType) \
  valueType CMsgBuffer::functionName() {           \
    ASSERT(Bytes() >= sizeof(valueType));           \
    valueType value = *reinterpret_cast<valueType *>(m_data + m_read); \
    m_read += sizeof(valueType);                    \
    return value;                                   \
  }

DEFINE_GET_SCALAR(GetChar, char)
DEFINE_GET_SCALAR(GetUchar, unsigned char)
DEFINE_GET_SCALAR(GetByte, unsigned char)
DEFINE_GET_SCALAR(GetTchar, char)
DEFINE_GET_SCALAR(GetShort, short)
DEFINE_GET_SCALAR(GetUshort, unsigned short)
DEFINE_GET_SCALAR(GetWord, unsigned short)
DEFINE_GET_SCALAR(GetInt, int)
DEFINE_GET_SCALAR(GetUint, unsigned int)
DEFINE_GET_SCALAR(GetLong, long)
DEFINE_GET_SCALAR(GetUlong, unsigned long)
DEFINE_GET_SCALAR(GetDword, unsigned long)
DEFINE_GET_SCALAR(GetLongLong, __int64)
DEFINE_GET_SCALAR(GetUlongLong, unsigned __int64)
DEFINE_GET_SCALAR(GetFloat, float)

#undef DEFINE_GET_SCALAR

void CMsgBuffer::GetTcharArray(char *buffer, unsigned int count) {
  GetData(buffer, count);
}

unsigned int CMsgBuffer::GetTcharStringBufferLength(int *wide) {
  unsigned char prefix = GetByte();
  unsigned int length = prefix & 0x3F;
  if (prefix & 0x40) {
    length |= static_cast<unsigned int>(GetByte()) << 8;
  }
  if (wide) {
    *wide = (prefix & 0x80) == 0x80;
  }
  return length + 1;
}

void CMsgBuffer::GetTcharString(char *buffer, unsigned int bufferLength, int wide) {
  if (wide) {
    GetTcharArray(buffer, bufferLength - 1);
    buffer[bufferLength - 1] = 0;
  } else {
    unsigned int count = bufferLength - 1;
    while (count--) {
      *buffer++ = GetByte();
    }
    *buffer = 0;
  }
}

const void *CMsgBuffer::GetData(int count) {
  ASSERT(Bytes() >= count);
  const void *data = m_data + m_read;
  m_read += count;
  return data;
}

void CMsgBuffer::GetData(void *buffer, int count) {
  memcpy(buffer, GetData(count), count);
}

#define DEFINE_GET_ARRAY(functionName, valueType)                    \
  void CMsgBuffer::functionName(valueType *buffer, unsigned int count) { \
    GetData(buffer, count * sizeof(valueType));                      \
  }

DEFINE_GET_ARRAY(GetWordArray, unsigned short)
DEFINE_GET_ARRAY(GetDwordArray, unsigned long)
DEFINE_GET_ARRAY(GetUintArray, unsigned int)
DEFINE_GET_ARRAY(GetFloatArray, float)

#undef DEFINE_GET_ARRAY
