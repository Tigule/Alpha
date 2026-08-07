#ifndef ENGINE_SOURCE_EVENT_COBSERVER_H
#define ENGINE_SOURCE_EVENT_COBSERVER_H

#include "Base/RefCount.h"

class CEvent;
class EventRegistry;
struct EventReg;

typedef int (*EVENTCALLBACK)(const CEvent &, LPVOID);

class CObserver : public TRefCnt {
 public:
  CObserver() : m_pEventRegistry(0) {
  }
  CObserver(const CObserver &) : TRefCnt(), m_pEventRegistry(0) {
  }
  virtual ~CObserver();

  virtual void RegisterCallback(UINT eventId, EVENTCALLBACK callback, LPVOID param);
  virtual void RegisterEvent(UINT id, int expectedEventId, CObserver *pObserver);
  virtual BOOL OnEvent(const CEvent &event);
  virtual BOOL DispatchEvent(CEvent &event);
  virtual BOOL DispatchEvent(int id, CEvent &event);

  void      UnregisterCallback(UINT eventId, EVENTCALLBACK callback);
  void      UnregisterEvent(UINT id, CObserver *pObserver);
  BOOL      IsEventRegistered(UINT id);
  BOOL      IsEventRegisteredBy(UINT id, CObserver *pObserver);
  void      ClearRegistry();
  EventReg *GetEventReg(UINT eventId, int create);

 protected:
  EventRegistry *GetRegistry(int create);

 private:
  EventRegistry *m_pEventRegistry;
};

#endif
