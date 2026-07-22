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
  TGA32Pixel    *ImageTGA32Pixel();
  int            RemoveAlphaChannels();
  void           RemoveHeaderTrailer();
  int            SetImage(
      const void   *pImg,
      unsigned int  width,
      unsigned int  height,
      unsigned char bPixelDepth,
      unsigned char bAlphaBits,
      int           bTopDown,
      int           bRightToLeft
  );
  int Write(const char *path);

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

 private:
  int           ValidateColorDepth();
  void          ConvertColorMapped(unsigned int flags);
  int           ReadColorMappedImage(unsigned int flags);
  unsigned long PreImageBytes();
  void          AddAlphaChannel(unsigned char *pAlphaData, unsigned char *pNoAlphaData, const unsigned char *alpha);
  int           ReadRawImage(unsigned int flags);
  int           ReadRleImage(unsigned int flags);
  int           RLEDecompressImage(unsigned char *pRLEData, unsigned char *pData);

 public:
  SFile         *m_file;
  unsigned char *m_image;
  TGAHeader      m_header;
  unsigned char *m_addlHeaderData;
  TGAFooter      m_footer;
  unsigned int   m_imageBytes;
  unsigned char *m_colorMap;
};
