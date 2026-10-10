#include <Base/Base.h>

#include "AaBsp.h"
#include "Tempest/c4plane.h"

#include <Ftol.h>
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
  nodes = (CAaBspNode *)SMemAlloc(sizeof(CAaBspNode) * nNodes, 0, 0, 0);
  FATALASSERT(nodes);
  rootNode = nodes;

  UINT i;
  for (i = 0; i < nNodes; ++i) {
    nodes[i] = rhs.nodes[i];
  }

  nNodeFaceIndices = rhs.nNodeFaceIndices;
  nodeFaceIndices = (WORD *)SMemAlloc(sizeof(WORD) * nNodeFaceIndices, 0, 0, 0);
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

  bFree = 1;
  this->vertices = vertices;
  this->nVertices = nVertices;
  this->faceVertexIndices = faceVertexIndices;
  this->nFaceVertexIndices = nFaceVertexIndices;
  aaBox = NTempest::CAaBox::Bounding(vertices, nVertices);

  if (!nodes) {
    nodes = (CAaBspNode *)SMemAlloc(0x100000, 0, 0, 0);
    FATALASSERT(nodes);
  }
  nodeSize = 0x10000;
  nodeNext = 0;
  nNodes = 0;

  if (!nodeFaceIndices) {
    nodeFaceIndices = (WORD *)SMemAlloc(0x80000, 0, 0, 0);
    FATALASSERT(nodeFaceIndices);
  }
  nodeFaceIndicesSize = 0x40000;
  nodeFaceIndicesNext = 0;
  nNodeFaceIndices = 0;

  if (!buildFaceIndices) {
    buildFaceIndices = (WORD *)SMemAlloc(0x400000, 0, 0, 0);
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
  avgNodeFaces /= nNodes;
  rootNode = nodes;
}

void CAaBsp::Set(CAaBspNode *nodeList, UINT nNodes, WORD *faceIndices, UINT nFaceIndices, const NTempest::CAaBox &box) {
  FATALASSERT(nodeList);
  FATALASSERT(faceIndices);

  nodes = nodeList;
  rootNode = nodeList;
  nodeFaceIndices = faceIndices;
  bFree = 0;
  this->nNodes = nNodes;
  this->nNodeFaceIndices = nFaceIndices;
  aaBox = box;
}

UINT CAaBsp::GetFaceIndices(NTempest::C3Segment &seg, WORD *indices, UINT maxCount) {
  s_faceQuery.indices = indices;
  s_faceQuery.maxCount = maxCount;
  s_faceQuery.count = 0;
  GetFaceIndices(0, seg);
  s_faceQuery.ClearFaceBits();
  return s_faceQuery.count;
}

UINT CAaBsp::GetFaceIndices(NTempest::CAaBox &aaBox, WORD *indices, UINT maxCount) {
  s_faceQuery.indices = indices;
  s_faceQuery.maxCount = maxCount;
  s_faceQuery.count = 0;
  GetFaceIndices(0, aaBox);
  s_faceQuery.ClearFaceBits();
  return s_faceQuery.count;
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
  if (treeDepth >= 8 || count <= 16) {
    WORD        leafIndex = AllocNode();
    CAaBspNode *node = &nodes[leafIndex];
    node->flags = CAaBspNode::Flag_Leaf;
    node->posChild = CAaBspNode::Flag_NoChild;
    node->negChild = CAaBspNode::Flag_NoChild;
    node->faceStart = AllocNodeFaceIndices(count);
    node->nFaces = count;
    node->planeDist = 0.0f;

    for (UINT n = 0; n < node->nFaces; ++n) {
      nodeFaceIndices[node->faceStart + n] = buildFaceIndices[n];
    }
    avgNodeFaces += count;
    return leafIndex;
  }

  UINT  axis;
  float dist;
  ChoosePlane(axis, dist, buildFaceIndices, count);

  WORD        nodeIndex = AllocNode();
  CAaBspNode *node = &nodes[nodeIndex];
  node->flags = axis;
  node->planeDist = dist;
  node->faceStart = 0;
  node->nFaces = 0;

  WORD *posIndices = AllocBuildFaceIndices(count);
  WORD *negIndices = AllocBuildFaceIndices(count);
  UINT  posCount = 0;
  UINT  negCount = 0;
  PartitionFaceList(axis, dist, buildFaceIndices, count, posIndices, posCount, negIndices, negCount);

  ++treeDepth;
  if (posCount) {
    node->posChild = BuildTree(posIndices, posCount);
  } else {
    node->posChild = CAaBspNode::Flag_NoChild;
  }
  if (negCount) {
    node->negChild = BuildTree(negIndices, negCount);
  } else {
    node->negChild = CAaBspNode::Flag_NoChild;
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
    UINT face = buildFaceIndices[n] * 3;
    for (UINT i = 0; i < 3; ++i) {
      NTempest::C3Vector &vertex = vertices[faceVertexIndices[face + i]];
      aaBox.Enclose(vertex);
    }
  }
}

void CAaBsp::ChoosePlane(UINT &bestAxis, float &bestDist, WORD *buildFaceIndices, UINT count) {
  UINT             bestPlaneAxis = 0;
  float            bestPlaneDist = 0.0f;
  float            bestPlaneScore = 999999.875f;
  NTempest::CAaBox aaBox;
  GenBoundingBox(aaBox, buildFaceIndices, count);

  for (UINT axis = 0; axis < 3; ++axis) {
    int minDist = Fast_ftol(aaBox.b[axis]);
    int maxDist = Fast_ftol(aaBox.t[axis]);
    int step = max(1, Fast_ftol((aaBox.t[axis] - aaBox.b[axis]) * 0.0625f));
    for (int dist = minDist; dist <= maxDist; dist += step) {
      float fDist = dist;
      UINT o = 0;
      UINT f = 0;
      UINT b = 0;
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
                           vertex.x * s_axisNormalTable[axis].x - fDist;
          if (distance > 0.0f) {
            ++front;
          } else if (distance < 0.0f) {
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

      float score = NTempest::CMath::fabs_((float)(f - b)) + 2 * s + o;
      if (score != 0.0f && score < bestPlaneScore) {
        bestPlaneScore = score;
        bestPlaneAxis = axis;
        bestPlaneDist = fDist;
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

  WORD *buildFaceIndex = buildFaceIndices;
  for (UINT i = 0; i < count; ++i, ++buildFaceIndex) {
    WORD *faceVertices = &faceVertexIndices[3 * *buildFaceIndex];
    UINT  front = 0;
    UINT  back = 0;

    for (UINT j = 0; j < 3; ++j) {
      NTempest::C3Vector &vertex = vertices[faceVertices[j]];
      float distance = plane.DistSigned(vertex);
      if (distance > 0.0f) {
        ++front;
      } else if (distance < 0.0f) {
        ++back;
      } else {
        ++front;
        ++back;
      }
    }

    if (back) {
      if (front) {
        posIndices[posCount] = *buildFaceIndex;
        ++posCount;
        negIndices[negCount] = *buildFaceIndex;
        ++negCount;
      } else {
        negIndices[negCount] = *buildFaceIndex;
        ++negCount;
      }
    } else {
      posIndices[posCount] = *buildFaceIndex;
      ++posCount;
    }
  }
}

void CAaBsp::GetFaceIndices(CAaBspNode *node) {
  WORD *faceIndices = &nodeFaceIndices[node->faceStart];
  for (UINT i = 0; i < node->nFaces; ++i) {
    s_faceQuery.AddFace(faceIndices[i]);
  }
}

void CAaBsp::GetFaceIndices(UINT nodeIndex, NTempest::C3Segment &seg) {
  CAaBspNode *node = &nodes[nodeIndex];
  if (node->flags & CAaBspNode::Flag_Leaf) {
    GetFaceIndices(node);
    return;
  }

  NTempest::C4Plane plane(s_axisNormalTable[node->flags & CAaBspNode::Flag_AxisMask], node->planeDist);
  float             d0 = plane.DistSigned(seg.start);
  float             d1 = plane.DistSigned(seg.end);
  if (d0 == 0.0f && d1 == 0.0f) {
    if (node->posChild != CAaBspNode::Flag_NoChild) {
      GetFaceIndices(node->posChild, seg);
    }
    if (node->negChild != CAaBspNode::Flag_NoChild) {
      GetFaceIndices(node->posChild, seg);
    }
  } else if (!((*(DWORD *)&d0 ^ *(DWORD *)&d1) & 0x80000000)) {
    if (d0 > 0.0f) {
      if (node->posChild != CAaBspNode::Flag_NoChild) {
        GetFaceIndices(node->posChild, seg);
      }
    } else {
      if (node->negChild != CAaBspNode::Flag_NoChild) {
        GetFaceIndices(node->negChild, seg);
      }
    }
  } else {
    float              t = d0 / (d0 - d1);
    NTempest::C3Vector mid = seg.start + (seg.end - seg.start) * t;
    if (d0 > 0.0f) {
      if (node->posChild != CAaBspNode::Flag_NoChild) {
        NTempest::C3Segment nSeg(seg.start, mid);
        GetFaceIndices(node->posChild, nSeg);
      }
      if (node->negChild != CAaBspNode::Flag_NoChild) {
        NTempest::C3Segment nSeg(mid, seg.end);
        GetFaceIndices(node->negChild, nSeg);
      }
    } else {
      if (node->negChild != CAaBspNode::Flag_NoChild) {
        NTempest::C3Segment nSeg(seg.start, mid);
        GetFaceIndices(node->negChild, nSeg);
      }
      if (node->posChild != CAaBspNode::Flag_NoChild) {
        NTempest::C3Segment nSeg(mid, seg.end);
        GetFaceIndices(node->posChild, nSeg);
      }
    }
  }
}

void CAaBsp::GetFaceIndices(UINT nodeIndex, NTempest::CAaBox &aaBox) {
  CAaBspNode *node = &nodes[nodeIndex];
  if (node->flags & CAaBspNode::Flag_Leaf) {
    GetFaceIndices(node);
    return;
  }

  UINT axis = node->flags & CAaBspNode::Flag_AxisMask;
  if (aaBox.b[axis] > node->planeDist) {
    if (node->posChild != CAaBspNode::Flag_NoChild) {
      GetFaceIndices(node->posChild, aaBox);
    }
  } else if (aaBox.t[axis] < node->planeDist) {
    if (node->negChild != CAaBspNode::Flag_NoChild) {
      GetFaceIndices(node->negChild, aaBox);
    }
  } else {
    if (node->posChild != CAaBspNode::Flag_NoChild) {
      NTempest::CAaBox nAaBox = aaBox;
      nAaBox.b[axis] = node->planeDist;
      GetFaceIndices(node->posChild, nAaBox);
    }
    if (node->negChild != CAaBspNode::Flag_NoChild) {
      NTempest::CAaBox nAaBox = aaBox;
      nAaBox.t[axis] = node->planeDist;
      GetFaceIndices(node->posChild, nAaBox);
    }
  }
}
