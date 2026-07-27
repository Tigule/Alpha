#ifndef WOW_SOURCE_OBJECTALLOC_OBJECTALLOCTEMPLATE_H
#define WOW_SOURCE_OBJECTALLOC_OBJECTALLOCTEMPLATE_H

#include <storm.h>

int ObjectAlloc(unsigned int heapId, unsigned int *memHandle);
unsigned int ObjectAllocAddHeap(unsigned int objectSize, unsigned int objsPerBlock, const char *name);
void ObjectAllocDestroy();
void ObjectAllocInitialize();
unsigned int ObjectAllocUsage(unsigned int heapId);
void ObjectFree(unsigned int memHandle);
void *ObjectPtr(unsigned int memHandle);

class TObjectAllocMemHandle {
 public:
  unsigned int GetMemHandle() {
    return memHandle;
  }

  void SetMemHandle(unsigned int handle) {
    memHandle = handle;
  }

  unsigned int memHandle;
};

template <class T>
class TObjectAlloc {
 public:
  TObjectAlloc(const char *heapName, unsigned int objectsPerBlock) : m_ID(ObjectAllocAddHeap(sizeof(T), objectsPerBlock, heapName)) {
  }

  T *New() {
    unsigned int memHandle;
    T           *obj = 0;

    if (ObjectAlloc(m_ID, &memHandle)) {
      obj = static_cast<T *>(ObjectPtr(memHandle));
      ASSERT(obj);
      obj->SetMemHandle(memHandle);
    }

    return obj;
  }

  void Free(T *obj) {
    ObjectFree(obj->GetMemHandle());
  }

 private:
  unsigned int m_ID;
};

#endif
