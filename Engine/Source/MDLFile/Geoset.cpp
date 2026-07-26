#include "MDLTypes.h"
#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

#include <stpl.h>

namespace MDL {
const char *__fastcall TokenText(unsigned int token);
void __cdecl WriteLine(TSGrowableArray<char> &buffer, const char *format, ...);
}

void __fastcall ReadVertices(
    Parser &parse,
    const char *title,
    TSGrowableArray<NTempest::C3Vector> *vertices
) {
  unsigned int token;
  const char *tokenText;
  long count = parse.GetOptionalInt(&token, &tokenText, 0);
  if (count > 0) {
    vertices->Reserve(count);
  }
  parse.Expect('{', token, tokenText);

  long actual = 0;
  token = parse.Token(&tokenText, 0);
  while (token == '{') {
    NTempest::C3Vector *vertex = vertices->New();
    vertex->x = parse.ExpectFloat();
    parse.Expect(',');
    vertex->y = parse.ExpectFloat();
    parse.Expect(',');
    vertex->z = parse.ExpectFloat();
    parse.Expect('}');
    parse.Expect(',');
    ++actual;
    token = parse.Token(&tokenText, 0);
  }
  parse.Expect('}', token, tokenText);
  if (count >= 0 && actual != count) {
    parse.WarningCount(title, count, actual);
  }
}

void __fastcall WriteVertices(
    const TSGrowableArray<NTempest::C3Vector> &vertices,
    unsigned int title,
    TSGrowableArray<char> &buffer
) {
  MDL::WriteLine(buffer, "\t%s %d {\n", MDL::TokenText(title), vertices.Count());
  for (unsigned int i = 0; i < vertices.Count(); ++i) {
    const NTempest::C3Vector &vertex = vertices[i];
    MDL::WriteLine(buffer, "\t\t{ %g, %g, %g },\n", vertex.x, vertex.y, vertex.z);
  }
  MDL::WriteLine(buffer, "\t}\n");
}

void __fastcall WriteBinC3VectorSection(
    CMsgBuffer &buffer,
    unsigned long title,
    const TSGrowableArray<NTempest::C3Vector> &section
) {
  buffer.AddDword(title);
  buffer.AddUint(section.Count());
  buffer.AddFloatArray(&section.Ptr()->x, 3 * section.Count());
}

int __fastcall ReadBinC3VectorSection(
    CMsgBuffer &buffer,
    unsigned long title,
    const char *name,
    TSGrowableArray<NTempest::C3Vector> *section,
    unsigned int *localBytesRead,
    CMDLStatus *status
) {
  if (buffer.GetDword() != title) {
    status->Add(STATUS_ERROR, "Invalid %s section detected in model.\n", name);
    return 0;
  }
  unsigned int count = buffer.GetUint();
  *localBytesRead += 8;
  section->SetCount(count);
  if (count) {
    *localBytesRead += 12 * count;
    buffer.GetFloatArray(&section->Ptr()->x, 3 * count);
  }
  return 1;
}

int __fastcall IReadBinUintSection(
    CMsgBuffer &buffer,
    unsigned long title,
    const char *name,
    TSGrowableArray<unsigned int> *section,
    unsigned int *localBytesRead,
    CMDLStatus *status
) {
  unsigned long found = buffer.GetDword();
  *localBytesRead += 4;
  if (found != title) {
    status->Add(STATUS_ERROR, "Invalid %s section.\n", name);
    return 0;
  }
  unsigned int count = buffer.GetUint();
  *localBytesRead += 4;
  if (count) {
    section->SetCount(count);
    buffer.GetUintArray(section->Ptr(), count);
    *localBytesRead += 4 * count;
  }
  return 1;
}

void __fastcall SetVertexGroupIndices(
    const TSGrowableArray<unsigned int> &groupVertexCounts,
    MDLGEOSETSECTION *geoset
) {
  FATALASSERT(geoset);
  geoset->vertGroupIndices.SetCount(geoset->vertices.Count());
  if (groupVertexCounts.Count() < 2) {
    if (geoset->vertGroupIndices.Count()) {
      memset(
          geoset->vertGroupIndices.Ptr(),
          0,
          geoset->vertGroupIndices.Count()
      );
    }
    return;
  }

  unsigned int vertex = 0;
  for (unsigned int group = 0; group < groupVertexCounts.Count(); ++group) {
    unsigned int count = groupVertexCounts[group];
    while (count-- && vertex < geoset->vertGroupIndices.Count()) {
      geoset->vertGroupIndices[vertex++] =
          static_cast<unsigned char>(group);
    }
  }
}

static void IGeosetAddErrors(TSet &errors) {
  errors.Add(0x1D8, 1, 0);
  errors.Add(0x17B, 0, 0);
  errors.Add(0x148, 1, 0);
  errors.Add(0x156, 1, 0);
  errors.Add(0x16D, 0, 0);
  errors.Add(0x170, 0, 0);
  errors.Add(0x16F, 0, 0);
  errors.Add(0x134, 0, 0);
  errors.Add(0x1B1, 0, 0);
  errors.Add(0x1D2, 0, 0);
  errors.Add(0x131, 0, 0);
  errors.Add(0x132, 0, 0);
}

static void IReadTVertices(
    Parser &parse,
    TSGrowableArray<NTempest::C2Vector> *texcoords
) {
  unsigned int token;
  const char *tokenText;
  long count = parse.GetOptionalInt(&token, &tokenText, 0);
  if (count > 0) {
    texcoords->Reserve(count);
  }
  parse.Expect('{', token, tokenText);
  long actual = 0;
  token = parse.Token(&tokenText, 0);
  while (token == '{') {
    NTempest::C2Vector *coord = texcoords->New();
    coord->x = parse.ExpectFloat();
    parse.Expect(',');
    coord->y = parse.ExpectFloat();
    parse.Expect('}');
    parse.Expect(',');
    ++actual;
    token = parse.Token(&tokenText, 0);
  }
  parse.Expect('}', token, tokenText);
  if (count >= 0 && count != actual) {
    parse.WarningCount("vertices", count, actual);
  }
}

static void ISkipDuplicates(Parser &parse) {
  unsigned int token;
  const char *tokenText;
  parse.GetOptionalInt(&token, &tokenText, 0);
  parse.Expect('{', token, tokenText);
  token = parse.Token(&tokenText, 0);
  while (token && token != '}') {
    if (token != 0x100) {
      parse.FatalUnexpected(tokenText);
    }
    parse.Expect(',');
    token = parse.Token(&tokenText, 0);
  }
  parse.Expect('}', token, tokenText);
}

static unsigned int IVertexList(
    Parser &parse,
    TSGrowableArray<unsigned short> *vertlist
) {
  FATALASSERT(vertlist);
  vertlist->New()[0] = static_cast<unsigned short>(parse.ExpectInt());
  unsigned int entries = 1;
  const char *tokenText;
  unsigned int token = parse.Token(&tokenText, 0);
  while (token == ',') {
    vertlist->New()[0] = static_cast<unsigned short>(parse.ExpectInt());
    ++entries;
    token = parse.Token(&tokenText, 0);
  }
  parse.Expect('}', token, tokenText);
  return entries;
}

static unsigned int IVertexListSet(
    Parser &parse,
    MDLPRIMITIVES *primitives,
    unsigned char type
) {
  *primitives->types.New() = type;
  unsigned int *count = primitives->counts.New();
  *count = IVertexList(parse, &primitives->vertices);
  parse.Expect(',');
  return *count;
}

static unsigned int IPrimitives(
    Parser &parse,
    MDLPRIMITIVES *primitives,
    long *entries,
    unsigned char type,
    int (__fastcall *IsInvalid)(unsigned int),
    const char *errorText
) {
  unsigned int added = 0;
  parse.Expect('{');
  const char *tokenText;
  unsigned int token = parse.Token(&tokenText, 0);
  while (token == '{') {
    unsigned int count = IVertexListSet(parse, primitives, type);
    if (IsInvalid(count)) {
      parse.FatalNotFound(errorText);
    }
    *entries += count;
    ++added;
    token = parse.Token(&tokenText, 0);
  }
  parse.Expect('}', token, tokenText);
  return added;
}

static int __fastcall NeverInvalid(unsigned int) {
  return 0;
}

static int __fastcall InvalidLines(unsigned int count) {
  return count & 1;
}

static int __fastcall InvalidLineStripLoop(unsigned int count) {
  return count < 2;
}

static int __fastcall InvalidTriangles(unsigned int count) {
  return count % 3;
}

static int __fastcall InvalidTriangleFanStrip(unsigned int count) {
  return count < 3;
}

static int __fastcall InvalidQuads(unsigned int count) {
  return count & 3;
}

static int __fastcall InvalidQuadStrip(unsigned int count) {
  return count < 4 || (count & 1);
}

static unsigned int IMultiPoints(
    Parser &parse,
    MDLPRIMITIVES *primitives,
    long *entries,
    unsigned char type
) {
  return IPrimitives(
      parse, primitives, entries, type, NeverInvalid, 0
  );
}

static unsigned int ILines(
    Parser &parse,
    MDLPRIMITIVES *primitives,
    long *entries
) {
  return IPrimitives(
      parse,
      primitives,
      entries,
      1,
      InvalidLines,
      "an even number of entries"
  );
}

static unsigned int ILineStripLoop(
    Parser &parse,
    MDLPRIMITIVES *primitives,
    long *entries,
    unsigned char type
) {
  return IPrimitives(
      parse,
      primitives,
      entries,
      type,
      InvalidLineStripLoop,
      "at least two entries"
  );
}

static unsigned int ITriangles(
    Parser &parse,
    MDLPRIMITIVES *primitives,
    long *entries
) {
  return IPrimitives(
      parse,
      primitives,
      entries,
      4,
      InvalidTriangles,
      "a multiple of three entries"
  );
}

static unsigned int ITriangleFanStrip(
    Parser &parse,
    MDLPRIMITIVES *primitives,
    long *entries,
    unsigned char type
) {
  return IPrimitives(
      parse,
      primitives,
      entries,
      type,
      InvalidTriangleFanStrip,
      "at least three entries"
  );
}

static unsigned int IQuads(
    Parser &parse,
    MDLPRIMITIVES *primitives,
    long *entries
) {
  return IPrimitives(
      parse,
      primitives,
      entries,
      7,
      InvalidQuads,
      "a multiple of four entries"
  );
}

static unsigned int IQuadStrip(
    Parser &parse,
    MDLPRIMITIVES *primitives,
    long *entries
) {
  return IPrimitives(
      parse,
      primitives,
      entries,
      8,
      InvalidQuadStrip,
      "at least four and a multiple of two entries"
  );
}

static void IReadPrimitives(Parser &parse, MDLPRIMITIVES *primitives) {
  unsigned int token;
  const char *tokenText;
  UTokenData tokenData;
  long estimatedPrimitives =
      parse.GetOptionalInt(&token, &tokenText, &tokenData);
  long estimatedVertices = -1;
  if (estimatedPrimitives > 0) {
    estimatedVertices = parse.GetOptionalInt(
        token, &tokenData, &token, &tokenText
    );
    primitives->ReserveSpace(
        estimatedPrimitives,
        estimatedVertices > 0
            ? estimatedVertices
            : 3 * estimatedPrimitives
    );
  }
  parse.Expect('{', token, tokenText);
  long actualVertices = 0;
  long actualPrimitives = 0;
  token = parse.Token(&tokenText, 0);
  while (token && token != '}') {
    switch (token) {
      case 0x168:
        actualPrimitives += ILines(parse, primitives, &actualVertices);
        break;
      case 0x16A:
        actualPrimitives +=
            ILineStripLoop(parse, primitives, &actualVertices, 2);
        break;
      case 0x16B:
        actualPrimitives +=
            ILineStripLoop(parse, primitives, &actualVertices, 3);
        break;
      case 0x1A3:
        actualPrimitives +=
            IMultiPoints(parse, primitives, &actualVertices, 0);
        break;
      case 0x1A4:
        actualPrimitives +=
            IMultiPoints(parse, primitives, &actualVertices, 9);
        break;
      case 0x1A8:
        actualPrimitives += IQuads(parse, primitives, &actualVertices);
        break;
      case 0x1A9:
        actualPrimitives += IQuadStrip(
            parse, primitives, &actualVertices
        );
        break;
      case 0x1C9:
        actualPrimitives +=
            ITriangles(parse, primitives, &actualVertices);
        break;
      case 0x1CA:
        actualPrimitives +=
            ITriangleFanStrip(parse, primitives, &actualVertices, 5);
        break;
      case 0x1CB:
        actualPrimitives +=
            ITriangleFanStrip(parse, primitives, &actualVertices, 6);
        break;
      default:
        parse.FatalUnexpected(tokenText);
        break;
    }
    token = parse.Token(&tokenText, 0);
  }
  parse.Expect('}', token, tokenText);
  if (estimatedVertices >= 0 && estimatedVertices != actualVertices) {
    parse.WarningCount(
        "primitive vertices", estimatedVertices, actualVertices
    );
  }
  if (estimatedPrimitives >= 0
      && estimatedPrimitives != actualPrimitives) {
    parse.WarningCount(
        "primitives", estimatedPrimitives, actualPrimitives
    );
  }
}

static void IReadMatrices(
    Parser &parse,
    unsigned int *mtxCount,
    TSGrowableArray<unsigned int> *matrixIdList
) {
  FATALASSERT(mtxCount);
  FATALASSERT(matrixIdList);
  *mtxCount = 0;
  parse.Expect('{');
  *matrixIdList->New() = parse.ExpectInt();
  ++*mtxCount;
  const char *tokenText;
  unsigned int token = parse.Token(&tokenText, 0);
  while (token == ',') {
    *matrixIdList->New() = parse.ExpectInt();
    ++*mtxCount;
    token = parse.Token(&tokenText, 0);
  }
  parse.Expect('}', token, tokenText);
}

static void IReadGroup(
    Parser &parse,
    MDLGEOSETSECTION *geoset,
    long *numMatrices,
    TSGrowableArray<unsigned int> *groupVertexCounts,
    CMDLStatus *status
) {
  TSet errors;
  errors.Add(0x1D6, 1, 0);
  errors.Add(0x16E, 1, 0);
  unsigned int *vertexCount = groupVertexCounts->New();
  *vertexCount = 0;
  unsigned int *matrixCount = geoset->groupMatrixCounts.New();
  *matrixCount = 0;
  parse.Expect('{');
  const char *tokenText;
  unsigned int token = parse.Token(&tokenText, 0);
  while (token && token != '}') {
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokenText);
    }
    if (token == 0x16E) {
      IReadMatrices(parse, matrixCount, &geoset->matrices);
      *numMatrices += *matrixCount;
    } else if (token == 0x1D6) {
      *vertexCount = parse.ExpectInt();
    } else {
      parse.FatalUnexpected(tokenText);
    }
    parse.Expect(',');
    token = parse.Token(&tokenText, 0);
  }
  parse.Expect('}', token, tokenText);
  errors.Complete(status);
}

static void IReadGroups(
    Parser &parse,
    MDLGEOSETSECTION *geoset,
    CMDLStatus *status
) {
  unsigned int token;
  const char *tokenText;
  UTokenData tokenData;
  long estimatedGroups =
      parse.GetOptionalInt(&token, &tokenText, &tokenData);
  long estimatedMatrices = -1;
  if (estimatedGroups > 0) {
    estimatedMatrices = parse.GetOptionalInt(
        token, &tokenData, &token, &tokenText
    );
    geoset->groupMatrixCounts.Reserve(estimatedGroups);
    if (estimatedMatrices > 0) {
      geoset->matrices.Reserve(estimatedMatrices);
    }
  }
  parse.Expect('{', token, tokenText);
  long actualGroups = 0;
  long actualMatrices = 0;
  TSGrowableArray<unsigned int> groupVertexCounts;
  token = parse.Token(&tokenText, 0);
  while (token && token != '}') {
    if (token == 0x155) {
      IReadGroup(
          parse,
          geoset,
          &actualMatrices,
          &groupVertexCounts,
          status
      );
    } else if (token == 0x16E) {
      unsigned int *count = geoset->groupMatrixCounts.New();
      IReadMatrices(parse, count, &geoset->matrices);
      actualMatrices += *count;
    } else {
      parse.FatalUnexpected(tokenText);
    }
    ++actualGroups;
    parse.Expect(',');
    token = parse.Token(&tokenText, 0);
  }
  parse.Expect('}', token, tokenText);
  if (groupVertexCounts.Count()) {
    SetVertexGroupIndices(groupVertexCounts, geoset);
  }
  if (estimatedGroups >= 0 && estimatedGroups != actualGroups) {
    parse.WarningCount("groups", estimatedGroups, actualGroups);
  }
  if (estimatedMatrices >= 0 && estimatedMatrices != actualMatrices) {
    parse.WarningCount(
        "matrices", estimatedMatrices, actualMatrices
    );
  }
}

static void IReadBoneWeights(Parser &parse, MDLGEOSETSECTION *geoset) {
  unsigned int count = parse.ExpectInt();
  geoset->boneWeights.Reserve(count);
  parse.Expect('{');
  for (unsigned int i = 0; i < count; ++i) {
    *geoset->boneWeights.New() = parse.ExpectInt();
    parse.Expect(',');
  }
  parse.Expect('}');
}

static void IReadBoneIndices(Parser &parse, MDLGEOSETSECTION *geoset) {
  unsigned int count = parse.ExpectInt();
  geoset->boneIndices.Reserve(count);
  parse.Expect('{');
  for (unsigned int i = 0; i < count; ++i) {
    *geoset->boneIndices.New() = parse.ExpectInt();
    parse.Expect(',');
  }
  parse.Expect('}');
}

static void IReadVertexGroupIds(
    Parser &parse,
    MDLGEOSETSECTION *geoset
) {
  geoset->vertGroupIndices.Reserve(geoset->vertices.Count());
  parse.Expect('{');
  const char *tokenText;
  UTokenData tokenData;
  unsigned int token = parse.Token(&tokenText, &tokenData);
  while (token && token != '}') {
    if (token == 0x100) {
      *geoset->vertGroupIndices.New() =
          static_cast<unsigned char>(tokenData.cVal);
    } else {
      parse.FatalUnexpected(tokenText);
    }
    parse.Expect(',');
    token = parse.Token(&tokenText, &tokenData);
  }
  parse.Expect('}', token, tokenText);
}

static void IReadVertex(Parser &parse, NTempest::C3Vector *vertex) {
  parse.Expect('{');
  vertex->x = parse.ExpectFloat();
  parse.Expect(',');
  vertex->y = parse.ExpectFloat();
  parse.Expect(',');
  vertex->z = parse.ExpectFloat();
  parse.Expect('}');
}

static void IAnimBoundsAddErrors(TSet &errors) {
  errors.Add(0x170, 0, 0);
  errors.Add(0x16F, 0, 0);
  errors.Add(0x134, 0, 0);
}

static void IReadAnimBounds(
    Parser &parse,
    CMdlBounds *bounds,
    CMDLStatus *status
) {
  TSet errors;
  IAnimBoundsAddErrors(errors);
  parse.Expect('{');
  const char *tokenText;
  unsigned int token = parse.Token(&tokenText, 0);
  while (token && token != '}') {
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokenText);
    }
    if (token == 0x170) {
      IReadVertex(parse, &bounds->extent.b);
    } else if (token == 0x16F) {
      IReadVertex(parse, &bounds->extent.t);
    } else if (token == 0x134) {
      bounds->radius = parse.ExpectFloat();
    } else {
      parse.FatalUnexpected(tokenText);
    }
    parse.Expect(',');
    token = parse.Token(&tokenText, 0);
  }
  parse.Expect('}', token, tokenText);
  errors.Complete(status);
}

static int ValidateVertexCounts(
    const MDLGEOSETSECTION &geoset,
    Parser &parse,
    CMDLStatus *status
) {
  unsigned int count = geoset.vertices.Count();
  if (count != geoset.normals.Count()
      || count != geoset.vertGroupIndices.Count()) {
    status->Add(
        STATUS_FATAL,
        "Error (line %d): Vertex count doesn't match normals and group indices\n",
        parse.GetLineNumber()
    );
    return 0;
  }
  for (unsigned int i = 0; i < geoset.texCoords.Count(); ++i) {
    if (geoset.texCoords[i].Count() != count) {
      status->Add(
          STATUS_FATAL,
          "Error (line %d): Vertex count doesn't match texture coordinates\n",
          parse.GetLineNumber()
      );
      return 0;
    }
  }
  return 1;
}

static void IGeosetAnimAddErrors(TSet &errors) {
  errors.Add(0x11C, 0, 0);
  errors.Add(0x136, 0, 0);
  errors.Add(0x150, 0, 0);
}

static int IReadAlpha(
    Parser &parse,
    int expectAnimation,
    MDLGEOSETANIMSECTION *geoset
) {
  if (expectAnimation) {
    ReadObjectFloatKeyframes(parse, &geoset->alphaKeys);
    return 1;
  }
  geoset->staticAlpha = parse.ExpectFloat();
  return 0;
}

static int IReadColor(
    Parser &parse,
    int expectAnimation,
    MDLGEOSETANIMSECTION *geoset
) {
  if (expectAnimation) {
    ReadObjectFloatKeyframes(parse, &geoset->colorKeys);
    return 1;
  }
  ReadFloatKeyData(parse, &geoset->staticColor.b, 3);
  return 0;
}

static int IllegalStaticToken(unsigned int token) {
  return token != 0x11C && token != 0x136 && token != 0x189;
}

static int IReadGeosetAnim(
    Parser &parse,
    unsigned int savedToken,
    const char *tokenText,
    TSet *errors,
    MDLGEOSETANIMSECTION *geoAnim
) {
  int expectAnimation = IExpectAnimation(
      parse,
      &savedToken,
      &tokenText
  );
  if (!expectAnimation && IllegalStaticToken(savedToken)) {
    parse.FatalUnexpected(tokenText);
  }
  if (!errors->Check(savedToken)) {
    parse.FatalDuplicate(tokenText);
  }
  if (!geoAnim) {
    return 0;
  }
  if (savedToken == 0x11C || savedToken == 0x189) {
    if (!IReadAlpha(parse, expectAnimation, geoAnim)) {
      parse.Expect(',');
    }
    return 1;
  }
  if (savedToken == 0x136) {
    geoAnim->flags |= 1;
    if (!IReadColor(parse, expectAnimation, geoAnim)) {
      parse.Expect(',');
    }
    return 1;
  }
  return 0;
}

static void IWriteGeosetTexCoords(
    const TSGrowableArray<NTempest::C2Vector> &texcoords,
    TSGrowableArray<char> &buffer
) {
  if (!texcoords.Count()) {
    return;
  }
  MDL::WriteLine(
      buffer,
      "\t%s %d {\n",
      MDL::TokenText(0x1CE),
      texcoords.Count()
  );
  for (unsigned int i = 0; i < texcoords.Count(); ++i) {
    MDL::WriteLine(
        buffer,
        "\t\t{ %g, %g },\n",
        texcoords[i].x,
        texcoords[i].y
    );
  }
  MDL::WriteLine(buffer, "\t}\n");
}

static void IWriteVertexGroupIndices(
    const TSGrowableArray<unsigned char> &indices,
    TSGrowableArray<char> &buffer
) {
  MDL::WriteLine(buffer, "\t%s {\n", MDL::TokenText(0x1D7));
  for (unsigned int i = 0; i < indices.Count(); ++i) {
    MDL::WriteLine(buffer, "\t\t%u,\n", indices[i]);
  }
  MDL::WriteLine(buffer, "\t}\n");
}

static const char *IGetPrimitiveText(unsigned char type) {
  switch (type) {
    case 0: return MDL::TokenText(0x1A3);
    case 1: return MDL::TokenText(0x168);
    case 2: return MDL::TokenText(0x16A);
    case 3: return MDL::TokenText(0x16B);
    case 4: return MDL::TokenText(0x1C9);
    case 5: return MDL::TokenText(0x1CB);
    case 6: return MDL::TokenText(0x1CA);
    case 7: return MDL::TokenText(0x1A8);
    case 8: return MDL::TokenText(0x1A9);
    case 9: return MDL::TokenText(0x1A4);
    default: return MDL::TokenText(0x1DF);
  }
}

static void IWriteGeosetPrimitives(
    const MDLPRIMITIVES &faces,
    TSGrowableArray<char> &buffer
) {
  MDL::WriteLine(
      buffer,
      "\t%s %d %d {\n",
      MDL::TokenText(0x148),
      faces.types.Count(),
      faces.vertices.Count()
  );
  for (unsigned int type = 0; type < 10; ++type) {
    const unsigned short *vertex = faces.vertices.Ptr();
    int opened = 0;
    for (unsigned int i = 0; i < faces.types.Count(); ++i) {
      if (faces.types[i] == type) {
        if (!opened) {
          MDL::WriteLine(
              buffer, "\t\t%s {\n", IGetPrimitiveText(type)
          );
          opened = 1;
        }
        MDL::WriteLine(buffer, "\t\t\t{ %d", vertex[0]);
        for (unsigned int j = 1; j < faces.counts[i]; ++j) {
          MDL::WriteLine(buffer, ", %d", vertex[j]);
        }
        MDL::WriteLine(buffer, " },\n");
      }
      vertex += faces.counts[i];
    }
    if (opened) {
      MDL::WriteLine(buffer, "\t\t}\n");
    }
  }
  MDL::WriteLine(buffer, "\t}\n");
}

static void IWriteGeosetGroups(
    const MDLGEOSETSECTION &section,
    TSGrowableArray<char> &buffer
) {
  MDL::WriteLine(
      buffer,
      "\t%s %u %u {\n",
      MDL::TokenText(0x156),
      section.groupMatrixCounts.Count(),
      section.matrices.Count()
  );
  const unsigned int *matrix = section.matrices.Ptr();
  for (unsigned int i = 0; i < section.groupMatrixCounts.Count(); ++i) {
    MDL::WriteLine(buffer, "\t\t%s { ", MDL::TokenText(0x16E));
    for (unsigned int j = 0; j < section.groupMatrixCounts[i]; ++j) {
      MDL::WriteLine(buffer, "%u", *matrix++);
      if (j + 1 < section.groupMatrixCounts[i]) {
        MDL::WriteLine(buffer, ", ");
      }
    }
    MDL::WriteLine(buffer, " },\n");
  }
  MDL::WriteLine(buffer, "\t}\n");
}

static void IWriteBoneWeights(
    const MDLGEOSETSECTION &section,
    TSGrowableArray<char> &buffer
) {
  unsigned int count = section.boneIndices.Count();
  MDL::WriteLine(
      buffer, "\t%s %u {\n", MDL::TokenText(0x131), count
  );
  for (unsigned int i = 0; i < count; ++i) {
    MDL::WriteLine(buffer, "\t\t0x%08X,\n", section.boneIndices[i]);
  }
  MDL::WriteLine(buffer, "\t}\n");
  MDL::WriteLine(
      buffer, "\t%s %u {\n", MDL::TokenText(0x132), count
  );
  for (unsigned int j = 0; j < count; ++j) {
    MDL::WriteLine(buffer, "\t\t0x%08X,\n", section.boneWeights[j]);
  }
  MDL::WriteLine(buffer, "\t}\n");
}

static void IWriteAnimBounds(
    const TSGrowableArray<CMdlBounds> &bounds,
    TSGrowableArray<char> &buffer
) {
  for (unsigned int i = 0; i < bounds.Count(); ++i) {
    MDL::WriteLine(buffer, "\t%s {\n", MDL::TokenText(0x122));
    WriteBounds(bounds[i], "\t\t", buffer);
    MDL::WriteLine(buffer, "\t}\n");
  }
}

static void IWriteGeosetSection(
    const MDLGEOSETSECTION &section,
    int writeMaterialId,
    TSGrowableArray<char> &buffer
) {
  MDL::WriteLine(buffer, "%s {\n", MDL::TokenText(0x10A));
  WriteVertices(section.vertices, 0x1D8, buffer);
  WriteVertices(section.normals, 0x17B, buffer);
  for (unsigned int i = 0; i < section.texCoords.Count(); ++i) {
    IWriteGeosetTexCoords(section.texCoords[i], buffer);
  }
  IWriteVertexGroupIndices(section.vertGroupIndices, buffer);
  IWriteGeosetPrimitives(section.primitives, buffer);
  IWriteGeosetGroups(section, buffer);
  IWriteBoneWeights(section, buffer);
  WriteBounds(section.bounds, "\t", buffer);
  IWriteAnimBounds(section.seqBounds, buffer);
  if (writeMaterialId) {
    MDL::WriteLine(
        buffer,
        "\t%s %d,\n",
        MDL::TokenText(0x16D),
        section.materialId
    );
  }
  MDL::WriteLine(
      buffer,
      "\t%s %d,\n",
      MDL::TokenText(0x1B1),
      section.selectionGroup
  );
  if (section.flags & 1) {
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(0x1D2));
  }
  MDL::WriteLine(buffer, "}\n");
}

namespace MDL {

int __fastcall ReadGeoset(
    Parser &parse,
    MDLDATA &data,
    CMDLStatus *status
) {
  MDLGEOSETSECTION *geoset = data.geosets.New();
  MDLGEOSETANIMSECTION *geosetAnim = 0;
  TSet geosetAnimErrors;
  if (data.version < 600) {
    geosetAnim = data.geosetAnims.New();
    geosetAnim->geosetId = data.geosets.Count() - 1;
    IGeosetAnimAddErrors(geosetAnimErrors);
  }
  geoset->seqBounds.Reserve(data.sequences.Count());

  TSet errors;
  IGeosetAddErrors(errors);
  parse.Expect('{');
  const char *tokenText;
  unsigned int token = parse.Token(&tokenText, 0);
  while (token && token != '}') {
    if (IReadGeosetAnim(
            parse,
            token,
            tokenText,
            &geosetAnimErrors,
            geosetAnim
        )) {
      token = parse.Token(&tokenText, 0);
      continue;
    }
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokenText);
    }
    switch (token) {
      case 0x122:
        IReadAnimBounds(parse, geoset->seqBounds.New(), status);
        break;
      case 0x131:
        IReadBoneIndices(parse, geoset);
        break;
      case 0x132:
        IReadBoneWeights(parse, geoset);
        break;
      case 0x134:
        geoset->bounds.radius = parse.ExpectFloat();
        break;
      case 0x142:
        ISkipDuplicates(parse);
        break;
      case 0x148:
        IReadPrimitives(parse, &geoset->primitives);
        break;
      case 0x156:
        IReadGroups(parse, geoset, status);
        break;
      case 0x16D:
        geoset->materialId = parse.ExpectInt();
        break;
      case 0x16F:
        IReadVertex(parse, &geoset->bounds.extent.t);
        break;
      case 0x170:
        IReadVertex(parse, &geoset->bounds.extent.b);
        break;
      case 0x17B:
        ReadVertices(parse, "normals", &geoset->normals);
        break;
      case 0x1B1:
        geoset->selectionGroup = parse.ExpectInt();
        break;
      case 0x1CE:
        IReadTVertices(parse, geoset->texCoords.New());
        break;
      case 0x1D2:
        geoset->flags |= 1;
        break;
      case 0x1D7:
        IReadVertexGroupIds(parse, geoset);
        break;
      case 0x1D8:
        ReadVertices(parse, "vertices", &geoset->vertices);
        break;
      default:
        parse.FatalUnexpected(tokenText);
        break;
    }
    parse.Expect(',');
    token = parse.Token(&tokenText, 0);
  }
  parse.Expect('}', token, tokenText);
  errors.Complete(status);
  geoset->seqBounds.TrimUnusedSpace();
  return ValidateVertexCounts(*geoset, parse, status)
      && !parse.FoundError();
}

int __fastcall WriteGeosets(
    const MDLDATA &data,
    TSGrowableArray<char> &buffer,
    CMDLStatus *
) {
  int writeMaterialId = data.materials.Count() != 0;
  for (unsigned int i = 0; i < data.geosets.Count(); ++i) {
    IWriteGeosetSection(data.geosets[i], writeMaterialId, buffer);
  }
  return 1;
}

int __fastcall ReadGeosetAnim(
    Parser &parse,
    MDLDATA &data,
    CMDLStatus *status
) {
  TSet errors;
  MDLGEOSETANIMSECTION *section = data.geosetAnims.New();
  section->geosetId = data.geosetAnims.Count() - 1;
  IGeosetAnimAddErrors(errors);
  parse.Expect('{');
  const char *tokenText;
  unsigned int token = parse.Token(&tokenText, 0);
  while (token && token != '}') {
    if (!IReadGeosetAnim(parse, token, tokenText, &errors, section)) {
      if (token == 0x150) {
        section->geosetId = parse.ExpectInt();
      } else {
        parse.FatalUnexpected(tokenText);
      }
      parse.Expect(',');
    }
    token = parse.Token(&tokenText, 0);
  }
  parse.Expect('}', token, tokenText);
  errors.Complete(status);
  return !parse.FoundError();
}

static void IWriteGeosetAnimSection(
    const MDLGEOSETANIMSECTION &section,
    TSGrowableArray<char> &buffer
) {
  WriteLine(buffer, "%s {\n", TokenText(0x10B));
  const char *indent = "\t";
  if (section.alphaKeys.keys.Count()) {
    const MDLKEYTRACK<float> &track = section.alphaKeys;
    WriteLine(
        buffer,
        "%s%s %d {\n",
        indent,
        TokenText(0x11C),
        track.keys.Count()
    );
    WriteTrackHeader(indent, track, buffer);
    for (unsigned int i = 0; i < track.keys.Count(); ++i) {
      const MDLKEYFRAME<float> &key = track.keys.Ptr()[i];
      WriteLine(buffer, "%s\t%d: ", indent, key.time);
      WriteKeyData(buffer, &key.value, 1);
      if (track.type > TRACK_LINEAR) {
        WriteLine(buffer, "%s\t\t%s ", indent, TokenText(0x15E));
        WriteKeyData(buffer, &key.inTan, 1);
        WriteLine(buffer, "%s\t\t%s ", indent, TokenText(0x18A));
        WriteKeyData(buffer, &key.outTan, 1);
      }
    }
    WriteLine(buffer, "%s}\n", indent);
  } else if (section.staticAlpha < 1.0f) {
    WriteLine(
        buffer,
        "%s%s %s ",
        indent,
        TokenText(0x1BB),
        TokenText(0x11C)
    );
    WriteKeyData(buffer, &section.staticAlpha, 1);
  }

  if (section.flags & 1) {
    if (section.colorKeys.keys.Count()) {
      const MDLKEYTRACK<C3Color> &track = section.colorKeys;
      WriteLine(
          buffer,
          "%s%s %d {\n",
          indent,
          TokenText(0x136),
          track.keys.Count()
      );
      WriteTrackHeader(indent, track, buffer);
      for (unsigned int i = 0; i < track.keys.Count(); ++i) {
        const MDLKEYFRAME<C3Color> &key = track.keys.Ptr()[i];
        WriteLine(buffer, "%s\t%d: ", indent, key.time);
        WriteKeyData(buffer, &key.value.b, 3);
        if (track.type > TRACK_LINEAR) {
          WriteLine(buffer, "%s\t\t%s ", indent, TokenText(0x15E));
          WriteKeyData(buffer, &key.inTan.b, 3);
          WriteLine(buffer, "%s\t\t%s ", indent, TokenText(0x18A));
          WriteKeyData(buffer, &key.outTan.b, 3);
        }
      }
      WriteLine(buffer, "%s}\n", indent);
    } else {
      WriteLine(
          buffer,
          "%s%s %s ",
          indent,
          TokenText(0x1BB),
          TokenText(0x136)
      );
      WriteKeyData(buffer, &section.staticColor.b, 3);
    }
  }
  WriteLine(
      buffer,
      "\t%s %d,\n",
      TokenText(0x150),
      section.geosetId
  );
  WriteLine(buffer, "}\n");
}

int __fastcall WriteGeosetAnims(
    const MDLDATA &data,
    TSGrowableArray<char> &buffer,
    CMDLStatus *
) {
  if (!static_cast<const char *>(data.model.animationFile)[0]) {
    for (unsigned int i = 0; i < data.geosetAnims.Count(); ++i) {
      IWriteGeosetAnimSection(data.geosetAnims.Ptr()[i], buffer);
    }
  }
  return 1;
}

static int IReadBinGeosetAnim(
    CMsgBuffer &buffer,
    MDLGEOSETANIMSECTION *geoAnim,
    unsigned int &totalRead,
    CMDLStatus *status
) {
  unsigned int sectionLength = buffer.GetUint();
  geoAnim->geosetId = buffer.GetUint();
  geoAnim->staticAlpha = buffer.GetFloat();
  geoAnim->staticColor.b = buffer.GetFloat();
  geoAnim->staticColor.g = buffer.GetFloat();
  geoAnim->staticColor.r = buffer.GetFloat();
  geoAnim->flags = buffer.GetUint();
  unsigned int localRead = 28;
  while (localRead < sectionLength) {
    unsigned long tag = buffer.GetDword();
    localRead += 4;
    if (tag == 'OAGK') {
      if (!ReadBinFloatKeyFrames(
              geoAnim->alphaKeys,
              buffer,
              localRead
          )) {
        return 0;
      }
    } else if (tag == 'CAGK') {
      if (!ReadBinFloatKeyFrames(
              geoAnim->colorKeys,
              buffer,
              localRead
          )) {
        return 0;
      }
    } else {
      SkipUnknown(buffer, localRead);
    }
    if (localRead > sectionLength) {
      status->FatalOverran("GeosetAnim keys", -1);
      return 0;
    }
  }
  totalRead += localRead;
  return 1;
}

int __fastcall ReadBinGeosetAnim(
    CMsgBuffer &buffer,
    unsigned int length,
    MDLDATA &data,
    CMDLStatus *status
) {
  unsigned int totalRead = 4;
  unsigned int count = buffer.GetUint();
  data.geosetAnims.Reserve(count);
  while (totalRead < length) {
    MDLGEOSETANIMSECTION *section = data.geosetAnims.New();
    if (!section) {
      status->FatalFlunked("GeosetAnim", -1);
      return 0;
    }
    if (!IReadBinGeosetAnim(buffer, section, totalRead, status)) {
      status->Add(STATUS_ERROR, "Error reading Geoset anim section.\n");
      return 0;
    }
    if (totalRead > length) {
      status->FatalOverran("GeosetAnim", -1);
      return 0;
    }
  }
  return 1;
}

static unsigned int IGetBinGeosetAnimSectionSize(
    const MDLGEOSETANIMSECTION &section
) {
  unsigned int size = 28;
  if (section.alphaKeys.keys.Count()) {
    unsigned int values =
        section.alphaKeys.type > TRACK_LINEAR ? 3 : 1;
    size += 16 + section.alphaKeys.keys.Count() * (4 + 4 * values);
  }
  if (section.colorKeys.keys.Count()) {
    unsigned int values =
        section.colorKeys.type > TRACK_LINEAR ? 9 : 3;
    size += 16 + section.colorKeys.keys.Count() * (4 + 4 * values);
  }
  return size;
}

static void IWriteBinGeosetAnimSection(
    const MDLGEOSETANIMSECTION &section,
    CMsgBuffer &buffer
) {
  buffer.AddUint(IGetBinGeosetAnimSectionSize(section));
  buffer.AddUint(section.geosetId);
  buffer.AddFloat(section.staticAlpha);
  buffer.AddFloat(section.staticColor.b);
  buffer.AddFloat(section.staticColor.g);
  buffer.AddFloat(section.staticColor.r);
  buffer.AddUint(section.flags);
  WriteBinFloatKeyFrames(section.alphaKeys, 'OAGK', buffer);
  if (section.colorKeys.keys.Count()) {
    const MDLKEYTRACK<C3Color> &track = section.colorKeys;
    buffer.AddDword('CAGK');
    buffer.AddUint(track.keys.Count());
    buffer.AddUint(track.type);
    buffer.AddUint(track.globalSeqId);
    unsigned int values = track.type > TRACK_LINEAR ? 9 : 3;
    for (unsigned int i = 0; i < track.keys.Count(); ++i) {
      const MDLKEYFRAME<C3Color> &key = track.keys.Ptr()[i];
      buffer.AddInt(key.time);
      buffer.AddFloatArray(&key.value.b, values);
    }
  }
}

int __fastcall WriteBinGeosetAnims(
    const MDLDATA &data,
    CMsgBuffer &buffer,
    CMDLStatus *
) {
  if (!static_cast<const char *>(data.model.animationFile)[0]
      && data.geosetAnims.Count()) {
    buffer.AddDword('AOEG');
    unsigned int totalSize = 4;
    unsigned int i;
    for (i = 0; i < data.geosetAnims.Count(); ++i) {
      totalSize += IGetBinGeosetAnimSectionSize(
          data.geosetAnims.Ptr()[i]
      );
    }
    buffer.AddUint(totalSize);
    buffer.AddUint(data.geosetAnims.Count());
    for (i = 0; i < data.geosetAnims.Count(); ++i) {
      IWriteBinGeosetAnimSection(data.geosetAnims.Ptr()[i], buffer);
    }
  }
  return 1;
}

static int IReadBinPrimitiveTypes(
    unsigned long magic,
    CMsgBuffer &buffer,
    TSGrowableArray<unsigned char> *section,
    unsigned int *localBytesRead,
    CMDLStatus *status
) {
  if (magic != 'PYTP') {
    status->Add(
        STATUS_ERROR,
        "Invalid primitives type section in Geoset.\n"
    );
    return 0;
  }
  unsigned int count = buffer.GetUint();
  *localBytesRead += 4;
  if (count) {
    section->SetCount(count);
    buffer.GetData(section->Ptr(), count);
    *localBytesRead += count;
  }
  return 1;
}

static void IReadBinAnimBounds(
    CMsgBuffer &buffer,
    MDLGEOSETSECTION *section,
    unsigned int *bytesRead
) {
  unsigned int count = buffer.GetUint();
  *bytesRead += 4;
  section->seqBounds.SetCount(count);
  for (unsigned int i = 0; i < count; ++i) {
    CMdlBounds &bounds = section->seqBounds[i];
    bounds.radius = buffer.GetFloat();
    buffer.GetFloatArray(&bounds.extent.b.x, 3);
    buffer.GetFloatArray(&bounds.extent.t.x, 3);
    *bytesRead += 28;
  }
}

static int ReadBinGeosetTags(
    CMsgBuffer &buffer,
    MDLGEOSETSECTION *geoset,
    CMDLStatus *status,
    unsigned int *localBytesRead
) {
  if (!ReadBinC3VectorSection(
          buffer,
          'XTRV',
          "vertex",
          &geoset->vertices,
          localBytesRead,
          status
      )) {
    return 0;
  }
  if (!ReadBinC3VectorSection(
          buffer,
          'SMRN',
          "normal",
          &geoset->normals,
          localBytesRead,
          status
      )) {
    return 0;
  }

  unsigned long magic = buffer.GetDword();
  *localBytesRead += 4;
  if (magic == 'SAVU') {
    unsigned int channels = buffer.GetUint();
    *localBytesRead += 4;
    FATALASSERT(channels);
    geoset->texCoords.SetCount(channels);
    unsigned int vertexCount = geoset->vertices.Count();
    for (unsigned int i = 0; i < channels; ++i) {
      geoset->texCoords[i].SetCount(vertexCount);
      buffer.GetFloatArray(
          &geoset->texCoords[i].Ptr()->x,
          2 * vertexCount
      );
    }
    *localBytesRead += 8 * channels * vertexCount;
    magic = buffer.GetDword();
    *localBytesRead += 4;
  }

  if (!IReadBinPrimitiveTypes(
          magic,
          buffer,
          &geoset->primitives.types,
          localBytesRead,
          status
      )) {
    return 0;
  }

  magic = buffer.GetDword();
  *localBytesRead += 4;
  if (magic != 'TNCP') {
    status->Add(
        STATUS_ERROR, "Invalid %s section.\n", "primitives count"
    );
    return 0;
  }
  unsigned int count = buffer.GetUint();
  *localBytesRead += 4;
  if (count) {
    geoset->primitives.counts.SetCount(count);
    buffer.GetUintArray(geoset->primitives.counts.Ptr(), count);
    *localBytesRead += 4 * count;
  }

  magic = buffer.GetDword();
  *localBytesRead += 4;
  if (magic != 'XTVP') {
    status->Add(
        STATUS_ERROR, "Invalid %s section.\n", "primitives vertices"
    );
    return 0;
  }
  count = buffer.GetUint();
  *localBytesRead += 4;
  if (count) {
    geoset->primitives.vertices.SetCount(count);
    buffer.GetWordArray(geoset->primitives.vertices.Ptr(), count);
    *localBytesRead += 2 * count;
  }

  magic = buffer.GetDword();
  *localBytesRead += 4;
  if (magic != 'XDNG') {
    status->Add(
        STATUS_ERROR, "Invalid %s section.\n", "vertex group indices"
    );
    return 0;
  }
  count = buffer.GetUint();
  *localBytesRead += 4;
  if (count) {
    geoset->vertGroupIndices.SetCount(count);
    buffer.GetData(geoset->vertGroupIndices.Ptr(), count);
    *localBytesRead += count;
  }

  return IReadBinUintSection(
             buffer,
             'CGTM',
             "group matrix counts",
             &geoset->groupMatrixCounts,
             localBytesRead,
             status
         )
      && IReadBinUintSection(
             buffer,
             'STAM',
             "matrices",
             &geoset->matrices,
             localBytesRead,
             status
         )
      && IReadBinUintSection(
             buffer,
             'XDIB',
             "bone indices",
             &geoset->boneIndices,
             localBytesRead,
             status
         )
      && IReadBinUintSection(
             buffer,
             'TGWB',
             "bone weights",
             &geoset->boneWeights,
             localBytesRead,
             status
         );
}

static int ReadBinGeoset(
    CMsgBuffer &buffer,
    MDLGEOSETSECTION *geoset,
    CMDLStatus *status,
    unsigned int &totalLength
) {
  unsigned int sectionLength = buffer.GetUint();
  unsigned int localBytesRead = 4;
  if (!ReadBinGeosetTags(
          buffer, geoset, status, &localBytesRead
      )) {
    return 0;
  }
  geoset->materialId = buffer.GetUint();
  geoset->selectionGroup = buffer.GetUint();
  geoset->flags = buffer.GetUint();
  geoset->bounds.radius = buffer.GetFloat();
  localBytesRead += 16;
  buffer.GetFloatArray(&geoset->bounds.extent.b.x, 3);
  buffer.GetFloatArray(&geoset->bounds.extent.t.x, 3);
  localBytesRead += 24;
  IReadBinAnimBounds(buffer, geoset, &localBytesRead);
  FATALASSERT(sectionLength == localBytesRead);
  totalLength += localBytesRead;
  return 1;
}

int __fastcall ReadBinGeosets(
    CMsgBuffer &buffer,
    unsigned int length,
    MDLDATA &data,
    CMDLStatus *status
) {
  FATALASSERT(status);
  unsigned int totalRead = 4;
  unsigned int count = buffer.GetUint();
  data.geosets.SetCount(0);
  data.geosets.Reserve(count);
  while (totalRead < length) {
    MDLGEOSETSECTION *section = data.geosets.New();
    if (!ReadBinGeoset(buffer, section, status, totalRead)) {
      status->Add(STATUS_ERROR, "Error reading Geoset.\n");
      return 0;
    }
    if (totalRead > length) {
      status->FatalOverran("Geoset", -1);
      return 0;
    }
  }
  return 1;
}

static unsigned int GetBinGeosetSize(
    const MDLGEOSETSECTION &section
) {
  unsigned int size = 4;
  size += 8 + 12 * section.vertices.Count();
  size += 8 + 12 * section.normals.Count();
  if (section.texCoords.Count()) {
    size += 8
        + 8 * section.texCoords.Count() * section.vertices.Count();
  }
  size += 8 + section.primitives.types.Count();
  size += 8 + 4 * section.primitives.counts.Count();
  size += 8 + 2 * section.primitives.vertices.Count();
  size += 8 + section.vertGroupIndices.Count();
  size += 8 + 4 * section.groupMatrixCounts.Count();
  size += 8 + 4 * section.matrices.Count();
  size += 8 + 4 * section.boneIndices.Count();
  size += 8 + 4 * section.boneWeights.Count();
  size += 40;
  size += 4 + 28 * section.seqBounds.Count();
  return size;
}

static void IWriteBinAnimBounds(
    const MDLGEOSETSECTION &section,
    CMsgBuffer &buffer
) {
  buffer.AddUint(section.seqBounds.Count());
  for (unsigned int i = 0; i < section.seqBounds.Count(); ++i) {
    const CMdlBounds &bounds = section.seqBounds[i];
    buffer.AddFloat(bounds.radius);
    buffer.AddFloatArray(&bounds.extent.b.x, 3);
    buffer.AddFloatArray(&bounds.extent.t.x, 3);
  }
}

static void WriteBinGeoset(
    CMsgBuffer &buffer,
    const MDLGEOSETSECTION &section
) {
  buffer.AddUint(GetBinGeosetSize(section));
  WriteBinC3VectorSection(buffer, 'XTRV', section.vertices);
  WriteBinC3VectorSection(buffer, 'SMRN', section.normals);
  if (section.texCoords.Count()) {
    buffer.AddDword('SAVU');
    buffer.AddUint(section.texCoords.Count());
    unsigned int floats = 2 * section.vertices.Count();
    for (unsigned int i = 0; i < section.texCoords.Count(); ++i) {
      buffer.AddFloatArray(&section.texCoords[i].Ptr()->x, floats);
    }
  }
  buffer.AddDword('PYTP');
  buffer.AddUint(section.primitives.types.Count());
  buffer.AddData(
      section.primitives.types.Ptr(),
      section.primitives.types.Count()
  );
  buffer.AddDword('TNCP');
  buffer.AddUint(section.primitives.counts.Count());
  buffer.AddUintArray(
      section.primitives.counts.Ptr(),
      section.primitives.counts.Count()
  );
  buffer.AddDword('XTVP');
  buffer.AddUint(section.primitives.vertices.Count());
  buffer.AddWordArray(
      section.primitives.vertices.Ptr(),
      section.primitives.vertices.Count()
  );
  buffer.AddDword('XDNG');
  buffer.AddUint(section.vertGroupIndices.Count());
  buffer.AddData(
      section.vertGroupIndices.Ptr(),
      section.vertGroupIndices.Count()
  );
  buffer.AddDword('CGTM');
  buffer.AddUint(section.groupMatrixCounts.Count());
  buffer.AddUintArray(
      section.groupMatrixCounts.Ptr(),
      section.groupMatrixCounts.Count()
  );
  buffer.AddDword('STAM');
  buffer.AddUint(section.matrices.Count());
  buffer.AddUintArray(section.matrices.Ptr(), section.matrices.Count());
  buffer.AddDword('XDIB');
  buffer.AddUint(section.boneIndices.Count());
  buffer.AddUintArray(
      section.boneIndices.Ptr(), section.boneIndices.Count()
  );
  buffer.AddDword('TGWB');
  buffer.AddUint(section.boneWeights.Count());
  buffer.AddUintArray(
      section.boneWeights.Ptr(), section.boneWeights.Count()
  );
  buffer.AddUint(section.materialId);
  buffer.AddUint(section.selectionGroup);
  buffer.AddUint(section.flags);
  buffer.AddFloat(section.bounds.radius);
  buffer.AddFloatArray(&section.bounds.extent.b.x, 3);
  buffer.AddFloatArray(&section.bounds.extent.t.x, 3);
  IWriteBinAnimBounds(section, buffer);
}

int __fastcall WriteBinGeosets(
    const MDLDATA &data,
    CMsgBuffer &buffer,
    CMDLStatus *
) {
  if (data.geosets.Count()) {
    buffer.AddDword('SOEG');
    unsigned int totalSize = 4;
    unsigned int i;
    for (i = 0; i < data.geosets.Count(); ++i) {
      totalSize += GetBinGeosetSize(data.geosets[i]);
    }
    buffer.AddUint(totalSize);
    buffer.AddUint(data.geosets.Count());
    for (i = 0; i < data.geosets.Count(); ++i) {
      WriteBinGeoset(buffer, data.geosets[i]);
    }
  }
  return 1;
}

}
