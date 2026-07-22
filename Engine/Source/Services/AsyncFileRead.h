#ifndef ENGINE_SOURCE_SERVICES_ASYNCFILEREAD_H
#define ENGINE_SOURCE_SERVICES_ASYNCFILEREAD_H

#include <stpl.h>

class CAsyncObject {
 public:
  SFile       *file;
  unsigned int offset;
  void        *buffer;
  unsigned int size;
  void        *userArg;
  void(__fastcall *userPostloadCallback)(void *);
  SCritSect           *critSect;
  unsigned char        isLoaded;
  unsigned char        canReorder;
  TSLink<CAsyncObject> link;
};

void __fastcall          AsyncFileReadInitialize();
void __fastcall          AsyncFileReadDestroy();
void __fastcall          AsyncFileReadAddHandler(void(__fastcall *handler)());
CAsyncObject *__fastcall AsyncFileReadCreateObject();
void __fastcall          AsyncFileReadDestroyObject(CAsyncObject *object);
void __fastcall          AsyncFileReadObject(CAsyncObject *object);
void __fastcall          AsyncFileReadWait(CAsyncObject *object);
void __fastcall          AsyncFileReadWaitAll();
bool __fastcall          AsyncFileReadIsReading();

#endif
