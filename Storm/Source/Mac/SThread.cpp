#include <storm.h>

#include <pthread.h>

DWORD SGetCurrentThreadId() {
  return (DWORD)(uintptr_t)pthread_self();
}

struct THREADSTART {
  STHREADPROC proc;
  LPVOID      param;
};

static LPVOID ThreadProc(LPVOID param) {
  THREADSTART start = *static_cast<THREADSTART *>(param);

  SMemFree(param, __FILE__, __LINE__, 0);
  start.proc(start.param);

  return 0;
}

LPVOID SCreateThread(DWORD stackSize, STHREADPROC proc, LPVOID param, DWORD flags, UINT *threadId, char *name) {
  pthread_attr_t attributes;
  pthread_t      thread;
  THREADSTART   *start;

  start = static_cast<THREADSTART *>(SMemAlloc(sizeof(THREADSTART), __FILE__, __LINE__, 0));
  start->proc = proc;
  start->param = param;

  pthread_attr_init(&attributes);

  if (stackSize) {
    pthread_attr_setstacksize(&attributes, stackSize);
  }

  if (pthread_create(&thread, &attributes, ThreadProc, start)) {
    SMemFree(start, __FILE__, __LINE__, 0);
    return 0;
  }

  if (threadId) {
    *threadId = static_cast<UINT>(reinterpret_cast<uintptr_t>(thread));
  }

  return thread;
}
