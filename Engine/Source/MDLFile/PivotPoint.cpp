#include "MDLTypes.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "Base/MsgBuffer.h"

#include <stpl.h>


namespace MDL {
  LPCSTR       TokenText(UINT token);
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);
  BOOL         ReadPivotPoints(Parser &, MDLDATA &, CMDLStatus *);
  BOOL         WritePivotPoints(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL         WriteBinPivotPoints(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  BOOL         ReadBinPivotPoints(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
}  // namespace MDL

static void IReadPivots(Parser &parse, TSGrowableArray<NTempest::C3Vector> *pivots) {
  UINT   savedToken;
  LPCSTR tokenText;
  long   actual = 0;
  long   count = parse.GetOptionalInt(&savedToken, &tokenText, 0);
  if (count > 0) {
    pivots->ReserveSpace(count);
  }
  parse.Expect('{', savedToken, tokenText);

  savedToken = parse.Token(&tokenText, 0);
  while (savedToken == '{') {
    NTempest::C3Vector *pivot = pivots->New();
    pivot->x = parse.ExpectFloat();
    parse.Expect(',');
    pivot->y = parse.ExpectFloat();
    parse.Expect(',');
    pivot->z = parse.ExpectFloat();
    parse.Expect('}');
    parse.Expect(',');
    ++actual;
    savedToken = parse.Token(&tokenText, 0);
  }
  parse.Expect('}', savedToken, tokenText);
  if (count >= 0 && actual != count) {
    parse.WarningCount("pivot points", count, actual);
  }
}

BOOL MDL::ReadPivotPoints(Parser &parse, MDLDATA &data, CMDLStatus *) {
  IReadPivots(parse, &data.pivotPoints);
  return !parse.FoundError();
}

BOOL MDL::WritePivotPoints(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
  if (data.pivotPoints.Count()) {
    MDL::WriteLine(buffer, "%s %d {\n", MDL::TokenText(MDLTOK_PIVOTPOINTS), data.pivotPoints.Count());
    for (UINT i = 0; i < data.pivotPoints.Count(); ++i) {
      const NTempest::C3Vector &pivot = data.pivotPoints.Ptr()[i];
      MDL::WriteLine(buffer, "\t{ %g, %g, %g },\n", pivot.x, pivot.y, pivot.z);
    }
    MDL::WriteLine(buffer, "}\n");
  }
  return 1;
}

BOOL MDL::WriteBinPivotPoints(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *) {
  if (data.pivotPoints.Count()) {
    buf.AddDword('TVIP');
    UINT count = data.pivotPoints.Count();
    buf.AddUint(12 * count);
    buf.AddFloatArray(&data.pivotPoints.Ptr()->x, 3 * count);
  }
  return 1;
}

BOOL MDL::ReadBinPivotPoints(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status) {
  data.pivotPoints.SetCount(0);
  data.pivotPoints.ReserveSpace(length / 12);

  UINT totalRead = 0;
  while (totalRead < length) {
    NTempest::C3Vector *pivot = data.pivotPoints.New();
    if (!pivot) {
      status->FatalFlunked("Pivot", -1);
      return 0;
    }
    buf.GetFloatArray(&pivot->x, 3);
    totalRead += 12;
    if (totalRead > length) {
      status->FatalOverran("Pivot", -1);
      return 0;
    }
  }
  return 1;
}
