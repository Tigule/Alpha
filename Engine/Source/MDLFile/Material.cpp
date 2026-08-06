#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

namespace MDL {
  LPCSTR       TokenText(UINT token);
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);

  static void IMaterialAddErrors(TSet &errors) {
    errors.Add(0x163, 1, 1);
    errors.Add(0x13A, 0, 0);
    errors.Add(0x1B8, 0, 0);
    errors.Add(0x1B7, 0, 0);
    errors.Add(0x1A6, 0, 0);
  }

  static void ITextureAddErrors(TSet &errors) {
    errors.Add(0x14B, 0, 0);
    errors.Add(0x1D3, 0, 0);
    errors.Add(0x1C3, 1, 0);
    errors.Add(0x1CD, 0, 0);
    errors.Add(0x13B, 0, 0);
    errors.Add(0x1B5, 0, 0);
    errors.Add(0x1CF, 0, 0);
    errors.Add(0x1D1, 0, 0);
    errors.Add(0x177, 0, 0);
    errors.Add(0x178, 0, 0);
  }

  static MDLTEXOP IReadFilterMode(Parser &parse) {
    LPCSTR tokenText;
    switch (parse.Token(&tokenText, 0)) {
      case 0x11A:
        return TEXOP_ADD;
      case 0x11B:
        return TEXOP_ADD_ALPHA;
      case 0x12E:
        return TEXOP_BLEND;
      case 0x172:
        return TEXOP_MODULATE;
      case 0x173:
        return TEXOP_MODULATE2X;
      case 0x179:
        return TEXOP_LOAD;
      case 0x1C8:
        return TEXOP_TRANSPARENT;
      default:
        parse.FatalUnexpected(tokenText);
        return TEXOP_LOAD;
    }
  }

  static int IReadAlpha(Parser &parse, int expectAnimation, MDLTEXLAYER *layer) {
    if (expectAnimation) {
      ReadObjectFloatKeyframes(parse, &layer->alphaKeys);
      return 1;
    }
    layer->staticAlpha = parse.ExpectFloat();
    return 0;
  }

  static int IReadFlipbook(Parser &parse, int expectAnimation, MDLTEXLAYER *layer) {
    if (!expectAnimation) {
      layer->textureId = parse.ExpectInt();
      return 0;
    }
    UINT       token;
    LPCSTR     tokenText;
    UTokenData tokenData;
    long       expected = parse.GetOptionalInt(&token, &tokenText, &tokenData);
    if (expected > 0) {
      layer->flipKeys.keys.ReserveSpace(expected);
    }
    parse.Expect('{', token, tokenText);
    token = ReadIntTrackHeader(parse, &layer->flipKeys, &tokenText, &tokenData);
    long actual = 0;
    while (token == 0x100) {
      MDLINTKEY *key = layer->flipKeys.keys.New();
      key->time = tokenData.lVal;
      parse.Expect(':');
      key->value = parse.ExpectInt();
      parse.Expect(',');
      ++actual;
      token = parse.Token(&tokenText, &tokenData);
    }
    parse.Expect('}', token, tokenText);
    if (expected >= 0 && actual != expected) {
      parse.WarningCount("key frames", expected, actual);
    }
    return 1;
  }

  static int IllegalStaticToken(UINT token) {
    return token != 0x11C && token != 0x1C3;
  }

  static void IReadTexLayer(Parser &parse, MDLTEXLAYER *layer, CMDLStatus *status, UINT version) {
    TSet errors;
    ITextureAddErrors(errors);
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
      int consumedBlock = 0;
      switch (token) {
        case 0x11C:
          consumedBlock = IReadAlpha(parse, expectAnimation, layer);
          break;
        case 0x13B:
          layer->coordId = parse.ExpectInt();
          break;
        case 0x14B:
          layer->blendMode = IReadFilterMode(parse);
          break;
        case 0x177:
          layer->flags |= 0x40;
          break;
        case 0x178:
          layer->flags |= 0x80;
          break;
        case 0x1B5:
          layer->flags |= 2;
          layer->coordId = static_cast<UINT>(-1);
          break;
        case 0x1C3:
          if (version >= 800) {
            consumedBlock = IReadFlipbook(parse, expectAnimation, layer);
          } else {
            layer->textureId = parse.ExpectInt();
          }
          break;
        case 0x1CD:
          layer->transformId = parse.ExpectInt();
          break;
        case 0x1CF:
          layer->flags |= 0x10;
          break;
        case 0x1D1:
          layer->flags |= 0x20;
          break;
        case 0x1D3:
          layer->flags |= 1;
          break;
        case 0x1DC:
          layer->flags |= 8;
          break;
        case 0x1DD:
          layer->flags |= 4;
          break;
        default:
          parse.FatalUnexpected(tokenText);
          break;
      }
      if (!consumedBlock) {
        parse.Expect(',');
      }
      token = parse.Token(&tokenText, 0);
    }
    parse.Expect('}', token, tokenText);
    errors.Complete(status);
  }

  static void IReadMaterial(Parser &parse, MDLMATERIALSECTION *material, CMDLStatus *status, UINT version) {
    TSet errors;
    IMaterialAddErrors(errors);
    parse.Expect('{');
    LPCSTR tokenText;
    UINT   token = parse.Token(&tokenText, 0);
    while (token && token != '}') {
      if (!errors.Check(token)) {
        parse.FatalDuplicate(tokenText);
      }
      switch (token) {
        case 0x163:
          IReadTexLayer(parse, material->texLayers.New(), status, version);
          break;
        case 0x1A6:
          material->priorityPlane = parse.ExpectInt();
          parse.Expect(',');
          break;
        case 0x13A:
        case 0x14F:
        case 0x1B7:
        case 0x1B8:
        case 0x1CF:
        case 0x1D1:
          parse.Expect(',');
          break;
        default:
          parse.FatalUnexpected(tokenText);
          parse.Expect(',');
          break;
      }
      token = parse.Token(&tokenText, 0);
    }
    parse.Expect('}', token, tokenText);
    errors.Complete(status);
  }

  int ReadMaterials(Parser &parse, MDLDATA &data, CMDLStatus *status) {
    UINT   savedtoken;
    LPCSTR tokentext;
    long   actual = 0;
    long   count = parse.GetOptionalInt(&savedtoken, &tokentext, 0);
    if (count > 0) {
      data.materials.ReserveSpace(count);
    }
    parse.Expect('{', savedtoken, tokentext);
    savedtoken = parse.Token(&tokentext, 0);
    while (savedtoken == 0x16C) {
      IReadMaterial(parse, data.materials.New(), status, data.version);
      ++actual;
      savedtoken = parse.Token(&tokentext, 0);
    }
    parse.Expect('}', savedtoken, tokentext);
    if (count >= 0 && actual != count) {
      parse.WarningCount("materials", count, actual);
    }
    return !parse.FoundError();
  }

  static UINT IGetFilterModeToken(MDLTEXOP mode) {
    switch (mode) {
      case TEXOP_LOAD:
        return 0x179;
      case TEXOP_TRANSPARENT:
        return 0x1C8;
      case TEXOP_BLEND:
        return 0x12E;
      case TEXOP_ADD:
        return 0x11A;
      case TEXOP_ADD_ALPHA:
        return 0x11B;
      case TEXOP_MODULATE:
        return 0x172;
      case TEXOP_MODULATE2X:
        return 0x173;
      default:
        return 0x1DF;
    }
  }

  static void IWriteTextureFlags(UINT flags, TSGrowableArray<char> &buffer) {
    static const UINT masks[6] = {1, 2, 0x10, 0x20, 0x40, 0x80};
    static const UINT tokens[6] = {0x1D3, 0x1B5, 0x1CF, 0x1D1, 0x177, 0x178};
    for (UINT i = 0; i < 6; ++i) {
      if (flags & masks[i]) {
        WriteLine(buffer, "\t\t\t%s,\n", TokenText(tokens[i]));
      }
    }
  }

  static void IWriteLayer(const MDLTEXLAYER &layer, int needCoordIds, TSGrowableArray<char> &buffer) {
    WriteLine(buffer, "\t\t%s {\n", TokenText(0x163));
    WriteLine(buffer, "\t\t\t%s %s,\n", TokenText(0x14B), TokenText(IGetFilterModeToken(layer.blendMode)));
    IWriteTextureFlags(layer.flags, buffer);
    if (layer.flipKeys.keys.Count()) {
      WriteIntKeyFrames(0x1C3, "\t\t\t", layer.flipKeys, buffer);
    } else {
      WriteLine(buffer, "\t\t\t%s %s ", TokenText(0x1BB), TokenText(0x1C3));
      WriteUintKeyData(buffer, &layer.textureId, 1);
    }
    if (layer.transformId != static_cast<UINT>(-1)) {
      WriteLine(buffer, "\t\t\t%s %u,\n", TokenText(0x1CD), layer.transformId);
    }
    if (needCoordIds && !(layer.flags & 2)) {
      WriteLine(buffer, "\t\t\t%s %u,\n", TokenText(0x13B), layer.coordId);
    }
    if (layer.alphaKeys.keys.Count() || layer.staticAlpha < 1.0f) {
      if (layer.alphaKeys.keys.Count()) {
        WriteFloatKeyFrames(0x11C, "\t\t\t", layer.alphaKeys, buffer);
      } else {
        WriteLine(buffer, "\t\t\t%s %s ", TokenText(0x1BB), TokenText(0x11C));
        WriteKeyData(buffer, &layer.staticAlpha, 1);
      }
    }
    WriteLine(buffer, "\t\t}\n");
  }

  static void IWriteMaterial(const MDLMATERIALSECTION &material, TSGrowableArray<char> &buffer) {
    WriteLine(buffer, "\t%s {\n", TokenText(0x16C));
    if (material.priorityPlane) {
      WriteLine(buffer, "\t\t%s %d,\n", TokenText(0x1A6), material.priorityPlane);
    }
    int needCoordIds = 0;
    for (UINT i = 0; i < material.texLayers.Count(); ++i) {
      const MDLTEXLAYER &layer = material.texLayers.Ptr()[i];
      if (!(layer.flags & 2) && layer.coordId) {
        needCoordIds = 1;
        break;
      }
    }
    for (UINT j = 0; j < material.texLayers.Count(); ++j) {
      IWriteLayer(material.texLayers.Ptr()[j], needCoordIds, buffer);
    }
    WriteLine(buffer, "\t}\n");
  }

  int WriteMaterials(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
    UINT numMaterials = data.materials.Count();
    if (numMaterials) {
      WriteLine(buffer, "%s %d {\n", TokenText(0x109), numMaterials);
      for (UINT i = 0; i < numMaterials; ++i) {
        IWriteMaterial(data.materials[i], buffer);
      }
      WriteLine(buffer, "}\n");
    }
    return 1;
  }

  static int ReadBinLayer(CMsgBuffer &buffer, CMDLStatus *status, UINT *bytesRead, MDLTEXLAYER *layer) {
    UINT sectionLength = buffer.GetUint();
    layer->blendMode = static_cast<MDLTEXOP>(buffer.GetUint());
    layer->flags = buffer.GetUint();
    layer->textureId = buffer.GetUint();
    layer->transformId = buffer.GetUint();
    layer->coordId = buffer.GetUint();
    layer->staticAlpha = buffer.GetFloat();
    UINT localRead = 28;
    while (localRead < sectionLength) {
      DWORD tag = buffer.GetDword();
      localRead += 4;
      if (tag == 'ATMK') {
        ReadBinFloatKeyFrames(layer->alphaKeys, buffer, localRead);
      } else if (tag == 'FTMK') {
        ReadBinUintKeyFrames(layer->flipKeys, buffer, localRead);
      } else {
        SkipUnknown(buffer, localRead);
      }
    }
    if (localRead > sectionLength) {
      status->FatalOverran("TexLayer", -1);
      return 0;
    }
    *bytesRead += localRead;
    return 1;
  }

  static int ReadBinMaterial(CMsgBuffer &buffer, UINT sectionLength, MDLMATERIALSECTION *material, CMDLStatus *status, UINT &bytesRead) {
    UINT localRead = 4;
    material->priorityPlane = buffer.GetInt();
    UINT numLayers = buffer.GetUint();
    localRead += 8;
    material->texLayers.SetCount(numLayers);
    for (UINT i = 0; i < numLayers; ++i) {
      if (!ReadBinLayer(buffer, status, &localRead, &material->texLayers.Ptr()[i])) {
        return 0;
      }
    }
    if (localRead > sectionLength) {
      status->FatalOverran("Material", -1);
      return 0;
    }
    bytesRead += localRead;
    return 1;
  }

  int ReadBinMaterials(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status) {
    UINT bytesRead = 8;
    UINT numMaterials = buf.GetUint();
    buf.GetUint();
    data.materials.SetCount(0);
    data.materials.ReserveSpace(numMaterials);
    while (bytesRead < length) {
      MDLMATERIALSECTION *material = data.materials.New();
      UINT                sectionLength = buf.GetUint();
      if (!ReadBinMaterial(buf, sectionLength, material, status, bytesRead)) {
        return 0;
      }
      if (bytesRead > length) {
        status->FatalOverran("MaterialSection", -1);
        return 0;
      }
    }
    return 1;
  }

  static UINT GetLayerSize(const MDLTEXLAYER &layer) {
    UINT size = 28;
    if (layer.alphaKeys.keys.Count()) {
      UINT dataSize = layer.alphaKeys.type > TRACK_LINEAR ? 12 : 4;
      size += 16 + layer.alphaKeys.keys.Count() * (4 + dataSize);
    }
    if (layer.flipKeys.keys.Count()) {
      size += 16 + layer.flipKeys.keys.Count() * 8;
    }
    return size;
  }

  static UINT GetMaterialSize(const MDLMATERIALSECTION &material) {
    if (!material.texLayers.Count()) {
      return 8;
    }
    UINT size = 12;
    for (UINT i = 0; i < material.texLayers.Count(); ++i) {
      size += GetLayerSize(material.texLayers.Ptr()[i]);
    }
    return size;
  }

  static void AddLayers(CMsgBuffer &buffer, const MDLMATERIALSECTION &material) {
    buffer.AddUint(material.texLayers.Count());
    for (UINT i = 0; i < material.texLayers.Count(); ++i) {
      const MDLTEXLAYER &layer = material.texLayers.Ptr()[i];
      buffer.AddUint(GetLayerSize(layer));
      buffer.AddUint(layer.blendMode);
      buffer.AddUint(layer.flags);
      buffer.AddUint(layer.textureId);
      buffer.AddUint(layer.transformId);
      buffer.AddUint(layer.flags & 2 ? static_cast<UINT>(-1) : layer.coordId);
      buffer.AddFloat(layer.staticAlpha);
      WriteBinFloatKeyFrames(layer.alphaKeys, 'ATMK', buffer);
      WriteBinUintKeyFrames(layer.flipKeys, 'FTMK', buffer);
    }
  }

  int WriteBinMaterials(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *) {
    UINT numLayers;
    UINT numMaterials = data.materials.Count();
    UINT animatedLayers;
    UINT i;
    UINT size;
    if (numMaterials) {
      buf.AddDword('SLTM');
      size = 8;
      for (i = 0; i < numMaterials; ++i) {
        size += GetMaterialSize(data.materials[i]);
      }
      buf.AddUint(size);
      buf.AddUint(numMaterials);
      animatedLayers = 0;
      for (i = 0; i < numMaterials; ++i) {
        numLayers = data.materials[i].texLayers.Count();
        for (UINT j = 0; j < numLayers; ++j) {
          if (data.materials[i].texLayers[j].alphaKeys.keys.Count() || data.materials[i].texLayers[j].flipKeys.keys.Count()) {
            ++animatedLayers;
          }
        }
      }
      buf.AddUint(animatedLayers);
      for (i = 0; i < numMaterials; ++i) {
        buf.AddUint(GetMaterialSize(data.materials[i]));
        buf.AddInt(data.materials[i].priorityPlane);
        AddLayers(buf, data.materials[i]);
      }
    }
    return 1;
  }

}  // namespace MDL
