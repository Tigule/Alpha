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

HTEXTFONT TextBlockGenerateFont(const char *fontName, unsigned int fontFlags, float fontHeight);
const char *TextBlockGetFontName(HTEXTFONT fontHandle);
unsigned int TextBlockGetFontFlags(HTEXTFONT fontHandle);
CGxFont *TextBlockGetFontPtr(HTEXTFONT fontHandle);
CGxString *TextBlockGetStringPtr(HTEXTBLOCK text);
void TextBlockAddShadow(HTEXTBLOCK text, NTempest::CImVector color, const NTempest::C2Vector &shadowOffset);
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
);
void TextBlockAnimate(HTEXTBLOCK htb, const NTempest::C3Vector &pos);
void TextBlockUpdateColor(HTEXTBLOCK htb, const NTempest::CImVector &textColor);
void TextBlockGetTextExtent(
    HTEXTFONT    font,
    const char  *text,
    unsigned int numChars,
    float        fontHeight,
    float       *extent,
    float        charSpacing,
    unsigned int flags
);
float
TextBlockGetWrappedTextHeight(HTEXTFONT font, const char *text, float fontHeight, float blockWidth, float spacing, unsigned int flags);
unsigned int TextBlockGetMaxCharsWithinWidthFromEnd(
    HTEXTFONT    font,
    const char  *text,
    float        height,
    float        maxWidth,
    unsigned int lineBytes,
    float       *extent,
    float        charSpacing,
    unsigned int flags
);
unsigned int TextBlockWrapText(
    HTEXTFONT     font,
    const char   *text,
    float         height,
    float         maxWidth,
    unsigned int *outputList,
    unsigned int  outputListElements,
    float         charSpacing,
    unsigned int  flags
);
int TextBlockSetGradient(HTEXTBLOCK text, int startChar, int length);
