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

#include <Model/IModel.h>
#include <Tempest/cmath.h>

static float       lastUpdateTime;
static const float OO_COORD_TO_SUBCHUNK = 1.0f / (150.0f / 36.0f);
static const float Gx_MinTexAspect = 0.125f;

void CMap::SnapBaseObjToSubChunk(CMapBaseObj *baseObj, NTempest::C3Vector &pos, float angle) {
  NTempest::C44Matrix mat;
  mat.Rotate(angle, NTempest::C3Vector(0.0f, 0.0f, 1.0f), 1);

  NTempest::CAaBox tAaBox;
  if (baseObj->GetType() & CMapBaseObj::Type_MapObjDef) {
    CMapObjDef *mapObjDef = (CMapObjDef *)baseObj;
    mapObjDef->mapObj->GetBounds(tAaBox);
  } else {
    CMapStaticEntity *entity = (CMapStaticEntity *)baseObj;
    ModelGetExtents(entity->model, &tAaBox);
  }

  NTempest::C3Vector cen = (tAaBox.b + tAaBox.t) * 0.5f;
  tAaBox.t -= cen;
  tAaBox.b -= cen;

  NTempest::CAaBox aaBox;
  CWorldMath::TransformAABox(mat, tAaBox, aaBox);

  NTempest::C3Vector tVec = aaBox.b + pos;
  int                x = Fast_ftol((tVec.x + 150.0f / 36.0f) * OO_COORD_TO_SUBCHUNK);
  int                y = Fast_ftol((tVec.y + 150.0f / 36.0f) * OO_COORD_TO_SUBCHUNK);
  pos.x -= tVec.x - x * (150.0f / 36.0f);
  pos.y -= tVec.y - y * (150.0f / 36.0f);
}

void CMap::Update() {
  NTempest::CiRect areaRect = CWorld::areaRect;

  for (long y = areaRect.miny; y <= areaRect.maxy; ++y) {
    for (long x = areaRect.minx; x <= areaRect.maxx; ++x) {
      CMapArea *area = areaTable[x + 64 * y];
      if (area) {
        UpdateChunks(area);
      }
    }
  }

  UpdateMapObjDefs();

  ITERATELIST(CMapEntity, entityList, entity) {
    entity->flagVisible = 0;
    if (!entity->flagInside) {
      CWorldScene::AddMapEntity(entity);
    }
  }

  if (skyTexid && CWorld::curTimeSec - lastUpdateTime > 2.0f) {
    lastUpdateTime = CWorld::curTimeSec;
    GxTexUpdate(skyTexid, 0, 0, (UINT)(Gx_MinTexAspect * SKYTEX_HEIGHT), SKYTEX_HEIGHT, 0);
  }

  UpdateLiquidTextures();

  SAFEITERATELIST(WaterRadWave, waterRipplesActive, wave) {
    if (!wave->Update(CWorld::tickTimeSec)) {
      waterRipplesFree.LinkNode(wave, LIST_TAIL, 0);
    }
  }
}

void CMap::UpdateDoodadDef(CMapDoodadDef *doodadDef, NTempest::C3Vector &pos, float angle) {
  FATALASSERT(doodadDef);

  doodadDef->flags = CMapBaseObj::Flag_LightUpdate;
  doodadDef->pos = pos;
  doodadDef->scale = 1.0f;
  doodadDef->mat.Identity();
  doodadDef->mat.Translate(pos);
  doodadDef->mat.Rotate(angle, NTempest::C3Vector(0.0f, 0.0f, 1.0f), 1);
  doodadDef->lMat.Identity();

  if (ModelIsLoaded(doodadDef->model, 1)) {
    InitializeDoodadBounds(doodadDef);
  } else {
    doodadDef->aaSphere.c = doodadDef->pos;
    doodadDef->aaSphere.r = 0.0f;
    doodadDef->aaBox.b = doodadDef->pos;
    doodadDef->aaBox.t = doodadDef->pos;
  }
}

void CMap::UpdateMapObjDefs() {
  ITERATELIST(CMapObjDef, mapObjDefHash, mapObjDef) {
    if ((mapObjDef->flags & CMapBaseObj::Flag_Loaded) && mapObjDef->aaBox.b.x <= CWorldScene::camFrustumBounds.t.x &&
        mapObjDef->aaBox.b.y <= CWorldScene::camFrustumBounds.t.y && mapObjDef->aaBox.b.z <= CWorldScene::camFrustumBounds.t.z &&
        mapObjDef->aaBox.t.x >= CWorldScene::camFrustumBounds.b.x && mapObjDef->aaBox.t.y >= CWorldScene::camFrustumBounds.b.y &&
        mapObjDef->aaBox.t.z >= CWorldScene::camFrustumBounds.b.z)
    {
      CWorldScene::AddMapObjDef(mapObjDef);
    }
  }
}

void CMap::UpdateMapObjDef(CMapObjDef *mapObjDef, NTempest::C3Vector &pos, float angle) {
  CMapObjGroup    *mapObjGroup;
  SMOLight        *sLight;
  UINT             i;
  CMapObjDefGroup *mapObjDefGroup;

  FATALASSERT(mapObjDef);
  mapObjDef->pos = pos;
  mapObjDef->mat.Identity();
  mapObjDef->mat.Translate(pos);
  mapObjDef->mat.Rotate(angle, NTempest::C3Vector(0.0f, 0.0f, 1.0f), 1);
  mapObjDef->invMat = mapObjDef->mat.AffineInverse();

  CMapObj *mapObj = mapObjDef->mapObj;
  FATALASSERT(mapObj);
  if (!mapObj->IsLoaded()) {
    mapObjDef->aaSphere.c = mapObjDef->pos;
    mapObjDef->aaSphere.r = 0.0f;
    mapObjDef->aaBox.t = mapObjDef->pos;
    mapObjDef->aaBox.b = mapObjDef->pos;
    return;
  }

  NTempest::CAaBox aaBox;
  mapObj->GetBounds(mapObjDef->aaSphere);
  mapObjDef->aaSphere.c *= mapObjDef->mat;
  mapObj->GetBounds(aaBox);
  CWorldMath::TransformAABox(mapObjDef->mat, aaBox, mapObjDef->aaBox);

  ITERATELIST(CMapBaseObjLink, mapObjDef->groupLinkList, groupLink) {
    mapObjDefGroup = (CMapObjDefGroup *)groupLink->owner;
    FATALASSERT(mapObjDefGroup);
    mapObjGroup = mapObj->GetGroup(mapObjDefGroup->groupNum, 0);
    if (mapObjGroup) {
      for (i = 0; i < mapObjGroup->lightRefCount; ++i) {
        UINT lightRef = mapObjGroup->lightRefList[i];
        sLight = &mapObj->lightList[lightRef];
        CMapLight *light = mapObjDef->lightList[lightRef];
        if (light) {
          light->gxLight.m_dir = sLight->position * mapObjDef->mat;
          if (!(mapObjDefGroup->flags & CMapBaseObj::Flag_InteriorLit)) {
            UpdateLight(light);
          }
        }
      }
      mapObjDefGroup->Update(mapObjDef->mat);
    }
  }
}

void CMap::UpdateChunks(CMapArea *area) {
  FATALASSERT(area);

  UINT corner = 0;
  if (CWorldScene::camTarg.x > CWorldScene::camPos.x) {
    corner = 2;
  }
  if (CWorldScene::camTarg.y > CWorldScene::camPos.y) {
    ++corner;
  }

  CMapChunk::farCornerIndex = 0;
  if (CWorldScene::camTarg.x < CWorldScene::camPos.x) {
    CMapChunk::farCornerIndex = 2;
  }
  if (CWorldScene::camTarg.y < CWorldScene::camPos.y) {
    ++CMapChunk::farCornerIndex;
  }

  ITERATELIST(CMapBaseObjLink, area->chunkLinkList, link) {
    CMapChunk          *chunk = (CMapChunk *)link->owner;
    chunk->camDist = CWorldScene::camPlaneXY.DistSigned(chunk->vertexList[CMapChunk::cornerVertexIndex[corner]] + chunk->corner);
    chunk->lod = CWorld::lodMax;
    if ((CWorld::enables & CWorld::Enable_Lod) && chunk->camDist > CWorld::lodDist) {
      chunk->lod = CWorld::lodMin;
    }
    chunk->Update();
  }
}
