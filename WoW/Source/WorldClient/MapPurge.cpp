#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

#include "WorldClient/World.h"
#include "WorldClient/CMapObj.h"

#include "Base/Handle.h"
#include "Services/AsyncFileRead.h"
#include "WorldClient/DetailDoodad.h"

void CMap::Purge() {
  CMapBaseObjLink *link = areaLinkList.Head();
  while (reinterpret_cast<long>(link) > 0) {
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
    while (doodadDef->cacheLightList.Head()) {
      FreeCacheLight(doodadDef->cacheLightList.Head());
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
    CMapBaseObjLink *link = mapObjDef->groupLinkList.Head();
    while (reinterpret_cast<long>(link) > 0) {
      CMapBaseObjLink *next = mapObjDef->groupLinkList.RawNext(link);
      CMapObjDefGroup *mapObjDefGroup = static_cast<CMapObjDefGroup *>(link->owner);
      FreeBaseObjLink(link);
      PurgeMapObjDefGroup(mapObjDefGroup);
      link = next;
    }

    for (unsigned int i = 0; i < mapObjDef->lightList.Count(); ++i) {
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
    CMapBaseObjLink *link = mapObjDefGroup->doodadDefLinkList.Head();
    while (reinterpret_cast<long>(link) > 0) {
      CMapBaseObjLink *next = mapObjDefGroup->doodadDefLinkList.RawNext(link);
      CMapDoodadDef   *doodadDef = static_cast<CMapDoodadDef *>(link->owner);
      FreeBaseObjLink(link);
      PurgeDoodadDef(doodadDef);
      link = next;
    }

    link = mapObjDefGroup->entityLinkList.Head();
    while (reinterpret_cast<long>(link) > 0) {
      CMapBaseObjLink *next = mapObjDefGroup->entityLinkList.RawNext(link);
      FreeBaseObjLink(link);
      link = next;
    }

    link = mapObjDefGroup->lightLinkList.Head();
    while (reinterpret_cast<long>(link) > 0) {
      CMapBaseObjLink *next = mapObjDefGroup->lightLinkList.RawNext(link);
      FreeBaseObjLink(link);
      link = next;
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
    unsigned char *buffer = static_cast<unsigned char *>(asyncObject->buffer);
    AsyncFileReadDestroyObject(asyncObject);
    asyncObject = 0;
    if (buffer) {
      FreeAsyncLoadBuffer(buffer);
    }
  }

  CMapBaseObjLink *link = chunkLinkList.Head();
  while (reinterpret_cast<long>(link) > 0) {
    CMapBaseObjLink *next = chunkLinkList.RawNext(link);
    CMapChunk       *chunk = static_cast<CMapChunk *>(link->owner);
    chunkTable[chunk->infoIndex] = 0;
    chunkInfo[chunk->infoIndex].flags &= ~1u;
    chunkInfo[chunk->infoIndex].asyncId = 0;
    CMap::FreeBaseObjLink(link);
    CMap::PurgeChunk(chunk);
    link = next;
  }

  for (unsigned int i = 0; i < texIdTable.Count(); ++i) {
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
    unsigned char *buffer = static_cast<unsigned char *>(asyncObject->buffer);
    AsyncFileReadDestroyObject(asyncObject);
    asyncObject = 0;
    if (buffer) {
      FreeAsyncLoadBuffer(buffer);
    }
  }

  for (unsigned int i = 0; i < nLayers; ++i) {
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
    }
  }

  CMapBaseObjLink *link = doodadDefLinkList.Head();
  while (reinterpret_cast<long>(link) > 0) {
    CMapBaseObjLink *next = doodadDefLinkList.RawNext(link);
    CMapDoodadDef   *doodadDef = static_cast<CMapDoodadDef *>(link->owner);
    CMap::FreeBaseObjLink(link);
    CMap::PurgeDoodadDef(doodadDef);
    link = next;
  }

  link = mapObjDefLinkList.Head();
  while (reinterpret_cast<long>(link) > 0) {
    CMapBaseObjLink *next = mapObjDefLinkList.RawNext(link);
    CMapObjDef      *mapObjDef = static_cast<CMapObjDef *>(link->owner);
    CMap::FreeBaseObjLink(link);
    CMap::PurgeMapObjDef(mapObjDef);
    link = next;
  }

  link = entityLinkList.Head();
  while (reinterpret_cast<long>(link) > 0) {
    CMapBaseObjLink *next = entityLinkList.RawNext(link);
    CMap::FreeBaseObjLink(link);
    link = next;
  }

  link = lightLinkList.Head();
  while (reinterpret_cast<long>(link) > 0) {
    CMapBaseObjLink *next = lightLinkList.RawNext(link);
    CMap::FreeBaseObjLink(link);
    link = next;
  }

  CMapSoundEmitter *soundEmitter = soundEmitterList.Head();
  while (reinterpret_cast<long>(soundEmitter) > 0) {
    CMapSoundEmitter *next = soundEmitterList.RawNext(soundEmitter);
    if (soundEmitterDestroyHandler) {
      soundEmitterDestroyHandler(soundEmitter->data.soundPointID);
    }
    CMap::FreeSoundEmitter(soundEmitter);
    soundEmitter = next;
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
