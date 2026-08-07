#include "MDLTypes.h"
#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

#include <storm.h>
#include <stpl.h>

namespace MDL {
  LPCSTR       TokenText(UINT token);
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);
}  // namespace MDL

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

static void IReadParticleKeyFrames(Parser &parse, UINT savedtoken, LPCSTR tokenText, MDLPARTICLE *options) {
  if (savedtoken == 0x15D) {
    ReadObjectFloatKeyframes(parse, &options->speed);
  } else if (savedtoken == 0x165) {
    ReadObjectFloatKeyframes(parse, &options->life);
  } else if (savedtoken == 0x1A0) {
    SStrCopy(options->path, parse.ExpectString(), 260);
    parse.Expect(',');
  } else {
    parse.FatalUnexpected(tokenText);
    parse.Expect(',');
  }
}

static void IReadParticleStaticData(Parser &parse, UINT savedtoken, LPCSTR tokenText, MDLPARTICLE *options) {
  if (savedtoken == 0x15D) {
    ReadFloatKeyData(parse, &options->staticSpeed, 1);
  } else if (savedtoken == 0x165) {
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
  LPCSTR tokentext;
  UINT   savedtoken = parse.Token(&tokentext, 0);
  while (savedtoken && savedtoken != '}') {
    int expectAnimation = IExpectAnimation(parse, &savedtoken, &tokentext);
    if (!errors.Check(savedtoken)) {
      parse.FatalDuplicate(tokentext);
    }
    if (expectAnimation) {
      IReadParticleKeyFrames(parse, savedtoken, tokentext, options);
    } else {
      IReadParticleStaticData(parse, savedtoken, tokentext, options);
    }
    savedtoken = parse.Token(&tokentext, 0);
  }
  parse.Expect('}', savedtoken, tokentext);
}

static void IReadParticleEmitterKeyFrames(Parser &parse, UINT savedtoken, LPCSTR tokenText, MDLPARTICLEEMITTER *emitter) {
  switch (savedtoken) {
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

static void IReadParticleEmitterStaticData(Parser &parse, UINT savedtoken, LPCSTR tokenText, MDLPARTICLEEMITTER *emitter) {
  float *value = 0;
  switch (savedtoken) {
    case 0x144:
      value = &emitter->staticEmissionRate;
      break;
    case 0x153:
      value = &emitter->staticGravity;
      break;
    case 0x161:
      value = &emitter->staticLongitude;
      break;
    case 0x162:
      value = &emitter->staticLatitude;
      break;
    default:
      parse.FatalUnexpected(tokenText);
      break;
  }
  if (value) {
    ReadFloatKeyData(parse, value, 1);
  }
  parse.Expect(',');
}

static BOOL IReadParticleEmitterFlags(Parser &parse, UINT savedtoken, MDLPARTICLEEMITTER *emitter) {
  if (savedtoken == 0x145) {
    emitter->flags |= 0x8000;
  } else if (savedtoken == 0x146) {
    emitter->flags |= 0x10000;
  } else {
    return 0;
  }
  parse.Expect(',');
  return 1;
}

static void IReadParticleEmitter(Parser &parse, TSet &errors, MDLPARTICLEEMITTER *emitter, CMDLStatus *status) {
  parse.Expect('{');
  LPCSTR tokentext;
  UINT   savedtoken = parse.Token(&tokentext, 0);
  while (savedtoken && savedtoken != '}') {
    int expectAnimation = IExpectAnimation(parse, &savedtoken, &tokentext);
    if (!errors.Check(savedtoken)) {
      parse.FatalDuplicate(tokentext);
    }
    if (!ReadObjectBody(parse, savedtoken, 0, emitter, status) && !IReadParticleEmitterFlags(parse, savedtoken, emitter)) {
      if (expectAnimation) {
        IReadParticleEmitterKeyFrames(parse, savedtoken, tokentext, emitter);
      } else {
        IReadParticleEmitterStaticData(parse, savedtoken, tokentext, emitter);
      }
    }
    savedtoken = parse.Token(&tokentext, 0);
  }
  parse.Expect('}', savedtoken, tokentext);
}

namespace MDL {

  BOOL ReadParticleEmitter(Parser &parse, MDLDATA &data, CMDLStatus *status) {
    TSet                errors;
    MDLPARTICLEEMITTER *emitter = data.particleEmitters.New();
    IAddParticleEmitterErrors(errors);
    ReadObjectName(parse, emitter->name);
    IReadParticleEmitter(parse, errors, emitter, status);
    ReadObjectEnd(errors, data, emitter, data.particleEmitters.Count() - 1, 0x50000000);
    errors.Complete(status);
    return !parse.FoundError();
  }

}  // namespace MDL

static void IWriteParticleOptions(const MDLPARTICLE &options, TSGrowableArray<char> &buffer) {
  MDL::WriteLine(buffer, "\t%s {\n", MDL::TokenText(0x18C));
  if (options.life.keys.Count()) {
    WriteFloatKeyFrames(0x165, "\t\t", options.life, buffer);
  } else {
    MDL::WriteLine(buffer, "\t\t%s %s ", MDL::TokenText(0x1BB), MDL::TokenText(0x165));
    WriteKeyData(buffer, &options.staticLife, 1);
  }
  if (options.speed.keys.Count()) {
    WriteFloatKeyFrames(0x15D, "\t\t", options.speed, buffer);
  } else {
    MDL::WriteLine(buffer, "\t\t%s %s ", MDL::TokenText(0x1BB), MDL::TokenText(0x15D));
    WriteKeyData(buffer, &options.staticSpeed, 1);
  }
  if (SStrLen(options.path)) {
    MDL::WriteLine(buffer, "\t\t%s \"%s\",\n", MDL::TokenText(0x1A0), static_cast<LPCSTR>(options.path));
  }
  MDL::WriteLine(buffer, "\t}\n");
}

static void IWritePEFlags(const MDLPARTICLEEMITTER &emitter, TSGrowableArray<char> &buffer) {
  MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText((emitter.flags & 0x10000) ? 0x146 : 0x145));
}

static void IWriteParticleEmitter(const MDLDATA &data, const MDLPARTICLEEMITTER &emitter, int needObjIds, TSGrowableArray<char> &buffer) {
  WriteObjectHeader(data, emitter, 0x112, needObjIds, buffer);
  IWritePEFlags(emitter, buffer);
  if (emitter.emissionRate.keys.Count()) {
    WriteFloatKeyFrames(0x144, "\t", emitter.emissionRate, buffer);
  } else {
    MDL::WriteLine(buffer, "\t%s %s ", MDL::TokenText(0x1BB), MDL::TokenText(0x144));
    WriteKeyData(buffer, &emitter.staticEmissionRate, 1);
  }
  if (emitter.gravity.keys.Count()) {
    WriteFloatKeyFrames(0x153, "\t", emitter.gravity, buffer);
  } else {
    MDL::WriteLine(buffer, "\t%s %s ", MDL::TokenText(0x1BB), MDL::TokenText(0x153));
    WriteKeyData(buffer, &emitter.staticGravity, 1);
  }
  if (emitter.latitude.keys.Count()) {
    WriteFloatKeyFrames(0x162, "\t", emitter.latitude, buffer);
  } else {
    MDL::WriteLine(buffer, "\t%s %s ", MDL::TokenText(0x1BB), MDL::TokenText(0x162));
    WriteKeyData(buffer, &emitter.staticLatitude, 1);
  }
  if (emitter.longitude.keys.Count()) {
    WriteFloatKeyFrames(0x161, "\t", emitter.longitude, buffer);
  } else {
    MDL::WriteLine(buffer, "\t%s %s ", MDL::TokenText(0x1BB), MDL::TokenText(0x161));
    WriteKeyData(buffer, &emitter.staticLongitude, 1);
  }
  WriteFloatKeyFrames(0x1D9, "\t", emitter.visibilityKeys, buffer);
  IWriteParticleOptions(emitter.particle, buffer);
  WriteObjectTrailer(emitter, buffer);
}

namespace MDL {

  BOOL WriteParticleEmitters(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
    if (!static_cast<LPCSTR>(data.model.animationFile)[0]) {
      for (UINT i = 0; i < data.particleEmitters.Count(); ++i) {
        IWriteParticleEmitter(data, data.particleEmitters[i], data.particleEmitters.Count() != data.objects.Count(), buffer);
      }
    }
    return 1;
  }

}  // namespace MDL

static UINT GetBinParticleEmitterSize(const MDLPARTICLEEMITTER &section) {
  UINT size = GetBinGenObjectSize(section) + 288;
  if (section.emissionRate.keys.Count()) {
    size += 16 + section.emissionRate.keys.Count() * (4 + 4 * (section.emissionRate.type > TRACK_LINEAR ? 3 : 1));
  }
  if (section.gravity.keys.Count()) {
    size += 16 + section.gravity.keys.Count() * (4 + 4 * (section.gravity.type > TRACK_LINEAR ? 3 : 1));
  }
  if (section.longitude.keys.Count()) {
    size += 16 + section.longitude.keys.Count() * (4 + 4 * (section.longitude.type > TRACK_LINEAR ? 3 : 1));
  }
  if (section.latitude.keys.Count()) {
    size += 16 + section.latitude.keys.Count() * (4 + 4 * (section.latitude.type > TRACK_LINEAR ? 3 : 1));
  }
  if (section.particle.life.keys.Count()) {
    size += 16 + section.particle.life.keys.Count() * (4 + 4 * (section.particle.life.type > TRACK_LINEAR ? 3 : 1));
  }
  if (section.particle.speed.keys.Count()) {
    size += 16 + section.particle.speed.keys.Count() * (4 + 4 * (section.particle.speed.type > TRACK_LINEAR ? 3 : 1));
  }
  if (section.visibilityKeys.keys.Count()) {
    size += 16 + section.visibilityKeys.keys.Count() * (4 + 4 * (section.visibilityKeys.type > TRACK_LINEAR ? 3 : 1));
  }
  return size;
}

static void IWriteBinParticleEmitter(const MDLPARTICLEEMITTER &section, CMsgBuffer &buf, CMDLStatus *status) {
  buf.AddUint(GetBinParticleEmitterSize(section));
  WriteBinGenObject(section, buf, status);
  buf.AddFloat(section.staticEmissionRate);
  buf.AddFloat(section.staticGravity);
  buf.AddFloat(section.staticLongitude);
  buf.AddFloat(section.staticLatitude);
  buf.AddTcharArray(section.particle.path, 260, 1);
  buf.AddFloat(section.particle.staticLife);
  buf.AddFloat(section.particle.staticSpeed);
  WriteBinFloatKeyFrames(section.emissionRate, 'EEPK', buf);
  WriteBinFloatKeyFrames(section.gravity, 'GEPK', buf);
  WriteBinFloatKeyFrames(section.longitude, 'NLPK', buf);
  WriteBinFloatKeyFrames(section.latitude, 'TLPK', buf);
  WriteBinFloatKeyFrames(section.particle.life, 'LEPK', buf);
  WriteBinFloatKeyFrames(section.particle.speed, 'SEPK', buf);
  WriteBinFloatKeyFrames(section.visibilityKeys, 'SIVK', buf);
}

static BOOL ReadBinParticleEmitter(CMsgBuffer &buf, MDLPARTICLEEMITTER *pEmit, CMDLStatus *status, UINT &totalRead) {
  UINT sectionLength = buf.GetUint();
  UINT localBytesRead = 4;
  if (!ReadBinGenObject(*pEmit, buf, status, localBytesRead)) {
    status->Add(STATUS_ERROR, "Error reading gen object portion of particle emitter.\n");
    return 0;
  }
  pEmit->staticEmissionRate = buf.GetFloat();
  pEmit->staticGravity = buf.GetFloat();
  pEmit->staticLongitude = buf.GetFloat();
  pEmit->staticLatitude = buf.GetFloat();
  buf.GetTcharArray(pEmit->particle.path, 260);
  pEmit->particle.staticLife = buf.GetFloat();
  pEmit->particle.staticSpeed = buf.GetFloat();
  localBytesRead += 284;
  while (localBytesRead < sectionLength) {
    DWORD tag = buf.GetDword();
    localBytesRead += 4;
    int ok = 1;
    switch (tag) {
      case 'EEPK':
        ok = ReadBinFloatKeyFrames(pEmit->emissionRate, buf, localBytesRead);
        break;
      case 'GEPK':
        ok = ReadBinFloatKeyFrames(pEmit->gravity, buf, localBytesRead);
        break;
      case 'NLPK':
        ok = ReadBinFloatKeyFrames(pEmit->longitude, buf, localBytesRead);
        break;
      case 'TLPK':
        ok = ReadBinFloatKeyFrames(pEmit->latitude, buf, localBytesRead);
        break;
      case 'LEPK':
        ok = ReadBinFloatKeyFrames(pEmit->particle.life, buf, localBytesRead);
        break;
      case 'SEPK':
        ok = ReadBinFloatKeyFrames(pEmit->particle.speed, buf, localBytesRead);
        break;
      case 'SIVK':
        ok = ReadBinFloatKeyFrames(pEmit->visibilityKeys, buf, localBytesRead);
        break;
      default:
        SkipUnknown(buf, localBytesRead);
        break;
    }
    if (!ok) {
      status->Add(STATUS_ERROR, "Error reading particle emitter key frames.\n");
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

  BOOL WriteBinParticleEmitters(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *status) {
    if (!static_cast<LPCSTR>(data.model.animationFile)[0] && data.particleEmitters.Count()) {
      buf.AddDword('MERP');
      UINT totalSize = 4;
      UINT i;
      for (i = 0; i < data.particleEmitters.Count(); ++i) {
        totalSize += GetBinParticleEmitterSize(data.particleEmitters[i]);
      }
      buf.AddUint(totalSize);
      buf.AddUint(data.particleEmitters.Count());
      for (i = 0; i < data.particleEmitters.Count(); ++i) {
        IWriteBinParticleEmitter(data.particleEmitters[i], buf, status);
      }
    }
    return 1;
  }

  BOOL ReadBinParticleEmitters(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status) {
    UINT totalRead = 4;
    UINT numEmitters = buf.GetUint();
    data.particleEmitters.SetCount(0);
    data.particleEmitters.ReserveSpace(numEmitters);
    while (totalRead < length) {
      MDLPARTICLEEMITTER *emitter = data.particleEmitters.New();
      if (!emitter) {
        status->FatalFlunked("ParticleEmitter", -1);
        return 0;
      }
      if (!ReadBinParticleEmitter(buf, emitter, status, totalRead)) {
        status->Add(STATUS_ERROR, "Error reading particle emitter.\n");
        return 0;
      }
      if (totalRead > length) {
        status->FatalOverran("ParticleEmitter Section", -1);
        return 0;
      }
      ReadBinObjectEnd(data, emitter, data.particleEmitters.Count() - 1, 0x50000000);
    }
    return 1;
  }

}  // namespace MDL
