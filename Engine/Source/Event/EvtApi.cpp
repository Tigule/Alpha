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
  int          interactive = 0;
  DWORD        id = (DWORD)PropGet(PROP_EVENTCONTEXT);
  INSTANCELOCK instanceLock;
  EvtContext  *context = EvtContext::GetTable().Lock(id, 0, instanceLock, __FILE__, __LINE__);
  if (context) {
    interactive = IEvtSchedulerIsContextInteractive(context);
    EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  }
  return interactive;
}

HEVENTCONTEXT EventGetCurrentContext() {
  return (HEVENTCONTEXT)PropGet(PROP_EVENTCONTEXT);
}

void EventSetContextIdleTime(DWORD idleTime, HEVENTCONTEXT hContext) {
  if (!hContext) {
    hContext = (HEVENTCONTEXT)PropGet(PROP_EVENTCONTEXT);
  }
  INSTANCELOCK instanceLock;
  EvtContext  *context = EvtContext::GetTable().Lock((DWORD)hContext, 0, instanceLock, __FILE__, __LINE__);
  if (context) {
    context->SchedSetIdleTime(idleTime);
    EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  }
}

DWORD EventGetContextIdleTime(HEVENTCONTEXT hContext) {
  if (!hContext) {
    hContext = (HEVENTCONTEXT)PropGet(PROP_EVENTCONTEXT);
  }
  DWORD idleTime = 0;
  INSTANCELOCK instanceLock;
  EvtContext  *context = EvtContext::GetTable().Lock((DWORD)hContext, 0, instanceLock, __FILE__, __LINE__);
  if (context) {
    idleTime = context->SchedGetIdleTime();
    EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  }
  return idleTime;
}

int EventIsButtonDown(MOUSEBUTTON button) {
  int          down = 0;
  DWORD        id = (DWORD)PropGet(PROP_EVENTCONTEXT);
  INSTANCELOCK instanceLock;
  EvtContext  *context = EvtContext::GetTable().Lock(id, 0, instanceLock, __FILE__, __LINE__);
  if (context) {
    down = IEvtQueueCheckSyncMouseState(context, button);
    EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  }
  return down;
}

int EventIsKeyDown(KEY key) {
  int          down = 0;
  DWORD        id = (DWORD)PropGet(PROP_EVENTCONTEXT);
  INSTANCELOCK instanceLock;
  EvtContext  *context = EvtContext::GetTable().Lock(id, 0, instanceLock, __FILE__, __LINE__);
  if (context) {
    down = IEvtQueueCheckSyncKeyState(context, key);
    EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  }
  return down;
}

void EventPostClose() {
  EventPostCloseEx(0);
}

void EventPostCloseEx(HEVENTCONTEXT hContext) {
  if (!hContext) {
    hContext = (HEVENTCONTEXT)PropGet(PROP_EVENTCONTEXT);
  }
  INSTANCELOCK instanceLock;
  EvtContext  *context = EvtContext::GetTable().Lock((DWORD)hContext, 0, instanceLock, __FILE__, __LINE__);
  if (context) {
    context->SchedSetClosed();
    EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  }
}

BOOL EventQueuePost(HEVENTCONTEXT hContext, EVENTID id, LPCVOID data, UINT bytes) {
  int result = 0;
  if (!hContext) {
    hContext = (HEVENTCONTEXT)PropGet(PROP_EVENTCONTEXT);
  }
  INSTANCELOCK instanceLock;
  EvtContext  *context = EvtContext::GetTable().Lock((DWORD)hContext, 0, instanceLock, __FILE__, __LINE__);
  if (context) {
    if (!context->SchedGetDestroyed()) {
      IEvtQueuePost(context, id, data, bytes);
      result = 1;
    }
    EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  }
  return result;
}

BOOL EventQueueScan(EVENTSCANHANDLER scanner, LPVOID param) {
  int          result = 0;
  DWORD        id = (DWORD)PropGet(PROP_EVENTCONTEXT);
  INSTANCELOCK instanceLock;
  EvtContext  *context = EvtContext::GetTable().Lock(id, 0, instanceLock, __FILE__, __LINE__);
  if (context) {
    if (!context->SchedGetDestroyed()) {
      IEvtQueueScan(context, scanner, param);
      result = 1;
    }
    EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  }
  return result;
}

void EventRegister(EVENTID id, EVENTHANDLER handler) {
  EventRegisterEx(id, handler, 0, EVENT_PRIORITY_NORMAL);
}

void EventRegisterEx(EVENTID id, EVENTHANDLER handler, LPVOID param, float priority) {
  HEVENTCONTEXT hContext;
  EvtContext   *context;

  VALIDATEBEGIN;
  VALIDATE(id >= 0);
  VALIDATE(id < EVENTIDS);
  VALIDATE(handler);
  VALIDATEENDVOID;

  INSTANCELOCK instanceLock;
  hContext = (HEVENTCONTEXT)PropGet(PROP_EVENTCONTEXT);
  context = EvtContext::GetTable().Lock((DWORD)hContext, 0, instanceLock, __FILE__, __LINE__);
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
  DWORD        contextId = (DWORD)PropGet(PROP_EVENTCONTEXT);
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
  int result = 0;
  if (!hContext) {
    hContext = (HEVENTCONTEXT)PropGet(PROP_EVENTCONTEXT);
  }
  INSTANCELOCK instanceLock;
  EvtContext  *context = EvtContext::GetTable().Lock((DWORD)hContext, 0, instanceLock, __FILE__, __LINE__);
  if (context) {
    int shutdown;
    result = IEvtInputProcess(context, &shutdown);
    EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  }
  return result;
}

UINT EventSetTimer(float timeout, EVENTHANDLER handler, LPVOID param) {
  UINT         timerId = 0;
  DWORD        contextId = (DWORD)PropGet(PROP_EVENTCONTEXT);
  INSTANCELOCK instanceLock;
  EvtContext  *context = EvtContext::GetTable().Lock(contextId, 0, instanceLock, __FILE__, __LINE__);
  if (context) {
    timerId = IEvtTimerSet(context, timeout, handler, param, 0, 0, 0);
    EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  }
  return timerId;
}

UINT EventSetTimer(float timeout, EVENTGUIDHANDLER handler, DWORDLONG param, LPVOID param2) {
  UINT         timerId = 0;
  DWORD        contextId = (DWORD)PropGet(PROP_EVENTCONTEXT);
  INSTANCELOCK instanceLock;
  EvtContext  *context = EvtContext::GetTable().Lock(contextId, 0, instanceLock, __FILE__, __LINE__);
  if (context) {
    timerId = IEvtTimerSet(context, timeout, 0, 0, handler, param, param2);
    EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  }
  return timerId;
}

UINT EventSetTimer(UINT timeout, EVENTHANDLER handler, LPVOID param) {
  UINT         timerId = 0;
  DWORD        contextId = (DWORD)PropGet(PROP_EVENTCONTEXT);
  INSTANCELOCK instanceLock;
  EvtContext  *context = EvtContext::GetTable().Lock(contextId, 0, instanceLock, __FILE__, __LINE__);
  if (context) {
    timerId = IEvtTimerSet(context, timeout, handler, param, 0, 0, 0);
    EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  }
  return timerId;
}

UINT EventSetTimer(UINT timeout, EVENTGUIDHANDLER handler, DWORDLONG param, LPVOID param2) {
  UINT         timerId = 0;
  DWORD        contextId = (DWORD)PropGet(PROP_EVENTCONTEXT);
  INSTANCELOCK instanceLock;
  EvtContext  *context = EvtContext::GetTable().Lock(contextId, 0, instanceLock, __FILE__, __LINE__);
  if (context) {
    timerId = IEvtTimerSet(context, timeout, 0, 0, handler, param, param2);
    EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  }
  return timerId;
}

UINT EventSetTimerAbsolute(DWORD triggerTime, EVENTHANDLER handler, LPVOID param) {
  UINT         timerId = 0;
  DWORD        contextId = (DWORD)PropGet(PROP_EVENTCONTEXT);
  INSTANCELOCK instanceLock;
  EvtContext  *context = EvtContext::GetTable().Lock(contextId, 0, instanceLock, __FILE__, __LINE__);
  if (context) {
    timerId = IEvtTimerSetAbsolute(context, triggerTime, handler, param, 0, 0, 0);
    EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  }
  return timerId;
}

UINT EventSetTimerAbsolute(DWORD triggerTime, EVENTGUIDHANDLER handler, DWORDLONG param, LPVOID param2) {
  UINT         timerId = 0;
  DWORD        contextId = (DWORD)PropGet(PROP_EVENTCONTEXT);
  INSTANCELOCK instanceLock;
  EvtContext  *context = EvtContext::GetTable().Lock(contextId, 0, instanceLock, __FILE__, __LINE__);
  if (context) {
    timerId = IEvtTimerSetAbsolute(context, triggerTime, 0, 0, handler, param, param2);
    EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  }
  return timerId;
}

void EventKillTimer(UINT timerId, EVENTHANDLER handlerFunction, LPCSTR functionName) {
  INSTANCELOCK instanceLock;
  DWORD        contextId = (DWORD)PropGet(PROP_EVENTCONTEXT);
  EvtContext  *context = EvtContext::GetTable().Lock(contextId, 0, instanceLock, __FILE__, __LINE__);
  if (context) {
    IEvtTimerKill(context, timerId, handlerFunction, functionName);
  }
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
}

float EventGetRemainingTime(UINT timerId) {
  DWORD        contextId = (DWORD)PropGet(PROP_EVENTCONTEXT);
  float        result = 0.0f;
  INSTANCELOCK instanceLock;
  EvtContext  *context = EvtContext::GetTable().Lock(contextId, 0, instanceLock, __FILE__, __LINE__);
  if (context) {
    result = IEvtTimerGetRemaining(context, timerId);
    EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
  }
  return result;
}

void EventSetMouseMode(MOUSEMODE mode, UINT holdButton) {
  INSTANCELOCK  instanceLock;
  HEVENTCONTEXT hContext;
  EvtContext   *context;

  VALIDATEBEGIN;
  VALIDATE(mode < MOUSE_MODES);
  VALIDATEENDVOID;

  hContext = (HEVENTCONTEXT)PropGet(PROP_EVENTCONTEXT);
  context = EvtContext::GetTable().Lock((DWORD)hContext, 0, instanceLock, __FILE__, __LINE__);
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

  hContext = (HEVENTCONTEXT)PropGet(PROP_EVENTCONTEXT);
  context = EvtContext::GetTable().Lock((DWORD)hContext, 0, instanceLock, __FILE__, __LINE__);
  if (context) {
    IEvtInputGetMousePosition(x, y);
  }
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
}

void EventInputSetMousePosition(float x, float y) {
  HEVENTCONTEXT hContext;
  INSTANCELOCK  instanceLock;
  EvtContext   *context;

  ASSERT(x);
  ASSERT(y);

  hContext = (HEVENTCONTEXT)PropGet(PROP_EVENTCONTEXT);
  context = EvtContext::GetTable().Lock((DWORD)hContext, 0, instanceLock, __FILE__, __LINE__);
  if (context) {
    IEvtInputSetMousePosition(x, y);
  }
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
}

void EventSetMouseBoundingRect(NTempest::CRect *rect) {
  INSTANCELOCK instanceLock;
  EvtContext  *context = EvtContext::GetTable().Lock((DWORD)PropGet(PROP_EVENTCONTEXT), 0, instanceLock, __FILE__, __LINE__);

  if (context) {
    IEvtInputSetMouseBoundingRect(rect);
  }
  EvtContext::GetTable().Unlock(instanceLock, __FILE__, __LINE__);
}
