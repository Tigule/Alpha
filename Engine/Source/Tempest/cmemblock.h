#pragma once

#include <storm.h>

#include <string.h>

namespace NTempest {

  class CEntity {
   public:
    CEntity() {
    }
    CEntity(const CEntity &) {
    }
    virtual ~CEntity() {
    }
  };

  class CMemBlock : public CEntity {
   public:
    CMemBlock(DWORD bsize, DWORD prologue, LPCSTR filen, long linen);
    CMemBlock(const CMemBlock &m);
    virtual ~CMemBlock();

    CMemBlock &operator=(const CMemBlock &m);

    static char *Allocate(DWORD size, LPCSTR filen, long linen);
    static void  Dispose(char *mem, LPCSTR filen, long linen);

    DWORD Copy(const CMemBlock &from);
    long  Compare(const CMemBlock &to) const;
    DWORD Copy_(const CMemBlock &from);
    long  Compare_(const CMemBlock &to) const;
    bool  Swap(CMemBlock &with);

    bool IsValid() const {
      return mem_ != 0;
    }
    char *Get() const {
      return mem;
    }
    DWORD Size() const {
      return size;
    }
    void Set(BYTE value) {
      SetM_(mem, value, size);
    }
    static void Set(char *dst, BYTE value, DWORD bytes) {
      SetM_(dst, value, bytes);
    }
    void Set32(DWORD value) {
      SetM_(reinterpret_cast<DWORD *>(mem), value, size);
    }
    static void Set32(DWORD *dst, DWORD value, DWORD bytes) {
      SetM_(dst, value, bytes);
    }
    void Zero() {
      SetM_(mem, 0, size);
    }
    static void Zero(char *dst, DWORD bytes) {
      SetM_(dst, 0, bytes);
    }
    static void Copy(char *dst, char *const src, DWORD bytes) {
      memmove(dst, src, bytes);
    }
    static long Compare(char *const a, char *const b, DWORD bytes) {
      return memcmp(a, b, bytes);
    }
    char *Get_() const {
      return mem_;
    }
    DWORD Size_() const {
      return size_;
    }
    DWORD Prologue_() const {
      return size_ - size;
    }
    void Set_(BYTE value) {
      SetM_(mem_, value, size_);
    }
    void Set32_(DWORD value) {
      SetM_(reinterpret_cast<DWORD *>(mem_), value, size_);
    }
    void Zero_() {
      SetM_(mem_, 0, size_);
    }

    bool Resize(DWORD newsize, bool preserve);
    void Detach(char *&mem, DWORD &size);
    void Attach(char *mem, DWORD size);
    void Detach_(char *&mem, DWORD &size, char *&mem_, DWORD &size_);
    void Attach_(char *mem, DWORD size, char *mem_, DWORD size_);

    LPCSTR FileN_() const;
    long   LineN_() const;
    void   SetFileN_(LPCSTR filen);
    void   SetLineN_(long linen);

   protected:
    static void Set32b_(char *c, BYTE value, DWORD size);
    static void Set32b_(DWORD *d, DWORD c, DWORD size);
    static void SetM_(char *c, BYTE value, DWORD size);
    static void SetM_(DWORD *d, DWORD c, DWORD size);

    void Constructor_(DWORD bsize, DWORD prologue, LPCSTR filen, long linen);
    void Destructor_();

    char  *mem_;
    DWORD  size_;
    char  *mem;
    DWORD  size;
    LPCSTR filen_;
    long   linen_;
  };

  template <class T>
  class CMemBlockT : public CMemBlock {
   public:
    CMemBlockT(DWORD count = 0, DWORD prologue = 0, LPCSTR filen = 0, long linen = 0)
        : CMemBlock(count * sizeof(T), prologue, filen ? filen : typeid(T).INTERNALRAWNAME(), filen ? linen : SERR_LINECODE_OBJECT) {
    }

    T *Get() const {
      return reinterpret_cast<T *>(CMemBlock::Get());
    }

    T &operator[](DWORD index) const {
      ASSERT(index < CMemBlock::Size() / sizeof(T));
      return Get()[index];
    }

    bool Resize(DWORD count, bool preserve) {
      return CMemBlock::Resize(count * sizeof(T), preserve);
    }
  };

#if defined(_M_IX86) || defined(__i386__)
#endif

}  // namespace NTempest
