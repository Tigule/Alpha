#pragma once

#include <Base/Base.h>
#include <Base/CDataRecycler.h>
#include <Base/InstanceId.h>
#include "EvtApi.h"

struct EvtContext;
struct EvtHandler;
struct EvtMessage;
struct EvtKeyDown;
struct EvtTimer;
struct EvtThread;

void OsCallSetContext(void *contextDataPtr);
void OsCallResetContext(void *contextDataPtr);
void OsCallDestroyContext(void *contextDataPtr);

template <class T>
class EvtIdTable {
 public:
  unsigned int Alloc() {
    unsigned int id;
    unsigned int count = m_freeArray.Count();

    if (count) {
      id = m_freeArray[count - 1];
      m_freeArray.SetCount(count - 1);
    } else {
      id = m_allocArray.Count();
      if (!id) {
        id = 1;
      }
      m_allocArray.GrowToFit(id, 1);
    }
    return id;
  }

  void Free(unsigned int id) {
    if (id) {
      ASSERT(id < m_allocArray.Count());
      m_freeArray.Add(1, &id);
    }
  }

  unsigned int NumAllocated() const {
    return m_allocArray.Count() - m_freeArray.Count();
  }

  T &operator[](unsigned int id) {
    return m_allocArray[id];
  }

  const T &operator[](unsigned int id) const {
    return const_cast<EvtIdTable<T> *>(this)->m_allocArray[id];
  }

  TSGrowableArray<T>            m_allocArray;
  TSGrowableArray<unsigned int> m_freeArray;
};

class EvtTimerQueue : public TSPriorityQueue<EvtTimer> {
 public:
  EvtTimerQueue() : TSPriorityQueue<EvtTimer>(4) {
  }
};

class EvtContextQueue : public TSPriorityQueue<EvtContext> {
 public:
  EvtContextQueue() : TSPriorityQueue<EvtContext>(0x30) {
  }
};

struct EvtHandler {
  LINKDECLEX(EvtHandler, link);
  EVENTHANDLER       func;
  void              *param;
  float              priority;
  int                marker;
};

struct EvtMessage : public TExtraInstanceRecyclable<EvtMessage> {
  LINKDECLEX(EvtMessage, link);
  EVENTID            id;
  BYTE               data[4];
};

struct EvtKeyDown {
  LINKDECLEX(EvtKeyDown, link);
  KEY                key;
};

struct EvtTimer {
  unsigned int           id;
  TSTimerPriority<DWORD> targetTime;
  float                  timeout;
  EVENTHANDLER           handler;
  void                  *param;
  EVENTGUIDHANDLER       guidHandler;
  unsigned __int64       guidParam;
  void                  *guidParam2;
};

struct EvtContext : public TSingletonInstanceId<EvtContext, 8> {
  enum SCHEDSTATE {
    SCHEDSTATE_ACTIVE = 0,
    SCHEDSTATE_CLOSED = 1,
    SCHEDSTATE_DESTROYED = 2
  };

  SCritSect                     m_critsect;
  DWORD                         m_currTime;
  SCHEDSTATE                    m_schedState;
  TSTimerPriority<DWORD>        m_schedNextWakeTime;
  DWORD                         m_schedLastIdle;
  DWORD                         m_schedFlags;
  DWORD                         m_schedIdleTime;
  DWORD                         m_schedInitialIdleTime;
  unsigned int                  m_schedWeight;
  unsigned int                  m_schedSmoothWeight;
  int                           m_schedRebalance;
  LISTDECLEX(EvtHandler, link, m_queueHandlerList[EVENTIDS]);
  LISTDECLEX(EvtMessage, link, m_queueMessageList);
  DWORD                         m_queueSyncButtonState;
  LISTDECLEX(EvtKeyDown, link, m_queueSyncKeyDownList);
  EvtIdTable<EvtTimer *>        m_timerIdTable;
  EvtTimerQueue                 m_timerQueue;
  HPROPCONTEXT                  m_propContext;
  void                         *m_callContext;
  unsigned int                  m_startWatchdog;

  ~EvtContext() {
    EvtTimer    *timer;
    unsigned int i;
    unsigned int loop;

    for (i = 0; i < EVENTIDS; ++i) {
      m_queueHandlerList[i].Clear();
    }
    m_queueMessageList.Clear();
    m_queueSyncKeyDownList.Clear();
    for (loop = 1; loop < m_timerIdTable.m_allocArray.Count(); ++loop) {
      ASSERT(m_timerIdTable[loop]);
      timer = m_timerIdTable[loop];
      if (timer) {
        DEL(timer);
      }
    }
    PropDeleteContext(m_propContext);
    OsCallDestroyContext(m_callContext);
  }

  int IsCurrentContext() {
    return !Id() || reinterpret_cast<void *>(Id()) == PropGet(PROP_EVENTCONTEXT);
  }

  HEVENTCONTEXT Handle() {
    return reinterpret_cast<HEVENTCONTEXT>(Id());
  }

  unsigned long GetCurrTime() {
    ASSERT(IsCurrentContext());
    return m_currTime;
  }

  void SetCurrTime(unsigned long currTime) {
    ASSERT(IsCurrentContext());
    m_currTime = currTime;
  }

  void SchedSelect() {
    PropSelectContext(m_propContext);
    PropSet(PROP_EVENTCONTEXT, reinterpret_cast<void *>(Id()));
    OsCallSetContext(m_callContext);
  }

  void SchedDeselect() {
    ASSERT(IsCurrentContext());
    OsCallResetContext(m_callContext);
    PropSelectContext(0);
  }

  int SchedGetClosed() {
    int closed;

    m_critsect.Enter();
    closed = m_schedState == SCHEDSTATE_CLOSED;
    m_critsect.Leave();
    return closed;
  }

  void SchedSetClosed() {
    m_critsect.Enter();
    if (m_schedState == SCHEDSTATE_ACTIVE) {
      m_schedState = SCHEDSTATE_CLOSED;
    }
    m_critsect.Leave();
  }

  int SchedGetDestroyed() {
    int destroyed;

    m_critsect.Enter();
    destroyed = m_schedState == SCHEDSTATE_DESTROYED;
    m_critsect.Leave();
    return destroyed;
  }

  void SchedSetDestroyed() {
    m_critsect.Enter();
    m_schedState = SCHEDSTATE_DESTROYED;
    m_critsect.Leave();
  }

  unsigned long SchedGetNextWakeTime() {
    return m_schedNextWakeTime.Get();
  }

  void SchedSetNextWakeTime(unsigned long nextWakeTime) {
    m_schedNextWakeTime.Set(nextWakeTime);
  }

  unsigned long SchedGetLastIdle() {
    ASSERT(IsCurrentContext());
    return m_schedLastIdle;
  }

  void SchedSetLastIdle(unsigned long lastIdle) {
    ASSERT(IsCurrentContext());
    m_schedLastIdle = lastIdle;
  }

  unsigned long SchedGetFlags(unsigned long flags) {
    ASSERT(IsCurrentContext());
    return m_schedFlags & flags;
  }

  void SchedSetFlags(unsigned long flags) {
    ASSERT(IsCurrentContext());
    m_schedFlags |= flags;
  }

  void SchedResetFlags(unsigned long flags) {
    ASSERT(IsCurrentContext());
    m_schedFlags &= ~flags;
  }

  unsigned long SchedGetInitialIdleTime() {
    ASSERT(IsCurrentContext());
    return m_schedInitialIdleTime;
  }

  unsigned long SchedGetIdleTime() {
    ASSERT(IsCurrentContext());
    return m_schedIdleTime;
  }

  void SchedSetIdleTime(unsigned long idleTime) {
    m_schedIdleTime = idleTime;
  }

  unsigned int SchedGetWeight() {
    return m_schedWeight;
  }

  void SchedSetWeight(unsigned int weight) {
    m_schedWeight = weight;
  }

  unsigned int SchedGetSmoothWeight() {
    return m_schedSmoothWeight;
  }

  void SchedSetSmoothWeight(unsigned int smoothWeight) {
    m_schedSmoothWeight = smoothWeight;
  }

  int SchedGetRebalance() {
    return m_schedRebalance;
  }

  void SchedSetRebalance(int rebalance) {
    m_schedRebalance = rebalance;
  }

  int StartWatchdog() {
    return m_startWatchdog;
  }

  TSExplicitList<EvtHandler, 0> &QueueLockHandlerList(EVENTID id) {
    ASSERT(IsCurrentContext());
    ASSERT(id >= 0 && id < EVENTIDS);
    return m_queueHandlerList[id];
  }

  void QueueUnlockHandlerList() {
    ASSERT(IsCurrentContext());
  }

  TSExplicitList<EvtMessage, 4> &QueueLockMessageList() {
    m_critsect.Enter();
    return m_queueMessageList;
  }

  void QueueUnlockMessageList() {
    m_critsect.Leave();
  }

  TSExplicitList<EvtKeyDown, 0> &QueueLockSyncKeyDownList() {
    ASSERT(IsCurrentContext());
    m_critsect.Enter();
    return m_queueSyncKeyDownList;
  }

  void QueueUnlockSyncKeyDownList() {
    ASSERT(IsCurrentContext());
    m_critsect.Leave();
  }

  unsigned int QueueGetSyncButtonState(unsigned int flags) {
    ASSERT(IsCurrentContext());
    return m_queueSyncButtonState & flags;
  }

  void QueueSetSyncButtonState(unsigned int flags) {
    ASSERT(IsCurrentContext());
    m_queueSyncButtonState |= flags;
  }

  void QueueResetSyncButtonState(unsigned int flags) {
    ASSERT(IsCurrentContext());
    m_queueSyncButtonState &= ~flags;
  }

  void TimerLockIdTableAndQueue(EvtIdTable<EvtTimer *> *&table, EvtTimerQueue *&queue) {
    ASSERT(IsCurrentContext());
    m_critsect.Enter();
    table = &m_timerIdTable;
    queue = &m_timerQueue;
  }

  void TimerUnlockIdTableAndQueue() {
    ASSERT(IsCurrentContext());
    m_critsect.Leave();
  }
};

NODEDECL(EvtThread) {
  DWORD           m_threadSlot;
  DWORD           m_threadCount;
  DWORD           m_weightTotal;
  DWORD           m_weightAvg;
  DWORD           m_contextCount;
  DWORD           m_rebalance;
  SEvent          m_wakeEvent;
  EvtContextQueue m_contextQueue;
};

void IEvtQueueInitialize();
void IEvtQueueDestroy();
void IEvtQueueRegister(EvtContext *context, EVENTID id, EVENTHANDLER handler, void *param, float priority);
void IEvtQueueUnregister(EvtContext *context, EVENTID id, EVENTHANDLER handler, void *param, unsigned int flags);
void IEvtQueueDispatch(EvtContext *context, EVENTID id, const void *data);
void IEvtQueueDispatchAll(EvtContext *context);
int IEvtQueueHasMessages(EvtContext *context);
int IEvtQueueDispatchNext(EvtContext *context);
void IEvtQueuePost(EvtContext *context, EVENTID id, const void *data, unsigned int bytes);
void IEvtQueueScan(EvtContext *context, EVENTSCANHANDLER scanner, void *param);
int IEvtQueueCheckSyncKeyState(EvtContext *context, KEY key);
int IEvtQueueCheckSyncMouseState(EvtContext *context, MOUSEBUTTON button);

int IEvtTimerDispatch(EvtContext *context);
unsigned int IEvtTimerGetNextTime(EvtContext *context, DWORD currTime);
float IEvtTimerGetRemaining(EvtContext *context, unsigned int id);
void IEvtTimerKill(EvtContext *context, unsigned int id, EVENTHANDLER handlerFunction, const char *functionName);
unsigned int IEvtTimerSet(
    EvtContext      *context,
    float            timeout,
    EVENTHANDLER     handler,
    void            *param,
    EVENTGUIDHANDLER guidHandler,
    unsigned __int64 guidParam,
    void            *guidParam2
);
unsigned int IEvtTimerSet(
    EvtContext      *context,
    unsigned int     timeout,
    EVENTHANDLER     handler,
    void            *param,
    EVENTGUIDHANDLER guidHandler,
    unsigned __int64 guidParam,
    void            *guidParam2
);
unsigned int IEvtTimerSetAbsolute(
    EvtContext      *context,
    DWORD            triggerTime,
    EVENTHANDLER     handler,
    void            *param,
    EVENTGUIDHANDLER guidHandler,
    unsigned __int64 guidParam,
    void            *guidParam2
);

void IEvtInputInitialize();
void IEvtInputDestroy();
int IEvtInputProcess(EvtContext *context, int *shutdown);
void IEvtInputSetMouseMode(EvtContext *context, MOUSEMODE mode, unsigned int holdButton);
void IEvtInputSetConfirmCloseCallback(EVENTCONFIRMCLOSEHANDLER inFunc, void *inParam);
void IEvtInputGetMousePosition(float *x, float *y);
void IEvtInputSetMousePosition(float x, float y);

namespace NTempest {
  class CRect;
}

void IEvtInputSetMouseBoundingRect(NTempest::CRect *rect);

void IEvtSchedulerInitialize(unsigned int threadCount, int netServer);
void IEvtSchedulerDestroy();
void IEvtSchedulerProcess();
void IEvtSchedulerShutdown();
HEVENTCONTEXT
IEvtSchedulerCreateContext(int interactive, EVENTHANDLER initializeHandler, EVENTHANDLER destroyHandler, DWORD idleTime, DWORD debugFlags);
int IEvtSchedulerIsContextInteractive(EvtContext *context);
