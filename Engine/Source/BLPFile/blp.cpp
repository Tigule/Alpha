#include "blp.h"

#include "Base/Status.h"
#include "Images/blit.h"
#include "Tempest/c2ivector.h"

#include <storm.h>
#include <stpl.h>

#include <string.h>

using NTempest::C2iVector;

unsigned char  CBLPFile::s_eightBitAlphaLookup[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
unsigned char  CBLPFile::s_oneBitAlphaLookup[2] = {0x00, 0xFF};
unsigned short CBLPFile::s_oneBitAlphaShort[2] = {0x0000, 0xF000};

static BlitFormat blitFmt[NUM_PIXEL_FORMATS] = {BlitFormat_Dxt1,   BlitFormat_Dxt3,    BlitFormat_Argb8888, BlitFormat_Argb1555, BlitFormat_Argb4444,
                                                BlitFormat_Rgb565, BlitFormat_Unknown, BlitFormat_Dxt5,     BlitFormat_Unknown};

static TSGrowableArray_<unsigned char, 'BLPB', 85> s_blpFileLoadBuffer;
static CNullStatus                                  s_nullStatus;

static int IsLegalDimension(unsigned int dimension);

void CBLPFile::Close() {
  m_inMemoryImage = 0;

  FREEIFUSED(m_images);

  m_images = 0;
}

int CBLPFile::CreateMipLevels(unsigned int width, unsigned int height) {
  m_images = MippedImgAllocA(PIXEL_ARGB8888, width, height, __FILE__, __LINE__);
  if (!m_images) {
    return 0;
  }

  m_numLevels = HasMips() ? CalcLevelCount(width, height) : 1;
  return 1;
}

int CBLPFile::Open(const char *filename) {
  FATALASSERT(filename);

  Close();

  SFile *hsFile;
  if (!SFile::Open(filename, &hsFile)) {
    return 0;
  }

  unsigned int filesize = SFile::GetFileSize(hsFile, 0);
  s_blpFileLoadBuffer.SetCount(filesize);

  DWORD bytesRead;
  SFile::Read(hsFile, s_blpFileLoadBuffer.Ptr(), filesize, &bytesRead, 0, 0);
  SFile::Close(hsFile);

  return Source(s_blpFileLoadBuffer.Ptr());
}

unsigned char *CBLPFile::Image(unsigned int level) {
  return m_images && IsValidMip(level) ? reinterpret_cast<unsigned char *>(m_images->mip[level]) : 0;
}

int CBLPFile::SetImage(CBLPFile &source, unsigned int mipLevel, CStatus *status) {
  if (!source.Image(mipLevel)) {
    if (status) {
      status->Add(STATUS_FATAL, "Tried to copy MIP %u from source image, but no such MIP exists.\n", mipLevel);
    }
    return 0;
  }

  return SetImage(
      source.Image(mipLevel),
      source.m_header.width,
      source.m_header.height,
      source.m_header.alphaSize,
      mipLevel,
      status
  );
}

int CBLPFile::SetImage(
    const void *pImg,
    unsigned int width,
    unsigned int height,
    unsigned int alphaBits,
    unsigned int mipLevel,
    CStatus *status
) {
  FATALASSERT(pImg);

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
    m_header.alphaSize = static_cast<unsigned char>(alphaBits);
  }

  if (!m_images) {
    if (!CreateMipLevels(width, height)) {
      status->Add(STATUS_FATAL, "Failed to allocate MIP buffers\n");
      return 0;
    }
    FATALASSERT(m_images);
  }

  if (mipLevel >= m_numLevels) {
    status->Add(STATUS_FATAL, "Illegal MIP level %u specified\n", mipLevel);
    return 0;
  }

  unsigned int expectedWidth = m_header.width >> mipLevel;
  unsigned int expectedHeight = m_header.height >> mipLevel;
  if (expectedWidth < 1) {
    expectedWidth = 1;
  }
  if (expectedHeight < 1) {
    expectedHeight = 1;
  }

  if (width != expectedWidth || height != expectedHeight) {
    status->Add(
        STATUS_FATAL,
        "Expected (%u,%u) size for MIP %u, but got (%u,%u)\n",
        expectedWidth,
        expectedHeight,
        mipLevel,
        width,
        height
    );
    return 0;
  }

  FATALASSERT(m_images->mip[mipLevel]);
  memcpy(m_images->mip[mipLevel], pImg, 4 * width * height);
  return 1;
}

static int IsLegalDimension(unsigned int dimension) {
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

int CBLPFile::SetAlphaBits(unsigned int alpha) {
  if (alpha > 8) {
    return 0;
  }

  m_header.alphaSize = static_cast<unsigned char>(alpha);
  return 1;
}

int CBLPFile::IsValidMip(unsigned int level) const {
  return !level || HasMips() && level < m_numLevels;
}

static BlitFormat GetBlitFormat(PIXEL_FORMAT pixelFormat) {
  ASSERT(pixelFormat < NUM_PIXEL_FORMATS);
  return blitFmt[pixelFormat];
}

int CBLPFile::Lock(PIXEL_FORMAT format, unsigned int mipLevel, unsigned char *&data, unsigned int &stride) {
  FATALASSERT(m_inMemoryImage);

  m_lockDecompMem = 0;

  if (!IsValidMip(mipLevel)) {
    return 0;
  }

  const unsigned char *tempBuffer = static_cast<const unsigned char *>(m_inMemoryImage) + m_header.mipOffsets[mipLevel];
  unsigned int         filesize = m_header.mipSizes[mipLevel];
  ASSERT(filesize);

  switch (m_header.colorEncoding) {
    case COLOR_PAL: {
      unsigned int size;
      if (!GetFormatSize(format, mipLevel, &size, &stride)) {
        return 0;
      }

      data = static_cast<unsigned char *>(ALLOC(size));
      int result = DecompPal(format, mipLevel, data, tempBuffer);
      m_lockDecompMem = data;
      return result;
    }

    case COLOR_DXT:
      switch (format) {
        case PIXEL_DXT1:
        case PIXEL_DXT3:
        case PIXEL_DXT5:
          data = const_cast<unsigned char *>(tempBuffer);
          return 1;

        case PIXEL_ARGB8888:
        case PIXEL_ARGB1555:
        case PIXEL_ARGB4444:
        case PIXEL_RGB565: {
          unsigned int size;
          if (!GetFormatSize(format, mipLevel, &size, &stride)) {
            return 0;
          }

          BlitFormat srcFormat = GetBlitFormat(static_cast<PIXEL_FORMAT>(m_header.preferredFormat));
          BlitFormat dstBlitFormat = GetBlitFormat(format);
          data = static_cast<unsigned char *>(ALLOC(size));
          m_lockDecompMem = data;

          C2iVector    mipSize(m_header.width >> mipLevel, m_header.height >> mipLevel);
          unsigned int srcStride = CalcRowStride(srcFormat, m_header.width) >> mipLevel;
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

int CBLPFile::Unlock(unsigned int mipLevel) {
  FREEIFUSED(m_lockDecompMem);
  return IsValidMip(mipLevel);
}

int CBLPFile::GetFormatSize(PIXEL_FORMAT format, unsigned int mipLevel, unsigned int *size, unsigned int *stride) const {
  unsigned int width = m_header.width >> mipLevel;
  unsigned int height = m_header.height >> mipLevel;
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
  m_header.hasMips = static_cast<unsigned char>(hasMips);
}

int CBLPFile::LockChain(PIXEL_FORMAT pixelFormat, MipBits *&images, unsigned int mipLevel) {
  if (!IsValidMip(mipLevel)) {
    return 0;
  }

  unsigned int width = m_header.width >> mipLevel;
  unsigned int height = m_header.height >> mipLevel;
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

  unsigned int level = mipLevel;
  unsigned int i = 0;
  while (level < m_numLevels) {
    unsigned char *result;
    unsigned int   dummy;
    ASSERT(Lock(pixelFormat, level, result, dummy));

    unsigned int size = CalcLevelSize(level, m_header.width, m_header.height, pixelFormat);
    memcpy(images[i].mip[0], result, size);
    Unlock(level);

    ++level;
    ++i;
  }

  return 1;
}

int CBLPFile::Source(void *fileBits) {
  ASSERT(fileBits);

  Close();
  m_inMemoryImage = fileBits;
  m_inMemoryNeedsFree = 0;
  memcpy(&m_header, fileBits, sizeof(m_header));

  if (m_header.magic != 0x32504C42 || m_header.formatVersion != 1) {
    return 0;
  }

  if (HasMips()) {
    m_numLevels = CalcLevelCount(m_header.width, m_header.height);
  } else {
    m_numLevels = 1;
  }

  return 1;
}

int CBLPFile::LockChain2(PIXEL_FORMAT pixelFormat, MipBits *&images, unsigned int mipLevel) {
  if (!IsValidMip(mipLevel)) {
    return 0;
  }

  unsigned int width = m_header.width >> mipLevel;
  unsigned int height = m_header.height >> mipLevel;
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

  unsigned int level = mipLevel;
  unsigned int imageIndex = 0;
  while (level < m_numLevels) {
    unsigned int dummy;
    ASSERT(Lock2(pixelFormat, level, reinterpret_cast<unsigned char *>(images[imageIndex].mip[0]), dummy));
    Unlock2(level);
    ++level;
    ++imageIndex;
  }

  m_inMemoryImage = 0;
  return 1;
}

void CBLPFile::DecompPalFastPath(unsigned char *data, const void *tempBuffer, unsigned int colorSize) {
  const unsigned char *colorData = static_cast<const unsigned char *>(tempBuffer);
  const unsigned char *alphaData = colorData + colorSize;
  unsigned int         i;

  for (i = 0; i < colorSize; ++i) {
    *reinterpret_cast<unsigned int *>(data) = *reinterpret_cast<const unsigned int *>(&m_header.extended.palette[colorData[i]]);
    data[3] = alphaData[i];
    data += 4;
  }
}

void CBLPFile::DecompPalARGB8888(unsigned char *data, const void *tempBuffer, unsigned int colorSize) {
  const unsigned char *colorData = static_cast<const unsigned char *>(tempBuffer);
  const unsigned char *alphaData = colorData + colorSize;
  unsigned int         i;

  for (i = 0; i < colorSize; ++i) {
    *reinterpret_cast<unsigned int *>(&data[4 * i]) = *reinterpret_cast<const unsigned int *>(&m_header.extended.palette[colorData[i]]);
    data[4 * i + 3] = 0xFF;
  }

  if (m_header.alphaSize == 1) {
    for (i = 0; i < colorSize; ++i) {
      data[4 * i + 3] = s_oneBitAlphaLookup[(alphaData[i >> 3] >> (i & 7)) & 1];
    }
  } else if (m_header.alphaSize == 4) {
    for (i = 0; i < colorSize; ++i) {
      unsigned char alpha = alphaData[i >> 1];
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

void CBLPFile::DecompPalARGB4444(unsigned char *data, const void *tempBuffer, unsigned int colorSize) {
  const unsigned char *colorData = static_cast<const unsigned char *>(tempBuffer);
  const unsigned char *alphaData = colorData + colorSize;
  unsigned short       pal[256];
  unsigned short      *pixels = reinterpret_cast<unsigned short *>(data);
  unsigned int         i;

  for (i = 0; i < 256; ++i) {
    const BlpPalPixel &color = m_header.extended.palette[i];
    pal[i] = static_cast<unsigned short>(((color.r & 0xF0) << 4) | (color.g & 0xF0) | (color.b >> 4));
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
      unsigned char alpha = alphaData[i >> 1];
      if (i & 1) {
        pixels[i] |= static_cast<unsigned short>((alpha & 0xF0) << 8);
      } else {
        pixels[i] |= static_cast<unsigned short>((alpha & 0x0F) << 12);
      }
    }
  } else if (m_header.alphaSize == 8) {
    for (i = 0; i < colorSize; ++i) {
      pixels[i] |= static_cast<unsigned short>((alphaData[i] & 0xF0) << 8);
    }
  }
}

void CBLPFile::DecompPalARGB1555(unsigned char *data, const void *tempBuffer, unsigned int colorSize) {
  const unsigned char *colorData = static_cast<const unsigned char *>(tempBuffer);
  const unsigned char *alphaData = colorData + colorSize;
  unsigned short       pal[256];
  unsigned short      *pPix = reinterpret_cast<unsigned short *>(data);
  unsigned int         i;

  for (i = 0; i < 256; ++i) {
    const BlpPalPixel &color = m_header.extended.palette[i];
    pal[i] = static_cast<unsigned short>(((color.r & 0xF8) << 7) | ((color.g & 0xF8) << 2) | (color.b >> 3));
  }

  for (i = 0; i < colorSize; ++i) {
    pPix[i] = pal[colorData[i]];
  }

  if (m_header.alphaSize == 1) {
    for (i = 0; i < colorSize; ++i) {
      unsigned int alphaBit = i & 7;
      pPix[i] |= static_cast<unsigned short>((alphaData[i >> 3] & (1U << alphaBit)) << (15 - alphaBit));
    }
  } else if (m_header.alphaSize == 4) {
    for (i = 0; i < colorSize; ++i) {
      unsigned char alpha = alphaData[i >> 1];
      if (i & 1) {
        pPix[i] |= static_cast<unsigned short>((alpha & 0x80) << 8);
      } else {
        pPix[i] |= static_cast<unsigned short>((alpha & 0x08) << 12);
      }
    }
  } else if (m_header.alphaSize == 8) {
    for (i = 0; i < colorSize; ++i) {
      pPix[i] |= static_cast<unsigned short>((alphaData[i] & 0x80) << 8);
    }
  }
}

void CBLPFile::DecompPalARGB565(unsigned char *data, const void *tempBuffer, unsigned int colorSize) {
  const unsigned char *colorData = static_cast<const unsigned char *>(tempBuffer);
  unsigned short       pal[256];
  unsigned short      *pixels = reinterpret_cast<unsigned short *>(data);
  unsigned int         i;

  for (i = 0; i < 256; ++i) {
    const BlpPalPixel &color = m_header.extended.palette[i];
    pal[i] = static_cast<unsigned short>(((color.r & 0xF8) << 8) | ((color.g & 0xFC) << 3) | (color.b >> 3));
  }

  for (i = 0; i < colorSize; ++i) {
    pixels[i] = pal[colorData[i]];
  }
}

int CBLPFile::DecompPal(PIXEL_FORMAT format, unsigned int mipLevel, unsigned char *data, const void *tempBuffer) {
  unsigned int width = m_header.width >> mipLevel;
  unsigned int height = m_header.height >> mipLevel;
  if (width < 1) {
    width = 1;
  }
  if (height < 1) {
    height = 1;
  }
  unsigned int colorSize = width * height;

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

int CBLPFile::Lock2(PIXEL_FORMAT format, unsigned int mipLevel, unsigned char *data, unsigned int &stride) {
  FATALASSERT(m_inMemoryImage);

  if (!IsValidMip(mipLevel)) {
    return 0;
  }

  const unsigned char *tempBuffer = static_cast<const unsigned char *>(m_inMemoryImage) + m_header.mipOffsets[mipLevel];
  unsigned int         filesize = m_header.mipSizes[mipLevel];
  ASSERT(filesize);

  switch (m_header.colorEncoding) {
    case COLOR_PAL: {
      unsigned int size;
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
          unsigned int width = m_header.width >> mipLevel;
          unsigned int height = m_header.height >> mipLevel;
          if (width < 1) {
            width = 1;
          }
          if (height < 1) {
            height = 1;
          }

          BlitFormat   srcFormat = GetBlitFormat(static_cast<PIXEL_FORMAT>(m_header.preferredFormat));
          BlitFormat   dstFormat = GetBlitFormat(format);
          C2iVector    mipSize(width, height);
          unsigned int srcStride = CalcRowStride(srcFormat, width);
          unsigned int dstStride = CalcRowStride(dstFormat, width);
          Blit(mipSize, BlitAlpha_0, tempBuffer, srcStride, srcFormat, data, dstStride, dstFormat);
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

int CBLPFile::Unlock2(unsigned int mipLevel) {
  return IsValidMip(mipLevel);
}

unsigned int CBLPFile::Bytes() const {
  if (m_header.colorEncoding == COLOR_DXT) {
    return m_header.mipSizes[0];
  }

  unsigned int size;
  unsigned int stride;
  GetFormatSize(PIXEL_ARGB8888, 0, &size, &stride);
  return size;
}

unsigned int CBLPFile::Bytes(unsigned int mipLevel) const {
  if (m_header.colorEncoding == COLOR_DXT) {
    return m_header.mipSizes[mipLevel];
  }

  unsigned int size;
  unsigned int stride;
  GetFormatSize(PIXEL_ARGB8888, mipLevel, &size, &stride);
  return size;
}
