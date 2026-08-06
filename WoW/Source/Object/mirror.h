#ifndef WOW_SOURCE_OBJECT_MIRROR_H
#define WOW_SOURCE_OBJECT_MIRROR_H

struct ObjDataDescriptor {
  LPCSTR debugName;
  UINT   fieldName;
  UINT   fieldSize;
  UINT   fieldMirrorType;
  UINT   fieldMirrorFlags;
};

void MirrorInitialize();

#endif
