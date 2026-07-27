#pragma once

#include "cdyntable.h"

namespace NTempest {

  template <class T, class B>
  class CPriorityQ : public CDynTable<T> {
   public:
    using CDynTable<T>::Grow;
    using CDynTable<T>::Used;

    enum {
      eRootIndex = 1
    };

    CPriorityQ(const CPriorityQ &other) : CDynTable<T>(other) {
    }
    CPriorityQ(const CDynParms &dp = CDynParms()) : CDynTable<T>(dp, 0, 0, 0) {
      if (!this->IsValid()) {
        SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "\"CPriorityQ<T, B>: construction of base class failed.\"", FALSE);
      }

      ASSERT(Grow());
    }

    virtual ~CPriorityQ() {
    }

    void Validate() const;

    bool HasEntries() const {
      ASSERT(Used() > 0);
      return Used() > eRootIndex;
    }

    unsigned long EntriesInQueue() const {
      ASSERT(Used() > 0);
      return Used() - eRootIndex;
    }
    T Root() const {
      ASSERT(HasEntries());
      return CDynTable<T>::operator[](eRootIndex);
    }

    void Enqueue(T value) {
      Grow();

      unsigned long entry = Used() - 1;
      while (entry > eRootIndex) {
        unsigned long parent = entry >> 1;
        if (!B::HasHigherPriority(value, CDynTable<T>::operator[](parent))) {
          break;
        }

        CDynTable<T>::operator[](entry) = CDynTable<T>::operator[](parent);
        entry = parent;
      }

      CDynTable<T>::operator[](entry) = value;
    }

    T Dequeue() {
      T root = CDynTable<T>::operator[](eRootIndex);
      T value = CDynTable<T>::operator[](CDynTable<T>::Used() - 1);

      CDynTable<T>::RemoveLast();

      if (CDynTable<T>::Used() >= 2) {
        unsigned long entry = eRootIndex;
        unsigned long hbound = CDynTable<T>::Used() - 1;
        unsigned long lbound = hbound >> 1;

        while (entry <= lbound) {
          unsigned long child = entry << 1;
          if (child < hbound && B::HasHigherPriority(CDynTable<T>::operator[](child + 1), CDynTable<T>::operator[](child))) {
            ++child;
          }

          if (B::HasHigherPriority(value, CDynTable<T>::operator[](child))) {
            break;
          }

          CDynTable<T>::operator[](entry) = CDynTable<T>::operator[](child);
          entry = child;
        }

        CDynTable<T>::operator[](entry) = value;
      }

      return root;
    }

    void DiscardAll() {
      if (Used() > eRootIndex) {
        CDynTable<T>::Remove(eRootIndex, Used() - eRootIndex);
      }
    }
  };

}  // namespace NTempest
