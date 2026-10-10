#include "EvtInt.h"

#include <Base/Activity.h>
#include <Base/CDataRecycler.h>

#include <string.h>

static TExtraInstanceRecycler<EvtMessage> s_messageRecycler(0x40, 0x40, 0x100);

static EvtMessage *MessageAlloc(DWORD bytes) {
  return s_messageRecycler.Get(max(bytes, 4) + 0x10);
}

static void MessageFree(EvtMessage *message) {
  s_messageRecycler.Put(message);
}

static void ResetSyncState(EvtContext *context) {
  context->QueueResetSyncButtonState(~0u);

  LISTEX(EvtKeyDown, link) &keyDownList = context->QueueLockSyncKeyDownList();

  keyDownList.Clear();

  context->QueueUnlockSyncKeyDownList();
}

static void UpdateSyncKeyState(EvtContext *context, KEY key, EVENTID &id) {
  int keyDown = 0;

  LISTEX(EvtKeyDown, link) &keyDownList = context->QueueLockSyncKeyDownList();
  ITERATELIST(EvtKeyDown, keyDownList, entry) {
    if (entry->key == key) {
      keyDown = 1;
      ITERATE_DELETE;
    }
  }

  if (id == EVENT_ID_KEYDOWN) {
    if (keyDown) {
      id = EVENT_ID_KEYDOWN_REPEATING;
    }

    entry = keyDownList.NewNode(LIST_TAIL, 0, 0);
    entry->key = key;
  }

  context->QueueUnlockSyncKeyDownList();
}

static void UpdateSyncMouseState(EvtContext *context, MOUSEBUTTON button, int down) {
  if (down) {
    context->QueueSetSyncButtonState(button);
  } else {
    context->QueueResetSyncButtonState(button);
  }
}

static void UpdateSyncState(EvtContext *context, EVENTID &id, LPCVOID data) {
  VALIDATEBEGIN;
  VALIDATE(context);
  VALIDATEENDVOID;

  switch (id) {
    case EVENT_ID_FOCUS:
      ResetSyncState(context);
      break;

    case EVENT_ID_KEYDOWN:
    case EVENT_ID_KEYUP:
      UpdateSyncKeyState(context, ((const EVENT_DATA_KEY *)data)->key, id);
      break;

    case EVENT_ID_MOUSEDOWN:
    case EVENT_ID_MOUSEUP:
      UpdateSyncMouseState(context, ((const EVENT_DATA_MOUSE *)data)->button, id == EVENT_ID_MOUSEDOWN);
      break;
  }
}

void IEvtQueueInitialize() {
}

void IEvtQueueDestroy() {
  s_messageRecycler.Clear();
}

BOOL IEvtQueueCheckSyncKeyState(EvtContext *context, KEY key) {
  VALIDATEBEGIN;
  VALIDATE(context);
  VALIDATEEND;

  BOOL found = 0;
  LISTEX(EvtKeyDown, link) &keyDownList = context->QueueLockSyncKeyDownList();
  ITERATELIST(EvtKeyDown, keyDownList, entry) {
    if (entry->key == key) {
      found = 1;
      break;
    }
  }

  context->QueueUnlockSyncKeyDownList();
  return found;
}

BOOL IEvtQueueCheckSyncMouseState(EvtContext *context, MOUSEBUTTON button) {
  VALIDATEBEGIN;
  VALIDATE(context);
  VALIDATEEND;

  return context->QueueGetSyncButtonState(button) != 0;
}

void IEvtQueueDispatch(EvtContext *context, EVENTID id, LPCVOID data) {
  VALIDATEBEGIN;
  VALIDATE(context);
  VALIDATEENDVOID;

  UpdateSyncState(context, id, data);
  if (SErrIsDisplayingError()) {
    return;
  }

  ACTIVITY activity = ACTIVITY_OTHER;
  if (IEvtSchedulerIsContextInteractive(context)) {
    switch (id) {
      case EVENT_ID_IDLE:
        activity = ACTIVITY_EVENTIDLE;
        break;

      case EVENT_ID_POLL:
        activity = ACTIVITY_EVENTPOLL;
        break;

      case EVENT_ID_CHAR:
      case EVENT_ID_KEYDOWN:
      case EVENT_ID_KEYUP:
      case EVENT_ID_KEYDOWN_REPEATING:
      case EVENT_ID_IME:
        activity = ACTIVITY_EVENTKEYANDCHAR;
        break;

      case EVENT_ID_MOUSEDOWN:
      case EVENT_ID_MOUSEMOVE:
      case EVENT_ID_MOUSEMOVE_RELATIVE:
      case EVENT_ID_MOUSEUP:
      case EVENT_ID_MOUSEMODE_CHANGED:
      case EVENT_ID_MOUSEWHEEL:
        activity = ACTIVITY_EVENTMOUSE;
        break;

      case EVENT_ID_PAINT:
        activity = ACTIVITY_EVENTPAINT;
        break;

      case EVENT_ID_NET_DATA:
      case EVENT_ID_NET_CONNECT:
      case EVENT_ID_NET_DISCONNECT:
      case EVENT_ID_NET_CANTCONNECT:
        activity = ACTIVITY_EVENTNET;
        break;

      default:
        activity = ACTIVITY_EVENTHANDLERS;
        break;
    }

    ActivityBegin(activity);
  }

  LISTEX(EvtHandler, link) &handlerList = context->QueueLockHandlerList(id);
  EvtHandler marker;
  marker.marker = 1;

  handlerList.LinkNode(&marker, LIST_HEAD, 0);
  EvtHandler *handler;
  while ((handler = handlerList.Next(&marker)) != 0) {
    handlerList.LinkNode(&marker, LIST_LINK_AFTER, handler);
    if (!handler->marker && !handler->func(data, handler->param)) {
      break;
    }
  }
  handlerList.UnlinkNode(&marker);

  context->QueueUnlockHandlerList();

  if (activity != ACTIVITY_OTHER) {
    ActivityEnd(activity);
  }
}

BOOL IEvtQueueHasMessages(EvtContext *context) {
  VALIDATEBEGIN;
  VALIDATE(context);
  VALIDATEEND;

  LISTEX(EvtMessage, link) &messageList = context->QueueLockMessageList();
  BOOL hasMessages = messageList.Head() != 0;
  context->QueueUnlockMessageList();
  return hasMessages;
}

BOOL IEvtQueueDispatchNext(EvtContext *context) {
  VALIDATEBEGIN;
  VALIDATE(context);
  VALIDATEEND;

  LISTEX(EvtMessage, link) &messageList = context->QueueLockMessageList();
  EvtMessage *message = messageList.Head();
  if (message) {
    messageList.UnlinkNode(message);
  }
  BOOL hasMore = messageList.Head() != 0;
  context->QueueUnlockMessageList();

  if (!message) {
    return 0;
  }

  IEvtQueueDispatch(context, message->id, message->data);
  MessageFree(message);
  return hasMore;
}

void IEvtQueueDispatchAll(EvtContext *context) {
  VALIDATEBEGIN;
  VALIDATE(context);
  VALIDATEENDVOID;

  LISTDECLEX(EvtMessage, link, localMessageList);

  LISTEX(EvtMessage, link) &messageList = context->QueueLockMessageList();
  localMessageList.Combine(&messageList, LIST_TAIL, 0);
  context->QueueUnlockMessageList();

  EvtMessage *message;
  while ((message = localMessageList.Head()) != 0) {
    IEvtQueueDispatch(context, message->id, message->data);
    MessageFree(message);
  }
}

void IEvtQueuePost(EvtContext *context, EVENTID id, LPCVOID data, UINT bytes) {
  VALIDATEBEGIN;
  VALIDATE(context);
  VALIDATEENDVOID;

  EvtMessage *message = MessageAlloc(bytes);
  message->id = id;
  if (data) {
    memcpy(message->data, data, bytes);
  }

  LISTEX(EvtMessage, link) &messageList = context->QueueLockMessageList();
  messageList.LinkNode(message, LIST_TAIL, 0);
  context->QueueUnlockMessageList();
}

void IEvtQueueScan(EvtContext *context, EVENTSCANHANDLER scanner, LPVOID param) {
  VALIDATEBEGIN;
  VALIDATE(context);
  VALIDATE(scanner);
  VALIDATEENDVOID;

  ASSERT(context->IsCurrentContext());

  LISTDECLEX(EvtMessage, link, localMessageList);

  LISTEX(EvtMessage, link) &messageList = context->QueueLockMessageList();
  localMessageList.Combine(&messageList, LIST_TAIL, 0);
  context->QueueUnlockMessageList();

  ITERATELIST(EvtMessage, localMessageList, message) {
    scanner(message->id, message->data, param);
  }

  context->QueueLockMessageList();
  localMessageList.Combine(&messageList, LIST_TAIL, 0);
  messageList.Combine(&localMessageList, LIST_TAIL, 0);
  context->QueueUnlockMessageList();
}

void IEvtQueueRegister(EvtContext *context, EVENTID id, EVENTHANDLER handler, LPVOID param, float priority) {
  VALIDATEBEGIN;
  VALIDATE(context);
  VALIDATEENDVOID;

  LISTEX(EvtHandler, link) &handlerList = context->QueueLockHandlerList(id);
  LPVOID      storage = SMemAlloc(sizeof(EvtHandler), typeid(EvtHandler).INTERNALRAWNAME(), SERR_LINECODE_OBJECT, SMEM_FLAG_ZEROMEMORY);
  EvtHandler *newHandler = storage ? new (storage) EvtHandler : 0;
  newHandler->func = handler;
  newHandler->param = param;
  newHandler->priority = priority;
  newHandler->marker = 0;

  EvtHandler *before = handlerList.Head();
  while (before && (before->priority > priority || before->marker)) {
    before = handlerList.Next(before);
  }
  handlerList.LinkNode(newHandler, LIST_LINK_BEFORE, before);

  context->QueueUnlockHandlerList();
}

void IEvtQueueUnregister(EvtContext *context, EVENTID id, EVENTHANDLER handler, LPVOID param, UINT flags) {
  VALIDATEBEGIN;
  VALIDATE(context);
  VALIDATEENDVOID;

  for (int checkId = 0; checkId < EVENTIDS; ++checkId) {
    if ((flags & 1) && checkId != id) {
      continue;
    }

    LISTEX(EvtHandler, link) &handlerList = context->QueueLockHandlerList((EVENTID)checkId);
    ITERATELIST(EvtHandler, handlerList, registered) {
      if ((registered->func == handler || !(flags & 2)) && (registered->param == param || !(flags & 4)) && !registered->marker) {
        ITERATE_DELETE;
      }
    }

    context->QueueUnlockHandlerList();
  }
}
