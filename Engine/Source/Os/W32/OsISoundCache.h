#pragma once

#include <stpl.h>

struct SoundFileInstance {
  BYTE inUse;
  int  currentOffset;
};

struct SoundFileObject {
  char              filename[MAX_PATH];
  UINT              hash;
  SFile            *file;
  UINT              baseHandle;
  UINT              size;
  UINT              openInstances;
  UINT              bigFileCacheBlockOffset;
  SoundFileInstance instances[16];
  LINKDECLEX(SoundFileObject, link);
};

struct SoundFileObjectCacheNode : public TSHashObject<SoundFileObjectCacheNode, HASHKEY_NONE> {
  SoundFileObject *object;
};

struct SoundFileDataCacheBlock : public TSHashObject<SoundFileDataCacheBlock, HASHKEY_LONGLONG> {
  LINKDECLEX(SoundFileDataCacheBlock, link);
  BYTE data[4096];
};

class SoundFileCache {
 public:
  static void Initialize(int cacheSizeMB);
  static void Shutdown();

  static UINT __stdcall Open(LPCSTR filename);
  static int __stdcall  Read(LPVOID buffer, int size, UINT handle);
  static int __stdcall  Seek(UINT handle, int pos, signed char mode);
  static int __stdcall  Tell(UINT handle);
  static void __stdcall Close(UINT handle);
};
