#ifndef WOW_SOURCE_OBJECT_MIRROR_H
#define WOW_SOURCE_OBJECT_MIRROR_H

struct ObjDataDescriptor {
  const char  *debugName;
  unsigned int fieldName;
  unsigned int fieldSize;
  unsigned int fieldMirrorType;
  unsigned int fieldMirrorFlags;
};

void MirrorInitialize();

#endif
