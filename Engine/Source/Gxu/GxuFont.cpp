#include "IGxuFont.h"

#include <Gx/Gx.h>

#include <freetype/ftmodule.h>

#include <malloc.h>
#include <new>
#include <typeinfo>

static void *FreeTypeAllocFunction(FT_Memory memory, long size) {
  ASSERT(size > 0);
  return ALLOC(size);
}

static void FreeTypeFreeFunction(FT_Memory memory, void *block) {
  FREEIFUSED(block);
}

static void *FreeTypeReallocFunction(FT_Memory memory, long currentSize, long newSize, void *block) {
  ASSERT(newSize > 0);
  return SMemReAlloc(block, newSize, __FILE__, __LINE__, 0);
}

static FT_Library                                         s_FTLibrary;
static LISTDECL(CGxFont, s_fonts);
static LISTDECL(CGxStringBatch, s_unusedBatches);
static float                                              s_pixelHeight;
static float                                              s_pixelWidth;
static CGxStringBatch                                     s_stringBatch;
static FT_MemoryRec_ s_GxuMemoryRecord = {0, FreeTypeAllocFunction, FreeTypeFreeFunction, FreeTypeReallocFunction};

float SignOf(float value) {
  return value >= 0.0f ? 1.0f : -1.0f;
}
FT_LibraryRec_ *GetFreeTypeLibrary() {
  return s_FTLibrary;
}
unsigned int GetScreenPixelHeight() {
  return g_heightPixels;
}
unsigned int GetScreenPixelWidth() {
  return g_widthPixels;
}
float ScreenToPixelHeight(int billboarded, float height) {
  float pixelCoords;

  if (billboarded) {
    return height;
  }

  pixelCoords = g_heightPixels * height;
  return static_cast<float>(static_cast<int>(pixelCoords + SignOf(pixelCoords) * 0.5f));
}
float ScreenToPixelWidth(int billboarded, float width) {
  float pixelCoords;

  if (billboarded) {
    return width;
  }

  pixelCoords = g_widthPixels * width;
  return static_cast<float>(static_cast<int>(pixelCoords + SignOf(pixelCoords) * 0.5f));
}
void GxuFontWindowSizeChanged() {
  static NTempest::CRect s_currentRect;
  NTempest::CRect        rect;

  GxCapsWindowSize(rect);
  if (rect.r - rect.l == 0.0f || rect.b - rect.t == 0.0f) {
    rect.t = 0.0f;
    rect.l = 0.0f;
    rect.b = 480.0f;
    rect.r = 640.0f;
  }

  if (rect.t == s_currentRect.t && rect.l == s_currentRect.l && rect.b == s_currentRect.b && rect.r == s_currentRect.r) {
    return;
  }

  s_currentRect = rect;
  g_widthPixels = static_cast<unsigned int>(rect.r - rect.l);
  g_heightPixels = static_cast<unsigned int>(rect.b - rect.t);
  s_pixelWidth = g_widthPixels ? 1.0f / g_widthPixels : 0.0f;
  s_pixelHeight = g_heightPixels ? 1.0f / g_heightPixels : 0.0f;

  ITERATELIST(CGxFont, s_fonts, font) {
    font->HandleScreenSizeChange();
  }
}
void GxuFontInitialize() {
  FT_Error error = FT_New_Library(&s_GxuMemoryRecord, &s_FTLibrary);

  FT_Add_Default_Modules(s_FTLibrary);
  ASSERT(!error);
  ASSERT(s_FTLibrary);
  GxuFontWindowSizeChanged();
  IGxuStringInitialize();
}
void GxuFontShutdown() {
  IGxuStringShutdown();
  g_strings.Clear();
  s_fonts.Clear();
  s_unusedBatches.Clear();
  g_freeStrings.Clear();
  g_freeTextLineTextures.Clear();
  g_freeTextLines.Clear();

  if (s_FTLibrary) {
    FT_Done_Library(s_FTLibrary);
  }
  s_FTLibrary = 0;
}

int GxuFontCreateFont(const char *name, float fontHeight, CGxFont *&face, unsigned int flags) {
  CGxFont *newFace;
  int      result;

  FATALASSERT(name);

  FATALASSERT(*name);

  FATALASSERT((fontHeight < 1.0f) && (fontHeight > 0));

  ASSERT(s_FTLibrary);

  newFace = NEWZERO(CGxFont);
  s_fonts.LinkNode(newFace, LIST_TAIL, 0);
  ASSERT(newFace);

  if (flags & 0x8) {
    flags |= 0x1;
  }
  if (flags & 0x10) {
    flags |= 0x2;
  }

  result = newFace->Initialize(name, flags, fontHeight);
  if (!result) {
    newFace->~CGxFont();
    SMemFree(newFace, typeid(CGxFont).raw_name(), -2, 0);
    newFace = 0;
  }

  face = newFace;
  return result;
}
const char *GxuFontGetFontName(CGxFont *fontName) {
  return fontName ? fontName->GetName() : 0;
}
unsigned int GxuFontGetFontFlags(CGxFont *fontName) {
  return fontName ? fontName->m_flags : 0;
}
void GxuFontDestroyFont(CGxFont *&face) {
  CGxFont *oldFace = face;

  if (oldFace) {
    oldFace->~CGxFont();
    SMemFree(oldFace, typeid(CGxFont).raw_name(), -2, 0);
  }
  face = 0;
}
int GxuFontCreateString(
    CGxFont                   *face,
    const char                *text,
    float                      fontHeight,
    const NTempest::C3Vector  &position,
    float                      blockWidth,
    float                      blockHeight,
    float                      spacing,
    CGxString                *&string,
    EGxFontVJusts              vertJustification,
    EGxFontHJusts              horzJustification,
    unsigned int               flags,
    const NTempest::CImVector &color,
    float                      charSpacing
) {
  CGxString *newString;
  int        result;

  FATALASSERT(face);

  FATALASSERT(text);

  FATALASSERT(fontHeight || (flags & EGxStringFlags_FixedSize));

  FATALASSERT(blockWidth);

  FATALASSERT(blockHeight);

  FATALASSERT(vertJustification < GxVJ_Last);

  FATALASSERT(horzJustification < GxHJ_Last);

  if (flags & 0x80) {
    flags &= ~0x1u;
  }
  if (face->m_flags & 0x9) {
    flags &= ~0x1u;
  }

  newString = CGxString::GetNewString(1);
  result =
      newString->Initialize(fontHeight, position, blockWidth, blockHeight, face, text, vertJustification, horzJustification, spacing, flags, color);
  if (!result) {
    GxuFontDestroyString(newString);
  }

  string = newString;
  return result;
}
void GxuFontDestroyString(CGxString *&string) {
  if (string) {
    string->Unlink();
    string->Recycle();
    string = 0;
  }
}
int GxuFontRenderString(
    CGxFont                  *font,
    const char               *text,
    float                     textHeight,
    const NTempest::C3Vector &position,
    NTempest::CImVector       color,
    float                     blockWidth,
    float                     blockHeight,
    EGxFontVJusts             vertJustification,
    EGxFontHJusts             horzJustification,
    unsigned int              flags,
    float                     spacing,
    float                     charSpacing
) {
  CGxString *newString;
  int        result;

  FATALASSERT(textHeight || (flags & EGxStringFlags_FixedSize));

  FATALASSERT(font);

  FATALASSERT(text);

  newString = CGxString::GetNewString(0);
  result =
      newString->Initialize(textHeight, position, blockWidth, blockHeight, font, text, vertJustification, horzJustification, spacing, flags, color);
  if (result) {
    newString->Render();
    result = 1;
  }
  if (newString) {
    newString->Recycle();
  }
  return result;
}
void GxuFontRender(CGxString *string) {
  if (string) {
    string->Render();
  }
}
void GxuFontRender(CGxString *string, const NTempest::C44Matrix &xform) {
  if (string) {
    string->Render(xform);
  }
}
float GxuFontGetStringHeight(CGxString *string) {
  return string ? string->GetStringHeight() : 0.0f;
}
CGxStringBatch *GxuFontCreateBatch() {
  CGxStringBatch *batch = s_unusedBatches.Head();

  if (batch) {
    batch->Unlink();
  } else {
    batch = NEWZERO(CGxStringBatch);
  }

  return batch;
}
int GxuFontAddToBatch(CGxStringBatch *batch, CGxString *string) {
  if (!batch || !string) {
    return 0;
  }
  batch->AddString(string);
  return 1;
}
int GxuFontRemoveFromBatch(CGxString *string) {
  if (!string) {
    return 0;
  }
  string->m_batchedStringLink.Unlink();
  return 1;
}
int GxuFontRenderBatch(CGxStringBatch *batch) {
  if (!batch) {
    return 0;
  }
  batch->RenderBatch();
  return 1;
}
int GxuFontClearBatch(CGxStringBatch *batch) {
  if (!batch) {
    return 0;
  }
  batch->Clear();
  return 1;
}
int GxuFontDestroyBatch(CGxStringBatch *batch) {
  if (!batch) {
    return 0;
  }
  batch->Clear();
  s_unusedBatches.LinkNode(batch, LIST_TAIL, 0);
  return 1;
}
int GxuFontAddToInternalBatch(CGxString *string) {
  if (!string) {
    return 0;
  }
  s_stringBatch.AddString(string);
  return 1;
}
void GxuFontRenderInternalBatch() {
  s_stringBatch.RenderBatch();
  s_stringBatch.Clear();
}
void
GxuFontGetTextExtent(CGxFont *face, const char *text, unsigned int numBytes, float height, float *extent, float charSpacing, unsigned int flags) {
  InternalGetTextExtent(face, text, numBytes, height, extent, flags);
}
void GxuFontGetWrapPoint(
    CGxFont      *face,
    const char   *text,
    float         fontHeight,
    float         blockWidth,
    unsigned int *numBytes,
    float        *pExtent,
    const char  **pNextText,
    float         spacing,
    unsigned int  flags
) {
  FATALASSERT(face);

  FATALASSERT(text);

  CalcWrapPoint(face, text, fontHeight, blockWidth, numBytes, pExtent, pNextText, flags);
}
float
GxuFontGetWrappedTextHeight(CGxFont *face, const char *text, float fontHeight, float blockWidth, float lineSpacing, unsigned int flags) {
  unsigned int advance;
  float        extent;
  unsigned int wide;
  unsigned int lines = 0;
  const char  *nextText = 0;
  const char  *currentText;

  FATALASSERT(face);

  FATALASSERT(text);

  if (flags & 0x4) {
    fontHeight = GxuFontGetOneToOneHeight(face);
  }

  currentText = text;
  while (*currentText) {
    QUOTEDCODE quoted = GxuDetermineQuotedCode(currentText, advance, 0, flags, wide, SStrLen(currentText));

    if (wide == '\n' || quoted == CODE_NEWLINE) {
      currentText += advance;
      nextText = currentText;
    } else {
      CalcWrapPoint(face, currentText, fontHeight, blockWidth, 0, &extent, &nextText, flags);
      currentText = nextText;
    }

    ++lines;
    if (flags & 0x2) {
      break;
    }

    if (!currentText) {
      break;
    }
  }

  return static_cast<float>(lines - 1) * lineSpacing + static_cast<float>(lines) * fontHeight;
}
unsigned int GxuFontGetMaxCharsWithinWidth(
    CGxFont     *face,
    const char  *text,
    float        height,
    float        maxWidth,
    unsigned int lineBytes,
    float       *extent,
    float        charSpacing,
    unsigned int flags
) {
  return InternalGetMaxCharsWithinWidth(face, text, height, maxWidth, lineBytes, extent, flags, 0, 0, 0);
}
unsigned int GxuFontGetMaxCharsWithinWidthFromEnd(
    CGxFont     *font,
    const char  *text,
    float        fontHeight,
    float        width,
    unsigned int lineBytes,
    float       *extent,
    float        charSpacing,
    unsigned int flags
) {
  unsigned int bytesInString;
  float        textExtent;
  float        remaining;
  float       *widthArray;
  float       *currentWidth;
  unsigned int charsToRemove;

  ASSERT(font);

  if (!text || !*text || !lineBytes || width == 0.0f) {
    return 0;
  }

  widthArray = static_cast<float *>(_alloca(sizeof(float) * lineBytes));
  bytesInString =
      InternalGetMaxCharsWithinWidth(font, text, fontHeight, 10000.0f, lineBytes, &textExtent, flags, 0, widthArray, widthArray + lineBytes);
  if (textExtent <= width) {
    if (extent) {
      *extent = textExtent;
    }
    return bytesInString;
  }

  remaining = textExtent - width;
  currentWidth = widthArray;
  charsToRemove = 0;
  while (charsToRemove < bytesInString && *currentWidth <= remaining) {
    ++charsToRemove;
    ++currentWidth;
  }

  if (charsToRemove >= bytesInString) {
    if (extent) {
      *extent = 0.0f;
    }
    return 0;
  }

  if (extent) {
    *extent = textExtent - *currentWidth;
  }
  return bytesInString - charsToRemove;
}
unsigned int GxuFontWrapText(
    CGxFont      *font,
    const char   *text,
    unsigned int  lineBytes,
    float         fontHeight,
    float         blockWidth,
    unsigned int *outputList,
    unsigned int  outputListElements,
    float         charSpacing,
    unsigned int  flags
) {
  unsigned int unusedNumBytes;
  float        unusedExtents;
  unsigned int advance;
  const char  *nextText;
  unsigned int wide;
  const char  *originalText;
  const char  *currentText;
  const char  *textEnd;
  unsigned int lines = 0;

  ASSERT(font);
  ASSERT(outputListElements);

  if (!text || !*text || !lineBytes || fontHeight == 0.0f || blockWidth == 0.0f) {
    return 0;
  }

  if (flags & 0x4) {
    fontHeight = GxuFontGetOneToOneHeight(font);
  }

  originalText = text;
  currentText = text;
  textEnd = text + lineBytes;

  while (currentText < textEnd) {
    QUOTEDCODE quoted;

    if (lines < outputListElements) {
      outputList[lines] = static_cast<unsigned int>(currentText - originalText);
    }
    ++lines;

    quoted = GxuDetermineQuotedCode(currentText, advance, 0, flags, wide, SStrLen(currentText));
    if (wide == '\n' || quoted == CODE_NEWLINE) {
      currentText += advance;
    } else {
      CalcWrapPoint(font, currentText, fontHeight, blockWidth, &unusedNumBytes, &unusedExtents, &nextText, flags);
      currentText = nextText;
    }

    if (!currentText) {
      break;
    }
  }

  if (textEnd[-1] == '\n') {
    if (lines < outputListElements) {
      outputList[lines] = lineBytes;
    }
    ++lines;
  }

  return lines;
}
float GxuFontGetOneToOneHeight(CGxFont *font) {
  FATALASSERT(font);

  ASSERT(font->m_cellHeight);
  ASSERT(font->m_cellHeight <= 32);

  return static_cast<float>(font->m_pixelSize) / static_cast<float>(g_heightPixels);
}
const char *
GxuFontStripEscapeCodes(const char *inputString, unsigned int numBytes, unsigned int flags, char *buffer, unsigned int bufferSize) {
  static struct {
    unsigned int stripFlags;
    char         charCode;
    int          addEscapeChar;
  } s_stripFlags[NUM_QUOTEDCODES] = {
      {0x000,  'C', 1},
      {0x100,  'R', 1},
      {0x200, '\n', 0},
      {0x800,  '|', 0},
      {0x400,  'H', 1},
      {0x400,  'h', 1},
      {0x000,  '-', 1}
  };
  unsigned int wide;
  const char  *originalString;
  unsigned int advance;
  unsigned int remainingBytes;
  unsigned int outputBytes;

  FATALASSERT(buffer);

  FATALASSERT(bufferSize);

  buffer[0] = 0;
  if (!inputString || !*inputString || !numBytes) {
    SStrPrintf(buffer, bufferSize, "%s", inputString);
    return inputString;
  }

  remainingBytes = numBytes;
  outputBytes = 0;
  while (*inputString && remainingBytes) {
    QUOTEDCODE quoted;

    originalString = inputString;
    quoted = GxuDetermineQuotedCode(inputString, advance, 0, 0, wide, remainingBytes);
    switch (quoted) {
      case CODE_COLORON:
      case CODE_COLORRESTORE:
      case CODE_HYPERLINKSTART:
        if (flags & s_stripFlags[quoted].stripFlags) {
          inputString += advance;
          remainingBytes -= advance;
          break;
        }
        if (outputBytes >= bufferSize - advance) {
          goto done;
        }
        while (advance) {
          buffer[outputBytes++] = *inputString++;
          --remainingBytes;
          --advance;
        }
        break;

      case CODE_NEWLINE:
      case CODE_PIPE:
      case CODE_HYPERLINKSTOP:
        inputString += advance;
        remainingBytes -= advance;
        if (flags & s_stripFlags[quoted].stripFlags) {
          break;
        }
        if (outputBytes >= bufferSize - advance) {
          goto copyOriginal;
        }
        if (s_stripFlags[quoted].addEscapeChar) {
          buffer[outputBytes++] = '|';
        }
        buffer[outputBytes++] = static_cast<char>(s_stripFlags[quoted].charCode);
        break;

      default:
      copyOriginal:
        if (outputBytes >= bufferSize - advance) {
          goto done;
        }
        for (wide = 0; wide < advance; ++wide) {
          buffer[outputBytes++] = originalString[wide];
        }
        inputString += advance;
        remainingBytes -= advance;
        break;
    }
  }

done:
  buffer[outputBytes] = 0;
  return buffer;
}
int GxuFontGetLastColorCode(const char *string, unsigned int numBytes, NTempest::CImVector *color) {
  unsigned int        wide;
  unsigned int        advance;
  NTempest::CImVector colorCode;
  NTempest::CImVector foundColor;
  int                 found;

  found = 0;
  while (*string && numBytes) {
    QUOTEDCODE quoted = GxuDetermineQuotedCode(string, advance, &colorCode, 0, wide, numBytes);
    if (quoted == CODE_COLORON) {
      foundColor = colorCode;
      found = 1;
    } else if (quoted == CODE_COLORRESTORE) {
      found = 0;
    }

    string += advance;
    numBytes -= advance;
  }

  if (found) {
    *color = foundColor;
  }
  return found;
}
int GxuFontGenerateColorString(char *buf, unsigned int bufSize, const NTempest::CImVector &color) {
  if (!buf || bufSize < 11) {
    return 0;
  }

  SStrPrintf(buf, bufSize, "|C%2.2x%2.2x%2.2x%2.2x", color.a, color.r, color.g, color.b);
  return 1;
}
int GxuFontSetStringColor(CGxString *string, NTempest::CImVector newColor) {
  FATALASSERT(string);
  string->SetColor(newColor);
  return 1;
}
void GxuFontSetStringPosition(CGxString *string, const NTempest::C3Vector &pos) {
  FATALASSERT(string);
  string->SetStringPosition(pos);
}
void GxuFontSetCharSpacing(CGxString *string, float spacing) {
}
void GxuFontAddShadow(CGxString *string, const NTempest::CImVector &color, const NTempest::C2Vector &offset) {
  if (string && !string->IsBillboarded()) {
    string->AddShadow(offset, color);
  }
}
void GxuFontRemoveShadow(CGxString *string) {
  if (string) {
    string->RemoveShadow();
  }
}
CGxString *GxuFontDuplicateString(const CGxString *rhs) {
  return rhs ? rhs->Duplicate() : 0;
}
int GxuFontGetStringWidth(CGxString *string, float *width) {
  FATALASSERT(width);

  if (string) {
    *width = string->GetSavedWidth();
  }
  return string != 0;
}
int GxuFontGetStringHeight(CGxString *string, float *height) {
  FATALASSERT(height);

  if (string) {
    *height = string->GetStringHeight();
  }
  return string != 0;
}
unsigned int GxuFontStringHyperLinkInfo(const CGxString *string, const GXUFONTHYPERLINKINFO *&list) {
  return string ? string->GetHyperLinkInfo(list) : 0;
}
int GxuFontStringSetGradient(CGxString *string, int startCharacter, int length) {
  if (!string) {
    return -1;
  }

  return string->SetGradient(startCharacter, length);
}
