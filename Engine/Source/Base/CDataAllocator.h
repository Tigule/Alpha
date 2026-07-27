#pragma once

#include <storm.h>

class CDataAllocator {
 public:
  struct Block {
    Block *m_next;
  };

  struct Data {
    Data *m_next;
  };

  CDataAllocator(unsigned long bytesPerData, unsigned long dataPerBlock);
  CDataAllocator(const CDataAllocator &source);
  ~CDataAllocator();

  void  Clear(const char *fileName, int lineNumber);
  void *GetData(int zero, const char *fileName, int lineNumber);
  void  PutData(void *data, const char *fileName, int lineNumber);
  unsigned long BytesPerData() const {
    return m_bytesPerData;
  }
  unsigned long DataPerBlock() const {
    return m_dataPerBlock;
  }
  unsigned long DataUsed() const {
    return m_dataUsed;
  }

 private:
  CDataAllocator &operator=(const CDataAllocator &source);

  unsigned long m_bytesPerData;
  unsigned long m_dataPerBlock;
  unsigned long m_dataUsed;
  Block       *m_blockList;
  Data        *m_dataList;
};

template <class T>
class TInstanceAllocator : protected CDataAllocator {
 public:
  TInstanceAllocator(unsigned long dataPerBlock) : CDataAllocator(sizeof(T), dataPerBlock) {
  }

  __forceinline void Clear() {
    CDataAllocator::Clear(typeid(T).raw_name(), SERR_LINECODE_OBJECT);
  }

  __forceinline T *Get(int zero) {
    void *data = CDataAllocator::GetData(zero, typeid(T).raw_name(), SERR_LINECODE_OBJECT);
    return data ? new (data) T : 0;
  }

  __forceinline void Put(T *data) {
    data->~T();
    CDataAllocator::PutData(data, 0, 0);
  }

  __forceinline unsigned long Used() const {
    return DataUsed();
  }

 private:
  TInstanceAllocator &operator=(const TInstanceAllocator &);
};

template <class T>
class TLockedInstanceAllocator : protected TInstanceAllocator<T> {
 public:
  TLockedInstanceAllocator(unsigned long dataPerBlock) : TInstanceAllocator<T>(dataPerBlock) {
  }
  TLockedInstanceAllocator(const TLockedInstanceAllocator &);

  __forceinline void Clear() {
    TInstanceAllocator<T>::Clear();
  }

  __forceinline T *Get(int zero) {
    m_critsect.Enter();
    T *data = TInstanceAllocator<T>::Get(zero);
    m_critsect.Leave();
    return data;
  }

  __forceinline void Put(T *data) {
    m_critsect.Enter();
    TInstanceAllocator<T>::Put(data);
    m_critsect.Leave();
  }

  __forceinline unsigned long Used() const {
    return TInstanceAllocator<T>::Used();
  }

 private:
  TLockedInstanceAllocator &operator=(const TLockedInstanceAllocator &);

  SCritSect m_critsect;
};
