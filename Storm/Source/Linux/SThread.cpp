#include <storm.h>
#include "../Mac/ISThread.h"

#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

BOOL SErrGetProcessStack(pid_t pid, char *buffer, int bufferSize);
void __cdecl SOutputDebugString(LPCSTR format, ...);

typedef void(APIENTRY *SPROCESSCOMPLETIONPROC)(LPVOID);

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

  LPVOID s_SLaunchThread(LPVOID lpThreadParameter);
}

DWORD SGetCurrentThreadId() {
  return (DWORD)(DWORD_PTR)pthread_self();
}

DWORD SGetCurrentProcessId() {
  return getpid();
}

LPVOID S_Thread::s_SLaunchThread(LPVOID lpThreadParameter) {
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
  SOutputDebugString("Found thread %d in slot %d, pid=%d\n", threadId, slot, getpid());
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
  return NULL;
}

LPVOID SCreateThread(DWORD stackSize, STHREADPROC proc, LPVOID param, DWORD flags, UINT *threadId, LPVOID sync, LPCSTR name) {
  using namespace S_Thread;

  SThreadParmBlock *params;
  pthread_attr_t    attributes;
  pthread_t         thread;
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

  pthread_attr_init(&attributes);
  pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);

  if (pthread_create(&thread, &attributes, s_SLaunchThread, params)) {
    perror("pthread_create");
  }

  ASSERT(s_numthreads < s_maxthreads);

  s_threads[s_numthreads].threadId = id;
  s_threads[s_numthreads].live = 1;
  s_threads[s_numthreads].suspended = 0;
  if (name) {
    SStrCopy(s_threads[s_numthreads].name, name, sizeof(s_threads[s_numthreads].name));
  }
  ++s_numthreads;

  s_threadCrit.Leave();

  return (LPVOID)(DWORD_PTR)id;
}

LPVOID SCreateThread(STHREADPROC lpStartAddress, LPVOID lpParameter, UINT *lpThreadId, LPVOID linuxData, char *threadName) {
  return SCreateThread(0, lpStartAddress, lpParameter, 0, lpThreadId, linuxData, threadName);
}

static void ParseArguments(LPCSTR commandLine, char **argv, int *argc) {
  char argument[0x400];
  int  length;

  while (*commandLine && isspace(*commandLine)) {
    ++commandLine;
  }

  while (*commandLine && *argc < 0x40) {
    length = 0;
    argument[0] = 0;

    while (*commandLine && !isspace(*commandLine) && length < 0x3FF) {
      argument[length++] = *commandLine++;
    }

    argument[length] = 0;

    if (length > 0) {
      argv[*argc] = (char *)malloc(strlen(argument) + 1);
      strcpy(argv[*argc], argument);
      ++*argc;
    }

    while (*commandLine && isspace(*commandLine)) {
      ++commandLine;
    }
  }
}

BOOL SCreateProcess(LPCSTR applicationName, char *commandLine, SPROCESSCOMPLETIONPROC completionProc, LPVOID completionParam) {
  char  application[0x1000];
  char *argv[0x80];
  int   argc;
  int   index;

  if (fork()) {
    sleep(1);
    return TRUE;
  }

  setsid();

  argc = 0;
  SStrCopy(application, applicationName, sizeof(application));
  argv[argc++] = application;
  ParseArguments(commandLine, argv, &argc);
  argv[argc] = 0;

  printf("Executing: ");
  for (index = 0; index < argc; ++index) {
    printf("%s ", argv[index]);
  }
  putc('\n', stdout);

  execvp(argv[0], argv);
  puts("Failed!");
  exit(0);
}

void SThreadGetAllStacks(char *buffer, int bufferSize) {
  using namespace S_Thread;

  char stack[0x2000];
  int  index;

  buffer[0] = 0;

  s_threadCrit.Enter();

  for (index = 0; index < s_maxthreads; ++index) {
    int written;

    if (!s_threads[index].live || !s_threads[index].pid) {
      continue;
    }

    stack[0] = 0;
    if (SErrGetProcessStack(s_threads[index].pid, stack, sizeof(stack))) {
      written = SStrPrintf(buffer, bufferSize, "Process %d\n\n%s\n======================================\n", s_threads[index].pid, stack);
    } else {
      written = SStrPrintf(buffer, bufferSize, "Process %d returned no stack\n", s_threads[index].pid);
    }

    buffer += written;
    bufferSize -= written;
  }

  s_threadCrit.Leave();
}
