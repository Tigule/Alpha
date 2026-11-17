#include "CPdfFile.h"

CPdfFile::CPdfFile() : m_imageRef(NULL), m_width(0), m_height(0) {
}

CPdfFile::~CPdfFile() {
  if (m_imageRef) {
    CFRelease(m_imageRef);
  }
}

BOOL CPdfFile::SetImage(LPCVOID pImg, UINT width, UINT height, BYTE bPixelDepth, BYTE bAlphaBits) {
  CGColorSpaceRef   colorSpace;
  CGDataProviderRef provider = NULL;
  CGImageRef        image = NULL;
  BOOL              result = FALSE;
  UINT              bytesPerPixel;

  VALIDATEBEGIN;
  VALIDATE(pImg);
  VALIDATE((bPixelDepth == 32) || (bPixelDepth == 24));
  VALIDATE((bAlphaBits == 0) || (bAlphaBits == 8));
  VALIDATEEND;

  colorSpace = CGColorSpaceCreateDeviceRGB();
  if (colorSpace) {
    bytesPerPixel = bPixelDepth >> 3;
    provider = CGDataProviderCreateWithData(0, pImg, bytesPerPixel * width * height, 0);

    if (provider) {
      image = CGImageCreate(
          width, height, 8, bPixelDepth, width * bytesPerPixel, colorSpace,
          (bAlphaBits ? kCGImageAlphaFirst : kCGImageAlphaNoneSkipFirst) | kCGBitmapByteOrder32Host, provider, 0, false,
          kCGRenderingIntentDefault
      );

      if (image) {
        if (m_imageRef) {
          CFRelease(m_imageRef);
        }

        m_imageRef = image;
        CFRetain(image);
        m_width = width;
        m_height = height;
        result = TRUE;
      }
    }
  }

  if (colorSpace) {
    CFRelease(colorSpace);
  }

  if (provider) {
    CFRelease(provider);
  }

  if (image) {
    CFRelease(image);
  }

  return result;
}

int CPdfFile::Write(LPCSTR filename, float pageWidth, float pageHeight) {
  CFStringRef  path;
  CFURLRef     url = NULL;
  CGContextRef context = NULL;
  CGRect       rect;
  int          result = 1;

  ASSERT(m_imageRef != 0);
  if (!m_imageRef) {
    return 0;
  }

  path = CFStringCreateWithCString(0, filename, kCFStringEncodingUTF8);
  if (path) {
    url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, path, kCFURLPOSIXPathStyle, false);

    if (url) {
      rect = CGRectMake(0.0, 0.0, pageWidth, pageHeight);
      context = CGPDFContextCreateWithURL(url, &rect, 0);

      if (context) {
        CGContextBeginPage(context, &rect);
        CGContextDrawImage(context, rect, m_imageRef);
        CGContextEndPage(context);
        result = 0;
      }
    }
  }

  if (path) {
    CFRelease(path);
  }

  if (url) {
    CFRelease(url);
  }

  if (context) {
    CFRelease(context);
  }

  return result;
}
