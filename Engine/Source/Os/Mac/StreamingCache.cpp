#include <Base/Base.h>

#include <storm.h>

#include "Os/Mac/GlobalAUGraph.h"
#include "Os/Mac/StreamingCache.h"
#include "Os/OsTime.h"

#include <stdlib.h>

StreamingCache *g_streamingCache;

struct ExpireData {
  CacheChunk *chunk;
  DWORD       lastAccess;
};

static int ExpireCompare(const void *a, const void *b) {
  LONG delta = (LONG)(((const ExpireData *)a)->lastAccess - ((const ExpireData *)b)->lastAccess);
  return delta < 0 ? -1 : delta > 0 ? 1 : 0;
}

StreamingCache::StreamingCache()
    : m_totalBytes(0),
      m_nextExpire(0) {
  pthread_mutex_init(&m_mutex, 0);
}

StreamingCache::~StreamingCache() {
  pthread_mutex_lock(&m_mutex);
  Expire(0);
  m_items.Clear();
  pthread_mutex_unlock(&m_mutex);
  pthread_mutex_destroy(&m_mutex);
}

void StreamingCache::SetCacheParameters(CacheData *cacheItem, UINT streambase, UINT streamsize, UINT chunksize) {
  ASSERT(cacheItem != 0);

  pthread_mutex_lock(&m_mutex);

  if (!cacheItem->chunks) {
    if (streambase == 0 && streamsize == 0) {
      streamsize = cacheItem->fileSize;
    }
    cacheItem->streamBase = streambase;
    cacheItem->streamSize = streamsize;
    cacheItem->chunkSize = chunksize;
    cacheItem->chunkCount = (streamsize + chunksize - 1) / chunksize;
    cacheItem->chunks = (CacheChunk *)SMemAlloc(cacheItem->chunkCount * sizeof(CacheChunk) + 1, __FILE__, __LINE__, SMEM_FLAG_ZEROMEMORY);
  } else {
    ASSERTVALUE(cacheItem->chunkSize == chunksize, chunksize);
    ASSERTVALUE(cacheItem->streamBase == streambase, streambase);
    ASSERTVALUE(cacheItem->streamSize == streamsize, streamsize);
  }

  pthread_mutex_unlock(&m_mutex);
}

UINT StreamingCache::GetTotalChunks(CacheData *cacheItem) {
  ASSERT(cacheItem != 0);
  return cacheItem->chunkCount;
}

void StreamingCache::Release(CacheData *cacheItem) {
  ASSERT(cacheItem != 0);
  CheckIn(&cacheItem->acquireCount);
}

CacheData *StreamingCache::Acquire(const char *name, UINT *headerSize, void **header, int readSize) {
  pthread_mutex_lock(&m_mutex);

  CacheData *cacheItem = m_items.Ptr(name);
  if (!cacheItem) {
    HSFILE file;
    if (!SFileOpenFile(name, &file)) {
      pthread_mutex_unlock(&m_mutex);
      return 0;
    }

    cacheItem = m_items.New(name, 0, 0);
    cacheItem->file = file;
    cacheItem->acquireCount = 0;
    cacheItem->chunks = 0;
    cacheItem->header = 0;
    cacheItem->filePosition = 0;
    cacheItem->streamBase = 0;
    cacheItem->streamSize = 0;
    cacheItem->chunkCount = 0;
    cacheItem->chunkSize = 0;
    cacheItem->fileSize = SFileGetFileSize(file, 0);
    cacheItem->headerSize = cacheItem->fileSize < (UINT)readSize ? cacheItem->fileSize : readSize;
    cacheItem->header = SMemAlloc(cacheItem->headerSize + 1, __FILE__, __LINE__, 0);

    DWORD actualRead = 0;
    BOOL  success = SFileReadFile(file, cacheItem->header, cacheItem->headerSize, &actualRead, 0);
    ASSERT(success);
    cacheItem->filePosition = cacheItem->headerSize;
  }

  CheckOut(&cacheItem->acquireCount);
  *headerSize = cacheItem->headerSize;
  *header = cacheItem->header;

  pthread_mutex_unlock(&m_mutex);
  return cacheItem;
}

bool StreamingCache::Load(CacheData *cacheItem, int chunkIndex, LONG **count) {
  ASSERT(cacheItem != 0);
  ASSERTVALUE(chunkIndex < cacheItem->chunkCount, chunkIndex);

  CacheChunk *chunk = &cacheItem->chunks[chunkIndex];

  if (!chunk->data) {
    UINT offset = chunkIndex * cacheItem->chunkSize;
    UINT position = cacheItem->streamBase + offset;
    if (cacheItem->filePosition != position) {
      SFileSetFilePointer(cacheItem->file, position, 0, 0);
    }

    UINT blockSize = cacheItem->streamSize - offset;
    if (blockSize > cacheItem->chunkSize) {
      blockSize = cacheItem->chunkSize;
    }

    void *data = SMemAlloc(blockSize, __FILE__, __LINE__, 0);
    DWORD actualRead = 0;
    BOOL  success = SFileReadFile(cacheItem->file, data, blockSize, &actualRead, 0);
    ASSERT(success);
    ASSERT(actualRead == blockSize);

    cacheItem->filePosition = position + blockSize;
    m_totalBytes += actualRead;

    CheckOut(&chunk->checkOuts);
    *count = &chunk->checkOuts;

    pthread_mutex_lock(&m_mutex);
    Expire(0x200000);
    chunk->size = actualRead;
    chunk->data = data;
    chunk->lastAccess = OsGetAsyncTimeMsPrecise();
    pthread_mutex_unlock(&m_mutex);
  } else {
    CheckOut(&chunk->checkOuts);
    *count = &chunk->checkOuts;
    chunk->lastAccess = OsGetAsyncTimeMsPrecise();
  }

  return true;
}

bool StreamingCache::Stream(CacheData *cacheItem, int chunkIndex, void **data, UINT *size, LONG **count) {
  CacheChunk *chunk = &cacheItem->chunks[chunkIndex];

  CheckOut(&chunk->checkOuts);

  if (!chunk->data) {
    CheckIn(&chunk->checkOuts);
    return false;
  }

  *data = chunk->data;
  *size = chunk->size;
  *count = &chunk->checkOuts;
  chunk->lastAccess = OsGetAsyncTimeMs();
  return true;
}

void StreamingCache::Expire(UINT maxBytes) {
  if (m_totalBytes <= maxBytes) {
    return;
  }

  DWORD now = OsGetAsyncTimeMsPrecise();
  if (m_nextExpire > now) {
    return;
  }
  m_nextExpire = now + 1000;

  TSGrowableArray<ExpireData> expire;
  CacheData                  *next;

  for (CacheData *item = m_items.Head(); item; item = next) {
    next = m_items.Next(item);
    bool loaded = false;
    for (UINT i = 0; i < item->chunkCount; ++i) {
      CacheChunk *chunk = &item->chunks[i];
      if (chunk->data) {
        loaded = true;
        if (!SInterlockedExchangeAdd(&chunk->checkOuts, 0)) {
          ExpireData *entry = expire.New();
          entry->chunk = chunk;
          entry->lastAccess = chunk->lastAccess;
        }
      }
    }
    if (!loaded && !SInterlockedExchangeAdd(&item->acquireCount, 0)) {
      m_items.Delete(item);
    }
  }

  qsort(expire.Ptr(), expire.Count(), sizeof(ExpireData), ExpireCompare);

  for (UINT i = 0; i < expire.Count(); ++i) {
    CacheChunk *chunk = expire[i].chunk;
    void       *data = SInterlockedExchangePointer((LPVOID *)&chunk->data, 0);
    if (SInterlockedExchangeAdd(&chunk->checkOuts, 0)) {
      chunk->data = data;
      continue;
    }
    m_totalBytes -= chunk->size;
    SMemFree(data, __FILE__, __LINE__, 0);
    chunk->size = 0;
    if (m_totalBytes <= maxBytes) {
      break;
    }
  }
}
