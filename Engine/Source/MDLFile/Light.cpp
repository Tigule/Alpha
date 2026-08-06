#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

namespace MDL {
  LPCSTR       TokenText(UINT token);
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);

  static void IAddLightErrors(TSet &errors) {
    AddObjectErrors(errors);
    errors.Add(0x126, 1, 0);
    errors.Add(0x127, 1, 0);
    errors.Add(0x15F, 1, 0);
    errors.Add(0x136, 1, 0);
    errors.Add(0x120, 0, 0);
    errors.Add(0x11F, 0, 0);
    errors.Add(0x1D9, 0, 0);
    errors.Add(0x121, 0, 0);
    errors.Add(0x13E, 0, 0);
    errors.Add(0x188, 0, 0);
  }

  static int IllegalStaticToken(UINT token) {
    switch (token) {
      case 0x11F:
      case 0x120:
      case 0x126:
      case 0x127:
      case 0x136:
      case 0x15F:
        return 0;
      default:
        return 1;
    }
  }

  static void IReadLightKeyFrames(Parser &parse, UINT savedToken, LPCSTR tokenText, MDLLIGHTSECTION *light, UINT) {
    switch (savedToken) {
      case 0x11F:
        ReadObjectFloatKeyframes(parse, &light->ambcolorkeys);
        break;
      case 0x120:
        ReadObjectFloatKeyframes(parse, &light->ambintensitykeys);
        break;
      case 0x126:
        ReadObjectFloatKeyframes(parse, &light->attenstartkeys);
        break;
      case 0x127:
        ReadObjectFloatKeyframes(parse, &light->attenendkeys);
        break;
      case 0x136:
        ReadObjectFloatKeyframes(parse, &light->colorkeys);
        break;
      case 0x15F:
        ReadObjectFloatKeyframes(parse, &light->intensitykeys);
        break;
      case 0x1D9:
        ReadObjectFloatKeyframes(parse, &light->visibilityKeys);
        break;
      default:
        parse.FatalUnexpected(tokenText);
        break;
    }
  }

  static void IReadLightStaticData(Parser &parse, UINT savedToken, LPCSTR tokenText, MDLLIGHTSECTION *light, UINT) {
    switch (savedToken) {
      case 0x11F:
        ReadFloatKeyData(parse, &light->staticAmbColor.b, 3);
        break;
      case 0x120:
        ReadFloatKeyData(parse, &light->staticAmbIntensity, 1);
        break;
      case 0x126:
        ReadFloatKeyData(parse, &light->staticAttenStart, 1);
        break;
      case 0x127:
        ReadFloatKeyData(parse, &light->staticAttenEnd, 1);
        break;
      case 0x136:
        ReadFloatKeyData(parse, &light->staticColor.b, 3);
        break;
      case 0x15F:
        ReadFloatKeyData(parse, &light->staticIntensity, 1);
        break;
      default:
        parse.FatalUnexpected(tokenText);
        break;
    }
    parse.Expect(',');
  }

  static int IReadLightProperties(Parser &parse, UINT savedToken, MDLLIGHTSECTION *light, UINT) {
    switch (savedToken) {
      case 0x121:
        light->type = LIGHTTYPE_AMBIENT;
        break;
      case 0x13E:
        light->type = LIGHTTYPE_DIRECT;
        break;
      case 0x188:
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
    while (token && token != '}') {
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

  int ReadLight(Parser &parse, MDLDATA &data, CMDLStatus *status) {
    FATALASSERT(status);
    TSet                errors;
    MDLLIGHTSECTION    *light = data.lights.New();
    NTempest::C3Vector *pivot = data.pivotPoints.Count() < 500 ? data.pivotPoints.New() : 0;
    ReadObjectName(parse, light->name);
    IAddLightErrors(errors);
    IReadLight(parse, errors, pivot, light, status, data.version);
    ReadObjectEnd(errors, data, light, data.lights.Count() - 1, 0x20000000);
    errors.Complete(status);
    return !parse.FoundError();
  }

  static void IWriteLightProperties(const MDLLIGHTSECTION &section, TSGrowableArray<char> &buffer) {
    UINT token = 0x188;
    if (section.type == LIGHTTYPE_DIRECT) {
      token = 0x13E;
    } else if (section.type == LIGHTTYPE_AMBIENT) {
      token = 0x121;
    }
    WriteLine(buffer, "\t%s,\n", TokenText(token));
  }

  static void IWriteLightSection(const MDLDATA &data, const MDLLIGHTSECTION &section, int needObjIds, TSGrowableArray<char> &buffer) {
    WriteObjectHeader(data, section, 0x10E, needObjIds, buffer);
    IWriteLightProperties(section, buffer);
    if (section.attenstartkeys.keys.Count()) {
      WriteFloatKeyFrames(0x126, "\t", section.attenstartkeys, buffer);
    } else {
      WriteLine(buffer, "\t%s %s ", TokenText(0x1BB), TokenText(0x126));
      WriteKeyData(buffer, &section.staticAttenStart, 1);
    }
    if (section.attenendkeys.keys.Count()) {
      WriteFloatKeyFrames(0x127, "\t", section.attenendkeys, buffer);
    } else {
      WriteLine(buffer, "\t%s %s ", TokenText(0x1BB), TokenText(0x127));
      WriteKeyData(buffer, &section.staticAttenEnd, 1);
    }
    if (section.intensitykeys.keys.Count()) {
      WriteFloatKeyFrames(0x15F, "\t", section.intensitykeys, buffer);
    } else {
      WriteLine(buffer, "\t%s %s ", TokenText(0x1BB), TokenText(0x15F));
      WriteKeyData(buffer, &section.staticIntensity, 1);
    }
    if (section.colorkeys.keys.Count()) {
      WriteFloatKeyFrames(0x136, "\t", section.colorkeys, buffer);
    } else {
      WriteLine(buffer, "\t%s %s ", TokenText(0x1BB), TokenText(0x136));
      WriteKeyData(buffer, &section.staticColor.b, 3);
    }
    if (section.ambintensitykeys.keys.Count()) {
      WriteFloatKeyFrames(0x120, "\t", section.ambintensitykeys, buffer);
    } else {
      WriteLine(buffer, "\t%s %s ", TokenText(0x1BB), TokenText(0x120));
      WriteKeyData(buffer, &section.staticAmbIntensity, 1);
    }
    if (section.ambcolorkeys.keys.Count()) {
      WriteFloatKeyFrames(0x11F, "\t", section.ambcolorkeys, buffer);
    } else {
      WriteLine(buffer, "\t%s %s ", TokenText(0x1BB), TokenText(0x11F));
      WriteKeyData(buffer, &section.staticAmbColor.b, 3);
    }
    WriteFloatKeyFrames(0x1D9, "\t", section.visibilityKeys, buffer);
    WriteObjectTrailer(section, buffer);
  }

  int WriteLights(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
    if (!static_cast<LPCSTR>(data.model.animationFile)[0]) {
      for (UINT i = 0; i < data.lights.Count(); ++i) {
        IWriteLightSection(data, data.lights.Ptr()[i], data.lights.Count() != data.objects.Count(), buffer);
      }
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
    if (section.colorkeys.keys.Count()) {
      const MDLKEYTRACK<C3Color> &track = section.colorkeys;
      buffer.AddDword('CALK');
      buffer.AddUint(track.keys.Count());
      buffer.AddUint(track.type);
      buffer.AddUint(track.globalSeqId);
      UINT values = track.type > TRACK_LINEAR ? 9 : 3;
      for (UINT i = 0; i < track.keys.Count(); ++i) {
        const MDLKEYFRAME<C3Color> &key = track.keys.Ptr()[i];
        buffer.AddInt(key.time);
        buffer.AddFloatArray(&key.value.b, values);
      }
    }
    WriteBinFloatKeyFrames(section.intensitykeys, 'IALK', buffer);
    if (section.ambcolorkeys.keys.Count()) {
      const MDLKEYTRACK<C3Color> &track = section.ambcolorkeys;
      buffer.AddDword('CBLK');
      buffer.AddUint(track.keys.Count());
      buffer.AddUint(track.type);
      buffer.AddUint(track.globalSeqId);
      UINT values = track.type > TRACK_LINEAR ? 9 : 3;
      for (UINT i = 0; i < track.keys.Count(); ++i) {
        const MDLKEYFRAME<C3Color> &key = track.keys.Ptr()[i];
        buffer.AddInt(key.time);
        buffer.AddFloatArray(&key.value.b, values);
      }
    }
    WriteBinFloatKeyFrames(section.ambintensitykeys, 'IBLK', buffer);
    WriteBinFloatKeyFrames(section.visibilityKeys, 'SIVK', buffer);
  }

  int WriteBinLights(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *status) {
    if (!static_cast<LPCSTR>(data.model.animationFile)[0] && data.lights.Count()) {
      buf.AddDword('ETIL');
      UINT totalSize = 4;
      UINT i;
      for (i = 0; i < data.lights.Count(); ++i) {
        totalSize += GetBinLightSize(data.lights.Ptr()[i]);
      }
      buf.AddUint(totalSize);
      buf.AddUint(data.lights.Count());
      for (i = 0; i < data.lights.Count(); ++i) {
        IWriteBinLightSection(data.lights.Ptr()[i], buf, status);
      }
    }
    return 1;
  }

  static int ReadBinLight(CMsgBuffer &buffer, MDLLIGHTSECTION *light, CMDLStatus *status, UINT &totalRead, UINT version) {
    UINT sectionLength = buffer.GetUint();
    UINT localRead = 4;
    if (!ReadBinGenObject(*light, buffer, status, localRead)) {
      status->Add(STATUS_ERROR, "Error reading gen object portion of light.\n");
      return 0;
    }
    light->type = static_cast<LIGHT_TYPE>(buffer.GetDword());
    light->staticAttenStart = buffer.GetFloat();
    light->staticAttenEnd = buffer.GetFloat();
    light->staticColor.r = buffer.GetFloat();
    light->staticColor.g = buffer.GetFloat();
    light->staticColor.b = buffer.GetFloat();
    light->staticIntensity = buffer.GetFloat();
    localRead += 28;
    if (version >= 700) {
      light->staticAmbColor.r = buffer.GetFloat();
      light->staticAmbColor.g = buffer.GetFloat();
      light->staticAmbColor.b = buffer.GetFloat();
      light->staticAmbIntensity = buffer.GetFloat();
      localRead += 16;
    }
    while (localRead < sectionLength) {
      DWORD tag = buffer.GetDword();
      localRead += 4;
      int ok = 1;
      if (tag == 'SALK') {
        ok = ReadBinFloatKeyFrames(light->attenstartkeys, buffer, localRead);
        if (!ok)
          status->Add(STATUS_ERROR, "Error reading light attenstart keys.\n");
      } else if (tag == 'EALK') {
        ok = ReadBinFloatKeyFrames(light->attenendkeys, buffer, localRead);
        if (!ok)
          status->Add(STATUS_ERROR, "Error reading light attenend keys.\n");
      } else if (tag == 'CALK') {
        ok = ReadBinFloatKeyFrames(light->colorkeys, buffer, localRead);
        if (!ok)
          status->Add(STATUS_ERROR, "Error reading light color keys.\n");
      } else if (tag == 'IALK') {
        ok = ReadBinFloatKeyFrames(light->intensitykeys, buffer, localRead);
        if (!ok)
          status->Add(STATUS_ERROR, "Error reading light intensity keys.\n");
      } else if (tag == 'CBLK') {
        ok = ReadBinFloatKeyFrames(light->ambcolorkeys, buffer, localRead);
        if (!ok)
          status->Add(STATUS_ERROR, "Error reading light color keys.\n");
      } else if (tag == 'IBLK') {
        ok = ReadBinFloatKeyFrames(light->ambintensitykeys, buffer, localRead);
        if (!ok)
          status->Add(STATUS_ERROR, "Error reading light intensity keys.\n");
      } else if (tag == 'SIVK') {
        ok = ReadBinFloatKeyFrames(light->visibilityKeys, buffer, localRead);
        if (!ok)
          status->Add(STATUS_ERROR, "Error reading light intensity keys.\n");
      } else {
        SkipUnknown(buffer, localRead);
      }
      if (!ok) {
        return 0;
      }
    }
    if (localRead > sectionLength) {
      status->FatalOverran("Lights", -1);
      return 0;
    }
    totalRead += localRead;
    return 1;
  }

  int ReadBinLights(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status) {
    UINT numLights = buf.GetUint();
    UINT totalRead = 4;
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

}  // namespace MDL
