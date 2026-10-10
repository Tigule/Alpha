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

#include "Base/Handle.h"
#include "Services/AsyncFileRead.h"

void CMap::Purge() {
  CMapBaseObjLink *link = areaLinkList.Head();
  while (1) {
    if ((int)link <= 0) {
      break;
    }

    CMapBaseObjLink *next = areaLinkList.RawNext(link);
    CMapArea        *area = (CMapArea *)link->owner;
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
    SAFEITERATELIST(CMapCacheLight, doodadDef->cacheLightList, cacheLight) {
      FreeCacheLight(cacheLight);
    }

    if (doodadDef->model) {
      HandleClose(doodadDef->model);
    }
    doodadDef->model = 0;

    FreeDoodadDef(doodadDef);
  }
}

void CMap::PurgeMapObjDef(CMapObjDef *mapObjDef) {
  FATALASSERT(mapObjDef);

  if (!mapObjDef->refCount) {
    SAFEITERATELIST(CMapBaseObjLink, mapObjDef->groupLinkList, groupLink) {
      CMapObjDefGroup *mapObjDefGroup = (CMapObjDefGroup *)groupLink->owner;
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
      CMapDoodadDef *doodadDef = (CMapDoodadDef *)doodadDefLink->owner;
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
    BYTE *buffer = (BYTE *)asyncObject->buffer;
    AsyncFileReadDestroyObject(asyncObject);
    asyncObject = 0;
    if (buffer) {
      FreeAsyncLoadBuffer(buffer);
    }
  }

  SAFEITERATELIST(CMapBaseObjLink, chunkLinkList, link) {
    CMapChunk *chunk = (CMapChunk *)link->owner;
    chunkTable[chunk->infoIndex] = 0;
    chunkInfo[chunk->infoIndex].flags &= ~1u;
    chunkInfo[chunk->infoIndex].asyncId = 0;
    CMap::FreeBaseObjLink(link);
    CMap::PurgeChunk(chunk);
  }

  for (UINT i = 0; i < texIdTable.Count(); ++i) {
    if (texIdTable[i]) {
      HandleClose(texIdTable[i]);
      texIdTable[i] = 0;
    }
  }
  texIdTable.SetCount(0);
}

void CMapArea::PurgeChunks() {
  NTempest::CiRect gbChunkRect = CWorld::gbChunkRect;
  SAFEITERATELIST(CMapBaseObjLink, chunkLinkList, chunkLink) {
    CMapChunk *chunk = (CMapChunk *)chunkLink->owner;
    NTempest::C2iVector chunkIndex(mIndex.x * 16 + chunk->aIndex.x, mIndex.y * 16 + chunk->aIndex.y);
    if (!gbChunkRect.Contains(chunkIndex)) {
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
    BYTE *buffer = (BYTE *)asyncObject->buffer;
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
    CMapDoodadDef *doodadDef = (CMapDoodadDef *)doodadDefLink->owner;
    CMap::FreeBaseObjLink(doodadDefLink);
    CMap::PurgeDoodadDef(doodadDef);
  }

  for (CMapBaseObjLink *mapObjDefLink = mapObjDefLinkList.Head();
       (int)mapObjDefLink > 0 ? ((next = mapObjDefLinkList.RawNext(mapObjDefLink)), 1) : 0; mapObjDefLink = next) {
    CMapObjDef *mapObjDef = (CMapObjDef *)mapObjDefLink->owner;
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

  SAFEITERATELIST(CMapSoundEmitter, soundEmitterList, soundEmitter) {
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
