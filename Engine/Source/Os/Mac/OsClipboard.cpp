#include <storm.h>

#include "Os/W32/OsClipboard.h"

#include <ApplicationServices/ApplicationServices.h>
#include <ctype.h>
#include <string.h>

BOOL OsClipboardGetString(char *buf, UINT bufSize) {
  PasteboardRef    pasteboard;
  PasteboardItemID item;
  CFDataRef        data;
  CFIndex          size = 0;
  OSStatus         err;

  buf[0] = 0;
  err = PasteboardCreate(kPasteboardClipboard, &pasteboard);
  if (!(err == noErr)) {
    SErrDisplayErrorFmt(
        STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
        isprint((err >> 24) & 0xFF) && isprint((err >> 16) & 0xFF) && isprint((err >> 8) & 0xFF) && isprint(err & 0xFF)
            ? "\"%s\", %s = %ld (0x%08X, '%c%c%c%c')"
            : "\"%s\", %s = %ld (0x%08X)",
        "err == noErr", "err", (long)err, err, (err >> 24) & 0xFF, (err >> 16) & 0xFF, (err >> 8) & 0xFF, err & 0xFF
    );
  }
  if (err == noErr) {
    PasteboardSynchronize(pasteboard);
    err = PasteboardGetItemIdentifier(pasteboard, 1, &item);
    if (err == noErr) {
      err = PasteboardCopyItemFlavorData(pasteboard, item, CFSTR("public.utf8-plain-text"), &data);
    }
    if (!(err == noErr || err == badPasteboardIndexErr || err == badPasteboardFlavorErr)) {
      SErrDisplayErrorFmt(
          STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
          isprint((err >> 24) & 0xFF) && isprint((err >> 16) & 0xFF) && isprint((err >> 8) & 0xFF) && isprint(err & 0xFF)
              ? "\"%s\", %s = %ld (0x%08X, '%c%c%c%c')"
              : "\"%s\", %s = %ld (0x%08X)",
          "err == noErr || err == badPasteboardIndexErr || err == badPasteboardFlavorErr", "err", (long)err, err, (err >> 24) & 0xFF,
          (err >> 16) & 0xFF, (err >> 8) & 0xFF, err & 0xFF
      );
    }
    if (err == noErr) {
      size = CFDataGetLength(data);
      if (size > 0 && bufSize > (UINT)size) {
        CFDataGetBytes(data, CFRangeMake(0, size), (UInt8 *)buf);
        buf[size] = 0;
      }
      CFRelease(data);
    }
    CFRelease(pasteboard);
  }

  return err == noErr;
}

char *OsClipboardGetString() {
  char *string = (char *)SMemAlloc(1024, __FILE__, __LINE__, 0);

  string[0] = 0;
  OsClipboardGetString(string, 1024);
  return (char *)SMemReAlloc(string, strlen(string) + 1, __FILE__, __LINE__, 0);
}

void OsClipboardFreeString(char *string) {
  if (string) {
    SMemFree(string, __FILE__, __LINE__, 0);
  }
}

BOOL OsClipboardPutString(LPCSTR string) {
  PasteboardRef pasteboard;
  CFDataRef     data;
  OSStatus      err;

  err = PasteboardCreate(kPasteboardClipboard, &pasteboard);
  if (!(err == noErr)) {
    SErrDisplayErrorFmt(
        STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
        isprint((err >> 24) & 0xFF) && isprint((err >> 16) & 0xFF) && isprint((err >> 8) & 0xFF) && isprint(err & 0xFF)
            ? "\"%s\", %s = %ld (0x%08X, '%c%c%c%c')"
            : "\"%s\", %s = %ld (0x%08X)",
        "err == noErr", "err", (long)err, err, (err >> 24) & 0xFF, (err >> 16) & 0xFF, (err >> 8) & 0xFF, err & 0xFF
    );
  }
  if (err == noErr) {
    err = PasteboardClear(pasteboard);
    if (!(err == noErr)) {
      SErrDisplayErrorFmt(
          STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
          isprint((err >> 24) & 0xFF) && isprint((err >> 16) & 0xFF) && isprint((err >> 8) & 0xFF) && isprint(err & 0xFF)
              ? "\"%s\", %s = %ld (0x%08X, '%c%c%c%c')"
              : "\"%s\", %s = %ld (0x%08X)",
          "err == noErr", "err", (long)err, err, (err >> 24) & 0xFF, (err >> 16) & 0xFF, (err >> 8) & 0xFF, err & 0xFF
      );
    }
    if (err == noErr) {
      data = CFDataCreate(kCFAllocatorDefault, (const UInt8 *)string, strlen(string));
      err = PasteboardPutItemFlavor(pasteboard, (PasteboardItemID)1, CFSTR("public.utf8-plain-text"), data, kPasteboardFlavorNoFlags);
      CFRelease(data);
      if (!(err == noErr)) {
        SErrDisplayErrorFmt(
            STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
            isprint((err >> 24) & 0xFF) && isprint((err >> 16) & 0xFF) && isprint((err >> 8) & 0xFF) && isprint(err & 0xFF)
                ? "\"%s\", %s = %ld (0x%08X, '%c%c%c%c')"
                : "\"%s\", %s = %ld (0x%08X)",
            "err == noErr", "err", (long)err, err, (err >> 24) & 0xFF, (err >> 16) & 0xFF, (err >> 8) & 0xFF, err & 0xFF
        );
      }
    }
    CFRelease(pasteboard);
  }

  return err == noErr;
}
