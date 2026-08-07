#include <new>

#include "Texture.h"

#include "AsyncFileRead.h"
#include "Base/FileCache.h"
#include "Base/Status.h"
#include "BLPFile/blp.h"
#include "Gx/CGxDevice.h"
#include "Gx/Gx.h"
#include "Images/dxt.h"
#include "Images/tga.h"
#include "Os/OsTime.h"
#include "Os/W32/Debugging.h"

#include <stpl.h>
#include <stdlib.h>
#include <string.h>

class HASHKEY_TEXTUREFILE {
 public:
  HASHKEY_TEXTUREFILE() : m_filename(0), m_flags(GxTex_Linear, 0, 0, 0, 0, 0, 1) {
  }

  HASHKEY_TEXTUREFILE(LPCSTR filename, CGxTexFlags flags) : m_filename(0), m_flags(GxTex_Linear, 0, 0, 0, 0, 0, 1) {
    m_filename = SStrDupA(filename, __FILE__, __LINE__);
    m_flags = flags;
  }

  HASHKEY_TEXTUREFILE(const HASHKEY_TEXTUREFILE &source) : m_filename(0), m_flags(GxTex_Linear, 0, 0, 0, 0, 0, 1) {
    m_filename = SStrDupA(source.m_filename, __FILE__, __LINE__);
    m_flags = source.m_flags;
  }

  ~HASHKEY_TEXTUREFILE() {
    FREEIFUSED(m_filename);
  }

  HASHKEY_TEXTUREFILE &operator=(const HASHKEY_TEXTUREFILE &source) {
    if (this != &source) {
      FREEIFUSED(m_filename);
      m_filename = SStrDupA(source.m_filename, __FILE__, __LINE__);
      m_flags = source.m_flags;
    }

    return *this;
  }

  bool operator==(const HASHKEY_TEXTUREFILE &source) const {
    return *reinterpret_cast<const UINT *>(&m_flags) == *reinterpret_cast<const UINT *>(&source.m_flags) &&
           SStrCmpI(m_filename, source.m_filename, 0x104) == 0;
  }

 private:
  char       *m_filename;
  CGxTexFlags m_flags;
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

  ~CTextureItem();

  HTEXTURE texture;
  int      fromColor;
  DWORD    timeStamp;
  LINKDECLEX(CTextureItem, link);
};

CTextureItem::~CTextureItem() {
  if (texture) {
    HandleClose(texture);
  }
}

struct CTextureHash : public TSHashObject<CTextureHash, HASHKEY_TEXTUREFILE>, public CTextureItem {
  CTextureHash();
};

struct CSolidTextureHash : public TSHashObject<CSolidTextureHash, HASHKEY_NONE>, public CTextureItem {
  CSolidTextureHash();
};

CTextureHash::CTextureHash() : CTextureItem(0) {
}

CSolidTextureHash::CSolidTextureHash() : CTextureItem(1) {
}

class CGxTexCache {
 public:
  CGxTexCache() : gxTex(0), timeStamp(0) {
  }

  ~CGxTexCache() {
  }

  CGxTex *gxTex;
  DWORD   timeStamp;
  LINKDECLEX(CGxTexCache, link);
};

enum EImageFormat {
  IMAGE_FORMAT_TGA = 0,
  IMAGE_FORMAT_BLP = 1,
  NUM_IMAGE_FORMATS = 2
};

static MipBits *tgaMips;
static LISTDECLEX(CTextureItem, link, s_textureCacheLRU);
static HASHKEY_NONE s_hashKeyNone;
static LISTDECLEX(CTexture, link, s_textureList);
static LPVOID              g_textureMipBits;
static NTempest::CImVector CRAPPY_GREEN(0xFF00FF00UL);
static LISTDECLEX(CGxTexCache, link, s_gxTexCacheList[5][5][GxTexFormats_Last]);
static TSHashTableReuse<CTextureHash, HASHKEY_TEXTUREFILE, 1> s_textureCache;
static const WORD                                             s_bitDepth[8] = {0, 32, 16, 16, 16, 4, 8, 8};
static LPCSTR                                                 s_formatExt[NUM_IMAGE_FORMATS] = {".tga", ".blp"};
static LISTDECLEX(CGxTexCache, link, s_gxTexCacheFreeList);
static TSCArray<BYTE, 1048576> s_asyncLoadBuffer;
static LISTDECLEX(CAsyncObject, link, s_asyncLoadList);
static UINT s_asyncLoadBufferUsed;
static char s_gxTexFormatStrings[8][32] = {"GxTex_Unknown", "GxTex_Argb8888", "GxTex_Argb4444", "GxTex_Argb1555",
                                           "GxTex_Rgb565",  "GxTex_Dxt1",     "GxTex_Dxt3",     "GxTex_Dxt5"};
static char s_gxTexFilterStrings[5][32] = {"GxTex_Nearest", "GxTex_Linear", "GxTex_LinearMipNearest", "GxTex_LinearMipLinear", ""};
char       *s_textureLogString[7] = {"character", "creature", "dungeon", "interface", "world", "tileset", "item"};
static int  s_asyncPending;
static TSHashTableReuse<CSolidTextureHash, HASHKEY_NONE, 1> s_solidTextureCache;

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
static void
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
static void         TextureGenerateMips(UINT width, UINT height, UINT levelsProvided, UINT levelsDesired, MipBits *levelBits);
static int __cdecl  TextureLogSortCallback(LPCVOID elem1, LPCVOID elem2);

LPCSTR CTexture::GetObjectName() {
  return filename;
}

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

static HTEXTURE GetTexture(LPCSTR texMap, CGxTexFlags flags) {
  CTextureHash *textureHash;

  ASSERT(texMap);

  HASHKEY_TEXTUREFILE hashKey(texMap, flags);
  textureHash = s_textureCache.Ptr(SStrHash(texMap, 0, 0), hashKey);
  if (!textureHash) {
    return 0;
  }

  return reinterpret_cast<HTEXTURE>(HandleDuplicate(textureHash->texture));
}

static HTEXTURE GetTexture(const NTempest::CImVector &color) {
  CSolidTextureHash *textureHash = s_solidTextureCache.Ptr(*color.IV_(), s_hashKeyNone);

  if (!textureHash) {
    return 0;
  }

  return reinterpret_cast<HTEXTURE>(HandleDuplicate(textureHash->texture));
}

static int TextureIsUsed(HTEXTURE texture) {
  ASSERT(texture);
  return texture->unused > 1;
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

static void HashNewTexture(const NTempest::CImVector &color, HTEXTURE texture, CStatus *status) {
  DWORD              currentTime = OsGetAsyncTimeMs();
  CSolidTextureHash *textureHash;

  TextureCacheUpdate(currentTime, status);
  textureHash = s_solidTextureCache.New(*color.IV_(), s_hashKeyNone, 0, 0);
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

static UINT LoadPredrawnMips(const CTgaFile &mipZero, LPCSTR filemask, MipBits *buffer) {
  char pathName[0x104];
  UINT width;
  UINT height;
  UINT levels;
  UINT index = 1;

  width = mipZero.Width();
  height = mipZero.Height();

  ASSERT(filemask);
  ASSERT(buffer);

  levels = TextureCalcMipCount(width, height);
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
    UINT        pixelCount = width * height;
    ++index;

    while (pixelCount) {
      dest->b = source->b;
      dest->g = source->g;
      dest->r = source->r;
      dest->a = source->a;
      ++source;
      ++dest;
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

static void RemoveExtension(char *path) {
  char *extension = SStrChrR(path, '.');

  if (extension) {
    *extension = 0;
  }
}

static void GenerateMipMask(LPCSTR mipZeroName, char *mipMask) {
  SStrCopy(mipMask, mipZeroName, 0x104);
  RemoveExtension(mipMask);
  SStrPack(mipMask, "_mip%d.tga", 0x104);
}

static UINT CalcLevelSize(UINT level, UINT width, UINT height, EGxTexFormat format) {
  UINT levelWidth = max(width >> level, 1U);
  UINT levelHeight = max(height >> level, 1U);

  return (levelWidth * levelHeight * s_bitDepth[format]) >> 3;
}

static UINT CalcLevelOffset(UINT level, UINT width, UINT height, EGxTexFormat format) {
  UINT offset = 0;
  UINT index;

  for (index = 0; index < level; ++index) {
    offset += CalcLevelSize(index, width, height, format);
  }

  return offset;
}

static MipBits *GetDefaultTexture(UINT height, UINT width, UINT format) {
  ASSERT(format == GxTex_Argb8888);

  MipBits *buffer = MippedImgAllocA(PIXEL_ARGB8888, width, height, __FILE__, __LINE__);
  MipBits *mip = buffer;

  while (width > 1 || height > 1) {
    memset(mip->mip, 0xFF, 4 * width * height);
    ++mip;

    width >>= 1;
    if (width < 1) {
      width = 1;
    }

    height >>= 1;
    if (height < 1) {
      height = 1;
    }
  }

  return buffer;
}

static MipBits *LoadTgaMips(LPCSTR fileName, UINT *width, UINT *height, EGxTexFormat *format, int *isOpaque, UINT *alphaBits) {
  char     mipFileMask[0x104];
  CTgaFile texFile;

  ASSERT(fileName);

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
  UINT        pixelCount = imageWidth * imageHeight;

  while (pixelCount) {
    dst->b = source->b;
    dst->g = source->g;
    dst->r = source->r;
    dst->a = source->a;
    ++source;
    ++dst;
    --pixelCount;
  }

  texFile.Close();
  GenerateMipMask(fileName, mipFileMask);
  UINT levelsProvided = LoadPredrawnMips(texFile, mipFileMask, buffer);
  TextureGenerateMips(imageWidth, imageHeight, levelsProvided, levels, buffer);

  if (width) {
    *width = imageWidth;
  }

  if (height) {
    *height = imageHeight;
  }

  if (format) {
    *format = GxTex_Argb8888;
  }

  if (alphaBits) {
    *alphaBits = texFile.AlphaBits();
  }

  return buffer;
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

static void RequestImageDimensions(UINT *width, UINT *height, UINT *bestMip) {
  CGxCaps systemCaps = GxCaps();

  FATALASSERT(systemCaps.m_maxTextureSize > 0);

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

static HTEXTURE CreateTgaTexture(LPCSTR file, CGxTexFlags flags, CStatus *status) {
  CTgaFile image;

  if (!image.Open(file)) {
    return 0;
  }

  CTexture *texture = new (SMemAlloc(sizeof(CTexture), "HTEXTURE", SERR_LINECODE_OBJECT, 0)) CTexture;
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

  HTEXTURE handle = reinterpret_cast<HTEXTURE>(HandleCreate(texture, "HTEXTURE"));
  return handle;
}

static BOOL LoadBlpMips(LPCSTR fileName, MipBits *&buffer, UINT *width, UINT *height, EGxTexFormat *format, int *isOpaque, UINT *alphaBits) {
  CBLPFile     texFile;
  EGxTexFormat gxFormat = GxTex_Argb8888;
  PIXEL_FORMAT pixelFormat = PIXEL_ARGB8888;

  ASSERT(fileName);

  if (!texFile.Open(fileName)) {
    return 0;
  }

  if (texFile.m_header.colorEncoding == COLOR_DXT) {
    pixelFormat = static_cast<PIXEL_FORMAT>(texFile.m_header.preferredFormat);

    switch (pixelFormat) {
      case PIXEL_DXT1:
        if (GxCaps().m_texFmtDxt) {
          gxFormat = GxTex_Dxt1;
        } else if (texFile.m_header.alphaSize) {
          gxFormat = GxTex_Argb1555;
          pixelFormat = PIXEL_ARGB1555;
        } else {
          gxFormat = GxTex_Rgb565;
          pixelFormat = PIXEL_RGB565;
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

  UINT imgWidth = texFile.m_header.width;
  UINT imgHeight = texFile.m_header.height;
  UINT bestMip = 0;
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

static void
UpdateTextureDefault(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &texels) {
  ASSERT(!userArg);
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
  CGxTexParmsEx gxTexParmsEx;
  CGxTexParmsEx gxTexParmsEx2;
  CGxTexCache  *gxTexCache = 0;
  CGxTex       *gxTex = 0;
  UINT          indexW = 0;
  UINT          indexH = 0;

  ASSERT((width & (width - 1)) == 0);
  ASSERT((height & (height - 1)) == 0);

  gxTexParmsEx.target = GxTex_2d;
  gxTexParmsEx.width = width;
  gxTexParmsEx.height = height;
  gxTexParmsEx.depth = 0;
  gxTexParmsEx.format = format;
  gxTexParmsEx.dataFormat = dataFormat;
  gxTexParmsEx.flags = flags;
  gxTexParmsEx.userArg = userArg;
  gxTexParmsEx.userFunc = userFunc;

  ASSERT(height < 1024);
  ASSERT(width < 1024);

  if (width > 16 && height > 16 && format <= GxTex_Dxt5) {
    UINT scaledWidth = width >> 5;
    UINT scaledHeight = height >> 5;

    while (!(scaledWidth & 1)) {
      scaledWidth >>= 1;
      ++indexW;
    }

    while (!(scaledHeight & 1)) {
      scaledHeight >>= 1;
      ++indexH;
    }

    LISTEX(CGxTexCache, link) &indexedCacheList = s_gxTexCacheList[indexW][indexH][format];
    gxTexCache = indexedCacheList.Head();
    while (gxTexCache) {
      GxTexParametersEx(gxTexCache->gxTex, gxTexParmsEx2);
      ASSERT(gxTexParmsEx2.width == gxTexParmsEx.width);
      ASSERT(gxTexParmsEx2.height == gxTexParmsEx.height);

      if (gxTexParmsEx2.flags.m_filter == gxTexParmsEx.flags.m_filter) {
        break;
      }

      gxTexCache = indexedCacheList.Next(gxTexCache);
    }
  }

  if (!gxTexCache) {
    int ret = GxTexCreate(gxTexParmsEx, gxTex);
    ASSERT(ret);
    return gxTex;
  }

  s_gxTexCacheList[indexW][indexH][format].UnlinkNode(gxTexCache);
  s_gxTexCacheFreeList.LinkNode(gxTexCache, LIST_HEAD, 0);
  GxTexSetDataFormat(gxTexCache->gxTex, dataFormat);
  GxTexSetUserData(gxTexCache->gxTex, userFunc, userArg);
  GxTexSetFlags(gxTexCache->gxTex, flags);
  return gxTexCache->gxTex;
}

static void TextureFreeGxTex(CGxTex *gxTex) {
  CGxTexParmsEx gxTexParmsEx;
  UINT          indexW;
  UINT          indexH;
  CGxTexCache  *gxTexCache;

  ASSERT(gxTex);

  GxTexParametersEx(gxTex, gxTexParmsEx);

  if (gxTexParmsEx.width < 32 || gxTexParmsEx.height < 32 || gxTexParmsEx.format >= GxTexFormats_Last) {
    GxTexDestroy(gxTex);
    return;
  }

  gxTexParmsEx.width >>= 5;
  indexW = 0;
  while (!(gxTexParmsEx.width & 1)) {
    gxTexParmsEx.width >>= 1;
    ++indexW;
  }

  gxTexParmsEx.height >>= 5;
  indexH = 0;
  while (!(gxTexParmsEx.height & 1)) {
    gxTexParmsEx.height >>= 1;
    ++indexH;
  }

  gxTexCache = s_gxTexCacheFreeList.Head();
  if (!gxTexCache) {
    gxTexCache = s_gxTexCacheFreeList.NewNode(LIST_HEAD, 0, 0);
    ASSERT(gxTexCache);
  }

  s_gxTexCacheFreeList.UnlinkNode(gxTexCache);
  gxTexCache->gxTex = gxTex;
  gxTexCache->timeStamp = OsGetAsyncTimeMs();
  s_gxTexCacheList[indexW][indexH][gxTexParmsEx.format].LinkNode(gxTexCache, LIST_HEAD, 0);
  GxTexSetUserData(gxTex, UpdateTextureDefault, 0);
}

static void
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
      texelStrideInBytes = (w * texture->pixBitDepth) >> 3;
      ASSERT(texture->mipBits);
      texels = texture->mipBits->mip[mipLevel];
      break;

    case GxTex_Unlock:
      texture->mipBits = 0;
      break;
  }
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
      if (!alphaBits) {
        gxTexFormat = GxTex_Rgb565;
      } else if (alphaBits == 1) {
        gxTexFormat = GxTex_Argb1555;
      } else if (alphaBits == 4) {
        gxTexFormat = GxTex_Argb4444;
      } else {
        gxTexFormat = GxTex_Argb8888;
      }
      break;

    case PIXEL_DXT1:
      if (GxCaps().m_texFmtDxt) {
        gxTexFormat = GxTex_Dxt1;
        dataFormat = GxTex_Dxt1;
        texture->pixBitDepth = 4;
        pixFormat = PIXEL_DXT1;
      } else if (alphaBits) {
        dataFormat = GxTex_Argb1555;
        gxTexFormat = GxTex_Argb1555;
        texture->pixBitDepth = 16;
        pixFormat = PIXEL_ARGB1555;
      } else {
        dataFormat = GxTex_Rgb565;
        gxTexFormat = GxTex_Rgb565;
        texture->pixBitDepth = 16;
        pixFormat = PIXEL_RGB565;
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

    default:
      ASSERT(1);
      break;
  }
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

  UINT         width = image.m_header.width;
  UINT         height = image.m_header.height;
  UINT         bestMip = 0;
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

static BOOL AsyncTextureLoadImageCreate(CTexture *texture) {
  CBLPFile image;

  if (!image.Source(texture->asyncObject->buffer)) {
    return 0;
  }

  texture->alphaBits = static_cast<WORD>(image.AlphaBits());
  if (!texture->alphaBits) {
    texture->flags |= 1;
  }

  UINT width = image.Width();
  UINT height = image.Height();
  UINT bestMip = 0;
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

static void FillInSolidTexture(const NTempest::CImVector &color, CTexture *texture) {
  GxTexCreate(
      8, 8, GxTex_Argb8888, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), reinterpret_cast<LPVOID>(*color.IV_()), GxuUpdateSingleColorTexture,
      texture->gxTex
  );

  if (color.a >= 0xFE) {
    texture->flags |= 1;
  } else {
    texture->flags &= ~1U;
  }
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

static void AsyncTextureHandler() {
  UINT          bufferRemaining;
  CAsyncObject *asyncObject;
  CAsyncObject *asyncObjectnext_node;

  ASSERT(s_asyncPending >= 0);

  if (s_asyncPending) {
    return;
  }

  s_asyncLoadBufferUsed = 0;
  bufferRemaining = s_asyncLoadBuffer.MaxCount();

  for (asyncObject = s_asyncLoadList.Head(); asyncObject; asyncObject = asyncObjectnext_node) {
    asyncObjectnext_node = s_asyncLoadList.Next(asyncObject);

    if (bufferRemaining >= asyncObject->size) {
      asyncObject->link.Unlink();
      asyncObject->canReorder = 1;
      asyncObject->buffer = &s_asyncLoadBuffer[s_asyncLoadBufferUsed];
      s_asyncLoadBufferUsed += asyncObject->size;
      bufferRemaining -= asyncObject->size;
      ++s_asyncPending;
      AsyncFileReadObject(asyncObject);
    }
  }
}

static void AsyncTextureWait(CTexture *texture) {
  ASSERT(texture);

  if (texture->asyncObject) {
    AsyncFileReadWait(texture->asyncObject);
  }
}

static HTEXTURE CreateBlpTexture(LPCSTR filename, CGxTexFlags flags, CStatus *status) {
  SFile *file;

  if (!SFile::Open(filename, &file)) {
    return 0;
  }

  CTexture *texture = new (SMemAlloc(sizeof(CTexture), "HTEXTURE", SERR_LINECODE_OBJECT, 0)) CTexture;
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

  if (s_asyncLoadBuffer.MaxCount() - s_asyncLoadBufferUsed >= texture->asyncObject->size) {
    texture->asyncObject->buffer = &s_asyncLoadBuffer[s_asyncLoadBufferUsed];
    texture->asyncObject->canReorder = 1;
    s_asyncLoadBufferUsed += texture->asyncObject->size;
    ++s_asyncPending;
    AsyncFileReadObject(texture->asyncObject);
  } else {
    texture->asyncObject->canReorder = 0;
    s_asyncLoadList.LinkNode(texture->asyncObject, LIST_TAIL, 0);
  }

  return reinterpret_cast<HTEXTURE>(HandleCreate(texture, "HTEXTURE"));
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

    if ((*ext)[3] == 'a') {
      if ((*ext)[1] == 't' && (*ext)[2] == 'g') {
        imageFormat = IMAGE_FORMAT_TGA;
      }
    } else if ((*ext)[3] == 'p' && (*ext)[1] == 'b' && (*ext)[2] == 'l') {
      imageFormat = IMAGE_FORMAT_BLP;
    }

    **ext = 0;
  }

  return imageFormat;
}

void TextureInitialize() {
  g_textureMipBits = SMemAlloc(MippedImgCalcSize(2, 512, 512));
  ASSERT(g_textureMipBits);

  AsyncFileReadAddHandler(AsyncTextureHandler);
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
        LISTEX(CGxTexCache, link) &cacheList = s_gxTexCacheList[x][y][z];
        CGxTexCache *gxTexCache;

        while ((gxTexCache = cacheList.Head()) != 0) {
          if (gxTexCache->gxTex) {
            GxTexDestroy(gxTexCache->gxTex);
          }
          cacheList.DeleteNode(gxTexCache);
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
        s_solidTextureCache.Delete(static_cast<CSolidTextureHash *>(textureItem));
      } else {
        s_textureCache.Delete(static_cast<CTextureHash *>(textureItem));
      }

      ++numTexturesFlushed;
    }
  }

  if (numTexturesFlushed && status) {
    status->Add(STATUS_INFO, "%d texture(s) flushed from texture cache\n", numTexturesFlushed);
  }
}

MipBits *TextureLoadImage(LPCSTR filename, UINT *width, UINT *height, UINT *gxTexFormat, int *isOpaque, CStatus *status, UINT *alphaBits) {
  char         loadFileName[0x104];
  char        *ext;
  EGxTexFormat format;
  MipBits     *mipImages;

  FATALASSERT(filename);

  FATALASSERT(width);

  FATALASSERT(height);

  FATALASSERT(gxTexFormat);

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

HTEXTURE TextureLoadImage(LPCSTR filename) {
  char   loadFileName[0x104];
  char  *ext;
  SFile *file;

  FATALASSERT(filename);

  IdentifyAndStripFileExtension(filename, loadFileName, &ext);
  SStrCopy(ext, s_formatExt[IMAGE_FORMAT_BLP], 0x7FFFFFFF);

  if (!SFile::Open(loadFileName, &file)) {
    return 0;
  }

  CTexture *texture = new (SMemAlloc(sizeof(CTexture), "HTEXTURE", SERR_LINECODE_OBJECT, 0)) CTexture;
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

  if (s_asyncLoadBuffer.MaxCount() - s_asyncLoadBufferUsed >= texture->asyncObject->size) {
    texture->asyncObject->buffer = &s_asyncLoadBuffer[s_asyncLoadBufferUsed];
    texture->asyncObject->canReorder = 1;
    s_asyncLoadBufferUsed += texture->asyncObject->size;
    ++s_asyncPending;
    AsyncFileReadObject(texture->asyncObject);
  } else {
    texture->asyncObject->canReorder = 0;
    s_asyncLoadList.LinkNode(texture->asyncObject, LIST_TAIL, 0);
  }

  return reinterpret_cast<HTEXTURE>(HandleCreate(texture, "HTEXTURE"));
}

MipBits *TextureLoadImage(HTEXTURE texture, UINT *width, UINT *height, UINT *gxTexFormat, CStatus *status, UINT *alphaBits) {
  FATALASSERT(texture);

  return TextureLoadImage(reinterpret_cast<CTexture *>(texture)->filename, width, height, gxTexFormat, 0, status, alphaBits);
}

HTEXTURE TextureAllocImage(EGxTexFormat format, UINT width, UINT height) {
  CTexture *texture = new (SMemAlloc(sizeof(CTexture), "HTEXTURE", SERR_LINECODE_OBJECT, 0)) CTexture;
  ASSERT(texture);

  texture->gxTex = 0;
  texture->gxTexFormat = format;
  texture->gxWidth = width;
  texture->gxHeight = height;
  texture->pixBitDepth = static_cast<WORD>(CGxDevice::s_texFormatBitDepth[format]);
  texture->asyncObject = 0;
  texture->mipBits = TextureAllocMippedImg(format, width, height);
  SStrCopy(texture->filename, "TextureAllocImage", sizeof(texture->filename));

  return reinterpret_cast<HTEXTURE>(HandleCreate(texture, "HTEXTURE"));
}

HTEXTURE TextureCreate(UINT width, UINT height, EGxTexFormat format, CGxTexFlags flags) {
  CTexture *texture = new (SMemAlloc(sizeof(CTexture), "HTEXTURE", SERR_LINECODE_OBJECT, 0)) CTexture;
  ASSERT(texture);

  texture->gxTex = TextureAllocGxTex(width, height, format, flags, texture, UpdateTextureDefault, GxTex_Argb8888);
  ASSERT(texture->gxTex);

  texture->gxWidth = width;
  texture->gxHeight = height;
  texture->gxTexFormat = format;
  texture->gxTexFlags = flags;
  texture->pixBitDepth = s_bitDepth[format];
  texture->asyncObject = 0;
  SStrCopy(texture->filename, "unique_texture", sizeof(texture->filename));

  return reinterpret_cast<HTEXTURE>(HandleCreate(texture, "HTEXTURE"));
}

void TextureUnloadImage(MipBits *image) {
  FREEIFUSED(image);
}

HTEXTURE
TextureCreate(LPCSTR name, UINT width, UINT height, EGxTexFormat format, EGxTexFormat dataFormat, CGxTexFlags flags) {
  CTexture *texture = new (SMemAlloc(sizeof(CTexture), "HTEXTURE", SERR_LINECODE_OBJECT, 0)) CTexture;
  ASSERT(texture);

  texture->gxTex = TextureAllocGxTex(width, height, format, flags, texture, UpdateTextureDefault, dataFormat);
  ASSERT(texture->gxTex);

  texture->gxWidth = width;
  texture->gxHeight = height;
  texture->gxTexFormat = format;
  texture->gxTexFlags = flags;
  texture->pixBitDepth = s_bitDepth[format];
  texture->asyncObject = 0;
  SStrCopy(texture->filename, name, sizeof(texture->filename));

  return reinterpret_cast<HTEXTURE>(HandleCreate(texture, "HTEXTURE"));
}

HTEXTURE TextureCreate(LPCSTR name, UINT width, UINT height, EGxTexFormat format, CGxTexFlags flags) {
  return TextureCreate(name, width, height, format, GxTex_Argb8888, flags);
}

HTEXTURE TextureCreate(LPCSTR fileName, CGxTexFlags flags, CStatus *status, int dontCache) {
  char  loadFileName[0x104];
  char *ext;

  FATALASSERT(fileName);

  FATALASSERT(fileName[0]);

  FATALASSERT(status);

  EImageFormat imageFormat = IdentifyAndStripFileExtension(fileName, loadFileName, &ext);

  if (dontCache) {
    BaseFileRegisterUncachable(fileName);
  } else {
    HTEXTURE texture = GetTexture(loadFileName, flags);
    if (texture) {
      return texture;
    }
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

  if (!texture) {
    FileError(status, "texture", fileName);
    return TextureCreateSolid(CRAPPY_GREEN, 0);
  }

  if (!dontCache) {
    *ext = 0;
    HashNewTexture(loadFileName, flags, texture, status);
  }

  return texture;
}

HTEXTURE TextureCreate(CGxTex *gxTex) {
  FATALASSERT(gxTex);

  CTexture *texture = new (SMemAlloc(sizeof(CTexture), "HTEXTURE", SERR_LINECODE_OBJECT, 0)) CTexture;
  if (!texture) {
    return 0;
  }

  texture->gxTex = gxTex;
  SStrCopy(texture->filename, "unique_texture", sizeof(texture->filename));
  return reinterpret_cast<HTEXTURE>(HandleCreate(texture, "HTEXTURE"));
}

HTEXTURE TextureCreateSolid(const NTempest::CImVector &color, CStatus *status) {
  HTEXTURE texture = GetTexture(color);

  if (texture) {
    return texture;
  }

  CTexture *textureObject = new (SMemAlloc(sizeof(CTexture), "HTEXTURE", SERR_LINECODE_OBJECT, 0)) CTexture;

  if (!textureObject) {
    return 0;
  }

  FillInSolidTexture(color, textureObject);
  texture = reinterpret_cast<HTEXTURE>(HandleCreate(textureObject, "HTEXTURE"));
  HashNewTexture(color, texture, status);
  return texture;
}

CGxTex *TextureGetGxTex(HTEXTURE texture, int force, CStatus *status) {
  CTexture *textureObject = reinterpret_cast<CTexture *>(texture);

  FATALASSERT(texture);

  if (!textureObject->gxTex) {
    if (!force) {
      return 0;
    }

    AsyncTextureWait(textureObject);
    if (status) {
      status->Add(textureObject->loadStatus);
    }
  }

  return textureObject->gxTex;
}

MipBits *TextureGetMips(HTEXTURE texture, int force) {
  CTexture *textureObject = reinterpret_cast<CTexture *>(texture);

  FATALASSERT(texture);

  if (!textureObject->mipBits) {
    if (!force) {
      return 0;
    }

    OsOutputDebugString("AsyncTextureWait: %s\n", textureObject->filename);
    AsyncTextureWait(textureObject);
  }

  return textureObject->mipBits;
}

int TextureIsOpaque(HTEXTURE__ *texture) {
  CTexture *texturePtr = reinterpret_cast<CTexture *>(texture);
  FATALASSERT(texturePtr);
  return texturePtr->flags & 1;
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

static void TextureGenerateMips(UINT width, UINT height, UINT levelsProvided, UINT levelsDesired, MipBits *levelBits) {
  UINT sourceWidth = width;
  UINT sourceHeight = height;
  UINT destWidth = width;
  UINT destHeight = height;
  UINT last_good_level;

  last_good_level = 0;

  if (levelsProvided) {
    last_good_level = levelsProvided - 1;
    sourceWidth = width >> last_good_level;
    sourceHeight = height >> last_good_level;
  }

  for (UINT level = 1; level < levelsDesired; ++level) {
    destWidth >>= 1;
    if (destWidth < 1) {
      destWidth = 1;
    }

    destHeight >>= 1;
    if (destHeight < 1) {
      destHeight = 1;
    }

    if (level >= levelsProvided) {
      FullShrink(levelBits->mip[level], destWidth, destHeight, levelBits->mip[last_good_level], sourceWidth, sourceHeight);
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

MipBits *TextureCopyMippedImage(MipBits *srcData, EGxTexFormat format, UINT width, UINT height) {
  if (!srcData) {
    return 0;
  }

  MipBits *destData = TextureAllocMippedImg(format, width, height);
  if (destData) {
    UINT mipCount = TextureCalcMipCount(width, height);
    memcpy(&destData->mip[mipCount], &srcData->mip[mipCount], CalcLevelOffset(mipCount, width, height, format));
  }
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
  FATALASSERT(texture);
  return reinterpret_cast<CTexture *>(texture)->filename;
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

static int __cdecl TextureLogSortCallback(LPCVOID elem1, LPCVOID elem2) {
  ASSERT(elem1);
  ASSERT(elem2);

  CTexture *const *texture1 = static_cast<CTexture *const *>(elem1);
  CTexture *const *texture2 = static_cast<CTexture *const *>(elem2);
  return SStrCmpI((*texture1)->filename, (*texture2)->filename, 0x7FFFFFFF);
}

void TextureLogTextures(HSLOG log) {
  UINT                        i;
  UINT                        texTotal;
  UINT                        texTypeTotal[7];
  TSGrowableArray<CTexture *> textureSortList;

  ASSERT(log);

  ITERATELIST(CTexture, s_textureList, texture) {
    if (texture->filename[0]) {
      *textureSortList.New() = texture;
    }
  }

  qsort(textureSortList.Ptr(), textureSortList.Count(), sizeof(CTexture *), TextureLogSortCallback);

  texTotal = 0;
  memset(texTypeTotal, 0, sizeof(texTypeTotal));

  for (i = 0; i < textureSortList.Count(); ++i) {
    CTexture *texture = textureSortList[i];
    ASSERT(texture);

    SLogWrite(log, "%s : %dx%dx%dbit", texture->filename, texture->gxWidth, texture->gxHeight, texture->pixBitDepth);

    UINT textureSize = texture->gxWidth * texture->gxHeight;
    UINT bytesPerPixel = texture->pixBitDepth >> 3;
    if (bytesPerPixel) {
      textureSize *= bytesPerPixel;
    } else {
      textureSize >>= 1;
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
