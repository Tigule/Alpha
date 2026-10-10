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
#include "Ftol.h"

#include "WorldCommon/WorldMath.h"
#include "Services/Texture.h"
#include "Tempest/c4vector.h"

#include <float.h>
#include <math.h>
#include <string.h>

namespace NTempest {
inline CRect operator+(const CRect &l, float a) {
  return CRect(l.t + a, l.l + a, l.b + a, l.r + a);
}

inline CRect operator-(const CRect &l, float a) {
  return CRect(l.t - a, l.l - a, l.b - a, l.r - a);
}

inline CRect operator*(const CRect &l, float a) {
  return CRect(l.t * a, l.l * a, l.b * a, l.r * a);
}
}

UINT CMapObj::DEFAULT_RLEVEL = 10;
UINT CMapObj::maxRLevel = CMapObj::DEFAULT_RLEVEL;
int  CMapObj::bIntRender;
void (*CMapObj::gRenderCallback)(const UINT, LPCVOID, const int);
LPVOID CMapObj::gRenderUserParam;

static WORD s_indexList[65535];
typedef void (CMapObj::*MapObjRenderFunc)(const CMapObjGroup *, UINT);
static MapObjRenderFunc    s_intFunc;
static MapObjRenderFunc    s_extFunc;
static NTempest::C44Matrix s_mvp;
static NTempest::C44Matrix s_mw;
static NTempest::C44Matrix s_cm;

TSCArray<NTempest::CRect, 16> CMapObj::extViewList;
TSCArray<SPortalExt, 2048>    CMapObj::portalExtList;

void CMapObj::SetGroupRenderCallback(void (*func)(const UINT, LPCVOID, const int), LPVOID userParam) {
  gRenderCallback = func;
  gRenderUserParam = userParam;
}

void CMapObj::PrepareUpdate() {
  CMapObj *mapObj;
  CMapObj *mapObjnext_node;

  extViewList.SetCount(0);
  s_extFunc = &CMapObj::RenderGroupLightTex;
  if (CWorld::enables & CWorld::Enable_MapObjTex) {
    s_intFunc = &CMapObj::RenderGroupTex;
    if (CWorld::enables & CWorld::Enable_MapObjLight) {
      if (!(CWorld::enables & CWorld::Enable_VertexLight)) {
        s_intFunc = &CMapObj::RenderGroupLightmapTex;
      } else {
        s_intFunc = &CMapObj::RenderGroupColorTex;
      }
    }
  } else {
    s_extFunc = &CMapObj::RenderGroup_Ext;
    s_intFunc = &CMapObj::RenderGroup_Int;
    if ((CWorld::enables & CWorld::Enable_MapObjLight) && !(CWorld::enables & CWorld::Enable_VertexLight)) {
      s_intFunc = &CMapObj::RenderGroupLightmap;
    }
  }

  if (CWorld::enables & CWorld::Enable_MapObjBSP) {
    s_extFunc = &CMapObj::RenderGroupBsp;
    s_intFunc = &CMapObj::RenderGroupBsp;
  }

  for (mapObj = mapObjHash.Head(); (int)mapObj > 0 ? (mapObjnext_node = mapObjHash.Next(mapObj), 1) : 0;
       mapObj = mapObjnext_node)
  {
    mapObj->UpdateMaterials();

    ITERATELIST(CMapObjGroup, mapObj->groupList, group) {
      group->lightmapTexFlushTime -= CWorld::GetTickTimeSec();
      group->flushTime -= CWorld::GetTickTimeSec();
      if (group->lightmapTexFlushTime <= 0.0f) {
        group->FreeLightmaps();
      }
      if (group->flushTime <= 0.0f && group->data) {
        group->Clear();
        group->lameAssLink.Unlink();
      }
    }

    if (!mapObj->refCount) {
      mapObj->flushTime -= CWorld::GetTickTimeSec();
      if (mapObj->flushTime <= 0.0f) {
        mapObjHash.Unlink(mapObj);
        CMap::FreeMapObj(mapObj);
      }
    }
  }
}

void CMapObj::LocateViewer(NTempest::C44Matrix &im, TSGrowableArray<UINT> &inGroups) {
  FATALASSERT(0);
}

UINT CMapObj::StabPortals(UINT groupIndex, const NTempest::C3Vector &start, const NTempest::C3Vector &end) {
  FATALASSERT(GetGroup(groupIndex));

  UINT               fromGroup = groupIndex;
  NTempest::C3Vector rayOrig = start;
  NTempest::C3Vector rayDir = end - start;

  while (1) {
    UINT nextGroup = StabPortals(fromGroup, groupIndex, rayOrig, rayDir);
    if (nextGroup == groupIndex) {
      if (groupInfoList[nextGroup].flags & 0x8) {
        return -1;
      }
      return nextGroup;
    }

    fromGroup = groupIndex;
    groupIndex = nextGroup;
  }
}

UINT CMapObj::StabPortals(UINT fromGroupIndex, UINT groupIndex, NTempest::C3Vector &rayOrig, NTempest::C3Vector &rayDir) {
  CMapObjGroup *group = GetGroup(groupIndex, 0);
  FATALASSERT(group);

  if (!portalRefCount) {
    return groupIndex;
  }

  SMOPortalRef *portalRef = &portalRefList[group->portalStart];
  for (UINT i = 0; i < group->portalCount; ++i, ++portalRef) {
    if (portalRef->groupIndex == fromGroupIndex || portalRef->groupIndex == 0xFFFF) {
      continue;
    }

    float         epsilon = 1.0194445f;
    CMapObjGroup *toGroup = GetGroup(portalRef->groupIndex, 0);
    FATALASSERT(toGroup);
    if (toGroup->flags & 0x8) {
      epsilon = 0.98055553f;
    }

    const SMOPortal *portal = &portalList[portalRef->portalIndex];
    for (WORD j = 1; j < portal->count - 1; ++j) {
      float dist;
      if (CWorldMath::RayIntersectTri(
              rayOrig, rayDir, portalVertexList[portal->startVertex], portalVertexList[portal->startVertex + j],
              portalVertexList[portal->startVertex + j + 1], dist
          ) &&
          !(dist < 0.0f) && !(dist > epsilon))
      {
        return portalRef->groupIndex;
      }
    }
  }

  return groupIndex;
}

void CMapObj::IntRender(NTempest::C44Matrix &mat, TSGrowableArray<UINT> &inGroups) {
  FATALASSERT(inGroups.Count());

  ++gRenderCount;
  bIntRender = 1;
  extViewList.SetCount(0);
  GxXformViewProj(s_mvp);
  GxXform(GxXform_World, s_mw);
  s_cm = s_mw * s_mvp;

  UINT i;
  CWorldScene::FrustumPush();
  CMapObjGroup::rDrawSharedLiquidFirst = 0;
  NTempest::CRect sRect(-1.0f, -1.0f, 1.0f, 1.0f);
  for (i = 0; i < inGroups.Count(); ++i) {
    RRenderThruPortals(inGroups[i], 0xFFFF, sRect, 0);
  }
  CWorldScene::FrustumPop();
  bIntRender = 0;

  NTempest::C33Matrix m = mat;
  NTempest::C3Vector  v = *mat.Row3AsVec3();

  for (i = 0; i < extViewList.Count(); ++i) {
    CWorldScene::FrustumPush();
    CWorldScene::FrustumSet(CWorldScene::camFrustumCorners, extViewList[i]);

    NTempest::CRect sRect = extViewList[i] * 2.0f - 1.0f;

    for (UINT groupIndex = 0; groupIndex < groupCount; ++groupIndex) {
      SMOGroupInfo *info = &groupInfoList[groupIndex];
      if (!(info->flags & 0x10000) && (info->flags & 8) && !CWorldScene::FrustumCull(info->aaBox, m, v)) {
        RRenderThruPortals(groupIndex, 0xFFFF, sRect, 0);
      }
    }

    CWorldScene::FrustumPop();
  }

  for (i = 0; i < groupCount; ++i) {
    SMOGroupInfo *info = &groupInfoList[i];
    FATALASSERT(info);
    if ((info->flags & 0x10000) && !CWorldScene::FrustumCull(info->aaBox, m, v)) {
      RenderAlways(i);
    }
  }
}

void CMapObj::ExtRender(NTempest::C44Matrix &mat, const NTempest::CRect &rect) {
  ++gRenderCount;
  bIntRender = 0;
  extViewList.SetCount(0);
  GxXformViewProj(s_mvp);
  GxXform(GxXform_World, s_mw);
  s_cm = s_mw * s_mvp;

  NTempest::C33Matrix m = mat;
  NTempest::C3Vector  v = *mat.Row3AsVec3();
  NTempest::CRect     sRect = rect * 2.0f - 1.0f;

  CWorldScene::FrustumPush();
  CWorldScene::FrustumSet(CWorldScene::camFrustumCorners, rect);
  for (UINT i = 0; i < groupCount; ++i) {
    SMOGroupInfo *info = &groupInfoList[i];
    FATALASSERT(info);

    if (info->flags & 0x10000) {
      if (!CWorldScene::FrustumCull(info->aaBox, m, v)) {
        RenderAlways(i);
      }
    } else if ((info->flags & 8) && !CWorldScene::FrustumCull(info->aaBox, m, v)) {
      RRenderThruPortals(i, 0xFFFF, sRect, 0);
    }
  }
  CWorldScene::FrustumPop();
}

void CMapObj::RenderGroup(
    UINT                       groupNum,
    int                        rDrawSharedLiquidToggle,
    const NTempest::C44Matrix &invMat,
    const LISTEX(CWFrustum, sceneLink) & frustumList
) {
  CMapObjGroup *group = GetGroup(groupNum, 0);
  FATALASSERT(group);
  group->CreateLightmaps();

  const CWFrustum *frustum = frustumList.RawNext(0);
  UINT             i = 0;
  while ((long)frustum > 0) {
    CWorldScene::FrustumSet(*frustum);
    CWorldScene::FrustumXform(invMat);
    if (!(group->flags & 0x48)) {
      (this->*s_intFunc)(group, i);
    } else {
      (this->*s_extFunc)(group, i);
    }
    ++i;
    frustum = frustumList.RawNext(frustum);
  }

  if (group->flags & 0x1000) {
    CMapObjGroup::rDrawSharedLiquidToggle = rDrawSharedLiquidToggle;
    RenderLiquid_0(group);
  }
  if (CWorld::enables & CWorld::Enable_ShowNormals) {
    RenderGroupNormals(group);
  }
  if (CWorld::enables & CWorld::Enable_Portals) {
    RenderPortals(group);
  }
}

void CMapObj::UpdateMaterials() {
  DNInfo *dnInfo = DayNightGetInfo();

  for (UINT lp = 0; lp < materialCount; ++lp) {
    if (materialList[lp].flags & 0x10) {
      materialList[lp].frameSidnColor = materialList[lp].sidnColor;
      materialList[lp].frameSidnColor.Scale(Fast_ftol(dnInfo->sidn * 255.0f));
    }
  }
}

void CMapObj::RenderAlways(UINT groupIdx) {
  CMapObjGroup *group = GetGroup(groupIdx, 0);
  if (group) {
    if ((group->flags & 0x1000) && !CMapObjGroup::rDrawSharedLiquidFirst) {
      CMapObjGroup::rDrawSharedLiquidFirst = 1;
      CMapObjGroup::rDrawSharedLiquidToggle = 0;
    }

    if (gRenderCallback) {
      gRenderCallback(groupIdx, gRenderUserParam, CMapObjGroup::rDrawSharedLiquidToggle == 0);
    }
  }
}

void CMapObj::RRenderThruPortals(UINT groupIdx, UINT parentIdx, NTempest::CRect &viewRect, UINT level) {
  NTempest::CRect newRect;
  UINT            toGroupIdx;
  UINT            i;
  CMapObjGroup   *group;
  CMapObjGroup   *toGroup;
  int             cpIgnore;
  SMOPortalRef   *portalRef;

  if (level > maxRLevel) {
    return;
  }

  group = GetGroup(groupIdx, 0);
  if (!group || (group->flags & 0x10000)) {
    return;
  }

  if ((group->flags & 0x1000) && !CMapObjGroup::rDrawSharedLiquidFirst) {
    CMapObjGroup::rDrawSharedLiquidFirst = 1;
    CMapObjGroup::rDrawSharedLiquidToggle = level & 1;
  }

  if (gRenderCallback) {
    gRenderCallback(groupIdx, gRenderUserParam, (level & 1) == CMapObjGroup::rDrawSharedLiquidToggle);
  }

  cpIgnore = 0;
  if (level == 0) {
    cpIgnore = 1;
  }

  if (!group->portalCount) {
    return;
  }

  portalRef = &portalRefList[group->portalStart];
  for (i = 0; i < group->portalCount; ++i, ++portalRef) {
    if (portalRef->groupIndex == 0xFFFF) {
      continue;
    }

    SMOPortal *portal = &portalList[portalRef->portalIndex];
    toGroupIdx = portalRef->groupIndex;
    if (toGroupIdx == parentIdx) {
      continue;
    }

    SPortalExt *portalExt = &portalExtList[portalRef->portalIndex];
    if (portalExt->xformTag != gRenderCount) {
      RTransformPortal(portal, portalExt, cpIgnore);
      portalExt->xformTag = gRenderCount;
    }

    if (portalExt->flags & 1) {
      continue;
    }

    if (!(portalExt->flags & 2)) {
      float dist = NTempest::C3Vector::Dot(localCamPos, portal->plane.n) + portal->plane.d;
      if (portalRef->side < 0) {
        dist = -dist;
      }
      if (dist < 0.0f) {
        continue;
      }
    }

    if (portalExt->sRect.l > viewRect.r || portalExt->sRect.r < viewRect.l || portalExt->sRect.t > viewRect.b || portalExt->sRect.b < viewRect.t) {
      continue;
    }

    newRect = portalExt->sRect;
    if (newRect.l < viewRect.l) {
      newRect.l = viewRect.l;
    }
    if (newRect.r > viewRect.r) {
      newRect.r = viewRect.r;
    }
    if (newRect.t < viewRect.t) {
      newRect.t = viewRect.t;
    }
    if (newRect.b > viewRect.b) {
      newRect.b = viewRect.b;
    }
    if (fabsf(newRect.r - newRect.l) < 0.001f || fabsf(newRect.b - newRect.t) < 0.001f) {
      continue;
    }

    toGroup = GetGroup(toGroupIdx, 0);
    if (toGroup && (toGroup->flags & 8) && portalExt->visitedTag != gRenderCount && bIntRender) {
      extViewList.SetCount(extViewList.Count() + 1);
      NTempest::CRect &extView = extViewList[extViewList.Count() - 1];
      extView = (newRect + 1.0f) * 0.5f;
      portalExt->visitedTag = gRenderCount;
    }

    NTempest::CRect sRect = (newRect + 1.0f) * 0.5f;
    CWorldScene::FrustumPush();
    CWorldScene::FrustumSet(CWorldScene::camFrustumCorners, sRect);
    RRenderThruPortals(toGroupIdx, groupIdx, newRect, level + 1);
    CWorldScene::FrustumPop();
  }
}

void CMapObj::RTransformPortal(SMOPortal *portal, SPortalExt *portalExt, int cpIgnore) {
  UINT i;

  FATALASSERT(portal);
  FATALASSERT(portalExt);
  (void)cpIgnore;

  static NTempest::C4Vector tv[16];
  static UINT               cnt;

  portalExt->flags = 0;
  NTempest::C3Vector *vertex = &portalVertexList[portal->startVertex];
  for (i = 0; i < portal->count; ++i, ++vertex) {
    tv[i] = *vertex;
    tv[i] = tv[i] * s_cm;
    if (tv[i].w < CWorld::nearClip) {
      portalExt->flags |= 2;
    }
  }

  cnt = portal->count;
  CWorldScene::ClipPortal(tv, cnt);
  if (!cnt) {
    portalExt->flags |= 1;
  } else if (portalExt->flags & 2) {
    portalExt->sRect = NTempest::CRect(-1.0f, -1.0f, 1.0f, 1.0f);
  } else {
    portalExt->sRect.t = FLT_MAX;
    portalExt->sRect.b = -FLT_MAX;
    portalExt->sRect.l = FLT_MAX;
    portalExt->sRect.r = -FLT_MAX;
    for (i = 0; i < cnt; ++i) {
      if (fabsf(tv[i].w) < 0.001f) {
        tv[i].w = 0.00001f;
      }
      tv[i].x *= 1.0f / tv[i].w;
      tv[i].y *= 1.0f / tv[i].w;
      if (tv[i].x < portalExt->sRect.l) {
        portalExt->sRect.l = tv[i].x;
      }
      if (tv[i].x > portalExt->sRect.r) {
        portalExt->sRect.r = tv[i].x;
      }
      if (tv[i].y < portalExt->sRect.t) {
        portalExt->sRect.t = tv[i].y;
      }
      if (tv[i].y > portalExt->sRect.b) {
        portalExt->sRect.b = tv[i].y;
      }
    }
  }
}

bool CMapObj::CullBatch(const SMOBatch *batch) {
  NTempest::CAaBox localBox(
      NTempest::C3Vector((float)batch->bx, (float)batch->by, (float)batch->bz),
      NTempest::C3Vector((float)batch->tx, (float)batch->ty, (float)batch->tz)
  );
  return CWorldScene::FrustumCull(localBox);
}

void CMapObjGroup::ExtGxBufFillVertex(CGxBufCommand &cmd, CGxBuf *buf) {
  CGxVertexPNT0      *vtx;
  UINT                i;
  NTempest::C3Vector *p;
  NTempest::C3Vector *n;
  NTempest::C2Vector *t0;

  switch (cmd.vertex.op) {
    case GxBufOp_Nop:
      return;
    case GxBufOp_Fill:
      vtx = (CGxVertexPNT0 *)*cmd.vertex.mem[GxVM_Position];
      p = vertexList + sLockGxBatch->vertStart;
      n = normalList + sLockGxBatch->vertStart;
      t0 = textureVertexList + sLockGxBatch->vertStart;
      for (i = 0; i < sLockGxBatch->vertCount; ++i, ++vtx) {
        vtx->p = *p++;
        vtx->n = *n++;
        vtx->tc[0] = *t0++;
      }
      return;
    case GxBufOp_Assign:
      *cmd.vertex.mem[GxVM_Position] = vertexList + sLockGxBatch->vertStart;
      *cmd.vertex.mem[GxVM_Normal] = normalList + sLockGxBatch->vertStart;
      *cmd.vertex.mem[GxVM_Texture0] = textureVertexList + sLockGxBatch->vertStart;
      cmd.vertex.stride[GxVM_Position] = sizeof(NTempest::C3Vector);
      cmd.vertex.stride[GxVM_Normal] = sizeof(NTempest::C3Vector);
      cmd.vertex.stride[GxVM_Texture0] = sizeof(NTempest::C2Vector);
      return;
  }
}

void CMapObjGroup::IntGxBufFillVertex(CGxBufCommand &cmd, CGxBuf *buf) {
  CGxVertexPNT0T1    *vtx;
  UINT                i;
  NTempest::C3Vector *n;
  NTempest::C3Vector *p;
  NTempest::C2Vector *t0;
  NTempest::C2Vector *t1;

  switch (cmd.vertex.op) {
    case GxBufOp_Nop:
      return;
    case GxBufOp_Fill:
      vtx = (CGxVertexPNT0T1 *)*cmd.vertex.mem[GxVM_Position];
      p = vertexList + sLockGxBatch->vertStart;
      n = normalList + sLockGxBatch->vertStart;
      t0 = textureVertexList + sLockGxBatch->vertStart;
      t1 = lightmapVertexList + sLockGxBatch->vertStart;
      for (i = 0; i < sLockGxBatch->vertCount; ++i, ++vtx) {
        vtx->p = *p++;
        vtx->n = *n++;
        vtx->tc[0] = *t0++;
        vtx->tc[1] = *t1++;
      }
      return;
    case GxBufOp_Assign:
      *cmd.vertex.mem[GxVM_Position] = vertexList + sLockGxBatch->vertStart;
      *cmd.vertex.mem[GxVM_Normal] = normalList + sLockGxBatch->vertStart;
      *cmd.vertex.mem[GxVM_Texture0] = textureVertexList + sLockGxBatch->vertStart;
      *cmd.vertex.mem[GxVM_Texture1] = lightmapVertexList + sLockGxBatch->vertStart;
      cmd.vertex.stride[GxVM_Position] = sizeof(NTempest::C3Vector);
      cmd.vertex.stride[GxVM_Normal] = sizeof(NTempest::C3Vector);
      cmd.vertex.stride[GxVM_Texture0] = sizeof(NTempest::C2Vector);
      cmd.vertex.stride[GxVM_Texture1] = sizeof(NTempest::C2Vector);
      return;
  }
}

void CMapObjGroup::GxBufFillIndex(CGxBufCommand &cmd, CGxBuf *buf) {
  switch (cmd.index.op) {
    case GxBufOp_Nop:
      return;
    case GxBufOp_Fill: {
      WORD *dst = (WORD *)*cmd.index.mem[GxVM_Indices];
      WORD *src = indexList + sLockGxBatch->vertStart;
      for (WORD i = 0; i < sLockGxBatch->vertCount; ++i) {
        dst[i] = src[i];
      }
      return;
    }
    case GxBufOp_Assign:
      *cmd.index.mem[GxVM_Indices] = indexList + sLockGxBatch->vertStart;
      return;
  }
}

void CMapObj::RenderGroupLightTex(const CMapObjGroup *group, UINT frustumCount) {
  UINT i;

  FATALASSERT(group);
  GxRsPush();
  GxVertexShaderSelect(GxVS_PassThru);

  for (i = 0; i < 4; ++i) {
    const SMOGxBatch &gxBatch = group->extBatch[i];
    if (!gxBatch.batchCount) {
      continue;
    }

    FATALASSERT(group->sLockGxBatch == 0);
    CMapObjGroup::sLockGxBatch = &gxBatch;
    GxBufLock(group->extGxBuf[i]);

    SMOBatch *batch = group->batchList + gxBatch.batchStart;
    for (UINT i = 0; i < gxBatch.batchCount; ++i, ++batch) {
      if (!frustumCount) {
        batch->flags &= 0x0F;
      }
      if (!(batch->flags & 0xF0) && !CullBatch(batch)) {
        batch->flags |= 0xF0;
        SMOMaterial &material = materialList[batch->texture];
        GxRsSet(GxRs_Lighting, !(material.flags & 0x1));
        GxRsSet(GxRs_Fog, !(material.flags & 0x2));
        GxRsSet(GxRs_Culling, !(material.flags & 0x4));
        GxRsSet(GxRs_MatEmissive, material.flags & 0x10 ? material.frameSidnColor : NTempest::CImVector(0ul));
        GxRsSet(GxRs_Blend, (int)material.blendMode);

        CGxTex *texture = TextureGetGxTex(material.hMaps[0], 1, 0);
        CGxTexFlags diffTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
        GxTexFlags(texture, diffTexFlags);
        diffTexFlags.m_wrapU = !(material.flags & 0x40);
        diffTexFlags.m_wrapV = !(material.flags & 0x80);
        GxTexSetFlags(texture, diffTexFlags);
        GxRsSet(GxRs_Texture0, texture);

        GxBufRender(CGxBatch(GxPrim_Triangles, batch->count, group->indexList[batch->startIndex], batch->minIndex, batch->maxIndex));
      }
    }

    GxBufUnlock();
    CMapObjGroup::sLockGxBatch = 0;
  }

  GxRsPop();
}

void CMapObj::RenderGroupLightmapTex_Int(const CMapObjGroup *group, UINT frustumCount) {
  DNInfo     *dnInfo = DayNightGetInfo();
  UINT        i;

  if (dnInfo->intFog && this == CWorldScene::camMapObj) {
    GxRsPush();
    GxRsSet(GxRs_FogStart, dnInfo->intFogInfo.start);
    GxRsSet(GxRs_FogEnd, dnInfo->intFogInfo.end);
    GxRsSet(GxRs_FogColor, dnInfo->intFogInfo.color);
  }

  for (i = 0; i < 4; ++i) {
    const SMOGxBatch &gxBatch = group->intBatch[i];
    if (!gxBatch.batchCount) {
      continue;
    }

    GxPrimLockVertexPtrs(
        gxBatch.vertCount, group->vertexList + gxBatch.vertStart, sizeof(NTempest::C3Vector), 0, 0, 0, 0, 0, 0,
        group->textureVertexList + gxBatch.vertStart, sizeof(NTempest::C2Vector), group->lightmapVertexList + gxBatch.vertStart,
        sizeof(NTempest::C2Vector)
    );

    SMOBatch *batch = group->batchList + gxBatch.batchStart;
    for (UINT i = 0; i < gxBatch.batchCount; ++i, ++batch) {
      if (!frustumCount) {
        batch->flags &= 0x0F;
      }
      if (!(batch->flags & 0xF0) && !CullBatch(batch)) {
        batch->flags |= 0xF0;
        SMOMaterial &material = materialList[batch->texture];
        GxRsSet(GxRs_Fog, !(material.flags & 0x2));
        GxRsSet(GxRs_Culling, !(material.flags & 0x4));
        GxRsSet(GxRs_Blend, (int)material.blendMode);

        CGxTex *texture = TextureGetGxTex(material.hMaps[0], 1, 0);
        CGxTexFlags diffTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
        GxTexFlags(texture, diffTexFlags);
        diffTexFlags.m_wrapU = !(material.flags & 0x40);
        diffTexFlags.m_wrapV = !(material.flags & 0x80);
        GxTexSetFlags(texture, diffTexFlags);
        GxRsSet(GxRs_Texture0, texture);

        texture = TextureGetGxTex(group->lightmapTexList[batch->lightMap].hTexture, 1, 0);
        GxRsSet(GxRs_Texture1, texture);
        GxPrimDrawElements(GxPrim_Triangles, batch->count, group->indexList + batch->startIndex);
      }
    }

    GxPrimUnlockVertexPtrs();
  }

  if (dnInfo->intFog && this == CWorldScene::camMapObj) {
    GxRsPop();
  }
}

void CMapObj::RenderGroupLightmapTex_Ext(const CMapObjGroup *group, UINT frustumCount) {
  UINT        i;
  DNInfo     *dnInfo = DayNightGetInfo();

  for (i = 0; i < 4; ++i) {
    const SMOGxBatch &gxBatch = group->extBatch[i];
    if (!gxBatch.batchCount) {
      continue;
    }

    GxPrimLockVertexPtrs(
        gxBatch.vertCount, group->vertexList + gxBatch.vertStart, sizeof(NTempest::C3Vector), group->normalList + gxBatch.vertStart,
        sizeof(NTempest::C3Vector), 0, 0, 0, 0, group->textureVertexList + gxBatch.vertStart, sizeof(NTempest::C2Vector), 0, 0
    );

    SMOBatch *batch = group->batchList + gxBatch.batchStart;
    for (UINT i = 0; i < gxBatch.batchCount; ++i, ++batch) {
      if (!frustumCount) {
        batch->flags &= 0x0F;
      }
      if (!(batch->flags & 0xF0) && !CullBatch(batch)) {
        batch->flags |= 0xF0;
        SMOMaterial &material = materialList[batch->texture];
        GxRsSet(GxRs_Fog, !(material.flags & 0x2));
        GxRsSet(GxRs_Culling, !(material.flags & 0x4));
        GxRsSet(GxRs_Blend, (int)material.blendMode);

        if (material.flags & 0x20) {
          CGxLight &gxLight = CMap::sunLight->gxLight;
          gxLight.m_dirColor = dnInfo->lightInfo.windowDirColor;
          gxLight.m_ambColor = dnInfo->lightInfo.windowAmbColor;
          GxLightSet(0, gxLight, NTempest::C3Vector(0.0f, 0.0f, 0.0f));
        }

        CGxTex *texture = TextureGetGxTex(material.hMaps[0], 1, 0);
        CGxTexFlags diffTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
        GxTexFlags(texture, diffTexFlags);
        diffTexFlags.m_wrapU = !(material.flags & 0x40);
        diffTexFlags.m_wrapV = !(material.flags & 0x80);
        GxTexSetFlags(texture, diffTexFlags);
        GxRsSet(GxRs_Texture0, texture);
        GxPrimDrawElements(GxPrim_Triangles, batch->count, group->indexList + batch->startIndex);

        if (materialList[batch->texture].flags & 0x20) {
          CGxLight &gxLight = CMap::sunLight->gxLight;
          gxLight.m_dirColor = dnInfo->lightInfo.dirColor;
          gxLight.m_ambColor = dnInfo->lightInfo.ambColor;
          GxLightSet(0, gxLight, NTempest::C3Vector(0.0f, 0.0f, 0.0f));
        }
      }
    }

    GxPrimUnlockVertexPtrs();
  }
}

void CMapObj::RenderGroupLightmapTex(const CMapObjGroup *group, UINT frustumCount) {
  FATALASSERT(group);
  GxRsPush();
  GxVertexShaderSelect(GxVS_PassThru);
  GxRsSet(GxRs_Lighting, 0);
  RenderGroupLightmapTex_Int(group, frustumCount);
  GxRsSet(GxRs_Lighting, 1);
  GxRsSet(GxRs_Texture1, 0);
  RenderGroupLightmapTex_Ext(group, frustumCount);
  GxRsPop();
}

void CMapObj::RenderGroupColorTex_Int(const CMapObjGroup *group, UINT frustumCount) {
  DNInfo     *dnInfo = DayNightGetInfo();
  UINT        i;

  if (dnInfo->intFog && this == CWorldScene::camMapObj) {
    GxRsPush();
    GxRsSet(GxRs_FogStart, dnInfo->intFogInfo.start);
    GxRsSet(GxRs_FogEnd, dnInfo->intFogInfo.end);
    GxRsSet(GxRs_FogColor, dnInfo->intFogInfo.color);
  }

  for (i = 0; i < 4; ++i) {
    const SMOGxBatch &gxBatch = group->intBatch[i];
    if (!gxBatch.batchCount) {
      continue;
    }

    GxPrimLockVertexPtrs(
        gxBatch.vertCount, group->vertexList + gxBatch.vertStart, sizeof(NTempest::C3Vector), 0, 0, group->colorVertexList + gxBatch.vertStart,
        sizeof(NTempest::CImVector), 0, 0, group->textureVertexList + gxBatch.vertStart, sizeof(NTempest::C2Vector), 0, 0
    );

    SMOBatch *batch = group->batchList + gxBatch.batchStart;
    for (UINT i = 0; i < gxBatch.batchCount; ++i, ++batch) {
      if (!frustumCount) {
        batch->flags &= 0x0F;
      }
      if (!(batch->flags & 0xF0) && !CullBatch(batch)) {
        batch->flags |= 0xF0;
        SMOMaterial &material = materialList[batch->texture];
        GxRsSet(GxRs_Fog, !(material.flags & 0x2));
        GxRsSet(GxRs_Culling, !(material.flags & 0x4));
        GxRsSet(GxRs_Blend, (int)material.blendMode);

        CGxTex *texture = TextureGetGxTex(material.hMaps[0], 1, 0);
        CGxTexFlags diffTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
        GxTexFlags(texture, diffTexFlags);
        diffTexFlags.m_wrapU = !(material.flags & 0x40);
        diffTexFlags.m_wrapV = !(material.flags & 0x80);
        GxTexSetFlags(texture, diffTexFlags);
        GxRsSet(GxRs_Texture0, texture);
        GxPrimDrawElements(GxPrim_Triangles, batch->count, group->indexList + batch->startIndex);
      }
    }

    GxPrimUnlockVertexPtrs();
  }

  if (dnInfo->intFog && this == CWorldScene::camMapObj) {
    GxRsPop();
  }
}

void CMapObj::RenderGroupColorTex_Ext(const CMapObjGroup *group, UINT frustumCount) {
  UINT        i;
  DNInfo     *dnInfo = DayNightGetInfo();

  for (i = 0; i < 4; ++i) {
    const SMOGxBatch &gxBatch = group->extBatch[i];
    if (!gxBatch.batchCount) {
      continue;
    }

    GxPrimLockVertexPtrs(
        gxBatch.vertCount, group->vertexList + gxBatch.vertStart, sizeof(NTempest::C3Vector), group->normalList + gxBatch.vertStart,
        sizeof(NTempest::C3Vector), group->colorVertexList + gxBatch.vertStart, sizeof(NTempest::CImVector), 0, 0,
        group->textureVertexList + gxBatch.vertStart, sizeof(NTempest::C2Vector), 0, 0
    );

    SMOBatch *batch = group->batchList + gxBatch.batchStart;
    for (UINT i = 0; i < gxBatch.batchCount; ++i, ++batch) {
      if (!frustumCount) {
        batch->flags &= 0x0F;
      }
      if (!(batch->flags & 0xF0) && !CullBatch(batch)) {
        batch->flags |= 0xF0;
        SMOMaterial &material = materialList[batch->texture];
        GxRsSet(GxRs_Fog, !(material.flags & 0x2));
        GxRsSet(GxRs_Culling, !(material.flags & 0x4));
        GxRsSet(GxRs_Blend, (int)material.blendMode);

        if (material.flags & 0x20) {
          CGxLight &gxLight = CMap::sunLight->gxLight;
          gxLight.m_dirColor = dnInfo->lightInfo.windowDirColor;
          gxLight.m_ambColor = dnInfo->lightInfo.windowAmbColor;
          GxLightSet(0, gxLight, NTempest::C3Vector(0.0f, 0.0f, 0.0f));
        }

        CGxTex *texture = TextureGetGxTex(material.hMaps[0], 1, 0);
        CGxTexFlags diffTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
        GxTexFlags(texture, diffTexFlags);
        diffTexFlags.m_wrapU = !(material.flags & 0x40);
        diffTexFlags.m_wrapV = !(material.flags & 0x80);
        GxTexSetFlags(texture, diffTexFlags);
        GxRsSet(GxRs_Texture0, texture);
        GxPrimDrawElements(GxPrim_Triangles, batch->count, group->indexList + batch->startIndex);

        if (materialList[batch->texture].flags & 0x20) {
          CGxLight &gxLight = CMap::sunLight->gxLight;
          gxLight.m_dirColor = dnInfo->lightInfo.dirColor;
          gxLight.m_ambColor = dnInfo->lightInfo.ambColor;
          GxLightSet(0, gxLight, NTempest::C3Vector(0.0f, 0.0f, 0.0f));
        }
      }
    }

    GxPrimUnlockVertexPtrs();
  }
}

void CMapObj::RenderGroupColorTex(const CMapObjGroup *group, UINT frustumCount) {
  FATALASSERT(group);
  GxRsPush();
  GxVertexShaderSelect(GxVS_PassThru);
  GxRsSet(GxRs_Lighting, 0);
  RenderGroupColorTex_Int(group, frustumCount);
  GxRsSet(GxRs_Lighting, 1);
  RenderGroupColorTex_Ext(group, frustumCount);
  GxRsPop();
}

void CMapObj::RenderGroupLightmap(const CMapObjGroup *group, UINT frustumCount) {
  NTempest::CImVector WHITE(0xFFFFFFFF);

  GxRsPush();
  GxRsSet(GxRs_Lighting, 0);

  for (UINT i = 0; i < 4; ++i) {
    const SMOGxBatch &gxBatch = group->intBatch[i];
    if (!gxBatch.batchCount) {
      continue;
    }

    GxPrimLockVertexPtrs(
        gxBatch.vertCount, group->vertexList + gxBatch.vertStart, sizeof(NTempest::C3Vector), 0, 0, &WHITE, 0, 0, 0,
        group->lightmapVertexList + gxBatch.vertStart, sizeof(NTempest::C2Vector), 0, 0
    );
    SMOBatch *batch = group->batchList + gxBatch.batchStart;
    for (UINT i = 0; i < gxBatch.batchCount; ++i, ++batch) {
      if (!frustumCount) {
        batch->flags &= 0x0F;
      }
      if (!(batch->flags & 0xF0) && !CullBatch(batch)) {
        batch->flags |= 0xF0;
        CGxTex *texture = TextureGetGxTex(group->lightmapTexList[batch->lightMap].hTexture, 1, 0);
        GxRsSet(GxRs_Texture0, texture);
        GxPrimDrawElements(GxPrim_Triangles, batch->count, group->indexList + batch->startIndex);
      }
    }
    GxPrimUnlockVertexPtrs();
  }

  GxRsPop();
}

void CMapObj::RenderGroupTex(const CMapObjGroup *group, UINT frustumCount) {
  NTempest::CImVector WHITE(0xFFFFFFFF);
  CGxTexFlags         diffTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1);

  GxRsPush();
  GxRsSet(GxRs_Lighting, 0);

  for (UINT i = 0; i < 4; ++i) {
    const SMOGxBatch &gxBatch = group->intBatch[i];
    if (!gxBatch.batchCount) {
      continue;
    }

    GxPrimLockVertexPtrs(
        gxBatch.vertCount, group->vertexList + gxBatch.vertStart, sizeof(NTempest::C3Vector), 0, 0, &WHITE, 0, 0, 0,
        group->textureVertexList + gxBatch.vertStart, sizeof(NTempest::C2Vector), 0, 0
    );
    SMOBatch *batch = group->batchList + gxBatch.batchStart;
    for (UINT i = 0; i < gxBatch.batchCount; ++i, ++batch) {
      if (!frustumCount) {
        batch->flags &= 0x0F;
      }
      if (!(batch->flags & 0xF0) && !CullBatch(batch)) {
        batch->flags |= 0xF0;
        SMOMaterial &material = materialList[batch->texture];
        CGxTex      *texture = TextureGetGxTex(material.hMaps[0], 1, 0);
        GxRsSet(GxRs_Texture0, texture);
        GxTexFlags(texture, diffTexFlags);
        diffTexFlags.m_wrapU = !(material.flags & 0x40);
        diffTexFlags.m_wrapV = !(material.flags & 0x80);
        GxTexSetFlags(texture, diffTexFlags);
        GxPrimDrawElements(GxPrim_Triangles, batch->count, group->indexList + batch->startIndex);
      }
    }
    GxPrimUnlockVertexPtrs();
  }

  GxRsPop();
}

void CMapObj::RenderGroup_Ext(const CMapObjGroup *group, UINT frustumCount) {
  NTempest::CImVector WHITE(0xFFFFFFFF);

  GxVertexShaderSelect(GxVS_PassThru);

  for (UINT i = 0; i < 4; ++i) {
    const SMOGxBatch &gxBatch = group->extBatch[i];
    if (!gxBatch.batchCount) {
      continue;
    }

    GxPrimLockVertexPtrs(gxBatch.vertCount, group->vertexList + gxBatch.vertStart, sizeof(NTempest::C3Vector), 0, 0, &WHITE, 0, 0, 0, 0, 0, 0, 0);
    SMOBatch *batch = group->batchList + gxBatch.batchStart;
    for (UINT i = 0; i < gxBatch.batchCount; ++i, ++batch) {
      if (!frustumCount) {
        batch->flags &= 0x0F;
      }
      if (!(batch->flags & 0xF0) && !CullBatch(batch)) {
        batch->flags |= 0xF0;
        GxPrimDrawElements(GxPrim_Triangles, batch->count, group->indexList + batch->startIndex);
      }
    }
    GxPrimUnlockVertexPtrs();
  }
}

void CMapObj::RenderGroup_Int(const CMapObjGroup *group, UINT frustumCount) {
  NTempest::CImVector WHITE(0xFFFFFFFF);

  GxRsPush();
  GxRsSet(GxRs_MatDiffuse, NTempest::CImVector(0x80A020A0));
  GxVertexShaderSelect(GxVS_PassThru);

  for (UINT i = 0; i < 4; ++i) {
    const SMOGxBatch &gxBatch = group->intBatch[i];
    if (!gxBatch.batchCount) {
      continue;
    }

    GxPrimLockVertexPtrs(gxBatch.vertCount, group->vertexList + gxBatch.vertStart, sizeof(NTempest::C3Vector), 0, 0, &WHITE, 0, 0, 0, 0, 0, 0, 0);
    SMOBatch *batch = group->batchList + gxBatch.batchStart;
    for (UINT i = 0; i < gxBatch.batchCount; ++i, ++batch) {
      if (!frustumCount) {
        batch->flags &= 0x0F;
      }
      if (!(batch->flags & 0xF0) && !CullBatch(batch)) {
        batch->flags |= 0xF0;
        GxPrimDrawElements(GxPrim_Triangles, batch->count, group->indexList + batch->startIndex);
      }
    }
    GxPrimUnlockVertexPtrs();
  }

  GxRsPop();
}

void CMapObj::RenderPortals(CMapObjGroup *group) {
  UINT i;
  UINT indexCount = 0;

  if (!group->portalCount) {
    return;
  }

  SMOPortalRef *portalRef = &portalRefList[group->portalStart];
  for (i = 0; i < group->portalCount; ++i, ++portalRef) {
    SMOPortal *portal = &portalList[portalRef->portalIndex];
    for (UINT j = 0; j < portal->count - 2; ++j) {
      s_indexList[indexCount++] = portal->startVertex + j;
      s_indexList[indexCount++] = portal->startVertex + j + 1;
      s_indexList[indexCount++] = portal->startVertex + portal->count - 1;
    }
  }

  GxRsPush();

  NTempest::CImVector argb(0x60FF00FF);
  GxRsSet(GxRs_Blend, 2);
  GxRsSet(GxRs_Lighting, 0);
  GxRsSet(GxRs_DepthTest, 0);
  GxRsSet(GxRs_DepthWrite, 0);
  GxRsSet(GxRs_Culling, 0);
  GxVertexShaderSelect(GxVS_PassThru);
  GxPrimLockVertexPtrs(portalVertexCount, portalVertexList, sizeof(NTempest::C3Vector), 0, 0, &argb, 0, 0, 0, 0, 0, 0, 0);
  GxPrimDrawElements(GxPrim_Triangles, indexCount, s_indexList);
  GxPrimUnlockVertexPtrs();
  GxRsPop();
}

void CMapObj::RenderPortals() {
  if (!portalCount) {
    return;
  }

  UINT       indexCount = 0;
  SMOPortal *portal = portalList;
  for (UINT i = 0; i < portalCount; ++i, ++portal) {
    for (UINT j = 0; j < portal->count - 2; ++j) {
      s_indexList[indexCount++] = portal->startVertex + j;
      s_indexList[indexCount++] = portal->startVertex + j + 1;
      s_indexList[indexCount++] = portal->startVertex + portal->count - 1;
    }
  }

  GxRsPush();
  GxRsSet(GxRs_MatDiffuse, NTempest::CImVector(0x60FF00FF));
  GxRsSet(GxRs_Blend, 2);
  GxRsSet(GxRs_Lighting, 0);
  GxRsSet(GxRs_DepthTest, 0);
  GxRsSet(GxRs_DepthWrite, 0);
  GxRsSet(GxRs_Culling, 0);
  GxVertexShaderSelect(GxVS_PassThru);
  GxPrimLockVertexPtrs(portalVertexCount, portalVertexList, sizeof(NTempest::C3Vector), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
  GxPrimDrawElements(GxPrim_Triangles, indexCount, s_indexList);
  GxPrimUnlockVertexPtrs();
  GxRsPop();
}

void CMapObj::RenderGroupBsp(const CMapObjGroup *group, UINT frustumCount) {
  if (frustumCount > 0) {
    return;
  }

  FATALASSERT(group);
  GxRsPush();
  GxRsSet(GxRs_Blend, 0);
  GxVertexShaderSelect(GxVS_PassThru);
  GxPrimLockVertexPtrs(
      group->vertexCount, group->vertexList, sizeof(NTempest::C3Vector), group->normalList, sizeof(NTempest::C3Vector), 0, 0, 0, 0, 0, 0, 0, 0
  );

  WORD *index = (WORD *)GxAllocIndexMem(49152);
  WORD *nextIndex;

  GxRsSet(GxRs_MatDiffuse, NTempest::CImVector(0xFF00FF00));
  nextIndex = index;
  {
    SMOPoly *poly = group->polyList;
    for (UINT i = 0; i < group->polyCount; ++i, ++poly) {
      if ((poly->flags & 0x20) && !(poly->flags & 0x4)) {
        *nextIndex++ = 3 * i;
        *nextIndex++ = 3 * i + 1;
        *nextIndex++ = 3 * i + 2;
        if ((UINT)(nextIndex - index) >= 0xBFFD) {
          GxPrimDrawElements(GxPrim_Triangles, nextIndex - index, index);
          nextIndex = index;
        }
      }
    }
  }
  if (nextIndex - index) {
    GxPrimDrawElements(GxPrim_Triangles, nextIndex - index, index);
  }

  GxRsSet(GxRs_MatDiffuse, NTempest::CImVector(0xFF8080FF));
  nextIndex = index;
  {
    SMOPoly *poly = group->polyList;
    for (UINT j = 0; j < group->polyCount; ++j, ++poly) {
      if (poly->flags & 0x4) {
        *nextIndex++ = 3 * j;
        *nextIndex++ = 3 * j + 1;
        *nextIndex++ = 3 * j + 2;
        if ((UINT)(nextIndex - index) >= 0xBFFD) {
          GxPrimDrawElements(GxPrim_Triangles, nextIndex - index, index);
          nextIndex = index;
        }
      }
    }
  }
  if (nextIndex - index) {
    GxPrimDrawElements(GxPrim_Triangles, nextIndex - index, index);
  }

  GxRsSet(GxRs_MatDiffuse, NTempest::CImVector(0x80FF0000));
  GxRsSet(GxRs_Blend, 2);
  nextIndex = index;
  {
    SMOPoly *poly = group->polyList;
    for (UINT k = 0; k < group->polyCount; ++k, ++poly) {
      if (poly->flags & 0x8) {
        *nextIndex++ = 3 * k;
        *nextIndex++ = 3 * k + 1;
        *nextIndex++ = 3 * k + 2;
        if ((UINT)(nextIndex - index) >= 0xBFFD) {
          GxPrimDrawElements(GxPrim_Triangles, nextIndex - index, index);
          nextIndex = index;
        }
      }
    }
  }
  if (nextIndex - index) {
    GxPrimDrawElements(GxPrim_Triangles, nextIndex - index, index);
  }

  GxPrimUnlockVertexPtrs();
  GxRsPop();
}

void CMapObj::RenderGroupNormals(const CMapObjGroup *group) {
  GxRsPush();
  GxRsSet(GxRs_Lighting, 0);

  UINT         maxBatchNormals = min(group->vertexCount, 0x2000);
  UINT         nVerts = maxBatchNormals * 2;
  CGxVertexPC *vertex = (CGxVertexPC *)GxAllocVertexMem(nVerts * sizeof(CGxVertexPC));
  WORD        *index = (WORD *)GxAllocIndexMem(nVerts * sizeof(WORD));
  UINT         base = 0;
  UINT         numNormals = group->vertexCount;
  while (numNormals) {
    UINT batchNormals = min(maxBatchNormals, numNormals);
    numNormals -= batchNormals;

    CGxVertexPC *vtx = vertex;
    WORD        *idx = index;
    for (UINT j = 0; j < batchNormals; ++j) {
      vtx->p = group->vertexList[base + j];
      vtx->c = 0xFFFFFF00;
      ++vtx;
      vtx->p = group->normalList[base + j] * 0.75f + group->vertexList[base + j];
      vtx->c = 0xFFFFFF00;
      ++vtx;
      *idx++ = 2 * j;
      *idx++ = 2 * j + 1;
    }

    base += batchNormals;
    GxPrimLockVertexPtrs(batchNormals * 2, &vertex[0].p, sizeof(CGxVertexPC), 0, 0, &vertex[0].c, sizeof(CGxVertexPC), 0, 0, 0, 0, 0, 0);
    GxPrimDrawElements(GxPrim_Lines, batchNormals * 2, index);
    GxPrimUnlockVertexPtrs();
  }
  GxRsPop();
}

void CMapObj::RenderWaterIndices_0(const CMapObjGroup *group, WORD *idxBase, UINT vtxSub, UINT &idxSub) {
  int             drawShared = CMapObjGroup::rDrawSharedLiquidToggle;
  int             ty;
  int             tx;
  WORD            lastRenderedVtx = 0;
  const SMOLTile *tile = group->liquidTileList;
  int             rendering = 0;
  WORD           *index = idxBase;

  for (ty = 0; ty < group->liquidTiles.y; ++ty) {
    WORD i1 = (WORD)(vtxSub + ty * group->liquidVerts.x);
    WORD i2 = (WORD)(i1 + group->liquidVerts.x);
    for (tx = 0; tx < group->liquidTiles.x; ++tx, ++tile) {
      int render = tile->IsLiquid();
      if (render && tile->GetShared()) {
        render = drawShared;
      }

      if (render) {
        if (!rendering) {
          rendering = 1;
          *index++ = i1;
          *index++ = i1;
          *index++ = i2;
        }
        *index++ = i1 + 1;
        *index++ = i2 + 1;
        lastRenderedVtx = i2 + 1;
      } else if (rendering) {
        *index++ = lastRenderedVtx;
        rendering = 0;
      }
      ++i1;
      ++i2;
    }

    if (rendering) {
      *index++ = lastRenderedVtx;
      rendering = 0;
    }
  }

  idxSub += index - idxBase;
}

void CMapObj::RenderLiquid_0(const CMapObjGroup *group) {
  UINT liquid = LIQUID_NONE;

  for (int i = 0; i < group->liquidTiles.x; ++i) {
    if (group->liquidTileList[i].IsLiquid()) {
      liquid = group->liquidTileList[i].GetLiquid();
      break;
    }
  }
  FATALASSERT(liquid != LIQUID_NONE);

  GxVertexShaderSelect(GxVS_PassThru);
  GxRsPush();
  GxRsSet(GxRs_Culling, 0);
  CGxTex *texture = TextureGetGxTex(CMap::GetLiquidTexture(liquid), 0, 0);
  if (texture) {
    GxRsSet(GxRs_Texture0, texture);
    switch (liquid) {
      case 2:
      case 3:
      case 6:
      case 7:
        RenderMagma(group, liquid);
        break;
      case 0:
      case 4:
      case 8:
        GxRsSet(GxRs_TexBlend0, 3);
        if (!(group->flags & 0x48)) {
          RenderInteriorWater_0(group, 4);
        } else {
          RenderExteriorWater_0(group, 4);
        }
        break;
    }
  }
  GxRsPop();
}

void CMapObj::RenderInteriorWater_0(const CMapObjGroup *group, UINT liquid) {
  SMOMaterial   *material = &materialList[group->liquidMtlId];
  int            nVerts = group->liquidVerts.x * group->liquidVerts.y;
  WORD          *idxBase;
  CGxVertexPCT0 *vtxBase;
  int            x;

  (void)liquid;
  FATALASSERT(nVerts < Gx_MaxVertices);
  vtxBase = (CGxVertexPCT0 *)GxAllocVertexMem(nVerts * sizeof(CGxVertexPCT0));
  idxBase = (WORD *)GxAllocIndexMem(nVerts * 3 * sizeof(WORD));

  UINT           idxSub = 0;
  SMOLVert      *liquidVert = group->liquidVertexList;
  CGxVertexPCT0 *vtx = vtxBase;
  float          px = group->liquidCorner.x;
  for (int y = 0; y < group->liquidVerts.y; ++y) {
    float py = group->liquidCorner.y;
    for (x = 0; x < group->liquidVerts.x; ++x, ++vtx, ++liquidVert) {
      vtx->p.Set(px, py, liquidVert->waterVert.height);
      vtx->c = material->diffColor;
      vtx->tc[0].x = (float)x;
      vtx->tc[0].y = (float)y;
      py += SMOLTILE_SIZE;
    }
    px -= SMOLTILE_SIZE;
  }

  RenderWaterIndices_0(group, idxBase, 0, idxSub);
  GxRsSet(GxRs_Lighting, 0);
  GxRsSet(GxRs_Fog, 1);
  DNInfo *dnInfo = DayNightGetInfo();
  if (dnInfo->intFog && this == CWorldScene::camMapObj) {
    GxRsSet(GxRs_FogStart, dnInfo->intFogInfo.start);
    GxRsSet(GxRs_FogEnd, dnInfo->intFogInfo.end);
    GxRsSet(GxRs_FogColor, dnInfo->intFogInfo.color);
  }
  GxPrimLockVertexPtrs(
      nVerts, &vtxBase[0].p, sizeof(CGxVertexPCT0), 0, 0, &vtxBase[0].c, sizeof(CGxVertexPCT0), 0, 0, &vtxBase[0].tc[0], sizeof(CGxVertexPCT0), 0, 0
  );
  GxPrimDrawElements(GxPrim_TriangleStrip, idxSub, idxBase);
  GxPrimUnlockVertexPtrs();
}

void CMapObj::RenderExteriorWater_0(const CMapObjGroup *group, UINT liquid) {
  NTempest::C3Vector  dumbNormal(0.0f, 0.0f, 1.0f);
  WORD               *idxBase;
  int                 nVerts = group->liquidVerts.x * group->liquidVerts.y;
  CGxVertexPNCT0     *vtxBase;
  int                 x;

  (void)liquid;
  FATALASSERT(nVerts < Gx_MaxVertices);
  vtxBase = (CGxVertexPNCT0 *)GxAllocVertexMem(nVerts * sizeof(CGxVertexPNCT0));
  idxBase = (WORD *)GxAllocIndexMem(nVerts * 3 * sizeof(WORD));

  UINT                idxSub = 0;
  DNInfo             *dnInfo = DayNightGetInfo();
  NTempest::CImVector shallowClr = dnInfo->light.WaterArray[3];
  SMOLVert           *liquidVert = group->liquidVertexList;
  CGxVertexPNCT0     *vtx = vtxBase;
  float               px = group->liquidCorner.x;
  for (int y = 0; y < group->liquidVerts.y; ++y) {
    float py = group->liquidCorner.y;
    for (x = 0; x < group->liquidVerts.x; ++x, ++vtx, ++liquidVert) {
      vtx->p.Set(px, py, liquidVert->waterVert.height);
      vtx->n = dumbNormal;
      vtx->c = shallowClr;
      vtx->tc[0].x = (float)x;
      vtx->tc[0].y = (float)y;
      py += SMOLTILE_SIZE;
    }
    px -= SMOLTILE_SIZE;
  }

  RenderWaterIndices_0(group, idxBase, 0, idxSub);
  if (idxSub) {
    GxPrimLockVertexPtrs(
        nVerts, &vtxBase[0].p, sizeof(CGxVertexPNCT0), &vtxBase[0].n, sizeof(CGxVertexPNCT0), &vtxBase[0].c, sizeof(CGxVertexPNCT0), 0, 0,
        &vtxBase[0].tc[0], sizeof(CGxVertexPNCT0), 0, 0
    );
    GxPrimDrawElements(GxPrim_TriangleStrip, idxSub, idxBase);
    GxPrimUnlockVertexPtrs();
  }
}

void CMapObj::RenderMagma(const CMapObjGroup *group, UINT liquid) {
  int            nVerts = group->liquidVerts.x * group->liquidVerts.y;
  CGxVertexPCT0 *vtxBase;
  WORD          *idxBase;
  int            y;
  CGxVertexPCT0 *vtx;

  FATALASSERT(nVerts < Gx_MaxVertices);
  vtxBase = (CGxVertexPCT0 *)GxAllocVertexMem(nVerts * sizeof(CGxVertexPCT0));
  idxBase = (WORD *)GxAllocIndexMem(nVerts * 3 * sizeof(WORD));

  UINT      idxSub = 0;
  SMOLVert *liquidVert = group->liquidVertexList;
  vtx = vtxBase;
  float px = group->liquidCorner.x;
  for (y = 0; y < group->liquidVerts.y; ++y) {
    float py = group->liquidCorner.y;
    for (int x = 0; x < group->liquidVerts.x; ++x, ++vtx, ++liquidVert) {
      vtx->p = NTempest::C3Vector(px, py, liquidVert->magmaVert.height);
      vtx->c = 0xFFFFFFFF;
      vtx->tc[0] = NTempest::C2Vector(liquidVert->magmaVert.s * 0.00390625f, liquidVert->magmaVert.t * 0.00390625f);
      py += SMOLTILE_SIZE;
    }
    px -= SMOLTILE_SIZE;
  }

  RenderWaterIndices_0(group, idxBase, 0, idxSub);
  int texXform = liquid & 0xC;
  if (texXform == 4) {
    GxXformPush(
        GxXform_Tex0,
        NTempest::C44Matrix(
            1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, (float)fmod(CWorld::GetCurTimeSec(), 10.0f) * 0.1f,
            0.0f, 1.0f
        )
    );
    GxRsSet(GxRs_TextureShader0, 1);
  }
  GxRsSet(GxRs_Lighting, 0);
  GxPrimLockVertexPtrs(
      nVerts, &vtxBase[0].p, sizeof(CGxVertexPCT0), 0, 0, &vtxBase[0].c, sizeof(CGxVertexPCT0), 0, 0, &vtxBase[0].tc[0], sizeof(CGxVertexPCT0), 0, 0
  );
  GxPrimDrawElements(GxPrim_TriangleStrip, idxSub, idxBase);
  GxPrimUnlockVertexPtrs();
  if (texXform == 4) {
    GxXformPop(GxXform_Tex0);
  }
}
