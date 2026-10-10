#include "MDLTypes.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "GenObject.h"
#include "Model/ModelInternal.h"
#include "Base/MsgBuffer.h"

#include <storm.h>
#include <stpl.h>

namespace MDL {
  LPCSTR       TokenText(UINT token);
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);
  BOOL         ReadModelGlobals(Parser &, MDLDATA &, CMDLStatus *);
  BOOL         WriteModelGlobals(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL         ReadBinModelGlobals(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  BOOL         WriteBinModelGlobals(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
}  // namespace MDL

void WriteBounds(const CMdlBounds &, LPCSTR, TSGrowableArray<char> &);


static void IModelAddErrors(TSet &errors) {
  errors.Add(MDLTOK_NUMHELPERS, 0, 0);
  errors.Add(MDLTOK_NUMLIGHTS, 0, 0);
  errors.Add(MDLTOK_NUMBONES, 0, 0);
  errors.Add(MDLTOK_MINIMUMEXTENT, 0, 0);
  errors.Add(MDLTOK_MAXIMUMEXTENT, 0, 0);
  errors.Add(MDLTOK_BOUNDS_RADIUS, 0, 0);
  errors.Add(MDLTOK_NUMATTACHMENTS, 0, 0);
  errors.Add(MDLTOK_NUMEVENTS, 0, 0);
  errors.Add(MDLTOK_NUMPARTICLEEMITTERS, 0, 0);
  errors.Add(MDLTOK_NUMPARTICLEEMITTERS2, 0, 0);
  errors.Add(MDLTOK_NUMRIBBONEMITTERS, 0, 0);
  errors.Add(MDLTOK_GROUNDTRACK, 0, 0);
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

static void AddGroundTrackErrors(TSet &errors) {
  errors.Add(MDLTOK_PITCH, 0, 0);
  errors.Add(MDLTOK_YAW, 0, 0);
  errors.Add(MDLTOK_ROLL, 0, 0);
}

static void IReadGroundTrack(Parser &parse, MDLMODELSECTION *model, CMDLStatus *status) {
  TSet errors;
  AddGroundTrackErrors(errors);
  parse.Expect('{');
  LPCSTR tokentext;
  UINT savedtoken = parse.Token(&tokentext, 0);
  do {
    if (!errors.Check(savedtoken)) {
      parse.FatalDuplicate(tokentext);
    }
    switch (savedtoken) {
      case MDLTOK_PITCH:
      case MDLTOK_ROLL:
      case MDLTOK_YAW:
        break;
      default:
        parse.FatalUnexpected(tokentext);
        break;
    }
  } while (parse.GetOptionalToken(',', &savedtoken, &tokentext));
  parse.Expect('}', savedtoken, tokentext);

  model->flags = (model->flags & ~GROUND_TRACK_MASK) |
                 ((errors.NotFound(MDLTOK_ROLL) && errors.NotFound(MDLTOK_PITCH))
                      ? (BYTE)TRACK_YAW_ONLY
                      : (BYTE)(errors.NotFound(MDLTOK_ROLL) ? TRACK_PITCH_YAW : TRACK_PITCH_YAW_ROLL));
  errors.Complete(status);
}

static void IReadModelGlobals(Parser &parse, TSet *errors, MDLMODELSECTION *model, CMDLStatus *status) {
  parse.Expect('{');
  LPCSTR tokenText;
  UINT   token = parse.Token(&tokenText, 0);
  while (token != '}' && token) {
    if (!errors->Check(token)) {
      parse.FatalDuplicate(tokenText);
    }
    switch (token) {
      case MDLTOK_NUMGEOSETS:
        model->geosetCount = parse.ExpectInt();
        break;
      case MDLTOK_NUMHELPERS:
        model->helperCount = parse.ExpectInt();
        break;
      case MDLTOK_NUMLIGHTS:
        model->lightCount = parse.ExpectInt();
        break;
      case MDLTOK_NUMEVENTS:
        model->eventCount = parse.ExpectInt();
        break;
      case MDLTOK_NUMBONES:
      case MDLTOK_NUMMESHES:
        model->boneCount = parse.ExpectInt();
        break;
      case MDLTOK_BLEND_TIME:
        model->blendTime = parse.ExpectInt();
        break;
      case MDLTOK_NUMPARTICLEEMITTERS:
        model->particleCount = parse.ExpectInt();
        break;
      case MDLTOK_NUMPARTICLEEMITTERS2:
        model->particle2Count = parse.ExpectInt();
        break;
      case MDLTOK_NUMRIBBONEMITTERS:
        model->ribbonCount = parse.ExpectInt();
        break;
      case MDLTOK_NUMATTACHMENTS:
        model->attachmentCount = parse.ExpectInt();
        break;
      case MDLTOK_NUMGEOSETANIMS:
        model->geosetAnimCount = parse.ExpectInt();
        break;
      case MDLTOK_MINIMUMEXTENT:
        IReadVertex(parse, &model->bounds.extent.b);
        break;
      case MDLTOK_MAXIMUMEXTENT:
        IReadVertex(parse, &model->bounds.extent.t);
        break;
      case MDLTOK_BOUNDS_RADIUS:
        model->bounds.radius = parse.ExpectFloat();
        break;
      case MDLTOK_ANIMATIONFILE: {
        LPCSTR animationFile = parse.ExpectString();
        if (animationFile) {
          SStrCopy(model->animationFile, animationFile, 260);
        }
        break;
      }
      case MDLTOK_GROUNDTRACK:
        IReadGroundTrack(parse, model, status);
        break;
      case MDLTOK_ALWAYS_ANIMATE:
        model->flags |= 4;
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

BOOL MDL::ReadModelGlobals(Parser &parse, MDLDATA &data, CMDLStatus *status) {
  TSet errors;
  IModelAddErrors(errors);
  LPCSTR name = parse.ExpectString();
  if (name) {
    SStrCopy(data.model.name, name, 80);
  }
  IReadModelGlobals(parse, &errors, &data.model, status);

  UINT objectCount = data.model.boneCount + data.model.lightCount + data.model.helperCount + data.model.attachmentCount + data.model.particleCount +
                     data.model.particle2Count + data.model.ribbonCount + data.model.eventCount;
  data.objects.ReserveSpace(objectCount);
  if (data.version < 500) {
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
  if (data.geosets.Count() > 0)
    MDL::WriteLine(buffer, "\t%s %d,\n", MDL::TokenText(MDLTOK_NUMGEOSETS), data.geosets.Count());
  if (data.geosetAnims.Count() > 0)
    MDL::WriteLine(buffer, "\t%s %d,\n", MDL::TokenText(MDLTOK_NUMGEOSETANIMS), data.geosetAnims.Count());
  if (data.helpers.Count() > 0)
    MDL::WriteLine(buffer, "\t%s %d,\n", MDL::TokenText(MDLTOK_NUMHELPERS), data.helpers.Count());
  if (data.lights.Count() > 0)
    MDL::WriteLine(buffer, "\t%s %d,\n", MDL::TokenText(MDLTOK_NUMLIGHTS), data.lights.Count());
  if (data.bones.Count() > 0)
    MDL::WriteLine(buffer, "\t%s %d,\n", MDL::TokenText(MDLTOK_NUMBONES), data.bones.Count());
  if (data.attachments.Count() > 0)
    MDL::WriteLine(buffer, "\t%s %d,\n", MDL::TokenText(MDLTOK_NUMATTACHMENTS), data.attachments.Count());
  if (data.particleEmitters.Count() > 0)
    MDL::WriteLine(buffer, "\t%s %d,\n", MDL::TokenText(MDLTOK_NUMPARTICLEEMITTERS), data.particleEmitters.Count());
  if (data.particleEmitters2.Count() > 0)
    MDL::WriteLine(buffer, "\t%s %d,\n", MDL::TokenText(MDLTOK_NUMPARTICLEEMITTERS2), data.particleEmitters2.Count());
  if (data.ribbonEmitters.Count() > 0)
    MDL::WriteLine(buffer, "\t%s %d,\n", MDL::TokenText(MDLTOK_NUMRIBBONEMITTERS), data.ribbonEmitters.Count());
  if (data.events.Count() > 0)
    MDL::WriteLine(buffer, "\t%s %d,\n", MDL::TokenText(MDLTOK_NUMEVENTS), data.events.Count());
}

static void IWriteGroundTrack(TSGrowableArray<char> &buffer, GROUND_TRACK groundTrack) {
  switch (groundTrack) {
    case TRACK_YAW_ONLY:
      MDL::WriteLine(buffer, "\t%s { %s },\n", MDL::TokenText(MDLTOK_GROUNDTRACK), MDL::TokenText(MDLTOK_YAW));
      break;
    case TRACK_PITCH_YAW:
      MDL::WriteLine(buffer, "\t%s { %s, %s },\n", MDL::TokenText(MDLTOK_GROUNDTRACK), MDL::TokenText(MDLTOK_PITCH), MDL::TokenText(MDLTOK_YAW));
      break;
    case TRACK_PITCH_YAW_ROLL:
      MDL::WriteLine(buffer, "\t%s { %s, %s, %s },\n", MDL::TokenText(MDLTOK_GROUNDTRACK), MDL::TokenText(MDLTOK_PITCH), MDL::TokenText(MDLTOK_YAW), MDL::TokenText(MDLTOK_ROLL));
      break;
  }
}

BOOL MDL::WriteModelGlobals(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
  const MDLMODELSECTION &model = data.model;
  if (SStrLen(model.name) || data.helpers.Count() || data.lights.Count() || data.bones.Count() || data.geosets.Count() ||
      data.attachments.Count() || data.particleEmitters.Count() || data.events.Count() || data.ribbonEmitters.Count() ||
      data.particleEmitters2.Count() || data.sequences.Count() || model.bounds.radius != 0.0f || model.bounds.extent.b.x != 0.0f ||
      model.bounds.extent.b.y != 0.0f || model.bounds.extent.b.z != 0.0f || model.bounds.extent.t.x != 0.0f || model.bounds.extent.t.y != 0.0f ||
      model.bounds.extent.t.z != 0.0f || model.flags)
  {
    MDL::WriteLine(buffer, "%s \"%s\" {\n", MDL::TokenText(MDLTOK_MODEL), (LPCSTR)model.name);
    IWriteModelObjectCounts(data, buffer);
    WriteBounds(model.bounds, "\t", buffer);
    if (SStrLen(model.animationFile)) {
      MDL::WriteLine(buffer, "\t%s \"%s\",\n", MDL::TokenText(MDLTOK_ANIMATIONFILE), (LPCSTR)model.animationFile);
    }
    if (data.sequences.Count()) {
      IWriteGroundTrack(buffer, (GROUND_TRACK)(model.flags & GROUND_TRACK_MASK));
      if (model.flags & 4) {
        MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(MDLTOK_ALWAYS_ANIMATE));
      }
    }
    MDL::WriteLine(buffer, "}\n");
  }
  return 1;
}

BOOL MDL::ReadBinModelGlobals(CMsgBuffer &buf, UINT len, MDLDATA &data, CMDLStatus *status) {
  VALIDATEBEGIN;
  VALIDATE(status != 0);
  VALIDATEEND;
  if (len != 373) {
    status->Add(STATUS_ERROR, "Invalid MODL section detected in model.\n");
    return 0;
  }
  buf.GetTcharArray(data.model.name, 80);
  buf.GetTcharArray(data.model.animationFile, 260);
  data.model.bounds.radius = buf.GetFloat();
  buf.GetFloatArray(&data.model.bounds.extent.b.x, 3);
  buf.GetFloatArray(&data.model.bounds.extent.t.x, 3);
  data.model.flags = buf.GetByte();
  data.objects.ReserveSpace(buf.GetUint());
  return 1;
}

BOOL MDL::WriteBinModelGlobals(const MDLDATA &data, CMsgBuffer &buffer, CMDLStatus *) {
  buffer.AddDword('LDOM');
  buffer.AddUint(373);
  buffer.AddTcharArray(data.model.name, 80, 1);
  buffer.AddTcharArray(data.model.animationFile, 260, 1);
  buffer.AddFloat(data.model.bounds.radius);
  buffer.AddFloatArray(&data.model.bounds.extent.b.x, 3);
  buffer.AddFloatArray(&data.model.bounds.extent.t.x, 3);
  buffer.AddByte(data.model.flags);
  buffer.AddUint(data.objects.Count());
  return 1;
}
