#include <Base/Base.h>

#include "cmemblock.h"

#include <string.h>

namespace NTempest {

  void CMemBlock::Set32b_(char *c, BYTE d, DWORD size) {
    DWORD prefix = -reinterpret_cast<DWORD>(c) & 3;
    DWORD suffix = (size - prefix) & 3;
    DWORD body = size - suffix - prefix;

    switch (prefix) {
      case 3:
        c[2] = d;
      case 2:
        c[1] = d;
      case 1:
        c[0] = d;
    }

    c += prefix;

    if (body) {
      ASSERT((body & 0x3) == 0);

      DWORD word = d;
      word |= word << 8;
      word |= word << 16;
      Set32b_(reinterpret_cast<DWORD *>(c), word, body);
      c += body;
    }

    switch (suffix) {
      case 3:
        c[2] = d;
      case 2:
        c[1] = d;
      case 1:
        c[0] = d;
    }
  }

  void CMemBlock::Set32b_(DWORD *c, DWORD d, DWORD size) {
    DWORD count = size >> 4;

    while (count) {
      c[0] = d;
      c[1] = d;
      c[2] = d;
      c[3] = d;
      c += 4;
      --count;
    }

    switch ((size >> 2) & 3) {
      case 3:
        c[2] = d;
      case 2:
        c[1] = d;
      case 1:
        c[0] = d;
    }
  }

  void CMemBlock::SetM_(char *c, BYTE d, DWORD size) {
    if (size >= 16) {
      Set32b_(c, d, size);
      return;
    }

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
  }

  void CMemBlock::SetM_(DWORD *d, DWORD c, DWORD size) {
    ASSERT((size & 0x3) == 0);
    Set32b_(d, c, size);
  }

  void CMemBlock::Constructor_(DWORD bsize, DWORD prologue, LPCSTR filen, long linen) {
    SetFileN_(filen);
    SetLineN_(linen);
    Destructor_();

    DWORD allocSize = bsize + prologue;
    mem_ = allocSize ? static_cast<char *>(SMemAlloc(allocSize, FileN_(), LineN_(), SMEM_FLAG_ZEROMEMORY)) : reinterpret_cast<char *>(-1);
    ASSERT(mem_ != 0);

    size_ = allocSize;
    size = bsize;
    mem = mem_ + prologue;
  }

  void CMemBlock::Destructor_() {
    if (mem_) {
      ASSERT(mem_ <= mem && size_ >= size);

      if (mem_ != reinterpret_cast<char *>(-1)) {
        SMemFree(mem_, FileN_(), LineN_(), 0);
      }

      size = 0;
      size_ = 0;
      mem = 0;
      mem_ = 0;
    }
  }

  CMemBlock::CMemBlock(DWORD bsize, DWORD prologue, LPCSTR filen, long linen) : mem_(0) {
    Constructor_(bsize, prologue, filen, linen);
  }

  CMemBlock::CMemBlock(const CMemBlock &m) : mem_(0) {
    ASSERT(!IsValid());
    ASSERT(m.IsValid());
    ASSERT(m.mem_ <= m.mem && m.size_ >= m.size);
    Constructor_(m.size, m.size_ - m.size, m.FileN_(), m.LineN_());
    if (mem_) {
      ASSERT(Copy_(m) == size_);
    }
  }

  CMemBlock &CMemBlock::operator=(const CMemBlock &m) {
    ASSERT(m.IsValid());
    ASSERT(m.mem_ <= m.mem && m.size_ >= m.size);
    ASSERT(mem_ <= mem && size_ >= size);
    Destructor_();
    Constructor_(m.size, m.size_ - m.size, m.FileN_(), m.LineN_());
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

      if (mem_ == reinterpret_cast<char *>(-1)) {
        mem_ = static_cast<char *>(SMemAlloc(allocSize, FileN_(), LineN_(), SMEM_FLAG_ZEROMEMORY));
      } else {
        mem_ = static_cast<char *>(SMemReAlloc(mem_, allocSize, FileN_(), LineN_(), SMEM_FLAG_ZEROMEMORY));
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

  void CMemBlock::Detach(char *&detachedMem, DWORD &detachedSize) {
    ASSERT(size_ == size);
    detachedMem = mem_;
    detachedSize = size_;
    mem_ = 0;
    mem = 0;
    size_ = 0;
    size = 0;
  }

  void CMemBlock::Attach(char *attachedMem, DWORD attachedSize) {
    ASSERT(!IsValid());
    ASSERT(attachedMem != 0);
    mem_ = attachedMem;
    mem = attachedMem;
    size_ = attachedSize;
    size = attachedSize;
  }

  void CMemBlock::Detach_(char *&detachedMem, DWORD &detachedSize, char *&detachedMem_, DWORD &detachedSize_) {
    detachedMem_ = mem_;
    detachedMem = mem;
    detachedSize_ = size_;
    detachedSize = size;
    mem_ = 0;
    mem = 0;
    size_ = 0;
    size = 0;
  }

  void CMemBlock::Attach_(char *attachedMem, DWORD attachedSize, char *attachedMem_, DWORD attachedSize_) {
    ASSERT(!IsValid());
    ASSERT(attachedSize_ >= attachedSize);
    ASSERT(attachedMem == attachedMem_ + (attachedSize_ - attachedSize));
    mem_ = attachedMem_;
    mem = attachedMem;
    size_ = attachedSize_;
    size = attachedSize;
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
