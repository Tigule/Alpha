#include <WowConst.h>
#include <MapDefs.h>

#include "WorldClient/CMapObj.h"
#include "WorldClient/World.h"

#include "Base/Base.h"
#include "Os/W32/Debugging.h"
#include "Services/AsyncFileRead.h"

#include <storm.h>

#include <float.h>
#include <string.h>

void CMapObj::AsyncPostloadCallbackHeader(void *userArg) {
  CMapObj *mapObj = static_cast<CMapObj *>(userArg);
  FATALASSERT(mapObj);

  AsyncFileReadDestroyObject(mapObj->asyncObject);
  mapObj->asyncObject = 0;
  if (mapObj->fileHeader.version != 14) {
    OsOutputDebugString("MAPWRONGVERSION|%s\n", mapObj->name);
    SFile::Close(mapObj->file);
    mapObj->file = 0;
    return;
  }

  FATALASSERT(mapObj->fileHeader.iffChunkHeader.token == 'MOMO');
  mapObj->dataBytes = mapObj->fileHeader.iffChunkHeader.size + sizeof(mapObj->fileHeader);
  mapObj->data = static_cast<unsigned char *>(SMemAlloc(mapObj->dataBytes, __FILE__, __LINE__, 0));
  FATALASSERT(mapObj->data);

  mapObj->asyncObject = AsyncFileReadCreateObject();
  FATALASSERT(mapObj->asyncObject);
  mapObj->asyncObject->file = mapObj->file;
  mapObj->asyncObject->offset = 0;
  mapObj->asyncObject->buffer = mapObj->data;
  mapObj->asyncObject->size = mapObj->dataBytes;
  mapObj->asyncObject->userArg = mapObj;
  mapObj->asyncObject->userPostloadCallback = AsyncPostloadCallbackAll;
  AsyncFileReadObject(mapObj->asyncObject);
}

void CMapObj::AsyncPostloadCallback(void *userArg) {
  CMapObj *mapObj = static_cast<CMapObj *>(userArg);
  FATALASSERT(mapObj);

  AsyncFileReadDestroyObject(mapObj->asyncObject);
  mapObj->asyncObject = 0;
  SFile::Close(mapObj->file);
  mapObj->file = 0;
  memcpy(&mapObj->fileHeader, mapObj->data, sizeof(mapObj->fileHeader));
  if (mapObj->fileHeader.version != 14) {
    OsOutputDebugString("MAPWRONGVERSION|%s\n", mapObj->name);
    SMemFree(mapObj->data, __FILE__, __LINE__, 0);
    mapObj->data = 0;
    mapObj->dataBytes = 0;
    return;
  }

  mapObj->CreateData();
  mapObj->CreateAllGroups();
}

void CMapObj::AsyncPostloadCallbackAll(void *userArg) {
  CMapObj *mapObj = static_cast<CMapObj *>(userArg);
  FATALASSERT(mapObj);

  AsyncFileReadDestroyObject(mapObj->asyncObject);
  mapObj->asyncObject = 0;
  mapObj->CreateData();
}

int CMapObj::Read(const char *fileName) {
  unsigned long bRead;

  FATALASSERT(file == 0);
  SStrCopy(name, fileName, 0x7FFFFFFF);
  if (!SFile::Open(fileName, &file)) {
    OsOutputDebugString("CMapObj::Read(): Failed to open %s\n", fileName);
    bLoaded = 1;
    return 0;
  }

  if (CMap::bPreload) {
    dataBytes = SFile::GetFileSize(file, 0);
    if (dataBytes < 0x500000) {
      data = static_cast<unsigned char *>(SMemAlloc(dataBytes, __FILE__, __LINE__, 0));
      FATALASSERT(data);
      SFile::Read(file, data, dataBytes, &bRead, 0, 0);
      SFile::Close(file);
      file = 0;
      memcpy(&fileHeader, data, sizeof(fileHeader));
      if (fileHeader.version != 14) {
        OsOutputDebugString("MAPWRONGVERSION|%s\n", name);
        SMemFree(data, __FILE__, __LINE__, 0);
        data = 0;
        dataBytes = 0;
        return 0;
      }

      CreateData();
      CreateAllGroups();
      return 1;
    }

    SFile::Read(file, &fileHeader, sizeof(fileHeader), &bRead, 0, 0);
    if (fileHeader.version != 14) {
      OsOutputDebugString("MAPWRONGVERSION|%s\n", name);
      SFile::Close(file);
      file = 0;
      return 0;
    }

    FATALASSERT(fileHeader.iffChunkHeader.token == 'MOMO');
    dataBytes = fileHeader.iffChunkHeader.size + sizeof(fileHeader);
    data = static_cast<unsigned char *>(SMemAlloc(dataBytes, __FILE__, __LINE__, 0));
    FATALASSERT(data);
    SFile::SetFilePointer(file, 0, 0, FILE_BEGIN);
    SFile::Read(file, data, dataBytes, &bRead, 0, 0);
    CreateData();
    return 1;
  }

  asyncObject = AsyncFileReadCreateObject();
  FATALASSERT(asyncObject);
  asyncObject->file = file;
  asyncObject->offset = 0;
  asyncObject->userArg = this;
  dataBytes = SFile::GetFileSize(file, 0);
  if (dataBytes < 0x500000) {
    data = static_cast<unsigned char *>(SMemAlloc(dataBytes, __FILE__, __LINE__, 0));
    FATALASSERT(data);
    asyncObject->buffer = data;
    asyncObject->size = dataBytes;
    asyncObject->userPostloadCallback = AsyncPostloadCallback;
  } else {
    asyncObject->buffer = &fileHeader;
    asyncObject->size = sizeof(fileHeader);
    asyncObject->userPostloadCallback = AsyncPostloadCallbackHeader;
  }
  AsyncFileReadObject(asyncObject);
  return 1;
}

void CMapObj::AllocGroups() {
  FATALASSERT(groupPtrList.Count() == 0);
  groupPtrList.SetCount(groupCount);
  for (unsigned int n = 0; n < groupCount; ++n) {
    groupPtrList[n] = CMap::AllocMapObjGroup();
    FATALASSERT(groupPtrList[n]);
  }
}

void CMapObj::CreateData() {
  unsigned int n;

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

  AllocGroups();
  bLoaded = 1;
}

void CMapObj::CreateAllGroups() {
  for (unsigned int i = 0; i < groupCount; ++i) {
    CreateGroup(groupPtrList[i], &groupInfoList[i]);
  }
}

void CMapObj::ReadExtGroups() {
  for (unsigned int i = 0; i < groupCount; ++i) {
    if (groupInfoList[i].flags & 0x88) {
      ReadGroup(groupPtrList[i], &groupInfoList[i], 0);
    }
  }
}

SIffChunk *CMapObj::ReadChunkHeader(unsigned char *&pData, unsigned long expectedToken) {
  SIffChunk *pIffChunk = reinterpret_cast<SIffChunk *>(pData);
  FATALASSERT(pIffChunk->token == expectedToken);
  pData += sizeof(SIffChunk);
  return pIffChunk;
}

SIffChunk *CMapObj::ReadOptionalChunkHeader(unsigned char *&pData, unsigned long expectedToken) {
  SIffChunk *pIffChunk = reinterpret_cast<SIffChunk *>(pData);
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
  unsigned char *pData = data + sizeof(fileHeader);
  SIffChunk     *pIffChunk;

  pIffChunk = ReadChunkHeader(pData, 'MOHD');
  header = reinterpret_cast<SMOHeader *>(pData);
  pData += pIffChunk->size;

  pIffChunk = ReadChunkHeader(pData, 'MOTX');
  textureNameList = reinterpret_cast<char *>(pData);
  textureNameCount = pIffChunk->size;
  pData += pIffChunk->size;

  pIffChunk = ReadChunkHeader(pData, 'MOMT');
  materialList = reinterpret_cast<SMOMaterial *>(pData);
  materialCount = pIffChunk->size / sizeof(*materialList);
  pData += pIffChunk->size;

  pIffChunk = ReadChunkHeader(pData, 'MOGN');
  groupNameList = reinterpret_cast<char *>(pData);
  groupNameCount = pIffChunk->size;
  pData += pIffChunk->size;

  pIffChunk = ReadChunkHeader(pData, 'MOGI');
  groupInfoList = reinterpret_cast<SMOGroupInfo *>(pData);
  groupCount = pIffChunk->size / sizeof(*groupInfoList);
  pData += pIffChunk->size;

  pIffChunk = ReadChunkHeader(pData, 'MOPV');
  portalVertexList = reinterpret_cast<NTempest::C3Vector *>(pData);
  portalVertexCount = pIffChunk->size / sizeof(*portalVertexList);
  pData += pIffChunk->size;

  pIffChunk = ReadChunkHeader(pData, 'MOPT');
  portalList = reinterpret_cast<SMOPortal *>(pData);
  portalCount = pIffChunk->size / 20;
  pData += pIffChunk->size;

  pIffChunk = ReadChunkHeader(pData, 'MOPR');
  portalRefList = reinterpret_cast<SMOPortalRef *>(pData);
  portalRefCount = pIffChunk->size / 8;
  pData += pIffChunk->size;

  pIffChunk = ReadChunkHeader(pData, 'MOLT');
  lightList = reinterpret_cast<SMOLight *>(pData);
  lightCount = pIffChunk->size / 32;
  pData += pIffChunk->size;

  pIffChunk = ReadChunkHeader(pData, 'MODS');
  doodadSetList = reinterpret_cast<SMODoodadSet *>(pData);
  doodadSetCount = pIffChunk->size / 32;
  pData += pIffChunk->size;

  pIffChunk = ReadChunkHeader(pData, 'MODN');
  doodadNameList = reinterpret_cast<char *>(pData);
  doodadNameCount = pIffChunk->size;
  pData += pIffChunk->size;

  pIffChunk = ReadChunkHeader(pData, 'MODD');
  doodadDefList = reinterpret_cast<SMODoodadDef *>(pData);
  doodadDefCount = pIffChunk->size / 40;
  pData += pIffChunk->size;

  pIffChunk = ReadChunkHeader(pData, 'MFOG');
  fogList = reinterpret_cast<SMOFog *>(pData);
  fogCount = pIffChunk->size / 48;
  pData += pIffChunk->size;

  pIffChunk = ReadOptionalChunkHeader(pData, 'MCVP');
  if (pIffChunk) {
    convexVolumePlanes = reinterpret_cast<NTempest::C4Plane *>(pData);
    volumePlaneCount = pIffChunk->size / sizeof(*convexVolumePlanes);
  }
}

void CMapObj::CreateMaterials() {
  for (unsigned int i = 0; i < materialCount; ++i) {
    materialList[i].hMaps[0] = 0;
    materialList[i].hMaps[1] = 0;
  }
}

void CMapObj::CreateMaterial(unsigned int materialId) {
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
  unsigned long bRead;

  FATALASSERT(group);
  FATALASSERT(groupInfo);

  group->data = static_cast<unsigned char *>(SMemAlloc(groupInfo->size, __FILE__, __LINE__, 0));
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

void CMapObjGroup::AsyncPostloadCallback(void *userArg) {
  CMapObjGroup *mapObjGroup = static_cast<CMapObjGroup *>(userArg);
  FATALASSERT(mapObjGroup);

  AsyncFileReadDestroyObject(mapObjGroup->asyncObject);
  mapObjGroup->asyncObject = 0;
  mapObjGroup->Create(mapObjGroup->data);
}

void CMapObjGroup::Create(unsigned char *rawData) {
  SMOGroupHeader *header = reinterpret_cast<SMOGroupHeader *>(rawData);
  FATALASSERT(header->iffChunk.token == 'MOGP');
  FATALASSERT(lameAssLink.IsLinked() == 0);
  FATALASSERT(parent);

  parent->groupList.LinkNode(this, LIST_TAIL, 0);
  dbgName = parent->groupNameList + header->nameOffset;
  flags = header->flags;
  aaBox = header->aaBox;
  portalStart = header->portalStart;
  portalCount = header->portalCount;
  memcpy(fogIds, header->fogIds, sizeof(fogIds));
  groupLiquid = header->groupLiquid;
  uniqueID = header->uniqueID;

  unsigned int i; 
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

  unsigned char *pData = reinterpret_cast<unsigned char *>(header + 1);
  CreateDataPointers(pData);

  CMapObj *mapObj = parent;
  FATALASSERT(mapObj);
  for (i = 0; i < 4; ++i) {
    unsigned int n; 
    for (n = 0; n < intBatch[i].batchCount; ++n) {
      mapObj->CreateMaterial(batchList[intBatch[i].batchStart + n].texture);
    }
    for (n = 0; n < extBatch[i].batchCount; ++n) {
      mapObj->CreateMaterial(batchList[extBatch[i].batchStart + n].texture);
    }
  }
  bLoaded = 1;
}

void CMapObjGroup::CreateOptionalDataPointers(unsigned char *pData) {
  SIffChunk *pIffChunk;
  if (flags & 0x200) {
    pIffChunk = reinterpret_cast<SIffChunk *>(pData);
    FATALASSERT(pIffChunk->token == 'MOLR');
    pData += sizeof(SIffChunk);
    lightRefList = reinterpret_cast<unsigned short *>(pData);
    lightRefCount = pIffChunk->size / 2;
    pData += pIffChunk->size;
  }
  if (flags & 0x800) {
    pIffChunk = reinterpret_cast<SIffChunk *>(pData);
    FATALASSERT(pIffChunk->token == 'MODR');
    pData += sizeof(SIffChunk);
    doodadRefList = reinterpret_cast<unsigned short *>(pData);
    doodadRefCount = pIffChunk->size / 2;
    pData += pIffChunk->size;
  }
  if (flags & 1) {
    pIffChunk = reinterpret_cast<SIffChunk *>(pData);
    FATALASSERT(pIffChunk->token == 'MOBN');
    pData += sizeof(SIffChunk);
    CAaBspNode  *bspNodeList = reinterpret_cast<CAaBspNode *>(pData);
    unsigned int nNodes = pIffChunk->size / 16;
    pData += pIffChunk->size;
    pIffChunk = reinterpret_cast<SIffChunk *>(pData);
    FATALASSERT(pIffChunk->token == 'MOBR');
    pData += sizeof(SIffChunk);
    unsigned short *faceIndices = reinterpret_cast<unsigned short *>(pData);
    unsigned int    nFaceIndices = pIffChunk->size / 2;
    pData += pIffChunk->size;
    aaBsp.Set(bspNodeList, nNodes, faceIndices, nFaceIndices, aaBox);
  }
  if (flags & 0x400) {
    const unsigned long tokens[4] = {'MPBV', 'MPBP', 'MPBI', 'MPBG'};
    for (unsigned int i = 0; i < 4; ++i) {
      pIffChunk = reinterpret_cast<SIffChunk *>(pData);
      FATALASSERT(pIffChunk->token == tokens[i]);
      pData += sizeof(SIffChunk) + pIffChunk->size;
    }
  }
  if (flags & 4) {
    pIffChunk = reinterpret_cast<SIffChunk *>(pData);
    FATALASSERT(pIffChunk->token == 'MOCV');
    pData += sizeof(SIffChunk);
    colorVertexList = reinterpret_cast<NTempest::CImVector *>(pData);
    colorVertexCount = pIffChunk->size / 4;
    pData += pIffChunk->size;
  }
  if (flags & 2) {
    CreateLightmapPointers(pData);
  }
  if (flags & 0x1000) {
    pIffChunk = reinterpret_cast<SIffChunk *>(pData);
    FATALASSERT(pIffChunk->token == 'MLIQ');
    pData += sizeof(SIffChunk);
    liquidVerts.x = *reinterpret_cast<unsigned int *>(pData);
    pData += sizeof(unsigned int);
    liquidVerts.y = *reinterpret_cast<unsigned int *>(pData);
    pData += sizeof(unsigned int);
    liquidTiles.x = *reinterpret_cast<unsigned int *>(pData);
    pData += sizeof(unsigned int);
    liquidTiles.y = *reinterpret_cast<unsigned int *>(pData);
    pData += sizeof(unsigned int);
    liquidCorner.x = *reinterpret_cast<float *>(pData);
    pData += sizeof(float);
    liquidCorner.y = *reinterpret_cast<float *>(pData);
    pData += sizeof(float);
    liquidCorner.z = *reinterpret_cast<float *>(pData);
    pData += sizeof(float);
    liquidMtlId = *reinterpret_cast<unsigned short *>(pData);
    pData += sizeof(unsigned short);
    liquidVertexList = reinterpret_cast<SMOLVert *>(pData);
    pData += sizeof(SMOLVert) * liquidVerts.x * liquidVerts.y;
    liquidTileList = reinterpret_cast<SMOLTile *>(pData);
  }
}

void CMapObjGroup::CreateDataPointers(unsigned char *pData) {
  SIffChunk *pIffChunk = reinterpret_cast<SIffChunk *>(pData);
  FATALASSERT(pIffChunk->token == 'MOPY');
  pData += sizeof(SIffChunk);
  polyList = reinterpret_cast<SMOPoly *>(pData);
  polyCount = pIffChunk->size / 4;
  pData += pIffChunk->size;

  pIffChunk = reinterpret_cast<SIffChunk *>(pData);
  FATALASSERT(pIffChunk->token == 'MOVT');
  pData += sizeof(SIffChunk);
  vertexList = reinterpret_cast<NTempest::C3Vector *>(pData);
  vertexCount = pIffChunk->size / 12;
  pData += pIffChunk->size;

  pIffChunk = reinterpret_cast<SIffChunk *>(pData);
  FATALASSERT(pIffChunk->token == 'MONR');
  pData += sizeof(SIffChunk);
  normalList = reinterpret_cast<NTempest::C3Vector *>(pData);
  normalCount = pIffChunk->size / 12;
  pData += pIffChunk->size;

  pIffChunk = reinterpret_cast<SIffChunk *>(pData);
  FATALASSERT(pIffChunk->token == 'MOTV');
  pData += sizeof(SIffChunk);
  textureVertexList = reinterpret_cast<NTempest::C2Vector *>(pData);
  textureVertexCount = pIffChunk->size / 8;
  pData += pIffChunk->size;

  pIffChunk = reinterpret_cast<SIffChunk *>(pData);
  FATALASSERT(pIffChunk->token == 'MOLV');
  pData += sizeof(SIffChunk);
  lightmapVertexList = reinterpret_cast<NTempest::C2Vector *>(pData);
  lightmapVertexCount = pIffChunk->size / 8;
  pData += pIffChunk->size;

  pIffChunk = reinterpret_cast<SIffChunk *>(pData);
  FATALASSERT(pIffChunk->token == 'MOIN');
  pData += sizeof(SIffChunk);
  indexList = reinterpret_cast<unsigned short *>(pData);
  indexCount = pIffChunk->size / 2;
  pData += pIffChunk->size;

  pIffChunk = reinterpret_cast<SIffChunk *>(pData);
  FATALASSERT(pIffChunk->token == 'MOBA');
  pData += sizeof(SIffChunk);
  batchList = reinterpret_cast<SMOBatch *>(pData);
  batchCount = pIffChunk->size / 24;
  pData += pIffChunk->size;
  CreateOptionalDataPointers(pData);
}

void CMapObjGroup::CreateLightmapPointers(unsigned char *&pData) {
  SIffChunk *pIffChunk = reinterpret_cast<SIffChunk *>(pData);
  FATALASSERT(pIffChunk->token == 'MOLM');
  pData += sizeof(SIffChunk);
  lightmapList = reinterpret_cast<SMOLightmap *>(pData);
  lightmapCount = pIffChunk->size / sizeof(*lightmapList);
  pData += pIffChunk->size;

  pIffChunk = reinterpret_cast<SIffChunk *>(pData);
  FATALASSERT(pIffChunk->token == 'MOLD');
  pData += sizeof(SIffChunk);
  lightmapTexList = reinterpret_cast<SMOLightmapTex *>(pData);
  lightmapTexCount = pIffChunk->size / sizeof(*lightmapTexList);
  pData += pIffChunk->size;
}
