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

static bool ReadSingleLightGroup(SFile *lightdata, LightDataItem *dataitem) {
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

bool LoadLightsAndFog(LPCSTR filename, LightGroup *lightgroup) {
  int    versionNumber;
  int    lightCount;
  SFile *lightdata = 0;

  if (SFile::Open(filename, &lightdata) && SFile::Read(lightdata, &versionNumber, sizeof(versionNumber), 0, 0, 0) &&
      versionNumber == (int)0x80000004 && SFile::Read(lightdata, &lightCount, sizeof(lightCount), 0, 0, 0))
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
  for (int n = 0; n < (int)markers.Count(); ++n) {
    int x1 = markers[n].time;
    int next = (n + 1) % markers.Count();
    int t2 = markers[next].time;

    if (t2 <= x1) {
      if (time > t2 && time < x1) {
        continue;
      }
      t2 += 2880;
      if (time < x1) {
        time += 2880;
      }
    } else {
      if (time < x1 || time > t2) {
        continue;
      }
    }

    int w = t2 - x1;
    int i = time - x1;
    if (oband < 18) {
      int col1 = markers[n].color.r;
      color->r = col1 + i * (markers[next].color.r - col1) / w;
      col1 = markers[n].color.g;
      color->g = col1 + i * (markers[next].color.g - col1) / w;
      col1 = markers[n].color.b;
      color->b = col1 + i * (markers[next].color.b - col1) / w;
      color->a = 255;
    } else if (distance) {
      float d1;
      float d2;
      switch (oband) {
        case 18:
          d1 = lightdata->m_fogData[n].m_fogEnd;
          d2 = lightdata->m_fogData[next].m_fogEnd;
          break;
        case 19:
          d1 = lightdata->m_fogData[n].m_fogStartScaler;
          d2 = lightdata->m_fogData[next].m_fogStartScaler;
          break;
        case 20:
          d1 = lightdata->m_skyData[n].m_skyData[0];
          d2 = lightdata->m_skyData[next].m_skyData[0];
          break;
        case 21:
          d1 = lightdata->m_skyData[n].m_skyData[1];
          d2 = lightdata->m_skyData[next].m_skyData[1];
          break;
        case 22:
          d1 = lightdata->m_skyData[n].m_skyData[2];
          d2 = lightdata->m_skyData[next].m_skyData[2];
          break;
        case 23:
          d1 = lightdata->m_skyData[n].m_skyData[3];
          d2 = lightdata->m_skyData[next].m_skyData[3];
          break;
        default:
          d1 = 0.0f;
          d2 = 0.0f;
          break;
      }
      *distance = (d2 - d1) * i / w + d1;
    }
  }
}

static DWORD BlendLightValue(DWORD base, DWORD storm, int stormpercent) {
  return stormpercent * storm / 100 + (100 - stormpercent) * base / 100;
}

void CalcLightColors(int time, CurrentLight *current, LightDataItem *lightdata, LightDataItem *stormdata, int stormpercent) {
  CalcIndividualLightColor(time, 0, lightdata, &current->DirectColor, 0);
  CalcIndividualLightColor(time, 1, lightdata, &current->AmbientColor, 0);
  CalcIndividualLightColor(time, 2, lightdata, &current->SkyArray[0], 0);
  CalcIndividualLightColor(time, 3, lightdata, &current->SkyArray[1], 0);
  CalcIndividualLightColor(time, 4, lightdata, &current->SkyArray[2], 0);
  CalcIndividualLightColor(time, 5, lightdata, &current->SkyArray[3], 0);
  CalcIndividualLightColor(time, 6, lightdata, &current->SkyArray[4], 0);
  CalcIndividualLightColor(time, 7, lightdata, &current->SkyArray[5], 0);
  CalcIndividualLightColor(time, 8, lightdata, &current->ShadowOpacity, 0);
  CalcIndividualLightColor(time, 9, lightdata, &current->CloudArray[0], 0);
  CalcIndividualLightColor(time, 10, lightdata, &current->CloudArray[1], 0);
  CalcIndividualLightColor(time, 11, lightdata, &current->CloudArray[2], 0);
  CalcIndividualLightColor(time, 12, lightdata, &current->CloudArray[3], 0);
  CalcIndividualLightColor(time, 13, lightdata, &current->CloudArray[4], 0);
  CalcIndividualLightColor(time, 14, lightdata, &current->WaterArray[0], 0);
  CalcIndividualLightColor(time, 15, lightdata, &current->WaterArray[1], 0);
  CalcIndividualLightColor(time, 16, lightdata, &current->WaterArray[2], 0);
  CalcIndividualLightColor(time, 17, lightdata, &current->WaterArray[3], 0);
  CalcIndividualLightColor(time, 18, lightdata, 0, &current->FogEnd);
  CalcIndividualLightColor(time, 19, lightdata, 0, &current->FogStartScalar);
  CalcIndividualLightColor(time, 21, lightdata, 0, &current->CloudData[1]);

  if (stormdata && stormpercent > 0) {
    CurrentLight currentStorm;
    CalcIndividualLightColor(time, 0, stormdata, &currentStorm.DirectColor, 0);
    CalcIndividualLightColor(time, 1, stormdata, &currentStorm.AmbientColor, 0);
    CalcIndividualLightColor(time, 2, stormdata, &currentStorm.SkyArray[0], 0);
    CalcIndividualLightColor(time, 3, stormdata, &currentStorm.SkyArray[1], 0);
    CalcIndividualLightColor(time, 4, stormdata, &currentStorm.SkyArray[2], 0);
    CalcIndividualLightColor(time, 5, stormdata, &currentStorm.SkyArray[3], 0);
    CalcIndividualLightColor(time, 6, stormdata, &currentStorm.SkyArray[4], 0);
    CalcIndividualLightColor(time, 7, stormdata, &currentStorm.SkyArray[5], 0);
    CalcIndividualLightColor(time, 8, stormdata, &currentStorm.ShadowOpacity, 0);
    CalcIndividualLightColor(time, 9, stormdata, &currentStorm.CloudArray[0], 0);
    CalcIndividualLightColor(time, 10, stormdata, &currentStorm.CloudArray[1], 0);
    CalcIndividualLightColor(time, 11, stormdata, &currentStorm.CloudArray[2], 0);
    CalcIndividualLightColor(time, 12, stormdata, &currentStorm.CloudArray[3], 0);
    CalcIndividualLightColor(time, 13, stormdata, &currentStorm.CloudArray[4], 0);
    CalcIndividualLightColor(time, 14, stormdata, &currentStorm.WaterArray[0], 0);
    CalcIndividualLightColor(time, 15, stormdata, &currentStorm.WaterArray[1], 0);
    CalcIndividualLightColor(time, 16, stormdata, &currentStorm.WaterArray[2], 0);
    CalcIndividualLightColor(time, 17, stormdata, &currentStorm.WaterArray[3], 0);
    CalcIndividualLightColor(time, 18, stormdata, 0, &currentStorm.FogEnd);
    CalcIndividualLightColor(time, 19, stormdata, 0, &currentStorm.FogStartScalar);
    CalcIndividualLightColor(time, 21, stormdata, 0, &currentStorm.CloudData[1]);

    int nonstormpercent = 100 - stormpercent;
    current->DirectColor = *current->DirectColor.IV_() * nonstormpercent / 100 + *currentStorm.DirectColor.IV_() * stormpercent / 100;
    current->AmbientColor = *current->AmbientColor.IV_() * nonstormpercent / 100 + *currentStorm.AmbientColor.IV_() * stormpercent / 100;
    current->SkyArray[0] = *current->SkyArray[0].IV_() * nonstormpercent / 100 + *currentStorm.SkyArray[0].IV_() * stormpercent / 100;
    current->SkyArray[1] = *current->SkyArray[1].IV_() * nonstormpercent / 100 + *currentStorm.SkyArray[1].IV_() * stormpercent / 100;
    current->SkyArray[2] = *current->SkyArray[2].IV_() * nonstormpercent / 100 + *currentStorm.SkyArray[2].IV_() * stormpercent / 100;
    current->SkyArray[3] = *current->SkyArray[3].IV_() * nonstormpercent / 100 + *currentStorm.SkyArray[3].IV_() * stormpercent / 100;
    current->SkyArray[4] = *current->SkyArray[4].IV_() * nonstormpercent / 100 + *currentStorm.SkyArray[4].IV_() * stormpercent / 100;
    current->SkyArray[5] = *current->SkyArray[5].IV_() * nonstormpercent / 100 + *currentStorm.SkyArray[5].IV_() * stormpercent / 100;
    current->CloudArray[0] = *current->CloudArray[0].IV_() * nonstormpercent / 100 + *currentStorm.CloudArray[0].IV_() * stormpercent / 100;
    current->CloudArray[1] = *current->CloudArray[1].IV_() * nonstormpercent / 100 + *currentStorm.CloudArray[1].IV_() * stormpercent / 100;
    current->CloudArray[2] = *current->CloudArray[2].IV_() * nonstormpercent / 100 + *currentStorm.CloudArray[2].IV_() * stormpercent / 100;
    current->CloudArray[3] = *current->CloudArray[3].IV_() * nonstormpercent / 100 + *currentStorm.CloudArray[3].IV_() * stormpercent / 100;
    current->CloudArray[4] = *current->CloudArray[4].IV_() * nonstormpercent / 100 + *currentStorm.CloudArray[4].IV_() * stormpercent / 100;
    current->FogEnd = currentStorm.FogEnd * stormpercent * 0.01f + current->FogEnd * nonstormpercent * 0.01f;
    current->FogStartScalar = currentStorm.FogStartScalar * stormpercent * 0.01f + current->FogStartScalar * nonstormpercent * 0.01f;
    current->ShadowOpacity = *current->ShadowOpacity.IV_() * nonstormpercent / 100 + *currentStorm.ShadowOpacity.IV_() * stormpercent / 100;
    current->CloudData[1] = currentStorm.CloudData[1] * stormpercent * 0.01f + current->CloudData[1] * nonstormpercent * 0.01f;
  }

  current->Darkness = lightdata->m_highlightSky;
}
