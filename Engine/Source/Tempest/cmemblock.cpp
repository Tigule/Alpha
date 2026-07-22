#include "cmemblock.h"

namespace NTempest {

  void __fastcall CMemBlock::Set32b_(char *c, unsigned char value, unsigned long size) {
    unsigned long prefix = -reinterpret_cast<unsigned long>(c) & 3;
    unsigned long suffix = (size - prefix) & 3;
    unsigned long body = size - suffix - prefix;

    switch (prefix) {
      case 3:
        c[2] = value;
      case 2:
        c[1] = value;
      case 1:
        c[0] = value;
    }

    c += prefix;

    if (body) {
      ASSERT((body & 0x3) == 0);

      unsigned long word = value;
      word |= word << 8;
      word |= word << 16;
      Set32b_(reinterpret_cast<unsigned long *>(c), word, body);
      c += body;
    }

    switch (suffix) {
      case 3:
        c[2] = value;
      case 2:
        c[1] = value;
      case 1:
        c[0] = value;
    }
  }

  void __fastcall CMemBlock::Set32b_(unsigned long *c, unsigned long value, unsigned long size) {
    unsigned long count = size >> 4;

    while (count) {
      c[0] = value;
      c[1] = value;
      c[2] = value;
      c[3] = value;
      c += 4;
      --count;
    }

    switch ((size >> 2) & 3) {
      case 3:
        c[2] = value;
      case 2:
        c[1] = value;
      case 1:
        c[0] = value;
    }
  }

  void __fastcall CMemBlock::SetM_(char *c, unsigned char value, unsigned long size) {
    if (size >= 16) {
      Set32b_(c, value, size);
      return;
    }

    switch (size) {
      case 15:
        c[14] = value;
      case 14:
        c[13] = value;
      case 13:
        c[12] = value;
      case 12:
        c[11] = value;
      case 11:
        c[10] = value;
      case 10:
        c[9] = value;
      case 9:
        c[8] = value;
      case 8:
        c[7] = value;
      case 7:
        c[6] = value;
      case 6:
        c[5] = value;
      case 5:
        c[4] = value;
      case 4:
        c[3] = value;
      case 3:
        c[2] = value;
      case 2:
        c[1] = value;
      case 1:
        c[0] = value;
    }
  }

  void CMemBlock::Constructor_(unsigned long bsize, unsigned long prologue, const char *filen, long linen) {
    SetFileN_(filen);
    SetLineN_(linen);
    Destructor_();

    unsigned long allocSize = bsize + prologue;
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

  CMemBlock::CMemBlock(unsigned long bsize, unsigned long prologue, const char *filen, long linen) : mem_(0) {
    Constructor_(bsize, prologue, filen, linen);
  }

  CMemBlock::~CMemBlock() {
    Destructor_();
  }

  bool CMemBlock::Resize(unsigned long newsize, bool preserve) {
    if (newsize != size) {
      ASSERT(IsValid());

      unsigned long prologue = size_ - size;
      unsigned long allocSize = prologue + newsize;

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

  const char *CMemBlock::FileN_() const {
    return filen_;
  }

  long CMemBlock::LineN_() const {
    return linen_;
  }

  void CMemBlock::SetFileN_(const char *filen) {
    filen_ = filen;
  }

  void CMemBlock::SetLineN_(long linen) {
    linen_ = linen;
  }

}  // namespace NTempest
