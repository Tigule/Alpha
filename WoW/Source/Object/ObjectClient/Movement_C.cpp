#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

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

LPVOID MovementTryLock(DWORDLONG) {
  return reinterpret_cast<LPVOID>(1);
}

void MovementUnlock(LPVOID obj) {
}

int MovementIsWorldServer() {
  return 0;
}

void MovementNotifyZoneMgr(DWORDLONG guid) {
}

void MovementUpdateProxMap(LPVOID obj) {
}

void MovementFixOutOfBoundsUnit(DWORDLONG guid) {
  if (guid == ClntObjMgrGetActivePlayer()) {
    CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
    if (unit && unit->IsDeathFlagSet()) {
      CGGameUI::UpdateActivePlayer();
    }
  }
}

void MovementAddTransport(CGGameObject_C *transport) {
  FATALASSERT(!s_transports.IsLinked(transport));
  s_transports.LinkNode(transport, LIST_TAIL, 0);
}

void MovementRemoveTransport(CGGameObject_C *transport) {
  FATALASSERT(s_transports.IsLinked(transport));
  s_transports.UnlinkNode(transport);
}

void MovementMoveTransports(DWORD eventTime, float elapsed) {
  ITERATELIST(CGGameObject_C, s_transports, transport) {
    transport->m_baseObj->UpdateMovement(eventTime, elapsed);
  }
}

void MovementAddToTransport(CMovementData *mover, DWORDLONG transportGUID) {
  CGGameObject_C *transport = static_cast<CGGameObject_C *>(ClntObjMgrObjectPtr(transportGUID, __FILE__, __LINE__));
  FATALASSERT(transport);
  transport->m_baseObj->AddPassenger(mover);
}

void MovementFixUpMoveHistory(DWORDLONG mover, const NTempest::C34Matrix &fixup) {
}

void MovementUpdateCameraYaw(DWORDLONG transportGUID) {
  CGCamera *camera = CGWorldFrame::GetActiveCamera();
  FATALASSERT(camera);
  camera->MakeRelativeTo(transportGUID);
}

void MovementSetGlobals(LPVOID ptr) {
  ClntObjMgrSetMovementGlobals(ptr);
}

LPVOID MovementGetGlobals() {
  return ClntObjMgrGetMovementGlobals();
}

NTempest::C3Vector MovementGetTransportVector(DWORDLONG transportGUID) {
  CGObject_C *transport = ClntObjMgrObjectPtr(transportGUID, __FILE__, __LINE__);
  FATALASSERT(transport);
  return transport->GetPosition();
}

void MovementClearClobals() {
  ClntObjMgrSetMovementGlobals(0);
}

int MovementGameObjIsTransport(DWORDLONG transportGUID) {
  CGGameObject_C *transport = static_cast<CGGameObject_C *>(ClntObjMgrObjectPtr(transportGUID, __FILE__, __LINE__));
  if (!transport) {
    return 0;
  }
  return transport->IsTransport();
}

void MovementGetTransportMtx(DWORDLONG transportGUID, NTempest::C34Matrix *transportMtx) {
  NTempest::C3Vector zAxis(0.0f, 0.0f, 1.0f);
  CGGameObject_C    *transport = static_cast<CGGameObject_C *>(ClntObjMgrObjectPtr(transportGUID, __FILE__, __LINE__));
  FATALASSERT(transport);

  transportMtx->Identity();
  transportMtx->Translate(transport->GetPosition());
  transportMtx->Rotate(transport->GetFacing(), zAxis, true);
}

float MovementGetTransportFacing(DWORDLONG transportGUID) {
  CGGameObject_C *transport = static_cast<CGGameObject_C *>(ClntObjMgrObjectPtr(transportGUID, __FILE__, __LINE__));
  FATALASSERT(transport);
  return transport->GetFacing();
}
int MovementInsideTransport(DWORDLONG transportGUID, const NTempest::C3Vector &position) {
  CGGameObject_C *transport = static_cast<CGGameObject_C *>(ClntObjMgrObjectPtr(transportGUID, __FILE__, __LINE__));
  FATALASSERT(transport);
  return transport->IsPointInside(position);
}
