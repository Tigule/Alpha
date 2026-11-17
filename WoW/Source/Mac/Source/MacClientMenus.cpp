#define BOOL OBJC_BOOL
#import <AppKit/AppKit.h>
#undef BOOL

#include <storm.h>

#include "MacClient.h"
#include "Client.h"
#include "Console/ConsoleCommand.h"
#include "Console/ConsoleVar.h"

#include <Os/Mac/InputMac.h>
#include <Os/W32/Input.h>

#include <ctype.h>

@interface MacClientMenuTarget : NSObject
- (void)processCommand:(id)sender;
@end

@implementation MacClientMenuTarget

- (void)processCommand:(id)sender {
  [NSApp postEvent:[NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                      location:NSZeroPoint
                                 modifierFlags:0
                                     timestamp:0
                                  windowNumber:0
                                       context:nil
                                       subtype:kEventCommandProcess
                                         data1:[sender tag]
                                         data2:0]
           atStart:NO];
}

@end

static const struct {
  UInt32 command;
  KEY    key;
} s_editMenuKeys[] = {
    {kHICommandUndo, KEY_Z}, {kHICommandCut, KEY_X}, {kHICommandCopy, KEY_C}, {kHICommandPaste, KEY_V}, {kHICommandSelectAll, KEY_A}
};

static OSStatus HICommandEditMenuHandler(OSMACEVENT *event, UInt32 command) {
  int key = -1;
  int i;

  for (i = 0; i < 5; ++i) {
    if (command == s_editMenuKeys[i].command) {
      key = s_editMenuKeys[i].key;
      break;
    }
  }

  if (key == -1) {
    return eventNotHandledErr;
  }

  OsInputPostEvent(OS_INPUT_KEY_DOWN, KEY_CONTROL, 0, 0, 0);
  OsInputPostEvent(OS_INPUT_KEY_DOWN, key, 0, 0, 0);
  OsInputPostEvent(OS_INPUT_KEY_UP, key, 0, 0, 0);
  OsInputPostEvent(OS_INPUT_KEY_UP, KEY_CONTROL, 0, 0, 0);
  return noErr;
}

static OSStatus HICommandAboutHandler(OSMACEVENT *event, UInt32 command) {
  if (!(command == kHICommandAbout)) {
    SErrDisplayErrorFmt(
        STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
        isprint((command >> 24) & 0xFF) && isprint((command >> 16) & 0xFF) && isprint((command >> 8) & 0xFF) && isprint(command & 0xFF)
            ? "\"%s\", %s = %ld (0x%08X, '%c%c%c%c')"
            : "\"%s\", %s = %ld (0x%08X)",
        "command == kHICommandAbout", "command", (long)command, command, (command >> 24) & 0xFF, (command >> 16) & 0xFF, (command >> 8) & 0xFF,
        command & 0xFF
    );
  }

  return MacClient::ShowAboutBox();
}

static OSStatus HICommandFullScreenHandler(OSMACEVENT *event, UInt32 command) {
  CVar *window;

  if (DrawingLoadingScreen()) {
    return noErr;
  }

  window = CVar::Lookup("gxWindow");
  if (window) {
    window->Set(window->GetInt() ? "0" : "1", true, false, false);
    ConsoleCommandExecute("gxRestart", 1);
  }

  return noErr;
}

void MacClient::InitializeMenus() {
  static MacClientMenuTarget *s_target;
  NSMenu                     *menuBar;
  NSMenu                     *menu;
  NSMenu                     *services;
  NSMenuItem                *item;

  s_target = [[MacClientMenuTarget alloc] init];
  menuBar = [[NSMenu alloc] initWithTitle:@""];

  menu = [[NSMenu alloc] initWithTitle:@"World of Warcraft"];
  item = [menu addItemWithTitle:@"About World of Warcraft..." action:@selector(processCommand:) keyEquivalent:@""];
  [item setTarget:s_target];
  [item setTag:kHICommandAbout];
  [menu addItem:[NSMenuItem separatorItem]];
  item = [menu addItemWithTitle:@"Switch To Full Screen Mode" action:@selector(processCommand:) keyEquivalent:@"m"];
  [item setTarget:s_target];
  [item setTag:'Full'];
  [menu addItem:[NSMenuItem separatorItem]];
  services = [[NSMenu alloc] initWithTitle:@"Services"];
  [[menu addItemWithTitle:@"Services" action:0 keyEquivalent:@""] setSubmenu:services];
  [NSApp setServicesMenu:services];
  [services release];
  [menu addItem:[NSMenuItem separatorItem]];
  [menu addItemWithTitle:@"Hide World of Warcraft" action:@selector(hide:) keyEquivalent:@"h"];
  item = [menu addItemWithTitle:@"Hide Others" action:@selector(hideOtherApplications:) keyEquivalent:@"h"];
  [item setKeyEquivalentModifierMask:NSEventModifierFlagOption | NSEventModifierFlagCommand];
  [menu addItemWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""];
  [menu addItem:[NSMenuItem separatorItem]];
  item = [menu addItemWithTitle:@"Quit World of Warcraft" action:@selector(processCommand:) keyEquivalent:@"q"];
  [item setTarget:s_target];
  [item setTag:kHICommandQuit];
  [[menuBar addItemWithTitle:@"" action:0 keyEquivalent:@""] setSubmenu:menu];
  [menu release];

  menu = [[NSMenu alloc] initWithTitle:@"Edit"];
  item = [menu addItemWithTitle:@"Undo" action:@selector(processCommand:) keyEquivalent:@"z"];
  [item setTarget:s_target];
  [item setTag:kHICommandUndo];
  [menu addItem:[NSMenuItem separatorItem]];
  item = [menu addItemWithTitle:@"Cut" action:@selector(processCommand:) keyEquivalent:@"x"];
  [item setTarget:s_target];
  [item setTag:kHICommandCut];
  item = [menu addItemWithTitle:@"Copy" action:@selector(processCommand:) keyEquivalent:@"c"];
  [item setTarget:s_target];
  [item setTag:kHICommandCopy];
  item = [menu addItemWithTitle:@"Paste" action:@selector(processCommand:) keyEquivalent:@"v"];
  [item setTarget:s_target];
  [item setTag:kHICommandPaste];
  item = [menu addItemWithTitle:@"Select All" action:@selector(processCommand:) keyEquivalent:@"a"];
  [item setTarget:s_target];
  [item setTag:kHICommandSelectAll];
  [[menuBar addItemWithTitle:@"Edit" action:0 keyEquivalent:@""] setSubmenu:menu];
  [menu release];

  menu = [[NSMenu alloc] initWithTitle:@"Window"];
  [menu addItemWithTitle:@"Minimize Window" action:@selector(performMiniaturize:) keyEquivalent:@""];
  item = [menu addItemWithTitle:@"Minimize All Windows" action:@selector(miniaturizeAll:) keyEquivalent:@""];
  [item setKeyEquivalentModifierMask:NSEventModifierFlagOption];
  [item setAlternate:YES];
  [menu addItem:[NSMenuItem separatorItem]];
  [menu addItemWithTitle:@"Bring All to Front" action:@selector(arrangeInFront:) keyEquivalent:@""];
  item = [menu addItemWithTitle:@"Arrange in Front" action:@selector(arrangeInFront:) keyEquivalent:@""];
  [item setKeyEquivalentModifierMask:NSEventModifierFlagOption | NSEventModifierFlagCommand];
  [item setAlternate:YES];
  [[menuBar addItemWithTitle:@"Window" action:0 keyEquivalent:@""] setSubmenu:menu];
  [NSApp setWindowsMenu:menu];
  [menu release];

  [NSApp setMainMenu:menuBar];
  [menuBar release];

  OsInputRegisterHICommandHandler('Full', HICommandFullScreenHandler);
  OsInputRegisterHICommandHandler(kHICommandAbout, HICommandAboutHandler);
  OsInputRegisterHICommandHandler(kHICommandUndo, HICommandEditMenuHandler);
  OsInputRegisterHICommandHandler(kHICommandCut, HICommandEditMenuHandler);
  OsInputRegisterHICommandHandler(kHICommandCopy, HICommandEditMenuHandler);
  OsInputRegisterHICommandHandler(kHICommandPaste, HICommandEditMenuHandler);
  OsInputRegisterHICommandHandler(kHICommandSelectAll, HICommandEditMenuHandler);
}
