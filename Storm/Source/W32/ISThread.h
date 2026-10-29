#ifndef STORM_SOURCE_W32_ISTHREAD_H
#define STORM_SOURCE_W32_ISTHREAD_H

#include <windows.h>
#include <sclass.h>

class CInitCritSect {
 private:
  LONG       m_spinLock;
  CCritSect *m_critsect;
  BYTE       m_critsectData[0x18];

 public:
  BOOL Enter();
  void Leave();
};

inline BOOL CInitCritSect::Enter() {
  int initialized = 0;

  if (!m_critsect) {
    do {
    } while (InterlockedExchange(&m_spinLock, 1));

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
