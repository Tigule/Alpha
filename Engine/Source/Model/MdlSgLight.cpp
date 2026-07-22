#include "Model/ModelInternal.h"

#include "Gx/CGxDevice.h"
#include "Gxu/IGxuLight.h"

unsigned char *__fastcall MDLFileBinarySeek(unsigned char *fileData, unsigned int fileBytes, unsigned long sectionTag);

static unsigned long __fastcall CreateGxLight(unsigned char *lightData) {
  unsigned long lightId = GxuLightCreate();
  if (!lightId) {
    return 0;
  }

  CGxLight *light = GxuLightLock(lightId);
  if (light) {
    unsigned int   staticDataOffset = *reinterpret_cast<unsigned int *>(lightData);
    unsigned char *staticData = lightData + staticDataOffset;

    unsigned int type = *reinterpret_cast<unsigned int *>(staticData);
    float       *values = reinterpret_cast<float *>(staticData + 12);

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

void __fastcall MdxReadLights(unsigned char *data, unsigned int fileBytes, CModelComplex *modelptr) {
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
