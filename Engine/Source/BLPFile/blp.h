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

static int LoadBlpMips(
    const char   *fileName,
    MipBits     *&buffer,
    unsigned int *width,
    unsigned int *height,
    EGxTexFormat *format,
    int          *isOpaque,
    unsigned int *alphaBits
);
static int PumpBlpTextureAsync(CTexture *texture);

struct BlpPalPixel {
  unsigned char b;
  unsigned char g;
  unsigned char r;
  unsigned char pad;
};

struct BLPHeader {
  unsigned long magic;
  unsigned long formatVersion;
  unsigned char colorEncoding;
  unsigned char alphaSize;
  unsigned char preferredFormat;
  unsigned char hasMips;
  unsigned long width;
  unsigned long height;
  unsigned long mipOffsets[16];
  unsigned long mipSizes[16];
  union {
    BlpPalPixel palette[256];
    struct {
      unsigned long headerSize;
      unsigned char headerData[1020];
    } jpeg;
  } extended;
};

class CBLPFile {
  friend int LoadBlpMips(
      const char   *fileName,
      MipBits     *&buffer,
      unsigned int *width,
      unsigned int *height,
      EGxTexFormat *format,
      int          *isOpaque,
      unsigned int *alphaBits
  );
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
  int       Open(const char *filename);
  int       Source(void *fileBits);
  int       Lock(PIXEL_FORMAT format, unsigned int mipLevel, unsigned char *&data, unsigned int &stride);
  int       Unlock(unsigned int mipLevel);
  int       LockChain(PIXEL_FORMAT pixelFormat, MipBits *&images, unsigned int mipLevel);
  int       LockChain2(PIXEL_FORMAT pixelFormat, MipBits *&images, unsigned int mipLevel);
  int       SetImage(CBLPFile &source, unsigned int mipLevel, CStatus *status);
  int       SetImage(
            const void *pImg,
            unsigned int width,
            unsigned int height,
            unsigned int alphaBits,
            unsigned int mipLevel,
            CStatus *status
  );
  int       SetAlphaBits(unsigned int alpha);
  MIPS_TYPE HasMips() const;
  void      SetHasMips(MIPS_TYPE hasMips);
  unsigned int Bytes() const;
  unsigned int Bytes(unsigned int mipLevel) const;

  unsigned int Width() const {
    return m_header.width;
  }
  unsigned int Width(unsigned int mipLevel) const {
    unsigned int width = m_header.width >> mipLevel;
    return width ? width : 1;
  }

  unsigned int Height() const {
    return m_header.height;
  }
  unsigned int Height(unsigned int mipLevel) const {
    unsigned int height = m_header.height >> mipLevel;
    return height ? height : 1;
  }

  unsigned int Pixels() const {
    return m_header.width * m_header.height;
  }
  unsigned int Pixels(unsigned int mipLevel) const {
    return Width(mipLevel) * Height(mipLevel);
  }
  unsigned int Quality() const {
    return m_quality;
  }
  COLOR_FILE_FORMAT GetColorEncoding() const {
    return static_cast<COLOR_FILE_FORMAT>(m_header.colorEncoding);
  }
  PIXEL_FORMAT GetPreferredFormat() const {
    return static_cast<PIXEL_FORMAT>(m_header.preferredFormat);
  }
  unsigned int GetNumLevels() {
    return m_numLevels;
  }
  void SetQuality(unsigned int quality) {
    m_quality = quality;
  }
  void SetPreferredFormat(PIXEL_FORMAT format) {
    m_header.preferredFormat = static_cast<unsigned char>(format);
  }
  void SetMipMapAlgorithm(MipMapAlgorithm algorithm) {
    m_mipMapAlgorithm = algorithm;
  }

  unsigned int AlphaBits() const {
    return m_header.alphaSize;
  }

  int GenerateMipLevel(unsigned int sourceLevel, unsigned int destinationLevel);
  int GenerateMipLevels(const char *name, CStatus *status);
  int GenerateMipLevels(CStatus *status);
  static void FlushFromReadCache(const char *name);
  int SetImages(CBLPFile &source);
  int Write(const char *name, COLOR_FILE_FORMAT format);

 protected:
  unsigned char *Image(unsigned int level);
  int  CreateMipLevels(unsigned int width, unsigned int height);
  int  IsValidMip(unsigned int level) const;
  int  GetFormatSize(PIXEL_FORMAT format, unsigned int mipLevel, unsigned int *size, unsigned int *stride) const;
  void DecompPalFastPath(unsigned char *data, const void *tempBuffer, unsigned int colorSize);
  void DecompPalARGB8888(unsigned char *data, const void *tempBuffer, unsigned int colorSize);
  void DecompPalARGB4444(unsigned char *data, const void *tempBuffer, unsigned int colorSize);
  void DecompPalARGB1555(unsigned char *data, const void *tempBuffer, unsigned int colorSize);
  void DecompPalARGB565(unsigned char *data, const void *tempBuffer, unsigned int colorSize);
  int  DecompPal(PIXEL_FORMAT format, unsigned int mipLevel, unsigned char *data, const void *tempBuffer);
  int  DecompJPEG(
      PIXEL_FORMAT format,
      unsigned int mipLevel,
      unsigned char *&data,
      unsigned int dataSize,
      const void *source,
      unsigned int &stride
  );
  int GetOctreePal(tagPALETTEENTRY *palette, unsigned int count);
  int AddSourceImages(HCOLORLIST__ *colors);
  int ComputePalette(HCOLORLIST__ *colors);
  int Palettize(void *output);
  int MakeAlpha(unsigned char **alpha, unsigned int mipLevel);
  int MakeJPEGS(void *output);
  int MakeDXT(void *output);
  int WriteOutputFile(void *file, unsigned char *header, unsigned char *data, unsigned int size);
  int WriteJPEGOutputFile(void *file, unsigned char *header, unsigned int headerSize, unsigned int dataSize);
  int WriteHeader(void *file);
  int PalettizeSourceImage(unsigned char **image, unsigned int mipLevel);
  int BuildPalettedImages(void *output);
  tagPALETTEENTRY *GetBackgroundColor(int alpha, tagPALETTEENTRY *color);
  void PaletteConvert(const tagPALETTEENTRY *source, BlpPalPixel *destination);

  static unsigned char  s_eightBitAlphaLookup[16];
  static unsigned char  s_oneBitAlphaLookup[2];
  static unsigned short s_oneBitAlphaShort[2];

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

  int Lock2(PIXEL_FORMAT format, unsigned int mipLevel, unsigned char *data, unsigned int &stride);
  int Unlock2(unsigned int mipLevel);

 protected:
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
