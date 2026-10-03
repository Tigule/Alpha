#include <Base/Base.h>
#include <Gx/Gx.h>
#include <WowConst.h>
#include <MapDefs.h>

#include "MovementData.h"

#include <storm.h>

#include "Tempest/c33matrix.h"
#include "Tempest/c34matrix.h"
#include "Tempest/cmath.h"
#include "Console/ConsoleClient.h"
#include "ObjectAlloc/ObjectAllocTemplate.h"
#include "Os/OsTime.h"
#include "Net/NetClient/NetClient.h"

#include <stdarg.h>
#include <stdio.h>
#include <float.h>
#include <math.h>

using NTempest::CMath;

void WVLog(UINT logMask, UINT priority, LPCSTR fmt, char *arglist);
void OnMoveUpdate(DWORDLONG unit, DWORD eventTime);
void OnCollideFalling(DWORDLONG unit, DWORD eventTime);
void UnitUpdateMovementAnim(const DWORDLONG &unit);

static UINT s_localMoveHeap = -1;

static void DisconnectLocalMover(CMovement *mover);

BOOL CMovementData::IsLocalPlayer() {
  return ((CMovementGlobals *)MovementGetGlobals())->m_localMover == this;
}

BOOL CMovement::MoversOnList() {
  CMovementGlobals *globals = (CMovementGlobals *)MovementGetGlobals();
  if (globals && globals->movers.Head()) {
    return 1;
  }
  return 0;
}

void CMovement::MoveUnit(DWORD timeNow, DWORD lastUpdate, LPVOID obj) {
  m_moveFlags &= ~0x00800000;

  if (int(timeNow - m_moveStartTime) < 0) {
    timeNow = m_moveStartTime;
  }

  if (m_moveFlags & 0x40FF) {
    ApplyMovement(lastUpdate + 1, FallTime(), timeNow - m_moveStartTime, timeNow - lastUpdate);
    m_moveFlags |= 0x00800000;
    OnMoveUpdate(GetGUID(), timeNow);

    if (m_moveFlags & 0x40FF) {
      if (CMath::isnan_(m_position.x) || CMath::isnan_(m_position.y) || CMath::isnan_(m_position.z)) {
        ConsolePrintf("Mover at invalid position");
        MovementFixOutOfBoundsUnit(GetGUID());
      } else {
        MovementUpdateProxMap(obj);
      }
    }
  }
}

void CMovement::MoveUnits(DWORD timeNow, DWORD lastUpdate) {
  CMovementGlobals *globals = (CMovementGlobals *)MovementGetGlobals();
  if (!globals) {
    return;
  }

  int entriesOnList = globals->movers.Head() != NULL;
  if (entriesOnList) {
    FallLogWrite("___IDLE EVENT: timeNow(0x%X)\n", timeNow);
  }

  for (CMovementData *baseMover = globals->movers.Head(), *baseMovernext_node;
       (int)baseMover > 0 ? (baseMovernext_node = globals->movers.RawNext(baseMover), 1) : 0; baseMover = baseMovernext_node) {
    CMovement *mover = static_cast<CMovement *>(baseMover);
    FATALASSERT(mover != ((CMovementGlobals *)MovementGetGlobals())->m_localMover);

    LPVOID obj = MovementTryLock(mover->GetGUID());
    if (obj) {
      mover->m_moveFlags &= ~0x00800000;

      DWORD moveStartTime = mover->m_moveStartTime;
      if (int(timeNow - moveStartTime) < 0) {
        timeNow = moveStartTime;
      }

      UINT moveTime = timeNow - moveStartTime;
      if (!(int(timeNow - moveStartTime) >= 0)) {
        mover ? SErrDisplayErrorFmt(
                    STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
                    "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
                    "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
                    "int(timeNow - moveStartTime) >= 0", mover->GetGUID(), mover->GetPosition().x, mover->GetPosition().y, mover->GetPosition().z,
                    mover->GetFacing(), static_cast<int>(mover->GetPosition().x), static_cast<int>(mover->GetPosition().y),
                    static_cast<int>(mover->GetPosition().z), static_cast<int>(mover->GetFacing())
                )
              : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "int(timeNow - moveStartTime) >= 0", FALSE, 1);
      }

      UINT elapsed = timeNow - lastUpdate;
      FallLogWrite(
          "0x%016I64X: last update(0x%08X) fall start time(0x%08X) current time(0x%08X) elapsed time(%u ms)\n", mover->GetGUID(), lastUpdate,
          mover->m_fallStartTime, timeNow, elapsed
      );

      if (mover->m_moveFlags & 0x40FF) {
        mover->ApplyMovement(lastUpdate + 1, mover->FallTime(), moveTime, elapsed);
        mover->m_moveFlags |= 0x00800000;
        OnMoveUpdate(mover->GetGUID(), timeNow);
        if (mover->m_moveFlags & 0x40FF) {
          if (CMath::isnan_(mover->m_position.x) || CMath::isnan_(mover->m_position.y) || CMath::isnan_(mover->m_position.z)) {
            ConsolePrintf("Mover at invalid position");
            MovementFixOutOfBoundsUnit(mover->GetGUID());
          } else {
            MovementUpdateProxMap(obj);
          }
          MovementUnlock(obj);
          continue;
        }
      } else {
        LogWrite("(%I64X) (%u) ERROR!!! Somehow we are in the mover list with a speed and turn rate both of zero\n", mover->GetGUID(), timeNow);
      }

      globals->movers.UnlinkNode(mover);
      --globals->numMovers;
      MovementUnlock(obj);
    }
  }

  if (entriesOnList) {
    FallLogWrite("___END IDLE EVENT\n");
  }
}

BOOL CMovement::UpdatePlayerMovement(DWORD timeNow) {
  while (((CMovementGlobals *)MovementGetGlobals())->m_localMoveQueue.HasEntries()) {
    CPlayerMoveEvent *event = ((CMovementGlobals *)MovementGetGlobals())->m_localMoveQueue.Root();
    if (int(timeNow - event->timeStamp) < 0) {
      break;
    }

    ((CMovementGlobals *)MovementGetGlobals())->m_localMoveQueue.Dequeue();

    switch (event->eventType) {
      case PMOVE_MOVE_START_FWD:
        if (StartMove(event->timeStamp, 1)) {
          ProcessLocalMoveEvent(MSG_MOVE_START_FORWARD);
        }
        break;
      case PMOVE_MOVE_START_BWD:
        if (StartMove(event->timeStamp, 0)) {
          ProcessLocalMoveEvent(MSG_MOVE_START_BACKWARD);
        }
        break;
      case PMOVE_MOVE_STOP:
        if (StopMove(event->timeStamp)) {
          ProcessLocalMoveEvent(MSG_MOVE_STOP);
        }
        break;
      case PMOVE_STRAFE_START_LFT:
        if (StartStrafe(event->timeStamp, 1)) {
          ProcessLocalMoveEvent(MSG_MOVE_START_STRAFE_LEFT);
        }
        break;
      case PMOVE_STRAFE_START_RGT:
        if (StartStrafe(event->timeStamp, 0)) {
          ProcessLocalMoveEvent(MSG_MOVE_START_STRAFE_RIGHT);
        }
        break;
      case PMOVE_STRAFE_STOP:
        if (StopStrafe(event->timeStamp)) {
          ProcessLocalMoveEvent(MSG_MOVE_STOP_STRAFE);
        }
        break;
      case PMOVE_FALL:
        StartFalling(event->timeStamp);
        OnCollideFalling(GetGUID(), event->timeStamp);
        break;
      case PMOVE_JUMP:
        if (Jump(event->timeStamp)) {
          ProcessLocalMoveEvent(MSG_MOVE_JUMP);
        }
        break;
      case PMOVE_TURN_START_LFT:
        StartTurn(event->timeStamp, 1);
        ProcessLocalMoveEvent(MSG_MOVE_START_TURN_LEFT);
        break;
      case PMOVE_TURN_START_RGT:
        StartTurn(event->timeStamp, 0);
        ProcessLocalMoveEvent(MSG_MOVE_START_TURN_RIGHT);
        break;
      case PMOVE_TURN_STOP:
        StopTurn(event->timeStamp);
        ProcessLocalMoveEvent(MSG_MOVE_STOP_TURN);
        break;
      case PMOVE_SET_RUN_MODE:
        SetRunMode(event->timeStamp, 1);
        ProcessLocalMoveEvent(MSG_MOVE_SET_RUN_MODE);
        break;
      case PMOVE_SET_WALK_MODE:
        SetRunMode(event->timeStamp, 0);
        ProcessLocalMoveEvent(MSG_MOVE_SET_WALK_MODE);
        break;
      case PMOVE_SET_FACING:
        SetFacing(event->timeStamp, event->facing);
        ProcessLocalMoveEvent(MSG_MOVE_SET_FACING);
        break;
      case PMOVE_SET_PITCH:
        SetPitch(event->timeStamp, event->facing);
        ProcessLocalMoveEvent(MSG_MOVE_SET_PITCH);
        break;
      case PMOVE_MOVE_START_SWIM:
        StartSwimLocal(event->timeStamp);
        ProcessLocalMoveEvent(MSG_MOVE_START_SWIM);
        break;
      case PMOVE_MOVE_STOP_SWIM:
        StopSwimLocal(event->timeStamp);
        ProcessLocalMoveEvent(MSG_MOVE_STOP_SWIM);
        break;
      case PMOVE_PITCH_START_UP:
        StartPitch(event->timeStamp, 1);
        ProcessLocalMoveEvent(MSG_MOVE_START_PITCH_UP);
        break;
      case PMOVE_PITCH_START_DOWN:
        StartPitch(event->timeStamp, 0);
        ProcessLocalMoveEvent(MSG_MOVE_START_PITCH_DOWN);
        break;
      case PMOVE_PITCH_STOP:
        StopPitch(event->timeStamp);
        ProcessLocalMoveEvent(MSG_MOVE_STOP_PITCH);
        break;
    }

    event->~CPlayerMoveEvent();
    ObjectFree(event->memHandle);
  }

  return ((CMovementGlobals *)MovementGetGlobals())->m_localMoveQueue.HasEntries() || (m_moveFlags & 0x40FF);
}

void CMovement::MoveLocalPlayer(DWORD timeNow, DWORD lastUpdate) {
  UINT elapsedMS = timeNow - lastUpdate;

  LogWrite("0x%016I64X: current time(0x%08X) last update(0x%08X) time elapsed(%u)\n", GetGUID(), timeNow, lastUpdate, elapsedMS);
  if (!(elapsedMS > 0)) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "elapsedMS > 0", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "elapsedMS > 0", FALSE, 1);
  }

  UINT timeUsed = 0;
  m_moveFlags &= ~0x00800000;

  while (timeUsed < elapsedMS) {
    DWORD updateTime = lastUpdate + timeUsed;
    UINT  timeToUse;

    if (((CMovementGlobals *)MovementGetGlobals())->m_localMoveQueue.HasEntries()) {
      CPlayerMoveEvent *event = ((CMovementGlobals *)MovementGetGlobals())->m_localMoveQueue.Root();
      timeToUse = min(event->timeStamp - updateTime, elapsedMS - timeUsed);
      LogWrite("0x%016I64X: first event time(0x%08X), time to consume(%u)\n", GetGUID(), event->timeStamp, timeToUse);
    } else {
      timeToUse = elapsedMS - timeUsed;
      LogWrite("0x%016I64X: no events, consuming(%u)\n", GetGUID(), timeToUse);
    }

    LogWrite(
        "0x%016I64X: update time(0x%08X) last update(0x%08X) move start(0x%08X) fall start (0x%08X) time used(%u)\n", GetGUID(), updateTime,
        lastUpdate, m_moveStartTime, m_fallStartTime, timeUsed
    );

    if ((m_moveFlags & 0x40FF) && timeToUse > 0) {
      ApplyMovement(updateTime + 1, m_moveFlags & 0x4000 ? updateTime - m_fallStartTime : 0, timeToUse - m_moveStartTime + updateTime, timeToUse);
      m_moveFlags |= 0x00800000;
    }

    if (!UpdatePlayerMovement(updateTime + 1)) {
      LogWrite("0x%016I64X: No more movement and player not in motion\n", GetGUID());
      ((CMovementGlobals *)MovementGetGlobals())->m_localMover = NULL;
      break;
    }

    timeUsed += timeToUse;
  }

  OnMoveUpdate(GetGUID(), timeNow);
}

BOOL MovementIdleMoveUnits(LPCVOID packetData, LPVOID param) {
  MovementLockMoversList(1);

  DWORD timeNow = OsGetAsyncTimeMs();
  DWORD lastUpdate = ((CMovementGlobals *)MovementGetGlobals())->m_lastUpdateTime;
  int   timeDiff = timeNow - lastUpdate;

  MovementMoveTransports(timeNow, timeDiff * 0.001f);

  if (!((CMovementGlobals *)MovementGetGlobals())->m_localMover && !CMovement::MoversOnList()) {
    MovementUnlockMoversList(1);
    ((CMovementGlobals *)MovementGetGlobals())->m_lastUpdateTime = timeNow;
    return 1;
  }

  FATALASSERT(timeDiff >= 0);

  if (timeDiff) {
    do {
    lastUpdate = ((CMovementGlobals *)MovementGetGlobals())->m_lastUpdateTime;

    if (timeDiff > 1000) {
      ((CMovementGlobals *)MovementGetGlobals())->m_lastUpdateTime = lastUpdate + 1000;
      timeDiff -= 1000;
    } else {
      ((CMovementGlobals *)MovementGetGlobals())->m_lastUpdateTime = timeNow;
      timeDiff = 0;
      CMovement::MoveUnits(timeNow, lastUpdate);
    }

    if (((CMovementGlobals *)MovementGetGlobals())->m_localMover) {
      ((CMovementGlobals *)MovementGetGlobals())
          ->m_localMover->MoveLocalPlayer(((CMovementGlobals *)MovementGetGlobals())->m_lastUpdateTime, lastUpdate);
    }
    } while (timeDiff > 0);
  }

  MovementUnlockMoversList(1);
  return 1;
}

void MovementInitialize(LPCSTR logFileName, bool needLocalHeap) {
  MovementSetGlobals(NEW(CMovementGlobals));
  CMovementGlobals *globals = (CMovementGlobals *)MovementGetGlobals();
  FATALASSERT(globals);

  SStrCopy(globals->logFileName, logFileName, sizeof(globals->logFileName));
  ((CMovementGlobals *)MovementGetGlobals())->m_localMover = NULL;

  if (needLocalHeap && s_localMoveHeap == 0xffffffff) {
    s_localMoveHeap = ObjectAllocAddHeap(sizeof(CPlayerMoveEvent), 1024, "CPlayerMoveEvent");
  }

  ((CMovementGlobals *)MovementGetGlobals())->m_lastUpdateTime = OsGetAsyncTimeMs();
}

void MovementDestroy() {
  CMovement::StopAllLogging();
  CMovementGlobals *globals = (CMovementGlobals *)MovementGetGlobals();
  globals->m_localMoveQueue.DiscardAll();
  DEL(globals);
  MovementSetGlobals(NULL);
}

int MovementGetNumMovers() {
  CMovementGlobals *globals = (CMovementGlobals *)MovementGetGlobals();
  return globals ? globals->numMovers : 0;
}

DWORD MovementGetLastUpdate() {
  CMovementGlobals *globals = (CMovementGlobals *)MovementGetGlobals();
  return globals ? globals->m_lastUpdateTime : 0;
}

void CMovement::GetMovingDirection(NTempest::C3Vector *direction) const {
  *direction = m_moveFlags & 2 ? -m_direction : m_direction;
}

void CMovement::GetStrafingDirection(NTempest::C3Vector *direction) const {
  direction->Set(m_direction.y, m_direction.x, 0.0f);
  if (m_moveFlags & 4) {
    direction->x = -direction->x;
  } else {
    direction->y = -direction->y;
  }
}

void CMovement::GetDiagonalDirection(NTempest::C3Vector *direction) const {
  NTempest::C3Vector moveDir;
  GetMovingDirection(&moveDir);
  GetStrafingDirection(direction);
  *direction += moveDir;
  direction->x *= 0.70710677f;
  direction->y *= 0.70710677f;
}

void CMovement::GetMovingDirection2d(NTempest::C2Vector *direction) const {
  *direction = m_moveFlags & 2 ? -m_direction2d : m_direction2d;
}

void CMovement::GetStrafingDirection2d(NTempest::C2Vector *direction) const {
  direction->Set(m_direction2d.y, m_direction2d.x);
  if (m_moveFlags & 4) {
    direction->x = -direction->x;
  } else {
    direction->y = -direction->y;
  }
}

void CMovement::GetDiagonalDirection2d(NTempest::C2Vector *direction) const {
  NTempest::C2Vector moveDir;
  GetMovingDirection2d(&moveDir);
  GetStrafingDirection2d(direction);
  direction->x = (direction->x + moveDir.x) * 0.70710677f;
  direction->y = (direction->y + moveDir.y) * 0.70710677f;
}

void CMovement::GetDirection(NTempest::C3Vector *direction) const {
  if ((m_moveFlags & 0xF) == 0xF) {
    GetDiagonalDirection(direction);
  } else if (IsStrafing()) {
    GetStrafingDirection(direction);
  } else {
    GetMovingDirection(direction);
  }
}

void CMovement::PlotLinearPosition(const NTempest::C3Vector &direction, float secsElapsed, NTempest::C3Vector *totalMove) {
  *totalMove = (direction * secsElapsed) * GetCurrentSpeed();
  LogWrite("0x%016I64X: Want to move (%g,%g,%g) straight for %g secs\n", m_guid, totalMove->x, totalMove->y, totalMove->z, secsElapsed);
}

void CMovement::PlotNormalLinearPosition(float secsElapsed, NTempest::C3Vector *totalMove) {
  if (!(IsMoving())) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "IsMoving()", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "IsMoving()", FALSE, 1);
  }

  NTempest::C3Vector direction;
  GetMovingDirection(&direction);
  PlotLinearPosition(direction, secsElapsed, totalMove);
}

void CMovement::PlotStrafeLinearPosition(float secsElapsed, NTempest::C3Vector *totalMove) {
  if (!(IsStrafing())) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "IsStrafing()", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "IsStrafing()", FALSE, 1);
  }

  NTempest::C3Vector direction;
  GetStrafingDirection(&direction);
  PlotLinearPosition(direction, secsElapsed, totalMove);
}

void CMovement::PlotDiagonalLinearPosition(float secsElapsed, NTempest::C3Vector *totalMove) {
  if (!(IsMoving())) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "IsMoving()", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "IsMoving()", FALSE, 1);
  }
  if (!(IsStrafing())) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "IsStrafing()", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "IsStrafing()", FALSE, 1);
  }

  NTempest::C3Vector direction;
  GetDiagonalDirection(&direction);
  PlotLinearPosition(direction, secsElapsed, totalMove);
}

void CMovement::PlotUnitRotation(float elapsedSec) {
  if (!(IsTurning())) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "IsTurning()", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "IsTurning()", FALSE, 1);
  }

  float turnRate = GetCurrentTurnRate();
  float facing = CMath::fmod_(turnRate * elapsedSec + m_anchorFacing, 6.2831855f);
  if (facing < 0.0f) {
    facing += 6.2831855f;
  }
  LogWrite("0x%016I64X: Turning for %g secs, from (%g) to (%g)\n", GetGUID(), elapsedSec, m_facing, facing);
  m_facing = facing;
}

void CMovement::PlotUnitPitch(float elapsedSec) {
  if (!(IsPitching())) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "IsPitching()", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "IsPitching()", FALSE, 1);
  }

  float pitch = CMath::clamp_(GetCurrentPitchRate() * elapsedSec + m_anchorPitch, -1.5707964f, 1.5707964f);
  LogWrite("0x%016I64X: Pitching for %g secs, from (%g) to (%g)\n", GetGUID(), elapsedSec, m_pitch, pitch);
  m_pitch = pitch;
}

void CMovement::PlotSpiralPosition(const NTempest::C2Vector &direction2d, float secsElapsed, NTempest::C3Vector *totalMove) {
  float currentSpeed = GetCurrentSpeed();
  float turnRate = GetCurrentTurnRate();
  float pitchRate = GetCurrentPitchRate();
  float turnRadius = currentSpeed / turnRate;
  float inversePitchRate = 1.0f / pitchRate;
  float pitchRadius = inversePitchRate * currentSpeed;
  float pitchChange = pitchRate * secsElapsed;
  float overflowTime;

  if (pitchChange + m_anchorPitch > 1.5707964f) {
    overflowTime = (pitchChange + m_anchorPitch - 1.5707964f) * inversePitchRate;
    pitchChange = 1.5707964f - m_anchorPitch;
  } else if (pitchChange + m_anchorPitch < -1.5707964f) {
    overflowTime = (pitchChange + m_anchorPitch + 1.5707964f) * inversePitchRate;
    pitchChange = -1.5707964f - m_anchorPitch;
  } else {
    overflowTime = 0.0f;
  }

  float remainingTurn = (secsElapsed - overflowTime) * turnRate;

  NTempest::C3Vector baseMove(
      (CMath::sin_(pitchChange) * pitchRadius + CMath::sin_(remainingTurn) * turnRadius) * 0.5f, CMath::cos_(remainingTurn) * turnRadius - turnRadius,
      pitchRadius - CMath::cos_(pitchChange) * pitchRadius
  );

  NTempest::C3Vector yawed(
      baseMove.x * direction2d.x - baseMove.y * direction2d.y, baseMove.y * direction2d.x + baseMove.x * direction2d.y, baseMove.z
  );

  totalMove->x = yawed.x * m_cosAnchorPitch - yawed.z * m_sinAnchorPitch;
  totalMove->y = yawed.y;
  totalMove->z = yawed.z * m_cosAnchorPitch + yawed.x * m_sinAnchorPitch;

  if (CMath::fnotequal4_(overflowTime, 0)) {
    if (CMath::fequal4_(pitchChange + m_anchorPitch, 1.5707964f)) {
      totalMove->z += overflowTime * currentSpeed;
    } else {
      totalMove->z -= overflowTime * currentSpeed;
    }
  }
}

void CMovement::PlotVertCircularPosition(const NTempest::C2Vector &direction2d, float secsElapsed, NTempest::C3Vector *totalMove) {
  float currentSpeed = GetCurrentSpeed();
  float pitchRate = GetCurrentPitchRate();
  float inversePitchRate = 1.0f / pitchRate;
  float pitchRadius = inversePitchRate * currentSpeed;
  float pitchAngle = pitchRate * secsElapsed;
  float overflowTime;

  if (pitchAngle + m_anchorPitch > 1.5707964f) {
    overflowTime = (pitchAngle + m_anchorPitch - 1.5707964f) * inversePitchRate;
    pitchAngle = 1.5707964f - m_anchorPitch;
  } else if (pitchAngle + m_anchorPitch < -1.5707964f) {
    overflowTime = (pitchAngle + m_anchorPitch + 1.5707964f) * inversePitchRate;
    pitchAngle = -1.5707964f - m_anchorPitch;
  } else {
    overflowTime = 0.0f;
  }

  float horizontal = CMath::sin_(pitchAngle) * pitchRadius;
  float vertical = pitchRadius - CMath::cos_(pitchAngle) * pitchRadius;

  totalMove->x = (horizontal * m_cosAnchorPitch - vertical * m_sinAnchorPitch) * direction2d.x;
  totalMove->y = (horizontal * m_cosAnchorPitch - vertical * m_sinAnchorPitch) * direction2d.y;
  totalMove->z = vertical * m_cosAnchorPitch + horizontal * m_sinAnchorPitch;

  if (CMath::fnotequal4_(overflowTime, 0)) {
    if (CMath::fequal4_(pitchAngle + m_anchorPitch, 1.5707964f)) {
      totalMove->z += overflowTime * currentSpeed;
    } else {
      totalMove->z -= overflowTime * currentSpeed;
    }
  }

  LogWrite("0x%016I64X: Want to move (%g,%g,%g) arcing for %g secs\n", GetGUID(), totalMove->x, totalMove->y, totalMove->z, secsElapsed);
}

void CMovement::PlotHorzCircularPosition(const NTempest::C2Vector &direction2d, float secsElapsed, NTempest::C3Vector *totalMove) {
  float currentSpeed = GetCurrentSpeed();
  float turnRate = GetCurrentTurnRate();
  float turnRadius = currentSpeed / turnRate;
  float turnX = CMath::sin_(turnRate * secsElapsed) * turnRadius;
  float turnY = turnRadius - CMath::cos_(turnRate * secsElapsed) * turnRadius;

  totalMove->x = (turnX * direction2d.x - turnY * direction2d.y) * m_cosAnchorPitch;
  totalMove->y = (turnY * direction2d.x + turnX * direction2d.y) * m_cosAnchorPitch;
  totalMove->z = currentSpeed * m_sinAnchorPitch * secsElapsed;
  LogWrite("0x%016I64X: Want to move (%g,%g,%g) arcing for %g secs\n", GetGUID(), totalMove->x, totalMove->y, totalMove->z, secsElapsed);
}

void CMovement::PlotNormalCircularPosition(float secsElapsed, NTempest::C3Vector *totalMove) {
  if (!(IsTurning())) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "IsTurning()", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "IsTurning()", FALSE, 1);
  }
  if (!(IsMoving())) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "IsMoving()", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "IsMoving()", FALSE, 1);
  }

  NTempest::C2Vector direction;
  GetMovingDirection2d(&direction);
  PlotHorzCircularPosition(direction, secsElapsed, totalMove);
}

void CMovement::PlotStrafeCircularPosition(float secsElapsed, NTempest::C3Vector *totalMove) {
  if (!(IsTurning())) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "IsTurning()", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "IsTurning()", FALSE, 1);
  }
  if (!(IsStrafing())) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "IsStrafing()", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "IsStrafing()", FALSE, 1);
  }

  NTempest::C2Vector direction;
  GetStrafingDirection2d(&direction);
  PlotHorzCircularPosition(direction, secsElapsed, totalMove);
}

void CMovement::PlotDiagonalCircularPosition(float secsElapsed, NTempest::C3Vector *totalMove) {
  if (!(IsTurning())) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "IsTurning()", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "IsTurning()", FALSE, 1);
  }
  if (!(IsMoving())) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "IsMoving()", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "IsMoving()", FALSE, 1);
  }
  if (!(IsStrafing())) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "IsStrafing()", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "IsStrafing()", FALSE, 1);
  }

  NTempest::C2Vector direction;
  GetDiagonalDirection2d(&direction);
  PlotHorzCircularPosition(direction, secsElapsed, totalMove);
}

void CMovement::PlotNormalPitchingCircularPosition(float secsElapsed, NTempest::C3Vector *totalMove) {
  if (!(IsMoving())) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "IsMoving()", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "IsMoving()", FALSE, 1);
  }
  if (!(IsPitching())) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "IsPitching()", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "IsPitching()", FALSE, 1);
  }

  NTempest::C2Vector direction;
  GetMovingDirection2d(&direction);
  PlotVertCircularPosition(direction, secsElapsed, totalMove);
}

void CMovement::PlotDiagonalPitchingCircularPosition(float secsElapsed, NTempest::C3Vector *totalMove) {
  if (!(IsMoving())) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "IsMoving()", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "IsMoving()", FALSE, 1);
  }
  if (!(IsPitching())) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "IsPitching()", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "IsPitching()", FALSE, 1);
  }

  NTempest::C2Vector direction;
  GetDiagonalDirection2d(&direction);
  PlotVertCircularPosition(direction, secsElapsed, totalMove);
}

void CMovement::PlotNormalSpiralPosition(float secsElapsed, NTempest::C3Vector *totalMove) {
  if (!(IsMoving())) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "IsMoving()", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "IsMoving()", FALSE, 1);
  }
  if (!(IsPitching())) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "IsPitching()", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "IsPitching()", FALSE, 1);
  }

  NTempest::C2Vector direction;
  GetMovingDirection2d(&direction);
  PlotSpiralPosition(direction, secsElapsed, totalMove);
}

void CMovement::PlotDiagonalSpiralPosition(float secsElapsed, NTempest::C3Vector *totalMove) {
  if (!(IsMoving())) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "IsMoving()", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "IsMoving()", FALSE, 1);
  }
  if (!(IsPitching())) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "IsPitching()", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "IsPitching()", FALSE, 1);
  }

  NTempest::C2Vector direction;
  GetDiagonalDirection2d(&direction);
  PlotSpiralPosition(direction, secsElapsed, totalMove);
}

BOOL CMovement::PlotUnitMovement(UINT moveTime, NTempest::C3Vector *move) {
  if (!move) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "move", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "move", FALSE, 1);
  }
  if (!moveTime) {
    return 0;
  }

  enum {
    IS_MOVING = 1,
    IS_TURNING = 2,
    IS_STRAFING = 4,
    IS_PITCHING = 8
  };

  float secsElapsed = static_cast<float>(moveTime) * 0.001f;
  UINT  actionFlags = 0;
  if (m_moveFlags & 0x30) {
    PlotUnitRotation(secsElapsed);
    if (!(m_moveFlags & 0x4000)) {
      actionFlags |= IS_TURNING;
    }
  }
  if (m_moveFlags & 0xC0) {
    PlotUnitPitch(secsElapsed);
    actionFlags |= IS_PITCHING;
  }
  if (m_moveFlags & 3) {
    actionFlags |= IS_MOVING;
  }
  if (m_moveFlags & 0xC) {
    actionFlags |= IS_STRAFING;
  }

  LogWrite("0x%016I64X: Plotting move for %u ms\n", m_guid, moveTime);

  int result;
  switch (actionFlags) {
    case IS_TURNING:
    case IS_PITCHING:
    case IS_TURNING | IS_PITCHING:
      result = 0;
      break;
    case IS_MOVING:
      PlotNormalLinearPosition(secsElapsed, move);
      result = 1;
      break;
    case IS_STRAFING:
    case IS_STRAFING | IS_PITCHING:
      PlotStrafeLinearPosition(secsElapsed, move);
      result = 1;
      break;
    case IS_MOVING | IS_STRAFING:
      PlotDiagonalLinearPosition(secsElapsed, move);
      result = 1;
      break;
    case IS_MOVING | IS_TURNING:
      PlotNormalCircularPosition(secsElapsed, move);
      result = 1;
      break;
    case IS_TURNING | IS_STRAFING:
    case IS_TURNING | IS_STRAFING | IS_PITCHING:
      PlotStrafeCircularPosition(secsElapsed, move);
      result = 1;
      break;
    case IS_MOVING | IS_TURNING | IS_STRAFING:
      PlotDiagonalCircularPosition(secsElapsed, move);
      result = 1;
      break;
    case IS_MOVING | IS_PITCHING:
      PlotNormalPitchingCircularPosition(secsElapsed, move);
      result = 1;
      break;
    case IS_MOVING | IS_STRAFING | IS_PITCHING:
      PlotDiagonalPitchingCircularPosition(secsElapsed, move);
      result = 1;
      break;
    case IS_MOVING | IS_TURNING | IS_PITCHING:
      PlotNormalSpiralPosition(secsElapsed, move);
      result = 1;
      break;
    case IS_MOVING | IS_TURNING | IS_STRAFING | IS_PITCHING:
      PlotDiagonalSpiralPosition(secsElapsed, move);
    default:
      result = 1;
      break;
  }
  return result;
}

BOOL CMovement::PlotUnitSplineMovement(DWORD eventTime, NTempest::C3Vector *move) {
  if (!(move)) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "move", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "move", FALSE, 1);
  }
  if (!(IsSplineMover())) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "IsSplineMover()", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "IsSplineMover()", FALSE, 1);
  }

  if (!IsMoving()) {
    return 0;
  }

  if (!(m_spline->flags & 0x80000000)) {
    int   timeUsed = eventTime - m_spline->start;
    float time;
    if (m_spline->time <= 0) {
      time = 1.0f;
    } else if (timeUsed < 0) {
      time = 0.0f;
    } else if (static_cast<UINT>(timeUsed) >= m_spline->time) {
      m_spline->flags |= 1;
      time = 1.0f;
    } else {
      time = static_cast<float>(timeUsed) / m_spline->time;
    }

    NTempest::C34Matrix matrix;
    *matrix.Row0AsVec3() = m_direction;
    m_spline->spline.Frame(time, matrix, NTempest::C3Spline::EVAL_ARCLENGTH);

    m_facing = CMath::atan2_(matrix.a1, matrix.a0);
    if (m_facing < 0.0f) {
      m_facing += 6.2831855f;
    }
    m_direction = *matrix.Row0AsVec3();

    if (m_spline->flags & 0x200) {
      NTempest::C34Matrix futureMatrix;
      *futureMatrix.Row0AsVec3() = m_direction;

      float futureTime = CMath::clamp_(static_cast<float>(timeUsed + 1000) / m_spline->time, 0.0f, 1.0f);
      m_spline->spline.Frame(futureTime, futureMatrix, NTempest::C3Spline::EVAL_ARCLENGTH);

      NTempest::C2Vector currentFacing;
      currentFacing.x = m_direction.x;
      currentFacing.y = m_direction.y;
      NTempest::C2Vector futureFacing = *futureMatrix.Row3AsVec3() - GetPosition();
      currentFacing.SafeNormalize();
      futureFacing.SafeNormalize();

      float cosTheta = NTempest::C2Vector::Dot(futureFacing, currentFacing);
      cosTheta = CMath::clamp_(cosTheta, -1.0f, 1.0f);

      float roll;
      if (NTempest::C2Vector::Cross(currentFacing, futureFacing) < 0.0f) {
        roll = CMath::acos_(cosTheta);
      } else {
        roll = -CMath::acos_(cosTheta);
      }
      roll = CMath::clamp_(roll * 2.0f, -1.5707964f, 1.5707964f);

      NTempest::C33Matrix rollMatrix = NTempest::C33Matrix::Rotation(roll, m_direction, 0);
      m_groundNormal = *matrix.Row2AsVec3() * rollMatrix;
    }

    *move = *matrix.Row3AsVec3() - m_anchorPosition;
  }
  return 1;
}

BOOL CMovement::CheckInvalidPositionOrMove(const NTempest::C3Vector &move, UINT moveTime) {
  if (!(CMath::isnan_(m_position.x) || CMath::isnan_(m_position.y) || m_position.x - 2.0f < -17066.666f || m_position.x + 2.0f > 17066.666f ||
        m_position.y - 2.0f < -17066.666f || m_position.y + 2.0f > 17066.666f))
  {
    NTempest::C3Vector moveVector = m_anchorPosition + move - m_position;
    float              distanceSq;
    if (m_moveFlags & 0x02000000) {
      distanceSq = moveVector.x * moveVector.x + moveVector.y * moveVector.y + moveVector.z * moveVector.z;
    } else {
      distanceSq = moveVector.x * moveVector.x + moveVector.y * moveVector.y;
    }

    if (!moveTime) {
      if (distanceSq < 0.00000095367432f) {
        return 1;
      }
      BothLogWrite(
          "0x%016I64X: Tried moving bogus distance (%g) in zero time, from (%g,%g,%g), teleporting\n", GetGUID(), CMath::sqrt_(distanceSq),
          m_position.x, m_position.y, m_position.z
      );
    } else {
      float seconds = moveTime * 0.001f;
      if (distanceSq / (seconds * seconds) > 3600.0f) {
        BothLogWrite(
          "0x%016I64X: Tried moving bogus speed (%g), from (%g,%g,%g), teleporting\n", GetGUID(), CMath::sqrt_(distanceSq) / seconds,
            m_position.x, m_position.y, m_position.z
        );
      } else {
        return 1;
      }
    }
  } else {
    BothLogWrite("0x%016I64X: Bogus position (%g,%g,%g), teleporting\n", GetGUID(), m_position.x, m_position.y, m_position.z);
  }

  MovementFixOutOfBoundsUnit(GetGUID());
  return 0;
}

void CMovement::ApplyAdjustedMove(DWORD timeStemp, const NTempest::C3Vector &moveWanted, BOOL wasAdjusted, UINT oldMoveFlags) {
  if (wasAdjusted || (m_moveFlags & 0x1000)) {
    if (IsSpline() && !(oldMoveFlags & 0x1000)) {
      m_spline->flags |= 2;
      m_direction = moveWanted;
      float mag = m_direction.Mag();
      if (CMath::fnotequal_(mag, 0)) {
        m_direction /= mag;
      }
    }
    UpdateAnchors(timeStemp);
  } else if (IsSpline()) {
    m_spline->flags &= ~2;
  }
}

void CMovement::SimpleRequestMove(UINT fallTime, const NTempest::C3Vector &moveVector) {
  m_position = m_anchorPosition + moveVector;
  if (m_jumpVelocity != 0.0f) {
    float seconds = fallTime * 0.001f;
    float distJumped = seconds * m_jumpVelocity * -0.5f;
    if (distJumped > 1.0f) {
      distJumped = 1.0f;
      StopFalling();
    }
    m_position.z = m_fallStartElevation + distJumped;
  }
}

void CMovement::ApplyMovement(DWORD eventTime, UINT fallTime, UINT moveTime, UINT elapsed) {
  NTempest::C3Vector move;
  if (m_spline && !(m_spline->flags & 6)) {
    PlotUnitSplineMovement(eventTime + elapsed, &move);
  } else {
    PlotUnitMovement(moveTime, &move);
  }

  if (!(m_moveFlags & 0x400F)) {
    return;
  }
  if (!CheckInvalidPositionOrMove(move, moveTime)) {
    return;
  }

  CMovementGlobals *globals = (CMovementGlobals *)MovementGetGlobals();
  FATALASSERT(globals);
  UINT oldMoveFlags = m_moveFlags;
  BOOL moveAdjusted = 0;
  BOOL wasJumping = m_jumpVelocity != 0.0f;

  if (globals->ignoreObstacles) {
    m_position = move + m_anchorPosition;
  } else if ((oldMoveFlags & 0x800) || (IsSpline() && (m_spline->flags & 0x200))) {
    SimpleRequestMove(fallTime + elapsed, move);
  } else if (oldMoveFlags & 0x400F) {
    m_moveFlags &= ~0x10000000;
    NTempest::C3Vector moveVector = m_anchorPosition + move - m_position;
    if (!(m_moveFlags & 0x02000000)) {
      moveVector.z = 0.0f;
    }
    moveAdjusted = CollideRequestMove(eventTime, elapsed, moveVector);
    ApplyAdjustedMove(eventTime + elapsed, move, moveAdjusted, oldMoveFlags);
  }

  CallMoveEventHandlers(eventTime, moveAdjusted, oldMoveFlags, wasJumping);
  LogWrite(
      "0x%016I64X: Moving for %u ms, (%g,%g) from (%g,%g) to (%g,%g), falling %u ms, %g from %g to %g\n", GetGUID(), moveTime,
      m_position.x - m_anchorPosition.x, m_position.y - m_anchorPosition.y, m_anchorPosition.x, m_anchorPosition.y, m_position.x, m_position.y,
      fallTime, m_fallStartElevation - m_position.z, m_fallStartElevation, m_position.z
  );
  if (IsSpline() && (m_spline->flags & 0x200)) {
    UpdateAnchors(eventTime + elapsed);
  }
  if (m_position.z < -333.33334f) {
    MovementFixOutOfBoundsUnit(GetGUID());
  }
}

CMovementData::CMovementData(const NTempest::C3Vector &position, float facing, const DWORDLONG &guid)
    : m_position(position),
      m_facing(facing),
      m_pitch(0.0f),
      m_groundNormal(0.0f, 0.0f, 1.0f),
      m_guid(guid),
      m_transportGUID(0),
      m_moveFlags(0x08000000),
      m_anchorPosition(position),
      m_anchorFacing(facing),
      m_anchorPitch(0.0f),
      m_direction(0.0f),
      m_direction2d(0.0f),
      m_cosAnchorPitch(1.0f),
      m_sinAnchorPitch(0.0f),
      m_reDirection(0.0f),
      m_lastReDirectionSent(0.0f),
      m_currentSpeed(0.0f),
      m_walkSpeed(0.0f),
      m_runSpeed(0.0f),
      m_swimSpeed(0.0f),
      m_turnRate(0.0f),
      m_collisionBoxHalfDepth(0.33333334f),
      m_collisionBoxHeight(2.0277777f),
      m_stepUpHeight(1.0f),
      m_jumpVelocity(0.0f),
      m_spline(0) {
  CalcDirection();
}

CMovementData::~CMovementData() {
  if (MovementGetGlobals() && static_cast<CMovementGlobals *>(MovementGetGlobals())->m_localMover == this) {
    static_cast<CMovementGlobals *>(MovementGetGlobals())->m_localMover = 0;
    FATALASSERT(!m_spline);
    FATALASSERT(!IsSplineMover());
  } else {
    RemoveSpline();
  }
}

NTempest::C3Vector CMovementData::GetPosition(const NTempest::C3Vector &position) const {
  if (!m_transportGUID) {
    return position;
  }

  NTempest::C34Matrix transportMtx;
  MovementGetTransportMtx(m_transportGUID, &transportMtx);
  return position * transportMtx;
}

float CMovementData::GetFacing(float facing) const {
  if (!m_transportGUID) {
    return facing;
  }

  facing += MovementGetTransportFacing(m_transportGUID);
  if (facing > 6.2831855f) {
    facing -= 6.2831855f;
  } else if (facing < 0.0f) {
    facing += 6.2831855f;
  }

  return facing;
}

BOOL CMovementData::ForceSetTransport(DWORDLONG guid) {
  if (guid == m_transportGUID || (guid && !MovementGameObjIsTransport(guid))) {
    return 0;
  }

  NTempest::C34Matrix transportMtx;
  float               transportFacing;
  if (guid) {
    MovementGetTransportMtx(guid, &transportMtx);
    transportFacing = MovementGetTransportFacing(guid);
    transportMtx = transportMtx.AffineInverse();
    m_position *= transportMtx;
    m_anchorPosition *= transportMtx;
    float facing = m_facing - transportFacing;
    if (facing > 6.2831855f) {
      facing -= 6.2831855f;
    } else if (facing < 0.0f) {
      facing += 6.2831855f;
    }
    m_facing = facing;
    facing = m_anchorFacing - transportFacing;
    if (facing > 6.2831855f) {
      facing -= 6.2831855f;
    } else if (facing < 0.0f) {
      facing += 6.2831855f;
    }
    m_anchorFacing = facing;
    m_fallStartElevation = transportMtx.c2 * m_fallStartElevation + transportMtx.d2;
    MovementAddToTransport(this, guid);
  } else {
    MovementGetTransportMtx(m_transportGUID, &transportMtx);
    transportFacing = MovementGetTransportFacing(m_transportGUID);
    m_position *= transportMtx;
    m_anchorPosition *= transportMtx;
    float facing = transportFacing + m_facing;
    if (facing > 6.2831855f) {
      facing -= 6.2831855f;
    } else if (facing < 0.0f) {
      facing += 6.2831855f;
    }
    m_facing = facing;
    facing = transportFacing + m_anchorFacing;
    if (facing > 6.2831855f) {
      facing -= 6.2831855f;
    } else if (facing < 0.0f) {
      facing += 6.2831855f;
    }
    m_anchorFacing = facing;
    m_fallStartElevation = transportMtx.c2 * m_fallStartElevation + transportMtx.d2;
    transportLink.Unlink();
  }

  MovementFixUpMoveHistory(GetGUID(), transportMtx);
  m_transportGUID = guid;
  if (IsLocalPlayer()) {
    MovementUpdateCameraYaw(guid);
    ProcessLocalMoveEvent(MSG_MOVE_HEARTBEAT);
  }
  return 1;
}

int CMovementData::SetTransport(DWORDLONG guid) {
  if (!guid && m_transportGUID && MovementInsideTransport(m_transportGUID, m_position)) {
    return 0;
  }
  return ForceSetTransport(guid);
}

BOOL CMovement::SetCollisionBox(const NTempest::CAaBox &box, float scale) {
  float boxDepth = box.t.x - box.b.x;
  float boxHeight = box.t.z - box.b.z;
  if (NTempest::CMath::fabs_(scale) < 0.00000095367432f || NTempest::CMath::fabs_(boxDepth) < 0.00000095367432f ||
      NTempest::CMath::fabs_(boxHeight) < 0.00000095367432f)
  {
    return 0;
  }

  m_stepUpHeight = scale;
  m_collisionBoxHalfDepth = boxDepth * scale * 0.5f;
  m_collisionBoxHeight = boxHeight * scale;
  if (1.849399f * m_collisionBoxHalfDepth > scale) {
    m_collisionBoxHalfDepth = scale * 0.54071623f;
  }
  return 1;
}

void CMovement::UpdateAnchors(DWORD eventTime) {
  m_anchorPosition = m_position;
  m_anchorFacing = m_facing;
  m_anchorPitch = m_pitch;
  m_moveStartTime = eventTime;
  m_moveFlags |= 0x200;
  FallLogWrite("0x%016I64X: setting move start time (0x%08X)\n", GetGUID(), eventTime);
  LogWrite(
      "0x%016I64X: Updating Anchors (0x%X): position(%g,%g) facing(%g degrees)", GetGUID(), m_moveStartTime, m_anchorPosition.x, m_anchorPosition.y,
      m_anchorFacing * 57.29578f
  );
  CalcDirection();
  LogWrite(" direction(%g,%g,%g)\n", m_direction.x, m_direction.y, m_direction.z);
  MovementNotifyZoneMgr(GetGUID());
}

BOOL CMovement::StartMove(DWORD eventTime, int forward) {
  m_moveFlags &= ~0x00190000;
  if (m_moveFlags & 0x4000) {
    if (m_moveFlags & 0xF) {
      if (forward) {
        if (!(m_moveFlags & 1)) {
          m_moveFlags |= 0x00080000;
        }
      } else {
        if (!(m_moveFlags & 2)) {
          m_moveFlags |= 0x00100000;
        }
      }
      LogWrite("0x%016I64X: Move start %s is pending\n", GetGUID(), forward ? "forward" : "backward");
      return 0;
    }
    m_moveFlags |= 0x20000000;
  }

  BothLogWrite(
      "0x%016I64X: Move start %s (0x%X) from (%g,%g,%g), (%g)\n", GetGUID(), forward ? "forward" : "backward", eventTime, m_position.x,
      m_position.y, m_position.z, m_facing
  );
  UpdateAnchors(eventTime);
  if (forward) {
    m_moveFlags = (m_moveFlags & ~2) | 1;
  } else {
    m_moveFlags = (m_moveFlags & ~1) | 2;
  }
  m_moveFlags |= 0x08000000;
  return 1;
}

void CMovement::OnMoveStart(DWORD eventTime, int forward) {
  if (StartMove(static_cast<CMovementGlobals *>(MovementGetGlobals())->m_lastUpdateTime, forward)) {
    AddToMoversList();
  }
}

void CMovement::AddPlayerMoveEvent(DWORD eventTime, int eventType, float facing) {
  if (!(uint(eventType) < NUM_PMOVE_EVTS)) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "uint(eventType) < NUM_PMOVE_EVTS", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "uint(eventType) < NUM_PMOVE_EVTS", FALSE, 1);
  }

  RemoveSpline();

  FATALASSERT(s_localMoveHeap != 0xffffffff);
  UINT              memHandle;
  CPlayerMoveEvent *event;
  if (!ObjectAlloc(s_localMoveHeap, &memHandle)) {
    event = NULL;
  } else {
    event = new (ObjectPtr(memHandle)) CPlayerMoveEvent;
    if (event) {
      event->memHandle = memHandle;
    }
  }

  event->eventType = static_cast<PLAYER_MOVE_EVT>(eventType);
  event->timeStamp = eventTime;
  event->facing = facing;

  ((CMovementGlobals *)MovementGetGlobals())->m_localMoveQueue.Enqueue(event);
  ((CMovementGlobals *)MovementGetGlobals())->m_localMover = this;
}

void CMovement::OnMoveStartLocal(DWORD eventTime, int forward) {
  AddPlayerMoveEvent(eventTime, forward ? PMOVE_MOVE_START_FWD : PMOVE_MOVE_START_BWD, 0.0f);
}

BOOL CMovement::StartStrafe(DWORD eventTime, int left) {
  m_moveFlags &= ~0x00620000;
  if (m_moveFlags & 0x4000) {
    if (m_moveFlags & 0xF) {
      if (left) {
        if (!(m_moveFlags & 4)) {
          m_moveFlags |= 0x00200000;
        }
      } else {
        if (!(m_moveFlags & 8)) {
          m_moveFlags |= 0x00400000;
        }
      }
      LogWrite("0x%016I64X: Strafe start %s is pending\n", GetGUID(), left ? "left" : "right");
      return 0;
    }
    m_moveFlags |= 0x20000000;
  }

  BothLogWrite(
      "0x%016I64X: Strafe start %s (0x%X) from (%g,%g,%g), (%g)\n", GetGUID(), left ? "left" : "right", eventTime, m_position.x, m_position.y,
      m_position.z, m_facing
  );
  UpdateAnchors(eventTime);
  if (left) {
    m_moveFlags = (m_moveFlags & ~8) | 4;
  } else {
    m_moveFlags = (m_moveFlags & ~4) | 8;
  }
  m_moveFlags |= 0x08000000;
  return 1;
}

void CMovement::OnStrafeStartLocal(DWORD eventTime, int left) {
  AddPlayerMoveEvent(eventTime, left ? PMOVE_STRAFE_START_LFT : PMOVE_STRAFE_START_RGT, 0.0f);
}

void CMovement::OnStrafeStart(DWORD eventTime, int left) {
  if (StartStrafe(((CMovementGlobals *)MovementGetGlobals())->m_lastUpdateTime, left)) {
    AddToMoversList();
  }
}

BOOL CMovement::ForceJump(DWORD eventTime) {
  if (m_moveFlags & 0x02000000) {
    return 0;
  }

  if (!((CMovementGlobals *)MovementGetGlobals())->ignoreObstacles) {
    UpdateAnchors(eventTime);
    FallFromTransport();
    m_jumpVelocity -= 7.9555473f;
    m_moveFlags = (m_moveFlags & ~0x1000) | 0x4000;
    m_fallStartElevation = m_position.z;
    m_fallStartTime = eventTime;
    BothLogWrite("0x%016I64X: Executed jump at (0x%08X) from Z(%g)\n", GetGUID(), eventTime, m_fallStartElevation);
  }
  return 1;
}

BOOL CMovement::Jump(DWORD eventTime) {
  if (m_moveFlags & 0x4000) {
    return 0;
  }
  return ForceJump(eventTime);
}

void CMovement::OnJumpLocal(DWORD eventTime) {
  AddPlayerMoveEvent(eventTime, PMOVE_JUMP, 0.0f);
}

void CMovement::OnJump(DWORD eventTime) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  if (Jump(globals->m_lastUpdateTime)) {
    AddToMoversList();
  }
}

void CMovement::OnFallLocal(DWORD eventTime) {
  AddPlayerMoveEvent(eventTime, PMOVE_FALL, 0.0f);
}

void CMovement::OnFall(DWORD eventTime) {
  StartFalling(eventTime);
  AddToMoversList();
}

void CMovement::OnCollideRedirServer(DWORD eventTime, const NTempest::C3Vector &position, float facing, const NTempest::C3Vector &redirection) {
  LogWrite(
      "0x%016I64X: Collide redirect (0x%X) at (%g,%g,%g), (%g), sliding (%g,%g,%g)\n", GetGUID(), eventTime, position.x, position.y, position.z,
      facing, redirection.x, redirection.y, redirection.z
  );
  LogWrite(
      "0x%016I64X: Moving for %u ms, (%g,%g) from (%g,%g) to (%g,%g)\n", GetGUID(), eventTime - m_moveStartTime, position.x - m_anchorPosition.x,
      position.y - m_anchorPosition.y, m_anchorPosition.x, m_anchorPosition.y, position.x, position.y
  );
  m_position = position;
  m_facing = facing;
  UpdateAnchors(eventTime);
  m_reDirection = redirection;
  UpdateCurrentSpeed();
  m_currentSpeed *= CMath::clamp_(NTempest::C3Vector::Dot(m_reDirection, m_direction), 0.0f, 1.0f);
  m_direction = redirection;
}

void CMovement::Halt(DWORD eventTime) {
  BothLogWrite(
      "0x%016I64X: Unit halted (0x%X) at (%g,%g,%g), (%g degrees)\n", GetGUID(), eventTime, m_position.x, m_position.y, m_position.z,
      m_facing * 57.29578f
  );
  if (!(m_moveFlags & 0x4000)) {
    m_moveFlags |= 0x10000000;
    return;
  }

  if (!(m_moveFlags & 0x10000)) {
    if (m_moveFlags & 1) {
      m_moveFlags |= 0x00080000;
    }
    if (m_moveFlags & 2) {
      m_moveFlags |= 0x00100000;
    }
  }
  if (!(m_moveFlags & 0x20000)) {
    if (m_moveFlags & 4) {
      m_moveFlags |= 0x00200000;
    }
    if (m_moveFlags & 8) {
      m_moveFlags |= 0x00400000;
    }
  }
  m_moveFlags = (m_moveFlags & 0xFFFCEFF0) | 0x08000000;
  UpdateAnchors(eventTime);
}

void CMovement::OnStuckServer(DWORD eventTime) {
  m_moveFlags = (m_moveFlags & 0xFFFCEFF0) | 0x18000000;
  UpdateAnchors(eventTime);
}

BOOL CMovement::StopMove(DWORD eventTime) {
  if (!(m_moveFlags & 3)) {
    if (m_moveFlags & 0x00180000) {
      m_moveFlags &= ~0x00180000;
      return 1;
    }
    return 0;
  }

  if (m_moveFlags & 0x4000) {
    BothLogWrite("0x%016I64X: Move stop is pending\n", GetGUID());
    m_moveFlags = (m_moveFlags & ~0x00180000) | 0x00010000;
    return 0;
  }

  ForceStopMove(eventTime);
  return 1;
}

void CMovement::ForceStopMove(DWORD eventTime) {
  BothLogWrite(
      "0x%016I64X: Move stop (0x%X) at (%g,%g,%g), (%g degrees)\n", GetGUID(), eventTime, m_position.x, m_position.y, m_position.z,
      m_facing * 57.29578f
  );
  m_moveFlags = (m_moveFlags & 0xFFFEEFFC) | 0x08000000;
  UpdateAnchors(eventTime);
}

void CMovement::OnMoveStopLocal(DWORD eventTime) {
  AddPlayerMoveEvent(eventTime, PMOVE_MOVE_STOP, 0.0f);
}

BOOL CMovement::OnMoveStop(DWORD eventTime) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  if (!StopMove(globals->m_lastUpdateTime) || (m_moveFlags & 0xC)) {
    return 0;
  }
  if (!(m_moveFlags & 0x40FF)) {
    RemoveFromMoversList();
  }
  return 1;
}

void CMovement::ForceStopStrafe(DWORD eventTime) {
  BothLogWrite(
      "0x%016I64X: Strafe stop (0x%X) at (%g,%g,%g), (%g degrees)\n", GetGUID(), eventTime, m_position.x, m_position.y, m_position.z,
      m_facing * 57.29578f
  );
  m_moveFlags = (m_moveFlags & 0xFFFDEFF3) | 0x08000000;
  UpdateAnchors(eventTime);
}

BOOL CMovement::StopStrafe(DWORD eventTime) {
  if (!(m_moveFlags & 0xC)) {
    if (m_moveFlags & 0x00600000) {
      m_moveFlags &= ~0x00600000;
      return 1;
    }
    return 0;
  }

  if (m_moveFlags & 0x4000) {
    BothLogWrite("0x%016I64X: Strafe stop is pending\n", GetGUID());
    m_moveFlags = (m_moveFlags & ~0x00600000) | 0x00020000;
    return 0;
  }

  ForceStopStrafe(eventTime);
  return 1;
}

void CMovement::OnStrafeStopLocal(DWORD eventTime) {
  AddPlayerMoveEvent(eventTime, PMOVE_STRAFE_STOP, 0.0f);
}

BOOL CMovement::OnStrafeStop(DWORD eventTime) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  if (!StopStrafe(globals->m_lastUpdateTime) || (m_moveFlags & 3)) {
    return 0;
  }
  if (!(m_moveFlags & 0x40FF)) {
    RemoveFromMoversList();
  }
  return 1;
}

void CMovement::StartTurn(DWORD eventTime, int left) {
  LogWrite(
      "0x%016I64X: Turn start %s (0x%X) from (%g,%g,%g), (%g)\n", GetGUID(), left ? "left" : "right", eventTime, m_position.x, m_position.y,
      m_position.z, m_facing
  );
  UpdateAnchors(eventTime);
  if (left) {
    m_moveFlags = (m_moveFlags & ~0x20) | 0x10;
  } else {
    m_moveFlags = (m_moveFlags & ~0x10) | 0x20;
  }
}

void CMovement::OnTurnStartLocal(DWORD eventTime, int left) {
  AddPlayerMoveEvent(eventTime, left ? PMOVE_TURN_START_LFT : PMOVE_TURN_START_RGT, 0.0f);
}

void CMovement::OnTurnStart(DWORD eventTime, int left) {
  StartTurn(((CMovementGlobals *)MovementGetGlobals())->m_lastUpdateTime, left);
  AddToMoversList();
}

void CMovement::StopTurn(DWORD eventTime) {
  if (IsTurning()) {
    LogWrite("0x%016I64X: Turn stop (0x%X) at (%g,%g,%g), (%g)\n", GetGUID(), eventTime, m_position.x, m_position.y, m_position.z, m_facing);
    UpdateAnchors(eventTime);
    m_moveFlags &= ~0x30;
  }
}

void CMovement::OnTurnStopLocal(DWORD eventTime) {
  AddPlayerMoveEvent(eventTime, PMOVE_TURN_STOP, 0.0f);
}

void CMovement::OnTurnStop(DWORD eventTime) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  StopTurn(globals->m_lastUpdateTime);
  if (!(m_moveFlags & 0x40FF)) {
    RemoveFromMoversList();
  }
}

void CMovement::OnSetRunModeLocal(DWORD eventTime, int run) {
  AddPlayerMoveEvent(eventTime, run ? PMOVE_SET_RUN_MODE : PMOVE_SET_WALK_MODE, 0.0f);
}

void CMovement::SetRunMode(DWORD eventTime, int run) {
  UpdateAnchors(eventTime);
  if (run) {
    m_moveFlags &= ~0x100U;
  } else {
    m_moveFlags |= 0x100;
  }
  m_moveFlags |= 0x08000000;
  LogWrite("0x%016I64X: Switching to %s mode (0x%08X)\n", m_guid, (m_moveFlags & 0x100) ? "walk" : "run", m_moveStartTime);
}

void CMovement::OnSetRunMode(DWORD eventTime, int run) {
  SetRunMode(static_cast<CMovementGlobals *>(MovementGetGlobals())->m_lastUpdateTime, run);
}

void CMovement::Teleport(DWORD eventTime, const NTempest::C3Vector &position, float facing) {
  LogWrite(
      "0x%016I64X: Teleporting (0x%X) from (%g,%g,%g) (%g) to (%g,%g,%g)\n", GetGUID(), eventTime, m_position.x, m_position.y, m_position.z,
      m_facing, position.x, position.y, position.z
  );
  m_position = position;
  m_facing = facing;
  m_pitch = 0.0f;
  ForceSetTransport(0);
  UpdateAnchors(eventTime);
  m_moveFlags = (m_moveFlags & 0xFDFCEF00) | 0x08000000;
  if (IsSpline()) {
    m_spline->flags |= 4;
  }

  if (!(IgnoresCollision() || IsSplineFlyer())) {
    CMovementGlobals *globals = (CMovementGlobals *)MovementGetGlobals();
    FATALASSERT(globals);
    if (!globals->ignoreObstacles && !(IgnoresCollision() || IsSplineFlyer())) {
      m_moveFlags |= 0x4000;
    }
    m_fallStartTime = eventTime;
    m_fallStartElevation = position.z;
    FallLogWrite("0x%016I64X: Started to fall from teleport at (0x%08X) from (%g elevation)\n", GetGUID(), eventTime, m_fallStartElevation);
    OnMoveUpdate(GetGUID(), eventTime);
  }
}

void CMovement::OnTeleport(DWORD eventTime, const NTempest::C3Vector &position, float facing) {
  DisconnectLocalMover(this);
  Teleport(((CMovementGlobals *)MovementGetGlobals())->m_lastUpdateTime, position, facing);
  AddToMoversList();
}

static void DisconnectLocalMover(CMovement *mover) {
  CMovementGlobals *globals = (CMovementGlobals *)MovementGetGlobals();
  if (globals && mover == globals->m_localMover) {
    globals->m_localMover = NULL;
    globals->m_localMoveQueue.DiscardAll();
  }
}

void CMovement::OnTeleportLocal(DWORD eventTime, const NTempest::C3Vector &position, float facing) {
  RemoveSpline();
  Teleport(((CMovementGlobals *)MovementGetGlobals())->m_lastUpdateTime, position, facing);
  ((CMovementGlobals *)MovementGetGlobals())->m_localMover = this;
}

void CMovement::OnSpline(DWORD eventTime, const NTempest::C3Vector *points, UINT count, DWORD duration, UINT flags) {
  AddSpline();
  m_spline->flags = flags;
  FATALASSERT(count >= 4);
  m_spline->start = eventTime;
  m_spline->time = duration;
  m_spline->spline.SetPoints(points, count);
  FATALASSERT(m_spline->time > 0);
  if (m_spline->flags & 0x200) {
    m_spline->spline.SetSplineMode(NTempest::C3Spline_CatmullRom::MODE_CATMULLROM);
  } else {
    m_spline->spline.SetSplineMode(NTempest::C3Spline_CatmullRom::MODE_LINEAR);
  }
  StopFalling();
  OnSetRunMode(eventTime, (flags >> 8) & 1);
  OnMoveStart(eventTime, 1);
  UnitUpdateMovementAnim(GetGUID());
}

void CMovement::OnSplineDoneFace(const NTempest::C3Vector &spot) {
  if (!m_spline || (m_spline->flags & 4)) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "IsSpline()", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "IsSpline()", FALSE, 1);
  }
  m_spline->flags |= 0x10000;
  m_spline->face.spot = spot;
}

void CMovement::OnSplineDoneFace(const DWORDLONG &guid) {
  if (!m_spline || (m_spline->flags & 4)) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "IsSpline()", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "IsSpline()", FALSE, 1);
  }
  m_spline->flags |= 0x20000;
  m_spline->face.guid = guid;
}

void CMovement::OnSplineDoneFace(float facing) {
  if (!m_spline || (m_spline->flags & 4)) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "IsSpline()", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "IsSpline()", FALSE, 1);
  }
  m_spline->flags |= 0x40000;
  m_spline->face.facing = facing;
}

void CMovement::GetMoveStatus(CMovementStatus *status) const {
  status->transport = m_transportGUID;
  status->moveFlags = m_moveFlags & 0xFAFF0BFF;
  status->transRelPosition = GetRawPosition();
  status->transRelFacing = GetRawFacing();
  status->worldPosition = GetPosition();
  status->worldFacing = GetFacing();
  status->pitch = GetPitch();

  if (IsSpline()) {
    status->moveFlags |= 0x04000000;
  }
}

void CMovement::GetUpdateInfo(CClientMoveUpdate *init) const {
  GetMoveStatus(&init->status);
  init->timeFallen = FallTime();
  init->walkSpeed = m_walkSpeed;
  init->runSpeed = m_runSpeed;
  init->swimSpeed = m_swimSpeed;
  init->turnRate = m_turnRate;

  if (IsSpline()) {
    init->spline = *m_spline;
  }

  LogWrite("0x%016I64X: Send move (0x%X) ", GetGUID(), m_moveStartTime);
  switch (m_moveFlags & 3) {
    case 2:
      LogWrite("moving backward from ");
      break;
    case 1:
      LogWrite("moving forward from ");
      break;
    case 0:
      LogWrite("standing at ");
      break;
  }
  LogWrite("(%g,%g), ", m_anchorPosition.x, m_anchorPosition.y);
  switch (m_moveFlags & 0x30) {
    case 0x20:
      LogWrite("turning right from ");
      break;
    case 0x10:
      LogWrite("turning left from ");
      break;
    case 0:
      LogWrite("facing ");
      break;
  }
  LogWrite("(%g)\n", m_anchorFacing);
}

void CMovement::UpdateCurrentSpeed() {
  if ((m_moveFlags & 0x4000) && !(m_moveFlags & 0x20000000)) {
    return;
  }

  if (!(m_moveFlags & 0xF)) {
    m_currentSpeed = 0.0f;
    LogWrite("0x%016I64X: Updating current speed to zero\n", GetGUID());
    return;
  }

  if (IsSpline()) {
    m_currentSpeed = m_spline->spline.Length() / m_spline->time * 1000.0f;
    LogWrite("0x%016I64X: Updating spline mover's current speed to (%g)\n", GetGUID(), m_currentSpeed);
    return;
  }

  float speed = m_runSpeed;
  UINT  slowFlags = 0x20000102;
  if (m_moveFlags & 0x02000000) {
    slowFlags = 0x2000010E;
    speed = m_swimSpeed;
  }
  if (slowFlags & m_moveFlags) {
    speed = min(speed, m_walkSpeed);
  }
  m_currentSpeed = speed;
  m_moveFlags &= ~0x20000000;
  LogWrite("0x%016I64X: Updating current speed to (%g)\n", GetGUID(), m_currentSpeed);
}

float CMovement::GetCurrentSpeed() {
  if (m_moveFlags & 0x08000000) {
    UpdateCurrentSpeed();
    m_moveFlags &= ~0x08000000;
  }
  return m_currentSpeed;
}

float CMovement::GetCurrentTurnRate() const {
  float rate;

  if (m_moveFlags & 0x10) {
    rate = m_turnRate;
  } else if (m_moveFlags & 0x20) {
    rate = -m_turnRate;
  } else {
    rate = 0.0f;
  }

  if (m_moveFlags & 0x400F) {
    rate *= 0.75f;
  }

  return rate;
}

float CMovement::GetCurrentPitchRate() const {
  float rate;

  if (m_moveFlags & 0x40) {
    rate = m_turnRate;
  } else if (m_moveFlags & 0x80) {
    rate = -m_turnRate;
  } else {
    rate = 0.0f;
  }

  if (m_moveFlags & 0x400F) {
    rate *= 0.75f;
  }

  return rate;
}

void CMovement::SetIdleUpdates() {
  if (m_moveFlags & 0x40FF) {
    AddToMoversList();
  } else {
    RemoveFromMoversList();
  }
}

void CMovement::SetServerInitData(float const runSpeed, float const walkSpeed, float const swimSpeed, float const turnRate) {
  FATALASSERT(CMath::fnotequal_(runSpeed,0));
  FATALASSERT(CMath::fnotequal_(walkSpeed,0));
  FATALASSERT(CMath::fnotequal_(swimSpeed,0));
  FATALASSERT(CMath::fnotequal_(turnRate,0));

  m_runSpeed = runSpeed;
  m_walkSpeed = walkSpeed;
  m_swimSpeed = swimSpeed;
  m_turnRate = turnRate;
}

void CMovementData::CalcDirection() {
  float sinFacing;
  float cosFacing;
  CMath::sincos_(m_anchorFacing, sinFacing, cosFacing);
  m_direction2d.Set(cosFacing, sinFacing);

  if (CMath::fequal4_(m_anchorPitch, 0)) {
    m_direction = m_direction2d;
    m_sinAnchorPitch = 0.0f;
    m_cosAnchorPitch = 1.0f;
  } else {
    CMath::sincos_(m_anchorPitch, m_sinAnchorPitch, m_cosAnchorPitch);
    m_direction.Set(m_cosAnchorPitch * cosFacing, m_cosAnchorPitch * sinFacing, m_sinAnchorPitch);
  }
}

void CMovement::UpdateTransportStatus(const CMovementStatus &update) {
  if (m_transportGUID == update.transport) {
    return;
  }

  NTempest::C34Matrix transportMtx;

  if (m_transportGUID) {
    transportLink.Unlink();
    MovementGetTransportMtx(m_transportGUID, &transportMtx);
    MovementFixUpMoveHistory(GetGUID(), transportMtx.AffineInverse());
    m_transportGUID = 0;
  }

  if (update.transport && MovementGameObjIsTransport(update.transport)) {
    MovementAddToTransport(this, update.transport);
    MovementGetTransportMtx(update.transport, &transportMtx);
    MovementFixUpMoveHistory(GetGUID(), transportMtx);

    m_position = update.transRelPosition;
    m_anchorPosition = update.transRelPosition;
    m_anchorFacing = m_facing = update.transRelFacing;
    m_transportGUID = update.transport;
  }
}

void CMovement::UpdateStatusInternal(DWORD eventTime, const CMovementStatus &update) {
  FallLogWrite(
      "0x%016I64X: time (0x%08X) updating from (%g,%g,%g) to (%g,%g,%g)\n", GetGUID(), eventTime, m_position.x, m_position.y, m_position.z,
      update.transRelPosition.x, update.transRelPosition.y, update.transRelPosition.z
  );

  m_moveFlags = (m_moveFlags & 0x0500E400) ^ (update.moveFlags & 0xFAFF0BFF) | 0x08000000;
  m_position = update.worldPosition;
  m_anchorPosition = update.worldPosition;
  m_anchorFacing = m_facing = update.worldFacing;
  m_anchorPitch = m_pitch = update.pitch;
  CalcDirection();

  if (m_moveFlags & 0xFF) {
    if (!(m_moveFlags & 0x200)) {
      this ? SErrDisplayErrorFmt(
                 STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
                 "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
                 "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
                 "m_moveFlags & MOVEFLAG_TIME_VALID", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
                 static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
                 static_cast<int>(GetFacing())
             )
           : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "m_moveFlags & MOVEFLAG_TIME_VALID", FALSE, 1);
    }
    m_moveStartTime = ((CMovementGlobals *)MovementGetGlobals())->m_lastUpdateTime;
    FallLogWrite("0x%016I64X: setting move start time (0x%08X)\n", GetGUID(), m_moveStartTime);
  }

  MovementNotifyZoneMgr(GetGUID());
}

void CMovement::UpdateStatusLocal(DWORD eventTime, const CMovementStatus &update) {
  UpdateStatusInternal(eventTime, update);
  ((CMovementGlobals *)MovementGetGlobals())->m_localMover = (m_moveFlags & 0x40FF) ? this : NULL;
}

void CMovement::UpdateStatus(DWORD eventTime, const CMovementStatus &update) {
  UpdateStatusInternal(eventTime, update);
  SetIdleUpdates();
  if (m_position.z < -333.33334f) {
    MovementFixOutOfBoundsUnit(m_guid);
  }
}

void CMovement::LogUpdateInfo(const CClientMoveUpdate &init) {
  FallLogWrite(
      "0x%016I64X: ___INIT EVENT: time(0x%X) timeFallen(%u) pos(%g,%g,%g)\n", GetGUID(),
      ((CMovementGlobals *)MovementGetGlobals())->m_lastUpdateTime, init.timeFallen, init.status.worldPosition.x, init.status.worldPosition.y,
      init.status.worldPosition.z
  );
  LogWrite("0x%016I64X: Receive move (0x%X) ", GetGUID(), m_moveStartTime);
  switch (m_moveFlags & 3) {
    case 2:
      LogWrite("moving backward from ");
      break;
    case 1:
      LogWrite("moving forward from ");
      break;
    case 0:
      LogWrite("standing at ");
      break;
  }
  LogWrite("(%g,%g), ", m_anchorPosition.x, m_anchorPosition.y);
  switch (m_moveFlags & 0x30) {
    case 0x20:
      LogWrite("turning right from ");
      break;
    case 0x10:
      LogWrite("turning left from ");
      break;
    case 0:
      LogWrite("facing ");
      break;
  }
  LogWrite("(%g)\n", m_anchorFacing);
}

void CMovement::AddSpline() {
  m_moveFlags |= 0x04000000;
  if (!m_spline) {
    m_spline = NEW(CMoveSpline);
  }
  DisconnectLocalMover(this);
}

void CMovementData::RemoveSpline() {
  m_moveFlags &= ~0x04000000u;
  if (m_spline) {
    delete m_spline;
    m_spline = 0;
  }
  RemoveFromMoversList();
}

void CMovement::SetUpdateInfo(DWORD eventTime, const CClientMoveUpdate &init, int localPlayer) {
  FATALASSERT(CMath::fnotequal_(init.runSpeed,0));
  FATALASSERT(CMath::fnotequal_(init.walkSpeed,0));
  FATALASSERT(CMath::fnotequal_(init.swimSpeed,0));
  FATALASSERT(CMath::fnotequal_(init.turnRate,0));

  m_walkSpeed = init.walkSpeed;
  m_runSpeed = init.runSpeed;
  m_swimSpeed = init.swimSpeed;
  m_turnRate = init.turnRate;

  m_moveStartTime = ((CMovementGlobals *)MovementGetGlobals())->m_lastUpdateTime;
  FallLogWrite("0x%016I64X: setting move start time (0x%08X)\n", GetGUID(), m_moveStartTime);
  m_fallStartTime = ((CMovementGlobals *)MovementGetGlobals())->m_lastUpdateTime - init.timeFallen;

  if (init.status.moveFlags & 0x04000000) {
    AddSpline();
    *m_spline = init.spline;
  } else {
    RemoveSpline();
  }

  if (((CMovementGlobals *)MovementGetGlobals())->ignoreObstacles) {
    m_moveFlags |= 0x08000000;
  } else {
    m_moveFlags |= 0x08004000;
  }

  if (localPlayer) {
    UpdateStatusLocal(((CMovementGlobals *)MovementGetGlobals())->m_lastUpdateTime, init.status);
  } else {
    UpdateStatus(((CMovementGlobals *)MovementGetGlobals())->m_lastUpdateTime, init.status);
  }

  m_moveFlags |= 0x200;
  m_fallStartElevation = CalcFallStartElevation(init.timeFallen);
  if (HasFallenFar()) {
    OnCollideFalling(GetGUID(), ((CMovementGlobals *)MovementGetGlobals())->m_lastUpdateTime);
  }

  LogUpdateInfo(init);
}

void CMovement::StartFallLogging() {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  ASSERT(globals);

  char fileName[MAX_PATH];
  char exten[5];
  SStrCopy(fileName, globals->logFileName, sizeof(fileName));

  char *dot = SStrChrR(fileName, '.');
  if (dot) {
    SStrCopy(exten, dot, sizeof(exten));
    *dot = 0;
  }

  SStrPack(fileName, "__fall", sizeof(fileName));
  SStrPack(fileName, exten, sizeof(fileName));

  if (!globals->fallingLog) {
    globals->fallingLog = fopen(fileName, "wt");
  }
}

int CMovement::ToggleFallLogging() {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  if (!globals) {
    return 0;
  }

  if (globals->fallingLog) {
    StopFallLogging();
  } else {
    StartFallLogging();
  }

  return IsFallLoggingOn();
}

BOOL CMovement::IsFallLoggingOn() {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  return globals && globals->fallingLog;
}

void __cdecl CMovement::BothLogWrite(LPCSTR format, ...) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  if (globals) {
    va_list arglist;
    va_start(arglist, format);
    WVLog(0x400, 0x1E, format, arglist);
    if (globals->fallingLog) {
      vfprintf(globals->fallingLog, format, arglist);
      fflush(globals->fallingLog);
    }
    if (globals->movementLog) {
      vfprintf(globals->movementLog, format, arglist);
      fflush(globals->movementLog);
    }
    va_end(arglist);
  }
}

void __cdecl CMovement::FallLogWrite(LPCSTR format, ...) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  if (globals && globals->fallingLog) {
    va_list arglist;
    va_start(arglist, format);
    WVLog(0x400, 0x1E, format, arglist);
    vfprintf(globals->fallingLog, format, arglist);
    fflush(globals->fallingLog);
    va_end(arglist);
  }
}

void CMovement::StopFallLogging() {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  if (globals && globals->fallingLog) {
    fclose(globals->fallingLog);
    globals->fallingLog = 0;
  }
}

void CMovement::StartLogging() {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  ASSERT(globals);

  if (!globals->movementLog) {
    globals->movementLog = fopen(globals->logFileName, "wt");
  }
}

int CMovement::ToggleLogging() {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  ASSERT(globals);

  if (globals->movementLog) {
    StopLogging();
  } else {
    StartLogging();
  }

  return IsLoggingOn();
}

BOOL CMovement::IsLoggingOn() {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  ASSERT(globals);
  return globals->movementLog != 0;
}

void __cdecl CMovement::LogWrite(LPCSTR format, ...) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  if (globals && globals->movementLog) {
    va_list arglist;
    va_start(arglist, format);
    WVLog(0x400, 0xA, format, arglist);
    vfprintf(globals->movementLog, format, arglist);
    fflush(globals->movementLog);
    va_end(arglist);
  }
}

void CMovement::StopAllLogging() {
  StopLogging();
  StopFallLogging();
}

void CMovement::StopLogging() {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  if (globals && globals->movementLog) {
    fclose(globals->movementLog);
    globals->movementLog = 0;
  }
}

BOOL CMovement::OnRunSpeedChange(DWORD eventTime, float speed) {
  if (!(CMath::fnotequal_(speed,0))) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "CMath::fnotequal_(speed,0)", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "CMath::fnotequal_(speed,0)", FALSE, 1);
  }
  if (CMath::fequal_(speed, m_runSpeed)) {
    return 0;
  }
  LogWrite("0x%016I64X: Changing run speed from (%g) to (%g) (0x%X)\n", GetGUID(), m_runSpeed, speed, m_moveStartTime);
  m_runSpeed = speed;
  m_moveFlags |= 0x08000000;
  UpdateAnchors(((CMovementGlobals *)MovementGetGlobals())->m_lastUpdateTime);
  return 1;
}

BOOL CMovement::OnWalkSpeedChange(DWORD eventTime, float speed) {
  if (!(CMath::fnotequal_(speed,0))) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "CMath::fnotequal_(speed,0)", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "CMath::fnotequal_(speed,0)", FALSE, 1);
  }
  if (CMath::fequal_(speed, m_walkSpeed)) {
    return 0;
  }
  LogWrite("0x%016I64X: Changing walk speed from (%g) to (%g) (0x%X)\n", GetGUID(), m_walkSpeed, speed, m_moveStartTime);
  m_walkSpeed = speed;
  m_moveFlags |= 0x08000000;
  UpdateAnchors(((CMovementGlobals *)MovementGetGlobals())->m_lastUpdateTime);
  return 1;
}

BOOL CMovement::OnSwimSpeedChange(DWORD eventTime, float speed) {
  if (!(CMath::fnotequal_(speed,0))) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "CMath::fnotequal_(speed,0)", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "CMath::fnotequal_(speed,0)", FALSE, 1);
  }
  if (CMath::fequal_(speed, m_swimSpeed)) {
    return 0;
  }
  LogWrite("0x%016I64X: Changing swim speed from (%g) to (%g) (0x%X)\n", GetGUID(), m_swimSpeed, speed, m_moveStartTime);
  m_swimSpeed = speed;
  m_moveFlags |= 0x08000000;
  UpdateAnchors(((CMovementGlobals *)MovementGetGlobals())->m_lastUpdateTime);
  return 1;
}

BOOL CMovement::OnTurnRateChange(DWORD eventTime, float rate) {
  if (!(CMath::fnotequal_(rate,0))) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "CMath::fnotequal_(rate,0)", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "CMath::fnotequal_(rate,0)", FALSE, 1);
  }
  if (CMath::fequal_(rate, m_turnRate)) {
    return 0;
  }
  LogWrite("0x%016I64X: Changing turn rate from (%g) to (%g) (0x%X)\n", GetGUID(), m_turnRate, rate, m_moveStartTime);
  m_turnRate = rate;
  UpdateAnchors(((CMovementGlobals *)MovementGetGlobals())->m_lastUpdateTime);
  return 1;
}

void CMovement::OnSetRawFacingLocal(DWORD eventTime, float facing) {
  AddPlayerMoveEvent(eventTime, PMOVE_SET_FACING, facing);
}

void CMovement::OnSetFacingLocal(DWORD eventTime, float facing) {
  facing -= m_transportGUID ? MovementGetTransportFacing(m_transportGUID) : 0.0f;
  if (facing > 6.2831855f) {
    facing -= 6.2831855f;
  } else if (facing < 0.0f) {
    facing += 6.2831855f;
  }

  AddPlayerMoveEvent(eventTime, PMOVE_SET_FACING, facing);
}

void CMovement::SetFacing(DWORD eventTime, float facing) {
  if (CMath::fnotequal4_(facing, m_facing)) {
    BothLogWrite(
        "0x%016I64X: Set Facing (0x%X) at (%g,%g,%g), (%g degrees), changed facing to (%g degrees)\n", GetGUID(), eventTime, m_position.x,
        m_position.y, m_position.z, m_facing * 57.29578f, facing * 57.29578f
    );
    m_facing = facing;
    if (!(m_moveFlags & 0x4000)) {
      UpdateAnchors(eventTime);
    }
  }
  m_moveFlags &= ~0x30;
}

void CMovement::OnSetPitchLocal(DWORD eventTime, float pitch) {
  AddPlayerMoveEvent(eventTime, PMOVE_SET_PITCH, pitch);
}

void CMovement::SetPitch(DWORD eventTime, float pitch) {
  if (IsSwimming() && CMath::fnotequal4_(pitch, m_pitch)) {
    BothLogWrite(
        "0x%016I64X: Set Pitch (0x%X) at (%g,%g,%g), (%g degrees), changed facing to (%g degrees)\n", GetGUID(), eventTime, m_position.x,
        m_position.y, m_position.z, m_pitch * 57.29578f, pitch * 57.29578f
    );
    m_pitch = pitch;
    UpdateAnchors(eventTime);
  }
  m_moveFlags &= ~0xC0;
}

void CMovement::OnSetFacing(DWORD eventTime, float facing) {
  facing -= m_transportGUID ? MovementGetTransportFacing(m_transportGUID) : 0.0f;
  if (facing > 6.2831855f) {
    facing -= 6.2831855f;
  } else if (facing < 0.0f) {
    facing += 6.2831855f;
  }

  SetFacing(((CMovementGlobals *)MovementGetGlobals())->m_lastUpdateTime, facing);
  if (!(m_moveFlags & 0x40FF)) {
    RemoveFromMoversList();
  }
}

void CMovement::OnSetPitch(DWORD eventTime, float pitch) {
  SetPitch(((CMovementGlobals *)MovementGetGlobals())->m_lastUpdateTime, pitch);
  if (!(m_moveFlags & 0x40FF)) {
    RemoveFromMoversList();
  }
}

void MovementEnableCollision(int enable) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  FATALASSERT(globals);
  globals->ignoreObstacles = enable == 0;
}

void CMovement::OnDisableGravity(DWORD eventTime) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  if (!globals->ignoreObstacles) {
    UINT oldMoveFlags = m_moveFlags;
    StopFalling();
    HandlePendingActions(eventTime);
    CallMoveEventHandlers(eventTime, 0, oldMoveFlags, 0);
  }
}

void CMovement::OnEnableGravity(DWORD eventTime) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  if (!globals->ignoreObstacles) {
    StartFalling(eventTime);
  }
}

void CMovement::CollisionStateChanged() {
  if (IgnoresCollision() || IsSplineFlyer()) {
    OnDisableGravity(((CMovementGlobals *)MovementGetGlobals())->m_lastUpdateTime);
    if (!(m_moveFlags & 0x40FF)) {
      RemoveFromMoversList();
    }
  } else {
    OnEnableGravity(((CMovementGlobals *)MovementGetGlobals())->m_lastUpdateTime);
    AddToMoversList();
  }
}

void CMovement::CollisionStateChangedLocal(DWORD eventTime) {
  if (IgnoresCollision() || IsSplineFlyer()) {
    OnDisableGravity(eventTime);
    if (!(m_moveFlags & 0xF)) {
      ((CMovementGlobals *)MovementGetGlobals())->m_localMover = NULL;
    }
  } else {
    OnEnableGravity(eventTime);
    ((CMovementGlobals *)MovementGetGlobals())->m_localMover = this;
  }
}

void CMovement::ToggleCollision(DWORD eventTime) {
  UpdateAnchors(((CMovementGlobals *)MovementGetGlobals())->m_lastUpdateTime);
  m_moveFlags ^= 0x800;
  CollisionStateChangedLocal(((CMovementGlobals *)MovementGetGlobals())->m_lastUpdateTime);
}

void CMovement::EnableCollision(DWORD eventTime, int enable) {
  UpdateAnchors(((CMovementGlobals *)MovementGetGlobals())->m_lastUpdateTime);
  if (enable) {
    m_moveFlags &= ~0x800;
  } else {
    m_moveFlags |= 0x800;
  }
  CollisionStateChanged();
}

void CMovement::BuildFullZoneUpdate(CDataStore *msg) {
  *msg << m_position;
  *msg << m_facing;
  if (IsSpline()) {
    *msg << m_moveFlags;
  } else {
    *msg << (m_moveFlags & ~0x04000000);
  }
  *msg << m_anchorPosition;
  *msg << m_anchorFacing;
  *msg << m_moveStartTime;
  *msg << m_direction;
  *msg << m_direction2d;
  *msg << m_reDirection;
  *msg << m_fallStartTime;
  *msg << m_fallStartElevation;
  *msg << m_walkSpeed;
  *msg << m_runSpeed;
  *msg << m_swimSpeed;
  *msg << m_turnRate;
  *msg << m_collisionBoxHalfDepth;
  *msg << m_collisionBoxHeight;
  *msg << m_stepUpHeight;
  *msg << m_jumpVelocity;
  *msg << m_transportGUID;
}

void CMovement::UnpackFullZoneUpdate(CDataStore *msg) {
  *msg >> m_position;
  *msg >> m_facing;
  *msg >> m_moveFlags;
  *msg >> m_anchorPosition;
  *msg >> m_anchorFacing;
  *msg >> m_moveStartTime;
  *msg >> m_direction;
  *msg >> m_direction2d;
  *msg >> m_reDirection;
  *msg >> m_fallStartTime;
  *msg >> m_fallStartElevation;
  *msg >> m_walkSpeed;
  *msg >> m_runSpeed;
  *msg >> m_swimSpeed;
  *msg >> m_turnRate;
  *msg >> m_collisionBoxHalfDepth;
  *msg >> m_collisionBoxHeight;
  *msg >> m_stepUpHeight;
  *msg >> m_jumpVelocity;
  *msg >> m_transportGUID;
}

int CMovement::SkipFullZoneUpdate(CDataStore *msg) {
  DWORDLONG fakeGuid;
  CMovement fakeMovement(fakeGuid);
  fakeMovement.UnpackFullZoneUpdate(msg);
  return fakeMovement.IsSplineMover();
}

void CMovement::PutHandoffData(CDataStore *msg) {
  BuildFullZoneUpdate(msg);
  if (IsSpline()) {
    *msg << *m_spline;
  }
}

void CMovement::GetHandoffData(CDataStore *msg) {
  UnpackFullZoneUpdate(msg);
  if (IsSplineMover()) {
    AddSpline();
    *msg >> *m_spline;
  } else {
    RemoveSpline();
  }
}

void CMovement::SkipHandoffData(CDataStore *msg) {
  if (SkipFullZoneUpdate(msg)) {
    CMoveSpline::Skip(msg);
  }
}

void CMovement::AddToMoversList() {
  CMovementGlobals *globals = (CMovementGlobals *)MovementGetGlobals();
  if (globals) {
    if (!(((CMovementGlobals *)MovementGetGlobals())->m_localMover != this)) {
      this ? SErrDisplayErrorFmt(
                 STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
                 "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
                 "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
                 "((CMovementGlobals *)MovementGetGlobals())->m_localMover != this", GetGUID(), GetPosition().x, GetPosition().y,
                 GetPosition().z, GetFacing(), static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y),
                 static_cast<int>(GetPosition().z), static_cast<int>(GetFacing())
             )
           : SErrDisplayError(
                 STORM_ERROR_ASSERTION, __FILE__, __LINE__, "((CMovementGlobals *)MovementGetGlobals())->m_localMover != this", FALSE, 1
             );
    }
    MovementLockMoversList(1);
    if (!globals->movers.IsLinked(this)) {
      globals->movers.LinkNode(this, LIST_TAIL, NULL);
      ++globals->numMovers;
    }
    MovementUnlockMoversList(1);
  }
}

void CMovementData::RemoveFromMoversList() {
  CMovementGlobals *globals = (CMovementGlobals *)MovementGetGlobals();
  if (globals) {
    MovementLockMoversList(1);
    if (globals->movers.IsLinked(this)) {
      globals->movers.UnlinkNode(this);
      --globals->numMovers;
    }
    MovementUnlockMoversList(1);
  }
}

UINT CMovement::FallTime() const {
  if (m_moveFlags & 0x4000) {
    CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
    return globals->m_lastUpdateTime - m_fallStartTime;
  }

  return 0;
}

void CMovement::SaveMoveState(CMoveState *state) const {
  state->position = m_position;
  state->facing = m_facing;
  state->pitch = m_pitch;
  state->moveFlags = m_moveFlags;
  state->anchorPosition = m_anchorPosition;
  state->anchorFacing = m_anchorFacing;
  state->anchorPitch = m_anchorPitch;
  state->moveStartTime = m_moveStartTime;
  state->direction = m_direction;
  state->direction2d = m_direction2d;
  state->cosAnchorPitch = m_cosAnchorPitch;
  state->sinAnchorPitch = m_sinAnchorPitch;
  state->reDirection = m_reDirection;
  state->fallStartTime = m_fallStartTime;
  state->fallStartElevation = m_fallStartElevation;
  state->jumpVelocity = m_jumpVelocity;
}

void CMovement::RestoreMoveState(const CMoveState &state) {
  m_position = state.position;
  m_facing = state.facing;
  m_pitch = state.pitch;
  m_moveFlags = state.moveFlags;
  m_anchorPosition = state.anchorPosition;
  m_anchorFacing = state.anchorFacing;
  m_anchorPitch = state.anchorPitch;
  m_moveStartTime = state.moveStartTime;
  m_direction = state.direction;
  m_direction2d = state.direction2d;
  m_cosAnchorPitch = state.cosAnchorPitch;
  m_sinAnchorPitch = state.sinAnchorPitch;
  m_reDirection = state.reDirection;
  m_fallStartTime = state.fallStartTime;
  m_fallStartElevation = state.fallStartElevation;
  m_jumpVelocity = state.jumpVelocity;
  m_moveFlags |= 0x08000000;
  FallLogWrite("0x%016I64X: restoring move state (0x%08X)\n", m_guid, m_moveStartTime);
}

int CMovement::GetMoveEventMsgId(UINT oldMoveFlags, BOOL wasJumping) {
  UINT changed = oldMoveFlags ^ m_moveFlags;
  if (changed & 3) {
    if (IsMovingForward()) {
      return MSG_MOVE_START_FORWARD;
    }
    if (IsMovingBackwards()) {
      return MSG_MOVE_START_BACKWARD;
    }
    return MSG_MOVE_STOP;
  }
  if (changed & 0xC) {
    if (IsStrafingLeft()) {
      return MSG_MOVE_START_STRAFE_LEFT;
    }
    if (IsStrafingRight()) {
      return MSG_MOVE_START_STRAFE_RIGHT;
    }
    return MSG_MOVE_STOP_STRAFE;
  }
  if (m_jumpVelocity != 0.0f && !wasJumping) {
    return MSG_MOVE_JUMP;
  }
  if (changed & 0x02000000) {
    if (IsSwimming()) {
      return MSG_MOVE_START_SWIM;
    }
    return MSG_MOVE_STOP_SWIM;
  }
  return MSG_MOVE_STOP;
}

void CMovement::OnSwimStartLocal(DWORD eventTime) {
  AddPlayerMoveEvent(eventTime, PMOVE_MOVE_START_SWIM, 0.0f);
}

void CMovement::OnSwimStopLocal(DWORD eventTime) {
  AddPlayerMoveEvent(eventTime, PMOVE_MOVE_STOP_SWIM, 0.0f);
}

void CMovement::OnSwimStart(DWORD eventTime) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  StartSwim(globals->m_lastUpdateTime);
  if (!(m_moveFlags & 0x40FF)) {
    RemoveFromMoversList();
  }
}

void CMovement::OnSwimStop(DWORD eventTime) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  StopSwim(globals->m_lastUpdateTime);
  AddToMoversList();
}

void CMovement::StartSwimLocal(DWORD eventTime) {
  StartSwim(eventTime);
  if (!(m_moveFlags & 0xF)) {
    static_cast<CMovementGlobals *>(MovementGetGlobals())->m_localMover = 0;
  }
}

void CMovement::StopSwimLocal(DWORD eventTime) {
  StopSwim(eventTime);
  static_cast<CMovementGlobals *>(MovementGetGlobals())->m_localMover = this;
}

void CMovement::StartSwim(DWORD eventTime) {
  BothLogWrite("0x%016I64X: Swim start (0x%X) from (%g,%g,%g), (%g)\n", m_guid, eventTime, m_position.x, m_position.y, m_position.z, m_facing);
  UpdateAnchors(eventTime);
  m_moveFlags |= 0x0A000000;
  OnDisableGravity(eventTime);
}

void CMovement::StopSwim(DWORD eventTime) {
  BothLogWrite("0x%016I64X: Swim stop (0x%X) from (%g,%g,%g), (%g)\n", m_guid, eventTime, m_position.x, m_position.y, m_position.z, m_facing);
  UpdateAnchors(eventTime);
  m_moveFlags = (m_moveFlags & 0xF5FFFF3F) | 0x08000000;
  m_anchorPitch = 0.0f;
  m_pitch = 0.0f;
  OnEnableGravity(eventTime);
}

void CMovement::OnPitchStartLocal(DWORD eventTime, int up) {
  if (!(IsSwimming())) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "IsSwimming()", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "IsSwimming()", FALSE, 1);
  }
  AddPlayerMoveEvent(eventTime, up ? PMOVE_PITCH_START_UP : PMOVE_PITCH_START_DOWN, 0.0f);
}

void CMovement::OnPitchStart(DWORD eventTime, int up) {
  StartPitch(((CMovementGlobals *)MovementGetGlobals())->m_lastUpdateTime, up);
  AddToMoversList();
}

void CMovement::OnPitchStopLocal(DWORD eventTime) {
  if (!(IsSwimming())) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "IsSwimming()", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "IsSwimming()", FALSE, 1);
  }
  AddPlayerMoveEvent(eventTime, PMOVE_PITCH_STOP, 0.0f);
}

void CMovement::OnPitchStop(DWORD eventTime) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  StopPitch(globals->m_lastUpdateTime);
  if (!(m_moveFlags & 0x40FF)) {
    RemoveFromMoversList();
  }
}

void CMovement::StartPitch(DWORD eventTime, int up) {
  if (IsSwimming()) {
    LogWrite(
        "0x%016I64X: Pitch start %s (0x%X) from (%g,%g,%g), (%g)\n", GetGUID(), up ? "up" : "down", eventTime, m_position.x, m_position.y,
        m_position.z, m_facing
    );
    UpdateAnchors(eventTime);
    if (up) {
      m_moveFlags = (m_moveFlags & ~0x80) | 0x40;
    } else {
      m_moveFlags = (m_moveFlags & ~0x40) | 0x80;
    }
  }
}

void CMovement::StopPitch(DWORD eventTime) {
  if (IsSwimming() && IsPitching()) {
    LogWrite("0x%016I64X: Pitch stop (0x%X) at (%g,%g,%g), (%g)\n", GetGUID(), eventTime, m_position.x, m_position.y, m_position.z, m_facing);
    UpdateAnchors(eventTime);
    m_moveFlags &= ~0xC0;
  }
}
