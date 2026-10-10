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
  BOOL         ReadGeoset(Parser &, MDLDATA &, CMDLStatus *);
  BOOL         WriteGeosets(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL         ReadGeosetAnim(Parser &, MDLDATA &, CMDLStatus *);
  BOOL         WriteGeosetAnims(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL         ReadBinGeosetAnim(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  BOOL         WriteBinGeosetAnims(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  BOOL         ReadBinGeosets(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  BOOL         WriteBinGeosets(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
}  // namespace MDL

static void IGeosetAddErrors(TSet &errors) {
  errors.Add(MDLTOK_VERTICES, 1, 0);
  errors.Add(MDLTOK_NORMALS, 0, 0);
  errors.Add(MDLTOK_FACES, 1, 0);
  errors.Add(MDLTOK_GROUPS, 1, 0);
  errors.Add(MDLTOK_MATERIAL_ID, 0, 0);
  errors.Add(MDLTOK_MINIMUMEXTENT, 0, 0);
  errors.Add(MDLTOK_MAXIMUMEXTENT, 0, 0);
  errors.Add(MDLTOK_BOUNDS_RADIUS, 0, 0);
  errors.Add(MDLTOK_SELECTION_GROUP, 0, 0);
  errors.Add(MDLTOK_UNSELECTABLE, 0, 0);
  errors.Add(MDLTOK_BONE_INDICES, 0, 0);
  errors.Add(MDLTOK_BONE_WEIGHTS, 0, 0);
}

static void IGeosetAnimAddErrors(TSet &errors) {
  errors.Add(MDLTOK_ALPHA, 0, 0);
  errors.Add(MDLTOK_COLOR, 0, 0);
  errors.Add(MDLTOK_GEOSETID, 0, 0);
}

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

static void IReadTVertices(Parser &parse, TSGrowableArray<NTempest::C2Vector> *texcoords) {
  UINT   savedtoken;
  LPCSTR tokentext;
  long   actual = 0;
  long   count = parse.GetOptionalInt(&savedtoken, &tokentext, 0);
  if (count > 0) {
    texcoords->ReserveSpace(count);
  }
  parse.Expect('{', savedtoken, tokentext);
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
  if (count >= 0 && actual != count) {
    parse.WarningCount("vertices", count, actual);
  }
}

static void ISkipDuplicates(Parser &parse) {
  UINT       savedtoken;
  LPCSTR     tokentext;
  UTokenData savedvalue;
  parse.GetOptionalInt(&savedtoken, &tokentext, 0);
  parse.Expect('{', savedtoken, tokentext);
  savedtoken = parse.Token(&tokentext, &savedvalue);
  while (savedtoken == MDLTOK_LONG) {
    parse.Expect(',');
    savedtoken = parse.Token(&tokentext, &savedvalue);
  }
  parse.Expect('}', savedtoken, tokentext);
}

static UINT IVertexList(Parser &parse, TSGrowableArray<WORD> *vertlist) {
  FATALASSERT(vertlist);
  vertlist->New()[0] = parse.ExpectInt();
  UINT   entries = 1;
  LPCSTR tokentext;
  UINT   token = parse.Token(&tokentext, 0);
  while (token == ',') {
    vertlist->New()[0] = parse.ExpectInt();
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

void WriteVertices(const TSGrowableArray<NTempest::C3Vector> &vertices, UINT title, TSGrowableArray<char> &buffer) {
  MDL::WriteLine(buffer, "\t%s %d {\n", MDL::TokenText(title), vertices.Count());
  const NTempest::C3Vector *vertex = vertices.Ptr();
  for (UINT i = vertices.Count(); i; --i, ++vertex) {
    MDL::WriteLine(buffer, "\t\t{ %g, %g, %g },\n", vertex->x, vertex->y, vertex->z);
  }
  MDL::WriteLine(buffer, "\t}\n");
}

void WriteBinC3VectorSection(CMsgBuffer &buf, DWORD title, const TSGrowableArray<NTempest::C3Vector> &section) {
  UINT count = section.Count();
  buf.AddDword(title);
  buf.AddUint(count);
  buf.AddFloatArray(&section.Ptr()->x, 3 * count);
}

BOOL ReadBinC3VectorSection(
    CMsgBuffer                          &buf,
    DWORD                                title,
    LPCSTR                               name,
    TSGrowableArray<NTempest::C3Vector> *section,
    UINT                                *localBytesRead,
    CMDLStatus                          *status
) {
  DWORD found = buf.GetDword();
  if (found != title) {
    status->Add(STATUS_ERROR, "Invalid %s section in Geoset.\n", name);
    return 0;
  }
  UINT count = buf.GetUint();
  *localBytesRead += 8;
  if (!count) {
    return 1;
  }
  section->SetCount(count);
  *localBytesRead += count * sizeof(NTempest::C3Vector);
  buf.GetFloatArray(&section->Ptr()->x, 3 * count);
  return 1;
}

inline BOOL IReadBinUintSection(CMsgBuffer &buf, DWORD title, LPCSTR name, TSGrowableArray<UINT> *section, UINT *localBytesRead, CMDLStatus *status) {
  DWORD found = buf.GetDword();
  *localBytesRead += 4;
  if (found != title) {
    status->Add(STATUS_ERROR, "Invalid %s section.\n", name);
    return 0;
  }
  UINT count = buf.GetUint();
  *localBytesRead += 4;
  if (!count) {
    return 1;
  }
  section->SetCount(count);
  buf.GetUintArray(section->Ptr(), count);
  *localBytesRead += 4 * count;
  return 1;
}

void SetVertexGroupIndices(const TSGrowableArray<UINT> &groupVertexCounts, MDLGEOSETSECTION *geoset) {
  geoset->vertGroupIndices.SetCount(geoset->vertices.Count());
  UINT numGroups = groupVertexCounts.Count();
  if (numGroups < 2) {
    geoset->vertGroupIndices.Zero();
    return;
  }
  const UINT *vertCount = groupVertexCounts.Ptr();
  BYTE       *groupId = geoset->vertGroupIndices.Ptr();
  for (UINT i = 0; i < numGroups; ++i, ++vertCount) {
    UINT count = *vertCount;
    if (count) {
      memset(groupId, i, count);
      groupId += count;
    }
  }
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
  long       actualVerts = 0;
  long       actualPrimitives = 0;
  long       estVerts = -1;
  long       estPrims = parse.GetOptionalInt(&savedtoken, &tokentext, &savedvalue);
  if (estPrims > 0) {
    estVerts = parse.GetOptionalInt(savedtoken, &savedvalue, &savedtoken, &tokentext);
    primitives->ReserveSpace(estPrims, estVerts > 0 ? estVerts : 3 * estPrims);
  }
  parse.Expect('{', savedtoken, tokentext);
  savedtoken = parse.Token(&tokentext, 0);
  while (savedtoken != '}' && savedtoken) {
    switch (savedtoken) {
      case MDLTOK_POINTS:
        actualPrimitives += IMultiPoints(parse, primitives, &actualVerts, 0);
        break;
      case MDLTOK_LINES:
        actualPrimitives += ILines(parse, primitives, &actualVerts);
        break;
      case MDLTOK_LINE_LOOP:
        actualPrimitives += ILineStripLoop(parse, primitives, &actualVerts, 2);
        break;
      case MDLTOK_LINE_STRIP:
        actualPrimitives += ILineStripLoop(parse, primitives, &actualVerts, 3);
        break;
      case MDLTOK_TRIANGLES:
        actualPrimitives += ITriangles(parse, primitives, &actualVerts);
        break;
      case MDLTOK_TRIANGLE_STRIP:
        actualPrimitives += ITriangleFanStrip(parse, primitives, &actualVerts, 5);
        break;
      case MDLTOK_TRIANGLE_FAN:
        actualPrimitives += ITriangleFanStrip(parse, primitives, &actualVerts, 6);
        break;
      case MDLTOK_QUADS:
        actualPrimitives += IQuads(parse, primitives, &actualVerts);
        break;
      case MDLTOK_QUAD_STRIP:
        actualPrimitives += IQuadStrip(parse, primitives, &actualVerts);
        break;
      case MDLTOK_POLYGON:
        actualPrimitives += IMultiPoints(parse, primitives, &actualVerts, 9);
        break;
      default:
        parse.FatalUnexpected(tokentext);
        break;
    }
    savedtoken = parse.Token(&tokentext, 0);
  }
  parse.Expect('}', savedtoken, tokentext);
  if (estVerts >= 0 && actualVerts != estVerts) {
    parse.WarningCount("primitive vertices", estVerts, actualVerts);
  }
  if (estPrims >= 0 && actualPrimitives != estPrims) {
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
  errors.Add(MDLTOK_VERTEXCOUNT, 1, 0);
  errors.Add(MDLTOK_MATRICES, 1, 0);
  UINT *vertexCount = groupVertexCounts->New();
  UINT *matrixCount = geoset->groupMatrixCounts.New();
  parse.Expect('{');
  LPCSTR tokentext;
  UINT   token = parse.Token(&tokentext, 0);
  while (token != '}' && token) {
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokentext);
    }
    switch (token) {
      case MDLTOK_VERTEXCOUNT:
        *vertexCount = parse.ExpectInt();
        break;
      case MDLTOK_MATRICES:
        IReadMatrices(parse, matrixCount, &geoset->matrices);
        *numMatrices += *matrixCount;
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
}

static void IReadGroups(Parser &parse, MDLGEOSETSECTION *geoset, CMDLStatus *status) {
  UINT       savedtoken;
  long       actualGroups = 0;
  long       estGroups;
  LPCSTR     tokentext;
  UTokenData savedvalue;
  long       estMatrices = -1;
  long       actualMatrices = 0;
  estGroups = parse.GetOptionalInt(&savedtoken, &tokentext, &savedvalue);
  if (estGroups > 0) {
    estMatrices = parse.GetOptionalInt(savedtoken, &savedvalue, &savedtoken, &tokentext);
    geoset->groupMatrixCounts.ReserveSpace(estGroups);
    if (estMatrices > 0) {
      geoset->matrices.ReserveSpace(estMatrices);
    }
  }
  parse.Expect('{', savedtoken, tokentext);
  savedtoken = parse.Token(&tokentext, 0);
  switch (savedtoken) {
    case MDLTOK_GROUP: {
      TSGrowableArray<UINT> groupVertexCounts;
      if (estGroups > 0) {
        groupVertexCounts.ReserveSpace(estGroups);
      }
      do {
        IReadGroup(parse, geoset, &actualMatrices, &groupVertexCounts, status);
        ++actualGroups;
      } while ((savedtoken = parse.Token(&tokentext, 0)) == MDLTOK_GROUP);
      SetVertexGroupIndices(groupVertexCounts, geoset);
      break;
    }
    case MDLTOK_MATRICES:
      do {
        UINT *count = geoset->groupMatrixCounts.New();
        IReadMatrices(parse, count, &geoset->matrices);
        parse.Expect(',');
        actualMatrices += *count;
        ++actualGroups;
      } while ((savedtoken = parse.Token(&tokentext, 0)) == MDLTOK_MATRICES);
      break;
  }
  parse.Expect('}', savedtoken, tokentext);
  if (estGroups >= 0 && actualGroups != estGroups) {
    parse.WarningCount("groups", estGroups, actualGroups);
  }
  if (estMatrices >= 0 && actualMatrices != estMatrices) {
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
  while (token == MDLTOK_LONG) {
    *geoset->vertGroupIndices.New() = savedvalue.cVal;
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
  errors.Add(MDLTOK_MINIMUMEXTENT, 0, 0);
  errors.Add(MDLTOK_MAXIMUMEXTENT, 0, 0);
  errors.Add(MDLTOK_BOUNDS_RADIUS, 0, 0);
}

static void IReadAnimBounds(Parser &parse, CMdlBounds *bounds, CMDLStatus *status) {
  parse.Expect('{');
  TSet   errors;
  LPCSTR tokentext;
  UINT   token = parse.Token(&tokentext, 0);
  IAnimBoundsAddErrors(errors);
  while (token != '}' && token) {
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokentext);
    }
    switch (token) {
      case MDLTOK_MINIMUMEXTENT:
        IReadVertex(parse, &bounds->extent.b);
        break;
      case MDLTOK_MAXIMUMEXTENT:
        IReadVertex(parse, &bounds->extent.t);
        break;
      case MDLTOK_BOUNDS_RADIUS:
        bounds->radius = parse.ExpectFloat();
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

static BOOL IReadAlpha(Parser &parse, int expectanimation, MDLGEOSETANIMSECTION *geoset) {
  if (!expectanimation) {
    geoset->staticAlpha = parse.ExpectFloat();
    return 0;
  }
  ReadObjectFloatKeyframes(parse, &geoset->alphaKeys);
  return 1;
}

static BOOL IReadColor(Parser &parse, int expectanimation, MDLGEOSETANIMSECTION *geoset) {
  if (!expectanimation) {
    ReadFloatKeyData(parse, &geoset->staticColor.b, 3);
    return 0;
  }
  ReadObjectFloatKeyframes(parse, &geoset->colorKeys);
  return 1;
}

static BOOL IllegalStaticToken(UINT token) {
  switch (token) {
    case MDLTOK_ALPHA:
    case MDLTOK_COLOR:
    case MDLTOK_OPACITY:
      return 0;
    default:
      return 1;
  }
}

static BOOL IReadGeosetAnim(Parser &parse, UINT savedtoken, LPCSTR tokentext, TSet *errors, MDLGEOSETANIMSECTION *geoAnim) {
  int expectanimation = 0;
  if (geoAnim) {
    expectanimation = IExpectAnimation(parse, &savedtoken, &tokentext);
    if (!expectanimation && IllegalStaticToken(savedtoken)) {
      parse.FatalUnexpected(tokentext);
    }
  }
  if (!errors->Check(savedtoken)) {
    parse.FatalDuplicate(tokentext);
  }
  if (!geoAnim) {
    return 0;
  }
  switch (savedtoken) {
    case MDLTOK_ALPHA:
    case MDLTOK_OPACITY:
      if (IReadAlpha(parse, expectanimation, geoAnim)) {
        return 1;
      }
      break;
    case MDLTOK_COLOR:
      geoAnim->flags |= 1;
      if (IReadColor(parse, expectanimation, geoAnim)) {
        return 1;
      }
      break;
    default:
      return 0;
  }
  parse.Expect(',');
  return 1;
}

static void IWriteGeosetTexCoords(const TSGrowableArray<NTempest::C2Vector> &texcoords, TSGrowableArray<char> &buffer) {
  if (!texcoords.Count()) {
    return;
  }
  MDL::WriteLine(buffer, "\t%s %d {\n", MDL::TokenText(MDLTOK_TVERTICES), texcoords.Count());
  const NTempest::C2Vector *coord = texcoords.Ptr();
  for (UINT i = texcoords.Count(); i; --i, ++coord) {
    MDL::WriteLine(buffer, "\t\t{ %g, %g },\n", coord->x, coord->y);
  }
  MDL::WriteLine(buffer, "\t}\n");
}

static void IWriteVertexGroupIndices(const TSGrowableArray<BYTE> &vertGroupIndices, TSGrowableArray<char> &buffer) {
  MDL::WriteLine(buffer, "\t%s {\n", MDL::TokenText(MDLTOK_VERTEX_GROUP));
  const BYTE *index = vertGroupIndices.Ptr();
  for (UINT i = vertGroupIndices.Count(); i; --i, ++index) {
    MDL::WriteLine(buffer, "\t\t%u,\n", *index);
  }
  MDL::WriteLine(buffer, "\t}\n");
}

static LPCSTR IGetPrimitiveText(BYTE type) {
  switch (type) {
    case 0:
      return MDL::TokenText(MDLTOK_POINTS);
    case 1:
      return MDL::TokenText(MDLTOK_LINES);
    case 2:
      return MDL::TokenText(MDLTOK_LINE_LOOP);
    case 3:
      return MDL::TokenText(MDLTOK_LINE_STRIP);
    case 4:
      return MDL::TokenText(MDLTOK_TRIANGLES);
    case 5:
      return MDL::TokenText(MDLTOK_TRIANGLE_STRIP);
    case 6:
      return MDL::TokenText(MDLTOK_TRIANGLE_FAN);
    case 7:
      return MDL::TokenText(MDLTOK_QUADS);
    case 8:
      return MDL::TokenText(MDLTOK_QUAD_STRIP);
    case 9:
      return MDL::TokenText(MDLTOK_POLYGON);
    default:
      return MDL::TokenText(MDLTOK_UNKNOWN);
  }
}

static void IWriteGeosetPrimitives(const MDLPRIMITIVES &faces, TSGrowableArray<char> &buffer) {
  MDL::WriteLine(buffer, "\t%s %d %d {\n", MDL::TokenText(MDLTOK_FACES), faces.types.Count(), faces.vertices.Count());
  for (int primtype = 0; primtype < 10; ++primtype) {
    int          foundType = 0;
    const BYTE  *primType = faces.types.Ptr();
    const UINT  *vertCount = faces.counts.Ptr();
    const WORD  *vertex = faces.vertices.Ptr();
    for (UINT i = faces.types.Count(); i; --i, ++primType, vertex += *vertCount++) {
      if (*primType == primtype) {
        if (!foundType) {
          MDL::WriteLine(buffer, "\t\t%s {\n", IGetPrimitiveText(*primType));
        }
        foundType = 1;
        const WORD *index = vertex;
        MDL::WriteLine(buffer, "\t\t\t{ %d", *index++);
        for (UINT j = *vertCount; --j; ++index) {
          MDL::WriteLine(buffer, ", %d", *index);
        }
        MDL::WriteLine(buffer, " },\n");
      }
    }
    if (foundType) {
      MDL::WriteLine(buffer, "\t\t}\n");
    }
  }
  MDL::WriteLine(buffer, "\t}\n");
}

static void IWriteGeosetGroups(const MDLGEOSETSECTION &section, TSGrowableArray<char> &buffer) {
  UINT        numGroups = section.groupMatrixCounts.Count();
  const UINT *numMatrices;
  const UINT *matrix;
  MDL::WriteLine(buffer, "\t%s %u %u {\n", MDL::TokenText(MDLTOK_GROUPS), numGroups, section.matrices.Count());
  numMatrices = section.groupMatrixCounts.Ptr();
  matrix = section.matrices.Ptr();
  for (; numGroups; --numGroups, ++numMatrices) {
    MDL::WriteLine(buffer, "\t\t%s { ", MDL::TokenText(MDLTOK_MATRICES));
    for (UINT count = *numMatrices; count--; ++matrix) {
      MDL::WriteLine(buffer, "%u", *matrix);
      if (count > 0) {
        MDL::WriteLine(buffer, ", ");
      }
    }
    MDL::WriteLine(buffer, " },\n");
  }
  MDL::WriteLine(buffer, "\t}\n");
}

static void IWriteBoneWeights(const MDLGEOSETSECTION &section, TSGrowableArray<char> &buffer) {
  UINT count = section.boneIndices.Count();
  MDL::WriteLine(buffer, "\t%s %u {\n", MDL::TokenText(MDLTOK_BONE_INDICES), count);
  for (UINT i = 0; i < count; ++i) {
    MDL::WriteLine(buffer, "\t\t0x%08X,\n", section.boneIndices[i]);
  }
  MDL::WriteLine(buffer, "\t}\n");
  MDL::WriteLine(buffer, "\t%s %u {\n", MDL::TokenText(MDLTOK_BONE_WEIGHTS), count);
  for (UINT j = 0; j < count; ++j) {
    MDL::WriteLine(buffer, "\t\t0x%08X,\n", section.boneWeights[j]);
  }
  MDL::WriteLine(buffer, "\t}\n");
}

static void IWriteAnimBounds(const TSGrowableArray<CMdlBounds> &geoBounds, TSGrowableArray<char> &buffer) {
  const CMdlBounds *bounds = geoBounds.Ptr();
  for (UINT i = geoBounds.Count(); i; --i, ++bounds) {
    MDL::WriteLine(buffer, "\t%s {\n", MDL::TokenText(MDLTOK_ANIM));
    WriteBounds(*bounds, "\t\t", buffer);
    MDL::WriteLine(buffer, "\t}\n");
  }
}

static void IWriteGeosetSection(const MDLGEOSETSECTION &section, int writeMaterialId, TSGrowableArray<char> &buffer) {
  MDL::WriteLine(buffer, "%s {\n", MDL::TokenText(MDLTOK_GEOSET));
  WriteVertices(section.vertices, MDLTOK_VERTICES, buffer);
  WriteVertices(section.normals, MDLTOK_NORMALS, buffer);
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
    MDL::WriteLine(buffer, "\t%s %d,\n", MDL::TokenText(MDLTOK_MATERIAL_ID), section.materialId);
  }
  MDL::WriteLine(buffer, "\t%s %d,\n", MDL::TokenText(MDLTOK_SELECTION_GROUP), section.selectionGroup);
  if (section.flags & 1) {
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(MDLTOK_UNSELECTABLE));
  }
  MDL::WriteLine(buffer, "}\n");
}


BOOL MDL::ReadGeoset(Parser &parse, MDLDATA &data, CMDLStatus *status) {
  LPCSTR                tokentext;
  MDLGEOSETSECTION     *geoset = data.geosets.New();
  TSet                  errors;
  MDLGEOSETANIMSECTION *geoAnim = 0;
  if (data.version < 600) {
    geoAnim = data.geosetAnims.New();
    IGeosetAnimAddErrors(errors);
    geoAnim->geosetId = data.geosets.Count() - 1;
  }
  geoset->seqBounds.ReserveSpace(data.sequences.Count());
  IGeosetAddErrors(errors);
  parse.Expect('{');
  UINT token;
  for (token = parse.Token(&tokentext, 0); token != '}' && token; token = parse.Token(&tokentext, 0)) {
    if (IReadGeosetAnim(parse, token, tokentext, &errors, geoAnim)) {
      continue;
    }
    switch (token) {
      case MDLTOK_DUPLICATES:
        ISkipDuplicates(parse);
        continue;
      case MDLTOK_FACES:
        IReadPrimitives(parse, &geoset->primitives);
        continue;
      case MDLTOK_GROUPS:
        IReadGroups(parse, geoset, status);
        continue;
      case MDLTOK_MATERIAL_ID:
        geoset->materialId = parse.ExpectInt();
        break;
      case MDLTOK_VERTICES:
        ReadVertices(parse, "vertices", &geoset->vertices);
        continue;
      case MDLTOK_NORMALS:
        ReadVertices(parse, "normals", &geoset->normals);
        continue;
      case MDLTOK_TVERTICES:
        IReadTVertices(parse, geoset->texCoords.New());
        continue;
      default:
        parse.FatalUnexpected(tokentext);
        break;
      case MDLTOK_VERTEX_GROUP:
        IReadVertexGroupIds(parse, geoset);
        continue;
      case MDLTOK_SELECTION_GROUP:
        geoset->selectionGroup = parse.ExpectInt();
        break;
      case MDLTOK_MINIMUMEXTENT:
        IReadVertex(parse, &geoset->bounds.extent.b);
        break;
      case MDLTOK_MAXIMUMEXTENT:
        IReadVertex(parse, &geoset->bounds.extent.t);
        break;
      case MDLTOK_BOUNDS_RADIUS:
        geoset->bounds.radius = parse.ExpectFloat();
        break;
      case MDLTOK_ANIM:
        IReadAnimBounds(parse, geoset->seqBounds.New(), status);
        continue;
      case MDLTOK_BONE_WEIGHTS:
        IReadBoneWeights(parse, geoset);
        continue;
      case MDLTOK_BONE_INDICES:
        IReadBoneIndices(parse, geoset);
        continue;
      case MDLTOK_UNSELECTABLE:
        geoset->flags |= 1;
        break;
    }
    parse.Expect(',');
  }
  parse.Expect('}', token, tokentext);
  errors.Complete(status);
  geoset->seqBounds.TrimUnusedSpace();
  return ValidateVertexCounts(*geoset, parse, status) && !parse.FoundError();
}

BOOL MDL::WriteGeosets(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
  int writeMaterialId = data.materials.Count() > 0;
  for (UINT i = 0; i < data.geosets.Count(); ++i) {
    IWriteGeosetSection(data.geosets[i], writeMaterialId, buffer);
  }
  return 1;
}

BOOL MDL::ReadGeosetAnim(Parser &parse, MDLDATA &data, CMDLStatus *status) {
  TSet                  errors;
  MDLGEOSETANIMSECTION *section = data.geosetAnims.New();
  section->geosetId = data.geosetAnims.Count() - 1;
  IGeosetAnimAddErrors(errors);
  parse.Expect('{');
  LPCSTR tokentext;
  UINT   token = parse.Token(&tokentext, 0);
  while (token != '}' && token) {
    if (!IReadGeosetAnim(parse, token, tokentext, &errors, section)) {
      switch (token) {
        case MDLTOK_GEOSETID:
          section->geosetId = parse.ExpectInt();
          break;
        default:
          parse.FatalUnexpected(tokentext);
          break;
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
  MDL::WriteLine(buffer, "%s {\n", MDL::TokenText(MDLTOK_GEOSETANIM));
  LPCSTR indent = "\t";
  if (section.alphaKeys.keys.Count()) {
    const MDLKEYTRACK<float> &track = section.alphaKeys;
    MDL::WriteLine(buffer, "%s%s %d {\n", indent, MDL::TokenText(MDLTOK_ALPHA), track.keys.Count());
    WriteTrackHeader(indent, track, buffer);
    for (UINT i = 0; i < track.keys.Count(); ++i) {
      const MDLKEYFRAME<float> &key = track.keys.Ptr()[i];
      MDL::WriteLine(buffer, "%s\t%d: ", indent, key.time);
      WriteKeyData(buffer, &key.value, 1);
      if (track.type > TRACK_LINEAR) {
        MDL::WriteLine(buffer, "%s\t\t%s ", indent, MDL::TokenText(MDLTOK_INTAN));
        WriteKeyData(buffer, &key.inTan, 1);
        MDL::WriteLine(buffer, "%s\t\t%s ", indent, MDL::TokenText(MDLTOK_OUTTAN));
        WriteKeyData(buffer, &key.outTan, 1);
      }
    }
    MDL::WriteLine(buffer, "%s}\n", indent);
  } else if (section.staticAlpha < 1.0f) {
    MDL::WriteLine(buffer, "%s%s %s ", indent, MDL::TokenText(MDLTOK_STATIC), MDL::TokenText(MDLTOK_ALPHA));
    WriteKeyData(buffer, &section.staticAlpha, 1);
  }

  if (section.flags & 1) {
    if (section.colorKeys.keys.Count()) {
      const MDLKEYTRACK<C3Color> &track = section.colorKeys;
      MDL::WriteLine(buffer, "%s%s %d {\n", indent, MDL::TokenText(MDLTOK_COLOR), track.keys.Count());
      WriteTrackHeader(indent, track, buffer);
      for (UINT i = 0; i < track.keys.Count(); ++i) {
        const MDLKEYFRAME<C3Color> &key = track.keys.Ptr()[i];
        MDL::WriteLine(buffer, "%s\t%d: ", indent, key.time);
        WriteKeyData(buffer, &key.value.b, 3);
        if (track.type > TRACK_LINEAR) {
          MDL::WriteLine(buffer, "%s\t\t%s ", indent, MDL::TokenText(MDLTOK_INTAN));
          WriteKeyData(buffer, &key.inTan.b, 3);
          MDL::WriteLine(buffer, "%s\t\t%s ", indent, MDL::TokenText(MDLTOK_OUTTAN));
          WriteKeyData(buffer, &key.outTan.b, 3);
        }
      }
      MDL::WriteLine(buffer, "%s}\n", indent);
    } else {
      MDL::WriteLine(buffer, "%s%s %s ", indent, MDL::TokenText(MDLTOK_STATIC), MDL::TokenText(MDLTOK_COLOR));
      WriteKeyData(buffer, &section.staticColor.b, 3);
    }
  }
  MDL::WriteLine(buffer, "\t%s %d,\n", MDL::TokenText(MDLTOK_GEOSETID), section.geosetId);
  MDL::WriteLine(buffer, "}\n");
}

BOOL MDL::WriteGeosetAnims(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
  if (((LPCSTR)data.model.animationFile)[0]) {
    return 1;
  }
  for (UINT i = 0; i < data.geosetAnims.Count(); ++i) {
    IWriteGeosetAnimSection(data.geosetAnims[i], buffer);
  }
  return 1;
}

static BOOL IReadBinGeosetAnim(CMsgBuffer &buf, MDLGEOSETANIMSECTION *geoAnim, UINT &totalRead, CMDLStatus *status) {
  UINT sectionLength = buf.GetUint();
  UINT localBytesRead = 4;
  geoAnim->geosetId = buf.GetUint();
  localBytesRead += 4;
  geoAnim->staticAlpha = buf.GetFloat();
  geoAnim->staticColor.r = buf.GetFloat();
  geoAnim->staticColor.g = buf.GetFloat();
  geoAnim->staticColor.b = buf.GetFloat();
  localBytesRead += 16;
  geoAnim->flags = buf.GetUint();
  localBytesRead += 4;
  while (localBytesRead < sectionLength) {
    DWORD tag = buf.GetDword();
    localBytesRead += 4;
    switch (tag) {
      case 'OAGK':
        ReadBinFloatKeyFrames(geoAnim->alphaKeys, buf, localBytesRead);
        break;
      case 'CAGK':
        ReadBinFloatKeyFrames(geoAnim->colorKeys, buf, localBytesRead);
        break;
      default:
        SkipUnknown(buf, localBytesRead);
        break;
    }
    if (localBytesRead > sectionLength) {
      status->FatalOverran("GeosetAnim keys", -1);
      return 0;
    }
  }
  totalRead += localBytesRead;
  return 1;
}

BOOL MDL::ReadBinGeosetAnim(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status) {
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

static void IWriteBinGeosetAnimSection(const MDLGEOSETANIMSECTION &section, CMsgBuffer &buf) {
  buf.AddUint(IGetBinGeosetAnimSectionSize(section));
  buf.AddUint(section.geosetId);
  buf.AddFloat(section.staticAlpha);
  buf.AddFloat(section.staticColor.r);
  buf.AddFloat(section.staticColor.g);
  buf.AddFloat(section.staticColor.b);
  buf.AddUint(section.flags);
  UINT count = section.alphaKeys.keys.Count();
  if (count) {
    buf.AddDword('OAGK');
    buf.AddUint(count);
    buf.AddUint(section.alphaKeys.type);
    buf.AddUint(section.alphaKeys.globalSeqId);
    UINT values = 1;
    if (section.alphaKeys.type > TRACK_LINEAR) {
      values = 3;
    }
    const MDLKEYFRAME<float> *key = section.alphaKeys.keys.Ptr();
    for (UINT i = count; i; --i, ++key) {
      buf.AddInt(key->time);
      buf.AddFloatArray(&key->value, values);
    }
  }
  count = section.colorKeys.keys.Count();
  if (count) {
    buf.AddDword('CAGK');
    buf.AddUint(count);
    buf.AddUint(section.colorKeys.type);
    buf.AddUint(section.colorKeys.globalSeqId);
    UINT values = 3;
    if (section.colorKeys.type > TRACK_LINEAR) {
      values = 9;
    }
    const MDLKEYFRAME<C3Color> *key = section.colorKeys.keys.Ptr();
    for (UINT i = count; i; --i, ++key) {
      buf.AddInt(key->time);
      buf.AddFloatArray(&key->value.b, values);
    }
  }
}

BOOL MDL::WriteBinGeosetAnims(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *) {
  UINT numGeosetAnims = data.geosetAnims.Count();
  if (((LPCSTR)data.model.animationFile)[0] || !numGeosetAnims) {
    return 1;
  }
  buf.AddDword('AOEG');
  UINT totalSize = 4;
  UINT i;
  for (i = 0; i < numGeosetAnims; ++i) {
    totalSize += IGetBinGeosetAnimSectionSize(data.geosetAnims[i]);
  }
  buf.AddUint(totalSize);
  buf.AddUint(numGeosetAnims);
  for (i = 0; i < numGeosetAnims; ++i) {
    IWriteBinGeosetAnimSection(data.geosetAnims[i], buf);
  }
  return 1;
}

static BOOL IReadBinPrimitiveTypes(DWORD magic, CMsgBuffer &buf, TSGrowableArray<BYTE> *section, UINT *localBytesRead, CMDLStatus *status) {
  if (magic != 'PYTP') {
    status->Add(STATUS_ERROR, "Invalid primitives type section in Geoset.\n");
    return 0;
  }
  UINT count = buf.GetUint();
  *localBytesRead += 4;
  if (!count) {
    return 1;
  }
  section->SetCount(count);
  buf.GetData(section->Ptr(), count);
  *localBytesRead += section->Count();
  return 1;
}

static void IReadBinAnimBounds(CMsgBuffer &buf, MDLGEOSETSECTION *section, UINT *bytesRead) {
  UINT count = buf.GetUint();
  *bytesRead += 4;
  section->seqBounds.SetCount(count);
  CMdlBounds *bounds = section->seqBounds.Ptr();
  for (UINT i = count; i; --i, ++bounds) {
    bounds->radius = buf.GetFloat();
    *bytesRead += 4;
    buf.GetFloatArray(&bounds->extent.b.x, 3);
    buf.GetFloatArray(&bounds->extent.t.x, 3);
    *bytesRead += 24;
  }
}

static BOOL ReadBinGeosetTags(CMsgBuffer &buf, MDLGEOSETSECTION *pGeoset, CMDLStatus *status, UINT *localBytesRead) {
  if (!ReadBinC3VectorSection(buf, 'XTRV', "vertex", &pGeoset->vertices, localBytesRead, status)) {
    return 0;
  }
  if (!ReadBinC3VectorSection(buf, 'SMRN', "normal", &pGeoset->normals, localBytesRead, status)) {
    return 0;
  }

  UINT  count;
  DWORD magic = buf.GetDword();
  *localBytesRead += 4;
  if (magic == 'SAVU') {
    count = buf.GetUint();
    *localBytesRead += 4;
    FATALASSERT(count);
    pGeoset->texCoords.SetCount(count);
    UINT numVertices = pGeoset->vertices.Count();
    UINT floatsToRead = 2 * numVertices;
    for (UINT i = 0; i < count; ++i) {
      pGeoset->texCoords.Ptr()[i].SetCount(numVertices);
      buf.GetFloatArray(&pGeoset->texCoords[i].Ptr()->x, floatsToRead);
    }
    *localBytesRead += floatsToRead * count * 4;
    magic = buf.GetDword();
    *localBytesRead += 4;
  }

  if (!IReadBinPrimitiveTypes(magic, buf, &pGeoset->primitives.types, localBytesRead, status)) {
    return 0;
  }

  if (!IReadBinUintSection(buf, 'TNCP', "primitives count", &pGeoset->primitives.counts, localBytesRead, status)) {
    return 0;
  }

  magic = buf.GetDword();
  *localBytesRead += 4;
  if (magic != 'XTVP') {
    status->Add(STATUS_ERROR, "Invalid %s section.\n", "primitives vertices");
    return 0;
  }
  count = buf.GetUint();
  *localBytesRead += 4;
  if (count) {
    pGeoset->primitives.vertices.SetCount(count);
    buf.GetWordArray(pGeoset->primitives.vertices.Ptr(), count);
    *localBytesRead += 2 * count;
  }

  magic = buf.GetDword();
  *localBytesRead += 4;
  if (magic != 'XDNG') {
    status->Add(STATUS_ERROR, "Invalid %s section.\n", "vertex group indices");
    return 0;
  }
  count = buf.GetUint();
  *localBytesRead += 4;
  if (count) {
    pGeoset->vertGroupIndices.SetCount(count);
    buf.GetData(pGeoset->vertGroupIndices.Ptr(), count);
    *localBytesRead += count;
  }

  if (!IReadBinUintSection(buf, 'CGTM', "group matrix counts", &pGeoset->groupMatrixCounts, localBytesRead, status)) {
    return 0;
  }
  if (!IReadBinUintSection(buf, 'STAM', "matrices", &pGeoset->matrices, localBytesRead, status)) {
    return 0;
  }
  if (!IReadBinUintSection(buf, 'XDIB', "bone indices", &pGeoset->boneIndices, localBytesRead, status)) {
    return 0;
  }
  return IReadBinUintSection(buf, 'TGWB', "bone weights", &pGeoset->boneWeights, localBytesRead, status) != 0;
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

BOOL MDL::ReadBinGeosets(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status) {
  VALIDATEBEGIN;
  VALIDATE(status != 0);
  VALIDATEEND;
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
  size += 8 + sizeof(NTempest::C3Vector) * section.vertices.Count();
  size += 8 + sizeof(NTempest::C3Vector) * section.normals.Count();
  size += 8 + section.primitives.types.Count();
  size += 8 + 4 * section.primitives.counts.Count();
  size += 8 + 2 * section.primitives.vertices.Count();
  size += 8 + section.vertGroupIndices.Count();
  size += 8 + 4 * section.groupMatrixCounts.Count();
  size += 8 + 4 * section.matrices.Count();
  size += 8 + 4 * section.boneIndices.Count();
  size += 8 + 4 * section.boneWeights.Count();
  size += 40;
  size += 4 + sizeof(CMdlBounds) * section.seqBounds.Count();
  if (section.texCoords.Count() > 0) {
    size += 8 + 8 * section.texCoords.Count() * section.vertices.Count();
  }
  return size;
}

static void IWriteBinAnimBounds(const MDLGEOSETSECTION &section, CMsgBuffer &buffer) {
  buffer.AddUint(section.seqBounds.Count());
  const CMdlBounds *bounds = section.seqBounds.Ptr();
  for (UINT i = section.seqBounds.Count(); i; --i, ++bounds) {
    buffer.AddFloat(bounds->radius);
    buffer.AddFloatArray(&bounds->extent.b.x, 3);
    buffer.AddFloatArray(&bounds->extent.t.x, 3);
  }
}

static void WriteBinGeoset(CMsgBuffer &buf, const MDLGEOSETSECTION &section) {
  buf.AddUint(GetBinGeosetSize(section));
  WriteBinC3VectorSection(buf, 'XTRV', section.vertices);
  WriteBinC3VectorSection(buf, 'SMRN', section.normals);
  UINT count = section.texCoords.Count();
  if (count > 0) {
    buf.AddDword('SAVU');
    buf.AddUint(count);
    UINT channelFloats = 2 * section.vertices.Count();
    for (UINT i = 0; i < count; ++i) {
      buf.AddFloatArray(&section.texCoords[i].Ptr()->x, channelFloats);
    }
  }
  buf.AddDword('PYTP');
  count = section.primitives.types.Count();
  buf.AddUint(count);
  buf.AddData(section.primitives.types.Ptr(), count);
  count = section.primitives.counts.Count();
  buf.AddDword('TNCP');
  buf.AddUint(count);
  buf.AddUintArray(section.primitives.counts.Ptr(), count);
  count = section.primitives.vertices.Count();
  buf.AddDword('XTVP');
  buf.AddUint(count);
  buf.AddWordArray(section.primitives.vertices.Ptr(), count);
  count = section.vertGroupIndices.Count();
  buf.AddDword('XDNG');
  buf.AddUint(count);
  buf.AddData(section.vertGroupIndices.Ptr(), count);
  count = section.groupMatrixCounts.Count();
  buf.AddDword('CGTM');
  buf.AddUint(count);
  buf.AddUintArray(section.groupMatrixCounts.Ptr(), count);
  count = section.matrices.Count();
  buf.AddDword('STAM');
  buf.AddUint(count);
  buf.AddUintArray(section.matrices.Ptr(), count);
  count = section.boneIndices.Count();
  buf.AddDword('XDIB');
  buf.AddUint(count);
  buf.AddUintArray(section.boneIndices.Ptr(), count);
  count = section.boneWeights.Count();
  buf.AddDword('TGWB');
  buf.AddUint(count);
  buf.AddUintArray(section.boneWeights.Ptr(), count);
  buf.AddUint(section.materialId);
  buf.AddUint(section.selectionGroup);
  buf.AddUint(section.flags);
  buf.AddFloat(section.bounds.radius);
  buf.AddFloatArray(&section.bounds.extent.b.x, 3);
  buf.AddFloatArray(&section.bounds.extent.t.x, 3);
  IWriteBinAnimBounds(section, buf);
}

BOOL MDL::WriteBinGeosets(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *) {
  UINT numGeosets = data.geosets.Count();
  if (!numGeosets) {
    return 1;
  }
  buf.AddDword('SOEG');
  UINT totalSize = 4;
  UINT i;
  for (i = 0; i < numGeosets; ++i) {
    totalSize += GetBinGeosetSize(data.geosets[i]);
  }
  buf.AddUint(totalSize);
  buf.AddUint(numGeosets);
  for (i = 0; i < numGeosets; ++i) {
    WriteBinGeoset(buf, data.geosets[i]);
  }
  return 1;
}
