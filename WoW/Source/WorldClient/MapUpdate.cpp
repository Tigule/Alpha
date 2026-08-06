#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

#include "WorldClient/World.h"

#include "WorldClient/CMapObj.h"
#include "WorldCommon/WorldMath.h"

#include <Model/IModel.h>
#include <Tempest/cmath.h>

static float       lastUpdateTime;
static const float OO_COORD_TO_SUBCHUNK = 1.0f / (150.0f / 36.0f);

void CMap::SnapBaseObjToSubChunk(CMapBaseObj *baseObj, NTempest::C3Vector &pos, float angle) {
  NTempest::C44Matrix mat;
  NTempest::CAaBox    tAaBox;
  NTempest::C3Vector  tVec;
  NTempest::C3Vector  cen;
  NTempest::CAaBox    aaBox;

  mat.Rotate(angle, NTempest::C3Vector(0.0f, 0.0f, 1.0f), 1);
  if (baseObj->GetType() & CMapBaseObj::Type_MapObjDef) {
    CMapObjDef *mapObjDef = static_cast<CMapObjDef *>(baseObj);
    mapObjDef->mapObj->GetBounds(tAaBox);
  } else {
    CMapStaticEntity *entity = static_cast<CMapStaticEntity *>(baseObj);
    ModelGetExtents(entity->model, &tAaBox);
  }

  cen = (tAaBox.b + tAaBox.t) * 0.5f;
  tAaBox.b = tAaBox.b - cen;
  tAaBox.t = tAaBox.t - cen;
  CWorldMath::TransformAABox(mat, tAaBox, aaBox);
  tVec = pos + aaBox.b;

  int x = static_cast<int>((tVec.x + 150.0f / 36.0f) * OO_COORD_TO_SUBCHUNK - 0.5f);
  pos.x -= tVec.x - x * (150.0f / 36.0f);
  pos.y -= tVec.y - static_cast<int>((tVec.y + 150.0f / 36.0f) * OO_COORD_TO_SUBCHUNK - 0.5f) * (150.0f / 36.0f);
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
    GxTexUpdate(skyTexid, 0, 0, 64, 8, 0);
  }

  UpdateLiquidTextures();

  WaterRadWave *wave = waterRipplesActive.Head();
  while (wave) {
    WaterRadWave *next = waterRipplesActive.Next(wave);
    if (!wave->Update(CWorld::tickTimeSec)) {
      waterRipplesActive.UnlinkNode(wave);
      waterRipplesFree.LinkNode(wave, LIST_TAIL, 0);
    }
    wave = next;
  }
}

void CMap::UpdateDoodadDef(CMapDoodadDef *doodadDef, NTempest::C3Vector &pos, float angle) {
  FATALASSERT(doodadDef);

  doodadDef->flags = CMapBaseObj::Flag_LightUpdate;
  doodadDef->pos = pos;
  doodadDef->scale = 1.0f;
  doodadDef->mat = NTempest::C44Matrix();
  doodadDef->mat.Translate(pos);
  doodadDef->mat.Rotate(angle, NTempest::C3Vector(0.0f, 0.0f, 1.0f), 1);
  doodadDef->lMat = NTempest::C44Matrix();

  if (ModelIsLoaded(doodadDef->model, 1)) {
    InitializeDoodadBounds(doodadDef);
  } else {
    doodadDef->aaSphere.c = pos;
    doodadDef->aaSphere.r = 0.0f;
    doodadDef->aaBox.b = pos;
    doodadDef->aaBox.t = pos;
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
  NTempest::CAaBox aaBox;
  CMapObjGroup    *mapObjGroup;
  SMOLight        *sLight;
  UINT             i;
  CMapObjDefGroup *mapObjDefGroup;

  FATALASSERT(mapObjDef);
  mapObjDef->pos = pos;
  mapObjDef->mat = NTempest::C44Matrix();
  mapObjDef->mat.Translate(pos);
  mapObjDef->mat.Rotate(angle, NTempest::C3Vector(0.0f, 0.0f, 1.0f), 1);
  mapObjDef->invMat = mapObjDef->mat.AffineInverse();

  FATALASSERT(mapObjDef->mapObj);
  if (!mapObjDef->mapObj->bLoaded) {
    mapObjDef->aaSphere.c = pos;
    mapObjDef->aaSphere.r = 0.0f;
    mapObjDef->aaBox.b = pos;
    mapObjDef->aaBox.t = pos;
    return;
  }

  mapObjDef->mapObj->GetBounds(mapObjDef->aaSphere);
  mapObjDef->aaSphere.c *= mapObjDef->mat;
  mapObjDef->mapObj->GetBounds(aaBox);
  CWorldMath::TransformAABox(mapObjDef->mat, aaBox, mapObjDef->aaBox);

  ITERATELIST(CMapBaseObjLink, mapObjDef->groupLinkList, groupLink) {
    mapObjDefGroup = static_cast<CMapObjDefGroup *>(groupLink->owner);
    FATALASSERT(mapObjDefGroup);
    mapObjGroup = mapObjDef->mapObj->GetGroup(mapObjDefGroup->groupNum, 0);
    if (mapObjGroup) {
      for (i = 0; i < mapObjGroup->lightRefCount; ++i) {
        sLight = &mapObjDef->mapObj->lightList[mapObjGroup->lightRefList[i]];
        if (mapObjDef->lightList[mapObjGroup->lightRefList[i]]) {
          mapObjDef->lightList[mapObjGroup->lightRefList[i]]->gxLight.m_dir = sLight->position * mapObjDef->mat;
          if (!(mapObjDefGroup->flags & CMapBaseObj::Flag_InteriorLit)) {
            UpdateLight(mapObjDef->lightList[mapObjGroup->lightRefList[i]]);
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
    CMapChunk          *chunk = static_cast<CMapChunk *>(link->owner);
    NTempest::C3Vector &cornerPos = chunk->vertexList[CMapChunk::cornerVertexIndex[corner]];
    chunk->camDist = CWorldScene::camPlaneXY.n.x * (cornerPos.x + chunk->corner.x) + CWorldScene::camPlaneXY.n.y * (cornerPos.y + chunk->corner.y) +
                     CWorldScene::camPlaneXY.n.z * (cornerPos.z + chunk->corner.z) + CWorldScene::camPlaneXY.d;
    chunk->lod = CWorld::lodMax;
    if ((CWorld::enables & CWorld::Enable_Lod) && chunk->camDist > CWorld::lodDist) {
      chunk->lod = CWorld::lodMin;
    }
    chunk->Update();
  }
}
