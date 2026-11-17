// include this header to provide win32 typedefs and macros on toolchains other than Visual C++
#pragma once

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#include <climits>
#include <new>
#include <typeinfo>
#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <pthread.h>
#include <sys/stat.h>

#define CALLBACK
#define WINAPI
#define CDECL
#define APIENTRY
#define CONST const
#define VOID  void

#define __cdecl
#define __stdcall
#define __fastcall
#define __int64 long long
#define __assume(e) ((void)0)
#define __PLACEMENT_NEW_INLINE

#if RAND_MAX != 0x7FFF
#define rand() (rand() & 0x7FFF)
#endif

#ifdef __APPLE__
extern "C" int vc_rand();
#endif

#define __min(a, b) (((a) < (b)) ? (a) : (b))
#define __max(a, b) (((a) > (b)) ? (a) : (b))

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define ERROR_SUCCESS           0L
#define ERROR_INVALID_FUNCTION  1L
#define ERROR_FILE_NOT_FOUND    2L
#define ERROR_PATH_NOT_FOUND    3L
#define ERROR_ACCESS_DENIED     5L
#define ERROR_INVALID_HANDLE    6L
#define ERROR_NOT_ENOUGH_MEMORY 8L
#define ERROR_BAD_FORMAT        11L
#define ERROR_INVALID_DATA      13L
#define ERROR_NO_MORE_FILES     18L
#define ERROR_INVALID_DRIVE     15L
#define ERROR_SHARING_VIOLATION 32L
#define ERROR_HANDLE_EOF        38L
#define ERROR_INVALID_PARAMETER 87L
#define ERROR_OPEN_FAILED       110L
#define ERROR_BAD_PATHNAME      161L
#define ERROR_INSUFFICIENT_BUFFER 122L
#define ERROR_ALREADY_EXISTS    183L
#define ERROR_FILE_INVALID      1006L
#define ERROR_NOT_AUTHENTICATED 1244L

#define INFINITE      0xFFFFFFFF
#define WAIT_OBJECT_0 0
#define WAIT_TIMEOUT  258L

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

#define FILE_SHARE_READ  0x00000001
#define FILE_SHARE_WRITE 0x00000002

#define FILE_ATTRIBUTE_READONLY  0x00000001
#define FILE_ATTRIBUTE_HIDDEN    0x00000002
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#define FILE_ATTRIBUTE_NORMAL    0x00000080

#define FILE_FLAG_SEQUENTIAL_SCAN 0x08000000

#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)

#define CONTAINING_RECORD(address, type, field) ((type *)((char *)(address) - offsetof(type, field)))

#define LOWORD(l) ((WORD)(l))
#define HIWORD(l) ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))

// clang-format off
typedef void                                   *LPVOID;
typedef const void                             *LPCVOID;
typedef int             BOOL,       *PBOOL,    *LPBOOL;
typedef unsigned char   BYTE,       *PBYTE,    *LPBYTE;
typedef unsigned short  WORD,       *PWORD,    *LPWORD;
typedef int             INT,        *PINT,     *LPINT;
typedef unsigned int    UINT,       *PUINT;
typedef float           FLOAT,      *PFLOAT;
typedef char                        *PSZ;
typedef char            CHAR;
typedef short           SHORT;
typedef unsigned char   UCHAR;
typedef unsigned short  USHORT;
typedef wchar_t         WCHAR;
typedef char            TCHAR;

#ifdef __LP64__
typedef int             LONG,       *PLONG,    *LPLONG;
typedef unsigned int    ULONG;
typedef unsigned int    DWORD,      *PDWORD,   *LPDWORD;
typedef long long          INT_PTR;
typedef unsigned long long UINT_PTR;
typedef long long          LONG_PTR;
typedef unsigned long long ULONG_PTR;
#else
typedef long            LONG,       *PLONG,    *LPLONG;
typedef unsigned long   ULONG;
typedef unsigned long   DWORD,      *PDWORD,   *LPDWORD;
typedef int             INT_PTR;
typedef unsigned int    UINT_PTR;
typedef long            LONG_PTR;
typedef unsigned long   ULONG_PTR;
#endif

typedef ULONG_PTR       DWORD_PTR;
typedef LONG_PTR        LRESULT;
typedef LONG_PTR        LPARAM;
typedef UINT_PTR        WPARAM;

typedef long long          LONGLONG;
typedef unsigned long long ULONGLONG;
typedef ULONGLONG          DWORDLONG;

typedef char            *LPSTR;
typedef const char      *LPCSTR;
typedef WCHAR           *LPWSTR;
typedef const WCHAR     *LPCWSTR;
typedef LPSTR            LPTSTR;
typedef LPCSTR           LPCTSTR;
typedef DWORD            LCID;
typedef WORD             LANGID;
typedef DWORD            COLORREF;
typedef void            *HANDLE;
typedef HANDLE           HWND;
// clang-format on

#define DECLARE_HANDLE(name) \
  struct name##__ {          \
    int unused;              \
  };                         \
  typedef struct name##__ *name

DECLARE_HANDLE(HINSTANCE);
DECLARE_HANDLE(HDC);
DECLARE_HANDLE(HGLRC);
typedef HINSTANCE HMODULE;

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

#define MAX_PATH PATH_MAX

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

typedef struct _OVERLAPPED {
  ULONG_PTR Internal;
  ULONG_PTR InternalHigh;
  DWORD     Offset;
  DWORD     OffsetHigh;
  HANDLE    hEvent;
} OVERLAPPED;
typedef struct _SYSTEMTIME SYSTEMTIME;
typedef struct _CONTEXT    CONTEXT;

#define _snprintf  snprintf
#define _vsnprintf vsnprintf
#define _stricmp   strcasecmp
#define _strnicmp  strncasecmp
#define _fileno    fileno
#define _stat      stat
#define _fstat     fstat
#define _isnan(x)  isnan(x)
#define _finite(x) isfinite(x)
#define _copysign  copysign

inline char *_strupr(char *str) {
  for (char *c = str; *c; ++c) {
    *c = (char)toupper((unsigned char)*c);
  }
  return str;
}

inline char *_strlwr(char *str) {
  for (char *c = str; *c; ++c) {
    *c = (char)tolower((unsigned char)*c);
  }
  return str;
}
