#include "tga.h"

#include <Os/W32/OsFile.h>

#include <string.h>
void CTgaFile::Close() {
  FREEIFUSED(m_image);

  m_image = 0;

  if (m_file) {
    SFile::Close(m_file);
  }

  m_file = 0;
  RemoveHeaderTrailer();

  FREEIFUSED(m_colorMap);

  m_colorMap = 0;
}

int CTgaFile::Open(const char *filename) {
  FATALASSERT(filename);

  FATALASSERT(*filename);

  Close();

  if (!SFile::Open(filename, &m_file)) {
    return 0;
  }

  if (!SFile::Read(m_file, &m_header, sizeof(m_header), 0, 0, 0)) {
    return 0;
  }

  if (m_header.bIDLength) {
    m_addlHeaderData = static_cast<unsigned char *>(ALLOC(m_header.bIDLength));
    if (!SFile::Read(m_file, m_addlHeaderData, m_header.bIDLength, 0, 0, 0)) {
      return 0;
    }
  } else {
    m_addlHeaderData = 0;
  }

  if (m_header.bColorMapType) {
    m_colorMap = static_cast<unsigned char *>(ALLOC(ColorMapBytes()));
    if (!SFile::Read(m_file, m_colorMap, ColorMapBytes(), 0, 0, 0)) {
      return 0;
    }
  } else {
    m_colorMap = 0;
  }

  if (m_header.bPixelDepth == 24 && m_header.Desc.bAlphaChannelBits == 8) {
    m_header.Desc.bAlphaChannelBits = 0;
    SErrSetLastError(0x8720012E);
  }

  return 1;
}

int CTgaFile::ValidateColorDepth() {
  if (m_header.bPixelDepth - m_header.Desc.bAlphaChannelBits == 24) {
    return 1;
  }

  if (m_header.bPixelDepth == 16) {
    SErrSetLastError(0xF720007C);
  } else {
    SErrSetLastError(0xF720007D);
  }

  return 0;
}

void CTgaFile::ConvertColorMapped(unsigned int flags) {
  unsigned int   newPixelDepth = m_header.Desc.bAlphaChannelBits + 24;
  unsigned int   addAlpha = flags & 1;
  unsigned int   pixelCount = m_header.wWidth * m_header.wHeight;
  unsigned char *oldImage = m_image;

  m_image = static_cast<unsigned char *>(ALLOC(pixelCount * (addAlpha + (newPixelDepth >> 3))));

  unsigned char *dstPix = m_image + addAlpha * pixelCount;
  unsigned char *srcPix = oldImage;
  unsigned int   pixel;
  for (pixel = 0; pixel < pixelCount; ++pixel) {
    memcpy(dstPix, &m_colorMap[ColorMapEntryBytes() * (*srcPix - m_header.wColorMapStartIndex)], ColorMapEntryBytes());
    dstPix += ColorMapEntryBytes();
    ++srcPix;
  }

  FREE(oldImage);
  FREE(m_colorMap);

  m_header.bPixelDepth = static_cast<unsigned char>(newPixelDepth);
  m_colorMap = 0;
  m_header.wColorMapEntries = 0;
  m_header.bColorMapType = 0;
  m_header.bImageType = 2;
  m_imageBytes = m_header.wWidth * m_header.wHeight * ((m_header.bPixelDepth + 7) / 8);

  if (addAlpha) {
    AddAlphaChannel(m_image, m_image + pixelCount, 0);
  }
}

int CTgaFile::ReadColorMappedImage(unsigned int flags) {
  if (!m_header.bColorMapType) {
    SErrSetLastError(0xF7200084);
    return 0;
  }

  int result;
  if (m_header.bImageType >= 9) {
    result = ReadRleImage(0);
  } else {
    result = ReadRawImage(0);
  }

  if (m_header.bColorMapType && (flags & 2)) {
    ConvertColorMapped(flags);
  }

  return result;
}

int CTgaFile::LoadImageData(unsigned int flags) {
  FATALASSERT(m_image == 0);

  if (!m_file) {
    SErrSetLastError(0xF720007E);
    return 0;
  }

  m_imageBytes = m_header.wWidth * m_header.wHeight * ((m_header.bPixelDepth + 7) / 8);

  switch (m_header.bImageType) {
    case 0:
      SErrSetLastError(0xF7200078);
      return 0;

    case 1:
    case 9:
      return ReadColorMappedImage(flags);

    case 2:
      if (!ValidateColorDepth()) {
        return 0;
      }
      return ReadRawImage(flags);

    case 3:
    case 11:
      SErrSetLastError(0xF720007A);
      return 0;

    case 10:
      if (!ValidateColorDepth()) {
        return 0;
      }
      return ReadRleImage(flags);

    default:
      SErrSetLastError(0xF720007B);
      return 0;
  }
}

unsigned long CTgaFile::PreImageBytes() {
  return sizeof(m_header) + m_header.bIDLength + ColorMapBytes();
}

void CTgaFile::AddAlphaChannel(unsigned char *pAlphaData, unsigned char *pNoAlphaData, const unsigned char *alpha) {
  unsigned int pixelCount = m_header.wWidth * m_header.wHeight;
  unsigned int pixelBytes = (m_header.bPixelDepth + 7) / 8;
  unsigned int pixel;

  for (pixel = 0; pixel < pixelCount; ++pixel) {
    memmove(pAlphaData, pNoAlphaData, pixelBytes);
    pAlphaData += pixelBytes;
    pNoAlphaData += pixelBytes;

    if (alpha) {
      *pAlphaData = *alpha++;
    } else {
      *pAlphaData = 0xFF;
    }
    ++pAlphaData;
  }

  m_header.bPixelDepth = static_cast<unsigned char>(m_header.bPixelDepth + 8 - m_header.Desc.bAlphaChannelBits);
  m_header.Desc.bAlphaChannelBits = 8;
  m_imageBytes = m_header.wWidth * m_header.wHeight * ((m_header.bPixelDepth + 7) / 8);
}

int CTgaFile::ReadRawImage(unsigned int flags) {
  int addAlpha = (flags & 1) && m_header.Desc.bAlphaChannelBits == 0;

  if (SFile::SetFilePointer(m_file, PreImageBytes(), 0, FILE_BEGIN) == static_cast<unsigned long>(-1)) {
    return 0;
  }

  unsigned int pixelCount = m_header.wWidth * m_header.wHeight;
  unsigned int sourceBytes = pixelCount * ((m_header.bPixelDepth + 7) / 8);
  ASSERT(m_image == 0);

  m_image = static_cast<unsigned char *>(ALLOC(sourceBytes + addAlpha * pixelCount));
  if (!m_image) {
    return 0;
  }

  if (!SFile::Read(m_file, m_image + addAlpha * pixelCount, sourceBytes, 0, 0, 0)) {
    return 0;
  }

  if (addAlpha) {
    AddAlphaChannel(m_image, m_image + pixelCount, 0);
  }

  return 1;
}

int CTgaFile::ReadRleImage(unsigned int flags) {
  int          addAlpha = (flags & 1) && m_header.Desc.bAlphaChannelBits == 0;
  unsigned int pixelCount = m_header.wWidth * m_header.wHeight;
  unsigned int imageBytes = pixelCount * ((m_header.bPixelDepth + 7) / 8);
  ASSERT(m_image == 0);

  m_image = static_cast<unsigned char *>(ALLOC(imageBytes + addAlpha * pixelCount));
  if (!m_image) {
    return 0;
  }

  unsigned long rleBytes = SFile::GetFileSize(m_file, 0) - PreImageBytes();
  if (rleBytes == static_cast<unsigned long>(-1)) {
    return 0;
  }

  unsigned char *rleData = static_cast<unsigned char *>(ALLOC(rleBytes));
  if (!rleData) {
    return 0;
  }

  if (SFile::SetFilePointer(m_file, PreImageBytes(), 0, FILE_BEGIN) == static_cast<unsigned long>(-1)) {
    return 0;
  }

  if (!SFile::Read(m_file, rleData, rleBytes, 0, 0, 0)) {
    return 0;
  }

  int result = RLEDecompressImage(rleData, m_image + addAlpha * pixelCount);
  FREE(rleData);
  if (!result) {
    return 0;
  }

  if (addAlpha) {
    AddAlphaChannel(m_image, m_image + pixelCount, 0);
  }

  return 1;
}

int CTgaFile::RLEDecompressImage(unsigned char *pRLEData, unsigned char *pData) {
  ASSERT(pRLEData);

  int          pixelsRemaining = m_header.wWidth * m_header.wHeight;
  unsigned int pixelBytes = (m_header.bPixelDepth + 7) / 8;

  while (pixelsRemaining) {
    unsigned int packetHeader = *pRLEData++;
    int          packetPixels;

    if (packetHeader & 0x80) {
      packetPixels = (packetHeader & 0x7F) + 1;
      pixelsRemaining -= packetPixels;

      int pixel;
      for (pixel = 0; pixel < packetPixels; ++pixel) {
        memcpy(pData, pRLEData, pixelBytes);
        pData += pixelBytes;
      }
      pRLEData += pixelBytes;
    } else {
      packetPixels = packetHeader + 1;
      memcpy(pData, pRLEData, pixelBytes * packetPixels);
      pixelsRemaining -= packetPixels;
      pRLEData += pixelBytes * packetPixels;
      pData += pixelBytes * packetPixels;
    }

    if (pixelsRemaining < 0) {
      SErrSetLastError(0xF7200077);
      return 0;
    }
  }

  m_header.bImageType -= 8;
  return 1;
}

int CTgaFile::AddAlphaChannel(const void *pImg) {
  if (!Image()) {
    return 0;
  }

  if (m_header.bImageType >= 9 && m_header.bImageType <= 11) {
    SErrSetLastError(0xF7200083);
    return 0;
  }

  if (m_header.Desc.bAlphaChannelBits) {
    RemoveAlphaChannels();
  }

  unsigned int   pixelCount = m_header.wWidth * m_header.wHeight;
  unsigned char *newImage = static_cast<unsigned char *>(ALLOC(pixelCount * (((m_header.bPixelDepth + 7) / 8) + 1)));
  if (!newImage) {
    return 0;
  }

  AddAlphaChannel(newImage, m_image, static_cast<const unsigned char *>(pImg));
  FREE(m_image);
  m_image = newImage;
  return 1;
}

int CTgaFile::SetTopDown(int set) {
  if ((set && m_header.Desc.bTopBottomOrder) || (!set && !m_header.Desc.bTopBottomOrder)) {
    return 1;
  }

  if (m_header.bImageType >= 9 && m_header.bImageType <= 11) {
    SErrSetLastError(0xF7200083);
    return 0;
  }

  unsigned int   rowBytes = m_header.wWidth * ((m_header.bPixelDepth + 7) / 8);
  unsigned char *newImage = static_cast<unsigned char *>(ALLOC(m_header.wHeight * rowBytes));
  unsigned char *source = m_image;
  unsigned char *dest = newImage + rowBytes * (m_header.wHeight - 1);

  unsigned int row;
  for (row = 0; row < m_header.wHeight; ++row) {
    memcpy(dest, source, rowBytes);
    source += rowBytes;
    dest -= rowBytes;
  }

  FREE(m_image);
  m_image = newImage;
  m_header.Desc.bTopBottomOrder = set != 0;
  return 1;
}

unsigned char *CTgaFile::Image() {
  if (!m_image) {
    SErrSetLastError(0xF720007F);
    return 0;
  }

  return m_image;
}

TGA32Pixel *CTgaFile::ImageTGA32Pixel() {
  if (!m_image) {
    SErrSetLastError(0xF720007F);
    return 0;
  }

  if (m_header.bPixelDepth != 32) {
    SErrSetLastError(0xF720007D);
    return 0;
  }

  return reinterpret_cast<TGA32Pixel *>(m_image);
}

int CTgaFile::RemoveAlphaChannels() {
  if (!Image()) {
    return 0;
  }

  if (!m_header.Desc.bAlphaChannelBits) {
    if (m_header.bPixelDepth == 24) {
      return 1;
    }

    SErrSetLastError(0x8720012D);
    m_header.Desc.bAlphaChannelBits = m_header.bPixelDepth - 8;
  }

  if (m_header.Desc.bAlphaChannelBits != 8) {
    SErrSetLastError(0xF7200082);
    return 0;
  }

  unsigned int pixelCount = m_header.wWidth * m_header.wHeight;
  m_header.bPixelDepth -= 8;
  m_header.Desc.bAlphaChannelBits = 0;

  unsigned int   pixelBytes = (m_header.bPixelDepth + 7) / 8;
  unsigned char *dst = m_image;
  unsigned char *src = m_image;
  unsigned int   pixel;
  for (pixel = 0; pixel < pixelCount; ++pixel) {
    memmove(dst, src, pixelBytes);
    dst += pixelBytes;
    src += pixelBytes + 1;
  }

  return 1;
}

void CTgaFile::RemoveHeaderTrailer() {
  FREEIFUSED(m_addlHeaderData);

  m_addlHeaderData = 0;
  m_header.bIDLength = 0;
}

int CTgaFile::SetImage(
    const void   *pImg,
    unsigned int  width,
    unsigned int  height,
    unsigned char bPixelDepth,
    unsigned char bAlphaBits,
    int           bTopDown,
    int           bRightToLeft
) {
  FATALASSERT(pImg);

  FATALASSERT((bPixelDepth == 32) || (bPixelDepth == 24));

  FATALASSERT((bAlphaBits == 0) || (bAlphaBits == 8));

  memset(&m_header, 0, sizeof(m_header));
  m_header.bPixelDepth = bPixelDepth;
  m_header.Desc.bAlphaChannelBits = bAlphaBits;
  m_header.Desc.bLeftRightOrder = bRightToLeft != 0;
  m_header.Desc.bTopBottomOrder = bTopDown != 0;
  m_header.bImageType = 2;
  m_header.wWidth = static_cast<unsigned short>(width);
  m_header.wHeight = static_cast<unsigned short>(height);
  m_imageBytes = m_header.wWidth * m_header.wHeight * ((m_header.bPixelDepth + 7) / 8);

  FREEIFUSED(m_image);

  m_image = static_cast<unsigned char *>(ALLOC(m_imageBytes));
  if (!m_image) {
    return 0;
  }

  memcpy(m_image, pImg, m_imageBytes);
  m_footer.dwExtensionOffset = 0;
  m_footer.dwDeveloperOffset = 0;
  SStrCopy(m_footer.szSigniture, "TRUEVISION-XFILE.", sizeof(m_footer.szSigniture));
  return 1;
}

int CTgaFile::Write(const char *path) {
  HOSFILE       fileHandle;
  int           success;
  unsigned long byteswritten;

  FATALASSERT(path);

  if (!m_image) {
    SErrSetLastError(0xF7200081);
    return 0;
  }

  fileHandle = OsCreateFile(path, 0x40000000, 0, 2, 0x80, 0x3F3F3F3F);
  if (fileHandle == reinterpret_cast<HOSFILE>(-1)) {
    return 0;
  }

  success = 0;
  if (!OsWriteFile(fileHandle, &m_header, sizeof(m_header), &byteswritten) || byteswritten != sizeof(m_header)) {
    goto finallylabel;
  }

  if (m_header.bIDLength > 0) {
    ASSERT(m_addlHeaderData);

    if (!OsWriteFile(fileHandle, m_addlHeaderData, m_header.bIDLength, &byteswritten) || m_header.bIDLength != byteswritten) {
      goto finallylabel;
    }
  }

  if (m_header.bColorMapType) {
    ASSERT(m_colorMap);

    if (!OsWriteFile(fileHandle, m_colorMap, ColorMapBytes(), &byteswritten) || byteswritten != ColorMapBytes()) {
      goto finallylabel;
    }
  }

  if (!OsWriteFile(fileHandle, m_image, m_imageBytes, &byteswritten) || byteswritten != m_imageBytes) {
    goto finallylabel;
  }

  if (!OsWriteFile(fileHandle, &m_footer, sizeof(m_footer), &byteswritten) || byteswritten != sizeof(m_footer)) {
    goto finallylabel;
  }

  success = 1;

finallylabel:
  OsCloseFile(fileHandle);
  if (!success) {
    OsDeleteFile(path);
  }

  return success;
}
