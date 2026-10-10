#include "Base/Base.h"
#include "Gx/Gx.h"
#include "Services/ParticleSystem2.h"
#include <WowConst.h>
#include "AaBsp.h"
#include <MapDefs.h>

#include "WorldClient/World.h"
#include "WorldClient/Map.h"
#include "WorldClient/WorldParam.h"
#include "WorldClient/DetailDoodad.h"
#include "WorldClient/CSimpleDoodad.h"
#include "DayNight.h"

#include <Ftol.h>

#include "WorldCommon/WorldMath.h"
#include "Images/blit.h"
#include "Services/AsyncFileRead.h"
#include "Services/Texture.h"
#include "Tempest/c3ray.h"
#include "Tempest/caasphere.h"
#include "Tempest/cfacet.h"
#include "Tempest/tempest_intersect.h"

#include <math.h>
#include <float.h>

TSCArray<CGxBuf *, 512> CMapObjGroup::extGxBufFreeList;
TSCArray<CGxBuf *, 512> CMapObjGroup::intGxBufFreeList;
const SMOGxBatch       *CMapObjGroup::sLockGxBatch;
const EGxTexFormat      CMapObjGroup::LIGHTMAP_FORMAT = GxTex_Dxt1;
UINT                    CMapObjGroup::rDrawSharedLiquidFirst;
UINT                    CMapObjGroup::rDrawSharedLiquidToggle;

static float *t[16];

CGxBuf *CMapObjGroup::AllocExtGxBuf(UINT nVerts, UINT nIndices) {
  if (!extGxBufFreeList.Count()) {
    return GxBufCreate(GxBWF_Low, GxVBF_PNT0, nVerts, nIndices, ExtGxBufFill, 0);
  }

  CGxBuf *gxBuf = extGxBufFreeList[extGxBufFreeList.Count() - 1];
  extGxBufFreeList.SetCount(extGxBufFreeList.Count() - 1);
  gxBuf->CountSet(nVerts, nIndices);
  gxBuf->Invalidate(CGxBuf::S_INVALID_DISCARD, CGxBuf::S_INVALID_DISCARD);
  return gxBuf;
}

void CMapObjGroup::FreeExtGxBuf(CGxBuf *&gxBuf) {
  ASSERT(gxBuf);
  extGxBufFreeList.SetCount(extGxBufFreeList.Count() + 1);
  extGxBufFreeList[extGxBufFreeList.Count() - 1] = gxBuf;
  gxBuf = 0;
}

void CMapObjGroup::ExtGxBufFill(CGxBufCommand &cmd, CGxBuf *buf) {
  CMapObjGroup *group = (CMapObjGroup *)buf->UserArg();
  FATALASSERT(group);
  group->ExtGxBufFillVertex(cmd, buf);
  group->GxBufFillIndex(cmd, buf);
}

CGxBuf *CMapObjGroup::AllocIntGxBuf(UINT nVerts, UINT nIndices) {
  if (!intGxBufFreeList.Count()) {
    return GxBufCreate(GxBWF_Low, GxVBF_PNT0, nVerts, nIndices, IntGxBufFill, 0);
  }

  CGxBuf *gxBuf = intGxBufFreeList[intGxBufFreeList.Count() - 1];
  intGxBufFreeList.SetCount(intGxBufFreeList.Count() - 1);
  gxBuf->CountSet(nVerts, nIndices);
  gxBuf->Invalidate(CGxBuf::S_INVALID_DISCARD, CGxBuf::S_INVALID_DISCARD);
  return gxBuf;
}

void CMapObjGroup::FreeIntGxBuf(CGxBuf *&gxBuf) {
  ASSERT(gxBuf);
  intGxBufFreeList.SetCount(intGxBufFreeList.Count() + 1);
  intGxBufFreeList[intGxBufFreeList.Count() - 1] = gxBuf;
  gxBuf = 0;
}

void CMapObjGroup::IntGxBufFill(CGxBufCommand &cmd, CGxBuf *buf) {
  CMapObjGroup *group = (CMapObjGroup *)buf->UserArg();
  FATALASSERT(group);
  group->IntGxBufFillVertex(cmd, buf);
  group->GxBufFillIndex(cmd, buf);
}

void CMapObjGroup::Destroy() {
  UINT i;

  for (i = 0; i < extGxBufFreeList.Count(); ++i) {
    GxBufDestroy(extGxBufFreeList[i]);
  }
  extGxBufFreeList.SetCount(0);

  for (i = 0; i < intGxBufFreeList.Count(); ++i) {
    GxBufDestroy(intGxBufFreeList[i]);
  }
  intGxBufFreeList.SetCount(0);
}

CMapObjGroup::CMapObjGroup() {
}

CMapObjGroup::~CMapObjGroup() {
}

class BspQuery {
 public:
  enum {
    MAX_FACES = 0x1000
  };

  static WORD testFaces[MAX_FACES];
  static UINT testFaceSub;
  static WORD hitFaces[MAX_FACES];
  static UINT hitFaceSub;

  ~BspQuery() {
    testFaceSub = 0;
    hitFaceSub = 0;
  }
};

WORD BspQuery::testFaces[BspQuery::MAX_FACES];
UINT BspQuery::testFaceSub;
WORD BspQuery::hitFaces[BspQuery::MAX_FACES];
UINT BspQuery::hitFaceSub;

class BspQuery_Segment : public BspQuery {
 private:
  void operator=(const BspQuery_Segment &);

 public:
  SMOPoly                  *faces;
  const NTempest::C3Vector *vertexList;
  float                    *hitT;
  float                     origHitT;
  NTempest::C3Ray           ray;
  float                     oosegMag;
  float                     maxT;
  WORD                      faceIgnoreFlags;

  void operator()(WORD faceIndex) {
    if (faces[faceIndex].flags & faceIgnoreFlags) {
      return;
    }

    FATALASSERT(testFaceSub < MAX_FACES);
    testFaces[testFaceSub++] = faceIndex;
    faces[faceIndex].flags |= 0x80;

    UINT             vertIdx = 3 * faceIndex;
    NTempest::CFacet facet(vertexList[vertIdx], vertexList[vertIdx + 1], vertexList[vertIdx + 2]);
    float            t;
    if (NTempest::Intersect(ray, facet, &t, 0) && t >= 0.0f && t <= maxT) {
      maxT = t;
      hitFaces[0] = faceIndex;
      hitFaceSub = 1;
      *hitT = t * oosegMag;
      *hitT = min(origHitT, *hitT);
    }
  }

  BspQuery_Segment(SMOPoly *faces, const NTempest::C3Vector *vertexList, const NTempest::C3Segment &seg, float *hitT, WORD faceIgnoreFlags)
      : faces(faces), vertexList(vertexList), hitT(hitT), faceIgnoreFlags(faceIgnoreFlags) {
    ray.origin = seg.start;
    ray.dir = seg.Direction();
    float segMag = NTempest::CMath::sqrt_(ray.dir.SquaredMag());
    maxT = segMag * *hitT;
    origHitT = *hitT;
    oosegMag = 1.0f / segMag;
    ray.dir.x = oosegMag * ray.dir.x;
    ray.dir.y = oosegMag * ray.dir.y;
    ray.dir.z = oosegMag * ray.dir.z;
  }

  ~BspQuery_Segment() {
    while (testFaceSub) {
      --testFaceSub;
      faces[testFaces[testFaceSub]].flags &= ~0x80;
    }
  }
};

bool QueryCull(const NTempest::CAaBox &aaBox, const NTempest::C3Vector *verts) {
  for (UINT cc = 0; cc < 3; ++cc) {
    UINT signMax = 0xFFFFFFFF;
    UINT signMin = 0xFFFFFFFF;

    for (UINT vertex = 0; vertex < 3; ++vertex) {
      float dmax = aaBox.t[cc] - verts[vertex][cc];
      signMax &= *(UINT *)&dmax & 0x80000000;

      float dmin = verts[vertex][cc] - aaBox.b[cc];
      signMin &= *(UINT *)&dmin & 0x80000000;
    }

    if (signMax || signMin) {
      return 1;
    }
  }

  return 0;
}

bool QueryCull(const CWFrustum &frustum, const NTempest::C3Vector *verts) {
  UINT cc[3];
  frustum.Cull(verts[0], cc[0]);
  frustum.Cull(verts[1], cc[1]);
  frustum.Cull(verts[2], cc[2]);
  return (cc[0] & cc[1] & cc[2]) ? true : false;
}

template <class VOLUME>
class BspQuery_Volume : public BspQuery {
 private:
  SMOPoly                  *faces;
  const NTempest::C3Vector *vertexList;
  const VOLUME             &volume;
  WORD                      faceIgnoreFlags;

  void operator=(const BspQuery_Volume &);

 public:
  void operator()(WORD faceIndex) {
    if (faces[faceIndex].flags & faceIgnoreFlags) {
      return;
    }

    FATALASSERT(testFaceSub < MAX_FACES);
    testFaces[testFaceSub++] = faceIndex;
    faces[faceIndex].flags |= 0x80;

    if (!QueryCull(volume, &vertexList[3 * faceIndex])) {
      FATALASSERT(hitFaceSub < MAX_FACES);
      hitFaces[hitFaceSub++] = faceIndex;
    }
  }

  BspQuery_Volume(SMOPoly *faces, const NTempest::C3Vector *vertexList, const VOLUME &volume, WORD faceIgnoreFlags)
      : faces(faces), vertexList(vertexList), volume(volume), faceIgnoreFlags(faceIgnoreFlags) {
  }

  ~BspQuery_Volume() {
    while (testFaceSub) {
      --testFaceSub;
      faces[testFaces[testFaceSub]].flags &= ~0x80;
    }
  }
};

void CMapObjGroup::GetTrisFromQuery(CWTriData &triData, BspQuery &q, const CMapObjDef *mapObjDef) {
  if (CWorld::enables & CWorld::Enable_ShowQuery) {
    UINT i;
    for (i = 0; i < q.testFaceSub; ++i) {
      UINT vertIdx = 3 * q.testFaces[i];
      CMap::TestQueryAdd(
          NTempest::CFacet(vertexList[vertIdx] * mapObjDef->mat, vertexList[vertIdx + 1] * mapObjDef->mat, vertexList[vertIdx + 2] * mapObjDef->mat),
          NTempest::CImVector(0x7FFF0000), 0
      );
    }
    for (i = 0; i < q.hitFaceSub; ++i) {
      UINT vertIdx = 3 * q.hitFaces[i];
      CMap::TestQueryAdd(
          NTempest::CFacet(vertexList[vertIdx] * mapObjDef->mat, vertexList[vertIdx + 1] * mapObjDef->mat, vertexList[vertIdx + 2] * mapObjDef->mat),
          NTempest::CImVector(0x7F00FF00), 0
      );
    }
  }

  if (!q.hitFaceSub) {
    return;
  }

  CWTriData::Batch *batch = triData.AllocBatch();
  batch->sourceID = (DWORD)mapObjDef;

  WORD *indices = triData.AllocVertexIndices(3 * q.hitFaceSub);
  WORD *tris = triData.AllocTriIndices(q.hitFaceSub);
  batch->matrix = &mapObjDef->mat;
  batch->vertices = vertexList;
  batch->normals = normalList;
  batch->triCount = q.hitFaceSub;

  UINT base = 0;
  for (UINT i = 0; i < q.hitFaceSub; ++i) {
    WORD face = q.hitFaces[i];
    tris[i] = face;
    for (UINT j = 0; j < 3; ++j) {
      WORD index = 3 * face + j;
      indices[base++] = index;
      if (index < batch->minIndex) {
        batch->minIndex = index;
      }
      if (index > batch->maxIndex) {
        batch->maxIndex = index;
      }
    }
  }

  batch->vertexIndices = indices;
  batch->indexCount = 3 * q.hitFaceSub;
  batch->triIndices = tris;
}

bool CMapObjGroup::GetTris(CWTriData &triData, const NTempest::CAaBox &aaBox, const CMapObjDef *mapObjDef, UINT faceIgnoreFlags) {
  BspQuery_Volume<NTempest::CAaBox> q(polyList, vertexList, aaBox, faceIgnoreFlags | 0x80);
  CAaBsp_Query_AaBox<BspQuery_Volume<NTempest::CAaBox> >(aaBsp, q, aaBox);
  GetTrisFromQuery(triData, q, mapObjDef);
  return q.hitFaceSub > 0;
}

bool CMapObjGroup::GetTris(CWTriData &triData, const NTempest::C3Segment &seg, float &maxT, const CMapObjDef *mapObjDef, UINT faceIgnoreFlags) {
  FATALASSERT(maxT >= 0.0f && maxT <= 1.0f);

  BspQuery_Segment q(polyList, vertexList, seg, &maxT, faceIgnoreFlags | 0x80);
  CAaBsp_Query_Segment<BspQuery_Segment>(aaBsp, q, seg);
  GetTrisFromQuery(triData, q, mapObjDef);
  return q.hitFaceSub > 0;
}

bool CMapObjGroup::GetTris(CWTriData &triData, const CWFrustum &frustum, const CMapObjDef *mapObjDef, UINT faceIgnoreFlags) {
  BspQuery_Volume<CWFrustum> q(polyList, vertexList, frustum, faceIgnoreFlags | 0x80);
  NTempest::CAaBox           aaBox = NTempest::CAaBox::Bounding(frustum.corners, 8);
  CAaBsp_Query_AaBox<BspQuery_Volume<CWFrustum> >(aaBsp, q, aaBox);
  GetTrisFromQuery(triData, q, mapObjDef);
  return q.hitFaceSub > 0;
}

void CMapObjGroup::Init() {
  flags = 0;
  frameCount = 0;
  rLevel = 0;
  minimapTag = 0;
  data = 0;
  parent = 0;
  flushTime = 0.0f;
  lightmapTexFlushTime = 0.0f;
  bLoaded = 0;
  asyncObject = 0;
  aaBox.b = NTempest::C3Vector(0.0f, 0.0f, 0.0f);
  aaBox.t = NTempest::C3Vector(0.0f, 0.0f, 0.0f);
  portalStart = 0;
  portalCount = 0;
  groupLiquid = 0;

  UINT i;
  for (i = 0; i < 4; ++i) {
    fogIds[i] = 0;
  }

  for (i = 0; i < 4; ++i) {
    intGxBuf[i] = 0;
    extGxBuf[i] = 0;
  }

  liquidVerts = NTempest::C2iVector(0, 0);
  liquidCorner = NTempest::C3Vector(0.0f, 0.0f, 0.0f);
  liquidMtlId = 0;

  InitPtrs();
}

void CMapObjGroup::InitPtrs() {
  dbgName = 0;
  planeList = 0;
  polyList = 0;
  vertexList = 0;
  normalList = 0;
  textureVertexList = 0;
  indexList = 0;
  batchList = 0;
  lightRefList = 0;
  doodadRefList = 0;
  colorVertexList = 0;
  lightmapVertexList = 0;
  lightmapList = 0;
  lightmapTexList = 0;
  liquidVertexList = 0;
  liquidTileList = 0;
  portalStart = 0;
  portalCount = 0;
  planeCount = 0;
  polyCount = 0;
  vertexCount = 0;
  normalCount = 0;
  textureVertexCount = 0;
  indexCount = 0;
  batchCount = 0;
  lightRefCount = 0;
  doodadRefCount = 0;
  colorVertexCount = 0;
  lightmapVertexCount = 0;
  lightmapCount = 0;
  lightmapTexCount = 0;
  liquidVerts = NTempest::C2iVector(0, 0);
  liquidTiles = NTempest::C2iVector(0, 0);
}

void CMapObjGroup::Clear() {
  FreeLightmaps();

  if (asyncObject) {
    AsyncFileReadDestroyObject(asyncObject);
  }
  asyncObject = 0;

  aaBsp.Clear();
  InitPtrs();

  if (data) {
    SMemFree(data, __FILE__, __LINE__, 0);
  }
  data = 0;

  for (UINT i = 0; i < 4; ++i) {
    if (intGxBuf[i]) {
      FreeIntGxBuf(intGxBuf[i]);
    }
    intGxBuf[i] = 0;

    if (extGxBuf[i]) {
      FreeExtGxBuf(extGxBuf[i]);
    }
    extGxBuf[i] = 0;
  }

  bLoaded = 0;
}

UINT CMapObjGroup::SphereIntersectPoly(const NTempest::CAaSphere &sphere, const UINT numVerts, const WORD *indicies) {
  NTempest::C3Vector origin = vertexList[indicies[0]];
  for (UINT i = 0; i < numVerts - 2; ++i) {
    NTempest::C3Vector edge0 = vertexList[indicies[i + 1]] - origin;
    NTempest::C3Vector edge1 = vertexList[indicies[i + 2]] - origin;
    float              distSq = CWorldMath::TriSqrDistance(sphere.c, origin, edge0, edge1);
    float              radiusSq = sphere.r * sphere.r;
    if (distSq < radiusSq) {
      return 1;
    }
  }

  return 0;
}

bool CMapObjGroup::PointInPoly(const NTempest::C3Vector *p, const UINT numIndicies, const WORD *indicies, const NTempest::C3Vector *n) {
  ASSERT(p);
  ASSERT(indicies);

  NTempest::C3Vector *verts = vertexList;
  ASSERT(verts);

  UINT i;
  for (i = 0; i < numIndicies; ++i) {
    t[i] = &verts[*indicies++].x;
  }

  float nx = fabs(n->x);
  float ns = n->y;
  float ny = fabs(n->y);
  float nz = fabs(n->z);
  UINT  axis0 = 0;
  UINT  axis1 = 2;
  if (nx > ny) {
    ns = n->x;
    ny = nx;
    axis0 = 2;
    axis1 = 1;
  }

  if (nz > ny) {
    ns = -n->z;
    axis0 = 0;
    axis1 = 1;
  }

  UINT next = 1;
  for (i = 0; i < numIndicies; ++i) {
    if (((t[next][axis0] - t[i][axis0]) * ((&p->x)[axis1] - t[next][axis1]) - (t[next][axis1] - t[i][axis1]) * ((&p->x)[axis0] - t[next][axis0])) *
            ns >
        0.019444443f)
    {
      return false;
    }

    ++next;
    if (next == numIndicies) {
      next = 0;
    }
  }

  return true;
}

bool CMapObjGroup::QueryLightmap(const NTempest::C3Vector &point, WORD polyIdx, NTempest::CImVector &color) {
  static const NTempest::C2iVector xyAxisTable[3] = {NTempest::C2iVector(1, 2), NTempest::C2iVector(2, 0), NTempest::C2iVector(0, 1)};

  UINT vertIdx = 3 * polyIdx;
  FATALASSERT(vertIdx < 65535);
  const SMOPoly &poly = polyList[polyIdx];

  NTempest::C4Plane plane(vertexList[vertIdx], vertexList[vertIdx + 1], vertexList[vertIdx + 2]);

  const NTempest::C2iVector &axes = xyAxisTable[plane.n.MajorAxis()];
  NTempest::C2Vector         projPoint(point[axes.x], point[axes.y]);

  NTempest::C2Vector a(vertexList[vertIdx][axes.x], vertexList[vertIdx][axes.y]);
  NTempest::C2Vector b(vertexList[vertIdx + 1][axes.x], vertexList[vertIdx + 1][axes.y]);
  NTempest::C2Vector c(vertexList[vertIdx + 2][axes.x], vertexList[vertIdx + 2][axes.y]);
  NTempest::C2Vector ab = b - a;
  NTempest::C2Vector ac = c - a;
  projPoint -= a;
  float              ooDenom = 1.0f / (ac.y * ab.x - ab.y * ac.x);
  NTempest::C2Vector bary((ac.y * projPoint.x - projPoint.y * ac.x) * ooDenom, (projPoint.y * ab.x - ab.y * projPoint.x) * ooDenom);

  if (bary.x >= 0.0f && bary.y >= 0.0f && bary.x + bary.y <= 1.0f) {
    NTempest::C2Vector lv(
        (1.0f - bary.x - bary.y) * lightmapVertexList[vertIdx].x + bary.x * lightmapVertexList[vertIdx + 1].x + bary.y * lightmapVertexList[vertIdx + 2].x,
        (1.0f - bary.x - bary.y) * lightmapVertexList[vertIdx].y + bary.x * lightmapVertexList[vertIdx + 1].y + bary.y * lightmapVertexList[vertIdx + 2].y
    );

    static const float LUMEL_SNAP = 1.0f / 256.0f;
    static const float OOLUMEL_SNAP = 1.0f / LUMEL_SNAP;
    NTempest::C2Vector lCorner(floorf(lv.x * OOLUMEL_SNAP) * LUMEL_SNAP, floorf(lv.y * OOLUMEL_SNAP) * LUMEL_SNAP);

    NTempest::C2Vector lInterp = OOLUMEL_SNAP * (lv - lCorner);

    NTempest::C2iVector ltex(NTempest::CMath::ftol_0_256_(lCorner.x * 256.0f), NTempest::CMath::ftol_0_256_(lCorner.y * 256.0f));

    const SMOLightmap    &lightmap = lightmapList[polyIdx];
    const SMOLightmapTex &lightmapTex = lightmapTexList[poly.lightmapTex];

    NTempest::C2iVector next(min(ltex.x + 1, lightmap.x + abs(lightmap.width) - 1), min(ltex.y + 1, lightmap.y + abs(lightmap.height) - 1));

    const UINT SRCSTRIDE = CalcRowStride(GxGetBlitFormat(GxTex_Dxt1), 256);

    NTempest::CRgb565   decomp[8][8];
    NTempest::C2iVector first(ltex.x >> 2, ltex.y >> 2);
    NTempest::C2iVector last(next.x >> 2, next.y >> 2);
    NTempest::C2iVector dxtex(first.x * 4, first.y * 4);

    Blit(
        NTempest::C2iVector((last.x - first.x) * 4 + 4, (last.y - first.y) * 4 + 4), BlitAlpha_0,
        lightmapTex.texels + 8 * (first.x + (first.y << 6)), SRCSTRIDE, GxGetBlitFormat(GxTex_Dxt1), decomp, 16, BlitFormat_Rgb565
    );

    UINT                fracY = Fast_ftol(lInterp.y * 256.0f);
    UINT                fracX = Fast_ftol(lInterp.x * 256.0f);
    NTempest::C2iVector dtex(ltex.x - dxtex.x, ltex.y - dxtex.y);
    NTempest::C2iVector dtexNext(next.x - dxtex.x, next.y - dxtex.y);
    NTempest::CRgb565  &topRight = decomp[dtex.y][dtexNext.x];
    NTempest::CRgb565  &topLeft = decomp[dtex.y][dtex.x];
    NTempest::CRgb565  &bottomRight = decomp[dtexNext.y][dtexNext.x];
    NTempest::CRgb565  &bottomLeft = decomp[dtexNext.y][dtex.x];

    NTempest::CRgb565 min;
    min.r = (BYTE)(topRight.r + ((WORD)(fracX * (topLeft.r - topRight.r)) >> 8));
    min.g = (BYTE)(topRight.g + ((WORD)(fracX * (topLeft.g - topRight.g)) >> 8));
    min.b = (BYTE)(topRight.b + ((WORD)(fracX * (topLeft.b - topRight.b)) >> 8));

    BYTE bottomR = bottomRight.r + ((WORD)(fracX * (bottomLeft.r - bottomRight.r)) >> 8);
    BYTE bottomG = bottomRight.g + ((WORD)(fracX * (bottomLeft.g - bottomRight.g)) >> 8);
    BYTE bottomB = bottomRight.b + ((WORD)(fracX * (bottomLeft.b - bottomRight.b)) >> 8);

    min.r = (BYTE)(bottomR + ((WORD)(fracY * (min.r - bottomR)) >> 8));
    min.g = (BYTE)(bottomG + ((WORD)(fracY * (min.g - bottomG)) >> 8));
    min.b = (BYTE)(bottomB + ((WORD)(fracY * (min.b - bottomB)) >> 8));

    static const BYTE minval = 24;
    color = min;
    if (color.r <= minval) {
      color.r = minval;
    }
    if (color.g <= minval) {
      color.g = minval;
    }
    if (color.b <= minval) {
      color.b = minval;
    }

    return 1;
  }

  return 0;
}

bool CMapObjGroup::QueryLightmap(const NTempest::C3Segment &seg, NTempest::CImVector &color) {
  if (flags & 0x48) {
    return true;
  }

  bool      hit = false;
  float     thisT = 1.0f;
  CWTriData triData;
  if (GetTris(triData, seg, thisT, 0, 8)) {
    hit = QueryLightmap(seg.Point(thisT), triData.GetBatch(0).triIndices[0], color);
  }
  return hit;
}

bool CMapObjGroup::QueryLiquidFishable(const NTempest::C3Vector &pos, int &fishable) {
  if (!(flags & 0x1000)) {
    fishable = 0;
    return true;
  }

  NTempest::C2Vector subf = OOSMOLTILE_SIZE * NTempest::C2Vector(pos.y - liquidCorner.y, -(pos.x - liquidCorner.x));
  int                tileX = floor(subf.x);
  int                tileY = floor(subf.y);
  if (tileX >= 0 && tileY >= 0 && tileX < liquidTiles.x && tileY < liquidTiles.y) {
    fishable = liquidTileList[tileY * liquidTiles.x + tileX].GetFishable();
  }

  return true;
}

bool CMapObjGroup::QueryLiquidStatus(const NTempest::C3Vector &pos, UINT &liquid, float &surface, NTempest::C3Vector &dir) {
  if (groupLiquid != LIQUID_NONE) {
    liquid = groupLiquid;
    surface = FLT_MAX;
    dir = NTempest::C3Vector(0.0f, 0.0f, 0.0f);
    return 1;
  }

  if (!(flags & 0x1000)) {
    return 0;
  }

  NTempest::C2Vector  subf = OOSMOLTILE_SIZE * NTempest::C2Vector(pos.y - liquidCorner.y, -(pos.x - liquidCorner.x));
  NTempest::C2iVector subi;
  subi.x = floor(subf.x);
  subi.y = floor(subf.y);
  if (subi.x < 0 || subi.y < 0 || subi.x >= liquidTiles.x || subi.y >= liquidTiles.y) {
    return 0;
  }

  UINT tile = liquidTileList[subi.y * liquidTiles.x + subi.x].GetLiquid();
  if (tile == LIQUID_NONE) {
    return 0;
  }

  NTempest::C2Vector frac(subf.x - subi.x, subf.y - subi.y);
  UINT               vertex = subi.y * liquidVerts.x + subi.x;
  UINT               next = vertex + liquidVerts.x;
  tile &= 3;
  switch (tile) {
    case 0:
    case 4:
    case 8: {
      float h0 = liquidVertexList[vertex].waterVert.height +
                 (liquidVertexList[vertex + 1].waterVert.height - liquidVertexList[vertex].waterVert.height) * frac.x;
      float h1 = liquidVertexList[next].waterVert.height +
                 (liquidVertexList[next + 1].waterVert.height - liquidVertexList[next].waterVert.height) * frac.x;
      float height = h0 + (h1 - h0) * frac.y;
      if (height <= pos.z) {
        return 0;
      }
      surface = height;
      dir = NTempest::C3Vector(0.0f, 0.0f, 0.0f);
      liquid = tile;
      return 1;
    }
    case 2:
    case 3:
    case 6:
    case 7: {
      float h0 = liquidVertexList[vertex].magmaVert.height +
                 (liquidVertexList[vertex + 1].magmaVert.height - liquidVertexList[vertex].magmaVert.height) * frac.x;
      float h1 = liquidVertexList[next].magmaVert.height +
                 (liquidVertexList[next + 1].magmaVert.height - liquidVertexList[next].magmaVert.height) * frac.x;
      float height = h0 + (h1 - h0) * frac.y;
      if (height <= pos.z) {
        return 0;
      }
      surface = height;
      dir = NTempest::C3Vector(0.0f, 0.0f, 0.0f);
      liquid = tile;
      return 1;
    }
  }

  return 0;
}
