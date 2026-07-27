#ifndef ENGINE_SOURCE_SERVICES_ASYNCFILEREAD_H
#define ENGINE_SOURCE_SERVICES_ASYNCFILEREAD_H

#include <stpl.h>

class CAsyncObject {
 public:
  SFile       *file;
  unsigned long offset;
  void        *buffer;
  unsigned long size;
  void        *userArg;
  void(*userPostloadCallback)(void *);
  SCritSect           *critSect;
  unsigned char        isLoaded;
  unsigned char        canReorder;
  TSLink<CAsyncObject> link;
};

void AsyncFileReadInitialize();
void AsyncFileReadDestroy();
void AsyncFileReadAddHandler(void(*handler)());
CAsyncObject *AsyncFileReadCreateObject();
void AsyncFileReadDestroyObject(CAsyncObject *object);
void AsyncFileReadObject(CAsyncObject *object);
void AsyncFileReadWait(CAsyncObject *object);
void AsyncFileReadWaitAll();
bool AsyncFileReadIsReading();

#endif
