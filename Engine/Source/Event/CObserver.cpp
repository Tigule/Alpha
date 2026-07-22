#include "Base/CDataAllocator.h"
#include "Base/InstanceId.h"
#include "CObserver.h"

#include <stpl.h>
#include <typeinfo>

struct EventReg : public TSHashObject<EventReg, HASHKEY_NONE> {
  struct EVENTCALLBACKREG;
  struct EVENTDISPATCHREG;

  unsigned int                                           flags;
  TSList<EVENTCALLBACKREG, TSGetLink<EVENTCALLBACKREG> > callbackList;
  TSList<EVENTDISPATCHREG, TSGetLink<EVENTDISPATCHREG> > dispatchList;
};

struct EventReg::EVENTCALLBACKREG : public TSLinkedNode<EventReg::EVENTCALLBACKREG> {
  CObserver::EVENTCALLBACK callback;
  void                    *param;
};

struct EventReg::EVENTDISPATCHREG : public TSLinkedNode<EventReg::EVENTDISPATCHREG> {
  TRefCntPtr<CObserver> pObserver;
  int                   expectedEventId;
};

static TLockedInstanceAllocator<EventReg>                   s_eventRegAllocator(0x400);
static TLockedInstanceAllocator<EventReg::EVENTCALLBACKREG> s_callbackRegAllocator(0x100);
static TLockedInstanceAllocator<EventReg::EVENTDISPATCHREG> s_dispatchRegAllocator(0x400);

void __fastcall ObserverInitialize() {
}

void __fastcall ObserverDestroy() {
  s_eventRegAllocator.Clear(typeid(EventReg).raw_name(), SERR_LINECODE_OBJECT);
  s_callbackRegAllocator.Clear(typeid(EventReg::EVENTCALLBACKREG).raw_name(), SERR_LINECODE_OBJECT);
  s_dispatchRegAllocator.Clear(typeid(EventReg::EVENTDISPATCHREG).raw_name(), SERR_LINECODE_OBJECT);
}
