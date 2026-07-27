#pragma once

#include <storm.h>

#include <string.h>
#include <typeinfo>

namespace NTempest {

  class CEntity {
   public:
    virtual ~CEntity() {
    }
  };

  class CMemBlock : public CEntity {
   public:
    CMemBlock(unsigned long bsize, unsigned long prologue, const char *filen, long linen);
    CMemBlock(const CMemBlock &m);
    virtual ~CMemBlock();

    CMemBlock &operator=(const CMemBlock &m);

    unsigned long Copy(const CMemBlock &from);
    long          Compare(const CMemBlock &to) const;
    unsigned long Copy_(const CMemBlock &from);
    long          Compare_(const CMemBlock &to) const;
    bool          Swap(CMemBlock &with);

    bool IsValid() const {
      return mem_ != 0;
    }
    char *Get() {
      return mem;
    }
    unsigned long Size() {
      return size;
    }
    void Set(char *dst, unsigned char value, unsigned long bytes) {
      SetM_(dst, value, bytes);
    }
    void Set(unsigned char value) {
      SetM_(mem, value, size);
    }
    void Set32(unsigned long *dst, unsigned long value, unsigned long bytes) {
      SetM_(dst, value, bytes);
    }
    void Set32(unsigned long value) {
      SetM_(reinterpret_cast<unsigned long *>(mem), value, size);
    }
    void Zero(char *dst, unsigned long bytes) {
      SetM_(dst, 0, bytes);
    }
    void Zero() {
      SetM_(mem, 0, size);
    }
    void Copy(char *dst, char *src, unsigned long bytes) {
      memmove(dst, src, bytes);
    }
    long Compare(char *a, char *b, unsigned long bytes) {
      return memcmp(a, b, bytes);
    }
    char *Get_() {
      return mem_;
    }
    unsigned long Size_() {
      return size_;
    }
    unsigned long Prologue_() {
      return size_ - size;
    }
    void Set_(unsigned char value) {
      SetM_(mem_, value, size_);
    }
    void Set32_(unsigned long value) {
      SetM_(reinterpret_cast<unsigned long *>(mem_), value, size_);
    }
    void Zero_() {
      SetM_(mem_, 0, size_);
    }

    bool Resize(unsigned long newsize, bool preserve);
    void Detach(char *&mem, unsigned long &size);
    void Attach(char *mem, unsigned long size);
    void Detach_(char *&mem, unsigned long &size, char *&mem_, unsigned long &size_);
    void Attach_(char *mem, unsigned long size, char *mem_, unsigned long size_);

    const char *FileN_() const;
    long        LineN_() const;
    void        SetFileN_(const char *filen);
    void        SetLineN_(long linen);

   protected:
    static void Set32b_(char *c, unsigned char value, unsigned long size);
    static void Set32b_(unsigned long *d, unsigned long c, unsigned long size);
    static void SetM_(char *c, unsigned char value, unsigned long size);
    static void SetM_(unsigned long *d, unsigned long c, unsigned long size);

    void Constructor_(unsigned long bsize, unsigned long prologue, const char *filen, long linen);
    void Destructor_();

    char         *mem_;
    unsigned long size_;
    char         *mem;
    unsigned long size;
    const char   *filen_;
    long          linen_;
  };

  template <class T>
  class CMemBlockT : public CMemBlock {
   public:
    CMemBlockT(unsigned long count = 0, unsigned long prologue = 0, const char *filen = 0, long linen = 0)
        : CMemBlock(count * sizeof(T), prologue, filen ? filen : typeid(T).raw_name(), filen ? linen : SERR_LINECODE_OBJECT) {
    }

    T *Get() {
      return reinterpret_cast<T *>(CMemBlock::Get());
    }

    T &operator[](unsigned long index) {
      ASSERT(index < CMemBlock::Size() / sizeof(T));
      return Get()[index];
    }

    bool Resize(unsigned long count, bool preserve) {
      return CMemBlock::Resize(count * sizeof(T), preserve);
    }

  };

#if defined(_M_IX86) || defined(__i386__)
#endif

}  // namespace NTempest
