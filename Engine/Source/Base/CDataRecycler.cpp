#include <Base/Base.h>
#include <time.h>

#include "CDataRecycler.h"
#include <stdio.h>

CDataRecycler::CDataRecycler(UINT nodesPerBlock, LONG maxNodes) : m_nodeBlockList(NULL), m_nodeFullList(NULL), m_nodeEmptyList(NULL) {
  m_nodesRecyclable = max(maxNodes, 1);
  m_nodesPerBlock = max(nodesPerBlock, 1);
  m_nodesPerBlock = min(m_nodesPerBlock, m_nodesRecyclable);
}

CDataRecycler::~CDataRecycler() {
}

void CDataRecycler::Clear() {
  Node      *node;
  NodeBlock *nodeBlock;

  while ((node = Unlink(&m_nodeFullList)) != NULL) {
    FreeData(node->m_data, 0, 0);
  }

  m_nodeEmptyList = NULL;
  while ((nodeBlock = Unlink(&m_nodeBlockList)) != NULL) {
    DEL(nodeBlock);
  }
}

void CDataRecycler::GetData(LPVOID &data, DWORD &bytes, LPCSTR fileName, int lineNumber) {
  Node *node = Unlink(&m_nodeFullList);

  if (node) {
    SInterlockedIncrement(&m_nodesRecyclable);
    data = node->m_data;
    bytes = node->m_bytes;
    Link(&m_nodeEmptyList, node);
  } else {
    data = NULL;
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

  while ((node = Unlink(&m_nodeEmptyList)) == NULL) {
    NodeBlock *nodeBlock = (NodeBlock *)ALLOC(sizeof(NodeBlock *) + m_nodesPerBlock * sizeof(Node));

    Link(&m_nodeBlockList, nodeBlock);
    Link(&m_nodeEmptyList, nodeBlock);
  }

  node->m_data = data;
  node->m_bytes = bytes;
  Link(&m_nodeFullList, node);
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
    *(LPVOID *)((char *)item + nextOffset) = head;
  } while (SInterlockedCompareExchangePointer(list, item, head) != head);
}

LPVOID CDataRecycler::Unlink(LPVOID *list, int nextOffset) {
  LPVOID head;

  while ((head = *list) != NULL) {
    if (SInterlockedCompareExchangePointer(list, *(LPVOID *)((char *)head + nextOffset), head) == head) {
      break;
    }
  }

  return head;
}

void CDataRecycler::Link(Node **list, NodeBlock *nodeBlock) {
  UINT index;

  for (index = 0; index < m_nodesPerBlock - 1; ++index) {
    nodeBlock->m_nodes[index].m_next = &nodeBlock->m_nodes[index + 1];
  }

  Link((LPVOID *)list, nodeBlock->m_nodes, (m_nodesPerBlock - 1) * sizeof(Node));
}
