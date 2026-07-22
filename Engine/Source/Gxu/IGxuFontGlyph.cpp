#include "IGxuFont.h"

#include <freetype/freetype.h>

static int __fastcall FREETYPE_RenderGlyph(FT_Face face, unsigned int charCode, int noHinting, int monochrome) {
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

  FT_Render_Mode mode = monochrome ? ft_render_mode_mono : ft_render_mode_normal;
  return FT_Render_Glyph(face->glyph, mode) == 0;
}

static void __fastcall CalculateYOffset(
    FT_Face       face,
    unsigned int *yOffsetPtr,
    unsigned int *glyphYStart,
    unsigned int  pixelHeight,
    int           baseLineRow,
    unsigned int  glyphHeight
) {
  ASSERT(face);
  ASSERT(yOffsetPtr);
  ASSERT(glyphYStart);
  ASSERT(pixelHeight);
  ASSERT(glyphHeight);

  unsigned int yStart = 0;
  unsigned int yOffset = 0;

  if (glyphHeight <= pixelHeight) {
    int bitmapTop = face->glyph->bitmap_top;
    if (bitmapTop > baseLineRow) {
      yOffset = bitmapTop - baseLineRow;
    } else {
      yStart = baseLineRow - bitmapTop;
    }
  }

  if (pixelHeight - glyphHeight < yStart) {
    *yOffsetPtr = pixelHeight - yStart - glyphHeight;
    *glyphYStart = pixelHeight - glyphHeight;
  } else {
    *yOffsetPtr = yOffset;
    *glyphYStart = yStart;
  }
}

int __fastcall IGxuFontGlyphRenderGlyph(
    FT_Face      face,
    unsigned int pixelHeight,
    unsigned int code,
    unsigned int baseLine,
    GLYPHDATA   *dataPtr,
    int          noHinting,
    int          monochrome
) {
  FATALASSERT(face);

  FATALASSERT(pixelHeight);

  FATALASSERT(dataPtr);

  FREEIFUSED(dataPtr->data);
  dataPtr->data = 0;

  if (!FT_Get_Char_Index(face, code)) {
    return 0;
  }

  if (!FREETYPE_RenderGlyph(face, code, noHinting, monochrome)) {
    return 0;
  }

  unsigned int width = face->glyph->bitmap.width;
  unsigned int height = min(pixelHeight, face->glyph->bitmap.rows);
  ASSERT(height <= pixelHeight);

  const void  *srcData = face->glyph->bitmap.buffer;
  unsigned int pitch = face->glyph->bitmap.pitch;
  unsigned int dataSize = face->glyph->bitmap.rows * face->glyph->bitmap.pitch;
  int          dummyGlyph = 0;

  if (!width || !height || !srcData || !pitch || !dataSize) {
    width = (pixelHeight + 3) / 4;
    height = pixelHeight;
    if (!width) {
      width = pixelHeight;
    }

    pitch = monochrome ? (width + 7) & ~7 : width;
    dataSize = height * pitch;
    dummyGlyph = 1;
  }

  void *data = ALLOC(dataSize);
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

  unsigned int yOffset = 0;
  unsigned int yStart = 0;
  if (height && width && data && !dummyGlyph) {
    CalculateYOffset(face, &yOffset, &yStart, pixelHeight, baseLine, height);
  }

  dataPtr->yOffset = yOffset;
  dataPtr->yStart = yStart;
  return 1;
}
