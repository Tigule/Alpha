#include "IGxuLight.h"

#include <Base/Activity.h>
#include <Gx/CGxDevice.h>
#include <Tempest/cmath.h>
#include <Tempest/cpriorityq.h>
#include <storm.h>

struct CGxuLight : public TSLinkedNode<CGxuLight> {
  CGxuLight() : m_lockCount(0) {
  }

  ~CGxuLight() {
  }

  float          Fitness(NTempest::C3Vector &pos, float linearAttenuation, float quadraticAttenuation);
  CGxuLightLink *AllocListLink();
  void           ClearListLinks();

  CGxLight                          m_light;
  unsigned int                      m_hwLight;
  unsigned long                     m_selectionCount;
  int                               m_lockCount;
  TSExplicitList<CGxuLightLink, 12> m_links;

  static TSList<CGxuLight, TSGetLink<CGxuLight> > s_lights;
  static TSList<CGxuLight, TSGetLink<CGxuLight> > s_lightsFreeList;
  static TSExplicitList<CGxuLightLink, 12>        s_linksFreeList;
};

static void IGxuLightShutdown();
static unsigned long IGxuLightCreate();
static void IGxuLightDestroy(unsigned long lightId);
static CGxLight *IGxuLightLock(unsigned long lightId);
static void IGxuLightUnlock(unsigned long lightId);
static void IGxuLightSelect(NTempest::C3Vector worldPos, const NTempest::C3Vector &cameraWorldPos, unsigned int maxLightsToUse);

static void IGxuLightInitialize();
static void IGxuLightEnableSet(unsigned long lightId, int enable);
static int IGxuLightEnable(unsigned long lightId);
static void IGxuLightSetMaxLights(unsigned int maxLightsToUse);
static float IGxuLightBucketSize();
static void IGxuLightBucketSizeSet(float bucketSize);
static void IGxuLightResetCache();

static const float                                  GxuLight_BucketSizeDefault = 1024.0f;
static float                                        s_bucketSize = GxuLight_BucketSizeDefault;
static float                                        s_halfBucket = GxuLight_BucketSizeDefault * 0.5f;
TSList<CGxuLight, TSGetLink<CGxuLight> >            CGxuLight::s_lights;
TSList<CGxuLight, TSGetLink<CGxuLight> >            CGxuLight::s_lightsFreeList;
TSExplicitList<CGxuLightLink, 12>                   CGxuLight::s_linksFreeList;
static unsigned long                                s_lastLightsHash;
static CLightList                                   s_dirLightList;
static unsigned int                                 s_updateDirLights;
static unsigned int                                 s_dirLightSet;
static unsigned char                                s_forceSettingLights = 1;
static unsigned int                                 s_maxLightsToUse = 8;
static unsigned long                                s_selectionCount;
TSHashTableReuse<CLightList, HASHKEY_DWORD, 1>      CLightList::s_lightHashTable;
static NTempest::CPriorityQ<CGxuLight *, CGxuLight> s_lightPriQ;
static TSGrowableArray<CGxuLight *>                 s_lightsToUse;
static NTempest::C3Vector                           s_cameraWorldPos;

inline float CGxuLight::Fitness(NTempest::C3Vector &pos, float linearAttenuation, float quadraticAttenuation) {
  NTempest::C3Vector l = pos - m_light.m_dir;
  float              distance = NTempest::CMath::sqrt_(l.SquaredMag());

  return m_light.m_dirIntensity / (distance * distance * quadraticAttenuation + distance * linearAttenuation + 1.0f) + m_light.m_ambIntensity;
}

inline CGxuLightLink *CGxuLight::AllocListLink() {
  CGxuLightLink *link = s_linksFreeList.Head();

  if (!link) {
    link = s_linksFreeList.NewNode(LIST_TAIL, 0, 0);
  }
  link->m_lightLink.Unlink();
  m_links.LinkNode(link, LIST_TAIL, 0);
  return link;
}

inline void CGxuLight::ClearListLinks() {
  CGxuLightLink *link;
  CGxuLightLink *next;

  link = m_links.Head();
  while (link) {
    next = m_links.Next(link);
    link->m_lightLink.Unlink();
    link->m_listLink.Unlink();
    if (m_light.m_isOmni && link->m_list->m_links.Head()) {
      CLightList::s_lightHashTable.Delete(link->m_list);
    }
    s_linksFreeList.LinkNode(link, LIST_TAIL, 0);
    link = next;
  }
}

void(*GxuLightInitialize)() = IGxuLightInitialize;
void(*GxuLightShutdown)() = IGxuLightShutdown;
unsigned long(*GxuLightCreate)() = IGxuLightCreate;
void(*GxuLightDestroy)(unsigned long) = IGxuLightDestroy;
CGxLight *(*GxuLightLock)(unsigned long) = IGxuLightLock;
void(*GxuLightUnlock)(unsigned long) = IGxuLightUnlock;
void(*GxuLightSelect)(NTempest::C3Vector, const NTempest::C3Vector &, unsigned int) = IGxuLightSelect;
int(*GxuLightEnable)(unsigned long) = IGxuLightEnable;
void(*GxuLightEnableSet)(unsigned long, int) = IGxuLightEnableSet;
void(*GxuLightSetMaxLights)(unsigned int) = IGxuLightSetMaxLights;
float(*GxuLightBucketSize)() = IGxuLightBucketSize;
void(*GxuLightBucketSizeSet)(float) = IGxuLightBucketSizeSet;
void(*GxuLightResetCache)() = IGxuLightResetCache;

void GxuLightFuncsSet(
    void(*initializeFunc)(),
    void(*shutDownFunc)(),
    unsigned long(*createFunc)(),
    void(*destroyFunc)(unsigned long),
    CGxLight *(*lockFunc)(unsigned long),
    void(*unlockFunc)(unsigned long),
    void(*selectFunc)(NTempest::C3Vector, const NTempest::C3Vector &, unsigned int),
    int(*enableFunc)(unsigned long),
    void(*enableSetFunc)(unsigned long, int),
    void(*setMaxLightsFunc)(unsigned int),
    float(*bucketSizeFunc)(),
    void(*bucketSizeSetFunc)(float),
    void(*resetCacheFunc)()
) {
  if (initializeFunc) {
    GxuLightInitialize = initializeFunc;
  }
  if (shutDownFunc) {
    GxuLightShutdown = shutDownFunc;
  }
  if (createFunc) {
    GxuLightCreate = createFunc;
  }
  if (destroyFunc) {
    GxuLightDestroy = destroyFunc;
  }
  if (lockFunc) {
    GxuLightLock = lockFunc;
  }
  if (unlockFunc) {
    GxuLightUnlock = unlockFunc;
  }
  if (selectFunc) {
    GxuLightSelect = selectFunc;
  }
  if (enableFunc) {
    GxuLightEnable = enableFunc;
  }
  if (enableSetFunc) {
    GxuLightEnableSet = enableSetFunc;
  }
  if (setMaxLightsFunc) {
    GxuLightSetMaxLights = setMaxLightsFunc;
  }
  if (bucketSizeFunc) {
    GxuLightBucketSize = bucketSizeFunc;
  }
  if (bucketSizeSetFunc) {
    GxuLightBucketSizeSet = bucketSizeSetFunc;
  }
  if (resetCacheFunc) {
    GxuLightResetCache = resetCacheFunc;
  }
}

static void IGxuLightInitialize() {
}

static void IGxuLightShutdown() {
  CGxuLight     *light;
  CGxuLightLink *link;

  ASSERT(!CGxuLight::s_lights.Head());
  CLightList::s_lightHashTable.Destroy();

  while ((light = CGxuLight::s_lightsFreeList.Head()) != 0) {
    CGxuLight::s_lightsFreeList.DeleteNode(light);
  }

  while ((link = CGxuLight::s_linksFreeList.Head()) != 0) {
    CGxuLight::s_linksFreeList.DeleteNode(link);
  }
}

static unsigned long IGxuLightCreate() {
  CGxuLight *light = CGxuLight::s_lightsFreeList.Head();

  if (!light) {
    light = CGxuLight::s_lights.NewNode(LIST_TAIL, 0, 0);
    ASSERT(light);
  } else {
    CGxuLight::s_lightsFreeList.UnlinkNode(light);
    CGxuLight::s_lights.LinkNode(light, LIST_TAIL, 0);
    new (&light->m_light) CGxLight;
  }

  return reinterpret_cast<unsigned long>(light);
}

static void IGxuLightDestroy(unsigned long lightId) {
  CGxuLight *light = reinterpret_cast<CGxuLight *>(lightId);

  ASSERT(light);
  s_forceSettingLights = 1;
  light->Unlink();
  light->ClearListLinks();
  CGxuLight::s_lightsFreeList.LinkNode(light, LIST_TAIL, 0);
}

static CGxLight *IGxuLightLock(unsigned long lightId) {
  CGxuLight *light = reinterpret_cast<CGxuLight *>(lightId);

  ASSERT(light);
  ++light->m_lockCount;
  return &light->m_light;
}

static void IGxuLightUnlock(unsigned long lightId) {
  CGxuLight         *light = reinterpret_cast<CGxuLight *>(lightId);
  NTempest::C3Vector pos;
  NTempest::C3Vector max;
  NTempest::C3Vector min;
  HASHKEY_DWORD      hashKey;
  float              fitness;
  int                y;
  float              radius;
  int                x;

  ASSERT(light);
  ASSERT(light->m_lockCount != 0);
  s_forceSettingLights = 1;
  --light->m_lockCount;
  light->ClearListLinks();

  if (!light->m_light.m_enabled) {
    return;
  }

  if (!light->m_light.m_isOmni) {
    CGxuLightLink *link = light->AllocListLink();

    s_dirLightList.m_links.LinkNode(link, LIST_TAIL, 0);
    link->m_light = light;
    link->m_list = &s_dirLightList;
    return;
  }

  radius = (light->m_light.m_dirIntensity + light->m_light.m_ambIntensity) * 20.0f;
  if (light->m_light.m_linearAttenuation != 0.0f) {
    radius /= light->m_light.m_linearAttenuation;
  }

  min = NTempest::C3Vector(light->m_light.m_dir.x - radius, light->m_light.m_dir.y - radius, light->m_light.m_dir.z - radius);
  max = NTempest::C3Vector(light->m_light.m_dir.x + radius, light->m_light.m_dir.y + radius, light->m_light.m_dir.z + radius);

  for (y = static_cast<int>(min.y / s_bucketSize - OneHalfOffset); y <= static_cast<int>(max.y / s_bucketSize - OneHalfOffset); ++y) {
    for (x = static_cast<int>(min.x / s_bucketSize - OneHalfOffset); x <= static_cast<int>(max.x / s_bucketSize - OneHalfOffset); ++x) {
      hashKey = HASHKEY_DWORD((y << 16) | static_cast<unsigned short>(x));
      unsigned int   hash = hashKey.GetDword() % 0x1FFF;
      CLightList    *list = CLightList::s_lightHashTable.Ptr(hash, hashKey);
      CGxuLightLink *link;
      CGxuLightLink *existing;

      if (!list) {
        list = CLightList::s_lightHashTable.New(hash, hashKey, 0, 0);
      }

      pos.x = x * s_bucketSize + s_halfBucket;
      pos.y = y * s_bucketSize + s_halfBucket;
      pos.z = min.z;
      fitness = light->Fitness(pos, light->m_light.m_linearAttenuation, light->m_light.m_quadraticAttenuation);

      link = light->AllocListLink();
      link->m_fitness = fitness;
      link->m_light = light;
      link->m_list = list;

      existing = list->m_links.Head();
      while (existing && fitness < existing->m_fitness) {
        existing = list->m_links.Next(existing);
      }

      if (existing) {
        list->m_links.LinkNode(link, LIST_LINK_BEFORE, existing);
      } else {
        list->m_links.LinkNode(link, LIST_TAIL, 0);
      }
    }
  }
}

static void IGxuLightSelect(NTempest::C3Vector worldPos, const NTempest::C3Vector &cameraWorldPos, unsigned int maxLightsToUse) {
  int            y;
  int            x;
  HASHKEY_DWORD  hashKey;
  unsigned long  hash;
  CLightList    *list;
  CGxuLightLink *link;
  unsigned int   whichLight;

  if (!CGxuLight::s_lights.Head()) {
    return;
  }

  ActivityBegin(ACTIVITY_LIGHTING);

  y = static_cast<int>(worldPos.y / s_bucketSize - OneHalfOffset);
  x = static_cast<int>(worldPos.x / s_bucketSize - OneHalfOffset);
  hashKey = HASHKEY_DWORD((y << 16) | static_cast<unsigned short>(x));
  hash = hashKey.GetDword() % 0x1FFF;
  list = CLightList::s_lightHashTable.Ptr(hash, hashKey);

  ++s_selectionCount;
  whichLight = 0;

  link = s_dirLightList.m_links.Head();
  while (link) {
    GxLightSet(whichLight, link->m_light->m_light, cameraWorldPos);
    link->m_light->m_selectionCount = s_selectionCount;
    link->m_light->m_hwLight = whichLight;
    link = s_dirLightList.m_links.Next(link);
    ++whichLight;
  }

  if (list) {
    link = list->m_links.Head();
    while (link && whichLight < 8) {
      GxLightSet(whichLight, link->m_light->m_light, cameraWorldPos);
      link->m_light->m_selectionCount = s_selectionCount;
      link->m_light->m_hwLight = whichLight;
      link = list->m_links.Next(link);
      ++whichLight;
    }
  }

  while (whichLight < 8) {
    GxLightEnable(whichLight, FALSE);
    ++whichLight;
  }

  s_forceSettingLights = 0;
  s_lastLightsHash = hash;
  ActivityEnd(ACTIVITY_LIGHTING);
}

static void IGxuLightEnableSet(unsigned long lightId, int enable) {
  unsigned long light = lightId;
  CGxLight     *gxLight;

  ASSERT(light);
  gxLight = GxuLightLock(light);
  gxLight->m_enabled = enable;
  GxuLightUnlock(light);
}

static int IGxuLightEnable(unsigned long lightId) {
  unsigned long light = lightId;
  int           enable;

  ASSERT(light);
  enable = GxuLightLock(light)->m_enabled;
  GxuLightUnlock(light);
  return enable;
}

static void IGxuLightSetMaxLights(unsigned int maxLightsToUse) {
  s_maxLightsToUse = maxLightsToUse;
  if (maxLightsToUse >= 8) {
    s_maxLightsToUse = 8;
  }
}

static float IGxuLightBucketSize() {
  return s_bucketSize;
}

static void IGxuLightBucketSizeSet(float bucketSize) {
  ASSERT(bucketSize > 0.0f);
  s_bucketSize = bucketSize;
  s_halfBucket = bucketSize * 0.5f;
  GxuLightResetCache();
}

static void IGxuLightResetCache() {
  s_forceSettingLights = 1;
}
