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

typedef void(*SYSMSGCALLBACK)(const char *msg, SYSMSG_TYPE severity);

int SysMsgAdd(const char *msg, SYSMSG_TYPE severity, unsigned int categoryMask);
int SysMsgAdd(const CStatus &status, unsigned int categoryMask);
int __cdecl    SysMsgVPrintf(SYSMSG_TYPE severity, unsigned int categoryMask, const char *format, char *arglist);
int __cdecl    SysMsgPrintf(SYSMSG_TYPE severity, unsigned int categoryMask, const char *format, ...);

inline int __cdecl SysMsgPrintf(SYSMSG_TYPE severity, const char *format, ...) {
  va_list arglist;

  va_start(arglist, format);
  return SysMsgVPrintf(severity, 1, format, arglist);
}

void SysMsgEnable(int enable);
int SysMsgEnabled();
void SysMsgSetMinDisplayLevel(SYSMSG_TYPE minSeverity);
void SysMsgSetMaxDisplayLevel(SYSMSG_TYPE maxSeverity);
SYSMSG_TYPE SysMsgGetMinDisplayLevel();
SYSMSG_TYPE SysMsgGetMaxDisplayLevel();
void SysMsgSetFilter(unsigned int categoryFilter);
unsigned int SysMsgGetFilter();
void SysMsgGetSeverityColor(SYSMSG_TYPE severity, float &r, float &g, float &b);
float SysMsgGetSeverityDuration(SYSMSG_TYPE severity);

void SysMsgInitialize();
void SysMsgShutdown();
void SysMsgEnableFileLog(const char *baseDir);
void SysMsgDisableFileLog();
void SysMsgSetCallback(SYSMSGCALLBACK callback);
