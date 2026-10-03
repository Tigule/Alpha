#pragma once

#include <storm.h>

enum {
  TGA_NO_IMAGE_DATA = 0,
  TGA_COLOR_MAPPED = 1,
  TGA_TRUE_COLOR = 2,
  TGA_BLACK_N_WHITE = 3,
  TGA_RLE_COLOR_MAPPED = 9,
  TGA_RLE_TRUE_COLOR = 10,
  TGA_RLE_BLACK_N_WHITE = 11
};

#pragma pack(push, 1)
struct TGAHeader {
  BYTE bIDLength;
  BYTE bColorMapType;
  BYTE bImageType;
  WORD wColorMapStartIndex;
  WORD wColorMapEntries;
  BYTE bColorMapEntrySize;
  WORD wXOrigin;
  WORD wYOrigin;
  WORD wWidth;
  WORD wHeight;
  BYTE bPixelDepth;
  union {
    BYTE bImageDescriptor;
    struct {
      BYTE bAlphaChannelBits : 4;
      BYTE bLeftRightOrder : 1;
      BYTE bTopBottomOrder : 1;
      BYTE bReserved : 2;
    } Desc;
  };
};

struct TGAFooter {
  DWORD dwExtensionOffset;
  DWORD dwDeveloperOffset;
  char  szSigniture[0x12];
};
#pragma pack(pop)

struct TGA32Pixel {
  TGA32Pixel();
  TGA32Pixel(BYTE b, BYTE g, BYTE r, BYTE a);
  TGA32Pixel(UINT color);
  operator UINT();

  BYTE b;
  BYTE g;
  BYTE r;
  BYTE a;
};

class CTgaFile {
 public:
  CTgaFile() : m_file(0), m_image(0), m_addlHeaderData(0), m_imageBytes(0), m_colorMap(0) {
  }

  CTgaFile(const CTgaFile &source);

  CTgaFile &operator=(const CTgaFile &source);

  ~CTgaFile() {
    Close();
  }

  void Close();

  UINT Width() const {
    return m_header.wWidth;
  }

  UINT Height() const {
    return m_header.wHeight;
  }

  UINT Size() const {
    return Width() * Height();
  }

  UINT BytesPerPixel() const {
    return (m_header.bPixelDepth + 7) / 8;
  }

  UINT Bytes() const {
    return Size() * BytesPerPixel();
  }

  BYTE AlphaBits() const {
    return m_header.Desc.bAlphaChannelBits;
  }

  BYTE PixelDepth() const {
    return m_header.bPixelDepth;
  }

  BOOL IsRightToLeft() const {
    return m_header.Desc.bLeftRightOrder;
  }

  BOOL IsTopDown() const {
    return m_header.Desc.bTopBottomOrder;
  }

  BOOL IsColorMapped() const {
    return m_header.bColorMapType != 0;
  }

  UINT ColorMapEntries() const {
    return m_header.wColorMapEntries;
  }

  UINT ColorMapEntryBytes() const {
    int componentBits = m_header.bColorMapEntrySize / 3;

    if (componentBits >= 8) {
      componentBits = 8;
    }

    return componentBits * 3 / 8;
  }

  UINT ColorMapBytes() const {
    return m_header.wColorMapEntries * ColorMapEntryBytes();
  }
  const BYTE *Image() const;
  BYTE       *Image();

  const BYTE *ColorMap() const {
    return m_colorMap;
  }

  BYTE *ColorMap() {
    return m_colorMap;
  }
  const TGA32Pixel *ImageTGA32Pixel() const;
  TGA32Pixel       *ImageTGA32Pixel();

  BOOL IsCompressed() const {
    return m_header.bImageType >= 9 && m_header.bImageType <= 11;
  }
  BOOL Open(LPCSTR filename);
  int  LoadImageData(UINT flags);
  BOOL SetImage(LPCVOID pImg, UINT width, UINT height, BYTE bPixelDepth, BYTE bAlphaBits, BOOL bTopDown, BOOL bRightToLeft);
  BOOL SetImage(const CTgaFile &source);
  BOOL SetTopDown(int set);
  BOOL AddAlphaChannel(LPCVOID pImg);

 private:
  void AddAlphaChannel(BYTE *pAlphaData, BYTE *pNoAlphaData, const BYTE *alpha);

 public:
  BOOL RemoveAlphaChannels();
  void RemoveHeaderTrailer();
  BOOL Compress();
  BOOL Write(LPCSTR path);

 private:
  DWORD PreImageBytes();
  BOOL  ReadRawImage(UINT flags);
  BOOL  RLEDecompressImage(BYTE *pRLEData, BYTE *pData);
  BOOL  ReadRleImage(UINT flags);
  int   ReadColorMappedImage(UINT flags);
  BOOL  ValidateColorDepth();
  void  ConvertColorMapped(UINT flags);
  BOOL  CountRun(BYTE *pImage, int nMax);
  BOOL  RleCompressLine(BYTE **uncompressed, BYTE **compressed);

  SFile    *m_file;
  BYTE     *m_image;
  TGAHeader m_header;
  BYTE     *m_addlHeaderData;
  TGAFooter m_footer;
  UINT      m_imageBytes;
  BYTE     *m_colorMap;
};
