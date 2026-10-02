#include "Base/Base.h"
#include "Gx/Gx.h"
#include "Services/ParticleSystem2.h"
#include <WowConst.h>
#include "AaBsp.h"
#include <MapDefs.h>
#include "Gx/CGxDevice.h"

#include "WorldClient/World.h"
#include "WorldClient/CMapObj.h"
#include "WorldClient/WorldParam.h"
#include "WorldClient/DetailDoodad.h"
#include "WorldClient/CSimpleDoodad.h"
#include "DayNight.h"

#include "SoundInterface/SoundInterface.h"

int CMap::counts[Cnt_Num];
int CMap::freeCounts[Cnt_Num];
LISTDECLEX(CMapBaseObjLink, ownerLink, CMap::baseObjLinkFreeList);
LISTDECLEX(CMapObjGroup, lameAssLink, CMap::mapObjGroupFreeList);
LISTDECLEX(CMapObj, lameAssLink, CMap::mapObjFreeList);
LISTDECLEX(CMapArea, lameAssLink, CMap::areaFreeList);
LISTDECLEX(CMapChunk, lameAssLink, CMap::chunkFreeList);
LISTDECLEX(CMapDoodadDef, lameAssLink, CMap::doodadDefFreeList);
LISTDECLEX(CMapEntity, lameAssLink, CMap::entityFreeList);
LISTDECLEX(CMapLight, lameAssLink, CMap::lightFreeList);
LISTDECLEX(CMapObjDefGroup, lameAssLink, CMap::mapObjDefGroupFreeList);
LISTDECLEX(CMapObjDef, lameAssLink, CMap::mapObjDefFreeList);
LISTDECLEX(CChunkLiquid, lameAssLink, CMap::chunkLiquidFreeList);
LISTDECLEX(CMapSoundEmitter, lameAssLink, CMap::soundEmitterFreeList);
LISTDECLEX(CMapCacheLight, lameAssLink, CMap::cacheLightFreeList);
LISTDECL(CChunkLayer, CMap::chunkLayerFreeList);
LISTDECL(CChunkTex, CMap::chunkTexFreeList);
LISTDECLEX(CMapArea, lameAssLink, CMap::areaList);
LISTDECLEX(CMapChunk, lameAssLink, CMap::chunkList);
TSHashTable<CMapDoodadDef, HASHKEY_DWORD> CMap::doodadDefHash;
LISTDECLEX(CMapEntity, lameAssLink, CMap::entityList);
LISTDECLEX(CMapLight, lameAssLink, CMap::lightList);
LISTDECLEX(CMapObjDefGroup, lameAssLink, CMap::mapObjDefGroupList);
TSHashTable<CMapObjDef, HASHKEY_NONE> CMap::mapObjDefHash;
LISTDECLEX(CChunkLiquid, lameAssLink, CMap::chunkLiquidList);

CMapObj *CMap::AllocMapObj() {
  CMapObj *mapObj = mapObjFreeList.Head();
  if (mapObj) {
    mapObj->lameAssLink.Unlink();
  } else {
    mapObj = NEWZERO(CMapObj);
    FATALASSERT(mapObj);
  }

  mapObj->Init();
  return mapObj;
}

void CMap::FreeMapObj(CMapObj *mapObj) {
  ASSERT(mapObj);

  mapObj->lameAssLink.Unlink();
  mapObj->Clear();
  mapObjFreeList.LinkNode(mapObj, LIST_TAIL, 0);
}

CMapObjGroup *CMap::AllocMapObjGroup() {
  CMapObjGroup *group = mapObjGroupFreeList.Head();
  if (group) {
    group->lameAssLink.Unlink();
  } else {
    group = NEWZERO(CMapObjGroup);
    FATALASSERT(group);
  }

  group->Init();
  return group;
}

void CMap::FreeMapObjGroup(CMapObjGroup *group) {
  ASSERT(group);

  group->lameAssLink.Unlink();
  group->Clear();
  mapObjGroupFreeList.LinkNode(group, LIST_TAIL, 0);
}

CChunkLayer *CMap::GetLayer() {
  CChunkLayer *layer = chunkLayerFreeList.Head();
  if (!layer) {
    layer = chunkLayerFreeList.NewNode(LIST_TAIL, 0, 0);
    ++freeCounts[Cnt_ChunkLayer];
    FATALASSERT(layer);
  }

  layer->Unlink();
  --freeCounts[Cnt_ChunkLayer];
  ++counts[Cnt_ChunkLayer];
  return layer;
}

void CMap::FreeLayer(CChunkLayer *layer) {
  ASSERT(layer);

  layer->Unlink();
  chunkLayerFreeList.LinkNode(layer, LIST_HEAD, 0);

  --counts[Cnt_ChunkLayer];
  ++freeCounts[Cnt_ChunkLayer];
}

CChunkTex *CMap::GetTex() {
  CChunkTex *tex = chunkTexFreeList.Head();
  if (!tex) {
    tex = chunkTexFreeList.NewNode(LIST_TAIL, 0, 0);
    ++freeCounts[Cnt_ChunkTex];
    FATALASSERT(tex);
  }

  tex->Unlink();
  --freeCounts[Cnt_ChunkTex];
  ++counts[Cnt_ChunkTex];
  return tex;
}

void CMap::FreeTex(CChunkTex *tex) {
  ASSERT(tex);

  tex->Unlink();
  chunkTexFreeList.LinkNode(tex, LIST_HEAD, 0);

  --counts[Cnt_ChunkTex];
  ++freeCounts[Cnt_ChunkTex];
}

CMapBaseObjLink *CMap::AllocBaseObjLink(CMapBaseObj *owner) {
  ASSERT(owner);

  CMapBaseObjLink *link = baseObjLinkFreeList.Head();
  if (!link) {
    link = baseObjLinkFreeList.NewNode(LIST_TAIL, 0, 0);
    ASSERT(link);
    ++freeCounts[Cnt_BaseObjLink];
  }

  link->ownerLink.Unlink();
  ++owner->refCount;
  link->owner = owner;
  link->ref = 0;
  owner->parentLinkList.LinkNode(link, LIST_TAIL, 0);

  --freeCounts[Cnt_BaseObjLink];
  ++counts[Cnt_BaseObjLink];
  return link;
}

void CMap::FreeBaseObjLink(CMapBaseObjLink *link) {
  ASSERT(link);

  link->ownerLink.Unlink();
  link->refLink.Unlink();

  CMapBaseObj *owner = link->owner;
  ASSERT(owner);
  --owner->refCount;

  link->ref = 0;
  link->owner = 0;
  baseObjLinkFreeList.LinkNode(link, LIST_TAIL, 0);

  --counts[Cnt_BaseObjLink];
  ++freeCounts[Cnt_BaseObjLink];
}

CMapArea *CMap::AllocArea() {
  CMapArea *area = areaFreeList.Head();
  if (!area) {
    area = areaFreeList.NewNode(LIST_TAIL, 0, 0);
    FATALASSERT(area);
    ++freeCounts[Cnt_Area];
  }

  area->lameAssLink.Unlink();
  areaList.LinkNode(area, LIST_TAIL, 0);

  --freeCounts[Cnt_Area];
  ++counts[Cnt_Area];
  return area;
}

void CMap::FreeArea(CMapArea *area) {
  FATALASSERT(area);
  FATALASSERT(area->parentLinkList.Head() == 0);
  FATALASSERT(area->chunkLinkList.Head() == 0);
  FATALASSERT(area->texIdTable.Count() == 0);

  area->lameAssLink.Unlink();
  areaFreeList.LinkNode(area, LIST_TAIL, 0);

  --counts[Cnt_Area];
  ++freeCounts[Cnt_Area];
}

CMapChunk *CMap::AllocChunk() {
  CMapChunk *chunk = chunkFreeList.Head();
  if (!chunk) {
    chunk = chunkFreeList.NewNode(LIST_TAIL, 0, 0);
    FATALASSERT(chunk);
    ++freeCounts[Cnt_Chunk];
  }

  chunk->lameAssLink.Unlink();
  chunkList.LinkNode(chunk, LIST_TAIL, 0);

  --freeCounts[Cnt_Chunk];
  ++counts[Cnt_Chunk];
  return chunk;
}

void CMap::FreeChunk(CMapChunk *chunk) {
  FATALASSERT(chunk);
  FATALASSERT(chunk->parentLinkList.Head() == 0);
  FATALASSERT(chunk->doodadDefLinkList.Head() == 0);
  FATALASSERT(chunk->mapObjDefLinkList.Head() == 0);
  FATALASSERT(chunk->entityLinkList.Head() == 0);
  FATALASSERT(chunk->shadowTexture == 0);
  FATALASSERT(chunk->shadowGxTexture == 0);
  FATALASSERT(chunk->detailDoodadInst == 0);
  FATALASSERT(chunk->nLayers == 0);

  chunk->lameAssLink.Unlink();
  chunkFreeList.LinkNode(chunk, LIST_TAIL, 0);

  --counts[Cnt_Chunk];
  ++freeCounts[Cnt_Chunk];
}

CMapDoodadDef *CMap::AllocDoodadDef() {
  CMapDoodadDef *doodadDef = doodadDefFreeList.Head();
  if (!doodadDef) {
    doodadDef = doodadDefFreeList.NewNode(LIST_TAIL, 0, 0);
    FATALASSERT(doodadDef);
    ++freeCounts[Cnt_DoodadDef];
  }

  doodadDef->lameAssLink.Unlink();
  doodadDef->RenderCB = 0;
  doodadDef->model = 0;

  --freeCounts[Cnt_DoodadDef];
  ++counts[Cnt_DoodadDef];
  return doodadDef;
}

void CMap::FreeDoodadDef(CMapDoodadDef *doodadDef) {
  FATALASSERT(doodadDef);

  if (doodadDef->doodadSoundHandle) {
    SndInterfaceHandleDoodadLoopStop(doodadDef->doodadSoundHandle);
  }

  FATALASSERT(doodadDef->parentLinkList.Head() == 0);
  FATALASSERT(doodadDef->model == 0);

  doodadDef->lameAssLink.Unlink();
  doodadDefHash.Unlink(doodadDef);
  doodadDefFreeList.LinkNode(doodadDef, LIST_TAIL, 0);

  --counts[Cnt_DoodadDef];
  ++freeCounts[Cnt_DoodadDef];
}

CMapEntity *CMap::AllocEntity() {
  CMapEntity *entity = entityFreeList.Head();
  if (!entity) {
    entity = entityFreeList.NewNode(LIST_TAIL, 0, 0);
    FATALASSERT(entity);
    ++freeCounts[Cnt_Entity];
  }

  entity->lameAssLink.Unlink();
  entityList.LinkNode(entity, LIST_TAIL, 0);

  --freeCounts[Cnt_Entity];
  ++counts[Cnt_Entity];
  return entity;
}

void CMap::FreeEntity(CMapEntity *entity) {
  FATALASSERT(entity);
  FATALASSERT(entity->parentLinkList.Head() == 0);

  entity->lameAssLink.Unlink();
  entityFreeList.LinkNode(entity, LIST_TAIL, 0);

  --counts[Cnt_Entity];
  ++freeCounts[Cnt_Entity];
}

CMapLight *CMap::AllocLight() {
  CMapLight *light = lightFreeList.Head();
  if (!light) {
    light = lightFreeList.NewNode(LIST_TAIL, 0, 0);
    ASSERT(light);
    ++freeCounts[Cnt_Light];
  }

  light->lameAssLink.Unlink();
  lightList.LinkNode(light, LIST_TAIL, 0);

  --freeCounts[Cnt_Light];
  ++counts[Cnt_Light];
  return light;
}

void CMap::FreeLight(CMapLight *light) {
  ASSERT(light);
  ASSERT(light->parentLinkList.Head() == 0);

  light->lameAssLink.Unlink();
  lightFreeList.LinkNode(light, LIST_TAIL, 0);

  --counts[Cnt_Light];
  ++freeCounts[Cnt_Light];
}

CMapCacheLight *CMap::AllocCacheLight() {
  CMapCacheLight *cacheLight = cacheLightFreeList.Head();
  if (!cacheLight) {
    cacheLight = cacheLightFreeList.NewNode(LIST_TAIL, 0, 0);
    ASSERT(cacheLight);
    ++freeCounts[Cnt_CacheLight];
  }

  cacheLight->lameAssLink.Unlink();

  --freeCounts[Cnt_CacheLight];
  ++counts[Cnt_CacheLight];
  return cacheLight;
}

void CMap::FreeCacheLight(CMapCacheLight *cacheLight) {
  ASSERT(cacheLight);

  cacheLight->lameAssLink.Unlink();
  cacheLightFreeList.LinkNode(cacheLight, LIST_TAIL, 0);

  --counts[Cnt_CacheLight];
  ++freeCounts[Cnt_CacheLight];
}

CMapObjDefGroup *CMap::AllocMapObjDefGroup() {
  CMapObjDefGroup *mapObjDefGroup = mapObjDefGroupFreeList.Head();
  if (!mapObjDefGroup) {
    mapObjDefGroup = mapObjDefGroupFreeList.NewNode(LIST_TAIL, 0, 0);
    FATALASSERT(mapObjDefGroup);
    ++freeCounts[Cnt_MapObjDefGroup];
  }

  mapObjDefGroup->lameAssLink.Unlink();
  mapObjDefGroupList.LinkNode(mapObjDefGroup, LIST_TAIL, 0);

  --freeCounts[Cnt_MapObjDefGroup];
  ++counts[Cnt_MapObjDefGroup];
  return mapObjDefGroup;
}

void CMap::FreeMapObjDefGroup(CMapObjDefGroup *mapObjDefGroup) {
  FATALASSERT(mapObjDefGroup);
  FATALASSERT(mapObjDefGroup->parentLinkList.Head() == 0);
  FATALASSERT(mapObjDefGroup->doodadDefLinkList.Head() == 0);
  FATALASSERT(mapObjDefGroup->entityLinkList.Head() == 0);
  FATALASSERT(mapObjDefGroup->lightLinkList.Head() == 0);

  mapObjDefGroup->lameAssLink.Unlink();
  mapObjDefGroupFreeList.LinkNode(mapObjDefGroup, LIST_TAIL, 0);

  --counts[Cnt_MapObjDefGroup];
  ++freeCounts[Cnt_MapObjDefGroup];
}

CMapObjDef *CMap::AllocMapObjDef() {
  CMapObjDef *mapObjDef = mapObjDefFreeList.Head();
  if (!mapObjDef) {
    mapObjDef = mapObjDefFreeList.NewNode(LIST_TAIL, 0, 0);
    FATALASSERT(mapObjDef);
    ++freeCounts[Cnt_MapObjDef];
  }

  mapObjDef->lameAssLink.Unlink();

  --freeCounts[Cnt_MapObjDef];
  ++counts[Cnt_MapObjDef];
  return mapObjDef;
}

void CMap::FreeMapObjDef(CMapObjDef *mapObjDef) {
  FATALASSERT(mapObjDef);
  FATALASSERT(mapObjDef->parentLinkList.Head() == 0);
  FATALASSERT(mapObjDef->groupLinkList.Head() == 0);

  mapObjDef->lameAssLink.Unlink();
  mapObjDefHash.Unlink(mapObjDef);
  mapObjDefFreeList.LinkNode(mapObjDef, LIST_TAIL, 0);

  --counts[Cnt_MapObjDef];
  ++freeCounts[Cnt_MapObjDef];
}

CChunkLiquid *CMap::AllocChunkLiquid() {
  CChunkLiquid *liquid = chunkLiquidFreeList.Head();
  if (!liquid) {
    liquid = chunkLiquidFreeList.NewNode(LIST_TAIL, 0, 0);
  }

  liquid->lameAssLink.Unlink();
  chunkLiquidList.LinkNode(liquid, LIST_TAIL, 0);
  return liquid;
}

void CMap::FreeChunkLiquid(CChunkLiquid *&cl) {
  FATALASSERT(cl);

  cl->lameAssLink.Unlink();
  chunkLiquidFreeList.LinkNode(cl, LIST_TAIL, 0);
  cl = 0;
}

CMapSoundEmitter *CMap::AllocSoundEmitter() {
  CMapSoundEmitter *soundEmitter = soundEmitterFreeList.Head();
  if (!soundEmitter) {
    soundEmitter = soundEmitterFreeList.NewNode(LIST_TAIL, 0, 0);
    FATALASSERT(soundEmitter);
  }

  soundEmitter->lameAssLink.Unlink();
  return soundEmitter;
}

void CMap::FreeSoundEmitter(CMapSoundEmitter *soundEmitter) {
  FATALASSERT(soundEmitter);

  soundEmitter->lameAssLink.Unlink();
  soundEmitterFreeList.LinkNode(soundEmitter, LIST_TAIL, 0);
}
