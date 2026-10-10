#ifndef WOW_SOURCE_WOWSERVICES_BITFIELD_H
#define WOW_SOURCE_WOWSERVICES_BITFIELD_H

#include <stpl.h>

#include <string.h>

template <class ARRAY>
class TSBitField {
 public:
  TSBitField(UINT numBits = 0) : m_numBits(0) {
    if (numBits) {
      SetCount(numBits);
    }
  }

  void SetAll() {
    if (m_numBits) {
      memset(m_array.Ptr(), 0xFF, m_array.Count() * m_array.SizeOfElement());
    }
  }

  void ClearAll() {
    if (m_numBits) {
      memset(m_array.Ptr(), 0, m_array.Count() * m_array.SizeOfElement());
    }
  }

  void SetBit(UINT bitNum) {
    UINT arrayIndex;
    UINT bitIndex;
    ComputeIndices(bitNum, arrayIndex, bitIndex);
    m_array[arrayIndex] |= 1 << bitIndex;
  }

  void ClearBit(UINT bitNum) {
    UINT arrayIndex;
    UINT bitIndex;
    ComputeIndices(bitNum, arrayIndex, bitIndex);
    m_array[arrayIndex] &= ~(1 << bitIndex);
  }

  bool IsBitSet(UINT bitNum) const {
    UINT arrayIndex;
    UINT bitIndex;
    ComputeIndices(bitNum, arrayIndex, bitIndex);
    return (m_array[arrayIndex] & (1 << bitIndex)) != 0;
  }

  bool IsBitClear(UINT bitNum) const {
    return !IsBitSet(bitNum);
  }

  void SetCount(UINT numBits) {
    m_numBits = numBits;
    m_array.SetCount((numBits - 1) / (m_array.SizeOfElement() * 8) + 1);
  }

  void Clear() {
    m_numBits = 0;
    m_array.SetCount(0);
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
    FATALASSERT(bitNum < m_numBits);
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
