#pragma once

#include "cmemblock.h"

namespace NTempest {

  class CDynParms {
   public:
    CDynParms(unsigned long prealloc_ = 32, unsigned long expandf_ = 32) : prealloc(prealloc_), expandf(expandf_) {
    }

    unsigned long Prealloc() const {
      return prealloc;
    }

    unsigned long ExpandF() const {
      return expandf;
    }

   protected:
    unsigned long prealloc;
    unsigned long expandf;
  };

  template <class T>
  class CDynTable : public CMemBlockT<T> {
   public:
    using CMemBlockT<T>::IsValid;

    CDynTable(const CDynTable &other);
    CDynTable(const CDynParms &dp = CDynParms(), unsigned long prologue = 0, const char *filen = 0, long linen = 0)
        : CMemBlockT<T>(dp.Prealloc(), prologue, filen, linen), expand(dp.ExpandF()), iallocated(dp.Prealloc()), iused(0) {
    }

    virtual ~CDynTable() {
    }

    CDynTable &operator=(const CDynTable &other);

    unsigned long Used() const {
      ASSERT(IsValid());
      return iused;
    }

    T &operator[](unsigned long i) const {
      ASSERT(IsValid() && i < iused);
      return reinterpret_cast<T *>(this->mem)[i];
    }

    bool Grow(const T *entry = 0, unsigned long count = 1) {
      ASSERT(IsValid());

      if (!count) {
        return true;
      }

      if (iused + count > iallocated) {
        if (!expand) {
          return false;
        }

        unsigned long toexpand = iused + count - iallocated;
        if (toexpand < expand) {
          toexpand = expand;
        }

        if (!this->Resize(sizeof(T) * (iallocated + toexpand), true)) {
          return false;
        }

        iallocated += toexpand;
      }

      unsigned long at = iused;
      iused += count;

      if (entry) {
        ASSERT(IsValid());

        unsigned long end = at + count;
        if (end > iused) {
          end = iused;
        }

        while (at < end) {
          reinterpret_cast<T *>(this->mem)[at++] = *entry;
        }
      }

      return true;
    }

    bool Remove(unsigned long at, unsigned long count) {
      ASSERT(IsValid());

      if (at >= iused) {
        return false;
      }

      if (at + count > iused) {
        count = iused - at;
      }

      unsigned long moventries = iused - at - count;
      if (iused - at != count) {
        memmove(&(*this)[at], &(*this)[at + count], sizeof(T) * moventries);
      }

      iused -= count;
      return true;
    }

    bool RemoveLast() {
      return iused ? Remove(iused - 1, 1) : false;
    }

   protected:
    unsigned long expand;
    unsigned long iallocated;
    unsigned long iused;
  };

#if defined(_M_IX86) || defined(__i386__)
#endif

}  // namespace NTempest
