#ifndef ENGINE_SOURCE_OS_MAC_INPUTMAC_H
#define ENGINE_SOURCE_OS_MAC_INPUTMAC_H

#include <Carbon/Carbon.h>

struct OSMACEVENT {
  UInt32    eventClass;
  UInt32    eventKind;
  void     *event;
  void     *window;
  HICommand command;
};

typedef OSStatus (*OSMACEVENTHANDLERPROC)(OSMACEVENT *event, void *userData);

void     OsMacApplicationStart();
OSStatus OsMacReceiveNextEvent(EventTimeout timeout);
OSStatus OsMacInstallEventHandler(
    void *window, OSMACEVENTHANDLERPROC handler, UInt32 numTypes, const EventTypeSpec *list, void *userData, void **outRef
);
void     OsMacRemoveEventHandler(void *handlerRef);
OSStatus OsMacGetEventParameter(
    OSMACEVENT *event, EventParamName name, EventParamType desiredType, EventParamType *actualType, ByteCount bufferSize,
    ByteCount *actualSize, void *data
);
void     OsMacGlobalToLocalPoint(void *window, HIPoint *point);
void     OsMacLocalToGlobalPoint(void *window, HIPoint *point);

void OsInputRegisterHICommandHandler(UInt32 command, OSStatus (*handler)(OSMACEVENT *event, UInt32 command));
void OsInputInstallGameWindowHandlers(void *window);
void OsInputRemoveGameWindowHandlers(void *window);

#endif
