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

  CAaBspNode() {
  }

  unsigned short flags;
  unsigned short negChild;
  unsigned short posChild;
  unsigned short nFaces;
  unsigned long  faceStart;
  float          planeDist;
};

class CAaBsp {
 public:
  enum {
    Plane_Front = 0,
    Plane_Back = 1,
    Plane_On = 2
  };

  CAaBsp();
  ~CAaBsp();

  void Clear();
  void Create(NTempest::C3Vector *vertices, unsigned int nVertices, unsigned short *faceVertexIndices, unsigned int nFaceVertexIndices);
  void Set(
      CAaBspNode *nodeList, unsigned int nNodes, unsigned short *faceIndices, unsigned int nFaceIndices, const NTempest::CAaBox &box
  );

  void                  GetFaceIndices(unsigned int nodeIndex, NTempest::CAaBox &aaBox);
  void                  GetFaceIndices(unsigned int nodeIndex, NTempest::C3Segment &seg);
  void                  GetFaceIndices(CAaBspNode *node);
  const unsigned short *GetFaceIndices() const {
    return nodeFaceIndices;
  }
  unsigned short *GetFaceIndices() {
    return nodeFaceIndices;
  }
  unsigned int GetFaceIndices(NTempest::CAaBox &aaBox, unsigned short *indices, unsigned int maxCount);
  unsigned int GetFaceIndices(NTempest::C3Segment &seg, unsigned short *indices, unsigned int maxCount);

  const CAaBspNode *GetNodeList() const {
    return nodes;
  }
  CAaBspNode *GetNodeList() {
    return nodes;
  }
  unsigned int GetNumNodes() const {
    return nNodes;
  }
  unsigned int GetNumFaceIndices() const {
    return nNodeFaceIndices;
  }
  const NTempest::CAaBox &GetAaBox() const {
    return aaBox;
  }
  void SetAaBox(const NTempest::CAaBox &box) {
    aaBox = box;
  }

  void operator=(const CAaBsp &rhs);

  static NTempest::C3Vector s_axisNormalTable[3];

 private:
  void Init();
  void Free();

  unsigned short *AllocBuildFaceIndices(unsigned int count);
  void            FreeBuildFaceIndices(unsigned int count);
  unsigned short  AllocNode();
  unsigned long   AllocNodeFaceIndices(unsigned int count);
  unsigned short  BuildTree(unsigned short *buildFaceIndices, unsigned int count);
  void            GenBoundingBox(NTempest::CAaBox &aaBox, unsigned short *buildFaceIndices, unsigned int count);
  void            ChoosePlane(unsigned int &bestAxis, float &bestDist, unsigned short *buildFaceIndices, unsigned int count);
  void PartitionFaceList(
      unsigned int axis,
      float dist,
      unsigned short *buildFaceIndices,
      unsigned int count,
      unsigned short *posIndices,
      unsigned int &posCount,
      unsigned short *negIndices,
      unsigned int &negCount
  );

  CAaBspNode         *rootNode;
  CAaBspNode         *nodes;
  unsigned short     *nodeFaceIndices;
  unsigned int        nNodes;
  unsigned int        nNodeFaceIndices;
  unsigned short     *faceVertexIndices;
  unsigned int        nFaceVertexIndices;
  NTempest::C3Vector *vertices;
  unsigned int        nVertices;
  unsigned int        nodeSize;
  unsigned int        nodeNext;
  unsigned int        nodeFaceIndicesSize;
  unsigned int        nodeFaceIndicesNext;
  unsigned short     *buildFaceIndices;
  unsigned int        buildFaceIndicesSize;
  unsigned int        buildFaceIndicesNext;
  unsigned int        treeDepth;
  unsigned int        avgNodeFaces;
  int                 bFree;
  NTempest::CAaBox    aaBox;
};

template <class QUERY>
class CAaBsp_Query {
 public:
  CAaBsp_Query(const CAaBsp &aaBsp, QUERY &f) : aaBsp(aaBsp), f(f) {
  }

 protected:
  void GetFaceIndices(const CAaBspNode *node) {
    const unsigned short *faceIndices = aaBsp.GetFaceIndices();
    for (unsigned int i = 0; i < node->nFaces; ++i) {
      f(faceIndices[node->faceStart + i]);
    }
  }

  const CAaBsp &aaBsp;
  QUERY        &f;

 private:
  void operator=(const CAaBsp_Query &);
};

template <class QUERY>
class CAaBsp_Query_Segment : public CAaBsp_Query<QUERY> {
 public:
  CAaBsp_Query_Segment(const CAaBsp &aaBsp, QUERY &f, const NTempest::C3Segment &seg) : CAaBsp_Query<QUERY>(aaBsp, f) {
    GetFaceIndices(0, seg, aaBsp.GetAaBox());
  }

 private:
  void operator=(const CAaBsp_Query_Segment &);

  void GetFaceIndices(unsigned int nodeIndex, const NTempest::C3Segment &seg, const NTempest::CAaBox &qbBox) {
    const CAaBspNode *node = &this->aaBsp.GetNodeList()[nodeIndex];
    if (node->flags & CAaBspNode::Flag_Leaf) {
      CAaBsp_Query<QUERY>::GetFaceIndices(node);
      return;
    }

    unsigned int axis = node->flags & CAaBspNode::Flag_AxisMask;
    float        segMin = seg.start[axis] < seg.end[axis] ? seg.start[axis] : seg.end[axis];
    float        segMax = seg.start[axis] > seg.end[axis] ? seg.start[axis] : seg.end[axis];
    if (segMax < qbBox.b[axis] || segMin > qbBox.t[axis]) {
      return;
    }

    NTempest::CAaBox negBox(qbBox);
    NTempest::CAaBox posBox(qbBox);
    negBox.t[axis] = node->planeDist;
    posBox.b[axis] = node->planeDist;

    float d0 = seg.start[axis] - node->planeDist;
    float d1 = seg.end[axis] - node->planeDist;
    if (d0 > 0.0f && d1 > 0.0f) {
      if (node->posChild != 0xFFFF) {
        GetFaceIndices(node->posChild, seg, posBox);
      }
      return;
    }
    if (d0 < 0.0f && d1 < 0.0f) {
      if (node->negChild != 0xFFFF) {
        GetFaceIndices(node->negChild, seg, negBox);
      }
      return;
    }

    if (d0 == 0.0f || d1 == 0.0f) {
      if (node->posChild != 0xFFFF) {
        GetFaceIndices(node->posChild, seg, posBox);
      }
      if (node->negChild != 0xFFFF) {
        GetFaceIndices(node->negChild, seg, negBox);
      }
      return;
    }

    float              frac = d0 / (d0 - d1);
    NTempest::C3Vector mid(
        seg.start.x + (seg.end.x - seg.start.x) * frac, seg.start.y + (seg.end.y - seg.start.y) * frac, seg.start.z + (seg.end.z - seg.start.z) * frac
    );
    if (d0 < 0.0f) {
      if (node->negChild != 0xFFFF) {
        NTempest::C3Segment nSeg(seg.start, mid);
        GetFaceIndices(node->negChild, nSeg, negBox);
      }
      if (node->posChild != 0xFFFF) {
        NTempest::C3Segment nSeg(mid, seg.end);
        GetFaceIndices(node->posChild, nSeg, posBox);
      }
    } else {
      if (node->posChild != 0xFFFF) {
        NTempest::C3Segment nSeg(seg.start, mid);
        GetFaceIndices(node->posChild, nSeg, posBox);
      }
      if (node->negChild != 0xFFFF) {
        NTempest::C3Segment nSeg(mid, seg.end);
        GetFaceIndices(node->negChild, nSeg, negBox);
      }
    }
  }
};

template <class QUERY>
class CAaBsp_Query_AaBox : public CAaBsp_Query<QUERY> {
 public:
  CAaBsp_Query_AaBox(const CAaBsp &aaBsp, QUERY &f, const NTempest::CAaBox &aaBox) : CAaBsp_Query<QUERY>(aaBsp, f) {
    GetFaceIndices(0, aaBsp.GetAaBox(), aaBox);
  }

 private:
  void operator=(const CAaBsp_Query_AaBox &);

  void GetFaceIndices(unsigned int nodeIndex, const NTempest::CAaBox &nodeBox, const NTempest::CAaBox &queryBox) {
    const CAaBspNode *node = &this->aaBsp.GetNodeList()[nodeIndex];
    if (node->flags & CAaBspNode::Flag_Leaf) {
      CAaBsp_Query<QUERY>::GetFaceIndices(node);
      return;
    }

    unsigned int axis = node->flags & CAaBspNode::Flag_AxisMask;
    if (nodeBox.t[axis] < queryBox.b[axis] || nodeBox.b[axis] > queryBox.t[axis]) {
      return;
    }

    NTempest::CAaBox posNodeBox(nodeBox);
    NTempest::CAaBox negNodeBox(nodeBox);
    posNodeBox.b[axis] = node->planeDist;
    negNodeBox.t[axis] = node->planeDist;

    if (queryBox.b[axis] <= node->planeDist && queryBox.t[axis] >= node->planeDist) {
      if (node->posChild != 0xFFFF) {
        NTempest::CAaBox posQueryBox(queryBox);
        posQueryBox.b[axis] = node->planeDist;
        GetFaceIndices(node->posChild, posNodeBox, posQueryBox);
      }
      if (node->negChild != 0xFFFF) {
        NTempest::CAaBox negQueryBox(queryBox);
        negQueryBox.t[axis] = node->planeDist;
        GetFaceIndices(node->negChild, negNodeBox, negQueryBox);
      }
    } else if (queryBox.b[axis] > node->planeDist) {
      if (node->posChild != 0xFFFF) {
        GetFaceIndices(node->posChild, posNodeBox, queryBox);
      }
    } else if (node->negChild != 0xFFFF) {
      GetFaceIndices(node->negChild, negNodeBox, queryBox);
    }
  }
};

#endif
