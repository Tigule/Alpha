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

#include "Services/AsyncFileRead.h"
#include "Services/Texture.h"

static SCritSect              s_fileCritSect;
static TSCArray<BYTE, 163840> s_asyncLoadBuffers[4];
static LISTDECLEX(CAsyncObject, link, s_asyncLoadList);
static BYTE *s_freeAsyncBuffer;
static BYTE  s_asyncBuffersInitialized;
static TSCArray<BYTE, 163840> s_syncLoadBuffer;

void CMapArea::FreeAsyncLoadBuffer(BYTE *buffer) {
  *(BYTE **)buffer = s_freeAsyncBuffer;
  s_freeAsyncBuffer = buffer;
}

void CMapArea::InitAsyncLoadBuffers() {
  for (UINT index = 0; index < 4; ++index) {
    FreeAsyncLoadBuffer(s_asyncLoadBuffers[index].Ptr());
  }
}

BYTE *CMapArea::AllocAsyncLoadBuffer() {
  if (!s_asyncBuffersInitialized) {
    InitAsyncLoadBuffers();
    s_asyncBuffersInitialized = 1;
  }

  if (!s_freeAsyncBuffer) {
    return 0;
  }

  BYTE *buffer = s_freeAsyncBuffer;
  s_freeAsyncBuffer = *(BYTE **)buffer;
  return buffer;
}

void CMapArea::Initialize() {
  AsyncFileReadAddHandler(AsyncPollHandler);
}

void CMapArea::Destroy() {
}

void CMapArea::AsyncPollHandler() {
  CAsyncObject *object = s_asyncLoadList.Head();

  while (object) {
    BYTE *buffer = AllocAsyncLoadBuffer();
    if (!buffer) {
      break;
    }

    CAsyncObject *next = object->link.Next();
    s_asyncLoadList.UnlinkNode(object);
    object->buffer = buffer;
    object->canReorder = 1;
    AsyncFileReadObject(object);
    object = next;
  }
}

CMapArea::CMapArea() {
  texCount = 0;
  asyncObject = 0;
  for (UINT i = 0; i < 256; ++i) {
    chunkTable[i] = 0;
    chunkInfo[i].offset = 0;
    chunkInfo[i].size = 0;
    chunkInfo[i].flags = 0;
    chunkInfo[i].asyncId = 0;
  }
  InitWater();
}

CMapArea::~CMapArea() {
}

void CMapArea::Load(SMAreaInfo *areaInfo) {
  FATALASSERT(areaInfo);
  FATALASSERT(areaInfo->size < 0x28000);

  if (CMap::bPreload) {
    s_fileCritSect.Enter();
    SFile::SetFilePointer(CMap::wdtFile, areaInfo->offset, 0, FILE_BEGIN);
    SFile::Read(CMap::wdtFile, s_syncLoadBuffer.Ptr(), areaInfo->size, 0, 0, 0);
    Create(s_syncLoadBuffer.Ptr());
    s_fileCritSect.Leave();
  } else {
    asyncObject = AsyncFileReadCreateObject();
    FATALASSERT(asyncObject);

    asyncObject->file = CMap::wdtFile;
    asyncObject->buffer = AllocAsyncLoadBuffer();
    asyncObject->offset = areaInfo->offset;
    asyncObject->size = areaInfo->size;
    asyncObject->userArg = this;
    asyncObject->userPostloadCallback = AsyncCallback;
    asyncObject->critSect = &s_fileCritSect;

    if (asyncObject->buffer) {
      AsyncFileReadObject(asyncObject);
    } else {
      asyncObject->canReorder = 0;
      s_asyncLoadList.LinkNode(asyncObject, LIST_TAIL, 0);
    }

    areaInfo->asyncId = (UINT)asyncObject;
  }
}

void CMapArea::PrepareLocalRect() {
  localRect.minx = CWorld::gbChunkRect.minx - 16 * mIndex.x;
  localRect.miny = CWorld::gbChunkRect.miny - 16 * mIndex.y;
  localRect.maxx = CWorld::gbChunkRect.maxx + localRect.minx - CWorld::gbChunkRect.minx;
  localRect.maxy = CWorld::gbChunkRect.maxy + localRect.miny - CWorld::gbChunkRect.miny;

  if (localRect.minx < 0) {
    localRect.minx = 0;
  }
  if (localRect.miny < 0) {
    localRect.miny = 0;
  }
  if (localRect.maxx >= 16) {
    localRect.maxx = 15;
  }
  if (localRect.maxy >= 16) {
    localRect.maxy = 15;
  }
}

void CMapArea::Create(BYTE *data) {
  FATALASSERT(data);
  FATALASSERT(CMap::bActive);

  SIffChunk *mIffChunk = (SIffChunk *)data;
  FATALASSERT(mIffChunk->token=='MHDR');

  data += sizeof(SIffChunk);

  mIffChunk = (SIffChunk *)(data + ((SMAreaHeader *)data)->offsInfo);
  FATALASSERT(mIffChunk->token == 'MCIN');
  memcpy(chunkInfo, mIffChunk + 1, mIffChunk->size);

  mIffChunk = (SIffChunk *)(data + ((SMAreaHeader *)data)->offsTex);
  FATALASSERT(mIffChunk->token == 'MTEX');
  char *mTexNames = (char *)(mIffChunk + 1);
  LoadTextures(mTexNames, mIffChunk->size);

  mIffChunk = (SIffChunk *)(data + ((SMAreaHeader *)data)->offsDoo);
  FATALASSERT(mIffChunk->token == 'MDDF');
  doodadDefList.SetCount(mIffChunk->size / sizeof(SMDoodadDef));
  if (doodadDefList.Count()) {
    SMDoodadDef *mDoodadDef = doodadDefList.Ptr();
    memcpy(mDoodadDef, mIffChunk + 1, mIffChunk->size);
  }

  mIffChunk = (SIffChunk *)(data + ((SMAreaHeader *)data)->offsMob);
  FATALASSERT(mIffChunk->token == 'MODF');
  mapObjDefList.SetCount(mIffChunk->size / sizeof(SMMapObjDef));
  if (mapObjDefList.Count()) {
    SMMapObjDef *mMapObjDef = mapObjDefList.Ptr();
    memcpy(mMapObjDef, mIffChunk + 1, mIffChunk->size);
  }

  CMap::areaTable[infoIndex] = this;
  CMap::areaInfo[infoIndex].asyncId = 0;
}

void CMapArea::LoadTextures(char *texNames, DWORD size) {
  FATALASSERT(texNames);

  texIdTable.SetCount(0);
  char *fileNames = texNames;
  UINT  i = 0;
  while (i < size) {
    HTEXTURE hTexture;

    if (CMap::EnableSpecularTerrain()) {
      const char *specExt = "_s";
      char        specFileName[MAX_PATH];

      FATALASSERT(SStrLen(&fileNames[i]) + SStrLen(specExt) < 260);
      SStrCopy(specFileName, &fileNames[i], 0x7FFFFFFF);
      char *dot = SStrChrR(specFileName, '.');
      FATALASSERT(dot);
      SStrCopy(dot, specExt, 0x7FFFFFFF);
      strcat(specFileName, &fileNames[i] + (dot - specFileName));
      hTexture = CMap::LoadTexture(specFileName);
    } else {
      hTexture = CMap::LoadTexture(&texNames[i]);
    }

    texIdTable.SetCount(texIdTable.Count() + 1);
    texIdTable[texIdTable.Count() - 1] = hTexture;
    ++texCount;
    while (texNames[i]) {
      ++i;
    }
    ++i;
  }
}

void CMapArea::AsyncCallback(LPVOID userArg) {
  CMapArea *area = (CMapArea *)userArg;
  FATALASSERT(area);

  area->Create((BYTE *)area->asyncObject->buffer);
  FreeAsyncLoadBuffer((BYTE *)area->asyncObject->buffer);
  area->asyncObject->buffer = 0;
  AsyncFileReadDestroyObject(area->asyncObject);
  area->asyncObject = 0;
}
