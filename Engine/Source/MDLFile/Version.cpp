#include "MDLTypes.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

#include <storm.h>
#include <stpl.h>

namespace MDL {
  LPCSTR       TokenText(UINT token);
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);

  int ReadVersion(Parser &parse, MDLDATA &data, CMDLStatus *status) {
    TSet errors;
    errors.Add(0x14C, 1, 0);
    parse.Expect('{');

    LPCSTR tokentext;
    UINT   token = parse.Token(&tokentext, 0);
    while (token && token != '}') {
      if (!errors.Check(token)) {
        parse.FatalDuplicate(tokentext);
      }
      if (token == 0x14C) {
        data.version = parse.ExpectInt();
      } else {
        parse.FatalUnexpected(tokentext);
      }
      parse.Expect(',');
      token = parse.Token(&tokentext, 0);
    }
    parse.Expect('}', token, tokentext);
    errors.Complete(status);

    if (errors.Found(0x14C) && data.version > 0x514) {
      status->Add(STATUS_FATAL, "Error: File version (%u) is newer than newest version (%u) supported by app\n", data.version, 0x514);
      return 0;
    }
    return !parse.FoundError();
  }

  int WriteVersion(const MDLDATA &, TSGrowableArray<char> &buffer, CMDLStatus *) {
    WriteLine(buffer, "%s {\n", TokenText(0x103));
    WriteLine(buffer, "\t%s %d,\n", TokenText(0x14C), 0x514);
    WriteLine(buffer, "}\n");
    return 1;
  }

  int ReadBinVersion(CMsgBuffer &buf, UINT len, MDLDATA &data, CMDLStatus *status) {
    FATALASSERT(status);
    if (len == 4) {
      data.version = buf.GetUint();
      return 1;
    }
    status->Add(STATUS_ERROR, "Invalid VERX section detected in model.\n");
    return 0;
  }

  int WriteBinVersion(const MDLDATA &data, CMsgBuffer &buffer, CMDLStatus *) {
    buffer.AddDword('SREV');
    buffer.AddUint(4);
    buffer.AddUint(data.version);
    return 1;
  }
}  // namespace MDL
