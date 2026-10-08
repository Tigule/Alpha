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
  errors.Add(0x1C7, 0, 0);
  errors.Add(0x1AD, 0, 0);
  errors.Add(0x1AF, 0, 0);
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
      case 0x1C7:
        ReadObjectFloatKeyframes(parse, &texAnim->transkeys);
        break;
      case 0x1AD:
        ReadObjectFloatKeyframes(parse, &texAnim->rotkeys);
        break;
      case 0x1AF:
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
  UINT   token;
  LPCSTR tokenText;
  long   expected = parse.GetOptionalInt(&token, &tokenText, 0);
  if (expected > 0) {
    data.textureanims.ReserveSpace(expected);
  }
  parse.Expect('{', token, tokenText);
  token = parse.Token(&tokenText, 0);
  long actual = 0;
  while (token == 0x1CC) {
    MDLTEXANIMSECTION *texAnim = data.textureanims.New();
    IReadTextureAnim(parse, texAnim, status);
    ++actual;
    token = parse.Token(&tokenText, 0);
  }
  parse.Expect('}', token, tokenText);
  if (expected >= 0 && actual != expected) {
    parse.WarningCount("texture animations", expected, actual);
  }
  return !parse.FoundError();
}

static void IWriteTextureAnim(const MDLTEXANIMSECTION &section, TSGrowableArray<char> &buffer) {
  MDL::WriteLine(buffer, "\t%s {\n", MDL::TokenText(0x1CC));
  WriteFloatKeyFrames(0x1C7, "\t\t", section.transkeys, buffer);
  WriteFloatKeyFrames(0x1AD, "\t\t", section.rotkeys, buffer);
  WriteFloatKeyFrames(0x1AF, "\t\t", section.scalekeys, buffer);
  MDL::WriteLine(buffer, "\t}\n");
}

BOOL MDL::WriteTextureAnims(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
  if (!static_cast<LPCSTR>(data.model.animationFile)[0] && data.textureanims.Count()) {
    MDL::WriteLine(buffer, "%s %d {\n", MDL::TokenText(0x107), data.textureanims.Count());
    for (UINT i = 0; i < data.textureanims.Count(); ++i) {
      IWriteTextureAnim(data.textureanims.Ptr()[i], buffer);
    }
    MDL::WriteLine(buffer, "}\n");
  }
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

BOOL MDL::WriteBinTextureAnims(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *) {
  if (!static_cast<LPCSTR>(data.model.animationFile)[0] && data.textureanims.Count()) {
    buf.AddDword('NAXT');
    UINT totalSize = 4;
    UINT i;
    for (i = 0; i < data.textureanims.Count(); ++i) {
      totalSize += GetBinTexAnimSize(data.textureanims.Ptr()[i]);
    }
    buf.AddUint(totalSize);
    buf.AddUint(data.textureanims.Count());
    for (i = 0; i < data.textureanims.Count(); ++i) {
      IWriteBinTextureAnim(data.textureanims.Ptr()[i], buf);
    }
  }
  return 1;
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
