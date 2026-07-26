#include "WorldClient/World.h"

#include "WorldClient/CMapObj.h"

#include "DayNight.h"

#include <DB/DBClient/DBClient.h>
#include <Gx/Gx.h>
#include <WorldCommon/WorldMath.h>

#include <Tempest/cpriorityq.h>

const float CMapEntity::ambLightScaleRate = 2.0f;
const float CMapEntity::dirLightScaleRate = 3.3333333f;

struct FogQ {
  FogQ() {
  }

  FogQ(float pDist, unsigned int pFogId) : dist(pDist), fogId(pFogId) {
  }

  static unsigned int HasHigherPriority(FogQ &a, FogQ &b) {
    return a.dist >= b.dist;
  }

  float        dist;
  unsigned int fogId;
};

int CMapStaticEntity::GetMapObjDef(CMapObjDef *&mapObjDef) {
  CMapBaseObjLink *parentLink = parentLinkList.Head();

  while (reinterpret_cast<long>(parentLink) > 0) {
    if (parentLink->ref->GetType() & Type_MapObjDefGroup) {
      CMapObjDefGroup *mapObjDefGroup = static_cast<CMapObjDefGroup *>(parentLink->ref);
      mapObjDef = static_cast<CMapObjDef *>(mapObjDefGroup->parentLinkList.Head()->ref);
      FATALASSERT(mapObjDef->GetType() & Type_MapObjDef);
      return 1;
    }

    parentLink = parentLinkList.RawNext(parentLink);
  }

  return 0;
}

CMapEntity::~CMapEntity() {
  FATALASSERT(refCount == 0);
}

int CMapStaticEntity::GetMapObjAndGroup(CMapObjDef *&mapObjDef, CMapObj *&mapObj, CMapObjDefGroup *&mapObjDefGroup, CMapObjGroup *&mapObjGroup) {
  if (!flagInside) {
    return 0;
  }

  CMapBaseObjLink *parentLink = parentLinkList.Head();
  while (reinterpret_cast<long>(parentLink) > 0) {
    if (parentLink->ref->GetType() & Type_MapObjDefGroup) {
      mapObjDefGroup = static_cast<CMapObjDefGroup *>(parentLink->ref);
      mapObjDef = static_cast<CMapObjDef *>(mapObjDefGroup->parentLinkList.Head()->ref);
      FATALASSERT(mapObjDef->GetType() & Type_MapObjDef);

      mapObj = mapObjDef->mapObj;
      FATALASSERT(mapObj);
      mapObjGroup = mapObj->GetGroup(mapObjDefGroup->groupNum, 0);
      FATALASSERT(mapObjGroup);
      return 1;
    }

    parentLink = parentLinkList.RawNext(parentLink);
  }

  return 0;
}

CMapEntity::CMapEntity() {
  type |= Type_Entity;
}

void CMapEntity::QueryLiquidSounds(
    int                *lbool,
    NTempest::C3Vector *ldelta,
    float              *ldsquared,
    unsigned int       &closestExtLevel
) {
  if (!flagInside) {
    return;
  }

  CMapBaseObjLink *parentLink = parentLinkList.Head();
  while (reinterpret_cast<long>(parentLink) > 0) {
    if (parentLink->ref->GetType() & Type_MapObjDefGroup) {
      CMapObjDefGroup *mapObjDefGroup = static_cast<CMapObjDefGroup *>(parentLink->ref);
      CMapObjDef *mapObjDef = static_cast<CMapObjDef *>(mapObjDefGroup->parentLinkList.Head()->ref);
      FATALASSERT(mapObjDef->GetType() & Type_MapObjDef);
      CMapObj *mapObj = mapObjDef->mapObj;
      FATALASSERT(mapObj);

      NTempest::C3Vector localPos = pos * mapObjDef->invMat;
      mapObj->QueryLiquidSounds(
          mapObjDefGroup->groupNum,
          mapObjDefGroup->groupNum,
          0,
          closestExtLevel,
          localPos,
          lbool,
          ldelta,
          ldsquared
      );
    }
    parentLink = parentLinkList.RawNext(parentLink);
  }
}

int CMapEntity::QueryMapObjZoneName(const char *&zoneName) {
  CMapObjDefGroup *mapObjDefGroup;
  CMapObjGroup    *mapObjGroup;
  CMapObj         *mapObj;
  CMapObjDef      *mapObjDef;
  if (!GetMapObjAndGroup(mapObjDef, mapObj, mapObjDefGroup, mapObjGroup)) {
    return 0;
  }

  if (!mapObjDef->zoneName) {
    mapObjDef->zoneName = SDBWMOAreaTableLookup(mapObj->GetWmoID(), mapObjDef->nameSet, -1);
  }

  zoneName = mapObjDef->zoneName;
  return 1;
}

int CMapEntity::QueryMapObjSubzoneName(const char *&subzoneName, unsigned int &subzoneId) {
  CMapObjDef      *mapObjDef;
  CMapObj         *mapObj;
  CMapObjGroup    *mapObjGroup;
  CMapObjDefGroup *mapObjDefGroup;
  if (!GetMapObjAndGroup(mapObjDef, mapObj, mapObjDefGroup, mapObjGroup)) {
    return 0;
  }

  if (!mapObjDefGroup->subzoneName) {
    mapObjDefGroup->subzoneName = SDBWMOAreaTableLookup(mapObj->GetWmoID(), mapObjDef->nameSet, mapObjGroup->uniqueID);
  }

  subzoneName = mapObjDefGroup->subzoneName;
  subzoneId = mapObjDefGroup->groupNum;
  return 1;
}

bool CMapEntity::QueryMapObjAreaTable(const WMOAreaTableRec *&subzoneRec, const WMOAreaTableRec *&globalRec) {
  CMapObjGroup    *mapObjGroup;
  CMapObjDefGroup *mapObjDefGroup;
  CMapObjDef      *mapObjDef;
  CMapObj         *mapObj;
  if (!GetMapObjAndGroup(mapObjDef, mapObj, mapObjDefGroup, mapObjGroup)) {
    return false;
  }

  if (!SDBWMOAreaTableLookup(mapObj->GetWmoID(), mapObjDef->nameSet, mapObjDefGroup->groupNum, subzoneRec)) {
    return false;
  }

  return SDBWMOAreaTableLookup(mapObj->GetWmoID(), mapObjDef->nameSet, -1, globalRec);
}

int CMapEntity::QueryMapObjFileName(const char *&fileName) {
  CMapObjDef      *mapObjDef;
  CMapObjDefGroup *mapObjDefGroup;
  CMapObjGroup    *mapObjGroup;
  CMapObj         *mapObj;
  if (!GetMapObjAndGroup(mapObjDef, mapObj, mapObjDefGroup, mapObjGroup)) {
    return 0;
  }

  fileName = mapObj->name;
  return 1;
}

int CMapEntity::QueryMapObjListenerId(unsigned int &listenerId) {
  CMapObjGroup    *mapObjGroup;
  CMapObjDefGroup *mapObjDefGroup;
  CMapObj         *mapObj;
  CMapObjDef      *mapObjDef;
  if (!GetMapObjAndGroup(mapObjDef, mapObj, mapObjDefGroup, mapObjGroup)) {
    return 0;
  }

  listenerId = 0;
  return 1;
}

unsigned int CMapEntity::QueryMapObjMinimap(NTempest::CAaBox &aaBox, TSStackArray<CWorldMinimapQuad> &quads) {
  NTempest::CAaBox localBox;
  CMapObjGroup    *mapObjGroup;
  CMapObj         *mapObj;
  CMapObjDefGroup *mapObjDefGroup;
  CMapObjDef      *mapObjDef;
  if (!GetMapObjAndGroup(mapObjDef, mapObj, mapObjDefGroup, mapObjGroup)) {
    return 0;
  }

  CWorldMath::TransformAABox(mapObjDef->invMat, aaBox, localBox);
  return mapObj->QueryMapObjMinimap(mapObjDefGroup->groupNum, localBox, quads);
}

unsigned int CMapEntity::QueryMapObjIDs(unsigned int &wmoID, unsigned int &instanceID, unsigned int &groupID) {
  CMapObjGroup    *mapObjGroup;
  CMapObjDefGroup *mapObjDefGroup;
  CMapObjDef      *mapObjDef;
  CMapObj         *mapObj;
  if (!GetMapObjAndGroup(mapObjDef, mapObj, mapObjDefGroup, mapObjGroup)) {
    return 0;
  }

  wmoID = mapObj->GetWmoID();
  instanceID = mapObj->m_hashval;
  groupID = mapObjDefGroup->groupNum;
  return 1;
}

unsigned int CMapEntity::QueryMapObjMatrix(NTempest::C44Matrix *mtx, NTempest::C44Matrix *invMtx) {
  CMapObj         *mapObj;
  CMapObjDefGroup *mapObjDefGroup;
  CMapObjGroup    *mapObjGroup;
  CMapObjDef      *mapObjDef;
  if (!GetMapObjAndGroup(mapObjDef, mapObj, mapObjDefGroup, mapObjGroup)) {
    return 0;
  }

  if (mtx) {
    *mtx = mapObjDef->mat;
  }
  if (invMtx) {
    *invMtx = mapObjDef->invMat;
  }
  return 1;
}

void CMapEntity::UpdateMapObjLiquid() {
  flagInLiquid = 0;
  flagDeepLiquid = 0;

  CMapObjDef      *mapObjDef;
  CMapObj         *mapObj;
  CMapObjDefGroup *mapObjDefGroup;
  CMapObjGroup    *mapObjGroup;
  if (!GetMapObjAndGroup(mapObjDef, mapObj, mapObjDefGroup, mapObjGroup)) {
    return;
  }

  NTempest::C3Vector localPos = pos * mapObjDef->invMat;
  NTempest::C3Vector direction(0.0f, 0.0f, 0.0f);
  float              surface;
  if (!mapObjGroup->QueryLiquidStatus(localPos, lqWhich, surface, direction) || lqWhich == 15) {
    return;
  }

  flagInLiquid = 1;
  NTempest::C3Vector worldPt(localPos.x, localPos.y, surface);
  worldPt = worldPt * mapObjDef->mat;
  lqSurface = worldPt.z;

  lqDirection.x = direction.x * mapObjDef->mat.a0 + direction.y * mapObjDef->mat.b0 + direction.z * mapObjDef->mat.c0;
  lqDirection.y = direction.x * mapObjDef->mat.a1 + direction.y * mapObjDef->mat.b1 + direction.z * mapObjDef->mat.c1;
  lqDirection.z = direction.x * mapObjDef->mat.a2 + direction.y * mapObjDef->mat.b2 + direction.z * mapObjDef->mat.c2;
}

float ComputeFogBlend(SMOFog &fog, float dist) {
  FATALASSERT(dist < fog.end);
  if (dist >= fog.start) {
    return 1.0f - (dist - fog.start) / (fog.end - fog.start);
  }
  return 1.0f;
}

int CMapEntity::QueryMapObjFog(SMOFog::Fogs &oFog, float &oPct) {
  CMapObjDef      *mapObjDef;
  CMapObj         *mapObj;
  CMapObjDefGroup *mapObjDefGroup;
  CMapObjGroup    *mapObjGroup;
  if (!GetMapObjAndGroup(mapObjDef, mapObj, mapObjDefGroup, mapObjGroup) || mapObj->fogCount <= 1) {
    return 0;
  }

  NTempest::C3Vector localPos = pos * mapObjDef->invMat;
  oFog = mapObj->GetFog(0).fogs;

  static NTempest::CPriorityQ<FogQ, FogQ> fogq;
  unsigned int                            ieBlendFogId = 0;
  float                                   ieDist = 0.0f;
  for (int i = 0; i < 4; ++i) {
    unsigned int fogId = mapObjGroup->fogIds[i];
    if (!fogId) {
      continue;
    }

    SMOFog            &fog = mapObj->GetFog(fogId);
    NTempest::C3Vector delta = fog.pos - localPos;
    float              dist = delta.Mag();
    if (dist < fog.end) {
      if (fog.flags & 1) {
        ieBlendFogId = fogId;
        ieDist = dist;
      } else {
        fogq.Enqueue(FogQ(dist, fogId));
      }
    }
  }

  while (fogq.HasEntries()) {
    FogQ    entry = fogq.Dequeue();
    SMOFog &fog = mapObj->GetFog(entry.fogId);
    oFog.Blend(fog.fogs, ComputeFogBlend(fog, entry.dist));
  }

  if (ieBlendFogId) {
    SMOFog &fog = mapObj->GetFog(ieBlendFogId);
    oPct = 1.0f - ComputeFogBlend(fog, ieDist);
  } else {
    oPct = 1.0f;
  }
  return 1;
}

int CMapEntity::QueryCameraFog(SMOFog::Fogs &oFog, float &oPct) {
  CMapObjDef   *mapObjDef = CWorldScene::camMapObjDef;
  CMapObj      *mapObj = CWorldScene::camMapObj;
  CMapObjGroup *mapObjGroup = CWorldScene::camMapObjGroup;
  if (!mapObjDef || !mapObj || !mapObjGroup || (mapObjGroup->flags & 0x40) || mapObjGroup->groupLiquid != 15 || mapObj->fogCount == 1) {
    return 0;
  }

  NTempest::C3Vector localPos = CWorldScene::camPos * mapObjDef->invMat;
  oFog = mapObj->GetFog(0).fogs;

  static NTempest::CPriorityQ<FogQ, FogQ> fogq;
  unsigned int                            ieBlendFogId = 0;
  float                                   ieDist = 0.0f;
  for (int i = 0; i < 4; ++i) {
    unsigned int fogId = mapObjGroup->fogIds[i];
    if (!fogId) {
      continue;
    }

    SMOFog            &fog = mapObj->GetFog(fogId);
    NTempest::C3Vector delta = fog.pos - localPos;
    float              dist = delta.Mag();
    if (dist < fog.end) {
      if (fog.flags & 1) {
        ieBlendFogId = fogId;
        ieDist = dist;
      } else {
        fogq.Enqueue(FogQ(dist, fogId));
      }
    }
  }

  while (fogq.HasEntries()) {
    FogQ    entry = fogq.Dequeue();
    SMOFog &fog = mapObj->GetFog(entry.fogId);
    oFog.Blend(fog.fogs, ComputeFogBlend(fog, entry.dist));
  }

  if (ieBlendFogId) {
    SMOFog &fog = mapObj->GetFog(ieBlendFogId);
    oPct = 1.0f - ComputeFogBlend(fog, ieDist);
  } else {
    oPct = 1.0f;
  }
  return 1;
}

void __fastcall CMap::UpdateEntity(CMapEntity *entity) {
  FATALASSERT(entity);

  while (entity->parentLinkList.Head()) {
    FreeBaseObjLink(entity->parentLinkList.Head());
  }

  entity->flags = (entity->flags & ~0x19) | CMapBaseObj::Flag_LightUpdate;
  entity->flagInside = 0;
  entity->flagShadowed = 0;
  entity->flagInLiquid = 0;
  entity->flagDeepLiquid = 0;

  LinkEntity(entity);

  int deep;
  if (entity->flagInside) {
    entity->UpdateMapObjLiquid();
  } else if (QueryLiquidStatus(entity->pos, entity->lqWhich, entity->lqSurface, entity->lqDirection, deep)) {
    entity->flagInLiquid = 1;
    if (deep) {
      entity->flagDeepLiquid = 1;
    }
  }

  if (!(entity->flags & CMapBaseObj::Flag_InteriorLit)) {
    entity->ambientTarget = CMap::sunLight->gxLight.m_ambColor;
    if (QueryShadow(entity->pos)) {
      entity->dirLightScaleTarget = CMapStaticEntity::dirLightScaleAmount;
      entity->flagShadowed = 1;
      return;
    }
  }
  entity->dirLightScaleTarget = 1.0f;
}

void __fastcall CMap::LinkEntityToMapObj(CMapStaticEntity *entity, CMapObjDef *mapObjDef, CMapObjDefGroup *mapObjDefGroup) {
  CMapBaseObjLink *link = AllocBaseObjLink(entity);
  link->ref = mapObjDefGroup;
  mapObjDefGroup->entityLinkList.LinkNode(link, LIST_TAIL, 0);

  FATALASSERT(mapObjDef->mapObj);
  CMapObjGroup *mapObjGroup = mapObjDef->mapObj->GetGroup(mapObjDefGroup->groupNum, 0);
  FATALASSERT(mapObjGroup);

  if (mapObjGroup->flags & 0x8) {
    entity->flags |= CMapBaseObj::Flag_ExteriorLit;
  } else {
    entity->flagInside = 1;
    if (mapObjGroup->flags & 0x40) {
      entity->flags |= CMapBaseObj::Flag_ExteriorLit;
    } else {
      entity->flags |= CMapBaseObj::Flag_InteriorLit;
      entity->QueryLightmap(mapObjDef, mapObjGroup);
    }
  }
}

void __fastcall CMap::LinkEntityToChunk(CMapStaticEntity *entity, CMapChunk *chunk) {
  CMapBaseObjLink *link = AllocBaseObjLink(entity);
  link->ref = chunk;
  chunk->entityLinkList.LinkNode(link, LIST_TAIL, 0);
  entity->flags |= CMapBaseObj::Flag_ExteriorLit;
}

void __fastcall CMap::LinkEntity(CMapStaticEntity *entity) {
  FATALASSERT(entity);

  NTempest::C3Vector lCen = entity->pos;
  lCen.z += 1.5f;
  NTempest::C3Vector lEnd = lCen;
  lEnd.z -= 1760.0f;

  CMapChunk   *chunk;
  float        chunkT = 1.0f;
  unsigned int hitChunk = VectorIntersectTerrain(&lCen, &lEnd, &chunkT, 0, &chunk);

  CMapObjDef      *mapObjDef;
  CMapObjDefGroup *mapObjDefGroup;
  float            mapObjT = 1.0f;
  unsigned int     hitMapObj = LinkIntersectMapObjs(lCen, lEnd, mapObjT, mapObjDef, mapObjDefGroup);

  if ((!hitChunk || (hitMapObj && chunkT >= mapObjT)) && hitMapObj) {
    LinkEntityToMapObj(entity, mapObjDef, mapObjDefGroup);
  } else if (hitChunk) {
    LinkEntityToChunk(entity, chunk);
  }
}

unsigned int __fastcall CMap::LinkIntersectMapObjs(
    NTempest::C3Vector &lCen,
    NTempest::C3Vector &lEnd,
    float              &hitT,
    CMapObjDef        *&hitMapObjDef,
    CMapObjDefGroup   *&hitMapObjDefGroup
) {
  hitMapObjDef = 0;
  hitMapObjDefGroup = 0;
  hitT = 1.0f;

  CMapObjDef *mapObjDef = mapObjDefHash.Head();
  while (mapObjDef) {
    if (CWorldMath::VectorIntersectAABox2(mapObjDef->aaBox, lCen, lEnd)) {
      CMapObj *mapObj = mapObjDef->mapObj;
      if (mapObj) {
        NTempest::C3Vector  v0 = lCen * mapObjDef->invMat;
        NTempest::C3Vector  v1 = lEnd * mapObjDef->invMat;
        NTempest::C3Segment seg(v0, v1);

        CMapBaseObjLink *link = mapObjDef->groupLinkList.Head();
        while (reinterpret_cast<long>(link) > 0) {
          CMapObjDefGroup *mapObjDefGroup = static_cast<CMapObjDefGroup *>(link->owner);
          if (mapObj->TestGroupBounds(seg.start, seg.end, mapObjDefGroup->groupNum)) {
            CMapObjGroup *mapObjGroup = mapObj->GetGroup(mapObjDefGroup->groupNum, 0);
            if (mapObjGroup) {
              CWTriData triData;
              float     thisT = hitT;
              if (mapObjGroup->GetTris(triData, seg, thisT, mapObjDef, 0) && thisT < hitT) {
                hitT = thisT;
                hitMapObjDef = mapObjDef;
                hitMapObjDefGroup = mapObjDefGroup;
              }
            }
          }
          link = mapObjDef->groupLinkList.RawNext(link);
        }
      }
    }
    mapObjDef = mapObjDefHash.Next(mapObjDef);
  }

  return hitMapObjDef != 0;
}

void CMapEntity::QueryLightmap(CMapObjDef *mapObjDef, CMapObjGroup *mapObjGroup) {
  NTempest::C3Vector start = pos;
  start.z += 2.0f / 3.0f;
  start = start * mapObjDef->invMat;

  NTempest::C3Vector end = pos;
  end.z -= 4.0f / 3.0f;
  end = end * mapObjDef->invMat;

  NTempest::CImVector lmColor(0);
  if (mapObjGroup->QueryLightmap(NTempest::C3Segment(start, end), lmColor)) {
    AdjustLightmap(lmColor, interiorDirColor, 168, ambientTarget, 96);
  }
}

void CMapEntity::Tick() {
  int amount = static_cast<int>(ambLightScaleRate * CWorld::GetTickTimeSec() * 255.0f);
  if (amount < 1) {
    amount = 1;
  }

  int ambRgb[3] = {ambient.r, ambient.g, ambient.b};
  int ambRgbT[3] = {ambientTarget.r, ambientTarget.g, ambientTarget.b};
  int ambDiff[3] = {ambRgbT[0] - ambRgb[0], ambRgbT[1] - ambRgb[1], ambRgbT[2] - ambRgb[2]};
  int ambUpdated = 0;
  for (unsigned int i = 0; i < 3; ++i) {
    if (ambDiff[i]) {
      ambUpdated = 1;
      if (ambDiff[i] > 0) {
        ambRgb[i] += amount;
        if (ambRgb[i] > ambRgbT[i]) {
          ambRgb[i] = ambRgbT[i];
        }
      } else {
        ambRgb[i] -= amount;
        if (ambRgb[i] < ambRgbT[i]) {
          ambRgb[i] = ambRgbT[i];
        }
      }
    }
  }

  ambient.r = ambRgb[0];
  ambient.g = ambRgb[1];
  ambient.b = ambRgb[2];

  if (!(flags & Flag_LightUpdate) && !ambUpdated) {
    ambient = DayNightGetInfo()->lightInfo.ambColor;
    ambientTarget = DayNightGetInfo()->lightInfo.ambColor;
  }

  float amountF = dirLightScaleRate * CWorld::GetTickTimeSec();
  float dirDiff = dirLightScale - dirLightScaleTarget;
  if (dirDiff != 0.0f) {
    if (dirDiff > 0.0f) {
      dirLightScale -= amountF;
      if (dirLightScale < dirLightScaleTarget) {
        dirLightScale = dirLightScaleTarget;
      }
    } else {
      dirLightScale += amountF;
      if (dirLightScale > dirLightScaleTarget) {
        dirLightScale = dirLightScaleTarget;
      }
    }
  }

  if (!parentLinkList.Head()) {
    CMap::UpdateEntity(this);
  }
}
