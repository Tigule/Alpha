#include <Base/Base.h>

#include "Os/OsTime.h"

#include <storm.h>

#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <x86intrin.h>

static SInitCritSect s_timeCritsect;
static DWORD         s_cachedTime;
static DWORD         s_cachedTimeStamp;
static float         s_cpuTicksDivisor;
static LONGLONG      s_cpuTicksPerSecond;
static bool          s_clocksPerMsInit;
static LONGLONG      s_clocksPerMs;

LONGLONG __cdecl OsGetAsyncTimeClocks() {
  return (LONGLONG)__rdtsc();
}

LONGLONG OsGetAsyncClocksPerSecond() {
  DWORDLONG      start;
  DWORDLONG      end;
  struct timeval startTime;
  struct timeval endTime;
  long           elapsed;

  if (s_cpuTicksPerSecond) {
    return s_cpuTicksPerSecond;
  }

  start = __rdtsc();
  gettimeofday(&startTime, 0);
  sleep(1);
  end = __rdtsc();
  gettimeofday(&endTime, 0);

  elapsed = (endTime.tv_sec - startTime.tv_sec) * 1000000 + (endTime.tv_usec - startTime.tv_usec);
  s_cpuTicksPerSecond = (LONGLONG)((float)(end - start) / (float)elapsed * 1000000.0f);
  return s_cpuTicksPerSecond;
}

float OsGetAsyncClocksDivisor() {
  if (!(s_cpuTicksDivisor == 0.0f)) {
    return s_cpuTicksDivisor;
  }

  s_cpuTicksDivisor = (float)(1.0 / (double)OsGetAsyncClocksPerSecond());
  return s_cpuTicksDivisor;
}

DWORD OsGetAsyncTimeMsPrecise() {
  if (!s_clocksPerMsInit) {
    s_clocksPerMs = OsGetAsyncClocksPerSecond() / 1000;
    s_clocksPerMsInit = true;
  }

  return (DWORD)(OsGetAsyncTimeClocks() / s_clocksPerMs);
}

void OsSleep(DWORD ms) {
  usleep(1000 * ms);
}

DWORD OsGetTime() {
  DWORD ms = OsGetAsyncTimeMsPrecise();

  if (!s_cachedTime || (int)(ms - s_cachedTimeStamp) >= 500) {
    s_timeCritsect.Enter();

    if (!s_cachedTime || (int)(ms - s_cachedTimeStamp) >= 500) {
      s_cachedTime = (DWORD)time(0);
      s_cachedTimeStamp = ms;
    }

    s_timeCritsect.Leave();
  }

  return s_cachedTime;
}

void OsGetTimeStr(char *timebuf, DWORD len) {
  time_t ltime;

  time(&ltime);
  SStrCopy(timebuf, ctime(&ltime), len);
  *SStrChrR(timebuf, '\n') = 0;
}

void OsGetTimeStamp(char *timeStamp, DWORD len) {
  time_t ltime;

  time(&ltime);
  strftime(timeStamp, len, "%m%d%y_%H%M%S", localtime(&ltime));
}

void OsTimeToFileTime(DWORD time, OSFILETIME *fileTime) {
  VALIDATE2VOID(fileTime);

  fileTime->m_value = (DWORDLONG)time * 10000000 + 116444736000000000ULL;
}

void OsFileTimeGetCurrent(OSFILETIME *filetime) {
  time_t seconds;

  VALIDATE2VOID(filetime);

  time(&seconds);
  OsTimeToFileTime((DWORD)seconds, filetime);
}

void OsFileTimeToLocalFileTime(const OSFILETIME *fileTime, OSFILETIME *localFileTime) {
  time_t seconds;

  VALIDATE2VOID(fileTime);
  VALIDATE2VOID(localFileTime);

  time(&seconds);
  localtime(&seconds);
  localFileTime->m_value = fileTime->m_value - (LONGLONG)timezone * 10000000;
}

void OsFileTimeToSystemTime(const OSFILETIME *fileTime, OSSYSTEMTIME *sysTime) {
  time_t     seconds;
  struct tm *broken;

  VALIDATE2VOID(fileTime);
  VALIDATE2VOID(sysTime);

  seconds = (time_t)((fileTime->m_value - 116444736000000000ULL) / 10000000);
  broken = gmtime(&seconds);

  sysTime->year = broken->tm_year + 1900;
  sysTime->month = broken->tm_mon + 1;
  sysTime->dayOfWeek = broken->tm_wday;
  sysTime->day = broken->tm_mday;
  sysTime->hour = broken->tm_hour;
  sysTime->minute = broken->tm_min;
  sysTime->second = broken->tm_sec;
  sysTime->milliseconds = broken->tm_sec * 1000;
}

void OsGetSystemTime(OSSYSTEMTIME *sysTime) {
  time_t     seconds;
  OSFILETIME fileTime;

  time(&seconds);
  OsTimeToFileTime((DWORD)seconds, &fileTime);
  OsFileTimeToSystemTime(&fileTime, sysTime);
}

void OsTimeToLocalSystemTime(DWORD time, OSSYSTEMTIME *localSysTime) {
  OSFILETIME fileTime;
  OSFILETIME localFileTime;

  VALIDATE2VOID(localSysTime);

  OsTimeToFileTime(time, &fileTime);
  OsFileTimeToLocalFileTime(&fileTime, &localFileTime);
  OsFileTimeToSystemTime(&localFileTime, localSysTime);
}

void OsGetLocalTime(OSSYSTEMTIME *sysTime) {
  time_t ltime;

  time(&ltime);
  OsTimeToLocalSystemTime((DWORD)ltime, sysTime);
}
