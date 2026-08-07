#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

namespace MDL {
  LPCSTR       TokenText(UINT token);
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);

  BOOL ReadHelper(Parser &parse, MDLDATA &data, CMDLStatus *status) {
    TSet                errors;
    MDLGENOBJECT       *helper = data.helpers.New();
    NTempest::C3Vector *pivot = data.pivotPoints.Count() < 500 ? data.pivotPoints.New() : 0;

    AddObjectErrors(errors);
    ReadObjectName(parse, helper->name);
    parse.Expect('{');

    LPCSTR tokenText;
    UINT   token = parse.Token(&tokenText, 0);
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
    ReadObjectEnd(errors, data, helper, data.helpers.Count() - 1, 0x10000000);
    errors.Complete(status);
    return !parse.FoundError();
  }

  BOOL WriteHelpers(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
    if (!static_cast<LPCSTR>(data.model.animationFile)[0]) {
      int needObjIds = data.helpers.Count() != data.objects.Count();
      for (UINT i = 0; i < data.helpers.Count(); ++i) {
        WriteObjectHeader(data, data.helpers.Ptr()[i], 0x10F, needObjIds, buffer);
        WriteObjectTrailer(data.helpers.Ptr()[i], buffer);
      }
    }
    return 1;
  }

  BOOL WriteBinHelpers(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *status) {
    if (!static_cast<LPCSTR>(data.model.animationFile)[0] && data.helpers.Count()) {
      buf.AddDword('PLEH');
      UINT totalSize = 4;
      UINT i;
      for (i = 0; i < data.helpers.Count(); ++i) {
        totalSize += GetBinGenObjectSize(data.helpers.Ptr()[i]);
      }
      buf.AddUint(totalSize);
      buf.AddUint(data.helpers.Count());
      for (i = 0; i < data.helpers.Count(); ++i) {
        WriteBinGenObject(data.helpers.Ptr()[i], buf, status);
      }
    }
    return 1;
  }

  BOOL ReadBinHelpers(CMsgBuffer &buffer, UINT length, MDLDATA &data, CMDLStatus *status) {
    UINT totalRead = 4;
    UINT count = buffer.GetUint();
    data.helpers.SetCount(0);
    data.helpers.ReserveSpace(count);

    while (totalRead < length) {
      MDLGENOBJECT *helper = data.helpers.New();
      if (!helper) {
        status->FatalFlunked("Helper", -1);
        return 0;
      }
      if (!ReadBinGenObject(*helper, buffer, status, totalRead)) {
        status->Add(STATUS_ERROR, "Error reading gen object portion of helper.\n");
        return 0;
      }
      if (totalRead > length) {
        status->FatalOverran("Helper section overran read buffer.\n", -1);
        return 0;
      }
      ReadBinObjectEnd(data, helper, data.helpers.Count() - 1, 0x10000000);
    }
    return 1;
  }

}  // namespace MDL
