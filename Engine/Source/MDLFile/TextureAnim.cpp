#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"


namespace MDL {
  LPCSTR       TokenText(UINT token);
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);
  BOOL         ReadTextureAnims(Parser &, MDLDATA &, CMDLStatus *);
  BOOL         WriteTextureAnims(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL         WriteBinTextureAnims(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  BOOL         ReadBinTextureAnims(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
}  // namespace MDL

static void ITextureAnimAddErrors(TSet &errors) {
  errors.Add(MDLTOK_TRANSLATION, 0, 0);
  errors.Add(MDLTOK_ROTATION, 0, 0);
  errors.Add(MDLTOK_SCALING, 0, 0);
}

static void IReadTextureAnim(Parser &parse, MDLTEXANIMSECTION *texAnim, CMDLStatus *status) {
  TSet errors;
  ITextureAnimAddErrors(errors);
  parse.Expect('{');
  LPCSTR tokenText;
  UINT   token = parse.Token(&tokenText, 0);
  while (token != '}' && token) {
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokenText);
    }
    switch (token) {
      case MDLTOK_TRANSLATION:
        ReadObjectFloatKeyframes(parse, &texAnim->transkeys);
        break;
      case MDLTOK_ROTATION:
        ReadObjectFloatKeyframes(parse, &texAnim->rotkeys);
        break;
      case MDLTOK_SCALING:
        ReadObjectFloatKeyframes(parse, &texAnim->scalekeys);
        break;
      default:
        parse.FatalUnexpected(tokenText);
        break;
    }
    token = parse.Token(&tokenText, 0);
  }
  parse.Expect('}', token, tokenText);
  errors.Complete(status);
}

BOOL MDL::ReadTextureAnims(Parser &parse, MDLDATA &data, CMDLStatus *status) {
  UINT   savedtoken;
  LPCSTR tokentext;
  long   actual = 0;
  long   count = parse.GetOptionalInt(&savedtoken, &tokentext, 0);
  if (count > 0) {
    data.textureanims.ReserveSpace(count);
  }
  parse.Expect('{', savedtoken, tokentext);
  savedtoken = parse.Token(&tokentext, 0);
  while (savedtoken == MDLTOK_TVERTEXANIM) {
    IReadTextureAnim(parse, data.textureanims.New(), status);
    ++actual;
    savedtoken = parse.Token(&tokentext, 0);
  }
  parse.Expect('}', savedtoken, tokentext);
  if (count >= 0 && actual != count) {
    parse.WarningCount("texture animations", count, actual);
  }
  return !parse.FoundError();
}

static void IWriteTextureAnim(const MDLTEXANIMSECTION &section, TSGrowableArray<char> &buffer) {
  MDL::WriteLine(buffer, "\t%s {\n", MDL::TokenText(MDLTOK_TVERTEXANIM));
  WriteFloatKeyFrames(MDLTOK_TRANSLATION, "\t\t", section.transkeys, buffer);
  WriteFloatKeyFrames(MDLTOK_ROTATION, "\t\t", section.rotkeys, buffer);
  WriteFloatKeyFrames(MDLTOK_SCALING, "\t\t", section.scalekeys, buffer);
  MDL::WriteLine(buffer, "\t}\n");
}

BOOL MDL::WriteTextureAnims(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
  if (data.model.animationFile[0]) {
    return 1;
  }
  UINT numTexAnims = data.textureanims.Count();
  if (!numTexAnims) {
    return 1;
  }
  MDL::WriteLine(buffer, "%s %d {\n", MDL::TokenText(MDLTOK_TEXTUREANIMS), numTexAnims);
  const MDLTEXANIMSECTION *texAnim = data.textureanims.Ptr();
  for (UINT i = numTexAnims; i; --i) {
    IWriteTextureAnim(*texAnim++, buffer);
  }
  MDL::WriteLine(buffer, "}\n");
  return 1;
}

static UINT GetBinTexAnimSize(const MDLTEXANIMSECTION &section) {
  UINT size = 4;
  if (section.transkeys.keys.Count()) {
    UINT dataSize = section.transkeys.type > TRACK_LINEAR ? 36 : 12;
    size += 16 + section.transkeys.keys.Count() * (4 + dataSize);
  }
  size += GetBinQuatKeyFramesSize(section.rotkeys);
  if (section.scalekeys.keys.Count()) {
    UINT dataSize = section.scalekeys.type > TRACK_LINEAR ? 36 : 12;
    size += 16 + section.scalekeys.keys.Count() * (4 + dataSize);
  }
  return size;
}

static void IWriteBinTextureAnim(const MDLTEXANIMSECTION &section, CMsgBuffer &buffer) {
  buffer.AddUint(GetBinTexAnimSize(section));
  WriteBinFloatKeyFrames(section.transkeys, 'TATK', buffer);
  WriteBinQuatKeyFrames(section.rotkeys, 'RATK', buffer);
  WriteBinFloatKeyFrames(section.scalekeys, 'SATK', buffer);
}

BOOL MDL::ReadBinTextureAnims(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status) {
  UINT numTexAnims = buf.GetUint();
  UINT totalRead = 4;
  data.textureanims.SetCount(0);
  data.textureanims.ReserveSpace(numTexAnims);
  while (totalRead < length) {
    MDLTEXANIMSECTION *section = data.textureanims.New();
    if (!section) {
      status->FatalFlunked("TexAnim", -1);
      return 0;
    }
    UINT sectionSize = buf.GetUint();
    UINT sectionRead = 4;
    while (sectionRead < sectionSize) {
      DWORD tag = buf.GetDword();
      sectionRead += 4;
      switch (tag) {
        case 'TATK':
          if (!ReadBinFloatKeyFrames(section->transkeys, buf, sectionRead)) {
            status->Add(STATUS_ERROR, "Error reading transkeys of texanim.\n");
            return 0;
          }
          break;
        case 'RATK':
          if (!ReadBinQuatKeyFrames(section->rotkeys, buf, sectionRead)) {
            status->Add(STATUS_ERROR, "Error reading rotkeys of texanim.\n");
            return 0;
          }
          break;
        case 'SATK':
          if (!ReadBinFloatKeyFrames(section->scalekeys, buf, sectionRead)) {
            status->Add(STATUS_ERROR, "Error reading scalekeys of texanim.\n");
            return 0;
          }
          break;
        default:
          SkipUnknown(buf, totalRead);
          break;
      }
    }
    totalRead += sectionRead;
    if (totalRead > length) {
      status->FatalOverran("TexAnim", -1);
      return 0;
    }
  }
  return 1;
}

BOOL MDL::WriteBinTextureAnims(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *) {
  UINT numTexAnims = data.textureanims.Count();
  if (data.model.animationFile[0] || !numTexAnims) {
    return 1;
  }
  buf.AddDword('NAXT');
  UINT totalSize = 4;
  UINT i;
  for (i = 0; i < numTexAnims; ++i) {
    totalSize += GetBinTexAnimSize(data.textureanims[i]);
  }
  buf.AddUint(totalSize);
  buf.AddUint(numTexAnims);
  for (i = 0; i < numTexAnims; ++i) {
    IWriteBinTextureAnim(data.textureanims[i], buf);
  }
  return 1;
}
