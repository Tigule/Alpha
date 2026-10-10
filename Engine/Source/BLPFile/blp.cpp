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

  s_blpFileLoadBuffer.SetCount(SFile::GetFileSize(hsFile, 0));

  DWORD bytesRead;
  SFile::Read(hsFile, s_blpFileLoadBuffer.Ptr(), s_blpFileLoadBuffer.Count(), &bytesRead, 0, 0);
  SFile::Close(hsFile);

  return Source(s_blpFileLoadBuffer.Ptr());
}

BYTE *CBLPFile::Image(UINT level) {
  return m_images && IsValidMip(level) ? (BYTE *)m_images->mip[level] : 0;
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
    m_header.alphaSize = alphaBits;
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

  UINT expectedWidth = Width(mipLevel);
  UINT expectedHeight = Height(mipLevel);
  if (width != expectedWidth || height != expectedHeight) {
    status->Add(STATUS_FATAL, "Expected (%u,%u) size for MIP %u, but got (%u,%u)\n", expectedWidth, expectedHeight, mipLevel, width, height);
    return 0;
  }

  VALIDATE(m_images->mip[mipLevel]);
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

  m_header.alphaSize = alpha;
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

  LPCVOID tempBuffer = (BYTE *)m_inMemoryImage + m_header.mipOffsets[mipLevel];
  UINT    filesize = m_header.mipSizes[mipLevel];
  ASSERT(filesize);

  UINT size;

  switch (m_header.colorEncoding) {
    case COLOR_PAL: {
      if (!GetFormatSize(format, mipLevel, &size, &stride)) {
        return 0;
      }

      data = (BYTE *)ALLOC(size);
      int result = DecompPal(format, mipLevel, data, tempBuffer);
      m_lockDecompMem = data;
      return result;
    }

    case COLOR_DXT:
      switch (format) {
        case PIXEL_ARGB8888:
        case PIXEL_ARGB1555:
        case PIXEL_ARGB4444:
        case PIXEL_RGB565: {
          if (!GetFormatSize(format, mipLevel, &size, &stride)) {
            return 0;
          }

          BlitFormat srcFormat = GetBlitFormat((PIXEL_FORMAT)m_header.preferredFormat);
          BlitFormat dstBlitFormat = GetBlitFormat(format);
          data = (BYTE *)ALLOC(size);
          m_lockDecompMem = data;

          C2iVector mipSize(m_header.width >> mipLevel, m_header.height >> mipLevel);
          Blit(mipSize, BlitAlpha_0, tempBuffer, CalcRowStride(srcFormat, m_header.width) >> mipLevel, srcFormat, data, stride, dstBlitFormat);
          return 1;
        }

        case PIXEL_DXT1:
        case PIXEL_DXT3:
        case PIXEL_DXT5:
          data = (BYTE *)tempBuffer;
          return 1;

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
  UINT width = Width(mipLevel);
  UINT pixels = width * Height(mipLevel);

  switch (format) {
    case PIXEL_ARGB8888:
      *size = 4 * pixels;
      *stride = 4 * width;
      return 1;

    case PIXEL_ARGB1555:
    case PIXEL_ARGB4444:
    case PIXEL_RGB565:
      *size = 2 * pixels;
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
  return (MIPS_TYPE)m_header.hasMips;
}

void CBLPFile::SetHasMips(MIPS_TYPE hasMips) {
  m_header.hasMips = hasMips;
}

BOOL CBLPFile::LockChain(PIXEL_FORMAT pixelFormat, MipBits *&images, UINT mipLevel) {
  if (!IsValidMip(mipLevel)) {
    return 0;
  }

  if (!images) {
    images = MippedImgAllocA(pixelFormat, Width(mipLevel), Height(mipLevel), __FILE__, __LINE__);
    if (!images) {
      return 0;
    }
  } else {
    MippedImgSet(pixelFormat, Width(mipLevel), Height(mipLevel), images);
  }

  for (UINT i = mipLevel; i < m_numLevels; ++i) {
    BYTE *result;
    UINT  dummy;
    if (!Lock(pixelFormat, i, result, dummy)) {
      SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "1", FALSE);
    }

    memcpy(images[i - mipLevel].mip[0], result, CalcLevelSize(i, Width(), Height(), pixelFormat));
    Unlock(i);
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

  m_numLevels = HasMips() ? CalcLevelCount(m_header.width, m_header.height) : 1;
  return 1;
}

BOOL CBLPFile::LockChain2(PIXEL_FORMAT pixelFormat, MipBits *&images, UINT mipLevel) {
  if (!IsValidMip(mipLevel)) {
    return 0;
  }

  if (!images) {
    images = MippedImgAllocA(pixelFormat, Width(mipLevel), Height(mipLevel), __FILE__, __LINE__);
    if (!images) {
      return 0;
    }
  } else {
    MippedImgSet(pixelFormat, Width(mipLevel), Height(mipLevel), images);
  }

  for (UINT i = mipLevel; i < m_numLevels; ++i) {
    UINT dummy;
    if (!Lock2(pixelFormat, i, (BYTE *)images[i - mipLevel].mip[0], dummy)) {
      SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "1", FALSE);
    }

    Unlock2(i);
  }

  m_inMemoryImage = 0;
  return 1;
}

void CBLPFile::DecompPalFastPath(BYTE *data, LPCVOID tempbuffer, UINT colorSize) {
  BYTE       *pPix = data;
  const BYTE *colorData = (const BYTE *)tempbuffer;
  UINT        i;

  for (i = colorSize; i; --i) {
    *(UINT *)pPix = *(const UINT *)&m_header.extended.palette[*colorData];
    pPix[3] = colorData[colorSize];
    ++colorData;
    pPix += 4;
  }
}

void CBLPFile::DecompPalARGB8888(BYTE *data, LPCVOID tempBuffer, UINT colorSize) {
  BYTE       *pPix = data;
  const BYTE *pComp = (const BYTE *)tempBuffer;
  UINT        i;

  for (i = 0; i < colorSize; ++i) {
    *(UINT *)pPix = *(const UINT *)&m_header.extended.palette[*pComp];
    pPix[3] = 0xFF;
    ++pComp;
    pPix += 4;
  }

  pPix = data;

  switch (m_header.alphaSize) {
    case 1: {
      for (i = 0; i < colorSize >> 3; ++i) {
        BYTE alpha = pComp[i];
        for (UINT j = 0; j < 8; ++j) {
          pPix[4 * j + 3] = s_oneBitAlphaLookup[alpha & 1];
          alpha >>= 1;
        }
        pPix += 32;
      }

      if (colorSize & 7) {
        BYTE alpha = pComp[colorSize >> 3];
        for (UINT j = 0; j < (colorSize & 7); ++j) {
          pPix[3] = s_oneBitAlphaLookup[alpha & 1];
          alpha >>= 1;
          pPix += 4;
        }
      }
      break;
    }

    case 4:
      for (i = 0; i < colorSize >> 1; ++i) {
        pPix[3] = s_eightBitAlphaLookup[*pComp & 0x0F];
        pPix += 4;
        pPix[3] = s_eightBitAlphaLookup[*pComp >> 4];
        pPix += 4;
        ++pComp;
      }

      if (colorSize & 1) {
        pPix[3] = s_eightBitAlphaLookup[*pComp & 0x0F];
      }
      break;

    case 8:
      for (i = 0; i < colorSize; ++i) {
        pPix[3] = *pComp;
        pPix += 4;
        ++pComp;
      }
      break;
  }
}

void CBLPFile::DecompPalARGB4444(BYTE *data, LPCVOID tempBuffer, UINT colorSize) {
  WORD        pal[256];
  WORD       *pPix = (WORD *)data;
  const BYTE *pComp = (const BYTE *)tempBuffer;
  UINT        i;

  for (i = 0; i < 256; ++i) {
    pal[i] = ((m_header.extended.palette[i].r & 0xF0) << 4) + (m_header.extended.palette[i].g & 0xF0) + (m_header.extended.palette[i].b >> 4);
  }

  for (i = 0; i < colorSize; ++i) {
    *pPix = pal[*pComp];
    ++pComp;
    ++pPix;
  }

  pPix = (WORD *)data;

  switch (m_header.alphaSize) {
    case 1: {
      UINT alphaBit = 1;
      for (i = 0; i < colorSize; ++i) {
        *pPix++ |= s_oneBitAlphaShort[*pComp & alphaBit];
        alphaBit <<= 1;
        if (alphaBit >= 0x100) {
          alphaBit = 1;
          ++pComp;
        }
      }
      break;
    }

    case 4:
      for (i = 0; i < colorSize; ++i) {
        if (i & 1) {
          *pPix |= (*pComp & 0xF0) << 8;
          ++pComp;
        } else {
          *pPix |= *pComp << 12;
        }
        ++pPix;
      }
      break;

    case 8:
      for (i = 0; i < colorSize; ++i) {
        *pPix++ |= (*pComp++ & 0xF0) << 8;
      }
      break;
  }
}

void CBLPFile::DecompPalARGB1555(BYTE *data, LPCVOID tempBuffer, UINT colorSize) {
  WORD        pal[256];
  WORD       *pPix = (WORD *)data;
  const BYTE *pComp = (const BYTE *)tempBuffer;
  UINT        alphaBit = 0;
  UINT        i;

  for (i = 0; i < 256; ++i) {
    pal[i] = ((m_header.extended.palette[i].r & 0xF8) << 7) + ((m_header.extended.palette[i].g & 0xF8) << 2) + (m_header.extended.palette[i].b >> 3);
  }

  for (i = 0; i < colorSize; ++i) {
    *pPix = pal[*pComp];
    ++pComp;
    ++pPix;
  }

  pPix = (WORD *)data;

  switch (m_header.alphaSize) {
    case 1:
      for (i = 0; i < colorSize; ++i) {
        *pPix++ |= (*pComp & (1U << alphaBit)) << (15 - alphaBit);
        if (++alphaBit >= 8) {
          alphaBit = 0;
          ++pComp;
        }
      }
      break;

    case 4:
      for (i = 0; i < colorSize; ++i) {
        if (!alphaBit) {
          *pPix |= (*pComp & ~7) << 12;
          alphaBit = m_header.alphaSize;
        } else {
          *pPix |= (*pComp & 0x80) << 8;
          alphaBit = 0;
          ++pComp;
        }
        ++pPix;
      }
      break;

    case 8:
      for (i = 0; i < colorSize; ++i) {
        *pPix++ |= (*pComp++ & 0x80) << 8;
      }
      break;
  }
}

void CBLPFile::DecompPalARGB565(BYTE *data, LPCVOID tempBuffer, UINT colorSize) {
  WORD        pal[256];
  WORD       *pixels = (WORD *)data;
  const BYTE *colorData = (const BYTE *)tempBuffer;
  UINT        i;

  for (i = 0; i < 256; ++i) {
    const BlpPalPixel &color = m_header.extended.palette[i];
    pal[i] = ((color.r & 0xF8) << 8) + ((color.g & 0xFC) << 3) + (color.b >> 3);
  }

  for (i = 0; i < colorSize; ++i) {
    *pixels = pal[*colorData];
    ++colorData;
    ++pixels;
  }
}

BOOL CBLPFile::DecompPal(PIXEL_FORMAT format, UINT mipLevel, BYTE *data, LPCVOID tempBuffer) {
  UINT colorSize = Height(mipLevel) * Width(mipLevel);

  switch (format) {
    case PIXEL_ARGB8888:
      if (m_header.alphaSize == 8) {
        DecompPalFastPath(data, tempBuffer, colorSize);
      } else {
        DecompPalARGB8888(data, tempBuffer, colorSize);
      }
      return 1;

    case PIXEL_ARGB4444:
      DecompPalARGB4444(data, tempBuffer, colorSize);
      return 1;

    case PIXEL_ARGB1555:
      DecompPalARGB1555(data, tempBuffer, colorSize);
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

  const BYTE *tempBuffer = (const BYTE *)m_inMemoryImage + m_header.mipOffsets[mipLevel];
  UINT        filesize = m_header.mipSizes[mipLevel];
  ASSERT(filesize);

  int success = 0;

  switch (m_header.colorEncoding) {
    case COLOR_PAL: {
      UINT size;
      if (!GetFormatSize(format, mipLevel, &size, &stride)) {
        return 0;
      }

      success = DecompPal(format, mipLevel, data, tempBuffer);
      m_lockDecompMem = data;
      break;
    }

    case COLOR_DXT:
      switch (format) {
        case PIXEL_DXT1:
        case PIXEL_DXT3:
        case PIXEL_DXT5:
          memcpy(data, tempBuffer, filesize);
          success = 1;
          break;

        case PIXEL_ARGB8888:
        case PIXEL_ARGB1555:
        case PIXEL_ARGB4444:
        case PIXEL_RGB565: {
          UINT       width = Width(mipLevel);
          BlitFormat srcFormat = GetBlitFormat(GetPreferredFormat());
          BlitFormat dstBlitFormat = GetBlitFormat(format);
          Blit(
              C2iVector(width, Height(mipLevel)),
              BlitAlpha_0,
              tempBuffer,
              CalcRowStride(srcFormat, width),
              srcFormat,
              data,
              CalcRowStride(dstBlitFormat, width),
              dstBlitFormat
          );
          success = 1;
          break;
        }

        default:
          ASSERT(!"CBLPFile::Lock2(): unhandled format");
          break;
      }
      break;

    default:
      ASSERT(!"JPEG decompresion not enabled");
      success = 0;
      break;
  }

  return success;
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
