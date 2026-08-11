#include <Base/Base.h>

#include <Base/CDataStore.h>
#include <Base/ConvertUTF.h>

#include <string.h>

void CDataStore::InternalInitialize(BYTE *&data, UINT &base, UINT &alloc) {
}

void CDataStore::InternalDestroy(BYTE *&data, UINT &base, UINT &alloc) {
  if (alloc && data) {
    Free(data, 0, 0);
  }

  data = 0;
  base = 0;
  alloc = 0;
}

BOOL CDataStore::InternalFetchRead(UINT pos, UINT bytes, BYTE *&data, UINT &base, UINT &alloc) {
  return 0;
}

BOOL CDataStore::InternalFetchWrite(UINT pos, UINT bytes, BYTE *&data, UINT &base, UINT &alloc, LPCSTR fileName, int lineNumber) {
  alloc = (pos + bytes + 0xFF) & 0xFFFFFF00;
  data = Realloc(data, alloc, fileName, lineNumber);

  return 1;
}

#define DATASTORE_SET(type)                                 \
  CDataStore &CDataStore::Set(UINT pos, type val) {         \
    ASSERT(!IsFinal());                                     \
    ASSERT(pos + sizeof(val) <= m_size);                    \
    AssertFetchWrite(pos, sizeof(val), 0, 0);               \
    *reinterpret_cast<type *>(m_data + pos - m_base) = val; \
    return *this;                                           \
  }

DATASTORE_SET(char)
DATASTORE_SET(BYTE)
DATASTORE_SET(short)
DATASTORE_SET(WORD)
DATASTORE_SET(int)
DATASTORE_SET(UINT)
DATASTORE_SET(long)
DATASTORE_SET(DWORD)
DATASTORE_SET(LONGLONG)
DATASTORE_SET(DWORDLONG)
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
DATASTORE_PUT(BYTE)
DATASTORE_PUT(short)
DATASTORE_PUT(WORD)
DATASTORE_PUT(int)
DATASTORE_PUT(UINT)
DATASTORE_PUT(long)
DATASTORE_PUT(DWORD)
DATASTORE_PUT(LONGLONG)
DATASTORE_PUT(DWORDLONG)
DATASTORE_PUT(float)

#undef DATASTORE_PUT

CDataStore &CDataStore::PutString(LPCSTR pval) {
  ASSERT(!IsFinal());

  FATALASSERT(pval);

  return PutArray(reinterpret_cast<const BYTE *>(pval), SStrLen(pval) + 1);
}

CDataStore &CDataStore::PutString(const WORD *pval) {
  UINT dstChars;
  UINT srcChars;
  UINT bytes;
  UINT minBytes;
  int  result;

  ASSERT(!IsFinal());
  FATALASSERT(pval);

  bytes = ConvertUTF16toUTF8Length(pval, 0x7FFFFFFF, 0);
  ASSERT(static_cast<int>(bytes) > 0);
  FetchWrite(m_size, bytes, 0, 0);
  minBytes = 1;

  do {
    UINT copyBytes = bytes;

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

CDataStore &CDataStore::PutArray(const BYTE *pval, UINT count) {
  UINT bytes;
  UINT copyBytes;

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

#define DATASTORE_PUT_ARRAY(type, elementSize)                                             \
  CDataStore &CDataStore::PutArray(const type *pval, UINT count) {                         \
    UINT bytes;                                                                            \
    ASSERT(!IsFinal());                                                                    \
    FATALASSERT(pval || !count);                                                           \
    bytes = count * elementSize;                                                           \
    FetchWrite(m_size, bytes, 0, 0);                                                       \
    while (bytes) {                                                                        \
      count = bytes;                                                                       \
      if (count >= m_alloc) {                                                              \
        count = m_alloc;                                                                   \
      }                                                                                    \
      if (count <= elementSize) {                                                          \
        count = elementSize;                                                               \
      }                                                                                    \
      count &= ~(elementSize - 1);                                                         \
      AssertFetchWrite(m_size, count, 0, 0);                                               \
      if (m_data + m_size - m_base != reinterpret_cast<const BYTE *>(pval)) {              \
        memcpy(m_data + m_size - m_base, pval, count);                                     \
      }                                                                                    \
      pval = reinterpret_cast<const type *>(reinterpret_cast<const BYTE *>(pval) + count); \
      m_size += count;                                                                     \
      bytes -= count;                                                                      \
    }                                                                                      \
    return *this;                                                                          \
  }

DATASTORE_PUT_ARRAY(WORD, 2)
DATASTORE_PUT_ARRAY(DWORD, 4)
DATASTORE_PUT_ARRAY(DWORDLONG, 8)
DATASTORE_PUT_ARRAY(float, 4)
DATASTORE_PUT_ARRAY(unreal, 4)

#undef DATASTORE_PUT_ARRAY

CDataStore &CDataStore::PutData(LPCVOID pval, UINT bytes) {
  return PutArray(static_cast<const BYTE *>(pval), bytes);
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
DATASTORE_GET(BYTE)
DATASTORE_GET(short)
DATASTORE_GET(WORD)
DATASTORE_GET(int)
DATASTORE_GET(UINT)
DATASTORE_GET(long)
DATASTORE_GET(DWORD)
DATASTORE_GET(LONGLONG)
DATASTORE_GET(DWORDLONG)
DATASTORE_GET(float)

#undef DATASTORE_GET
CDataStore &CDataStore::GetString(char *pval, UINT maxChars) {
  UINT length;
  UINT copyBytes;

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

CDataStore &CDataStore::GetString(WORD *pval, UINT maxChars) {
  UINT dstChars;
  UINT srcChars;
  UINT peek;
  UINT length;
  int  result;

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

        UINT bytes = m_base + m_alloc;

        if (bytes >= m_size) {
          bytes = m_size;
        }

        bytes -= m_read;
        result =
            ConvertUTF8toUTF16(pval + length, maxChars - length, reinterpret_cast<LPCSTR>(m_data + m_read - m_base), bytes, &dstChars, &srcChars);
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

CDataStore &CDataStore::GetArray(BYTE *pval, UINT count) {
  UINT bytes;

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

#define DATASTORE_GET_ARRAY(type, elementSize)                                 \
  CDataStore &CDataStore::GetArray(type *pval, UINT count) {                   \
    UINT bytes;                                                                \
    ASSERT(IsFinal());                                                         \
    FATALASSERT(pval || !count);                                               \
    if (m_read > m_size) {                                                     \
      return *this;                                                            \
    }                                                                          \
    bytes = count * elementSize;                                               \
    while (bytes) {                                                            \
      count = m_size - m_read;                                                 \
      if (count >= bytes) {                                                    \
        count = bytes;                                                         \
      }                                                                        \
      if (count >= m_alloc) {                                                  \
        count = m_alloc;                                                       \
      }                                                                        \
      if (count <= elementSize) {                                              \
        count = elementSize;                                                   \
      }                                                                        \
      count &= ~(elementSize - 1);                                             \
      if (!FetchRead(m_read, count)) {                                         \
        return *this;                                                          \
      }                                                                        \
      if (reinterpret_cast<BYTE *>(pval) != m_data + m_read - m_base) {        \
        memcpy(pval, m_data + m_read - m_base, count);                         \
      }                                                                        \
      m_read += count;                                                         \
      pval = reinterpret_cast<type *>(reinterpret_cast<BYTE *>(pval) + count); \
      bytes -= count;                                                          \
    }                                                                          \
    return *this;                                                              \
  }

DATASTORE_GET_ARRAY(WORD, 2)
DATASTORE_GET_ARRAY(DWORD, 4)
DATASTORE_GET_ARRAY(DWORDLONG, 8)
DATASTORE_GET_ARRAY(float, 4)
DATASTORE_GET_ARRAY(unreal, 4)

#undef DATASTORE_GET_ARRAY

CDataStore &CDataStore::GetData(LPVOID pval, UINT bytes) {
  return GetArray(static_cast<BYTE *>(pval), bytes);
}

CDataStore &CDataStore::GetDataInSitu(LPVOID &pval, UINT bytes) {
  pval = 0;

  if (!FetchRead(m_read, bytes)) {
    return *this;
  }

  pval = m_data + m_read - m_base;
  m_read += bytes;

  return *this;
}

void CDataStore::GetBufferParams(LPCVOID *data, UINT *size, UINT *alloc) const {
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

void CDataStore::DetachBuffer(LPVOID *data, UINT *size, UINT *alloc) {
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
