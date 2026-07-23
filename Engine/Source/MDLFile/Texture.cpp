#include <stpl.h>

class Parser;
class CMDLStatus;
class TSet;
struct MDLTEXTURESECTION;

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
    // TODO: implement
}

static void IAddBitmapErrors(TSet& errors) {
    // TODO: implement
}

static void IReadBitmap(Parser& parse, MDLTEXTURESECTION* bitmap, CMDLStatus* status) {
    // TODO: implement
}
