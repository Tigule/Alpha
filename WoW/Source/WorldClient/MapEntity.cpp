#include "Base/Base.h"
#include "Gx/Gx.h"
#include "Services/ParticleSystem2.h"
#include <WowConst.h>
#include "AaBsp.h"
#include <MapDefs.h>

#include "WorldClient/World.h"
#include "WorldClient/CMapObj.h"
#include "WorldClient/WorldParam.h"
#include "WorldClient/DetailDoodad.h"
#include "WorldClient/CSimpleDoodad.h"
#include "DayNight.h"

#include <DB/DBClient/DBClient.h>
#include <WorldCommon/WorldMath.h>

#include <Tempest/cpriorityq.h>

const float CMapEntity::ambLightScaleRate = 2.0f;
const float CMapEntity::dirLightScaleRate = 3.3333333f;

class FogQ {
 public:
  float dist;
  int   subscript;

  FogQ(float pDist, int pSubscript) : dist(pDist), subscript(pSubscript) {
  }

  static bool HasHigherPriority(const FogQ &a, const FogQ &b) {
    return a.dist >= b.dist;
  }
};

BOOL CMapStaticEntity::GetMapObjDef(CMapObjDef *&mapObjDef) {
  BOOL found = 0;
  ITERATELIST(CMapBaseObjLink, parentLinkList, parentLink) {
    if (parentLink->ref->GetType() & Type_MapObjDefGroup) {
      CMapObjDefGroup *mapObjDefGroup = static_cast<CMapObjDefGroup *>(parentLink->ref);
      mapObjDef = static_cast<CMapObjDef *>(mapObjDefGroup->parentLinkList.Head()->ref);
      FATALASSERT(mapObjDef->GetType() & Type_MapObjDef);
      found = 1;
      break;
    }
  }

  return found;
}

BOOL CMapStaticEntity::GetMapObjAndGroup(CMapObjDef *&mapObjDef, CMapObj *&mapObj, CMapObjDefGroup *&mapObjDefGroup, CMapObjGroup *&mapObjGroup) {
  BOOL found = 0;
  if (flagInside) {
    ITERATELIST(CMapBaseObjLink, parentLinkList, parentLink) {
      if (parentLink->ref->GetType() & Type_MapObjDefGroup) {
        mapObjDefGroup = static_cast<CMapObjDefGroup *>(parentLink->ref);
        mapObjDef = static_cast<CMapObjDef *>(mapObjDefGroup->parentLinkList.Head()->ref);
        FATALASSERT(mapObjDef->GetType() & Type_MapObjDef);

        mapObj = mapObjDef->mapObj;
        FATALASSERT(mapObj);
        mapObjGroup = mapObj->GetGroup(mapObjDefGroup->groupNum, 0);
        FATALASSERT(mapObjGroup);
        found = 1;
        break;
      }
    }
  }

  return found;
}

CMapEntity::CMapEntity() {
  type |= Type_Entity;
}

CMapEntity::~CMapEntity() {
  FATALASSERT(refCount==0);
}

BOOL CMapEntity::QueryMapObjZoneName(LPCSTR &zoneName) {
  CMapObjDefGroup *mapObjDefGroup;
  CMapObjGroup    *mapObjGroup;
  CMapObj         *mapObj;
  CMapObjDef      *mapObjDef;
  BOOL result = 0;
  if (GetMapObjAndGroup(mapObjDef, mapObj, mapObjDefGroup, mapObjGroup)) {
    if (!mapObjDef->zoneName) {
      mapObjDef->zoneName = SDBWMOAreaTableLookup(mapObj->GetWmoID(), mapObjDef->nameSet, -1);
    }

    zoneName = mapObjDef->zoneName;
    result = 1;
  }

  return result;
}

BOOL CMapEntity::QueryMapObjSubzoneName(LPCSTR &subzoneName, UINT &subzoneId) {
  CMapObjDef      *mapObjDef;
  CMapObj         *mapObj;
  CMapObjGroup    *mapObjGroup;
  CMapObjDefGroup *mapObjDefGroup;
  BOOL result = 0;
  if (GetMapObjAndGroup(mapObjDef, mapObj, mapObjDefGroup, mapObjGroup)) {
    if (!mapObjDefGroup->subzoneName) {
      mapObjDefGroup->subzoneName = SDBWMOAreaTableLookup(mapObj->GetWmoID(), mapObjDef->nameSet, mapObjGroup->GetUniqueID());
    }

    subzoneName = mapObjDefGroup->subzoneName;
    subzoneId = mapObjDefGroup->groupNum;
    result = 1;
  }

  return result;
}

bool CMapEntity::QueryMapObjAreaTable(const WMOAreaTableRec *&subzoneRec, const WMOAreaTableRec *&globalRec) {
  CMapObjGroup    *mapObjGroup;
  CMapObjDefGroup *mapObjDefGroup;
  CMapObjDef      *mapObjDef;
  CMapObj         *mapObj;
  bool result = false;
  if (GetMapObjAndGroup(mapObjDef, mapObj, mapObjDefGroup, mapObjGroup)) {
    result = SDBWMOAreaTableLookup(mapObj->GetWmoID(), mapObjDef->nameSet, mapObjDefGroup->groupNum, subzoneRec) &&
             SDBWMOAreaTableLookup(mapObj->GetWmoID(), mapObjDef->nameSet, -1, globalRec);
  }

  return result;
}

BOOL CMapEntity::QueryMapObjFileName(LPCSTR &fileName) {
  CMapObjDef      *mapObjDef;
  CMapObjDefGroup *mapObjDefGroup;
  CMapObjGroup    *mapObjGroup;
  CMapObj         *mapObj;
  BOOL result = 0;
  if (GetMapObjAndGroup(mapObjDef, mapObj, mapObjDefGroup, mapObjGroup)) {
    fileName = mapObj->name;
    result = 1;
  }

  return result;
}

BOOL CMapEntity::QueryMapObjListenerId(UINT &listenerId) {
  CMapObjGroup    *mapObjGroup;
  CMapObjDefGroup *mapObjDefGroup;
  CMapObj         *mapObj;
  CMapObjDef      *mapObjDef;
  BOOL result = 0;
  if (GetMapObjAndGroup(mapObjDef, mapObj, mapObjDefGroup, mapObjGroup)) {
    listenerId = 0;
    result = 1;
  }

  return result;
}

bool CMapEntity::QueryMapObjMinimap(const NTempest::CAaBox &aaBox, TSStackArray<CWorld::MinimapQuad> &quads) {
  CMapObjGroup    *mapObjGroup;
  CMapObj         *mapObj;
  CMapObjDefGroup *mapObjDefGroup;
  CMapObjDef      *mapObjDef;
  bool result = false;
  if (GetMapObjAndGroup(mapObjDef, mapObj, mapObjDefGroup, mapObjGroup)) {
    NTempest::CAaBox localBox;
    CWorldMath::TransformAABox(mapObjDef->invMat, aaBox, localBox);
    result = mapObj->QueryMapObjMinimap(mapObjDefGroup->groupNum, localBox, quads);
  }

  return result;
}

bool CMapEntity::QueryMapObjIDs(UINT &wmoID, UINT &instanceID, UINT &groupID) {
  CMapObjGroup    *mapObjGroup;
  CMapObjDefGroup *mapObjDefGroup;
  CMapObjDef      *mapObjDef;
  CMapObj         *mapObj;
  bool result = false;
  if (GetMapObjAndGroup(mapObjDef, mapObj, mapObjDefGroup, mapObjGroup)) {
    wmoID = mapObj->GetWmoID();
    instanceID = mapObjDef->mapObj->GetHashValue();
    groupID = mapObjDefGroup->groupNum;
    result = true;
  }

  return result;
}

bool CMapEntity::QueryMapObjMatrix(NTempest::C44Matrix *mtx, NTempest::C44Matrix *invMtx) {
  CMapObj         *mapObj;
  CMapObjDefGroup *mapObjDefGroup;
  CMapObjGroup    *mapObjGroup;
  CMapObjDef      *mapObjDef;
  bool result = false;
  if (GetMapObjAndGroup(mapObjDef, mapObj, mapObjDefGroup, mapObjGroup)) {
    if (mtx) {
      *mtx = mapObjDef->mat;
    }
    if (invMtx) {
      *invMtx = mapObjDef->invMat;
    }
    result = true;
  }

  return result;
}

void CMapEntity::UpdateMapObjLiquid() {
  flagInLiquid = 0;
  flagDeepLiquid = 0;

  CMapObjDefGroup *mapObjDefGroup;
  CMapObjGroup    *mapObjGroup;
  CMapObjDef      *mapObjDef;
  CMapObj         *mapObj;
  if (GetMapObjAndGroup(mapObjDef, mapObj, mapObjDefGroup, mapObjGroup)) {
    NTempest::C3Vector localPos = pos * mapObjDef->invMat;
    NTempest::C3Vector direction(0.0f, 0.0f, 0.0f);
    float              surface;
    if (mapObjGroup->QueryLiquidStatus(localPos, lqWhich, surface, direction) && lqWhich != 15) {
      flagInLiquid = 1;
      NTempest::C3Vector worldPt = NTempest::C3Vector(localPos.x, localPos.y, surface) * mapObjDef->mat;
      lqSurface = worldPt.z;
      lqDirection = NTempest::C3Vector(
          direction.z * mapObjDef->mat.c0 + direction.y * mapObjDef->mat.b0 + direction.x * mapObjDef->mat.a0,
          direction.z * mapObjDef->mat.c1 + direction.y * mapObjDef->mat.b1 + direction.x * mapObjDef->mat.a1,
          direction.z * mapObjDef->mat.c2 + direction.y * mapObjDef->mat.b2 + direction.x * mapObjDef->mat.a2
      );
    }
  }
}

void CMapEntity::QueryLiquidSounds(int *lbool, NTempest::C3Vector *ldelta, float *ldsquared, UINT &closestExtLevel) {
  if (!flagInside) {
    return;
  }

  ITERATELIST(CMapBaseObjLink, parentLinkList, parentLink) {
    if (parentLink->ref->GetType() & Type_MapObjDefGroup) {
      CMapObjDefGroup *mapObjDefGroup = static_cast<CMapObjDefGroup *>(parentLink->ref);
      CMapObjDef      *mapObjDef = static_cast<CMapObjDef *>(mapObjDefGroup->parentLinkList.Head()->ref);
      FATALASSERT(mapObjDef->GetType() & Type_MapObjDef);
      CMapObj *mapObj = mapObjDef->mapObj;
      FATALASSERT(mapObj);

      NTempest::C3Vector localPos = pos * mapObjDef->invMat;
      mapObj->QueryLiquidSounds(mapObjDefGroup->groupNum, mapObjDefGroup->groupNum, 0, closestExtLevel, localPos, lbool, ldelta, ldsquared);
    }
  }
}

static float ComputeFogBlend(const SMOFog &fog, float dist) {
  FATALASSERT(dist < fog.end);
  if (dist < fog.start) {
    return 1.0f;
  }
  return 1.0f - (dist - fog.start) / (fog.end - fog.start);
}

BOOL CMapEntity::QueryMapObjFog(SMOFog::Fogs &oFog, float &oPct) {
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
  BYTE                                    ieBlendFogId = 0;
  float                                   ieDist = 0.0f;
  for (int i = 0; i < 4; ++i) {
    BYTE fogId = mapObjGroup->GetFogId(i);
    if (!fogId) {
      continue;
    }

    const SMOFog      &fog = mapObj->GetFog(fogId);
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
    FogQ          entry = fogq.Dequeue();
    const SMOFog &fog = mapObj->GetFog(entry.subscript);
    oFog.Blend(fog.fogs, ComputeFogBlend(fog, entry.dist));
  }

  if (ieBlendFogId) {
    const SMOFog &fog = mapObj->GetFog(ieBlendFogId);
    oPct = 1.0f - ComputeFogBlend(fog, ieDist);
  } else {
    oPct = 1.0f;
  }
  return 1;
}

BOOL CMapEntity::QueryCameraFog(SMOFog::Fogs &oFog, float &oPct) {
  CMapObjDef   *mapObjDef = CWorldScene::camMapObjDef;
  CMapObj      *mapObj = CWorldScene::camMapObj;
  CMapObjGroup *mapObjGroup = CWorldScene::camMapObjGroup;
  if (!mapObjDef || !mapObj || !mapObjGroup || (mapObjGroup->GetFlags() & 0x40) || mapObjGroup->GetGroupLiquid() != 15 || mapObj->fogCount == 1) {
    return 0;
  }

  NTempest::C3Vector localPos = CWorldScene::camPos * mapObjDef->invMat;
  oFog = mapObj->GetFog(0).fogs;

  static NTempest::CPriorityQ<FogQ, FogQ> fogq;
  BYTE                                    ieBlendFogId = 0;
  float                                   ieDist = 0.0f;
  for (int i = 0; i < 4; ++i) {
    BYTE fogId = mapObjGroup->GetFogId(i);
    if (!fogId) {
      continue;
    }

    const SMOFog      &fog = mapObj->GetFog(fogId);
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
    FogQ          entry = fogq.Dequeue();
    const SMOFog &fog = mapObj->GetFog(entry.subscript);
    oFog.Blend(fog.fogs, ComputeFogBlend(fog, entry.dist));
  }

  if (ieBlendFogId) {
    const SMOFog &fog = mapObj->GetFog(ieBlendFogId);
    oPct = 1.0f - ComputeFogBlend(fog, ieDist);
  } else {
    oPct = 1.0f;
  }
  return 1;
}

void CMap::UpdateEntity(CMapEntity *entity) {
  FATALASSERT(entity);

  for (CMapBaseObjLink *parentLink = entity->parentLinkList.Head(), *next;
       (int)parentLink > 0 ? ((next = entity->parentLinkList.RawNext(parentLink)), 1) : 0; parentLink = next) {
    FreeBaseObjLink(parentLink);
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

void CMap::LinkEntityToMapObj(CMapStaticEntity *entity, CMapObjDef *mapObjDef, CMapObjDefGroup *mapObjDefGroup) {
  CMapBaseObjLink *link = AllocBaseObjLink(entity);
  link->ref = mapObjDefGroup;
  if (entity->GetType() & CMapBaseObj::Type_Entity) {
    mapObjDefGroup->entityLinkList.LinkNode(link, LIST_TAIL, 0);
  } else if (entity->GetType() & CMapBaseObj::Type_DoodadDef) {
    mapObjDefGroup->doodadDefLinkList.LinkNode(link, LIST_TAIL, 0);
  } else {
    FATALASSERT(0);
  }

  CMapObj *hitMapObj = mapObjDef->mapObj;
  FATALASSERT(hitMapObj);
  CMapObjGroup *hitMapObjGroup = hitMapObj->GetGroup(mapObjDefGroup->groupNum, 0);
  FATALASSERT(hitMapObjGroup);

  UINT flags = hitMapObjGroup->flags;
  if (flags & 0x8) {
    entity->flags |= CMapBaseObj::Flag_ExteriorLit;
  } else {
    entity->flagInside = 1;
    if (flags & 0x40) {
      entity->flags |= CMapBaseObj::Flag_ExteriorLit;
    } else {
      entity->flags |= CMapBaseObj::Flag_InteriorLit;
      entity->QueryLightmap(mapObjDef, hitMapObjGroup);
    }
  }
}

void CMap::LinkEntityToChunk(CMapStaticEntity *entity, CMapChunk *chunk) {
  CMapBaseObjLink *link = AllocBaseObjLink(entity);
  link->ref = chunk;
  if (entity->GetType() & CMapBaseObj::Type_Entity) {
    chunk->entityLinkList.LinkNode(link, LIST_TAIL, 0);
  } else if (entity->GetType() & CMapBaseObj::Type_DoodadDef) {
    chunk->doodadDefLinkList.LinkNode(link, LIST_TAIL, 0);
  } else {
    FATALASSERT(0);
  }
  entity->flags |= CMapBaseObj::Flag_ExteriorLit;
}

void CMap::LinkEntity(CMapStaticEntity *entity) {
  FATALASSERT(entity);

  NTempest::C3Vector lCen = entity->pos;
  lCen.z += 1.5f;
  NTempest::C3Vector lEnd = lCen;
  lEnd.z -= 1760.0f;

  CMapChunk       *chunk;
  float            chunkT = 1.0f;
  float            mapObjT = 1.0f;
  UINT             hitChunk = VectorIntersectTerrain(&lCen, &lEnd, &chunkT, 0, &chunk);
  CMapObjDef      *mapObjDef;
  CMapObjDefGroup *mapObjDefGroup;
  UINT             hitMapObj = LinkIntersectMapObjs(lCen, lEnd, mapObjT, mapObjDef, mapObjDefGroup);

  if ((!hitChunk || (hitMapObj && !(chunkT < mapObjT))) && hitMapObj) {
    LinkEntityToMapObj(entity, mapObjDef, mapObjDefGroup);
  } else if (hitChunk) {
    LinkEntityToChunk(entity, chunk);
  }
}

bool CMap::LinkIntersectMapObjs(
    const NTempest::C3Vector &lCen,
    const NTempest::C3Vector &lEnd,
    float                    &hitT,
    CMapObjDef              *&hitMapObjDef,
    CMapObjDefGroup         *&hitMapObjDefGroup
) {
  hitMapObjDef = 0;
  hitMapObjDefGroup = 0;
  hitT = 1.0f;

  ITERATELIST(CMapObjDef, mapObjDefHash, mapObjDef) {
    if (mapObjDef->TestAABox(lCen, lEnd)) {
      CMapObj *mapObj = mapObjDef->mapObj;
      if (mapObj) {
        NTempest::C3Vector v0 = lCen * mapObjDef->invMat;
        NTempest::C3Vector v1 = lEnd * mapObjDef->invMat;

        ITERATELIST(CMapBaseObjLink, mapObjDef->groupLinkList, link) {
          CMapObjDefGroup *mapObjDefGroup = static_cast<CMapObjDefGroup *>(link->owner);
          if (mapObj->TestGroupBounds(v0, v1, mapObjDefGroup->groupNum)) {
            CMapObjGroup *mapObjGroup = mapObj->GetGroup(mapObjDefGroup->groupNum, 0);
            if (mapObjGroup) {
              CWTriData triData;
              float     thisT = hitT;
              if (mapObjGroup->GetTris(triData, NTempest::C3Segment(v0, v1), thisT, mapObjDef, 0) && thisT < hitT) {
                hitT = thisT;
                hitMapObjDef = mapObjDef;
                hitMapObjDefGroup = mapObjDefGroup;
              }
            }
          }
        }
      }
    }
  }

  return hitMapObjDef != 0;
}

void CMapEntity::QueryLightmap(CMapObjDef *mapObjDef, CMapObjGroup *mapObjGroup) {
  NTempest::CImVector lmColor(0ul);
  NTempest::C3Vector  start = NTempest::C3Vector(pos.x, pos.y, pos.z + 2.0f / 3.0f) * mapObjDef->invMat;
  NTempest::C3Vector  end = NTempest::C3Vector(pos.x, pos.y, pos.z - 4.0f / 3.0f) * mapObjDef->invMat;
  if (mapObjGroup->QueryLightmap(NTempest::C3Segment(start, end), lmColor)) {
    AdjustLightmap(lmColor, interiorDirColor, 168, ambientTarget, 96);
  }
}

void CMapEntity::Tick() {
  int amount = static_cast<int>(ambLightScaleRate * CWorld::GetTickTimeSec() * 255.0f);
  if (amount < 1) {
    amount = 1;
  }

  int ambUpdated = 0;
  int ambRgb[3] = {ambient.r, ambient.g, ambient.b};
  int ambRgbT[3] = {ambientTarget.r, ambientTarget.g, ambientTarget.b};
  int ambDiff[3] = {ambRgbT[0] - ambRgb[0], ambRgbT[1] - ambRgb[1], ambRgbT[2] - ambRgb[2]};
  for (UINT i = 0; i < 3; ++i) {
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

  if (!flagInside && !ambUpdated) {
    DNInfo *dnInfo = DayNightGetInfo();
    ambientTarget = ambient = dnInfo->lightInfo.ambColor;
  }

  float amountF = dirLightScaleRate * CWorld::GetTickTimeSec();
  float dirDiff = dirLightScale - dirLightScaleTarget;
  if (dirDiff != 0.0f) {
    if (dirDiff < 0.0f) {
      dirLightScale += amountF;
      if (dirLightScale > dirLightScaleTarget) {
        dirLightScale = dirLightScaleTarget;
      }
    } else {
      dirLightScale -= amountF;
      if (dirLightScale < dirLightScaleTarget) {
        dirLightScale = dirLightScaleTarget;
      }
    }
  }

  if (!parentLinkList.Head()) {
    CMap::UpdateEntity(this);
  }
}
