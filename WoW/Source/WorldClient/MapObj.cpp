#include "Base/Base.h"
#include "Gx/Gx.h"
#include "Services/ParticleSystem2.h"
#include <WowConst.h>
#include "AaBsp.h"
#include <MapDefs.h>

#include "WorldClient/World.h"
#include "WorldClient/CMapObj.h"
#include "WorldClient/WorldParam.h"
#include "WorldClient/DetailDoodad.h"
#include "WorldClient/CSimpleDoodad.h"
#include "DayNight.h"

#include "Os/W32/Debugging.h"
#include "Services/AsyncFileRead.h"
#include "Services/Texture.h"
#include "WorldCommon/WorldMath.h"

#include <Tempest/c3ray.h>
#include <Tempest/tempest_intersect.h>

#include <math.h>

UINT                               CMapObj::MAX_SOUND_RLEVEL;
CMapObjDef                        *CMapObj::curMapObjDef;
UINT                               CMapObj::sMinimapTag;
TSHashTable<CMapObj, HASHKEY_NONE> CMapObj::mapObjHash;
HASHKEY_NONE                       CMapObj::nullHashKey;
NTempest::C3Vector                 CMapObj::localCamPos;
UINT                               CMapObj::gRenderCount;

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
  for (CMapObj *mapObj = mapObjHash.Head(), *mapObjnext_node; (int)mapObj > 0 ? (mapObjnext_node = mapObjHash.Next(mapObj), 1) : 0;
       mapObj = mapObjnext_node)
  {
    if (!mapObj->refCount) {
      mapObjHash.Unlink(mapObj);
      CMap::FreeMapObj(mapObj);
    }
  }
}

CMapObj *CMapObj::Create(LPCSTR fileName) {
  UINT     mapObjId = SStrHash(fileName, 0, 0);
  CMapObj *mapObj = mapObjHash.Ptr(mapObjId, nullHashKey);
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
  file = 0;
  refCount = 0;
  data = 0;
  dataBytes = 0;
  asyncObject = 0;
  flushTime = 0.0f;
  bLoaded = 0;
  nGroupsRead = 0;
  materialList = 0;
  materialCount = 0;
  groupPtrList.SetCount(0);
  InitPtrs();
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
  UINT i;

  if (asyncObject) {
    AsyncFileReadDestroyObject(asyncObject);
  }
  asyncObject = 0;

  SMOMaterial *material = materialList;
  for (i = 0; i < materialCount; ++i, ++material) {
    for (UINT map = 0; map < 2; ++map) {
      if (material->hMaps[map]) {
        HandleClose(material->hMaps[map]);
        material->hMaps[map] = 0;
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

bool CMapObj::VectorIntersect(
    CMapObjDef               *mapObjDef,
    const NTempest::C3Vector *v0,
    const NTempest::C3Vector *v1,
    UINT                      queryFlags,
    UINT                      polyIgnoreFlags,
    UINT                      groupIgnoreFlags,
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
      hitPoly = group->GetPoly(triData.GetBatch(0).triIndices[0]);
      hit = true;
    }

    if ((queryFlags & 0xF) && (CMap::VectorIntersectDoodadDefLinkList(mapObjDefGroup->doodadDefLinkList, &wsp0, &wsp1, dist, queryFlags) ||
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

bool CMapObj::GetTris(CWTriData &tris, const NTempest::CAaBox &aaBox, const CMapObjDef *mapObjDef, UINT queryFlags) {
  bool hit = false;
  WORD ignoreFlags = 0x80;
  if (queryFlags & 0x10) {
    ignoreFlags = 0x84;
  }
  if (queryFlags & 0x20) {
    ignoreFlags |= 0x08;
  }

  SMOGroupInfo *groupInfo = groupInfoList;
  for (UINT i = 0; i < groupCount; ++i, ++groupInfo) {
    if (aaBox.Intersects(groupInfo->aaBox)) {
      CMapObjGroup *group = GetGroup(i, 0);
      if (group) {
        hit |= group->GetTris(tris, aaBox, mapObjDef, ignoreFlags);
      }
    }
  }
  return hit;
}

bool CMapObj::GetTris(CWTriData &tris, const NTempest::C3Segment &seg, float &maxT, const CMapObjDef *mapObjDef, UINT queryFlags) {
  bool hit = false;
  WORD ignoreFlags = 0x80;
  if (queryFlags & 0x10) {
    ignoreFlags = 0x84;
  }
  if (queryFlags & 0x20) {
    ignoreFlags |= 0x08;
  }

  SMOGroupInfo *groupInfo = groupInfoList;
  for (UINT i = 0; i < groupCount; ++i, ++groupInfo) {
    if (CWorldMath::VectorIntersectAABox2(groupInfo->aaBox, seg.start, seg.end)) {
      CMapObjGroup *group = GetGroup(i, 0);
      if (group) {
        hit |= group->GetTris(tris, seg, maxT, mapObjDef, ignoreFlags);
      }
    }
  }
  return hit;
}

bool CMapObj::GetTris(CWTriData &tris, const CWFrustum &frustum, const CMapObjDef *mapObjDef, UINT queryFlags) {
  bool hit = false;
  WORD ignoreFlags = 0x80;
  if (queryFlags & 0x10) {
    ignoreFlags = 0x84;
  }
  if (queryFlags & 0x20) {
    ignoreFlags |= 0x08;
  }

  NTempest::CAaBox frustumBox = NTempest::CAaBox::Bounding(frustum.corners, 8);
  SMOGroupInfo *groupInfo = groupInfoList;
  for (UINT i = 0; i < groupCount; ++i, ++groupInfo) {
    if (frustumBox.Intersects(groupInfo->aaBox)) {
      CMapObjGroup *group = GetGroup(i, 0);
      if (group) {
        hit |= group->GetTris(tris, frustum, mapObjDef, ignoreFlags);
      }
    }
  }
  return hit;
}

bool CMapObj::VectorIntersectPortals(const NTempest::C3Segment &seg, float &maxT, UINT groupIDs[]) {
  NTempest::C3Vector dir = seg.Direction();
  float              dirMag = NTempest::CMath::sqrt_(dir.SquaredMag());
  float              oodirMag = 1.0f / dirMag;
  NTempest::C3Ray    ray(seg.start, dir * oodirMag);
  float              rayT = dirMag * maxT;
  bool               hit = false;

  for (UINT i = 0; i < groupCount; ++i) {
    if (!TestGroupBounds(seg.start, seg.end, i)) {
      continue;
    }

    const CMapObjGroup *group = GetGroup(i, 0);
    if (!group) {
      continue;
    }

    SMOPortalRef *portalRef = &portalRefList[group->portalStart];
    for (UINT j = 0; j < group->portalCount; ++j, ++portalRef) {
      const SMOPortal   *portal = &portalList[portalRef->portalIndex];
      NTempest::C3Vector point(0.0f);
      float              thisT;
      if (NTempest::Intersect(ray, portal->plane, &thisT, &point) && thisT >= 0.0f && thisT <= rayT &&
          NTempest::Intersect(point, &portalVertexList[portal->startVertex], portal->count, portal->plane.n.MajorAxis()))
      {
        hit = true;
        rayT = thisT;
        if (portal->plane.DistSigned(seg.start) >= 0.0f) {
          if (portalRef->side <= 0) {
            groupIDs[0] = portalRef->groupIndex;
            groupIDs[1] = i;
          } else {
            groupIDs[0] = i;
            groupIDs[1] = portalRef->groupIndex;
          }
        } else {
          if (portalRef->side > 0) {
            groupIDs[0] = portalRef->groupIndex;
            groupIDs[1] = i;
          } else {
            groupIDs[0] = i;
            groupIDs[1] = portalRef->groupIndex;
          }
        }
      }
    }
  }

  if (hit) {
    maxT = rayT * oodirMag;
  }
  return hit;
}

bool CMapObj::VectorIntersectPortal(const NTempest::C3Vector &v0, const NTempest::C3Vector &v1, UINT fromGroup, UINT &toGroup) {
  CMapObjGroup *group = GetGroup(fromGroup, 0);
  if (group) {
    NTempest::C3Vector rayOrig = v0;
    NTempest::C3Vector rayDir = v1 - v0;
    float              dist = FLT_MAX;
    SMOPortalRef      *portalRef = &portalRefList[group->portalStart];
    for (UINT i = 0; i < group->portalCount; ++i, ++portalRef) {
      const SMOPortal *portal = &portalList[portalRef->portalIndex];
      for (WORD j = 1; j < portal->count - 1; ++j) {
        if (CWorldMath::RayIntersectTri(
                rayOrig, rayDir, portalVertexList[portal->startVertex], portalVertexList[portal->startVertex + j],
                portalVertexList[portal->startVertex + j + 1], dist
            ) &&
            !(dist < 0.0f) && !(dist > 1.0f))
        {
          toGroup = portalRef->groupIndex;
          return true;
        }
      }
    }
  }

  toGroup = 0xFFFF;
  return false;
}

bool CMapObj::IsGroupLoaded(UINT index) {
  if (!bLoaded) {
    return false;
  }

  ASSERT(groupPtrList[index]);
  return groupPtrList[index]->bLoaded;
}

bool CMapObj::IsGroupLoading(UINT index) {
  if (!bLoaded) {
    return false;
  }

  FATALASSERT(groupPtrList[index]);
  return groupPtrList[index]->asyncObject ? true : false;
}

void CMapObj::GetBounds(NTempest::CAaSphere &aaSphere) {
  if (!bLoaded) {
    aaSphere.c = NTempest::C3Vector(0.0f);
    aaSphere.r = 0.0f;
  } else {
    aaSphere.c = aaBox.Center();
    aaSphere.r = (aaBox.t - aaSphere.c).Mag();
  }
}

void CMapObj::GetBounds(NTempest::CAaBox &aaBox) {
  if (!bLoaded) {
    aaBox.b = NTempest::C3Vector(0.0f);
    aaBox.t = NTempest::C3Vector(0.0f);
  } else {
    aaBox = this->aaBox;
  }
}

void CMapObj::GetGroupBounds(NTempest::CAaSphere &aaSphere, UINT index) {
  if (!bLoaded) {
    aaSphere.c = NTempest::C3Vector(0.0f);
    aaSphere.r = 0.0f;
  } else {
    SMOGroupInfo *info = &groupInfoList[index];
    aaSphere.c = info->aaBox.Center();
    aaSphere.r = (info->aaBox.t - aaSphere.c).Mag();
  }
}

void CMapObj::GetGroupBounds(NTempest::CAaBox &aaBox, UINT index) {
  if (!bLoaded) {
    aaBox.b = NTempest::C3Vector(0.0f);
    aaBox.t = NTempest::C3Vector(0.0f);
  } else {
    aaBox = groupInfoList[index].aaBox;
  }
}

UINT CMapObj::GetGroupFlags(UINT index) {
  if (!bLoaded) {
    return 0;
  }

  SMOGroupInfo *info = &groupInfoList[index];
  FATALASSERT(info);
  return info->flags;
}

bool CMapObj::TestBounds(const NTempest::CAaBox &box) {
  if (!bLoaded) {
    return false;
  }

  return aaBox.Intersects(box) != 0;
}

bool CMapObj::TestBounds(const NTempest::C3Vector &point) {
  if (!bLoaded) {
    return false;
  }

  return aaBox.Contains(point) != 0;
}

bool CMapObj::TestBounds(const NTempest::C3Vector &v0, const NTempest::C3Vector &v1) {
  if (!bLoaded) {
    return false;
  }

  return CWorldMath::VectorIntersectAABox2(aaBox, v0, v1) != 0;
}

bool CMapObj::TestGroupBounds(const NTempest::C3Vector &v0, const NTempest::C3Vector &v1, UINT index) {
  if (bLoaded && IsGroupLoaded(index)) {
    SMOGroupInfo *info = &groupInfoList[index];
    return CWorldMath::VectorIntersectAABox2(info->aaBox, v0, v1) != 0;
  }

  return 0;
}

bool CMapObj::TestGroupBounds(const NTempest::CAaBox &box, const UINT index) {
  if (!bLoaded || !IsGroupLoaded(index)) {
    return false;
  }

  return groupInfoList[index].aaBox.Intersects(box) != 0;
}

bool CMapObj::TestGroupBounds(const NTempest::C3Vector &point, const UINT index) {
  if (!bLoaded || !IsGroupLoaded(index)) {
    return false;
  }

  return groupInfoList[index].aaBox.Contains(point) != 0;
}

bool CMapObj::TestConvexVolume(const NTempest::C3Vector &point) {
  if (!bLoaded) {
    return false;
  }

  for (UINT i = 0; i < volumePlaneCount; ++i) {
    if (convexVolumePlanes[i].DistSigned(point) > 0.0f) {
      return false;
    }
  }
  return true;
}

CMapObjGroup *CMapObj::GetGroup(UINT index, int force) {
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

void CMapObj::ReadGroup(UINT index) {
  CMapObjGroup *group = groupPtrList[index];
  FATALASSERT(group);
  FATALASSERT(group->data == 0);
  FATALASSERT(group->asyncObject == 0);
  ReadGroup(groupPtrList[index], &groupInfoList[index], 0);
}

void CMapObj::WaitLoad() {
  if (fileHeader.version != 14) {
    OsOutputDebugString("CMapObj::WaitLoad(): %s wrong version\n", name);
    FATALASSERT(asyncObject);
  } else {
    FATALASSERT(asyncObject);
  }

  OsOutputDebugString("CMapObj::WaitLoad()\n");
  while (asyncObject) {
    AsyncFileReadWait(asyncObject);
  }
}

void CMapObj::WaitLoadGroup(UINT index) {
  CMapObjGroup *group = groupPtrList[index];
  FATALASSERT(group);
  FATALASSERT(group->asyncObject);

  OsOutputDebugString("CMapObj::WaitLoadGroup(%d)\n", index);
  while (group->asyncObject) {
    AsyncFileReadWait(group->asyncObject);
  }
}

char *CMapObj::GetGroupName(UINT index) {
  if (!bLoaded || !IsGroupLoaded(index)) {
    return 0;
  }

  CMapObjGroup *group = groupPtrList[index];
  FATALASSERT(group);
  return group->dbgName;
}

const SMOGroupInfo *CMapObj::GetGroupInfo(UINT index) {
  if (!bLoaded) {
    return 0;
  }
  return &groupInfoList[index];
}

bool CMapObj::QueryLightmap(const NTempest::C3Segment &seg, NTempest::CImVector &color, float *t) {
  float            hitT = 1.0f;
  WORD             hitPoly = 0;
  CMapObjGroup    *hitGroup = 0;
  NTempest::CAaBox gbox;
  UINT             grouplp;

  for (grouplp = 0; grouplp < groupCount; ++grouplp) {
    GetGroupBounds(gbox, grouplp);
    if (!CWorldMath::VectorIntersectAABox2(gbox, seg)) {
      continue;
    }

    CMapObjGroup *group = GetGroup(grouplp, 0);
    if (!group || group->flags & 0x48) {
      continue;
    }

    CWTriData triData;
    float     thisT = hitT;
    if (group->GetTris(triData, seg, thisT, 0, 8)) {
      hitT = thisT;
      hitPoly = triData.GetBatch(0).triIndices[0];
      hitGroup = group;
    }
  }

  if (hitGroup) {
    hitGroup->QueryLightmap(seg.Point(hitT), hitPoly, color);
    if (t) {
      *t = hitT;
    }
    return true;
  }

  return false;
}

bool CMapObj::QueryLiquidStatus(UINT ignoreGroupFlags, const NTempest::C3Vector &pos, UINT &liquid, float &surface, NTempest::C3Vector &dir) {
  for (UINT grouplp = 0; grouplp < groupCount; ++grouplp) {
    const SMOGroupInfo *groupInfo = GetGroupInfo(grouplp);
    if ((ignoreGroupFlags & groupInfo->flags) || !(pos.x > groupInfo->aaBox.b.x) || !(pos.y > groupInfo->aaBox.b.y) ||
        !(pos.z > groupInfo->aaBox.b.z) || !(pos.x < groupInfo->aaBox.t.x) || !(pos.y < groupInfo->aaBox.t.y) || !(pos.z < groupInfo->aaBox.t.z) ||
        !IsGroupLoaded(grouplp))
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

bool CMapObj::QueryLiquidFishable(UINT ignoreGroupFlags, const NTempest::C3Vector &pos, int &fishable) {
  for (UINT grouplp = 0; grouplp < groupCount; ++grouplp) {
    const SMOGroupInfo *groupInfo = GetGroupInfo(grouplp);
    if ((ignoreGroupFlags & groupInfo->flags) || !(pos.x > groupInfo->aaBox.b.x) || !(pos.y > groupInfo->aaBox.b.y) ||
        !(pos.z > groupInfo->aaBox.b.z) || !(pos.x < groupInfo->aaBox.t.x) || !(pos.y < groupInfo->aaBox.t.y) || !(pos.z < groupInfo->aaBox.t.z) ||
        !IsGroupLoaded(grouplp))
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

UINT CMapObj::GetDoodadSet(UINT doodadIndex) {
  if (!bLoaded) {
    return -1;
  }

  if (doodadIndex < doodadDefCount) {
    for (UINT i = 0; i < doodadSetCount; ++i) {
      if (doodadSetList[i].count && doodadIndex >= doodadSetList[i].startIndex &&
          doodadIndex <= doodadSetList[i].startIndex + doodadSetList[i].count - 1) {
        return i;
      }
    }
  }
  return -1;
}

static int NearestPow2(float value) {
  return static_cast<int>(ceil(log10f(value) / log10f(2.0f)));
}

void CMapObj::QueryMapObjMinimapGroup(UINT groupID, UINT parentID, const NTempest::CAaBox &localBox, TSStackArray<CWorld::MinimapQuad> &quads) {
  CMapObjGroup *group = GetGroup(groupID, 0);
  if (!group || group->minimapTag == sMinimapTag) {
    return;
  }

  group->minimapTag = sMinimapTag;
  if (localBox.b.x > group->aaBox.t.x || localBox.b.y > group->aaBox.t.y || localBox.t.x < group->aaBox.b.x || localBox.t.y < group->aaBox.b.y) {
    return;
  }

  group->QueryMinimap(groupID, localBox, quads);
  SMOPortalRef *portalRef = &portalRefList[group->portalStart];
  for (UINT i = 0; i < group->portalCount; ++i, ++portalRef) {
    if (portalRef->groupIndex == 0xFFFF) {
      continue;
    }

    UINT toGroupID = portalRef->groupIndex;
    if (toGroupID == parentID) {
      continue;
    }

    SMOPortal                *portal = &portalList[portalRef->portalIndex];
    UINT                      clipFlags = 0xFFFFFFFF;
    const NTempest::C3Vector *point = &portalVertexList[portal->startVertex];
    for (UINT vertex = 0; vertex < portal->count; ++vertex, ++point) {
      UINT pointFlags = static_cast<UINT>(NTempest::CMath::realasint32_(point->x - localBox.b.x)) >> 31;
      pointFlags |= static_cast<UINT>(NTempest::CMath::realasint32_(point->y - localBox.b.y)) >> 31 << 1;
      pointFlags |= static_cast<UINT>(NTempest::CMath::realasint32_(point->z - localBox.b.z)) >> 31 << 2;
      pointFlags |= static_cast<UINT>(NTempest::CMath::realasint32_(localBox.t.x - point->x)) >> 31 << 3;
      pointFlags |= static_cast<UINT>(NTempest::CMath::realasint32_(localBox.t.y - point->y)) >> 31 << 4;
      pointFlags |= static_cast<UINT>(NTempest::CMath::realasint32_(localBox.t.z - point->z)) >> 31 << 5;
      clipFlags &= pointFlags;
    }

    if (!clipFlags) {
      QueryMapObjMinimapGroup(toGroupID, groupID, localBox, quads);
    }
  }
}

bool CMapObj::QueryMapObjMinimap(UINT groupID, const NTempest::CAaBox &localBox, TSStackArray<CWorld::MinimapQuad> &quads) {
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

void CMapObjGroup::QueryMinimap(UINT groupID, const NTempest::CAaBox &localBox, TSStackArray<CWorld::MinimapQuad> &quads) {
  if (flags & 0x88) {
    return;
  }

  NTempest::C3Vector center = aaBox.Center();
  NTempest::C2Vector nQuads(aaBox.t.x - aaBox.b.x, aaBox.t.y - aaBox.b.y);
  nQuads /= WMOMM_QUAD_SIZE;

  UINT                height = max(32, min(1u << NearestPow2(nQuads.y * 256.0f), 256));
  UINT                width = max(32, min(1u << NearestPow2(nQuads.x * 256.0f), 256));
  NTempest::C2iVector nTex(NTempest::CMath::ftol_0_256_(ceilf(nQuads.x)), NTempest::CMath::ftol_0_256_(ceilf(nQuads.y)));
  NTempest::C3Vector  blockSize(width * WMOMM_INCHES_PER_PIXEL, height * WMOMM_INCHES_PER_PIXEL, 0.0f);
  NTempest::C2iVector quad;

  for (quad.y = 0; quad.y < nTex.y; ++quad.y) {
    for (quad.x = 0; quad.x < nTex.x; ++quad.x) {
      NTempest::C3Vector corner(quad.x * blockSize.x + aaBox.b.x, quad.y * blockSize.y + aaBox.b.y, center.z);
      NTempest::CAaBox   quadBox(corner, NTempest::C3Vector(corner.x + blockSize.x, corner.y + blockSize.y, corner.z));
      if (localBox.b.x <= quadBox.t.x && localBox.b.y <= quadBox.t.y && localBox.t.x >= quadBox.b.x && localBox.t.y >= quadBox.b.y) {
        CWorld::MinimapQuad *result = quads.New();
        result->quad = quad;
        result->groupNum = groupID;
        result->aaBox = quadBox;
      }
    }
  }
}
