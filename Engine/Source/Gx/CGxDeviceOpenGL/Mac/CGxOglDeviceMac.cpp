#include "../CGxDeviceOpenGl.h"
#include "../GlExtSupport.h"

#include <storm.h>

#include <string.h>

#include "GlWndSupportMac.h"

#include <OpenGL/gl.h>

#include <ApplicationServices/ApplicationServices.h>

static int s_inCreateOrDestroy;

static HWND WindowCreate(CGxDeviceOpenGl *dev, const CGxFormat &format) {
  return reinterpret_cast<HWND>(GxMacWindowCreate(format.pos.x, format.pos.y, format.size.x, format.size.y, format.window));
}

static void WindowDestroy(HWND &hwnd) {
  if (hwnd) {
    GxMacWindowDestroy(reinterpret_cast<LPVOID>(hwnd));
    hwnd = 0;
  }
}

void CGxDeviceOpenGl::DeviceCreatePbuffer() {
  m_hPbuffer = 0;
  m_hPbufferDC = 0;
  m_hPbufferRC = 0;

  // render targets are served by copying out of the back buffer rather than
  // by a pbuffer, so the format the window was made with is what they get
  m_caps.m_rttFormat[GxTex_Argb8888] = 1;
}

void CGxDeviceOpenGl::DeviceQueryPbuffer() {
}

void CGxDeviceOpenGl::DeviceDestroyPbuffer() {
  m_hPbuffer = 0;
  m_hPbufferDC = 0;
  m_hPbufferRC = 0;
}

void CGxDeviceOpenGl::IDevSetFocus(int focus, const CGxFormat &format) {
  if (!m_hwnd) {
    return;
  }

  GxMacWindowShow(reinterpret_cast<LPVOID>(m_hwnd), format.window, focus);

  if (!format.window) {
    if (focus) {
      CGDisplayCapture(kCGDirectMainDisplay);
    } else {
      CGDisplayRelease(kCGDirectMainDisplay);
    }
  }
}

int CGxDeviceOpenGl::SetFormatMode(const CGxFormat &format) {
  if (format.window) {
    format.apiSpecificModeID = 0;
    return 1;
  }

  CFArrayRef modes = CGDisplayCopyAllDisplayModes(kCGDirectMainDisplay, 0);
  CFIndex    count;
  CFIndex    index;

  if (!modes) {
    return 0;
  }

  count = CFArrayGetCount(modes);

  for (index = 0; index < count; ++index) {
    CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(modes, index);

    if (static_cast<int>(CGDisplayModeGetWidth(mode)) == format.size.x && static_cast<int>(CGDisplayModeGetHeight(mode)) == format.size.y) {
      format.apiSpecificModeID = static_cast<DWORD>(index);
      CFRelease(modes);
      return 1;
    }
  }

  CFRelease(modes);
  return 0;
}

int CGxDeviceOpenGl::IDevAttachGlContext(const CGxFormat &format) {
  FATALASSERT(m_hdc == 0 && m_hglrc == 0);

  LPVOID view = GxMacWindowContentView(reinterpret_cast<LPVOID>(m_hwnd));
  LPVOID context;

  if (!view) {
    IDevRemoveGlContext();
    return 0;
  }

  context = GxMacContextCreate(
      view, format.colorFormat ? 32 : 16, format.depthFormat == CGxFormat::Fmt_Ds160 ? 16 : 24, format.depthFormat == CGxFormat::Fmt_Ds248 ? 8 : 0,
      format.vsync
  );

  if (!context) {
    IDevRemoveGlContext();
    return 0;
  }

  m_hdc = reinterpret_cast<HDC>(view);
  m_hglrc = reinterpret_cast<HGLRC>(context);

  GxMacContextMakeCurrent(context);

  // the client draws its own cursor over the context
  CGDisplayHideCursor(kCGDirectMainDisplay);

  BindGlExtensions();
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

    GxMacContextClearCurrent();
    GxMacContextDestroy(reinterpret_cast<LPVOID>(m_hglrc));

    CGDisplayShowCursor(kCGDirectMainDisplay);

    m_hglrc = 0;
  }

  if (m_hdc) {
    CGDisplayRestoreColorSyncSettings();
    m_hdc = 0;
  }

  m_context = 0;
}

void CGxDeviceOpenGl::DeviceWM(EGxWM wm, intptr_t param1, intptr_t param2) {
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
  m_gammaRamp = m_systemGammaRamp;

  if (CGxDevice::DeviceCreate(windowProc, format)) {
    return 1;
  }

  DeviceDestroy();
  return 0;
}

int CGxDeviceOpenGl::DeviceCreate(UINT clienthwnd, const CGxFormat &format) {
  s_inCreateOrDestroy = 1;
  m_ownhwnd = 0;
  m_gammaRamp = m_systemGammaRamp;
  m_hwnd = reinterpret_cast<HWND>(static_cast<uintptr_t>(clienthwnd));

  if (IDevAttachGlContext(format)) {
    s_inCreateOrDestroy = 0;

    if (CGxDevice::DeviceCreate(clienthwnd, format)) {
      return 1;
    }
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
        NTempest::CRect rect(0.0f, 0.0f, static_cast<float>(format.size.y), static_cast<float>(format.size.x));

        // cocoa has no window proc, so the size the window was made with is
        // reported the way WM_SIZE would have reported it
        DeviceWM(GxWM_Size, reinterpret_cast<intptr_t>(&rect), 0);
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

void CGxDeviceOpenGl::DeviceSetBaseMipLevel(UINT baseMipLevel) {
  CGxDevice::DeviceSetBaseMipLevel(baseMipLevel);
  ITexForceRecreation();
}

static void ApplyGammaRamp(const CGxGammaRamp &ramp) {
  CGGammaValue red[CGxGammaRamp::ENTRIES];
  CGGammaValue green[CGxGammaRamp::ENTRIES];
  CGGammaValue blue[CGxGammaRamp::ENTRIES];
  UINT         index;

  for (index = 0; index < CGxGammaRamp::ENTRIES; ++index) {
    red[index] = ramp.red[index] / 65535.0f;
    green[index] = ramp.green[index] / 65535.0f;
    blue[index] = ramp.blue[index] / 65535.0f;
  }

  CGSetDisplayTransferByTable(kCGDirectMainDisplay, CGxGammaRamp::ENTRIES, red, green, blue);
}

void CGxDeviceOpenGl::DeviceSetGamma(float gamma) {
  CGxDevice::DeviceSetGamma(gamma);

  if (!m_format.window) {
    ApplyGammaRamp(m_gammaRamp);
  }
}

void CGxDeviceOpenGl::DeviceSetGamma(const CGxGammaRamp &ramp) {
  CGxDevice::DeviceSetGamma(ramp);

  if (!m_format.window) {
    ApplyGammaRamp(m_gammaRamp);
  }
}

void CGxDeviceOpenGl::DeviceSetRenderTarget(EGxBuffer buffer, CGxTex *gxTex, UINT plane) {
  TextureTarget &target = m_textureTarget[buffer];

  if (target.m_texture == gxTex && target.m_plane == plane) {
    return;
  }

  CGxDevice::DeviceSetRenderTarget(buffer, gxTex, plane);

  CGxTex *oldTex = static_cast<CGxTex *>(target.m_apiSpecific);
  if (oldTex) {
    BindTexture(oldTex, static_cast<UINT>(-1));

    if (oldTex->m_needsCreation) {
      glCopyTexImage2D(GL_TEXTURE_2D, 0, s_convertTexFmt[oldTex->m_format], 0, 0, oldTex->m_width, oldTex->m_height, 0);
    } else {
      glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, oldTex->m_width, oldTex->m_height);
    }

    oldTex->m_needsCreation = 0;
    target.m_apiSpecific = 0;
  }

  target.m_apiSpecific = gxTex;

  GxMacContextMakeCurrent(reinterpret_cast<LPVOID>(m_hglrc));

  XformSetViewport(m_viewport.x.l, m_viewport.x.h, m_viewport.y.l, m_viewport.y.h, m_viewport.z.l, m_viewport.z.h);
  XformSetProjection(m_projection);
}

void CGxDeviceOpenGl::DeviceSetTextureQuality(int force32) {
  CGxDevice::DeviceSetTextureQuality(force32);
  ITexForceRecreation();
}

uintptr_t CGxDeviceOpenGl::DeviceWindow() {
  return reinterpret_cast<uintptr_t>(m_hwnd);
}

void CGxDeviceOpenGl::CapsWindowSizeInScreenCoords(NTempest::CRect &dst) {
  const NTempest::CRect &windowRect = DeviceCurWindow();

  ASSERT(windowRect.Width() * windowRect.Height() > 1.0f);

  double l;
  double t;
  double r;
  double b;

  GxMacWindowContentRectInScreen(reinterpret_cast<LPVOID>(m_hwnd), windowRect.r, windowRect.b, &l, &t, &r, &b);

  dst.l = static_cast<float>(l);
  dst.t = static_cast<float>(t);
  dst.r = static_cast<float>(r);
  dst.b = static_cast<float>(b);
}

static int IsGlDisplayModeGood(CGDisplayModeRef mode) {
  return CGDisplayModeGetWidth(mode) >= 640 && CGDisplayModeGetHeight(mode) >= 480;
}

int CGxDevice::OpenGlEnumFormats(TSGrowableArray<CGxFormat> &formats) {
  CFArrayRef modes = CGDisplayCopyAllDisplayModes(kCGDirectMainDisplay, 0);
  CFIndex    count;
  CFIndex    index;

  if (!modes) {
    return 0;
  }

  count = CFArrayGetCount(modes);

  for (index = 0; index < count; ++index) {
    CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(modes, index);
    CGxFormat        fmt;

    if (!IsGlDisplayModeGood(mode)) {
      continue;
    }

    memset(&fmt, 0, sizeof(fmt));
    fmt.apiSpecificModeID = static_cast<DWORD>(index);
    fmt.size.x = static_cast<int>(CGDisplayModeGetWidth(mode));
    fmt.size.y = static_cast<int>(CGDisplayModeGetHeight(mode));
    fmt.refreshRate = static_cast<UINT>(CGDisplayModeGetRefreshRate(mode));
    formats.Add(1, &fmt);
  }

  CFRelease(modes);
  return formats.Count() != 0;
}

int CGxDevice::D3dEnumFormats(TSGrowableArray<CGxFormat> &formats) {
  return 0;
}

CGxDevice *CGxDevice::NewD3d() {
  return 0;
}

int CGxDevice::AdapterID(WORD &vendorID, WORD &deviceID, UINT &driverVersionHi, UINT &driverVersionLow) {
  vendorID = 0;
  deviceID = 0;
  driverVersionHi = 0;
  driverVersionLow = 0;
  return 0;
}

int CGxDevice::AdapterInfer(WORD &deviceID) {
  deviceID = 0;
  return 0;
}

int CGxDevice::AdapterMonitorModes(TSGrowableArray<CGxMonitorMode> &modes) {
  CFArrayRef displayModes = CGDisplayCopyAllDisplayModes(kCGDirectMainDisplay, 0);
  CFIndex    count;
  CFIndex    index;

  if (!displayModes) {
    return 0;
  }

  count = CFArrayGetCount(displayModes);

  for (index = 0; index < count; ++index) {
    CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(displayModes, index);
    CGxMonitorMode   monitorMode;

    monitorMode.size.x = static_cast<int>(CGDisplayModeGetWidth(mode));
    monitorMode.size.y = static_cast<int>(CGDisplayModeGetHeight(mode));
    monitorMode.bpp = 32;
    monitorMode.refreshRate = static_cast<UINT>(CGDisplayModeGetRefreshRate(mode));
    modes.Add(1, &monitorMode);
  }

  CFRelease(displayModes);
  return modes.Count() != 0;
}

int CGxDevice::AdapterDesktopMode(CGxMonitorMode &mode) {
  CGDisplayModeRef displayMode = CGDisplayCopyDisplayMode(kCGDirectMainDisplay);

  if (!displayMode) {
    return 0;
  }

  mode.size.x = static_cast<int>(CGDisplayModeGetWidth(displayMode));
  mode.size.y = static_cast<int>(CGDisplayModeGetHeight(displayMode));
  mode.bpp = 32;
  mode.refreshRate = static_cast<UINT>(CGDisplayModeGetRefreshRate(displayMode));

  CGDisplayModeRelease(displayMode);
  return 1;
}
