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

extern void(__fastcall *GxuLightInitialize)();
extern void(__fastcall *GxuLightShutdown)();
extern unsigned long(__fastcall *GxuLightCreate)();
extern void(__fastcall *GxuLightDestroy)(unsigned long lightId);
extern CGxLight *(__fastcall *GxuLightLock)(unsigned long lightId);
extern void(__fastcall *GxuLightUnlock)(unsigned long lightId);
extern void(__fastcall *GxuLightSelect)(NTempest::C3Vector worldPos, const NTempest::C3Vector &cameraWorldPos, unsigned int maxLightsToUse);
extern int(__fastcall *GxuLightEnable)(unsigned long lightId);
extern void(__fastcall *GxuLightEnableSet)(unsigned long lightId, int enable);
extern void(__fastcall *GxuLightSetMaxLights)(unsigned int maxLightsToUse);
extern float(__fastcall *GxuLightBucketSize)();
extern void(__fastcall *GxuLightBucketSizeSet)(float bucketSize);
extern void(__fastcall *GxuLightResetCache)();

void __fastcall GxuLightFuncsSet(
    void(__fastcall *initializeFunc)(),
    void(__fastcall *shutDownFunc)(),
    unsigned long(__fastcall *createFunc)(),
    void(__fastcall *destroyFunc)(unsigned long lightId),
    CGxLight *(__fastcall *lockFunc)(unsigned long lightId),
    void(__fastcall *unlockFunc)(unsigned long lightId),
    void(__fastcall *selectFunc)(NTempest::C3Vector worldPos, const NTempest::C3Vector &cameraWorldPos, unsigned int maxLightsToUse),
    int(__fastcall *enableFunc)(unsigned long lightId),
    void(__fastcall *enableSetFunc)(unsigned long lightId, int enable),
    void(__fastcall *setMaxLightsFunc)(unsigned int maxLightsToUse),
    float(__fastcall *bucketSizeFunc)(),
    void(__fastcall *bucketSizeSetFunc)(float bucketSize),
    void(__fastcall *resetCacheFunc)()
);
