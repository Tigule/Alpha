#include "../CGxDeviceOpenGl.h"
#include "../GlExtSupport.h"

#include <storm.h>

#include <gl/gl.h>
#include <string.h>

typedef BOOL(WINAPI *EnumDisplayDevicesAProc)(LPCSTR device, DWORD deviceNum, DISPLAY_DEVICEA *displayDevice, DWORD flags);

static BOOL EnumDisplayDevicesTarget(LPCSTR device, DWORD deviceNum, DISPLAY_DEVICEA *displayDevice, DWORD flags) {
  static EnumDisplayDevicesAProc enumDisplayDevices =
      reinterpret_cast<EnumDisplayDevicesAProc>(GetProcAddress(GetModuleHandleA("user32.dll"), "EnumDisplayDevicesA"));
  return enumDisplayDevices && enumDisplayDevices(device, deviceNum, displayDevice, flags);
}

static const char s_WndClassName[] = "GxWindowClassOpenGl";

static int s_inCreateOrDestroy;

HGLRC AttachGlContext(HWND hwnd, HDC hdc, const CGxFormat &format);
void RemoveGlContext(HGLRC context);

static unsigned short WindowClassCreate() {
  HINSTANCE   instance = GetModuleHandle(0);
  WNDCLASSEXA wc;
  memset(&wc, 0, sizeof(wc));
  wc.cbSize = sizeof(wc);
  wc.style = CS_OWNDC;
  wc.lpfnWndProc = CGxDeviceOpenGl::WindowProcGl;
  wc.hInstance = instance;
  wc.lpszClassName = s_WndClassName;
  wc.hIcon = static_cast<HICON>(LoadImageA(instance, "BlizzardIcon.ico", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
  wc.hCursor = LoadCursorA(instance, "BlizzardCursor.cur");
  if (!wc.hCursor) {
    wc.hCursor = LoadCursorA(instance, IDC_ARROW);
  }
  return RegisterClassExA(&wc);
}

void WindowClassDestroy(unsigned short &hwndClass) {
  UnregisterClass(reinterpret_cast<const char *>(hwndClass), GetModuleHandle(0));
  hwndClass = 0;
}

static HWND WindowCreate(CGxDeviceOpenGl *dev, const CGxFormat &format) {
  unsigned long style =
      format.window ? WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS : WS_POPUP | WS_MAXIMIZE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

  return CreateWindowExA(
      WS_EX_APPWINDOW, s_WndClassName, "A game in progress", style, format.pos.x, format.pos.y, format.size.x, format.size.y, 0, 0,
      GetModuleHandle(0), dev
  );
}

void WindowDestroy(HWND &hwnd) {
  DestroyWindow(hwnd);
  hwnd = 0;
}

void CGxDeviceOpenGl::DeviceCreatePbuffer() {
  if (wglARBPbuffer) {
    int                       pixelFormat = GetPixelFormat(m_hdc);
    PixelFormatAttribute<int> createAttribsi(0, 0);
    m_hPbuffer = wglCreatePbufferARB(m_hdc, pixelFormat, m_caps.m_maxTextureSize, m_caps.m_maxTextureSize, &createAttribsi.attribute);
    m_hPbufferDC = wglGetPbufferDCARB(m_hPbuffer);

    PIXELFORMATDESCRIPTOR pfd;
    DescribePixelFormat(m_hPbufferDC, pixelFormat, sizeof(pfd), &pfd);
    if (pfd.cRedBits == 8 && pfd.cGreenBits == 8 && pfd.cBlueBits == 8) {
      m_caps.m_rttFormat[GxTex_Argb8888] = 1;
    } else if (pfd.cRedBits == 5 && pfd.cGreenBits == 6 && pfd.cBlueBits == 5) {
      m_caps.m_rttFormat[GxTex_Rgb565] = 1;
    }

    m_hPbufferRC = wglGetCurrentContext();
  }
}

void CGxDeviceOpenGl::DeviceQueryPbuffer() {
  if (m_hPbuffer) {
    int flag;
    wglQueryPbufferARB(m_hPbuffer, WGL_PBUFFER_LOST_ARB, &flag);
    if (flag) {
      DeviceDestroyPbuffer();
      DeviceCreatePbuffer();
    }
  }
}

void CGxDeviceOpenGl::DeviceDestroyPbuffer() {
  if (m_hPbuffer) {
    wglDeleteContext(m_hPbufferRC);
    wglReleasePbufferDCARB(m_hPbuffer, m_hPbufferDC);
    wglDestroyPbufferARB(m_hPbuffer);
  }

  m_hPbuffer = 0;
  m_hPbufferDC = 0;
  m_hPbufferRC = 0;
}

void CGxDeviceOpenGl::IDevSetFocus(int focus, const CGxFormat &format) {
  if (format.window) {
    if (focus) {
      ShowWindow(m_hwnd, SW_SHOWNORMAL);
    }
  } else if (focus) {
    DISPLAY_DEVICEA dd;
    DEVMODEA        dm;
    memset(&dd, 0, sizeof(dd));
    memset(&dm, 0, sizeof(dm));
    dd.cb = sizeof(dd);
    dm.dmSize = sizeof(dm);
    EnumDisplayDevicesTarget(0, 0, &dd, 0);
    EnumDisplaySettingsA(reinterpret_cast<const char *>(dd.DeviceName), format.apiSpecificModeID, &dm);
    FATALASSERT(ChangeDisplaySettingsExA(reinterpret_cast<const char *>(dd.DeviceName), &dm, 0, CDS_FULLSCREEN, 0) == DISP_CHANGE_SUCCESSFUL);
    SetWindowPos(m_hwnd, 0, 0, 0, format.size.x, format.size.y, SWP_DEFERERASE | SWP_NOCOPYBITS | SWP_NOREDRAW);
    ShowWindow(m_hwnd, SW_SHOWMAXIMIZED);
  } else {
    ChangeDisplaySettingsExA(0, 0, 0, 0, 0);
    ShowWindow(m_hwnd, SW_MINIMIZE);
  }
}

int CGxDeviceOpenGl::SetFormatMode(const CGxFormat &format) {
  if (format.window) {
    format.apiSpecificModeID = 0;
    return 1;
  }

  DISPLAY_DEVICEA dd;
  DEVMODEA        dm;
  dd.cb = sizeof(dd);
  EnumDisplayDevicesTarget(0, 0, &dd, 0);

  unsigned int mode = 0;
  unsigned int bitsPerPixel = format.colorFormat ? 32 : 16;
  dm.dmSize = sizeof(dm);
  while (EnumDisplaySettingsA(reinterpret_cast<const char *>(dd.DeviceName), mode, &dm)) {
    if (dm.dmBitsPerPel == bitsPerPixel && dm.dmDisplayFrequency == format.refreshRate &&
        dm.dmPelsWidth == static_cast<unsigned int>(format.size.x) && dm.dmPelsHeight == static_cast<unsigned int>(format.size.y))
    {
      format.apiSpecificModeID = mode;
      return 1;
    }
    ++mode;
  }

  return 0;
}

int CGxDeviceOpenGl::IDevAttachGlContext(const CGxFormat &format) {
  FATALASSERT(m_hdc == 0 && m_hglrc == 0);
  m_hdc = GetDC(m_hwnd);
  if (m_hdc) {
    m_hglrc = AttachGlContext(m_hwnd, m_hdc, format);
  }
  if (!m_hdc || !m_hglrc) {
    IDevRemoveGlContext();
    return 0;
  }

  wglMakeCurrent(m_hdc, m_hglrc);
  BindGlExtensions();
  if (wglEXTSwapControl) {
    wglSwapIntervalEXT(format.vsync != 0);
  }
  ISetGlCaps();
  DeviceCreatePbuffer();
  IStateSetContextDefaults();
  IRsSync(1);
  ISceneBegin(3);
  m_context = 1;
  return 1;
}

void CGxDeviceOpenGl::IDevRemoveGlContext() {
  DeviceDestroyPbuffer();
  if (m_hglrc) {
    ITexForceRecreation();
    IShaderForceRecreation();
    UnbindGlExtensions();
    RemoveGlContext(m_hglrc);
    m_hglrc = 0;
  }
  if (m_hdc) {
    SetDeviceGammaRamp(m_hdc, &m_systemGammaRamp);
    ReleaseDC(m_hwnd, m_hdc);
    m_hdc = 0;
  }
  m_context = 0;
}

long CALLBACK CGxDeviceOpenGl::WindowProcGl(HWND window, unsigned int message, unsigned int wparam, long lparam) {
  CGxDeviceOpenGl *dev = reinterpret_cast<CGxDeviceOpenGl *>(GetWindowLongA(window, GWL_USERDATA));
  switch (message) {
    case WM_CREATE: {
      CREATESTRUCTA *create = reinterpret_cast<CREATESTRUCTA *>(lparam);
      SetWindowLongA(window, GWL_USERDATA, reinterpret_cast<LONG>(create->lpCreateParams));
      return 0;
    }
    case WM_DESTROY:
      dev->DeviceWM(GxWM_Destroy, 0, 0);
      return 0;
    case WM_SIZE: {
      NTempest::CRect rect(0.0f, 0.0f, static_cast<float>(HIWORD(lparam)), static_cast<float>(LOWORD(lparam)));
      dev->DeviceWM(GxWM_Size, reinterpret_cast<long>(&rect), 0);
      break;
    }
    case WM_SETFOCUS:
      dev->DeviceWM(GxWM_SetFocus, 0, 0);
      return 0;
    case WM_KILLFOCUS:
      dev->DeviceWM(GxWM_KillFocus, 0, 0);
      return 0;
    case WM_PAINT: {
      PAINTSTRUCT ps;
      BeginPaint(window, &ps);
      EndPaint(window, &ps);
      return 0;
    }
    case WM_ERASEBKGND:
      return 0;
    case WM_SYSCOMMAND:
      if (wparam == SC_SCREENSAVE || wparam == SC_MONITORPOWER) {
        return 0;
      }
      break;
  }
  if (dev && dev->m_windowProc) {
    return dev->m_windowProc(window, message, wparam, lparam);
  }
  return DefWindowProcA(window, message, wparam, lparam);
}

void CGxDeviceOpenGl::DeviceWM(EGxWM wm, long param1, long param2) {
  switch (wm) {
    case GxWM_Size:
      DeviceSetDefWindow(*reinterpret_cast<NTempest::CRect *>(param1));
      DeviceQueryPbuffer();
      break;
    case GxWM_Destroy:
      IDevRemoveGlContext();
      IDevSetFocus(0, m_format);
      break;
    case GxWM_SetFocus:
      if (!s_inCreateOrDestroy && !IDevIsWindowed()) {
        if (m_hglrc) {
          DeviceQueryPbuffer();
        } else {
          IDevSetFocus(1, m_format);
          IDevAttachGlContext(m_format);
          DeviceSetGamma(m_gammaRamp);
        }
      }
      break;
    case GxWM_KillFocus:
      if (!s_inCreateOrDestroy && !IDevIsWindowed()) {
        IDevRemoveGlContext();
        IDevSetFocus(0, m_format);
      }
      break;
  }
}

int CGxDeviceOpenGl::DeviceCreate(GXWINDOWPROC windowProc, const CGxFormat &format) {
  m_ownhwnd = 1;
  HDC hDC = GetDC(0);
  if (GetDeviceGammaRamp(hDC, &m_systemGammaRamp)) {
    m_gammaRamp = m_systemGammaRamp;
  }
  ReleaseDC(0, hDC);
  m_hwndClass = WindowClassCreate();
  if (m_hwndClass && CGxDevice::DeviceCreate(windowProc, format)) {
    return 1;
  }
  DeviceDestroy();
  return 0;
}

int CGxDeviceOpenGl::DeviceCreate(unsigned int clienthwnd, const CGxFormat &format) {
  s_inCreateOrDestroy = 1;
  m_ownhwnd = 0;
  HDC hDC = GetDC(0);
  if (GetDeviceGammaRamp(hDC, &m_systemGammaRamp)) {
    m_gammaRamp = m_systemGammaRamp;
  }
  ReleaseDC(0, hDC);
  m_hwnd = reinterpret_cast<HWND>(clienthwnd);
  if (IDevAttachGlContext(format)) {
    s_inCreateOrDestroy = 0;
    if (CGxDevice::DeviceCreate(clienthwnd, format))
      return 1;
  }
  DeviceDestroy();
  s_inCreateOrDestroy = 0;
  return 0;
}

void CGxDeviceOpenGl::DeviceDestroy() {
  CGxDevice::DeviceDestroy();
  DeviceDestroyPbuffer();

  s_inCreateOrDestroy = 1;
  if (m_hwnd && m_ownhwnd) {
    WindowDestroy(m_hwnd);
  }
  if (m_hwndClass) {
    WindowClassDestroy(m_hwndClass);
  }
  s_inCreateOrDestroy = 0;
}

int CGxDeviceOpenGl::DeviceSetFormat(const CGxFormat &format) {
  FATALASSERT(m_ownhwnd);
  Log("CGxDeviceOpenGl::DeviceSetFormat():");
  Log(format);

  s_inCreateOrDestroy = 1;
  IDevRemoveGlContext();
  WindowDestroy(m_hwnd);
  if (SetFormatMode(format)) {
    m_hwnd = WindowCreate(this, format);
    if (m_hwnd) {
      IDevSetFocus(1, format);
      if (IDevAttachGlContext(format)) {
        DeviceSetGamma(m_gammaRamp);
        s_inCreateOrDestroy = 0;
        if (CGxDevice::DeviceSetFormat(format)) {
          return 1;
        }
      }
    }
  }
  s_inCreateOrDestroy = 0;
  return 0;
}

void CGxDeviceOpenGl::DeviceSetBaseMipLevel(unsigned int baseMipLevel) {
  CGxDevice::DeviceSetBaseMipLevel(baseMipLevel);
  ITexForceRecreation();
}

void CGxDeviceOpenGl::DeviceSetGamma(float gamma) {
  CGxDevice::DeviceSetGamma(gamma);

  if (!m_format.window) {
    SetDeviceGammaRamp(m_hdc, &m_gammaRamp);
  }
}

void CGxDeviceOpenGl::DeviceSetGamma(const CGxGammaRamp &ramp) {
  CGxDevice::DeviceSetGamma(ramp);

  if (!m_format.window) {
    SetDeviceGammaRamp(m_hdc, &m_gammaRamp);
  }
}

void CGxDeviceOpenGl::DeviceSetRenderTarget(EGxBuffer buffer, CGxTex *gxTex, unsigned int plane) {
  TextureTarget &target = m_textureTarget[buffer];
  if (target.m_texture == gxTex && target.m_plane == plane) {
    return;
  }

  CGxDevice::DeviceSetRenderTarget(buffer, gxTex, plane);
  CGxTex *oldTex = static_cast<CGxTex *>(target.m_apiSpecific);
  if (oldTex) {
    BindTexture(oldTex, static_cast<unsigned int>(-1));
    if (oldTex->m_needsCreation) {
      glCopyTexImage2D(GL_TEXTURE_2D, 0, s_convertTexFmt[oldTex->m_format], 0, 0, oldTex->m_width, oldTex->m_height, 0);
    } else {
      glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, oldTex->m_width, oldTex->m_height);
    }
    oldTex->m_needsCreation = 0;
    target.m_apiSpecific = 0;
  }

  target.m_apiSpecific = gxTex;
  HDC   hdc = m_hdc;
  HGLRC hglrc = m_hglrc;
  if (m_textureTarget[GxBuffers_Color].m_apiSpecific || m_textureTarget[GxBuffers_Depth].m_apiSpecific) {
    hdc = m_hPbufferDC;
    hglrc = m_hPbufferRC;
  }
  wglMakeCurrent(hdc, hglrc);
  XformSetViewport(
      m_viewport.x.l, m_viewport.x.h, m_viewport.y.l, m_viewport.y.h, m_viewport.z.l, m_viewport.z.h
  );
  XformSetProjection(m_projection);
}

void CGxDeviceOpenGl::DeviceSetTextureQuality(int force32) {
  CGxDevice::DeviceSetTextureQuality(force32);
  ITexForceRecreation();
}

unsigned long CGxDeviceOpenGl::DeviceWindow() {
  return reinterpret_cast<unsigned long>(m_hwnd);
}

static int IsGlDisplayModeGood(const DEVMODEA &dm) {
  return (dm.dmBitsPerPel == 16 || dm.dmBitsPerPel == 32) && dm.dmPelsWidth >= 640 && dm.dmPelsHeight >= 480;
}

int CGxDevice::OpenGlEnumFormats(TSGrowableArray<CGxFormat> &formats) {
  DISPLAY_DEVICEA dd;
  DEVMODEA        dm;
  CGxFormat       fmt;
  unsigned int    mode;

  dd.cb = sizeof(dd);
  EnumDisplayDevicesTarget(0, 0, &dd, 0);
  if (!(dd.StateFlags & 1)) {
    return 0;
  }

  mode = 0;
  dm.dmSize = sizeof(dm);

  while (EnumDisplaySettingsA(reinterpret_cast<const char *>(dd.DeviceName), mode, &dm)) {
    if (IsGlDisplayModeGood(dm)) {
      memset(&fmt, 0, sizeof(fmt));
      fmt.apiSpecificModeID = mode;
      fmt.size.x = dm.dmPelsWidth;
      fmt.size.y = dm.dmPelsHeight;
      fmt.refreshRate = dm.dmDisplayFrequency;
      formats.Add(1, &fmt);
    }
    ++mode;
  }

  return formats.Count() != 0;
}

void CGxDeviceOpenGl::CapsWindowSizeInScreenCoords(NTempest::CRect &dst) {
  const NTempest::CRect &windowRect = DeviceCurWindow();

  ASSERT(windowRect.Width() * windowRect.Height() > 1.0f);

  RECT wrect = {0, 0, static_cast<LONG>(windowRect.r), static_cast<LONG>(windowRect.b)};
  MapWindowPoints(m_hwnd, 0, reinterpret_cast<LPPOINT>(&wrect), 2);

  dst.t = static_cast<float>(wrect.top);
  dst.l = static_cast<float>(wrect.left);
  dst.b = static_cast<float>(wrect.bottom);
  dst.r = static_cast<float>(wrect.right);
}
