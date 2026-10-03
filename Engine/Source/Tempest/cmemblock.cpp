#include <Base/Base.h>

#include "cmemblock.h"

#include <string.h>

namespace NTempest {

  void CMemBlock::Set32b_(char *d, BYTE c, DWORD size) {
    DWORD prefix = -reinterpret_cast<DWORD>(d) & 3;
    DWORD suffix = (size - prefix) & 3;
    DWORD body = size - suffix - prefix;

    switch (prefix) {
      case 3:
        d[2] = c;
      case 2:
        d[1] = c;
      case 1:
        d[0] = c;
    }

    d += prefix;

    if (body) {
      ASSERT((body & 0x3) == 0);

      Set32b_(reinterpret_cast<DWORD *>(d), ((static_cast<DWORD>(c) << 8 | c) << 8 | c) << 8 | c, body);
      d += body;
    }

    switch (suffix) {
      case 3:
        d[2] = c;
      case 2:
        d[1] = c;
      case 1:
        d[0] = c;
    }
  }

  void CMemBlock::Set32b_(DWORD *d, DWORD c, DWORD size) {
    DWORD body = size >> 2;
    DWORD count = body >> 2;
    DWORD suffix = body - (count << 2);

    for (; count > 0; --count) {
      d[0] = c;
      d[1] = c;
      d[2] = c;
      d[3] = c;
      d += 4;
    }

    switch (suffix) {
      case 3:
        d[2] = c;
      case 2:
        d[1] = c;
      case 1:
        d[0] = c;
    }
  }

  void CMemBlock::SetM_(char *c, BYTE d, DWORD size) {
    if (size < 16) {
      switch (size) {
      case 15:
        c[14] = d;
      case 14:
        c[13] = d;
      case 13:
        c[12] = d;
      case 12:
        c[11] = d;
      case 11:
        c[10] = d;
      case 10:
        c[9] = d;
      case 9:
        c[8] = d;
      case 8:
        c[7] = d;
      case 7:
        c[6] = d;
      case 6:
        c[5] = d;
      case 5:
        c[4] = d;
      case 4:
        c[3] = d;
      case 3:
        c[2] = d;
      case 2:
        c[1] = d;
      case 1:
        c[0] = d;
      }
    } else {
      Set32b_(c, d, size);
    }
  }

  void CMemBlock::SetM_(DWORD *d, DWORD c, DWORD size) {
    ASSERT((size & 0x3) == 0);
    Set32b_(d, c, size);
  }

  inline char *CMemBlock::Allocate(DWORD size, LPCSTR filen, long linen) {
    return size ? static_cast<char *>(SMemAlloc(size, filen, linen, SMEM_FLAG_ZEROMEMORY)) : reinterpret_cast<char *>(-1);
  }

  inline void CMemBlock::Dispose(char *mem, LPCSTR filen, long linen) {
    if (mem != reinterpret_cast<char *>(-1)) {
      SMemFree(mem, filen, linen, 0);
    }
  }

  void CMemBlock::Constructor_(DWORD bsize, DWORD prologue, LPCSTR filen, long linen) {
    SetFileN_(filen);
    SetLineN_(linen);
    Destructor_();

    mem_ = Allocate(bsize + prologue, FileN_(), LineN_());
    ASSERT(mem_ != 0);

    size_ = bsize + prologue;
    size = bsize;
    mem = mem_ + prologue;
  }

  void CMemBlock::Destructor_() {
    if (mem_) {
      ASSERT(mem_ <= mem && size_ >= size);

      Dispose(mem_, FileN_(), LineN_());

      size = 0;
      size_ = 0;
      mem = 0;
      mem_ = 0;
    }
  }

  CMemBlock::CMemBlock(DWORD bsize, DWORD prologue, LPCSTR filen, long linen) : mem_(0) {
    Constructor_(bsize, prologue, filen, linen);
  }

  CMemBlock::CMemBlock(const CMemBlock &m) {
    ASSERT(IsValid() == false);
    ASSERT(m.IsValid());
    ASSERT(m.mem_ <= m.mem && m.size_ >= m.size);
    Constructor_(m.size, m.Prologue_(), m.FileN_(), m.LineN_());
    if (mem_) {
      ASSERT(Copy_(m) == size_);
    }
  }

  CMemBlock &CMemBlock::operator=(const CMemBlock &m) {
    ASSERT(m.IsValid());
    ASSERT(m.mem_ <= m.mem && m.size_ >= m.size);
    ASSERT(mem_ <= mem && size_ >= size);
    Destructor_();
    Constructor_(m.size, m.Prologue_(), m.FileN_(), m.LineN_());
    if (mem_) {
      ASSERT(Copy_(m) == size_);
    }
    return *this;
  }

  CMemBlock::~CMemBlock() {
    Destructor_();
  }

  DWORD CMemBlock::Copy(const CMemBlock &from) {
    ASSERT(IsValid());
    ASSERT(from.IsValid());
    DWORD copySize = size < from.size ? size : from.size;
    memmove(mem, from.mem, copySize);
    return copySize;
  }

  long CMemBlock::Compare(const CMemBlock &to) const {
    ASSERT(IsValid());
    ASSERT(to.IsValid());
    DWORD compareSize = size < to.size ? size : to.size;
    return memcmp(mem, to.mem, compareSize);
  }

  DWORD CMemBlock::Copy_(const CMemBlock &from) {
    ASSERT(IsValid());
    ASSERT(from.IsValid());
    DWORD copySize = size_ < from.size_ ? size_ : from.size_;
    memmove(mem_, from.mem_, copySize);
    return copySize;
  }

  long CMemBlock::Compare_(const CMemBlock &to) const {
    ASSERT(IsValid());
    ASSERT(to.IsValid());
    DWORD compareSize = size_ < to.size_ ? size_ : to.size_;
    return memcmp(mem_, to.mem_, compareSize);
  }

  bool CMemBlock::Swap(CMemBlock &with) {
    ASSERT(IsValid());
    ASSERT(with.IsValid());
    ASSERT(size_ >= size && with.size_ >= with.size);
    ASSERT(mem_ <= mem && with.mem_ <= with.mem);

    char *oldMem_ = mem_;
    char *oldMem = mem;
    DWORD oldSize_ = size_;
    DWORD oldSize = size;
    mem_ = with.mem_;
    mem = with.mem;
    size_ = with.size_;
    size = with.size;
    with.mem_ = oldMem_;
    with.mem = oldMem;
    with.size_ = oldSize_;
    with.size = oldSize;
    return true;
  }

  bool CMemBlock::Resize(DWORD newsize, bool preserve) {
    if (newsize != size) {
      ASSERT(IsValid());

      DWORD prologue = size_ - size;
      DWORD allocSize = prologue + newsize;

      if (mem_ != reinterpret_cast<char *>(-1)) {
        mem_ = static_cast<char *>(SMemReAlloc(mem_, allocSize, FileN_(), LineN_(), SMEM_FLAG_ZEROMEMORY));
      } else {
        mem_ = static_cast<char *>(SMemAlloc(allocSize, FileN_(), LineN_(), SMEM_FLAG_ZEROMEMORY));
      }

      mem = mem_ + prologue;
      ASSERT(mem_ != 0);
      size_ = allocSize;
      size = newsize;
    }

    if (!preserve) {
      SetM_(mem, 0, size);
    }

    return true;
  }

  void CMemBlock::Detach(char *&mem, DWORD &size) {
    ASSERT(this->size_ == this->size);
    mem = this->mem_;
    this->mem_ = this->mem = 0;
    size = this->size_;
    this->size_ = this->size = 0;
  }

  void CMemBlock::Attach(char *mem, DWORD size) {
    ASSERT(IsValid() == false);
    ASSERT(mem != 0);
    this->mem_ = this->mem = mem;
    this->size_ = this->size = size;
  }

  void CMemBlock::Detach_(char *&mem, DWORD &size, char *&mem_, DWORD &size_) {
    mem_ = this->mem_;
    this->mem_ = 0;
    mem = this->mem;
    this->mem = 0;
    size_ = this->size_;
    this->size_ = 0;
    size = this->size;
    this->size = 0;
  }

  void CMemBlock::Attach_(char *mem, DWORD size, char *mem_, DWORD size_) {
    ASSERT(IsValid() == false);
    ASSERT(size_ >= size);
    ASSERT(mem == mem_ + (size_ - size));
    this->mem_ = mem_;
    this->mem = mem;
    this->size_ = size_;
    this->size = size;
  }

  LPCSTR CMemBlock::FileN_() const {
    return filen_;
  }

  long CMemBlock::LineN_() const {
    return linen_;
  }

  void CMemBlock::SetFileN_(LPCSTR filen) {
    filen_ = filen;
  }

  void CMemBlock::SetLineN_(long linen) {
    linen_ = linen;
  }

}  // namespace NTempest
