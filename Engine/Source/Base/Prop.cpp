#include "Base.h"

namespace {

  DWORD s_tlsIndex;

}  // namespace

void __fastcall PropInitialize() {
  s_tlsIndex = OsTlsAlloc();
  OsTlsSetValue(s_tlsIndex, 0);
}

void __fastcall PropDestroy() {
  OsTlsFree(s_tlsIndex);
}

HPROPCONTEXT __fastcall PropCreateContext() {
  return static_cast<HPROPCONTEXT>(ALLOCZERO(PROPERTIES * sizeof(void *)));
}

void __fastcall PropSelectContext(HPROPCONTEXT context) {
  OsTlsSetValue(s_tlsIndex, context);
}

HPROPCONTEXT __fastcall PropGetSelectedContext() {
  return static_cast<HPROPCONTEXT>(OsTlsGetValue(s_tlsIndex));
}

void __fastcall PropDeleteContext(HPROPCONTEXT context) {
  if (context) {
    DEL(context);
  }
}

void *__fastcall PropGet(PROPERTY id) {
  void **context = static_cast<void **>(OsTlsGetValue(s_tlsIndex));
  return context ? context[id] : 0;
}

void __fastcall PropSet(PROPERTY id, void *value) {
  void **context = static_cast<void **>(OsTlsGetValue(s_tlsIndex));
  if (context) {
    context[id] = value;
  }
}
