#ifndef WOW_SOURCE_WOWSERVICES_BITFIELD_H
#define WOW_SOURCE_WOWSERVICES_BITFIELD_H

#include <stpl.h>

#include <string.h>

template <class ARRAY>
class TSBitField {
 public:
  TSBitField(UINT numBits = 0) : m_numBits(0) {
    SetCount(numBits);
  }

  void SetCount(UINT numBits) {
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

  void SetBit(UINT bitNum) {
    FATALASSERT(bitNum < m_numBits);
    m_array[bitNum >> 5] |= 1 << (bitNum & 31);
  }

  void ClearBit(UINT bitNum) {
    FATALASSERT(bitNum < m_numBits);
    m_array[bitNum >> 5] &= ~(1 << (bitNum & 31));
  }

  bool IsBitSet(UINT bitNum) const {
    FATALASSERT(bitNum < m_numBits);
    return (m_array[bitNum >> 5] & (1 << (bitNum & 31))) != 0;
  }

  bool IsBitClear(UINT bitNum) const {
    return !IsBitSet(bitNum);
  }

  void Clear() {
    m_numBits = 0;
    m_array.Clear();
  }

  void Load(LPCVOID data, UINT byteCount) {
    ASSERT((byteCount & (m_array.SizeOfElement() - 1)) == 0);
    SetCount(byteCount * 8);
    memcpy(m_array.Ptr(), data, byteCount);
  }

  void Save(LPVOID &data, UINT &byteCount) {
    data = m_array.Ptr();
    byteCount = m_array.Count() * m_array.SizeOfElement();
  }

 protected:
  void ComputeIndices(UINT bitNum, UINT &arrayIndex, UINT &bitIndex) const {
    arrayIndex = bitNum >> 5;
    bitIndex = bitNum & 31;
  }

  UINT  m_numBits;
  ARRAY m_array;
};

class FBitField : public TSBitField<TSFixedArray<UINT> > {
 public:
  FBitField(UINT numBits = 0) : TSBitField<TSFixedArray<UINT> >(numBits) {
  }
};

#endif
