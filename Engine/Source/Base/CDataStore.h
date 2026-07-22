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
  CDataStore &PutString(const char *pval);
  CDataStore &PutString(const unsigned short *pval);
  CDataStore &PutArray(const unsigned char *pval, unsigned int count);
  CDataStore &PutArray(const unsigned short *pval, unsigned int count);
  CDataStore &PutArray(const unsigned long *pval, unsigned int count);
  CDataStore &PutArray(const unsigned __int64 *pval, unsigned int count);
  CDataStore &PutArray(const float *pval, unsigned int count);
  CDataStore &PutArray(const unreal *pval, unsigned int count);
  CDataStore &PutData(const void *pval, unsigned int bytes);

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
  CDataStore &GetArray(unsigned short *pval, unsigned int count);
  CDataStore &GetArray(unsigned long *pval, unsigned int count);
  CDataStore &GetArray(unsigned __int64 *pval, unsigned int count);
  CDataStore &GetArray(float *pval, unsigned int count);
  CDataStore &GetArray(unreal *pval, unsigned int count);
  CDataStore &GetData(void *pval, unsigned int bytes);
  CDataStore &GetDataInSitu(void *&pval, unsigned int bytes);

 private:
  unsigned char *m_data;
  unsigned int   m_base;
  unsigned int   m_alloc;
  unsigned int   m_size;
  unsigned int   m_read;
};

#endif
