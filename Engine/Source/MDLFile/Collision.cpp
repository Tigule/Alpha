#include "MDLTypes.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

#include <stpl.h>

void ReadVertices(Parser &, LPCSTR, TSGrowableArray<NTempest::C3Vector> *);
void WriteVertices(const TSGrowableArray<NTempest::C3Vector> &, UINT, TSGrowableArray<char> &);
void WriteBinC3VectorSection(CMsgBuffer &, DWORD, const TSGrowableArray<NTempest::C3Vector> &);
BOOL ReadBinC3VectorSection(CMsgBuffer &, DWORD, LPCSTR, TSGrowableArray<NTempest::C3Vector> *, UINT *, CMDLStatus *);

namespace MDL {
  LPCSTR       TokenText(UINT token);
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);
  BOOL         ReadCollision(Parser &, MDLDATA &, CMDLStatus *);
  BOOL         WriteCollision(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL         ReadBinCollision(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  BOOL         WriteBinCollision(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
}  // namespace MDL

static void ICollisionAddErrors(TSet &errors) {
  errors.Add(MDLTOK_VERTICES, 1, 0);
  errors.Add(MDLTOK_TRIANGLES, 1, 0);
  errors.Add(MDLTOK_NORMALS, 1, 0);
}

static void IReadTriangleIndices(Parser &parse, TSGrowableArray<WORD> *triIndices) {
  UINT   savedtoken;
  LPCSTR tokentext;
  long   count = parse.GetOptionalInt(&savedtoken, &tokentext, 0);
  if (count > 0) {
    triIndices->ReserveSpace(3 * count);
  }
  parse.Expect('{', savedtoken, tokentext);
  long actual = 0;
  savedtoken = parse.Token(&tokentext, 0);
  while (savedtoken == '{') {
    *triIndices->New() = parse.ExpectInt();
    parse.Expect(',');
    *triIndices->New() = parse.ExpectInt();
    parse.Expect(',');
    *triIndices->New() = parse.ExpectInt();
    parse.Expect('}');
    parse.Expect(',');
    ++actual;
    savedtoken = parse.Token(&tokentext, 0);
  }
  parse.Expect('}', savedtoken, tokentext);
  if (count >= 0 && actual != count) {
    parse.WarningCount("collision triangle indices", count, actual);
  }
}

static void IWriteTriangleIndices(const TSGrowableArray<WORD> &triIndices, TSGrowableArray<char> &buffer) {
  UINT numTriangles = triIndices.Count() / 3;
  MDL::WriteLine(buffer, "\t%s %u {\n", MDL::TokenText(MDLTOK_TRIANGLES), numTriangles);
  for (UINT i = 0; i < numTriangles; ++i) {
    MDL::WriteLine(buffer, "\t\t{ %hu, %hu, %hu },\n", triIndices[i * 3], triIndices[i * 3 + 1], triIndices[i * 3 + 2]);
  }
  MDL::WriteLine(buffer, "\t}\n");
}

static UINT GetSectionSize(const MDLCOLLISION &collision) {
  return 24 + 12 * (collision.vertices.Count() + collision.facetNormals.Count()) + 2 * collision.triIndices.Count();
}

BOOL MDL::ReadCollision(Parser &parse, MDLDATA &data, CMDLStatus *status) {
  TSet errors;
  ICollisionAddErrors(errors);
  parse.Expect('{');
  LPCSTR tokentext;
  UINT   token;
  for (token = parse.Token(&tokentext, 0); token != '}'; token = parse.Token(&tokentext, 0)) {
    if (!token) {
      break;
    }
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokentext);
    }
    switch (token) {
      case MDLTOK_VERTICES:
        ReadVertices(parse, "vertices", &data.collision.vertices);
        break;
      case MDLTOK_TRIANGLES:
        IReadTriangleIndices(parse, &data.collision.triIndices);
        break;
      case MDLTOK_NORMALS:
        ReadVertices(parse, "normals", &data.collision.facetNormals);
        break;
      default:
        parse.FatalUnexpected(tokentext);
        break;
    }
  }
  parse.Expect('}', token, tokentext);
  errors.Complete(status);
  return !parse.FoundError();
}

BOOL MDL::WriteCollision(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
  if (data.collision.vertices.Count()) {
    WriteLine(buffer, "%s {\n", TokenText(MDLTOK_COLLISION));
    WriteVertices(data.collision.vertices, MDLTOK_VERTICES, buffer);
    IWriteTriangleIndices(data.collision.triIndices, buffer);
    WriteVertices(data.collision.facetNormals, MDLTOK_NORMALS, buffer);
    WriteLine(buffer, "}\n");
  }
  return 1;
}

BOOL MDL::ReadBinCollision(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status) {
  UINT totalRead = 0;
  while (totalRead < length) {
    if (!ReadBinC3VectorSection(buf, 'XTRV', "vertex", &data.collision.vertices, &totalRead, status)) {
      return 0;
    }
    if (buf.GetDword() != ' IRT') {
      status->Add(STATUS_ERROR, "Invalid %s section.\n", "triangle index");
      return 0;
    }
    totalRead += 4;
    data.collision.triIndices.SetCount(buf.GetUint());
    totalRead += 4;
    if (data.collision.triIndices.Count()) {
      buf.GetWordArray(data.collision.triIndices.Ptr(), data.collision.triIndices.Count());
      totalRead += 2 * data.collision.triIndices.Count();
    }
    if (!ReadBinC3VectorSection(buf, 'SMRN', "facet normal", &data.collision.facetNormals, &totalRead, status)) {
      return 0;
    }
    if (totalRead > length) {
      status->FatalOverran("Collision section overran read buffer.\n", -1);
      return 0;
    }
  }
  return 1;
}

BOOL MDL::WriteBinCollision(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *) {
  if (data.collision.vertices.Count()) {
    buf.AddDword('DILC');
    buf.AddUint(GetSectionSize(data.collision));
    WriteBinC3VectorSection(buf, 'XTRV', data.collision.vertices);
    buf.AddDword(' IRT');
    buf.AddUint(data.collision.triIndices.Count());
    buf.AddWordArray(data.collision.triIndices.Ptr(), data.collision.triIndices.Count());
    WriteBinC3VectorSection(buf, 'SMRN', data.collision.facetNormals);
  }
  return 1;
}
