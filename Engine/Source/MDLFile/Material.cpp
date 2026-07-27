#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

namespace MDL {
const char *TokenText(unsigned int token);
void __cdecl WriteLine(TSGrowableArray<char> &buffer, const char *format, ...);

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
  const char *tokenText;
  switch (parse.Token(&tokenText, 0)) {
    case 0x11A: return TEXOP_ADD;
    case 0x11B: return TEXOP_ADD_ALPHA;
    case 0x12E: return TEXOP_BLEND;
    case 0x172: return TEXOP_MODULATE;
    case 0x173: return TEXOP_MODULATE2X;
    case 0x179: return TEXOP_LOAD;
    case 0x1C8: return TEXOP_TRANSPARENT;
    default:
      parse.FatalUnexpected(tokenText);
      return TEXOP_LOAD;
  }
}

static int IReadAlpha(
    Parser &parse,
    int expectAnimation,
    MDLTEXLAYER *layer
) {
  if (expectAnimation) {
    ReadObjectFloatKeyframes(parse, &layer->alphaKeys);
    return 1;
  }
  layer->staticAlpha = parse.ExpectFloat();
  return 0;
}

static int IReadFlipbook(
    Parser &parse,
    int expectAnimation,
    MDLTEXLAYER *layer
) {
  if (!expectAnimation) {
    layer->textureId = parse.ExpectInt();
    return 0;
  }
  unsigned int token;
  const char *tokenText;
  UTokenData tokenData;
  long expected = parse.GetOptionalInt(&token, &tokenText, &tokenData);
  if (expected > 0) {
    layer->flipKeys.keys.Reserve(expected);
  }
  parse.Expect('{', token, tokenText);
  token = ReadIntTrackHeader(
      parse,
      &layer->flipKeys,
      &tokenText,
      &tokenData
  );
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

static int IllegalStaticToken(unsigned int token) {
  return token != 0x11C && token != 0x1C3;
}

static void IReadTexLayer(
    Parser &parse,
    MDLTEXLAYER *layer,
    CMDLStatus *status,
    unsigned int version
) {
  TSet errors;
  ITextureAddErrors(errors);
  parse.Expect('{');
  const char *tokenText;
  unsigned int token = parse.Token(&tokenText, 0);
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
        layer->coordId = static_cast<unsigned int>(-1);
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

static void IReadMaterial(
    Parser &parse,
    MDLMATERIALSECTION *material,
    CMDLStatus *status,
    unsigned int version
) {
  TSet errors;
  IMaterialAddErrors(errors);
  parse.Expect('{');
  const char *tokenText;
  unsigned int token = parse.Token(&tokenText, 0);
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

int ReadMaterials(
    Parser &parse,
    MDLDATA &data,
    CMDLStatus *status
) {
  unsigned int token;
  const char *tokenText;
  UTokenData tokenData;
  long expected = parse.GetOptionalInt(&token, &tokenText, &tokenData);
  if (expected > 0) {
    data.materials.Reserve(expected);
  }
  parse.Expect('{', token, tokenText);
  token = parse.Token(&tokenText, 0);
  long actual = 0;
  while (token == 0x16C) {
    IReadMaterial(
        parse,
        data.materials.New(),
        status,
        data.version
    );
    ++actual;
    token = parse.Token(&tokenText, 0);
  }
  parse.Expect('}', token, tokenText);
  if (expected >= 0 && actual != expected) {
    parse.WarningCount("materials", expected, actual);
  }
  return !parse.FoundError();
}

static unsigned int IGetFilterModeToken(MDLTEXOP mode) {
  switch (mode) {
    case TEXOP_LOAD: return 0x179;
    case TEXOP_TRANSPARENT: return 0x1C8;
    case TEXOP_BLEND: return 0x12E;
    case TEXOP_ADD: return 0x11A;
    case TEXOP_ADD_ALPHA: return 0x11B;
    case TEXOP_MODULATE: return 0x172;
    case TEXOP_MODULATE2X: return 0x173;
    default: return 0x1DF;
  }
}

static void IWriteTextureFlags(
    unsigned int flags,
    TSGrowableArray<char> &buffer
) {
  static const unsigned int masks[6] = {
      1, 2, 0x10, 0x20, 0x40, 0x80
  };
  static const unsigned int tokens[6] = {
      0x1D3, 0x1B5, 0x1CF, 0x1D1, 0x177, 0x178
  };
  for (unsigned int i = 0; i < 6; ++i) {
    if (flags & masks[i]) {
      WriteLine(buffer, "\t\t\t%s,\n", TokenText(tokens[i]));
    }
  }
}

static void IWriteLayer(
    const MDLTEXLAYER &layer,
    int needCoordIds,
    TSGrowableArray<char> &buffer
) {
  WriteLine(buffer, "\t\t%s {\n", TokenText(0x163));
  WriteLine(
      buffer,
      "\t\t\t%s %s,\n",
      TokenText(0x14B),
      TokenText(IGetFilterModeToken(layer.blendMode))
  );
  IWriteTextureFlags(layer.flags, buffer);
  if (layer.flipKeys.keys.Count()) {
    WriteIntKeyFrames(0x1C3, "\t\t\t", layer.flipKeys, buffer);
  } else {
    WriteLine(
        buffer,
        "\t\t\t%s %s ",
        TokenText(0x1BB),
        TokenText(0x1C3)
    );
    WriteUintKeyData(buffer, &layer.textureId, 1);
  }
  if (layer.transformId != static_cast<unsigned int>(-1)) {
    WriteLine(
        buffer,
        "\t\t\t%s %u,\n",
        TokenText(0x1CD),
        layer.transformId
    );
  }
  if (needCoordIds && !(layer.flags & 2)) {
    WriteLine(
        buffer,
        "\t\t\t%s %u,\n",
        TokenText(0x13B),
        layer.coordId
    );
  }
  if (layer.alphaKeys.keys.Count() || layer.staticAlpha < 1.0f) {
    if (layer.alphaKeys.keys.Count()) {
      WriteFloatKeyFrames(0x11C, "\t\t\t", layer.alphaKeys, buffer);
    } else {
      WriteLine(
          buffer,
          "\t\t\t%s %s ",
          TokenText(0x1BB),
          TokenText(0x11C)
      );
      WriteKeyData(buffer, &layer.staticAlpha, 1);
    }
  }
  WriteLine(buffer, "\t\t}\n");
}

static void IWriteMaterial(
    const MDLMATERIALSECTION &material,
    TSGrowableArray<char> &buffer
) {
  WriteLine(buffer, "\t%s {\n", TokenText(0x16C));
  if (material.priorityPlane) {
    WriteLine(
        buffer,
        "\t\t%s %d,\n",
        TokenText(0x1A6),
        material.priorityPlane
    );
  }
  int needCoordIds = 0;
  for (unsigned int i = 0; i < material.texLayers.Count(); ++i) {
    const MDLTEXLAYER &layer = material.texLayers.Ptr()[i];
    if (!(layer.flags & 2) && layer.coordId) {
      needCoordIds = 1;
      break;
    }
  }
  for (unsigned int j = 0; j < material.texLayers.Count(); ++j) {
    IWriteLayer(material.texLayers.Ptr()[j], needCoordIds, buffer);
  }
  WriteLine(buffer, "\t}\n");
}

int WriteMaterials(
    const MDLDATA &data,
    TSGrowableArray<char> &buffer,
    CMDLStatus *
) {
  if (data.materials.Count()) {
    WriteLine(
        buffer,
        "%s %d {\n",
        TokenText(0x109),
        data.materials.Count()
    );
    for (unsigned int i = 0; i < data.materials.Count(); ++i) {
      IWriteMaterial(data.materials.Ptr()[i], buffer);
    }
    WriteLine(buffer, "}\n");
  }
  return 1;
}

static int ReadBinLayer(
    CMsgBuffer &buffer,
    CMDLStatus *status,
    unsigned int *bytesRead,
    MDLTEXLAYER *layer
) {
  unsigned int sectionLength = buffer.GetUint();
  layer->blendMode = static_cast<MDLTEXOP>(buffer.GetUint());
  layer->flags = buffer.GetUint();
  layer->textureId = buffer.GetUint();
  layer->transformId = buffer.GetUint();
  layer->coordId = buffer.GetUint();
  layer->staticAlpha = buffer.GetFloat();
  unsigned int localRead = 28;
  while (localRead < sectionLength) {
    unsigned long tag = buffer.GetDword();
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

static int ReadBinMaterial(
    CMsgBuffer &buffer,
    unsigned int sectionLength,
    MDLMATERIALSECTION *material,
    CMDLStatus *status,
    unsigned int &bytesRead
) {
  unsigned int localRead = 4;
  material->priorityPlane = buffer.GetInt();
  unsigned int numLayers = buffer.GetUint();
  localRead += 8;
  material->texLayers.SetCount(numLayers);
  for (unsigned int i = 0; i < numLayers; ++i) {
    if (!ReadBinLayer(
        buffer,
        status,
        &localRead,
        &material->texLayers.Ptr()[i]
    )) {
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

int ReadBinMaterials(
    CMsgBuffer &buffer,
    unsigned int length,
    MDLDATA &data,
    CMDLStatus *status
) {
  unsigned int numMaterials = buffer.GetUint();
  buffer.GetUint();
  unsigned int bytesRead = 8;
  data.materials.SetCount(0);
  data.materials.Reserve(numMaterials);
  while (bytesRead < length) {
    MDLMATERIALSECTION *material = data.materials.New();
    unsigned int sectionLength = buffer.GetUint();
    if (!ReadBinMaterial(
        buffer,
        sectionLength,
        material,
        status,
        bytesRead
    )) {
      return 0;
    }
    if (bytesRead > length) {
      status->FatalOverran("MaterialSection", -1);
      return 0;
    }
  }
  return 1;
}

static unsigned int GetLayerSize(const MDLTEXLAYER &layer) {
  unsigned int size = 28;
  if (layer.alphaKeys.keys.Count()) {
    unsigned int dataSize =
        layer.alphaKeys.type > TRACK_LINEAR ? 12 : 4;
    size += 16 + layer.alphaKeys.keys.Count() * (4 + dataSize);
  }
  if (layer.flipKeys.keys.Count()) {
    size += 16 + layer.flipKeys.keys.Count() * 8;
  }
  return size;
}

static unsigned int GetMaterialSize(
    const MDLMATERIALSECTION &material
) {
  if (!material.texLayers.Count()) {
    return 8;
  }
  unsigned int size = 12;
  for (unsigned int i = 0; i < material.texLayers.Count(); ++i) {
    size += GetLayerSize(material.texLayers.Ptr()[i]);
  }
  return size;
}

static void AddLayers(
    CMsgBuffer &buffer,
    const MDLMATERIALSECTION &material
) {
  buffer.AddUint(material.texLayers.Count());
  for (unsigned int i = 0; i < material.texLayers.Count(); ++i) {
    const MDLTEXLAYER &layer = material.texLayers.Ptr()[i];
    buffer.AddUint(GetLayerSize(layer));
    buffer.AddUint(layer.blendMode);
    buffer.AddUint(layer.flags);
    buffer.AddUint(layer.textureId);
    buffer.AddUint(layer.transformId);
    buffer.AddUint(layer.flags & 2
        ? static_cast<unsigned int>(-1)
        : layer.coordId);
    buffer.AddFloat(layer.staticAlpha);
    WriteBinFloatKeyFrames(layer.alphaKeys, 'ATMK', buffer);
    WriteBinUintKeyFrames(layer.flipKeys, 'FTMK', buffer);
  }
}

int WriteBinMaterials(
    const MDLDATA &data,
    CMsgBuffer &buffer,
    CMDLStatus *
) {
  if (data.materials.Count()) {
    buffer.AddDword('SLTM');
    unsigned int size = 8;
    unsigned int i;
    for (i = 0; i < data.materials.Count(); ++i) {
      size += GetMaterialSize(data.materials.Ptr()[i]);
    }
    buffer.AddUint(size);
    buffer.AddUint(data.materials.Count());
    unsigned int animatedLayers = 0;
    for (i = 0; i < data.materials.Count(); ++i) {
      const MDLMATERIALSECTION &material = data.materials.Ptr()[i];
      for (unsigned int j = 0; j < material.texLayers.Count(); ++j) {
        const MDLTEXLAYER &layer = material.texLayers.Ptr()[j];
        if (layer.alphaKeys.keys.Count() || layer.flipKeys.keys.Count()) {
          ++animatedLayers;
        }
      }
    }
    buffer.AddUint(animatedLayers);
    for (i = 0; i < data.materials.Count(); ++i) {
      const MDLMATERIALSECTION &material = data.materials.Ptr()[i];
      buffer.AddUint(GetMaterialSize(material));
      buffer.AddInt(material.priorityPlane);
      AddLayers(buffer, material);
    }
  }
  return 1;
}

} // namespace MDL
