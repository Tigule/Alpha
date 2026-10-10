#include "Base.h"

static DWORD s_tlsIndex;

void PropInitialize() {
  s_tlsIndex = OsTlsAlloc();
  OsTlsSetValue(s_tlsIndex, 0);
}

void PropDestroy() {
  OsTlsFree(s_tlsIndex);
}

HPROPCONTEXT PropCreateContext() {
  return (HPROPCONTEXT)ALLOCZERO(PROPERTIES * sizeof(LPVOID));
}

void PropSelectContext(HPROPCONTEXT context) {
  OsTlsSetValue(s_tlsIndex, context);
}

HPROPCONTEXT PropGetSelectedContext() {
  return (HPROPCONTEXT)OsTlsGetValue(s_tlsIndex);
}

void PropDeleteContext(HPROPCONTEXT context) {
  if (context) {
    DEL(context);
  }
}

LPVOID PropGet(PROPERTY id) {
  LPVOID *context = (LPVOID *)OsTlsGetValue(s_tlsIndex);
  return context ? context[id] : 0;
}

void PropSet(PROPERTY id, LPVOID value) {
  LPVOID *context = (LPVOID *)OsTlsGetValue(s_tlsIndex);
  if (context) {
    context[id] = value;
  }
}
