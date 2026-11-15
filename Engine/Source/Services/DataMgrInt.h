#pragma once

#include "Services/DataMgr.h"

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

  LINKDECLEX(CBaseManaged, m_link);
  BYTE m_dataTypeId;
  BYTE m_flags;
  void (*m_updateFcn)(float, LPVOID, LPVOID);
  LPVOID m_updateData;
  float  m_updatePriority;

  CBaseManaged() : m_dataTypeId(0), m_flags(0), m_updateFcn(0), m_updateData(0), m_updatePriority(0.0f) {
  }
  virtual ~CBaseManaged() {
  }
  void GetInfo(UpdateInfo *info);
  void SetUpdate(void (*fcn)(float, LPVOID, LPVOID), LPVOID data, float priority) {
    m_updateFcn = fcn;
    m_updateData = data;
    m_updatePriority = priority;
  }

  virtual void Update(float) {
  }
  virtual void UpdateR(float) {
  }

};

class CAngle;

template <class T>
class TManaged : public CBaseManaged {
 private:
  T m_data;

 public:
  TManaged() : m_data() {
  }

  TManaged(const T &data) : m_data(data) {
  }

  virtual void Set_(const T &data) {
    if (m_data != data) {
      m_data = data;
      m_flags |= UPDATED;
    }
  }

  TManaged<T> &operator+=(const T &data);
  TManaged<T> &operator-=(const T &data);
  TManaged<T> &operator*=(const T &data);
  TManaged<T> &operator/=(const T &data);

  T &Get() {
    return m_data;
  }

  void Set(const T &data) {
    SetUpdate(0, 0, 0.0f);
    Set_(data);
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
      T saved = m_data;
      m_updateFcn(elapsedSec, m_updateData, &data);
      VALIDATE2VOID(data == saved);
    }
    m_flags &= ~REQUIRESUPDATE;
  }

 private:
  friend class CAngle;

};

template <>
inline void TManaged<NTempest::C3Vector>::Set_(const NTempest::C3Vector &val) {
  if (m_data.x != val.x || m_data.y != val.y || m_data.z != val.z) {
    m_data = val;
    m_flags |= UPDATED;
  }
}

template <>
inline void TManaged<C3Color>::Set_(const C3Color &val) {
  if (m_data.r != val.r || m_data.g != val.g || m_data.b != val.b) {
    m_data = val;
    m_flags |= UPDATED;
  }
}

class CAngle : public TManaged<float> {
 private:
  float m_cos;
  float m_sin;

  void  Calc();
  float ClampTo2Pi(float angle);

 protected:
  virtual void Set_(const float &angle);

 public:
  CAngle();
  CAngle(float angle);
  float Cos() {
    return m_cos;
  }
  float Sin() {
    return m_sin;
  }
};

inline void CAngle::Calc() {
  NTempest::CMath::sincos_(m_data, m_sin, m_cos);
}

inline CAngle::CAngle(float angle) {
  TManaged<float>::Set_(ClampTo2Pi(angle));
  Calc();
}

inline CAngle::CAngle() : TManaged<float>() {
  Calc();
}

inline void CAngle::Set_(const float &angle) {
  const float wrapped = ClampTo2Pi(angle);
  TManaged<float>::Set_(wrapped);
  Calc();
}

class CDataMgr : public CHandleObject {
 public:
  TSFixedArray<CBaseManaged *> m_managedArray;
  LISTDECLEX(CBaseManaged, m_link, m_updateList);

 private:
  void AddManaged(CBaseManaged *manage, UINT fieldId, UINT flags, UINT dataTypeId);

 protected:
  void AddManaged(TManaged<NTempest::CImVector> *manage, UINT fieldId, UINT flags);
  void AddManaged(TManaged<C3Color> *manage, UINT fieldId, UINT flags);
  void AddManaged(TManaged<NTempest::C3Vector> *manage, UINT fieldId, UINT flags);
  void AddManaged(TManaged<NTempest::C33Matrix> *manage, UINT fieldId, UINT flags);
  void AddManaged(TManaged<int> *manage, UINT fieldId, UINT flags);
  void AddManaged(TManaged<float> *manage, UINT fieldId, UINT flags);

  CDataMgr(UINT count) {
    m_managedArray.SetCount(count);
  }

 public:
  void LinkManaged(CBaseManaged *m);
  void Update(float elapsedSec);
};
