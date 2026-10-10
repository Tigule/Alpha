#pragma once

#include "cmemblock.h"

namespace NTempest {

  class CIterator {
   protected:
    DWORD iscan;

   public:
    CIterator();
    DWORD Index() const;
    void  SetIndex(DWORD index);
  };

  class CDynParms {
   protected:
    DWORD prealloc;
    DWORD expandf;

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
  };

  template <class T>
  class CDynTable : public CMemBlockT<T> {
   protected:
    DWORD expand;
    DWORD iallocated;
    DWORD iused;

    T *Item_(DWORD i) const {
      return GetEntry(i);
    }
   public:

    enum {
      eLessThan = -1,
      eEqualTo = 0,
      eGreaterThan = 1
    };

    static long __cdecl MemSortP(const T *, const T *);
    static long __cdecl Int32SortP(const long *, const long *);
    static long __cdecl UInt32SortP(const DWORD *, const DWORD *);

    typedef long(__cdecl *TSort)(const T *, const T *);
    using CMemBlockT<T>::IsValid;

    CDynTable(const CDynParms &dp = CDynParms(), DWORD prologue = 0, LPCSTR filen = 0, long linen = 0)
        : CMemBlockT<T>(dp.Prealloc(), prologue, filen, linen) {
      expand = dp.ExpandF();
      iallocated = dp.Prealloc();
      iused = 0;
    }

    CDynTable(const CDynTable &other) : CMemBlockT<T>(other), expand(other.expand), iallocated(other.iallocated), iused(other.iused) {
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

    bool Swap(CMemBlock &other) {
      return CMemBlock::Swap(other);
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
      return ((T *)this->mem)[i];
    }

    T *GetEntry(DWORD i) const {
      return i < iused ? &((T *)this->mem)[i] : 0;
    }

    void SetEntry(DWORD at, const T &entry, DWORD count = 1) const {
      SetEntry(at, &entry, count);
    }

    void SetEntry(DWORD at, const T *entry, DWORD count = 1) const {
      ASSERT(IsValid());

      DWORD end = min(at + count, iused);

      for (; at < end; ++at) {
        ((T *)this->mem)[at] = *entry;
      }
    }

    void SetAllEntries(const T &entry) const {
      SetAllEntries(&entry);
    }

    void SetAllEntries(const T *entry) const {
      SetEntry(0, entry, iused);
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

    long CompareEntries(const T *a, const T *b, const TSort compare) const {
      ASSERT(a && b && compare);
      return compare(a, b);
    }

    long CompareEntries(DWORD a, DWORD b, const TSort compare) const {
      ASSERT(a < iused && b < iused && compare);
      return compare(&(*this)[a], &(*this)[b]);
    }

    bool Grow(const T &entry, DWORD count = 1) {
      return Grow(&entry, count);
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

        DWORD toexpand = max(expand, iused - iallocated + count);

        if (!CMemBlock::Resize((iallocated + toexpand) * sizeof(T), true)) {
          return false;
        }

        iallocated += toexpand;
      }

      DWORD i = iused;
      iused += count;

      if (entry) {
        SetEntry(i, entry, iused);
      }

      return true;
    }

    bool GrowAll(const T &entry) {
      return GrowAll(&entry);
    }

    bool GrowAll(const T *entry) {
      return Grow(entry, Unused());
    }

    bool Insert(DWORD at, const T &entry, DWORD count = 1) {
      return Insert(at, &entry, count);
    }

    bool Insert(DWORD at, const T *entry, DWORD count = 1) {
      if (at > iused || !Grow(0, count)) {
        return false;
      }
      memmove(&((T *)this->mem)[at + count], &((T *)this->mem)[at], sizeof(T) * (iused - at - count));
      SetEntry(at, entry, count);
      return true;
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
      if (moventries) {
        memmove(&(*this)[at], &(*this)[at + count], sizeof(T) * moventries);
      }

      iused -= count;
      return true;
    }

    bool RemoveLast() {
      if (!iused) {
        return false;
      }

      return Remove(iused - 1, 1);
    }

    bool RemoveAll() {
      iused = 0;
      return true;
    }

    DWORD Optimize();
    bool  Search(const T &entry, DWORD &at, const TSort compare);
    bool  Search(const T *entry, DWORD &at, const TSort compare);
    bool  Sort(const TSort compare);
    bool  BeginScan(CIterator &iterator);
    T    *Current(CIterator &iterator);
    DWORD CurrentIndex(CIterator &iterator);
    bool  Goto(DWORD at, CIterator &iterator);
    bool  Previous(CIterator &iterator);
    bool  Next(CIterator &iterator);
    bool  SearchBackwards(const T &entry, CIterator &iterator, const TSort compare);
    bool  SearchBackwards(const T *entry, CIterator &iterator, const TSort compare);
    bool  SearchForward(const T &entry, CIterator &iterator, const TSort compare);
    bool  SearchForward(const T *entry, CIterator &iterator, const TSort compare);
    void  EndScan(CIterator &iterator);
  };

#if defined(_M_IX86) || defined(__i386__)
#endif

}  // namespace NTempest
