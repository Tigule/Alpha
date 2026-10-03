#pragma once

#include <storm.h>

DECLARE_STRICT_HANDLE(HOBJECT);

// names unconfirmed
#define NEWHANDLE(handle, struct) new (SMemAlloc(sizeof(struct), #handle, SERR_LINECODE_OBJECT, 0)) struct
#define CREATEHANDLE(handle, ptr) ((handle)HandleCreate(ptr, #handle))

class CHandleObject {
 private:
  int m_refcount;

 public:
  CHandleObject() : m_refcount(0) {
  }

  CHandleObject(const CHandleObject &) : m_refcount(0) {
  }

  CHandleObject &operator=(const CHandleObject &) {
    m_refcount = 0;
    return *this;
  }

  virtual ~CHandleObject() {
  }

  void DecRef() {
    ASSERT(m_refcount > 0);

    if (!--m_refcount) {
      DEL(this);
    }
  }

  void IncRef() {
    ++m_refcount;
  }

  int GetRefCount() const {
    return m_refcount;
  }

  virtual LPCSTR GetObjectName();

};

void           HandleClose(HOBJECT handle);
HOBJECT        HandleCreate(CHandleObject *ptr, LPCSTR handleName);
CHandleObject *HandleDereference(HOBJECT handle);
void           HandleDestroy();
HOBJECT        HandleDuplicate(HOBJECT handle);
void           HandleInitialize();
int            HandleObjectCompare(HOBJECT object1, HOBJECT object2);
