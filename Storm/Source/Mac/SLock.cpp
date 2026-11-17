#include <storm.h>
#include "ISThread.h"

SCritSect::SCritSect() {
  static unsigned _ASSERTSAMESIZE_SCritSect_pthread_mutex_t[sizeof(SCritSect) == sizeof(pthread_mutex_t) ? 1 : -1];

  pthread_mutexattr_t attributes;

  pthread_mutexattr_init(&attributes);
  pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init((pthread_mutex_t *)m_opaqueData, &attributes);
}

SCritSect::~SCritSect() {
  pthread_mutex_destroy((pthread_mutex_t *)m_opaqueData);
}

void SCritSect::Enter() {
  pthread_mutex_lock((pthread_mutex_t *)m_opaqueData);
}

BOOL SCritSect::TryEnter() {
  return pthread_mutex_trylock((pthread_mutex_t *)m_opaqueData) == 0;
}

void SCritSect::Leave() {
  pthread_mutex_unlock((pthread_mutex_t *)m_opaqueData);
}

BOOL SInitCritSect::Enter() {
  int created;

  created = 0;
  if (!m_critsect) {
    do {
    } while (SInterlockedExchange(&m_spinLock, 1));

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
