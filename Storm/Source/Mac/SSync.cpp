#include <storm.h>

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define SYNC_EVENT_MANUAL 1
#define SYNC_EVENT_AUTO   2
#define SYNC_CLOSED       3
#define SYNC_SEMAPHORE    4

struct SYNCDATA {
  int             type;
  int             count;
  pthread_mutex_t mutex;
  pthread_cond_t  cond;
};

static SYNCDATA *SyncData(LPVOID opaqueData) {
  return reinterpret_cast<SYNCDATA *>(opaqueData);
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

static void SyncAcquire(SYNCDATA *sync) {
  if (sync->type == SYNC_EVENT_AUTO) {
    sync->count = 0;
  } else if (sync->type == SYNC_SEMAPHORE) {
    sync->count = sync->count - 1;
  }
}

SSyncObject::SSyncObject() {
  SYNCDATA *sync = SyncData(m_opaqueData);

  sync->type = SYNC_CLOSED;
  sync->count = 0;
  pthread_mutex_init(&sync->mutex, 0);
  pthread_cond_init(&sync->cond, 0);
}

SSyncObject::SSyncObject(const SSyncObject &rhs) {
  Copy(rhs);
}

SSyncObject::~SSyncObject() {
  Close();
}

SSyncObject &SSyncObject::operator=(const SSyncObject &rhs) {
  Copy(rhs);
  return *this;
}

void SSyncObject::Copy(const SSyncObject &rhs) {
  memcpy(m_opaqueData, rhs.m_opaqueData, sizeof(m_opaqueData));
}

BOOL SSyncObject::Valid() {
  return SyncData(m_opaqueData)->type != SYNC_CLOSED;
}

void SSyncObject::Close() {
}

DWORD SSyncObject::Wait(DWORD timeoutMs) {
  SYNCDATA       *sync = SyncData(m_opaqueData);
  struct timespec deadline;

  if (timeoutMs == INFINITE) {
    pthread_mutex_lock(&sync->mutex);

    if (sync->type == SYNC_CLOSED) {
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
      return static_cast<DWORD>(-1);
    }

    gettimeofday(&now, 0);
    if (now.tv_sec > deadline.tv_sec || (now.tv_sec == deadline.tv_sec && 1000 * now.tv_usec >= deadline.tv_nsec)) {
      return WAIT_TIMEOUT;
    }

    usleep(0);
  }

  if (sync->type == SYNC_CLOSED) {
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
      return waited == ETIMEDOUT ? WAIT_TIMEOUT : static_cast<DWORD>(-1);
    }
  }
}

SEvent::SEvent(int manualReset, int initialValue) {
  SYNCDATA *sync = SyncData(m_opaqueData);

  pthread_mutex_init(&sync->mutex, 0);
  sync->type = manualReset ? SYNC_EVENT_MANUAL : SYNC_EVENT_AUTO;
  sync->count = initialValue;
  pthread_cond_init(&sync->cond, 0);
}

SEvent &SEvent::operator=(const SEvent &rhs) {
  Copy(rhs);
  return *this;
}

BOOL SEvent::Set() {
  SYNCDATA *sync = SyncData(m_opaqueData);

  pthread_mutex_lock(&sync->mutex);
  sync->count = 1;
  pthread_mutex_unlock(&sync->mutex);
  pthread_cond_signal(&sync->cond);

  return 1;
}

BOOL SEvent::Reset() {
  SYNCDATA *sync = SyncData(m_opaqueData);

  pthread_mutex_lock(&sync->mutex);
  sync->count = 0;
  pthread_mutex_unlock(&sync->mutex);

  return 1;
}

SSemaphore::SSemaphore(UINT initialCount, UINT maximumCount) {
  SYNCDATA *sync = SyncData(m_opaqueData);

  pthread_mutex_init(&sync->mutex, 0);
  sync->type = SYNC_SEMAPHORE;
  sync->count = initialCount;
  pthread_cond_init(&sync->cond, 0);
}

SSemaphore &SSemaphore::operator=(const SSemaphore &rhs) {
  Copy(rhs);
  return *this;
}

BOOL SSemaphore::Signal(UINT count) {
  SYNCDATA *sync = SyncData(m_opaqueData);

  pthread_mutex_lock(&sync->mutex);
  sync->count = sync->count + count;
  pthread_mutex_unlock(&sync->mutex);
  pthread_cond_broadcast(&sync->cond);

  return 1;
}

CSRWLock::CSRWLock() {
  pthread_rwlock_init(reinterpret_cast<pthread_rwlock_t *>(m_opaqueData), 0);
}

CSRWLock::~CSRWLock() {
  pthread_rwlock_destroy(reinterpret_cast<pthread_rwlock_t *>(m_opaqueData));
}

void CSRWLock::Enter(int forwriting) {
  if (forwriting) {
    pthread_rwlock_wrlock(reinterpret_cast<pthread_rwlock_t *>(m_opaqueData));
  } else {
    pthread_rwlock_rdlock(reinterpret_cast<pthread_rwlock_t *>(m_opaqueData));
  }
}

void CSRWLock::Leave(int fromwriting) {
  pthread_rwlock_unlock(reinterpret_cast<pthread_rwlock_t *>(m_opaqueData));
}

BOOL CSRWLock::TryEnter(int forwriting) {
  if (forwriting) {
    return pthread_rwlock_trywrlock(reinterpret_cast<pthread_rwlock_t *>(m_opaqueData)) == 0;
  }

  return pthread_rwlock_tryrdlock(reinterpret_cast<pthread_rwlock_t *>(m_opaqueData)) == 0;
}

int SGetCurrentThreadPriority() {
  return 0;
}

void SSetCurrentThreadPriority(int priority) {
}

void __cdecl SOutputDebugString(LPCSTR format, ...) {
  char    buffer[256];
  va_list args;

  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  printf("%s", buffer);
}

BOOL SThread::Create(STHREADPROC proc, LPVOID param, SThread &thread, char *name) {
  return SCreateThread(0, proc, param, 0, 0, name) != 0;
}

SThread &SThread::operator=(const SThread &rhs) {
  Copy(rhs);
  return *this;
}

DWORD WaitMultiplePtr(UINT count, SSyncObject **const objectPtrs, int waitAll, DWORD timeoutMs) {
  UINT index;

  for (index = 0; index < count; ++index) {
    DWORD result = objectPtrs[index]->Wait(waitAll ? timeoutMs : 0);

    if (waitAll) {
      if (result != WAIT_OBJECT_0) {
        return result;
      }
    } else if (result == WAIT_OBJECT_0) {
      return WAIT_OBJECT_0 + index;
    }
  }

  return waitAll ? WAIT_OBJECT_0 : WAIT_TIMEOUT;
}
