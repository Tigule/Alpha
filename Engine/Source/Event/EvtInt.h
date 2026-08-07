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

void OsCallSetContext(LPVOID contextDataPtr);
void OsCallResetContext(LPVOID contextDataPtr);
void OsCallDestroyContext(LPVOID contextDataPtr);

template <class T>
class EvtIdTable {
 public:
  UINT Alloc() {
    UINT id;
    UINT count = m_freeArray.Count();

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

  void Free(UINT id) {
    if (id) {
      ASSERT(id < m_allocArray.Count());
      m_freeArray.Add(1, &id);
    }
  }

  UINT NumAllocated() const {
    return m_allocArray.Count() - m_freeArray.Count();
  }

  T &operator[](UINT id) {
    return m_allocArray[id];
  }

  const T &operator[](UINT id) const {
    return const_cast<EvtIdTable<T> *>(this)->m_allocArray[id];
  }

 private:
  TSGrowableArray<T>    m_allocArray;
  TSGrowableArray<UINT> m_freeArray;

  friend struct EvtContext;
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
  EVENTHANDLER func;
  LPVOID       param;
  float        priority;
  int          marker;
};

struct EvtMessage : public TExtraInstanceRecyclable<EvtMessage> {
  LINKDECLEX(EvtMessage, link);
  EVENTID id;
  BYTE    data[4];
};

struct EvtKeyDown {
  LINKDECLEX(EvtKeyDown, link);
  KEY key;
};

struct EvtTimer {
  UINT                   id;
  TSTimerPriority<DWORD> targetTime;
  float                  timeout;
  EVENTHANDLER           handler;
  LPVOID                 param;
  EVENTGUIDHANDLER       guidHandler;
  DWORDLONG              guidParam;
  LPVOID                 guidParam2;
};

struct EvtContext : public TSingletonInstanceId<EvtContext, 8> {
  enum SCHEDSTATE {
    SCHEDSTATE_ACTIVE = 0,
    SCHEDSTATE_CLOSED = 1,
    SCHEDSTATE_DESTROYED = 2,
    _UNIQUE_SYMBOL_SCHEDSTATE_96 = -1
  };

 private:
  EvtContext(DWORD idleTime, DWORD flags, UINT weight, LPVOID callContext, int startWatchdog);
  EvtContext(const EvtContext &);
  EvtContext &operator=(const EvtContext &);

  friend void          DestroySchedulerThread(UINT hThread);
  friend HEVENTCONTEXT AttachContextToThread(EvtContext *context);
  friend void          DetachContextFromThread(UINT hThread, EvtContext *context);
  friend void          PutContext(UINT hThread, EvtContext *context, DWORD nextWakeTime, DWORD newSmoothWeight);
  friend HEVENTCONTEXT
  IEvtSchedulerCreateContext(int interactive, EVENTHANDLER initializeHandler, EVENTHANDLER destroyHandler, DWORD idleTime, DWORD debugFlags);

  SCritSect              m_critsect;
  DWORD                  m_currTime;
  SCHEDSTATE             m_schedState;
  TSTimerPriority<DWORD> m_schedNextWakeTime;
  DWORD                  m_schedLastIdle;
  DWORD                  m_schedFlags;
  DWORD                  m_schedIdleTime;
  DWORD                  m_schedInitialIdleTime;
  UINT                   m_schedWeight;
  UINT                   m_schedSmoothWeight;
  int                    m_schedRebalance;
  LISTDECLEX(EvtHandler, link, m_queueHandlerList[EVENTIDS]);
  LISTDECLEX(EvtMessage, link, m_queueMessageList);
  UINT m_queueSyncButtonState;
  LISTDECLEX(EvtKeyDown, link, m_queueSyncKeyDownList);
  EvtIdTable<EvtTimer *> m_timerIdTable;
  EvtTimerQueue          m_timerQueue;
  HPROPCONTEXT           m_propContext;
  LPVOID                 m_callContext;
  UINT                   m_startWatchdog;

 public:
  ~EvtContext() {
    EvtTimer *timer;
    UINT      i;
    UINT      loop;

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

  BOOL IsCurrentContext() const {
    return !Id() || reinterpret_cast<LPVOID>(Id()) == PropGet(PROP_EVENTCONTEXT);
  }

  HEVENTCONTEXT Handle() const {
    return reinterpret_cast<HEVENTCONTEXT>(Id());
  }

  DWORD GetCurrTime() const {
    ASSERT(IsCurrentContext());
    return m_currTime;
  }

  void SetCurrTime(DWORD currTime) {
    ASSERT(IsCurrentContext());
    m_currTime = currTime;
  }

  void SchedSelect() {
    PropSelectContext(m_propContext);
    PropSet(PROP_EVENTCONTEXT, reinterpret_cast<LPVOID>(Id()));
    OsCallSetContext(m_callContext);
  }

  void SchedDeselect() {
    ASSERT(IsCurrentContext());
    OsCallResetContext(m_callContext);
    PropSelectContext(0);
  }

  BOOL SchedGetClosed() {
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

  BOOL SchedGetDestroyed() {
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

  DWORD SchedGetNextWakeTime() const {
    return m_schedNextWakeTime.Get();
  }

  void SchedSetNextWakeTime(DWORD nextWakeTime) {
    m_schedNextWakeTime.Set(nextWakeTime);
  }

  DWORD SchedGetLastIdle() const {
    ASSERT(IsCurrentContext());
    return m_schedLastIdle;
  }

  void SchedSetLastIdle(DWORD lastIdle) {
    ASSERT(IsCurrentContext());
    m_schedLastIdle = lastIdle;
  }

  DWORD SchedGetFlags(DWORD flags) const {
    ASSERT(IsCurrentContext());
    return m_schedFlags & flags;
  }

  void SchedSetFlags(DWORD flags) {
    ASSERT(IsCurrentContext());
    m_schedFlags |= flags;
  }

  void SchedResetFlags(DWORD flags) {
    ASSERT(IsCurrentContext());
    m_schedFlags &= ~flags;
  }

  DWORD SchedGetInitialIdleTime() const {
    ASSERT(IsCurrentContext());
    return m_schedInitialIdleTime;
  }

  DWORD SchedGetIdleTime() const {
    ASSERT(IsCurrentContext());
    return m_schedIdleTime;
  }

  void SchedSetIdleTime(DWORD idleTime) {
    m_schedIdleTime = idleTime;
  }

  UINT SchedGetWeight() const {
    return m_schedWeight;
  }

  void SchedSetWeight(UINT weight) {
    m_schedWeight = weight;
  }

  UINT SchedGetSmoothWeight() const {
    return m_schedSmoothWeight;
  }

  void SchedSetSmoothWeight(UINT smoothWeight) {
    m_schedSmoothWeight = smoothWeight;
  }

  int SchedGetRebalance() const {
    return m_schedRebalance;
  }

  void SchedSetRebalance(int rebalance) {
    m_schedRebalance = rebalance;
  }

  int StartWatchdog() {
    return m_startWatchdog;
  }

  LISTEX(EvtHandler, link) & QueueLockHandlerList(EVENTID id) {
    ASSERT(IsCurrentContext());
    ASSERT(id >= 0 && id < EVENTIDS);
    return m_queueHandlerList[id];
  }

  void QueueUnlockHandlerList() {
    ASSERT(IsCurrentContext());
  }

  LISTEX(EvtMessage, link) & QueueLockMessageList() {
    m_critsect.Enter();
    return m_queueMessageList;
  }

  void QueueUnlockMessageList() {
    m_critsect.Leave();
  }

  LISTEX(EvtKeyDown, link) & QueueLockSyncKeyDownList() {
    ASSERT(IsCurrentContext());
    m_critsect.Enter();
    return m_queueSyncKeyDownList;
  }

  void QueueUnlockSyncKeyDownList() {
    ASSERT(IsCurrentContext());
    m_critsect.Leave();
  }

  UINT QueueGetSyncButtonState(UINT flags) {
    ASSERT(IsCurrentContext());
    return m_queueSyncButtonState & flags;
  }

  void QueueSetSyncButtonState(UINT flags) {
    ASSERT(IsCurrentContext());
    m_queueSyncButtonState |= flags;
  }

  void QueueResetSyncButtonState(UINT flags) {
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
  UINT            m_threadSlot;
  UINT            m_threadCount;
  UINT            m_weightTotal;
  UINT            m_weightAvg;
  UINT            m_contextCount;
  UINT            m_rebalance;
  SEvent          m_wakeEvent;
  EvtContextQueue m_contextQueue;
};

void IEvtQueueInitialize();
void IEvtQueueDestroy();
void IEvtQueueRegister(EvtContext *context, EVENTID id, EVENTHANDLER handler, LPVOID param, float priority);
void IEvtQueueUnregister(EvtContext *context, EVENTID id, EVENTHANDLER handler, LPVOID param, UINT flags);
void IEvtQueueDispatch(EvtContext *context, EVENTID id, LPCVOID data);
void IEvtQueueDispatchAll(EvtContext *context);
BOOL IEvtQueueHasMessages(EvtContext *context);
BOOL IEvtQueueDispatchNext(EvtContext *context);
void IEvtQueuePost(EvtContext *context, EVENTID id, LPCVOID data, UINT bytes);
void IEvtQueueScan(EvtContext *context, EVENTSCANHANDLER scanner, LPVOID param);
BOOL IEvtQueueCheckSyncKeyState(EvtContext *context, KEY key);
BOOL IEvtQueueCheckSyncMouseState(EvtContext *context, MOUSEBUTTON button);

BOOL  IEvtTimerDispatch(EvtContext *context);
UINT  IEvtTimerGetNextTime(EvtContext *context, DWORD currTime);
float IEvtTimerGetRemaining(EvtContext *context, UINT id);
void  IEvtTimerKill(EvtContext *context, UINT id, EVENTHANDLER handlerFunction, LPCSTR functionName);
UINT  IEvtTimerSet(
    EvtContext      *context,
    float            timeout,
    EVENTHANDLER     handler,
    LPVOID           param,
    EVENTGUIDHANDLER guidHandler,
    DWORDLONG        guidParam,
    LPVOID           guidParam2
);
UINT IEvtTimerSet(
    EvtContext      *context,
    UINT             timeout,
    EVENTHANDLER     handler,
    LPVOID           param,
    EVENTGUIDHANDLER guidHandler,
    DWORDLONG        guidParam,
    LPVOID           guidParam2
);
UINT IEvtTimerSetAbsolute(
    EvtContext      *context,
    DWORD            triggerTime,
    EVENTHANDLER     handler,
    LPVOID           param,
    EVENTGUIDHANDLER guidHandler,
    DWORDLONG        guidParam,
    LPVOID           guidParam2
);

void IEvtInputInitialize();
void IEvtInputDestroy();
BOOL IEvtInputProcess(EvtContext *context, int *shutdown);
void IEvtInputSetMouseMode(EvtContext *context, MOUSEMODE mode, UINT holdButton);
void IEvtInputSetConfirmCloseCallback(EVENTCONFIRMCLOSEHANDLER inFunc, LPVOID inParam);
void IEvtInputGetMousePosition(float *x, float *y);
void IEvtInputSetMousePosition(float x, float y);

namespace NTempest {
  class CRect;
}

void IEvtInputSetMouseBoundingRect(NTempest::CRect *rect);

void IEvtSchedulerInitialize(UINT threadCount, int netServer);
void IEvtSchedulerDestroy();
void IEvtSchedulerProcess();
void IEvtSchedulerShutdown();
HEVENTCONTEXT
IEvtSchedulerCreateContext(int interactive, EVENTHANDLER initializeHandler, EVENTHANDLER destroyHandler, DWORD idleTime, DWORD debugFlags);
BOOL IEvtSchedulerIsContextInteractive(EvtContext *context);
