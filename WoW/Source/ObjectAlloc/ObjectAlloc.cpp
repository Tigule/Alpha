#include <Base/Base.h>

#include <Console/ConsoleClient.h>
#include <Console/ConsoleCommand.h>
#include <ObjectAlloc/ObjectAllocTemplate.h>

#include <storm.h>
#include <stpl.h>

enum {
  MAX_HEAPS = 32
};

static BOOL CCommand_HeapUsage(LPCSTR command, LPCSTR arguments);

class CObjectHeap {
 public:
  CObjectHeap() : m_obj(0), m_indexStack(0), m_allocated(0), m_bytes(0) {
  }

  CObjectHeap(const CObjectHeap &heap);

  ~CObjectHeap() {
    if (m_obj) {
      FREE(m_obj);
    }
  }

  BOOL   Allocate(UINT objSize, UINT heapObjects);
  BOOL   New(UINT objSize, UINT heapObjects, UINT *index);
  void   Delete(UINT index, UINT objSize, UINT heapObjects);
  LPVOID Ptr(UINT index, UINT objSize, UINT heapObjects);

  UINT BlocksAllocated() const {
    return m_allocated;
  }

  BOOL IsFull(UINT heapObjects) const {
    return m_allocated == heapObjects;
  }

 private:
  CObjectHeap &operator=(const CObjectHeap &heap);

  mutable LPVOID m_obj;
  mutable UINT  *m_indexStack;
  UINT           m_allocated;
  mutable UINT   m_bytes;
};

class CObjectHeapList {
 public:
  CObjectHeapList() : m_objSize(0), m_objsPerBlock(1024), m_numFullHeaps(0) {
  }

  void SetObjectSize(UINT objSize) {
    m_objSize = objSize;
  }

  UINT GetObjectSize() const {
    return m_objSize;
  }

  void SetObjectsPerBlock(UINT objsPerBlock) {
    m_objsPerBlock = objsPerBlock;
  }

  UINT GetObjectsPerBlock() const {
    return m_objsPerBlock;
  }

  UINT GetHeapBytes() const {
    return m_objsPerBlock * m_objSize * TotalHeaps();
  }

  UINT GetBytesAllocated() const {
    return GetObjectSize() * BlocksAllocated();
  }

  BOOL   New(UINT *index);
  LPVOID Ptr(UINT index);
  void   Delete(UINT index);

  UINT HeapsAvailable() const {
    return m_heaps.Count() - m_numFullHeaps;
  }

  UINT BlocksAllocated() const;

  UINT TotalHeaps() const {
    return m_heaps.Count();
  }

  void SetName(LPCSTR name) {
    SStrCopy(m_heapName, name, sizeof(m_heapName));
  }

  LPCSTR GetName() const {
    return m_heapName;
  }

 private:
  BOOL IsHeapFull(UINT heap) const {
    return m_heaps[heap].BlocksAllocated() == m_objsPerBlock;
  }

  TSGrowableArray<CObjectHeap> m_heaps;
  UINT                         m_objSize;
  UINT                         m_objsPerBlock;
  UINT                         m_numFullHeaps;
  char                         m_heapName[80];
};

struct OBJALLOCGLOBALS {
  TSGrowableArray<CObjectHeapList> objects;
};

static OBJALLOCGLOBALS s_globals;
static SCritSect       s_globalsLock;

BOOL CObjectHeapList::New(UINT *index) {
  CObjectHeap *heap;
  UINT         fullestHeap;
  UINT         largestUsage;

  ASSERT(index);

  if (!HeapsAvailable()) {
    fullestHeap = m_heaps.Count();
    heap = m_heaps.New();
    if (!heap->Allocate(m_objSize, m_objsPerBlock)) {
      return 0;
    }
  } else {
    UINT currentHeap = m_heaps.Count();

    largestUsage = 0;
    fullestHeap = 0;
    while (currentHeap--) {
      if (m_heaps[currentHeap].BlocksAllocated() >= largestUsage && !IsHeapFull(currentHeap)) {
        largestUsage = m_heaps[currentHeap].BlocksAllocated();
        fullestHeap = currentHeap;
      }
    }

    heap = &m_heaps[fullestHeap];
  }

  if (!heap->New(m_objSize, m_objsPerBlock, index)) {
    return 0;
  }

  *index += fullestHeap * m_objsPerBlock;
  if (heap->IsFull(m_objsPerBlock)) {
    ++m_numFullHeaps;
  }
  return 1;
}

LPVOID CObjectHeapList::Ptr(UINT index) {
  UINT heap = index / m_objsPerBlock;
  UINT object = index % m_objsPerBlock;

  return m_heaps[heap].Ptr(object, m_objSize, m_objsPerBlock);
}

void CObjectHeapList::Delete(UINT index) {
  UINT heap = index / m_objsPerBlock;
  UINT object = index % m_objsPerBlock;

  if (IsHeapFull(heap)) {
    --m_numFullHeaps;
  }
  m_heaps[heap].Delete(object, m_objSize, m_objsPerBlock);
}

UINT CObjectHeapList::BlocksAllocated() const {
  UINT numHeaps = m_heaps.Count();
  UINT blocks = 0;
  UINT heap;

  for (heap = 0; heap < numHeaps; ++heap) {
    blocks += m_heaps[heap].BlocksAllocated();
  }
  return blocks;
}

CObjectHeap::CObjectHeap(const CObjectHeap &heap) {
  m_bytes = heap.m_bytes;
  heap.m_bytes = 0;
  m_obj = heap.m_obj;
  m_indexStack = heap.m_indexStack;
  heap.m_obj = 0;
  heap.m_indexStack = 0;
  m_allocated = heap.m_allocated;
}

CObjectHeap &CObjectHeap::operator=(const CObjectHeap &heap) {
  return *this;
}

BOOL CObjectHeap::Allocate(UINT objSize, UINT heapObjects) {
  UINT index;

  ASSERT(m_obj == 0);

  m_obj = ALLOC(heapObjects * (objSize + sizeof(UINT)));
  m_indexStack = (UINT *)((char *)m_obj + heapObjects * objSize);
  m_bytes = heapObjects * objSize;

  for (index = 0; index < heapObjects; ++index) {
    m_indexStack[index] = index;
  }

  m_allocated = 0;
  return m_obj != 0;
}

BOOL CObjectHeap::New(UINT objSize, UINT heapObjects, UINT *index) {
  ASSERT(index);
  ASSERT(m_obj != 0);

  if (m_allocated >= heapObjects) {
    return 0;
  }

  *index = m_indexStack[m_allocated];
  ++m_allocated;
  return 1;
}

void CObjectHeap::Delete(UINT index, UINT objSize, UINT heapObjects) {
  ASSERT(m_obj != 0);
  if (!(index < heapObjects)) {
    SErrDisplayErrorFmt(
        STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
        isprint((index >> 24) & 0xFF) && isprint((index >> 16) & 0xFF) && isprint((index >> 8) & 0xFF) && isprint(index & 0xFF)
            ? "\"%s\", %s = %ld (0x%08X, '%c%c%c%c')"
            : "\"%s\", %s = %ld (0x%08X)",
        "(index < heapObjects)", "index", index, index, (index >> 24) & 0xFF, (index >> 16) & 0xFF, (index >> 8) & 0xFF, index & 0xFF
    );
  }
  ASSERT(m_allocated);

  --m_allocated;
  m_indexStack[m_allocated] = index;
}

LPVOID CObjectHeap::Ptr(UINT index, UINT objSize, UINT heapObjects) {
  ASSERT(m_obj != 0);
  if (!(index < heapObjects)) {
    SErrDisplayErrorFmt(
        STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
        isprint((index >> 24) & 0xFF) && isprint((index >> 16) & 0xFF) && isprint((index >> 8) & 0xFF) && isprint(index & 0xFF)
            ? "\"%s\", %s = %ld (0x%08X, '%c%c%c%c')"
            : "\"%s\", %s = %ld (0x%08X)",
        "(index < heapObjects)", "index", index, index, (index >> 24) & 0xFF, (index >> 16) & 0xFF, (index >> 8) & 0xFF, index & 0xFF
    );
  }

  if (objSize * index >= m_bytes) {
    FATALERROR(("CObjectHeap::Ptr(): index(%u), objSize(%u), m_bytes(%u)", index, objSize, m_bytes));
  }

  return (char *)m_obj + objSize * index;
}

void ObjectAllocInitialize() {
  ConsoleCommandRegister("HeapUsage", CCommand_HeapUsage, GAME, 0);
}

static BOOL CCommand_HeapUsage(LPCSTR command, LPCSTR arguments) {
  OBJALLOCGLOBALS *globals = &s_globals;

  s_globalsLock.Enter();

  UINT numHeaps = globals->objects.Count();
  UINT totalBytes = 0;
  UINT totalHeapBytes = 0;
  UINT heap;

  ConsoleWriteA("%u Heaps in use:", HIGHLIGHT_COLOR, numHeaps);
  for (heap = 0; heap < numHeaps; ++heap) {
    ConsoleWriteA(
        "    \"%s\" (%u byte blocks): %u blocks allocated", HIGHLIGHT_COLOR, globals->objects[heap].GetName(), globals->objects[heap].GetObjectSize(),
        globals->objects[heap].BlocksAllocated()
    );

    totalBytes += globals->objects[heap].GetBytesAllocated();
    totalHeapBytes += globals->objects[heap].GetHeapBytes();
  }

  ConsoleWriteA("%u total object bytes used, %u bytes allocated for heaps", HIGHLIGHT_COLOR, totalBytes, totalHeapBytes);

  s_globalsLock.Leave();
  return 1;
}

UINT ObjectAllocAddHeap(UINT objectSize, UINT objsPerBlock, LPCSTR name) {
  OBJALLOCGLOBALS *globals = &s_globals;
  UINT             heapId;
  CObjectHeapList *heap;

  VALIDATEBEGIN;
  VALIDATE(objectSize > 0);
  VALIDATEEND;

  s_globalsLock.Enter();
  ASSERT(globals->objects.Count() < MAX_HEAPS);

  heapId = globals->objects.Count();
  heap = globals->objects.New();
  heap->SetObjectSize(objectSize);
  heap->SetObjectsPerBlock(objsPerBlock);
  heap->SetName(name);

  s_globalsLock.Leave();
  return heapId;
}

UINT ObjectAllocUsage(UINT heapId) {
  OBJALLOCGLOBALS *globals = &s_globals;
  UINT             usage;

  s_globalsLock.Enter();
  VALIDATEBEGIN;
  VALIDATE(heapId < globals->objects.Count());
  VALIDATEEND;

  usage = globals->objects[heapId].BlocksAllocated();
  s_globalsLock.Leave();
  return usage;
}

BOOL ObjectAlloc(UINT heapId, UINT *memHandle) {
  OBJALLOCGLOBALS *globals = &s_globals;
  UINT             index;

  VALIDATEBEGIN;
  VALIDATE(memHandle);

  *memHandle = 0;
  s_globalsLock.Enter();
  VALIDATE(heapId < globals->objects.Count());
  VALIDATEEND;

  if (!globals->objects[heapId].New(&index)) {
    s_globalsLock.Leave();
    return 0;
  }

  s_globalsLock.Leave();
  *memHandle = index | (heapId << 27);
  return 1;
}

void ObjectFree(UINT memHandle) {
  OBJALLOCGLOBALS *globals = &s_globals;
  UINT             heapId;

  s_globalsLock.Enter();
  heapId = memHandle >> 27;
  globals->objects[heapId].Delete(memHandle & 0x07FFFFFF);
  s_globalsLock.Leave();
}

LPVOID ObjectPtr(UINT memHandle) {
  OBJALLOCGLOBALS *globals = &s_globals;
  UINT             heapId;
  LPVOID           object;

  s_globalsLock.Enter();
  heapId = memHandle >> 27;
  object = globals->objects[heapId].Ptr(memHandle & 0x07FFFFFF);
  s_globalsLock.Leave();
  return object;
}

void ObjectAllocDestroy() {
  ConsoleCommandUnregister("HeapUsage");

  s_globalsLock.Enter();
  s_globals.objects.Clear();
  s_globalsLock.Leave();
}
