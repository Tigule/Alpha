#include "MDLTypes.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

#include <storm.h>
#include <stpl.h>

namespace MDL {

  const char *TokenText(unsigned int token);
  void __cdecl           WriteLine(TSGrowableArray<char> &buffer, const char *format, ...);

}  // namespace MDL

struct TOKENFLAG {
  unsigned int flag;
  unsigned int token;
};

static TOKENFLAG s_textureFlags[2] = {
    {0x1, 0x1DD},
    {0x2, 0x1DC}
};

void IWriteTextureFlags(unsigned int flags, TSGrowableArray<char> &buffer) {
  for (unsigned int i = 0; i < 2; ++i) {
    if (flags & s_textureFlags[i].flag) {
      MDL::WriteLine(buffer, "\t\t%s,\n", MDL::TokenText(s_textureFlags[i].token));
    }
  }
}

static void IReadFilename(Parser& parse, char* dest) {
  const char *filename = parse.ExpectString();
  if (filename) {
    SStrCopy(dest, filename, 260);
  }
}

static void IAddBitmapErrors(TSet& errors) {
  errors.Add(0x15C, 1, 0);
  errors.Add(0x1DD, 0, 0);
  errors.Add(0x1DC, 0, 0);
  errors.Add(0x1AA, 0, 0);
}

static void IReadBitmap(Parser& parse, MDLTEXTURESECTION* bitmap, CMDLStatus* status) {
  TSet errors;
  IAddBitmapErrors(errors);
  parse.Expect('{');

  const char *tokentext;
  unsigned int token = parse.Token(&tokentext, 0);
  while (token && token != '}') {
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokentext);
    }

    switch (token) {
      case 0x15C:
        IReadFilename(parse, bitmap->image);
        break;
      case 0x1AA:
        bitmap->replaceableId = parse.ExpectInt();
        break;
      case 0x1DC:
        bitmap->flags |= 2;
        break;
      case 0x1DD:
        bitmap->flags |= 1;
        break;
      default:
        parse.FatalUnexpected(tokentext);
        break;
    }

    parse.Expect(',');
    token = parse.Token(&tokentext, 0);
  }

  parse.Expect('}', token, tokentext);
  errors.Complete(status);
}

static void IWriteTexture(const MDLTEXTURESECTION &texture, TSGrowableArray<char> &buffer) {
  MDL::WriteLine(buffer, "\t%s {\n", MDL::TokenText(0x12D));
  MDL::WriteLine(buffer, "\t\t%s \"%s\",\n", MDL::TokenText(0x15C), static_cast<const char *>(texture.image));
  if (texture.replaceableId) {
    MDL::WriteLine(buffer, "\t\t%s %d,\n", MDL::TokenText(0x1AA), texture.replaceableId);
  }
  IWriteTextureFlags(texture.flags, buffer);
  MDL::WriteLine(buffer, "\t},\n");
}

namespace MDL {

int ReadTextures(Parser &parse, MDLDATA &data, CMDLStatus *status) {
  unsigned int savedtoken;
  const char *tokentext;
  long count = parse.GetOptionalInt(&savedtoken, &tokentext, 0);
  parse.Expect('{', savedtoken, tokentext);
  if (count > 0) {
    data.textures.ReserveSpace(count);
  }

  long actual = 0;
  savedtoken = parse.Token(&tokentext, 0);
  while (savedtoken == 0x12D) {
    MDLTEXTURESECTION *texture = data.textures.New();
    texture->replaceableId = 0;
    static_cast<char *>(texture->image)[0] = 0;
    texture->flags = 0;
    IReadBitmap(parse, texture, status);
    ++actual;
    savedtoken = parse.Token(&tokentext, 0);
  }
  parse.Expect('}', savedtoken, tokentext);
  if (count >= 0 && actual != count) {
    parse.WarningCount("textures", count, actual);
  }
  return !parse.FoundError();
}

int WriteTextures(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
  FATALASSERT(data.textures.Count() > 0 || data.bones.Count() == 0);
  if (data.textures.Count()) {
    WriteLine(buffer, "%s %d {\n", TokenText(0x108), data.textures.Count());
    for (unsigned int i = 0; i < data.textures.Count(); ++i) {
      IWriteTexture(data.textures[i], buffer);
    }
    WriteLine(buffer, "}\n");
  }
  return 1;
}

int ReadBinTextures(
    CMsgBuffer &buf,
    unsigned int length,
    MDLDATA &data,
    CMDLStatus *status
) {
  FATALASSERT(status);
  if (length % 268) {
    status->Add(STATUS_ERROR, "Invalid TEXS section detected in model.\n");
    return 0;
  }

  data.textures.SetCount(length / 268);
  for (unsigned int i = 0; i < length / 268; ++i) {
    MDLTEXTURESECTION &texture = data.textures[i];
    texture.replaceableId = buf.GetUint();
    buf.GetTcharArray(texture.image, 260);
    texture.flags = buf.GetUint();
  }
  return 1;
}

int WriteBinTextures(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *) {
  FATALASSERT(data.textures.Count() > 0 || data.bones.Count() == 0);
  if (data.textures.Count()) {
    buf.AddDword('SXET');
    buf.AddUint(268 * data.textures.Count());
    for (unsigned int i = 0; i < data.textures.Count(); ++i) {
      const MDLTEXTURESECTION &texture = data.textures[i];
      buf.AddUint(texture.replaceableId);
      buf.AddTcharArray(texture.image, 260, 1);
      buf.AddUint(texture.flags);
    }
  }
  return 1;
}

}
