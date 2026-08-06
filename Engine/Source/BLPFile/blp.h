#ifndef ENGINE_SOURCE_BLPFILE_BLP_H
#define ENGINE_SOURCE_BLPFILE_BLP_H

#include "Gx/Gx.h"
#include "Images/dxt.h"

#include <string.h>

class CStatus;
class CTexture;
struct HCOLORMAP__;
struct HCOLORLIST__;
struct tagPALETTEENTRY;

enum PIXEL_FORMAT {
  PIXEL_DXT1 = 0,
  PIXEL_DXT3 = 1,
  PIXEL_ARGB8888 = 2,
  PIXEL_ARGB1555 = 3,
  PIXEL_ARGB4444 = 4,
  PIXEL_RGB565 = 5,
  PIXEL_A8 = 6,
  PIXEL_DXT5 = 7,
  PIXEL_UNSPECIFIED = 8,
  NUM_PIXEL_FORMATS = 9
};

enum MIPS_TYPE {
  MIPS_NONE = 0,
  MIPS_GENERATED = 1,
  MIPS_HANDMADE = 2
};

enum MipMapAlgorithm {
  MMA_BOX = 0,
  MMA_CUBIC = 1,
  MMA_FULLDFT = 2,
  MMA_KAISER = 3,
  MMA_LINEARLIGHTKAISER = 4
};

enum COLOR_FILE_FORMAT {
  COLOR_JPEG = 0,
  COLOR_PAL = 1,
  COLOR_DXT = 2
};

static int LoadBlpMips(LPCSTR fileName, MipBits *&buffer, UINT *width, UINT *height, EGxTexFormat *format, int *isOpaque, UINT *alphaBits);
static int PumpBlpTextureAsync(CTexture *texture);

struct BlpPalPixel {
  BYTE b;
  BYTE g;
  BYTE r;
  BYTE pad;
};

struct BLPHeader {
  DWORD magic;
  DWORD formatVersion;
  BYTE  colorEncoding;
  BYTE  alphaSize;
  BYTE  preferredFormat;
  BYTE  hasMips;
  DWORD width;
  DWORD height;
  DWORD mipOffsets[16];
  DWORD mipSizes[16];
  union {
    BlpPalPixel palette[256];
    struct {
      DWORD headerSize;
      BYTE  headerData[1020];
    } jpeg;
  } extended;
};

class CBLPFile {
  friend int LoadBlpMips(LPCSTR fileName, MipBits *&buffer, UINT *width, UINT *height, EGxTexFormat *format, int *isOpaque, UINT *alphaBits);
  friend int PumpBlpTextureAsync(CTexture *texture);

 public:
  enum {
    m_firstBuildIndex = 0,
    m_numColorsToBuild = 256,
    m_noColorTrimming = 0,
    m_firstMapIndex = 0,
    m_lastMapIndex = 255,
    m_versionMagic = 0x32504C42
  };

  CBLPFile() : m_images(0), m_quality(100) {
    SharedInit();
  }

  CBLPFile(CBLPFile &source);

  ~CBLPFile() {
    Close();
  }

  void      Close();
  int       Open(LPCSTR filename);
  int       Source(LPVOID fileBits);
  int       Lock(PIXEL_FORMAT format, UINT mipLevel, BYTE *&data, UINT &stride);
  int       Unlock(UINT mipLevel);
  int       LockChain(PIXEL_FORMAT pixelFormat, MipBits *&images, UINT mipLevel);
  int       LockChain2(PIXEL_FORMAT pixelFormat, MipBits *&images, UINT mipLevel);
  int       SetImage(CBLPFile &source, UINT mipLevel, CStatus *status);
  int       SetImage(LPCVOID pImg, UINT width, UINT height, UINT alphaBits, UINT mipLevel, CStatus *status);
  int       SetAlphaBits(UINT alpha);
  MIPS_TYPE HasMips() const;
  void      SetHasMips(MIPS_TYPE hasMips);
  UINT      Bytes() const;
  UINT      Bytes(UINT mipLevel) const;

  UINT Width() const {
    return m_header.width;
  }
  UINT Width(UINT mipLevel) const {
    UINT width = m_header.width >> mipLevel;
    return width ? width : 1;
  }

  UINT Height() const {
    return m_header.height;
  }
  UINT Height(UINT mipLevel) const {
    UINT height = m_header.height >> mipLevel;
    return height ? height : 1;
  }

  UINT Pixels() const {
    return m_header.width * m_header.height;
  }
  UINT Pixels(UINT mipLevel) const {
    return Width(mipLevel) * Height(mipLevel);
  }
  UINT Quality() const {
    return m_quality;
  }
  COLOR_FILE_FORMAT GetColorEncoding() const {
    return static_cast<COLOR_FILE_FORMAT>(m_header.colorEncoding);
  }
  PIXEL_FORMAT GetPreferredFormat() const {
    return static_cast<PIXEL_FORMAT>(m_header.preferredFormat);
  }
  UINT GetNumLevels() {
    return m_numLevels;
  }
  void SetQuality(UINT quality) {
    m_quality = quality;
  }
  void SetPreferredFormat(PIXEL_FORMAT format) {
    m_header.preferredFormat = static_cast<BYTE>(format);
  }
  void SetMipMapAlgorithm(MipMapAlgorithm algorithm) {
    m_mipMapAlgorithm = algorithm;
  }

  UINT AlphaBits() const {
    return m_header.alphaSize;
  }

  int         GenerateMipLevel(UINT sourceLevel, UINT destinationLevel);
  int         GenerateMipLevels(LPCSTR name, CStatus *status);
  int         GenerateMipLevels(CStatus *status);
  static void FlushFromReadCache(LPCSTR name);
  int         SetImages(CBLPFile &source);
  int         Write(LPCSTR name, COLOR_FILE_FORMAT format);

 protected:
  BYTE            *Image(UINT level);
  int              CreateMipLevels(UINT width, UINT height);
  int              IsValidMip(UINT level) const;
  int              GetFormatSize(PIXEL_FORMAT format, UINT mipLevel, UINT *size, UINT *stride) const;
  void             DecompPalFastPath(BYTE *data, LPCVOID tempBuffer, UINT colorSize);
  void             DecompPalARGB8888(BYTE *data, LPCVOID tempBuffer, UINT colorSize);
  void             DecompPalARGB4444(BYTE *data, LPCVOID tempBuffer, UINT colorSize);
  void             DecompPalARGB1555(BYTE *data, LPCVOID tempBuffer, UINT colorSize);
  void             DecompPalARGB565(BYTE *data, LPCVOID tempBuffer, UINT colorSize);
  int              DecompPal(PIXEL_FORMAT format, UINT mipLevel, BYTE *data, LPCVOID tempBuffer);
  int              DecompJPEG(PIXEL_FORMAT format, UINT mipLevel, BYTE *&data, UINT dataSize, LPCVOID source, UINT &stride);
  int              GetOctreePal(tagPALETTEENTRY *palette, UINT count);
  int              AddSourceImages(HCOLORLIST__ *colors);
  int              ComputePalette(HCOLORLIST__ *colors);
  int              Palettize(LPVOID output);
  int              MakeAlpha(BYTE **alpha, UINT mipLevel);
  int              MakeJPEGS(LPVOID output);
  int              MakeDXT(LPVOID output);
  int              WriteOutputFile(LPVOID file, BYTE *header, BYTE *data, UINT size);
  int              WriteJPEGOutputFile(LPVOID file, BYTE *header, UINT headerSize, UINT dataSize);
  int              WriteHeader(LPVOID file);
  int              PalettizeSourceImage(BYTE **image, UINT mipLevel);
  int              BuildPalettedImages(LPVOID output);
  tagPALETTEENTRY *GetBackgroundColor(int alpha, tagPALETTEENTRY *color);
  void             PaletteConvert(const tagPALETTEENTRY *source, BlpPalPixel *destination);

  static BYTE s_eightBitAlphaLookup[16];
  static BYTE s_oneBitAlphaLookup[2];
  static WORD s_oneBitAlphaShort[2];

 private:
  CBLPFile &operator=(CBLPFile &source);

  void SharedInit() {
    memset(&m_header, 0, sizeof(m_header));
    m_header.magic = 0x32504C42;
    m_header.formatVersion = 1;
    m_header.preferredFormat = PIXEL_ARGB8888;
    m_inMemoryImage = 0;
    m_mipMapAlgorithm = MMA_BOX;
  }

  int Lock2(PIXEL_FORMAT format, UINT mipLevel, BYTE *data, UINT &stride);
  int Unlock2(UINT mipLevel);

 protected:
  MipBits        *m_images;
  BLPHeader       m_header;
  LPVOID          m_inMemoryImage;
  int             m_inMemoryNeedsFree;
  UINT            m_numLevels;
  UINT            m_quality;
  HCOLORMAP__    *m_colorMapping;
  MipMapAlgorithm m_mipMapAlgorithm;
  BYTE           *m_lockDecompMem;
};

#endif
