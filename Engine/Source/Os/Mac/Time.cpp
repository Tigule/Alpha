#include <Base/Base.h>

#include "Os/OsTime.h"

#include <storm.h>

#include <time.h>
#include <unistd.h>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

extern "C" void FastMilliseconds(DWORD *hi, DWORD *lo);
extern "C" void FastMicroseconds(DWORD *hi, DWORD *lo);

#define OS_FILETIME_EPOCH_SECONDS 11644473600ULL
#define OS_FILETIME_PER_SECOND    10000000ULL

static SInitCritSect s_timeCritsect;
static DWORD     s_cachedTime;
static DWORD     s_cachedTimeStamp;
static int       s_timeZoneValid;
static LONGLONG  s_timeZoneSeconds;
static float     s_cpuTicksDivisor;
static DWORDLONG s_cpuTicksPerSecond;

LONGLONG __cdecl OsGetAsyncTimeClocks() {
  DWORD hi;
  DWORD lo;

  FastMicroseconds(&hi, &lo);
  return ((LONGLONG)hi << 32) | lo;
}

LONGLONG OsGetAsyncClocksPerSecond() {
  if (!s_cpuTicksPerSecond) {
    s_cpuTicksPerSecond = 1000000;
  }

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
  DWORD hi;
  DWORD ms;

  FastMilliseconds(&hi, &ms);
  return ms;
}

void OsSleep(DWORD ms) {
  usleep(1000 * ms);
}

DWORD OsGetTime() {
  DWORD hi;
  DWORD ms;

  FastMilliseconds(&hi, &ms);

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
  FATALASSERT(fileTime);

  fileTime->m_value = ((DWORDLONG)time + OS_FILETIME_EPOCH_SECONDS) * OS_FILETIME_PER_SECOND;
}

void OsFileTimeGetCurrent(OSFILETIME *filetime) {
  time_t seconds;

  FATALASSERT(filetime);

  time(&seconds);
  OsTimeToFileTime((DWORD)seconds, filetime);
}

static LONGLONG TimeZoneSeconds() {
  if (!s_timeZoneValid) {
    CFTimeZoneRef timeZone = CFTimeZoneCopySystem();

    s_timeZoneSeconds = (LONGLONG)CFTimeZoneGetSecondsFromGMT(timeZone, CFAbsoluteTimeGetCurrent());
    CFRelease(timeZone);

    s_timeZoneValid = 1;
  }

  return s_timeZoneSeconds;
}

void OsFileTimeToLocalFileTime(const OSFILETIME *fileTime, OSFILETIME *localFileTime) {
  FATALASSERT(fileTime);
  FATALASSERT(localFileTime);

  localFileTime->m_value = fileTime->m_value + TimeZoneSeconds() * OS_FILETIME_PER_SECOND;
}

void OsFileTimeToSystemTime(const OSFILETIME *fileTime, OSSYSTEMTIME *sysTime) {
  time_t     seconds;
  struct tm *broken;

  VALIDATE2VOID(fileTime);
  VALIDATE2VOID(sysTime);

  seconds = (time_t)(fileTime->m_value / OS_FILETIME_PER_SECOND - OS_FILETIME_EPOCH_SECONDS);
  broken = gmtime(&seconds);

  sysTime->year = broken->tm_year + 1900;
  sysTime->month = broken->tm_mon + 1;
  sysTime->dayOfWeek = broken->tm_wday;
  sysTime->day = broken->tm_mday;
  sysTime->hour = broken->tm_hour;
  sysTime->minute = broken->tm_min;
  sysTime->second = broken->tm_sec;
  sysTime->milliseconds = (WORD)(fileTime->m_value / 10000 % 1000);
}

void OsGetSystemTime(OSSYSTEMTIME *sysTime) {
  OSFILETIME fileTime;

  OsFileTimeGetCurrent(&fileTime);
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
