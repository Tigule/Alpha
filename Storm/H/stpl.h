#pragma once

#include "storm.h"

#include <new>
#include <string.h>

class CSBasePriorityQueue;

template <class T>
class TSStackArray {
 private:

  UINT m_maxCount;
 protected:

  UINT m_count;

  T   *m_data;

  void FatalArrayBounds() const {
    SErrDisplayError(STORM_ERROR_ACCESS_OUT_OF_BOUNDS, typeid(T).INTERNALRAWNAME(), SERR_LINECODE_OBJECT, 0, TRUE, 1);
  }
 public:

  TSStackArray(LPVOID data, UINT maxCount, int count) : m_maxCount(maxCount), m_count(count), m_data(static_cast<T *>(data)) {
    for (UINT index = 0; index < m_count; ++index) {
      new (&m_data[index]) T;
    }
  }

  ~TSStackArray() {
    for (UINT index = 0; index < m_count; ++index) {
      m_data[index].~T();
    }
  }

  TSStackArray<T> &operator=(const TSStackArray<T> &source) {
    if (this != &source) {
      Set(source.Count(), source.Ptr());
    }
    return *this;
  }

  T &operator[](UINT index) {
    if (index >= m_count) {
      FatalArrayBounds();
    }
    return m_data[index];
  }

  const T &operator[](UINT index) const {
    if (index >= m_count) {
      FatalArrayBounds();
    }
    return m_data[index];
  }

  UINT Count() const {
    return m_count;
  }

  UINT Bytes() const {
    return m_count * sizeof(T);
  }

  T *Ptr() {
    return m_data;
  }

  const T *Ptr() const {
    return m_data;
  }

  void Set(UINT count, const T *data) {
    Set(count, 0, data);
  }

  void Set(UINT count, int, const T *data) {
    SetCount(0);
    Add(count, 0, data);
  }

  void SetCount(UINT count) {
    if (count > m_maxCount) {
      FatalArrayBounds();
    }

    while (m_count > count) {
      m_data[--m_count].~T();
    }
    while (m_count < count) {
      new (&m_data[m_count++]) T;
    }
  }

  void Zero() {
    memset(m_data, 0, Bytes());
  }

  UINT SizeOfElement() const {
    return sizeof(T);
  }

  void Add(UINT count, const T *data) {
    Add(count, 0, data);
  }

  void Add(UINT count, int, const T *data) {
    if (m_count + count > m_maxCount) {
      FatalArrayBounds();
    }

    for (UINT index = 0; index < count; ++index) {
      new (&m_data[m_count++]) T(data[index]);
    }
  }

  T *New() {
    if (m_count >= m_maxCount) {
      FatalArrayBounds();
    }
    T *result = &m_data[m_count++];
    new (result) T;
    return result;
  }

  T *New(const T &value) {
    if (m_count >= m_maxCount) {
      FatalArrayBounds();
    }
    T *result = &m_data[m_count++];
    new (result) T(value);
    return result;
  }
};

template <class T, UINT MAXCOUNT>
class TSCArray {
 protected:
  UINT m_count;
  T    m_data[MAXCOUNT];

  LPCSTR MemFileName() const {
    return typeid(T).INTERNALRAWNAME();
  }

  int MemLineNo() const {
    return SERR_LINECODE_OBJECT;
  }

  void FatalArrayBounds() const {
    SErrDisplayError(STORM_ERROR_ACCESS_OUT_OF_BOUNDS, MemFileName(), MemLineNo(), 0, TRUE, 1);
  }

 public:
  TSCArray() : m_count(MAXCOUNT) {
  }

  TSCArray(const TSCArray<T, MAXCOUNT> &source) {
    Set(source.Count(), source.Ptr());
  }

  TSCArray<T, MAXCOUNT> &operator=(const TSCArray<T, MAXCOUNT> &source) {
    if (this != &source) {
      Set(source.Count(), source.Ptr());
    }
    return *this;
  }

  T &operator[](UINT index) {
    if (index >= m_count) {
      FatalArrayBounds();
    }
    return m_data[index];
  }

  const T &operator[](UINT index) const {
    if (index >= m_count) {
      FatalArrayBounds();
    }
    return m_data[index];
  }

  UINT MaxCount() const {
    return MAXCOUNT;
  }

  UINT Count() const {
    return m_count;
  }

  UINT Bytes() const {
    return m_count * sizeof(T);
  }

  T *Ptr() {
    return m_data;
  }

  const T *Ptr() const {
    return m_data;
  }

  void Set(UINT count, const T *data) {
    if (count > MAXCOUNT) {
      FatalArrayBounds();
    }
    for (UINT i = 0; i < count; ++i) {
      m_data[i] = data[i];
    }
    m_count = count;
  }

  void Set(UINT count, int, const T *data) {
    Set(count, data);
  }

  void SetCount(UINT count) {
    if (count > MAXCOUNT) {
      FatalArrayBounds();
    }
    m_count = count;
  }

  void Zero() {
    memset(m_data, 0, Bytes());
  }

  UINT SizeOfElement() const {
    return sizeof(T);
  }
};

template <class T>
class TSBaseArray {
 protected:
  UINT m_alloc;
  UINT m_count;
  T   *m_data;

  void Constructor() {
    m_alloc = 0;
    m_count = 0;
    m_data = 0;
  }

  virtual LPCSTR MemFileName() const {
    return typeid(T).INTERNALRAWNAME();
  }

  virtual int MemLineNo() const {
    return -2;
  }

 protected:
  void CheckArrayBounds(UINT index) const {
    if (index >= m_count) {
      SErrDisplayErrorFmt(0x85100080, MemFileName(), MemLineNo(), TRUE, 1, "index (0x%08X), array size (0x%08X)", index, m_count);
    }
  }

 public:
  UINT Count() const {
    return m_count;
  }

  T &operator[](UINT index) {
    CheckArrayBounds(index);
    return m_data[index];
  }

  const T &operator[](UINT index) const {
    CheckArrayBounds(index);
    return m_data[index];
  }

  UINT Bytes() const {
    return m_count * sizeof(T);
  }

  T *Ptr() {
    return m_data;
  }

  const T *Ptr() const {
    return m_data;
  }

  UINT SizeOfElement() const {
    return sizeof(T);
  }

  T *Top() {
    return m_count ? &m_data[m_count - 1] : 0;
  }

  const T *Top() const {
    return m_count ? &m_data[m_count - 1] : 0;
  }

  UINT NumElements() const {
    return Count();
  }
};

template <class T>
class TSFixedArray : public TSBaseArray<T> {
 protected:
  void ReallocAndClearData(UINT count) {
    UINT index;

    for (index = 0; index < this->m_count; ++index) {
      this->m_data[index].~T();
    }

    this->m_alloc = count;
    if (this->m_data || count) {
      this->m_data = static_cast<T *>(SMemReAlloc(this->m_data, count * sizeof(T), this->MemFileName(), this->MemLineNo(), 0));
    }
  }
  void ReallocData(UINT count) {
    T   *oldData = this->m_data;
    UINT index;

    for (index = count; index < this->m_count; ++index) {
      (oldData + index)->~T();
    }

    this->m_alloc = count;
    this->m_data = static_cast<T *>(SMemReAlloc(oldData, count * sizeof(T), this->MemFileName(), this->MemLineNo(), 0x10));
    if (!this->m_data) {
      this->m_data = static_cast<T *>(SMemAlloc(count * sizeof(T), this->MemFileName(), this->MemLineNo(), 0));
      if (oldData) {
        UINT copyCount = min(count, this->m_count);

        for (index = 0; index < copyCount; ++index) {
          new (&this->m_data[index]) T(oldData[index]);
          (oldData + index)->~T();
        }
        SMemFree(oldData, this->MemFileName(), this->MemLineNo(), 0);
      }
    }
  }

 public:
  TSFixedArray() {
    this->Constructor();
  }

  TSFixedArray(const TSFixedArray<T> &source) {
    this->Constructor();
    Set(source.Count(), source.Ptr());
  }
  TSFixedArray(const TSBaseArray<T> &source) {
    this->Constructor();
    Set(source.Count(), source.Ptr());
  }

  ~TSFixedArray() {
    UINT index;

    for (index = 0; index < this->m_count; ++index) {
      this->m_data[index].~T();
    }

    if (this->m_data) {
      SMemFree(this->m_data, this->MemFileName(), this->MemLineNo(), 0);
    }
  }

  inline TSFixedArray<T> &operator=(const TSFixedArray<T> &source) {
    if (this != &source) {
      Set(source.Count(), source.Ptr());
    }
    return *this;
  }
  inline TSFixedArray<T> &operator=(const TSBaseArray<T> &source) {
    if (this != &source) {
      Set(source.Count(), source.Ptr());
    }
    return *this;
  }

  void Clear() {
    this->~TSFixedArray<T>();
    this->Constructor();
  }
  void Detach(T **data, UINT *count, UINT *alloc) {
    *data = this->m_data;
    *count = this->m_count;
    *alloc = this->m_alloc;
    this->m_data = 0;
    this->m_count = 0;
    this->m_alloc = 0;
  }
  void Exchange(TSFixedArray<T> *array) {
    T   *data = this->m_data;
    UINT count = this->m_count;
    UINT alloc = this->m_alloc;

    this->m_data = array->m_data;
    this->m_count = array->m_count;
    this->m_alloc = array->m_alloc;
    array->m_data = data;
    array->m_count = count;
    array->m_alloc = alloc;
  }
  void Set(UINT count, const T *data) {
    ReallocAndClearData(count);
    for (UINT index = 0; index < count; ++index) {
      new (&this->m_data[index]) T(data[index]);
    }
    this->m_count = count;
  }
  void Set(UINT count, int, const T *data) {
    Set(count, data);
  }
  void SetCount(UINT count) {
    UINT index;

    if (count == this->m_count) {
      return;
    }

    if (!count) {
      Clear();
      return;
    }

    ReallocData(count);
    for (index = this->m_count; index < count; ++index) {
      new (&this->m_data[index]) T;
    }
    this->m_count = count;
  }
  void SetOptional(UINT count, const T *data) {
    if (data) {
      Set(count, data);
    } else {
      SetCount(count);
    }
  }
  void Zero() {
    memset(this->m_data, 0, this->Bytes());
  }
};

template <class T>
class TSGrowableArray : public TSFixedArray<T> {
 private:
  UINT m_chunk;

  UINT CalcChunkSize(UINT count) {
    const UINT maxChunk = sizeof(T) < 0x20 ? 0x100 / sizeof(T) : 8;

    if (count < maxChunk) {
      while ((count - 1) & count) {
        count = (count - 1) & count;
      }
      if (count < 1) {
        count = 1;
      }
      return count;
    }

    m_chunk = maxChunk;
    return maxChunk;
  }

  UINT RoundToChunk(UINT count, UINT chunk) const {
    UINT remainder = count % chunk;

    if (remainder) {
      count += chunk - remainder;
    }

    return count;
  }

  void Reserve(UINT count, int round) {
    count += this->m_count;
    if (count <= this->m_alloc) {
      return;
    }

    if (round) {
      UINT chunk = m_chunk;

      if (!chunk) {
        chunk = CalcChunkSize(count);
      }
      count = RoundToChunk(count, chunk);
    }

    this->ReallocData(count);
  }

  friend class CSBasePriorityQueue;

 public:
  TSGrowableArray() {
    m_chunk = 0;
  }

  TSGrowableArray(const TSGrowableArray<T> &source) : TSFixedArray<T>(source), m_chunk(source.m_chunk) {
  }

  UINT Add(UINT count, const T *data) {
    Reserve(count, 1);
    for (UINT index = 0; index < count; ++index) {
      new (&this->m_data[this->m_count + index]) T(data[index]);
    }
    this->m_count += count;
    return this->m_count - count;
  }
  UINT Add(UINT count, int incr, const T *data) {
    Reserve(count, 1);
    for (UINT index = 0; index < count; ++index) {
      new (&this->m_data[this->m_count + index]) T(*data);
      data += incr;
    }
    this->m_count += count;
    return this->m_count - count;
  }

  UINT Add(const T *data) {
    return Add(1, data);
  }

  void GrowToFit(UINT index, int zero) {
    if (index >= this->m_count) {
      Reserve(index - this->m_count + 1, 1);
      if (zero) {
        memset(&this->m_data[this->m_count], 0, (index - this->m_count + 1) * sizeof(T));
      }
      this->m_count = index + 1;
    }
  }

  T *New() {
    Reserve(1, 1);
    return new (&this->m_data[this->m_count++]) T;
  }

  T *New(const T &source) {
    Reserve(1, 1);
    T *value = &this->m_data[this->m_count++];
    if (value) {
      new (value) T(source);
    }
    return value;
  }

  UINT Reserved() const {
    ASSERT(m_alloc >= m_count);
    return m_alloc - m_count;
  }

  void ReserveSpace(UINT count) {
    Reserve(count, 0);
  }

  void SetChunkSize(UINT chunk) {
    m_chunk = chunk;
  }

  void SetCount(UINT count) {
    UINT index;

    if (count > this->m_count) {
      Reserve(count - this->m_count, 1);
      for (index = this->m_count; index < count; ++index) {
        new (&this->m_data[index]) T;
      }
    } else if (count < this->m_count) {
      for (index = count; index < this->m_count; ++index) {
        (this->m_data + index)->~T();
      }
    }

    this->m_count = count;
  }

  void TrimUnusedSpace() {
    this->ReallocData(this->m_count);
  }

  UINT AddElement(const T *data) {
    return Add(data);
  }

  UINT AddElements(UINT count, const T *data) {
    return Add(count, data);
  }

  T *NewElement() {
    return New();
  }

  void SetNumElements(UINT count) {
    SetCount(count);
  }
};

template <class T, UINT TAG, int LINE>
class TSFixedArray_ : public TSFixedArray<T> {
 public:
  TSFixedArray_<T, TAG, LINE> &operator=(const TSFixedArray<T> &source) {
    TSFixedArray<T>::operator=(source);
    return *this;
  }

  TSFixedArray_<T, TAG, LINE> &operator=(const TSFixedArray_<T, TAG, LINE> &source) {
    TSFixedArray<T>::operator=(source);
    return *this;
  }

 protected:
  virtual LPCSTR MemFileName() const {
    return s_name;
  }

  virtual int MemLineNo() const {
    return LINE;
  }

 private:
  static char s_name[5];
};

template <class T, UINT TAG, int LINE>
char TSFixedArray_<T, TAG, LINE>::s_name[5] = {
    static_cast<char>((TAG >> 24) & 0xFF), static_cast<char>((TAG >> 16) & 0xFF), static_cast<char>((TAG >> 8) & 0xFF), static_cast<char>(TAG & 0xFF),
    0
};

template <class T, UINT TAG, int LINE>
class TSGrowableArray_ : public TSGrowableArray<T> {
 public:
  TSGrowableArray_<T, TAG, LINE> &operator=(const TSGrowableArray<T> &source) {
    TSGrowableArray<T>::operator=(source);
    return *this;
  }

  TSGrowableArray_<T, TAG, LINE> &operator=(const TSGrowableArray_<T, TAG, LINE> &source) {
    TSGrowableArray<T>::operator=(source);
    return *this;
  }

 protected:
  virtual LPCSTR MemFileName() const {
    return s_name;
  }

  virtual int MemLineNo() const {
    return LINE;
  }

 private:
  static char s_name[5];
};

template <class T, UINT TAG, int LINE>
char TSGrowableArray_<T, TAG, LINE>::s_name[5] = {
    static_cast<char>((TAG >> 24) & 0xFF), static_cast<char>((TAG >> 16) & 0xFF), static_cast<char>((TAG >> 8) & 0xFF), static_cast<char>(TAG & 0xFF),
    0
};

class CSBasePriority {
 private:
  CSBasePriorityQueue *m_queue;
  UINT                 m_index;

  void Construct() {
    m_queue = 0;
    m_index = 0;
  }

 public:
  CSBasePriority() {
    Construct();
  }

  ~CSBasePriority();

  CSBasePriority &operator=(const CSBasePriority &);

  virtual int Compare(CSBasePriority *priority) const = 0;

  BOOL IsLinked() const {
    return m_queue != 0;
  }

  void Relink();

  void SetQueuePosition(CSBasePriorityQueue *queue, UINT index) {
    m_queue = queue;
    m_index = index;
  }

  void Unlink();
};

template <class T>
class TSTimerPriority : public CSBasePriority {
 private:
  T m_val;

 public:
  TSTimerPriority() : m_val(0) {
  }

  BOOL Compare(CSBasePriority *priority) const {
    TSTimerPriority<T> *timerPriority = static_cast<TSTimerPriority<T> *>(priority);
    return static_cast<long>(m_val - timerPriority->m_val) <= 0;
  }

  T Get() const {
    return m_val;
  }

  void Set(T val) {
    if (m_val != val) {
      m_val = val;
      Relink();
    }
  }
};

class CSBasePriorityQueue : public TSGrowableArray<LPVOID> {
 private:
  friend class CSBasePriority;

  enum {
    ROOT_INDEX = 0
  };

  UINT m_linkOffset;

  UINT Child(UINT index) const {
    return index * 2 + 1;
  }

  UINT Parent(UINT index) const {
    return (index - 1) >> 1;
  }

  CSBasePriority *Link(LPVOID value) const {
    return reinterpret_cast<CSBasePriority *>(reinterpret_cast<BYTE *>(value) + m_linkOffset);
  }

  CSBasePriority *Link(UINT index) const {
    return Link((*this)[index]);
  }

  void SetLink(UINT index) {
    Link(index)->SetQueuePosition(this, index);
  }

  void UnsetLink(UINT index) {
    Link(index)->SetQueuePosition(0, 0);
  }

  int Compare(CSBasePriority *left, CSBasePriority *right) {
    return left->Compare(right);
  }

 public:
  CSBasePriorityQueue(int linkOffset) : m_linkOffset(linkOffset) {
  }

  ~CSBasePriorityQueue();

  LPVOID Root() {
    return this->m_count ? this->m_data[ROOT_INDEX] : 0;
  }

  LPVOID Dequeue() {
    LPVOID val;

    if (!Count()) {
      return 0;
    }

    val = (*this)[ROOT_INDEX];
    Remove(ROOT_INDEX);
    return val;
  }

  void Enqueue(LPVOID val) {
    UINT index = Count();

    GrowToFit(index, 0);
    do {
      UINT parent = Parent(index);

      if (index <= 0 || Compare(Link(parent), Link(val))) {
        break;
      }
      (*this)[index] = (*this)[parent];
      SetLink(index);
      index = parent;
    } while (1);
    (*this)[index] = val;
    SetLink(index);
  }

  void Remove(UINT index) {
    UnsetLink(index);

    LPVOID top = *Top();

    SetCount(Count() - 1);
    if (Count() == index) {
      return;
    }

    UINT hBound = Count() - 1;
    UINT lBound = Parent(hBound);

    if (index < hBound) {
      while (index <= lBound) {
        UINT child = Child(index);

        if (child < hBound && Compare(Link(child + 1), Link(child))) {
          ++child;
        }
        if (Compare(Link(top), Link(child))) {
          break;
        }
        (*this)[index] = (*this)[child];
        SetLink(index);
        index = child;
      }
    }
    (*this)[index] = top;
    SetLink(index);
  }
};

template <class T>
class TSPriorityQueue : public CSBasePriorityQueue {
 public:
  T *operator[](UINT index) {
    return static_cast<T *>(CSBasePriorityQueue::operator[](index));
  }

  const T *operator[](UINT index) const {
    this->CheckArrayBounds(index);
    return static_cast<const T *>(this->m_data[index]);
  }

  TSPriorityQueue(int linkOffset) : CSBasePriorityQueue(linkOffset) {
  }

  T *Root() {
    return static_cast<T *>(CSBasePriorityQueue::Root());
  }

  T *Dequeue() {
    return static_cast<T *>(CSBasePriorityQueue::Dequeue());
  }

  void Enqueue(T *value) {
    CSBasePriorityQueue::Enqueue(value);
  }

  void Remove(UINT index) {
    CSBasePriorityQueue::Remove(index);
  }
};

inline CSBasePriorityQueue::~CSBasePriorityQueue() {
  UINT index;

  for (index = 0; index < this->m_count; ++index) {
    UnsetLink(index);
  }
}

inline CSBasePriority::~CSBasePriority() {
  Unlink();
}

inline void CSBasePriority::Relink() {
  CSBasePriorityQueue *queue = m_queue;

  if (queue) {
    LPVOID ptr = (*queue)[m_index];

    queue->Remove(m_index);
    queue->Enqueue(ptr);
  }
}

inline void CSBasePriority::Unlink() {
  if (m_queue) {
    m_queue->Remove(m_index);
  }
}

template <class T, class GETLINK>
class TSList;

template <class T>
class TSGetLink;

template <class T>
class TSGetExplicitLink;

template <class T>
class TSLinkedNode;

template <class T>
class TSLink {
  friend class TSList<T, TSGetLink<T> >;
  friend class TSList<T, TSGetExplicitLink<T> >;

 private:
  TSLink<T> *m_prevlink;
  T         *m_next;

  void Constructor() {
    m_prevlink = 0;
    m_next = 0;
  }

  void CopyConstructor(const TSLink<T> &) {
    Constructor();
  }

  TSLink<T> *NextLink(int linkoffset) const {
    if (reinterpret_cast<int>(m_next) <= 0) {
      return reinterpret_cast<TSLink<T> *>(~reinterpret_cast<int>(m_next));
    }

    if (linkoffset < 0) {
      linkoffset = reinterpret_cast<int>(this) - reinterpret_cast<int>(m_prevlink->m_next);
    }

    return reinterpret_cast<TSLink<T> *>(reinterpret_cast<int>(m_next) + linkoffset);
  }

 public:
  TSLink() {
    Constructor();
  }

  TSLink(const TSLink<T> &source) {
    CopyConstructor(source);
  }

  ~TSLink() {
    Unlink();
  }

  TSLink<T> &operator=(const TSLink<T> &) {
    return *this;
  }

  int IsLinked() const {
    return m_next != 0;
  }

  T *Next() {
    return reinterpret_cast<int>(m_next) > 0 ? m_next : 0;
  }

  const T *Next() const {
    return reinterpret_cast<int>(m_next) > 0 ? m_next : 0;
  }

  T *Prev() {
    return m_prevlink->m_prevlink->Next();
  }

  const T *Prev() const {
    return m_prevlink->m_prevlink->Next();
  }

  T *RawNext() {
    return m_next;
  }

  const T *RawNext() const {
    return m_next;
  }

  void Unlink() {
    if (m_prevlink) {
      NextLink(-1)->m_prevlink = m_prevlink;
      m_prevlink->m_next = m_next;
      m_prevlink = 0;
      m_next = 0;
    }
  }
};

template <class T>
class TSLinkedNode {
  friend class TSGetLink<T>;

 private:
  TSLink<T> m_link;

 public:
  ~TSLinkedNode() {
    Unlink();
  }

  BOOL IsLinked() const {
    return m_link.IsLinked();
  }

  T *Next() {
    return m_link.Next();
  }

  const T *Next() const {
    return m_link.Next();
  }

  T *Prev() {
    return m_link.Prev();
  }

  const T *Prev() const {
    return m_link.Prev();
  }

  T *RawNext() {
    return m_link.RawNext();
  }

  const T *RawNext() const {
    return m_link.RawNext();
  }

  void Unlink() {
    m_link.Unlink();
  }
};

template <class T>
class TSGetLink {
 public:
  static TSLink<T> *Link(const TSLinkedNode<T> *instance, int) {
    return &const_cast<TSLinkedNode<T> *>(instance)->m_link;
  }
};

template <class T>
class TSGetExplicitLink {
 public:
  static TSLink<T> *Link(LPCVOID instance, int linkoffset) {
    return reinterpret_cast<TSLink<T> *>(reinterpret_cast<BYTE *>(const_cast<LPVOID>(instance)) + linkoffset);
  }
};

template <class T, class GETLINK>
class TSList {
 private:
  int       m_linkoffset;
  TSLink<T> m_terminator;

  void Constructor() {
    m_linkoffset = 0;
    InitializeTerminator();
  }

  void CopyConstructor(const TSList<T, GETLINK> &source) {
    m_linkoffset = source.m_linkoffset;
    InitializeTerminator();
  }

  void InitializeTerminator() {
    m_terminator.m_prevlink = &m_terminator;
    m_terminator.m_next = reinterpret_cast<T *>(~reinterpret_cast<DWORD>(&m_terminator));
  }

  TSLink<T> *Link(const T *ptr) const {
    return ptr ? GETLINK::Link(ptr, m_linkoffset) : const_cast<TSLink<T> *>(&m_terminator);
  }

 protected:
  void SetLinkOffset(int linkoffset) {
    m_linkoffset = linkoffset;
    InitializeTerminator();
  }

 public:
  TSList() {
    Constructor();
  }

  TSList(const TSList<T, GETLINK> &source) {
    CopyConstructor(source);
  }

  TSList(int linkoffset) {
    Constructor();
    SetLinkOffset(linkoffset);
  }

  ~TSList() {
    UnlinkAll();
  }

  TSList<T, GETLINK> &operator=(const TSList<T, GETLINK> &);

  void ChangeLinkOffset(int linkoffset) {
    if (linkoffset == m_linkoffset) {
      return;
    }

    UnlinkAll();
    SetLinkOffset(linkoffset);
  }

  void Clear() {
    T *ptr;

    while ((ptr = Head()) != 0) {
      ptr->~T();
      SMemFree(ptr, typeid(T).INTERNALRAWNAME(), SERR_LINECODE_OBJECT, 0);
    }
  }

  void Combine(TSList<T, GETLINK> *list, DWORD linktype, T *existingptr) {
    VALIDATEBEGIN;
    VALIDATE(list);
    VALIDATE(list != this);
    VALIDATE(list->m_linkoffset == m_linkoffset);
    VALIDATEENDVOID;

    TSLink<T> *terminator = &list->m_terminator;

    if (terminator->m_prevlink == terminator) {
      return;
    }

    TSLink<T> *link = existingptr ? Link(existingptr) : &m_terminator;

    switch (linktype) {
      case LIST_LINK_AFTER: {
        T *next = link->m_next;

        link->NextLink(m_linkoffset)->m_prevlink = terminator->m_prevlink;
        link->m_next = terminator->m_next;
        terminator->NextLink(list->m_linkoffset)->m_prevlink = link;
        terminator->m_prevlink->m_next = next;
        break;
      }

      default:
        FATALERROR(("Invalid case: %s=%u", "linktype", linktype));
        __assume(0);

      case LIST_LINK_BEFORE: {
        TSLink<T> *prev = link->m_prevlink;
        T         *next = prev->m_next;

        prev->m_next = terminator->m_next;
        link->m_prevlink = terminator->m_prevlink;
        terminator->NextLink(list->m_linkoffset)->m_prevlink = prev;
        terminator->m_prevlink->m_next = next;
        break;
      }
    }

    list->InitializeTerminator();
  }

  T *DeleteNode(T *ptr) {
    T *next = Next(ptr);

    ptr->~T();
    SMemFree(ptr, typeid(T).INTERNALRAWNAME(), SERR_LINECODE_OBJECT, 0);
    return next;
  }

  T *Head() {
    return m_terminator.Next();
  }

  const T *Head() const {
    return m_terminator.Next();
  }

  BOOL IsEmpty() const {
    return m_terminator.Next() == 0;
  }

  BOOL IsLinked(const T *instance) const {
    return Link(instance)->IsLinked();
  }

  void LinkNode(T *ptr, DWORD linktype, T *existingptr) {
    TSLink<T> *link = Link(ptr);

    if (link->m_prevlink) {
      link->Unlink();
    }

    TSLink<T> *existing = existingptr ? Link(existingptr) : &m_terminator;

    switch (linktype) {
      case LIST_LINK_AFTER:
        link->m_prevlink = existing;
        link->m_next = existing->m_next;
        existing->NextLink(m_linkoffset)->m_prevlink = link;
        existing->m_next = ptr;
        break;

      default:
        FATALERROR(("Invalid case: %s=%u", "linktype", linktype));
        __assume(0);

      case LIST_LINK_BEFORE: {
        TSLink<T> *previous = existing->m_prevlink;

        link->m_prevlink = previous;
        link->m_next = previous->m_next;
        previous->m_next = ptr;
        existing->m_prevlink = link;
        break;
      }
    }
  }

  T *NewNode(DWORD location, DWORD extrabytes, DWORD flags) {
    T *ptr = new (SMemAlloc(sizeof(T) + extrabytes, typeid(T).INTERNALRAWNAME(), SERR_LINECODE_OBJECT, flags | SMEM_FLAG_ZEROMEMORY)) T;

    if (location) {
      LinkNode(ptr, location, 0);
    }

    return ptr;
  }

  T *Next(const T *instance) {
    return Link(instance)->Next();
  }

  const T *Next(const T *instance) const {
    return Link(instance)->Next();
  }

  T *Prev(const T *instance) {
    return Link(instance)->Prev();
  }

  const T *Prev(const T *instance) const {
    TSLink<T> *link = Link(instance);
    const T   *previous = link->m_prevlink->m_prevlink->m_next;
    return reinterpret_cast<long>(previous) > 0 ? previous : 0;
  }

  T *RawNext(const T *instance) {
    return Link(instance)->RawNext();
  }

  const T *RawNext(const T *instance) const {
    return Link(instance)->RawNext();
  }

  T *Tail() {
    return m_terminator.Prev();
  }

  const T *Tail() const {
    return m_terminator.Prev();
  }

  void UnlinkAll() {
    T *instance;

    while ((instance = Head()) != 0) {
      UnlinkNode(instance);
    }
  }

  void UnlinkNode(T *ptr) {
    Link(ptr)->Unlink();
  }
};

template <class T, int LINKOFFSET>
class TSExplicitList : public TSList<T, TSGetExplicitLink<T> > {
 public:
  TSExplicitList() {
    this->SetLinkOffset(LINKOFFSET);
  }

  TSExplicitList(const TSExplicitList<T, LINKOFFSET> &source) : TSList<T, TSGetExplicitLink<T> >(source) {
  }
};

#define LINKEX(structname)              TSLink<structname>
#define LINKDECLEX(structname, varname) TSLink<structname> varname
#define LIST(structname)                TSList<structname, TSGetLink<structname> >
#define LISTDECL(structname, varname)   TSList<structname, TSGetLink<structname> > varname
#define LISTPTR(structname)             TSList<structname, TSGetLink<structname> > *
#define LISTPTREX(structname)           TSList<structname, TSGetExplicitLink<structname> > *
#define NODEDECL(structname)            struct structname : public TSLinkedNode<structname>
#define NODEDECLEX(structname)          typedef struct structname : public TSExplicitNode<structname>

#define LISTEX(structname, linkname) TSExplicitList<structname, (int)&(((structname *)0)->linkname)>

#define LISTEXDYN(structname) TSExplicitList<structname, (int)0xDDDDDDDD>

#define LISTEXSETLINK(structname, listname, linkname) listname.ChangeLinkOffset((int)&(((structname *)0)->linkname));

#define LISTDECLEX(structname, linkname, varname) TSExplicitList<structname, (int)&(((structname *)0)->linkname)> varname

#define ITERATEFORWARDTEMPLATE(structname, listname, start, ptrname, op)                                                                        \
  for (structname *ptrname = start, *iterate_delete = NULL; (int)ptrname > 0;                                                                   \
       iterate_delete                                                                                                                           \
           ? (ptrname = (listname)op DeleteNode(ptrname), ptrname = ((int)iterate_delete > 0) ? ptrname : NULL, iterate_delete = NULL, ptrname) \
           : ptrname = (listname)op RawNext(ptrname))

#define ITERATEREVERSETEMPLATE(structname, listname, start, ptrname, op)                                                                        \
  for (structname *ptrname = start, *iterate_delete = NULL, *iterate_delete_temp = NULL; ptrname;                                               \
       iterate_delete ? (iterate_delete_temp = ((int)iterate_delete > 0) ? (listname)op Prev(ptrname) : NULL, (listname)op DeleteNode(ptrname), \
                        iterate_delete = NULL, ptrname = iterate_delete_temp)                                                                   \
                      : ptrname = (listname)op Prev(ptrname))

#define ITERATELIST(structname, listname, ptrname) ITERATEFORWARDTEMPLATE(structname, listname, (listname).Head(), ptrname, .)

#define ITERATELISTPTR(structname, listname, ptrname) ITERATEFORWARDTEMPLATE(structname, listname, (listname)->Head(), ptrname, ->)

#define ITERATEPARTIALLIST(structname, listname, start, ptrname) ITERATEFORWARDTEMPLATE(structname, listname, start, ptrname, .)

#define ITERATEPARTIALLISTPTR(structname, listname, start, ptrname) ITERATEFORWARDTEMPLATE(structname, listname, start, ptrname, ->)

#define ITERATELISTREVERSE(structname, listname, ptrname) ITERATEREVERSETEMPLATE(structname, listname, (listname).Tail(), ptrname, .)

#define ITERATELISTREVERSEPTR(structname, listname, ptrname) ITERATEREVERSETEMPLATE(structname, listname, (listname)->Tail(), ptrname, ->)

#define ITERATEPARTIALLISTREVERSE(structname, listname, start, ptrname) ITERATEREVERSETEMPLATE(structname, listname, start, ptrname, .)

#define ITERATEPARTIALLISTREVERSEPTR(structname, listname, start, ptrname) ITERATEREVERSETEMPLATE(structname, listname, start, ptrname, ->)

#define ITERATE_DELETE \
  {                    \
    ++iterate_delete;  \
    continue;          \
  }

#define ITERATE_DELETEANDBREAK \
  {                            \
    --iterate_delete;          \
    continue;                  \
  }

class HASHKEY_NONE {
 public:
  bool operator==(const HASHKEY_NONE &) const {
    return true;
  }
};

class HASHKEY_DWORD {
 private:
  DWORD m_key;

 public:
  HASHKEY_DWORD(DWORD key = 0) : m_key(key) {
  }

  HASHKEY_DWORD(const HASHKEY_DWORD &key) : m_key(key.m_key) {
  }

  int operator==(const HASHKEY_DWORD &key) const {
    return m_key == key.m_key;
  }

  DWORD GetDword() const {
    return m_key;
  }
};

struct SoundFileDataCacheBlock;
class SoundFileCache;
template <class T, class KEY>
class TSHashObject;
template <class T, class KEY>
class TSHashTable;

class HASHKEY_LONGLONG {
 private:
  LONGLONG m_key;

  friend class TSHashObject<SoundFileDataCacheBlock, HASHKEY_LONGLONG>;

  friend void                     DataCacheInitialize(int cacheSizeMB);
  friend SoundFileDataCacheBlock *AllocCacheBlock(LONGLONG hashKey);
  friend class SoundFileCache;

  HASHKEY_LONGLONG(int key) : m_key(key) {
  }

  HASHKEY_LONGLONG() : m_key(0) {
  }

  HASHKEY_LONGLONG(LONGLONG key) : m_key(key) {
  }

  HASHKEY_LONGLONG(const HASHKEY_LONGLONG &key) : m_key(key.m_key) {
  }

 public:
  HASHKEY_LONGLONG &operator=(const HASHKEY_LONGLONG &key) {
    m_key = key.m_key;
    return *this;
  }

  int operator==(const HASHKEY_LONGLONG &key) const {
    return m_key == key.m_key;
  }

  LONGLONG GetLongLong() const {
    return m_key;
  }
};

class HASHKEY_STR {
 protected:
  char *m_str;

 public:
  HASHKEY_STR() : m_str(0) {
  }

  HASHKEY_STR(const HASHKEY_STR &key) : m_str(SStrDupA(key.m_str, __FILE__, __LINE__)) {
  }

  HASHKEY_STR(LPCSTR str) : m_str(SStrDupA(str, __FILE__, __LINE__)) {
  }

  ~HASHKEY_STR() {
    if (m_str) {
      SMemFree(m_str, __FILE__, __LINE__, 0);
    }
  }

  HASHKEY_STR &operator=(const HASHKEY_STR &key) {
    return operator=(key.m_str);
  }

  HASHKEY_STR &operator=(LPCSTR str) {
    if (m_str != str) {
      if (m_str) {
        SMemFree(m_str, __FILE__, __LINE__, 0);
      }
      m_str = SStrDupA(str, __FILE__, __LINE__);
    }
    return *this;
  }

  bool operator==(const HASHKEY_STR &key) const {
    return operator==(key.m_str);
  }

  bool operator==(LPCSTR str) const {
    return SStrCmp(m_str, str, 0x7FFFFFFF) == 0;
  }

  LPCSTR GetString() const {
    return m_str;
  }
};

class HASHKEY_STRI : public HASHKEY_STR {
 public:
  HASHKEY_STRI() {
  }

  HASHKEY_STRI(const HASHKEY_STRI &key) : HASHKEY_STR(key) {
  }

  HASHKEY_STRI(LPCSTR str) : HASHKEY_STR(str) {
  }

  HASHKEY_STRI &operator=(LPCSTR str) {
    HASHKEY_STR::operator=(str);
    return *this;
  }

  HASHKEY_STRI &operator=(const HASHKEY_STRI &key) {
    HASHKEY_STR::operator=(key);
    return *this;
  }

  bool operator==(const HASHKEY_STRI &key) const {
    return operator==(key.m_str);
  }

  bool operator==(LPCSTR str) const {
    return SStrCmpI(m_str, str, 0x7FFFFFFF) == 0;
  }
};

class HASHKEY_CONSTSTR {
 protected:
  LPCSTR m_str;

 public:
  HASHKEY_CONSTSTR() : m_str(0) {
  }

  HASHKEY_CONSTSTR(LPCSTR str) : m_str(str) {
  }

  bool operator==(const HASHKEY_CONSTSTR &key) const {
    return operator==(key.m_str);
  }

  bool operator==(LPCSTR str) const {
    return SStrCmp(m_str, str, 0x7FFFFFFF) == 0;
  }

  LPCSTR GetString() const {
    return m_str;
  }
};

class HASHKEY_CONSTSTRI : public HASHKEY_CONSTSTR {
 public:
  HASHKEY_CONSTSTRI() {
  }

  HASHKEY_CONSTSTRI(LPCSTR str) : HASHKEY_CONSTSTR(str) {
  }

  bool operator==(const HASHKEY_CONSTSTRI &key) const {
    return operator==(key.m_str);
  }

  bool operator==(LPCSTR str) const {
    return m_str == str || SStrCmpI(m_str, str, 0x7FFFFFFF) == 0;
  }
};

class HASHKEY_PTR {
 private:
  LPVOID m_key;

 public:
  HASHKEY_PTR() : m_key(0) {
  }

  HASHKEY_PTR(LPVOID key) : m_key(key) {
  }

  HASHKEY_PTR(const HASHKEY_PTR &key) : m_key(key.m_key) {
  }

  HASHKEY_PTR &operator=(const HASHKEY_PTR &key) {
    m_key = key.m_key;
    return *this;
  }

  int operator==(const HASHKEY_PTR &key) const {
    return m_key == key.m_key;
  }

  LPVOID GetPtr() const {
    return m_key;
  }
};

template <class T, class KEY>
class TSHashObject {
  friend class TSHashTable<T, KEY>;

 private:
  UINT      m_hashval;
  TSLink<T> m_linktoslot;
  TSLink<T> m_linktofull;
  KEY       m_key;

 public:
  TSHashObject() {
  }

  TSHashObject(const TSHashObject<T, KEY> &) {
  }

  TSHashObject<T, KEY> &operator=(const TSHashObject<T, KEY> &) {
    return *this;
  }

  KEY GetKey() const {
    return m_key;
  }

  LPCVOID GetData() const;

  LPCSTR GetString() const {
    return m_key.GetString();
  }

  UINT GetHashValue() const {
    return m_hashval;
  }
};

template <class T, class KEY>
class TSHashTable {
 private:
  friend class CGameTime;

 protected:
  LISTEXDYN(T) m_fulllist;

 private:
  UINT                          m_fullnessIndicator;
  TSGrowableArray<LISTEXDYN(T)> m_slotlistarray;
  UINT                          m_slotmask;

  UINT ComputeSlot(UINT hashval) const {
    return hashval & m_slotmask;
  }

  void GrowListArray(UINT newarraysize) {
    UINT oldarraysize = m_slotmask + 1;
    int  linkoffset = GetLinkOffset();
    LISTEXDYN(T) templist;
    UINT loop;
    T   *ptr;

    templist.ChangeLinkOffset(linkoffset);
    for (loop = 0; loop < oldarraysize; ++loop) {
      while ((ptr = m_slotlistarray[loop].Head()) != 0) {
        templist.LinkNode(ptr, LIST_TAIL, 0);
      }
    }

    m_slotlistarray.SetCount(newarraysize);
    for (loop = 0; loop < newarraysize; ++loop) {
      m_slotlistarray[loop].ChangeLinkOffset(linkoffset);
    }

    m_slotmask = newarraysize - 1;
    while ((ptr = templist.Head()) != 0) {
      UINT slot = ComputeSlot(ptr->m_hashval);

      m_slotlistarray[slot].LinkNode(ptr, LIST_TAIL, 0);
    }
  }
  void Initialize() {
    m_slotmask = 3;
    m_slotlistarray.SetCount(4);

    int linkoffset = GetLinkOffset();

    for (UINT loop = 0; loop <= m_slotmask; ++loop) {
      m_slotlistarray[loop].ChangeLinkOffset(linkoffset);
    }
  }

  BOOL Initialized() {
    return m_slotmask != 0xFFFFFFFF;
  }

  void InternalClear(int warn) {
    UINT loop;
    T   *ptr;

    m_fullnessIndicator = 0;
    m_fulllist.UnlinkAll();
    for (loop = 0; loop < m_slotlistarray.Count(); ++loop) {
      while ((ptr = m_slotlistarray[loop].Head()) != 0) {
        if (warn) {
          m_slotlistarray[loop].UnlinkNode(ptr);
        } else {
          InternalDelete(ptr);
        }
      }
    }
  }
  virtual void InternalDelete(T *ptr) {
    ptr->~T();
    SMemFree(ptr, typeid(T).INTERNALRAWNAME(), SERR_LINECODE_OBJECT, 0);
  }
  virtual T *InternalNew(LISTEXDYN(T) * listptr, DWORD extrabytes, DWORD flags) {
    return listptr->NewNode(LIST_HEAD, extrabytes, flags);
  }
  int MonitorFullness(UINT slot) {
    if (m_slotmask >= 0x1FFF) {
      return 0;
    }

    if (m_fullnessIndicator > 3) {
      m_fullnessIndicator -= 3;
    } else {
      m_fullnessIndicator = 0;
    }

    ITERATELIST(T, m_slotlistarray[slot], ptr) {
      ++m_fullnessIndicator;
      if (m_fullnessIndicator > 13) {
        m_fullnessIndicator = 0;
        GrowListArray((m_slotmask + 1) * 2);
        return 1;
      }
    }

    return 0;
  }
  void InternalLinkNode(T *ptr, UINT hashval) {
    if (!Initialized()) {
      Initialize();
    }

    UINT slot = ComputeSlot(hashval);

    if (MonitorFullness(slot)) {
      slot = ComputeSlot(hashval);
    }

    m_slotlistarray[slot].LinkNode(ptr, LIST_TAIL, 0);
    m_fulllist.LinkNode(ptr, LIST_TAIL, 0);
  }
  T *InternalNewNode(UINT hashval, DWORD extrabytes, DWORD flags) {
    if (!Initialized()) {
      Initialize();
    }

    UINT slot = ComputeSlot(hashval);

    if (MonitorFullness(slot)) {
      slot = ComputeSlot(hashval);
    }

    T *ptr = InternalNew(&m_slotlistarray[slot], extrabytes, flags);

    m_fulllist.LinkNode(ptr, LIST_TAIL, 0);
    return ptr;
  }

  TSHashTable<T, KEY> &NonConst() const {
    return const_cast<TSHashTable<T, KEY> &>(*this);
  }

 protected:
  int GetLinkOffset() const {
    return reinterpret_cast<int>(&((T *)0)->m_linktoslot);
  }

 public:
  TSHashTable(const TSHashTable<T, KEY> &);
  TSHashTable() {
    m_fullnessIndicator = 0;
    m_fulllist.ChangeLinkOffset(reinterpret_cast<int>(&((T *)0)->m_linktofull));
    m_slotmask = 0xFFFFFFFF;
  }
  TSHashTable<T, KEY> &operator=(const TSHashTable<T, KEY> &);
  virtual ~TSHashTable() {
    InternalClear(1);
  }

  void Clear() {
    InternalClear(0);
  }

  void Delete(T *ptr) {
    Unlink(ptr);
    InternalDelete(ptr);
  }

  void Delete(UINT hashval, const KEY &key) {
    T *ptr = Ptr(hashval, key);

    FATALASSERT(ptr);

    Delete(ptr);
  }

  void Delete(UINT hashval, LPCSTR key) {
    T *ptr = Ptr(hashval, key);

    FATALASSERT(ptr);

    Delete(ptr);
  }

  void Delete(LPCSTR key) {
    T *ptr = Ptr(key);

    FATALASSERT(ptr);

    Delete(ptr);
  }

  T *DeleteNode(T *ptr) {
    T *next = Next(ptr);
    Delete(ptr);
    return next;
  }

  virtual void Destroy() {
    InternalClear(1);
    m_fullnessIndicator = 0;
    m_slotmask = 0xFFFFFFFF;
    m_slotlistarray.Clear();
  }

  T *Head() {
    return m_fulllist.Head();
  }

  const T *Head() const {
    return NonConst().Head();
  }

  void Insert(T *ptr, UINT hashval, const KEY &key) {
    InternalLinkNode(ptr, hashval);
    ptr->m_hashval = hashval;
    ptr->m_key = key;
  }

  void Insert(T *ptr, UINT hashval, LPCSTR str) {
    InternalLinkNode(ptr, hashval);
    ptr->m_hashval = hashval;
    ptr->m_key = str;
  }

  void Insert(T *ptr, LPCSTR str) {
    UINT hashval = SStrHashHT(str);

    Insert(ptr, hashval, str);
  }

  T *New(UINT hashval, const KEY &key, DWORD extrabytes, DWORD flags) {
    T *ptr = InternalNewNode(hashval, extrabytes, flags);

    ptr->m_hashval = hashval;
    ptr->m_key = key;
    return ptr;
  }

  T *New(UINT hashval, LPCSTR str, DWORD extrabytes, DWORD flags) {
    T *ptr = InternalNewNode(hashval, extrabytes, flags);

    ptr->m_hashval = hashval;
    ptr->m_key = str;
    return ptr;
  }

  T *New(LPCSTR str, DWORD extrabytes, DWORD flags) {
    UINT hashval = SStrHashHT(str);

    return New(hashval, str, extrabytes, flags);
  }

  T *Next(const T *ptr) {
    return m_fulllist.Next(ptr);
  }

  const T *Next(const T *ptr) const {
    return NonConst().Next(ptr);
  }

  T *Prev(const T *ptr) {
    return m_fulllist.Prev(ptr);
  }

  const T *Prev(const T *ptr) const {
    return NonConst().Prev(ptr);
  }

  T *Ptr(UINT hashval, const KEY &key) {
    if (!Initialized()) {
      return 0;
    }

    ITERATELIST(T, m_slotlistarray[ComputeSlot(hashval)], ptr) {
      if (ptr->m_hashval == hashval && ptr->m_key == key) {
        return ptr;
      }
    }

    return 0;
  }

  const T *Ptr(UINT hashval, const KEY &key) const {
    return NonConst().Ptr(hashval, key);
  }

  T *Ptr(UINT hashval, LPCSTR str) {
    if (!Initialized()) {
      return 0;
    }

    ITERATELIST(T, m_slotlistarray[ComputeSlot(hashval)], ptr) {
      if (ptr->m_hashval == hashval && ptr->m_key == str) {
        return ptr;
      }
    }

    return 0;
  }
  const T *Ptr(UINT hashval, LPCSTR key) const;
  T *Ptr(LPCSTR str) {
    if (!Initialized()) {
      return 0;
    }

    UINT hashval = SStrHashHT(str);

    ITERATELIST(T, m_slotlistarray[ComputeSlot(hashval)], ptr) {
      if (ptr->m_hashval == hashval && ptr->m_key == str) {
        return ptr;
      }
    }

    return 0;
  }

  const T *Ptr(LPCSTR key) const {
    return NonConst().Ptr(key);
  }

  T *RawNext(const T *ptr) {
    return m_fulllist.Next(ptr);
  }

  const T *RawNext(const T *ptr) const {
    return NonConst().RawNext(ptr);
  }

  T *Tail() {
    return m_fulllist.Tail();
  }

  const T *Tail() const {
    return NonConst().Tail();
  }

  void Unlink(T *ptr) {
    if (ptr->m_linktoslot.IsLinked()) {
      ptr->m_linktoslot.Unlink();
      ptr->m_linktofull.Unlink();
    }
  }

  void SetTableSize(UINT count) {
    UINT requested = count * 2;
    UINT tableSize;
    UINT value;
    UINT shift;

    if (requested <= m_slotmask + 1) {
      return;
    }

    if (requested > 0x2000) {
      tableSize = 0x2000;
    } else {
      value = requested;
      shift = 0;
      while (value > 1) {
        value >>= 1;
        ++shift;
      }

      tableSize = 1 << shift;
      if (requested > tableSize) {
        tableSize *= 2;
      }
    }

    GrowListArray(tableSize);
  }

  float GetAverageBinDepth() const;
  UINT  GetPeakBinDepth() const;

  static UINT Hash(LPCSTR key) {
    return SStrHashHT(key);
  }
};

template <class T, class KEY>
class TSHashObjectChunk {
 public:
  TSGrowableArray<T>                 m_array;
  TSLink<TSHashObjectChunk<T, KEY> > m_link;
};

template <class T, class KEY, int REUSE>
class TSHashTableReuse : public TSHashTable<T, KEY> {
 private:
  LISTEXDYN(T) m_reuseList;
  DWORD                                         m_chunkSize;
  TSExplicitList<TSHashObjectChunk<T, KEY>, 20> m_chunkList;

  void         Destructor();
  virtual void InternalDelete(T *ptr) {
    this->m_fulllist.UnlinkNode(ptr);
    m_reuseList.LinkNode(ptr, LIST_HEAD, 0);
  }
  virtual T *InternalNew(LISTEXDYN(T) * listptr, DWORD extrabytes, DWORD flags) {
    ASSERT(!extrabytes);

    T *ptr = m_reuseList.Head();

    if (!ptr) {
      TSHashObjectChunk<T, KEY> *chunk;

      for (;;) {
        chunk = m_chunkList.Head();
        if (chunk && chunk->m_array.Reserved() > 0) {
          break;
        }

        chunk = m_chunkList.NewNode(LIST_HEAD, 0, 0);
        chunk->m_array.ReserveSpace(m_chunkSize);
        m_chunkSize *= 2;
      }

      ptr = chunk->m_array.NewElement();
    }

    listptr->LinkNode(ptr, LIST_HEAD, 0);
    return ptr;
  }

 public:
  TSHashTableReuse() {
    m_chunkSize = 16;
    m_reuseList.ChangeLinkOffset(this->GetLinkOffset());
  }
  virtual ~TSHashTableReuse() {
    Destructor();
  }
  virtual void Destroy() {
    this->Clear();
    Destructor();
  }
};

template <class T, class HANDLE, int REUSE>
class TSExportTableSimple : public TSHashTableReuse<T, HASHKEY_NONE, REUSE> {
 private:
  HASHKEY_NONE m_key;
  UINT         m_sequence;
  BOOL         m_wrapped;

  HANDLE GenerateUniqueHandle() {
    for (;;) {
      ++m_sequence;
      if (!m_sequence) {
        m_wrapped = 1;
        continue;
      }
      if (!m_wrapped || !Ptr(reinterpret_cast<HANDLE>(m_sequence))) {
        return reinterpret_cast<HANDLE>(m_sequence);
      }
    }
  }

 public:
  TSExportTableSimple() : m_sequence(~reinterpret_cast<UINT>(this) & 0x0FFFFFFF) {
    m_wrapped = 0;
  }

  void Delete(T *ptr) {
    TSHashTable<T, HASHKEY_NONE>::Delete(ptr);
  }
  void Delete(HANDLE handle);
  T *New(HANDLE *handle) {
    HANDLE newhandle = GenerateUniqueHandle();

    *handle = newhandle;
    return TSHashTable<T, HASHKEY_NONE>::New(reinterpret_cast<UINT>(newhandle), m_key, 0, 0);
  }
  T *Ptr(HANDLE handle) {
    return TSHashTable<T, HASHKEY_NONE>::Ptr(reinterpret_cast<UINT>(handle), m_key);
  }
};

template <class T, class HANDLE, class LOCKED, class SYNC, int REUSE>
class TSExportTableSync : public TSExportTableSimple<T, HANDLE, REUSE> {
 private:
  SYNC m_sync;

  int IsForWriting(LOCKED lockedhandle) {
    return reinterpret_cast<DWORD>(lockedhandle) == 1;
  }
  void SyncEnterLock(LOCKED *lockedhandle, int forwriting) {
    m_sync.Enter(forwriting);
    *lockedhandle = forwriting ? reinterpret_cast<LOCKED>(1) : reinterpret_cast<LOCKED>(-1);
  }
  void SyncLeaveLock(LOCKED lockedhandle) {
    if (lockedhandle) {
      m_sync.Leave(IsForWriting(lockedhandle));
    }
  }

 public:
  TSExportTableSync() {
  }

  void Delete(HANDLE handle) {
    LOCKED lockedhandle;
    T     *ptr = Lock(handle, &lockedhandle, 1);

    DeleteUnlock(ptr, lockedhandle);
  }
  void DeleteUnlock(T *ptr, LOCKED lockedhandle) {
    TSExportTableSimple<T, HANDLE, REUSE>::Delete(ptr);
    Unlock(lockedhandle);
  }
  T *Lock(HANDLE handle, LOCKED *lockedhandle, int forwriting) {
    T *ptr;

    SyncEnterLock(lockedhandle, forwriting);
    ptr = TSExportTableSimple<T, HANDLE, REUSE>::Ptr(handle);
    if (!ptr) {
      SyncLeaveLock(*lockedhandle);
      *lockedhandle = 0;
    }
    return ptr;
  }
  void New(HANDLE *handle);
  T *NewLock(HANDLE *handle, LOCKED *lockedhandle) {
    SyncEnterLock(lockedhandle, 1);
    return TSExportTableSimple<T, HANDLE, REUSE>::New(handle);
  }
  void Unlock(LOCKED lockedhandle) {
    SyncLeaveLock(lockedhandle);
  }
};

template <class T, class KEY, int REUSE>
void TSHashTableReuse<T, KEY, REUSE>::Destructor() {
  m_chunkList.Clear();
  m_reuseList.Clear();
  m_chunkSize = 16;
}

