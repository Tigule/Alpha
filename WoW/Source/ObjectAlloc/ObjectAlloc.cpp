#include <Base/Base.h>

#include <Console/ConsoleClient.h>
#include <Console/ConsoleCommand.h>
#include <ObjectAlloc/ObjectAllocTemplate.h>

#include <storm.h>
#include <stpl.h>

enum {
  MAX_HEAPS = 32
};

BOOL CCommand_HeapUsage(LPCSTR command, LPCSTR arguments);

class CObjectHeap {
 public:
  CObjectHeap() : m_obj(0), m_indexStack(0), m_allocated(0), m_bytes(0) {
  }

  CObjectHeap(const CObjectHeap &heap);
  ~CObjectHeap();

  BOOL   Allocate(UINT objSize, UINT heapObjects);
  BOOL   New(UINT objSize, UINT heapObjects, UINT *index);
  void   Delete(UINT index, UINT objSize, UINT heapObjects);
  LPVOID Ptr(UINT index, UINT objSize, UINT heapObjects);
  UINT   BlocksAllocated() const;
  BOOL   IsFull(UINT heapObjects) const;

 private:
  CObjectHeap &operator=(const CObjectHeap &heap);

  friend class CObjectHeapList;

  mutable LPVOID m_obj;
  mutable UINT  *m_indexStack;
  UINT           m_allocated;
  mutable UINT   m_bytes;
};

class CObjectHeapList {
 public:
  CObjectHeapList() : m_objSize(0), m_objsPerBlock(1024), m_numFullHeaps(0) {
  }

  void   SetObjectSize(UINT objSize);
  UINT   GetObjectSize() const;
  void   SetObjectsPerBlock(UINT objsPerBlock);
  UINT   GetObjectsPerBlock() const;
  UINT   GetHeapBytes() const;
  UINT   GetBytesAllocated() const;
  BOOL   New(UINT *index);
  LPVOID Ptr(UINT index);
  void   Delete(UINT index);
  UINT   HeapsAvailable() const;
  UINT   BlocksAllocated() const;
  UINT   TotalHeaps() const;
  void   SetName(LPCSTR name);
  LPCSTR GetName() const;

 private:
  friend BOOL CCommand_HeapUsage(LPCSTR command, LPCSTR arguments);
  friend UINT ObjectAllocAddHeap(UINT objectSize, UINT objsPerBlock, LPCSTR name);

  BOOL IsHeapFull(UINT heap) const;

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

UINT CObjectHeapList::BlocksAllocated() const {
  UINT numHeaps = m_heaps.Count();
  UINT blocks = 0;
  UINT heap;

  for (heap = 0; heap < numHeaps; ++heap) {
    blocks += m_heaps[heap].m_allocated;
  }
  return blocks;
}

CObjectHeap::~CObjectHeap() {
  if (m_obj) {
    FREE(m_obj);
  }
}

CObjectHeap &CObjectHeap::operator=(const CObjectHeap &heap) {
  return *this;
}

BOOL CObjectHeap::Allocate(UINT objSize, UINT heapObjects) {
  UINT index;

  ASSERT(m_obj == 0);

  m_obj = ALLOC(heapObjects * (objSize + sizeof(UINT)));
  m_indexStack = reinterpret_cast<UINT *>(static_cast<char *>(m_obj) + heapObjects * objSize);
  m_bytes = heapObjects * objSize;

  for (index = 0; index < heapObjects; ++index) {
    m_indexStack[index] = index;
  }

  m_allocated = 0;
  return m_obj != 0;
}

BOOL CObjectHeapList::New(UINT *index) {
  UINT         heapIndex;
  CObjectHeap *heap;

  ASSERT(index);

  if (m_heaps.Count() == m_numFullHeaps) {
    heapIndex = m_heaps.Count();
    heap = m_heaps.New();
    if (!heap->Allocate(m_objSize, m_objsPerBlock)) {
      return 0;
    }
  } else {
    UINT largestUsage = 0;
    UINT fullestHeap = 0;
    UINT currentHeap;

    for (currentHeap = m_heaps.Count(); currentHeap; --currentHeap) {
      if (m_heaps[currentHeap - 1].m_allocated >= largestUsage && m_heaps[currentHeap - 1].m_allocated != m_objsPerBlock) {
        largestUsage = m_heaps[currentHeap - 1].m_allocated;
        fullestHeap = currentHeap - 1;
      }
    }

    heapIndex = fullestHeap;
    heap = &m_heaps[fullestHeap];
  }

  if (!heap->New(m_objSize, m_objsPerBlock, index)) {
    return 0;
  }

  *index += heapIndex * m_objsPerBlock;
  if (heap->m_allocated == m_objsPerBlock) {
    ++m_numFullHeaps;
  }
  return 1;
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

void CObjectHeapList::Delete(UINT index) {
  UINT heap = index / m_objsPerBlock;
  UINT object = index % m_objsPerBlock;

  if (m_heaps[heap].m_allocated == m_objsPerBlock) {
    --m_numFullHeaps;
  }
  m_heaps[heap].Delete(object, m_objSize, m_objsPerBlock);
}

LPVOID CObjectHeapList::Ptr(UINT index) {
  UINT heap = index / m_objsPerBlock;
  UINT object = index % m_objsPerBlock;

  return m_heaps[heap].Ptr(object, m_objSize, m_objsPerBlock);
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
  ASSERT(index < heapObjects);
  ASSERT(m_allocated);

  --m_allocated;
  m_indexStack[m_allocated] = index;
}

LPVOID CObjectHeap::Ptr(UINT index, UINT objSize, UINT heapObjects) {
  ASSERT(m_obj != 0);
  ASSERT(index < heapObjects);

  if (objSize * index >= m_bytes) {
    FATALERROR(("CObjectHeap::Ptr(): index(%u), objSize(%u), m_bytes(%u)", index, objSize, m_bytes));
  }

  return static_cast<char *>(m_obj) + objSize * index;
}

BOOL CCommand_HeapUsage(LPCSTR command, LPCSTR arguments) {
  OBJALLOCGLOBALS *globals = &s_globals;
  UINT             numHeaps;
  UINT             totalBytes = 0;
  UINT             totalHeapBytes = 0;
  UINT             heap;

  s_globalsLock.Enter();

  numHeaps = globals->objects.Count();
  ConsoleWriteA("%u Heaps in use:", HIGHLIGHT_COLOR, numHeaps);
  for (heap = 0; heap < numHeaps; ++heap) {
    UINT objSize = globals->objects[heap].m_objSize;

    ConsoleWriteA(
        "    \"%s\" (%u byte blocks): %u blocks allocated", HIGHLIGHT_COLOR, globals->objects[heap].m_heapName, objSize,
        globals->objects[heap].BlocksAllocated()
    );

    totalBytes += globals->objects[heap].m_objSize * globals->objects[heap].BlocksAllocated();
    totalHeapBytes += globals->objects[heap].m_heaps.Count() * globals->objects[heap].m_objSize * globals->objects[heap].m_objsPerBlock;
  }

  ConsoleWriteA("%u total object bytes used, %u bytes allocated for heaps", HIGHLIGHT_COLOR, totalBytes, totalHeapBytes);

  s_globalsLock.Leave();
  return 1;
}

void ObjectAllocInitialize() {
  ConsoleCommandRegister("HeapUsage", CCommand_HeapUsage, GAME, 0);
}

UINT ObjectAllocAddHeap(UINT objectSize, UINT objsPerBlock, LPCSTR name) {
  OBJALLOCGLOBALS *globals = &s_globals;
  UINT             heapId;
  CObjectHeapList *heap;

  FATALASSERT(objectSize > 0);

  s_globalsLock.Enter();
  ASSERT(globals->objects.Count() < MAX_HEAPS);

  heapId = globals->objects.Count();
  heap = globals->objects.New();
  heap->m_objSize = objectSize;
  heap->m_objsPerBlock = objsPerBlock;
  SStrCopy(heap->m_heapName, name, sizeof(heap->m_heapName));

  s_globalsLock.Leave();
  return heapId;
}

UINT ObjectAllocUsage(UINT heapId) {
  OBJALLOCGLOBALS *globals = &s_globals;
  UINT             usage;

  s_globalsLock.Enter();
  FATALASSERT(heapId < globals->objects.Count());

  usage = globals->objects[heapId].BlocksAllocated();
  s_globalsLock.Leave();
  return usage;
}

BOOL ObjectAlloc(UINT heapId, UINT *memHandle) {
  OBJALLOCGLOBALS *globals = &s_globals;
  UINT             index;

  FATALASSERT(memHandle);

  *memHandle = 0;
  s_globalsLock.Enter();
  FATALASSERT(heapId < globals->objects.Count());

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
  UINT             index;

  s_globalsLock.Enter();
  heapId = memHandle >> 27;
  index = memHandle & 0x07FFFFFF;
  globals->objects[heapId].Delete(index);
  s_globalsLock.Leave();
}

LPVOID ObjectPtr(UINT memHandle) {
  OBJALLOCGLOBALS *globals = &s_globals;
  UINT             heapId;
  UINT             index;
  LPVOID           object;

  s_globalsLock.Enter();
  heapId = memHandle >> 27;
  index = memHandle & 0x07FFFFFF;
  object = globals->objects[heapId].Ptr(index);
  s_globalsLock.Leave();
  return object;
}

void ObjectAllocDestroy() {
  ConsoleCommandUnregister("HeapUsage");

  s_globalsLock.Enter();
  s_globals.objects.Clear();
  s_globalsLock.Leave();
}
