#include <Base/Base.h>

#include <storm.h>

#include <errno.h>
#include <string.h>

char *OsGetLastErrorStr() {
  LPCSTR message = strerror(errno);
  DWORD  bytes = SStrLen(message) + 1;
  char  *buffer = static_cast<char *>(SMemAlloc(bytes, __FILE__, __LINE__, 0));

  SStrCopy(buffer, message, bytes);
  return buffer;
}

void OsFreeLastErrorStr(char *msgBuf) {
  SMemFree(msgBuf, __FILE__, __LINE__, 0);
}
