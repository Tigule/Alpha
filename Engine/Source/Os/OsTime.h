#ifndef ENGINE_SOURCE_OS_OSTIME_H
#define ENGINE_SOURCE_OS_OSTIME_H

struct OSFILETIME;
struct OSSYSTEMTIME;

__int64 __cdecl          OsGetAsyncTimeClocks();
__int64 OsGetAsyncClocksPerSecond();
float OsGetAsyncClocksDivisor();
unsigned long OsGetAsyncTimeMs();
unsigned long OsGetAsyncTimeMsPrecise();
float OsGetAsyncTimeSec();
void OsGetTimeStr(char* timebuf, unsigned long len);
void OsGetTimeStr(char* timebuf, unsigned long len, const char* format, long timer);
long OsGetTime(long* timer);
unsigned long OsGetTime();
void OsFileTimeGetCurrent(OSFILETIME* filetime);
int OsFileTimeCompare(const OSFILETIME* filetime1, const OSFILETIME* filetime2);
void OsFileTimeAdd(OSFILETIME* filetime, unsigned int seconds);
unsigned __int64 OsGetAsyncThreadTimeMs();
void OsGetSystemTime(OSSYSTEMTIME* sysTime);
void OsGetLocalTime(OSSYSTEMTIME* sysTime);
int OsSystemTimeCompare(const OSSYSTEMTIME* sysTime1, const OSSYSTEMTIME* sysTime2);
void OsTimeToFileTime(unsigned long time, OSFILETIME* fileTime);
void OsFileTimeToLocalFileTime(const OSFILETIME* fileTime, OSFILETIME* localFileTime);
void OsFileTimeToSystemTime(const OSFILETIME* fileTime, OSSYSTEMTIME* sysTime);
void OsSystemTimeToFileTime(const OSSYSTEMTIME* sysTime, OSFILETIME* fileTime);
void OsTimeToLocalSystemTime(unsigned long time, OSSYSTEMTIME* localSysTime);
void OsSleep(unsigned long ms);
void OsGetTimeStamp(char *timeStamp, unsigned long len);
void OsTimeStartup();
void OsTimeShutdown();

#endif
