#include "MDLTypes.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "Base/MsgBuffer.h"

#include <stpl.h>

namespace MDL {
const char *TokenText(unsigned int token);
void __cdecl WriteLine(TSGrowableArray<char> &buffer, const char *format, ...);

static void IReadPivots(Parser &parse, TSGrowableArray<NTempest::C3Vector> *pivots) {
  unsigned int savedToken;
  const char *tokenText;
  long count = parse.GetOptionalInt(&savedToken, &tokenText, 0);
  if (count > 0) {
    pivots->ReserveSpace(count);
  }
  parse.Expect('{', savedToken, tokenText);

  long actual = 0;
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

int ReadPivotPoints(Parser &parse, MDLDATA &data, CMDLStatus *) {
  IReadPivots(parse, &data.pivotPoints);
  return !parse.FoundError();
}

int WritePivotPoints(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
  if (data.pivotPoints.Count()) {
    WriteLine(buffer, "%s %d {\n", TokenText(0x111), data.pivotPoints.Count());
    for (unsigned int i = 0; i < data.pivotPoints.Count(); ++i) {
      const NTempest::C3Vector &pivot = data.pivotPoints[i];
      WriteLine(buffer, "\t{ %g, %g, %g },\n", pivot.x, pivot.y, pivot.z);
    }
    WriteLine(buffer, "}\n");
  }
  return 1;
}

int WriteBinPivotPoints(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *) {
  if (data.pivotPoints.Count()) {
    buf.AddDword('TVIP');
    buf.AddUint(12 * data.pivotPoints.Count());
    buf.AddFloatArray(&data.pivotPoints.Ptr()->x, 3 * data.pivotPoints.Count());
  }
  return 1;
}

int ReadBinPivotPoints(
    CMsgBuffer &buf,
    unsigned int length,
    MDLDATA &data,
    CMDLStatus *status
) {
  data.pivotPoints.SetCount(0);
  data.pivotPoints.ReserveSpace(length / 12);

  unsigned int totalRead = 0;
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
}
