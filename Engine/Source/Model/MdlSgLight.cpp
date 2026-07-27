#include "Model/ModelInternal.h"

#include "Gx/CGxDevice.h"
#include "Gxu/IGxuLight.h"
#include "MDLFile/MDLTypes.h"

unsigned char *MDLFileBinarySeek(unsigned char *fileData, unsigned int fileBytes, unsigned long sectionTag);

static unsigned long CreateGxLight(const unsigned char *lightData) {
  unsigned long lightId = GxuLightCreate();
  if (!lightId) {
    return 0;
  }

  CGxLight *light = GxuLightLock(lightId);
  if (light) {
    unsigned int   staticDataOffset = *reinterpret_cast<const unsigned int *>(lightData);
    const unsigned char *staticData = lightData + staticDataOffset;

    unsigned int type = *reinterpret_cast<const unsigned int *>(staticData);
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

int MdlReadLoadLights(const MDLDATA& data, CModelComplex* modelptr) {
  FATALASSERT(modelptr);
  unsigned int numLights = data.lights.Count();
  modelptr->m_lights.SetCount(numLights);
  const unsigned char *lightData =
      reinterpret_cast<const unsigned char *>(data.lights.Ptr());
  unsigned int i;
  for (i = 0; i < numLights; ++i) {
    modelptr->m_lights[i] =
        CreateGxLight(lightData + i * 416);
  }
  return 1;
}

void MdxReadLights(unsigned char *data, unsigned int fileBytes, CModelComplex *modelptr) {
  ASSERT(data);
  ASSERT(modelptr);

  unsigned char *section = MDLFileBinarySeek(data, fileBytes, 0x4554494C);
  if (!section) {
    return;
  }

  unsigned int   sectionBytes = *reinterpret_cast<unsigned int *>(section) - 4;
  unsigned int   numLights = *reinterpret_cast<unsigned int *>(section + 4);
  unsigned char *lightData = section + 8;

  modelptr->m_lights.SetCount(numLights);

  for (unsigned int i = 0; i < numLights; ++i) {
    unsigned int bytesThisLight = *reinterpret_cast<unsigned int *>(lightData);
    modelptr->m_lights[i] = CreateGxLight(lightData + 4);

    ASSERT(sectionBytes >= bytesThisLight);
    sectionBytes -= bytesThisLight;
    lightData += bytesThisLight;
  }

  ASSERT(sectionBytes == 0);
}
