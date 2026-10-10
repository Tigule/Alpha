#include <Base/Base.h>
#include <Gx/Gx.h>
#include <MapDefs.h>
#include <WorldClient/World.h>
#include <WowConst.h>
#include <DayNight.h>

#include "DayNight.h"
#include "Ftol.h"
#include "WorldClient/World.h"
#include "Tempest/cpriorityq.h"
#include "Base/Status.h"
#include "Model/IModel.h"
#include "Services/Texture.h"
#include "Console/ConsoleCommand.h"
#include "Console/ConsoleClient.h"
#include "Console/ConsoleVar.h"

#include <stdio.h>

const float DEFAULT_FOG_SCALER = 0.5f;

static CurrentLight s_magmaLight;
static CurrentLight s_slimeLight;
static DNInfo       s_dnInfo;

const float SUNRISE = 0.22916667f;
const float NOON = 0.5f;
const float SUNSET = 0.89583331f;
const float MOONRISE = 0.91666669f;
const float DAYEND = 1.0f;
const float MIDNIGHT = 0.0f;
const float MOONSET = 0.16666667f;

static int   s_initialized;
static int   s_paused;
static CVar *s_cloudLODCvar;
static float valueTable[256];
static BYTE  perm[256] = {225, 155, 210, 108, 175, 199, 221, 144, 203, 116, 70,  213, 69,  158, 33,  252, 5,   82,  173, 133, 222, 139, 174, 27,
                          9,   71,  90,  246, 75,  130, 91,  191, 169, 138, 2,   151, 194, 235, 81,  7,   25,  113, 228, 159, 205, 253, 134, 142,
                          248, 65,  224, 217, 22,  121, 229, 63,  89,  103, 96,  104, 156, 17,  201, 129, 36,  8,   165, 110, 237, 117, 231, 56,
                          132, 211, 152, 20,  181, 111, 239, 218, 170, 163, 51,  172, 157, 47,  80,  212, 176, 250, 87,  49,  99,  242, 136, 189,
                          162, 115, 44,  43,  124, 94,  150, 16,  141, 247, 32,  10,  198, 223, 255, 72,  53,  131, 84,  57,  220, 197, 58,  50,
                          208, 11,  241, 28,  3,   192, 62,  202, 18,  215, 153, 24,  76,  41,  15,  179, 39,  46,  55,  6,   128, 167, 23,  188,
                          106, 34,  187, 140, 164, 73,  112, 182, 244, 195, 227, 13,  35,  77,  196, 185, 26,  200, 226, 119, 31,  123, 168, 125,
                          249, 68,  183, 230, 177, 135, 160, 180, 12,  1,   243, 148, 102, 166, 38,  238, 251, 37,  240, 126, 64,  74,  161, 40,
                          184, 149, 171, 178, 101, 66,  29,  59,  146, 61,  254, 107, 42,  86,  154, 4,   236, 232, 120, 21,  233, 209, 45,  98,
                          193, 114, 78,  19,  206, 14,  118, 127, 48,  79,  147, 85,  30,  207, 219, 54,  88,  234, 190, 122, 95,  67,  143, 109,
                          137, 214, 145, 93,  92,  100, 245, 0,   216, 186, 60,  83,  105, 97,  204, 52};
static BYTE  cloudTable[256];
static float coserpTable[256];

DWORD DNClouds::m_tmSizeTable[] = {128, 256, 512, 1024, 2048};
DWORD DNClouds::m_tmShiftTable[] = {7, 8, 9, 10, 11};

const NTempest::C2Vector DNSky::m_darkTable[] = {NTempest::C2Vector(SUNRISE - 0.104166664f, 0.0f), NTempest::C2Vector(SUNRISE + 0.041666668f, 1.0f),
                                                 NTempest::C2Vector(SUNRISE + 0.0625f, 0.0f),     NTempest::C2Vector(SUNSET - 0.041666668f, 0.0f),
                                                 NTempest::C2Vector(SUNSET, 1.0f),                NTempest::C2Vector(SUNSET + 0.103472225f, 0.0f)};
const NTempest::C2Vector DNSky::m_fadeTable[] = {NTempest::C2Vector(0.125f, 1.0f), NTempest::C2Vector(0.375f, 0.0f),
                                                 NTempest::C2Vector(0.5f, -0.5f),  NTempest::C2Vector(0.625f, -0.69999999f),
                                                 NTempest::C2Vector(0.75f, -0.5f), NTempest::C2Vector(0.875f, 0.0f)};
const float DNSky::m_fadeAngle[DNSky::SKY_NUMBANDS] = {0.0f, PI * 0.5f, PI * 0.5f, PI * 0.5f, 0.0000001f, 0.0f, 0.0f};
const float DNSky::m_darkAngle[DNSky::SKY_NUMBANDS] = {0.0f, PI - m_fadeAngle[1], PI - m_fadeAngle[2], PI - m_fadeAngle[3], PI - m_fadeAngle[4], 0.0f, 0.0f};
const float DNSky::m_stripSizes[DNSky::SKY_NUMBANDS] = {0.0f, 0.35f, 0.41f, 0.46f, 0.49f, 0.5f, 1.0f};

const float DNClouds::BUMPFADETIME = 0.027777778f;
const NTempest::C2Vector DNClouds::m_bumpFadeTable[] = {NTempest::C2Vector(MOONSET, 1.0f),                 NTempest::C2Vector(MOONSET + BUMPFADETIME, 0.0f),
                                                        NTempest::C2Vector(SUNRISE - BUMPFADETIME, 0.0f),  NTempest::C2Vector(SUNRISE, 1.0f),
                                                        NTempest::C2Vector(SUNSET, 1.0f),                  NTempest::C2Vector(SUNSET + BUMPFADETIME, 0.0f),
                                                        NTempest::C2Vector(MOONRISE - BUMPFADETIME, 0.0f), NTempest::C2Vector(MOONRISE, 1.0f)};

const NTempest::C2Vector DNStars::m_fadeTable[4] = {
    NTempest::C2Vector(SUNRISE - 0.104166664f, 1.0f), NTempest::C2Vector(SUNRISE - 0.041666668f, 0.0f),
    NTempest::C2Vector(SUNSET + 0.041666668f, 0.0f), NTempest::C2Vector(SUNSET + 0.104166664f, 1.0f)
};

const NTempest::C2Vector DNPlanet::m_scaleTable[8] = {
    NTempest::C2Vector(MOONSET - 0.041666668f, 2.0f), NTempest::C2Vector(MOONSET, 4.0f),
    NTempest::C2Vector(SUNRISE, 4.0f),                NTempest::C2Vector(SUNRISE + 0.041666668f, 2.0f),
    NTempest::C2Vector(SUNSET - 0.083333336f, 2.0f),  NTempest::C2Vector(SUNSET, 4.0f),
    NTempest::C2Vector(MOONRISE, 4.0f),               NTempest::C2Vector(MOONRISE + 0.041666668f, 2.0f)
};

static const NTempest::C2Vector s_sidnTable[4] = {
    NTempest::C2Vector(0.25f, 1.0f), NTempest::C2Vector(0.29166666f, 0.0f), NTempest::C2Vector(SUNSET - 0.041666668f, 0.0f),
    NTempest::C2Vector(SUNSET, 1.0f)
};
static const NTempest::C2Vector s_unitColorTable[2] = {NTempest::C2Vector(0.083333336f, 0.25f), NTempest::C2Vector(0.5f, 1.0f)};

static DNSky        s_sky;
static DNClouds     s_clouds;
LightGroup          g_areaLights;
DNPlanet    s_planets[4];
DNSunGlare  s_sunGlare;
DNMoonGlare s_moonGlare;
DNStars     s_stars;

static void ValueTableInit() {
  int lp;

  for (lp = 0; lp < 256; ++lp) {
    valueTable[lp] = 1.0f - 2.0f * (rand() * 0.000030518509f);
  }

  for (lp = 0; lp < 256; ++lp) {
    coserpTable[lp] = (1.0f - NTempest::CMath::cos_(lp * PI * 0.00390625f)) * 0.5f;
  }
}

static inline float Interp(float range1, float range2, float percent) {
  if (range2 >= range1) {
    percent *= range2 - range1;
    return range1 + percent;
  }

  percent *= range1 - range2;
  return range1 - percent;
}

class LightQE {
 public:
  float dist;
  int   subscript;

  LightQE(float pDist, int pSubscript) : dist(pDist), subscript(pSubscript) {
  }

  static bool HasHigherPriority(const LightQE &a, const LightQE &b) {
    return a.dist >= b.dist;
  }
};

static float InterpTable(const NTempest::C2Vector *table, DWORD size, float key) {
  DWORD next;
  for (next = 0; next < size; ++next) {
    if (key <= table[next].x) {
      break;
    }
  }

  DWORD previous;
  if (next == size) {
    next = 0;
    previous = size - 1;
  } else if (!next) {
    previous = size - 1;
  } else {
    previous = next - 1;
  }

  float range = table[next].x - table[previous].x;
  if (range < 0.0f) {
    range += 1.0f;
  }

  float position = key - table[previous].x;
  if (position < 0.0f) {
    position += 1.0f;
  }

  return Interp(table[previous].y, table[next].y, position / range);
}

static void ScaleOutputs(CurrentLight &globalLight, CurrentLight &areaLight, float scale) {
  int i;

  globalLight.AmbientColor = NTempest::CImVector(
      255, Fast_ftol(Interp(globalLight.AmbientColor.r, areaLight.AmbientColor.r, scale)),
      Fast_ftol(Interp(globalLight.AmbientColor.g, areaLight.AmbientColor.g, scale)),
      Fast_ftol(Interp(globalLight.AmbientColor.b, areaLight.AmbientColor.b, scale))
  );
  globalLight.DirectColor = NTempest::CImVector(
      255, Fast_ftol(Interp(globalLight.DirectColor.r, areaLight.DirectColor.r, scale)),
      Fast_ftol(Interp(globalLight.DirectColor.g, areaLight.DirectColor.g, scale)),
      Fast_ftol(Interp(globalLight.DirectColor.b, areaLight.DirectColor.b, scale))
  );
  globalLight.ShadowOpacity = NTempest::CImVector(
      255, Fast_ftol(Interp(globalLight.ShadowOpacity.r, areaLight.ShadowOpacity.r, scale)),
      Fast_ftol(Interp(globalLight.ShadowOpacity.g, areaLight.ShadowOpacity.g, scale)),
      Fast_ftol(Interp(globalLight.ShadowOpacity.b, areaLight.ShadowOpacity.b, scale))
  );

  for (i = 0; i < 6; ++i) {
    globalLight.SkyArray[i] = NTempest::CImVector(
        255, Fast_ftol(Interp(globalLight.SkyArray[i].r, areaLight.SkyArray[i].r, scale)),
        Fast_ftol(Interp(globalLight.SkyArray[i].g, areaLight.SkyArray[i].g, scale)),
        Fast_ftol(Interp(globalLight.SkyArray[i].b, areaLight.SkyArray[i].b, scale))
    );
  }

  globalLight.FogEnd = Interp(globalLight.FogEnd, areaLight.FogEnd, scale);
  globalLight.FogStartScalar = Interp(globalLight.FogStartScalar, areaLight.FogStartScalar, scale);
  globalLight.Darkness = Interp(globalLight.Darkness, areaLight.Darkness, scale);

  for (i = 0; i < 5; ++i) {
    globalLight.CloudArray[i] = NTempest::CImVector(
        255, Fast_ftol(Interp(globalLight.CloudArray[i].r, areaLight.CloudArray[i].r, scale)),
        Fast_ftol(Interp(globalLight.CloudArray[i].g, areaLight.CloudArray[i].g, scale)),
        Fast_ftol(Interp(globalLight.CloudArray[i].b, areaLight.CloudArray[i].b, scale))
    );
  }

  for (i = 0; i < 4; ++i) {
    globalLight.WaterArray[i] = NTempest::CImVector(
        255, Fast_ftol(Interp(globalLight.WaterArray[i].r, areaLight.WaterArray[i].r, scale)),
        Fast_ftol(Interp(globalLight.WaterArray[i].g, areaLight.WaterArray[i].g, scale)),
        Fast_ftol(Interp(globalLight.WaterArray[i].b, areaLight.WaterArray[i].b, scale))
    );
  }

  globalLight.CloudData[1] = Interp(globalLight.CloudData[1], areaLight.CloudData[1], scale);
}

static void DoAreaLights(int underWater) {
  NTempest::CPriorityQ<LightQE, LightQE> lightq;

  for (UINT i = 1; i < g_areaLights.m_lightData.Count(); ++i) {
    LightData &light = g_areaLights.m_lightData[i];
    float      dist = (s_dnInfo.cameraPos - light.m_lightlist.m_lightLocation).Mag();
    if (dist < light.m_lightlist.m_lightDropoff) {
      lightq.Enqueue(LightQE(dist, i));
    }
  }

  while (lightq.HasEntries()) {
    LightQE        entry = lightq.Dequeue();
    LightData     &light = g_areaLights.m_lightData[entry.subscript];
    LightDataItem *lightdata = underWater ? &light.m_lightdataWater : &light.m_lightdata;
    LightDataItem *stormdata = underWater ? &light.m_stormdataWater : &light.m_stormdata;

    CurrentLight areaLight;
    CalcLightColors(Fast_ftol(s_dnInfo.dayProgression * 2880.0f), &areaLight, lightdata, stormdata, 0);

    float alpha = 1.0f;
    if (entry.dist > light.m_lightlist.m_lightRadius) {
      alpha = 1.0f - (entry.dist - light.m_lightlist.m_lightRadius) / (light.m_lightlist.m_lightDropoff - light.m_lightlist.m_lightRadius);
    }
    ScaleOutputs(s_dnInfo.light, areaLight, alpha);
  }
}

static void ResetLightPos() {
  NTempest::C2Vector offset(0.0f, 0.0f);

  if (!CWorld::MapIsDungeon()) {
    offset = NTempest::C2Vector(17066.666f, 17066.666f);
  }

  for (int lp = 0; lp < (int)g_areaLights.m_lightData.Count(); ++lp) {
    LightData &light = g_areaLights.m_lightData[lp];

    if (lp >= 1) {
      light.m_lightlist.m_lightLocation *= 0.027777778f;

      NTempest::C3Vector pos(-(light.m_lightlist.m_lightLocation.z - offset.x), -(light.m_lightlist.m_lightLocation.x - offset.y), light.m_lightlist.m_lightLocation.y);
      light.m_lightlist.m_lightLocation = pos;
      light.m_lightlist.m_lightRadius *= 0.027777778f;
      light.m_lightlist.m_lightDropoff *= 0.027777778f;
    }

    UINT i;
    UINT count = light.m_lightdata.m_fogData.Count();
    for (i = 0; i < count; ++i) {
      light.m_lightdata.m_fogData[i].m_fogEnd *= 0.027777778f;
    }
    count = light.m_stormdata.m_fogData.Count();
    for (i = 0; i < count; ++i) {
      light.m_stormdata.m_fogData[i].m_fogEnd *= 0.027777778f;
    }
    count = light.m_lightdataWater.m_fogData.Count();
    for (i = 0; i < count; ++i) {
      light.m_lightdataWater.m_fogData[i].m_fogEnd *= 0.027777778f;
    }
    count = light.m_stormdataWater.m_fogData.Count();
    for (i = 0; i < count; ++i) {
      light.m_stormdataWater.m_fogData[i].m_fogEnd *= 0.027777778f;
    }
  }
}

static NTempest::CImVector DarkenColor(const NTempest::CImVector clr, float amount) {
  NTempest::C3Vector rgb(clr.r * 0.0039215689f, clr.g * 0.0039215689f, clr.b * 0.0039215689f);
  NTempest::C3Vector hsv;
  NTempest::RGBtoHSV(rgb, hsv);
  hsv.z *= amount;
  NTempest::HSVtoRGB(hsv, rgb);
  return NTempest::CImVector(
      255, NTempest::CMath::ftol_0_256_(rgb.x * 255.0f), NTempest::CMath::ftol_0_256_(rgb.y * 255.0f), NTempest::CMath::ftol_0_256_(rgb.z * 255.0f)
  );
}

static void SetLightColors() {
  s_dnInfo.lightInfo.dirColor = s_dnInfo.light.DirectColor;
  s_dnInfo.lightInfo.ambColor = s_dnInfo.light.AmbientColor;
  s_dnInfo.lightInfo.windowDirColor = s_dnInfo.lightInfo.dirColor;
  s_dnInfo.lightInfo.windowAmbColor = s_dnInfo.lightInfo.ambColor;
  s_dnInfo.lightInfo.windowAmbColor.Blend255(128, s_dnInfo.lightInfo.dirColor);
  s_dnInfo.lightInfo.windowAmbColor =
      ((((s_dnInfo.lightInfo.windowAmbColor ^ (s_dnInfo.lightInfo.windowAmbColor + 0xFF101010)) ^ 0xFFFEFEFF) & 0x01010100) -
       ((((s_dnInfo.lightInfo.windowAmbColor ^ (s_dnInfo.lightInfo.windowAmbColor + 0xFF101010)) ^ 0xFFFEFEFF) & 0x01010100) >> 8)) |
      ((s_dnInfo.lightInfo.windowAmbColor + 0xFF101010) -
       (((s_dnInfo.lightInfo.windowAmbColor ^ (s_dnInfo.lightInfo.windowAmbColor + 0xFF101010)) ^ 0xFFFEFEFF) & 0x01010100));
  s_dnInfo.lightInfo.windowDirColor.Blend255(128, s_dnInfo.lightInfo.ambColor);

  s_dnInfo.shadowClr.Set(
      s_dnInfo.light.ShadowOpacity.r, (BYTE)((85 * s_dnInfo.light.AmbientColor.r + 255) >> 8),
      (BYTE)((85 * s_dnInfo.light.AmbientColor.g + 255) >> 8), (BYTE)((85 * s_dnInfo.light.AmbientColor.b + 255) >> 8)
  );

  static const float HUE_SCALE = 1.0f;
  static const float VALUE_SCALE = 1.25f;
  static const float SATURATION_SCALE = 0.33f;
  NTempest::C3Vector rgb = s_dnInfo.lightInfo.ambColor;
  NTempest::C3Vector hsv;
  NTempest::RGBtoHSV(rgb, hsv);
  hsv.x *= HUE_SCALE;
  hsv.y *= SATURATION_SCALE;
  hsv.z *= VALUE_SCALE;
  NTempest::HSVtoRGB(hsv, rgb);
  s_dnInfo.lightInfo.shaderShadowColor.x = rgb.x;
  s_dnInfo.lightInfo.shaderShadowColor.y = rgb.y;
  s_dnInfo.lightInfo.shaderShadowColor.z = rgb.z;
  s_dnInfo.lightInfo.shaderShadowColor.w = 1.0f;
}

static void SetFogColors() {
  s_dnInfo.fogInfo.end = s_dnInfo.light.FogEnd < s_dnInfo.farClip ? s_dnInfo.light.FogEnd : s_dnInfo.farClip;
  s_dnInfo.fogInfo.start = s_dnInfo.light.FogStartScalar * s_dnInfo.fogInfo.end;
  s_dnInfo.fogInfo.color = s_dnInfo.light.SkyArray[5];

  SMOFog::Fogs intFog;
  float        intPct;
  s_dnInfo.intFog = CWorld::QueryMapObjFog(0, intFog, intPct) == 1;
  if (!s_dnInfo.intFog) {
    return;
  }

  UINT liquid = CWorld::SceneCamLiquidStatus();
  if (liquid != 15) {
    liquid &= 3;
  }

  if (liquid == 2 || liquid == 3) {
    s_dnInfo.intFogInfo = s_dnInfo.fogInfo;
  } else {
    int          underWater = liquid == 1 || liquid == 0;
    SMOFog::Fog &fog = intFog[underWater ? 1 : 0];
    float        fogEnd = min(fog.end, s_dnInfo.farClip);
    s_dnInfo.intFogInfo.end = (fogEnd - s_dnInfo.fogInfo.end) * intPct + s_dnInfo.fogInfo.end;
    s_dnInfo.intFogInfo.start = ((fog.startScalar - s_dnInfo.light.FogStartScalar) * intPct + s_dnInfo.light.FogStartScalar) * s_dnInfo.intFogInfo.end;
    s_dnInfo.intFogInfo.color.Blend255RGB(NTempest::CMath::ftol_0_256_(intPct * 255.0f), fog.color);
  }
}

static void SetColors() {
  UINT camLiquid = CWorld::SceneCamLiquidStatus();
  int  underWater = (camLiquid & 0xF) == 1 || (camLiquid & 0xF) == 0;

  if (g_areaLights.m_lightData.Count()) {
    if (camLiquid == 15 || underWater) {
      LightData     &global = g_areaLights.m_lightData[0];
      LightDataItem *lightdata = underWater ? &global.m_lightdataWater : &global.m_lightdata;
      LightDataItem *stormdata = underWater ? &global.m_stormdataWater : &global.m_stormdata;
      CalcLightColors(Fast_ftol(s_dnInfo.dayProgression * 2880.0f), &s_dnInfo.light, lightdata, stormdata, s_dnInfo.stormPercentage * 100.0f);
      DoAreaLights(underWater);
    } else {
      switch (camLiquid & 0xF) {
        case 2: s_dnInfo.light = s_magmaLight; break;
        case 3: s_dnInfo.light = s_slimeLight; break;
      }
    }
  } else {
    memset(&s_dnInfo.light, 0xFF, sizeof(s_dnInfo.light));
    s_dnInfo.light.FogEnd = 10000000000.0f;
    s_dnInfo.light.FogStartScalar = DEFAULT_FOG_SCALER;
  }

  SetLightColors();
  SetFogColors();
  if (underWater) {
    float darken = 1.0f;
    switch (camLiquid) {
      case 1:
        darken = 1.0f - max(s_dnInfo.cameraPos.z, -30.0f) * -0.033333335f;
        break;
    }
    s_dnInfo.fogInfo.color = DarkenColor(s_dnInfo.fogInfo.color, (darken + 1.0f) * 0.5f);
    s_dnInfo.lightInfo.ambColor = DarkenColor(s_dnInfo.lightInfo.ambColor, (darken + 1.0f) * 0.5f);
    s_dnInfo.lightInfo.dirColor = DarkenColor(s_dnInfo.lightInfo.dirColor, darken * 0.25f + 0.75f);
  }

  s_sky.SetColors();
  s_planets[0].m_color = s_dnInfo.light.CloudArray[0];
  s_sunGlare.m_color = s_dnInfo.light.CloudArray[0];
  s_planets[1].m_color = s_dnInfo.light.CloudArray[0];
  s_moonGlare.m_color = s_dnInfo.light.CloudArray[0];
  if (s_dnInfo.eclipseAmount != 0.0f) {
    s_dnInfo.fogInfo.color.Blend255RGB(s_dnInfo.eclipseAmount, s_dnInfo.eclipseColor);
    s_dnInfo.lightInfo.ambColor.Blend255RGB(s_dnInfo.eclipseAmount, s_dnInfo.eclipseColor);
    s_dnInfo.lightInfo.dirColor.Blend255RGB(s_dnInfo.eclipseAmount, s_dnInfo.eclipseColor);
    s_planets[0].m_color.Blend255RGB(s_dnInfo.eclipseAmount, s_dnInfo.eclipseColor);
    s_planets[1].m_color.Blend255RGB(s_dnInfo.eclipseAmount, s_dnInfo.eclipseColor);
  }
  s_dnInfo.sidn = DayNightSI(0.0f);
  s_dnInfo.unitSelect = DayNightUnitSelectColor();
}

static void SetDirection() {
  static NTempest::C2Vector phiTable[4] = {
      NTempest::C2Vector(0.0f, PI * 0.70555556f), NTempest::C2Vector(0.25f, PI * 0.6111111f), NTempest::C2Vector(0.5f, PI * 0.70555556f),
      NTempest::C2Vector(0.75f, PI * 0.6111111f)
  };
  static NTempest::C2Vector thetaTable[4] = {
      NTempest::C2Vector(0.0f, PI * 1.25f), NTempest::C2Vector(0.25f, PI * 1.25f), NTempest::C2Vector(0.5f, PI * 1.25f),
      NTempest::C2Vector(0.75f, PI * 1.25f)
  };

  float phi = InterpTable(phiTable, 4, s_dnInfo.dayProgression);
  float theta = InterpTable(thetaTable, 4, s_dnInfo.dayProgression);
  s_dnInfo.lightInfo.dir.x = cos(theta) * sin(phi);
  s_dnInfo.lightInfo.dir.y = sin(theta) * sin(phi);
  s_dnInfo.lightInfo.dir.z = cos(phi);
}

static void SetPlanets() {
  s_dnInfo.billbRescale = (s_dnInfo.nearClipScaled + 10.0f) * 0.1f;

  static NTempest::C2Vector sunPhiTable[5] = {
      NTempest::C2Vector(SUNRISE, PI * 0.55555558f), NTempest::C2Vector(NOON - 0.0034722222f, PI * 0.027777778f),
      NTempest::C2Vector(NOON, PI * 0.027777778f), NTempest::C2Vector(NOON + 0.0034722222f, PI * 0.027777778f),
      NTempest::C2Vector(SUNSET, PI * 0.55555558f)
  };
  static NTempest::C2Vector sunThetaTable[3] = {
      NTempest::C2Vector(SUNRISE, PI * 0.25f), NTempest::C2Vector(NOON, PI * 0.25f), NTempest::C2Vector(SUNSET, PI * 0.25f)
  };
  static NTempest::C2Vector moonPhiTable[5] = {
      NTempest::C2Vector(MIDNIGHT, PI * 0.19444445f), NTempest::C2Vector(MIDNIGHT + 0.0034722222f, PI * 0.19444445f),
      NTempest::C2Vector(MOONSET, PI * 0.55555558f), NTempest::C2Vector(MOONRISE, PI * 0.55555558f),
      NTempest::C2Vector(0.99652779f, PI * 0.19444445f)
  };
  static NTempest::C2Vector moonThetaTable[3] = {
      NTempest::C2Vector(MIDNIGHT, PI * 0.25f), NTempest::C2Vector(MOONSET, PI * 0.25f), NTempest::C2Vector(MOONRISE, PI * 0.25f)
  };
  static NTempest::C2Vector moon2PhiTable[5] = {
      NTempest::C2Vector(MIDNIGHT, PI * 0.19444445f), NTempest::C2Vector(MIDNIGHT + 0.0034722222f, PI * 0.19444445f),
      NTempest::C2Vector(MOONSET, PI * 0.55555558f), NTempest::C2Vector(MOONRISE, PI * 0.55555558f),
      NTempest::C2Vector(0.99652779f, PI * 0.19444445f)
  };
  static NTempest::C2Vector moon2ThetaTable[3] = {
      NTempest::C2Vector(MIDNIGHT, PI * 0.75f), NTempest::C2Vector(MOONSET, PI * 0.83333331f), NTempest::C2Vector(MOONRISE, PI * 0.91666669f)
  };
  static NTempest::C2Vector sunScaleTable[4] = {
      NTempest::C2Vector(SUNRISE + 0.020833334f, 2.0f), NTempest::C2Vector(SUNRISE + 0.052083332f, 1.0f),
      NTempest::C2Vector(SUNSET - 0.052083332f, 1.0f), NTempest::C2Vector(SUNSET - 0.020833334f, 2.0f)
  };
  static NTempest::C2Vector moonScaleTable[4] = {
      NTempest::C2Vector(MOONSET - 0.125f, 1.0f), NTempest::C2Vector(MOONSET, 1.5f), NTempest::C2Vector(MOONRISE, 1.5f),
      NTempest::C2Vector(MOONRISE + 0.082638889f, 1.0f)
  };

  float phi = InterpTable(sunPhiTable, 5, s_dnInfo.dayProgression);
  float theta = InterpTable(sunThetaTable, 3, s_dnInfo.dayProgression);
  s_planets[0].m_pos.x = cos(theta) * sin(phi);
  s_planets[0].m_pos.y = sin(theta) * sin(phi);
  s_planets[0].m_pos.z = cos(phi);
  s_planets[0].m_pos *= 12.0f / NTempest::CMath::hypot_(s_planets[0].m_pos.x, s_planets[0].m_pos.y, s_planets[0].m_pos.z);
  s_planets[0].m_pos += s_dnInfo.cameraPos;
  s_planets[0].m_scale = InterpTable(sunScaleTable, 4, s_dnInfo.dayProgression) * s_planets[0].m_baseScale * s_dnInfo.billbRescale;
  s_sunGlare.m_pos = s_planets[0].m_pos;

  phi = InterpTable(moonPhiTable, 5, s_dnInfo.dayProgression);
  theta = InterpTable(moonThetaTable, 3, s_dnInfo.dayProgression);
  s_planets[1].m_pos.x = cos(theta) * sin(phi);
  s_planets[1].m_pos.y = sin(theta) * sin(phi);
  s_planets[1].m_pos.z = cos(phi);
  s_planets[1].m_pos *= 12.0f / NTempest::CMath::hypot_(s_planets[1].m_pos.x, s_planets[1].m_pos.y, s_planets[1].m_pos.z);
  s_planets[1].m_pos += s_dnInfo.cameraPos;
  s_planets[1].m_scale = InterpTable(moonScaleTable, 4, s_dnInfo.dayProgression) * s_planets[1].m_baseScale * s_dnInfo.billbRescale;
  s_moonGlare.m_pos = s_planets[1].m_pos;
  s_moonGlare.m_scaleMax = s_moonGlare.m_scaleMin = s_planets[1].m_scale * s_dnInfo.billbRescale;

  s_sunGlare.Update(s_dnInfo.elapsedSec);
  s_moonGlare.Update(s_dnInfo.elapsedSec);

  UINT curTime = Fast_ftol(s_dnInfo.day * 65536.0f) + Fast_ftol(s_dnInfo.dayProgression * 65536.0f);
  UINT periodStart =
      Fast_ftol(s_planets[2].m_period * (float)floor((s_dnInfo.day + s_dnInfo.dayProgression) / s_planets[2].m_period) * 65536.0f);
  if (periodStart >= curTime) {
    periodStart = curTime;
  }
  float moon2t = (curTime - periodStart) * 0.000015258789f / s_planets[2].m_period;
  phi = InterpTable(moon2PhiTable, 5, moon2t);
  theta = InterpTable(moon2ThetaTable, 3, moon2t);
  s_planets[2].m_pos.x = cos(theta) * sin(phi);
  s_planets[2].m_pos.y = sin(theta) * sin(phi);
  s_planets[2].m_pos.z = cos(phi);
  s_planets[2].m_pos *= 12.0f / NTempest::CMath::hypot_(s_planets[2].m_pos.x, s_planets[2].m_pos.y, s_planets[2].m_pos.z);
  s_planets[2].m_pos += s_dnInfo.cameraPos;
  s_planets[2].m_scale = InterpTable(moonScaleTable, 4, moon2t) * s_planets[2].m_baseScale * s_dnInfo.billbRescale;

  if (s_dnInfo.dayProgression >= SUNRISE && s_dnInfo.dayProgression < NOON) {
    s_dnInfo.sunMoonPath = (s_dnInfo.dayProgression - SUNRISE) / (NOON - SUNRISE);
  } else if (s_dnInfo.dayProgression >= NOON && s_dnInfo.dayProgression < SUNSET) {
    s_dnInfo.sunMoonPath = 1.0f - (s_dnInfo.dayProgression - NOON) / (SUNSET - NOON);
  } else if (s_dnInfo.dayProgression >= MOONRISE && s_dnInfo.dayProgression < DAYEND) {
    s_dnInfo.sunMoonPath = (s_dnInfo.dayProgression - MOONRISE) / (DAYEND - MOONRISE);
  } else if (s_dnInfo.dayProgression >= MIDNIGHT && s_dnInfo.dayProgression < MOONSET) {
    s_dnInfo.sunMoonPath = 1.0f - (s_dnInfo.dayProgression - MIDNIGHT) / (MOONSET - MIDNIGHT);
  } else {
    s_dnInfo.sunMoonPath = 0.0f;
  }
}

int                GlareBase::m_masterEnable;
NTempest::C3Vector GlareBase::m_geov[4] = {
    NTempest::C3Vector(0.0f, -0.5f, 0.5f), NTempest::C3Vector(0.0f, 0.5f, 0.5f), NTempest::C3Vector(0.0f, -0.5f, -0.5f),
    NTempest::C3Vector(0.0f, 0.5f, -0.5f)
};
NTempest::C2Vector GlareBase::m_texv[4] = {
    NTempest::C2Vector(0.0f, 0.0f), NTempest::C2Vector(1.0f, 0.0f), NTempest::C2Vector(0.0f, 1.0f), NTempest::C2Vector(1.0f, 1.0f)
};
WORD GlareBase::m_idx[4] = {0, 2, 1, 3};

void DNGlare::Update(float elapsedSec) {
  m_targetOpacity = 1.0f;

  NTempest::C3Vector glareDir = m_pos - s_dnInfo.cameraPos;
  glareDir.Normalize();

  m_targetOpacity *= GetCloudDensityFade();
  m_targetOpacity *= InterpTable(m_fadeTable, 4, s_dnInfo.dayProgression);
  if (m_targetOpacity != 0.0f && !IsVisible()) {
    m_targetOpacity = 0.0f;
  }

  if (m_targetOpacity > m_opacity) {
    m_opacity += elapsedSec * m_fadeRate;
    if (m_opacity > m_targetOpacity) {
      m_opacity = m_targetOpacity;
    }
  } else if (m_targetOpacity < m_opacity) {
    m_opacity -= elapsedSec * m_fadeRate;
    if (m_opacity < m_targetOpacity) {
      m_opacity = m_targetOpacity;
    }
  }

  float dot = NTempest::C3Vector::Dot(s_dnInfo.cameraDir, glareDir);
  if (dot < m_dotMin) {
    dot = m_dotMin;
  }
  float pct = (dot - m_dotMin) / (1.0f - m_dotMin);
  m_curScale = ((m_scaleMax - m_scaleMin) * pct + m_scaleMin) * m_baseScale * s_dnInfo.billbRescale;
  m_color.a = (BYTE)(((m_alphaMax - m_alphaMin) * pct + m_alphaMin) * m_opacity * 255.0f);
}

float DNSunGlare::GetCloudDensityFade() {
  return 1.0f - s_clouds.GetDensity(m_pos, 1.0f);
}

float DNMoonGlare::GetCloudDensityFade() {
  float density = s_clouds.GetDensity(m_pos, 1.0f);
  return 1.0f - NTempest::CMath::fabs_((density - 0.5f) + (density - 0.5f));
}

void DNClouds::WorldToTexture(const NTempest::C3Vector &worldPt, NTempest::C2Vector &tex) {
  NTempest::C3Vector localPt = worldPt - s_dnInfo.cameraPos;
  NTempest::C3Vector sphColpt = localPt - NTempest::C3Vector(0.0f, 0.0f, -736.0f);
  NTempest::C3Vector up(0.0f, 0.0f, 800.0f);
  float              texRadius;
  float              idenom;

  float angle = NTempest::CMath::acos_(NTempest::C3Vector::Dot(up, sphColpt) / (up.Mag() * sphColpt.Mag()));
  if (angle > PI * 0.22222222f) {
    angle = PI * 0.22222222f;
  }

  texRadius = (angle - PI * 0.0f) * 0.5f / ((PI * 0.22222222f - PI * 0.0f) - (PI * 0.22222222f - PI * 0.0f) * 0.25f);
  idenom = NTempest::CMath::sqrt_(localPt.x * localPt.x + localPt.y * localPt.y);
  if (idenom > 0.00001) {
    idenom = 1.0f / idenom;
    localPt.x *= idenom;
    localPt.y *= idenom;
  } else {
    localPt.x = 0.0f;
    localPt.y = 0.0f;
  }

  tex.x = (localPt.x * texRadius + 0.5f) * m_tmSize;
  tex.y = (localPt.y * texRadius + 0.5f) * m_tmSize;
}

void DNClouds::Collide(const NTempest::C3Vector &origin, const NTempest::C3Vector &dir, NTempest::C3Vector &hitPoint) {
  float r1;
  float r2;

  FATALASSERT(origin.x == 0 && origin.y == 0 && origin.z == 0);

  NTempest::CMath::solvequad_(dir.SquaredMag(), dir.z * 1472.0f, -98304.0f, r1, r2);
  hitPoint = s_dnInfo.cameraPos + (origin + dir * r2);
}

float DNClouds::GetDensity(const NTempest::C3Vector &worldPoint, float area) {
  if (!m_nLayers) {
    return 0.0f;
  }

  NTempest::C2Vector texv(0.0f, 0.0f);
  WorldToTexture(worldPoint, texv);
  NTempest::C2iVector texel(texv.x, texv.y);
  return m_height[(texel.y << m_tmShift) + texel.x] * 0.0039215689f;
}

void DNClouds::BumpMap() {
  NTempest::C2Vector  *clbump = &m_bump[m_updateRow << m_tmShift];
  BYTE                *clheight = &m_height[m_updateRow << m_tmShift];
  NTempest::CImVector *cltexels = &m_texels[m_updateRow << m_tmShift];
  NTempest::C3Vector   rayDir;

  if (s_dnInfo.dayProgression >= SUNRISE - BUMPFADETIME && s_dnInfo.dayProgression <= SUNSET + BUMPFADETIME) {
    rayDir = s_planets[0].m_pos;
  } else {
    rayDir = s_planets[1].m_pos;
  }
  rayDir -= s_dnInfo.cameraPos;

  NTempest::C3Vector rayOrg(0.0f);
  NTempest::C3Vector colPt(0.0f);
  NTempest::C2Vector texPt(0.0f);
  Collide(rayOrg, rayDir, colPt);
  WorldToTexture(colPt, texPt);
  s_dnInfo.sunPosTexPt = texPt;
  s_dnInfo.sunPosTexPt /= m_tmSize;

  NTempest::C3Vector sunLightPos(texPt.x, texPt.y, 16.0f);
  NTempest::C3Vector ambColor(
      s_dnInfo.light.CloudArray[2].r * 0.0039215689f, s_dnInfo.light.CloudArray[2].g * 0.0039215689f, s_dnInfo.light.CloudArray[2].b * 0.0039215689f
  );
  NTempest::C3Vector sunColor(
      s_dnInfo.light.CloudArray[1].r * 0.0039215689f, s_dnInfo.light.CloudArray[1].g * 0.0039215689f, s_dnInfo.light.CloudArray[1].b * 0.0039215689f
  );
  NTempest::C3Vector emsColor(
      s_dnInfo.light.CloudArray[3].r * 0.0039215689f, s_dnInfo.light.CloudArray[3].g * 0.0039215689f, s_dnInfo.light.CloudArray[3].b * 0.0039215689f
  );
  float sunScaler = InterpTable(m_bumpFadeTable, 8, s_dnInfo.dayProgression);

  for (DWORD y = 0; y < m_updateSize; ++y) {
    for (DWORD x = 0; x < m_tmSize; ++x) {
      if (*clheight) {
        NTempest::C3Vector texelLightPos(sunLightPos.x - (float)x, sunLightPos.y - (float)(y + m_updateRow), 16.0f);
        NTempest::C3Vector color(
            ambColor.x * ((BYTE)(((255 - *clheight) >> 1) + 64) * 0.0039215689f) + emsColor.x,
            ambColor.y * ((BYTE)(((255 - *clheight) >> 1) + 64) * 0.0039215689f) + emsColor.y,
            ambColor.z * ((BYTE)(((255 - *clheight) >> 1) + 64) * 0.0039215689f) + emsColor.z
        );
        rayDir.Set(clbump->x, clbump->y, 1.0f);
        float dot = (texelLightPos.x * rayDir.x + texelLightPos.y * rayDir.y + 16.0f) *
                    NTempest::CMath::frsqrte_(rayDir.SquaredMag() * texelLightPos.SquaredMag(), 0x5F3997BB);
        if (dot > 0.0f) {
          dot *= sunScaler;
          color.x += dot * sunColor.x;
          color.y += dot * sunColor.y;
          color.z += dot * sunColor.z;
        }
        if (color.x > 1.0) {
          color.x = 1.0f;
        }
        if (color.y > 1.0) {
          color.y = 1.0f;
        }
        if (color.z > 1.0) {
          color.z = 1.0f;
        }
        cltexels->r = NTempest::CMath::ftol_0_256_(color.x * 255.0f);
        cltexels->g = NTempest::CMath::ftol_0_256_(color.y * 255.0f);
        cltexels->b = NTempest::CMath::ftol_0_256_(color.z * 255.0f);
        cltexels->a = *clheight;
      } else if (x >= 1) {
        *cltexels = *(cltexels - 1);
        cltexels->a = 0;
      }

      ++clbump;
      ++clheight;
      ++cltexels;
    }
  }
}

void DNClouds::FullUpdate() {
  m_updateRow = 0;
  m_waitTime = 0.0f;
  DWORD updateSize = m_updateSize;
  m_updateSize = m_tmSize;
  Update();
  m_updateSize = updateSize;
}

void DNClouds::Update() {
  static const float CLOUD_UPDATE_TIME = 0.1f;
  static WORD        deltaTable[5][5] = {
      {16, 32, 64, 128, 256},
      { 8, 16, 32,  64, 128},
      { 4,  8, 16,  32,  64},
      { 2,  4,  8,  16,  32},
      { 1,  2,  4,   8,   4}
  };

  if (!m_nLayers) {
    return;
  }

  m_waitTime -= s_dnInfo.elapsedSec;
  if (m_waitTime > 0.0f) {
    return;
  }

  m_waitTime = CLOUD_UPDATE_TIME;
  SetDensity(s_dnInfo.light.CloudData[1]);

  Octave octaves[5];
  UINT   permz0 = perm[m_timeX >> 8];
  UINT   permz1 = perm[((m_timeX >> 8) + 1) & 0xFF];
  UINT   oct;
  for (oct = 0; oct < m_nOctaves; ++oct) {
    octaves[oct].delta.x = octaves[oct].delta.y = deltaTable[m_lod][oct];
    octaves[oct].min.x = m_timeX >> 8;
    octaves[oct].min.z = m_timeX >> 8;
    octaves[oct].value.y = m_updateRow * octaves[oct].delta.y;
    octaves[oct].max.x = octaves[oct].min.x + ((octaves[oct].delta.x << m_tmShift) >> 8) + 2;
    octaves[oct].max.z = octaves[oct].min.z + 1;
    octaves[oct].amplitude = 1.0f / (1 << oct);
    octaves[oct].value.z = m_timeX;
  }

  memset(&m_noise[m_updateRow * m_tmSize], 0, m_updateSize * m_tmSize * sizeof(float));

  UINT y;
  for (y = 0; y < m_updateSize; ++y) {
    float              *cn = &m_noise[(m_updateRow + y) << m_tmShift];
    NTempest::C2Vector *clbump = &m_bump[(m_updateRow + y) << m_tmShift];

    for (oct = 0; oct < m_nOctaves; ++oct) {
      octaves[oct].min.y = octaves[oct].value.y >> 8;
      octaves[oct].max.y = (octaves[oct].min.y + 1) & 0xFF;
      octaves[oct].permy00 = perm[(octaves[oct].min.y + permz0) & 0xFF];
      octaves[oct].permy10 = perm[(octaves[oct].max.y + permz0) & 0xFF];
      octaves[oct].permy01 = perm[(octaves[oct].min.y + permz1) & 0xFF];
      octaves[oct].permy11 = perm[(octaves[oct].max.y + permz1) & 0xFF];
      octaves[oct].value.x = m_timeX;
      octaves[oct].lastiv = 0xFFFFFFFF;
    }

    float lastBumpNoiseX = 0.0f;
    for (UINT x = 0; x < m_tmSize; ++x) {
      Octave *o = octaves;
      for (oct = 0; oct < m_nOctaves; ++oct, ++o) {
        NTempest::C3iVector fv(o->value.x & 0xFF, o->value.y & 0xFF, o->value.z & 0xFF);

        o->iv = o->value.x >> 8;
        if (o->iv != o->lastiv) {
          o->lastiv = o->iv;
          o->x000 = valueTable[perm[(o->permy00 + o->iv) & 0xFF]];
          o->x100 = valueTable[perm[(o->permy00 + o->iv + 1) & 0xFF]] - o->x000;
          o->x010 = valueTable[perm[(o->permy10 + o->iv) & 0xFF]];
          o->x110 = valueTable[perm[(o->permy10 + o->iv + 1) & 0xFF]] - o->x010;
          o->x001 = valueTable[perm[(o->permy01 + o->iv) & 0xFF]];
          o->x101 = valueTable[perm[(o->permy01 + o->iv + 1) & 0xFF]] - o->x001;
          o->x011 = valueTable[perm[(o->permy11 + o->iv) & 0xFF]];
          o->x111 = valueTable[perm[(o->permy11 + o->iv + 1) & 0xFF]] - o->x011;
        }

        float fx = coserpTable[fv.x];
        float x0 = fx * o->x100 + o->x000;
        float x2 = fx * o->x101 + o->x001;
        float x1 = fx * o->x110 + o->x010;
        float y0 = (x1 - x0) * coserpTable[fv.y] + x0;
        float x3 = fx * o->x111 + o->x011;
        float y1 = (x3 - x2) * coserpTable[fv.y] + x2;
        cn[x] += ((y1 - y0) * coserpTable[fv.z] + y0) * o->amplitude;
        o->value.x += o->delta.x;

        if (oct == 2) {
          clbump[x].x = (1 << (m_tmShift - 7)) * (lastBumpNoiseX - cn[x]);
          clbump[x].y = (1 << (m_tmShift - 7)) * (m_lastBumpNoiseY[x] - cn[x]);
          lastBumpNoiseX = cn[x];
          m_lastBumpNoiseY[x] = cn[x];
        }
      }
    }

    for (oct = 0; oct < m_nOctaves; ++oct) {
      octaves[oct].value.y += octaves[oct].delta.y;
    }
  }

  BYTE  *clheight = &m_height[m_updateRow * m_tmSize];
  float *cn = &m_noise[m_updateRow * m_tmSize];
  for (y = 0; y < m_updateSize; ++y) {
    for (UINT x = 0; x < m_tmSize; ++x) {
      int  height = NTempest::CMath::ftol_0_256_(cn[x] * 64.0f + 128.0f) - m_density;
      BYTE cloud = 0;
      if (height >= 0) {
        cloud = cloudTable[height];
      }
      clheight[x] = cloud;
    }
    clheight += m_tmSize;
    cn += m_tmSize;
  }

  BumpMap();
  GxTexUpdate(m_texid, 0, m_updateRow, m_tmSize, m_updateRow + m_updateSize - 1, 1);
  m_updateRow += m_updateSize;
  if (m_updateRow >= m_tmSize) {
    ++m_timeX;
    m_updateRow = 0;
  }

  m_fogInfo.color = s_dnInfo.fogInfo.color;
  m_fogInfo.end = 8.333333f;
  m_fogInfo.start = 4.1666665f;
}

void DNClouds::GenSphere(float size) {
  float start = PI * 0.0f;
  float phiDelta = (PI * 0.22222222f - start) * 0.25f;
  float thetaStart = PI * 0.0f;
  float thetaDelta = (PI * 2.0f - thetaStart) * 0.0625f;
  float zoffs = -(size * 0.92f);
  float phi = start;
  float prevPhi = start;
  int   prevRowIdx = 0;
  m_geoVerts.SetCount(64);
  m_texVerts.SetCount(64);
  m_indices.SetCount(102);

  NTempest::C3Vector *newGeoVert = &m_geoVerts[0];
  NTempest::C2Vector *newTexVert = &m_texVerts[0];
  WORD               *newIndex = &m_indices[0];
  for (int phiStep = 0; phiStep < 4; ++phiStep) {
    float cosPhi = NTempest::CMath::cos_(phi);
    float sinPhi = NTempest::CMath::sin_(phi);
    float theta = thetaStart;
    float texRadius = 0.5f * (phiStep * 0.33333334f);
    int   thisRowIdx = newGeoVert - &m_geoVerts[0];

    for (int thetaStep = 0; thetaStep < 16; ++thetaStep) {
      float cs = NTempest::CMath::cos_(theta);
      float sn = NTempest::CMath::sin_(theta);
      newGeoVert->x = sinPhi * sn * size;
      newGeoVert->y = cs * sinPhi * size;
      newGeoVert->z = cosPhi * size + zoffs;
      newTexVert->x = sn * texRadius + 0.5f;
      newTexVert->y = cs * texRadius + 0.5f;
      ++newGeoVert;
      ++newTexVert;

      if (NTempest::CMath::fequal4_(phi, 0.0f) || NTempest::CMath::fequal4_(phi, 6.2831855f)) {
        break;
      }

      theta += thetaDelta;
    }

    if (phiStep > 0) {
      for (int thetaStep = 0; thetaStep < 17; ++thetaStep) {
        int prev = NTempest::CMath::fequal4_(prevPhi, 0.0f) ? 0 : thetaStep % 16;
        int current = NTempest::CMath::fequal4_(phi, 6.2831855f) ? 0 : thetaStep % 16;
        *newIndex++ = prevRowIdx + prev;
        *newIndex++ = thisRowIdx + current;
      }
    }

    prevPhi = phi;
    prevRowIdx = thisRowIdx;
    phi += phiDelta;
  }

  m_nVerts = (WORD)(newGeoVert - &m_geoVerts[0]);
  m_nIndices = 102;
}

void DNClouds::Callback_GxTex(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &gxTexels) {
  if (cmd == GxTex_Latch && mipLevel == 0) {
    texelStrideInBytes = w * sizeof(NTempest::CImVector);
    gxTexels = userArg;
  }
}

void DNClouds::Destroy() {
  GxTexDestroy(m_texid);
  m_texid = 0;
}

void DNClouds::OverrideDensitySharpness(float newDensity, float newSharpness) {
  m_densityOverride = newDensity;
  SetDensity(newDensity);
}

void DNClouds::SetSharpness(float newSharpness) {
  m_sharpness = newSharpness;
  float clval = 0.0f;
  float cldelta = (255 - m_density) * 0.00390625f;
  for (int i = 0; i < 256; ++i) {
    cloudTable[i] = (BYTE)(255.0f - NTempest::CMath::pow_(m_sharpness, clval) * 255.0f);
    clval += cldelta;
  }
}

void DNClouds::SetDensity(float newDensity) {
  if (m_densityOverride != 0.0f) {
    newDensity = m_densityOverride;
  }
  m_density = (BYTE)((1.0f - newDensity) * 255.0f);
}

void DNClouds::SetLOD(DWORD newlod, DWORD newUpdateSize) {
  if (m_texid) {
    GxTexDestroy(m_texid);
    m_texid = 0;
    m_height.Clear();
    m_texels.Clear();
    m_noise.Clear();
    m_lastBumpNoiseY.Clear();
    m_bump.Clear();
  }

  m_lod = newlod;
  m_tmSize = m_tmSizeTable[newlod];
  if (!newUpdateSize) {
    m_updateSize = 32;
  } else {
    m_updateSize = newUpdateSize;
  }
  m_wrapMask = m_tmSize - 1;
  m_tmShift = m_tmShiftTable[newlod];
  m_texels.SetCount(m_tmSize * m_tmSize);
  m_height.SetCount(m_tmSize * m_tmSize);
  m_noise.SetCount(m_tmSize * m_tmSize);
  m_lastBumpNoiseY.SetCount(m_tmSize);
  m_bump.SetCount(m_tmSize * m_tmSize);

  GxTexCreate(m_tmSize, m_tmSize, GxTex_Argb8888, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &m_texels[0], Callback_GxTex, m_texid);
  m_updateRow = 0;
  m_lastTime = 0;
}

DNClouds::DNClouds() {
  ValueTableInit();
  m_waitTime = 0.0f;
  m_nOctaves = 4;
}

void DNSky::GenSphere(float sphRadius) {
  m_sphThetaTess = 16;
  m_geoVerts.SetCount(112);
  m_clrVerts.SetCount(112);
  m_indices.SetCount(204);

  NTempest::C3Vector *newVert = &m_geoVerts[0];
  WORD               *newIndex = &m_indices[0];
  float               prevPhi = 0.0f;
  int                 prevRowIdx = 0;
  for (int phiStep = 0; phiStep < SKY_NUMBANDS; ++phiStep) {
    float phi = PI * m_stripSizes[phiStep];
    float cosPhi = NTempest::CMath::cos_(phi);
    float sinPhi = NTempest::CMath::sin_(phi);
    int   thisRowIdx = newVert - &m_geoVerts[0];

    for (int thetaStep = 0; thetaStep < 16; ++thetaStep) {
      float theta = 2.0f * PI * (thetaStep * 0.0625f);
      newVert->x = NTempest::CMath::sin_(theta) * sinPhi * sphRadius;
      newVert->y = NTempest::CMath::cos_(theta) * sinPhi * sphRadius;
      newVert->z = cosPhi * sphRadius;
      ++newVert;
      if (NTempest::CMath::fequal4_(phi, 0.0f) || NTempest::CMath::fequal4_(phi, 3.1415927f)) {
        break;
      }
    }

    if (phiStep > 0) {
      for (int thetaStep = 0; thetaStep < 17; ++thetaStep) {
        int prev = NTempest::CMath::fequal4_(prevPhi, 0.0f) ? 0 : thetaStep % 16;
        int current = NTempest::CMath::fequal4_(phi, 3.1415927f) ? 0 : thetaStep % 16;
        *newIndex++ = prevRowIdx + prev;
        *newIndex++ = thisRowIdx + current;
      }
    }

    prevPhi = phi;
    prevRowIdx = thisRowIdx;
  }

  m_nVerts = (WORD)(newVert - &m_geoVerts[0]);
  m_nIndices = 204;
}

void DNSky::GenTexture(UINT w, UINT h, NTempest::CImVector *texels) {
  NTempest::CImVector *texptr = texels;
  UINT                 halfh = h >> 1;

  {
    for (UINT i = 0; i < 5; ++i) {
      UINT  startY = m_stripSizes[i] * 2.0f * halfh;
      UINT  endY = m_stripSizes[i + 1] * 2.0f * halfh;
      float blend = 0.0f;
      float delta = 1.0f / (endY - startY + 1);
      UINT  next = i + 1;

      if (i == 4) {
        next = 4;
      }

      for (UINT y = startY; y < endY; ++y) {
        NTempest::CImVector clr;
        clr.Set(
            255, (BYTE)Fast_ftol(Interp(s_dnInfo.light.SkyArray[i].r, s_dnInfo.light.SkyArray[next].r, blend)),
            (BYTE)Fast_ftol(Interp(s_dnInfo.light.SkyArray[i].g, s_dnInfo.light.SkyArray[next].g, blend)),
            (BYTE)Fast_ftol(Interp(s_dnInfo.light.SkyArray[i].b, s_dnInfo.light.SkyArray[next].b, blend))
        );

        for (UINT x = 0; x < w; ++x) {
          *texptr++ = clr;
        }

        blend += delta;
      }
    }
  }

  NTempest::CImVector *src = texptr - w;
  {
    for (UINT mirrorY = 0; mirrorY < halfh; ++mirrorY) {
      for (UINT mirrorX = 0; mirrorX < w; ++mirrorX) {
        texptr[mirrorX] = src[mirrorX];
      }

      texptr += w;
      src -= w;
    }
  }
}

void DNSky::SetColors() {
  NTempest::CImVector *color = &m_clrVerts[0];
  NTempest::CImVector  midColors[6];
  float                darkness = InterpTable(m_darkTable, 6, s_dnInfo.dayProgression) * s_dnInfo.light.Darkness;

  for (int i = 0; i < 5; ++i) {
    midColors[i + 1] = NTempest::CImVector(
        255, Fast_ftol(Interp(s_dnInfo.light.SkyArray[i + 1].r, s_dnInfo.light.SkyArray[1].r, darkness)),
        Fast_ftol(Interp(s_dnInfo.light.SkyArray[i + 1].g, s_dnInfo.light.SkyArray[1].g, darkness)),
        Fast_ftol(Interp(s_dnInfo.light.SkyArray[i + 1].b, s_dnInfo.light.SkyArray[1].b, darkness))
    );
  }

  NTempest::CImVector topColor = s_dnInfo.light.SkyArray[0];
  topColor = DarkenColor(topColor, 1.0f);
  *color++ = topColor;

  float angleDelta = -1.0f / m_sphThetaTess;
  for (int band = 1; band <= 4; ++band) {
    float angle = s_dnInfo.faceAngle * 0.15915494f + 0.25f;
    if (angle > 1.0f) {
      angle -= 1.0f;
    }

    for (int theta = 0; theta < m_sphThetaTess; ++theta, ++color) {
      if (angle < 0.0f) {
        angle += 1.0f;
      }

      float fade = InterpTable(m_fadeTable, 6, angle);
      if (fade >= 0.0f) {
        NTempest::CImVector bandColor(
            255, Fast_ftol(Interp(s_dnInfo.light.SkyArray[band].r, midColors[band].r, (1.0f - fade) * darkness)),
            Fast_ftol(Interp(s_dnInfo.light.SkyArray[band].g, midColors[band].g, (1.0f - fade) * darkness)),
            Fast_ftol(Interp(s_dnInfo.light.SkyArray[band].b, midColors[band].b, (1.0f - fade) * darkness))
        );
        if (s_dnInfo.eclipseAmount) {
          bandColor.Blend255RGB(s_dnInfo.eclipseAmount, s_dnInfo.eclipseColor);
        }
        *color = bandColor;
      } else {
        NTempest::CImVector darkClr = midColors[band];
        darkClr.Scale255(Fast_ftol(darkness * 255.0f));
        darkClr = NTempest::CImVector(
            255, Fast_ftol(Interp(midColors[band].r, s_dnInfo.light.SkyArray[0].r, darkness * 0.69999999f)),
            Fast_ftol(Interp(midColors[band].g, s_dnInfo.light.SkyArray[0].g, darkness * 0.69999999f)),
            Fast_ftol(Interp(midColors[band].b, s_dnInfo.light.SkyArray[0].b, darkness * 0.69999999f))
        );
        darkClr = NTempest::CImVector(
            255, Fast_ftol(Interp(midColors[band].r, darkClr.r, -fade * darkness)),
            Fast_ftol(Interp(midColors[band].g, darkClr.g, -fade * darkness)),
            Fast_ftol(Interp(midColors[band].b, darkClr.b, -fade * darkness))
        );
        if (s_dnInfo.eclipseAmount) {
          darkClr.Blend255RGB(s_dnInfo.eclipseAmount, s_dnInfo.eclipseColor);
        }
        *color = darkClr;
      }

      angle += angleDelta;
    }
  }

  topColor = s_dnInfo.light.SkyArray[5];
  if (s_dnInfo.eclipseAmount) {
    topColor.Blend255RGB(s_dnInfo.eclipseAmount, s_dnInfo.eclipseColor);
  }
  for (int theta = 0; theta < m_sphThetaTess; ++theta) {
    *color++ = topColor;
  }

  NTempest::CImVector botColor = s_dnInfo.light.SkyArray[5];
  if (s_dnInfo.eclipseAmount) {
    botColor.Blend255RGB(s_dnInfo.eclipseAmount, s_dnInfo.eclipseColor);
  }
  *color = botColor;
}

void DNPlanet::GenGeometry(
    NTempest::C3Vector  geov[],
    NTempest::C2Vector  texv[],
    NTempest::CImVector clrv[],
    WORD                idx[],
    DWORD              &vertCount,
    DWORD              &idxCount
) {
  static WORD               noFadeIdx[4] = {0, 2, 1, 3};
  static WORD               fadeIdx[8] = {0, 4, 1, 5, 5, 3, 4, 2};
  static NTempest::C3Vector billbGeov[6] = {NTempest::C3Vector(0.0f, -0.5f, 0.5f),  NTempest::C3Vector(0.0f, 0.5f, 0.5f),
                                            NTempest::C3Vector(0.0f, -0.5f, -0.5f), NTempest::C3Vector(0.0f, 0.5f, -0.5f),
                                            NTempest::C3Vector(0.0f, -0.5f, 99.0f), NTempest::C3Vector(0.0f, 0.5f, 99.0f)};
  static NTempest::C2Vector billbTexv[6] = {NTempest::C2Vector(0.0f, 0.0f), NTempest::C2Vector(1.0f, 0.0f),  NTempest::C2Vector(0.0f, 1.0f),
                                            NTempest::C2Vector(1.0f, 1.0f), NTempest::C2Vector(0.0f, 99.0f), NTempest::C2Vector(1.0f, 99.0f)};

  float localZ = m_pos.z - s_dnInfo.cameraPos.z;

  for (UINT i = 0; i < 6; ++i) {
    geov[i] = billbGeov[i] * m_scale;
    texv[i] = billbTexv[i];
    clrv[i] = s_dnInfo.light.CloudArray[0];
  }

  memcpy(idx, noFadeIdx, sizeof(noFadeIdx));
  idxCount = 4;
  vertCount = 0;

  float clip1 = localZ + geov[0].z;
  float clip2 = localZ + geov[2].z;
  if (clip1 > 0.0f && clip2 > 0.0f) {
    vertCount = 4;
  } else if (clip1 < 0.0f && clip2 < 0.0f) {
    return;
  } else {
    vertCount = 4;
    float clipt = clip1 / (clip1 - clip2);
    geov[3].z = geov[2].z = (geov[2].z - geov[0].z) * clipt + geov[0].z;
    texv[3].y = texv[2].y = clipt;
  }

  const float fadeBegin = s_dnInfo.billbRescale * 0.4f;
  float       fade1 = localZ + geov[0].z - fadeBegin;
  float       fade2 = localZ + geov[2].z - fadeBegin;
  if (fade1 > 0.001 && fade2 < 0.001) {
    vertCount = 6;
    idxCount = 8;
    memcpy(idx, fadeIdx, sizeof(fadeIdx));
    float clipt = fade1 / (fade1 - fade2);
    geov[5].z = geov[4].z = (geov[2].z - geov[0].z) * clipt + geov[0].z;
    texv[5].y = texv[4].y = (texv[2].y - texv[0].y) * clipt + texv[0].y;
  }

  for (UINT j = 0; j < vertCount; ++j) {
    float fade = localZ + geov[j].z - fadeBegin;
    if (fade < 0.001) {
      clrv[j].a = (BYTE)((fadeBegin - -fade) / fadeBegin * 255.0f);
    }
  }
}

void DNStars::Update() {
  m_pos = s_dnInfo.cameraPos;
  m_color.a = InterpTable(m_fadeTable, 4, s_dnInfo.dayProgression) * 254.0f + 1.0f;
}

static BOOL ConsoleCommand_SkyCloudDensity(LPCSTR, LPCSTR args) {
  char  msg[256];
  float density;

  if (sscanf(args, "%f", &density) == 1) {
    if (density < 0.0) {
      density = 0.0f;
    } else if (density > 1.0f) {
      density = 1.0f;
    }

    s_clouds.OverrideDensitySharpness(density, 0.0f);
    if (density != 0.0f) {
      sprintf(msg, "SkyCloudDensity set from override to %f", density);
    } else {
      sprintf(msg, "SkyCloudDensity set from data");
    }
    ConsoleWrite(msg, DEFAULT_COLOR);
  } else {
    ConsoleWrite("SkyCloudDensity set from data", DEFAULT_COLOR);
  }

  return 0;
}

static bool CloudLODCallback(CVar *h, LPCSTR oldValue, LPCSTR newValue, LPVOID arg) {
  char message[64];
  int  lod = SStrToInt(newValue);

  if (lod < 0) {
    lod = 0;
  } else if (lod > 1) {
    lod = 1;
  }

  s_clouds.SetLOD(lod, 0);
  if (oldValue) {
    SStrPrintf(message, sizeof(message), "SkyCloudLOD set to %i", lod);
    ConsoleWrite(message, DEFAULT_COLOR);
  }
  return true;
}

static BOOL ConsoleCommand_SkyCloudLayers(LPCSTR, LPCSTR args) {
  BOOL result = 0;
  int  layers = -1;
  sscanf(args, "%d", &layers);
  if (layers >= 0 && layers <= 1) {
    s_clouds.SetLayers(layers);
    ConsoleWrite("CloudLayers set", DEFAULT_COLOR);
    result = 1;
  } else {
    ConsoleWrite("CloudLayers must be in the range [0, 1]", DEFAULT_COLOR);
  }

  return result;
}

static BOOL ConsoleCommand_SkySunGlare(LPCSTR, LPCSTR args) {
  int glareOn;
  if (sscanf(args, "%d", &glareOn) && glareOn) {
    ConsoleWrite("SunGlare enabled.  Don't look directly at it.", DEFAULT_COLOR);
    s_sunGlare.m_enabled = 1;
    s_moonGlare.m_enabled = 1;
  } else {
    ConsoleWrite("SunGlare disabled", DEFAULT_COLOR);
    s_sunGlare.m_enabled = 0;
    s_moonGlare.m_enabled = 0;
  }
  return 1;
}

static BOOL ConsoleCommand_SkyShow(LPCSTR, LPCSTR args) {
  int skyOn;
  if (sscanf(args, "%d", &skyOn) && skyOn) {
    ConsoleWrite("Sky enabled", DEFAULT_COLOR);
    s_dnInfo.showSky = 1;
  } else {
    ConsoleWrite("The sky is falling.", DEFAULT_COLOR);
    s_dnInfo.showSky = 0;
  }
  return 1;
}

void DayNightInitialize(LPCSTR litFile) {
  LoadLightsAndFog(litFile, &g_areaLights);
  ResetLightPos();

  ConsoleCommandRegister("SkyCloudLayers", ConsoleCommand_SkyCloudLayers, GRAPHICS, "0, 1");
  ConsoleCommandRegister("SkyCloudDensity", ConsoleCommand_SkyCloudDensity, GRAPHICS, "[0, 1]");
  s_cloudLODCvar = CVar::Register("SkyCloudLOD", 0, 0, "0", CloudLODCallback, GRAPHICS, false, 0);
  ConsoleCommandRegister("SkySunGlare", ConsoleCommand_SkySunGlare, GRAPHICS, "0, 1");
  ConsoleCommandRegister("SkyShow", ConsoleCommand_SkyShow, GRAPHICS, "0, 1");

  s_sky.GenSphere(1.0f);
  s_clouds.GenSphere(22.222221f);
  s_clouds.SetDensity(0.60000002f);
  s_clouds.SetSharpness(0.95999998f);
  s_clouds.SetLayers(1);
  s_dnInfo.cloudTex = s_clouds.m_texid;

  GlareBase::m_masterEnable = 1;
  s_sunGlare.Initialize("Textures\\sunGlare.blp");
  s_sunGlare.m_baseScale = 1.0f;
  s_sunGlare.m_fadeRate = 1.0f;
  s_sunGlare.m_enabled = 1;
  s_sunGlare.m_alphaMin = 0.5f;
  s_sunGlare.m_alphaMax = 1.0f;
  s_sunGlare.m_scaleMin = 0.5f;
  s_sunGlare.m_scaleMax = 6.0f;
  s_sunGlare.m_dotMin = 0.7f;
  s_sunGlare.m_fadeTable[0].Set(SUNRISE + 0.041666668f, 0.0f);
  s_sunGlare.m_fadeTable[1].Set(SUNRISE + 0.083333336f, 1.0f);
  s_sunGlare.m_fadeTable[2].Set(SUNSET - 0.083333336f, 1.0f);
  s_sunGlare.m_fadeTable[3].Set(SUNSET - 0.020833334f, 0.0f);

  s_moonGlare.Initialize("Textures\\moonGlare.blp");
  s_moonGlare.m_baseScale = 2.0f;
  s_moonGlare.m_fadeRate = 1.0f;
  s_moonGlare.m_enabled = 1;
  s_moonGlare.m_alphaMin = 0.1f;
  s_moonGlare.m_alphaMax = 1.0f;
  s_moonGlare.m_scaleMin = 1.0f;
  s_moonGlare.m_scaleMax = 1.0f;
  s_moonGlare.m_dotMin = 0.7f;
  s_moonGlare.m_fadeTable[0].Set(MOONSET - 0.083333336f, 1.0f);
  s_moonGlare.m_fadeTable[1].Set(MOONSET - 0.03125f, 0.0f);
  s_moonGlare.m_fadeTable[2].Set(MOONRISE + 0.03125f, 0.0f);
  s_moonGlare.m_fadeTable[3].Set(MOONRISE + 0.082638889f, 1.0f);

  s_planets[0].Initialize("Textures\\sunCenter.blp");
  s_planets[0].m_baseScale = 1.0f;
  s_planets[0].m_period = 1.0f;
  s_planets[1].Initialize("Textures\\moon.blp");
  s_planets[1].m_baseScale = 1.75f;
  s_planets[1].m_period = 1.0f;
  s_planets[2].Initialize("Textures\\moon02.blp");
  s_planets[2].m_baseScale = 1.0f;
  s_planets[2].m_period = 1.70000005f;
  s_stars.Initialize();

  s_dnInfo.showSky = 1;
  s_initialized = 1;

  s_magmaLight.AmbientColor = 0xFF6F2000;
  s_magmaLight.DirectColor = 0xFFFF5500;
  s_magmaLight.FogEnd = 27.0f;
  s_magmaLight.FogStartScalar = -2.0f;
  s_magmaLight.ShadowOpacity = 0xFF808080;
  s_magmaLight.Darkness = 0.0f;
  s_magmaLight.SkyArray[5] = 0xFFC83400;

  s_slimeLight.AmbientColor = 0xFF006000;
  s_slimeLight.DirectColor = 0xFF4E9314;
  s_slimeLight.FogEnd = 50.0f;
  s_slimeLight.FogStartScalar = -1.0f;
  s_slimeLight.ShadowOpacity = 0xFF808080;
  s_slimeLight.Darkness = 0.0f;
  s_slimeLight.SkyArray[5] = 0xFF00FF00;

  s_dnInfo.eclipseAmount = 0;
}

void DayNightDestroy() {
  if (s_initialized) {
    s_clouds.Destroy();

    s_planets[0].Destroy();
    s_planets[1].Destroy();
    s_planets[2].Destroy();
    s_sunGlare.Destroy();
    s_moonGlare.Destroy();
    s_stars.Destroy();
    s_initialized = 0;
  }
}

void DayNightForceFullUpdate() {
  SetColors();
  SetDirection();
  SetPlanets();
  s_clouds.FullUpdate();
}

void DayNightUpdateLighting() {
  double x = s_dnInfo.cameraDir.x;
  double y = s_dnInfo.cameraDir.y;
  if (y * y + x * x > 0.0001) {
    float angle = atan2(y, x);
    s_dnInfo.faceAngle = angle;
  } else {
    s_dnInfo.faceAngle = atan2(s_dnInfo.cameraDir.z, s_dnInfo.cameraDir.x);
  }

  if (s_dnInfo.faceAngle < 0.0f) {
    s_dnInfo.faceAngle += 6.2831855f;
  }

  if (!s_paused) {
    SetColors();
    SetDirection();
    SetPlanets();
    s_clouds.Update();
    s_stars.Update();
  }
}

void DayNightSetEclipse(NTempest::CImVector color, float amount) {
  FATALASSERT(amount >= 0.0f && amount <= 1.0f);

  s_dnInfo.eclipseColor = color;
  s_dnInfo.eclipseAmount = NTempest::CMath::ftol_0_256_(amount * 255.0f);
}

float DayNightSI(float offset) {
  return InterpTable(s_sidnTable, 4, s_dnInfo.dayProgression + offset);
}

float DayNightUnitSelectColor() {
  return InterpTable(s_unitColorTable, 2, s_dnInfo.dayProgression);
}

DNInfo *DayNightGetInfo() {
  return &s_dnInfo;
}

void DayNightRenderGlares() {
  if (s_dnInfo.showSky) {
    s_sunGlare.Render();
    s_moonGlare.Render();
  }
}

void DayNightRenderSky() {
  if (s_dnInfo.showSky) {
    if (CWorld::SceneCamLiquidStatus() != 15) {
      GxSceneSetClearColor(s_dnInfo.fogInfo.color);
      GxSceneClear(3);
    } else {
      GxSceneSetClearColor(NTempest::CImVector(0xFF000000));
      if (!GxMasterEnable(GxMasterEnable_PolygonFill)) {
        GxSceneClear(3);
      }

      s_sky.Render();
      s_stars.Render();
      s_planets[0].Render();
      s_planets[1].Render();
      s_planets[2].Render();
      s_clouds.Render();
    }
  } else {
    GxSceneSetClearColor(NTempest::CImVector(0xFF000000));
    GxSceneClear(3);
  }
}

void DayNightSkyTexCallback(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &texels) {
  if (cmd == GxTex_Latch && mipLevel == 0) {
    texelStrideInBytes = w * sizeof(NTempest::CImVector);
    s_sky.GenTexture(w, h, (NTempest::CImVector *)userArg);
    texels = userArg;
  }
}
