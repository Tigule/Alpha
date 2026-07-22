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
  ~CDataAllocator();

  void  Clear(const char *fileName, int lineNumber);
  void *GetData(int zero, const char *fileName, int lineNumber);
  void  PutData(void *data, const char *fileName, int lineNumber);

  unsigned int m_bytesPerData;
  unsigned int m_dataPerBlock;
  unsigned int m_dataUsed;
  Block       *m_blockList;
  Data        *m_dataList;
};

template <class T>
class TInstanceAllocator : public CDataAllocator {
 public:
  TInstanceAllocator(unsigned long dataPerBlock) : CDataAllocator(sizeof(T), dataPerBlock) {
  }
};

template <class T>
class TLockedInstanceAllocator : public TInstanceAllocator<T> {
 public:
  TLockedInstanceAllocator(unsigned long dataPerBlock) : TInstanceAllocator<T>(dataPerBlock) {
  }

  SCritSect m_critsect;
};
