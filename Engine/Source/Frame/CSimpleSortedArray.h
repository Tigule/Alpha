#ifndef ENGINE_SOURCE_FRAME_CSIMPLESORTEDARRAY_H
#define ENGINE_SOURCE_FRAME_CSIMPLESORTEDARRAY_H

#include <stpl.h>

template <class T>
class CSimpleSortedArray {
 public:
  CSimpleSortedArray() : m_count(0), m_maxcount(0), m_iterator(0) {
  }

  ~CSimpleSortedArray() {
    m_array.Clear();
  }

  UINT Count() {
    return m_count;
  }

  void Insert(T value) {
    UINT index;
    UINT valuePriority = value->SimpleSortedArrayValue();

    if (m_count == m_maxcount) {
      m_maxcount += 8;
      m_array.SetCount(m_maxcount);
    }

    for (index = 0; index < m_count; ++index) {
      if (valuePriority > m_array[index]->SimpleSortedArrayValue()) {
        UINT move = m_count;

        while (move > index) {
          m_array[move] = m_array[move - 1];
          --move;
        }
        break;
      }
    }

    m_array[index] = value;
    if (index < m_iterator) {
      ++m_iterator;
    }
    ++m_count;
  }

  void Remove(UINT index) {
    UINT move;

    ASSERT(index < m_count);
    for (move = index; move + 1 < m_count; ++move) {
      m_array[move] = m_array[move + 1];
    }

    if (index < m_iterator) {
      --m_iterator;
    }

    --m_count;
    if (m_maxcount - m_count > 8) {
      m_maxcount -= 8;
      m_array.SetCount(m_maxcount);
    }
  }

  void IterateBegin() {
    m_iterator = 0;
  }

  T *IterateNext() {
    if (m_iterator >= m_count) {
      return 0;
    }

    return &m_array[m_iterator++];
  }

  T &operator[](UINT index) {
    ASSERT(index < m_count);
    return m_array[index];
  }

 protected:
  TSGrowableArray<T> m_array;
  UINT               m_count;
  UINT               m_maxcount;
  UINT               m_iterator;
};

#endif
