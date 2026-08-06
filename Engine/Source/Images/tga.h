#pragma once

#include <storm.h>

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
  TGA32Pixel(UINT color);
  TGA32Pixel(BYTE b, BYTE g, BYTE r, BYTE a);
  operator UINT();

  BYTE b;
  BYTE g;
  BYTE r;
  BYTE a;
};

class CTgaFile {
 public:
  CTgaFile(const CTgaFile &source);

  CTgaFile() : m_file(0), m_image(0), m_addlHeaderData(0), m_imageBytes(0), m_colorMap(0) {
  }

  CTgaFile &operator=(const CTgaFile &source);

  ~CTgaFile() {
    Close();
  }

  void              Close();
  int               Open(LPCSTR filename);
  int               LoadImageData(UINT flags);
  int               AddAlphaChannel(LPCVOID pImg);
  int               SetTopDown(int set);
  BYTE             *Image();
  const BYTE       *Image() const;
  TGA32Pixel       *ImageTGA32Pixel();
  const TGA32Pixel *ImageTGA32Pixel() const;
  int               RemoveAlphaChannels();
  void              RemoveHeaderTrailer();
  int               SetImage(const CTgaFile &source);
  int               SetImage(LPCVOID pImg, UINT width, UINT height, BYTE bPixelDepth, BYTE bAlphaBits, int bTopDown, int bRightToLeft);
  int               Compress();
  int               Write(LPCSTR path);

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

  int IsRightToLeft() const {
    return m_header.Desc.bLeftRightOrder;
  }

  int IsTopDown() const {
    return m_header.Desc.bTopBottomOrder;
  }

  int IsColorMapped() const {
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

  BYTE *ColorMap() {
    return m_colorMap;
  }

  const BYTE *ColorMap() const {
    return m_colorMap;
  }

  int IsCompressed() const {
    return m_header.bImageType >= 9 && m_header.bImageType <= 11;
  }

 private:
  int   ValidateColorDepth();
  void  ConvertColorMapped(UINT flags);
  int   ReadColorMappedImage(UINT flags);
  DWORD PreImageBytes();
  void  AddAlphaChannel(BYTE *pAlphaData, BYTE *pNoAlphaData, const BYTE *alpha);
  int   ReadRawImage(UINT flags);
  int   ReadRleImage(UINT flags);
  int   RLEDecompressImage(BYTE *pRLEData, BYTE *pData);
  int   CountRun(BYTE *pImage, int nMax);
  int   RleCompressLine(BYTE **uncompressed, BYTE **compressed);

 private:
  SFile    *m_file;
  BYTE     *m_image;
  TGAHeader m_header;
  BYTE     *m_addlHeaderData;
  TGAFooter m_footer;
  UINT      m_imageBytes;
  BYTE     *m_colorMap;
};
