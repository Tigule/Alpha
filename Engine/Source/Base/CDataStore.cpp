#include <Base/Base.h>

#include <Base/CDataStore.h>
#include <Base/ConvertUTF.h>

#include <string.h>

void CDataStore::InternalInitialize(unsigned char *&data, unsigned int &base, unsigned int &alloc) {
}

void CDataStore::InternalDestroy(unsigned char *&data, unsigned int &base, unsigned int &alloc) {
  if (alloc && data) {
    Free(data, 0, 0);
  }

  data = 0;
  base = 0;
  alloc = 0;
}

int CDataStore::InternalFetchRead(unsigned int pos, unsigned int bytes, unsigned char *&data, unsigned int &base, unsigned int &alloc) {
  return 0;
}

int CDataStore::InternalFetchWrite(
    unsigned int    pos,
    unsigned int    bytes,
    unsigned char *&data,
    unsigned int   &base,
    unsigned int   &alloc,
    const char     *fileName,
    int             lineNumber
) {
  alloc = (pos + bytes + 0xFF) & 0xFFFFFF00;
  data = Realloc(data, alloc, fileName, lineNumber);

  return 1;
}

void CDataStore::SetSize(unsigned int size) {
  ASSERT(!IsFinal());
  if (size > m_size) {
    AssertFetchWrite(m_size, size - m_size, 0, 0);
  }
  m_size = size;
}

void CDataStore::Reserve(unsigned int bytes, const char *fileName, int lineNumber) {
  ASSERT(!IsFinal());
  if (bytes > m_alloc) {
    AssertFetchWrite(0, bytes, fileName, lineNumber);
  }
}

#define DATASTORE_SET(type)                                 \
  CDataStore &CDataStore::Set(unsigned int pos, type val) { \
    ASSERT(!IsFinal());                                     \
    ASSERT(pos + sizeof(val) <= m_size);                    \
    AssertFetchWrite(pos, sizeof(val), 0, 0);               \
    *reinterpret_cast<type *>(m_data + pos - m_base) = val; \
    return *this;                                           \
  }

DATASTORE_SET(char)
DATASTORE_SET(unsigned char)
DATASTORE_SET(short)
DATASTORE_SET(unsigned short)
DATASTORE_SET(int)
DATASTORE_SET(unsigned int)
DATASTORE_SET(long)
DATASTORE_SET(unsigned long)
DATASTORE_SET(__int64)
DATASTORE_SET(unsigned __int64)
DATASTORE_SET(float)

#undef DATASTORE_SET

#define DATASTORE_PUT(type)                                    \
  CDataStore &CDataStore::Put(type val) {                      \
    ASSERT(!IsFinal());                                        \
    AssertFetchWrite(m_size, sizeof(val), 0, 0);               \
    *reinterpret_cast<type *>(m_data + m_size - m_base) = val; \
    m_size += sizeof(val);                                     \
    return *this;                                              \
  }

DATASTORE_PUT(char)
DATASTORE_PUT(unsigned char)
DATASTORE_PUT(short)
DATASTORE_PUT(unsigned short)
DATASTORE_PUT(int)
DATASTORE_PUT(unsigned int)
DATASTORE_PUT(long)
DATASTORE_PUT(unsigned long)
DATASTORE_PUT(__int64)
DATASTORE_PUT(unsigned __int64)
DATASTORE_PUT(float)

#undef DATASTORE_PUT

CDataStore &CDataStore::PutString(const char *pval) {
  ASSERT(!IsFinal());

  FATALASSERT(pval);

  return PutArray(reinterpret_cast<const unsigned char *>(pval), SStrLen(pval) + 1);
}

CDataStore &CDataStore::PutString(const unsigned short *pval) {
  unsigned int dstChars;
  unsigned int srcChars;
  unsigned int bytes;
  unsigned int minBytes;
  int          result;

  ASSERT(!IsFinal());
  FATALASSERT(pval);

  bytes = ConvertUTF16toUTF8Length(pval, 0x7FFFFFFF, 0);
  ASSERT(static_cast<int>(bytes) > 0);
  FetchWrite(m_size, bytes, 0, 0);
  minBytes = 1;

  do {
    unsigned int copyBytes = bytes;

    if (copyBytes >= m_alloc) {
      copyBytes = m_alloc;
    }

    if (copyBytes <= minBytes) {
      copyBytes = minBytes;
    }

    AssertFetchWrite(m_size, copyBytes, 0, 0);
    result = ConvertUTF16toUTF8(reinterpret_cast<char *>(m_data + m_size - m_base), copyBytes, pval, 0x7FFFFFFF, &dstChars, &srcChars);
    ASSERT(result >= 0);

    if (!result) {
      break;
    }

    pval += srcChars;
    m_size += dstChars;
    bytes -= dstChars;
    minBytes = result;
  } while (bytes);

  return *this;
}

CDataStore &CDataStore::PutArray(const unsigned char *pval, unsigned int count) {
  unsigned int bytes;
  unsigned int copyBytes;

  ASSERT(!IsFinal());

  FATALASSERT(pval || !count);

  bytes = count;
  FetchWrite(m_size, bytes, 0, 0);

  while (bytes) {
    copyBytes = bytes;

    if (copyBytes >= m_alloc) {
      copyBytes = m_alloc;
    }

    if (copyBytes <= 1) {
      copyBytes = 1;
    }

    AssertFetchWrite(m_size, copyBytes, 0, 0);

    if (m_data + m_size - m_base != pval) {
      memcpy(m_data + m_size - m_base, pval, copyBytes);
    }

    pval += copyBytes;
    m_size += copyBytes;
    bytes -= copyBytes;
  }

  return *this;
}

#define DATASTORE_PUT_ARRAY(type, elementSize)                                                      \
  CDataStore &CDataStore::PutArray(const type *pval, unsigned int count) {                          \
    unsigned int bytes;                                                                             \
    ASSERT(!IsFinal());                                                                             \
    FATALASSERT(pval || !count);                                                                    \
    bytes = count * elementSize;                                                                    \
    FetchWrite(m_size, bytes, 0, 0);                                                                \
    while (bytes) {                                                                                 \
      count = bytes;                                                                                \
      if (count >= m_alloc) {                                                                       \
        count = m_alloc;                                                                            \
      }                                                                                             \
      if (count <= elementSize) {                                                                   \
        count = elementSize;                                                                        \
      }                                                                                             \
      count &= ~(elementSize - 1);                                                                  \
      AssertFetchWrite(m_size, count, 0, 0);                                                        \
      if (m_data + m_size - m_base != reinterpret_cast<const unsigned char *>(pval)) {              \
        memcpy(m_data + m_size - m_base, pval, count);                                              \
      }                                                                                             \
      pval = reinterpret_cast<const type *>(reinterpret_cast<const unsigned char *>(pval) + count); \
      m_size += count;                                                                              \
      bytes -= count;                                                                               \
    }                                                                                               \
    return *this;                                                                                   \
  }

DATASTORE_PUT_ARRAY(unsigned short, 2)
DATASTORE_PUT_ARRAY(unsigned long, 4)
DATASTORE_PUT_ARRAY(unsigned __int64, 8)
DATASTORE_PUT_ARRAY(float, 4)
DATASTORE_PUT_ARRAY(unreal, 4)

#undef DATASTORE_PUT_ARRAY

CDataStore &CDataStore::PutData(const void *pval, unsigned int bytes) {
  return PutArray(static_cast<const unsigned char *>(pval), bytes);
}

#define DATASTORE_GET(type)                                      \
  CDataStore &CDataStore::Get(type &val) {                       \
    ASSERT(IsFinal());                                           \
    if (FetchRead(m_read, sizeof(val))) {                        \
      val = *reinterpret_cast<type *>(m_data + m_read - m_base); \
      m_read += sizeof(val);                                     \
    }                                                            \
    return *this;                                                \
  }

DATASTORE_GET(char)
DATASTORE_GET(unsigned char)
DATASTORE_GET(short)
DATASTORE_GET(unsigned short)
DATASTORE_GET(int)
DATASTORE_GET(unsigned int)
DATASTORE_GET(long)
DATASTORE_GET(unsigned long)
DATASTORE_GET(__int64)
DATASTORE_GET(unsigned __int64)
DATASTORE_GET(float)

#undef DATASTORE_GET
CDataStore &CDataStore::GetString(char *pval, unsigned int maxChars) {
  unsigned int length;
  unsigned int copyBytes;

  ASSERT(IsFinal());

  FATALASSERT(pval || !maxChars);

  if (pval && maxChars) {
    if (IsValid()) {
      length = 0;

      for (;;) {
        if (!FetchRead(m_read, 1)) {
          break;
        }

        copyBytes = m_base + m_alloc;

        if (copyBytes >= m_size) {
          copyBytes = m_size;
        }

        copyBytes -= m_read;

        if (copyBytes >= maxChars - length) {
          copyBytes = maxChars - length;
        }

        while (copyBytes && (pval[length++] = m_data[m_read++ - m_base])) {
          --copyBytes;
        }

        if (copyBytes) {
          break;
        }

        if (length >= maxChars) {
          Seek(m_size + 1);
          break;
        }
      }
    }

    if (!IsValid()) {
      pval[0] = 0;
    }
  }

  return *this;
}

CDataStore &CDataStore::GetString(unsigned short *pval, unsigned int maxChars) {
  unsigned int dstChars;
  unsigned int srcChars;
  unsigned int peek;
  unsigned int length;
  int          result;

  ASSERT(IsFinal());
  FATALASSERT(pval || !maxChars);

  if (pval && maxChars) {
    if (IsValid()) {
      length = 0;
      peek = 1;

      for (;;) {
        if (!FetchRead(m_read, peek)) {
          break;
        }

        unsigned int bytes = m_base + m_alloc;

        if (bytes >= m_size) {
          bytes = m_size;
        }

        bytes -= m_read;
        result = ConvertUTF8toUTF16(
            pval + length, maxChars - length, reinterpret_cast<const char *>(m_data + m_read - m_base), bytes, &dstChars, &srcChars
        );
        if (result > 0) {
          Seek(m_size + 1);
          break;
        }

        m_read += srcChars;

        if (!result) {
          break;
        }

        length += dstChars;
        peek = -result;
      }
    }

    if (!IsValid()) {
      pval[0] = 0;
    }
  }

  return *this;
}

CDataStore &CDataStore::GetArray(unsigned char *pval, unsigned int count) {
  unsigned int bytes;

  ASSERT(IsFinal());

  FATALASSERT(pval || !count);

  if (m_read > m_size) {
    return *this;
  }

  bytes = count;

  while (bytes) {
    count = m_size - m_read;

    if (count >= bytes) {
      count = bytes;
    }

    if (count >= m_alloc) {
      count = m_alloc;
    }

    if (count <= 1) {
      count = 1;
    }

    if (!FetchRead(m_read, count)) {
      return *this;
    }

    if (pval != m_data + m_read - m_base) {
      memcpy(pval, m_data + m_read - m_base, count);
    }

    m_read += count;
    pval += count;
    bytes -= count;
  }

  return *this;
}

#define DATASTORE_GET_ARRAY(type, elementSize)                                          \
  CDataStore &CDataStore::GetArray(type *pval, unsigned int count) {                    \
    unsigned int bytes;                                                                 \
    ASSERT(IsFinal());                                                                  \
    FATALASSERT(pval || !count);                                                        \
    if (m_read > m_size) {                                                              \
      return *this;                                                                     \
    }                                                                                   \
    bytes = count * elementSize;                                                        \
    while (bytes) {                                                                     \
      count = m_size - m_read;                                                          \
      if (count >= bytes) {                                                             \
        count = bytes;                                                                  \
      }                                                                                 \
      if (count >= m_alloc) {                                                           \
        count = m_alloc;                                                                \
      }                                                                                 \
      if (count <= elementSize) {                                                       \
        count = elementSize;                                                            \
      }                                                                                 \
      count &= ~(elementSize - 1);                                                      \
      if (!FetchRead(m_read, count)) {                                                  \
        return *this;                                                                   \
      }                                                                                 \
      if (reinterpret_cast<unsigned char *>(pval) != m_data + m_read - m_base) {        \
        memcpy(pval, m_data + m_read - m_base, count);                                  \
      }                                                                                 \
      m_read += count;                                                                  \
      pval = reinterpret_cast<type *>(reinterpret_cast<unsigned char *>(pval) + count); \
      bytes -= count;                                                                   \
    }                                                                                   \
    return *this;                                                                       \
  }

DATASTORE_GET_ARRAY(unsigned short, 2)
DATASTORE_GET_ARRAY(unsigned long, 4)
DATASTORE_GET_ARRAY(unsigned __int64, 8)
DATASTORE_GET_ARRAY(float, 4)
DATASTORE_GET_ARRAY(unreal, 4)

#undef DATASTORE_GET_ARRAY

CDataStore &CDataStore::GetData(void *pval, unsigned int bytes) {
  return GetArray(static_cast<unsigned char *>(pval), bytes);
}

CDataStore &CDataStore::GetDataInSitu(void *&pval, unsigned int bytes) {
  pval = 0;

  if (!FetchRead(m_read, bytes)) {
    return *this;
  }

  pval = m_data + m_read - m_base;
  m_read += bytes;

  return *this;
}

void CDataStore::GetBufferParams(const void **data, unsigned int *size, unsigned int *alloc) const {
  if (data) {
    *data = m_data;
  }

  if (size) {
    *size = m_size;
  }

  if (alloc) {
    *alloc = m_alloc;
  }
}

void CDataStore::DetachBuffer(void **data, unsigned int *size, unsigned int *alloc) {
  if (data) {
    *data = m_data;
  }

  if (size) {
    *size = m_size;
  }

  if (alloc) {
    *alloc = m_alloc;
  }

  m_data = 0;
  m_alloc = 0;
  Reset();
}
