#include "EvtInt.h"

#include "Base/CDataAllocator.h"
#include "Base/InstanceId.h"
#include "CObserver.h"
#include "CMouseEvent.h"

#include <stpl.h>

struct EventReg : public TSHashObject<EventReg, HASHKEY_NONE> {
  struct EVENTCALLBACKREG;
  typedef EVENTCALLBACKREG       *PEVENTCALLBACKREG;
  typedef const EVENTCALLBACKREG *PCEVENTCALLBACKREG;
  struct EVENTDISPATCHREG;
  typedef EVENTDISPATCHREG       *PEVENTDISPATCHREG;
  typedef const EVENTDISPATCHREG *PCEVENTDISPATCHREG;

 private:
  enum {
    LOCKED = 0x0FFFFFFF,
    CHANGED = 0x80000000
  };

  DWORD flags;

  void SetFlag(DWORD flag) {
    flags |= flag;
  }

  void ClearFlag(DWORD flag) {
    flags &= ~flag;
  }

  BOOL TestFlag(DWORD flag) const {
    return (flags & flag) != 0;
  }

  LISTDECL(EVENTCALLBACKREG, callbackList);
  LISTDECL(EVENTDISPATCHREG, dispatchList);

 public:
  EventReg();
  ~EventReg();

  void RegisterCallback(EVENTCALLBACK callback, LPVOID param);
  void RegisterEvent(int expectedEventId, CObserver *pObserver);
  void UnregisterCallback(EVENTCALLBACK callback);
  void UnregisterEvent(CObserver *pObserver);
  void CleanupCallbacks();
  void CleanupEvents();
  BOOL IsCallbackRegistered(EVENTCALLBACK callback) const;
  BOOL IsEventRegistered(CObserver *pObserver) const;
  int  DispatchCallback(CEvent &event);
  int  DispatchEvent(CEvent &event);

  BOOL IsEmpty() const {
    return !callbackList.Head() && !dispatchList.Head();
  }

  BOOL Locked() {
    return (flags & LOCKED) > 0;
  }

  void IncLock() {
    ++flags;
  }

  void DecLock() {
    --flags;
  }

  BOOL Changed() {
    return (flags & CHANGED) > 0;
  }

  void MarkChanged() {
    flags |= 0x80000000;
  }

  void ResetChanged() {
    flags &= 0x7FFFFFFF;
  }

  class EventIterator;
};

NODEDECL(EventReg::EVENTCALLBACKREG) {
  EVENTCALLBACK callback;
  LPVOID        param;

  EVENTCALLBACKREG() : callback(0), param(0) {
  }
  EVENTCALLBACKREG(const EVENTCALLBACKREG &);
  ~EVENTCALLBACKREG() {
    callback = 0;
    param = 0;
  }
};

NODEDECL(EventReg::EVENTDISPATCHREG) {
  TRefCntPtr<CObserver> pObserver;
  int                   expectedEventId;

  EVENTDISPATCHREG() : pObserver(0), expectedEventId(-1) {
  }
  EVENTDISPATCHREG(const EVENTDISPATCHREG &);
  ~EVENTDISPATCHREG() {
    pObserver = static_cast<CObserver *>(0);
    expectedEventId = -1;
  }
};

class EventReg::EventIterator {
  EventReg         &m_reg;
  EVENTDISPATCHREG *m_ptr;

  EventIterator(const EventIterator &);
  EventIterator(EventReg &);
  EventIterator &operator=(const EventIterator &);

 public:
  int Next(int &, CObserver *&);
};

class EventRegistry : public TSHashTable<EventReg, HASHKEY_NONE> {
 public:
  EventRegistry() {
  }
  EventRegistry(const EventRegistry &);

 private:
  EventRegistry    &operator=(const EventRegistry &);
  virtual void      InternalDelete(EventReg *pReg);
  virtual EventReg *InternalNew(LISTEXDYN(EventReg) * list, DWORD extrabytes, DWORD flags);
};

static HASHKEY_NONE                                         s_eventRegistryKey;
static TLockedInstanceAllocator<EventReg>                   s_eventRegAllocator(0x400);
static TLockedInstanceAllocator<EventReg::EVENTCALLBACKREG> s_callbackRegAllocator(0x100);
static TLockedInstanceAllocator<EventReg::EVENTDISPATCHREG> s_dispatchRegAllocator(0x400);

void ObserverInitialize() {
}

void ObserverDestroy() {
  s_eventRegAllocator.Clear();
  s_callbackRegAllocator.Clear();
  s_dispatchRegAllocator.Clear();
}

CObserver::~CObserver() {
  ClearRegistry();
  ASSERT(!m_pEventRegistry);
}

void CObserver::ClearRegistry() {
  if (!m_pEventRegistry) {
    return;
  }

  int       deleteRegistry = 1;
  for (EventReg *pReg = m_pEventRegistry->Head(), *pRegnext_node;
       (int)pReg > 0 ? ((pRegnext_node = m_pEventRegistry->Next(pReg)), 1) : 0;
       pReg = pRegnext_node) {
    pReg->UnregisterCallback(0);
    pReg->UnregisterEvent(0);
    if (pReg->IsEmpty()) {
      m_pEventRegistry->Delete(pReg);
    } else {
      deleteRegistry = 0;
    }
  }

  if (deleteRegistry) {
    DEL(m_pEventRegistry);
    m_pEventRegistry = 0;
  }
}

EventRegistry *CObserver::GetRegistry(int create) {
  if (!m_pEventRegistry && create) {
    m_pEventRegistry = NEW(EventRegistry);
  }

  return m_pEventRegistry;
}

EventReg *CObserver::GetEventReg(UINT eventId, int create) {
  EventRegistry *registry = GetRegistry(create);
  if (!registry) {
    return 0;
  }

  EventReg *reg = registry->Ptr(eventId, s_eventRegistryKey);
  if (!reg && create) {
    reg = registry->New(eventId, s_eventRegistryKey, 0, 0);
  }
  return reg;
}

BOOL CObserver::DispatchEvent(CEvent &event) {
  return DispatchEvent(event.Id(), event);
}

BOOL CObserver::DispatchEvent(int id, CEvent &event) {
  EventReg *reg = GetEventReg(id, 0);
  if (!reg) {
    return 0;
  }

  IncrRef();
  int handled = 0;
  if (reg->DispatchCallback(event)) {
    handled = 1;
  }
  if (reg->DispatchEvent(event)) {
    handled = 1;
  }

  if (!reg->Locked() && reg->Changed()) {
    reg->CleanupCallbacks();
    reg->CleanupEvents();
    reg->ResetChanged();
    if (reg->IsEmpty()) {
      GetRegistry(0)->Delete(reg);
    }
  }

  DecrRef();
  return handled;
}

void CObserver::RegisterCallback(UINT id, EVENTCALLBACK callback, LPVOID param) {
  EventReg *pReg = GetEventReg(id, 1);
  ASSERT(pReg);
  pReg->RegisterCallback(callback, param);
}

void CObserver::RegisterEvent(UINT id, int expectedEventId, CObserver *pObserver) {
  EventReg *pReg = GetEventReg(id, 1);
  ASSERT(pReg);
  pReg->RegisterEvent(expectedEventId, pObserver);
}

void CObserver::UnregisterCallback(UINT id, EVENTCALLBACK callback) {
  EventReg *reg = GetEventReg(id, 0);
  if (!reg) {
    return;
  }

  reg->UnregisterCallback(callback);
  if (reg->IsEmpty()) {
    GetRegistry(0)->Delete(reg);
  }
}

void CObserver::UnregisterEvent(UINT id, CObserver *pObserver) {
  EventReg *reg = GetEventReg(id, 0);
  if (!reg) {
    return;
  }

  reg->UnregisterEvent(pObserver);
  if (reg->IsEmpty()) {
    GetRegistry(0)->Delete(reg);
  }
}

BOOL CObserver::IsEventRegistered(UINT id) {
  return GetEventReg(id, 0) != 0;
}

BOOL CObserver::IsEventRegisteredBy(UINT id, CObserver *pObserver) {
  EventReg *reg = GetEventReg(id, 0);
  if (!reg) {
    return 0;
  }

  return reg->IsEventRegistered(pObserver);
}

void EventRegistry::InternalDelete(EventReg *pReg) {
  FATALASSERT(!pReg->Locked());
  s_eventRegAllocator.Put(pReg);
}

EventReg *EventRegistry::InternalNew(LISTEXDYN(EventReg) * list, DWORD extrabytes, DWORD flags) {
  FATALASSERT(extrabytes == 0);
  EventReg *pReg = s_eventRegAllocator.Get(0);
  list->LinkNode(pReg, LIST_HEAD, 0);
  return pReg;
}

EventReg::EventReg() : flags(0) {
}

EventReg::~EventReg() {
  for (EVENTCALLBACKREG *pCallbackReg = callbackList.Head(), *pCallbackRegnext_node;
       (int)pCallbackReg > 0 ? ((pCallbackRegnext_node = callbackList.RawNext(pCallbackReg)), 1) : 0;
       pCallbackReg = pCallbackRegnext_node) {
    s_callbackRegAllocator.Put(pCallbackReg);
  }

  for (EVENTDISPATCHREG *pDispatchReg = dispatchList.Head(), *pDispatchRegnext_node;
       (int)pDispatchReg > 0 ? ((pDispatchRegnext_node = dispatchList.RawNext(pDispatchReg)), 1) : 0;
       pDispatchReg = pDispatchRegnext_node) {
    s_dispatchRegAllocator.Put(pDispatchReg);
  }
}

void EventReg::RegisterCallback(EVENTCALLBACK callback, LPVOID param) {
  ITERATELIST(EVENTCALLBACKREG, callbackList, pCallbackReg) {
    if (pCallbackReg->callback == callback) {
      pCallbackReg->callback = callback;
      pCallbackReg->param = param;
      return;
    }
  }

  pCallbackReg = s_callbackRegAllocator.Get(0);
  callbackList.LinkNode(pCallbackReg, LIST_TAIL, 0);
  ASSERT(pCallbackReg);
  pCallbackReg->callback = callback;
  pCallbackReg->param = param;
}

void EventReg::RegisterEvent(int expectedEventId, CObserver *pObserver) {
  ITERATELIST(EVENTDISPATCHREG, dispatchList, pDispatchReg) {
    if (pDispatchReg->pObserver.m_ptr == pObserver) {
      pDispatchReg->pObserver = pObserver;
      pDispatchReg->expectedEventId = expectedEventId;
      return;
    }
  }

  pDispatchReg = s_dispatchRegAllocator.Get(0);
  dispatchList.LinkNode(pDispatchReg, LIST_TAIL, 0);
  ASSERT(pDispatchReg);
  pDispatchReg->pObserver = pObserver;
  pDispatchReg->expectedEventId = expectedEventId;
}

void EventReg::UnregisterCallback(EVENTCALLBACK callback) {
  for (EVENTCALLBACKREG *pCallbackReg = callbackList.Head(), *pCallbackRegnext_node;
       (int)pCallbackReg > 0 ? ((pCallbackRegnext_node = callbackList.RawNext(pCallbackReg)), 1) : 0;
       pCallbackReg = pCallbackRegnext_node) {
    if (pCallbackReg->callback == callback || !callback) {
      pCallbackReg->callback = 0;
      if (Locked()) {
        MarkChanged();
      } else {
        s_callbackRegAllocator.Put(pCallbackReg);
      }
      if (callback) {
        break;
      }
    }
  }
}

void EventReg::UnregisterEvent(CObserver *pObserver) {
  for (EVENTDISPATCHREG *pDispatchReg = dispatchList.Head(), *pDispatchRegnext_node;
       (int)pDispatchReg > 0 ? ((pDispatchRegnext_node = dispatchList.RawNext(pDispatchReg)), 1) : 0;
       pDispatchReg = pDispatchRegnext_node) {
    if (pDispatchReg->pObserver.m_ptr == pObserver || !pObserver) {
      pDispatchReg->pObserver = static_cast<CObserver *>(0);
      if (Locked()) {
        MarkChanged();
      } else {
        s_dispatchRegAllocator.Put(pDispatchReg);
      }
      if (pObserver) {
        break;
      }
    }
  }
}

void EventReg::CleanupCallbacks() {
  for (EVENTCALLBACKREG *pCallbackReg = callbackList.Head(), *pCallbackRegnext_node;
       (int)pCallbackReg > 0 ? ((pCallbackRegnext_node = callbackList.RawNext(pCallbackReg)), 1) : 0;
       pCallbackReg = pCallbackRegnext_node) {
    if (!pCallbackReg->callback) {
      s_callbackRegAllocator.Put(pCallbackReg);
    }
  }
}

void EventReg::CleanupEvents() {
  for (EVENTDISPATCHREG *pDispatchReg = dispatchList.Head(), *pDispatchRegnext_node;
       (int)pDispatchReg > 0 ? ((pDispatchRegnext_node = dispatchList.RawNext(pDispatchReg)), 1) : 0;
       pDispatchReg = pDispatchRegnext_node) {
    if (!pDispatchReg->pObserver) {
      s_dispatchRegAllocator.Put(pDispatchReg);
    }
  }
}

BOOL EventReg::IsCallbackRegistered(EVENTCALLBACK callback) const {
  for (const EVENTCALLBACKREG *entry = callbackList.Head(); (int)entry > 0; entry = callbackList.RawNext(entry)) {
    if (entry->callback == callback) {
      return 1;
    }
  }
  return 0;
}

BOOL EventReg::IsEventRegistered(CObserver *pObserver) const {
  for (const EVENTDISPATCHREG *entry = dispatchList.Head(); (int)entry > 0; entry = dispatchList.RawNext(entry)) {
    if (entry->pObserver == pObserver) {
      return 1;
    }
  }
  return 0;
}

BOOL EventReg::DispatchCallback(CEvent &event) {
  int handled = 0;

  IncLock();

  EVENTCALLBACKREG endOfList;

  callbackList.LinkNode(&endOfList, LIST_TAIL, 0);

  for (EVENTCALLBACKREG *entry = callbackList.Head(); entry != &endOfList; entry = callbackList.Next(entry)) {
    if (entry->callback && entry->callback(event, entry->param)) {
      handled = 1;
    }
  }

  callbackList.UnlinkNode(&endOfList);
  DecLock();
  return handled;
}

BOOL EventReg::DispatchEvent(CEvent &event) {
  int handled = 0;

  IncLock();

  EVENTDISPATCHREG endOfList;

  dispatchList.LinkNode(&endOfList, LIST_TAIL, 0);

  for (EVENTDISPATCHREG *entry = dispatchList.Head(); entry != &endOfList; entry = dispatchList.Next(entry)) {
    if (entry->pObserver) {
      event.SetId(entry->expectedEventId);
      if (entry->pObserver->OnEvent(event)) {
        handled = 1;
      }
    }
  }

  dispatchList.UnlinkNode(&endOfList);
  DecLock();
  return handled;
}
