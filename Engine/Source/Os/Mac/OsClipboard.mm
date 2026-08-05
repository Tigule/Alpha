#include "Os/W32/OsClipboard.h"

#include <stdlib.h>
#include <string.h>

#import <AppKit/AppKit.h>

int OsClipboardGetString(char *buf, unsigned int bufSize) {
  NSString *string = [[NSPasteboard generalPasteboard] stringForType:NSPasteboardTypeString];

  if (!string) {
    return 0;
  }

  return [string getCString:buf maxLength:bufSize encoding:NSUTF8StringEncoding];
}

char *OsClipboardGetString() {
  NSString *string = [[NSPasteboard generalPasteboard] stringForType:NSPasteboardTypeString];
  const char *utf8;
  char       *buffer;
  size_t      bytes;

  if (!string) {
    return 0;
  }

  utf8 = [string UTF8String];
  if (!utf8) {
    return 0;
  }

  bytes = strlen(utf8) + 1;
  buffer = static_cast<char *>(malloc(bytes));
  memcpy(buffer, utf8, bytes);

  return buffer;
}

void OsClipboardFreeString(char *string) {
  free(string);
}

int OsClipboardPutString(const char *string) {
  NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
  NSString     *value = [NSString stringWithUTF8String:string];

  if (!value) {
    return 0;
  }

  [pasteboard clearContents];
  return [pasteboard setString:value forType:NSPasteboardTypeString];
}
