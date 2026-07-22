#ifndef WOW_SOURCE_WOWSERVICES_BITFIELD_H
#define WOW_SOURCE_WOWSERVICES_BITFIELD_H

#include <stpl.h>

#include <string.h>

template <class ARRAY>
class TSBitField {
 public:
  TSBitField() : m_numBits(0) {
  }

  void SetCount(unsigned int numBits) {
    m_numBits = numBits;
    if (numBits) {
      m_array.SetCount((numBits - 1) / (m_array.SizeOfElement() * 8) + 1);
    } else {
      m_array.Clear();
    }
  }

  void ClearAll() {
    if (m_numBits) {
      memset(m_array.Ptr(), 0, m_array.Count() * m_array.SizeOfElement());
    }
  }

  void SetAll() {
    if (m_numBits) {
      memset(m_array.Ptr(), 0xFF, m_array.Count() * m_array.SizeOfElement());
    }
  }

  void SetBit(unsigned int bitNum) {
    FATALASSERT(bitNum < m_numBits);
    m_array[bitNum >> 5] |= 1 << (bitNum & 31);
  }

  void ClearBit(unsigned int bitNum) {
    FATALASSERT(bitNum < m_numBits);
    m_array[bitNum >> 5] &= ~(1 << (bitNum & 31));
  }

  unsigned int IsSet(unsigned int bitNum) const {
    FATALASSERT(bitNum < m_numBits);
    return (m_array[bitNum >> 5] & (1 << (bitNum & 31))) != 0;
  }

  void SetData(const void *data, unsigned int byteCount) {
    ASSERT((byteCount & (m_array.SizeOfElement() - 1)) == 0);
    SetCount(byteCount * 8);
    memcpy(m_array.Ptr(), data, byteCount);
  }

 protected:
  unsigned int m_numBits;
  ARRAY        m_array;
};

typedef TSBitField<TSFixedArray<unsigned int> > FBitField;

#endif
