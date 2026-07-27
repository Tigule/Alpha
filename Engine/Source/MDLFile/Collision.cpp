#include "MDLTypes.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

#include <stpl.h>

void ReadVertices(Parser &, const char *, TSGrowableArray<NTempest::C3Vector> *);
void WriteVertices(
    const TSGrowableArray<NTempest::C3Vector> &, unsigned int, TSGrowableArray<char> &
);
void WriteBinC3VectorSection(
    CMsgBuffer &, unsigned long, const TSGrowableArray<NTempest::C3Vector> &
);
int ReadBinC3VectorSection(
    CMsgBuffer &, unsigned long, const char *, TSGrowableArray<NTempest::C3Vector> *,
    unsigned int *, CMDLStatus *
);

namespace MDL {
const char *TokenText(unsigned int token);
void __cdecl WriteLine(TSGrowableArray<char> &buffer, const char *format, ...);

static void ICollisionAddErrors(TSet &errors) {
  errors.Add(0x1D8, 1, 0);
  errors.Add(0x1C9, 1, 0);
  errors.Add(0x17B, 1, 0);
}

static void IReadTriangleIndices(Parser &parse, TSGrowableArray<unsigned short> *indices) {
  unsigned int token;
  const char *tokenText;
  long count = parse.GetOptionalInt(&token, &tokenText, 0);
  if (count > 0) {
    indices->Reserve(3 * count);
  }
  parse.Expect('{', token, tokenText);
  long actual = 0;
  token = parse.Token(&tokenText, 0);
  while (token == '{') {
    unsigned short value = static_cast<unsigned short>(parse.ExpectInt());
    indices->Add(&value);
    parse.Expect(',');
    value = static_cast<unsigned short>(parse.ExpectInt());
    indices->Add(&value);
    parse.Expect(',');
    value = static_cast<unsigned short>(parse.ExpectInt());
    indices->Add(&value);
    parse.Expect('}');
    parse.Expect(',');
    ++actual;
    token = parse.Token(&tokenText, 0);
  }
  parse.Expect('}', token, tokenText);
  if (count >= 0 && actual != count) {
    parse.WarningCount("collision triangles", count, actual);
  }
}

static void IWriteTriangleIndices(
    const TSGrowableArray<unsigned short> &indices,
    TSGrowableArray<char> &buffer
) {
  unsigned int count = indices.Count() / 3;
  WriteLine(buffer, "\t%s %u {\n", TokenText(0x1C9), count);
  for (unsigned int i = 0; i < count; ++i) {
    WriteLine(
        buffer, "\t\t{ %hu, %hu, %hu },\n",
        indices[i * 3], indices[i * 3 + 1], indices[i * 3 + 2]
    );
  }
  WriteLine(buffer, "\t}\n");
}

static unsigned int GetSectionSize(const MDLCOLLISION &collision) {
  return 24 + 12 * (collision.vertices.Count() + collision.facetNormals.Count())
      + 2 * collision.triIndices.Count();
}

int ReadCollision(Parser &parse, MDLDATA &data, CMDLStatus *status) {
  TSet errors;
  ICollisionAddErrors(errors);
  parse.Expect('{');
  const char *tokenText;
  unsigned int token = parse.Token(&tokenText, 0);
  while (token && token != '}') {
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokenText);
    }
    if (token == 0x1D8) {
      ReadVertices(parse, "collision vertices", &data.collision.vertices);
    } else if (token == 0x1C9) {
      IReadTriangleIndices(parse, &data.collision.triIndices);
    } else if (token == 0x17B) {
      ReadVertices(parse, "facet normals", &data.collision.facetNormals);
    } else {
      parse.FatalUnexpected(tokenText);
    }
    token = parse.Token(&tokenText, 0);
  }
  parse.Expect('}', token, tokenText);
  errors.Complete(status);
  return !parse.FoundError();
}

int WriteCollision(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
  if (data.collision.vertices.Count()) {
    WriteLine(buffer, "%s {\n", TokenText(0x119));
    WriteVertices(data.collision.vertices, 0x1D8, buffer);
    IWriteTriangleIndices(data.collision.triIndices, buffer);
    WriteVertices(data.collision.facetNormals, 0x17B, buffer);
    WriteLine(buffer, "}\n");
  }
  return 1;
}

int ReadBinCollision(
    CMsgBuffer &buffer,
    unsigned int length,
    MDLDATA &data,
    CMDLStatus *status
) {
  unsigned int totalRead = 0;
  if (!length) {
    return 1;
  }
  if (!ReadBinC3VectorSection(buffer, 'XTRV', "Vertex", &data.collision.vertices, &totalRead, status)) {
    return 0;
  }
  if (buffer.GetDword() != ' IRT') {
    status->Add(STATUS_ERROR, "Invalid %s section detected in model.\n", "Triangle Index");
    return 0;
  }
  unsigned int count = buffer.GetUint();
  totalRead += 8;
  data.collision.triIndices.SetCount(count);
  if (count) {
    buffer.GetWordArray(data.collision.triIndices.Ptr(), count);
    totalRead += 2 * count;
  }
  if (!ReadBinC3VectorSection(
          buffer, 'SMRN', "Facet Normal", &data.collision.facetNormals, &totalRead, status)) {
    return 0;
  }
  if (totalRead > length) {
    status->FatalOverran("Collision Section", -1);
    return 0;
  }
  return totalRead >= length;
}

int WriteBinCollision(const MDLDATA &data, CMsgBuffer &buffer, CMDLStatus *) {
  if (data.collision.vertices.Count()) {
    buffer.AddDword('DILC');
    buffer.AddUint(GetSectionSize(data.collision));
    WriteBinC3VectorSection(buffer, 'XTRV', data.collision.vertices);
    buffer.AddDword(' IRT');
    buffer.AddUint(data.collision.triIndices.Count());
    buffer.AddWordArray(data.collision.triIndices.Ptr(), data.collision.triIndices.Count());
    WriteBinC3VectorSection(buffer, 'SMRN', data.collision.facetNormals);
  }
  return 1;
}

}
