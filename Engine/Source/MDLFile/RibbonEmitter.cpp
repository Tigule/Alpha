#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

#include <math.h>

namespace MDL {
  LPCSTR       TokenText(UINT token);
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);

  static void IAddRibbonEmitterErrors(TSet &errors) {
    AddObjectErrors(errors);
    errors.Add(0x144, 1, 0);
    errors.Add(0x1AE, 1, 0);
    errors.Add(0x137, 1, 0);
    errors.Add(0x1D9, 0, 0);
    errors.Add(0x11C, 1, 0);
    errors.Add(0x136, 1, 0);
    errors.Add(0x159, 1, 0);
    errors.Add(0x15A, 1, 0);
    errors.Add(0x165, 1, 0);
    errors.Add(0x1C4, 1, 0);
    errors.Add(0x16D, 1, 0);
  }

  static void IReadRibbonEmitterKeyFrames(Parser &parse, UINT savedToken, LPCSTR tokenText, MDLRIBBONEMITTER *emitter) {
    switch (savedToken) {
      case 0x11C:
        ReadObjectFloatKeyframes(parse, &emitter->alphaKeys);
        break;
      case 0x136:
        ReadObjectFloatKeyframes(parse, &emitter->colorKeys);
        break;
      case 0x137:
        emitter->textureCols = parse.ExpectInt();
        parse.Expect(',');
        break;
      case 0x144:
        emitter->edgesPerSecond = parse.ExpectInt();
        parse.Expect(',');
        break;
      case 0x153:
        emitter->gravity = parse.ExpectFloat();
        parse.Expect(',');
        break;
      case 0x159:
        ReadObjectFloatKeyframes(parse, &emitter->heightAbove);
        break;
      case 0x15A:
        ReadObjectFloatKeyframes(parse, &emitter->heightBelow);
        break;
      case 0x165:
        emitter->edgeLifetime = parse.ExpectFloat();
        parse.Expect(',');
        break;
      case 0x16D:
        emitter->materialId = parse.ExpectInt();
        parse.Expect(',');
        break;
      case 0x1AE:
        emitter->textureRows = parse.ExpectInt();
        parse.Expect(',');
        break;
      case 0x1C4: {
        UINT       token;
        LPCSTR     text;
        UTokenData tokenData;
        long       expected = parse.GetOptionalInt(&token, &text, &tokenData);
        if (expected > 0) {
          emitter->textureSlot.keys.ReserveSpace(expected);
        }
        parse.Expect('{', token, text);
        token = ReadIntTrackHeader(parse, &emitter->textureSlot, &text, &tokenData);
        long actual = 0;
        while (token == 0x100) {
          MDLINTKEY *key = emitter->textureSlot.keys.New();
          key->time = tokenData.lVal;
          parse.Expect(':');
          key->value = parse.ExpectInt();
          parse.Expect(',');
          ++actual;
          token = parse.Token(&text, &tokenData);
        }
        parse.Expect('}', token, text);
        if (expected >= 0 && actual != expected) {
          parse.WarningCount("key frames", expected, actual);
        }
        break;
      }
      case 0x1D9:
        ReadObjectFloatKeyframes(parse, &emitter->visibilityKeys);
        break;
      default:
        parse.FatalUnexpected(tokenText);
        break;
    }
  }

  static void IReadRibbonEmitterStaticData(Parser &parse, UINT savedToken, LPCSTR tokenText, MDLRIBBONEMITTER *emitter) {
    switch (savedToken) {
      case 0x11C:
        ReadFloatKeyData(parse, &emitter->staticAlpha, 1);
        break;
      case 0x136:
        ReadFloatKeyData(parse, &emitter->staticColor.b, 3);
        break;
      case 0x159:
        ReadFloatKeyData(parse, &emitter->staticHeightAbove, 1);
        break;
      case 0x15A:
        ReadFloatKeyData(parse, &emitter->staticHeightBelow, 1);
        break;
      case 0x1C4:
        emitter->staticTextureSlot = parse.ExpectInt();
        break;
      default:
        parse.FatalUnexpected(tokenText);
        break;
    }
    parse.Expect(',');
  }

  static void IReadRibbonEmitter(Parser &parse, TSet &errors, MDLRIBBONEMITTER *emitter, CMDLStatus *status) {
    parse.Expect('{');
    LPCSTR tokenText;
    UINT   token = parse.Token(&tokenText, 0);
    while (token && token != '}') {
      int expectAnimation = IExpectAnimation(parse, &token, &tokenText);
      if (!errors.Check(token)) {
        parse.FatalDuplicate(tokenText);
      }
      if (!ReadObjectBody(parse, token, 0, emitter, status)) {
        if (expectAnimation) {
          IReadRibbonEmitterKeyFrames(parse, token, tokenText, emitter);
        } else {
          IReadRibbonEmitterStaticData(parse, token, tokenText, emitter);
        }
      }
      token = parse.Token(&tokenText, 0);
    }
    parse.Expect('}', token, tokenText);
  }

  int ReadRibbonEmitter(Parser &parse, MDLDATA &data, CMDLStatus *status) {
    FATALASSERT(status);
    TSet              errors;
    MDLRIBBONEMITTER *emitter = data.ribbonEmitters.New();
    IAddRibbonEmitterErrors(errors);
    ReadObjectName(parse, emitter->name);
    IReadRibbonEmitter(parse, errors, emitter, status);
    ReadObjectEnd(errors, data, emitter, data.ribbonEmitters.Count() - 1, 0x90000000);
    errors.Complete(status);
    return !parse.FoundError();
  }

  static void IWriteRibbonEmitter(const MDLDATA &data, const MDLRIBBONEMITTER &emitter, int needObjIds, TSGrowableArray<char> &buffer) {
    WriteObjectHeader(data, emitter, 0x118, needObjIds, buffer);
    if (emitter.heightAbove.keys.Count()) {
      WriteFloatKeyFrames(0x159, "\t", emitter.heightAbove, buffer);
    } else {
      WriteLine(buffer, "\t%s %s ", TokenText(0x1BB), TokenText(0x159));
      WriteKeyData(buffer, &emitter.staticHeightAbove, 1);
    }
    if (emitter.heightBelow.keys.Count()) {
      WriteFloatKeyFrames(0x15A, "\t", emitter.heightBelow, buffer);
    } else {
      WriteLine(buffer, "\t%s %s ", TokenText(0x1BB), TokenText(0x15A));
      WriteKeyData(buffer, &emitter.staticHeightBelow, 1);
    }
    if (emitter.alphaKeys.keys.Count()) {
      WriteFloatKeyFrames(0x11C, "\t", emitter.alphaKeys, buffer);
    } else {
      WriteLine(buffer, "\t%s %s ", TokenText(0x1BB), TokenText(0x11C));
      WriteKeyData(buffer, &emitter.staticAlpha, 1);
    }
    if (emitter.colorKeys.keys.Count()) {
      WriteFloatKeyFrames(0x136, "\t", emitter.colorKeys, buffer);
    } else {
      WriteLine(buffer, "\t%s %s ", TokenText(0x1BB), TokenText(0x136));
      WriteKeyData(buffer, &emitter.staticColor.b, 3);
    }
    if (emitter.textureSlot.keys.Count()) {
      WriteIntKeyFrames(0x1C4, "\t", emitter.textureSlot, buffer);
    } else {
      WriteLine(buffer, "\t%s %s ", TokenText(0x1BB), TokenText(0x1C4));
      WriteUintKeyData(buffer, &emitter.staticTextureSlot, 1);
    }
    WriteFloatKeyFrames(0x1D9, "\t", emitter.visibilityKeys, buffer);
    WriteLine(buffer, "\t%s %u,\n", TokenText(0x144), emitter.edgesPerSecond);
    WriteLine(buffer, "\t%s %g,\n", TokenText(0x165), emitter.edgeLifetime);
    if (fabs(emitter.gravity) >= 2.3841858e-7f) {
      WriteLine(buffer, "\t%s %g,\n", TokenText(0x153), emitter.gravity);
    }
    WriteLine(buffer, "\t%s %u,\n", TokenText(0x1AE), emitter.textureRows);
    WriteLine(buffer, "\t%s %u,\n", TokenText(0x137), emitter.textureCols);
    WriteLine(buffer, "\t%s %u,\n", TokenText(0x16D), emitter.materialId);
    WriteObjectTrailer(emitter, buffer);
  }

  int WriteRibbonEmitters(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
    if (!static_cast<LPCSTR>(data.model.animationFile)[0]) {
      for (UINT i = 0; i < data.ribbonEmitters.Count(); ++i) {
        IWriteRibbonEmitter(data, data.ribbonEmitters.Ptr()[i], data.ribbonEmitters.Count() != data.objects.Count(), buffer);
      }
    }
    return 1;
  }

  static UINT GetRibbonFixedDataSize() {
    return 56;
  }

  static UINT GetBinRibbonEmitterSize(const MDLRIBBONEMITTER &section) {
    UINT size = 4 + GetBinGenObjectSize(section) + GetRibbonFixedDataSize();
    if (section.heightAbove.keys.Count()) {
      UINT dataSize = section.heightAbove.type > TRACK_LINEAR ? 12 : 4;
      size += 16 + section.heightAbove.keys.Count() * (4 + dataSize);
    }
    if (section.heightBelow.keys.Count()) {
      UINT dataSize = section.heightBelow.type > TRACK_LINEAR ? 12 : 4;
      size += 16 + section.heightBelow.keys.Count() * (4 + dataSize);
    }
    if (section.alphaKeys.keys.Count()) {
      UINT dataSize = section.alphaKeys.type > TRACK_LINEAR ? 12 : 4;
      size += 16 + section.alphaKeys.keys.Count() * (4 + dataSize);
    }
    if (section.colorKeys.keys.Count()) {
      UINT dataSize = section.colorKeys.type > TRACK_LINEAR ? 36 : 12;
      size += 16 + section.colorKeys.keys.Count() * (4 + dataSize);
    }
    if (section.textureSlot.keys.Count()) {
      size += 16 + section.textureSlot.keys.Count() * 8;
    }
    if (section.visibilityKeys.keys.Count()) {
      UINT dataSize = section.visibilityKeys.type > TRACK_LINEAR ? 12 : 4;
      size += 16 + section.visibilityKeys.keys.Count() * (4 + dataSize);
    }
    return size;
  }

  static void IWriteBinRibbonEmitter(const MDLRIBBONEMITTER &section, CMsgBuffer &buffer, CMDLStatus *status) {
    buffer.AddUint(GetBinRibbonEmitterSize(section));
    WriteBinGenObject(section, buffer, status);
    buffer.AddUint(GetRibbonFixedDataSize());
    buffer.AddFloat(section.staticHeightAbove);
    buffer.AddFloat(section.staticHeightBelow);
    buffer.AddFloat(section.staticAlpha);
    buffer.AddFloat(section.staticColor.r);
    buffer.AddFloat(section.staticColor.g);
    buffer.AddFloat(section.staticColor.b);
    buffer.AddFloat(section.edgeLifetime);
    buffer.AddUint(section.staticTextureSlot);
    buffer.AddUint(section.edgesPerSecond);
    buffer.AddUint(section.textureRows);
    buffer.AddUint(section.textureCols);
    buffer.AddUint(section.materialId);
    buffer.AddFloat(section.gravity);
    WriteBinFloatKeyFrames(section.heightAbove, 'AHRK', buffer);
    WriteBinFloatKeyFrames(section.heightBelow, 'BHRK', buffer);
    WriteBinFloatKeyFrames(section.alphaKeys, 'LARK', buffer);
    if (section.colorKeys.keys.Count()) {
      buffer.AddDword('OCRK');
      buffer.AddUint(section.colorKeys.keys.Count());
      buffer.AddUint(section.colorKeys.type);
      buffer.AddUint(section.colorKeys.globalSeqId);
      UINT values = section.colorKeys.type > TRACK_LINEAR ? 9 : 3;
      for (UINT i = 0; i < section.colorKeys.keys.Count(); ++i) {
        const MDLKEYFRAME<C3Color> &key = section.colorKeys.keys.Ptr()[i];
        buffer.AddInt(key.time);
        buffer.AddFloatArray(&key.value.b, values);
      }
    }
    if (section.textureSlot.keys.Count()) {
      buffer.AddDword('XTRK');
      buffer.AddUint(section.textureSlot.keys.Count());
      buffer.AddUint(0);
      buffer.AddUint(section.textureSlot.globalSeqId);
      for (UINT i = 0; i < section.textureSlot.keys.Count(); ++i) {
        const MDLINTKEY &key = section.textureSlot.keys.Ptr()[i];
        buffer.AddInt(key.time);
        buffer.AddUint(key.value);
      }
    }
    WriteBinFloatKeyFrames(section.visibilityKeys, 'SIVK', buffer);
  }

  int WriteBinRibbonEmitters(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *status) {
    if (!static_cast<LPCSTR>(data.model.animationFile)[0] && data.ribbonEmitters.Count()) {
      buf.AddDword('BBIR');
      UINT totalSize = 4;
      UINT i;
      for (i = 0; i < data.ribbonEmitters.Count(); ++i) {
        totalSize += GetBinRibbonEmitterSize(data.ribbonEmitters.Ptr()[i]);
      }
      buf.AddUint(totalSize);
      buf.AddUint(data.ribbonEmitters.Count());
      for (i = 0; i < data.ribbonEmitters.Count(); ++i) {
        IWriteBinRibbonEmitter(data.ribbonEmitters.Ptr()[i], buf, status);
      }
    }
    return 1;
  }

  static int ReadBinRibbonEmitter(CMsgBuffer &buffer, MDLRIBBONEMITTER *ribbon, CMDLStatus *status, UINT &totalRead) {
    UINT sectionLength = buffer.GetUint();
    UINT localRead = 4;
    if (!ReadBinGenObject(*ribbon, buffer, status, localRead)) {
      status->Add(STATUS_ERROR, "Error reading gen object portion of RibbonEmitter.\n");
      return 0;
    }
    buffer.GetUint();
    localRead += 4;
    ribbon->staticHeightAbove = buffer.GetFloat();
    ribbon->staticHeightBelow = buffer.GetFloat();
    ribbon->staticAlpha = buffer.GetFloat();
    ribbon->staticColor.r = buffer.GetFloat();
    ribbon->staticColor.g = buffer.GetFloat();
    ribbon->staticColor.b = buffer.GetFloat();
    ribbon->edgeLifetime = buffer.GetFloat();
    ribbon->staticTextureSlot = buffer.GetUint();
    ribbon->edgesPerSecond = buffer.GetUint();
    ribbon->textureRows = buffer.GetUint();
    ribbon->textureCols = buffer.GetUint();
    ribbon->materialId = buffer.GetUint();
    ribbon->gravity = buffer.GetFloat();
    localRead += 52;
    while (localRead < sectionLength) {
      DWORD tag = buffer.GetDword();
      localRead += 4;
      int ok = 1;
      if (tag == 'AHRK') {
        ok = ReadBinFloatKeyFrames(ribbon->heightAbove, buffer, localRead);
        if (!ok) {
          status->Add(STATUS_ERROR, "Error reading height above portion of RibbonEmitter.\n");
        }
      } else if (tag == 'BHRK') {
        ok = ReadBinFloatKeyFrames(ribbon->heightBelow, buffer, localRead);
        if (!ok) {
          status->Add(STATUS_ERROR, "Error reading height below of RibbonEmitter.\n");
        }
      } else if (tag == 'LARK') {
        ok = ReadBinFloatKeyFrames(ribbon->alphaKeys, buffer, localRead);
        if (!ok) {
          status->Add(STATUS_ERROR, "Error reading alpha portion of RibbonEmitter.\n");
        }
      } else if (tag == 'OCRK') {
        ok = ReadBinFloatKeyFrames(ribbon->colorKeys, buffer, localRead);
        if (!ok) {
          status->Add(STATUS_ERROR, "Error reading color portion of RibbonEmitter.\n");
        }
      } else if (tag == 'XTRK') {
        ok = ReadBinUintKeyFrames(ribbon->textureSlot, buffer, localRead);
        if (!ok) {
          status->Add(STATUS_ERROR, "Error reading texture slot keys portion of RibbonEmitter.\n");
        }
      } else if (tag == 'SIVK') {
        ok = ReadBinFloatKeyFrames(ribbon->visibilityKeys, buffer, localRead);
        if (!ok) {
          status->Add(STATUS_ERROR, "Error reading visibility keys portion of RibbonEmitter.\n");
        }
      } else {
        SkipUnknown(buffer, localRead);
      }
      if (!ok) {
        return 0;
      }
    }
    if (localRead > sectionLength) {
      status->FatalOverran("RibbonEmitters", -1);
      return 0;
    }
    totalRead += localRead;
    return 1;
  }

  int ReadBinRibbonEmitters(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status) {
    UINT totalRead = 4;
    UINT numEmitters = buf.GetUint();
    data.ribbonEmitters.SetCount(0);
    data.ribbonEmitters.ReserveSpace(numEmitters);
    while (totalRead < length) {
      MDLRIBBONEMITTER *ribbon = data.ribbonEmitters.New();
      if (!ribbon) {
        status->FatalFlunked("RibbonEmitter", -1);
        return 0;
      }
      if (!ReadBinRibbonEmitter(buf, ribbon, status, totalRead)) {
        status->Add(STATUS_ERROR, "Error reading RibbonEmitter.\n");
        return 0;
      }
      if (totalRead > length) {
        status->FatalOverran("RibbonEmitter Section", -1);
        return 0;
      }
      ReadBinObjectEnd(data, ribbon, data.ribbonEmitters.Count() - 1, 0x90000000);
    }
    return 1;
  }

}  // namespace MDL
