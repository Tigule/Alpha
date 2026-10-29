#pragma once

#ifndef _WIN32
#include <otherwin32.h>
#elif defined(_WINDOWS_)

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#else
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>

#define WINAPI   __stdcall
#define APIENTRY WINAPI
#define CALLBACK __stdcall
#define CONST    const
#define VOID     void

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define MAX_PATH 260

#define ERROR_INVALID_FUNCTION  1L
#define ERROR_FILE_NOT_FOUND    2L
#define ERROR_PATH_NOT_FOUND    3L
#define ERROR_INVALID_PARAMETER 87L

#define INFINITE      0xFFFFFFFF
#define WAIT_OBJECT_0 0
#define WAIT_TIMEOUT  258L

#define ERROR_SUCCESS 0L

#define FILE_BEGIN   0
#define FILE_CURRENT 1
#define FILE_END     2

#define GENERIC_READ  0x80000000L
#define GENERIC_WRITE 0x40000000L

#define CREATE_NEW        1
#define CREATE_ALWAYS     2
#define OPEN_EXISTING     3
#define OPEN_ALWAYS       4
#define TRUNCATE_EXISTING 5

#define FILE_ATTRIBUTE_NORMAL 0x00000080

#define LOWORD(l) ((WORD)(l))
#define HIWORD(l) ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))

typedef unsigned long  DWORD;
typedef int            BOOL;
typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef float          FLOAT;
typedef int            INT;
typedef unsigned int   UINT;
typedef char           CHAR;
typedef short          SHORT;
typedef long           LONG;
typedef unsigned char  UCHAR;
typedef unsigned short USHORT;
typedef unsigned long  ULONG;
typedef wchar_t        WCHAR;
typedef char           TCHAR;
typedef __int64          LONGLONG;
typedef unsigned __int64 ULONGLONG;
typedef ULONGLONG        DWORDLONG;
typedef void          *LPVOID;
typedef const void    *LPCVOID;
typedef void          *HANDLE;
typedef HANDLE         HWND;
typedef BYTE          *PBYTE;
typedef BYTE          *LPBYTE;
typedef DWORD         *PDWORD;
typedef DWORD         *LPDWORD;
typedef LONG          *PLONG;
typedef CHAR          *LPSTR;
typedef const CHAR    *LPCSTR;
typedef WCHAR         *LPWSTR;
typedef const WCHAR   *LPCWSTR;
typedef LPSTR          LPTSTR;
typedef LPCSTR         LPCTSTR;
typedef DWORD          LCID;
typedef WORD           LANGID;
typedef DWORD          COLORREF;
typedef LONG           LRESULT;
typedef LONG           LPARAM;
typedef UINT           WPARAM;

typedef struct HINSTANCE__ *HINSTANCE;

typedef LRESULT(CALLBACK *WNDPROC)(HWND, UINT, WPARAM, LPARAM);

typedef struct tagPOINT {
  LONG x;
  LONG y;
} POINT;

typedef struct tagRECT {
  LONG left;
  LONG top;
  LONG right;
  LONG bottom;
} RECT, *LPRECT;

typedef struct _FILETIME {
  DWORD dwLowDateTime;
  DWORD dwHighDateTime;
} FILETIME;

typedef struct _WIN32_FIND_DATAA {
  DWORD    dwFileAttributes;
  FILETIME ftCreationTime;
  FILETIME ftLastAccessTime;
  FILETIME ftLastWriteTime;
  DWORD    nFileSizeHigh;
  DWORD    nFileSizeLow;
  DWORD    dwReserved0;
  DWORD    dwReserved1;
  CHAR     cFileName[MAX_PATH];
  CHAR     cAlternateFileName[14];
} WIN32_FIND_DATAA;

typedef struct _OVERLAPPED  OVERLAPPED;
typedef struct _SYSTEMTIME  SYSTEMTIME;
typedef struct _CONTEXT     CONTEXT;
#endif

#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>

typedef unsigned int  uint;
typedef unsigned long ulong;

#if defined(_MSC_VER) && _MSC_VER == 1200
const UINT  INFINITY_ENCODING = 0x7F800000;
const float INFINITY = *(const float *)&INFINITY_ENCODING;
#endif

#define DECLARE_STRICT_HANDLE(name) \
  typedef struct name##__ {         \
    int unused;                     \
  } *name
#define DECLARE_DERIVED_HANDLE(name, base)    \
  typedef struct name##__ : public base##__ { \
    int unused;                               \
  } *name

#include <sapibase.h>
#include "sapi.h"

// --------------------------------
// Interlocked functions
// --------------------------------

LPVOID   SInterlockedExchangePointer(LPVOID *destPtr, LPVOID exchange);
LPVOID   SInterlockedCompareExchangePointer(LPVOID *destPtr, LPVOID exchange, LPVOID comperand);
long     SInterlockedIncrement(long *valuePtr);
long     SInterlockedDecrement(long *valuePtr);
long     SInterlockedExchangeAdd(long *valuePtr, long delta);
long     SInterlockedExchangeSub(long *valuePtr, long delta);
long     SInterlockedExchange(long *destPtr, long exchange);
long     SInterlockedCompareExchange(long *destPtr, long exchange, long comperand);
LONGLONG SInterlockedIncrement(LONGLONG *valuePtr);
LONGLONG SInterlockedDecrement(LONGLONG *valuePtr);
LONGLONG SInterlockedExchangeAdd(LONGLONG *valuePtr, long delta);
LONGLONG SInterlockedExchangeSub(LONGLONG *valuePtr, long delta);
LONGLONG SInterlockedExchangeAdd(LONGLONG *valuePtr, const LONGLONG &delta);
LONGLONG SInterlockedExchangeSub(LONGLONG *valuePtr, const LONGLONG &delta);
LONGLONG SInterlockedRead(const LONGLONG *sourcePtr);
LONGLONG SInterlockedExchange(LONGLONG *destPtr, const LONGLONG &exchange);
LONGLONG SInterlockedCompareExchange(LONGLONG *destPtr, const LONGLONG &exchange, const LONGLONG &comperand);
void     SInterlockedIncrementNonAtomic(LONGLONG *valuePtr);
void     SInterlockedDecrementNonAtomic(LONGLONG *valuePtr);
void     SInterlockedAddNonAtomic(LONGLONG *valuePtr, long delta);
void     SInterlockedSubNonAtomic(LONGLONG *valuePtr, long delta);
void     SInterlockedAddNonAtomic(LONGLONG *valuePtr, const LONGLONG &delta);
void     SInterlockedSubNonAtomic(LONGLONG *valuePtr, const LONGLONG &delta);

// --------------------------------
// Synchronization functions
// --------------------------------

class SCritSect {
 private:
  BYTE m_opaqueData[0x18];
  SCritSect &operator=(const SCritSect &);
  SCritSect(const SCritSect &);

 public:
  SCritSect();
  ~SCritSect();
  void Enter();
  void Leave();
  BOOL TryEnter();
};

class CDebugSCritSect : private SCritSect {
 private:
  BYTE m_debugData[0x0C];

  CDebugSCritSect &operator=(const CDebugSCritSect &);
  CDebugSCritSect(const CDebugSCritSect &);
  CDebugSCritSect();

 public:
  ~CDebugSCritSect();
  void        Enter(LPCSTR fileName, DWORD line);
  void        Leave(LPCSTR fileName, DWORD line);
  BOOL        TryEnter(LPCSTR fileName, DWORD line);
  static void DumpAllEntries();
};

class CSRWLock {
 private:
  BYTE m_opaqueData[0x0C];
  CSRWLock &operator=(const CSRWLock &);
  CSRWLock(const CSRWLock &);

 public:
  CSRWLock();
  ~CSRWLock();
  void Enter(int forwriting);
  void Leave(int fromwriting);
  BOOL TryEnter(int forwriting);
};

class CDebugSRWLock : private CSRWLock {
 private:
  BYTE m_debugData[0x0C];

  CDebugSRWLock &operator=(const CDebugSRWLock &);
  CDebugSRWLock(const CDebugSRWLock &);
  CDebugSRWLock();

 public:
  ~CDebugSRWLock();
  void        Enter(int forwriting, LPCSTR fileName, DWORD line);
  void        Leave(int fromwriting, LPCSTR fileName, DWORD line);
  BOOL        TryEnter(int forwriting, LPCSTR fileName, DWORD line);
  static void DumpAllEntries();
};

#include <SThread.h>

// --------------------------------
// Unicode functions
// --------------------------------

extern "C" int APIENTRY   SUniConvertUTF16to8Len(const WORD *src, DWORD srcMaxChars, DWORD *srcChars);
extern "C" int APIENTRY   SUniConvertUTF16to8(char *dst, DWORD dstMaxChars, const WORD *src, DWORD srcMaxChars, DWORD *dstChars, DWORD *srcChars);
extern "C" int APIENTRY   SUniConvertUTF8to16Len(LPCSTR src, DWORD srcMaxChars, DWORD *srcChars);
extern "C" int APIENTRY   SUniConvertUTF8to16(WORD *dst, DWORD dstMaxChars, LPCSTR src, DWORD srcMaxChars, DWORD *dstChars, DWORD *srcChars);
extern "C" UINT APIENTRY  SUniSGetUTF8(const BYTE *strptr, int *chars);
extern "C" char *APIENTRY SUniSPutUTF8(DWORD c, char *strptr);
extern "C" int APIENTRY   SUniFindUTF8ChrStart(LPCSTR utf8String, int index);
extern "C" int APIENTRY   SUniFindAfterUTF8Chr(LPCSTR utf8String, int index);
extern "C" DWORD APIENTRY SUniConvertUTF16ToWin(char *dest, const WORD *source, DWORD destsize);
extern "C" DWORD APIENTRY SUniConvertUTF16ToMac(char *dest, const WORD *source, DWORD destsize);
extern "C" DWORD APIENTRY SUniConvertUTF16ToDos(char *dest, const WORD *source, DWORD destsize);
extern "C" DWORD APIENTRY SUniConvertWinToUTF16(WORD *dest, LPCSTR source, DWORD destsize);
extern "C" DWORD APIENTRY SUniConvertMacToUTF16(WORD *dest, LPCSTR source, DWORD destsize);
extern "C" DWORD APIENTRY SUniConvertDosToUTF16(WORD *dest, LPCSTR source, DWORD destsize);

// --------------------------------
// System functions
// --------------------------------

typedef struct OSFILETIME {
  ULONGLONG m_value;
} OSFILETIME, *LPOSFILETIME;

typedef struct OSSYSTEMTIME {
  WORD year;
  WORD month;
  WORD dayOfWeek;
  WORD day;
  WORD hour;
  WORD minute;
  WORD second;
  WORD milliseconds;
} OSSYSTEMTIME, *LPOSSYSTEMTIME;

void  OsGetSystemTime(OSSYSTEMTIME *sysTime);
void  OsGetLocalTime(OSSYSTEMTIME *sysTime);
int   OsSystemTimeCompare(const OSSYSTEMTIME *sysTime1, const OSSYSTEMTIME *sysTime2);
void  OsTimeToFileTime(DWORD time, OSFILETIME *fileTime);
void  OsFileTimeToLocalFileTime(const OSFILETIME *fileTime, OSFILETIME *localFileTime);
void  OsFileTimeToSystemTime(const OSFILETIME *fileTime, OSSYSTEMTIME *sysTime);
void  OsSystemTimeToFileTime(const OSSYSTEMTIME *sysTime, OSFILETIME *fileTime);
void  OsTimeToLocalSystemTime(DWORD time, OSSYSTEMTIME *localSysTime);
BOOL  OsDirectoryExists(LPCSTR dirName);
DWORD OsGetFileAttributes(LPCSTR fileName);
void  OsSystemObjectCreate(LPCSTR name);
BOOL  OsSystemObjectExists(LPCSTR name);

// --------------------------------
// Directory functions
// --------------------------------

typedef struct SDIRENT {
  char d_name[MAX_PATH];
} SDIRENT;

typedef struct SDIR {
  char             name[MAX_PATH];
  HANDLE           handle;
  WIN32_FIND_DATAA findData;
  SDIRENT          dirent;
} SDIR;

DECLARE_STRICT_HANDLE(HSFILE);
DECLARE_STRICT_HANDLE(HSARCHIVE);

#define SFILE_AUTH_UNABLETOAUTHENTICATE 0
#define SFILE_AUTH_NOSIGNATURE          1
#define SFILE_AUTH_BADSIGNATURE         2
#define SFILE_AUTH_UNKNOWNSIGNATURE     3
#define SFILE_AUTH_FIRSTAUTHENTIC       5
#define SFILE_AUTH_AUTHENTICBLIZZARD    5

#define SFILE_DDA_LOOP 0x00040000

#define SFILE_ERRORMODE_RETURNCODE 0
#define SFILE_ERRORMODE_CUSTOM     1
#define SFILE_ERRORMODE_FATAL      2

#define SREG_FLAG_USERSPECIFIC 0x00000001
#define SREG_FLAG_BATTLENET    0x00000002
#define SREG_FLAG_FLUSHTODISK  0x00000008
#define SREG_FLAG_MULTISZ      0x00000080

struct _TASYNCPARAMBLOCK {
  LPVOID pvCallback;
};
struct IDirectSound;
struct IDirectSoundBuffer;
struct ZipFileFCB;
struct z_stream_s;
class ASYNCREAD;

enum SFILE_TYPE {
  SFILE_PLAIN = 0,
  SFILE_COMPRESSED = 1,
  SFILE_PAQ = 2,
  SFILE_OLD_SFILE = 3,
  SFILE_ZIP_FILE = 4
};

enum SARCHIVE_TYPE {
  SARCHIVE_MPQ = 0,
  SARCHIVE_ZIP = 1
};

class SArchive {
  friend class SFile;

 public:
  SARCHIVE_TYPE m_type;
  LPVOID        m_archive;
};

typedef struct SOVERLAPPED {
  UINT    Offset;
  SEvent *hEvent;
} SOVERLAPPED;

#include <SFile2.h>

class SFile {
  SFile(SFILE_TYPE type);
  SFile(const SFile &);
  ~SFile();
  SFile &operator=(const SFile &);

  SFILE_TYPE  m_type;
  LPVOID      m_fileptr;
  SArchive   *m_archive;
  char       *m_filename;
  char       *m_actualname;
  UINT        m_size;
  BYTE       *m_zbuffer;
  z_stream_s *m_zstream;
  UINT        m_curOffset;
  SCritSect   m_lock;
  LPVOID      m_hsfile;
  ZipFileFCB *m_zipFile;
  MD5         m_md5;
  int         m_haveMD5;
  int         m_closeAfterLoad;
  UINT        m_asyncCount;

  static void          DoAsyncRead(ASYNCREAD *ptr);
  static UINT APIENTRY ReadProc(LPVOID);
  static void          InitializeReadThread();
  static void          QueueReadRequest(SFile *fileptr, LPVOID buffer, DWORD bytestoread, SOVERLAPPED *overlapped);
  static int           DoZRead(SFile *fileptr, LPVOID buffer, DWORD bytestoread, DWORD *bytesread);

 public:
  SFILE_TYPE GetDiskType();
  DWORD      GetFileSize();
  int        GetMD5(MD5 &sum);

  static DWORD APIENTRY Open(LPCSTR filename, SFile **file);
  static DWORD APIENTRY OpenEx(SArchive *archive, LPCSTR filename, DWORD flags, SFile **file);
  static DWORD APIENTRY
  Read(SFile *fileptr, LPVOID buffer, DWORD bytestoread, DWORD *bytesread, SOVERLAPPED *overlapped, _TASYNCPARAMBLOCK *asyncparam);
  static DWORD APIENTRY
                      Load(SArchive *archive, LPCSTR filename, LPVOID *buffer, DWORD *bytes, DWORD extraBytes, DWORD flags, SOVERLAPPED *overlapped);
  static DWORD APIENTRY LoadFile(LPCSTR filename, LPVOID *buffer, DWORD *bytes, DWORD extraBytes, SOVERLAPPED *overlapped);
  static int APIENTRY      Unload(LPVOID buffer);
  static DWORD APIENTRY    Close(SFile *file);
  static int APIENTRY      GetActualFileName(SFile *file, char *buffer, DWORD bufferchars);
  static int APIENTRY      GetBasePath(char *buffer, DWORD bufferchars);
  static int APIENTRY      SetBasePath(LPCSTR path);
  static int APIENTRY      SetDataPath(LPCSTR path);
  static int APIENTRY      SetDataPathAlternate(LPCSTR path);
  static int APIENTRY      FileExists(LPCSTR filename);
  static DWORD APIENTRY    GetFileSize(SFile *file, DWORD *filesizehigh);
  static DWORD APIENTRY    SetFilePointer(SFile *file, LONG distancetomove, LONG *distancetomovehigh, DWORD movemethod);
  static int APIENTRY      EnableDirectAccess(DWORD access);
  static void APIENTRY     CreateOverlapped(SOVERLAPPED *overlapped);
  static void APIENTRY     DestroyOverlapped(SOVERLAPPED *overlapped);
  static void APIENTRY     ResetOverlapped(SOVERLAPPED *overlapped);
  static void APIENTRY     WaitOverlapped(SOVERLAPPED *overlapped);
  static int APIENTRY      PollOverlapped(SOVERLAPPED *overlapped);
  static int APIENTRY      OpenArchive(LPCSTR archivename, int priority, DWORD flags, SArchive **handle);
  static int APIENTRY      CloseArchive(SArchive *archive);
  static int APIENTRY      List(SArchive *archive, int (*cb)(LPCSTR filename, LPVOID param), LPVOID param);
  static SDIR *APIENTRY    OpenDir(LPCSTR path);
  static void APIENTRY     CloseDir(SDIR *dir);
  static SDIRENT *APIENTRY ReadDir(SDIR *dir);
  static void APIENTRY     EnableHash(bool enable);
  static void APIENTRY     RebuildHash();
  static void APIENTRY     DisableSFileCheckDisk();
  static void APIENTRY     DisableSFileCritSection();
  static int APIENTRY      GetMD5(SFile *file, MD5 &sum);
  static void              Destroy();
};

extern "C" DWORD APIENTRY SFileOpenFile(LPCSTR filename, HSFILE *handle);
extern "C" BOOL APIENTRY  SFileOpenFileAsArchive(HSARCHIVE ownerarchive, LPCSTR filename, int priority, DWORD flags, HSARCHIVE *handle);
extern "C" DWORD APIENTRY SFileOpenFileEx(HSARCHIVE archivehandle, LPCSTR filename, DWORD flags, HSFILE *handle);
extern "C" DWORD APIENTRY SFileFileExists(LPCSTR filename);
extern "C" DWORD APIENTRY SFileFileExistsEx(HSARCHIVE archivehandle, LPCSTR filename, DWORD flags);
extern "C" BOOL APIENTRY  SFileCloseFile(HSFILE handle);
extern "C" BOOL APIENTRY  SFileReadFile(HSFILE handle, LPVOID buffer, DWORD bytestoread, DWORD *bytesread, OVERLAPPED *overlapped);
extern "C" BOOL APIENTRY
SFileReadFileEx(HSFILE handle, LPVOID buffer, DWORD bytestoread, DWORD *bytesread, OVERLAPPED *overlapped, _TASYNCPARAMBLOCK *asyncparam);
extern "C" BOOL APIENTRY SFileReadFileEx2(
    HSFILE             handle,
    LPVOID             buffer,
    DWORD              bytestoread,
    DWORD             *bytesread,
    OVERLAPPED        *overlapped,
    LONG               overlappedpriority,
    _TASYNCPARAMBLOCK *asyncparam
);
extern "C" DWORD APIENTRY SFileGetFileSize(HSFILE handle, DWORD *filesizehigh);
extern "C" DWORD APIENTRY SFileGetFileCompressedSize(HSFILE handle, DWORD *fileSizeHigh);
extern "C" DWORD APIENTRY SFileSetFilePointer(HSFILE handle, LONG distancetomove, LONG *distancetomovehigh, DWORD movemethod);
extern "C" BOOL APIENTRY  SFileLoadFile(LPCSTR filename, LPVOID *buffer, DWORD *bytes, DWORD extraBytes, OVERLAPPED *overlapped);
extern "C" BOOL APIENTRY
SFileLoadFileEx(HSARCHIVE archive, LPCSTR filename, LPVOID *buffer, DWORD *bytes, DWORD extraBytes, DWORD flags, OVERLAPPED *overlapped);
extern "C" BOOL APIENTRY SFileLoadFileEx2(
    HSARCHIVE   archive,
    LPCSTR      filename,
    LPVOID     *buffer,
    DWORD      *bytes,
    DWORD       extraBytes,
    DWORD       flags,
    OVERLAPPED *overlapped,
    LONG        overlappedpriority
);
extern "C" BOOL APIENTRY  SFileUnloadFile(LPVOID buffer);
extern "C" BOOL APIENTRY  SFileOpenArchive(LPCSTR archivename, int priority, DWORD flags, HSARCHIVE *handle);
extern "C" BOOL APIENTRY  SFileCloseArchive(HSARCHIVE handle);
extern "C" BOOL APIENTRY  SFileGetArchiveName(HSARCHIVE archive, char *buffer, DWORD bufferchars);
extern "C" BOOL APIENTRY  SFileGetArchiveInfo(HSARCHIVE archive, int *priority, int *cdrom);
extern "C" BOOL APIENTRY  SFileGetFileArchive(HSFILE file, HSARCHIVE *archive);
extern "C" DWORD APIENTRY SFileCalcFileCrc(LPCSTR filename);
extern "C" DWORD APIENTRY SFileGetFileCrc(HSFILE handle);
extern "C" BOOL APIENTRY  SFileGetFileMD5(HSFILE handle, BYTE *md5);
extern "C" BOOL APIENTRY  SFileGetFileTime(HSFILE handle, FILETIME *filetime);
extern "C" BOOL APIENTRY  SFileGetActualFileName(HSFILE file, char *buffer, DWORD bufferchars);
extern "C" BOOL APIENTRY  SFileSetBasePath(LPCSTR path);
extern "C" BOOL APIENTRY  SFileGetBasePath(char *buffer, DWORD bufferchars);
extern "C" BOOL APIENTRY  SFileGetFileName(HSFILE file, char *buffer, DWORD bufferchars);
extern "C" void APIENTRY  SFileSetAsyncBudget(DWORD bytesPerSec);
extern "C" void APIENTRY  SFileSetDataChunkSize(DWORD bytes);
extern "C" BOOL APIENTRY  SFileSetIoErrorMode(DWORD errormode, int(APIENTRY *errorproc)(LPCSTR, DWORD, DWORD));
extern "C" void APIENTRY  SFileSetLocale(LCID lcid);
extern "C" LANGID APIENTRY SFileGetLocale();
extern "C" void APIENTRY  SFileSetPlatform(DWORD platformId);
extern "C" void APIENTRY  SFileCancelRequest(LPVOID buffer);
extern "C" BOOL APIENTRY  SFileCancelRequestEx(LPVOID buffer);
extern "C" void APIENTRY  SFilePrioritizeRequest(LPVOID buffer, LONG overlappedpriority);
extern "C" void APIENTRY  SFileRegisterLoadNotifyProc(void(APIENTRY *f)(LPCSTR, LPVOID), LPVOID opaqueData);
extern "C" BOOL APIENTRY  SFileEnableArchive(HSARCHIVE archive, int enable);
extern "C" BOOL APIENTRY  SFileEnableDirectAccess(DWORD access);
extern "C" void APIENTRY  SFileEnableSeekOptimization(int enable);
extern "C" BOOL APIENTRY  SFileAuthenticateArchive(HSARCHIVE handle, DWORD *extendedresult);
extern "C" BOOL APIENTRY
SFileAuthenticateArchiveEx(HSARCHIVE handle, DWORD *extendedresult, const BYTE *modulus, DWORD modulusSize, const BYTE *exponent, DWORD exponentSize);
extern "C" BOOL APIENTRY SFileDdaInitialize(IDirectSound *directsound);
extern "C" BOOL APIENTRY SFileDdaDestroy();
extern "C" BOOL APIENTRY SFileDestroy();
extern "C" BOOL APIENTRY SFileDdaBegin(HSFILE handle, DWORD buffersize, DWORD flags);
extern "C" BOOL APIENTRY
SFileDdaBeginEx(HSFILE handle, DWORD buffersize, DWORD flags, DWORD offset, LONG volume, LONG pan, IDirectSoundBuffer *soundbuffer);
extern "C" BOOL APIENTRY SFileDdaEnd(HSFILE handle);
extern "C" BOOL APIENTRY SFileDdaGetPos(HSFILE handle, DWORD *position, DWORD *maxposition);
extern "C" BOOL APIENTRY SFileDdaGetVolume(HSFILE handle, LONG *volume, LONG *pan);
extern "C" BOOL APIENTRY SFileDdaSetVolume(HSFILE handle, LONG volume, LONG pan);
extern "C" void APIENTRY SFileLoadDump();
extern "C" void APIENTRY SFileArchiveDump();

// --------------------------------
// Log functions
// --------------------------------

DECLARE_STRICT_HANDLE(HSLOG);

extern "C" void APIENTRY SLogClose(HSLOG log);
extern "C" BOOL APIENTRY SLogCreate(LPCSTR filename, DWORD flags, HSLOG *log);
extern "C" void APIENTRY SLogDestroy();
extern "C" void APIENTRY SLogDump(HSLOG log, LPCVOID data, DWORD bytes);
extern "C" void APIENTRY SLogFlush(HSLOG log);
extern "C" void APIENTRY SLogFlushAll();
extern "C" void APIENTRY SLogGetDefaultDirectory(char *dirname, DWORD dirnamesize);
extern "C" void APIENTRY SLogInitialize();
extern "C" BOOL APIENTRY SLogIsInitialized();
extern "C" void __cdecl  SLogPend(HSLOG log, LPCSTR format, ...);
extern "C" long APIENTRY SLogSetAbsIndent(HSLOG log, long indent);
extern "C" void APIENTRY SLogSetDefaultDirectory(LPCSTR dirname);
extern "C" long APIENTRY SLogSetIndent(HSLOG log, long deltaIndent);
extern "C" void APIENTRY SLogSetTimestamp(HSLOG log, BOOL timeStamp);
extern "C" void APIENTRY SLogVWrite(HSLOG log, LPCSTR format, va_list arglist);
extern "C" void __cdecl  SLogWrite(HSLOG log, LPCSTR format, ...);

// --------------------------------
// Region functions
// --------------------------------

DECLARE_STRICT_HANDLE(HSRGN);

typedef struct RECTF {
  float left;
  float bottom;
  float right;
  float top;
} RECTF, *LPRECTF;

extern "C" void APIENTRY SRgnClear(HSRGN handle);
extern "C" void APIENTRY SRgnCombineRectf(HSRGN handle, const RECTF *rect, LPVOID param, int combinemode);
extern "C" void APIENTRY SRgnCombineRecti(HSRGN handle, const RECT *rect, LPVOID param, int combinemode);
extern "C" void APIENTRY SRgnCreate(HSRGN *handle, DWORD reserved);
extern "C" void APIENTRY SRgnDelete(HSRGN handle);
extern "C" void APIENTRY SRgnDestroy();
extern "C" void APIENTRY SRgnDuplicate(HSRGN orighandle, HSRGN *handle, DWORD reserved);
extern "C" void APIENTRY SRgnGetBoundingRectf(HSRGN handle, RECTF *rect);
extern "C" void APIENTRY SRgnGetBoundingRecti(HSRGN handle, RECT *rect);
extern "C" void APIENTRY SRgnGetRectParamsf(HSRGN handle, const RECTF *rect, DWORD *numparams, LPVOID *buffer);
extern "C" void APIENTRY SRgnGetRectParamsi(HSRGN handle, const RECT *rect, DWORD *numparams, LPVOID *buffer);
extern "C" void APIENTRY SRgnGetRectsf(HSRGN handle, DWORD *numrects, RECTF *buffer);
extern "C" void APIENTRY SRgnGetRectsi(HSRGN handle, DWORD *numrects, RECT *buffer);
extern "C" BOOL APIENTRY SRgnIsPointInRegionf(HSRGN handle, float x, float y);
extern "C" BOOL APIENTRY SRgnIsPointInRegioni(HSRGN handle, int x, int y);
extern "C" BOOL APIENTRY SRgnIsRectInRegionf(HSRGN handle, const RECTF *rect);
extern "C" BOOL APIENTRY SRgnIsRectInRegioni(HSRGN handle, const RECT *rect);
extern "C" void APIENTRY SRgnOffsetf(HSRGN handle, float xoffset, float yoffset);
extern "C" void APIENTRY SRgnOffseti(HSRGN handle, int xoffset, int yoffset);

class CSRgn {
 private:
  HSRGN m_handle;

  void CopyConstructor(const CSRgn &copy);

 public:
  CSRgn();
  CSRgn(const CSRgn &copy);
  ~CSRgn();
  CSRgn &operator=(const CSRgn &copy);
  void   AddParamf(const RECTF *rect, LPVOID param);
  void   AddParami(const RECT *rect, LPVOID param);
  void   AddRectf(const RECTF *rect, LPVOID param);
  void   AddRecti(const RECT *rect, LPVOID param);
  void   Clear();
  void   CombineRectf(const RECTF *rect, LPVOID param, int combinemode);
  void   CombineRecti(const RECT *rect, LPVOID param, int combinemode);
  void   GetBoundingRectf(RECTF *rect);
  void   GetBoundingRecti(RECT *rect);
  void   GetRectsf(DWORD *numrects, RECTF *buffer);
  void   GetRectsi(DWORD *numrects, RECT *buffer);
  void   GetRectParamsf(const RECTF *rect, DWORD *numparams, LPVOID *buffer);
  void   GetRectParamsi(const RECT *rect, DWORD *numparams, LPVOID *buffer);
  BOOL   IsPointInRegionf(float x, float y);
  BOOL   IsPointInRegioni(int x, int y);
  BOOL   IsRectInRegionf(const RECTF *rect);
  BOOL   IsRectInRegioni(const RECT *rect);
  void   Offsetf(float xoffset, float yoffset);
  void   Offseti(int xoffset, int yoffset);
};

// --------------------------------
// Storm core functions
// --------------------------------

typedef struct _STORMOPTIONS {
  int   smemleaksilentwarning;
  int   serrleaksilentwarning;
  DWORD wavechunksize;
  int   alignstreamingwavedata;
  int   echotooutputdebugstring;
  int   serrsuppresslogs;
  int   crcenabled;
  int   orderedprintfenabled;
} STORMOPTIONS, *LPSTORMOPTIONS;

extern STORMOPTIONS g_opt;

extern "C" void APIENTRY      StormInitialize();
extern "C" void APIENTRY      StormDestroy();
extern "C" HINSTANCE APIENTRY StormGetInstance();
extern "C" BOOL APIENTRY      StormGetOption(int optname, LPVOID optval, LPDWORD optlen);
extern "C" BOOL APIENTRY      StormSetOption(int optname, LPVOID optval, DWORD optlen);
extern "C" int __cdecl        StormCallService(int service, ...);
void                          StormRtlInitialize();
void                          StormRtlDestroy();
void                          IncrementAllocCount();
void                          IncrementFreeCount();

namespace STypeCache {
  void   Shutdown();
  void   Grow();
  int    GetProbe(LPCSTR rawname);
  LPCSTR Get(LPCSTR rawname);
  LPCSTR Set(LPCSTR rawname, LPCSTR decname);
}

// --------------------------------
// Registry functions
// --------------------------------

extern "C" BOOL APIENTRY SRegDeleteValue(LPCSTR keyname, LPCSTR valuename, UINT flags);
extern "C" BOOL APIENTRY SRegDeleteKey(LPCSTR keyname, UINT flags);
extern "C" BOOL APIENTRY SRegGetBaseKey(UINT flags, char *buffer, UINT bufferchars);
extern "C" BOOL APIENTRY SRegLoadData(LPCSTR keyname, LPCSTR valuename, UINT flags, LPVOID buffer, DWORD buffersize, LPDWORD bytesread);
extern "C" BOOL APIENTRY SRegLoadString(LPCSTR keyname, LPCSTR valuename, UINT flags, char *buffer, DWORD buffersize);
extern "C" BOOL APIENTRY SRegLoadValue(LPCSTR keyname, LPCSTR valuename, UINT flags, LPDWORD value);
extern "C" BOOL APIENTRY SRegSaveData(LPCSTR keyname, LPCSTR valuename, UINT flags, LPCVOID data, DWORD databytes);
extern "C" BOOL APIENTRY SRegSaveString(LPCSTR keyname, LPCSTR valuename, UINT flags, LPCSTR string);
extern "C" BOOL APIENTRY SRegSaveValue(LPCSTR keyname, LPCSTR valuename, UINT flags, DWORD value);
extern "C" BOOL APIENTRY SRegEnumKey(LPCSTR baseKeyName, UINT flags, UINT subKeyIndex, char *keyNameBuffer, UINT bufferChars);
extern "C" BOOL APIENTRY SRegGetNumSubKeys(LPCSTR keyName, UINT flags, UINT *numSubKeys);

// --------------------------------
// String functions
// --------------------------------

extern "C" void APIENTRY SStrInitialize();
extern "C" void APIENTRY SStrDestroy();

LPCSTR            SStrChr(LPCSTR string, char ch);
char             *SStrChr(char *string, char ch);
LPCSTR            SStrChrR(LPCSTR string, char ch);
char             *SStrChrR(char *string, char ch);
int APIENTRY      SStrCmp(LPCSTR string1, LPCSTR string2, DWORD maxchars);
int APIENTRY      SStrCmpI(LPCSTR string1, LPCSTR string2, DWORD maxchars);
DWORD APIENTRY    SStrCopy(char *dest, LPCSTR source, DWORD destsize);
char *APIENTRY    SStrDupA(LPCSTR string, LPCSTR fileName, UINT lineNumber);
DWORD APIENTRY    SStrLen(LPCSTR string);
DWORD APIENTRY    SStrLen(const WORD *string);
DWORD APIENTRY    SStrPack(char *dest, LPCSTR source, DWORD destsize);
DWORD __cdecl     SStrPrintf(char *dest, DWORD maxchars, LPCSTR format, ...);
DWORD __cdecl     SStrVPrintf(char *dest, DWORD maxchars, LPCSTR format, va_list arglist);
double APIENTRY   SStrToDouble(LPCSTR string);
float APIENTRY    SStrToFloat(LPCSTR string);
int APIENTRY      SStrToInt(LPCSTR string);
LONGLONG APIENTRY SStrToInt64(LPCSTR string);
UINT APIENTRY     SStrToUnsigned(LPCSTR string);
void APIENTRY     SStrTokenize(LPCSTR *string, char *buffer, DWORD bufferchars, LPCSTR whitespace, int *quoted);
DWORD APIENTRY    SStrHash(LPCSTR string, DWORD flags, DWORD seed);
LONGLONG APIENTRY SStrHash64(LPCSTR string, DWORD flags, LONGLONG seed);
DWORD APIENTRY    SStrHashHT(LPCSTR string);
void APIENTRY     SStrUpper(char *string);
void APIENTRY     SStrLower(char *string);
LPCSTR            SStrStr(LPCSTR string, LPCSTR search);
char             *SStrStr(char *string, LPCSTR search);
LPCSTR            SStrStrI(LPCSTR string, LPCSTR search);
char             *SStrStrI(char *string, LPCSTR search);

#define ALLOC(bytes)     SMemAlloc(bytes, __FILE__, __LINE__, 0)
#define ALLOCZERO(bytes) SMemAlloc(bytes, __FILE__, __LINE__, SMEM_FLAG_ZEROMEMORY)
#define DEL(ptr)         delete (ptr)
#define DELIFUSED(ptr) \
  if (ptr)             \
    delete ptr
#define FREE(ptr) SMemFree(ptr, __FILE__, __LINE__, 0)
#define FREEIFUSED(ptr) \
  if (ptr)              \
    SMemFree(ptr, __FILE__, __LINE__, 0)
#define NEW(struct)     new (SMemAlloc(sizeof(struct), __FILE__, __LINE__, 0)) struct
#define NEWZERO(struct) (new (SMemAlloc(sizeof(struct), __FILE__, __LINE__, SMEM_FLAG_ZEROMEMORY)) struct)
