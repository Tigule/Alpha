#include <Console/ConsoleClient.h>
#include <Console/ConsoleCommand.h>
#include <ObjectAlloc/ObjectAllocTemplate.h>

#include <storm.h>
#include <stpl.h>

enum {
  MAX_HEAPS = 32
};

int CCommand_HeapUsage(const char *command, const char *arguments);

class CObjectHeap {
 public:
  CObjectHeap() : m_obj(0), m_indexStack(0), m_allocated(0), m_bytes(0) {
  }

  CObjectHeap(const CObjectHeap &heap);
  ~CObjectHeap();

  int   Allocate(unsigned int objSize, unsigned int heapObjects);
  int   New(unsigned int objSize, unsigned int heapObjects, unsigned int *index);
  void  Delete(unsigned int index, unsigned int objSize, unsigned int heapObjects);
  void *Ptr(unsigned int index, unsigned int objSize, unsigned int heapObjects);

 private:
  CObjectHeap &operator=(const CObjectHeap &heap);

  friend class CObjectHeapList;

  mutable void         *m_obj;
  mutable unsigned int *m_indexStack;
  unsigned int          m_allocated;
  mutable unsigned int  m_bytes;
};

class CObjectHeapList {
 public:
  CObjectHeapList() : m_objSize(0), m_objsPerBlock(1024), m_numFullHeaps(0) {
  }

  int          New(unsigned int *index);
  void        *Ptr(unsigned int index);
  void         Delete(unsigned int index);
  unsigned int BlocksAllocated() const;

 private:
  friend int CCommand_HeapUsage(const char *command, const char *arguments);
  friend unsigned int ObjectAllocAddHeap(unsigned int objectSize, unsigned int objsPerBlock, const char *name);

  TSGrowableArray<CObjectHeap> m_heaps;
  unsigned int                 m_objSize;
  unsigned int                 m_objsPerBlock;
  unsigned int                 m_numFullHeaps;
  char                         m_heapName[80];
};

struct OBJALLOCGLOBALS {
  TSGrowableArray<CObjectHeapList> objects;
};

static OBJALLOCGLOBALS s_globals;
static SCritSect       s_globalsLock;

unsigned int CObjectHeapList::BlocksAllocated() const {
  unsigned int numHeaps = m_heaps.Count();
  unsigned int blocks = 0;
  unsigned int heap;

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

int CObjectHeap::Allocate(unsigned int objSize, unsigned int heapObjects) {
  unsigned int index;

  ASSERT(m_obj == 0);

  m_obj = ALLOC(heapObjects * (objSize + sizeof(unsigned int)));
  m_indexStack = reinterpret_cast<unsigned int *>(static_cast<char *>(m_obj) + heapObjects * objSize);
  m_bytes = heapObjects * objSize;

  for (index = 0; index < heapObjects; ++index) {
    m_indexStack[index] = index;
  }

  m_allocated = 0;
  return m_obj != 0;
}

int CObjectHeapList::New(unsigned int *index) {
  unsigned int heapIndex;
  CObjectHeap *heap;

  ASSERT(index);

  if (m_heaps.Count() == m_numFullHeaps) {
    heapIndex = m_heaps.Count();
    heap = m_heaps.New();
    if (!heap->Allocate(m_objSize, m_objsPerBlock)) {
      return 0;
    }
  } else {
    unsigned int largestUsage = 0;
    unsigned int fullestHeap = 0;
    unsigned int currentHeap;

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

void CObjectHeapList::Delete(unsigned int index) {
  unsigned int heap = index / m_objsPerBlock;
  unsigned int object = index % m_objsPerBlock;

  if (m_heaps[heap].m_allocated == m_objsPerBlock) {
    --m_numFullHeaps;
  }
  m_heaps[heap].Delete(object, m_objSize, m_objsPerBlock);
}

void *CObjectHeapList::Ptr(unsigned int index) {
  unsigned int heap = index / m_objsPerBlock;
  unsigned int object = index % m_objsPerBlock;

  return m_heaps[heap].Ptr(object, m_objSize, m_objsPerBlock);
}

int CObjectHeap::New(unsigned int objSize, unsigned int heapObjects, unsigned int *index) {
  ASSERT(index);
  ASSERT(m_obj != 0);

  if (m_allocated >= heapObjects) {
    return 0;
  }

  *index = m_indexStack[m_allocated];
  ++m_allocated;
  return 1;
}

void CObjectHeap::Delete(unsigned int index, unsigned int objSize, unsigned int heapObjects) {
  ASSERT(m_obj != 0);
  ASSERT(index < heapObjects);
  ASSERT(m_allocated);

  --m_allocated;
  m_indexStack[m_allocated] = index;
}

void *CObjectHeap::Ptr(unsigned int index, unsigned int objSize, unsigned int heapObjects) {
  ASSERT(m_obj != 0);
  ASSERT(index < heapObjects);

  if (objSize * index >= m_bytes) {
    FATALERROR(("CObjectHeap::Ptr(): index(%u), objSize(%u), m_bytes(%u)", index, objSize, m_bytes));
  }

  return static_cast<char *>(m_obj) + objSize * index;
}

int CCommand_HeapUsage(const char *command, const char *arguments) {
  OBJALLOCGLOBALS *globals = &s_globals;
  unsigned int     numHeaps;
  unsigned int     totalBytes = 0;
  unsigned int     totalHeapBytes = 0;
  unsigned int     heap;

  s_globalsLock.Enter();

  numHeaps = globals->objects.Count();
  ConsoleWriteA("%u Heaps in use:", HIGHLIGHT_COLOR, numHeaps);
  for (heap = 0; heap < numHeaps; ++heap) {
    unsigned int objSize = globals->objects[heap].m_objSize;

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

unsigned int ObjectAllocAddHeap(unsigned int objectSize, unsigned int objsPerBlock, const char *name) {
  OBJALLOCGLOBALS *globals = &s_globals;
  unsigned int     heapId;
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

unsigned int ObjectAllocUsage(unsigned int heapId) {
  OBJALLOCGLOBALS *globals = &s_globals;
  unsigned int     usage;

  s_globalsLock.Enter();
  FATALASSERT(heapId < globals->objects.Count());

  usage = globals->objects[heapId].BlocksAllocated();
  s_globalsLock.Leave();
  return usage;
}

int ObjectAlloc(unsigned int heapId, unsigned int *memHandle) {
  OBJALLOCGLOBALS *globals = &s_globals;
  unsigned int     index;

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

void ObjectFree(unsigned int memHandle) {
  OBJALLOCGLOBALS *globals = &s_globals;
  unsigned int     heapId;
  unsigned int     index;

  s_globalsLock.Enter();
  heapId = memHandle >> 27;
  index = memHandle & 0x07FFFFFF;
  globals->objects[heapId].Delete(index);
  s_globalsLock.Leave();
}

void *ObjectPtr(unsigned int memHandle) {
  OBJALLOCGLOBALS *globals = &s_globals;
  unsigned int     heapId;
  unsigned int     index;
  void            *object;

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
