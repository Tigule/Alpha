#include "DayNight.h"
#include "WorldClient/CMapObj.h"
#include "WorldClient/World.h"
#include "Tempest/cpriorityq.h"
#include "Base/Status.h"
#include "Model/IModel.h"
#include "Services/Texture.h"
#include "Console/ConsoleCommand.h"
#include "Console/ConsoleClient.h"
#include "Console/ConsoleVar.h"

#include <stdio.h>

static DNInfo        s_dnInfo;
static int           s_initialized;
static int           s_paused;
static DNSky         s_sky;
static DNClouds      s_clouds;
static CurrentLight  s_magmaLight;
static CurrentLight  s_slimeLight;
static CVar         *s_cloudLODCvar;
LightGroup           g_areaLights;
static float         valueTable[256];
static unsigned char perm[256] = {
    225, 155, 210, 108, 175, 199, 221, 144, 203, 116, 70,  213, 69,  158, 33,  252, 5,   82,  173, 133, 222, 139, 174, 27,  9,   71,  90,  246, 75,
    130, 91,  191, 169, 138, 2,   151, 194, 235, 81,  7,   25,  113, 228, 159, 205, 253, 134, 142, 248, 65,  224, 217, 22,  121, 229, 63,  89,  103,
    96,  104, 156, 17,  201, 129, 36,  8,   165, 110, 237, 117, 231, 56,  132, 211, 152, 20,  181, 111, 239, 218, 170, 163, 51,  172, 157, 47,  80,
    212, 176, 250, 87,  49,  99,  242, 136, 189, 162, 115, 44,  43,  124, 94,  150, 16,  141, 247, 32,  10,  198, 223, 255, 72,  53,  131, 84,  57,
    220, 197, 58,  50,  208, 11,  241, 28,  3,   192, 62,  202, 18,  215, 153, 24,  76,  41,  15,  179, 39,  46,  55,  6,   128, 167, 23,  188, 106,
    34,  187, 140, 164, 73,  112, 182, 244, 195, 227, 13,  35,  77,  196, 185, 26,  200, 226, 119, 31,  123, 168, 125, 249, 68,  183, 230, 177, 135,
    160, 180, 12,  1,   243, 148, 102, 166, 38,  238, 251, 37,  240, 126, 64,  74,  161, 40,  184, 149, 171, 178, 101, 66,  29,  59,  146, 61,  254,
    107, 42,  86,  154, 4,   236, 232, 120, 21,  233, 209, 45,  98,  193, 114, 78,  19,  206, 14,  118, 127, 48,  79,  147, 85,  30,  207, 219, 54,
    88,  234, 190, 122, 95,  67,  143, 109, 137, 214, 145, 93,  92,  100, 245, 0,   216, 186, 60,  83,  105, 97,  204, 52
};
static unsigned char cloudTable[256];
static float         coserpTable[256];

unsigned long      DNClouds::m_tmSizeTable[] = {128, 256, 512, 1024, 2048};
unsigned long      DNClouds::m_tmShiftTable[] = {7, 8, 9, 10, 11};
const float        DNClouds::BUMPFADETIME = 0.027777778f;
const NTempest::C2Vector DNClouds::m_bumpFadeTable[] = {NTempest::C2Vector(0.16666667f, 1.0f), NTempest::C2Vector(0.19444445f, 0.0f),
                                                  NTempest::C2Vector(0.2013889f, 0.0f),  NTempest::C2Vector(0.22916667f, 1.0f),
                                                  NTempest::C2Vector(0.89583331f, 1.0f), NTempest::C2Vector(0.9236111f, 0.0f),
                                                  NTempest::C2Vector(0.8888889f, 0.0f),  NTempest::C2Vector(0.91666669f, 1.0f)};

static const float PI = 3.1415927f;

static NTempest::C2Vector s_sidnTable[4] = {
    NTempest::C2Vector(0.25f, 1.0f), NTempest::C2Vector(0.29166667f, 0.0f), NTempest::C2Vector(0.85416669f, 0.0f),
    NTempest::C2Vector(0.89583331f, 1.0f)
};
static NTempest::C2Vector s_unitColorTable[2] = {NTempest::C2Vector(0.083333336f, 0.25f), NTempest::C2Vector(0.5f, 1.0f)};

const NTempest::C2Vector DNStars::m_fadeTable[4] = {
    NTempest::C2Vector(0.22916667f - 0.10416666f, 1.0f), NTempest::C2Vector(0.22916667f - 0.041666668f, 0.0f),
    NTempest::C2Vector(0.89583331f + 0.041666668f, 0.0f), NTempest::C2Vector(0.89583331f + 0.10416666f, 1.0f)
};

DNStars     s_stars;
DNMoonGlare s_moonGlare;
DNSunGlare  s_sunGlare;
DNPlanet    s_planets[4];

int                GlareBase::m_masterEnable;
NTempest::C2Vector GlareBase::m_texv[4] = {
    NTempest::C2Vector(0.0f, 0.0f), NTempest::C2Vector(1.0f, 0.0f), NTempest::C2Vector(0.0f, 1.0f), NTempest::C2Vector(1.0f, 1.0f)
};
NTempest::C3Vector GlareBase::m_geov[4] = {
    NTempest::C3Vector(0.0f, -0.5f, 0.5f), NTempest::C3Vector(0.0f, 0.5f, 0.5f), NTempest::C3Vector(0.0f, -0.5f, -0.5f),
    NTempest::C3Vector(0.0f, 0.5f, -0.5f)
};
unsigned short GlareBase::m_idx[4] = {0, 2, 1, 3};

const float              DNSky::m_stripSizes[DNSky::SKY_NUMBANDS] = {0.0f, 0.35f, 0.41f, 0.46f, 0.49f, 0.5f, 1.0f};
const NTempest::C2Vector DNSky::m_fadeTable[] = {NTempest::C2Vector(0.125f, 1.0f), NTempest::C2Vector(0.375f, 0.0f),
                                                 NTempest::C2Vector(0.5f, -0.5f),  NTempest::C2Vector(0.625f, -0.69999999f),
                                                 NTempest::C2Vector(0.75f, -0.5f), NTempest::C2Vector(0.875f, 0.0f)};
const NTempest::C2Vector DNSky::m_darkTable[] = {NTempest::C2Vector(0.125f, 0.0f),      NTempest::C2Vector(0.27083334f, 1.0f),
                                                 NTempest::C2Vector(0.29166666f, 0.0f), NTempest::C2Vector(0.85416663f, 0.0f),
                                                 NTempest::C2Vector(0.89583331f, 1.0f), NTempest::C2Vector(0.99930555f, 0.0f),
                                                 NTempest::C2Vector(0.0f, 0.0f)};
const float              DNSky::m_fadeAngle[DNSky::SKY_NUMBANDS] = {0.0f, PI * 0.5f, PI * 0.5f, PI * 0.5f, 0.0000001f, 0.0f, 0.0f};
const float DNSky::m_darkAngle[DNSky::SKY_NUMBANDS] = {0.0f, PI - PI * 0.5f, PI - PI * 0.5f, PI - PI * 0.5f, PI - 0.0000001f, 0.0f, 0.0f};

static void ValueTableInit() {
  int lp;

  for (lp = 0; lp < 256; ++lp) {
    int value = rand();
    valueTable[lp] = 1.0f - (static_cast<float>(value) * 0.000030518509f + static_cast<float>(value) * 0.000030518509f);
  }

  for (lp = 0; lp < 256; ++lp) {
    coserpTable[lp] = (1.0f - cos(static_cast<float>(lp) * PI * 0.00390625f)) * 0.5f;
  }
}

static inline float Interp(float range1, float range2, float percent) {
  if (range2 < range1) {
    return range1 - (range1 - range2) * percent;
  }

  return (range2 - range1) * percent + range1;
}

class LightQE {
 public:
  LightQE(float pDist, int pSubscript) : dist(pDist), subscript(pSubscript) {
  }

  static bool HasHigherPriority(const LightQE &a, const LightQE &b) {
    return a.dist >= b.dist;
  }

  float dist;
  int   subscript;
};

static NTempest::CImVector BlendColor(NTempest::CImVector from, NTempest::CImVector to, float scale) {
  NTempest::CImVector color;
  color.Set(
      static_cast<unsigned char>(Interp(from.a, to.a, scale)), static_cast<unsigned char>(Interp(from.r, to.r, scale)),
      static_cast<unsigned char>(Interp(from.g, to.g, scale)), static_cast<unsigned char>(Interp(from.b, to.b, scale))
  );
  return color;
}

static float InterpTable(const NTempest::C2Vector *table, unsigned long size, float key) {
  unsigned long next;
  for (next = 0; next < size; ++next) {
    if (key <= table[next].x) {
      break;
    }
  }

  unsigned long previous;
  if (next == size) {
    next = 0;
    previous = size - 1;
  } else if (next) {
    previous = next - 1;
  } else {
    previous = size - 1;
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
  globalLight.DirectColor = BlendColor(globalLight.DirectColor, areaLight.DirectColor, scale);
  globalLight.AmbientColor = BlendColor(globalLight.AmbientColor, areaLight.AmbientColor, scale);
  for (unsigned int i = 0; i < 6; ++i) {
    globalLight.SkyArray[i] = BlendColor(globalLight.SkyArray[i], areaLight.SkyArray[i], scale);
  }
  for (i = 0; i < 5; ++i) {
    globalLight.CloudArray[i] = BlendColor(globalLight.CloudArray[i], areaLight.CloudArray[i], scale);
  }
  for (i = 0; i < 4; ++i) {
    globalLight.WaterArray[i] = BlendColor(globalLight.WaterArray[i], areaLight.WaterArray[i], scale);
  }
  globalLight.FogEnd = Interp(globalLight.FogEnd, areaLight.FogEnd, scale);
  globalLight.FogStartScalar = Interp(globalLight.FogStartScalar, areaLight.FogStartScalar, scale);
  globalLight.ShadowOpacity = BlendColor(globalLight.ShadowOpacity, areaLight.ShadowOpacity, scale);
  globalLight.Darkness = Interp(globalLight.Darkness, areaLight.Darkness, scale);
  for (i = 0; i < 4; ++i) {
    globalLight.CloudData[i] = Interp(globalLight.CloudData[i], areaLight.CloudData[i], scale);
  }
}

static void DoAreaLights(int underWater) {
  NTempest::CPriorityQ<LightQE, LightQE> lightq;

  for (unsigned int i = 1; i < g_areaLights.m_lightData.Count(); ++i) {
    LightData         &light = g_areaLights.m_lightData[i];
    NTempest::C3Vector delta = s_dnInfo.playerPos - light.m_lightlist.m_lightLocation;
    float              dist = delta.Mag();
    if (dist < light.m_lightlist.m_lightRadius) {
      lightq.Enqueue(LightQE(dist, i));
    }
  }

  while (lightq.HasEntries()) {
    LightQE        entry = lightq.Dequeue();
    LightData     &light = g_areaLights.m_lightData[entry.subscript];
    LightDataItem *lightdata;
    LightDataItem *stormdata;
    if (underWater) {
      lightdata = &light.m_lightdataWater;
      stormdata = &light.m_stormdataWater;
    } else {
      lightdata = &light.m_lightdata;
      stormdata = &light.m_stormdata;
    }

    CurrentLight areaLight;
    CalcLightColors(
        static_cast<int>(s_dnInfo.dayProgression * 2880.0f), &areaLight, lightdata, stormdata, static_cast<int>(s_dnInfo.stormPercentage * 100.0f)
    );

    float alpha = 1.0f;
    if (entry.dist > light.m_lightlist.m_lightDropoff) {
      alpha = 1.0f - (entry.dist - light.m_lightlist.m_lightDropoff) / (light.m_lightlist.m_lightRadius - light.m_lightlist.m_lightDropoff);
    }
    ScaleOutputs(s_dnInfo.light, areaLight, alpha);
  }
}

static void BlendRGB255(NTempest::CImVector &from, NTempest::CImVector to, unsigned int amount) {
  if (!amount) {
    return;
  }
  if (amount == 255) {
    from.r = to.r;
    from.g = to.g;
    from.b = to.b;
    return;
  }
  from.r = static_cast<unsigned char>(from.r + ((amount * (to.r - from.r)) >> 8));
  from.g = static_cast<unsigned char>(from.g + ((amount * (to.g - from.g)) >> 8));
  from.b = static_cast<unsigned char>(from.b + ((amount * (to.b - from.b)) >> 8));
}

static void ResetLightPos() {
  NTempest::C2Vector offset(0.0f, 0.0f);
  if (!CMap::bDungeon) {
    offset.x = 17066.666f;
    offset.y = 17066.666f;
  }

  unsigned int count;
  for (unsigned int lp = 0; lp < g_areaLights.m_lightData.Count(); ++lp) {
    LightData &light = g_areaLights.m_lightData[lp];
    if (lp) {
      NTempest::C3Vector pos = light.m_lightlist.m_lightLocation * 0.027777778f;
      light.m_lightlist.m_lightLocation.Set(-(pos.z - offset.x), -(pos.x - offset.y), pos.y);
      light.m_lightlist.m_lightRadius *= 0.027777778f;
      light.m_lightlist.m_lightDropoff *= 0.027777778f;
    }

    count = light.m_lightdata.m_fogData.Count();
    for (unsigned int i = 0; i < count; ++i) {
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
  NTempest::C3Vector rgb = clr;
  NTempest::C3Vector hsv;
  NTempest::RGBtoHSV(rgb, hsv);
  hsv.z *= amount;
  NTempest::HSVtoRGB(hsv, rgb);
  return NTempest::CImVector(
      clr.a, static_cast<unsigned char>(NTempest::CMath::fuint_n(rgb.x * 255.0f)),
      static_cast<unsigned char>(NTempest::CMath::fuint_n(rgb.y * 255.0f)), static_cast<unsigned char>(NTempest::CMath::fuint_n(rgb.z * 255.0f))
  );
}

static void SetLightColors() {
  s_dnInfo.lightInfo.dirColor = s_dnInfo.light.DirectColor;
  s_dnInfo.lightInfo.ambColor = s_dnInfo.light.AmbientColor;
  s_dnInfo.lightInfo.windowDirColor = BlendColor(s_dnInfo.light.DirectColor, s_dnInfo.light.AmbientColor, 0.5f);
  s_dnInfo.lightInfo.windowAmbColor = BlendColor(s_dnInfo.light.AmbientColor, s_dnInfo.light.DirectColor, 0.5f);
  s_dnInfo.lightInfo.windowAmbColor.r =
      static_cast<unsigned char>(s_dnInfo.lightInfo.windowAmbColor.r > 239 ? 255 : s_dnInfo.lightInfo.windowAmbColor.r + 16);
  s_dnInfo.lightInfo.windowAmbColor.g =
      static_cast<unsigned char>(s_dnInfo.lightInfo.windowAmbColor.g > 239 ? 255 : s_dnInfo.lightInfo.windowAmbColor.g + 16);
  s_dnInfo.lightInfo.windowAmbColor.b =
      static_cast<unsigned char>(s_dnInfo.lightInfo.windowAmbColor.b > 239 ? 255 : s_dnInfo.lightInfo.windowAmbColor.b + 16);

  s_dnInfo.shadowClr.Set(
      s_dnInfo.light.ShadowOpacity.r, static_cast<unsigned char>((s_dnInfo.light.AmbientColor.r + 3) / 3),
      static_cast<unsigned char>((s_dnInfo.light.AmbientColor.g + 3) / 3), static_cast<unsigned char>((s_dnInfo.light.AmbientColor.b + 3) / 3)
  );

  NTempest::C3Vector rgb = s_dnInfo.lightInfo.ambColor;
  NTempest::C3Vector hsv;
  NTempest::RGBtoHSV(rgb, hsv);
  hsv.x *= 1.0f;
  hsv.y *= 0.33000001f;
  hsv.z *= 1.25f;
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
  s_dnInfo.intFog = 0;
  s_dnInfo.intFogInfo.color = NTempest::CImVector(0ul);
  s_dnInfo.intFogInfo.start = 0.0f;
  s_dnInfo.intFogInfo.end = 0.0f;

  SMOFog::Fogs intFog;
  float        intPct;
  s_dnInfo.intFog = CWorld::QueryMapObjFog(0, intFog, intPct) == 1;
  if (!s_dnInfo.intFog) {
    return;
  }

  unsigned int liquid = CWorld::SceneCamLiquidStatus();
  if (liquid != 15) {
    liquid &= 3;
  }

  if (liquid == 2 || liquid == 3) {
    s_dnInfo.intFogInfo = s_dnInfo.fogInfo;
    return;
  }

  SMOFog::Fog &fog = intFog[liquid < 2];
  float        fogEnd = fog.end < s_dnInfo.farClip ? fog.end : s_dnInfo.farClip;
  s_dnInfo.intFogInfo.end = (fogEnd - s_dnInfo.fogInfo.end) * intPct + s_dnInfo.fogInfo.end;
  s_dnInfo.intFogInfo.start = ((fog.startScalar - s_dnInfo.light.FogStartScalar) * intPct + s_dnInfo.light.FogStartScalar) * s_dnInfo.intFogInfo.end;
  s_dnInfo.intFogInfo.color = s_dnInfo.fogInfo.color;
  BlendRGB255(s_dnInfo.intFogInfo.color, fog.color, NTempest::CMath::fuint_n(intPct * 255.0f));
}

void DNSky::SetColors() {
  NTempest::CImVector midColors[6];
  float               darkness = InterpTable(m_darkTable, 6, s_dnInfo.dayProgression) * s_dnInfo.light.Darkness;

  for (unsigned int i = 0; i < 5; ++i) {
    midColors[i + 1] = BlendColor(s_dnInfo.light.SkyArray[i + 1], s_dnInfo.light.SkyArray[0], darkness);
  }

  NTempest::CImVector *color = m_clrVerts.Ptr();
  NTempest::CImVector  topColor = DarkenColor(s_dnInfo.light.SkyArray[0], 1.0f);
  *color++ = topColor;

  for (unsigned int band = 1; band <= 4; ++band) {
    float angle = s_dnInfo.faceAngle * 0.15915494f + 0.25f;
    if (angle > 1.0f) {
      angle -= 1.0f;
    }
    float angleDelta = -1.0f / m_sphThetaTess;

    for (int theta = 0; theta < m_sphThetaTess; ++theta) {
      if (angle < 0.0f) {
        angle += 1.0f;
      }
      float               fade = InterpTable(m_fadeTable, 6, angle);
      NTempest::CImVector bandColor;
      if (fade >= 0.0f) {
        bandColor = BlendColor(midColors[band], s_dnInfo.light.SkyArray[band], (1.0f - fade) * darkness);
      } else {
        NTempest::CImVector darkClr = BlendColor(midColors[band], s_dnInfo.light.SkyArray[0], darkness * 0.69999999f);
        bandColor = BlendColor(midColors[band], darkClr, -fade * darkness);
      }
      BlendRGB255(bandColor, s_dnInfo.eclipseColor, s_dnInfo.eclipseAmount);
      *color++ = bandColor;
      angle += angleDelta;
    }
  }

  NTempest::CImVector botColor = s_dnInfo.light.SkyArray[5];
  BlendRGB255(botColor, s_dnInfo.eclipseColor, s_dnInfo.eclipseAmount);
  for (int theta = 0; theta < m_sphThetaTess; ++theta) {
    *color++ = botColor;
  }
  *color = s_dnInfo.light.SkyArray[5];
  BlendRGB255(*color, s_dnInfo.eclipseColor, s_dnInfo.eclipseAmount);
}

static void SetDirection() {
  static NTempest::C2Vector thetaTable[4] = {
      NTempest::C2Vector(0.0f, PI * 0.70555556f), NTempest::C2Vector(0.25f, PI * 0.6111111f), NTempest::C2Vector(0.5f, PI * 0.70555556f),
      NTempest::C2Vector(0.75f, PI * 0.6111111f)
  };
  static NTempest::C2Vector phiTable[4] = {
      NTempest::C2Vector(0.0f, PI * 1.25f), NTempest::C2Vector(0.25f, PI * 1.25f), NTempest::C2Vector(0.5f, PI * 1.25f),
      NTempest::C2Vector(0.75f, PI * 1.25f)
  };

  float phi = InterpTable(thetaTable, 4, s_dnInfo.dayProgression);
  float theta = InterpTable(phiTable, 4, s_dnInfo.dayProgression);
  float sinPhi = sin(phi);
  s_dnInfo.lightInfo.dir.x = cos(theta) * sinPhi;
  s_dnInfo.lightInfo.dir.y = sin(theta) * sinPhi;
  s_dnInfo.lightInfo.dir.z = cos(phi);
}

static void SetPlanets() {
  static NTempest::C2Vector moon2ThetaTable[5] = {
      NTempest::C2Vector(0.22916667f, PI * 0.55555558f), NTempest::C2Vector(0.49652778f, PI * 0.027777778f),
      NTempest::C2Vector(0.5f, PI * 0.027777778f), NTempest::C2Vector(0.50347222f, PI * 0.027777778f),
      NTempest::C2Vector(0.89583331f, PI * 0.55555558f)
  };
  static NTempest::C2Vector moon2PhiTable[3] = {
      NTempest::C2Vector(0.22916667f, PI * 0.25f), NTempest::C2Vector(0.5f, PI * 0.25f), NTempest::C2Vector(0.89583331f, PI * 0.25f)
  };
  static NTempest::C2Vector sunThetaTable[5] = {
      NTempest::C2Vector(0.0f, PI * 0.19444445f), NTempest::C2Vector(0.0034722222f, PI * 0.19444445f),
      NTempest::C2Vector(0.16666667f, PI * 0.55555558f), NTempest::C2Vector(0.91666669f, PI * 0.55555558f),
      NTempest::C2Vector(0.99993056f, PI * 0.19444445f)
  };
  static NTempest::C2Vector sunPhiTable[3] = {
      NTempest::C2Vector(0.0f, PI * 0.25f), NTempest::C2Vector(0.16666667f, PI * 0.25f), NTempest::C2Vector(0.91666669f, PI * 0.25f)
  };
  static NTempest::C2Vector moonThetaTable[5] = {
      NTempest::C2Vector(0.0f, PI * 0.19444445f), NTempest::C2Vector(0.0034722222f, PI * 0.19444445f),
      NTempest::C2Vector(0.16666667f, PI * 0.55555558f), NTempest::C2Vector(0.91666669f, PI * 0.55555558f),
      NTempest::C2Vector(0.99993056f, PI * 0.19444445f)
  };
  static NTempest::C2Vector moonPhiTable[3] = {
      NTempest::C2Vector(0.0f, PI * 0.75f), NTempest::C2Vector(0.16666667f, PI * 0.83333331f), NTempest::C2Vector(0.91666669f, PI * 0.91666669f)
  };
  static NTempest::C2Vector sunScaleTable[4] = {
      NTempest::C2Vector(0.25f, 2.0f), NTempest::C2Vector(0.28125f, 1.0f), NTempest::C2Vector(0.84375f, 1.0f), NTempest::C2Vector(0.875f, 2.0f)
  };
  static NTempest::C2Vector moonScaleTable[4] = {
      NTempest::C2Vector(0.041666668f, 1.0f), NTempest::C2Vector(0.16666667f, 1.5f), NTempest::C2Vector(0.91666669f, 1.5f),
      NTempest::C2Vector(0.99930555f, 1.0f)
  };

  s_dnInfo.billbRescale = (s_dnInfo.nearClipScaled + 10.0f) * 0.1f;

  float phi = InterpTable(moon2ThetaTable, 5, s_dnInfo.dayProgression);
  float theta = InterpTable(moon2PhiTable, 3, s_dnInfo.dayProgression);
  s_planets[0].m_pos.Set(cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi));
  s_planets[0].m_pos.Normalize();
  s_planets[0].m_pos *= 12.0f;
  s_planets[0].m_pos += s_dnInfo.cameraPos;
  s_planets[0].m_scale = InterpTable(sunScaleTable, 4, s_dnInfo.dayProgression) * s_planets[0].m_baseScale * s_dnInfo.billbRescale;
  s_sunGlare.m_pos = s_planets[0].m_pos;

  phi = InterpTable(sunThetaTable, 5, s_dnInfo.dayProgression);
  theta = InterpTable(sunPhiTable, 3, s_dnInfo.dayProgression);
  s_planets[1].m_pos.Set(cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi));
  s_planets[1].m_pos.Normalize();
  s_planets[1].m_pos *= 12.0f;
  s_planets[1].m_pos += s_dnInfo.cameraPos;
  s_planets[1].m_scale = InterpTable(moonScaleTable, 4, s_dnInfo.dayProgression) * s_planets[1].m_baseScale * s_dnInfo.billbRescale;
  s_moonGlare.m_pos = s_planets[1].m_pos;
  s_moonGlare.m_scaleMin = s_planets[1].m_scale * s_dnInfo.billbRescale;
  s_moonGlare.m_scaleMax = s_moonGlare.m_scaleMin;

  s_sunGlare.Update(s_dnInfo.elapsedSec);
  s_moonGlare.Update(s_dnInfo.elapsedSec);

  float moon2t =
      (s_dnInfo.day + s_dnInfo.dayProgression) - floor((s_dnInfo.day + s_dnInfo.dayProgression) / s_planets[2].m_period) * s_planets[2].m_period;
  moon2t /= s_planets[2].m_period;
  phi = InterpTable(moonThetaTable, 5, moon2t);
  theta = InterpTable(moonPhiTable, 3, moon2t);
  s_planets[2].m_pos.Set(cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi));
  s_planets[2].m_pos.Normalize();
  s_planets[2].m_pos *= 12.0f;
  s_planets[2].m_pos += s_dnInfo.cameraPos;
  s_planets[2].m_scale = InterpTable(moonScaleTable, 4, moon2t) * s_planets[2].m_baseScale * s_dnInfo.billbRescale;

  if (s_dnInfo.dayProgression >= 0.22916667f && s_dnInfo.dayProgression < 0.5f) {
    s_dnInfo.sunMoonPath = (s_dnInfo.dayProgression - 0.22916667f) / (0.5f - 0.22916667f);
  } else if (s_dnInfo.dayProgression >= 0.5f && s_dnInfo.dayProgression < 0.89583331f) {
    s_dnInfo.sunMoonPath = 1.0f - (s_dnInfo.dayProgression - 0.5f) / (0.89583331f - 0.5f);
  } else if (s_dnInfo.dayProgression >= 0.91666669f) {
    s_dnInfo.sunMoonPath = (s_dnInfo.dayProgression - 0.91666669f) / (1.0f - 0.91666669f);
  } else if (s_dnInfo.dayProgression < 0.16666667f) {
    s_dnInfo.sunMoonPath = 1.0f - s_dnInfo.dayProgression / 0.16666667f;
  } else {
    s_dnInfo.sunMoonPath = 0.0f;
  }
}

static void SetColors() {
  unsigned int camLiquid = CWorld::SceneCamLiquidStatus();
  int          underWater = (camLiquid & 0xF) == 0 || (camLiquid & 0xF) == 1;

  if (g_areaLights.m_lightData.Count()) {
    if (camLiquid == 15 || underWater) {
      LightData     &global = g_areaLights.m_lightData[0];
      LightDataItem *lightdata;
      LightDataItem *stormdata;
      if (underWater) {
        lightdata = &global.m_lightdataWater;
        stormdata = &global.m_stormdataWater;
      } else {
        lightdata = &global.m_lightdata;
        stormdata = &global.m_stormdata;
      }
      CalcLightColors(
          static_cast<int>(s_dnInfo.dayProgression * 2880.0f), &s_dnInfo.light, lightdata, stormdata,
          static_cast<int>(s_dnInfo.stormPercentage * 100.0f)
      );
      DoAreaLights(underWater);
    } else if ((camLiquid & 0xF) == 2) {
      s_dnInfo.light = s_magmaLight;
    } else if ((camLiquid & 0xF) == 3) {
      s_dnInfo.light = s_slimeLight;
    }
  } else {
    memset(&s_dnInfo.light, 0xFF, sizeof(s_dnInfo.light));
    s_dnInfo.light.FogEnd = 10000000000.0f;
    s_dnInfo.light.FogStartScalar = 0.5f;
  }

  SetLightColors();
  SetFogColors();
  if (underWater) {
    float darken = 1.0f;
    if (camLiquid == 1) {
      float z = s_dnInfo.playerPos.z < -30.0f ? -30.0f : s_dnInfo.playerPos.z;
      darken = 1.0f + z / 30.0f;
    }
    s_dnInfo.fogInfo.color = DarkenColor(s_dnInfo.fogInfo.color, (darken + 1.0f) * 0.5f);
    s_dnInfo.lightInfo.ambColor = DarkenColor(s_dnInfo.lightInfo.ambColor, (darken + 1.0f) * 0.5f);
    s_dnInfo.lightInfo.dirColor = DarkenColor(s_dnInfo.lightInfo.dirColor, darken * 0.25f + 0.75f);
  }

  s_sky.SetColors();
  s_planets[0].m_color = s_dnInfo.light.SkyArray[5];
  s_sunGlare.m_color = s_dnInfo.light.SkyArray[5];
  s_planets[1].m_color = s_dnInfo.light.SkyArray[5];
  s_moonGlare.m_color = s_dnInfo.light.SkyArray[5];
  if (s_dnInfo.eclipseAmount) {
    BlendRGB255(s_dnInfo.fogInfo.color, s_dnInfo.eclipseColor, s_dnInfo.eclipseAmount);
    BlendRGB255(s_dnInfo.lightInfo.ambColor, s_dnInfo.eclipseColor, s_dnInfo.eclipseAmount);
    BlendRGB255(s_dnInfo.lightInfo.dirColor, s_dnInfo.eclipseColor, s_dnInfo.eclipseAmount);
    BlendRGB255(s_planets[0].m_color, s_dnInfo.eclipseColor, s_dnInfo.eclipseAmount);
    BlendRGB255(s_planets[1].m_color, s_dnInfo.eclipseColor, s_dnInfo.eclipseAmount);
  }
  s_dnInfo.sidn = DayNightSI(0.0f);
  s_dnInfo.unitSelect = DayNightUnitSelectColor();
}

float DNSunGlare::GetCloudDensityFade() {
  return 1.0f - s_clouds.GetDensity(m_pos, 1.0f);
}

void DNClouds::WorldToTexture(const NTempest::C3Vector &worldPt, NTempest::C2Vector &tex) {
  NTempest::C3Vector localPt = worldPt - s_dnInfo.cameraPos;
  NTempest::C3Vector sphColpt(localPt.x, localPt.y, localPt.z + 736.0f);
  NTempest::C3Vector up(0.0f, 0.0f, 800.0f);
  float              texRadius;
  float              idenom;

  float       angle = static_cast<float>(acos(NTempest::C3Vector::Dot(up, sphColpt) / (up.Mag() * sphColpt.Mag())));
  const float maxAngle = PI * 0.22222222f;
  if (angle > maxAngle) {
    angle = maxAngle;
  }

  texRadius = angle * 0.5f / (maxAngle - maxAngle * 0.25f);
  idenom = NTempest::CMath::sqrt_(localPt.x * localPt.x + localPt.y * localPt.y);
  if (idenom <= 0.00001f) {
    localPt.x = 0.0f;
    localPt.y = 0.0f;
  } else {
    idenom = 1.0f / idenom;
    localPt.x *= idenom;
    localPt.y *= idenom;
  }

  tex.x = (localPt.x * texRadius + 0.5f) * m_tmSize;
  tex.y = (localPt.y * texRadius + 0.5f) * m_tmSize;
}

void DNClouds::Collide(const NTempest::C3Vector &origin, const NTempest::C3Vector &dir, NTempest::C3Vector &hitPoint) {
  float r1;
  float r2;

  FATALASSERT(origin.x == 0.0f && origin.y == 0.0f && origin.z == 0.0f);

  float b = dir.z * 1472.0f;
  float c = -98304.0f;
  float root = NTempest::CMath::sqrt_(b * b - 4.0f * dir.SquaredMag() * c);
  float q = -0.5f * (b <= 0.0f ? b - root : b + root);
  r1 = q / dir.SquaredMag();
  r2 = c / q;
  if (r2 < r1) {
    float swap = r1;
    r1 = r2;
    r2 = swap;
  }

  hitPoint = s_dnInfo.cameraPos + origin + dir * r1;
}

float DNClouds::GetDensity(const NTempest::C3Vector &worldPoint, float area) {
  if (!m_nLayers) {
    return 0.0f;
  }

  NTempest::C2Vector texv;
  WorldToTexture(worldPoint, texv);
  unsigned int index = static_cast<unsigned int>(texv.x) + (static_cast<unsigned int>(texv.y) << m_tmShift);
  return static_cast<float>(m_height[index]) * 0.0039215689f;
}

float DNMoonGlare::GetCloudDensityFade() {
  float density = s_clouds.GetDensity(m_pos, 1.0f);
  return 1.0f - fabs((density - 0.5f) + (density - 0.5f));
}

void DNClouds::BumpMap() {
  NTempest::C2Vector  *clbump = &m_bump[m_updateRow << m_tmShift];
  unsigned char       *clheight = &m_height[m_updateRow << m_tmShift];
  NTempest::CImVector *cltexels = &m_texels[m_updateRow << m_tmShift];
  NTempest::C3Vector   sunLightPos;
  NTempest::C3Vector   rayOrg(0.0f);
  NTempest::C3Vector   colPt(0.0f);
  NTempest::C2Vector   texPt(0.0f);

  if (s_dnInfo.dayProgression < 0.22916667f - BUMPFADETIME || s_dnInfo.dayProgression > 0.89583331f + BUMPFADETIME) {
    sunLightPos = s_planets[1].m_pos - s_dnInfo.cameraPos;
  } else {
    sunLightPos = s_planets[0].m_pos - s_dnInfo.cameraPos;
  }

  Collide(rayOrg, sunLightPos, colPt);
  WorldToTexture(colPt, texPt);
  s_dnInfo.sunPosTexPt.x = texPt.x / m_tmSize;
  s_dnInfo.sunPosTexPt.y = texPt.y / m_tmSize;

  NTempest::C3Vector ambColor = s_dnInfo.light.CloudArray[2];
  NTempest::C3Vector sunColor = s_dnInfo.light.CloudArray[1];
  NTempest::C3Vector emsColor = s_dnInfo.light.CloudArray[3];
  float              sunScaler = InterpTable(m_bumpFadeTable, 8, s_dnInfo.dayProgression);

  for (unsigned long y = 0; y < m_updateSize; ++y) {
    for (unsigned long x = 0; x < m_tmSize; ++x) {
      if (*clheight) {
        NTempest::C3Vector texelLightPos(texPt.x - static_cast<float>(x), texPt.y - static_cast<float>(y + m_updateRow), 16.0f);
        NTempest::C3Vector rayDir(clbump->x, clbump->y, 1.0f);
        float              dot = (texelLightPos.x * rayDir.x + texelLightPos.y * rayDir.y + 16.0f) /
                                 NTempest::CMath::sqrt_(rayDir.SquaredMag() * texelLightPos.SquaredMag());
        NTempest::C3Vector color(
            emsColor.x + ambColor.x * (static_cast<unsigned char>(((255 - *clheight) >> 1) + 64) * 0.0039215689f),
            emsColor.y + ambColor.y * (static_cast<unsigned char>(((255 - *clheight) >> 1) + 64) * 0.0039215689f),
            emsColor.z + ambColor.z * (static_cast<unsigned char>(((255 - *clheight) >> 1) + 64) * 0.0039215689f)
        );
        if (dot > 0.0f) {
          dot *= sunScaler;
          color.x += dot * sunColor.x;
          color.y += dot * sunColor.y;
          color.z += dot * sunColor.z;
        }
        if (color.x > 1.0f)
          color.x = 1.0f;
        if (color.y > 1.0f)
          color.y = 1.0f;
        if (color.z > 1.0f)
          color.z = 1.0f;
        cltexels->Set(
            *clheight, static_cast<unsigned char>(NTempest::CMath::fuint_n(color.x * 255.0f)),
            static_cast<unsigned char>(NTempest::CMath::fuint_n(color.y * 255.0f)),
            static_cast<unsigned char>(NTempest::CMath::fuint_n(color.z * 255.0f))
        );
      } else if (x) {
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
  unsigned long updateSize = m_updateSize;
  m_updateSize = m_tmSize;
  Update();
  m_updateSize = updateSize;
}

void DNGlare::Update(float elapsedSec) {
  m_targetOpacity = 1.0f;

  NTempest::C3Vector glareDir = m_pos - s_dnInfo.playerPos;
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
  m_curScale = ((m_scaleMax - m_scaleMin) * pct + m_scaleMin) * m_baseScale * s_dnInfo.sunMoonPath;
  m_color.a = static_cast<unsigned char>(((m_alphaMax - m_alphaMin) * pct + m_alphaMin) * m_opacity * 255.0f);
}

void DNSky::GenSphere(float sphRadius) {
  m_sphThetaTess = 16;
  m_geoVerts.SetCount(112);
  m_clrVerts.SetCount(112);
  m_indices.SetCount(204);

  NTempest::C3Vector *newVert = m_geoVerts.Ptr();
  unsigned short     *newIndex = m_indices.Ptr();
  float               prevPhi = 0.0f;
  int                 prevRowIdx = 0;
  for (int phiStep = 0; phiStep < SKY_NUMBANDS; ++phiStep) {
    float phi = PI * m_stripSizes[phiStep];
    float cosPhi = static_cast<float>(cos(phi));
    float sinPhi = static_cast<float>(sin(phi));
    int   thisRowIdx = static_cast<int>(newVert - m_geoVerts.Ptr());

    for (int thetaStep = 0; thetaStep < m_sphThetaTess; ++thetaStep) {
      float theta = thetaStep * (1.0f / m_sphThetaTess) * PI * 2.0f;
      newVert->x = static_cast<float>(sin(theta)) * sinPhi * sphRadius;
      newVert->y = static_cast<float>(cos(theta)) * sinPhi * sphRadius;
      newVert->z = cosPhi * sphRadius;
      ++newVert;
      if (NTempest::CMath::fabs_(phi) < 0.00000095367432f || NTempest::CMath::fabs_(phi - PI) < 0.00000095367432f) {
        break;
      }
    }

    if (phiStep > 0) {
      for (int thetaStep = 0; thetaStep <= m_sphThetaTess; ++thetaStep) {
        *newIndex++ = static_cast<unsigned short>(
            prevRowIdx + (NTempest::CMath::fabs_(prevPhi) < 0.00000095367432f ? 0 : static_cast<unsigned short>(thetaStep % m_sphThetaTess)));
        *newIndex++ = static_cast<unsigned short>(
            thisRowIdx + (NTempest::CMath::fabs_(phi - PI) < 0.00000095367432f ? 0 : static_cast<unsigned short>(thetaStep % m_sphThetaTess)));
      }
    }

    prevRowIdx = thisRowIdx;
    prevPhi = phi;
  }

  m_nVerts = static_cast<unsigned short>(newVert - m_geoVerts.Ptr());
  m_nIndices = 204;
  m_sphRadius = sphRadius;
}

void DNClouds::Callback_GxTex(
    EGxTexCommand cmd,
    unsigned int  w,
    unsigned int  h,
    unsigned int  d,
    unsigned int  mipLevel,
    void         *userArg,
    unsigned int &texelStrideInBytes,
    const void  *&gxTexels
) {
  if (cmd == GxTex_Latch && mipLevel == 0) {
    DNClouds *clouds = static_cast<DNClouds *>(userArg);
    texelStrideInBytes = w * sizeof(NTempest::CImVector);
    gxTexels = clouds->m_texels.Ptr();
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
  float cldelta = static_cast<float>(255 - static_cast<unsigned char>(m_density)) / 256.0f;
  m_sharpness = newSharpness;
  for (unsigned int i = 0; i < 256; ++i) {
    cloudTable[i] = static_cast<unsigned char>(255.0f - pow(m_sharpness, i * cldelta) * 255.0f);
  }
}

void DNClouds::SetDensity(float newDensity) {
  if (m_densityOverride != 0.0f) {
    newDensity = m_densityOverride;
  }
  m_density = static_cast<unsigned char>((1.0f - newDensity) * 255.0f);
}

void DNClouds::SetLOD(unsigned long newlod, unsigned long newUpdateSize) {
  if (m_texid) {
    GxTexDestroy(m_texid);
    m_texid = 0;
    m_texels.SetCount(0);
    m_height.SetCount(0);
    m_noise.SetCount(0);
    m_lastBumpNoiseY.SetCount(0);
    m_bump.SetCount(0);
  }

  m_lod = newlod;
  m_tmSize = m_tmSizeTable[newlod];
  m_updateSize = newUpdateSize ? newUpdateSize : 32;
  m_wrapMask = m_tmSize - 1;
  m_tmShift = m_tmShiftTable[newlod];
  unsigned long texelCount = m_tmSize * m_tmSize;
  m_texels.SetCount(texelCount);
  m_height.SetCount(texelCount);
  m_noise.SetCount(texelCount);
  m_lastBumpNoiseY.SetCount(m_tmSize);
  m_bump.SetCount(texelCount);

  GxTexCreate(m_tmSize, m_tmSize, GxTex_Argb8888, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), this, Callback_GxTex, m_texid);
  m_updateRow = 0;
  m_lastTime = 0;
}

DNClouds::DNClouds() {
  ValueTableInit();
  m_texid = 0;
  m_fogInfo.color = 0;
  m_waitTime = 0.0f;
  m_nOctaves = 4;
}

void DNClouds::Update() {
  static const unsigned short deltaTable[5][5] = {
      {16, 32, 64, 128, 256},
      { 8, 16, 32,  64, 128},
      { 4,  8, 16,  32,  64},
      { 2,  4,  8,  16,  32},
      { 1,  2,  4,   8,  16}
  };

  if (!m_nLayers) {
    return;
  }

  m_waitTime -= s_dnInfo.elapsedSec;
  if (m_waitTime > 0.0f) {
    return;
  }

  m_waitTime = 0.1f;
  SetDensity(s_dnInfo.light.CloudData[1]);

  Octave       octaves[5];
  unsigned int permz0 = perm[static_cast<unsigned char>(m_timeX >> 8)];
  unsigned int permz1 = perm[static_cast<unsigned char>((m_timeX >> 8) + 1)];
  unsigned int oct;
  for (oct = 0; oct < m_nOctaves; ++oct) {
    Octave        &o = octaves[oct];
    unsigned short delta = deltaTable[m_lod][oct];
    memset(&o, 0, sizeof(o));
    o.value.x = 0;
    o.value.y = static_cast<unsigned short>(delta * m_updateRow);
    o.value.z = m_timeX;
    o.delta.x = delta;
    o.delta.y = delta;
    o.delta.z = 1;
    o.amplitude = 1.0f / static_cast<float>(1 << oct);
    o.permy00 = permz0;
    o.permy10 = permz1;
  }

  float *cn = &m_noise[m_updateRow << m_tmShift];
  memset(cn, 0, sizeof(float) * m_tmSize * m_updateSize);
  NTempest::C2Vector *clbump = &m_bump[m_updateRow << m_tmShift];

  unsigned int y;
  for (y = 0; y < m_updateSize; ++y) {
    float lastBumpNoiseX = 0.0f;
    for (unsigned int x = 0; x < m_tmSize; ++x) {
      float x2 = 0.0f;
      for (oct = 0; oct < m_nOctaves; ++oct) {
        Octave             &o = octaves[oct];
        NTempest::C3iVector fv(o.value.x & 0xFF, o.value.y & 0xFF, o.value.z & 0xFF);
        unsigned int        ix0 = (o.value.x >> 8) & 0xFF;
        unsigned int        iy0 = (o.value.y >> 8) & 0xFF;
        unsigned int        iz0 = (o.value.z >> 8) & 0xFF;
        unsigned int        ix1 = (ix0 + 1) & 0xFF;
        unsigned int        iy1 = (iy0 + 1) & 0xFF;
        unsigned int        iz1 = (iz0 + 1) & 0xFF;

        o.permy00 = perm[(perm[ix0] + iy0) & 0xFF];
        o.permy10 = perm[(perm[ix1] + iy0) & 0xFF];
        o.permy01 = perm[(perm[ix0] + iy1) & 0xFF];
        o.permy11 = perm[(perm[ix1] + iy1) & 0xFF];
        o.x000 = valueTable[perm[(o.permy00 + iz0) & 0xFF]];
        o.x100 = valueTable[perm[(o.permy10 + iz0) & 0xFF]];
        o.x010 = valueTable[perm[(o.permy01 + iz0) & 0xFF]];
        o.x110 = valueTable[perm[(o.permy11 + iz0) & 0xFF]];
        o.x001 = valueTable[perm[(o.permy00 + iz1) & 0xFF]];
        o.x101 = valueTable[perm[(o.permy10 + iz1) & 0xFF]];
        o.x011 = valueTable[perm[(o.permy01 + iz1) & 0xFF]];
        o.x111 = valueTable[perm[(o.permy11 + iz1) & 0xFF]];

        float fx = coserpTable[fv.x];
        float fy = coserpTable[fv.y];
        float fz = coserpTable[fv.z];
        float x00 = o.x000 + (o.x100 - o.x000) * fx;
        float x10 = o.x010 + (o.x110 - o.x010) * fx;
        float x01 = o.x001 + (o.x101 - o.x001) * fx;
        float x11 = o.x011 + (o.x111 - o.x011) * fx;
        float y0 = x00 + (x10 - x00) * fy;
        float y1 = x01 + (x11 - x01) * fy;
        x2 += (y0 + (y1 - y0) * fz) * o.amplitude;

        o.value.x = static_cast<unsigned short>(o.value.x + o.delta.x);
        if (oct == 2) {
          float scale = static_cast<float>(1 << (m_tmShift - 7));
          clbump[x].x = scale * (lastBumpNoiseX - x2);
          clbump[x].y = scale * (m_lastBumpNoiseY[x] - x2);
          lastBumpNoiseX = x2;
          m_lastBumpNoiseY[x] = x2;
        }
      }
      cn[x] = x2;
    }

    for (oct = 0; oct < m_nOctaves; ++oct) {
      octaves[oct].value.x = 0;
      octaves[oct].value.y = static_cast<unsigned short>(octaves[oct].value.y + octaves[oct].delta.y);
    }
    cn += m_tmSize;
    clbump += m_tmSize;
  }

  unsigned char *clheight = &m_height[m_updateRow << m_tmShift];
  cn = &m_noise[m_updateRow << m_tmShift];
  for (y = 0; y < m_updateSize; ++y) {
    for (unsigned int x = 0; x < m_tmSize; ++x) {
      int height = static_cast<unsigned char>(NTempest::CMath::fuint_n(cn[x] * 64.0f + 128.0f)) - static_cast<unsigned char>(m_density);
      clheight[x] = height < 0 ? 0 : cloudTable[height];
    }
    cn += m_tmSize;
    clheight += m_tmSize;
  }

  BumpMap();
  GxTexUpdate(m_texid, 0, m_updateRow, m_tmSize, m_updateRow + m_updateSize - 1, 1);
  m_updateRow += m_updateSize;
  if (m_updateRow >= m_tmSize) {
    ++m_timeX;
    m_updateRow = 0;
  }

  m_fogInfo.color = s_dnInfo.fogInfo.color;
  m_fogInfo.start = 4.1666665f;
  m_fogInfo.end = 8.333333f;
}

void DNClouds::GenSphere(float size) {
  float phiDelta = (PI * 0.22222222f) * 0.25f;
  float prevPhi = 0.0f;
  int   prevRowIdx = 0;
  m_geoVerts.SetCount(64);
  m_texVerts.SetCount(64);
  m_indices.SetCount(102);

  NTempest::C3Vector *newGeoVert = m_geoVerts.Ptr();
  NTempest::C2Vector *newTexVert = m_texVerts.Ptr();
  unsigned short     *newIndex = m_indices.Ptr();
  for (int phiStep = 0; phiStep < 4; ++phiStep) {
    float phi = phiStep * phiDelta;
    float cosPhi = static_cast<float>(cos(phi));
    float sinPhi = static_cast<float>(sin(phi));
    float texRadius = phiStep * 0.33333334f * 0.5f;
    int   thisRowIdx = static_cast<int>(newGeoVert - m_geoVerts.Ptr());

    for (int thetaStep = 0; thetaStep < 16; ++thetaStep) {
      float theta = thetaStep * 0.0625f * PI * 2.0f;
      newGeoVert->x = static_cast<float>(sin(theta)) * sinPhi * size;
      newGeoVert->y = static_cast<float>(cos(theta)) * sinPhi * size;
      newGeoVert->z = cosPhi * size - size * 0.92f;
      newTexVert->x = static_cast<float>(sin(theta)) * texRadius + 0.5f;
      newTexVert->y = static_cast<float>(cos(theta)) * texRadius + 0.5f;
      ++newGeoVert;
      ++newTexVert;

      if (NTempest::CMath::fabs_(phi) < 0.00000095367432f || NTempest::CMath::fabs_(phi - PI * 2.0f) < 0.00000095367432f) {
        break;
      }
    }

    if (phiStep > 0) {
      for (int thetaStep = 0; thetaStep < 17; ++thetaStep) {
        unsigned short prev = NTempest::CMath::fabs_(prevPhi) < 0.00000095367432f ? 0 : static_cast<unsigned short>(thetaStep % 16);
        unsigned short current = NTempest::CMath::fabs_(phi - PI * 2.0f) < 0.00000095367432f ? 0 : static_cast<unsigned short>(thetaStep % 16);
        *newIndex++ = static_cast<unsigned short>(prevRowIdx + prev);
        *newIndex++ = static_cast<unsigned short>(thisRowIdx + current);
      }
    }

    prevRowIdx = thisRowIdx;
    prevPhi = phi;
  }

  m_nVerts = static_cast<unsigned short>(newGeoVert - m_geoVerts.Ptr());
  m_nIndices = 102;
}

void DNSky::GenTexture(unsigned int w, unsigned int h, NTempest::CImVector *texels) {
  unsigned int         halfh = h >> 1;
  float                halfhFloat = static_cast<float>(halfh);
  NTempest::CImVector *texptr = texels;

  {
    for (unsigned int i = 0; i < 5; ++i) {
      unsigned int startY = static_cast<unsigned int>((m_stripSizes[i] + m_stripSizes[i]) * halfhFloat);
      unsigned int endY = static_cast<unsigned int>((m_stripSizes[i + 1] + m_stripSizes[i + 1]) * halfhFloat);
      float        blend = 0.0f;
      unsigned int next = i + 1;
      float        delta = 1.0f / (endY - startY + 1);

      if (i == 4) {
        next = 4;
      }

      for (unsigned int y = startY; y < endY; ++y) {
        unsigned char       b = static_cast<unsigned char>(NTempest::CMath::fint_mi(
            Interp(static_cast<float>(s_dnInfo.light.SkyArray[i].b), static_cast<float>(s_dnInfo.light.SkyArray[next].b), blend)
        ));
        unsigned char       g = static_cast<unsigned char>(NTempest::CMath::fint_mi(
            Interp(static_cast<float>(s_dnInfo.light.SkyArray[i].g), static_cast<float>(s_dnInfo.light.SkyArray[next].g), blend)
        ));
        unsigned char       r = static_cast<unsigned char>(NTempest::CMath::fint_mi(
            Interp(static_cast<float>(s_dnInfo.light.SkyArray[i].r), static_cast<float>(s_dnInfo.light.SkyArray[next].r), blend)
        ));
        NTempest::CImVector clr;

        clr.Set(0xFF, r, g, b);
        for (unsigned int x = 0; x < w; ++x) {
          texptr[x] = clr;
        }

        texptr += w;
        blend += delta;
      }
    }
  }

  NTempest::CImVector *src = texptr - w;
  {
    for (unsigned int mirrorY = 0; mirrorY < halfh; ++mirrorY) {
      for (unsigned int mirrorX = 0; mirrorX < w; ++mirrorX) {
        texptr[mirrorX] = src[mirrorX];
      }

      texptr += w;
      src -= w;
    }
  }
}

void DNPlanet::GenGeometry(
    NTempest::C3Vector  *geov,
    NTempest::C2Vector  *texv,
    NTempest::CImVector *clrv,
    unsigned short      *idx,
    unsigned long       &vertCount,
    unsigned long       &idxCount
) {
  static const NTempest::C3Vector s_geov[6] = {NTempest::C3Vector(0.0f, -0.5f, 0.5f),   NTempest::C3Vector(0.0f, 0.5f, 0.5f),
                                               NTempest::C3Vector(0.0f, -0.5f, -0.5f),  NTempest::C3Vector(0.0f, 0.5f, -0.5f),
                                               NTempest::C3Vector(0.0f, -0.5f, 100.0f), NTempest::C3Vector(0.0f, 0.5f, 100.0f)};
  static const NTempest::C2Vector s_texv[6] = {NTempest::C2Vector(0.0f, 0.0f), NTempest::C2Vector(0.0f, 1.0f),   NTempest::C2Vector(1.0f, 0.0f),
                                               NTempest::C2Vector(1.0f, 1.0f), NTempest::C2Vector(100.0f, 0.0f), NTempest::C2Vector(100.0f, 1.0f)};

  for (unsigned int i = 0; i < 6; ++i) {
    geov[i] = s_geov[i] * m_period;
    texv[i] = s_texv[i];
    clrv[i] = m_color;
  }

  idx[0] = 0;
  idx[1] = 2;
  idx[2] = 1;
  idx[3] = 3;
  idxCount = 4;
  vertCount = 0;

  float localZ = m_pos.z - DayNightGetInfo()->playerPos.z;
  float clip1 = localZ + geov[0].z;
  float clip2 = localZ + geov[2].z;
  if (clip1 > 0.0f || clip2 > 0.0f) {
    if (clip1 <= 0.0f || clip2 <= 0.0f) {
      vertCount = 4;
      float clipt = clip1 / (clip1 - clip2);
      geov[2].z = geov[3].z = geov[0].z + (geov[2].z - geov[0].z) * clipt;
      texv[2].y = texv[3].y = clipt;
    } else {
      vertCount = 4;
    }

    float fadeBegin = DayNightGetInfo()->farClip * 0.4f;
    float fade1 = clip1 - fadeBegin;
    float fade2 = clip2 - fadeBegin;
    if (fade1 > 0.001f && fade2 < 0.001f) {
      vertCount = 6;
      idxCount = 8;
      idx[0] = 0;
      idx[1] = 4;
      idx[2] = 1;
      idx[3] = 5;
      idx[4] = 5;
      idx[5] = 3;
      idx[6] = 4;
      idx[7] = 2;
      float clipt = fade1 / (fade1 - fade2);
      geov[4].z = geov[5].z = geov[0].z + (geov[2].z - geov[0].z) * clipt;
      texv[4].y = texv[5].y = texv[0].y + (texv[2].y - texv[0].y) * clipt;
    }

    for (unsigned int i = 0; i < vertCount; ++i) {
      float fade = localZ + geov[i].z - fadeBegin;
      if (fade < 0.001f) {
        clrv[i].a = static_cast<unsigned char>((fadeBegin - -fade) / fadeBegin * 255.0f);
      }
    }
  }
}

static int ConsoleCommand_SkyCloudDensity(const char *__formal, const char *args) {
  char  msg[256];
  float density;

  if (sscanf(args, "%f", &density) == 1) {
    if (density < 0.0f) {
      density = 0.0f;
    } else if (density > 1.0f) {
      density = 1.0f;
    }

    s_clouds.OverrideDensitySharpness(density, 0.0f);
    if (density == 0.0f) {
      sprintf(msg, "SkyCloudDensity set from data");
    } else {
      sprintf(msg, "SkyCloudDensity set from override to %f", density);
    }
    ConsoleWrite(msg, DEFAULT_COLOR);
  } else {
    ConsoleWrite("SkyCloudDensity set from data", DEFAULT_COLOR);
  }

  return 0;
}

static bool CloudLODCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
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

static int ConsoleCommand_SkyCloudLayers(const char *__formal, const char *args) {
  int layers = -1;
  sscanf(args, "%d", &layers);
  if (static_cast<unsigned int>(layers) > 1) {
    ConsoleWrite("CloudLayers must be in the range [0, 1]", DEFAULT_COLOR);
    return 0;
  }

  s_clouds.SetLayers(layers);
  ConsoleWrite("CloudLayers set", DEFAULT_COLOR);
  return 1;
}

static int ConsoleCommand_SkySunGlare(const char *__formal, const char *args) {
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

void DNStars::Update() {
  m_pos = s_dnInfo.cameraPos;
  m_color.a = InterpTable(m_fadeTable, 4, s_dnInfo.dayProgression) * 254.0f + 1.0f;
}

static int ConsoleCommand_SkyShow(const char *__formal, const char *args) {
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

void DayNightInitialize(const char *litFile) {
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
  s_sunGlare.m_fadeTable[0] = NTempest::C2Vector(0.27083334f, 0.0f);
  s_sunGlare.m_fadeTable[1] = NTempest::C2Vector(0.3125f, 1.0f);
  s_sunGlare.m_fadeTable[2] = NTempest::C2Vector(0.8125f, 1.0f);
  s_sunGlare.m_fadeTable[3] = NTempest::C2Vector(0.875f, 0.0f);
  s_sunGlare.m_scaleMin = 0.5f;
  s_sunGlare.m_scaleMax = 6.0f;
  s_sunGlare.m_dotMin = 0.7f;
  s_sunGlare.m_alphaMin = 0.5f;
  s_sunGlare.m_alphaMax = 1.0f;

  s_moonGlare.Initialize("Textures\\moonGlare.blp");
  s_moonGlare.m_baseScale = 2.0f;
  s_moonGlare.m_fadeRate = 1.0f;
  s_moonGlare.m_enabled = 1;
  s_moonGlare.m_fadeTable[0] = NTempest::C2Vector(0.083333336f, 0.0f);
  s_moonGlare.m_fadeTable[1] = NTempest::C2Vector(0.13541667f, 1.0f);
  s_moonGlare.m_fadeTable[2] = NTempest::C2Vector(0.94791669f, 1.0f);
  s_moonGlare.m_fadeTable[3] = NTempest::C2Vector(0.99930555f, 0.0f);
  s_moonGlare.m_scaleMin = 1.0f;
  s_moonGlare.m_scaleMax = 1.0f;
  s_moonGlare.m_dotMin = 0.7f;
  s_moonGlare.m_alphaMin = 0.1f;
  s_moonGlare.m_alphaMax = 1.0f;

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

  s_slimeLight.AmbientColor = 0xFF6F2000;
  s_slimeLight.DirectColor = 0xFFFF5500;
  s_slimeLight.FogEnd = 27.0f;
  s_slimeLight.FogStartScalar = -2.0f;
  s_slimeLight.ShadowOpacity = 0xFF808080;
  s_slimeLight.Darkness = 0.0f;
  s_slimeLight.SkyArray[5] = 0xFFC83400;

  s_magmaLight.AmbientColor = 0xFF006000;
  s_magmaLight.DirectColor = 0xFF4E9314;
  s_magmaLight.FogEnd = 50.0f;
  s_magmaLight.FogStartScalar = -1.0f;
  s_magmaLight.ShadowOpacity = 0xFF808080;
  s_magmaLight.Darkness = 0.0f;
  s_magmaLight.SkyArray[5] = 0xFF00FF00;

  s_dnInfo.eclipseAmount = 0;
}

void DayNightDestroy() {
  if (s_initialized) {
    s_clouds.Destroy();

    for (unsigned int i = 0; i < 3; ++i) {
      s_planets[i].Destroy();
    }

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
  if (s_dnInfo.cameraDir.y * s_dnInfo.cameraDir.y + s_dnInfo.cameraDir.x * s_dnInfo.cameraDir.x <= 0.0001f) {
    s_dnInfo.faceAngle = atan2(s_dnInfo.cameraDir.z, s_dnInfo.cameraDir.x);
  } else {
    s_dnInfo.faceAngle = atan2(s_dnInfo.cameraDir.y, s_dnInfo.cameraDir.x);
  }

  if (s_dnInfo.faceAngle < 0.0f) {
    s_dnInfo.faceAngle += 2.0f * PI;
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
  s_dnInfo.eclipseAmount = static_cast<unsigned char>(NTempest::CMath::fuint_n(amount * 255.0f));
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
  if (s_initialized) {
    s_sunGlare.Render();
    s_moonGlare.Render();
  }
}

void DayNightRenderSky() {
  if (s_dnInfo.showSky) {
    if (CWorld::SceneCamLiquidStatus() == 15) {
      GxSceneSetClearColor(NTempest::CImVector(0xFF000000));
      if (!GxMasterEnable(GxMasterEnable_PolygonFill)) {
        GxSceneClear(3);
      }

      s_sky.Render();
      s_stars.Render();
      for (unsigned int i = 0; i < 3; ++i) {
        s_planets[i].Render();
      }
      s_clouds.Render();
    } else {
      GxSceneSetClearColor(s_dnInfo.fogInfo.color);
      GxSceneClear(3);
    }
  } else {
    GxSceneSetClearColor(NTempest::CImVector(0xFF000000));
    GxSceneClear(3);
  }
}

void DayNightSkyTexCallback(
    EGxTexCommand cmd,
    unsigned int  w,
    unsigned int  h,
    unsigned int  d,
    unsigned int  mipLevel,
    void         *userArg,
    unsigned int &texelStrideInBytes,
    const void  *&texels
) {
  if (cmd == GxTex_Latch && mipLevel == 0) {
    texelStrideInBytes = w * sizeof(NTempest::CImVector);
    s_sky.GenTexture(w, h, static_cast<NTempest::CImVector *>(userArg));
    texels = userArg;
  }
}
