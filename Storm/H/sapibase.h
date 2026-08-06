#pragma once

#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
inline LPVOID __cdecl operator new(size_t, LPVOID ptr) throw() {
  return ptr;
}

void __cdecl operator delete(LPVOID, LPVOID);
#endif
