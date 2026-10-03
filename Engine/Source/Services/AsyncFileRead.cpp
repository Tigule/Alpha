#include "Base/Base.h"
#include "Gx/Gx.h"
#include "BLPFile/blp.h"

#include "AsyncFileRead.h"

#include "Event/EvtApi.h"
#include "Os/OsTime.h"
#include "Os/W32/Debugging.h"

#define OSWAIT_OBJECT_0 WAIT_OBJECT_0
#define OSWAIT_TIMEOUT  WAIT_TIMEOUT

BOOL          AsyncFileReadPollHandler(LPCVOID, LPVOID);
static UINT APIENTRY AsyncFileReadThread(LPVOID param);

static UINT s_waiting;
static LISTDECLEX(CAsyncObject, link, s_asyncFileReadList);
static LISTDECLEX(CAsyncObject, link, s_asyncFileReadFreeList);
static LISTDECLEX(CAsyncObject, link, s_asyncFileReadPostList);
static CAsyncObject                   *s_asyncCurrentObject;
static HPROPCONTEXT                    s_propContext;
static SCritSect                       s_queueLock;
static SThread                         s_asyncReadThread;
static SEvent                          s_shutdownEvent(1, 0);
static SEvent                          s_queueEvent(0, 0);
static CAsyncObject volatile          *s_asyncWaitObject;
static TSGrowableArray<void (*)(void)> s_handlers;

void AsyncFileReadInitialize() {
  EventRegisterEx(EVENT_ID_POLL, AsyncFileReadPollHandler, 0, EVENT_PRIORITY_NORMAL);

  s_asyncCurrentObject = 0;
  s_asyncWaitObject = 0;
  s_propContext = PropGetSelectedContext();
  s_shutdownEvent.Reset();
  SThread::Create(AsyncFileReadThread, 0, s_asyncReadThread, const_cast<char *>("AsyncFileLoader"));
}

static UINT APIENTRY AsyncFileReadThread(LPVOID param) {
  DWORD waitResult;

  PropSelectContext(s_propContext);

  while (s_shutdownEvent.Wait(0) != OSWAIT_OBJECT_0) {
    waitResult = s_queueEvent.Wait(500);
    if (waitResult != OSWAIT_TIMEOUT) {
      ASSERT(waitResult == OSWAIT_OBJECT_0);

      for (;;) {
        CAsyncObject *object;

        s_queueLock.Enter();
        object = s_asyncFileReadList.Head();
        if (!object) {
          s_queueLock.Leave();
          break;
        }

        object->link.Unlink();
        ASSERT(!s_asyncCurrentObject);
        s_asyncCurrentObject = object;
        s_queueLock.Leave();

        ASSERT(!object->isLoaded);
        ASSERT(object->file);
        ASSERT(object->buffer);
        ASSERT(object->size);
        ASSERT(object->userPostloadCallback);

        if (object->critSect) {
          object->critSect->Enter();
        }

        SFile::SetFilePointer(object->file, object->offset, 0, FILE_BEGIN);
        SFile::Read(object->file, object->buffer, object->size, 0, 0, 0);

        if (object->critSect) {
          object->critSect->Leave();
        }

        s_queueLock.Enter();
        s_asyncFileReadPostList.LinkNode(object, LIST_TAIL, 0);
        s_asyncCurrentObject = 0;
        s_queueLock.Leave();

        OsSleep(1);
      }
    }
  }

  s_queueEvent.Set();
  return 0;
}

void AsyncFileReadDestroy() {
  s_shutdownEvent.Set();
  s_queueEvent.Set();
  s_asyncReadThread.Wait(INFINITE);

  while (s_asyncFileReadFreeList.Head()) {
    s_asyncFileReadFreeList.DeleteNode(s_asyncFileReadFreeList.Head());
  }
  ASSERT(s_asyncFileReadList.Head() == 0);

  while (s_asyncFileReadPostList.Head()) {
    s_asyncFileReadPostList.DeleteNode(s_asyncFileReadPostList.Head());
  }
  ASSERT(s_asyncCurrentObject == 0);

  s_handlers.SetCount(0);
  EventUnregisterEx(EVENT_ID_POLL, AsyncFileReadPollHandler, 0, 0xFFFFFFFF);
}

void AsyncFileReadAddHandler(void (*handler)()) {
  UINT index;

  for (index = 0; index < s_handlers.Count(); ++index) {
    if (s_handlers[index] == handler) {
      return;
    }
  }

  s_handlers.Add(&handler);
}

CAsyncObject *AsyncFileReadCreateObject() {
  CAsyncObject *object;

  s_queueLock.Enter();
  object = s_asyncFileReadFreeList.Head();
  if (!object) {
    object = s_asyncFileReadFreeList.NewNode(LIST_HEAD, 0, 0);
  }
  object->link.Unlink();
  s_queueLock.Leave();

  object->file = 0;
  object->offset = 0;
  object->buffer = 0;
  object->size = 0;
  object->userArg = 0;
  object->userPostloadCallback = 0;
  object->critSect = 0;
  object->isLoaded = 0;
  object->canReorder = 1;

  return object;
}

void AsyncFileReadDestroyObject(CAsyncObject *object) {
  ASSERT(object);

  s_queueLock.Enter();
  if (object == s_asyncCurrentObject) {
    s_queueLock.Leave();
    while (object == s_asyncCurrentObject) {
      OsSleep(1);
    }
    s_queueLock.Enter();
  }

  object->link.Unlink();
  s_asyncFileReadFreeList.LinkNode(object, LIST_HEAD, 0);
  s_queueLock.Leave();
}

void AsyncFileReadObject(CAsyncObject *object) {
  DWORD location;

  ASSERT(object);
  ASSERT(!object->isLoaded);
  ASSERT(object->file);
  ASSERT(object->buffer);
  ASSERT(object->size);
  ASSERT(object->userPostloadCallback);

  s_queueLock.Enter();
  location = object == s_asyncWaitObject && object->canReorder ? LIST_HEAD : LIST_TAIL;
  s_asyncFileReadList.LinkNode(object, location, 0);
  s_queueLock.Leave();

  s_queueEvent.Set();
}

void AsyncFileReadWait(CAsyncObject *object) {
  ASSERT(object);
  ASSERT(!s_waiting);

  ++s_waiting;
  s_queueLock.Enter();

  if (object->isLoaded) {
    s_queueLock.Leave();
    return;
  }

  ASSERT(s_asyncWaitObject == 0);
  s_asyncWaitObject = object;

  if (s_asyncCurrentObject != object && object->canReorder) {
    object->link.Unlink();
    s_asyncFileReadList.LinkNode(object, LIST_HEAD, 0);
  }

  s_queueLock.Leave();

  OsOutputDebugString("AsyncFileReadWait() Loop\n");
  AsyncFileReadPollHandler(0, 0);
  while (s_asyncWaitObject) {
    OsSleep(1);
    AsyncFileReadPollHandler(0, 0);
  }

  --s_waiting;
}

void AsyncFileReadWaitAll() {
  while (AsyncFileReadIsReading()) {
    AsyncFileReadPollHandler(0, 0);
    OsSleep(1);
  }
}

bool AsyncFileReadIsReading() {
  bool reading;

  s_queueLock.Enter();
  reading = s_asyncCurrentObject != 0 || s_asyncFileReadList.Head() != 0 || s_asyncFileReadPostList.Head() != 0;
  s_queueLock.Leave();

  return reading;
}

BOOL AsyncFileReadPollHandler(LPCVOID, LPVOID) {
  UINT index;
  UINT start;

  for (index = 0; index < s_handlers.Count(); ++index) {
    s_handlers[index]();
  }

  s_queueEvent.Set();
  start = OsGetAsyncTimeMsPrecise();

  for (;;) {
    CAsyncObject *object;

    s_queueLock.Enter();
    object = s_asyncFileReadPostList.Head();
    if (!object) {
      s_queueLock.Leave();
      return 1;
    }

    object->link.Unlink();
    if (object == s_asyncWaitObject) {
      s_asyncWaitObject = 0;
    }
    object->isLoaded = 1;
    s_queueLock.Leave();

    ASSERT(object->userPostloadCallback);
    object->userPostloadCallback(object->userArg);

    if (OsGetAsyncTimeMsPrecise() - start > 20) {
      return 1;
    }
  }
}
