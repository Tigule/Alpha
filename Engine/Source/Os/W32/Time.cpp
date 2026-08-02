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

class OsTimeManager {
  public:
    OsTimeManager();
    ~OsTimeManager();

    void Shutdown();

  private:
    struct TimeSnapshot {
      __int64       rdtsc;
      unsigned long tickCount;

      LARGE_INTEGER qperfCount;
    };

    void Snapshot(TimeSnapshot* time);
    static unsigned int __stdcall TimeKeeper(void*);
    void Calibrate();

  public:
    __int64 cpuTicksPerSecond_qp;
    __int64 cpuTicksPerSecond_ti;

  private:
    SThread       timeMgrThread;
    SEvent        shutdownEvt;
    unsigned long sleepVal;
    int           hasQPF;
};

static OsTimeManager* s_OsTimeMgr;

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

__int64 OsGetAsyncClocksPerSecond() {
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

float OsGetAsyncTimeSec() {
  return (float)GetTickCount() * 0.001f;
}

void OsGetTimeStr(char* timebuf, unsigned long len) {
  time_t ltime;
  time(&ltime);
  SStrCopy(timebuf, ctime(&ltime), len);
  *SStrChrR(timebuf, '\n') = 0;
}

void OsGetTimeStamp(char *timeStamp, unsigned long len) {
  time_t ltime;

  time(&ltime);
  strftime(timeStamp, len, "%m%d%y_%H%M%S", localtime(&ltime));
}

void OsGetTimeStr(char* timebuf, unsigned long len, const char* format, long timer) {
  strftime(timebuf, len, format, localtime(&timer));
}

long OsGetTime(long* timer) {
  return time(timer);
}

void OsFileTimeGetCurrent(OSFILETIME* filetime) {
  FATALASSERT(filetime);
  SYSTEMTIME systime;
  GetSystemTime(&systime);
  SystemTimeToFileTime(&systime, reinterpret_cast<FILETIME *>(&filetime->m_value));
}

int OsFileTimeCompare(const OSFILETIME* filetime1, const OSFILETIME* filetime2) {
  FATALASSERT(filetime1);
  FATALASSERT(filetime2);
  return CompareFileTime(
      reinterpret_cast<const FILETIME *>(&filetime1->m_value),
      reinterpret_cast<const FILETIME *>(&filetime2->m_value)
  );
}

void OsFileTimeAdd(OSFILETIME* filetime, unsigned int seconds) {
  FATALASSERT(filetime);
  filetime->m_value += 10000000ui64 * seconds;
}

unsigned __int64 OsGetAsyncThreadTimeMs() {
  return GetTickCount();
}

unsigned long OsGetTime() {
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
  unsigned __int64 value;

  FATALASSERT(fileTime);

  value = ((unsigned __int64)time + 11644473600ui64) * 10000000ui64;
  fileTime->m_value = value;
}

void OsFileTimeToLocalFileTime(const OSFILETIME *fileTime, OSFILETIME *localFileTime) {
  FATALASSERT(fileTime);

  FATALASSERT(localFileTime);

  FileTimeToLocalFileTime((const FILETIME *)&fileTime->m_value, (FILETIME *)&localFileTime->m_value);
}

void OsFileTimeToSystemTime(const OSFILETIME *fileTime, OSSYSTEMTIME *sysTime) {
  FATALASSERT(fileTime);

  FATALASSERT(sysTime);

  FileTimeToSystemTime((const FILETIME *)&fileTime->m_value, (SYSTEMTIME *)sysTime);
}

void OsSystemTimeToFileTime(const OSSYSTEMTIME *sysTime, OSFILETIME *fileTime) {
  FATALASSERT(fileTime);

  FATALASSERT(sysTime);

  SystemTimeToFileTime((const SYSTEMTIME *)sysTime, (FILETIME *)&fileTime->m_value);
}

void OsTimeToLocalSystemTime(DWORD time, OSSYSTEMTIME *localSysTime) {
  OSFILETIME fileTime;
  OSFILETIME localFileTime;

  FATALASSERT(localSysTime);

  OsTimeToFileTime(time, &fileTime);
  OsFileTimeToLocalFileTime(&fileTime, &localFileTime);
  OsFileTimeToSystemTime(&localFileTime, localSysTime);
}

OsTimeManager::OsTimeManager()
  : shutdownEvt(1, 0) {
  s_OsTimeMgr = this;
  sleepVal = 50;
  shutdownEvt.Reset();
  SThread::Create(TimeKeeper, 0, timeMgrThread, const_cast<char*>("OsTime"));
}

OsTimeManager::~OsTimeManager() {
}

void OsTimeManager::Shutdown() {
  shutdownEvt.Set();
}

void OsTimeManager::Snapshot(TimeSnapshot* time) {
  HANDLE thread = GetCurrentThread();
  int priority = GetThreadPriority(thread);

  if (priority != THREAD_PRIORITY_ERROR_RETURN) {
    SetThreadPriority(GetCurrentThread(), priority);
  }

  time->rdtsc = OsGetAsyncTimeClocks();
  time->tickCount = GetTickCount();

  if (hasQPF) {
    QueryPerformanceCounter(&time->qperfCount);
  }
}

unsigned int __stdcall OsTimeManager::TimeKeeper(void*) {
  s_OsTimeMgr->Calibrate();

  OsTimeManager* timeMgr;
  do {
    timeMgr = reinterpret_cast<OsTimeManager*>(
        SInterlockedExchange(reinterpret_cast<long*>(&s_OsTimeMgr), 0)
    );
  } while (!timeMgr);

  delete timeMgr;
  SRegSaveData("Internal", "CpuTicksPerSecond", 0, &s_cpuTicksPerSecond, sizeof(s_cpuTicksPerSecond));
  return 0;
}

void OsTimeManager::Calibrate() {
  LARGE_INTEGER qPerfFreq;
  TimeSnapshot baseTime;
  TimeSnapshot interval;
  SSyncObject* waitObjectPtrs[1];

  if (QueryPerformanceFrequency(&qPerfFreq) && qPerfFreq.QuadPart) {
    hasQPF = 1;
  }

  Snapshot(&baseTime);
  waitObjectPtrs[0] = &shutdownEvt;

  while (WaitMultiplePtr(1, waitObjectPtrs, TRUE, sleepVal)) {
    Snapshot(&interval);

    if (interval.tickCount - baseTime.tickCount >= 30000) {
      break;
    }

    __int64 deltaCpu = interval.rdtsc - baseTime.rdtsc;

    if (hasQPF) {
      if (interval.qperfCount.QuadPart - baseTime.qperfCount.QuadPart) {
        cpuTicksPerSecond_qp =
            static_cast<__int64>(
                (double)qPerfFreq.QuadPart /
                (double)(interval.qperfCount.QuadPart - baseTime.qperfCount.QuadPart) *
                (double)deltaCpu
            );
      }
    }

    if (interval.tickCount - baseTime.tickCount) {
      cpuTicksPerSecond_ti =
          1000 * deltaCpu / (interval.tickCount - baseTime.tickCount);
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
  new OsTimeManager;

  DWORD len = sizeof(s_cpuTicksPerSecond);
  if (!SRegLoadData("Internal", "CpuTicksPerSecond", 0, &s_cpuTicksPerSecond, sizeof(s_cpuTicksPerSecond), &len)) {
    s_cpuTicksPerSecond = 0;
  }
  Sleep(0);
}

void OsTimeShutdown() {
  OsTimeManager* timeMgr = reinterpret_cast<OsTimeManager*>(
      SInterlockedExchange(reinterpret_cast<long*>(&s_OsTimeMgr), 0)
  );

  if (timeMgr) {
    timeMgr->Shutdown();
    SInterlockedExchange(
        reinterpret_cast<long*>(&s_OsTimeMgr),
        reinterpret_cast<long>(timeMgr)
    );
  }
}
