#include <new>

#include "Texture.h"

#include "AsyncFileRead.h"
#include "Base/FileCache.h"
#include "Base/Status.h"
#include "BLPFile/blp.h"
#include "Gx/Gx.h"
#include "Images/dxt.h"
#include "Images/tga.h"
#include "Os/OsTime.h"
#include "Os/W32/Debugging.h"

#include <stpl.h>
#include <stdlib.h>
#include <string.h>

class HASHKEY_TEXTUREFILE {
 private:
  char       *m_filename;
  CGxTexFlags m_flags;
 public:
  HASHKEY_TEXTUREFILE() : m_filename(0), m_flags(GxTex_Linear, 0, 0, 0, 0, 0, 1) {
  }

  HASHKEY_TEXTUREFILE(const HASHKEY_TEXTUREFILE &source) : m_filename(0), m_flags(GxTex_Linear, 0, 0, 0, 0, 0, 1) {
    m_filename = SStrDupA(source.m_filename, __FILE__, __LINE__);
    m_flags = source.m_flags;
  }

  HASHKEY_TEXTUREFILE(LPCSTR filename, CGxTexFlags flags) : m_flags(GxTex_Linear, 0, 0, 0, 0, 0, 1) {
    m_filename = SStrDupA(filename, __FILE__, __LINE__);
    m_flags = flags;
  }

  ~HASHKEY_TEXTUREFILE() {
    FREEIFUSED(m_filename);
  }

  HASHKEY_TEXTUREFILE &operator=(const HASHKEY_TEXTUREFILE &source) {
    FREEIFUSED(m_filename);
    m_filename = SStrDupA(source.m_filename, __FILE__, __LINE__);
    m_flags = source.m_flags;
    return *this;
  }

  bool operator==(const HASHKEY_TEXTUREFILE &source) const {
    return *reinterpret_cast<const UINT *>(&m_flags) == *reinterpret_cast<const UINT *>(&source.m_flags) &&
           SStrCmpI(m_filename, source.m_filename, 0x104) == 0;
  }

};

class CTexture : public CHandleObject {
 public:
  CTexture();
  virtual ~CTexture();

  virtual LPCSTR GetObjectName();

  char          filename[0x104];
  UINT          flags;
  WORD          pixBitDepth;
  WORD          alphaBits;
  MipBits      *mipBits;
  CStatus       loadStatus;
  CGxTex       *gxTex;
  UINT          gxWidth;
  UINT          gxHeight;
  EGxTexFormat  gxTexFormat;
  EGxTexFormat  dataFormat;
  CGxTexFlags   gxTexFlags;
  CAsyncObject *asyncObject;
  LINKDECLEX(CTexture, link);
};

struct CTextureItem {
  CTextureItem(int p_fromColor = 0) : texture(0), fromColor(p_fromColor), timeStamp(0) {
  }

  CTextureItem(const CTextureItem &source) : fromColor(source.fromColor), timeStamp(source.timeStamp) {
    texture = reinterpret_cast<HTEXTURE>(HandleDuplicate(source.texture));
  }

  ~CTextureItem() {
    if (texture)
      HandleClose(texture);
  }

  HTEXTURE texture;
  int      fromColor;
  DWORD    timeStamp;
  LINKDECLEX(CTextureItem, link);
};

struct CTextureHash : public TSHashObject<CTextureHash, HASHKEY_TEXTUREFILE>, public CTextureItem {
  CTextureHash() : CTextureItem(0) {
  }
};

struct CSolidTextureHash : public TSHashObject<CSolidTextureHash, HASHKEY_NONE>, public CTextureItem {
  CSolidTextureHash() : CTextureItem(1) {
  }
};

class CGxTexCache {
 public:
  CGxTex *gxTex;
  DWORD   timeStamp;
  LINKDECLEX(CGxTexCache, link);

  CGxTexCache() {
    gxTex = 0;
    timeStamp = 0;
  }

  ~CGxTexCache() {
  }
};

enum EImageFormat {
  IMAGE_FORMAT_TGA = 0,
  IMAGE_FORMAT_BLP = 1,
  NUM_IMAGE_FORMATS = 2
};

static TSHashTableReuse<CTextureHash, HASHKEY_TEXTUREFILE, 1> s_textureCache;
static TSHashTableReuse<CSolidTextureHash, HASHKEY_NONE, 1>   s_solidTextureCache;
static MipBits *tgaMips;
static LISTDECLEX(CTextureItem, link, s_textureCacheLRU);
static HASHKEY_NONE s_hashKeyNone;
static LISTDECLEX(CTexture, link, s_textureList);
static LPVOID              g_textureMipBits;
static NTempest::CImVector CRAPPY_GREEN(0xFF00FF00UL);
static LISTDECLEX(CGxTexCache, link, s_gxTexCacheList[5][5][GxTexFormats_Last]);
static const WORD s_bitDepth[8] = {0, 32, 16, 16, 16, 4, 8, 8};
static LPCSTR     s_formatExt[NUM_IMAGE_FORMATS] = {".tga", ".blp"};
static LISTDECLEX(CGxTexCache, link, s_gxTexCacheFreeList);
static TSCArray<BYTE, 0x100000> s_asyncLoadBuffer;
static LISTDECLEX(CAsyncObject, link, s_asyncLoadList);
static UINT s_asyncLoadBufferUsed;
static char s_gxTexFormatStrings[8][32] = {"GxTex_Unknown", "GxTex_Argb8888", "GxTex_Argb4444", "GxTex_Argb1555",
                                           "GxTex_Rgb565",  "GxTex_Dxt1",     "GxTex_Dxt3",     "GxTex_Dxt5"};
static char s_gxTexFilterStrings[5][32] = {"GxTex_Nearest", "GxTex_Linear", "GxTex_LinearMipNearest", "GxTex_LinearMipLinear", ""};
char       *s_textureLogString[7] = {"character", "creature", "dungeon", "interface", "world", "tileset", "item"};
static int  s_asyncPending;

static HTEXTURE GetTexture(LPCSTR texMap, CGxTexFlags flags);
static HTEXTURE GetTexture(const NTempest::CImVector &color);
static int      TextureIsUsed(HTEXTURE texture);
static void     HashNewTexture(LPCSTR texMap, CGxTexFlags flags, HTEXTURE texture, CStatus *status);
static void     HashNewTexture(const NTempest::CImVector &color, HTEXTURE texture, CStatus *status);
static void     FileError(CStatus *status, LPCSTR description, LPCSTR path);
static UINT     LoadPredrawnMips(const CTgaFile &mipZero, LPCSTR filemask, MipBits *buffer);
static void     RemoveExtension(char *path);
static void     GenerateMipMask(LPCSTR mipZeroName, char *mipMask);
static UINT     CalcLevelSize(UINT level, UINT width, UINT height, EGxTexFormat format);
static UINT     CalcLevelOffset(UINT level, UINT width, UINT height, EGxTexFormat format);
static MipBits *GetDefaultTexture(UINT height, UINT width, UINT format);
static MipBits *LoadTgaMips(LPCSTR fileName, UINT *width, UINT *height, EGxTexFormat *format, int *isOpaque, UINT *alphaBits);
static void     UpdateTgaTexture(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &texels);
static void     RequestImageDimensions(UINT *width, UINT *height, UINT *bestMip);
static HTEXTURE CreateTgaTexture(LPCSTR file, CGxTexFlags flags, CStatus *status);
static void UpdateTextureDefault(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &texels);
static CGxTex *TextureAllocGxTex(
    UINT         width,
    UINT         height,
    EGxTexFormat format,
    CGxTexFlags  flags,
    LPVOID       userArg,
    void (*userFunc)(EGxTexCommand, UINT, UINT, UINT, UINT, LPVOID, UINT &, LPCVOID &),
    EGxTexFormat dataFormat
);
static void TextureFreeGxTex(CGxTex *gxTex);
void
UpdateBlpTextureAsync(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &texels);
static void GetTextureFormats(
    CTexture          *texture,
    PIXEL_FORMAT      &pixFormat,
    EGxTexFormat      &dataFormat,
    EGxTexFormat      &gxTexFormat,
    const PIXEL_FORMAT preferredFormat,
    const UINT         alphaBits
);
static BOOL         AsyncTextureLoadImageCreate(CTexture *texture);
static void         FillInSolidTexture(const NTempest::CImVector &color, CTexture *texture);
static void         AsyncCreateBlpTextureCallback(LPVOID arg);
static void         AsyncTextureLoadImageCallback(LPVOID arg);
static void         AsyncTextureHandler();
static void         AsyncTextureWait(CTexture *texture);
static HTEXTURE     CreateBlpTexture(LPCSTR filename, CGxTexFlags flags, CStatus *status);
static EImageFormat IdentifyAndStripFileExtension(LPCSTR fileName, char *stripped, char **ext);
void         TextureGenerateMips(UINT width, UINT height, UINT levelsProvided, UINT levelsDesired, MipBits *levelBits);
static int __cdecl  TextureLogSortCallback(LPCVOID elem1, LPCVOID elem2);

CTexture::CTexture() : gxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1) {
  filename[0] = 0;
  flags = 0;
  alphaBits = 0;
  mipBits = 0;
  asyncObject = 0;
  gxTex = 0;
  gxWidth = 0;
  gxHeight = 0;
  gxTexFormat = GxTex_Unknown;
  dataFormat = GxTex_Unknown;
  pixBitDepth = 0;

  s_textureList.LinkNode(this, LIST_TAIL, 0);
}

LPCSTR CTexture::GetObjectName() {
  return filename;
}

CTexture::~CTexture() {
  s_textureList.UnlinkNode(this);

  if (gxTex) {
    TextureFreeGxTex(gxTex);
  }

  if (mipBits) {
    TextureFreeMippedImg(mipBits);
  }

  if (asyncObject) {
    SFile *file;

    OsOutputDebugString("Texture destroyed before loading completed: %s\n", filename);

    if (asyncObject->buffer) {
      --s_asyncPending;
    }

    file = asyncObject->file;
    AsyncFileReadDestroyObject(asyncObject);
    asyncObject = 0;
    SFile::Close(file);
  }
}

static void TextureFreeGxTex(CGxTex *gxTex) {
  ASSERT(gxTex);

  CGxTexParmsEx gxTexParmsEx;
  GxTexParametersEx(gxTex, gxTexParmsEx);

  if (gxTexParmsEx.width < 32 || gxTexParmsEx.height < 32 || gxTexParmsEx.format > GxTex_Dxt5) {
    GxTexDestroy(gxTex);
    return;
  }

  gxTexParmsEx.width >>= 5;
  gxTexParmsEx.height >>= 5;

  UINT indexW = 0;
  while (!(gxTexParmsEx.width & 1)) {
    gxTexParmsEx.width >>= 1;
    ++indexW;
  }

  UINT indexH = 0;
  while (!(gxTexParmsEx.height & 1)) {
    gxTexParmsEx.height >>= 1;
    ++indexH;
  }

  CGxTexCache *gxTexCache = s_gxTexCacheFreeList.Head();
  if (!gxTexCache) {
    gxTexCache = s_gxTexCacheFreeList.NewNode(LIST_HEAD, 0, 0);
    ASSERT(gxTexCache);
  }

  gxTexCache->link.Unlink();
  gxTexCache->gxTex = gxTex;
  gxTexCache->timeStamp = OsGetAsyncTimeMs();
  s_gxTexCacheList[indexW][indexH][gxTexParmsEx.format].LinkNode(gxTexCache, LIST_HEAD, 0);
  GxTexSetUserData(gxTexCache->gxTex, UpdateTextureDefault, 0);
}

static void
UpdateTextureDefault(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &texels) {
  if (userArg) {
    SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "1", FALSE);
  }
}

void
UpdateBlpTextureAsync(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &texels) {
  CTexture *texture = static_cast<CTexture *>(userArg);

  ASSERT(texture);

  switch (cmd) {
    case GxTex_Lock:
      if (!texture->mipBits) {
        EGxTexFormat gxFormat;

        OsOutputDebugString("UpdateBlpTextureAsync(): GxTex_Lock loading: %s\n", texture->filename);
        texture->mipBits = static_cast<MipBits *>(g_textureMipBits);
        if (!LoadBlpMips(texture->filename, texture->mipBits, 0, 0, &gxFormat, 0, 0)) {
          texture->mipBits = GetDefaultTexture(w, h, GxTex_Argb8888);
          GetGlobalStatusObj().Add(STATUS_ERROR, "Texture %s not loaded -- replaced with default.\n", texture->filename);
        }
      }
      break;

    case GxTex_Latch:
      ASSERT(texture->mipBits);
      ASSERT(texture->mipBits);
      texelStrideInBytes = (w * texture->pixBitDepth) >> 3;
      texels = texture->mipBits->mip[mipLevel];
      break;

    case GxTex_Unlock:
      texture->mipBits = 0;
      break;
  }
}

static MipBits *GetDefaultTexture(UINT height, UINT width, UINT format) {
  ASSERT(format == GxTex_Argb8888);

  MipBits *buffer = MippedImgAllocA(PIXEL_ARGB8888, width, height, __FILE__, __LINE__);
  MipBits *mip = buffer;

  while (width > 1 || height > 1) {
    memset(mip->mip, 0xFF, 4 * width * height);
    ++mip;

    width = max(1, width >> 1);
    height = max(1, height >> 1);
  }

  return buffer;
}

static BOOL LoadBlpMips(LPCSTR fileName, MipBits *&buffer, UINT *width, UINT *height, EGxTexFormat *format, int *isOpaque, UINT *alphaBits) {
  ASSERT(fileName);

  CBLPFile texFile;
  if (!texFile.Open(fileName)) {
    return 0;
  }

  EGxTexFormat gxFormat = GxTex_Argb8888;
  PIXEL_FORMAT pixelFormat = PIXEL_ARGB8888;

  if (texFile.m_header.colorEncoding == COLOR_DXT) {
    pixelFormat = static_cast<PIXEL_FORMAT>(texFile.m_header.preferredFormat);

    switch (pixelFormat) {
      case PIXEL_DXT1:
        if (GxCaps().m_texFmtDxt) {
          gxFormat = GxTex_Dxt1;
        } else if (!texFile.m_header.alphaSize) {
          gxFormat = GxTex_Rgb565;
          pixelFormat = PIXEL_RGB565;
        } else {
          gxFormat = GxTex_Argb1555;
          pixelFormat = PIXEL_ARGB1555;
        }
        break;

      case PIXEL_DXT3:
        if (GxCaps().m_texFmtDxt) {
          gxFormat = GxTex_Dxt3;
        } else {
          gxFormat = GxTex_Argb4444;
          pixelFormat = PIXEL_ARGB4444;
        }
        break;

      case PIXEL_DXT5:
        if (GxCaps().m_texFmtDxt) {
          gxFormat = GxTex_Dxt5;
        } else {
          gxFormat = GxTex_Argb4444;
          pixelFormat = PIXEL_ARGB4444;
        }
        break;

      default:
        ASSERT(0);
        break;
    }
  }

  if (isOpaque) {
    *isOpaque = texFile.m_header.alphaSize == 0;
  }

  if (format) {
    *format = gxFormat;
  }

  UINT bestMip = 0;
  UINT imgWidth = texFile.m_header.width;
  UINT imgHeight = texFile.m_header.height;
  RequestImageDimensions(&imgWidth, &imgHeight, &bestMip);

  if (width) {
    *width = imgWidth;
  }

  if (height) {
    *height = imgHeight;
  }

  if (alphaBits) {
    *alphaBits = texFile.m_header.alphaSize;
  }

  if (!texFile.LockChain(pixelFormat, buffer, bestMip)) {
    return 0;
  }

  return 1;
}

static void RequestImageDimensions(UINT *width, UINT *height, UINT *bestMip) {
  CGxCaps systemCaps = GxCaps();

  VALIDATEBEGIN;
  VALIDATE(systemCaps.m_maxTextureSize > 0);
  VALIDATEENDVOID;

  while (*width > systemCaps.m_maxTextureSize || *height > systemCaps.m_maxTextureSize) {
    *width >>= 1;
    *height >>= 1;
    ++*bestMip;

    if (!*width) {
      *width = 1;
    }

    if (!*height) {
      *height = 1;
    }
  }
}

void TextureInitialize() {
  g_textureMipBits = SMemAlloc(MippedImgCalcSize(2, 512, 512));
  ASSERT(g_textureMipBits);

  AsyncFileReadAddHandler(AsyncTextureHandler);
}

static void AsyncTextureHandler() {
  UINT bufferRemaining;

  ASSERT(s_asyncPending >= 0);

  if (s_asyncPending) {
    return;
  }

  s_asyncLoadBufferUsed = 0;
  bufferRemaining = s_asyncLoadBuffer.MaxCount();

  for (CAsyncObject *asyncObject = s_asyncLoadList.Head(), *asyncObjectnext_node;
       (int)asyncObject > 0 ? ((asyncObjectnext_node = s_asyncLoadList.RawNext(asyncObject)), 1) : 0; asyncObject = asyncObjectnext_node) {
    if (bufferRemaining >= asyncObject->size) {
      asyncObject->link.Unlink();
      asyncObject->buffer = &s_asyncLoadBuffer[s_asyncLoadBufferUsed];
      asyncObject->canReorder = 1;
      s_asyncLoadBufferUsed += asyncObject->size;
      bufferRemaining -= asyncObject->size;
      ++s_asyncPending;
      AsyncFileReadObject(asyncObject);
    }
  }
}

void TextureCacheFlush() {
  s_textureCacheLRU.UnlinkAll();
  s_textureCache.Destroy();
  s_solidTextureCache.Destroy();
}

void TextureGxCacheFlush() {
  UINT x;
  UINT y;
  UINT z;

  for (x = 0; x < 5; ++x) {
    for (y = 0; y < 5; ++y) {
      for (z = 0; z < GxTexFormats_Last; ++z) {
        CGxTexCache *gxTexCache;

        while ((gxTexCache = s_gxTexCacheList[x][y][z].Head()) != 0) {
          if (gxTexCache->gxTex) {
            GxTexDestroy(gxTexCache->gxTex);
          }
          s_gxTexCacheList[x][y][z].DeleteNode(gxTexCache);
        }
      }
    }
  }
}

void TextureCacheUpdate(DWORD currentTime, CStatus *status) {
  UINT numTexturesFlushed = 0;

  while (CTextureItem *textureItem = s_textureCacheLRU.Head()) {
    if (numTexturesFlushed >= 4) {
      break;
    }

    if (static_cast<long>(currentTime - textureItem->timeStamp) < 30000L) {
      break;
    }

    s_textureCacheLRU.UnlinkNode(textureItem);

    if (TextureIsUsed(textureItem->texture)) {
      s_textureCacheLRU.LinkNode(textureItem, LIST_TAIL, 0);
      textureItem->timeStamp = currentTime;
    } else {
      HandleClose(textureItem->texture);
      textureItem->texture = 0;

      if (textureItem->fromColor) {
        s_solidTextureCache.DeleteNode(static_cast<CSolidTextureHash *>(textureItem));
      } else {
        s_textureCache.DeleteNode(static_cast<CTextureHash *>(textureItem));
      }

      ++numTexturesFlushed;
    }
  }

  if (numTexturesFlushed > 0 && status) {
    status->Add(STATUS_INFO, "%d texture(s) flushed from texture cache\n", numTexturesFlushed);
  }
}

static int TextureIsUsed(HTEXTURE texture) {
  CTexture *textureptr = reinterpret_cast<CTexture *>(texture);
  ASSERT(textureptr);
  return textureptr->GetRefCount() > 1;
}

MipBits *TextureLoadImage(LPCSTR filename, UINT *width, UINT *height, UINT *gxTexFormat, int *isOpaque, CStatus *status, UINT *alphaBits) {
  char         loadFileName[0x104];
  char        *ext;
  EGxTexFormat format;
  MipBits     *mipImages;

  VALIDATEBEGIN;
  VALIDATE(filename);
  VALIDATE(width);
  VALIDATE(height);
  VALIDATE(gxTexFormat);
  VALIDATEEND;

  mipImages = 0;
  format = GxTex_Argb8888;

  EImageFormat imageFormat = IdentifyAndStripFileExtension(filename, loadFileName, &ext);

  for (UINT i = 0; i < NUM_IMAGE_FORMATS; ++i) {
    SStrCopy(ext, s_formatExt[imageFormat], 0x7FFFFFFF);

    switch (imageFormat) {
      case IMAGE_FORMAT_TGA:
        mipImages = LoadTgaMips(loadFileName, width, height, &format, isOpaque, alphaBits);
        break;

      case IMAGE_FORMAT_BLP:
        LoadBlpMips(loadFileName, mipImages, width, height, &format, isOpaque, alphaBits);
        break;
    }

    if (mipImages) {
      break;
    }

    imageFormat = static_cast<EImageFormat>((imageFormat + 1) % NUM_IMAGE_FORMATS);
  }

  if (!mipImages && status) {
    status->Add(
        STATUS_FATAL,
        "Error loading texure file \"%s\": "
        "unsupported image format\n",
        filename
    );
  }

  *gxTexFormat = format;
  return mipImages;
}

static MipBits *LoadTgaMips(LPCSTR fileName, UINT *width, UINT *height, EGxTexFormat *format, int *isOpaque, UINT *alphaBits) {
  char mipFileMask[0x104];

  ASSERT(fileName);

  CTgaFile texFile;
  if (!texFile.Open(fileName)) {
    return 0;
  }

  if (isOpaque) {
    *isOpaque = texFile.AlphaBits() == 0;
  }

  if (!texFile.LoadImageData(3)) {
    return 0;
  }

  texFile.SetTopDown(1);

  UINT     imageWidth = texFile.Width();
  UINT     imageHeight = texFile.Height();
  UINT     levels = TextureCalcMipCount(imageWidth, imageHeight);
  MipBits *buffer;
  buffer = TextureAllocMippedImg(GxTex_Argb8888, imageWidth, imageHeight);
  TGA32Pixel *source = texFile.ImageTGA32Pixel();
  C4Pixel    *dst = buffer->mip[0];
  UINT        pixelCount = imageHeight * imageWidth;

  while (pixelCount) {
    (dst++)->C4Pixel::C4Pixel(*source++);
    --pixelCount;
  }

  texFile.Close();
  GenerateMipMask(fileName, mipFileMask);
  UINT levelsProvided = LoadPredrawnMips(texFile, mipFileMask, buffer);
  TextureGenerateMips(texFile.Width(), texFile.Height(), levelsProvided, levels, buffer);

  if (width) {
    *width = texFile.Width();
  }

  if (height) {
    *height = texFile.Height();
  }

  if (format) {
    *format = GxTex_Argb8888;
  }

  if (alphaBits) {
    *alphaBits = texFile.AlphaBits();
  }

  return buffer;
}

static UINT LoadPredrawnMips(const CTgaFile &mipZero, LPCSTR filemask, MipBits *buffer) {
  char pathName[0x104];

  ASSERT(filemask);
  ASSERT(buffer);

  UINT index = 1;
  UINT width = mipZero.Width();
  UINT height = mipZero.Height();
  UINT levels = TextureCalcMipCount(width, height);
  ASSERT(levels > 0);

  --levels;
  width >>= 1;
  height >>= 1;

  while (levels) {
    --levels;
    SStrPrintf(pathName, sizeof(pathName), filemask, index);
    if (!SFile::FileExists(pathName)) {
      break;
    }

    CTgaFile mipTga;
    if (!mipTga.Open(pathName) || width != mipTga.Width() || height != mipTga.Height() || !mipTga.LoadImageData(2)) {
      return index;
    }

    if (!mipTga.AlphaBits()) {
      mipTga.AddAlphaChannel(0);
    }

    mipTga.SetTopDown(1);
    TGA32Pixel *source = mipTga.ImageTGA32Pixel();
    C4Pixel    *dest = buffer->mip[index];
    UINT        pixelCount = height * width;
    ++index;

    while (pixelCount) {
      (dest++)->C4Pixel::C4Pixel(*source++);
      --pixelCount;
    }

    if (width > 1) {
      width >>= 1;
    }

    if (height > 1) {
      height >>= 1;
    }
  }

  return index;
}

static void GenerateMipMask(LPCSTR mipZeroName, char *mipMask) {
  SStrCopy(mipMask, mipZeroName, 0x104);
  RemoveExtension(mipMask);
  SStrPack(mipMask, "_mip%d.tga", 0x104);
}

static void RemoveExtension(char *path) {
  char *extension = SStrChrR(path, '.');

  if (extension) {
    *extension = 0;
  }
}

static EImageFormat IdentifyAndStripFileExtension(LPCSTR fileName, char *stripped, char **ext) {
  EImageFormat imageFormat = IMAGE_FORMAT_BLP;
  UINT         length = SStrCopy(stripped, fileName, 0x104);

  if (length >= 4 && stripped[length - 4] == '.') {
    length -= 4;
  }

  *ext = &stripped[length];
  if (**ext == '.') {
    SStrLower(*ext + 1);

    switch ((*ext)[3]) {
      case 'a':
        if ((*ext)[1] == 't' && (*ext)[2] == 'g') {
          imageFormat = IMAGE_FORMAT_TGA;
        }
        break;
      case 'p':
        if ((*ext)[1] == 'b' && (*ext)[2] == 'l') {
          imageFormat = IMAGE_FORMAT_BLP;
        }
        break;
    }

    **ext = 0;
  }

  return imageFormat;
}

HTEXTURE TextureLoadImage(LPCSTR filename) {
  char   loadFileName[0x104];
  char  *ext;
  SFile *file;

  VALIDATEBEGIN;
  VALIDATE(filename);
  VALIDATEEND;

  IdentifyAndStripFileExtension(filename, loadFileName, &ext);
  SStrCopy(ext, s_formatExt[IMAGE_FORMAT_BLP], 0x7FFFFFFF);

  if (!SFile::Open(loadFileName, &file)) {
    return 0;
  }

  CTexture *texture = NEWHANDLE(HTEXTURE, CTexture);
  ASSERT(texture);

  SStrCopy(texture->filename, loadFileName, sizeof(texture->filename));
  texture->mipBits = 0;
  texture->asyncObject = AsyncFileReadCreateObject();
  ASSERT(texture->asyncObject);

  texture->asyncObject->userArg = texture;
  texture->asyncObject->userPostloadCallback = AsyncTextureLoadImageCallback;
  texture->asyncObject->file = file;
  texture->asyncObject->offset = 0;
  texture->asyncObject->size = SFile::GetFileSize(file, 0);

  UINT bufferRemaining = s_asyncLoadBuffer.MaxCount() - s_asyncLoadBufferUsed;
  if (bufferRemaining >= texture->asyncObject->size) {
    texture->asyncObject->buffer = &s_asyncLoadBuffer[s_asyncLoadBufferUsed];
    texture->asyncObject->canReorder = 1;
    s_asyncLoadBufferUsed += texture->asyncObject->size;
    ++s_asyncPending;
    AsyncFileReadObject(texture->asyncObject);
  } else {
    texture->asyncObject->canReorder = 0;
    s_asyncLoadList.LinkNode(texture->asyncObject, LIST_TAIL, 0);
  }

  return CREATEHANDLE(HTEXTURE, texture);
}

static void AsyncTextureLoadImageCallback(LPVOID arg) {
  CTexture *texture = static_cast<CTexture *>(arg);

  ASSERT(texture);
  AsyncTextureLoadImageCreate(texture);
  SFile::Close(texture->asyncObject->file);
  texture->asyncObject->file = 0;
  AsyncFileReadDestroyObject(texture->asyncObject);
  texture->asyncObject = 0;
  --s_asyncPending;
}

static BOOL AsyncTextureLoadImageCreate(CTexture *texture) {
  CBLPFile image;

  if (!image.Source(texture->asyncObject->buffer)) {
    return 0;
  }

  texture->alphaBits = static_cast<WORD>(image.AlphaBits());
  if (!texture->alphaBits) {
    texture->flags |= 1;
  }

  UINT bestMip = 0;
  UINT width = image.Width();
  UINT height = image.Height();
  RequestImageDimensions(&width, &height, &bestMip);

  texture->mipBits = 0;
  if (!image.LockChain2(PIXEL_ARGB8888, texture->mipBits, bestMip)) {
    return 0;
  }

  texture->gxWidth = width;
  texture->gxHeight = height;
  texture->gxTexFormat = GxTex_Argb8888;
  texture->dataFormat = GxTex_Argb8888;
  texture->gxTex = 0;

  if (image.HasMips() == MIPS_GENERATED) {
    texture->gxTexFlags.m_generateMipMaps = 1;
  }

  return 1;
}

MipBits *TextureLoadImage(HTEXTURE texture, UINT *width, UINT *height, UINT *gxTexFormat, CStatus *status, UINT *alphaBits) {
  CTexture *textureptr = reinterpret_cast<CTexture *>(texture);
  int       isOpaque;

  VALIDATEBEGIN;
  VALIDATE(textureptr);
  VALIDATEEND;

  return TextureLoadImage(textureptr->filename, width, height, gxTexFormat, &isOpaque, status, alphaBits);
}

HTEXTURE TextureAllocImage(EGxTexFormat format, UINT width, UINT height) {
  CTexture *texture = NEWHANDLE(HTEXTURE, CTexture);
  ASSERT(texture);

  texture->gxTex = 0;
  texture->gxTexFormat = format;
  texture->gxWidth = width;
  texture->gxHeight = height;
  texture->pixBitDepth = s_bitDepth[format];
  texture->asyncObject = 0;
  texture->mipBits = TextureAllocMippedImg(format, width, height);
  SStrCopy(texture->filename, "TextureAllocImage", sizeof(texture->filename));

  return CREATEHANDLE(HTEXTURE, texture);
}

void TextureUnloadImage(MipBits *image) {
  FREEIFUSED(image);
}

HTEXTURE TextureCreate(UINT width, UINT height, EGxTexFormat format, CGxTexFlags flags) {
  CTexture *texture = NEWHANDLE(HTEXTURE, CTexture);
  ASSERT(texture);

  texture->gxTex = TextureAllocGxTex(width, height, format, flags, texture, UpdateTextureDefault, GxTex_Argb8888);
  ASSERT(texture->gxTex);

  texture->gxTexFormat = format;
  texture->gxWidth = width;
  texture->gxHeight = height;
  texture->gxTexFlags = flags;
  texture->pixBitDepth = s_bitDepth[format];
  texture->asyncObject = 0;
  SStrCopy(texture->filename, "unique_texture", sizeof(texture->filename));

  return CREATEHANDLE(HTEXTURE, texture);
}

static CGxTex *TextureAllocGxTex(
    UINT         width,
    UINT         height,
    EGxTexFormat format,
    CGxTexFlags  flags,
    LPVOID       userArg,
    void (*userFunc)(EGxTexCommand, UINT, UINT, UINT, UINT, LPVOID, UINT &, LPCVOID &),
    EGxTexFormat dataFormat
) {
  ASSERT((width & (width - 1)) == 0);
  ASSERT((height & (height - 1)) == 0);

  CGxTexParmsEx gxTexParmsEx;
  gxTexParmsEx.target = GxTex_2d;
  gxTexParmsEx.depth = 0;
  gxTexParmsEx.width = width;
  gxTexParmsEx.height = height;
  gxTexParmsEx.format = format;
  gxTexParmsEx.dataFormat = dataFormat;
  gxTexParmsEx.flags = flags;
  gxTexParmsEx.userArg = userArg;
  gxTexParmsEx.userFunc = userFunc;

  ASSERT(height < 1024);
  ASSERT(width < 1024);

  if (width > 16 && height > 16 && format <= GxTex_Dxt5) {
    width >>= 5;
    height >>= 5;

    UINT indexW = 0;
    while (!(width & 1)) {
      width >>= 1;
      ++indexW;
    }

    UINT indexH = 0;
    while (!(height & 1)) {
      height >>= 1;
      ++indexH;
    }

    CGxTexParmsEx gxTexParmsEx2;
    CGxTexCache  *gxTexCache = s_gxTexCacheList[indexW][indexH][gxTexParmsEx.format].Head();
    while (gxTexCache) {
      GxTexParametersEx(gxTexCache->gxTex, gxTexParmsEx2);
      ASSERT(gxTexParmsEx2.width == gxTexParmsEx.width);
      ASSERT(gxTexParmsEx2.height == gxTexParmsEx.height);

      if (gxTexParmsEx2.flags.m_filter == gxTexParmsEx.flags.m_filter) {
        gxTexCache->link.Unlink();
        s_gxTexCacheFreeList.LinkNode(gxTexCache, LIST_HEAD, 0);
        GxTexSetDataFormat(gxTexCache->gxTex, gxTexParmsEx.dataFormat);
        GxTexSetUserData(gxTexCache->gxTex, gxTexParmsEx.userFunc, gxTexParmsEx.userArg);
        GxTexSetFlags(gxTexCache->gxTex, gxTexParmsEx.flags);
        return gxTexCache->gxTex;
      }

      gxTexCache = gxTexCache->link.Next();
    }
  }

  CGxTex *gxTex = 0;
  int     ret = GxTexCreate(gxTexParmsEx, gxTex);
  ASSERT(ret);
  return gxTex;
}

HTEXTURE TextureCreate(LPCSTR name, UINT width, UINT height, EGxTexFormat format, CGxTexFlags flags) {
  CTexture *texture = NEWHANDLE(HTEXTURE, CTexture);
  ASSERT(texture);

  texture->gxTex = TextureAllocGxTex(width, height, format, flags, texture, UpdateTextureDefault, GxTex_Argb8888);
  ASSERT(texture->gxTex);

  texture->gxTexFormat = format;
  texture->gxWidth = width;
  texture->gxHeight = height;
  texture->gxTexFlags = flags;
  texture->pixBitDepth = s_bitDepth[format];
  texture->asyncObject = 0;
  SStrCopy(texture->filename, name, sizeof(texture->filename));

  return CREATEHANDLE(HTEXTURE, texture);
}

HTEXTURE
TextureCreate(LPCSTR name, UINT width, UINT height, EGxTexFormat format, EGxTexFormat dataFormat, CGxTexFlags flags) {
  CTexture *texture = NEWHANDLE(HTEXTURE, CTexture);
  ASSERT(texture);

  texture->gxTex = TextureAllocGxTex(width, height, format, flags, texture, UpdateTextureDefault, dataFormat);
  ASSERT(texture->gxTex);

  texture->gxTexFormat = format;
  texture->dataFormat = dataFormat;
  texture->gxWidth = width;
  texture->gxHeight = height;
  texture->gxTexFlags = flags;
  texture->pixBitDepth = s_bitDepth[format];
  texture->asyncObject = 0;
  SStrCopy(texture->filename, name, sizeof(texture->filename));

  return CREATEHANDLE(HTEXTURE, texture);
}

HTEXTURE TextureCreate(LPCSTR fileName, CGxTexFlags flags, CStatus *status, int dontCache) {
  char  loadFileName[0x104];
  char *ext;

  VALIDATEBEGIN;
  VALIDATE(fileName);
  VALIDATE(fileName[0]);
  VALIDATE(status);
  VALIDATEEND;

  EImageFormat imageFormat = IdentifyAndStripFileExtension(fileName, loadFileName, &ext);

  if (!dontCache) {
    HTEXTURE texture = GetTexture(loadFileName, flags);
    if (texture) {
      return texture;
    }
  } else {
    BaseFileRegisterUncachable(fileName);
  }

  HTEXTURE texture = 0;
  for (UINT i = 0; i < NUM_IMAGE_FORMATS; ++i) {
    SStrCopy(ext, s_formatExt[imageFormat], 0x7FFFFFFF);

    switch (imageFormat) {
      case IMAGE_FORMAT_TGA:
        texture = CreateTgaTexture(loadFileName, flags, status);
        break;

      case IMAGE_FORMAT_BLP:
        texture = CreateBlpTexture(loadFileName, flags, status);
        break;
    }

    if (texture) {
      break;
    }

    imageFormat = static_cast<EImageFormat>((imageFormat + 1) % NUM_IMAGE_FORMATS);
  }

  if (texture) {
    if (!dontCache) {
      *ext = 0;
      HashNewTexture(loadFileName, flags, texture, status);
    }

    return texture;
  }

  FileError(status, "texture", fileName);
  return TextureCreateSolid(CRAPPY_GREEN, 0);
}

static HTEXTURE GetTexture(LPCSTR texMap, CGxTexFlags flags) {
  ASSERT(texMap);

  UINT                hash = SStrHash(texMap, 0, 0);
  HASHKEY_TEXTUREFILE hashKey(texMap, flags);
  CTextureHash       *textureHash = s_textureCache.Ptr(hash, hashKey);

  return textureHash ? reinterpret_cast<HTEXTURE>(HandleDuplicate(textureHash->texture)) : 0;
}

static void HashNewTexture(LPCSTR texMap, CGxTexFlags flags, HTEXTURE texture, CStatus *status) {
  DWORD         currentTime;
  UINT          hash;
  CTextureHash *textureHash;

  ASSERT(texMap);

  currentTime = OsGetAsyncTimeMs();
  TextureCacheUpdate(currentTime, status);
  hash = SStrHash(texMap, 0, 0);
  HASHKEY_TEXTUREFILE hashKey(texMap, flags);
  textureHash = s_textureCache.New(hash, hashKey, 0, 0);
  s_textureCacheLRU.LinkNode(textureHash, LIST_TAIL, 0);
  textureHash->texture = reinterpret_cast<HTEXTURE>(HandleDuplicate(texture));
  textureHash->timeStamp = currentTime;
}

static void FileError(CStatus *status, LPCSTR description, LPCSTR path) {
  char error[0x100];

  SErrGetErrorStr(SErrGetLastError(), error, sizeof(error));
  status->Add(STATUS_FATAL, "Error loading %s file \"%s\": %s\n", description, path, error);
  SErrSetLastError(0);
}

static HTEXTURE CreateTgaTexture(LPCSTR file, CGxTexFlags flags, CStatus *status) {
  CTgaFile image;

  if (!image.Open(file)) {
    return 0;
  }

  CTexture *texture = NEWHANDLE(HTEXTURE, CTexture);
  if (!texture) {
    return 0;
  }

  texture->alphaBits = image.AlphaBits();
  if (!texture->alphaBits) {
    texture->flags |= 1;
  }

  SStrCopy(texture->filename, file, sizeof(texture->filename));
  texture->gxTex = TextureAllocGxTex(image.Width(), image.Height(), GxTex_Argb8888, flags, texture, UpdateTgaTexture, GxTex_Argb8888);
  ASSERT(texture->gxTex);

  HTEXTURE handle = CREATEHANDLE(HTEXTURE, texture);
  return handle;
}

static void UpdateTgaTexture(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &texels) {
  CTexture *texture = static_cast<CTexture *>(userArg);

  switch (cmd) {
    case GxTex_Lock:
      ASSERT(texture);
      tgaMips = LoadTgaMips(texture->filename, 0, 0, 0, 0, 0);
      if (!tgaMips) {
        tgaMips = GetDefaultTexture(w, h, GxTex_Argb8888);
        GetGlobalStatusObj().Add(STATUS_ERROR, "Texture %s not loaded -- replaced with default.\n", texture->filename);
        ASSERT(tgaMips);
      }
      break;

    case GxTex_Latch:
      texelStrideInBytes = 4 * w;
      texels = tgaMips->mip[mipLevel];
      break;

    case GxTex_Unlock:
      TextureFreeMippedImg(tgaMips);
      break;
  }
}

static HTEXTURE CreateBlpTexture(LPCSTR filename, CGxTexFlags flags, CStatus *status) {
  SFile *file;

  if (!SFile::Open(filename, &file)) {
    return 0;
  }

  CTexture *texture = NEWHANDLE(HTEXTURE, CTexture);
  ASSERT(texture);

  texture->gxTexFlags = flags;
  SStrCopy(texture->filename, filename, sizeof(texture->filename));
  texture->asyncObject = AsyncFileReadCreateObject();
  ASSERT(texture->asyncObject);

  texture->asyncObject->userArg = texture;
  texture->asyncObject->userPostloadCallback = AsyncCreateBlpTextureCallback;
  texture->asyncObject->file = file;
  texture->asyncObject->offset = 0;
  texture->asyncObject->size = SFile::GetFileSize(file, 0);

  UINT bufferRemaining = s_asyncLoadBuffer.MaxCount() - s_asyncLoadBufferUsed;
  if (bufferRemaining >= texture->asyncObject->size) {
    texture->asyncObject->buffer = &s_asyncLoadBuffer[s_asyncLoadBufferUsed];
    texture->asyncObject->canReorder = 1;
    s_asyncLoadBufferUsed += texture->asyncObject->size;
    ++s_asyncPending;
    AsyncFileReadObject(texture->asyncObject);
  } else {
    texture->asyncObject->canReorder = 0;
    s_asyncLoadList.LinkNode(texture->asyncObject, LIST_TAIL, 0);
  }

  return CREATEHANDLE(HTEXTURE, texture);
}

static void AsyncCreateBlpTextureCallback(LPVOID arg) {
  CTexture *texture = static_cast<CTexture *>(arg);

  ASSERT(texture);
  if (!PumpBlpTextureAsync(texture)) {
    FillInSolidTexture(CRAPPY_GREEN, texture);
  }

  SFile::Close(texture->asyncObject->file);
  texture->asyncObject->file = 0;
  AsyncFileReadDestroyObject(texture->asyncObject);
  texture->asyncObject = 0;
  --s_asyncPending;
}

static BOOL PumpBlpTextureAsync(CTexture *texture) {
  CBLPFile image;

  if (!image.Source(texture->asyncObject->buffer)) {
    texture->loadStatus.Add(STATUS_FATAL, "Texture failure: \"%s\" invalid file version\n", texture->filename);
    return 0;
  }

  texture->alphaBits = static_cast<WORD>(image.m_header.alphaSize);
  if (!texture->alphaBits) {
    texture->flags |= 1;
  }

  UINT         bestMip = 0;
  UINT         width = image.m_header.width;
  UINT         height = image.m_header.height;
  PIXEL_FORMAT pixFormat;
  EGxTexFormat dataFormat;
  EGxTexFormat gxTexFormat;

  RequestImageDimensions(&width, &height, &bestMip);
  GetTextureFormats(texture, pixFormat, dataFormat, gxTexFormat, static_cast<PIXEL_FORMAT>(image.m_header.preferredFormat), image.m_header.alphaSize);

  texture->mipBits = static_cast<MipBits *>(g_textureMipBits);
  if (!image.LockChain2(pixFormat, texture->mipBits, bestMip)) {
    texture->loadStatus.Add(STATUS_FATAL, "Texture failure: \"%s\" decompression failed.\n", texture->filename);
    return 0;
  }

  texture->gxWidth = width;
  texture->gxHeight = height;
  texture->gxTexFormat = gxTexFormat;
  texture->dataFormat = dataFormat;

  if ((dataFormat < GxTex_Dxt1 || dataFormat > GxTex_Dxt5) && GxCaps().m_generateMipMaps && image.HasMips() == MIPS_GENERATED) {
    texture->gxTexFlags.m_generateMipMaps = 1;
  }

  texture->gxTex = TextureAllocGxTex(width, height, gxTexFormat, texture->gxTexFlags, texture, UpdateBlpTextureAsync, dataFormat);
  ASSERT(texture->gxTex);
  GxTexUpdate(texture->gxTex, 0, 0, width, height, 1);
  texture->mipBits = 0;
  return 1;
}

static void GetTextureFormats(
    CTexture          *texture,
    PIXEL_FORMAT      &pixFormat,
    EGxTexFormat      &dataFormat,
    EGxTexFormat      &gxTexFormat,
    const PIXEL_FORMAT preferredFormat,
    const UINT         alphaBits
) {
  pixFormat = PIXEL_ARGB8888;
  dataFormat = GxTex_Argb8888;
  texture->pixBitDepth = 32;
  gxTexFormat = GxTex_Argb8888;

  switch (preferredFormat) {
    case PIXEL_UNSPECIFIED:
      switch (alphaBits) {
        case 0:
          gxTexFormat = GxTex_Rgb565;
          break;

        case 1:
          gxTexFormat = GxTex_Argb1555;
          break;

        case 4:
          gxTexFormat = GxTex_Argb4444;
          break;

        default:
          gxTexFormat = GxTex_Argb8888;
          break;
      }
      break;

    case PIXEL_ARGB8888:
      gxTexFormat = GxTex_Argb8888;
      break;

    case PIXEL_ARGB1555:
      gxTexFormat = GxTex_Argb1555;
      break;

    case PIXEL_ARGB4444:
      gxTexFormat = GxTex_Argb4444;
      break;

    case PIXEL_RGB565:
      gxTexFormat = GxTex_Rgb565;
      break;

    case PIXEL_DXT1:
      if (GxCaps().m_texFmtDxt) {
        gxTexFormat = GxTex_Dxt1;
        dataFormat = GxTex_Dxt1;
        texture->pixBitDepth = 4;
        pixFormat = PIXEL_DXT1;
      } else if (alphaBits == 0) {
        dataFormat = GxTex_Rgb565;
        gxTexFormat = GxTex_Rgb565;
        texture->pixBitDepth = 16;
        pixFormat = PIXEL_RGB565;
      } else {
        dataFormat = GxTex_Argb1555;
        gxTexFormat = GxTex_Argb1555;
        texture->pixBitDepth = 16;
        pixFormat = PIXEL_ARGB1555;
      }
      break;

    case PIXEL_DXT3:
      if (GxCaps().m_texFmtDxt) {
        gxTexFormat = GxTex_Dxt3;
        dataFormat = GxTex_Dxt3;
        texture->pixBitDepth = 8;
        pixFormat = PIXEL_DXT3;
      } else {
        dataFormat = GxTex_Argb4444;
        gxTexFormat = GxTex_Argb4444;
        texture->pixBitDepth = 16;
        pixFormat = PIXEL_ARGB4444;
      }
      break;

    case PIXEL_DXT5:
      if (GxCaps().m_texFmtDxt) {
        gxTexFormat = GxTex_Dxt5;
        dataFormat = GxTex_Dxt5;
        texture->pixBitDepth = 8;
        pixFormat = PIXEL_DXT5;
      } else {
        dataFormat = GxTex_Argb4444;
        gxTexFormat = GxTex_Argb4444;
        texture->pixBitDepth = 16;
        pixFormat = PIXEL_ARGB4444;
      }
      break;

    default:
      SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "1", FALSE);
      break;
  }
}

static void FillInSolidTexture(const NTempest::CImVector &color, CTexture *texture) {
  GxTexCreate(
      8, 8, GxTex_Argb8888, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), reinterpret_cast<LPVOID>(static_cast<DWORD>(color)), GxuUpdateSingleColorTexture,
      texture->gxTex
  );

  if (color.a >= 0xFE) {
    texture->flags |= 1;
  } else {
    texture->flags &= ~1U;
  }
}

HTEXTURE TextureCreate(CGxTex *gxTex) {
  VALIDATEBEGIN;
  VALIDATE(gxTex);
  VALIDATEEND;

  CTexture *texture = NEWHANDLE(HTEXTURE, CTexture);
  if (!texture) {
    return 0;
  }

  texture->gxTex = gxTex;
  SStrCopy(texture->filename, "unique_texture", sizeof(texture->filename));
  return CREATEHANDLE(HTEXTURE, texture);
}

HTEXTURE TextureCreateSolid(const NTempest::CImVector &color, CStatus *status) {
  HTEXTURE texture = GetTexture(color);

  if (texture) {
    return texture;
  }

  CTexture *textureObject = NEWHANDLE(HTEXTURE, CTexture);

  if (!textureObject) {
    return 0;
  }

  FillInSolidTexture(color, textureObject);
  texture = CREATEHANDLE(HTEXTURE, textureObject);
  HashNewTexture(color, texture, status);
  return texture;
}

static HTEXTURE GetTexture(const NTempest::CImVector &color) {
  CSolidTextureHash *textureHash = s_solidTextureCache.Ptr(*color.IV_(), s_hashKeyNone);

  if (!textureHash) {
    return 0;
  }

  return reinterpret_cast<HTEXTURE>(HandleDuplicate(textureHash->texture));
}

static void HashNewTexture(const NTempest::CImVector &color, HTEXTURE texture, CStatus *status) {
  DWORD              currentTime = OsGetAsyncTimeMs();
  CSolidTextureHash *textureHash;

  TextureCacheUpdate(currentTime, status);
  textureHash = s_solidTextureCache.New(*color.IV_(), s_hashKeyNone, 0, 0);
  s_textureCacheLRU.LinkNode(textureHash, LIST_TAIL, 0);
  textureHash->texture = reinterpret_cast<HTEXTURE>(HandleDuplicate(texture));
  textureHash->timeStamp = currentTime;
}

CGxTex *TextureGetGxTex(HTEXTURE texture, int force, CStatus *status) {
  CTexture *textureptr = reinterpret_cast<CTexture *>(texture);

  VALIDATEBEGIN;
  VALIDATE(textureptr);
  VALIDATEEND;

  if (!textureptr->gxTex) {
    if (!force) {
      return 0;
    }

    AsyncTextureWait(textureptr);
    if (status) {
      status->Add(textureptr->loadStatus);
    }
  }

  return textureptr->gxTex;
}

static void AsyncTextureWait(CTexture *texture) {
  ASSERT(texture);

  if (texture->asyncObject) {
    AsyncFileReadWait(texture->asyncObject);
  }
}

MipBits *TextureGetMips(HTEXTURE texture, int force) {
  CTexture *textureptr = reinterpret_cast<CTexture *>(texture);

  VALIDATEBEGIN;
  VALIDATE(textureptr);
  VALIDATEEND;

  if (!textureptr->mipBits) {
    if (!force) {
      return 0;
    }

    OsOutputDebugString("AsyncTextureWait: %s\n", textureptr->filename);
    AsyncTextureWait(textureptr);
  }

  return textureptr->mipBits;
}

int TextureIsOpaque(HTEXTURE__ *texture) {
  CTexture *textureptr = reinterpret_cast<CTexture *>(texture);

  VALIDATEBEGIN;
  VALIDATE(textureptr);
  VALIDATEEND;

  return textureptr->flags & 1;
}

UINT TextureCalcMipCount(UINT width, UINT height) {
  UINT mipCount = 1;

  while (width > 1 || height > 1) {
    width >>= 1;
    ++mipCount;

    if (width < 1) {
      width = 1;
    }

    height >>= 1;

    if (height < 1) {
      height = 1;
    }
  }

  return mipCount;
}

void TextureGenerateMips(UINT width, UINT height, UINT levelsProvided, UINT levelsDesired, MipBits *levelBits) {
  UINT destWidth = width;
  UINT destHeight = height;
  UINT last_good_level = 0;

  if (levelsProvided) {
    last_good_level = levelsProvided - 1;
    width >>= last_good_level;
    height >>= last_good_level;
  }

  for (UINT level = 1; level < levelsDesired; ++level) {
    destWidth = max(1, destWidth >> 1);
    destHeight = max(1, destHeight >> 1);

    if (level >= levelsProvided) {
      FullShrink(levelBits->mip[level], destWidth, destHeight, levelBits->mip[last_good_level], width, height);
    }
  }
}

MipBits *TextureAllocMippedImg(EGxTexFormat format, UINT width, UINT height) {
  UINT  mipCount;
  UINT  levelDataSize;
  UINT *ptr;
  UINT  offset;
  UINT  level;

  mipCount = TextureCalcMipCount(width, height);
  levelDataSize = CalcLevelOffset(mipCount, width, height, format);
  ptr = static_cast<UINT *>(ALLOC(levelDataSize + 4 * mipCount));
  offset = 0;

  for (level = 0; level < mipCount; ++level) {
    reinterpret_cast<MipBits *>(ptr)->mip[level] = reinterpret_cast<C4Pixel *>(reinterpret_cast<BYTE *>(&ptr[mipCount]) + offset);
    offset += CalcLevelSize(level, width, height, format);
  }

  ASSERT(offset == levelDataSize);
  return reinterpret_cast<MipBits *>(ptr);
}

static UINT CalcLevelSize(UINT level, UINT width, UINT height, EGxTexFormat format) {
  width = max(1, width >> level);
  height = max(1, height >> level);

  return (width * height * s_bitDepth[format]) >> 3;
}

static UINT CalcLevelOffset(UINT level, UINT width, UINT height, EGxTexFormat format) {
  UINT offset = 0;
  UINT index;

  for (index = 0; index < level; ++index) {
    offset += CalcLevelSize(index, width, height, format);
  }

  return offset;
}

MipBits *TextureCopyMippedImage(MipBits *srcData, EGxTexFormat format, UINT width, UINT height) {
  if (!srcData) {
    return 0;
  }

  MipBits *destData = TextureAllocMippedImg(format, width, height);
  if (!destData) {
    return 0;
  }

  UINT mipCount = TextureCalcMipCount(width, height);
  memcpy(&destData->mip[mipCount], &srcData->mip[mipCount], CalcLevelOffset(mipCount, width, height, format));
  return destData;
}

void TextureFreeMippedImg(MipBits *image) {
  FREEIFUSED(image);
}

TEXFILETYPE TextureDiscoverFileType(LPCSTR path) {
  LPCSTR extension = SStrChrR(path, '.');

  if (!extension || SStrLen(extension) != 4) {
    return TEXFILETYPE_UNKNOWN;
  }

  if (!SStrCmpI(extension, ".TGA", 0x7FFFFFFF)) {
    return TEXFILETYPE_TGA;
  }

  if (!SStrCmpI(extension, ".BLP", 0x7FFFFFFF)) {
    return TEXFILETYPE_BLP;
  }

  return TEXFILETYPE_UNKNOWN;
}

UINT TexturePickAlternateFilename(LPCSTR path, TEXFILETYPE fileType, char *newpath, UINT size) {
  char *extension;

  if (path != newpath) {
    SStrCopy(newpath, path, size);
  }

  if (fileType == TEXFILETYPE_UNKNOWN) {
    return TEXFILETYPE_UNKNOWN;
  }

  extension = SStrChrR(newpath, '.');
  if (extension) {
    *extension = 0;
  }

  switch (fileType) {
    case TEXFILETYPE_TGA:
      SStrPack(newpath, ".BLP", size);
      fileType = TEXFILETYPE_BLP;
      break;

    case TEXFILETYPE_BLP:
      SStrPack(newpath, ".TGA", size);
      fileType = TEXFILETYPE_TGA;
      break;
  }

  return fileType;
}

DWORD TextureGetUniqueID(HTEXTURE__ *texture) {
  return reinterpret_cast<DWORD>(texture);
}

void TextureGetDimensions(HTEXTURE texture, UINT *width, UINT *height) {
  if (width) {
    *width = reinterpret_cast<CTexture *>(texture)->gxWidth;
  }
  if (height) {
    *height = reinterpret_cast<CTexture *>(texture)->gxHeight;
  }
}

void TextureDestroy() {
  CGxTexCache *gxTexCache;

  TextureCacheFlush();
  TextureGxCacheFlush();

  while ((gxTexCache = s_gxTexCacheFreeList.Head()) != 0) {
    if (gxTexCache->gxTex) {
      GxTexDestroy(gxTexCache->gxTex);
    }
    s_gxTexCacheFreeList.DeleteNode(gxTexCache);
  }

  SMemFree(g_textureMipBits);
  g_textureMipBits = 0;
}

LPCSTR TextureGetFilename(HTEXTURE texture) {
  CTexture *textureptr = reinterpret_cast<CTexture *>(texture);

  VALIDATEBEGIN;
  VALIDATE(textureptr);
  VALIDATEEND;

  return textureptr->filename;
}

BOOL TextureGetInfo(HTEXTURE texture, UINT &width, UINT &height, EGxTexFormat &format, int &opaque, UINT &alphaBits, BOOL bForce) {
  CTexture *textureObject = reinterpret_cast<CTexture *>(texture);

  if (!textureObject->mipBits) {
    if (!bForce) {
      return 0;
    }

    OsOutputDebugString("AsyncTextureWait: %s\n", textureObject->filename);
    AsyncTextureWait(textureObject);
  }

  width = textureObject->gxWidth;
  height = textureObject->gxHeight;
  format = textureObject->dataFormat;
  opaque = textureObject->flags & 1;
  alphaBits = textureObject->alphaBits;
  return 1;
}

void TextureLogGxCache(HSLOG log) {
  ASSERT(log);

  CGxTexParmsEx gxTexParmsEx;
  for (UINT x = 0; x < 5; ++x) {
    for (UINT y = 0; y < 5; ++y) {
      for (UINT format = 0; format < GxTexFormats_Last; ++format) {
        LISTEX(CGxTexCache, link) &cacheList = s_gxTexCacheList[x][y][format];
        ITERATELIST(CGxTexCache, cacheList, gxTexCache) {
          GxTexParametersEx(gxTexCache->gxTex, gxTexParmsEx);
          SLogWrite(
              log, "Format: %s Size: %dx%d Filter: %s", s_gxTexFormatStrings[gxTexParmsEx.format], gxTexParmsEx.width, gxTexParmsEx.height,
              s_gxTexFilterStrings[gxTexParmsEx.flags.m_filter]
          );
        }
      }
    }
  }
}

void TextureLogTextures(HSLOG log) {
  UINT i;
  UINT texTotal;
  UINT texTypeTotal[7];

  ASSERT(log);

  TSGrowableArray<CTexture *> textureSortList;
  ITERATELIST(CTexture, s_textureList, texture) {
    if (texture->filename[0]) {
      textureSortList.Add(&texture);
    }
  }

  qsort(textureSortList.Ptr(), textureSortList.Count(), sizeof(CTexture *), TextureLogSortCallback);

  texTotal = 0;
  memset(texTypeTotal, 0, sizeof(texTypeTotal));

  for (i = 0; i < textureSortList.Count(); ++i) {
    CTexture *texture = textureSortList[i];
    ASSERT(texture);

    SLogWrite(log, "%s : %dx%dx%dbit", texture->filename, texture->gxWidth, texture->gxHeight, texture->pixBitDepth);

    UINT bytesPerPixel = texture->pixBitDepth >> 3;
    UINT textureSize = texture->gxHeight * texture->gxWidth;
    if (!bytesPerPixel) {
      textureSize >>= 1;
    } else {
      textureSize *= bytesPerPixel;
    }

    if (texture->gxTexFlags.m_filter > GxTex_Linear) {
      textureSize = static_cast<UINT>(textureSize * 1.33f);
    }

    for (UINT type = 0; type < 7; ++type) {
      if (SStrStrI(texture->filename, s_textureLogString[type])) {
        texTypeTotal[type] += textureSize;
        break;
      }
    }

    texTotal += textureSize;
  }

  SLogWrite(log, "##############################################################");
  for (i = 0; i < 7; ++i) {
    SLogWrite(log, "%s Texture in Mbytes:\t\t%.2f", s_textureLogString[i], static_cast<float>(texTypeTotal[i]) / 1048576.0f);
  }
  SLogWrite(log, "##############################################################");
  SLogWrite(log, "Total Texture in Mbytes:\t\t%.2f", static_cast<float>(texTotal) / 1048576.0f);
  SLogWrite(log, "##############################################################");
}

static int __cdecl TextureLogSortCallback(LPCVOID elem1, LPCVOID elem2) {
  ASSERT(elem1);
  ASSERT(elem2);

  CTexture *const *texture1 = static_cast<CTexture *const *>(elem1);
  CTexture *const *texture2 = static_cast<CTexture *const *>(elem2);
  return SStrCmpI((*texture1)->filename, (*texture2)->filename, 0x7FFFFFFF);
}
