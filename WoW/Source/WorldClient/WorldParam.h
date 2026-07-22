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

  static void __fastcall Initialize();
  static void __fastcall Destroy();

 private:
  static bool __fastcall LodCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool __fastcall FullAlphaCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool __fastcall DoodadAnimCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool __fastcall MapShadowsCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool __fastcall LightMapsCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool __fastcall LodDistCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool __fastcall SmallCullCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool __fastcall DistCullCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool __fastcall MaxLightsCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool __fastcall ShadowLevelCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool __fastcall AlphaLevelCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool __fastcall TexLodBiasCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool __fastcall TrilinearCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool __fastcall FarClipCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool __fastcall NearClipCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool __fastcall FovCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool __fastcall DetailDoodadDensityCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool __fastcall SpecularCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool __fastcall PixelShadersCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool __fastcall ParticleDensityCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool __fastcall UnitDrawDistCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool __fastcall WaterLodCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool __fastcall BaseMipCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool __fastcall AnisotropicCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
  static bool __fastcall TextureLodDistCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
};
