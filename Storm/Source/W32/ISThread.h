#ifndef STORM_SOURCE_W32_ISTHREAD_H
#define STORM_SOURCE_W32_ISTHREAD_H

#include <windows.h>

class CCritSect {
 private:
  CRITICAL_SECTION m_critsect;

 public:
  CCritSect() throw();
  CCritSect(const CCritSect &);
  ~CCritSect();
  CCritSect &operator=(const CCritSect &);

  void Enter();

  void Enter(int) {
    EnterCriticalSection(&m_critsect);
  }

  void Leave();

  void Leave(int) {
    LeaveCriticalSection(&m_critsect);
  }
};

class CInitCritSect {
 private:
  LONG       m_spinLock;
  CCritSect *m_critsect;
  BYTE       m_critsectData[0x18];

 public:
  BOOL Enter();
  void Leave();
};

#include <sclass.h>

inline BOOL CInitCritSect::Enter() {
  int initialized = 0;

  if (!m_critsect) {
    do {
    } while (InterlockedExchange(&m_spinLock, 1));

    if (!m_critsect) {
      m_critsect = reinterpret_cast<CCritSect *>(m_critsectData);
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
