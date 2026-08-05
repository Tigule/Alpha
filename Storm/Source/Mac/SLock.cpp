#include <storm.h>
#include "ISThread.h"

SCritSect::SCritSect() {
  static DWORD _ASSERTSAMESIZE_SCritSect_pthread_mutex_t[sizeof(SCritSect) == sizeof(pthread_mutex_t)];

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

int SCritSect::TryEnter() {
  return pthread_mutex_trylock((pthread_mutex_t *)m_opaqueData) == 0;
}

void SCritSect::Leave() {
  pthread_mutex_unlock((pthread_mutex_t *)m_opaqueData);
}
