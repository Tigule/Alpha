#ifndef STORM_SOURCE_MAC_ISTHREAD_H
#define STORM_SOURCE_MAC_ISTHREAD_H

#include <storm.h>

#include <pthread.h>

struct SyncObjectData {
  int             type;
  int             count;
  int             max;
  pthread_mutex_t mutex;
  pthread_cond_t  cond;
};

class CCritSect {
 private:
  pthread_mutex_t m_critsect;

 public:
  CCritSect() throw() {
    pthread_mutex_init(&m_critsect, 0);
  }

  CCritSect(const CCritSect &);

  ~CCritSect() {
    pthread_mutex_destroy(&m_critsect);
  }

  CCritSect &operator=(const CCritSect &);

  void Enter() {
    pthread_mutex_lock(&m_critsect);
  }

  void Enter(int) {
    pthread_mutex_lock(&m_critsect);
  }

  void Leave() {
    pthread_mutex_unlock(&m_critsect);
  }

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

inline BOOL CInitCritSect::Enter() {
  int initialized = 0;

  if (!m_critsect) {
    do {
    } while (SInterlockedExchange(&m_spinLock, 1));

    if (!m_critsect) {
      m_critsect = (CCritSect *)m_critsectData;
      new (m_critsect) CCritSect;
      initialized = 1;
    }

    m_spinLock = 0;
  }

  m_critsect->Enter();
  return initialized;
}

inline void CInitCritSect::Leave() {
  m_critsect->Leave();
}

#endif
