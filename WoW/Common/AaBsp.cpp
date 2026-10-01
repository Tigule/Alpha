#include <Base/Base.h>

#include "AaBsp.h"
#include "Tempest/c4plane.h"

#include <storm.h>
#include <string.h>

static BYTE s_faceBitsMask[8] = {1, 2, 4, 8, 16, 32, 64, 128};

class CFaceQuery {
 public:
  WORD *indices;
  UINT  maxCount;
  UINT  count;
  BYTE  faceBits[8192];

  CFaceQuery() : indices(0), maxCount(0), count(0) {
    memset(faceBits, 0, sizeof(faceBits));
  }

  void AddFace(WORD face) {
    if (!(faceBits[face >> 3] & s_faceBitsMask[face & 7])) {
      indices[count++] = face;
      FATALASSERT(count < maxCount);
    }
  }

  void ClearFaceBits() {
    for (UINT i = 0; i < count; ++i) {
      faceBits[indices[i] >> 3] = 0;
    }
  }
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

  UINT i;
  for (i = 0; i < nNodes; ++i) {
    nodes[i].flags = rhs.nodes[i].flags;
    nodes[i].negChild = rhs.nodes[i].negChild;
    nodes[i].posChild = rhs.nodes[i].posChild;
    nodes[i].nFaces = rhs.nodes[i].nFaces;
    nodes[i].faceStart = rhs.nodes[i].faceStart;
    nodes[i].planeDist = rhs.nodes[i].planeDist;
  }

  nNodeFaceIndices = rhs.nNodeFaceIndices;
  nodeFaceIndices = static_cast<WORD *>(SMemAlloc(sizeof(WORD) * nNodeFaceIndices, 0, 0, 0));
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

void CAaBsp::Create(NTempest::C3Vector *vertices, UINT nVertices, WORD *faceVertexIndices, UINT nFaceVertexIndices) {
  FATALASSERT(vertices);
  FATALASSERT(nVertices < 0x40000);
  FATALASSERT(faceVertexIndices);
  FATALASSERT(nFaceVertexIndices < 0x40000);

  this->nFaceVertexIndices = nFaceVertexIndices;
  bFree = 1;
  this->vertices = vertices;
  this->nVertices = nVertices;
  this->faceVertexIndices = faceVertexIndices;
  aaBox = NTempest::CAaBox::Bounding(vertices, nVertices);

  if (!nodes) {
    nodes = static_cast<CAaBspNode *>(SMemAlloc(0x100000, 0, 0, 0));
    FATALASSERT(nodes);
  }
  nodeSize = 0x10000;
  nodeNext = 0;
  nNodes = 0;

  if (!nodeFaceIndices) {
    nodeFaceIndices = static_cast<WORD *>(SMemAlloc(0x80000, 0, 0, 0));
    FATALASSERT(nodeFaceIndices);
  }
  nodeFaceIndicesSize = 0x40000;
  nodeFaceIndicesNext = 0;
  nNodeFaceIndices = 0;

  if (!buildFaceIndices) {
    buildFaceIndices = static_cast<WORD *>(SMemAlloc(0x400000, 0, 0, 0));
    FATALASSERT(buildFaceIndices);
  }
  buildFaceIndicesSize = 0x200000;
  buildFaceIndicesNext = 0;

  UINT faceCount = nFaceVertexIndices / 3;
  WORD *faceIndices = AllocBuildFaceIndices(faceCount);
  for (WORD i = 0; i < faceCount; ++i) {
    faceIndices[i] = i;
  }

  BuildTree(faceIndices, faceCount);
  FreeBuildFaceIndices(faceCount);
  rootNode = nodes;
  avgNodeFaces /= nNodes;
}

void CAaBsp::Set(CAaBspNode *nodeList, UINT nNodes, WORD *faceIndices, UINT nFaceIndices, const NTempest::CAaBox &box) {
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

WORD *CAaBsp::AllocBuildFaceIndices(UINT count) {
  FATALASSERT(buildFaceIndicesNext + count < 0x200000);

  WORD *indices = &buildFaceIndices[buildFaceIndicesNext];
  buildFaceIndicesNext += count;
  return indices;
}

void CAaBsp::FreeBuildFaceIndices(UINT count) {
  buildFaceIndicesNext -= count;
}

WORD CAaBsp::AllocNode() {
  FATALASSERT(nNodes < 0x10000);

  WORD nodeIndex = nodeNext;
  ++nodeNext;
  ++nNodes;
  return nodeIndex;
}

DWORD CAaBsp::AllocNodeFaceIndices(UINT count) {
  FATALASSERT(nodeFaceIndicesNext + count < 0x40000);

  DWORD faceStart = nodeFaceIndicesNext;
  nodeFaceIndicesNext += count;
  nNodeFaceIndices += count;
  return faceStart;
}

WORD CAaBsp::BuildTree(WORD *buildFaceIndices, UINT count) {
  UINT posCount;
  if (treeDepth >= 8 || count <= 16) {
    WORD leafIndex = AllocNode();
    nodes[leafIndex].flags = CAaBspNode::Flag_Leaf;
    nodes[leafIndex].negChild = CAaBspNode::Flag_NoChild;
    nodes[leafIndex].posChild = CAaBspNode::Flag_NoChild;
    nodes[leafIndex].faceStart = AllocNodeFaceIndices(count);
    nodes[leafIndex].nFaces = count;
    nodes[leafIndex].planeDist = 0.0f;

    for (posCount = 0; posCount < nodes[leafIndex].nFaces; ++posCount) {
      nodeFaceIndices[nodes[leafIndex].faceStart + posCount] = buildFaceIndices[posCount];
    }
    avgNodeFaces += count;
    return leafIndex;
  }

  UINT  axis;
  float dist;
  ChoosePlane(axis, dist, buildFaceIndices, count);
  WORD nodeIndex = AllocNode();

  nodes[nodeIndex].flags = axis;
  nodes[nodeIndex].planeDist = dist;
  nodes[nodeIndex].faceStart = 0;
  nodes[nodeIndex].nFaces = 0;

  WORD *posIndices = AllocBuildFaceIndices(count);
  WORD *negIndices = AllocBuildFaceIndices(count);
  posCount = 0;
  UINT negCount = 0;
  PartitionFaceList(axis, dist, buildFaceIndices, count, posIndices, posCount, negIndices, negCount);

  ++treeDepth;
  if (posCount) {
    nodes[nodeIndex].posChild = BuildTree(posIndices, posCount);
  } else {
    nodes[nodeIndex].posChild = CAaBspNode::Flag_NoChild;
  }
  if (negCount) {
    nodes[nodeIndex].negChild = BuildTree(negIndices, negCount);
  } else {
    nodes[nodeIndex].negChild = CAaBspNode::Flag_NoChild;
  }
  --treeDepth;

  FreeBuildFaceIndices(count);
  FreeBuildFaceIndices(count);

  return nodeIndex;
}

void CAaBsp::GenBoundingBox(NTempest::CAaBox &aaBox, WORD *buildFaceIndices, UINT count) {
  aaBox.t = NTempest::C3Vector(-FLT_MAX);
  aaBox.b = NTempest::C3Vector(FLT_MAX);

  UINT n;
  for (n = 0; n < count; ++n) {
    for (UINT i = 0; i < 3; ++i) {
      NTempest::C3Vector &vertex = vertices[faceVertexIndices[3 * buildFaceIndices[n] + i]];
      aaBox.b = NTempest::C3Vector(
          vertex.x > aaBox.b.x ? aaBox.b.x : vertex.x, vertex.y > aaBox.b.y ? aaBox.b.y : vertex.y,
          vertex.z > aaBox.b.z ? aaBox.b.z : vertex.z
      );
      aaBox.t = NTempest::C3Vector(
          aaBox.t.x <= vertex.x ? vertex.x : aaBox.t.x, aaBox.t.y <= vertex.y ? vertex.y : aaBox.t.y,
          aaBox.t.z <= vertex.z ? vertex.z : aaBox.t.z
      );
    }
  }
}

void CAaBsp::ChoosePlane(UINT &bestAxis, float &bestDist, WORD *buildFaceIndices, UINT count) {
  float bestPlaneDist = 0.0f;
  NTempest::CAaBox aaBox;
  UINT bestPlaneAxis = 0;
  float bestPlaneScore = 999999.875f;
  GenBoundingBox(aaBox, buildFaceIndices, count);

  for (UINT axis = 0; axis < 3; ++axis) {
    for (int dist = static_cast<int>(aaBox.b[axis] - 0.5f);
         dist <= static_cast<int>(aaBox.t[axis] - 0.5f);
         dist += (static_cast<int>((aaBox.t[axis] - aaBox.b[axis]) * 0.0625f - 0.5f) < 1
                      ? 1
                      : static_cast<int>((aaBox.t[axis] - aaBox.b[axis]) * 0.0625f - 0.5f))) {
      UINT f = 0;
      UINT b = 0;
      UINT o = 0;
      UINT s = 0;

      UINT n;
      for (n = 0; n < count; ++n) {
        WORD  face = buildFaceIndices[n];
        WORD *faceVertices = &faceVertexIndices[3 * face];
        UINT  front = 0;
        UINT  back = 0;
        UINT  on = 0;

        for (UINT j = 0; j < 3; ++j) {
          NTempest::C3Vector &vertex = vertices[faceVertices[j]];
          float distance = vertex.z * s_axisNormalTable[axis].z + vertex.y * s_axisNormalTable[axis].y +
                           vertex.x * s_axisNormalTable[axis].x - dist;
          if (distance > 0.0f) {
            ++front;
          } else if (0.0f > distance) {
            ++back;
          } else {
            ++front;
            ++back;
            ++on;
          }
        }

        if (on == 3) {
          ++o;
        } else if (front == 3) {
          ++f;
        } else if (back == 3) {
          ++b;
        } else {
          ++s;
        }
      }

      double score = NTempest::CMath::fabs_(static_cast<double>(f - b));
      score += static_cast<int>(2 * s);
      score += static_cast<int>(o);
      if (score && score < bestPlaneScore) {
        bestPlaneAxis = axis;
        bestPlaneDist = static_cast<float>(dist);
        bestPlaneScore = score;
      }
    }
  }

  bestAxis = bestPlaneAxis;
  bestDist = bestPlaneDist;
}

void CAaBsp::PartitionFaceList(
    UINT axis, float dist, WORD *buildFaceIndices, UINT count, WORD *posIndices, UINT &posCount, WORD *negIndices, UINT &negCount
) {
  NTempest::C4Plane plane(s_axisNormalTable[axis], -dist);

  for (UINT i = 0; i < count; ++i) {
    WORD  face = buildFaceIndices[i];
    WORD *faceVertices = &faceVertexIndices[3 * face];
    UINT  front = 0;
    UINT  back = 0;

    for (UINT j = 0; j < 3; ++j) {
      NTempest::C3Vector &vertex = vertices[faceVertices[j]];
      float distance = plane.DistSigned(vertex);
      if (distance > 0.0f) {
        ++front;
      } else if (0.0f > distance) {
        ++back;
      } else {
        ++front;
        ++back;
      }
    }

    if (!back) {
      posIndices[posCount++] = face;
    } else if (!front) {
      negIndices[negCount++] = face;
    } else {
      posIndices[posCount++] = face;
      negIndices[negCount++] = face;
    }
  }
}
