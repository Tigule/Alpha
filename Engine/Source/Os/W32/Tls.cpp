#include <Base/Base.h>

DWORD OsTlsAlloc() {
  DWORD tlsIndex = TlsAlloc();

  ASSERT(tlsIndex != (DWORD)-1);

  return tlsIndex;
}

void OsTlsFree(DWORD index) {
  // tigule: Modern Windows may allocate valid TLS indices above the
  // legacy 63-slot limit assumed by the original client.
  ASSERT(index != TLS_OUT_OF_INDEXES);

  TlsFree(index);
}

void *OsTlsGetValue(DWORD index) {
  void *value;

  // tigule: Modern Windows may allocate valid TLS indices above the
  // legacy 63-slot limit assumed by the original client.
  ASSERT(index != TLS_OUT_OF_INDEXES);

  value = TlsGetValue(index);
  FATALASSERT(GetLastError() == 0);

  return value;
}

BOOL OsTlsSetValue(DWORD index, void *value) {
  BOOL set = TlsSetValue(index, value);

  ASSERT(set);

  return set;
}
