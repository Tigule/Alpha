#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"


namespace MDL {
  LPCSTR       TokenText(UINT token);
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);
  BOOL         ReadMaterials(Parser &, MDLDATA &, CMDLStatus *);
  BOOL         WriteMaterials(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL         ReadBinMaterials(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  BOOL         WriteBinMaterials(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
}  // namespace MDL

struct TOKENFLAG {
  UINT mask;
  UINT token;
};

static TOKENFLAG s_textureFlags[6] = {
    { 0x1, MDLTOK_UNSHADED},
    { 0x2, MDLTOK_SPHERE_ENV_MAP},
    {0x10, MDLTOK_TWO_SIDED},
    {0x20, MDLTOK_UNFOGGED},
    {0x40, MDLTOK_NO_DEPTH_TEST},
    {0x80, MDLTOK_NO_DEPTH_SET}
};

static void IMaterialAddErrors(TSet &errors) {
  errors.Add(MDLTOK_LAYER, 1, 1);
  errors.Add(MDLTOK_CONSTANTCOLOR, 0, 0);
  errors.Add(MDLTOK_SORTPRIMSNEARZ, 0, 0);
  errors.Add(MDLTOK_SORTPRIMSFARZ, 0, 0);
  errors.Add(MDLTOK_PRIORITYPLANE, 0, 0);
}

static void ITextureAddErrors(TSet &errors) {
  errors.Add(MDLTOK_FILTERMODE, 0, 0);
  errors.Add(MDLTOK_UNSHADED, 0, 0);
  errors.Add(MDLTOK_TEXTURE_ID, 1, 0);
  errors.Add(MDLTOK_TVERTEXANIMID, 0, 0);
  errors.Add(MDLTOK_COORD_ID, 0, 0);
  errors.Add(MDLTOK_SPHERE_ENV_MAP, 0, 0);
  errors.Add(MDLTOK_TWO_SIDED, 0, 0);
  errors.Add(MDLTOK_UNFOGGED, 0, 0);
  errors.Add(MDLTOK_NO_DEPTH_TEST, 0, 0);
  errors.Add(MDLTOK_NO_DEPTH_SET, 0, 0);
}

static MDLTEXOP IReadFilterMode(Parser &parse) {
  LPCSTR tokenText;
  switch (parse.Token(&tokenText, 0)) {
    case MDLTOK_TRANSPARENT:
      return TEXOP_TRANSPARENT;
    case MDLTOK_BLEND:
      return TEXOP_BLEND;
    case MDLTOK_ADDITIVE:
      return TEXOP_ADD;
    case MDLTOK_ADD_ALPHA:
      return TEXOP_ADD_ALPHA;
    case MDLTOK_MODULATE:
      return TEXOP_MODULATE;
    case MDLTOK_MODULATE2X:
      return TEXOP_MODULATE2X;
    case MDLTOK_CAPNONE:
      return TEXOP_LOAD;
    default:
      parse.FatalUnexpected(tokenText);
      return TEXOP_LOAD;
  }
}

static BOOL IReadAlpha(Parser &parse, int expectanimation, MDLTEXLAYER *layer) {
  if (!expectanimation) {
    layer->staticAlpha = parse.ExpectFloat();
    return 0;
  }
  ReadObjectFloatKeyframes(parse, &layer->alphaKeys);
  return 1;
}

static BOOL IReadFlipbook(Parser &parse, int expectanimation, MDLTEXLAYER *layer) {
  if (!expectanimation) {
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
  while (token == MDLTOK_LONG) {
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

static BOOL IllegalStaticToken(UINT token) {
  return token != MDLTOK_ALPHA && token != MDLTOK_TEXTURE_ID;
}

static void IReadTexLayer(Parser &parse, MDLTEXLAYER *layer, CMDLStatus *status, UINT version) {
  TSet errors;
  ITextureAddErrors(errors);
  parse.Expect('{');
  LPCSTR tokentext;
  UINT   savedtoken;
  for (savedtoken = parse.Token(&tokentext, 0); savedtoken != '}' && savedtoken; savedtoken = parse.Token(&tokentext, 0)) {
    int expectanimation = IExpectAnimation(parse, &savedtoken, &tokentext);
    if (!expectanimation && IllegalStaticToken(savedtoken)) {
      parse.FatalUnexpected(tokentext);
    }
    if (!errors.Check(savedtoken)) {
      parse.FatalDuplicate(tokentext);
    }
    switch (savedtoken) {
      case MDLTOK_FILTERMODE:
        layer->blendMode = IReadFilterMode(parse);
        break;
      case MDLTOK_SPHERE_ENV_MAP:
        layer->flags |= 2;
        layer->coordId = -1;
        break;
      case MDLTOK_UNSHADED:
        layer->flags |= 1;
        break;
      case MDLTOK_WRAPWIDTH:
        layer->flags |= 4;
        break;
      case MDLTOK_WRAPHEIGHT:
        layer->flags |= 8;
        break;
      case MDLTOK_TWO_SIDED:
        layer->flags |= 0x10;
        break;
      case MDLTOK_UNFOGGED:
        layer->flags |= 0x20;
        break;
      case MDLTOK_NO_DEPTH_TEST:
        layer->flags |= 0x40;
        break;
      case MDLTOK_NO_DEPTH_SET:
        layer->flags |= 0x80;
        break;
      case MDLTOK_TEXTURE_ID:
        if (version < 800) {
          layer->textureId = parse.ExpectInt();
        } else if (IReadFlipbook(parse, expectanimation, layer)) {
          continue;
        }
        break;
      case MDLTOK_TVERTEXANIMID:
        layer->transformId = parse.ExpectInt();
        break;
      case MDLTOK_COORD_ID:
        layer->coordId = parse.ExpectInt();
        break;
      case MDLTOK_ALPHA:
        if (IReadAlpha(parse, expectanimation, layer)) {
          continue;
        }
        break;
      default:
        parse.FatalUnexpected(tokentext);
        break;
    }
    parse.Expect(',');
  }
  parse.Expect('}', savedtoken, tokentext);
  errors.Complete(status);
}

static void IReadMaterial(Parser &parse, MDLMATERIALSECTION *material, CMDLStatus *status, UINT version) {
  TSet errors;
  IMaterialAddErrors(errors);
  parse.Expect('{');
  LPCSTR tokentext;
  UINT   token;
  for (token = parse.Token(&tokentext, 0); token != '}' && token; token = parse.Token(&tokentext, 0)) {
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokentext);
    }
    switch (token) {
      case MDLTOK_LAYER:
        IReadTexLayer(parse, material->texLayers.New(), status, version);
        continue;
      case MDLTOK_CONSTANTCOLOR:
      case MDLTOK_FULLRESOLUTION:
      case MDLTOK_SORTPRIMSFARZ:
      case MDLTOK_SORTPRIMSNEARZ:
      case MDLTOK_TWO_SIDED:
      case MDLTOK_UNFOGGED:
        break;
      case MDLTOK_PRIORITYPLANE:
        material->priorityPlane = parse.ExpectInt();
        break;
      default:
        parse.FatalUnexpected(tokentext);
        break;
    }
    parse.Expect(',');
  }
  parse.Expect('}', token, tokentext);
  errors.Complete(status);
}

static UINT IGetFilterModeToken(MDLTEXOP mode) {
  switch (mode) {
    case TEXOP_LOAD:
      return MDLTOK_CAPNONE;
    case TEXOP_TRANSPARENT:
      return MDLTOK_TRANSPARENT;
    case TEXOP_BLEND:
      return MDLTOK_BLEND;
    case TEXOP_ADD:
      return MDLTOK_ADDITIVE;
    case TEXOP_ADD_ALPHA:
      return MDLTOK_ADD_ALPHA;
    case TEXOP_MODULATE:
      return MDLTOK_MODULATE;
    case TEXOP_MODULATE2X:
      return MDLTOK_MODULATE2X;
    default:
      return MDLTOK_UNKNOWN;
  }
}

static void IWriteTextureFlags(UINT flags, TSGrowableArray<char> &buffer) {
  for (UINT i = 0; i < 6; ++i) {
    if (flags & s_textureFlags[i].mask) {
      MDL::WriteLine(buffer, "\t\t\t%s,\n", MDL::TokenText(s_textureFlags[i].token));
    }
  }
}

static void IWriteLayer(const MDLTEXLAYER &layer, int needCoordIds, TSGrowableArray<char> &buffer) {
  MDL::WriteLine(buffer, "\t\t%s {\n", MDL::TokenText(MDLTOK_LAYER));
  MDL::WriteLine(buffer, "\t\t\t%s %s,\n", MDL::TokenText(MDLTOK_FILTERMODE), MDL::TokenText(IGetFilterModeToken(layer.blendMode)));
  IWriteTextureFlags(layer.flags, buffer);
  if (layer.flipKeys.keys.Count()) {
    WriteIntKeyFrames(MDLTOK_TEXTURE_ID, "\t\t\t", layer.flipKeys, buffer);
  } else {
    MDL::WriteLine(buffer, "\t\t\t%s %s ", MDL::TokenText(MDLTOK_STATIC), MDL::TokenText(MDLTOK_TEXTURE_ID));
    WriteUintKeyData(buffer, &layer.textureId, 1);
  }
  if (layer.transformId != (UINT)-1) {
    MDL::WriteLine(buffer, "\t\t\t%s %u,\n", MDL::TokenText(MDLTOK_TVERTEXANIMID), layer.transformId);
  }
  if (needCoordIds && !(layer.flags & 2)) {
    MDL::WriteLine(buffer, "\t\t\t%s %u,\n", MDL::TokenText(MDLTOK_COORD_ID), layer.coordId);
  }
  if (layer.alphaKeys.keys.Count() || layer.staticAlpha < 1.0f) {
    if (layer.alphaKeys.keys.Count()) {
      WriteFloatKeyFrames(MDLTOK_ALPHA, "\t\t\t", layer.alphaKeys, buffer);
    } else {
      MDL::WriteLine(buffer, "\t\t\t%s %s ", MDL::TokenText(MDLTOK_STATIC), MDL::TokenText(MDLTOK_ALPHA));
      WriteKeyData(buffer, &layer.staticAlpha, 1);
    }
  }
  MDL::WriteLine(buffer, "\t\t}\n");
}

static void IWriteMaterial(const MDLMATERIALSECTION &material, TSGrowableArray<char> &buffer) {
  MDL::WriteLine(buffer, "\t%s {\n", MDL::TokenText(MDLTOK_MATERIAL));
  if (material.priorityPlane) {
    MDL::WriteLine(buffer, "\t\t%s %d,\n", MDL::TokenText(MDLTOK_PRIORITYPLANE), material.priorityPlane);
  }
  const MDLTEXLAYER *layer = material.texLayers.Ptr();
  int needCoordIds = 0;
  for (UINT i = material.texLayers.Count(); i; ++layer) {
    --i;
    if (!(layer->flags & 2) && layer->coordId) {
      needCoordIds = 1;
      break;
    }
  }
  layer = material.texLayers.Ptr();
  for (UINT j = material.texLayers.Count(); j; --j, ++layer) {
    IWriteLayer(*layer, needCoordIds, buffer);
  }
  MDL::WriteLine(buffer, "\t}\n");
}

static BOOL ReadBinLayer(CMsgBuffer &buf, CMDLStatus *status, UINT *bytesRead, MDLTEXLAYER *layer) {
  UINT sectionLength = buf.GetUint();
  UINT localBytesRead = 4;
  layer->blendMode = (MDLTEXOP)buf.GetUint();
  layer->flags = buf.GetUint();
  layer->textureId = buf.GetUint();
  layer->transformId = buf.GetUint();
  layer->coordId = buf.GetUint();
  layer->staticAlpha = buf.GetFloat();
  localBytesRead += 24;
  while (localBytesRead < sectionLength) {
    DWORD tag = buf.GetDword();
    localBytesRead += 4;
    if (tag == 'ATMK') {
      ReadBinFloatKeyFrames(layer->alphaKeys, buf, localBytesRead);
    } else if (tag == 'FTMK') {
      ReadBinUintKeyFrames(layer->flipKeys, buf, localBytesRead);
    } else {
      SkipUnknown(buf, localBytesRead);
    }
  }
  if (localBytesRead > sectionLength) {
    status->FatalOverran("TexLayer", -1);
    return 0;
  }
  *bytesRead += localBytesRead;
  return 1;
}

static BOOL ReadBinMaterial(CMsgBuffer &buf, UINT sectionLength, MDLMATERIALSECTION *pMat, CMDLStatus *status, UINT &bytesRead) {
  UINT localBytesRead = 4;
  pMat->priorityPlane = buf.GetInt();
  localBytesRead += 4;
  UINT numLayers = buf.GetUint();
  localBytesRead += 4;
  pMat->texLayers.SetCount(numLayers);
  for (UINT i = 0; i < numLayers; ++i) {
    if (!ReadBinLayer(buf, status, &localBytesRead, &pMat->texLayers[i])) {
      return 0;
    }
  }
  if (localBytesRead > sectionLength) {
    status->FatalOverran("Material", -1);
    return 0;
  }
  bytesRead += localBytesRead;
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
  UINT               size = 12;
  const MDLTEXLAYER *layer = material.texLayers.Ptr();
  for (UINT i = material.texLayers.Count(); i; --i, ++layer) {
    size += GetLayerSize(*layer);
  }
  return size;
}

static void AddLayers(CMsgBuffer &buf, const MDLMATERIALSECTION &material) {
  UINT numLayers = material.texLayers.Count();
  buf.AddUint(numLayers);
  for (UINT i = 0; i < numLayers; ++i) {
    buf.AddUint(GetLayerSize(material.texLayers[i]));
    buf.AddUint(material.texLayers[i].blendMode);
    buf.AddUint(material.texLayers[i].flags);
    buf.AddUint(material.texLayers[i].textureId);
    buf.AddUint(material.texLayers[i].transformId);
    if (material.texLayers[i].flags & 2) {
      buf.AddUint(-1);
    } else {
      buf.AddUint(material.texLayers[i].coordId);
    }
    buf.AddFloat(material.texLayers[i].staticAlpha);
    WriteBinFloatKeyFrames(material.texLayers[i].alphaKeys, 'ATMK', buf);
    WriteBinUintKeyFrames(material.texLayers[i].flipKeys, 'FTMK', buf);
  }
}

BOOL MDL::ReadMaterials(Parser &parse, MDLDATA &data, CMDLStatus *status) {
  UINT   savedtoken;
  LPCSTR tokentext;
  long   actual = 0;
  long   count = parse.GetOptionalInt(&savedtoken, &tokentext, 0);
  if (count > 0) {
    data.materials.ReserveSpace(count);
  }
  parse.Expect('{', savedtoken, tokentext);
  savedtoken = parse.Token(&tokentext, 0);
  while (savedtoken == MDLTOK_MATERIAL) {
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

BOOL MDL::WriteMaterials(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
  UINT numMaterials = data.materials.Count();
  if (numMaterials) {
    MDL::WriteLine(buffer, "%s %d {\n", MDL::TokenText(MDLTOK_MATERIALS), numMaterials);
    for (UINT i = 0; i < numMaterials; ++i) {
      IWriteMaterial(data.materials[i], buffer);
    }
    MDL::WriteLine(buffer, "}\n");
  }
  return 1;
}

BOOL MDL::ReadBinMaterials(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status) {
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

BOOL MDL::WriteBinMaterials(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *) {
  UINT numLayers;
  UINT numMaterials = data.materials.Count();
  UINT animatedLayers;
  UINT i;
  UINT size;
  if (!numMaterials) {
    return 1;
  }
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
      if (data.materials[i].texLayers[j].alphaKeys.keys.Count() > 0 || data.materials[i].texLayers[j].flipKeys.keys.Count() > 0) {
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
  return 1;
}
