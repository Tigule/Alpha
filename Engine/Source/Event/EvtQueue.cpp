#include "EvtInt.h"

#include <Base/Activity.h>
#include <Base/CDataRecycler.h>

#include <string.h>

const float PI = 3.14159265358979323846f;
const float TWO_PI = PI + PI;
const float OO_TWO_PI = 1.0f / TWO_PI;

static TExtraInstanceRecycler<EvtMessage> s_messageRecycler(0x40, 0x40, 0x100);

EvtMessage *MessageAlloc(unsigned long bytes) {
  if (bytes <= 4) {
    bytes = 4;
  }
  return s_messageRecycler.Get(bytes + 0x10);
}

void MessageFree(EvtMessage *message) {
  s_messageRecycler.Put(message);
}

void ResetSyncState(EvtContext *context) {
  context->QueueResetSyncButtonState(~0u);

  LISTEX(EvtKeyDown, link) &keyDownList = context->QueueLockSyncKeyDownList();

  EvtKeyDown *keyDown;
  while ((keyDown = keyDownList.Head()) != 0) {
    keyDown->link.Unlink();
    SMemFree(keyDown, typeid(EvtKeyDown).raw_name(), SERR_LINECODE_OBJECT, 0);
  }

  context->QueueUnlockSyncKeyDownList();
}

void UpdateSyncKeyState(EvtContext *context, KEY key, EVENTID &id) {
  int keyDown = 0;

  LISTEX(EvtKeyDown, link) &keyDownList = context->QueueLockSyncKeyDownList();
  EvtKeyDown                    *entry = keyDownList.Head();
  while (entry) {
    if (entry->key == key) {
      EvtKeyDown *next = keyDownList.Next(entry);
      keyDown = 1;
      entry->link.Unlink();
      SMemFree(entry, typeid(EvtKeyDown).raw_name(), SERR_LINECODE_OBJECT, 0);
      entry = next;
    } else {
      entry = keyDownList.Next(entry);
    }
  }

  if (id == EVENT_ID_KEYDOWN) {
    if (keyDown) {
      id = EVENT_ID_KEYDOWN_REPEATING;
    }

    void *storage = SMemAlloc(sizeof(EvtKeyDown), typeid(EvtKeyDown).raw_name(), SERR_LINECODE_OBJECT, SMEM_FLAG_ZEROMEMORY);
    entry = storage ? new (storage) EvtKeyDown : 0;
    keyDownList.LinkNode(entry, LIST_TAIL, 0);
    entry->key = key;
  }

  context->QueueUnlockSyncKeyDownList();
}

void UpdateSyncMouseState(EvtContext *context, MOUSEBUTTON button, int down) {
  if (down) {
    context->QueueSetSyncButtonState(button);
  } else {
    context->QueueResetSyncButtonState(button);
  }
}

void UpdateSyncState(EvtContext *context, EVENTID &id, const void *data) {
  FATALASSERT(context);

  switch (id) {
    case EVENT_ID_FOCUS:
      ResetSyncState(context);
      break;

    case EVENT_ID_KEYDOWN:
    case EVENT_ID_KEYUP:
      UpdateSyncKeyState(context, static_cast<const EVENT_DATA_KEY *>(data)->key, id);
      break;

    case EVENT_ID_MOUSEDOWN:
    case EVENT_ID_MOUSEUP:
      UpdateSyncMouseState(context, static_cast<const EVENT_DATA_MOUSE *>(data)->button, id == EVENT_ID_MOUSEDOWN);
      break;
  }
}

void IEvtQueueInitialize() {
}

void IEvtQueueDestroy() {
  s_messageRecycler.Clear();
}

int IEvtQueueCheckSyncKeyState(EvtContext *context, KEY key) {
  FATALASSERT(context);

  LISTEX(EvtKeyDown, link) &keyDownList = context->QueueLockSyncKeyDownList();
  EvtKeyDown                    *entry = keyDownList.Head();
  while (entry) {
    if (entry->key == key) {
      context->QueueUnlockSyncKeyDownList();
      return 1;
    }
    entry = keyDownList.Next(entry);
  }

  context->QueueUnlockSyncKeyDownList();
  return 0;
}

int IEvtQueueCheckSyncMouseState(EvtContext *context, MOUSEBUTTON button) {
  FATALASSERT(context);

  return context->QueueGetSyncButtonState(button) != 0;
}

void IEvtQueueDispatch(EvtContext *context, EVENTID id, const void *data) {
  FATALASSERT(context);

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
  EvtHandler                     marker;
  marker.marker = 1;

  handlerList.LinkNode(&marker, LIST_HEAD, 0);
  EvtHandler *handler;
  while ((handler = handlerList.Next(&marker)) != 0) {
    handlerList.LinkNode(&marker, LIST_LINK_AFTER, handler);
    if (!handler->marker && !handler->func(data, handler->param)) {
      break;
    }
  }
  marker.link.Unlink();

  context->QueueUnlockHandlerList();

  if (activity != ACTIVITY_OTHER) {
    ActivityEnd(activity);
  }
}

int IEvtQueueHasMessages(EvtContext *context) {
  FATALASSERT(context);

  LISTEX(EvtMessage, link) &messageList = context->QueueLockMessageList();
  int                            hasMessages = messageList.Head() != 0;
  context->QueueUnlockMessageList();
  return hasMessages;
}

int IEvtQueueDispatchNext(EvtContext *context) {
  FATALASSERT(context);

  LISTEX(EvtMessage, link) &messageList = context->QueueLockMessageList();
  EvtMessage                    *message = messageList.Head();
  if (message) {
    message->link.Unlink();
  }
  int hasMore = messageList.Head() != 0;
  context->QueueUnlockMessageList();

  if (!message) {
    return 0;
  }

  IEvtQueueDispatch(context, message->id, message->data);
  MessageFree(message);
  return hasMore;
}

void IEvtQueueDispatchAll(EvtContext *context) {
  FATALASSERT(context);

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

void IEvtQueuePost(EvtContext *context, EVENTID id, const void *data, unsigned int bytes) {
  FATALASSERT(context);

  EvtMessage *message = MessageAlloc(bytes);
  message->id = id;
  if (data) {
    memcpy(message->data, data, bytes);
  }

  LISTEX(EvtMessage, link) &messageList = context->QueueLockMessageList();
  messageList.LinkNode(message, LIST_TAIL, 0);
  context->QueueUnlockMessageList();
}

void IEvtQueueScan(EvtContext *context, EVENTSCANHANDLER scanner, void *param) {
  FATALASSERT(context);

  FATALASSERT(scanner);

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

void IEvtQueueRegister(EvtContext *context, EVENTID id, EVENTHANDLER handler, void *param, float priority) {
  FATALASSERT(context);

  LISTEX(EvtHandler, link) &handlerList = context->QueueLockHandlerList(id);
  void                          *storage = SMemAlloc(sizeof(EvtHandler), typeid(EvtHandler).raw_name(), SERR_LINECODE_OBJECT, SMEM_FLAG_ZEROMEMORY);
  EvtHandler                    *newHandler = storage ? new (storage) EvtHandler : 0;
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

void IEvtQueueUnregister(EvtContext *context, EVENTID id, EVENTHANDLER handler, void *param, unsigned int flags) {
  FATALASSERT(context);

  for (int checkId = 0; checkId < EVENTIDS; ++checkId) {
    if ((flags & 1) && checkId != id) {
      continue;
    }

    LISTEX(EvtHandler, link) &handlerList = context->QueueLockHandlerList(static_cast<EVENTID>(checkId));
    EvtHandler                    *registered = handlerList.Head();
    while (registered) {
      if (((flags & 2) && registered->func != handler) || ((flags & 4) && registered->param != param) || registered->marker) {
        registered = handlerList.Next(registered);
      } else {
        EvtHandler *next = handlerList.Next(registered);
        registered->link.Unlink();
        SMemFree(registered, typeid(EvtHandler).raw_name(), SERR_LINECODE_OBJECT, 0);
        registered = next;
      }
    }

    context->QueueUnlockHandlerList();
  }
}
