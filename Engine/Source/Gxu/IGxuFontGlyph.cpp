#include <Base/Base.h>

#include "IGxuFont.h"

#include <freetype/freetype.h>

static BOOL FREETYPE_RenderGlyph(FT_Face face, UINT charCode, int noHinting, int monochrome) {
  ASSERT(face);

  FT_UInt glyphIndex = FT_Get_Char_Index(face, charCode);
  if (!glyphIndex) {
    return 0;
  }

  int loadFlags = FT_LOAD_NO_BITMAP | FT_LOAD_PEDANTIC | FT_LOAD_LINEAR_DESIGN;
  if (noHinting) {
    loadFlags |= FT_LOAD_NO_HINTING;
  }

  if (FT_Load_Glyph(face, glyphIndex, loadFlags)) {
    return 0;
  }

  FT_Error error;
  if (monochrome) {
    error = FT_Render_Glyph(face->glyph, ft_render_mode_mono);
  } else {
    error = FT_Render_Glyph(face->glyph, ft_render_mode_normal);
  }
  return error == 0;
}

static void CalculateYOffset(FT_Face face, UINT *yOffsetPtr, UINT *glyphYStart, UINT pixelHeight, UINT baseLineRow, UINT glyphHeight) {
  ASSERT(face);
  ASSERT(yOffsetPtr);
  ASSERT(glyphYStart);
  ASSERT(pixelHeight);
  ASSERT(glyphHeight);

  UINT yStart;
  UINT yOffset = 0;
  if (glyphHeight > pixelHeight) {
    yStart = 0;
  } else if (face->glyph->bitmap_top > (int)baseLineRow) {
    yStart = 0;
    yOffset = face->glyph->bitmap_top - baseLineRow;
  } else {
    yStart = baseLineRow - face->glyph->bitmap_top;
  }

  yStart = max(yStart, 0);
  if (pixelHeight - glyphHeight < yStart) {
    yOffset = pixelHeight - yStart - glyphHeight;
    yStart = pixelHeight - glyphHeight;
  }

  *yOffsetPtr = yOffset;
  *glyphYStart = yStart;
}

BOOL IGxuFontGlyphRenderGlyph(FT_Face face, UINT pixelHeight, UINT code, UINT baseLine, GLYPHDATA *dataPtr, int noHinting, int monochrome) {
  VALIDATEBEGIN;
  VALIDATE(face);
  VALIDATE(pixelHeight);
  VALIDATE(dataPtr);
  VALIDATEEND;

  FREEIFUSED(dataPtr->data);
  dataPtr->data = 0;

  if (!FT_Get_Char_Index(face, code)) {
    return 0;
  }

  if (!FREETYPE_RenderGlyph(face, code, noHinting, monochrome)) {
    return 0;
  }

  UINT width = face->glyph->bitmap.width;
  UINT height = min(pixelHeight, face->glyph->bitmap.rows);
  ASSERT(height <= pixelHeight);

  LPCVOID srcData = face->glyph->bitmap.buffer;
  UINT    pitch = face->glyph->bitmap.pitch;
  UINT    dataSize = face->glyph->bitmap.rows * face->glyph->bitmap.pitch;
  int     dummyGlyph = 0;

  if (!width || !height || !srcData || !pitch || !dataSize) {
    height = pixelHeight;
    width = (pixelHeight + 3) / 4;
    if (!width) {
      width = pixelHeight;
    }
    if (monochrome) {
      pitch = (width + 7) & ~7;
    } else {
      pitch = width;
    }
    dataSize = pitch * height;
    dummyGlyph = 1;
  }

  LPVOID data = ALLOC(dataSize);
  if (data) {
    memset(data, 0, dataSize);
  }
  if (srcData) {
    memcpy(data, srcData, dataSize);
  }

  dataPtr->data = data;
  dataPtr->dataSize = dataSize;
  dataPtr->freeTypeGlyphWidth = width;
  dataPtr->freeTypeGlyphHeight = height;
  dataPtr->freeTypeGlyphPitch = pitch;
  dataPtr->freeTypeGlyphAdvance = face->glyph->linearHoriAdvance;
  dataPtr->freeTypeGlyphBearing = face->glyph->metrics.horiBearingX * (1.0f / 64.0f);

  UINT yOffset = 0;
  UINT YStart = 0;
  if (height && width && data && !dummyGlyph) {
    CalculateYOffset(face, &yOffset, &YStart, pixelHeight, baseLine, height);
  }

  dataPtr->yOffset = yOffset;
  dataPtr->yStart = YStart;
  return 1;
}
