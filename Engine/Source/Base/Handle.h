#pragma once

#include <Base/Base.h>

DECLARE_STRICT_HANDLE(HOBJECT);

// names unconfirmed
#define NEWHANDLE(handle, struct) new (SMemAlloc(sizeof(struct), #handle, SERR_LINECODE_OBJECT, 0)) struct
#define CREATEHANDLE(handle, ptr) ((handle)HandleCreate(ptr, #handle))

void           HandleClose(HOBJECT handle);
HOBJECT        HandleCreate(CHandleObject *ptr, LPCSTR handleName);
CHandleObject *HandleDereference(HOBJECT handle);
void           HandleDestroy();
HOBJECT        HandleDuplicate(HOBJECT handle);
void           HandleInitialize();
int            HandleObjectCompare(HOBJECT object1, HOBJECT object2);
