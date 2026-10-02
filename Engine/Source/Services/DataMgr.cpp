#include <Base/Base.h>

#include "Services/Camera.h"
#include "Base/Color.h"
#include "Tempest/c33matrix.h"
#include "Tempest/cimvector.h"

float CAngle::ClampTo2Pi(float angle) {
  const float twoPi = 2.0f * 3.14159265358979323846f;
  float       wrapped = angle - static_cast<int>(angle / twoPi) * twoPi;
  if (angle < 0.0f) {
    wrapped += twoPi;
  }
  return wrapped;
}

void CDataMgr::AddManaged(CBaseManaged *manage, UINT fieldId, UINT flags, UINT dataTypeId) {
  ASSERT(manage);
  ASSERT(fieldId < m_managedArray.Count());
  ASSERT(dataTypeId < CBaseManaged::DATATYPEIDS);

  manage->m_flags = static_cast<BYTE>(flags);
  manage->m_dataTypeId = static_cast<BYTE>(dataTypeId);
  m_managedArray[fieldId] = manage;

  if (flags & 0x1) {
    LinkManaged(manage);
  }
}

void CDataMgr::AddManaged(TManaged<NTempest::C3Vector> *manage, UINT fieldId, UINT flags) {
  AddManaged(manage, fieldId, flags, 3);
}

void CDataMgr::AddManaged(TManaged<NTempest::CImVector> *manage, UINT fieldId, UINT flags) {
  AddManaged(manage, fieldId, flags, 1);
}

void CDataMgr::AddManaged(TManaged<C3Color> *manage, UINT fieldId, UINT flags) {
  AddManaged(manage, fieldId, flags, 2);
}

void CDataMgr::AddManaged(TManaged<NTempest::C33Matrix> *manage, UINT fieldId, UINT flags) {
  AddManaged(manage, fieldId, flags, 4);
}

void CDataMgr::AddManaged(TManaged<int> *manage, UINT fieldId, UINT flags) {
  AddManaged(manage, fieldId, flags, 5);
}

void CDataMgr::AddManaged(TManaged<float> *manage, UINT fieldId, UINT flags) {
  AddManaged(manage, fieldId, flags, 6);
}

void CDataMgr::LinkManaged(CBaseManaged *m) {
  ITERATELIST(CBaseManaged, m_updateList, managed) {
    if (!(m->m_updatePriority < managed->m_updatePriority)) {
      m_updateList.LinkNode(m, LIST_LINK_BEFORE, managed);
      return;
    }
  }
  m_updateList.LinkNode(m, LIST_TAIL, 0);
}

void CDataMgr::Update(float elapsedSec) {
  ITERATELIST(CBaseManaged, m_updateList, managed) {
    if (managed->m_flags & 2) {
      managed->UpdateR(elapsedSec);
    } else {
      managed->Update(elapsedSec);
    }
  }
}

int DataMgrGetBool(HDATAMGR mgr, UINT fieldId) {
  FATALASSERT(mgr);
  CDataMgr  *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  const UINT typeId = 5;
  VALIDATEBEGIN;
  VALIDATE(mgrPtr);
  VALIDATE(fieldId < mgrPtr->m_managedArray.Count());
  VALIDATE(mgrPtr->m_managedArray[fieldId]);
  VALIDATE(typeId == mgrPtr->m_managedArray[fieldId]->m_dataTypeId);
  VALIDATEEND;
  CBaseManaged *managed = mgrPtr->m_managedArray[fieldId];
  if (managed->m_flags & 4) {
    if (managed->m_flags & 2) {
      managed->UpdateR(0.0f);
    } else {
      managed->Update(0.0f);
    }
  }
  return static_cast<TManaged<int> *>(managed)->Get();
}

void DataMgrGetColor(HDATAMGR mgr, UINT fieldId, NTempest::CImVector *result) {
  FATALASSERT(mgr);
  CDataMgr  *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  const UINT typeId = 1;
  VALIDATEBEGIN;
  VALIDATE(result);
  VALIDATE(mgrPtr);
  VALIDATE(fieldId < mgrPtr->m_managedArray.Count());
  VALIDATE(mgrPtr->m_managedArray[fieldId]);
  VALIDATE(typeId == mgrPtr->m_managedArray[fieldId]->m_dataTypeId);
  VALIDATEENDVOID;
  CBaseManaged *managed = mgrPtr->m_managedArray[fieldId];
  if (managed->m_flags & 4) {
    if (managed->m_flags & 2) {
      managed->UpdateR(0.0f);
    } else {
      managed->Update(0.0f);
    }
  }
  *result = static_cast<TManaged<NTempest::CImVector> *>(managed)->Get();
}

void DataMgrGetColor(HDATAMGR mgr, UINT fieldId, C3Color *result) {
  FATALASSERT(mgr);
  CDataMgr  *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  const UINT typeId = 2;
  VALIDATEBEGIN;
  VALIDATE(result);
  VALIDATE(mgrPtr);
  VALIDATE(fieldId < mgrPtr->m_managedArray.Count());
  VALIDATE(mgrPtr->m_managedArray[fieldId]);
  VALIDATE(typeId == mgrPtr->m_managedArray[fieldId]->m_dataTypeId);
  VALIDATEENDVOID;
  CBaseManaged *managed = mgrPtr->m_managedArray[fieldId];
  if (managed->m_flags & 4) {
    if (managed->m_flags & 2) {
      managed->UpdateR(0.0f);
    } else {
      managed->Update(0.0f);
    }
  }
  *result = static_cast<TManaged<C3Color> *>(managed)->Get();
}

void DataMgrGetCoord(HDATAMGR mgr, UINT fieldId, NTempest::C3Vector *result) {
  FATALASSERT(mgr);
  CDataMgr  *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  const UINT typeId = 3;
  VALIDATEBEGIN;
  VALIDATE(result);
  VALIDATE(mgrPtr);
  VALIDATE(fieldId < mgrPtr->m_managedArray.Count());
  VALIDATE(mgrPtr->m_managedArray[fieldId]);
  VALIDATE(typeId == mgrPtr->m_managedArray[fieldId]->m_dataTypeId);
  VALIDATEENDVOID;
  CBaseManaged *managed = mgrPtr->m_managedArray[fieldId];

  if (managed->m_flags & 0x4) {
    if (managed->m_flags & 0x2) {
      managed->UpdateR(0.0f);
    } else {
      managed->Update(0.0f);
    }
  }

  *result = static_cast<TManaged<NTempest::C3Vector> *>(managed)->Get();
}

void DataMgrGetC33Matrix(HDATAMGR mgr, UINT fieldId, NTempest::C33Matrix *result) {
  FATALASSERT(mgr);
  CDataMgr  *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  const UINT typeId = 4;
  VALIDATEBEGIN;
  VALIDATE(result);
  VALIDATE(mgrPtr);
  VALIDATE(fieldId < mgrPtr->m_managedArray.Count());
  VALIDATE(mgrPtr->m_managedArray[fieldId]);
  VALIDATE(typeId == mgrPtr->m_managedArray[fieldId]->m_dataTypeId);
  VALIDATEENDVOID;
  CBaseManaged *managed = mgrPtr->m_managedArray[fieldId];
  if (managed->m_flags & 4) {
    if (managed->m_flags & 2) {
      managed->UpdateR(0.0f);
    } else {
      managed->Update(0.0f);
    }
  }
  *result = static_cast<TManaged<NTempest::C33Matrix> *>(managed)->Get();
}

int DataMgrGetInt(HDATAMGR mgr, UINT fieldId) {
  FATALASSERT(mgr);
  CDataMgr  *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  const UINT typeId = 5;
  VALIDATEBEGIN;
  VALIDATE(mgrPtr);
  VALIDATE(fieldId < mgrPtr->m_managedArray.Count());
  VALIDATE(mgrPtr->m_managedArray[fieldId]);
  VALIDATE(typeId == mgrPtr->m_managedArray[fieldId]->m_dataTypeId);
  VALIDATEEND;
  CBaseManaged *managed = mgrPtr->m_managedArray[fieldId];
  if (managed->m_flags & 4) {
    if (managed->m_flags & 2) {
      managed->UpdateR(0.0f);
    } else {
      managed->Update(0.0f);
    }
  }
  return static_cast<TManaged<int> *>(managed)->Get();
}

float DataMgrGetFloat(HDATAMGR mgr, UINT fieldId) {
  FATALASSERT(mgr);
  CDataMgr  *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  const UINT typeId = 6;
  VALIDATEBEGIN;
  VALIDATE(mgrPtr);
  VALIDATE(fieldId < mgrPtr->m_managedArray.Count());
  VALIDATE(mgrPtr->m_managedArray[fieldId]);
  VALIDATE(typeId == mgrPtr->m_managedArray[fieldId]->m_dataTypeId);
  VALIDATEEND;
  CBaseManaged *managed = mgrPtr->m_managedArray[fieldId];

  if (managed->m_flags & 0x4) {
    if (managed->m_flags & 0x2) {
      managed->UpdateR(0.0f);
    } else {
      managed->Update(0.0f);
    }
  }

  return static_cast<TManaged<float> *>(managed)->Get();
}

void DataMgrGetUpdateInfo(HDATAMGR mgr, UINT fieldId, UpdateInfo *info) {
  ASSERT(mgr);
  CDataMgr *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  VALIDATEBEGIN;
  VALIDATE(mgrPtr);
  VALIDATE(fieldId < mgrPtr->m_managedArray.Count());
  VALIDATE(mgrPtr->m_managedArray[fieldId]);
  VALIDATE(info);
  VALIDATEENDVOID;
  CBaseManaged *managed = mgrPtr->m_managedArray[fieldId];
  info->updateFcn = managed->m_updateFcn;
  info->updateData = managed->m_updateData;
  info->updatePriority = managed->m_updatePriority;
}

void DataMgrSetBool(HDATAMGR mgr, UINT fieldId, int val) {
  FATALASSERT(mgr);
  CDataMgr  *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  const UINT typeId = 5;
  VALIDATEBEGIN;
  VALIDATE(mgrPtr);
  VALIDATE(fieldId < mgrPtr->m_managedArray.Count());
  VALIDATE(mgrPtr->m_managedArray[fieldId]);
  VALIDATE(typeId == mgrPtr->m_managedArray[fieldId]->m_dataTypeId);
  VALIDATE(!(mgrPtr->m_managedArray[fieldId]->m_flags & CBaseManaged::READONLY));
  VALIDATEENDVOID;
  CBaseManaged *managed = mgrPtr->m_managedArray[fieldId];
  managed->m_updateFcn = 0;
  managed->m_updateData = 0;
  managed->m_updatePriority = 0.0f;
  static_cast<TManaged<int> *>(managed)->Set_(val);
}

void DataMgrSetColor(HDATAMGR mgr, UINT fieldId, const NTempest::CImVector &color) {
  FATALASSERT(mgr);
  CDataMgr  *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  const UINT typeId = 1;
  VALIDATEBEGIN;
  VALIDATE(mgrPtr);
  VALIDATE(fieldId < mgrPtr->m_managedArray.Count());
  VALIDATE(mgrPtr->m_managedArray[fieldId]);
  VALIDATE(typeId == mgrPtr->m_managedArray[fieldId]->m_dataTypeId);
  VALIDATE(!(mgrPtr->m_managedArray[fieldId]->m_flags & CBaseManaged::READONLY));
  VALIDATEENDVOID;
  CBaseManaged *managed = mgrPtr->m_managedArray[fieldId];
  managed->m_updateFcn = 0;
  managed->m_updateData = 0;
  managed->m_updatePriority = 0.0f;
  static_cast<TManaged<NTempest::CImVector> *>(managed)->Set_(color);
}

void DataMgrSetColor(HDATAMGR mgr, UINT fieldId, const C3Color &color) {
  FATALASSERT(mgr);
  CDataMgr  *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  const UINT typeId = 2;
  VALIDATEBEGIN;
  VALIDATE(mgrPtr);
  VALIDATE(fieldId < mgrPtr->m_managedArray.Count());
  VALIDATE(mgrPtr->m_managedArray[fieldId]);
  VALIDATE(typeId == mgrPtr->m_managedArray[fieldId]->m_dataTypeId);
  VALIDATE(!(mgrPtr->m_managedArray[fieldId]->m_flags & CBaseManaged::READONLY));
  VALIDATEENDVOID;
  CBaseManaged *managed = mgrPtr->m_managedArray[fieldId];
  managed->m_updateFcn = 0;
  managed->m_updateData = 0;
  managed->m_updatePriority = 0.0f;
  static_cast<TManaged<C3Color> *>(managed)->Set_(color);
}

void DataMgrSetCoord(HDATAMGR mgr, UINT fieldId, const NTempest::C3Vector &coord, UINT coordFlags) {
  NTempest::C3Vector curr(0.0f);
  DataMgrGetCoord(mgr, fieldId, &curr);

  NTempest::C3Vector setTo(coordFlags & 0x1 ? curr.x : coord.x, coordFlags & 0x2 ? curr.y : coord.y, coordFlags & 0x4 ? curr.z : coord.z);

  FATALASSERT(mgr);
  CDataMgr  *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  const UINT typeId = 3;
  VALIDATEBEGIN;
  VALIDATE(mgrPtr);
  VALIDATE(fieldId < mgrPtr->m_managedArray.Count());
  VALIDATE(mgrPtr->m_managedArray[fieldId]);
  VALIDATE(typeId == mgrPtr->m_managedArray[fieldId]->m_dataTypeId);
  VALIDATE(!(mgrPtr->m_managedArray[fieldId]->m_flags & CBaseManaged::READONLY));
  VALIDATEENDVOID;
  CBaseManaged *managed = mgrPtr->m_managedArray[fieldId];

  managed->m_updateFcn = 0;
  managed->m_updateData = 0;
  managed->m_updatePriority = 0.0f;
  static_cast<TManaged<NTempest::C3Vector> *>(managed)->Set_(setTo);
}

void DataMgrSetC33Matrix(HDATAMGR mgr, UINT fieldId, const NTempest::C33Matrix &matrix) {
  FATALASSERT(mgr);
  CDataMgr  *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  const UINT typeId = 4;
  VALIDATEBEGIN;
  VALIDATE(mgrPtr);
  VALIDATE(fieldId < mgrPtr->m_managedArray.Count());
  VALIDATE(mgrPtr->m_managedArray[fieldId]);
  VALIDATE(typeId == mgrPtr->m_managedArray[fieldId]->m_dataTypeId);
  VALIDATE(!(mgrPtr->m_managedArray[fieldId]->m_flags & CBaseManaged::READONLY));
  VALIDATEENDVOID;
  CBaseManaged *managed = mgrPtr->m_managedArray[fieldId];
  managed->m_updateFcn = 0;
  managed->m_updateData = 0;
  managed->m_updatePriority = 0.0f;
  static_cast<TManaged<NTempest::C33Matrix> *>(managed)->Set_(matrix);
}

void DataMgrSetInt(HDATAMGR mgr, UINT fieldId, int val) {
  FATALASSERT(mgr);
  CDataMgr  *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  const UINT typeId = 5;
  VALIDATEBEGIN;
  VALIDATE(mgrPtr);
  VALIDATE(fieldId < mgrPtr->m_managedArray.Count());
  VALIDATE(mgrPtr->m_managedArray[fieldId]);
  VALIDATE(typeId == mgrPtr->m_managedArray[fieldId]->m_dataTypeId);
  VALIDATE(!(mgrPtr->m_managedArray[fieldId]->m_flags & CBaseManaged::READONLY));
  VALIDATEENDVOID;
  CBaseManaged *managed = mgrPtr->m_managedArray[fieldId];
  managed->m_updateFcn = 0;
  managed->m_updateData = 0;
  managed->m_updatePriority = 0.0f;
  static_cast<TManaged<int> *>(managed)->Set_(val);
}

void DataMgrSetFloat(HDATAMGR mgr, UINT fieldId, float val) {
  FATALASSERT(mgr);
  CDataMgr  *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  const UINT typeId = 6;
  VALIDATEBEGIN;
  VALIDATE(mgrPtr);
  VALIDATE(fieldId < mgrPtr->m_managedArray.Count());
  VALIDATE(mgrPtr->m_managedArray[fieldId]);
  VALIDATE(typeId == mgrPtr->m_managedArray[fieldId]->m_dataTypeId);
  VALIDATE(!(mgrPtr->m_managedArray[fieldId]->m_flags & CBaseManaged::READONLY));
  VALIDATEENDVOID;
  CBaseManaged *managed = mgrPtr->m_managedArray[fieldId];

  managed->m_updateFcn = 0;
  managed->m_updateData = 0;
  managed->m_updatePriority = 0.0f;
  static_cast<TManaged<float> *>(managed)->Set_(val);
}

static void
DataMgrSetFieldUpdate(HDATAMGR mgr, UINT fieldId, void (*updateFcn)(float, LPVOID, LPVOID), LPVOID updateData, float updatePriority, UINT typeId) {
  FATALASSERT(mgr);
  CDataMgr *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  VALIDATEBEGIN;
  VALIDATE(mgrPtr);
  VALIDATE(fieldId < mgrPtr->m_managedArray.Count());
  VALIDATE(mgrPtr->m_managedArray[fieldId]);
  VALIDATE(typeId == mgrPtr->m_managedArray[fieldId]->m_dataTypeId);
  VALIDATEENDVOID;

  CBaseManaged *managed = mgrPtr->m_managedArray[fieldId];
  managed->m_updateFcn = updateFcn;
  managed->m_updateData = updateData;
  managed->m_updatePriority = updatePriority;
  managed->m_flags |= CBaseManaged::REQUIRESUPDATE;
  managed->m_link.Unlink();

  if ((managed->m_flags & 1) || updateFcn) {
    mgrPtr->LinkManaged(managed);
  }
}

void DataMgrSetBoolUpdate(HDATAMGR mgr, UINT fieldId, void (*updateFcn)(float, LPVOID, int *), LPVOID updateData, float updatePriority) {
  DataMgrSetFieldUpdate(mgr, fieldId, reinterpret_cast<void (*)(float, LPVOID, LPVOID)>(updateFcn), updateData, updatePriority, 5);
}

void DataMgrSetColorUpdate(
    HDATAMGR mgr,
    UINT     fieldId,
    void (*updateFcn)(float, LPVOID, NTempest::CImVector *),
    LPVOID updateData,
    float  updatePriority
) {
  DataMgrSetFieldUpdate(mgr, fieldId, reinterpret_cast<void (*)(float, LPVOID, LPVOID)>(updateFcn), updateData, updatePriority, 1);
}

void DataMgrSetColorUpdate(HDATAMGR mgr, UINT fieldId, void (*updateFcn)(float, LPVOID, C3Color *), LPVOID updateData, float updatePriority) {
  DataMgrSetFieldUpdate(mgr, fieldId, reinterpret_cast<void (*)(float, LPVOID, LPVOID)>(updateFcn), updateData, updatePriority, 2);
}

void DataMgrSetCoordUpdate(
    HDATAMGR mgr,
    UINT     fieldId,
    void (*updateFcn)(float, LPVOID, NTempest::C3Vector *),
    LPVOID updateData,
    float  updatePriority
) {
  DataMgrSetFieldUpdate(mgr, fieldId, reinterpret_cast<void (*)(float, LPVOID, LPVOID)>(updateFcn), updateData, updatePriority, 3);
}

void DataMgrSetC33MatrixUpdate(
    HDATAMGR mgr,
    UINT     fieldId,
    void (*updateFcn)(float, LPVOID, NTempest::C33Matrix *),
    LPVOID updateData,
    float  updatePriority
) {
  DataMgrSetFieldUpdate(mgr, fieldId, reinterpret_cast<void (*)(float, LPVOID, LPVOID)>(updateFcn), updateData, updatePriority, 4);
}

void DataMgrSetIntUpdate(HDATAMGR mgr, UINT fieldId, void (*updateFcn)(float, LPVOID, int *), LPVOID updateData, float updatePriority) {
  DataMgrSetFieldUpdate(mgr, fieldId, reinterpret_cast<void (*)(float, LPVOID, LPVOID)>(updateFcn), updateData, updatePriority, 5);
}

void DataMgrSetFloatUpdate(HDATAMGR mgr, UINT fieldId, void (*updateFcn)(float, LPVOID, float *), LPVOID updateData, float updatePriority) {
  DataMgrSetFieldUpdate(mgr, fieldId, reinterpret_cast<void (*)(float, LPVOID, LPVOID)>(updateFcn), updateData, updatePriority, 6);
}
