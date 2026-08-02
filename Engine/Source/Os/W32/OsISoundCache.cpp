#include <new>

#include "OsISoundCache.h"

#include <string.h>

enum {
  MAX_FILES = 256,
  MAX_FILE_INSTANCES = 16,
  CACHE_BLOCK_SIZE = 4096,
  BIG_FILE_SIZE = 0x40000
};

static TSHashTableReuse<SoundFileObjectCacheNode, HASHKEY_NONE, 1>    s_soundFileObjectHashTable;
static TSFixedArray<SoundFileObject>                                  s_soundFileObjects;
static TSHashTableReuse<SoundFileDataCacheBlock, HASHKEY_LONGLONG, 1> s_soundFileDataCache;
static LISTDECLEX(SoundFileDataCacheBlock, link, s_soundFileDataCacheLRU);
static LISTDECLEX(SoundFileObject, link, s_freeSoundFileObjects);
static SCritSect                                                      s_soundFileCacheLock;
static unsigned int                                                   s_openRequests;
static unsigned int                                                   s_openPhysicalFile;
static unsigned int                                                   s_reopenPhysicalFile;
static unsigned int                                                   s_openNoFile;
static unsigned int                                                   s_openNoObjects;
static unsigned int                                                   s_openNoInstances;
static unsigned int                                                   s_readRequests;
static unsigned int                                                   s_readPhysicalFile;
static unsigned int                                                   s_closeRequests;
static unsigned int                                                   s_closePhysicalFile;

SoundFileDataCacheBlock *AllocCacheBlock(__int64 hashKey);
void DataCacheInitialize(int cacheSizeMB);
void DataCacheShutdown();

void DataCacheInitialize(int cacheSizeMB) {
  unsigned int numCacheBlocks;
  unsigned int i;

  if (cacheSizeMB < 1) {
    cacheSizeMB = 1;
  } else if (cacheSizeMB > 16) {
    cacheSizeMB = 16;
  }

  numCacheBlocks = (cacheSizeMB << 20) / CACHE_BLOCK_SIZE;
  s_soundFileDataCache.SetTableSize(numCacheBlocks);
  for (i = 0; i < numCacheBlocks; ++i) {
    s_soundFileDataCacheLRU.LinkNode(s_soundFileDataCache.New(i, HASHKEY_LONGLONG(static_cast<__int64>(i)), 0, 0), LIST_TAIL, 0);
  }
  s_soundFileDataCache.Clear();
}

void DataCacheShutdown() {
  s_soundFileDataCacheLRU.UnlinkAll();
  s_soundFileDataCache.Clear();
}

SoundFileDataCacheBlock *AllocCacheBlock(__int64 hashKey) {
  SoundFileDataCacheBlock *cacheBlock = s_soundFileDataCacheLRU.Head();
  s_soundFileDataCacheLRU.UnlinkNode(cacheBlock);

  s_soundFileDataCache.Unlink(cacheBlock);

  s_soundFileDataCache.Insert(cacheBlock, static_cast<unsigned int>(hashKey), HASHKEY_LONGLONG(hashKey));
  s_soundFileDataCacheLRU.LinkNode(cacheBlock, LIST_TAIL, 0);
  return cacheBlock;
}

unsigned int __stdcall SoundFileCache::Open(const char *filename) {
  unsigned int hash;
  SFile       *file;

  s_soundFileCacheLock.Enter();
  ++s_openRequests;

  if (!filename || !*filename) {
    s_soundFileCacheLock.Leave();
    return 0;
  }

  hash = SStrHashHT(filename);
  SoundFileObjectCacheNode *cacheNode = s_soundFileObjectHashTable.Ptr(hash, HASHKEY_NONE());
  SoundFileObject          *object;

  if (cacheNode) {
    object = cacheNode->object;
    if (object->openInstances == MAX_FILE_INSTANCES) {
      ++s_openNoInstances;
      s_soundFileCacheLock.Leave();
      return 0;
    }

    if (!object->openInstances) {
      s_freeSoundFileObjects.UnlinkNode(object);
      if (object->size > BIG_FILE_SIZE) {
        SFile::Open(object->filename, &object->file);
        ASSERT(object->file);
        ++s_reopenPhysicalFile;
      }
    }

    unsigned int i;
    for (i = 0; i < MAX_FILE_INSTANCES && object->instances[i].inUse; ++i) {
    }
    ASSERT(i < MAX_FILE_INSTANCES);

    object->instances[i].currentOffset = 0;
    object->instances[i].inUse = 1;
    ++object->openInstances;

    unsigned int handle = (object->baseHandle << 8) | static_cast<unsigned char>(i + 1);
    s_soundFileCacheLock.Leave();
    return handle;
  }

  SFile::Open(filename, &file);
  if (!file) {
    ++s_openNoFile;
    s_soundFileCacheLock.Leave();
    return 0;
  }

  object = s_freeSoundFileObjects.Head();
  if (!object) {
    ++s_openNoObjects;
    s_soundFileCacheLock.Leave();
    return 0;
  }
  s_freeSoundFileObjects.UnlinkNode(object);

  if (object->filename[0]) {
    s_soundFileObjectHashTable.Delete(object->hash, HASHKEY_NONE());
  }

  object->hash = hash;
  SStrCopy(object->filename, filename, 0x7FFFFFFF);
  object->file = file;
  object->size = SFile::GetFileSize(file, 0);
  ++s_openPhysicalFile;
  memset(object->instances, 0, sizeof(object->instances));
  object->openInstances = 1;
  object->instances[0].inUse = 1;
  object->instances[0].currentOffset = 0;

  cacheNode = s_soundFileObjectHashTable.New(hash, HASHKEY_NONE(), 0, 0);
  cacheNode->object = object;

  unsigned int handle = (object->baseHandle << 8) | 1;
  s_soundFileCacheLock.Leave();
  return handle;
}

int __stdcall SoundFileCache::Read(void *buffer, int size, unsigned int handle) {
  __int64          hashKey;
  unsigned int     blockStartOffset;
  int              bytesReadFromCache;
  unsigned int     instanceNumber;
  SoundFileObject *object;
  unsigned int     needToRead;
  unsigned int     physicalReadDone;
  unsigned int     bigFile;
  unsigned int     baseHandle;
  int              currentOffset;

  s_soundFileCacheLock.Enter();
  ++s_readRequests;

  baseHandle = handle >> 8;
  ASSERT(baseHandle < MAX_FILES);

  instanceNumber = static_cast<unsigned char>(handle) - 1;
  ASSERT(instanceNumber < MAX_FILE_INSTANCES);

  object = &s_soundFileObjects[baseHandle];
  ASSERT(object);
  ASSERT(object->instances[instanceNumber].inUse);

  bigFile = object->size > BIG_FILE_SIZE;
  bytesReadFromCache = 0;
  physicalReadDone = 0;
  currentOffset = object->instances[instanceNumber].currentOffset;

  if (currentOffset >= object->size) {
    size = 0;
  }
  if (currentOffset + size > object->size) {
    size = object->size - currentOffset;
  }

  while (size) {
    blockStartOffset = object->instances[instanceNumber].currentOffset & ~(CACHE_BLOCK_SIZE - 1);
    hashKey = (static_cast<__int64>(object->hash) << 32) | (bigFile ? 0 : blockStartOffset);

    SoundFileDataCacheBlock *cacheBlock = s_soundFileDataCache.Ptr(bigFile ? 0 : blockStartOffset, HASHKEY_LONGLONG(hashKey));
    needToRead = cacheBlock == 0;
    if (!cacheBlock) {
      cacheBlock = AllocCacheBlock(hashKey);
    }

    if (needToRead || (bigFile && blockStartOffset != object->bigFileCacheBlockOffset)) {
      if (!object->file) {
        SFile::Open(object->filename, &object->file);
        ASSERT(object->file);
        ++s_reopenPhysicalFile;
      }

      SFile::SetFilePointer(object->file, blockStartOffset, 0, 0);
      SFile::Read(object->file, cacheBlock->data, CACHE_BLOCK_SIZE, 0, 0, 0);
      physicalReadDone = 1;
      object->bigFileCacheBlockOffset = blockStartOffset;
    }

    unsigned int offsetInBlock = object->instances[instanceNumber].currentOffset & (CACHE_BLOCK_SIZE - 1);
    unsigned int readSize = CACHE_BLOCK_SIZE - offsetInBlock;
    if (readSize >= static_cast<unsigned int>(size)) {
      readSize = size;
    }

    memcpy(static_cast<unsigned char *>(buffer) + bytesReadFromCache, cacheBlock->data + offsetInBlock, readSize);
    object->instances[instanceNumber].currentOffset += readSize;
    size -= readSize;
    bytesReadFromCache += readSize;
  }

  if (physicalReadDone) {
    ++s_readPhysicalFile;
  }

  s_soundFileCacheLock.Leave();
  return bytesReadFromCache;
}

int __stdcall SoundFileCache::Seek(unsigned int handle, int pos, signed char mode) {
  s_soundFileCacheLock.Enter();

  unsigned int baseHandle = handle >> 8;
  ASSERT(baseHandle < MAX_FILES);

  unsigned int instanceNumber = static_cast<unsigned char>(handle) - 1;
  ASSERT(instanceNumber < MAX_FILE_INSTANCES);

  SoundFileObject *object = &s_soundFileObjects[baseHandle];
  ASSERT(object);
  ASSERT(object->instances[instanceNumber].inUse);

  switch (mode) {
    case 0:
      object->instances[instanceNumber].currentOffset = pos;
      break;
    case 1:
      object->instances[instanceNumber].currentOffset += pos;
      break;
    case 2:
      object->instances[instanceNumber].currentOffset = object->size + pos;
      break;
    default:
      s_soundFileCacheLock.Leave();
      return -1;
  }

  s_soundFileCacheLock.Leave();
  return object->instances[instanceNumber].currentOffset;
}

int __stdcall SoundFileCache::Tell(unsigned int handle) {
  s_soundFileCacheLock.Enter();

  unsigned int baseHandle = handle >> 8;
  ASSERT(baseHandle < MAX_FILES);

  unsigned int instanceNumber = static_cast<unsigned char>(handle) - 1;
  ASSERT(instanceNumber < MAX_FILE_INSTANCES);

  SoundFileObject *object = &s_soundFileObjects[baseHandle];
  ASSERT(object);
  ASSERT(object->instances[instanceNumber].inUse);

  int currentOffset = object->instances[instanceNumber].currentOffset;
  s_soundFileCacheLock.Leave();
  return currentOffset;
}

void __stdcall SoundFileCache::Close(unsigned int handle) {
  s_soundFileCacheLock.Enter();
  ++s_closeRequests;

  unsigned int baseHandle = handle >> 8;
  ASSERT(baseHandle < MAX_FILES);

  unsigned int instanceNumber = static_cast<unsigned char>(handle) - 1;
  ASSERT(instanceNumber < MAX_FILE_INSTANCES);

  SoundFileObject *object = &s_soundFileObjects[baseHandle];
  ASSERT(object);
  ASSERT(object->instances[instanceNumber].inUse);

  object->instances[instanceNumber].inUse = 0;
  if (!--object->openInstances) {
    if (object->file) {
      SFile::Close(object->file);
      object->file = 0;
      ++s_closePhysicalFile;
    }
    s_freeSoundFileObjects.LinkNode(object, LIST_TAIL, 0);
  }

  s_soundFileCacheLock.Leave();
}

void SoundFileCache::Initialize(int cacheSizeMB) {
  DataCacheInitialize(cacheSizeMB);

  s_soundFileObjects.SetCount(MAX_FILES);
  for (unsigned int i = 0; i < MAX_FILES; ++i) {
    SoundFileObject *object = &s_soundFileObjects[i];
    object->filename[0] = 0;
    object->baseHandle = i;
    s_freeSoundFileObjects.LinkNode(object, LIST_TAIL, 0);
  }

  s_soundFileObjectHashTable.SetTableSize(MAX_FILES);
  for (unsigned int j = 0; j < MAX_FILES; ++j) {
    s_soundFileObjectHashTable.New(j, HASHKEY_NONE(), 0, 0);
  }
  s_soundFileObjectHashTable.Clear();
}

void SoundFileCache::Shutdown() {
  s_freeSoundFileObjects.UnlinkAll();
  s_soundFileObjectHashTable.Clear();
  s_soundFileObjects.Clear();
  DataCacheShutdown();
}
