#include <Base/Base.h>

#include <windows.h>

char *OsGetLastErrorStr() {
  char *msgBuf;
  FormatMessageA(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, GetLastError(), 0x400,
      reinterpret_cast<char *>(&msgBuf), 0, 0
  );
  return msgBuf;
}

void OsFreeLastErrorStr(char *msgBuf) {
  LocalFree(msgBuf);
}
