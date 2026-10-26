#pragma once

#include <Tempest/c2vector.h>
#include <Tempest/c3vector.h>
#include <Tempest/c44matrix.h>
#include <Tempest/cimvector.h>
#include <Tempest/crect.h>

struct CGxFont;
struct CGxString;
struct CGxStringBatch;

enum EGxFontVJusts {
  GxVJ_Top = 0,
  GxVJ_Middle = 1,
  GxVJ_Bottom = 2,
  GxVJ_Last = 3
};

enum EGxFontHJusts {
  GxHJ_Left = 0,
  GxHJ_Center = 1,
  GxHJ_Right = 2,
  GxHJ_Last = 3
};

struct GXUFONTHYPERLINKINFO {
  NTempest::CRect extent;
  LPCSTR          link;
  UINT            linkLength;
};

void GxuFontInitialize();
void GxuFontShutdown();
void GxuFontWindowSizeChanged();

int  GxuFontCreateFont(LPCSTR name, float fontHeight, CGxFont *&face, UINT flags);
void GxuFontDestroyFont(CGxFont *&face);

int GxuFontCreateString(
    CGxFont                   *face,
    LPCSTR                     text,
    float                      fontHeight,
    const NTempest::C3Vector  &position,
    float                      blockWidth,
    float                      blockHeight,
    float                      spacing,
    CGxString                *&string,
    EGxFontVJusts              vertJustification,
    EGxFontHJusts              horzJustification,
    UINT                       flags,
    const NTempest::CImVector &color,
    float                      charSpacing
);
void  GxuFontDestroyString(CGxString *&string);
void  GxuFontRender(CGxString *string);
void  GxuFontRender(CGxString *string, const NTempest::C44Matrix &xform);
float GxuFontGetStringHeight(CGxString *string);
void  GxuFontAddShadow(CGxString *string, const NTempest::CImVector &color, const NTempest::C2Vector &offset);
int   GxuFontStringSetGradient(CGxString *string, int startCharacter, int length);
UINT  GxuFontStringHyperLinkInfo(const CGxString *string, const GXUFONTHYPERLINKINFO *&list);
int   GxuFontRenderString(
    CGxFont                  *font,
    LPCSTR                    text,
    float                     textHeight,
    const NTempest::C3Vector &position,
    NTempest::CImVector       color,
    float                     blockWidth,
    float                     blockHeight,
    EGxFontVJusts             vertJustification,
    EGxFontHJusts             horzJustification,
    UINT                      flags,
    float                     spacing,
    float                     charSpacing
);

CGxStringBatch *GxuFontCreateBatch();
BOOL            GxuFontAddToBatch(CGxStringBatch *batch, CGxString *string);
BOOL            GxuFontRemoveFromBatch(CGxString *string);
BOOL            GxuFontRenderBatch(CGxStringBatch *batch);
BOOL            GxuFontClearBatch(CGxStringBatch *batch);
BOOL            GxuFontDestroyBatch(CGxStringBatch *batch);
BOOL            GxuFontAddToInternalBatch(CGxString *string);
void            GxuFontRenderInternalBatch();

void GxuFontGetTextExtent(CGxFont *face, LPCSTR text, UINT numBytes, float height, float *extent, float charSpacing, UINT flags);
void GxuFontGetWrapPoint(
    CGxFont *face,
    LPCSTR   text,
    float    fontHeight,
    float    blockWidth,
    UINT    *numBytes,
    float   *pExtent,
    LPCSTR  *pNextText,
    float    spacing,
    UINT     flags
);
UINT GxuFontGetMaxCharsWithinWidth(
    CGxFont *face,
    LPCSTR   text,
    float    height,
    float    maxWidth,
    UINT     lineBytes,
    float   *extent,
    float    charSpacing,
    UINT     flags
);
UINT GxuFontGetMaxCharsWithinWidthFromEnd(
    CGxFont *font,
    LPCSTR   text,
    float    fontHeight,
    float    width,
    UINT     lineBytes,
    float   *extent,
    float    charSpacing,
    UINT     flags
);
LPCSTR
GxuFontStripEscapeCodes(LPCSTR inputString, UINT numBytes, UINT flags, char *buffer, UINT bufferSize);
BOOL       GxuFontGetLastColorCode(LPCSTR string, UINT numBytes, NTempest::CImVector *color);
BOOL       GxuFontGenerateColorString(char *buf, UINT bufSize, const NTempest::CImVector &color);
int        GxuFontSetStringColor(CGxString *string, NTempest::CImVector newColor);
void       GxuFontSetStringPosition(CGxString *string, const NTempest::C3Vector &pos);
void       GxuFontSetCharSpacing(CGxString *string, float spacing);
void       GxuFontRemoveShadow(CGxString *string);
CGxString *GxuFontDuplicateString(const CGxString *rhs);
BOOL       GxuFontGetStringWidth(CGxString *string, float *width);
BOOL       GxuFontGetStringHeight(CGxString *string, float *height);

float  GxuFontGetOneToOneHeight(CGxFont *font);
LPCSTR GxuFontGetFontName(CGxFont *fontName);
UINT   GxuFontGetFontFlags(CGxFont *fontName);
float  GxuFontGetWrappedTextHeight(CGxFont *face, LPCSTR text, float fontHeight, float blockWidth, float lineSpacing, UINT flags);
UINT   GxuFontWrapText(
    CGxFont *font,
    LPCSTR   text,
    UINT     lineBytes,
    float    fontHeight,
    float    blockWidth,
    UINT    *outputList,
    UINT     outputListElements,
    float    charSpacing,
    UINT     flags
);
void GxuFontSetUseAdvanceWidth(int useAdvanceWidth);
