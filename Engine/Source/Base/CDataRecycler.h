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

  unsigned long GetRecycleBytes() const {
    return m_recycleBytes;
  }

 private:
  unsigned long m_recycleBytes;
};

class CDataRecycler {
 public:
  enum {
    eDefaultNodesPerBlock = 16
  };

  enum {
    eDefaultMaxNodes = 0x7FFFFFFF
  };

  struct Node {
    Node         *m_next;
    void         *m_data;
    unsigned long m_bytes;
  };

  struct NodeBlock {
    NodeBlock *m_next;
    Node       m_nodes[1];
  };

  CDataRecycler(
      unsigned int nodesPerBlock = eDefaultNodesPerBlock,
      long maxNodes = eDefaultMaxNodes);
  CDataRecycler(const CDataRecycler &);
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
  CDataRecycler &operator=(const CDataRecycler &);

  void  Link(void **list, void *item, int nextOffset);
  void  Link(NodeBlock **list, NodeBlock *nodeBlock);
  void  Link(Node **list, Node *node);
  void *Unlink(void **list, int nextOffset);
  NodeBlock *Unlink(NodeBlock **list);
  Node      *Unlink(Node **list);
  void  Link(Node **list, NodeBlock *nodeBlock);

  long         m_nodesRecyclable;
  unsigned int m_nodesPerBlock;
  NodeBlock   *m_nodeBlockList;
  Node        *m_nodeFullList;
  Node        *m_nodeEmptyList;
};

template <class T>
class TExtraInstanceRecycler : protected CDataRecycler {
 public:
  enum {
    eDefaultMaxBytesPerInstance = -1
  };

  TExtraInstanceRecycler(
      unsigned int nodesPerBlock = 0,
      long maxNodes = 0,
      unsigned long maxBytesPerInstance = eDefaultMaxBytesPerInstance)
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

 private:
  TExtraInstanceRecycler &operator=(const TExtraInstanceRecycler &);

  unsigned long m_maxBytesPerInstance;
};
