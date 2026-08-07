#ifndef ENGINE_SOURCE_FRAME_CSIMPLERENDER_H
#define ENGINE_SOURCE_FRAME_CSIMPLERENDER_H

#include "Base/RCString.h"
#include "Frame/CLayoutFrame.h"
#include "FrameScript/FrameScript.h"
#include "Gx/Gx.h"
#include "Tempest/c2vector.h"
#include "Tempest/c3vector.h"
#include "Tempest/cimvector.h"

#include <stpl.h>

struct CSimpleBatchedTexture;
class CSimpleEditBox;
class CSimpleFrame;
class CSimpleFontString;
class CGGameUI;
class CSimpleHTML;
class CSimpleTexture;
struct CGxStringBatch;
struct HTEXTBLOCK__;
struct HTEXTFONT__;
struct HTEXTURE__;
struct CGxString;

LPCSTR     TextBlockGetFontName(HTEXTFONT__ *font);
UINT       TextBlockGetFontFlags(HTEXTFONT__ *font);
CGxString *TextBlockGetStringPtr(HTEXTBLOCK__ *text);

class CRenderBatch;
class CSimpleMessageScrollFrame;

class CSimpleRender {
 public:
  static void DrawBatch(CRenderBatch *batch);

 protected:
  static NTempest::C3Vector s_normal;
  static WORD               s_indices[4];
};

NODEDECL(RENDERCALLBACKNODE) {
  void (*callback)(LPVOID);
  LPVOID param;
};

class CRenderBatch {
  friend class CSimpleRender;

 public:
  CRenderBatch();
  virtual ~CRenderBatch();

  void Clear();
  void Finish();
  void QueueCallback(void (*callback)(LPVOID), LPVOID param);
  void QueueFontString(CSimpleFontString *string);
  void QueueTexture(CSimpleTexture *texture);

  UINT Count() {
    return m_count;
  }

 protected:
  UINT                                   m_count;
  TSGrowableArray<CSimpleBatchedTexture> m_texturelist;
  CGxStringBatch                        *m_stringbatch;
  LISTDECL(RENDERCALLBACKNODE, m_callbacks);

 public:
  LINKDECLEX(CRenderBatch, renderLink);
};

class CSimpleFontStringAttributes {
  friend class CSimpleEditBox;
  friend class CSimpleHTML;

 public:
  enum {
    FLAG_FONT_UPDATE = 0x01,
    FLAG_STYLE_UPDATE = 0x02,
    FLAG_COLOR_UPDATE = 0x04,
    FLAG_SHADOW_UPDATE = 0x08,
    FLAG_SPACING_UPDATE = 0x10,
    FLAG_COMPLETE_UPDATE = 0x1F
  };

  CSimpleFontStringAttributes(const CSimpleFontStringAttributes *attrib = 0)
      : m_font(0), m_fontHeight(0.0f), m_fontFlags(0), m_styleFlags(0x292), m_shadowColor(0ul), m_shadowOffset(0.0f) {
    m_color.Set(1.0f, 1.0f, 1.0f, 1.0f);

    if (attrib) {
      *this = *attrib;
    }

    m_flags = FLAG_COMPLETE_UPDATE;
  }

  const CSimpleFontStringAttributes &operator=(const CSimpleFontStringAttributes &rhs);
  const CSimpleFontStringAttributes &operator=(const CSimpleFontString &rhs);
  void                               SetFont(LPCSTR font, float fontHeight, UINT fontFlags) {
    m_font = font;
    m_fontHeight = fontHeight;
    m_fontFlags = fontFlags;
    m_flags |= FLAG_FONT_UPDATE;
  }
  BYTE HasFont() const {
    LPCSTR font = m_font;
    return font && *font;
  }
  LPCSTR GetFontName() const {
    return m_font;
  }
  float GetFontHeight() const {
    return m_fontHeight;
  }
  UINT GetFontFlags() const {
    return m_fontFlags;
  }
  void SetHorizontalAlignment(UINT alignment) {
    m_styleFlags = (m_styleFlags & ~0x7U) | alignment;
    m_flags |= FLAG_STYLE_UPDATE;
  }
  void SetVerticalAlignment(UINT alignment) {
    m_styleFlags = (m_styleFlags & ~0x38U) | alignment;
    m_flags |= FLAG_STYLE_UPDATE;
  }
  void SetStyleFlags(UINT flags) {
    m_styleFlags = flags;
    m_flags |= FLAG_STYLE_UPDATE;
  }
  float GetSpacing() const {
    return m_spacing;
  }
  void SetColor(const NTempest::CImVector &color) {
    m_color = color;
    m_flags |= FLAG_COLOR_UPDATE;
  }
  void SetAlpha(BYTE alpha) {
    m_color.a = alpha;
    m_flags |= FLAG_COLOR_UPDATE;
  }
  const NTempest::CImVector &GetColor() const {
    return m_color;
  }
  void AddShadow(const NTempest::CImVector &color, const NTempest::C2Vector &offset) {
    m_shadowColor = color;
    m_shadowOffset = offset;
    m_flags |= FLAG_SHADOW_UPDATE;
  }
  void SetSpacing(float spacing) {
    m_spacing = spacing;
    m_flags |= FLAG_SPACING_UPDATE;
  }
  void CopyFlags(const CSimpleFontStringAttributes &rhs);
  void UpdateString(CSimpleFontString *string, int force);

 protected:
  int                 m_flags;
  RCString            m_font;
  float               m_fontHeight;
  UINT                m_fontFlags;
  float               m_spacing;
  UINT                m_styleFlags;
  NTempest::CImVector m_color;
  NTempest::CImVector m_shadowColor;
  NTempest::C2Vector  m_shadowOffset;
};

class CSimpleRegion : public CLayoutFrame {
 public:
  CSimpleRegion(CSimpleFrame *frame, UINT drawlayer, int show);
  virtual ~CSimpleRegion();

  virtual CLayoutFrame *GetLayoutParent();
  virtual void          OnGxColorChanged();
  virtual void          Draw(CRenderBatch *batch) = 0;
  virtual void          ClearFromSimpleRegistry() = 0;

  void                       SetVertexColor(const NTempest::CImVector &color);
  void                       GetVertexColor(NTempest::CImVector &color) const;
  const NTempest::CImVector *GetGxColor() const {
    return m_GxColor;
  }
  void Show();
  void Hide();
  BOOL IsVisible() {
    return m_visible;
  }
  CSimpleFrame *GetParentFrame() {
    return m_frame;
  }
  void SetFrame(CSimpleFrame *frame, UINT drawlayer, int show);
  void OnRegionChanged();

 protected:
  BYTE                       m_color_a;
  NTempest::CImVector        m_color;
  const NTempest::CImVector *m_GxColor;
  CSimpleFrame              *m_frame;
  UINT                       m_drawlayer;
  BOOL                       m_visible;
};

class CSimpleFontString : public FrameScript_Object, public CSimpleRegion {
  friend class CRenderBatch;
  friend class CSimpleEditBox;
  friend class CSimpleFontStringAttributes;
  friend class CSimpleHTML;
  friend class CSimpleMessageScrollFrame;

 public:
  CSimpleFontString(CSimpleFrame *frame, UINT drawlayer, int show);
  virtual ~CSimpleFontString();

  static void RegisterScriptMethods();
  static void UnregisterScriptMethods();

  virtual LPCSTR GetName() const {
    return m_name;
  }
  virtual float         GetWidth();
  virtual float         GetHeight();
  virtual void          OnGxColorChanged();
  virtual void          LoadXML(const XMLNode *node, CStatus *status);
  virtual CLayoutFrame *GetLayoutFrameByName(LPCSTR name);
  virtual void          SetLayoutScale(float scale, bool force);
  void                  PreLoadXML(const XMLNode *node, CStatus *status);
  void                  PostLoadXML(const XMLNode *node, CStatus *status);
  BOOL                  AddToRegistry(LPCSTR name, UINT context);
  void                  SetAttributes(CSimpleFontStringAttributes &attrib) {
    attrib.UpdateString(this, 0);
  }
  BOOL   SetFont(LPCSTR font, float fontHeight, UINT fontFlags);
  void   SetTextLength(int size);
  void   SetText(LPCSTR text);
  void   SetText(int value);
  void   GetText(char *buffer, int bufferBytes) const;
  LPCSTR GetText() const {
    return m_text;
  }
  int GetTextLength() {
    return m_text ? SStrLen(m_text) : 0;
  }
  float GetFontHeight() const {
    return m_fontHeight;
  }
  float GetSpacing() const {
    return m_spacing;
  }
  float GetStringWidth();
  float GetStringHeight();
  float GetTextWidth(LPCSTR text, UINT textBytes);
  UINT  WrapText(LPCSTR text, float maxWidth, UINT *lineOffsets, UINT maxLines);
  UINT  GetNumCharsWithinWidth(LPCSTR text, UINT textBytes, float maxWidth);
  UINT  GetNumCharsWithinWidthFromEnd(LPCSTR text, UINT textBytes, float maxWidth);
  void  SetTextHeight(float height);
  bool  SetAlphaGradient(int startChar, int length);
  void  SetSpacing(float spacing);
  void  AddShadow(const NTempest::CImVector &color, const NTempest::C2Vector &offset);
  void  RemoveShadow();
  BOOL  HasShadow() const {
    return (m_styleFlags & 0x100) != 0;
  }
  LPCSTR GetFontName() const {
    return m_font ? TextBlockGetFontName(m_font) : 0;
  }
  UINT GetFontFlags() const {
    return m_font ? TextBlockGetFontFlags(m_font) : 0;
  }
  void GetShadowColor(NTempest::CImVector &color) const {
    color = m_shadowColor;
  }
  void GetShadowOffset(NTempest::C2Vector &offset) const {
    offset = m_shadowOffset;
  }
  CGxString *GetString() {
    return m_string ? TextBlockGetStringPtr(m_string) : 0;
  }
  void SetJustificationOffset(float x, float y);
  void SetHorizontalAlignment(UINT alignment) {
    ChangeStyleFlags(0x7, alignment);
  }
  UINT GetHorizontalAlignment() const {
    return m_styleFlags & 0x7;
  }

  void SetVerticalAlignment(UINT alignment) {
    ChangeStyleFlags(0x38, alignment);
  }
  UINT GetVerticalAlignment() const {
    return m_styleFlags & 0x38;
  }

  void SetTextColor(const NTempest::CImVector &color) {
    SetVertexColor(color);
  }
  void SetTextColor(float red, float green, float blue, float alpha);
  void GetTextColor(NTempest::CImVector &color) const {
    GetVertexColor(color);
  }
  void SetStyleFlags(UINT flags) {
    ChangeStyleFlags(0xFFFFFFFF, flags);
  }
  UINT GetStyleFlags() const {
    return m_styleFlags;
  }
  void SetCanWrapOnSpace(BOOL canWrap) {
    ChangeStyleFlags(0x1000, canWrap ? 0x1000 : 0);
  }
  void SetFixedColor(int fixed) {
    ChangeStyleFlags(0x400, fixed ? 0x400 : 0);
  }
  void SetIgnoreColorCodes(int ignore) {
    ChangeStyleFlags(0x2000, ignore ? 0x2000 : 0);
  }

  void SetIgnoreNewlines(int ignore) {
    ChangeStyleFlags(0x4000, ignore ? 0x4000 : 0);
  }
  void SetIgnoreHyperlinks(int ignore) {
    ChangeStyleFlags(0x8000, ignore ? 0x8000 : 0);
  }

  void         UpdateString(const NTempest::CRect *rect);
  virtual void OnFrameSizeChanged(const NTempest::CRect &rect);
  virtual void Draw(CRenderBatch *batch);
  virtual void ClearFromSimpleRegistry();

 protected:
  virtual BOOL LookupScriptMethod(lua_State *L, LPCSTR name);

  void ChangeStyleFlags(UINT mask, int flags) {
    UINT styleFlags = (m_styleFlags & ~mask) | flags;

    if (styleFlags != m_styleFlags) {
      m_styleFlags = styleFlags;
      if (m_string) {
        UpdateString(0);
      }
    }
  }

  static TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> s_scriptMethods;

  char               *m_name;
  UINT                m_registryContext;
  HTEXTFONT__        *m_font;
  float               m_fontHeight;
  int                 m_textMaxSize;
  int                 m_textCurSize;
  char               *m_text;
  float               m_spacing;
  HTEXTBLOCK__       *m_string;
  float               m_cachedWidth;
  float               m_cachedHeight;
  NTempest::CImVector m_shadowColor;
  NTempest::C2Vector  m_shadowOffset;
  NTempest::C2Vector  m_justificationOffset;
  int                 m_alphaGradientStart;
  int                 m_alphaGradientLength;
  UINT                m_styleFlags;
};

class CSimpleTexture : public FrameScript_Object, public CSimpleRegion {
  friend class CRenderBatch;
  friend class CGGameUI;
  friend class CSimpleButton;
  friend class CSimpleStatusBar;

 public:
  CSimpleTexture(CSimpleFrame *frame, UINT drawlayer, int show);
  virtual ~CSimpleTexture();

  static void RegisterScriptMethods();
  static void UnregisterScriptMethods();

  virtual LPCSTR GetName() const {
    return m_name;
  }
  virtual void          LoadXML(const XMLNode *node, CStatus *status);
  virtual CLayoutFrame *GetLayoutFrameByName(LPCSTR name);
  void                  PreLoadXML(const XMLNode *node, CStatus *status);
  void                  PostLoadXML(const XMLNode *node, CStatus *status);
  BOOL                  AddToRegistry(LPCSTR name, UINT context);
  BOOL                  SetTexture(HTEXTURE__ *texHandle);
  BOOL                  SetTexture(LPCSTR file, int uvWrapping);
  BOOL                  SetTexture(const NTempest::CImVector &color);
  void                  SetBlendMode(EGxBlend mode);
  void                  SetTexCoord(const NTempest::CRect &rect);
  void                  SetTexCoord(const NTempest::C2Vector *texCoord);
  void                  SetTexCoordModifiesPosition(int modifies) {
    m_TexCoordModifiesPosition = modifies;
  }
  void          TexCorrectRect(NTempest::CRect &rect);
  void          SetPosition(const NTempest::CRect &rect);
  virtual float GetWidth();
  virtual float GetHeight();
  virtual void  OnFrameSizeChanged(const NTempest::CRect &rect);
  CGxTex       *GetTexture();
  HTEXTURE__   *GetHTEXTURE() {
    return m_texture;
  }
  EGxBlend GetAlphaMode() {
    return m_alphamode;
  }
  const NTempest::C3Vector *GetPosition() {
    return m_position;
  }
  const NTempest::C2Vector *GetTexCoord() {
    return m_texCoord;
  }
  static void SetTextureFilterMode(EGxTexFilter mode) {
    s_textureFilterMode = mode;
  }
  virtual void Draw(CRenderBatch *batch);
  virtual void ClearFromSimpleRegistry();

 protected:
  virtual BOOL LookupScriptMethod(lua_State *L, LPCSTR name);

  static EGxTexFilter                                         s_textureFilterMode;
  static TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> s_scriptMethods;

  char              *m_name;
  UINT               m_registryContext;
  HTEXTURE__        *m_texture;
  EGxBlend           m_alphamode;
  NTempest::C3Vector m_position[4];
  NTempest::C2Vector m_texCoord[4];
  int                m_TexCoordModifiesPosition;
};

struct CSimpleBatchedTexture {
  DWORD                      textureID;
  const NTempest::C3Vector  *position;
  const NTempest::C2Vector  *texCoord;
  EGxBlend                   alphamode;
  const NTempest::CImVector *GxColor;
};

#endif
