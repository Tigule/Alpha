#include "Base/Base.h"
#include "Gx/Gx.h"
#include "Services/ParticleSystem2.h"
#include <WowConst.h>
#include "AaBsp.h"
#include <MapDefs.h>

#include "WorldClient/World.h"
#include "WorldClient/Map.h"
#include "WorldClient/WorldParam.h"
#include "WorldClient/DetailDoodad.h"
#include "WorldClient/CSimpleDoodad.h"
#include "DayNight.h"

#include "Os/W32/Debugging.h"
#include "Services/AsyncFileRead.h"
#include "Services/SysMessage.h"

#include <storm.h>

#include <ctype.h>
#include <float.h>
#include <string.h>

void CMapObj::AsyncPostloadCallbackHeader(LPVOID userArg) {
  CMapObj *mapObj = (CMapObj *)userArg;
  FATALASSERT(mapObj);

  AsyncFileReadDestroyObject(mapObj->asyncObject);
  mapObj->asyncObject = 0;
  if (mapObj->fileHeader.version != 14) {
    SysMsgPrintf(SYSMSG_FATAL, 2, "MAPWRONGVERSION|%s", mapObj->name);
    SFile::Close(mapObj->file);
    mapObj->file = 0;
    return;
  }

  FATALASSERT(mapObj->fileHeader.iffChunkHeader.token=='MOMO');
  DWORD dataBytes = mapObj->fileHeader.iffChunkHeader.size + sizeof(mapObj->fileHeader);
  mapObj->data = (BYTE *)SMemAlloc(dataBytes, __FILE__, __LINE__, 0);
  FATALASSERT(mapObj->data);
  mapObj->dataBytes = dataBytes;

  mapObj->asyncObject = AsyncFileReadCreateObject();
  FATALASSERT(mapObj->asyncObject);
  mapObj->asyncObject->file = mapObj->file;
  mapObj->asyncObject->buffer = mapObj->data;
  mapObj->asyncObject->offset = 0;
  mapObj->asyncObject->size = dataBytes;
  mapObj->asyncObject->userArg = mapObj;
  mapObj->asyncObject->userPostloadCallback = AsyncPostloadCallback;
  AsyncFileReadObject(mapObj->asyncObject);
}

void CMapObj::AsyncPostloadCallback(LPVOID userArg) {
  CMapObj *mapObj = (CMapObj *)userArg;
  FATALASSERT(mapObj);

  AsyncFileReadDestroyObject(mapObj->asyncObject);
  mapObj->asyncObject = 0;
  mapObj->CreateData();
}

void CMapObj::AsyncPostloadCallbackAll(LPVOID userArg) {
  CMapObj *mapObj = (CMapObj *)userArg;
  FATALASSERT(mapObj);

  AsyncFileReadDestroyObject(mapObj->asyncObject);
  mapObj->asyncObject = 0;
  SFile::Close(mapObj->file);
  mapObj->file = 0;
  memcpy(&mapObj->fileHeader, mapObj->data, sizeof(mapObj->fileHeader));
  if (mapObj->fileHeader.version != 14) {
    SysMsgPrintf(SYSMSG_FATAL, 2, "MAPWRONGVERSION|%s", mapObj->name);
    SMemFree(mapObj->data, __FILE__, __LINE__, 0);
    mapObj->dataBytes = 0;
    mapObj->data = 0;
    return;
  }

  mapObj->CreateData();
  mapObj->CreateAllGroups();
}

BOOL CMapObj::Read(LPCSTR fileName) {
  FATALASSERT(file == 0);
  SStrCopy(name, fileName, 0x7FFFFFFF);
  if (!SFile::Open(fileName, &file)) {
    OsOutputDebugString("CMapObj::Read(): Failed to open %s\n", fileName);
    bLoaded = 1;
    return 0;
  }

  if (CMap::bPreload) {
    DWORD fileSize = SFile::GetFileSize(file, 0);
    if (fileSize < 0x500000) {
      data = (BYTE *)SMemAlloc(fileSize, __FILE__, __LINE__, 0);
      FATALASSERT(data);
      SFile::Read(file, data, fileSize, &dataBytes, 0, 0);
      SFile::Close(file);
      file = 0;
      memcpy(&fileHeader, data, sizeof(fileHeader));
      if (fileHeader.version != 14) {
        SysMsgPrintf(SYSMSG_FATAL, 2, "MAPWRONGVERSION|%s", name);
        SMemFree(data, __FILE__, __LINE__, 0);
        dataBytes = 0;
        data = 0;
        return 0;
      }

      CreateData();
      CreateAllGroups();
      return 1;
    }

    DWORD bRead;
    SFile::Read(file, &fileHeader, sizeof(fileHeader), &bRead, 0, 0);
    if (fileHeader.version != 14) {
      SysMsgPrintf(SYSMSG_FATAL, 2, "MAPWRONGVERSION|%s", name);
      SFile::Close(file);
      file = 0;
      return 0;
    }

    FATALASSERT(fileHeader.iffChunkHeader.token=='MOMO');
    fileSize = fileHeader.iffChunkHeader.size + sizeof(fileHeader);
    data = (BYTE *)SMemAlloc(fileSize, __FILE__, __LINE__, 0);
    FATALASSERT(data);
    SFile::SetFilePointer(file, 0, 0, FILE_BEGIN);
    SFile::Read(file, data, fileSize, &dataBytes, 0, 0);
    CreateData();
    return 1;
  }

  asyncObject = AsyncFileReadCreateObject();
  FATALASSERT(asyncObject);
  asyncObject->file = file;
  asyncObject->offset = 0;
  asyncObject->userArg = this;
  DWORD fileSize = SFile::GetFileSize(file, 0);
  if (fileSize < 0x500000) {
    data = (BYTE *)SMemAlloc(fileSize, __FILE__, __LINE__, 0);
    FATALASSERT(data);
    dataBytes = fileSize;
    asyncObject->buffer = data;
    asyncObject->size = fileSize;
    asyncObject->userPostloadCallback = AsyncPostloadCallbackAll;
  } else {
    asyncObject->buffer = &fileHeader;
    asyncObject->size = sizeof(fileHeader);
    asyncObject->userPostloadCallback = AsyncPostloadCallbackHeader;
  }
  AsyncFileReadObject(asyncObject);
  return 1;
}

void CMapObj::CreateData() {
  UINT n;

  CreateDataPointers();
  CreateMaterials();
  ambColor = header->ambColor;
  aaBox.b.x = FLT_MAX;
  aaBox.b.y = FLT_MAX;
  aaBox.b.z = FLT_MAX;
  aaBox.t.x = -FLT_MAX;
  aaBox.t.y = -FLT_MAX;
  aaBox.t.z = -FLT_MAX;
  for (n = 0; n < groupCount; ++n) {
    if (groupInfoList[n].aaBox.b.x < aaBox.b.x)
      aaBox.b.x = groupInfoList[n].aaBox.b.x;
    if (groupInfoList[n].aaBox.b.y < aaBox.b.y)
      aaBox.b.y = groupInfoList[n].aaBox.b.y;
    if (groupInfoList[n].aaBox.b.z < aaBox.b.z)
      aaBox.b.z = groupInfoList[n].aaBox.b.z;
    if (groupInfoList[n].aaBox.t.x > aaBox.t.x)
      aaBox.t.x = groupInfoList[n].aaBox.t.x;
    if (groupInfoList[n].aaBox.t.y > aaBox.t.y)
      aaBox.t.y = groupInfoList[n].aaBox.t.y;
    if (groupInfoList[n].aaBox.t.z > aaBox.t.z)
      aaBox.t.z = groupInfoList[n].aaBox.t.z;
  }

  FATALASSERT(groupPtrList.Count() == 0);
  groupPtrList.SetCount(groupCount);
  for (n = 0; n < groupCount; ++n) {
    groupPtrList[n] = CMap::AllocMapObjGroup();
    FATALASSERT(groupPtrList[n]);
  }

  bLoaded = 1;
}

void CMapObj::CreateAllGroups() {
  SMOGroupInfo *groupInfo = groupInfoList;
  for (UINT i = 0; i < groupCount; ++i, ++groupInfo) {
    CreateGroup(groupPtrList[i], groupInfo);
  }
}

void CMapObj::ReadExtGroups() {
  SMOGroupInfo *groupInfo = groupInfoList;
  for (UINT i = 0; i < groupCount; ++i, ++groupInfo) {
    if (groupInfo->flags & 0x88) {
      ReadGroup(groupPtrList[i], groupInfo, 0);
    }
  }
}

SIffChunk *CMapObj::ReadChunkHeader(BYTE *&pData, DWORD expectedToken) {
  SIffChunk *pIffChunk = (SIffChunk *)pData;
  if (!(pIffChunk->token==expectedToken)) {
    SErrDisplayErrorFmt(
        STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
        isprint((expectedToken >> 24) & 0xFF) && isprint((expectedToken >> 16) & 0xFF) && isprint((expectedToken >> 8) & 0xFF) &&
                isprint(expectedToken & 0xFF)
            ? "\"%s\", %s = %ld (0x%08X, '%c%c%c%c')"
            : "\"%s\", %s = %ld (0x%08X)",
        "(pIffChunk->token==expectedToken)", "expectedToken", expectedToken, expectedToken, (expectedToken >> 24) & 0xFF,
        (expectedToken >> 16) & 0xFF, (expectedToken >> 8) & 0xFF, expectedToken & 0xFF
    );
  }
  pData += sizeof(SIffChunk);
  return pIffChunk;
}

SIffChunk *CMapObj::ReadOptionalChunkHeader(BYTE *&pData, DWORD expectedToken) {
  SIffChunk *pIffChunk = (SIffChunk *)pData;
  if (pData >= data + dataBytes) {
    return 0;
  }
  if (pIffChunk->token != expectedToken) {
    return 0;
  }
  pData += sizeof(SIffChunk);
  return pIffChunk;
}

void CMapObj::CreateDataPointers() {
  BYTE      *pData = data + sizeof(fileHeader);
  SIffChunk *pIffChunk;

  pIffChunk = ReadChunkHeader(pData, 'MOHD');
  header = (SMOHeader *)pData;
  pData += pIffChunk->size;

  pIffChunk = ReadChunkHeader(pData, 'MOTX');
  textureNameList = (char *)pData;
  textureNameCount = pIffChunk->size;
  pData += pIffChunk->size;

  pIffChunk = ReadChunkHeader(pData, 'MOMT');
  materialList = (SMOMaterial *)pData;
  materialCount = pIffChunk->size / sizeof(*materialList);
  pData += pIffChunk->size;

  pIffChunk = ReadChunkHeader(pData, 'MOGN');
  groupNameList = (char *)pData;
  groupNameCount = pIffChunk->size;
  pData += pIffChunk->size;

  pIffChunk = ReadChunkHeader(pData, 'MOGI');
  groupInfoList = (SMOGroupInfo *)pData;
  groupCount = pIffChunk->size / sizeof(*groupInfoList);
  pData += pIffChunk->size;

  pIffChunk = ReadChunkHeader(pData, 'MOPV');
  portalVertexList = (NTempest::C3Vector *)pData;
  portalVertexCount = pIffChunk->size / sizeof(*portalVertexList);
  pData += pIffChunk->size;

  pIffChunk = ReadChunkHeader(pData, 'MOPT');
  portalList = (SMOPortal *)pData;
  portalCount = pIffChunk->size / 20;
  pData += pIffChunk->size;

  pIffChunk = ReadChunkHeader(pData, 'MOPR');
  portalRefList = (SMOPortalRef *)pData;
  portalRefCount = pIffChunk->size / 8;
  pData += pIffChunk->size;

  pIffChunk = ReadChunkHeader(pData, 'MOLT');
  lightList = (SMOLight *)pData;
  lightCount = pIffChunk->size / 32;
  pData += pIffChunk->size;

  pIffChunk = ReadChunkHeader(pData, 'MODS');
  doodadSetList = (SMODoodadSet *)pData;
  doodadSetCount = pIffChunk->size / 32;
  pData += pIffChunk->size;

  pIffChunk = ReadChunkHeader(pData, 'MODN');
  doodadNameList = (char *)pData;
  doodadNameCount = pIffChunk->size;
  pData += pIffChunk->size;

  pIffChunk = ReadChunkHeader(pData, 'MODD');
  doodadDefList = (SMODoodadDef *)pData;
  doodadDefCount = pIffChunk->size / 40;
  pData += pIffChunk->size;

  pIffChunk = ReadChunkHeader(pData, 'MFOG');
  fogList = (SMOFog *)pData;
  fogCount = pIffChunk->size / 48;
  pData += pIffChunk->size;

  pIffChunk = ReadOptionalChunkHeader(pData, 'MCVP');
  if (pIffChunk) {
    convexVolumePlanes = (NTempest::C4Plane *)pData;
    volumePlaneCount = pIffChunk->size / sizeof(*convexVolumePlanes);
  }
}

void CMapObj::CreateMaterials() {
  SMOMaterial *material = materialList;
  for (UINT i = 0; i < materialCount; ++i, ++material) {
    material->hMaps[0] = 0;
    material->hMaps[1] = 0;
  }
}

void CMapObj::CreateMaterial(UINT materialId) {
  FATALASSERT(materialId < materialCount);

  SMOMaterial *material = &materialList[materialId];
  if (!material->hMaps[0]) {
    char *textureName = textureNameList + material->diffuseNameIndex;
    FATALASSERT(textureName);
    if (*textureName) {
      material->hMaps[0] = CMap::LoadTexture(textureName);
      textureName = textureNameList + material->envNameIndex;
      if (*textureName) {
        material->hMaps[1] = CMap::LoadTexture(textureName);
      } else {
        material->hMaps[1] = 0;
      }
    }
  }
}

void CMapObj::CreateGroup(CMapObjGroup *group, SMOGroupInfo *groupInfo) {
  FATALASSERT(group);
  FATALASSERT(groupInfo);

  group->data = 0;
  group->parent = this;
  group->Create(data + groupInfo->offset);
}

void CMapObj::ReadGroup(CMapObjGroup *group, SMOGroupInfo *groupInfo, int preLoad) {
  DWORD bRead;

  FATALASSERT(group);
  FATALASSERT(groupInfo);

  group->data = (BYTE *)SMemAlloc(groupInfo->size, __FILE__, __LINE__, 0);
  FATALASSERT(group->data);
  group->parent = this;

  if (preLoad) {
    SFile::SetFilePointer(file, groupInfo->offset, 0, FILE_BEGIN);
    SFile::Read(file, group->data, groupInfo->size, &bRead, 0, 0);
    group->Create(group->data);
  } else {
    CAsyncObject *asyncObject = AsyncFileReadCreateObject();
    FATALASSERT(asyncObject);
    asyncObject->file = file;
    asyncObject->buffer = group->data;
    asyncObject->offset = groupInfo->offset;
    asyncObject->size = groupInfo->size;
    asyncObject->userArg = group;
    asyncObject->userPostloadCallback = CMapObjGroup::AsyncPostloadCallback;
    group->asyncObject = asyncObject;
    AsyncFileReadObject(asyncObject);
  }
}

void CMapObjGroup::AsyncPostloadCallback(LPVOID userArg) {
  CMapObjGroup *mapObjGroup = (CMapObjGroup *)userArg;
  FATALASSERT(mapObjGroup);

  AsyncFileReadDestroyObject(mapObjGroup->asyncObject);
  mapObjGroup->asyncObject = 0;
  mapObjGroup->Create(mapObjGroup->data);
}

void CMapObjGroup::Create(BYTE *rawData) {
  SIffChunk *iffChunk = (SIffChunk *)rawData;
  FATALASSERT(iffChunk->token=='MOGP');
  FATALASSERT(lameAssLink.IsLinked() == 0);
  FATALASSERT(parent);
  parent->groupList.LinkNode(this, LIST_TAIL, 0);

  BYTE     *pData = rawData + sizeof(SIffChunk);
  SMOGroup *header = (SMOGroup *)pData;
  pData += sizeof(SMOGroup);

  dbgName = parent->groupNameList + header->nameOffset;
  flags = header->flags;
  aaBox = header->aaBox;
  portalStart = header->portalStart;
  portalCount = header->portalCount;
  memcpy(fogIds, header->fogIds, sizeof(fogIds));
  groupLiquid = header->groupLiquid;
  uniqueID = header->uniqueID;

  UINT i;
  for (i = 0; i < 4; ++i) {
    intBatch[i] = header->intBatch[i];
    extBatch[i] = header->extBatch[i];
    if (extBatch[i].batchCount) {
      extGxBuf[i] = AllocExtGxBuf(extBatch[i].vertCount, extBatch[i].vertCount);
      extGxBuf[i]->UserArgSet(this);
    }
    if (intBatch[i].batchCount) {
      intGxBuf[i] = AllocIntGxBuf(intBatch[i].vertCount, intBatch[i].vertCount);
      intGxBuf[i]->UserArgSet(this);
    }
  }

  CreateDataPointers(pData);

  CMapObj *mapObj = parent;
  FATALASSERT(mapObj);
  for (i = 0; i < 4; ++i) {
    UINT n;
    if (intBatch[i].batchCount) {
      SMOBatch *batch = &batchList[intBatch[i].batchStart];
      for (n = 0; n < intBatch[i].batchCount; ++n) {
        mapObj->CreateMaterial(batch[n].texture);
      }
    }
    if (extBatch[i].batchCount) {
      SMOBatch *batch = &batchList[extBatch[i].batchStart];
      for (n = 0; n < extBatch[i].batchCount; ++n) {
        mapObj->CreateMaterial(batch[n].texture);
      }
    }
  }
  bLoaded = 1;
}

void CMapObjGroup::CreateDataPointers(BYTE *pData) {
  SIffChunk *pIffChunk = (SIffChunk *)pData;
  FATALASSERT(pIffChunk->token=='MOPY');
  pData += sizeof(SIffChunk);
  polyList = (SMOPoly *)pData;
  polyCount = pIffChunk->size / 4;
  pData += pIffChunk->size;

  pIffChunk = (SIffChunk *)pData;
  FATALASSERT(pIffChunk->token=='MOVT');
  pData += sizeof(SIffChunk);
  vertexList = (NTempest::C3Vector *)pData;
  vertexCount = pIffChunk->size / 12;
  pData += pIffChunk->size;

  pIffChunk = (SIffChunk *)pData;
  FATALASSERT(pIffChunk->token=='MONR');
  pData += sizeof(SIffChunk);
  normalList = (NTempest::C3Vector *)pData;
  normalCount = pIffChunk->size / 12;
  pData += pIffChunk->size;

  pIffChunk = (SIffChunk *)pData;
  FATALASSERT(pIffChunk->token=='MOTV');
  pData += sizeof(SIffChunk);
  textureVertexList = (NTempest::C2Vector *)pData;
  textureVertexCount = pIffChunk->size / 8;
  pData += pIffChunk->size;

  pIffChunk = (SIffChunk *)pData;
  FATALASSERT(pIffChunk->token=='MOLV');
  pData += sizeof(SIffChunk);
  lightmapVertexList = (NTempest::C2Vector *)pData;
  lightmapVertexCount = pIffChunk->size / 8;
  pData += pIffChunk->size;

  pIffChunk = (SIffChunk *)pData;
  FATALASSERT(pIffChunk->token=='MOIN');
  pData += sizeof(SIffChunk);
  indexList = (WORD *)pData;
  indexCount = pIffChunk->size / 2;
  pData += pIffChunk->size;

  pIffChunk = (SIffChunk *)pData;
  FATALASSERT(pIffChunk->token=='MOBA');
  pData += sizeof(SIffChunk);
  batchList = (SMOBatch *)pData;
  batchCount = pIffChunk->size / 24;
  pData += pIffChunk->size;
  CreateOptionalDataPointers(pData);
}

void CMapObjGroup::CreateOptionalDataPointers(BYTE *pData) {
  SIffChunk *pIffChunk;
  if (flags & 0x200) {
    pIffChunk = (SIffChunk *)pData;
    FATALASSERT(pIffChunk->token=='MOLR');
    pData += sizeof(SIffChunk);
    lightRefList = (WORD *)pData;
    lightRefCount = pIffChunk->size / 2;
    pData += pIffChunk->size;
  }
  if (flags & 0x800) {
    pIffChunk = (SIffChunk *)pData;
    FATALASSERT(pIffChunk->token=='MODR');
    pData += sizeof(SIffChunk);
    doodadRefList = (WORD *)pData;
    doodadRefCount = pIffChunk->size / 2;
    pData += pIffChunk->size;
  }
  if (flags & 1) {
    pIffChunk = (SIffChunk *)pData;
    FATALASSERT(pIffChunk->token=='MOBN');
    pData += sizeof(SIffChunk);
    CAaBspNode *bspNodeList = (CAaBspNode *)pData;
    UINT        nNodes = pIffChunk->size / 16;
    pData += pIffChunk->size;
    pIffChunk = (SIffChunk *)pData;
    FATALASSERT(pIffChunk->token=='MOBR');
    pData += sizeof(SIffChunk);
    WORD *faceIndices = (WORD *)pData;
    UINT  nFaceIndices = pIffChunk->size / 2;
    pData += pIffChunk->size;
    aaBsp.Set(bspNodeList, nNodes, faceIndices, nFaceIndices, aaBox);
  }
  if (flags & 0x400) {
    pIffChunk = (SIffChunk *)pData;
    FATALASSERT(pIffChunk->token=='MPBV');
    pData += sizeof(SIffChunk);
    pData += pIffChunk->size;
    pIffChunk = (SIffChunk *)pData;
    FATALASSERT(pIffChunk->token=='MPBP');
    pData += sizeof(SIffChunk);
    pData += pIffChunk->size;
    pIffChunk = (SIffChunk *)pData;
    FATALASSERT(pIffChunk->token=='MPBI');
    pData += sizeof(SIffChunk);
    pData += pIffChunk->size;
    pIffChunk = (SIffChunk *)pData;
    FATALASSERT(pIffChunk->token=='MPBG');
    pData += sizeof(SIffChunk);
    pData += pIffChunk->size;
  }
  if (flags & 4) {
    pIffChunk = (SIffChunk *)pData;
    FATALASSERT(pIffChunk->token=='MOCV');
    pData += sizeof(SIffChunk);
    colorVertexList = (NTempest::CImVector *)pData;
    colorVertexCount = pIffChunk->size / 4;
    pData += pIffChunk->size;
  }
  if (flags & 2) {
    CreateLightmapPointers(pData);
  }
  if (flags & 0x1000) {
    pIffChunk = (SIffChunk *)pData;
    FATALASSERT(pIffChunk->token == 'MLIQ');
    pData += sizeof(SIffChunk);
    liquidVerts = *(NTempest::C2iVector *)pData;
    pData += sizeof(NTempest::C2iVector);
    liquidTiles = *(NTempest::C2iVector *)pData;
    pData += sizeof(NTempest::C2iVector);
    liquidCorner = *(NTempest::C3Vector *)pData;
    pData += sizeof(NTempest::C3Vector);
    liquidMtlId = *(WORD *)pData;
    pData += sizeof(WORD);
    liquidVertexList = (SMOLVert *)pData;
    pData += sizeof(SMOLVert) * liquidVerts.x * liquidVerts.y;
    liquidTileList = (SMOLTile *)pData;
  }
}

void CMapObjGroup::CreateLightmapPointers(BYTE *&pData) {
  SIffChunk *pIffChunk = (SIffChunk *)pData;
  FATALASSERT(pIffChunk->token=='MOLM');
  pData += sizeof(SIffChunk);
  lightmapList = (SMOLightmap *)pData;
  lightmapCount = pIffChunk->size / sizeof(*lightmapList);
  pData += pIffChunk->size;

  pIffChunk = (SIffChunk *)pData;
  FATALASSERT(pIffChunk->token=='MOLD');
  pData += sizeof(SIffChunk);
  lightmapTexList = (SMOLightmapTex *)pData;
  lightmapTexCount = pIffChunk->size / sizeof(*lightmapTexList);
  pData += pIffChunk->size;
}
