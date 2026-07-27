#include "EvtInt.h"

#include "Os/OsTime.h"

#include <new>
#include <string.h>
#include <stpl.h>

unsigned int OsGetProcessorCount();
void OsNetPump(DWORD timeout);

void OsCallInitialize(const char *name);
void OsCallDestroy();
void *OsCallInitializeContext(const char *name);
int              s_watchdogActive;

static unsigned int                             s_hThread;
static int                                      s_netServer;
static long                                     s_threadListContention = -1;
static SCritSect                                s_threadListCritsect;
static LISTDECL(EvtThread, s_threadList);
static SCritSect                               *s_threadSlotCritsects;
static EvtThread                              **s_threadSlots;
static unsigned int                             s_threadSlotCount;
static TSGrowableArray<SThread *>               s_schedulerThreads;
static SEvent                                   s_startEvent(1, 0);
static SEvent                                   s_shutdownEvent(1, 0);
static long                                     s_interactiveCount;
static int                                      s_originalThreadPriority;
static unsigned int                             s_mainThread;

static int SynthesizeInitialize(EvtContext *context);
static void SynthesizeDestroy(EvtContext *context);
static void SynthesizeIdle(EvtContext *context);
static void SynthesizePoll(EvtContext *context);
static void SynthesizePaint(EvtContext *context);
static unsigned int InitializeSchedulerThread();
static void DestroySchedulerThread(unsigned int hThread);
static void DetachContextFromThread(unsigned int hThread, EvtContext *context);
static EvtContext *GetNextContext(unsigned int hThread);
static SEvent *GetWakeEvent(unsigned int hThread);
static void PutContext(unsigned int hThread, EvtContext *context, DWORD nextWakeTime, DWORD newSmoothWeight);
static HEVENTCONTEXT AttachContextToThread(EvtContext *context);
static unsigned int APIENTRY    SchedulerThreadProc(void *mainThread);
static unsigned int APIENTRY    ShutdownThreadProc(void *pEvent);

static int SynthesizeInitialize(EvtContext *context) {
  if (context->SchedGetFlags(0x1)) {
    return 0;
  }
  context->SchedSetFlags(0x1);
  context->SchedSetLastIdle(context->GetCurrTime());
  IEvtQueueDispatch(context, EVENT_ID_INITIALIZE, 0);
  return 1;
}

static void SynthesizeDestroy(EvtContext *context) {
  INSTANCELOCK instanceLock;

  if (!context->SchedGetFlags(0x1)) {
    SynthesizeInitialize(context);
  }

  ASSERT(context->SchedGetClosed());
  if (context->SchedGetFlags(0x2) && !SInterlockedDecrement(&s_interactiveCount)) {
    IEvtSchedulerShutdown();
  }

  IEvtQueueDispatch(context, EVENT_ID_CLOSE, 0);
  context->SchedSetDestroyed();
  IEvtQueueDispatchAll(context);
  IEvtQueueDispatch(context, EVENT_ID_DESTROY, 0);

  context->SchedDeselect();

  EvtContext::GetTable().Lock(context->Id(), 1, instanceLock, __FILE__, __LINE__);
  DEL(context);
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
}

static void SynthesizeIdle(EvtContext *context) {
  EVENT_DATA_IDLE data;
  float           elapsedSec;
  DWORD           currTime;

  if (context->SchedGetClosed()) {
    return;
  }

  currTime = OsGetAsyncTimeMs();
  elapsedSec = static_cast<float>(static_cast<LONG>(currTime - context->SchedGetLastIdle())) * 0.001f;
  context->SchedSetLastIdle(currTime);
  if (context->SchedGetFlags(0x2)) {
    context->SchedSetFlags(0x4);
  }
  data.elapsedSec = elapsedSec;
  data.time = currTime;
  IEvtQueueDispatch(context, EVENT_ID_IDLE, &data);
}

static void SynthesizePoll(EvtContext *context) {
  if (!context->SchedGetClosed()) {
    IEvtQueueDispatch(context, EVENT_ID_POLL, 0);
  }
}

static void SynthesizePaint(EvtContext *context) {
  if (context->SchedGetClosed()) {
    return;
  }
  if (context->SchedGetFlags(0x4)) {
    context->SchedResetFlags(0x4);
    IEvtQueueDispatch(context, EVENT_ID_PAINT, 0);
  }
}

static unsigned int InitializeSchedulerThread() {
  unsigned int slot;
  unsigned int bestSlot = s_threadSlotCount;
  EvtThread   *thread;
  EvtThread   *bestThread;

  SInterlockedIncrement(&s_threadListContention);
  s_threadListCritsect.Enter();
  for (slot = 0; slot < s_threadSlotCount; ++slot) {
    if (bestSlot == s_threadSlotCount || !s_threadSlots[slot] || s_threadSlots[slot]->m_threadCount < s_threadSlots[bestSlot]->m_threadCount) {
      bestSlot = slot;
      if (!s_threadSlots[slot]) {
        break;
      }
    }
  }

  bestThread = s_threadSlots[bestSlot];
  if (!bestThread) {
    thread = s_threadList.NewNode(LIST_TAIL, 0, 0);

    thread->m_threadSlot = bestSlot;
    s_threadSlotCritsects[bestSlot].Enter();
    s_threadSlots[bestSlot] = thread;
    s_threadSlotCritsects[bestSlot].Leave();
    bestThread = thread;
  }
  ++bestThread->m_threadCount;
  s_threadListCritsect.Leave();
  SInterlockedDecrement(&s_threadListContention);
  return bestSlot;
}

static void DestroySchedulerThread(unsigned int hThread) {
  TSGrowableArray<EvtContext *> contextArray;
  EvtThread                    *thread;
  EvtContextQueue              *queue;
  EvtContext                   *context;
  unsigned int                  index;

  SInterlockedIncrement(&s_threadListContention);
  s_threadListCritsect.Enter();
  thread = s_threadSlots[hThread];
  if (thread && !--thread->m_threadCount) {
    s_threadSlotCritsects[hThread].Enter();
    s_threadSlots[hThread] = 0;
    s_threadSlotCritsects[hThread].Leave();

    queue = &thread->m_contextQueue;
    contextArray.ReserveSpace(queue->Count());
    while ((context = queue->Dequeue()) != 0) {
      contextArray.Add(1, &context);
    }
    s_threadList.DeleteNode(thread);
  }
  s_threadListCritsect.Leave();
  SInterlockedDecrement(&s_threadListContention);

  index = contextArray.Count();
  while (index) {
    --index;
    context = contextArray[index];
    context->m_critsect.Enter();
    if (context->m_schedState == EvtContext::SCHEDSTATE_ACTIVE) {
      context->m_schedState = EvtContext::SCHEDSTATE_CLOSED;
    }
    context->m_critsect.Leave();
    context->SchedSelect();
    SynthesizeDestroy(context);
  }
}

static HEVENTCONTEXT AttachContextToThread(EvtContext *context) {
  EvtThread              *thread;
  EvtThread              *candidate;
  EvtContextQueue        *queue;
  TSTimerPriority<DWORD> *priority;
  DWORD                   contextId = 0;
  DWORD                   currTime;

  SInterlockedIncrement(&s_threadListContention);
  s_threadListCritsect.Enter();
  thread = 0;
  candidate = s_threadList.Head();
  while (candidate) {
    if (!thread || candidate->m_weightTotal < thread->m_weightTotal) {
      thread = candidate;
    }
    candidate = candidate->Next();
  }

  if (thread) {
    if (!context->Id()) {
      contextId = EvtContext::GetTable().Link(context);
    } else {
      contextId = context->Id();
    }
    priority = &context->m_schedNextWakeTime;
    currTime = OsGetAsyncTimeMs();
    priority->Set(currTime);
    s_threadSlotCritsects[thread->m_threadSlot].Enter();
    queue = &thread->m_contextQueue;
    queue->Enqueue(context);
    s_threadSlotCritsects[thread->m_threadSlot].Leave();
    thread->m_wakeEvent.Set();
    thread->m_weightTotal += context->m_schedWeight;
    ++thread->m_contextCount;
    thread->m_weightAvg = thread->m_weightTotal / thread->m_contextCount;
  } else if (context) {
    DEL(context);
  }

  s_threadListCritsect.Leave();
  SInterlockedDecrement(&s_threadListContention);
  return reinterpret_cast<HEVENTCONTEXT>(contextId);
}

static void DetachContextFromThread(unsigned int hThread, EvtContext *context) {
  EvtThread   *thread;
  EvtThread   *other;
  unsigned int amount;

  SInterlockedIncrement(&s_threadListContention);
  s_threadListCritsect.Enter();
  thread = s_threadSlots[hThread];
  if (thread) {
    thread->m_weightTotal -= context->m_schedWeight;
    --thread->m_contextCount;
    thread->m_weightAvg = thread->m_contextCount ? thread->m_weightTotal / thread->m_contextCount : 0;

    other = s_threadList.Head();
    while (other) {
      if (other != thread && other->m_weightAvg && other->m_weightTotal >= other->m_weightAvg + thread->m_weightTotal) {
        amount = (other->m_weightTotal - thread->m_weightTotal) / other->m_weightAvg;
        other->m_rebalance += amount;
        if (other->m_rebalance >= other->m_contextCount) {
          other->m_rebalance = other->m_contextCount;
        }
      }
      other = other->Next();
    }
  }
  s_threadListCritsect.Leave();
  SInterlockedDecrement(&s_threadListContention);
}

static EvtContext *GetNextContext(unsigned int hThread) {
  EvtThread       *thread;
  EvtContextQueue *queue;
  EvtContext      *context = 0;

  s_threadSlotCritsects[hThread].Enter();
  thread = s_threadSlots[hThread];
  ASSERT(thread);
  queue = &thread->m_contextQueue;
  context = queue->Dequeue();
  s_threadSlotCritsects[hThread].Leave();
  return context;
}

static SEvent *GetWakeEvent(unsigned int hThread) {
  EvtThread *thread = s_threadSlots[hThread];
  ASSERT(thread);
  return &thread->m_wakeEvent;
}

static void PutContext(unsigned int hThread, EvtContext *context, DWORD nextWakeTime, DWORD newSmoothWeight) {
  TSTimerPriority<DWORD> *priority = &context->m_schedNextWakeTime;
  EvtThread              *thread;
  EvtThread              *candidate;
  EvtThread              *bestThread;
  EvtContextQueue        *queue;
  DWORD                   oldWeight;
  DWORD                   delta;
  unsigned int            threadSlot = hThread;
  DWORD                   bestWeightTotal;

  priority->Set(nextWakeTime);
  if (context->m_schedSmoothWeight != newSmoothWeight) {
    oldWeight = context->m_schedWeight;
    context->m_schedSmoothWeight = newSmoothWeight;
    delta = newSmoothWeight > oldWeight ? newSmoothWeight - oldWeight : oldWeight - newSmoothWeight;
    context->m_schedRebalance = delta >= (oldWeight >> 3);
  }

  if (!SInterlockedIncrement(&s_threadListContention)) {
    s_threadListCritsect.Enter();
    thread = s_threadSlots[threadSlot];
    ASSERT(thread);
    if (thread->m_rebalance || context->m_schedRebalance) {
      if (context->m_schedRebalance) {
        thread->m_weightTotal -= context->m_schedWeight;
        context->m_schedWeight = context->m_schedSmoothWeight;
        thread->m_weightTotal += context->m_schedWeight;
      }

      bestThread = thread;
      bestWeightTotal = thread->m_weightTotal;
      candidate = s_threadList.Head();
      while (candidate) {
        if (candidate != thread && context->m_schedWeight + candidate->m_weightTotal < bestWeightTotal) {
          bestThread = candidate;
          bestWeightTotal = context->m_schedWeight + candidate->m_weightTotal;
        }
        candidate = candidate->Next();
      }

      if (bestThread != thread) {
        thread->m_weightTotal -= context->m_schedWeight;
        --thread->m_contextCount;
        ASSERT(thread->m_contextCount);
        thread->m_weightAvg = thread->m_weightTotal / thread->m_contextCount;
        ++bestThread->m_contextCount;
        bestThread->m_weightTotal = bestWeightTotal;
        bestThread->m_weightAvg = bestWeightTotal / bestThread->m_contextCount;
        context->m_schedRebalance = 0;
        if (thread->m_rebalance) {
          --thread->m_rebalance;
        }

        s_threadSlotCritsects[bestThread->m_threadSlot].Enter();
        queue = &bestThread->m_contextQueue;
        queue->Enqueue(context);
        s_threadSlotCritsects[bestThread->m_threadSlot].Leave();
        bestThread->m_wakeEvent.Set();
        threadSlot = s_threadSlotCount;
      } else {
        if (context->m_schedRebalance) {
          context->m_schedRebalance = 0;
          thread->m_weightAvg = thread->m_weightTotal / thread->m_contextCount;
        }
        if (context->m_schedWeight <= thread->m_weightAvg) {
          thread->m_rebalance = 0;
        }
      }
    }
    s_threadListCritsect.Leave();
  }
  SInterlockedDecrement(&s_threadListContention);

  if (threadSlot < s_threadSlotCount) {
    s_threadSlotCritsects[threadSlot].Enter();
    thread = s_threadSlots[threadSlot];
    ASSERT(thread);
    queue = &thread->m_contextQueue;
    queue->Enqueue(context);
    s_threadSlotCritsects[threadSlot].Leave();
  }
}

static unsigned int APIENTRY ShutdownThreadProc(void *pEvent) {
  SEvent *shutdownEvent = static_cast<SEvent *>(pEvent);

  ASSERT(pEvent);
  while (shutdownEvent->Wait(0) != WAIT_OBJECT_0) {
    OsNetPump(100);
  }
  return 0;
}

static unsigned int APIENTRY SchedulerThreadProc(void *mainThread) {
  unsigned int hThread;
  EvtContext  *context;
  DWORD        nextDelay;
  DWORD        currTime;
  DWORD        idleTime;
  DWORD        waitResult;
  LONG         signedDelay;
  int          shutdown;
  int          watchdogActive = 0;
  int          closed;
  int          currentPriority;
  char         callName[64];

  PropSelectContext(0);
  hThread = mainThread ? s_mainThread : InitializeSchedulerThread();
  s_startEvent.Wait(INFINITE);
  SStrPrintf(callName, sizeof(callName), "Engine %x", SGetCurrentThreadId());
  OsCallInitialize(callName);

  for (;;) {
    currentPriority = SGetCurrentThreadPriority();
    if (currentPriority != s_originalThreadPriority) {
      SSetCurrentThreadPriority(s_originalThreadPriority);
    }
    if (watchdogActive) {
      SErrPingWatchdog();
    }
    if (s_shutdownEvent.Wait(0) == WAIT_OBJECT_0) {
      break;
    }

    context = GetNextContext(hThread);
    nextDelay = INFINITE;
    if (context) {
      signedDelay = static_cast<LONG>(context->SchedGetNextWakeTime() - OsGetAsyncTimeMs());
      nextDelay = signedDelay < 0 ? 0 : static_cast<DWORD>(signedDelay);
    }
    if (s_netServer) {
      if (nextDelay == INFINITE) {
        nextDelay = 100;
      }
      OsNetPump(nextDelay);
      waitResult = WAIT_TIMEOUT;
    } else {
      waitResult = GetWakeEvent(hThread)->Wait(nextDelay);
    }

    if (!context) {
      continue;
    }
    context->SchedSelect();
    currTime = OsGetAsyncTimeMs();
    context->SetCurrTime(currTime);

    if (waitResult == WAIT_TIMEOUT) {
      if (SynthesizeInitialize(context) && context->StartWatchdog()) {
        SErrStartWatchdog(20, TRUE);
        watchdogActive = 1;
      }
      IEvtTimerDispatch(context);
      shutdown = 0;
      if (context->SchedGetFlags(0x2)) {
        IEvtInputProcess(context, &shutdown);
        if (shutdown) {
          context->SchedSetClosed();
          IEvtSchedulerShutdown();
        }
      }
      SynthesizePoll(context);
      IEvtQueueDispatchAll(context);
      SynthesizeIdle(context);
      SynthesizePaint(context);
    }

    closed = context->SchedGetClosed();
    if (closed) {
      DetachContextFromThread(hThread, context);
      SynthesizeDestroy(context);
      continue;
    }

    if (context->SchedGetFlags(0x4)) {
      nextDelay = 0;
    } else {
      nextDelay = IEvtTimerGetNextTime(context, currTime);
      idleTime = context->SchedGetIdleTime();
      if (idleTime != context->SchedGetInitialIdleTime()) {
        nextDelay = idleTime;
      }
      signedDelay = static_cast<LONG>(idleTime + context->SchedGetLastIdle() - currTime);
      if (nextDelay >= static_cast<DWORD>(signedDelay < 0 ? 0 : signedDelay)) {
        nextDelay = signedDelay < 0 ? 0 : signedDelay;
      }
    }
    context->SchedDeselect();
    PutContext(hThread, context, currTime + nextDelay, context->SchedGetSmoothWeight());
  }

  if (watchdogActive) {
    SErrStopWatchdog();
  }
  DestroySchedulerThread(hThread);
  OsCallDestroy();
  return 0;
}

void IEvtSchedulerProcess() {
  s_startEvent.Set();
  SchedulerThreadProc(reinterpret_cast<void *>(1));
  s_mainThread = 0;
}

void IEvtSchedulerInitialize(unsigned int threadCount, int netServer) {
  unsigned int threadSlotCount = 1;
  unsigned int remaining;
  SThread     *thread;
  char         threadname[16];

  if (s_threadSlotCount) {
    FATALERROR(("IEvtScheduler already initialized"));
  }
  s_netServer = netServer;
  s_originalThreadPriority = SGetCurrentThreadPriority();
  while (threadSlotCount < threadCount) {
    threadSlotCount <<= 1;
    if (!threadSlotCount) {
      ASSERT(threadSlotCount);
      break;
    }
  }
  s_threadSlotCount = threadSlotCount;

  s_threadSlotCritsects = new SCritSect[threadSlotCount];
  s_threadSlots = new EvtThread *[s_threadSlotCount];
  memset(s_threadSlots, 0, sizeof(EvtThread *) * s_threadSlotCount);

  s_startEvent.Reset();
  s_shutdownEvent.Reset();

  s_mainThread = InitializeSchedulerThread();
  remaining = threadCount - 1;
  while (remaining) {
    thread = NEW(SThread);
    s_schedulerThreads.Add(1, &thread);
    SStrPrintf(threadname, sizeof(threadname), "EvtSched#%d", remaining);
    if (!SThread::Create(SchedulerThreadProc, 0, **s_schedulerThreads.Top(), threadname)) {
      thread = *s_schedulerThreads.Top();
      if (thread) {
        DEL(thread);
      }
      s_schedulerThreads.SetCount(s_schedulerThreads.Count() - 1);
    }
    --remaining;
  }
}

void IEvtSchedulerDestroy() {
  unsigned int processorCount;
  unsigned int index;
  SThread     *threadPtr;

  if (!s_threadSlotCount) {
    return;
  }

  {
    SEvent                     shutdownThreadEvent(1, 0);
    TSGrowableArray<SThread *> shutdownThreads;

    if (s_netServer) {
      processorCount = OsGetProcessorCount();
      threadPtr = NEW(SThread);
      shutdownThreads.Add(1, &threadPtr);
      while (processorCount) {
        SThread thread;
        if (!SThread::Create(ShutdownThreadProc, &shutdownThreadEvent, **shutdownThreads.Top(), "EvtShutdown")) {
          threadPtr = *shutdownThreads.Top();
          if (threadPtr) {
            DEL(threadPtr);
          }
          shutdownThreads.SetCount(shutdownThreads.Count() - 1);
        }
        --processorCount;
      }
    }

    WaitMultiplePtr(s_schedulerThreads.Count(), reinterpret_cast<SSyncObject **>(s_schedulerThreads.Ptr()), TRUE, INFINITE);
    if (shutdownThreads.Count()) {
      shutdownThreadEvent.Set();
      WaitMultiplePtr(shutdownThreads.Count(), reinterpret_cast<SSyncObject **>(shutdownThreads.Ptr()), TRUE, INFINITE);
      index = shutdownThreads.Count();
      while (index) {
        threadPtr = shutdownThreads[--index];
        if (threadPtr) {
          DEL(threadPtr);
        }
      }
    }
  }

  ASSERT(!s_threadList.Head());
  ASSERT(s_interactiveCount == 0);

  index = s_schedulerThreads.Count();
  while (index) {
    threadPtr = s_schedulerThreads[--index];
    if (threadPtr) {
      DEL(threadPtr);
    }
  }
  s_schedulerThreads.Clear();

  delete[] s_threadSlots;
  s_threadSlots = 0;

  delete[] s_threadSlotCritsects;
  s_threadSlotCritsects = 0;
  s_threadSlotCount = 0;
}

void IEvtSchedulerShutdown() {
  unsigned int slot;
  EvtThread   *thread;

  s_shutdownEvent.Set();
  if (!s_netServer) {
    s_threadListCritsect.Enter();
    for (slot = 0; slot < s_threadSlotCount; ++slot) {
      thread = s_threadSlots[slot];
      if (thread) {
        thread->m_wakeEvent.Set();
      }
    }
    s_threadListCritsect.Leave();
  }
}

HEVENTCONTEXT
IEvtSchedulerCreateContext(int interactive, EVENTHANDLER initializeHandler, EVENTHANDLER destroyHandler, DWORD idleTime, DWORD debugFlags) {
  char        contextName[256];
  int         startWatchdog;
  void       *callContext = 0;
  EvtContext *context;

  if (!idleTime) {
    idleTime = 1;
  }
  if (debugFlags & 0x1) {
    SStrPrintf(contextName, sizeof(contextName), "Context: interactive = %u, idleTime = %u", interactive, idleTime);
    callContext = OsCallInitializeContext(contextName);
  }
  startWatchdog = (debugFlags >> 1) & 1;

  context = NEW(EvtContext);
  if (context) {
    context->m_currTime = 0;
    context->m_schedState = EvtContext::SCHEDSTATE_ACTIVE;
    context->m_schedLastIdle = OsGetAsyncTimeMs();
    context->m_schedFlags = interactive ? 0x2 : 0;
    context->m_schedIdleTime = idleTime;
    context->m_schedInitialIdleTime = idleTime;
    context->m_schedWeight = interactive ? 1000 : 1;
    context->m_schedSmoothWeight = context->m_schedWeight;
    context->m_schedRebalance = 0;
    context->m_queueSyncButtonState = 0;
    context->m_propContext = PropCreateContext();
    context->m_callContext = callContext;
    context->m_startWatchdog = startWatchdog;
  }

  if (interactive) {
    SInterlockedIncrement(&s_interactiveCount);
  }
  if (initializeHandler) {
    IEvtQueueRegister(context, EVENT_ID_INITIALIZE, initializeHandler, 0, 1000.0f);
  }
  if (destroyHandler) {
    IEvtQueueRegister(context, EVENT_ID_DESTROY, destroyHandler, 0, 1000.0f);
  }
  return AttachContextToThread(context);
}

int IEvtSchedulerIsContextInteractive(EvtContext *context) {
  return context->SchedGetFlags(0x2) != 0;
}

void EventProcessStart() {
  char callName[64];

  PropSelectContext(0);
  s_hThread = s_mainThread;
  SStrPrintf(callName, sizeof(callName), "Engine %x", SGetCurrentThreadId());
  OsCallInitialize(callName);
  s_watchdogActive = 0;
}

void EventProcessOnce() {
  EvtContext *context;
  DWORD       nextDelay = INFINITE;
  DWORD       currTime;
  DWORD       idleTime;
  DWORD       wait;
  LONG        signedDelay;
  int         currentPriority;
  int         shutdown;
  int         closed;

  currentPriority = SGetCurrentThreadPriority();
  if (currentPriority != s_originalThreadPriority) {
    SSetCurrentThreadPriority(s_originalThreadPriority);
  }
  if (s_watchdogActive) {
    SErrPingWatchdog();
  }
  if (s_shutdownEvent.Wait(0) == WAIT_OBJECT_0) {
    return;
  }

  context = GetNextContext(s_hThread);
  if (context) {
    signedDelay = static_cast<LONG>(context->SchedGetNextWakeTime() - OsGetAsyncTimeMs());
    nextDelay = signedDelay < 0 ? 0 : static_cast<DWORD>(signedDelay);
  }
  if (s_netServer) {
    if (nextDelay == INFINITE) {
      nextDelay = 100;
    }
    OsNetPump(nextDelay);
    wait = WAIT_TIMEOUT;
  } else {
    wait = GetWakeEvent(s_hThread)->Wait(nextDelay);
  }

  if (!context) {
    return;
  }
  context->SchedSelect();
  currTime = OsGetAsyncTimeMs();
  context->SetCurrTime(currTime);

  if (wait == WAIT_TIMEOUT) {
    if (SynthesizeInitialize(context) && context->StartWatchdog()) {
      SErrStartWatchdog(20, TRUE);
      s_watchdogActive = 1;
    }
    IEvtTimerDispatch(context);
    shutdown = 0;
    if (context->SchedGetFlags(0x2)) {
      IEvtInputProcess(context, &shutdown);
      if (shutdown) {
        context->SchedSetClosed();
        IEvtSchedulerShutdown();
      }
    }
    SynthesizePoll(context);
    IEvtQueueDispatchAll(context);
    SynthesizeIdle(context);
    SynthesizePaint(context);
  }

  closed = context->SchedGetClosed();
  if (closed) {
    DetachContextFromThread(s_hThread, context);
    SynthesizeDestroy(context);
    return;
  }

  if (context->SchedGetFlags(0x4)) {
    nextDelay = 0;
  } else {
    nextDelay = IEvtTimerGetNextTime(context, currTime);
    idleTime = context->SchedGetIdleTime();
    if (idleTime != context->SchedGetInitialIdleTime()) {
      nextDelay = idleTime;
    }
    signedDelay = static_cast<LONG>(idleTime + context->SchedGetLastIdle() - currTime);
    if (nextDelay >= static_cast<DWORD>(signedDelay < 0 ? 0 : signedDelay)) {
      nextDelay = signedDelay < 0 ? 0 : signedDelay;
    }
  }
  PutContext(s_hThread, context, currTime + nextDelay, context->SchedGetSmoothWeight());
}

void EventProcessDone() {
  if (s_watchdogActive) {
    SErrStopWatchdog();
  }
  DestroySchedulerThread(s_hThread);
  OsCallDestroy();
}
