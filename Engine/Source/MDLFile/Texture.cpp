#include "MDLTypes.h"

#include <storm.h>
#include <stpl.h>

class CMDLStatus;
union UTokenData;

class Parser {
 public:
  void Expect(unsigned int what);
  void Expect(unsigned int what, unsigned int cachedToken, const char *tokenText);
  long ExpectInt();
  const char *ExpectString();
  unsigned int Token(const char **tokenText, UTokenData *data);
  void FatalDuplicate(const char *found);
  void FatalUnexpected(const char *found);
};

class TSet {
 public:
  TSet();
  void Add(unsigned int token, int needed, int allowDuplicates);
  int Check(unsigned int token);
  void Complete(CMDLStatus *status);

 private:
  char m_data[1028];
};

namespace MDL {

  const char *__fastcall TokenText(unsigned int token);
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

void __fastcall IWriteTextureFlags(unsigned int flags, TSGrowableArray<char> &buffer) {
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
