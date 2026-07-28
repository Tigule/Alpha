#include "WorldClient/World.h"
#include "WorldClient/CMapObj.h"

#include "Base/Base.h"
#include "SoundInterface/SoundInterface.h"

int                                          CMap::counts[Cnt_Num];
int                                          CMap::freeCounts[Cnt_Num];
LISTDECLEX(CMapObjGroup, lameAssLink, CMap::mapObjGroupFreeList);
LISTDECLEX(CMapChunk, lameAssLink, CMap::chunkList);
LISTDECLEX(CMapLight, lameAssLink, CMap::lightList);
TSHashTable<CMapObjDef, HASHKEY_NONE>        CMap::mapObjDefHash;
LISTDECLEX(CMapCacheLight, lameAssLink, CMap::cacheLightFreeList);
LISTDECLEX(CMapLight, lameAssLink, CMap::lightFreeList);
LISTDECLEX(CMapBaseObjLink, ownerLink, CMap::baseObjLinkFreeList);
LISTDECLEX(CMapObj, lameAssLink, CMap::mapObjFreeList);
LISTDECLEX(CMapDoodadDef, lameAssLink, CMap::doodadDefFreeList);
LISTDECLEX(CMapEntity, lameAssLink, CMap::entityFreeList);
LISTDECLEX(CMapEntity, lameAssLink, CMap::entityList);
LISTDECLEX(CMapArea, lameAssLink, CMap::areaFreeList);
LISTDECLEX(CMapArea, lameAssLink, CMap::areaList);
LISTDECLEX(CMapChunk, lameAssLink, CMap::chunkFreeList);
TSHashTable<CMapDoodadDef, HASHKEY_DWORD>    CMap::doodadDefHash;
LISTDECLEX(CChunkLiquid, lameAssLink, CMap::chunkLiquidList);
LISTDECLEX(CChunkLiquid, lameAssLink, CMap::chunkLiquidFreeList);
LISTDECLEX(CMapSoundEmitter, lameAssLink, CMap::soundEmitterFreeList);
LISTDECLEX(CMapObjDefGroup, lameAssLink, CMap::mapObjDefGroupFreeList);
LISTDECLEX(CMapObjDefGroup, lameAssLink, CMap::mapObjDefGroupList);
LISTDECLEX(CMapObjDef, lameAssLink, CMap::mapObjDefFreeList);
LISTDECL(CChunkLayer, CMap::chunkLayerFreeList);
LISTDECL(CChunkTex, CMap::chunkTexFreeList);

CMapObj *CMap::AllocMapObj() {
  CMapObj *mapObj = mapObjFreeList.Head();
  if (!mapObj) {
    mapObj = NEWZERO(CMapObj);
    FATALASSERT(mapObj);
  } else {
    mapObj->lameAssLink.Unlink();
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
  if (!group) {
    group = NEWZERO(CMapObjGroup);
    FATALASSERT(group);
  } else {
    group->lameAssLink.Unlink();
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
    layer = NEWZERO(CChunkLayer);
    chunkLayerFreeList.LinkNode(layer, LIST_TAIL, 0);
    FATALASSERT(layer);
    ++freeCounts[3];
  }

  layer->Unlink();
  ++counts[3];
  --freeCounts[3];
  return layer;
}

void CMap::FreeLayer(CChunkLayer *layer) {
  ASSERT(layer);

  layer->Unlink();
  layer->~CChunkLayer();
  chunkLayerFreeList.LinkNode(layer, LIST_TAIL, 0);

  --counts[3];
  ++freeCounts[3];
}

CChunkTex *CMap::GetTex() {
  CChunkTex *tex = chunkTexFreeList.Head();
  if (!tex) {
    tex = NEWZERO(CChunkTex);
    chunkTexFreeList.LinkNode(tex, LIST_TAIL, 0);
    FATALASSERT(tex);
    ++freeCounts[4];
  }

  tex->Unlink();
  ++counts[4];
  --freeCounts[4];
  return tex;
}

void CMap::FreeTex(CChunkTex *tex) {
  ASSERT(tex);

  tex->Unlink();
  tex->~CChunkTex();
  chunkTexFreeList.LinkNode(tex, LIST_TAIL, 0);

  --counts[4];
  ++freeCounts[4];
}

CMapBaseObjLink *CMap::AllocBaseObjLink(CMapBaseObj *owner) {
  ASSERT(owner);

  CMapBaseObjLink *link = baseObjLinkFreeList.Head();
  if (!link) {
    link = NEWZERO(CMapBaseObjLink);
    baseObjLinkFreeList.LinkNode(link, LIST_TAIL, 0);
    ASSERT(link);
    ++freeCounts[9];
  }

  link->ownerLink.Unlink();
  ++owner->refCount;
  link->owner = owner;
  link->ref = 0;
  owner->parentLinkList.LinkNode(link, LIST_TAIL, 0);

  ++counts[9];
  --freeCounts[9];
  return link;
}

void CMap::FreeBaseObjLink(CMapBaseObjLink *link) {
  ASSERT(link);

  link->ownerLink.Unlink();
  link->refLink.Unlink();

  ASSERT(link->owner);
  --link->owner->refCount;

  link->ref = 0;
  link->owner = 0;
  baseObjLinkFreeList.LinkNode(link, LIST_TAIL, 0);

  --counts[9];
  ++freeCounts[9];
}

CMapArea *CMap::AllocArea() {
  CMapArea *area = areaFreeList.Head();
  if (!area) {
    area = NEWZERO(CMapArea);
    areaFreeList.LinkNode(area, LIST_TAIL, 0);
    FATALASSERT(area);
    ++freeCounts[0];
  }

  area->lameAssLink.Unlink();
  areaList.LinkNode(area, LIST_TAIL, 0);

  ++counts[0];
  --freeCounts[0];
  return area;
}

void CMap::FreeArea(CMapArea *area) {
  FATALASSERT(area);
  FATALASSERT(area->parentLinkList.Head() == 0);
  FATALASSERT(area->chunkLinkList.Head() == 0);
  FATALASSERT(area->texIdTable.Count() == 0);

  area->lameAssLink.Unlink();
  areaFreeList.LinkNode(area, LIST_TAIL, 0);

  --counts[0];
  ++freeCounts[0];
}

CMapChunk *CMap::AllocChunk() {
  CMapChunk *chunk = chunkFreeList.Head();
  if (!chunk) {
    chunk = NEWZERO(CMapChunk);
    chunkFreeList.LinkNode(chunk, LIST_TAIL, 0);
    FATALASSERT(chunk);
    ++freeCounts[2];
  }

  chunk->lameAssLink.Unlink();
  chunkList.LinkNode(chunk, LIST_TAIL, 0);

  ++counts[2];
  --freeCounts[2];
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

  --counts[2];
  ++freeCounts[2];
}

CMapDoodadDef *CMap::AllocDoodadDef() {
  CMapDoodadDef *doodadDef = doodadDefFreeList.Head();
  if (!doodadDef) {
    doodadDef = NEWZERO(CMapDoodadDef);
    doodadDefFreeList.LinkNode(doodadDef, LIST_TAIL, 0);
    FATALASSERT(doodadDef);
    ++freeCounts[1];
  }

  doodadDef->lameAssLink.Unlink();
  doodadDef->RenderCB = 0;
  doodadDef->model = 0;

  ++counts[1];
  --freeCounts[1];
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

  --counts[1];
  ++freeCounts[1];
}

CMapEntity *CMap::AllocEntity() {
  CMapEntity *entity = entityFreeList.Head();
  if (!entity) {
    entity = NEWZERO(CMapEntity);
    entityFreeList.LinkNode(entity, LIST_TAIL, 0);
    FATALASSERT(entity);
    ++freeCounts[7];
  }

  entity->lameAssLink.Unlink();
  entityList.LinkNode(entity, LIST_TAIL, 0);

  ++counts[7];
  --freeCounts[7];
  return entity;
}

void CMap::FreeEntity(CMapEntity *entity) {
  FATALASSERT(entity);
  FATALASSERT(entity->parentLinkList.Head() == 0);

  entity->lameAssLink.Unlink();
  entityFreeList.LinkNode(entity, LIST_TAIL, 0);

  --counts[7];
  ++freeCounts[7];
}

CMapLight *CMap::AllocLight() {
  CMapLight *light = lightFreeList.Head();
  if (!light) {
    light = NEWZERO(CMapLight);
    lightFreeList.LinkNode(light, LIST_TAIL, 0);
    ASSERT(light);
    ++freeCounts[8];
  }

  light->lameAssLink.Unlink();
  lightList.LinkNode(light, LIST_TAIL, 0);

  ++counts[8];
  --freeCounts[8];
  return light;
}

void CMap::FreeLight(CMapLight *light) {
  ASSERT(light);
  ASSERT(light->parentLinkList.Head() == 0);

  light->lameAssLink.Unlink();
  lightFreeList.LinkNode(light, LIST_TAIL, 0);

  --counts[8];
  ++freeCounts[8];
}

CMapCacheLight *CMap::AllocCacheLight() {
  CMapCacheLight *cacheLight = cacheLightFreeList.Head();
  if (!cacheLight) {
    cacheLight = NEWZERO(CMapCacheLight);
    cacheLightFreeList.LinkNode(cacheLight, LIST_TAIL, 0);
    ASSERT(cacheLight);
    ++freeCounts[10];
  }

  cacheLight->lameAssLink.Unlink();

  ++counts[10];
  --freeCounts[10];
  return cacheLight;
}

void CMap::FreeCacheLight(CMapCacheLight *cacheLight) {
  ASSERT(cacheLight);

  cacheLight->lameAssLink.Unlink();
  cacheLightFreeList.LinkNode(cacheLight, LIST_TAIL, 0);

  --counts[10];
  ++freeCounts[10];
}

CMapObjDefGroup *CMap::AllocMapObjDefGroup() {
  CMapObjDefGroup *mapObjDefGroup = mapObjDefGroupFreeList.Head();
  if (!mapObjDefGroup) {
    mapObjDefGroup = NEWZERO(CMapObjDefGroup);
    mapObjDefGroupFreeList.LinkNode(mapObjDefGroup, LIST_TAIL, 0);
    FATALASSERT(mapObjDefGroup);
    ++freeCounts[6];
  }

  mapObjDefGroup->lameAssLink.Unlink();
  mapObjDefGroupList.LinkNode(mapObjDefGroup, LIST_TAIL, 0);

  ++counts[6];
  --freeCounts[6];
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

  --counts[6];
  ++freeCounts[6];
}

CMapObjDef *CMap::AllocMapObjDef() {
  CMapObjDef *mapObjDef = mapObjDefFreeList.Head();
  if (!mapObjDef) {
    mapObjDef = NEWZERO(CMapObjDef);
    mapObjDefFreeList.LinkNode(mapObjDef, LIST_TAIL, 0);
    FATALASSERT(mapObjDef);
    ++freeCounts[5];
  }

  mapObjDef->lameAssLink.Unlink();

  ++counts[5];
  --freeCounts[5];
  return mapObjDef;
}

void CMap::FreeMapObjDef(CMapObjDef *mapObjDef) {
  FATALASSERT(mapObjDef);
  FATALASSERT(mapObjDef->parentLinkList.Head() == 0);
  FATALASSERT(mapObjDef->groupLinkList.Head() == 0);

  mapObjDef->lameAssLink.Unlink();
  mapObjDefHash.Unlink(mapObjDef);
  mapObjDefFreeList.LinkNode(mapObjDef, LIST_TAIL, 0);

  --counts[5];
  ++freeCounts[5];
}

CChunkLiquid *CMap::AllocChunkLiquid() {
  CChunkLiquid *liquid = chunkLiquidFreeList.Head();
  if (!liquid) {
    liquid = NEWZERO(CChunkLiquid);
    chunkLiquidFreeList.LinkNode(liquid, LIST_TAIL, 0);
  }

  liquid->lameAssLink.Unlink();
  chunkLiquidList.LinkNode(liquid, LIST_TAIL, 0);
  return liquid;
}

void CMap::FreeChunkLiquid(CChunkLiquid *&cl) {
  FATALASSERT(cl);

  chunkLiquidList.UnlinkNode(cl);
  chunkLiquidFreeList.LinkNode(cl, LIST_TAIL, 0);
  cl = 0;
}

CMapSoundEmitter *CMap::AllocSoundEmitter() {
  CMapSoundEmitter *soundEmitter = soundEmitterFreeList.Head();
  if (!soundEmitter) {
    soundEmitter = NEWZERO(CMapSoundEmitter);
    soundEmitterFreeList.LinkNode(soundEmitter, LIST_TAIL, 0);
    FATALASSERT(soundEmitter);
  }

  soundEmitter->lameAssLink.Unlink();
  return soundEmitter;
}

void CMap::FreeSoundEmitter(CMapSoundEmitter *soundEmitter) {
  FATALASSERT(soundEmitter);

  soundEmitterFreeList.UnlinkNode(soundEmitter);
  soundEmitterFreeList.LinkNode(soundEmitter, LIST_TAIL, 0);
}
