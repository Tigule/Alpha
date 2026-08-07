#include <Base/Base.h>

#include "MDLTypes.h"

#include <storm.h>
#include <stpl.h>

class CMDLStatus;
void OsGetTimeStr(char *timeBuffer, DWORD length);

namespace MDL {
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);

  BOOL WriteHeaderComment(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
    char timebuf[80];
    WriteLine(buffer, "// MDLFile version\tDec 11 2003 17:58:18\n");
    OsGetTimeStr(timebuf, sizeof(timebuf));
    WriteLine(buffer, "// Exported on %s", timebuf);
    if (SStrLen(data.header.userName)) {
      WriteLine(buffer, " by %s", static_cast<LPCSTR>(data.header.userName));
    }
    WriteLine(buffer, "\n");
    if (SStrLen(data.header.sourceFilename)) {
      WriteLine(buffer, "// SCENE_FILENAME \"%s\"\n", static_cast<LPCSTR>(data.header.sourceFilename));
    }
    return 1;
  }
}  // namespace MDL
