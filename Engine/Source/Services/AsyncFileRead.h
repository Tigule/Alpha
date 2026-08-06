#ifndef ENGINE_SOURCE_SERVICES_ASYNCFILEREAD_H
#define ENGINE_SOURCE_SERVICES_ASYNCFILEREAD_H

#include <stpl.h>

class CAsyncObject {
 public:
  SFile *file;
  DWORD  offset;
  LPVOID buffer;
  DWORD  size;
  LPVOID userArg;
  void (*userPostloadCallback)(LPVOID);
  SCritSect *critSect;
  BYTE       isLoaded;
  BYTE       canReorder;
  LINKDECLEX(CAsyncObject, link);
};

void          AsyncFileReadInitialize();
void          AsyncFileReadDestroy();
void          AsyncFileReadAddHandler(void (*handler)());
CAsyncObject *AsyncFileReadCreateObject();
void          AsyncFileReadDestroyObject(CAsyncObject *object);
void          AsyncFileReadObject(CAsyncObject *object);
void          AsyncFileReadWait(CAsyncObject *object);
void          AsyncFileReadWaitAll();
bool          AsyncFileReadIsReading();

#endif
