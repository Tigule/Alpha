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

#include "Base/Handle.h"
#include "Services/AsyncFileRead.h"

void CMap::Purge() {
  CMapBaseObjLink *link = areaLinkList.Head();
  while (1) {
    if ((int)link <= 0) {
      break;
    }

    CMapBaseObjLink *next = areaLinkList.RawNext(link);
    CMapArea        *area = static_cast<CMapArea *>(link->owner);
    areaTable[area->infoIndex] = 0;
    areaInfo[area->infoIndex].flags &= ~1u;
    areaInfo[area->infoIndex].asyncId = 0;
    FreeBaseObjLink(link);
    PurgeArea(area);
    link = next;
  }

  CWorldScene::camMapObj = 0;
  CWorldScene::camMapObjDef = 0;
  CWorldScene::camMapObjGroup = 0;
}

void CMap::PurgeDoodadDef(CMapDoodadDef *doodadDef) {
  FATALASSERT(doodadDef);

  if (!doodadDef->refCount) {
    for (CMapCacheLight *cacheLight = doodadDef->cacheLightList.Head(), *next;
         (int)cacheLight > 0 ? ((next = doodadDef->cacheLightList.RawNext(cacheLight)), 1) : 0; cacheLight = next) {
      FreeCacheLight(cacheLight);
    }

    if (doodadDef->model) {
      HandleClose(reinterpret_cast<HOBJECT>(doodadDef->model));
    }
    doodadDef->model = 0;

    FreeDoodadDef(doodadDef);
  }
}

void CMap::PurgeMapObjDef(CMapObjDef *mapObjDef) {
  FATALASSERT(mapObjDef);

  if (!mapObjDef->refCount) {
    for (CMapBaseObjLink *groupLink = mapObjDef->groupLinkList.Head(), *next;
         (int)groupLink > 0 ? ((next = mapObjDef->groupLinkList.RawNext(groupLink)), 1) : 0; groupLink = next) {
      CMapObjDefGroup *mapObjDefGroup = static_cast<CMapObjDefGroup *>(groupLink->owner);
      FreeBaseObjLink(groupLink);
      PurgeMapObjDefGroup(mapObjDefGroup);
    }

    for (UINT i = 0; i < mapObjDef->lightList.Count(); ++i) {
      if (mapObjDef->lightList[i]) {
        DestroyLight(mapObjDef->lightList[i]);
      }
    }
    mapObjDef->lightList.SetCount(0);

    CMapObj::Delete(mapObjDef->mapObj);
    mapObjDef->mapObj = 0;
    FreeMapObjDef(mapObjDef);
  }
}

void CMap::PurgeMapObjDefGroup(CMapObjDefGroup *mapObjDefGroup) {
  FATALASSERT(mapObjDefGroup);

  if (!mapObjDefGroup->refCount) {
    CMapBaseObjLink *next;
    for (CMapBaseObjLink *doodadDefLink = mapObjDefGroup->doodadDefLinkList.Head();
         (int)doodadDefLink > 0 ? ((next = mapObjDefGroup->doodadDefLinkList.RawNext(doodadDefLink)), 1) : 0;
         doodadDefLink = next) {
      CMapDoodadDef *doodadDef = static_cast<CMapDoodadDef *>(doodadDefLink->owner);
      FreeBaseObjLink(doodadDefLink);
      PurgeDoodadDef(doodadDef);
    }

    for (CMapBaseObjLink *entityLink = mapObjDefGroup->entityLinkList.Head();
         (int)entityLink > 0 ? ((next = mapObjDefGroup->entityLinkList.RawNext(entityLink)), 1) : 0; entityLink = next) {
      FreeBaseObjLink(entityLink);
    }

    for (CMapBaseObjLink *lightLink = mapObjDefGroup->lightLinkList.Head();
         (int)lightLink > 0 ? ((next = mapObjDefGroup->lightLinkList.RawNext(lightLink)), 1) : 0; lightLink = next) {
      FreeBaseObjLink(lightLink);
    }

    FreeMapObjDefGroup(mapObjDefGroup);
  }
}

void CMap::PurgeArea(CMapArea *area) {
  area->Purge();
  FreeArea(area);
}

void CMap::PurgeChunk(CMapChunk *chunk) {
  chunk->Purge();
  FreeChunk(chunk);
}

void CMapArea::Purge() {
  if (asyncObject) {
    BYTE *buffer = static_cast<BYTE *>(asyncObject->buffer);
    AsyncFileReadDestroyObject(asyncObject);
    asyncObject = 0;
    if (buffer) {
      FreeAsyncLoadBuffer(buffer);
    }
  }

  CMapBaseObjLink *link = chunkLinkList.Head();
  while (1) {
    if ((int)link <= 0) {
      break;
    }

    CMapBaseObjLink *next = chunkLinkList.RawNext(link);
    CMapChunk       *chunk = static_cast<CMapChunk *>(link->owner);
    chunkTable[chunk->infoIndex] = 0;
    chunkInfo[chunk->infoIndex].flags &= ~1u;
    chunkInfo[chunk->infoIndex].asyncId = 0;
    CMap::FreeBaseObjLink(link);
    CMap::PurgeChunk(chunk);
    link = next;
  }

  for (UINT i = 0; i < texIdTable.Count(); ++i) {
    if (texIdTable[i]) {
      HandleClose(reinterpret_cast<HOBJECT>(texIdTable[i]));
      texIdTable[i] = 0;
    }
  }
  texIdTable.SetCount(0);
}

void CMapArea::PurgeChunks() {
  NTempest::CiRect gbChunkRect = CWorld::gbChunkRect;
  CMapBaseObjLink *chunkLinknext_node;

  for (CMapBaseObjLink *chunkLink = chunkLinkList.Head(); chunkLink; chunkLink = chunkLinknext_node) {
    chunkLinknext_node = chunkLinkList.Next(chunkLink);
    CMapChunk *chunk = static_cast<CMapChunk *>(chunkLink->owner);
    if (chunk->aIndex.x + 16 * mIndex.x < gbChunkRect.minx || chunk->aIndex.x + 16 * mIndex.x > gbChunkRect.maxx ||
        chunk->aIndex.y + 16 * mIndex.y < gbChunkRect.miny || chunk->aIndex.y + 16 * mIndex.y > gbChunkRect.maxy)
    {
      chunkTable[chunk->infoIndex] = 0;
      chunkInfo[chunk->infoIndex].flags &= ~1u;
      chunkInfo[chunk->infoIndex].asyncId = 0;
      CMap::FreeBaseObjLink(chunkLink);
      CMap::PurgeChunk(chunk);
    }
  }
}

void CMapChunk::Purge() {
  if (asyncObject) {
    BYTE *buffer = static_cast<BYTE *>(asyncObject->buffer);
    AsyncFileReadDestroyObject(asyncObject);
    asyncObject = 0;
    if (buffer) {
      FreeAsyncLoadBuffer(buffer);
    }
  }

  UINT i;
  for (i = 0; i < nLayers; ++i) {
    PurgeLayer(layerList[i]);
    layerList[i] = 0;
  }
  nLayers = 0;

  if (detailDoodadInst) {
    CDetailDoodad::FreeInst(detailDoodadInst);
    detailDoodadInst = 0;
  }
  if (gxBuf) {
    FreeGxBuf(gxBuf);
    gxBuf = 0;
  }
  if (shadowGxTexture) {
    FreeShadowGxTex(shadowGxTexture);
    shadowGxTexture = 0;
  }
  if (shaderGxTexture) {
    FreeShadowGxTex(shaderGxTexture);
    shaderGxTexture = 0;
  }
  if (shadowTexture) {
    CMap::FreeTex(shadowTexture);
    shadowTexture = 0;
  }

  for (i = 0; i < 4; ++i) {
    if (liquids[i]) {
      CMap::FreeChunkLiquid(liquids[i]);
      liquids[i] = 0;
    }
  }

  CMapBaseObjLink *next;
  for (CMapBaseObjLink *doodadDefLink = doodadDefLinkList.Head();
       (int)doodadDefLink > 0 ? ((next = doodadDefLinkList.RawNext(doodadDefLink)), 1) : 0; doodadDefLink = next) {
    CMapDoodadDef *doodadDef = static_cast<CMapDoodadDef *>(doodadDefLink->owner);
    CMap::FreeBaseObjLink(doodadDefLink);
    CMap::PurgeDoodadDef(doodadDef);
  }

  for (CMapBaseObjLink *mapObjDefLink = mapObjDefLinkList.Head();
       (int)mapObjDefLink > 0 ? ((next = mapObjDefLinkList.RawNext(mapObjDefLink)), 1) : 0; mapObjDefLink = next) {
    CMapObjDef *mapObjDef = static_cast<CMapObjDef *>(mapObjDefLink->owner);
    CMap::FreeBaseObjLink(mapObjDefLink);
    CMap::PurgeMapObjDef(mapObjDef);
  }

  for (CMapBaseObjLink *entityLink = entityLinkList.Head();
       (int)entityLink > 0 ? ((next = entityLinkList.RawNext(entityLink)), 1) : 0; entityLink = next) {
    CMap::FreeBaseObjLink(entityLink);
  }

  for (CMapBaseObjLink *lightLink = lightLinkList.Head();
       (int)lightLink > 0 ? ((next = lightLinkList.RawNext(lightLink)), 1) : 0; lightLink = next) {
    CMap::FreeBaseObjLink(lightLink);
  }

  for (CMapSoundEmitter *soundEmitter = soundEmitterList.Head(), *pNext;
       (int)soundEmitter > 0 ? ((pNext = soundEmitterList.RawNext(soundEmitter)), 1) : 0; soundEmitter = pNext) {
    if (soundEmitterDestroyHandler) {
      soundEmitterDestroyHandler(soundEmitter->data.soundPointID);
    }
    CMap::FreeSoundEmitter(soundEmitter);
  }
}

void CMapChunk::PurgeLayer(CChunkLayer *layer) {
  FATALASSERT(layer);

  layer->texId = 0;
  layer->props = 0;

  if (layer->gxTexture) {
    FreeAlphaGxTex(layer->gxTexture);
    layer->gxTexture = 0;
  }

  if (layer->tex) {
    CMap::FreeTex(layer->tex);
    layer->tex = 0;
  }

  CMap::FreeLayer(layer);
}
