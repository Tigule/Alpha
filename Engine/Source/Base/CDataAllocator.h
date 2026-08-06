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

  CDataAllocator(DWORD bytesPerData, DWORD dataPerBlock);
  CDataAllocator(const CDataAllocator &source);
  ~CDataAllocator();

  void   Clear(LPCSTR fileName, int lineNumber);
  LPVOID GetData(int zero, LPCSTR fileName, int lineNumber);
  void   PutData(LPVOID data, LPCSTR fileName, int lineNumber);
  DWORD  BytesPerData() const {
    return m_bytesPerData;
  }
  DWORD DataPerBlock() const {
    return m_dataPerBlock;
  }
  DWORD DataUsed() const {
    return m_dataUsed;
  }

 private:
  CDataAllocator &operator=(const CDataAllocator &source);

  DWORD  m_bytesPerData;
  DWORD  m_dataPerBlock;
  DWORD  m_dataUsed;
  Block *m_blockList;
  Data  *m_dataList;
};

template <class T>
class TInstanceAllocator : protected CDataAllocator {
 public:
  TInstanceAllocator(DWORD dataPerBlock) : CDataAllocator(sizeof(T), dataPerBlock) {
  }

  __forceinline void Clear() {
    CDataAllocator::Clear(typeid(T).INTERNALRAWNAME(), SERR_LINECODE_OBJECT);
  }

  __forceinline T *Get(int zero) {
    LPVOID data = CDataAllocator::GetData(zero, typeid(T).INTERNALRAWNAME(), SERR_LINECODE_OBJECT);
    return data ? new (data) T : 0;
  }

  __forceinline void Put(T *data) {
    data->~T();
    CDataAllocator::PutData(data, 0, 0);
  }

  __forceinline DWORD Used() const {
    return DataUsed();
  }

 private:
  TInstanceAllocator &operator=(const TInstanceAllocator &);
};

template <class T>
class TLockedInstanceAllocator : protected TInstanceAllocator<T> {
 public:
  TLockedInstanceAllocator(DWORD dataPerBlock) : TInstanceAllocator<T>(dataPerBlock) {
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

  __forceinline DWORD Used() const {
    return TInstanceAllocator<T>::Used();
  }

 private:
  TLockedInstanceAllocator &operator=(const TLockedInstanceAllocator &);

  SCritSect m_critsect;
};
