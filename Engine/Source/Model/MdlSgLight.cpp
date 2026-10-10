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

  data = MDLFileBinarySeek(data, fileBytes, 'ETIL');
  if (!data) {
    return;
  }

  UINT sectionBytes = *(UINT *)data - 4;
  data += 4;
  UINT numLights = *(UINT *)data;
  data += 4;

  modelptr->m_lights.SetCount(numLights);

  for (UINT i = 0; i < numLights; ++i) {
    UINT bytesThisEmitter = *(UINT *)data;
    modelptr->m_lights[i] = CreateGxLight(data + 4);
    data += bytesThisEmitter;

    ASSERT(sectionBytes >= bytesThisEmitter);
    sectionBytes -= bytesThisEmitter;
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

  lightData += *(UINT *)lightData;
  light->m_isOmni = *(UINT *)lightData == 0;
  lightData += 12;
  light->m_dirColor.r = NTempest::CMath::ftol_0_256_(*(float *)lightData * 255.0f);
  lightData += 4;
  light->m_dirColor.g = NTempest::CMath::ftol_0_256_(*(float *)lightData * 255.0f);
  lightData += 4;
  light->m_dirColor.b = NTempest::CMath::ftol_0_256_(*(float *)lightData * 255.0f);
  lightData += 4;
  light->m_dirIntensity = *(float *)lightData;
  lightData += 4;
  light->m_ambColor.r = NTempest::CMath::ftol_0_256_(*(float *)lightData * 255.0f);
  lightData += 4;
  light->m_ambColor.g = NTempest::CMath::ftol_0_256_(*(float *)lightData * 255.0f);
  lightData += 4;
  light->m_ambColor.b = NTempest::CMath::ftol_0_256_(*(float *)lightData * 255.0f);
  lightData += 4;
  light->m_ambIntensity = *(float *)lightData;
  light->m_enabled = 0;

  GxuLightUnlock(lightId);

  return lightId;
}
