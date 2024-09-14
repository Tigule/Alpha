// include this header to provide win32 typedefs and macros on toolchains other than Visual C++
#pragma once

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#include <climits>

#define CALLBACK
#define WINAPI
#define CDECL
#define APIENTRY

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

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

// todo: x64 builds may need a different typedef
typedef long                                   *LPLONG;
typedef unsigned long   DWORD,      *PDWORD,   *LPDWORD;

typedef char            *LPSTR;
typedef const char      *LPCSTR;
// clang-format on

#define MAX_PATH PATH_MAX
