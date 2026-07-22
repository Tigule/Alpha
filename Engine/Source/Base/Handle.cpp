#include "Handle.h"

void __fastcall HandleClose(HOBJECT handle) {
  CHandleObject *ptr = HandleDereference(handle);

  ptr->DecRef();
}

HOBJECT __fastcall HandleCreate(CHandleObject *ptr, const char *handleName) {
  FATALASSERT(ptr);

  ptr->IncRef();
  return reinterpret_cast<HOBJECT>(ptr);
}

CHandleObject *__fastcall HandleDereference(HOBJECT handle) {
  return reinterpret_cast<CHandleObject *>(handle);
}

void __fastcall HandleDestroy() {
}

HOBJECT __fastcall HandleDuplicate(HOBJECT handle) {
  CHandleObject *ptr;

  if (!handle) {
    return 0;
  }

  ptr = HandleDereference(handle);
  ptr->IncRef();
  return reinterpret_cast<HOBJECT>(ptr);
}

void __fastcall HandleInitialize() {
}

int __fastcall HandleObjectCompare(HOBJECT object1, HOBJECT object2) {
  return object1 - object2;
}
