#include "ObjectMgrClient/ObjectMgrClient.h"

#include "Object/ObjectClient/GameObject_C.h"
#include "Object/ObjectClient/Unit_C.h"

#include "Tempest/c34matrix.h"
#include "Ui/GameUI.h"

static TSExplicitList<CGGameObject_C, 52> s_transports;

void __fastcall MovementLockMoversList(int forWriting) {
}

void __fastcall MovementUnlockMoversList(int fromWriting) {
}

void *__fastcall MovementTryLock(unsigned __int64) {
  return reinterpret_cast<void *>(1);
}

void __fastcall MovementUnlock(void *obj) {
}

int __fastcall MovementIsWorldServer() {
  return 0;
}

void __fastcall MovementNotifyZoneMgr(unsigned __int64 guid) {
}

void __fastcall MovementUpdateProxMap(void *obj) {
}

void __fastcall MovementFixOutOfBoundsUnit(unsigned __int64 guid) {
  if (guid == ClntObjMgrGetActivePlayer()) {
    CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
    if (unit && (unit->m_animFlags & 0x2000)) {
      CGGameUI::UpdateActivePlayer();
    }
  }
}

void __fastcall MovementAddTransport(CGGameObject_C *transport) {
  s_transports.LinkNode(transport, LIST_TAIL, 0);
}

void __fastcall MovementRemoveTransport(CGGameObject_C *transport) {
  s_transports.UnlinkNode(transport);
}

void __fastcall MovementMoveTransports(unsigned long eventTime, float elapsed) {
  for (CGGameObject_C *transport = s_transports.Head(); transport; transport = s_transports.RawNext(transport)) {
    transport->m_baseObj->UpdateMovement(eventTime, elapsed);
  }
}

void __fastcall MovementAddToTransport(CMovementData *mover, unsigned __int64 transportGUID) {
  CGGameObject_C *transport = static_cast<CGGameObject_C *>(ClntObjMgrObjectPtr(transportGUID, __FILE__, __LINE__));
  FATALASSERT(transport);
  transport->m_baseObj->AddPassenger(mover);
}

void __fastcall MovementFixUpMoveHistory(unsigned __int64 mover, const NTempest::C34Matrix &fixup) {
}

void __fastcall MovementUpdateCameraYaw(unsigned __int64 transportGUID) {
    // TODO: implement
}

void __fastcall MovementSetGlobals(void *ptr) {
  ClntObjMgrSetMovementGlobals(ptr);
}

void *__fastcall MovementGetGlobals() {
  return ClntObjMgrGetMovementGlobals();
}

void __fastcall MovementClearClobals() {
    // TODO: implement
}

int __fastcall MovementGameObjIsTransport(unsigned __int64 transportGUID) {
  CGGameObject_C *transport = static_cast<CGGameObject_C *>(ClntObjMgrObjectPtr(transportGUID, __FILE__, __LINE__));
  if (!transport) {
    return 0;
  }
  return transport->IsTransport();
}

void __fastcall MovementGetTransportMtx(unsigned __int64 transportGUID, NTempest::C34Matrix *transportMtx) {
  NTempest::C3Vector zAxis(0.0f, 0.0f, 1.0f);
  CGGameObject_C    *transport = static_cast<CGGameObject_C *>(ClntObjMgrObjectPtr(transportGUID, __FILE__, __LINE__));
  FATALASSERT(transport);

  transportMtx->Identity();
  transportMtx->Translate(transport->GetPosition());
  transportMtx->Rotate(transport->GetFacing(), zAxis, true);
}

float __fastcall MovementGetTransportFacing(unsigned __int64 transportGUID) {
  CGGameObject_C *transport = static_cast<CGGameObject_C *>(ClntObjMgrObjectPtr(transportGUID, __FILE__, __LINE__));
  FATALASSERT(transport);
  return transport->GetFacing();
}
int __fastcall MovementInsideTransport(unsigned __int64 transportGUID, const NTempest::C3Vector& position) {
    // TODO: implement
    return 0;
}
