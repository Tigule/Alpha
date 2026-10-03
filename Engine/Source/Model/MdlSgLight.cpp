#include <Base/Base.h>

#include "Model/ModelInternal.h"

#include "Gx/CGxDevice.h"
#include "Gxu/IGxuLight.h"
#include "MDLFile/MDLTypes.h"

BYTE *MDLFileBinarySeek(BYTE *fileData, UINT fileBytes, DWORD sectionTag);

static DWORD CreateGxLight(const MDLLIGHTSECTION &data);
static DWORD CreateGxLight(BYTE *lightData);

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

static DWORD CreateGxLight(const MDLLIGHTSECTION &data) {
  DWORD lightId = GxuLightCreate();
  if (!lightId) {
    return 0;
  }

  CGxLight *light = GxuLightLock(lightId);
  if (!light) {
    return lightId;
  }

  light->m_isOmni = data.type == LIGHTTYPE_OMNI;
  light->m_dirColor.r = NTempest::CMath::ftol_0_256_(data.staticColor.r * 255.0f);
  light->m_dirColor.g = NTempest::CMath::ftol_0_256_(data.staticColor.g * 255.0f);
  light->m_dirColor.b = NTempest::CMath::ftol_0_256_(data.staticColor.b * 255.0f);
  light->m_dirIntensity = data.staticIntensity;
  light->m_ambColor.r = NTempest::CMath::ftol_0_256_(data.staticAmbColor.r * 255.0f);
  light->m_ambColor.g = NTempest::CMath::ftol_0_256_(data.staticAmbColor.g * 255.0f);
  light->m_ambColor.b = NTempest::CMath::ftol_0_256_(data.staticAmbColor.b * 255.0f);
  light->m_ambIntensity = data.staticAmbIntensity;
  light->m_enabled = 0;

  GxuLightUnlock(lightId);

  return lightId;
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

static DWORD CreateGxLight(BYTE *lightData) {
  DWORD lightId = GxuLightCreate();
  if (!lightId) {
    return 0;
  }

  CGxLight *light = GxuLightLock(lightId);
  if (!light) {
    return lightId;
  }

  lightData += *reinterpret_cast<UINT *>(lightData);
  light->m_isOmni = *reinterpret_cast<UINT *>(lightData) == 0;
  lightData += 12;
  light->m_dirColor.r = NTempest::CMath::ftol_0_256_(*reinterpret_cast<float *>(lightData) * 255.0f);
  lightData += 4;
  light->m_dirColor.g = NTempest::CMath::ftol_0_256_(*reinterpret_cast<float *>(lightData) * 255.0f);
  lightData += 4;
  light->m_dirColor.b = NTempest::CMath::ftol_0_256_(*reinterpret_cast<float *>(lightData) * 255.0f);
  lightData += 4;
  light->m_dirIntensity = *reinterpret_cast<float *>(lightData);
  lightData += 4;
  light->m_ambColor.r = NTempest::CMath::ftol_0_256_(*reinterpret_cast<float *>(lightData) * 255.0f);
  lightData += 4;
  light->m_ambColor.g = NTempest::CMath::ftol_0_256_(*reinterpret_cast<float *>(lightData) * 255.0f);
  lightData += 4;
  light->m_ambColor.b = NTempest::CMath::ftol_0_256_(*reinterpret_cast<float *>(lightData) * 255.0f);
  lightData += 4;
  light->m_ambIntensity = *reinterpret_cast<float *>(lightData);
  light->m_enabled = 0;

  GxuLightUnlock(lightId);

  return lightId;
}
