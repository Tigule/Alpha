#include <Base/Base.h>
#include <stpl.h>
#include <W32/ISThread.h>

#include <windows.h>

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

struct ContextTurn : public TSLinkedNode<ContextTurn> {
  unsigned long m_turnId;
  unsigned long m_callBufferHead;
};

struct ContextData;

struct ThreadData : public TSLinkedNode<ThreadData> {
  unsigned long m_threadId;
  void         *m_threadHandle;
  int           m_enabled;
  ContextData  *m_contextData;
  ThreadStack   m_funcStack[0x200];
  unsigned long m_funcStackIndex;
  char          m_title[0x80];
};

struct ContextData : public TSLinkedNode<ContextData> {
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
  TSList<ThreadData, TSGetLink<ThreadData> >   s_threadDataList;
  TSList<ContextData, TSGetLink<ContextData> > s_contextDataList;
  unsigned long                                s_tlsIndex;
  unsigned long                                s_initCount;
  int                                          s_enable = 1;

}  // namespace

void __fastcall OsCallInitialize(const char *threadName) {
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

void __fastcall OsCallDestroy() {
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

void __fastcall OsCallGlobalEnable(int enable) {
  s_enable = enable;
}

void __fastcall OsCallEnable(int enable) {
  if (s_initCount) {
    ThreadData *threadData = static_cast<ThreadData *>(OsTlsGetValue(s_tlsIndex));
    if (threadData) {
      threadData->m_enabled = enable;
    }
  }
}

void *__fastcall OsCallInitializeContext(const char *contextName) {
  ContextData *contextData = NEWZERO(ContextData);

  SStrCopy(contextData->m_title, contextName, sizeof(contextData->m_title));
  contextData->m_checksum = static_cast<unsigned long>(-1);

  s_critsect.Enter();

  s_contextDataList.LinkNode(contextData, LIST_TAIL, 0);

  s_critsect.Leave();
  return contextData;
}

void __fastcall OsCallDestroyContext(void *contextDataPtr) {
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

void __fastcall OsCallSetContext(void *contextDataPtr) {
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

void __fastcall OsCallResetContext(void *contextDataPtr) {
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

void __fastcall OsCallBeginTurn() {
    // TODO: implement
}

unsigned long __fastcall OsCallEndTurn() {
    // TODO: implement
    return 0;
}

void __fastcall OsCallCompleteTurn() {
    // TODO: implement
}

static void OsCallDumpContextData(_iobuf* file, const ContextData* contextData) {
    // TODO: implement
}

static void OsCallDumpProfileData(const char* fileName, const ContextData* contextData) {
    // TODO: implement
}

void __fastcall OsCallDump(const char* fileName) {
    // TODO: implement
}

int __cdecl OsCallEnter(unsigned long funcAddr, unsigned long retAddr) {
    // TODO: implement
    return 0;
}

unsigned long __cdecl OsCallExit() {
    // TODO: implement
    return 0;
}

void __fastcall OsCallData(unsigned long data) {
    // TODO: implement
}

void __fastcall OsCallData(float data) {
    // TODO: implement
}
