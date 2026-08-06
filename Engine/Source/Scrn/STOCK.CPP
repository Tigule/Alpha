#include <Base/Base.h>

#include "Scrn.h"

#include <Base/Coordinate.h>
#include <Base/Handle.h>
#include <Gxu/IGxuFont.h>
#include <Services/TextBlock.h>

static HTEXTFONT__ *s_stockObjects[SCRNSTOCKOBJECTS];
static const float  s_stockObjectHeights[SCRNSTOCKOBJECTS] = {0.01953125f, 0.01953125f};

void        OsBuildFontFilePath(LPCSTR fileName, char *buffer, UINT size);
static void CreateSysFont(SCRNSTOCK stockID, LPCSTR fontFilePath) {
  ASSERT(stockID < SCRNSTOCKOBJECTS);

  if (s_stockObjects[stockID]) {
    HandleClose(reinterpret_cast<HOBJECT>(s_stockObjects[stockID]));
  }

  s_stockObjects[stockID] = TextBlockGenerateFont(fontFilePath, 0, NDCToDDCHeight(s_stockObjectHeights[stockID]));

  ASSERT(s_stockObjects[stockID]);
}

void IStockDestroy() {
  UINT i;

  for (i = 0; i < SCRNSTOCKOBJECTS; ++i) {
    if (s_stockObjects[i]) {
      HandleClose(reinterpret_cast<HOBJECT>(s_stockObjects[i]));
    }

    s_stockObjects[i] = 0;
  }

  GxuFontShutdown();
}

void IStockInitialize() {
  char fontFile[0x104];

  GxuFontInitialize();
  OsBuildFontFilePath("FRIZQT__.TTF", fontFile, sizeof(fontFile));
  ScrnSetStockFont(STOCK_SYSFONT, fontFile);
  ScrnSetStockFont(STOCK_PERFFONT, fontFile);
}

HTEXTFONT__ *ScrnGetStockFont(SCRNSTOCK id) {
  FATALASSERT(id < SCRNSTOCKOBJECTS);

  return s_stockObjects[id];
}

void ScrnSetStockFont(SCRNSTOCK stockID, LPCSTR fontTexturePath) {
  FATALASSERT(stockID < SCRNSTOCKOBJECTS);

  FATALASSERT(fontTexturePath && *fontTexturePath);

  CreateSysFont(stockID, fontTexturePath);
}

float ScrnGetStockFontHeight(SCRNSTOCK stockID) {
  FATALASSERT(stockID < SCRNSTOCKOBJECTS);

  return s_stockObjectHeights[stockID];
}
