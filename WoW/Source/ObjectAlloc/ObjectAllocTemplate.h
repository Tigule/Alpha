#ifndef WOW_SOURCE_OBJECTALLOC_OBJECTALLOCTEMPLATE_H
#define WOW_SOURCE_OBJECTALLOC_OBJECTALLOCTEMPLATE_H

#include <storm.h>

BOOL   ObjectAlloc(UINT heapId, UINT *memHandle);
UINT   ObjectAllocAddHeap(UINT objectSize, UINT objsPerBlock, LPCSTR name);
void   ObjectAllocDestroy();
void   ObjectAllocInitialize();
UINT   ObjectAllocUsage(UINT heapId);
void   ObjectFree(UINT memHandle);
LPVOID ObjectPtr(UINT memHandle);

class TObjectAllocMemHandle {
 public:
  UINT memHandle;

  UINT GetMemHandle() {
    return memHandle;
  }

  void SetMemHandle(UINT handle) {
    memHandle = handle;
  }

};

template <class T>
class TObjectAlloc {
  UINT m_ID;

 public:
  TObjectAlloc(LPCSTR heapName, UINT objectsPerBlock) : m_ID(ObjectAllocAddHeap(sizeof(T), objectsPerBlock, heapName)) {
  }

  T *New() {
    UINT memHandle;

    if (!ObjectAlloc(m_ID, &memHandle)) {
      return 0;
    }

    T *obj = static_cast<T *>(ObjectPtr(memHandle));
    ASSERT(obj);
    obj->SetMemHandle(memHandle);
    return obj;
  }

  void Free(T *obj) {
    ObjectFree(obj->GetMemHandle());
  }
};

#endif
