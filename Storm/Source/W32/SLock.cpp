#include <storm.h>
#include "ISThread.h"

void __cdecl SOutputDebugString(const char *format, ...);

namespace SRWLock {
  struct SUNNLOCK {
    volatile long m_state;
    volatile long m_event;
  };

  struct SURWLOCK {
    SUNNLOCK      m_mutex;
    volatile long m_readerEvent;
  };

  void __fastcall IInitialize();
  void __fastcall IDestroy();
  void __fastcall IOsRWLockIncRef();
  void __fastcall IOsRWLockDecRef();
  int __fastcall  IWaitAndCheckForDeadlock(void *hevent);
  long __fastcall IAllocEvent(unsigned long evtype);
  void __fastcall IFreeEvent(unsigned long evtype, long event, int forcereset);
  int __fastcall  IWaitForEvent(unsigned long evtype, long event);
  void __fastcall ISetEvent(unsigned long evtype, long event);
  long __fastcall IEventIncRefCountOnly(long volatile *eventptr, long increment);
  long __fastcall IAllocEventOrIncRefCount(unsigned long evtype, long volatile *eventptr, long increment);
  int __fastcall  IDecRefCountAndFreeEvent(unsigned long evtype, long volatile *eventptr, long finalevent, long decrement);
  void __fastcall SUNNLockInitialize(SUNNLOCK volatile *sunnlock);
  void __fastcall SUNNLockDelete(SUNNLOCK volatile *sunnlock);
  void __fastcall SUNNLockEnter(SUNNLOCK volatile *sunnlock);
  int __fastcall  SUNNLockTryEnter(SUNNLOCK volatile *sunnlock);
  void __fastcall SUNNLockLeave(SUNNLOCK volatile *sunnlock);
  void __fastcall SURWLockInitialize(SURWLOCK volatile *surwlock);
  void __fastcall SURWLockDelete(SURWLOCK volatile *surwlock);
  void __fastcall SURWLockEnter(SURWLOCK volatile *surwlock, int forwriting);
  int __fastcall  SURWLockTryEnter(SURWLOCK volatile *surwlock, int forwriting);
  void __fastcall SURWLockLeave(SURWLOCK volatile *surwlock, int fromwriting);
}  // namespace SRWLock

struct CDebugLockData {
  DWORD           entry;
  CDebugLockData *prev;
  CDebugLockData *next;
};

struct CDebugLockEntry {
  DWORD       tick;
  DWORD       threadId;
  DWORD       next;
  const char *filename;
  DWORD       flags;
};

#define SRW_EVENT_TYPES      2
#define SRW_EVENT_COUNT      1024
#define SRW_EVENT_SLOT_COUNT 2048
#define SRW_EVENT_VALUE_STEP 0x00200000UL
#define SRW_EVENT_VALUE_MASK 0xFFE00000UL

static long          s_dupcount;
static CInitCritSect s_initCritsect;
static long          s_initCount;
static unsigned long s_spinCount = 1;
static void         *s_handle[SRW_EVENT_TYPES][SRW_EVENT_COUNT];
static long          s_free[SRW_EVENT_TYPES][SRW_EVENT_SLOT_COUNT];
static long          s_freeHead[SRW_EVENT_TYPES];
static long          s_freeTail[SRW_EVENT_TYPES];
static long          s_freeCount[SRW_EVENT_TYPES];

template <class T>
class CDebugLock {
 public:
  static void __fastcall Construct(CDebugLockData *lock);
  static void __fastcall Destruct(CDebugLockData *lock);
  static void __fastcall IEnter();
  static void __fastcall ILeave();
  static void __fastcall IDumpAllEntries();

  static void __fastcall IDumpEntries(CDebugLockData *lock);

  static DWORD __fastcall IClashingEntry(CDebugLockData *lock, DWORD threadId, int forwriting);
  static DWORD __fastcall IAddEntry(CDebugLockData *lock, DWORD threadId, int forwriting, const char *fileName, DWORD line);
  static DWORD __fastcall IDeleteEntry(CDebugLockData *lock, DWORD threadId, int fromwriting);
  static void __fastcall  IEnterEntry(DWORD e);

 private:
  static void __fastcall IRepairBadEntry(CDebugLockData *lock, DWORD e, CDebugLockEntry *eptr, const char *fileName, DWORD line);

  static CInitCritSect   s_critsect;
  static CDebugLockEntry s_entries[256];
  static DWORD           s_freeEntries;
  static CDebugLockData *s_locks;
};

void __fastcall Pause() {
}

void __fastcall SRWLock::IInitialize() {
  SYSTEM_INFO sysinfo;
  DWORD       type;
  void      **eventHandle;
  long       *slotBase;

  type = 0;
  eventHandle = &s_handle[0][0];
  slotBase = &s_free[0][0];
  do {
    DWORD index = 0;
    DWORD value = SRW_EVENT_VALUE_STEP;
    long *slot = slotBase;
    BOOL  manualReset = type == 1;

    do {
      *eventHandle = CreateEventA(NULL, manualReset, FALSE, NULL);
      if (!*eventHandle) {
        FATALERROR(("IInitialize: failed to create event [%u][%u]", type, index));
      }

      *slot = (long)value;
      ++index;
      ++eventHandle;
      value += SRW_EVENT_VALUE_STEP;
      ++slot;
    } while (index < SRW_EVENT_COUNT);

    slotBase += SRW_EVENT_SLOT_COUNT;
    s_freeTail[type] = SRW_EVENT_SLOT_COUNT - 1;
    s_freeHead[type] = SRW_EVENT_COUNT - 1;
    s_freeCount[type] = SRW_EVENT_COUNT;
    ++type;
  } while (slotBase < &s_free[0][0] + SRW_EVENT_TYPES * SRW_EVENT_SLOT_COUNT);

  memset(&sysinfo, 0, sizeof(sysinfo));
  GetSystemInfo(&sysinfo);
  s_spinCount = sysinfo.dwNumberOfProcessors > 1 ? 0xFA0 : 1;
  SServerInitialize();
}

void __fastcall SRWLock::IDestroy() {
  DWORD  type;
  void **eventHandle;
  long  *slotBase;

  type = 0;
  eventHandle = &s_handle[0][0];
  slotBase = &s_free[0][0];
  do {
    DWORD index = SRW_EVENT_COUNT;
    long *slot = slotBase;

    do {
      CloseHandle(*eventHandle);
      *eventHandle = NULL;
      *slot = 0;
      ++eventHandle;
      ++slot;
    } while (--index);

    s_freeTail[type] = 0;
    s_freeHead[type] = 0;
    s_freeCount[type] = 0;
    slotBase += SRW_EVENT_SLOT_COUNT;
    ++type;
  } while (slotBase < &s_free[0][0] + SRW_EVENT_TYPES * SRW_EVENT_SLOT_COUNT);

  SServerDestroy();
}

void __fastcall SRWLock::IOsRWLockIncRef() {
  s_initCritsect.Enter();
  if (s_initCount++ == 0) {
    IInitialize();
  }
  s_initCritsect.Leave();
}

void __fastcall SRWLock::IOsRWLockDecRef() {
  s_initCritsect.Enter();
  if (--s_initCount == 0) {
    IDestroy();
  }
  s_initCritsect.Leave();
}

int __fastcall SRWLock::IWaitAndCheckForDeadlock(void *hevent) {
  if (WaitForSingleObject((HANDLE)hevent, 60000) == WAIT_OBJECT_0) {
    return 1;
  }

  SInterlockedIncrement(&s_dupcount);
  CDebugSCritSect::DumpAllEntries();
  CDebugSRWLock::DumpAllEntries();
  return 0;
}

long __fastcall SRWLock::IAllocEvent(unsigned long evtype) {
  long *freeptr;
  long  event;

  if (SInterlockedDecrement(&s_freeCount[evtype]) < 0) {
    FATALERROR(("IAllocEvent: too many %s event allocs", evtype ? "SUMREVTYPE" : "SUAREVTYPE"));
  }

  freeptr = &s_free[evtype][SInterlockedIncrement(&s_freeTail[evtype]) & (SRW_EVENT_SLOT_COUNT - 1)];
  event = SInterlockedExchange(freeptr, 0);
  while (!event) {
    unsigned long spin = s_spinCount;
    do {
      event = SInterlockedExchange(freeptr, 0);
      if (event) {
        return event;
      }
      Pause();
    } while (--spin);
    Sleep(0);
  }

  return event;
}

void __fastcall SRWLock::IFreeEvent(unsigned long evtype, long event, int forcereset) {
  long *freeptr;
  long  oldevent;

  if (SInterlockedIncrement(&s_freeCount[evtype]) > SRW_EVENT_COUNT) {
    FATALERROR(("IFreeEvent: too many %s event frees", evtype ? "SUMREVTYPE" : "SUAREVTYPE"));
  }

  if (forcereset) {
    ResetEvent(s_handle[evtype][((unsigned long)event >> 21) - 1]);
  }

  freeptr = &s_free[evtype][SInterlockedIncrement(&s_freeHead[evtype]) & (SRW_EVENT_SLOT_COUNT - 1)];
  oldevent = SInterlockedExchange(freeptr, event & SRW_EVENT_VALUE_MASK);
  while (oldevent) {
    unsigned long spin = s_spinCount;
    do {
      oldevent = SInterlockedExchange(freeptr, oldevent);
      if (!oldevent) {
        return;
      }
      Pause();
    } while (--spin);
    Sleep(0);
  }
}

int __fastcall SRWLock::IWaitForEvent(unsigned long evtype, long event) {
  return IWaitAndCheckForDeadlock(s_handle[evtype][((unsigned long)event >> 21) - 1]);
}

void __fastcall SRWLock::ISetEvent(unsigned long evtype, long event) {
  if (event) {
    SetEvent(s_handle[evtype][((unsigned long)event >> 21) - 1]);
  }
}

long __fastcall SRWLock::IEventIncRefCountOnly(long volatile *eventptr, long increment) {
  long value;

  value = *eventptr;
  if (!value) {
    return 0;
  }

  do {
    if (SInterlockedCompareExchange((long *)eventptr, value + increment, value) == value) {
      return value + increment;
    }

    value = *eventptr;
  } while (value);

  return 0;
}

long __fastcall SRWLock::IAllocEventOrIncRefCount(unsigned long evtype, long volatile *eventptr, long increment) {
  long allocated;
  long value;

  allocated = IAllocEvent(evtype);
  value = SInterlockedCompareExchange((long *)eventptr, allocated, 0);
  while (value) {
    if (SInterlockedCompareExchange((long *)eventptr, value + increment, value) == value) {
      IFreeEvent(evtype, allocated, 0);
      return value + increment;
    }

    value = SInterlockedCompareExchange((long *)eventptr, allocated, 0);
  }

  return allocated;
}

int __fastcall SRWLock::IDecRefCountAndFreeEvent(unsigned long evtype, long volatile *eventptr, long finalevent, long decrement) {
  long previous;

  previous = SInterlockedCompareExchange((long *)eventptr, 0, finalevent);
  while (previous != finalevent) {
    if (SInterlockedCompareExchange((long *)eventptr, previous - decrement, previous) == previous) {
      return 0;
    }
    previous = SInterlockedCompareExchange((long *)eventptr, 0, finalevent);
  }

  IFreeEvent(evtype, finalevent, 1);
  return 1;
}

void __fastcall SRWLock::SUNNLockInitialize(SUNNLOCK volatile *sunnlock) {
  sunnlock->m_state = 1;
  sunnlock->m_event = 0;
}

void __fastcall SRWLock::SUNNLockDelete(SUNNLOCK volatile *sunnlock) {
  long eventValue;

  sunnlock->m_state = 0;
  eventValue = SInterlockedExchange((long *)&sunnlock->m_event, 0);
  if (eventValue) {
    IFreeEvent(0, eventValue, 1);
  }
}

void __fastcall SRWLock::SUNNLockEnter(SUNNLOCK volatile *sunnlock) {
  DWORD spin;
  long  eventValue;
  long  baseValue;

  if (!sunnlock->m_event) {
    spin = s_spinCount;
    for (;;) {
      if (SInterlockedExchange((long *)&sunnlock->m_state, 0) != 0) {
        return;
      }
      Pause();
      if (!--spin || sunnlock->m_event) {
        break;
      }
    }
  }

  eventValue = IAllocEventOrIncRefCount(0, (volatile long *)&sunnlock->m_event, 1);
  baseValue = (long)((unsigned long)eventValue & SRW_EVENT_VALUE_MASK);
  for (;;) {
    spin = s_spinCount;
    do {
      if (SInterlockedExchange((long *)&sunnlock->m_state, 0) != 0) {
        IDecRefCountAndFreeEvent(0, (volatile long *)&sunnlock->m_event, baseValue, 1);
        return;
      }
      Pause();
    } while (--spin);

    IWaitForEvent(0, sunnlock->m_event);
  }
}

int __fastcall SRWLock::SUNNLockTryEnter(SUNNLOCK volatile *sunnlock) {
  return SInterlockedExchange((long *)&sunnlock->m_state, 0) != 0;
}

void __fastcall SRWLock::SUNNLockLeave(SUNNLOCK volatile *sunnlock) {
  sunnlock->m_state = 1;
  ISetEvent(0, sunnlock->m_event);
}

void __fastcall SRWLock::SURWLockInitialize(SURWLOCK volatile *surwlock) {
  SUNNLockInitialize(&surwlock->m_mutex);
  surwlock->m_readerEvent = 0;
}

void __fastcall SRWLock::SURWLockDelete(SURWLOCK volatile *surwlock) {
  long eventValue;

  SUNNLockDelete(&surwlock->m_mutex);
  eventValue = SInterlockedExchange((long *)&surwlock->m_readerEvent, 0);
  if (eventValue) {
    IFreeEvent(1, eventValue, 1);
  }
}

void __fastcall SRWLock::SURWLockEnter(SURWLOCK volatile *surwlock, int forwriting) {
  DWORD spin;
  long  eventValue;
  long  baseValue;

  if (forwriting) {
    SUNNLockEnter(&surwlock->m_mutex);
    return;
  }

  eventValue = IAllocEventOrIncRefCount(1, (volatile long *)&surwlock->m_readerEvent, 2);
  baseValue = (long)((unsigned long)eventValue & SRW_EVENT_VALUE_MASK);
  if (eventValue == baseValue) {
    SUNNLockEnter(&surwlock->m_mutex);
    SInterlockedIncrement((long *)&surwlock->m_readerEvent);
    ISetEvent(1, eventValue);
    return;
  }

  for (;;) {
    spin = s_spinCount;
    while (!(surwlock->m_readerEvent & 1)) {
      Pause();
      if (--spin == 0) {
        if (IWaitForEvent(1, eventValue)) {
          return;
        }
        break;
      }
    }
    if (surwlock->m_readerEvent & 1) {
      return;
    }
  }
}

int __fastcall SRWLock::SURWLockTryEnter(SURWLOCK volatile *surwlock, int forwriting) {
  DWORD spin;
  long  eventValue;
  long  baseValue;

  if (forwriting) {
    return SUNNLockTryEnter(&surwlock->m_mutex);
  }

  if (SUNNLockTryEnter(&surwlock->m_mutex)) {
    eventValue = IAllocEventOrIncRefCount(1, (volatile long *)&surwlock->m_readerEvent, 2);
    baseValue = (long)((unsigned long)eventValue & SRW_EVENT_VALUE_MASK);
    if (eventValue == baseValue) {
      SInterlockedIncrement((long *)&surwlock->m_readerEvent);
      ISetEvent(1, eventValue);
      goto success;
    }

    SUNNLockLeave(&surwlock->m_mutex);
  } else if (!IEventIncRefCountOnly((volatile long *)&surwlock->m_readerEvent, 2)) {
    return 0;
  }

  spin = s_spinCount;
  while (!(surwlock->m_readerEvent & 1)) {
    Pause();
    if (--spin == 0) {
      SURWLockLeave(surwlock, 0);
      return 0;
    }
  }

success:
  return 1;
}

void __fastcall SRWLock::SURWLockLeave(SURWLOCK volatile *surwlock, int fromwriting) {
  if (fromwriting) {
    SUNNLockLeave(&surwlock->m_mutex);
    return;
  }

  if (IDecRefCountAndFreeEvent(1, (volatile long *)&surwlock->m_readerEvent, (surwlock->m_readerEvent & SRW_EVENT_VALUE_MASK) + 1, 2)) {
    SUNNLockLeave(&surwlock->m_mutex);
  }
}

template <class T>
void __fastcall CDebugLock<T>::IRepairBadEntry(CDebugLockData *lock, DWORD e, CDebugLockEntry *eptr, const char *fileName, DWORD line) {
  SOutputDebugString("%s(%u) : CDebugLock:%08x: entry has bad next %u\n", fileName, line, lock, e);
  if (eptr) {
    eptr->next = 0;
  } else {
    lock->entry = 0;
  }
}

template <class T>
void __fastcall CDebugLock<T>::IEnter() {
  DWORD i;

  if (s_critsect.Enter()) {
    memset(s_entries, 0, sizeof(s_entries));
    for (i = 1; i < 255; ++i) {
      s_entries[i].next = i + 1;
    }
    s_freeEntries = 1;
  }
}

template <class T>
void __fastcall CDebugLock<T>::ILeave() {
  s_critsect.Leave();
}

template <class T>
void __fastcall CDebugLock<T>::IDumpAllEntries() {
  CDebugLockData *data;

  data = s_locks;
  while (data) {
    IDumpEntries(data);
    data = data->next;
  }
}

template <class T>
void __fastcall CDebugLock<T>::IDumpEntries(CDebugLockData *lock) {
  DWORD            index;
  DWORD            now;
  CDebugLockEntry *entry;
  CDebugLockEntry *previous;

  previous = NULL;
  now = GetTickCount();
  index = lock->entry;
  while (index) {
    if (index >= 256) {
      IRepairBadEntry(lock, index, previous, __FILE__, __LINE__);
      return;
    }

    entry = &s_entries[index];
    SOutputDebugString(
        "%s(%u) : CDebugLock:%08x: tid:%03x %c %c t:%u\n", entry->filename, entry->flags & 0x3FFFFFFF, lock, entry->threadId,
        (entry->flags & 0x40000000) ? 'W' : 'R', (entry->flags & 0x80000000) ? 'T' : 'F', now - entry->tick
    );
    previous = entry;
    index = entry->next;
  }
}

template <class T>
DWORD __fastcall CDebugLock<T>::IClashingEntry(CDebugLockData *lock, DWORD threadId, int forwriting) {
  DWORD            index;
  CDebugLockEntry *entry;
  CDebugLockEntry *previous;

  previous = NULL;
  index = lock->entry;
  while (index) {
    if (index >= 256) {
      IRepairBadEntry(lock, index, previous, __FILE__, __LINE__);
      return 0;
    }

    entry = &s_entries[index];
    if (entry->threadId == threadId && (forwriting || (entry->flags & 0x40000000))) {
      return index;
    }
    previous = entry;
    index = entry->next;
  }
  return 0;
}

template <class T>
DWORD __fastcall CDebugLock<T>::IAddEntry(CDebugLockData *lock, DWORD threadId, int forwriting, const char *fileName, DWORD line) {
  DWORD            index;
  CDebugLockEntry *entry;

  index = s_freeEntries;
  if (!index) {
    SOutputDebugString("%s(%u) : CDebugLock:%08x no free entries\n", fileName, line, lock);
    IDumpEntries(lock);
    return 0;
  }

  entry = &s_entries[index];
  entry->tick = GetTickCount();
  entry->threadId = threadId;
  entry->filename = fileName;
  entry->flags = (line & 0x3FFFFFFF) | (forwriting ? 0x40000000 : 0);
  s_freeEntries = entry->next;
  entry->next = lock->entry;
  lock->entry = index;
  return index;
}

template <class T>
DWORD __fastcall CDebugLock<T>::IDeleteEntry(CDebugLockData *lock, DWORD threadId, int fromwriting) {
  DWORD            index;
  CDebugLockEntry *entry;
  CDebugLockEntry *previous;

  previous = NULL;
  index = lock->entry;
  while (index) {
    if (index >= 256) {
      IRepairBadEntry(lock, index, previous, __FILE__, __LINE__);
      return index;
    }

    entry = &s_entries[index];
    if (entry->threadId == threadId && (entry->flags & 0x80000000) && ((entry->flags >> 30) & 1) == (DWORD)fromwriting) {
      if (previous) {
        previous->next = entry->next;
      } else {
        lock->entry = entry->next;
      }
      entry->threadId = 0;
      entry->next = s_freeEntries;
      s_freeEntries = index;
      return index;
    }

    previous = entry;
    index = entry->next;
  }
  return 0;
}

template <class T>
void __fastcall CDebugLock<T>::IEnterEntry(DWORD e) {
  if (e) {
    s_entries[e].flags |= 0x80000000;
  }
}

template <class T>
void __fastcall CDebugLock<T>::Construct(CDebugLockData *lock) {
  lock->entry = 0;
  lock->prev = NULL;

  IEnter();
  if (s_locks) {
    s_locks->prev = lock;
  }
  lock->next = s_locks;
  s_locks = lock;
  ILeave();
}

template <class T>
void __fastcall CDebugLock<T>::Destruct(CDebugLockData *lock) {
  DWORD            index;
  CDebugLockEntry *entry;

  IEnter();
  if (lock->prev) {
    lock->prev->next = lock->next;
  } else {
    s_locks = lock->next;
  }
  if (lock->next) {
    lock->next->prev = lock->prev;
  }

  index = lock->entry;
  while (index) {
    if (index >= 256) {
      IRepairBadEntry(lock, index, NULL, __FILE__, __LINE__);
      break;
    }

    entry = &s_entries[index];
    lock->entry = entry->next;
    entry->threadId = 0;
    entry->next = s_freeEntries;
    s_freeEntries = index;
    index = lock->entry;
  }
  ILeave();
}

template <class T>
CInitCritSect CDebugLock<T>::s_critsect;
template <class T>
CDebugLockEntry CDebugLock<T>::s_entries[256];
template <class T>
DWORD CDebugLock<T>::s_freeEntries;
template <class T>
CDebugLockData *CDebugLock<T>::s_locks;

template void __fastcall CDebugLock<CDebugSCritSect>::Construct(CDebugLockData *lock);
template void __fastcall CDebugLock<CDebugSCritSect>::Destruct(CDebugLockData *lock);
template void __fastcall CDebugLock<CDebugSCritSect>::IEnter();
template void __fastcall CDebugLock<CDebugSCritSect>::ILeave();
template void __fastcall CDebugLock<CDebugSCritSect>::IDumpAllEntries();
template DWORD __fastcall CDebugLock<
    CDebugSCritSect>::IAddEntry(CDebugLockData *lock, DWORD threadId, int forwriting, const char *fileName, DWORD line);
template DWORD __fastcall CDebugLock<CDebugSCritSect>::IDeleteEntry(CDebugLockData *lock, DWORD threadId, int fromwriting);
template void __fastcall CDebugLock<CDebugSCritSect>::IEnterEntry(DWORD e);

template void __fastcall CDebugLock<CDebugSRWLock>::Construct(CDebugLockData *lock);
template void __fastcall CDebugLock<CDebugSRWLock>::Destruct(CDebugLockData *lock);
template void __fastcall CDebugLock<CDebugSRWLock>::IEnter();
template void __fastcall CDebugLock<CDebugSRWLock>::ILeave();
template void __fastcall CDebugLock<CDebugSRWLock>::IDumpAllEntries();
template DWORD __fastcall CDebugLock<CDebugSRWLock>::IClashingEntry(CDebugLockData *lock, DWORD threadId, int forwriting);
template DWORD __fastcall CDebugLock<
    CDebugSRWLock>::IAddEntry(CDebugLockData *lock, DWORD threadId, int forwriting, const char *fileName, DWORD line);
template DWORD __fastcall CDebugLock<CDebugSRWLock>::IDeleteEntry(CDebugLockData *lock, DWORD threadId, int fromwriting);
template void __fastcall CDebugLock<CDebugSRWLock>::IEnterEntry(DWORD e);

SCritSect::SCritSect() {
  InitializeCriticalSection((LPCRITICAL_SECTION)m_opaqueData);
}

SCritSect::~SCritSect() {
  DeleteCriticalSection((LPCRITICAL_SECTION)m_opaqueData);
}

CDebugSCritSect::CDebugSCritSect() {
  CDebugLock<CDebugSCritSect>::Construct((CDebugLockData *)m_debugData);
}

void SCritSect::Enter() {
  EnterCriticalSection((LPCRITICAL_SECTION)m_opaqueData);
}

int SCritSect::TryEnter() {
  return STryEnterCriticalSection((LPCRITICAL_SECTION)m_opaqueData);
}

void SCritSect::Leave() {
  LeaveCriticalSection((LPCRITICAL_SECTION)m_opaqueData);
}

CDebugSCritSect::~CDebugSCritSect() {
  CDebugLock<CDebugSCritSect>::Destruct((CDebugLockData *)m_debugData);
}

void CDebugSCritSect::Enter(const char *fileName, unsigned long line) {
  DWORD entry;
  DWORD threadId;

  threadId = GetCurrentThreadId();
  CDebugLock<CDebugSCritSect>::IEnter();
  entry = CDebugLock<CDebugSCritSect>::IAddEntry((CDebugLockData *)m_debugData, threadId, 1, fileName, line);
  CDebugLock<CDebugSCritSect>::ILeave();
  SCritSect::Enter();
  CDebugLock<CDebugSCritSect>::IEnterEntry(entry);
}

int CDebugSCritSect::TryEnter(const char *fileName, unsigned long line) {
  CDebugLockData *data;
  DWORD           e;
  DWORD           threadId;

  threadId = GetCurrentThreadId();
  data = (CDebugLockData *)m_debugData;
  CDebugLock<CDebugSCritSect>::IEnter();
  e = CDebugLock<CDebugSCritSect>::IAddEntry(data, threadId, 1, fileName, line);
  CDebugLock<CDebugSCritSect>::ILeave();
  if (SCritSect::TryEnter()) {
    CDebugLock<CDebugSCritSect>::IEnterEntry(e);
    return 1;
  }

  CDebugLock<CDebugSCritSect>::IEnter();
  CDebugLock<CDebugSCritSect>::IDeleteEntry(data, threadId, 1);
  CDebugLock<CDebugSCritSect>::ILeave();
  return 0;
}

void CDebugSCritSect::Leave(const char *fileName, unsigned long line) {
  CDebugLockData *data;
  DWORD           threadId;

  threadId = GetCurrentThreadId();
  data = (CDebugLockData *)m_debugData;
  CDebugLock<CDebugSCritSect>::IEnter();
  if (!CDebugLock<CDebugSCritSect>::IDeleteEntry(data, threadId, 1)) {
    SOutputDebugString("%s(%u) : CDebugSCritSect:%08x:Leave without Enter\n", fileName, line, this);
    CDebugLock<CDebugSCritSect>::IDumpEntries(data);
  }
  CDebugLock<CDebugSCritSect>::ILeave();
  SCritSect::Leave();
}

void __fastcall CDebugSCritSect::DumpAllEntries() {
  CDebugLock<CDebugSCritSect>::IEnter();
  SOutputDebugString("%s(%u) : CDebugSCritSect:DumpAllEntries\n", __FILE__, __LINE__);
  CDebugLock<CDebugSCritSect>::IDumpAllEntries();
  CDebugLock<CDebugSCritSect>::ILeave();
}

int SInitCritSect::Enter() {
  int created;

  created = 0;
  if (!m_critsect) {
    do {
    } while (SInterlockedExchange((long *)&m_spinLock, 1));

    if (!m_critsect) {
      m_critsect = (SCritSect *)m_critsectData;
      new (m_critsect) SCritSect;
      created = 1;
    }

    m_spinLock = 0;
  }

  m_critsect->Enter();
  return created;
}

void SInitCritSect::Leave() {
  m_critsect->Leave();
}

CSRWLock::CSRWLock() {
  SRWLock::IOsRWLockIncRef();
  SRWLock::SURWLockInitialize((SRWLock::SURWLOCK *)m_opaqueData);
}

CSRWLock::~CSRWLock() {
  SRWLock::SURWLockDelete((SRWLock::SURWLOCK *)m_opaqueData);
  SRWLock::IOsRWLockDecRef();
}

void CSRWLock::Enter(int forwriting) {
  SRWLock::SURWLockEnter((SRWLock::SURWLOCK *)m_opaqueData, forwriting);
}

void CSRWLock::Leave(int fromwriting) {
  SRWLock::SURWLockLeave((SRWLock::SURWLOCK *)m_opaqueData, fromwriting);
}

int CSRWLock::TryEnter(int forwriting) {
  return SRWLock::SURWLockTryEnter((SRWLock::SURWLOCK *)m_opaqueData, forwriting);
}

CDebugSRWLock::CDebugSRWLock() {
  CDebugLock<CDebugSRWLock>::Construct((CDebugLockData *)m_debugData);
}

CDebugSRWLock::~CDebugSRWLock() {
  CDebugLock<CDebugSRWLock>::Destruct((CDebugLockData *)m_debugData);
}

void CDebugSRWLock::Enter(int forwriting, const char *fileName, unsigned long line) {
  CDebugLockData *data;
  DWORD           entry;
  DWORD           threadId;

  threadId = GetCurrentThreadId();
  data = (CDebugLockData *)m_debugData;
  CDebugLock<CDebugSRWLock>::IEnter();
  if (CDebugLock<CDebugSRWLock>::IClashingEntry(data, threadId, forwriting)) {
    SOutputDebugString("%s(%u) : CDebugSRWLock:%08x:Enter(%c) already owned\n", fileName, line, this, forwriting ? 'W' : 'R');
    CDebugLock<CDebugSRWLock>::IDumpEntries(data);
    entry = 0;
  } else {
    entry = CDebugLock<CDebugSRWLock>::IAddEntry(data, threadId, forwriting, fileName, line);
  }
  CDebugLock<CDebugSRWLock>::ILeave();
  CSRWLock::Enter(forwriting);
  CDebugLock<CDebugSRWLock>::IEnterEntry(entry);
}

int CDebugSRWLock::TryEnter(int forwriting, const char *fileName, unsigned long line) {
  CDebugLockData *data;
  DWORD           e;
  DWORD           threadId;

  threadId = GetCurrentThreadId();
  data = (CDebugLockData *)m_debugData;
  CDebugLock<CDebugSRWLock>::IEnter();
  if (CDebugLock<CDebugSRWLock>::IClashingEntry(data, threadId, forwriting)) {
    SOutputDebugString("%s(%u) : CDebugSRWLock:%08x:TryEnter(%c) already owned\n", fileName, line, this, forwriting ? 'W' : 'R');
    CDebugLock<CDebugSRWLock>::IDumpEntries(data);
    e = 0;
  } else {
    e = CDebugLock<CDebugSRWLock>::IAddEntry(data, threadId, forwriting, fileName, line);
  }
  CDebugLock<CDebugSRWLock>::ILeave();

  if (CSRWLock::TryEnter(forwriting)) {
    CDebugLock<CDebugSRWLock>::IEnterEntry(e);
    return 1;
  }

  CDebugLock<CDebugSRWLock>::IEnter();
  CDebugLock<CDebugSRWLock>::IDeleteEntry(data, threadId, forwriting);
  CDebugLock<CDebugSRWLock>::ILeave();
  return 0;
}

void CDebugSRWLock::Leave(int fromwriting, const char *fileName, unsigned long line) {
  CDebugLockData *data;
  DWORD           threadId;

  threadId = GetCurrentThreadId();
  data = (CDebugLockData *)m_debugData;
  CDebugLock<CDebugSRWLock>::IEnter();
  if (!CDebugLock<CDebugSRWLock>::IDeleteEntry(data, threadId, fromwriting)) {
    SOutputDebugString("%s(%u) : CDebugSRWLock:%08x:Leave(%c) without Enter\n", fileName, line, this, fromwriting ? 'W' : 'R');
    CDebugLock<CDebugSRWLock>::IDumpEntries(data);
  }
  CDebugLock<CDebugSRWLock>::ILeave();
  CSRWLock::Leave(fromwriting);
}

void __fastcall CDebugSRWLock::DumpAllEntries() {
  CDebugLock<CDebugSRWLock>::IEnter();
  SOutputDebugString("%s(%u) : CDebugSRWLock:DumpAllEntries\n", __FILE__, __LINE__);
  CDebugLock<CDebugSRWLock>::IDumpAllEntries();
  CDebugLock<CDebugSRWLock>::ILeave();
}

void SSyncObject::Copy(const SSyncObject &rhs) {
  if (!DuplicateHandle(
          GetCurrentProcess(), *(const HANDLE *)rhs.m_opaqueData, GetCurrentProcess(), (HANDLE *)m_opaqueData, 0, FALSE, DUPLICATE_SAME_ACCESS
      ))
  {
    FATALERROR(("SSyncObject::Copy: DuplicateHandle failed"));
  }
}

SSyncObject::SSyncObject() {
  *(HANDLE *)m_opaqueData = NULL;
}

SSyncObject::SSyncObject(const SSyncObject &rhs) {
  Copy(rhs);
}

SSyncObject::~SSyncObject() {
  Close();
}

SSyncObject &SSyncObject::operator=(const SSyncObject &rhs) {
  Close();
  Copy(rhs);
  return *this;
}

int SSyncObject::Valid() {
  return *(HANDLE *)m_opaqueData != NULL;
}

void SSyncObject::Close() {
  HANDLE *handle = (HANDLE *)m_opaqueData;
  if (*handle) {
    CloseHandle(*handle);
    *handle = NULL;
  }
}

unsigned long SSyncObject::Wait(unsigned long timeoutMs) {
  return WaitForSingleObject(*(HANDLE *)m_opaqueData, timeoutMs);
}

unsigned long __fastcall WaitMultiple(unsigned int count, SSyncObject *const objects, int waitAll, unsigned long timeoutMs) {
  if (count > MAXIMUM_WAIT_OBJECTS) {
    return 0xFFFFFFFF;
  }

  return WaitForMultipleObjects(count, (const HANDLE *)objects, waitAll, timeoutMs);
}

unsigned long __fastcall WaitMultiplePtr(unsigned int count, SSyncObject **const objectPtrs, int waitAll, unsigned long timeoutMs) {
  HANDLE       objects[MAXIMUM_WAIT_OBJECTS];
  unsigned int i;
  unsigned int handlecount;

  if (count > MAXIMUM_WAIT_OBJECTS) {
    return 0xFFFFFFFF;
  }

  handlecount = 0;
  for (i = 0; i < count; ++i) {
    if (objectPtrs[i] && *(HANDLE *)objectPtrs[i]->m_opaqueData) {
      objects[handlecount++] = *(HANDLE *)objectPtrs[i]->m_opaqueData;
    }
  }

  return WaitForMultipleObjects(handlecount, objects, waitAll, timeoutMs);
}

SEvent::SEvent(int manualReset, int initialValue) {
  HANDLE *handle = (HANDLE *)m_opaqueData;

  *handle = CreateEventA(NULL, manualReset, initialValue, NULL);
  if (!*handle) {
    FATALERROR(("SEvent::SEvent: CreateEvent failed"));
  }
}

int SEvent::Set() {
  return SetEvent(*(HANDLE *)m_opaqueData);
}

int SEvent::Reset() {
  return ResetEvent(*(HANDLE *)m_opaqueData);
}

SSemaphore::SSemaphore(unsigned int initialCount, unsigned int maximumCount) {
  *(HANDLE *)m_opaqueData = CreateSemaphoreA(NULL, initialCount, maximumCount, NULL);
}

int SSemaphore::Signal(unsigned int count) {
  return ReleaseSemaphore(*(HANDLE *)m_opaqueData, count, NULL);
}

int __fastcall SThread::Create(STHREADPROC threadProc, void *param, SThread &thread, char *threadName) {
  unsigned int id;

  (void)threadName;
  *(HANDLE *)thread.m_opaqueData = (HANDLE)SCreateThread(threadProc, param, &id, NULL, NULL);
  return *(HANDLE *)thread.m_opaqueData != NULL;
}

SMutex::SMutex() {
  *(HANDLE *)m_opaqueData = NULL;
  Create(0, NULL);
}

SMutex::SMutex(int initialOwner, const char *name) {
  *(HANDLE *)m_opaqueData = NULL;
  Create(initialOwner, name);
}

SMutex::SMutex(const char *name) {
  *(HANDLE *)m_opaqueData = NULL;
  Open(name);
}

void SMutex::Create(int initialOwner, const char *name) {
  HANDLE *handle;

  Close();
  handle = (HANDLE *)m_opaqueData;
  *handle = CreateMutexA(NULL, initialOwner, name);
  if (!*handle) {
    FATALERROR(("SMutex::Create: CreateMutex failed"));
  }
}

void SMutex::Open(const char *name) {
  Close();
  *(HANDLE *)m_opaqueData = OpenMutexA(0x1F0001, TRUE, name);
}

int SMutex::Release() {
  return ReleaseMutex(*(HANDLE *)m_opaqueData);
}
