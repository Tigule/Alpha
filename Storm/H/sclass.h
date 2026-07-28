#pragma once

inline CCritSect::CCritSect() throw() {
  InitializeCriticalSection(&m_critsect);
}

inline CCritSect::~CCritSect() {
  DeleteCriticalSection(&m_critsect);
}

inline void CCritSect::Enter() {
  EnterCriticalSection(&m_critsect);
}

inline void CCritSect::Leave() {
  LeaveCriticalSection(&m_critsect);
}
