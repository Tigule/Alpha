#include "MDLTypes.h"
#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

#include <stpl.h>

namespace MDL {
  LPCSTR       TokenText(UINT token);
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);
}  // namespace MDL

void ReadVertices(Parser &parse, LPCSTR title, TSGrowableArray<NTempest::C3Vector> *vertices) {
  UINT   savedtoken;
  LPCSTR tokentext;
  long   actual = 0;
  long   count = parse.GetOptionalInt(&savedtoken, &tokentext, 0);
  if (count > 0) {
    vertices->ReserveSpace(count);
  }
  parse.Expect('{', savedtoken, tokentext);

  savedtoken = parse.Token(&tokentext, 0);
  while (savedtoken == '{') {
    NTempest::C3Vector *vertex = vertices->New();
    vertex->x = parse.ExpectFloat();
    parse.Expect(',');
    vertex->y = parse.ExpectFloat();
    parse.Expect(',');
    vertex->z = parse.ExpectFloat();
    parse.Expect('}');
    parse.Expect(',');
    ++actual;
    savedtoken = parse.Token(&tokentext, 0);
  }
  parse.Expect('}', savedtoken, tokentext);
  if (count >= 0 && actual != count) {
    parse.WarningCount(title, count, actual);
  }
}

void WriteVertices(const TSGrowableArray<NTempest::C3Vector> &vertices, UINT title, TSGrowableArray<char> &buffer) {
  MDL::WriteLine(buffer, "\t%s %d {\n", MDL::TokenText(title), vertices.Count());
  for (UINT i = 0; i < vertices.Count(); ++i) {
    const NTempest::C3Vector &vertex = vertices[i];
    MDL::WriteLine(buffer, "\t\t{ %g, %g, %g },\n", vertex.x, vertex.y, vertex.z);
  }
  MDL::WriteLine(buffer, "\t}\n");
}

void WriteBinC3VectorSection(CMsgBuffer &buf, DWORD title, const TSGrowableArray<NTempest::C3Vector> &section) {
  buf.AddDword(title);
  buf.AddUint(section.Count());
  buf.AddFloatArray(&section.Ptr()->x, 3 * section.Count());
}

BOOL ReadBinC3VectorSection(
    CMsgBuffer                          &buf,
    DWORD                                title,
    LPCSTR                               name,
    TSGrowableArray<NTempest::C3Vector> *section,
    UINT                                *localBytesRead,
    CMDLStatus                          *status
) {
  if (buf.GetDword() != title) {
    status->Add(STATUS_ERROR, "Invalid %s section detected in model.\n", name);
    return 0;
  }
  UINT count = buf.GetUint();
  *localBytesRead += 8;
  section->SetCount(count);
  if (count) {
    *localBytesRead += 12 * count;
    buf.GetFloatArray(&section->Ptr()->x, 3 * count);
  }
  return 1;
}

BOOL IReadBinUintSection(CMsgBuffer &buf, DWORD title, LPCSTR name, TSGrowableArray<UINT> *section, UINT *localBytesRead, CMDLStatus *status) {
  DWORD found = buf.GetDword();
  *localBytesRead += 4;
  if (found != title) {
    status->Add(STATUS_ERROR, "Invalid %s section.\n", name);
    return 0;
  }
  UINT count = buf.GetUint();
  *localBytesRead += 4;
  if (count) {
    section->SetCount(count);
    buf.GetUintArray(section->Ptr(), count);
    *localBytesRead += 4 * count;
  }
  return 1;
}

void SetVertexGroupIndices(const TSGrowableArray<UINT> &groupVertexCounts, MDLGEOSETSECTION *geoset) {
  FATALASSERT(geoset);
  geoset->vertGroupIndices.SetCount(geoset->vertices.Count());
  if (groupVertexCounts.Count() < 2) {
    if (geoset->vertGroupIndices.Count()) {
      memset(geoset->vertGroupIndices.Ptr(), 0, geoset->vertGroupIndices.Count());
    }
    return;
  }

  UINT vertex = 0;
  for (UINT group = 0; group < groupVertexCounts.Count(); ++group) {
    UINT count = groupVertexCounts[group];
    while (count-- && vertex < geoset->vertGroupIndices.Count()) {
      geoset->vertGroupIndices[vertex++] = static_cast<BYTE>(group);
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

static void IReadTVertices(Parser &parse, TSGrowableArray<NTempest::C2Vector> *texcoords) {
  UINT   savedtoken;
  LPCSTR tokentext;
  long   count = parse.GetOptionalInt(&savedtoken, &tokentext, 0);
  if (count > 0) {
    texcoords->ReserveSpace(count);
  }
  parse.Expect('{', savedtoken, tokentext);
  long actual = 0;
  savedtoken = parse.Token(&tokentext, 0);
  while (savedtoken == '{') {
    NTempest::C2Vector *coord = texcoords->New();
    coord->x = parse.ExpectFloat();
    parse.Expect(',');
    coord->y = parse.ExpectFloat();
    parse.Expect('}');
    parse.Expect(',');
    ++actual;
    savedtoken = parse.Token(&tokentext, 0);
  }
  parse.Expect('}', savedtoken, tokentext);
  if (count >= 0 && count != actual) {
    parse.WarningCount("vertices", count, actual);
  }
}

static void ISkipDuplicates(Parser &parse) {
  UINT       savedtoken;
  LPCSTR     tokentext;
  UTokenData savedvalue;
  parse.GetOptionalInt(&savedtoken, &tokentext, &savedvalue);
  parse.Expect('{', savedtoken, tokentext);
  savedtoken = parse.Token(&tokentext, &savedvalue);
  while (savedtoken && savedtoken != '}') {
    if (savedtoken != 0x100) {
      parse.FatalUnexpected(tokentext);
    }
    parse.Expect(',');
    savedtoken = parse.Token(&tokentext, &savedvalue);
  }
  parse.Expect('}', savedtoken, tokentext);
}

static UINT IVertexList(Parser &parse, TSGrowableArray<WORD> *vertlist) {
  FATALASSERT(vertlist);
  vertlist->New()[0] = static_cast<WORD>(parse.ExpectInt());
  UINT   entries = 1;
  LPCSTR tokentext;
  UINT   token = parse.Token(&tokentext, 0);
  while (token == ',') {
    vertlist->New()[0] = static_cast<WORD>(parse.ExpectInt());
    ++entries;
    token = parse.Token(&tokentext, 0);
  }
  parse.Expect('}', token, tokentext);
  return entries;
}

static UINT IVertexListSet(Parser &parse, MDLPRIMITIVES *primitives, BYTE type) {
  *primitives->types.New() = type;
  UINT *count = primitives->counts.New();
  *count = IVertexList(parse, &primitives->vertices);
  parse.Expect(',');
  return *count;
}

static UINT IPrimitives(Parser &parse, MDLPRIMITIVES *primitives, long *entries, BYTE type, int (*IsInvalid)(UINT), LPCSTR errorText) {
  UINT primsAdded = 0;
  parse.Expect('{');
  LPCSTR tokentext;
  UINT   token = parse.Token(&tokentext, 0);
  while (token == '{') {
    UINT count = IVertexListSet(parse, primitives, type);
    if (IsInvalid(count)) {
      parse.FatalNotFound(errorText);
    }
    *entries += count;
    ++primsAdded;
    token = parse.Token(&tokentext, 0);
  }
  parse.Expect('}', token, tokentext);
  return primsAdded;
}

static int NeverInvalid(UINT) {
  return 0;
}

static int InvalidLines(UINT numVerts) {
  return numVerts & 1;
}

static int InvalidLineStripLoop(UINT numVerts) {
  return numVerts < 2;
}

static int InvalidTriangles(UINT numVerts) {
  return numVerts % 3;
}

static int InvalidTriangleFanStrip(UINT numVerts) {
  return numVerts < 3;
}

static int InvalidQuads(UINT numVerts) {
  return numVerts & 3;
}

static BOOL InvalidQuadStrip(UINT numVerts) {
  return numVerts < 4 || (numVerts & 1);
}

static UINT IMultiPoints(Parser &parse, MDLPRIMITIVES *primitives, long *entries, BYTE type) {
  return IPrimitives(parse, primitives, entries, type, NeverInvalid, 0);
}

static UINT ILines(Parser &parse, MDLPRIMITIVES *primitives, long *entries) {
  return IPrimitives(parse, primitives, entries, 1, InvalidLines, "an even number of entries");
}

static UINT ILineStripLoop(Parser &parse, MDLPRIMITIVES *primitives, long *entries, BYTE type) {
  return IPrimitives(parse, primitives, entries, type, InvalidLineStripLoop, "at least two entries");
}

static UINT ITriangles(Parser &parse, MDLPRIMITIVES *primitives, long *entries) {
  return IPrimitives(parse, primitives, entries, 4, InvalidTriangles, "a multiple of three entries");
}

static UINT ITriangleFanStrip(Parser &parse, MDLPRIMITIVES *primitives, long *entries, BYTE type) {
  return IPrimitives(parse, primitives, entries, type, InvalidTriangleFanStrip, "at least three entries");
}

static UINT IQuads(Parser &parse, MDLPRIMITIVES *primitives, long *entries) {
  return IPrimitives(parse, primitives, entries, 7, InvalidQuads, "a multiple of four entries");
}

static UINT IQuadStrip(Parser &parse, MDLPRIMITIVES *primitives, long *entries) {
  return IPrimitives(parse, primitives, entries, 8, InvalidQuadStrip, "at least four and a multiple of two entries");
}

static void IReadPrimitives(Parser &parse, MDLPRIMITIVES *primitives) {
  UINT       savedtoken;
  LPCSTR     tokentext;
  UTokenData savedvalue;
  long       estPrims = parse.GetOptionalInt(&savedtoken, &tokentext, &savedvalue);
  long       estVerts = -1;
  if (estPrims > 0) {
    estVerts = parse.GetOptionalInt(savedtoken, &savedvalue, &savedtoken, &tokentext);
    primitives->ReserveSpace(estPrims, estVerts > 0 ? estVerts : 3 * estPrims);
  }
  parse.Expect('{', savedtoken, tokentext);
  long actualVerts = 0;
  long actualPrimitives = 0;
  savedtoken = parse.Token(&tokentext, 0);
  while (savedtoken && savedtoken != '}') {
    switch (savedtoken) {
      case 0x168:
        actualPrimitives += ILines(parse, primitives, &actualVerts);
        break;
      case 0x16A:
        actualPrimitives += ILineStripLoop(parse, primitives, &actualVerts, 2);
        break;
      case 0x16B:
        actualPrimitives += ILineStripLoop(parse, primitives, &actualVerts, 3);
        break;
      case 0x1A3:
        actualPrimitives += IMultiPoints(parse, primitives, &actualVerts, 0);
        break;
      case 0x1A4:
        actualPrimitives += IMultiPoints(parse, primitives, &actualVerts, 9);
        break;
      case 0x1A8:
        actualPrimitives += IQuads(parse, primitives, &actualVerts);
        break;
      case 0x1A9:
        actualPrimitives += IQuadStrip(parse, primitives, &actualVerts);
        break;
      case 0x1C9:
        actualPrimitives += ITriangles(parse, primitives, &actualVerts);
        break;
      case 0x1CA:
        actualPrimitives += ITriangleFanStrip(parse, primitives, &actualVerts, 5);
        break;
      case 0x1CB:
        actualPrimitives += ITriangleFanStrip(parse, primitives, &actualVerts, 6);
        break;
      default:
        parse.FatalUnexpected(tokentext);
        break;
    }
    savedtoken = parse.Token(&tokentext, 0);
  }
  parse.Expect('}', savedtoken, tokentext);
  if (estVerts >= 0 && estVerts != actualVerts) {
    parse.WarningCount("primitive vertices", estVerts, actualVerts);
  }
  if (estPrims >= 0 && estPrims != actualPrimitives) {
    parse.WarningCount("primitives", estPrims, actualPrimitives);
  }
}

static void IReadMatrices(Parser &parse, UINT *mtxCount, TSGrowableArray<UINT> *matrixIdList) {
  FATALASSERT(mtxCount);
  FATALASSERT(matrixIdList);
  *mtxCount = 0;
  parse.Expect('{');
  *matrixIdList->New() = parse.ExpectInt();
  ++*mtxCount;
  LPCSTR tokentext;
  UINT   token = parse.Token(&tokentext, 0);
  while (token == ',') {
    *matrixIdList->New() = parse.ExpectInt();
    ++*mtxCount;
    token = parse.Token(&tokentext, 0);
  }
  parse.Expect('}', token, tokentext);
}

static void IReadGroup(Parser &parse, MDLGEOSETSECTION *geoset, long *numMatrices, TSGrowableArray<UINT> *groupVertexCounts, CMDLStatus *status) {
  TSet errors;
  errors.Add(0x1D6, 1, 0);
  errors.Add(0x16E, 1, 0);
  UINT *vertexCount = groupVertexCounts->New();
  *vertexCount = 0;
  UINT *matrixCount = geoset->groupMatrixCounts.New();
  *matrixCount = 0;
  parse.Expect('{');
  LPCSTR tokentext;
  UINT   token = parse.Token(&tokentext, 0);
  while (token && token != '}') {
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokentext);
    }
    if (token == 0x16E) {
      IReadMatrices(parse, matrixCount, &geoset->matrices);
      *numMatrices += *matrixCount;
    } else if (token == 0x1D6) {
      *vertexCount = parse.ExpectInt();
    } else {
      parse.FatalUnexpected(tokentext);
    }
    parse.Expect(',');
    token = parse.Token(&tokentext, 0);
  }
  parse.Expect('}', token, tokentext);
  errors.Complete(status);
}

static void IReadGroups(Parser &parse, MDLGEOSETSECTION *geoset, CMDLStatus *status) {
  UINT       savedtoken;
  LPCSTR     tokentext;
  UTokenData savedvalue;
  long       estGroups = parse.GetOptionalInt(&savedtoken, &tokentext, &savedvalue);
  long       estMatrices = -1;
  if (estGroups > 0) {
    estMatrices = parse.GetOptionalInt(savedtoken, &savedvalue, &savedtoken, &tokentext);
    geoset->groupMatrixCounts.ReserveSpace(estGroups);
    if (estMatrices > 0) {
      geoset->matrices.ReserveSpace(estMatrices);
    }
  }
  parse.Expect('{', savedtoken, tokentext);
  long                  actualGroups = 0;
  long                  actualMatrices = 0;
  TSGrowableArray<UINT> groupVertexCounts;
  savedtoken = parse.Token(&tokentext, 0);
  while (savedtoken && savedtoken != '}') {
    if (savedtoken == 0x155) {
      IReadGroup(parse, geoset, &actualMatrices, &groupVertexCounts, status);
    } else if (savedtoken == 0x16E) {
      UINT *count = geoset->groupMatrixCounts.New();
      IReadMatrices(parse, count, &geoset->matrices);
      actualMatrices += *count;
    } else {
      parse.FatalUnexpected(tokentext);
    }
    ++actualGroups;
    parse.Expect(',');
    savedtoken = parse.Token(&tokentext, 0);
  }
  parse.Expect('}', savedtoken, tokentext);
  if (groupVertexCounts.Count()) {
    SetVertexGroupIndices(groupVertexCounts, geoset);
  }
  if (estGroups >= 0 && estGroups != actualGroups) {
    parse.WarningCount("groups", estGroups, actualGroups);
  }
  if (estMatrices >= 0 && estMatrices != actualMatrices) {
    parse.WarningCount("matrices", estMatrices, actualMatrices);
  }
}

static void IReadBoneWeights(Parser &parse, MDLGEOSETSECTION *geoset) {
  UINT count = parse.ExpectInt();
  geoset->boneWeights.ReserveSpace(count);
  parse.Expect('{');
  for (UINT i = 0; i < count; ++i) {
    *geoset->boneWeights.New() = parse.ExpectInt();
    parse.Expect(',');
  }
  parse.Expect('}');
}

static void IReadBoneIndices(Parser &parse, MDLGEOSETSECTION *geoset) {
  UINT count = parse.ExpectInt();
  geoset->boneIndices.ReserveSpace(count);
  parse.Expect('{');
  for (UINT i = 0; i < count; ++i) {
    *geoset->boneIndices.New() = parse.ExpectInt();
    parse.Expect(',');
  }
  parse.Expect('}');
}

static void IReadVertexGroupIds(Parser &parse, MDLGEOSETSECTION *geoset) {
  geoset->vertGroupIndices.ReserveSpace(geoset->vertices.Count());
  parse.Expect('{');
  LPCSTR     tokentext;
  UTokenData savedvalue;
  UINT       token = parse.Token(&tokentext, &savedvalue);
  while (token && token != '}') {
    if (token == 0x100) {
      *geoset->vertGroupIndices.New() = static_cast<BYTE>(savedvalue.cVal);
    } else {
      parse.FatalUnexpected(tokentext);
    }
    parse.Expect(',');
    token = parse.Token(&tokentext, &savedvalue);
  }
  parse.Expect('}', token, tokentext);
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

static void IReadAnimBounds(Parser &parse, CMdlBounds *bounds, CMDLStatus *status) {
  TSet errors;
  IAnimBoundsAddErrors(errors);
  parse.Expect('{');
  LPCSTR tokentext;
  UINT   token = parse.Token(&tokentext, 0);
  while (token && token != '}') {
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokentext);
    }
    if (token == 0x170) {
      IReadVertex(parse, &bounds->extent.b);
    } else if (token == 0x16F) {
      IReadVertex(parse, &bounds->extent.t);
    } else if (token == 0x134) {
      bounds->radius = parse.ExpectFloat();
    } else {
      parse.FatalUnexpected(tokentext);
    }
    parse.Expect(',');
    token = parse.Token(&tokentext, 0);
  }
  parse.Expect('}', token, tokentext);
  errors.Complete(status);
}

static BOOL ValidateVertexCounts(const MDLGEOSETSECTION &geoset, Parser &parse, CMDLStatus *status) {
  UINT numVertices = geoset.vertices.Count();
  if (numVertices != geoset.normals.Count() || numVertices != geoset.vertGroupIndices.Count()) {
    status->Add(STATUS_FATAL, "Error (line %d): Vertex count doesn't match normals and group indices\n", parse.GetLineNumber());
    return 0;
  }
  UINT numTexLayers = geoset.texCoords.Count();
  for (UINT i = 0; i < numTexLayers; ++i) {
    if (geoset.texCoords[i].Count() != numVertices) {
      status->Add(STATUS_FATAL, "Error (line %d): Vertex count doesn't match texture coordinates\n", parse.GetLineNumber());
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

static BOOL IReadAlpha(Parser &parse, int expectanimation, MDLGEOSETANIMSECTION *geoset) {
  if (expectanimation) {
    ReadObjectFloatKeyframes(parse, &geoset->alphaKeys);
    return 1;
  }
  geoset->staticAlpha = parse.ExpectFloat();
  return 0;
}

static BOOL IReadColor(Parser &parse, int expectanimation, MDLGEOSETANIMSECTION *geoset) {
  if (expectanimation) {
    ReadObjectFloatKeyframes(parse, &geoset->colorKeys);
    return 1;
  }
  ReadFloatKeyData(parse, &geoset->staticColor.b, 3);
  return 0;
}

static BOOL IllegalStaticToken(UINT token) {
  return token != 0x11C && token != 0x136 && token != 0x189;
}

static BOOL IReadGeosetAnim(Parser &parse, UINT savedtoken, LPCSTR tokenText, TSet *errors, MDLGEOSETANIMSECTION *geoAnim) {
  int expectAnimation = IExpectAnimation(parse, &savedtoken, &tokenText);
  if (!expectAnimation && IllegalStaticToken(savedtoken)) {
    parse.FatalUnexpected(tokenText);
  }
  if (!errors->Check(savedtoken)) {
    parse.FatalDuplicate(tokenText);
  }
  if (!geoAnim) {
    return 0;
  }
  if (savedtoken == 0x11C || savedtoken == 0x189) {
    if (!IReadAlpha(parse, expectAnimation, geoAnim)) {
      parse.Expect(',');
    }
    return 1;
  }
  if (savedtoken == 0x136) {
    geoAnim->flags |= 1;
    if (!IReadColor(parse, expectAnimation, geoAnim)) {
      parse.Expect(',');
    }
    return 1;
  }
  return 0;
}

static void IWriteGeosetTexCoords(const TSGrowableArray<NTempest::C2Vector> &texcoords, TSGrowableArray<char> &buffer) {
  if (!texcoords.Count()) {
    return;
  }
  MDL::WriteLine(buffer, "\t%s %d {\n", MDL::TokenText(0x1CE), texcoords.Count());
  for (UINT i = 0; i < texcoords.Count(); ++i) {
    MDL::WriteLine(buffer, "\t\t{ %g, %g },\n", texcoords[i].x, texcoords[i].y);
  }
  MDL::WriteLine(buffer, "\t}\n");
}

static void IWriteVertexGroupIndices(const TSGrowableArray<BYTE> &vertGroupIndices, TSGrowableArray<char> &buffer) {
  MDL::WriteLine(buffer, "\t%s {\n", MDL::TokenText(0x1D7));
  for (UINT i = 0; i < vertGroupIndices.Count(); ++i) {
    MDL::WriteLine(buffer, "\t\t%u,\n", vertGroupIndices[i]);
  }
  MDL::WriteLine(buffer, "\t}\n");
}

static LPCSTR IGetPrimitiveText(BYTE type) {
  switch (type) {
    case 0:
      return MDL::TokenText(0x1A3);
    case 1:
      return MDL::TokenText(0x168);
    case 2:
      return MDL::TokenText(0x16A);
    case 3:
      return MDL::TokenText(0x16B);
    case 4:
      return MDL::TokenText(0x1C9);
    case 5:
      return MDL::TokenText(0x1CB);
    case 6:
      return MDL::TokenText(0x1CA);
    case 7:
      return MDL::TokenText(0x1A8);
    case 8:
      return MDL::TokenText(0x1A9);
    case 9:
      return MDL::TokenText(0x1A4);
    default:
      return MDL::TokenText(0x1DF);
  }
}

static void IWriteGeosetPrimitives(const MDLPRIMITIVES &faces, TSGrowableArray<char> &buffer) {
  MDL::WriteLine(buffer, "\t%s %d %d {\n", MDL::TokenText(0x148), faces.types.Count(), faces.vertices.Count());
  for (UINT type = 0; type < 10; ++type) {
    const WORD *vertex = faces.vertices.Ptr();
    int         opened = 0;
    for (UINT i = 0; i < faces.types.Count(); ++i) {
      if (faces.types[i] == type) {
        if (!opened) {
          MDL::WriteLine(buffer, "\t\t%s {\n", IGetPrimitiveText(type));
          opened = 1;
        }
        MDL::WriteLine(buffer, "\t\t\t{ %d", vertex[0]);
        for (UINT j = 1; j < faces.counts[i]; ++j) {
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

static void IWriteGeosetGroups(const MDLGEOSETSECTION &section, TSGrowableArray<char> &buffer) {
  MDL::WriteLine(buffer, "\t%s %u %u {\n", MDL::TokenText(0x156), section.groupMatrixCounts.Count(), section.matrices.Count());
  const UINT *matrix = section.matrices.Ptr();
  for (UINT i = 0; i < section.groupMatrixCounts.Count(); ++i) {
    MDL::WriteLine(buffer, "\t\t%s { ", MDL::TokenText(0x16E));
    for (UINT j = 0; j < section.groupMatrixCounts[i]; ++j) {
      MDL::WriteLine(buffer, "%u", *matrix++);
      if (j + 1 < section.groupMatrixCounts[i]) {
        MDL::WriteLine(buffer, ", ");
      }
    }
    MDL::WriteLine(buffer, " },\n");
  }
  MDL::WriteLine(buffer, "\t}\n");
}

static void IWriteBoneWeights(const MDLGEOSETSECTION &section, TSGrowableArray<char> &buffer) {
  UINT count = section.boneIndices.Count();
  MDL::WriteLine(buffer, "\t%s %u {\n", MDL::TokenText(0x131), count);
  for (UINT i = 0; i < count; ++i) {
    MDL::WriteLine(buffer, "\t\t0x%08X,\n", section.boneIndices[i]);
  }
  MDL::WriteLine(buffer, "\t}\n");
  MDL::WriteLine(buffer, "\t%s %u {\n", MDL::TokenText(0x132), count);
  for (UINT j = 0; j < count; ++j) {
    MDL::WriteLine(buffer, "\t\t0x%08X,\n", section.boneWeights[j]);
  }
  MDL::WriteLine(buffer, "\t}\n");
}

static void IWriteAnimBounds(const TSGrowableArray<CMdlBounds> &geoBounds, TSGrowableArray<char> &buffer) {
  for (UINT i = 0; i < geoBounds.Count(); ++i) {
    MDL::WriteLine(buffer, "\t%s {\n", MDL::TokenText(0x122));
    WriteBounds(geoBounds[i], "\t\t", buffer);
    MDL::WriteLine(buffer, "\t}\n");
  }
}

static void IWriteGeosetSection(const MDLGEOSETSECTION &section, int writeMaterialId, TSGrowableArray<char> &buffer) {
  MDL::WriteLine(buffer, "%s {\n", MDL::TokenText(0x10A));
  WriteVertices(section.vertices, 0x1D8, buffer);
  WriteVertices(section.normals, 0x17B, buffer);
  for (UINT i = 0; i < section.texCoords.Count(); ++i) {
    IWriteGeosetTexCoords(section.texCoords[i], buffer);
  }
  IWriteVertexGroupIndices(section.vertGroupIndices, buffer);
  IWriteGeosetPrimitives(section.primitives, buffer);
  IWriteGeosetGroups(section, buffer);
  IWriteBoneWeights(section, buffer);
  WriteBounds(section.bounds, "\t", buffer);
  IWriteAnimBounds(section.seqBounds, buffer);
  if (writeMaterialId) {
    MDL::WriteLine(buffer, "\t%s %d,\n", MDL::TokenText(0x16D), section.materialId);
  }
  MDL::WriteLine(buffer, "\t%s %d,\n", MDL::TokenText(0x1B1), section.selectionGroup);
  if (section.flags & 1) {
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(0x1D2));
  }
  MDL::WriteLine(buffer, "}\n");
}

namespace MDL {

  BOOL ReadGeoset(Parser &parse, MDLDATA &data, CMDLStatus *status) {
    MDLGEOSETSECTION     *geoset = data.geosets.New();
    MDLGEOSETANIMSECTION *geoAnim = 0;
    TSet                  geosetAnimErrors;
    if (data.version < 600) {
      geoAnim = data.geosetAnims.New();
      geoAnim->geosetId = data.geosets.Count() - 1;
      IGeosetAnimAddErrors(geosetAnimErrors);
    }
    geoset->seqBounds.ReserveSpace(data.sequences.Count());

    TSet errors;
    IGeosetAddErrors(errors);
    parse.Expect('{');
    LPCSTR tokentext;
    UINT   token = parse.Token(&tokentext, 0);
    while (token && token != '}') {
      if (IReadGeosetAnim(parse, token, tokentext, &geosetAnimErrors, geoAnim)) {
        token = parse.Token(&tokentext, 0);
        continue;
      }
      if (!errors.Check(token)) {
        parse.FatalDuplicate(tokentext);
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
          parse.FatalUnexpected(tokentext);
          break;
      }
      parse.Expect(',');
      token = parse.Token(&tokentext, 0);
    }
    parse.Expect('}', token, tokentext);
    errors.Complete(status);
    geoset->seqBounds.TrimUnusedSpace();
    return ValidateVertexCounts(*geoset, parse, status) && !parse.FoundError();
  }

  BOOL WriteGeosets(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
    int writeMaterialId = data.materials.Count() != 0;
    for (UINT i = 0; i < data.geosets.Count(); ++i) {
      IWriteGeosetSection(data.geosets[i], writeMaterialId, buffer);
    }
    return 1;
  }

  BOOL ReadGeosetAnim(Parser &parse, MDLDATA &data, CMDLStatus *status) {
    TSet                  errors;
    MDLGEOSETANIMSECTION *section = data.geosetAnims.New();
    section->geosetId = data.geosetAnims.Count() - 1;
    IGeosetAnimAddErrors(errors);
    parse.Expect('{');
    LPCSTR tokentext;
    UINT   token = parse.Token(&tokentext, 0);
    while (token && token != '}') {
      if (!IReadGeosetAnim(parse, token, tokentext, &errors, section)) {
        if (token == 0x150) {
          section->geosetId = parse.ExpectInt();
        } else {
          parse.FatalUnexpected(tokentext);
        }
        parse.Expect(',');
      }
      token = parse.Token(&tokentext, 0);
    }
    parse.Expect('}', token, tokentext);
    errors.Complete(status);
    return !parse.FoundError();
  }

  static void IWriteGeosetAnimSection(const MDLGEOSETANIMSECTION &section, TSGrowableArray<char> &buffer) {
    WriteLine(buffer, "%s {\n", TokenText(0x10B));
    LPCSTR indent = "\t";
    if (section.alphaKeys.keys.Count()) {
      const MDLKEYTRACK<float> &track = section.alphaKeys;
      WriteLine(buffer, "%s%s %d {\n", indent, TokenText(0x11C), track.keys.Count());
      WriteTrackHeader(indent, track, buffer);
      for (UINT i = 0; i < track.keys.Count(); ++i) {
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
      WriteLine(buffer, "%s%s %s ", indent, TokenText(0x1BB), TokenText(0x11C));
      WriteKeyData(buffer, &section.staticAlpha, 1);
    }

    if (section.flags & 1) {
      if (section.colorKeys.keys.Count()) {
        const MDLKEYTRACK<C3Color> &track = section.colorKeys;
        WriteLine(buffer, "%s%s %d {\n", indent, TokenText(0x136), track.keys.Count());
        WriteTrackHeader(indent, track, buffer);
        for (UINT i = 0; i < track.keys.Count(); ++i) {
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
        WriteLine(buffer, "%s%s %s ", indent, TokenText(0x1BB), TokenText(0x136));
        WriteKeyData(buffer, &section.staticColor.b, 3);
      }
    }
    WriteLine(buffer, "\t%s %d,\n", TokenText(0x150), section.geosetId);
    WriteLine(buffer, "}\n");
  }

  BOOL WriteGeosetAnims(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
    if (!static_cast<LPCSTR>(data.model.animationFile)[0]) {
      for (UINT i = 0; i < data.geosetAnims.Count(); ++i) {
        IWriteGeosetAnimSection(data.geosetAnims.Ptr()[i], buffer);
      }
    }
    return 1;
  }

  static BOOL IReadBinGeosetAnim(CMsgBuffer &buffer, MDLGEOSETANIMSECTION *geoAnim, UINT &totalRead, CMDLStatus *status) {
    UINT sectionLength = buffer.GetUint();
    geoAnim->geosetId = buffer.GetUint();
    geoAnim->staticAlpha = buffer.GetFloat();
    geoAnim->staticColor.b = buffer.GetFloat();
    geoAnim->staticColor.g = buffer.GetFloat();
    geoAnim->staticColor.r = buffer.GetFloat();
    geoAnim->flags = buffer.GetUint();
    UINT localRead = 28;
    while (localRead < sectionLength) {
      DWORD tag = buffer.GetDword();
      localRead += 4;
      if (tag == 'OAGK') {
        if (!ReadBinFloatKeyFrames(geoAnim->alphaKeys, buffer, localRead)) {
          return 0;
        }
      } else if (tag == 'CAGK') {
        if (!ReadBinFloatKeyFrames(geoAnim->colorKeys, buffer, localRead)) {
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

  BOOL ReadBinGeosetAnim(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status) {
    UINT totalRead = 4;
    UINT count = buf.GetUint();
    data.geosetAnims.ReserveSpace(count);
    while (totalRead < length) {
      MDLGEOSETANIMSECTION *section = data.geosetAnims.New();
      if (!section) {
        status->FatalFlunked("GeosetAnim", -1);
        return 0;
      }
      if (!IReadBinGeosetAnim(buf, section, totalRead, status)) {
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

  static UINT IGetBinGeosetAnimSectionSize(const MDLGEOSETANIMSECTION &section) {
    UINT size = 28;
    if (section.alphaKeys.keys.Count()) {
      UINT values = section.alphaKeys.type > TRACK_LINEAR ? 3 : 1;
      size += 16 + section.alphaKeys.keys.Count() * (4 + 4 * values);
    }
    if (section.colorKeys.keys.Count()) {
      UINT values = section.colorKeys.type > TRACK_LINEAR ? 9 : 3;
      size += 16 + section.colorKeys.keys.Count() * (4 + 4 * values);
    }
    return size;
  }

  static void IWriteBinGeosetAnimSection(const MDLGEOSETANIMSECTION &section, CMsgBuffer &buffer) {
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
      UINT values = track.type > TRACK_LINEAR ? 9 : 3;
      for (UINT i = 0; i < track.keys.Count(); ++i) {
        const MDLKEYFRAME<C3Color> &key = track.keys.Ptr()[i];
        buffer.AddInt(key.time);
        buffer.AddFloatArray(&key.value.b, values);
      }
    }
  }

  BOOL WriteBinGeosetAnims(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *) {
    if (!static_cast<LPCSTR>(data.model.animationFile)[0] && data.geosetAnims.Count()) {
      buf.AddDword('AOEG');
      UINT totalSize = 4;
      UINT i;
      for (i = 0; i < data.geosetAnims.Count(); ++i) {
        totalSize += IGetBinGeosetAnimSectionSize(data.geosetAnims.Ptr()[i]);
      }
      buf.AddUint(totalSize);
      buf.AddUint(data.geosetAnims.Count());
      for (i = 0; i < data.geosetAnims.Count(); ++i) {
        IWriteBinGeosetAnimSection(data.geosetAnims.Ptr()[i], buf);
      }
    }
    return 1;
  }

  static BOOL IReadBinPrimitiveTypes(DWORD magic, CMsgBuffer &buffer, TSGrowableArray<BYTE> *section, UINT *localBytesRead, CMDLStatus *status) {
    if (magic != 'PYTP') {
      status->Add(STATUS_ERROR, "Invalid primitives type section in Geoset.\n");
      return 0;
    }
    UINT count = buffer.GetUint();
    *localBytesRead += 4;
    if (count) {
      section->SetCount(count);
      buffer.GetData(section->Ptr(), count);
      *localBytesRead += count;
    }
    return 1;
  }

  static void IReadBinAnimBounds(CMsgBuffer &buffer, MDLGEOSETSECTION *section, UINT *bytesRead) {
    UINT count = buffer.GetUint();
    *bytesRead += 4;
    section->seqBounds.SetCount(count);
    for (UINT i = 0; i < count; ++i) {
      CMdlBounds &bounds = section->seqBounds[i];
      bounds.radius = buffer.GetFloat();
      buffer.GetFloatArray(&bounds.extent.b.x, 3);
      buffer.GetFloatArray(&bounds.extent.t.x, 3);
      *bytesRead += 28;
    }
  }

  static BOOL ReadBinGeosetTags(CMsgBuffer &buffer, MDLGEOSETSECTION *geoset, CMDLStatus *status, UINT *localBytesRead) {
    if (!ReadBinC3VectorSection(buffer, 'XTRV', "vertex", &geoset->vertices, localBytesRead, status)) {
      return 0;
    }
    if (!ReadBinC3VectorSection(buffer, 'SMRN', "normal", &geoset->normals, localBytesRead, status)) {
      return 0;
    }

    DWORD magic = buffer.GetDword();
    *localBytesRead += 4;
    if (magic == 'SAVU') {
      UINT channels = buffer.GetUint();
      *localBytesRead += 4;
      FATALASSERT(channels);
      geoset->texCoords.SetCount(channels);
      UINT vertexCount = geoset->vertices.Count();
      for (UINT i = 0; i < channels; ++i) {
        geoset->texCoords[i].SetCount(vertexCount);
        buffer.GetFloatArray(&geoset->texCoords[i].Ptr()->x, 2 * vertexCount);
      }
      *localBytesRead += 8 * channels * vertexCount;
      magic = buffer.GetDword();
      *localBytesRead += 4;
    }

    if (!IReadBinPrimitiveTypes(magic, buffer, &geoset->primitives.types, localBytesRead, status)) {
      return 0;
    }

    magic = buffer.GetDword();
    *localBytesRead += 4;
    if (magic != 'TNCP') {
      status->Add(STATUS_ERROR, "Invalid %s section.\n", "primitives count");
      return 0;
    }
    UINT count = buffer.GetUint();
    *localBytesRead += 4;
    if (count) {
      geoset->primitives.counts.SetCount(count);
      buffer.GetUintArray(geoset->primitives.counts.Ptr(), count);
      *localBytesRead += 4 * count;
    }

    magic = buffer.GetDword();
    *localBytesRead += 4;
    if (magic != 'XTVP') {
      status->Add(STATUS_ERROR, "Invalid %s section.\n", "primitives vertices");
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
      status->Add(STATUS_ERROR, "Invalid %s section.\n", "vertex group indices");
      return 0;
    }
    count = buffer.GetUint();
    *localBytesRead += 4;
    if (count) {
      geoset->vertGroupIndices.SetCount(count);
      buffer.GetData(geoset->vertGroupIndices.Ptr(), count);
      *localBytesRead += count;
    }

    return IReadBinUintSection(buffer, 'CGTM', "group matrix counts", &geoset->groupMatrixCounts, localBytesRead, status) &&
           IReadBinUintSection(buffer, 'STAM', "matrices", &geoset->matrices, localBytesRead, status) &&
           IReadBinUintSection(buffer, 'XDIB', "bone indices", &geoset->boneIndices, localBytesRead, status) &&
           IReadBinUintSection(buffer, 'TGWB', "bone weights", &geoset->boneWeights, localBytesRead, status);
  }

  static BOOL ReadBinGeoset(CMsgBuffer &buffer, MDLGEOSETSECTION *geoset, CMDLStatus *status, UINT &totalLength) {
    UINT sectionLength = buffer.GetUint();
    UINT localBytesRead = 4;
    if (!ReadBinGeosetTags(buffer, geoset, status, &localBytesRead)) {
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

  BOOL ReadBinGeosets(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status) {
    FATALASSERT(status);
    UINT totalRead = 4;
    UINT numGeosets = buf.GetUint();
    data.geosets.SetCount(0);
    data.geosets.ReserveSpace(numGeosets);
    while (totalRead < length) {
      MDLGEOSETSECTION *section = data.geosets.New();
      if (!ReadBinGeoset(buf, section, status, totalRead)) {
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

  static UINT GetBinGeosetSize(const MDLGEOSETSECTION &section) {
    UINT size = 4;
    size += 8 + 12 * section.vertices.Count();
    size += 8 + 12 * section.normals.Count();
    if (section.texCoords.Count()) {
      size += 8 + 8 * section.texCoords.Count() * section.vertices.Count();
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

  static void IWriteBinAnimBounds(const MDLGEOSETSECTION &section, CMsgBuffer &buffer) {
    buffer.AddUint(section.seqBounds.Count());
    for (UINT i = 0; i < section.seqBounds.Count(); ++i) {
      const CMdlBounds &bounds = section.seqBounds[i];
      buffer.AddFloat(bounds.radius);
      buffer.AddFloatArray(&bounds.extent.b.x, 3);
      buffer.AddFloatArray(&bounds.extent.t.x, 3);
    }
  }

  static void WriteBinGeoset(CMsgBuffer &buffer, const MDLGEOSETSECTION &section) {
    buffer.AddUint(GetBinGeosetSize(section));
    WriteBinC3VectorSection(buffer, 'XTRV', section.vertices);
    WriteBinC3VectorSection(buffer, 'SMRN', section.normals);
    if (section.texCoords.Count()) {
      buffer.AddDword('SAVU');
      buffer.AddUint(section.texCoords.Count());
      UINT floats = 2 * section.vertices.Count();
      for (UINT i = 0; i < section.texCoords.Count(); ++i) {
        buffer.AddFloatArray(&section.texCoords[i].Ptr()->x, floats);
      }
    }
    buffer.AddDword('PYTP');
    buffer.AddUint(section.primitives.types.Count());
    buffer.AddData(section.primitives.types.Ptr(), section.primitives.types.Count());
    buffer.AddDword('TNCP');
    buffer.AddUint(section.primitives.counts.Count());
    buffer.AddUintArray(section.primitives.counts.Ptr(), section.primitives.counts.Count());
    buffer.AddDword('XTVP');
    buffer.AddUint(section.primitives.vertices.Count());
    buffer.AddWordArray(section.primitives.vertices.Ptr(), section.primitives.vertices.Count());
    buffer.AddDword('XDNG');
    buffer.AddUint(section.vertGroupIndices.Count());
    buffer.AddData(section.vertGroupIndices.Ptr(), section.vertGroupIndices.Count());
    buffer.AddDword('CGTM');
    buffer.AddUint(section.groupMatrixCounts.Count());
    buffer.AddUintArray(section.groupMatrixCounts.Ptr(), section.groupMatrixCounts.Count());
    buffer.AddDword('STAM');
    buffer.AddUint(section.matrices.Count());
    buffer.AddUintArray(section.matrices.Ptr(), section.matrices.Count());
    buffer.AddDword('XDIB');
    buffer.AddUint(section.boneIndices.Count());
    buffer.AddUintArray(section.boneIndices.Ptr(), section.boneIndices.Count());
    buffer.AddDword('TGWB');
    buffer.AddUint(section.boneWeights.Count());
    buffer.AddUintArray(section.boneWeights.Ptr(), section.boneWeights.Count());
    buffer.AddUint(section.materialId);
    buffer.AddUint(section.selectionGroup);
    buffer.AddUint(section.flags);
    buffer.AddFloat(section.bounds.radius);
    buffer.AddFloatArray(&section.bounds.extent.b.x, 3);
    buffer.AddFloatArray(&section.bounds.extent.t.x, 3);
    IWriteBinAnimBounds(section, buffer);
  }

  BOOL WriteBinGeosets(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *) {
    if (data.geosets.Count()) {
      buf.AddDword('SOEG');
      UINT totalSize = 4;
      UINT i;
      for (i = 0; i < data.geosets.Count(); ++i) {
        totalSize += GetBinGeosetSize(data.geosets[i]);
      }
      buf.AddUint(totalSize);
      buf.AddUint(data.geosets.Count());
      for (i = 0; i < data.geosets.Count(); ++i) {
        WriteBinGeoset(buf, data.geosets[i]);
      }
    }
    return 1;
  }

}  // namespace MDL
