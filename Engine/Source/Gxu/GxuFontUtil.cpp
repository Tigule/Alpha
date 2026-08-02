#include "IGxuFont.h"

#include <Base/ConvertUTF.h>
#include <Gx/CGxDevice.h>

#include <freetype/freetype.h>

#include <math.h>
#include <stdlib.h>
#include <wctype.h>

unsigned int                                         g_heightPixels;
unsigned int                                         g_widthPixels;
LISTDECL(TEXTLINETEXTURE, g_freeTextLineTextures);
LISTDECL(IGXUTEXTLINE, g_freeTextLines);
LISTDECL(CGxString, g_freeStrings);
LISTDECL(CGxString, g_strings);

static float                                        leftPixelAdjustment;
static float                                        rightPixelAdjustment;
static float                                        bottomPixelAdjustment;
static float                                        topPixelAdjustment;
static int                                          adjustmentsInitialized;
static int                                          pixelCenterOnEdge;
static int                                          initialized;
static const unsigned char                          pixelsLitLevels[10] = {0x00, 0x1F, 0x1F, 0x3F, 0x5F, 0x7F, 0x9F, 0xBF, 0xDF, 0xFF};
static TSHashTable<STRINGVIEWMATRICES, HASHKEY_PTR> s_stringViewMatrices;
static LISTDECLEX(STRINGVIEWMATRICES, m_freeLink, s_freeStringMatrices);
static HASHKEY_NONE                                 s_nullHashKey;
static const float                                  ONEOVERTEXSIZE = 1.0f / 256.0f;
static const float                                  ONEHALFONEOVERTEXSIZE = ONEOVERTEXSIZE * 0.5f;

QUOTEDCODE GxuDetermineQuotedCode(
    const char          *text,
    unsigned int        &advance,
    NTempest::CImVector *color,
    unsigned int         flags,
    unsigned int        &wide,
    unsigned int         remainingBytes
) {
  int          ignoreNewlines = flags & 0x200;
  int          ignoreColors = flags & 0x100;
  int          ignoreHyperlinks = flags & 0x400;
  int          ignorePipes = flags & 0x800;
  const char  *firstText;
  const char  *codePtr;
  unsigned int firstCharAdvance;
  unsigned int comps[4];
  char         hex[3];
  char        *error;
  QUOTEDCODE   result;
  unsigned int i;

  ASSERT(text);
  ASSERT(*text);

  firstText = text;
  wide = sgetu8(reinterpret_cast<const unsigned char *>(text), reinterpret_cast<int *>(&advance));
  firstCharAdvance = advance;

  if (wide == static_cast<unsigned int>(-1)) {
    return CODE_INVALIDCODE;
  }

  if (wide == '\n') {
    advance = 1;
    return CODE_NEWLINE;
  }

  if (wide != '|' || ignorePipes) {
    return CODE_INVALIDCODE;
  }

  codePtr = text + advance;
  wide = sgetu8(reinterpret_cast<const unsigned char *>(codePtr), reinterpret_cast<int *>(&advance));

  switch (*codePtr) {
    case 'H':
    case 'h':
      if (*codePtr == 'h') {
        if (ignoreHyperlinks) {
          goto invalidQuotedCode;
        }

        advance = 2;
        return CODE_HYPERLINKSTOP;
      }

      if (ignoreHyperlinks) {
        goto invalidQuotedCode;
      }

      codePtr += advance;
      while (*codePtr) {
        wide = sgetu8(reinterpret_cast<const unsigned char *>(codePtr), reinterpret_cast<int *>(&advance));
        codePtr += advance;

        if (wide != '|') {
          continue;
        }

        wide = sgetu8(reinterpret_cast<const unsigned char *>(codePtr), reinterpret_cast<int *>(&advance));
        codePtr += advance;

        if (wide != 'h') {
          goto invalidQuotedCode;
        }

        advance = static_cast<unsigned int>(codePtr - firstText);
        if (advance == 4 || (codePtr[0] == '|' && codePtr[1] == 'h')) {
          goto invalidQuotedCode;
        }

        return CODE_HYPERLINKSTART;
      }
      goto invalidQuotedCode;

    case 'N':
    case 'n':
      if (ignoreNewlines) {
        goto invalidQuotedCode;
      }

      advance = 2;
      if (remainingBytes < 2) {
        advance = firstCharAdvance;
        wide = '|';
      }
      return CODE_NEWLINE;

    case 'R':
    case 'r':
      if (ignoreColors) {
        goto invalidQuotedCode;
      }

      advance = 2;
      if (remainingBytes < 2) {
        advance = firstCharAdvance;
        wide = '|';
      }
      return CODE_COLORRESTORE;

    case '|':
      result = CODE_PIPE;
      if (remainingBytes < advance) {
        advance = firstCharAdvance;
        wide = '|';
        result = CODE_INVALIDCODE;
      }
      advance = 2;
      return result;

    case 'C':
    case 'c':
      if (ignoreColors) {
        goto invalidQuotedCode;
      }

      result = CODE_COLORON;
      advance = 10;
      if (remainingBytes < 10) {
        goto invalidQuotedCode;
      }

      codePtr += 1;
      hex[2] = 0;
      for (i = 0; i < 4; ++i) {
        hex[0] = *codePtr++;
        hex[1] = *codePtr++;
        error = 0;

        unsigned long component = strtol(hex, &error, 16);
        if (error && *error) {
          advance = firstCharAdvance;
          wide = '|';
          result = CODE_INVALIDCODE;
          break;
        }

        comps[i] = static_cast<unsigned char>(component);
      }

      if (color) {
        color->Set(
            0xFF000000ul | (static_cast<unsigned long>(comps[1]) << 16) | (static_cast<unsigned long>(comps[2]) << 8) |
            static_cast<unsigned long>(comps[3])
        );
      }
      return result;
  }

invalidQuotedCode:
  advance = firstCharAdvance;
  wide = '|';
  return CODE_INVALIDCODE;
}

static int CanWrapBetween(unsigned int lastChar, unsigned int wideChar, unsigned int flags) {
  int value;

  if (!lastChar) {
    return 0;
  }
  if (lastChar == '-') {
    return 1;
  }
  if (wideChar == static_cast<unsigned int>(-1)) {
    return 1;
  }
  if (iswspace(static_cast<wint_t>(lastChar))) {
    return 0;
  }
  if (iswspace(static_cast<wint_t>(wideChar))) {
    return 1;
  }

  if (lastChar > 0x3010) {
    if (lastChar > 0xFF04) {
      switch (lastChar) {
        case 0xFF08:
        case 0xFF3B:
        case 0xFF5B:
        case 0xFFE1:
        case 0xFFE5:
        case 0xFFE6:
          return 0;
      }
      goto checkCurrentCode;
    }

    if (lastChar == 0xFF04) {
      return 0;
    }
    if (lastChar > 0xFE59) {
      value = lastChar - 0xFE5B;
      if (!value || value == 2) {
        return 0;
      }
    } else {
      if (lastChar == 0xFE59) {
        return 0;
      }
      value = lastChar - 0x3014;
      if (!value || value == 9) {
        return 0;
      }
    }
  } else {
    if (lastChar == 0x3010) {
      return 0;
    }
    if (lastChar > 0x201C) {
      if (lastChar > 0x300A) {
        value = lastChar - 0x300C;
        if (!value || value == 2) {
          return 0;
        }
      } else {
        if (lastChar == 0x300A || lastChar == 0x2035 || lastChar == 0x3008) {
          return 0;
        }
      }
    } else {
      if (lastChar == 0x201C) {
        return 0;
      }
      if (lastChar > '\\') {
        if (lastChar == '{' || lastChar == 0x2018) {
          return 0;
        }
      } else if (lastChar >= '[' || lastChar == '$' || lastChar == '(') {
        return 0;
      }
    }
  }

checkCurrentCode:
  if (wideChar <= 0x300F) {
    if (wideChar != 0x300F) {
      if (wideChar > 0x2014) {
        if (wideChar > 0x2103) {
          switch (wideChar) {
            case 0x3001:
            case 0x3002:
            case 0x3009:
            case 0x300B:
            case 0x300D:
              return 0;
          }
          goto checkWideCharacterRange;
        }
        if (wideChar != 0x2103) {
          switch (wideChar) {
            case 0x2019:
            case 0x201D:
            case 0x2022:
            case 0x2026:
            case 0x2027:
            case 0x2032:
            case 0x2033:
              return 0;
          }
          goto checkWideCharacterRange;
        }
      } else if (wideChar < 0x2013) {
        switch (wideChar) {
          case '!':
          case '%':
          case ')':
          case ',':
          case '.':
          case ':':
          case ';':
          case '?':
          case ']':
          case '}':
          case 0xB0:
          case 0xB7:
            return 0;
        }
        goto checkWideCharacterRange;
      }
    }
    return 0;
  }

  if (wideChar > 0xFF05) {
    switch (wideChar) {
      case 0xFF09:
      case 0xFF0C:
      case 0xFF0E:
      case 0xFF1A:
      case 0xFF1B:
      case 0xFF1F:
      case 0xFF3D:
      case 0xFF5D:
      case 0xFF70:
      case 0xFF9E:
      case 0xFF9F:
      case 0xFFE0:
        return 0;
    }
    goto checkWideCharacterRange;
  }
  if (wideChar == 0xFF05) {
    return 0;
  }
  if (wideChar > 0xFE52) {
    switch (wideChar) {
      case 0xFE54:
      case 0xFE55:
      case 0xFE56:
      case 0xFE57:
      case 0xFE5A:
      case 0xFE5C:
      case 0xFE5E:
      case 0xFF01:
        return 0;
    }
    goto checkWideCharacterRange;
  }
  if (wideChar >= 0xFE50) {
    return 0;
  }
  if (wideChar > 0x301E) {
    if (wideChar == 0x30FC || wideChar == 0xFE30) {
      return 0;
    }
  } else if (wideChar == 0x301E || wideChar == 0x3011 || wideChar == 0x3015) {
    return 0;
  }

checkWideCharacterRange:
  if ((wideChar < 0x1100 || wideChar > 0x11FF) && (wideChar < 0x3000 || wideChar > 0xD7AF) && (wideChar < 0xF900 || wideChar > 0xFAFF) &&
      (wideChar < 0xFF00 || wideChar > 0xFF9F) && (wideChar < 0xFFA0 || wideChar > 0xFFDC))
  {
    return flags & 0x40;
  }

  return 1;
}

static unsigned int FindWrappingIndex(const char *currentText, unsigned int lineBytes, unsigned int flags, const char **nextText) {
  const char  *startingText;
  const char  *scan;
  unsigned int wrapIndex;
  unsigned int lastChar;
  unsigned int wideChar;
  unsigned int advance;

  ASSERT(nextText);

  if (!currentText || !*currentText || !lineBytes) {
    *nextText = currentText;
    return 0;
  }

  startingText = currentText;
  wrapIndex = 0;
  lastChar = 0;
  wideChar = 0;

  while (*currentText && lineBytes) {
    QUOTEDCODE quotedCode = GxuDetermineQuotedCode(currentText, advance, 0, flags, wideChar, lineBytes);

    if (quotedCode == CODE_NEWLINE) {
      *nextText = currentText + advance;
      scan = startingText + wrapIndex;

      while (scan < *nextText) {
        unsigned int code = sgetu8(reinterpret_cast<const unsigned char *>(scan), reinterpret_cast<int *>(&advance));
        scan += advance;
        if (!iswspace(static_cast<wint_t>(code))) {
          wrapIndex = static_cast<unsigned int>(scan - startingText);
        }
      }
      return wrapIndex;
    }

    if (quotedCode == CODE_INVALIDCODE) {
      if (CanWrapBetween(lastChar, wideChar, flags)) {
        wrapIndex = static_cast<unsigned int>(currentText - startingText);
      }
      lastChar = wideChar;
    }

    currentText += advance;
    lineBytes -= advance;
  }

  if (GxuDetermineQuotedCode(currentText, advance, 0, flags, wideChar, SStrLen(currentText)) == CODE_INVALIDCODE &&
      CanWrapBetween(lastChar, wideChar, flags))
  {
    wrapIndex = static_cast<unsigned int>(currentText - startingText);
  }

  if (!wrapIndex) {
    wrapIndex = static_cast<unsigned int>(currentText - startingText);
  }

  scan = startingText + wrapIndex;
  while (*scan) {
    unsigned int code = sgetu8(reinterpret_cast<const unsigned char *>(scan), reinterpret_cast<int *>(&advance));
    if (!iswspace(static_cast<wint_t>(code))) {
      break;
    }
    scan += advance;
  }

  *nextText = scan;
  return wrapIndex;
}

static STRINGVIEWMATRICES *GetNewStringMatrix(CGxString *stringPtr) {
  STRINGVIEWMATRICES *head = s_stringViewMatrices.Ptr(reinterpret_cast<unsigned int>(stringPtr), HASHKEY_PTR(stringPtr));

  if (head) {
    ASSERT(!head->m_freeLink.IsLinked());
    return head;
  }

  head = s_freeStringMatrices.Head();
  if (head) {
    s_freeStringMatrices.UnlinkNode(head);
  } else {
    head = s_freeStringMatrices.NewNode(LIST_TAIL, 0, 0);
  }

  s_stringViewMatrices.Insert(head, reinterpret_cast<unsigned int>(stringPtr), HASHKEY_PTR(stringPtr));
  return head;
}

void CGxString::InitializeTextLine(
    const char               *currentText,
    unsigned int              numBytes,
    NTempest::CImVector      &workingColor,
    const NTempest::C3Vector &position,
    unsigned int             *texturePagesUsedFlag,
    HYPERLINKPARSEINFO       &info
) {
  unsigned int       i;
  unsigned int       advance;
  unsigned int       wide;
  unsigned int       charsInTexturePage[8] = {0};
  int                fixedCharWidths;
  IGXUTEXTLINE      *newLine;
  NTempest::C3Vector currPos(position);
  float              glyphPixelHeight;
  const char        *scan;
  unsigned int       scanBytes;

  ASSERT(m_currentFace);
  ASSERT(currentText);
  ASSERT(texturePagesUsedFlag);

  fixedCharWidths = m_flags & 0x10;
  newLine = m_textBlock.NewLine();
  newLine->Reserve(m_currentFace->GetNumCurrentTextures());

  if (m_flags & 0x8) {
    for (i = 0; i < newLine->m_texturePages.Count(); ++i) {
      newLine->m_texturePages[i]->m_colors.SetCount(0);
    }
  }

  const float screenPixelHeight = ScreenToPixelHeight(m_flags & 0x80, m_currentFontHeight);
  const float glyphToScreenPixels = screenPixelHeight / static_cast<float>(m_currentFace->m_pixelSize);
  glyphPixelHeight = ScreenToPixelHeight(m_flags & 0x80, m_currentFontHeight);

  if (m_currentFace->m_flags & 0x1) {
    glyphPixelHeight += 2.0f;
  }
  if (m_currentFace->m_flags & 0x8) {
    glyphPixelHeight += 2.0f;
  }

  scan = currentText;
  scanBytes = numBytes;
  while (*scan && scanBytes) {
    QUOTEDCODE quotedCode = GxuDetermineQuotedCode(scan, advance, 0, m_flags, wide, scanBytes);
    scan += advance;
    scanBytes -= advance;

    switch (quotedCode) {
      case CODE_COLORON:
      case CODE_COLORRESTORE:
      case CODE_NEWLINE:
      case CODE_HYPERLINKSTART:
      case CODE_HYPERLINKSTOP:
        break;

      default: {
        const CHARCODEDESC *code = m_currentFace->NewCodeDesc(wide);
        if (code && code->bitmapData) {
          ++charsInTexturePage[code->textureNumber];
        }
        break;
      }
    }
  }

  for (i = 0; i < newLine->m_texturePages.Count(); ++i) {
    unsigned int vertexCount = charsInTexturePage[i] * 4;
    newLine->m_texturePages[i]->m_vert.ReserveSpace(vertexCount);

    if (!(m_flags & 0x8)) {
      newLine->m_texturePages[i]->m_colors.ReserveSpace(vertexCount);
    }
  }

  if (!adjustmentsInitialized) {
    CGxCaps gxCaps = GxCaps();
    adjustmentsInitialized = 1;

    if (gxCaps.m_pixelCenterOnEdge) {
      leftPixelAdjustment = 0.0f;
      rightPixelAdjustment = 0.0f;
      topPixelAdjustment = 0.0f;
      bottomPixelAdjustment = 0.0f;
    } else {
      leftPixelAdjustment = -0.5f;
      rightPixelAdjustment = -0.5f;
      topPixelAdjustment = -0.5f;
      bottomPixelAdjustment = -0.5f;
    }
  }

  if (info.hyperlinkParseMode == HYPERLINKDISPLAY) {
    info.currentParseInfo.extent.r = currPos.x;
  }

  if (m_flags & 0x1) {
    NTempest::C3Vector offset3(m_shadowOffset.x, m_shadowOffset.y, 0.0f);
    offset3.x = static_cast<float>(floor(ScreenToPixelWidth(0, offset3.x)));
    offset3.y = static_cast<float>(floor(ScreenToPixelHeight(0, offset3.y)));
  }

  unsigned int prevCode = 0;
  float        step = 0.0f;

  while (*currentText && numBytes) {
    NTempest::CImVector color(0ul);
    QUOTEDCODE          quotedCode = GxuDetermineQuotedCode(currentText, advance, &color, m_flags, wide, numBytes);
    currentText += advance;
    numBytes -= advance;

    if (prevCode) {
      if (fixedCharWidths) {
        step = m_currentFace->ComputeStepFixedWidth(prevCode, wide);
      } else {
        step = m_currentFace->ComputeStep(prevCode, wide);
      }
    }
    step *= glyphToScreenPixels;

    switch (quotedCode) {
      case CODE_COLORON:
        if (!(m_flags & 0x8)) {
          color.a = m_fontColor.a;
          workingColor = color;
        }
        break;

      case CODE_COLORRESTORE:
        workingColor = m_fontColor;
        break;

      case CODE_NEWLINE:
        break;

      case CODE_HYPERLINKSTART:
        if (info.hyperlinkParseMode != HYPERLINKDISPLAY) {
          const char  *link = currentText - advance + 2;
          unsigned int linkLength = advance - 4;

          info.currentParseInfo.extent.l = currPos.x + step;
          info.hyperlinkParseMode = HYPERLINKDISPLAY;
          info.lastLinkStartPtr = link;
          info.lastLinkLength = linkLength;
          info.currentParseInfo.link = link;
          info.currentParseInfo.linkLength = linkLength;
        }
        break;

      case CODE_HYPERLINKSTOP:
        if (info.hyperlinkParseMode == HYPERLINKDISPLAY) {
          info.currentParseInfo.extent.r = currPos.x + step;
          info.hyperlinkParseMode = HYPERLINKNONE;
          AddHyperlinkParseInfo(info.currentParseInfo);
        }
        break;

      default: {
        const CHARCODEDESC *code = m_currentFace->NewCodeDesc(wide);

        if (!code || !code->bitmapData) {
          break;
        }

        ASSERT(code->dataValid);

        unsigned int textureNumber = code->textureNumber;
        ASSERT(textureNumber < (sizeof(m_currentFace->m_textureCache) / sizeof(m_currentFace->m_textureCache[0])));
        ASSERT(m_currentFace->m_textureCache[textureNumber].GetTexturePtr());

        *texturePagesUsedFlag |= 1 << textureNumber;
        TEXTLINETEXTURE *textLinePage = newLine->m_texturePages[textureNumber];

        if (!(m_flags & 0x8)) {
          NTempest::CImVector sColors[4];
          for (i = 0; i < 4; ++i) {
            sColors[i] = workingColor;
          }

          unsigned int oldIndex = textLinePage->m_colors.Add(4, sColors);

          if (m_flags & 0x1000) {
            NTempest::CImVector *ptrArray[4];
            ptrArray[0] = &textLinePage->m_colors[oldIndex];
            ptrArray[1] = &textLinePage->m_colors[oldIndex + 3];
            ptrArray[2] = &textLinePage->m_colors[oldIndex + 1];
            ptrArray[3] = &textLinePage->m_colors[oldIndex + 2];
            m_colorGradients.Add(4, ptrArray);

            if (m_flags & 0x1) {
              for (i = 0; i < 4; ++i) {
                sColors[i] = m_shadowColor;
              }

              unsigned int oldShadowIndex = textLinePage->m_shadowColors.Add(4, sColors);
              ptrArray[0] = &textLinePage->m_shadowColors[oldShadowIndex];
              ptrArray[1] = &textLinePage->m_shadowColors[oldShadowIndex + 3];
              ptrArray[2] = &textLinePage->m_shadowColors[oldShadowIndex + 1];
              ptrArray[3] = &textLinePage->m_shadowColors[oldShadowIndex + 2];
              ASSERT(oldShadowIndex == oldIndex);
              m_colorGradientShadows.Add(4, ptrArray);
            }
          }
        }

        if (static_cast<signed char>(m_flags) >= 0) {
          step = static_cast<float>(floor(SignOf(step) * 0.5f + step));
        }

        currPos.x += step;

        VERT vert;
        vert.vc.x = currPos.x;
        vert.vc.y = currPos.y;
        vert.vc.z = currPos.z;
        vert.tc.x = 0.0f;
        vert.tc.y = 0.0f;

        if (static_cast<signed char>(m_flags) >= 0) {
          vert.vc.x += static_cast<float>(ceil(code->bitmapData->m_glyphBearing));
        } else {
          vert.vc.x += code->bitmapData->m_glyphBearing * glyphToScreenPixels;
        }

        float charWidth = static_cast<float>(code->bitmapData->m_glyphCellWidth) * glyphToScreenPixels;
        if (static_cast<signed char>(m_flags) >= 0) {
          charWidth = static_cast<float>(floor(charWidth));
        }

        if (m_currentFace->m_flags & 0x1) {
          vert.vc.y -= 1.0f;
        }
        if (m_currentFace->m_flags & 0x8) {
          vert.vc.y -= 1.0f;
        }

        TSGrowableArray<VERT> &verts = textLinePage->m_vert;
        unsigned int           oldIndex = verts.Count();
        verts.SetCount(oldIndex + 4);
        for (i = 0; i < 4; ++i) {
          verts[oldIndex + i] = vert;
        }

        VERT *quad = &verts[oldIndex];
        if (static_cast<signed char>(m_flags) < 0) {
          quad[0].vc.x = quad[3].vc.x;
          quad[0].vc.y = quad[1].vc.y;
          quad[1].vc.x += charWidth;
          quad[2].vc.x += charWidth;
          quad[2].vc.y += screenPixelHeight;
          quad[3].vc.y += screenPixelHeight;
        } else {
          quad[0].vc.x = quad[3].vc.x += leftPixelAdjustment;
          quad[1].vc.x = quad[2].vc.x += charWidth + rightPixelAdjustment;
          quad[2].vc.y = quad[3].vc.y += glyphPixelHeight + topPixelAdjustment;
          quad[0].vc.y = quad[1].vc.y += bottomPixelAdjustment;
        }

        if (code->bitmapData->m_yOffset) {
          const int yOffset = code->bitmapData->m_yOffset;
          for (i = 0; i < 4; ++i) {
            quad[i].vc.y += yOffset * glyphToScreenPixels;
          }
        }

        quad[0].tc.y = code->bitmapData->m_textureCoords.b;
        quad[1].tc.y = code->bitmapData->m_textureCoords.b;
        quad[2].tc.y = code->bitmapData->m_textureCoords.t;
        quad[3].tc.y = code->bitmapData->m_textureCoords.t;
        quad[0].tc.x = code->bitmapData->m_textureCoords.l;
        quad[3].tc.x = code->bitmapData->m_textureCoords.l;
        quad[1].tc.x = code->bitmapData->m_textureCoords.r;
        quad[2].tc.x = code->bitmapData->m_textureCoords.r;

        prevCode = wide;
        break;
      }
    }
  }

  info.currentParseInfo.extent.r = currPos.x + step;
}

CGxString *CGxString::Duplicate() const {
  CGxString *newString;

  GxuFontCreateString(
      m_currentFace, m_text, m_requestedFontHeight, m_position, m_blockWidth, m_blockHeight, m_spacing, newString, m_vertJust, m_horzJust, m_flags,
      m_fontColor, 0.0f
  );
  return newString;
}

void CalcWrapPoint(
    CGxFont      *face,
    const char   *currentText,
    float         fontHeight,
    float         blockWidth,
    unsigned int *numBytes,
    float        *pExtent,
    const char  **pNextText,
    unsigned int  flags
) {
  unsigned int lineBytes = currentText ? SStrLen(currentText) : 0;
  unsigned int bytesInString;
  const char  *nextText;
  float        extent;

  InternalGetMaxCharsWithinWidth(face, currentText, fontHeight, blockWidth, lineBytes, &extent, flags, &bytesInString, 0, 0);

  if (bytesInString == lineBytes) {
    nextText = 0;
  } else {
    bytesInString = FindWrappingIndex(currentText, bytesInString, flags, &nextText);
    InternalGetTextExtent(face, currentText, bytesInString, fontHeight, &extent, flags);
  }

  if (numBytes) {
    *numBytes = bytesInString;
  }
  *pExtent = extent;
  *pNextText = nextText;
}

GLYPHBITMAPDATA::GLYPHBITMAPDATA() : m_code(0), m_data(0), m_dataSize(0), m_dirty(1), m_textureCoords(0.0f), m_textureValid(0) {
}

GLYPHBITMAPDATA::~GLYPHBITMAPDATA() {
  Clear();
}

void CHARCODEDESC::GenerateTextureCoords(unsigned int rowNumber, unsigned int glyphSide) {
  CGxCaps gxCaps;
  int     top;
  float   pr;

  ASSERT(bitmapData);
  ASSERT(glyphSide);

  if (initialized) {
    initialized = 1;
    gxCaps = GxCaps();
    pixelCenterOnEdge = gxCaps.m_pixelCenterOnEdge;
  }

  unsigned int width = bitmapData->m_glyphCellWidth;
  ASSERT(width);

  top = rowNumber * glyphSide;
  pr = static_cast<float>(glyphStartPixel);
  bitmapData->m_textureCoords.Set(
      top * ONEOVERTEXSIZE, pr * ONEOVERTEXSIZE + (pixelCenterOnEdge ? 0.0f : ONEHALFONEOVERTEXSIZE), (top + glyphSide) * ONEOVERTEXSIZE,
      (pr + width) * ONEOVERTEXSIZE - (pixelCenterOnEdge ? 0.0f : ONEHALFONEOVERTEXSIZE)
  );
  bitmapData->m_textureValid = 1;
}

unsigned int CHARCODEDESC::GapToNextTexture() const {
  ASSERT(ValidBlockEndPoints());

  const CHARCODEDESC *next = textureRowLink.Next();
  if (!next) {
    return 255 - glyphEndPixel;
  }

  ASSERT(next->ValidBlockEndPoints());
  return next->glyphStartPixel - glyphEndPixel - 1;
}

unsigned int CHARCODEDESC::GapToPreviousTexture() const {
  ASSERT(ValidBlockEndPoints());

  const CHARCODEDESC *previous = textureRowLink.Prev();
  if (!previous) {
    return glyphStartPixel;
  }

  ASSERT(previous->ValidBlockEndPoints());
  return glyphStartPixel - previous->glyphEndPixel - 1;
}

void TEXTURECACHE::PasteGlyphOutlinedMonochrome(GLYPHBITMAPDATA *glyphData, unsigned long *dst, int thick) {
  ASSERT(glyphData);
  ASSERT(dst);

  unsigned int scratch[32][256];
  memset(scratch, 0, sizeof(scratch));

  const unsigned char *src = static_cast<const unsigned char *>(glyphData->m_data);
  unsigned int         glyphX = 1 + (thick ? 1 : 0);
  unsigned int         glyphY = glyphData->m_yStart + 1;
  for (unsigned int y = 0; y < glyphData->m_glyphHeight; ++y) {
    for (unsigned int x = 0; x < glyphData->m_glyphWidth; ++x) {
      scratch[glyphY + y][glyphX + x] = (src[x >> 3] >> (7 - (x & 7))) & 1;
    }
    src += glyphData->m_glyphPitch;
  }

  unsigned int cellHeight = m_theFace->m_cellHeight;
  ASSERT(cellHeight);
  unsigned int cellWidth = glyphData->m_glyphCellWidth;
  unsigned int passes = 1 + (thick ? 1 : 0);

  for (unsigned int pass = 0; pass < passes; ++pass) {
    unsigned int source = pass ? 4 : 1;
    unsigned int replacement = pass ? 2 : 4;

    for (unsigned int y = 0; y < cellHeight; ++y) {
      for (unsigned int x = 0; x < cellWidth; ++x) {
        if ((!pass && scratch[y][x] == 1) || (pass && scratch[y][x] != 0)) {
          continue;
        }

        if (!y) {
          if (!x) {
            if ((scratch[y][x + 1] & source) || (scratch[y + 1][x + 1] & source) || (scratch[y + 1][x] & source)) {
              if (scratch[y][x] != 1) {
                scratch[y][x] = replacement;
              }
            }
          } else if (x == cellWidth - 1) {
            if ((scratch[y][x - 1] & source) || (scratch[y + 1][x - 1] & source) || (scratch[y + 1][x] & source)) {
              if (scratch[y][x] != 1) {
                scratch[y][x] = replacement;
              }
            }
          } else if (
              (scratch[y][x - 1] & source) || (scratch[y][x + 1] & source) || (scratch[y + 1][x - 1] & source) || (scratch[y + 1][x] & source) ||
              (scratch[y + 1][x + 1] & source)
          )
          {
            if (scratch[y][x] != 1) {
              scratch[y][x] = replacement;
            }
          }
        } else if (y == cellHeight - 1) {
          if (!x) {
            if ((scratch[y - 1][x] & source) || (scratch[y - 1][x + 1] & source) || (scratch[y][x + 1] & source)) {
              if (scratch[y][x] != 1) {
                scratch[y][x] = replacement;
              }
            }
          } else if (x == cellWidth - 1) {
            if ((scratch[y][x - 1] & source) || (scratch[y - 1][x - 1] & source) || (scratch[y - 1][x] & source)) {
              if (scratch[y][x] != 1) {
                scratch[y][x] = replacement;
              }
            }
          } else if (
              (scratch[y - 1][x - 1] & source) || (scratch[y - 1][x] & source) || (scratch[y - 1][x + 1] & source) || (scratch[y][x - 1] & source) ||
              (scratch[y][x + 1] & source)
          )
          {
            if (scratch[y][x] != 1) {
              scratch[y][x] = replacement;
            }
          }
        } else if (!x) {
          if ((scratch[y - 1][x] & source) || (scratch[y - 1][x + 1] & source) || (scratch[y][x + 1] & source) || (scratch[y + 1][x + 1] & source) ||
              (scratch[y + 1][x] & source))
          {
            if (scratch[y][x] != 1) {
              scratch[y][x] = replacement;
            }
          }
        } else if (x == cellWidth - 1) {
          if ((scratch[y - 1][x] & source) || (scratch[y - 1][x - 1] & source) || (scratch[y][x - 1] & source) || (scratch[y + 1][x - 1] & source) ||
              (scratch[y + 1][x] & source))
          {
            if (scratch[y][x] != 1) {
              scratch[y][x] = replacement;
            }
          }
        } else if (
            (scratch[y - 1][x - 1] & source) || (scratch[y - 1][x] & source) || (scratch[y - 1][x + 1] & source) || (scratch[y][x - 1] & source) ||
            (scratch[y][x + 1] & source) || (scratch[y + 1][x - 1] & source) || (scratch[y + 1][x] & source) || (scratch[y + 1][x + 1] & source)
        )
        {
          if (scratch[y][x] != 1) {
            scratch[y][x] = replacement;
          }
        }
      }
    }
  }

  for (unsigned int outputY = 0; outputY < cellHeight; ++outputY) {
    for (unsigned int x = 0; x < cellWidth; ++x) {
      switch (scratch[outputY][x]) {
        case 1:
          dst[x] = 0xFFFFFFFF;
          break;
        case 2:
          dst[x] = 0x7F000000;
          break;
        case 4:
          dst[x] = 0xFF000000;
          break;
        default:
          dst[x] = 0;
          break;
      }
    }
    dst += 256;
  }
}

void TEXTURECACHE::PasteGlyphNonOutlinedMonochrome(GLYPHBITMAPDATA *glyphData, unsigned long *dst) {
  ASSERT(glyphData);
  ASSERT(dst);

  char        *src = static_cast<char *>(glyphData->m_data);
  unsigned int pitch = glyphData->m_glyphPitch;
  unsigned int dstCellStride = 4 * glyphData->m_glyphCellWidth;

  unsigned int y;
  for (y = 0; y < static_cast<unsigned int>(glyphData->m_yStart); ++y) {
    memset(dst, 0, dstCellStride);
    dst += 256;
  }

  for (y = 0; y < glyphData->m_glyphHeight; ++y) {
    for (unsigned int x = 0; x < glyphData->m_glyphWidth; ++x) {
      unsigned int bit = (src[x >> 3] >> (7 - (x & 7))) & 1;
      dst[x] = bit ? 0xFFFFFFFF : 0;
    }
    dst += 256;
    src += pitch;
  }

  int remaining = static_cast<int>(m_theFace->m_cellHeight) - static_cast<int>(glyphData->m_glyphHeight) - glyphData->m_yStart;
  while (remaining > 0) {
    memset(dst, 0, dstCellStride);
    dst += 256;
    --remaining;
  }
}

void TEXTURECACHE::PasteGlyphOutlinedAA(GLYPHBITMAPDATA *glyphData, unsigned long *dst, int thick) {
  ASSERT(glyphData);
  ASSERT(dst);
  ASSERT(glyphData->m_data);

  unsigned int outlineScratch[32][256];
  unsigned int savedGlyphScratch[32][256];
  unsigned int blurScratch[32][256];
  memset(outlineScratch, 0, sizeof(outlineScratch));
  memset(savedGlyphScratch, 0, sizeof(savedGlyphScratch));

  const unsigned char *src = static_cast<const unsigned char *>(glyphData->m_data);
  unsigned int         glyphX = 1 + (thick ? 1 : 0);
  unsigned int         glyphY = glyphData->m_yStart + 1;
  unsigned int        *savedGlyphScratchDst = &savedGlyphScratch[glyphY][glyphX];
  for (unsigned int y = 0; y < glyphData->m_glyphHeight; ++y) {
    for (unsigned int x = 0; x < glyphData->m_glyphWidth; ++x) {
      unsigned int alpha = src[x];
      if (alpha) {
        outlineScratch[glyphY + y][glyphX + x] = 1;
        savedGlyphScratchDst[x] = alpha;
      }
    }
    src += glyphData->m_glyphPitch;
    savedGlyphScratchDst += 256;
  }

  unsigned int cellHeight = m_theFace->m_cellHeight;
  unsigned int cellWidth = glyphData->m_glyphCellWidth;
  unsigned int passes = 1 + (thick ? 1 : 0);

  for (unsigned int pass = 0; pass < passes; ++pass) {
    unsigned int sourceMask = pass ? 3 : 1;
    unsigned int writeBit = pass ? 4 : 2;

    for (unsigned int y = 0; y < cellHeight; ++y) {
      for (unsigned int x = 0; x < cellWidth; ++x) {
        if (outlineScratch[y][x] & sourceMask) {
          continue;
        }

        if (!y) {
          if (!x) {
            if ((outlineScratch[y][x + 1] & sourceMask) || (outlineScratch[y + 1][x + 1] & sourceMask) || (outlineScratch[y + 1][x] & sourceMask)) {
              outlineScratch[y][x] = writeBit;
            }
          } else if (x == cellWidth - 1) {
            if ((outlineScratch[y][x - 1] & sourceMask) || (outlineScratch[y + 1][x - 1] & sourceMask) || (outlineScratch[y + 1][x] & sourceMask)) {
              outlineScratch[y][x] = writeBit;
            }
          } else if (
              (outlineScratch[y][x - 1] & sourceMask) || (outlineScratch[y][x + 1] & sourceMask) || (outlineScratch[y + 1][x - 1] & sourceMask) ||
              (outlineScratch[y + 1][x] & sourceMask) || (outlineScratch[y + 1][x + 1] & sourceMask)
          )
          {
            outlineScratch[y][x] = writeBit;
          }
        } else if (y == cellHeight - 1) {
          if (!x) {
            if ((outlineScratch[y - 1][x] & sourceMask) || (outlineScratch[y - 1][x + 1] & sourceMask) || (outlineScratch[y][x + 1] & sourceMask)) {
              outlineScratch[y][x] = writeBit;
            }
          } else if (x == cellWidth - 1) {
            if ((outlineScratch[y][x - 1] & sourceMask) || (outlineScratch[y - 1][x - 1] & sourceMask) || (outlineScratch[y - 1][x] & sourceMask)) {
              outlineScratch[y][x] = writeBit;
            }
          } else if (
              (outlineScratch[y - 1][x - 1] & sourceMask) || (outlineScratch[y - 1][x] & sourceMask) || (outlineScratch[y - 1][x + 1] & sourceMask) ||
              (outlineScratch[y][x - 1] & sourceMask) || (outlineScratch[y][x + 1] & sourceMask)
          )
          {
            outlineScratch[y][x] = writeBit;
          }
        } else if (!x) {
          if ((outlineScratch[y - 1][x] & sourceMask) || (outlineScratch[y - 1][x + 1] & sourceMask) || (outlineScratch[y][x + 1] & sourceMask) ||
              (outlineScratch[y + 1][x + 1] & sourceMask) || (outlineScratch[y + 1][x] & sourceMask))
          {
            outlineScratch[y][x] = writeBit;
          }
        } else if (x == cellWidth - 1) {
          if ((outlineScratch[y - 1][x] & sourceMask) || (outlineScratch[y - 1][x - 1] & sourceMask) || (outlineScratch[y][x - 1] & sourceMask) ||
              (outlineScratch[y + 1][x - 1] & sourceMask) || (outlineScratch[y + 1][x] & sourceMask))
          {
            outlineScratch[y][x] = writeBit;
          }
        } else if (
            (outlineScratch[y - 1][x - 1] & sourceMask) || (outlineScratch[y - 1][x] & sourceMask) || (outlineScratch[y - 1][x + 1] & sourceMask) ||
            (outlineScratch[y][x - 1] & sourceMask) || (outlineScratch[y][x + 1] & sourceMask) || (outlineScratch[y + 1][x - 1] & sourceMask) ||
            (outlineScratch[y + 1][x] & sourceMask) || (outlineScratch[y + 1][x + 1] & sourceMask)
        )
        {
          outlineScratch[y][x] = writeBit;
        }
      }
    }
  }

  memset(blurScratch, 0, sizeof(blurScratch));
  for (unsigned int lightY = 0; lightY < cellHeight; ++lightY) {
    for (unsigned int x = 0; x < cellWidth; ++x) {
      unsigned int pixelsLit = outlineScratch[lightY][x] != 0;

      if (!lightY) {
        if (!x) {
          pixelsLit += outlineScratch[lightY + 1][x] != 0;
          pixelsLit += outlineScratch[lightY + 1][x + 1] != 0;
          pixelsLit += outlineScratch[lightY][x + 1] != 0;
        } else if (x == cellWidth - 1) {
          pixelsLit += outlineScratch[lightY][x - 1] != 0;
          pixelsLit += outlineScratch[lightY + 1][x - 1] != 0;
          pixelsLit += outlineScratch[lightY + 1][x] != 0;
        } else {
          pixelsLit += outlineScratch[lightY][x - 1] != 0;
          pixelsLit += outlineScratch[lightY + 1][x - 1] != 0;
          pixelsLit += outlineScratch[lightY + 1][x] != 0;
          pixelsLit += outlineScratch[lightY + 1][x + 1] != 0;
          pixelsLit += outlineScratch[lightY][x + 1] != 0;
        }
      } else if (lightY == cellHeight - 1) {
        if (!x) {
          pixelsLit += outlineScratch[lightY - 1][x] != 0;
          pixelsLit += outlineScratch[lightY - 1][x + 1] != 0;
          pixelsLit += outlineScratch[lightY][x + 1] != 0;
        } else if (x == cellWidth - 1) {
          pixelsLit += outlineScratch[lightY - 1][x - 1] != 0;
          pixelsLit += outlineScratch[lightY - 1][x] != 0;
          pixelsLit += outlineScratch[lightY][x - 1] != 0;
        } else {
          pixelsLit += outlineScratch[lightY - 1][x - 1] != 0;
          pixelsLit += outlineScratch[lightY - 1][x] != 0;
          pixelsLit += outlineScratch[lightY - 1][x + 1] != 0;
          pixelsLit += outlineScratch[lightY][x - 1] != 0;
          pixelsLit += outlineScratch[lightY][x + 1] != 0;
        }
      } else if (!x) {
        pixelsLit += outlineScratch[lightY - 1][x] != 0;
        pixelsLit += outlineScratch[lightY - 1][x + 1] != 0;
        pixelsLit += outlineScratch[lightY + 1][x] != 0;
        pixelsLit += outlineScratch[lightY + 1][x + 1] != 0;
        pixelsLit += outlineScratch[lightY][x + 1] != 0;
      } else if (x == cellWidth - 1) {
        pixelsLit += outlineScratch[lightY - 1][x - 1] != 0;
        pixelsLit += outlineScratch[lightY - 1][x] != 0;
        pixelsLit += outlineScratch[lightY + 1][x - 1] != 0;
        pixelsLit += outlineScratch[lightY + 1][x] != 0;
        pixelsLit += outlineScratch[lightY][x - 1] != 0;
      } else {
        pixelsLit += outlineScratch[lightY - 1][x - 1] != 0;
        pixelsLit += outlineScratch[lightY - 1][x] != 0;
        pixelsLit += outlineScratch[lightY - 1][x + 1] != 0;
        pixelsLit += outlineScratch[lightY + 1][x - 1] != 0;
        pixelsLit += outlineScratch[lightY + 1][x] != 0;
        pixelsLit += outlineScratch[lightY + 1][x + 1] != 0;
        pixelsLit += outlineScratch[lightY][x + 1] != 0;
        pixelsLit += outlineScratch[lightY][x - 1] != 0;
      }

      blurScratch[lightY][x] = pixelsLitLevels[pixelsLit];
    }
  }

  for (unsigned int outputY = 0; outputY < cellHeight; ++outputY) {
    for (unsigned int x = 0; x < cellWidth; ++x) {
      if (!outlineScratch[outputY][x]) {
        dst[x] = 0;
      } else if (savedGlyphScratch[outputY][x]) {
        unsigned int gray = (255 * savedGlyphScratch[outputY][x]) >> 8;
        dst[x] = (blurScratch[outputY][x] << 24) | (gray << 16) | (gray << 8) | gray;
      } else {
        dst[x] = blurScratch[outputY][x] << 24;
      }
    }
    dst += 256;
  }
}

void TEXTURECACHE::PasteGlyphNonOutlinedAA(GLYPHBITMAPDATA *glyphData, unsigned long *dst) {
  ASSERT(glyphData);
  ASSERT(dst);

  char        *src = static_cast<char *>(glyphData->m_data);
  unsigned int pitch = glyphData->m_glyphPitch;
  unsigned int dstCellStride = 4 * glyphData->m_glyphCellWidth;

  unsigned int y;
  for (y = 0; y < static_cast<unsigned int>(glyphData->m_yStart); ++y) {
    memset(dst, 0, dstCellStride);
    dst += 256;
  }

  for (y = 0; y < glyphData->m_glyphHeight; ++y) {
    for (unsigned int x = 0; x < glyphData->m_glyphWidth; ++x) {
      dst[x] = (static_cast<unsigned int>(src[x]) << 24) | 0x00FFFFFF;
    }
    src += pitch;
    dst += 256;
  }

  int remaining = static_cast<int>(m_theFace->m_cellHeight) - static_cast<int>(glyphData->m_glyphHeight) - glyphData->m_yStart;
  while (remaining > 0) {
    memset(dst, 0, dstCellStride);
    dst += 256;
    --remaining;
  }
}

void TEXTURECACHE::PasteGlyph(GLYPHBITMAPDATA *data, unsigned long *dst, int thick) {
  ASSERT(m_theFace);

  if (m_theFace->m_flags & 0x1) {
    if (m_theFace->m_flags & 0x10) {
      PasteGlyphOutlinedMonochrome(data, dst, thick);
    } else {
      PasteGlyphOutlinedAA(data, dst, thick);
    }
  } else if (m_theFace->m_flags & 0x10) {
    PasteGlyphNonOutlinedMonochrome(data, dst);
  } else {
    PasteGlyphNonOutlinedAA(data, dst);
  }
}

void CGxString::SetStringPosition(const NTempest::C3Vector &position) {
  m_position = position;
  InitializeViewportOffsets();

  if (m_batchedStringLink.IsLinked()) {
    float minx;
    float maxx;
    float miny;
    float maxy;
    float minz;
    float maxz;

    GxXformViewport(minx, maxx, miny, maxy, minz, maxz);
    ClearStringMatrixEntry();
  }
}

void CGxString::SetColor(const NTempest::CImVector &color) {
  if (*reinterpret_cast<const unsigned long *>(&m_fontColor) == *reinterpret_cast<const unsigned long *>(&color)) {
    return;
  }

  m_fontColor = color;
  m_shadowColor.a = min(color.a, m_shadowColor.a);

  if (!(m_flags & 0x8) || (m_flags & 0x1000) || m_colorGradients.Count() || m_colorGradientShadows.Count()) {
    CreateGeometry();
  }
}

void CGxString::InternalRender() {
  GxVertexShaderSelect(GxVS_PassThru);
  GxRsPush();
  GxRsSet(GxRs_Lighting, 0);
  GxRsSet(GxRs_Culling, 0);
  GxRsSet(GxRs_Blend, GxBlend_Alpha);

  static NTempest::C3Vector normal(0.0f, 0.0f, -1.0f);

  if (static_cast<signed char>(m_flags) >= 0) {
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_DepthTest, 0);
    GxRsSet(GxRs_Fog, 0);
  }

  ASSERT(m_currentFace);

  for (unsigned int line = 0; line < m_textBlock.m_lines.Count(); ++line) {
    for (unsigned int i = 0; i < m_textBlock.m_lines[line]->m_texturePages.Count(); ++i) {
      m_textBlock.m_lines[line]->m_texturePages[i]->InternalRenderTexture(
          i, m_currentFace, m_flags & 0x1, m_shadowColor, m_shadowOffset, m_fontColor
      );
    }
  }

  GxRsPop();
}

void CGxString::Recycle() {
  m_batchedStringLink.Unlink();
  g_freeStrings.LinkNode(this, LIST_TAIL, 0);
  ClearInstanceData();
  ClearStringMatrixEntry();
}

void CGxString::ClearInstanceData() {
  m_hyperlinkInfo.SetCount(0);
  m_textBlock.Recycle();
  m_colorGradients.SetCount(0);
  m_colorGradientShadows.SetCount(0);
  m_lastGradientStart = -1;
  m_lastGradientLength = -1;
}

void CGxString::Render(const NTempest::C44Matrix &xform) {
  if (m_textBlock.m_lines.Count()) {
    CheckEvictedTextures();
    GxXformPush(GxXform_World, xform);
    InternalRender();
    GxXformPop(GxXform_World);
  }
}

void CGxString::Render() {
  float               minx;
  float               maxx;
  float               miny;
  float               maxy;
  float               minz;
  float               maxz;
  NTempest::C44Matrix oldProjection;
  NTempest::C44Matrix oldView;
  NTempest::C44Matrix proj;
  NTempest::C44Matrix view;
  float               pixWidth;
  float               pixHeight;

  if (!m_textBlock.m_lines.Count()) {
    return;
  }

  CheckEvictedTextures();
  GxXformViewport(minx, maxx, miny, maxy, minz, maxz);
  GxXformProjection(oldProjection);
  GxXformView(oldView);

  pixWidth = static_cast<float>(GetScreenPixelWidth());
  pixHeight = static_cast<float>(GetScreenPixelHeight());
  BuildProjection(&proj, minx, maxx, miny, maxy, pixWidth, pixHeight);
  GxXformSetProjection(proj);
  BuildView(&view, maxx - minx, maxy - miny);
  GxXformSetView(view);
  InternalRender();

  GxXformSetProjection(oldProjection);
  GxXformSetView(oldView);
}

void CGxString::CreateGeometry() {
  HYPERLINKPARSEINFO  info;
  NTempest::C3Vector  linePos;
  int                 gStart = m_lastGradientStart;
  unsigned int        advance;
  NTempest::CImVector workingColor;
  unsigned int        wide;
  int                 gLength = m_lastGradientLength;
  unsigned int        texturePagesUsedFlag;
  unsigned int        numBytes;
  float               widestLineExtent;
  const char         *nextText;
  float               height;
  unsigned int        loop;
  float               lineHeight;
  float               extent;
  const char         *currentText;

  ClearInstanceData();

  linePos.x = 0.0f;
  linePos.y = ScreenToPixelHeight(m_flags & 0x80, -m_currentFontHeight);
  linePos.z = m_position.z;
  workingColor = m_fontColor;

  lineHeight = ScreenToPixelHeight(m_flags & 0x80, m_spacing + m_currentFontHeight);

  info.hyperlinkParseMode = HYPERLINKNONE;
  info.currentParseInfo.extent.l = 0.0f;
  info.currentParseInfo.extent.r = 0.0f;
  info.currentParseInfo.extent.b = linePos.y;
  info.currentParseInfo.extent.t = linePos.y + lineHeight;

  currentText = m_text;
  nextText = 0;
  numBytes = 0;
  widestLineExtent = 0.0f;

  height = m_blockHeight / (m_spacing + m_currentFontHeight);
  loop = static_cast<unsigned int>(NTempest::CMath::fuint_n(height));

  if (currentText) {
    while (loop-- && *currentText) {
      QUOTEDCODE quotedCode = GxuDetermineQuotedCode(currentText, advance, 0, m_flags, wide, SStrLen(currentText));
      extent = 0.0f;

      if (wide == '\n' || quotedCode == CODE_NEWLINE) {
        currentText += advance;
        m_textBlock.NewLine();
        linePos.y -= lineHeight;
      } else {
        CalcWrapPoint(m_currentFace, currentText, m_currentFontHeight, m_blockWidth, &numBytes, &extent, &nextText, m_flags);

        if (extent > widestLineExtent) {
          widestLineExtent = extent;
        }

        if (!numBytes && !*nextText) {
          break;
        }

        if (m_horzJust == GxHJ_Right) {
          linePos.x = ScreenToPixelWidth(m_flags & 0x80, -extent);
        } else if (m_horzJust == GxHJ_Center) {
          linePos.x = ScreenToPixelWidth(m_flags & 0x80, -extent * 0.5f);
        }

        texturePagesUsedFlag = 0;
        if (info.hyperlinkParseMode != HYPERLINKNONE) {
          info.currentParseInfo.extent.l = linePos.x;
        }

        InitializeTextLine(currentText, numBytes, workingColor, linePos, &texturePagesUsedFlag, info);

        ASSERT(info.hyperlinkParseMode != HYPERLINKHREF);
        if (info.hyperlinkParseMode == HYPERLINKDISPLAY) {
          AddHyperlinkParseInfo(info.currentParseInfo);
        }

        currentText = nextText;
        m_texturePagesUsed |= texturePagesUsedFlag;
        linePos.y -= lineHeight;

        if (m_flags & 0x2) {
          break;
        }

        info.currentParseInfo.extent.b -= lineHeight;
        info.currentParseInfo.extent.t -= lineHeight;
      }

      if (!currentText) {
        break;
      }
    }
  }

  m_savedWidth = widestLineExtent;
  InitializeViewportOffsets();
  GenerateVertexIndices();

  if ((m_flags & 0x1000) && (gStart != -1 || gLength != -1)) {
    SetGradient(gStart, gLength);
  }
}

void CGxString::InitializeViewportOffsets() {
  unsigned int lineCount = m_textBlock.m_lines.Count();
  float        tHeight =
      (static_cast<float>(lineCount - 1) * m_spacing + static_cast<float>(lineCount) * m_currentFontHeight) * static_cast<float>(g_heightPixels);
  float              blockWidth = static_cast<float>(g_widthPixels) * m_blockWidth;
  float              blockHeight = static_cast<float>(g_heightPixels) * m_blockHeight;
  NTempest::C2Vector position(static_cast<float>(g_widthPixels) * m_position.x, static_cast<float>(g_heightPixels) * m_position.y);
  float              minx;
  float              maxx;
  float              miny;
  float              maxy;
  float              minz;
  float              maxz;

  m_stringHeight = tHeight;
  GxXformViewport(minx, maxx, miny, maxy, minz, maxz);

  if (m_horzJust == GxHJ_Right) {
    position.x += blockWidth;
  } else if (m_horzJust == GxHJ_Center) {
    position.x += blockWidth * 0.5f;
  }

  if (m_vertJust == GxVJ_Top) {
    position.y += blockHeight;
  } else {
    if (m_vertJust == GxVJ_Middle) {
      position.y += (blockHeight - tHeight) * 0.5f;
    }
    position.y += tHeight;
  }

  m_viewportOffset.x = position.x / (maxx - minx);
  m_viewportOffset.y = position.y / (maxy - miny);
}

void CGxString::TexturePageEvicted(unsigned int pageNumber) {
  if ((1 << pageNumber) & m_texturePagesUsed) {
    m_textureEvicted = 1;
  }
}

void CGxString::GenerateVertexIndices() {
  static const unsigned short baseIndices[6] = {0, 1, 3, 1, 2, 3};
  IGXUTEXTLINE              **textLine;
  TEXTLINETEXTURE           **textureLine;

  for (textLine = m_textBlock.m_lines.Ptr(); textLine < m_textBlock.m_lines.Ptr() + m_textBlock.m_lines.Count(); ++textLine) {
    for (textureLine = (*textLine)->m_texturePages.Ptr(); textureLine < (*textLine)->m_texturePages.Ptr() + (*textLine)->m_texturePages.Count();
         ++textureLine)
    {
      TEXTLINETEXTURE *page = *textureLine;
      unsigned int     numVerts = page->m_vert.Count();
      ASSERT(!(numVerts % 4));

      unsigned int numQuads = numVerts / 4;
      page->m_vertIndices.SetCount(numQuads * 6);

      for (unsigned int quad = 0; quad < numQuads; ++quad) {
        for (unsigned int index = 0; index < 6; ++index) {
          page->m_vertIndices[quad * 6 + index] = static_cast<unsigned short>(quad * 4 + baseIndices[index]);
        }
      }
    }
  }
}

CGxString *CGxString::GetNewString(int linkonList) {
  CGxString *string = g_freeStrings.Head();

  if (string) {
    g_freeStrings.UnlinkNode(string);
    return string;
  }

  return g_strings.NewNode(linkonList ? LIST_TAIL : 0, 0, 0);
}

void CGxString::BuildView(NTempest::C44Matrix *viewPtr, float width, float height) {
  ASSERT(viewPtr);

  NTempest::C44Matrix view;
  float               translateX;
  float               translateY;

  if (m_flags & 0x20) {
    translateX = m_viewportOffset.x;
    translateY = m_viewportOffset.y;
  } else {
    translateX = width * m_viewportOffset.x;
    translateY = height * m_viewportOffset.y;
  }

  view.Translate(NTempest::C3Vector(static_cast<float>(floor(translateX)), static_cast<float>(floor(translateY)), 0.0f));
  *viewPtr = view;
}

void CGxString::BuildProjection(NTempest::C44Matrix *projPtr, float minx, float maxx, float miny, float maxy, float pixWidth, float pixHeight) {
  ASSERT(projPtr);

  NTempest::C44Matrix proj;
  float               pixelMinY = static_cast<float>(floor(miny * pixHeight));
  float               pixelMaxX = static_cast<float>(floor(maxx * pixWidth));
  float               pixelMinX = static_cast<float>(floor(minx * pixWidth));
  float               pixelMaxY = static_cast<float>(floor(maxy * pixHeight));

  GxuXformCreateOrtho(pixelMinX, pixelMaxX, pixelMinY, pixelMaxY, -1.0f, 1.0f, proj);
  *projPtr = proj;
}

void CGxString::SetCharSpacing(float spacing) {
}

CGxString::CGxString()
    : m_requestedFontHeight(0.02f),
      m_currentFontHeight(0.02f),
      m_position(0.0f),
      m_fontColor(0xFFFFFFFFul),
      m_shadowColor(0xFF000000ul),
      m_shadowOffset(0.00125f, -0.00125f),
      m_blockWidth(1.0f),
      m_blockHeight(1.0f),
      m_currentFace(0),
      m_text(0),
      m_textLen(0),
      m_vertJust(GxVJ_Top),
      m_horzJust(GxHJ_Left),
      m_spacing(0.0f),
      m_flags(0),
      m_viewportOffset(0.0f),
      m_texturePagesUsed(0),
      m_textureEvicted(0),
      m_stringHeight(0.0f),
      m_savedWidth(0.0f),
      m_lastGradientStart(-1),
      m_lastGradientLength(-1) {
  m_colorGradients.SetChunkSize(128);
  m_colorGradientShadows.SetChunkSize(128);
}

void CGxString::RemoveShadow() {
  m_flags &= ~m_flags & 0x1;
}

void CGxFont::RegisterEvictNotice(unsigned int pageNumber) {
  ASSERT(pageNumber < 8);

  ITERATELIST(CGxString, m_strings, string) {
    string->TexturePageEvicted(pageNumber);
  }
}

int CGxFont::CheckStringGlyphs(const char *string) {
  while (*string) {
    if (*string != '\n' && !m_activeCharacters.Ptr(static_cast<signed char>(*string), s_nullHashKey)) {
      return 0;
    }
    ++string;
  }

  return 1;
}

CGxString::~CGxString() {
  ClearStringMatrixEntry();
  m_textBlock.Destroy();
  FREEIFUSED(m_text);
}

int CGxFont::UpdateDimensions() {
  m_currentFontHeight = max(m_requestedFontHeight, 2.0f / static_cast<float>(g_heightPixels));
  m_pixelSize = min(28, static_cast<int>(ScreenToPixelHeight(0, m_currentFontHeight)));

  if (!m_pixelSize) {
    FATALERROR(
        ("Error, font %s being created with height %g at screen res of %dx%d which is %d pixels!", m_fontName, m_currentFontHeight, g_widthPixels,
         g_heightPixels, m_pixelSize)
    );
  }

  ASSERT(m_faceHandle);
  FT_Face theFace = FontFaceGetFace(m_faceHandle);
  ASSERT(theFace);

  float baseLine = theFace->ascender / (fabs(static_cast<float>(theFace->descender)) + theFace->ascender) * m_pixelSize;
  m_baseline = static_cast<unsigned int>(baseLine + SignOf(baseLine) * 0.5f);

  m_cellHeight = m_pixelSize;
  if (m_flags & 0x1) {
    m_cellHeight += 2;
  }
  if (m_flags & 0x8) {
    m_cellHeight += 2;
  }

  FT_Error error = FT_Set_Pixel_Sizes(theFace, m_pixelSize, 0);
  m_pixelsPerUnit = static_cast<float>(theFace->size->metrics.x_ppem) / static_cast<float>(theFace->units_per_EM);
  ASSERT(m_pixelsPerUnit != 0.0f);
  return !error;
}

const CHARCODEDESC *CGxFont::NewCodeDesc(unsigned int code) {
  CHARCODEDESC *desc = m_activeCharacters.Ptr(code, s_nullHashKey);
  if (desc) {
    m_activeCharacterCache.LinkNode(desc, LIST_HEAD, 0);
    return desc;
  }

  ASSERT(m_faceHandle);
  FT_Face theFace = FontFaceGetFace(m_faceHandle);
  ASSERT(theFace);

  GLYPHBITMAPDATA *data = m_glyphBitmapData.Ptr(code, s_nullHashKey);
  if (!data) {
    data = NEW(GLYPHBITMAPDATA);
    if (!GetGlyphData(data, theFace, code)) {
      delete data;
      return 0;
    }
    m_glyphBitmapData.Insert(data, code, s_nullHashKey);
  }

  unsigned int textureNumber = 0;
  while (textureNumber < 8 && m_textureCache[textureNumber].m_texture) {
    desc = m_textureCache[textureNumber].AllocateNewGlyph(data);
    if (desc) {
      desc->textureNumber = textureNumber;
      break;
    }
    ++textureNumber;
  }

  if (!desc && textureNumber < 8) {
    TEXTURECACHE *texture = &m_textureCache[textureNumber];
    texture->CreateTexture(m_flags & 0x4);
    texture->Initialize(this, textureNumber, m_cellHeight);
    desc = texture->AllocateNewGlyph(data);
    if (desc) {
      desc->textureNumber = textureNumber;
    }
  }

  if (!desc) {
    CHARCODEDESC *oldestDesc = m_activeCharacterCache.Tail();
    if (oldestDesc) {
      textureNumber = oldestDesc->textureNumber;
      ASSERT(textureNumber < (sizeof(m_textureCache) / sizeof(m_textureCache[0])));

      unsigned int  rowNumber = oldestDesc->rowNumber;
      TEXTURECACHE *texture = &m_textureCache[textureNumber];
      ASSERT(rowNumber < texture->m_textureRows.Count());

      TEXTURECACHEROW *row = &texture->m_textureRows[rowNumber];
      row->EvictGlyph(oldestDesc);
      RegisterEvictNotice(textureNumber);
      desc = row->CreateNewDesc(data, rowNumber, m_cellHeight);
      if (desc) {
        desc->rowNumber = rowNumber;
        desc->textureNumber = textureNumber;
        texture->m_anyDirtyGlyphs = 1;
      }
    }
  }

  if (desc) {
    data = 0;
    ASSERT(desc->ValidTextureCoords());
    m_activeCharacters.Insert(desc, code, s_nullHashKey);
    m_activeCharacterCache.LinkNode(desc, LIST_HEAD, 0);
  }

  if (data) {
    delete data;
  }
  return desc;
}

int CGxFont::GetGlyphData(GLYPHBITMAPDATA *glyphData, FT_Face face, unsigned int code) {
  ASSERT(face);
  ASSERT(glyphData);

  FT_Error error = FT_Set_Pixel_Sizes(face, m_pixelSize, 0);
  ASSERT(!error);

  GLYPHDATA data;
  if (!IGxuFontGlyphRenderGlyph(face, m_pixelSize, code, m_baseline, &data, (~m_flags >> 1) & 1, m_flags & 0x10)) {
    return 0;
  }

  glyphData->Clear();
  glyphData->m_code = code;
  glyphData->m_data = data.data;
  glyphData->m_dataSize = data.dataSize;
  glyphData->m_dirty = 1;
  glyphData->m_glyphWidth = data.freeTypeGlyphWidth;
  glyphData->m_glyphCellWidth = data.freeTypeGlyphWidth;
  if (m_flags & 0x1) {
    glyphData->m_glyphCellWidth += 2;
  }
  if (m_flags & 0x8) {
    glyphData->m_glyphCellWidth += 2;
  }
  glyphData->m_glyphHeight = data.freeTypeGlyphHeight;
  glyphData->m_glyphPitch = data.freeTypeGlyphPitch;
  glyphData->m_yOffset = data.yOffset;
  glyphData->m_yStart = data.yStart;
  glyphData->m_glyphAdvance = data.freeTypeGlyphAdvance;
  glyphData->m_glyphBearing = data.freeTypeGlyphBearing;
  data.data = 0;
  return 1;
}

int CGxString::Initialize(
    float                      fontHeight,
    const NTempest::C3Vector  &position,
    float                      blockWidth,
    float                      blockHeight,
    CGxFont                   *face,
    const char                *text,
    EGxFontVJusts              vertJust,
    EGxFontHJusts              horzJust,
    float                      spacing,
    unsigned int               flags,
    const NTempest::CImVector &color
) {
  unsigned int textLen;

  ASSERT(text);
  ASSERT(face);

  textLen = SStrLen(text) + 1;
  if (textLen > m_textLen) {
    FREEIFUSED(m_text);
    m_textLen = textLen;
    m_text = static_cast<char *>(ALLOC(m_textLen));
  }

  SStrCopy(m_text, text, m_textLen);
  m_blockWidth = blockWidth;
  m_blockHeight = blockHeight;
  m_spacing = spacing;
  m_position = position;
  m_vertJust = vertJust;
  m_horzJust = horzJust;
  m_flags = flags;
  m_fontColor = color;

  if (!(m_flags & 0x80)) {
    m_position.z = 0.0f;
  }

  m_currentFace = face;
  m_currentFace->m_strings.LinkNode(this, LIST_TAIL, 0);

  if ((flags & 0x4) && !(flags & 0x80)) {
    m_requestedFontHeight = GxuFontGetOneToOneHeight(face);
  } else {
    m_requestedFontHeight = fontHeight;
  }

  m_currentFontHeight = max(m_requestedFontHeight, 2.0f / static_cast<float>(g_heightPixels));
  CreateGeometry();
  return 1;
}

unsigned int CGxFont::GetNumCurrentTextures() {
  unsigned int count = 0;
  int          emptyFound = 0;

  for (unsigned int i = 0; i < 8; ++i) {
    if (m_textureCache[i].m_texture) {
      ASSERT(!emptyFound);
      ++count;
    } else {
      emptyFound = 1;
    }
  }

  return count;
}

float CGxFont::GetCharAdvance(unsigned int code) {
  GLYPHBITMAPDATA *glyph = m_glyphBitmapData.Ptr(code, s_nullHashKey);

  ASSERT(glyph);
  return glyph->m_glyphAdvance * m_pixelsPerUnit;
}

int CGxFont::Initialize(const char *name, unsigned int newFlags, float fontHeight) {
  ASSERT(name && *name);

  SStrPrintf(m_fontName, sizeof(m_fontName), "%s", name);
  m_requestedFontHeight = fontHeight;
  m_currentFontHeight = max(fontHeight, 2.0f / static_cast<float>(g_heightPixels));
  m_pixelSize = min(static_cast<unsigned int>(ScreenToPixelHeight(0, m_currentFontHeight)), 32u);

  if (!m_pixelSize) {
    FATALERROR(
        ("Error, font %s being created with height %g at screen res of %dx%d which is %d pixels!", name, fontHeight, g_widthPixels, g_heightPixels,
         m_pixelSize)
    );
  }

  Clear();
  m_flags = newFlags;
  if (m_faceHandle) {
    FontFaceCloseHandle(m_faceHandle);
  }

  m_faceHandle = FontFaceGetHandle(name, GetFreeTypeLibrary());
  if (!m_faceHandle) {
    return 0;
  }

  return UpdateDimensions();
}

float CGxFont::ComputeStep(unsigned int currentCode, unsigned int nextCode) {
  KERNNODE *node = m_kernInfo.Ptr(currentCode, KERNINGHASHKEY(currentCode, nextCode));

  if (node && (node->flags & 0x2)) {
    return node->proporportionalSpacing;
  }

  ASSERT(m_faceHandle);

  FT_Face theFace = FontFaceGetFace(m_faceHandle);
  ASSERT(theFace);

  unsigned int currentGlyph;
  FT_Vector    vector;

  currentGlyph = FT_Get_Char_Index(theFace, currentCode);
  vector.x = 0;
  if (FT_HAS_KERNING(theFace)) {
    FT_Get_Kerning(theFace, currentGlyph, FT_Get_Char_Index(theFace, nextCode), ft_kerning_unscaled, &vector);
    vector.x = min(vector.x, 0);
  }

  GLYPHBITMAPDATA *glyph = m_glyphBitmapData.Ptr(currentCode, s_nullHashKey);
  ASSERT(glyph);

  float spacing = (glyph->m_glyphAdvance + vector.x) * m_pixelsPerUnit;

  if (!node) {
    node = m_kernInfo.New(currentCode, KERNINGHASHKEY(currentCode, nextCode), 0, 0);
  }

  node->flags |= 0x2;
  node->proporportionalSpacing = static_cast<float>(ceil(spacing));
  return node->proporportionalSpacing;
}

float CGxFont::ComputeStepFixedWidth(unsigned int currentCode, unsigned int nextCode) {
  KERNNODE *node = m_kernInfo.Ptr(currentCode, KERNINGHASHKEY(currentCode, nextCode));

  if (node && (node->flags & 0x1)) {
    return node->fixedWidthSpacing;
  }

  GLYPHBITMAPDATA *currentGlyph = m_glyphBitmapData.Ptr(currentCode, s_nullHashKey);
  GLYPHBITMAPDATA *nextGlyph;
  nextGlyph = m_glyphBitmapData.Ptr(nextCode, s_nullHashKey);

  if (currentGlyph && nextGlyph) {
    unsigned int currentAdvance = static_cast<unsigned int>(currentGlyph->m_glyphAdvance * m_pixelsPerUnit);

    if (currentAdvance >= m_cellHeight) {
      return static_cast<float>(m_cellHeight);
    }

    unsigned int currentPadding = m_cellHeight - currentAdvance;
    if (currentPadding & 0x1) {
      --currentPadding;
    }

    unsigned int spacing = m_cellHeight - currentPadding / 2;
    unsigned int nextAdvance = static_cast<unsigned int>(nextGlyph->m_glyphAdvance * m_pixelsPerUnit);

    if (nextAdvance >= m_cellHeight) {
      return static_cast<float>(spacing);
    }

    unsigned int nextPadding = m_cellHeight - nextAdvance;
    if (nextPadding & 0x1) {
      --nextPadding;
    }

    return static_cast<float>(spacing + nextPadding / 2);
  }

  if (!node) {
    node = m_kernInfo.New(currentCode, KERNINGHASHKEY(currentCode, nextCode), 0, 0);
  }

  node->flags |= 0x1;
  node->fixedWidthSpacing = 0.0f;
  return 0.0f;
}

void CGxString::HandleScreenSizeChange() {
  ASSERT(m_currentFace);
  ClearStringMatrixEntry();

  if (m_flags & 0x4) {
    m_requestedFontHeight = GxuFontGetOneToOneHeight(m_currentFace);
  }

  m_currentFontHeight = max(m_requestedFontHeight, 2.0f / static_cast<float>(g_heightPixels));
  CreateGeometry();
}

const char *CGxFont::GetName() const {
  ASSERT(m_faceHandle);
  return FontFaceGetFontName(m_faceHandle);
}

void CGxFont::HandleScreenSizeChange() {
  int success;

  ASSERT(m_faceHandle);
  ClearGlyphs();
  success = UpdateDimensions();
  ASSERT(success);

  ITERATELIST(CGxString, m_strings, string) {
    string->HandleScreenSizeChange();
  }
}

void GLYPHBITMAPDATA::Clear() {
  FREEIFUSED(m_data);
  m_data = 0;
}

void CGxFont::Clear() {
  if (m_faceHandle) {
    FontFaceCloseHandle(m_faceHandle);
  }

  m_faceHandle = 0;
  ClearGlyphs();
}

void CGxFont::ClearGlyphs() {
  unsigned int i;

  for (i = 0; i < 8; ++i) {
    m_textureCache[i].Clear();
  }

  m_activeCharacters.Clear();
  m_glyphBitmapData.Clear();
  m_activeCharacterCache.Clear();
  m_kernInfo.Clear();
}

CGxFont::CGxFont() : m_cellHeight(0), m_pixelsPerUnit(0.0f) {
}

CGxFont::~CGxFont() {
  Clear();
}

void CGxFont::UpdateTextures() {
  for (unsigned int i = 0; i < 8; ++i) {
    m_textureCache[i].Update();
  }
}

void TEXTURECACHEROW::EvictGlyph(CHARCODEDESC *&desc) {
  ASSERT(desc);
  ASSERT(desc->bitmapData);

  unsigned int pixelsNeeded = desc->bitmapData->m_glyphCellWidth;
  ASSERT(pixelsNeeded);

  CHARCODEDESC *current = glyphList.Head();
  while (current && current != desc) {
    current = glyphList.Next(current);
  }

  if (!current) {
    FATALERROR(("Error, can't locate cell in the row to evict!"));
  }

  unsigned int freedPixels = 0;
  while (desc && freedPixels < pixelsNeeded) {
    unsigned int currentCellUsage = desc->glyphEndPixel - desc->glyphStartPixel + 1;
    ASSERT(currentCellUsage);
    freedPixels += currentCellUsage + desc->GapToNextTexture();
    desc = glyphList.DeleteNode(desc);
  }

  if (!desc && freedPixels < pixelsNeeded) {
    desc = glyphList.Tail();
    while (desc && freedPixels < pixelsNeeded) {
      unsigned int currentCellUsage = desc->glyphEndPixel - desc->glyphStartPixel + 1;
      ASSERT(currentCellUsage);
      freedPixels += currentCellUsage + desc->GapToPreviousTexture();
      desc = glyphList.DeleteNode(desc);
    }
    ASSERT(freedPixels >= pixelsNeeded);
  }

  current = glyphList.Head();
  widestFreeSlot = current ? current->GapToPreviousTexture() : 0;
  while (current) {
    unsigned int gap = current->GapToNextTexture();
    if (gap > widestFreeSlot) {
      widestFreeSlot = gap;
    }
    current = glyphList.Next(current);
  }
}

CHARCODEDESC *TEXTURECACHEROW::CreateNewDesc(GLYPHBITMAPDATA *data, unsigned int rowNumber, unsigned int glyphCellHeight) {
  int           inserted;
  unsigned int  glyphWidth = data->m_glyphCellWidth;
  CHARCODEDESC *next;
  CHARCODEDESC *current;
  CHARCODEDESC *newNode;

  ASSERT(glyphWidth);

  if (widestFreeSlot < glyphWidth) {
    return 0;
  }

  current = glyphList.Head();
  if (!current) {
    newNode = NEW(CHARCODEDESC);
    glyphList.LinkNode(newNode, LIST_TAIL, 0);
    widestFreeSlot -= glyphWidth;
    newNode->glyphStartPixel = 0;
    newNode->glyphEndPixel = glyphWidth - 1;
    newNode->bitmapData = data;
    newNode->GenerateTextureCoords(rowNumber, glyphCellHeight);
    newNode->dataValid = 1;
    return newNode;
  }

  widestFreeSlot = current->GapToPreviousTexture();
  if (widestFreeSlot >= glyphWidth) {
    newNode = NEW(CHARCODEDESC);
    glyphList.LinkNode(newNode, LIST_LINK_BEFORE, current);
    ASSERT(current->glyphStartPixel >= 1);
    newNode->glyphEndPixel = current->glyphStartPixel - 1;
    newNode->glyphStartPixel = newNode->glyphEndPixel - glyphWidth + 1;
    newNode->bitmapData = data;
    newNode->GenerateTextureCoords(rowNumber, glyphCellHeight);
    newNode->dataValid = 1;

    widestFreeSlot = glyphList.Head()->GapToPreviousTexture();
    current = glyphList.Head();
    while (current) {
      if (current->GapToNextTexture() > widestFreeSlot) {
        widestFreeSlot = current->GapToNextTexture();
      }
      current = glyphList.Next(current);
    }
    return newNode;
  }

  inserted = 0;
  newNode = 0;
  while (current) {
    next = glyphList.Next(current);
    ASSERT(current->ValidTextureCoords());
    ASSERT(!next || next->ValidTextureCoords());

    if (inserted) {
      if (current->GapToNextTexture() > widestFreeSlot) {
        widestFreeSlot = current->GapToNextTexture();
      }
    } else if (current->GapToNextTexture() >= glyphWidth) {
      newNode = NEW(CHARCODEDESC);
      glyphList.LinkNode(newNode, LIST_LINK_AFTER, current);
      newNode->glyphStartPixel = current->glyphEndPixel + 1;
      newNode->glyphEndPixel = current->glyphEndPixel + glyphWidth;
      newNode->bitmapData = data;
      newNode->GenerateTextureCoords(rowNumber, glyphCellHeight);
      newNode->dataValid = 1;
      next = newNode;
      inserted = 1;
    } else if (!next) {
      return 0;
    }

    current = next;
  }

  return newNode;
}

TEXTURECACHE::TEXTURECACHE() : m_anyDirtyGlyphs(1), m_data(0), m_texture(0), m_theFace(0), m_page(0) {
}

void TEXTURECACHE::Initialize(CGxFont *face, unsigned int thePage, unsigned int pixelSize) {
  m_theFace = face;
  m_page = thePage;
  ASSERT(pixelSize);

  unsigned int rowCount = 256 / pixelSize;
  m_textureRows.SetCount(rowCount);
  for (unsigned int row = 0; row < rowCount; ++row) {
    m_textureRows[row].widestFreeSlot = 256;
  }
}

void TEXTURECACHE::CreateTexture(int filter) {
  ASSERT(!m_texture);

  int success = GxTexCreate(
      GxTex_2d, 256, 256, 0, GxTex_Argb4444, GxTex_Argb8888, CGxTexFlags(filter ? GxTex_Linear : GxTex_Nearest, 0, 0, 0, 0, 0, 1), this,
      TextureCallback, m_texture
  );
  ASSERT(success);
}

TEXTURECACHE::~TEXTURECACHE() {
  Clear();
}

void TEXTURECACHE::Update() {
  if (m_texture && m_anyDirtyGlyphs) {
    GxTexUpdate(m_texture, 0, 0, 256, 256, 1);
  }
}

void TEXTURECACHE::TextureCallback(
    EGxTexCommand cmd,
    unsigned int  w,
    unsigned int  h,
    unsigned int  d,
    unsigned int  mipLevel,
    void         *userArg,
    unsigned int &texelStrideInBytes,
    const void  *&texels
) {
  ASSERT(w);
  ASSERT(h);
  ASSERT(userArg);

  static_cast<TEXTURECACHE *>(userArg)->TextureCallbackHandler(cmd, w, h, mipLevel, texelStrideInBytes, texels);
}

void TEXTURECACHE::TextureCallbackHandler(
    EGxTexCommand cmd,
    unsigned int  w,
    unsigned int  h,
    unsigned int  mipLevel,
    unsigned int &texelStrideInBytes,
    const void  *&texels
) {
  ASSERT(w);
  ASSERT(h);

  if (cmd == GxTex_Unlock) {
    return;
  }

  if (!m_data) {
    unsigned int dataSize = 4 * w * h;
    m_data = ALLOC(dataSize);
    memset(m_data, 0, dataSize);
  }

  texelStrideInBytes = 4 * 256;
  texels = m_data;

  if (!m_theFace || !m_anyDirtyGlyphs) {
    return;
  }

  unsigned int glyphHeight = m_theFace->m_cellHeight;
  ASSERT(glyphHeight);

  unsigned int count = m_textureRows.Count();
  for (unsigned int row = 0; row < count; ++row) {
    ITERATELIST(CHARCODEDESC, m_textureRows[row].glyphList, current) {
      if (!current->bitmapData->m_dirty) {
        continue;
      }

      current->bitmapData->m_dirty = 0;
      ASSERT(current->bitmapData);
      ASSERT(current->bitmapData->m_data);
      ASSERT(current->ValidTextureCoords());
      ASSERT(current->ValidBlockEndPoints());

      GLYPHBITMAPDATA *glyphData = current->bitmapData;
      ASSERT(glyphData->m_yStart <= glyphHeight);
      unsigned long *dst = static_cast<unsigned long *>(m_data) + row * glyphHeight * 256 + current->glyphStartPixel;
      PasteGlyph(glyphData, dst, m_theFace->m_flags & 0x8);
    }
  }

  m_anyDirtyGlyphs = 0;
}

CHARCODEDESC *TEXTURECACHE::AllocateNewGlyph(GLYPHBITMAPDATA *data) {
  ASSERT(data);
  ASSERT(m_theFace);

  unsigned int count = m_textureRows.Count();
  for (unsigned int row = 0; row < count; ++row) {
    CHARCODEDESC *desc = m_textureRows[row].CreateNewDesc(data, row, m_theFace->m_cellHeight);
    if (desc) {
      m_anyDirtyGlyphs = 1;
      desc->rowNumber = row;
      return desc;
    }
  }

  return 0;
}

void IGXUTEXTBLOCK::Destroy() {
  for (unsigned int i = 0; i < m_lines.Count(); ++i) {
    if (m_lines[i]) {
      DEL(m_lines[i]);
    }
  }
}

void IGXUTEXTBLOCK::Recycle() {
  for (unsigned int i = 0; i < m_lines.Count(); ++i) {
    m_lines[i]->Recycle();
  }

  m_lines.SetCount(0);
}

IGXUTEXTLINE *IGXUTEXTBLOCK::NewLine() {
  IGXUTEXTLINE **line = m_lines.New();
  *line = IGXUTEXTLINE::NewGxuTextLine();
  return *line;
}

IGXUTEXTLINE *IGXUTEXTLINE::NewGxuTextLine() {
  IGXUTEXTLINE *line = g_freeTextLines.Head();

  if (line) {
    g_freeTextLines.UnlinkNode(line);
  } else {
    line = NEW(IGXUTEXTLINE);
  }

  return line;
}

void IGXUTEXTLINE::Destroy() {
  for (unsigned int i = 0; i < m_texturePages.Count(); ++i) {
    if (m_texturePages[i]) {
      DEL(m_texturePages[i]);
    }
  }
}

void IGXUTEXTLINE::Recycle() {
  for (unsigned int i = 0; i < m_texturePages.Count(); ++i) {
    m_texturePages[i]->Recycle();
  }

  m_texturePages.SetCount(0);
  g_freeTextLines.LinkNode(this, LIST_TAIL, 0);
}

void IGXUTEXTLINE::Reserve(unsigned int numTextLineTextures) {
  unsigned int i;

  for (i = 0; i < m_texturePages.Count(); ++i) {
    m_texturePages[i]->Recycle();
  }

  m_texturePages.SetCount(numTextLineTextures);
  for (i = 0; i < numTextLineTextures; ++i) {
    m_texturePages[i] = TEXTLINETEXTURE::NewTextLineTexture();
  }
}

void TEXTLINETEXTURE::Recycle() {
  m_vert.SetCount(0);
  m_colors.SetCount(0);
  m_vertIndices.SetCount(0);
  g_freeTextLineTextures.LinkNode(this, LIST_TAIL, 0);
}

TEXTLINETEXTURE::~TEXTLINETEXTURE() {
  m_vert.Clear();
  m_colors.Clear();
  m_vertIndices.Clear();
}

TEXTLINETEXTURE *TEXTLINETEXTURE::NewTextLineTexture() {
  TEXTLINETEXTURE *texture = g_freeTextLineTextures.Head();

  if (texture) {
    g_freeTextLineTextures.UnlinkNode(texture);
  } else {
    texture = NEW(TEXTLINETEXTURE);
  }

  return texture;
}

void BATCHEDRENDERFONTDESC::RenderBatch() {
  static NTempest::C3Vector normal(0.0f, 0.0f, -1.0f);
  float                     minz;
  float                     maxz;
  float                     pixWidth;
  float                     pixHeight;
  float                     minx;
  float                     maxx;
  float                     miny;
  float                     maxy;
  unsigned int              i;

  ASSERT(face);

  GxRsPush();
  GxRsSet(GxRs_Blend, GxBlend_Alpha);
  GxRsSet(GxRs_Lighting, 0);
  GxXformViewport(minx, maxx, miny, maxy, minz, maxz);

  pixWidth = static_cast<float>(GetScreenPixelWidth());
  pixHeight = static_cast<float>(GetScreenPixelHeight());

  for (i = 0; i < 8; ++i) {
    ITERATELIST(CGxString, m_strings, string) {
      int                 depth = (static_cast<signed char>(string->m_flags) < 0);
      STRINGVIEWMATRICES *matrices;

      GxRsSet(GxRs_DepthWrite, depth);
      GxRsSet(GxRs_DepthTest, depth);
      GxRsSet(GxRs_Fog, depth);

      matrices = s_stringViewMatrices.Ptr(reinterpret_cast<unsigned int>(string), HASHKEY_PTR(string));
      if (!matrices) {
        matrices = GetNewStringMatrix(string);
        string->BuildProjection(&matrices->projection, minx, maxx, miny, maxy, pixWidth, pixHeight);
        string->BuildView(&matrices->view, maxx - minx, maxy - miny);
      }

      GxXformSetProjection(matrices->projection);
      GxXformSetView(matrices->view);
      string->RenderTexture(false, i);
    }
  }

  GxRsPop();
}

BATCHEDRENDERFONTDESC::~BATCHEDRENDERFONTDESC() {
  ITERATELIST(CGxString, m_strings, string) {
    string->ClearStringMatrixEntry();
  }
}

void CGxStringBatch::RenderBatch() {
  NTempest::C44Matrix    oldView;
  NTempest::C44Matrix    oldProjection;

  GxVertexShaderSelect(GxVS_PassThru);
  GxXformProjection(oldProjection);
  GxXformView(oldView);

  ITERATELIST(BATCHEDRENDERFONTDESC, m_fontBatch, batchDesc) {
    batchDesc->RenderBatch();
  }

  GxXformSetProjection(oldProjection);
  GxXformSetView(oldView);
}

void CGxStringBatch::AddString(CGxString *string) {
  CGxFont *currentFace;

  ASSERT(string);
  currentFace = string->GetCurrentFace();
  ASSERT(currentFace);

  BATCHEDRENDERFONTDESC *batchDesc = m_fontBatch.Ptr(reinterpret_cast<unsigned int>(currentFace), HASHKEY_PTR(currentFace));
  if (!batchDesc) {
    batchDesc = m_fontBatch.New(reinterpret_cast<unsigned int>(currentFace), HASHKEY_PTR(currentFace), 0, 0);
    batchDesc->face = currentFace;
  } else {
    ASSERT(batchDesc->face == currentFace);
  }

  batchDesc->m_strings.LinkNode(string, LIST_TAIL, 0);
}

inline void CGxString::ClearStringMatrixEntry() {
  STRINGVIEWMATRICES *matrices = s_stringViewMatrices.Ptr(reinterpret_cast<unsigned int>(this), HASHKEY_PTR(this));

  if (matrices) {
    s_stringViewMatrices.Unlink(matrices);
    s_freeStringMatrices.LinkNode(matrices, LIST_TAIL, 0);
  }
}

void CGxString::AddHyperlinkParseInfo(GXUFONTHYPERLINKINFO currentParseInfo) {
  currentParseInfo.extent.t *= 1.0f / static_cast<float>(g_heightPixels);
  currentParseInfo.extent.b *= 1.0f / static_cast<float>(g_heightPixels);
  currentParseInfo.extent.l *= 1.0f / static_cast<float>(g_widthPixels);
  currentParseInfo.extent.r *= 1.0f / static_cast<float>(g_widthPixels);
  m_hyperlinkInfo.Add(1, &currentParseInfo);
}

void CGxString::CheckEvictedTextures() {
  if (m_textureEvicted) {
    ASSERT(m_currentFace);

    if (!m_currentFace->CheckStringGlyphs(m_text)) {
      CreateGeometry();
    }

    m_textureEvicted = 0;
  }
}

unsigned int CGxString::GetHyperLinkInfo(const GXUFONTHYPERLINKINFO *&list) const {
  unsigned int count = m_hyperlinkInfo.Count();

  if (!count || !g_widthPixels || !g_heightPixels) {
    return 0;
  }

  list = m_hyperlinkInfo.Ptr();
  return count;
}

int CGxString::SetGradient(int startCharacter, int length) {
  unsigned int flags;

  if (startCharacter < 0 || length < 0) {
    return 2;
  }

  if (startCharacter == m_lastGradientStart && length == m_lastGradientLength) {
    return 0;
  }

  flags = m_flags;
  m_lastGradientStart = startCharacter;
  m_lastGradientLength = length;

  if (flags & 0x1000) {
    ASSERT(m_colorGradients.Count());
  } else {
    m_flags = (flags & ~0x8U) | 0x1000;
    CreateGeometry();
  }

  if (m_colorGradientShadows.Count()) {
    SetGradient(startCharacter, length, m_colorGradientShadows, m_shadowColor.a);
  }

  return SetGradient(startCharacter, length, m_colorGradients, m_fontColor.a);
}

int CGxString::SetGradient(int startCharacter, int length, const TSGrowableArray<NTempest::CImVector *> &array, unsigned char alpha) {
  int verts = static_cast<int>(array.Count());
  int startIndex;
  int index;

  if (!verts) {
    return 2;
  }

  startIndex = 4 * startCharacter;
  index = min(verts, startIndex);

  while (index) {
    array[--index]->a = alpha;
  }

  if (startIndex >= verts) {
    return 2;
  }

  index = startIndex;
  if (length > 0) {
    int gradient;
    gradient = static_cast<int>(static_cast<float>(alpha) / length);

    while (1) {
      array[index++]->a = alpha;
      array[index++]->a = alpha;
      alpha = static_cast<unsigned char>(max(static_cast<int>(alpha) - gradient, 0));
      array[index++]->a = alpha;
      array[index++]->a = alpha;

      if (!alpha) {
        break;
      }

      if (index >= verts) {
        return 1;
      }
    }
  }

  if (index >= verts) {
    return 1;
  }

  while (index < verts) {
    array[index++]->a = 0;
  }

  return 0;
}

void IGxuStringInitialize() {
}

void IGxuStringShutdown() {
  s_freeStringMatrices.Clear();
}

void InternalGetTextExtent(CGxFont *face, const char *text, unsigned int numBytes, float height, float *extent, unsigned int flags) {
  float        width = 0.0f;
  float        lastWidth = 0.0f;
  float        maxWidth = 0.0f;
  unsigned int advance;
  unsigned int wide;
  unsigned int prevCode = 0;

  FATALASSERT(face);

  FATALASSERT(text);

  FATALASSERT(extent);

  if (height == 0.0f || (flags & 0x4)) {
    height = GxuFontGetOneToOneHeight(face);
  }

  while (*text && numBytes) {
    QUOTEDCODE   quoted = GxuDetermineQuotedCode(text, advance, 0, flags, wide, numBytes);

    text += advance;
    numBytes -= advance;

    if (quoted == CODE_COLORON || quoted == CODE_COLORRESTORE || quoted == CODE_HYPERLINKSTART || quoted == CODE_HYPERLINKSTOP) {
      continue;
    }

    if (quoted == CODE_NEWLINE || wide == 10) {
      float lineWidth = lastWidth + width;

      if (lineWidth >= maxWidth) {
        maxWidth = lineWidth;
      }
      width = 0.0f;
      lastWidth = 0.0f;
      continue;
    }

    {
      const CHARCODEDESC *codeDesc = face->NewCodeDesc(wide);

      if (codeDesc && codeDesc->bitmapData) {
        float step = 0.0f;

        ASSERT(codeDesc->dataValid);
        if (prevCode) {
          if (flags & 0x10) {
            step = face->ComputeStepFixedWidth(prevCode, wide);
          } else {
            step = face->ComputeStep(prevCode, wide);
          }
        }

        lastWidth = static_cast<float>(ceil(face->GetCharAdvance(wide)));
        prevCode = wide;
        width += step;
      }
    }
  }

  if (lastWidth + width >= maxWidth) {
    maxWidth = lastWidth + width;
  }

  {
    int   billboarded = flags & 0x80;
    float pixelScale = ScreenToPixelHeight(billboarded, height) / face->m_pixelSize;

    *extent = maxWidth * pixelScale;
    if (!billboarded) {
      *extent /= g_widthPixels;
    }
  }
}

unsigned int InternalGetMaxCharsWithinWidth(
    CGxFont      *face,
    const char   *text,
    float         height,
    float         maxWidth,
    unsigned int  lineBytes,
    float        *extent,
    unsigned int  flags,
    unsigned int *bytesInString,
    float        *widthArray,
    float        *widthArrayGuard
) {
  unsigned int numChars = 0;
  float        pixelWidth;
  float        pixelHeight;
  unsigned int pixWidth;
  float        pixelScale;
  float        width = 0.0f;
  float        lastWidth = 0.0f;
  unsigned int advance;
  unsigned int wide;
  unsigned int prevCode = 0;
  const char  *originalText;

  FATALASSERT(face);

  FATALASSERT(text);

  FATALASSERT(extent);

  if (!(flags & 0x80) && (height == 0.0f || (flags & 0x4))) {
    height = GxuFontGetOneToOneHeight(face);
  }

  pixelWidth = face->m_pixelSize * static_cast<float>(g_widthPixels) * maxWidth;
  pixelHeight = ScreenToPixelHeight(flags & 0x80, height);
  if (pixelHeight < 1.0f) {
    pixelHeight = 1.0f;
  }
  pixWidth = static_cast<unsigned int>(pixelWidth / pixelHeight + 1.0f);
  pixelScale = ScreenToPixelHeight(flags & 0x80, height) / face->m_pixelSize;
  originalText = text;

  while (*text && lineBytes) {
    QUOTEDCODE   quoted = GxuDetermineQuotedCode(text, advance, 0, flags, wide, lineBytes);

    text += advance;
    lineBytes -= advance;

    if (quoted == CODE_COLORON || quoted == CODE_COLORRESTORE || quoted == CODE_HYPERLINKSTART || quoted == CODE_HYPERLINKSTOP) {
      continue;
    }

    if (quoted == CODE_NEWLINE) {
      break;
    }

    {
      const CHARCODEDESC *codeDesc = face->NewCodeDesc(wide);

      if (!codeDesc || !codeDesc->bitmapData) {
        ++numChars;
        continue;
      }

      {
        float step = 0.0f;
        float charWidth;

        ASSERT(codeDesc->dataValid);
        if (prevCode) {
          if (flags & 0x10) {
            step = face->ComputeStepFixedWidth(prevCode, wide);
          } else {
            step = face->ComputeStep(prevCode, wide);
          }
        }

        charWidth = static_cast<float>(ceil(face->GetCharAdvance(wide)));
        if (static_cast<float>(pixWidth) < step + charWidth + width) {
          text -= advance;
          break;
        }

        width += step;
        prevCode = wide;
        ++numChars;
        lastWidth = charWidth;

        if (widthArray) {
          float currentWidth;

          ASSERT(widthArray < widthArrayGuard);
          currentWidth = width * pixelScale;
          if (!(flags & 0x80)) {
            currentWidth /= g_widthPixels;
          }
          *widthArray++ = currentWidth;
        }
      }
    }
  }

  *extent = (lastWidth + width) * pixelScale;
  if (!(flags & 0x80)) {
    *extent /= g_widthPixels;
  }

  if (bytesInString) {
    *bytesInString = static_cast<unsigned int>(text - originalText);
  }

  return numChars;
}

void GxuFontSetUseAdvanceWidth(int useAdvanceWidth) {
}

void CGxString::RenderTexture(bool initGxRenderStates, int texture) {
  if (initGxRenderStates) {
    GxRsPush();
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_Blend, GxBlend_Alpha);
  }

  unsigned int line = m_textBlock.m_lines.Count();
  while (line) {
    --line;
    RenderTexture(static_cast<int>(line), texture);
  }

  if (initGxRenderStates) {
    GxRsPop();
  }
}

void CGxString::RenderTexture(int line, int texture) {
  if (line >= static_cast<int>(m_textBlock.m_lines.Count())) {
    return;
  }

  IGXUTEXTLINE *textLine = m_textBlock.m_lines[line];
  if (!textLine || texture >= static_cast<int>(textLine->m_texturePages.Count())) {
    return;
  }

  textLine->m_texturePages[texture]->InternalRenderTexture(texture, m_currentFace, m_flags & 0x1, m_shadowColor, m_shadowOffset, m_fontColor);
}

void TEXTLINETEXTURE::InternalRenderTexture(
    int                        textureNum,
    CGxFont                   *face,
    bool                       showShadow,
    const NTempest::CImVector &shadowColor,
    const NTempest::C2Vector  &shadowOffset,
    const NTempest::CImVector &fontColor
) {
  ASSERT(face);

  CGxTex *texture = face->m_textureCache[textureNum].m_texture;
  ASSERT(texture);

  GxRsSet(GxRs_Texture0, texture);
  face->UpdateTextures();

  unsigned int numVerts = m_vert.Count();
  if (!numVerts) {
    return;
  }

  static NTempest::C3Vector  normal(0.0f, 0.0f, -1.0f);
  const NTempest::CImVector *shadowPointer;
  const NTempest::CImVector *colorPointer;
  int                        shadowStride;
  unsigned int               colorStride;

  if (m_shadowColors.Count()) {
    shadowPointer = m_shadowColors.Ptr();
    shadowStride = sizeof(NTempest::CImVector);
  } else {
    shadowPointer = &shadowColor;
    shadowStride = 0;
  }

  if (m_colors.Count()) {
    colorPointer = m_colors.Ptr();
    colorStride = sizeof(NTempest::CImVector);
  } else {
    colorPointer = &fontColor;
    colorStride = 0;
  }

  if (showShadow) {
    NTempest::C3Vector translate(shadowOffset.x, shadowOffset.y, 0.0f);
    translate.x = static_cast<float>(floor(ScreenToPixelWidth(0, translate.x)));
    translate.y = static_cast<float>(floor(ScreenToPixelHeight(0, translate.y)));

    NTempest::C44Matrix mat;
    mat.Translate(translate);
    GxXformPush(GxXform_World, mat);

    GxPrimLockVertexPtrs(
        m_vert.Count(), &m_vert[0].vc, sizeof(VERT), &normal, 0, shadowPointer, shadowStride, 0, 0, &m_vert[0].tc, sizeof(VERT), 0, 0
    );
    GxPrimDrawElements(GxPrim_Triangles, m_vertIndices.Count(), m_vertIndices.Ptr());
    GxPrimUnlockVertexPtrs();
    GxXformPop(GxXform_World);
  }

  GxPrimLockVertexPtrs(numVerts, &m_vert[0].vc, sizeof(VERT), &normal, 0, colorPointer, colorStride, 0, 0, &m_vert[0].tc, sizeof(VERT), 0, 0);
  GxPrimDrawElements(GxPrim_Triangles, m_vertIndices.Count(), m_vertIndices.Ptr());
  GxPrimUnlockVertexPtrs();
}

void CGxString::AddShadow(const NTempest::C2Vector &offset, const NTempest::CImVector &color) {
  m_shadowColor = color;
  m_shadowColor.a = min(color.a, m_fontColor.a);
  m_shadowOffset = offset;
  m_flags |= 0x1;

  if (m_flags & 0x1000) {
    CreateGeometry();
  } else {
    m_colorGradientShadows.SetCount(0);
  }
}

void CGxString::AddShadowFixedGeometry() {
  NTempest::C3Vector offset3;
  TEXTLINETEXTURE   *p;
  unsigned int       i;
  unsigned int       ti;
  IGXUTEXTLINE     **curr;

  offset3.x = static_cast<float>(floor(ScreenToPixelWidth(false, m_shadowOffset.x)));
  offset3.y = static_cast<float>(floor(ScreenToPixelHeight(false, m_shadowOffset.y)));
  offset3.z = 0.0f;

  curr = m_textBlock.m_lines.Ptr();
  for (i = 0; i < m_textBlock.m_lines.Count(); ++i, ++curr) {
    for (ti = 0; ti < (*curr)->m_texturePages.Count(); ++ti) {
      p = (*curr)->m_texturePages[ti];
      p->m_shadowColors.SetCount(p->m_vert.Count());

      for (unsigned int vi = 0; vi < p->m_vert.Count(); ++vi) {
        p->m_shadowColors[vi] = m_shadowColor;
      }
    }
  }
}

