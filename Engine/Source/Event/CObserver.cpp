#include "Base/CDataAllocator.h"
#include "Base/InstanceId.h"
#include "CObserver.h"
#include "CMouseEvent.h"

#include <stpl.h>
#include <typeinfo>

struct EventReg : public TSHashObject<EventReg, HASHKEY_NONE> {
  struct EVENTCALLBACKREG;
  struct EVENTDISPATCHREG;
  class EventIterator;
  typedef EVENTCALLBACKREG *PEVENTCALLBACKREG;
  typedef const EVENTCALLBACKREG *PCEVENTCALLBACKREG;
  typedef EVENTDISPATCHREG *PEVENTDISPATCHREG;
  typedef const EVENTDISPATCHREG *PCEVENTDISPATCHREG;

 private:
  enum {
    LOCKED = 0x0FFFFFFF,
    CHANGED = 0x80000000
  };

 public:
  EventReg();
  ~EventReg();

  void RegisterCallback(EVENTCALLBACK callback, void *param);
  void RegisterEvent(int expectedEventId, CObserver *pObserver);
  void UnregisterCallback(EVENTCALLBACK callback);
  void UnregisterEvent(CObserver *pObserver);
  void CleanupCallbacks();
  void CleanupEvents();
  int  IsCallbackRegistered(EVENTCALLBACK callback) const;
  int  IsEventRegistered(CObserver *pObserver) const;
  int  DispatchCallback(CEvent &event);
  int  DispatchEvent(CEvent &event);

 private:
  void SetFlag(unsigned long flag) {
    flags |= flag;
  }

  void ClearFlag(unsigned long flag) {
    flags &= ~flag;
  }

  int TestFlag(unsigned long flag) const {
    return (flags & flag) != 0;
  }

 public:
  int IsEmpty() const {
    return !callbackList.Head() && !dispatchList.Head();
  }

  int Locked() const {
    return (flags & 0x0FFFFFFF) != 0;
  }

  void IncLock() {
    ++flags;
  }

  void DecLock() {
    --flags;
  }

  int Changed() const {
    return (flags & 0x80000000) != 0;
  }

  void MarkChanged() {
    flags |= 0x80000000;
  }

  void ResetChanged() {
    flags &= 0x7FFFFFFF;
  }

 private:
  unsigned long                                          flags;
  LISTDECL(EVENTCALLBACKREG, callbackList);
  LISTDECL(EVENTDISPATCHREG, dispatchList);
};

struct EventReg::EVENTCALLBACKREG : public TSLinkedNode<EventReg::EVENTCALLBACKREG> {
  EVENTCALLBACK callback;
  void         *param;
};

struct EventReg::EVENTDISPATCHREG : public TSLinkedNode<EventReg::EVENTDISPATCHREG> {
  TRefCntPtr<CObserver> pObserver;
  int                   expectedEventId;
};

class EventReg::EventIterator {
  EventIterator(EventReg &);
  EventIterator(const EventIterator &);
  EventIterator &operator=(const EventIterator &);

 public:
  int Next(int &, CObserver *&);

 private:
  EventReg         &m_reg;
  EVENTDISPATCHREG *m_ptr;
};

class EventRegistry : public TSHashTable<EventReg, HASHKEY_NONE> {
 public:
  EventRegistry() {
  }
  EventRegistry(const EventRegistry &);

 private:
  EventRegistry &operator=(const EventRegistry &);
  virtual void      InternalDelete(EventReg *pReg);
  virtual EventReg *InternalNew(TSExplicitList<EventReg, -572662307> *list, unsigned long extrabytes, unsigned long flags);
};

static HASHKEY_NONE                                           s_eventRegistryKey;
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

  EventReg *reg = m_pEventRegistry->Head();
  while (reg) {
    EventReg *next = m_pEventRegistry->Next(reg);
    reg->UnregisterCallback(0);
    reg->UnregisterEvent(0);
    if (reg->IsEmpty() && !reg->Locked()) {
      m_pEventRegistry->Delete(reg);
    }
    reg = next;
  }

  if (!m_pEventRegistry->Head()) {
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

EventReg *CObserver::GetEventReg(unsigned int eventId, int create) {
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

int CObserver::DispatchEvent(CEvent &event) {
  return DispatchEvent(event.Id(), event);
}

int CObserver::OnEvent(const CEvent &) {
  return 0;
}

int CObserver::DispatchEvent(int id, CEvent &event) {
  EventReg *reg = GetEventReg(id, 0);
  if (!reg) {
    return 0;
  }

  IncrRef();
  int handled = reg->DispatchCallback(event) != 0;
  if (reg->DispatchEvent(event)) {
    handled = 1;
  }

  if (!reg->Locked() && reg->Changed()) {
    reg->CleanupCallbacks();
    reg->CleanupEvents();
    reg->ResetChanged();
  }

  if (reg->IsEmpty() && !reg->Locked()) {
    m_pEventRegistry->Delete(reg);
    if (!m_pEventRegistry->Head()) {
      DEL(m_pEventRegistry);
      m_pEventRegistry = 0;
    }
  }

  DecrRef();
  return handled;
}

void CObserver::RegisterCallback(unsigned int id, EVENTCALLBACK callback, void *param) {
  EventReg *reg = GetEventReg(id, 1);
  ASSERT(reg);
  reg->RegisterCallback(callback, param);
}

void CObserver::RegisterEvent(unsigned int id, int expectedEventId, CObserver *pObserver) {
  EventReg *reg = GetEventReg(id, 1);
  ASSERT(reg);
  reg->RegisterEvent(expectedEventId, pObserver);
}

void CObserver::UnregisterCallback(unsigned int id, EVENTCALLBACK callback) {
  EventReg *reg = GetEventReg(id, 0);
  if (!reg) {
    return;
  }

  reg->UnregisterCallback(callback);
  if (reg->IsEmpty() && !reg->Locked()) {
    m_pEventRegistry->Delete(reg);
    if (!m_pEventRegistry->Head()) {
      DEL(m_pEventRegistry);
      m_pEventRegistry = 0;
    }
  }
}

void CObserver::UnregisterEvent(unsigned int id, CObserver *pObserver) {
  EventReg *reg = GetEventReg(id, 0);
  if (!reg) {
    return;
  }

  reg->UnregisterEvent(pObserver);
  if (reg->IsEmpty() && !reg->Locked()) {
    m_pEventRegistry->Delete(reg);
    if (!m_pEventRegistry->Head()) {
      DEL(m_pEventRegistry);
      m_pEventRegistry = 0;
    }
  }
}

int CObserver::IsEventRegistered(unsigned int id) {
  EventReg *reg = GetEventReg(id, 0);
  return reg && !reg->IsEmpty();
}

int CObserver::IsEventRegisteredBy(unsigned int id, CObserver *pObserver) {
  EventReg *reg = GetEventReg(id, 0);
  return reg && reg->IsEventRegistered(pObserver);
}

void EventRegistry::InternalDelete(EventReg *pReg) {
  FATALASSERT(!pReg->Locked());
  s_eventRegAllocator.Put(pReg);
}

EventReg *EventRegistry::InternalNew(TSExplicitList<EventReg, -572662307> *, unsigned long extrabytes, unsigned long flags) {
  FATALASSERT(!extrabytes);
  return s_eventRegAllocator.Get((flags & SMEM_FLAG_ZEROMEMORY) != 0);
}

EventReg::EventReg() : flags(0) {
}

EventReg::~EventReg() {
  CleanupCallbacks();
  CleanupEvents();
}

void EventReg::RegisterCallback(EVENTCALLBACK callback, void *param) {
  EVENTCALLBACKREG *entry;
  for (entry = callbackList.Head(); entry; entry = callbackList.Next(entry)) {
    if (entry->callback == callback) {
      entry->param = param;
      return;
    }
  }

  entry = s_callbackRegAllocator.Get(0);
  ASSERT(entry);
  callbackList.LinkNode(entry, LIST_TAIL, 0);
  entry->callback = callback;
  entry->param = param;
}

void EventReg::RegisterEvent(int expectedEventId, CObserver *pObserver) {
  EVENTDISPATCHREG *entry;
  for (entry = dispatchList.Head(); entry; entry = dispatchList.Next(entry)) {
    if (entry->pObserver == pObserver) {
      entry->expectedEventId = expectedEventId;
      return;
    }
  }

  entry = s_dispatchRegAllocator.Get(0);
  ASSERT(entry);
  dispatchList.LinkNode(entry, LIST_TAIL, 0);
  entry->pObserver = pObserver;
  entry->expectedEventId = expectedEventId;
}

void EventReg::UnregisterCallback(EVENTCALLBACK callback) {
  EVENTCALLBACKREG *entry = callbackList.Head();
  while (entry) {
    EVENTCALLBACKREG *next = callbackList.Next(entry);
    if (!callback || entry->callback == callback) {
      entry->callback = 0;
      if (Locked()) {
        MarkChanged();
      } else {
        callbackList.UnlinkNode(entry);
        s_callbackRegAllocator.Put(entry);
      }
      if (callback) {
        break;
      }
    }
    entry = next;
  }
}

void EventReg::UnregisterEvent(CObserver *pObserver) {
  EVENTDISPATCHREG *entry = dispatchList.Head();
  while (entry) {
    EVENTDISPATCHREG *next = dispatchList.Next(entry);
    if (!pObserver || entry->pObserver == pObserver) {
      entry->pObserver = static_cast<CObserver *>(0);
      if (Locked()) {
        MarkChanged();
      } else {
        dispatchList.UnlinkNode(entry);
        s_dispatchRegAllocator.Put(entry);
      }
      if (pObserver) {
        break;
      }
    }
    entry = next;
  }
}

void EventReg::CleanupCallbacks() {
  EVENTCALLBACKREG *entry = callbackList.Head();
  while (entry) {
    EVENTCALLBACKREG *next = callbackList.Next(entry);
    if (!entry->callback) {
      callbackList.UnlinkNode(entry);
      s_callbackRegAllocator.Put(entry);
    }
    entry = next;
  }
}

void EventReg::CleanupEvents() {
  EVENTDISPATCHREG *entry = dispatchList.Head();
  while (entry) {
    EVENTDISPATCHREG *next = dispatchList.Next(entry);
    if (!entry->pObserver) {
      dispatchList.UnlinkNode(entry);
      s_dispatchRegAllocator.Put(entry);
    }
    entry = next;
  }
}

int EventReg::IsCallbackRegistered(EVENTCALLBACK callback) const {
  const EVENTCALLBACKREG *entry;
  for (entry = callbackList.Head(); entry; entry = callbackList.Next(entry)) {
    if (entry->callback == callback) {
      return 1;
    }
  }
  return 0;
}

int EventReg::IsEventRegistered(CObserver *pObserver) const {
  const EVENTDISPATCHREG *entry;
  for (entry = dispatchList.Head(); entry; entry = dispatchList.Next(entry)) {
    if (entry->pObserver == pObserver) {
      return 1;
    }
  }
  return 0;
}

int EventReg::DispatchCallback(CEvent &event) {
  IncLock();
  EVENTCALLBACKREG endOfList;
  callbackList.LinkNode(&endOfList, LIST_TAIL, 0);

  int handled = 0;
  EVENTCALLBACKREG *entry = callbackList.Head();
  while (entry != &endOfList) {
    if (entry->callback && entry->callback(event, entry->param)) {
      handled = 1;
    }
    entry = callbackList.Next(entry);
  }

  callbackList.UnlinkNode(&endOfList);
  DecLock();
  return handled;
}

int EventReg::DispatchEvent(CEvent &event) {
  IncLock();
  EVENTDISPATCHREG endOfList;
  dispatchList.LinkNode(&endOfList, LIST_TAIL, 0);

  int handled = 0;
  EVENTDISPATCHREG *entry = dispatchList.Head();
  while (entry != &endOfList) {
    if (entry->pObserver) {
      event.SetId(entry->expectedEventId);
      if (entry->pObserver->OnEvent(event)) {
        handled = 1;
      }
    }
    entry = dispatchList.Next(entry);
  }

  dispatchList.UnlinkNode(&endOfList);
  DecLock();
  return handled;
}
