#ifndef WOW_COMMON_DAYNIGHT_H
#define WOW_COMMON_DAYNIGHT_H

#include "LightsAndFog.h"
#include "Gx/Gx.h"
#include "Tempest/c2vector.h"
#include "Tempest/c3ivector.h"
#include "Tempest/c3vector.h"
#include "Tempest/c4vector.h"

#include <stpl.h>

struct HMODEL__;
struct HTEXTURE__;

struct DNFogInfo {
  NTempest::CImVector color;
  float               start;
  float               end;
};

struct DNLightInfo {
  NTempest::C3Vector  dir;
  NTempest::CImVector dirColor;
  NTempest::CImVector ambColor;
  NTempest::CImVector windowDirColor;
  NTempest::CImVector windowAmbColor;
  NTempest::C4Vector  shaderShadowColor;
};

struct DNInfo {
  UINT                time;
  float               dayProgression;
  float               day;
  NTempest::C3Vector  playerPos;
  NTempest::C3Vector  cameraPos;
  NTempest::C3Vector  cameraDir;
  float               faceAngle;
  float               nearClipScaled;
  float               farClip;
  float               elapsedSec;
  float               stormPercentage;
  BYTE                eclipseAmount;
  NTempest::CImVector eclipseColor;
  CurrentLight        light;
  DNFogInfo           fogInfo;
  BYTE                intFog;
  DNFogInfo           intFogInfo;
  DNLightInfo         lightInfo;
  NTempest::CImVector shadowClr;
  float               billbRescale;
  int                 showSky;
  CGxTex             *cloudTex;
  NTempest::C2Vector  sunPosTexPt;
  float               sunMoonPath;
  float               sidn;
  float               unitSelect;
};

class GlareBase {
 protected:
  static NTempest::C3Vector m_geov[4];
  static NTempest::C2Vector m_texv[4];
  static WORD               m_idx[4];

 public:
  virtual void Update(float elapsedSec) = 0;
  virtual void Render() = 0;
  virtual int  IsVisible() = 0;

  static int          m_masterEnable;
  int                 m_enabled;
  NTempest::C3Vector  m_pos;
  NTempest::CImVector m_color;
  HTEXTURE__         *m_texid;
  float               m_baseScale;
  float               m_curScale;
  float               m_fadeRate;
  float               m_opacity;
  float               m_targetOpacity;
};

class DNGlare : public GlareBase {
 public:
  void         Initialize(LPCSTR filename);
  virtual void Update(float elapsedSec);
  virtual void Render();
  virtual int  IsVisible();

  void Destroy();

  NTempest::C2Vector m_fadeTable[4];
  float              m_scaleMin;
  float              m_scaleMax;
  float              m_dotMin;
  float              m_alphaMin;
  float              m_alphaMax;

 protected:
  virtual float GetCloudDensityFade() = 0;
};

class DNSunGlare : public DNGlare {
 protected:
  virtual float GetCloudDensityFade();
};

class DNMoonGlare : public DNGlare {
 public:
  virtual float GetCloudDensityFade();
};

class DNPlanet {
 public:
  enum {
    SUN,
    MOON,
    MOON2
  };

  void Initialize(LPCSTR filename);
  void Destroy();
  void GenGeometry(NTempest::C3Vector *geov, NTempest::C2Vector *texv, NTempest::CImVector *clrv, WORD *idx, DWORD &vertCount, DWORD &idxCount);
  void Render();
  void Update();

 private:
  static const NTempest::C2Vector m_scaleTable[];

 public:
  NTempest::C3Vector  m_pos;
  NTempest::CImVector m_color;
  HTEXTURE__         *m_texid;
  float               m_scale;
  float               m_baseScale;
  float               m_period;
};

class DNStars {
 public:
  void Initialize();
  void Destroy();
  void Update();
  void Render();

 private:
  friend void                     DayNightInitialize(LPCSTR litFile);
  static const NTempest::C2Vector m_fadeTable[4];
  HMODEL__                       *m_hModel;
  NTempest::CImVector             m_color;
  NTempest::C3Vector              m_pos;
};

class DNClouds {
 public:
  DNClouds();
  void  Collide(const NTempest::C3Vector &origin, const NTempest::C3Vector &dir, NTempest::C3Vector &hitPoint);
  void  Destroy();
  void  FullUpdate();
  void  GenSphere(float size);
  float GetDensity(const NTempest::C3Vector &worldPoint, float area);
  void  Render();
  void  SetLOD(DWORD newlod, DWORD newUpdateSize);
  void  SetLayers(DWORD layers) {
    m_nLayers = layers;
  }
  void SetDensity(float newDensity);
  void SetSharpness(float newSharpness);
  void OverrideDensitySharpness(float newDensity, float newSharpness);
  void Update();

 private:
  void BumpMap();
  void WorldToTexture(const NTempest::C3Vector &worldPt, NTempest::C2Vector &tex);

  static void Callback_GxTex(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &gxTexels);

  struct Vector3us {
    WORD x;
    WORD y;
    WORD z;
  };

  struct Octave {
    Vector3us value;
    Vector3us delta;
    Vector3us min;
    Vector3us max;
    float     amplitude;
    UINT      permy00;
    UINT      permy10;
    UINT      permy01;
    UINT      permy11;
    float     x000;
    float     x100;
    float     x010;
    float     x110;
    float     x001;
    float     x101;
    float     x011;
    float     x111;
    UINT      iv;
    UINT      lastiv;
  };

  static DWORD                    m_tmSizeTable[];
  static DWORD                    m_tmShiftTable[];
  static const NTempest::C2Vector m_bumpFadeTable[];
  static const float              BUMPFADETIME;

  UINT                              m_lastTime;
  float                             m_sharpness;
  BYTE                              m_density;
  float                             m_densityOverride;
  DWORD                             m_lod;
  DWORD                             m_updateSize;
  DWORD                             m_updateRow;
  DWORD                             m_tmSize;
  DWORD                             m_tmShift;
  DWORD                             m_wrapMask;
  DWORD                             m_nOctaves;
  DWORD                             m_nLayers;
  TSFixedArray<NTempest::CImVector> m_texels;
  TSFixedArray<BYTE>                m_height;
  TSFixedArray<float>               m_noise;
  TSFixedArray<float>               m_lastBumpNoiseY;
  TSFixedArray<NTempest::C2Vector>  m_bump;
  TSFixedArray<NTempest::C3Vector>  m_geoVerts;
  TSFixedArray<NTempest::C2Vector>  m_texVerts;
  TSFixedArray<WORD>                m_indices;
  WORD                              m_nIndices;
  WORD                              m_nVerts;
  WORD                              m_timeX;
  DNFogInfo                         m_fogInfo;
  float                             m_waitTime;

 public:
  CGxTex *m_texid;
};

extern DNStars     s_stars;
extern DNMoonGlare s_moonGlare;
extern DNSunGlare  s_sunGlare;
extern DNPlanet    s_planets[4];

class DNSky {
 public:
  enum {
    SKY_NUMBANDS = 7
  };

  void GenTexture(UINT w, UINT h, NTempest::CImVector *texels);
  void GenSphere(float sphRadius);
  void SetColors();
  void Render();

 private:
  TSFixedArray<NTempest::C3Vector>  m_geoVerts;
  TSFixedArray<NTempest::CImVector> m_clrVerts;
  TSFixedArray<WORD>                m_indices;
  int                               m_sphThetaTess;
  WORD                              m_nVerts;
  WORD                              m_nIndices;
  float                             m_sphRadius;

  static const NTempest::C2Vector m_darkTable[];
  static const float              m_stripSizes[SKY_NUMBANDS];
  static const float              m_fadeAngle[SKY_NUMBANDS];
  static const float              m_darkAngle[SKY_NUMBANDS];
  static const NTempest::C2Vector m_fadeTable[];
};

DNInfo *DayNightGetInfo();
void    DayNightInitialize(LPCSTR litFile);
void    DayNightForceFullUpdate();
void    DayNightUpdateLighting();
float   DayNightSI(float offset);
float   DayNightUnitSelectColor();
void    DayNightRenderGlares();
void    DayNightRenderSky();
void    DayNightSetEclipse(NTempest::CImVector color, float amount);
void    DayNightDestroy();
void    DayNightSkyTexCallback(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &texels);

#endif
