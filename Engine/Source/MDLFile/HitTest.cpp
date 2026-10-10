#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

#include <string.h>

namespace MDL {
  LPCSTR       TokenText(UINT token);
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);
  BOOL         ReadHitTest(Parser &, MDLDATA &, CMDLStatus *);
  BOOL         WriteHitTests(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL         WriteBinHitTests(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  BOOL         ReadBinHitTests(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
}  // namespace MDL

void ReadVertices(Parser &parse, LPCSTR item, TSGrowableArray<NTempest::C3Vector> *vertices);


BOOL MDL::ReadHitTest(Parser &parse, MDLDATA &data, CMDLStatus *status) {
  TSet             errors;
  MDLHITTESTSHAPE *section = data.hitTestShapes.New();
  AddObjectErrors(errors);
  ReadObjectName(parse, section->name);
  parse.Expect('{');

  TSGrowableArray<NTempest::C3Vector> vertices;
  float                               radius = 0.0f;
  LPCSTR                              tokentext;
  UINT                                token;
  for (token = parse.Token(&tokentext, 0); token != '}'; token = parse.Token(&tokentext, 0)) {
    if (!token) {
      break;
    }
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokentext);
    }
    if (!ReadObjectBody(parse, token, 0, section, status)) {
      switch (token) {
        case MDLTOK_BOX:
          section->type = SHAPE_BOX;
          break;
        case MDLTOK_CYLINDER:
          section->type = SHAPE_CYLINDER;
          break;
        case MDLTOK_SPHERE:
          section->type = SHAPE_SPHERE;
          break;
        case MDLTOK_PLANE:
          section->type = SHAPE_PLANE;
          break;
        case MDLTOK_VERTICES:
          ReadVertices(parse, "vertices", &vertices);
          continue;
        case MDLTOK_BOUNDS_RADIUS:
          radius = parse.ExpectFloat();
          break;
        case MDLTOK_LENGTH:
          section->shape.plane.length = parse.ExpectFloat();
          break;
        case MDLTOK_WIDTH:
          section->shape.plane.width = parse.ExpectFloat();
          break;
        default:
          parse.FatalUnexpected(tokentext);
          break;
      }
      parse.Expect(',');
    }
  }

  switch (section->type) {
    case SHAPE_BOX:
      vertices[0].Get(section->shape.box.minimum.x, section->shape.box.minimum.y, section->shape.box.minimum.z);
      vertices[1].Get(section->shape.box.maximum.x, section->shape.box.maximum.y, section->shape.box.maximum.z);
      break;
    case SHAPE_CYLINDER:
      vertices[0].Get(section->shape.cylinder.base.x, section->shape.cylinder.base.y, section->shape.cylinder.base.z);
      section->shape.cylinder.height = vertices[1].z - vertices[0].z;
      section->shape.cylinder.radius = radius;
      break;
    case SHAPE_SPHERE:
      vertices[0].Get(section->shape.sphere.center.x, section->shape.sphere.center.y, section->shape.sphere.center.z);
      section->shape.sphere.radius = radius;
      break;
  }

  parse.Expect('}', token, tokentext);
  ReadObjectEnd(errors, data, section, data.hitTestShapes.Count() - 1, 0x80000000);
  errors.Complete(status);
  return !parse.FoundError();
}

static void IWriteHitTestSection(const MDLDATA &data, const MDLHITTESTSHAPE &section, int needObjIds, TSGrowableArray<char> &buffer) {
  Vector3 top;
  WriteObjectHeader(data, section, MDLTOK_HITTESTSHAPE, needObjIds, buffer);
  switch (section.type) {
    case SHAPE_BOX:
      MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(MDLTOK_BOX));
      MDL::WriteLine(buffer, "\t%s 2 {\n", MDL::TokenText(MDLTOK_VERTICES));
      MDL::WriteLine(buffer, "\t\t{ %g, %g, %g },\n", section.shape.box.minimum.x, section.shape.box.minimum.y, section.shape.box.minimum.z);
      MDL::WriteLine(buffer, "\t\t{ %g, %g, %g },\n", section.shape.box.maximum.x, section.shape.box.maximum.y, section.shape.box.maximum.z);
      MDL::WriteLine(buffer, "\t}\n");
      break;
    case SHAPE_CYLINDER:
      MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(MDLTOK_CYLINDER));
      MDL::WriteLine(buffer, "\t%s 2 {\n", MDL::TokenText(MDLTOK_VERTICES));
      MDL::WriteLine(buffer, "\t\t{ %g, %g, %g },\n", section.shape.cylinder.base.x, section.shape.cylinder.base.y, section.shape.cylinder.base.z);
      top = section.shape.cylinder.base;
      top.z += section.shape.cylinder.height;
      MDL::WriteLine(buffer, "\t\t{ %g, %g, %g },\n", top.x, top.y, top.z);
      MDL::WriteLine(buffer, "\t}\n");
      MDL::WriteLine(buffer, "\t%s %g,\n", MDL::TokenText(MDLTOK_BOUNDS_RADIUS), section.shape.cylinder.radius);
      break;
    case SHAPE_SPHERE:
      MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(MDLTOK_SPHERE));
      MDL::WriteLine(buffer, "\t%s 1 {\n", MDL::TokenText(MDLTOK_VERTICES));
      MDL::WriteLine(buffer, "\t\t{ %g, %g, %g },\n", section.shape.sphere.center.x, section.shape.sphere.center.y, section.shape.sphere.center.z);
      MDL::WriteLine(buffer, "\t}\n");
      MDL::WriteLine(buffer, "\t%s %g,\n", MDL::TokenText(MDLTOK_BOUNDS_RADIUS), section.shape.sphere.radius);
      break;
    case SHAPE_PLANE:
      MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(MDLTOK_PLANE));
      MDL::WriteLine(buffer, "\t%s %g,\n", MDL::TokenText(MDLTOK_LENGTH), section.shape.plane.length);
      MDL::WriteLine(buffer, "\t%s %g,\n", MDL::TokenText(MDLTOK_WIDTH), section.shape.plane.width);
      break;
  }
  WriteObjectTrailer(section, buffer);
}

BOOL MDL::WriteHitTests(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
  UINT                   numHitTestShapes = data.hitTestShapes.Count();
  int                    needObjIds = numHitTestShapes != data.objects.Count();
  const MDLHITTESTSHAPE *section = data.hitTestShapes.Ptr();
  for (UINT i = numHitTestShapes; i; --i, ++section) {
    IWriteHitTestSection(data, *section, needObjIds, buffer);
  }
  return 1;
}

BOOL MDL::WriteBinHitTests(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *status) {
  UINT numHitTestShapes = data.hitTestShapes.Count();
  if (!numHitTestShapes) {
    return 1;
  }
  buf.AddDword('TSTH');
  UINT totalSize = 4;
  UINT n;
  for (n = 0; n < numHitTestShapes; ++n) {
    const MDLHITTESTSHAPE &section = data.hitTestShapes[n];
    UINT                   sectionSize = 5;
    switch (section.type) {
      case SHAPE_BOX:
        sectionSize = 29;
        break;
      case SHAPE_CYLINDER:
        sectionSize = 25;
        break;
      case SHAPE_SPHERE:
        sectionSize = 21;
        break;
      case SHAPE_PLANE:
        sectionSize = 13;
        break;
    }
    totalSize += GetBinGenObjectSize(section) + sectionSize;
  }
  buf.AddUint(totalSize);
  buf.AddUint(numHitTestShapes);
  for (n = 0; n < numHitTestShapes; ++n) {
    const MDLHITTESTSHAPE &section = data.hitTestShapes[n];
    UINT                   sectionSize = 5;
    switch (section.type) {
      case SHAPE_BOX:
        sectionSize = 29;
        break;
      case SHAPE_CYLINDER:
        sectionSize = 25;
        break;
      case SHAPE_SPHERE:
        sectionSize = 21;
        break;
      case SHAPE_PLANE:
        sectionSize = 13;
        break;
    }
    sectionSize += GetBinGenObjectSize(section);
    buf.AddUint(sectionSize);
    WriteBinGenObject(section, buf, status);
    buf.AddByte((BYTE)section.type);
    switch (section.type) {
      case SHAPE_BOX:
        buf.AddFloatArray(&section.shape.box.minimum.x, 3);
        buf.AddFloatArray(&section.shape.box.maximum.x, 3);
        break;
      case SHAPE_CYLINDER:
        buf.AddFloatArray(&section.shape.cylinder.base.x, 3);
        buf.AddFloat(section.shape.cylinder.height);
        buf.AddFloat(section.shape.cylinder.radius);
        break;
      case SHAPE_SPHERE:
        buf.AddFloatArray(&section.shape.sphere.center.x, 3);
        buf.AddFloat(section.shape.sphere.radius);
        break;
      case SHAPE_PLANE:
        buf.AddFloat(section.shape.plane.length);
        buf.AddFloat(section.shape.plane.width);
        break;
    }
  }
  return 1;
}

BOOL MDL::ReadBinHitTests(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status) {
  UINT numHitTestShapes = buf.GetUint();
  UINT totalRead = 4;
  data.hitTestShapes.SetCount(0);
  data.hitTestShapes.ReserveSpace(numHitTestShapes);
  while (totalRead < length) {
    MDLHITTESTSHAPE *section = data.hitTestShapes.New();
    if (!section) {
      status->FatalFlunked("Hit test", -1);
      return 0;
    }
    buf.GetUint();
    totalRead += 4;
    if (!ReadBinGenObject(*section, buf, status, totalRead)) {
      status->Add(STATUS_ERROR, "Error reading gen object portion of hit test shape.\n");
      return 0;
    }
    section->type = (GEOM_SHAPE)buf.GetByte();
    ++totalRead;
    switch (section->type) {
      case SHAPE_BOX:
        buf.GetFloatArray(&section->shape.box.minimum.x, 3);
        buf.GetFloatArray(&section->shape.box.maximum.x, 3);
        totalRead += 24;
        break;
      case SHAPE_CYLINDER:
        buf.GetFloatArray(&section->shape.cylinder.base.x, 3);
        section->shape.cylinder.height = buf.GetFloat();
        section->shape.cylinder.radius = buf.GetFloat();
        totalRead += 20;
        break;
      case SHAPE_SPHERE:
        buf.GetFloatArray(&section->shape.sphere.center.x, 3);
        section->shape.sphere.radius = buf.GetFloat();
        totalRead += 16;
        break;
      case SHAPE_PLANE:
        section->shape.plane.length = buf.GetFloat();
        section->shape.plane.width = buf.GetFloat();
        totalRead += 8;
        break;
      default:
        break;
    }
    if (totalRead > length) {
      status->FatalOverran("Hit test section overran read buf.\n", -1);
      return 0;
    }
    ReadBinObjectEnd(data, section, data.hitTestShapes.Count() - 1, 0x80000000);
  }
  return 1;
}
