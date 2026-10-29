#pragma once

class CCritSect {
 private:
  CRITICAL_SECTION m_critsect;

 public:
  CCritSect() throw() { InitializeCriticalSection(&m_critsect); }
  ~CCritSect() { DeleteCriticalSection(&m_critsect); }
  CCritSect(const CCritSect &);
  CCritSect &operator=(const CCritSect &);
  void Enter() { EnterCriticalSection(&m_critsect); }
  void Enter(int) { EnterCriticalSection(&m_critsect); }
  void Leave() { LeaveCriticalSection(&m_critsect); }
  void Leave(int) { LeaveCriticalSection(&m_critsect); }
};
