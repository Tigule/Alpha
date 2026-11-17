#include "../CGxDeviceOpenGl.h"
#include "../GlExtSupport.h"

#include <gl/gl.h>

#include <storm.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>
#include <SDL_syswm.h>
#include <X11/Xlib.h>

#define GL_FRAMEBUFFER_EXT       0x8D40
#define GL_RENDERBUFFER_EXT      0x8D41
#define GL_COLOR_ATTACHMENT0_EXT 0x8CE0
#define GL_DEPTH_ATTACHMENT_EXT  0x8D00
#define GL_DEPTH_COMPONENT24     0x81A6

static void(APIENTRY *s_glGenFramebuffersEXT)(GLsizei, GLuint *);
static void(APIENTRY *s_glDeleteFramebuffersEXT)(GLsizei, const GLuint *);
static void(APIENTRY *s_glBindFramebufferEXT)(GLenum, GLuint);
static void(APIENTRY *s_glGenRenderbuffersEXT)(GLsizei, GLuint *);
static void(APIENTRY *s_glDeleteRenderbuffersEXT)(GLsizei, const GLuint *);
static void(APIENTRY *s_glBindRenderbufferEXT)(GLenum, GLuint);
static void(APIENTRY *s_glRenderbufferStorageEXT)(GLenum, GLenum, GLsizei, GLsizei);
static void(APIENTRY *s_glFramebufferRenderbufferEXT)(GLenum, GLenum, GLenum, GLuint);

static GLuint s_renderTarget;
static GLuint s_renderTargetBuffers[2];

static BOOL APIENTRY SDLErrorHandler(DWORD errorcode, LPCSTR errorstr, LPCSTR filename, int linenumber, LPCSTR description) {
  SDL_Quit();
  return 1;
}

void CGxDeviceOpenGl::DeviceCreatePbuffer() {
  if (FindGlExt("GL_EXT_framebuffer_object")) {
    s_glGenFramebuffersEXT = (void(APIENTRY *)(GLsizei, GLuint *))SDL_GL_GetProcAddress("glGenFramebuffersEXT");
    s_glDeleteFramebuffersEXT = (void(APIENTRY *)(GLsizei, const GLuint *))SDL_GL_GetProcAddress("glDeleteFramebuffersEXT");
    s_glBindFramebufferEXT = (void(APIENTRY *)(GLenum, GLuint))SDL_GL_GetProcAddress("glBindFramebufferEXT");
    s_glGenRenderbuffersEXT = (void(APIENTRY *)(GLsizei, GLuint *))SDL_GL_GetProcAddress("glGenRenderbuffersEXT");
    s_glDeleteRenderbuffersEXT = (void(APIENTRY *)(GLsizei, const GLuint *))SDL_GL_GetProcAddress("glDeleteRenderbuffersEXT");
    s_glBindRenderbufferEXT = (void(APIENTRY *)(GLenum, GLuint))SDL_GL_GetProcAddress("glBindRenderbufferEXT");
    s_glRenderbufferStorageEXT = (void(APIENTRY *)(GLenum, GLenum, GLsizei, GLsizei))SDL_GL_GetProcAddress("glRenderbufferStorageEXT");
    s_glFramebufferRenderbufferEXT =
        (void(APIENTRY *)(GLenum, GLenum, GLenum, GLuint))SDL_GL_GetProcAddress("glFramebufferRenderbufferEXT");

    s_glGenFramebuffersEXT(1, &s_renderTarget);
    s_glGenRenderbuffersEXT(2, s_renderTargetBuffers);
    s_glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, s_renderTarget);
    s_glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, s_renderTargetBuffers[0]);
    s_glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA8, 1024, 1024);
    s_glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, s_renderTargetBuffers[0]);
    s_glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, s_renderTargetBuffers[1]);
    s_glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, 1024, 1024);
    s_glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, s_renderTargetBuffers[1]);
    s_glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
    s_glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
  }

  if (SDL_BYTESPERPIXEL(SDL_GetWindowPixelFormat((SDL_Window *)m_hwnd)) > 2) {
    m_caps.m_rttFormat[GxTex_Argb8888] = 1;
  } else {
    m_caps.m_rttFormat[GxTex_Rgb565] = 1;
  }
}

void CGxDeviceOpenGl::DeviceQueryPbuffer() {
}

void CGxDeviceOpenGl::DeviceDestroyPbuffer() {
  if (s_renderTarget) {
    s_glDeleteFramebuffersEXT(1, &s_renderTarget);
    s_glDeleteRenderbuffersEXT(2, s_renderTargetBuffers);
    s_renderTarget = 0;
  }
}

void CGxDeviceOpenGl::IDevSetFocus(int focus, const CGxFormat &format) {
  ASSERT(!"Not used");
}

BOOL CGxDeviceOpenGl::SetFormatMode(const CGxFormat &format) {
  ASSERT(!"Not used");
  return 0;
}

BOOL CGxDeviceOpenGl::IDevAttachGlContext(const CGxFormat &format) {
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
  ITexForceRecreation();
  IShaderForceRecreation();
  UnbindGlExtensions();
  m_context = 0;
}

void CGxDeviceOpenGl::DeviceWM(EGxWM wm, LONG_PTR param1, LONG_PTR param2) {
  if (wm == GxWM_Size && m_hwnd) {
    DeviceSetDefWindow(*(NTempest::CRect *)param1);
    DeviceQueryPbuffer();
  }
}

BOOL CGxDeviceOpenGl::DeviceCreate(GXWINDOWPROC windowProc, const CGxFormat &format) {
  if (SDL_Init(SDL_INIT_VIDEO) >= 0) {
    SErrRegisterHandler(SDLErrorHandler);
    SDL_GL_LoadLibrary("libGL.so.1");
    if (&m_gammaRamp != &m_systemGammaRamp) {
      m_gammaRamp = m_systemGammaRamp;
    }

    if (CGxDevice::DeviceCreate(windowProc, format)) {
      return 1;
    }
  }

  DeviceDestroy();
  return 0;
}

BOOL CGxDeviceOpenGl::DeviceCreate(UINT_PTR hwnd, const CGxFormat &format) {
  ASSERT(!"Not yet implemented");
  return 0;
}

void CGxDeviceOpenGl::DeviceDestroy() {
  CGxDevice::DeviceDestroy();
  IDevRemoveGlContext();

  if (m_hglrc) {
    SDL_GL_DeleteContext((SDL_GLContext)m_hglrc);
    m_hglrc = 0;
  }
  if (m_hwnd) {
    SDL_DestroyWindow((SDL_Window *)m_hwnd);
    m_hwnd = 0;
  }

  SDL_Quit();
}

BOOL CGxDeviceOpenGl::DeviceSetFormat(const CGxFormat &format) {
  SDL_DisplayMode desktop;
  Uint32          flags;
  char            errorString[128];

  IDevRemoveGlContext();

  if (m_hglrc) {
    SDL_GL_DeleteContext((SDL_GLContext)m_hglrc);
    m_hglrc = 0;
  }
  if (m_hwnd) {
    SDL_DestroyWindow((SDL_Window *)m_hwnd);
    m_hwnd = 0;
  }

  SDL_GetDesktopDisplayMode(0, &desktop);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, SDL_BYTESPERPIXEL(desktop.format) * 8 < 17 ? 16 : 24);

  flags = format.window ? SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE : SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN;
  m_hwnd = (HWND)SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, format.size.x, format.size.y, flags);
  if (m_hwnd) {
    m_hglrc = (HGLRC)SDL_GL_CreateContext((SDL_Window *)m_hwnd);
  }

  if (!m_hglrc) {
    SStrPrintf(
        errorString, sizeof(errorString), "Couldn't set %dx%d %s video mode: %s", format.size.x, format.size.y,
        format.window ? "windowed" : "fullscreen", SDL_GetError()
    );
    FATALERROR((errorString));
    return 0;
  }

  DeviceSetDefWindow(NTempest::CRect(0.0f, 0.0f, (float)format.size.y, (float)format.size.x));

  if (!IDevAttachGlContext(format)) {
    return 0;
  }

  DeviceSetGamma(m_gammaRamp);

  return CGxDevice::DeviceSetFormat(format);
}

void CGxDeviceOpenGl::DeviceSetBaseMipLevel(UINT baseMipLevel) {
  CGxDevice::DeviceSetBaseMipLevel(baseMipLevel);
  ITexForceRecreation();
}

void CGxDeviceOpenGl::DeviceSetGamma(float gamma) {
  Uint16 ramp[CGxGammaRamp::ENTRIES];

  SDL_CalculateGammaRamp(1.0f / gamma, ramp);
  if (SDL_SetWindowGammaRamp((SDL_Window *)m_hwnd, ramp, ramp, ramp) == 0) {
    CGxDevice::DeviceSetGamma(gamma);
  }
}

void CGxDeviceOpenGl::DeviceSetGamma(const CGxGammaRamp &ramp) {
  if (SDL_SetWindowGammaRamp((SDL_Window *)m_hwnd, ramp.red, ramp.green, ramp.blue) == 0) {
    CGxDevice::DeviceSetGamma(ramp);
  }
}

void CGxDeviceOpenGl::DeviceSetRenderTarget(EGxBuffer buffer, CGxTex *gxTex, UINT plane) {
  TextureTarget &target = m_textureTarget[buffer];
  if (target.m_texture == gxTex && target.m_plane == plane) {
    return;
  }

  CGxDevice::DeviceSetRenderTarget(buffer, gxTex, plane);
  CGxTex *oldTex = (CGxTex *)target.m_apiSpecific;
  if (oldTex) {
    BindTexture(oldTex, kNullTmu);
    if (oldTex->m_needsCreation) {
      glCopyTexImage2D(GL_TEXTURE_2D, 0, s_convertTexFmt[oldTex->m_format], 0, 0, oldTex->m_width, oldTex->m_height, 0);
    } else {
      glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, oldTex->m_width, oldTex->m_height);
    }
    oldTex->m_needsCreation = 0;
    if (s_renderTarget) {
      s_glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    }
  }

  target.m_apiSpecific = gxTex;
  if ((m_textureTarget[GxBuffers_Color].m_apiSpecific || m_textureTarget[GxBuffers_Depth].m_apiSpecific) && s_renderTarget) {
    s_glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, s_renderTarget);
  }

  XformSetViewport(m_viewport.x.l, m_viewport.x.h, m_viewport.y.l, m_viewport.y.h, m_viewport.z.l, m_viewport.z.h);
  XformSetProjection(m_projection);
}

void CGxDeviceOpenGl::DeviceSetTextureQuality(int force32) {
  CGxDevice::DeviceSetTextureQuality(force32);
  ITexForceRecreation();
}

DWORD_PTR CGxDeviceOpenGl::DeviceWindow() {
  SDL_SysWMinfo info;

  SDL_VERSION(&info.version);
  if (m_hwnd && SDL_GetWindowWMInfo((SDL_Window *)m_hwnd, &info)) {
    return (DWORD_PTR)info.info.x11.window;
  }

  return 0;
}

BOOL CGxDevice::OpenGlEnumFormats(TSGrowableArray<CGxFormat> &formats) {
  SDL_DisplayMode mode;
  int             prevWidth = 0;
  int             prevHeight = 0;

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    return 0;
  }

  for (int index = 0; index < SDL_GetNumDisplayModes(0); ++index) {
    if (SDL_GetDisplayMode(0, index, &mode) || (mode.w == prevWidth && mode.h == prevHeight)) {
      continue;
    }

    prevWidth = mode.w;
    prevHeight = mode.h;

    CGxFormat fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.size = NTempest::C2iVector(mode.w, mode.h);
    fmt.refreshRate = 0;
    fmt.apiSpecificModeID = 0;
    *formats.New() = fmt;
  }

  return formats.Count() != 0;
}

void CGxDeviceOpenGl::CapsWindowSizeInScreenCoords(NTempest::CRect &dst) {
  const NTempest::CRect &windowRect = DeviceCurWindow();

  ASSERT(windowRect.Width() * windowRect.Height() > 1.0f);

  int               left = 0;
  int               top = 0;
  int               right = (int)windowRect.r;
  int               bottom = (int)windowRect.b;
  SDL_SysWMinfo     info;
  XWindowAttributes attributes;

  SDL_VERSION(&info.version);
  if (m_hwnd && SDL_GetWindowWMInfo((SDL_Window *)m_hwnd, &info)) {
    XGetWindowAttributes(info.info.x11.display, info.info.x11.window, &attributes);
    top += attributes.y;
    bottom += attributes.y;
    left += attributes.x;
    right += attributes.x;
  }

  dst = NTempest::CRect((float)top, (float)left, (float)bottom, (float)right);
}

static int ReadPciClass(int bus, int device, int function) {
  char  path[1024];
  FILE *file;
  int   value = 0;

  sprintf(path, "/proc/bus/pci/%02x/%02x.%d", bus, device, function);
  file = fopen(path, "rb");
  if (file) {
    fseek(file, 10, SEEK_SET);
    for (int i = 1; i >= 0; --i) {
      value |= fgetc(file) << 8;
    }
    fclose(file);
  }

  return value;
}

static BOOL FindPciDisplayAdapter(WORD &vendorID, WORD &deviceID) {
  char         line[1024];
  unsigned int devfn;
  unsigned int vendorDevice;
  BOOL         found = 0;
  FILE        *file = fopen("/proc/bus/pci/devices", "r");

  if (!file) {
    return 0;
  }

  while (fgets(line, sizeof(line), file)) {
    sscanf(line, "%x %x", &devfn, &vendorDevice);
    if (ReadPciClass((devfn >> 8) & 0xFF, (devfn >> 3) & 0x1F, devfn & 7) == 0x300) {
      vendorID = vendorDevice >> 16;
      deviceID = vendorDevice & 0xFFFF;
      found = 1;
      break;
    }
  }

  fclose(file);
  return found;
}

BOOL CGxDevice::AdapterID(WORD &vendorID, WORD &deviceID, DWORD &driverVersionHi, DWORD &driverVersionLow) {
  driverVersionHi = 0;
  vendorID = 0xFFFF;
  driverVersionLow = 0;
  deviceID = 0xFFFF;
  return FindPciDisplayAdapter(vendorID, deviceID);
}

BOOL CGxDevice::AdapterInfer(WORD &deviceID) {
  WORD  vendorID;
  DWORD driverVersionHi;
  DWORD driverVersionLow;

  AdapterID(vendorID, deviceID, driverVersionHi, driverVersionLow);
  printf("Warning: Unknown video card: 0x%.4x, 0x%.4x\n", vendorID, deviceID);
  deviceID = 0;
  return 1;
}

static int CGxMonitorModeSort(const void *a, const void *b) {
  const CGxMonitorMode *modeA = (const CGxMonitorMode *)a;
  const CGxMonitorMode *modeB = (const CGxMonitorMode *)b;

  return modeA->size.x * modeA->size.y - modeB->size.x * modeB->size.y;
}

BOOL CGxDevice::AdapterMonitorModes(TSGrowableArray<CGxMonitorMode> &modes) {
  SDL_DisplayMode desktop;
  SDL_DisplayMode mode;
  int             prevWidth = 0;
  int             prevHeight = 0;

  modes.SetCount(0);

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    return 0;
  }

  SDL_GetDesktopDisplayMode(0, &desktop);

  for (int index = 0; index < SDL_GetNumDisplayModes(0); ++index) {
    if (SDL_GetDisplayMode(0, index, &mode) || (mode.w == prevWidth && mode.h == prevHeight)) {
      continue;
    }

    prevWidth = mode.w;
    prevHeight = mode.h;

    CGxMonitorMode *monitorMode = modes.New();
    monitorMode->size.x = mode.w;
    monitorMode->size.y = mode.h;
    monitorMode->bpp = SDL_BYTESPERPIXEL(desktop.format) * 8;
    monitorMode->refreshRate = 0;
  }

  qsort(modes.Ptr(), modes.Count(), sizeof(CGxMonitorMode), CGxMonitorModeSort);
  return modes.Count() != 0;
}

BOOL CGxDevice::AdapterDesktopMode(CGxMonitorMode &mode) {
  return 0;
}

BOOL CGxDevice::D3dEnumFormats(TSGrowableArray<CGxFormat> &formats) {
  return 0;
}

CGxDevice *CGxDevice::NewD3d() {
  return 0;
}
