#import <AppKit/AppKit.h>

#include "MacClient.h"

long MacClient::ShowAboutBox() {
  NSWindow    *window;
  NSTextField *text;

  window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 499, 78)
                                       styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                                         backing:NSBackingStoreBuffered
                                           defer:NO];
  [window setTitle:@"About World of Warcraft"];

  text = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 20, 459, 38)];
  [text setStringValue:(NSString *)CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(), CFSTR("CFBundleGetInfoString"))];
  [text setEditable:NO];
  [text setSelectable:NO];
  [text setBezeled:NO];
  [text setDrawsBackground:NO];
  [[window contentView] addSubview:text];
  [text release];

  [window center];
  [window makeKeyAndOrderFront:nil];
  return noErr;
}
