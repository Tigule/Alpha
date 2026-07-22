#pragma once

#ifdef STORM_CRITSECT_COMDAT_IMPLEMENTATION
inline CCritSect::CCritSect() throw() {
  InitializeCriticalSection(&m_critsect);
}

inline CCritSect::~CCritSect() {
  DeleteCriticalSection(&m_critsect);
}
#endif

#ifdef STORM_CRITSECT_METHOD_IMPLEMENTATION
void CCritSect::Enter() {
  EnterCriticalSection(&m_critsect);
}

void CCritSect::Leave() {
  LeaveCriticalSection(&m_critsect);
}
#endif
