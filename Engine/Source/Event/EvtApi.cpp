#include "EvtInt.h"

void ObserverInitialize();
void ObserverDestroy();
void InputObserverInitialize();
void InputObserverDestroy();
BOOL IEvtQueueCheckSyncKeyState(EvtContext *context, KEY key);
BOOL IEvtQueueCheckSyncMouseState(EvtContext *context, MOUSEBUTTON button);
void IEvtQueueScan(EvtContext *context, EVENTSCANHANDLER scanner, LPVOID param);
void IEvtInputSetConfirmCloseCallback(EVENTCONFIRMCLOSEHANDLER callback, LPVOID param);

void EventInitialize(UINT threadCount, int netServer) {
  ObserverInitialize();
  InputObserverInitialize();
  IEvtQueueInitialize();
  IEvtInputInitialize();

  if (threadCount < 1) {
    threadCount = 1;
  }

  IEvtSchedulerInitialize(threadCount, netServer);
}

void EventDestroy() {
  IEvtSchedulerDestroy();
  IEvtInputDestroy();
  IEvtQueueDestroy();
  InputObserverDestroy();
  ObserverDestroy();
}

void EventDoMessageLoop() {
  IEvtSchedulerProcess();
}

void EventInitiateShutdown() {
  IEvtSchedulerShutdown();
}

HEVENTCONTEXT
EventCreateContextEx(int interactive, EVENTHANDLER initializeHandler, EVENTHANDLER destroyHandler, DWORD idleTime, DWORD debugFlags) {
  return IEvtSchedulerCreateContext(interactive, initializeHandler, destroyHandler, idleTime, debugFlags);
}

void EventCreateContext(int interactive, EVENTHANDLER initializeHandler, EVENTHANDLER destroyHandler) {
  EventCreateContextEx(interactive, initializeHandler, destroyHandler, interactive ? 1 : 500, 0);
}

int EventIsContextInteractive() {
  INSTANCELOCK instanceLock;
  DWORD        id = reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT));
  EvtContext  *context = EvtContext::GetTable().Lock(id, 0, instanceLock, __FILE__, __LINE__);
  int          interactive = context ? IEvtSchedulerIsContextInteractive(context) : 0;
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  return interactive;
}

HEVENTCONTEXT EventGetCurrentContext() {
  return reinterpret_cast<HEVENTCONTEXT>(PropGet(PROP_EVENTCONTEXT));
}

void EventSetContextIdleTime(DWORD idleTime, HEVENTCONTEXT hContext) {
  INSTANCELOCK instanceLock;
  EvtContext  *context = EvtContext::GetTable().Lock(
      hContext ? reinterpret_cast<DWORD>(hContext) : reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT)), 0, instanceLock, __FILE__, __LINE__
  );
  if (!context) {
    return;
  }

  context->SchedSetIdleTime(idleTime);
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
}

DWORD EventGetContextIdleTime(HEVENTCONTEXT hContext) {
  INSTANCELOCK instanceLock;
  EvtContext  *context = EvtContext::GetTable().Lock(
      hContext ? reinterpret_cast<DWORD>(hContext) : reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT)), 0, instanceLock, __FILE__, __LINE__
  );
  DWORD idleTime;

  if (!context) {
    return 0;
  }

  idleTime = context->SchedGetIdleTime();
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);

  return idleTime;
}

int EventIsButtonDown(MOUSEBUTTON button) {
  INSTANCELOCK instanceLock;
  DWORD        id = reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT));
  EvtContext  *context = EvtContext::GetTable().Lock(id, 0, instanceLock, __FILE__, __LINE__);
  int          down = context ? IEvtQueueCheckSyncMouseState(context, button) : 0;
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  return down;
}

int EventIsKeyDown(KEY key) {
  INSTANCELOCK instanceLock;
  DWORD        id = reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT));
  EvtContext  *context = EvtContext::GetTable().Lock(id, 0, instanceLock, __FILE__, __LINE__);
  int          down = context ? IEvtQueueCheckSyncKeyState(context, key) : 0;
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  return down;
}

void EventPostClose() {
  EventPostCloseEx(0);
}

void EventPostCloseEx(HEVENTCONTEXT hContext) {
  INSTANCELOCK instanceLock;
  EvtContext  *context = EvtContext::GetTable().Lock(
      hContext ? reinterpret_cast<DWORD>(hContext) : reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT)), 0, instanceLock, __FILE__, __LINE__
  );
  if (context) {
    context->SchedSetClosed();
  }
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
}

BOOL EventQueuePost(HEVENTCONTEXT hContext, EVENTID id, LPCVOID data, UINT bytes) {
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

BOOL EventQueueScan(EVENTSCANHANDLER scanner, LPVOID param) {
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

void EventRegister(EVENTID id, EVENTHANDLER handler) {
  EventRegisterEx(id, handler, 0, EVENT_PRIORITY_NORMAL);
}

void EventRegisterEx(EVENTID id, EVENTHANDLER handler, LPVOID param, float priority) {
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

void EventUnregister(EVENTID id, EVENTHANDLER handler) {
  EventUnregisterEx(id, handler, 0, 0xFFFFFFFF);
}

void EventUnregisterEx(EVENTID id, EVENTHANDLER handler, LPVOID param, UINT flags) {
  INSTANCELOCK instanceLock;
  DWORD        contextId = reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT));
  EvtContext  *context = EvtContext::GetTable().Lock(contextId, 0, instanceLock, __FILE__, __LINE__);
  if (context) {
    IEvtQueueUnregister(context, id, handler, param, flags);
  }
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
}

void EventSetConfirmCloseCallback(EVENTCONFIRMCLOSEHANDLER inFunc, LPVOID inParam) {
  IEvtInputSetConfirmCloseCallback(inFunc, inParam);
}

int EventInputProcess(HEVENTCONTEXT hContext) {
  INSTANCELOCK instanceLock;
  EvtContext  *context = EvtContext::GetTable().Lock(
      hContext ? reinterpret_cast<DWORD>(hContext) : reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT)), 0, instanceLock, __FILE__, __LINE__
  );
  if (!context) {
    return 0;
  }

  int shutdown = 0;
  int result = IEvtInputProcess(context, &shutdown);
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  return result;
}

UINT EventSetTimer(float timeout, EVENTHANDLER handler, LPVOID param) {
  INSTANCELOCK instanceLock;
  DWORD        contextId = reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT));
  EvtContext  *context = EvtContext::GetTable().Lock(contextId, 0, instanceLock, __FILE__, __LINE__);
  UINT         timerId = context ? IEvtTimerSet(context, timeout, handler, param, 0, 0, 0) : 0;
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  return timerId;
}

UINT EventSetTimer(float timeout, EVENTGUIDHANDLER handler, DWORDLONG param, LPVOID param2) {
  INSTANCELOCK instanceLock;
  DWORD        contextId = reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT));
  EvtContext  *context = EvtContext::GetTable().Lock(contextId, 0, instanceLock, __FILE__, __LINE__);
  UINT         timerId = context ? IEvtTimerSet(context, timeout, 0, 0, handler, param, param2) : 0;
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  return timerId;
}

UINT EventSetTimer(UINT timeout, EVENTHANDLER handler, LPVOID param) {
  INSTANCELOCK instanceLock;
  DWORD        contextId = reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT));
  EvtContext  *context = EvtContext::GetTable().Lock(contextId, 0, instanceLock, __FILE__, __LINE__);
  UINT         timerId = context ? IEvtTimerSet(context, timeout, handler, param, 0, 0, 0) : 0;
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  return timerId;
}

UINT EventSetTimer(UINT timeout, EVENTGUIDHANDLER handler, DWORDLONG param, LPVOID param2) {
  INSTANCELOCK instanceLock;
  DWORD        contextId = reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT));
  EvtContext  *context = EvtContext::GetTable().Lock(contextId, 0, instanceLock, __FILE__, __LINE__);
  UINT         timerId = context ? IEvtTimerSet(context, timeout, 0, 0, handler, param, param2) : 0;
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  return timerId;
}

UINT EventSetTimerAbsolute(DWORD triggerTime, EVENTHANDLER handler, LPVOID param) {
  INSTANCELOCK instanceLock;
  DWORD        contextId = reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT));
  EvtContext  *context = EvtContext::GetTable().Lock(contextId, 0, instanceLock, __FILE__, __LINE__);
  UINT         timerId = context ? IEvtTimerSetAbsolute(context, triggerTime, handler, param, 0, 0, 0) : 0;
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  return timerId;
}

UINT EventSetTimerAbsolute(DWORD triggerTime, EVENTGUIDHANDLER handler, DWORDLONG param, LPVOID param2) {
  INSTANCELOCK instanceLock;
  DWORD        contextId = reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT));
  EvtContext  *context = EvtContext::GetTable().Lock(contextId, 0, instanceLock, __FILE__, __LINE__);
  UINT         timerId = context ? IEvtTimerSetAbsolute(context, triggerTime, 0, 0, handler, param, param2) : 0;
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  return timerId;
}

void EventKillTimer(UINT timerId, EVENTHANDLER handlerFunction, LPCSTR functionName) {
  INSTANCELOCK instanceLock;
  DWORD        contextId = reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT));
  EvtContext  *context = EvtContext::GetTable().Lock(contextId, 0, instanceLock, __FILE__, __LINE__);
  if (context) {
    IEvtTimerKill(context, timerId, handlerFunction, functionName);
  }
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
}

float EventGetRemainingTime(UINT timerId) {
  INSTANCELOCK instanceLock;
  DWORD        contextId = reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT));
  EvtContext  *context = EvtContext::GetTable().Lock(contextId, 0, instanceLock, __FILE__, __LINE__);
  float        result = context ? IEvtTimerGetRemaining(context, timerId) : 0.0f;
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  return result;
}

void EventSetMouseMode(MOUSEMODE mode, UINT holdButton) {
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

void EventInputGetMousePosition(float *x, float *y) {
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

void EventInputSetMousePosition(float x, float y) {
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

void EventSetMouseBoundingRect(NTempest::CRect *rect) {
  INSTANCELOCK instanceLock;
  EvtContext  *context = EvtContext::GetTable().Lock(reinterpret_cast<DWORD>(PropGet(PROP_EVENTCONTEXT)), 0, instanceLock, __FILE__, __LINE__);

  if (context) {
    IEvtInputSetMouseBoundingRect(rect);
  }
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
}
