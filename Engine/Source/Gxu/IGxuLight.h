#pragma once

#include <Tempest/c3vector.h>
#include <storm.h>
#include <stpl.h>

class CGxLight;
struct CGxuLight;
struct CLightList;

class CGxuLightLink {
 public:
  CGxuLight  *m_light;
  CLightList *m_list;
  float       m_fitness;
  LINKDECLEX(CGxuLightLink, m_lightLink);
  LINKDECLEX(CGxuLightLink, m_listLink);
};

struct CLightList : public TSHashObject<CLightList, HASHKEY_DWORD> {
  CLightList() {
  }

  LISTDECLEX(CGxuLightLink, m_listLink, m_links);

  static TSHashTableReuse<CLightList, HASHKEY_DWORD, 1> s_lightHashTable;
};

extern void (*GxuLightInitialize)();
extern void (*GxuLightShutdown)();
extern DWORD (*GxuLightCreate)();
extern void (*GxuLightDestroy)(DWORD lightId);
extern CGxLight *(*GxuLightLock)(DWORD lightId);
extern void (*GxuLightUnlock)(DWORD lightId);
extern void (*GxuLightSelect)(NTempest::C3Vector worldPos, const NTempest::C3Vector &cameraWorldPos, UINT maxLightsToUse);
extern int (*GxuLightEnable)(DWORD lightId);
extern void (*GxuLightEnableSet)(DWORD lightId, int enable);
extern void (*GxuLightSetMaxLights)(UINT maxLightsToUse);
extern float (*GxuLightBucketSize)();
extern void (*GxuLightBucketSizeSet)(float bucketSize);
extern void (*GxuLightResetCache)();

void GxuLightFuncsSet(
    void (*initializeFunc)(),
    void (*shutDownFunc)(),
    DWORD (*createFunc)(),
    void (*destroyFunc)(DWORD lightId),
    CGxLight *(*lockFunc)(DWORD lightId),
    void (*unlockFunc)(DWORD lightId),
    void (*selectFunc)(NTempest::C3Vector worldPos, const NTempest::C3Vector &cameraWorldPos, UINT maxLightsToUse),
    int (*enableFunc)(DWORD lightId),
    void (*enableSetFunc)(DWORD lightId, int enable),
    void (*setMaxLightsFunc)(UINT maxLightsToUse),
    float (*bucketSizeFunc)(),
    void (*bucketSizeSetFunc)(float bucketSize),
    void (*resetCacheFunc)()
);
