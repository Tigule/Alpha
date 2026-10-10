#ifndef ENGINE_SOURCE_BLPFILE_BLP_H
#define ENGINE_SOURCE_BLPFILE_BLP_H

#include "Images/blit.h"
#include "Images/dxt.h"

#include <string.h>

enum EGxTexFormat;
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

enum {
  ALPHA_0 = 0,
  ALPHA_1 = 1,
  ALPHA_4 = 4,
  ALPHA_8 = 8
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

static BOOL LoadBlpMips(LPCSTR fileName, MipBits *&buffer, UINT *width, UINT *height, EGxTexFormat *format, int *isOpaque, UINT *alphaBits);
static BOOL PumpBlpTextureAsync(CTexture *texture);

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
  friend BOOL LoadBlpMips(LPCSTR fileName, MipBits *&buffer, UINT *width, UINT *height, EGxTexFormat *format, int *isOpaque, UINT *alphaBits);
  friend BOOL PumpBlpTextureAsync(CTexture *texture);

 public:
  enum {
    m_firstBuildIndex = 0,
    m_numColorsToBuild = 256,
    m_noColorTrimming = 0,
    m_firstMapIndex = 0,
    m_lastMapIndex = 255,
    m_versionMagic = '2PLB'
  };

 private:
  void SharedInit() {
    memset(&m_header, 0, sizeof(m_header));
    m_header.magic = m_versionMagic;
    m_header.formatVersion = 1;
    m_header.preferredFormat = PIXEL_ARGB8888;
    m_inMemoryImage = 0;
    m_mipMapAlgorithm = MMA_BOX;
  }

 public:
  CBLPFile() : m_images(0), m_quality(100) {
    SharedInit();
  }

  CBLPFile(CBLPFile &source);

  ~CBLPFile() {
    Close();
  }

  void Close();

  UINT Width() const {
    return m_header.width;
  }
  UINT Width(UINT mipLevel) const {
    return max(m_header.width >> mipLevel, 1);
  }

  UINT Height() const {
    return m_header.height;
  }
  UINT Height(UINT mipLevel) const {
    return max(m_header.height >> mipLevel, 1);
  }

  UINT Pixels() const {
    return m_header.width * m_header.height;
  }
  UINT Pixels(UINT mipLevel) const {
    return Width(mipLevel) * Height(mipLevel);
  }
  UINT Bytes() const;
  UINT Bytes(UINT mipLevel) const;
  UINT Quality() const {
    return m_quality;
  }
  MIPS_TYPE HasMips() const;

  UINT AlphaBits() const {
    return m_header.alphaSize;
  }
  BOOL SetAlphaBits(UINT alpha);

  int  GenerateMipLevel(UINT sourceLevel, UINT destinationLevel);
  int  GenerateMipLevels(CStatus *status);
  int  GenerateMipLevels(LPCSTR name, CStatus *status);
  int  Open(LPCSTR filename);
  BOOL Source(LPVOID fileBits);
  COLOR_FILE_FORMAT GetColorEncoding() const {
    return (COLOR_FILE_FORMAT)m_header.colorEncoding;
  }
  PIXEL_FORMAT GetPreferredFormat() const {
    return (PIXEL_FORMAT)m_header.preferredFormat;
  }
  int  Lock(PIXEL_FORMAT format, UINT mipLevel, BYTE *&data, UINT &stride);
  int  Unlock(UINT mipLevel);
  BOOL LockChain(PIXEL_FORMAT pixelFormat, MipBits *&images, UINT mipLevel);
  UINT GetNumLevels() {
    return m_numLevels;
  }
  static void FlushFromReadCache(LPCSTR name);
  void        SetHasMips(MIPS_TYPE hasMips);
  BOOL        SetImage(LPCVOID pImg, UINT width, UINT height, UINT alphaBits, UINT mipLevel, CStatus *status);
  BOOL        SetImage(CBLPFile &source, UINT mipLevel, CStatus *status);
  int         SetImages(CBLPFile &source);
  int         Write(LPCSTR name, COLOR_FILE_FORMAT format);
  void SetQuality(UINT quality) {
    m_quality = quality;
  }
  void SetPreferredFormat(PIXEL_FORMAT format) {
    m_header.preferredFormat = (BYTE)format;
  }
  void SetMipMapAlgorithm(MipMapAlgorithm algorithm) {
    m_mipMapAlgorithm = algorithm;
  }

 protected:
  int              GetOctreePal(tagPALETTEENTRY *palette, UINT count);
  BYTE            *Image(UINT level);
  BOOL             CreateMipLevels(UINT width, UINT height);
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
  BOOL             IsValidMip(UINT level) const;

  MipBits        *m_images;
  BLPHeader       m_header;
  LPVOID          m_inMemoryImage;
  int             m_inMemoryNeedsFree;
  UINT            m_numLevels;
  UINT            m_quality;
  HCOLORMAP__    *m_colorMapping;
  MipMapAlgorithm m_mipMapAlgorithm;

  int  DecompJPEG(PIXEL_FORMAT format, UINT mipLevel, BYTE *&data, UINT dataSize, LPCVOID source, UINT &stride);
  BOOL DecompPal(PIXEL_FORMAT format, UINT mipLevel, BYTE *data, LPCVOID tempBuffer);
  void DecompPalFastPath(BYTE *data, LPCVOID tempBuffer, UINT colorSize);
  void DecompPalARGB8888(BYTE *data, LPCVOID tempBuffer, UINT colorSize);
  void DecompPalARGB4444(BYTE *data, LPCVOID tempBuffer, UINT colorSize);
  void DecompPalARGB1555(BYTE *data, LPCVOID tempBuffer, UINT colorSize);
  void DecompPalARGB565(BYTE *data, LPCVOID tempBuffer, UINT colorSize);
  BOOL GetFormatSize(PIXEL_FORMAT format, UINT mipLevel, UINT *size, UINT *stride) const;

  static BYTE s_eightBitAlphaLookup[16];
  static BYTE s_oneBitAlphaLookup[2];
  static WORD s_oneBitAlphaShort[2];
  BYTE       *m_lockDecompMem;

 private:
  CBLPFile &operator=(CBLPFile &source);

 public:
  BOOL LockChain2(PIXEL_FORMAT pixelFormat, MipBits *&images, UINT mipLevel);

 private:
  int Lock2(PIXEL_FORMAT format, UINT mipLevel, BYTE *data, UINT &stride);
  int Unlock2(UINT mipLevel);
};

#endif
