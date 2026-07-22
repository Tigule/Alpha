#include "EvtInt.h"

void __fastcall ObserverInitialize();
void __fastcall ObserverDestroy();
void __fastcall InputObserverInitialize();
void __fastcall InputObserverDestroy();
int __fastcall  IEvtQueueCheckSyncKeyState(EvtContext *context, KEY key);
int __fastcall  IEvtQueueCheckSyncMouseState(EvtContext *context, MOUSEBUTTON button);
void __fastcall IEvtQueueScan(EvtContext *context, EVENTSCANHANDLER scanner, void *param);
void __fastcall IEvtInputSetConfirmCloseCallback(EVENTCONFIRMCLOSEHANDLER callback, void *param);

void __fastcall EventInitialize(unsigned int threadCount, int netServer) {
  ObserverInitialize();
  InputObserverInitialize();
  IEvtQueueInitialize();
  IEvtInputInitialize();

  if (threadCount < 1) {
    threadCount = 1;
  }

  IEvtSchedulerInitialize(threadCount, netServer);
}

void __fastcall EventDestroy() {
  IEvtSchedulerDestroy();
  IEvtInputDestroy();
  IEvtQueueDestroy();
  InputObserverDestroy();
  ObserverDestroy();
}

void __fastcall EventDoMessageLoop() {
  IEvtSchedulerProcess();
}

void __fastcall EventInitiateShutdown() {
  IEvtSchedulerShutdown();
}

HEVENTCONTEXT __fastcall
EventCreateContextEx(int interactive, EVENTHANDLER initializeHandler, EVENTHANDLER destroyHandler, DWORD idleTime, DWORD debugFlags) {
  return IEvtSchedulerCreateContext(interactive, initializeHandler, destroyHandler, idleTime, debugFlags);
}

void __fastcall EventCreateContext(int interactive, EVENTHANDLER initializeHandler, EVENTHANDLER destroyHandler) {
  EventCreateContextEx(interactive, initializeHandler, destroyHandler, interactive ? 1 : 500, 0);
}

int __fastcall EventIsContextInteractive() {
  INSTANCELOCK instanceLock;
  DWORD        id = reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT));
  EvtContext  *context = EvtContext::GetTable().Lock(id, 0, instanceLock, __FILE__, __LINE__);
  int          interactive = context ? IEvtSchedulerIsContextInteractive(context) : 0;
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  return interactive;
}

HEVENTCONTEXT __fastcall EventGetCurrentContext() {
  return reinterpret_cast<HEVENTCONTEXT>(PropGet(PROP_EVENTCONTEXT));
}

void __fastcall EventSetContextIdleTime(DWORD idleTime, HEVENTCONTEXT hContext) {
  INSTANCELOCK instanceLock;
  DWORD        id = hContext ? reinterpret_cast<DWORD>(hContext) : reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT));
  EvtContext  *context = EvtContext::GetTable().Lock(id, 0, instanceLock, __FILE__, __LINE__);
  if (!context) {
    return;
  }

  context->SchedSetIdleTime(idleTime);
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
}

DWORD __fastcall EventGetContextIdleTime(HEVENTCONTEXT hContext) {
  INSTANCELOCK instanceLock;
  DWORD        id = hContext ? reinterpret_cast<DWORD>(hContext) : reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT));
  EvtContext  *context = EvtContext::GetTable().Lock(id, 0, instanceLock, __FILE__, __LINE__);
  DWORD        idleTime;

  if (!context) {
    return 0;
  }

  idleTime = context->SchedGetIdleTime();
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);

  return idleTime;
}

int __fastcall EventIsButtonDown(MOUSEBUTTON button) {
  INSTANCELOCK instanceLock;
  DWORD        id = reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT));
  EvtContext  *context = EvtContext::GetTable().Lock(id, 0, instanceLock, __FILE__, __LINE__);
  int          down = context ? IEvtQueueCheckSyncMouseState(context, button) : 0;
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  return down;
}

int __fastcall EventIsKeyDown(KEY key) {
  INSTANCELOCK instanceLock;
  DWORD        id = reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT));
  EvtContext  *context = EvtContext::GetTable().Lock(id, 0, instanceLock, __FILE__, __LINE__);
  int          down = context ? IEvtQueueCheckSyncKeyState(context, key) : 0;
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  return down;
}

void __fastcall EventPostClose() {
  EventPostCloseEx(0);
}

void __fastcall EventPostCloseEx(HEVENTCONTEXT hContext) {
  INSTANCELOCK instanceLock;
  DWORD        id = hContext ? reinterpret_cast<DWORD>(hContext) : reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT));
  EvtContext  *context = EvtContext::GetTable().Lock(id, 0, instanceLock, __FILE__, __LINE__);
  if (context) {
    context->SchedSetClosed();
  }
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
}

int __fastcall EventQueuePost(HEVENTCONTEXT hContext, EVENTID id, const void *data, unsigned int bytes) {
  INSTANCELOCK instanceLock;
  DWORD        contextId = hContext ? reinterpret_cast<DWORD>(hContext) : reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT));
  EvtContext  *context = EvtContext::GetTable().Lock(contextId, 0, instanceLock, __FILE__, __LINE__);
  int          result = 0;

  if (context && !context->SchedGetDestroyed()) {
    IEvtQueuePost(context, id, data, bytes);
    result = 1;
  }

  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  return result;
}

int __fastcall EventQueueScan(EVENTSCANHANDLER scanner, void *param) {
  INSTANCELOCK instanceLock;
  DWORD        id = reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT));
  EvtContext  *context = EvtContext::GetTable().Lock(id, 0, instanceLock, __FILE__, __LINE__);
  int          result = 0;

  if (context && !context->SchedGetDestroyed()) {
    IEvtQueueScan(context, scanner, param);
    result = 1;
  }

  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  return result;
}

void __fastcall EventRegister(EVENTID id, EVENTHANDLER handler) {
  EventRegisterEx(id, handler, 0, EVENT_PRIORITY_NORMAL);
}

void __fastcall EventRegisterEx(EVENTID id, EVENTHANDLER handler, void *param, float priority) {
  HEVENTCONTEXT hContext;
  EvtContext   *context;

  FATALASSERT(id >= 0);
  FATALASSERT(id < EVENTIDS);
  FATALASSERT(handler);

  INSTANCELOCK instanceLock;
  hContext = reinterpret_cast<HEVENTCONTEXT>(PropGet(PROP_EVENTCONTEXT));
  context = EvtContext::GetTable().Lock(reinterpret_cast<DWORD>(hContext), 0, instanceLock, __FILE__, __LINE__);
  if (context) {
    IEvtQueueRegister(context, id, handler, param, priority);
  }
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
}

void __fastcall EventUnregister(EVENTID id, EVENTHANDLER handler) {
  EventUnregisterEx(id, handler, 0, 0xFFFFFFFF);
}

void __fastcall EventUnregisterEx(EVENTID id, EVENTHANDLER handler, void *param, unsigned int flags) {
  INSTANCELOCK instanceLock;
  DWORD        contextId = reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT));
  EvtContext  *context = EvtContext::GetTable().Lock(contextId, 0, instanceLock, __FILE__, __LINE__);
  if (context) {
    IEvtQueueUnregister(context, id, handler, param, flags);
  }
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
}

void __fastcall EventSetConfirmCloseCallback(EVENTCONFIRMCLOSEHANDLER inFunc, void *inParam) {
  IEvtInputSetConfirmCloseCallback(inFunc, inParam);
}

int __fastcall EventInputProcess(HEVENTCONTEXT hContext) {
  INSTANCELOCK instanceLock;
  DWORD        id = hContext ? reinterpret_cast<DWORD>(hContext) : reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT));
  EvtContext  *context = EvtContext::GetTable().Lock(id, 0, instanceLock, __FILE__, __LINE__);
  if (!context) {
    return 0;
  }

  int shutdown = 0;
  int result = IEvtInputProcess(context, &shutdown);
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  return result;
}

unsigned int __fastcall EventSetTimer(float timeout, EVENTHANDLER handler, void *param) {
  INSTANCELOCK instanceLock;
  DWORD        contextId = reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT));
  EvtContext  *context = EvtContext::GetTable().Lock(contextId, 0, instanceLock, __FILE__, __LINE__);
  unsigned int timerId = context ? IEvtTimerSet(context, timeout, handler, param, 0, 0, 0) : 0;
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  return timerId;
}

unsigned int __fastcall EventSetTimer(float timeout, EVENTGUIDHANDLER handler, unsigned __int64 param, void *param2) {
  INSTANCELOCK instanceLock;
  DWORD        contextId = reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT));
  EvtContext  *context = EvtContext::GetTable().Lock(contextId, 0, instanceLock, __FILE__, __LINE__);
  unsigned int timerId = context ? IEvtTimerSet(context, timeout, 0, 0, handler, param, param2) : 0;
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  return timerId;
}

unsigned int __fastcall EventSetTimer(unsigned int timeout, EVENTHANDLER handler, void *param) {
  INSTANCELOCK instanceLock;
  DWORD        contextId = reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT));
  EvtContext  *context = EvtContext::GetTable().Lock(contextId, 0, instanceLock, __FILE__, __LINE__);
  unsigned int timerId = context ? IEvtTimerSet(context, timeout, handler, param, 0, 0, 0) : 0;
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  return timerId;
}

unsigned int __fastcall EventSetTimer(unsigned int timeout, EVENTGUIDHANDLER handler, unsigned __int64 param, void *param2) {
  INSTANCELOCK instanceLock;
  DWORD        contextId = reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT));
  EvtContext  *context = EvtContext::GetTable().Lock(contextId, 0, instanceLock, __FILE__, __LINE__);
  unsigned int timerId = context ? IEvtTimerSet(context, timeout, 0, 0, handler, param, param2) : 0;
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  return timerId;
}

unsigned int __fastcall EventSetTimerAbsolute(DWORD triggerTime, EVENTHANDLER handler, void *param) {
  INSTANCELOCK instanceLock;
  DWORD        contextId = reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT));
  EvtContext  *context = EvtContext::GetTable().Lock(contextId, 0, instanceLock, __FILE__, __LINE__);
  unsigned int timerId = context ? IEvtTimerSetAbsolute(context, triggerTime, handler, param, 0, 0, 0) : 0;
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  return timerId;
}

unsigned int __fastcall EventSetTimerAbsolute(DWORD triggerTime, EVENTGUIDHANDLER handler, unsigned __int64 param, void *param2) {
  INSTANCELOCK instanceLock;
  DWORD        contextId = reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT));
  EvtContext  *context = EvtContext::GetTable().Lock(contextId, 0, instanceLock, __FILE__, __LINE__);
  unsigned int timerId = context ? IEvtTimerSetAbsolute(context, triggerTime, 0, 0, handler, param, param2) : 0;
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  return timerId;
}

void __fastcall EventKillTimer(unsigned int timerId, EVENTHANDLER handlerFunction, const char *functionName) {
  INSTANCELOCK instanceLock;
  DWORD        contextId = reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT));
  EvtContext  *context = EvtContext::GetTable().Lock(contextId, 0, instanceLock, __FILE__, __LINE__);
  if (context) {
    IEvtTimerKill(context, timerId, handlerFunction, functionName);
  }
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
}

float __fastcall EventGetRemainingTime(unsigned int timerId) {
  INSTANCELOCK instanceLock;
  DWORD        contextId = reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT));
  EvtContext  *context = EvtContext::GetTable().Lock(contextId, 0, instanceLock, __FILE__, __LINE__);
  float        result = context ? IEvtTimerGetRemaining(context, timerId) : 0.0f;
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  return result;
}

void __fastcall EventSetMouseMode(MOUSEMODE mode, unsigned int holdButton) {
  INSTANCELOCK  instanceLock;
  HEVENTCONTEXT hContext;
  EvtContext   *context;

  FATALASSERT(mode < MOUSE_MODES);

  hContext = reinterpret_cast<HEVENTCONTEXT>(PropGet(PROP_EVENTCONTEXT));
  context = EvtContext::GetTable().Lock(reinterpret_cast<DWORD>(hContext), 0, instanceLock, __FILE__, __LINE__);
  if (context) {
    IEvtInputSetMouseMode(context, mode, holdButton);
  }
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
}

void __fastcall EventInputGetMousePosition(float *x, float *y) {
  HEVENTCONTEXT hContext;
  INSTANCELOCK  instanceLock;
  EvtContext   *context;

  ASSERT(x);
  ASSERT(y);

  hContext = reinterpret_cast<HEVENTCONTEXT>(PropGet(PROP_EVENTCONTEXT));
  context = EvtContext::GetTable().Lock(reinterpret_cast<DWORD>(hContext), 0, instanceLock, __FILE__, __LINE__);
  if (context) {
    IEvtInputGetMousePosition(x, y);
  }
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
}

void __fastcall EventInputSetMousePosition(float x, float y) {
  HEVENTCONTEXT hContext;
  INSTANCELOCK  instanceLock;
  EvtContext   *context;

  ASSERT(x != INFINITY);
  ASSERT(y != INFINITY);

  hContext = reinterpret_cast<HEVENTCONTEXT>(PropGet(PROP_EVENTCONTEXT));
  context = EvtContext::GetTable().Lock(reinterpret_cast<DWORD>(hContext), 0, instanceLock, __FILE__, __LINE__);
  if (context) {
    IEvtInputSetMousePosition(x, y);
  }
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
}

void __fastcall EventSetMouseBoundingRect(NTempest::CRect *rect) {
  INSTANCELOCK instanceLock;
  EvtContext  *context = EvtContext::GetTable().Lock(reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT)), 0, instanceLock, __FILE__, __LINE__);

  if (context) {
    IEvtInputSetMouseBoundingRect(rect);
  }
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
}
