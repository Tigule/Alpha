#include <string.h>
#include <storm.h>
#include <stpl.h>

extern "C" {
#include "lua.h"

#include "lmem.h"
#include "lstate.h"
}

#define NUMPOOLS	3
#define CHUNKSIZE	0x40000

static const unsigned long s_blockSizes[NUMPOOLS] = { 36, 72, 160 };

class MemChunk {
  public:
    void *m_data;
    void *m_freeList;
    unsigned long m_size;
    unsigned long m_blockSize;
    unsigned long m_numBlocks;

    MemChunk(unsigned long blockSize, unsigned long chunkSize) {
      m_blockSize = blockSize;
      m_numBlocks = chunkSize / blockSize;
      m_size = m_numBlocks * m_blockSize;
      m_data = ALLOC(m_size);
      m_freeList = m_data;
      char *block;
      for (block = (char *)m_data; block < (char *)m_data + m_size - m_blockSize; block += m_blockSize)
        *(void **)block = block + m_blockSize;
      *(void **)block = NULL;
    }

    bool Contains(void *ptr) {
      return ptr >= m_data && ptr < (char *)m_data + m_size;
    }

    void *MemAlloc() {
      void *mem = NULL;
      if (m_freeList) {
        mem = m_freeList;
        m_freeList = *(void **)m_freeList;
        --m_numBlocks;
      }
      return mem;
    }

    void MemFree(void *ptr) {
      *(void **)ptr = m_freeList;
      m_freeList = ptr;
      ++m_numBlocks;
    }

    unsigned long NumBlocks() {
      return m_numBlocks;
    }
};

class MemPool {
  public:
    TSFixedArray<MemChunk *> m_memchunks;
    unsigned long m_chunkSize;
    unsigned long m_blockSize;
    unsigned long m_blocksPerChunk;

    MemPool() {
    }

    void Init(unsigned long blockSize, unsigned long chunkSize) {
      ASSERT(m_memchunks.Count() == 0);
      m_blockSize = blockSize;
      m_blocksPerChunk = chunkSize / blockSize;
      m_chunkSize = m_blocksPerChunk * m_blockSize;
    }

    unsigned long BlockSize() {
      return m_blockSize;
    }

    unsigned long NumBlocks() {
      unsigned long numBlocks = 0;
      for (unsigned int i = m_memchunks.Count(); i--; )
        numBlocks += m_memchunks[i]->NumBlocks();
      return numBlocks;
    }

    BOOL Contains(void *ptr) {
      unsigned int count = m_memchunks.Count();
      for (unsigned int i = 0; i < count; i++)
        if (m_memchunks[i]->Contains(ptr))
          return TRUE;
      return FALSE;
    }

    void *MemAlloc() {
      unsigned int count = m_memchunks.Count();
      for (unsigned int i = 0; i < count; i++) {
        void *mem = m_memchunks[i]->MemAlloc();
        if (mem)
          return mem;
      }
      m_memchunks.SetCount(count + 1);
      m_memchunks[count] = NEW(MemChunk)(m_blockSize, m_chunkSize);
      return m_memchunks[count]->MemAlloc();
    }

    BOOL MemFree(void *ptr) {
      unsigned int count = m_memchunks.Count();
      for (unsigned int i = 0; i < count; i++) {
        if (m_memchunks[i]->Contains(ptr)) {
          m_memchunks[i]->MemFree(ptr);
          return TRUE;
        }
      }
      return FALSE;
    }
};

void luaM_adjustGC (lua_State *L) {
  MemPool **pools = (MemPool **)G(L)->mempools;
  lu_mem freemem = G(L)->GCthreshold - G(L)->nblocks;
  int i;
  for (i = 0; i < NUMPOOLS; i++) {
    lu_mem poolfree = pools[i]->NumBlocks() * pools[i]->BlockSize();
    if (poolfree < freemem)
      freemem = poolfree;
  }
  if (freemem < G(L)->nblocks / 4)
    freemem = G(L)->nblocks / 4;
  G(L)->GCthreshold = G(L)->nblocks + freemem;
}

void *luaM_reallocPool (lua_State *L, void *block, lu_mem size) {
  if (!L)
    return SMemReAlloc(block, size, __FILE__, __LINE__, 0);

  MemPool **pools = (MemPool **)G(L)->mempools;
  if (!pools) {
    pools = (MemPool **)ALLOC(NUMPOOLS * sizeof(MemPool *));
    for (int i = 0; i < NUMPOOLS; i++) {
      pools[i] = NEW(MemPool);
      pools[i]->Init(s_blockSizes[i], CHUNKSIZE);
    }
    G(L)->mempools = pools;
  }

  if (size == 0) {
    if (block) {
      for (int i = 0; i < NUMPOOLS; i++)
        if (pools[i]->MemFree(block))
          return NULL;
      FREE(block);
    }
    return NULL;
  }

  if (block) {
    for (int i = 0; i < NUMPOOLS; i++) {
      if (pools[i]->Contains(block)) {
        lu_mem oldsize = pools[i]->BlockSize();
        if (size <= oldsize)
          return block;
        void *oldblock = block;
        for (int j = i + 1; j < NUMPOOLS; j++) {
          if (size <= pools[j]->BlockSize()) {
            block = pools[j]->MemAlloc();
            memcpy(block, oldblock, oldsize);
            pools[i]->MemFree(oldblock);
            return block;
          }
        }
        block = ALLOC(size);
        memcpy(block, oldblock, oldsize);
        pools[i]->MemFree(oldblock);
        return block;
      }
    }
    return SMemReAlloc(block, size, __FILE__, __LINE__, 0);
  }

  for (int i = 0; i < NUMPOOLS; i++)
    if (size <= pools[i]->BlockSize())
      return pools[i]->MemAlloc();
  return ALLOC(size);
}
