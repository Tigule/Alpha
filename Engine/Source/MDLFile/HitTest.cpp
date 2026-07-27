#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

#include <string.h>

void ReadVertices(
    Parser &parse,
    const char *item,
    TSGrowableArray<NTempest::C3Vector> *vertices
);

namespace MDL {
const char *TokenText(unsigned int token);
void __cdecl WriteLine(TSGrowableArray<char> &buffer, const char *format, ...);

int ReadHitTest(
    Parser &parse,
    MDLDATA &data,
    CMDLStatus *status
) {
  TSet errors;
  MDLHITTESTSHAPE *section = data.hitTestShapes.New();
  AddObjectErrors(errors);
  ReadObjectName(parse, section->name);
  parse.Expect('{');

  TSGrowableArray<NTempest::C3Vector> vertices;
  float radius = 0.0f;
  const char *tokenText;
  unsigned int token = parse.Token(&tokenText, 0);
  while (token && token != '}') {
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokenText);
    }
    if (!ReadObjectBody(parse, token, 0, section, status)) {
      switch (token) {
        case 0x134:
          radius = parse.ExpectFloat();
          break;
        case 0x135:
          section->type = SHAPE_BOX;
          break;
        case 0x13C:
          section->type = SHAPE_CYLINDER;
          break;
        case 0x164:
          section->shape.plane.length = parse.ExpectFloat();
          break;
        case 0x1A2:
          section->type = SHAPE_PLANE;
          break;
        case 0x1B4:
          section->type = SHAPE_SPHERE;
          break;
        case 0x1D8:
          ReadVertices(parse, "vertices", &vertices);
          token = parse.Token(&tokenText, 0);
          continue;
        case 0x1DA:
          section->shape.plane.width = parse.ExpectFloat();
          break;
        default:
          parse.FatalUnexpected(tokenText);
          break;
      }
      parse.Expect(',');
    }
    token = parse.Token(&tokenText, 0);
  }

  switch (section->type) {
    case SHAPE_BOX:
      section->shape.box.minimum.x = vertices.Ptr()[0].x;
      section->shape.box.minimum.y = vertices.Ptr()[0].y;
      section->shape.box.minimum.z = vertices.Ptr()[0].z;
      section->shape.box.maximum.x = vertices.Ptr()[1].x;
      section->shape.box.maximum.y = vertices.Ptr()[1].y;
      section->shape.box.maximum.z = vertices.Ptr()[1].z;
      break;
    case SHAPE_CYLINDER:
      section->shape.cylinder.base.x = vertices.Ptr()[0].x;
      section->shape.cylinder.base.y = vertices.Ptr()[0].y;
      section->shape.cylinder.base.z = vertices.Ptr()[0].z;
      section->shape.cylinder.height =
          vertices.Ptr()[1].z - vertices.Ptr()[0].z;
      section->shape.cylinder.radius = radius;
      break;
    case SHAPE_SPHERE:
      section->shape.sphere.center.x = vertices.Ptr()[0].x;
      section->shape.sphere.center.y = vertices.Ptr()[0].y;
      section->shape.sphere.center.z = vertices.Ptr()[0].z;
      section->shape.sphere.radius = radius;
      break;
    case SHAPE_PLANE:
      break;
  }

  parse.Expect('}', token, tokenText);
  ReadObjectEnd(
      errors,
      data,
      section,
      data.hitTestShapes.Count() - 1,
      0x80000000
  );
  errors.Complete(status);
  return !parse.FoundError();
}

static void IWriteHitTestSection(
    const MDLDATA &data,
    const MDLHITTESTSHAPE &section,
    int needObjectIds,
    TSGrowableArray<char> &buffer
) {
  WriteObjectHeader(data, section, 0x116, needObjectIds, buffer);
  switch (section.type) {
    case SHAPE_BOX:
      WriteLine(buffer, "\t%s,\n", TokenText(0x135));
      WriteLine(buffer, "\t%s 2 {\n", TokenText(0x1D8));
      WriteLine(
          buffer,
          "\t\t{ %g, %g, %g },\n",
          section.shape.box.minimum.x,
          section.shape.box.minimum.y,
          section.shape.box.minimum.z
      );
      WriteLine(
          buffer,
          "\t\t{ %g, %g, %g },\n",
          section.shape.box.maximum.x,
          section.shape.box.maximum.y,
          section.shape.box.maximum.z
      );
      WriteLine(buffer, "\t}\n");
      break;
    case SHAPE_CYLINDER:
      WriteLine(buffer, "\t%s,\n", TokenText(0x13C));
      WriteLine(buffer, "\t%s 2 {\n", TokenText(0x1D8));
      WriteLine(
          buffer,
          "\t\t{ %g, %g, %g },\n",
          section.shape.cylinder.base.x,
          section.shape.cylinder.base.y,
          section.shape.cylinder.base.z
      );
      WriteLine(
          buffer,
          "\t\t{ %g, %g, %g },\n",
          section.shape.cylinder.base.x,
          section.shape.cylinder.base.y,
          section.shape.cylinder.base.z + section.shape.cylinder.height
      );
      WriteLine(buffer, "\t}\n");
      WriteLine(
          buffer,
          "\t%s %g,\n",
          TokenText(0x134),
          section.shape.cylinder.radius
      );
      break;
    case SHAPE_SPHERE:
      WriteLine(buffer, "\t%s,\n", TokenText(0x1B4));
      WriteLine(buffer, "\t%s 1 {\n", TokenText(0x1D8));
      WriteLine(
          buffer,
          "\t\t{ %g, %g, %g },\n",
          section.shape.sphere.center.x,
          section.shape.sphere.center.y,
          section.shape.sphere.center.z
      );
      WriteLine(buffer, "\t}\n");
      WriteLine(
          buffer,
          "\t%s %g,\n",
          TokenText(0x134),
          section.shape.sphere.radius
      );
      break;
    case SHAPE_PLANE:
      WriteLine(buffer, "\t%s,\n", TokenText(0x1A2));
      WriteLine(
          buffer,
          "\t%s %g,\n",
          TokenText(0x164),
          section.shape.plane.length
      );
      WriteLine(
          buffer,
          "\t%s %g,\n",
          TokenText(0x1DA),
          section.shape.plane.width
      );
      break;
  }
  WriteObjectTrailer(section, buffer);
}

int WriteHitTests(
    const MDLDATA &data,
    TSGrowableArray<char> &buffer,
    CMDLStatus *
) {
  int needObjectIds = data.hitTestShapes.Count() != data.objects.Count();
  for (unsigned int i = 0; i < data.hitTestShapes.Count(); ++i) {
    IWriteHitTestSection(
        data,
        data.hitTestShapes.Ptr()[i],
        needObjectIds,
        buffer
    );
  }
  return 1;
}

int WriteBinHitTests(
    const MDLDATA &data,
    CMsgBuffer &buffer,
    CMDLStatus *status
) {
  if (data.hitTestShapes.Count()) {
    buffer.AddDword('TSTH');
    unsigned int totalSize = 4;
    unsigned int i;
    for (i = 0; i < data.hitTestShapes.Count(); ++i) {
      const MDLHITTESTSHAPE &section = data.hitTestShapes.Ptr()[i];
      unsigned int sectionSize = 5;
      switch (section.type) {
        case SHAPE_BOX: sectionSize = 29; break;
        case SHAPE_CYLINDER: sectionSize = 25; break;
        case SHAPE_SPHERE: sectionSize = 21; break;
        case SHAPE_PLANE: sectionSize = 13; break;
      }
      totalSize += GetBinGenObjectSize(section) + sectionSize;
    }
    buffer.AddUint(totalSize);
    buffer.AddUint(data.hitTestShapes.Count());
    for (i = 0; i < data.hitTestShapes.Count(); ++i) {
      const MDLHITTESTSHAPE &section = data.hitTestShapes.Ptr()[i];
      unsigned int sectionSize = 5;
      switch (section.type) {
        case SHAPE_BOX: sectionSize = 29; break;
        case SHAPE_CYLINDER: sectionSize = 25; break;
        case SHAPE_SPHERE: sectionSize = 21; break;
        case SHAPE_PLANE: sectionSize = 13; break;
      }
      buffer.AddUint(GetBinGenObjectSize(section) + sectionSize);
      WriteBinGenObject(section, buffer, status);
      buffer.AddByte(static_cast<unsigned char>(section.type));
      switch (section.type) {
        case SHAPE_BOX:
          buffer.AddFloatArray(&section.shape.box.minimum.x, 3);
          buffer.AddFloatArray(&section.shape.box.maximum.x, 3);
          break;
        case SHAPE_CYLINDER:
          buffer.AddFloatArray(&section.shape.cylinder.base.x, 3);
          buffer.AddFloat(section.shape.cylinder.height);
          buffer.AddFloat(section.shape.cylinder.radius);
          break;
        case SHAPE_SPHERE:
          buffer.AddFloatArray(&section.shape.sphere.center.x, 3);
          buffer.AddFloat(section.shape.sphere.radius);
          break;
        case SHAPE_PLANE:
          buffer.AddFloat(section.shape.plane.length);
          buffer.AddFloat(section.shape.plane.width);
          break;
      }
    }
  }
  return 1;
}

int ReadBinHitTests(
    CMsgBuffer &buffer,
    unsigned int length,
    MDLDATA &data,
    CMDLStatus *status
) {
  unsigned int count = buffer.GetUint();
  unsigned int totalRead = 4;
  data.hitTestShapes.SetCount(0);
  data.hitTestShapes.Reserve(count);
  while (totalRead < length) {
    MDLHITTESTSHAPE *section = data.hitTestShapes.New();
    if (!section) {
      status->FatalFlunked("Hit test", -1);
      return 0;
    }
    buffer.GetUint();
    totalRead += 4;
    if (!ReadBinGenObject(*section, buffer, status, totalRead)) {
      status->Add(
          STATUS_ERROR,
          "Error reading gen object portion of hit test shape.\n"
      );
      return 0;
    }
    section->type = static_cast<GEOM_SHAPE>(buffer.GetByte());
    ++totalRead;
    switch (section->type) {
      case SHAPE_BOX:
        buffer.GetFloatArray(&section->shape.box.minimum.x, 3);
        buffer.GetFloatArray(&section->shape.box.maximum.x, 3);
        totalRead += 24;
        break;
      case SHAPE_CYLINDER:
        buffer.GetFloatArray(&section->shape.cylinder.base.x, 3);
        section->shape.cylinder.height = buffer.GetFloat();
        section->shape.cylinder.radius = buffer.GetFloat();
        totalRead += 20;
        break;
      case SHAPE_SPHERE:
        buffer.GetFloatArray(&section->shape.sphere.center.x, 3);
        section->shape.sphere.radius = buffer.GetFloat();
        totalRead += 16;
        break;
      case SHAPE_PLANE:
        section->shape.plane.length = buffer.GetFloat();
        section->shape.plane.width = buffer.GetFloat();
        totalRead += 8;
        break;
      default:
        break;
    }
    if (totalRead > length) {
      status->FatalOverran(
          "Hit test section overran read buffer.\n",
          -1
      );
      return 0;
    }
    ReadBinObjectEnd(
        data,
        section,
        data.hitTestShapes.Count() - 1,
        0x80000000
    );
  }
  return 1;
}

}
