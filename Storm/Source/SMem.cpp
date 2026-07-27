#include <storm.h>

#include <stdio.h>

#define FIRSTUSERHEAP 0x80000000
#define MAXALLOCSIZE  0xFE5F
#define MAXFREEMAINT  4
#define MAXHEAPSIZE   0x7FFFFFFF
#define MINBLOCKSIZE  (sizeof(BLOCK) + 2 * sizeof(DWORD))
#define PAGESIZE      0x1000
#define RESERVESIZE   0x10000
#define SIGNATURE1    0x6F6D
#define SIGNATURE2    0x12B1
#define TABLESIZE     256

#define REGKEY           "Internal"
#define REGVAL_DEBUG     "Debug Memory"
#define REGVAL_GUARD     "Protect Memory"
#define REGVAL_TRACEFILE "SMem Trace File"
// not original macro name
#define REGVAL_REALLOCSHUFFLE "Realloc Shuffle"
// not original macro name
#define REGVAL_DEBUGERROUTPUT "Debug Error Output"

#define SMEM_FLAG_REALLOC_IN_PLACE 0x00000010

#define BF_BOUNDINGSIG 0x01
#define BF_FREEBLOCK   0x02
#define BF_LARGEALLOC  0x04
#define BF_OUTSIDEHEAP 0x08
#define BF_PREVFREE    0x10
#define BF_PRESERVE    0x80

typedef struct _BASEBLOCK {
  WORD bytes;
  BYTE padding;
  BYTE flags;
} BASEBLOCK, *BASEBLOCKPTR;

typedef struct _BLOCK : public _BASEBLOCK {
  WORD heapaddr;
  WORD signature1;
} BLOCK, *BLOCKPTR;

typedef struct _FREEBLOCK : public _BASEBLOCK {
  _FREEBLOCK *next;
} FREEBLOCK, *FREEBLOCKPTR;

typedef struct _HEAP {
  _HEAP       *next;
  HSHEAP       handle;
  DWORD        slot;
  DWORD        addrsig;
  BOOL         active;
  DWORD        allocatedblocks;
  DWORD        allocatedbytes;
  BLOCKPTR     firstblock;
  BLOCKPTR     termblock;
  DWORD        uncombinedfree;
  DWORD        chunksize;
  DWORD        committedbytes;
  DWORD        reservedbytes;
  DWORD        externalbytes;
  DWORD        cumulativeAllocs;
  DWORD        cumulativeFrees;
  DWORD        cumulativeReallocs;
  DWORD        mark_externalbytes;
  DWORD        mark_allocatedblocks;
  DWORD        mark_allocatedbytes;
  DWORD        mark_committedbytes;
  DWORD        mark_cumulativeAllocs;
  DWORD        mark_cumulativeFrees;
  DWORD        mark_cumulativeReallocs;
  FREEBLOCKPTR firstfreeblock[9];
  int          linenumber;
  char         filename[1];
} HEAP, *HEAPPTR;

DECLARE_STRICT_HANDLE(HLOCKEDHEAP);

static int    lastline;
static LPCSTR lastptr;
static HSHEAP lasthandle;
static DWORD  lastchars;
static int    cacheenabled = TRUE;
static HSHEAP handle = (HSHEAP)FIRSTUSERHEAP;

static BOOL             s_emptyheap[TABLESIZE];
static CRITICAL_SECTION s_critsect[TABLESIZE];
static BOOL             s_debugmode;
static BOOL             s_fillmode;
static BOOL             s_guardmode;
static BOOL             s_reallocshufflemode;
static HEAPPTR          s_heaphead[TABLESIZE];
static BOOL             s_initialized;
static HEAPPTR          s_lastemptyheap;
static DWORD            s_pagesize;
static BOOL             s_warnings;
static DWORD            s_totalAllocated;
BOOL                    g_memFullError;
#define smemOptions g_opt

static BOOL CheckInitialized() {
  if (!s_initialized) {
    SLogInitialize();
    SMemInitialize();
  }

  return s_initialized;
}

static void FatalError(DWORD errorcode, LPCSTR filename, int linenumber) {
  g_memFullError = errorcode == ERROR_NOT_ENOUGH_MEMORY;
  SErrDisplayError(errorcode, filename, linenumber, NULL, FALSE, 1);
  ExitProcess(1);
}

static BLOCKPTR GetBlockPtrByPtr(LPVOID ptr) {
  if (!ptr) {
    return NULL;
  }

  BLOCKPTR blockptr = (BLOCKPTR)ptr - 1;
  if (blockptr->flags & BF_OUTSIDEHEAP) {
    blockptr = *(BLOCKPTR *)((LPBYTE)blockptr - sizeof(BLOCKPTR));
  }

  return blockptr;
}

static HSHEAP GetHandleByBlockPtr(BLOCKPTR blockptr) {
  HEAPPTR heapptr = (HEAPPTR)((DWORD)(blockptr->heapaddr) << 16);
  return heapptr->handle;
}

static HSHEAP GetHandleByCaller(LPCSTR filename, int linenumber) {
  DWORD  chars;
  HSHEAP handle;

  if (filename) {
    chars = *(DWORD *)filename;
  } else {
    chars = 0;
  }

  if (cacheenabled) {
    if (filename == lastptr && linenumber == lastline) {
      if (chars == lastchars) {
        return lasthandle;
      }

      cacheenabled = FALSE;
    }
  }

  if (filename) {
    handle = (HSHEAP)(SStrHash(filename, 1, linenumber) & 0x7FFFFFFF);
  } else {
    handle = (HSHEAP)(linenumber & 0x7FFFFFFF);
  }

  if (!handle) {
    handle = (HSHEAP)1;
  }

  lastline = linenumber;
  lastptr = filename;
  lastchars = chars;
  lasthandle = handle;

  return handle;
}

static LPVOID GetPtrByBlockPtr(BLOCKPTR blockptr) {
  if (!blockptr) {
    return NULL;
  }

  LPVOID ptr = blockptr + 1;
  if (blockptr->flags & BF_LARGEALLOC) {
    ptr = *(LPVOID *)ptr;
  }

  return ptr;
}

static DWORD GetSlotByHandle(HSHEAP handle) {
  return (DWORD)handle & (TABLESIZE - 1);
}

static HEAPPTR LockHeapByBlockPtr(BLOCKPTR blockptr, HLOCKEDHEAP *lockedhandle) {
  HEAPPTR heapptr = (HEAPPTR)((DWORD)blockptr->heapaddr << 16);

  EnterCriticalSection(&s_critsect[heapptr->slot]);
  *(DWORD *)lockedhandle = heapptr->slot;

  return heapptr;
}

static HEAPPTR LockHeapByHandle(HSHEAP handle, HLOCKEDHEAP *lockedhandle, BOOL heapmustexist) {
  DWORD   slot = GetSlotByHandle(handle);
  HEAPPTR heapptr;

  EnterCriticalSection(&s_critsect[slot]);
  *(DWORD *)lockedhandle = slot;

  for (heapptr = s_heaphead[slot]; heapptr; heapptr = heapptr->next) {
    if (heapptr->handle == handle) {
      return heapptr;
    }
  }

  if (heapmustexist) {
    LeaveCriticalSection(&s_critsect[slot]);
    *(DWORD *)lockedhandle = 0xFFFFFFFF;
  }

  return NULL;
}

static HEAPPTR LockNextHeapByHandle(HSHEAP prevheap, HLOCKEDHEAP *lockedhandle) {
  HSHEAP  lastheap;
  DWORD   slot;
  HEAPPTR heapptr;

  lastheap = NULL;
  slot = prevheap ? GetSlotByHandle(prevheap) : 0;
  while (slot < TABLESIZE) {
    EnterCriticalSection(&s_critsect[slot]);
    *(DWORD *)lockedhandle = slot;

    for (heapptr = s_heaphead[slot]; heapptr; heapptr = heapptr->next) {
      if (!heapptr->active) {
        continue;
      }
      if (lastheap == prevheap && heapptr->handle != prevheap) {
        return heapptr;
      }
      lastheap = heapptr->handle;
    }

    LeaveCriticalSection(&s_critsect[slot]);
    *(DWORD *)lockedhandle = 0xFFFFFFFF;
    slot++;
  }

  return NULL;
}

static void Warning(DWORD errorcode, LPCSTR filename, int linenumber) {
  SErrSetLastError(errorcode);
  if (s_warnings) {
    SErrDisplayError(errorcode, filename, linenumber, NULL, TRUE, 1);
  }
}

static void UnlockHeap(HLOCKEDHEAP *lockedhandle) {
  if (*(DWORD *)lockedhandle != -1) {
    DWORD slot = *(DWORD *)lockedhandle;
    LeaveCriticalSection(&s_critsect[slot]);

    *(DWORD *)lockedhandle = -1;
  }
}

// --------------------------------
// Block allocation/deallocation functions
// --------------------------------

static void CombineFreeBlocks(HEAPPTR heapptr);
static void ComputeBlockSize(DWORD bytes, LPDWORD blockSize, LPDWORD padding, LPBOOL largeAlloc, LPBOOL boundingSig);
static DWORD ComputeFreeSlot(DWORD bytes);
static void ComputePageSize();
static void FillBlockHeaderAndSignatures(HEAPPTR heapptr, BLOCKPTR blockptr, DWORD blockSize, DWORD padding, BYTE flags);
static void FreeHeap(HEAPPTR *nextptr);
static void FreeHeapBlock(HEAPPTR heapptr, BLOCKPTR block);
static int GrowCommitSize(HEAPPTR heapptr, DWORD newheapsize);
static BOOL GrowHeapBlock(HEAPPTR heapptr, BLOCKPTR blockptr, DWORD sourceBytes, DWORD bytes);
static void ShrinkHeapBlock(HEAPPTR heapptr, BLOCKPTR blockptr, DWORD sourceBytes, DWORD bytes);
static void SubdivideBlock(HEAPPTR heapptr, BLOCKPTR blockptr, LPDWORD blocksize, LPDWORD padding);

static HEAPPTR
AllocateHeap(LPCSTR filename, int linenumber, HSHEAP handle, DWORD slot, DWORD chunksize, DWORD commitsize, DWORD reservesize) {
  BLOCK    block;
  DWORD    filenamebytes;
  DWORD    headerbytes;
  HEAPPTR  heapptr;
  HEAPPTR  scan;
  HEAPPTR *insertAfter;

  FATALASSERT(slot == GetSlotByHandle(handle));

  heapptr = (HEAPPTR)VirtualAlloc(NULL, reservesize, MEM_RESERVE, PAGE_NOACCESS);
  if (!heapptr) {
    FatalError(ERROR_NOT_ENOUGH_MEMORY, filename, linenumber);
  }

  if (!VirtualAlloc(heapptr, commitsize, MEM_COMMIT, PAGE_READWRITE)) {
    FatalError(ERROR_NOT_ENOUGH_MEMORY, filename, linenumber);
  }

  filenamebytes = (filename ? SStrLen(filename) : 0) + 1;
  headerbytes = 0x8B + filenamebytes;
  if (headerbytes & 7) {
    headerbytes += 8 - (headerbytes & 7);
  }

  heapptr->handle = handle;
  heapptr->firstblock = (BLOCKPTR)((LPBYTE)heapptr + headerbytes);
  heapptr->termblock = heapptr->firstblock;
  heapptr->slot = slot;
  heapptr->chunksize = chunksize;
  heapptr->committedbytes = commitsize;
  heapptr->reservedbytes = reservesize;
  heapptr->linenumber = linenumber;
  heapptr->active = TRUE;
  heapptr->uncombinedfree = 0;
  ZeroMemory(heapptr->firstfreeblock, sizeof(heapptr->firstfreeblock));
  if (filename) {
    memcpy(heapptr->filename, filename, filenamebytes);
  } else {
    heapptr->filename[0] = 0;
  }
  block.heapaddr = (WORD)((DWORD)heapptr >> 16);
  block.signature1 = SIGNATURE1;
  heapptr->addrsig = *(DWORD *)&block.heapaddr;

  scan = s_heaphead[slot];
  if (!scan || scan->handle == handle) {
    heapptr->next = scan;
    s_heaphead[slot] = heapptr;
    return heapptr;
  }

  insertAfter = &scan->next;
  while (*insertAfter && (*insertAfter)->handle != handle) {
    insertAfter = &(*insertAfter)->next;
  }
  if (!*insertAfter) {
    heapptr->next = scan;
    s_heaphead[slot] = heapptr;
    return heapptr;
  }

  heapptr->next = *insertAfter;
  *insertAfter = heapptr;

  return heapptr;
}

static LPVOID AllocateHeapBlock(HEAPPTR heapptr, DWORD bytes, BYTE baseflags) {
  BOOL          boundingSig;
  DWORD         prevfree;
  BOOL          largeAlloc;
  DWORD         padding;
  FREEBLOCKPTR *bestfreeblock;
  DWORD         blocksize;
  DWORD         slot;
  FREEBLOCKPTR *link;
  FREEBLOCKPTR  scan;
  BLOCKPTR      blockptr;
  DWORD         bestdiff;
  DWORD         threshold;
  DWORD         newheapsize;

  ComputeBlockSize(bytes, &blocksize, &padding, &largeAlloc, &boundingSig);
  slot = ComputeFreeSlot(blocksize);

  if (heapptr->uncombinedfree >= MAXFREEMAINT && !heapptr->firstfreeblock[slot]) {
    CombineFreeBlocks(heapptr);
  }

  link = &heapptr->firstfreeblock[slot];
  while (!*link && ++slot < 9) {
    link = &heapptr->firstfreeblock[slot];
  }

  bestfreeblock = NULL;
  bestdiff = 0x7FFFFFFF;
  threshold = MINBLOCKSIZE;
  for (scan = *link; scan; scan = scan->next) {
    DWORD diff = scan->bytes - blocksize;
    if (diff < bestdiff) {
      bestdiff = diff;
      bestfreeblock = link;
      if (diff < threshold) {
        break;
      }
      threshold += sizeof(DWORD);
    }
    link = &scan->next;
  }

  prevfree = 0;
  if (bestfreeblock) {
    BLOCKPTR nextblock;

    blockptr = (BLOCKPTR)*bestfreeblock;
    *bestfreeblock = (*bestfreeblock)->next;
    prevfree = (blockptr->flags >> 4) & 1;
    nextblock = (BLOCKPTR)((LPBYTE)blockptr + blockptr->bytes);
    if (heapptr->uncombinedfree) {
      if (prevfree || (nextblock != heapptr->termblock && (nextblock->flags & BF_FREEBLOCK))) {
        heapptr->uncombinedfree--;
      }
    }

    SubdivideBlock(heapptr, blockptr, &blocksize, &padding);
  } else {
    blockptr = heapptr->termblock;
    newheapsize = (DWORD)((LPBYTE)heapptr->termblock - (LPBYTE)heapptr) + blocksize;
    if (newheapsize > heapptr->reservedbytes) {
      DWORD   reservesize = heapptr->reservedbytes < 0x10000000 ? heapptr->reservedbytes * 2 : heapptr->reservedbytes;
      DWORD   commitsize = reservesize >> 3;
      HEAPPTR newheap = AllocateHeap(heapptr->filename, heapptr->linenumber, heapptr->handle, heapptr->slot, commitsize, commitsize, reservesize);

      if (!newheap) {
        return NULL;
      }

      heapptr->active = FALSE;
      heapptr = newheap;
      blockptr = heapptr->termblock;
      newheapsize = (DWORD)((LPBYTE)blockptr - (LPBYTE)heapptr) + blocksize;
    }

    if (newheapsize > heapptr->committedbytes && !GrowCommitSize(heapptr, newheapsize)) {
      return NULL;
    }

    heapptr->termblock = (BLOCKPTR)((LPBYTE)heapptr->termblock + blocksize);
  }

  heapptr->allocatedblocks++;
  heapptr->allocatedbytes += bytes;

  FillBlockHeaderAndSignatures(
      heapptr, blockptr, blocksize, padding,
      baseflags | (prevfree ? BF_PREVFREE : 0) | (largeAlloc ? BF_LARGEALLOC : 0) | (boundingSig ? BF_BOUNDINGSIG : 0)
  );

  if (!largeAlloc) {
    return (LPBYTE)blockptr + sizeof(BLOCK);
  } else {
    DWORD  externalSize;
    DWORD  allocOffset;
    DWORD  allocSize;
    LPBYTE externalBase;
    LPBYTE allocBase;
    LPBYTE externalPtr;

    *(LPVOID *)((LPBYTE)blockptr + sizeof(BLOCK)) = NULL;
    if (!s_pagesize) {
      ComputePageSize();
    }

    externalSize = bytes + 0x10;
    allocOffset = 0;
    if (s_debugmode || s_guardmode) {
      DWORD pageMask = s_pagesize - 1;
      allocOffset = s_pagesize - (externalSize & pageMask);
      if (s_guardmode) {
        allocOffset &= pageMask;
      } else {
        allocOffset &= s_pagesize - 4;
      }
    }

    allocSize = allocOffset + externalSize;
    allocBase = NULL;
    if (s_guardmode) {
      allocBase = (LPBYTE)VirtualAlloc(NULL, allocSize + sizeof(DWORD), MEM_RESERVE, PAGE_NOACCESS);
    }

    allocBase = (LPBYTE)VirtualAlloc(allocBase, allocSize, MEM_COMMIT, PAGE_READWRITE);
    if (!allocBase) {
      FreeHeapBlock(heapptr, blockptr);
      return NULL;
    }

    externalBase = allocBase + allocOffset;

    *(DWORD *)(externalBase + 0) = bytes;
    *(BLOCKPTR *)(externalBase + 4) = blockptr;
    *(WORD *)(externalBase + 8) = (WORD)((bytes + 0xFFFF) >> 16);
    *(BYTE *)(externalBase + 0xA) = 0;
    *(BYTE *)(externalBase + 0xB) = BF_LARGEALLOC | BF_OUTSIDEHEAP;
    *(DWORD *)(externalBase + 0xC) = heapptr->addrsig;

    externalPtr = externalBase + 0x10;
    *(LPVOID *)((LPBYTE)blockptr + sizeof(BLOCK)) = externalPtr;
    heapptr->externalbytes += bytes;

    return externalPtr;
  }
}

static BOOL CheckValidBlock(LPVOID ptr, BOOL displayerror, LPCSTR filename, int linenumber) {
  BLOCKPTR blockptr;

  if (!ptr) {
    if (displayerror) {
      Warning(STORM_ERROR_MEMORY_NULL_POINTER, filename, linenumber);
    }

    return FALSE;
  }

  blockptr = (BLOCKPTR)((LPBYTE)ptr - sizeof(BLOCK));

  if (blockptr->signature1 != SIGNATURE1) {
    if (displayerror) {
      Warning(STORM_ERROR_MEMORY_INVALID_BLOCK, filename, linenumber);
    }

    return FALSE;
  }

  if (blockptr->flags & BF_FREEBLOCK) {
    if (displayerror) {
      Warning(STORM_ERROR_MEMORY_ALREADY_FREED, filename, linenumber);
    }

    return FALSE;
  }

  if (blockptr->flags & BF_BOUNDINGSIG) {
    if (*(WORD *)((LPBYTE)blockptr + blockptr->bytes - blockptr->padding - sizeof(WORD)) != SIGNATURE2) {
      if (displayerror) {
        Warning(STORM_ERROR_MEMORY_CORRUPT, filename, linenumber);
      }
    }
  }

  return TRUE;
}

static void CombineFreeBlocks(HEAPPTR heapptr) {
  FREEBLOCKPTR *nextfreeblock[9];
  BLOCKPTR      blockptr;
  BLOCKPTR      lastfree;
  DWORD         slot;

  for (slot = 0; slot < 9; slot++) {
    nextfreeblock[slot] = &heapptr->firstfreeblock[slot];
  }

  blockptr = heapptr->firstblock;
  lastfree = NULL;
  if (blockptr != heapptr->termblock) {
    do {
      if (blockptr->flags & BF_FREEBLOCK) {
        ((FREEBLOCKPTR)blockptr)->next = NULL;
        if (lastfree) {
          if ((LPBYTE)lastfree + lastfree->bytes == (LPBYTE)blockptr && lastfree->bytes + blockptr->bytes <= 0xFFFFU) {
            lastfree->bytes += blockptr->bytes;
            goto nextblock;
          }
          slot = ComputeFreeSlot(lastfree->bytes);
          *nextfreeblock[slot] = (FREEBLOCKPTR)lastfree;
          nextfreeblock[slot] = &((FREEBLOCKPTR)lastfree)->next;
        }
        lastfree = blockptr;
      }

    nextblock:
      blockptr = (BLOCKPTR)((LPBYTE)blockptr + blockptr->bytes);
    } while (blockptr != heapptr->termblock);
  }

  if (lastfree) {
    slot = ComputeFreeSlot(lastfree->bytes);
    *nextfreeblock[slot] = (FREEBLOCKPTR)lastfree;
    nextfreeblock[slot] = &((FREEBLOCKPTR)lastfree)->next;
  }

  for (slot = 0; slot < 9; slot++) {
    *nextfreeblock[slot] = NULL;
  }

  heapptr->uncombinedfree = 0;
}

static DWORD ComputeFreeSlot(DWORD bytes) {
  bytes >>= 5;
  if (bytes >= 8) {
    return 8;
  }

  return bytes;
}

static void ComputeBlockSize(DWORD bytes, LPDWORD blockSize, LPDWORD padding, LPBOOL largeAlloc, LPBOOL boundingSig) {
  *largeAlloc = s_guardmode || bytes > MAXALLOCSIZE;
  *boundingSig = s_debugmode && !*largeAlloc;

  if (*largeAlloc) {
    bytes = sizeof(LPVOID);
  }
  bytes = (*boundingSig ? sizeof(WORD) : 0) + bytes + sizeof(BLOCK);

  *blockSize = bytes + (-bytes & 7);
  *padding = *blockSize - bytes;
}

static void ComputePageSize() {
  SYSTEM_INFO sysinfo;

  GetSystemInfo(&sysinfo);

  s_pagesize = 1;
  if (sysinfo.dwPageSize > 1) {
    do {
      s_pagesize <<= 1;
    } while (s_pagesize < sysinfo.dwPageSize);
  }
}

static HEAPPTR *DestroyHeap(HEAPPTR *nextptr) {
  HEAPPTR  heapptr;
  BLOCKPTR blockptr;
  int      preserve;

  heapptr = *nextptr;
  blockptr = heapptr->firstblock;
  preserve = FALSE;

  while (blockptr < heapptr->termblock) {
    BLOCKPTR nextblock = (BLOCKPTR)((LPBYTE)blockptr + blockptr->bytes);
    BYTE     flags = blockptr->flags;

    if (flags & (BF_PRESERVE | BF_FREEBLOCK)) {
      if (flags & BF_PRESERVE) {
        preserve = TRUE;
      }
    } else {
      if (!(flags & 0x40)) {
        if (smemOptions.smemleaksilentwarning) {
          char szMessage[200];

          wsprintfA(szMessage, "Storm Error : memory never released -- %s:%d\n", heapptr->filename, heapptr->linenumber);
          OutputDebugStringA(szMessage);
        } else {
          Warning(STORM_ERROR_MEMORY_NEVER_RELEASED, heapptr->filename, heapptr->linenumber);
        }
      }
      FreeHeapBlock(heapptr, blockptr);
    }

    blockptr = nextblock;
  }

  if (preserve) {
    return (HEAPPTR *)heapptr;
  }

  FreeHeap(nextptr);
  return nextptr;
}

static void FillBlockHeaderAndSignatures(HEAPPTR heapptr, BLOCKPTR blockptr, DWORD blockSize, DWORD padding, BYTE flags) {
  blockptr->bytes = (WORD)blockSize;
  blockptr->padding = (BYTE)padding;
  blockptr->flags = flags;
  *(DWORD *)&blockptr->heapaddr = heapptr->addrsig;

  if (flags & BF_BOUNDINGSIG) {
    *(WORD *)((LPBYTE)blockptr + blockSize - padding - sizeof(WORD)) = SIGNATURE2;
  }
}

static void FreeEmptyHeaps() {
  DWORD slot;

  s_lastemptyheap = NULL;
  for (slot = 0; slot < TABLESIZE; slot++) {
    HEAPPTR *nextptr;

    if (!s_emptyheap[slot]) {
      continue;
    }

    EnterCriticalSection(&s_critsect[slot]);
    s_emptyheap[slot] = FALSE;
    nextptr = &s_heaphead[slot];
    while (*nextptr) {
      HEAPPTR heapptr = *nextptr;
      if (!heapptr->allocatedblocks && heapptr->handle < (HSHEAP)FIRSTUSERHEAP) {
        FreeHeap(nextptr);
      } else {
        nextptr = &heapptr->next;
      }
    }
    LeaveCriticalSection(&s_critsect[slot]);
  }
}

static void FreeHeap(HEAPPTR *nextptr) {
  HEAPPTR heapptr = *nextptr;

  *nextptr = heapptr->next;
  VirtualFree(heapptr, 0, MEM_RELEASE);
}

static void FreeHeapBlock(HEAPPTR heapptr, BLOCKPTR block) {
  DWORD        bytes;
  LPVOID       externalptr;
  FREEBLOCKPTR freeblock;
  BLOCKPTR     nextblock;

  if (block->flags & BF_LARGEALLOC) {
    externalptr = *(LPVOID *)((LPBYTE)block + sizeof(BLOCK));
    if (externalptr) {
      LPBYTE externalbase = (LPBYTE)externalptr - 0x10;
      bytes = *(DWORD *)externalbase;
      heapptr->externalbytes -= bytes;
      externalbase = (LPBYTE)((DWORD)externalbase & ~(s_pagesize - 1));
      VirtualFree(externalbase, 0, MEM_RELEASE);
    } else {
      bytes = 0;
    }
  } else {
    bytes = block->bytes - block->padding - sizeof(BLOCK);
    if (block->flags & BF_BOUNDINGSIG) {
      bytes -= sizeof(WORD);
    }
  }

  heapptr->allocatedblocks--;
  heapptr->allocatedbytes -= bytes;

  block->flags = (block->flags & BF_PREVFREE) | BF_FREEBLOCK;
  block->padding = 0;
  nextblock = (BLOCKPTR)((LPBYTE)block + block->bytes);
  freeblock = (FREEBLOCKPTR)block;
  if (nextblock == heapptr->termblock) {
    freeblock->next = NULL;
    heapptr->termblock = block;
  } else {
    DWORD slot;

    nextblock->flags |= BF_PREVFREE;
    slot = ComputeFreeSlot(block->bytes);
    freeblock->next = heapptr->firstfreeblock[slot];
    heapptr->firstfreeblock[slot] = freeblock;
    if ((block->flags & BF_PREVFREE) || (nextblock->flags & BF_FREEBLOCK)) {
      heapptr->uncombinedfree++;
    }
  }

  if (!heapptr->allocatedblocks) {
    ZeroMemory(heapptr->firstfreeblock, sizeof(heapptr->firstfreeblock));
    heapptr->termblock = heapptr->firstblock;
    heapptr->uncombinedfree = 0;
    if (heapptr->handle < (HSHEAP)FIRSTUSERHEAP) {
      s_emptyheap[heapptr->slot] = TRUE;
      s_lastemptyheap = heapptr;
    }
  }
}

static void GetBlockSize(BLOCKPTR blockptr, LPVOID ptr, LPDWORD bytes, LPDWORD overhead) {
  if (blockptr->flags & BF_LARGEALLOC) {
    *bytes = *(DWORD *)((LPBYTE)ptr - 0x10);
    *overhead = blockptr->padding + 0x1C;
    return;
  }

  *overhead = blockptr->padding + sizeof(BLOCK);
  if (blockptr->flags & BF_BOUNDINGSIG) {
    *overhead += sizeof(WORD);
  }

  *bytes = blockptr->bytes - *overhead;
}

static int GrowCommitSize(HEAPPTR heapptr, DWORD newheapsize) {
  newheapsize -= heapptr->committedbytes;
  if (newheapsize & (heapptr->chunksize - 1)) {
    newheapsize += heapptr->chunksize - (newheapsize & (heapptr->chunksize - 1));
  }

  if (heapptr->committedbytes + newheapsize > heapptr->reservedbytes) {
    newheapsize = heapptr->reservedbytes - heapptr->committedbytes;
  }

  if (!VirtualAlloc((LPBYTE)heapptr + heapptr->committedbytes, newheapsize, MEM_COMMIT, PAGE_READWRITE)) {
    return FALSE;
  }

  heapptr->committedbytes += newheapsize;
  return TRUE;
}

static BOOL GrowHeapBlock(HEAPPTR heapptr, BLOCKPTR blockptr, DWORD sourceBytes, DWORD bytes) {
  BOOL     boundingSig;
  BOOL     largeAlloc;
  BLOCKPTR newEndBlock;
  DWORD    newBlockSize;
  DWORD    blockSize;
  DWORD    padding;

  ASSERT(bytes > sourceBytes);
  ASSERT(!(blockptr->flags & BF_LARGEALLOC));

  ComputeBlockSize(bytes, &blockSize, &padding, &largeAlloc, &boundingSig);
  if (blockSize > 0xFFFF || largeAlloc) {
    return FALSE;
  }

  newBlockSize = blockptr->bytes;
  if (blockSize <= newBlockSize) {
    padding += newBlockSize - blockSize;
    blockSize = newBlockSize;
  } else {
    DWORD    neededBytes;
    DWORD    adjacentFreeBytes;
    BLOCKPTR scan;

    neededBytes = blockSize - newBlockSize;
    newEndBlock = (BLOCKPTR)((LPBYTE)blockptr + blockSize);
    adjacentFreeBytes = 0;
    scan = (BLOCKPTR)((LPBYTE)blockptr + newBlockSize);

    while (adjacentFreeBytes < neededBytes) {
      if (scan == heapptr->termblock) {
        break;
      }
      if (!(scan->flags & BF_FREEBLOCK)) {
        return FALSE;
      }

      adjacentFreeBytes += scan->bytes;
      scan = (BLOCKPTR)((LPBYTE)scan + scan->bytes);
    }

    newBlockSize += adjacentFreeBytes > neededBytes ? adjacentFreeBytes : neededBytes;
    if (newBlockSize > 0xFFFF) {
      return FALSE;
    }

    if (adjacentFreeBytes < neededBytes) {
      DWORD newheapsize = (DWORD)((LPBYTE)newEndBlock - (LPBYTE)heapptr);

      if (newheapsize > heapptr->committedbytes) {
        if (newheapsize > heapptr->reservedbytes) {
          return FALSE;
        }
        if (!GrowCommitSize(heapptr, newheapsize)) {
          return FALSE;
        }
      }

      heapptr->termblock = newEndBlock;
    }

    blockptr->bytes = (WORD)newBlockSize;
    SubdivideBlock(heapptr, blockptr, &blockSize, &padding);
    blockptr->bytes = (WORD)blockSize;

    if (newEndBlock != heapptr->termblock) {
      newEndBlock->flags &= ~BF_PREVFREE;
      if ((newEndBlock->flags & BF_FREEBLOCK) && heapptr->uncombinedfree) {
        --heapptr->uncombinedfree;
      }
    }

    CombineFreeBlocks(heapptr);
  }

  FillBlockHeaderAndSignatures(heapptr, blockptr, blockSize, padding, (blockptr->flags & ~BF_BOUNDINGSIG) | (boundingSig ? BF_BOUNDINGSIG : 0));

  heapptr->allocatedbytes += bytes - sourceBytes;
  return TRUE;
}

static LPVOID SatisfyAllocRequest(HLOCKEDHEAP *lockedhandle, HEAPPTR heapptr, DWORD flags, DWORD bytes) {
  BYTE   baseflags;
  LPVOID ptr;

  if (bytes > MAXHEAPSIZE) {
    UnlockHeap(lockedhandle);
    FatalError(ERROR_NOT_ENOUGH_MEMORY, heapptr->filename, heapptr->linenumber);
  }

  ptr = NULL;
  if (heapptr) {
    baseflags = 0;
    if (flags & 0x04000000) {
      baseflags = 0x40;
    }
    if (flags & SMEM_FLAG_PRESERVEONDESTROY) {
      baseflags |= BF_PRESERVE;
    }

    ptr = AllocateHeapBlock(heapptr, bytes, baseflags);
  }

  if (!ptr) {
    UnlockHeap(lockedhandle);
    if (heapptr->filename[0]) {
      FatalError(ERROR_NOT_ENOUGH_MEMORY, heapptr->filename, heapptr->linenumber);
    } else {
      FatalError(ERROR_NOT_ENOUGH_MEMORY, "SMemHeapAlloc()", SERR_LINECODE_FUNCTION);
    }
  }

  if (flags & SMEM_FLAG_ZEROMEMORY) {
    memset(ptr, 0, bytes);
  } else if (s_fillmode) {
    memset(ptr, 0xEE, bytes);
  }

  IncrementAllocCount();
  s_totalAllocated += bytes;
  heapptr->cumulativeAllocs++;

  return ptr;
}

static void SatisfyFreeRequest(HEAPPTR heapptr, LPVOID ptr, BLOCKPTR blockptr) {
  DWORD bytes;
  DWORD overhead;

  GetBlockSize(blockptr, ptr, &bytes, &overhead);
  if (s_fillmode && !(blockptr->flags & BF_LARGEALLOC)) {
    memset(ptr, 0xDD, bytes);
  }

  s_totalAllocated -= bytes;
  heapptr->cumulativeFrees++;
  FreeHeapBlock(heapptr, blockptr);
  IncrementFreeCount();
}

static LPVOID SatisfyReAllocRequest(HLOCKEDHEAP *lockedhandle, HEAPPTR heapptr, LPVOID ptr, BLOCKPTR blockptr, DWORD bytes, DWORD flags) {
  DWORD  sourceBytes;
  DWORD  overhead;
  LPVOID newptr;
  DWORD  copyBytes;

  GetBlockSize(blockptr, ptr, &sourceBytes, &overhead);
  newptr = NULL;

  if (!s_reallocshufflemode && !(blockptr->flags & BF_LARGEALLOC)) {
    if (bytes < sourceBytes) {
      ShrinkHeapBlock(heapptr, blockptr, sourceBytes, bytes);
      newptr = ptr;
    } else if (bytes > sourceBytes) {
      if (GrowHeapBlock(heapptr, blockptr, sourceBytes, bytes)) {
        newptr = ptr;
      }
    }
  }

  heapptr->cumulativeReallocs++;
  if (newptr) {
    s_totalAllocated += bytes - sourceBytes;
    heapptr->cumulativeAllocs++;
  } else {
    if (flags & SMEM_FLAG_REALLOC_IN_PLACE) {
      return NULL;
    }

    newptr = SatisfyAllocRequest(lockedhandle, heapptr, 0, bytes);
    if (newptr && sourceBytes && bytes) {
      copyBytes = sourceBytes < bytes ? sourceBytes : bytes;
      memcpy(newptr, ptr, copyBytes);
    }

    SatisfyFreeRequest(heapptr, ptr, blockptr);
    if (!newptr) {
      return NULL;
    }
  }

  if (bytes > sourceBytes) {
    if (flags & SMEM_FLAG_ZEROMEMORY) {
      memset((LPBYTE)newptr + sourceBytes, 0, bytes - sourceBytes);
    } else if (s_fillmode) {
      memset((LPBYTE)newptr + sourceBytes, 0xEE, bytes - sourceBytes);
    }
  }

  return newptr;
}

static void ShrinkHeapBlock(HEAPPTR heapptr, BLOCKPTR blockptr, DWORD sourceBytes, DWORD bytes) {
  BOOL  largeAlloc;
  BOOL  boundingSig;
  BYTE  flags;
  DWORD padding;
  DWORD blockSize;

  ASSERT(bytes < sourceBytes);
  ASSERT(!(blockptr->flags & BF_LARGEALLOC));

  ComputeBlockSize(bytes, &blockSize, &padding, &largeAlloc, &boundingSig);
  ASSERT(blockSize <= blockptr->bytes);

  flags = blockptr->flags & ~BF_BOUNDINGSIG;
  if (boundingSig) {
    flags |= BF_BOUNDINGSIG;
  }
  SubdivideBlock(heapptr, blockptr, &blockSize, &padding);
  FillBlockHeaderAndSignatures(heapptr, blockptr, blockSize, padding, flags);

  heapptr->allocatedbytes += bytes - sourceBytes;
}

static void SubdivideBlock(HEAPPTR heapptr, BLOCKPTR blockptr, LPDWORD blocksize, LPDWORD padding) {
  DWORD    remaining;
  BLOCKPTR endblock;

  remaining = blockptr->bytes - *blocksize;
  endblock = (BLOCKPTR)((LPBYTE)blockptr + blockptr->bytes);
  if (endblock == heapptr->termblock) {
    heapptr->termblock = (BLOCKPTR)((LPBYTE)blockptr + *blocksize);
    return;
  }

  if (remaining >= MINBLOCKSIZE) {
    FREEBLOCKPTR freeblock = (FREEBLOCKPTR)((LPBYTE)blockptr + *blocksize);
    DWORD        slot;

    freeblock->bytes = (WORD)remaining;
    freeblock->padding = 0;
    freeblock->flags = BF_FREEBLOCK;
    slot = ComputeFreeSlot(remaining);
    freeblock->next = heapptr->firstfreeblock[slot];
    heapptr->firstfreeblock[slot] = freeblock;
    return;
  }

  endblock->flags &= ~BF_PREVFREE;
  *blocksize += remaining;
  *padding += remaining;
}

// --------------------------------
// Exported functions
// --------------------------------

LPVOID APIENTRY SMemAlloc(DWORD bytes, LPCSTR filename, int linenumber, DWORD flags) {
  HLOCKEDHEAP lockedhandle;
  HSHEAP      handle;
  HEAPPTR     heapptr;
  LPVOID      ptr;

  if (!CheckInitialized()) {
    FatalError(STORM_ERROR_MEMORY_MANAGER_INACTIVE, "SMemAlloc()", SERR_LINECODE_FUNCTION);
  }

  handle = GetHandleByCaller(filename, linenumber);
  heapptr = LockHeapByHandle(handle, &lockedhandle, FALSE);
  if (!heapptr) {
    heapptr = AllocateHeap(filename, linenumber, handle, GetSlotByHandle(handle), PAGESIZE, PAGESIZE, RESERVESIZE);
  }

  ptr = SatisfyAllocRequest(&lockedhandle, heapptr, flags, bytes);
  UnlockHeap(&lockedhandle);

  if (s_lastemptyheap && s_lastemptyheap != heapptr) {
    FreeEmptyHeaps();
  }

  return ptr;
}

BOOL APIENTRY SMemDestroy() {
  DWORD             slot = 0;
  CRITICAL_SECTION *critsect;
  DWORD             remaining;

  if (s_initialized) {
    STypeCache::Shutdown();
    s_initialized = FALSE;
    critsect = s_critsect;
    remaining = TABLESIZE;
    do {
      HEAPPTR *nextptr;

      EnterCriticalSection(critsect);
      s_emptyheap[slot] = FALSE;
      nextptr = &s_heaphead[slot];
      while (*nextptr) {
        HEAPPTR heapptr = *nextptr;

        if (heapptr->allocatedblocks) {
          nextptr = DestroyHeap(nextptr);
        } else {
          if (heapptr->active && heapptr->handle >= (HSHEAP)FIRSTUSERHEAP) {
            SErrReportResourceLeak("HSHEAP");
          }
          FreeHeap(nextptr);
        }
      }
      LeaveCriticalSection(critsect);
      DeleteCriticalSection(critsect);
      slot++;
      critsect++;
    } while (--remaining);
  }

  return TRUE;
}

BOOL APIENTRY SMemDumpState(SMEMDUMPPROC outputproc, HOUTPUTCONTEXT outputcontext) {
  char            buffer[256];
  SMEMHEAPDETAILS heapdetails;
  HSHEAP          heap;

  if (!CheckInitialized()) {
    Warning(STORM_ERROR_MEMORY_MANAGER_INACTIVE, "SMemDumpState()", SERR_LINECODE_FUNCTION);
    return FALSE;
  }

  FATALASSERT(outputproc);

  heapdetails.size = sizeof(heapdetails);
  heap = NULL;
  if (SMemFindNextHeap(NULL, &heap, &heapdetails)) {
    do {
      sprintf(
          buffer, "%s:%d  blocks=%u  %u/%u/%u", heapdetails.filename, heapdetails.linenumber, heapdetails.allocatedblocks, heapdetails.allocatedbytes,
          heapdetails.committedbytes, heapdetails.reservedbytes
      );
      outputproc(outputcontext, buffer);
    } while (SMemFindNextHeap(heap, &heap, &heapdetails));
  }

  return TRUE;
}

BOOL APIENTRY SMemDumpStateEx(char *arglist) {
  SMemReportByCallerInfo info;
  SMEMHEAPDETAILS2       heapdetails;
  HSHEAP                 heap;
  SMEMREPORTTYPE         reporttype;
  SMEMREPORTPROC         outputproc;
  HOUTPUTCONTEXT         outputcontext;

  if (!CheckInitialized()) {
    Warning(STORM_ERROR_MEMORY_MANAGER_INACTIVE, "SMemDumpStateEx()", SERR_LINECODE_FUNCTION);
    return FALSE;
  }

  reporttype = *(SMEMREPORTTYPE *)arglist;
  arglist += sizeof(SMEMREPORTTYPE);
  outputproc = *(SMEMREPORTPROC *)arglist;
  arglist += sizeof(SMEMREPORTPROC);
  outputcontext = *(HOUTPUTCONTEXT *)arglist;

  FATALASSERT(outputproc);

  if (reporttype == SMEM_REPORT_BY_CALLER) {
    heap = NULL;
    heapdetails.size = sizeof(heapdetails);
    while (SMemFindNextHeap2(heap, &heap, &heapdetails)) {
      info.numSubHeaps = heapdetails.regions;
      info.cumulativeAllocs = heapdetails.cumulativeAllocs;
      info.cumulativeFrees = heapdetails.cumulativeFrees;
      info.cumulativeReallocs = heapdetails.cumulativeReallocs;
      info.allocatedBlocks = heapdetails.allocatedblocks;
      info.allocatedBytes = heapdetails.allocatedbytes;
      info.committedBytes = heapdetails.committedbytes;
      info.reservedBytes = heapdetails.reservedbytes;
      info.mark_allocatedBlocks = heapdetails.mark_allocatedblocks;
      info.mark_allocatedBytes = heapdetails.mark_allocatedbytes;
      info.mark_committedBytes = heapdetails.mark_committedbytes;
      info.mark_cumulativeAllocs = heapdetails.mark_cumulativeAllocs;
      info.mark_cumulativeFrees = heapdetails.mark_cumulativeFrees;
      info.mark_cumulativeReallocs = heapdetails.mark_cumulativeReallocs;
      info.lineNumber = heapdetails.linenumber;
      SStrCopy(info.fileName, heapdetails.filename, sizeof(info.fileName));

      outputproc(outputcontext, (const char *)&info);
      heapdetails.size = sizeof(heapdetails);
    }
    return TRUE;
  }

  if (reporttype == SMEM_REPORT_HISTOGRAM) {
    return FALSE;
  }

  return TRUE;
}

BOOL APIENTRY SMemMarkAllHeapsEx(char *arglist) {
  DWORD slot;

  if (!CheckInitialized()) {
    Warning(STORM_ERROR_MEMORY_MANAGER_INACTIVE, "SMemMarkAllHeapsEx()", SERR_LINECODE_FUNCTION);
    return FALSE;
  }

  for (slot = 0; slot < TABLESIZE; slot++) {
    HEAPPTR heapptr;
    EnterCriticalSection(&s_critsect[slot]);
    for (heapptr = s_heaphead[slot]; heapptr; heapptr = heapptr->next) {
      heapptr->mark_externalbytes = heapptr->externalbytes;
      heapptr->mark_allocatedblocks = heapptr->allocatedblocks;
      heapptr->mark_allocatedbytes = heapptr->allocatedbytes;
      heapptr->mark_committedbytes = heapptr->committedbytes;
      heapptr->mark_cumulativeAllocs = heapptr->cumulativeAllocs;
      heapptr->mark_cumulativeFrees = heapptr->cumulativeFrees;
      heapptr->mark_cumulativeReallocs = heapptr->cumulativeReallocs;
    }
    LeaveCriticalSection(&s_critsect[slot]);
  }

  return TRUE;
}

BOOL APIENTRY SMemFindNextBlock(HSHEAP heap, LPVOID prevblock, LPVOID *nextblock, LPSMEMBLOCKDETAILS details) {
  DWORD    slot;
  HEAPPTR  heapptr;
  HEAPPTR  lastheapptr;
  BLOCKPTR blockptr;
  BLOCKPTR prevblockptr;
  BLOCKPTR lastblockptr;
  BOOL     found;
  LPVOID   ptr;

  if (!CheckInitialized()) {
    Warning(STORM_ERROR_MEMORY_MANAGER_INACTIVE, "SMemFindNextBlock()", SERR_LINECODE_FUNCTION);
    return FALSE;
  }

  FATALASSERT(heap);
  FATALASSERT(nextblock);
  FATALASSERT(details);
  FATALASSERT(details->size == sizeof(SMEMBLOCKDETAILS));

  ZeroMemory((LPBYTE)details + sizeof(DWORD), sizeof(SMEMBLOCKDETAILS) - sizeof(DWORD));
  slot = GetSlotByHandle(heap);
  EnterCriticalSection(&s_critsect[slot]);

  prevblockptr = GetBlockPtrByPtr(prevblock);
  lastblockptr = NULL;
  blockptr = NULL;
  found = FALSE;

  heapptr = s_heaphead[slot];
  while (heapptr && heapptr->next) {
    heapptr = heapptr->next;
  }

  while (heapptr && !found) {
    if (heapptr->handle == heap) {
      for (blockptr = heapptr->firstblock; blockptr != heapptr->termblock; blockptr = (BLOCKPTR)((LPBYTE)blockptr + blockptr->bytes)) {
        if (lastblockptr == prevblockptr) {
          found = TRUE;
          break;
        }
        lastblockptr = blockptr;
      }
    }

    if (heapptr == s_heaphead[slot]) {
      break;
    }

    lastheapptr = heapptr;
    heapptr = s_heaphead[slot];
    while (heapptr->next != lastheapptr) {
      heapptr = heapptr->next;
    }
  }

  if (!found) {
    *nextblock = NULL;
    LeaveCriticalSection(&s_critsect[slot]);
    return FALSE;
  }

  ptr = GetPtrByBlockPtr(blockptr);
  *nextblock = ptr;
  details->ptr = ptr;
  details->allocated = (blockptr->flags & BF_FREEBLOCK) == 0;
  details->valid = CheckValidBlock(ptr, FALSE, NULL, 0);
  GetBlockSize(blockptr, ptr, &details->bytes, &details->overhead);

  LeaveCriticalSection(&s_critsect[slot]);
  return TRUE;
}

void APIENTRY SMemHeapGetDetails(HSHEAP heap, LPSMEMHEAPDETAILS details) {
  DWORD   slot;
  HEAPPTR pHeap;

  ZeroMemory((LPBYTE)details + sizeof(DWORD), details->size - sizeof(DWORD));
  slot = GetSlotByHandle(heap);
  EnterCriticalSection(&s_critsect[slot]);
  pHeap = s_heaphead[slot];
  ASSERT(pHeap);

  details->handle = pHeap->handle;
  SStrCopy(details->filename, pHeap->filename, sizeof(details->filename));
  details->linenumber = pHeap->linenumber;
  details->maximumsize = MAXHEAPSIZE;

  do {
    if (pHeap->handle == heap) {
      details->committedbytes += pHeap->committedbytes + pHeap->externalbytes;
      details->reservedbytes += pHeap->reservedbytes + pHeap->externalbytes;
      details->allocatedblocks += pHeap->allocatedblocks;
      details->allocatedbytes += pHeap->allocatedbytes;
    }
    pHeap = pHeap->next;
  } while (pHeap);

  LeaveCriticalSection(&s_critsect[slot]);
}

BOOL APIENTRY SMemFindNextHeap(HSHEAP prevheap, HSHEAP *nextheap, LPSMEMHEAPDETAILS details) {
  HLOCKEDHEAP lockedhandle;
  HEAPPTR     heapptr;

  if (!CheckInitialized()) {
    Warning(STORM_ERROR_MEMORY_MANAGER_INACTIVE, "SMemFindNextHeap()", SERR_LINECODE_FUNCTION);
    return FALSE;
  }

  FATALASSERT(nextheap);
  FATALASSERT(details);
  FATALASSERT(details->size == sizeof(SMEMHEAPDETAILS));

  ZeroMemory((LPBYTE)details + sizeof(DWORD), sizeof(SMEMHEAPDETAILS) - sizeof(DWORD));
  lockedhandle = (HLOCKEDHEAP)-1;
  heapptr = LockNextHeapByHandle(prevheap, &lockedhandle);
  if (!heapptr) {
    *nextheap = NULL;
    return FALSE;
  }

  *nextheap = heapptr->handle;
  details->handle = heapptr->handle;
  SStrCopy(details->filename, heapptr->filename, sizeof(details->filename));
  details->linenumber = heapptr->linenumber;
  details->maximumsize = MAXHEAPSIZE;

  do {
    if (heapptr->handle == *nextheap) {
      details->committedbytes += heapptr->committedbytes + heapptr->externalbytes;
      details->reservedbytes += heapptr->reservedbytes + heapptr->externalbytes;
      details->allocatedblocks += heapptr->allocatedblocks;
      details->allocatedbytes += heapptr->allocatedbytes;
    }
    heapptr = heapptr->next;
  } while (heapptr);

  UnlockHeap(&lockedhandle);
  return TRUE;
}

BOOL APIENTRY SMemFindNextHeap2(HSHEAP prevheap, HSHEAP *nextheap, LPSMEMHEAPDETAILS2 details) {
  HLOCKEDHEAP lockedhandle;
  HEAPPTR     heapptr;
  HEAPPTR     scan;

  if (!CheckInitialized()) {
    Warning(STORM_ERROR_MEMORY_MANAGER_INACTIVE, "SMemFindNextHeap2()", SERR_LINECODE_FUNCTION);
    return FALSE;
  }

  FATALASSERT(nextheap);
  FATALASSERT(details);
  FATALASSERT(details->size == sizeof(SMEMHEAPDETAILS2));

  ZeroMemory((LPBYTE)details + sizeof(DWORD), sizeof(SMEMHEAPDETAILS2) - sizeof(DWORD));
  heapptr = LockNextHeapByHandle(prevheap, &lockedhandle);
  if (!heapptr) {
    *nextheap = NULL;
    return FALSE;
  }

  *nextheap = heapptr->handle;
  details->handle = heapptr->handle;
  SStrCopy(details->filename, heapptr->filename, sizeof(details->filename));
  details->linenumber = heapptr->linenumber;
  details->maximumsize = MAXHEAPSIZE;
  details->regions = 0;

  for (scan = heapptr; scan; scan = scan->next) {
    if (scan->handle == *nextheap) {
      details->regions++;
      details->committedbytes += scan->committedbytes + scan->externalbytes;
      details->reservedbytes += scan->reservedbytes + scan->externalbytes;
      details->cumulativeAllocs += scan->cumulativeAllocs;
      details->cumulativeFrees += scan->cumulativeFrees;
      details->cumulativeReallocs += scan->cumulativeReallocs;
      details->allocatedblocks += scan->allocatedblocks;
      details->allocatedbytes += scan->allocatedbytes;
      details->mark_allocatedblocks += scan->mark_allocatedblocks;
      details->mark_allocatedbytes += scan->mark_allocatedbytes;
      details->mark_committedbytes += scan->mark_committedbytes + scan->mark_externalbytes;
      details->mark_cumulativeAllocs += scan->mark_cumulativeAllocs;
      details->mark_cumulativeFrees += scan->mark_cumulativeFrees;
      details->mark_cumulativeReallocs += scan->mark_cumulativeReallocs;
    }
  }

  UnlockHeap(&lockedhandle);
  return TRUE;
}

BOOL APIENTRY SMemFree(LPVOID ptr, LPCSTR filename, int linenumber, DWORD flags) {
  HLOCKEDHEAP lockedhandle;
  BLOCKPTR    blockptr;
  HEAPPTR     heapptr;

  if (!CheckInitialized()) {
    Warning(STORM_ERROR_MEMORY_MANAGER_INACTIVE, "SMemFree()", SERR_LINECODE_FUNCTION);
    return FALSE;
  }

  if (!CheckValidBlock(ptr, TRUE, filename, linenumber)) {
    return FALSE;
  }

  blockptr = GetBlockPtrByPtr(ptr);
  heapptr = LockHeapByBlockPtr(blockptr, &lockedhandle);
  SatisfyFreeRequest(heapptr, ptr, blockptr);
  UnlockHeap(&lockedhandle);
  return heapptr != NULL;
}

DWORD APIENTRY SMemGetAllocated(LPDWORD allocated, LPDWORD committed, LPDWORD reserved) {
  if (allocated) {
    *allocated = s_totalAllocated;
  }

  if (committed) {
    *committed = s_totalAllocated;
  }

  if (reserved) {
    *reserved = s_totalAllocated;
  }

  return s_totalAllocated;
}

HSHEAP APIENTRY SMemGetHeapByCaller(LPCSTR filename, int linenumber) {
  if (!CheckInitialized()) {
    Warning(STORM_ERROR_MEMORY_MANAGER_INACTIVE, "SMemGetHeapByCaller()", SERR_LINECODE_FUNCTION);
    return NULL;
  }

  return GetHandleByCaller(filename, linenumber);
}

HSHEAP APIENTRY SMemGetHeapByPtr(LPVOID ptr) {
  if (!CheckInitialized()) {
    Warning(STORM_ERROR_MEMORY_MANAGER_INACTIVE, "SMemGetHeapByPtr()", SERR_LINECODE_FUNCTION);
    return NULL;
  }

  if (!CheckValidBlock(ptr, FALSE, NULL, 0)) {
    return NULL;
  }

  return GetHandleByBlockPtr(GetBlockPtrByPtr(ptr));
}

DWORD APIENTRY SMemGetSize(LPVOID ptr, LPCSTR filename, int linenumber) {
  if (!CheckInitialized()) {
    Warning(STORM_ERROR_MEMORY_MANAGER_INACTIVE, "SMemGetSize()", SERR_LINECODE_FUNCTION);
    return -1;
  }

  if (!CheckValidBlock(ptr, TRUE, filename, linenumber)) {
    return -1;
  }

  {
    DWORD overhead;
    DWORD bytes;
    GetBlockSize(GetBlockPtrByPtr(ptr), ptr, &bytes, &overhead);
    return bytes;
  }
}

LPVOID APIENTRY SMemHeapAlloc(HSHEAP handle, DWORD flags, DWORD bytes) {
  HLOCKEDHEAP lockedhandle;
  HEAPPTR     heapptr;
  LPVOID      ptr;

  if (!CheckInitialized()) {
    FatalError(STORM_ERROR_MEMORY_MANAGER_INACTIVE, "SMemHeapAlloc()", SERR_LINECODE_FUNCTION);
  }

  heapptr = LockHeapByHandle(handle, &lockedhandle, TRUE);
  if (!heapptr) {
    FatalError(ERROR_INVALID_HANDLE, "SMemHeapAlloc()", SERR_LINECODE_FUNCTION);
  }

  ptr = SatisfyAllocRequest(&lockedhandle, heapptr, flags, bytes);
  UnlockHeap(&lockedhandle);

  if (s_lastemptyheap && s_lastemptyheap != heapptr) {
    FreeEmptyHeaps();
  }

  return ptr;
}

HSHEAP APIENTRY SMemHeapCreate(DWORD options, DWORD initialsize, DWORD maximumsize, LPCSTR filename, int linenumber) {
  DWORD       slot;
  HEAPPTR     heapptr;
  HLOCKEDHEAP lockedhandle;

  if (!CheckInitialized()) {
    Warning(STORM_ERROR_MEMORY_MANAGER_INACTIVE, "SMemHeapCreate()", SERR_LINECODE_FUNCTION);
    return NULL;
  }

  if (options) {
    Warning(ERROR_INVALID_PARAMETER, "SMemHeapCreate()", SERR_LINECODE_FUNCTION);
    return NULL;
  }

  if (initialsize & (PAGESIZE - 1)) {
    initialsize += PAGESIZE - (initialsize & (PAGESIZE - 1));
  }
  if (initialsize <= PAGESIZE) {
    initialsize = PAGESIZE;
  }

  for (;;) {
    handle = (HSHEAP)((DWORD)handle + 1);
    if (!handle) {
      handle = (HSHEAP)FIRSTUSERHEAP;
    }

    heapptr = LockHeapByHandle(handle, &lockedhandle, TRUE);
    if (!heapptr) {
      break;
    }

    UnlockHeap(&lockedhandle);
  }

  slot = GetSlotByHandle(handle);
  EnterCriticalSection(&s_critsect[slot]);
  heapptr = AllocateHeap(filename, linenumber, handle, slot, PAGESIZE, initialsize, RESERVESIZE);
  LeaveCriticalSection(&s_critsect[slot]);

  if (!heapptr) {
    Warning(ERROR_NOT_ENOUGH_MEMORY, "SMemHeapCreate()", SERR_LINECODE_FUNCTION);
    return NULL;
  }

  return handle;
}

BOOL APIENTRY SMemHeapDestroy(HSHEAP handle) {
  DWORD    slot;
  HEAPPTR *nextptr;
  HEAPPTR  heapptr;
  BOOL     destroyed;

  if (!CheckInitialized()) {
    Warning(STORM_ERROR_MEMORY_MANAGER_INACTIVE, "SMemHeapDestroy()", SERR_LINECODE_FUNCTION);
    return FALSE;
  }

  slot = GetSlotByHandle(handle);
  destroyed = FALSE;

  EnterCriticalSection(&s_critsect[slot]);
  nextptr = &s_heaphead[slot];
  heapptr = *nextptr;
  while (heapptr) {
    if (heapptr->handle == handle) {
      destroyed = TRUE;
      nextptr = DestroyHeap(nextptr);
    } else {
      nextptr = &heapptr->next;
    }

    heapptr = *nextptr;
  }
  LeaveCriticalSection(&s_critsect[slot]);

  return destroyed;
}

BOOL APIENTRY SMemHeapFree(HSHEAP handle, DWORD flags, LPVOID ptr) {
  BLOCKPTR blockptr;
  HEAPPTR  heapptr;

  if (!CheckInitialized()) {
    Warning(STORM_ERROR_MEMORY_MANAGER_INACTIVE, "SMemHeapFree()", SERR_LINECODE_FUNCTION);
    return FALSE;
  }

  if (!CheckValidBlock(ptr, TRUE, NULL, 0)) {
    return FALSE;
  }

  blockptr = GetBlockPtrByPtr(ptr);
  if (GetHandleByBlockPtr(blockptr) != handle) {
    return FALSE;
  }

  {
    HLOCKEDHEAP lockedhandle;
    heapptr = LockHeapByBlockPtr(blockptr, &lockedhandle);
    SatisfyFreeRequest(heapptr, ptr, blockptr);
    UnlockHeap(&lockedhandle);
    return heapptr != NULL;
  }
}

LPVOID APIENTRY SMemHeapReAlloc(HSHEAP handle, DWORD flags, LPVOID ptr, DWORD bytes) {
  HLOCKEDHEAP lockedhandle;
  BLOCKPTR    blockptr;
  HEAPPTR     heapptr;
  LPVOID      newptr;

  if (!CheckInitialized()) {
    FatalError(STORM_ERROR_MEMORY_MANAGER_INACTIVE, "SMemHeapReAlloc()", SERR_LINECODE_FUNCTION);
  }

  if (ptr && CheckValidBlock(ptr, TRUE, NULL, 0)) {
    blockptr = GetBlockPtrByPtr(ptr);
    if (GetHandleByBlockPtr(blockptr) != handle) {
      return NULL;
    }

    heapptr = LockHeapByBlockPtr(blockptr, &lockedhandle);
    newptr = SatisfyReAllocRequest(&lockedhandle, heapptr, ptr, blockptr, bytes, flags);
    UnlockHeap(&lockedhandle);

    return newptr;
  }

  return SMemHeapAlloc(handle, flags, bytes);
}

DWORD APIENTRY SMemHeapSize(HSHEAP handle, DWORD flags, LPVOID ptr) {
  BLOCKPTR blockptr;

  if (!CheckInitialized()) {
    Warning(STORM_ERROR_MEMORY_MANAGER_INACTIVE, "SMemHeapSize()", SERR_LINECODE_FUNCTION);
    return -1;
  }

  if (!CheckValidBlock(ptr, TRUE, NULL, 0)) {
    return -1;
  }

  blockptr = GetBlockPtrByPtr(ptr);
  if (GetHandleByBlockPtr(blockptr) != handle) {
    return -1;
  }

  {
    DWORD overhead;
    DWORD bytes;
    GetBlockSize(blockptr, ptr, &bytes, &overhead);
    return bytes;
  }
}

void APIENTRY SMemInitialize() {
  CRITICAL_SECTION *critsect;
  DWORD             remaining;

  if (s_initialized) {
    return;
  }

  s_debugmode = TRUE;
  SRegLoadValue(REGKEY, REGVAL_DEBUG, 0, (LPDWORD)&s_debugmode);
  SRegLoadValue(REGKEY, REGVAL_GUARD, 0, (LPDWORD)&s_guardmode);
  SRegLoadValue(REGKEY, REGVAL_REALLOCSHUFFLE, 0, (LPDWORD)&s_reallocshufflemode);
  SRegLoadValue(REGKEY, REGVAL_DEBUGERROUTPUT, 0, (LPDWORD)&smemOptions.smemleaksilentwarning);

  SRegSaveValue(REGKEY, REGVAL_DEBUG, 0, (DWORD)s_debugmode);
  SRegSaveValue(REGKEY, REGVAL_GUARD, 0, (DWORD)s_guardmode);

  s_debugmode = TRUE;
  s_fillmode = TRUE;
  s_warnings = TRUE;

  SRegSaveValue(REGKEY, REGVAL_DEBUGERROUTPUT, 0, smemOptions.serrleaksilentwarning || smemOptions.smemleaksilentwarning);

  critsect = s_critsect;
  remaining = TABLESIZE;
  do {
    InitializeCriticalSection(critsect++);
  } while (--remaining);

  smemOptions.crcenabled = TRUE;
  s_initialized = TRUE;
}

BOOL APIENTRY SMemIsValidPointer(LPCVOID address, DWORD size, BOOL forWriting) {
  if (forWriting) {
    return !IsBadWritePtr((LPVOID)address, size);
  }

  return !IsBadReadPtr(address, size);
}

LPVOID APIENTRY SMemReAlloc(LPVOID ptr, DWORD bytes, LPCSTR filename, int linenumber, DWORD flags) {
  BLOCKPTR blockptr;
  HEAPPTR  heapptr;
  LPVOID   newptr;

  if (!CheckInitialized()) {
    FatalError(STORM_ERROR_MEMORY_MANAGER_INACTIVE, "SMemReAlloc()", SERR_LINECODE_FUNCTION);
  }

  if (ptr && CheckValidBlock(ptr, TRUE, filename, linenumber)) {
    HLOCKEDHEAP lockedhandle;
    blockptr = GetBlockPtrByPtr(ptr);
    heapptr = LockHeapByBlockPtr(blockptr, &lockedhandle);
    newptr = SatisfyReAllocRequest(&lockedhandle, heapptr, ptr, blockptr, bytes, flags);
    UnlockHeap(&lockedhandle);

    return newptr;
  }

  return SMemAlloc(bytes, filename, linenumber, flags);
}

void APIENTRY SMemSetDebugFlags(DWORD flags, DWORD changeMask) {
  if (changeMask & 0x1) {
    s_debugmode = flags & 0x1;
  }

  if (changeMask & 0x8) {
    s_fillmode = (flags >> 3) & 0x1;
  }

  if (changeMask & 0x4) {
    s_guardmode = (flags >> 2) & 0x1;
  }

  if (changeMask & 0x2) {
    s_warnings = (flags >> 1) & 0x1;
  }
}

void __cdecl SMemTrace(LPCSTR format, ...) {
  // trace logging was disabled in this build
}
