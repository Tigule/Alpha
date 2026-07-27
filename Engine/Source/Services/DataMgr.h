#pragma once

#include "Base/Handle.h"
#include "Base/Color.h"
#include "Tempest/c33matrix.h"
#include "Tempest/c3vector.h"
#include "Tempest/cimvector.h"

#include <stddef.h>
#include <string.h>
#include <stpl.h>

DECLARE_DERIVED_HANDLE(HDATAMGR, HOBJECT);

struct UpdateInfo {
  void(*updateFcn)(float, void *, void *);
  void *updateData;
  float updatePriority;
};

class CBaseManaged {
 public:
  CBaseManaged() : m_dataTypeId(0), m_flags(0), m_updateFcn(0), m_updateData(0), m_updatePriority(0.0f) {
  }
  virtual ~CBaseManaged() {
  }
  virtual void Update(float __formal) {
  }
  virtual void UpdateR(float __formal) {
  }
  void SetUpdate(void(*fcn)(float, void *, void *), void *data, float priority) {
    m_updateFcn = fcn;
    m_updateData = data;
    m_updatePriority = priority;
  }

  TSLink<CBaseManaged> m_link;
  unsigned char        m_dataTypeId;
  unsigned char        m_flags;
  void(*m_updateFcn)(float, void *, void *);
  void *m_updateData;
  float m_updatePriority;
};

template <class T>
class TManaged : public CBaseManaged {
 public:
  TManaged() : m_data() {
  }

  TManaged(const T &data) : m_data(data) {
  }

  virtual void Set_(const T &data) {
    if (m_data != data) {
      m_data = data;
      m_flags |= 0x8;
    }
  }

  T m_data;
};

template <>
inline void TManaged<NTempest::C3Vector>::Set_(const NTempest::C3Vector &val) {
  if (m_data.x != val.x || m_data.y != val.y || m_data.z != val.z) {
    m_data = val;
    m_flags |= 0x8;
  }
}

template <>
inline void TManaged<NTempest::CImVector>::Set_(const NTempest::CImVector &val) {
  if (*m_data.IV_() != *val.IV_()) {
    m_data = val;
    m_flags |= 0x8;
  }
}

template <>
inline void TManaged<C3Color>::Set_(const C3Color &val) {
  if (m_data.r != val.r || m_data.g != val.g || m_data.b != val.b) {
    m_data = val;
    m_flags |= 0x8;
  }
}

template <>
inline void TManaged<NTempest::C33Matrix>::Set_(const NTempest::C33Matrix &val) {
  if (memcmp(&m_data, &val, sizeof(val))) {
    m_data = val;
    m_flags |= 0x8;
  }
}

class CDataMgr : public CHandleObject {
 public:
  TSFixedArray<CBaseManaged *>    m_managedArray;
  TSExplicitList<CBaseManaged, 4> m_updateList;

 protected:
  CDataMgr(unsigned int count) {
    m_managedArray.SetCount(count);
  }

 private:
  void AddManaged(CBaseManaged *manage, unsigned int fieldId, unsigned int flags, unsigned int dataTypeId);

 protected:
  void AddManaged(TManaged<NTempest::CImVector> *manage, unsigned int fieldId, unsigned int flags);
  void AddManaged(TManaged<C3Color> *manage, unsigned int fieldId, unsigned int flags);
  void AddManaged(TManaged<NTempest::C3Vector> *manage, unsigned int fieldId, unsigned int flags);
  void AddManaged(TManaged<NTempest::C33Matrix> *manage, unsigned int fieldId, unsigned int flags);
  void AddManaged(TManaged<int> *manage, unsigned int fieldId, unsigned int flags);
  void AddManaged(TManaged<float> *manage, unsigned int fieldId, unsigned int flags);

 public:
  void LinkManaged(CBaseManaged *m);
  void Update(float elapsedSec);
};

void DataMgrGetCoord(HDATAMGR mgr, unsigned int fieldId, NTempest::C3Vector *coord);
float DataMgrGetFloat(HDATAMGR mgr, unsigned int fieldId);

void DataMgrSetCoord(HDATAMGR mgr, unsigned int fieldId, const NTempest::C3Vector &coord, unsigned int coordFlags);

void DataMgrSetFloat(HDATAMGR mgr, unsigned int fieldId, float val);

void DataMgrSetBoolUpdate(HDATAMGR mgr, unsigned int fieldId, void(*updateFcn)(float, void *, int *), void *updateData, float updatePriority);
void DataMgrSetColorUpdate(HDATAMGR mgr, unsigned int fieldId, void(*updateFcn)(float, void *, NTempest::CImVector *), void *updateData, float updatePriority);
void DataMgrSetColorUpdate(HDATAMGR mgr, unsigned int fieldId, void(*updateFcn)(float, void *, C3Color *), void *updateData, float updatePriority);
void DataMgrSetCoordUpdate(HDATAMGR mgr, unsigned int fieldId, void(*updateFcn)(float, void *, NTempest::C3Vector *), void *updateData, float updatePriority);
void DataMgrSetC33MatrixUpdate(HDATAMGR mgr, unsigned int fieldId, void(*updateFcn)(float, void *, NTempest::C33Matrix *), void *updateData, float updatePriority);
void DataMgrSetIntUpdate(HDATAMGR mgr, unsigned int fieldId, void(*updateFcn)(float, void *, int *), void *updateData, float updatePriority);
void DataMgrSetFloatUpdate(HDATAMGR mgr, unsigned int fieldId, void(*updateFcn)(float, void *, float *), void *updateData, float updatePriority);
