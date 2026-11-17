#include "InputMac.h"

#import <AppKit/AppKit.h>

#include <math.h>
#include <string.h>

@interface OsMacEventHandler : NSObject <NSWindowDelegate> {
 @public
  NSWindow             *m_window;
  OSMACEVENTHANDLERPROC m_handler;
  void                 *m_userData;
  EventTypeSpec        *m_list;
  UInt32                m_numTypes;
  id                    m_monitor;
  NSTrackingArea       *m_trackingArea;
}
@end

@implementation OsMacEventHandler

- (BOOL)wants:(UInt32)eventClass kind:(UInt32)eventKind {
  UInt32 i;

  for (i = 0; i < m_numTypes; ++i) {
    if (m_list[i].eventClass == eventClass && m_list[i].eventKind == eventKind) {
      return YES;
    }
  }

  return NO;
}

- (OSStatus)send:(OSMACEVENT *)event {
  if (![self wants:event->eventClass kind:event->eventKind]) {
    return eventNotHandledErr;
  }

  return m_handler(event, m_userData);
}

- (void)sendWindowEvent:(UInt32)eventKind {
  OSMACEVENT event;

  memset(&event, 0, sizeof(event));
  event.eventClass = kEventClassWindow;
  event.eventKind = eventKind;
  event.window = m_window;
  [self send:&event];
}

- (NSEvent *)dispatch:(NSEvent *)nsEvent {
  OSMACEVENT event;
  OSStatus   result;

  if (m_window && [nsEvent window] != m_window) {
    return nsEvent;
  }

  memset(&event, 0, sizeof(event));
  event.event = nsEvent;
  event.window = [nsEvent window];

  switch ([nsEvent type]) {
    case NSEventTypeLeftMouseDown:
    case NSEventTypeRightMouseDown:
    case NSEventTypeOtherMouseDown:
      event.eventClass = kEventClassMouse;
      event.eventKind = kEventMouseDown;
      break;

    case NSEventTypeLeftMouseUp:
    case NSEventTypeRightMouseUp:
    case NSEventTypeOtherMouseUp:
      event.eventClass = kEventClassMouse;
      event.eventKind = kEventMouseUp;
      break;

    case NSEventTypeMouseMoved:
      event.eventClass = kEventClassMouse;
      event.eventKind = kEventMouseMoved;
      break;

    case NSEventTypeLeftMouseDragged:
    case NSEventTypeRightMouseDragged:
    case NSEventTypeOtherMouseDragged:
      event.eventClass = kEventClassMouse;
      event.eventKind = kEventMouseDragged;
      break;

    case NSEventTypeScrollWheel:
      if ([nsEvent deltaY] == 0.0) {
        return nsEvent;
      }
      event.eventClass = kEventClassMouse;
      event.eventKind = kEventMouseWheelMoved;
      break;

    case NSEventTypeKeyDown:
      if (([nsEvent modifierFlags] & NSEventModifierFlagCommand) && [[nsEvent charactersIgnoringModifiers] isEqualToString:@"q"]) {
        event.eventClass = kEventClassCommand;
        event.eventKind = kEventCommandProcess;
        event.command.commandID = kHICommandQuit;
        break;
      }

      if (![self wants:kEventClassKeyboard kind:kEventRawKeyDown]) {
        return nsEvent;
      }

      event.eventClass = kEventClassKeyboard;
      event.eventKind = [nsEvent isARepeat] ? kEventRawKeyRepeat : kEventRawKeyDown;
      [self send:&event];

      event.eventClass = kEventClassTextInput;
      event.eventKind = kEventTextInputUnicodeForKeyEvent;
      [self send:&event];
      return nil;

    case NSEventTypeKeyUp:
      if (![self wants:kEventClassKeyboard kind:kEventRawKeyUp]) {
        return nsEvent;
      }

      event.eventClass = kEventClassKeyboard;
      event.eventKind = kEventRawKeyUp;
      [self send:&event];
      return nil;

    case NSEventTypeFlagsChanged:
      event.eventClass = kEventClassKeyboard;
      event.eventKind = kEventRawKeyModifiersChanged;
      break;

    case NSEventTypeAppKitDefined:
      if ([nsEvent subtype] == NSEventSubtypeApplicationActivated) {
        event.eventClass = kEventClassApplication;
        event.eventKind = kEventAppActivated;
      } else if ([nsEvent subtype] == NSEventSubtypeApplicationDeactivated) {
        event.eventClass = kEventClassApplication;
        event.eventKind = kEventAppDeactivated;
      } else {
        return nsEvent;
      }
      break;

    case NSEventTypeApplicationDefined:
      event.eventClass = kEventClassCommand;
      event.eventKind = [nsEvent subtype];
      event.command.commandID = (UInt32)[nsEvent data1];
      break;

    default:
      return nsEvent;
  }

  result = [self send:&event];
  return result == noErr ? nil : nsEvent;
}

- (void)windowBecameKey:(NSNotification *)notification {
  [self sendWindowEvent:kEventWindowActivated];
}

- (void)windowResignedKey:(NSNotification *)notification {
  [self sendWindowEvent:kEventWindowDeactivated];
}

- (void)windowDidChangeBounds:(NSNotification *)notification {
  [self sendWindowEvent:kEventWindowBoundsChanged];
}

- (BOOL)windowShouldClose:(NSWindow *)sender {
  OSMACEVENT event;

  memset(&event, 0, sizeof(event));
  event.eventClass = kEventClassWindow;
  event.eventKind = kEventWindowClose;
  event.window = m_window;
  return [self send:&event] != noErr;
}

- (void)mouseEntered:(NSEvent *)nsEvent {
  OSMACEVENT event;

  memset(&event, 0, sizeof(event));
  event.eventClass = kEventClassMouse;
  event.eventKind = kEventMouseEntered;
  event.event = nsEvent;
  event.window = m_window;
  [self send:&event];
}

- (void)mouseExited:(NSEvent *)nsEvent {
  OSMACEVENT event;

  memset(&event, 0, sizeof(event));
  event.eventClass = kEventClassMouse;
  event.eventKind = kEventMouseExited;
  event.event = nsEvent;
  event.window = m_window;
  [self send:&event];
}

@end

void OsMacApplicationStart() {
  [NSApplication sharedApplication];
  [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
  [NSApp finishLaunching];
  [NSApp activateIgnoringOtherApps:YES];
}

OSStatus OsMacReceiveNextEvent(EventTimeout timeout) {
  OSStatus result = eventLoopTimedOutErr;

  @autoreleasepool {
    NSEvent *nsEvent = [NSApp nextEventMatchingMask:NSEventMaskAny
                                          untilDate:[NSDate dateWithTimeIntervalSinceNow:timeout]
                                             inMode:NSDefaultRunLoopMode
                                            dequeue:YES];

    if (nsEvent) {
      [NSApp sendEvent:nsEvent];
      result = noErr;
    }
  }

  return result;
}

OSStatus OsMacInstallEventHandler(
    void *window, OSMACEVENTHANDLERPROC handler, UInt32 numTypes, const EventTypeSpec *list, void *userData, void **outRef
) {
  OsMacEventHandler    *eventHandler = [[OsMacEventHandler alloc] init];
  NSNotificationCenter *center = [NSNotificationCenter defaultCenter];

  eventHandler->m_window = (NSWindow *)window;
  eventHandler->m_handler = handler;
  eventHandler->m_userData = userData;
  eventHandler->m_numTypes = numTypes;
  eventHandler->m_list = new EventTypeSpec[numTypes];
  memcpy(eventHandler->m_list, list, numTypes * sizeof(EventTypeSpec));

  eventHandler->m_monitor = [[NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskAny
                                                                   handler:^NSEvent *(NSEvent *nsEvent) {
                                                                     return [eventHandler dispatch:nsEvent];
                                                                   }] retain];

  if (window) {
    NSWindow *nsWindow = (NSWindow *)window;

    [center addObserver:eventHandler selector:@selector(windowBecameKey:) name:NSWindowDidBecomeKeyNotification object:nsWindow];
    [center addObserver:eventHandler selector:@selector(windowResignedKey:) name:NSWindowDidResignKeyNotification object:nsWindow];
    [center addObserver:eventHandler selector:@selector(windowDidChangeBounds:) name:NSWindowDidResizeNotification object:nsWindow];
    [center addObserver:eventHandler selector:@selector(windowDidChangeBounds:) name:NSWindowDidMoveNotification object:nsWindow];

    if ([eventHandler wants:kEventClassWindow kind:kEventWindowClose]) {
      [nsWindow setDelegate:eventHandler];
    }

    if ([eventHandler wants:kEventClassMouse kind:kEventMouseEntered] || [eventHandler wants:kEventClassMouse kind:kEventMouseExited]) {
      eventHandler->m_trackingArea = [[NSTrackingArea alloc]
          initWithRect:NSZeroRect
               options:NSTrackingMouseEnteredAndExited | NSTrackingActiveAlways | NSTrackingInVisibleRect
                 owner:eventHandler
              userInfo:nil];
      [[nsWindow contentView] addTrackingArea:eventHandler->m_trackingArea];
    }
  }

  *outRef = eventHandler;
  return noErr;
}

void OsMacRemoveEventHandler(void *handlerRef) {
  OsMacEventHandler *eventHandler = (OsMacEventHandler *)handlerRef;

  [[NSNotificationCenter defaultCenter] removeObserver:eventHandler];

  if (eventHandler->m_window && [eventHandler->m_window delegate] == eventHandler) {
    [eventHandler->m_window setDelegate:nil];
  }

  if (eventHandler->m_trackingArea) {
    [[eventHandler->m_window contentView] removeTrackingArea:eventHandler->m_trackingArea];
    [eventHandler->m_trackingArea release];
  }

  [NSEvent removeMonitor:eventHandler->m_monitor];
  [eventHandler->m_monitor release];

  delete[] eventHandler->m_list;
  [eventHandler release];
}

OSStatus OsMacGetEventParameter(
    OSMACEVENT *event, EventParamName name, EventParamType desiredType, EventParamType *actualType, ByteCount bufferSize,
    ByteCount *actualSize, void *data
) {
  NSEvent         *nsEvent = (NSEvent *)event->event;
  NSUInteger       flags;
  NSString        *characters;
  NSUInteger       count;
  NSUInteger       i;
  UniChar         *text;
  HIPoint          point;
  UInt32           value;
  EventMouseButton button;
  SInt32           delta;
  const void      *source;
  ByteCount        size;

  switch (name) {
    case kEventParamMouseLocation:
      point = CGEventGetLocation([nsEvent CGEvent]);
      source = &point;
      size = sizeof(point);
      break;

    case kEventParamMouseDelta:
      point.x = [nsEvent deltaX];
      point.y = [nsEvent deltaY];
      source = &point;
      size = sizeof(point);
      break;

    case kEventParamKeyModifiers:
      flags = [nsEvent modifierFlags];
      value = 0;
      if (flags & NSEventModifierFlagCommand) {
        value |= cmdKey;
      }
      if (flags & NSEventModifierFlagShift) {
        value |= shiftKey;
      }
      if (flags & NSEventModifierFlagCapsLock) {
        value |= alphaLock;
      }
      if (flags & NSEventModifierFlagOption) {
        value |= optionKey;
      }
      if (flags & NSEventModifierFlagControl) {
        value |= controlKey;
      }
      source = &value;
      size = sizeof(value);
      break;

    case kEventParamMouseButton:
      button = (EventMouseButton)([nsEvent buttonNumber] + 1);
      source = &button;
      size = sizeof(button);
      break;

    case kEventParamMouseWheelDelta:
      delta = [nsEvent deltaY] > 0.0 ? (SInt32)ceil([nsEvent deltaY]) : (SInt32)floor([nsEvent deltaY]);
      source = &delta;
      size = sizeof(delta);
      break;

    case kEventParamKeyCode:
      value = [nsEvent keyCode];
      source = &value;
      size = sizeof(value);
      break;

    case kEventParamTextInputSendText:
      characters = [nsEvent characters];
      count = [characters length];
      size = count * sizeof(UniChar);
      if (count > bufferSize / sizeof(UniChar)) {
        count = bufferSize / sizeof(UniChar);
      }

      text = (UniChar *)data;
      [characters getCharacters:text range:NSMakeRange(0, count)];

      for (i = 0; i < count; ++i) {
        if (text[i] == 0x7F) {
          text[i] = kBackspaceCharCode;
        } else if (text[i] == NSDeleteFunctionKey) {
          text[i] = kDeleteCharCode;
        } else if (text[i] >= 0xF700 && text[i] <= 0xF8FF) {
          text[i] = kFunctionKeyCharCode;
        }
      }

      if (actualType) {
        *actualType = typeUnicodeText;
      }
      if (actualSize) {
        *actualSize = size;
      }
      return noErr;

    case kEventParamDirectObject:
      if (event->eventClass == kEventClassCommand) {
        source = &event->command;
        size = sizeof(event->command);
      } else {
        source = &event->window;
        size = sizeof(event->window);
      }
      break;

    default:
      if (actualSize) {
        *actualSize = 0;
      }
      return eventParameterNotFoundErr;
  }

  memcpy(data, source, size < bufferSize ? size : bufferSize);

  if (actualType) {
    *actualType = desiredType;
  }
  if (actualSize) {
    *actualSize = size;
  }
  return noErr;
}

void OsMacGlobalToLocalPoint(void *window, HIPoint *point) {
  NSWindow *nsWindow = (NSWindow *)window;
  NSView   *view = [[[nsWindow contentView] subviews] firstObject];
  NSPoint   location;

  location = NSMakePoint(point->x, NSMaxY([[[NSScreen screens] objectAtIndex:0] frame]) - point->y);
  location = [nsWindow convertPointFromScreen:location];
  location = [view convertPoint:location fromView:nil];

  point->x = location.x * [nsWindow backingScaleFactor];
  point->y = ([view isFlipped] ? location.y : NSHeight([view bounds]) - location.y) * [nsWindow backingScaleFactor];
}

void OsMacLocalToGlobalPoint(void *window, HIPoint *point) {
  NSWindow *nsWindow = (NSWindow *)window;
  NSView   *view = [[[nsWindow contentView] subviews] firstObject];
  NSPoint   location;

#if BUILD_FIXES
  if (!nsWindow) {
    return;
  }
#endif

  location = NSMakePoint(point->x / [nsWindow backingScaleFactor], [view isFlipped] ? point->y / [nsWindow backingScaleFactor] : NSHeight([view bounds]) - point->y / [nsWindow backingScaleFactor]);
  location = [view convertPoint:location toView:nil];
  location = [nsWindow convertPointToScreen:location];

  point->x = location.x;
  point->y = NSMaxY([[[NSScreen screens] objectAtIndex:0] frame]) - location.y;
}
