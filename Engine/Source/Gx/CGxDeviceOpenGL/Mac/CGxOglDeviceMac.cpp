#include "CGxOglPrim_OSX.h"
#include "../GlExtSupport.h"

#include <storm.h>

#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysctl.h>

#include "GlWndSupportMac.h"
#include "Os/W32/Input.h"
#include "Os/Mac/InputMac.h"
#include "Os/W32/Debugging.h"

#include <ApplicationServices/ApplicationServices.h>
#include <OpenGL/OpenGL.h>

static GLuint           s_pbufferFramebuffer;
static GLuint           s_pbufferRenderbuffers[2];
static RECT             s_defaultWindowRect;
static RECT             s_windowedBounds;
static RECT             s_windowInset;
static void            *s_gxWindowHandler;
static int              s_fullscreenInWindow = -1;
static int              s_displayCaptured;
static CGDisplayModeRef s_desktopMode;

static SGxVxrBufferDef s_vxrBufferDefs[] = {{21, kVxrBuffer_Prim}, {21, kVxrBuffer_Dynamic}, {18, kVxrBuffer_Index}, {0, kVxrBufferCount}};

class AutoCGDisplayFade {
 public:
  float                         m_fadeInTime;
  float                         m_fadeOutTime;
  bool                          m_faded;
  CGDisplayFadeReservationToken m_token;

  AutoCGDisplayFade() : m_fadeInTime(0.5f), m_fadeOutTime(0.5f), m_faded(false), m_token(0) {
  }

  void Initialize() {
    CGError err = CGAcquireDisplayFadeReservation(15.0f, &m_token);
    if (err) {
      OsOutputDebugString("CGAcquireDisplayFadeReservation failed: err = %d", err);
      m_token = 0;
    }
  }

  void Fade() {
    if (m_token) {
      CGError err = CGDisplayFade(m_token, m_fadeOutTime, kCGDisplayBlendNormal, kCGDisplayBlendSolidColor, 0.0f, 0.0f, 0.0f, true);
      m_faded = err == kCGErrorSuccess;
      OsOutputDebugString("fade result = %d\n", err);
    }
  }

  ~AutoCGDisplayFade() {
    if (m_token) {
      if (m_faded) {
        CGDisplayFade(m_token, m_fadeInTime, kCGDisplayBlendSolidColor, kCGDisplayBlendNormal, 0.0f, 0.0f, 0.0f, false);
        m_faded = false;
      }
      CGReleaseDisplayFadeReservation(m_token);
      m_token = 0;
    }
  }
};

static const EventTypeSpec s_gxWindowEvents[] = {
    {kEventClassWindow, kEventWindowBoundsChanged},
    {kEventClassWindow, kEventWindowBoundsChanging},
    {kEventClassWindow, kEventWindowConstrain},
    {kEventClassWindow, kEventWindowClose},
    {kEventClassWindow, kEventWindowActivated},
    {kEventClassWindow, kEventWindowDeactivated},
    {kEventClassWindow, kEventWindowGetIdealSize},
    {kEventClassMouse, kEventMouseEntered},
    {kEventClassMouse, kEventMouseExited}
};

#define GL_FRAMEBUFFER_EXT          0x8D40
#define GL_RENDERBUFFER_EXT         0x8D41
#define GL_COLOR_ATTACHMENT0_EXT    0x8CE0
#define GL_DEPTH_ATTACHMENT_EXT     0x8D00
#define GL_FRAMEBUFFER_COMPLETE_EXT 0x8CD5

extern "C" {
void   glGenFramebuffersEXT(GLsizei n, GLuint *framebuffers);
void   glDeleteFramebuffersEXT(GLsizei n, const GLuint *framebuffers);
void   glBindFramebufferEXT(GLenum target, GLuint framebuffer);
void   glGenRenderbuffersEXT(GLsizei n, GLuint *renderbuffers);
void   glDeleteRenderbuffersEXT(GLsizei n, const GLuint *renderbuffers);
void   glBindRenderbufferEXT(GLenum target, GLuint renderbuffer);
void   glRenderbufferStorageEXT(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
void   glFramebufferRenderbufferEXT(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
GLenum glCheckFramebufferStatusEXT(GLenum target);
}

static void Console(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  printf("\n");
}

static float s_fontTweaks[4];

const float *GxGetFontTweaks() {
  return s_fontTweaks;
}

void GxPrepareFontTweaks() {
  LPCSTR renderer = (LPCSTR)glGetString(GL_RENDERER);
  int    vendor;

  if (SStrStr(renderer, "ATI") || SStrStr(renderer, "Rage") || SStrStr(renderer, "Radeon") || SStrStr(renderer, "RADEON")) {
    s_fontTweaks[1] = 0.125f;
    vendor = 1;
    s_fontTweaks[3] = 0.0f;
    s_fontTweaks[0] = 0.0f;
    s_fontTweaks[2] = 0.0f;
  } else if (SStrStr(renderer, "NVIDIA") || SStrStr(renderer, "GeForce")) {
    s_fontTweaks[0] = 0.0f;
    vendor = 2;
    s_fontTweaks[1] = 0.0f;
    s_fontTweaks[3] = -0.125f;
    s_fontTweaks[2] = 0.0f;
  } else {
    s_fontTweaks[1] = 0.125f;
    vendor = 0;
    s_fontTweaks[3] = 0.0f;
    s_fontTweaks[0] = 0.0f;
    s_fontTweaks[2] = 0.0f;
  }

  Console("Detected new renderer/vendor: [%s] => %d", renderer, vendor);
}

static int FullscreenInWindow() {
  if (s_fullscreenInWindow == -1) {
    DWORD value;
    s_fullscreenInWindow = 0;
    if (SRegLoadValue("WoW\\Client", "fullscreeninwindow", 0, &value)) {
      s_fullscreenInWindow = value;
    }
  }
  return s_fullscreenInWindow;
}

static int GxDevDefaultWindowRect(RECT *rect) {
  *rect = s_defaultWindowRect;
  return 1;
}

static int s_ConstrainSize(RECT &rect, const RECT &bounds) {
  int changed = 0;

  if (rect.right > rect.left + (bounds.right - bounds.left)) {
    rect.right = rect.left + (bounds.right - bounds.left);
    changed = 1;
  }
  if (rect.bottom > rect.top + (bounds.bottom - bounds.top)) {
    rect.bottom = rect.top + (bounds.bottom - bounds.top);
    changed = 1;
  }

  int width = rect.right - rect.left;
  int height = rect.bottom - rect.top;
  int newWidth = width;
  int newHeight = height;

  if (4 * height < 3 * width) {
    newWidth = 4 * height / 3;
    changed = 1;
  } else if (4 * height > 3 * width) {
    newHeight = 3 * width / 4;
    changed = 1;
  }

  if (width < 800 || height < 600) {
    newWidth = 800;
    newHeight = 600;
    changed = 1;
  }

  if (changed) {
    rect.right = rect.left + newWidth;
    rect.bottom = rect.top + newHeight;
  }

  return changed;
}

static int s_ConstrainOrigin(RECT &rect, const RECT &bounds) {
  RECT constrained = rect;
  int  changed = 0;

  if (rect.right - rect.left > bounds.right - bounds.left || rect.bottom - rect.top > bounds.bottom - bounds.top) {
    constrained = bounds;
    s_ConstrainSize(constrained, bounds);
    changed = 1;
  } else {
    if (rect.right > bounds.right) {
      constrained.left = rect.left - (rect.right - bounds.right);
      constrained.right = constrained.left + (rect.right - rect.left);
      changed = 1;
    }
    if (rect.left < bounds.left) {
      constrained.left = bounds.left;
      constrained.right = bounds.left + (rect.right - rect.left);
      changed = 1;
    }
    if (rect.bottom > bounds.bottom) {
      constrained.top = rect.top - (rect.bottom - bounds.bottom);
      constrained.bottom = constrained.top + (rect.bottom - rect.top);
      changed = 1;
    }
    if (rect.top < bounds.top) {
      constrained.top = bounds.top;
      constrained.bottom = bounds.top + (rect.bottom - rect.top);
      changed = 1;
    }
  }

  if (changed) {
    rect = constrained;
  }

  return changed;
}

static void InstallGxWindowHandlers(LPVOID window, CGxDeviceOpenGl *device) {
  RECT structure;
  RECT content;

  GxMacSetWindowResizeLimits(window, 800, 600);
  GxMacGetWindowBounds(window, 1, &structure.left, &structure.top, &structure.right, &structure.bottom);
  GxMacGetWindowBounds(window, 0, &content.left, &content.top, &content.right, &content.bottom);
  s_windowInset.left = content.left - structure.left;
  s_windowInset.top = content.top - structure.top;
  s_windowInset.right = content.right - structure.right;
  s_windowInset.bottom = content.bottom - structure.bottom;
  OsMacInstallEventHandler(
      window, CGxDeviceOpenGl::GxWindowEventHandler, sizeof(s_gxWindowEvents) / sizeof(s_gxWindowEvents[0]), s_gxWindowEvents, device,
      &s_gxWindowHandler
  );
}

int CGxDeviceOpenGl::GxWindowEventHandler(OSMACEVENT *event, LPVOID userData) {
  CGxDeviceOpenGl *device = (CGxDeviceOpenGl *)userData;
  OSStatus         result = eventNotHandledErr;
  RECT             window;
  RECT             bounds;
  double           scale;

  if (!device->m_format.window) {
    return noErr;
  }

  switch (event->eventClass) {
    case kEventClassWindow:
      switch (event->eventKind) {
        case kEventWindowBoundsChanged:
          GxMacGetWindowBounds((LPVOID)device->m_hwnd, 0, &window.left, &window.top, &window.right, &window.bottom);
          device->m_contextUpdatePending = 1;
          bounds = window;
          s_ConstrainSize(window, bounds);
          scale = GxMacWindowBackingScale((LPVOID)device->m_hwnd);
          s_defaultWindowRect.left = 0;
          s_defaultWindowRect.top = 0;
          s_defaultWindowRect.right = (int)((window.right - window.left) * scale);
          s_defaultWindowRect.bottom = (int)((window.bottom - window.top) * scale);
          device->DeviceSetDefWindow(NTempest::CRect(0.0f, 0.0f, (float)s_defaultWindowRect.bottom, (float)s_defaultWindowRect.right));
          OsInputPostEvent(OS_INPUT_SIZE, window.right - window.left, window.bottom - window.top, 0, 0);
          break;

        case kEventWindowClose:
          OsInputPostEvent(OS_INPUT_CLOSE, 0, 0, 0, 0);
          result = noErr;
          break;
      }
      break;

    case kEventClassMouse:
      switch (event->eventKind) {
        case kEventMouseEntered:
          GxMacHideCursor();
          break;

        case kEventMouseExited:
          GxMacInitCursor();
          break;
      }
      break;
  }

  return result;
}

void CGxDeviceOpenGl::DeviceSetupVXR() {
  if (glExtVertexArrayRangeAPPLE && glExtFenceAPPLE) {
    int success = 0;
    m_vxrManager = new CGxVxrManager(s_vxrBufferDefs, &success);
    if (!success) {
      delete m_vxrManager;
      m_vxrManager = 0;
      OsOutputDebugString(" error activating VXR support");
    } else {
      OsOutputDebugString(" activated VXR support");
    }
  } else {
    OsOutputDebugString(" deactivated VXR support (1)");
  }

  if (glExtVertexArrayObjectAPPLE) {
    m_vaoManager = new CGxVaoManager;
    OsOutputDebugString(" activated VAO support");
  } else {
    OsOutputDebugString(" error activating VAO support");
  }
}

void CGxDeviceOpenGl::HandleModeChanges() {
  RECT   window;
  RECT   bounds;
  double scale;

  if (m_modeChangePending && FullscreenInWindow()) {
    if (!m_format.window) {
      GxMacGetWindowBounds((LPVOID)m_hwnd, 0, &s_windowedBounds.left, &s_windowedBounds.top, &s_windowedBounds.right, &s_windowedBounds.bottom);
      GxMacGetScreenBounds(0, &window.left, &window.top, &window.right, &window.bottom);
      GxMacSetSystemUIMode(1);
      GxMacHideCursor();
      GxMacWindowSetFullScreenStyle((LPVOID)m_hwnd, 1);
      GxMacSetWindowBounds((LPVOID)m_hwnd, window.left, window.top, window.right, window.bottom);
      bounds = window;
      s_ConstrainSize(window, bounds);
    } else {
      GxMacSetSystemUIMode(0);
      GxMacInitCursor();
      GxMacWindowSetFullScreenStyle((LPVOID)m_hwnd, 0);

      if (s_windowedBounds.right > s_windowedBounds.left && s_windowedBounds.bottom > s_windowedBounds.top) {
        window = s_windowedBounds;
      } else {
        GxMacGetWindowBounds((LPVOID)m_hwnd, 0, &window.left, &window.top, &window.right, &window.bottom);
      }

      GxMacGetScreenBounds(1, &bounds.left, &bounds.top, &bounds.right, &bounds.bottom);
      bounds.left += s_windowInset.left;
      bounds.top += s_windowInset.top;
      bounds.right += s_windowInset.right;
      bounds.bottom += s_windowInset.bottom;
      s_ConstrainOrigin(window, bounds);
      GxMacSetWindowBounds((LPVOID)m_hwnd, window.left, window.top, window.right, window.bottom);
    }

    scale = GxMacWindowBackingScale((LPVOID)m_hwnd);
    s_defaultWindowRect.left = 0;
    s_defaultWindowRect.top = 0;
    s_defaultWindowRect.right = (int)((window.right - window.left) * scale);
    s_defaultWindowRect.bottom = (int)((window.bottom - window.top) * scale);
    DeviceSetDefWindow(NTempest::CRect(0.0f, 0.0f, (float)s_defaultWindowRect.bottom, (float)s_defaultWindowRect.right));
    OsInputPostEvent(OS_INPUT_SIZE, window.right - window.left, window.bottom - window.top, 0, 0);

    m_modeChangePending = 0;
    m_contextUpdatePending = 1;
  } else if (m_modeChangePending) {
    OsInputPostEvent(
        OS_INPUT_SIZE, (int)((s_defaultWindowRect.right - s_defaultWindowRect.left) / GxMacWindowBackingScale((LPVOID)m_hwnd)),
        (int)((s_defaultWindowRect.bottom - s_defaultWindowRect.top) / GxMacWindowBackingScale((LPVOID)m_hwnd)), 0, 0
    );
    m_modeChangePending = 0;
  }

  if (m_contextUpdatePending) {
    const NTempest::CRect &defWindow = DeviceDefWindow();
    int                    width;
    int                    height;

    scale = GxMacWindowBackingScale((LPVOID)m_hwnd);
    width = (int)((defWindow.r - defWindow.l) / scale);
    height = (int)((defWindow.b - defWindow.t) / scale);
    GxMacGetWindowBounds((LPVOID)m_hwnd, 0, &window.left, &window.top, &window.right, &window.bottom);
    GxMacSetBufferRect(
        (LPVOID)m_hwnd, window.right - window.left > width ? (window.right - window.left - width) / 2 : 0,
        window.bottom - window.top > height ? (window.bottom - window.top - height) / 2 : 0, width, height
    );
    GxMacContextUpdate((LPVOID)m_hglrc);
    m_contextUpdatePending = 0;
  }
}

void CGxDeviceOpenGl::DeviceCreatePbuffer() {
  ASSERT(m_renderTargetType != kRenderTargetType_None);

  m_hPbuffer = 0;
  m_hPbufferDC = 0;
  m_hPbufferRC = 0;

  if (m_renderTargetType != kRenderTargetType_PBuffer) {
    return;
  }

  glGenFramebuffersEXT(1, &s_pbufferFramebuffer);
  glGenRenderbuffersEXT(2, s_pbufferRenderbuffers);
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, s_pbufferFramebuffer);
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, s_pbufferRenderbuffers[0]);
  glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA8, m_caps.m_maxTextureSize, m_caps.m_maxTextureSize);
  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, s_pbufferRenderbuffers[0]);
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, s_pbufferRenderbuffers[1]);
  glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, m_caps.m_maxTextureSize, m_caps.m_maxTextureSize);
  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, s_pbufferRenderbuffers[1]);
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);

  int created = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) == GL_FRAMEBUFFER_COMPLETE_EXT;
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

  ASSERT(created);
}

void CGxDeviceOpenGl::DeviceQueryPbuffer() {
}

void CGxDeviceOpenGl::DeviceDestroyPbuffer() {
  if (s_pbufferFramebuffer) {
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    glDeleteFramebuffersEXT(1, &s_pbufferFramebuffer);
    glDeleteRenderbuffersEXT(2, s_pbufferRenderbuffers);
    s_pbufferFramebuffer = 0;
    s_pbufferRenderbuffers[0] = 0;
    s_pbufferRenderbuffers[1] = 0;
  }

  m_hPbuffer = 0;
  m_hPbufferDC = 0;
  m_hPbufferRC = 0;
}

void CGxDeviceOpenGl::IDevSetFocus(int focus, const CGxFormat &format) {
  if (!m_hwnd) {
    return;
  }

  if (focus) {
    if (!format.window && !FullscreenInWindow() && !s_displayCaptured) {
      CGDisplayCapture(kCGDirectMainDisplay);
      s_desktopMode = CGDisplayCopyDisplayMode(kCGDirectMainDisplay);

      CFArrayRef modes = CGDisplayCopyAllDisplayModes(kCGDirectMainDisplay, 0);
      if (modes) {
        if ((CFIndex)format.apiSpecificModeID < CFArrayGetCount(modes)) {
          CGDisplaySetDisplayMode(kCGDirectMainDisplay, (CGDisplayModeRef)CFArrayGetValueAtIndex(modes, format.apiSpecificModeID), 0);
        }
        CFRelease(modes);
      }

      s_displayCaptured = 1;
    }
  } else if (s_displayCaptured) {
    if (s_desktopMode) {
      CGDisplaySetDisplayMode(kCGDirectMainDisplay, s_desktopMode, 0);
      CGDisplayModeRelease(s_desktopMode);
      s_desktopMode = 0;
    }

    CGDisplayRelease(kCGDirectMainDisplay);
    CGDisplayRestoreColorSyncSettings();
    s_displayCaptured = 0;
  }

  GxMacWindowShow((LPVOID)m_hwnd, format.window || FullscreenInWindow(), focus);
}

BOOL CGxDeviceOpenGl::SetFormatMode(const CGxFormat &format) {
  if (format.window) {
    format.apiSpecificModeID = 0;
    return 1;
  }

  CFArrayRef modes = CGDisplayCopyAllDisplayModes(kCGDirectMainDisplay, 0);
  CFIndex    count;
  CFIndex    index;

  s_fullscreenInWindow = -1;

  if (!modes) {
    return 0;
  }

  count = CFArrayGetCount(modes);

  for (index = 0; index < count; ++index) {
    CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(modes, index);

    if ((int)CGDisplayModeGetWidth(mode) == format.size.x && (int)CGDisplayModeGetHeight(mode) == format.size.y) {
      format.apiSpecificModeID = (DWORD)index;
      CFRelease(modes);
      return 1;
    }
  }

  CFRelease(modes);
  s_fullscreenInWindow = 1;
  format.apiSpecificModeID = 0;
  return 1;
}

BOOL CGxDeviceOpenGl::IDevAttachGlContext(const CGxFormat &format) {
  FATALASSERT(m_hdc == 0 && m_hglrc == 0);

  LPVOID            view;
  LPVOID            context;
  CGEventRef        mouseEvent;
  CGPoint           mouse;
  RECT              window;
  int               captured = !format.window && !FullscreenInWindow();
  AutoCGDisplayFade fade;

  m_modeChangePending = 0;
  m_contextUpdatePending = 0;
  fade.Initialize();

  if (captured) {
    fade.Fade();
  }

  if (m_ownhwnd) {
    m_hwnd = (HWND)GxMacWindowCreate(format.pos.x, format.pos.y, format.size.x, format.size.y, !captured);

    if (!m_hwnd) {
      return 0;
    }

    IDevSetFocus(1, format);
  }

  view = GxMacWindowContentView((LPVOID)m_hwnd);

  if (!view) {
    IDevRemoveGlContext();
    return 0;
  }

  context = GxMacContextCreate(view);

  if (!context) {
    IDevRemoveGlContext();
    return 0;
  }

  GxMacGetWindowBounds((LPVOID)m_hwnd, 0, &window.left, &window.top, &window.right, &window.bottom);

  mouseEvent = CGEventCreate(0);
  mouse = CGEventGetLocation(mouseEvent);
  CFRelease(mouseEvent);

  if (captured || (mouse.x >= window.left && mouse.x < window.right && mouse.y >= window.top && mouse.y < window.bottom)) {
    GxMacHideCursor();
  }

  if (!captured) {
    InstallGxWindowHandlers((LPVOID)m_hwnd, this);
    OsInputInstallGameWindowHandlers((LPVOID)m_hwnd);
  } else {
    OsInputInstallGameWindowHandlers(0);
  }

  m_hdc = (HDC)view;
  m_hglrc = (HGLRC)context;

  GxMacContextMakeCurrent(context);

  s_defaultWindowRect.left = 0;
  s_defaultWindowRect.top = 0;
  s_defaultWindowRect.right = (int)((window.right - window.left) * GxMacWindowBackingScale((LPVOID)m_hwnd));
  s_defaultWindowRect.bottom = (int)((window.bottom - window.top) * GxMacWindowBackingScale((LPVOID)m_hwnd));

  DeviceSetDefWindow(NTempest::CRect(0.0f, 0.0f, (float)s_defaultWindowRect.bottom, (float)s_defaultWindowRect.right));
  SetOsGetDefaultWindowRectCB(GxDevDefaultWindowRect);

  BindGlExtensions();
  m_renderTargetType = kRenderTargetType_None;
  m_caps.m_rttFormat[GxTex_Argb8888] = 1;
  m_caps.m_rttOriginUpperLeft = 0;
  ISetGlCaps();
  IStateSetContextDefaults();
  IRsSync(1);
  ISceneBegin(3);
  GxPrepareFontTweaks();
  DeviceSetupVXR();
  m_context = 1;
  m_modeChangePending = 1;
  HandleModeChanges();
  return 1;
}

void CGxDeviceOpenGl::IDevRemoveGlContext() {
  AutoCGDisplayFade fade;

  Console("Scrubbing GxBufs...");
  ITERATELIST(CGxBuf, m_bufList, buf) {
    buf->Invalidate(CGxBuf::S_INVALID_DISCARD, CGxBuf::S_INVALID_DISCARD);
    ((CGxBufOgl *)buf)->m_vaoToken = -1;
  }

  ITexForceRecreation();
  IShaderForceRecreation();
  UnbindGlExtensions();

  if (m_hglrc) {
    switch (m_renderTargetType) {
      case kRenderTargetType_None:
        break;

      case kRenderTargetType_SurfaceTexture:
      case kRenderTargetType_PBuffer:
        DeviceDestroyPbuffer();
        break;

      default:
        SErrDisplayErrorFmt(
            STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
            isprint((m_renderTargetType >> 24) & 0xFF) && isprint((m_renderTargetType >> 16) & 0xFF) && isprint((m_renderTargetType >> 8) & 0xFF) &&
                    isprint(m_renderTargetType & 0xFF)
                ? "\"%s\", %s = %ld (0x%08X, '%c%c%c%c')"
                : "\"%s\", %s = %ld (0x%08X)",
            "\"Invalid Render Target Type\"", "m_renderTargetType", m_renderTargetType, m_renderTargetType, (m_renderTargetType >> 24) & 0xFF,
            (m_renderTargetType >> 16) & 0xFF, (m_renderTargetType >> 8) & 0xFF, m_renderTargetType & 0xFF
        );
        break;
    }
    m_renderTargetType = kRenderTargetType_None;

    delete m_vaoManager;
    m_vaoManager = 0;
    if (m_vxrManager) {
      glDisableClientState(GL_VERTEX_ARRAY_RANGE_APPLE);
      delete m_vxrManager;
    }
    m_vxrManager = 0;

    GxMacContextClearCurrent();
    GxMacContextDestroy((LPVOID)m_hglrc);

    m_hglrc = 0;
  }

  fade.Initialize();
  if (s_displayCaptured) {
    fade.Fade();
  }

  m_hdc = 0;

  if (s_gxWindowHandler) {
    OsMacRemoveEventHandler(s_gxWindowHandler);
    s_gxWindowHandler = 0;
  }

  OsInputRemoveGameWindowHandlers((LPVOID)m_hwnd);

  if (m_hwnd && m_ownhwnd) {
    IDevSetFocus(0, m_format);
    GxMacSetSystemUIMode(0);
    GxMacWindowDestroy((LPVOID)m_hwnd);
    m_hwnd = 0;
  }

#if BUILD_FIXES
  GxMacInitCursor();
#endif
  m_context = 0;
}

void CGxDeviceOpenGl::DeviceWM(EGxWM wm, LONG_PTR param1, LONG_PTR param2) {
  ASSERT(0);
}

BOOL CGxDeviceOpenGl::DeviceCreate(GXWINDOWPROC windowProc, const CGxFormat &format) {
  VALIDATEBEGIN;
  VALIDATE(m_hdc == 0);
  VALIDATE(m_hglrc == 0);
  VALIDATEEND;

  m_ownhwnd = 1;
  m_vxrManager = 0;
  m_vaoManager = 0;
  m_renderTargetType = kRenderTargetType_None;
  m_modeChangePending = 0;
  m_contextUpdatePending = 0;

  if (CGxDevice::DeviceCreate(windowProc, format)) {
    CreateDynamicBufs();
    return 1;
  }

  DeviceDestroy();
  return 0;
}

BOOL CGxDeviceOpenGl::DeviceCreate(UINT_PTR hwnd, const CGxFormat &format) {
  ASSERT(!"CGxDeviceOpenGl::DeviceCreate (uint /*hwnd*/, const CGxFormat& /*format*/)");
  return 0;
}

void CGxDeviceOpenGl::DeviceDestroy() {
  CGxDevice::DeviceDestroy();
  IDevRemoveGlContext();
}

BOOL CGxDeviceOpenGl::DeviceSetFormat(const CGxFormat &format) {
  Log("CGxDeviceOpenGl::DeviceSetFormat():");
  Log(format);

  CGxFormat prevFormat = m_format;

  if (CGxDevice::DeviceSetFormat(format)) {
    IDevRemoveGlContext();

    if (SetFormatMode(format) && IDevAttachGlContext(format)) {
      DeviceSetGamma(m_gammaRamp);

      if (CGxDevice::DeviceSetFormat(format)) {
        return 1;
      }
    }
  }

  CGxDevice::DeviceSetFormat(prevFormat);
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

void CGxDeviceOpenGl::IDeviceSetRenderTargetSurfaceTexture(EGxBuffer buffer, CGxTex *gxTex, UINT plane) {
  TextureTarget &target = m_textureTarget[buffer];

  if (target.m_texture == gxTex && target.m_plane == plane) {
    return;
  }

  CGxDevice::DeviceSetRenderTarget(buffer, gxTex, plane);

  CGxTex *oldTex = (CGxTex *)target.m_apiSpecific;
  if (oldTex) {
    BindTexture(oldTex, (UINT)-1);

    if (oldTex->m_needsCreation) {
      glCopyTexImage2D(GL_TEXTURE_2D, 0, s_convertTexFmt[oldTex->m_format], 0, 0, oldTex->m_width, oldTex->m_height, 0);
    } else {
      glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, oldTex->m_width, oldTex->m_height);
    }

    oldTex->m_needsCreation = 0;
    target.m_apiSpecific = 0;
  }

  if (gxTex) {
    target.m_apiSpecific = gxTex;
  }

  XformSetViewport(m_viewport.x.l, m_viewport.x.h, m_viewport.y.l, m_viewport.y.h, m_viewport.z.l, m_viewport.z.h);
  XformSetProjection(m_projection);
}

void CGxDeviceOpenGl::IDeviceSetRenderTargetPBuffer(EGxBuffer buffer, CGxTex *gxTex, UINT plane) {
  TextureTarget &target = m_textureTarget[buffer];

  if (target.m_texture == gxTex && target.m_plane == plane) {
    return;
  }

  CGxDevice::DeviceSetRenderTarget(buffer, gxTex, plane);

  CGxTex *oldTex = (CGxTex *)target.m_apiSpecific;
  if (oldTex) {
    BindTexture(oldTex, (UINT)-1);

    if (oldTex->m_needsCreation) {
      glCopyTexImage2D(GL_TEXTURE_2D, 0, s_convertTexFmt[oldTex->m_format], 0, 0, oldTex->m_width, oldTex->m_height, 0);
    } else {
      glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, oldTex->m_width, oldTex->m_height);
    }

    oldTex->m_needsCreation = 0;
    target.m_apiSpecific = 0;
  }

  if (gxTex) {
    target.m_apiSpecific = gxTex;
  }

  if (m_textureTarget[GxBuffers_Color].m_apiSpecific || m_textureTarget[GxBuffers_Depth].m_apiSpecific) {
    glFlush();
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, s_pbufferFramebuffer);
  } else {
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
  }

  XformSetViewport(m_viewport.x.l, m_viewport.x.h, m_viewport.y.l, m_viewport.y.h, m_viewport.z.l, m_viewport.z.h);
  XformSetProjection(m_projection);
}

void CGxDeviceOpenGl::DeviceSetRenderTarget(EGxBuffer buffer, CGxTex *gxTex, UINT plane) {
  if (m_renderTargetType == kRenderTargetType_None) {
    m_renderTargetType = agl_HasPBufferSupport ? kRenderTargetType_PBuffer : kRenderTargetType_SurfaceTexture;
    OsOutputDebugString("Creating Render Target, type = %s\n", m_renderTargetType == kRenderTargetType_PBuffer ? "pbuffer" : "surface texture");
    DeviceCreatePbuffer();
  }

  switch (m_renderTargetType) {
    case kRenderTargetType_PBuffer:
      IDeviceSetRenderTargetPBuffer(buffer, gxTex, plane);
      break;

    case kRenderTargetType_SurfaceTexture:
      IDeviceSetRenderTargetSurfaceTexture(buffer, gxTex, plane);
      break;

    default:
      SErrDisplayErrorFmt(
          STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
          isprint((m_renderTargetType >> 24) & 0xFF) && isprint((m_renderTargetType >> 16) & 0xFF) && isprint((m_renderTargetType >> 8) & 0xFF) &&
                  isprint(m_renderTargetType & 0xFF)
              ? "\"%s\", %s = %ld (0x%08X, '%c%c%c%c')"
              : "\"%s\", %s = %ld (0x%08X)",
          "\"Invalid Render Target Type\"", "m_renderTargetType", m_renderTargetType, m_renderTargetType, (m_renderTargetType >> 24) & 0xFF,
          (m_renderTargetType >> 16) & 0xFF, (m_renderTargetType >> 8) & 0xFF, m_renderTargetType & 0xFF
      );
      break;
  }
}

void CGxDeviceOpenGl::DeviceSetTextureQuality(int force32) {
  CGxDevice::DeviceSetTextureQuality(force32);
  ITexForceRecreation();
}

DWORD_PTR CGxDeviceOpenGl::DeviceWindow() {
  return (DWORD_PTR)m_hwnd;
}

void CGxDeviceOpenGl::CapsWindowSizeInScreenCoords(NTempest::CRect &dst) {
  if (s_displayCaptured) {
    CGRect display = CGDisplayBounds(kCGDirectMainDisplay);
    dst.t = (float)CGRectGetMinY(display);
    dst.l = (float)CGRectGetMinX(display);
    dst.b = (float)CGRectGetMaxY(display);
    dst.r = (float)CGRectGetMaxX(display);
  } else if (m_hwnd) {
    RECT window;
    GxMacGetWindowBounds((LPVOID)m_hwnd, 0, &window.left, &window.top, &window.right, &window.bottom);
    dst.t = (float)window.top;
    dst.l = (float)window.left;
    dst.b = (float)window.bottom;
    dst.r = (float)window.right;
  } else {
    dst.t = 0.0f;
    dst.l = 0.0f;
    dst.b = 0.0f;
    dst.r = 0.0f;
  }
}

BOOL CGxDevice::OpenGlEnumFormats(TSGrowableArray<CGxFormat> &formats) {
  return 0;
}

BOOL CGxDevice::D3dEnumFormats(TSGrowableArray<CGxFormat> &formats) {
  return 0;
}

CGxDevice *CGxDevice::NewD3d() {
  return 0;
}

BOOL CGxDevice::AdapterID(WORD &vendorID, WORD &deviceID, UINT &driverVersionHi, UINT &driverVersionLow) {
  vendorID = 0;
  deviceID = 0;
  driverVersionHi = 0;
  driverVersionLow = 0;
  return 1;
}

static uint64_t GetTotalVRAM() {
  CGLRendererInfoObj info;
  GLint              count = 0;
  GLint              videoMemory = 0;
  uint64_t           screenBytes = (uint64_t)CGDisplayPixelsWide(kCGDirectMainDisplay) * CGDisplayPixelsHigh(kCGDirectMainDisplay) * 4;

  if (CGLQueryRendererInfo(CGDisplayIDToOpenGLDisplayMask(kCGDirectMainDisplay), &info, &count) == kCGLNoError) {
    for (GLint i = 0; i < count; ++i) {
      GLint accelerated = 0;
      GLint megabytes = 0;
      CGLDescribeRenderer(info, i, kCGLRPAccelerated, &accelerated);
      CGLDescribeRenderer(info, i, kCGLRPVideoMemoryMegabytes, &megabytes);
      if (accelerated && videoMemory < megabytes) {
        videoMemory = megabytes;
      }
    }
    CGLDestroyRendererInfo(info);
  }

  return screenBytes + ((uint64_t)videoMemory << 20);
}

BOOL CGxDevice::AdapterInfer(WORD &deviceID) {
  uint64_t vram = GetTotalVRAM();
  uint64_t cpuFrequency = 0;
  size_t   length = sizeof(cpuFrequency);

  sysctlbyname("hw.cpufrequency", &cpuFrequency, &length, 0, 0);

  if (vram <= 0x1E00000) {
    OsOutputDebugString("SYSTEM DETECTION: VRAM is %dMB (< 32MB) -- using 'low'\n", (int)(vram >> 20));
    deviceID = 0;
  } else if (vram <= 0x3E00000) {
    OsOutputDebugString("SYSTEM DETECTION: VRAM is %dMB (< 64MB) -- using 'medium'\n", (int)(vram >> 20));
    deviceID = 1;
  } else if (cpuFrequency <= 1480000000) {
    OsOutputDebugString("SYSTEM DETECTION: CPU is %dMhz (< 1500MHz) -- using 'medium'\n", (int)(cpuFrequency / 1000000));
    deviceID = 1;
  } else {
    OsOutputDebugString("SYSTEM DETECTION: Your system rocks! -- using 'high'\n");
    deviceID = 2;
  }

  return 1;
}

static int CGxMonitorModeSort(const void *a, const void *b) {
  const CGxMonitorMode *modeA = (const CGxMonitorMode *)a;
  const CGxMonitorMode *modeB = (const CGxMonitorMode *)b;

  return modeA->size.x * modeA->size.y - modeB->size.x * modeB->size.y;
}

BOOL CGxDevice::AdapterMonitorModes(TSGrowableArray<CGxMonitorMode> &modes) {
  CGDirectDisplayID displays[10];
  CGDisplayCount    displayCount;

  modes.SetCount(0);

  if (CGGetActiveDisplayList(10, displays, &displayCount) == kCGErrorSuccess) {
    for (CGDisplayCount display = 0; display < displayCount; ++display) {
      CFArrayRef displayModes = CGDisplayCopyAllDisplayModes(displays[display], 0);

      if (!displayModes) {
        continue;
      }

      CFIndex count = CFArrayGetCount(displayModes);

      for (CFIndex index = 0; index < count; ++index) {
        CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(displayModes, index);
        CGxMonitorMode   monitorMode;

        monitorMode.size.x = (int)CGDisplayModeGetWidth(mode);
        monitorMode.size.y = (int)CGDisplayModeGetHeight(mode);
        monitorMode.bpp = 32;
        monitorMode.refreshRate = (UINT)CGDisplayModeGetRefreshRate(mode);
        if (!monitorMode.refreshRate) {
          monitorMode.refreshRate = 60;
        }

        if (monitorMode.size.x >= 640 && monitorMode.size.y >= 480 && monitorMode.bpp >= 16 && monitorMode.refreshRate >= 60) {
          modes.Add(1, &monitorMode);
        }
      }

      CFRelease(displayModes);
    }
  }

  qsort(modes.Ptr(), modes.Count(), sizeof(CGxMonitorMode), CGxMonitorModeSort);
  return modes.Count() != 0;
}

BOOL CGxDevice::AdapterDesktopMode(CGxMonitorMode &mode) {
  CGDisplayModeRef displayMode = CGDisplayCopyDisplayMode(kCGDirectMainDisplay);

  if (!displayMode) {
    return 0;
  }

  mode.size.x = (int)CGDisplayModeGetWidth(displayMode);
  mode.size.y = (int)CGDisplayModeGetHeight(displayMode);
  mode.bpp = 32;
  mode.refreshRate = (UINT)CGDisplayModeGetRefreshRate(displayMode);
  if (!mode.refreshRate) {
    mode.refreshRate = 60;
  }

  CGDisplayModeRelease(displayMode);
  return 1;
}
