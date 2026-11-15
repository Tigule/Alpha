#include <winsock2.h>
#include <windows.h>
#include <Base/Base.h>
#include <time.h>

#include "OsGui.h"
#include <stdio.h>

DWORD OsTlsAlloc() {
  DWORD tlsIndex = TlsAlloc();

  ASSERT(tlsIndex!=(DWORD)-1);

  return tlsIndex;
}

void OsTlsFree(DWORD index) {
#if !BUILD_FIXES
  ASSERT(index<(64-1));
#else
  // tigule: Modern Windows may allocate valid TLS indices above the
  // legacy 63-slot limit assumed by the original client.
  ASSERT(index != TLS_OUT_OF_INDEXES);
#endif

  TlsFree(index);
}

LPVOID OsTlsGetValue(DWORD index) {
  LPVOID value;

#if !BUILD_FIXES
  ASSERT(index<(64-1));
#else
  // tigule: Modern Windows may allocate valid TLS indices above the
  // legacy 63-slot limit assumed by the original client.
  ASSERT(index != TLS_OUT_OF_INDEXES);
#endif

  value = TlsGetValue(index);
  VALIDATE2(GetLastError()==0);

  return value;
}

BOOL OsTlsSetValue(DWORD index, LPVOID value) {
  BOOL set = TlsSetValue(index, value);

  ASSERT(set);

  return set;
}
