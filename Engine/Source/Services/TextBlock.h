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

HTEXTFONT __fastcall    TextBlockGenerateFont(const char *fontName, unsigned int fontFlags, float fontHeight);
const char *__fastcall  TextBlockGetFontName(HTEXTFONT fontHandle);
unsigned int __fastcall TextBlockGetFontFlags(HTEXTFONT fontHandle);
CGxFont *__fastcall     TextBlockGetFontPtr(HTEXTFONT fontHandle);
CGxString *__fastcall   TextBlockGetStringPtr(HTEXTBLOCK text);
void __fastcall         TextBlockAddShadow(HTEXTBLOCK text, NTempest::CImVector color, const NTempest::C2Vector &shadowOffset);
HTEXTBLOCK __fastcall   TextBlockCreate(
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
void __fastcall TextBlockAnimate(HTEXTBLOCK htb, const NTempest::C3Vector &pos);
void __fastcall TextBlockUpdateColor(HTEXTBLOCK htb, const NTempest::CImVector &textColor);
void __fastcall TextBlockGetTextExtent(
    HTEXTFONT    font,
    const char  *text,
    unsigned int numChars,
    float        fontHeight,
    float       *extent,
    float        charSpacing,
    unsigned int flags
);
float __fastcall
TextBlockGetWrappedTextHeight(HTEXTFONT font, const char *text, float fontHeight, float blockWidth, float spacing, unsigned int flags);
unsigned int __fastcall TextBlockGetMaxCharsWithinWidthFromEnd(
    HTEXTFONT    font,
    const char  *text,
    float        height,
    float        maxWidth,
    unsigned int lineBytes,
    float       *extent,
    float        charSpacing,
    unsigned int flags
);
unsigned int __fastcall TextBlockWrapText(
    HTEXTFONT     font,
    const char   *text,
    float         height,
    float         maxWidth,
    unsigned int *outputList,
    unsigned int  outputListElements,
    float         charSpacing,
    unsigned int  flags
);
int __fastcall TextBlockSetGradient(HTEXTBLOCK text, int startChar, int length);
