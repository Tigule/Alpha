#include "EvtInt.h"

#include "Os/OsTime.h"

BOOL IEvtTimerDispatch(EvtContext *context) {
  EvtIdTable<EvtTimer *> *table;
  EvtTimerQueue          *queue;
  DWORD                   currTime;
  int                     dispatchedAny;

  VALIDATEBEGIN;
  VALIDATE(context);
  VALIDATEEND;

  context->TimerLockIdTableAndQueue(table, queue);
  currTime = OsGetAsyncTimeMs();
  dispatchedAny = 0;

  while (queue->Count()) {
    EvtTimer *timer = (*queue)[0];

    if (timer->handler || timer->guidHandler) {
      if ((LONG)(timer->targetTime.Get() - currTime) > 0) {
        break;
      }

      EVENT_DATA_TIMER data;
      data.elapsedSec = timer->timeout;
      data.currTime = currTime;

      if (timer->handler) {
        EVENTHANDLER handler = timer->handler;
        LPVOID       param = timer->param;

        queue->Dequeue();
        table->Free(timer->id);
        context->TimerUnlockIdTableAndQueue();
        handler(&data, param);
        context->TimerLockIdTableAndQueue(table, queue);
      } else {
        ASSERT(timer->guidHandler);

        DWORDLONG        param = timer->guidParam;
        LPVOID           param2 = timer->guidParam2;
        EVENTGUIDHANDLER handler = timer->guidHandler;

        queue->Dequeue();
        table->Free(timer->id);
        context->TimerUnlockIdTableAndQueue();
        handler(&data, param, param2);
        context->TimerLockIdTableAndQueue(table, queue);
      }

      dispatchedAny = 1;
    } else {
      queue->Dequeue();
      table->Free(timer->id);
    }
  }

  context->TimerUnlockIdTableAndQueue();
  return dispatchedAny;
}

UINT IEvtTimerGetNextTime(EvtContext *context, DWORD currTime) {
  EvtIdTable<EvtTimer *> *table;
  EvtTimerQueue          *queue;
  UINT                    nextTime;

  VALIDATEBEGIN;
  VALIDATE(context);
  VALIDATEEND;

  context->TimerLockIdTableAndQueue(table, queue);
  nextTime = INFINITE;

  if (queue->Count()) {
    LONG remaining = (*queue)[0]->targetTime.Get() - currTime;
    nextTime = max(0, remaining);
  }

  context->TimerUnlockIdTableAndQueue();
  return nextTime;
}

float IEvtTimerGetRemaining(EvtContext *context, UINT id) {
  EvtIdTable<EvtTimer *> *table;
  EvtTimerQueue          *queue;
  EvtTimer               *timer;
  float                   remaining;

  VALIDATEBEGIN;
  VALIDATE(context);
  VALIDATEEND;

  if (!id) {
    return 0.0f;
  }

  remaining = 0.0f;
  context->TimerLockIdTableAndQueue(table, queue);
  timer = (*table)[id];

  if (timer && (timer->handler || timer->guidHandler)) {
    DWORD currTime = OsGetAsyncTimeMs();
    int   remainingMs = timer->targetTime.Get() - currTime;

    if (remainingMs > 0) {
      remaining = remainingMs * 0.001f;
    }
  }

  context->TimerUnlockIdTableAndQueue();
  return remaining;
}

void IEvtTimerKill(EvtContext *context, UINT id, EVENTHANDLER handlerFunctionPtr, LPCSTR functionName) {
  EvtIdTable<EvtTimer *> *table;
  EvtTimerQueue          *queue;
  EvtTimer               *timer;

  VALIDATEBEGIN;
  VALIDATE(context);
  VALIDATEENDVOID;

  if (!id) {
    return;
  }

  context->TimerLockIdTableAndQueue(table, queue);
  timer = (*table)[id];

  if (timer && (timer->handler || timer->guidHandler)) {
    if ((timer->handler && timer->handler != handlerFunctionPtr) ||
        (timer->guidHandler && (LPVOID)timer->guidHandler != (LPVOID)handlerFunctionPtr))
    {
      FATALERROR(("Error, attempt to kill eventID %d with mismatching handler (%s)!", id, functionName ? functionName : ""));
    }

    timer->handler = 0;
    timer->guidHandler = 0;

    while (queue->Count()) {
      timer = (*queue)[0];

      if (timer->handler || timer->guidHandler) {
        break;
      }

      queue->Dequeue();
      table->Free(timer->id);
    }
  }

  context->TimerUnlockIdTableAndQueue();
}

UINT IEvtTimerSet(
    EvtContext      *context,
    float            timeout,
    EVENTHANDLER     handler,
    LPVOID           param,
    EVENTGUIDHANDLER guidHandler,
    DWORDLONG        guidParam,
    LPVOID           guidParam2
) {
  EvtIdTable<EvtTimer *> *table;
  EvtTimerQueue          *queue;
  EvtTimer               *timer;
  UINT                    id;

  VALIDATEBEGIN;
  VALIDATE(context);
  VALIDATEEND;

  if (!handler && !guidHandler) {
    return 0;
  }

  context->TimerLockIdTableAndQueue(table, queue);
  id = table->Alloc();
  timer = (*table)[id];

  if (!timer) {
    timer = NEW(EvtTimer);
    (*table)[id] = timer;
  }

  timer->targetTime.Set(OsGetAsyncTimeMs() - (long)(timeout * -1000.0f));
  timer->id = id;
  timer->timeout = timeout;
  timer->handler = handler;
  timer->param = param;
  timer->guidHandler = guidHandler;
  timer->guidParam = guidParam;
  timer->guidParam2 = guidParam2;
  queue->Enqueue(timer);
  context->TimerUnlockIdTableAndQueue();
  return id;
}

UINT IEvtTimerSet(
    EvtContext      *context,
    UINT             timeout,
    EVENTHANDLER     handler,
    LPVOID           param,
    EVENTGUIDHANDLER guidHandler,
    DWORDLONG        guidParam,
    LPVOID           guidParam2
) {
  EvtIdTable<EvtTimer *> *table;
  EvtTimerQueue          *queue;
  EvtTimer               *timer;
  UINT                    id;

  VALIDATEBEGIN;
  VALIDATE(context);
  VALIDATEEND;

  if (!handler && !guidHandler) {
    return 0;
  }

  context->TimerLockIdTableAndQueue(table, queue);
  id = table->Alloc();
  timer = (*table)[id];

  if (!timer) {
    timer = NEW(EvtTimer);
    (*table)[id] = timer;
  }

  timer->targetTime.Set(OsGetAsyncTimeMs() + timeout);
  timer->id = id;
  timer->timeout = timeout * 0.001f;
  timer->handler = handler;
  timer->param = param;
  timer->guidHandler = guidHandler;
  timer->guidParam = guidParam;
  timer->guidParam2 = guidParam2;
  queue->Enqueue(timer);
  context->TimerUnlockIdTableAndQueue();
  return id;
}

UINT IEvtTimerSetAbsolute(
    EvtContext      *context,
    DWORD            triggerTime,
    EVENTHANDLER     handler,
    LPVOID           param,
    EVENTGUIDHANDLER guidHandler,
    DWORDLONG        guidParam,
    LPVOID           guidParam2
) {
  EvtIdTable<EvtTimer *> *table;
  EvtTimerQueue          *queue;
  EvtTimer               *timer;
  UINT                    id;

  VALIDATEBEGIN;
  VALIDATE(context);
  VALIDATEEND;

  if (!handler && !guidHandler) {
    return 0;
  }

  context->TimerLockIdTableAndQueue(table, queue);
  id = table->Alloc();
  timer = (*table)[id];

  if (!timer) {
    timer = NEW(EvtTimer);
    (*table)[id] = timer;
  }

  DWORD currTime = OsGetAsyncTimeMs();

  timer->targetTime.Set(triggerTime);
  timer->id = id;
  timer->timeout = (triggerTime - currTime) * 0.001f;
  timer->handler = handler;
  timer->param = param;
  timer->guidHandler = guidHandler;
  timer->guidParam = guidParam;
  timer->guidParam2 = guidParam2;
  queue->Enqueue(timer);
  context->TimerUnlockIdTableAndQueue();
  return id;
}
