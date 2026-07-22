#include "Services/Camera.h"

float CAngle::ClampTo2Pi(float angle) {
  const float twoPi = 2.0f * 3.14159265358979323846f;
  float       wrapped = angle - static_cast<int>(angle / twoPi) * twoPi;
  if (angle < 0.0f) {
    wrapped -= twoPi;
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

void __fastcall DataMgrGetCoord(HDATAMGR mgr, unsigned int fieldId, NTempest::C3Vector *coord) {
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

  *coord = static_cast<TManaged<NTempest::C3Vector> *>(managed)->m_data;
}

float __fastcall DataMgrGetFloat(HDATAMGR mgr, unsigned int fieldId) {
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

  return static_cast<TManaged<float> *>(managed)->m_data;
}

void __fastcall DataMgrSetCoord(HDATAMGR mgr, unsigned int fieldId, const NTempest::C3Vector &coord, unsigned int coordFlags) {
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

void __fastcall DataMgrSetFloat(HDATAMGR mgr, unsigned int fieldId, float val) {
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
