#include "WorldClient/World.h"

#include "DayNight.h"
#include "SoundInterface/SoundInterface.h"
#include "WorldCommon/WorldMath.h"
#include "WorldClient/CMapObj.h"

#include <Model/IModel.h>
#include <Model/CollisionData.h>
#include <Base/Status.h>
#include <Services/AsyncFileRead.h>
#include <Services/SysMessage.h>
#include <Services/Texture.h>
#include <storm.h>

#include <float.h>

static const char *s_animationNames[1] = {"Stand"};

static int __fastcall OnPickNextFidget(void *param) {
  ModelSetRandomSequenceFidget(static_cast<HMODEL>(param), 0, 0);
  return 1;
}

static void __fastcall DoodadEventCallback(const char *eventName, const NTempest::C3Vector &position, void *param) {
  CMapDoodadDef *doodadDef = static_cast<CMapDoodadDef *>(param);
  unsigned int   event = *reinterpret_cast<const unsigned int *>(eventName);

  if (event == 0x4C534424) {
    if (!doodadDef->doodadSoundHandle) {
      doodadDef->doodadSoundHandle = SndInterfaceHandleDoodadLoopStart(SStrToUnsigned(eventName + 4), position);
    }
  } else if (event == 0x4F534424) {
    SndInterfaceHandleDoodadOneShot(SStrToUnsigned(eventName + 4), position);
  }
}

void __fastcall CMap::Load(const char *fileName) {
  char lightPath[256];

  enablePixelShaders = (CWorld::enables & CWorld::Enable_PixelShaders) != 0;
  enableSpecular = (CWorld::enables & CWorld::Enable_Specular) != 0 && enablePixelShaders;
  enableSpecularTerrain = enableSpecular && psSpecTerrain->Valid() && psSpecUTerrain->Valid();
  enableSpecularWater = enableSpecular && psOcean0->Valid();
  enableTerrainShader = enablePixelShaders && psTerrain->Valid() && psUTerrain->Valid();

  SStrCopy(mapPath + SStrCopy(mapPath, "World\\Maps\\", 0x7FFFFFFF), fileName, 0x7FFFFFFF);
  SStrCopy(mapName, fileName, 0x7FFFFFFF);

  sunLight = CreateLight(true);
  EnableLight(sunLight);
  UpdateLight(sunLight);

  Purge();
  FATALASSERT(doodadDefLinkList.Head() == 0);
  FATALASSERT(mapObjDefLinkList.Head() == 0);
  CMapObj::ClearCache(1);
  bActive = 1;
  bDungeon = 0;
  bPreload = 1;

  LoadWdl();

  FATALASSERT(!wdtFile);
  SStrPrintf(wdtFilename, sizeof(wdtFilename), "%s\\%s.wdt", mapPath, mapName);
  SFile::Open(wdtFilename, &wdtFile);
  FATALASSERT(wdtFile);

  LoadWdt();
  PrepareUpdate();
  AsyncFileReadWaitAll();

  SStrPrintf(lightPath, 255, "%s\\%s", mapPath, "lights.lit");
  DayNightInitialize(lightPath);
  bPreload = 0;
}

void __fastcall CMap::LoadWdl() {
  short         heights[545];
  char          wdlFilename[256];
  unsigned long version;
  SIffChunk     iffChunk;
  unsigned int  index;
  float         min;
  SFile        *wdlFile;
  float         temp;

  SStrPrintf(wdlFilename, sizeof(wdlFilename), "%s\\%s.wdl", mapPath, mapName);
  SFile::Open(wdlFilename, &wdlFile);
  if (!wdlFile) {
    return;
  }

  SFile::Read(wdlFile, &iffChunk, sizeof(iffChunk), 0, 0, 0);
  FATALASSERT(iffChunk.token == 'MVER');
  SFile::Read(wdlFile, &version, sizeof(version), 0, 0, 0);
  FATALASSERT(version == 0x0012);
  SFile::Read(wdlFile, &iffChunk, sizeof(iffChunk), 0, 0, 0);
  FATALASSERT(iffChunk.token == 'MAOF');
  SFile::Read(wdlFile, areaLowOffsets, sizeof(areaLowOffsets), 0, 0, 0);

  index = 0;
  for (unsigned int y = 0; y < 64 * 16; y += 16) {
    for (unsigned int x = 0; x < 64 * 16; x += 16) {
      if (areaLowOffsets[index]) {
        SFile::SetFilePointer(wdlFile, areaLowOffsets[index], 0, FILE_BEGIN);
        SFile::Read(wdlFile, &iffChunk, sizeof(iffChunk), 0, 0, 0);
        FATALASSERT(iffChunk.token == 'MARE');

        CMapAreaLow *mapAreaLow = new CMapAreaLow;
        FATALASSERT(mapAreaLow);
        areaLowTable[index] = mapAreaLow;
        SFile::Read(wdlFile, heights, sizeof(heights), 0, 0, 0);

        min = FLT_MAX;
        temp = -FLT_MAX;
        for (unsigned int h = 0; h < 545; ++h) {
          mapAreaLow->heights[h] = static_cast<float>(heights[h]);
          if (mapAreaLow->heights[h] < min) {
            min = mapAreaLow->heights[h];
          }
          if (mapAreaLow->heights[h] > temp) {
            temp = mapAreaLow->heights[h];
          }
        }

        mapAreaLow->corner.x = 17066.666f - static_cast<float>(y) * 33.333332f;
        mapAreaLow->corner.y = 17066.666f - static_cast<float>(x) * 33.333332f;
        mapAreaLow->aaBox.b.x = mapAreaLow->corner.x - 533.33331f;
        mapAreaLow->aaBox.b.y = mapAreaLow->corner.y - 533.33331f;
        mapAreaLow->aaBox.b.z = min;
        mapAreaLow->aaBox.t.x = mapAreaLow->corner.x;
        mapAreaLow->aaBox.t.y = mapAreaLow->corner.y;
        mapAreaLow->aaBox.t.z = temp;
        mapAreaLow->aaSphere.c = (mapAreaLow->aaBox.b + mapAreaLow->aaBox.t) * 0.5f;
        mapAreaLow->aaSphere.r = (mapAreaLow->aaBox.t - mapAreaLow->aaSphere.c).Mag();
      }

      ++index;
    }
  }

  SFile::Close(wdlFile);
}

void __fastcall CMap::LoadWdt() {
  SMMapObjDef        smMapObjDef;
  NTempest::C3Vector pos;
  SIffChunk          iffChunk;

  SFile::Read(wdtFile, &iffChunk, sizeof(iffChunk), 0, 0, 0);
  FATALASSERT(iffChunk.token == 'MVER');
  SFile::Read(wdtFile, &version, sizeof(version), 0, 0, 0);

  SFile::Read(wdtFile, &iffChunk, sizeof(iffChunk), 0, 0, 0);
  FATALASSERT(iffChunk.token == 'MPHD');
  SFile::Read(wdtFile, &header, 0x80, 0, 0, 0);

  SFile::Read(wdtFile, &iffChunk, sizeof(iffChunk), 0, 0, 0);
  FATALASSERT(iffChunk.token == 'MAIN');
  SFile::Read(wdtFile, areaInfo, sizeof(areaInfo), 0, 0, 0);

  LoadDoodadNames();
  LoadMapObjNames();

  SFile::Read(wdtFile, &iffChunk, sizeof(iffChunk), 0, 0, 0);
  if (iffChunk.token == 'MODF') {
    SFile::Read(wdtFile, &smMapObjDef, sizeof(smMapObjDef), 0, 0, 0);
    smMapObjDef.uniqueId = uniqueId--;
    pos.Set(0.0f, 0.0f, 0.0f);

    CMapObjDef      *mapObjDef = CreateMapObjDef(smMapObjDef, pos);
    CMapBaseObjLink *link = AllocBaseObjLink(mapObjDef);
    link->ref = 0;
    mapObjDefLinkList.LinkNode(link, LIST_TAIL, 0);
    bDungeon = 1;
  }
}

void __fastcall CMap::Preload() {
  Purge();
  CMapObj::ClearCache(0);
  ModelCacheFlush();
  TextureCacheFlush();
  bPreload = 1;
  PrepareUpdate();
  AsyncFileReadWaitAll();
  bPreload = 0;
}

void __fastcall CMap::Open() {
}

void __fastcall CMap::LoadDoodadNames() {
  SIffChunk     iffChunk;
  unsigned long bRead;
  unsigned int  cnt;

  SFile::Read(wdtFile, &iffChunk, sizeof(iffChunk), 0, 0, 0);
  FATALASSERT(iffChunk.token == 'MDNM');

  if (iffChunk.size) {
    doodadNames.SetCount(iffChunk.size);
    doodadNamesIndex.SetCount(header.nDoodadNames);
    SFile::Read(wdtFile, doodadNames.Ptr(), iffChunk.size, &bRead, 0, 0);

    cnt = 0;
    for (unsigned int i = 0; i < doodadNames.Count(); ++i) {
      doodadNamesIndex[cnt++] = i;
      while (doodadNames.Ptr()[i]) {
        ++i;
      }
    }
  }
}

void __fastcall CMap::LoadMapObjNames() {
  SIffChunk     iffChunk;
  unsigned long bRead;
  unsigned int  cnt;

  SFile::Read(wdtFile, &iffChunk, sizeof(iffChunk), 0, 0, 0);
  FATALASSERT(iffChunk.token == 'MONM');

  mapObjNames.SetCount(iffChunk.size);
  mapObjNamesIndex.SetCount(header.nMapObjNames);
  if (iffChunk.size) {
    SFile::Read(wdtFile, mapObjNames.Ptr(), iffChunk.size, &bRead, 0, 0);

    cnt = 0;
    for (unsigned int i = 0; i < mapObjNames.Count(); ++i) {
      mapObjNamesIndex[cnt++] = i;
      while (mapObjNames.Ptr()[i]) {
        ++i;
      }
    }
  }
}

CMapDoodadDef *__fastcall CMap::CreateDoodadDef(SMDoodadDef &smDoodadDef, NTempest::C3Vector &pos) {
  HASHKEY_DWORD  key;
  CMapDoodadDef *doodadDef = doodadDefHash.Ptr(smDoodadDef.uniqueId, key);
  while (doodadDef) {
    if (doodadDef->m_hashval == smDoodadDef.uniqueId && doodadDef->m_key == key) {
      return doodadDef;
    }
    doodadDef = doodadDef->m_linktoslot.Next();
  }

  doodadDef = AllocDoodadDef();
  FATALASSERT(doodadDef);
  doodadDefHash.Insert(doodadDef, smDoodadDef.uniqueId, key);

  doodadDef->pos.Set(-smDoodadDef.pos.z, -smDoodadDef.pos.x, smDoodadDef.pos.y);
  doodadDef->pos += pos;
  doodadDef->corner = doodadDef->pos;
  doodadDef->scale = static_cast<float>(smDoodadDef.scale) * 0.0009765625f;
  doodadDef->aaBox.b = doodadDef->pos;
  doodadDef->aaBox.t = doodadDef->pos;
  doodadDef->aaSphere.c = doodadDef->pos;
  doodadDef->aaSphere.r = 0.0f;
  doodadDef->flags = CMapBaseObj::Flag_LightUpdate;

  doodadDef->modelName = &doodadNames[doodadNamesIndex[smDoodadDef.nameId]];
  doodadDef->model = 0;

  NTempest::C3Vector rot(smDoodadDef.rot.x * 0.017453292f, smDoodadDef.rot.y * 0.017453292f, smDoodadDef.rot.z * 0.017453292f + 3.1415927f);
  doodadDef->mat = NTempest::C44Matrix();
  doodadDef->mat.Translate(doodadDef->pos);
  doodadDef->mat.Rotate(rot.z, NTempest::C3Vector(0.0f, 0.0f, 1.0f), 1);
  doodadDef->mat.Rotate(rot.y, NTempest::C3Vector(0.0f, 1.0f, 0.0f), 1);
  doodadDef->mat.Rotate(rot.x, NTempest::C3Vector(1.0f, 0.0f, 0.0f), 1);
  doodadDef->mat.Scale(doodadDef->scale);
  doodadDef->lMat = NTempest::C44Matrix();

  return doodadDef;
}

CMapDoodadDef *__fastcall CMap::CreateDoodadDef(
    unsigned int         doodadRef,
    SMODoodadDef        &smoDoodadDef,
    const char          *fileName,
    unsigned int         mapObjDefId,
    NTempest::C44Matrix &mapObjDefMat
) {
  HASHKEY_DWORD  key(mapObjDefId);
  CMapDoodadDef *doodadDef = doodadDefHash.Ptr(doodadRef, key);
  while (doodadDef) {
    if (doodadDef->m_hashval == doodadRef && doodadDef->m_key == key) {
      return doodadDef;
    }
    doodadDef = doodadDef->m_linktoslot.Next();
  }

  doodadDef = AllocDoodadDef();
  FATALASSERT(doodadDef);
  doodadDefHash.Insert(doodadDef, doodadRef, key);

  doodadDef->pos = smoDoodadDef.pos * mapObjDefMat;
  doodadDef->scale = smoDoodadDef.scale;
  doodadDef->corner = doodadDef->pos;
  doodadDef->aaBox.b = doodadDef->pos;
  doodadDef->aaBox.t = doodadDef->pos;
  doodadDef->aaSphere.c = doodadDef->pos;
  doodadDef->aaSphere.r = 0.0f;
  doodadDef->flags = CMapBaseObj::Flag_LightUpdate;
  doodadDef->model = 0;
  doodadDef->modelName = fileName;
  doodadDef->ambient = NTempest::CImVector(0);
  doodadDef->interiorDirColor = NTempest::CImVector(0);
  doodadDef->dirLightScale = 1.0f;

  doodadDef->mat = NTempest::C44Matrix();
  doodadDef->mat.Translate(smoDoodadDef.pos);
  doodadDef->mat.Rotate(smoDoodadDef.rot);
  doodadDef->mat.Scale(smoDoodadDef.scale);
  doodadDef->lMat = doodadDef->mat;
  doodadDef->mat *= mapObjDefMat;
  doodadDef->AdjustLightmap(smoDoodadDef.color, doodadDef->interiorDirColor, 112, doodadDef->ambient, 96);
  return doodadDef;
}

CMapObjDef *__fastcall CMap::CreateMapObjDef(SMMapObjDef &smMapObjDef, NTempest::C3Vector &pos) {
  CMapObjDef *mapObjDef = mapObjDefHash.Ptr(smMapObjDef.uniqueId, nullHashKey);
  if (mapObjDef) {
    return mapObjDef;
  }

  mapObjDef = AllocMapObjDef();
  FATALASSERT(mapObjDef);
  mapObjDefHash.Insert(mapObjDef, smMapObjDef.uniqueId, nullHashKey);

  mapObjDef->pos.Set(-smMapObjDef.pos.z, -smMapObjDef.pos.x, smMapObjDef.pos.y);
  mapObjDef->pos += pos;

  NTempest::C3Vector rot(
      smMapObjDef.rot.z * 0.017453292f,
      smMapObjDef.rot.x * 0.017453292f,
      smMapObjDef.rot.y * 0.017453292f + 3.1415927f
  );
  mapObjDef->flags = 0;
  mapObjDef->nameId = smMapObjDef.nameId;
  mapObjDef->doodadSet = smMapObjDef.doodadSet;
  mapObjDef->nameSet = smMapObjDef.nameSet;
  mapObjDef->zoneName = 0;
  mapObjDef->param64 = 0;

  mapObjDef->mat = NTempest::C44Matrix();
  mapObjDef->mat.Translate(mapObjDef->pos);
  mapObjDef->mat.Rotate(rot.z, NTempest::C3Vector(0.0f, 0.0f, 1.0f), 1);
  mapObjDef->mat.Rotate(rot.y, NTempest::C3Vector(0.0f, 1.0f, 0.0f), 1);
  mapObjDef->mat.Rotate(rot.x, NTempest::C3Vector(1.0f, 0.0f, 0.0f), 1);
  mapObjDef->invMat = mapObjDef->mat.AffineInverse();

  mapObjDef->aaBox.b.Set(-smMapObjDef.extents.t.z + pos.x, -smMapObjDef.extents.t.x + pos.y, smMapObjDef.extents.b.y + pos.z);
  mapObjDef->aaBox.t.Set(-smMapObjDef.extents.b.z + pos.x, -smMapObjDef.extents.b.x + pos.y, smMapObjDef.extents.t.y + pos.z);
  mapObjDef->aaSphere.c = (mapObjDef->aaBox.b + mapObjDef->aaBox.t) * 0.5f;
  mapObjDef->aaSphere.r = (mapObjDef->aaBox.t - mapObjDef->aaSphere.c).Mag();

  mapObjDef->lightList.SetCount(0);
  mapObjDef->mapObj = CMapObj::Create(&mapObjNames[mapObjNamesIndex[smMapObjDef.nameId]]);
  return mapObjDef;
}

void __fastcall CMap::InitializeDoodadBounds(CMapDoodadDef *doodadDef) {
  NTempest::CAaBox    localCollExtents;
  NTempest::CAaBox    localExtents;
  NTempest::CAaSphere localSphere;

  if (doodadDef->model) {
    ModelGetExtents(doodadDef->model, &localExtents);
    ModelGetBounds(doodadDef->model, &localSphere);
    ModelGetCollisionExtents(doodadDef->model, &localCollExtents);
  }

  doodadDef->aaSphere.c = localSphere.c * doodadDef->mat;
  doodadDef->aaSphere.r = localSphere.r * doodadDef->scale;
  CWorldMath::TransformAABox(doodadDef->mat, localExtents, doodadDef->aaBox);
  CWorldMath::TransformAABox(doodadDef->mat, localCollExtents, doodadDef->collideExt);
}

int __fastcall CMap::LoadDoodadModel(CMapDoodadDef *doodadDef, int bWait) {
  FATALASSERT(doodadDef);

  CModelCreate createData;
  CStatus      status;

  memset(&createData, 0, sizeof(createData));
  createData.flags = 0x2802;
  createData.sequenceNames = s_animationNames;
  createData.numSequences = 1;

  if (CWorld::enables & CWorld::Enable_NoFullAlpha) {
    createData.flags = 0x2882;
  }
  if (CWorld::enables & CWorld::Enable_NoAnimation) {
    createData.flags |= 0x100;
  }
  if (bPreload || bWait) {
    createData.flags |= 0x8000;
  }
  if (CWorld::enables & CWorld::Enable_Anisotropic) {
    createData.flags |= (CWorld::texMaxAnisotropyLog2 << 17) | 0x10000;
  } else if (CWorld::enables & CWorld::Enable_Trilinear) {
    createData.flags |= 0x1000;
  }

  doodadDef->model = ModelCreate(doodadDef->modelName, &createData, &status);
  FATALASSERT(doodadDef->model);

  ModelSetRandomSequenceFidget(doodadDef->model, 0, 0);
  ModelSetSeqFinishedHandler(doodadDef->model, OnPickNextFidget, doodadDef->model);
  ModelSetEventCallback(doodadDef->model, DoodadEventCallback, doodadDef, 0);
  ModelSetLightSelectCallback(doodadDef->model, SelectLight, doodadDef, 1);

  if (bPreload || ModelIsLoaded(doodadDef->model, 1)) {
    InitializeDoodadBounds(doodadDef);
  }

  return 1;
}

void __fastcall CMap::ReloadDoodadModels() {
  bPreload = 1;

  CMapDoodadDef *doodadDef = doodadDefHash.Head();
  while (doodadDef) {
    ModelRemoveFromCache(doodadDef->modelName);
    doodadDef = doodadDefHash.Next(doodadDef);
  }

  doodadDef = doodadDefHash.Head();
  while (doodadDef) {
    if (doodadDef->model) {
      HandleClose(doodadDef->model);
    }
    doodadDef->model = 0;
    LoadDoodadModel(doodadDef, 0);
    doodadDef = doodadDefHash.Next(doodadDef);
  }

  bPreload = 0;
}

void __fastcall CMap::EnableDoodadFullAlpha(int enable) {
  CMapDoodadDef *doodadDef = doodadDefHash.Head();
  while (doodadDef) {
    if (doodadDef->model) {
      ModelEnableFullAlpha(doodadDef->model, enable);
    }
    doodadDef = doodadDefHash.Next(doodadDef);
  }
}

void __fastcall CMap::CreateMapObjDefGroups(CMapObj *mapObj, CMapObjDef *mapObjDef) {
  FATALASSERT(mapObj);
  FATALASSERT(mapObjDef);

  for (unsigned int groupNum = 0; groupNum < mapObj->groupCount; ++groupNum) {
    CMapObjDefGroup *mapObjDefGroup = AllocMapObjDefGroup();
    CMapBaseObjLink *link = AllocBaseObjLink(mapObjDefGroup);
    link->ref = mapObjDef;
    mapObjDef->groupLinkList.LinkNode(link, LIST_TAIL, 0);

    NTempest::CAaBox aaBox;
    mapObj->GetGroupBounds(mapObjDefGroup->aaSphere, groupNum);
    mapObjDefGroup->aaSphere.c *= mapObjDef->mat;
    mapObj->GetGroupBounds(aaBox, groupNum);
    CWorldMath::TransformAABox(mapObjDef->mat, aaBox, mapObjDefGroup->aaBox);
    FATALASSERT(mapObjDefGroup->aaBox.b != mapObjDefGroup->aaBox.t);

    mapObjDefGroup->groupNum = groupNum;
    mapObjDefGroup->ambient = mapObjDef->ambient;
    mapObjDefGroup->flags = 0;
    if (mapObj->GetGroupFlags(groupNum) & 0x48) {
      mapObjDefGroup->flags |= CMapBaseObj::Flag_ExteriorLit;
    } else {
      mapObjDefGroup->flags |= CMapBaseObj::Flag_InteriorLit;
    }
  }
}

void __fastcall CMap::CreateMapObjDefGroupDoodads(
    CMapObj         *mapObj,
    CMapObjGroup    *mapObjGroup,
    CMapObjDef      *mapObjDef,
    CMapObjDefGroup *mapObjDefGroup
) {
  FATALASSERT(mapObj);
  FATALASSERT(mapObjGroup);
  FATALASSERT(mapObjDef);
  FATALASSERT(mapObjDefGroup);

  for (unsigned int i = 0; i < mapObjGroup->doodadRefCount; ++i) {
    unsigned int doodadRef = mapObjGroup->doodadRefList[i];
    unsigned int doodadSet = mapObj->GetDoodadSet(doodadRef);
    if (doodadSet && doodadSet != mapObjDef->doodadSet) {
      continue;
    }

    SMODoodadDef  &smoDoodadDef = mapObj->doodadDefList[doodadRef];
    CMapDoodadDef *doodadDef =
        CreateDoodadDef(doodadRef, smoDoodadDef, mapObj->doodadNameList + smoDoodadDef.nameIndex, mapObjDef->m_hashval + 1, mapObjDef->mat);
    if (doodadDef) {
      CMapBaseObjLink *link = AllocBaseObjLink(doodadDef);
      link->ref = mapObjDefGroup;
      mapObjDefGroup->doodadDefLinkList.LinkNode(link, LIST_TAIL, 0);
      if (mapObjDefGroup->flags & CMapBaseObj::Flag_InteriorLit) {
        doodadDef->flags |= CMapBaseObj::Flag_InteriorLit;
      } else {
        doodadDef->dirLightScale = 1.0f;
      }
    }
  }
  mapObjDefGroup->flags |= CMapBaseObj::Flag_HasDoodadRefs;
}

void __fastcall CMap::CreateMapObjDefLights(CMapObj *mapObj, CMapObjGroup *mapObjGroup, CMapObjDef *mapObjDef, CMapObjDefGroup *mapObjDefGroup) {
  FATALASSERT(mapObj);
  FATALASSERT(mapObjGroup);
  FATALASSERT(mapObjDef);
  FATALASSERT(mapObjDefGroup);

  for (unsigned int i = 0; i < mapObjGroup->lightRefCount; ++i) {
    unsigned int idx = mapObjGroup->lightRefList[i];
    SMOLight    *sLight = &mapObj->lightList[idx];
    if (sLight->type) {
      continue;
    }

    CMapLight *light = mapObjDef->lightList[idx];
    if (!light) {
      light = CreateLight(false);
      mapObjDef->lightList[idx] = light;
      NTempest::C3Vector lightPos = sLight->position * mapObjDef->mat;
      light->attenStart = sLight->attenStart;
      light->attenEnd = sLight->attenEnd;
      light->attenDenom = 1.0f / (sLight->attenEnd - sLight->attenStart);
      light->gxLight.m_enabled = 1;
      light->gxLight.m_isOmni = 1;
      light->gxLight.m_dir = lightPos;
      light->gxLight.m_ambColor = sLight->color;
      light->gxLight.m_ambIntensity = sLight->intensity;
      light->gxLight.m_constantAttenuation = 0.0f;
      light->gxLight.m_linearAttenuation = 0.7f;
    }

    if (mapObjDefGroup->flags & CMapBaseObj::Flag_InteriorLit) {
      UpdateLightBounds(light);
    } else {
      UpdateLight(light);
    }
    CMapBaseObjLink *link = AllocBaseObjLink(light);
    link->ref = mapObjDefGroup;
    mapObjDefGroup->lightLinkList.LinkNode(link, LIST_TAIL, 0);
  }
  mapObjDefGroup->flags |= CMapBaseObj::Flag_HasLights;
}

void DNPlanet::Initialize(const char *filename) {
  CStatus     status;
  CGxTexFlags flags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
  m_texid = TextureCreate(filename, flags, &status, 0);
  SysMsgAdd(status, 2);
}

void DNStars::Initialize() {
  CStatus status;
  m_hModel = ModelCreate("Environments\\Stars\\stars.mdl", 0, &status);
  if (m_hModel) {
    ModelSetSequence(m_hModel, 0, 0);
  }
  SysMsgAdd(status, 16);
}

void DNGlare::Initialize(const char *filename) {
  CStatus     status;
  CGxTexFlags flags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
  m_texid = TextureCreate(filename, flags, &status, 0);
  SysMsgAdd(status, 2);
}
