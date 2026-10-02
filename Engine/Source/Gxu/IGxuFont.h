#pragma once

#include <Tempest/c2vector.h>
#include <Tempest/c3vector.h>
#include <Tempest/c44matrix.h>
#include <Tempest/cimvector.h>
#include <Tempest/crect.h>

#include <Gx/Gx.h>
#include <stpl.h>

struct CGxFont;
struct CGxString;
struct CGxTex;
struct HFACE__;
struct FT_FaceRec_;

struct GLYPHDATA {
  GLYPHDATA() : data(0) {
  }

  ~GLYPHDATA() {
    FREEIFUSED(data);
  }

  LPVOID data;
  UINT   dataSize;
  UINT   freeTypeGlyphWidth;
  UINT   freeTypeGlyphHeight;
  UINT   freeTypeGlyphPitch;
  int    freeTypeGlyphAdvance;
  float  freeTypeGlyphBearing;
  UINT   yOffset;
  UINT   yStart;
};

enum QUOTEDCODE {
  CODE_COLORON = 0,
  CODE_COLORRESTORE = 1,
  CODE_NEWLINE = 2,
  CODE_PIPE = 3,
  CODE_HYPERLINKSTART = 4,
  CODE_HYPERLINKSTOP = 5,
  CODE_INVALIDCODE = 6,
  NUM_QUOTEDCODES = 7
};

enum EGxStringFlags {
  EGxStringFlags_FixedSize = 0x4
};

struct VERT {
  NTempest::C3Vector vc;
  NTempest::C2Vector tc;
};

NODEDECL(TEXTLINETEXTURE) {
  TSGrowableArray_<VERT, 'GxuF', __LINE__>                m_vert;
  TSGrowableArray_<NTempest::CImVector, 'GxuF', __LINE__> m_shadowColors;
  TSGrowableArray_<NTempest::CImVector, 'GxuF', __LINE__> m_colors;
  TSGrowableArray_<WORD, 'GxuF', __LINE__>                m_vertIndices;

  TEXTLINETEXTURE() {
    m_vert.SetChunkSize(64);
    m_colors.SetChunkSize(64);
    m_vertIndices.SetChunkSize(64);
  }

  ~TEXTLINETEXTURE();
  void                    Recycle();
  static TEXTLINETEXTURE *NewTextLineTexture();
  void                    InternalRenderTexture(
      int textureNum, CGxFont *face, bool showShadow, const NTempest::CImVector &shadowColor, const NTempest::C2Vector &shadowOffset,
      const NTempest::CImVector &fontColor
  );
};

NODEDECL(IGXUTEXTLINE) {
  TSGrowableArray<TEXTLINETEXTURE *> m_texturePages;

  void                 Destroy();
  void                 Recycle();
  void                 Reserve(UINT numTextLineTextures);
  static IGXUTEXTLINE *NewGxuTextLine();

  ~IGXUTEXTLINE() {
    Destroy();
  }
};

struct IGXUTEXTBLOCK {
 private:
  float                           m_offsetY;
  TSGrowableArray<IGXUTEXTLINE *> m_lines;

 public:
  TSGrowableArray<IGXUTEXTLINE *> &GetLines() {
    return m_lines;
  }

  float YOffset() {
    return m_offsetY;
  }

  void SetYOffset(float offsetY) {
    m_offsetY = offsetY;
  }

  UINT NumLines() {
    return m_lines.Count();
  }

  void          Destroy();
  IGXUTEXTLINE *NewLine();
  void          Recycle();

  IGXUTEXTBLOCK() : m_offsetY(0.0f) {
  }

  ~IGXUTEXTBLOCK() {
    m_lines.Clear();
  }
};

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

struct GLYPHBITMAPDATA : public TSHashObject<GLYPHBITMAPDATA, HASHKEY_NONE> {
  GLYPHBITMAPDATA();
  ~GLYPHBITMAPDATA();
  void Clear();

  UINT            m_code;
  LPVOID          m_data;
  UINT            m_dataSize;
  int             m_dirty;
  UINT            m_glyphWidth;
  UINT            m_glyphHeight;
  UINT            m_glyphCellWidth;
  int             m_glyphAdvance;
  float           m_glyphBearing;
  UINT            m_glyphPitch;
  int             m_yOffset;
  int             m_yStart;
  NTempest::CRect m_textureCoords;
  int             m_textureValid;
};

struct CHARCODEDESC : public TSHashObject<CHARCODEDESC, HASHKEY_NONE> {
  CHARCODEDESC()
      : dataValid(0),
        textureNumber(static_cast<UINT>(-1)),
        rowNumber(static_cast<UINT>(-1)),
        glyphStartPixel(static_cast<UINT>(-1)),
        glyphEndPixel(0),
        bitmapData(0) {
  }

  BOOL ValidBlockEndPoints() const {
    return glyphStartPixel <= glyphEndPixel;
  }

  int ValidTextureCoords() const {
    return bitmapData->m_textureValid;
  }

  UINT GetCellWidth() const {
    return glyphEndPixel - glyphStartPixel + 1;
  }

  void GenerateTextureCoords(UINT rowNumber, UINT glyphSide);
  UINT GapToNextTexture() const;
  UINT GapToPreviousTexture() const;

  LINKDECLEX(CHARCODEDESC, textureRowLink);
  LINKDECLEX(CHARCODEDESC, fontGlyphLink);
  int              dataValid;
  UINT             textureNumber;
  UINT             rowNumber;
  UINT             glyphStartPixel;
  UINT             glyphEndPixel;
  GLYPHBITMAPDATA *bitmapData;
};

struct TEXTURECACHEROW {
  TEXTURECACHEROW() : widestFreeSlot(0) {
  }

  void          EvictGlyph(CHARCODEDESC *&desc);
  CHARCODEDESC *CreateNewDesc(GLYPHBITMAPDATA *data, UINT rowNumber, UINT glyphCellHeight);

  UINT widestFreeSlot;
  LISTDECLEX(CHARCODEDESC, textureRowLink, glyphList);
};

struct TEXTURECACHE {
  BOOL                          m_anyDirtyGlyphs;
  LPVOID                        m_data;
  CGxTex                       *m_texture;
  CGxFont                      *m_theFace;
  UINT                          m_page;
  TSFixedArray<TEXTURECACHEROW> m_textureRows;

  TEXTURECACHE();
  ~TEXTURECACHE();
  void          CreateTexture(int filter);
  CHARCODEDESC *AllocateNewGlyph(GLYPHBITMAPDATA *data);

  CGxTex *GetTexturePtr() {
    return m_texture;
  }

  void Initialize(CGxFont *face, UINT thePage, UINT pixelSize);

  void Clear() {
    if (m_texture) {
      GxTexDestroy(m_texture);
    }
    m_texture = 0;
    FREEIFUSED(m_data);
    m_data = 0;
    m_textureRows.Clear();
  }

  void        Update();
  void        PasteGlyph(GLYPHBITMAPDATA *data, DWORD *dst, int thick);
  void        PasteGlyphOutlinedMonochrome(GLYPHBITMAPDATA *glyphData, DWORD *dst, int thick);
  void        PasteGlyphNonOutlinedMonochrome(GLYPHBITMAPDATA *glyphData, DWORD *dst);
  void        PasteGlyphOutlinedAA(GLYPHBITMAPDATA *glyphData, DWORD *dst, int thick);
  void        PasteGlyphNonOutlinedAA(GLYPHBITMAPDATA *glyphData, DWORD *dst);
  static void TextureCallback(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &texels);
  void        TextureCallbackHandler(EGxTexCommand cmd, UINT w, UINT h, UINT mipLevel, UINT &texelStrideInBytes, LPCVOID &texels);
};

struct GXUFONTHYPERLINKINFO {
  NTempest::CRect extent;
  LPCSTR          link;
  UINT            linkLength;
};

enum HYPERLINKPARSEMODE {
  HYPERLINKNONE = 0,
  HYPERLINKHREF = 1,
  HYPERLINKDISPLAY = 2
};

struct HYPERLINKPARSEINFO {
  HYPERLINKPARSEINFO() {
  }

  HYPERLINKPARSEMODE   hyperlinkParseMode;
  GXUFONTHYPERLINKINFO currentParseInfo;
  LPCSTR               lastLinkStartPtr;
  UINT                 lastLinkLength;
};

NODEDECL(CGxString) {
  LINKDECLEX(CGxString, m_fontStringLink);
  LINKDECLEX(CGxString, m_batchedStringLink);

 private:
  float                                  m_requestedFontHeight;
  float                                  m_currentFontHeight;
  NTempest::C3Vector                     m_position;
  NTempest::CImVector                    m_fontColor;
  NTempest::CImVector                    m_shadowColor;
  NTempest::C2Vector                     m_shadowOffset;
  float                                  m_blockWidth;
  float                                  m_blockHeight;
  CGxFont                               *m_currentFace;
  IGXUTEXTBLOCK                          m_textBlock;
  char                                  *m_text;
  UINT                                   m_textLen;
  EGxFontVJusts                          m_vertJust;
  EGxFontHJusts                          m_horzJust;
  float                                  m_spacing;
  UINT                                   m_flags;
  NTempest::C2Vector                     m_viewportOffset;
  UINT                                   m_texturePagesUsed;
  BOOL                                   m_textureEvicted;
  float                                  m_stringHeight;
  float                                  m_savedWidth;
  TSGrowableArray<GXUFONTHYPERLINKINFO>  m_hyperlinkInfo;
  TSGrowableArray<NTempest::CImVector *> m_colorGradients;
  TSGrowableArray<NTempest::CImVector *> m_colorGradientShadows;
  int                                    m_lastGradientStart;
  int                                    m_lastGradientLength;

 public:
  BOOL IsBillboarded() const {
    return (m_flags & 0x80) != 0;
  }

  void Render();
  void Render(const NTempest::C44Matrix &xform);
  CGxString();
  CGxString(const CGxString &);
  ~CGxString();

  int Initialize(
      float fontHeight, const NTempest::C3Vector &position, float blockWidth, float blockHeight, CGxFont *face, LPCSTR text, EGxFontVJusts vertJust,
      EGxFontHJusts horzJust, float spacing, UINT flags, const NTempest::CImVector &color
  );
  void Recycle();
  void CreateGeometry();
  void InitializeViewportOffsets();
  void HandleScreenSizeChange();
  void RemoveShadow();
  void SetColor(const NTempest::CImVector &color);
  void SetStringPosition(const NTempest::C3Vector &position);
  void TexturePageEvicted(UINT pageNumber);
  void GenerateVertexIndices();
  void SetCharSpacing(float spacing);

  float GetStringHeight() const {
    return m_stringHeight;
  }

  UINT Flags() {
    return m_flags;
  }

  void AddFlag(UINT flag) {
    m_flags |= flag;
  }

  static CGxString *GetNewString(int linkonList);

  CGxFont *GetCurrentFace() const {
    return m_currentFace;
  }

  void BuildView(NTempest::C44Matrix * viewPtr, float width, float height);
  void BuildProjection(NTempest::C44Matrix * projPtr, float minx, float maxx, float miny, float maxy, float pixWidth, float pixHeight);
  void InitializeTextLine(
      LPCSTR currentText, UINT numBytes, NTempest::CImVector & workingColor, const NTempest::C3Vector &position, UINT *texturePagesUsedFlag,
      HYPERLINKPARSEINFO &info
  );
  CGxString *Duplicate() const;

  float GetSavedWidth() const {
    return m_savedWidth;
  }

  float GetSavedHeight() const {
    return m_stringHeight;
  }

  UINT GetHyperLinkInfo(const GXUFONTHYPERLINKINFO *&list) const;
  int  SetGradient(int startCharacter, int length);
  int  SetGradient(int startCharacter, int length, const TSGrowableArray<NTempest::CImVector *> &array, BYTE alpha);
  void AddShadow(const NTempest::C2Vector &offset, const NTempest::CImVector &color);
  void AddShadowFixedGeometry();
  void ClearInstanceData();

 private:
  void InternalRender();
  void InternalRender(BYTE);
  void ClearStringMatrixEntry();
  void AddHyperlinkParseInfo(GXUFONTHYPERLINKINFO currentParseInfo);
  void CheckEvictedTextures();
  void RenderTexture(bool initGxRenderStates, int texture);
  void RenderTexture(int line, int texture);

  friend struct BATCHEDRENDERFONTDESC;
};

class KERNINGHASHKEY {
 private:
  UINT code;

 public:
  KERNINGHASHKEY() : code(0) {
  }

  KERNINGHASHKEY(const KERNINGHASHKEY &key) : code(key.code) {
  }

  KERNINGHASHKEY(UINT currentCode, UINT nextCode) : code((currentCode << 16) ^ (nextCode & 0xFFFF)) {
  }

  KERNINGHASHKEY &operator=(const KERNINGHASHKEY &rhs) {
    if (this != &rhs) {
      code = rhs.code;
    }

    return *this;
  }

  int operator==(const KERNINGHASHKEY &rhs) const {
    return this == &rhs || code == rhs.code;
  }

};

struct KERNNODE : public TSHashObject<KERNNODE, KERNINGHASHKEY> {
  KERNNODE() : flags(0), proporportionalSpacing(0.0f), fixedWidthSpacing(0.0f) {
  }

  KERNNODE(const KERNNODE &);

  UINT  flags;
  float proporportionalSpacing;
  float fixedWidthSpacing;
};

NODEDECL(CGxFont) {
  LISTDECLEX(CGxString, m_fontStringLink, m_strings);
  LINKDECLEX(CGxFont, m_batchedRenderLink);
  TSHashTable<GLYPHBITMAPDATA, HASHKEY_NONE> m_glyphBitmapData;
  TSHashTable<CHARCODEDESC, HASHKEY_NONE>    m_activeCharacters;
  TSHashTable<KERNNODE, KERNINGHASHKEY>      m_kernInfo;
  LISTDECLEX(CHARCODEDESC, fontGlyphLink, m_activeCharacterCache);
  HFACE__     *m_faceHandle;
  UINT         m_pixelSize;
  UINT         m_rasterPixelSize;
  char         m_fontName[MAX_PATH];
  UINT         m_cellHeight;
  UINT         m_baseline;
  UINT         m_flags;
  float        m_requestedFontHeight;
  float        m_currentFontHeight;
  float        m_pixelsPerUnit;
  TEXTURECACHE m_textureCache[8];

  CGxFont();
  ~CGxFont();
  void                Clear();
  void                ClearGlyphs();
  void                UpdateTextures();
  const CHARCODEDESC *NewCodeDesc(UINT code);
  int                 GetGlyphData(GLYPHBITMAPDATA * glyphData, FT_FaceRec_ * face, UINT code);
  void                RegisterEvictNotice(UINT pageNumber);
  int                 CheckStringGlyphs(LPCSTR string);
  int                 UpdateDimensions();
  int                 Initialize(LPCSTR name, UINT newFlags, float fontHeight);
  UINT                GetNumCurrentTextures();
  float               ComputeStep(UINT currentCode, UINT nextCode);
  float               ComputeStepFixedWidth(UINT currentCode, UINT nextCode);
  float               GetCharAdvance(UINT code);
  void                HandleScreenSizeChange();
  LPCSTR              GetName() const;

  UINT GetFlags() const {
    return m_flags;
  }
};

struct BATCHEDRENDERFONTDESC : public TSHashObject<BATCHEDRENDERFONTDESC, HASHKEY_PTR> {
  CGxFont *face;
  LISTDECLEX(CGxString, m_batchedStringLink, m_strings);

  BATCHEDRENDERFONTDESC() : face(0) {
  }
  BATCHEDRENDERFONTDESC(const BATCHEDRENDERFONTDESC &);
  ~BATCHEDRENDERFONTDESC();
  void RenderBatch();
};

NODEDECL(CGxStringBatch) {
  ~CGxStringBatch() {
    Clear();
  }

  void Clear() {
    m_fontBatch.Clear();
  }
  void AddString(CGxString * string);
  void RenderBatch();

 private:
  TSHashTable<BATCHEDRENDERFONTDESC, HASHKEY_PTR> m_fontBatch;
};

struct STRINGVIEWMATRICES : public TSHashObject<STRINGVIEWMATRICES, HASHKEY_PTR> {
  LINKDECLEX(STRINGVIEWMATRICES, m_freeLink);
  NTempest::C44Matrix projection;
  NTempest::C44Matrix view;
};

extern UINT g_heightPixels;
extern UINT g_widthPixels;
extern LISTDECL(TEXTLINETEXTURE, g_freeTextLineTextures);
extern LISTDECL(IGXUTEXTLINE, g_freeTextLines);
extern LISTDECL(CGxString, g_freeStrings);
extern LISTDECL(CGxString, g_strings);

struct FT_LibraryRec_;

FT_LibraryRec_ *GetFreeTypeLibrary();
float           SignOf(float value);
HFACE__        *FontFaceGetHandle(LPCSTR name, FT_LibraryRec_ *library);
FT_FaceRec_    *FontFaceGetFace(HFACE__ *handle);
void            FontFaceCloseHandle(HFACE__ *handle);
LPCSTR          FontFaceGetFontName(HFACE__ *handle);
UINT            GetScreenPixelHeight();
UINT            GetScreenPixelWidth();
float           ScreenToPixelHeight(int billboarded, float height);
float           ScreenToPixelWidth(int billboarded, float width);
float           GxuFontGetOneToOneHeight(CGxFont *font);
LPCSTR          GxuFontGetFontName(CGxFont *fontName);
UINT            GxuFontGetFontFlags(CGxFont *fontName);
float           GxuFontGetWrappedTextHeight(CGxFont *face, LPCSTR text, float fontHeight, float blockWidth, float lineSpacing, UINT flags);
UINT            GxuFontWrapText(
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
QUOTEDCODE GxuDetermineQuotedCode(LPCSTR text, UINT &advance, NTempest::CImVector *color, UINT flags, UINT &wide, UINT remainingBytes);
void       CalcWrapPoint(
    CGxFont *face,
    LPCSTR   currentText,
    float    fontHeight,
    float    blockWidth,
    UINT    *numBytes,
    float   *pExtent,
    LPCSTR  *pNextText,
    UINT     flags
);
BOOL IGxuFontGlyphRenderGlyph(FT_FaceRec_ *face, UINT pixelHeight, UINT code, UINT baseLine, GLYPHDATA *dataPtr, int noHinting, int monochrome);

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

void IGxuStringInitialize();
void IGxuStringShutdown();
void GxuFontSetUseAdvanceWidth(int useAdvanceWidth);
void InternalGetTextExtent(CGxFont *face, LPCSTR text, UINT numBytes, float height, float *extent, UINT flags);
UINT InternalGetMaxCharsWithinWidth(
    CGxFont *face,
    LPCSTR   text,
    float    height,
    float    maxWidth,
    UINT     lineBytes,
    float   *extent,
    UINT     flags,
    UINT    *bytesInString,
    float   *widthArray,
    float   *widthArrayGuard
);
