#include "MDLTypes.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "GenObject.h"
#include "Base/MsgBuffer.h"

#include <storm.h>
#include <stpl.h>

void WriteBounds(const CMdlBounds &, const char *, TSGrowableArray<char> &);

namespace MDL {
const char *TokenText(unsigned int token);
void __cdecl WriteLine(TSGrowableArray<char> &buffer, const char *format, ...);

static void IModelAddErrors(TSet &errors) {
  errors.Add(0x181, 0, 0);
  errors.Add(0x182, 0, 0);
  errors.Add(0x17D, 0, 0);
  errors.Add(0x170, 0, 0);
  errors.Add(0x16F, 0, 0);
  errors.Add(0x134, 0, 0);
  errors.Add(0x17C, 0, 0);
  errors.Add(0x17E, 0, 0);
  errors.Add(0x184, 0, 0);
  errors.Add(0x185, 0, 0);
  errors.Add(0x186, 0, 0);
  errors.Add(0x154, 0, 0);
}

static void IReadVertex(
    Parser &parse,
    NTempest::C3Vector *vertex
) {
  parse.Expect('{');
  vertex->x = parse.ExpectFloat();
  parse.Expect(',');
  vertex->y = parse.ExpectFloat();
  parse.Expect(',');
  vertex->z = parse.ExpectFloat();
  parse.Expect('}');
}

static void AddGroundTrackErrors(TSet &errors) {
  errors.Add(0x1A1, 0, 0);
  errors.Add(0x1DE, 0, 0);
  errors.Add(0x1AC, 0, 0);
}

static void IReadGroundTrack(
    Parser &parse,
    MDLMODELSECTION *model,
    CMDLStatus *status
) {
  TSet errors;
  AddGroundTrackErrors(errors);
  parse.Expect('{');
  const char *tokenText;
  unsigned int token = parse.Token(&tokenText, 0);
  do {
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokenText);
    }
    if (token != 0x1A1 && token != 0x1AC && token != 0x1DE) {
      parse.FatalUnexpected(tokenText);
    }
  } while (parse.GetOptionalToken(',', &token, &tokenText));
  parse.Expect('}', token, tokenText);

  unsigned int groundTrack;
  if (errors.NotFound(0x1AC) && errors.NotFound(0x1A1)) {
    groundTrack = 0;
  } else {
    groundTrack = errors.NotFound(0x1AC) ? 1 : 2;
  }
  model->flags = (model->flags & ~3U) | groundTrack;
  errors.Complete(status);
}

static void IReadModelGlobals(
    Parser &parse,
    TSet *errors,
    MDLMODELSECTION *model,
    CMDLStatus *status
) {
  parse.Expect('{');
  const char *tokenText;
  unsigned int token = parse.Token(&tokenText, 0);
  while (token && token != '}') {
    if (!errors->Check(token)) {
      parse.FatalDuplicate(tokenText);
    }
    switch (token) {
      case 0x11E:
        model->flags |= 4;
        break;
      case 0x123: {
        const char *animationFile = parse.ExpectString();
        if (animationFile) {
          SStrCopy(model->animationFile, animationFile, 260);
        }
        break;
      }
      case 0x130:
        model->blendTime = parse.ExpectInt();
        break;
      case 0x134:
        model->bounds.radius = parse.ExpectFloat();
        break;
      case 0x154:
        IReadGroundTrack(parse, model, status);
        break;
      case 0x16F:
        IReadVertex(parse, &model->bounds.extent.t);
        break;
      case 0x170:
        IReadVertex(parse, &model->bounds.extent.b);
        break;
      case 0x17C:
        model->attachmentCount = parse.ExpectInt();
        break;
      case 0x17D:
      case 0x183:
        model->boneCount = parse.ExpectInt();
        break;
      case 0x17E:
        model->eventCount = parse.ExpectInt();
        break;
      case 0x17F:
        model->geosetCount = parse.ExpectInt();
        break;
      case 0x180:
        model->geosetAnimCount = parse.ExpectInt();
        break;
      case 0x181:
        model->helperCount = parse.ExpectInt();
        break;
      case 0x182:
        model->lightCount = parse.ExpectInt();
        break;
      case 0x184:
        model->particleCount = parse.ExpectInt();
        break;
      case 0x185:
        model->particle2Count = parse.ExpectInt();
        break;
      case 0x186:
        model->ribbonCount = parse.ExpectInt();
        break;
      default:
        parse.FatalUnexpected(tokenText);
        break;
    }
    parse.Expect(',');
    token = parse.Token(&tokenText, 0);
  }
  parse.Expect('}', token, tokenText);
}

int ReadModelGlobals(
    Parser &parse,
    MDLDATA &data,
    CMDLStatus *status
) {
  TSet errors;
  IModelAddErrors(errors);
  const char *name = parse.ExpectString();
  if (name) {
    SStrCopy(data.model.name, name, 80);
  }
  IReadModelGlobals(parse, &errors, &data.model, status);

  unsigned int objectCount =
      data.model.boneCount
      + data.model.lightCount
      + data.model.helperCount
      + data.model.attachmentCount
      + data.model.particleCount
      + data.model.particle2Count
      + data.model.ribbonCount
      + data.model.eventCount;
  data.objects.ReserveSpace(objectCount);
  if (data.pivotPoints.Count() < 500) {
    data.pivotPoints.ReserveSpace(objectCount);
  }
  data.geosets.ReserveSpace(data.model.geosetCount);
  data.geosetAnims.ReserveSpace(data.model.geosetAnimCount);
  data.bones.ReserveSpace(data.model.boneCount);
  data.lights.ReserveSpace(data.model.lightCount);
  data.helpers.ReserveSpace(data.model.helperCount);
  data.attachments.ReserveSpace(data.model.attachmentCount);
  data.particleEmitters.ReserveSpace(data.model.particleCount);
  data.particleEmitters2.ReserveSpace(data.model.particle2Count);
  data.ribbonEmitters.ReserveSpace(data.model.ribbonCount);
  data.events.ReserveSpace(data.model.eventCount);

  errors.Complete(status);
  return !parse.FoundError();
}

static void IWriteModelObjectCounts(const MDLDATA &data, TSGrowableArray<char> &buffer) {
  if (data.geosets.Count()) WriteLine(buffer, "\t%s %d,\n", TokenText(0x17F), data.geosets.Count());
  if (data.geosetAnims.Count()) WriteLine(buffer, "\t%s %d,\n", TokenText(0x180), data.geosetAnims.Count());
  if (data.helpers.Count()) WriteLine(buffer, "\t%s %d,\n", TokenText(0x181), data.helpers.Count());
  if (data.lights.Count()) WriteLine(buffer, "\t%s %d,\n", TokenText(0x182), data.lights.Count());
  if (data.bones.Count()) WriteLine(buffer, "\t%s %d,\n", TokenText(0x17D), data.bones.Count());
  if (data.attachments.Count()) WriteLine(buffer, "\t%s %d,\n", TokenText(0x17C), data.attachments.Count());
  if (data.particleEmitters.Count()) WriteLine(buffer, "\t%s %d,\n", TokenText(0x184), data.particleEmitters.Count());
  if (data.particleEmitters2.Count()) WriteLine(buffer, "\t%s %d,\n", TokenText(0x185), data.particleEmitters2.Count());
  if (data.ribbonEmitters.Count()) WriteLine(buffer, "\t%s %d,\n", TokenText(0x186), data.ribbonEmitters.Count());
  if (data.events.Count()) WriteLine(buffer, "\t%s %d,\n", TokenText(0x17E), data.events.Count());
}

static void IWriteGroundTrack(TSGrowableArray<char> &buffer, unsigned int groundTrack) {
  if (groundTrack == 0) {
    WriteLine(buffer, "\t%s { %s },\n", TokenText(0x154), TokenText(0x1DE));
  } else if (groundTrack == 1) {
    WriteLine(buffer, "\t%s { %s, %s },\n", TokenText(0x154), TokenText(0x1A1), TokenText(0x1DE));
  } else if (groundTrack == 2) {
    WriteLine(
        buffer, "\t%s { %s, %s, %s },\n",
        TokenText(0x154), TokenText(0x1A1), TokenText(0x1DE), TokenText(0x1AC)
    );
  }
}

int WriteModelGlobals(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
  const MDLMODELSECTION &model = data.model;
  if (SStrLen(model.name)
      || data.helpers.Count() || data.lights.Count() || data.bones.Count()
      || data.geosets.Count() || data.attachments.Count() || data.particleEmitters.Count()
      || data.events.Count() || data.ribbonEmitters.Count() || data.particleEmitters2.Count()
      || data.sequences.Count()
      || model.bounds.radius != 0.0f
      || model.bounds.extent.b.x != 0.0f || model.bounds.extent.b.y != 0.0f
      || model.bounds.extent.b.z != 0.0f || model.bounds.extent.t.x != 0.0f
      || model.bounds.extent.t.y != 0.0f || model.bounds.extent.t.z != 0.0f
      || model.flags) {
    WriteLine(buffer, "%s \"%s\" {\n", TokenText(0x104), static_cast<const char *>(model.name));
    IWriteModelObjectCounts(data, buffer);
    WriteBounds(model.bounds, "\t", buffer);
    if (SStrLen(model.animationFile)) {
      WriteLine(buffer, "\t%s \"%s\",\n", TokenText(0x123), static_cast<const char *>(model.animationFile));
    }
    if (data.sequences.Count()) {
      IWriteGroundTrack(buffer, model.flags & 3);
      if (model.flags & 4) {
        WriteLine(buffer, "\t%s,\n", TokenText(0x11E));
      }
    }
    WriteLine(buffer, "}\n");
  }
  return 1;
}

int ReadBinModelGlobals(
    CMsgBuffer &buffer,
    unsigned int length,
    MDLDATA &data,
    CMDLStatus *status
) {
  FATALASSERT(status);
  if (length != 373) {
    status->Add(STATUS_ERROR, "Invalid MODL section detected in model.\n");
    return 0;
  }
  buffer.GetTcharArray(data.model.name, 80);
  buffer.GetTcharArray(data.model.animationFile, 260);
  data.model.bounds.radius = buffer.GetFloat();
  buffer.GetFloatArray(&data.model.bounds.extent.b.x, 3);
  buffer.GetFloatArray(&data.model.bounds.extent.t.x, 3);
  data.model.flags = buffer.GetByte();
  data.objects.ReserveSpace(buffer.GetUint());
  return 1;
}

int WriteBinModelGlobals(const MDLDATA &data, CMsgBuffer &buffer, CMDLStatus *) {
  buffer.AddDword('LDOM');
  buffer.AddUint(373);
  buffer.AddTcharArray(data.model.name, 80, 1);
  buffer.AddTcharArray(data.model.animationFile, 260, 1);
  buffer.AddFloat(data.model.bounds.radius);
  buffer.AddFloatArray(&data.model.bounds.extent.b.x, 3);
  buffer.AddFloatArray(&data.model.bounds.extent.t.x, 3);
  buffer.AddByte(static_cast<unsigned char>(data.model.flags));
  buffer.AddUint(data.objects.Count());
  return 1;
}

}
