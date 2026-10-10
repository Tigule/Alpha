#include "MDLTypes.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

#include <storm.h>
#include <stpl.h>

namespace MDL {

  LPCSTR       TokenText(UINT token);
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);

}  // namespace MDL

struct TOKENFLAG {
  UINT mask;
  UINT token;
};

static TOKENFLAG s_textureFlags[2] = {
    {0x1, MDLTOK_WRAPWIDTH},
    {0x2, MDLTOK_WRAPHEIGHT}
};

static void IWriteTextureFlags(UINT flags, TSGrowableArray<char> &buffer) {
  for (UINT i = 0; i < 2; ++i) {
    if (flags & s_textureFlags[i].mask) {
      MDL::WriteLine(buffer, "\t\t%s,\n", MDL::TokenText(s_textureFlags[i].token));
    }
  }
}

static void IReadFilename(Parser &parse, char *dest) {
  LPCSTR filename = parse.ExpectString();
  if (filename) {
    SStrCopy(dest, filename, 260);
  }
}

static void IAddBitmapErrors(TSet &errors) {
  errors.Add(MDLTOK_IMAGE, 1, 0);
  errors.Add(MDLTOK_WRAPWIDTH, 0, 0);
  errors.Add(MDLTOK_WRAPHEIGHT, 0, 0);
  errors.Add(MDLTOK_REPLACEABLE_ID, 0, 0);
}

static void IReadBitmap(Parser &parse, MDLTEXTURESECTION *bitmap, CMDLStatus *status) {
  TSet errors;
  IAddBitmapErrors(errors);
  parse.Expect('{');

  LPCSTR tokentext;
  UINT   token;
  for (token = parse.Token(&tokentext, 0); token != '}'; token = parse.Token(&tokentext, 0)) {
    if (!token) {
      break;
    }
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokentext);
    }

    switch (token) {
      case MDLTOK_IMAGE:
        IReadFilename(parse, bitmap->image);
        break;
      case MDLTOK_REPLACEABLE_ID:
        bitmap->replaceableId = parse.ExpectInt();
        break;
      case MDLTOK_WRAPWIDTH:
        bitmap->flags |= 1;
        break;
      case MDLTOK_WRAPHEIGHT:
        bitmap->flags |= 2;
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

static void IWriteTexture(const MDLTEXTURESECTION &texture, TSGrowableArray<char> &buffer) {
  MDL::WriteLine(buffer, "\t%s {\n", MDL::TokenText(MDLTOK_BITMAP));
  MDL::WriteLine(buffer, "\t\t%s \"%s\",\n", MDL::TokenText(MDLTOK_IMAGE), (LPCSTR)texture.image);
  if (texture.replaceableId) {
    MDL::WriteLine(buffer, "\t\t%s %d,\n", MDL::TokenText(MDLTOK_REPLACEABLE_ID), texture.replaceableId);
  }
  IWriteTextureFlags(texture.flags, buffer);
  MDL::WriteLine(buffer, "\t}\n");
}

namespace MDL {

  BOOL ReadTextures(Parser &parse, MDLDATA &data, CMDLStatus *status) {
    UINT   savedtoken;
    LPCSTR tokentext;
    long   count = parse.GetOptionalInt(&savedtoken, &tokentext, 0);
    parse.Expect('{', savedtoken, tokentext);
    if (count > 0) {
      data.textures.ReserveSpace(count);
    }

    long actual = 0;
    savedtoken = parse.Token(&tokentext, 0);
    while (savedtoken == MDLTOK_BITMAP) {
      MDLTEXTURESECTION *texture = data.textures.New();
      texture->replaceableId = 0;
      ((char *)texture->image)[0] = 0;
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

  BOOL WriteTextures(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
    UINT numTextures = data.textures.Count();
    VALIDATEBEGIN;
    VALIDATE((numTextures > 0) || (data.bones.Count() == 0));
    VALIDATEEND;
    if (numTextures) {
      WriteLine(buffer, "%s %d {\n", TokenText(MDLTOK_TEXTURES), numTextures);
      const MDLTEXTURESECTION *texture = data.textures.Ptr();
      for (UINT i = numTextures; i; --i, ++texture) {
        IWriteTexture(*texture, buffer);
      }
      WriteLine(buffer, "}\n");
    }
    return 1;
  }

  BOOL ReadBinTextures(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status) {
    VALIDATEBEGIN;
    VALIDATE(status != 0);
    VALIDATEEND;
    if (length % 268) {
      status->Add(STATUS_ERROR, "Invalid TXTX section detected in model -- nonintegral number of textures.\n");
      return 0;
    }

    length /= 268;
    data.textures.SetCount(length);
    MDLTEXTURESECTION *texture = data.textures.Ptr();
    for (UINT i = length; i; --i, ++texture) {
      texture->replaceableId = buf.GetUint();
      buf.GetTcharArray(texture->image, 260);
      texture->flags = buf.GetUint();
    }
    return 1;
  }

  BOOL WriteBinTextures(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *) {
    UINT numTextures = data.textures.Count();
    VALIDATEBEGIN;
    VALIDATE((numTextures > 0) || (data.bones.Count() == 0));
    VALIDATEEND;
    if (numTextures) {
      buf.AddDword('SXET');
      buf.AddUint(268 * numTextures);
      const MDLTEXTURESECTION *texture = data.textures.Ptr();
      for (UINT i = numTextures; i; --i, ++texture) {
        buf.AddUint(texture->replaceableId);
        buf.AddTcharArray(texture->image, 260, 1);
        buf.AddUint(texture->flags);
      }
    }
    return 1;
  }

}  // namespace MDL
