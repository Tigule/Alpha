#include "Os/OsTime.h"

#include <windows.h>

#include <storm.h>
#include <time.h>

#if defined(_MSC_VER) && _MSC_VER >= 1400
#include <intrin.h>
#endif

static int     s_qpcExists;
static int     s_qpcExistsTested;
static float   s_qpcScaleToMs;
static __int64 s_cpuTicksPerSecond;
static float   s_cpuTicksDivisor;
static __int64 s_lastTimeAndTickCount;

#if defined(_MSC_VER) && _MSC_VER < 1400 && defined(_M_IX86)
__declspec(naked) __int64 __cdecl OsGetAsyncTimeClocks() {
  __asm {
    rdtsc
    ret
  }
}
#else
__int64 __cdecl OsGetAsyncTimeClocks() {
#if defined(_MSC_VER) && _MSC_VER >= 1400 && (defined(_M_IX86) || defined(_M_X64))
  return static_cast<__int64>(__rdtsc());
#else
  LARGE_INTEGER clocks;
  QueryPerformanceCounter(&clocks);
  return clocks.QuadPart;
#endif
}
#endif

__int64 __fastcall OsGetAsyncClocksPerSecond() {
  LARGE_INTEGER qwTickStart;
  LARGE_INTEGER qwTickEnd;
  LARGE_INTEGER liPerfFreq;
  __int64       start;
  __int64       end;
  int           priority;
  HANDLE        thread;

  if (s_cpuTicksPerSecond) {
    return s_cpuTicksPerSecond;
  }

  thread = GetCurrentThread();
  priority = GetThreadPriority(thread);

  if (priority != THREAD_PRIORITY_ERROR_RETURN) {
    SetThreadPriority(GetCurrentThread(), priority);
  }

  start = OsGetAsyncTimeClocks();
  QueryPerformanceCounter(&qwTickStart);
  Sleep(priority != THREAD_PRIORITY_ERROR_RETURN ? 50 : 500);

  thread = GetCurrentThread();
  priority = GetThreadPriority(thread);

  if (priority != THREAD_PRIORITY_ERROR_RETURN) {
    SetThreadPriority(GetCurrentThread(), priority);
  }

  QueryPerformanceCounter(&qwTickEnd);
  end = OsGetAsyncTimeClocks();
  QueryPerformanceFrequency(&liPerfFreq);

  s_cpuTicksPerSecond = ((end - start) * liPerfFreq.QuadPart) / (qwTickEnd.QuadPart - qwTickStart.QuadPart);

  return s_cpuTicksPerSecond;
}

float __fastcall OsGetAsyncClocksDivisor() {
  if (!(s_cpuTicksDivisor == 0.0f)) {
    return s_cpuTicksDivisor;
  }

  s_cpuTicksDivisor = (float)(1.0 / (double)OsGetAsyncClocksPerSecond());
  return s_cpuTicksDivisor;
}

DWORD __fastcall OsGetAsyncTimeMs() {
  return GetTickCount();
}

DWORD __fastcall OsGetAsyncTimeMsPrecise() {
  LARGE_INTEGER freq;
  LARGE_INTEGER currTime;

  if (!s_qpcExistsTested) {
    if (QueryPerformanceFrequency(&freq) && freq.QuadPart) {
      s_qpcExists = 1;
      s_qpcScaleToMs = 1000.0 / (double)freq.QuadPart;
    }

    s_qpcExistsTested = 1;
  }

  if (s_qpcExists) {
    QueryPerformanceCounter(&currTime);
    return (DWORD)((double)currTime.QuadPart * s_qpcScaleToMs);
  }

  return GetTickCount();
}

float __fastcall OsGetAsyncTimeSec() {
  return (float)GetTickCount() * 0.001f;
}

void __fastcall OsGetTimeStr(char* timebuf, unsigned long len) {
    // TODO: implement
}

void __fastcall OsGetTimeStamp(char *timeStamp, unsigned long len) {
  time_t ltime;

  time(&ltime);
  strftime(timeStamp, len, "%m%d%y_%H%M%S", localtime(&ltime));
}

void __fastcall OsGetTimeStr(char* timebuf, unsigned long len, const char* format, long timer) {
    // TODO: implement
}

void __fastcall OsFileTimeGetCurrent(OSFILETIME* filetime) {
    // TODO: implement
}

int __fastcall OsFileTimeCompare(const OSFILETIME* filetime1, const OSFILETIME* filetime2) {
    // TODO: implement
    return 0;
}

void __fastcall OsFileTimeAdd(OSFILETIME* filetime, unsigned int seconds) {
    // TODO: implement
}

unsigned __int64 __fastcall OsGetAsyncThreadTimeMs() {
    // TODO: implement
    return 0;
}

unsigned long __fastcall OsGetTime() {
  __int64              lastTimeAndTickCount;
  __int64              currTimeAndTickCount;
  __int64              observedTimeAndTickCount;
  unsigned long       *curr;
  const unsigned long *last;
  const unsigned long *observed;

  curr = reinterpret_cast<unsigned long *>(&currTimeAndTickCount);
  currTimeAndTickCount = GetTickCount();

  lastTimeAndTickCount = SInterlockedRead(&s_lastTimeAndTickCount);
  last = reinterpret_cast<const unsigned long *>(&lastTimeAndTickCount);

  if (last[1] && static_cast<long>(curr[0] - last[0]) < 500) {
    return last[1];
  }

  currTimeAndTickCount |= static_cast<__int64>(time(0)) << 32;
  observedTimeAndTickCount = SInterlockedCompareExchange(&s_lastTimeAndTickCount, currTimeAndTickCount, lastTimeAndTickCount);

  if (observedTimeAndTickCount != lastTimeAndTickCount) {
    observed = reinterpret_cast<const unsigned long *>(&observedTimeAndTickCount);
    return observed[1];
  }

  return curr[1];
}

void __fastcall OsGetSystemTime(OSSYSTEMTIME *sysTime) {
  GetSystemTime((SYSTEMTIME *)sysTime);
}

void __fastcall OsGetLocalTime(OSSYSTEMTIME *sysTime) {
  GetLocalTime((SYSTEMTIME *)sysTime);
}

int __fastcall OsSystemTimeCompare(const OSSYSTEMTIME *sysTime1, const OSSYSTEMTIME *sysTime2) {
  int diff;

  diff = (int)sysTime1->year - (int)sysTime2->year;
  if (diff) {
    return diff;
  }

  diff = (int)sysTime1->month - (int)sysTime2->month;
  if (diff) {
    return diff;
  }

  diff = (int)sysTime1->day - (int)sysTime2->day;
  if (diff) {
    return diff;
  }

  diff = (int)sysTime1->hour - (int)sysTime2->hour;
  if (diff) {
    return diff;
  }

  diff = (int)sysTime1->minute - (int)sysTime2->minute;
  if (diff) {
    return diff;
  }

  diff = (int)sysTime1->second - (int)sysTime2->second;
  if (diff) {
    return diff;
  }

  return (int)sysTime1->milliseconds - (int)sysTime2->milliseconds;
}

void __fastcall OsTimeToFileTime(DWORD time, OSFILETIME *fileTime) {
  unsigned __int64 value;

  FATALASSERT(fileTime);

  value = ((unsigned __int64)time + 11644473600ui64) * 10000000ui64;
  fileTime->m_value = value;
}

void __fastcall OsFileTimeToLocalFileTime(const OSFILETIME *fileTime, OSFILETIME *localFileTime) {
  FATALASSERT(fileTime);

  FATALASSERT(localFileTime);

  FileTimeToLocalFileTime((const FILETIME *)&fileTime->m_value, (FILETIME *)&localFileTime->m_value);
}

void __fastcall OsFileTimeToSystemTime(const OSFILETIME *fileTime, OSSYSTEMTIME *sysTime) {
  FATALASSERT(fileTime);

  FATALASSERT(sysTime);

  FileTimeToSystemTime((const FILETIME *)&fileTime->m_value, (SYSTEMTIME *)sysTime);
}

void __fastcall OsSystemTimeToFileTime(const OSSYSTEMTIME *sysTime, OSFILETIME *fileTime) {
  FATALASSERT(fileTime);

  FATALASSERT(sysTime);

  SystemTimeToFileTime((const SYSTEMTIME *)sysTime, (FILETIME *)&fileTime->m_value);
}

void __fastcall OsTimeToLocalSystemTime(DWORD time, OSSYSTEMTIME *localSysTime) {
  OSFILETIME fileTime;
  OSFILETIME localFileTime;

  FATALASSERT(localSysTime);

  OsTimeToFileTime(time, &fileTime);
  OsFileTimeToLocalFileTime(&fileTime, &localFileTime);
  OsFileTimeToSystemTime(&localFileTime, localSysTime);
}

void __fastcall OsTimeStartup() {
    // TODO: implement
}

void __fastcall OsTimeShutdown() {
    // TODO: implement
}
