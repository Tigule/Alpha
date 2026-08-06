#pragma once

#include <Base/Handle.h>
#include <Gxu/IGxuFont.h>
#include <Tempest/c2vector.h>
#include <Tempest/c3vector.h>
#include <Tempest/cimvector.h>

struct CGxFont;
struct CGxString;

DECLARE_DERIVED_HANDLE(HTEXTFONT, HOBJECT);
DECLARE_DERIVED_HANDLE(HTEXTBLOCK, HOBJECT);

struct TEXTBLOCK : public CHandleObject {
  TEXTBLOCK() : string(0) {
  }

  virtual ~TEXTBLOCK() {
    GxuFontDestroyString(string);
  }

  CGxString *string;
};

HTEXTFONT  TextBlockGenerateFont(LPCSTR fontName, UINT fontFlags, float fontHeight);
LPCSTR     TextBlockGetFontName(HTEXTFONT fontHandle);
UINT       TextBlockGetFontFlags(HTEXTFONT fontHandle);
CGxFont   *TextBlockGetFontPtr(HTEXTFONT fontHandle);
CGxString *TextBlockGetStringPtr(HTEXTBLOCK text);
void       TextBlockAddShadow(HTEXTBLOCK text, NTempest::CImVector color, const NTempest::C2Vector &shadowOffset);
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
);
void  TextBlockAnimate(HTEXTBLOCK htb, const NTempest::C3Vector &pos);
void  TextBlockUpdateColor(HTEXTBLOCK htb, const NTempest::CImVector &textColor);
void  TextBlockGetTextExtent(HTEXTFONT font, LPCSTR text, UINT numChars, float fontHeight, float *extent, float charSpacing, UINT flags);
float TextBlockGetWrappedTextHeight(HTEXTFONT font, LPCSTR text, float fontHeight, float blockWidth, float spacing, UINT flags);
UINT  TextBlockGetMaxCharsWithinWidthFromEnd(
    HTEXTFONT font,
    LPCSTR    text,
    float     height,
    float     maxWidth,
    UINT      lineBytes,
    float    *extent,
    float     charSpacing,
    UINT      flags
);
UINT TextBlockWrapText(
    HTEXTFONT font,
    LPCSTR    text,
    float     height,
    float     maxWidth,
    UINT     *outputList,
    UINT      outputListElements,
    float     charSpacing,
    UINT      flags
);
int TextBlockSetGradient(HTEXTBLOCK text, int startChar, int length);
