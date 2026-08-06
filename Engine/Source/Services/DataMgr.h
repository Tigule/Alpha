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

namespace NTempest {
  bool operator==(const CImVector &left, const CImVector &right);
  bool operator!=(const CImVector &left, const CImVector &right);
  bool operator==(const C33Matrix &left, const C33Matrix &right);
  bool operator!=(const C33Matrix &left, const C33Matrix &right);
}  // namespace NTempest

struct UpdateInfo {
  void (*updateFcn)(float, LPVOID, LPVOID);
  LPVOID updateData;
  float  updatePriority;

  UpdateInfo();
};

class CBaseManaged {
 public:
  enum ManagedTypeIds {
    UNKNOWN,
    ALPHACOLOR,
    COLOR,
    COORD,
    C33MATRIX,
    INT,
    FLOAT,
    DATATYPEIDS
  };

  enum {
    ALWAYSUPDATE = 0x1,
    READONLY = 0x2,
    REQUIRESUPDATE = 0x4,
    UPDATED = 0x8
  };

  CBaseManaged() : m_dataTypeId(0), m_flags(0), m_updateFcn(0), m_updateData(0), m_updatePriority(0.0f) {
  }
  virtual ~CBaseManaged() {
  }
  virtual void Update(float) {
  }
  virtual void UpdateR(float) {
  }
  void GetInfo(UpdateInfo *info);
  void SetUpdate(void (*fcn)(float, LPVOID, LPVOID), LPVOID data, float priority) {
    m_updateFcn = fcn;
    m_updateData = data;
    m_updatePriority = priority;
  }

  LINKDECLEX(CBaseManaged, m_link);
  BYTE m_dataTypeId;
  BYTE m_flags;
  void (*m_updateFcn)(float, LPVOID, LPVOID);
  LPVOID m_updateData;
  float  m_updatePriority;
};

class CAngle;

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

  virtual void Update(float elapsedSec) {
    if (m_updateFcn) {
      T data = m_data;
      m_updateFcn(elapsedSec, m_updateData, &data);
      Set_(data);
    }
    m_flags &= ~REQUIRESUPDATE;
  }

  virtual void UpdateR(float elapsedSec) {
    if (m_updateFcn) {
      T data = m_data;
      T saved = data;
      m_updateFcn(elapsedSec, m_updateData, &data);
      ASSERT(data == saved);
    }
    m_flags &= ~REQUIRESUPDATE;
  }

  TManaged<T> &operator+=(const T &data);
  TManaged<T> &operator-=(const T &data);
  TManaged<T> &operator*=(const T &data);
  TManaged<T> &operator/=(const T &data);

  T &Get() {
    return m_data;
  }

  void Set(const T &data);

 private:
  friend class CAngle;

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
inline void TManaged<C3Color>::Set_(const C3Color &val) {
  if (m_data.r != val.r || m_data.g != val.g || m_data.b != val.b) {
    m_data = val;
    m_flags |= 0x8;
  }
}

class CDataMgr : public CHandleObject {
 public:
  TSFixedArray<CBaseManaged *> m_managedArray;
  LISTDECLEX(CBaseManaged, m_link, m_updateList);

 protected:
  CDataMgr(UINT count) {
    m_managedArray.SetCount(count);
  }

 private:
  void AddManaged(CBaseManaged *manage, UINT fieldId, UINT flags, UINT dataTypeId);

 protected:
  void AddManaged(TManaged<NTempest::CImVector> *manage, UINT fieldId, UINT flags);
  void AddManaged(TManaged<C3Color> *manage, UINT fieldId, UINT flags);
  void AddManaged(TManaged<NTempest::C3Vector> *manage, UINT fieldId, UINT flags);
  void AddManaged(TManaged<NTempest::C33Matrix> *manage, UINT fieldId, UINT flags);
  void AddManaged(TManaged<int> *manage, UINT fieldId, UINT flags);
  void AddManaged(TManaged<float> *manage, UINT fieldId, UINT flags);

 public:
  void LinkManaged(CBaseManaged *m);
  void Update(float elapsedSec);
};

void  DataMgrGetCoord(HDATAMGR mgr, UINT fieldId, NTempest::C3Vector *coord);
float DataMgrGetFloat(HDATAMGR mgr, UINT fieldId);

void DataMgrSetCoord(HDATAMGR mgr, UINT fieldId, const NTempest::C3Vector &coord, UINT coordFlags);

void DataMgrSetFloat(HDATAMGR mgr, UINT fieldId, float val);

void DataMgrSetBoolUpdate(HDATAMGR mgr, UINT fieldId, void (*updateFcn)(float, LPVOID, int *), LPVOID updateData, float updatePriority);
void DataMgrSetColorUpdate(
    HDATAMGR mgr,
    UINT     fieldId,
    void (*updateFcn)(float, LPVOID, NTempest::CImVector *),
    LPVOID updateData,
    float  updatePriority
);
void DataMgrSetColorUpdate(HDATAMGR mgr, UINT fieldId, void (*updateFcn)(float, LPVOID, C3Color *), LPVOID updateData, float updatePriority);
void DataMgrSetCoordUpdate(
    HDATAMGR mgr,
    UINT     fieldId,
    void (*updateFcn)(float, LPVOID, NTempest::C3Vector *),
    LPVOID updateData,
    float  updatePriority
);
void DataMgrSetC33MatrixUpdate(
    HDATAMGR mgr,
    UINT     fieldId,
    void (*updateFcn)(float, LPVOID, NTempest::C33Matrix *),
    LPVOID updateData,
    float  updatePriority
);
void DataMgrSetIntUpdate(HDATAMGR mgr, UINT fieldId, void (*updateFcn)(float, LPVOID, int *), LPVOID updateData, float updatePriority);
void DataMgrSetFloatUpdate(HDATAMGR mgr, UINT fieldId, void (*updateFcn)(float, LPVOID, float *), LPVOID updateData, float updatePriority);
