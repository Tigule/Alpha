#include <Base/Base.h>

#include "WDataStore.h"

#include "ObjectAlloc/ObjectAllocTemplate.h"

#include <new>
#include <string.h>

enum {
  SMALL_BUFFER_SIZE = 0x300,
  LARGE_BUFFER_SIZE = 0x4000
};

template <UINT SIZE>
class WDataStoreBuffer : public TObjectAllocMemHandle {
 public:
  int GetSize();

  BYTE buf[SIZE];
};

static TObjectAlloc<WDataStoreBuffer<SMALL_BUFFER_SIZE> > *s_smallHeap;
static TObjectAlloc<WDataStoreBuffer<LARGE_BUFFER_SIZE> > *s_largeHeap;
static BYTE                                                s_heapsInitialized;

void WDataStore::StaticInitialize() {
  if (s_heapsInitialized) {
    return;
  }

  s_smallHeap = new (ALLOC(sizeof(TObjectAlloc<WDataStoreBuffer<SMALL_BUFFER_SIZE> >)))
      TObjectAlloc<WDataStoreBuffer<SMALL_BUFFER_SIZE> >("WDataStoreSmallBuffer", 1024);
  s_largeHeap = new (ALLOC(sizeof(TObjectAlloc<WDataStoreBuffer<LARGE_BUFFER_SIZE> >)))
      TObjectAlloc<WDataStoreBuffer<LARGE_BUFFER_SIZE> >("WDataStoreLargeBuffer", 32);
  s_heapsInitialized = 1;
}

void WDataStore::StaticDestroy() {
  if (s_smallHeap) {
    DEL(s_smallHeap);
  }

  if (s_largeHeap) {
    DEL(s_largeHeap);
  }

  s_heapsInitialized = 0;
}

void WDataStore::InternalInitialize(BYTE *&data, UINT &base, UINT &alloc) {
  m_bufferObj = 0;
  alloc = 0;
  data = 0;
  base = 0;
}

void WDataStore::InternalDestroy(BYTE *&data, UINT &base, UINT &alloc) {
  if (m_bufferObj) {
    if (alloc == SMALL_BUFFER_SIZE) {
      s_smallHeap->Free(static_cast<WDataStoreBuffer<SMALL_BUFFER_SIZE> *>(m_bufferObj));
    } else if (alloc == LARGE_BUFFER_SIZE) {
      s_largeHeap->Free(static_cast<WDataStoreBuffer<LARGE_BUFFER_SIZE> *>(m_bufferObj));
    } else {
      ASSERT(0);
    }
  } else if (data) {
    Free(data, 0, 0);
  }

  data = 0;
  base = 0;
  alloc = 0;
}

int WDataStore::InternalFetchWrite(UINT pos, UINT bytes, BYTE *&data, UINT &base, UINT &alloc, LPCSTR fileName, int lineNumber) {
  if (!s_heapsInitialized) {
    StaticInitialize();
  }

  ASSERT(alloc == 0 || alloc == SMALL_BUFFER_SIZE || alloc >= LARGE_BUFFER_SIZE);

  UINT newAlloc = (pos + bytes + 0xFF) & 0xFFFFFF00;
  UINT oldAlloc = alloc;
  if (newAlloc < oldAlloc) {
    return 1;
  }

  LPVOID oldBufferObj = m_bufferObj;

  if (newAlloc <= SMALL_BUFFER_SIZE) {
    WDataStoreBuffer<SMALL_BUFFER_SIZE> *obj = s_smallHeap->New();
    if (data) {
      memcpy(obj->buf, data, oldAlloc);
    }

    data = obj->buf;
    alloc = SMALL_BUFFER_SIZE;
    m_bufferObj = obj;
  } else if (newAlloc <= LARGE_BUFFER_SIZE) {
    WDataStoreBuffer<LARGE_BUFFER_SIZE> *obj = s_largeHeap->New();
    if (data) {
      memcpy(obj->buf, data, oldAlloc);
    }

    data = obj->buf;
    alloc = LARGE_BUFFER_SIZE;
    m_bufferObj = obj;
  } else {
    if (!oldBufferObj) {
      data = Realloc(data, newAlloc, fileName, lineNumber);
    } else {
      BYTE *newData = Alloc(newAlloc, fileName, lineNumber);
      memcpy(newData, data, oldAlloc);
      data = newData;
      m_bufferObj = 0;
    }

    alloc = newAlloc;
  }

  if (oldBufferObj) {
    if (oldAlloc == SMALL_BUFFER_SIZE) {
      s_smallHeap->Free(static_cast<WDataStoreBuffer<SMALL_BUFFER_SIZE> *>(oldBufferObj));
    } else if (oldAlloc == LARGE_BUFFER_SIZE) {
      s_largeHeap->Free(static_cast<WDataStoreBuffer<LARGE_BUFFER_SIZE> *>(oldBufferObj));
    } else {
      ASSERT(0);
    }
  }

  return 1;
}

LPVOID WDataStore::AllocBuffer(UINT size) {
  if (size <= SMALL_BUFFER_SIZE) {
    return s_smallHeap->New()->buf;
  }

  if (size <= LARGE_BUFFER_SIZE) {
    return s_largeHeap->New()->buf;
  }

  return ALLOC(size);
}

void WDataStore::FreeBuffer(LPVOID buffer, UINT size) {
  if (size <= SMALL_BUFFER_SIZE) {
    WDataStoreBuffer<SMALL_BUFFER_SIZE> *obj = CONTAINING_RECORD(buffer, WDataStoreBuffer<SMALL_BUFFER_SIZE>, buf);
    s_smallHeap->Free(obj);
    return;
  }

  if (size <= LARGE_BUFFER_SIZE) {
    WDataStoreBuffer<LARGE_BUFFER_SIZE> *obj = CONTAINING_RECORD(buffer, WDataStoreBuffer<LARGE_BUFFER_SIZE>, buf);
    s_largeHeap->Free(obj);
    return;
  }

  FREE(buffer);
}
