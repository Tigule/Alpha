#include <Base/EngineHook.h>
#include <storm.h>

#include "CDataStore.h"

void CDataStore::Get(unsigned char *val) {
}

#ifdef WOWHOOK
void __fastcall Proxy_Get(CDataStore *pThis, unsigned char *val) {
  pThis->Get(val);
}

void EngineHook_CDataStore() {
  WOWHOOK_ATTACH(Proxy_Get, 0xb7a0);
}
#endif
