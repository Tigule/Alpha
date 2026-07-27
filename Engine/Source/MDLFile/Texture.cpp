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

  const char *tokenText;
  unsigned int token = parse.Token(&tokenText, 0);
  while (token && token != '}') {
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokenText);
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
        parse.FatalUnexpected(tokenText);
        break;
    }

    parse.Expect(',');
    token = parse.Token(&tokenText, 0);
  }

  parse.Expect('}', token, tokenText);
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
  unsigned int savedToken;
  const char *tokenText;
  long count = parse.GetOptionalInt(&savedToken, &tokenText, 0);
  parse.Expect('{', savedToken, tokenText);
  if (count > 0) {
    data.textures.Reserve(count);
  }

  long actual = 0;
  savedToken = parse.Token(&tokenText, 0);
  while (savedToken == 0x12D) {
    MDLTEXTURESECTION *texture = data.textures.New();
    texture->replaceableId = 0;
    static_cast<char *>(texture->image)[0] = 0;
    texture->flags = 0;
    IReadBitmap(parse, texture, status);
    ++actual;
    savedToken = parse.Token(&tokenText, 0);
  }
  parse.Expect('}', savedToken, tokenText);
  if (count >= 0 && actual != count) {
    parse.WarningCount("textures", count, actual);
  }
  return !parse.FoundError();
}

int WriteTextures(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
  unsigned int count = data.textures.Count();
  FATALASSERT(count > 0 || data.bones.Count() == 0);
  if (count) {
    WriteLine(buffer, "%s %d {\n", TokenText(0x108), count);
    for (unsigned int i = 0; i < count; ++i) {
      IWriteTexture(data.textures[i], buffer);
    }
    WriteLine(buffer, "}\n");
  }
  return 1;
}

int ReadBinTextures(
    CMsgBuffer &buffer,
    unsigned int length,
    MDLDATA &data,
    CMDLStatus *status
) {
  FATALASSERT(status);
  if (length % 268) {
    status->Add(STATUS_ERROR, "Invalid TEXS section detected in model.\n");
    return 0;
  }

  unsigned int count = length / 268;
  data.textures.SetCount(count);
  for (unsigned int i = 0; i < count; ++i) {
    MDLTEXTURESECTION &texture = data.textures[i];
    texture.replaceableId = buffer.GetUint();
    buffer.GetTcharArray(texture.image, 260);
    texture.flags = buffer.GetUint();
  }
  return 1;
}

int WriteBinTextures(const MDLDATA &data, CMsgBuffer &buffer, CMDLStatus *) {
  unsigned int count = data.textures.Count();
  FATALASSERT(count > 0 || data.bones.Count() == 0);
  if (count) {
    buffer.AddDword('SXET');
    buffer.AddUint(268 * count);
    for (unsigned int i = 0; i < count; ++i) {
      const MDLTEXTURESECTION &texture = data.textures[i];
      buffer.AddUint(texture.replaceableId);
      buffer.AddTcharArray(texture.image, 260, 1);
      buffer.AddUint(texture.flags);
    }
  }
  return 1;
}

}
