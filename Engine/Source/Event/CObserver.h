#ifndef ENGINE_SOURCE_EVENT_COBSERVER_H
#define ENGINE_SOURCE_EVENT_COBSERVER_H

#include "Base/RefCount.h"

class CEvent;
class EventRegistry;
struct EventReg;

class CObserver : public TRefCnt {
 public:
  typedef int(__fastcall *EVENTCALLBACK)(const CEvent &, void *);

  CObserver();
  CObserver(const CObserver &);
  virtual ~CObserver();

  virtual void RegisterCallback(unsigned int eventId, EVENTCALLBACK callback, void *param);
  virtual void RegisterEvent(unsigned int id, int expectedEventId, CObserver *pObserver);
  virtual int  OnEvent(const CEvent &event);
  virtual int  DispatchEvent(CEvent &event);
  virtual int  DispatchEvent(int id, CEvent &event);

  void      UnregisterCallback(unsigned int eventId, EVENTCALLBACK callback);
  void      UnregisterEvent(unsigned int id, CObserver *pObserver);
  int       IsEventRegistered(unsigned int id);
  int       IsEventRegisteredBy(unsigned int id, CObserver *pObserver);
  void      ClearRegistry();
  EventReg *GetEventReg(unsigned int eventId, int create);

  CObserver &operator=(const CObserver &);

 protected:
  EventRegistry *GetRegistry(int create);

  EventRegistry *m_pEventRegistry;
};

#endif
