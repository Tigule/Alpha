#include "MDLTypes.h"

#include <storm.h>
#include <stpl.h>

class CMDLStatus;
void OsGetTimeStr(char *timeBuffer, unsigned long length);

namespace MDL {
void __cdecl WriteLine(TSGrowableArray<char> &buffer, const char *format, ...);

int WriteHeaderComment(
    const MDLDATA &data,
    TSGrowableArray<char> &buffer,
    CMDLStatus *
) {
  char timeBuffer[80];
  WriteLine(buffer, "// MDLFile version\tDec 11 2003 17:58:18\n");
  OsGetTimeStr(timeBuffer, sizeof(timeBuffer));
  WriteLine(buffer, "// Exported on %s", timeBuffer);
  if (SStrLen(data.header.userName)) {
    WriteLine(buffer, " by %s", static_cast<const char *>(data.header.userName));
  }
  WriteLine(buffer, "\n");
  if (SStrLen(data.header.sourceFilename)) {
    WriteLine(buffer, "// SCENE_FILENAME \"%s\"\n", static_cast<const char *>(data.header.sourceFilename));
  }
  return 1;
}
}
