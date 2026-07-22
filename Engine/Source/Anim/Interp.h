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

  CArray(CArray<T> &source) : m_data(0), m_count(0) {
    Set(source.m_count, source.m_data);
  }

  ~CArray() {
    delete[] m_data;
  }

  CArray<T> &operator=(const CArray<T> &source) {
    if (this != &source) {
      Set(source.m_count, source.m_data);
    }
    return *this;
  }

  CArray<T> &operator=(TSFixedArray<T> &source) {
    Set(source.Count(), source.Ptr());
    return *this;
  }

  void Exchange(TSGrowableArray<T> *source) {
    unsigned int alloc;

    delete[] m_data;
    source->Detach(&m_data, &m_count, &alloc);
  }

  void ReserveSpace(unsigned int elements) {
    delete[] m_data;
    m_data = elements ? new T[elements] : 0;
  }

  void Set(unsigned int elements, const T *data) {
    ReserveSpace(elements);
    m_count = elements;
    if (elements) {
      ASSERT(data);
      memcpy(m_data, data, elements * sizeof(T));
    }
  }

  unsigned int Count() const {
    return m_count;
  }

  T &operator[](unsigned int index) {
    ASSERT(index < m_count);
    return m_data[index];
  }

  const T &operator[](unsigned int index) const {
    ASSERT(index < m_count);
    return m_data[index];
  }

  T           *m_data;
  unsigned int m_count;
};

struct CBaseStatus {
  CBaseStatus() : currSeq(0), flags(0x10) {
  }

  unsigned char currSeq;
  unsigned char flags;
};

struct CKeyTrackStatus {
  CKeyTrackStatus() : currKey(0), nextKey(0), timepastkey(0) {
  }

  unsigned int currKey;
  unsigned int nextKey;
  int          timepastkey;
};

struct CAnimObjStatus {
  CAnimObjStatus() {
    memset(this, 0, sizeof(*this));
    base.flags = 0x10;
  }

  CKeyTrackStatus translation;
  CKeyTrackStatus rotation;
  CKeyTrackStatus scale;
  CBaseStatus     base;
  unsigned char   lookAtId;
};

struct CAnimEventObjStatus : public CAnimObjStatus {
  CAnimEventObjStatus() {
    memset(this, 0, sizeof(*this));
    base.flags = 0x10;
  }

  CKeyTrackStatus    event;
  NTempest::C3Vector position;
};

struct CAnimModelObjStatus : public CAnimObjStatus {
  CAnimModelObjStatus() {
    memset(this, 0, sizeof(*this));
    base.flags = 0x10;
    visible = 1.0f;
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
  CAnimCameraObjStatus() {
    memset(this, 0, sizeof(*this));
    visible = 1.0f;
    base.flags = 0x10;
  }

  float           visible;
  CKeyTrackStatus visibility;
  CKeyTrackStatus translation;
  CKeyTrackStatus roll;
  CKeyTrackStatus targetTranslation;
  CBaseStatus     base;
};

struct CAnimLayerStatus {
  CAnimLayerStatus() {
    memset(this, 0, sizeof(*this));
    base.flags = 0x10;
  }

  CKeyTrackStatus visibility;
  CKeyTrackStatus flipIndex;
  CBaseStatus     base;
};

struct CAnimEmitter2ObjStatus : public CAnimObjStatus {
  CAnimEmitter2ObjStatus() {
    memset(this, 0, sizeof(*this));
    base.flags = 0x10;
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
  CAnimLightObjStatus() {
    memset(this, 0, sizeof(*this));
    base.flags = 0x10;
  }

  CKeyTrackStatus attenstart;
  CKeyTrackStatus attenend;
  CKeyTrackStatus color;
  CKeyTrackStatus intensity;
  CKeyTrackStatus visibility;
  CKeyTrackStatus ambColor;
  CKeyTrackStatus ambIntensity;
};

struct CAnimRibbonObjStatus : public CAnimObjStatus {
  CAnimRibbonObjStatus() {
    memset(this, 0, sizeof(*this));
    base.flags = 0x10;
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
    memset(this, 0, sizeof(*this));
    base.flags = 0x11;
  }

  CKeyTrackStatus color;
  CKeyTrackStatus visibility;
  CBaseStatus     base;
};
