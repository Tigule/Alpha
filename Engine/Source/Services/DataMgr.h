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
  void(__fastcall *updateFcn)(float, void *, void *);
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

  TSLink<CBaseManaged> m_link;
  unsigned char        m_dataTypeId;
  unsigned char        m_flags;
  void(__fastcall *m_updateFcn)(float, void *, void *);
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
  virtual ~CDataMgr() {
  }

  TSFixedArray<CBaseManaged *>    m_managedArray;
  TSExplicitList<CBaseManaged, 4> m_updateList;

 protected:
  CDataMgr(unsigned int count) {
    m_managedArray.SetCount(count);
  }

 private:
  void AddManaged(CBaseManaged *manage, unsigned int fieldId, unsigned int flags, unsigned int dataTypeId);

 protected:
  void AddManaged(TManaged<NTempest::C3Vector> *manage, unsigned int fieldId, unsigned int flags);
  void AddManaged(TManaged<float> *manage, unsigned int fieldId, unsigned int flags);

 public:
  void LinkManaged(CBaseManaged *m);
  void Update(float elapsedSec);
};

void __fastcall  DataMgrGetCoord(HDATAMGR mgr, unsigned int fieldId, NTempest::C3Vector *coord);
float __fastcall DataMgrGetFloat(HDATAMGR mgr, unsigned int fieldId);

void __fastcall DataMgrSetCoord(HDATAMGR mgr, unsigned int fieldId, const NTempest::C3Vector &coord, unsigned int coordFlags);

void __fastcall DataMgrSetFloat(HDATAMGR mgr, unsigned int fieldId, float val);
