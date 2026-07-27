#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

namespace MDL {
const char *TokenText(unsigned int token);
void __cdecl WriteLine(TSGrowableArray<char> &buffer, const char *format, ...);

int ReadHelper(
    Parser &parse,
    MDLDATA &data,
    CMDLStatus *status
) {
  TSet errors;
  MDLGENOBJECT *helper = data.helpers.New();
  NTempest::C3Vector *pivot =
      data.pivotPoints.Count() < 500 ? data.pivotPoints.New() : 0;

  AddObjectErrors(errors);
  ReadObjectName(parse, helper->name);
  parse.Expect('{');

  const char *tokenText;
  unsigned int token = parse.Token(&tokenText, 0);
  while (token && token != '}') {
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokenText);
    }
    if (!ReadObjectBody(parse, token, pivot, helper, status)) {
      parse.FatalUnexpected(tokenText);
    }
    token = parse.Token(&tokenText, 0);
  }
  parse.Expect('}', token, tokenText);
  ReadObjectEnd(
      errors,
      data,
      helper,
      data.helpers.Count() - 1,
      0x10000000
  );
  errors.Complete(status);
  return !parse.FoundError();
}

int WriteHelpers(
    const MDLDATA &data,
    TSGrowableArray<char> &buffer,
    CMDLStatus *
) {
  if (!static_cast<const char *>(data.model.animationFile)[0]) {
    int writeObjectId = data.helpers.Count() != data.objects.Count();
    for (unsigned int i = 0; i < data.helpers.Count(); ++i) {
      WriteObjectHeader(
          data,
          data.helpers.Ptr()[i],
          0x10F,
          writeObjectId,
          buffer
      );
      WriteObjectTrailer(data.helpers.Ptr()[i], buffer);
    }
  }
  return 1;
}

int WriteBinHelpers(
    const MDLDATA &data,
    CMsgBuffer &buffer,
    CMDLStatus *status
) {
  if (!static_cast<const char *>(data.model.animationFile)[0]
      && data.helpers.Count()) {
    buffer.AddDword('PLEH');
    unsigned int totalSize = 4;
    unsigned int i;
    for (i = 0; i < data.helpers.Count(); ++i) {
      totalSize += GetBinGenObjectSize(data.helpers.Ptr()[i]);
    }
    buffer.AddUint(totalSize);
    buffer.AddUint(data.helpers.Count());
    for (i = 0; i < data.helpers.Count(); ++i) {
      WriteBinGenObject(data.helpers.Ptr()[i], buffer, status);
    }
  }
  return 1;
}

int ReadBinHelpers(
    CMsgBuffer &buffer,
    unsigned int length,
    MDLDATA &data,
    CMDLStatus *status
) {
  unsigned int totalRead = 4;
  unsigned int count = buffer.GetUint();
  data.helpers.SetCount(0);
  data.helpers.ReserveSpace(count);

  while (totalRead < length) {
    MDLGENOBJECT *helper = data.helpers.New();
    if (!helper) {
      status->FatalFlunked("Helper", -1);
      return 0;
    }
    if (!ReadBinGenObject(*helper, buffer, status, totalRead)) {
      status->Add(
          STATUS_ERROR,
          "Error reading gen object portion of helper.\n"
      );
      return 0;
    }
    if (totalRead > length) {
      status->FatalOverran("Helper section overran read buffer.\n", -1);
      return 0;
    }
    ReadBinObjectEnd(
        data,
        helper,
        data.helpers.Count() - 1,
        0x10000000
    );
  }
  return 1;
}

}
