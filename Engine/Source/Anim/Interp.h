#pragma once

#include "Tempest/c3vector.h"
#include "Tempest/c4quaternion.h"

#include <storm.h>
#include <stpl.h>

template <class T>
class CArray {
 public:
  CArray() : m_data(0), m_count(0) {
  }

  CArray(const CArray<T> &source) : m_data(0), m_count(0) {
    Set(source.m_count, source.m_data);
  }

  ~CArray() {
    if (m_data) {
      delete[] m_data;
      m_data = 0;
    }
  }

  CArray<T> &operator=(const CArray<T> &source) {
    if (this != &source) {
      Set(source.m_count, source.m_data);
    }
    return *this;
  }

  CArray<T> &operator=(const TSFixedArray<T> &source) {
    Set(source.Count(), source.Ptr());
    return *this;
  }

  void Exchange(TSGrowableArray<T> *source) {
    UINT alloc;

    delete[] m_data;
    source->Detach(&m_data, &m_count, &alloc);
  }

  void ReserveSpace(UINT elements) {
    if (m_data) {
      delete[] m_data;
    }
    if (!elements) {
      m_data = 0;
      return;
    }

    m_data = new (__FILE__, __LINE__) T[elements];
  }

  void Zero() {
    memset(m_data, 0, Bytes());
  }

  T *New() {
    return &m_data[m_count++];
  }

  UINT Count() const {
    return m_count;
  }

  UINT Bytes() const {
    return m_count * sizeof(T);
  }

  T &operator[](UINT index) {
    ASSERT(index < m_count);
    return m_data[index];
  }

  const T &operator[](UINT index) const {
    ASSERT(index < m_count);
    return m_data[index];
  }

  T *Ptr() {
    return m_data;
  }

  const T *Ptr() const {
    return m_data;
  }

  void Clear() {
    m_count = 0;
  }

  void SetCount(UINT count) {
    m_count = count;
  }

  void Set(UINT elements, const T *data) {
    ReserveSpace(elements);
    m_count = elements;
    if (elements) {
      ASSERT(data);
      memcpy(m_data, data, elements * sizeof(T));
    }
  }

 private:
  T   *m_data;
  UINT m_count;
};

struct CBaseStatus {
  CBaseStatus() : currSeq(0), flags(0x10) {
  }

  BYTE currSeq;
  BYTE flags;
};

struct CKeyTrackStatus {
  CKeyTrackStatus() : currKey(0), nextKey(0), timepastkey(0) {
  }
  CKeyTrackStatus(const CKeyTrackStatus &source) : currKey(source.currKey), nextKey(source.nextKey), timepastkey(source.timepastkey) {
  }

  UINT currKey;
  UINT nextKey;
  int  timepastkey;
};

struct CAnimObjStatus {
  CAnimObjStatus() : lookAtId(0) {
  }
  CAnimObjStatus(const CAnimObjStatus &source)
      : translation(source.translation), rotation(source.rotation), scale(source.scale), base(source.base), lookAtId(source.lookAtId) {
  }

  CKeyTrackStatus translation;
  CKeyTrackStatus rotation;
  CKeyTrackStatus scale;
  CBaseStatus     base;
  BYTE            lookAtId;
};

struct CAnimEventObjStatus : public CAnimObjStatus {
  CAnimEventObjStatus() {
  }

  CKeyTrackStatus    event;
  NTempest::C3Vector position;
};

struct CAnimModelObjStatus : public CAnimObjStatus {
  CAnimModelObjStatus() : visible(1.0f) {
  }

  BOOL IsVisible() const {
    return visible > 0.0f;
  }

  CKeyTrackStatus visibility;
  float           visible;
};

struct CAnimObjBlendStatus {
  CAnimObjBlendStatus()
      : blendTimer(0),
        prevSeqPosition(0.0f),
        prevSeqRotation(1.0f, 0.0f, 0.0f, 0.0f),
        prevSeqScale(1.0f),
        blendPosition(0.0f),
        blendRotation(1.0f, 0.0f, 0.0f, 0.0f),
        blendScale(0.0f) {
  }

  int                    blendTimer;
  NTempest::C3Vector     prevSeqPosition;
  NTempest::C4Quaternion prevSeqRotation;
  NTempest::C3Vector     prevSeqScale;
  NTempest::C3Vector     blendPosition;
  NTempest::C4Quaternion blendRotation;
  NTempest::C3Vector     blendScale;
};

struct CAnimCameraObjStatus {
  CAnimCameraObjStatus() : visible(1.0f) {
  }

  BOOL IsVisible() const {
    return visible > 0.0f;
  }

  float           visible;
  CKeyTrackStatus visibility;
  CKeyTrackStatus translation;
  CKeyTrackStatus roll;
  CKeyTrackStatus targetTranslation;
  CBaseStatus     base;
};

struct CAnimLayerStatus {
  CKeyTrackStatus visibility;
  CKeyTrackStatus flipIndex;
  CBaseStatus     base;
};

struct CAnimEmitter2ObjStatus : public CAnimObjStatus {
  CAnimEmitter2ObjStatus() : elapsedTime(0.0f) {
  }

  CKeyTrackStatus speed;
  CKeyTrackStatus emissionRate;
  CKeyTrackStatus gravity;
  CKeyTrackStatus latitude;
  CKeyTrackStatus longitude;
  CKeyTrackStatus visibility;
  CKeyTrackStatus variation;
  CKeyTrackStatus length;
  CKeyTrackStatus width;
  CKeyTrackStatus zsource;
  CKeyTrackStatus lifeSpan;
  float           elapsedTime;
};

struct CAnimLightObjStatus : public CAnimObjStatus {
  CKeyTrackStatus attenstart;
  CKeyTrackStatus attenend;
  CKeyTrackStatus color;
  CKeyTrackStatus intensity;
  CKeyTrackStatus visibility;
  CKeyTrackStatus ambColor;
  CKeyTrackStatus ambIntensity;
};

struct CAnimRibbonObjStatus : public CAnimObjStatus {
  CAnimRibbonObjStatus() : elapsedTime(0.0f) {
  }

  CKeyTrackStatus visibility;
  CKeyTrackStatus heightAbove;
  CKeyTrackStatus heightBelow;
  CKeyTrackStatus color;
  CKeyTrackStatus alpha;
  CKeyTrackStatus slot;
  float           elapsedTime;
};

struct CAnimGeosetObjStatus {
  CAnimGeosetObjStatus() {
    base.flags |= 1;
  }

  BOOL IsVisible() const {
    return base.flags & 1;
  }

  CKeyTrackStatus color;
  CKeyTrackStatus visibility;
  CBaseStatus     base;
};
