#include "MDLTypes.h"
#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

#include <storm.h>

namespace MDL {
  LPCSTR       TokenText(UINT token);
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);
}  // namespace MDL

void ReadVertices(Parser &parse, LPCSTR title, TSGrowableArray<NTempest::C3Vector> *vertices);
void WriteVertices(const TSGrowableArray<NTempest::C3Vector> &vertices, UINT title, TSGrowableArray<char> &buffer);

static void IAddParticleEmitter2Errors(TSet &errors);
static void IReadParticleEmitter2(Parser &parse, TSet &errors, MDLPARTICLEEMITTER2 *emitter, CMDLStatus *status);
static void IReadParticleEmitter2KeyFrames(Parser &parse, UINT savedtoken, LPCSTR tokentext, MDLPARTICLEEMITTER2 *emitter);
static void IReadIntOption(Parser &parse, UINT &p1, UINT &b, UINT &c);
static void IReadByteOption(Parser &parse, BYTE &p1, BYTE &b, BYTE &c);
static void IReadFloatOption(Parser &parse, float &p1, float &b, float &c);
static void IReadFloatOption(Parser &parser, float &f1, float &b);
static void IReadFloatOption(Parser &parser, float &f1, float &b, float &c, float &d, float &e, float &f);
static void IReadParticleEmitter2Color(Parser &parse, MDLPARTICLEEMITTER2 *emitter);
static void IReadSpline(Parser &parse, TSGrowableArray<NTempest::C3Vector> &spline);
static void IReadParticleEmitter2StaticData(Parser &parse, UINT savedtoken, LPCSTR tokentext, MDLPARTICLEEMITTER2 *emitter);
static BOOL ReadParticleEmitter2BlendMode(Parser &parse, UINT savedtoken, MDLPARTICLEEMITTER2 *emitter);
static BOOL IReadParticleEmitter2EmitterType(Parser &parse, UINT savedtoken, MDLPARTICLEEMITTER2 *emitter);
static BOOL ReadParticleEmitter2Type(Parser &parse, UINT savedtoken, MDLPARTICLEEMITTER2 *emitter);
static BOOL IReadParticleEmitter2Flags(Parser &parse, UINT savedtoken, MDLPARTICLEEMITTER2 *emitter);
static void IWriteParticleEmitter2(const MDLDATA &data, const MDLPARTICLEEMITTER2 &emitter, int needObjIds, TSGrowableArray<char> &buffer);
static void IWriteParticleEmitter2BlendMode(const MDLPARTICLEEMITTER2 &section, TSGrowableArray<char> &buffer);
static void IWriteParticleEmitter2Type(const MDLPARTICLEEMITTER2 &section, TSGrowableArray<char> &buffer);
static void IWriteParticleEmitter2Colors(const MDLPARTICLEEMITTER2 &emitter, TSGrowableArray<char> &buffer);
static void IWritePE2Flags(const MDLPARTICLEEMITTER2 &emitter, TSGrowableArray<char> &buffer);
static void IWriteSpline(const TSGrowableArray<NTempest::C3Vector> &points, TSGrowableArray<char> &buffer);
static UINT GetBinParticleEmitter2Size(const MDLPARTICLEEMITTER2 &section);
static UINT GetNonAnimEmitterDataSize(const MDLPARTICLEEMITTER2 &section);
static void IWriteBinParticleEmitter2(const MDLPARTICLEEMITTER2 &section, CMsgBuffer &buf, CMDLStatus *status);
static BOOL ReadBinParticleEmitter2(CMsgBuffer &buf, MDLPARTICLEEMITTER2 *pEmit, CMDLStatus *status, UINT &totalRead);

namespace MDL {

  BOOL ReadParticleEmitter2(Parser &parse, MDLDATA &data, CMDLStatus *status) {
    TSet                 errors;
    MDLPARTICLEEMITTER2 *emitter = data.particleEmitters2.New();
    IAddParticleEmitter2Errors(errors);
    ReadObjectName(parse, emitter->name);
    IReadParticleEmitter2(parse, errors, emitter, status);
    ReadObjectEnd(errors, data, emitter, data.particleEmitters2.Count() - 1, 0x70000000);
    errors.Complete(status);
    return !parse.FoundError();
  }

}

static void IAddParticleEmitter2Errors(TSet &errors) {
  AddObjectErrors(errors);
  errors.Add(MDLTOK_EMISSION_RATE, 1, 0);
  errors.Add(MDLTOK_GRAVITY, 0, 0);
  errors.Add(MDLTOK_LATITUDE, 1, 0);
  errors.Add(MDLTOK_VISIBILITY, 0, 0);
  errors.Add(MDLTOK_TEXTURE_ID, 1, 0);
  errors.Add(MDLTOK_ROWS, 1, 0);
  errors.Add(MDLTOK_COLS, 1, 0);
  errors.Add(MDLTOK_SEGMENT_COLOR, 0, 0);
  errors.Add(MDLTOK_TIME, 1, 0);
  errors.Add(MDLTOK_PARTICLE_SCALING, 0, 0);
  errors.Add(MDLTOK_ALPHA, 0, 0);
  errors.Add(MDLTOK_LIFESPAN_UV, 0, 0);
  errors.Add(MDLTOK_DECAY_UV, 0, 0);
  errors.Add(MDLTOK_TAIL_DECAY_UV, 0, 0);
  errors.Add(MDLTOK_TAIL_UV, 0, 0);
  errors.Add(MDLTOK_WIDTH, 0, 0);
  errors.Add(MDLTOK_HEIGHT, 0, 0);
  errors.Add(MDLTOK_LINE_EMITTER, 0, 0);
  errors.Add(MDLTOK_TAIL_LENGTH, 0, 0);
  errors.Add(MDLTOK_REPLACEABLE_ID, 0, 0);
  errors.Add(MDLTOK_UNFOGGED, 0, 0);
  errors.Add(MDLTOK_MODEL_SPACE, 0, 0);
  errors.Add(MDLTOK_PARTICLE_IVEL_LIN, 0, 0);
  errors.Add(MDLTOK_PARTICLE_IVEL_SCALE, 0, 0);
  errors.Add(MDLTOK_PARTICLE_GEOMETRY_MDL, 0, 0);
  errors.Add(MDLTOK_PARTICLE_RECURSION_MDL, 0, 0);
  errors.Add(MDLTOK_PARTICLE_TUMBLE, 0, 0);
  errors.Add(MDLTOK_PARTICLE_TWINKLE_ONOFF, 0, 0);
  errors.Add(MDLTOK_PARTICLE_TWINKLE_SCALE, 0, 0);
  errors.Add(MDLTOK_PARTICLE_ROTATION, 0, 0);
  errors.Add(MDLTOK_FPS, 0, 0);
  errors.Add(MDLTOK_DRAG, 0, 0);
  errors.Add(MDLTOK_PARTICLE_TUMBLE, 0, 0);
  errors.Add(MDLTOK_WIND, 0, 0);
  errors.Add(MDLTOK_SPLINE, 0, 0);
  errors.Add(MDLTOK_PARTICLE_ZSOURCE, 0, 0);
  errors.Add(MDLTOK_PARTICLE_PROJECT, 0, 0);
  errors.Add(MDLTOK_PARTICLE_FOLLOW, 0, 0);
  errors.Add(MDLTOK_PARTICLE_FOLLOW_PARAMS, 0, 0);
}

static void IReadParticleEmitter2(Parser &parse, TSet &errors, MDLPARTICLEEMITTER2 *emitter, CMDLStatus *status) {
  parse.Expect('{');
  LPCSTR tokentext;
  UINT   savedtoken = parse.Token(&tokentext, 0);
  while (savedtoken != '}' && savedtoken) {
    int expectAnimation = IExpectAnimation(parse, &savedtoken, &tokentext);
    if (!errors.Check(savedtoken)) {
      parse.FatalDuplicate(tokentext);
    }
    if (!ReadObjectBody(parse, savedtoken, 0, emitter, status) && !IReadParticleEmitter2Flags(parse, savedtoken, emitter) &&
        !IReadParticleEmitter2EmitterType(parse, savedtoken, emitter) && !ReadParticleEmitter2BlendMode(parse, savedtoken, emitter) &&
        !ReadParticleEmitter2Type(parse, savedtoken, emitter))
    {
      if (expectAnimation) {
        IReadParticleEmitter2KeyFrames(parse, savedtoken, tokentext, emitter);
      } else {
        IReadParticleEmitter2StaticData(parse, savedtoken, tokentext, emitter);
      }
    }
    savedtoken = parse.Token(&tokentext, 0);
  }
  parse.Expect('}', savedtoken, tokentext);
}

static void IReadParticleEmitter2KeyFrames(Parser &parse, UINT savedtoken, LPCSTR tokentext, MDLPARTICLEEMITTER2 *emitter) {
  switch (savedtoken) {
    case MDLTOK_SPEED:
      ReadObjectFloatKeyframes(parse, &emitter->speed);
      return;
    case MDLTOK_VARIATION:
      ReadObjectFloatKeyframes(parse, &emitter->variation);
      return;
    case MDLTOK_GRAVITY:
      ReadObjectFloatKeyframes(parse, &emitter->gravity);
      return;
    case MDLTOK_LATITUDE:
      ReadObjectFloatKeyframes(parse, &emitter->latitude);
      return;
    case MDLTOK_LONGITUDE:
      ReadObjectFloatKeyframes(parse, &emitter->longitude);
      return;
    case MDLTOK_VISIBILITY:
      ReadObjectFloatKeyframes(parse, &emitter->visibilityKeys);
      return;
    case MDLTOK_SQUIRT:
      emitter->squirts = 1;
      break;
    case MDLTOK_EMISSION_RATE:
      ReadObjectFloatKeyframes(parse, &emitter->emissionRate);
      return;
    case MDLTOK_SEGMENT_COLOR:
      IReadParticleEmitter2Color(parse, emitter);
      break;
    case MDLTOK_TIME:
      emitter->middleTime = parse.ExpectFloat();
      break;
    case MDLTOK_PARTICLE_SCALING:
      IReadFloatOption(parse, emitter->startScale, emitter->middleScale, emitter->endScale);
      break;
    case MDLTOK_ALPHA:
      IReadByteOption(parse, emitter->startAlpha, emitter->middleAlpha, emitter->endAlpha);
      break;
    case MDLTOK_LIFESPAN_UV:
      IReadIntOption(parse, emitter->lifespanUVAnimStart, emitter->lifespanUVAnimEnd, emitter->lifespanUVAnimRepeat);
      break;
    case MDLTOK_DECAY_UV:
      IReadIntOption(parse, emitter->decayUVAnimStart, emitter->decayUVAnimEnd, emitter->decayUVAnimRepeat);
      break;
    case MDLTOK_TAIL_UV:
      IReadIntOption(parse, emitter->tailUVAnimStart, emitter->tailUVAnimEnd, emitter->tailUVAnimRepeat);
      break;
    case MDLTOK_TAIL_DECAY_UV:
      IReadIntOption(parse, emitter->tailDecayUVAnimStart, emitter->tailDecayUVAnimEnd, emitter->tailDecayUVAnimRepeat);
      break;
    case MDLTOK_LIFESPAN:
      ReadObjectFloatKeyframes(parse, &emitter->life);
      return;
    case MDLTOK_WIDTH:
      ReadObjectFloatKeyframes(parse, &emitter->width);
      return;
    case MDLTOK_LENGTH:
      ReadObjectFloatKeyframes(parse, &emitter->length);
      return;
    case MDLTOK_PARTICLE_ZSOURCE:
      ReadObjectFloatKeyframes(parse, &emitter->zsource);
      return;
    case MDLTOK_ROWS:
      emitter->rows = parse.ExpectInt();
      break;
    case MDLTOK_COLS:
      emitter->cols = parse.ExpectInt();
      break;
    case MDLTOK_TAIL_LENGTH:
      emitter->tailLength = parse.ExpectFloat();
      break;
    case MDLTOK_TEXTURE_ID:
      emitter->textureId = parse.ExpectInt();
      break;
    case MDLTOK_PRIORITYPLANE:
      emitter->priorityPlane = parse.ExpectInt();
      break;
    case MDLTOK_REPLACEABLE_ID:
      emitter->replaceableId = parse.ExpectInt();
      break;
    case MDLTOK_PARTICLE_GEOMETRY_MDL:
      if (tokentext = parse.ExpectString()) {
        SStrCopy(emitter->geometryMdl, tokentext, 260);
      }
      break;
    case MDLTOK_PARTICLE_RECURSION_MDL:
      if (tokentext = parse.ExpectString()) {
        SStrCopy(emitter->recursionMdl, tokentext, 260);
      }
      break;
    case MDLTOK_FPS:
      emitter->twinkleFPS = parse.ExpectFloat();
      break;
    case MDLTOK_PARTICLE_TWINKLE_ONOFF:
      emitter->twinkleOnOff = parse.ExpectFloat();
      break;
    case MDLTOK_PARTICLE_TWINKLE_SCALE:
      IReadFloatOption(parse, emitter->twinkleScaleMin, emitter->twinkleScaleMax);
      break;
    case MDLTOK_PARTICLE_IVEL_SCALE:
      emitter->ivelScale = parse.ExpectFloat();
      break;
    case MDLTOK_PARTICLE_TUMBLE:
      IReadFloatOption(
          parse, emitter->tumblexMin, emitter->tumblexMax, emitter->tumbleyMin, emitter->tumbleyMax, emitter->tumblezMin, emitter->tumblezMax
      );
      break;
    case MDLTOK_DRAG:
      emitter->drag = parse.ExpectFloat();
      break;
    case MDLTOK_PARTICLE_ROTATION:
      emitter->spin = parse.ExpectFloat();
      break;
    case MDLTOK_WIND:
      parse.Expect('{');
      IReadFloatOption(parse, emitter->windVector.x, emitter->windVector.y, emitter->windVector.z);
      parse.Expect(',');
      emitter->windTime = parse.ExpectFloat();
      parse.Expect('}');
      break;
    case MDLTOK_SPLINE:
      IReadSpline(parse, emitter->spline);
      return;
    case MDLTOK_PARTICLE_FOLLOW_PARAMS:
      parse.Expect('{');
      emitter->followSpeed1 = parse.ExpectFloat();
      parse.Expect(',');
      emitter->followScale1 = parse.ExpectFloat();
      parse.Expect(',');
      emitter->followSpeed2 = parse.ExpectFloat();
      parse.Expect(',');
      emitter->followScale2 = parse.ExpectFloat();
      parse.Expect('}');
      break;
    default:
      parse.FatalUnexpected(tokentext);
      return;
  }
  parse.Expect(',');
}

static void IReadIntOption(Parser &parse, UINT &p1, UINT &b, UINT &c) {
  parse.Expect('{');
  p1 = parse.ExpectInt();
  parse.Expect(',');
  b = parse.ExpectInt();
  parse.Expect(',');
  c = parse.ExpectInt();
  parse.Expect('}');
}

static void IReadByteOption(Parser &parse, BYTE &p1, BYTE &b, BYTE &c) {
  parse.Expect('{');
  p1 = parse.ExpectInt();
  parse.Expect(',');
  b = parse.ExpectInt();
  parse.Expect(',');
  c = parse.ExpectInt();
  parse.Expect('}');
}

static void IReadFloatOption(Parser &parse, float &p1, float &b, float &c) {
  parse.Expect('{');
  p1 = parse.ExpectFloat();
  parse.Expect(',');
  b = parse.ExpectFloat();
  parse.Expect(',');
  c = parse.ExpectFloat();
  parse.Expect('}');
}

static void IReadFloatOption(Parser &parser, float &f1, float &b) {
  parser.Expect('{');
  f1 = parser.ExpectFloat();
  parser.Expect(',');
  b = parser.ExpectFloat();
  parser.Expect('}');
}

static void IReadFloatOption(Parser &parser, float &f1, float &b, float &c, float &d, float &e, float &f) {
  parser.Expect('{');
  f1 = parser.ExpectFloat();
  parser.Expect(',');
  b = parser.ExpectFloat();
  parser.Expect(',');
  c = parser.ExpectFloat();
  parser.Expect(',');
  d = parser.ExpectFloat();
  parser.Expect(',');
  e = parser.ExpectFloat();
  parser.Expect(',');
  f = parser.ExpectFloat();
  parser.Expect('}');
}

static void IReadParticleEmitter2Color(Parser &parse, MDLPARTICLEEMITTER2 *emitter) {
  parse.Expect('{');
  parse.Expect(MDLTOK_COLOR);
  IReadFloatOption(parse, emitter->startColor.b, emitter->startColor.g, emitter->startColor.r);
  parse.Expect(',');
  parse.Expect(MDLTOK_COLOR);
  IReadFloatOption(parse, emitter->middleColor.b, emitter->middleColor.g, emitter->middleColor.r);
  parse.Expect(',');
  parse.Expect(MDLTOK_COLOR);
  IReadFloatOption(parse, emitter->endColor.b, emitter->endColor.g, emitter->endColor.r);
  parse.Expect(',');
  parse.Expect('}');
}

static void IReadSpline(Parser &parse, TSGrowableArray<NTempest::C3Vector> &spline) {
  parse.Expect('{');
  parse.Expect(MDLTOK_BEZIER);
  parse.Expect(MDLTOK_VERTICES);
  ReadVertices(parse, MDL::TokenText(MDLTOK_VERTICES), &spline);
  parse.Expect('}');
}

static void IReadParticleEmitter2StaticData(Parser &parse, UINT savedtoken, LPCSTR tokentext, MDLPARTICLEEMITTER2 *emitter) {
  switch (savedtoken) {
    case MDLTOK_SPEED:
      ReadFloatKeyData(parse, &emitter->staticSpeed, 1);
      break;
    case MDLTOK_GRAVITY:
      ReadFloatKeyData(parse, &emitter->staticGravity, 1);
      break;
    case MDLTOK_LATITUDE:
      ReadFloatKeyData(parse, &emitter->staticLatitude, 1);
      break;
    case MDLTOK_LONGITUDE:
      ReadFloatKeyData(parse, &emitter->staticLongitude, 1);
      break;
    case MDLTOK_VARIATION:
      ReadFloatKeyData(parse, &emitter->staticVariation, 1);
      break;
    case MDLTOK_EMISSION_RATE:
      ReadFloatKeyData(parse, &emitter->staticEmissionRate, 1);
      break;
    case MDLTOK_LENGTH:
      ReadFloatKeyData(parse, &emitter->staticLength, 1);
      break;
    case MDLTOK_WIDTH:
      ReadFloatKeyData(parse, &emitter->staticWidth, 1);
      break;
    case MDLTOK_PARTICLE_ZSOURCE:
      ReadFloatKeyData(parse, &emitter->staticZsource, 1);
      break;
    case MDLTOK_LIFESPAN:
      ReadFloatKeyData(parse, &emitter->staticLife, 1);
      break;
    default:
      parse.FatalUnexpected(tokentext);
      break;
  }
  parse.Expect(',');
}

static BOOL ReadParticleEmitter2BlendMode(Parser &parse, UINT savedtoken, MDLPARTICLEEMITTER2 *emitter) {
  switch (savedtoken) {
    case MDLTOK_BLEND:
      emitter->blendMode = MDLPARTICLEEMITTER2::PBM_BLEND;
      break;
    case MDLTOK_ADDITIVE:
      emitter->blendMode = MDLPARTICLEEMITTER2::PBM_ADD;
      break;
    case MDLTOK_MODULATE:
      emitter->blendMode = MDLPARTICLEEMITTER2::PBM_MODULATE;
      break;
    case MDLTOK_MODULATE2X:
      emitter->blendMode = MDLPARTICLEEMITTER2::PBM_MODULATE_2X;
      break;
    case MDLTOK_ALPHA_KEY:
      emitter->blendMode = MDLPARTICLEEMITTER2::PBM_ALPHA_KEY;
      break;
    default:
      return 0;
  }
  parse.Expect(',');
  return 1;
}

static BOOL IReadParticleEmitter2EmitterType(Parser &parse, UINT savedtoken, MDLPARTICLEEMITTER2 *emitter) {
  if (savedtoken == MDLTOK_TYPE) {
    emitter->emitterType = (MDLPARTICLEEMITTER2::PARTICLE_EMITTER_TYPE)parse.ExpectInt();
    parse.Expect(',');
    return 1;
  }
  return 0;
}

static BOOL ReadParticleEmitter2Type(Parser &parse, UINT savedtoken, MDLPARTICLEEMITTER2 *emitter) {
  switch (savedtoken) {
    case MDLTOK_BOTH:
      emitter->type = MDLPARTICLEEMITTER2::PT_BOTH;
      break;
    case MDLTOK_HEAD:
      emitter->type = MDLPARTICLEEMITTER2::PT_HEAD;
      break;
    case MDLTOK_TAIL:
      emitter->type = MDLPARTICLEEMITTER2::PT_TAIL;
      break;
    default:
      return 0;
  }
  parse.Expect(',');
  return 1;
}

static BOOL IReadParticleEmitter2Flags(Parser &parse, UINT savedtoken, MDLPARTICLEEMITTER2 *emitter) {
  switch (savedtoken) {
    case MDLTOK_UNSHADED:
      emitter->flags |= 0x00008000;
      break;
    case MDLTOK_SORTPRIMSFARZ:
      emitter->flags |= 0x00010000;
      break;
    case MDLTOK_LINE_EMITTER:
      emitter->flags |= 0x00020000;
      break;
    case MDLTOK_UNFOGGED:
      emitter->flags |= 0x00040000;
      break;
    case MDLTOK_MODEL_SPACE:
      emitter->flags |= 0x00080000;
      break;
    case MDLTOK_PARTICLE_IVEL_LIN:
      emitter->flags |= 0x00200000;
      break;
    case MDLTOK_PARTICLE_INHERIT_SCALE:
      emitter->flags |= 0x00100000;
      break;
    case MDLTOK_PARTICLE_0XKILL:
      emitter->flags |= 0x00400000;
      break;
    case MDLTOK_PARTICLE_ZVEL_ONLY:
      emitter->flags |= 0x00800000;
      break;
    case MDLTOK_PARTICLE_TUMBLER:
      emitter->flags |= 0x01000000;
      break;
    case MDLTOK_TAIL_GROWS:
      emitter->flags |= 0x02000000;
      break;
    case MDLTOK_PARTICLE_EXTRUDE:
      emitter->flags |= 0x04000000;
      break;
    case MDLTOK_PARTICLE_XYQUADS:
      emitter->flags |= 0x08000000;
      break;
    case MDLTOK_PARTICLE_PROJECT:
      emitter->flags |= 0x10000000;
      break;
    case MDLTOK_PARTICLE_FOLLOW:
      emitter->flags |= 0x20000000;
      break;
    default:
      return 0;
  }
  parse.Expect(',');
  return 1;
}

namespace MDL {

  BOOL WriteParticleEmitters2(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
    if (data.model.animationFile[0]) {
      return 1;
    }
    UINT                       numEmitters = data.particleEmitters2.Count();
    int                        needObjIds = numEmitters != data.objects.Count();
    const MDLPARTICLEEMITTER2 *pEmit = data.particleEmitters2.Ptr();
    for (UINT i = numEmitters; i; --i, ++pEmit) {
      IWriteParticleEmitter2(data, *pEmit, needObjIds, buffer);
    }
    return 1;
  }

}

static void IWriteParticleEmitter2(const MDLDATA &data, const MDLPARTICLEEMITTER2 &emitter, int needObjIds, TSGrowableArray<char> &buffer) {
  WriteObjectHeader(data, emitter, MDLTOK_PARTICLEEMITTER2, needObjIds, buffer);
  IWritePE2Flags(emitter, buffer);
  MDL::WriteLine(buffer, "\t%s %d,\n", MDL::TokenText(MDLTOK_TYPE), emitter.emitterType);

  if (emitter.speed.keys.Count()) {
    WriteFloatKeyFrames(MDLTOK_SPEED, "\t", emitter.speed, buffer);
  } else {
    MDL::WriteLine(buffer, "%s%s %s ", "\t", MDL::TokenText(MDLTOK_STATIC), MDL::TokenText(MDLTOK_SPEED));
    WriteKeyData(buffer, &emitter.staticSpeed, 1);
  }
  if (emitter.variation.keys.Count()) {
    WriteFloatKeyFrames(MDLTOK_VARIATION, "\t", emitter.variation, buffer);
  } else {
    MDL::WriteLine(buffer, "%s%s %s ", "\t", MDL::TokenText(MDLTOK_STATIC), MDL::TokenText(MDLTOK_VARIATION));
    WriteKeyData(buffer, &emitter.staticVariation, 1);
  }
  if (emitter.latitude.keys.Count()) {
    WriteFloatKeyFrames(MDLTOK_LATITUDE, "\t", emitter.latitude, buffer);
  } else {
    MDL::WriteLine(buffer, "%s%s %s ", "\t", MDL::TokenText(MDLTOK_STATIC), MDL::TokenText(MDLTOK_LATITUDE));
    WriteKeyData(buffer, &emitter.staticLatitude, 1);
  }
  if (emitter.longitude.keys.Count()) {
    WriteFloatKeyFrames(MDLTOK_LONGITUDE, "\t", emitter.longitude, buffer);
  } else {
    MDL::WriteLine(buffer, "%s%s %s ", "\t", MDL::TokenText(MDLTOK_STATIC), MDL::TokenText(MDLTOK_LONGITUDE));
    WriteKeyData(buffer, &emitter.staticLongitude, 1);
  }
  if (emitter.gravity.keys.Count()) {
    WriteFloatKeyFrames(MDLTOK_GRAVITY, "\t", emitter.gravity, buffer);
  } else {
    MDL::WriteLine(buffer, "%s%s %s ", "\t", MDL::TokenText(MDLTOK_STATIC), MDL::TokenText(MDLTOK_GRAVITY));
    WriteKeyData(buffer, &emitter.staticGravity, 1);
  }
  WriteFloatKeyFrames(MDLTOK_VISIBILITY, "\t", emitter.visibilityKeys, buffer);
  if (emitter.squirts) {
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(MDLTOK_SQUIRT));
  }
  if (emitter.life.keys.Count()) {
    WriteFloatKeyFrames(MDLTOK_LIFESPAN, "\t", emitter.life, buffer);
  } else {
    MDL::WriteLine(buffer, "%s%s %s ", "\t", MDL::TokenText(MDLTOK_STATIC), MDL::TokenText(MDLTOK_LIFESPAN));
    WriteKeyData(buffer, &emitter.staticLife, 1);
  }
  if (emitter.emissionRate.keys.Count()) {
    WriteFloatKeyFrames(MDLTOK_EMISSION_RATE, "\t", emitter.emissionRate, buffer);
  } else {
    MDL::WriteLine(buffer, "%s%s %s ", "\t", MDL::TokenText(MDLTOK_STATIC), MDL::TokenText(MDLTOK_EMISSION_RATE));
    WriteKeyData(buffer, &emitter.staticEmissionRate, 1);
  }
  if (emitter.width.keys.Count()) {
    WriteFloatKeyFrames(MDLTOK_WIDTH, "\t", emitter.width, buffer);
  } else {
    MDL::WriteLine(buffer, "%s%s %s ", "\t", MDL::TokenText(MDLTOK_STATIC), MDL::TokenText(MDLTOK_WIDTH));
    WriteKeyData(buffer, &emitter.staticWidth, 1);
  }
  if (emitter.length.keys.Count()) {
    WriteFloatKeyFrames(MDLTOK_LENGTH, "\t", emitter.length, buffer);
  } else {
    MDL::WriteLine(buffer, "%s%s %s ", "\t", MDL::TokenText(MDLTOK_STATIC), MDL::TokenText(MDLTOK_LENGTH));
    WriteKeyData(buffer, &emitter.staticLength, 1);
  }
  if (emitter.zsource.keys.Count()) {
    WriteFloatKeyFrames(MDLTOK_PARTICLE_ZSOURCE, "\t", emitter.zsource, buffer);
  } else {
    MDL::WriteLine(buffer, "%s%s %s ", "\t", MDL::TokenText(MDLTOK_STATIC), MDL::TokenText(MDLTOK_PARTICLE_ZSOURCE));
    WriteKeyData(buffer, &emitter.staticZsource, 1);
  }

  IWriteParticleEmitter2BlendMode(emitter, buffer);
  MDL::WriteLine(buffer, "\t%s %u,\n", MDL::TokenText(MDLTOK_ROWS), emitter.rows);
  MDL::WriteLine(buffer, "\t%s %u,\n", MDL::TokenText(MDLTOK_COLS), emitter.cols);
  IWriteParticleEmitter2Type(emitter, buffer);
  MDL::WriteLine(buffer, "\t%s %g,\n", MDL::TokenText(MDLTOK_TAIL_LENGTH), emitter.tailLength);
  MDL::WriteLine(buffer, "\t%s %g,\n", MDL::TokenText(MDLTOK_TIME), emitter.middleTime);
  IWriteParticleEmitter2Colors(emitter, buffer);
  MDL::WriteLine(buffer, "\t%s {%u, %u, %u},\n", MDL::TokenText(MDLTOK_ALPHA), emitter.startAlpha, emitter.middleAlpha, emitter.endAlpha);
  MDL::WriteLine(buffer, "\t%s {%g, %g, %g},\n", MDL::TokenText(MDLTOK_PARTICLE_SCALING), emitter.startScale, emitter.middleScale, emitter.endScale);
  MDL::WriteLine(
      buffer, "\t%s {%u, %u, %u},\n", MDL::TokenText(MDLTOK_LIFESPAN_UV), emitter.lifespanUVAnimStart, emitter.lifespanUVAnimEnd, emitter.lifespanUVAnimRepeat
  );
  MDL::WriteLine(buffer, "\t%s {%u, %u, %u},\n", MDL::TokenText(MDLTOK_DECAY_UV), emitter.decayUVAnimStart, emitter.decayUVAnimEnd, emitter.decayUVAnimRepeat);
  MDL::WriteLine(buffer, "\t%s {%u, %u, %u},\n", MDL::TokenText(MDLTOK_TAIL_UV), emitter.tailUVAnimStart, emitter.tailUVAnimEnd, emitter.tailUVAnimRepeat);
  MDL::WriteLine(
      buffer, "\t%s {%u, %u, %u},\n", MDL::TokenText(MDLTOK_TAIL_DECAY_UV), emitter.tailDecayUVAnimStart, emitter.tailDecayUVAnimEnd, emitter.tailDecayUVAnimRepeat
  );
  MDL::WriteLine(buffer, "\t%s %u,\n", MDL::TokenText(MDLTOK_TEXTURE_ID), emitter.textureId);
  if (emitter.priorityPlane) {
    MDL::WriteLine(buffer, "\t%s %d,\n", MDL::TokenText(MDLTOK_PRIORITYPLANE), emitter.priorityPlane);
  }
  if (emitter.replaceableId) {
    MDL::WriteLine(buffer, "\t%s %d,\n", MDL::TokenText(MDLTOK_REPLACEABLE_ID), emitter.replaceableId);
  }
  if (SStrLen(emitter.geometryMdl)) {
    MDL::WriteLine(buffer, "\t%s \"%s\",\n", MDL::TokenText(MDLTOK_PARTICLE_GEOMETRY_MDL), (LPCSTR)emitter.geometryMdl);
  }
  if (SStrLen(emitter.recursionMdl)) {
    MDL::WriteLine(buffer, "\t%s \"%s\",\n", MDL::TokenText(MDLTOK_PARTICLE_RECURSION_MDL), (LPCSTR)emitter.recursionMdl);
  }
  MDL::WriteLine(buffer, "\t%s %f,\n", MDL::TokenText(MDLTOK_FPS), emitter.twinkleFPS);
  MDL::WriteLine(buffer, "\t%s %f,\n", MDL::TokenText(MDLTOK_PARTICLE_TWINKLE_ONOFF), emitter.twinkleOnOff);
  MDL::WriteLine(buffer, "\t%s {%f, %f},\n", MDL::TokenText(MDLTOK_PARTICLE_TWINKLE_SCALE), emitter.twinkleScaleMin, emitter.twinkleScaleMax);
  MDL::WriteLine(buffer, "\t%s %f,\n", MDL::TokenText(MDLTOK_PARTICLE_IVEL_SCALE), emitter.ivelScale);
  MDL::WriteLine(
      buffer, "\t%s {%f, %f, %f, %f, %f, %f},\n", MDL::TokenText(MDLTOK_PARTICLE_TUMBLE), emitter.tumblexMin, emitter.tumblexMax, emitter.tumbleyMin,
      emitter.tumbleyMax, emitter.tumblezMin, emitter.tumblezMax
  );
  MDL::WriteLine(buffer, "\t%s %f,\n", MDL::TokenText(MDLTOK_DRAG), emitter.drag);
  MDL::WriteLine(buffer, "\t%s %f,\n", MDL::TokenText(MDLTOK_PARTICLE_ROTATION), emitter.spin);
  MDL::WriteLine(
      buffer, "\t%s { { %f, %f, %f }, %f },\n", MDL::TokenText(MDLTOK_WIND), emitter.windVector.x, emitter.windVector.y, emitter.windVector.z,
      emitter.windTime
  );
  MDL::WriteLine(
      buffer, "\t%s { %f, %f, %f, %f },\n", MDL::TokenText(MDLTOK_PARTICLE_FOLLOW_PARAMS), emitter.followSpeed1, emitter.followScale1, emitter.followSpeed2,
      emitter.followScale2
  );
  if (emitter.emitterType == MDLPARTICLEEMITTER2::PET_SPLINE) {
    IWriteSpline(emitter.spline, buffer);
  }
  WriteObjectTrailer(emitter, buffer);
}

static void IWriteParticleEmitter2BlendMode(const MDLPARTICLEEMITTER2 &section, TSGrowableArray<char> &buffer) {
  UINT token;
  switch (section.blendMode) {
    case MDLPARTICLEEMITTER2::PBM_ADD:
      token = MDLTOK_ADDITIVE;
      break;
    case MDLPARTICLEEMITTER2::PBM_MODULATE:
      token = MDLTOK_MODULATE;
      break;
    case MDLPARTICLEEMITTER2::PBM_MODULATE_2X:
      token = MDLTOK_MODULATE2X;
      break;
    case MDLPARTICLEEMITTER2::PBM_ALPHA_KEY:
      token = MDLTOK_ALPHA_KEY;
      break;
    default:
      token = MDLTOK_BLEND;
      break;
  }
  MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(token));
}

static void IWriteParticleEmitter2Type(const MDLPARTICLEEMITTER2 &section, TSGrowableArray<char> &buffer) {
  UINT token;
  switch (section.type) {
    case MDLPARTICLEEMITTER2::PT_TAIL:
      token = MDLTOK_TAIL;
      break;
    case MDLPARTICLEEMITTER2::PT_BOTH:
      token = MDLTOK_BOTH;
      break;
    default:
      token = MDLTOK_HEAD;
      break;
  }
  MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(token));
}

static void IWriteParticleEmitter2Colors(const MDLPARTICLEEMITTER2 &emitter, TSGrowableArray<char> &buffer) {
  MDL::WriteLine(buffer, "\t%s {\n", MDL::TokenText(MDLTOK_SEGMENT_COLOR));
  MDL::WriteLine(buffer, "%s%s ", "\t\t", MDL::TokenText(MDLTOK_COLOR));
  WriteKeyData(buffer, &emitter.startColor.b, 3);
  MDL::WriteLine(buffer, "%s%s ", "\t\t", MDL::TokenText(MDLTOK_COLOR));
  WriteKeyData(buffer, &emitter.middleColor.b, 3);
  MDL::WriteLine(buffer, "%s%s ", "\t\t", MDL::TokenText(MDLTOK_COLOR));
  WriteKeyData(buffer, &emitter.endColor.b, 3);
  MDL::WriteLine(buffer, "\t},\n");
}

static void IWritePE2Flags(const MDLPARTICLEEMITTER2 &emitter, TSGrowableArray<char> &buffer) {
  if (emitter.flags & 0x00010000)
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(MDLTOK_SORTPRIMSFARZ));
  if (emitter.flags & 0x00008000)
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(MDLTOK_UNSHADED));
  if (emitter.flags & 0x00020000)
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(MDLTOK_LINE_EMITTER));
  if (emitter.flags & 0x00040000)
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(MDLTOK_UNFOGGED));
  if (emitter.flags & 0x00080000)
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(MDLTOK_MODEL_SPACE));
  if (emitter.flags & 0x00200000)
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(MDLTOK_PARTICLE_IVEL_LIN));
  if (emitter.flags & 0x00100000)
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(MDLTOK_PARTICLE_INHERIT_SCALE));
  if (emitter.flags & 0x00400000)
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(MDLTOK_PARTICLE_0XKILL));
  if (emitter.flags & 0x00800000)
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(MDLTOK_PARTICLE_ZVEL_ONLY));
  if (emitter.flags & 0x01000000)
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(MDLTOK_PARTICLE_TUMBLER));
  if (emitter.flags & 0x02000000)
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(MDLTOK_TAIL_GROWS));
  if (emitter.flags & 0x04000000)
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(MDLTOK_PARTICLE_EXTRUDE));
  if (emitter.flags & 0x08000000)
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(MDLTOK_PARTICLE_XYQUADS));
  if (emitter.flags & 0x10000000)
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(MDLTOK_PARTICLE_PROJECT));
  if (emitter.flags & 0x20000000)
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(MDLTOK_PARTICLE_FOLLOW));
}

static void IWriteSpline(const TSGrowableArray<NTempest::C3Vector> &points, TSGrowableArray<char> &buffer) {
  MDL::WriteLine(buffer, "%s {\n", MDL::TokenText(MDLTOK_SPLINE));
  MDL::WriteLine(buffer, "\t%s\n", MDL::TokenText(MDLTOK_BEZIER));
  WriteVertices(points, MDLTOK_VERTICES, buffer);
  MDL::WriteLine(buffer, "}\n");
}

namespace MDL {

  BOOL WriteBinParticleEmitters2(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *status) {
    UINT numEmitters = data.particleEmitters2.Count();
    if (data.model.animationFile[0] || !numEmitters) {
      return 1;
    }
    buf.AddDword('2ERP');
    UINT totalSize = 4;
    UINT i;
    for (i = 0; i < numEmitters; ++i) {
      totalSize += GetBinParticleEmitter2Size(data.particleEmitters2[i]);
    }
    buf.AddUint(totalSize);
    buf.AddUint(numEmitters);
    for (i = 0; i < numEmitters; ++i) {
      IWriteBinParticleEmitter2(data.particleEmitters2[i], buf, status);
    }
    return 1;
  }

}

static UINT GetBinParticleEmitter2Size(const MDLPARTICLEEMITTER2 &section) {
  UINT size = GetBinGenObjectSize(section) + 4 + GetNonAnimEmitterDataSize(section);
  if (section.speed.keys.Count()) {
    size += 16 + section.speed.keys.Count() * (4 + 4 * (section.speed.type > TRACK_LINEAR ? 3 : 1));
  }
  if (section.variation.keys.Count()) {
    size += 16 + section.variation.keys.Count() * (4 + 4 * (section.variation.type > TRACK_LINEAR ? 3 : 1));
  }
  if (section.latitude.keys.Count()) {
    size += 16 + section.latitude.keys.Count() * (4 + 4 * (section.latitude.type > TRACK_LINEAR ? 3 : 1));
  }
  if (section.longitude.keys.Count()) {
    size += 16 + section.longitude.keys.Count() * (4 + 4 * (section.longitude.type > TRACK_LINEAR ? 3 : 1));
  }
  if (section.gravity.keys.Count()) {
    size += 16 + section.gravity.keys.Count() * (4 + 4 * (section.gravity.type > TRACK_LINEAR ? 3 : 1));
  }
  if (section.life.keys.Count()) {
    size += 16 + section.life.keys.Count() * (4 + 4 * (section.life.type > TRACK_LINEAR ? 3 : 1));
  }
  if (section.emissionRate.keys.Count()) {
    size += 16 + section.emissionRate.keys.Count() * (4 + 4 * (section.emissionRate.type > TRACK_LINEAR ? 3 : 1));
  }
  if (section.width.keys.Count()) {
    size += 16 + section.width.keys.Count() * (4 + 4 * (section.width.type > TRACK_LINEAR ? 3 : 1));
  }
  if (section.length.keys.Count()) {
    size += 16 + section.length.keys.Count() * (4 + 4 * (section.length.type > TRACK_LINEAR ? 3 : 1));
  }
  if (section.zsource.keys.Count()) {
    size += 16 + section.zsource.keys.Count() * (4 + 4 * (section.zsource.type > TRACK_LINEAR ? 3 : 1));
  }
  if (section.visibilityKeys.keys.Count()) {
    size += 16 + section.visibilityKeys.keys.Count() * (4 + 4 * (section.visibilityKeys.type > TRACK_LINEAR ? 3 : 1));
  }
  return size;
}

static UINT GetNonAnimEmitterDataSize(const MDLPARTICLEEMITTER2 &section) {
  return 791 + 12 * section.spline.Count();
}

static void IWriteBinParticleEmitter2(const MDLPARTICLEEMITTER2 &section, CMsgBuffer &buf, CMDLStatus *status) {
  buf.AddUint(GetBinParticleEmitter2Size(section));
  WriteBinGenObject(section, buf, status);
  buf.AddUint(GetNonAnimEmitterDataSize(section));
  buf.AddUint(section.emitterType);
  buf.AddFloat(section.staticSpeed);
  buf.AddFloat(section.staticVariation);
  buf.AddFloat(section.staticLatitude);
  buf.AddFloat(section.staticLongitude);
  buf.AddFloat(section.staticGravity);
  buf.AddFloat(section.staticZsource);
  buf.AddFloat(section.staticLife);
  buf.AddFloat(section.staticEmissionRate);
  buf.AddFloat(section.staticLength);
  buf.AddFloat(section.staticWidth);
  buf.AddUint(section.rows);
  buf.AddUint(section.cols);
  buf.AddUint(section.type);
  buf.AddFloat(section.tailLength);
  buf.AddFloat(section.middleTime);
  buf.AddFloat(section.startColor.r);
  buf.AddFloat(section.startColor.g);
  buf.AddFloat(section.startColor.b);
  buf.AddFloat(section.middleColor.r);
  buf.AddFloat(section.middleColor.g);
  buf.AddFloat(section.middleColor.b);
  buf.AddFloat(section.endColor.r);
  buf.AddFloat(section.endColor.g);
  buf.AddFloat(section.endColor.b);
  buf.AddByte(section.startAlpha);
  buf.AddByte(section.middleAlpha);
  buf.AddByte(section.endAlpha);
  buf.AddFloat(section.startScale);
  buf.AddFloat(section.middleScale);
  buf.AddFloat(section.endScale);
  buf.AddUint(section.lifespanUVAnimStart);
  buf.AddUint(section.lifespanUVAnimEnd);
  buf.AddUint(section.lifespanUVAnimRepeat);
  buf.AddUint(section.decayUVAnimStart);
  buf.AddUint(section.decayUVAnimEnd);
  buf.AddUint(section.decayUVAnimRepeat);
  buf.AddUint(section.tailUVAnimStart);
  buf.AddUint(section.tailUVAnimEnd);
  buf.AddUint(section.tailUVAnimRepeat);
  buf.AddUint(section.tailDecayUVAnimStart);
  buf.AddUint(section.tailDecayUVAnimEnd);
  buf.AddUint(section.tailDecayUVAnimRepeat);
  buf.AddUint(section.blendMode);
  buf.AddUint(section.textureId);
  buf.AddInt(section.priorityPlane);
  buf.AddUint(section.replaceableId);
  buf.AddTcharArray(section.geometryMdl, 260, 1);
  buf.AddTcharArray(section.recursionMdl, 260, 1);
  buf.AddFloat(section.twinkleFPS);
  buf.AddFloat(section.twinkleOnOff);
  buf.AddFloat(section.twinkleScaleMin);
  buf.AddFloat(section.twinkleScaleMax);
  buf.AddFloat(section.ivelScale);
  buf.AddFloat(section.tumblexMin);
  buf.AddFloat(section.tumblexMax);
  buf.AddFloat(section.tumbleyMin);
  buf.AddFloat(section.tumbleyMax);
  buf.AddFloat(section.tumblezMin);
  buf.AddFloat(section.tumblezMax);
  buf.AddFloat(section.drag);
  buf.AddFloat(section.spin);
  buf.AddFloat(section.windVector.x);
  buf.AddFloat(section.windVector.y);
  buf.AddFloat(section.windVector.z);
  buf.AddFloat(section.windTime);
  buf.AddFloat(section.followSpeed1);
  buf.AddFloat(section.followScale1);
  buf.AddFloat(section.followSpeed2);
  buf.AddFloat(section.followScale2);
  buf.AddUint(section.spline.Count());
  if (section.spline.Count()) {
    buf.AddFloatArray(&section.spline[0].x, 3 * section.spline.Count());
  }
  buf.AddUint(section.squirts);
  WriteBinFloatKeyFrames(section.emissionRate, 'E2PK', buf);
  WriteBinFloatKeyFrames(section.gravity, 'G2PK', buf);
  WriteBinFloatKeyFrames(section.longitude, 'NLPK', buf);
  WriteBinFloatKeyFrames(section.latitude, 'L2PK', buf);
  WriteBinFloatKeyFrames(section.speed, 'S2PK', buf);
  WriteBinFloatKeyFrames(section.variation, 'R2PK', buf);
  WriteBinFloatKeyFrames(section.length, 'N2PK', buf);
  WriteBinFloatKeyFrames(section.width, 'W2PK', buf);
  WriteBinFloatKeyFrames(section.zsource, 'Z2PK', buf);
  WriteBinFloatKeyFrames(section.visibilityKeys, 'SIVK', buf);
  WriteBinFloatKeyFrames(section.life, 'FILK', buf);
}

namespace MDL {

  BOOL ReadBinParticleEmitters2(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status) {
    UINT totalRead = 4;
    UINT numEmitters = buf.GetUint();
    data.particleEmitters2.SetCount(0);
    data.particleEmitters2.ReserveSpace(numEmitters);
    while (totalRead < length) {
      MDLPARTICLEEMITTER2 *pEmit = data.particleEmitters2.New();
      if (!pEmit) {
        status->FatalFlunked("ParticleEmitter2", -1);
        return 0;
      }
      if (!ReadBinParticleEmitter2(buf, pEmit, status, totalRead)) {
        status->Add(STATUS_ERROR, "Error reading ParticleEmitter2.\n");
        return 0;
      }
      if (totalRead > length) {
        status->FatalOverran("ParticleEmitter2 Section", -1);
        return 0;
      }
      ReadBinObjectEnd(data, pEmit, data.particleEmitters2.Count() - 1, 0x70000000);
    }
    return 1;
  }

}

static BOOL ReadBinParticleEmitter2(CMsgBuffer &buf, MDLPARTICLEEMITTER2 *pEmit, CMDLStatus *status, UINT &totalRead) {
  UINT sectionLength = buf.GetUint();
  UINT localBytesRead = 4;
  if (!ReadBinGenObject(*pEmit, buf, status, localBytesRead)) {
    status->Add(STATUS_ERROR, "Error reading gen object portion of ParticleEmitter2.\n");
    return 0;
  }
  buf.GetUint();
  localBytesRead += 4;
  pEmit->emitterType = (MDLPARTICLEEMITTER2::PARTICLE_EMITTER_TYPE)buf.GetUint();
  localBytesRead += 4;
  pEmit->staticSpeed = buf.GetFloat();
  localBytesRead += 4;
  pEmit->staticVariation = buf.GetFloat();
  localBytesRead += 4;
  pEmit->staticLatitude = buf.GetFloat();
  localBytesRead += 4;
  pEmit->staticLongitude = buf.GetFloat();
  localBytesRead += 4;
  pEmit->staticGravity = buf.GetFloat();
  localBytesRead += 4;
  pEmit->staticZsource = buf.GetFloat();
  localBytesRead += 4;
  pEmit->staticLife = buf.GetFloat();
  localBytesRead += 4;
  pEmit->staticEmissionRate = buf.GetFloat();
  localBytesRead += 4;
  pEmit->staticLength = buf.GetFloat();
  localBytesRead += 4;
  pEmit->staticWidth = buf.GetFloat();
  localBytesRead += 4;
  pEmit->rows = buf.GetUint();
  localBytesRead += 4;
  pEmit->cols = buf.GetUint();
  localBytesRead += 4;
  pEmit->type = (MDLPARTICLEEMITTER2::PARTICLE_TYPE)buf.GetUint();
  localBytesRead += 4;
  pEmit->tailLength = buf.GetFloat();
  localBytesRead += 4;
  pEmit->middleTime = buf.GetFloat();
  localBytesRead += 4;
  pEmit->startColor.r = buf.GetFloat();
  pEmit->startColor.g = buf.GetFloat();
  pEmit->startColor.b = buf.GetFloat();
  localBytesRead += 12;
  pEmit->middleColor.r = buf.GetFloat();
  pEmit->middleColor.g = buf.GetFloat();
  pEmit->middleColor.b = buf.GetFloat();
  localBytesRead += 12;
  pEmit->endColor.r = buf.GetFloat();
  pEmit->endColor.g = buf.GetFloat();
  pEmit->endColor.b = buf.GetFloat();
  localBytesRead += 12;
  pEmit->startAlpha = buf.GetByte();
  pEmit->middleAlpha = buf.GetByte();
  pEmit->endAlpha = buf.GetByte();
  localBytesRead += 3;
  pEmit->startScale = buf.GetFloat();
  pEmit->middleScale = buf.GetFloat();
  pEmit->endScale = buf.GetFloat();
  localBytesRead += 12;
  pEmit->lifespanUVAnimStart = buf.GetUint();
  pEmit->lifespanUVAnimEnd = buf.GetUint();
  pEmit->lifespanUVAnimRepeat = buf.GetUint();
  pEmit->decayUVAnimStart = buf.GetUint();
  pEmit->decayUVAnimEnd = buf.GetUint();
  pEmit->decayUVAnimRepeat = buf.GetUint();
  pEmit->tailUVAnimStart = buf.GetUint();
  pEmit->tailUVAnimEnd = buf.GetUint();
  pEmit->tailUVAnimRepeat = buf.GetUint();
  pEmit->tailDecayUVAnimStart = buf.GetUint();
  pEmit->tailDecayUVAnimEnd = buf.GetUint();
  pEmit->tailDecayUVAnimRepeat = buf.GetUint();
  localBytesRead += 48;
  pEmit->blendMode = (MDLPARTICLEEMITTER2::PARTICLE_BLEND_MODE)buf.GetUint();
  localBytesRead += 4;
  pEmit->textureId = buf.GetUint();
  localBytesRead += 4;
  pEmit->priorityPlane = buf.GetInt();
  localBytesRead += 4;
  pEmit->replaceableId = buf.GetUint();
  localBytesRead += 4;
  buf.GetTcharArray(pEmit->geometryMdl, 260);
  localBytesRead += 260;
  buf.GetTcharArray(pEmit->recursionMdl, 260);
  localBytesRead += 260;
  pEmit->twinkleFPS = buf.GetFloat();
  localBytesRead += 4;
  pEmit->twinkleOnOff = buf.GetFloat();
  localBytesRead += 4;
  pEmit->twinkleScaleMin = buf.GetFloat();
  localBytesRead += 4;
  pEmit->twinkleScaleMax = buf.GetFloat();
  localBytesRead += 4;
  pEmit->ivelScale = buf.GetFloat();
  localBytesRead += 4;
  pEmit->tumblexMin = buf.GetFloat();
  localBytesRead += 4;
  pEmit->tumblexMax = buf.GetFloat();
  localBytesRead += 4;
  pEmit->tumbleyMin = buf.GetFloat();
  localBytesRead += 4;
  pEmit->tumbleyMax = buf.GetFloat();
  localBytesRead += 4;
  pEmit->tumblezMin = buf.GetFloat();
  localBytesRead += 4;
  pEmit->tumblezMax = buf.GetFloat();
  localBytesRead += 4;
  pEmit->drag = buf.GetFloat();
  localBytesRead += 4;
  pEmit->spin = buf.GetFloat();
  localBytesRead += 4;
  pEmit->windVector.x = buf.GetFloat();
  localBytesRead += 4;
  pEmit->windVector.y = buf.GetFloat();
  localBytesRead += 4;
  pEmit->windVector.z = buf.GetFloat();
  localBytesRead += 4;
  pEmit->windTime = buf.GetFloat();
  localBytesRead += 4;
  pEmit->followSpeed1 = buf.GetFloat();
  localBytesRead += 4;
  pEmit->followScale1 = buf.GetFloat();
  localBytesRead += 4;
  pEmit->followSpeed2 = buf.GetFloat();
  localBytesRead += 4;
  pEmit->followScale2 = buf.GetFloat();
  localBytesRead += 4;
  UINT splineCount = buf.GetUint();
  localBytesRead += 4;
  if (splineCount) {
    pEmit->spline.SetCount(splineCount);
    buf.GetFloatArray(&pEmit->spline[0].x, 3 * splineCount);
    localBytesRead += 12 * splineCount;
  }
  pEmit->squirts = buf.GetUint();
  localBytesRead += 4;
  while (localBytesRead < sectionLength) {
    DWORD tag = buf.GetDword();
    localBytesRead += 4;
    switch (tag) {
      case 'E2PK':
        if (!ReadBinFloatKeyFrames(pEmit->emissionRate, buf, localBytesRead)) {
          status->Add(STATUS_ERROR, "Error reading emission rate portion of ParticleEmitter2.\n");
          return 0;
        }
        break;
      case 'G2PK':
        if (!ReadBinFloatKeyFrames(pEmit->gravity, buf, localBytesRead)) {
          status->Add(STATUS_ERROR, "Error reading gravity portion of ParticleEmitter2.\n");
          return 0;
        }
        break;
      case 'L2PK':
        if (!ReadBinFloatKeyFrames(pEmit->latitude, buf, localBytesRead)) {
          status->Add(STATUS_ERROR, "Error reading latitude portion of ParticleEmitter2.\n");
          return 0;
        }
        break;
      case 'NLPK':
        if (!ReadBinFloatKeyFrames(pEmit->longitude, buf, localBytesRead)) {
          status->Add(STATUS_ERROR, "Error reading longitude portion of ParticleEmitter2.\n");
          return 0;
        }
        break;
      case 'R2PK':
        if (!ReadBinFloatKeyFrames(pEmit->variation, buf, localBytesRead)) {
          status->Add(STATUS_ERROR, "Error reading variation portion of ParticleEmitter2.\n");
          return 0;
        }
        break;
      case 'N2PK':
        if (!ReadBinFloatKeyFrames(pEmit->length, buf, localBytesRead)) {
          status->Add(STATUS_ERROR, "Error reading length keys portion of ParticleEmitter2.\n");
          return 0;
        }
        break;
      case 'S2PK':
        if (!ReadBinFloatKeyFrames(pEmit->speed, buf, localBytesRead)) {
          status->Add(STATUS_ERROR, "Error reading speed portion of ParticleEmitter2.\n");
          return 0;
        }
        break;
      case 'SIVK':
        if (!ReadBinFloatKeyFrames(pEmit->visibilityKeys, buf, localBytesRead)) {
          status->Add(STATUS_ERROR, "Error reading visibility keys portion of ParticleEmitter2.\n");
          return 0;
        }
        break;
      case 'W2PK':
        if (!ReadBinFloatKeyFrames(pEmit->width, buf, localBytesRead)) {
          status->Add(STATUS_ERROR, "Error reading width keys portion of ParticleEmitter2.\n");
          return 0;
        }
        break;
      case 'Z2PK':
        if (!ReadBinFloatKeyFrames(pEmit->zsource, buf, localBytesRead)) {
          status->Add(STATUS_ERROR, "Error reading zsource keys portion of ParticleEmitter2.\n");
          return 0;
        }
        break;
      case 'FILK':
        if (!ReadBinFloatKeyFrames(pEmit->life, buf, localBytesRead)) {
          status->Add(STATUS_ERROR, "Error reading zsource keys portion of ParticleEmitter2.\n");
          return 0;
        }
        break;
      default:
        SkipUnknown(buf, localBytesRead);
        break;
    }
    if (localBytesRead > sectionLength) {
      status->FatalOverran("ParticleEmitters2", -1);
      return 0;
    }
  }
  totalRead += localBytesRead;
  return 1;
}
