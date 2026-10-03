#include "blp.h"

#include "Base/Status.h"
#include "Images/blit.h"
#include "Tempest/c2ivector.h"

#include <storm.h>
#include <stpl.h>

#include <string.h>

enum {
  dSaveButton = 1,
  dCancelButton = 2,
  dDXT1 = 10,
  dTextureFormatFirst = 10,
  dDXT1a = 11,
  dDXT3 = 12,
  dDXT5 = 13,
  d4444 = 14,
  d1555 = 15,
  d565 = 16,
  d8888 = 17,
  d888 = 18,
  d555 = 19,
  d8 = 20,
  dNVHS = 21,
  dNVHU = 22,
  dTextureFormatLast = 22,
  d3DPreviewButton = 300,
  dViewDXT1 = 200,
  dViewDXT2 = 201,
  dViewDXT3 = 202,
  dViewDXT5 = 203,
  dViewA4R4G4B4 = 204,
  dViewA1R5G5B5 = 205,
  dViewR5G6B5 = 206,
  dViewA8R8G8B8 = 207,
  dGenerateMipMaps = 30,
  dMIPMapSourceFirst = 30,
  dSpecifyMipMaps = 31,
  dUseExistingMipMaps = 32,
  dNoMipMaps = 33,
  dMIPMapSourceLast = 33,
  dSpecifiedMipMaps = 39,
  dMIPFilterBox = 133,
  dMIPFilterFirst = 133,
  dMIPFilterCubic = 134,
  dMIPFilterFullDFT = 135,
  dMIPFilterKaiser = 136,
  dMIPFilterLinearLightKaiser = 137,
  dMIPFilterLast = 137,
  dShowDifferences = 40,
  dShowFiltering = 41,
  dShowMipMapping = 42,
  dShowAnisotropic = 43,
  dChangeClearColorButton = 50,
  dViewXBOX1c = 51,
  dViewXBOX1a = 52,
  dDitherColor = 53,
  dLoadBackgroundImageButton = 54,
  dUseBackgroundImage = 55,
  dBinaryAlpha = 56,
  dAlphaBlending = 57,
  dFadeColor = 58,
  dFadeAlpha = 59,
  dFadeToColorButton = 60,
  dAlphaBorder = 61,
  dBorder = 62,
  dBorderColorButton = 63,
  dNormalMap = 64,
  dDuDvMap = 65,
  dDitherEachMIPLevel = 66,
  dGreyScale = 67,
  dZoom = 70,
  dTextureType2D = 80,
  dTextureTypeFirst = 80,
  dTextureTypeCube = 81,
  dTextureTypeImage = 82,
  dTextureTypeLast = 82,
  dFadeAmount = 90,
  dFadeToAlpha = 91,
  dFadeToDelay = 92,
  dAskToLoadMIPMaps = 400,
  dShowAlphaWarning = 401,
  dShowPower2Warning = 402,
  dAdvancedBlendingButton = 500,
  dUserSpecifiedFadingAmounts = 501
};

using NTempest::C2iVector;

DECLARE_STRICT_HANDLE(HCOLORMAP);
DECLARE_STRICT_HANDLE(HCOLORLIST);

BYTE CBLPFile::s_eightBitAlphaLookup[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
BYTE CBLPFile::s_oneBitAlphaLookup[2] = {0x00, 0xFF};
WORD CBLPFile::s_oneBitAlphaShort[2] = {0x0000, 0xF000};

static BlitFormat blitFmt[NUM_PIXEL_FORMATS] = {BlitFormat_Dxt1,   BlitFormat_Dxt3,    BlitFormat_Argb8888, BlitFormat_Argb1555, BlitFormat_Argb4444,
                                                BlitFormat_Rgb565, BlitFormat_Unknown, BlitFormat_Dxt5,     BlitFormat_Unknown};

static CNullStatus                              s_nullStatus;
static TSGrowableArray_<BYTE, 'BLPB', __LINE__> s_blpFileLoadBuffer;

static BOOL IsLegalDimension(UINT dimension);

void CBLPFile::Close() {
  m_inMemoryImage = 0;

  FREEIFUSED(m_images);

  m_images = 0;
}

BOOL CBLPFile::CreateMipLevels(UINT width, UINT height) {
  m_images = MippedImgAllocA(PIXEL_ARGB8888, width, height, __FILE__, __LINE__);
  if (!m_images) {
    return 0;
  }

  m_numLevels = HasMips() ? CalcLevelCount(width, height) : 1;
  return 1;
}

int CBLPFile::Open(LPCSTR filename) {
  VALIDATEBEGIN;
  VALIDATE(filename);
  VALIDATEEND;

  Close();

  SFile *hsFile;
  if (!SFile::Open(filename, &hsFile)) {
    return 0;
  }

  UINT filesize = SFile::GetFileSize(hsFile, 0);
  s_blpFileLoadBuffer.SetCount(filesize);

  DWORD bytesRead;
  SFile::Read(hsFile, s_blpFileLoadBuffer.Ptr(), filesize, &bytesRead, 0, 0);
  SFile::Close(hsFile);

  return Source(s_blpFileLoadBuffer.Ptr());
}

BYTE *CBLPFile::Image(UINT level) {
  return m_images && IsValidMip(level) ? reinterpret_cast<BYTE *>(m_images->mip[level]) : 0;
}

BOOL CBLPFile::SetImage(CBLPFile &source, UINT mipLevel, CStatus *status) {
  if (!source.Image(mipLevel)) {
    if (status) {
      status->Add(STATUS_FATAL, "Tried to copy MIP %u from source image, but no such MIP exists.\n", mipLevel);
    }
    return 0;
  }

  return SetImage(source.Image(mipLevel), source.Width(), source.Height(), source.AlphaBits(), mipLevel, status);
}

BOOL CBLPFile::SetImage(LPCVOID pImg, UINT width, UINT height, UINT alphaBits, UINT mipLevel, CStatus *status) {
  VALIDATEBEGIN;
  VALIDATE(pImg);
  if (!status) {
    status = &s_nullStatus;
  }

  if (!IsLegalDimension(width) || !IsLegalDimension(height)) {
    status->Add(STATUS_FATAL, "Illegal source dimensions (%u,%u)\n", width, height);
    return 0;
  }

  if (!mipLevel) {
    m_header.width = width;
    m_header.height = height;
    m_header.alphaSize = static_cast<BYTE>(alphaBits);
  }

  if (!m_images) {
    if (!CreateMipLevels(width, height)) {
      status->Add(STATUS_FATAL, "Failed to allocate MIP buffers\n");
      return 0;
    }
    VALIDATE(m_images);
    VALIDATEEND;
  }

  if (mipLevel >= m_numLevels) {
    status->Add(STATUS_FATAL, "Illegal MIP level %u specified\n", mipLevel);
    return 0;
  }

  UINT expectedWidth = m_header.width >> mipLevel;
  UINT expectedHeight = m_header.height >> mipLevel;
  if (expectedWidth < 1) {
    expectedWidth = 1;
  }
  if (expectedHeight < 1) {
    expectedHeight = 1;
  }

  if (width != expectedWidth || height != expectedHeight) {
    status->Add(STATUS_FATAL, "Expected (%u,%u) size for MIP %u, but got (%u,%u)\n", expectedWidth, expectedHeight, mipLevel, width, height);
    return 0;
  }

  FATALASSERT(m_images->mip[mipLevel]);
  memcpy(m_images->mip[mipLevel], pImg, 4 * width * height);
  return 1;
}

static BOOL IsLegalDimension(UINT dimension) {
  switch (dimension) {
    case 1:
    case 2:
    case 4:
    case 8:
    case 16:
    case 32:
    case 64:
    case 128:
    case 256:
    case 512:
      return 1;
    default:
      return 0;
  }
}

BOOL CBLPFile::SetAlphaBits(UINT alpha) {
  if (alpha > 8) {
    return 0;
  }

  m_header.alphaSize = static_cast<BYTE>(alpha);
  return 1;
}

BOOL CBLPFile::IsValidMip(UINT level) const {
  return !level || HasMips() && level < m_numLevels;
}

static BlitFormat GetBlitFormat(PIXEL_FORMAT pixelFormat) {
  ASSERT(pixelFormat < NUM_PIXEL_FORMATS);
  return blitFmt[pixelFormat];
}

int CBLPFile::Lock(PIXEL_FORMAT format, UINT mipLevel, BYTE *&data, UINT &stride) {
  VALIDATEBEGIN;
  VALIDATE(m_inMemoryImage);
  VALIDATEEND;

  m_lockDecompMem = 0;

  if (!IsValidMip(mipLevel)) {
    return 0;
  }

  const BYTE *tempBuffer = static_cast<const BYTE *>(m_inMemoryImage) + m_header.mipOffsets[mipLevel];
  UINT        filesize = m_header.mipSizes[mipLevel];
  ASSERT(filesize);

  switch (m_header.colorEncoding) {
    case COLOR_PAL: {
      UINT size;
      if (!GetFormatSize(format, mipLevel, &size, &stride)) {
        return 0;
      }

      data = static_cast<BYTE *>(ALLOC(size));
      int result = DecompPal(format, mipLevel, data, tempBuffer);
      m_lockDecompMem = data;
      return result;
    }

    case COLOR_DXT:
      switch (format) {
        case PIXEL_DXT1:
        case PIXEL_DXT3:
        case PIXEL_DXT5:
          data = const_cast<BYTE *>(tempBuffer);
          return 1;

        case PIXEL_ARGB8888:
        case PIXEL_ARGB1555:
        case PIXEL_ARGB4444:
        case PIXEL_RGB565: {
          UINT size;
          if (!GetFormatSize(format, mipLevel, &size, &stride)) {
            return 0;
          }

          BlitFormat srcFormat = GetBlitFormat(static_cast<PIXEL_FORMAT>(m_header.preferredFormat));
          BlitFormat dstBlitFormat = GetBlitFormat(format);
          data = static_cast<BYTE *>(ALLOC(size));
          m_lockDecompMem = data;

          C2iVector mipSize(m_header.width >> mipLevel, m_header.height >> mipLevel);
          UINT      srcStride = CalcRowStride(srcFormat, m_header.width) >> mipLevel;
          Blit(mipSize, BlitAlpha_0, tempBuffer, srcStride, srcFormat, data, stride, dstBlitFormat);
          return 1;
        }

        default:
          ASSERT(0);
          return 1;
      }

    default:
      ASSERT(!"JPEG decompresion not enabled");
      return 0;
  }
}

int CBLPFile::Unlock(UINT mipLevel) {
  FREEIFUSED(m_lockDecompMem);
  return IsValidMip(mipLevel);
}

BOOL CBLPFile::GetFormatSize(PIXEL_FORMAT format, UINT mipLevel, UINT *size, UINT *stride) const {
  UINT width = m_header.width >> mipLevel;
  UINT height = m_header.height >> mipLevel;
  if (width < 1) {
    width = 1;
  }
  if (height < 1) {
    height = 1;
  }

  switch (format) {
    case PIXEL_ARGB8888:
      *size = 4 * width * height;
      *stride = 4 * width;
      return 1;

    case PIXEL_ARGB1555:
    case PIXEL_ARGB4444:
    case PIXEL_RGB565:
      *size = 2 * width * height;
      *stride = 2 * width;
      return 1;

    default:
      ASSERT(0);
      *size = 0;
      *stride = 0;
      return 0;
  }
}

MIPS_TYPE CBLPFile::HasMips() const {
  return static_cast<MIPS_TYPE>(m_header.hasMips);
}

void CBLPFile::SetHasMips(MIPS_TYPE hasMips) {
  m_header.hasMips = static_cast<BYTE>(hasMips);
}

BOOL CBLPFile::LockChain(PIXEL_FORMAT pixelFormat, MipBits *&images, UINT mipLevel) {
  if (!IsValidMip(mipLevel)) {
    return 0;
  }

  UINT width = m_header.width >> mipLevel;
  UINT height = m_header.height >> mipLevel;
  if (width < 1) {
    width = 1;
  }
  if (height < 1) {
    height = 1;
  }

  if (images) {
    MippedImgSet(pixelFormat, width, height, images);
  } else {
    images = MippedImgAllocA(pixelFormat, width, height, __FILE__, __LINE__);
    if (!images) {
      return 0;
    }
  }

  UINT level = mipLevel;
  UINT i = 0;
  while (level < m_numLevels) {
    BYTE *result;
    UINT  dummy;
    ASSERT(Lock(pixelFormat, level, result, dummy));

    UINT size = CalcLevelSize(level, m_header.width, m_header.height, pixelFormat);
    memcpy(images[i].mip[0], result, size);
    Unlock(level);

    ++level;
    ++i;
  }

  return 1;
}

BOOL CBLPFile::Source(LPVOID fileBits) {
  ASSERT(fileBits);

  Close();
  m_inMemoryImage = fileBits;
  m_inMemoryNeedsFree = 0;
  memcpy(&m_header, fileBits, sizeof(m_header));

  if (m_header.magic != m_versionMagic || m_header.formatVersion != 1) {
    return 0;
  }

  if (HasMips()) {
    m_numLevels = CalcLevelCount(m_header.width, m_header.height);
  } else {
    m_numLevels = 1;
  }

  return 1;
}

BOOL CBLPFile::LockChain2(PIXEL_FORMAT pixelFormat, MipBits *&images, UINT mipLevel) {
  if (!IsValidMip(mipLevel)) {
    return 0;
  }

  UINT width = m_header.width >> mipLevel;
  UINT height = m_header.height >> mipLevel;
  if (width < 1) {
    width = 1;
  }
  if (height < 1) {
    height = 1;
  }

  if (images) {
    MippedImgSet(pixelFormat, width, height, images);
  } else {
    images = MippedImgAllocA(pixelFormat, width, height, __FILE__, __LINE__);
    if (!images) {
      return 0;
    }
  }

  UINT level = mipLevel;
  UINT imageIndex = 0;
  while (level < m_numLevels) {
    UINT dummy;
    ASSERT(Lock2(pixelFormat, level, reinterpret_cast<BYTE *>(images[imageIndex].mip[0]), dummy));
    Unlock2(level);
    ++level;
    ++imageIndex;
  }

  m_inMemoryImage = 0;
  return 1;
}

void CBLPFile::DecompPalFastPath(BYTE *data, LPCVOID tempbuffer, UINT colorSize) {
  BYTE       *pPix = data;
  const BYTE *colorData = static_cast<const BYTE *>(tempbuffer);
  UINT        i;

  for (i = colorSize; i; --i) {
    *reinterpret_cast<UINT *>(pPix) = *reinterpret_cast<const UINT *>(&m_header.extended.palette[*colorData]);
    pPix[3] = colorData[colorSize];
    ++colorData;
    pPix += 4;
  }
}

void CBLPFile::DecompPalARGB8888(BYTE *data, LPCVOID tempBuffer, UINT colorSize) {
  const BYTE *colorData = static_cast<const BYTE *>(tempBuffer);
  const BYTE *alphaData = colorData + colorSize;
  UINT        i;

  for (i = 0; i < colorSize; ++i) {
    *reinterpret_cast<UINT *>(&data[4 * i]) = *reinterpret_cast<const UINT *>(&m_header.extended.palette[colorData[i]]);
    data[4 * i + 3] = 0xFF;
  }

  if (m_header.alphaSize == 1) {
    for (i = 0; i < colorSize; ++i) {
      data[4 * i + 3] = s_oneBitAlphaLookup[(alphaData[i >> 3] >> (i & 7)) & 1];
    }
  } else if (m_header.alphaSize == 4) {
    for (i = 0; i < colorSize; ++i) {
      BYTE alpha = alphaData[i >> 1];
      if (i & 1) {
        alpha >>= 4;
      }
      data[4 * i + 3] = s_eightBitAlphaLookup[alpha & 0x0F];
    }
  } else if (m_header.alphaSize == 8) {
    for (i = 0; i < colorSize; ++i) {
      data[4 * i + 3] = alphaData[i];
    }
  }
}

void CBLPFile::DecompPalARGB4444(BYTE *data, LPCVOID tempBuffer, UINT colorSize) {
  const BYTE *colorData = static_cast<const BYTE *>(tempBuffer);
  const BYTE *alphaData = colorData + colorSize;
  WORD        pal[256];
  WORD       *pixels = reinterpret_cast<WORD *>(data);
  UINT        i;

  for (i = 0; i < 256; ++i) {
    const BlpPalPixel &color = m_header.extended.palette[i];
    pal[i] = static_cast<WORD>(((color.r & 0xF0) << 4) | (color.g & 0xF0) | (color.b >> 4));
  }

  for (i = 0; i < colorSize; ++i) {
    pixels[i] = pal[colorData[i]];
  }

  if (m_header.alphaSize == 1) {
    for (i = 0; i < colorSize; ++i) {
      pixels[i] |= s_oneBitAlphaShort[(alphaData[i >> 3] >> (i & 7)) & 1];
    }
  } else if (m_header.alphaSize == 4) {
    for (i = 0; i < colorSize; ++i) {
      BYTE alpha = alphaData[i >> 1];
      if (i & 1) {
        pixels[i] |= static_cast<WORD>((alpha & 0xF0) << 8);
      } else {
        pixels[i] |= static_cast<WORD>((alpha & 0x0F) << 12);
      }
    }
  } else if (m_header.alphaSize == 8) {
    for (i = 0; i < colorSize; ++i) {
      pixels[i] |= static_cast<WORD>((alphaData[i] & 0xF0) << 8);
    }
  }
}

void CBLPFile::DecompPalARGB1555(BYTE *data, LPCVOID tempBuffer, UINT colorSize) {
  const BYTE *pComp = static_cast<const BYTE *>(tempBuffer);
  WORD        pal[256];
  WORD       *pPix = reinterpret_cast<WORD *>(data);
  UINT        alphaBit = 0;
  UINT        i;

  for (i = 0; i < 256; ++i) {
    const BlpPalPixel &color = m_header.extended.palette[i];
    pal[i] = static_cast<WORD>(((color.r & 0xF8) << 7) + ((color.g & 0xF8) << 2) + (color.b >> 3));
  }

  for (i = 0; i < colorSize; ++i) {
    *pPix++ = pal[*pComp++];
  }

  pPix = reinterpret_cast<WORD *>(data);

  switch (m_header.alphaSize) {
    case 1:
      for (i = 0; i < colorSize; ++i) {
        *pPix++ |= static_cast<WORD>((*pComp & (1U << alphaBit)) << (15 - alphaBit));
        if (++alphaBit >= 8) {
          alphaBit = 0;
          ++pComp;
        }
      }
      break;

    case 4:
      for (i = 0; i < colorSize; ++i) {
        if (alphaBit) {
          alphaBit = 0;
          *pPix |= static_cast<WORD>((*pComp++ & 0x80) << 8);
        } else {
          *pPix |= static_cast<WORD>((*pComp & 0x08) << 12);
          alphaBit = m_header.alphaSize;
        }
        ++pPix;
      }
      break;

    case 8:
      for (i = 0; i < colorSize; ++i) {
        *pPix++ |= static_cast<WORD>((*pComp++ & 0x80) << 8);
      }
      break;
  }
}

void CBLPFile::DecompPalARGB565(BYTE *data, LPCVOID tempBuffer, UINT colorSize) {
  WORD        pal[256];
  WORD       *pixels = reinterpret_cast<WORD *>(data);
  const BYTE *colorData = static_cast<const BYTE *>(tempBuffer);
  UINT        i;

  for (i = 0; i < 256; ++i) {
    const BlpPalPixel &color = m_header.extended.palette[i];
    pal[i] = static_cast<WORD>(((color.r & 0xF8) << 8) + ((color.g & 0xFC) << 3) + (color.b >> 3));
  }

  for (i = 0; i < colorSize; ++i) {
    *pixels = pal[*colorData];
    ++colorData;
    ++pixels;
  }
}

BOOL CBLPFile::DecompPal(PIXEL_FORMAT format, UINT mipLevel, BYTE *data, LPCVOID tempBuffer) {
  UINT width = m_header.width >> mipLevel;
  UINT height = m_header.height >> mipLevel;
  if (width < 1) {
    width = 1;
  }
  if (height < 1) {
    height = 1;
  }
  UINT colorSize = width * height;

  switch (format) {
    case PIXEL_ARGB8888:
      if (m_header.alphaSize == 8) {
        DecompPalFastPath(data, tempBuffer, colorSize);
      } else {
        DecompPalARGB8888(data, tempBuffer, colorSize);
      }
      return 1;

    case PIXEL_ARGB1555:
      DecompPalARGB1555(data, tempBuffer, colorSize);
      return 1;

    case PIXEL_ARGB4444:
      DecompPalARGB4444(data, tempBuffer, colorSize);
      return 1;

    case PIXEL_RGB565:
      DecompPalARGB565(data, tempBuffer, colorSize);
      return 1;

    default:
      return 0;
  }
}

int CBLPFile::Lock2(PIXEL_FORMAT format, UINT mipLevel, BYTE *data, UINT &stride) {
  VALIDATEBEGIN;
  VALIDATE(m_inMemoryImage);
  VALIDATEEND;

  if (!IsValidMip(mipLevel)) {
    return 0;
  }

  const BYTE *tempBuffer = static_cast<const BYTE *>(m_inMemoryImage) + m_header.mipOffsets[mipLevel];
  UINT        filesize = m_header.mipSizes[mipLevel];
  ASSERT(filesize);

  switch (m_header.colorEncoding) {
    case COLOR_PAL: {
      UINT size;
      if (!GetFormatSize(format, mipLevel, &size, &stride)) {
        return 0;
      }

      int success = DecompPal(format, mipLevel, data, tempBuffer);
      m_lockDecompMem = data;
      return success;
    }

    case COLOR_DXT:
      switch (format) {
        case PIXEL_DXT1:
        case PIXEL_DXT3:
        case PIXEL_DXT5:
          memcpy(data, tempBuffer, filesize);
          return 1;

        case PIXEL_ARGB8888:
        case PIXEL_ARGB1555:
        case PIXEL_ARGB4444:
        case PIXEL_RGB565: {
          UINT width = m_header.width >> mipLevel;
          if (width < 1) {
            width = 1;
          }

          BlitFormat srcFormat = GetBlitFormat(static_cast<PIXEL_FORMAT>(m_header.preferredFormat));
          BlitFormat dstFormat = GetBlitFormat(format);
          UINT       height = m_header.height >> mipLevel;
          if (height < 1) {
            height = 1;
          }

          UINT srcStride = CalcRowStride(srcFormat, width);
          UINT dstStride = CalcRowStride(dstFormat, width);
          Blit(C2iVector(width, height), BlitAlpha_0, tempBuffer, srcStride, srcFormat, data, dstStride, dstFormat);
          return 1;
        }

        default:
          ASSERT(!"CBLPFile::Lock2(): unhandled format");
          return 0;
      }

    default:
      ASSERT(!"JPEG decompresion not enabled");
      return 0;
  }
}

int CBLPFile::Unlock2(UINT mipLevel) {
  return IsValidMip(mipLevel);
}

UINT CBLPFile::Bytes() const {
  if (m_header.colorEncoding == COLOR_DXT) {
    return m_header.mipSizes[0];
  }

  UINT bytes;
  UINT stride;
  GetFormatSize(PIXEL_ARGB8888, 0, &bytes, &stride);
  return bytes;
}

UINT CBLPFile::Bytes(UINT mipLevel) const {
  if (m_header.colorEncoding == COLOR_DXT) {
    return m_header.mipSizes[mipLevel];
  }

  UINT bytes;
  UINT stride;
  GetFormatSize(PIXEL_ARGB8888, mipLevel, &bytes, &stride);
  return bytes;
}
