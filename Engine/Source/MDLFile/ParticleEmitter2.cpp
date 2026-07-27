#include "MDLTypes.h"
#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

#include <storm.h>

namespace MDL {
const char *TokenText(unsigned int token);
void __cdecl WriteLine(TSGrowableArray<char> &buffer, const char *format, ...);
}

void ReadVertices(
    Parser &parse,
    const char *title,
    TSGrowableArray<NTempest::C3Vector> *vertices
);
void WriteVertices(
    const TSGrowableArray<NTempest::C3Vector> &vertices,
    unsigned int title,
    TSGrowableArray<char> &buffer
);

static void IAddParticleEmitter2Errors(TSet &errors) {
  AddObjectErrors(errors);
  errors.Add(0x144, 1, 0);
  errors.Add(0x153, 0, 0);
  errors.Add(0x161, 1, 0);
  errors.Add(0x1D9, 0, 0);
  errors.Add(0x1C3, 1, 0);
  errors.Add(0x1AE, 1, 0);
  errors.Add(0x137, 1, 0);
  errors.Add(0x1B0, 0, 0);
  errors.Add(0x1C6, 1, 0);
  errors.Add(0x198, 0, 0);
  errors.Add(0x11C, 0, 0);
  errors.Add(0x166, 0, 0);
  errors.Add(0x13D, 0, 0);
  errors.Add(0x1BF, 0, 0);
  errors.Add(0x1C0, 0, 0);
  errors.Add(0x1DA, 0, 0);
  errors.Add(0x158, 0, 0);
  errors.Add(0x169, 0, 0);
  errors.Add(0x1BE, 0, 0);
  errors.Add(0x1AA, 0, 0);
  errors.Add(0x1D1, 0, 0);
  errors.Add(0x171, 0, 0);
  errors.Add(0x192, 0, 0);
  errors.Add(0x193, 0, 0);
  errors.Add(0x194, 0, 0);
  errors.Add(0x196, 0, 0);
  errors.Add(0x199, 0, 0);
  errors.Add(0x19B, 0, 0);
  errors.Add(0x19C, 0, 0);
  errors.Add(0x197, 0, 0);
  errors.Add(0x14D, 0, 0);
  errors.Add(0x141, 0, 0);
  errors.Add(0x199, 0, 0);
  errors.Add(0x1DB, 0, 0);
  errors.Add(0x1B6, 0, 0);
  errors.Add(0x19E, 0, 0);
  errors.Add(0x195, 0, 0);
  errors.Add(0x18F, 0, 0);
  errors.Add(0x190, 0, 0);
}

static void IReadIntOption(
    Parser &parse,
    unsigned int &a,
    unsigned int &b,
    unsigned int &c
) {
  parse.Expect('{');
  a = parse.ExpectInt();
  parse.Expect(',');
  b = parse.ExpectInt();
  parse.Expect(',');
  c = parse.ExpectInt();
  parse.Expect('}');
}

static void IReadByteOption(
    Parser &parse,
    unsigned char &a,
    unsigned char &b,
    unsigned char &c
) {
  parse.Expect('{');
  a = static_cast<unsigned char>(parse.ExpectInt());
  parse.Expect(',');
  b = static_cast<unsigned char>(parse.ExpectInt());
  parse.Expect(',');
  c = static_cast<unsigned char>(parse.ExpectInt());
  parse.Expect('}');
}

static void IReadFloatOption(
    Parser &parse,
    float &a,
    float &b,
    float &c
) {
  parse.Expect('{');
  a = parse.ExpectFloat();
  parse.Expect(',');
  b = parse.ExpectFloat();
  parse.Expect(',');
  c = parse.ExpectFloat();
  parse.Expect('}');
}

static void IReadFloatOption(Parser &parse, float &a, float &b) {
  parse.Expect('{');
  a = parse.ExpectFloat();
  parse.Expect(',');
  b = parse.ExpectFloat();
  parse.Expect('}');
}

static void IReadFloatOption(
    Parser &parse,
    float &a,
    float &b,
    float &c,
    float &d,
    float &e,
    float &f
) {
  parse.Expect('{');
  a = parse.ExpectFloat();
  parse.Expect(',');
  b = parse.ExpectFloat();
  parse.Expect(',');
  c = parse.ExpectFloat();
  parse.Expect(',');
  d = parse.ExpectFloat();
  parse.Expect(',');
  e = parse.ExpectFloat();
  parse.Expect(',');
  f = parse.ExpectFloat();
  parse.Expect('}');
}

static void IReadParticleEmitter2Color(
    Parser &parse,
    MDLPARTICLEEMITTER2 *emitter
) {
  parse.Expect('{');
  parse.Expect(0x136);
  IReadFloatOption(
      parse,
      emitter->startColor.b,
      emitter->startColor.g,
      emitter->startColor.r
  );
  parse.Expect(',');
  parse.Expect(0x136);
  IReadFloatOption(
      parse,
      emitter->middleColor.b,
      emitter->middleColor.g,
      emitter->middleColor.r
  );
  parse.Expect(',');
  parse.Expect(0x136);
  IReadFloatOption(
      parse,
      emitter->endColor.b,
      emitter->endColor.g,
      emitter->endColor.r
  );
  parse.Expect(',');
  parse.Expect('}');
}

static void IReadSpline(
    Parser &parse,
    TSGrowableArray<NTempest::C3Vector> &spline
) {
  parse.Expect('{');
  parse.Expect(0x128);
  parse.Expect(0x1D8);
  ReadVertices(parse, MDL::TokenText(0x1D8), &spline);
  parse.Expect('}');
}

static int ReadParticleEmitter2BlendMode(
    Parser &parse,
    unsigned int token,
    MDLPARTICLEEMITTER2 *emitter
) {
  switch (token) {
    case 0x11A: emitter->blendMode = MDLPARTICLEEMITTER2::PBM_ADD; break;
    case 0x11D: emitter->blendMode = MDLPARTICLEEMITTER2::PBM_ALPHA_KEY; break;
    case 0x12E: emitter->blendMode = MDLPARTICLEEMITTER2::PBM_BLEND; break;
    case 0x172: emitter->blendMode = MDLPARTICLEEMITTER2::PBM_MODULATE; break;
    case 0x173: emitter->blendMode = MDLPARTICLEEMITTER2::PBM_MODULATE_2X; break;
    default: return 0;
  }
  parse.Expect(',');
  return 1;
}

static int IReadParticleEmitter2EmitterType(
    Parser &parse,
    unsigned int token,
    MDLPARTICLEEMITTER2 *emitter
) {
  if (token != 0x1D0) {
    return 0;
  }
  emitter->emitterType =
      static_cast<MDLPARTICLEEMITTER2::PARTICLE_EMITTER_TYPE>(
          parse.ExpectInt()
      );
  parse.Expect(',');
  return 1;
}

static int ReadParticleEmitter2Type(
    Parser &parse,
    unsigned int token,
    MDLPARTICLEEMITTER2 *emitter
) {
  switch (token) {
    case 0x133: emitter->type = MDLPARTICLEEMITTER2::PT_BOTH; break;
    case 0x157: emitter->type = MDLPARTICLEEMITTER2::PT_HEAD; break;
    case 0x1BC: emitter->type = MDLPARTICLEEMITTER2::PT_TAIL; break;
    default: return 0;
  }
  parse.Expect(',');
  return 1;
}

static int IReadParticleEmitter2Flags(
    Parser &parse,
    unsigned int token,
    MDLPARTICLEEMITTER2 *emitter
) {
  unsigned int flag = 0;
  switch (token) {
    case 0x169: flag = 0x00020000; break;
    case 0x171: flag = 0x00080000; break;
    case 0x18D: flag = 0x00400000; break;
    case 0x18E: flag = 0x04000000; break;
    case 0x18F: flag = 0x20000000; break;
    case 0x191: flag = 0x00100000; break;
    case 0x192: flag = 0x00200000; break;
    case 0x195: flag = 0x10000000; break;
    case 0x19A: flag = 0x01000000; break;
    case 0x19D: flag = 0x08000000; break;
    case 0x19F: flag = 0x00800000; break;
    case 0x1B7: flag = 0x00010000; break;
    case 0x1BD: flag = 0x02000000; break;
    case 0x1D1: flag = 0x00040000; break;
    case 0x1D3: flag = 0x00008000; break;
    default: return 0;
  }
  emitter->flags |= flag;
  parse.Expect(',');
  return 1;
}

static void IReadParticleEmitter2KeyFrames(
    Parser &parse,
    unsigned int token,
    const char *tokenText,
    MDLPARTICLEEMITTER2 *emitter
) {
  switch (token) {
    case 0x11C:
      IReadByteOption(
          parse,
          emitter->startAlpha,
          emitter->middleAlpha,
          emitter->endAlpha
      );
      parse.Expect(',');
      break;
    case 0x137:
      emitter->cols = parse.ExpectInt();
      parse.Expect(',');
      break;
    case 0x13D:
      IReadIntOption(
          parse,
          emitter->decayUVAnimStart,
          emitter->decayUVAnimEnd,
          emitter->decayUVAnimRepeat
      );
      parse.Expect(',');
      break;
    case 0x141:
      emitter->drag = parse.ExpectFloat();
      parse.Expect(',');
      break;
    case 0x144:
      ReadObjectFloatKeyframes(parse, &emitter->emissionRate);
      break;
    case 0x14D:
      emitter->twinkleFPS = parse.ExpectFloat();
      parse.Expect(',');
      break;
    case 0x153:
      ReadObjectFloatKeyframes(parse, &emitter->gravity);
      break;
    case 0x161:
      ReadObjectFloatKeyframes(parse, &emitter->latitude);
      break;
    case 0x162:
      ReadObjectFloatKeyframes(parse, &emitter->longitude);
      break;
    case 0x164:
      ReadObjectFloatKeyframes(parse, &emitter->length);
      break;
    case 0x165:
      ReadObjectFloatKeyframes(parse, &emitter->life);
      break;
    case 0x166:
      IReadIntOption(
          parse,
          emitter->lifespanUVAnimStart,
          emitter->lifespanUVAnimEnd,
          emitter->lifespanUVAnimRepeat
      );
      parse.Expect(',');
      break;
    case 0x190:
      parse.Expect('{');
      emitter->followSpeed1 = parse.ExpectFloat();
      parse.Expect(',');
      emitter->followScale1 = parse.ExpectFloat();
      parse.Expect(',');
      emitter->followSpeed2 = parse.ExpectFloat();
      parse.Expect(',');
      emitter->followScale2 = parse.ExpectFloat();
      parse.Expect('}');
      parse.Expect(',');
      break;
    case 0x193:
      emitter->ivelScale = parse.ExpectFloat();
      parse.Expect(',');
      break;
    case 0x194:
      SStrCopy(emitter->geometryMdl, parse.ExpectString(), 260);
      parse.Expect(',');
      break;
    case 0x196:
      SStrCopy(emitter->recursionMdl, parse.ExpectString(), 260);
      parse.Expect(',');
      break;
    case 0x197:
      emitter->spin = parse.ExpectFloat();
      parse.Expect(',');
      break;
    case 0x198:
      IReadFloatOption(
          parse,
          emitter->startScale,
          emitter->middleScale,
          emitter->endScale
      );
      parse.Expect(',');
      break;
    case 0x199:
      IReadFloatOption(
          parse,
          emitter->tumblexMin,
          emitter->tumblexMax,
          emitter->tumbleyMin,
          emitter->tumbleyMax,
          emitter->tumblezMin,
          emitter->tumblezMax
      );
      parse.Expect(',');
      break;
    case 0x19B:
      emitter->twinkleOnOff = parse.ExpectFloat();
      parse.Expect(',');
      break;
    case 0x19C:
      IReadFloatOption(
          parse,
          emitter->twinkleScaleMin,
          emitter->twinkleScaleMax
      );
      parse.Expect(',');
      break;
    case 0x19E:
      ReadObjectFloatKeyframes(parse, &emitter->zsource);
      break;
    case 0x1A6:
      emitter->priorityPlane = parse.ExpectInt();
      parse.Expect(',');
      break;
    case 0x1AA:
      emitter->replaceableId = parse.ExpectInt();
      parse.Expect(',');
      break;
    case 0x1AE:
      emitter->rows = parse.ExpectInt();
      parse.Expect(',');
      break;
    case 0x1B0:
      IReadParticleEmitter2Color(parse, emitter);
      parse.Expect(',');
      break;
    case 0x1B6:
      IReadSpline(parse, emitter->spline);
      break;
    case 0x1B9:
      ReadObjectFloatKeyframes(parse, &emitter->speed);
      break;
    case 0x1BA:
      emitter->squirts = 1;
      parse.Expect(',');
      break;
    case 0x1BE:
      emitter->tailLength = parse.ExpectFloat();
      parse.Expect(',');
      break;
    case 0x1BF:
      IReadIntOption(
          parse,
          emitter->tailDecayUVAnimStart,
          emitter->tailDecayUVAnimEnd,
          emitter->tailDecayUVAnimRepeat
      );
      parse.Expect(',');
      break;
    case 0x1C0:
      IReadIntOption(
          parse,
          emitter->tailUVAnimStart,
          emitter->tailUVAnimEnd,
          emitter->tailUVAnimRepeat
      );
      parse.Expect(',');
      break;
    case 0x1C3:
      emitter->textureId = parse.ExpectInt();
      parse.Expect(',');
      break;
    case 0x1C6:
      emitter->middleTime = parse.ExpectFloat();
      parse.Expect(',');
      break;
    case 0x1D4:
      ReadObjectFloatKeyframes(parse, &emitter->variation);
      break;
    case 0x1D9:
      ReadObjectFloatKeyframes(parse, &emitter->visibilityKeys);
      break;
    case 0x1DA:
      ReadObjectFloatKeyframes(parse, &emitter->width);
      break;
    case 0x1DB:
      parse.Expect('{');
      IReadFloatOption(
          parse,
          emitter->windVector.x,
          emitter->windVector.y,
          emitter->windVector.z
      );
      parse.Expect(',');
      emitter->windTime = parse.ExpectFloat();
      parse.Expect('}');
      parse.Expect(',');
      break;
    default:
      parse.FatalUnexpected(tokenText);
      break;
  }
}

static void IReadParticleEmitter2StaticData(
    Parser &parse,
    unsigned int token,
    const char *tokenText,
    MDLPARTICLEEMITTER2 *emitter
) {
  float *value = 0;
  switch (token) {
    case 0x144: value = &emitter->staticEmissionRate; break;
    case 0x153: value = &emitter->staticGravity; break;
    case 0x161: value = &emitter->staticLatitude; break;
    case 0x162: value = &emitter->staticLongitude; break;
    case 0x164: value = &emitter->staticLength; break;
    case 0x165: value = &emitter->staticLife; break;
    case 0x19E: value = &emitter->staticZsource; break;
    case 0x1B9: value = &emitter->staticSpeed; break;
    case 0x1D4: value = &emitter->staticVariation; break;
    case 0x1DA: value = &emitter->staticWidth; break;
    default: parse.FatalUnexpected(tokenText); break;
  }
  if (value) {
    ReadFloatKeyData(parse, value, 1);
  }
  parse.Expect(',');
}

static void IReadParticleEmitter2(
    Parser &parse,
    TSet &errors,
    MDLPARTICLEEMITTER2 *emitter,
    CMDLStatus *status
) {
  parse.Expect('{');
  const char *tokenText;
  unsigned int token = parse.Token(&tokenText, 0);
  while (token && token != '}') {
    int expectAnimation = IExpectAnimation(parse, &token, &tokenText);
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokenText);
    }
    if (!ReadObjectBody(parse, token, 0, emitter, status)
        && !IReadParticleEmitter2Flags(parse, token, emitter)
        && !IReadParticleEmitter2EmitterType(parse, token, emitter)
        && !ReadParticleEmitter2BlendMode(parse, token, emitter)
        && !ReadParticleEmitter2Type(parse, token, emitter)) {
      if (expectAnimation) {
        IReadParticleEmitter2KeyFrames(
            parse, token, tokenText, emitter
        );
      } else {
        IReadParticleEmitter2StaticData(
            parse, token, tokenText, emitter
        );
      }
    }
    token = parse.Token(&tokenText, 0);
  }
  parse.Expect('}', token, tokenText);
}

namespace MDL {

int ReadParticleEmitter2(
    Parser &parse,
    MDLDATA &data,
    CMDLStatus *status
) {
  TSet errors;
  MDLPARTICLEEMITTER2 *emitter = data.particleEmitters2.New();
  IAddParticleEmitter2Errors(errors);
  ReadObjectName(parse, emitter->name);
  IReadParticleEmitter2(parse, errors, emitter, status);
  ReadObjectEnd(
      errors,
      data,
      emitter,
      data.particleEmitters2.Count() - 1,
      0x70000000
  );
  errors.Complete(status);
  return !parse.FoundError();
}

}

static void IWriteParticleEmitter2BlendMode(
    const MDLPARTICLEEMITTER2 &emitter,
    TSGrowableArray<char> &buffer
) {
  unsigned int token = 0x12E;
  switch (emitter.blendMode) {
    case MDLPARTICLEEMITTER2::PBM_ADD: token = 0x11A; break;
    case MDLPARTICLEEMITTER2::PBM_MODULATE: token = 0x172; break;
    case MDLPARTICLEEMITTER2::PBM_MODULATE_2X: token = 0x173; break;
    case MDLPARTICLEEMITTER2::PBM_ALPHA_KEY: token = 0x11D; break;
    default: break;
  }
  MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(token));
}

static void IWriteParticleEmitter2Type(
    const MDLPARTICLEEMITTER2 &emitter,
    TSGrowableArray<char> &buffer
) {
  unsigned int token = 0x157;
  if (emitter.type == MDLPARTICLEEMITTER2::PT_TAIL) {
    token = 0x1BC;
  } else if (emitter.type == MDLPARTICLEEMITTER2::PT_BOTH) {
    token = 0x133;
  }
  MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(token));
}

static void IWriteParticleEmitter2Colors(
    const MDLPARTICLEEMITTER2 &emitter,
    TSGrowableArray<char> &buffer
) {
  MDL::WriteLine(buffer, "\t%s {\n", MDL::TokenText(0x1B0));
  MDL::WriteLine(buffer, "\t\t%s ", MDL::TokenText(0x136));
  WriteKeyData(buffer, &emitter.startColor.b, 3);
  MDL::WriteLine(buffer, "\t\t%s ", MDL::TokenText(0x136));
  WriteKeyData(buffer, &emitter.middleColor.b, 3);
  MDL::WriteLine(buffer, "\t\t%s ", MDL::TokenText(0x136));
  WriteKeyData(buffer, &emitter.endColor.b, 3);
  MDL::WriteLine(buffer, "\t},\n");
}

static void IWritePE2Flags(
    const MDLPARTICLEEMITTER2 &emitter,
    TSGrowableArray<char> &buffer
) {
  if (emitter.flags & 0x00010000)
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(0x1B7));
  if (emitter.flags & 0x00008000)
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(0x1D3));
  if (emitter.flags & 0x00020000)
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(0x169));
  if (emitter.flags & 0x00040000)
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(0x1D1));
  if (emitter.flags & 0x00080000)
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(0x171));
  if (emitter.flags & 0x00200000)
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(0x192));
  if (emitter.flags & 0x00100000)
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(0x191));
  if (emitter.flags & 0x00400000)
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(0x18D));
  if (emitter.flags & 0x00800000)
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(0x19F));
  if (emitter.flags & 0x01000000)
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(0x19A));
  if (emitter.flags & 0x02000000)
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(0x1BD));
  if (emitter.flags & 0x04000000)
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(0x18E));
  if (emitter.flags & 0x08000000)
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(0x19D));
  if (emitter.flags & 0x10000000)
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(0x195));
  if (emitter.flags & 0x20000000)
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(0x18F));
}

static void IWriteSpline(
    const TSGrowableArray<NTempest::C3Vector> &spline,
    TSGrowableArray<char> &buffer
) {
  MDL::WriteLine(buffer, "\t%s {\n", MDL::TokenText(0x1B6));
  MDL::WriteLine(buffer, "\t\t%s\n", MDL::TokenText(0x128));
  WriteVertices(spline, 0x1D8, buffer);
  MDL::WriteLine(buffer, "\t}\n");
}

static void IWriteParticleEmitter2(
    const MDLDATA &data,
    const MDLPARTICLEEMITTER2 &emitter,
    int needObjIds,
    TSGrowableArray<char> &buffer
) {
  WriteObjectHeader(data, emitter, 0x113, needObjIds, buffer);
  IWritePE2Flags(emitter, buffer);
  MDL::WriteLine(
      buffer, "\t%s %d,\n", MDL::TokenText(0x1D0), emitter.emitterType
  );

  if (emitter.speed.keys.Count()) {
    WriteFloatKeyFrames(0x1B9, "\t", emitter.speed, buffer);
  } else {
    MDL::WriteLine(
        buffer, "\t%s %s ",
        MDL::TokenText(0x1BB), MDL::TokenText(0x1B9)
    );
    WriteKeyData(buffer, &emitter.staticSpeed, 1);
  }
  if (emitter.variation.keys.Count()) {
    WriteFloatKeyFrames(0x1D4, "\t", emitter.variation, buffer);
  } else {
    MDL::WriteLine(
        buffer, "\t%s %s ",
        MDL::TokenText(0x1BB), MDL::TokenText(0x1D4)
    );
    WriteKeyData(buffer, &emitter.staticVariation, 1);
  }
  if (emitter.latitude.keys.Count()) {
    WriteFloatKeyFrames(0x161, "\t", emitter.latitude, buffer);
  } else {
    MDL::WriteLine(
        buffer, "\t%s %s ",
        MDL::TokenText(0x1BB), MDL::TokenText(0x161)
    );
    WriteKeyData(buffer, &emitter.staticLatitude, 1);
  }
  if (emitter.longitude.keys.Count()) {
    WriteFloatKeyFrames(0x162, "\t", emitter.longitude, buffer);
  } else {
    MDL::WriteLine(
        buffer, "\t%s %s ",
        MDL::TokenText(0x1BB), MDL::TokenText(0x162)
    );
    WriteKeyData(buffer, &emitter.staticLongitude, 1);
  }
  if (emitter.gravity.keys.Count()) {
    WriteFloatKeyFrames(0x153, "\t", emitter.gravity, buffer);
  } else {
    MDL::WriteLine(
        buffer, "\t%s %s ",
        MDL::TokenText(0x1BB), MDL::TokenText(0x153)
    );
    WriteKeyData(buffer, &emitter.staticGravity, 1);
  }
  WriteFloatKeyFrames(0x1D9, "\t", emitter.visibilityKeys, buffer);
  if (emitter.squirts) {
    MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(0x1BA));
  }
  if (emitter.life.keys.Count()) {
    WriteFloatKeyFrames(0x165, "\t", emitter.life, buffer);
  } else {
    MDL::WriteLine(
        buffer, "\t%s %s ",
        MDL::TokenText(0x1BB), MDL::TokenText(0x165)
    );
    WriteKeyData(buffer, &emitter.staticLife, 1);
  }
  if (emitter.emissionRate.keys.Count()) {
    WriteFloatKeyFrames(0x144, "\t", emitter.emissionRate, buffer);
  } else {
    MDL::WriteLine(
        buffer, "\t%s %s ",
        MDL::TokenText(0x1BB), MDL::TokenText(0x144)
    );
    WriteKeyData(buffer, &emitter.staticEmissionRate, 1);
  }
  if (emitter.width.keys.Count()) {
    WriteFloatKeyFrames(0x1DA, "\t", emitter.width, buffer);
  } else {
    MDL::WriteLine(
        buffer, "\t%s %s ",
        MDL::TokenText(0x1BB), MDL::TokenText(0x1DA)
    );
    WriteKeyData(buffer, &emitter.staticWidth, 1);
  }
  if (emitter.length.keys.Count()) {
    WriteFloatKeyFrames(0x164, "\t", emitter.length, buffer);
  } else {
    MDL::WriteLine(
        buffer, "\t%s %s ",
        MDL::TokenText(0x1BB), MDL::TokenText(0x164)
    );
    WriteKeyData(buffer, &emitter.staticLength, 1);
  }
  if (emitter.zsource.keys.Count()) {
    WriteFloatKeyFrames(0x19E, "\t", emitter.zsource, buffer);
  } else {
    MDL::WriteLine(
        buffer, "\t%s %s ",
        MDL::TokenText(0x1BB), MDL::TokenText(0x19E)
    );
    WriteKeyData(buffer, &emitter.staticZsource, 1);
  }

  IWriteParticleEmitter2BlendMode(emitter, buffer);
  MDL::WriteLine(buffer, "\t%s %u,\n", MDL::TokenText(0x1AE), emitter.rows);
  MDL::WriteLine(buffer, "\t%s %u,\n", MDL::TokenText(0x137), emitter.cols);
  IWriteParticleEmitter2Type(emitter, buffer);
  MDL::WriteLine(
      buffer, "\t%s %g,\n", MDL::TokenText(0x1BE), emitter.tailLength
  );
  MDL::WriteLine(
      buffer, "\t%s %g,\n", MDL::TokenText(0x1C6), emitter.middleTime
  );
  IWriteParticleEmitter2Colors(emitter, buffer);
  MDL::WriteLine(
      buffer,
      "\t%s {%u, %u, %u},\n",
      MDL::TokenText(0x11C),
      emitter.startAlpha,
      emitter.middleAlpha,
      emitter.endAlpha
  );
  MDL::WriteLine(
      buffer,
      "\t%s {%g, %g, %g},\n",
      MDL::TokenText(0x198),
      emitter.startScale,
      emitter.middleScale,
      emitter.endScale
  );
  MDL::WriteLine(
      buffer, "\t%s {%u, %u, %u},\n", MDL::TokenText(0x166),
      emitter.lifespanUVAnimStart, emitter.lifespanUVAnimEnd,
      emitter.lifespanUVAnimRepeat
  );
  MDL::WriteLine(
      buffer, "\t%s {%u, %u, %u},\n", MDL::TokenText(0x13D),
      emitter.decayUVAnimStart, emitter.decayUVAnimEnd,
      emitter.decayUVAnimRepeat
  );
  MDL::WriteLine(
      buffer, "\t%s {%u, %u, %u},\n", MDL::TokenText(0x1C0),
      emitter.tailUVAnimStart, emitter.tailUVAnimEnd,
      emitter.tailUVAnimRepeat
  );
  MDL::WriteLine(
      buffer, "\t%s {%u, %u, %u},\n", MDL::TokenText(0x1BF),
      emitter.tailDecayUVAnimStart, emitter.tailDecayUVAnimEnd,
      emitter.tailDecayUVAnimRepeat
  );
  MDL::WriteLine(
      buffer, "\t%s %u,\n", MDL::TokenText(0x1C3), emitter.textureId
  );
  if (emitter.priorityPlane) {
    MDL::WriteLine(
        buffer, "\t%s %d,\n", MDL::TokenText(0x1A6), emitter.priorityPlane
    );
  }
  if (emitter.replaceableId) {
    MDL::WriteLine(
        buffer, "\t%s %d,\n", MDL::TokenText(0x1AA),
        emitter.replaceableId
    );
  }
  if (SStrLen(emitter.geometryMdl)) {
    MDL::WriteLine(
        buffer, "\t%s \"%s\",\n", MDL::TokenText(0x194),
        static_cast<const char *>(emitter.geometryMdl)
    );
  }
  if (SStrLen(emitter.recursionMdl)) {
    MDL::WriteLine(
        buffer, "\t%s \"%s\",\n", MDL::TokenText(0x196),
        static_cast<const char *>(emitter.recursionMdl)
    );
  }
  MDL::WriteLine(
      buffer, "\t%s %f,\n", MDL::TokenText(0x14D), emitter.twinkleFPS
  );
  MDL::WriteLine(
      buffer, "\t%s %f,\n", MDL::TokenText(0x19B), emitter.twinkleOnOff
  );
  MDL::WriteLine(
      buffer, "\t%s {%f, %f},\n", MDL::TokenText(0x19C),
      emitter.twinkleScaleMin, emitter.twinkleScaleMax
  );
  MDL::WriteLine(
      buffer, "\t%s %f,\n", MDL::TokenText(0x193), emitter.ivelScale
  );
  MDL::WriteLine(
      buffer,
      "\t%s {%f, %f, %f, %f, %f, %f},\n",
      MDL::TokenText(0x199),
      emitter.tumblexMin,
      emitter.tumblexMax,
      emitter.tumbleyMin,
      emitter.tumbleyMax,
      emitter.tumblezMin,
      emitter.tumblezMax
  );
  MDL::WriteLine(
      buffer, "\t%s %f,\n", MDL::TokenText(0x141), emitter.drag
  );
  MDL::WriteLine(
      buffer, "\t%s %f,\n", MDL::TokenText(0x197), emitter.spin
  );
  MDL::WriteLine(
      buffer,
      "\t%s { { %f, %f, %f }, %f },\n",
      MDL::TokenText(0x1DB),
      emitter.windVector.x,
      emitter.windVector.y,
      emitter.windVector.z,
      emitter.windTime
  );
  MDL::WriteLine(
      buffer,
      "\t%s { %f, %f, %f, %f },\n",
      MDL::TokenText(0x190),
      emitter.followSpeed1,
      emitter.followScale1,
      emitter.followSpeed2,
      emitter.followScale2
  );
  if (emitter.emitterType == MDLPARTICLEEMITTER2::PET_SPLINE) {
    IWriteSpline(emitter.spline, buffer);
  }
  WriteObjectTrailer(emitter, buffer);
}

namespace MDL {

int WriteParticleEmitters2(
    const MDLDATA &data,
    TSGrowableArray<char> &buffer,
    CMDLStatus *
) {
  if (!static_cast<const char *>(data.model.animationFile)[0]) {
    int needObjIds =
        data.particleEmitters2.Count() != data.objects.Count();
    for (unsigned int i = 0; i < data.particleEmitters2.Count(); ++i) {
      IWriteParticleEmitter2(
          data, data.particleEmitters2[i], needObjIds, buffer
      );
    }
  }
  return 1;
}

}

static unsigned int GetNonAnimEmitterDataSize(
    const MDLPARTICLEEMITTER2 &section
) {
  return 791 + 12 * section.spline.Count();
}

static unsigned int GetBinParticleEmitter2Size(
    const MDLPARTICLEEMITTER2 &section
) {
  unsigned int size =
      GetBinGenObjectSize(section) + 4 + GetNonAnimEmitterDataSize(section);
  const MDLKEYTRACK<float> *tracks[] = {
      &section.speed, &section.variation, &section.latitude,
      &section.longitude, &section.gravity, &section.life,
      &section.emissionRate, &section.width, &section.length,
      &section.zsource, &section.visibilityKeys
  };
  for (unsigned int i = 0; i < 11; ++i) {
    if (tracks[i]->keys.Count()) {
      unsigned int values = tracks[i]->type > TRACK_LINEAR ? 3 : 1;
      size += 16 + tracks[i]->keys.Count() * (4 + 4 * values);
    }
  }
  return size;
}

static void IWriteBinParticleEmitter2(
    const MDLPARTICLEEMITTER2 &section,
    CMsgBuffer &buffer,
    CMDLStatus *status
) {
  buffer.AddUint(GetBinParticleEmitter2Size(section));
  WriteBinGenObject(section, buffer, status);
  buffer.AddUint(GetNonAnimEmitterDataSize(section));
  buffer.AddUint(section.emitterType);
  buffer.AddFloat(section.staticSpeed);
  buffer.AddFloat(section.staticVariation);
  buffer.AddFloat(section.staticLatitude);
  buffer.AddFloat(section.staticLongitude);
  buffer.AddFloat(section.staticGravity);
  buffer.AddFloat(section.staticZsource);
  buffer.AddFloat(section.staticLife);
  buffer.AddFloat(section.staticEmissionRate);
  buffer.AddFloat(section.staticLength);
  buffer.AddFloat(section.staticWidth);
  buffer.AddUint(section.rows);
  buffer.AddUint(section.cols);
  buffer.AddUint(section.type);
  buffer.AddFloat(section.tailLength);
  buffer.AddFloat(section.middleTime);
  buffer.AddFloat(section.startColor.r);
  buffer.AddFloat(section.startColor.g);
  buffer.AddFloat(section.startColor.b);
  buffer.AddFloat(section.middleColor.r);
  buffer.AddFloat(section.middleColor.g);
  buffer.AddFloat(section.middleColor.b);
  buffer.AddFloat(section.endColor.r);
  buffer.AddFloat(section.endColor.g);
  buffer.AddFloat(section.endColor.b);
  buffer.AddByte(section.startAlpha);
  buffer.AddByte(section.middleAlpha);
  buffer.AddByte(section.endAlpha);
  buffer.AddFloat(section.startScale);
  buffer.AddFloat(section.middleScale);
  buffer.AddFloat(section.endScale);
  buffer.AddUint(section.lifespanUVAnimStart);
  buffer.AddUint(section.lifespanUVAnimEnd);
  buffer.AddUint(section.lifespanUVAnimRepeat);
  buffer.AddUint(section.decayUVAnimStart);
  buffer.AddUint(section.decayUVAnimEnd);
  buffer.AddUint(section.decayUVAnimRepeat);
  buffer.AddUint(section.tailUVAnimStart);
  buffer.AddUint(section.tailUVAnimEnd);
  buffer.AddUint(section.tailUVAnimRepeat);
  buffer.AddUint(section.tailDecayUVAnimStart);
  buffer.AddUint(section.tailDecayUVAnimEnd);
  buffer.AddUint(section.tailDecayUVAnimRepeat);
  buffer.AddUint(section.blendMode);
  buffer.AddUint(section.textureId);
  buffer.AddInt(section.priorityPlane);
  buffer.AddUint(section.replaceableId);
  buffer.AddTcharArray(section.geometryMdl, 260, 1);
  buffer.AddTcharArray(section.recursionMdl, 260, 1);
  buffer.AddFloat(section.twinkleFPS);
  buffer.AddFloat(section.twinkleOnOff);
  buffer.AddFloat(section.twinkleScaleMin);
  buffer.AddFloat(section.twinkleScaleMax);
  buffer.AddFloat(section.ivelScale);
  buffer.AddFloat(section.tumblexMin);
  buffer.AddFloat(section.tumblexMax);
  buffer.AddFloat(section.tumbleyMin);
  buffer.AddFloat(section.tumbleyMax);
  buffer.AddFloat(section.tumblezMin);
  buffer.AddFloat(section.tumblezMax);
  buffer.AddFloat(section.drag);
  buffer.AddFloat(section.spin);
  buffer.AddFloat(section.windVector.x);
  buffer.AddFloat(section.windVector.y);
  buffer.AddFloat(section.windVector.z);
  buffer.AddFloat(section.windTime);
  buffer.AddFloat(section.followSpeed1);
  buffer.AddFloat(section.followScale1);
  buffer.AddFloat(section.followSpeed2);
  buffer.AddFloat(section.followScale2);
  buffer.AddUint(section.spline.Count());
  if (section.spline.Count()) {
    buffer.AddFloatArray(
        &section.spline[0].x, 3 * section.spline.Count()
    );
  }
  buffer.AddUint(section.squirts);
  WriteBinFloatKeyFrames(section.emissionRate, 0x4532504B, buffer);
  WriteBinFloatKeyFrames(section.gravity, 0x4732504B, buffer);
  WriteBinFloatKeyFrames(section.longitude, 0x4E4C504B, buffer);
  WriteBinFloatKeyFrames(section.latitude, 0x4C32504B, buffer);
  WriteBinFloatKeyFrames(section.speed, 0x5332504B, buffer);
  WriteBinFloatKeyFrames(section.variation, 0x5232504B, buffer);
  WriteBinFloatKeyFrames(section.length, 0x4E32504B, buffer);
  WriteBinFloatKeyFrames(section.width, 0x5732504B, buffer);
  WriteBinFloatKeyFrames(section.zsource, 0x5A32504B, buffer);
  WriteBinFloatKeyFrames(section.visibilityKeys, 0x5349564B, buffer);
  WriteBinFloatKeyFrames(section.life, 0x46494C4B, buffer);
}

static int ReadBinParticleEmitter2(
    CMsgBuffer &buffer,
    MDLPARTICLEEMITTER2 *emitter,
    CMDLStatus *status,
    unsigned int &totalRead
) {
  unsigned int sectionLength = buffer.GetUint();
  unsigned int localBytesRead = 4;
  if (!ReadBinGenObject(
          *emitter, buffer, status, localBytesRead
      )) {
    status->Add(
        STATUS_ERROR,
        "Error reading gen object portion of ParticleEmitter2.\n"
    );
    return 0;
  }
  buffer.GetUint();
  localBytesRead += 4;
  emitter->emitterType =
      static_cast<MDLPARTICLEEMITTER2::PARTICLE_EMITTER_TYPE>(
          buffer.GetUint()
      );
  emitter->staticSpeed = buffer.GetFloat();
  emitter->staticVariation = buffer.GetFloat();
  emitter->staticLatitude = buffer.GetFloat();
  emitter->staticLongitude = buffer.GetFloat();
  emitter->staticGravity = buffer.GetFloat();
  emitter->staticZsource = buffer.GetFloat();
  emitter->staticLife = buffer.GetFloat();
  emitter->staticEmissionRate = buffer.GetFloat();
  emitter->staticLength = buffer.GetFloat();
  emitter->staticWidth = buffer.GetFloat();
  localBytesRead += 44;
  emitter->rows = buffer.GetUint();
  emitter->cols = buffer.GetUint();
  emitter->type =
      static_cast<MDLPARTICLEEMITTER2::PARTICLE_TYPE>(buffer.GetUint());
  emitter->tailLength = buffer.GetFloat();
  emitter->middleTime = buffer.GetFloat();
  localBytesRead += 20;
  emitter->startColor.r = buffer.GetFloat();
  emitter->startColor.g = buffer.GetFloat();
  emitter->startColor.b = buffer.GetFloat();
  emitter->middleColor.r = buffer.GetFloat();
  emitter->middleColor.g = buffer.GetFloat();
  emitter->middleColor.b = buffer.GetFloat();
  emitter->endColor.r = buffer.GetFloat();
  emitter->endColor.g = buffer.GetFloat();
  emitter->endColor.b = buffer.GetFloat();
  localBytesRead += 36;
  emitter->startAlpha = buffer.GetByte();
  emitter->middleAlpha = buffer.GetByte();
  emitter->endAlpha = buffer.GetByte();
  localBytesRead += 3;
  emitter->startScale = buffer.GetFloat();
  emitter->middleScale = buffer.GetFloat();
  emitter->endScale = buffer.GetFloat();
  localBytesRead += 12;
  emitter->lifespanUVAnimStart = buffer.GetUint();
  emitter->lifespanUVAnimEnd = buffer.GetUint();
  emitter->lifespanUVAnimRepeat = buffer.GetUint();
  emitter->decayUVAnimStart = buffer.GetUint();
  emitter->decayUVAnimEnd = buffer.GetUint();
  emitter->decayUVAnimRepeat = buffer.GetUint();
  emitter->tailUVAnimStart = buffer.GetUint();
  emitter->tailUVAnimEnd = buffer.GetUint();
  emitter->tailUVAnimRepeat = buffer.GetUint();
  emitter->tailDecayUVAnimStart = buffer.GetUint();
  emitter->tailDecayUVAnimEnd = buffer.GetUint();
  emitter->tailDecayUVAnimRepeat = buffer.GetUint();
  localBytesRead += 48;
  emitter->blendMode =
      static_cast<MDLPARTICLEEMITTER2::PARTICLE_BLEND_MODE>(
          buffer.GetUint()
      );
  emitter->textureId = buffer.GetUint();
  emitter->priorityPlane = buffer.GetInt();
  emitter->replaceableId = buffer.GetUint();
  localBytesRead += 16;
  buffer.GetTcharArray(emitter->geometryMdl, 260);
  buffer.GetTcharArray(emitter->recursionMdl, 260);
  localBytesRead += 520;
  emitter->twinkleFPS = buffer.GetFloat();
  emitter->twinkleOnOff = buffer.GetFloat();
  emitter->twinkleScaleMin = buffer.GetFloat();
  emitter->twinkleScaleMax = buffer.GetFloat();
  emitter->ivelScale = buffer.GetFloat();
  emitter->tumblexMin = buffer.GetFloat();
  emitter->tumblexMax = buffer.GetFloat();
  emitter->tumbleyMin = buffer.GetFloat();
  emitter->tumbleyMax = buffer.GetFloat();
  emitter->tumblezMin = buffer.GetFloat();
  emitter->tumblezMax = buffer.GetFloat();
  emitter->drag = buffer.GetFloat();
  emitter->spin = buffer.GetFloat();
  emitter->windVector.x = buffer.GetFloat();
  emitter->windVector.y = buffer.GetFloat();
  emitter->windVector.z = buffer.GetFloat();
  emitter->windTime = buffer.GetFloat();
  emitter->followSpeed1 = buffer.GetFloat();
  emitter->followScale1 = buffer.GetFloat();
  emitter->followSpeed2 = buffer.GetFloat();
  emitter->followScale2 = buffer.GetFloat();
  localBytesRead += 84;
  unsigned int splineCount = buffer.GetUint();
  localBytesRead += 4;
  emitter->spline.SetCount(splineCount);
  if (splineCount) {
    buffer.GetFloatArray(&emitter->spline[0].x, 3 * splineCount);
    localBytesRead += 12 * splineCount;
  }
  emitter->squirts = buffer.GetUint();
  localBytesRead += 4;
  while (localBytesRead < sectionLength) {
    unsigned long tag = buffer.GetDword();
    localBytesRead += 4;
    int ok = 1;
    switch (tag) {
      case 0x4532504B:
        ok = ReadBinFloatKeyFrames(
            emitter->emissionRate, buffer, localBytesRead
        );
        break;
      case 0x4732504B:
        ok = ReadBinFloatKeyFrames(
            emitter->gravity, buffer, localBytesRead
        );
        break;
      case 0x4E4C504B:
        ok = ReadBinFloatKeyFrames(
            emitter->longitude, buffer, localBytesRead
        );
        break;
      case 0x4C32504B:
        ok = ReadBinFloatKeyFrames(
            emitter->latitude, buffer, localBytesRead
        );
        break;
      case 0x5332504B:
        ok = ReadBinFloatKeyFrames(
            emitter->speed, buffer, localBytesRead
        );
        break;
      case 0x5232504B:
        ok = ReadBinFloatKeyFrames(
            emitter->variation, buffer, localBytesRead
        );
        break;
      case 0x4E32504B:
        ok = ReadBinFloatKeyFrames(
            emitter->length, buffer, localBytesRead
        );
        break;
      case 0x5732504B:
        ok = ReadBinFloatKeyFrames(
            emitter->width, buffer, localBytesRead
        );
        break;
      case 0x5A32504B:
        ok = ReadBinFloatKeyFrames(
            emitter->zsource, buffer, localBytesRead
        );
        break;
      case 0x5349564B:
        ok = ReadBinFloatKeyFrames(
            emitter->visibilityKeys, buffer, localBytesRead
        );
        break;
      case 0x46494C4B:
        ok = ReadBinFloatKeyFrames(
            emitter->life, buffer, localBytesRead
        );
        break;
      default:
        SkipUnknown(buffer, localBytesRead);
        break;
    }
    if (!ok) {
      status->Add(
          STATUS_ERROR,
          "Error reading ParticleEmitter2 key frames.\n"
      );
      return 0;
    }
    if (localBytesRead > sectionLength) {
      status->FatalOverran("ParticleEmitters2", -1);
      return 0;
    }
  }
  totalRead += localBytesRead;
  return 1;
}

namespace MDL {

int WriteBinParticleEmitters2(
    const MDLDATA &data,
    CMsgBuffer &buffer,
    CMDLStatus *status
) {
  if (!static_cast<const char *>(data.model.animationFile)[0]
      && data.particleEmitters2.Count()) {
    buffer.AddDword('2ERP');
    unsigned int totalSize = 4;
    unsigned int i;
    for (i = 0; i < data.particleEmitters2.Count(); ++i) {
      totalSize += GetBinParticleEmitter2Size(data.particleEmitters2[i]);
    }
    buffer.AddUint(totalSize);
    buffer.AddUint(data.particleEmitters2.Count());
    for (i = 0; i < data.particleEmitters2.Count(); ++i) {
      IWriteBinParticleEmitter2(data.particleEmitters2[i], buffer, status);
    }
  }
  return 1;
}

int ReadBinParticleEmitters2(
    CMsgBuffer &buffer,
    unsigned int length,
    MDLDATA &data,
    CMDLStatus *status
) {
  unsigned int totalRead = 4;
  unsigned int count = buffer.GetUint();
  data.particleEmitters2.SetCount(0);
  data.particleEmitters2.ReserveSpace(count);
  while (totalRead < length) {
    MDLPARTICLEEMITTER2 *emitter = data.particleEmitters2.New();
    if (!emitter) {
      status->FatalFlunked("ParticleEmitter2", -1);
      return 0;
    }
    if (!ReadBinParticleEmitter2(
            buffer, emitter, status, totalRead
        )) {
      status->Add(STATUS_ERROR, "Error reading ParticleEmitter2.\n");
      return 0;
    }
    if (totalRead > length) {
      status->FatalOverran("ParticleEmitter2 Section", -1);
      return 0;
    }
    ReadBinObjectEnd(
        data,
        emitter,
        data.particleEmitters2.Count() - 1,
        0x70000000
    );
  }
  return 1;
}

}
