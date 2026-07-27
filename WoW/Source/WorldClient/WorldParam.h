#pragma once

struct CVar;

class CWorldParam {
 public:
  static CVar *cvar_triLinear;
  static CVar *cvar_detailDensity;
  static CVar *cvar_shadowLevel;
  static CVar *cvar_particleDensity;
  static CVar *cvar_pixelShaders;
  static CVar *cvar_lod;
  static CVar *cvar_fov;
  static CVar *cvar_anisotropic;
  static CVar *cvar_distCull;
  static CVar *cvar_doodadAnim;
  static CVar *cvar_textureLodDist;
  static CVar *cvar_mapShadows;
  static CVar *cvar_nearClip;
  static CVar *cvar_alphaLevel;
  static CVar *cvar_farClip;
  static CVar *cvar_lodDist;
  static CVar *cvar_unitDrawDist;
  static CVar *cvar_maxLights;
  static CVar *cvar_lightMaps;
  static CVar *cvar_waterLod;
  static CVar *cvar_baseMip;
  static CVar *cvar_texLodBias;
  static CVar *cvar_fullAlpha;
  static CVar *cvar_smallCull;
  static CVar *cvar_specular;

  static void Initialize();
  static void Destroy();

 private:
  static bool LodCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool FullAlphaCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool DoodadAnimCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool MapShadowsCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool LightMapsCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool LodDistCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool SmallCullCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool DistCullCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool MaxLightsCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool ShadowLevelCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool AlphaLevelCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool TexLodBiasCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool TrilinearCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool FarClipCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool NearClipCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool FovCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool DetailDoodadDensityCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool SpecularCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool PixelShadersCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool ParticleDensityCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool UnitDrawDistCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool WaterLodCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool BaseMipCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool AnisotropicCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool TextureLodDistCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
};
