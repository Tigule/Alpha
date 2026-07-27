#include "Base.h"

namespace {

  DWORD s_tlsIndex;

}  // namespace

void PropInitialize() {
  s_tlsIndex = OsTlsAlloc();
  OsTlsSetValue(s_tlsIndex, 0);
}

void PropDestroy() {
  OsTlsFree(s_tlsIndex);
}

HPROPCONTEXT PropCreateContext() {
  return static_cast<HPROPCONTEXT>(ALLOCZERO(PROPERTIES * sizeof(void *)));
}

void PropSelectContext(HPROPCONTEXT context) {
  OsTlsSetValue(s_tlsIndex, context);
}

HPROPCONTEXT PropGetSelectedContext() {
  return static_cast<HPROPCONTEXT>(OsTlsGetValue(s_tlsIndex));
}

void PropDeleteContext(HPROPCONTEXT context) {
  if (context) {
    DEL(context);
  }
}

void *PropGet(PROPERTY id) {
  void **context = static_cast<void **>(OsTlsGetValue(s_tlsIndex));
  return context ? context[id] : 0;
}

void PropSet(PROPERTY id, void *value) {
  void **context = static_cast<void **>(OsTlsGetValue(s_tlsIndex));
  if (context) {
    context[id] = value;
  }
}
