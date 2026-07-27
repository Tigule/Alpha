#pragma once

#include "cmemblock.h"

namespace NTempest {

  class CIterator;

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

    CDynTable(const CDynTable &other)
        : CMemBlockT<T>(other), expand(other.expand), iallocated(other.iallocated), iused(other.iused) {
    }
    CDynTable(const CDynParms &dp = CDynParms(), unsigned long prologue = 0, const char *filen = 0, long linen = 0)
        : CMemBlockT<T>(dp.Prealloc(), prologue, filen, linen), expand(dp.ExpandF()), iallocated(dp.Prealloc()), iused(0) {
    }

    CDynTable &operator=(const CDynTable &other) {
      CMemBlockT<T>::operator=(other);
      expand = other.expand;
      iallocated = other.iallocated;
      iused = other.iused;
      return *this;
    }

    unsigned long Expansion() {
      return expand;
    }

    unsigned long OutIndex() {
      return static_cast<unsigned long>(-1);
    }

    unsigned long EntrySize() {
      return sizeof(T);
    }

    unsigned long Allocated() {
      return iallocated;
    }

    unsigned long Unused() {
      return iallocated - iused;
    }

    unsigned long Used() const {
      ASSERT(IsValid());
      return iused;
    }

    void SetExpansion(unsigned long expansion) {
      expand = expansion;
    }

    bool Swap(CMemBlock &other) {
      return CMemBlock::Swap(other);
    }

    bool Swap(CDynTable &other) {
      if (!CMemBlock::Swap(other)) {
        return false;
      }

      unsigned long value = expand;
      expand = other.expand;
      other.expand = value;
      value = iallocated;
      iallocated = other.iallocated;
      other.iallocated = value;
      value = iused;
      iused = other.iused;
      other.iused = value;
      return true;
    }

    bool Resize(unsigned long allocated, bool preserve) {
      if (!CMemBlockT<T>::Resize(allocated, preserve)) {
        return false;
      }
      iallocated = allocated;
      if (iused > iallocated) {
        iused = iallocated;
      }
      return true;
    }

    T &operator[](unsigned long i) const {
      ASSERT(IsValid() && i < iused);
      return reinterpret_cast<T *>(this->mem)[i];
    }

    T *GetEntry(unsigned long i) {
      return i < iused ? &reinterpret_cast<T *>(this->mem)[i] : 0;
    }

    void SetEntry(unsigned long at, const T *entry, unsigned long count = 1) {
      ASSERT(entry);
      ASSERT(at + count <= iused);
      while (count--) {
        reinterpret_cast<T *>(this->mem)[at++] = *entry;
      }
    }

    void SetEntry(unsigned long at, const T &entry, unsigned long count = 1) {
      SetEntry(at, &entry, count);
    }

    void SetAllEntries(const T *entry) {
      SetEntry(0, entry, iused);
    }

    void SetAllEntries(const T &entry) {
      SetAllEntries(&entry);
    }

    bool SwapEntries(unsigned long a, unsigned long b) {
      if (a >= iused || b >= iused) {
        return false;
      }
      T value = (*this)[a];
      (*this)[a] = (*this)[b];
      (*this)[b] = value;
      return true;
    }

    long CompareEntries(unsigned long a, unsigned long b, long (*compare)(const T *, const T *)) {
      ASSERT(a < iused && b < iused && compare);
      return compare(&(*this)[a], &(*this)[b]);
    }

    long CompareEntries(const T *a, const T *b, long (*compare)(const T *, const T *)) {
      ASSERT(a && b && compare);
      return compare(a, b);
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

        if (!Resize(iallocated + toexpand, true)) {
          return false;
        }
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

    bool Grow(const T &entry, unsigned long count = 1) {
      return Grow(&entry, count);
    }

    bool GrowAll(const T *entry) {
      return Grow(entry, Unused());
    }

    bool GrowAll(const T &entry) {
      return GrowAll(&entry);
    }

    bool Insert(unsigned long at, const T *entry, unsigned long count = 1) {
      if (at > iused || !Grow(0, count)) {
        return false;
      }
      memmove(&reinterpret_cast<T *>(this->mem)[at + count], &reinterpret_cast<T *>(this->mem)[at], sizeof(T) * (iused - at - count));
      SetEntry(at, entry, count);
      return true;
    }

    bool Insert(unsigned long at, const T &entry, unsigned long count = 1) {
      return Insert(at, &entry, count);
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

    bool RemoveAll() {
      iused = 0;
      return true;
    }

    unsigned long Optimize();
    bool Search(const T *entry, unsigned long &at, long (*compare)(const T *, const T *));
    bool Search(const T &entry, unsigned long &at, long (*compare)(const T *, const T *));
    bool Sort(long (*compare)(const T *, const T *));
    bool BeginScan(CIterator &iterator);
    T   *Current(CIterator &iterator);
    unsigned long CurrentIndex(CIterator &iterator);
    bool Goto(unsigned long at, CIterator &iterator);
    bool Previous(CIterator &iterator);
    bool Next(CIterator &iterator);
    bool SearchBackwards(const T *entry, CIterator &iterator, long (*compare)(const T *, const T *));
    bool SearchBackwards(const T &entry, CIterator &iterator, long (*compare)(const T *, const T *));
    bool SearchForward(const T *entry, CIterator &iterator, long (*compare)(const T *, const T *));
    bool SearchForward(const T &entry, CIterator &iterator, long (*compare)(const T *, const T *));
    void EndScan(CIterator &iterator);

   protected:
    T *Item_(unsigned long i) {
      return GetEntry(i);
    }

    unsigned long expand;
    unsigned long iallocated;
    unsigned long iused;
  };

#if defined(_M_IX86) || defined(__i386__)
#endif

}  // namespace NTempest
