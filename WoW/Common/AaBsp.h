#ifndef WOW_COMMON_AABSP_H
#define WOW_COMMON_AABSP_H

#include "Tempest/caabox.h"
#include "Tempest/c3segment.h"

class CAaBspNode {
 public:
  unsigned short flags;
  unsigned short negChild;
  unsigned short posChild;
  unsigned short nFaces;
  unsigned long  faceStart;
  float          planeDist;
};

class CAaBsp {
 public:
  ~CAaBsp();
  void Clear();
  void Set(CAaBspNode *nodeList, unsigned int nNodes, unsigned short *faceIndices, unsigned int nFaceIndices, NTempest::CAaBox &box);

 private:
  void Init();
  void Free();

 public:
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
  CAaBsp_Query(CAaBsp &bsp, QUERY &query) : bsp(bsp), query(query) {
  }

 protected:
  CAaBsp &bsp;
  QUERY  &query;
};

template <class QUERY>
class CAaBsp_Query_Segment : public CAaBsp_Query<QUERY> {
 public:
  CAaBsp_Query_Segment(CAaBsp &bsp, QUERY &query, const NTempest::C3Segment &seg) : CAaBsp_Query<QUERY>(bsp, query) {
    GetFaceIndices(0, seg, bsp.aaBox);
  }

 private:
  void GetFaceIndices(unsigned int nodeIndex, const NTempest::C3Segment &seg, const NTempest::CAaBox &qbBox) {
    CAaBspNode *node = &this->bsp.nodes[nodeIndex];
    if (node->flags & 4) {
      for (unsigned int i = 0; i < node->nFaces; ++i) {
        this->query(this->bsp.nodeFaceIndices[node->faceStart + i]);
      }
      return;
    }

    unsigned int axis = node->flags & 3;
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

#endif
