#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

namespace MDL {
const char *TokenText(unsigned int token);
void __cdecl WriteLine(TSGrowableArray<char> &buffer, const char *format, ...);

static void ITextureAnimAddErrors(TSet &errors) {
  errors.Add(0x1C7, 0, 0);
  errors.Add(0x1AD, 0, 0);
  errors.Add(0x1AF, 0, 0);
}

static void IReadTextureAnim(
    Parser &parse,
    MDLTEXANIMSECTION *texAnim,
    CMDLStatus *status
) {
  TSet errors;
  ITextureAnimAddErrors(errors);
  parse.Expect('{');
  const char *tokenText;
  unsigned int token = parse.Token(&tokenText, 0);
  while (token && token != '}') {
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokenText);
    }
    if (token == 0x1C7) {
      ReadObjectFloatKeyframes(parse, &texAnim->transkeys);
    } else if (token == 0x1AF) {
      ReadObjectFloatKeyframes(parse, &texAnim->scalekeys);
    } else if (token == 0x1AD) {
      MDLKEYTRACK<NTempest::C4Quaternion> &track = texAnim->rotkeys;
      UTokenData tokenData;
      long expected = parse.GetOptionalInt(&token, &tokenText, &tokenData);
      if (expected > 0) {
        track.keys.ReserveSpace(expected);
      }
      parse.Expect('{', token, tokenText);
      token = ReadFloatTrackHeader(
          parse,
          &track,
          &tokenText,
          &tokenData
      );
      long actual = 0;
      while (token == 0x100) {
        MDLKEYFRAME<NTempest::C4Quaternion> *key = track.keys.New();
        key->time = tokenData.lVal;
        parse.Expect(':');
        ReadFloatKeyData(parse, &key->value.x, 4);
        parse.Expect(',');
        ++actual;
        if (track.type > TRACK_LINEAR) {
          parse.Expect(0x15E);
          ReadFloatKeyData(parse, &key->inTan.x, 4);
          parse.Expect(',');
          parse.Expect(0x18A);
          ReadFloatKeyData(parse, &key->outTan.x, 4);
          parse.Expect(',');
        }
        token = parse.Token(&tokenText, &tokenData);
      }
      parse.Expect('}', token, tokenText);
      if (expected >= 0 && actual != expected) {
        parse.WarningCount("key frames", expected, actual);
      }
    } else {
      parse.FatalUnexpected(tokenText);
    }
    token = parse.Token(&tokenText, 0);
  }
  parse.Expect('}', token, tokenText);
  errors.Complete(status);
}

int ReadTextureAnims(
    Parser &parse,
    MDLDATA &data,
    CMDLStatus *status
) {
  unsigned int token;
  const char *tokenText;
  long expected = parse.GetOptionalInt(&token, &tokenText, 0);
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

static void IWriteTextureAnim(
    const MDLTEXANIMSECTION &section,
    TSGrowableArray<char> &buffer
) {
  WriteLine(buffer, "\t%s {\n", TokenText(0x1CC));
  const char *indent = "\t\t";

  if (section.transkeys.keys.Count()) {
    const MDLKEYTRACK<NTempest::C3Vector> &track = section.transkeys;
    WriteLine(
        buffer,
        "%s%s %d {\n",
        indent,
        TokenText(0x1C7),
        track.keys.Count()
    );
    WriteTrackHeader(indent, track, buffer);
    for (unsigned int i = 0; i < track.keys.Count(); ++i) {
      const MDLKEYFRAME<NTempest::C3Vector> &key = track.keys.Ptr()[i];
      WriteLine(buffer, "%s\t%d: ", indent, key.time);
      WriteKeyData(buffer, &key.value.x, 3);
      if (track.type > TRACK_LINEAR) {
        WriteLine(buffer, "%s\t\t%s ", indent, TokenText(0x15E));
        WriteKeyData(buffer, &key.inTan.x, 3);
        WriteLine(buffer, "%s\t\t%s ", indent, TokenText(0x18A));
        WriteKeyData(buffer, &key.outTan.x, 3);
      }
    }
    WriteLine(buffer, "%s}\n", indent);
  }

  if (section.rotkeys.keys.Count()) {
    const MDLKEYTRACK<NTempest::C4Quaternion> &track = section.rotkeys;
    WriteLine(
        buffer,
        "%s%s %d {\n",
        indent,
        TokenText(0x1AD),
        track.keys.Count()
    );
    WriteTrackHeader(indent, track, buffer);
    for (unsigned int i = 0; i < track.keys.Count(); ++i) {
      const MDLKEYFRAME<NTempest::C4Quaternion> &key = track.keys.Ptr()[i];
      WriteLine(buffer, "%s\t%d: ", indent, key.time);
      WriteKeyData(buffer, &key.value.x, 4);
      if (track.type > TRACK_LINEAR) {
        WriteLine(buffer, "%s\t\t%s ", indent, TokenText(0x15E));
        WriteKeyData(buffer, &key.inTan.x, 4);
        WriteLine(buffer, "%s\t\t%s ", indent, TokenText(0x18A));
        WriteKeyData(buffer, &key.outTan.x, 4);
      }
    }
    WriteLine(buffer, "%s}\n", indent);
  }

  if (section.scalekeys.keys.Count()) {
    const MDLKEYTRACK<NTempest::C3Vector> &track = section.scalekeys;
    WriteLine(
        buffer,
        "%s%s %d {\n",
        indent,
        TokenText(0x1AF),
        track.keys.Count()
    );
    WriteTrackHeader(indent, track, buffer);
    for (unsigned int i = 0; i < track.keys.Count(); ++i) {
      const MDLKEYFRAME<NTempest::C3Vector> &key = track.keys.Ptr()[i];
      WriteLine(buffer, "%s\t%d: ", indent, key.time);
      WriteKeyData(buffer, &key.value.x, 3);
      if (track.type > TRACK_LINEAR) {
        WriteLine(buffer, "%s\t\t%s ", indent, TokenText(0x15E));
        WriteKeyData(buffer, &key.inTan.x, 3);
        WriteLine(buffer, "%s\t\t%s ", indent, TokenText(0x18A));
        WriteKeyData(buffer, &key.outTan.x, 3);
      }
    }
    WriteLine(buffer, "%s}\n", indent);
  }
  WriteLine(buffer, "\t}\n");
}

int WriteTextureAnims(
    const MDLDATA &data,
    TSGrowableArray<char> &buffer,
    CMDLStatus *
) {
  if (!static_cast<const char *>(data.model.animationFile)[0]
      && data.textureanims.Count()) {
    WriteLine(
        buffer,
        "%s %d {\n",
        TokenText(0x107),
        data.textureanims.Count()
    );
    for (unsigned int i = 0; i < data.textureanims.Count(); ++i) {
      IWriteTextureAnim(data.textureanims.Ptr()[i], buffer);
    }
    WriteLine(buffer, "}\n");
  }
  return 1;
}

static unsigned int GetBinTexAnimSize(
    const MDLTEXANIMSECTION &section
) {
  unsigned int size = 4;
  if (section.transkeys.keys.Count()) {
    unsigned int dataSize =
        section.transkeys.type > TRACK_LINEAR ? 36 : 12;
    size += 16
        + section.transkeys.keys.Count() * (4 + dataSize);
  }
  size += GetBinQuatKeyFramesSize(section.rotkeys);
  if (section.scalekeys.keys.Count()) {
    unsigned int dataSize =
        section.scalekeys.type > TRACK_LINEAR ? 36 : 12;
    size += 16
        + section.scalekeys.keys.Count() * (4 + dataSize);
  }
  return size;
}

static void IWriteBinTextureAnim(
    const MDLTEXANIMSECTION &section,
    CMsgBuffer &buffer
) {
  buffer.AddUint(GetBinTexAnimSize(section));
  if (section.transkeys.keys.Count()) {
    buffer.AddDword('TATK');
    buffer.AddUint(section.transkeys.keys.Count());
    buffer.AddUint(section.transkeys.type);
    buffer.AddUint(section.transkeys.globalSeqId);
    unsigned int values =
        section.transkeys.type > TRACK_LINEAR ? 9 : 3;
    for (unsigned int i = 0; i < section.transkeys.keys.Count(); ++i) {
      const MDLKEYFRAME<NTempest::C3Vector> &key =
          section.transkeys.keys.Ptr()[i];
      buffer.AddInt(key.time);
      buffer.AddFloatArray(&key.value.x, values);
    }
  }
  WriteBinQuatKeyFrames(section.rotkeys, 'RATK', buffer);
  if (section.scalekeys.keys.Count()) {
    buffer.AddDword('SATK');
    buffer.AddUint(section.scalekeys.keys.Count());
    buffer.AddUint(section.scalekeys.type);
    buffer.AddUint(section.scalekeys.globalSeqId);
    unsigned int values =
        section.scalekeys.type > TRACK_LINEAR ? 9 : 3;
    for (unsigned int i = 0; i < section.scalekeys.keys.Count(); ++i) {
      const MDLKEYFRAME<NTempest::C3Vector> &key =
          section.scalekeys.keys.Ptr()[i];
      buffer.AddInt(key.time);
      buffer.AddFloatArray(&key.value.x, values);
    }
  }
}

int WriteBinTextureAnims(
    const MDLDATA &data,
    CMsgBuffer &buffer,
    CMDLStatus *
) {
  if (!static_cast<const char *>(data.model.animationFile)[0]
      && data.textureanims.Count()) {
    buffer.AddDword('NAXT');
    unsigned int totalSize = 4;
    unsigned int i;
    for (i = 0; i < data.textureanims.Count(); ++i) {
      totalSize += GetBinTexAnimSize(data.textureanims.Ptr()[i]);
    }
    buffer.AddUint(totalSize);
    buffer.AddUint(data.textureanims.Count());
    for (i = 0; i < data.textureanims.Count(); ++i) {
      IWriteBinTextureAnim(data.textureanims.Ptr()[i], buffer);
    }
  }
  return 1;
}

int ReadBinTextureAnims(
    CMsgBuffer &buffer,
    unsigned int length,
    MDLDATA &data,
    CMDLStatus *status
) {
  unsigned int count = buffer.GetUint();
  unsigned int totalRead = 4;
  data.textureanims.SetCount(0);
  data.textureanims.ReserveSpace(count);
  while (totalRead < length) {
    MDLTEXANIMSECTION *section = data.textureanims.New();
    if (!section) {
      status->FatalFlunked("TexAnim", -1);
      return 0;
    }
    unsigned int sectionSize = buffer.GetUint();
    unsigned int localRead = 4;
    while (localRead < sectionSize) {
      unsigned long tag = buffer.GetDword();
      localRead += 4;
      if (tag == 'RATK') {
        if (!ReadBinQuatKeyFrames(
                section->rotkeys,
                buffer,
                localRead
            )) {
          status->Add(
              STATUS_ERROR,
              "Error reading rotkeys of texanim.\n"
          );
          return 0;
        }
      } else if (tag == 'SATK') {
        if (!ReadBinFloatKeyFrames(
                section->scalekeys,
                buffer,
                localRead
            )) {
          status->Add(
              STATUS_ERROR,
              "Error reading scalekeys of texanim.\n"
          );
          return 0;
        }
      } else if (tag == 'TATK') {
        if (!ReadBinFloatKeyFrames(
                section->transkeys,
                buffer,
                localRead
            )) {
          status->Add(
              STATUS_ERROR,
              "Error reading transkeys of texanim.\n"
          );
          return 0;
        }
      } else {
        SkipUnknown(buffer, localRead);
      }
    }
    totalRead += localRead;
    if (totalRead > length) {
      status->FatalOverran("TexAnim", -1);
      return 0;
    }
  }
  return 1;
}

}
