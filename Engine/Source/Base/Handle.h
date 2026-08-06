#pragma once

#include <storm.h>

DECLARE_STRICT_HANDLE(HOBJECT);

class CHandleObject {
 public:
  CHandleObject() : m_refcount(0) {
  }

  CHandleObject(const CHandleObject &) : m_refcount(0) {
  }

  CHandleObject &operator=(const CHandleObject &) {
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

 private:
  int m_refcount;
};

void           HandleClose(HOBJECT handle);
HOBJECT        HandleCreate(CHandleObject *ptr, LPCSTR handleName);
CHandleObject *HandleDereference(HOBJECT handle);
void           HandleDestroy();
HOBJECT        HandleDuplicate(HOBJECT handle);
void           HandleInitialize();
int            HandleObjectCompare(HOBJECT object1, HOBJECT object2);
