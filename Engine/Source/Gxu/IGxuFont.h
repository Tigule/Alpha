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

  void        *data;
  unsigned int dataSize;
  unsigned int freeTypeGlyphWidth;
  unsigned int freeTypeGlyphHeight;
  unsigned int freeTypeGlyphPitch;
  int          freeTypeGlyphAdvance;
  float        freeTypeGlyphBearing;
  unsigned int yOffset;
  unsigned int yStart;
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

enum EGxStringFlags {
  EGxStringFlags_FixedSize = 0x4
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

struct GLYPHBITMAPDATA : public TSHashObject<GLYPHBITMAPDATA, HASHKEY_NONE> {
  GLYPHBITMAPDATA();
  ~GLYPHBITMAPDATA();
  void Clear();

  unsigned int    m_code;
  void           *m_data;
  unsigned int    m_dataSize;
  int             m_dirty;
  unsigned int    m_glyphWidth;
  unsigned int    m_glyphHeight;
  unsigned int    m_glyphCellWidth;
  int             m_glyphAdvance;
  float           m_glyphBearing;
  unsigned int    m_glyphPitch;
  int             m_yOffset;
  int             m_yStart;
  NTempest::CRect m_textureCoords;
  int             m_textureValid;
};

struct CHARCODEDESC : public TSHashObject<CHARCODEDESC, HASHKEY_NONE> {
  CHARCODEDESC()
      : dataValid(0),
        textureNumber(static_cast<unsigned int>(-1)),
        rowNumber(static_cast<unsigned int>(-1)),
        glyphStartPixel(static_cast<unsigned int>(-1)),
        glyphEndPixel(0),
        bitmapData(0) {
  }

  ~CHARCODEDESC();

  void         GenerateTextureCoords(unsigned int rowNumber, unsigned int glyphSide);
  unsigned int GapToNextTexture() const;
  unsigned int GapToPreviousTexture() const;

  int ValidBlockEndPoints() const {
    return glyphStartPixel <= glyphEndPixel;
  }

  int ValidTextureCoords() const {
    return bitmapData->m_textureValid;
  }

  unsigned int GetCellWidth() {
    return glyphEndPixel - glyphStartPixel + 1;
  }

  TSLink<CHARCODEDESC> textureRowLink;
  TSLink<CHARCODEDESC> fontGlyphLink;
  int                  dataValid;
  unsigned int         textureNumber;
  unsigned int         rowNumber;
  unsigned int         glyphStartPixel;
  unsigned int         glyphEndPixel;
  GLYPHBITMAPDATA     *bitmapData;
};

class KERNINGHASHKEY {
 public:
  KERNINGHASHKEY(unsigned int currentCode, unsigned int nextCode) : code((currentCode << 16) ^ (nextCode & 0xFFFF)) {
  }

  KERNINGHASHKEY(const KERNINGHASHKEY &key) : code(key.code) {
  }

  KERNINGHASHKEY() {
  }

  KERNINGHASHKEY &operator=(const KERNINGHASHKEY &rhs) {
    if (this != &rhs) {
      code = rhs.code;
    }

    return *this;
  }

  int operator==(const KERNINGHASHKEY &rhs) {
    return this == &rhs || code == rhs.code;
  }

 private:
  unsigned int code;
};

struct KERNNODE : public TSHashObject<KERNNODE, KERNINGHASHKEY> {
  unsigned int flags;
  float        proporportionalSpacing;
  float        fixedWidthSpacing;
};

struct TEXTURECACHEROW {
  TEXTURECACHEROW() : widestFreeSlot(0) {
  }

  CHARCODEDESC *CreateNewDesc(GLYPHBITMAPDATA *data, unsigned int rowNumber, unsigned int glyphCellHeight);
  void          EvictGlyph(CHARCODEDESC *&desc);

  unsigned int                     widestFreeSlot;
  TSExplicitList<CHARCODEDESC, 24> glyphList;
};

struct TEXTURECACHE {
  TEXTURECACHE();
  ~TEXTURECACHE();
  CHARCODEDESC          *AllocateNewGlyph(GLYPHBITMAPDATA *data);
  void                   CreateTexture(int filter);
  void                   Initialize(CGxFont *face, unsigned int thePage, unsigned int pixelSize);
  void                   PasteGlyph(GLYPHBITMAPDATA *data, unsigned long *dst, int thick);
  void                   PasteGlyphNonOutlinedAA(GLYPHBITMAPDATA *glyphData, unsigned long *dst);
  void                   PasteGlyphNonOutlinedMonochrome(GLYPHBITMAPDATA *glyphData, unsigned long *dst);
  void                   PasteGlyphOutlinedAA(GLYPHBITMAPDATA *glyphData, unsigned long *dst, int thick);
  void                   PasteGlyphOutlinedMonochrome(GLYPHBITMAPDATA *glyphData, unsigned long *dst, int thick);
  static void __fastcall TextureCallback(
      EGxTexCommand cmd,
      unsigned int  w,
      unsigned int  h,
      unsigned int  d,
      unsigned int  mipLevel,
      void         *userArg,
      unsigned int &texelStrideInBytes,
      const void  *&texels
  );
  void TextureCallbackHandler(
      EGxTexCommand cmd,
      unsigned int  w,
      unsigned int  h,
      unsigned int  mipLevel,
      unsigned int &texelStrideInBytes,
      const void  *&texels
  );
  void Update();

  CGxTex *GetTexturePtr() const {
    return m_texture;
  }

  void Clear() {
    if (m_texture) {
      GxTexDestroy(m_texture);
    }
    m_texture = 0;
    FREEIFUSED(m_data);
    m_data = 0;
    m_textureRows.Clear();
  }

  int                           m_anyDirtyGlyphs;
  void                         *m_data;
  CGxTex                       *m_texture;
  CGxFont                      *m_theFace;
  unsigned int                  m_page;
  TSFixedArray<TEXTURECACHEROW> m_textureRows;
};

struct CGxFont : public TSLinkedNode<CGxFont> {
  CGxFont();
  ~CGxFont();

  int                 Initialize(const char *name, unsigned int newFlags, float fontHeight);
  void                HandleScreenSizeChange();
  const char         *GetName() const;
  void                Clear();
  void                ClearGlyphs();
  int                 UpdateDimensions();
  void                UpdateTextures();
  int                 CheckStringGlyphs(const char *string);
  unsigned int        GetNumCurrentTextures();
  const CHARCODEDESC *NewCodeDesc(unsigned int code);
  int                 GetGlyphData(GLYPHBITMAPDATA *glyphData, FT_FaceRec_ *face, unsigned int code);
  void                RegisterEvictNotice(unsigned int pageNumber);
  float               ComputeStep(unsigned int currentCode, unsigned int nextCode);
  float               ComputeStepFixedWidth(unsigned int currentCode, unsigned int nextCode);
  float               GetCharAdvance(unsigned int code);
  unsigned int        GetFlags() {
    return m_flags;
  }

  TSExplicitList<CGxString, 8>               m_strings;
  TSLink<CGxFont>                            m_batchedRenderLink;
  TSHashTable<GLYPHBITMAPDATA, HASHKEY_NONE> m_glyphBitmapData;
  TSHashTable<CHARCODEDESC, HASHKEY_NONE>    m_activeCharacters;
  TSHashTable<KERNNODE, KERNINGHASHKEY>      m_kernInfo;
  TSExplicitList<CHARCODEDESC, 32>           m_activeCharacterCache;
  HFACE__                                   *m_faceHandle;
  unsigned int                               m_pixelSize;
  unsigned int                               m_rasterPixelSize;
  char                                       m_fontName[0x104];
  unsigned int                               m_cellHeight;
  unsigned int                               m_baseline;
  unsigned int                               m_flags;
  float                                      m_requestedFontHeight;
  float                                      m_currentFontHeight;
  float                                      m_pixelsPerUnit;
  TEXTURECACHE                               m_textureCache[8];
};

struct GXUFONTHYPERLINKINFO {
  NTempest::CRect extent;
  const char     *link;
  unsigned int    linkLength;
};

enum HYPERLINKPARSEMODE {
  HYPERLINKNONE = 0,
  HYPERLINKHREF = 1,
  HYPERLINKDISPLAY = 2
};

struct HYPERLINKPARSEINFO {
  HYPERLINKPARSEMODE   hyperlinkParseMode;
  GXUFONTHYPERLINKINFO currentParseInfo;
  const char          *lastLinkStartPtr;
  unsigned int         lastLinkLength;
};

struct VERT {
  ~VERT();

  NTempest::C3Vector vc;
  NTempest::C2Vector tc;
};

struct TEXTLINETEXTURE : public TSLinkedNode<TEXTLINETEXTURE> {
  TEXTLINETEXTURE() {
    m_vert.SetChunkSize(64);
    m_colors.SetChunkSize(64);
    m_vertIndices.SetChunkSize(64);
  }

  ~TEXTLINETEXTURE();
  static TEXTLINETEXTURE *NewTextLineTexture();
  void                    Recycle();
  void                    InternalRenderTexture(
      int                        textureNum,
      CGxFont                   *face,
      bool                       showShadow,
      const NTempest::CImVector &shadowColor,
      const NTempest::C2Vector  &shadowOffset,
      const NTempest::CImVector &fontColor
  );

  TSGrowableArray_<VERT, 'GxuF', __LINE__>                m_vert;
  TSGrowableArray_<NTempest::CImVector, 'GxuF', __LINE__> m_shadowColors;
  TSGrowableArray_<NTempest::CImVector, 'GxuF', __LINE__> m_colors;
  TSGrowableArray_<unsigned short, 'GxuF', __LINE__>      m_vertIndices;
};

struct IGXUTEXTLINE : public TSLinkedNode<IGXUTEXTLINE> {
  ~IGXUTEXTLINE() {
    Destroy();
  }

  void                 Destroy();
  static IGXUTEXTLINE *NewGxuTextLine();
  void                 Recycle();
  void                 Reserve(unsigned int numTextLineTextures);

  TSGrowableArray<TEXTLINETEXTURE *> m_texturePages;
};

struct IGXUTEXTBLOCK {
  IGXUTEXTBLOCK() : m_offsetY(0.0f) {
  }

  ~IGXUTEXTBLOCK() {
    m_lines.Clear();
  }

  void          Destroy();
  IGXUTEXTLINE *NewLine();
  void          Recycle();

  float                           m_offsetY;
  TSGrowableArray<IGXUTEXTLINE *> m_lines;
};

struct CGxString : public TSLinkedNode<CGxString> {
  CGxString();
  ~CGxString();
  CGxString *Duplicate() const;

  int Initialize(
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
  );
  void Recycle();
  void Render();
  void Render(const NTempest::C44Matrix &xform);
  void HandleScreenSizeChange();
  void AddShadow(const NTempest::C2Vector &offset, const NTempest::CImVector &color);
  void AddShadowFixedGeometry();
  void RemoveShadow();
  void SetCharSpacing(float spacing);
  int  SetGradient(int startCharacter, int length);
  int  SetGradient(int startCharacter, int length, const TSGrowableArray<NTempest::CImVector *> &array, unsigned char alpha);
  void SetColor(const NTempest::CImVector &color);
  void SetStringPosition(const NTempest::C3Vector &position);
  int IsBillboarded() {
    return (m_flags & 0x80) != 0;
  }
  float GetStringHeight() {
    return m_stringHeight;
  }
  unsigned int Flags() {
    return m_flags;
  }
  void AddFlag(unsigned int flag) {
    m_flags |= flag;
  }
  CGxFont *GetCurrentFace() {
    return m_currentFace;
  }
  float GetSavedWidth() {
    return m_savedWidth;
  }
  float GetSavedHeight() {
    return m_stringHeight;
  }
  void BuildProjection(NTempest::C44Matrix *projPtr, float minx, float maxx, float miny, float maxy, float pixWidth, float pixHeight);
  void BuildView(NTempest::C44Matrix *viewPtr, float width, float height);
  void ClearInstanceData();
  void CreateGeometry();
  void GenerateVertexIndices();
  void TexturePageEvicted(unsigned int pageNumber);
  void InitializeTextLine(
      const char               *currentText,
      unsigned int              numBytes,
      NTempest::CImVector      &workingColor,
      const NTempest::C3Vector &position,
      unsigned int             *texturePagesUsedFlag,
      HYPERLINKPARSEINFO       &info
  );
  void              InitializeViewportOffsets();
  unsigned int      GetHyperLinkInfo(const GXUFONTHYPERLINKINFO *&list) const;
  static CGxString *GetNewString(int linkonList);

  TSLink<CGxString>                      m_fontStringLink;
  TSLink<CGxString>                      m_batchedStringLink;
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
  unsigned int                           m_textLen;
  EGxFontVJusts                          m_vertJust;
  EGxFontHJusts                          m_horzJust;
  float                                  m_spacing;
  unsigned int                           m_flags;
  NTempest::C2Vector                     m_viewportOffset;
  unsigned int                           m_texturePagesUsed;
  int                                    m_textureEvicted;
  float                                  m_stringHeight;
  float                                  m_savedWidth;
  TSGrowableArray<GXUFONTHYPERLINKINFO>  m_hyperlinkInfo;
  TSGrowableArray<NTempest::CImVector *> m_colorGradients;
  TSGrowableArray<NTempest::CImVector *> m_colorGradientShadows;
  int                                    m_lastGradientStart;
  int                                    m_lastGradientLength;

 private:
  void        CheckEvictedTextures();
  void        AddHyperlinkParseInfo(GXUFONTHYPERLINKINFO currentParseInfo);
  inline void ClearStringMatrixEntry();
  void        InternalRender();
  void        RenderTexture(bool initGxRenderStates, int texture);
  void        RenderTexture(int line, int texture);

  friend struct BATCHEDRENDERFONTDESC;
};

struct BATCHEDRENDERFONTDESC : public TSHashObject<BATCHEDRENDERFONTDESC, HASHKEY_PTR> {
  ~BATCHEDRENDERFONTDESC();
  void RenderBatch();

  CGxFont                      *face;
  TSExplicitList<CGxString, 16> m_strings;
};

struct CGxStringBatch : public TSLinkedNode<CGxStringBatch> {
  ~CGxStringBatch() {
    Clear();
  }

  void AddString(CGxString *string);
  void Clear() {
    m_fontBatch.Clear();
  }
  void RenderBatch();

  TSHashTable<BATCHEDRENDERFONTDESC, HASHKEY_PTR> m_fontBatch;
};

struct STRINGVIEWMATRICES : public TSHashObject<STRINGVIEWMATRICES, HASHKEY_PTR> {
  TSLink<STRINGVIEWMATRICES> m_freeLink;
  NTempest::C44Matrix        projection;
  NTempest::C44Matrix        view;
};

extern unsigned int                                         g_heightPixels;
extern unsigned int                                         g_widthPixels;
extern TSList<TEXTLINETEXTURE, TSGetLink<TEXTLINETEXTURE> > g_freeTextLineTextures;
extern TSList<IGXUTEXTLINE, TSGetLink<IGXUTEXTLINE> >       g_freeTextLines;
extern TSList<CGxString, TSGetLink<CGxString> >             g_freeStrings;
extern TSList<CGxString, TSGetLink<CGxString> >             g_strings;

struct FT_LibraryRec_;

FT_LibraryRec_ *__fastcall GetFreeTypeLibrary();
float __fastcall           SignOf(float value);
HFACE__ *__fastcall        FontFaceGetHandle(const char *name, FT_LibraryRec_ *library);
FT_FaceRec_ *__fastcall    FontFaceGetFace(HFACE__ *handle);
void __fastcall            FontFaceCloseHandle(HFACE__ *handle);
const char *__fastcall     FontFaceGetFontName(HFACE__ *handle);
unsigned int __fastcall    GetScreenPixelHeight();
unsigned int __fastcall    GetScreenPixelWidth();
float __fastcall           ScreenToPixelHeight(int billboarded, float height);
float __fastcall           ScreenToPixelWidth(int billboarded, float width);
float __fastcall           GxuFontGetOneToOneHeight(CGxFont *font);
const char *__fastcall     GxuFontGetFontName(CGxFont *fontName);
unsigned int __fastcall    GxuFontGetFontFlags(CGxFont *fontName);
float __fastcall
GxuFontGetWrappedTextHeight(CGxFont *face, const char *text, float fontHeight, float blockWidth, float lineSpacing, unsigned int flags);
unsigned int __fastcall GxuFontWrapText(
    CGxFont      *font,
    const char   *text,
    unsigned int  lineBytes,
    float         fontHeight,
    float         blockWidth,
    unsigned int *outputList,
    unsigned int  outputListElements,
    float         charSpacing,
    unsigned int  flags
);
QUOTEDCODE __fastcall GxuDetermineQuotedCode(
    const char          *text,
    unsigned int        &advance,
    NTempest::CImVector *color,
    unsigned int         flags,
    unsigned int        &wide,
    unsigned int         remainingBytes
);
void __fastcall CalcWrapPoint(
    CGxFont      *face,
    const char   *currentText,
    float         fontHeight,
    float         blockWidth,
    unsigned int *numBytes,
    float        *pExtent,
    const char  **pNextText,
    unsigned int  flags
);
int __fastcall IGxuFontGlyphRenderGlyph(
    FT_FaceRec_ *face,
    unsigned int pixelHeight,
    unsigned int code,
    unsigned int baseLine,
    GLYPHDATA   *dataPtr,
    int          noHinting,
    int          monochrome
);

void __fastcall GxuFontInitialize();
void __fastcall GxuFontShutdown();
void __fastcall GxuFontWindowSizeChanged();

int __fastcall  GxuFontCreateFont(const char *name, float fontHeight, CGxFont *&face, unsigned int flags);
void __fastcall GxuFontDestroyFont(CGxFont *&face);

int __fastcall GxuFontCreateString(
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
);
void __fastcall         GxuFontDestroyString(CGxString *&string);
void __fastcall         GxuFontRender(CGxString *string);
void __fastcall         GxuFontRender(CGxString *string, const NTempest::C44Matrix &xform);
float __fastcall        GxuFontGetStringHeight(CGxString *string);
void __fastcall         GxuFontAddShadow(CGxString *string, const NTempest::CImVector &color, const NTempest::C2Vector &offset);
int __fastcall          GxuFontStringSetGradient(CGxString *string, int startCharacter, int length);
unsigned int __fastcall GxuFontStringHyperLinkInfo(const CGxString *string, const GXUFONTHYPERLINKINFO *&list);
int __fastcall          GxuFontRenderString(
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
);

CGxStringBatch *__fastcall GxuFontCreateBatch();
int __fastcall             GxuFontAddToBatch(CGxStringBatch *batch, CGxString *string);
int __fastcall             GxuFontRemoveFromBatch(CGxString *string);
int __fastcall             GxuFontRenderBatch(CGxStringBatch *batch);
int __fastcall             GxuFontClearBatch(CGxStringBatch *batch);
int __fastcall             GxuFontDestroyBatch(CGxStringBatch *batch);
int __fastcall             GxuFontAddToInternalBatch(CGxString *string);
void __fastcall            GxuFontRenderInternalBatch();

void __fastcall
GxuFontGetTextExtent(CGxFont *face, const char *text, unsigned int numBytes, float height, float *extent, float charSpacing, unsigned int flags);
void __fastcall GxuFontGetWrapPoint(
    CGxFont      *face,
    const char   *text,
    float         fontHeight,
    float         blockWidth,
    unsigned int *numBytes,
    float        *pExtent,
    const char  **pNextText,
    float         spacing,
    unsigned int  flags
);
unsigned int __fastcall GxuFontGetMaxCharsWithinWidth(
    CGxFont     *face,
    const char  *text,
    float        height,
    float        maxWidth,
    unsigned int lineBytes,
    float       *extent,
    float        charSpacing,
    unsigned int flags
);
unsigned int __fastcall GxuFontGetMaxCharsWithinWidthFromEnd(
    CGxFont     *font,
    const char  *text,
    float        fontHeight,
    float        width,
    unsigned int lineBytes,
    float       *extent,
    float        charSpacing,
    unsigned int flags
);
const char *__fastcall
GxuFontStripEscapeCodes(const char *inputString, unsigned int numBytes, unsigned int flags, char *buffer, unsigned int bufferSize);
int __fastcall        GxuFontGetLastColorCode(const char *string, unsigned int numBytes, NTempest::CImVector *color);
int __fastcall        GxuFontGenerateColorString(char *buf, unsigned int bufSize, const NTempest::CImVector &color);
int __fastcall        GxuFontSetStringColor(CGxString *string, NTempest::CImVector newColor);
void __fastcall       GxuFontSetStringPosition(CGxString *string, const NTempest::C3Vector &pos);
void __fastcall       GxuFontSetCharSpacing(CGxString *string, float spacing);
void __fastcall       GxuFontRemoveShadow(CGxString *string);
CGxString *__fastcall GxuFontDuplicateString(const CGxString *rhs);
int __fastcall        GxuFontGetStringWidth(CGxString *string, float *width);
int __fastcall        GxuFontGetStringHeight(CGxString *string, float *height);

void __fastcall IGxuStringInitialize();
void __fastcall IGxuStringShutdown();
void __fastcall GxuFontSetUseAdvanceWidth(int useAdvanceWidth);
void __fastcall InternalGetTextExtent(CGxFont *face, const char *text, unsigned int numBytes, float height, float *extent, unsigned int flags);
unsigned int __fastcall InternalGetMaxCharsWithinWidth(
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
);
