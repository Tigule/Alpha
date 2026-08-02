#include <WowConst.h>
#include <MapDefs.h>

#include "WorldClient/CMapObj.h"
#include "WorldClient/World.h"

#include "Base/Base.h"
#include "Os/W32/Debugging.h"
#include "Services/AsyncFileRead.h"
#include "Services/Texture.h"
#include "WorldCommon/WorldMath.h"

#include <Tempest/c3ray.h>
#include <Tempest/tempest_intersect.h>

#include <math.h>

TSCArray<NTempest::CRect, 16>      CMapObj::extViewList;
TSCArray<SPortalExt, 2048>         CMapObj::portalExtList;
unsigned int                       CMapObj::DEFAULT_RLEVEL = 10;
unsigned int                       CMapObj::maxRLevel = CMapObj::DEFAULT_RLEVEL;
unsigned int                       CMapObj::MAX_SOUND_RLEVEL;
NTempest::C3Vector                 CMapObj::localCamPos;
CMapObjDef                        *CMapObj::curMapObjDef;
int                                CMapObj::bIntRender;
unsigned int                       CMapObj::sMinimapTag;
TSHashTable<CMapObj, HASHKEY_NONE> CMapObj::mapObjHash;
HASHKEY_NONE                       CMapObj::nullHashKey;
void(*CMapObj::gRenderCallback)(const unsigned int, const void *, const int);
void        *CMapObj::gRenderUserParam;
unsigned int CMapObj::gRenderCount;

void CMapObj::Initialize() {
  gRenderCount = 0;
  portalExtList.SetCount(2048);
  CMapObjGroup::extGxBufFreeList.SetCount(0);
  CMapObjGroup::intGxBufFreeList.SetCount(0);
}

void CMapObj::Destroy() {
  ClearCache(1);
  CMapObjGroup::Destroy();
}

void CMapObj::ClearCache(int force) {
  CMapObj *mapObj = mapObjHash.Head();
  CMapObj *mapObjnext_node;

  (void)force;
  while (mapObj) {
    mapObjnext_node = mapObjHash.Next(mapObj);
    if (!mapObj->refCount) {
      mapObjHash.Unlink(mapObj);
      CMap::FreeMapObj(mapObj);
    }
    mapObj = mapObjnext_node;
  }
}

CMapObj *CMapObj::Create(const char *fileName) {
  unsigned int mapObjId = SStrHashHT(fileName);
  CMapObj     *mapObj = mapObjHash.Ptr(mapObjId, nullHashKey);
  if (mapObj) {
    ++mapObj->refCount;
    return mapObj;
  }

  mapObj = CMap::AllocMapObj();
  FATALASSERT(mapObj);
  if (!mapObj->Read(fileName)) {
    FATALERROR(("CMapObj::Create(): mapObj->Read(\"%s\") failed", fileName));
  }

  mapObjHash.Insert(mapObj, mapObjId, nullHashKey);
  mapObj->refCount = 1;
  return mapObj;
}

void CMapObj::Delete(CMapObj *mapObj) {
  --mapObj->refCount;
  if (mapObj->refCount <= 0) {
    mapObj->flushTime = 30.0f;
  }
}

CMapObj::CMapObj() {
}

CMapObj::~CMapObj() {
}

void CMapObj::Init() {
  aaBox.b.Set(0.0f, 0.0f, 0.0f);
  aaBox.t.Set(0.0f, 0.0f, 0.0f);
  InitPtrs();
  file = 0;
  data = 0;
  dataBytes = 0;
  refCount = 0;
  flushTime = 0.0f;
  asyncObject = 0;
  bLoaded = 0;
  materialList = 0;
  materialCount = 0;
  nGroupsRead = 0;
  groupPtrList.SetCount(0);
}

void CMapObj::InitPtrs() {
  header = 0;
  textureNameList = 0;
  groupNameList = 0;
  groupInfoList = 0;
  portalVertexList = 0;
  portalList = 0;
  portalRefList = 0;
  lightList = 0;
  doodadSetList = 0;
  doodadNameList = 0;
  doodadDefList = 0;
  fogList = 0;
  convexVolumePlanes = 0;
  textureNameCount = 0;
  groupNameCount = 0;
  groupCount = 0;
  portalVertexCount = 0;
  portalCount = 0;
  portalRefCount = 0;
  lightCount = 0;
  doodadSetCount = 0;
  doodadNameCount = 0;
  doodadDefCount = 0;
  fogCount = 0;
  volumePlaneCount = 0;
}

void CMapObj::Clear() {
  unsigned int i;

  if (asyncObject) {
    AsyncFileReadDestroyObject(asyncObject);
  }
  asyncObject = 0;

  for (i = 0; i < materialCount; ++i) {
    for (unsigned int map = 0; map < 2; ++map) {
      if (materialList[i].hMaps[map]) {
        HandleClose(materialList[i].hMaps[map]);
        materialList[i].hMaps[map] = 0;
      }
    }
  }

  for (i = 0; i < groupPtrList.Count(); ++i) {
    CMap::FreeMapObjGroup(groupPtrList[i]);
    groupPtrList[i] = 0;
  }
  groupPtrList.SetCount(0);

  InitPtrs();
  if (data) {
    SMemFree(data, __FILE__, __LINE__, 0);
  }
  data = 0;
  dataBytes = 0;

  if (file) {
    SFile::Close(file);
  }
  file = 0;
}

bool CMapObj::IsGroupLoaded(unsigned int index) {
  if (!bLoaded) {
    return false;
  }

  ASSERT(groupPtrList[index]);
  return groupPtrList[index]->bLoaded;
}

bool CMapObj::IsGroupLoading(unsigned int index) {
  if (!bLoaded) {
    return false;
  }

  FATALASSERT(groupPtrList[index]);
  return groupPtrList[index]->asyncObject != 0;
}

void CMapObj::GetBounds(NTempest::CAaBox &aaBox) {
  if (bLoaded) {
    aaBox = this->aaBox;
  } else {
    aaBox.b.Set(0.0f, 0.0f, 0.0f);
    aaBox.t.Set(0.0f, 0.0f, 0.0f);
  }
}

bool CMapObj::TestBounds(const NTempest::CAaBox &box) {
  if (!bLoaded) {
    return false;
  }

  return box.b <= aaBox.t && box.t >= aaBox.b;
}

bool CMapObj::TestBounds(const NTempest::C3Vector &point) {
  if (!bLoaded) {
    return false;
  }

  return point >= aaBox.b && point <= aaBox.t;
}

bool CMapObj::TestBounds(const NTempest::C3Vector &v0, const NTempest::C3Vector &v1) {
  return bLoaded && CWorldMath::VectorIntersectAABox2(aaBox, v0, v1);
}

bool CMapObj::TestGroupBounds(const NTempest::CAaBox &box, const unsigned int index) {
  if (!bLoaded || !IsGroupLoaded(index)) {
    return false;
  }

  return box.b <= groupInfoList[index].aaBox.t && box.t >= groupInfoList[index].aaBox.b;
}

bool CMapObj::TestGroupBounds(const NTempest::C3Vector &point, const unsigned int index) {
  if (!bLoaded || !IsGroupLoaded(index)) {
    return false;
  }

  return point >= groupInfoList[index].aaBox.b && point <= groupInfoList[index].aaBox.t;
}

bool CMapObj::TestGroupBounds(
    const NTempest::C3Vector &v0,
    const NTempest::C3Vector &v1,
    unsigned int              index
) {
  return bLoaded && IsGroupLoaded(index) && CWorldMath::VectorIntersectAABox2(groupInfoList[index].aaBox, v0, v1);
}

void CMapObj::GetBounds(NTempest::CAaSphere &aaSphere) {
  if (bLoaded) {
    aaSphere.c = (aaBox.b + aaBox.t) * 0.5f;
    aaSphere.r = (aaBox.t - aaSphere.c).Mag();
  } else {
    aaSphere.c.Set(0.0f, 0.0f, 0.0f);
    aaSphere.r = 0.0f;
  }
}

void CMapObj::GetGroupBounds(NTempest::CAaSphere &aaSphere, unsigned int index) {
  if (bLoaded) {
    SMOGroupInfo *info = &groupInfoList[index];
    aaSphere.c = (info->aaBox.b + info->aaBox.t) * 0.5f;
    aaSphere.r = (info->aaBox.t - aaSphere.c).Mag();
  } else {
    aaSphere.c.Set(0.0f, 0.0f, 0.0f);
    aaSphere.r = 0.0f;
  }
}

void CMapObj::GetGroupBounds(NTempest::CAaBox &aaBox, unsigned int index) {
  if (bLoaded) {
    aaBox = groupInfoList[index].aaBox;
  } else {
    aaBox.b.Set(0.0f, 0.0f, 0.0f);
    aaBox.t.Set(0.0f, 0.0f, 0.0f);
  }
}

unsigned int CMapObj::GetGroupFlags(unsigned int index) {
  if (!bLoaded) {
    return 0;
  }

  SMOGroupInfo *info = &groupInfoList[index];
  FATALASSERT(info);
  return info->flags;
}

CMapObjGroup *CMapObj::GetGroup(unsigned int index, int force) {
  if (!bLoaded) {
    return 0;
  }

  CMapObjGroup *group = groupPtrList[index];
  ASSERT(group);

  if (!group->bLoaded && !force) {
    return 0;
  }

  return group;
}

const SMOGroupInfo *CMapObj::GetGroupInfo(unsigned int index) {
  return bLoaded ? &groupInfoList[index] : 0;
}

char *CMapObj::GetGroupName(unsigned int index) {
  if (!bLoaded || !IsGroupLoaded(index)) {
    return 0;
  }

  CMapObjGroup *group = groupPtrList[index];
  FATALASSERT(group);
  return group->dbgName;
}

bool CMapObj::TestConvexVolume(const NTempest::C3Vector &point) {
  if (!bLoaded) {
    return false;
  }

  for (unsigned int i = 0; i < volumePlaneCount; ++i) {
    if (NTempest::C3Vector::Dot(convexVolumePlanes[i].n, point) +
            convexVolumePlanes[i].d >
        0.0f) {
      return false;
    }
  }
  return true;
}

bool CMapObj::VectorIntersect(
    CMapObjDef               *mapObjDef,
    const NTempest::C3Vector *v0,
    const NTempest::C3Vector *v1,
    unsigned int              queryFlags,
    unsigned int              polyIgnoreFlags,
    unsigned int              groupIgnoreFlags,
    float                    *dist,
    SMOPoly                 **poly
) {
  FATALASSERT(v0);
  FATALASSERT(v1);
  FATALASSERT(*dist >= 0.0f && *dist <= 1.0f);

  if (!CWorldMath::VectorIntersectAABox2(aaBox, *v0, *v1)) {
    return false;
  }

  NTempest::C3Vector wsp0 = *v0 * mapObjDef->mat;
  NTempest::C3Vector wsp1 = *v1 * mapObjDef->mat;
  SMOPoly           *hitPoly = 0;
  bool               hit = false;

  ITERATELIST(CMapBaseObjLink, mapObjDef->groupLinkList, groupLink) {
    CMapObjDefGroup *mapObjDefGroup = static_cast<CMapObjDefGroup *>(groupLink->owner);
    if (!CWorldMath::VectorIntersectAABox2(groupInfoList[mapObjDefGroup->groupNum].aaBox, *v0, *v1)) {
      continue;
    }

    CMapObjGroup *group = GetGroup(mapObjDefGroup->groupNum, 0);
    if (!group || (groupIgnoreFlags & group->flags)) {
      continue;
    }

    CWTriData triData;
    if (group->GetTris(triData, NTempest::C3Segment(*v0, *v1), *dist, mapObjDef, polyIgnoreFlags)) {
      FATALASSERT(triData.GetNumBatches());
      unsigned int polyIndex = triData.GetBatch(0).triIndices[0];
      FATALASSERT(polyIndex < group->polyCount);
      FATALASSERT(group->polyList);
      hitPoly = &group->polyList[polyIndex];
      hit = true;
    }

    if ((queryFlags & 0xF) &&
        (CMap::VectorIntersectDoodadDefLinkList(mapObjDefGroup->doodadDefLinkList, &wsp0, &wsp1, dist, queryFlags) ||
         CMap::VectorIntersectGameObjLinkList(mapObjDefGroup->entityLinkList, &wsp0, &wsp1, dist, queryFlags)))
    {
      hitPoly = 0;
      hit = true;
    }
  }

  if (hit && poly) {
    *poly = hitPoly;
  }
  return hit;
}

bool CMapObj::VectorIntersectPortals(const NTempest::C3Segment &seg, float &maxT, unsigned int *groupIDs) {
  NTempest::C3Vector dir = seg.end - seg.start;
  float              dirMag = dir.Mag();
  float              oodirMag = 1.0f / dirMag;
  NTempest::C3Ray    ray(seg.start, dir * oodirMag);
  float              rayT = dirMag * maxT;
  bool               hit = false;

  for (unsigned int i = 0; i < groupCount; ++i) {
    if (!TestGroupBounds(seg.start, seg.end, i)) {
      continue;
    }

    const CMapObjGroup *group = GetGroup(i, 0);
    if (!group) {
      continue;
    }

    SMOPortalRef *portalRef = &portalRefList[group->portalStart];
    for (unsigned int j = 0; j < group->portalCount; ++j, ++portalRef) {
      const SMOPortal *portal = &portalList[portalRef->portalIndex];
      NTempest::C3Vector point(0.0f);
      float              thisT;
      if (!NTempest::Intersect(ray, portal->plane, &thisT, &point) || thisT < 0.0f || thisT > rayT ||
          !NTempest::Intersect(point, &portalVertexList[portal->startVertex], portal->count, portal->plane.n.MajorAxis()))
      {
        continue;
      }

      hit = true;
      rayT = thisT;
      if (portal->plane.DistSigned(seg.start) < 0.0f) {
        if (portalRef->side > 0) {
          groupIDs[0] = portalRef->groupIndex;
          groupIDs[1] = i;
        } else {
          groupIDs[0] = i;
          groupIDs[1] = portalRef->groupIndex;
        }
      } else {
        if (portalRef->side <= 0) {
          groupIDs[0] = portalRef->groupIndex;
          groupIDs[1] = i;
        } else {
          groupIDs[0] = i;
          groupIDs[1] = portalRef->groupIndex;
        }
      }
    }
  }

  if (hit) {
    maxT = rayT * oodirMag;
  }
  return hit;
}

bool CMapObj::VectorIntersectPortal(
    const NTempest::C3Vector &v0, const NTempest::C3Vector &v1, unsigned int fromGroup, unsigned int &toGroup
) {
  CMapObjGroup *group = GetGroup(fromGroup, 0);
  if (!group) {
    toGroup = 0xFFFF;
    return false;
  }

  NTempest::C3Vector rayOrig = v0;
  NTempest::C3Vector rayDir = v1 - v0;
  float              dist = FLT_MAX;
  SMOPortalRef      *portalRef = &portalRefList[group->portalStart];
  for (unsigned int i = 0; i < group->portalCount; ++i, ++portalRef) {
    const SMOPortal *portal = &portalList[portalRef->portalIndex];
    for (unsigned int j = 1; j < portal->count - 1; ++j) {
      if (CWorldMath::RayIntersectTri(
              rayOrig,
              rayDir,
              portalVertexList[portal->startVertex],
              portalVertexList[portal->startVertex + j],
              portalVertexList[portal->startVertex + j + 1],
              dist
          ) &&
          dist >= 0.0f && dist <= 1.0f)
      {
        toGroup = portalRef->groupIndex;
        return true;
      }
    }
  }

  toGroup = 0xFFFF;
  return false;
}

bool CMapObj::GetTris(
    CWTriData                 &triData,
    const NTempest::CAaBox    &aaBox,
    const CMapObjDef          *mapObjDef,
    unsigned int               queryFlags
) {
  unsigned int ignoreFlags = 0x80;
  if (queryFlags & 0x10) {
    ignoreFlags = 0x84;
  }
  if (queryFlags & 0x20) {
    ignoreFlags |= 0x08;
  }

  bool hit = false;
  for (unsigned int i = 0; i < groupCount; ++i) {
    if (groupInfoList[i].aaBox.b <= aaBox.t && groupInfoList[i].aaBox.t >= aaBox.b) {
      CMapObjGroup *group = GetGroup(i, 0);
      if (group) {
        hit |= group->GetTris(triData, aaBox, mapObjDef, ignoreFlags);
      }
    }
  }
  return hit;
}

bool CMapObj::GetTris(
    CWTriData                  &triData,
    const NTempest::C3Segment  &seg,
    float                      &maxT,
    const CMapObjDef           *mapObjDef,
    unsigned int                queryFlags
) {
  unsigned int ignoreFlags = 0x80;
  if (queryFlags & 0x10) {
    ignoreFlags = 0x84;
  }
  if (queryFlags & 0x20) {
    ignoreFlags |= 0x08;
  }

  bool hit = false;
  for (unsigned int i = 0; i < groupCount; ++i) {
    if (CWorldMath::VectorIntersectAABox2(groupInfoList[i].aaBox, seg)) {
      CMapObjGroup *group = GetGroup(i, 0);
      if (group) {
        hit |= group->GetTris(triData, seg, maxT, mapObjDef, ignoreFlags);
      }
    }
  }
  return hit;
}

bool CMapObj::GetTris(
    CWTriData             &triData,
    const CWFrustum       &frustum,
    const CMapObjDef      *mapObjDef,
    unsigned int           queryFlags
) {
  unsigned int ignoreFlags = 0x80;
  if (queryFlags & 0x10) {
    ignoreFlags = 0x84;
  }
  if (queryFlags & 0x20) {
    ignoreFlags |= 0x08;
  }

  NTempest::CAaBox frustumBox = NTempest::CAaBox::Bounding(frustum.corners, 8);
  bool hit = false;
  for (unsigned int i = 0; i < groupCount; ++i) {
    if (groupInfoList[i].aaBox.b <= frustumBox.t && groupInfoList[i].aaBox.t >= frustumBox.b) {
      CMapObjGroup *group = GetGroup(i, 0);
      if (group) {
        hit |= group->GetTris(triData, frustum, mapObjDef, ignoreFlags);
      }
    }
  }
  return hit;
}

void CMapObj::ReadGroup(unsigned int index) {
  CMapObjGroup *group = groupPtrList[index];
  FATALASSERT(group);
  FATALASSERT(group->data == 0);
  FATALASSERT(group->asyncObject == 0);
  ReadGroup(group, &groupInfoList[index], 0);
}

void CMapObj::WaitLoad() {
  if (version != 14) {
    OsOutputDebugString("CMapObj::WaitLoad(): %s wrong version\n", name);
  }
  FATALASSERT(asyncObject);

  OsOutputDebugString("CMapObj::WaitLoad()\n");
  while (asyncObject) {
    AsyncFileReadWait(asyncObject);
  }
}

void CMapObj::WaitLoadGroup(unsigned int index) {
  CMapObjGroup *group = groupPtrList[index];
  FATALASSERT(group);
  FATALASSERT(group->asyncObject);

  OsOutputDebugString("CMapObj::WaitLoadGroup(%d)\n", index);
  while (group->asyncObject) {
    AsyncFileReadWait(group->asyncObject);
  }
}

bool CMapObj::QueryLightmap(const NTempest::C3Segment &seg, NTempest::CImVector &color, float *t) {
  NTempest::CAaBox gbox;
  CWTriData        triData;
  unsigned short   hitPoly = 0;
  CMapObjGroup    *hitGroup = 0;
  unsigned int     grouplp;
  float            hitT = 1.0f;

  for (grouplp = 0; grouplp < groupCount; ++grouplp) {
    gbox = groupInfoList[grouplp].aaBox;
    if (!CWorldMath::VectorIntersectAABox2(gbox, seg)) {
      continue;
    }

    CMapObjGroup *group = GetGroup(grouplp, 0);
    if (!group || group->flags & 0x48) {
      continue;
    }

    triData.Clear();
    float thisT = hitT;
    if (group->GetTris(triData, seg, thisT, 0, 8)) {
      hitT = thisT;
      FATALASSERT(triData.GetNumBatches());
      hitPoly = triData.GetBatch(0).triIndices[0];
      hitGroup = group;
    }
  }

  if (!hitGroup) {
    return false;
  }

  hitGroup->QueryLightmap(
      NTempest::C3Vector(
          seg.start.x + (seg.end.x - seg.start.x) * hitT, seg.start.y + (seg.end.y - seg.start.y) * hitT,
          seg.start.z + (seg.end.z - seg.start.z) * hitT
      ),
      hitPoly, color
  );
  if (t) {
    *t = hitT;
  }
  return true;
}

bool
CMapObj::QueryLiquidStatus(
    unsigned int ignoreGroupFlags, const NTempest::C3Vector &pos, unsigned int &liquid, float &surface, NTempest::C3Vector &dir
) {
  for (unsigned int grouplp = 0; grouplp < groupCount; ++grouplp) {
    SMOGroupInfo *groupInfo = &groupInfoList[grouplp];
    if ((ignoreGroupFlags & groupInfo->flags) || pos.x <= groupInfo->aaBox.b.x || pos.y <= groupInfo->aaBox.b.y || pos.z <= groupInfo->aaBox.b.z ||
        pos.x >= groupInfo->aaBox.t.x || pos.y >= groupInfo->aaBox.t.y || pos.z >= groupInfo->aaBox.t.z || !IsGroupLoaded(grouplp))
    {
      continue;
    }

    CMapObjGroup *group = GetGroup(grouplp, 0);
    if (group && group->QueryLiquidStatus(pos, liquid, surface, dir)) {
      return 1;
    }
  }

  return 0;
}

bool CMapObj::QueryLiquidFishable(unsigned int ignoreGroupFlags, const NTempest::C3Vector &pos, int &fishable) {
  for (unsigned int grouplp = 0; grouplp < groupCount; ++grouplp) {
    const SMOGroupInfo *groupInfo = GetGroupInfo(grouplp);
    if ((ignoreGroupFlags & groupInfo->flags) || pos.x <= groupInfo->aaBox.b.x || pos.y <= groupInfo->aaBox.b.y ||
        pos.z <= groupInfo->aaBox.b.z || pos.x >= groupInfo->aaBox.t.x || pos.y >= groupInfo->aaBox.t.y ||
        pos.z >= groupInfo->aaBox.t.z || !IsGroupLoaded(grouplp))
    {
      continue;
    }

    CMapObjGroup *group = GetGroup(grouplp, 0);
    if (group && group->QueryLiquidFishable(pos, fishable)) {
      return true;
    }
  }

  return false;
}

unsigned int CMapObj::GetDoodadSet(unsigned int doodadIndex) {
  if (!bLoaded || doodadIndex >= doodadDefCount || !doodadSetCount) {
    return -1;
  }

  for (unsigned int i = 0; i < doodadSetCount; ++i) {
    if (doodadSetList[i].count && doodadIndex >= doodadSetList[i].startIndex && doodadIndex < doodadSetList[i].startIndex + doodadSetList[i].count) {
      return i;
    }
  }
  return -1;
}

static int NearestPow2(float value) {
  return static_cast<int>(ceil(log(value) / log(2.0f)));
}

void CMapObj::QueryMapObjMinimapGroup(
    unsigned int                       groupID,
    unsigned int                       parentID,
    const NTempest::CAaBox            &localBox,
    TSStackArray<CWorld::MinimapQuad> &quads
) {
  CMapObjGroup *group = GetGroup(groupID, 0);
  if (!group || group->minimapTag == sMinimapTag) {
    return;
  }

  group->minimapTag = sMinimapTag;
  if (localBox.b.x > group->aaBox.t.x || localBox.b.y > group->aaBox.t.y || localBox.t.x < group->aaBox.b.x || localBox.t.y < group->aaBox.b.y) {
    return;
  }

  group->QueryMinimap(groupID, localBox, quads);
  for (unsigned int i = 0; i < group->portalCount; ++i) {
    SMOPortalRef *portalRef = &portalRefList[group->portalStart + i];
    unsigned int  toGroupID = portalRef->groupIndex;
    if (toGroupID == 0xFFFF || toGroupID == parentID) {
      continue;
    }

    SMOPortal   *portal = &portalList[portalRef->portalIndex];
    unsigned int clipFlags = ~0u;
    for (unsigned int vertex = 0; vertex < portal->count; ++vertex) {
      const NTempest::C3Vector &point = portalVertexList[portal->startVertex + vertex];
      unsigned int              pointFlags = 0;
      if (point.x < localBox.b.x)
        pointFlags |= 0x01;
      if (point.y < localBox.b.y)
        pointFlags |= 0x02;
      if (point.z < localBox.b.z)
        pointFlags |= 0x04;
      if (point.x > localBox.t.x)
        pointFlags |= 0x08;
      if (point.y > localBox.t.y)
        pointFlags |= 0x10;
      if (point.z > localBox.t.z)
        pointFlags |= 0x20;
      clipFlags &= pointFlags;
    }

    if (!clipFlags) {
      QueryMapObjMinimapGroup(toGroupID, groupID, localBox, quads);
    }
  }
}

bool CMapObj::QueryMapObjMinimap(
    unsigned int groupID, const NTempest::CAaBox &localBox, TSStackArray<CWorld::MinimapQuad> &quads
) {
  ++sMinimapTag;
  if (!bLoaded) {
    return 0;
  }

  NTempest::CAaBox clipLocalBox = localBox;
  CMapObjGroup    *group = GetGroup(groupID, 0);
  if (!group) {
    return 0;
  }

  clipLocalBox.t.z = group->aaBox.t.z;
  QueryMapObjMinimapGroup(groupID, groupID, clipLocalBox, quads);
  return 1;
}

void CMapObjGroup::QueryMinimap(unsigned int groupID, const NTempest::CAaBox &localBox, TSStackArray<CWorld::MinimapQuad> &quads) {
  if (flags & 0x88) {
    return;
  }

  NTempest::C3Vector  center((aaBox.b.x + aaBox.t.x) * 0.5f, (aaBox.b.y + aaBox.t.y) * 0.5f, (aaBox.b.z + aaBox.t.z) * 0.5f);
  NTempest::C3Vector  blockSize((aaBox.t.x - aaBox.b.x) / 128.0f, (aaBox.t.y - aaBox.b.y) / 128.0f, 0.0f);
  NTempest::C2iVector nTex;
  int                 tex = 1 << NearestPow2(blockSize.y * 256.0f);
  nTex.y = tex < 32 ? 32 : (tex >= 256 ? 256 : tex);
  tex = 1 << NearestPow2(blockSize.x * 256.0f);
  nTex.x = tex < 32 ? 32 : (tex >= 256 ? 256 : tex);

  NTempest::C2Vector nQuads(static_cast<float>(ceil(blockSize.x)), static_cast<float>(ceil(blockSize.y)));
  blockSize.x = nTex.x * 0.5f;
  blockSize.y = nTex.y * 0.5f;

  for (int y = 0; y < static_cast<int>(nQuads.y); ++y) {
    for (int x = 0; x < static_cast<int>(nQuads.x); ++x) {
      NTempest::CAaBox quadBox;
      quadBox.b = NTempest::C3Vector(aaBox.b.x + x * blockSize.x, aaBox.b.y + y * blockSize.y, center.z);
      quadBox.t = NTempest::C3Vector(quadBox.b.x + blockSize.x, quadBox.b.y + blockSize.y, center.z);
      if (localBox.b.x <= quadBox.t.x && localBox.b.y <= quadBox.t.y && localBox.t.x >= quadBox.b.x && localBox.t.y >= quadBox.b.y) {
        CWorld::MinimapQuad *quad = quads.New();
        quad->groupNum = groupID;
        quad->quad.x = x;
        quad->quad.y = y;
        quad->aaBox = quadBox;
      }
    }
  }
}
