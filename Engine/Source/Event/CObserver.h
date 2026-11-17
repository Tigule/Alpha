#ifndef ENGINE_SOURCE_EVENT_COBSERVER_H
#define ENGINE_SOURCE_EVENT_COBSERVER_H

#include "Base/RefCount.h"

#include <stpl.h>

class CEvent;
class EventRegistry;
struct EventReg;

typedef int (*EVENTCALLBACK)(const CEvent &, LPVOID);

class CObserver : public TRefCnt {
 private:
  EventRegistry *m_pEventRegistry;

 protected:
  EventRegistry *GetRegistry(int create);

 public:
  CObserver() : m_pEventRegistry(NULL) {
  }
  CObserver(const CObserver &) : TRefCnt(), m_pEventRegistry(NULL) {
  }
  virtual ~CObserver();

  virtual void RegisterCallback(UINT eventId, EVENTCALLBACK callback, LPVOID param);
  virtual void RegisterEvent(UINT id, int expectedEventId, CObserver *pObserver);

  void      UnregisterCallback(UINT eventId, EVENTCALLBACK callback);
  void      UnregisterEvent(UINT id, CObserver *pObserver);
  BOOL      IsEventRegistered(UINT id);
  BOOL      IsEventRegisteredBy(UINT id, CObserver *pObserver);

  virtual BOOL OnEvent(const CEvent &event) { return FALSE; }
  virtual BOOL DispatchEvent(int id, CEvent &event);
  virtual BOOL DispatchEvent(CEvent &event);

  void      ClearRegistry();
  EventReg *GetEventReg(UINT eventId, int create);
};

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

  EVENTCALLBACKREG() : callback(NULL), param(NULL) {
  }
  EVENTCALLBACKREG(const EVENTCALLBACKREG &);
  ~EVENTCALLBACKREG() {
    callback = NULL;
    param = NULL;
  }
};

NODEDECL(EventReg::EVENTDISPATCHREG) {
  TRefCntPtr<CObserver> pObserver;
  int                   expectedEventId;

  EVENTDISPATCHREG() : pObserver(0), expectedEventId(-1) {
  }
  EVENTDISPATCHREG(const EVENTDISPATCHREG &);
  ~EVENTDISPATCHREG() {
    pObserver = 0;
    expectedEventId = -1;
  }
};

#endif
