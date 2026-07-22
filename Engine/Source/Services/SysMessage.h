#pragma once

#include <stdarg.h>

class CStatus;

enum SYSMSG_TYPE {
  SYSMSG_INFO = 0,
  SYSMSG_WARNING = 1,
  SYSMSG_ERROR = 2,
  SYSMSG_FATAL = 3,
  SYSMSG_NUMTYPES = 4
};

typedef void(__fastcall *SYSMSGCALLBACK)(const char *msg, SYSMSG_TYPE severity);

int __fastcall SysMsgAdd(const char *msg, SYSMSG_TYPE severity, unsigned int categoryMask);
int __fastcall SysMsgAdd(const CStatus &status, unsigned int categoryMask);
int __cdecl    SysMsgVPrintf(SYSMSG_TYPE severity, unsigned int categoryMask, const char *format, char *arglist);
int __cdecl    SysMsgPrintf(SYSMSG_TYPE severity, unsigned int categoryMask, const char *format, ...);

inline int __cdecl SysMsgPrintf(SYSMSG_TYPE severity, const char *format, ...) {
  va_list arglist;

  va_start(arglist, format);
  return SysMsgVPrintf(severity, 1, format, arglist);
}

void __fastcall         SysMsgEnable(int enable);
int __fastcall          SysMsgEnabled();
void __fastcall         SysMsgSetMinDisplayLevel(SYSMSG_TYPE minSeverity);
void __fastcall         SysMsgSetMaxDisplayLevel(SYSMSG_TYPE maxSeverity);
SYSMSG_TYPE __fastcall  SysMsgGetMinDisplayLevel();
SYSMSG_TYPE __fastcall  SysMsgGetMaxDisplayLevel();
void __fastcall         SysMsgSetFilter(unsigned int categoryFilter);
unsigned int __fastcall SysMsgGetFilter();
void __fastcall         SysMsgGetSeverityColor(SYSMSG_TYPE severity, float &r, float &g, float &b);
float __fastcall        SysMsgGetSeverityDuration(SYSMSG_TYPE severity);

void __fastcall SysMsgInitialize();
void __fastcall SysMsgShutdown();
void __fastcall SysMsgEnableFileLog(const char *baseDir);
void __fastcall SysMsgDisableFileLog();
void __fastcall SysMsgSetCallback(SYSMSGCALLBACK callback);
