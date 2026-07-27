#include "WorldClient/CMapObj.h"
#include "WorldClient/World.h"

#include "Base/Base.h"
#include "DayNight.h"
#include "Gx/Gx.h"
#include "Services/Texture.h"
#include "Tempest/c4vector.h"

#include <float.h>
#include <math.h>
#include <string.h>

static NTempest::C4Vector tv[16];
static unsigned int       cnt;
static unsigned short     s_indexList[65535];
typedef void (CMapObj::*MapObjRenderFunc)(CMapObjGroup *, unsigned int);
static MapObjRenderFunc    s_intFunc;
static MapObjRenderFunc    s_extFunc;
static NTempest::C44Matrix s_mvp;
static NTempest::C44Matrix s_mw;
static NTempest::C44Matrix s_cm;

void CMapObj::SetGroupRenderCallback(void(*func)(const unsigned int, const void *, const int), void *userParam) {
  gRenderCallback = func;
  gRenderUserParam = userParam;
}

void CMapObj::PrepareUpdate() {
  CMapObj *mapObjnext_node;

  extViewList.SetCount(0);
  s_extFunc = &CMapObj::RenderGroupLightTex;
  if (CWorld::enables & CWorld::Enable_MapObjTex) {
    s_intFunc = &CMapObj::RenderGroupTex;
    if (CWorld::enables & CWorld::Enable_MapObjLight) {
      if (CWorld::enables & CWorld::Enable_VertexLight) {
        s_intFunc = &CMapObj::RenderGroupColorTex;
      } else {
        s_intFunc = &CMapObj::RenderGroupLightmapTex;
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
    s_intFunc = &CMapObj::RenderGroupBsp;
    s_extFunc = &CMapObj::RenderGroupBsp;
  }

  CMapObj *mapObj = mapObjHash.Head();
  while (mapObj) {
    mapObjnext_node = mapObjHash.Next(mapObj);
    mapObj->UpdateMaterials();

    CMapObjGroup *group = mapObj->groupList.Head();
    while (group) {
      group->lightmapTexFlushTime -= CWorld::GetTickTimeSec();
      group->flushTime -= CWorld::GetTickTimeSec();
      if (group->lightmapTexFlushTime <= 0.0f) {
        group->FreeLightmaps();
      }
      if (group->flushTime <= 0.0f && group->data) {
        group->Clear();
        mapObj->groupList.UnlinkNode(group);
      }
      group = mapObj->groupList.Next(group);
    }

    if (!mapObj->refCount) {
      mapObj->flushTime -= CWorld::GetTickTimeSec();
      if (mapObj->flushTime <= 0.0f) {
        mapObjHash.Unlink(mapObj);
        CMap::FreeMapObj(mapObj);
      }
    }
    mapObj = mapObjnext_node;
  }
}

void CMapObj::LocateViewer(NTempest::C44Matrix &im, TSGrowableArray<unsigned int> &inGroups) {
  FATALASSERT(0);
}

void CMapObj::IntRender(NTempest::C44Matrix &mat, TSGrowableArray<unsigned int> &inGroups) {
  FATALASSERT(inGroups.Count());

  ++gRenderCount;
  bIntRender = 1;
  extViewList.SetCount(0);
  GxXformViewProj(s_mvp);
  GxXform(GxXform_World, s_mw);
  s_cm = s_mw * s_mvp;

  unsigned int i;
  CWorldScene::FrustumPush();
  CMapObjGroup::rDrawSharedLiquidFirst = 0;
  {
    NTempest::CRect sRect(-1.0f, -1.0f, 1.0f, 1.0f);
    for (i = 0; i < inGroups.Count(); ++i) {
      RRenderThruPortals(inGroups[i], 0xFFFF, sRect, 0);
    }
  }
  CWorldScene::FrustumPop();
  bIntRender = 0;

  NTempest::C33Matrix m(mat.a0, mat.a1, mat.a2, mat.b0, mat.b1, mat.b2, mat.c0, mat.c1, mat.c2);
  NTempest::C3Vector  v(mat.d0, mat.d1, mat.d2);

  for (i = 0; i < extViewList.Count(); ++i) {
    NTempest::CRect sRect;
    CWorldScene::FrustumPush();
    CWorldScene::FrustumSet(CWorldScene::camFrustumCorners, extViewList[i]);

    sRect.Set(
        extViewList[i].t + extViewList[i].t - 1.0f, extViewList[i].l + extViewList[i].l - 1.0f, extViewList[i].b + extViewList[i].b - 1.0f,
        extViewList[i].r + extViewList[i].r - 1.0f
    );

    for (unsigned int groupIndex = 0; groupIndex < groupCount; ++groupIndex) {
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

void CMapObj::ExtRender(NTempest::C44Matrix &mat, NTempest::CRect &rect) {
  NTempest::C33Matrix m(mat.a0, mat.a1, mat.a2, mat.b0, mat.b1, mat.b2, mat.c0, mat.c1, mat.c2);
  NTempest::CRect     sRect;
  NTempest::C3Vector  v;

  ++gRenderCount;
  bIntRender = 0;
  extViewList.SetCount(0);
  GxXformViewProj(s_mvp);
  GxXform(GxXform_World, s_mw);
  s_cm = s_mw * s_mvp;

  v.Set(mat.d0, mat.d1, mat.d2);
  sRect.Set(rect.t + rect.t - 1.0f, rect.l + rect.l - 1.0f, rect.b + rect.b - 1.0f, rect.r + rect.r - 1.0f);

  CWorldScene::FrustumPush();
  CWorldScene::FrustumSet(CWorldScene::camFrustumCorners, rect);
  for (unsigned int i = 0; i < groupCount; ++i) {
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
    unsigned int                    groupNum,
    int                             rDrawSharedLiquidToggle,
    NTempest::C44Matrix            &invMat,
    TSExplicitList<CWFrustum, 244> &frustumList
) {
  CMapObjGroup *group = GetGroup(groupNum, 0);
  FATALASSERT(group);
  group->CreateLightmaps();

  CWFrustum   *frustum = frustumList.Head();
  unsigned int i = 0;
  while (reinterpret_cast<long>(frustum) > 0) {
    CWorldScene::FrustumSet(*frustum);
    CWorldScene::FrustumXform(invMat);
    if (group->flags & 0x48) {
      (this->*s_extFunc)(group, i);
    } else {
      (this->*s_intFunc)(group, i);
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
  DNInfo      *dnInfo = DayNightGetInfo();
  unsigned int amount = static_cast<unsigned int>(dnInfo->sidn * 255.0f - OneHalfOffset);

  for (unsigned int lp = 0; lp < materialCount; ++lp) {
    SMOMaterial &material = materialList[lp];
    if (material.flags & 0x10) {
      material.frameSidnColor = material.sidnColor;
      material.frameSidnColor.Set(
          material.frameSidnColor.a, static_cast<unsigned char>((amount * material.frameSidnColor.r) >> 8),
          static_cast<unsigned char>((amount * material.frameSidnColor.g) >> 8), static_cast<unsigned char>((amount * material.frameSidnColor.b) >> 8)
      );
    }
  }
}

void CMapObj::RenderAlways(unsigned int groupIdx) {
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

void CMapObj::RRenderThruPortals(unsigned int groupIdx, unsigned int parentIdx, NTempest::CRect &viewRect, unsigned int level) {
  NTempest::CRect sRect;
  NTempest::CRect newRect;
  unsigned int    toGroupIdx;
  unsigned int    i;
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
    gRenderCallback(groupIdx, gRenderUserParam, CMapObjGroup::rDrawSharedLiquidToggle == (level & 1));
  }

  cpIgnore = level == 0;
  portalRef = &portalRefList[group->portalStart];
  for (i = 0; i < group->portalCount; ++i, ++portalRef) {
    if (portalRef->groupIndex == 0xFFFF || portalRef->groupIndex == parentIdx) {
      continue;
    }

    toGroupIdx = portalRef->groupIndex;
    if (portalExtList[portalRef->portalIndex].xformTag != gRenderCount) {
      RTransformPortal(&portalList[portalRef->portalIndex], &portalExtList[portalRef->portalIndex], cpIgnore);
      portalExtList[portalRef->portalIndex].xformTag = gRenderCount;
    }

    if (portalExtList[portalRef->portalIndex].flags & 1) {
      continue;
    }

    if (!(portalExtList[portalRef->portalIndex].flags & 2)) {
      if ((NTempest::C3Vector::Dot(localCamPos, portalList[portalRef->portalIndex].plane.n) + portalList[portalRef->portalIndex].plane.d) *
              (portalRef->side < 0 ? -1.0f : 1.0f) <
          0.0f)
      {
        continue;
      }
    }

    if (portalExtList[portalRef->portalIndex].sRect.l > viewRect.r ||
        portalExtList[portalRef->portalIndex].sRect.r < viewRect.l ||
        portalExtList[portalRef->portalIndex].sRect.t > viewRect.b ||
        portalExtList[portalRef->portalIndex].sRect.b < viewRect.t)
    {
      continue;
    }

    newRect = NTempest::CRect::Intersection(portalExtList[portalRef->portalIndex].sRect, viewRect);
    if (fabs(newRect.b - newRect.t) < 0.001f || fabs(newRect.r - newRect.l) < 0.001f) {
      continue;
    }

    toGroup = GetGroup(toGroupIdx, 0);
    if (toGroup && (toGroup->flags & 8) && portalExtList[portalRef->portalIndex].visitedTag != gRenderCount && bIntRender)
    {
      sRect.Set((newRect.t + 1.0f) * 0.5f, (newRect.l + 1.0f) * 0.5f, (newRect.b + 1.0f) * 0.5f, (newRect.r + 1.0f) * 0.5f);
      extViewList.SetCount(extViewList.Count() + 1);
      extViewList[extViewList.Count() - 1] = sRect;
      portalExtList[portalRef->portalIndex].visitedTag = gRenderCount;
    }

    sRect.Set((newRect.t + 1.0f) * 0.5f, (newRect.l + 1.0f) * 0.5f, (newRect.b + 1.0f) * 0.5f, (newRect.r + 1.0f) * 0.5f);
    CWorldScene::FrustumPush();
    CWorldScene::FrustumSet(CWorldScene::camFrustumCorners, sRect);
    RRenderThruPortals(toGroupIdx, groupIdx, newRect, level + 1);
    CWorldScene::FrustumPop();
  }
}

void CMapObj::RTransformPortal(SMOPortal *portal, SPortalExt *portalExt, int cpIgnore) {
  unsigned int i;

  FATALASSERT(portal);
  FATALASSERT(portalExt);
  (void)cpIgnore;

  portalExt->flags = 0;
  for (i = 0; i < portal->count; ++i) {
    tv[i] = NTempest::C4Vector(
                portalVertexList[portal->startVertex + i].x, portalVertexList[portal->startVertex + i].y, portalVertexList[portal->startVertex + i].z,
                1.0f
            ) *
            s_cm;
    if (tv[i].w < CWorld::nearClip) {
      portalExt->flags |= 2;
    }
  }

  cnt = portal->count;
  CWorldScene::ClipPortal(tv, cnt);
  if (!cnt) {
    portalExt->flags |= 1;
  } else if (portalExt->flags & 2) {
    portalExt->sRect.Set(-1.0f, -1.0f, 1.0f, 1.0f);
  } else {
    portalExt->sRect.Set(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);
    for (i = 0; i < cnt; ++i) {
      if (fabs(tv[i].w) < 0.001f) {
        tv[i].w = 0.00001f;
      }
      tv[i].x /= tv[i].w;
      tv[i].y /= tv[i].w;
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

unsigned int CMapObj::CullBatch(SMOBatch *batch) {
  NTempest::CAaBox localBox(
      NTempest::C3Vector(static_cast<float>(batch->bx), static_cast<float>(batch->by), static_cast<float>(batch->bz)),
      NTempest::C3Vector(static_cast<float>(batch->tx), static_cast<float>(batch->ty), static_cast<float>(batch->tz))
  );
  return CWorldScene::FrustumCull(localBox);
}

void CMapObjGroup::ExtGxBufFillVertex(CGxBufCommand &cmd, CGxBuf *buf) {
  CGxVertexPNT0      *vtx;
  unsigned int        i;
  NTempest::C2Vector *t0;

  switch (cmd.vertex.op) {
    case GxBufOp_Nop:
      return;
    case GxBufOp_Fill:
      vtx = static_cast<CGxVertexPNT0 *>(*cmd.vertex.mem[GxVM_Position]);
      t0 = textureVertexList + sLockGxBatch->vertStart;
      for (i = 0; i < sLockGxBatch->vertCount; ++i) {
        vtx[i].p = vertexList[sLockGxBatch->vertStart + i];
        vtx[i].n = normalList[sLockGxBatch->vertStart + i];
        vtx[i].tc[0] = t0[i];
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
  unsigned int        i;
  NTempest::C3Vector *n;
  NTempest::C3Vector *p;

  switch (cmd.vertex.op) {
    case GxBufOp_Nop:
      return;
    case GxBufOp_Fill:
      vtx = static_cast<CGxVertexPNT0T1 *>(*cmd.vertex.mem[GxVM_Position]);
      p = vertexList + sLockGxBatch->vertStart;
      n = normalList + sLockGxBatch->vertStart;
      for (i = 0; i < sLockGxBatch->vertCount; ++i) {
        vtx[i].p = p[i];
        vtx[i].n = n[i];
        vtx[i].tc[0] = textureVertexList[sLockGxBatch->vertStart + i];
        vtx[i].tc[1] = lightmapVertexList[sLockGxBatch->vertStart + i];
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
    case GxBufOp_Fill:
      memcpy(*cmd.index.mem[GxVM_Indices], indexList + sLockGxBatch->vertStart, sLockGxBatch->vertCount * sizeof(unsigned short));
      return;
    case GxBufOp_Assign:
      *cmd.index.mem[GxVM_Indices] = indexList + sLockGxBatch->vertStart;
      return;
  }
}

void CMapObj::RenderGroupLightTex(CMapObjGroup *group, unsigned int frustumCount) {
  unsigned int i;
  CGxTexFlags  diffTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1);

  FATALASSERT(group);
  GxRsPush();
  GxVertexShaderSelect(GxVS_PassThru);

  for (i = 0; i < 4; ++i) {
    SMOGxBatch &gxBatch = group->extBatch[i];
    if (!gxBatch.batchCount) {
      continue;
    }

    FATALASSERT(!CMapObjGroup::sLockGxBatch);
    CMapObjGroup::sLockGxBatch = &gxBatch;
    GxBufLock(group->extGxBuf[i]);

    SMOBatch *batch = group->batchList + gxBatch.batchStart;
    for (unsigned int j = 0; j < gxBatch.batchCount; ++j, ++batch) {
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
        GxRsSet(GxRs_Blend, static_cast<int>(material.blendMode));

        CGxTex *texture = TextureGetGxTex(material.hMaps[0], 1, 0);
        GxTexFlags(texture, diffTexFlags);
        diffTexFlags.m_wrapU = !(material.flags & 0x40);
        diffTexFlags.m_wrapV = !(material.flags & 0x80);
        GxTexSetFlags(texture, diffTexFlags);
        GxRsSet(GxRs_Texture0, texture);

        CGxBatch drawBatch(GxPrim_Triangles, batch->count, group->indexList[batch->startIndex], batch->minIndex, batch->maxIndex);
        GxBufRender(drawBatch);
      }
    }

    GxBufUnlock();
    CMapObjGroup::sLockGxBatch = 0;
  }

  GxRsPop();
}

void CMapObj::RenderGroupLightmapTex_Int(CMapObjGroup *group, unsigned int frustumCount) {
  DNInfo      *dnInfo = DayNightGetInfo();
  unsigned int i;
  CGxTexFlags  diffTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1);

  if (dnInfo->intFog && this == CWorldScene::camMapObj) {
    GxRsPush();
    GxRsSet(GxRs_FogStart, dnInfo->intFogInfo.start);
    GxRsSet(GxRs_FogEnd, dnInfo->intFogInfo.end);
    GxRsSet(GxRs_FogColor, dnInfo->intFogInfo.color);
  }

  for (i = 0; i < 4; ++i) {
    SMOGxBatch &gxBatch = group->intBatch[i];
    if (!gxBatch.batchCount) {
      continue;
    }

    GxPrimLockVertexPtrs(
        gxBatch.vertCount, group->vertexList + gxBatch.vertStart, sizeof(NTempest::C3Vector), 0, 0, 0, 0, 0, 0,
        group->textureVertexList + gxBatch.vertStart, sizeof(NTempest::C2Vector), group->lightmapVertexList + gxBatch.vertStart,
        sizeof(NTempest::C2Vector)
    );

    SMOBatch *batch = group->batchList + gxBatch.batchStart;
    for (unsigned int j = 0; j < gxBatch.batchCount; ++j, ++batch) {
      if (!frustumCount) {
        batch->flags &= 0x0F;
      }
      if (!(batch->flags & 0xF0) && !CullBatch(batch)) {
        batch->flags |= 0xF0;
        SMOMaterial &material = materialList[batch->texture];
        GxRsSet(GxRs_Fog, !(material.flags & 0x2));
        GxRsSet(GxRs_Culling, !(material.flags & 0x4));
        GxRsSet(GxRs_Blend, static_cast<int>(material.blendMode));

        CGxTex *texture = TextureGetGxTex(material.hMaps[0], 1, 0);
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

void CMapObj::RenderGroupLightmapTex_Ext(CMapObjGroup *group, unsigned int frustumCount) {
  unsigned int i;
  DNInfo      *dnInfo = DayNightGetInfo();
  CGxTexFlags  diffTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1);

  for (i = 0; i < 4; ++i) {
    SMOGxBatch &gxBatch = group->extBatch[i];
    if (!gxBatch.batchCount) {
      continue;
    }

    GxPrimLockVertexPtrs(
        gxBatch.vertCount, group->vertexList + gxBatch.vertStart, sizeof(NTempest::C3Vector), group->normalList + gxBatch.vertStart,
        sizeof(NTempest::C3Vector), 0, 0, 0, 0, group->textureVertexList + gxBatch.vertStart, sizeof(NTempest::C2Vector), 0, 0
    );

    SMOBatch *batch = group->batchList + gxBatch.batchStart;
    for (unsigned int j = 0; j < gxBatch.batchCount; ++j, ++batch) {
      if (!frustumCount) {
        batch->flags &= 0x0F;
      }
      if (!(batch->flags & 0xF0) && !CullBatch(batch)) {
        batch->flags |= 0xF0;
        SMOMaterial &material = materialList[batch->texture];
        GxRsSet(GxRs_Fog, !(material.flags & 0x2));
        GxRsSet(GxRs_Culling, !(material.flags & 0x4));
        GxRsSet(GxRs_Blend, static_cast<int>(material.blendMode));

        if (material.flags & 0x20) {
          CMap::sunLight->gxLight.m_dirColor = dnInfo->lightInfo.windowDirColor;
          CMap::sunLight->gxLight.m_ambColor = dnInfo->lightInfo.windowAmbColor;
          GxLightSet(0, CMap::sunLight->gxLight, CWorldScene::camPos);
        }

        CGxTex *texture = TextureGetGxTex(material.hMaps[0], 1, 0);
        GxTexFlags(texture, diffTexFlags);
        diffTexFlags.m_wrapU = !(material.flags & 0x40);
        diffTexFlags.m_wrapV = !(material.flags & 0x80);
        GxTexSetFlags(texture, diffTexFlags);
        GxRsSet(GxRs_Texture0, texture);
        GxPrimDrawElements(GxPrim_Triangles, batch->count, group->indexList + batch->startIndex);

        if (material.flags & 0x20) {
          CMap::sunLight->gxLight.m_dirColor = dnInfo->lightInfo.dirColor;
          CMap::sunLight->gxLight.m_ambColor = dnInfo->lightInfo.ambColor;
          GxLightSet(0, CMap::sunLight->gxLight, CWorldScene::camPos);
        }
      }
    }

    GxPrimUnlockVertexPtrs();
  }
}

void CMapObj::RenderGroupLightmapTex(CMapObjGroup *group, unsigned int frustumCount) {
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

void CMapObj::RenderGroupColorTex_Int(CMapObjGroup *group, unsigned int frustumCount) {
  DNInfo      *dnInfo = DayNightGetInfo();
  unsigned int i;
  CGxTexFlags  diffTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1);

  if (dnInfo->intFog && this == CWorldScene::camMapObj) {
    GxRsPush();
    GxRsSet(GxRs_FogStart, dnInfo->intFogInfo.start);
    GxRsSet(GxRs_FogEnd, dnInfo->intFogInfo.end);
    GxRsSet(GxRs_FogColor, dnInfo->intFogInfo.color);
  }

  for (i = 0; i < 4; ++i) {
    SMOGxBatch &gxBatch = group->intBatch[i];
    if (!gxBatch.batchCount) {
      continue;
    }

    GxPrimLockVertexPtrs(
        gxBatch.vertCount, group->vertexList + gxBatch.vertStart, sizeof(NTempest::C3Vector), 0, 0, group->colorVertexList + gxBatch.vertStart,
        sizeof(NTempest::CImVector), 0, 0, group->textureVertexList + gxBatch.vertStart, sizeof(NTempest::C2Vector), 0, 0
    );

    SMOBatch *batch = group->batchList + gxBatch.batchStart;
    for (unsigned int j = 0; j < gxBatch.batchCount; ++j, ++batch) {
      if (!frustumCount) {
        batch->flags &= 0x0F;
      }
      if (!(batch->flags & 0xF0) && !CullBatch(batch)) {
        batch->flags |= 0xF0;
        SMOMaterial &material = materialList[batch->texture];
        GxRsSet(GxRs_Fog, !(material.flags & 0x2));
        GxRsSet(GxRs_Culling, !(material.flags & 0x4));
        GxRsSet(GxRs_Blend, static_cast<int>(material.blendMode));

        CGxTex *texture = TextureGetGxTex(material.hMaps[0], 1, 0);
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

void CMapObj::RenderGroupColorTex_Ext(CMapObjGroup *group, unsigned int frustumCount) {
  unsigned int i;
  DNInfo      *dnInfo = DayNightGetInfo();
  CGxTexFlags  diffTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1);

  for (i = 0; i < 4; ++i) {
    SMOGxBatch &gxBatch = group->extBatch[i];
    if (!gxBatch.batchCount) {
      continue;
    }

    GxPrimLockVertexPtrs(
        gxBatch.vertCount, group->vertexList + gxBatch.vertStart, sizeof(NTempest::C3Vector), group->normalList + gxBatch.vertStart,
        sizeof(NTempest::C3Vector), group->colorVertexList + gxBatch.vertStart, sizeof(NTempest::CImVector), 0, 0,
        group->textureVertexList + gxBatch.vertStart, sizeof(NTempest::C2Vector), 0, 0
    );

    SMOBatch *batch = group->batchList + gxBatch.batchStart;
    for (unsigned int j = 0; j < gxBatch.batchCount; ++j, ++batch) {
      if (!frustumCount) {
        batch->flags &= 0x0F;
      }
      if (!(batch->flags & 0xF0) && !CullBatch(batch)) {
        batch->flags |= 0xF0;
        SMOMaterial &material = materialList[batch->texture];
        GxRsSet(GxRs_Fog, !(material.flags & 0x2));
        GxRsSet(GxRs_Culling, !(material.flags & 0x4));
        GxRsSet(GxRs_Blend, static_cast<int>(material.blendMode));

        if (material.flags & 0x20) {
          CMap::sunLight->gxLight.m_dirColor = dnInfo->lightInfo.windowDirColor;
          CMap::sunLight->gxLight.m_ambColor = dnInfo->lightInfo.windowAmbColor;
          GxLightSet(0, CMap::sunLight->gxLight, CWorldScene::camPos);
        }

        CGxTex *texture = TextureGetGxTex(material.hMaps[0], 1, 0);
        GxTexFlags(texture, diffTexFlags);
        diffTexFlags.m_wrapU = !(material.flags & 0x40);
        diffTexFlags.m_wrapV = !(material.flags & 0x80);
        GxTexSetFlags(texture, diffTexFlags);
        GxRsSet(GxRs_Texture0, texture);
        GxPrimDrawElements(GxPrim_Triangles, batch->count, group->indexList + batch->startIndex);

        if (material.flags & 0x20) {
          CMap::sunLight->gxLight.m_dirColor = dnInfo->lightInfo.dirColor;
          CMap::sunLight->gxLight.m_ambColor = dnInfo->lightInfo.ambColor;
          GxLightSet(0, CMap::sunLight->gxLight, CWorldScene::camPos);
        }
      }
    }

    GxPrimUnlockVertexPtrs();
  }
}

void CMapObj::RenderGroupColorTex(CMapObjGroup *group, unsigned int frustumCount) {
  FATALASSERT(group);
  GxRsPush();
  GxVertexShaderSelect(GxVS_PassThru);
  GxRsSet(GxRs_Lighting, 0);
  RenderGroupColorTex_Int(group, frustumCount);
  GxRsSet(GxRs_Lighting, 1);
  RenderGroupColorTex_Ext(group, frustumCount);
  GxRsPop();
}

void CMapObj::RenderGroupLightmap(CMapObjGroup *group, unsigned int frustumCount) {
  NTempest::CImVector WHITE(0xFFFFFFFF);

  GxRsPush();
  GxRsSet(GxRs_Lighting, 0);

  for (unsigned int i = 0; i < 4; ++i) {
    SMOGxBatch &gxBatch = group->intBatch[i];
    if (!gxBatch.batchCount) {
      continue;
    }

    GxPrimLockVertexPtrs(
        gxBatch.vertCount, group->vertexList + gxBatch.vertStart, sizeof(NTempest::C3Vector), 0, 0, &WHITE, 0, 0, 0,
        group->lightmapVertexList + gxBatch.vertStart, sizeof(NTempest::C2Vector), 0, 0
    );
    SMOBatch *batch = group->batchList + gxBatch.batchStart;
    for (unsigned int j = 0; j < gxBatch.batchCount; ++j, ++batch) {
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

void CMapObj::RenderGroupTex(CMapObjGroup *group, unsigned int frustumCount) {
  NTempest::CImVector WHITE(0xFFFFFFFF);
  CGxTexFlags         diffTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1);

  GxRsPush();
  GxRsSet(GxRs_Lighting, 0);

  for (unsigned int i = 0; i < 4; ++i) {
    SMOGxBatch &gxBatch = group->intBatch[i];
    if (!gxBatch.batchCount) {
      continue;
    }

    GxPrimLockVertexPtrs(
        gxBatch.vertCount, group->vertexList + gxBatch.vertStart, sizeof(NTempest::C3Vector), 0, 0, &WHITE, 0, 0, 0,
        group->textureVertexList + gxBatch.vertStart, sizeof(NTempest::C2Vector), 0, 0
    );
    SMOBatch *batch = group->batchList + gxBatch.batchStart;
    for (unsigned int j = 0; j < gxBatch.batchCount; ++j, ++batch) {
      if (!frustumCount) {
        batch->flags &= 0x0F;
      }
      if (!(batch->flags & 0xF0) && !CullBatch(batch)) {
        batch->flags |= 0xF0;
        SMOMaterial &material = materialList[batch->texture];
        CGxTex      *texture = TextureGetGxTex(material.hMaps[0], 1, 0);
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

  GxRsPop();
}

void CMapObj::RenderGroup_Ext(CMapObjGroup *group, unsigned int frustumCount) {
  NTempest::CImVector WHITE(0xFFFFFFFF);

  GxVertexShaderSelect(GxVS_PassThru);

  for (unsigned int i = 0; i < 4; ++i) {
    SMOGxBatch &gxBatch = group->extBatch[i];
    if (!gxBatch.batchCount) {
      continue;
    }

    GxPrimLockVertexPtrs(
        gxBatch.vertCount, group->vertexList + gxBatch.vertStart, sizeof(NTempest::C3Vector), 0, 0, &WHITE, 0, 0, 0, 0, 0, 0, 0
    );
    SMOBatch *batch = group->batchList + gxBatch.batchStart;
    for (unsigned int j = 0; j < gxBatch.batchCount; ++j, ++batch) {
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

void CMapObj::RenderGroup_Int(CMapObjGroup *group, unsigned int frustumCount) {
  NTempest::CImVector WHITE(0xFFFFFFFF);

  GxRsPush();
  GxRsSet(GxRs_MatDiffuse, NTempest::CImVector(0x80A020A0));
  GxVertexShaderSelect(GxVS_PassThru);

  for (unsigned int i = 0; i < 4; ++i) {
    SMOGxBatch &gxBatch = group->intBatch[i];
    if (!gxBatch.batchCount) {
      continue;
    }

    GxPrimLockVertexPtrs(
        gxBatch.vertCount, group->vertexList + gxBatch.vertStart, sizeof(NTempest::C3Vector), 0, 0, &WHITE, 0, 0, 0, 0, 0, 0, 0
    );
    SMOBatch *batch = group->batchList + gxBatch.batchStart;
    for (unsigned int j = 0; j < gxBatch.batchCount; ++j, ++batch) {
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
  unsigned int        i;
  NTempest::CImVector argb(0x60FF00FF);
  unsigned int        indexCount = 0;

  if (!group->portalCount) {
    return;
  }

  SMOPortalRef *portalRef = portalRefList + group->portalStart;
  for (i = 0; i < group->portalCount; ++i, ++portalRef) {
    SMOPortal &portal = portalList[portalRef->portalIndex];
    for (unsigned int j = 0; j < portal.count - 2; ++j) {
      s_indexList[indexCount++] = portal.startVertex + j;
      s_indexList[indexCount++] = portal.startVertex + j + 1;
      s_indexList[indexCount++] = portal.startVertex + portal.count - 1;
    }
  }

  GxRsPush();
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

  unsigned int indexCount = 0;
  SMOPortal   *portal = portalList;
  for (unsigned int i = 0; i < portalCount; ++i, ++portal) {
    for (unsigned int j = 0; j < portal->count - 2; ++j) {
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

void CMapObj::RenderGroupBsp(CMapObjGroup *group, unsigned int frustumCount) {
  SMOPoly *poly;

  if (frustumCount) {
    return;
  }

  FATALASSERT(group);
  GxRsPush();
  GxRsSet(GxRs_Blend, 0);
  GxVertexShaderSelect(GxVS_PassThru);
  GxPrimLockVertexPtrs(
      group->vertexCount, group->vertexList, sizeof(NTempest::C3Vector), group->normalList, sizeof(NTempest::C3Vector), 0, 0, 0, 0, 0, 0, 0, 0
  );

  unsigned short *index = static_cast<unsigned short *>(GxAllocIndexMem(49152));
  unsigned short *nextIndex = index;
  poly = group->polyList;
  for (unsigned int i = 0; i < group->polyCount; ++i, ++poly) {
    if ((poly->flags & 0x20) && !(poly->flags & 0x4)) {
      *nextIndex++ = 3 * i;
      *nextIndex++ = 3 * i + 1;
      *nextIndex++ = 3 * i + 2;
      if (nextIndex - index >= 0xBFFD) {
        GxPrimDrawElements(GxPrim_Triangles, nextIndex - index, index);
        nextIndex = index;
      }
    }
  }
  if (nextIndex != index) {
    GxPrimDrawElements(GxPrim_Triangles, nextIndex - index, index);
  }

  GxRsSet(GxRs_MatDiffuse, NTempest::CImVector(0xFF8080FF));
  nextIndex = index;
  poly = group->polyList;
  for (unsigned int j = 0; j < group->polyCount; ++j, ++poly) {
    if (poly->flags & 0x4) {
      *nextIndex++ = 3 * j;
      *nextIndex++ = 3 * j + 1;
      *nextIndex++ = 3 * j + 2;
      if (nextIndex - index >= 0xBFFD) {
        GxPrimDrawElements(GxPrim_Triangles, nextIndex - index, index);
        nextIndex = index;
      }
    }
  }
  if (nextIndex != index) {
    GxPrimDrawElements(GxPrim_Triangles, nextIndex - index, index);
  }

  GxRsSet(GxRs_MatDiffuse, NTempest::CImVector(0x80FF0000));
  GxRsSet(GxRs_Blend, 2);
  nextIndex = index;
  poly = group->polyList;
  for (unsigned int k = 0; k < group->polyCount; ++k, ++poly) {
    if (poly->flags & 0x8) {
      *nextIndex++ = 3 * k;
      *nextIndex++ = 3 * k + 1;
      *nextIndex++ = 3 * k + 2;
      if (nextIndex - index >= 0xBFFD) {
        GxPrimDrawElements(GxPrim_Triangles, nextIndex - index, index);
        nextIndex = index;
      }
    }
  }
  if (nextIndex != index) {
    GxPrimDrawElements(GxPrim_Triangles, nextIndex - index, index);
  }

  GxPrimUnlockVertexPtrs();
  GxRsPop();
}

void CMapObj::RenderGroupNormals(CMapObjGroup *group) {
  unsigned int    numNormals = group->vertexCount;
  unsigned int    maxBatchNormals = numNormals >= 0x2000 ? 0x2000 : numNormals;
  CGxVertexPC    *vertex = static_cast<CGxVertexPC *>(GxAllocVertexMem(2 * maxBatchNormals * sizeof(CGxVertexPC)));
  unsigned short *index = static_cast<unsigned short *>(GxAllocIndexMem(2 * maxBatchNormals * sizeof(unsigned short)));
  unsigned int    base = 0;

  GxRsPush();
  GxRsSet(GxRs_Lighting, 0);
  while (numNormals) {
    unsigned int batchNormals = maxBatchNormals < numNormals ? maxBatchNormals : numNormals;
    numNormals -= batchNormals;

    for (unsigned int j = 0; j < batchNormals; ++j) {
      vertex[2 * j].p = group->vertexList[base + j];
      vertex[2 * j].c = 0xFFFFFF00;
      vertex[2 * j + 1].p = group->vertexList[base + j] + group->normalList[base + j] * 0.75f;
      vertex[2 * j + 1].c = 0xFFFFFF00;
      index[2 * j] = 2 * j;
      index[2 * j + 1] = 2 * j + 1;
    }

    base += batchNormals;
    GxPrimLockVertexPtrs(2 * batchNormals, &vertex[0].p, sizeof(CGxVertexPC), 0, 0, &vertex[0].c, sizeof(CGxVertexPC), 0, 0, 0, 0, 0, 0);
    GxPrimDrawElements(GxPrim_Lines, 2 * batchNormals, index);
    GxPrimUnlockVertexPtrs();
  }
  GxRsPop();
}

void CMapObj::RenderWaterIndices_0(CMapObjGroup *group, unsigned short *idxBase, unsigned int vtxSub, unsigned int &idxSub) {
  int             drawShared = CMapObjGroup::rDrawSharedLiquidToggle;
  int             ty;
  int             tx;
  unsigned short  lastRenderedVtx = 0;
  SMOLTile       *tile = group->liquidTileList;
  int             rendering = 0;
  unsigned short *index = idxBase;

  for (ty = 0; ty < group->liquidTiles.y; ++ty) {
    unsigned short i2 = static_cast<unsigned short>(vtxSub + ty * group->liquidVerts.x);
    unsigned short i3 = static_cast<unsigned short>(i2 + group->liquidVerts.x);
    for (tx = 0; tx < group->liquidTiles.x; ++tx, ++tile) {
      int render = (tile->flags & 0xF) != 0xF;
      if ((tile->flags & 0xF) == 0xF) {
        render = 0;
      } else if (tile->flags & 0x80) {
        render = drawShared;
      }

      if (!render) {
        if (rendering) {
          *index++ = lastRenderedVtx;
          rendering = 0;
        }
      } else {
        if (!rendering) {
          *index++ = i2;
          *index++ = i2;
          *index++ = i3;
          rendering = 1;
        }
        *index++ = i2 + 1;
        *index++ = i3 + 1;
        lastRenderedVtx = i3 + 1;
      }
      ++i2;
      ++i3;
    }

    if (rendering) {
      *index++ = lastRenderedVtx;
      rendering = 0;
    }
  }

  idxSub += index - idxBase;
}

void CMapObj::RenderLiquid_0(CMapObjGroup *group) {
  unsigned int liquid = 15;
  int          i = 0;

  while (i < group->liquidTiles.x && (group->liquidTileList[i].flags & 0xF) == 0xF) {
    ++i;
  }
  if (i < group->liquidTiles.x) {
    liquid = group->liquidTileList[i].flags & 0xF;
  }
  FATALASSERT(liquid != 15);

  GxVertexShaderSelect(GxVS_PassThru);
  GxRsPush();
  GxRsSet(GxRs_Culling, 0);
  CGxTex *texture = TextureGetGxTex(CMap::GetLiquidTexture(liquid), 0, 0);
  if (texture) {
    GxRsSet(GxRs_Texture0, texture);
    switch (liquid) {
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
      case 2:
      case 3:
      case 6:
      case 7:
        RenderMagma(group, liquid);
        break;
    }
  }
  GxRsPop();
}

void CMapObj::RenderInteriorWater_0(CMapObjGroup *group, unsigned int liquid) {
  int             nVerts = group->liquidVerts.x * group->liquidVerts.y;
  unsigned int    idxSub = 0;
  unsigned short *idxBase;
  CGxVertexPCT0  *vtxBase;
  int             x;
  int             y;

  (void)liquid;
  FATALASSERT(nVerts < Gx_MaxVertices);
  vtxBase = static_cast<CGxVertexPCT0 *>(GxAllocVertexMem(nVerts * sizeof(CGxVertexPCT0)));
  idxBase = static_cast<unsigned short *>(GxAllocIndexMem(nVerts * 3 * sizeof(unsigned short)));

  SMOMaterial   &material = materialList[group->liquidMtlId];
  SMOLVert      *liquidVert = group->liquidVertexList;
  CGxVertexPCT0 *vtx = vtxBase;
  float          px = group->liquidCorner.x;
  for (y = 0; y < group->liquidVerts.y; ++y) {
    float py = group->liquidCorner.y;
    for (x = 0; x < group->liquidVerts.x; ++x, ++vtx, ++liquidVert) {
      vtx->p.Set(px, py, liquidVert->height);
      vtx->c = material.diffColor;
      vtx->tc[0] = NTempest::C2Vector(static_cast<float>(x), static_cast<float>(y));
      py += 4.1666665f;
    }
    px -= 4.1666665f;
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

void CMapObj::RenderExteriorWater_0(CMapObjGroup *group, unsigned int liquid) {
  NTempest::C3Vector  dumbNormal(0.0f, 0.0f, 1.0f);
  unsigned short     *idxBase;
  int                 nVerts = group->liquidVerts.x * group->liquidVerts.y;
  NTempest::CImVector shallowClr = DayNightGetInfo()->light.WaterArray[3];
  CGxVertexPNCT0     *vtxBase;
  unsigned int        idxSub = 0;
  int                 x;
  int                 y;

  (void)liquid;
  FATALASSERT(nVerts < Gx_MaxVertices);
  vtxBase = static_cast<CGxVertexPNCT0 *>(GxAllocVertexMem(nVerts * sizeof(CGxVertexPNCT0)));
  idxBase = static_cast<unsigned short *>(GxAllocIndexMem(nVerts * 3 * sizeof(unsigned short)));

  SMOLVert       *liquidVert = group->liquidVertexList;
  CGxVertexPNCT0 *vtx = vtxBase;
  float           px = group->liquidCorner.x;
  for (y = 0; y < group->liquidVerts.y; ++y) {
    float py = group->liquidCorner.y;
    for (x = 0; x < group->liquidVerts.x; ++x, ++vtx, ++liquidVert) {
      vtx->p.Set(px, py, liquidVert->height);
      vtx->n = dumbNormal;
      vtx->c = shallowClr;
      vtx->tc[0] = NTempest::C2Vector(static_cast<float>(x), static_cast<float>(y));
      py += 4.1666665f;
    }
    px -= 4.1666665f;
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

void CMapObj::RenderMagma(CMapObjGroup *group, unsigned int liquid) {
  int             nVerts = group->liquidVerts.x * group->liquidVerts.y;
  CGxVertexPCT0  *vtxBase;
  unsigned int    idxSub = 0;
  unsigned short *idxBase;
  int             y;
  CGxVertexPCT0  *vtx;

  FATALASSERT(nVerts < Gx_MaxVertices);
  vtxBase = static_cast<CGxVertexPCT0 *>(GxAllocVertexMem(nVerts * sizeof(CGxVertexPCT0)));
  idxBase = static_cast<unsigned short *>(GxAllocIndexMem(nVerts * 3 * sizeof(unsigned short)));

  SMOLVert *liquidVert = group->liquidVertexList;
  vtx = vtxBase;
  float px = group->liquidCorner.x;
  for (y = 0; y < group->liquidVerts.y; ++y) {
    float py = group->liquidCorner.y;
    for (int x = 0; x < group->liquidVerts.x; ++x, ++vtx, ++liquidVert) {
      short *flow = reinterpret_cast<short *>(&liquidVert->color);
      vtx->p.Set(px, py, liquidVert->height);
      vtx->c = 0xFFFFFFFF;
      vtx->tc[0] = NTempest::C2Vector(flow[0] * 0.00390625f, flow[1] * 0.00390625f);
      py += 4.1666665f;
    }
    px -= 4.1666665f;
  }

  RenderWaterIndices_0(group, idxBase, 0, idxSub);
  int texXform = liquid & 0xC;
  if (texXform == 4) {
    NTempest::C44Matrix texMat;
    texMat.d1 = fmod(CWorld::GetCurTimeSec(), 10.0f) * 0.1f;
    GxXformSet(GxXform_Tex0, texMat);
    GxRsSet(GxRs_TextureShader0, 1);
  }
  GxRsSet(GxRs_Lighting, 0);
  GxPrimLockVertexPtrs(
      nVerts, &vtxBase[0].p, sizeof(CGxVertexPCT0), 0, 0, &vtxBase[0].c, sizeof(CGxVertexPCT0), 0, 0, &vtxBase[0].tc[0], sizeof(CGxVertexPCT0), 0, 0
  );
  GxPrimDrawElements(GxPrim_TriangleStrip, idxSub, idxBase);
  GxPrimUnlockVertexPtrs();
  if (texXform == 4) {
    GxXformIdentity(GxXform_Tex0);
  }
}
