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
    if ((INT_PTR)link <= 0) {
      break;
    }

    CMapBaseObjLink *next = areaLinkList.RawNext(link);
    CMapArea        *area = (CMapArea *)link->owner;
    areaTable[area->infoIndex] = NULL;
    areaInfo[area->infoIndex].flags &= ~1u;
    areaInfo[area->infoIndex].asyncId = 0;
    FreeBaseObjLink(link);
    PurgeArea(area);
    link = next;
  }

  CWorldScene::camMapObj = NULL;
  CWorldScene::camMapObjDef = NULL;
  CWorldScene::camMapObjGroup = NULL;
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
    doodadDef->model = NULL;

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
    mapObjDef->mapObj = NULL;
    FreeMapObjDef(mapObjDef);
  }
}

void CMap::PurgeMapObjDefGroup(CMapObjDefGroup *mapObjDefGroup) {
  FATALASSERT(mapObjDefGroup);

  if (!mapObjDefGroup->refCount) {
    SAFEITERATELIST(CMapBaseObjLink, mapObjDefGroup->doodadDefLinkList, doodadDefLink) {
      CMapDoodadDef *doodadDef = (CMapDoodadDef *)doodadDefLink->owner;
      FreeBaseObjLink(doodadDefLink);
      PurgeDoodadDef(doodadDef);
    }

    SAFEITERATELIST(CMapBaseObjLink, mapObjDefGroup->entityLinkList, entityLink) {
      FreeBaseObjLink(entityLink);
    }

    SAFEITERATELIST(CMapBaseObjLink, mapObjDefGroup->lightLinkList, lightLink) {
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
    asyncObject = NULL;
    if (buffer) {
      FreeAsyncLoadBuffer(buffer);
    }
  }

  SAFEITERATELIST(CMapBaseObjLink, chunkLinkList, link) {
    CMapChunk *chunk = (CMapChunk *)link->owner;
    chunkTable[chunk->infoIndex] = NULL;
    chunkInfo[chunk->infoIndex].flags &= ~1u;
    chunkInfo[chunk->infoIndex].asyncId = 0;
    CMap::FreeBaseObjLink(link);
    CMap::PurgeChunk(chunk);
  }

  for (UINT i = 0; i < texIdTable.Count(); ++i) {
    if (texIdTable[i]) {
      HandleClose(texIdTable[i]);
      texIdTable[i] = NULL;
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
      chunkTable[chunk->infoIndex] = NULL;
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
    asyncObject = NULL;
    if (buffer) {
      FreeAsyncLoadBuffer(buffer);
    }
  }

  UINT i;
  for (i = 0; i < nLayers; ++i) {
    PurgeLayer(layerList[i]);
    layerList[i] = NULL;
  }
  nLayers = 0;

  if (detailDoodadInst) {
    CDetailDoodad::FreeInst(detailDoodadInst);
    detailDoodadInst = NULL;
  }
  if (gxBuf) {
    FreeGxBuf(gxBuf);
    gxBuf = NULL;
  }
  if (shadowGxTexture) {
    FreeShadowGxTex(shadowGxTexture);
    shadowGxTexture = NULL;
  }
  if (shaderGxTexture) {
    FreeShadowGxTex(shaderGxTexture);
    shaderGxTexture = NULL;
  }
  if (shadowTexture) {
    CMap::FreeTex(shadowTexture);
    shadowTexture = NULL;
  }

  for (i = 0; i < 4; ++i) {
    if (liquids[i]) {
      CMap::FreeChunkLiquid(liquids[i]);
      liquids[i] = NULL;
    }
  }

  SAFEITERATELIST(CMapBaseObjLink, doodadDefLinkList, doodadDefLink) {
    CMapDoodadDef *doodadDef = (CMapDoodadDef *)doodadDefLink->owner;
    CMap::FreeBaseObjLink(doodadDefLink);
    CMap::PurgeDoodadDef(doodadDef);
  }

  SAFEITERATELIST(CMapBaseObjLink, mapObjDefLinkList, mapObjDefLink) {
    CMapObjDef *mapObjDef = (CMapObjDef *)mapObjDefLink->owner;
    CMap::FreeBaseObjLink(mapObjDefLink);
    CMap::PurgeMapObjDef(mapObjDef);
  }

  SAFEITERATELIST(CMapBaseObjLink, entityLinkList, entityLink) {
    CMap::FreeBaseObjLink(entityLink);
  }

  SAFEITERATELIST(CMapBaseObjLink, lightLinkList, lightLink) {
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

  layer->texId = NULL;
  layer->props = 0;

  if (layer->gxTexture) {
    FreeAlphaGxTex(layer->gxTexture);
    layer->gxTexture = NULL;
  }

  if (layer->tex) {
    CMap::FreeTex(layer->tex);
    layer->tex = NULL;
  }

  CMap::FreeLayer(layer);
}
