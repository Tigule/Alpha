#include <storm.h>
#include "ISThread.h"

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define SYNC_EVENT_MANUAL 1
#define SYNC_EVENT_AUTO   2
#define SYNC_MUTEX        3
#define SYNC_SEMAPHORE    4
#define SYNC_THREAD       5

static SyncObjectData *SyncData(LPVOID opaqueData) {
  return (SyncObjectData *)opaqueData;
}

static void SyncDeadline(DWORD timeoutMs, struct timespec *deadline) {
  struct timeval now;

  gettimeofday(&now, 0);

  deadline->tv_sec = now.tv_sec + timeoutMs / 1000;
  deadline->tv_nsec = 1000 * now.tv_usec + timeoutMs % 1000 * 1000000;

  if (deadline->tv_nsec >= 1000000000) {
    deadline->tv_sec += 1;
    deadline->tv_nsec -= 1000000000;
  }
}

static void SyncAcquire(SyncObjectData *sync) {
  if (sync->type == SYNC_EVENT_AUTO) {
    sync->count = 0;
  } else if (sync->type == SYNC_SEMAPHORE) {
    sync->count = sync->count - 1;
  }
}

SSyncObject::SSyncObject() {
  pthread_mutex_init(&SyncData(m_opaqueData)->mutex, 0);
}

SSyncObject::SSyncObject(const SSyncObject &rhs) {
  Copy(rhs);
}

SSyncObject::~SSyncObject() {
  pthread_mutex_destroy(&SyncData(m_opaqueData)->mutex);
}

SSyncObject &SSyncObject::operator=(const SSyncObject &rhs) {
  Copy(rhs);
  return *this;
}

void SSyncObject::Copy(const SSyncObject &rhs) {
  memcpy(m_opaqueData, rhs.m_opaqueData, sizeof(m_opaqueData));
}

BOOL SSyncObject::Valid() {
  UINT type = SyncData(m_opaqueData)->type;

  return type > 0 && type < 6;
}

void SSyncObject::Close() {
}

DWORD SSyncObject::Wait(DWORD timeoutMs) {
  SyncObjectData *sync = SyncData(m_opaqueData);
  struct timespec deadline;

  if (timeoutMs == INFINITE) {
    pthread_mutex_lock(&sync->mutex);

    if (sync->type == SYNC_MUTEX) {
      return WAIT_OBJECT_0;
    }

    while (!sync->count) {
      pthread_cond_wait(&sync->cond, &sync->mutex);
    }

    SyncAcquire(sync);
    pthread_mutex_unlock(&sync->mutex);
    return WAIT_OBJECT_0;
  }

  SyncDeadline(timeoutMs, &deadline);

  for (;;) {
    struct timeval now;
    int            locked = pthread_mutex_trylock(&sync->mutex);

    if (!locked) {
      break;
    }

    if (locked != EBUSY) {
      return (DWORD)-1;
    }

    gettimeofday(&now, 0);
    if (now.tv_sec > deadline.tv_sec || (now.tv_sec == deadline.tv_sec && 1000 * now.tv_usec >= deadline.tv_nsec)) {
      return WAIT_TIMEOUT;
    }

    usleep(0);
  }

  if (sync->type == SYNC_MUTEX) {
    return WAIT_OBJECT_0;
  }

  for (;;) {
    int waited;

    if (sync->count) {
      SyncAcquire(sync);
      pthread_mutex_unlock(&sync->mutex);
      return WAIT_OBJECT_0;
    }

    waited = pthread_cond_timedwait(&sync->cond, &sync->mutex, &deadline);
    if (waited) {
      pthread_mutex_unlock(&sync->mutex);
      return waited == ETIMEDOUT ? WAIT_TIMEOUT : (DWORD)-1;
    }
  }
}

SEvent::SEvent(int manualReset, int initialValue) {
  SyncObjectData *sync = SyncData(m_opaqueData);

  sync->type = manualReset ? SYNC_EVENT_MANUAL : SYNC_EVENT_AUTO;
  sync->count = initialValue;
  pthread_cond_init(&sync->cond, 0);
}

SEvent &SEvent::operator=(const SEvent &rhs) {
  Copy(rhs);
  return *this;
}

BOOL SEvent::Set() {
  SyncObjectData *sync = SyncData(m_opaqueData);

  pthread_mutex_lock(&sync->mutex);
  sync->count = 1;
  pthread_mutex_unlock(&sync->mutex);
  pthread_cond_signal(&sync->cond);

  return TRUE;
}

BOOL SEvent::Reset() {
  SyncObjectData *sync = SyncData(m_opaqueData);

  pthread_mutex_lock(&sync->mutex);
  sync->count = 0;
  pthread_mutex_unlock(&sync->mutex);

  return TRUE;
}

SSemaphore::SSemaphore(UINT initialCount, UINT maximumCount) {
  SyncObjectData *sync = SyncData(m_opaqueData);

  sync->count = initialCount;
  sync->max = maximumCount;
  sync->type = SYNC_SEMAPHORE;
  pthread_cond_init(&sync->cond, 0);
}

SSemaphore &SSemaphore::operator=(const SSemaphore &rhs) {
  Copy(rhs);
  return *this;
}

BOOL SSemaphore::Signal(UINT count) {
  SyncObjectData *sync = SyncData(m_opaqueData);
  UINT      newCount;

  pthread_mutex_lock(&sync->mutex);

  newCount = sync->count + count;
  if (newCount > (UINT)sync->max) {
    pthread_mutex_unlock(&sync->mutex);
    return FALSE;
  }

  sync->count = newCount;
  pthread_mutex_unlock(&sync->mutex);

  if (count > 1) {
    pthread_cond_broadcast(&sync->cond);
  } else {
    pthread_cond_signal(&sync->cond);
  }

  return TRUE;
}

CSRWLock::CSRWLock() {
  pthread_rwlock_init((pthread_rwlock_t *)m_opaqueData, 0);
}

CSRWLock::~CSRWLock() {
  pthread_rwlock_destroy((pthread_rwlock_t *)m_opaqueData);
}

void CSRWLock::Enter(int forwriting) {
  if (forwriting) {
    pthread_rwlock_wrlock((pthread_rwlock_t *)m_opaqueData);
  } else {
    pthread_rwlock_rdlock((pthread_rwlock_t *)m_opaqueData);
  }
}

void CSRWLock::Leave(int fromwriting) {
  pthread_rwlock_unlock((pthread_rwlock_t *)m_opaqueData);
}

int SGetCurrentThreadPriority() {
  return 0;
}

void SSetCurrentThreadPriority(int priority) {
}

void __cdecl SOutputDebugString(LPCSTR format, ...) {
  va_list args;

  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
}

BOOL SThread::Create(STHREADPROC proc, LPVOID param, SThread &thread, char *name) {
  SyncObjectData *sync = SyncData(thread.m_opaqueData);
  UINT            threadId;

  sync->count = 0;
  sync->max = 0;
  sync->type = SYNC_THREAD;
  pthread_cond_init(&sync->cond, 0);
  return SCreateThread(proc, param, &threadId, &thread, NULL) != NULL;
}

SThread &SThread::operator=(const SThread &rhs) {
  Copy(rhs);
  return *this;
}

DWORD WaitMultiplePtr(UINT count, SSyncObject **objectPtrs, int waitAll, DWORD timeoutMs) {
  if (!(count <= 1)) {
    SErrDisplayErrorFmt(
        STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
        isprint((count >> 24) & 0xFF) && isprint((count >> 16) & 0xFF) && isprint((count >> 8) & 0xFF) && isprint(count & 0xFF)
            ? "\"%s\", %s = %ld (0x%08X, '%c%c%c%c')"
            : "\"%s\", %s = %ld (0x%08X)",
        "count <= 1", "count", (long)count, count, (count >> 24) & 0xFF, (count >> 16) & 0xFF, (count >> 8) & 0xFF, count & 0xFF
    );
  }

  return count ? objectPtrs[0]->Wait(timeoutMs) : (DWORD)-1;
}
