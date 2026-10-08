#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

#include <math.h>


namespace MDL {
  LPCSTR       TokenText(UINT token);
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);
  BOOL         ReadRibbonEmitter(Parser &, MDLDATA &, CMDLStatus *);
  BOOL         WriteRibbonEmitters(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL         WriteBinRibbonEmitters(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  BOOL         ReadBinRibbonEmitters(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
}  // namespace MDL

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

static void IReadRibbonEmitterKeyFrames(Parser &parse, UINT savedtoken, LPCSTR tokentext, MDLRIBBONEMITTER *emitter) {
  switch (savedtoken) {
    case 0x159:
      ReadObjectFloatKeyframes(parse, &emitter->heightAbove);
      return;
    case 0x15A:
      ReadObjectFloatKeyframes(parse, &emitter->heightBelow);
      return;
    case 0x11C:
      ReadObjectFloatKeyframes(parse, &emitter->alphaKeys);
      return;
    case 0x136:
      ReadObjectFloatKeyframes(parse, &emitter->colorKeys);
      return;
    case 0x1D9:
      ReadObjectFloatKeyframes(parse, &emitter->visibilityKeys);
      return;
    case 0x1C4: {
      UINT       token;
      LPCSTR     text;
      UTokenData tokenData;
      long       actual = 0;
      long       expected = parse.GetOptionalInt(&token, &text, &tokenData);
      if (expected > 0) {
        emitter->textureSlot.keys.ReserveSpace(expected);
      }
      parse.Expect('{', token, text);
      token = ReadIntTrackHeader(parse, &emitter->textureSlot, &text, &tokenData);
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
      return;
    }
    case 0x144:
      emitter->edgesPerSecond = parse.ExpectInt();
      break;
    case 0x165:
      emitter->edgeLifetime = parse.ExpectFloat();
      break;
    case 0x153:
      emitter->gravity = parse.ExpectFloat();
      break;
    case 0x1AE:
      emitter->textureRows = parse.ExpectInt();
      break;
    case 0x137:
      emitter->textureCols = parse.ExpectInt();
      break;
    case 0x16D:
      emitter->materialId = parse.ExpectInt();
      break;
    default:
      parse.FatalUnexpected(tokentext);
      return;
  }
  parse.Expect(',');
}

static void IReadRibbonEmitterStaticData(Parser &parse, UINT savedToken, LPCSTR tokenText, MDLRIBBONEMITTER *emitter) {
  switch (savedToken) {
    case 0x159:
      ReadFloatKeyData(parse, &emitter->staticHeightAbove, 1);
      break;
    case 0x15A:
      ReadFloatKeyData(parse, &emitter->staticHeightBelow, 1);
      break;
    case 0x11C:
      ReadFloatKeyData(parse, &emitter->staticAlpha, 1);
      break;
    case 0x136:
      ReadFloatKeyData(parse, &emitter->staticColor.b, 3);
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
  while (token != '}' && token) {
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

BOOL MDL::ReadRibbonEmitter(Parser &parse, MDLDATA &data, CMDLStatus *status) {
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
    MDL::WriteLine(buffer, "%s%s %s ", "\t", MDL::TokenText(0x1BB), MDL::TokenText(0x159));
    WriteKeyData(buffer, &emitter.staticHeightAbove, 1);
  }
  if (emitter.heightBelow.keys.Count()) {
    WriteFloatKeyFrames(0x15A, "\t", emitter.heightBelow, buffer);
  } else {
    MDL::WriteLine(buffer, "%s%s %s ", "\t", MDL::TokenText(0x1BB), MDL::TokenText(0x15A));
    WriteKeyData(buffer, &emitter.staticHeightBelow, 1);
  }
  if (emitter.alphaKeys.keys.Count()) {
    WriteFloatKeyFrames(0x11C, "\t", emitter.alphaKeys, buffer);
  } else {
    MDL::WriteLine(buffer, "%s%s %s ", "\t", MDL::TokenText(0x1BB), MDL::TokenText(0x11C));
    WriteKeyData(buffer, &emitter.staticAlpha, 1);
  }
  if (emitter.colorKeys.keys.Count()) {
    WriteFloatKeyFrames(0x136, "\t", emitter.colorKeys, buffer);
  } else {
    MDL::WriteLine(buffer, "%s%s %s ", "\t", MDL::TokenText(0x1BB), MDL::TokenText(0x136));
    WriteKeyData(buffer, &emitter.staticColor.b, 3);
  }
  if (emitter.textureSlot.keys.Count()) {
    WriteIntKeyFrames(0x1C4, "\t", emitter.textureSlot, buffer);
  } else {
    MDL::WriteLine(buffer, "%s%s %s ", "\t", MDL::TokenText(0x1BB), MDL::TokenText(0x1C4));
    WriteUintKeyData(buffer, &emitter.staticTextureSlot, 1);
  }
  WriteFloatKeyFrames(0x1D9, "\t", emitter.visibilityKeys, buffer);
  MDL::WriteLine(buffer, "\t%s %u,\n", MDL::TokenText(0x144), emitter.edgesPerSecond);
  MDL::WriteLine(buffer, "\t%s %g,\n", MDL::TokenText(0x165), emitter.edgeLifetime);
  if (fabs(emitter.gravity) >= 2.3841858e-7f) {
    MDL::WriteLine(buffer, "\t%s %g,\n", MDL::TokenText(0x153), emitter.gravity);
  }
  MDL::WriteLine(buffer, "\t%s %u,\n", MDL::TokenText(0x1AE), emitter.textureRows);
  MDL::WriteLine(buffer, "\t%s %u,\n", MDL::TokenText(0x137), emitter.textureCols);
  MDL::WriteLine(buffer, "\t%s %u,\n", MDL::TokenText(0x16D), emitter.materialId);
  WriteObjectTrailer(emitter, buffer);
}

BOOL MDL::WriteRibbonEmitters(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
  if (!data.model.animationFile[0]) {
    UINT numEmitters = data.ribbonEmitters.Count();
    int needObjIds = numEmitters != data.objects.Count();
    const MDLRIBBONEMITTER *ribbon = data.ribbonEmitters.Ptr();
    for (UINT i = numEmitters; i; --i, ++ribbon) {
      IWriteRibbonEmitter(data, *ribbon, needObjIds, buffer);
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
  WriteBinFloatKeyFrames(section.colorKeys, 'OCRK', buffer);
  WriteBinUintKeyFrames(section.textureSlot, 'XTRK', buffer);
  WriteBinFloatKeyFrames(section.visibilityKeys, 'SIVK', buffer);
}

BOOL MDL::WriteBinRibbonEmitters(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *status) {
  UINT numEmitters = data.ribbonEmitters.Count();
  if (!data.model.animationFile[0] && numEmitters) {
    buf.AddDword('BBIR');
    UINT totalSize = 4;
    UINT i;
    for (i = 0; i < numEmitters; ++i) {
      totalSize += GetBinRibbonEmitterSize(data.ribbonEmitters[i]);
    }
    buf.AddUint(totalSize);
    buf.AddUint(numEmitters);
    for (i = 0; i < numEmitters; ++i) {
      IWriteBinRibbonEmitter(data.ribbonEmitters[i], buf, status);
    }
  }
  return 1;
}

static BOOL ReadBinRibbonEmitter(CMsgBuffer &buf, MDLRIBBONEMITTER *ribbon, CMDLStatus *status, UINT &totalRead) {
  UINT sectionLength = buf.GetUint();
  UINT localBytesRead = 4;
  if (!ReadBinGenObject(*ribbon, buf, status, localBytesRead)) {
    status->Add(STATUS_ERROR, "Error reading gen object portion of RibbonEmitter.\n");
    return 0;
  }
  buf.GetUint();
  localBytesRead += 4;
  ribbon->staticHeightAbove = buf.GetFloat();
  localBytesRead += 4;
  ribbon->staticHeightBelow = buf.GetFloat();
  localBytesRead += 4;
  ribbon->staticAlpha = buf.GetFloat();
  localBytesRead += 4;
  ribbon->staticColor.r = buf.GetFloat();
  localBytesRead += 4;
  ribbon->staticColor.g = buf.GetFloat();
  localBytesRead += 4;
  ribbon->staticColor.b = buf.GetFloat();
  localBytesRead += 4;
  ribbon->edgeLifetime = buf.GetFloat();
  localBytesRead += 4;
  ribbon->staticTextureSlot = buf.GetUint();
  localBytesRead += 4;
  ribbon->edgesPerSecond = buf.GetUint();
  localBytesRead += 4;
  ribbon->textureRows = buf.GetUint();
  localBytesRead += 4;
  ribbon->textureCols = buf.GetUint();
  localBytesRead += 4;
  ribbon->materialId = buf.GetUint();
  localBytesRead += 4;
  ribbon->gravity = buf.GetFloat();
  localBytesRead += 4;
  while (localBytesRead < sectionLength) {
    DWORD tag = buf.GetDword();
    localBytesRead += 4;
    switch (tag) {
      case 'AHRK':
        if (!ReadBinFloatKeyFrames(ribbon->heightAbove, buf, localBytesRead)) {
          status->Add(STATUS_ERROR, "Error reading height above portion of RibbonEmitter.\n");
          return 0;
        }
        break;
      case 'BHRK':
        if (!ReadBinFloatKeyFrames(ribbon->heightBelow, buf, localBytesRead)) {
          status->Add(STATUS_ERROR, "Error reading height below of RibbonEmitter.\n");
          return 0;
        }
        break;
      case 'LARK':
        if (!ReadBinFloatKeyFrames(ribbon->alphaKeys, buf, localBytesRead)) {
          status->Add(STATUS_ERROR, "Error reading alpha portion of RibbonEmitter.\n");
          return 0;
        }
        break;
      case 'OCRK':
        if (!ReadBinFloatKeyFrames(ribbon->colorKeys, buf, localBytesRead)) {
          status->Add(STATUS_ERROR, "Error reading color portion of RibbonEmitter.\n");
          return 0;
        }
        break;
      case 'XTRK':
        if (!ReadBinUintKeyFrames(ribbon->textureSlot, buf, localBytesRead)) {
          status->Add(STATUS_ERROR, "Error reading texture slot keys portion of RibbonEmitter.\n");
          return 0;
        }
        break;
      case 'SIVK':
        if (!ReadBinFloatKeyFrames(ribbon->visibilityKeys, buf, localBytesRead)) {
          status->Add(STATUS_ERROR, "Error reading visibility keys portion of RibbonEmitter.\n");
          return 0;
        }
        break;
      default:
        SkipUnknown(buf, localBytesRead);
        break;
    }
    if (localBytesRead > sectionLength) {
      status->FatalOverran("RibbonEmitters", -1);
      return 0;
    }
  }
  totalRead += localBytesRead;
  return 1;
}

BOOL MDL::ReadBinRibbonEmitters(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status) {
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
