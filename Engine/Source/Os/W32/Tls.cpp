#include <Base/Base.h>

DWORD __fastcall OsTlsAlloc() {
  DWORD tlsIndex = TlsAlloc();

  ASSERT(tlsIndex != (DWORD)-1);

  return tlsIndex;
}

void __fastcall OsTlsFree(DWORD index) {
  // tigule: Modern Windows may allocate valid TLS indices above the
  // legacy 63-slot limit assumed by the original client.
  ASSERT(index != TLS_OUT_OF_INDEXES);

  TlsFree(index);
}

void *__fastcall OsTlsGetValue(DWORD index) {
  void *value;

  // tigule: Modern Windows may allocate valid TLS indices above the
  // legacy 63-slot limit assumed by the original client.
  ASSERT(index != TLS_OUT_OF_INDEXES);

  value = TlsGetValue(index);
  FATALASSERT(GetLastError() == 0);

  return value;
}

BOOL __fastcall OsTlsSetValue(DWORD index, void *value) {
  BOOL set = TlsSetValue(index, value);

  ASSERT(set);

  return set;
}
