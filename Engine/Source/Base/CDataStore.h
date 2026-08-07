#ifndef ENGINE_SOURCE_BASE_CDATASTORE_H
#define ENGINE_SOURCE_BASE_CDATASTORE_H

#include <storm.h>

class unreal;

class CDataStore {
 public:
  template <class T>
  struct Space {
    UINT m_pos;

    Space();
    UINT GetDifferenceInclusive(const CDataStore &) const;
    UINT GetDifferenceExclusive(const CDataStore &) const;
    void Set(CDataStore &, T);
    void SetDifferenceInclusive(CDataStore &);
    void SetDifferenceExclusive(CDataStore &);
  };

  template <class T, UINT MAXSIZE>
  struct FixedString {
    T m_data[MAXSIZE];

    FixedString();
    FixedString(const T *);
    FixedString &operator=(const T *);
    UINT         MaxSize() const;
    int          Compare(const T *) const;
    int          CompareI(const T *) const;
    void         Copy(const T *);
    void         Reset();
  };

  template <class SIZET, UINT MAXSIZE>
  struct FixedBuffer {
    SIZET m_size;
    BYTE  m_data[MAXSIZE];

    FixedBuffer();
    FixedBuffer(const FixedBuffer &);
    FixedBuffer &operator=(const FixedBuffer &);
    FixedBuffer &operator=(const CDataStore &);
                 operator LPVOID();
                 operator LPCVOID() const;
    UINT         MaxSize() const;
    UINT         Size() const;
    UINT         MaxBytes() const;
    UINT         Bytes() const;
    int          Compare(const FixedBuffer &) const;
    void         Copy(const FixedBuffer &);
    void         Copy(LPCVOID, SIZET);
    void         Copy(const CDataStore &);
    void         Reset();
  };

  template <class SIZET, class T, UINT MAXSIZE>
  struct FixedArray {
    SIZET m_size;
    T     m_data[MAXSIZE];

    FixedArray();
    FixedArray(const FixedArray &);
    FixedArray &operator=(const FixedArray &);
    UINT        MaxSize() const;
    UINT        Size() const;
    UINT        MaxBytes() const;
    UINT        Bytes() const;
    void        Copy(const FixedArray &);
    void        Copy(const T *, SIZET);
    void        Reset();
    void        PutFast(CDataStore &) const;
    void        GetFast(CDataStore &);
  };

  static BYTE *Alloc(UINT bytes, LPCSTR fileName, int lineNumber) {
    if (!bytes) {
      return 0;
    }

    if (!fileName) {
      fileName = __FILE__;
      lineNumber = __LINE__;
    }

    return static_cast<BYTE *>(SMemAlloc(bytes, fileName, lineNumber, 0));
  }

  static void Free(BYTE *data, LPCSTR fileName, int lineNumber) {
    if (!fileName) {
      fileName = __FILE__;
      lineNumber = __LINE__;
    }

    SMemFree(data, fileName, lineNumber, 0);
  }

  static BYTE *Realloc(BYTE *data, UINT bytes, LPCSTR fileName, int lineNumber) {
    if (!fileName) {
      fileName = __FILE__;
      lineNumber = __LINE__;
    }

    return static_cast<BYTE *>(SMemReAlloc(data, bytes, fileName, lineNumber, 0));
  }

 protected:
  virtual void InternalInitialize(BYTE *&data, UINT &base, UINT &alloc);
  virtual void InternalDestroy(BYTE *&data, UINT &base, UINT &alloc);
  virtual BOOL InternalFetchRead(UINT pos, UINT bytes, BYTE *&data, UINT &base, UINT &alloc);
  virtual BOOL InternalFetchWrite(UINT pos, UINT bytes, BYTE *&data, UINT &base, UINT &alloc, LPCSTR fileName, int lineNumber);

  void Initialize() {
    if (m_alloc != static_cast<UINT>(-1)) {
      InternalInitialize(m_data, m_base, m_alloc);
    }
  }

  void Destroy() {
    if (m_alloc != static_cast<UINT>(-1)) {
      InternalDestroy(m_data, m_base, m_alloc);
    }
  }

  BOOL FetchRead(UINT pos, UINT bytes) {
    if (pos + bytes > m_size) {
      Seek(m_size + 1);
      return 0;
    }

    if (pos < m_base || pos + bytes > m_base + m_alloc) {
      if (!InternalFetchRead(pos, bytes, m_data, m_base, m_alloc)) {
        Seek(m_size + 1);
        return 0;
      }

      ASSERT(pos >= m_base);
      ASSERT(pos + bytes <= m_base + m_alloc);
    }

    return 1;
  }

  BOOL FetchWrite(UINT pos, UINT bytes, LPCSTR fileName, int lineNumber) {
    if (pos < m_base || pos + bytes > m_base + m_alloc) {
      if (!InternalFetchWrite(pos, bytes, m_data, m_base, m_alloc, fileName, lineNumber)) {
        return 0;
      }

      ASSERT(pos >= m_base);
      ASSERT(pos + bytes <= m_base + m_alloc);
    }

    return 1;
  }

  void AssertFetchWrite(UINT pos, UINT bytes, LPCSTR fileName, int lineNumber) {
    if (!FetchWrite(pos, bytes, fileName, lineNumber)) {
      FATALERROR(("CDataStore::AssertFetchWrite(%u, %u) failed", pos, bytes));
    }
  }

  void PutSpace(UINT bytes);

 public:
  CDataStore() : m_data(0), m_base(0), m_alloc(0), m_size(0), m_read(-1) {
  }

  CDataStore(BYTE *data, UINT size) : m_data(data), m_base(0), m_alloc(-1), m_size(size), m_read(0) {
  }

  CDataStore(BYTE *data, UINT size, UINT read);
  CDataStore(const CDataStore &store);
  virtual ~CDataStore() {
    Destroy();
  }

  BOOL IsFinal() const {
    return m_read != static_cast<UINT>(-1);
  }

  BOOL IsValid() const {
    return m_read <= m_size;
  }

  BOOL IsReadOnly() const {
    return m_alloc == static_cast<UINT>(-1);
  }

  operator LPVOID() const;

  int operator!() const {
    return !IsValid();
  }

  void Unfinalize() {
    m_read = static_cast<UINT>(-1);
  }

  void Invalidate() {
    m_read = m_size + 1;
  }

  virtual BOOL IsRead() const;
  virtual void Reset();
  virtual void Finalize() {
    ASSERT(!IsFinal());
    m_read = 0;
  }

  void Seek(UINT pos) {
    ASSERT(IsFinal());
    m_read = pos;
  }

  UINT Tell() const {
    ASSERT(IsFinal());
    return m_read;
  }

  UINT Size() const {
    return m_size;
  }

  void SetSize(UINT size);
  void Reserve(UINT bytes, LPCSTR fileName = 0, int lineNumber = 0);

  virtual void GetBufferParams(LPCVOID *data, UINT *size, UINT *alloc) const;
  virtual void DetachBuffer(LPVOID *data, UINT *size, UINT *alloc);

  CDataStore &Set(UINT pos, char val);
  CDataStore &Set(UINT pos, BYTE val);
  CDataStore &Set(UINT pos, short val);
  CDataStore &Set(UINT pos, WORD val);
  CDataStore &Set(UINT pos, int val);
  CDataStore &Set(UINT pos, UINT val);
  CDataStore &Set(UINT pos, long val);
  CDataStore &Set(UINT pos, DWORD val);
  CDataStore &Set(UINT pos, LONGLONG val);
  CDataStore &Set(UINT pos, DWORDLONG val);
  CDataStore &Set(UINT pos, float val);

  CDataStore &Put(char val);
  CDataStore &Put(BYTE val);
  CDataStore &Put(short val);
  CDataStore &Put(WORD val);
  CDataStore &Put(int val);
  CDataStore &Put(UINT val);
  CDataStore &Put(long val);
  CDataStore &Put(DWORD val);
  CDataStore &Put(LONGLONG val);
  CDataStore &Put(DWORDLONG val);
  CDataStore &Put(float val);
  CDataStore &Put(CDataStore &store) {
    LPCVOID data;
    UINT    size;
    store.GetBufferParams(&data, &size, 0);
    return PutData(data, size);
  }
  CDataStore &PutString(LPCSTR pval);
  CDataStore &PutString(const WORD *pval);
  CDataStore &PutArray(const BYTE *pval, UINT count);
  CDataStore &PutArray(LPCSTR pval, UINT count) {
    return PutArray(reinterpret_cast<const BYTE *>(pval), count);
  }
  CDataStore &PutArray(const short *pval, UINT count) {
    return PutArray(reinterpret_cast<const WORD *>(pval), count);
  }
  CDataStore &PutArray(const WORD *pval, UINT count);
  CDataStore &PutArray(const long *pval, UINT count) {
    return PutArray(reinterpret_cast<const DWORD *>(pval), count);
  }
  CDataStore &PutArray(const DWORD *pval, UINT count);
  CDataStore &PutArray(const DWORDLONG *pval, UINT count);
  CDataStore &PutArray(const float *pval, UINT count);
  CDataStore &PutArray(const unreal *pval, UINT count);
  CDataStore &PutData(LPCVOID pval, UINT bytes);

  CDataStore &PutBool(int val) {
    return Put(val);
  }
  CDataStore &PutChar(char val) {
    return Put(val);
  }
  CDataStore &PutUchar(BYTE val) {
    return Put(val);
  }
  CDataStore &PutByte(BYTE val) {
    return Put(val);
  }
  CDataStore &PutTchar(char val) {
    return Put(val);
  }
  CDataStore &PutShort(short val) {
    return Put(val);
  }
  CDataStore &PutUshort(WORD val) {
    return Put(val);
  }
  CDataStore &PutWord(WORD val) {
    return Put(val);
  }
  CDataStore &PutInt(int val) {
    return Put(val);
  }
  CDataStore &PutUint(UINT val) {
    return Put(val);
  }
  CDataStore &PutLong(long val) {
    return Put(val);
  }
  CDataStore &PutUlong(DWORD val) {
    return Put(val);
  }
  CDataStore &PutDword(DWORD val) {
    return Put(val);
  }
  CDataStore &PutLonglong(LONGLONG val) {
    return Put(val);
  }
  CDataStore &PutUlonglong(DWORDLONG val) {
    return Put(val);
  }
  CDataStore &PutFloat(float val) {
    return Put(val);
  }
  CDataStore &PutCharString(LPCSTR val) {
    return PutString(val);
  }
  CDataStore &PutWcharString(const WORD *val) {
    return PutString(val);
  }
  CDataStore &PutTcharString(LPCSTR val) {
    return PutString(val);
  }
  CDataStore &PutUcharArray(const BYTE *val, UINT count) {
    return PutArray(val, count);
  }
  CDataStore &PutUshortArray(const WORD *val, UINT count) {
    return PutArray(val, count);
  }
  CDataStore &PutUlongArray(const DWORD *val, UINT count) {
    return PutArray(val, count);
  }
  CDataStore &PutUlonglongArray(const DWORDLONG *val, UINT count) {
    return PutArray(val, count);
  }
  CDataStore &PutFloatArray(const float *val, UINT count) {
    return PutArray(val, count);
  }

  CDataStore &operator<<(char val) {
    return Put(val);
  }
  CDataStore &operator<<(BYTE val) {
    return Put(val);
  }
  CDataStore &operator<<(short val) {
    return Put(val);
  }
  CDataStore &operator<<(WORD val) {
    return Put(val);
  }
  CDataStore &operator<<(int val) {
    return Put(val);
  }
  CDataStore &operator<<(UINT val) {
    return Put(val);
  }
  CDataStore &operator<<(long val) {
    return Put(val);
  }
  CDataStore &operator<<(DWORD val) {
    return Put(val);
  }
  CDataStore &operator<<(LONGLONG val) {
    return Put(val);
  }
  CDataStore &operator<<(DWORDLONG val) {
    return Put(val);
  }
  CDataStore &operator<<(float val) {
    return Put(val);
  }
  CDataStore &operator<<(LPCSTR val) {
    return PutString(val);
  }
  CDataStore &operator<<(const WORD *val) {
    return PutString(val);
  }

  CDataStore &Get(char &val);
  CDataStore &Get(BYTE &val);
  CDataStore &Get(short &val);
  CDataStore &Get(WORD &val);
  CDataStore &Get(int &val);
  CDataStore &Get(UINT &val);
  CDataStore &Get(long &val);
  CDataStore &Get(DWORD &val);
  CDataStore &Get(LONGLONG &val);
  CDataStore &Get(DWORDLONG &val);
  CDataStore &Get(float &val);
  CDataStore &GetString(char *pval, UINT maxChars);
  CDataStore &GetString(WORD *pval, UINT maxChars);
  CDataStore &GetArray(BYTE *pval, UINT count);
  CDataStore &GetArray(char *pval, UINT count) {
    return GetArray(reinterpret_cast<BYTE *>(pval), count);
  }
  CDataStore &GetArray(short *pval, UINT count) {
    return GetArray(reinterpret_cast<WORD *>(pval), count);
  }
  CDataStore &GetArray(WORD *pval, UINT count);
  CDataStore &GetArray(long *pval, UINT count) {
    return GetArray(reinterpret_cast<DWORD *>(pval), count);
  }
  CDataStore &GetArray(DWORD *pval, UINT count);
  CDataStore &GetArray(DWORDLONG *pval, UINT count);
  CDataStore &GetArray(float *pval, UINT count);
  CDataStore &GetArray(unreal *pval, UINT count);
  CDataStore &GetData(LPVOID pval, UINT bytes);
  CDataStore &GetDataInSitu(LPVOID &pval, UINT bytes);

  int GetBool() {
    int val;
    Get(val);
    return val;
  }

  CDataStore &GetBool(int &val) {
    return Get(val);
  }
  char GetChar() {
    char val;
    Get(val);
    return val;
  }
  CDataStore &GetChar(char &val) {
    return Get(val);
  }
  BYTE GetUchar() {
    BYTE val;
    Get(val);
    return val;
  }
  CDataStore &GetUchar(BYTE &val) {
    return Get(val);
  }
  BYTE GetByte() {
    return GetUchar();
  }
  CDataStore &GetByte(BYTE &val) {
    return Get(val);
  }
  CDataStore &GetTchar(char &val) {
    return Get(val);
  }
  short GetShort() {
    short val;
    Get(val);
    return val;
  }
  CDataStore &GetShort(short &val) {
    return Get(val);
  }
  WORD GetUshort() {
    WORD val;
    Get(val);
    return val;
  }
  CDataStore &GetUshort(WORD &val) {
    return Get(val);
  }
  WORD GetWord() {
    return GetUshort();
  }
  CDataStore &GetWord(WORD &val) {
    return Get(val);
  }
  int GetInt() {
    int val;
    Get(val);
    return val;
  }
  CDataStore &GetInt(int &val) {
    return Get(val);
  }
  UINT GetUint() {
    UINT val;
    Get(val);
    return val;
  }
  CDataStore &GetUint(UINT &val) {
    return Get(val);
  }
  long GetLong() {
    long val;
    Get(val);
    return val;
  }
  CDataStore &GetLong(long &val) {
    return Get(val);
  }
  DWORD GetUlong() {
    DWORD val;
    Get(val);
    return val;
  }
  CDataStore &GetUlong(DWORD &val) {
    return Get(val);
  }
  DWORD GetDword() {
    return GetUlong();
  }
  CDataStore &GetDword(DWORD &val) {
    return Get(val);
  }
  LONGLONG GetLonglong() {
    LONGLONG val;
    Get(val);
    return val;
  }
  CDataStore &GetLonglong(LONGLONG &val) {
    return Get(val);
  }
  DWORDLONG GetUlonglong() {
    DWORDLONG val;
    Get(val);
    return val;
  }
  CDataStore &GetUlonglong(DWORDLONG &val) {
    return Get(val);
  }
  float GetFloat() {
    float val;
    Get(val);
    return val;
  }
  CDataStore &GetFloat(float &val) {
    return Get(val);
  }
  CDataStore &GetCharString(char *val, UINT maxChars) {
    return GetString(val, maxChars);
  }
  CDataStore &GetWcharString(WORD *val, UINT maxChars) {
    return GetString(val, maxChars);
  }
  CDataStore &GetTcharString(char *val, UINT maxChars);
  CDataStore &GetUcharArray(BYTE *val, UINT count) {
    return GetArray(val, count);
  }
  CDataStore &GetUshortArray(WORD *val, UINT count) {
    return GetArray(val, count);
  }
  CDataStore &GetUlongArray(DWORD *val, UINT count) {
    return GetArray(val, count);
  }
  CDataStore &GetUlonglongArray(DWORDLONG *val, UINT count) {
    return GetArray(val, count);
  }
  CDataStore &GetFloatArray(float *val, UINT count) {
    return GetArray(val, count);
  }

  CDataStore &operator>>(char &val) {
    return Get(val);
  }
  CDataStore &operator>>(BYTE &val) {
    return Get(val);
  }
  CDataStore &operator>>(short &val) {
    return Get(val);
  }
  CDataStore &operator>>(WORD &val) {
    return Get(val);
  }
  CDataStore &operator>>(int &val) {
    return Get(val);
  }
  CDataStore &operator>>(UINT &val) {
    return Get(val);
  }
  CDataStore &operator>>(long &val) {
    return Get(val);
  }
  CDataStore &operator>>(DWORD &val) {
    return Get(val);
  }
  CDataStore &operator>>(LONGLONG &val) {
    return Get(val);
  }
  CDataStore &operator>>(DWORDLONG &val) {
    return Get(val);
  }
  CDataStore &operator>>(float &val) {
    return Get(val);
  }

 private:
  BYTE *m_data;
  UINT  m_base;
  UINT  m_alloc;
  UINT  m_size;
  UINT  m_read;
};

#endif
