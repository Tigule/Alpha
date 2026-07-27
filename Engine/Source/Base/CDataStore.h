#ifndef ENGINE_SOURCE_BASE_CDATASTORE_H
#define ENGINE_SOURCE_BASE_CDATASTORE_H

#include <storm.h>

class unreal;

class CDataStore {
 public:
  unsigned char *Alloc(unsigned int bytes, const char *fileName, int lineNumber) {
    if (!bytes) {
      return 0;
    }

    if (!fileName) {
      fileName = __FILE__;
      lineNumber = __LINE__;
    }

    return static_cast<unsigned char *>(SMemAlloc(bytes, fileName, lineNumber, 0));
  }

  void Free(unsigned char *data, const char *fileName, int lineNumber) {
    if (!fileName) {
      fileName = __FILE__;
      lineNumber = __LINE__;
    }

    SMemFree(data, fileName, lineNumber, 0);
  }

  unsigned char *Realloc(unsigned char *data, unsigned int bytes, const char *fileName, int lineNumber) {
    if (!fileName) {
      fileName = __FILE__;
      lineNumber = __LINE__;
    }

    return static_cast<unsigned char *>(SMemReAlloc(data, bytes, fileName, lineNumber, 0));
  }

 protected:
  virtual void InternalInitialize(unsigned char *&data, unsigned int &base, unsigned int &alloc);
  virtual void InternalDestroy(unsigned char *&data, unsigned int &base, unsigned int &alloc);
  virtual int  InternalFetchRead(unsigned int pos, unsigned int bytes, unsigned char *&data, unsigned int &base, unsigned int &alloc);
  virtual int  InternalFetchWrite(
      unsigned int    pos,
      unsigned int    bytes,
      unsigned char *&data,
      unsigned int   &base,
      unsigned int   &alloc,
      const char     *fileName,
      int             lineNumber
  );

  void Initialize() {
    if (m_alloc != static_cast<unsigned int>(-1)) {
      InternalInitialize(m_data, m_base, m_alloc);
    }
  }

  void Destroy() {
    if (m_alloc != static_cast<unsigned int>(-1)) {
      InternalDestroy(m_data, m_base, m_alloc);
    }
  }

  int FetchRead(unsigned int pos, unsigned int bytes) {
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

  int FetchWrite(unsigned int pos, unsigned int bytes, const char *fileName, int lineNumber) {
    if (pos < m_base || pos + bytes > m_base + m_alloc) {
      if (!InternalFetchWrite(pos, bytes, m_data, m_base, m_alloc, fileName, lineNumber)) {
        return 0;
      }

      ASSERT(pos >= m_base);
      ASSERT(pos + bytes <= m_base + m_alloc);
    }

    return 1;
  }

  void AssertFetchWrite(unsigned int pos, unsigned int bytes, const char *fileName, int lineNumber) {
    if (!FetchWrite(pos, bytes, fileName, lineNumber)) {
      FATALERROR(("CDataStore::AssertFetchWrite(%u, %u) failed", pos, bytes));
    }
  }

  void PutSpace(unsigned int bytes);

 public:
  CDataStore() : m_data(0), m_base(0), m_alloc(0), m_size(0), m_read(-1) {
  }

  CDataStore(unsigned char *data, unsigned int size) : m_data(data), m_base(0), m_alloc(-1), m_size(size), m_read(0) {
  }

  CDataStore(unsigned char *data, unsigned int size, unsigned int read);
  CDataStore(const CDataStore &store);
  virtual ~CDataStore() {
    Destroy();
  }

  int IsFinal() const {
    return m_read != static_cast<unsigned int>(-1);
  }

  int IsValid() const {
    return m_read <= m_size;
  }

  int IsReadOnly() {
    return m_alloc == static_cast<unsigned int>(-1);
  }

  operator void *() {
    return IsValid() ? this : 0;
  }

  int operator!() {
    return !IsValid();
  }

  void Unfinalize() {
    m_read = static_cast<unsigned int>(-1);
  }

  void Invalidate() {
    m_read = m_size + 1;
  }

  virtual int  IsRead() const;
  virtual void Reset();
  virtual void Finalize();

  void Seek(unsigned int pos) {
    ASSERT(IsFinal());
    m_read = pos;
  }

  unsigned int Tell() const {
    ASSERT(IsFinal());
    return m_read;
  }

  unsigned int Size() const {
    return m_size;
  }

  void SetSize(unsigned int size);
  void Reserve(unsigned int bytes, const char *fileName = 0, int lineNumber = 0);

  virtual void GetBufferParams(const void **data, unsigned int *size, unsigned int *alloc) const;
  virtual void DetachBuffer(void **data, unsigned int *size, unsigned int *alloc);

  CDataStore &Set(unsigned int pos, char val);
  CDataStore &Set(unsigned int pos, unsigned char val);
  CDataStore &Set(unsigned int pos, short val);
  CDataStore &Set(unsigned int pos, unsigned short val);
  CDataStore &Set(unsigned int pos, int val);
  CDataStore &Set(unsigned int pos, unsigned int val);
  CDataStore &Set(unsigned int pos, long val);
  CDataStore &Set(unsigned int pos, unsigned long val);
  CDataStore &Set(unsigned int pos, __int64 val);
  CDataStore &Set(unsigned int pos, unsigned __int64 val);
  CDataStore &Set(unsigned int pos, float val);

  CDataStore &Put(char val);
  CDataStore &Put(unsigned char val);
  CDataStore &Put(short val);
  CDataStore &Put(unsigned short val);
  CDataStore &Put(int val);
  CDataStore &Put(unsigned int val);
  CDataStore &Put(long val);
  CDataStore &Put(unsigned long val);
  CDataStore &Put(__int64 val);
  CDataStore &Put(unsigned __int64 val);
  CDataStore &Put(float val);
  CDataStore &Put(CDataStore &store) {
    const void  *data;
    unsigned int size;
    store.GetBufferParams(&data, &size, 0);
    return PutData(data, size);
  }
  CDataStore &PutString(const char *pval);
  CDataStore &PutString(const unsigned short *pval);
  CDataStore &PutArray(const unsigned char *pval, unsigned int count);
  CDataStore &PutArray(const char *pval, unsigned int count) {
    return PutArray(reinterpret_cast<const unsigned char *>(pval), count);
  }
  CDataStore &PutArray(const short *pval, unsigned int count) {
    return PutArray(reinterpret_cast<const unsigned short *>(pval), count);
  }
  CDataStore &PutArray(const unsigned short *pval, unsigned int count);
  CDataStore &PutArray(const long *pval, unsigned int count) {
    return PutArray(reinterpret_cast<const unsigned long *>(pval), count);
  }
  CDataStore &PutArray(const unsigned long *pval, unsigned int count);
  CDataStore &PutArray(const unsigned __int64 *pval, unsigned int count);
  CDataStore &PutArray(const float *pval, unsigned int count);
  CDataStore &PutArray(const unreal *pval, unsigned int count);
  CDataStore &PutData(const void *pval, unsigned int bytes);

  CDataStore &PutBool(int val) { return Put(val); }
  CDataStore &PutChar(char val) { return Put(val); }
  CDataStore &PutUchar(unsigned char val) { return Put(val); }
  CDataStore &PutByte(unsigned char val) { return Put(val); }
  CDataStore &PutTchar(char val) { return Put(val); }
  CDataStore &PutShort(short val) { return Put(val); }
  CDataStore &PutUshort(unsigned short val) { return Put(val); }
  CDataStore &PutWord(unsigned short val) { return Put(val); }
  CDataStore &PutInt(int val) { return Put(val); }
  CDataStore &PutUint(unsigned int val) { return Put(val); }
  CDataStore &PutLong(long val) { return Put(val); }
  CDataStore &PutUlong(unsigned long val) { return Put(val); }
  CDataStore &PutDword(unsigned long val) { return Put(val); }
  CDataStore &PutLonglong(__int64 val) { return Put(val); }
  CDataStore &PutUlonglong(unsigned __int64 val) { return Put(val); }
  CDataStore &PutFloat(float val) { return Put(val); }
  CDataStore &PutCharString(const char *val) { return PutString(val); }
  CDataStore &PutWcharString(const unsigned short *val) { return PutString(val); }
  CDataStore &PutTcharString(const char *val) { return PutString(val); }
  CDataStore &PutUcharArray(const unsigned char *val, unsigned int count) { return PutArray(val, count); }
  CDataStore &PutUshortArray(const unsigned short *val, unsigned int count) { return PutArray(val, count); }
  CDataStore &PutUlongArray(const unsigned long *val, unsigned int count) { return PutArray(val, count); }
  CDataStore &PutUlonglongArray(const unsigned __int64 *val, unsigned int count) { return PutArray(val, count); }
  CDataStore &PutFloatArray(const float *val, unsigned int count) { return PutArray(val, count); }

  CDataStore &operator<<(char val) { return Put(val); }
  CDataStore &operator<<(unsigned char val) { return Put(val); }
  CDataStore &operator<<(short val) { return Put(val); }
  CDataStore &operator<<(unsigned short val) { return Put(val); }
  CDataStore &operator<<(int val) { return Put(val); }
  CDataStore &operator<<(unsigned int val) { return Put(val); }
  CDataStore &operator<<(long val) { return Put(val); }
  CDataStore &operator<<(unsigned long val) { return Put(val); }
  CDataStore &operator<<(__int64 val) { return Put(val); }
  CDataStore &operator<<(unsigned __int64 val) { return Put(val); }
  CDataStore &operator<<(float val) { return Put(val); }
  CDataStore &operator<<(const char *val) { return PutString(val); }
  CDataStore &operator<<(const unsigned short *val) { return PutString(val); }

  CDataStore &Get(char &val);
  CDataStore &Get(unsigned char &val);
  CDataStore &Get(short &val);
  CDataStore &Get(unsigned short &val);
  CDataStore &Get(int &val);
  CDataStore &Get(unsigned int &val);
  CDataStore &Get(long &val);
  CDataStore &Get(unsigned long &val);
  CDataStore &Get(__int64 &val);
  CDataStore &Get(unsigned __int64 &val);
  CDataStore &Get(float &val);
  CDataStore &GetString(char *pval, unsigned int maxChars);
  CDataStore &GetString(unsigned short *pval, unsigned int maxChars);
  CDataStore &GetArray(unsigned char *pval, unsigned int count);
  CDataStore &GetArray(char *pval, unsigned int count) {
    return GetArray(reinterpret_cast<unsigned char *>(pval), count);
  }
  CDataStore &GetArray(short *pval, unsigned int count) {
    return GetArray(reinterpret_cast<unsigned short *>(pval), count);
  }
  CDataStore &GetArray(unsigned short *pval, unsigned int count);
  CDataStore &GetArray(long *pval, unsigned int count) {
    return GetArray(reinterpret_cast<unsigned long *>(pval), count);
  }
  CDataStore &GetArray(unsigned long *pval, unsigned int count);
  CDataStore &GetArray(unsigned __int64 *pval, unsigned int count);
  CDataStore &GetArray(float *pval, unsigned int count);
  CDataStore &GetArray(unreal *pval, unsigned int count);
  CDataStore &GetData(void *pval, unsigned int bytes);
  CDataStore &GetDataInSitu(void *&pval, unsigned int bytes);

  int GetBool() {
    int val;
    Get(val);
    return val;
  }

  CDataStore &GetBool(int &val) { return Get(val); }
  char GetChar() { char val; Get(val); return val; }
  CDataStore &GetChar(char &val) { return Get(val); }
  unsigned char GetUchar() { unsigned char val; Get(val); return val; }
  CDataStore &GetUchar(unsigned char &val) { return Get(val); }
  unsigned char GetByte() { return GetUchar(); }
  CDataStore &GetByte(unsigned char &val) { return Get(val); }
  CDataStore &GetTchar(char &val) { return Get(val); }
  short GetShort() { short val; Get(val); return val; }
  CDataStore &GetShort(short &val) { return Get(val); }
  unsigned short GetUshort() { unsigned short val; Get(val); return val; }
  CDataStore &GetUshort(unsigned short &val) { return Get(val); }
  unsigned short GetWord() { return GetUshort(); }
  CDataStore &GetWord(unsigned short &val) { return Get(val); }
  int GetInt() { int val; Get(val); return val; }
  CDataStore &GetInt(int &val) { return Get(val); }
  unsigned int GetUint() { unsigned int val; Get(val); return val; }
  CDataStore &GetUint(unsigned int &val) { return Get(val); }
  long GetLong() { long val; Get(val); return val; }
  CDataStore &GetLong(long &val) { return Get(val); }
  unsigned long GetUlong() { unsigned long val; Get(val); return val; }
  CDataStore &GetUlong(unsigned long &val) { return Get(val); }
  unsigned long GetDword() { return GetUlong(); }
  CDataStore &GetDword(unsigned long &val) { return Get(val); }
  __int64 GetLonglong() { __int64 val; Get(val); return val; }
  CDataStore &GetLonglong(__int64 &val) { return Get(val); }
  unsigned __int64 GetUlonglong() { unsigned __int64 val; Get(val); return val; }
  CDataStore &GetUlonglong(unsigned __int64 &val) { return Get(val); }
  float GetFloat() { float val; Get(val); return val; }
  CDataStore &GetFloat(float &val) { return Get(val); }
  CDataStore &GetCharString(char *val, unsigned int maxChars) { return GetString(val, maxChars); }
  CDataStore &GetWcharString(unsigned short *val, unsigned int maxChars) { return GetString(val, maxChars); }
  CDataStore &GetUcharArray(unsigned char *val, unsigned int count) { return GetArray(val, count); }
  CDataStore &GetUshortArray(unsigned short *val, unsigned int count) { return GetArray(val, count); }
  CDataStore &GetUlongArray(unsigned long *val, unsigned int count) { return GetArray(val, count); }
  CDataStore &GetUlonglongArray(unsigned __int64 *val, unsigned int count) { return GetArray(val, count); }
  CDataStore &GetFloatArray(float *val, unsigned int count) { return GetArray(val, count); }

  CDataStore &operator>>(char &val) { return Get(val); }
  CDataStore &operator>>(unsigned char &val) { return Get(val); }
  CDataStore &operator>>(short &val) { return Get(val); }
  CDataStore &operator>>(unsigned short &val) { return Get(val); }
  CDataStore &operator>>(int &val) { return Get(val); }
  CDataStore &operator>>(unsigned int &val) { return Get(val); }
  CDataStore &operator>>(long &val) { return Get(val); }
  CDataStore &operator>>(unsigned long &val) { return Get(val); }
  CDataStore &operator>>(__int64 &val) { return Get(val); }
  CDataStore &operator>>(unsigned __int64 &val) { return Get(val); }
  CDataStore &operator>>(float &val) { return Get(val); }

 private:
  unsigned char *m_data;
  unsigned int   m_base;
  unsigned int   m_alloc;
  unsigned int   m_size;
  unsigned int   m_read;
};

#endif
