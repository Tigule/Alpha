#pragma once

#include <stpl.h>

struct SoundFileInstance {
  unsigned char inUse;
  int           currentOffset;
};

struct SoundFileObject {
  char                    filename[260];
  unsigned int            hash;
  SFile                  *file;
  unsigned int            baseHandle;
  unsigned int            size;
  unsigned int            openInstances;
  unsigned int            bigFileCacheBlockOffset;
  SoundFileInstance       instances[16];
  LINKDECLEX(SoundFileObject, link);
};

struct SoundFileObjectCacheNode : public TSHashObject<SoundFileObjectCacheNode, HASHKEY_NONE> {
  SoundFileObject *object;
};

struct SoundFileDataCacheBlock : public TSHashObject<SoundFileDataCacheBlock, HASHKEY_LONGLONG> {
  LINKDECLEX(SoundFileDataCacheBlock, link);
  unsigned char                   data[4096];
};

class SoundFileCache {
 public:
  static void Initialize(int cacheSizeMB);
  static void Shutdown();

  static unsigned int __stdcall Open(const char *filename);
  static int __stdcall          Read(void *buffer, int size, unsigned int handle);
  static int __stdcall          Seek(unsigned int handle, int pos, signed char mode);
  static int __stdcall          Tell(unsigned int handle);
  static void __stdcall         Close(unsigned int handle);
};
