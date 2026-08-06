#pragma once

#include "cmemblock.h"

namespace NTempest {

  class CIterator {
   public:
    CIterator();
    DWORD Index() const;
    void  SetIndex(DWORD index);

   protected:
    DWORD iscan;
  };

  class CDynParms {
   public:
    CDynParms(DWORD prealloc_ = 32, DWORD expandf_ = 32) : prealloc(prealloc_), expandf(expandf_) {
    }

    DWORD Prealloc() const {
      return prealloc;
    }

    DWORD ExpandF() const {
      return expandf;
    }

    void SetPrealloc(DWORD);
    void SetExpandF(DWORD);

   protected:
    DWORD prealloc;
    DWORD expandf;
  };

  template <class T>
  class CDynTable : public CMemBlockT<T> {
   public:
    using CMemBlockT<T>::IsValid;

    typedef long(__cdecl *TSort)(const T *, const T *);

    enum {
      eLessThan = -1,
      eEqualTo = 0,
      eGreaterThan = 1
    };

    static long __cdecl MemSortP(const T *, const T *);
    static long __cdecl Int32SortP(const long *, const long *);
    static long __cdecl UInt32SortP(const DWORD *, const DWORD *);

    CDynTable(const CDynTable &other) : CMemBlockT<T>(other), expand(other.expand), iallocated(other.iallocated), iused(other.iused) {
    }
    CDynTable(const CDynParms &dp = CDynParms(), DWORD prologue = 0, LPCSTR filen = 0, long linen = 0)
        : CMemBlockT<T>(dp.Prealloc(), prologue, filen, linen), expand(dp.ExpandF()), iallocated(dp.Prealloc()), iused(0) {
    }

    CDynTable &operator=(const CDynTable &other) {
      CMemBlockT<T>::operator=(other);
      expand = other.expand;
      iallocated = other.iallocated;
      iused = other.iused;
      return *this;
    }

    DWORD Expansion() const {
      return expand;
    }

    DWORD OutIndex() const {
      return -1;
    }

    DWORD EntrySize() const {
      return sizeof(T);
    }

    DWORD Allocated() const {
      return iallocated;
    }

    DWORD Unused() const {
      return iallocated - iused;
    }

    DWORD Used() const {
      ASSERT(IsValid());
      return iused;
    }

    void SetExpansion(DWORD expansion) {
      expand = expansion;
    }

    bool Swap(CMemBlock &other) {
      return CMemBlock::Swap(other);
    }

    bool Swap(CDynTable &other) {
      if (!CMemBlock::Swap(other)) {
        return false;
      }

      DWORD value = expand;
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

    bool Resize(DWORD allocated, bool preserve) {
      if (!CMemBlockT<T>::Resize(allocated, preserve)) {
        return false;
      }
      iallocated = allocated;
      if (iused > iallocated) {
        iused = iallocated;
      }
      return true;
    }

    T &operator[](DWORD i) const {
      ASSERT(IsValid() && i < iused);
      return reinterpret_cast<T *>(this->mem)[i];
    }

    T *GetEntry(DWORD i) const {
      return i < iused ? &reinterpret_cast<T *>(this->mem)[i] : 0;
    }

    void SetEntry(DWORD at, const T *entry, DWORD count = 1) const {
      ASSERT(entry);
      ASSERT(at + count <= iused);
      while (count--) {
        reinterpret_cast<T *>(this->mem)[at++] = *entry;
      }
    }

    void SetEntry(DWORD at, const T &entry, DWORD count = 1) const {
      SetEntry(at, &entry, count);
    }

    void SetAllEntries(const T *entry) const {
      SetEntry(0, entry, iused);
    }

    void SetAllEntries(const T &entry) const {
      SetAllEntries(&entry);
    }

    bool SwapEntries(DWORD a, DWORD b) const {
      if (a >= iused || b >= iused) {
        return false;
      }
      T value = (*this)[a];
      (*this)[a] = (*this)[b];
      (*this)[b] = value;
      return true;
    }

    long CompareEntries(DWORD a, DWORD b, const TSort compare) const {
      ASSERT(a < iused && b < iused && compare);
      return compare(&(*this)[a], &(*this)[b]);
    }

    long CompareEntries(const T *a, const T *b, const TSort compare) const {
      ASSERT(a && b && compare);
      return compare(a, b);
    }

    bool Grow(const T *entry = 0, DWORD count = 1) {
      ASSERT(IsValid());

      if (!count) {
        return true;
      }

      if (iused + count > iallocated) {
        if (!expand) {
          return false;
        }

        DWORD toexpand = iused + count - iallocated;
        if (toexpand < expand) {
          toexpand = expand;
        }

        if (!Resize(iallocated + toexpand, true)) {
          return false;
        }
      }

      DWORD at = iused;
      iused += count;

      if (entry) {
        ASSERT(IsValid());

        DWORD end = at + count;
        if (end > iused) {
          end = iused;
        }

        while (at < end) {
          reinterpret_cast<T *>(this->mem)[at++] = *entry;
        }
      }

      return true;
    }

    bool Grow(const T &entry, DWORD count = 1) {
      return Grow(&entry, count);
    }

    bool GrowAll(const T *entry) {
      return Grow(entry, Unused());
    }

    bool GrowAll(const T &entry) {
      return GrowAll(&entry);
    }

    bool Insert(DWORD at, const T *entry, DWORD count = 1) {
      if (at > iused || !Grow(0, count)) {
        return false;
      }
      memmove(&reinterpret_cast<T *>(this->mem)[at + count], &reinterpret_cast<T *>(this->mem)[at], sizeof(T) * (iused - at - count));
      SetEntry(at, entry, count);
      return true;
    }

    bool Insert(DWORD at, const T &entry, DWORD count = 1) {
      return Insert(at, &entry, count);
    }

    bool Remove(DWORD at, DWORD count) {
      ASSERT(IsValid());

      if (at >= iused) {
        return false;
      }

      if (at + count > iused) {
        count = iused - at;
      }

      DWORD moventries = iused - at - count;
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

    DWORD Optimize();
    bool  Search(const T *entry, DWORD &at, const TSort compare);
    bool  Search(const T &entry, DWORD &at, const TSort compare);
    bool  Sort(const TSort compare);
    bool  BeginScan(CIterator &iterator);
    T    *Current(CIterator &iterator);
    DWORD CurrentIndex(CIterator &iterator);
    bool  Goto(DWORD at, CIterator &iterator);
    bool  Previous(CIterator &iterator);
    bool  Next(CIterator &iterator);
    bool  SearchBackwards(const T *entry, CIterator &iterator, const TSort compare);
    bool  SearchBackwards(const T &entry, CIterator &iterator, const TSort compare);
    bool  SearchForward(const T *entry, CIterator &iterator, const TSort compare);
    bool  SearchForward(const T &entry, CIterator &iterator, const TSort compare);
    void  EndScan(CIterator &iterator);

   protected:
    T *Item_(DWORD i) const {
      return GetEntry(i);
    }

    DWORD expand;
    DWORD iallocated;
    DWORD iused;
  };

#if defined(_M_IX86) || defined(__i386__)
#endif

}  // namespace NTempest
