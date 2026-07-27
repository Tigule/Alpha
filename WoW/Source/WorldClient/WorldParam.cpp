#include "WorldParam.h"

#include "World.h"

#include <Console/ConsoleClient.h>
#include <Console/ConsoleVar.h>
#include <Gx/Gx.h>
#include <Model/IModel.h>
#include <Services/IParticleMisc.h>
#include <storm.h>

#include <stdio.h>

CVar *CWorldParam::cvar_triLinear;
CVar *CWorldParam::cvar_detailDensity;
CVar *CWorldParam::cvar_shadowLevel;
CVar *CWorldParam::cvar_particleDensity;
CVar *CWorldParam::cvar_pixelShaders;
CVar *CWorldParam::cvar_lod;
CVar *CWorldParam::cvar_fov;
CVar *CWorldParam::cvar_anisotropic;
CVar *CWorldParam::cvar_distCull;
CVar *CWorldParam::cvar_doodadAnim;
CVar *CWorldParam::cvar_textureLodDist;
CVar *CWorldParam::cvar_mapShadows;
CVar *CWorldParam::cvar_nearClip;
CVar *CWorldParam::cvar_alphaLevel;
CVar *CWorldParam::cvar_farClip;
CVar *CWorldParam::cvar_lodDist;
CVar *CWorldParam::cvar_unitDrawDist;
CVar *CWorldParam::cvar_maxLights;
CVar *CWorldParam::cvar_lightMaps;
CVar *CWorldParam::cvar_waterLod;
CVar *CWorldParam::cvar_baseMip;
CVar *CWorldParam::cvar_texLodBias;
CVar *CWorldParam::cvar_fullAlpha;
CVar *CWorldParam::cvar_smallCull;
CVar *CWorldParam::cvar_specular;

void CWorldParam::Initialize() {
  cvar_lod = CVar::Register("lod", "Video option: Toggle Lod", 1, "1", LodCallback, GRAPHICS, false, 0);
  cvar_fullAlpha = CVar::Register("fullAlpha", "Video option: Toggle full alpha", 1, "0", FullAlphaCallback, GRAPHICS, false, 0);
  cvar_doodadAnim = CVar::Register("doodadAnim", "Video option: Toggle doodad anim", 1, "1", DoodadAnimCallback, GRAPHICS, false, 0);
  cvar_mapShadows = CVar::Register("mapShadows", "Video option: Toggle map shadows", 1, "1", MapShadowsCallback, GRAPHICS, false, 0);
  cvar_lightMaps = CVar::Register("lightMaps", "Video option: Toggle light maps", 1, "1", LightMapsCallback, GRAPHICS, false, 0);
  cvar_lodDist = CVar::Register("lodDist", "Video option: Set Lod distance", 1, "100.0", LodDistCallback, GRAPHICS, false, 0);
  cvar_smallCull = CVar::Register("SmallCull", "Object size culling", 1, "0.04", SmallCullCallback, GRAPHICS, false, 0);
  cvar_distCull = CVar::Register("DistCull", "Object distance culling", 1, "500", DistCullCallback, GRAPHICS, false, 0);
  cvar_maxLights = CVar::Register("MaxLights", "Max number of hardware lights", 1, "4", MaxLightsCallback, GRAPHICS, false, 0);
  cvar_shadowLevel = CVar::Register("shadowLevel", "Terrain shadow map mip level", 1, "1", ShadowLevelCallback, GRAPHICS, false, 0);
  cvar_alphaLevel = CVar::Register("alphaLevel", "Terain alpha map mip level", 1, "0", AlphaLevelCallback, GRAPHICS, false, 0);
  cvar_texLodBias = CVar::Register("texLodBias", "Texture LOD Bias", 1, "0.5", TexLodBiasCallback, GRAPHICS, false, 0);
  cvar_triLinear = CVar::Register("trilinear", "Enable Trilinear texture filtering", 1, "0", TrilinearCallback, GRAPHICS, false, 0);
  cvar_detailDensity = CVar::Register("detailDensity", "Detail doodad density", 1, "16", DetailDoodadDensityCallback, GRAPHICS, false, 0);
  cvar_farClip = CVar::Register("farclip", "Far clip plane distance", 1, "350", FarClipCallback, GRAPHICS, false, 0);
  cvar_nearClip = CVar::Register("nearclip", "Near clip plane distance", 1, "0.1", NearClipCallback, GRAPHICS, false, 0);
  cvar_fov = CVar::Register("fov", "Field of view angle", 1, "90", FovCallback, GRAPHICS, false, 0);
  cvar_specular = CVar::Register("specular", "Specularity", 1, "0", SpecularCallback, GRAPHICS, false, 0);
  cvar_pixelShaders = CVar::Register("pixelShaders", "Use pixel shaders", 1, "0", PixelShadersCallback, GRAPHICS, false, 0);
  cvar_particleDensity = CVar::Register("particleDensity", "Video option: Particle density", 1, "1.0", ParticleDensityCallback, GRAPHICS, false, 0);
  cvar_unitDrawDist = CVar::Register("unitDrawDist", "Unit draw distance", 1, "150.0", UnitDrawDistCallback, GRAPHICS, false, 0);
  cvar_waterLod = CVar::Register("waterLOD", "Water geometry LOD", 1, "0", WaterLodCallback, GRAPHICS, false, 0);
  cvar_baseMip = CVar::Register("baseMip", "base mipmap level", 1, "0", BaseMipCallback, GRAPHICS, false, 0);
  cvar_anisotropic = CVar::Register("anisotropic", "Anisotropic texture filtering", 1, "1", AnisotropicCallback, GRAPHICS, false, 0);
  cvar_textureLodDist = CVar::Register("textureLodDist", "Video option: texture detail", 1, "777.0", TextureLodDistCallback, GRAPHICS, false, 0);
}

void CWorldParam::Destroy() {
}

bool CWorldParam::LodCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  if (SStrToInt(newValue)) {
    ConsoleWrite("Terrain LOD enabled.", DEFAULT_COLOR);
    CWorld::enables |= CWorld::Enable_Lod;
  } else {
    ConsoleWrite("Terrain LOD disabled.", DEFAULT_COLOR);
    CWorld::enables &= ~CWorld::Enable_Lod;
  }

  return true;
}

bool CWorldParam::FullAlphaCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  if (SStrToInt(newValue)) {
    ConsoleWrite("Full alpha on doodads enabled.", DEFAULT_COLOR);
    CWorld::enables &= ~CWorld::Enable_NoFullAlpha;
    CMap::EnableDoodadFullAlpha(1);
  } else if (!(CWorld::enables & CWorld::Enable_NoFullAlpha)) {
    ConsoleWrite("Full alpha on doodads disabled.", DEFAULT_COLOR);
    CWorld::enables |= CWorld::Enable_NoFullAlpha;
    CMap::EnableDoodadFullAlpha(0);
  }

  return true;
}

bool CWorldParam::DoodadAnimCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  if (SStrToInt(newValue)) {
    ConsoleWrite("Doodad animation enabled.", DEFAULT_COLOR);
    CWorld::enables &= ~CWorld::Enable_NoAnimation;
    CMap::ReloadDoodadModels();
  } else if (!(CWorld::enables & CWorld::Enable_NoAnimation)) {
    ConsoleWrite("Doodad animation disabled.", DEFAULT_COLOR);
    CWorld::enables |= CWorld::Enable_NoAnimation;
    CMap::ReloadDoodadModels();
  }

  return true;
}

bool CWorldParam::MapShadowsCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  if (SStrToInt(newValue)) {
    ConsoleWrite("Terrain shadows enabled.", DEFAULT_COLOR);
    CWorld::enables |= CWorld::Enable_Shadow;
  } else {
    ConsoleWrite("Terrain shadows disabled.", DEFAULT_COLOR);
    CWorld::enables &= ~CWorld::Enable_Shadow;
  }

  return true;
}

bool CWorldParam::LightMapsCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  return false;
}

bool CWorldParam::LodDistCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  float dist;

  sscanf(newValue, "%f", &dist);
  if (dist < 50.0f || dist > 250.0f) {
    ConsoleWrite("Lod distance must be in range 50.0f - 250.0f.", DEFAULT_COLOR);
    dist = 77.0f;
  }

  return CWorld::SetLodDist(dist);
}

bool CWorldParam::SmallCullCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  float thres;

  sscanf(newValue, "%f", &thres);
  if (thres < 0.001f || thres > 2.0f) {
    ConsoleWrite("SmallCull must be in range 0.001 - 2.0.", DEFAULT_COLOR);
    return false;
  }

  CWorldScene::cullSmallThreshold = thres;
  return true;
}

bool CWorldParam::DistCullCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  float dist = SStrToFloat(newValue);

  if (dist < 1.0f || dist > 888.8889f) {
    ConsoleWriteA("DistCull must be in range 1.0 - %f.", DEFAULT_COLOR, 888.8889f);
    return false;
  }

  CWorldScene::cullDistance = dist;
  return true;
}

bool CWorldParam::MaxLightsCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  int lights = SStrToInt(newValue);

  if (lights < 1 || (unsigned int)lights > 8) {
    ConsoleWriteA("MaxLights must be in range 1 - %i.", DEFAULT_COLOR, 8);
    return false;
  }

  CWorld::maxLights = lights;
  return true;
}

bool CWorldParam::ShadowLevelCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  if (SStrToInt(newValue) > 1) {
    ConsoleWrite("Shadow mip level must be in range 0 - 1.", DEFAULT_COLOR);
    return false;
  }

  ConsoleWrite("Shadow mip level changed upon restart.", DEFAULT_COLOR);
  return true;
}

bool CWorldParam::AlphaLevelCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  if (SStrToInt(newValue) > 1) {
    ConsoleWrite("Alpha mip level must be in range 0 - 1.", DEFAULT_COLOR);
    return false;
  }

  ConsoleWrite("Alpha mip level changed upon restart.", DEFAULT_COLOR);
  return true;
}

bool CWorldParam::TexLodBiasCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  float v = SStrToFloat(newValue);

  if (v < -1.0f || v > 1.0f) {
    ConsoleWrite("TexLodBias must be in range -1.0 - 1.0.", DEFAULT_COLOR);
    return false;
  }

  CWorld::SetTexLodBias(v);
  ModelSceneSetSharpness(v);
  return true;
}

bool CWorldParam::TrilinearCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  if (SStrToInt(newValue)) {
    ConsoleWrite("Trilinear filtering enabled upon restart.", DEFAULT_COLOR);
    CWorld::enables |= CWorld::Enable_Trilinear;
  } else {
    ConsoleWrite("Trilinear filtering disabled upon restart.", DEFAULT_COLOR);
    CWorld::enables &= ~CWorld::Enable_Trilinear;
  }

  return true;
}

bool CWorldParam::FarClipCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  float v = SStrToFloat(newValue);

  if (v < 177.0f || v > 777.0f) {
    ConsoleWrite("FarClip must be in range 177.0 - 777.0.", DEFAULT_COLOR);
    return false;
  }

  CWorld::SetFarClip(v);
  return true;
}

bool CWorldParam::NearClipCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  float v = SStrToFloat(newValue);

  if (v < 0.01f || v > 1.0f) {
    ConsoleWrite("NearClip must be in range 0.01 - 1.0", DEFAULT_COLOR);
    return false;
  }

  CWorld::SetNearClip(v);
  return true;
}

bool CWorldParam::FovCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  float fov = SStrToFloat(newValue);

  if (fov < 1.0f || fov > 179.0f) {
    ConsoleWrite("Fov must be in range 1.0 - 179.0.", DEFAULT_COLOR);
    return false;
  }

  return true;
}

bool CWorldParam::DetailDoodadDensityCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  unsigned int density = SStrToInt(newValue);

  if (density < 1 || density > 128) {
    ConsoleWriteA("Detail Doodad Density must be in range 1 - 128.", DEFAULT_COLOR);
    return false;
  }

  CWorld::SetDetailDoodadDensity(density);
  return true;
}

bool CWorldParam::SpecularCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  if (SStrToInt(newValue)) {
    if (GxCaps().m_pixelShaderTarget <= -1) {
      ConsoleWrite("Specular unsupported on current API/HW.", DEFAULT_COLOR);
      return false;
    }

    ConsoleWrite("Specular enabled", DEFAULT_COLOR);
    CWorld::enables |= CWorld::Enable_Specular;
    return true;
  }

  ConsoleWrite("Specular disabled.", DEFAULT_COLOR);
  CWorld::enables &= ~CWorld::Enable_Specular;
  return true;
}

bool CWorldParam::PixelShadersCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  if (SStrToInt(newValue)) {
    if (GxCaps().m_pixelShaderTarget <= -1) {
      ConsoleWrite("Pixel shaders unsupported on current API/HW.", DEFAULT_COLOR);
      return false;
    }

    ConsoleWrite("Pixel shaders enabled.", DEFAULT_COLOR);
    CWorld::enables |= CWorld::Enable_PixelShaders;
    return true;
  }

  ConsoleWrite("Pixel shaders disabled.", DEFAULT_COLOR);
  CWorld::enables &= ~CWorld::Enable_PixelShaders;
  return true;
}

bool CWorldParam::ParticleDensityCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  float v = SStrToFloat(newValue);

  if (v < 0.3f || v > 1.0f) {
    ConsoleWrite("Value must be between 0.3 and 1.0.", DEFAULT_COLOR);
    return false;
  }

  ParticleSystemManager::SetScaler(v);
  return true;
}

bool CWorldParam::UnitDrawDistCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  float dist = SStrToFloat(newValue);

  if (dist < 20.0f || dist > 150.0f) {
    ConsoleWrite("Value must be between 20 and 150.", DEFAULT_COLOR);
    return false;
  }

  CWorld::unitDrawDist = dist;
  return true;
}

bool CWorldParam::WaterLodCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  int lod = SStrToInt(newValue);

  if (lod < 0 || lod > 1) {
    ConsoleWrite("Water LOD must be between 0 and 1", DEFAULT_COLOR);
    return false;
  }

  CMapArea::ccWaterLOD = lod;
  return true;
}

bool CWorldParam::BaseMipCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  int mip = SStrToInt(newValue);

  if (mip < 0 || mip > 1) {
    ConsoleWrite("BaseMip must be 0 or 1", DEFAULT_COLOR);
    return false;
  }

  GxDevSetBaseMipLevel(mip);
  ConsoleWrite("Set upon game restart", DEFAULT_COLOR);
  return true;
}

bool CWorldParam::AnisotropicCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  int anisotropy = SStrToInt(newValue);

  if ((unsigned int)anisotropy < 1 || (unsigned int)anisotropy > 16) {
    ConsoleWrite("Anisotropy must be between 1 and 16 inclusive", DEFAULT_COLOR);
    return false;
  }

  if (anisotropy > (int)GxCaps().m_maxTexAnisotropy) {
    char msg[1024];

    SStrPrintf(msg, sizeof(msg), "max anisotropy is %d, not set", GxCaps().m_maxTexAnisotropy);
    ConsoleWrite(msg, DEFAULT_COLOR);
    return false;
  }

  if (anisotropy > 1) {
    CWorld::enables |= CWorld::Enable_Anisotropic;
    ConsoleWrite("anisotropic enabled, set upon game restart", DEFAULT_COLOR);
  } else {
    CWorld::enables &= ~CWorld::Enable_Anisotropic;
    ConsoleWrite("anisotropic disabled, set upon game restart", DEFAULT_COLOR);
  }

  CWorld::SetTexAnisotropy(anisotropy);
  return true;
}

bool CWorldParam::TextureLodDistCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  float dist;

  sscanf(newValue, "%f", &dist);
  if (dist < 80.0f || dist > 777.0f) {
    ConsoleWrite("Texure Lod distance must be in range 80.0f - 777.0f.", DEFAULT_COLOR);
    dist = 80.0f;
  }

  return CWorld::SetTextureLodDist(dist);
}
