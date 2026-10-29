#include <winsock2.h>
#include <windows.h>
#include <Base/Base.h>
#include <time.h>

#include "OsGui.h"
#include <stdio.h>

char *OsGetLastErrorStr() {
  char *msgBuf;
  FormatMessageA(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, GetLastError(), 0x400,
      (char *)&msgBuf, 0, 0
  );
  return msgBuf;
}

void OsFreeLastErrorStr(char *msgBuf) {
  LocalFree(msgBuf);
}
