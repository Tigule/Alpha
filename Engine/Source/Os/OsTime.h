#ifndef ENGINE_SOURCE_OS_OSTIME_H
#define ENGINE_SOURCE_OS_OSTIME_H

struct OSFILETIME;
struct OSSYSTEMTIME;

__int64 __cdecl          OsGetAsyncTimeClocks();
__int64 __fastcall       OsGetAsyncClocksPerSecond();
float __fastcall         OsGetAsyncClocksDivisor();
unsigned long __fastcall OsGetAsyncTimeMs();
unsigned long __fastcall OsGetAsyncTimeMsPrecise();
float __fastcall         OsGetAsyncTimeSec();
void __fastcall          OsGetTimeStr(char* timebuf, unsigned long len);
void __fastcall          OsGetTimeStr(char* timebuf, unsigned long len, const char* format, long timer);
long __fastcall          OsGetTime(long* timer);
unsigned long __fastcall OsGetTime();
void __fastcall          OsFileTimeGetCurrent(OSFILETIME* filetime);
int __fastcall           OsFileTimeCompare(const OSFILETIME* filetime1, const OSFILETIME* filetime2);
void __fastcall          OsFileTimeAdd(OSFILETIME* filetime, unsigned int seconds);
unsigned __int64 __fastcall OsGetAsyncThreadTimeMs();
void __fastcall          OsGetSystemTime(OSSYSTEMTIME* sysTime);
void __fastcall          OsGetLocalTime(OSSYSTEMTIME* sysTime);
int __fastcall           OsSystemTimeCompare(const OSSYSTEMTIME* sysTime1, const OSSYSTEMTIME* sysTime2);
void __fastcall          OsTimeToFileTime(unsigned long time, OSFILETIME* fileTime);
void __fastcall          OsFileTimeToLocalFileTime(const OSFILETIME* fileTime, OSFILETIME* localFileTime);
void __fastcall          OsFileTimeToSystemTime(const OSFILETIME* fileTime, OSSYSTEMTIME* sysTime);
void __fastcall          OsSystemTimeToFileTime(const OSSYSTEMTIME* sysTime, OSFILETIME* fileTime);
void __fastcall          OsTimeToLocalSystemTime(unsigned long time, OSSYSTEMTIME* localSysTime);
void __fastcall          OsSleep(unsigned long ms);
void __fastcall          OsGetTimeStamp(char *timeStamp, unsigned long len);
void __fastcall          OsTimeStartup();
void __fastcall          OsTimeShutdown();

#endif
