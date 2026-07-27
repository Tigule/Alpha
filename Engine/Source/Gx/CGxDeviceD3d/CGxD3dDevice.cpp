#include "CGxDeviceD3d.h"

static const char s_WndClassName[] = "GxWindowClassD3d";

CGxDeviceD3d *CGxDeviceD3d::m_thisDevice;

const _D3DFORMAT CGxDeviceD3d::s_GxFormatToD3dFormat[CGxFormat::Formats_Last] = {D3DFMT_R5G6B5, D3DFMT_X8R8G8B8, D3DFMT_A8R8G8B8, D3DFMT_A2R10G10B10,
                                                                                 D3DFMT_D16,    D3DFMT_D24X8,    D3DFMT_D24S8,    D3DFMT_D32};

LRESULT CALLBACK CGxDeviceD3d::WindowProcD3d(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  CGxDeviceD3d *device = reinterpret_cast<CGxDeviceD3d *>(GetWindowLongA(hWnd, GWL_USERDATA));

  switch (uMsg) {
    case WM_CREATE: {
      CREATESTRUCTA *create = reinterpret_cast<CREATESTRUCTA *>(lParam);
      SetWindowLongA(hWnd, GWL_USERDATA, reinterpret_cast<LONG>(create->lpCreateParams));
      return 0;
    }

    case WM_ERASEBKGND:
      return 0;

    case WM_PAINT: {
      PAINTSTRUCT ps;
      BeginPaint(hWnd, &ps);
      EndPaint(hWnd, &ps);
      return 0;
    }

    case WM_SIZE: {
      NTempest::CRect rect;
      rect.t = 0.0f;
      rect.l = 0.0f;
      rect.b = static_cast<float>(HIWORD(lParam));
      rect.r = static_cast<float>(LOWORD(lParam));

      long sizeCode = 0;
      if (wParam == SIZE_MINIMIZED) {
        sizeCode = 1;
      } else if (wParam == SIZE_MAXHIDE) {
        sizeCode = 2;
      }
      device->DeviceWM(GxWM_Size, reinterpret_cast<long>(&rect), sizeCode);
      break;
    }

    case WM_DISPLAYCHANGE: {
      NTempest::CRect rect;
      rect.t = 0.0f;
      rect.l = 0.0f;
      rect.b = static_cast<float>(HIWORD(lParam));
      rect.r = static_cast<float>(LOWORD(lParam));
      device->DeviceWM(GxWM_DisplayChange, reinterpret_cast<long>(&rect), 0);
      break;
    }

    case WM_SYSCOMMAND:
      if (wParam == SC_SCREENSAVE || wParam == SC_MONITORPOWER) {
        return 0;
      }
      break;
  }

  if (device && device->m_windowProc) {
    return device->m_windowProc(hWnd, uMsg, wParam, lParam);
  }
  return DefWindowProcA(hWnd, uMsg, wParam, lParam);
}

static unsigned short WindowClassCreate() {
  HINSTANCE   instance = GetModuleHandleA(0);
  WNDCLASSEXA wc;

  memset(&wc, 0, sizeof(wc));
  wc.cbSize = sizeof(wc);
  wc.style = CS_OWNDC;
  wc.lpfnWndProc = CGxDeviceD3d::WindowProcD3d;
  wc.hInstance = instance;
  wc.lpszClassName = s_WndClassName;
  wc.hIcon = static_cast<HICON>(LoadImageA(instance, "BlizzardIcon.ico", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
  wc.hCursor = LoadCursorA(instance, "BlizzardCursor.cur");
  if (!wc.hCursor) {
    wc.hCursor = LoadCursorA(instance, IDC_ARROW);
  }

  return RegisterClassExA(&wc);
}

static HWND WindowCreate(CGxDeviceD3d *dev, const CGxFormat &format) {
  HINSTANCE     instance = GetModuleHandleA(0);
  CGxFormat     fmt = format;
  unsigned long style =
      format.window ? WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS : WS_POPUP | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

  if (!fmt.size.x) {
    fmt.size.x = CW_USEDEFAULT;
  }
  if (!fmt.size.y) {
    fmt.size.y = CW_USEDEFAULT;
  }

  HWND window = CreateWindowExA(
      WS_EX_APPWINDOW, s_WndClassName, "A game in progress", style, fmt.pos.x, fmt.pos.y, fmt.size.x, fmt.size.y, 0, 0, instance, dev
  );
  ShowWindow(window, SW_SHOWNORMAL);
  return window;
}

static void WindowDestroy(HWND &hwnd) {
  DestroyWindow(hwnd);
  hwnd = 0;
}

static void WindowClassDestroy(unsigned short &hClass) {
  UnregisterClassA(reinterpret_cast<const char *>(hClass), GetModuleHandleA(0));
  hClass = 0;
}

CGxDeviceD3d::CGxDeviceD3d() {
  for (unsigned int i = 0; i < 8; ++i) {
    m_d3dStatesLight[i].which = static_cast<unsigned long>(-1);
    m_d3dStatesLight[i].chkSum = 0;
  }

  m_api = GxApi_Direct3d;
  m_hwnd = 0;
  m_d3dLib = 0;
  m_d3d = 0;
  m_d3dDevice = 0;
  m_d3dIsHwDevice = 0;
  m_d3dNeedsReset = 0;
  m_caps.m_colorFormat = GxCF_argb;
  memset(m_IB, 0, sizeof(m_IB));
  m_primType = GxPrims_Last;
  m_primIndexCount = 0;
  m_windowVisible = 0;
  memset(&m_desktopDisplayMode, 0, sizeof(m_desktopDisplayMode));
  m_deviceSupports32BitTextures = 0;

  ASSERT(m_thisDevice == 0);
  m_thisDevice = this;

  memset(m_deviceState, 0xFF, sizeof(m_deviceState));
  memset(m_texEnable, 0, sizeof(m_texEnable));
  m_processedVertexPtrs = 0;
  m_processedIndexPtrs = 0;
  m_rttColorSurface = 0;
  m_rttDepthSurface = 0;
  m_defColorSurface = 0;
  m_defDepthSurface = 0;
}

CGxDeviceD3d::~CGxDeviceD3d() {
  m_thisDevice = 0;
}

int CGxDeviceD3d::ILoadD3dLib(HINSTANCE &d3dLib, IDirect3D9 *&d3d) {
  typedef IDirect3D9 *(__stdcall * D3dCreateProc)(unsigned int);

  D3dCreateProc d3dCreateProc;
  const char   *failure;

  d3dLib = 0;
  d3d = 0;

  d3dLib = LoadLibraryA("d3d9.dll");
  if (!d3dLib) {
    failure = "CGxDeviceD3d::ILoadD3dLib(): unable to LoadLibrary()";
    goto failed;
  }

  d3dCreateProc = reinterpret_cast<D3dCreateProc>(GetProcAddress(d3dLib, "Direct3DCreate9"));
  if (!d3dCreateProc) {
    failure = "CGxDeviceD3d::ILoadD3dLib(): unable to GetProcAddress()";
    goto failed;
  }

  d3d = d3dCreateProc(D3D_SDK_VERSION);
  if (!d3d) {
    failure = "CGxDeviceD3d::ILoadD3dLib(): unable to d3dCreateProc()";
    goto failed;
  }

  return 1;

failed:
  Log(failure);
  IUnloadD3dLib(d3dLib, d3d);
  return 0;
}

void CGxDeviceD3d::IUnloadD3dLib(HINSTANCE &d3dLib, IDirect3D9 *&d3d) {
  if (d3d) {
    d3d->Release();
    d3d = 0;
  }
  if (d3dLib) {
    FreeLibrary(d3dLib);
    d3dLib = 0;
  }
}

void CGxDeviceD3d::ISetCaps() {
  m_caps.m_numTmus = m_d3dCaps.MaxSimultaneousTextures < 4 ? m_d3dCaps.MaxSimultaneousTextures : 4;
  m_caps.m_pixelCenterOnEdge = 0;
  m_caps.m_texelCenterOnEdge = 1;
  m_caps.m_maxTextureSize = m_d3dCaps.MaxTextureWidth <= 0x200 ? m_d3dCaps.MaxTextureWidth : 0x200;
  m_caps.m_texOpAdd = (m_d3dCaps.TextureOpCaps >> 6) & 1;
  m_caps.m_texOpMod2X = (m_d3dCaps.TextureOpCaps & 5) != 0;
  m_caps.m_maxIndex = m_d3dCaps.MaxVertexIndex;
  m_caps.m_texFilterTrilinear = (m_d3dCaps.TextureFilterCaps >> 17) & 1;
  m_caps.m_texFilterAnisotropic = (m_d3dCaps.TextureFilterCaps >> 10) & 1;
  m_caps.m_maxTexAnisotropy = m_d3dCaps.MaxAnisotropy;
  if (m_caps.m_texFilterAnisotropic && m_caps.m_maxTexAnisotropy < 2) {
    m_caps.m_texFilterAnisotropic = 0;
  }
  m_caps.m_mipMapLodBias = (m_d3dCaps.RasterCaps >> 13) & 1;
  m_caps.m_depthBias = (m_d3dCaps.RasterCaps >> 26) & 1;

  switch (m_d3dCaps.PixelShaderVersion) {
    case 0xFFFF0200:
      m_caps.m_pixelShaderTarget = CGxPixelShader::Target_ps_2_0;
      break;
    case 0xFFFF0104:
    case 0xFFFF0103:
      m_caps.m_pixelShaderTarget = CGxPixelShader::Target_ps_1_3;
      break;
    case 0xFFFF0102:
      m_caps.m_pixelShaderTarget = CGxPixelShader::Target_ps_1_2;
      break;
    case 0xFFFF0101:
      m_caps.m_pixelShaderTarget = CGxPixelShader::Target_ps_1_1;
      break;
    default:
      m_caps.m_pixelShaderTarget = CGxPixelShader::Target_gx;
      break;
  }

  m_deviceSupports32BitTextures = ICheckTextureFormat(0, D3DFMT_A8R8G8B8);
  m_caps.m_texFmtDxt = ICheckTextureFormat(0, D3DFMT_DXT1) && ICheckTextureFormat(0, D3DFMT_DXT3) && ICheckTextureFormat(0, D3DFMT_DXT5);
  if (m_d3dCaps.Caps2 & 0x40000000) {
    m_caps.m_generateMipMaps = ICheckTextureFormat(0x400, D3DFMT_A8R8G8B8) && ICheckTextureFormat(0x400, D3DFMT_A4R4G4B4) &&
                               ICheckTextureFormat(0x400, D3DFMT_A1R5G5B5) && ICheckTextureFormat(0x400, D3DFMT_R5G6B5);
  }
  m_caps.m_rttFormat[GxTex_Argb8888] = ICheckTextureFormat(1, D3DFMT_A8R8G8B8);
  m_caps.m_rttFormat[GxTex_Rgb565] = ICheckTextureFormat(1, D3DFMT_R5G6B5);
}

int CGxDeviceD3d::ICreateD3d() {
  if (!ILoadD3dLib(m_d3dLib, m_d3d) || m_d3d->GetDeviceCaps(0, D3DDEVTYPE_HAL, &m_d3dCaps) < 0) {
    IDestroyD3d();
    return 0;
  }

  if (m_desktopDisplayMode.Format == D3DFMT_UNKNOWN) {
    D3DDISPLAYMODE oldDisplayMode;
    if (m_d3d->GetAdapterDisplayMode(0, &oldDisplayMode) < 0) {
      IDestroyD3d();
      return 0;
    }
    m_desktopDisplayMode = oldDisplayMode;
  }

  return 1;
}

void CGxDeviceD3d::IDestroyD3d() {
  IDestroyD3dDevice();
  IUnloadD3dLib(m_d3dLib, m_d3d);
}

int CGxDeviceD3d::ICheckTextureFormat(unsigned long usage, _D3DFORMAT textureFormat) {
  return m_d3d->CheckDeviceFormat(0, D3DDEVTYPE_HAL, m_devAdapterFormat, usage, D3DRTYPE_TEXTURE, textureFormat) == 0;
}

int CGxDeviceD3d::IAllocBuffers() {
  for (unsigned int format = 0; format < GxVertexBufferFormats_Last; ++format) {
    ICreateBuffers(static_cast<EGxVertexBufferFormat>(format), 0x4000, m_VBL[GxBWF_Dynamic][format], 0xC000, m_IB[GxBWF_Dynamic][0]);

    if (!m_VBL[GxBWF_Dynamic][format].m_vbList.Count() || !m_IB[GxBWF_Dynamic][0]) {
      return 0;
    }

    for (unsigned int frequency = GxBWF_Low; frequency <= GxBWF_Medium; ++frequency) {
      BufReserve(
          static_cast<EGxBufWriteFreq>(frequency), static_cast<EGxVertexBufferFormat>(format), m_VBReserve[frequency][format],
          m_IBReserve[frequency][format]
      );
    }
  }

  return 1;
}

int CGxDeviceD3d::ICreateD3dDevice(const CGxFormat &format) {
  bool hwTnL = format.hwTnL;
  if (hwTnL && !(m_d3dCaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)) {
    hwTnL = false;
  }
  m_d3dIsHwDevice = hwTnL;

  D3DPRESENT_PARAMETERS d3dpp;
  ISetPresentParms(d3dpp, format);

  long result = m_d3d->CreateDevice(
      0, D3DDEVTYPE_HAL, m_hwnd, D3DCREATE_FPU_PRESERVE | (hwTnL ? D3DCREATE_HARDWARE_VERTEXPROCESSING : D3DCREATE_SOFTWARE_VERTEXPROCESSING), &d3dpp,
      &m_d3dDevice
  );
  if (result < 0) {
    switch (result) {
      case D3DERR_DRIVERINTERNALERROR:
        Log("CGxDeviceD3d::ICreateD3dDevice(): D3D CreateDevice failure: Driver internal error");
        break;
      case D3DERR_OUTOFVIDEOMEMORY:
        Log("CGxDeviceD3d::ICreateD3dDevice(): D3D CreateDevice failure: Direct3D does not have enough display memory to perform the operation");
        break;
      case D3DERR_NOTAVAILABLE:
        Log("CGxDeviceD3d::ICreateD3dDevice(): D3D CreateDevice failure: This device does not support the queried technique");
        break;
      case D3DERR_INVALIDCALL:
        Log("CGxDeviceD3d::ICreateD3dDevice(): D3D CreateDevice failure: Invalid call");
        break;
    }
    m_d3dDevice = 0;
    return 0;
  }

  m_devAdapterFormat = d3dpp.BackBufferFormat;
  m_devDepthFormat = d3dpp.AutoDepthStencilFormat;
  if (!IAllocBuffers()) {
    IDestroyD3dDevice();
    return 0;
  }

  ISetCaps();
  Log(m_caps);
  IStateSetD3DDefaults();
  DeviceSetGamma(m_gammaRamp);
  return 1;
}

void CGxDeviceD3d::IDestroyD3dDevice() {
  IReleaseD3dResources(1);
  if (m_d3dDevice) {
    m_d3dDevice->Release();
    m_d3dDevice = 0;
  }
}

void CGxDeviceD3d::ISetPresentParms(D3DPRESENT_PARAMETERS &d3dpp, const CGxFormat &format) {
  memset(&d3dpp, 0, sizeof(d3dpp));

  if (format.window) {
    D3DDISPLAYMODE currMode;
    if (m_d3d->GetAdapterDisplayMode(0, &currMode) < 0) {
      ASSERT(0);
      currMode.Format = m_desktopDisplayMode.Format;
    }
    d3dpp.Windowed = 1;
    d3dpp.BackBufferFormat = currMode.Format;
    d3dpp.FullScreen_RefreshRateInHz = 0;
  } else {
    d3dpp.BackBufferWidth = format.size.x;
    d3dpp.BackBufferHeight = format.size.y;
    d3dpp.BackBufferFormat = s_GxFormatToD3dFormat[format.colorFormat];
    d3dpp.BackBufferCount = 1;
    d3dpp.FullScreen_RefreshRateInHz = format.refreshRate;
  }

  d3dpp.hDeviceWindow = m_hwnd;
  d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
  d3dpp.EnableAutoDepthStencil = 1;
  d3dpp.AutoDepthStencilFormat = s_GxFormatToD3dFormat[format.depthFormat];
  d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
  d3dpp.PresentationInterval = format.vsync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
}

void CGxDeviceD3d::IReleaseD3dResources(int freeTextures) {
  for (unsigned int i = 0; i < 8; ++i) {
    StateD3dLight state;
    state.which = -1;
    state.chkSum = 0;
    m_d3dStatesLight[i] = state;
  }

  ITexForceRecreation(freeTextures);
  IShaderForceRecreation(freeTextures);

  for (CGxBuf *buf = m_bufList.Head(); buf; buf = m_bufList.Next(buf)) {
    static_cast<CGxBufD3d *>(buf)->Release();
  }

  for (unsigned int frequency = 0; frequency < 4; ++frequency) {
    for (unsigned int format = 0; format < GxVertexBufferFormats_Last; ++format) {
      m_VBL[frequency][format].Release();
      if (m_IB[frequency][format]) {
        DEL(m_IB[frequency][format]);
        m_IB[frequency][format] = 0;
      }
    }
  }

  memset(m_deviceState, 0xFF, sizeof(m_deviceState));

  if (m_defColorSurface) {
    m_defColorSurface->Release();
    m_defColorSurface = 0;
  }
  if (m_defDepthSurface) {
    m_defDepthSurface->Release();
    m_defDepthSurface = 0;
  }
  if (m_rttColorSurface) {
    m_rttColorSurface->Release();
    m_rttColorSurface = 0;
  }
  if (m_rttDepthSurface) {
    m_rttDepthSurface->Release();
    m_rttDepthSurface = 0;
  }
}

int CGxDeviceD3d::DeviceCreate(GXWINDOWPROC windowProc, const CGxFormat &format) {
  m_ownhwnd = 1;

  HDC hDC = GetDC(0);
  if (GetDeviceGammaRamp(hDC, &m_systemGammaRamp)) {
    m_gammaRamp = m_systemGammaRamp;
  }
  ReleaseDC(0, hDC);

  m_hwndClass = WindowClassCreate();
  if (m_hwndClass && ICreateD3d() && CGxDevice::DeviceCreate(windowProc, format)) {
    return 1;
  }

  CGxDevice::DeviceDestroy();
  return 0;
}

int CGxDeviceD3d::DeviceCreate(unsigned int hwnd, const CGxFormat &format) {
  m_ownhwnd = 0;
  CGxDevice::DeviceCreate(hwnd, format);
  m_hwnd = reinterpret_cast<HWND>(hwnd);
  ASSERT(!"FIX ME");
  if (ICreateD3d()) {
    return 1;
  }
  CGxDevice::DeviceDestroy();
  return 0;
}

void CGxDeviceD3d::DeviceDestroy() {
  CGxDevice::DeviceDestroy();
  IDestroyD3d();
  if (m_hwnd && m_ownhwnd) {
    WindowDestroy(m_hwnd);
  }
  if (m_hwndClass) {
    WindowClassDestroy(m_hwndClass);
  }
}

int CGxDeviceD3d::DeviceSetFormat(const CGxFormat &format) {
  ASSERT(m_ownhwnd);
  Log("CGxDeviceD3d::DeviceSetFormat():");
  Log(format);

  IDestroyD3dDevice();
  WindowDestroy(m_hwnd);
  m_hwnd = WindowCreate(this, format);
  if (m_hwnd && ICreateD3dDevice(format) && CGxDevice::DeviceSetFormat(format)) {
    m_context = 1;
    return 1;
  }

  Log("CGxDeviceD3d::DeviceSetFormat(): unable to set format!");
  IDestroyD3dDevice();
  WindowDestroy(m_hwnd);
  return 0;
}

void CGxDeviceD3d::DeviceSetBaseMipLevel(unsigned int baseMipLevel) {
  CGxDevice::DeviceSetBaseMipLevel(baseMipLevel);
  ITexForceRecreation(1);
}

void CGxDeviceD3d::DeviceSetGamma(float gamma) {
  CGxDevice::DeviceSetGamma(gamma);

  if ((m_d3dCaps.Caps2 & 0x00020000) && !IDevIsWindowed()) {
    m_d3dDevice->SetGammaRamp(0, 0, reinterpret_cast<const D3DGAMMARAMP *>(&m_gammaRamp));
  }
}

void CGxDeviceD3d::DeviceSetGamma(const CGxGammaRamp &ramp) {
  CGxDevice::DeviceSetGamma(ramp);

  if ((m_d3dCaps.Caps2 & 0x00020000) && !IDevIsWindowed()) {
    m_d3dDevice->SetGammaRamp(0, 0, reinterpret_cast<const D3DGAMMARAMP *>(&m_gammaRamp));
  }
}

void CGxDeviceD3d::DeviceSetTextureQuality(int force32) {
  CGxDevice::DeviceSetTextureQuality(force32);
  ITexForceRecreation(1);
}

unsigned long CGxDeviceD3d::DeviceWindow() {
  return reinterpret_cast<unsigned long>(m_hwnd);
}

void CGxDeviceD3d::DeviceReadPixels(NTempest::CiRect &rect, TSGrowableArray<NTempest::CImVector> &pixels) {
  _D3DSURFACE_DESC   desc;
  _D3DLOCKED_RECT    r;
  int                width;
  IDirect3DSurface9 *bb;

  ClampRectToWindow(rect);

  width = rect.r - rect.l;
  pixels.SetCount(width * (rect.b - rect.t));

  if (m_d3dDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb) < 0) {
    return;
  }

  bb->GetDesc(&desc);

  if (bb->LockRect(&r, 0, D3DLOCK_READONLY | D3DLOCK_NOSYSLOCK) >= 0) {
    unsigned char *src = static_cast<unsigned char *>(r.pBits);
    unsigned char *dst = reinterpret_cast<unsigned char *>(pixels.Ptr());

    for (int y = rect.b - rect.t; y; --y) {
      switch (desc.Format) {
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
          memcpy(dst, src, width * 4);
          break;

        case D3DFMT_R5G6B5: {
          for (int x = 0; x < width; ++x) {
            unsigned short pixel = reinterpret_cast<unsigned short *>(src)[x];
            dst[x * 4 + 0] = static_cast<unsigned char>(pixel << 3);
            dst[x * 4 + 1] = static_cast<unsigned char>((pixel >> 3) & 0xFC);
            dst[x * 4 + 2] = static_cast<unsigned char>((pixel >> 8) & 0xF8);
            dst[x * 4 + 3] = 0xFF;
          }
          break;
        }

        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5: {
          for (int x = 0; x < width; ++x) {
            unsigned short pixel = reinterpret_cast<unsigned short *>(src)[x];
            dst[x * 4 + 0] = static_cast<unsigned char>(pixel << 3);
            dst[x * 4 + 1] = static_cast<unsigned char>((pixel >> 2) & 0xF8);
            dst[x * 4 + 2] = static_cast<unsigned char>((pixel >> 7) & 0xF8);
            dst[x * 4 + 3] = 0xFF;
          }
          break;
        }

        default:
          memset(dst, 0xFF, width * 4);
          break;
      }

      src += r.Pitch;
      dst += width * 4;
    }

    bb->UnlockRect();
  }

  bb->Release();
}

void CGxDeviceD3d::DeviceReadDepths(NTempest::CiRect &rect, TSGrowableArray<float> &depths) {
  ClampRectToWindow(rect);
  depths.SetCount((rect.b - rect.t) * (rect.r - rect.l));
  ASSERT(!"CGxDeviceD3d::DeviceReadDepth() not coded");
  memset(depths.Ptr(), 0, depths.Count() * sizeof(float));
}

void CGxDeviceD3d::DeviceWM(EGxWM wm, long param1, long param2) {
  const NTempest::CRect &rect = *reinterpret_cast<const NTempest::CRect *>(param1);

  switch (wm) {
    case GxWM_Size:
      if (param2 == 1 || param2 == 2) {
        m_windowVisible = 0;
        m_context = 0;
      } else {
        m_windowVisible = 1;
        m_d3dNeedsReset = 1;
        DeviceSetDefWindow(rect);
      }
      break;

    case GxWM_DisplayChange:
      if (m_windowVisible) {
        m_d3dNeedsReset = 1;
        DeviceSetDefWindow(rect);
      }
      break;
  }
}

void CGxDeviceD3d::DeviceSetRenderTarget(EGxBuffer buffer, CGxTex *gxTex, unsigned int plane) {
  TextureTarget &target = m_textureTarget[buffer];
  if (target.m_texture == gxTex && target.m_plane == plane) {
    return;
  }

  CGxDevice::DeviceSetRenderTarget(buffer, gxTex, plane);

  if (target.m_apiSpecific) {
    static_cast<IDirect3DSurface9 *>(target.m_apiSpecific)->Release();
    target.m_apiSpecific = 0;
  }

  if (gxTex) {
    ASSERT(gxTex->m_apiSpecificData);
    IDirect3DBaseTexture9 *tex = static_cast<IDirect3DBaseTexture9 *>(gxTex->m_apiSpecificData);

    if (gxTex->m_target == GxTex_CubeMap) {
      reinterpret_cast<IDirect3DCubeTexture9 *>(tex)->GetCubeMapSurface(
          s_d3dCubeMapFaces[plane], 0, reinterpret_cast<IDirect3DSurface9 **>(&target.m_apiSpecific)
      );
    } else {
      static_cast<IDirect3DTexture9 *>(tex)->GetSurfaceLevel(0, reinterpret_cast<IDirect3DSurface9 **>(&target.m_apiSpecific));
    }
  }

  IDirect3DSurface9 *colorSurface = m_defColorSurface;
  if (m_textureTarget[GxBuffers_Color].m_apiSpecific || m_textureTarget[GxBuffers_Depth].m_apiSpecific) {
    colorSurface = static_cast<IDirect3DSurface9 *>(m_textureTarget[GxBuffers_Color].m_apiSpecific);
  }
  m_d3dDevice->SetRenderTarget(0, colorSurface);

  XformSetViewport(
      m_viewport.x.l, m_viewport.x.h, m_viewport.y.l, m_viewport.y.h, m_viewport.z.l, m_viewport.z.h
  );
}

void CGxDeviceD3d::DeviceOverride(EGxOverride override, unsigned long value) {
  CGxDevice::DeviceOverride(override, value);

  if (override == GxOverride_PixelShader) {
    ASSERT(value >= CGxPixelShader::Target_ps_1_1 && value <= CGxPixelShader::Target_ps_2_0);
    m_caps.m_pixelShaderTarget = static_cast<CGxPixelShader::Target>(value);
  }
}
