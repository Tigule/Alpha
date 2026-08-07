#include <storm.h>
#include <W32/ISThread.h>

#define STHREAD_CREATE_SUSPENDED 0x00000004
#define STHREAD_MAX_TRACKED      128

typedef struct SThreadLaunchInfo {
  STHREADPROC proc;
  LPVOID      param;
  DWORD       threadId;
  HANDLE      handle;
} SThreadLaunchInfo;

typedef void (*SPROCESSCOMPLETIONPROC)(LPVOID);

typedef struct SProcessCompletionInfo {
  SPROCESSCOMPLETIONPROC proc;
  LPVOID                 param;
  HANDLE                 process;
} SProcessCompletionInfo;

namespace S_Thread {
  struct SThreadTrack {
    int    suspended;
    int    live;
    DWORD  threadId;
    LPVOID threadH;
    char   name[16];
  };

  CInitCritSect s_threadCrit;
  SThreadTrack  s_threads[STHREAD_MAX_TRACKED];
  int           s_numthreads;

  DWORD WINAPI s_SLaunchThread(LPVOID lpThreadParameter);
}  // namespace S_Thread

static UINT APIENTRY ProcessCompletionCallbackThread(LPVOID vdata) {
  SProcessCompletionInfo *info;

  info = (SProcessCompletionInfo *)vdata;
  WaitForSingleObject(info->process, INFINITE);
  info->proc(info->param);
  delete info;

  return 0;
}

BOOL SCreateProcess(LPCSTR appName, char *commandLine, SPROCESSCOMPLETIONPROC callbackWhenProcessCompletes, LPVOID callbackData) {
  WCHAR               appNameW[MAX_PATH];
  WCHAR               commandLineW[MAX_PATH];
  STARTUPINFOW        startInfo;
  PROCESS_INFORMATION processInfo;

  ZeroMemory(&startInfo, sizeof(startInfo));
  startInfo.cb = sizeof(startInfo);
  ZeroMemory(&processInfo, sizeof(processInfo));

  SUniConvertUTF8to16((WORD *)appNameW, MAX_PATH, appName, 0x7FFFFFFF, NULL, NULL);
  SUniConvertUTF8to16((WORD *)commandLineW, MAX_PATH, commandLine, 0x7FFFFFFF, NULL, NULL);

  if (!CreateProcessW(appNameW, commandLineW, NULL, NULL, FALSE, 0, NULL, NULL, &startInfo, &processInfo)) {
    return 0;
  }

  if (callbackWhenProcessCompletes) {
    SProcessCompletionInfo *completionInfo;

    if (WaitForInputIdle(processInfo.hProcess, 10000) != 0) {
      return 0;
    }

    completionInfo = new SProcessCompletionInfo;
    completionInfo->proc = callbackWhenProcessCompletes;
    completionInfo->param = callbackData;
    completionInfo->process = processInfo.hProcess;
    SCreateThread(ProcessCompletionCallbackThread, completionInfo, reinterpret_cast<UINT *>(&callbackWhenProcessCompletes), NULL, NULL);
  }

  return 1;
}

DWORD SGetCurrentThreadId() {
  return GetCurrentThreadId();
}

int SGetCurrentThreadPriority() {
  return GetThreadPriority(GetCurrentThread());
}

void SSetCurrentThreadPriority(int priority) {
  SetThreadPriority(GetCurrentThread(), priority);
}

DWORD WINAPI S_Thread::s_SLaunchThread(LPVOID lpThreadParameter) {
  DWORD              threadVal;
  DWORD              threadId;
  SThreadLaunchInfo *launch = (SThreadLaunchInfo *)lpThreadParameter;
  STHREADPROC        proc;
  LPVOID             userParam;
  int                index;

  proc = launch->proc;
  userParam = launch->param;
  threadId = launch->threadId;
  SMemFree(launch, __FILE__, __LINE__, 0);

  threadVal = proc(userParam);
  s_threadCrit.Enter();
  for (index = 0; index < STHREAD_MAX_TRACKED; index++) {
    if (s_threads[index].live && s_threads[index].threadId == threadId) {
      memcpy(&s_threads[index], &s_threads[index + 1], (STHREAD_MAX_TRACKED - index - 1) * sizeof(s_threads[0]));
      s_numthreads--;
    }
  }
  s_threadCrit.Leave();

  return threadVal;
}

LPVOID
SCreateThread(DWORD dwStackSize, STHREADPROC lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, UINT *lpThreadId, char *threadName) {
  struct {
    DWORD  cdThreadId;
    LPVOID cdThreadH;
  } buf;
  DWORD              bufsize;
  HANDLE             hThread;
  SThreadLaunchInfo *launch;

  if (!threadName) {
    threadName = "";
  }

  S_Thread::s_threadCrit.Enter();
  if (!S_Thread::s_numthreads) {
    S_Thread::s_threads[S_Thread::s_numthreads].threadId = GetCurrentThreadId();
    S_Thread::s_threads[S_Thread::s_numthreads].threadH = NULL;
    S_Thread::s_threads[S_Thread::s_numthreads].live = 1;
    S_Thread::s_threads[S_Thread::s_numthreads].suspended = 0;
    SStrCopy(S_Thread::s_threads[S_Thread::s_numthreads].name, "main", sizeof(S_Thread::s_threads[S_Thread::s_numthreads].name));
    S_Thread::s_numthreads++;

    bufsize = sizeof(buf);
    if (StormGetOption(8, &buf, &bufsize)) {
      S_Thread::s_threads[S_Thread::s_numthreads].threadId = buf.cdThreadId;
      S_Thread::s_threads[S_Thread::s_numthreads].threadH = buf.cdThreadH;
      S_Thread::s_threads[S_Thread::s_numthreads].live = 1;
      S_Thread::s_threads[S_Thread::s_numthreads].suspended = 0;
      SStrCopy(S_Thread::s_threads[S_Thread::s_numthreads].name, "CdThreadProc", sizeof(S_Thread::s_threads[S_Thread::s_numthreads].name));
      S_Thread::s_numthreads++;
    }
  }
  S_Thread::s_threadCrit.Leave();

  launch = (SThreadLaunchInfo *)SMemAlloc(0x10, __FILE__, __LINE__, SMEM_FLAG_ZEROMEMORY);
  launch->proc = lpStartAddress;
  launch->param = lpParameter;

  hThread = CreateThread(NULL, dwStackSize, S_Thread::s_SLaunchThread, launch, dwCreationFlags | STHREAD_CREATE_SUSPENDED, (DWORD *)lpThreadId);
  launch->threadId = *lpThreadId;
  launch->handle = hThread;

  S_Thread::s_threadCrit.Enter();
  if (S_Thread::s_numthreads >= STHREAD_MAX_TRACKED) {
    SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "s_numthreads < s_maxthreads", FALSE, 1);
  }
  S_Thread::s_threads[S_Thread::s_numthreads].threadId = launch->threadId;
  S_Thread::s_threads[S_Thread::s_numthreads].threadH = launch->handle;
  S_Thread::s_threads[S_Thread::s_numthreads].live = 1;
  S_Thread::s_threads[S_Thread::s_numthreads].suspended = (dwCreationFlags >> 2) & 1;
  if (threadName) {
    SStrCopy(S_Thread::s_threads[S_Thread::s_numthreads].name, threadName, sizeof(S_Thread::s_threads[S_Thread::s_numthreads].name));
  }
  S_Thread::s_numthreads++;
  S_Thread::s_threadCrit.Leave();

  if (!(dwCreationFlags & STHREAD_CREATE_SUSPENDED)) {
    ResumeThread(hThread);
  }

  return hThread;
}

LPVOID SCreateThread(STHREADPROC lpStartAddress, LPVOID lpParameter, UINT *lpThreadId, LPVOID linuxData, char *threadName) {
  return SCreateThread(0, lpStartAddress, lpParameter, 0, lpThreadId, threadName);
}
