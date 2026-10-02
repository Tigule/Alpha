#pragma once

#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
inline LPVOID __cdecl operator new(size_t, LPVOID ptr) throw() {
  return ptr;
}

inline void __cdecl operator delete(LPVOID, LPVOID) {
}
#endif

#if defined(_MSC_VER) && _MSC_VER == 1200
inline void __cdecl operator delete(LPVOID ptr) {
  if (ptr) {
    SMemFree(ptr, "delete", -1, 0);
  }
}

inline LPVOID __cdecl operator new(size_t bytes) {
  return SMemAlloc(bytes, "new", -1, 0);
}

inline void __cdecl operator delete[](LPVOID ptr) {
  if (ptr) {
    SMemFree(ptr, "delete[]", -1, 0);
  }
}

inline LPVOID __cdecl operator new[](size_t bytes) {
  return SMemAlloc(bytes, "new[]", -1, 0);
}

inline LPVOID __cdecl operator new[](size_t bytes, LPCSTR filename, int linenumber) {
  return SMemAlloc(bytes, filename, linenumber, 0);
}
#endif
