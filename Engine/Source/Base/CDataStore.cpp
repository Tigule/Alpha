#include <Base/Base.h>

#include <Base/CDataStore.h>
#include <Base/CUnreal.h>
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

  if (!pval) {
    FATALERROR(("pval"));
    SErrSetLastError(ERROR_INVALID_PARAMETER);
    return *this;
  }

  PutArray(reinterpret_cast<const BYTE *>(pval), SStrLen(pval) + 1);
  return *this;
}

CDataStore &CDataStore::PutString(const WORD *pval) {
  UINT dstChars;
  UINT srcChars;
  UINT bytes;
  UINT minBytes;
  int  result;

  ASSERT(!IsFinal());

  if (!pval) {
    FATALERROR(("pval"));
    SErrSetLastError(ERROR_INVALID_PARAMETER);
    return *this;
  }

  bytes = ConvertUTF16toUTF8Length(pval, 0x7FFFFFFF, 0);
  ASSERT((int)bytes > 0);
  FetchWrite(m_size, bytes, 0, 0);
  minBytes = 1;

  for (;;) {
    UINT copyBytes = min(bytes, m_alloc);
    copyBytes = max(copyBytes, minBytes);

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
  }

  return *this;
}

CDataStore &CDataStore::PutArray(const BYTE *pval, UINT count) {
  UINT bytes;
  UINT copyBytes;

  ASSERT(!IsFinal());

  if (!(pval || !count)) {
    FATALERROR(("pval || !count"));
    SErrSetLastError(ERROR_INVALID_PARAMETER);
    return *this;
  }

  if (pval) {
    bytes = count;
    FetchWrite(m_size, bytes, 0, 0);

    while (bytes) {
      copyBytes = min(bytes, m_alloc);
      copyBytes = max(copyBytes, 1);

      AssertFetchWrite(m_size, copyBytes, 0, 0);

      BYTE *dst = m_data + m_size - m_base;
      if (dst != pval) {
        memcpy(dst, pval, copyBytes);
      }

      pval += copyBytes;
      m_size += copyBytes;
      bytes -= copyBytes;
    }
  }

  return *this;
}

CDataStore &CDataStore::PutArray(const WORD *pval, UINT count) {
  ASSERT(!IsFinal());

  if (!(pval || !count)) {
    FATALERROR(("pval || !count"));
    SErrSetLastError(ERROR_INVALID_PARAMETER);
    return *this;
  }

  if (pval) {
    UINT bytes = count * sizeof(WORD);
    FetchWrite(m_size, bytes, 0, 0);

    while (bytes) {
      UINT copyBytes = min(bytes, m_alloc);
      copyBytes = max(copyBytes, sizeof(WORD));
      copyBytes &= ~(sizeof(WORD) - 1);

      AssertFetchWrite(m_size, copyBytes, 0, 0);

      count = copyBytes / sizeof(WORD);
      memcpy(m_data + m_size - m_base, pval, count * sizeof(WORD));
      pval += count;
      m_size += copyBytes;
      bytes -= copyBytes;
    }
  }

  return *this;
}

CDataStore &CDataStore::PutArray(const DWORD *pval, UINT count) {
  ASSERT(!IsFinal());

  if (!(pval || !count)) {
    FATALERROR(("pval || !count"));
    SErrSetLastError(ERROR_INVALID_PARAMETER);
    return *this;
  }

  if (pval) {
    UINT bytes = count * sizeof(DWORD);
    FetchWrite(m_size, bytes, 0, 0);

    while (bytes) {
      UINT copyBytes = min(bytes, m_alloc);
      copyBytes = max(copyBytes, sizeof(DWORD));
      copyBytes &= ~(sizeof(DWORD) - 1);

      AssertFetchWrite(m_size, copyBytes, 0, 0);

      count = copyBytes / sizeof(DWORD);
      memcpy(m_data + m_size - m_base, pval, count * sizeof(DWORD));
      pval += count;
      m_size += copyBytes;
      bytes -= copyBytes;
    }
  }

  return *this;
}

CDataStore &CDataStore::PutArray(const DWORDLONG *pval, UINT count) {
  ASSERT(!IsFinal());

  if (!(pval || !count)) {
    FATALERROR(("pval || !count"));
    SErrSetLastError(ERROR_INVALID_PARAMETER);
    return *this;
  }

  if (pval) {
    UINT bytes = count * sizeof(DWORDLONG);
    FetchWrite(m_size, bytes, 0, 0);

    while (bytes) {
      UINT copyBytes = min(bytes, m_alloc);
      copyBytes = max(copyBytes, sizeof(DWORDLONG));
      copyBytes &= ~(sizeof(DWORDLONG) - 1);

      AssertFetchWrite(m_size, copyBytes, 0, 0);

      count = copyBytes / sizeof(DWORDLONG);
      memcpy(m_data + m_size - m_base, pval, count * sizeof(DWORDLONG));
      pval += count;
      m_size += copyBytes;
      bytes -= copyBytes;
    }
  }

  return *this;
}

CDataStore &CDataStore::PutArray(const float *pval, UINT count) {
  ASSERT(!IsFinal());

  if (!(pval || !count)) {
    FATALERROR(("pval || !count"));
    SErrSetLastError(ERROR_INVALID_PARAMETER);
    return *this;
  }

  if (pval) {
    UINT bytes = count * sizeof(float);
    FetchWrite(m_size, bytes, 0, 0);

    while (bytes) {
      UINT copyBytes = min(bytes, m_alloc);
      copyBytes = max(copyBytes, sizeof(float));
      copyBytes &= ~(sizeof(float) - 1);

      AssertFetchWrite(m_size, copyBytes, 0, 0);

      count = copyBytes / sizeof(float);
      memcpy(m_data + m_size - m_base, pval, count * sizeof(float));
      pval += count;
      m_size += copyBytes;
      bytes -= copyBytes;
    }
  }

  return *this;
}

CDataStore &CDataStore::PutArray(const unreal *pval, UINT count) {
  ASSERT(!IsFinal());

  if (!(pval || !count)) {
    FATALERROR(("pval || !count"));
    SErrSetLastError(ERROR_INVALID_PARAMETER);
    return *this;
  }

  if (pval) {
    UINT bytes = count * sizeof(unreal);
    FetchWrite(m_size, bytes, 0, 0);

    while (bytes) {
      UINT copyBytes = min(bytes, m_alloc);
      copyBytes = max(copyBytes, sizeof(unreal));
      copyBytes &= ~(sizeof(unreal) - 1);

      AssertFetchWrite(m_size, copyBytes, 0, 0);

      count = copyBytes / sizeof(unreal);
      memcpy(m_data + m_size - m_base, pval, count * sizeof(unreal));
      pval += count;
      m_size += copyBytes;
      bytes -= copyBytes;
    }
  }

  return *this;
}

CDataStore &CDataStore::PutData(LPCVOID pval, UINT bytes) {
  return PutArray(static_cast<const BYTE *>(pval), bytes);
}

#define DATASTORE_GET(type)                                      \
  CDataStore &CDataStore::Get(type &val) {                       \
    ASSERT(IsFinal());                                           \
    if (FetchRead(m_read, sizeof(val))) {                        \
      val = *reinterpret_cast<type *>(m_data - m_base + m_read); \
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

  if (!(pval || !maxChars)) {
    FATALERROR(("pval || !maxChars"));
    SErrSetLastError(ERROR_INVALID_PARAMETER);
    return *this;
  }

  if (pval && maxChars) {
    if (IsValid()) {
      length = 0;

      for (;;) {
        if (!FetchRead(m_read, 1)) {
          break;
        }

        copyBytes = m_alloc + m_base;
        copyBytes = min(copyBytes, m_size) - m_read;
        copyBytes = min(copyBytes, maxChars - length);

        const char *src = reinterpret_cast<const char *>(m_data - m_base + m_read);
        UINT        i = 0;

        while (copyBytes && (pval[length++] = src[i++])) {
          --copyBytes;
        }

        m_read += i;

        if (copyBytes) {
          break;
        }

        if (length >= maxChars) {
          Invalidate();
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

  if (!(pval || !maxChars)) {
    FATALERROR(("pval || !maxChars"));
    SErrSetLastError(ERROR_INVALID_PARAMETER);
    return *this;
  }

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
          Invalidate();
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

  if (!(pval || !count)) {
    FATALERROR(("pval || !count"));
    SErrSetLastError(ERROR_INVALID_PARAMETER);
    return *this;
  }

  if (IsValid()) {
    bytes = count;

    while (bytes) {
      UINT copyBytes = m_size - m_read;
      copyBytes = min(copyBytes, bytes);
      copyBytes = min(copyBytes, m_alloc);
      copyBytes = max(copyBytes, 1);

      if (!FetchRead(m_read, copyBytes)) {
        break;
      }

      BYTE *src = m_data - m_base + m_read;
      if (pval != src) {
        memcpy(pval, src, copyBytes);
      }

      pval += copyBytes;
      m_read += copyBytes;
      bytes -= copyBytes;
    }
  }

  return *this;
}

CDataStore &CDataStore::GetArray(WORD *pval, UINT count) {
  ASSERT(IsFinal());

  if (!(pval || !count)) {
    FATALERROR(("pval || !count"));
    SErrSetLastError(ERROR_INVALID_PARAMETER);
    return *this;
  }

  if (IsValid()) {
    UINT bytes = count * sizeof(WORD);

    while (bytes) {
      UINT copyBytes = m_size - m_read;
      copyBytes = min(copyBytes, bytes);
      copyBytes = min(copyBytes, m_alloc);
      copyBytes = max(copyBytes, sizeof(WORD));
      copyBytes &= ~(sizeof(WORD) - 1);

      if (!FetchRead(m_read, copyBytes)) {
        break;
      }

      count = copyBytes / sizeof(WORD);
      memcpy(pval, m_data - m_base + m_read, count * sizeof(WORD));
      pval += count;
      m_read += copyBytes;
      bytes -= copyBytes;
    }
  }

  return *this;
}

CDataStore &CDataStore::GetArray(DWORD *pval, UINT count) {
  ASSERT(IsFinal());

  if (!(pval || !count)) {
    FATALERROR(("pval || !count"));
    SErrSetLastError(ERROR_INVALID_PARAMETER);
    return *this;
  }

  if (IsValid()) {
    UINT bytes = count * sizeof(DWORD);

    while (bytes) {
      UINT copyBytes = m_size - m_read;
      copyBytes = min(copyBytes, bytes);
      copyBytes = min(copyBytes, m_alloc);
      copyBytes = max(copyBytes, sizeof(DWORD));
      copyBytes &= ~(sizeof(DWORD) - 1);

      if (!FetchRead(m_read, copyBytes)) {
        break;
      }

      count = copyBytes / sizeof(DWORD);
      memcpy(pval, m_data - m_base + m_read, count * sizeof(DWORD));
      pval += count;
      m_read += copyBytes;
      bytes -= copyBytes;
    }
  }

  return *this;
}

CDataStore &CDataStore::GetArray(DWORDLONG *pval, UINT count) {
  ASSERT(IsFinal());

  if (!(pval || !count)) {
    FATALERROR(("pval || !count"));
    SErrSetLastError(ERROR_INVALID_PARAMETER);
    return *this;
  }

  if (IsValid()) {
    UINT bytes = count * sizeof(DWORDLONG);

    while (bytes) {
      UINT copyBytes = m_size - m_read;
      copyBytes = min(copyBytes, bytes);
      copyBytes = min(copyBytes, m_alloc);
      copyBytes = max(copyBytes, sizeof(DWORDLONG));
      copyBytes &= ~(sizeof(DWORDLONG) - 1);

      if (!FetchRead(m_read, copyBytes)) {
        break;
      }

      count = copyBytes / sizeof(DWORDLONG);
      memcpy(pval, m_data - m_base + m_read, count * sizeof(DWORDLONG));
      pval += count;
      m_read += copyBytes;
      bytes -= copyBytes;
    }
  }

  return *this;
}

CDataStore &CDataStore::GetArray(float *pval, UINT count) {
  ASSERT(IsFinal());

  if (!(pval || !count)) {
    FATALERROR(("pval || !count"));
    SErrSetLastError(ERROR_INVALID_PARAMETER);
    return *this;
  }

  if (IsValid()) {
    UINT bytes = count * sizeof(float);

    while (bytes) {
      UINT copyBytes = m_size - m_read;
      copyBytes = min(copyBytes, bytes);
      copyBytes = min(copyBytes, m_alloc);
      copyBytes = max(copyBytes, sizeof(float));
      copyBytes &= ~(sizeof(float) - 1);

      if (!FetchRead(m_read, copyBytes)) {
        break;
      }

      count = copyBytes / sizeof(float);
      memcpy(pval, m_data - m_base + m_read, count * sizeof(float));
      pval += count;
      m_read += copyBytes;
      bytes -= copyBytes;
    }
  }

  return *this;
}

CDataStore &CDataStore::GetArray(unreal *pval, UINT count) {
  ASSERT(IsFinal());

  if (!(pval || !count)) {
    FATALERROR(("pval || !count"));
    SErrSetLastError(ERROR_INVALID_PARAMETER);
    return *this;
  }

  if (IsValid()) {
    UINT bytes = count * sizeof(unreal);

    while (bytes) {
      UINT copyBytes = m_size - m_read;
      copyBytes = min(copyBytes, bytes);
      copyBytes = min(copyBytes, m_alloc);
      copyBytes = max(copyBytes, sizeof(unreal));
      copyBytes &= ~(sizeof(unreal) - 1);

      if (!FetchRead(m_read, copyBytes)) {
        break;
      }

      count = copyBytes / sizeof(unreal);
      memcpy(pval, m_data - m_base + m_read, count * sizeof(unreal));
      pval += count;
      m_read += copyBytes;
      bytes -= copyBytes;
    }
  }

  return *this;
}

CDataStore &CDataStore::GetData(LPVOID pval, UINT bytes) {
  return GetArray(static_cast<BYTE *>(pval), bytes);
}

CDataStore &CDataStore::GetDataInSitu(LPVOID &pval, UINT bytes) {
  pval = 0;

  if (FetchRead(m_read, bytes)) {
    pval = m_data - m_base + m_read;
    m_read += bytes;
  }

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
