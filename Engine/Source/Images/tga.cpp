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

BOOL CTgaFile::Open(LPCSTR filename) {
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
    m_addlHeaderData = static_cast<BYTE *>(ALLOC(m_header.bIDLength));
    if (!SFile::Read(m_file, m_addlHeaderData, m_header.bIDLength, 0, 0, 0)) {
      return 0;
    }
  } else {
    m_addlHeaderData = 0;
  }

  if (m_header.bColorMapType) {
    m_colorMap = static_cast<BYTE *>(ALLOC(ColorMapBytes()));
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

BOOL CTgaFile::ValidateColorDepth() {
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

void CTgaFile::ConvertColorMapped(UINT flags) {
  UINT  newPixelDepth = m_header.Desc.bAlphaChannelBits + 24;
  UINT  addAlpha = flags & 1;
  UINT  pixelCount = m_header.wWidth * m_header.wHeight;
  BYTE *oldImage = m_image;

  m_image = static_cast<BYTE *>(ALLOC(pixelCount * (addAlpha + (newPixelDepth >> 3))));

  BYTE *dstPix = m_image + addAlpha * pixelCount;
  BYTE *srcPix = oldImage;
  UINT  pixel;
  for (pixel = 0; pixel < pixelCount; ++pixel) {
    memcpy(dstPix, &m_colorMap[ColorMapEntryBytes() * (*srcPix - m_header.wColorMapStartIndex)], ColorMapEntryBytes());
    dstPix += ColorMapEntryBytes();
    ++srcPix;
  }

  FREE(oldImage);
  FREE(m_colorMap);

  m_header.bPixelDepth = static_cast<BYTE>(newPixelDepth);
  m_colorMap = 0;
  m_header.wColorMapEntries = 0;
  m_header.bColorMapType = 0;
  m_header.bImageType = 2;
  m_imageBytes = m_header.wWidth * m_header.wHeight * ((m_header.bPixelDepth + 7) / 8);

  if (addAlpha) {
    AddAlphaChannel(m_image, m_image + pixelCount, 0);
  }
}

int CTgaFile::ReadColorMappedImage(UINT flags) {
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

int CTgaFile::LoadImageData(UINT flags) {
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

DWORD CTgaFile::PreImageBytes() {
  return sizeof(m_header) + m_header.bIDLength + ColorMapBytes();
}

void CTgaFile::AddAlphaChannel(BYTE *pAlphaData, BYTE *pNoAlphaData, const BYTE *alpha) {
  UINT pixelCount = m_header.wWidth * m_header.wHeight;
  UINT pixelBytes = (m_header.bPixelDepth + 7) / 8;
  UINT pixel;

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

  m_header.bPixelDepth = static_cast<BYTE>(m_header.bPixelDepth + 8 - m_header.Desc.bAlphaChannelBits);
  m_header.Desc.bAlphaChannelBits = 8;
  m_imageBytes = m_header.wWidth * m_header.wHeight * ((m_header.bPixelDepth + 7) / 8);
}

BOOL CTgaFile::ReadRawImage(UINT flags) {
  int addAlpha = (flags & 1) && m_header.Desc.bAlphaChannelBits == 0;

  if (SFile::SetFilePointer(m_file, PreImageBytes(), 0, FILE_BEGIN) == static_cast<DWORD>(-1)) {
    return 0;
  }

  UINT sourceBytes = m_header.wWidth * m_header.wHeight * ((m_header.bPixelDepth + 7) / 8);
  ASSERT(m_image == 0);

  m_image = static_cast<BYTE *>(ALLOC(sourceBytes + addAlpha * m_header.wWidth * m_header.wHeight));
  if (!m_image) {
    return 0;
  }

  if (!SFile::Read(m_file, m_image + addAlpha * m_header.wWidth * m_header.wHeight, sourceBytes, 0, 0, 0)) {
    return 0;
  }

  if (addAlpha) {
    AddAlphaChannel(m_image, m_image + m_header.wWidth * m_header.wHeight, 0);
  }

  return 1;
}

BOOL CTgaFile::ReadRleImage(UINT flags) {
  int  addAlpha = (flags & 1) && m_header.Desc.bAlphaChannelBits == 0;
  UINT imageBytes = m_header.wWidth * m_header.wHeight * ((m_header.bPixelDepth + 7) / 8);
  ASSERT(m_image == 0);

  m_image = static_cast<BYTE *>(ALLOC(imageBytes + addAlpha * m_header.wWidth * m_header.wHeight));
  if (!m_image) {
    return 0;
  }

  imageBytes = SFile::GetFileSize(m_file, 0) - PreImageBytes();
  if (imageBytes == static_cast<DWORD>(-1)) {
    return 0;
  }

  BYTE *rleData = static_cast<BYTE *>(ALLOC(imageBytes));
  if (!rleData) {
    return 0;
  }

  if (SFile::SetFilePointer(m_file, PreImageBytes(), 0, FILE_BEGIN) == static_cast<DWORD>(-1)) {
    return 0;
  }

  if (!SFile::Read(m_file, rleData, imageBytes, 0, 0, 0)) {
    return 0;
  }

  int result = RLEDecompressImage(rleData, m_image + addAlpha * m_header.wWidth * m_header.wHeight);
  FREE(rleData);
  if (!result) {
    return 0;
  }

  if (addAlpha) {
    AddAlphaChannel(m_image, m_image + m_header.wWidth * m_header.wHeight, 0);
  }

  return 1;
}

BOOL CTgaFile::RLEDecompressImage(BYTE *pRLEData, BYTE *pData) {
  ASSERT(pRLEData);

  int  pixelsRemaining = m_header.wWidth * m_header.wHeight;
  UINT pixelBytes = (m_header.bPixelDepth + 7) / 8;

  while (pixelsRemaining) {
    UINT packetHeader = *pRLEData++;
    int  packetPixels;

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

BOOL CTgaFile::AddAlphaChannel(LPCVOID pImg) {
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

  UINT  pixelCount = m_header.wWidth * m_header.wHeight;
  BYTE *newImage = static_cast<BYTE *>(ALLOC(pixelCount * (((m_header.bPixelDepth + 7) / 8) + 1)));
  if (!newImage) {
    return 0;
  }

  AddAlphaChannel(newImage, m_image, static_cast<const BYTE *>(pImg));
  FREE(m_image);
  m_image = newImage;
  return 1;
}

BOOL CTgaFile::SetTopDown(int set) {
  if ((set && m_header.Desc.bTopBottomOrder) || (!set && !m_header.Desc.bTopBottomOrder)) {
    return 1;
  }

  if (m_header.bImageType >= 9 && m_header.bImageType <= 11) {
    SErrSetLastError(0xF7200083);
    return 0;
  }

  UINT  rowBytes = m_header.wWidth * ((m_header.bPixelDepth + 7) / 8);
  BYTE *newImage = static_cast<BYTE *>(ALLOC(m_header.wHeight * rowBytes));
  BYTE *source = m_image;
  BYTE *dest = newImage + rowBytes * (m_header.wHeight - 1);

  UINT row;
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

BYTE *CTgaFile::Image() {
  if (!m_image) {
    SErrSetLastError(0xF720007F);
    return 0;
  }

  return m_image;
}

const BYTE *CTgaFile::Image() const {
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

const TGA32Pixel *CTgaFile::ImageTGA32Pixel() const {
  if (!m_image) {
    SErrSetLastError(0xF720007F);
    return 0;
  }

  if (m_header.bPixelDepth != 32) {
    SErrSetLastError(0xF720007D);
    return 0;
  }

  return reinterpret_cast<const TGA32Pixel *>(m_image);
}

BOOL CTgaFile::RemoveAlphaChannels() {
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

  UINT pixelCount = m_header.wWidth * m_header.wHeight;
  m_header.bPixelDepth -= 8;
  m_header.Desc.bAlphaChannelBits = 0;

  UINT  pixelBytes = (m_header.bPixelDepth + 7) / 8;
  BYTE *dst = m_image;
  BYTE *src = m_image;
  UINT  pixel;
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

BOOL CTgaFile::SetImage(const CTgaFile &source) {
  if (!source.Image()) {
    return 0;
  }

  return SetImage(
      source.Image(), source.m_header.wWidth, source.m_header.wHeight, source.m_header.bPixelDepth, source.m_header.Desc.bAlphaChannelBits,
      source.m_header.Desc.bTopBottomOrder, source.m_header.Desc.bLeftRightOrder
  );
}

BOOL CTgaFile::SetImage(LPCVOID pImg, UINT width, UINT height, BYTE bPixelDepth, BYTE bAlphaBits, BOOL bTopDown, BOOL bRightToLeft) {
  FATALASSERT(pImg);

  FATALASSERT((bPixelDepth == 32) || (bPixelDepth == 24));

  FATALASSERT((bAlphaBits == 0) || (bAlphaBits == 8));

  memset(&m_header, 0, sizeof(m_header));
  m_header.bPixelDepth = bPixelDepth;
  m_header.Desc.bAlphaChannelBits = bAlphaBits;
  m_header.Desc.bLeftRightOrder = bRightToLeft != 0;
  m_header.Desc.bTopBottomOrder = bTopDown != 0;
  m_header.bImageType = 2;
  m_header.wWidth = static_cast<WORD>(width);
  m_header.wHeight = static_cast<WORD>(height);
  m_imageBytes = m_header.wWidth * m_header.wHeight * ((m_header.bPixelDepth + 7) / 8);

  FREEIFUSED(m_image);

  m_image = static_cast<BYTE *>(ALLOC(m_imageBytes));
  if (!m_image) {
    return 0;
  }

  memcpy(m_image, pImg, m_imageBytes);
  m_footer.dwExtensionOffset = 0;
  m_footer.dwDeveloperOffset = 0;
  SStrCopy(m_footer.szSigniture, "TRUEVISION-XFILE.", sizeof(m_footer.szSigniture));
  return 1;
}

BOOL CTgaFile::CountRun(BYTE *pImage, int nMax) {
  ASSERT(pImage != 0);

  UINT pixelBytes = (m_header.bPixelDepth + 7) / 8;
  int  nCount = 0;
  UINT dwCheck = 0;
  memcpy(&dwCheck, pImage, pixelBytes);

  if (nMax > 128) {
    nMax = 128;
  }

  while (nMax) {
    --nMax;
    if (memcmp(pImage, &dwCheck, pixelBytes)) {
      break;
    }

    pImage += pixelBytes;
    ++nCount;
  }

  return nCount;
}

BOOL CTgaFile::RleCompressLine(BYTE **uncompressed, BYTE **compressed) {
  ASSERT(uncompressed != 0);
  ASSERT(*uncompressed != 0);
  ASSERT(compressed != 0);
  ASSERT(*compressed != 0);

  BYTE *pRawImage = *uncompressed;
  BYTE *pRLEData = *compressed;
  BYTE *pbCopyLen = 0;
  int   nLineWid = m_header.wWidth;

  while (nLineWid) {
    int nRunLen = CountRun(pRawImage, nLineWid);
    if (nRunLen >= 2) {
      m_imageBytes += (m_header.bPixelDepth + 7) / 8 + 1;
      pbCopyLen = 0;
      if (m_imageBytes >= m_header.wWidth * m_header.wHeight * ((m_header.bPixelDepth + 7) / 8)) {
        return 0;
      }

      *pRLEData = static_cast<BYTE>((nRunLen - 1) | 0x80);
      memcpy(pRLEData + 1, pRawImage, (m_header.bPixelDepth + 7) / 8);
      pRLEData += (m_header.bPixelDepth + 7) / 8 + 1;
      pRawImage += nRunLen * ((m_header.bPixelDepth + 7) / 8);
      nLineWid -= nRunLen;
      continue;
    }

    if (!pbCopyLen || *pbCopyLen == 127) {
      *pRLEData = 0;
      pbCopyLen = pRLEData++;
      ++m_imageBytes;
    } else {
      ++*pbCopyLen;
    }

    m_imageBytes += (m_header.bPixelDepth + 7) / 8;
    if (m_imageBytes >= m_header.wWidth * m_header.wHeight * ((m_header.bPixelDepth + 7) / 8)) {
      return 0;
    }

    memcpy(pRLEData, pRawImage, (m_header.bPixelDepth + 7) / 8);
    pRLEData += (m_header.bPixelDepth + 7) / 8;
    pRawImage += (m_header.bPixelDepth + 7) / 8;
    --nLineWid;
  }

  *uncompressed = pRawImage;
  *compressed = pRLEData;
  return 1;
}

BOOL CTgaFile::Compress() {
  if (!m_image) {
    SErrSetLastError(0xF7200081);
    return 0;
  }

  if (m_header.bImageType >= 9) {
    SErrSetLastError(0xF7200083);
    return 0;
  }

  BYTE *rawImage = m_image;
  BYTE *compressedImage = static_cast<BYTE *>(ALLOC(m_imageBytes));
  if (!compressedImage) {
    return 0;
  }

  BYTE *pRawImage = rawImage;
  BYTE *pRLEData = compressedImage;
  int   rows = m_header.wHeight;
  m_imageBytes = 0;

  while (rows--) {
    if (!RleCompressLine(&pRawImage, &pRLEData)) {
      FREE(compressedImage);
      m_imageBytes = m_header.wWidth * m_header.wHeight * ((m_header.bPixelDepth + 7) / 8);
      return 1;
    }
  }

  m_header.bImageType += 8;
  FREE(m_image);
  m_image = compressedImage;
  return 1;
}

BOOL CTgaFile::Write(LPCSTR path) {
  HOSFILE fileHandle;
  int     success;
  DWORD   byteswritten;

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
