#pragma once

#include <storm.h>

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
    static void __fastcall Set32b_(char *c, unsigned char value, unsigned long size);
    static void __fastcall Set32b_(unsigned long *d, unsigned long c, unsigned long size);
    static void __fastcall SetM_(char *c, unsigned char value, unsigned long size);
    static void __fastcall SetM_(unsigned long *d, unsigned long c, unsigned long size);

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
    CMemBlockT(unsigned long count, unsigned long prologue = 0, const char *filen = 0, long linen = 0)
        : CMemBlock(count * sizeof(T), prologue, filen ? filen : typeid(T).raw_name(), filen ? linen : SERR_LINECODE_OBJECT) {
    }

    virtual ~CMemBlockT() {
    }
  };

#if defined(_M_IX86) || defined(__i386__)
#endif

}  // namespace NTempest
