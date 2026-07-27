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

void CDataMgr::AddManaged(CBaseManaged *manage, unsigned int fieldId, unsigned int flags, unsigned int dataTypeId) {
  ASSERT(manage);
  ASSERT(fieldId < m_managedArray.Count());
  ASSERT(dataTypeId < 7);

  manage->m_flags = static_cast<unsigned char>(flags);
  manage->m_dataTypeId = static_cast<unsigned char>(dataTypeId);
  m_managedArray[fieldId] = manage;

  if (flags & 0x1) {
    LinkManaged(manage);
  }
}

void CDataMgr::AddManaged(TManaged<NTempest::C3Vector> *manage, unsigned int fieldId, unsigned int flags) {
  AddManaged(manage, fieldId, flags, 3);
}

void CDataMgr::AddManaged(TManaged<NTempest::CImVector> *manage, unsigned int fieldId, unsigned int flags) {
  AddManaged(manage, fieldId, flags, 1);
}

void CDataMgr::AddManaged(TManaged<C3Color> *manage, unsigned int fieldId, unsigned int flags) {
  AddManaged(manage, fieldId, flags, 2);
}

void CDataMgr::AddManaged(TManaged<NTempest::C33Matrix> *manage, unsigned int fieldId, unsigned int flags) {
  AddManaged(manage, fieldId, flags, 4);
}

void CDataMgr::AddManaged(TManaged<int> *manage, unsigned int fieldId, unsigned int flags) {
  AddManaged(manage, fieldId, flags, 5);
}

void CDataMgr::AddManaged(TManaged<float> *manage, unsigned int fieldId, unsigned int flags) {
  AddManaged(manage, fieldId, flags, 6);
}

void CDataMgr::LinkManaged(CBaseManaged *m) {
  CBaseManaged *insertBefore = m_updateList.Head();
  while (insertBefore && m->m_updatePriority < insertBefore->m_updatePriority) {
    insertBefore = m_updateList.Next(insertBefore);
  }
  m_updateList.LinkNode(m, LIST_LINK_BEFORE, insertBefore);
}

void CDataMgr::Update(float elapsedSec) {
  CBaseManaged *managed = m_updateList.Head();
  while (managed) {
    if (managed->m_flags & 2) {
      managed->UpdateR(elapsedSec);
    } else {
      managed->Update(elapsedSec);
    }
    managed = m_updateList.Next(managed);
  }
}

int DataMgrGetBool(HDATAMGR__* mgr, unsigned int fieldId) {
  CDataMgr *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  FATALASSERT(mgrPtr);
  FATALASSERT(fieldId < mgrPtr->m_managedArray.Count());
  FATALASSERT(mgrPtr->m_managedArray[fieldId]);
  CBaseManaged *managed = mgrPtr->m_managedArray[fieldId];
  FATALASSERT(managed->m_dataTypeId == 5);
  if (managed->m_flags & 4) {
    if (managed->m_flags & 2) {
      managed->UpdateR(0.0f);
    } else {
      managed->Update(0.0f);
    }
  }
  return static_cast<TManaged<int> *>(managed)->Get();
}

void DataMgrGetColor(HDATAMGR__* mgr, unsigned int fieldId, NTempest::CImVector* color) {
  ASSERT(mgr);
  FATALASSERT(color);
  CDataMgr *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  FATALASSERT(mgrPtr);
  FATALASSERT(fieldId < mgrPtr->m_managedArray.Count());
  FATALASSERT(mgrPtr->m_managedArray[fieldId]);
  CBaseManaged *managed = mgrPtr->m_managedArray[fieldId];
  FATALASSERT(managed->m_dataTypeId == 1);
  if (managed->m_flags & 4) {
    if (managed->m_flags & 2) {
      managed->UpdateR(0.0f);
    } else {
      managed->Update(0.0f);
    }
  }
  *color = static_cast<TManaged<NTempest::CImVector> *>(managed)->Get();
}

void DataMgrGetColor(HDATAMGR__* mgr, unsigned int fieldId, C3Color* color) {
  ASSERT(mgr);
  FATALASSERT(color);
  CDataMgr *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  FATALASSERT(mgrPtr);
  FATALASSERT(fieldId < mgrPtr->m_managedArray.Count());
  FATALASSERT(mgrPtr->m_managedArray[fieldId]);
  CBaseManaged *managed = mgrPtr->m_managedArray[fieldId];
  FATALASSERT(managed->m_dataTypeId == 2);
  if (managed->m_flags & 4) {
    if (managed->m_flags & 2) {
      managed->UpdateR(0.0f);
    } else {
      managed->Update(0.0f);
    }
  }
  *color = static_cast<TManaged<C3Color> *>(managed)->Get();
}

void DataMgrGetCoord(HDATAMGR mgr, unsigned int fieldId, NTempest::C3Vector *coord) {
  ASSERT(mgr);
  ASSERT(coord);

  CDataMgr *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  ASSERT(fieldId < mgrPtr->m_managedArray.Count());
  ASSERT(mgrPtr->m_managedArray[fieldId]);

  CBaseManaged      *managed = mgrPtr->m_managedArray[fieldId];
  const unsigned int typeId = 3;
  ASSERT(typeId == managed->m_dataTypeId);

  if (managed->m_flags & 0x4) {
    if (managed->m_flags & 0x2) {
      managed->UpdateR(0.0f);
    } else {
      managed->Update(0.0f);
    }
  }

  *coord = static_cast<TManaged<NTempest::C3Vector> *>(managed)->Get();
}

void DataMgrGetC33Matrix(HDATAMGR__* mgr, unsigned int fieldId, NTempest::C33Matrix* matrix) {
  ASSERT(mgr);
  FATALASSERT(matrix);
  CDataMgr *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  FATALASSERT(mgrPtr);
  FATALASSERT(fieldId < mgrPtr->m_managedArray.Count());
  FATALASSERT(mgrPtr->m_managedArray[fieldId]);
  CBaseManaged *managed = mgrPtr->m_managedArray[fieldId];
  FATALASSERT(managed->m_dataTypeId == 4);
  if (managed->m_flags & 4) {
    if (managed->m_flags & 2) {
      managed->UpdateR(0.0f);
    } else {
      managed->Update(0.0f);
    }
  }
  *matrix = static_cast<TManaged<NTempest::C33Matrix> *>(managed)->Get();
}

int DataMgrGetInt(HDATAMGR__* mgr, unsigned int fieldId) {
  CDataMgr *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  FATALASSERT(mgrPtr);
  FATALASSERT(fieldId < mgrPtr->m_managedArray.Count());
  FATALASSERT(mgrPtr->m_managedArray[fieldId]);
  CBaseManaged *managed = mgrPtr->m_managedArray[fieldId];
  FATALASSERT(managed->m_dataTypeId == 5);
  if (managed->m_flags & 4) {
    if (managed->m_flags & 2) {
      managed->UpdateR(0.0f);
    } else {
      managed->Update(0.0f);
    }
  }
  return static_cast<TManaged<int> *>(managed)->Get();
}

float DataMgrGetFloat(HDATAMGR mgr, unsigned int fieldId) {
  CDataMgr *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  FATALASSERT(mgrPtr);
  FATALASSERT(fieldId < mgrPtr->m_managedArray.Count());
  FATALASSERT(mgrPtr->m_managedArray[fieldId]);

  CBaseManaged      *managed = mgrPtr->m_managedArray[fieldId];
  const unsigned int typeId = 6;
  FATALASSERT(typeId == managed->m_dataTypeId);

  if (managed->m_flags & 0x4) {
    if (managed->m_flags & 0x2) {
      managed->UpdateR(0.0f);
    } else {
      managed->Update(0.0f);
    }
  }

  return static_cast<TManaged<float> *>(managed)->Get();
}

void DataMgrGetUpdateInfo(HDATAMGR__* mgr, unsigned int fieldId, UpdateInfo* info) {
  ASSERT(mgr);
  CDataMgr *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  FATALASSERT(mgrPtr);
  FATALASSERT(fieldId < mgrPtr->m_managedArray.Count());
  FATALASSERT(mgrPtr->m_managedArray[fieldId]);
  FATALASSERT(info);
  CBaseManaged *managed = mgrPtr->m_managedArray[fieldId];
  info->updateFcn = managed->m_updateFcn;
  info->updateData = managed->m_updateData;
  info->updatePriority = managed->m_updatePriority;
}

void DataMgrSetBool(HDATAMGR__* mgr, unsigned int fieldId, int val) {
  CDataMgr *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  ASSERT(mgrPtr);
  ASSERT(fieldId < mgrPtr->m_managedArray.Count());
  ASSERT(mgrPtr->m_managedArray[fieldId]);
  CBaseManaged *managed = mgrPtr->m_managedArray[fieldId];
  ASSERT(managed->m_dataTypeId == 5);
  ASSERT(!(managed->m_flags & 2));
  managed->m_updateFcn = 0;
  managed->m_updateData = 0;
  managed->m_updatePriority = 0.0f;
  static_cast<TManaged<int> *>(managed)->Set_(val);
}

void DataMgrSetColor(HDATAMGR__* mgr, unsigned int fieldId, const NTempest::CImVector& color) {
  CDataMgr *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  ASSERT(mgrPtr);
  ASSERT(fieldId < mgrPtr->m_managedArray.Count());
  ASSERT(mgrPtr->m_managedArray[fieldId]);
  CBaseManaged *managed = mgrPtr->m_managedArray[fieldId];
  ASSERT(managed->m_dataTypeId == 1);
  ASSERT(!(managed->m_flags & 2));
  managed->m_updateFcn = 0;
  managed->m_updateData = 0;
  managed->m_updatePriority = 0.0f;
  static_cast<TManaged<NTempest::CImVector> *>(managed)->Set_(color);
}

void DataMgrSetColor(HDATAMGR__* mgr, unsigned int fieldId, const C3Color& color) {
  CDataMgr *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  ASSERT(mgrPtr);
  ASSERT(fieldId < mgrPtr->m_managedArray.Count());
  ASSERT(mgrPtr->m_managedArray[fieldId]);
  CBaseManaged *managed = mgrPtr->m_managedArray[fieldId];
  ASSERT(managed->m_dataTypeId == 2);
  ASSERT(!(managed->m_flags & 2));
  managed->m_updateFcn = 0;
  managed->m_updateData = 0;
  managed->m_updatePriority = 0.0f;
  static_cast<TManaged<C3Color> *>(managed)->Set_(color);
}

void DataMgrSetCoord(HDATAMGR mgr, unsigned int fieldId, const NTempest::C3Vector &coord, unsigned int coordFlags) {
  NTempest::C3Vector current(0.0f);
  DataMgrGetCoord(mgr, fieldId, &current);

  NTempest::C3Vector setTo(coordFlags & 0x1 ? current.x : coord.x, coordFlags & 0x2 ? current.y : coord.y, coordFlags & 0x4 ? current.z : coord.z);

  CDataMgr *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  ASSERT(mgrPtr);
  ASSERT(fieldId < mgrPtr->m_managedArray.Count());
  ASSERT(mgrPtr->m_managedArray[fieldId]);

  CBaseManaged *managed = mgrPtr->m_managedArray[fieldId];
  ASSERT(managed->m_dataTypeId == 3);
  ASSERT(!(managed->m_flags & 0x2));

  managed->m_updateFcn = 0;
  managed->m_updateData = 0;
  managed->m_updatePriority = 0.0f;
  static_cast<TManaged<NTempest::C3Vector> *>(managed)->Set_(setTo);
}

void DataMgrSetC33Matrix(HDATAMGR__* mgr, unsigned int fieldId, const NTempest::C33Matrix& matrix) {
  CDataMgr *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  ASSERT(mgrPtr);
  ASSERT(fieldId < mgrPtr->m_managedArray.Count());
  ASSERT(mgrPtr->m_managedArray[fieldId]);
  CBaseManaged *managed = mgrPtr->m_managedArray[fieldId];
  ASSERT(managed->m_dataTypeId == 4);
  ASSERT(!(managed->m_flags & 2));
  managed->m_updateFcn = 0;
  managed->m_updateData = 0;
  managed->m_updatePriority = 0.0f;
  static_cast<TManaged<NTempest::C33Matrix> *>(managed)->Set_(matrix);
}

void DataMgrSetInt(HDATAMGR__* mgr, unsigned int fieldId, int val) {
  CDataMgr *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  ASSERT(mgrPtr);
  ASSERT(fieldId < mgrPtr->m_managedArray.Count());
  ASSERT(mgrPtr->m_managedArray[fieldId]);
  CBaseManaged *managed = mgrPtr->m_managedArray[fieldId];
  ASSERT(managed->m_dataTypeId == 5);
  ASSERT(!(managed->m_flags & 2));
  managed->m_updateFcn = 0;
  managed->m_updateData = 0;
  managed->m_updatePriority = 0.0f;
  static_cast<TManaged<int> *>(managed)->Set_(val);
}

void DataMgrSetFloat(HDATAMGR mgr, unsigned int fieldId, float val) {
  CDataMgr *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  ASSERT(mgrPtr);
  ASSERT(fieldId < mgrPtr->m_managedArray.Count());
  ASSERT(mgrPtr->m_managedArray[fieldId]);

  CBaseManaged *managed = mgrPtr->m_managedArray[fieldId];
  ASSERT(managed->m_dataTypeId == 6);
  ASSERT(!(managed->m_flags & 0x2));

  managed->m_updateFcn = 0;
  managed->m_updateData = 0;
  managed->m_updatePriority = 0.0f;
  static_cast<TManaged<float> *>(managed)->Set_(val);
}

static void DataMgrSetFieldUpdate(
    HDATAMGR mgr,
    unsigned int fieldId,
    void(*updateFcn)(float, void *, void *),
    void *updateData,
    float updatePriority,
    unsigned int typeId
) {
  ASSERT(mgr);
  CDataMgr *mgrPtr = reinterpret_cast<CDataMgr *>(mgr);
  FATALASSERT(mgrPtr);
  FATALASSERT(fieldId < mgrPtr->m_managedArray.Count());
  FATALASSERT(mgrPtr->m_managedArray[fieldId]);
  FATALASSERT(typeId == mgrPtr->m_managedArray[fieldId]->m_dataTypeId);

  CBaseManaged *managed = mgrPtr->m_managedArray[fieldId];
  managed->m_updatePriority = updatePriority;
  managed->m_flags |= 4;
  managed->m_updateFcn = updateFcn;
  managed->m_updateData = updateData;
  mgrPtr->m_updateList.UnlinkNode(managed);

  if ((managed->m_flags & 1) || updateFcn) {
    mgrPtr->LinkManaged(managed);
  }
}

void DataMgrSetBoolUpdate(HDATAMGR mgr, unsigned int fieldId, void(*updateFcn)(float, void *, int *), void *updateData, float updatePriority) {
  DataMgrSetFieldUpdate(mgr, fieldId, reinterpret_cast<void(*)(float, void *, void *)>(updateFcn), updateData, updatePriority, 5);
}

void DataMgrSetColorUpdate(HDATAMGR mgr, unsigned int fieldId, void(*updateFcn)(float, void *, NTempest::CImVector *), void *updateData, float updatePriority) {
  DataMgrSetFieldUpdate(mgr, fieldId, reinterpret_cast<void(*)(float, void *, void *)>(updateFcn), updateData, updatePriority, 1);
}

void DataMgrSetColorUpdate(HDATAMGR mgr, unsigned int fieldId, void(*updateFcn)(float, void *, C3Color *), void *updateData, float updatePriority) {
  DataMgrSetFieldUpdate(mgr, fieldId, reinterpret_cast<void(*)(float, void *, void *)>(updateFcn), updateData, updatePriority, 2);
}

void DataMgrSetCoordUpdate(HDATAMGR mgr, unsigned int fieldId, void(*updateFcn)(float, void *, NTempest::C3Vector *), void *updateData, float updatePriority) {
  DataMgrSetFieldUpdate(mgr, fieldId, reinterpret_cast<void(*)(float, void *, void *)>(updateFcn), updateData, updatePriority, 3);
}

void DataMgrSetC33MatrixUpdate(HDATAMGR mgr, unsigned int fieldId, void(*updateFcn)(float, void *, NTempest::C33Matrix *), void *updateData, float updatePriority) {
  DataMgrSetFieldUpdate(mgr, fieldId, reinterpret_cast<void(*)(float, void *, void *)>(updateFcn), updateData, updatePriority, 4);
}

void DataMgrSetIntUpdate(HDATAMGR mgr, unsigned int fieldId, void(*updateFcn)(float, void *, int *), void *updateData, float updatePriority) {
  DataMgrSetFieldUpdate(mgr, fieldId, reinterpret_cast<void(*)(float, void *, void *)>(updateFcn), updateData, updatePriority, 5);
}

void DataMgrSetFloatUpdate(HDATAMGR mgr, unsigned int fieldId, void(*updateFcn)(float, void *, float *), void *updateData, float updatePriority) {
  DataMgrSetFieldUpdate(mgr, fieldId, reinterpret_cast<void(*)(float, void *, void *)>(updateFcn), updateData, updatePriority, 6);
}
