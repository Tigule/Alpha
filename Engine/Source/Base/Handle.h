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

  int GetRefCount() {
    return m_refcount;
  }

  virtual const char *GetObjectName();

 private:
  int m_refcount;
};

void __fastcall           HandleClose(HOBJECT handle);
HOBJECT __fastcall        HandleCreate(CHandleObject *ptr, const char *handleName);
CHandleObject *__fastcall HandleDereference(HOBJECT handle);
void __fastcall           HandleDestroy();
HOBJECT __fastcall        HandleDuplicate(HOBJECT handle);
void __fastcall           HandleInitialize();
int __fastcall            HandleObjectCompare(HOBJECT object1, HOBJECT object2);
