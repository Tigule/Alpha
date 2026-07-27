#include "World.h"
#include "WorldClient/CMapObj.h"

#include "Services/AsyncFileRead.h"
#include "Services/Texture.h"

static SCritSect                        s_fileCritSect;
static TSCArray<unsigned char, 163840>  s_asyncLoadBuffers[4];
static TSExplicitList<CAsyncObject, 32> s_asyncLoadList;
static unsigned int                    *s_freeAsyncBuffer;
static unsigned int                     s_asyncBuffersInitialized;

void CMapArea::FreeAsyncLoadBuffer(unsigned int *buffer) {
  *reinterpret_cast<unsigned int **>(buffer) = s_freeAsyncBuffer;
  s_freeAsyncBuffer = buffer;
}

void CMapArea::InitAsyncLoadBuffers() {
  for (unsigned int index = 0; index < 4; ++index) {
    FreeAsyncLoadBuffer(reinterpret_cast<unsigned int *>(s_asyncLoadBuffers[index].Ptr()));
  }
}

unsigned int *CMapArea::AllocAsyncLoadBuffer() {
  if (!s_asyncBuffersInitialized) {
    InitAsyncLoadBuffers();
    s_asyncBuffersInitialized = 1;
  }

  unsigned int *buffer = s_freeAsyncBuffer;
  if (buffer) {
    s_freeAsyncBuffer = *reinterpret_cast<unsigned int **>(buffer);
  }

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
    unsigned int *buffer = AllocAsyncLoadBuffer();
    if (!buffer) {
      break;
    }

    CAsyncObject *next = s_asyncLoadList.Next(object);
    object->link.Unlink();
    object->buffer = buffer;
    object->canReorder = 1;
    AsyncFileReadObject(object);
    object = next;
  }
}

CMapArea::CMapArea() {
  infoIndex = 0;
  asyncObject = 0;
  for (unsigned int i = 0; i < 256; ++i) {
    chunkInfo[i].offset = 0;
    chunkInfo[i].size = 0;
    chunkInfo[i].flags = 0;
    chunkInfo[i].asyncId = 0;
    chunkTable[i] = 0;
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
    SFile::Read(CMap::wdtFile, s_asyncLoadBuffers[0].Ptr(), areaInfo->size, 0, 0, 0);
    Create(reinterpret_cast<unsigned int *>(s_asyncLoadBuffers[0].Ptr()));
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

    areaInfo->asyncId = reinterpret_cast<unsigned int>(asyncObject);
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

void CMapArea::Create(unsigned int *data) {
  FATALASSERT(data);
  FATALASSERT(CMap::bActive);

  SIffChunk *mIffChunk = reinterpret_cast<SIffChunk *>(data);
  FATALASSERT(mIffChunk->token == 'MHDR');

  unsigned int *areaData = data + 2;

  mIffChunk = reinterpret_cast<SIffChunk *>(reinterpret_cast<unsigned char *>(areaData) + reinterpret_cast<SMAreaHeader *>(areaData)->offsInfo);
  FATALASSERT(mIffChunk->token == 'MCIN');
  memcpy(chunkInfo, mIffChunk + 1, mIffChunk->size);

  mIffChunk = reinterpret_cast<SIffChunk *>(reinterpret_cast<unsigned char *>(areaData) + reinterpret_cast<SMAreaHeader *>(areaData)->offsTex);
  FATALASSERT(mIffChunk->token == 'MTEX');
  char *mTexNames = reinterpret_cast<char *>(mIffChunk + 1);
  LoadTextures(mTexNames, mIffChunk->size);

  mIffChunk = reinterpret_cast<SIffChunk *>(reinterpret_cast<unsigned char *>(areaData) + reinterpret_cast<SMAreaHeader *>(areaData)->offsDoo);
  FATALASSERT(mIffChunk->token == 'MDDF');
  doodadDefList.SetCount(mIffChunk->size / sizeof(SMDoodadDef));
  if (doodadDefList.Count()) {
    SMDoodadDef *mDoodadDef = &doodadDefList[0];
    memcpy(mDoodadDef, mIffChunk + 1, mIffChunk->size);
  }

  mIffChunk = reinterpret_cast<SIffChunk *>(reinterpret_cast<unsigned char *>(areaData) + reinterpret_cast<SMAreaHeader *>(areaData)->offsMob);
  FATALASSERT(mIffChunk->token == 'MODF');
  mapObjDefList.SetCount(mIffChunk->size / sizeof(SMMapObjDef));
  if (mapObjDefList.Count()) {
    SMMapObjDef *mMapObjDef = &mapObjDefList[0];
    memcpy(mMapObjDef, mIffChunk + 1, mIffChunk->size);
  }

  CMap::areaTable[infoIndex] = this;
  CMap::areaInfo[infoIndex].asyncId = 0;
}

void CMapArea::LoadTextures(char *texNames, unsigned long size) {
  FATALASSERT(texNames);

  texIdTable.SetCount(0);
  unsigned int i = 0;
  while (i < size) {
    HTEXTURE hTexture;

    if (CMap::EnableSpecularTerrain()) {
      static const char specExt[] = "_s";
      char              specFileName[260];

      FATALASSERT(SStrLen(&texNames[i]) + SStrLen(specExt) < 260);
      SStrCopy(specFileName, &texNames[i], 0x7FFFFFFF);
      char *extension = SStrChrR(specFileName, '.');
      FATALASSERT(extension);
      SStrCopy(extension, specExt, 0x7FFFFFFF);
      strcat(specFileName, &texNames[i] + (extension - specFileName));
      hTexture = CMap::LoadTexture(specFileName);
    } else {
      hTexture = CMap::LoadTexture(&texNames[i]);
    }

    texIdTable.SetCount(texIdTable.Count() + 1);
    texIdTable[texIdTable.Count() - 1] = hTexture;
    ++texCount;
    i += SStrLen(&texNames[i]) + 1;
  }
}

void CMapArea::AsyncCallback(void *userArg) {
  CMapArea *area = static_cast<CMapArea *>(userArg);
  FATALASSERT(area);

  area->Create(static_cast<unsigned int *>(area->asyncObject->buffer));
  FreeAsyncLoadBuffer(static_cast<unsigned int *>(area->asyncObject->buffer));
  area->asyncObject->buffer = 0;
  AsyncFileReadDestroyObject(area->asyncObject);
  area->asyncObject = 0;
}

SMDoodadDef::~SMDoodadDef() {
}

SMMapObjDef::~SMMapObjDef() {
}
