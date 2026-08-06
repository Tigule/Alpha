#include <WowConst.h>
#include <MapDefs.h>

#include "WorldClient/World.h"

#include "WorldClient/CMapObj.h"
#include "WorldCommon/WorldMath.h"

#include "Base/Activity.h"
#include "Base/Status.h"
#include "Gx/Gx.h"
#include "Gxu/IGxuLight.h"
#include "Services/Texture.h"
#include "Tempest/cmath.h"
#include "Tempest/crect.h"

#include <new>

static const float OO_COORD_TO_CHUNK = 1.0f / ((150.0f / 36.0f) * 8);

HTEXTURE__ *CMapLight::s_hPointAttenTex;
LISTDECLEX(CMapBaseObjLink, refLink, CMapLight::dirLightLinkList);
UINT  CMapLight::maxLights = 4;
float CMapLight::bucketSize = 33.33f;
float CMapLight::halfBucketSize = 16.665f;

void CMap::ProjectLights() {
}

void CMap::SetLightFuncs() {
  GxuLightFuncsSet(
      CMap::GxuLightInitialize, CMap::GxuLightShutdown, CMap::GxuLightCreate, CMap::GxuLightDestroy, CMap::GxuLightLock, CMap::GxuLightUnlock,
      CMap::GxuLightSelect, CMap::GxuLightEnable, CMap::GxuLightEnableSet, CMap::GxuLightSetMaxLights, CMap::GxuLightBucketSize,
      CMap::GxuLightBucketSizeSet, CMap::GxuLightResetCache
  );
}

void CMap::GxuLightInitialize() {
}

void CMap::GxuLightShutdown() {
}

DWORD CMap::GxuLightCreate() {
  return reinterpret_cast<DWORD>(CreateLight(true));
}

void CMap::GxuLightDestroy(DWORD lightId) {
  CMapLight *light = reinterpret_cast<CMapLight *>(lightId);

  ASSERT(light);
  DestroyLight(light);
}

CGxLight *CMap::GxuLightLock(DWORD lightId) {
  CMapLight *light = reinterpret_cast<CMapLight *>(lightId);

  ASSERT(light);
  return &light->gxLight;
}

void CMap::GxuLightUnlock(DWORD lightId) {
  CMapLight *light = reinterpret_cast<CMapLight *>(lightId);

  ASSERT(light);
  UpdateLight(light);
}

void CMap::GxuLightSelect(NTempest::C3Vector worldPos, const NTempest::C3Vector &cameraWorldPos, UINT maxLightsToUse) {
  UINT whichLight = 0;

  oldSelectLightParm = 0;
  if (bActive) {
    GxLightSet(0, sunLight->gxLight, cameraWorldPos);
    whichLight = 1;
  } else {
    ITERATELIST(CMapBaseObjLink, CMapLight::dirLightLinkList, link) {
      CMapLight *light = static_cast<CMapLight *>(link->owner);
      if (light->flags & CMapBaseObj::Flag_Enabled) {
        GxLightSet(whichLight, light->gxLight, cameraWorldPos);
        ++whichLight;
        if (whichLight >= maxLightsToUse) {
          break;
        }
      }
    }
  }

  while (whichLight < 8) {
    GxLightEnable(whichLight, 0);
    ++whichLight;
  }
}

int CMap::GxuLightEnable(DWORD lightId) {
  CMapLight *light = reinterpret_cast<CMapLight *>(lightId);

  ASSERT(light);
  return (light->flags & CMapBaseObj::Flag_Enabled) != 0;
}

void CMap::GxuLightEnableSet(DWORD lightId, int enable) {
  CMapLight *light = reinterpret_cast<CMapLight *>(lightId);

  ASSERT(light);
  if (enable) {
    EnableLight(light);
  } else {
    DisableLight(light);
  }
}

void CMap::GxuLightSetMaxLights(UINT maxLightsToUse) {
}

float CMap::GxuLightBucketSize() {
  return CMapLight::bucketSize;
}

void CMap::GxuLightBucketSizeSet(float bucketSize) {
}

void CMap::GxuLightResetCache() {
}

CMapLight::CMapLight() {
  type |= 0x80u;
  attenStart = attenEnd = attenDenom = 0.0f;
}

CMapLight::~CMapLight() {
  ASSERT(refCount == 0);
}

void CMapLight::CreatePointAtten() {
  CStatus lame;

  s_hPointAttenTex = TextureCreate("Textures\\PointAtten.blp", CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &lame, 0);
}

void CMapLight::DestroyPointAtten() {
  HandleClose(s_hPointAttenTex);
  s_hPointAttenTex = 0;
}

CGxTex *CMapLight::GetPointAttenTex() {
  return TextureGetGxTex(s_hPointAttenTex, 1, 0);
}

void CMapLight::ProjectLightRenderPN(CGxBufCommand &cmd, CGxBuf *buf) {
  const CWTriData::Batch *batch = static_cast<const CWTriData::Batch *>(buf->UserArg());
  CGxVertexPN            *vertices = 0;

  switch (cmd.vertex.op) {
    case GxBufOp_Fill:
      vertices = static_cast<CGxVertexPN *>(*cmd.vertex.mem[GxVM_Position]);
      break;

    case GxBufOp_Assign:
      vertices = static_cast<CGxVertexPN *>(GxAllocVertexMem(buf->VertexCount() * sizeof(*vertices)));
      *cmd.vertex.mem[GxVM_Position] = &vertices->p;
      *cmd.vertex.mem[GxVM_Normal] = &vertices->n;
      break;

    default:
      FATALASSERT(0);
  }

  WORD vidx = batch->GetMinIndex();
  for (UINT i = 0; i < batch->GetVertexCount(); ++i, ++vidx) {
    vertices[i].p = batch->GetVertex(vidx);
    vertices[i].n = batch->GetNormal(vidx);
  }

  WORD *indices = 0;
  switch (cmd.index.op) {
    case GxBufOp_Fill:
      indices = static_cast<WORD *>(*cmd.index.mem[GxVM_Indices]);
      break;

    case GxBufOp_Assign:
      indices = static_cast<WORD *>(GxAllocIndexMem(buf->IndexCount() * sizeof(*indices)));
      *cmd.index.mem[GxVM_Indices] = indices;
      break;

    default:
      FATALASSERT(0);
  }

  for (UINT j = 0; j < batch->GetIndexCount(); ++j) {
    indices[j] = batch->GetIndex(j) - batch->GetMinIndex();
  }
}

void CMapLight::Project() {
  NTempest::C44Matrix texMtx0;
  NTempest::C44Matrix texMtx1;
  NTempest::C44Matrix worldTransMat;
  worldTransMat.Translate(CWorldScene::camPos - pos);

  NTempest::C44Matrix texScale;
  texScale.a0 = 1.0f / (aaBox.t.x - aaBox.b.x);
  texScale.b1 = texScale.a0;
  texScale.c2 = texScale.a0;

  texMtx0 = worldTransMat * texScale;
  texMtx0.d0 += 0.5f;
  texMtx0.d1 += 0.5f;
  texMtx0.d2 += 0.5f;

  texMtx1 = worldTransMat * texScale;
  NTempest::C44Matrix rotateToScreen(0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
  texMtx1 *= rotateToScreen;
  texMtx1.d0 += 0.5f;
  texMtx1.d1 += 0.5f;
  texMtx1.d2 += 0.5f;

  GxXformSet(GxXform_Tex0, texMtx0);
  GxXformSet(GxXform_Tex1, texMtx1);

  CGxBuf *buf = GxBufGetDynamic(GxVBF_PN);
  buf->UserCallbackSet(ProjectLightRenderPN);

  CWTriData triData;
  CMap::GetTris(aaBox, triData, 0x122);

  NTempest::C44Matrix worldMtx;
  worldMtx.Translate(-CWorldScene::camPos);

  for (UINT i = 0; i < triData.GetNumBatches(); ++i) {
    const CWTriData::Batch &batch = triData.GetBatch(i);
    NTempest::C44Matrix     batchMtx = *batch.matrix * worldMtx;
    GxXformSet(GxXform_World, batchMtx);
    buf->UserArgSet(const_cast<CWTriData::Batch *>(&batch));
    buf->CountSet(batch.GetVertexCount(), batch.GetIndexCount());
    GxBufLock(buf);
    CGxBatch gxBatch(GxPrim_Triangles, batch.GetIndexCount(), 0, -1, -1);
    GxBufRender(gxBatch);
    GxBufUnlock();
  }
}

CMapLight *CMap::CreateLight(bool dynamic) {
  CMapLight *light = AllocLight();

  light->pos = NTempest::C3Vector(0.0f);
  light->aaSphere.c = NTempest::C3Vector(0.0f);
  light->aaBox.b = NTempest::C3Vector(0.0f);
  light->aaBox.t = NTempest::C3Vector(0.0f);
  light->aaSphere.r = 0.0f;
  light->flags = 0;
  light->attenStart = light->attenEnd = light->attenDenom = 0.0f;
  light->dynamic = dynamic;
  new (&light->gxLight) CGxLight;

  return light;
}

void CMap::DestroyLight(CMapLight *light) {
  ASSERT(light);

  CMapBaseObjLink *link = light->parentLinkList.Head();
  while (link) {
    CMapBaseObjLink *next = light->parentLinkList.Next(link);

    if (link->ref) {
      if (link->ref->GetType() == CMapBaseObj::Type_Chunk) {
        static_cast<CMapChunk *>(link->ref)->UpdateLights();
      } else if (link->ref->GetType() == CMapBaseObj::Type_MapObjDefGroup) {
        static_cast<CMapObjDefGroup *>(link->ref)->UpdateLights();
      }
    }

    FreeBaseObjLink(link);
    link = next;
  }

  FreeLight(light);
}

void CMap::UpdateLight(CMapLight *light) {
  ASSERT(light);

  if (!light->parentLinkList.Head() && !light->gxLight.m_isOmni) {
    CMapBaseObjLink *link = AllocBaseObjLink(light);
    link->ref = 0;
    CMapLight::dirLightLinkList.LinkNode(link, LIST_TAIL, 0);
  }

  if (bActive && light->gxLight.m_isOmni) {
    ActivityBegin(ACTIVITY_LIGHTING);

    CMapBaseObjLink *link = light->parentLinkList.Head();
    while (link) {
      CMapBaseObjLink *next = light->parentLinkList.Next(link);
      FreeBaseObjLink(link);
      link = next;
    }

    UpdateLightBounds(light);
    LinkLightToMapObjDefs(light);
    LinkLightToChunks(light);

    ActivityEnd(ACTIVITY_LIGHTING);
  }
}

void CMap::UpdateLightBounds(CMapLight *light) {
  float radius = light->attenEnd;
  if (light->attenDenom == 0.0f) {
    radius = 5.0f;
  }

  light->attenEnd = radius;

  NTempest::C3Vector min = NTempest::C3Vector(light->gxLight.m_dir.x - radius, light->gxLight.m_dir.y - radius, light->gxLight.m_dir.z - radius);
  NTempest::C3Vector max = NTempest::C3Vector(light->gxLight.m_dir.x + radius, light->gxLight.m_dir.y + radius, light->gxLight.m_dir.z + radius);

  light->pos = light->gxLight.m_dir;
  light->aaBox.b = min;
  light->aaBox.t = max;
  light->aaSphere.r = radius;
  light->aaSphere.c = light->gxLight.m_dir;
}

void CMap::EnableLight(CMapLight *light) {
  ASSERT(light);

  light->flags |= CMapBaseObj::Flag_Enabled;
  light->gxLight.m_enabled = 1;
}

void CMap::DisableLight(CMapLight *light) {
  ASSERT(light);

  light->flags &= ~CMapBaseObj::Flag_Enabled;
  light->gxLight.m_enabled = 0;
}

void CMap::SelectLight(CMapBaseObj *baseObj) {
  FATALASSERT(baseObj);
  if (baseObj->camDist < CWorld::farFog) {
    oldSelectLightParm = baseObj;
    baseObj->SelectLights();
  }
}

void CMap::SelectLight(LPVOID parm, NTempest::C3Vector worldPos, const NTempest::C3Vector &cameraWorldPos, UINT maxLightsToUse) {
  CMapBaseObj *baseObj = static_cast<CMapBaseObj *>(parm);

  ASSERT(baseObj);
  if (baseObj->camDist < CWorld::farFog) {
    oldSelectLightParm = baseObj;
    baseObj->SelectLights();
  }
}

void CMap::LinkLightToMapObjDefs(CMapLight *light) {
  ASSERT(light);

  NTempest::CAaBox   tBox;
  NTempest::C3Vector lCen = (light->aaBox.b + light->aaBox.t) * 0.5f;
  NTempest::CAaBox   lBox = light->aaBox;
  NTempest::C3Vector tCen(-lCen.x, -lCen.y, -lCen.z);
  lBox.b += tCen;
  lBox.t += tCen;

  ITERATELIST(CMapObjDef, mapObjDefHash, mapObjDef) {
    if (mapObjDef->aaBox.b <= light->aaBox.t && mapObjDef->aaBox.t >= light->aaBox.b) {
      tCen = lCen * mapObjDef->invMat;
      NTempest::C33Matrix tMat(
          mapObjDef->invMat.a0, mapObjDef->invMat.a1, mapObjDef->invMat.a2, mapObjDef->invMat.b0, mapObjDef->invMat.b1, mapObjDef->invMat.b2,
          mapObjDef->invMat.c0, mapObjDef->invMat.c1, mapObjDef->invMat.c2
      );
      CWorldMath::TransformAABox(tMat, lBox, tBox);
      tBox.b += tCen;
      tBox.t += tCen;

      CMapObj *mapObj = mapObjDef->mapObj;
      if (mapObj && mapObj->TestBounds(tBox)) {
        ITERATELIST(CMapBaseObjLink, mapObjDef->groupLinkList, link) {
          CMapObjDefGroup *group = static_cast<CMapObjDefGroup *>(link->owner);
          if (!(group->flags & CMapBaseObj::Flag_InteriorLit) && mapObj->TestGroupBounds(tBox, group->groupNum)) {
            CMapBaseObjLink *lightLink = AllocBaseObjLink(light);
            lightLink->ref = group;
            group->lightLinkList.LinkNode(lightLink, LIST_TAIL, 0);
            group->UpdateLights();
          }
        }
      }
    }
  }
}

void CMap::LinkLightToChunks(CMapLight *light) {
  ASSERT(light);

  NTempest::CRect tLocation(
      -(light->aaBox.t.x - 17066.666f), -(light->aaBox.t.y - 17066.666f), -(light->aaBox.b.x - 17066.666f), -(light->aaBox.b.y - 17066.666f)
  );

  ASSERT(tLocation.minx >= 0.0f && tLocation.miny >= 0.0f);
  ASSERT(tLocation.maxy < ((64 * 16) * ((150.0f / 36.0f) * 8)) && tLocation.maxy < ((64 * 16) * ((150.0f / 36.0f) * 8)));

  NTempest::CiRect cLocation(
      NTempest::CMath::fint_mi(tLocation.miny * OO_COORD_TO_CHUNK), NTempest::CMath::fint_mi(tLocation.minx * OO_COORD_TO_CHUNK),
      NTempest::CMath::fint_mi(tLocation.maxy * OO_COORD_TO_CHUNK), NTempest::CMath::fint_mi(tLocation.maxx * OO_COORD_TO_CHUNK)
  );

  for (int cy = cLocation.miny; cy <= cLocation.maxy; ++cy) {
    for (int cx = cLocation.minx; cx <= cLocation.maxx; ++cx) {
      CMapArea *area = areaTable[(((cy >> 4) & 0x3F) << 6) + ((cx >> 4) & 0x3F)];
      if (area) {
        CMapChunk *chunk = area->chunkTable[((cy & 0xF) << 4) + (cx & 0xF)];
        if (chunk) {
          CMapBaseObjLink *link = AllocBaseObjLink(light);
          link->ref = chunk;
          chunk->lightLinkList.LinkNode(link, LIST_TAIL, 0);
          chunk->UpdateLights();
        }
      }
    }
  }
}
