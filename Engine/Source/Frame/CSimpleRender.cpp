#include "Frame/CSimpleRender.h"

#include "Base/Coordinate.h"
#include "Base/Handle.h"
#include "Base/Status.h"
#include "Frame/CSimpleFrame.h"
#include "Frame/SimpleFrameRegistry.h"
#include "FrameXML/LoadXML.h"
#include "FrameXML/XMLTree.h"
#include "Gxu/IGxuFont.h"
#include "Services/SysMessage.h"
#include "Services/TextBlock.h"
#include "Services/Texture.h"

#include <ctype.h>
#include <stdlib.h>
#include <storm.h>

static int __cdecl            SortByTexture(const void *A, const void *B);
static const char *LanguageProcess(const char *text);
static const char *LanguageRule1(const char *text);
static bool CheckJongsung(const unsigned short *text, int position);
void TextureGetDimensions(HTEXTURE texture, unsigned int *width, unsigned int *height);
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
void TextBlockUpdateColor(HTEXTBLOCK htb, const NTempest::CImVector &textColor);

static char           output8[0x1000];
static unsigned short output16[0x1000];

static const unsigned int NumericJongsung[10] = {1, 1, 0, 1, 0, 0, 1, 1, 1, 0};
static const unsigned int AlphabeticJongsung[26] = {0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0};

NTempest::C3Vector CSimpleRender::s_normal(0.0f, 0.0f, 1.0f);
unsigned short     CSimpleRender::s_indices[4] = {0, 1, 2, 3};

EGxTexFilter CSimpleTexture::s_textureFilterMode = GxTex_Linear;

void CSimpleRender::DrawBatch(CRenderBatch *batch) {
  unsigned int count = batch->m_texturelist.Count();

  if (count) {
    unsigned long texture = static_cast<unsigned long>(-1);
    unsigned int  i;

    GxVertexShaderSelect(GxVS_PassThru);
    GxRsPush();
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_Fog, 0);
    GxRsSet(GxRs_DepthTest, 0);
    GxRsSet(GxRs_DepthWrite, 0);

    for (i = 0; i < count; ++i) {
      CSimpleBatchedTexture &batched = batch->m_texturelist[i];

      if (texture != batched.textureID) {
        texture = batched.textureID;
        GxRsSet(GxRs_Texture0, (void *)texture);
      }

      if (batched.GxColor && batched.alphamode < GxBlend_Alpha) {
        GxRsSet(GxRs_Blend, GxBlend_Alpha);
      } else if (batched.alphamode != GxBlends_Last) {
        GxRsSet(GxRs_Blend, batched.alphamode);
      }

      GxPrimLockVertexPtrs(4, batched.position, 12, &s_normal, 0, batched.GxColor, 0, 0, 0, batched.texCoord, 8, 0, 0);
      GxPrimDrawElements(GxPrim_TriangleStrip, 4, s_indices);
      GxPrimUnlockVertexPtrs();
    }

    GxRsPop();
  }

  if (batch->m_stringbatch) {
    GxuFontRenderBatch(batch->m_stringbatch);
  }

  ITERATELIST(RENDERCALLBACKNODE, batch->m_callbacks, node) {
    node->callback(node->param);
  }
}

CSimpleTexture::CSimpleTexture(CSimpleFrame *frame, unsigned int drawlayer, int show)
    : CSimpleRegion(frame, drawlayer, show),
      m_name(0),
      m_registryContext(0),
      m_texture(0),
      m_alphamode(GxBlend_Alpha),
      m_TexCoordModifiesPosition(0) {
  NTempest::CRect rect;

  rect.l = 0.0f;
  rect.r = 1.0f;
  rect.t = 0.0f;
  rect.b = 1.0f;
  SetTexCoord(rect);
}

CSimpleTexture::~CSimpleTexture() {
  SetTexture(0, 0);
  ClearFromSimpleRegistry();
}

void CSimpleTexture::SetBlendMode(EGxBlend mode) {
  m_alphamode = mode;
  OnRegionChanged();
}

void CSimpleTexture::TexCorrectRect(NTempest::CRect &rect) {
  using namespace NTempest;

  const float y = rect.t;
  const float h = rect.b - rect.t;
  const float x = rect.l;
  const float w = rect.r - rect.l;

  ASSERT(CMath::fequal_(m_texCoord[0].x, m_texCoord[1].x));
  ASSERT(CMath::fequal_(m_texCoord[2].x, m_texCoord[3].x));
  ASSERT(CMath::fequal_(m_texCoord[0].y, m_texCoord[2].y));
  ASSERT(CMath::fequal_(m_texCoord[1].y, m_texCoord[3].y));

  rect.l = w * m_texCoord[0].x + x;
  rect.r = w * m_texCoord[2].x + x;
  rect.t = h * m_texCoord[0].y + y;
  rect.b = h * m_texCoord[1].y + y;

  ASSERT(rect.minx <= rect.maxx);
  ASSERT(rect.miny <= rect.maxy);
}

void CSimpleTexture::SetPosition(const NTempest::CRect &rect) {
  m_position[0].z = 0.0f;
  m_position[0].x = rect.l;
  m_position[0].y = rect.b;
  m_position[1].z = 0.0f;
  m_position[1].x = rect.l;
  m_position[1].y = rect.t;
  m_position[2].z = 0.0f;
  m_position[2].x = rect.r;
  m_position[2].y = rect.b;
  m_position[3].z = 0.0f;
  m_position[3].x = rect.r;
  m_position[3].y = rect.t;
}

CGxTex *CSimpleTexture::GetTexture() {
  if (m_texture) {
    return TextureGetGxTex(m_texture, 1, 0);
  }

  return 0;
}

void CSimpleFontStringAttributes::CopyFlags(const CSimpleFontStringAttributes &rhs) {
  m_flags = rhs.m_flags;
}

int CSimpleTexture::SetTexture(const char *file, int uvWrapping) {
  if (m_texture) {
    HandleClose(m_texture);
    m_texture = 0;
  }

  if (file) {
    CStatus     status;
    CGxTexFlags flags(s_textureFilterMode, uvWrapping, uvWrapping, 0, 0, 0, 1);

    if (*file) {
      m_texture = TextureCreate(file, flags, &status, 0);
      SysMsgAdd(status, 4);
    }
  }

  OnRegionChanged();
  return 1;
}

int CSimpleTexture::SetTexture(HTEXTURE__ *texHandle) {
  int result = 1;

  if (m_texture) {
    HandleClose(m_texture);
    m_texture = 0;
  }

  if (texHandle) {
    m_texture = static_cast<HTEXTURE>(HandleDuplicate(texHandle));
  }

  if (!m_texture) {
    m_texture = TextureCreateSolid(NTempest::CImVector(0xFF00FF00), 0);
    result = 0;
  }

  OnRegionChanged();
  return result;
}

int CSimpleTexture::SetTexture(const NTempest::CImVector &color) {
  int result = 1;

  if (m_texture) {
    HandleClose(m_texture);
    m_texture = 0;
  }

  m_texture = TextureCreateSolid(color, 0);
  ASSERT(m_texture);
  if (!m_texture) {
    result = 0;
  }

  OnRegionChanged();
  return result;
}

CSimpleFontString::CSimpleFontString(CSimpleFrame *frame, unsigned int drawlayer, int show)
    : CSimpleRegion(frame, drawlayer, show),
      m_name(0),
      m_registryContext(0),
      m_font(0),
      m_textMaxSize(0),
      m_textCurSize(0),
      m_text(0),
      m_spacing(0.0f),
      m_string(0),
      m_cachedWidth(0.0f),
      m_cachedHeight(0.0f),
      m_shadowColor(0xFF000000ul),
      m_shadowOffset(0.0f),
      m_justificationOffset(0.0f),
      m_alphaGradientStart(-1),
      m_styleFlags(0x292) {
}

void CSimpleTexture::SetTexCoord(const NTempest::CRect &rect) {
  m_texCoord[0].x = rect.l;
  m_texCoord[0].y = rect.t;
  m_texCoord[1].x = rect.l;
  m_texCoord[1].y = rect.b;
  m_texCoord[2].x = rect.r;
  m_texCoord[2].y = rect.t;
  m_texCoord[3].x = rect.r;
  m_texCoord[3].y = rect.b;

  if (m_TexCoordModifiesPosition) {
    NTempest::CRect texRect;

    if (GetRect(&texRect)) {
      TexCorrectRect(texRect);
      SetPosition(texRect);
    }
  }
}

void CSimpleTexture::SetTexCoord(const NTempest::C2Vector *texCoord) {
  m_texCoord[0] = texCoord[0];
  m_texCoord[1] = texCoord[1];
  m_texCoord[2] = texCoord[2];
  m_texCoord[3] = texCoord[3];

  if (m_TexCoordModifiesPosition) {
    NTempest::CRect texRect;

    if (GetRect(&texRect)) {
      TexCorrectRect(texRect);
      SetPosition(texRect);
    }
  }
}

CLayoutFrame *CSimpleTexture::GetLayoutFrameByName(const char *name) {
  char newName[1024];

  if (!SStrCmpI(name, "$parent", SStrLen("$parent"))) {
    CSimpleFrame *frame;

    SStrCopy(newName, "Top", 0x7FFFFFFF);
    for (frame = m_frame; frame; frame = frame->m_parent) {
      const char *frameName = frame->GetName();

      if (frameName && *frameName) {
        SStrCopy(newName, frameName, sizeof(newName));
        break;
      }
    }

    SStrPack(newName, name + SStrLen("$parent"), sizeof(newName));
  } else {
    SStrCopy(newName, name, sizeof(newName));
  }

  return CLayoutFrame::GetLayoutFrameByName(newName);
}

void CSimpleTexture::PreLoadXML(const XMLNode *node, CStatus *status) {
  const char *textureName = node->GetAttributeByName("name");

  if (textureName && *textureName) {
    char name[1024];

    if (!SStrCmpI(textureName, "$parent", SStrLen("$parent"))) {
      CSimpleFrame *frame;

      SStrCopy(name, "Top", 0x7FFFFFFF);
      for (frame = m_frame; frame; frame = frame->m_parent) {
        const char *frameName = frame->GetName();

        if (frameName && *frameName) {
          SStrCopy(name, frameName, sizeof(name));
          break;
        }
      }

      SStrPack(name, textureName + SStrLen("$parent"), sizeof(name));
    } else {
      SStrCopy(name, textureName, sizeof(name));
    }

    if (!AddToRegistry(name, 0)) {
      status->Add(STATUS_WARNING, "Texture named '%s' already registered", name);
    }
  }
}

void CSimpleTexture::LoadXML(const XMLNode *node, CStatus *status) {
  const char    *value;
  const XMLNode *child;
  int            uvWrapping = 0;

  CLayoutFrame::LoadXML(node, status);

  value = node->GetAttributeByName("hidden");
  if (value && *value) {
    if (StringToBOOL(value)) {
      Hide();
    } else {
      Show();
    }
  }

  for (child = node->GetChild(); child; child = child->GetSibling()) {
    if (!SStrCmpI(child->GetName(), "TexCoords", 0x7FFFFFFF)) {
      NTempest::C2Vector coords[4];
      float              l = 0.0f;
      float              r = 1.0f;
      float              t = 0.0f;
      float              b = 1.0f;

      value = child->GetAttributeByName("left");
      if (value && *value) {
        l = SStrToFloat(value);
      }

      value = child->GetAttributeByName("right");
      if (value && *value) {
        r = SStrToFloat(value);
      }

      value = child->GetAttributeByName("top");
      if (value && *value) {
        t = SStrToFloat(value);
      }

      value = child->GetAttributeByName("bottom");
      if (value && *value) {
        b = SStrToFloat(value);
      }

      coords[0].x = l;
      coords[0].y = t;
      coords[1].x = l;
      coords[1].y = b;
      coords[2].x = r;
      coords[2].y = t;
      coords[3].x = r;
      coords[3].y = b;
      SetTexCoord(coords);

      if (l < 0.0f || l > 1.0f || r < 0.0f || r > 1.0f || t < 0.0f || t > 1.0f || b < 0.0f || b > 1.0f) {
        uvWrapping = 1;
      }
    } else if (!SStrCmpI(child->GetName(), "Color", 0x7FFFFFFF)) {
      NTempest::CImVector color;

      LoadXML_Color(child, color, status);
      SetTexture(color);
    }
  }

  value = node->GetAttributeByName("file");
  if (value && *value) {
    SetTexture(value, uvWrapping);
  }

  value = node->GetAttributeByName("alphaMode");
  if (value && *value) {
    EGxBlend mode;

    if (StringToBlendMode(value, mode)) {
      SetBlendMode(mode);
    }
  }
}

void CSimpleTexture::PostLoadXML(const XMLNode *node, CStatus *status) {
  if (m_frame) {
    unsigned int count = m_points.Count();

    while (count) {
      if (m_points[--count]) {
        return;
      }
    }

    SetAllPoints(m_frame, 1);
  }
}

int CSimpleTexture::AddToRegistry(const char *name, unsigned int context) {
  if (m_name) {
    UnregisterScriptObject(m_name);
    SimpleTextureRegistryRemoveEntry(m_name, m_registryContext);
    FREE(m_name);
    m_name = 0;
  }

  if (!name || !*name) {
    return 0;
  }

  if (!SimpleTextureRegistryAddEntry(name, this, context)) {
    return 0;
  }

  m_name = SStrDupA(name, __FILE__, __LINE__);
  m_registryContext = context;
  RegisterScriptObject(m_name);
  return 1;
}

int CSimpleFontString::SetFont(const char *font, float fontHeight, unsigned int fontFlags) {
  int okay = 1;

  m_fontHeight = fontHeight;

  if (m_string) {
    HandleClose(m_string);
    m_string = 0;
  }

  if (m_font) {
    HandleClose(m_font);
    m_font = 0;
  }

  m_cachedWidth = 0.0f;
  m_cachedHeight = 0.0f;
  OnRegionChanged();

  if (font && fontHeight != 0.0f) {
    m_font = TextBlockGenerateFont(font, fontFlags, m_fontHeight * m_layoutScale);
    if (!m_font) {
      okay = 0;
    } else if (m_styleFlags & 0x200) {
      float height = GxuFontGetOneToOneHeight(TextBlockGetFontPtr(m_font)) / m_layoutScale;
      NDCToDDC(0.0f, height, 0, &m_fontHeight);
    }
  }

  if (m_font && m_text && *m_text) {
    UpdateString(0);
  }

  return okay;
}

void CSimpleFontString::SetTextHeight(float height) {
  static const float EPSILON = 2.38418579e-7f;

  ASSERT(height > 0.0f);

  if (fabs(height - m_fontHeight) >= EPSILON) {
    m_styleFlags &= ~0x200;
    m_fontHeight = height;
    m_cachedWidth = 0.0f;
    m_cachedHeight = 0.0f;

    if (m_string) {
      HandleClose(m_string);
      m_string = 0;
    }

    Resize(0);
  }
}

void CSimpleFontString::SetTextLength(int size) {
  if (size != m_textMaxSize) {
    if (size) {
      ASSERT(size > 0);

      char *text = static_cast<char *>(ALLOC(size + 1));
      *text = 0;
      ASSERT(!m_text || !*m_text);
      FREEIFUSED(m_text);
      m_text = text;
      m_textCurSize = size;
    }

    m_textMaxSize = size;
  }
}

static bool CheckJongsung(const unsigned short *text, int position) {
  while (position >= 0) {
    unsigned short character = text[position];

    if (character & 0xFF00) {
      if (character >= 0xAC00 && character <= 0xD7A3) {
        return (character - 0xAC00) % 28 != 0;
      }
    } else if (isdigit(character)) {
      return NumericJongsung[character - '0'] != 0;
    } else if (isalpha(character)) {
      return AlphabeticJongsung[tolower(character) - 'a'] != 0;
    }

    --position;
  }

  return false;
}

const CSimpleFontStringAttributes &CSimpleFontStringAttributes::operator=(const CSimpleFontString &rhs) {
  unsigned int        fontFlags = rhs.m_font ? TextBlockGetFontFlags(rhs.m_font) : 0;
  float               fontHeight = rhs.m_fontHeight;
  const char         *fontName = rhs.m_font ? TextBlockGetFontName(rhs.m_font) : 0;
  NTempest::CImVector color(0ul);
  NTempest::CImVector shadowColor;
  NTempest::C2Vector  shadowOffset(0.0f);

  m_font = fontName;
  m_fontHeight = fontHeight;
  m_fontFlags = fontFlags;
  m_flags |= FLAG_FONT_UPDATE;

  m_styleFlags = rhs.m_styleFlags;
  m_flags |= FLAG_STYLE_UPDATE;

  rhs.GetVertexColor(color);
  m_color = color;
  m_flags |= FLAG_COLOR_UPDATE;

  shadowColor = color;
  if (rhs.m_styleFlags & 0x100) {
    shadowColor = rhs.m_shadowColor;
    shadowOffset = rhs.m_shadowOffset;
  }
  m_shadowColor = shadowColor;
  m_shadowOffset = shadowOffset;
  m_flags |= FLAG_SHADOW_UPDATE;

  m_spacing = rhs.m_spacing;
  m_flags |= FLAG_SPACING_UPDATE;
  return *this;
}

const CSimpleFontStringAttributes &CSimpleFontStringAttributes::operator=(const CSimpleFontStringAttributes &rhs) {
  const char *font = m_font;
  const char *attribFont = rhs.m_font;

  if (font != attribFont || m_fontHeight != rhs.m_fontHeight || m_fontFlags != rhs.m_fontFlags) {
    m_font = attribFont;
    m_fontHeight = rhs.m_fontHeight;
    m_fontFlags = rhs.m_fontFlags;
    m_flags |= FLAG_FONT_UPDATE;
  }

  if (m_styleFlags != rhs.m_styleFlags) {
    m_styleFlags = rhs.m_styleFlags;
    m_flags |= FLAG_STYLE_UPDATE;
  }

  if (*reinterpret_cast<unsigned long *>(&m_color) != *reinterpret_cast<const unsigned long *>(&rhs.m_color)) {
    m_color = rhs.m_color;
    m_flags |= FLAG_COLOR_UPDATE;
  }

  if (*reinterpret_cast<unsigned long *>(&m_shadowColor) != *reinterpret_cast<const unsigned long *>(&rhs.m_shadowColor) ||
      m_shadowOffset.x != rhs.m_shadowOffset.x || m_shadowOffset.y != rhs.m_shadowOffset.y)
  {
    m_shadowColor = rhs.m_shadowColor;
    m_shadowOffset = rhs.m_shadowOffset;
    m_flags |= FLAG_SHADOW_UPDATE;
  }

  if (m_spacing != rhs.m_spacing) {
    m_spacing = rhs.m_spacing;
    m_flags |= FLAG_SPACING_UPDATE;
  }

  return *this;
}

static const char *LanguageRule1(const char *text) {
  unsigned short *readpos;
  unsigned short *writepos;

  SUniConvertUTF8to16(output16, 0x1000, text, 0x7FFFFFFF, 0, 0);

  readpos = output16;
  writepos = output16;

  while (*readpos) {
    if (*readpos == '|') {
      ++readpos;

      if (*readpos == '1' && (readpos[1] < '0' || readpos[1] > '9')) {
        unsigned short *jongsungText = readpos + 1;
        unsigned short *nonJongsungText = jongsungText;
        unsigned short *end;
        unsigned short *selectedText;

        while (*nonJongsungText && *nonJongsungText++ != ';') {
        }

        end = nonJongsungText;
        while (*end && *end++ != ';') {
        }

        readpos = end - 1;
        selectedText = CheckJongsung(output16, static_cast<int>(writepos - output16) - 1) ? jongsungText : nonJongsungText;

        while (*selectedText && *selectedText != ';') {
          *writepos++ = *selectedText++;
        }

        ++readpos;
        continue;
      }

      *writepos++ = '|';
    }

    *writepos++ = *readpos++;
  }

  *writepos = 0;
  SUniConvertUTF16to8(output8, 0x1000, output16, 0x7FFFFFFF, 0, 0);
  return output8;
}

static const char *LanguageProcess(const char *text) {
  while (*text) {
    const char *rule = text;

    while (*rule) {
      if (*rule == '|') {
        ++rule;

        if (*rule != '|' && isdigit(*rule)) {
          break;
        }
      }

      ++rule;
    }

    if (!*rule) {
      return text;
    }

    if (SStrToUnsigned(rule) == 1) {
      text = LanguageRule1(text);
    }
  }

  return text;
}

CSimpleFontString::~CSimpleFontString() {
  FREEIFUSED(m_text);
  SetFont(0, 0.0f, 0);
  ClearFromSimpleRegistry();
}

CLayoutFrame *CSimpleFontString::GetLayoutFrameByName(const char *name) {
  char newName[1024];

  if (!SStrCmpI(name, "$parent", SStrLen("$parent"))) {
    CSimpleFrame *frame;

    SStrCopy(newName, "Top", 0x7FFFFFFF);
    for (frame = m_frame; frame; frame = frame->m_parent) {
      const char *frameName = frame->GetName();

      if (frameName && *frameName) {
        SStrCopy(newName, frameName, sizeof(newName));
        break;
      }
    }

    SStrPack(newName, name + SStrLen("$parent"), sizeof(newName));
  } else {
    SStrCopy(newName, name, sizeof(newName));
  }

  return CLayoutFrame::GetLayoutFrameByName(newName);
}

void CSimpleFontString::PreLoadXML(const XMLNode *node, CStatus *status) {
  const char *fontStringName = node->GetAttributeByName("name");

  if (fontStringName && *fontStringName) {
    char name[1024];

    if (!SStrCmpI(fontStringName, "$parent", SStrLen("$parent"))) {
      CSimpleFrame *frame;

      SStrCopy(name, "Top", 0x7FFFFFFF);
      for (frame = m_frame; frame; frame = frame->m_parent) {
        const char *frameName = frame->GetName();

        if (frameName && *frameName) {
          SStrCopy(name, frameName, sizeof(name));
          break;
        }
      }

      SStrPack(name, fontStringName + SStrLen("$parent"), sizeof(name));
    } else {
      SStrCopy(name, fontStringName, sizeof(name));
    }

    if (!AddToRegistry(name, 0)) {
      status->Add(STATUS_WARNING, "FontString named '%s' already registered", name);
    }
  }
}

void CSimpleFontString::LoadXML(const XMLNode *node, CStatus *status) {
  const char    *value;
  const XMLNode *child;

  CLayoutFrame::LoadXML(node, status);

  value = node->GetAttributeByName("hidden");
  if (value && *value) {
    if (StringToBOOL(value)) {
      Hide();
    } else {
      Show();
    }
  }

  value = node->GetAttributeByName("font");
  if (value && *value) {
    const char  *font = value;
    float        fontHeight = 0.0f;
    unsigned int fontFlags = 0;

    child = node->GetChildByName("FontHeight");
    if (child) {
      LoadXML_Value(child, fontHeight, status);
    }

    if (fontHeight <= 0.0f) {
      status->Add(STATUS_WARNING, "Missing font height in %s element", node->GetName());
      return;
    }

    value = node->GetAttributeByName("outline");
    if (value && *value) {
      if (!SStrCmpI(value, "NORMAL", 0x7FFFFFFF)) {
        fontFlags = 1;
      } else if (!SStrCmpI(value, "THICK", 0x7FFFFFFF)) {
        fontFlags = 5;
      }
    }

    value = node->GetAttributeByName("monochrome");
    if (value && *value) {
      fontFlags |= 8;
    }

    SetFont(font, fontHeight, fontFlags);
  }

  value = node->GetAttributeByName("spacing");
  if (value && *value) {
    SetSpacing(SStrToFloat(value) * 0.0009765625f * 0.8f);
  }

  value = node->GetAttributeByName("bytes");
  if (value && *value) {
    SetTextLength(SStrToInt(value));
  }

  value = node->GetAttributeByName("text");
  if (value && *value) {
    const char *text = FrameScript_GetText(value, -1, GENDER_NOT_APPLICABLE);

    if (!text || !*text) {
      text = value;
    }

    SetText(text);
  }

  value = node->GetAttributeByName("justifyV");
  if (value && *value) {
    unsigned int justify;

    if (StringToJustify(value, justify)) {
      ChangeStyleFlags(0x38, justify);
    }
  }

  value = node->GetAttributeByName("justifyH");
  if (value && *value) {
    unsigned int justify;

    if (StringToJustify(value, justify)) {
      ChangeStyleFlags(0x7, justify);
    }
  }

  value = node->GetAttributeByName("wraponspaces");
  if (value && *value) {
    ChangeStyleFlags(0x1000, StringToBOOL(value) ? 0x1000 : 0);
  }

  for (child = node->GetChild(); child; child = child->GetSibling()) {
    if (!SStrCmpI(child->GetName(), "Color", 0x7FFFFFFF)) {
      NTempest::CImVector color;

      LoadXML_Color(child, color, status);
      SetVertexColor(color);
    } else if (!SStrCmpI(child->GetName(), "Shadow", 0x7FFFFFFF)) {
      NTempest::CImVector color(0xFF000000ul);
      float               x = 0.001f;
      float               y = -0.001f;
      NTempest::C2Vector  offset;
      const XMLNode      *shadowChild = child->GetChildByName("Color");

      if (shadowChild) {
        LoadXML_Color(shadowChild, color, status);
      }

      shadowChild = child->GetChildByName("Offset");
      if (shadowChild) {
        LoadXML_Dimensions(shadowChild, x, y, status);
      }

      offset.x = x;
      offset.y = y;
      AddShadow(color, offset);
    }
  }
}

void CSimpleFontString::PostLoadXML(const XMLNode *node, CStatus *status) {
  if (m_frame) {
    unsigned int count = m_points.Count();

    while (count) {
      if (m_points[--count]) {
        return;
      }
    }

    if (m_styleFlags & 0x1) {
      SetPoint(FRAMEPOINT_LEFT, m_frame, FRAMEPOINT_LEFT, 0.0f, 0.0f, 1);
    } else if (m_styleFlags & 0x4) {
      SetPoint(FRAMEPOINT_RIGHT, m_frame, FRAMEPOINT_RIGHT, 0.0f, 0.0f, 1);
    } else {
      SetPoint(FRAMEPOINT_CENTER, m_frame, FRAMEPOINT_CENTER, 0.0f, 0.0f, 1);
    }
  }
}

int CSimpleFontString::AddToRegistry(const char *name, unsigned int context) {
  if (m_name) {
    UnregisterScriptObject(m_name);
    SimpleFontStringRegistryRemoveEntry(m_name, m_registryContext);
    FREE(m_name);
    m_name = 0;
  }

  if (!name || !*name) {
    return 0;
  }

  if (!SimpleFontStringRegistryAddEntry(name, this, context)) {
    return 0;
  }

  m_name = SStrDupA(name, __FILE__, __LINE__);
  m_registryContext = context;
  RegisterScriptObject(m_name);
  return 1;
}

void CSimpleFontString::SetText(const char *text) {
  ASSERT(m_font);

  if (m_text) {
    if (text && !SStrCmp(text, m_text, 0x7FFFFFFF)) {
      return;
    }

    *m_text = 0;
  }

  m_cachedWidth = 0.0f;
  m_cachedHeight = 0.0f;

  if (text && *text) {
    const char *processedText = LanguageProcess(text);

    if (m_textMaxSize) {
      SStrCopy(m_text, processedText, m_textMaxSize);
    } else {
      int textLength = SStrLen(processedText);

      if (textLength <= m_textCurSize) {
        SStrCopy(m_text, processedText, 0x7FFFFFFF);
      } else {
        FREEIFUSED(m_text);
        m_text = SStrDupA(processedText, __FILE__, __LINE__);
        m_textCurSize = textLength;
      }
    }
  }

  if (m_string) {
    HandleClose(m_string);
    m_string = 0;
  }

  Resize(0);
}

void CSimpleFontString::SetJustificationOffset(float x, float y) {
  if (x != m_justificationOffset.x || y != m_justificationOffset.y) {
    m_justificationOffset.x = x;
    m_justificationOffset.y = y;

    if (m_string) {
      NTempest::C3Vector position(m_rect.l + m_justificationOffset.x * m_layoutScale, m_rect.t + m_justificationOffset.y * m_layoutScale, 0.0f);
      TextBlockAnimate(m_string, position);
    }
  }
}

void CSimpleFontString::OnGxColorChanged() {
  CSimpleRegion::OnGxColorChanged();

  if (m_string) {
    TextBlockUpdateColor(m_string, m_color);

    if (m_styleFlags & 0x100) {
      TextBlockAddShadow(m_string, m_shadowColor, m_shadowOffset);
    }
  }
}

void CSimpleFontString::SetSpacing(float spacing) {
  if (spacing != m_spacing) {
    m_spacing = spacing;
    m_cachedHeight = 0.0f;

    if (m_string) {
      UpdateString(0);
    }
  }
}

void CSimpleFontString::AddShadow(const NTempest::CImVector &color, const NTempest::C2Vector &offset) {
  m_styleFlags |= 0x100;
  m_shadowColor = color;
  m_shadowOffset = offset;

  if (m_string) {
    UpdateString(0);
  }
}

void CSimpleFontString::RemoveShadow() {
  m_styleFlags &= ~0x100U;

  if (m_string) {
    UpdateString(0);
  }
}

bool CSimpleFontString::SetAlphaGradient(int startChar, int length) {
  m_alphaGradientStart = startChar;
  m_alphaGradientLength = length;

  return !m_string || TextBlockSetGradient(m_string, startChar, length) < 2;
}

void CSimpleFontStringAttributes::UpdateString(CSimpleFontString *string, int force) {
  FATALASSERT(string);

  if (force) {
    m_flags |= FLAG_COMPLETE_UPDATE;
  }

  if (m_flags & FLAG_FONT_UPDATE) {
    string->SetFont(m_font, m_fontHeight, m_fontFlags);
    m_flags &= ~FLAG_FONT_UPDATE;
  }

  if (m_flags & FLAG_STYLE_UPDATE) {
    if (string->m_styleFlags != m_styleFlags) {
      string->m_styleFlags = m_styleFlags;
      if (string->m_string) {
        string->UpdateString(0);
      }
    }
    m_flags &= ~FLAG_STYLE_UPDATE;
  }

  if (m_flags & FLAG_COLOR_UPDATE) {
    string->SetVertexColor(m_color);
    m_flags &= ~FLAG_COLOR_UPDATE;
  }

  if (m_flags & FLAG_SHADOW_UPDATE) {
    if (m_shadowOffset.x == 0.0f && m_shadowOffset.y == 0.0f) {
      string->RemoveShadow();
    } else {
      string->AddShadow(m_shadowColor, m_shadowOffset);
    }
    m_flags &= ~FLAG_SHADOW_UPDATE;
  }

  if (m_flags & FLAG_SPACING_UPDATE) {
    string->SetSpacing(m_spacing);
    m_flags &= ~FLAG_SPACING_UPDATE;
  }
}

float CSimpleFontString::GetStringWidth() {
  if (m_cachedWidth == 0.0f && m_text && *m_text) {
    TextBlockGetTextExtent(m_font, m_text, SStrLen(m_text), m_fontHeight * m_layoutScale, &m_cachedWidth, 0.0f, m_styleFlags);
    m_cachedWidth /= m_layoutScale;
  }

  return m_cachedWidth;
}

float CSimpleTexture::GetWidth() {
  float width = CLayoutFrame::GetWidth();

  if (width == 0.0f && m_texture) {
    unsigned int pixels;

    TextureGetDimensions(m_texture, &pixels, 0);
    width = pixels * 0.0009765625f * 0.8f;
  }

  return width;
}

float CSimpleFontString::GetStringHeight() {
  if (m_cachedHeight == 0.0f && m_text && *m_text) {
    m_cachedHeight =
        TextBlockGetWrappedTextHeight(m_font, m_text, m_fontHeight * m_layoutScale, GetWidth() * m_layoutScale, m_spacing, m_styleFlags) /
        m_layoutScale;
  }

  return m_cachedHeight;
}

float CSimpleTexture::GetHeight() {
  float height = CLayoutFrame::GetHeight();

  if (height == 0.0f && m_texture) {
    unsigned int pixels;

    TextureGetDimensions(m_texture, 0, &pixels);
    height = pixels * 0.0009765625f * 0.8f;
  }

  return height;
}

float CSimpleFontString::GetTextWidth(const char *text, unsigned int textBytes) {
  float width;

  ASSERT(m_font);

  if (!textBytes) {
    textBytes = SStrLen(text);
  }

  TextBlockGetTextExtent(m_font, text, textBytes, m_fontHeight * m_layoutScale, &width, 0.0f, m_styleFlags);
  return width / m_layoutScale;
}

void CSimpleFontString::UpdateString(const NTempest::CRect *rect) {
  if (!rect) {
    if (!(m_flags & 0x1)) {
      return;
    }

    rect = &m_rect;
  }

  if (m_string) {
    HandleClose(m_string);
    m_string = 0;
  }

  if (m_text && *m_text) {
    unsigned int       styleFlags = m_styleFlags;
    NTempest::C3Vector position(rect->l + m_justificationOffset.x * m_layoutScale, rect->t + m_justificationOffset.y * m_layoutScale, 0.0f);

    if (!(styleFlags & 0x400)) {
      const char *scan = m_text;

      while (*scan) {
        if (*scan == '|') {
          if (scan[1] == '|') {
            ++scan;
          } else if (scan[1] == 'C' || scan[1] == 'c') {
            break;
          }
        }

        ++scan;
      }

      if (!*scan) {
        styleFlags |= 0x400;
      }
    }

    m_string = TextBlockCreate(
        m_font, m_text, m_color, position, m_fontHeight * m_layoutScale, rect->r - rect->l, rect->b - rect->t, styleFlags, 0.0f, m_spacing
    );
    ASSERT(m_string);

    if (m_styleFlags & 0x100) {
      NTempest::CImVector shadowColor(m_shadowColor);
      shadowColor.a = static_cast<unsigned char>(m_shadowColor.a * m_frame->GetAlpha() / 255);
      NTempest::C2Vector shadowOffset(m_shadowOffset.x * m_layoutScale, m_shadowOffset.y * m_layoutScale);
      TextBlockAddShadow(m_string, shadowColor, shadowOffset);
    }

    if (m_alphaGradientStart >= 0) {
      TextBlockSetGradient(m_string, m_alphaGradientStart, m_alphaGradientLength);
    }
  }

  OnRegionChanged();
}

unsigned int CSimpleFontString::WrapText(const char *text, float maxWidth, unsigned int *lineOffsets, unsigned int maxLines) {
  ASSERT(m_font);

  return TextBlockWrapText(m_font, text, m_fontHeight * m_layoutScale, maxWidth * m_layoutScale, lineOffsets, maxLines, 0.0f, m_styleFlags);
}

float CSimpleFontString::GetWidth() {
  float width = CLayoutFrame::GetWidth();

  return width == 0.0f ? GetStringWidth() : width;
}

unsigned int CSimpleFontString::GetNumCharsWithinWidth(const char *text, unsigned int textBytes, float maxWidth) {
  float width;

  ASSERT(m_font);
  if (!textBytes) {
    textBytes = SStrLen(text);
  }

  return GxuFontGetMaxCharsWithinWidth(
      TextBlockGetFontPtr(m_font), text, m_fontHeight * m_layoutScale, maxWidth * m_layoutScale, textBytes, &width, 0.0f, m_styleFlags
  );
}

float CSimpleFontString::GetHeight() {
  float height = CLayoutFrame::GetHeight();

  return height == 0.0f ? GetStringHeight() : height;
}

unsigned int CSimpleFontString::GetNumCharsWithinWidthFromEnd(const char *text, unsigned int textBytes, float maxWidth) {
  float width;

  ASSERT(m_font);
  if (!textBytes) {
    textBytes = SStrLen(text);
  }

  return TextBlockGetMaxCharsWithinWidthFromEnd(
      m_font, text, m_fontHeight * m_layoutScale, maxWidth * m_layoutScale, textBytes, &width, 0.0f, m_styleFlags
  );
}

void CSimpleFontString::SetLayoutScale(float scale, bool force) {
  static const float EPSILON = 2.38418579e-7f;

  if (force || fabs(scale - m_layoutScale) >= EPSILON) {
    CLayoutFrame::SetLayoutScale(scale, force);

    if (m_font) {
      char         fontName[128];
      unsigned int fontFlags;

      SStrCopy(fontName, TextBlockGetFontName(m_font), sizeof(fontName));
      fontFlags = m_font ? TextBlockGetFontFlags(m_font) : 0;
      SetFont(fontName, m_fontHeight, fontFlags);
    }
  }
}

void CSimpleTexture::OnFrameSizeChanged(const NTempest::CRect &rect) {
  CLayoutFrame::OnFrameSizeChanged(rect);

  if (m_TexCoordModifiesPosition) {
    NTempest::CRect texRect = rect;

    TexCorrectRect(texRect);
    SetPosition(texRect);
  } else {
    SetPosition(rect);
  }
}

void CSimpleTexture::Draw(CRenderBatch *batch) {
  if (m_texture) {
    batch->QueueTexture(this);
  }
}

void CSimpleTexture::ClearFromSimpleRegistry() {
  AddToRegistry(0, 0);
}

void CSimpleFontString::OnFrameSizeChanged(const NTempest::CRect &rect) {
  CLayoutFrame::OnFrameSizeChanged(rect);

  if (m_string) {
    NTempest::C3Vector position(rect.l + m_justificationOffset.x * m_layoutScale, rect.t + m_justificationOffset.y * m_layoutScale, 0.0f);
    TextBlockAnimate(m_string, position);
  } else {
    UpdateString(&rect);
  }
}

void CSimpleFontString::Draw(CRenderBatch *batch) {
  if (m_font && m_text && *m_text) {
    batch->QueueFontString(this);
  }
}

void CSimpleFontString::ClearFromSimpleRegistry() {
  AddToRegistry(0, 0);
}

CRenderBatch::CRenderBatch() {
  m_count = 0;
  m_stringbatch = 0;
}

CRenderBatch::~CRenderBatch() {
  Clear();
}

void CRenderBatch::QueueTexture(CSimpleTexture *texture) {
  CGxTex *texturedata = texture->GetTexture();

  if (texturedata) {
    unsigned int index = m_texturelist.Count();

    m_texturelist.SetCount(index + 1);
    CSimpleBatchedTexture &batched = m_texturelist[index];
    batched.textureID = reinterpret_cast<unsigned long>(texturedata);
    batched.position = texture->m_position;
    batched.texCoord = texture->m_texCoord;
    batched.alphamode = texture->m_alphamode;
    batched.GxColor = texture->m_GxColor;
    ++m_count;
  }
}

void CRenderBatch::QueueFontString(CSimpleFontString *string) {
  if (string->m_string) {
    CGxString *gxString = TextBlockGetStringPtr(string->m_string);

    if (gxString) {
      if (!m_stringbatch) {
        m_stringbatch = GxuFontCreateBatch();
        ASSERT(m_stringbatch);
      }

      GxuFontAddToBatch(m_stringbatch, gxString);
      ++m_count;
    }
  }
}

void CRenderBatch::QueueCallback(void(*callback)(void *), void *param) {
  RENDERCALLBACKNODE *node = m_callbacks.NewNode(LIST_TAIL, 0, 0);
  node->callback = callback;
  node->param = param;
  ++m_count;
}

static int __cdecl SortByTexture(const void *A, const void *B) {
  return static_cast<const CSimpleBatchedTexture *>(A)->textureID - static_cast<const CSimpleBatchedTexture *>(B)->textureID;
}

void CRenderBatch::Finish() {
  unsigned int count = m_texturelist.Count();

  if (count > 1) {
    qsort(m_texturelist.Ptr(), count, sizeof(CSimpleBatchedTexture), SortByTexture);
  }
}

void CRenderBatch::Clear() {
  m_texturelist.SetCount(0);

  if (m_stringbatch) {
    GxuFontDestroyBatch(m_stringbatch);
    m_stringbatch = 0;
  }

  while (m_callbacks.Head()) {
    m_callbacks.DeleteNode(m_callbacks.Head());
  }

  m_count = 0;
}
