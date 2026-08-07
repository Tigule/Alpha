#include <Base/Base.h>

#include "Model/ModelInternal.h"

#include "Gx/CGxDevice.h"
#include "Gxu/IGxuLight.h"
#include "MDLFile/MDLTypes.h"

BYTE *MDLFileBinarySeek(BYTE *fileData, UINT fileBytes, DWORD sectionTag);

static DWORD CreateGxLight(const MDLLIGHTSECTION &data) {
  DWORD lightId = GxuLightCreate();
  if (!lightId) {
    return 0;
  }

  CGxLight *light = GxuLightLock(lightId);
  if (light) {
    light->m_isOmni = data.type == LIGHTTYPE_OMNI;
    light->m_dirColor.Set(1.0f, data.staticColor.r, data.staticColor.g, data.staticColor.b);
    light->m_dirIntensity = data.staticIntensity;
    light->m_ambColor.Set(1.0f, data.staticAmbColor.r, data.staticAmbColor.g, data.staticAmbColor.b);
    light->m_ambIntensity = data.staticAmbIntensity;
    light->m_enabled = 0;

    GxuLightUnlock(lightId);
  }

  return lightId;
}

static DWORD CreateGxLight(BYTE *lightData) {
  DWORD lightId = GxuLightCreate();
  if (!lightId) {
    return 0;
  }

  CGxLight *light = GxuLightLock(lightId);
  if (light) {
    UINT        staticDataOffset = *reinterpret_cast<const UINT *>(lightData);
    const BYTE *staticData = lightData + staticDataOffset;

    UINT         type = *reinterpret_cast<const UINT *>(staticData);
    const float *values = reinterpret_cast<const float *>(staticData + 12);

    light->m_isOmni = type == 0;
    light->m_dirColor.Set(1.0f, values[0], values[1], values[2]);
    light->m_dirIntensity = values[3];
    light->m_ambColor.Set(1.0f, values[4], values[5], values[6]);
    light->m_ambIntensity = values[7];
    light->m_enabled = 0;

    GxuLightUnlock(lightId);
  }

  return lightId;
}

BOOL MdlReadLoadLights(const MDLDATA &data, CModelComplex *modelptr) {
  FATALASSERT(modelptr);
  UINT numLights = data.lights.Count();
  modelptr->m_lights.SetCount(numLights);
  UINT i;
  for (i = 0; i < numLights; ++i) {
    modelptr->m_lights[i] = CreateGxLight(data.lights[i]);
  }
  return 1;
}

void MdxReadLights(BYTE *data, UINT fileBytes, CModelComplex *modelptr) {
  ASSERT(data);
  ASSERT(modelptr);

  BYTE *section = MDLFileBinarySeek(data, fileBytes, 'ETIL');
  if (!section) {
    return;
  }

  UINT  sectionBytes = *reinterpret_cast<UINT *>(section) - 4;
  UINT  numLights = *reinterpret_cast<UINT *>(section + 4);
  BYTE *lightData = section + 8;

  modelptr->m_lights.SetCount(numLights);

  for (UINT i = 0; i < numLights; ++i) {
    UINT bytesThisLight = *reinterpret_cast<UINT *>(lightData);
    modelptr->m_lights[i] = CreateGxLight(lightData + 4);

    ASSERT(sectionBytes >= bytesThisLight);
    sectionBytes -= bytesThisLight;
    lightData += bytesThisLight;
  }

  ASSERT(sectionBytes == 0);
}
