#include "../CGxDeviceOpenGl.h"

#include <gl/gl.h>

int SetupPixelFormat(HDC hdc, const CGxFormat &format) {
  BYTE gBits = 6;
  BYTE bBits = 5;
  BYTE rBits = 5;
  BYTE stencilBits = 0;
  BYTE colorBits = 16;
  BYTE depthBits = 16;
  BYTE alphaBits = 0;

  switch (format.colorFormat) {
    case CGxFormat::Fmt_Rgb565:
      colorBits = 16;
      rBits = 5;
      gBits = 6;
      bBits = 5;
      alphaBits = 0;
      break;
    case CGxFormat::Fmt_ArgbX888:
      colorBits = 24;
      rBits = gBits = bBits = 8;
      break;
    case CGxFormat::Fmt_Argb8888:
      colorBits = 24;
      rBits = gBits = bBits = 8;
      alphaBits = 8;
      break;
    case CGxFormat::Fmt_Argb2101010:
      colorBits = 30;
      rBits = gBits = bBits = 10;
      alphaBits = 2;
      break;
  }

  switch (format.depthFormat) {
    case CGxFormat::Fmt_Ds160:
      depthBits = 16;
      stencilBits = 0;
      break;
    case CGxFormat::Fmt_Ds24X:
      depthBits = 24;
      stencilBits = 0;
      break;
    case CGxFormat::Fmt_Ds248:
      depthBits = 24;
      stencilBits = 8;
      break;
    case CGxFormat::Fmt_Ds320:
      depthBits = 32;
      stencilBits = 0;
      break;
  }

  PIXELFORMATDESCRIPTOR pfd = {
      sizeof(pfd),
      1,
      PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
      PFD_TYPE_RGBA,
      colorBits,
      0,
      0,
      0,
      0,
      0,
      0,
      alphaBits,
      0,
      0,
      0,
      0,
      0,
      0,
      depthBits,
      stencilBits,
      0,
      PFD_MAIN_PLANE,
      0,
      0,
      0,
      0
  };

  int pixelFormat = ChoosePixelFormat(hdc, &pfd);
  if (!pixelFormat) {
    CGxDevice::Log("ChoosePixelFormat failed, color = %d, alpha = %d, depth = %d\n", colorBits, alphaBits, depthBits);
    return 0;
  }

  DescribePixelFormat(hdc, pixelFormat, sizeof(pfd), &pfd);
  int matches = 1;
  if (!format.window) {
    if (format.colorFormat != CGxFormat::Fmt_ArgbX888) {
      matches = pfd.cAlphaBits == alphaBits;
    }
    matches &= pfd.cRedBits == rBits && pfd.cBlueBits == bBits && pfd.cGreenBits == gBits;
  }
  if (format.depthFormat != CGxFormat::Fmt_Ds24X) {
    matches &= pfd.cStencilBits == stencilBits;
  }
  matches &= pfd.cDepthBits == depthBits;

  if (!matches) {
    CGxDevice::Log("SetupPixelFormat(): pfd does not match requested");
    return 0;
  }

  if (!SetPixelFormat(hdc, pixelFormat, &pfd)) {
    CGxDevice::Log("SetPixelFormat failed, color = %d, alpha = %d, depth = %d, fmt = %d\n", colorBits, alphaBits, depthBits, pixelFormat);
    return 0;
  }

  return 1;
}

HGLRC AttachGlContext(HWND hwnd, HDC hdc, const CGxFormat &format) {
  (void)hdc;
  HDC windowDC = GetDC(hwnd);
  if (windowDC && SetupPixelFormat(windowDC, format)) {
    return wglCreateContext(windowDC);
  }
  return 0;
}

void RemoveGlContext(HGLRC hglrc) {
  wglMakeCurrent(0, 0);
  wglDeleteContext(hglrc);
}
