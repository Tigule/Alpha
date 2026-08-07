#pragma once

#include <Base/Base.h>
#include <stdarg.h>

class CStatus;

enum SYSMSG_TYPE {
  SYSMSG_INFO = 0,
  SYSMSG_WARNING = 1,
  SYSMSG_ERROR = 2,
  SYSMSG_FATAL = 3,
  SYSMSG_NUMTYPES = 4
};

typedef void (*SYSMSGCALLBACK)(LPCSTR msg, SYSMSG_TYPE severity);

BOOL        SysMsgAdd(LPCSTR msg, SYSMSG_TYPE severity, UINT categoryMask);
BOOL        SysMsgAdd(const CStatus &status, UINT categoryMask);
int __cdecl SysMsgVPrintf(SYSMSG_TYPE severity, UINT categoryMask, LPCSTR format, char *arglist);
int __cdecl SysMsgPrintf(SYSMSG_TYPE severity, UINT categoryMask, LPCSTR format, ...);

inline int __cdecl SysMsgPrintf(SYSMSG_TYPE severity, LPCSTR format, ...) {
  va_list arglist;

  va_start(arglist, format);
  return SysMsgVPrintf(severity, 1, format, arglist);
}

void        SysMsgEnable(int enable);
int         SysMsgEnabled();
void        SysMsgSetMinDisplayLevel(SYSMSG_TYPE minSeverity);
void        SysMsgSetMaxDisplayLevel(SYSMSG_TYPE maxSeverity);
SYSMSG_TYPE SysMsgGetMinDisplayLevel();
SYSMSG_TYPE SysMsgGetMaxDisplayLevel();
void        SysMsgSetFilter(UINT categoryFilter);
UINT        SysMsgGetFilter();
void        SysMsgGetSeverityColor(SYSMSG_TYPE severity, float &r, float &g, float &b);
float       SysMsgGetSeverityDuration(SYSMSG_TYPE severity);

void SysMsgInitialize();
void SysMsgShutdown();
void SysMsgEnableFileLog(LPCSTR baseDir);
void SysMsgDisableFileLog();
void SysMsgSetCallback(SYSMSGCALLBACK callback);
