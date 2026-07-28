#include "ObjectMgrClient/ObjectMgrClient.h"

#include "Object/ObjectClient/GameObject_C.h"
#include "Object/ObjectClient/Unit_C.h"

#include "Tempest/c34matrix.h"
#include "UIUtil/Camera.h"
#include "Ui/GameUI.h"
#include "Ui/WorldFrame.h"

static LISTDECLEX(CGGameObject_C, moveLink, s_transports);

void MovementLockMoversList(int forWriting) {
}

void MovementUnlockMoversList(int fromWriting) {
}

void *MovementTryLock(unsigned __int64) {
  return reinterpret_cast<void *>(1);
}

void MovementUnlock(void *obj) {
}

int MovementIsWorldServer() {
  return 0;
}

void MovementNotifyZoneMgr(unsigned __int64 guid) {
}

void MovementUpdateProxMap(void *obj) {
}

void MovementFixOutOfBoundsUnit(unsigned __int64 guid) {
  if (guid == ClntObjMgrGetActivePlayer()) {
    CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
    if (unit && (unit->m_animFlags & 0x2000)) {
      CGGameUI::UpdateActivePlayer();
    }
  }
}

void MovementAddTransport(CGGameObject_C *transport) {
  s_transports.LinkNode(transport, LIST_TAIL, 0);
}

void MovementRemoveTransport(CGGameObject_C *transport) {
  s_transports.UnlinkNode(transport);
}

void MovementMoveTransports(unsigned long eventTime, float elapsed) {
  ITERATELIST(CGGameObject_C, s_transports, transport) {
    transport->m_baseObj->UpdateMovement(eventTime, elapsed);
  }
}

void MovementAddToTransport(CMovementData *mover, unsigned __int64 transportGUID) {
  CGGameObject_C *transport = static_cast<CGGameObject_C *>(ClntObjMgrObjectPtr(transportGUID, __FILE__, __LINE__));
  FATALASSERT(transport);
  transport->m_baseObj->AddPassenger(mover);
}

void MovementFixUpMoveHistory(unsigned __int64 mover, const NTempest::C34Matrix &fixup) {
}

void MovementUpdateCameraYaw(unsigned __int64 transportGUID) {
  CGCamera *camera = CGWorldFrame::GetActiveCamera();
  FATALASSERT(camera);
  camera->MakeRelativeTo(transportGUID);
}

void MovementSetGlobals(void *ptr) {
  ClntObjMgrSetMovementGlobals(ptr);
}

void *MovementGetGlobals() {
  return ClntObjMgrGetMovementGlobals();
}

NTempest::C3Vector MovementGetTransportVector(unsigned __int64 transportGUID) {
  CGObject_C *transport = ClntObjMgrObjectPtr(transportGUID, __FILE__, __LINE__);
  FATALASSERT(transport);
  return transport->GetPosition();
}

void MovementClearClobals() {
  ClntObjMgrSetMovementGlobals(0);
}

int MovementGameObjIsTransport(unsigned __int64 transportGUID) {
  CGGameObject_C *transport = static_cast<CGGameObject_C *>(ClntObjMgrObjectPtr(transportGUID, __FILE__, __LINE__));
  if (!transport) {
    return 0;
  }
  return transport->IsTransport();
}

void MovementGetTransportMtx(unsigned __int64 transportGUID, NTempest::C34Matrix *transportMtx) {
  NTempest::C3Vector zAxis(0.0f, 0.0f, 1.0f);
  CGGameObject_C    *transport = static_cast<CGGameObject_C *>(ClntObjMgrObjectPtr(transportGUID, __FILE__, __LINE__));
  FATALASSERT(transport);

  transportMtx->Identity();
  transportMtx->Translate(transport->GetPosition());
  transportMtx->Rotate(transport->GetFacing(), zAxis, true);
}

float MovementGetTransportFacing(unsigned __int64 transportGUID) {
  CGGameObject_C *transport = static_cast<CGGameObject_C *>(ClntObjMgrObjectPtr(transportGUID, __FILE__, __LINE__));
  FATALASSERT(transport);
  return transport->GetFacing();
}
int MovementInsideTransport(unsigned __int64 transportGUID, const NTempest::C3Vector& position) {
  CGGameObject_C *transport = static_cast<CGGameObject_C *>(ClntObjMgrObjectPtr(transportGUID, __FILE__, __LINE__));
  FATALASSERT(transport);
  return transport->IsPointInside(position);
}
