#include "WorldClient/World.h"
#include "WorldClient/CMapObj.h"

#include "DayNight.h"
#include "Gx/Gx.h"
#include "Model/CollisionData.h"
#include "Model/IModel.h"
#include "WorldCommon/WorldMath.h"

#include <float.h>
#include <math.h>

const float              CMapStaticEntity::dirLightScaleAmount = 0.5f;
const NTempest::C3Vector CMapStaticEntity::interiorSunDir(-0.30822f, -0.30822f, -0.9f);

void CMapStaticEntity::AdjustLightmap(
    NTempest::CImVector &lmColor,
    NTempest::CImVector &dirColor,
    unsigned int         minDir,
    NTempest::CImVector &ambColor,
    unsigned int         maxAmbient
) {
  unsigned int maxMag = lmColor.r;
  if (lmColor.g > maxMag) {
    maxMag = lmColor.g;
  }
  if (lmColor.b > maxMag) {
    maxMag = lmColor.b;
  }
  if (!maxMag) {
    maxMag = 1;
  }

  dirColor = lmColor;
  if (maxMag < minDir) {
    NTempest::C3Vector rgb = dirColor;
    NTempest::C3Vector hsv;
    NTempest::RGBtoHSV(rgb, hsv);
    hsv.z *= static_cast<float>(minDir) / maxMag;
    NTempest::HSVtoRGB(hsv, rgb);
    dirColor = rgb;
  }

  ambColor = lmColor;
  if (maxMag > maxAmbient) {
    unsigned int scale = static_cast<unsigned int>(static_cast<float>(maxAmbient) * 255.0f / maxMag);
    ambColor.r = static_cast<unsigned char>((scale * ambColor.r + 255) >> 8);
    ambColor.g = static_cast<unsigned char>((scale * ambColor.g + 255) >> 8);
    ambColor.b = static_cast<unsigned char>((scale * ambColor.b + 255) >> 8);
  }
}

void CMapStaticEntity::FindLights() {
  CMapCacheLight *cacheLight = cacheLightList.Head();
  while (reinterpret_cast<long>(cacheLight) > 0) {
    CMapCacheLight *next = cacheLightList.RawNext(cacheLight);
    CMap::FreeCacheLight(cacheLight);
    cacheLight = next;
  }

  CMapBaseObjLink *parentLink = parentLinkList.Head();
  while (reinterpret_cast<long>(parentLink) > 0) {
    CMapBaseObj *parent = parentLink->ref;
    if (parent->GetType() & Type_Chunk) {
      CMapChunk       *chunk = static_cast<CMapChunk *>(parent);
      CMapBaseObjLink *lightLink = chunk->lightLinkList.Head();
      while (reinterpret_cast<long>(lightLink) > 0) {
        CreateCacheLight(static_cast<CMapLight *>(lightLink->owner));
        lightLink = chunk->lightLinkList.RawNext(lightLink);
      }
    } else if (parent->GetType() & Type_MapObjDefGroup) {
      CMapObjDefGroup *group = static_cast<CMapObjDefGroup *>(parent);
      CMapBaseObjLink *lightLink = group->lightLinkList.Head();
      while (reinterpret_cast<long>(lightLink) > 0) {
        CreateCacheLight(static_cast<CMapLight *>(lightLink->owner));
        lightLink = group->lightLinkList.RawNext(lightLink);
      }
    }

    parentLink = parentLinkList.RawNext(parentLink);
  }

  flags &= ~Flag_LightUpdate;
}

void CMapStaticEntity::CreateCacheLight(CMapLight *light) {
  if (light->attenDenom == 0.0f) {
    CMapCacheLight *cacheLight = CMap::AllocCacheLight();
    FATALASSERT(cacheLight);
    cacheLightList.LinkNode(cacheLight, LIST_TAIL, 0);
    cacheLight->gxLight = light->gxLight;
    return;
  }

  NTempest::C3Vector lightDir(pos.x - light->gxLight.m_dir.x, pos.y - light->gxLight.m_dir.y, pos.z + 1.1666666f - light->gxLight.m_dir.z);
  float              lightDist = lightDir.Mag();
  if (lightDist >= light->attenEnd) {
    return;
  }

  float dirIntensity;
  if (lightDist < light->attenStart) {
    dirIntensity = 1.0f;
  } else {
    dirIntensity = 1.0f - (lightDist - light->attenStart) * light->attenDenom;
  }

  CMapCacheLight *cacheLight = CMap::AllocCacheLight();
  FATALASSERT(cacheLight);
  cacheLightList.LinkNode(cacheLight, LIST_TAIL, 0);
  cacheLight->gxLight = light->gxLight;

  lightDir.Normalize();
  cacheLight->gxLight.m_enabled = 1;
  cacheLight->gxLight.m_isOmni = 0;
  cacheLight->gxLight.m_dir = lightDir;
  cacheLight->gxLight.m_dirIntensity = dirIntensity;
  cacheLight->gxLight.m_constantAttenuation = 1.0f;
  cacheLight->gxLight.m_linearAttenuation = 0.0f;
  cacheLight->gxLight.m_quadraticAttenuation = 0.0f;
}

CMapDoodadDef::CMapDoodadDef() {
  type |= Type_DoodadDef;
  modelName = 0;
  model = 0;
  doodadSoundHandle = 0;
}

void CMapStaticEntity::SelectLights() {
  if (flags & Flag_LightUpdate) {
    FindLights();
  }

  CGxLight    gxLight = CMap::sunLight->gxLight;
  CMapObjDef *mapObjDef;

  gxLight.m_ambColor = ambient;
  gxLight.m_ambIntensity = 1.0f;

  if (flags & Flag_ExteriorLit) {
    gxLight.m_dirIntensity *= dirLightScale;
  } else {
    gxLight.m_dir = interiorSunDir;
    gxLight.m_dirColor = interiorDirColor;
    gxLight.m_dirIntensity = dirLightScale;

    if (GetMapObjDef(mapObjDef)) {
      DNInfo *dnInfo = DayNightGetInfo();
      if (dnInfo->intFog && CWorldScene::camMapObjDef == mapObjDef) {
        GxRsSet(GxRs_FogStart, dnInfo->intFogInfo.start);
        GxRsSet(GxRs_FogEnd, dnInfo->intFogInfo.end);
        GxRsSet(GxRs_FogColor, dnInfo->intFogInfo.color);
      }
    }
  }

  GxLightSet(0, gxLight, CWorldScene::camPos);

  unsigned int    whichLight = 1;
  CMapCacheLight *cacheLight = cacheLightList.Head();
  while (reinterpret_cast<long>(cacheLight) > 0 && whichLight < 8) {
    GxLightSet(whichLight, cacheLight->gxLight, CWorldScene::camPos);
    ++whichLight;
    cacheLight = cacheLightList.RawNext(cacheLight);
  }

  while (whichLight < 8) {
    GxLightEnable(whichLight, 0);
    ++whichLight;
  }
}

CMapDoodadDef::~CMapDoodadDef() {
  FATALASSERT(refCount == 0);
}

void CMapDoodadDef::SelectLights() {
  if (flags & Flag_LightUpdate) {
    FindLights();
  }

  CGxLight    gxLight = CMap::sunLight->gxLight;
  CMapObjDef *mapObjDef;

  if (flags & Flag_InteriorLit) {
    gxLight.m_dir = interiorSunDir;
    gxLight.m_ambColor = ambient;
    gxLight.m_dirColor = interiorDirColor;
    gxLight.m_ambIntensity = 1.0f;
    gxLight.m_dirIntensity = 1.0f;

    if (GetMapObjDef(mapObjDef)) {
      DNInfo *dnInfo = DayNightGetInfo();
      if (dnInfo->intFog && CWorldScene::camMapObjDef == mapObjDef) {
        GxRsSet(GxRs_FogStart, dnInfo->intFogInfo.start);
        GxRsSet(GxRs_FogEnd, dnInfo->intFogInfo.end);
        GxRsSet(GxRs_FogColor, dnInfo->intFogInfo.color);
      }
    }
  } else {
    gxLight.m_dirIntensity *= dirLightScale;
  }

  GxLightSet(0, gxLight, CWorldScene::camPos);

  unsigned int    whichLight = 1;
  CMapCacheLight *cacheLight = cacheLightList.Head();
  while (reinterpret_cast<long>(cacheLight) > 0 && whichLight < 8) {
    GxLightSet(whichLight, cacheLight->gxLight, CWorldScene::camPos);
    ++whichLight;
    cacheLight = cacheLightList.RawNext(cacheLight);
  }

  while (whichLight < 8) {
    GxLightEnable(whichLight, 0);
    ++whichLight;
  }
}

void CMapDoodadDef::Update(const NTempest::C44Matrix &newMat) {
  NTempest::CAaBox    localExt;
  NTempest::CAaSphere localSphere;

  localSphere.c = NTempest::C3Vector();
  localSphere.r = 0.0f;

  if (model) {
    flags |= Flag_LightUpdate;
    pos = NTempest::C3Vector(lMat.d0, lMat.d1, lMat.d2);
    pos *= newMat;
    mat = lMat * newMat;
    ModelGetExtents(model, &localExt);
    ModelGetBounds(model, &localSphere);
    aaSphere.c = localSphere.c * mat;
    aaSphere.r = localSphere.r * scale;
    CWorldMath::TransformAABox(mat, localExt, aaBox);
    ModelGetCollisionExtents(model, &localExt);
    CWorldMath::TransformAABox(mat, localExt, collideExt);
  }
}

void CMapDoodadDef::GetBounds(NTempest::CAaSphere &bounds) {
  bounds = aaSphere;
}

void CMapDoodadDef::GetBounds(NTempest::CAaBox &bounds) {
  bounds = aaBox;
}

void CMapDoodadDef::GetCollideExt(NTempest::CAaBox &bounds) {
  bounds = collideExt;
}

void CMapDoodadDef::QueryLightmap(CMapObjDef *mapObjDef, CMapObjGroup *mapObjGroup) {
  static NTempest::C3Vector dirs[6] = {NTempest::C3Vector(0.0f, 0.0f, 1.0f), NTempest::C3Vector(0.0f, 0.0f, -1.0f),
                                       NTempest::C3Vector(1.0f, 0.0f, 0.0f), NTempest::C3Vector(-1.0f, 0.0f, 0.0f),
                                       NTempest::C3Vector(0.0f, 1.0f, 0.0f), NTempest::C3Vector(0.0f, -1.0f, 0.0f)};
  NTempest::C3Segment       lmQuerySeg;
  NTempest::C3Vector        localPos = pos * mapObjDef->invMat;
  NTempest::C3Vector        localRadVec;
  CMapObj                  *mapObj = mapObjDef->mapObj;
  float                     invScale = 1.0f / static_cast<float>(sqrt(lMat.a0 * lMat.a0 + lMat.a1 * lMat.a1 + lMat.a2 * lMat.a2));
  NTempest::CImVector       closestC;
  unsigned int              tries;
  NTempest::CImVector       lmColor;
  float                     dirDist;
  float                     closestT;

  (void)mapObjGroup;
  if (mapObj) {
    for (tries = 0; tries < 2; ++tries) {
      closestT = FLT_MAX;
      closestC = NTempest::CImVector(0);
      for (unsigned int i = 0; i < 6; ++i) {
        localRadVec.x = dirs[i].x * lMat.a0 + dirs[i].y * lMat.b0 + dirs[i].z * lMat.c0;
        localRadVec.y = dirs[i].x * lMat.a1 + dirs[i].y * lMat.b1 + dirs[i].z * lMat.c1;
        localRadVec.z = dirs[i].x * lMat.a2 + dirs[i].y * lMat.b2 + dirs[i].z * lMat.c2;
        lmQuerySeg.start = localPos;
        lmQuerySeg.end.x = localPos.x + localRadVec.x * (tries ? 50.0f : (aaSphere.r >= 3.0f ? aaSphere.r : 3.0f)) * invScale;
        lmQuerySeg.end.y = localPos.y + localRadVec.y * (tries ? 50.0f : (aaSphere.r >= 3.0f ? aaSphere.r : 3.0f)) * invScale;
        lmQuerySeg.end.z = localPos.z + localRadVec.z * (tries ? 50.0f : (aaSphere.r >= 3.0f ? aaSphere.r : 3.0f)) * invScale;

        lmColor = NTempest::CImVector(0);
        if (mapObj->QueryLightmap(lmQuerySeg, lmColor, &dirDist) && dirDist < closestT) {
          closestT = dirDist;
          closestC = lmColor;
        }
      }

      if (closestT != FLT_MAX) {
        AdjustLightmap(closestC, interiorDirColor, 112, ambient, 96);
        return;
      }
    }
  }

  // authentically double-set
  ambient = NTempest::CImVector(0xFF808080); // grey
  ambient = NTempest::CImVector(0xFFFFFF00); // yellow
}
