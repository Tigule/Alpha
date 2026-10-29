#pragma once

#define LIST_UNLINKED 0
#define LIST_HEAD     1
#define LIST_TAIL     2

#define LIST_LINK_AFTER  LIST_HEAD
#define LIST_LINK_BEFORE LIST_TAIL

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

#define SERR_LINECODE_FUNCTION  -1
#define SERR_LINECODE_OBJECT    -2
#define SERR_LINECODE_HANDLE    -3
#define SERR_LINECODE_FILE      -4
#define SERR_LINECODE_EXCEPTION -5

#define REPORTRESOURCELEAK(handle)

// --------------------------------
// Error handling functions
// --------------------------------

typedef BOOL(APIENTRY *SERRHANDLER)(DWORD errorcode, LPCSTR errorstr, LPCSTR filename, int linenumber, LPCSTR description);
typedef int(APIENTRY *SERRLOGCALLBACK)(LPSTR buffer, DWORD bufferchars);

void                      SErrInitialize();
extern "C" BOOL APIENTRY  SErrDestroy();
extern "C" BOOL APIENTRY  SErrCheckDebugSymbolLibrary(BOOL warnnotfound);
extern "C" void APIENTRY  SErrPrepareAppFatal(LPCSTR filename, int linenumber);
extern "C" void __cdecl   SErrDisplayAppFatal(LPCSTR format, ...);
extern "C" BOOL APIENTRY  SErrDisplayError(DWORD errorcode, LPCSTR filename, int linenumber, LPCSTR description, BOOL recoverable, UINT exitcode = 1);
extern "C" BOOL __cdecl   SErrDisplayErrorFmt(DWORD errorcode, LPCSTR filename, int linenumber, BOOL recoverable, UINT exitcode, LPCSTR format, ...);
extern "C" BOOL APIENTRY  SErrGetErrorStr(DWORD errorcode, char *buffer, DWORD bufferchars);
extern "C" DWORD APIENTRY SErrGetLastError();
extern "C" BOOL APIENTRY  SErrIsDisplayingError();
extern "C" void APIENTRY  SErrRegisterHandler(SERRHANDLER handler);
extern "C" BOOL APIENTRY  SErrRegisterMessageSource(WORD facility, HINSTANCE module, LPVOID reserved);
extern "C" void APIENTRY  SErrReportResourceLeak(LPCSTR handlename);
extern "C" void APIENTRY  SErrReportNamedResourceLeak(LPCSTR handlename, LPCSTR name);
extern "C" void APIENTRY  SErrSetLastError(DWORD errorcode);
extern "C" void APIENTRY  SErrSetLogCallback(SERRLOGCALLBACK cb);
extern "C" void APIENTRY  SErrSetLogTitleString(LPCSTR title);
extern "C" BOOL APIENTRY  SErrGetLogLastPath(char *buf, int size);
extern "C" void APIENTRY  SErrSuppressErrors(BOOL suppress);
extern "C" void APIENTRY  SErrUnregisterHandler(SERRHANDLER handler);
extern "C" void APIENTRY  SErrRegisterThread(HANDLE thread, DWORD threadid);
extern "C" void APIENTRY  SErrUnregisterThread(HANDLE thread, DWORD threadid);
extern "C" void APIENTRY  SErrLogRegisteredThreads(LPCSTR description, LPCSTR suffix);
extern "C" void APIENTRY  SErrLogThreads(HANDLE *threads, LPDWORD threadids, int numthreads, LPCSTR description, LPCSTR suffix);
extern "C" void APIENTRY  SErrStartWatchdog(DWORD freezeSeconds, BOOL checkKeyboard);
extern "C" void APIENTRY  SErrPauseWatchdog();
extern "C" void APIENTRY  SErrResumeWatchdog();
extern "C" void APIENTRY  SErrStopWatchdog();
extern "C" void APIENTRY  SErrPingWatchdog();
extern "C" void APIENTRY  SErrCatchUnhandledExceptions();

// --------------------------------
// Event functions
// --------------------------------

typedef void(APIENTRY *SEVTHANDLER)(LPVOID data);

extern "C" BOOL APIENTRY SEvtBreakHandlerChain(LPVOID data);
extern "C" BOOL APIENTRY SEvtDestroy();
extern "C" BOOL APIENTRY SEvtDispatch(DWORD type, DWORD subtype, DWORD id, LPVOID data);
extern "C" BOOL APIENTRY SEvtPopState(DWORD type, DWORD subtype);
extern "C" BOOL APIENTRY SEvtPushState(DWORD type, DWORD subtype);
extern "C" BOOL APIENTRY SEvtRegisterHandler(DWORD type, DWORD subtype, DWORD id, DWORD flags, SEVTHANDLER handler);
extern "C" BOOL APIENTRY SEvtUnregisterHandler(DWORD type, DWORD subtype, DWORD id, SEVTHANDLER handler);
extern "C" BOOL APIENTRY SEvtUnregisterType(DWORD type, DWORD subtype);

// --------------------------------
// Message functions
// --------------------------------

typedef BOOL(APIENTRY *SMSGIDLEPROC)(DWORD count);

typedef struct _PARAMS {
  HWND   window;
  UINT   message;
  UINT   wparam;
  LONG   lparam;
  UINT   notifycode;
  LPVOID extra;
  BOOL   useresult;
  LONG   result;
} SMSGPARAMS, *LPSMSGPARAMS;

typedef void(APIENTRY *SMSGHANDLER)(SMSGPARAMS *params);

extern "C" BOOL APIENTRY    SMsgBreakHandlerChain(SMSGPARAMS *params);
extern "C" BOOL APIENTRY    SMsgDestroy();
extern "C" BOOL APIENTRY    SMsgDispatchMessage(HWND window, UINT message, UINT wparam, LONG lparam, BOOL *useresult, LONG *result);
extern "C" int APIENTRY     SMsgDoMessageLoop(SMSGIDLEPROC idleproc, BOOL cleanuponquit);
extern "C" HWND APIENTRY    SMsgGetDefaultWindow();
extern "C" BOOL APIENTRY    SMsgGetDefaultWindowRect(LPRECT rect);
extern "C" WNDPROC APIENTRY SMsgGetGenericWndProc(DWORD id);
extern "C" BOOL APIENTRY    SMsgPopRegisterState(HWND window);
extern "C" BOOL APIENTRY    SMsgPushRegisterState(HWND window);
extern "C" BOOL APIENTRY    SMsgRegisterCommand(HWND window, UINT id, SMSGHANDLER handler);
extern "C" BOOL APIENTRY    SMsgRegisterSysCommand(HWND window, UINT id, SMSGHANDLER handler);
extern "C" BOOL APIENTRY    SMsgRegisterKeyDown(HWND window, UINT id, SMSGHANDLER handler);
extern "C" BOOL APIENTRY    SMsgRegisterKeyUp(HWND window, UINT id, SMSGHANDLER handler);
extern "C" BOOL APIENTRY    SMsgRegisterMessage(HWND window, UINT id, SMSGHANDLER handler);
extern "C" BOOL APIENTRY    SMsgSetDefaultWindow(HWND window);
extern "C" void APIENTRY    SMsgSetDefaultWindowRect(const RECT *rect);
extern "C" BOOL APIENTRY    SMsgUnregisterCommand(HWND window, UINT id, SMSGHANDLER handler);
extern "C" BOOL APIENTRY    SMsgUnregisterSysCommand(HWND window, UINT id, SMSGHANDLER handler);
extern "C" BOOL APIENTRY    SMsgUnregisterKeyDown(HWND window, UINT id, SMSGHANDLER handler);
extern "C" BOOL APIENTRY    SMsgUnregisterKeyUp(HWND window, UINT id, SMSGHANDLER handler);
extern "C" BOOL APIENTRY    SMsgUnregisterMessage(HWND window, UINT id, SMSGHANDLER handler);

// --------------------------------
// Command-line functions
// --------------------------------

#define SCMD_ARG_FLAGGED  (0 << 24)
#define SCMD_ARG_OPTIONAL (1 << 24)
#define SCMD_ARG_REQUIRED (2 << 24)
#define SCMD_ARG_MASK     (SCMD_ARG_FLAGGED | SCMD_ARG_OPTIONAL | SCMD_ARG_REQUIRED)

#define SCMD_BOOL_SET   0
#define SCMD_BOOL_CLEAR 1
#define SCMD_BOOL_MASK  (SCMD_BOOL_SET | SCMD_BOOL_CLEAR)

#define SCMD_CASESENSITIVE (0x01 << 8)

#define SCMD_NUM_UNSIGNED 0
#define SCMD_NUM_SIGNED   1
#define SCMD_NUM_MASK     (SCMD_NUM_UNSIGNED | SCMD_NUM_SIGNED)

#define SCMD_TYPE_BOOL    (0 << 16)
#define SCMD_TYPE_NUMERIC (1 << 16)
#define SCMD_TYPE_STRING  (2 << 16)
#define SCMD_TYPE_MASK    (SCMD_TYPE_BOOL | SCMD_TYPE_NUMERIC | SCMD_TYPE_STRING)

typedef struct _CMDERROR {
  DWORD  errorcode;
  LPCSTR itemstr;
  LPCSTR errorstr;
} CMDERROR, *CMDERRORPTR;

typedef CMDERROR  SCMDERROR;
typedef CMDERROR *LPSCMDERROR;

typedef struct _CMDPARAMS {
  DWORD  flags;
  DWORD  id;
  LPCSTR name;
  LPVOID variable;
  DWORD  setvalue;
  DWORD  setmask;
  union {
    BOOL   boolvalue;
    LONG   signedvalue;
    DWORD  unsignedvalue;
    LPCSTR stringvalue;
  };
} CMDPARAMS, *CMDPARAMSPTR;

typedef BOOL(APIENTRY *SCMDPROCESSCALLBACK)(LPCSTR value);
typedef BOOL(APIENTRY *SCMDCALLBACK)(CMDPARAMSPTR params, LPCSTR value);
typedef void(APIENTRY *SCMDERRORCALLBACK)(CMDERRORPTR error);
typedef SCMDCALLBACK SCMDARGCALLBACK;

typedef struct _ARGLIST {
  DWORD        flags;
  DWORD        id;
  LPCSTR       name;
  SCMDCALLBACK callback;
} ARGLIST, *LPARGLIST;

extern "C" BOOL APIENTRY  SCmdCheckId(DWORD id);
extern "C" BOOL APIENTRY  SCmdDestroy();
extern "C" BOOL APIENTRY  SCmdGetBool(DWORD id);
extern "C" DWORD APIENTRY SCmdGetNum(DWORD id);
extern "C" BOOL APIENTRY  SCmdGetString(DWORD id, char *buffer, DWORD bufferchars);
extern "C" BOOL APIENTRY  SCmdGetStringAlloc(DWORD id, char **buffer);
extern "C" BOOL APIENTRY  SCmdProcess(LPCSTR cmdline, int skipprogname, SCMDPROCESSCALLBACK extracallback, SCMDERRORCALLBACK errorcallback);
extern "C" BOOL APIENTRY  SCmdProcessCommandLine(SCMDPROCESSCALLBACK extracallback, SCMDERRORCALLBACK errorcallback);
extern "C" BOOL APIENTRY  SCmdRegisterArgList(const ARGLIST *listptr, DWORD numargs);
extern "C" BOOL APIENTRY  SCmdRegisterArgument(
    DWORD        flags,
    DWORD        id,
    LPCSTR       name,
    LPVOID       variableptr,
    DWORD        variablebytes,
    DWORD        setvalue,
    DWORD        setmask,
    SCMDCALLBACK callback
);

// --------------------------------
// Compression functions
// --------------------------------

#define SCOMP_HUFFMAN          0x00000001
#define SCOMP_ZLIB             0x00000002
#define SCOMP_PKWARE           0x00000008
#define SCOMP_IMA_ADPCM_MONO   0x00000040
#define SCOMP_IMA_ADPCM_STEREO 0x00000080

extern "C" int APIENTRY
SCompCompress(LPVOID dest, DWORD *destsize, LPCVOID source, DWORD sourcesize, DWORD compressiontypes, DWORD hint, DWORD optimization);
extern "C" int APIENTRY SCompDecompress(LPVOID dest, DWORD *destsize, LPCVOID source, DWORD sourcesize);
int APIENTRY            SCompDecompress2(LPVOID dest, DWORD *destsize, LPCVOID source, DWORD sourcesize, LPCSTR filename);
BOOL                    SCompPkwareDecompressBuffer(BYTE *dest, DWORD *destsize, const BYTE *source, DWORD sourcesize);
extern "C" int APIENTRY SCompDestroy();

// --------------------------------
// Big integer functions
// --------------------------------

class BigData;
template <class T>
class TSGrowableArray;

extern "C" void APIENTRY SBigAdd(BigData *a, const BigData &b, const BigData &c);
extern "C" void APIENTRY SBigAnd(BigData *a, const BigData &b, const BigData &c);
extern "C" void APIENTRY SBigBitLen(BigData *a, UINT *bits);
extern "C" int APIENTRY  SBigCompare(const BigData &b, const BigData &c);
extern "C" void APIENTRY SBigCopy(BigData *a, const BigData &b);
extern "C" void APIENTRY SBigDec(BigData *a, const BigData &b);
extern "C" void APIENTRY SBigDel(BigData *num);
extern "C" void APIENTRY SBigDiv(BigData *a, const BigData &b, const BigData &c);
extern "C" void APIENTRY SBigFindPrime(BigData *a, UINT bits, const BigData &c, const BigData &d);
extern "C" void APIENTRY SBigFromBinary(BigData *num, LPCVOID data, UINT bytes);
extern "C" void APIENTRY SBigFromStr(BigData *num, LPCSTR str);
extern "C" void APIENTRY SBigFromStream(BigData *num, LPCVOID data, UINT maxBytes, UINT *bytes);
extern "C" void APIENTRY SBigFromUnsigned(BigData *num, UINT val);
extern "C" void APIENTRY SBigGcd(BigData *a, const BigData &b, const BigData &c);
extern "C" void APIENTRY SBigInc(BigData *a, const BigData &b);
extern "C" void APIENTRY SBigInvMod(BigData *a, const BigData &b, const BigData &c);
extern "C" int APIENTRY  SBigIsEven(const BigData &a);
extern "C" int APIENTRY  SBigIsOdd(const BigData &a);
extern "C" int APIENTRY  SBigIsOne(const BigData &a);
extern "C" int APIENTRY  SBigIsPrime(const BigData &a);
extern "C" int APIENTRY  SBigIsZero(const BigData &a);
extern "C" void APIENTRY SBigMod(BigData *a, const BigData &b, const BigData &c);
extern "C" void APIENTRY SBigMul(BigData *a, const BigData &b, const BigData &c);
extern "C" void APIENTRY SBigMulMod(BigData *a, const BigData &b, const BigData &c, const BigData &d);
extern "C" void APIENTRY SBigNew(BigData **num);
extern "C" void APIENTRY SBigNot(BigData *a, const BigData &b);
extern "C" void APIENTRY SBigOr(BigData *a, const BigData &b, const BigData &c);
extern "C" void APIENTRY SBigPow(BigData *a, const BigData &b, UINT c);
extern "C" void APIENTRY SBigPowMod(BigData *a, const BigData &b, const BigData &c, const BigData &d);
extern "C" void APIENTRY SBigRand(BigData *a, const BigData &b, BigData *seed);
extern "C" void APIENTRY SBigSet2Exp(BigData *a, UINT b);
extern "C" void APIENTRY SBigSetOne(BigData *a);
extern "C" void APIENTRY SBigSetZero(BigData *a);
extern "C" void APIENTRY SBigShl(BigData *a, const BigData &b, UINT bits);
extern "C" void APIENTRY SBigShr(BigData *a, const BigData &b, UINT bits);
extern "C" void APIENTRY SBigSquare(BigData *a, const BigData &b);
extern "C" void APIENTRY SBigSub(BigData *a, const BigData &b, const BigData &c);
extern "C" void APIENTRY SBigToBinaryArray(const BigData &num, TSGrowableArray<BYTE> *array, int append);
extern "C" void APIENTRY SBigToBinaryBuffer(const BigData &num, LPVOID data, UINT maxBytes, UINT *bytes);
extern "C" void APIENTRY SBigToBinaryPtr(const BigData &num, LPCVOID *data, UINT *bytes);
extern "C" void APIENTRY SBigToStrArray(const BigData &num, TSGrowableArray<char> *array, int append);
extern "C" void APIENTRY SBigToStrBuffer(const BigData &num, char *str, UINT chars);
extern "C" void APIENTRY SBigToStrPtr(const BigData &num, LPCSTR *str);
extern "C" void APIENTRY SBigToStreamArray(const BigData &num, TSGrowableArray<BYTE> *array, int append);
extern "C" void APIENTRY SBigToStreamBuffer(const BigData &num, LPVOID data, UINT maxBytes, UINT *bytes);
extern "C" void APIENTRY SBigToStreamPtr(const BigData &num, LPCVOID *data, UINT *bytes);
extern "C" void APIENTRY SBigToUnsigned(const BigData &num, UINT *val);
extern "C" void APIENTRY SBigXor(BigData *a, const BigData &b, const BigData &c);

// --------------------------------
// Machine-state logging
// --------------------------------

typedef void(__cdecl *LOGMACHINESTATEPROC)(LPVOID param, LPCSTR format, ...);

void LogComputerInfoHeader(UINT logOptions, LOGMACHINESTATEPROC logLineProc, LPVOID logLineProcParam, LPCSTR title, SYSTEMTIME *localTime);
void LogMachineState(UINT logOptions, LOGMACHINESTATEPROC logLineProc, LPVOID logLineProcParam, UINT stackFramesToSkip, CONTEXT *context);

// --------------------------------
// Utility functions
// --------------------------------

int __cdecl vsnoprintf(char *out, int outSize, LPCSTR format, va_list argumentList);
int __cdecl vsoprintf(char *out, LPCSTR format, va_list argumentList);
int __cdecl snoprintf(char *out, int maxchars, LPCSTR format, ...);
int __cdecl soprintf(char *out, LPCSTR format, ...);

DWORD                     CrcBuffer(LPCVOID buffer, DWORD len, DWORD *pcrc, DWORD stage);
extern "C" DWORD APIENTRY SCrcBuffer(LPCVOID buffer, DWORD len, DWORD *pcrc, DWORD stage);

class Sha1 {
 public:
  enum {
    DIGEST = 0x14,
    DATA_SIZE = 0x40,
    DATA_MASK = 0x3F
  };

 private:
  ULONGLONG m_size;
  DWORD     m_hash[5];
  BYTE      m_data[DATA_SIZE];

  static void Pump(DWORD hash[], const BYTE data[]);

 public:
  void Initialize();
  void Append(LPCVOID _data, DWORD size);
  void Append(LPCSTR data);
  void Finalize(BYTE hash[]);

  static void Hash(BYTE *hash, LPCVOID data, DWORD size);
  static void Hash(BYTE *hash, LPCSTR data);
};

// clang-format off
#ifdef ASSERTIONS_ENABLED
#define ASSERT(a)                         \
  if (!(a))                              \
    SErrDisplayError(STORM_ERROR_ASSERTION, \
                     __FILE__,           \
                     __LINE__,           \
                     #a,                 \
                     FALSE)
#define FATALASSERT(a)                    \
  if (!(a))                              \
    SErrDisplayError(STORM_ERROR_ASSERTION, \
                     __FILE__,           \
                     __LINE__,           \
                     #a,                 \
                     FALSE)
#define FATALERROR(args)                  \
  if (1) {                                \
    SErrPrepareAppFatal(__FILE__, __LINE__); \
    SErrDisplayAppFatal args;             \
  }
#define VALIDATEBEGIN
#define VALIDATE(a)                          \
  if (!(a)) {                                \
    SErrPrepareAppFatal(__FILE__, __LINE__); \
    SErrDisplayAppFatal(#a);                 \
    goto validatefailed;                     \
  }
#else
#define ASSERT(a)
#define FATALASSERT(a)
#define FATALERROR(args)                  \
  if (1) {                                \
    SErrDisplayAppFatal args;             \
  }
#define VALIDATEBEGIN
#define VALIDATE(a)                          \
  if (!(a)) {                                \
    goto validatefailed;                     \
  }
#endif
#define VALIDATEANDBLANK(a) \
  VALIDATE(a)               \
  *(a) = 0;
#define VALIDATEEND                            \
  if (0) {                                     \
  validatefailed:                              \
    SErrSetLastError(ERROR_INVALID_PARAMETER); \
    return FALSE;                              \
  }
#define VALIDATEENDVOID                        \
  if (0) {                                     \
  validatefailed:                              \
    SErrSetLastError(ERROR_INVALID_PARAMETER); \
    return;                                    \
  }
// clang-format on

// --------------------------------
// Memory allocation functions
// --------------------------------

#define SMEM_FLAG_ZEROMEMORY        0x00000008
#define SMEM_FLAG_PRESERVEONDESTROY 0x08000000

DECLARE_STRICT_HANDLE(HSHEAP);
DECLARE_STRICT_HANDLE(HOUTPUTCONTEXT);

typedef enum SMEMREPORTTYPE {
  SMEM_REPORT_BY_CALLER = 0,
  SMEM_REPORT_HISTOGRAM = 1
} SMEMREPORTTYPE;

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

typedef struct SMemReportByCallerInfo {
  DWORD numSubHeaps;
  DWORD cumulativeAllocs;
  DWORD cumulativeFrees;
  DWORD cumulativeReallocs;
  DWORD allocatedBlocks;
  DWORD allocatedBytes;
  DWORD committedBytes;
  DWORD reservedBytes;
  DWORD mark_allocatedBlocks;
  DWORD mark_allocatedBytes;
  DWORD mark_committedBytes;
  DWORD mark_cumulativeAllocs;
  DWORD mark_cumulativeFrees;
  DWORD mark_cumulativeReallocs;
  int   lineNumber;
  char  fileName[0x100];
} SMemReportByCallerInfo;

typedef void(APIENTRY *SMEMREPORTPROC)(HOUTPUTCONTEXT outputcontext, LPCSTR report);
typedef void(APIENTRY *SMEMDUMPPROC)(HOUTPUTCONTEXT outputcontext, LPCSTR text);

extern "C" LPVOID APIENTRY SMemAlloc(DWORD bytes, LPCSTR filename = NULL, int linenumber = 0, DWORD flags = 0);
extern "C" BOOL APIENTRY   SMemDestroy();
extern "C" BOOL APIENTRY   SMemDumpState(SMEMDUMPPROC outputproc, HOUTPUTCONTEXT outputcontext);
BOOL APIENTRY              SMemDumpStateEx(char *arglist);
void                       SMemGenerateReport(SMEMREPORTTYPE reporttype, SMEMREPORTPROC outputproc, HOUTPUTCONTEXT outputcontext);
BOOL APIENTRY              SMemMarkAllHeapsEx(char *arglist);
extern "C" BOOL APIENTRY   SMemFindNextBlock(HSHEAP heap, LPVOID prevblock, LPVOID *nextblock, LPSMEMBLOCKDETAILS details);
extern "C" void APIENTRY   SMemHeapGetDetails(HSHEAP heap, LPSMEMHEAPDETAILS details);
extern "C" BOOL APIENTRY   SMemFindNextHeap(HSHEAP prevheap, HSHEAP *nextheap, LPSMEMHEAPDETAILS details);
BOOL APIENTRY              SMemFindNextHeap2(HSHEAP prevheap, HSHEAP *nextheap, LPSMEMHEAPDETAILS2 details);
extern "C" BOOL APIENTRY   SMemFree(LPVOID ptr, LPCSTR filename = NULL, int linenumber = 0, DWORD flags = 0);
extern "C" DWORD APIENTRY  SMemGetAllocated(LPDWORD allocated, LPDWORD committed, LPDWORD reserved);
extern "C" HSHEAP APIENTRY SMemGetHeapByCaller(LPCSTR filename, int linenumber);
extern "C" HSHEAP APIENTRY SMemGetHeapByPtr(LPVOID ptr);
extern "C" DWORD APIENTRY  SMemGetSize(LPVOID ptr, LPCSTR filename, int linenumber);
extern "C" LPVOID APIENTRY SMemHeapAlloc(HSHEAP handle, DWORD flags, DWORD bytes);
extern "C" HSHEAP APIENTRY SMemHeapCreate(DWORD options, DWORD initialsize, DWORD maximumsize, LPCSTR filename, int linenumber);
extern "C" BOOL APIENTRY   SMemHeapDestroy(HSHEAP handle);
extern "C" BOOL APIENTRY   SMemHeapFree(HSHEAP handle, DWORD flags, LPVOID ptr);
extern "C" LPVOID APIENTRY SMemHeapReAlloc(HSHEAP handle, DWORD flags, LPVOID ptr, DWORD bytes);
extern "C" DWORD APIENTRY  SMemHeapSize(HSHEAP handle, DWORD flags, LPVOID ptr);
extern "C" void APIENTRY   SMemInitialize();
extern "C" BOOL APIENTRY   SMemIsValidPointer(LPCVOID address, DWORD size, BOOL forWriting);
extern "C" LPVOID APIENTRY SMemReAlloc(LPVOID ptr, DWORD bytes, LPCSTR filename = NULL, int linenumber = 0, DWORD flags = 0);
extern "C" void APIENTRY   SMemSetDebugFlags(DWORD flags, DWORD changeMask);
extern "C" void __cdecl    SMemTrace(LPCSTR format, ...);

#if defined(_MSC_VER)
inline void __cdecl operator delete(LPVOID ptr) {
  if (ptr) {
    SMemFree(ptr, "delete", -1, 0);
  }
}

inline LPVOID __cdecl operator new(size_t bytes) {
  return SMemAlloc(bytes, "new", -1, 0);
}

inline void __cdecl operator delete[](LPVOID ptr) {
  if (ptr) {
    SMemFree(ptr, "delete[]", -1, 0);
  }
}

inline LPVOID __cdecl operator new[](size_t bytes) {
  return SMemAlloc(bytes, "new[]", -1, 0);
}

inline LPVOID __cdecl operator new[](size_t bytes, LPCSTR filename, int linenumber) {
  return SMemAlloc(bytes, filename, linenumber, 0);
}
#endif

#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
inline LPVOID __cdecl operator new(size_t, LPVOID ptr) throw() {
  return ptr;
}

inline void __cdecl operator delete(LPVOID, LPVOID) {
}
#endif
