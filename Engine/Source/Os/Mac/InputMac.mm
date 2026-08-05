#include "InputMac.h"

#import <AppKit/AppKit.h>
#import <Carbon/Carbon.h>

static int TranslateKey(unsigned short keyCode) {
  switch (keyCode) {
    case kVK_ANSI_A: return OSMAC_KEY_A;
    case kVK_ANSI_B: return OSMAC_KEY_A + 1;
    case kVK_ANSI_C: return OSMAC_KEY_A + 2;
    case kVK_ANSI_D: return OSMAC_KEY_A + 3;
    case kVK_ANSI_E: return OSMAC_KEY_A + 4;
    case kVK_ANSI_F: return OSMAC_KEY_A + 5;
    case kVK_ANSI_G: return OSMAC_KEY_A + 6;
    case kVK_ANSI_H: return OSMAC_KEY_A + 7;
    case kVK_ANSI_I: return OSMAC_KEY_A + 8;
    case kVK_ANSI_J: return OSMAC_KEY_A + 9;
    case kVK_ANSI_K: return OSMAC_KEY_A + 10;
    case kVK_ANSI_L: return OSMAC_KEY_A + 11;
    case kVK_ANSI_M: return OSMAC_KEY_A + 12;
    case kVK_ANSI_N: return OSMAC_KEY_A + 13;
    case kVK_ANSI_O: return OSMAC_KEY_A + 14;
    case kVK_ANSI_P: return OSMAC_KEY_A + 15;
    case kVK_ANSI_Q: return OSMAC_KEY_A + 16;
    case kVK_ANSI_R: return OSMAC_KEY_A + 17;
    case kVK_ANSI_S: return OSMAC_KEY_A + 18;
    case kVK_ANSI_T: return OSMAC_KEY_A + 19;
    case kVK_ANSI_U: return OSMAC_KEY_A + 20;
    case kVK_ANSI_V: return OSMAC_KEY_A + 21;
    case kVK_ANSI_W: return OSMAC_KEY_A + 22;
    case kVK_ANSI_X: return OSMAC_KEY_A + 23;
    case kVK_ANSI_Y: return OSMAC_KEY_A + 24;
    case kVK_ANSI_Z: return OSMAC_KEY_A + 25;

    case kVK_ANSI_0: return OSMAC_KEY_0;
    case kVK_ANSI_1: return OSMAC_KEY_0 + 1;
    case kVK_ANSI_2: return OSMAC_KEY_0 + 2;
    case kVK_ANSI_3: return OSMAC_KEY_0 + 3;
    case kVK_ANSI_4: return OSMAC_KEY_0 + 4;
    case kVK_ANSI_5: return OSMAC_KEY_0 + 5;
    case kVK_ANSI_6: return OSMAC_KEY_0 + 6;
    case kVK_ANSI_7: return OSMAC_KEY_0 + 7;
    case kVK_ANSI_8: return OSMAC_KEY_0 + 8;
    case kVK_ANSI_9: return OSMAC_KEY_0 + 9;

    case kVK_Escape:        return OSMAC_KEY_ESCAPE;
    case kVK_Return:        return OSMAC_KEY_ENTER;
    case kVK_ANSI_KeypadEnter: return OSMAC_KEY_ENTER;
    case kVK_Delete:        return OSMAC_KEY_BACKSPACE;
    case kVK_Tab:           return OSMAC_KEY_TAB;
    case kVK_LeftArrow:     return OSMAC_KEY_LEFT;
    case kVK_UpArrow:       return OSMAC_KEY_UP;
    case kVK_RightArrow:    return OSMAC_KEY_RIGHT;
    case kVK_DownArrow:     return OSMAC_KEY_DOWN;
    case kVK_Help:          return OSMAC_KEY_INSERT;
    case kVK_ForwardDelete: return OSMAC_KEY_DELETE;
    case kVK_Home:          return OSMAC_KEY_HOME;
    case kVK_End:           return OSMAC_KEY_END;
    case kVK_PageUp:        return OSMAC_KEY_PAGEUP;
    case kVK_PageDown:      return OSMAC_KEY_PAGEDOWN;
    case kVK_CapsLock:      return OSMAC_KEY_CAPSLOCK;

    case kVK_F1:  return OSMAC_KEY_F1;
    case kVK_F2:  return OSMAC_KEY_F1 + 1;
    case kVK_F3:  return OSMAC_KEY_F1 + 2;
    case kVK_F4:  return OSMAC_KEY_F1 + 3;
    case kVK_F5:  return OSMAC_KEY_F1 + 4;
    case kVK_F6:  return OSMAC_KEY_F1 + 5;
    case kVK_F7:  return OSMAC_KEY_F1 + 6;
    case kVK_F8:  return OSMAC_KEY_F1 + 7;
    case kVK_F9:  return OSMAC_KEY_F1 + 8;
    case kVK_F10: return OSMAC_KEY_F1 + 9;
    case kVK_F11: return OSMAC_KEY_F1 + 10;
    case kVK_F12: return OSMAC_KEY_F1 + 11;

    case kVK_Space:         return ' ';
    case kVK_ANSI_Minus:    return OSMAC_KEY_MINUS;
    case kVK_ANSI_Equal:    return OSMAC_KEY_PLUS;
    case kVK_ANSI_Comma:    return OSMAC_KEY_COMMA;
    case kVK_ANSI_Period:   return OSMAC_KEY_PERIOD;

    default: return -1;
  }
}

// the engine takes a button mask, not the ordinal cocoa reports
static int TranslateButton(NSInteger buttonNumber) {
  switch (buttonNumber) {
    case 0:  return OSMAC_MOUSE_LEFT;
    case 1:  return OSMAC_MOUSE_RIGHT;
    case 2:  return OSMAC_MOUSE_MIDDLE;
    case 3:  return OSMAC_MOUSE_XBUTTON1;
    case 4:  return OSMAC_MOUSE_XBUTTON2;
    default: return 0;
  }
}

void OsMacApplicationStart() {
  [NSApplication sharedApplication];
  [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
  [NSApp finishLaunching];
  [NSApp activateIgnoringOtherApps:YES];
}

static NSWindow *GameWindow() {
  NSWindow *window = [NSApp mainWindow];

  if (!window) {
    window = [NSApp keyWindow];
  }

  if (!window) {
    for (NSWindow *candidate in [NSApp windows]) {
      if ([candidate isVisible]) {
        window = candidate;
        break;
      }
    }
  }

  return window;
}

// locationInWindow is measured from the corner of the frame, title bar and
// all, so it has to come across into the content view the device draws into
static NSPoint ContentLocation(NSEvent *nsEvent) {
  NSWindow *window = [nsEvent window];
  NSPoint   location = [nsEvent locationInWindow];

  if (!window) {
    window = GameWindow();
  }

  if (window) {
    location = [[window contentView] convertPoint:location fromView:nil];
  }

  return location;
}

// the close box orders the window out rather than ending the process, so the
// disappearance of the last window is what stands in for WM_CLOSE
static int WindowsGone() {
  static int s_sawWindow;

  for (NSWindow *window in [NSApp windows]) {
    if ([window isVisible]) {
      s_sawWindow = 1;
      return 0;
    }
  }

  return s_sawWindow;
}

int OsMacPollEvent(OSMACEVENT *event) {
  NSEvent *nsEvent = [NSApp nextEventMatchingMask:NSEventMaskAny
                                        untilDate:[NSDate distantPast]
                                           inMode:NSDefaultRunLoopMode
                                          dequeue:YES];

  if (!nsEvent) {
    if (WindowsGone()) {
      event->type = OSMAC_EVENT_CLOSE;
      event->param[0] = 0;
      event->param[1] = 0;
      event->param[2] = 0;
      event->param[3] = 0;
      return 1;
    }

    return 0;
  }

  event->type = OSMAC_EVENT_NONE;
  event->param[0] = 0;
  event->param[1] = 0;
  event->param[2] = 0;
  event->param[3] = 0;

  switch ([nsEvent type]) {
    case NSEventTypeKeyDown:
    case NSEventTypeKeyUp: {
      int key;

      if ([nsEvent type] == NSEventTypeKeyDown && ([nsEvent modifierFlags] & NSEventModifierFlagCommand)) {
        NSString *unmodified = [nsEvent charactersIgnoringModifiers];

        // there is no menu bar to carry the quit item, so the key equivalent
        // is answered here
        if ([unmodified length] && [unmodified characterAtIndex:0] == 'q') {
          event->type = OSMAC_EVENT_CLOSE;
          return 1;
        }
      }

      key = TranslateKey([nsEvent keyCode]);

      if (key >= 0) {
        event->type = [nsEvent type] == NSEventTypeKeyDown ? OSMAC_EVENT_KEY_DOWN : OSMAC_EVENT_KEY_UP;
        event->param[0] = key;
      }

      if ([nsEvent type] == NSEventTypeKeyDown) {
        NSString *characters = [nsEvent characters];

        if ([characters length]) {
          unichar character = [characters characterAtIndex:0];

          if (character >= 0x20 && character != 0x7F) {
            event->param[1] = character;
          }
        }
      }
      break;
    }

    case NSEventTypeLeftMouseDown:
    case NSEventTypeRightMouseDown:
    case NSEventTypeOtherMouseDown:
    case NSEventTypeLeftMouseUp:
    case NSEventTypeRightMouseUp:
    case NSEventTypeOtherMouseUp: {
      NSPoint location = ContentLocation(nsEvent);
      int     down = [nsEvent type] == NSEventTypeLeftMouseDown || [nsEvent type] == NSEventTypeRightMouseDown
                  || [nsEvent type] == NSEventTypeOtherMouseDown;

      event->type = down ? OSMAC_EVENT_MOUSE_DOWN : OSMAC_EVENT_MOUSE_UP;
      event->param[0] = TranslateButton([nsEvent buttonNumber]);
      event->param[1] = static_cast<int>(location.x);
      event->param[2] = static_cast<int>(location.y);

      if (!event->param[0]) {
        event->type = OSMAC_EVENT_NONE;
      }
      break;
    }

    case NSEventTypeMouseMoved:
    case NSEventTypeLeftMouseDragged:
    case NSEventTypeRightMouseDragged:
    case NSEventTypeOtherMouseDragged: {
      NSPoint location = ContentLocation(nsEvent);

      event->type = OSMAC_EVENT_MOUSE_MOVE;
      event->param[0] = static_cast<int>(location.x);
      event->param[1] = static_cast<int>(location.y);
      event->param[2] = static_cast<int>([nsEvent deltaX]);
      event->param[3] = static_cast<int>([nsEvent deltaY]);
      break;
    }

    case NSEventTypeScrollWheel:
      event->type = OSMAC_EVENT_MOUSE_WHEEL;
      event->param[0] = static_cast<int>([nsEvent deltaY] * 120.0);
      break;

    default:
      break;
  }

  [NSApp sendEvent:nsEvent];
  return 1;
}

// the position arrives in content view coordinates measured from the top
// left, the same space LocalToGlobal took, and has to reach the warp in
// display coordinates
void OsMacSetCursorPosition(int x, int y) {
  NSWindow *window = GameWindow();
  NSRect    content;
  NSRect    onScreen;
  CGPoint   global;

  if (!window) {
    return;
  }

  content = [[window contentView] bounds];
  onScreen = [window convertRectToScreen:NSMakeRect(x, NSHeight(content) - y, 0.0, 0.0)];

  global.x = NSMinX(onScreen);
  global.y = NSMaxY([[[NSScreen screens] objectAtIndex:0] frame]) - NSMinY(onScreen);

  CGWarpMouseCursorPosition(global);
}

int OsMacIsWindowActive() {
  NSWindow *window = GameWindow();

  return window && [window isKeyWindow] && [NSApp isActive];
}

void OsMacGetMainWindowRect(int *width, int *height) {
  NSWindow *window = GameWindow();
  NSRect    frame;

  if (window) {
    frame = [[window contentView] frame];
  } else {
    frame = [[NSScreen mainScreen] frame];
  }

  *width = static_cast<int>(NSWidth(frame));
  *height = static_cast<int>(NSHeight(frame));
}

int OsMacMessageBox(const char *message, const char *title) {
  NSAlert *alert = [[NSAlert alloc] init];

  [alert setMessageText:[NSString stringWithUTF8String:title ? title : ""]];
  [alert setInformativeText:[NSString stringWithUTF8String:message ? message : ""]];
  [alert addButtonWithTitle:@"OK"];

  return static_cast<int>([alert runModal]);
}

void OsMacSetWindowTitle(void *window, const char *text) {
  NSWindow *nsWindow = (__bridge NSWindow *)window;
  NSString *title = [NSString stringWithUTF8String:text ? text : ""];

  [nsWindow setTitle:title];
}
