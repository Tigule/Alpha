#include <Base/Base.h>

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

HTEXTFONT TextBlockGenerateFont(LPCSTR fontName, UINT fontFlags, float fontHeight) {
  char         buffer[0x114];
  FONTHASHOBJ *fontObj;
  UINT         gxFontFlags;

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

LPCSTR TextBlockGetFontName(HTEXTFONT fontHandle) {
  FATALASSERT(fontHandle);

  return GxuFontGetFontName(reinterpret_cast<FONTHASHOBJ *>(fontHandle)->font);
}

UINT TextBlockGetFontFlags(HTEXTFONT fontHandle) {
  UINT flags;
  UINT textFlags;

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

float TextBlockGetOneToOneHeight(HTEXTFONT__ *fontHandle) {
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
    LPCSTR                     text,
    const NTempest::CImVector &color,
    const NTempest::C3Vector  &pos,
    float                      fontHeight,
    float                      blockWidth,
    float                      blockHeight,
    UINT                       flags,
    float                      charSpacing,
    float                      lineSpacing
) {
  LPVOID             storage;
  TEXTBLOCK         *textPtr;
  NTempest::C3Vector position;
  UINT               gxFlags = 0;

  FATALASSERT(font);

  FATALASSERT(text);

  storage = SMemAlloc(sizeof(TEXTBLOCK), "HTEXTBLOCK", SERR_LINECODE_OBJECT, 0);
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

  GxuFontCreateString(
      reinterpret_cast<FONTHASHOBJ *>(font)->font, text, fontHeight, position, blockWidth, blockHeight, lineSpacing, textPtr->string,
      flags & 0x8    ? GxVJ_Top
      : flags & 0x20 ? GxVJ_Bottom
                     : GxVJ_Middle,
      flags & 0x4   ? GxHJ_Right
      : flags & 0x2 ? GxHJ_Center
      : flags & 0x1 ? GxHJ_Left
                    : GxHJ_Center,
      gxFlags, color, charSpacing
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

void TextBlockRender(HTEXTBLOCK__ *htb) {
  FATALASSERT(htb);
  GxuFontRender(reinterpret_cast<TEXTBLOCK *>(htb)->string);
}

void TextBlockUpdateColor(HTEXTBLOCK htb, const NTempest::CImVector &textColor) {
  FATALASSERT(htb);

  GxuFontSetStringColor(reinterpret_cast<TEXTBLOCK *>(htb)->string, textColor);
}

float TextBlockGetHeight(HTEXTBLOCK__ *htb) {
  FATALASSERT(htb);
  float height = GxuFontGetStringHeight(reinterpret_cast<TEXTBLOCK *>(htb)->string);
  NDCToDDC(0.0f, height, 0, &height);
  return height;
}

void TextBlockGetTextExtent(HTEXTFONT font, LPCSTR text, UINT numChars, float fontHeight, float *extent, float charSpacing, UINT flags) {
  FONTHASHOBJ *fontPtr;
  UINT         gxFlags;

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

void TextBlockGetWrapPoint(
    HTEXTFONT__ *font,
    LPCSTR       text,
    float        fontHeight,
    float        blockWidth,
    UINT        *numBytes,
    float       *pExtent,
    LPCSTR      *pNextText,
    float        spacing,
    UINT         flags
) {
  FATALASSERT(font);
  FATALASSERT(text);
  FONTHASHOBJ *fontPtr = reinterpret_cast<FONTHASHOBJ *>(font);
  FATALASSERT(fontPtr->font);
  fontHeight = DDCToNDCHeight(fontHeight);
  blockWidth = DDCToNDCWidth(blockWidth);
  GxuFontGetWrapPoint(fontPtr->font, text, fontHeight, blockWidth, numBytes, pExtent, pNextText, spacing, flags);
  NDCToDDC(*pExtent, 0.0f, pExtent, 0);
}

float TextBlockGetWrappedTextHeight(HTEXTFONT font, LPCSTR text, float fontHeight, float blockWidth, float spacing, UINT flags) {
  FONTHASHOBJ *fontPtr;
  UINT         gxFlags;
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

UINT TextBlockGetMaxCharsWithinWidth(
    HTEXTFONT__ *font,
    LPCSTR       text,
    float        height,
    float        maxWidth,
    UINT         lineBytes,
    float       *extent,
    float        charSpacing,
    UINT         flags
) {
  FATALASSERT(font);
  FATALASSERT(text);
  FONTHASHOBJ *fontPtr = reinterpret_cast<FONTHASHOBJ *>(font);
  FATALASSERT(fontPtr->font);
  height = DDCToNDCHeight(height);
  maxWidth = DDCToNDCWidth(maxWidth);
  UINT gxFlags = (flags & 0x100) != 0;
  if (flags & 0x200)
    gxFlags |= 0x4;
  if (flags & 0x400)
    gxFlags |= 0x8;
  if (flags & 0x800)
    gxFlags |= 0x10;
  if (flags & 0x40)
    gxFlags |= 0x2;
  if (flags & 0x80)
    gxFlags |= 0x20;
  if (flags & 0x1000)
    gxFlags |= 0x40;
  if (flags & 0x2000)
    gxFlags |= 0x100;
  if (flags & 0x4000)
    gxFlags |= 0x200;
  if (flags & 0x8000)
    gxFlags |= 0x400;
  if (flags & 0x10000)
    gxFlags |= 0x800;
  UINT chars = GxuFontGetMaxCharsWithinWidth(fontPtr->font, text, height, maxWidth, lineBytes, extent, charSpacing, gxFlags);
  NDCToDDC(*extent, 0.0f, extent, 0);
  return chars;
}

UINT TextBlockGetMaxCharsWithinWidthFromEnd(
    HTEXTFONT font,
    LPCSTR    text,
    float     height,
    float     maxWidth,
    UINT      lineBytes,
    float    *extent,
    float     charSpacing,
    UINT      flags
) {
  FONTHASHOBJ *fontPtr;
  UINT         gxFlags;
  UINT         chars;

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

UINT TextBlockWrapText(
    HTEXTFONT font,
    LPCSTR    text,
    float     height,
    float     maxWidth,
    UINT     *outputList,
    UINT      outputListElements,
    float     charSpacing,
    UINT      flags
) {
  FONTHASHOBJ *fontPtr;
  UINT         gxFlags;

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
