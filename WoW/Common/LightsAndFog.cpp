#include "LightsAndFog.h"

#include <storm.h>

struct DiskLightDataItem {
  int         m_highlightCount[18];
  LightMarker m_highlightMarker[18][32];
  float       m_fogEnd[32];
  float       m_fogStartScaler[32];
  int         m_highlightSky;
  float       m_skyData[4][32];
  int         m_cloudMask;
};

unsigned int ReadSingleLightGroup(SFile *lightdata, LightDataItem *dataitem) {
  DiskLightDataItem diskdataitem;
  int               markerCount;
  int               i;
  int               n;

  if (!SFile::Read(lightdata, &diskdataitem, sizeof(diskdataitem), 0, 0, 0)) {
    return 0;
  }

  for (i = 0; i < 18; ++i) {
    markerCount = diskdataitem.m_highlightCount[i];
    dataitem->m_highlightMarker[i].SetCount(markerCount);
    for (n = 0; n < markerCount; ++n) {
      dataitem->m_highlightMarker[i][n] = diskdataitem.m_highlightMarker[i][n];
    }

    if (i == 9) {
      dataitem->m_skyData.SetCount(markerCount);
      for (n = 0; n < markerCount; ++n) {
        dataitem->m_skyData[n].m_skyData[0] = diskdataitem.m_skyData[0][n];
        dataitem->m_skyData[n].m_skyData[1] = diskdataitem.m_skyData[1][n];
        dataitem->m_skyData[n].m_skyData[2] = diskdataitem.m_skyData[2][n];
        dataitem->m_skyData[n].m_skyData[3] = diskdataitem.m_skyData[3][n];
      }
    } else if (i == 7) {
      dataitem->m_fogData.SetCount(markerCount);
      for (n = 0; n < markerCount; ++n) {
        dataitem->m_fogData[n].m_fogEnd = diskdataitem.m_fogEnd[n];
        dataitem->m_fogData[n].m_fogStartScaler = diskdataitem.m_fogStartScaler[n];
      }
    }
  }

  dataitem->m_highlightSky = diskdataitem.m_highlightSky;
  dataitem->m_cloudMask = diskdataitem.m_cloudMask;
  return 1;
}

unsigned int LoadLightsAndFog(const char *filename, LightGroup *lightgroup) {
  int    versionNumber;
  int    lightCount;
  SFile *lightdata = 0;

  if (SFile::Open(filename, &lightdata) && SFile::Read(lightdata, &versionNumber, sizeof(versionNumber), 0, 0, 0) &&
      versionNumber == static_cast<int>(0x80000004) && SFile::Read(lightdata, &lightCount, sizeof(lightCount), 0, 0, 0))
  {
    lightgroup->m_lightData.SetCount(lightCount);

    int i;
    for (i = 0; i < lightCount; ++i) {
      if (!SFile::Read(lightdata, &lightgroup->m_lightData[i].m_lightlist, sizeof(LightListData), 0, 0, 0)) {
        break;
      }
    }

    if (i == lightCount) {
      for (i = 0; i < lightCount; ++i) {
        LightData &data = lightgroup->m_lightData[i];
        if (!ReadSingleLightGroup(lightdata, &data.m_lightdata) || !ReadSingleLightGroup(lightdata, &data.m_stormdata) ||
            !ReadSingleLightGroup(lightdata, &data.m_lightdataWater) || !ReadSingleLightGroup(lightdata, &data.m_stormdataWater))
        {
          break;
        }
      }

      if (i == lightCount) {
        SFile::Close(lightdata);
        return 1;
      }
    }
  }

  lightgroup->m_lightData.Clear();
  if (lightdata) {
    SFile::Close(lightdata);
  }
  return 0;
}

void CalcIndividualLightColor(int time, int oband, LightDataItem *lightdata, NTempest::CImVector *color, float *distance) {
  int band = oband;
  if (oband >= 18) {
    band = 7;
  }
  if (oband >= 20) {
    band = 9;
  }

  TSFixedArray<LightMarker> &markers = lightdata->m_highlightMarker[band];
  for (unsigned int i = 0; i < markers.Count(); ++i) {
    unsigned int next = (i + 1) % markers.Count();
    int          t1 = markers[i].time;
    int          t2 = markers[next].time;
    int          sample = time;

    if (t2 > t1) {
      if (sample < t1 || sample > t2) {
        continue;
      }
    } else {
      if (sample > t2 && sample < t1) {
        continue;
      }
      t2 += 2880;
      if (sample < t1) {
        sample += 2880;
      }
    }

    int width = t2 - t1;
    int position = sample - t1;
    if (oband >= 18) {
      if (distance) {
        float d1;
        float d2;
        switch (oband) {
          case 18:
            d1 = lightdata->m_fogData[i].m_fogEnd;
            d2 = lightdata->m_fogData[next].m_fogEnd;
            break;
          case 19:
            d1 = lightdata->m_fogData[i].m_fogStartScaler;
            d2 = lightdata->m_fogData[next].m_fogStartScaler;
            break;
          case 20:
            d1 = lightdata->m_skyData[i].m_skyData[0];
            d2 = lightdata->m_skyData[next].m_skyData[0];
            break;
          case 21:
            d1 = lightdata->m_skyData[i].m_skyData[1];
            d2 = lightdata->m_skyData[next].m_skyData[1];
            break;
          case 22:
            d1 = lightdata->m_skyData[i].m_skyData[2];
            d2 = lightdata->m_skyData[next].m_skyData[2];
            break;
          case 23:
            d1 = lightdata->m_skyData[i].m_skyData[3];
            d2 = lightdata->m_skyData[next].m_skyData[3];
            break;
          default:
            d1 = 0.0f;
            d2 = 0.0f;
            break;
        }
        *distance = d1 + (d2 - d1) * position / width;
      }
    } else {
      NTempest::CImVector &c1 = markers[i].color;
      NTempest::CImVector &c2 = markers[next].color;
      color->r = static_cast<unsigned char>(c1.r + position * (c2.r - c1.r) / width);
      color->g = static_cast<unsigned char>(c1.g + position * (c2.g - c1.g) / width);
      color->b = static_cast<unsigned char>(c1.b + position * (c2.b - c1.b) / width);
      color->a = 255;
    }
    break;
  }
}

static unsigned long BlendLightValue(unsigned long base, unsigned long storm, int stormpercent) {
  return stormpercent * storm / 100 + (100 - stormpercent) * base / 100;
}

void CalcLightColors(int time, CurrentLight *current, LightDataItem *lightdata, LightDataItem *stormdata, int stormpercent) {
  CalcIndividualLightColor(time, 0, lightdata, &current->DirectColor, 0);
  CalcIndividualLightColor(time, 1, lightdata, &current->AmbientColor, 0);
  for (int i = 0; i < 6; ++i) {
    CalcIndividualLightColor(time, i + 2, lightdata, &current->SkyArray[i], 0);
  }
  CalcIndividualLightColor(time, 8, lightdata, &current->ShadowOpacity, 0);
  for (i = 0; i < 5; ++i) {
    CalcIndividualLightColor(time, i + 9, lightdata, &current->CloudArray[i], 0);
  }
  for (i = 0; i < 4; ++i) {
    CalcIndividualLightColor(time, i + 14, lightdata, &current->WaterArray[i], 0);
  }
  CalcIndividualLightColor(time, 18, lightdata, 0, &current->FogEnd);
  CalcIndividualLightColor(time, 19, lightdata, 0, &current->FogStartScalar);
  CalcIndividualLightColor(time, 21, lightdata, 0, &current->CloudData[1]);

  if (stormdata && stormpercent > 0) {
    CurrentLight currentStorm;
    CalcIndividualLightColor(time, 0, stormdata, &currentStorm.DirectColor, 0);
    CalcIndividualLightColor(time, 1, stormdata, &currentStorm.AmbientColor, 0);
    for (i = 0; i < 6; ++i) {
      CalcIndividualLightColor(time, i + 2, stormdata, &currentStorm.SkyArray[i], 0);
    }
    CalcIndividualLightColor(time, 8, stormdata, &currentStorm.ShadowOpacity, 0);
    for (i = 0; i < 5; ++i) {
      CalcIndividualLightColor(time, i + 9, stormdata, &currentStorm.CloudArray[i], 0);
    }
    for (i = 0; i < 4; ++i) {
      CalcIndividualLightColor(time, i + 14, stormdata, &currentStorm.WaterArray[i], 0);
    }
    CalcIndividualLightColor(time, 18, stormdata, 0, &currentStorm.FogEnd);
    CalcIndividualLightColor(time, 19, stormdata, 0, &currentStorm.FogStartScalar);
    CalcIndividualLightColor(time, 21, stormdata, 0, &currentStorm.CloudData[1]);

    current->DirectColor = BlendLightValue(*current->DirectColor.IV_(), *currentStorm.DirectColor.IV_(), stormpercent);
    current->AmbientColor = BlendLightValue(*current->AmbientColor.IV_(), *currentStorm.AmbientColor.IV_(), stormpercent);
    for (i = 0; i < 6; ++i) {
      current->SkyArray[i] = BlendLightValue(*current->SkyArray[i].IV_(), *currentStorm.SkyArray[i].IV_(), stormpercent);
    }
    for (i = 0; i < 5; ++i) {
      current->CloudArray[i] = BlendLightValue(*current->CloudArray[i].IV_(), *currentStorm.CloudArray[i].IV_(), stormpercent);
    }
    current->FogEnd = currentStorm.FogEnd * stormpercent * 0.01f + current->FogEnd * (100 - stormpercent) * 0.01f;
    current->FogStartScalar = currentStorm.FogStartScalar * stormpercent * 0.01f + current->FogStartScalar * (100 - stormpercent) * 0.01f;
    current->ShadowOpacity = BlendLightValue(*current->ShadowOpacity.IV_(), *currentStorm.ShadowOpacity.IV_(), stormpercent);
    current->CloudData[1] = currentStorm.CloudData[1] * stormpercent * 0.01f + current->CloudData[1] * (100 - stormpercent) * 0.01f;
  }

  current->Darkness = static_cast<float>(lightdata->m_cloudMask);
}
