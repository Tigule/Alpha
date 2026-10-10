#include <Base/Base.h>

#include "Handle.h"

void HandleClose(HOBJECT handle) {
  CHandleObject *ptr = HandleDereference(handle);

  ptr->DecRef();
}

HOBJECT HandleCreate(CHandleObject *ptr, LPCSTR handleName) {
  VALIDATEBEGIN;
  VALIDATE(ptr);
  VALIDATEEND;

  ptr->IncRef();
  return (HOBJECT)ptr;
}

CHandleObject *HandleDereference(HOBJECT handle) {
  return (CHandleObject *)handle;
}

void HandleDestroy() {
}

HOBJECT HandleDuplicate(HOBJECT handle) {
  CHandleObject *ptr;

  if (!handle) {
    return 0;
  }

  ptr = HandleDereference(handle);
  ptr->IncRef();
  return (HOBJECT)ptr;
}

void HandleInitialize() {
}

int HandleObjectCompare(HOBJECT object1, HOBJECT object2) {
  return object1 - object2;
}
