#include <Base/Base.h>

#include "AaBsp.h"

#include <storm.h>
#include <string.h>

static const unsigned char s_faceBitsMask[8] = {1, 2, 4, 8, 16, 32, 64, 128};

class CFaceQuery {
 public:
  CFaceQuery() : indices(0), maxCount(0), count(0) {
    memset(faceBits, 0, sizeof(faceBits));
  }

  void AddFace(unsigned short face) {
    if (!(faceBits[face >> 3] & s_faceBitsMask[face & 7])) {
      indices[count++] = face;
      FATALASSERT(count < maxCount);
    }
  }

  void ClearFaceBits() {
    for (unsigned int i = 0; i < count; ++i) {
      faceBits[indices[i] >> 3] = 0;
    }
  }

  unsigned short *indices;
  unsigned int    maxCount;
  unsigned int    count;
  unsigned char   faceBits[8192];
};

static CFaceQuery s_faceQuery;

NTempest::C3Vector CAaBsp::s_axisNormalTable[3] = {
    NTempest::C3Vector(1.0f, 0.0f, 0.0f), NTempest::C3Vector(0.0f, 1.0f, 0.0f), NTempest::C3Vector(0.0f, 0.0f, 1.0f)
};

CAaBsp::CAaBsp() {
  Init();
}

void CAaBsp::operator=(const CAaBsp &rhs) {
  FATALASSERT(nodes == 0);
  FATALASSERT(nodeFaceIndices == 0);

  nNodes = rhs.nNodes;
  nodes = static_cast<CAaBspNode *>(SMemAlloc(sizeof(CAaBspNode) * nNodes, 0, 0, 0));
  FATALASSERT(nodes);
  rootNode = nodes;

  unsigned int i;
  for (i = 0; i < nNodes; ++i) {
    nodes[i].flags = rhs.nodes[i].flags;
    nodes[i].negChild = rhs.nodes[i].negChild;
    nodes[i].posChild = rhs.nodes[i].posChild;
    nodes[i].nFaces = rhs.nodes[i].nFaces;
    nodes[i].faceStart = rhs.nodes[i].faceStart;
    nodes[i].planeDist = rhs.nodes[i].planeDist;
  }

  nNodeFaceIndices = rhs.nNodeFaceIndices;
  nodeFaceIndices = static_cast<unsigned short *>(SMemAlloc(sizeof(unsigned short) * nNodeFaceIndices, 0, 0, 0));
  FATALASSERT(nodeFaceIndices);
  for (i = 0; i < nNodeFaceIndices; ++i) {
    nodeFaceIndices[i] = rhs.nodeFaceIndices[i];
  }

  aaBox = rhs.aaBox;
}

CAaBsp::~CAaBsp() {
  Free();
}

void CAaBsp::Clear() {
  Free();
  Init();
}

void CAaBsp::Set(
    CAaBspNode *nodeList,
    unsigned int nNodes,
    unsigned short *faceIndices,
    unsigned int nFaceIndices,
    const NTempest::CAaBox &box
) {
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
