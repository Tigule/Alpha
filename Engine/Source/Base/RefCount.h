#ifndef ENGINE_SOURCE_BASE_REFCOUNT_H
#define ENGINE_SOURCE_BASE_REFCOUNT_H

#include <storm.h>

class TRefCnt {
 public:
  virtual void DeleteSelf();

  void IncrRef() {
    ++m_refcnt;
  }

  void DecrRef() {
    if (!--m_refcnt) {
      DeleteSelf();
    }
  }

 protected:
  TRefCnt() : m_refcnt(0) {
  }

  TRefCnt(const TRefCnt &) : m_refcnt(0) {
  }

  virtual ~TRefCnt() {
  }

  unsigned long m_refcnt;
};

template <class T>
class TRefCntPtr {
 public:
  TRefCntPtr(T *ptr = 0) : m_ptr(ptr) {
    if (m_ptr) {
      m_ptr->IncrRef();
    }
  }

  TRefCntPtr(const TRefCntPtr<T> &ptr) : m_ptr(ptr.m_ptr) {
    if (m_ptr) {
      m_ptr->IncrRef();
    }
  }

  ~TRefCntPtr() {
    if (m_ptr) {
      m_ptr->DecrRef();
    }
  }

  TRefCntPtr<T> &operator=(T *rhs) {
    if (rhs) {
      rhs->IncrRef();
    }

    if (m_ptr) {
      m_ptr->DecrRef();
    }

    m_ptr = rhs;
    return *this;
  }

  TRefCntPtr<T> &operator=(const TRefCntPtr<T> &rhs) {
    if (rhs.m_ptr) {
      rhs.m_ptr->IncrRef();
    }

    if (m_ptr) {
      m_ptr->DecrRef();
    }

    m_ptr = rhs.m_ptr;
    return *this;
  }

  T &operator*() {
    ASSERT(m_ptr);
    return *m_ptr;
  }

  const T &operator*() const {
    ASSERT(m_ptr);
    return *m_ptr;
  }

  T *operator->() {
    ASSERT(m_ptr);
    return m_ptr;
  }

  const T *operator->() const {
    ASSERT(m_ptr);
    return m_ptr;
  }

  unsigned char operator==(const T *rhs) const {
    return m_ptr == rhs;
  }

  unsigned char operator==(const TRefCntPtr<T> &rhs) const {
    return m_ptr == rhs.m_ptr;
  }

  unsigned char operator!=(const T *rhs) const {
    return m_ptr != rhs;
  }

  unsigned char operator!=(const TRefCntPtr<T> &rhs) const {
    return m_ptr != rhs.m_ptr;
  }

  unsigned char operator!() const {
    return m_ptr == 0;
  }

  operator T *() {
    return m_ptr;
  }

  operator const T *() const {
    return m_ptr;
  }

  T *m_ptr;
};

#endif
