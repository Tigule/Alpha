#pragma once

#include <pthread.h>

struct CacheChunk {
  void *volatile data;
  UINT          size;
  LONG          checkOuts;
  DWORD         lastAccess;
};

struct CacheData : public TSHashObject<CacheData, HASHKEY_STRI> {
  HSFILE      file;
  LONG        acquireCount;
  CacheChunk *chunks;
  void       *header;
  UINT        headerSize;
  UINT        filePosition;
  UINT        streamBase;
  UINT        streamSize;
  UINT        fileSize;
  UINT        chunkCount;
  UINT        chunkSize;

  ~CacheData() {
    if (header) {
      SMemFree(header, __FILE__, __LINE__, 0);
    }
    if (chunks) {
      SMemFree(chunks, __FILE__, __LINE__, 0);
    }
    if (file) {
      SFileCloseFile(file);
    }
  }
};

inline void CheckOut(LONG *count) {
  LONG acquireCountForCheckOut = SInterlockedIncrement(count);
  ASSERTVALUE(acquireCountForCheckOut > 0, acquireCountForCheckOut);
}

inline void CheckIn(LONG *count) {
  LONG acquireCountForCheckIn = SInterlockedDecrement(count);
  ASSERTVALUE(acquireCountForCheckIn >= 0, acquireCountForCheckIn);
}

class StreamingCache {
 public:
  StreamingCache();
  ~StreamingCache();
  CacheData *Acquire(const char *name, UINT *headerSize, void **header, int readSize);
  void       Release(CacheData *cacheItem);
  UINT       GetTotalChunks(CacheData *cacheItem);
  void       SetCacheParameters(CacheData *cacheItem, UINT streambase, UINT streamsize, UINT chunksize);
  bool       Load(CacheData *cacheItem, int chunkIndex, LONG **count);
  bool       Stream(CacheData *cacheItem, int chunkIndex, void **data, UINT *size, LONG **count);
  void       Expire(UINT maxBytes);

  TSHashTable<CacheData, HASHKEY_STRI> m_items;
  pthread_mutex_t                      m_mutex;
  UINT                                 m_totalBytes;
  DWORD                                m_nextExpire;
};

extern StreamingCache *g_streamingCache;
