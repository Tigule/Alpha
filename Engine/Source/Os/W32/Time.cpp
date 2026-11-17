#include <winsock2.h>
#include <windows.h>
#include <Base/Base.h>
#include <time.h>

#include "OsGui.h"

#include "Os/OsTime.h"


#include <storm.h>

#if defined(_MSC_VER) && _MSC_VER >= 1400
#include <intrin.h>
#include <stdio.h>
#endif

static int      s_qpcExists;
static int      s_qpcExistsTested;
static float    s_qpcScaleToMs;
static LONGLONG s_cpuTicksPerSecond;
static float    s_cpuTicksDivisor;
static LONGLONG s_lastTimeAndTickCount;

class OsTimeManager {
 public:
  OsTimeManager();
  ~OsTimeManager();

  void Shutdown();

 private:
  struct TimeSnapshot {
    LONGLONG rdtsc;
    DWORD    tickCount;

    LARGE_INTEGER qperfCount;
  };

 public:
  LONGLONG cpuTicksPerSecond_qp;
  LONGLONG cpuTicksPerSecond_ti;

 private:
  static UINT WINAPI    TimeKeeper(LPVOID);
  void                  Calibrate();

  void Snapshot(TimeSnapshot *time);

  SThread timeMgrThread;
  SEvent  shutdownEvt;
  DWORD   sleepVal;
  BOOL    hasQPF;
};

static OsTimeManager *s_OsTimeMgr;

#if !BUILD_PORTABLE
__declspec(naked) LONGLONG __cdecl OsGetAsyncTimeClocks() {
  __asm {
    rdtsc
    ret
  }
}
#else
LONGLONG __cdecl OsGetAsyncTimeClocks() {
#if defined(_MSC_VER) && _MSC_VER >= 1400 && (defined(_M_IX86) || defined(_M_X64))
  return (LONGLONG)__rdtsc();
#else
  LARGE_INTEGER clocks;
  QueryPerformanceCounter(&clocks);
  return clocks.QuadPart;
#endif
}
#endif

LONGLONG OsGetAsyncClocksPerSecond() {
  LARGE_INTEGER qwTickStart;
  LARGE_INTEGER qwTickEnd;
  LARGE_INTEGER liPerfFreq;
  LONGLONG      start;
  LONGLONG      end;
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

float OsGetAsyncClocksDivisor() {
  if (!(s_cpuTicksDivisor == 0.0f)) {
    return s_cpuTicksDivisor;
  }

  s_cpuTicksDivisor = (float)(1.0 / (double)OsGetAsyncClocksPerSecond());
  return s_cpuTicksDivisor;
}

DWORD OsGetAsyncTimeMs() {
  return GetTickCount();
}

DWORD OsGetAsyncTimeMsPrecise() {
  if (!s_qpcExistsTested) {
    LARGE_INTEGER freq;
    if (QueryPerformanceFrequency(&freq) && freq.QuadPart) {
      s_qpcExists = 1;
      s_qpcScaleToMs = 1000.0f / freq.QuadPart;
    }

    s_qpcExistsTested = 1;
  }

  if (s_qpcExists) {
    LARGE_INTEGER currTime;
    QueryPerformanceCounter(&currTime);
    return (DWORD)(currTime.QuadPart * s_qpcScaleToMs);
  }

  return GetTickCount();
}

float OsGetAsyncTimeSec() {
  return (float)GetTickCount() * 0.001f;
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
  strftime(timeStamp, len, "%m%d%y_%H%M%S\0", localtime(&ltime));
}

void OsGetTimeStr(char *timebuf, DWORD len, LPCSTR format, long timer) {
#ifdef _WIN64
  strftime(timebuf, len, format, _localtime32(&timer));
#else
  strftime(timebuf, len, format, localtime(&timer));
#endif
}

long OsGetTime(long *timer) {
#ifdef _WIN64
  return _time32(timer);
#else
  return time(timer);
#endif
}

void OsFileTimeGetCurrent(OSFILETIME *filetime) {
  VALIDATE2VOID(filetime);
  SYSTEMTIME systime;
  GetSystemTime(&systime);
  SystemTimeToFileTime(&systime, (FILETIME *)&filetime->m_value);
}

int OsFileTimeCompare(const OSFILETIME *filetime1, const OSFILETIME *filetime2) {
  VALIDATE2(filetime1);
  VALIDATE2(filetime2);
  return CompareFileTime((const FILETIME *)&filetime1->m_value, (const FILETIME *)&filetime2->m_value);
}

void OsFileTimeAdd(OSFILETIME *filetime, UINT seconds) {
  VALIDATE2VOID(filetime);
  filetime->m_value += 10000000ui64 * seconds;
}

DWORDLONG OsGetAsyncThreadTimeMs() {
  return GetTickCount();
}

DWORD OsGetTime() {
  LONGLONG     lastTimeAndTickCount;
  LONGLONG     currTimeAndTickCount;
  LONGLONG     observedTimeAndTickCount;
  DWORD       *curr;
  const DWORD *last;
  const DWORD *observed;

  curr = (DWORD *)&currTimeAndTickCount;
  currTimeAndTickCount = GetTickCount();

  lastTimeAndTickCount = SInterlockedRead(&s_lastTimeAndTickCount);
  last = (const DWORD *)&lastTimeAndTickCount;

  if (last[1] && (long)(curr[0] - last[0]) < 500) {
    return last[1];
  }

  currTimeAndTickCount |= (LONGLONG)time(0) << 32;
  observedTimeAndTickCount = SInterlockedCompareExchange(&s_lastTimeAndTickCount, currTimeAndTickCount, lastTimeAndTickCount);

  if (observedTimeAndTickCount != lastTimeAndTickCount) {
    observed = (const DWORD *)&observedTimeAndTickCount;
    return observed[1];
  }

  return curr[1];
}

void OsGetSystemTime(OSSYSTEMTIME *sysTime) {
  GetSystemTime((SYSTEMTIME *)sysTime);
}

void OsGetLocalTime(OSSYSTEMTIME *sysTime) {
  GetLocalTime((SYSTEMTIME *)sysTime);
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

void OsTimeToFileTime(DWORD time, OSFILETIME *fileTime) {
  DWORDLONG value;

  VALIDATE2VOID(fileTime);

  value = ((DWORDLONG)time + 11644473600ui64) * 10000000ui64;
  fileTime->m_value = value;
}

void OsFileTimeToLocalFileTime(const OSFILETIME *fileTime, OSFILETIME *localFileTime) {
  VALIDATE2VOID(fileTime);
  VALIDATE2VOID(localFileTime);

  FileTimeToLocalFileTime((const FILETIME *)&fileTime->m_value, (FILETIME *)&localFileTime->m_value);
}

void OsFileTimeToSystemTime(const OSFILETIME *fileTime, OSSYSTEMTIME *sysTime) {
  VALIDATE2VOID(fileTime);
  VALIDATE2VOID(sysTime);

  FileTimeToSystemTime((const FILETIME *)&fileTime->m_value, (SYSTEMTIME *)sysTime);
}

void OsSystemTimeToFileTime(const OSSYSTEMTIME *sysTime, OSFILETIME *fileTime) {
  VALIDATE2VOID(fileTime);
  VALIDATE2VOID(sysTime);

  SystemTimeToFileTime((const SYSTEMTIME *)sysTime, (FILETIME *)&fileTime->m_value);
}

void OsTimeToLocalSystemTime(DWORD time, OSSYSTEMTIME *localSysTime) {
  OSFILETIME fileTime;
  OSFILETIME localFileTime;

  VALIDATE2VOID(localSysTime);

  OsTimeToFileTime(time, &fileTime);
  OsFileTimeToLocalFileTime(&fileTime, &localFileTime);
  OsFileTimeToSystemTime(&localFileTime, localSysTime);
}

OsTimeManager::OsTimeManager() : shutdownEvt(1, 0) {
  s_OsTimeMgr = this;
  sleepVal = 50;
  shutdownEvt.Reset();
  SThread::Create(TimeKeeper, 0, timeMgrThread, (char *)"OsTime");
}

OsTimeManager::~OsTimeManager() {
}

void OsTimeManager::Shutdown() {
  shutdownEvt.Set();
}

void OsTimeManager::Snapshot(TimeSnapshot *time) {
  HANDLE thread = GetCurrentThread();
  int    priority = GetThreadPriority(thread);

  if (priority != THREAD_PRIORITY_ERROR_RETURN) {
    SetThreadPriority(GetCurrentThread(), priority);
  }

  time->rdtsc = OsGetAsyncTimeClocks();
  time->tickCount = GetTickCount();

  if (hasQPF) {
    QueryPerformanceCounter(&time->qperfCount);
  }
}

UINT WINAPI OsTimeManager::TimeKeeper(LPVOID) {
  s_OsTimeMgr->Calibrate();

  OsTimeManager *timeMgr;
  do {
#ifdef _WIN64
    timeMgr = (OsTimeManager *)SInterlockedExchangePointer((LPVOID *)&s_OsTimeMgr, 0);
#else
    timeMgr = (OsTimeManager *)SInterlockedExchange((long *)&s_OsTimeMgr, 0);
#endif
  } while (!timeMgr);

  delete timeMgr;
  SRegSaveData("Internal", "CpuTicksPerSecond", 0, &s_cpuTicksPerSecond, sizeof(s_cpuTicksPerSecond));
  return 0;
}

void OsTimeManager::Calibrate() {
  LARGE_INTEGER qPerfFreq;
  TimeSnapshot  baseTime;
  TimeSnapshot  interval;
  SSyncObject  *waitObjectPtrs[1];

  if (QueryPerformanceFrequency(&qPerfFreq) && qPerfFreq.QuadPart) {
    hasQPF = TRUE;
  }

  Snapshot(&baseTime);

  while (waitObjectPtrs[0] = &shutdownEvt, WaitMultiplePtr(1, waitObjectPtrs, TRUE, sleepVal)) {
    Snapshot(&interval);

    if (interval.tickCount - baseTime.tickCount >= 30000) {
      break;
    }

    LONGLONG deltaCpu = interval.rdtsc - baseTime.rdtsc;

    if (hasQPF) {
      double deltaQp = (double)(interval.qperfCount.QuadPart - baseTime.qperfCount.QuadPart);
      if (deltaQp) {
        cpuTicksPerSecond_qp = (LONGLONG)((double)qPerfFreq.QuadPart / deltaQp * (double)deltaCpu);
      }
    }

    if (interval.tickCount - baseTime.tickCount) {
      cpuTicksPerSecond_ti = 1000 * deltaCpu / (interval.tickCount - baseTime.tickCount);
    }

    if (hasQPF && cpuTicksPerSecond_qp) {
      SInterlockedExchange(&s_cpuTicksPerSecond, cpuTicksPerSecond_qp);
    } else if (cpuTicksPerSecond_ti) {
      SInterlockedExchange(&s_cpuTicksPerSecond, cpuTicksPerSecond_ti);
    }

    sleepVal = (3 * sleepVal) >> 1;
    if (sleepVal > 1000) {
      sleepVal = 1000;
    }
  }
}

void OsTimeStartup() {
  NEWZERO(OsTimeManager);

  DWORD len;
  if (!SRegLoadData("Internal", "CpuTicksPerSecond", 0, &s_cpuTicksPerSecond, sizeof(s_cpuTicksPerSecond), &len)) {
    memset(&s_cpuTicksPerSecond, 0, sizeof(s_cpuTicksPerSecond));
  }
  Sleep(0);
}

void OsTimeShutdown() {
#ifdef _WIN64
  OsTimeManager *timeMgr = (OsTimeManager *)SInterlockedExchangePointer((LPVOID *)&s_OsTimeMgr, 0);
#else
  OsTimeManager *timeMgr = (OsTimeManager *)SInterlockedExchange((long *)&s_OsTimeMgr, 0);
#endif

  if (timeMgr) {
    timeMgr->Shutdown();
#ifdef _WIN64
    SInterlockedExchangePointer((LPVOID *)&s_OsTimeMgr, timeMgr);
#else
    SInterlockedExchange((long *)&s_OsTimeMgr, (long)timeMgr);
#endif
  }
}
