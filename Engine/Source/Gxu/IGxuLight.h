#pragma once

#include <Tempest/c3vector.h>
#include <storm.h>
#include <stpl.h>

class CGxLight;
struct CGxuLight;
struct CLightList;

class CGxuLightLink {
 public:
  CGxuLight            *m_light;
  CLightList           *m_list;
  float                 m_fitness;
  TSLink<CGxuLightLink> m_lightLink;
  TSLink<CGxuLightLink> m_listLink;
};

struct CLightList : public TSHashObject<CLightList, HASHKEY_DWORD> {
  CLightList() {
  }

  TSExplicitList<CGxuLightLink, 20> m_links;

  static TSHashTableReuse<CLightList, HASHKEY_DWORD, 1> s_lightHashTable;
};

extern void(*GxuLightInitialize)();
extern void(*GxuLightShutdown)();
extern unsigned long(*GxuLightCreate)();
extern void(*GxuLightDestroy)(unsigned long lightId);
extern CGxLight *(*GxuLightLock)(unsigned long lightId);
extern void(*GxuLightUnlock)(unsigned long lightId);
extern void(*GxuLightSelect)(NTempest::C3Vector worldPos, const NTempest::C3Vector &cameraWorldPos, unsigned int maxLightsToUse);
extern int(*GxuLightEnable)(unsigned long lightId);
extern void(*GxuLightEnableSet)(unsigned long lightId, int enable);
extern void(*GxuLightSetMaxLights)(unsigned int maxLightsToUse);
extern float(*GxuLightBucketSize)();
extern void(*GxuLightBucketSizeSet)(float bucketSize);
extern void(*GxuLightResetCache)();

void GxuLightFuncsSet(
    void(*initializeFunc)(),
    void(*shutDownFunc)(),
    unsigned long(*createFunc)(),
    void(*destroyFunc)(unsigned long lightId),
    CGxLight *(*lockFunc)(unsigned long lightId),
    void(*unlockFunc)(unsigned long lightId),
    void(*selectFunc)(NTempest::C3Vector worldPos, const NTempest::C3Vector &cameraWorldPos, unsigned int maxLightsToUse),
    int(*enableFunc)(unsigned long lightId),
    void(*enableSetFunc)(unsigned long lightId, int enable),
    void(*setMaxLightsFunc)(unsigned int maxLightsToUse),
    float(*bucketSizeFunc)(),
    void(*bucketSizeSetFunc)(float bucketSize),
    void(*resetCacheFunc)()
);
