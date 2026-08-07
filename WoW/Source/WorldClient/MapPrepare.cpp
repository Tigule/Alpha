#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

#include "WorldClient/World.h"
#include "WorldClient/CMapObj.h"
#include "WorldCommon/WorldMath.h"

#include <Model/IModel.h>
#include <Services/AsyncFileRead.h>

void CMap::PrepareUpdate() {
  bspRecurseCount = 0;
  mapGetFacetsCount = 0;
  oldSelectLightParm = 0;
  CMapObj::PrepareUpdate();
  PrepareAreas();
  PrepareChunks();
  PrepareMapObjDefs();
  PrepareDoodadDefs();
}

void CMap::PrepareAreas() {
  CMapBaseObjLink *areaLink = areaLinkList.Head();

  while (reinterpret_cast<long>(areaLink) > 0) {
    CMapArea        *area = static_cast<CMapArea *>(areaLink->owner);
    CMapBaseObjLink *next = areaLinkList.RawNext(areaLink);

    if (area->mIndex.x >= CWorld::areaRect.minx && area->mIndex.x <= CWorld::areaRect.maxx && area->mIndex.y >= CWorld::areaRect.miny &&
        area->mIndex.y <= CWorld::areaRect.maxy)
    {
      area->PrepareLocalRect();
      area->PurgeChunks();
    } else {
      areaTable[area->infoIndex] = 0;
      areaInfo[area->infoIndex].flags &= ~1u;
      areaInfo[area->infoIndex].asyncId = 0;
      FreeBaseObjLink(areaLink);
      PurgeArea(area);
    }

    areaLink = next;
  }
}

void CMap::PrepareMapObjDefs() {
  ITERATELIST(CMapObjDef, mapObjDefHash, mapObjDef) {
    FATALASSERT(mapObjDef->mapObj);

    if (CWorld::objectAoi.b <= mapObjDef->aaBox.t && CWorld::objectAoi.t >= mapObjDef->aaBox.b) {
      while (!mapObjDef->mapObj->bLoaded) {
        mapObjDef->mapObj->WaitLoad();
      }
      if (!(mapObjDef->flags & CMapBaseObj::Flag_Loaded)) {
        PrepareMapObjDef(mapObjDef, mapObjDef->mapObj);
      }
    }

    ITERATELIST(CMapBaseObjLink, mapObjDef->groupLinkList, groupLink) {
      CMapObjDefGroup *mapObjDefGroup = static_cast<CMapObjDefGroup *>(groupLink->owner);
      FATALASSERT(mapObjDefGroup);
      CMapObjGroup *mapObjGroup = mapObjDef->mapObj->GetGroup(mapObjDefGroup->groupNum, 1);
      FATALASSERT(mapObjGroup);

      if (CWorld::objectAoi.b <= mapObjDefGroup->aaBox.t && CWorld::objectAoi.t >= mapObjDefGroup->aaBox.b) {
        mapObjGroup->flushTime = 30.0f;
        if (!mapObjGroup->bLoaded) {
          if (!mapObjGroup->asyncObject) {
            mapObjDef->mapObj->ReadGroup(mapObjDefGroup->groupNum);
          }
          if (CWorld::groupAoi.b <= mapObjDefGroup->aaBox.t && CWorld::groupAoi.t >= mapObjDefGroup->aaBox.b) {
            mapObjDef->mapObj->WaitLoadGroup(mapObjDefGroup->groupNum);
          }
        }

        if (mapObjGroup->bLoaded) {
          if (!(mapObjDefGroup->flags & CMapBaseObj::Flag_HasLights)) {
            CreateMapObjDefLights(mapObjDef->mapObj, mapObjGroup, mapObjDef, mapObjDefGroup);
          }
          if (!(mapObjDefGroup->flags & CMapBaseObj::Flag_HasDoodadRefs)) {
            CreateMapObjDefGroupDoodads(mapObjDef->mapObj, mapObjGroup, mapObjDef, mapObjDefGroup);
          }
        }
      }
    }
  }
}

void CMap::PrepareDoodadDefs() {
  UINT count = 0;
  ITERATELIST(CMapDoodadDef, doodadDefHash, doodadDef) {
    if (!(doodadDef->flags & CMapBaseObj::Flag_Loaded)) {
      if (!doodadDef->model) {
        ++count;
        LoadDoodadModel(doodadDef, 0);
        ModelSetLightSelectCallback(doodadDef->model, SelectLight, doodadDef, 1);
        if (!(doodadDef->flags & CMapBaseObj::Flag_InteriorLit) && QueryShadow(doodadDef->pos)) {
          doodadDef->dirLightScale = CMapStaticEntity::dirLightScaleAmount;
        }
      }

      if (ModelIsLoaded(doodadDef->model, 1)) {
        InitializeDoodadBounds(doodadDef);
        doodadDef->flags |= CMapBaseObj::Flag_LightUpdate | CMapBaseObj::Flag_Loaded;
      }

      if (count > 16 && !bPreload) {
        break;
      }
    }
  }
}

void CMap::QueryLightmap(CMapDoodadDef *doodadDef) {
  CMapBaseObjLink *mapObjDefGroupLink = doodadDef->parentLinkList.Head();
  CMapObjDefGroup *mapObjDefGroup = static_cast<CMapObjDefGroup *>(mapObjDefGroupLink->ref);
  CMapBaseObjLink *mapObjDefLink = mapObjDefGroup->parentLinkList.Head();
  CMapObjDef      *mapObjDef = static_cast<CMapObjDef *>(mapObjDefLink->ref);
  CMapObjGroup    *mapObjGroup = mapObjDef->mapObj->GetGroup(mapObjDefGroup->groupNum, 0);
  FATALASSERT(mapObjGroup);
  doodadDef->QueryLightmap(mapObjDef, mapObjGroup);
}

void CMap::UpdateMapObjDefGroupDoodads(CMapObj *mapObj, CMapObjGroup *mapObjGroup, CMapObjDef *mapObjDef, CMapObjDefGroup *mapObjDefGroup) {
  BOOL bFini = 1;
  FATALASSERT(mapObj);
  FATALASSERT(mapObjGroup);
  FATALASSERT(mapObjDef);
  FATALASSERT(mapObjDefGroup);

  UINT             count = 0;
  CMapBaseObjLink *doodadDefLink = mapObjDefGroup->doodadDefLinkList.Head();

  while (reinterpret_cast<long>(doodadDefLink) > 0) {
    CMapDoodadDef *doodadDef = static_cast<CMapDoodadDef *>(doodadDefLink->owner);

    if (!doodadDef->model) {
      bFini = 0;
      LoadDoodadModel(doodadDef, 0);
      ModelSetLightSelectCallback(doodadDef->model, SelectLight, doodadDef, 1);

      if (!(mapObjDefGroup->flags & CMapBaseObj::Flag_InteriorLit) && QueryShadow(doodadDef->pos)) {
        doodadDef->dirLightScale = CMapStaticEntity::dirLightScaleAmount;
      }

      ++count;
    }

    if (count >= 16 && !bPreload) {
      break;
    }

    doodadDefLink = mapObjDefGroup->doodadDefLinkList.RawNext(doodadDefLink);
  }

  if (bFini || bPreload) {
    mapObjDefGroup->flags |= CMapBaseObj::Flag_HasAllDoodads;
  }
}

void CMap::PrepareMapObjDef(CMapObjDef *mapObjDef, CMapObj *mapObj) {
  FATALASSERT(mapObjDef);
  FATALASSERT(mapObj);

  mapObjDef->flags |= CMapBaseObj::Flag_Loaded;
  NTempest::CAaBox aaBox;
  mapObj->GetBounds(mapObjDef->aaSphere);
  mapObjDef->aaSphere.c *= mapObjDef->mat;
  mapObj->GetBounds(aaBox);
  CWorldMath::TransformAABox(mapObjDef->mat, aaBox, mapObjDef->aaBox);
  mapObjDef->ambient = mapObj->ambColor;

  mapObjDef->lightList.SetCount(mapObj->lightCount);
  for (UINT i = 0; i < mapObjDef->lightList.Count(); ++i) {
    mapObjDef->lightList[i] = 0;
  }

  CreateMapObjDefGroups(mapObj, mapObjDef);
}

void CMap::PrepareChunks() {
  FATALASSERT(wdtFile);

  nChunksPrepared = 0;
  nGbChunksPrepared = 0;

  NTempest::C2iVector chunkIndex;
  for (chunkIndex.y = CWorld::gbChunkRect.miny; chunkIndex.y <= CWorld::gbChunkRect.maxy; ++chunkIndex.y) {
    for (chunkIndex.x = CWorld::gbChunkRect.minx; chunkIndex.x <= CWorld::gbChunkRect.maxx; ++chunkIndex.x) {
      UINT areaIndex = (chunkIndex.x >> 4) + 64 * (chunkIndex.y >> 4);
      int  cIdx = (chunkIndex.x & 0xF) + 16 * (chunkIndex.y & 0xF);

      if (!areaInfo[areaIndex].offset) {
        continue;
      }

      CMapArea *area = areaTable[areaIndex];
      if (chunkIndex.x >= CWorld::chunkRectHi.minx && chunkIndex.x <= CWorld::chunkRectHi.maxx && chunkIndex.y >= CWorld::chunkRectHi.miny &&
          chunkIndex.y <= CWorld::chunkRectHi.maxy)
      {
        if (!area) {
          if (!areaInfo[areaIndex].asyncId) {
            PrepareArea(chunkIndex.x >> 4, chunkIndex.y >> 4);
          }
          if (areaInfo[areaIndex].asyncId) {
            AsyncFileReadWait(reinterpret_cast<CAsyncObject *>(areaInfo[areaIndex].asyncId));
          }
          area = areaTable[areaIndex];
          FATALASSERT(area);
        }

        if (!area->chunkTable[cIdx]) {
          if (!area->chunkInfo[cIdx].asyncId) {
            PrepareChunk(area, chunkIndex.x & 0xF, chunkIndex.y & 0xF);
          }
          if (area->chunkInfo[cIdx].asyncId) {
            AsyncFileReadWait(reinterpret_cast<CAsyncObject *>(area->chunkInfo[cIdx].asyncId));
          }
          FATALASSERT(area->chunkTable[cIdx]);
        }
      } else if (area) {
        FATALASSERT(area->chunkInfo[cIdx].offset);
        if (!(area->chunkInfo[cIdx].flags & 1)) {
          PrepareChunk(area, chunkIndex.x & 0xF, chunkIndex.y & 0xF);
          ++nChunksPrepared;
        }
      } else if (!(areaInfo[areaIndex].flags & 1)) {
        PrepareArea(chunkIndex.x >> 4, chunkIndex.y >> 4);
      }
    }
  }
}

void CMap::PrepareArea(int x, int y) {
  FATALASSERT(wdtFile);

  DWORD index = x + 64 * y;
  FATALASSERT(areaInfo[index].flags == 0);
  FATALASSERT(areaTable[index] == 0);

  areaInfo[index].flags |= 1;
  CMapArea *area = AllocArea();
  FATALASSERT(area);

  CMapBaseObjLink *areaLink = AllocBaseObjLink(area);
  areaLinkList.LinkNode(areaLink, LIST_TAIL, 0);

  area->mIndex.x = x;
  area->mIndex.y = y;
  area->infoIndex = index;
  area->asyncObject = 0;
  area->cOffset.x = 16 * x;
  area->cOffset.y = 16 * y;
  area->corner.x = static_cast<float>(16 * y) * -33.333332f + 17066.666f;
  area->corner.y = static_cast<float>(16 * x) * -33.333332f + 17066.666f;
  area->PrepareLocalRect();
  area->Load(&areaInfo[index]);
}

void CMap::PrepareChunk(CMapArea *area, int x, int y) {
  FATALASSERT(area);
  FATALASSERT(wdtFile);

  UINT index = x + 16 * y;
  FATALASSERT(area->chunkInfo[index].flags == 0);
  FATALASSERT(area->chunkTable[index] == 0);

  area->chunkInfo[index].flags |= 1;
  CMapChunk       *chunk = AllocChunk();
  CMapBaseObjLink *chunkLink = AllocBaseObjLink(chunk);
  chunkLink->ref = area;
  area->chunkLinkList.LinkNode(chunkLink, LIST_TAIL, 0);

  chunk->aIndex.x = x;
  chunk->aIndex.y = y;
  chunk->infoIndex = index;
  chunk->cOffset.x = x + area->cOffset.x;
  chunk->cOffset.y = y + area->cOffset.y;
  chunk->sOffset.x = 8 * chunk->cOffset.x;
  chunk->sOffset.y = 8 * chunk->cOffset.y;
  chunk->Load(&area->chunkInfo[index]);
}

void CMap::CreateChunkNeighborPtrs(CMapChunk *chunk) {
  FATALASSERT(chunk);

  CMapArea *area = static_cast<CMapArea *>(chunk->parentLinkList.Head()->ref);
  FATALASSERT(area);

  UINT areaIndex = area->mIndex.x + 64 * area->mIndex.y;
  UINT chunkIndex = chunk->aIndex.x + 16 * chunk->aIndex.y;

  chunk->neighbor[0] = 0;
  chunk->neighbor[1] = 0;
  chunk->neighbor[2] = 0;
  chunk->neighbor[3] = 0;

  if (chunk->aIndex.y > 0) {
    chunk->neighbor[0] = area->chunkTable[chunkIndex - 16];
  } else if (chunk->aIndex.y == 0 && area->mIndex.y > 0 && areaTable[areaIndex - 64]) {
    chunk->neighbor[0] = areaTable[areaIndex - 64]->chunkTable[chunk->aIndex.x + 240];
  }

  if (chunk->neighbor[0]) {
    chunk->neighbor[0]->neighbor[2] = chunk;
  }

  if (chunk->aIndex.x < 15) {
    chunk->neighbor[1] = area->chunkTable[chunkIndex + 1];
  } else if (chunk->aIndex.x == 15 && area->mIndex.x < 63 && areaTable[areaIndex + 1]) {
    chunk->neighbor[1] = areaTable[areaIndex + 1]->chunkTable[chunkIndex - 15];
  }

  if (chunk->neighbor[1]) {
    chunk->neighbor[1]->neighbor[3] = chunk;
  }

  if (chunk->aIndex.y < 15) {
    chunk->neighbor[2] = area->chunkTable[chunkIndex + 16];
  } else if (chunk->aIndex.y == 15 && area->mIndex.y < 63 && areaTable[areaIndex + 64]) {
    chunk->neighbor[2] = areaTable[areaIndex + 64]->chunkTable[chunk->aIndex.x];
  }

  if (chunk->neighbor[2]) {
    chunk->neighbor[2]->neighbor[0] = chunk;
  }

  if (chunk->aIndex.x > 0) {
    chunk->neighbor[3] = area->chunkTable[chunkIndex - 1];
  } else if (chunk->aIndex.x == 0 && area->mIndex.x > 0 && areaTable[areaIndex - 1]) {
    chunk->neighbor[3] = areaTable[areaIndex - 1]->chunkTable[chunkIndex + 15];
  }

  if (chunk->neighbor[3]) {
    chunk->neighbor[3]->neighbor[1] = chunk;
  }
}
