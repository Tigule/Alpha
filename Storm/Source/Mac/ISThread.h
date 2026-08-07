#ifndef STORM_SOURCE_MAC_ISTHREAD_H
#define STORM_SOURCE_MAC_ISTHREAD_H

#include <storm.h>

#include <pthread.h>

class CCritSect {
 private:
  pthread_mutex_t m_critsect;

 public:
  CCritSect() throw();
  CCritSect(const CCritSect &);
  ~CCritSect();
  CCritSect &operator=(const CCritSect &);

  void Enter();

  void Enter(int) {
    pthread_mutex_lock(&m_critsect);
  }

  void Leave();

  void Leave(int) {
    pthread_mutex_unlock(&m_critsect);
  }
};

class CInitCritSect {
 private:
  LONG       m_spinLock;
  CCritSect *m_critsect;
  BYTE       m_critsectData[sizeof(pthread_mutex_t)];

 public:
  BOOL Enter();
  void Leave();
};

#include <sclass.h>

inline BOOL CInitCritSect::Enter() {
  int initialized = 0;

  if (!m_critsect) {
    do {
    } while (SInterlockedExchange(&m_spinLock, 1));

    if (!m_critsect) {
      pthread_mutexattr_t attributes;

      m_critsect = reinterpret_cast<CCritSect *>(m_critsectData);

      pthread_mutexattr_init(&attributes);
      pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE);
      pthread_mutex_init(reinterpret_cast<pthread_mutex_t *>(m_critsectData), &attributes);

      initialized = 1;
    }

    m_spinLock = 0;
  }

  pthread_mutex_lock(reinterpret_cast<pthread_mutex_t *>(m_critsect));
  return initialized;
}

inline void CInitCritSect::Leave() {
  pthread_mutex_unlock(reinterpret_cast<pthread_mutex_t *>(m_critsect));
}

#endif
