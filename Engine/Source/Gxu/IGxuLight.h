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
  LISTDECLEX(CGxuLightLink, m_listLink, m_links);

  CLightList() {
  }

  static LISTDECLEX(CGxuLightLink, m_listLink, s_dirLightList);
  static TSHashTableReuse<CLightList, HASHKEY_DWORD, 1> s_lightHashTable;
};

extern void (*GxuLightInitialize)();
extern void (*GxuLightShutdown)();
extern DWORD_PTR (*GxuLightCreate)();
extern void (*GxuLightDestroy)(DWORD_PTR lightId);
extern CGxLight *(*GxuLightLock)(DWORD_PTR lightId);
extern void (*GxuLightUnlock)(DWORD_PTR lightId);
extern void (*GxuLightSelect)(NTempest::C3Vector worldPos, const NTempest::C3Vector &cameraWorldPos, UINT maxLightsToUse);
extern int (*GxuLightEnable)(DWORD_PTR lightId);
extern void (*GxuLightEnableSet)(DWORD_PTR lightId, int enable);
extern void (*GxuLightSetMaxLights)(UINT maxLightsToUse);
extern float (*GxuLightBucketSize)();
extern void (*GxuLightBucketSizeSet)(float bucketSize);
extern void (*GxuLightResetCache)();

void GxuLightFuncsSet(
    void (*initializeFunc)(),
    void (*shutDownFunc)(),
    DWORD_PTR (*createFunc)(),
    void (*destroyFunc)(DWORD_PTR lightId),
    CGxLight *(*lockFunc)(DWORD_PTR lightId),
    void (*unlockFunc)(DWORD_PTR lightId),
    void (*selectFunc)(NTempest::C3Vector worldPos, const NTempest::C3Vector &cameraWorldPos, UINT maxLightsToUse),
    int (*enableFunc)(DWORD_PTR lightId),
    void (*enableSetFunc)(DWORD_PTR lightId, int enable),
    void (*setMaxLightsFunc)(UINT maxLightsToUse),
    float (*bucketSizeFunc)(),
    void (*bucketSizeSetFunc)(float bucketSize),
    void (*resetCacheFunc)()
);
