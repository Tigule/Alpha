#pragma once

#include <storm.h>

#include <new>
#include <typeinfo>

template <class T>
class TExtraInstanceRecycler;

template <class T>
class TExtraInstanceRecyclable {
  friend class TExtraInstanceRecycler<T>;

 protected:
  void SetRecycleBytes(unsigned long recycleBytes) {
    m_recycleBytes = recycleBytes;
  }

  unsigned long GetRecycleBytes() {
    return m_recycleBytes;
  }

  unsigned long m_recycleBytes;
};

class CDataRecycler {
 public:
  struct Node {
    Node         *m_next;
    void         *m_data;
    unsigned long m_bytes;
  };

  struct NodeBlock {
    NodeBlock *m_next;
    Node       m_nodes[1];
  };

  CDataRecycler(unsigned int nodesPerBlock, long maxNodes);
  virtual ~CDataRecycler();

  virtual void  Clear();
  virtual void *AllocData(unsigned long allocBytes, unsigned long *bytes, const char *fileName, int lineNumber);
  virtual void *ReallocData(void *data, unsigned long allocBytes, unsigned long *bytes, const char *fileName, int lineNumber);
  virtual void  FreeData(void *data, const char *fileName, int lineNumber);

  void GetData(void *&data, unsigned long &bytes, const char *fileName, int lineNumber);
  void GetAndResizeData(unsigned long allocBytes, void *&data, unsigned long &bytes, const char *fileName, int lineNumber) {
    GetData(data, bytes, fileName, lineNumber);
    if (bytes < allocBytes) {
      data = ReallocData(data, allocBytes, &bytes, fileName, lineNumber);
    }
  }
  void PutData(void *data, unsigned long bytes, const char *fileName, int lineNumber);

 private:
  void  Link(void **list, void *item, int nextOffset);
  void *Unlink(void **list, int nextOffset);
  void  Link(Node **list, NodeBlock *nodeBlock);

  long         m_nodesRecyclable;
  unsigned int m_nodesPerBlock;
  NodeBlock   *m_nodeBlockList;
  Node        *m_nodeFullList;
  Node        *m_nodeEmptyList;
};

template <class T>
class TExtraInstanceRecycler : public CDataRecycler {
 public:
  TExtraInstanceRecycler(unsigned int nodesPerBlock, long maxNodes, unsigned long maxBytesPerInstance)
      : CDataRecycler(nodesPerBlock, maxNodes), m_maxBytesPerInstance(maxBytesPerInstance) {
  }

  virtual ~TExtraInstanceRecycler() {
  }

  virtual void Clear() {
    CDataRecycler::Clear();
  }

  T *Get(unsigned long bytes) {
    unsigned long recycleBytes;
    void         *data;

    if (bytes > m_maxBytesPerInstance) {
      data = AllocData(bytes, &recycleBytes, typeid(T).raw_name(), SERR_LINECODE_OBJECT);
    } else {
      GetData(data, recycleBytes, typeid(T).raw_name(), SERR_LINECODE_OBJECT);
      if (recycleBytes < bytes) {
        data = ReallocData(data, bytes, &recycleBytes, typeid(T).raw_name(), SERR_LINECODE_OBJECT);
      }
    }

    T *instance = data ? new (data) T : 0;
    instance->SetRecycleBytes(recycleBytes);
    return instance;
  }

  void Put(T *instance) {
    unsigned long recycleBytes = instance->GetRecycleBytes();

    instance->~T();
    if (recycleBytes > m_maxBytesPerInstance) {
      FreeData(instance, typeid(T).raw_name(), SERR_LINECODE_OBJECT);
    } else {
      PutData(instance, recycleBytes, typeid(T).raw_name(), SERR_LINECODE_OBJECT);
    }
  }

  unsigned long m_maxBytesPerInstance;
};
