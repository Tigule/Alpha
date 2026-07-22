#include "CInputObserver.h"

static TRefCntPtr<CInputObserver> s_pInputObserver;

void __fastcall InputObserverInitialize() {
}

void __fastcall InputObserverDestroy() {
  s_pInputObserver = static_cast<CInputObserver *>(0);
}
