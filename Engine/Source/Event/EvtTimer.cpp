#include "EvtInt.h"

#include "Os/OsTime.h"

int IEvtTimerDispatch(EvtContext *context) {
  EvtIdTable<EvtTimer *> *table;
  EvtTimerQueue          *queue;
  DWORD                   currTime;
  int                     dispatched = 0;

  FATALASSERT(context);

  context->TimerLockIdTableAndQueue(table, queue);
  currTime = OsGetAsyncTimeMs();

  while (queue->Count()) {
    EvtTimer *timer = (*queue)[0];

    if (!timer->handler && !timer->guidHandler) {
      queue->Dequeue();
      table->Free(timer->id);
      continue;
    }

    if (static_cast<LONG>(timer->targetTime.Get() - currTime) > 0) {
      break;
    }

    {
      EVENT_DATA_TIMER data;
      EVENTHANDLER     handler = timer->handler;
      EVENTGUIDHANDLER guidHandler = 0;
      void            *param = 0;
      unsigned __int64 guidParam = 0;
      void            *guidParam2 = 0;

      if (handler) {
        param = timer->param;
      } else {
        ASSERT(timer->guidHandler);
        guidHandler = timer->guidHandler;
        guidParam = timer->guidParam;
        guidParam2 = timer->guidParam2;
      }

      data.elapsedSec = timer->timeout;
      data.currTime = currTime;
      queue->Remove(0);
      table->Free(timer->id);

      context->TimerUnlockIdTableAndQueue();
      if (handler) {
        handler(&data, param);
      } else {
        guidHandler(&data, guidParam, guidParam2);
      }
      context->TimerLockIdTableAndQueue(table, queue);
      dispatched = 1;
    }
  }

  context->TimerUnlockIdTableAndQueue();
  return dispatched;
}

unsigned int IEvtTimerGetNextTime(EvtContext *context, DWORD currTime) {
  EvtIdTable<EvtTimer *> *table;
  EvtTimerQueue          *queue;
  unsigned int            result = INFINITE;

  FATALASSERT(context);

  context->TimerLockIdTableAndQueue(table, queue);
  if (queue->Count()) {
    LONG remaining = static_cast<LONG>((*queue)[0]->targetTime.Get() - currTime);
    result = remaining < 0 ? 0 : static_cast<DWORD>(remaining);
  }
  context->TimerUnlockIdTableAndQueue();
  return result;
}

float IEvtTimerGetRemaining(EvtContext *context, unsigned int id) {
  EvtIdTable<EvtTimer *> *table;
  EvtTimerQueue          *queue;
  EvtTimer               *timer;
  float                   remaining = 0.0f;

  FATALASSERT(context);
  if (!id) {
    return 0.0f;
  }

  context->TimerLockIdTableAndQueue(table, queue);
  timer = (*table)[id];
  if (timer && (timer->handler || timer->guidHandler)) {
    LONG remainingMs = static_cast<LONG>(timer->targetTime.Get() - OsGetAsyncTimeMs());
    if (remainingMs > 0) {
      remaining = static_cast<float>(remainingMs * 0.001);
    }
  }
  context->TimerUnlockIdTableAndQueue();
  return remaining;
}

void IEvtTimerKill(EvtContext *context, unsigned int id, EVENTHANDLER handlerFunction, const char *functionName) {
  EvtIdTable<EvtTimer *> *table;
  EvtTimerQueue          *queue;
  EvtTimer               *timer;

  FATALASSERT(context);
  if (!id) {
    return;
  }

  context->TimerLockIdTableAndQueue(table, queue);
  timer = (*table)[id];
  if (!timer || (!timer->handler && !timer->guidHandler)) {
    context->TimerUnlockIdTableAndQueue();
    return;
  }

  if ((timer->handler && timer->handler != handlerFunction) ||
      (timer->guidHandler && reinterpret_cast<void *>(timer->guidHandler) != reinterpret_cast<void *>(handlerFunction)))
  {
    FATALERROR(("Error, attempt to kill eventID %d with mismatching handler (%s)!", id, functionName ? functionName : ""));
  }

  timer->handler = 0;
  timer->guidHandler = 0;
  while (queue->Count()) {
    EvtTimer *head = (*queue)[0];
    if (head->handler || head->guidHandler) {
      break;
    }
    queue->Remove(0);
    table->Free(head->id);
  }

  context->TimerUnlockIdTableAndQueue();
}

unsigned int IEvtTimerSet(
    EvtContext      *context,
    float            timeout,
    EVENTHANDLER     handler,
    void            *param,
    EVENTGUIDHANDLER guidHandler,
    unsigned __int64 guidParam,
    void            *guidParam2
) {
  LONG                    timeoutMs;
  DWORD                   targetTime;
  EvtIdTable<EvtTimer *> *table;
  EvtTimerQueue          *queue;
  EvtTimer               *timer;
  unsigned int            id;

  FATALASSERT(context);
  if (!handler && !guidHandler) {
    return 0;
  }

  timeoutMs = static_cast<LONG>(timeout * 1000.0f);
  targetTime = OsGetAsyncTimeMs() + timeoutMs;
  context->TimerLockIdTableAndQueue(table, queue);
  id = table->Alloc();
  timer = (*table)[id];
  if (!timer) {
    timer = NEW(EvtTimer);
    (*table)[id] = timer;
  }
  timer->targetTime.Set(targetTime);
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

unsigned int IEvtTimerSet(
    EvtContext      *context,
    unsigned int     timeout,
    EVENTHANDLER     handler,
    void            *param,
    EVENTGUIDHANDLER guidHandler,
    unsigned __int64 guidParam,
    void            *guidParam2
) {
  EvtIdTable<EvtTimer *> *table;
  EvtTimerQueue          *queue;
  EvtTimer               *timer;
  unsigned int            id;
  DWORD                   targetTime;

  FATALASSERT(context);
  if (!handler && !guidHandler) {
    return 0;
  }

  targetTime = OsGetAsyncTimeMs() + timeout;
  context->TimerLockIdTableAndQueue(table, queue);
  id = table->Alloc();
  timer = (*table)[id];
  if (!timer) {
    timer = NEW(EvtTimer);
    (*table)[id] = timer;
  }
  timer->targetTime.Set(targetTime);
  timer->id = id;
  timer->timeout = static_cast<float>(timeout * 0.001);
  timer->handler = handler;
  timer->param = param;
  timer->guidHandler = guidHandler;
  timer->guidParam = guidParam;
  timer->guidParam2 = guidParam2;
  queue->Enqueue(timer);
  context->TimerUnlockIdTableAndQueue();
  return id;
}

unsigned int IEvtTimerSetAbsolute(
    EvtContext      *context,
    DWORD            triggerTime,
    EVENTHANDLER     handler,
    void            *param,
    EVENTGUIDHANDLER guidHandler,
    unsigned __int64 guidParam,
    void            *guidParam2
) {
  DWORD                   currTime;
  float                   timeout;
  EvtIdTable<EvtTimer *> *table;
  EvtTimerQueue          *queue;
  EvtTimer               *timer;
  unsigned int            id;

  FATALASSERT(context);
  if (!handler && !guidHandler) {
    return 0;
  }

  currTime = OsGetAsyncTimeMs();
  timeout = static_cast<float>(static_cast<double>(triggerTime - currTime) * 0.001);
  context->TimerLockIdTableAndQueue(table, queue);
  id = table->Alloc();
  timer = (*table)[id];
  if (!timer) {
    timer = NEW(EvtTimer);
    (*table)[id] = timer;
  }
  timer->targetTime.Set(triggerTime);
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
