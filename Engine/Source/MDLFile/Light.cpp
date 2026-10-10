#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"


namespace MDL {
  LPCSTR       TokenText(UINT token);
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);
  BOOL         ReadLight(Parser &, MDLDATA &, CMDLStatus *);
  BOOL         WriteLights(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL         WriteBinLights(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  BOOL         ReadBinLights(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
}  // namespace MDL

static void IAddLightErrors(TSet &errors) {
  AddObjectErrors(errors);
  errors.Add(MDLTOK_ATTENUATIONSTART, 1, 0);
  errors.Add(MDLTOK_ATTENUATIONEND, 1, 0);
  errors.Add(MDLTOK_INTENSITY, 1, 0);
  errors.Add(MDLTOK_COLOR, 1, 0);
  errors.Add(MDLTOK_AMB_INTENSITY, 0, 0);
  errors.Add(MDLTOK_AMB_COLOR, 0, 0);
  errors.Add(MDLTOK_VISIBILITY, 0, 0);
  errors.Add(MDLTOK_AMBIENT, 0, 0);
  errors.Add(MDLTOK_DIRECTIONAL, 0, 0);
  errors.Add(MDLTOK_OMNIDIRECTIONAL, 0, 0);
}

static BOOL IllegalStaticToken(UINT token) {
  switch (token) {
    case MDLTOK_AMB_COLOR:
    case MDLTOK_AMB_INTENSITY:
    case MDLTOK_ATTENUATIONSTART:
    case MDLTOK_ATTENUATIONEND:
    case MDLTOK_COLOR:
    case MDLTOK_INTENSITY:
      return 0;
    default:
      return 1;
  }
}

static void IReadLightKeyFrames(Parser &parse, UINT savedToken, LPCSTR tokenText, MDLLIGHTSECTION *light, UINT version) {
  switch (savedToken) {
    case MDLTOK_ATTENUATIONSTART:
      ReadObjectFloatKeyframes(parse, &light->attenstartkeys);
      break;
    case MDLTOK_ATTENUATIONEND:
      ReadObjectFloatKeyframes(parse, &light->attenendkeys);
      break;
    case MDLTOK_COLOR:
      if (version < 700 && light->type == LIGHTTYPE_AMBIENT) {
        ReadObjectFloatKeyframes(parse, &light->ambcolorkeys);
      } else {
        ReadObjectFloatKeyframes(parse, &light->colorkeys);
      }
      break;
    case MDLTOK_INTENSITY:
      if (version < 700 && light->type == LIGHTTYPE_AMBIENT) {
        ReadObjectFloatKeyframes(parse, &light->ambintensitykeys);
      } else {
        ReadObjectFloatKeyframes(parse, &light->intensitykeys);
      }
      break;
    case MDLTOK_AMB_COLOR:
      ReadObjectFloatKeyframes(parse, &light->ambcolorkeys);
      break;
    case MDLTOK_AMB_INTENSITY:
      ReadObjectFloatKeyframes(parse, &light->ambintensitykeys);
      break;
    case MDLTOK_VISIBILITY:
      ReadObjectFloatKeyframes(parse, &light->visibilityKeys);
      break;
    default:
      parse.FatalUnexpected(tokenText);
      break;
  }
}

static void IReadLightStaticData(Parser &parse, UINT savedToken, LPCSTR tokenText, MDLLIGHTSECTION *light, UINT version) {
  switch (savedToken) {
    case MDLTOK_ATTENUATIONSTART:
      ReadFloatKeyData(parse, &light->staticAttenStart, 1);
      break;
    case MDLTOK_ATTENUATIONEND:
      ReadFloatKeyData(parse, &light->staticAttenEnd, 1);
      break;
    case MDLTOK_COLOR:
      if (version < 700 && light->type == LIGHTTYPE_AMBIENT) {
        ReadFloatKeyData(parse, &light->staticAmbColor.b, 3);
      } else {
        ReadFloatKeyData(parse, &light->staticColor.b, 3);
      }
      break;
    case MDLTOK_INTENSITY:
      if (version < 700 && light->type == LIGHTTYPE_AMBIENT) {
        ReadFloatKeyData(parse, &light->staticAmbIntensity, 1);
      } else {
        ReadFloatKeyData(parse, &light->staticIntensity, 1);
      }
      break;
    case MDLTOK_AMB_COLOR:
      ReadFloatKeyData(parse, &light->staticAmbColor.b, 3);
      break;
    case MDLTOK_AMB_INTENSITY:
      ReadFloatKeyData(parse, &light->staticAmbIntensity, 1);
      break;
    default:
      parse.FatalUnexpected(tokenText);
      break;
  }
  parse.Expect(',');
}

static BOOL IReadLightProperties(Parser &parse, UINT savedToken, MDLLIGHTSECTION *light, UINT) {
  switch (savedToken) {
    case MDLTOK_AMBIENT:
      light->type = LIGHTTYPE_AMBIENT;
      break;
    case MDLTOK_DIRECTIONAL:
      light->type = LIGHTTYPE_DIRECT;
      break;
    case MDLTOK_OMNIDIRECTIONAL:
      light->type = LIGHTTYPE_OMNI;
      break;
    default:
      return 0;
  }
  parse.Expect(',');
  return 1;
}

static void IReadLight(Parser &parse, TSet &errors, NTempest::C3Vector *pivot, MDLLIGHTSECTION *light, CMDLStatus *status, UINT version) {
  parse.Expect('{');
  LPCSTR tokenText;
  UINT   token = parse.Token(&tokenText, 0);
  while (token != '}' && token) {
    int expectAnimation = IExpectAnimation(parse, &token, &tokenText);
    if (!expectAnimation && IllegalStaticToken(token)) {
      parse.FatalUnexpected(tokenText);
    }
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokenText);
    }
    if (!ReadObjectBody(parse, token, pivot, light, status) && !IReadLightProperties(parse, token, light, version)) {
      if (expectAnimation) {
        IReadLightKeyFrames(parse, token, tokenText, light, version);
      } else {
        IReadLightStaticData(parse, token, tokenText, light, version);
      }
    }
    token = parse.Token(&tokenText, 0);
  }
  parse.Expect('}', token, tokenText);
}

BOOL MDL::ReadLight(Parser &parse, MDLDATA &data, CMDLStatus *status) {
  FATALASSERT(status);
  TSet                errors;
  MDLLIGHTSECTION    *light = data.lights.New();
  NTempest::C3Vector *pivot = data.version < 500 ? data.pivotPoints.New() : 0;
  ReadObjectName(parse, light->name);
  IAddLightErrors(errors);
  IReadLight(parse, errors, pivot, light, status, data.version);
  ReadObjectEnd(errors, data, light, data.lights.Count() - 1, 0x20000000);
  errors.Complete(status);
  return !parse.FoundError();
}

static void IWriteLightProperties(const MDLLIGHTSECTION &section, TSGrowableArray<char> &buffer) {
  UINT token;
  switch (section.type) {
    case LIGHTTYPE_DIRECT:
      token = MDLTOK_DIRECTIONAL;
      break;
    case LIGHTTYPE_AMBIENT:
      token = MDLTOK_AMBIENT;
      break;
    default:
      token = MDLTOK_OMNIDIRECTIONAL;
      break;
  }
  MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(token));
}

static void IWriteLightSection(const MDLDATA &data, const MDLLIGHTSECTION &section, int needObjIds, TSGrowableArray<char> &buffer) {
  WriteObjectHeader(data, section, MDLTOK_LIGHT, needObjIds, buffer);
  IWriteLightProperties(section, buffer);
  if (section.attenstartkeys.keys.Count()) {
    WriteFloatKeyFrames(MDLTOK_ATTENUATIONSTART, "\t", section.attenstartkeys, buffer);
  } else {
    MDL::WriteLine(buffer, "%s%s %s ", "\t", MDL::TokenText(MDLTOK_STATIC), MDL::TokenText(MDLTOK_ATTENUATIONSTART));
    WriteKeyData(buffer, &section.staticAttenStart, 1);
  }
  if (section.attenendkeys.keys.Count()) {
    WriteFloatKeyFrames(MDLTOK_ATTENUATIONEND, "\t", section.attenendkeys, buffer);
  } else {
    MDL::WriteLine(buffer, "%s%s %s ", "\t", MDL::TokenText(MDLTOK_STATIC), MDL::TokenText(MDLTOK_ATTENUATIONEND));
    WriteKeyData(buffer, &section.staticAttenEnd, 1);
  }
  if (section.intensitykeys.keys.Count()) {
    WriteFloatKeyFrames(MDLTOK_INTENSITY, "\t", section.intensitykeys, buffer);
  } else {
    MDL::WriteLine(buffer, "%s%s %s ", "\t", MDL::TokenText(MDLTOK_STATIC), MDL::TokenText(MDLTOK_INTENSITY));
    WriteKeyData(buffer, &section.staticIntensity, 1);
  }
  if (section.colorkeys.keys.Count()) {
    WriteFloatKeyFrames(MDLTOK_COLOR, "\t", section.colorkeys, buffer);
  } else {
    MDL::WriteLine(buffer, "%s%s %s ", "\t", MDL::TokenText(MDLTOK_STATIC), MDL::TokenText(MDLTOK_COLOR));
    WriteKeyData(buffer, &section.staticColor.b, 3);
  }
  if (section.ambintensitykeys.keys.Count()) {
    WriteFloatKeyFrames(MDLTOK_AMB_INTENSITY, "\t", section.ambintensitykeys, buffer);
  } else {
    MDL::WriteLine(buffer, "%s%s %s ", "\t", MDL::TokenText(MDLTOK_STATIC), MDL::TokenText(MDLTOK_AMB_INTENSITY));
    WriteKeyData(buffer, &section.staticAmbIntensity, 1);
  }
  if (section.ambcolorkeys.keys.Count()) {
    WriteFloatKeyFrames(MDLTOK_AMB_COLOR, "\t", section.ambcolorkeys, buffer);
  } else {
    MDL::WriteLine(buffer, "%s%s %s ", "\t", MDL::TokenText(MDLTOK_STATIC), MDL::TokenText(MDLTOK_AMB_COLOR));
    WriteKeyData(buffer, &section.staticAmbColor.b, 3);
  }
  WriteFloatKeyFrames(MDLTOK_VISIBILITY, "\t", section.visibilityKeys, buffer);
  WriteObjectTrailer(section, buffer);
}

BOOL MDL::WriteLights(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
  if (data.model.animationFile[0]) {
    return 1;
  }
  UINT numLights = data.lights.Count();
  int needObjIds = numLights != data.objects.Count();
  const MDLLIGHTSECTION *pLight = data.lights.Ptr();
  for (UINT i = numLights; i; --i, ++pLight) {
    IWriteLightSection(data, *pLight, needObjIds, buffer);
  }
  return 1;
}

static UINT GetBinLightSize(const MDLLIGHTSECTION &section) {
  UINT                      size = GetBinGenObjectSize(section) + 48;
  const MDLKEYTRACK<float> *floatTracks[5] = {
      &section.attenstartkeys, &section.attenendkeys, &section.intensitykeys, &section.ambintensitykeys, &section.visibilityKeys
  };
  for (UINT i = 0; i < 5; ++i) {
    const MDLKEYTRACK<float> &track = *floatTracks[i];
    if (track.keys.Count()) {
      UINT dataSize = track.type > TRACK_LINEAR ? 12 : 4;
      size += 16 + track.keys.Count() * (4 + dataSize);
    }
  }
  const MDLKEYTRACK<C3Color> *colorTracks[2] = {&section.colorkeys, &section.ambcolorkeys};
  for (UINT j = 0; j < 2; ++j) {
    const MDLKEYTRACK<C3Color> &track = *colorTracks[j];
    if (track.keys.Count()) {
      UINT dataSize = track.type > TRACK_LINEAR ? 36 : 12;
      size += 16 + track.keys.Count() * (4 + dataSize);
    }
  }
  return size;
}

static void IWriteBinLightSection(const MDLLIGHTSECTION &section, CMsgBuffer &buffer, CMDLStatus *status) {
  buffer.AddUint(GetBinLightSize(section));
  WriteBinGenObject(section, buffer, status);
  buffer.AddDword(section.type);
  buffer.AddFloat(section.staticAttenStart);
  buffer.AddFloat(section.staticAttenEnd);
  buffer.AddFloat(section.staticColor.r);
  buffer.AddFloat(section.staticColor.g);
  buffer.AddFloat(section.staticColor.b);
  buffer.AddFloat(section.staticIntensity);
  buffer.AddFloat(section.staticAmbColor.r);
  buffer.AddFloat(section.staticAmbColor.g);
  buffer.AddFloat(section.staticAmbColor.b);
  buffer.AddFloat(section.staticAmbIntensity);
  WriteBinFloatKeyFrames(section.attenstartkeys, 'SALK', buffer);
  WriteBinFloatKeyFrames(section.attenendkeys, 'EALK', buffer);
  WriteBinFloatKeyFrames(section.colorkeys, 'CALK', buffer);
  WriteBinFloatKeyFrames(section.intensitykeys, 'IALK', buffer);
  WriteBinFloatKeyFrames(section.ambcolorkeys, 'CBLK', buffer);
  WriteBinFloatKeyFrames(section.ambintensitykeys, 'IBLK', buffer);
  WriteBinFloatKeyFrames(section.visibilityKeys, 'SIVK', buffer);
}

BOOL MDL::WriteBinLights(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *status) {
  if (data.model.animationFile[0]) {
    return 1;
  }
  if (!data.lights.Count()) {
    return 1;
  }
  buf.AddDword('ETIL');
  UINT numLights = data.lights.Count();
  UINT totalSize = 4;
  UINT i;
  for (i = 0; i < numLights; ++i) {
    totalSize += GetBinLightSize(data.lights[i]);
  }
  buf.AddUint(totalSize);
  buf.AddUint(numLights);
  for (i = 0; i < numLights; ++i) {
    IWriteBinLightSection(data.lights[i], buf, status);
  }
  return 1;
}

static BOOL ReadBinLight(CMsgBuffer &buf, MDLLIGHTSECTION *pLight, CMDLStatus *status, UINT &totalRead, UINT version) {
  UINT sectionLength = buf.GetUint();
  UINT localBytesRead = 4;
  if (!ReadBinGenObject(*pLight, buf, status, localBytesRead)) {
    status->Add(STATUS_ERROR, "Error reading gen object portion of light.\n");
    return 0;
  }
  pLight->type = (LIGHT_TYPE)buf.GetDword();
  localBytesRead += 4;
  pLight->staticAttenStart = buf.GetFloat();
  localBytesRead += 4;
  pLight->staticAttenEnd = buf.GetFloat();
  localBytesRead += 4;
  pLight->staticColor.r = buf.GetFloat();
  pLight->staticColor.g = buf.GetFloat();
  pLight->staticColor.b = buf.GetFloat();
  localBytesRead += 12;
  pLight->staticIntensity = buf.GetFloat();
  localBytesRead += 4;
  if (version >= 700) {
    pLight->staticAmbColor.r = buf.GetFloat();
    pLight->staticAmbColor.g = buf.GetFloat();
    pLight->staticAmbColor.b = buf.GetFloat();
    localBytesRead += 12;
    pLight->staticAmbIntensity = buf.GetFloat();
    localBytesRead += 4;
  }
  while (localBytesRead < sectionLength) {
    DWORD tag = buf.GetDword();
    localBytesRead += 4;
    switch (tag) {
      case 'SALK':
        if (!ReadBinFloatKeyFrames(pLight->attenstartkeys, buf, localBytesRead)) {
          status->Add(STATUS_ERROR, "Error reading light attenstart keys.\n");
          return 0;
        }
        break;
      case 'EALK':
        if (!ReadBinFloatKeyFrames(pLight->attenendkeys, buf, localBytesRead)) {
          status->Add(STATUS_ERROR, "Error reading light attenend keys.\n");
          return 0;
        }
        break;
      case 'CALK':
        if (!ReadBinFloatKeyFrames(pLight->colorkeys, buf, localBytesRead)) {
          status->Add(STATUS_ERROR, "Error reading light color keys.\n");
          return 0;
        }
        break;
      case 'IALK':
        if (!ReadBinFloatKeyFrames(pLight->intensitykeys, buf, localBytesRead)) {
          status->Add(STATUS_ERROR, "Error reading light intensity keys.\n");
          return 0;
        }
        break;
      case 'SIVK':
        if (!ReadBinFloatKeyFrames(pLight->visibilityKeys, buf, localBytesRead)) {
          status->Add(STATUS_ERROR, "Error reading light intensity keys.\n");
          return 0;
        }
        break;
      case 'CBLK':
        if (!ReadBinFloatKeyFrames(pLight->ambcolorkeys, buf, localBytesRead)) {
          status->Add(STATUS_ERROR, "Error reading light color keys.\n");
          return 0;
        }
        break;
      case 'IBLK':
        if (!ReadBinFloatKeyFrames(pLight->ambintensitykeys, buf, localBytesRead)) {
          status->Add(STATUS_ERROR, "Error reading light intensity keys.\n");
          return 0;
        }
        break;
      default:
        SkipUnknown(buf, localBytesRead);
        break;
    }
    if (localBytesRead > sectionLength) {
      status->FatalOverran("Lights", -1);
      return 0;
    }
  }
  totalRead += localBytesRead;
  return 1;
}

BOOL MDL::ReadBinLights(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status) {
  UINT totalRead = 4;
  UINT numLights = buf.GetUint();
  data.lights.SetCount(0);
  data.lights.ReserveSpace(numLights);
  while (totalRead < length) {
    MDLLIGHTSECTION *light = data.lights.New();
    if (!light) {
      status->FatalFlunked("Light", -1);
      return 0;
    }
    if (!ReadBinLight(buf, light, status, totalRead, data.version)) {
      status->Add(STATUS_ERROR, "Error reading light section.\n");
      return 0;
    }
    if (totalRead > length) {
      status->FatalOverran("Light", -1);
      return 0;
    }
    ReadBinObjectEnd(data, light, data.lights.Count() - 1, 0x20000000);
  }
  return 1;
}
