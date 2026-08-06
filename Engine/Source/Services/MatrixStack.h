#pragma once

#include <stpl.h>

template <class T>
class CMatrixStack {
 public:
  CMatrixStack() {
    m_stack.New();
  }

  ~CMatrixStack() {
    ASSERT(m_stack.Count() == 1);
  }

  void Push() {
    T *entry = m_stack.New();
    *entry = entry[-1];
  }

  void Pop() {
    UINT stackCount = m_stack.Count();

    ASSERT(stackCount > 1);
    m_stack.SetCount(stackCount - 1);
  }

  void Load(const T &value) {
    *m_stack.Top() = value;
  }

  void Mult(const T &value);
  void Remove(UINT removeFlags);
  void Identity();

  T &Get() {
    return *m_stack.Top();
  }

  const T &Get() const;

  void Get(T *value) const {
    *value = *m_stack.Top();
  }

 private:
  TSGrowableArray<T> m_stack;
};
