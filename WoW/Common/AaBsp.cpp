#include "AaBsp.h"

#include <storm.h>

CAaBsp::~CAaBsp() {
  Free();
}

void CAaBsp::Clear() {
  Free();
  Init();
}

void CAaBsp::Set(CAaBspNode *nodeList, unsigned int nNodes, unsigned short *faceIndices, unsigned int nFaceIndices, NTempest::CAaBox &box) {
  FATALASSERT(nodeList);
  FATALASSERT(faceIndices);

  rootNode = nodeList;
  nodes = nodeList;
  nodeFaceIndices = faceIndices;
  this->nNodes = nNodes;
  this->nNodeFaceIndices = nFaceIndices;
  bFree = 0;
  aaBox = box;
}

void CAaBsp::Init() {
  rootNode = 0;
  nodes = 0;
  nodeFaceIndices = 0;
  nNodes = 0;
  nNodeFaceIndices = 0;
  faceVertexIndices = 0;
  nFaceVertexIndices = 0;
  vertices = 0;
  nVertices = 0;
  nodeSize = 0;
  nodeNext = 0;
  nodeFaceIndicesSize = 0;
  nodeFaceIndicesNext = 0;
  buildFaceIndices = 0;
  buildFaceIndicesNext = 0;
  buildFaceIndicesSize = 0;
  treeDepth = 0;
  bFree = 1;
  avgNodeFaces = 0;
}

void CAaBsp::Free() {
  if (bFree) {
    if (nodes) {
      SMemFree(nodes, 0, 0, 0);
    }
    if (nodeFaceIndices) {
      SMemFree(nodeFaceIndices, 0, 0, 0);
    }
  }

  if (buildFaceIndices) {
    SMemFree(buildFaceIndices, 0, 0, 0);
  }
}
