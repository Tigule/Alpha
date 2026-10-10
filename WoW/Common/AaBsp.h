#ifndef WOW_COMMON_AABSP_H
#define WOW_COMMON_AABSP_H

#include "Tempest/caabox.h"
#include "Tempest/c3segment.h"

class CAaBspNode {
 public:
  enum {
    Flag_XAxis = 0,
    Flag_YAxis = 1,
    Flag_ZAxis = 2,
    Flag_AxisMask = 3,
    Flag_Leaf = 4,
    Flag_NoChild = 0xFFFF
  };

  WORD  flags;
  WORD  negChild;
  WORD  posChild;
  WORD  nFaces;
  DWORD faceStart;
  float planeDist;

  CAaBspNode() {
  }
};

class CAaBsp {
 public:
  enum {
    Plane_Front = 0,
    Plane_Back = 1,
    Plane_On = 2
  };

  static NTempest::C3Vector s_axisNormalTable[3];

  CAaBsp();
  ~CAaBsp();

  void Clear();
  void Create(NTempest::C3Vector *vertices, UINT nVertices, WORD *faceVertexIndices, UINT nFaceVertexIndices);
  void Set(CAaBspNode *nodeList, UINT nNodes, WORD *faceIndices, UINT nFaceIndices, const NTempest::CAaBox &box);

  UINT GetFaceIndices(NTempest::C3Segment &seg, WORD *indices, UINT maxCount);
  UINT GetFaceIndices(NTempest::CAaBox &aaBox, WORD *indices, UINT maxCount);
  WORD *GetFaceIndices() {
    return nodeFaceIndices;
  }
  const WORD *GetFaceIndices() const {
    return nodeFaceIndices;
  }

  CAaBspNode *GetNodeList() {
    return nodes;
  }
  const CAaBspNode *GetNodeList() const {
    return nodes;
  }
  UINT GetNumNodes() const {
    return nNodes;
  }
  UINT GetNumFaceIndices() const {
    return nNodeFaceIndices;
  }
  const NTempest::CAaBox &GetAaBox() const {
    return aaBox;
  }
  void SetAaBox(const NTempest::CAaBox &box) {
    aaBox = box;
  }

  void operator=(const CAaBsp &rhs);

 private:
  void Init();
  void Free();

  WORD *AllocBuildFaceIndices(UINT count);
  void  FreeBuildFaceIndices(UINT count);
  WORD  AllocNode();
  DWORD AllocNodeFaceIndices(UINT count);
  WORD  BuildTree(WORD *buildFaceIndices, UINT count);
  void  GenBoundingBox(NTempest::CAaBox &aaBox, WORD *buildFaceIndices, UINT count);
  void  ChoosePlane(UINT &bestAxis, float &bestDist, WORD *buildFaceIndices, UINT count);
  void
  PartitionFaceList(UINT axis, float dist, WORD *buildFaceIndices, UINT count, WORD *posIndices, UINT &posCount, WORD *negIndices, UINT &negCount);
  void GetFaceIndices(CAaBspNode *node);
  void GetFaceIndices(UINT nodeIndex, NTempest::C3Segment &seg);
  void GetFaceIndices(UINT nodeIndex, NTempest::CAaBox &aaBox);

  CAaBspNode         *rootNode;
  CAaBspNode         *nodes;
  WORD               *nodeFaceIndices;
  UINT                nNodes;
  UINT                nNodeFaceIndices;
  WORD               *faceVertexIndices;
  UINT                nFaceVertexIndices;
  NTempest::C3Vector *vertices;
  UINT                nVertices;
  UINT                nodeSize;
  UINT                nodeNext;
  UINT                nodeFaceIndicesSize;
  UINT                nodeFaceIndicesNext;
  WORD               *buildFaceIndices;
  UINT                buildFaceIndicesSize;
  UINT                buildFaceIndicesNext;
  UINT                treeDepth;
  UINT                avgNodeFaces;
  BOOL                bFree;
  NTempest::CAaBox    aaBox;
};

template <class QUERY>
class CAaBsp_Query {
  void operator=(const CAaBsp_Query &);

 protected:
  const CAaBsp &aaBsp;
  QUERY        &f;

  void GetFaceIndices(const CAaBspNode *node) {
    const WORD *faceIndices = &aaBsp.GetFaceIndices()[node->faceStart];
    for (UINT i = 0; i < node->nFaces; ++i) {
      f(faceIndices[i]);
    }
  }

 public:
  CAaBsp_Query(const CAaBsp &aaBsp, QUERY &f) : aaBsp(aaBsp), f(f) {
  }
};

template <class QUERY>
class CAaBsp_Query_Segment : public CAaBsp_Query<QUERY> {
  void operator=(const CAaBsp_Query_Segment &);

  void GetFaceIndices(UINT nodeIndex, const NTempest::C3Segment &seg, const NTempest::CAaBox &qbBox) {
    const CAaBspNode *node = &this->aaBsp.GetNodeList()[nodeIndex];
    if (node->flags & CAaBspNode::Flag_Leaf) {
      CAaBsp_Query<QUERY>::GetFaceIndices(node);
      return;
    }

    UINT axis = node->flags & CAaBspNode::Flag_AxisMask;
    if ((seg.start[axis] < qbBox.b[axis] && seg.end[axis] < qbBox.b[axis]) || (seg.start[axis] > qbBox.t[axis] && seg.end[axis] > qbBox.t[axis])) {
      return;
    }

    NTempest::CAaBox posBox(qbBox);
    posBox.b[axis] = node->planeDist;
    NTempest::CAaBox negBox(qbBox);
    negBox.t[axis] = node->planeDist;

    float d0 = seg.start[axis] - node->planeDist;
    float d1 = seg.end[axis] - node->planeDist;
    if (d0 == 0.0f || d1 == 0.0f) {
      if (node->posChild != CAaBspNode::Flag_NoChild) {
        GetFaceIndices(node->posChild, seg, posBox);
      }
      if (node->negChild != CAaBspNode::Flag_NoChild) {
        GetFaceIndices(node->negChild, seg, negBox);
      }
      return;
    }
    if (d0 > 0.0f && d1 > 0.0f) {
      if (node->posChild != CAaBspNode::Flag_NoChild) {
        GetFaceIndices(node->posChild, seg, posBox);
      }
      return;
    }
    if (d0 < 0.0f && d1 < 0.0f) {
      if (node->negChild != CAaBspNode::Flag_NoChild) {
        GetFaceIndices(node->negChild, seg, negBox);
      }
      return;
    }

    NTempest::C3Vector mid = seg.start + seg.Direction() * (d0 / (d0 - d1));
    if (d0 > 0.0f) {
      if (node->posChild != CAaBspNode::Flag_NoChild) {
        NTempest::C3Segment nSeg;
        nSeg.start = seg.start;
        nSeg.end = mid;
        GetFaceIndices(node->posChild, nSeg, posBox);
      }
      if (node->negChild != CAaBspNode::Flag_NoChild) {
        NTempest::C3Segment nSeg;
        nSeg.start = mid;
        nSeg.end = seg.end;
        GetFaceIndices(node->negChild, nSeg, negBox);
      }
    } else {
      if (node->negChild != CAaBspNode::Flag_NoChild) {
        NTempest::C3Segment nSeg;
        nSeg.start = seg.start;
        nSeg.end = mid;
        GetFaceIndices(node->negChild, nSeg, negBox);
      }
      if (node->posChild != CAaBspNode::Flag_NoChild) {
        NTempest::C3Segment nSeg;
        nSeg.start = mid;
        nSeg.end = seg.end;
        GetFaceIndices(node->posChild, nSeg, posBox);
      }
    }
  }

 public:
  CAaBsp_Query_Segment(const CAaBsp &aaBsp, QUERY &f, const NTempest::C3Segment &seg) : CAaBsp_Query<QUERY>(aaBsp, f) {
    GetFaceIndices(0, seg, aaBsp.GetAaBox());
  }
};

template <class QUERY>
class CAaBsp_Query_AaBox : public CAaBsp_Query<QUERY> {
  void operator=(const CAaBsp_Query_AaBox &);

  void GetFaceIndices(UINT nodeIndex, const NTempest::CAaBox &nodeBox, const NTempest::CAaBox &qbBox) {
    const CAaBspNode *node = &this->aaBsp.GetNodeList()[nodeIndex];
    if (node->flags & CAaBspNode::Flag_Leaf) {
      CAaBsp_Query<QUERY>::GetFaceIndices(node);
      return;
    }

    UINT axis = node->flags & CAaBspNode::Flag_AxisMask;
    if (nodeBox.t[axis] < qbBox.b[axis] || nodeBox.b[axis] > qbBox.t[axis]) {
      return;
    }

    NTempest::CAaBox posBox(qbBox);
    posBox.b[axis] = node->planeDist;
    NTempest::CAaBox negBox(qbBox);
    negBox.t[axis] = node->planeDist;

    if (nodeBox.b[axis] > node->planeDist) {
      if (node->posChild != CAaBspNode::Flag_NoChild) {
        GetFaceIndices(node->posChild, nodeBox, posBox);
      }
      return;
    }
    if (nodeBox.t[axis] < node->planeDist) {
      if (node->negChild != CAaBspNode::Flag_NoChild) {
        GetFaceIndices(node->negChild, nodeBox, negBox);
      }
      return;
    }

    if (node->posChild != CAaBspNode::Flag_NoChild) {
      NTempest::CAaBox nAaBox(nodeBox);
      nAaBox.b[axis] = node->planeDist;
      GetFaceIndices(node->posChild, nAaBox, posBox);
    }
    if (node->negChild != CAaBspNode::Flag_NoChild) {
      NTempest::CAaBox nAaBox(nodeBox);
      nAaBox.t[axis] = node->planeDist;
      GetFaceIndices(node->negChild, nAaBox, negBox);
    }
  }

 public:
  CAaBsp_Query_AaBox(const CAaBsp &aaBsp, QUERY &f, const NTempest::CAaBox &aaBox) : CAaBsp_Query<QUERY>(aaBsp, f) {
    GetFaceIndices(0, aaBox, aaBsp.GetAaBox());
  }
};

#endif
