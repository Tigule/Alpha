#include "TextBlock.h"

#include <Base/Coordinate.h>
#include <Gxu/IGxuFont.h>

#include <stpl.h>

struct FONTHASHOBJ : public CHandleObject, public TSHashObject<FONTHASHOBJ, HASHKEY_STR> {
  FONTHASHOBJ() : font(0) {
  }

  virtual ~FONTHASHOBJ() {
    if (font) {
      GxuFontDestroyFont(font);
    }
  }

  CGxFont *font;
};

static TSHashTable<FONTHASHOBJ, HASHKEY_STR> s_fontHash;

HTEXTFONT TextBlockGenerateFont(const char *fontName, unsigned int fontFlags, float fontHeight) {
  char         buffer[0x114];
  FONTHASHOBJ *fontObj;
  unsigned int gxFontFlags;

  FATALASSERT(fontName);

  FATALASSERT(*fontName);

  fontHeight = DDCToNDCHeight(fontHeight);
  if (fontHeight >= 1.0f) {
    fontHeight = 1.0f;
  }

  SStrPrintf(buffer, sizeof(buffer), "%s-%d-%f", fontName, fontFlags, fontHeight);

  fontObj = s_fontHash.Ptr(buffer);
  if (fontObj) {
    ASSERT(fontObj->font);
    return reinterpret_cast<HTEXTFONT>(HandleCreate(fontObj, "HTEXTFONT"));
  }

  fontObj = s_fontHash.New(buffer, 0, 0);
  gxFontFlags = (fontFlags & 0x1) != 0;
  if (fontFlags & 0x4) {
    gxFontFlags |= 0x8;
  }
  if (fontFlags & 0x8) {
    gxFontFlags |= 0x10;
  }
  if (fontFlags & 0x2) {
    gxFontFlags |= 0x2;
  }

  if (GxuFontCreateFont(fontName, fontHeight, fontObj->font, gxFontFlags)) {
    return reinterpret_cast<HTEXTFONT>(HandleCreate(fontObj, "HTEXTFONT"));
  }

  s_fontHash.Delete(fontObj);
  return 0;
}

const char *TextBlockGetFontName(HTEXTFONT fontHandle) {
  FATALASSERT(fontHandle);

  return GxuFontGetFontName(reinterpret_cast<FONTHASHOBJ *>(fontHandle)->font);
}

unsigned int TextBlockGetFontFlags(HTEXTFONT fontHandle) {
  unsigned int flags;
  unsigned int textFlags;

  FATALASSERT(fontHandle);

  flags = GxuFontGetFontFlags(reinterpret_cast<FONTHASHOBJ *>(fontHandle)->font);
  textFlags = (flags & 0x1) != 0;
  if (flags & 0x8) {
    textFlags |= 0x4;
  }
  if (flags & 0x2) {
    textFlags |= 0x2;
  }
  if (flags & 0x10) {
    textFlags |= 0x8;
  }

  return textFlags;
}

CGxFont *TextBlockGetFontPtr(HTEXTFONT fontHandle) {
  FATALASSERT(fontHandle);

  return reinterpret_cast<FONTHASHOBJ *>(fontHandle)->font;
}

CGxString *TextBlockGetStringPtr(HTEXTBLOCK text) {
  FATALASSERT(text);

  return reinterpret_cast<TEXTBLOCK *>(text)->string;
}

float TextBlockGetOneToOneHeight(HTEXTFONT__* fontHandle) {
  ASSERT(fontHandle);
  FONTHASHOBJ *fontPtr = reinterpret_cast<FONTHASHOBJ *>(fontHandle);
  ASSERT(fontPtr->font);
  float height = GxuFontGetOneToOneHeight(fontPtr->font);
  NDCToDDC(0.0f, height, 0, &height);
  return height;
}

void TextBlockAddShadow(HTEXTBLOCK text, NTempest::CImVector color, const NTempest::C2Vector &shadowOffset) {
  NTempest::C2Vector offset;

  FATALASSERT(text);

  DDCToNDC(shadowOffset.x, shadowOffset.y, &offset.x, &offset.y);
  GxuFontAddShadow(reinterpret_cast<TEXTBLOCK *>(text)->string, color, offset);
}

HTEXTBLOCK TextBlockCreate(
    HTEXTFONT                  font,
    const char                *text,
    const NTempest::CImVector &color,
    const NTempest::C3Vector  &pos,
    float                      fontHeight,
    float                      blockWidth,
    float                      blockHeight,
    unsigned int               flags,
    float                      charSpacing,
    float                      lineSpacing
) {
  void              *storage;
  TEXTBLOCK         *textPtr;
  NTempest::C3Vector position;
  unsigned int       gxFlags = 0;
  EGxFontHJusts      horzJustification = GxHJ_Center;
  EGxFontVJusts      vertJustification = GxVJ_Middle;

  FATALASSERT(font);

  FATALASSERT(text);

  storage = SMemAlloc(sizeof(TEXTBLOCK), "HTEXTBLOCK", -2, 0);
  textPtr = storage ? new (storage) TEXTBLOCK : 0;

  position.z = pos.z;
  DDCToNDC(pos.x, pos.y, &position.x, &position.y);
  DDCToNDC(blockWidth, blockHeight, &blockWidth, &blockHeight);
  DDCToNDC(0.0f, fontHeight, 0, &fontHeight);

  if (flags & 0x100) {
    gxFlags |= 0x1;
  }
  if (flags & 0x200) {
    gxFlags |= 0x4;
  }
  if (flags & 0x400) {
    gxFlags |= 0x8;
  }
  if (flags & 0x800) {
    gxFlags |= 0x10;
  }
  if (flags & 0x40) {
    gxFlags |= 0x2;
  }
  if (flags & 0x80) {
    gxFlags |= 0x20;
  }
  if (flags & 0x1000) {
    gxFlags |= 0x40;
  }
  if (flags & 0x2000) {
    gxFlags |= 0x100;
  }
  if (flags & 0x4000) {
    gxFlags |= 0x200;
  }
  if (flags & 0x8000) {
    gxFlags |= 0x400;
  }
  if (flags & 0x10000) {
    gxFlags |= 0x800;
  }

  if (flags & 0x4) {
    horzJustification = GxHJ_Right;
  } else if (flags & 0x2) {
    horzJustification = GxHJ_Center;
  } else if (flags & 0x1) {
    horzJustification = GxHJ_Left;
  }

  if (flags & 0x8) {
    vertJustification = GxVJ_Top;
  } else if (flags & 0x20) {
    vertJustification = GxVJ_Bottom;
  } else if (flags & 0x10) {
    vertJustification = GxVJ_Middle;
  }

  GxuFontCreateString(
      reinterpret_cast<FONTHASHOBJ *>(font)->font, text, fontHeight, position, blockWidth, blockHeight, lineSpacing, textPtr->string,
      vertJustification, horzJustification, gxFlags, color, charSpacing
  );

  return reinterpret_cast<HTEXTBLOCK>(HandleCreate(textPtr, "HTEXTBLOCK"));
}

void TextBlockAnimate(HTEXTBLOCK htb, const NTempest::C3Vector &pos) {
  FATALASSERT(htb);

  NTempest::C3Vector position;
  position.z = pos.z;
  DDCToNDC(pos.x, pos.y, &position.x, &position.y);
  GxuFontSetStringPosition(reinterpret_cast<TEXTBLOCK *>(htb)->string, position);
}

void TextBlockRender(HTEXTBLOCK__* htb) {
  FATALASSERT(htb);
  GxuFontRender(reinterpret_cast<TEXTBLOCK *>(htb)->string);
}

void TextBlockUpdateColor(HTEXTBLOCK htb, const NTempest::CImVector &textColor) {
  FATALASSERT(htb);

  GxuFontSetStringColor(reinterpret_cast<TEXTBLOCK *>(htb)->string, textColor);
}

float TextBlockGetHeight(HTEXTBLOCK__* htb) {
  FATALASSERT(htb);
  float height = GxuFontGetStringHeight(reinterpret_cast<TEXTBLOCK *>(htb)->string);
  NDCToDDC(0.0f, height, 0, &height);
  return height;
}

void TextBlockGetTextExtent(
    HTEXTFONT    font,
    const char  *text,
    unsigned int numChars,
    float        fontHeight,
    float       *extent,
    float        charSpacing,
    unsigned int flags
) {
  FONTHASHOBJ *fontPtr;
  unsigned int gxFlags;

  FATALASSERT(font);

  FATALASSERT(text);

  FATALASSERT(extent);

  *extent = 0.0f;
  fontPtr = reinterpret_cast<FONTHASHOBJ *>(font);
  FATALASSERT(fontPtr->font);

  DDCToNDC(0.0f, fontHeight, 0, &fontHeight);
  gxFlags = (flags & 0x100) != 0;
  if (flags & 0x200) {
    gxFlags |= 0x4;
  }
  if (flags & 0x400) {
    gxFlags |= 0x8;
  }
  if (flags & 0x800) {
    gxFlags |= 0x10;
  }
  if (flags & 0x40) {
    gxFlags |= 0x2;
  }
  if (flags & 0x80) {
    gxFlags |= 0x20;
  }
  if (flags & 0x1000) {
    gxFlags |= 0x40;
  }
  if (flags & 0x2000) {
    gxFlags |= 0x100;
  }
  if (flags & 0x4000) {
    gxFlags |= 0x200;
  }
  if (flags & 0x8000) {
    gxFlags |= 0x400;
  }
  if (flags & 0x10000) {
    gxFlags |= 0x800;
  }

  GxuFontGetTextExtent(fontPtr->font, text, numChars, fontHeight, extent, charSpacing, gxFlags);
  NDCToDDC(*extent, 0.0f, extent, 0);
}

void TextBlockGetWrapPoint(HTEXTFONT__* font, const char* text, float fontHeight, float blockWidth, unsigned int* numBytes, float* pExtent, const char** pNextText, float spacing, unsigned int flags) {
  FATALASSERT(font);
  FATALASSERT(text);
  FONTHASHOBJ *fontPtr = reinterpret_cast<FONTHASHOBJ *>(font);
  FATALASSERT(fontPtr->font);
  fontHeight = DDCToNDCHeight(fontHeight);
  blockWidth = DDCToNDCWidth(blockWidth);
  GxuFontGetWrapPoint(fontPtr->font, text, fontHeight, blockWidth, numBytes, pExtent, pNextText, spacing, flags);
  NDCToDDC(*pExtent, 0.0f, pExtent, 0);
}

float
TextBlockGetWrappedTextHeight(HTEXTFONT font, const char *text, float fontHeight, float blockWidth, float spacing, unsigned int flags) {
  FONTHASHOBJ *fontPtr;
  unsigned int gxFlags;
  float        height;

  FATALASSERT(font);

  FATALASSERT(text);

  fontPtr = reinterpret_cast<FONTHASHOBJ *>(font);
  FATALASSERT(fontPtr->font);

  fontHeight = DDCToNDCHeight(fontHeight);
  blockWidth = DDCToNDCWidth(blockWidth);
  gxFlags = (flags & 0x100) != 0;
  if (flags & 0x200) {
    gxFlags |= 0x4;
  }
  if (flags & 0x400) {
    gxFlags |= 0x8;
  }
  if (flags & 0x800) {
    gxFlags |= 0x10;
  }
  if (flags & 0x40) {
    gxFlags |= 0x2;
  }
  if (flags & 0x80) {
    gxFlags |= 0x20;
  }
  if (flags & 0x1000) {
    gxFlags |= 0x40;
  }
  if (flags & 0x2000) {
    gxFlags |= 0x100;
  }
  if (flags & 0x4000) {
    gxFlags |= 0x200;
  }
  if (flags & 0x8000) {
    gxFlags |= 0x400;
  }
  if (flags & 0x10000) {
    gxFlags |= 0x800;
  }

  height = GxuFontGetWrappedTextHeight(fontPtr->font, text, fontHeight, blockWidth, spacing, gxFlags);
  NDCToDDC(0.0f, height, 0, &height);
  return height;
}

unsigned int TextBlockGetMaxCharsWithinWidth(HTEXTFONT__* font, const char* text, float height, float maxWidth, unsigned int lineBytes, float* extent, float charSpacing, unsigned int flags) {
  FATALASSERT(font);
  FATALASSERT(text);
  FONTHASHOBJ *fontPtr = reinterpret_cast<FONTHASHOBJ *>(font);
  FATALASSERT(fontPtr->font);
  height = DDCToNDCHeight(height);
  maxWidth = DDCToNDCWidth(maxWidth);
  unsigned int gxFlags = (flags & 0x100) != 0;
  if (flags & 0x200) gxFlags |= 0x4;
  if (flags & 0x400) gxFlags |= 0x8;
  if (flags & 0x800) gxFlags |= 0x10;
  if (flags & 0x40) gxFlags |= 0x2;
  if (flags & 0x80) gxFlags |= 0x20;
  if (flags & 0x1000) gxFlags |= 0x40;
  if (flags & 0x2000) gxFlags |= 0x100;
  if (flags & 0x4000) gxFlags |= 0x200;
  if (flags & 0x8000) gxFlags |= 0x400;
  if (flags & 0x10000) gxFlags |= 0x800;
  unsigned int chars =
      GxuFontGetMaxCharsWithinWidth(fontPtr->font, text, height, maxWidth, lineBytes, extent, charSpacing, gxFlags);
  NDCToDDC(*extent, 0.0f, extent, 0);
  return chars;
}

unsigned int TextBlockGetMaxCharsWithinWidthFromEnd(
    HTEXTFONT    font,
    const char  *text,
    float        height,
    float        maxWidth,
    unsigned int lineBytes,
    float       *extent,
    float        charSpacing,
    unsigned int flags
) {
  FONTHASHOBJ *fontPtr;
  unsigned int gxFlags;
  unsigned int chars;

  FATALASSERT(font);

  FATALASSERT(text);

  fontPtr = reinterpret_cast<FONTHASHOBJ *>(font);
  FATALASSERT(fontPtr->font);

  DDCToNDC(0.0f, height, 0, &height);
  DDCToNDC(maxWidth, 0.0f, &maxWidth, 0);
  gxFlags = (flags & 0x100) != 0;
  if (flags & 0x200) {
    gxFlags |= 0x4;
  }
  if (flags & 0x400) {
    gxFlags |= 0x8;
  }
  if (flags & 0x800) {
    gxFlags |= 0x10;
  }
  if (flags & 0x40) {
    gxFlags |= 0x2;
  }
  if (flags & 0x80) {
    gxFlags |= 0x20;
  }
  if (flags & 0x1000) {
    gxFlags |= 0x40;
  }
  if (flags & 0x2000) {
    gxFlags |= 0x100;
  }
  if (flags & 0x4000) {
    gxFlags |= 0x200;
  }
  if (flags & 0x8000) {
    gxFlags |= 0x400;
  }
  if (flags & 0x10000) {
    gxFlags |= 0x800;
  }

  chars = GxuFontGetMaxCharsWithinWidthFromEnd(fontPtr->font, text, height, maxWidth, lineBytes, extent, charSpacing, gxFlags);
  NDCToDDC(*extent, 0.0f, extent, 0);
  return chars;
}

unsigned int TextBlockWrapText(
    HTEXTFONT     font,
    const char   *text,
    float         height,
    float         maxWidth,
    unsigned int *outputList,
    unsigned int  outputListElements,
    float         charSpacing,
    unsigned int  flags
) {
  FONTHASHOBJ *fontPtr;
  unsigned int gxFlags;

  FATALASSERT(font);

  FATALASSERT(text);

  fontPtr = reinterpret_cast<FONTHASHOBJ *>(font);
  FATALASSERT(fontPtr->font);

  DDCToNDC(0.0f, height, 0, &height);
  DDCToNDC(maxWidth, 0.0f, &maxWidth, 0);
  gxFlags = (flags & 0x100) != 0;
  if (flags & 0x200) {
    gxFlags |= 0x4;
  }
  if (flags & 0x400) {
    gxFlags |= 0x8;
  }
  if (flags & 0x800) {
    gxFlags |= 0x10;
  }
  if (flags & 0x40) {
    gxFlags |= 0x2;
  }
  if (flags & 0x80) {
    gxFlags |= 0x20;
  }
  if (flags & 0x1000) {
    gxFlags |= 0x40;
  }
  if (flags & 0x2000) {
    gxFlags |= 0x100;
  }
  if (flags & 0x4000) {
    gxFlags |= 0x200;
  }
  if (flags & 0x8000) {
    gxFlags |= 0x400;
  }
  if (flags & 0x10000) {
    gxFlags |= 0x800;
  }

  return GxuFontWrapText(fontPtr->font, text, SStrLen(text), height, maxWidth, outputList, outputListElements, charSpacing, gxFlags);
}

int TextBlockSetGradient(HTEXTBLOCK text, int startChar, int length) {
  ASSERT(text);

  TEXTBLOCK *textPtr = reinterpret_cast<TEXTBLOCK *>(text);
  ASSERT(textPtr);

  return GxuFontStringSetGradient(textPtr->string, startChar, length);
}
