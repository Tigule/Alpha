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
  unsigned int        time;
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
  unsigned char       eclipseAmount;
  NTempest::CImVector eclipseColor;
  CurrentLight        light;
  DNFogInfo           fogInfo;
  unsigned char       intFog;
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
  static unsigned short     m_idx[4];

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
  void         Initialize(const char *filename);
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

  void Initialize(const char *filename);
  void Destroy();
  void GenGeometry(
      NTempest::C3Vector  *geov,
      NTempest::C2Vector  *texv,
      NTempest::CImVector *clrv,
      unsigned short      *idx,
      unsigned long       &vertCount,
      unsigned long       &idxCount
  );
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
  friend void DayNightInitialize(const char *litFile);
  static const NTempest::C2Vector m_fadeTable[4];
  HMODEL__              *m_hModel;
  NTempest::CImVector    m_color;
  NTempest::C3Vector     m_pos;
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
  void  SetLOD(unsigned long newlod, unsigned long newUpdateSize);
  void  SetLayers(unsigned long layers) {
    m_nLayers = layers;
  }
  void SetDensity(float newDensity);
  void SetSharpness(float newSharpness);
  void OverrideDensitySharpness(float newDensity, float newSharpness);
  void Update();
 private:
  void BumpMap();
  void WorldToTexture(const NTempest::C3Vector &worldPt, NTempest::C2Vector &tex);

  static void Callback_GxTex(
      EGxTexCommand cmd,
      unsigned int  w,
      unsigned int  h,
      unsigned int  d,
      unsigned int  mipLevel,
      void         *userArg,
      unsigned int &texelStrideInBytes,
      const void  *&gxTexels
  );

  struct Vector3us {
    unsigned short x;
    unsigned short y;
    unsigned short z;
  };

  struct Octave {
    Vector3us    value;
    Vector3us    delta;
    Vector3us    min;
    Vector3us    max;
    float        amplitude;
    unsigned int permy00;
    unsigned int permy10;
    unsigned int permy01;
    unsigned int permy11;
    float        x000;
    float        x100;
    float        x010;
    float        x110;
    float        x001;
    float        x101;
    float        x011;
    float        x111;
    unsigned int iv;
    unsigned int lastiv;
  };

  static unsigned long      m_tmSizeTable[];
  static unsigned long      m_tmShiftTable[];
  static const NTempest::C2Vector m_bumpFadeTable[];
  static const float        BUMPFADETIME;

  unsigned int                      m_lastTime;
  float                             m_sharpness;
  unsigned char                     m_density;
  float                             m_densityOverride;
  unsigned long                     m_lod;
  unsigned long                     m_updateSize;
  unsigned long                     m_updateRow;
  unsigned long                     m_tmSize;
  unsigned long                     m_tmShift;
  unsigned long                     m_wrapMask;
  unsigned long                     m_nOctaves;
  unsigned long                     m_nLayers;
  TSFixedArray<NTempest::CImVector> m_texels;
  TSFixedArray<unsigned char>       m_height;
  TSFixedArray<float>               m_noise;
  TSFixedArray<float>               m_lastBumpNoiseY;
  TSFixedArray<NTempest::C2Vector>  m_bump;
  TSFixedArray<NTempest::C3Vector>  m_geoVerts;
  TSFixedArray<NTempest::C2Vector>  m_texVerts;
  TSFixedArray<unsigned short>      m_indices;
  unsigned short                    m_nIndices;
  unsigned short                    m_nVerts;
  unsigned short                    m_timeX;
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

  void GenTexture(unsigned int w, unsigned int h, NTempest::CImVector *texels);
  void GenSphere(float sphRadius);
  void SetColors();
  void Render();

 private:
  TSFixedArray<NTempest::C3Vector>  m_geoVerts;
  TSFixedArray<NTempest::CImVector> m_clrVerts;
  TSFixedArray<unsigned short>      m_indices;
  int                               m_sphThetaTess;
  unsigned short                    m_nVerts;
  unsigned short                    m_nIndices;
  float                             m_sphRadius;

  static const NTempest::C2Vector m_darkTable[];
  static const float              m_stripSizes[SKY_NUMBANDS];
  static const float              m_fadeAngle[SKY_NUMBANDS];
  static const float              m_darkAngle[SKY_NUMBANDS];
  static const NTempest::C2Vector m_fadeTable[];
};

DNInfo *DayNightGetInfo();
void DayNightInitialize(const char *litFile);
void DayNightForceFullUpdate();
void DayNightUpdateLighting();
float DayNightSI(float offset);
float DayNightUnitSelectColor();
void DayNightRenderGlares();
void DayNightRenderSky();
void DayNightSetEclipse(NTempest::CImVector color, float amount);
void DayNightDestroy();
void DayNightSkyTexCallback(
    EGxTexCommand cmd,
    unsigned int  w,
    unsigned int  h,
    unsigned int  d,
    unsigned int  mipLevel,
    void         *userArg,
    unsigned int &texelStrideInBytes,
    const void  *&texels
);

#endif
