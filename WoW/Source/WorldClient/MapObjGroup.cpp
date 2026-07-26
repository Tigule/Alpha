#include "WorldClient/CMapObj.h"
#include "WorldClient/World.h"

#include "Gx/Gx.h"
#include "Images/blit.h"
#include "Base/Base.h"
#include "Services/AsyncFileRead.h"
#include "Services/Texture.h"
#include "Tempest/c3ray.h"
#include "Tempest/cfacet.h"
#include "Tempest/tempest_intersect.h"

#include <math.h>
#include <float.h>

TSCArray<CGxBuf *, 512> CMapObjGroup::extGxBufFreeList;
TSCArray<CGxBuf *, 512> CMapObjGroup::intGxBufFreeList;
SMOGxBatch             *CMapObjGroup::sLockGxBatch;
unsigned int            CMapObjGroup::rDrawSharedLiquidFirst;
unsigned int            CMapObjGroup::rDrawSharedLiquidToggle;

void __fastcall CMapObjGroup::UpdateLightmapTex(
    EGxTexCommand cmd,
    unsigned int  w,
    unsigned int  h,
    unsigned int  d,
    unsigned int  mipLevel,
    void         *userArg,
    unsigned int &texelStrideInBytes,
    const void  *&texels
) {
  FATALASSERT(userArg);

  if (cmd == GxTex_Latch) {
    texelStrideInBytes = CalcRowStride(GxGetBlitFormat(GxTex_Dxt1), w);
    texels = userArg;
  }
}

void CMapObjGroup::CreateLightmaps() {
  lightmapTexFlushTime = 30.0f;

  for (unsigned int i = 0; i < lightmapTexCount; ++i) {
    SMOLightmapTex &lightmapTex = lightmapTexList[i];
    if (!lightmapTex.hTexture) {
      EGxTexFormat format = GxCaps().m_texFmtDxt ? GxTex_Dxt1 : GxTex_Rgb565;
      CGxTexFlags  flags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
      lightmapTex.hTexture = TextureCreate("Lightmap", 256, 256, format, GxTex_Dxt1, flags);
      CGxTex *texture = TextureGetGxTex(lightmapTex.hTexture, 1, 0);
      GxTexSetUserData(texture, UpdateLightmapTex, &lightmapTex);
    }
  }
}

class BspQuery {
 public:
  enum {
    MaxFaces = 0x1000
  };

  static unsigned short testFaces[MaxFaces];
  static unsigned int   testFaceSub;
  static unsigned short hitFaces[MaxFaces];
  static unsigned int   hitFaceSub;
};

unsigned char __fastcall QueryCull(const NTempest::CAaBox &aaBox, const NTempest::C3Vector *verts);
unsigned char __fastcall QueryCull(const CWFrustum &frustum, const NTempest::C3Vector *verts);

template <class VOLUME>
class BspQuery_Volume : public BspQuery {
 public:
  BspQuery_Volume(SMOPoly *faces, NTempest::C3Vector *vertexList, const VOLUME &volume, unsigned short faceIgnoreFlags)
      : faces(faces), vertexList(vertexList), volume(volume), faceIgnoreFlags(faceIgnoreFlags) {
  }

  void operator()(unsigned short faceIndex) {
    if (faces[faceIndex].flags & faceIgnoreFlags) {
      return;
    }

    FATALASSERT(testFaceSub < MaxFaces);
    testFaces[testFaceSub++] = faceIndex;
    faces[faceIndex].flags |= 0x80;

    if (!QueryCull(volume, &vertexList[3 * faceIndex])) {
      FATALASSERT(hitFaceSub < MaxFaces);
      hitFaces[hitFaceSub++] = faceIndex;
    }
  }

  SMOPoly             *faces;
  NTempest::C3Vector  *vertexList;
  const VOLUME        &volume;
  unsigned short       faceIgnoreFlags;
};

unsigned short BspQuery::testFaces[BspQuery::MaxFaces];
unsigned int   BspQuery::testFaceSub;
unsigned short BspQuery::hitFaces[BspQuery::MaxFaces];
unsigned int   BspQuery::hitFaceSub;

class BspQuery_Segment : public BspQuery {
 public:
  BspQuery_Segment(SMOPoly *faces, NTempest::C3Vector *vertexList, const NTempest::C3Segment &seg, float *hitT, unsigned short faceIgnoreFlags)
      : faces(faces), vertexList(vertexList), hitT(hitT), origHitT(*hitT), faceIgnoreFlags(faceIgnoreFlags) {
    ray.origin = seg.start;
    NTempest::C3Vector delta = seg.end - seg.start;
    float              segMag = delta.Mag();
    oosegMag = 1.0f / segMag;
    ray.dir.x = delta.x * oosegMag;
    ray.dir.y = delta.y * oosegMag;
    ray.dir.z = delta.z * oosegMag;
    maxT = segMag * *hitT;
  }





  void operator()(unsigned short faceIndex) {
    if (faces[faceIndex].flags & faceIgnoreFlags) {
      return;
    }

    FATALASSERT(testFaceSub < MaxFaces);
    testFaces[testFaceSub++] = faceIndex;
    faces[faceIndex].flags |= 0x80;

    unsigned int     vertIdx = 3 * faceIndex;
    NTempest::CFacet facet(vertexList[vertIdx], vertexList[vertIdx + 1], vertexList[vertIdx + 2]);
    float            t;
    if (NTempest::Intersect(ray, facet, &t, 0) && t >= 0.0f && t <= maxT) {
      maxT = t;
      hitFaces[0] = faceIndex;
      hitFaceSub = 1;
      *hitT = t * oosegMag;
      if (*hitT > origHitT) {
        *hitT = origHitT;
      }
    }
  }

  SMOPoly            *faces;
  NTempest::C3Vector *vertexList;
  float              *hitT;
  float               origHitT;
  NTempest::C3Ray     ray;
  float               oosegMag;
  float               maxT;
  unsigned short      faceIgnoreFlags;
};

CGxBuf *__fastcall CMapObjGroup::AllocExtGxBuf(unsigned int nVerts, unsigned int nIndices) {
  if (!extGxBufFreeList.Count()) {
    return GxBufCreate(GxBWF_Low, static_cast<EGxVertexBufferFormat>(2), nVerts, nIndices, ExtGxBufFill, 0);
  }

  CGxBuf *gxBuf = extGxBufFreeList[extGxBufFreeList.Count() - 1];
  extGxBufFreeList.SetCount(extGxBufFreeList.Count() - 1);
  gxBuf->CountSet(nVerts, nIndices);
  gxBuf->Invalidate(CGxBuf::S_INVALID_DISCARD, CGxBuf::S_INVALID_DISCARD);
  return gxBuf;
}

void __fastcall CMapObjGroup::FreeExtGxBuf(CGxBuf *&gxBuf) {
  unsigned int index;

  ASSERT(gxBuf);
  index = extGxBufFreeList.Count();
  extGxBufFreeList.SetCount(index + 1);
  extGxBufFreeList[index] = gxBuf;
  gxBuf = 0;
}

void __fastcall CMapObjGroup::ExtGxBufFill(CGxBufCommand &cmd, CGxBuf *buf) {
  CMapObjGroup *group = static_cast<CMapObjGroup *>(buf->UserArg());
  FATALASSERT(group);
  group->ExtGxBufFillVertex(cmd, buf);
  group->GxBufFillIndex(cmd, buf);
}

CGxBuf *__fastcall CMapObjGroup::AllocIntGxBuf(unsigned int nVerts, unsigned int nIndices) {
  if (!intGxBufFreeList.Count()) {
    return GxBufCreate(GxBWF_Low, static_cast<EGxVertexBufferFormat>(2), nVerts, nIndices, IntGxBufFill, 0);
  }

  CGxBuf *gxBuf = intGxBufFreeList[intGxBufFreeList.Count() - 1];
  intGxBufFreeList.SetCount(intGxBufFreeList.Count() - 1);
  gxBuf->CountSet(nVerts, nIndices);
  gxBuf->Invalidate(CGxBuf::S_INVALID_DISCARD, CGxBuf::S_INVALID_DISCARD);
  return gxBuf;
}

void __fastcall CMapObjGroup::FreeIntGxBuf(CGxBuf *&gxBuf) {
  unsigned int index;

  ASSERT(gxBuf);
  index = intGxBufFreeList.Count();
  intGxBufFreeList.SetCount(index + 1);
  intGxBufFreeList[index] = gxBuf;
  gxBuf = 0;
}

void __fastcall CMapObjGroup::IntGxBufFill(CGxBufCommand &cmd, CGxBuf *buf) {
  CMapObjGroup *group = static_cast<CMapObjGroup *>(buf->UserArg());
  FATALASSERT(group);
  group->IntGxBufFillVertex(cmd, buf);
  group->GxBufFillIndex(cmd, buf);
}

void __fastcall CMapObjGroup::Destroy() {
  unsigned int i;

  for (i = 0; i < extGxBufFreeList.Count(); ++i) {
    GxBufDestroy(extGxBufFreeList[i]);
  }
  extGxBufFreeList.SetCount(0);

  for (i = 0; i < intGxBufFreeList.Count(); ++i) {
    GxBufDestroy(intGxBufFreeList[i]);
  }
  intGxBufFreeList.SetCount(0);
}

CMapObjGroup::~CMapObjGroup() {
}

void CMapObjGroup::GetTrisFromQuery(CWTriData &triData, BspQuery &q, const CMapObjDef *mapObjDef) {
  if (CWorld::enables & CWorld::Enable_ShowQuery) {
    unsigned int i;
    for (i = 0; i < q.testFaceSub; ++i) {
      unsigned int     vertIdx = 3 * q.testFaces[i];
      NTempest::CFacet facet(vertexList[vertIdx], vertexList[vertIdx + 1], vertexList[vertIdx + 2]);
      CMap::TestQueryAdd(facet, NTempest::CImVector(0x7FFF0000), &mapObjDef->mat);
    }
    for (i = 0; i < q.hitFaceSub; ++i) {
      unsigned int     vertIdx = 3 * q.hitFaces[i];
      NTempest::CFacet facet(vertexList[vertIdx], vertexList[vertIdx + 1], vertexList[vertIdx + 2]);
      CMap::TestQueryAdd(facet, NTempest::CImVector(0x7F00FF00), &mapObjDef->mat);
    }
  }

  if (!q.hitFaceSub) {
    return;
  }

  CWTriData::Batch *batch = triData.AllocBatch();
  batch->matrix = &mapObjDef->mat;
  batch->vertices = vertexList;
  batch->normals = normalList;
  batch->sourceID = reinterpret_cast<unsigned long>(mapObjDef);

  unsigned short *indices = triData.AllocVertexIndices(3 * q.hitFaceSub);
  unsigned short *tris = triData.AllocTriIndices(q.hitFaceSub);
  batch->vertexIndices = indices;
  batch->triIndices = tris;
  batch->indexCount = static_cast<unsigned short>(3 * q.hitFaceSub);
  batch->triCount = static_cast<unsigned short>(q.hitFaceSub);

  unsigned int base = 0;
  for (unsigned int i = 0; i < q.hitFaceSub; ++i) {
    unsigned short face = q.hitFaces[i];
    tris[i] = face;
    for (unsigned int j = 0; j < 3; ++j) {
      unsigned short index = static_cast<unsigned short>(3 * face + j);
      indices[base++] = index;
      if (index < batch->minIndex) {
        batch->minIndex = index;
      }
      if (index > batch->maxIndex) {
        batch->maxIndex = index;
      }
    }
  }
}

bool CMapObjGroup::GetTris(
    CWTriData                 &triData,
    const NTempest::C3Segment &seg,
    float                     &maxT,
    const CMapObjDef          *mapObjDef,
    unsigned int               faceIgnoreFlags
) {
  FATALASSERT(maxT >= 0.0f && maxT <= 1.0f);

  BspQuery_Segment q(polyList, vertexList, seg, &maxT, static_cast<unsigned short>(faceIgnoreFlags | 0x80));
  CAaBsp_Query_Segment<BspQuery_Segment>(aaBsp, q, seg);
  GetTrisFromQuery(triData, q, mapObjDef);

  bool result = q.hitFaceSub != 0;
  while (q.testFaceSub) {
    --q.testFaceSub;
    polyList[q.testFaces[q.testFaceSub]].flags &= ~0x80;
  }
  q.testFaceSub = 0;
  q.hitFaceSub = 0;
  return result;
}

bool CMapObjGroup::GetTris(
    CWTriData              &triData,
    const NTempest::CAaBox &aaBox,
    const CMapObjDef       *mapObjDef,
    unsigned int            faceIgnoreFlags
) {
  BspQuery_Volume<NTempest::CAaBox> q(polyList, vertexList, aaBox, static_cast<unsigned short>(faceIgnoreFlags | 0x80));
  CAaBsp_Query_AaBox<BspQuery_Volume<NTempest::CAaBox> >(aaBsp, q, aaBox);

  GetTrisFromQuery(triData, q, mapObjDef);
  bool result = q.hitFaceSub != 0;

  while (q.testFaceSub) {
    --q.testFaceSub;
    polyList[q.testFaces[q.testFaceSub]].flags &= ~0x80;
  }
  q.hitFaceSub = 0;
  return result;
}

bool CMapObjGroup::GetTris(
    CWTriData          &triData,
    const CWFrustum    &frustum,
    const CMapObjDef   *mapObjDef,
    unsigned int        faceIgnoreFlags
) {
  BspQuery_Volume<CWFrustum> q(polyList, vertexList, frustum, static_cast<unsigned short>(faceIgnoreFlags | 0x80));
  NTempest::CAaBox aaBox = NTempest::CAaBox::Bounding(frustum.corners, 8);
  CAaBsp_Query_AaBox<BspQuery_Volume<CWFrustum> >(aaBsp, q, aaBox);

  GetTrisFromQuery(triData, q, mapObjDef);
  bool result = q.hitFaceSub != 0;

  while (q.testFaceSub) {
    --q.testFaceSub;
    polyList[q.testFaces[q.testFaceSub]].flags &= ~0x80;
  }
  q.hitFaceSub = 0;
  return result;
}

void CMapObjGroup::Init() {
  flags = 0;
  aaBox.b.x = 0.0f;
  aaBox.b.y = 0.0f;
  aaBox.b.z = 0.0f;
  aaBox.t.x = 0.0f;
  aaBox.t.y = 0.0f;
  aaBox.t.z = 0.0f;
  portalStart = 0;
  portalCount = 0;
  groupLiquid = 0;
  fogIds[0] = 0;
  fogIds[1] = 0;
  fogIds[2] = 0;
  fogIds[3] = 0;
  for (unsigned int i = 0; i < 4; ++i) {
    intGxBuf[i] = 0;
    extGxBuf[i] = 0;
  }
  frameCount = 0;
  lightmapTexFlushTime = 0.0f;
  dbgName = 0;
  parent = 0;
  data = 0;
  asyncObject = 0;
  bLoaded = 0;
  flushTime = 0.0f;
  rLevel = 0;
  minimapTag = 0;
  liquidCorner.x = 0.0f;
  liquidCorner.y = 0.0f;
  liquidCorner.z = 0.0f;
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
  liquidVerts.x = 0;
  liquidVerts.y = 0;
  liquidTiles.x = 0;
  liquidTiles.y = 0;
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

  for (unsigned int i = 0; i < 4; ++i) {
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

unsigned int CMapObjGroup::QueryLiquidStatus(NTempest::C3Vector &pos, unsigned int &liquid, float &surface, NTempest::C3Vector &dir) {
  if (groupLiquid != 15) {
    liquid = groupLiquid;
    surface = FLT_MAX;
    dir.x = 0.0f;
    dir.y = 0.0f;
    dir.z = 0.0f;
    return 1;
  }

  if (!(flags & 0x1000)) {
    return 0;
  }

  NTempest::C2Vector subf;
  subf.x = (pos.y - liquidCorner.y) / 4.1666665f;
  subf.y = -(pos.x - liquidCorner.x) / 4.1666665f;

  NTempest::C2iVector subi;
  subi.x = static_cast<int>(floor(subf.x));
  subi.y = static_cast<int>(floor(subf.y));
  if (subi.x < 0 || subi.y < 0 || subi.x >= liquidTiles.x || subi.y >= liquidTiles.y) {
    return 0;
  }

  unsigned int tile = liquidTileList[subi.y * liquidTiles.x + subi.x].flags & 0xF;
  if (tile == 0xF) {
    return 0;
  }

  if ((tile & 3) == 1) {
    return 0;
  }

  NTempest::C2Vector frac;
  frac.x = subf.x - static_cast<float>(subi.x);
  frac.y = subf.y - static_cast<float>(subi.y);

  unsigned int vertex = subi.y * liquidVerts.x + subi.x;
  float        h0 = liquidVertexList[vertex].height + (liquidVertexList[vertex + 1].height - liquidVertexList[vertex].height) * frac.x;
  vertex += liquidVerts.x;
  float h1 = liquidVertexList[vertex].height + (liquidVertexList[vertex + 1].height - liquidVertexList[vertex].height) * frac.x;
  float height = h0 + (h1 - h0) * frac.y;
  if (height <= pos.z) {
    return 0;
  }

  surface = height;
  dir.x = 0.0f;
  dir.y = 0.0f;
  dir.z = 0.0f;
  liquid = tile & 3;
  return 1;
}

bool CMapObjGroup::QueryLightmap(const NTempest::C3Vector &point, unsigned short polyIdx, NTempest::CImVector &color) {
  static NTempest::C2iVector projectionAxes[3] = {NTempest::C2iVector(1, 2), NTempest::C2iVector(2, 0), NTempest::C2iVector(0, 1)};
  NTempest::CRgb565          decomp[8][8];
  NTempest::C4Plane          plane;
  NTempest::C2Vector         b;
  NTempest::C2iVector        dtex;
  SMOPoly                   &poly = polyList[polyIdx];
  const unsigned int         SRCSTRIDE = CalcRowStride(GxGetBlitFormat(GxTex_Dxt1), 256);
  NTempest::C2Vector         ab;
  NTempest::C2iVector        ltex;
  SMOLightmapTex            &lightmapTex = lightmapTexList[poly.lightmapTex];
  NTempest::C2Vector         projPoint;
  NTempest::C2Vector         lInterp;
  NTempest::C2iVector        dtexNext;
  NTempest::C2Vector         a;
  NTempest::C2iVector        dxtex;
  NTempest::C2Vector         lv;
  NTempest::C2Vector         ac;
  NTempest::C2Vector         bary;
  NTempest::C2Vector         lCorner;
  NTempest::CRgb565          min;
  unsigned int               vertIdx = 3 * polyIdx;

  FATALASSERT(vertIdx < 65535);

  NTempest::C3Vector edge0 = vertexList[vertIdx + 1] - vertexList[vertIdx];
  NTempest::C3Vector edge1 = vertexList[vertIdx + 2] - vertexList[vertIdx];
  plane.n = NTempest::C3Vector::Cross(edge0, edge1);
  float ooMag = 1.0f / static_cast<float>(sqrt(plane.n.SquaredMag()));
  plane.n.x *= ooMag;
  plane.n.y *= ooMag;
  plane.n.z *= ooMag;
  plane.d = -NTempest::C3Vector::Dot(plane.n, vertexList[vertIdx]);

  NTempest::C2iVector &axes = projectionAxes[plane.n.MajorAxis()];
  a.x = vertexList[vertIdx][axes.x];
  a.y = vertexList[vertIdx][axes.y];
  b.x = point[axes.x];
  b.y = point[axes.y];
  ab.x = vertexList[vertIdx + 1][axes.x] - a.x;
  ab.y = vertexList[vertIdx + 1][axes.y] - a.y;
  ac.x = vertexList[vertIdx + 2][axes.x] - a.x;
  ac.y = vertexList[vertIdx + 2][axes.y] - a.y;
  projPoint.x = b.x - a.x;
  projPoint.y = b.y - a.y;

  float invDenom = 1.0f / (ac.y * ab.x - ab.y * ac.x);
  bary.x = (ac.y * projPoint.x - projPoint.y * ac.x) * invDenom;
  bary.y = (projPoint.y * ab.x - ab.y * projPoint.x) * invDenom;
  if (bary.x < 0.0f || bary.y < 0.0f || bary.x + bary.y > 1.0f) {
    return 0;
  }

  lv = lightmapVertexList[vertIdx];
  lInterp.x = (1.0f - bary.x - bary.y) * lv.x;
  lInterp.y = (1.0f - bary.x - bary.y) * lv.y;
  lv = lightmapVertexList[vertIdx + 1];
  lInterp.x += bary.x * lv.x;
  lInterp.y += bary.x * lv.y;
  lv = lightmapVertexList[vertIdx + 2];
  lInterp.x += bary.y * lv.x;
  lInterp.y += bary.y * lv.y;

  lCorner.x = floor(lInterp.x * 256.0f) * 0.00390625f;
  lCorner.y = floor(lInterp.y * 256.0f) * 0.00390625f;
  bary.x = (lInterp.x - lCorner.x) * 256.0f;
  bary.y = (lInterp.y - lCorner.y) * 256.0f;
  ltex.x = NTempest::CMath::ftol_0_256_(lCorner.x * 256.0f);
  ltex.y = NTempest::CMath::ftol_0_256_(lCorner.y * 256.0f);

  SMOLightmap &lightmap = lightmapList[polyIdx];
  int          lightmapWidth = lightmap.width < 0 ? -lightmap.width : lightmap.width;
  int          lightmapHeight = lightmap.height < 0 ? -lightmap.height : lightmap.height;
  dtexNext.x = ltex.x + 1;
  dtexNext.y = ltex.y + 1;
  int maxX = lightmap.x + lightmapWidth - 1;
  int maxY = lightmap.y + lightmapHeight - 1;
  if (dtexNext.x >= maxX) {
    dtexNext.x = maxX;
  }
  if (dtexNext.y >= maxY) {
    dtexNext.y = maxY;
  }

  dxtex.x = 4 * (ltex.x >> 2);
  dxtex.y = 4 * (ltex.y >> 2);
  dtex.x = 4 * ((dtexNext.x >> 2) - (ltex.x >> 2)) + 4;
  dtex.y = 4 * ((dtexNext.y >> 2) - (ltex.y >> 2)) + 4;
  Blit(
      dtex, BlitAlpha_0, lightmapTex.texels + 8 * ((ltex.x >> 2) + ((ltex.y >> 2) << 6)), SRCSTRIDE, GxGetBlitFormat(GxTex_Dxt1), decomp, 16,
      BlitFormat_Rgb565
  );

  unsigned int       fracX = static_cast<unsigned int>(bary.x * 256.0f - 0.5f);
  unsigned int       fracY = static_cast<unsigned int>(bary.y * 256.0f - 0.5f);
  NTempest::CRgb565 &topLeft = decomp[ltex.y - dxtex.y][ltex.x - dxtex.x];
  NTempest::CRgb565 &topRight = decomp[ltex.y - dxtex.y][dtexNext.x - dxtex.x];
  NTempest::CRgb565 &bottomLeft = decomp[dtexNext.y - dxtex.y][ltex.x - dxtex.x];
  NTempest::CRgb565 &bottomRight = decomp[dtexNext.y - dxtex.y][dtexNext.x - dxtex.x];

  min.r = static_cast<unsigned char>(topRight.r + (static_cast<unsigned short>(fracX * (topLeft.r - topRight.r)) >> 8));
  min.g = static_cast<unsigned char>(topRight.g + (static_cast<unsigned short>(fracX * (topLeft.g - topRight.g)) >> 8));
  min.b = static_cast<unsigned char>(topRight.b + (static_cast<unsigned short>(fracX * (topLeft.b - topRight.b)) >> 8));

  unsigned char bottomR = static_cast<unsigned char>(bottomRight.r + (static_cast<unsigned short>(fracX * (bottomLeft.r - bottomRight.r)) >> 8));
  unsigned char bottomG = static_cast<unsigned char>(bottomRight.g + (static_cast<unsigned short>(fracX * (bottomLeft.g - bottomRight.g)) >> 8));
  unsigned char bottomB = static_cast<unsigned char>(bottomRight.b + (static_cast<unsigned short>(fracX * (bottomLeft.b - bottomRight.b)) >> 8));

  min.r = static_cast<unsigned char>(bottomR + (static_cast<unsigned short>(fracY * (min.r - bottomR)) >> 8));
  min.g = static_cast<unsigned char>(bottomG + (static_cast<unsigned short>(fracY * (min.g - bottomG)) >> 8));
  min.b = static_cast<unsigned char>(bottomB + (static_cast<unsigned short>(fracY * (min.b - bottomB)) >> 8));

  color = min;
  if (color.r <= 24) {
    color.r = 24;
  }
  if (color.g <= 24) {
    color.g = 24;
  }
  if (color.b <= 24) {
    color.b = 24;
  }

  return 1;
}

bool CMapObjGroup::QueryLightmap(const NTempest::C3Segment &seg, NTempest::CImVector &color) {
  if (flags & 0x48) {
    return true;
  }

  CWTriData triData;
  float     thisT = 1.0f;
  if (GetTris(triData, seg, thisT, 0, 8)) {
    FATALASSERT(triData.GetNumBatches());
    return QueryLightmap(
        NTempest::C3Vector(
            seg.start.x + (seg.end.x - seg.start.x) * thisT, seg.start.y + (seg.end.y - seg.start.y) * thisT,
            seg.start.z + (seg.end.z - seg.start.z) * thisT
        ),
        triData.GetBatch(0).triIndices[0], color
    );
  }
  return false;
}

unsigned char __fastcall QueryCull(const NTempest::CAaBox& aaBox, const NTempest::C3Vector* verts) {
  for (unsigned int component = 0; component < 3; ++component) {
    unsigned int signMax = 0xFFFFFFFF;
    unsigned int signMin = 0xFFFFFFFF;

    for (unsigned int vertex = 0; vertex < 3; ++vertex) {
      float dmax = aaBox.t[component] - verts[vertex][component];
      signMax &= *reinterpret_cast<unsigned int *>(&dmax) & 0x80000000;

      float dmin = verts[vertex][component] - aaBox.b[component];
      signMin &= *reinterpret_cast<unsigned int *>(&dmin) & 0x80000000;
    }

    if (signMax || signMin) {
      return 1;
    }
  }

  return 0;
}

unsigned char __fastcall QueryCull(const CWFrustum& frustum, const NTempest::C3Vector* verts) {
  unsigned int cc[3];
  const_cast<CWFrustum &>(frustum).Cull(const_cast<NTempest::C3Vector &>(verts[0]), cc[0]);
  const_cast<CWFrustum &>(frustum).Cull(const_cast<NTempest::C3Vector &>(verts[1]), cc[1]);
  const_cast<CWFrustum &>(frustum).Cull(const_cast<NTempest::C3Vector &>(verts[2]), cc[2]);
  return (cc[0] & cc[1] & cc[2]) != 0;
}
