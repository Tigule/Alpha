#include <Base/Base.h>
#include <stpl.h>
#include <W32/ISThread.h>

#include <windows.h>
#include <stdio.h>

struct ThreadStack {
  unsigned long m_retAddr;
  unsigned long m_funcAddr;
  int           m_logExit;
};

#pragma pack(push, 1)
struct ContextCall {
  unsigned long m_funcAddr;
  unsigned char m_depth;
};
#pragma pack(pop)

NODEDECL(ContextTurn) {
  unsigned long m_turnId;
  unsigned long m_callBufferHead;
};

struct ContextData;

NODEDECL(ThreadData) {
  unsigned long m_threadId;
  void         *m_threadHandle;
  int           m_enabled;
  ContextData  *m_contextData;
  ThreadStack   m_funcStack[0x200];
  unsigned long m_funcStackIndex;
  char          m_title[0x80];
};

NODEDECL(ContextData) {
  ThreadData   *m_threadData;
  unsigned long m_checksum;
  unsigned long m_turnId;
  unsigned long m_turnIdComplete;
  ContextTurn   m_turnBuffer[0x400];
  unsigned long m_turnBufferHead;
  unsigned long m_turnBufferTail;
  ContextCall   m_callBuffer[0x100];
  unsigned long m_callBufferHead;
  unsigned long m_callBufferTail;
  char          m_title[0x80];
};

namespace {

  CInitCritSect                                s_critsect;
  LISTDECL(ThreadData, s_threadDataList);
  LISTDECL(ContextData, s_contextDataList);
  unsigned long                                s_tlsIndex;
  unsigned long                                s_initCount;
  int                                          s_enable = 1;

}  // namespace

void OsCallInitialize(const char *threadName) {
  s_critsect.Enter();

  if (!s_initCount++) {
    s_tlsIndex = OsTlsAlloc();
  }

  ThreadData *threadData = static_cast<ThreadData *>(OsTlsGetValue(s_tlsIndex));
  if (threadData) {
    DEL(threadData);
  }

  threadData = NEWZERO(ThreadData);

  threadData->m_threadId = GetCurrentThreadId();
  DuplicateHandle(
      GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), reinterpret_cast<HANDLE *>(&threadData->m_threadHandle), 0, FALSE,
      DUPLICATE_SAME_ACCESS
  );
  SStrPrintf(threadData->m_title, sizeof(threadData->m_title), "%s [%u]", threadName, threadData->m_threadId);
  OsTlsSetValue(s_tlsIndex, threadData);

  s_threadDataList.LinkNode(threadData, LIST_TAIL, 0);

  s_critsect.Leave();
}

void OsCallDestroy() {
  s_critsect.Enter();

  if (s_initCount) {
    ThreadData *threadData = static_cast<ThreadData *>(OsTlsGetValue(s_tlsIndex));
    if (threadData) {
      OsTlsSetValue(s_tlsIndex, 0);

      if (threadData->m_contextData) {
        threadData->m_contextData->m_threadData = 0;
      }

      CloseHandle(static_cast<HANDLE>(threadData->m_threadHandle));
      DEL(threadData);
    }
  }

  if (!--s_initCount) {
    OsTlsFree(s_tlsIndex);
  }

  s_critsect.Leave();
}

void OsCallGlobalEnable(int enable) {
  s_enable = enable;
}

void OsCallEnable(int enable) {
  if (s_initCount) {
    ThreadData *threadData = static_cast<ThreadData *>(OsTlsGetValue(s_tlsIndex));
    if (threadData) {
      threadData->m_enabled = enable;
    }
  }
}

void *OsCallInitializeContext(const char *contextName) {
  ContextData *contextData = NEWZERO(ContextData);

  SStrCopy(contextData->m_title, contextName, sizeof(contextData->m_title));
  contextData->m_checksum = static_cast<unsigned long>(-1);

  s_critsect.Enter();

  s_contextDataList.LinkNode(contextData, LIST_TAIL, 0);

  s_critsect.Leave();
  return contextData;
}

void OsCallDestroyContext(void *contextDataPtr) {
  ContextData *contextData = static_cast<ContextData *>(contextDataPtr);
  if (!contextData) {
    return;
  }

  s_critsect.Enter();

  if (contextData->m_threadData) {
    contextData->m_threadData->m_contextData = 0;
  }

  DEL(contextData);

  s_critsect.Leave();
}

void OsCallSetContext(void *contextDataPtr) {
  ContextData *contextData = static_cast<ContextData *>(contextDataPtr);
  if (!s_initCount || !contextData) {
    return;
  }

  ThreadData *threadData = static_cast<ThreadData *>(OsTlsGetValue(s_tlsIndex));
  if (!threadData) {
    return;
  }

  ASSERT(contextData->m_threadData == 0);
  ASSERT(threadData->m_contextData == 0);

  s_critsect.Enter();
  contextData->m_threadData = threadData;
  threadData->m_contextData = contextData;
  s_critsect.Leave();
}

void OsCallResetContext(void *contextDataPtr) {
  ContextData *contextData = static_cast<ContextData *>(contextDataPtr);
  if (!s_initCount || !contextData) {
    return;
  }

  ThreadData *threadData = static_cast<ThreadData *>(OsTlsGetValue(s_tlsIndex));
  if (!threadData) {
    return;
  }

  if (!threadData->m_contextData && !contextData->m_threadData) {
    return;
  }

  ASSERT(threadData->m_contextData == contextData);
  ASSERT(contextData->m_threadData == threadData);

  s_critsect.Enter();
  threadData->m_contextData = 0;
  contextData->m_threadData = 0;
  s_critsect.Leave();
}

void OsCallBeginTurn() {
  if (!s_initCount) {
    return;
  }
  ThreadData *threadData = static_cast<ThreadData *>(OsTlsGetValue(s_tlsIndex));
  if (!threadData || !threadData->m_contextData) {
    return;
  }

  ContextData *contextData = threadData->m_contextData;
  unsigned long turnId = ++contextData->m_turnId;
  ContextTurn &turn = contextData->m_turnBuffer[contextData->m_turnBufferHead++ & 0x3FF];
  contextData->m_checksum = static_cast<unsigned long>(-1);
  turn.m_turnId = turnId;
  turn.m_callBufferHead = contextData->m_callBufferHead;
  if (contextData->m_turnBufferHead == contextData->m_turnBufferTail + 0x401) {
    ++contextData->m_turnBufferTail;
  }
}

unsigned long OsCallEndTurn() {
  if (!s_initCount) {
    return 0;
  }
  ThreadData *threadData = static_cast<ThreadData *>(OsTlsGetValue(s_tlsIndex));
  return threadData && threadData->m_contextData ? ~threadData->m_contextData->m_checksum : 0;
}

void OsCallCompleteTurn() {
  if (!s_initCount) {
    return;
  }
  ThreadData *threadData = static_cast<ThreadData *>(OsTlsGetValue(s_tlsIndex));
  if (!threadData || !threadData->m_contextData) {
    return;
  }

  ContextData *contextData = threadData->m_contextData;
  ++contextData->m_turnIdComplete;
  ASSERT(static_cast<long>(contextData->m_turnIdComplete - contextData->m_turnId) <= 0);

  ContextTurn &turn = contextData->m_turnBuffer[contextData->m_turnBufferTail & 0x3FF];
  if (turn.m_turnId == contextData->m_turnIdComplete) {
    ++contextData->m_turnBufferTail;
    if (static_cast<long>(turn.m_callBufferHead - contextData->m_callBufferTail) >= 0) {
      contextData->m_callBufferTail = turn.m_callBufferHead;
    }
  }
}

static void OsCallDumpContextData(_iobuf* file, const ContextData* contextData) {
  fprintf(file, "; Context: %s\r\n", contextData->m_title);

  unsigned long turnOffset = contextData->m_turnBufferTail;
  const ContextTurn *turn = 0;
  while (turnOffset != contextData->m_turnBufferHead) {
    const ContextTurn &candidate =
        contextData->m_turnBuffer[turnOffset & 0x3FF];
    if (static_cast<long>(
            candidate.m_callBufferHead - contextData->m_callBufferTail) >= 0) {
      turn = &candidate;
      break;
    }
    ++turnOffset;
  }

  for (unsigned long callOffset = contextData->m_callBufferTail;
       callOffset != contextData->m_callBufferHead;
       ++callOffset) {
    const ContextCall &call =
        contextData->m_callBuffer[callOffset & 0xFF];

    if (turn && callOffset == turn->m_callBufferHead) {
      fprintf(file, ";[TURN %05u]\r\n", turn->m_turnId);
      ++turnOffset;
      turn = turnOffset == contextData->m_turnBufferHead
          ? 0
          : &contextData->m_turnBuffer[turnOffset & 0x3FF];
    }

    if (call.m_depth & 0x80) {
      fprintf(
          file, "%*s;data = 0x%08x = %f\r\n",
          call.m_depth & 0x7F, "", call.m_funcAddr,
          *reinterpret_cast<const float *>(&call.m_funcAddr));
    } else if (call.m_funcAddr & 0x80000000) {
      fprintf(
          file, "%*s%08x\r\n",
          call.m_depth, "", call.m_funcAddr & 0x7FFFFFFF);
    }
  }
  fprintf(file, "\r\n");
}

static void OsCallDumpProfileData(const char* fileName, const ContextData* contextData) {
}

void OsCallDump(const char* fileName) {
  s_critsect.Enter();

  if (!fileName) {
    fileName = "CallDump.log";
  }

  FILE *file = fopen(fileName, "wb");
  if (file) {
    setvbuf(file, 0, _IOFBF, 0x7FFF);
    fprintf(file, ";Call Trace Log %s, %s\r\n", "Dec 11 2003", "17:58:40");

    for (ContextData *contextData = s_contextDataList.Head();
         contextData;
         contextData = s_contextDataList.Next(contextData)) {
      ThreadData *threadData = contextData->m_threadData;
      if (threadData &&
          threadData->m_threadId != GetCurrentThreadId() &&
          SuspendThread(static_cast<HANDLE>(threadData->m_threadHandle)) ==
              static_cast<DWORD>(-1)) {
        fprintf(
            file, ";Couldn't dump thread: %s\r\n", threadData->m_title);
      } else {
        OsCallDumpContextData(file, contextData);
        OsCallDumpProfileData(fileName, contextData);
        if (threadData && threadData->m_threadId != GetCurrentThreadId()) {
          ResumeThread(static_cast<HANDLE>(threadData->m_threadHandle));
        }
      }
    }
    fclose(file);
  }

  s_critsect.Leave();
}

int __cdecl OsCallEnter(unsigned long funcAddr, unsigned long retAddr) {
  if (!s_initCount) {
    return 0;
  }

  ThreadData *threadData = static_cast<ThreadData *>(OsTlsGetValue(s_tlsIndex));
  if (!threadData || threadData->m_funcStackIndex >= 0x200) {
    return 0;
  }

  unsigned long stackIndex = threadData->m_funcStackIndex++;
  ThreadStack &stack = threadData->m_funcStack[stackIndex];
  stack.m_retAddr = retAddr;
  stack.m_funcAddr = funcAddr;

  ContextData *contextData = threadData->m_contextData;
  if (contextData) {
    stack.m_logExit = s_enable && threadData->m_enabled;
    if (stack.m_logExit) {
      unsigned long head = contextData->m_callBufferHead++;
      ContextCall &call = contextData->m_callBuffer[head & 0xFF];
      call.m_depth = static_cast<unsigned char>(stackIndex & 0x7F);
      call.m_funcAddr = funcAddr | 0x80000000;
      if (contextData->m_callBufferHead == contextData->m_callBufferTail + 0x101) {
        ++contextData->m_callBufferTail;
      }
    }
  }
  return 1;
}

unsigned long __cdecl OsCallExit() {
  if (!s_initCount) {
    return 0;
  }
  ThreadData *threadData = static_cast<ThreadData *>(OsTlsGetValue(s_tlsIndex));
  if (!threadData) {
    return 0;
  }
  return threadData->m_funcStack[--threadData->m_funcStackIndex].m_retAddr;
}

void OsCallData(unsigned long data) {
  if (!s_initCount) {
    return;
  }
  ThreadData *threadData = static_cast<ThreadData *>(OsTlsGetValue(s_tlsIndex));
  if (!threadData || !threadData->m_contextData || !s_enable || !threadData->m_enabled) {
    return;
  }

  ContextData *contextData = threadData->m_contextData;
  CrcBuffer(&data, sizeof(data), &contextData->m_checksum, 2);
  unsigned long head = contextData->m_callBufferHead++;
  ContextCall &call = contextData->m_callBuffer[head & 0xFF];
  call.m_funcAddr = data;
  call.m_depth = static_cast<unsigned char>(threadData->m_funcStackIndex | 0x80);
  if (contextData->m_callBufferHead == contextData->m_callBufferTail + 0x101) {
    ++contextData->m_callBufferTail;
  }
}

void OsCallData(float data) {
  OsCallData(*reinterpret_cast<unsigned long *>(&data));
}
