#ifndef ENGINE_SOURCE_OS_OSTIME_H
#define ENGINE_SOURCE_OS_OSTIME_H

__int64 __cdecl          OsGetAsyncTimeClocks();
__int64 __fastcall       OsGetAsyncClocksPerSecond();
float __fastcall         OsGetAsyncClocksDivisor();
unsigned long __fastcall OsGetAsyncTimeMs();
unsigned long __fastcall OsGetAsyncTimeMsPrecise();
float __fastcall         OsGetAsyncTimeSec();
unsigned long __fastcall OsGetTime();
void __fastcall          OsSleep(unsigned long ms);
void __fastcall          OsGetTimeStamp(char *timeStamp, unsigned long len);

#endif
