#include "GlWndSupportMac.h"

#import <AppKit/AppKit.h>

void *GxMacWindowCreate(int x, int y, int width, int height, int windowed) {
  NSRect     frame = NSMakeRect(x, y, width, height);
  NSUInteger style = windowed ? (NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable)
                              : NSWindowStyleMaskBorderless;
  NSWindow  *window;
  NSView    *view;

  window = [[NSWindow alloc] initWithContentRect:frame styleMask:style backing:NSBackingStoreBuffered defer:NO];
  [window setTitle:@"A game in progress"];
  [window setReleasedWhenClosed:NO];

  // without this cocoa withholds mouse movement unless a button is held
  [window setAcceptsMouseMovedEvents:YES];

  view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, width, height)];
  [view setWantsBestResolutionOpenGLSurface:NO];
  [window setContentView:view];

  if (!windowed) {
    [window setLevel:NSMainMenuWindowLevel + 1];
    [window setFrame:[[NSScreen mainScreen] frame] display:NO];
  }

  return (__bridge_retained void *)window;
}

void GxMacWindowDestroy(void *window) {
  NSWindow *nsWindow = (__bridge_transfer NSWindow *)window;

  [nsWindow orderOut:nil];
}

void GxMacWindowShow(void *window, int windowed, int focus) {
  NSWindow *nsWindow = (__bridge NSWindow *)window;

  if (!nsWindow) {
    return;
  }

  if (windowed) {
    if (focus) {
      [nsWindow makeKeyAndOrderFront:nil];
    }
  } else if (focus) {
    [nsWindow setFrame:[[NSScreen mainScreen] frame] display:YES];
    [nsWindow makeKeyAndOrderFront:nil];
  } else {
    [nsWindow miniaturize:nil];
  }
}

void *GxMacWindowContentView(void *window) {
  NSWindow *nsWindow = (__bridge NSWindow *)window;

  return (__bridge void *)[nsWindow contentView];
}

void GxMacWindowContentRectInScreen(void *window, double width, double height, double *l, double *t, double *r, double *b) {
  NSWindow *nsWindow = (__bridge NSWindow *)window;
  NSRect    content = NSMakeRect(0, 0, width, height);
  NSRect    screen = nsWindow ? [nsWindow convertRectToScreen:content] : content;

  *l = NSMinX(screen);
  *t = NSMinY(screen);
  *r = NSMaxX(screen);
  *b = NSMaxY(screen);
}

void *GxMacContextCreate(void *view, int colorBits, int depthBits, int stencilBits, int vsync) {
  NSOpenGLPixelFormatAttribute attributes[16];
  unsigned int                 count = 0;
  NSOpenGLPixelFormat         *pixelFormat;
  NSOpenGLContext             *context;
  GLint                        swapInterval = vsync != 0;

  attributes[count++] = NSOpenGLPFADoubleBuffer;
  attributes[count++] = NSOpenGLPFAAccelerated;
  attributes[count++] = NSOpenGLPFAColorSize;
  attributes[count++] = colorBits;
  attributes[count++] = NSOpenGLPFADepthSize;
  attributes[count++] = depthBits;
  attributes[count++] = NSOpenGLPFAStencilSize;
  attributes[count++] = stencilBits;
  attributes[count++] = 0;

  pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
  if (!pixelFormat) {
    return 0;
  }

  context = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
  if (!context) {
    return 0;
  }

  [context setView:(__bridge NSView *)view];
  [context setValues:&swapInterval forParameter:NSOpenGLContextParameterSwapInterval];

  return (__bridge_retained void *)context;
}

void GxMacContextDestroy(void *context) {
  NSOpenGLContext *nsContext = (__bridge_transfer NSOpenGLContext *)context;

  [nsContext clearDrawable];
}

void GxMacContextMakeCurrent(void *context) {
  [(__bridge NSOpenGLContext *)context makeCurrentContext];
}

void GxMacContextClearCurrent() {
  [NSOpenGLContext clearCurrentContext];
}
