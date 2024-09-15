#pragma once

#ifndef _WIN32
#include <otherwin32.h>
#else
// clang-format off
#include <windows.h>
#include <StormHook.h>
// clang-format on

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#endif

#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define DECLARE_STRICT_HANDLE(name) \
  typedef struct name##__ {         \
    int unused;                     \
  } *name
#define DECLARE_DERIVED_HANDLE(name, base)    \
  typedef struct name##__ : public base##__ { \
    int unused;                               \
  } *name

// --------------------------------
// Error codes
// --------------------------------

#define STORMFAC         0x510
#define STORMERROR(code) (0x80000000 | (STORMFAC << 16) | ((code) & 0xFFFF))

#define STORM_ERROR_ASSERTION               STORMERROR(0)
#define STORM_ERROR_BAD_ARGUMENT            STORMERROR(101)
#define STORM_ERROR_GAME_ALREADY_STARTED    STORMERROR(102)
#define STORM_ERROR_GAME_FULL               STORMERROR(103)
#define STORM_ERROR_GAME_NOT_FOUND          STORMERROR(104)
#define STORM_ERROR_GAME_TERMINATED         STORMERROR(105)
#define STORM_ERROR_INVALID_PLAYER          STORMERROR(106)
#define STORM_ERROR_NO_MESSAGES_WAITING     STORMERROR(107)
#define STORM_ERROR_NOT_ARCHIVE             STORMERROR(108)
#define STORM_ERROR_NOT_ENOUGH_ARGUMENTS    STORMERROR(109)
#define STORM_ERROR_NOT_IMPLEMENTED         STORMERROR(110)
#define STORM_ERROR_NOT_IN_ARCHIVE          STORMERROR(111)
#define STORM_ERROR_NOT_IN_GAME             STORMERROR(112)
#define STORM_ERROR_NOT_INITIALIZED         STORMERROR(113)
#define STORM_ERROR_NOT_PLAYING             STORMERROR(114)
#define STORM_ERROR_NOT_REGISTERED          STORMERROR(115)
#define STORM_ERROR_REQUIRES_CODEC          STORMERROR(116)
#define STORM_ERROR_REQUIRES_DDRAW          STORMERROR(117)
#define STORM_ERROR_REQUIRES_DSOUND         STORMERROR(118)
#define STORM_ERROR_REQUIRES_UPGRADE        STORMERROR(119)
#define STORM_ERROR_STILL_ACTIVE            STORMERROR(120)
#define STORM_ERROR_VERSION_MISMATCH        STORMERROR(121)
#define STORM_ERROR_MEMORY_ALREADY_FREED    STORMERROR(122)
#define STORM_ERROR_MEMORY_CORRUPT          STORMERROR(123)
#define STORM_ERROR_MEMORY_INVALID_BLOCK    STORMERROR(124)
#define STORM_ERROR_MEMORY_MANAGER_INACTIVE STORMERROR(125)
#define STORM_ERROR_MEMORY_NEVER_RELEASED   STORMERROR(126)
#define STORM_ERROR_HANDLE_NEVER_RELEASED   STORMERROR(127)
#define STORM_ERROR_ACCESS_OUT_OF_BOUNDS    STORMERROR(128)
#define STORM_ERROR_MEMORY_NULL_POINTER     STORMERROR(129)

// --------------------------------
// Error handling functions
// --------------------------------

#define SERR_LINECODE_FUNCTION -1
#define SERR_LINECODE_OBJECT   -2
#define SERR_LINECODE_HANDLE   -3
#define SERR_LINECODE_FILE     -4

#define REPORTRESOURCELEAK(handle)

// clang-format off
#define ASSERT(a)
#define VALIDATEBEGIN \
  do {
#define VALIDATE(a) ASSERT(a)
#define VALIDATEANDBLANK(a) \
  do {                      \
    ASSERT(a);              \
    *(a) = 0;               \
  } while (0)
#define VALIDATEEND \
  } while (0)
#define VALIDATEENDVOID \
  } while (0)
// clang-format on

// --------------------------------
// Memory allocation functions
// --------------------------------

#define SMEM_FLAG_ZEROMEMORY        0x00000008
#define SMEM_FLAG_PRESERVEONDESTROY 0x08000000

DECLARE_STRICT_HANDLE(HSHEAP);
DECLARE_STRICT_HANDLE(HOUTPUTCONTEXT);

typedef struct _SMEMBLOCKDETAILS {
  DWORD  size;
  LPVOID ptr;
  BOOL   allocated;
  BOOL   valid;
  DWORD  bytes;
  DWORD  overhead;
  DWORD  flags;
} SMEMBLOCKDETAILS, *LPSMEMBLOCKDETAILS;

typedef struct _SMEMHEAPDETAILS {
  DWORD  size;
  HSHEAP handle;
  char   filename[MAX_PATH];
  int    linenumber;
  DWORD  regions;
  DWORD  committedbytes;
  DWORD  reservedbytes;
  DWORD  maximumsize;
  DWORD  allocatedblocks;
  DWORD  allocatedbytes;
} SMEMHEAPDETAILS, *LPSMEMHEAPDETAILS;

typedef struct _SMEMHEAPDETAILS2 {
  DWORD  size;
  HSHEAP handle;
  char   filename[MAX_PATH];
  int    linenumber;
  DWORD  regions;
  DWORD  committedbytes;
  DWORD  reservedbytes;
  DWORD  maximumsize;
  DWORD  cumulativeAllocs;
  DWORD  cumulativeFrees;
  DWORD  cumulativeReallocs;
  DWORD  allocatedblocks;
  DWORD  allocatedbytes;
  DWORD  mark_allocatedblocks;
  DWORD  mark_allocatedbytes;
  DWORD  mark_committedbytes;
  DWORD  mark_cumulativeAllocs;
  DWORD  mark_cumulativeFrees;
  DWORD  mark_cumulativeReallocs;
} SMEMHEAPDETAILS2, *LPSMEMHEAPDETAILS2;

extern "C" LPVOID APIENTRY SMemAlloc(DWORD bytes, LPCSTR filename = NULL, int linenumber = 0, DWORD flags = 0);
extern "C" BOOL APIENTRY   SMemDestroy();
// extern "C" void APIENTRY SMemDumpState(void *outputproc, HOUTPUTCONTEXT outputcontext);
extern "C" BOOL APIENTRY   SMemDumpStateEx(...);
BOOL APIENTRY              SMemMarkAllHeapsEx(...);
extern "C" BOOL APIENTRY   SMemFindNextBlock(HSHEAP heap, LPVOID prevblock, LPVOID *nextblock, LPSMEMBLOCKDETAILS details);
extern "C" BOOL APIENTRY   SMemHeapGetDetails(HSHEAP heap, LPSMEMHEAPDETAILS details);
extern "C" BOOL APIENTRY   SMemFindNextHeap(HSHEAP prevheap, HSHEAP *nextheap, LPSMEMHEAPDETAILS details);
BOOL APIENTRY              SMemFindNextHeap2(HSHEAP prevheap, HSHEAP *nextheap, LPSMEMHEAPDETAILS2 details);
extern "C" BOOL APIENTRY   SMemFree(LPVOID ptr, LPCSTR filename = NULL, int linenumber = 0, DWORD flags = 0);
extern "C" DWORD APIENTRY  SMemGetAllocated(LPDWORD allocated, LPDWORD committed, LPDWORD reserved);
extern "C" HSHEAP APIENTRY SMemGetHeapByCaller(LPCSTR filename, int linenumber);
extern "C" HSHEAP APIENTRY SMemGetHeapByPtr(LPDWORD ptr);
extern "C" DWORD APIENTRY  SMemGetSize(LPDWORD ptr, LPCSTR filename, int linenumber);
extern "C" LPVOID APIENTRY SMemHeapAlloc(HSHEAP handle, DWORD flags, DWORD bytes);
extern "C" HSHEAP APIENTRY SMemHeapCreate(DWORD options, DWORD initialsize, DWORD maximumsize, LPCSTR filename, int linenumber);
extern "C" BOOL APIENTRY   SMemHeapDestroy(HSHEAP handle);
extern "C" BOOL APIENTRY   SMemHeapFree(HSHEAP handle, DWORD flags, LPVOID ptr);
extern "C" LPVOID APIENTRY SMemHeapReAlloc(HSHEAP handle, DWORD flags, LPVOID ptr, DWORD bytes);
extern "C" DWORD APIENTRY  SMemHeapSize(HSHEAP handle, DWORD flags, LPDWORD ptr);
extern "C" void APIENTRY   SMemInitialize();
extern "C" BOOL APIENTRY   SMemIsValidPointer(LPDWORD address, DWORD size, BOOL forWriting);
extern "C" LPVOID APIENTRY SMemReAlloc(LPVOID ptr, DWORD bytes, LPCSTR filename, int linenumber, DWORD flags);
extern "C" void APIENTRY   SMemSetDebugFlags(DWORD flags, DWORD changeMask);
extern "C" void APIENTRY   SMemTrace(LPCSTR format);

inline void __cdecl operator delete(void *ptr) {
  if (ptr) {
    SMemFree(ptr, __FILE__, __LINE__, 0);
  }
}

inline void *__cdecl operator new(size_t bytes) {
  return SMemAlloc(bytes, __FILE__, __LINE__, 0);
}

inline void __cdecl operator delete[](void *ptr) {
  if (ptr) {
    SMemFree(ptr, __FILE__, __LINE__, 0);
  }
}

inline void *__cdecl operator new[](size_t bytes) {
  return SMemAlloc(bytes, __FILE__, __LINE__, 0);
}

#define ALLOC(bytes)     SMemAlloc(bytes, __FILE__, __LINE__, 0)
#define ALLOCZERO(bytes) SMemAlloc(bytes, __FILE__, __LINE__, SMEM_FLAG_ZEROMEMORY)
#define DEL(ptr)         delete (ptr)
#define DELIFUSED(ptr) \
  do                   \
    if (ptr)           \
      delete ptr;      \
  while (0)
#define FREE(ptr) SMemFree(ptr, __FILE__, __LINE__, 0)
#define FREEIFUSED(ptr)                     \
  do                                        \
    if (ptr)                                \
      SMemFree(ptr, __FILE__, __LINE__, 0); \
  while (0)
#define NEW(struct)     (new (SMemAlloc(sizeof(struct), __FILE__, __LINE__, 0)) struct)
#define NEWZERO(struct) (new (SMemAlloc(sizeof(struct), __FILE__, __LINE__, SMEM_FLAG_ZEROMEMORY)) struct)
