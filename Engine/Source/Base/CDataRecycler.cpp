#include <Base/Base.h>

#include "CDataRecycler.h"

CDataRecycler::CDataRecycler(UINT nodesPerBlock, long maxNodes) : m_nodeBlockList(0), m_nodeFullList(0), m_nodeEmptyList(0) {
  if (maxNodes <= 1) {
    maxNodes = 1;
  }
  if (nodesPerBlock <= 1) {
    nodesPerBlock = 1;
  }

  m_nodesRecyclable = maxNodes;
  m_nodesPerBlock = nodesPerBlock < static_cast<UINT>(maxNodes) ? nodesPerBlock : static_cast<UINT>(maxNodes);
}

CDataRecycler::~CDataRecycler() {
}

void CDataRecycler::Clear() {
  Node      *node;
  NodeBlock *nodeBlock;

  while ((node = static_cast<Node *>(Unlink(reinterpret_cast<LPVOID *>(&m_nodeFullList), 0))) != 0) {
    FreeData(node->m_data, 0, 0);
  }

  m_nodeEmptyList = 0;
  while ((nodeBlock = static_cast<NodeBlock *>(Unlink(reinterpret_cast<LPVOID *>(&m_nodeBlockList), 0))) != 0) {
    DEL(nodeBlock);
  }
}

void CDataRecycler::GetData(LPVOID &data, DWORD &bytes, LPCSTR fileName, int lineNumber) {
  Node *node = static_cast<Node *>(Unlink(reinterpret_cast<LPVOID *>(&m_nodeFullList), 0));

  if (node) {
    SInterlockedIncrement(&m_nodesRecyclable);
    data = node->m_data;
    bytes = node->m_bytes;
    Link(reinterpret_cast<LPVOID *>(&m_nodeEmptyList), node, 0);
  } else {
    data = 0;
    bytes = 0;
  }
}

void CDataRecycler::PutData(LPVOID data, DWORD bytes, LPCSTR fileName, int lineNumber) {
  Node *node;

  if (SInterlockedDecrement(&m_nodesRecyclable) < 0) {
    SInterlockedIncrement(&m_nodesRecyclable);
    FreeData(data, fileName, lineNumber);
    return;
  }

  do {
    node = static_cast<Node *>(Unlink(reinterpret_cast<LPVOID *>(&m_nodeEmptyList), 0));
    if (!node) {
      UINT       allocBytes = sizeof(NodeBlock *) + m_nodesPerBlock * sizeof(Node);
      NodeBlock *nodeBlock = static_cast<NodeBlock *>(ALLOC(allocBytes));

      Link(reinterpret_cast<LPVOID *>(&m_nodeBlockList), nodeBlock, 0);
      Link(&m_nodeEmptyList, nodeBlock);
    }
  } while (!node);

  node->m_data = data;
  node->m_bytes = bytes;
  Link(reinterpret_cast<LPVOID *>(&m_nodeFullList), node, 0);
}

LPVOID CDataRecycler::AllocData(DWORD allocBytes, DWORD *bytes, LPCSTR fileName, int lineNumber) {
  LPVOID data = SMemAlloc(allocBytes, fileName, lineNumber, 0);
  if (bytes) {
    *bytes = allocBytes;
  }
  return data;
}

LPVOID CDataRecycler::ReallocData(LPVOID data, DWORD allocBytes, DWORD *bytes, LPCSTR fileName, int lineNumber) {
  LPVOID newData = SMemReAlloc(data, allocBytes, fileName, lineNumber, 0);
  if (bytes) {
    *bytes = allocBytes;
  }
  return newData;
}

void CDataRecycler::FreeData(LPVOID data, LPCSTR fileName, int lineNumber) {
  SMemFree(data, fileName, lineNumber, 0);
}

void CDataRecycler::Link(LPVOID *list, LPVOID item, int nextOffset) {
  LPVOID head;

  do {
    head = *list;
    *reinterpret_cast<LPVOID *>(static_cast<char *>(item) + nextOffset) = head;
  } while (SInterlockedCompareExchangePointer(list, item, head) != head);
}

LPVOID CDataRecycler::Unlink(LPVOID *list, int nextOffset) {
  LPVOID head;

  do {
    head = *list;
    if (!head) {
      return 0;
    }
  } while (SInterlockedCompareExchangePointer(list, *reinterpret_cast<LPVOID *>(static_cast<char *>(head) + nextOffset), head) != head);

  return head;
}

void CDataRecycler::Link(Node **list, NodeBlock *nodeBlock) {
  UINT index;

  for (index = 0; index + 1 < m_nodesPerBlock; ++index) {
    nodeBlock->m_nodes[index].m_next = &nodeBlock->m_nodes[index + 1];
  }

  Link(reinterpret_cast<LPVOID *>(list), nodeBlock->m_nodes, (m_nodesPerBlock - 1) * sizeof(Node));
}
