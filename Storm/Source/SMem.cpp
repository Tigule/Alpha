#include <storm.h>

#define FIRSTUSERHEAP 0x80000000
#define MAXALLOCSIZE  (0xFFFF - (sizeof(HEAP) + MAX_PATH + sizeof(BLOCK) + 2 * sizeof(DWORD)))
#define MAXHEAPSIZE   0x7FFFFFFF
#define MINBLOCKSIZE  (sizeof(BLOCK) + 2 * sizeof(DWORD))
#define PAGESIZE      0x1000
#define RESERVESIZE   0x10000
#define SIGNATURE1    0x6F6D
#define SIGNATURE2    0xB112
#define TABLESIZE     256

#define REGKEY                "Internal"
#define REGVAL_DEBUG          "Debug Memory"
#define REGVAL_GUARD          "Protect Memory"
#define REGVAL_TRACEFILE      "SMem Trace File"
// not original macro name
#define REGVAL_REALLOCSHUFFLE "Realloc Shuffle"
// not original macro name
#define REGVAL_DEBUGERROUTPUT "Debug Error Output"

#define BF_BOUNDINGSIG 0x01
#define BF_FREEBLOCK   0x02
#define BF_LARGEALLOC  0x04
#define BF_OUTSIDEHEAP 0x08
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

// addr:234990 lines:199-207
static inline BOOL CheckInitialized() {
  if (!s_initialized) {
    SMemInitialize();
  }

  return s_initialized;
}

// addr:2349B0 lines:212-219
static inline void FatalError(DWORD errorcode, LPCSTR filename, int linenumber) {
  printf("%s:%d %x\n", filename, linenumber, errorcode);
  ExitProcess(1);
}

// addr:235B10 lines:223-230
static inline BLOCKPTR GetBlockPtrByPtr(LPVOID ptr) {
  if (!ptr) {
    return NULL;
  }

  BLOCKPTR blockptr = (BLOCKPTR)ptr - 1;
  if (blockptr->flags & BF_OUTSIDEHEAP) {
    blockptr = *(BLOCKPTR *)((LPBYTE)blockptr - sizeof(BLOCKPTR));
  }

  return blockptr;
}

// addr:2363D0 lines:233-236
static inline HSHEAP GetHandleByBlockPtr(BLOCKPTR blockptr) {
  HEAPPTR heapptr = (HEAPPTR)((DWORD)(blockptr->heapaddr) << 16);
  return heapptr->handle;
}

// addr:2349E0 lines:240-284
static inline HSHEAP GetHandleByCaller(LPCSTR filename, int linenumber) {
  return NULL;
}

// addr:235B30 lines:287-294
static inline LPVOID GetPtrByBlockPtr(BLOCKPTR blockptr) {
  if (!blockptr) {
    return NULL;
  }

  LPVOID ptr = blockptr + 1;
  if (blockptr->flags & BF_LARGEALLOC) {
    ptr = *(LPVOID *)ptr;
  }

  return ptr;
}

// addr:234A60 lines:297-299
static inline DWORD GetSlotByHandle(HSHEAP handle) {
  return (DWORD)handle & (TABLESIZE - 1);
}

// addr:236240 lines:303-313
static inline HEAPPTR LockHeapByBlockPtr(BLOCKPTR blockptr, HLOCKEDHEAP *lockedhandle) {
  return NULL;
}

// addr:234A70 lines:318-341
static inline HEAPPTR LockHeapByHandle(HSHEAP handle, HLOCKEDHEAP *lockedhandle, BOOL heapmustexist) {
  return NULL;
}

// addr:235ED0 lines:345-374
static inline HEAPPTR LockNextHeapByHandle(HSHEAP prevheap, HLOCKEDHEAP *lockedhandle) {
  return NULL;
}

// addr:2355E0 lines:379-387
static inline void Warning(DWORD errorcode, LPCSTR filename, int linenumber) {
  printf("%s:%d %x\n", filename, linenumber, errorcode);
}

// addr:234AD0 lines:390-396
static inline void UnlockHeap(HLOCKEDHEAP *lockedhandle) {
  if (*(DWORD *)lockedhandle != -1) {
    DWORD slot = (DWORD)lockedhandle;
    LeaveCriticalSection(&s_critsect[slot]);

    *(DWORD *)lockedhandle = -1;
  }
}

// --------------------------------
// Block allocation/deallocation functions
// --------------------------------

static inline void CombineFreeBlocks(HEAPPTR heapptr);
static void        ComputePageSize();
static inline void FreeHeap(HEAPPTR *nextptr);
static inline void FreeHeapBlock(HEAPPTR heapptr, BLOCKPTR block);
static inline void SubdivideBlock(HEAPPTR heapptr, BLOCKPTR blockptr, LPDWORD blocksize, LPDWORD padding);

// addr:234B00 lines:443-550
static inline HEAPPTR AllocateHeap(LPCSTR filename, int linenumber, HSHEAP handle, DWORD slot, DWORD chunksize, DWORD commitsize, DWORD reservesize) {
  return NULL;
}

// addr:234E30 lines:555-744
static inline LPVOID AllocateHeapBlock(HEAPPTR heapptr, DWORD bytes, BYTE baseflags) {
  return NULL;
}

// addr:235B50 lines:750-789
static inline BOOL CheckValidBlock(LPVOID ptr, BOOL displayerror, LPCSTR filename, int linenumber) {
  return FALSE;
}

// addr:2350F0 lines:792-845
static inline void CombineFreeBlocks(HEAPPTR heapptr) {
}

// addr:2351C0 lines:848-851
static inline DWORD ComputeFreeSlot(DWORD bytes) {
  return -1;
}

// addr:2351D0 lines:858-866
static inline void ComputeBlockSize(DWORD bytes, LPDWORD blocksize, LPDWORD padding, LPBOOL largeAlloc, LPBOOL boundingSig) {
}

// addr:235240 lines:869-880
static void ComputePageSize() {
}

// addr:235510 lines:883-926
static inline HEAPPTR *DestroyHeap(HEAPPTR *nextptr) {
  return NULL;
}

// addr:235270 lines:933-941
static inline void FillBlockHeaderAndSignatures(HEAPPTR heapptr, BLOCKPTR blockptr, DWORD blockSize, DWORD padding, BYTE flags) {
}

// addr:234C90 lines:944-959
static void FreeEmptyHeaps() {
}

// addr:234D20 lines:962-971
static inline void FreeHeap(HEAPPTR *nextptr) {
}

// addr:2352A0 lines:975-1044
static inline void FreeHeapBlock(HEAPPTR heapptr, BLOCKPTR block) {
}

// addr:235BF0 lines:1050-1070
static inline void GetBlockSize(BLOCKPTR blockptr, LPVOID ptr, LPDWORD bytes, LPDWORD overhead) {
}

// addr:235390 lines:1074-1087
static inline int GrowCommitSize(HEAPPTR heapptr, DWORD newheapsize) {
  return -1;
}

// addr:2368F0 lines:1093-1190
static inline void GrowHeapBlock(HEAPPTR heapptr, BLOCKPTR blockptr, DWORD sourceBytes, DWORD bytes) {
}

// addr:234D40 lines:1196-1244
static inline LPVOID SatisfyAllocRequest(HLOCKEDHEAP lockedhandle, HEAPPTR heapptr, DWORD flags, DWORD bytes) {
  return NULL;
}

// addr:236270 lines:1249-1285
static inline void SatisfyFreeRequest(HEAPPTR heapptr, LPVOID ptr, BLOCKPTR blockptr) {
}

// addr:2367B0 lines:1293-1365
static inline LPVOID SatisfyReAllocRequest(HLOCKEDHEAP lockedhandle, HEAPPTR heapptr, LPVOID ptr, BLOCKPTR blockptr, DWORD bytes, DWORD flags) {
  return NULL;
}

// addr:236AA0 lines:1371-1407
static inline void ShrinkHeapBlock(HEAPPTR heapptr, BLOCKPTR blockptr, DWORD sourceBytes, DWORD bytes) {
}

// addr:2353E0 lines:1413-1432
static inline void SubdivideBlock(HEAPPTR heapptr, BLOCKPTR blockptr, LPDWORD blocksize, LPDWORD padding) {
}

// --------------------------------
// Exported functions
// --------------------------------

#define CHECKINITIALIZED(name, errortype, retval)                                   \
  do                                                                                \
    if (!CheckInitialized()) {                                                      \
      errortype(STORM_ERROR_MEMORY_MANAGER_INACTIVE, name, SERR_LINECODE_FUNCTION); \
      return retval;                                                                \
    }                                                                               \
  while (0)

// addr:2348F0 lines:1529-1581
LPVOID APIENTRY SMemAlloc(DWORD bytes, LPCSTR filename, int linenumber, DWORD flags) {
  return NULL;
}

// addr:235450 lines:1588-1613
BOOL APIENTRY SMemDestroy() {
  return FALSE;
}

// addr:235610 lines:1617-1647
void APIENTRY SMemDumpState(void *outputproc, HOUTPUTCONTEXT outputcontext) {
}

// addr:235700 lines:1650-1728
BOOL APIENTRY SMemDumpStateEx(...) {
  return FALSE;
}

// addr:235890 lines:1731-1756
BOOL APIENTRY SMemMarkAllHeapsEx(...) {
  return FALSE;
}

// addr:235920 lines:1762-1843
BOOL APIENTRY SMemFindNextBlock(HSHEAP heap, LPVOID prevblock, LPVOID *nextblock, LPSMEMBLOCKDETAILS details) {
  return FALSE;
}

// addr:235C40 lines:1846-1876
BOOL APIENTRY SMemHeapGetDetails(HSHEAP heap, LPSMEMHEAPDETAILS details) {
  return FALSE;
}

// addr:235D40 lines:1881-1927
BOOL APIENTRY SMemFindNextHeap(HSHEAP prevheap, HSHEAP *nextheap, LPSMEMHEAPDETAILS details) {
  return FALSE;
}

// addr:235F70 lines:1932-1999
BOOL APIENTRY SMemFindNextHeap2(HSHEAP prevheap, HSHEAP *nextheap, LPSMEMHEAPDETAILS2 details) {
  return FALSE;
}

// addr:2361B0 lines:2005-2032
BOOL APIENTRY SMemFree(LPVOID ptr, LPCSTR filename, int linenumber, DWORD flags) {
  return FALSE;
}

// addr:2362F0 lines:2037-2046
DWORD APIENTRY SMemGetAllocated(LPDWORD allocated, LPDWORD committed, LPDWORD reserved) {
  return 0;
}

// addr:236330 lines:2050-2055
HSHEAP APIENTRY SMemGetHeapByCaller(LPCSTR filename, int linenumber) {
  return NULL;
}

// addr:236370 lines:2058-2069
HSHEAP APIENTRY SMemGetHeapByPtr(LPDWORD ptr) {
  return NULL;
}

// addr:2363E0 lines:2074-2094
DWORD APIENTRY SMemGetSize(LPDWORD ptr, LPCSTR filename, int linenumber) {
  return -1;
}

// addr:236450 lines:2099-2128
LPVOID APIENTRY SMemHeapAlloc(HSHEAP handle, DWORD flags, DWORD bytes) {
  return NULL;
}

// addr:2364D0 lines:2135-2190
HSHEAP APIENTRY SMemHeapCreate(DWORD options, DWORD initialsize, DWORD maximumsize, LPCSTR filename, int linenumber) {
  return NULL;
}

// addr:2365E0 lines:2193-2215
BOOL APIENTRY SMemHeapDestroy(HSHEAP handle) {
  return FALSE;
}

// addr:236670 lines:2220-2251
BOOL APIENTRY SMemHeapFree(HSHEAP handle, DWORD flags, LPVOID ptr) {
  return FALSE;
}

// addr:236700 lines:2257-2293
LPVOID APIENTRY SMemHeapReAlloc(HSHEAP handle, DWORD flags, LPVOID ptr, DWORD bytes) {
  return NULL;
}

// addr:236B90 lines:2298-2324
DWORD APIENTRY SMemHeapSize(HSHEAP handle, DWORD flags, LPDWORD ptr) {
  return -1;
}

// addr:236C10 lines:2327-2363
void APIENTRY SMemInitialize() {
}

// addr:236D20 lines:2366-2383
BOOL APIENTRY SMemIsValidPointer(LPDWORD address, DWORD size, BOOL forWriting) {
  return FALSE;
}

// addr:236D60 lines:2390-2448
LPVOID APIENTRY SMemReAlloc(LPVOID ptr, DWORD bytes, LPCSTR filename, int linenumber, DWORD flags) {
  return NULL;
}

// addr:236DF0 lines:2451-2460
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

// addr:236E50 lines:2463-2475
void APIENTRY SMemTrace(LPCSTR format) {
  // trace logging was disabled in this build
}

#ifdef WOWHOOK
void StormHook_SMem() {
  WOWHOOK_ATTACH(CheckInitialized, 0x234990);
  WOWHOOK_ATTACH(FatalError, 0x2349B0);
  WOWHOOK_ATTACH(GetBlockPtrByPtr, 0x235B10);
  WOWHOOK_ATTACH(GetHandleByBlockPtr, 0x2363D0);
  WOWHOOK_ATTACH(GetHandleByCaller, 0x2349E0);
  WOWHOOK_ATTACH(GetPtrByBlockPtr, 0x235B30);
  WOWHOOK_ATTACH(GetSlotByHandle, 0x234A60);
  WOWHOOK_ATTACH(LockHeapByBlockPtr, 0x236240);
  WOWHOOK_ATTACH(LockHeapByHandle, 0x234A70);
  WOWHOOK_ATTACH(LockNextHeapByHandle, 0x235ED0);
  WOWHOOK_ATTACH(Warning, 0x2355E0);
  WOWHOOK_ATTACH(UnlockHeap, 0x234AD0);

  WOWHOOK_ATTACH(AllocateHeap, 0x234B00);
  WOWHOOK_ATTACH(AllocateHeapBlock, 0x234E30);
  WOWHOOK_ATTACH(CheckValidBlock, 0x235B50);
  WOWHOOK_ATTACH(CombineFreeBlocks, 0x2350F0);
  WOWHOOK_ATTACH(ComputeFreeSlot, 0x2351C0);
  WOWHOOK_ATTACH(ComputeBlockSize, 0x2351D0);
  WOWHOOK_ATTACH(ComputePageSize, 0x235240);
  WOWHOOK_ATTACH(DestroyHeap, 0x235510);
  WOWHOOK_ATTACH(FillBlockHeaderAndSignatures, 0x235270);
  WOWHOOK_ATTACH(FreeEmptyHeaps, 0x234C90);
  WOWHOOK_ATTACH(FreeHeap, 0x234D20);
  WOWHOOK_ATTACH(FreeHeapBlock, 0x2352A0);
  WOWHOOK_ATTACH(GetBlockSize, 0x235BF0);
  WOWHOOK_ATTACH(GrowCommitSize, 0x235390);
  WOWHOOK_ATTACH(GrowHeapBlock, 0x2368F0);
  WOWHOOK_ATTACH(SatisfyAllocRequest, 0x234D40);
  WOWHOOK_ATTACH(SatisfyFreeRequest, 0x236270);
  WOWHOOK_ATTACH(SatisfyReAllocRequest, 0x2367B0);
  WOWHOOK_ATTACH(ShrinkHeapBlock, 0x236AA0);
  WOWHOOK_ATTACH(SubdivideBlock, 0x2353E0);

  WOWHOOK_ATTACH(SMemAlloc, 0x2348F0);
  WOWHOOK_ATTACH(SMemDestroy, 0x235450);
  WOWHOOK_ATTACH(SMemDumpState, 0x235610);
  WOWHOOK_ATTACH(SMemDumpStateEx, 0x235700);
  WOWHOOK_ATTACH(SMemMarkAllHeapsEx, 0x235890);
  WOWHOOK_ATTACH(SMemFindNextBlock, 0x235920);
  WOWHOOK_ATTACH(SMemHeapGetDetails, 0x235C40);
  WOWHOOK_ATTACH(SMemFindNextHeap, 0x235D40);
  WOWHOOK_ATTACH(SMemFindNextHeap2, 0x235F70);
  WOWHOOK_ATTACH(SMemFree, 0x2361B0);
  WOWHOOK_ATTACH(SMemGetAllocated, 0x2362F0);
  WOWHOOK_ATTACH(SMemGetHeapByCaller, 0x236330);
  WOWHOOK_ATTACH(SMemGetHeapByPtr, 0x236370);
  WOWHOOK_ATTACH(SMemGetSize, 0x2363E0);
  WOWHOOK_ATTACH(SMemHeapAlloc, 0x236450);
  WOWHOOK_ATTACH(SMemHeapCreate, 0x2364D0);
  WOWHOOK_ATTACH(SMemHeapDestroy, 0x2365E0);
  WOWHOOK_ATTACH(SMemHeapFree, 0x236670);
  WOWHOOK_ATTACH(SMemHeapReAlloc, 0x236700);
  WOWHOOK_ATTACH(SMemHeapSize, 0x236B90);
  WOWHOOK_ATTACH(SMemInitialize, 0x236C10);
  WOWHOOK_ATTACH(SMemIsValidPointer, 0x236D20);
  WOWHOOK_ATTACH(SMemReAlloc, 0x236D60);
  WOWHOOK_ATTACH(SMemSetDebugFlags, 0x236DF0);
  WOWHOOK_ATTACH(SMemTrace, 0x236E50);
}
#endif
