#include "CDataRecycler.h"

CDataRecycler::CDataRecycler(unsigned int nodesPerBlock, long maxNodes) : m_nodeBlockList(0), m_nodeFullList(0), m_nodeEmptyList(0) {
  if (maxNodes <= 1) {
    maxNodes = 1;
  }
  if (nodesPerBlock <= 1) {
    nodesPerBlock = 1;
  }

  m_nodesRecyclable = maxNodes;
  m_nodesPerBlock = nodesPerBlock < static_cast<unsigned int>(maxNodes) ? nodesPerBlock : static_cast<unsigned int>(maxNodes);
}

CDataRecycler::~CDataRecycler() {
}

void CDataRecycler::Clear() {
  Node      *node;
  NodeBlock *nodeBlock;

  while ((node = static_cast<Node *>(Unlink(reinterpret_cast<void **>(&m_nodeFullList), 0))) != 0) {
    FreeData(node->m_data, 0, 0);
  }

  m_nodeEmptyList = 0;
  while ((nodeBlock = static_cast<NodeBlock *>(Unlink(reinterpret_cast<void **>(&m_nodeBlockList), 0))) != 0) {
    DEL(nodeBlock);
  }
}

void CDataRecycler::GetData(void *&data, unsigned long &bytes, const char *fileName, int lineNumber) {
  Node *node = static_cast<Node *>(Unlink(reinterpret_cast<void **>(&m_nodeFullList), 0));

  if (node) {
    SInterlockedIncrement(&m_nodesRecyclable);
    data = node->m_data;
    bytes = node->m_bytes;
    Link(reinterpret_cast<void **>(&m_nodeEmptyList), node, 0);
  } else {
    data = 0;
    bytes = 0;
  }
}

void CDataRecycler::PutData(void *data, unsigned long bytes, const char *fileName, int lineNumber) {
  Node *node;

  if (SInterlockedDecrement(&m_nodesRecyclable) < 0) {
    SInterlockedIncrement(&m_nodesRecyclable);
    FreeData(data, fileName, lineNumber);
    return;
  }

  do {
    node = static_cast<Node *>(Unlink(reinterpret_cast<void **>(&m_nodeEmptyList), 0));
    if (!node) {
      unsigned int allocBytes = sizeof(NodeBlock *) + m_nodesPerBlock * sizeof(Node);
      NodeBlock   *nodeBlock = static_cast<NodeBlock *>(ALLOC(allocBytes));

      Link(reinterpret_cast<void **>(&m_nodeBlockList), nodeBlock, 0);
      Link(&m_nodeEmptyList, nodeBlock);
    }
  } while (!node);

  node->m_data = data;
  node->m_bytes = bytes;
  Link(reinterpret_cast<void **>(&m_nodeFullList), node, 0);
}

void *CDataRecycler::AllocData(unsigned long allocBytes, unsigned long *bytes, const char *fileName, int lineNumber) {
  void *data = SMemAlloc(allocBytes, fileName, lineNumber, 0);
  if (bytes) {
    *bytes = allocBytes;
  }
  return data;
}

void *CDataRecycler::ReallocData(void *data, unsigned long allocBytes, unsigned long *bytes, const char *fileName, int lineNumber) {
  void *newData = SMemReAlloc(data, allocBytes, fileName, lineNumber, 0);
  if (bytes) {
    *bytes = allocBytes;
  }
  return newData;
}

void CDataRecycler::FreeData(void *data, const char *fileName, int lineNumber) {
  SMemFree(data, fileName, lineNumber, 0);
}

void CDataRecycler::Link(void **list, void *item, int nextOffset) {
  void *head;

  do {
    head = *list;
    *reinterpret_cast<void **>(static_cast<char *>(item) + nextOffset) = head;
  } while (SInterlockedCompareExchangePointer(list, item, head) != head);
}

void *CDataRecycler::Unlink(void **list, int nextOffset) {
  void *head;

  do {
    head = *list;
    if (!head) {
      return 0;
    }
  } while (SInterlockedCompareExchangePointer(list, *reinterpret_cast<void **>(static_cast<char *>(head) + nextOffset), head) != head);

  return head;
}

void CDataRecycler::Link(Node **list, NodeBlock *nodeBlock) {
  unsigned int index;

  for (index = 0; index + 1 < m_nodesPerBlock; ++index) {
    nodeBlock->m_nodes[index].m_next = &nodeBlock->m_nodes[index + 1];
  }

  Link(reinterpret_cast<void **>(list), nodeBlock->m_nodes, (m_nodesPerBlock - 1) * sizeof(Node));
}
