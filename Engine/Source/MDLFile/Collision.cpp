#include "MDLTypes.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

#include <stpl.h>

void ReadVertices(Parser &, LPCSTR, TSGrowableArray<NTempest::C3Vector> *);
void WriteVertices(const TSGrowableArray<NTempest::C3Vector> &, UINT, TSGrowableArray<char> &);
void WriteBinC3VectorSection(CMsgBuffer &, DWORD, const TSGrowableArray<NTempest::C3Vector> &);
int  ReadBinC3VectorSection(CMsgBuffer &, DWORD, LPCSTR, TSGrowableArray<NTempest::C3Vector> *, UINT *, CMDLStatus *);

namespace MDL {
  LPCSTR       TokenText(UINT token);
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);
  int          ReadCollision(Parser &, MDLDATA &, CMDLStatus *);
  int          WriteCollision(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  int          ReadBinCollision(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  int          WriteBinCollision(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
}  // namespace MDL

static void ICollisionAddErrors(TSet &errors) {
  errors.Add(0x1D8, 1, 0);
  errors.Add(0x1C9, 1, 0);
  errors.Add(0x17B, 1, 0);
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
    *triIndices->New() = static_cast<WORD>(parse.ExpectInt());
    parse.Expect(',');
    *triIndices->New() = static_cast<WORD>(parse.ExpectInt());
    parse.Expect(',');
    *triIndices->New() = static_cast<WORD>(parse.ExpectInt());
    parse.Expect('}');
    parse.Expect(',');
    ++actual;
    savedtoken = parse.Token(&tokentext, 0);
  }
  parse.Expect('}', savedtoken, tokentext);
  if (count >= 0 && actual != count) {
    parse.WarningCount("collision triangles", count, actual);
  }
}

static void IWriteTriangleIndices(const TSGrowableArray<WORD> &triIndices, TSGrowableArray<char> &buffer) {
  UINT numTriangles = triIndices.Count() / 3;
  MDL::WriteLine(buffer, "\t%s %u {\n", MDL::TokenText(0x1C9), numTriangles);
  for (UINT i = 0; i < numTriangles; ++i) {
    MDL::WriteLine(buffer, "\t\t{ %hu, %hu, %hu },\n", triIndices[i * 3], triIndices[i * 3 + 1], triIndices[i * 3 + 2]);
  }
  MDL::WriteLine(buffer, "\t}\n");
}

static UINT GetSectionSize(const MDLCOLLISION &collision) {
  return 24 + 12 * (collision.vertices.Count() + collision.facetNormals.Count()) + 2 * collision.triIndices.Count();
}

int MDL::ReadCollision(Parser &parse, MDLDATA &data, CMDLStatus *status) {
  TSet errors;
  ICollisionAddErrors(errors);
  parse.Expect('{');
  LPCSTR tokentext;
  UINT   token = parse.Token(&tokentext, 0);
  while (token && token != '}') {
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokentext);
    }
    if (token == 0x1D8) {
      ReadVertices(parse, "collision vertices", &data.collision.vertices);
    } else if (token == 0x1C9) {
      IReadTriangleIndices(parse, &data.collision.triIndices);
    } else if (token == 0x17B) {
      ReadVertices(parse, "facet normals", &data.collision.facetNormals);
    } else {
      parse.FatalUnexpected(tokentext);
    }
    token = parse.Token(&tokentext, 0);
  }
  parse.Expect('}', token, tokentext);
  errors.Complete(status);
  return !parse.FoundError();
}

int MDL::WriteCollision(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
  if (data.collision.vertices.Count()) {
    WriteLine(buffer, "%s {\n", TokenText(0x119));
    WriteVertices(data.collision.vertices, 0x1D8, buffer);
    IWriteTriangleIndices(data.collision.triIndices, buffer);
    WriteVertices(data.collision.facetNormals, 0x17B, buffer);
    WriteLine(buffer, "}\n");
  }
  return 1;
}

int MDL::ReadBinCollision(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status) {
  UINT totalRead = 0;
  if (!length) {
    return 1;
  }
  if (!ReadBinC3VectorSection(buf, 'XTRV', "Vertex", &data.collision.vertices, &totalRead, status)) {
    return 0;
  }
  if (buf.GetDword() != ' IRT') {
    status->Add(STATUS_ERROR, "Invalid %s section detected in model.\n", "Triangle Index");
    return 0;
  }
  UINT count = buf.GetUint();
  totalRead += 8;
  data.collision.triIndices.SetCount(count);
  if (count) {
    buf.GetWordArray(data.collision.triIndices.Ptr(), count);
    totalRead += 2 * count;
  }
  if (!ReadBinC3VectorSection(buf, 'SMRN', "Facet Normal", &data.collision.facetNormals, &totalRead, status)) {
    return 0;
  }
  if (totalRead > length) {
    status->FatalOverran("Collision Section", -1);
    return 0;
  }
  return totalRead >= length;
}

int MDL::WriteBinCollision(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *) {
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
