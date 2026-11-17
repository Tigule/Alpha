#define BOOL OBJC_BOOL
#import <Foundation/Foundation.h>
#undef BOOL

#include <storm.h>
#include "ISThread.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

struct SThreadParmBlock {
  STHREADPROC     proc;
  LPVOID          param;
  DWORD           threadId;
  SyncObjectData *sync;
};

namespace S_Thread {
  struct SThreadTrack {
    int   suspended;
    int   live;
    DWORD threadId;
    char  name[16];
    pid_t pid;
  };

  const int s_maxthreads = 256;

  SThreadTrack  s_threads[s_maxthreads + 1];
  int           s_numthreads;
  CInitCritSect s_threadCrit;
  DWORD         s_threadID;

  DWORD s_SLaunchThread(LPVOID lpThreadParameter);
}

@interface SThreadRunner : NSObject {
  SThreadParmBlock  *m_params;
  NSAutoreleasePool *m_autoreleasePool;
}
- (id)init:(SThreadParmBlock *)params;
- (void)run:(id)arg;
@end

@implementation SThreadRunner

- (id)init:(SThreadParmBlock *)params {
  self = [super init];
  if (self) {
    m_params = params;
    m_autoreleasePool = nil;
  }
  return self;
}

- (void)run:(id)arg {
  m_autoreleasePool = [[NSAutoreleasePool alloc] init];
  S_Thread::s_SLaunchThread(m_params);
  [m_autoreleasePool release];
}

@end

DWORD SGetCurrentThreadId() {
  return (DWORD)(uintptr_t)pthread_self();
}

DWORD S_Thread::s_SLaunchThread(LPVOID lpThreadParameter) {
  SThreadParmBlock *params = (SThreadParmBlock *)lpThreadParameter;
  STHREADPROC       proc = params->proc;
  LPVOID            param = params->param;
  DWORD             threadId = params->threadId;
  int               slot;
  int               i;

  s_threadCrit.Enter();
  slot = -1;
  for (i = 0; i < s_maxthreads; ++i) {
    if (s_threads[i].live && s_threads[i].threadId == threadId) {
      s_threads[i].pid = getpid();
      slot = i;
      break;
    }
  }
  printf("Found thread %d in slot %d, pid=%d\n", threadId, slot, getpid());
  s_threadCrit.Leave();

  proc(param);

  s_threadCrit.Enter();
  for (i = 0; i < s_maxthreads; ++i) {
    if (s_threads[i].live && s_threads[i].threadId == threadId) {
      memmove(&s_threads[i], &s_threads[i + 1], (s_maxthreads - 1 - i) * sizeof(SThreadTrack));
      --s_numthreads;
    }
  }
  s_threadCrit.Leave();

  if (params->sync) {
    pthread_mutex_lock(&params->sync->mutex);
    params->sync->count = 1;
    pthread_mutex_unlock(&params->sync->mutex);
    pthread_cond_signal(&params->sync->cond);
  }

  FREE(params);
  return 0;
}

LPVOID SCreateThread(DWORD stackSize, STHREADPROC proc, LPVOID param, DWORD flags, UINT *threadId, LPVOID sync, LPCSTR name) {
  using namespace S_Thread;

  SThreadParmBlock *params;
  SThreadRunner    *runner;
  DWORD             id;

  if (!name) {
    name = "";
  }

  s_threadCrit.Enter();

  if (!s_numthreads) {
    DWORD cdThread[2];
    DWORD cdThreadSize;

    s_threads[s_numthreads].threadId = s_threadID++;
    s_threads[s_numthreads].live = 1;
    s_threads[s_numthreads].suspended = 0;
    SStrCopy(s_threads[s_numthreads].name, "main", sizeof(s_threads[s_numthreads].name));
    ++s_numthreads;

    cdThreadSize = sizeof(cdThread);
    if (StormGetOption(8, cdThread, &cdThreadSize)) {
      s_threads[s_numthreads].threadId = cdThread[0];
      s_threads[s_numthreads].live = 1;
      s_threads[s_numthreads].suspended = 0;
      SStrCopy(s_threads[s_numthreads].name, "CdThreadProc", sizeof(s_threads[s_numthreads].name));
      ++s_numthreads;
    }
  }

  id = s_threadID++;

  params = (SThreadParmBlock *)ALLOCZERO(sizeof(SThreadParmBlock));
  params->proc = proc;
  params->param = param;
  params->threadId = id;
  params->sync = (SyncObjectData *)sync;

  if (params->sync) {
    params->sync->count = 0;
    pthread_mutex_init(&params->sync->mutex, 0);
    pthread_cond_init(&params->sync->cond, 0);
  }

  runner = [[SThreadRunner alloc] init:params];
  [NSThread detachNewThreadSelector:@selector(run:) toTarget:runner withObject:nil];

  ASSERT(s_numthreads < s_maxthreads);

  s_threads[s_numthreads].threadId = id;
  s_threads[s_numthreads].live = 1;
  s_threads[s_numthreads].suspended = 0;
  if (name) {
    SStrCopy(s_threads[s_numthreads].name, name, sizeof(s_threads[s_numthreads].name));
  }
  ++s_numthreads;

  s_threadCrit.Leave();

  return (LPVOID)(uintptr_t)id;
}

LPVOID SCreateThread(STHREADPROC lpStartAddress, LPVOID lpParameter, UINT *lpThreadId, LPVOID linuxData, char *threadName) {
  return SCreateThread(0, lpStartAddress, lpParameter, 0, lpThreadId, linuxData, threadName);
}
