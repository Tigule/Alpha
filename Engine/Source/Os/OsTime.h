#ifndef ENGINE_SOURCE_OS_OSTIME_H
#define ENGINE_SOURCE_OS_OSTIME_H

struct OSFILETIME;
struct OSSYSTEMTIME;

LONGLONG __cdecl OsGetAsyncTimeClocks();
LONGLONG         OsGetAsyncClocksPerSecond();
float            OsGetAsyncClocksDivisor();
DWORD            OsGetAsyncTimeMs();
DWORD            OsGetAsyncTimeMsPrecise();
float            OsGetAsyncTimeSec();
void             OsGetTimeStr(char *timebuf, DWORD len);
void             OsGetTimeStr(char *timebuf, DWORD len, LPCSTR format, long timer);
long             OsGetTime(long *timer);
DWORD            OsGetTime();
void             OsFileTimeGetCurrent(OSFILETIME *filetime);
int              OsFileTimeCompare(const OSFILETIME *filetime1, const OSFILETIME *filetime2);
void             OsFileTimeAdd(OSFILETIME *filetime, UINT seconds);
DWORDLONG        OsGetAsyncThreadTimeMs();
void             OsGetSystemTime(OSSYSTEMTIME *sysTime);
void             OsGetLocalTime(OSSYSTEMTIME *sysTime);
int              OsSystemTimeCompare(const OSSYSTEMTIME *sysTime1, const OSSYSTEMTIME *sysTime2);
void             OsTimeToFileTime(DWORD time, OSFILETIME *fileTime);
void             OsFileTimeToLocalFileTime(const OSFILETIME *fileTime, OSFILETIME *localFileTime);
void             OsFileTimeToSystemTime(const OSFILETIME *fileTime, OSSYSTEMTIME *sysTime);
void             OsSystemTimeToFileTime(const OSSYSTEMTIME *sysTime, OSFILETIME *fileTime);
void             OsTimeToLocalSystemTime(DWORD time, OSSYSTEMTIME *localSysTime);
void             OsSleep(DWORD ms);
void             OsGetTimeStamp(char *timeStamp, DWORD len);
void             OsTimeStartup();
void             OsTimeShutdown();

#endif
