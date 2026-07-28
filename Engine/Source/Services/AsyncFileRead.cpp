#include "AsyncFileRead.h"

#include "Base/Base.h"
#include "Event/EvtApi.h"
#include "Os/OsTime.h"
#include "Os/W32/Debugging.h"

#define OSWAIT_OBJECT_0 WAIT_OBJECT_0
#define OSWAIT_TIMEOUT  WAIT_TIMEOUT

static int AsyncFileReadPollHandler(const void *, void *);
static unsigned int APIENTRY AsyncFileReadThread(void *param);

static unsigned int                              s_waiting;
static LISTDECLEX(CAsyncObject, link, s_asyncFileReadList);
static LISTDECLEX(CAsyncObject, link, s_asyncFileReadFreeList);
static LISTDECLEX(CAsyncObject, link, s_asyncFileReadPostList);
static CAsyncObject                             *s_asyncCurrentObject;
static HPROPCONTEXT                              s_propContext;
static SCritSect                                 s_queueLock;
static SThread                                   s_asyncReadThread;
static SEvent                                    s_shutdownEvent(1, 0);
static SEvent                                    s_queueEvent(0, 0);
static CAsyncObject                             *s_asyncWaitObject;
static TSGrowableArray<void(*)(void)> s_handlers;

static unsigned int APIENTRY AsyncFileReadThread(void *param) {
  unsigned long waitResult;

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

void AsyncFileReadInitialize() {
  EventRegisterEx(EVENT_ID_POLL, AsyncFileReadPollHandler, 0, EVENT_PRIORITY_NORMAL);

  s_asyncCurrentObject = 0;
  s_asyncWaitObject = 0;
  s_propContext = PropGetSelectedContext();
  s_shutdownEvent.Reset();
  SThread::Create(AsyncFileReadThread, 0, s_asyncReadThread, const_cast<char *>("AsyncFileLoader"));
}

void AsyncFileReadDestroy() {
  s_shutdownEvent.Set();
  s_queueEvent.Set();
  s_asyncReadThread.Wait(INFINITE);

  s_asyncFileReadFreeList.Clear();
  ASSERT(s_asyncFileReadList.Head() == 0);
  s_asyncFileReadPostList.Clear();
  ASSERT(s_asyncCurrentObject == 0);

  s_handlers.SetCount(0);
  EventUnregisterEx(EVENT_ID_POLL, AsyncFileReadPollHandler, 0, 0xFFFFFFFF);
}

void AsyncFileReadAddHandler(void(*handler)()) {
  unsigned int index;

  for (index = 0; index < s_handlers.Count(); ++index) {
    if (s_handlers[index] == handler) {
      return;
    }
  }

  *s_handlers.New() = handler;
}

CAsyncObject *AsyncFileReadCreateObject() {
  CAsyncObject *object;

  s_queueLock.Enter();
  object = s_asyncFileReadFreeList.Head();
  if (!object) {
    object = static_cast<CAsyncObject *>(SMemAlloc(sizeof(CAsyncObject), typeid(CAsyncObject).raw_name(), -2, SMEM_FLAG_ZEROMEMORY));
    if (object) {
      new (object) CAsyncObject;
    }
    s_asyncFileReadFreeList.LinkNode(object, LIST_HEAD, 0);
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
  unsigned long location;

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

static int AsyncFileReadPollHandler(const void *, void *) {
  unsigned int index;
  unsigned int start;

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
