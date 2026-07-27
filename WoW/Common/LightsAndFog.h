#ifndef WOW_COMMON_LIGHTSANDFOG_H
#define WOW_COMMON_LIGHTSANDFOG_H

#include "Tempest/cimvector.h"
#include "Tempest/c2ivector.h"
#include "Tempest/c3vector.h"

#include <stddef.h>
#include <stpl.h>

class SFile;

struct LightMarker {
  int                 time;
  NTempest::CImVector color;
};

struct LightDataSky {
  float m_skyData[4];
};

struct LightDataFog {
  float m_fogEnd;
  float m_fogStartScaler;
};

struct LightListData {
  NTempest::C2iVector m_chunk;
  int                 m_chunkRadius;
  NTempest::C3Vector  m_lightLocation;
  float               m_lightRadius;
  float               m_lightDropoff;
  char                m_lightName[32];
};

struct LightDataItem {
  TSFixedArray<LightMarker>  m_highlightMarker[18];
  TSFixedArray<LightDataSky> m_skyData;
  TSFixedArray<LightDataFog> m_fogData;
  int                        m_highlightSky;
  int                        m_cloudMask;
};

struct LightData {
  LightListData m_lightlist;
  LightDataItem m_lightdata;
  LightDataItem m_stormdata;
  LightDataItem m_lightdataWater;
  LightDataItem m_stormdataWater;
};

struct LightGroup {
  TSFixedArray<LightData> m_lightData;
};

struct CurrentLight {
  NTempest::CImVector DirectColor;
  NTempest::CImVector AmbientColor;
  NTempest::CImVector SkyArray[6];
  NTempest::CImVector CloudArray[5];
  NTempest::CImVector WaterArray[4];
  float               FogEnd;
  float               FogStartScalar;
  NTempest::CImVector ShadowOpacity;
  float               Darkness;
  float               CloudData[4];
};

unsigned int ReadSingleLightGroup(SFile *lightdata, LightDataItem *dataitem);
unsigned int LoadLightsAndFog(const char *filename, LightGroup *lightgroup);
void CalcIndividualLightColor(int time, int oband, LightDataItem *lightdata, NTempest::CImVector *color, float *distance);
void CalcLightColors(int time, CurrentLight *current, LightDataItem *lightdata, LightDataItem *stormdata, int stormpercent);

#endif
