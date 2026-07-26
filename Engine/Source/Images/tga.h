#pragma once

#include <storm.h>

#pragma pack(push, 1)
struct TGAHeader {
  unsigned char  bIDLength;
  unsigned char  bColorMapType;
  unsigned char  bImageType;
  unsigned short wColorMapStartIndex;
  unsigned short wColorMapEntries;
  unsigned char  bColorMapEntrySize;
  unsigned short wXOrigin;
  unsigned short wYOrigin;
  unsigned short wWidth;
  unsigned short wHeight;
  unsigned char  bPixelDepth;
  union {
    unsigned char bImageDescriptor;
    struct {
      unsigned char bAlphaChannelBits : 4;
      unsigned char bLeftRightOrder : 1;
      unsigned char bTopBottomOrder : 1;
      unsigned char bReserved : 2;
    } Desc;
  };
};

struct TGAFooter {
  unsigned long dwExtensionOffset;
  unsigned long dwDeveloperOffset;
  char          szSigniture[0x12];
};
#pragma pack(pop)

struct TGA32Pixel {
  unsigned char b;
  unsigned char g;
  unsigned char r;
  unsigned char a;
};

class CTgaFile {
 public:
  CTgaFile() : m_file(0), m_image(0), m_addlHeaderData(0), m_imageBytes(0), m_colorMap(0) {
  }

  ~CTgaFile() {
    Close();
  }

  void           Close();
  int            Open(const char *filename);
  int            LoadImageData(unsigned int flags);
  int            AddAlphaChannel(const void *pImg);
  int            SetTopDown(int set);
  unsigned char *Image();
  const unsigned char *Image() const;
  TGA32Pixel    *ImageTGA32Pixel();
  const TGA32Pixel *ImageTGA32Pixel() const;
  int            RemoveAlphaChannels();
  void           RemoveHeaderTrailer();
  int            SetImage(const CTgaFile &source);
  int            SetImage(
      const void   *pImg,
      unsigned int  width,
      unsigned int  height,
      unsigned char bPixelDepth,
      unsigned char bAlphaBits,
      int           bTopDown,
      int           bRightToLeft
  );
  int Compress();
  int Write(const char *path);

  unsigned int Width() const {
    return m_header.wWidth;
  }

  unsigned int Height() const {
    return m_header.wHeight;
  }

  unsigned int Size() const {
    return Width() * Height();
  }

  unsigned int BytesPerPixel() const {
    return (m_header.bPixelDepth + 7) / 8;
  }

  unsigned int Bytes() const {
    return Size() * BytesPerPixel();
  }

  unsigned char AlphaBits() const {
    return m_header.Desc.bAlphaChannelBits;
  }

  unsigned char PixelDepth() const {
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

  unsigned int ColorMapEntries() const {
    return m_header.wColorMapEntries;
  }

  unsigned int ColorMapEntryBytes() {
    int componentBits = m_header.bColorMapEntrySize / 3;

    if (componentBits >= 8) {
      componentBits = 8;
    }

    return componentBits * 3 / 8;
  }

  unsigned int ColorMapBytes() {
    return m_header.wColorMapEntries * ColorMapEntryBytes();
  }

  unsigned char *ColorMap() {
    return m_colorMap;
  }

  const unsigned char *ColorMap() const {
    return m_colorMap;
  }

  int IsCompressed() const {
    return m_header.bImageType >= 9 && m_header.bImageType <= 11;
  }

 private:
  int           ValidateColorDepth();
  void          ConvertColorMapped(unsigned int flags);
  int           ReadColorMappedImage(unsigned int flags);
  unsigned long PreImageBytes();
  void          AddAlphaChannel(unsigned char *pAlphaData, unsigned char *pNoAlphaData, const unsigned char *alpha);
  int           ReadRawImage(unsigned int flags);
  int           ReadRleImage(unsigned int flags);
  int           RLEDecompressImage(unsigned char *pRLEData, unsigned char *pData);
  int           CountRun(unsigned char *pImage, int nMax);
  int           RleCompressLine(unsigned char **uncompressed, unsigned char **compressed);

 public:
  SFile         *m_file;
  unsigned char *m_image;
  TGAHeader      m_header;
  unsigned char *m_addlHeaderData;
  TGAFooter      m_footer;
  unsigned int   m_imageBytes;
  unsigned char *m_colorMap;
};
