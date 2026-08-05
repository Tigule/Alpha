#include <Base/Base.h>

#include "Os/OsTime.h"

#include <storm.h>

#include <time.h>
#include <unistd.h>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

void FastMilliseconds(DWORD *hi, DWORD *lo);
void FastMicroseconds(DWORD *hi, DWORD *lo);

#define OS_FILETIME_EPOCH_SECONDS 11644473600ULL
#define OS_FILETIME_PER_SECOND    10000000ULL

static SCritSect        s_timeCritsect;
static DWORD            s_cachedTime;
static DWORD            s_cachedTimeStamp;
static int              s_timeZoneValid;
static __int64          s_timeZoneSeconds;
static float            s_cpuTicksDivisor;
static unsigned __int64 s_cpuTicksPerSecond;

__int64 __cdecl OsGetAsyncTimeClocks() {
  DWORD hi;
  DWORD lo;

  FastMicroseconds(&hi, &lo);
  return (static_cast<__int64>(hi) << 32) | lo;
}

__int64 OsGetAsyncClocksPerSecond() {
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

DWORD OsGetAsyncTimeMs() {
  DWORD hi;
  DWORD ms;

  FastMilliseconds(&hi, &ms);
  return ms;
}

DWORD OsGetAsyncTimeMsPrecise() {
  DWORD hi;
  DWORD ms;

  FastMilliseconds(&hi, &ms);
  return ms;
}

float OsGetAsyncTimeSec() {
  return (float)OsGetAsyncTimeMs() * 0.001f;
}

unsigned __int64 OsGetAsyncThreadTimeMs() {
  return OsGetAsyncTimeMs();
}

void OsSleep(DWORD ms) {
  usleep(1000 * ms);
}

DWORD OsGetTime() {
  DWORD hi;
  DWORD ms;

  FastMilliseconds(&hi, &ms);

  if (!s_cachedTime || ms - s_cachedTimeStamp >= 500) {
    s_timeCritsect.Enter();

    if (!s_cachedTime || ms - s_cachedTimeStamp >= 500) {
      s_cachedTime = static_cast<DWORD>(time(0));
      s_cachedTimeStamp = ms;
    }

    s_timeCritsect.Leave();
  }

  return s_cachedTime;
}

LONG OsGetTime(LONG *timer) {
  time_t seconds = time(0);

  if (timer) {
    *timer = static_cast<LONG>(seconds);
  }

  return static_cast<LONG>(seconds);
}

void OsGetTimeStr(char *timebuf, DWORD len) {
  time_t ltime;

  time(&ltime);
  SStrCopy(timebuf, ctime(&ltime), len);
  *SStrChrR(timebuf, '\n') = 0;
}

void OsGetTimeStr(char *timebuf, DWORD len, const char *format, LONG timer) {
  time_t ltime = timer;

  strftime(timebuf, len, format, localtime(&ltime));
}

void OsGetTimeStamp(char *timeStamp, DWORD len) {
  time_t ltime;

  time(&ltime);
  strftime(timeStamp, len, "%m%d%y_%H%M%S", localtime(&ltime));
}

void OsTimeToFileTime(DWORD time, OSFILETIME *fileTime) {
  FATALASSERT(fileTime);

  fileTime->m_value = ((unsigned __int64)time + OS_FILETIME_EPOCH_SECONDS) * OS_FILETIME_PER_SECOND;
}

void OsFileTimeGetCurrent(OSFILETIME *filetime) {
  time_t seconds;

  FATALASSERT(filetime);

  time(&seconds);
  OsTimeToFileTime(static_cast<DWORD>(seconds), filetime);
}

int OsFileTimeCompare(const OSFILETIME *filetime1, const OSFILETIME *filetime2) {
  FATALASSERT(filetime1);
  FATALASSERT(filetime2);

  if (filetime1->m_value < filetime2->m_value) {
    return -1;
  }

  return filetime1->m_value > filetime2->m_value;
}

void OsFileTimeAdd(OSFILETIME *filetime, unsigned int seconds) {
  FATALASSERT(filetime);

  filetime->m_value += OS_FILETIME_PER_SECOND * seconds;
}

static __int64 TimeZoneSeconds() {
  if (!s_timeZoneValid) {
    CFTimeZoneRef timeZone = CFTimeZoneCopySystem();

    s_timeZoneSeconds = static_cast<__int64>(CFTimeZoneGetSecondsFromGMT(timeZone, CFAbsoluteTimeGetCurrent()));
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

  FATALASSERT(fileTime);
  FATALASSERT(sysTime);

  seconds = static_cast<time_t>(fileTime->m_value / OS_FILETIME_PER_SECOND - OS_FILETIME_EPOCH_SECONDS);
  broken = gmtime(&seconds);

  sysTime->year = broken->tm_year + 1900;
  sysTime->month = broken->tm_mon + 1;
  sysTime->dayOfWeek = broken->tm_wday;
  sysTime->day = broken->tm_mday;
  sysTime->hour = broken->tm_hour;
  sysTime->minute = broken->tm_min;
  sysTime->second = broken->tm_sec;
  sysTime->milliseconds = static_cast<WORD>(fileTime->m_value / 10000 % 1000);
}

void OsSystemTimeToFileTime(const OSSYSTEMTIME *sysTime, OSFILETIME *fileTime) {
  struct tm broken;
  time_t    seconds;

  FATALASSERT(fileTime);
  FATALASSERT(sysTime);

  memset(&broken, 0, sizeof(broken));
  broken.tm_year = sysTime->year - 1900;
  broken.tm_mon = sysTime->month - 1;
  broken.tm_mday = sysTime->day;
  broken.tm_hour = sysTime->hour;
  broken.tm_min = sysTime->minute;
  broken.tm_sec = sysTime->second;

  seconds = timegm(&broken);

  fileTime->m_value = ((unsigned __int64)seconds + OS_FILETIME_EPOCH_SECONDS) * OS_FILETIME_PER_SECOND
                    + sysTime->milliseconds * 10000;
}

void OsGetSystemTime(OSSYSTEMTIME *sysTime) {
  OSFILETIME fileTime;

  OsFileTimeGetCurrent(&fileTime);
  OsFileTimeToSystemTime(&fileTime, sysTime);
}

void OsGetLocalTime(OSSYSTEMTIME *sysTime) {
  OSFILETIME fileTime;
  OSFILETIME localFileTime;

  FATALASSERT(sysTime);

  OsFileTimeGetCurrent(&fileTime);
  OsFileTimeToLocalFileTime(&fileTime, &localFileTime);
  OsFileTimeToSystemTime(&localFileTime, sysTime);
}

void OsTimeToLocalSystemTime(DWORD time, OSSYSTEMTIME *localSysTime) {
  OSFILETIME fileTime;
  OSFILETIME localFileTime;

  OsTimeToFileTime(time, &fileTime);
  OsFileTimeToLocalFileTime(&fileTime, &localFileTime);
  OsFileTimeToSystemTime(&localFileTime, localSysTime);
}

int OsSystemTimeCompare(const OSSYSTEMTIME *sysTime1, const OSSYSTEMTIME *sysTime2) {
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

void OsTimeStartup() {
}

void OsTimeShutdown() {
}
