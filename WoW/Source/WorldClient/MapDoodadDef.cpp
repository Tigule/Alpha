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

#include "Model/IModel.h"
#include "Model/CollisionData.h"
#include "WorldCommon/WorldMath.h"

#include <Ftol.h>
#include <float.h>
#include <math.h>

namespace NTempest {

  inline void CImVector::Scale255RGB_(DWORD scale) {
    DWORD d = *IV_();
    DWORD dr;
    DWORD dg;
    DWORD db;
    Get_(d, dr, dg, db);
    *IV_() = MakeRGB((scale * dr + 255) >> 8, (scale * dg + 255) >> 8, (scale * db + 255) >> 8) |
             (d & eAlphaMask);
  }

  inline void CImVector::Scale255RGB(DWORD scale) {
    Scale255RGB_(scale);
  }

}

const float              CMapStaticEntity::dirLightScaleAmount = 0.5f;
const NTempest::C3Vector CMapStaticEntity::interiorSunDir(-0.30822f, -0.30822f, -0.9f);

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

  UINT whichLight = 1;
  ITERATELIST(CMapCacheLight, cacheLightList, cacheLight) {
    GxLightSet(whichLight, cacheLight->gxLight, CWorldScene::camPos);
    ++whichLight;
    if (whichLight == 8) {
      return;
    }
  }

  do {
    GxLightEnable(whichLight, 0);
  } while (++whichLight != 8);
}

void CMapStaticEntity::AdjustLightmap(
    const NTempest::CImVector &lmColor,
    NTempest::CImVector       &dirColor,
    BYTE                       minDir,
    NTempest::CImVector       &ambColor,
    BYTE                       maxAmbient
) {
  BYTE maxMag = max(1, max(max(lmColor.r, lmColor.g), lmColor.b));

  dirColor = lmColor;
  if (maxMag < minDir) {
    NTempest::C3Vector rgb(dirColor.r * 0.0039215689f, dirColor.g * 0.0039215689f, dirColor.b * 0.0039215689f);
    NTempest::C3Vector hsv;
    NTempest::RGBtoHSV(rgb, hsv);
    hsv.z *= (float)minDir / maxMag;
    NTempest::HSVtoRGB(hsv, rgb);
    dirColor = NTempest::CImVector(
        255, NTempest::CMath::ftol_0_256_(rgb.x * 255.0f), NTempest::CMath::ftol_0_256_(rgb.y * 255.0f), NTempest::CMath::ftol_0_256_(rgb.z * 255.0f)
    );
  }

  ambColor = lmColor;
  if (maxMag > maxAmbient) {
    ambColor.Scale255RGB(Fast_ftol((float)maxAmbient * 255.0f / maxMag));
  }
}

void CMapStaticEntity::FindLights() {
  CMapCacheLight *cacheLight = cacheLightList.Head();
  while (1) {
    if ((int)cacheLight <= 0) {
      break;
    }

    CMapCacheLight *next = cacheLightList.RawNext(cacheLight);
    CMap::FreeCacheLight(cacheLight);
    cacheLight = next;
  }

  ITERATELIST(CMapBaseObjLink, parentLinkList, parentLink) {
    CMapBaseObj *parent = parentLink->ref;
    if (parent->GetType() & Type_Chunk) {
      CMapChunk *chunk = (CMapChunk *)parent;
      ITERATELIST(CMapBaseObjLink, chunk->lightLinkList, lightLink) {
        CreateCacheLight((CMapLight *)lightLink->owner);
      }
    } else if (parent->GetType() & Type_MapObjDefGroup) {
      CMapObjDefGroup *group = (CMapObjDefGroup *)parent;
      ITERATELIST(CMapBaseObjLink, group->lightLinkList, lightLink) {
        CreateCacheLight((CMapLight *)lightLink->owner);
      }
    }
  }

  flags &= ~Flag_LightUpdate;
}

void CMapStaticEntity::CreateCacheLight(CMapLight *light) {
  if (light->attenDenom != 0.0f) {
    float dirIntensity;
    NTempest::C3Vector lightDir = pos + NTempest::C3Vector(0.0f, 0.0f, 1.1666666f) - light->gxLight.m_dir;
    float lightDist = lightDir.Mag();
    if (lightDist < light->attenStart) {
      dirIntensity = 1.0f;
    } else if (lightDist < light->attenEnd) {
      dirIntensity = 1.0f - (lightDist - light->attenStart) * light->attenDenom;
    } else {
      return;
    }

    CMapCacheLight *cacheLight = CMap::AllocCacheLight();
    ASSERT(cacheLight);
    cacheLightList.LinkNode(cacheLight, LIST_TAIL, 0);
    cacheLight->gxLight = light->gxLight;
    cacheLight->gxLight.m_dir = lightDir * (1.0f / lightDist);
    cacheLight->gxLight.m_dirIntensity = dirIntensity;
    cacheLight->gxLight.m_enabled = 1;
    cacheLight->gxLight.m_isOmni = 0;
    cacheLight->gxLight.m_constantAttenuation = 1.0f;
    cacheLight->gxLight.m_linearAttenuation = cacheLight->gxLight.m_quadraticAttenuation = 0.0f;
  } else {
    CMapCacheLight *cacheLight = CMap::AllocCacheLight();
    ASSERT(cacheLight);
    cacheLightList.LinkNode(cacheLight, LIST_TAIL, 0);
    cacheLight->gxLight = light->gxLight;
  }
}

CMapDoodadDef::CMapDoodadDef() {
  type |= Type_DoodadDef;
  modelName = 0;
  model = 0;
  doodadSoundHandle = 0;
}

CMapDoodadDef::~CMapDoodadDef() {
  FATALASSERT(refCount==0);
}

void CMapDoodadDef::SelectLights() {
  if (flags & Flag_LightUpdate) {
    FindLights();
  }

  CGxLight    gxLight = CMap::sunLight->gxLight;
  CMapObjDef *mapObjDef;

  if (flags & Flag_InteriorLit) {
    gxLight.m_dir = interiorSunDir;
    gxLight.m_dirColor = interiorDirColor;
    gxLight.m_ambColor = ambient;
    gxLight.m_dirIntensity = 1.0f;
    gxLight.m_ambIntensity = 1.0f;

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

  UINT whichLight = 1;
  ITERATELIST(CMapCacheLight, cacheLightList, cacheLight) {
    GxLightSet(whichLight, cacheLight->gxLight, CWorldScene::camPos);
    ++whichLight;
    if (whichLight == 8) {
      return;
    }
  }

  do {
    GxLightEnable(whichLight, 0);
  } while (++whichLight != 8);
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

void CMapDoodadDef::Update(const NTempest::C44Matrix &newMat) {
  if (model) {
    flags |= Flag_LightUpdate;
    pos = *lMat.Row3AsVec3();
    pos *= newMat;
    mat = lMat * newMat;
    NTempest::CAaBox    localExt;
    NTempest::CAaSphere localSphere;
    ModelGetExtents(model, &localExt);
    ModelGetBounds(model, &localSphere);
    aaSphere.c = localSphere.c * mat;
    aaSphere.r = localSphere.r * scale;
    CWorldMath::TransformAABox(mat, localExt, aaBox);
    ModelGetCollisionExtents(model, &localExt);
    CWorldMath::TransformAABox(mat, localExt, collideExt);
  }
}

void CMapDoodadDef::QueryLightmap(CMapObjDef *mapObjDef, CMapObjGroup *mapObjGroup) {
  static NTempest::C3Vector dirs[6] = {NTempest::C3Vector(0.0f, 0.0f, 1.0f), NTempest::C3Vector(0.0f, 0.0f, -1.0f),
                                       NTempest::C3Vector(1.0f, 0.0f, 0.0f), NTempest::C3Vector(-1.0f, 0.0f, 0.0f),
                                       NTempest::C3Vector(0.0f, 1.0f, 0.0f), NTempest::C3Vector(0.0f, -1.0f, 0.0f)};
  CMapObj           *mapObj = mapObjDef->mapObj;
  NTempest::C3Vector localPos = pos * mapObjDef->invMat;
  float              invScale = 1.0f / lMat.Row0AsVec3()->Mag();
  if (mapObj) {
    for (UINT tries = 0; tries < 2; ++tries) {
      float               closestT = FLT_MAX;
      NTempest::CImVector closestC(0ul);
      float               radius;
      if (!tries) {
        radius = max(3.0f, aaSphere.r);
      } else {
        radius = 50.0f * invScale;
      }

      for (UINT i = 0; i < 6; ++i) {
        NTempest::C3Vector localRadVec = NTempest::C44Matrix::mul3v33m_(dirs[i], lMat);
        localRadVec *= radius;
        NTempest::C3Segment lmQuerySeg(localPos, localPos + localRadVec);

        float               dirDist;
        NTempest::CImVector lmColor(0ul);
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
  ambient = NTempest::CImVector(0xFF808080);  // grey
  ambient = NTempest::CImVector(0xFFFFFF00);  // yellow
}
