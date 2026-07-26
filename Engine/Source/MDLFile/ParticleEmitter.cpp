#include "MDLTypes.h"
#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

#include <storm.h>
#include <stpl.h>

namespace MDL {
const char *__fastcall TokenText(unsigned int token);
void __cdecl WriteLine(TSGrowableArray<char> &buffer, const char *format, ...);
}

static void IAddParticleEmitterErrors(TSet &errors) {
  AddObjectErrors(errors);
  errors.Add(0x144, 1, 0);
  errors.Add(0x153, 1, 0);
  errors.Add(0x161, 1, 0);
  errors.Add(0x162, 1, 0);
  errors.Add(0x1D9, 0, 0);
  errors.Add(0x18C, 1, 0);
}

static void IAddParticleErrors(TSet &errors) {
  errors.Add(0x165, 1, 0);
  errors.Add(0x15D, 1, 0);
  errors.Add(0x1A0, 1, 0);
}

static void IReadParticleKeyFrames(
    Parser &parse,
    unsigned int savedToken,
    const char *tokenText,
    MDLPARTICLE *options
) {
  if (savedToken == 0x15D) {
    ReadObjectFloatKeyframes(parse, &options->speed);
  } else if (savedToken == 0x165) {
    ReadObjectFloatKeyframes(parse, &options->life);
  } else if (savedToken == 0x1A0) {
    SStrCopy(options->path, parse.ExpectString(), 260);
    parse.Expect(',');
  } else {
    parse.FatalUnexpected(tokenText);
    parse.Expect(',');
  }
}

static void IReadParticleStaticData(
    Parser &parse,
    unsigned int savedToken,
    const char *tokenText,
    MDLPARTICLE *options
) {
  if (savedToken == 0x15D) {
    ReadFloatKeyData(parse, &options->staticSpeed, 1);
  } else if (savedToken == 0x165) {
    ReadFloatKeyData(parse, &options->staticLife, 1);
  } else {
    parse.FatalUnexpected(tokenText);
  }
  parse.Expect(',');
}

static void IReadParticleOptions(Parser &parse, MDLPARTICLE *options) {
  TSet errors;
  IAddParticleErrors(errors);
  parse.Expect('{');
  const char *tokenText;
  unsigned int savedToken = parse.Token(&tokenText, 0);
  while (savedToken && savedToken != '}') {
    int expectAnimation =
        IExpectAnimation(parse, &savedToken, &tokenText);
    if (!errors.Check(savedToken)) {
      parse.FatalDuplicate(tokenText);
    }
    if (expectAnimation) {
      IReadParticleKeyFrames(
          parse, savedToken, tokenText, options
      );
    } else {
      IReadParticleStaticData(
          parse, savedToken, tokenText, options
      );
    }
    savedToken = parse.Token(&tokenText, 0);
  }
  parse.Expect('}', savedToken, tokenText);
}

static void IReadParticleEmitterKeyFrames(
    Parser &parse,
    unsigned int savedToken,
    const char *tokenText,
    MDLPARTICLEEMITTER *emitter
) {
  switch (savedToken) {
    case 0x144:
      ReadObjectFloatKeyframes(parse, &emitter->emissionRate);
      break;
    case 0x153:
      ReadObjectFloatKeyframes(parse, &emitter->gravity);
      break;
    case 0x161:
      ReadObjectFloatKeyframes(parse, &emitter->longitude);
      break;
    case 0x162:
      ReadObjectFloatKeyframes(parse, &emitter->latitude);
      break;
    case 0x18C:
      IReadParticleOptions(parse, &emitter->particle);
      break;
    case 0x1D9:
      ReadObjectFloatKeyframes(parse, &emitter->visibilityKeys);
      break;
    default:
      parse.FatalUnexpected(tokenText);
      break;
  }
}

static void IReadParticleEmitterStaticData(
    Parser &parse,
    unsigned int savedToken,
    const char *tokenText,
    MDLPARTICLEEMITTER *emitter
) {
  float *value = 0;
  switch (savedToken) {
    case 0x144: value = &emitter->staticEmissionRate; break;
    case 0x153: value = &emitter->staticGravity; break;
    case 0x161: value = &emitter->staticLongitude; break;
    case 0x162: value = &emitter->staticLatitude; break;
    default: parse.FatalUnexpected(tokenText); break;
  }
  if (value) {
    ReadFloatKeyData(parse, value, 1);
  }
  parse.Expect(',');
}

static int IReadParticleEmitterFlags(
    Parser &parse,
    unsigned int savedToken,
    MDLPARTICLEEMITTER *emitter
) {
  if (savedToken == 0x145) {
    emitter->flags |= 0x8000;
  } else if (savedToken == 0x146) {
    emitter->flags |= 0x10000;
  } else {
    return 0;
  }
  parse.Expect(',');
  return 1;
}

static void IReadParticleEmitter(
    Parser &parse,
    TSet &errors,
    MDLPARTICLEEMITTER *emitter,
    CMDLStatus *status
) {
  parse.Expect('{');
  const char *tokenText;
  unsigned int savedToken = parse.Token(&tokenText, 0);
  while (savedToken && savedToken != '}') {
    int expectAnimation =
        IExpectAnimation(parse, &savedToken, &tokenText);
    if (!errors.Check(savedToken)) {
      parse.FatalDuplicate(tokenText);
    }
    if (!ReadObjectBody(
            parse, savedToken, 0, emitter, status
        )
        && !IReadParticleEmitterFlags(
            parse, savedToken, emitter
        )) {
      if (expectAnimation) {
        IReadParticleEmitterKeyFrames(
            parse, savedToken, tokenText, emitter
        );
      } else {
        IReadParticleEmitterStaticData(
            parse, savedToken, tokenText, emitter
        );
      }
    }
    savedToken = parse.Token(&tokenText, 0);
  }
  parse.Expect('}', savedToken, tokenText);
}

namespace MDL {

int __fastcall ReadParticleEmitter(
    Parser &parse,
    MDLDATA &data,
    CMDLStatus *status
) {
  TSet errors;
  MDLPARTICLEEMITTER *emitter = data.particleEmitters.New();
  IAddParticleEmitterErrors(errors);
  ReadObjectName(parse, emitter->name);
  IReadParticleEmitter(parse, errors, emitter, status);
  ReadObjectEnd(
      errors,
      data,
      emitter,
      data.particleEmitters.Count() - 1,
      0x50000000
  );
  errors.Complete(status);
  return !parse.FoundError();
}

}

static void IWriteParticleOptions(
    const MDLPARTICLE &options,
    TSGrowableArray<char> &buffer
) {
  MDL::WriteLine(buffer, "\t%s {\n", MDL::TokenText(0x18C));
  if (options.life.keys.Count()) {
    WriteFloatKeyFrames(0x165, "\t\t", options.life, buffer);
  } else {
    MDL::WriteLine(
        buffer,
        "\t\t%s %s ",
        MDL::TokenText(0x1BB),
        MDL::TokenText(0x165)
    );
    WriteKeyData(buffer, &options.staticLife, 1);
  }
  if (options.speed.keys.Count()) {
    WriteFloatKeyFrames(0x15D, "\t\t", options.speed, buffer);
  } else {
    MDL::WriteLine(
        buffer,
        "\t\t%s %s ",
        MDL::TokenText(0x1BB),
        MDL::TokenText(0x15D)
    );
    WriteKeyData(buffer, &options.staticSpeed, 1);
  }
  if (SStrLen(options.path)) {
    MDL::WriteLine(
        buffer,
        "\t\t%s \"%s\",\n",
        MDL::TokenText(0x1A0),
        static_cast<const char *>(options.path)
    );
  }
  MDL::WriteLine(buffer, "\t}\n");
}

static void IWritePEFlags(
    const MDLPARTICLEEMITTER &emitter,
    TSGrowableArray<char> &buffer
) {
  MDL::WriteLine(
      buffer,
      "\t%s,\n",
      MDL::TokenText((emitter.flags & 0x10000) ? 0x146 : 0x145)
  );
}

static void IWriteParticleEmitter(
    const MDLDATA &data,
    const MDLPARTICLEEMITTER &emitter,
    int needObjIds,
    TSGrowableArray<char> &buffer
) {
  WriteObjectHeader(data, emitter, 0x112, needObjIds, buffer);
  IWritePEFlags(emitter, buffer);
  if (emitter.emissionRate.keys.Count()) {
    WriteFloatKeyFrames(
        0x144, "\t", emitter.emissionRate, buffer
    );
  } else {
    MDL::WriteLine(
        buffer,
        "\t%s %s ",
        MDL::TokenText(0x1BB),
        MDL::TokenText(0x144)
    );
    WriteKeyData(buffer, &emitter.staticEmissionRate, 1);
  }
  if (emitter.gravity.keys.Count()) {
    WriteFloatKeyFrames(0x153, "\t", emitter.gravity, buffer);
  } else {
    MDL::WriteLine(
        buffer,
        "\t%s %s ",
        MDL::TokenText(0x1BB),
        MDL::TokenText(0x153)
    );
    WriteKeyData(buffer, &emitter.staticGravity, 1);
  }
  if (emitter.latitude.keys.Count()) {
    WriteFloatKeyFrames(0x162, "\t", emitter.latitude, buffer);
  } else {
    MDL::WriteLine(
        buffer,
        "\t%s %s ",
        MDL::TokenText(0x1BB),
        MDL::TokenText(0x162)
    );
    WriteKeyData(buffer, &emitter.staticLatitude, 1);
  }
  if (emitter.longitude.keys.Count()) {
    WriteFloatKeyFrames(0x161, "\t", emitter.longitude, buffer);
  } else {
    MDL::WriteLine(
        buffer,
        "\t%s %s ",
        MDL::TokenText(0x1BB),
        MDL::TokenText(0x161)
    );
    WriteKeyData(buffer, &emitter.staticLongitude, 1);
  }
  WriteFloatKeyFrames(
      0x1D9, "\t", emitter.visibilityKeys, buffer
  );
  IWriteParticleOptions(emitter.particle, buffer);
  WriteObjectTrailer(emitter, buffer);
}

namespace MDL {

int __fastcall WriteParticleEmitters(
    const MDLDATA &data,
    TSGrowableArray<char> &buffer,
    CMDLStatus *
) {
  if (!static_cast<const char *>(data.model.animationFile)[0]) {
    int needObjIds =
        data.particleEmitters.Count() != data.objects.Count();
    for (unsigned int i = 0; i < data.particleEmitters.Count(); ++i) {
      IWriteParticleEmitter(
          data, data.particleEmitters[i], needObjIds, buffer
      );
    }
  }
  return 1;
}

}

static unsigned int GetBinParticleEmitterSize(
    const MDLPARTICLEEMITTER &section
) {
  unsigned int size = GetBinGenObjectSize(section) + 288;
  const MDLKEYTRACK<float> *tracks[7] = {
      &section.emissionRate,
      &section.gravity,
      &section.longitude,
      &section.latitude,
      &section.particle.life,
      &section.particle.speed,
      &section.visibilityKeys
  };
  for (unsigned int i = 0; i < 7; ++i) {
    if (tracks[i]->keys.Count()) {
      unsigned int values = tracks[i]->type > TRACK_LINEAR ? 3 : 1;
      size += 16
          + tracks[i]->keys.Count() * (4 + 4 * values);
    }
  }
  return size;
}

static void IWriteBinParticleEmitter(
    const MDLPARTICLEEMITTER &section,
    CMsgBuffer &buffer,
    CMDLStatus *status
) {
  buffer.AddUint(GetBinParticleEmitterSize(section));
  WriteBinGenObject(section, buffer, status);
  buffer.AddFloat(section.staticEmissionRate);
  buffer.AddFloat(section.staticGravity);
  buffer.AddFloat(section.staticLongitude);
  buffer.AddFloat(section.staticLatitude);
  buffer.AddTcharArray(section.particle.path, 260, 1);
  buffer.AddFloat(section.particle.staticLife);
  buffer.AddFloat(section.particle.staticSpeed);
  WriteBinFloatKeyFrames(section.emissionRate, 0x4545504B, buffer);
  WriteBinFloatKeyFrames(section.gravity, 0x4745504B, buffer);
  WriteBinFloatKeyFrames(section.longitude, 0x4E4C504B, buffer);
  WriteBinFloatKeyFrames(section.latitude, 0x544C504B, buffer);
  WriteBinFloatKeyFrames(section.particle.life, 0x4C45504B, buffer);
  WriteBinFloatKeyFrames(section.particle.speed, 0x5345504B, buffer);
  WriteBinFloatKeyFrames(section.visibilityKeys, 0x5349564B, buffer);
}

static int ReadBinParticleEmitter(
    CMsgBuffer &buffer,
    MDLPARTICLEEMITTER *emitter,
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
        "Error reading gen object portion of particle emitter.\n"
    );
    return 0;
  }
  emitter->staticEmissionRate = buffer.GetFloat();
  emitter->staticGravity = buffer.GetFloat();
  emitter->staticLongitude = buffer.GetFloat();
  emitter->staticLatitude = buffer.GetFloat();
  buffer.GetTcharArray(emitter->particle.path, 260);
  emitter->particle.staticLife = buffer.GetFloat();
  emitter->particle.staticSpeed = buffer.GetFloat();
  localBytesRead += 284;
  while (localBytesRead < sectionLength) {
    unsigned long tag = buffer.GetDword();
    localBytesRead += 4;
    int ok = 1;
    switch (tag) {
      case 0x4545504B:
        ok = ReadBinFloatKeyFrames(
            emitter->emissionRate, buffer, localBytesRead
        );
        break;
      case 0x4745504B:
        ok = ReadBinFloatKeyFrames(
            emitter->gravity, buffer, localBytesRead
        );
        break;
      case 0x4E4C504B:
        ok = ReadBinFloatKeyFrames(
            emitter->longitude, buffer, localBytesRead
        );
        break;
      case 0x544C504B:
        ok = ReadBinFloatKeyFrames(
            emitter->latitude, buffer, localBytesRead
        );
        break;
      case 0x4C45504B:
        ok = ReadBinFloatKeyFrames(
            emitter->particle.life, buffer, localBytesRead
        );
        break;
      case 0x5345504B:
        ok = ReadBinFloatKeyFrames(
            emitter->particle.speed, buffer, localBytesRead
        );
        break;
      case 0x5349564B:
        ok = ReadBinFloatKeyFrames(
            emitter->visibilityKeys, buffer, localBytesRead
        );
        break;
      default:
        SkipUnknown(buffer, localBytesRead);
        break;
    }
    if (!ok) {
      status->Add(
          STATUS_ERROR,
          "Error reading particle emitter key frames.\n"
      );
      return 0;
    }
    if (localBytesRead > sectionLength) {
      status->FatalOverran("ParticleEmitters", -1);
      return 0;
    }
  }
  totalRead += localBytesRead;
  return 1;
}

namespace MDL {

int __fastcall WriteBinParticleEmitters(
    const MDLDATA &data,
    CMsgBuffer &buffer,
    CMDLStatus *status
) {
  if (!static_cast<const char *>(data.model.animationFile)[0]
      && data.particleEmitters.Count()) {
    buffer.AddDword('MERP');
    unsigned int totalSize = 4;
    unsigned int i;
    for (i = 0; i < data.particleEmitters.Count(); ++i) {
      totalSize += GetBinParticleEmitterSize(
          data.particleEmitters[i]
      );
    }
    buffer.AddUint(totalSize);
    buffer.AddUint(data.particleEmitters.Count());
    for (i = 0; i < data.particleEmitters.Count(); ++i) {
      IWriteBinParticleEmitter(
          data.particleEmitters[i], buffer, status
      );
    }
  }
  return 1;
}

int __fastcall ReadBinParticleEmitters(
    CMsgBuffer &buffer,
    unsigned int length,
    MDLDATA &data,
    CMDLStatus *status
) {
  unsigned int totalRead = 4;
  unsigned int count = buffer.GetUint();
  data.particleEmitters.SetCount(0);
  data.particleEmitters.Reserve(count);
  while (totalRead < length) {
    MDLPARTICLEEMITTER *emitter = data.particleEmitters.New();
    if (!emitter) {
      status->FatalFlunked("ParticleEmitter", -1);
      return 0;
    }
    if (!ReadBinParticleEmitter(
            buffer, emitter, status, totalRead
        )) {
      status->Add(
          STATUS_ERROR, "Error reading particle emitter.\n"
      );
      return 0;
    }
    if (totalRead > length) {
      status->FatalOverran("ParticleEmitter Section", -1);
      return 0;
    }
    ReadBinObjectEnd(
        data,
        emitter,
        data.particleEmitters.Count() - 1,
        0x50000000
    );
  }
  return 1;
}

}
