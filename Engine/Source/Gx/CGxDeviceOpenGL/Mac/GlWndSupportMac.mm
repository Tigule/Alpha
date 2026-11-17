#include "GlWndSupportMac.h"

#import <AppKit/AppKit.h>

static BOOL s_cursorHidden;

void *GxMacWindowCreate(int x, int y, int width, int height, int windowed) {
  NSRect     frame = NSMakeRect(x, y, width, height);
  NSUInteger style = windowed ? (NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable)
                              : NSWindowStyleMaskBorderless;
  NSWindow  *window;
  NSView    *view;
  static BOOL s_liveResizeObserved;

  if (!s_liveResizeObserved) {
    [[NSNotificationCenter defaultCenter] addObserverForName:NSWindowWillStartLiveResizeNotification
                                                      object:nil
                                                       queue:nil
                                                  usingBlock:^(NSNotification *note) {
                                                    [[[[note.object contentView] subviews] firstObject] setHidden:YES];
                                                  }];
    [[NSNotificationCenter defaultCenter] addObserverForName:NSWindowDidEndLiveResizeNotification
                                                      object:nil
                                                       queue:nil
                                                  usingBlock:^(NSNotification *note) {
                                                    [[[[note.object contentView] subviews] firstObject] setHidden:NO];
                                                  }];
    s_liveResizeObserved = YES;
  }

  window = [[NSWindow alloc] initWithContentRect:frame styleMask:style backing:NSBackingStoreBuffered defer:NO];
  [window setReleasedWhenClosed:NO];
  [window setAcceptsMouseMovedEvents:YES];
  [window setBackgroundColor:[NSColor blackColor]];
  [window setCollectionBehavior:windowed ? NSWindowCollectionBehaviorFullScreenPrimary : NSWindowCollectionBehaviorFullScreenNone];

  view = [[NSView alloc] initWithFrame:[[window contentView] bounds]];
  [view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
  [view setWantsBestResolutionOpenGLSurface:YES];
  [[window contentView] addSubview:view];
  [view release];

  if (windowed) {
    [window setTitle:@"World of Warcraft"];
    NSScreen *main = [[NSScreen screens] objectAtIndex:0];
    if (x == 0 && y == 0) {
      NSRect visible = [main visibleFrame];
      [window setFrameOrigin:NSMakePoint(
                                 NSMinX(visible) + (NSWidth(visible) - NSWidth([window frame])) / 2,
                                 NSMinY(visible) + (NSHeight(visible) - NSHeight([window frame])) / 2
                             )];
    } else {
      [window setFrameTopLeftPoint:NSMakePoint(x, NSMaxY([main frame]) - y)];
    }
  } else {
    [window setLevel:CGShieldingWindowLevel()];
    [window setFrame:NSMakeRect(0, 0, CGDisplayPixelsWide(kCGDirectMainDisplay), CGDisplayPixelsHigh(kCGDirectMainDisplay)) display:NO];
  }

  return window;
}

void GxMacWindowDestroy(void *window) {
  NSWindow *nsWindow = (NSWindow *)window;

  [nsWindow orderOut:nil];
  [nsWindow release];
}

void GxMacWindowShow(void *window, int windowed, int focus) {
  NSWindow *nsWindow = (NSWindow *)window;

  if (!nsWindow) {
    return;
  }

  if (windowed) {
    if (focus) {
      [nsWindow makeKeyAndOrderFront:nil];
    }
  } else if (focus) {
    [nsWindow setFrame:NSMakeRect(0, 0, CGDisplayPixelsWide(kCGDirectMainDisplay), CGDisplayPixelsHigh(kCGDirectMainDisplay)) display:YES];
    [nsWindow makeKeyAndOrderFront:nil];
  } else {
    [nsWindow orderOut:nil];
  }
}

void *GxMacWindowContentView(void *window) {
  NSWindow *nsWindow = (NSWindow *)window;

  return [[[nsWindow contentView] subviews] firstObject];
}

double GxMacWindowBackingScale(void *window) {
  return [(NSWindow *)window backingScaleFactor];
}

void GxMacGetWindowBounds(void *window, int structure, int *left, int *top, int *right, int *bottom) {
  NSWindow *nsWindow = (NSWindow *)window;
  NSRect    bounds = structure ? [nsWindow frame] : [nsWindow convertRectToScreen:[[nsWindow contentView] frame]];
  CGFloat   screenTop = NSMaxY([[[NSScreen screens] objectAtIndex:0] frame]);

  *left = (int)NSMinX(bounds);
  *top = (int)(screenTop - NSMaxY(bounds));
  *right = (int)NSMaxX(bounds);
  *bottom = (int)(screenTop - NSMinY(bounds));
}

void GxMacSetWindowResizeLimits(void *window, int minWidth, int minHeight) {
  NSWindow *nsWindow = (NSWindow *)window;

  [nsWindow setContentMinSize:NSMakeSize(minWidth, minHeight)];
  [nsWindow setContentAspectRatio:NSMakeSize(4, 3)];
}

void GxMacSetWindowBounds(void *window, int left, int top, int right, int bottom) {
  NSWindow *nsWindow = (NSWindow *)window;
  CGFloat   screenTop = NSMaxY([[[NSScreen screens] objectAtIndex:0] frame]);
  NSRect    content = NSMakeRect(left, screenTop - bottom, right - left, bottom - top);

  [nsWindow setFrame:[nsWindow frameRectForContentRect:content] display:YES];
}

void GxMacSetBufferRect(void *window, int x, int y, int width, int height) {
  NSWindow *nsWindow = (NSWindow *)window;

  [[[[nsWindow contentView] subviews] firstObject] setFrame:NSMakeRect(x, y, width, height)];
}

void GxMacWindowSetFullScreenStyle(void *window, int fullscreen) {
  NSWindow *nsWindow = (NSWindow *)window;

  if (fullscreen) {
    [nsWindow setStyleMask:NSWindowStyleMaskBorderless];
    [nsWindow setCollectionBehavior:NSWindowCollectionBehaviorFullScreenNone];
  } else {
    [nsWindow setStyleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable];
    [nsWindow setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
    [nsWindow setTitle:@"World of Warcraft"];
  }
}

void GxMacGetScreenBounds(int available, int *left, int *top, int *right, int *bottom) {
  NSScreen *screen = [[NSScreen screens] objectAtIndex:0];
  NSRect    bounds = available ? [screen visibleFrame] : [screen frame];
  CGFloat   screenTop = NSMaxY([screen frame]);

  *left = (int)NSMinX(bounds);
  *top = (int)(screenTop - NSMaxY(bounds));
  *right = (int)NSMaxX(bounds);
  *bottom = (int)(screenTop - NSMinY(bounds));
}

void GxMacSetSystemUIMode(int hidden) {
  [NSApp setPresentationOptions:hidden ? (NSApplicationPresentationHideDock | NSApplicationPresentationHideMenuBar) : NSApplicationPresentationDefault];
}

void GxMacHideCursor() {
  if (!s_cursorHidden) {
    CGDisplayHideCursor(kCGDirectMainDisplay);
    s_cursorHidden = YES;
  }
}

void GxMacInitCursor() {
  if (s_cursorHidden) {
    CGDisplayShowCursor(kCGDirectMainDisplay);
    s_cursorHidden = NO;
  }
}

void *GxMacContextCreate(void *view) {
  NSOpenGLPixelFormatAttribute attributes[16];
  unsigned int                 count = 0;
  NSOpenGLPixelFormat         *pixelFormat;
  NSOpenGLContext             *context;

  attributes[count++] = NSOpenGLPFADoubleBuffer;
  attributes[count++] = NSOpenGLPFAAccelerated;
  attributes[count++] = NSOpenGLPFADepthSize;
  attributes[count++] = 32;
  attributes[count++] = NSOpenGLPFANoRecovery;
  attributes[count++] = 0;

  pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
  if (!pixelFormat) {
    return 0;
  }

  context = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
  [pixelFormat release];
  if (!context) {
    return 0;
  }

  [context setView:(NSView *)view];

  return context;
}

void GxMacContextDestroy(void *context) {
  NSOpenGLContext *nsContext = (NSOpenGLContext *)context;

  [nsContext clearDrawable];
  [nsContext release];
}

void GxMacContextMakeCurrent(void *context) {
  [(NSOpenGLContext *)context makeCurrentContext];
}

void GxMacContextClearCurrent() {
  [NSOpenGLContext clearCurrentContext];
}

void GxMacContextUpdate(void *context) {
  [(NSOpenGLContext *)context update];
}
