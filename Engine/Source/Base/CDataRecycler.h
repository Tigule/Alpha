#pragma once

#include <storm.h>

#include <new>

template <class T>
class TExtraInstanceRecycler;

template <class T>
class TExtraInstanceRecyclable {
  friend class TExtraInstanceRecycler<T>;

 protected:
  void SetRecycleBytes(DWORD recycleBytes) {
    m_recycleBytes = recycleBytes;
  }

  DWORD GetRecycleBytes() const {
    return m_recycleBytes;
  }

 private:
  DWORD m_recycleBytes;
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
    Node  *m_next;
    LPVOID m_data;
    DWORD  m_bytes;
  };

  struct NodeBlock {
    NodeBlock *m_next;
    Node       m_nodes[1];
  };

  CDataRecycler(UINT nodesPerBlock = eDefaultNodesPerBlock, long maxNodes = eDefaultMaxNodes);
  CDataRecycler(const CDataRecycler &);
  virtual ~CDataRecycler();

  virtual void   Clear();
  virtual LPVOID AllocData(DWORD allocBytes, DWORD *bytes, LPCSTR fileName, int lineNumber);
  virtual LPVOID ReallocData(LPVOID data, DWORD allocBytes, DWORD *bytes, LPCSTR fileName, int lineNumber);
  virtual void   FreeData(LPVOID data, LPCSTR fileName, int lineNumber);

  void GetData(LPVOID &data, DWORD &bytes, LPCSTR fileName, int lineNumber);
  void GetAndResizeData(DWORD allocBytes, LPVOID &data, DWORD &bytes, LPCSTR fileName, int lineNumber) {
    GetData(data, bytes, fileName, lineNumber);
    if (bytes < allocBytes) {
      data = ReallocData(data, allocBytes, &bytes, fileName, lineNumber);
    }
  }
  void PutData(LPVOID data, DWORD bytes, LPCSTR fileName, int lineNumber);

 private:
  CDataRecycler &operator=(const CDataRecycler &);

  void       Link(LPVOID *list, LPVOID item, int nextOffset);
  void       Link(NodeBlock **list, NodeBlock *nodeBlock);
  void       Link(Node **list, Node *node);
  LPVOID     Unlink(LPVOID *list, int nextOffset);
  NodeBlock *Unlink(NodeBlock **list);
  Node      *Unlink(Node **list);
  void       Link(Node **list, NodeBlock *nodeBlock);

  long       m_nodesRecyclable;
  UINT       m_nodesPerBlock;
  NodeBlock *m_nodeBlockList;
  Node      *m_nodeFullList;
  Node      *m_nodeEmptyList;
};

template <class T>
class TExtraInstanceRecycler : protected CDataRecycler {
 public:
  enum {
    eDefaultMaxBytesPerInstance = -1
  };

  TExtraInstanceRecycler(UINT nodesPerBlock = 0, long maxNodes = 0, DWORD maxBytesPerInstance = eDefaultMaxBytesPerInstance)
      : CDataRecycler(nodesPerBlock, maxNodes), m_maxBytesPerInstance(maxBytesPerInstance) {
  }

  virtual ~TExtraInstanceRecycler() {
  }

  virtual void Clear() {
    CDataRecycler::Clear();
  }

  T *Get(DWORD bytes) {
    DWORD  recycleBytes;
    LPVOID data;

    if (bytes > m_maxBytesPerInstance) {
      data = AllocData(bytes, &recycleBytes, typeid(T).INTERNALRAWNAME(), SERR_LINECODE_OBJECT);
    } else {
      GetData(data, recycleBytes, typeid(T).INTERNALRAWNAME(), SERR_LINECODE_OBJECT);
      if (recycleBytes < bytes) {
        data = ReallocData(data, bytes, &recycleBytes, typeid(T).INTERNALRAWNAME(), SERR_LINECODE_OBJECT);
      }
    }

    T *instance = data ? new (data) T : 0;
    instance->SetRecycleBytes(recycleBytes);
    return instance;
  }

  void Put(T *instance) {
    DWORD recycleBytes = instance->GetRecycleBytes();

    instance->~T();
    if (recycleBytes > m_maxBytesPerInstance) {
      FreeData(instance, typeid(T).INTERNALRAWNAME(), SERR_LINECODE_OBJECT);
    } else {
      PutData(instance, recycleBytes, typeid(T).INTERNALRAWNAME(), SERR_LINECODE_OBJECT);
    }
  }

 private:
  TExtraInstanceRecycler &operator=(const TExtraInstanceRecycler &);

  DWORD m_maxBytesPerInstance;
};
