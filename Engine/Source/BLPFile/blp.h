#ifndef ENGINE_SOURCE_BLPFILE_BLP_H
#define ENGINE_SOURCE_BLPFILE_BLP_H

#include "Gx/Gx.h"
#include "Images/dxt.h"

#include <string.h>

class CStatus;
class CTexture;
struct HCOLORMAP__;

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

enum {
  COLOR_JPEG = 0,
  COLOR_PAL = 1,
  COLOR_DXT = 2
};

static int __fastcall LoadBlpMips(
    const char   *fileName,
    MipBits     *&buffer,
    unsigned int *width,
    unsigned int *height,
    EGxTexFormat *format,
    int          *isOpaque,
    unsigned int *alphaBits
);
static int __fastcall PumpBlpTextureAsync(CTexture *texture);

struct BlpPalPixel {
  unsigned char b;
  unsigned char g;
  unsigned char r;
  unsigned char pad;
};

struct BLPHeader {
  unsigned int  magic;
  unsigned int  formatVersion;
  unsigned char colorEncoding;
  unsigned char alphaSize;
  unsigned char preferredFormat;
  unsigned char hasMips;
  unsigned int  width;
  unsigned int  height;
  unsigned int  mipOffsets[16];
  unsigned int  mipSizes[16];
  union {
    BlpPalPixel palette[256];
    struct {
      unsigned int  headerSize;
      unsigned char headerData[1020];
    } jpeg;
  } extended;
};

class CBLPFile {
  friend int __fastcall LoadBlpMips(
      const char   *fileName,
      MipBits     *&buffer,
      unsigned int *width,
      unsigned int *height,
      EGxTexFormat *format,
      int          *isOpaque,
      unsigned int *alphaBits
  );
  friend int __fastcall PumpBlpTextureAsync(CTexture *texture);

 public:
  CBLPFile() : m_images(0), m_quality(100) {
    SharedInit();
  }

  ~CBLPFile() {
    Close();
  }

  void      Close();
  int       Open(const char *filename);
  int       Source(void *fileBits);
  int       Lock(PIXEL_FORMAT format, unsigned int mipLevel, unsigned char *&data, unsigned int &stride);
  int       Unlock(unsigned int mipLevel);
  int       LockChain(PIXEL_FORMAT pixelFormat, MipBits *&images, unsigned int mipLevel);
  int       LockChain2(PIXEL_FORMAT pixelFormat, MipBits *&images, unsigned int mipLevel);
  MIPS_TYPE HasMips() const;

  unsigned int Width() const {
    return m_header.width;
  }

  unsigned int Height() const {
    return m_header.height;
  }

  unsigned int AlphaBits() const {
    return m_header.alphaSize;
  }

 protected:
  int  IsValidMip(unsigned int level) const;
  int  GetFormatSize(PIXEL_FORMAT format, unsigned int mipLevel, unsigned int *size, unsigned int *stride) const;
  void DecompPalFastPath(unsigned char *data, const void *tempBuffer, unsigned int colorSize);
  void DecompPalARGB8888(unsigned char *data, const void *tempBuffer, unsigned int colorSize);
  void DecompPalARGB4444(unsigned char *data, const void *tempBuffer, unsigned int colorSize);
  void DecompPalARGB1555(unsigned char *data, const void *tempBuffer, unsigned int colorSize);
  void DecompPalARGB565(unsigned char *data, const void *tempBuffer, unsigned int colorSize);
  int  DecompPal(PIXEL_FORMAT format, unsigned int mipLevel, unsigned char *data, const void *tempBuffer);

  static unsigned char  s_eightBitAlphaLookup[16];
  static unsigned char  s_oneBitAlphaLookup[2];
  static unsigned short s_oneBitAlphaShort[2];

 private:
  void SharedInit() {
    memset(&m_header, 0, sizeof(m_header));
    m_header.magic = 0x32504C42;
    m_header.formatVersion = 1;
    m_header.preferredFormat = PIXEL_ARGB8888;
    m_inMemoryImage = 0;
    m_mipMapAlgorithm = MMA_BOX;
  }

  int Lock2(PIXEL_FORMAT format, unsigned int mipLevel, unsigned char *data, unsigned int &stride);
  int Unlock2(unsigned int mipLevel);

  MipBits        *m_images;
  BLPHeader       m_header;
  void           *m_inMemoryImage;
  int             m_inMemoryNeedsFree;
  unsigned int    m_numLevels;
  unsigned int    m_quality;
  HCOLORMAP__    *m_colorMapping;
  MipMapAlgorithm m_mipMapAlgorithm;
  unsigned char  *m_lockDecompMem;
};

#endif
