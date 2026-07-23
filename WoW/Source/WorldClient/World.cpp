#include "World.h"

#include "CMapObj.h"
#include "WorldParam.h"
#include "WorldCommon/WorldMath.h"

#include "DayNight.h"

#include <Base/Activity.h>
#include <Base/Handle.h>
#include <Anim/AnimTypes.h>
#include <Console/ConsoleClient.h>
#include <Console/ConsoleVar.h>
#include <Gx/Gx.h>
#include <Model/IModel.h>
#include <Os/OsTime.h>
#include <Services/IParticleMisc.h>
#include <Services/Texture.h>
#include <Tempest/c33matrix.h>
#include <Tempest/cmath.h>

#include <stdio.h>

NTempest::C44Matrix CWTriData::matrices[CWTriData::MaxBatches];
unsigned short      CWTriData::vertexIndices[CWTriData::MaxVertexIndices];
unsigned short      CWTriData::triIndices[CWTriData::MaxTriIndices];
CWTriData::Batch    CWTriData::batches[CWTriData::MaxBatches];
unsigned int        CWTriData::nMatrices;
unsigned int        CWTriData::nVertexIndices;
unsigned int        CWTriData::nTriIndices;
unsigned int        CWTriData::nBatches;

unsigned int        CWorld::frameCnt;
unsigned int        CWorld::chunkCnt;
NTempest::CiRect    CWorld::chunkRectHi;
NTempest::CiRect    CWorld::gbChunkRect;
NTempest::CiRect    CWorld::areaRect;
NTempest::C2iVector CWorld::chunkAoiSize;
int                 CWorld::prepareAll;
NTempest::C44Matrix CWorld::idMat;
NTempest::C4Vector  CWorld::texVect[8];
float               CWorld::detailDoodadDist = 100.0f;
float               CWorld::detailDoodadDistS;
unsigned int        CWorld::detailDoodadDensity;
int                 CWorld::detailDoodadTest;
unsigned int        CWorld::detailDoodadAlphaRef;
float               CWorld::textureLodDist = 777.0f;
float               CWorld::lodDist = 77.0f;
unsigned int        CWorld::lodMax = 3;
unsigned int        CWorld::lodMin = 2;
unsigned int        CWorld::pnEstimateVertex;
unsigned int        CWorld::pnEstimateIndex;
unsigned int        CWorld::pnt0EstimateVertex;
unsigned int        CWorld::pnt0EstimateIndex;
unsigned int        CWorld::pnct0EstimateVertex;
unsigned int        CWorld::pnct0EstimateIndex;
float               CWorld::farFog;
float               CWorld::farClip;
float               CWorld::nearClip;
float               CWorld::unitDrawDist;
NTempest::CAaBox    CWorld::groupAoi;
NTempest::CAaBox    CWorld::objectAoi;
unsigned long       CWorld::enables;
float               CWorld::curTimeSec;
float               CWorld::tickTimeSec;
unsigned int        CWorld::curTimeMs;
unsigned int        CWorld::tickTimeMs;
unsigned long       CWorld::enableLayerCnt;
unsigned int        CWorld::maxLights;
NTempest::CImVector CWorld::shadowColor;
unsigned int        CWorld::shadowModColor[64];
CGxTex             *CWorld::shadowModGxTex;
unsigned int        CWorld::shadowMipLevel;
unsigned int        CWorld::alphaMipLevel;
float               CWorld::texLodBias;
unsigned int        CWorld::texMaxAnisotropy;
unsigned int        CWorld::texMaxAnisotropyLog2;
Particulate        *CWorld::particulate;
int                 CWorld::bLoadSimpleDoodads;
int                 CWorld::bShowSimpleDoodads;

static float profTimes[30];
static int   profIdx;
static float s_texDir[8][2] = {
    {-1.0f,  0.0f},
    {-1.0f,  1.0f},
    { 0.0f,  1.0f},
    { 1.0f,  1.0f},
    { 1.0f,  0.0f},
    { 1.0f, -1.0f},
    { 0.0f, -1.0f},
    {-1.0f, -1.0f}
};

static void __fastcall UpdateShadowGxTex(
    EGxTexCommand cmd,
    unsigned int  w,
    unsigned int  h,
    unsigned int  d,
    unsigned int  mipLevel,
    void         *userArg,
    unsigned int &texelStrideInBytes,
    const void  *&texels
);

void __fastcall CWorld::Initialize() {
  enables |= 0x07100B73;
  frameCnt = 0;
  chunkCnt = 0;
  enableLayerCnt = 4;
  prepareAll = 0;
  bShowSimpleDoodads = 0;
  bLoadSimpleDoodads = 0;
  shadowColor = 0xFFFFFFFF;

  for (unsigned int i = 0; i < 8; ++i) {
    texVect[i] = NTempest::C4Vector(0.0f, 0.0f, 0.0f, 1.0f);
  }

  memset(shadowModColor, 0xFF, sizeof(shadowModColor));
  idMat = NTempest::C44Matrix();
  shadowModGxTex = 0;

  CGxTexFlags flags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
  GxTexCreate(8, 8, GxTex_Argb8888, flags, shadowModColor, UpdateShadowGxTex, shadowModGxTex);

  detailDoodadAlphaRef = 128;
  detailDoodadTest = 0;
  detailDoodadDistS = detailDoodadDist * detailDoodadDist;
  FATALASSERT(farClip != 0.0f);

  CWorldScene::Initialize();
  CMap::Initialize();

  shadowMipLevel = CWorldParam::cvar_shadowLevel->GetInt();
  alphaMipLevel = 1;
  farFog = CWorldParam::cvar_farClip->GetFloat();
  groupAoi = NTempest::CAaBox(0.0f);
  lodMax = 3;
  lodMin = 2;
  objectAoi = NTempest::CAaBox(0.0f);

  particulate = new Particulate(1.0f / 36.0f, 30.0f, "Textures\\WaterPoop02.blp");

  ModelSetProject2dCallback(ModelGeoProjectCallback);
  AnimSetBoneProjectCallback(AnimBoneProjectCallback, 20.0f);
  ParticleSystemManager::SetProjectCallback(ParticleProjectCallback, 20.0f);
}

void __fastcall CWorld::Destroy() {
  CWorldParam::Destroy();
  CMap::Destroy();
  CWorldScene::Destroy();

  if (shadowModGxTex) {
    GxTexDestroy(shadowModGxTex);
  }
  shadowModGxTex = 0;

  delete particulate;
}

void __fastcall CWorld::LoadMap(const char *mapName, NTempest::C3Vector &position, int preLoad) {
  FATALASSERT(mapName);

  PrepareAreaOfInterest(position, position);
  CMap::Load(mapName);
}

void __fastcall CWorld::UnloadMap() {
  CMap::Unload();
}

void __fastcall CWorld::PrepareUpdate(NTempest::C3Vector &position, NTempest::C3Vector &target) {
  ActivityBegin(ACTIVITY_WORLD);

  NTempest::CiRect oldGbChunkRect = gbChunkRect;
  PrepareAreaOfInterest(position, target);
  if (oldGbChunkRect.maxy >= gbChunkRect.maxy) {
    oldGbChunkRect.maxy = gbChunkRect.maxy;
  }
  if (oldGbChunkRect.maxx >= gbChunkRect.maxx) {
    oldGbChunkRect.maxx = gbChunkRect.maxx;
  }
  if (oldGbChunkRect.miny <= gbChunkRect.miny) {
    oldGbChunkRect.miny = gbChunkRect.miny;
  }
  if (oldGbChunkRect.minx <= gbChunkRect.minx) {
    oldGbChunkRect.minx = gbChunkRect.minx;
  }
  if (oldGbChunkRect.minx >= oldGbChunkRect.maxx || oldGbChunkRect.miny >= oldGbChunkRect.maxy) {
    prepareAll = 1;
  }

  ++frameCnt;
  CWorldScene::PrepareRender(position, target);
  CMap::PrepareUpdate();
  ActivityEnd(ACTIVITY_WORLD);
}

void __fastcall CWorld::SetUpdateTime(float elapsedSec, unsigned long pCurTimeMs) {
  curTimeMs = pCurTimeMs;
  tickTimeMs = static_cast<unsigned int>(elapsedSec * 1000.0f);
  tickTimeSec = elapsedSec;
  curTimeSec = static_cast<float>(pCurTimeMs) * 0.001f;
}

void __fastcall CWorld::Update() {
  ActivityBegin(ACTIVITY_WORLD);
  CalcFPS();
  for (unsigned int i = 0; i < 8; ++i) {
    texVect[i].x += tickTimeSec * s_texDir[i][0];
    texVect[i].y += tickTimeSec * s_texDir[i][1];
    if (texVect[i].x >= 64.0f) {
      texVect[i].x = 0.0f;
    }
    if (texVect[i].y >= 64.0f) {
      texVect[i].y = 0.0f;
    }
  }

  CWorldScene::Update();
  CMap::Update();
  UpdateDayNight(0, 0);
  farFog = DayNightGetInfo()->fogInfo.end;
  if ((enables & Enable_Particulates) && CWorldScene::camLiquid != 15) {
    particulate->Update();
  }
  ActivityEnd(ACTIVITY_WORLD);
}

void __fastcall CWorld::SetEnvironment() {
  DNInfo *dnInfo = DayNightGetInfo();
  GxRsSet(GxRs_FogStart, dnInfo->fogInfo.start);
  GxRsSet(GxRs_FogEnd, dnInfo->fogInfo.end);
  GxRsSet(GxRs_FogColor, dnInfo->fogInfo.color);
}

void __fastcall CWorld::UpdateDayNight(int forceFull, NTempest::C3Vector *position) {
  DNInfo *dnInfo = DayNightGetInfo();
  if (forceFull) {
    if (position) {
      dnInfo->playerPos = *position;
    }
    DayNightForceFullUpdate();
  } else {
    DayNightUpdateLighting();
  }

  SetShadowColor(dnInfo->shadowClr);
  CMap::sunLight->gxLight.m_dir = dnInfo->lightInfo.dir;
  CMap::sunLight->gxLight.m_ambColor = dnInfo->lightInfo.ambColor;
  CMap::sunLight->gxLight.m_dirColor = dnInfo->lightInfo.dirColor;
  CMap::sunLight->gxLight.m_specColor = dnInfo->light.SkyArray[5];
  CMap::sunLight->gxLight.m_specIntensity = 1.0f;
}

void __fastcall CWorld::Render() {
  ActivityBegin(ACTIVITY_WORLD);
  if (enables & Enable_ShowTris) {
    GxMasterEnableSet(GxMasterEnable_PolygonFill, 0);
  }

  CWorldScene::Render();

  if ((enables & Enable_Particulates) && CWorldScene::camLiquid != 15) {
    ModelAddToScene(NTempest::C3Vector(), 0, Particulate::CustomRenderCallback, particulate, 0);
  }

  if (enables & Enable_ShowTris) {
    GxMasterEnableSet(GxMasterEnable_PolygonFill, 1);
  }

  CMap::ProjectLights();
  ActivityEnd(ACTIVITY_WORLD);
}

void __fastcall CWorld::RenderAlpha() {
  ActivityBegin(ACTIVITY_WORLD);
  CWorldScene::RenderAlpha();
  ActivityEnd(ACTIVITY_WORLD);
}

unsigned int __fastcall CWorld::QueryAreaId(float x, float y) {
  return CMap::QueryAreaId(x, y);
}

int __fastcall CWorld::QueryObjectInside(unsigned long hWorldObject) {
  CMapEntity *entity = reinterpret_cast<CMapEntity *>(hWorldObject);

  FATALASSERT(entity);
  FATALASSERT(entity->GetType() & CMapBaseObj::Type_Entity);
  return entity->flagInside;
}

int __fastcall CWorld::QueryMapObjZoneName(unsigned long hWorldObject, const char *&zoneName) {
  CMapEntity *entity = reinterpret_cast<CMapEntity *>(hWorldObject);

  FATALASSERT(entity);
  FATALASSERT(entity->GetType() & CMapBaseObj::Type_Entity);
  return entity->QueryMapObjZoneName(zoneName);
}

int __fastcall CWorld::QueryMapObjSubzoneName(unsigned long hWorldObject, const char *&subzoneName, unsigned int &subzoneId) {
  CMapEntity *entity = reinterpret_cast<CMapEntity *>(hWorldObject);

  FATALASSERT(entity);
  FATALASSERT(entity->GetType() & CMapBaseObj::Type_Entity);
  return entity->QueryMapObjSubzoneName(subzoneName, subzoneId);
}

int __fastcall CWorld::QueryMapObjFileName(unsigned long hWorldObject, const char *&fileName) {
  CMapEntity *entity = reinterpret_cast<CMapEntity *>(hWorldObject);

  FATALASSERT(entity);
  FATALASSERT(entity->GetType() & CMapBaseObj::Type_Entity);
  return entity->QueryMapObjFileName(fileName);
}

unsigned int __fastcall CWorld::QueryMapObjMinimap(unsigned long hWorldObject, NTempest::CAaBox &aaBox, TSStackArray<MinimapQuad> &quads) {
  CMapEntity *entity = reinterpret_cast<CMapEntity *>(hWorldObject);
  FATALASSERT(entity);
  FATALASSERT(entity->GetType() & CMapBaseObj::Type_Entity);
  return entity->QueryMapObjMinimap(aaBox, quads);
}

unsigned int __fastcall CWorld::QueryMapObjIDs(unsigned long hWorldObject, unsigned int &wmoID, unsigned int &instanceID, unsigned int &groupID) {
  CMapEntity *entity = reinterpret_cast<CMapEntity *>(hWorldObject);
  FATALASSERT(entity);
  FATALASSERT(entity->GetType() & CMapBaseObj::Type_Entity);
  return entity->flagInside ? entity->QueryMapObjIDs(wmoID, instanceID, groupID) : 0;
}

unsigned int __fastcall CWorld::QueryMapObjMatrix(unsigned long hWorldObject, NTempest::C44Matrix *mtx, NTempest::C44Matrix *invMtx) {
  CMapEntity *entity = reinterpret_cast<CMapEntity *>(hWorldObject);
  FATALASSERT(entity);
  FATALASSERT(entity->GetType() & CMapBaseObj::Type_Entity);
  return entity->flagInside ? entity->QueryMapObjMatrix(mtx, invMtx) : 0;
}

bool __fastcall CWorld::QueryMapObjAreaTable(unsigned long hWorldObject, const WMOAreaTableRec *&subzoneRec, const WMOAreaTableRec *&globalRec) {
  CMapEntity *entity = reinterpret_cast<CMapEntity *>(hWorldObject);

  FATALASSERT(entity);
  FATALASSERT(entity->GetType() & CMapBaseObj::Type_Entity);
  return entity->QueryMapObjAreaTable(subzoneRec, globalRec);
}

int __fastcall CWorld::QueryMapObjFog(unsigned long hWorldObject, SMOFog::Fogs &oFogs, float &oPct) {
  if (!hWorldObject) {
    return CMapEntity::QueryCameraFog(oFogs, oPct);
  }

  CMapEntity *entity = reinterpret_cast<CMapEntity *>(hWorldObject);
  FATALASSERT(entity->GetType() & CMapBaseObj::Type_Entity);
  if (entity->flagInside) {
    return entity->QueryMapObjFog(oFogs, oPct);
  }
  return 0;
}

int __fastcall CWorld::QueryObjectLiquid(unsigned long hWorldObject, unsigned int &liquid, float &surface, NTempest::C3Vector &flowDir, int &deep) {
  CMapEntity *entity = reinterpret_cast<CMapEntity *>(hWorldObject);

  FATALASSERT(entity);
  FATALASSERT(entity->GetType() & CMapBaseObj::Type_Entity);
  if (!entity->flagInLiquid) {
    return 0;
  }

  liquid = entity->lqWhich;
  flowDir = entity->lqDirection;
  surface = entity->lqSurface;
  deep = entity->flagDeepLiquid;
  return 1;
}

void __fastcall CWorld::ObjectUpdate(unsigned int id, NTempest::C3Vector &pos, float angle, int bSnap) {
  FATALASSERT(reinterpret_cast<CMapBaseObj *>(id));
  if (bSnap) {
    CMap::SnapBaseObjToSubChunk(reinterpret_cast<CMapBaseObj *>(id), pos, angle);
  }
  if (reinterpret_cast<CMapBaseObj *>(id)->GetType() & CMapBaseObj::Type_MapObjDef) {
    CMap::UpdateMapObjDef(static_cast<CMapObjDef *>(reinterpret_cast<CMapBaseObj *>(id)), pos, angle);
  } else {
    CMap::UpdateDoodadDef(static_cast<CMapDoodadDef *>(reinterpret_cast<CMapBaseObj *>(id)), pos, angle);
  }
}

void __fastcall CWorld::ObjectGetExtents(unsigned int id, NTempest::CAaBox &extents) {
  CMapBaseObj *baseObj = reinterpret_cast<CMapBaseObj *>(id);

  FATALASSERT(baseObj);
  if (baseObj->GetType() & CMapBaseObj::Type_MapObjDef) {
    CMapObjDef *mapObjDef = static_cast<CMapObjDef *>(baseObj);
    FATALASSERT(mapObjDef->mapObj);
    mapObjDef->mapObj->GetBounds(extents);
  } else {
    ModelGetExtents(static_cast<CMapStaticEntity *>(baseObj)->model, &extents);
  }
}

void __fastcall CWorld::ObjectDelete(unsigned int id) {
  CMapBaseObj *baseObj = reinterpret_cast<CMapBaseObj *>(id);

  FATALASSERT(baseObj);
  CMapBaseObjLink *link = baseObj->parentLinkList.Head();
  while (reinterpret_cast<long>(link) > 0) {
    CMapBaseObjLink *next = baseObj->parentLinkList.RawNext(link);
    CMap::FreeBaseObjLink(link);
    link = next;
  }

  --baseObj->refCount;
  if (baseObj->GetType() & CMapBaseObj::Type_MapObjDef) {
    CMap::PurgeMapObjDef(static_cast<CMapObjDef *>(baseObj));
  } else {
    CMap::PurgeDoodadDef(static_cast<CMapDoodadDef *>(baseObj));
  }
}

void __fastcall CWorld::SetObjectHandler(int(__fastcall *handler)(void *, unsigned long, unsigned __int64, unsigned long), void *handlerParam) {
  CMap::entityHandler = handler;
  CMap::entityHandlerParam = handlerParam;
}

void __fastcall CWorld::SetObjectCollisionHandler(int(__fastcall *handler)(unsigned __int64, unsigned long, WorldObjCollisionHandlerData *)) {
  CMap::entityCollisionHandler = handler;
}

unsigned long __fastcall CWorld::AddObject(unsigned __int64 param64, unsigned long param32, HMODEL__ *hModel, unsigned int objFlags) {
  CMapEntity *entity = CMap::AllocEntity();
  FATALASSERT(entity);

  entity->model = hModel ? reinterpret_cast<HMODEL__ *>(HandleDuplicate(reinterpret_cast<HOBJECT>(hModel))) : 0;
  entity->param64 = param64;
  entity->param32 = param32;
  entity->pos = NTempest::C3Vector(10000000.0f, 10000000.0f, 10000000.0f);
  entity->scale = 1.0f;
  entity->dirLightScaleTarget = 1.0f;
  entity->handler = 0;
  entity->rFrameCount = 0;
  entity->flags = 0;
  entity->flagCollidable = (objFlags & 1) != 0;
  entity->flagCastShadow = (objFlags & 2) == 0;
  entity->ambient = CMap::sunLight->gxLight.m_ambColor;
  entity->ambientTarget = CMap::sunLight->gxLight.m_ambColor;

  return reinterpret_cast<unsigned long>(entity);
}

unsigned long __fastcall CWorld::AddDoodad(const char *fileName, HMODEL__ *hModel, const NTempest::C44Matrix &mat, unsigned int objFlags) {
  CMapDoodadDef *doodad = CMap::AllocDoodadDef();
  FATALASSERT(doodad);

  doodad->model = hModel ? reinterpret_cast<HMODEL__ *>(HandleDuplicate(reinterpret_cast<HOBJECT>(hModel))) : 0;
  doodad->flagCollidable = (objFlags & 1) != 0;
  doodad->flagCastShadow = (objFlags & 2) == 0;
  doodad->flagAlwaysAnimate = (objFlags & 4) != 0;
  doodad->modelName = fileName;
  doodad->flags = CMapBaseObj::Flag_LightUpdate;
  doodad->mat = mat;
  doodad->scale = NTempest::CMath::sqrt_(mat.a0 * mat.a0 + mat.a1 * mat.a1 + mat.a2 * mat.a2);
  doodad->pos = NTempest::C3Vector(mat.d0, mat.d1, mat.d2);
  doodad->lMat = NTempest::C44Matrix();

  CMap::InitializeDoodadBounds(doodad);
  CMap::LinkEntity(doodad);
  return reinterpret_cast<unsigned long>(doodad);
}

HMODEL__ *__fastcall CWorld::GetModel(unsigned long doodad) {
  CMapDoodadDef *entity = reinterpret_cast<CMapDoodadDef *>(doodad);
  FATALASSERT(entity);
  FATALASSERT(entity->GetType() & CMapBaseObj::Type_DoodadDef);
  return entity->model;
}

void __fastcall CWorld::SetObjectRenderCallback(unsigned long hWorldObject, void(__fastcall *cb)(void *, const NTempest::C44Matrix &), void *param) {
  CMapDoodadDef *doodad = reinterpret_cast<CMapDoodadDef *>(hWorldObject);
  FATALASSERT(doodad);
  FATALASSERT(doodad->GetType() & CMapBaseObj::Type_DoodadDef);
  doodad->RenderCB = cb;
  doodad->renderCBParam = param;
}

void __fastcall CWorld::UpdateObject(unsigned long hWorldObject, NTempest::C44Matrix &mat, NTempest::CAaBox &aaBox) {
  CMapBaseObj *baseObj = reinterpret_cast<CMapBaseObj *>(hWorldObject);

  ActivityBegin(ACTIVITY_WORLD);
  FATALASSERT(baseObj);
  FATALASSERT(baseObj->GetType() & (CMapBaseObj::Type_Entity | CMapBaseObj::Type_DoodadDef));

  baseObj->pos = NTempest::C3Vector(mat.d0, mat.d1, mat.d2);
  baseObj->scale = NTempest::CMath::sqrt_(mat.a0 * mat.a0 + mat.a1 * mat.a1 + mat.a2 * mat.a2);

  NTempest::C33Matrix normMat(mat.a0, mat.a1, mat.a2, mat.b0, mat.b1, mat.b2, mat.c0, mat.c1, mat.c2);
  if (baseObj->scale != 1.0f) {
    normMat.a0 *= 1.0f / baseObj->scale;
    normMat.a1 *= 1.0f / baseObj->scale;
    normMat.a2 *= 1.0f / baseObj->scale;
    normMat.b0 *= 1.0f / baseObj->scale;
    normMat.b1 *= 1.0f / baseObj->scale;
    normMat.b2 *= 1.0f / baseObj->scale;
    normMat.c0 *= 1.0f / baseObj->scale;
    normMat.c1 *= 1.0f / baseObj->scale;
    normMat.c2 *= 1.0f / baseObj->scale;
  }
  baseObj->rot.FromRotationMatrix(normMat);

  NTempest::CAaBox nAaBox;
  CWorldMath::TransformAABox(mat, aaBox, nAaBox);
  baseObj->aaBox = nAaBox;
  baseObj->aaSphere.c = (nAaBox.b + nAaBox.t) * 0.5f;
  baseObj->aaSphere.r = NTempest::CMath::sqrt_(
      (nAaBox.t.x - baseObj->aaSphere.c.x) * (nAaBox.t.x - baseObj->aaSphere.c.x) +
      (nAaBox.t.y - baseObj->aaSphere.c.y) * (nAaBox.t.y - baseObj->aaSphere.c.y) +
      (nAaBox.t.z - baseObj->aaSphere.c.z) * (nAaBox.t.z - baseObj->aaSphere.c.z)
  );

  if (baseObj->GetType() & CMapBaseObj::Type_Entity) {
    CMap::UpdateEntity(static_cast<CMapEntity *>(baseObj));
  }
  ActivityEnd(ACTIVITY_WORLD);
}

void __fastcall CWorld::TickObject(unsigned long hWorldObject) {
  CMapEntity *entity = reinterpret_cast<CMapEntity *>(hWorldObject);
  FATALASSERT(entity);
  FATALASSERT(entity->GetType() & CMapBaseObj::Type_Entity);
  entity->Tick();
}

void __fastcall CWorld::SetHidden(unsigned long hWorldObject, int hidden) {
  CMapEntity *entity = reinterpret_cast<CMapEntity *>(hWorldObject);
  FATALASSERT(entity);
  FATALASSERT(entity->GetType() & CMapBaseObj::Type_Entity);
  entity->flagHidden = hidden != 0;
}

void __fastcall CWorld::RemoveObject(unsigned long hWorldObject) {
  CMapStaticEntity *entity = reinterpret_cast<CMapStaticEntity *>(hWorldObject);
  FATALASSERT(entity);

  while (entity->parentLinkList.Head()) {
    CMap::FreeBaseObjLink(entity->parentLinkList.Head());
  }

  if (entity->GetType() & CMapBaseObj::Type_DoodadDef) {
    CMap::PurgeDoodadDef(static_cast<CMapDoodadDef *>(entity));
    return;
  }

  if (entity->model) {
    HandleClose(reinterpret_cast<HOBJECT>(entity->model));
  }

  while (entity->cacheLightList.Head()) {
    CMap::FreeCacheLight(entity->cacheLightList.Head());
  }

  if (entity->GetType() & CMapBaseObj::Type_Entity) {
    CMap::FreeEntity(static_cast<CMapEntity *>(entity));
  } else {
    FATALASSERT(!("CWorld::RemoveObject(): unhandled type"));
  }
}

void __fastcall CWorld::SetCameraTarget(unsigned long hWorldObject) {
  CMapEntity *entity = reinterpret_cast<CMapEntity *>(hWorldObject);
  FATALASSERT(entity);
  FATALASSERT(entity->GetType() & CMapBaseObj::Type_Entity);
  CWorldScene::camTargEntity = entity;
}

float __fastcall CWorld::CalcAltitude(float x, float y, float radius) {
  return CMap::PointIntersect(x, y, radius);
}

bool __fastcall CWorld::Intersect(
    const NTempest::C3Vector *a,
    const NTempest::C3Vector *b,
    float                     radius,
    NTempest::C3Vector       *ip,
    float                    *dist,
    unsigned int              queryFlags
) {
  return false;
}

bool __fastcall CWorld::GetFacet(const NTempest::C3Segment &seg, float &t, NTempest::C4Plane &facet, unsigned int queryFlags) {
  ActivityBegin(ACTIVITY_WORLD);
  bool result = CMap::GetFacet(seg, t, facet, queryFlags);
  ActivityEnd(ACTIVITY_WORLD);
  return result;
}

unsigned int __fastcall CWorld::GetTris(NTempest::CAaBox &aaBox, CWTriData &triData, unsigned int queryFlags) {
  ActivityBegin(ACTIVITY_WORLD);
  unsigned int result = CMap::GetTris(aaBox, triData, queryFlags);
  ActivityEnd(ACTIVITY_WORLD);
  return result;
}

int __fastcall CWorld::QueryLiquidStatus(NTempest::C3Vector &point, unsigned int &liquid, float &surface, NTempest::C3Vector &waterDir) {
  int deep;
  return CMap::QueryLiquidStatus(point, liquid, surface, waterDir, deep);
}

unsigned int __fastcall CWorld::SceneCamLiquidStatus() {
  return CWorldScene::camLiquid;
}

void __fastcall CWorld::WaterRipple(NTempest::C3Vector &pos, float len, float time, float amp, float vel, float freq) {
  CMap::WaterRipple(pos, len, time, amp, vel, freq);
}

float __fastcall CWorld::GetFramerate() {
  float elapsed = 0.0f;
  int   index = profIdx;
  for (int count = 0; count < 30; ++count) {
    elapsed += profTimes[index++];
    if (index == 30) {
      index = 0;
    }
  }

  elapsed *= 1.0f / 30.0f;
  return elapsed >= 0.01f ? 1.0f / elapsed : 100.0f;
}

void __fastcall CWorld::GetCounts(int *const counts) {
  CMap::GetCounts(counts);
}

void __fastcall CWorld::GetFacets(NTempest::CAaBox &aaBox, CWFacetData *facetData, unsigned int queryFlags) {
  ActivityBegin(ACTIVITY_WORLD);
  CMap::GetFacets(aaBox, facetData, queryFlags);
  ActivityEnd(ACTIVITY_WORLD);
}

void __fastcall CWorld::GetFacets(CWFrustum &frustum, CWFacetData *facetData, unsigned int queryFlags) {
  ActivityBegin(ACTIVITY_WORLD);
  CMap::GetFacets(frustum, facetData, queryFlags);
  ActivityEnd(ACTIVITY_WORLD);
}

const char *__fastcall CWorld::QueryChunkName() {
  return CWorldScene::currentChunkName;
}

const NTempest::C3Vector &__fastcall CWorld::GetCamPos() {
  return CWorldScene::camPos;
}

const NTempest::C3Vector &__fastcall CWorld::GetCamTarget() {
  return CWorldScene::camTarg;
}

void __fastcall CWorld::SetShadowColor(NTempest::CImVector &color) {
  shadowColor = color;
}

void __fastcall CWorld::SetDetailDoodadDensity(unsigned int density) {
  int          chunkWidth;
  unsigned int estimate;
  unsigned int vertices;

  if (detailDoodadDensity == density) {
    return;
  }

  detailDoodadDensity = density;
  CMap::ClearDetailDoodads();

  chunkWidth = 2 - static_cast<int>(detailDoodadDist * -0.030000001f);
  estimate = chunkWidth * chunkWidth * detailDoodadDensity;
  vertices = estimate << 7;

  if (vertices > pnct0EstimateVertex) {
    unsigned int indices = estimate * 192;

    pnct0EstimateVertex = vertices;
    pnct0EstimateIndex = indices;
    GxBufReserve(GxBWF_Medium, GxVBF_PNCT0, vertices, indices);
  }
}

void __fastcall CWorld::SetNearClip(float nearClip) {
  CWorld::nearClip = nearClip;
}

void __fastcall CWorld::SetFarClip(float farClip) {
  int          aoiSize;
  int          aoiCount;
  int          estimate;
  unsigned int vertices;
  unsigned int indices;
  if (CWorld::farClip != farClip) {
    CWorld::farClip = farClip;
    aoiSize = 1 - static_cast<int>(farClip * -0.030000001f);
    chunkAoiSize.y = aoiSize;
    chunkAoiSize.x = aoiSize;
    aoiCount = 4 * aoiSize * aoiSize;
    estimate = aoiCount / 2;
    vertices = 145 * estimate;

    if (vertices > pnEstimateVertex) {
      indices = 768 * estimate;
      pnEstimateVertex = vertices;
      pnEstimateIndex = indices;
      GxBufReserve(GxBWF_Low, GxVBF_PN, vertices, indices);
    }

    if (pnt0EstimateVertex < 0x18000) {
      pnt0EstimateVertex = 0x18000;
      pnt0EstimateIndex = 0x20000;
      GxBufReserve(GxBWF_Low, GxVBF_PNT0, 0x18000, 0x20000);
      GxBufReserve(GxBWF_Low, GxVBF_PT0T1, 0x10000, 0x10000);
    }
  }
}

void __fastcall CWorld::SetTexLodBias(float bias) {
  texLodBias = bias;
}

void __fastcall CWorld::SetTexAnisotropy(unsigned int anisotropy) {
  texMaxAnisotropy = anisotropy;
  texMaxAnisotropyLog2 = 0;
  anisotropy >>= 1;
  while (anisotropy) {
    anisotropy >>= 1;
    ++texMaxAnisotropyLog2;
  }
}

bool __fastcall CWorld::SetLodDist(float dist) {
  if (dist != dist || (dist >= 50.0f && dist <= 250.0f)) {
    lodDist = dist;
    return true;
  }

  return false;
}

bool __fastcall CWorld::SetTextureLodDist(float dist) {
  if (dist != dist || (dist >= 80.0f && dist <= 777.0f)) {
    textureLodDist = dist;
    return true;
  }

  return false;
}

void __fastcall CWorld::CalcFPS() {
  profTimes[profIdx] = tickTimeSec;
  if (++profIdx == 30) {
    profIdx = 0;
  }
}

void __fastcall CWorld::PrepareAreaOfInterest(NTempest::C3Vector &position, NTempest::C3Vector &target) {
  float mx = -(position.y - 17066.666f);

  chunkRectHi.minx = static_cast<int>(mx * 0.03f - 0.5f);
  mx = -(position.x - 17066.666f);
  chunkRectHi.miny = static_cast<int>(mx * 0.03f - 0.5f);
  chunkRectHi.maxx = chunkRectHi.minx + chunkAoiSize.x;
  chunkRectHi.minx -= chunkAoiSize.x;
  chunkRectHi.maxy = chunkRectHi.miny + chunkAoiSize.y;
  chunkRectHi.miny -= chunkAoiSize.y;

  FATALASSERT(chunkRectHi.maxx > 0);
  FATALASSERT(chunkRectHi.minx <= (64 * 16));
  FATALASSERT(chunkRectHi.maxy > 0);
  FATALASSERT(chunkRectHi.miny <= (64 * 16));

  if (chunkRectHi.minx < 0) {
    chunkRectHi.minx = 0;
  }
  if (chunkRectHi.maxx >= 64 * 16) {
    chunkRectHi.maxx = 64 * 16 - 1;
  }
  if (chunkRectHi.miny < 0) {
    chunkRectHi.miny = 0;
  }
  if (chunkRectHi.maxy >= 64 * 16) {
    chunkRectHi.maxy = 64 * 16 - 1;
  }

  gbChunkRect.minx = chunkRectHi.minx - 1;
  gbChunkRect.maxx = chunkRectHi.maxx + 1;
  gbChunkRect.miny = chunkRectHi.miny - 1;
  gbChunkRect.maxy = chunkRectHi.maxy + 1;

  if (gbChunkRect.minx < 0) {
    gbChunkRect.minx = 0;
  }
  if (gbChunkRect.maxx >= 64 * 16) {
    gbChunkRect.maxx = 64 * 16 - 1;
  }
  if (gbChunkRect.miny < 0) {
    gbChunkRect.miny = 0;
  }
  if (gbChunkRect.maxy >= 64 * 16) {
    gbChunkRect.maxy = 64 * 16 - 1;
  }

  areaRect.minx = gbChunkRect.minx >> 4;
  areaRect.miny = gbChunkRect.miny >> 4;
  areaRect.maxx = gbChunkRect.maxx >> 4;
  areaRect.maxy = gbChunkRect.maxy >> 4;

  groupAoi.b = position - 150.0f;
  groupAoi.t = position + NTempest::C3Vector(150.0f);
  objectAoi.b = position - farClip;
  objectAoi.t = position + NTempest::C3Vector(farClip);
}

static void __fastcall UpdateShadowGxTex(
    EGxTexCommand cmd,
    unsigned int  w,
    unsigned int  h,
    unsigned int  d,
    unsigned int  mipLevel,
    void         *userArg,
    unsigned int &texelStrideInBytes,
    const void  *&texels
) {
  switch (cmd) {
    case GxTex_Lock:
      break;

    case GxTex_Latch:
      texelStrideInBytes = 4 * w;
      texels = userArg;
      break;
  }
}

void __fastcall CWorld::ModelGeoProjectCallback(NTempest::CAaBox &worldBox, NTempest::CImVector color, NTempest::C44Matrix &basis) {
  ProjectTex2d(worldBox, color, &basis, 0.5f);
}

int __fastcall CWorld::ParticleProjectCallback(const NTempest::C3Segment &seg, float &z) {
  NTempest::C4Plane facet;
  float             segT = 1.0f;

  if (!GetFacet(seg, segT, facet, 0x111)) {
    return 0;
  }

  z = (seg.end.z - seg.start.z) * segT + seg.start.z;
  return 1;
}

int __fastcall CWorld::AnimBoneProjectCallback(const NTempest::C3Segment &seg, float &z) {
  NTempest::C4Plane facet;
  float             segT = 1.0f;

  if (!GetFacet(seg, segT, facet, 0x111)) {
    return 0;
  }

  z = (seg.end.z - seg.start.z) * segT + seg.start.z;
  return 1;
}

int __fastcall CWorld::ConsoleCommand_ShowDetailDoodads(const char *, const char *) {
  if (enables & Enable_DetailDoodads) {
    ConsoleWrite("Detail doodads disabled.", DEFAULT_COLOR);
    enables &= ~Enable_DetailDoodads;
  } else {
    ConsoleWrite("Detail doodads enabled.", DEFAULT_COLOR);
    enables |= Enable_DetailDoodads;
  }

  return 1;
}

int __fastcall CWorld::ConsoleCommand_MaxLOD(const char *__formal, const char *arguments) {
  unsigned int maxLod;

  sscanf(arguments, "%d", &maxLod);
  if (maxLod > 3) {
    lodMax = 3;
    return 1;
  }
  if (maxLod < 2) {
    maxLod = 2;
  }
  lodMax = maxLod;
  return 1;
}

int __fastcall CWorld::ConsoleCommand_ShowCull(const char *, const char *) {
  if (enables & Enable_Culling) {
    ConsoleWrite("Terrain culling disabled.", DEFAULT_COLOR);
    enables &= ~Enable_Culling;
  } else {
    ConsoleWrite("Terrain culling enabled.", DEFAULT_COLOR);
    enables |= Enable_Culling;
  }

  return 1;
}

int __fastcall CWorld::ConsoleCommand_SetShadow(const char *__formal, const char *arguments) {
  float               color[4];
  NTempest::CImVector argb;

  sscanf(arguments, "%f %f %f %f", &color[0], &color[1], &color[2], &color[3]);

  for (int component = 0; component < 4; ++component) {
    if (color[component] < 0.0f || color[component] > 1.0f) {
      goto shadowColorRangeInvalid;
    }
  }

  argb.Set(
      static_cast<unsigned char>(color[0] * 255.0f), static_cast<unsigned char>(color[1] * 255.0f), static_cast<unsigned char>(color[2] * 255.0f),
      static_cast<unsigned char>(color[3] * 255.0f)
  );
  SetShadowColor(argb);
  return 1;

shadowColorRangeInvalid:
  ConsoleWrite("Color values must be in range (0.0,1.0).", DEFAULT_COLOR);
  return 0;
}

int __fastcall CWorld::ConsoleCommand_MapObjLightMode(const char *, const char *) {
  if (enables & Enable_VertexLight) {
    ConsoleWrite("MapObj lightmaps enabled.", DEFAULT_COLOR);
    enables &= ~Enable_VertexLight;
  } else {
    ConsoleWrite("MapObj vertex light enabled.", DEFAULT_COLOR);
    enables |= Enable_VertexLight;
  }

  return 1;
}

int __fastcall CWorld::ConsoleCommand_WaterShow(const char *, const char *) {
  if (enables & Enable_Water) {
    ConsoleWrite("Water disabled", DEFAULT_COLOR);
    enables &= ~Enable_Water;
  } else {
    ConsoleWrite("Water enabled", DEFAULT_COLOR);
    enables |= Enable_Water;
  }

  return 1;
}

int __fastcall CWorld::ConsoleCommand_WaterMaxLOD(const char *__formal, const char *arguments) {
  sscanf(arguments, "%d", &CMapArea::ccWaterMaxLOD);
  if (CMapArea::ccWaterMaxLOD > 4) {
    CMapArea::ccWaterMaxLOD = 4;
    return 1;
  }
  if (CMapArea::ccWaterMaxLOD < 0) {
    CMapArea::ccWaterMaxLOD = 0;
  }
  return 1;
}

int __fastcall CWorld::ConsoleCommand_WaterWaves(const char *__formal, const char *arguments) {
  sscanf(arguments, "%d", &CMapArea::ccWaterWaves);
  return 1;
}

int __fastcall CWorld::ConsoleCommand_WaterSpecular(const char *__formal, const char *arguments) {
  sscanf(arguments, "%d", &CMapArea::ccWaterSpecular);
  return 1;
}

int __fastcall CWorld::ConsoleCommand_WaterRipples(const char *__formal, const char *arguments) {
  sscanf(arguments, "%d", &CMapArea::ccWaterRipples);
  return 1;
}

int __fastcall CWorld::ConsoleCommand_WaterParticulates(const char *, const char *) {
  if (enables & Enable_Particulates) {
    ConsoleWrite("Particulates disabled", DEFAULT_COLOR);
    enables &= ~Enable_Particulates;
  } else {
    ConsoleWrite("Particulates enabled", DEFAULT_COLOR);
    enables |= Enable_Particulates;
  }

  return 1;
}

int __fastcall CWorld::ConsoleCommand_DetailDoodadAlpha(const char *__formal, const char *arguments) {
  unsigned int alphaRef;

  sscanf(arguments, "%d", &alphaRef);
  if (alphaRef > 255) {
    ConsoleWrite("Alpha ref range 0 - 255.", DEFAULT_COLOR);
  } else {
    detailDoodadAlphaRef = alphaRef;
  }
  return 1;
}

int __fastcall CWorld::ConsoleCommand_ShowShadow(const char *, const char *) {
  if (enables & Enable_Shadow) {
    ConsoleWrite("Terrain shadow disabled.", DEFAULT_COLOR);
    enables &= ~Enable_Shadow;
  } else {
    ConsoleWrite("Terrain shadow enabled.", DEFAULT_COLOR);
    enables |= Enable_Shadow;
  }

  return 1;
}

int __fastcall CWorld::ConsoleCommand_ShowLowDetail(const char *, const char *) {
  if (enables & Enable_LowDetail) {
    ConsoleWrite("Terrain low detail disabled.", DEFAULT_COLOR);
    enables &= ~Enable_LowDetail;
  } else {
    ConsoleWrite("Terrain low detail enabled.", DEFAULT_COLOR);
    enables |= Enable_LowDetail;
  }

  return 1;
}

int __fastcall CWorld::ConsoleCommand_ShowSimpleDoodads(const char *, const char *) {
  if (bShowSimpleDoodads) {
    ConsoleWrite("Simple doodads disabled.", DEFAULT_COLOR);
    bShowSimpleDoodads = 0;
  } else {
    ConsoleWrite("Simple doodads enabled.", DEFAULT_COLOR);
    bShowSimpleDoodads = 1;
  }

  return 1;
}

int __fastcall CWorld::ConsoleCommand_EnumTextures(const char *__formal, const char *name) {
  char  buffer[256];
  char  timeStamp[256];
  HSLOG log;

  if (!*name) {
    ConsoleWrite("Must specify log file name.", DEFAULT_COLOR);
    return 0;
  }

  OsGetTimeStamp(timeStamp, sizeof(timeStamp));
  SStrPrintf(buffer, sizeof(buffer), "%s_%s.log", name, timeStamp);
  SLogCreate(buffer, 1, &log);
  TextureLogTextures(log);
  SLogWrite(log, "Terrain Texture in Mbytes:\t\t%.2f", static_cast<float>(CMap::GetTextureUseage()) / 1048576.0f);
  SLogClose(log);
  return 1;
}

int __fastcall CWorld::ConsoleCommand_EnumTextureGxCache(const char *__formal, const char *name) {
  char  buffer[256];
  char  timeStamp[256];
  HSLOG log;

  if (!*name) {
    ConsoleWrite("Must specify log file name.", DEFAULT_COLOR);
    return 0;
  }

  OsGetTimeStamp(timeStamp, sizeof(timeStamp));
  SStrPrintf(buffer, sizeof(buffer), "%s_%s.log", name, timeStamp);
  SLogCreate(buffer, 1, &log);
  TextureLogGxCache(log);
  SLogClose(log);
  return 1;
}
