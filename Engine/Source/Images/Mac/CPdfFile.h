#ifndef ENGINE_SOURCE_IMAGES_MAC_CPDFFILE_H
#define ENGINE_SOURCE_IMAGES_MAC_CPDFFILE_H

#include <storm.h>

#include <ApplicationServices/ApplicationServices.h>

class CPdfFile {
 public:
  CPdfFile();
  ~CPdfFile();

  BOOL SetImage(LPCVOID pImg, UINT width, UINT height, BYTE bPixelDepth, BYTE bAlphaBits);
  int  Write(LPCSTR filename, float pageWidth, float pageHeight);

 private:
  CGImageRef m_imageRef;
  UINT       m_width;
  UINT       m_height;
};

#endif
