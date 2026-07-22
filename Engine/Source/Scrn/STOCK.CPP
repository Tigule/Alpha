#include "Scrn.h"

#include <Base/Coordinate.h>
#include <Base/Handle.h>
#include <Gxu/IGxuFont.h>
#include <Services/TextBlock.h>
const float PI = 3.14159265358979323846f;
const float TWO_PI = PI + PI;
const float OO_TWO_PI = 1.0f / TWO_PI;

static HTEXTFONT__ *s_stockObjects[SCRNSTOCKOBJECTS];
static const float s_stockObjectHeights[SCRNSTOCKOBJECTS] = {
  0.01953125f,
  0.01953125f
};

void __fastcall OsBuildFontFilePath(const char *fileName,
                                    char *buffer,
                                    unsigned int size);
static void __fastcall CreateSysFont(SCRNSTOCK stockID,
                                     const char *fontFilePath) {
  ASSERT(stockID < SCRNSTOCKOBJECTS);

  if (s_stockObjects[stockID]) {
    HandleClose(reinterpret_cast<HOBJECT>(s_stockObjects[stockID]));
  }

  s_stockObjects[stockID] = TextBlockGenerateFont(
      fontFilePath,
      0,
      NDCToDDCHeight(s_stockObjectHeights[stockID]));

  ASSERT(s_stockObjects[stockID]);
}

void __fastcall IStockDestroy() {
  unsigned int i;

  for (i = 0; i < SCRNSTOCKOBJECTS; ++i) {
    if (s_stockObjects[i]) {
      HandleClose(reinterpret_cast<HOBJECT>(s_stockObjects[i]));
    }

    s_stockObjects[i] = 0;
  }

  GxuFontShutdown();
}

void __fastcall IStockInitialize() {
  char fontFile[0x104];

  GxuFontInitialize();
  OsBuildFontFilePath("FRIZQT__.TTF", fontFile, sizeof(fontFile));
  ScrnSetStockFont(STOCK_SYSFONT, fontFile);
  ScrnSetStockFont(STOCK_PERFFONT, fontFile);
}

HTEXTFONT__ *__fastcall ScrnGetStockFont(SCRNSTOCK id) {
  FATALASSERT(id < SCRNSTOCKOBJECTS);

  return s_stockObjects[id];
}

void __fastcall ScrnSetStockFont(SCRNSTOCK stockID,
                                 const char *fontTexturePath) {
  FATALASSERT(stockID < SCRNSTOCKOBJECTS);

  FATALASSERT(fontTexturePath && *fontTexturePath);

  CreateSysFont(stockID, fontTexturePath);
}

float __fastcall ScrnGetStockFontHeight(SCRNSTOCK stockID) {
  FATALASSERT(stockID < SCRNSTOCKOBJECTS);

  return s_stockObjectHeights[stockID];
}
