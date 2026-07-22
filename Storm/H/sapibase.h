#pragma once

#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
#ifdef STORM_SAPIBASE_IMPLEMENTATION
void *__cdecl operator new(size_t, void *ptr) throw() {
  return ptr;
}
#else
void *__cdecl operator new(size_t, void *ptr) throw();
#endif

void __cdecl operator delete(void *, void *);
#endif
