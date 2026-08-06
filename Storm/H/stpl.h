#pragma once

#include "storm.h"

#include <new>
#include <string.h>

class CSBasePriorityQueue;

template <class T>
class TSStackArray {
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

  const T &operator[](UINT index) const {
    if (index >= m_count) {
      FatalArrayBounds();
    }
    return m_data[index];
  }

  T &operator[](UINT index) {
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

  const T *Ptr() const {
    return m_data;
  }

  T *Ptr() {
    return m_data;
  }

  void Set(UINT count, int, const T *data) {
    SetCount(0);
    Add(count, 0, data);
  }

  void Set(UINT count, const T *data) {
    Set(count, 0, data);
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

  void Add(UINT count, int, const T *data) {
    if (m_count + count > m_maxCount) {
      FatalArrayBounds();
    }

    for (UINT index = 0; index < count; ++index) {
      new (&m_data[m_count++]) T(data[index]);
    }
  }

  void Add(UINT count, const T *data) {
    Add(count, 0, data);
  }

  T *New(const T &value) {
    if (m_count >= m_maxCount) {
      FatalArrayBounds();
    }
    T *result = &m_data[m_count++];
    new (result) T(value);
    return result;
  }

  T *New() {
    if (m_count >= m_maxCount) {
      FatalArrayBounds();
    }
    T *result = &m_data[m_count++];
    new (result) T;
    return result;
  }

 protected:
  void FatalArrayBounds() const {
    SErrDisplayError(STORM_ERROR_ACCESS_OUT_OF_BOUNDS, typeid(T).INTERNALRAWNAME(), SERR_LINECODE_OBJECT, 0, TRUE, 1);
  }

 private:
  UINT m_maxCount;

 protected:
  UINT m_count;
  T   *m_data;
};

template <class T, UINT MAXCOUNT>
class TSCArray {
 protected:
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

  const T &operator[](UINT index) const {
    if (index >= m_count) {
      FatalArrayBounds();
    }
    return m_data[index];
  }

  T &operator[](UINT index) {
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

  void SetCount(UINT count) {
    if (count > MAXCOUNT) {
      FatalArrayBounds();
    }
    m_count = count;
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

  void Zero() {
    memset(m_data, 0, Bytes());
  }

  UINT SizeOfElement() const {
    return sizeof(T);
  }

  UINT MaxCount() const {
    return MAXCOUNT;
  }

  const T *Ptr() const {
    return m_data;
  }

  T *Ptr() {
    return m_data;
  }

 protected:
  UINT m_count;
  T    m_data[MAXCOUNT];
};

template <class T>
class TSBaseArray {
 protected:
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

 public:
  UINT Count() const {
    return m_count;
  }

  UINT SizeOfElement() const {
    return sizeof(T);
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

  T *Top() {
    return m_count ? &m_data[m_count - 1] : 0;
  }

  const T *Top() const {
    return m_count ? &m_data[m_count - 1] : 0;
  }

  T &operator[](UINT index) {
    CheckArrayBounds(index);
    return m_data[index];
  }

  const T &operator[](UINT index) const {
    CheckArrayBounds(index);
    return m_data[index];
  }

  UINT NumElements() const {
    return Count();
  }

 protected:
  void CheckArrayBounds(UINT index) const {
    if (index >= m_count) {
      SErrDisplayErrorFmt(0x85100080, MemFileName(), MemLineNo(), TRUE, 1, "index (0x%08X), array size (0x%08X)", index, m_count);
    }
  }

  UINT m_alloc;
  UINT m_count;
  T   *m_data;
};

template <class T>
class TSFixedArray : public TSBaseArray<T> {
 public:
  TSFixedArray() {
    this->Constructor();
  }

  TSFixedArray(const TSBaseArray<T> &source);
  TSFixedArray(const TSFixedArray<T> &source);

  inline TSFixedArray<T> &operator=(const TSBaseArray<T> &source);
  inline TSFixedArray<T> &operator=(const TSFixedArray<T> &source);

  ~TSFixedArray();

  void Clear();
  void Detach(T **data, UINT *count, UINT *alloc);
  void Exchange(TSFixedArray<T> *array);
  void Set(UINT count, const T *data);
  void Set(UINT count, int, const T *data);
  void SetCount(UINT count);
  void SetOptional(UINT count, const T *data);
  void Zero();

 protected:
  void ReallocAndClearData(UINT count);
  void ReallocData(UINT count);
};

template <class T>
class TSGrowableArray : public TSFixedArray<T> {
 public:
  TSGrowableArray() {
    m_chunk = 0;
  }

  TSGrowableArray(const TSGrowableArray<T> &source) : TSFixedArray<T>(source), m_chunk(source.m_chunk) {
  }

  T *New() {
    T *value;

    Reserve(1, 1);
    value = &this->m_data[this->m_count];
    ++this->m_count;
    if (value) {
      new (value) T;
    }
    return value;
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

  void SetCount(UINT count) {
    UINT index;

    if (count > this->m_count) {
      Reserve(count - this->m_count, 1);
      for (index = this->m_count; index < count; ++index) {
        new (&this->m_data[index]) T;
      }
    } else {
      for (index = this->m_count; index > count; --index) {
        this->m_data[index - 1].~T();
      }
    }

    this->m_count = count;
  }

  void ReserveSpace(UINT count) {
    Reserve(count, 0);
  }

  void SetChunkSize(UINT chunk) {
    m_chunk = chunk;
  }

  void TrimUnusedSpace() {
    this->ReallocData(this->m_count);
  }

  void GrowToFit(UINT index, int zero);

  UINT Add(const T *data) {
    return Add(1, data);
  }

  UINT Add(UINT count, const T *data);
  UINT Add(UINT count, int, const T *data) {
    return Add(count, data);
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

 private:
  void Reserve(UINT count, int round);

  UINT CalcChunkSize(UINT count);

  UINT RoundToChunk(UINT count, UINT chunk) const;

  friend class CSBasePriorityQueue;

  UINT m_chunk;
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

template <class T>
TSFixedArray<T>::TSFixedArray(const TSBaseArray<T> &source) {
  this->Constructor();
  Set(source.Count(), source.Ptr());
}

template <class T>
TSFixedArray<T>::TSFixedArray(const TSFixedArray<T> &source) {
  this->Constructor();
  UINT     count = source.m_count;
  const T *data = source.m_data;
  UINT     index;

  ReallocAndClearData(count);
  for (index = 0; index < count; ++index) {
    T *value = &this->m_data[index];
    if (value) {
      new (value) T(data[index]);
    }
  }
  this->m_count = count;
}

template <class T>
inline TSFixedArray<T> &TSFixedArray<T>::operator=(const TSBaseArray<T> &source) {
  if (this != &source) {
    Set(source.Count(), source.Ptr());
  }
  return *this;
}

template <class T>
inline TSFixedArray<T> &TSFixedArray<T>::operator=(const TSFixedArray<T> &source) {
  if (this != &source) {
    Set(source.Count(), source.Ptr());
  }
  return *this;
}

template <class T>
TSFixedArray<T>::~TSFixedArray() {
  UINT index;

  for (index = 0; index < this->m_count; ++index) {
    this->m_data[index].~T();
  }

  if (this->m_data) {
    SMemFree(this->m_data, this->MemFileName(), this->MemLineNo(), 0);
  }
}

template <class T>
void TSFixedArray<T>::Clear() {
  this->TSFixedArray<T>::~TSFixedArray();
  this->m_alloc = 0;
  this->m_count = 0;
  this->m_data = 0;
}

template <class T>
void TSFixedArray<T>::Exchange(TSFixedArray<T> *array) {
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

template <class T>
void TSFixedArray<T>::Set(UINT count, const T *data) {
  UINT index;

  ReallocAndClearData(count);
  for (index = 0; index < count; ++index) {
    T *value = &this->m_data[index];
    if (value) {
      new (value) T(data[index]);
    }
  }
  this->m_count = count;
}

template <class T>
void TSFixedArray<T>::Set(UINT count, int, const T *data) {
  Set(count, data);
}

template <class T>
void TSFixedArray<T>::SetOptional(UINT count, const T *data) {
  if (data) {
    Set(count, data);
  } else {
    SetCount(count);
  }
}

template <class T>
void TSFixedArray<T>::Zero() {
  memset(this->m_data, 0, this->Bytes());
}

template <class T>
void TSFixedArray<T>::SetCount(UINT count) {
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

template <class T>
void TSFixedArray<T>::ReallocAndClearData(UINT count) {
  UINT index;

  for (index = 0; index < this->m_count; ++index) {
    this->m_data[index].~T();
  }

  this->m_alloc = count;
  if (this->m_data || count) {
    this->m_data = static_cast<T *>(SMemReAlloc(this->m_data, count * sizeof(T), this->MemFileName(), this->MemLineNo(), 0));
  }
}

template <class T>
void TSFixedArray<T>::ReallocData(UINT count) {
  T   *oldData = this->m_data;
  T   *newData;
  UINT copyCount;
  UINT index;

  this->m_alloc = count;
  newData = static_cast<T *>(SMemReAlloc(oldData, count * sizeof(T), this->MemFileName(), this->MemLineNo(), 0x10));
  this->m_data = newData;
  if (newData) {
    return;
  }

  newData = static_cast<T *>(SMemAlloc(count * sizeof(T), this->MemFileName(), this->MemLineNo(), 0));
  this->m_data = newData;
  if (!oldData) {
    return;
  }

  copyCount = count < this->m_count ? count : this->m_count;
  for (index = 0; index < copyCount; ++index) {
    if (this->m_data) {
      new (&this->m_data[index]) T(oldData[index]);
    }
  }

  SMemFree(oldData, this->MemFileName(), this->MemLineNo(), 0);
}

template <class T>
void TSGrowableArray<T>::Reserve(UINT count, int round) {
  UINT needed = this->m_count + count;
  UINT chunk;

  if (needed <= this->m_alloc) {
    return;
  }

  if (round) {
    chunk = m_chunk;
    if (!chunk) {
      chunk = CalcChunkSize(needed);
    }
    needed = RoundToChunk(needed, chunk);
  }

  this->ReallocData(needed);
}

template <class T>
void TSGrowableArray<T>::GrowToFit(UINT index, int zero) {
  if (index >= this->m_count) {
    Reserve(index - this->m_count + 1, 1);
    if (zero) {
      memset(&this->m_data[this->m_count], 0, (index - this->m_count + 1) * sizeof(T));
    }
    this->m_count = index + 1;
  }
}

template <class T>
void TSFixedArray<T>::Detach(T **data, UINT *count, UINT *alloc) {
  *data = this->m_data;
  *count = this->m_count;
  *alloc = this->m_alloc;
  this->m_data = 0;
  this->m_count = 0;
  this->m_alloc = 0;
}

template <class T>
UINT TSGrowableArray<T>::Add(UINT count, const T *data) {
  UINT first = this->m_count;
  UINT index;
  T   *destination;

  Reserve(count, 1);
  for (index = 0; index < count; ++index) {
    destination = &this->m_data[first + index];
    if (destination) {
      new (destination) T(data[index]);
    }
  }
  this->m_count += count;
  return first;
}

template <class T>
UINT TSGrowableArray<T>::CalcChunkSize(UINT count) {
  const UINT maxChunk = sizeof(T) < 0x20 ? 0x100 / sizeof(T) : 8;
  UINT       chunk = count;
  UINT       next;

  if (count >= maxChunk) {
    m_chunk = maxChunk;
    return maxChunk;
  }

  next = (count - 1) & count;
  while (next) {
    chunk = next;
    next = (chunk - 1) & chunk;
  }
  return chunk < 1 ? 1 : chunk;
}

template <class T>
UINT TSGrowableArray<T>::RoundToChunk(UINT count, UINT chunk) const {
  UINT remainder = count % chunk;
  return remainder ? count + chunk - remainder : count;
}

class CSBasePriority {
 private:
  void Construct() {
    m_queue = 0;
    m_index = 0;
  }

 public:
  CSBasePriority() {
    Construct();
  }

  ~CSBasePriority();

  virtual int Compare(CSBasePriority *priority) const = 0;

  CSBasePriority &operator=(const CSBasePriority &);

  int IsLinked() const {
    return m_queue != 0;
  }

  void Relink();

  void SetQueuePosition(CSBasePriorityQueue *queue, UINT index) {
    m_queue = queue;
    m_index = index;
  }

  void Unlink();

 private:
  CSBasePriorityQueue *m_queue;
  UINT                 m_index;
};

template <class T>
class TSTimerPriority : public CSBasePriority {
 public:
  TSTimerPriority() : m_val(0) {
  }

  int Compare(CSBasePriority *priority) const {
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

 private:
  T m_val;
};

class CSBasePriorityQueue : public TSGrowableArray<LPVOID> {
 private:
  friend class CSBasePriority;

  UINT Child(UINT index) const {
    return index * 2 + 1;
  }

  UINT Parent(UINT index) const {
    return (index - 1) >> 1;
  }

  CSBasePriority *Link(UINT index) const {
    this->CheckArrayBounds(index);
    return Link(this->m_data[index]);
  }

  CSBasePriority *Link(LPVOID value) const {
    return reinterpret_cast<CSBasePriority *>(reinterpret_cast<BYTE *>(value) + m_linkOffset);
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
  enum {
    ROOT_INDEX = 0
  };

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
    UINT            index = this->m_count;
    UINT            parent;
    CSBasePriority *valueLink = Link(val);

    Reserve(1, 1);
    ++this->m_count;
    while (index) {
      parent = Parent(index);
      if (Compare(Link(parent), valueLink)) {
        break;
      }
      (*this)[index] = (*this)[parent];
      SetLink(index);
      index = parent;
    }
    (*this)[index] = val;
    SetLink(index);
  }

  void Remove(UINT index) {
    UINT            newCount;
    UINT            child;
    LPVOID          replacement;
    CSBasePriority *replacementLink;

    this->CheckArrayBounds(index);
    UnsetLink(index);

    newCount = this->m_count - 1;
    replacement = (*this)[newCount];
    this->m_count = newCount;
    if (index == newCount) {
      return;
    }

    replacementLink = Link(replacement);
    while (index <= (newCount - 2) / 2 && newCount > 1) {
      child = Child(index);
      if (child + 1 < newCount && Compare(Link(child + 1), Link(child))) {
        ++child;
      }
      if (Compare(replacementLink, Link(child))) {
        break;
      }
      (*this)[index] = (*this)[child];
      SetLink(index);
      index = child;
    }
    (*this)[index] = replacement;
    SetLink(index);
  }

 private:
  UINT m_linkOffset;
};

template <class T>
class TSPriorityQueue : public CSBasePriorityQueue {
 public:
  TSPriorityQueue(int linkOffset) : CSBasePriorityQueue(linkOffset) {
  }

  T *operator[](UINT index) {
    return static_cast<T *>(CSBasePriorityQueue::operator[](index));
  }

  const T *operator[](UINT index) const {
    this->CheckArrayBounds(index);
    return static_cast<const T *>(this->m_data[index]);
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
  UINT                 index;
  LPVOID               value;

  if (!queue) {
    return;
  }
  index = m_index;
  value = reinterpret_cast<BYTE *>(this) - queue->m_linkOffset;
  queue->Remove(index);
  queue->Enqueue(value);
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

  TSLink<T> *NextLink(int linkoffset) const;

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

  int IsLinked() const;

  void Unlink();
};

template <class T>
TSLink<T> *TSLink<T>::NextLink(int linkoffset) const {
  int next = reinterpret_cast<int>(m_next);

  if (next <= 0) {
    return reinterpret_cast<TSLink<T> *>(~next);
  }

  if (linkoffset < 0) {
    linkoffset = reinterpret_cast<const BYTE *>(this) - reinterpret_cast<const BYTE *>(m_prevlink->m_next);
  }

  return reinterpret_cast<TSLink<T> *>(reinterpret_cast<BYTE *>(m_next) + linkoffset);
}

template <class T>
int TSLink<T>::IsLinked() const {
  return m_next != 0;
}

template <class T>
void TSLink<T>::Unlink() {
  TSLink<T> *prevlink = m_prevlink;

  if (prevlink) {
    NextLink(-1)->m_prevlink = prevlink;
    m_prevlink->m_next = m_next;
    Constructor();
  }
}

template <class T>
class TSLinkedNode {
  friend class TSGetLink<T>;

 public:
  ~TSLinkedNode() {
    Unlink();
  }

  int IsLinked() const {
    return m_link.IsLinked();
  }

  void Unlink() {
    m_link.Unlink();
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

 private:
  TSLink<T> m_link;
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

  TSLink<T> *Link(const T *instance) const;

  void InitializeTerminator() {
    m_terminator.m_prevlink = &m_terminator;
    m_terminator.m_next = reinterpret_cast<T *>(~reinterpret_cast<DWORD>(&m_terminator));
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

  TSList<T, GETLINK> &operator=(const TSList<T, GETLINK> &);

  TSList(int linkoffset) {
    Constructor();
    SetLinkOffset(linkoffset);
  }

  ~TSList() {
    UnlinkAll();
  }

  void ChangeLinkOffset(int linkoffset) {
    if (m_linkoffset == linkoffset) {
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

  T *Head() {
    return m_terminator.Next();
  }

  const T *Head() const {
    return m_terminator.Next();
  }

  T *Tail() {
    return m_terminator.Prev();
  }

  const T *Tail() const {
    return m_terminator.Prev();
  }

  int IsEmpty() const {
    return m_terminator.Next() == 0;
  }

  int IsLinked(const T *instance) const {
    return Link(instance)->IsLinked();
  }

  T *Next(const T *instance) {
    return Link(instance)->Next();
  }

  const T *Next(const T *instance) const {
    return Link(instance)->Next();
  }

  T *RawNext(const T *instance) {
    return Link(instance)->RawNext();
  }

  const T *RawNext(const T *instance) const {
    return Link(instance)->RawNext();
  }

  T *Prev(const T *instance) {
    TSLink<T> *link = Link(instance);
    T         *previous = link->m_prevlink->m_prevlink->m_next;
    return reinterpret_cast<long>(previous) > 0 ? previous : 0;
  }

  const T *Prev(const T *instance) const {
    TSLink<T> *link = Link(instance);
    const T   *previous = link->m_prevlink->m_prevlink->m_next;
    return reinterpret_cast<long>(previous) > 0 ? previous : 0;
  }

  void UnlinkNode(T *instance) {
    Link(instance)->Unlink();
  }

  void UnlinkAll() {
    T *instance;

    while ((instance = Head()) != 0) {
      UnlinkNode(instance);
    }
  }

  void LinkNode(T *instance, DWORD linktype, T *existingInstance);

  T *NewNode(DWORD location, DWORD extrabytes, DWORD flags) {
    T *ptr = static_cast<T *>(SMemAlloc(sizeof(T) + extrabytes, typeid(T).INTERNALRAWNAME(), -2, flags | SMEM_FLAG_ZEROMEMORY));

    if (ptr) {
      new (ptr) T;
    }

    if (location) {
      LinkNode(ptr, location, 0);
    }

    return ptr;
  }

  T *DeleteNode(T *ptr) {
    T *next = Next(ptr);

    ptr->~T();
    SMemFree(ptr, typeid(T).INTERNALRAWNAME(), SERR_LINECODE_OBJECT, 0);
    return next;
  }

  void Combine(TSList<T, GETLINK> *list, DWORD linktype, T *existingInstance) {
    TSLink<T> *listTerminator;
    TSLink<T> *existing;
    TSLink<T> *first;
    TSLink<T> *last;

    FATALASSERT(list);

    FATALASSERT(list != this);

    FATALASSERT(list->m_linkoffset == m_linkoffset);

    listTerminator = &list->m_terminator;
    if (listTerminator->m_prevlink == listTerminator) {
      return;
    }

    existing = Link(existingInstance);
    first = list->Link(list->Head());
    last = listTerminator->m_prevlink;

    if (linktype == LIST_LINK_AFTER) {
      TSLink<T> *next = existing->NextLink(m_linkoffset);

      last->m_next = existing->m_next;
      next->m_prevlink = last;
      existing->m_next = listTerminator->m_next;
      first->m_prevlink = existing;
    } else {
      TSLink<T> *previous;
      T         *previousNext;

      if (linktype != LIST_LINK_BEFORE) {
        FATALERROR(("Invalid case: %s=%u", "linktype", linktype));
      }

      previous = existing->m_prevlink;
      previousNext = previous->m_next;
      previous->m_next = listTerminator->m_next;
      first->m_prevlink = previous;
      last->m_next = previousNext;
      existing->m_prevlink = last;
    }

    list->InitializeTerminator();
  }
};

template <class T, class GETLINK>
TSLink<T> *TSList<T, GETLINK>::Link(const T *instance) const {
  return instance ? GETLINK::Link(instance, m_linkoffset) : const_cast<TSLink<T> *>(&m_terminator);
}

template <class T, class GETLINK>
void TSList<T, GETLINK>::LinkNode(T *instance, DWORD linktype, T *existingInstance) {
  TSLink<T> *link = Link(instance);
  TSLink<T> *existing = Link(existingInstance);

  if (link->m_prevlink) {
    link->Unlink();
  }

  if (linktype == LIST_LINK_AFTER) {
    TSLink<T> *nextLink = existing->NextLink(m_linkoffset);

    link->m_prevlink = existing;
    link->m_next = existing->m_next;
    nextLink->m_prevlink = link;
    existing->m_next = instance;
  } else {
    TSLink<T> *previous;

    if (linktype != LIST_LINK_BEFORE) {
      FATALERROR(("Invalid case: %s=%u", "linktype", linktype));
    }

    previous = existing->m_prevlink;
    link->m_prevlink = previous;
    link->m_next = previous->m_next;
    previous->m_next = instance;
    existing->m_prevlink = link;
  }
}

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

 private:
  DWORD m_key;
};

struct SoundFileDataCacheBlock;
class SoundFileCache;
template <class T, class KEY>
class TSHashObject;
template <class T, class KEY>
class TSHashTable;

class HASHKEY_LONGLONG {
 private:
  friend class TSHashObject<SoundFileDataCacheBlock, HASHKEY_LONGLONG>;

  friend void                     DataCacheInitialize(int cacheSizeMB);
  friend SoundFileDataCacheBlock *AllocCacheBlock(LONGLONG hashKey);
  friend class SoundFileCache;

  HASHKEY_LONGLONG() : m_key(0) {
  }

  HASHKEY_LONGLONG(int key) : m_key(key) {
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

 private:
  LONGLONG m_key;
};

class HASHKEY_STR {
 public:
  HASHKEY_STR() : m_str(0) {
  }

  HASHKEY_STR(LPCSTR str) : m_str(SStrDupA(str, __FILE__, __LINE__)) {
  }

  HASHKEY_STR(const HASHKEY_STR &key) : m_str(SStrDupA(key.m_str, __FILE__, __LINE__)) {
  }

  ~HASHKEY_STR();

  HASHKEY_STR &operator=(LPCSTR str);

  HASHKEY_STR &operator=(const HASHKEY_STR &key) {
    return operator=(key.m_str);
  }

  bool operator==(LPCSTR str) const {
    return SStrCmp(m_str, str, 0x7FFFFFFF) == 0;
  }

  bool operator==(const HASHKEY_STR &key) const {
    return operator==(key.m_str);
  }

  LPCSTR GetString() const {
    return m_str;
  }

 protected:
  char *m_str;
};

class HASHKEY_STRI : public HASHKEY_STR {
 public:
  HASHKEY_STRI() {
  }

  HASHKEY_STRI(LPCSTR str) : HASHKEY_STR(str) {
  }

  HASHKEY_STRI(const HASHKEY_STRI &key) : HASHKEY_STR(key) {
  }

  HASHKEY_STRI &operator=(LPCSTR str) {
    HASHKEY_STR::operator=(str);
    return *this;
  }

  HASHKEY_STRI &operator=(const HASHKEY_STRI &key) {
    HASHKEY_STR::operator=(key);
    return *this;
  }

  bool operator==(LPCSTR str) const {
    return SStrCmpI(m_str, str, 0x7FFFFFFF) == 0;
  }

  bool operator==(const HASHKEY_STRI &key) const {
    return operator==(key.m_str);
  }
};

class HASHKEY_CONSTSTR {
 public:
  HASHKEY_CONSTSTR() : m_str(0) {
  }

  HASHKEY_CONSTSTR(LPCSTR str) : m_str(str) {
  }

  bool operator==(LPCSTR str) const {
    return SStrCmp(m_str, str, 0x7FFFFFFF) == 0;
  }

  bool operator==(const HASHKEY_CONSTSTR &key) const {
    return operator==(key.m_str);
  }

  LPCSTR GetString() const {
    return m_str;
  }

 protected:
  LPCSTR m_str;
};

class HASHKEY_CONSTSTRI : public HASHKEY_CONSTSTR {
 public:
  HASHKEY_CONSTSTRI() {
  }

  HASHKEY_CONSTSTRI(LPCSTR str) : HASHKEY_CONSTSTR(str) {
  }

  bool operator==(LPCSTR str) const {
    return m_str == str || SStrCmpI(m_str, str, 0x7FFFFFFF) == 0;
  }

  bool operator==(const HASHKEY_CONSTSTRI &key) const {
    return operator==(key.m_str);
  }
};

class HASHKEY_PTR {
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

 private:
  LPVOID m_key;
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

  LPCSTR GetString() const {
    return m_key.GetString();
  }

  KEY GetKey() const {
    return m_key;
  }

  LPCVOID GetData() const;

  UINT GetHashValue() const {
    return m_hashval;
  }
};

template <class T, class KEY>
class TSHashTable {
 private:
  friend class CGameTime;

  virtual void InternalDelete(T *ptr);
  virtual T   *InternalNew(LISTEXDYN(T) * list, DWORD extrabytes, DWORD flags);

  UINT ComputeSlot(UINT hashval) const {
    return hashval & m_slotmask;
  }

  void GrowListArray(UINT newarraysize);
  void Initialize();

  int Initialized() {
    return m_slotmask != 0xFFFFFFFF;
  }

  void InternalClear(int warn);
  void InternalLinkNode(T *ptr, UINT hashval);
  T   *InternalNewNode(UINT hashval, DWORD extrabytes, DWORD flags);
  int  MonitorFullness(UINT slot);

  TSHashTable<T, KEY> &NonConst() const {
    return const_cast<TSHashTable<T, KEY> &>(*this);
  }

 protected:
  int GetLinkOffset() const {
    return reinterpret_cast<int>(&((T *)0)->m_linktoslot);
  }

 public:
  TSHashTable();
  TSHashTable<T, KEY> &operator=(const TSHashTable<T, KEY> &);
  virtual ~TSHashTable();

  void Clear() {
    InternalClear(0);
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

  void Delete(LPCSTR key) {
    T *ptr = Ptr(key);

    FATALASSERT(ptr);

    Delete(ptr);
  }

  void Delete(UINT hashval, LPCSTR key) {
    T *ptr = Ptr(hashval, key);

    FATALASSERT(ptr);

    Delete(ptr);
  }

  void Delete(UINT hashval, const KEY &key) {
    T *ptr = Ptr(hashval, key);

    FATALASSERT(ptr);

    Delete(ptr);
  }

  void Delete(T *ptr);

  T *DeleteNode(T *ptr) {
    T *next = Next(ptr);
    Delete(ptr);
    return next;
  }

  virtual void Destroy();

  T *Head() {
    return m_fulllist.Head();
  }

  const T *Head() const {
    return NonConst().Head();
  }

  void Insert(T *ptr, LPCSTR key) {
    Insert(ptr, SStrHashHT(key), key);
  }

  void Insert(T *ptr, UINT hashval, LPCSTR key) {
    InternalLinkNode(ptr, hashval);
    ptr->m_key = key;
  }

  void Insert(T *ptr, UINT hashval, const KEY &key) {
    InternalLinkNode(ptr, hashval);
    ptr->m_key = key;
  }

  T *New(LPCSTR key, DWORD extrabytes, DWORD flags) {
    return New(SStrHashHT(key), key, extrabytes, flags);
  }

  T *New(UINT hashval, LPCSTR key, DWORD extrabytes, DWORD flags) {
    T *ptr = InternalNewNode(hashval, extrabytes, flags);
    ptr->m_key = key;
    return ptr;
  }

  T *New(UINT hashval, const KEY &key, DWORD extrabytes, DWORD flags) {
    T *ptr = InternalNewNode(hashval, extrabytes, flags);
    ptr->m_key = key;
    return ptr;
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

  T *Ptr(LPCSTR key);

  const T *Ptr(LPCSTR key) const {
    return NonConst().Ptr(key);
  }

  T *Ptr(UINT hashval, LPCSTR key);

  const T *Ptr(UINT hashval, LPCSTR key) const;

  T *Ptr(UINT hashval, const KEY &key);

  const T *Ptr(UINT hashval, const KEY &key) const {
    return NonConst().Ptr(hashval, key);
  }

  T *RawNext(const T *ptr) {
    return m_fulllist.RawNext(ptr);
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

  void Unlink(T *ptr);

  static UINT Hash(LPCSTR key) {
    return SStrHashHT(key);
  }

 protected:
  LISTEXDYN(T) m_fulllist;

 private:
  UINT                          m_fullnessIndicator;
  TSGrowableArray<LISTEXDYN(T)> m_slotlistarray;
  UINT                          m_slotmask;
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
  void         Destructor();
  virtual void InternalDelete(T *ptr);
  virtual T   *InternalNew(LISTEXDYN(T) * list, DWORD extrabytes, DWORD flags);

 public:
  TSHashTableReuse();
  virtual ~TSHashTableReuse();
  virtual void Destroy();

 private:
  LISTEXDYN(T) m_reuseList;
  DWORD                                         m_chunkSize;
  TSExplicitList<TSHashObjectChunk<T, KEY>, 20> m_chunkList;
};

template <class T, class HANDLE, int REUSE>
class TSExportTableSimple : public TSHashTableReuse<T, HASHKEY_NONE, REUSE> {
 private:
  HASHKEY_NONE m_key;
  UINT         m_sequence;
  int          m_wrapped;

  HANDLE GenerateUniqueHandle();

 public:
  TSExportTableSimple();

  void Delete(HANDLE handle);
  void Delete(T *ptr);
  T   *New(HANDLE *handle);
  T   *Ptr(HANDLE handle);
};

template <class T, class HANDLE, class LOCKED, class SYNC, int REUSE>
class TSExportTableSync : public TSExportTableSimple<T, HANDLE, REUSE> {
 private:
  SYNC m_sync;

  int  IsForWriting(LOCKED lockedhandle);
  void SyncEnterLock(LOCKED *lockedhandle, int forwriting);
  void SyncLeaveLock(LOCKED lockedhandle);

 public:
  TSExportTableSync();

  void Delete(HANDLE handle);
  void DeleteUnlock(T *ptr, LOCKED lockedhandle);
  T   *Lock(HANDLE handle, LOCKED *lockedhandle, int forwriting);
  void New(HANDLE *handle);
  T   *NewLock(HANDLE *handle, LOCKED *lockedhandle);
  void Unlock(LOCKED lockedhandle);
};

template <class T, class KEY>
TSHashTable<T, KEY>::TSHashTable() : m_fullnessIndicator(0), m_slotmask(0xFFFFFFFF) {
  m_fulllist.ChangeLinkOffset(reinterpret_cast<int>(&((T *)0)->m_linktofull));
}

template <class T, class KEY>
TSHashTable<T, KEY>::~TSHashTable() {
  Destroy();
}

template <class T, class KEY>
void TSHashTable<T, KEY>::Delete(T *ptr) {
  Unlink(ptr);
  InternalDelete(ptr);
}

template <class T, class KEY>
void TSHashTable<T, KEY>::InternalDelete(T *ptr) {
  ptr->~T();
  SMemFree(ptr, typeid(T).INTERNALRAWNAME(), SERR_LINECODE_OBJECT, 0);
}

template <class T, class KEY>
T *TSHashTable<T, KEY>::InternalNew(LISTEXDYN(T) * list, DWORD extrabytes, DWORD flags) {
  return list->NewNode(LIST_HEAD, extrabytes, flags);
}

template <class T, class KEY>
void TSHashTable<T, KEY>::Initialize() {
  UINT index;

  m_slotmask = 3;
  m_slotlistarray.SetCount(4);
  for (index = 0; index <= m_slotmask; ++index) {
    m_slotlistarray[index].ChangeLinkOffset(GetLinkOffset());
  }
}

template <class T, class KEY>
void TSHashTable<T, KEY>::InternalClear(int warn) {
  UINT index;
  T   *ptr;

  m_fullnessIndicator = 0;
  m_fulllist.UnlinkAll();
  for (index = 0; index < m_slotlistarray.Count(); ++index) {
    while ((ptr = m_slotlistarray[index].Head()) != 0) {
      if (warn) {
        m_slotlistarray[index].UnlinkNode(ptr);
      } else {
        InternalDelete(ptr);
      }
    }
  }
}

template <class T, class KEY>
void TSHashTable<T, KEY>::Destroy() {
  InternalClear(1);
  m_fullnessIndicator = 0;
  m_slotmask = 0xFFFFFFFF;
  m_slotlistarray.Clear();
}

template <class T, class KEY>
void TSHashTable<T, KEY>::InternalLinkNode(T *ptr, UINT hashval) {
  UINT slot;

  if (!Initialized()) {
    Initialize();
  }

  slot = ComputeSlot(hashval);
  if (MonitorFullness(slot)) {
    slot = ComputeSlot(hashval);
  }

  m_slotlistarray[slot].LinkNode(ptr, LIST_LINK_AFTER, 0);
  m_fulllist.LinkNode(ptr, LIST_LINK_BEFORE, 0);
  ptr->m_hashval = hashval;
}

template <class T, class KEY>
T *TSHashTable<T, KEY>::InternalNewNode(UINT hashval, DWORD extrabytes, DWORD flags) {
  UINT slot;
  T   *ptr;

  if (!Initialized()) {
    Initialize();
  }

  slot = ComputeSlot(hashval);
  if (MonitorFullness(slot)) {
    slot = ComputeSlot(hashval);
  }

  ptr = InternalNew(&m_slotlistarray[slot], extrabytes, flags);
  m_fulllist.LinkNode(ptr, LIST_LINK_BEFORE, 0);
  ptr->m_hashval = hashval;
  return ptr;
}

template <class T, class KEY>
T *TSHashTable<T, KEY>::Ptr(LPCSTR key) {
  UINT hashval;
  UINT slot;
  T   *ptr;

  if (!Initialized()) {
    return 0;
  }

  hashval = SStrHashHT(key);
  slot = ComputeSlot(hashval);
  ptr = m_slotlistarray[slot].Head();
  while (reinterpret_cast<long>(ptr) > 0) {
    if (ptr->m_hashval == hashval && ptr->m_key == key) {
      return ptr;
    }
    ptr = m_slotlistarray[slot].RawNext(ptr);
  }
  return 0;
}

template <class T, class KEY>
T *TSHashTable<T, KEY>::Ptr(UINT hashval, LPCSTR key) {
  UINT slot;
  T   *ptr;

  if (!Initialized()) {
    return 0;
  }

  slot = ComputeSlot(hashval);
  ptr = m_slotlistarray[slot].Head();
  while (reinterpret_cast<long>(ptr) > 0) {
    if (ptr->m_hashval == hashval && ptr->m_key == key) {
      return ptr;
    }
    ptr = m_slotlistarray[slot].RawNext(ptr);
  }
  return 0;
}

template <class T, class KEY>
T *TSHashTable<T, KEY>::Ptr(UINT hashval, const KEY &key) {
  UINT slot;
  T   *ptr;

  if (!Initialized()) {
    return 0;
  }

  slot = ComputeSlot(hashval);
  ptr = m_slotlistarray[slot].Head();
  while (reinterpret_cast<long>(ptr) > 0) {
    if (ptr->m_hashval == hashval && ptr->m_key == key) {
      return ptr;
    }
    ptr = m_slotlistarray[slot].RawNext(ptr);
  }
  return 0;
}

template <class T, class KEY>
void TSHashTable<T, KEY>::Unlink(T *ptr) {
  if (ptr->m_linktoslot.IsLinked()) {
    ptr->m_linktoslot.Unlink();
    ptr->m_linktofull.Unlink();
  }
}

template <class T, class KEY>
int TSHashTable<T, KEY>::MonitorFullness(UINT slot) {
  T *ptr;

  if (m_slotmask >= 0x1FFF) {
    return 0;
  }

  if (m_fullnessIndicator > 3) {
    m_fullnessIndicator -= 3;
  } else {
    m_fullnessIndicator = 0;
  }

  ptr = m_slotlistarray[slot].Head();
  while (reinterpret_cast<long>(ptr) > 0) {
    ++m_fullnessIndicator;
    if (m_fullnessIndicator > 13) {
      break;
    }
    ptr = m_slotlistarray[slot].RawNext(ptr);
  }

  if (reinterpret_cast<long>(ptr) <= 0) {
    return 0;
  }

  m_fullnessIndicator = 0;
  GrowListArray((m_slotmask + 1) * 2);
  return 1;
}

template <class T, class KEY>
void TSHashTable<T, KEY>::GrowListArray(UINT newarraysize) {
  LISTEXDYN(T) templist;
  UINT oldarraysize = m_slotmask + 1;
  UINT index;
  T   *ptr;

  templist.ChangeLinkOffset(GetLinkOffset());
  for (index = 0; index < oldarraysize; ++index) {
    while ((ptr = m_slotlistarray[index].Head()) != 0) {
      templist.LinkNode(ptr, LIST_LINK_BEFORE, 0);
    }
  }

  m_slotlistarray.SetCount(newarraysize);
  for (index = 0; index < newarraysize; ++index) {
    m_slotlistarray[index].ChangeLinkOffset(GetLinkOffset());
  }

  m_slotmask = newarraysize - 1;
  while ((ptr = templist.Head()) != 0) {
    m_slotlistarray[ComputeSlot(ptr->m_hashval)].LinkNode(ptr, LIST_LINK_BEFORE, 0);
  }
}

template <class T, class KEY, int REUSE>
TSHashTableReuse<T, KEY, REUSE>::TSHashTableReuse() : m_chunkSize(16) {
  m_reuseList.ChangeLinkOffset(this->GetLinkOffset());
}

template <class T, class KEY, int REUSE>
TSHashTableReuse<T, KEY, REUSE>::~TSHashTableReuse() {
  Destructor();
}

template <class T, class KEY, int REUSE>
void TSHashTableReuse<T, KEY, REUSE>::Destructor() {
  m_chunkList.Clear();
  m_reuseList.Clear();
  m_chunkSize = 16;
}

template <class T, class KEY, int REUSE>
void TSHashTableReuse<T, KEY, REUSE>::InternalDelete(T *ptr) {
  this->m_fulllist.UnlinkNode(ptr);
  m_reuseList.LinkNode(ptr, LIST_HEAD, 0);
}

template <class T, class KEY, int REUSE>
T *TSHashTableReuse<T, KEY, REUSE>::InternalNew(LISTEXDYN(T) * list, DWORD extrabytes, DWORD flags) {
  T                         *ptr;
  TSHashObjectChunk<T, KEY> *chunk;

  ASSERT(!extrabytes);
  ptr = m_reuseList.Head();
  if (!ptr) {
    chunk = m_chunkList.Head();
    if (!chunk || !chunk->m_array.Reserved()) {
      chunk = m_chunkList.NewNode(LIST_HEAD, 0, 0);
      chunk->m_array.ReserveSpace(m_chunkSize);
      m_chunkSize *= 2;
    }
    ptr = chunk->m_array.NewElement();
  }

  list->LinkNode(ptr, LIST_HEAD, 0);
  return ptr;
}

template <class T, class KEY, int REUSE>
void TSHashTableReuse<T, KEY, REUSE>::Destroy() {
  this->Clear();
  Destructor();
}

template <class T, class HANDLE, int REUSE>
HANDLE TSExportTableSimple<T, HANDLE, REUSE>::GenerateUniqueHandle() {
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

template <class T, class HANDLE, int REUSE>
TSExportTableSimple<T, HANDLE, REUSE>::TSExportTableSimple() : m_sequence(0), m_wrapped(0) {
}

template <class T, class HANDLE, int REUSE>
void TSExportTableSimple<T, HANDLE, REUSE>::Delete(T *ptr) {
  TSHashTable<T, HASHKEY_NONE>::Delete(ptr);
}

template <class T, class HANDLE, int REUSE>
T *TSExportTableSimple<T, HANDLE, REUSE>::New(HANDLE *handle) {
  HANDLE newhandle = GenerateUniqueHandle();

  *handle = newhandle;
  return TSHashTable<T, HASHKEY_NONE>::New(reinterpret_cast<UINT>(newhandle), m_key, 0, 0);
}

template <class T, class HANDLE, int REUSE>
T *TSExportTableSimple<T, HANDLE, REUSE>::Ptr(HANDLE handle) {
  return TSHashTable<T, HASHKEY_NONE>::Ptr(reinterpret_cast<UINT>(handle), m_key);
}

template <class T, class HANDLE, class LOCKED, class SYNC, int REUSE>
int TSExportTableSync<T, HANDLE, LOCKED, SYNC, REUSE>::IsForWriting(LOCKED lockedhandle) {
  return reinterpret_cast<DWORD>(lockedhandle) == 1;
}

template <class T, class HANDLE, class LOCKED, class SYNC, int REUSE>
void TSExportTableSync<T, HANDLE, LOCKED, SYNC, REUSE>::SyncEnterLock(LOCKED *lockedhandle, int forwriting) {
  m_sync.Enter(forwriting);
  *lockedhandle = forwriting ? reinterpret_cast<LOCKED>(1) : reinterpret_cast<LOCKED>(-1);
}

template <class T, class HANDLE, class LOCKED, class SYNC, int REUSE>
void TSExportTableSync<T, HANDLE, LOCKED, SYNC, REUSE>::SyncLeaveLock(LOCKED lockedhandle) {
  if (lockedhandle) {
    m_sync.Leave(IsForWriting(lockedhandle));
  }
}

template <class T, class HANDLE, class LOCKED, class SYNC, int REUSE>
TSExportTableSync<T, HANDLE, LOCKED, SYNC, REUSE>::TSExportTableSync() {
}

template <class T, class HANDLE, class LOCKED, class SYNC, int REUSE>
void TSExportTableSync<T, HANDLE, LOCKED, SYNC, REUSE>::Delete(HANDLE handle) {
  LOCKED lockedhandle;
  T     *ptr = Lock(handle, &lockedhandle, 1);

  DeleteUnlock(ptr, lockedhandle);
}

template <class T, class HANDLE, class LOCKED, class SYNC, int REUSE>
void TSExportTableSync<T, HANDLE, LOCKED, SYNC, REUSE>::DeleteUnlock(T *ptr, LOCKED lockedhandle) {
  TSExportTableSimple<T, HANDLE, REUSE>::Delete(ptr);
  Unlock(lockedhandle);
}

template <class T, class HANDLE, class LOCKED, class SYNC, int REUSE>
T *TSExportTableSync<T, HANDLE, LOCKED, SYNC, REUSE>::Lock(HANDLE handle, LOCKED *lockedhandle, int forwriting) {
  T *ptr;

  SyncEnterLock(lockedhandle, forwriting);
  ptr = TSExportTableSimple<T, HANDLE, REUSE>::Ptr(handle);
  if (!ptr) {
    SyncLeaveLock(*lockedhandle);
    *lockedhandle = 0;
  }
  return ptr;
}

template <class T, class HANDLE, class LOCKED, class SYNC, int REUSE>
T *TSExportTableSync<T, HANDLE, LOCKED, SYNC, REUSE>::NewLock(HANDLE *handle, LOCKED *lockedhandle) {
  SyncEnterLock(lockedhandle, 1);
  return TSExportTableSimple<T, HANDLE, REUSE>::New(handle);
}

template <class T, class HANDLE, class LOCKED, class SYNC, int REUSE>
void TSExportTableSync<T, HANDLE, LOCKED, SYNC, REUSE>::Unlock(LOCKED lockedhandle) {
  SyncLeaveLock(lockedhandle);
}
