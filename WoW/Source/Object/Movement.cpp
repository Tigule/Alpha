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

void WVLog(unsigned int logMask, unsigned int priority, const char *fmt, char *arglist);
void OnMoveUpdate(unsigned __int64 unit, unsigned long eventTime);
void OnCollideFalling(unsigned __int64 unit, unsigned long eventTime);
void UnitUpdateMovementAnim(const unsigned __int64 &unit);

static unsigned int s_localMoveHeap = -1;

int CMovementData::IsLocalPlayer() {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  return globals && globals->m_localMover == this;
}

CMovementData::~CMovementData() {
  if (MovementGetGlobals() &&
      static_cast<CMovementGlobals *>(MovementGetGlobals())->m_localMover == this)
  {
    static_cast<CMovementGlobals *>(MovementGetGlobals())->m_localMover = 0;
    FATALASSERT(!m_spline);
    FATALASSERT(!IsSplineMover());
  } else {
    RemoveSpline();
  }
}

int CMovement::MoversOnList() {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  return globals && globals->numMovers > 0;
}

void CMovement::MoveUnit(unsigned long timeNow, unsigned long lastUpdate, void *obj) {
  m_moveFlags &= ~0x00800000;

  if (static_cast<int>(timeNow - m_moveStartTime) < 0) {
    timeNow = m_moveStartTime;
  }

  if (m_moveFlags & 0x40FF) {
    ApplyMovement(lastUpdate + 1, FallTime(), timeNow - m_moveStartTime, timeNow - lastUpdate);
    m_moveFlags |= 0x00800000;
    OnMoveUpdate(m_guid, timeNow);

    if (m_moveFlags & 0x40FF) {
      if (_isnan(static_cast<double>(m_position.x)) ||
          _isnan(static_cast<double>(m_position.y)) ||
          _isnan(static_cast<double>(m_position.z))) {
        ConsolePrintf("Mover at invalid position");
        MovementFixOutOfBoundsUnit(m_guid);
      } else {
        MovementUpdateProxMap(obj);
      }
    }
  }
}

void CMovement::MoveUnits(unsigned long timeNow, unsigned long lastUpdate) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  if (!globals) {
    return;
  }

  int entriesOnList = globals->movers.Head() != 0;
  if (entriesOnList) {
    FallLogWrite("___IDLE EVENT: timeNow(0x%X)\n", timeNow);
  }

  CMovementData *baseMover = globals->movers.Head();
  while (reinterpret_cast<long>(baseMover) > 0) {
    CMovementData *baseMovernext_node = globals->movers.RawNext(baseMover);
    CMovement     *mover = static_cast<CMovement *>(baseMover);
    FATALASSERT(mover != ((CMovementGlobals *)MovementGetGlobals())->m_localMover);

    void *obj = MovementTryLock(mover->m_guid);
    if (obj) {
      mover->m_moveFlags &= ~0x00800000;

      unsigned long moveStartTime = mover->m_moveStartTime;
      if (int(timeNow - moveStartTime) < 0) {
        timeNow = moveStartTime;
      }

      unsigned int moveTime = timeNow - moveStartTime;
      if (int(timeNow - moveStartTime) < 0) {
        NTempest::C3Vector intPositionZ = mover->GetPosition(mover->m_position);
        NTempest::C3Vector intPositionY = mover->GetPosition(mover->m_position);
        NTempest::C3Vector intPositionX = mover->GetPosition(mover->m_position);
        float              facing = mover->GetFacing(mover->m_facing);
        NTempest::C3Vector positionZ = mover->GetPosition(mover->m_position);
        NTempest::C3Vector positionY = mover->GetPosition(mover->m_position);
        NTempest::C3Vector positionX = mover->GetPosition(mover->m_position);
        int                intFacing = static_cast<int>(mover->GetFacing(mover->m_facing));
        SErrDisplayErrorFmt(
            0x85100000,
            __FILE__,
            __LINE__,
            0,
            1,
            "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
            "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
            "int(timeNow - moveStartTime) >= 0",
            mover->m_guid,
            positionX.x,
            positionY.y,
            positionZ.z,
            facing,
            static_cast<int>(intPositionX.x),
            static_cast<int>(intPositionY.y),
            static_cast<int>(intPositionZ.z),
            intFacing
        );
      }

      unsigned int elapsed = timeNow - lastUpdate;
      FallLogWrite(
          "0x%016I64X: last update(0x%08X) fall start time(0x%08X) current time(0x%08X) elapsed time(%u ms)\n",
          mover->m_guid,
          lastUpdate,
          mover->m_fallStartTime,
          timeNow,
          elapsed
      );

      if (mover->m_moveFlags & 0x40FF) {
        mover->ApplyMovement(lastUpdate + 1, mover->FallTime(), moveTime, elapsed);
        mover->m_moveFlags |= 0x00800000;
        OnMoveUpdate(mover->m_guid, timeNow);
      } else {
        LogWrite(
            "(%I64X) (%u) ERROR!!! Somehow we are in the mover list with a speed and turn rate both of zero\n",
            mover->m_guid,
            timeNow
        );
      }

      if (!(mover->m_moveFlags & 0x40FF)) {
        globals->movers.UnlinkNode(mover);
        --globals->numMovers;
        MovementUnlock(obj);
        baseMover = baseMovernext_node;
        continue;
      } else {
        if (_isnan(static_cast<double>(mover->m_position.x)) ||
            _isnan(static_cast<double>(mover->m_position.y)) ||
            _isnan(static_cast<double>(mover->m_position.z))) {
          ConsolePrintf("Mover at invalid position");
          MovementFixOutOfBoundsUnit(mover->m_guid);
          MovementUnlock(obj);
          baseMover = baseMovernext_node;
          continue;
        } else {
          MovementUpdateProxMap(obj);
          MovementUnlock(obj);
          baseMover = baseMovernext_node;
          continue;
        }
      }
    }

    baseMover = baseMovernext_node;
  }

  if (entriesOnList) {
    FallLogWrite("___END IDLE EVENT\n");
  }
}

int CMovement::UpdatePlayerMovement(unsigned long timeNow) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());

  while (CPlayerMoveEvent *event = globals->m_localMoveQueue.m_events.Head()) {
    if (static_cast<int>(timeNow - event->timeStamp) < 0) {
      break;
    }

    globals->m_localMoveQueue.m_events.UnlinkNode(event);

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
        OnCollideFalling(m_guid, event->timeStamp);
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
      default:
        break;
    }

    ObjectFree(event->memHandle);
  }

  return !globals->m_localMoveQueue.m_events.IsEmpty() || (m_moveFlags & 0x40FF) != 0;
}

void CMovement::MoveLocalPlayer(unsigned long timeNow, unsigned long lastUpdate) {
  unsigned int elapsedMS = timeNow - lastUpdate;
  unsigned int timeUsed = 0;

  LogWrite(
      "0x%016I64X: current time(0x%08X) last update(0x%08X) time elapsed(%u)\n",
      m_guid,
      timeNow,
      lastUpdate,
      elapsedMS
  );
  if (!elapsedMS) {
    NTempest::C3Vector intPositionZ = GetPosition(m_position);
    NTempest::C3Vector intPositionY = GetPosition(m_position);
    NTempest::C3Vector intPositionX = GetPosition(m_position);
    float              facing = GetFacing(m_facing);
    NTempest::C3Vector positionZ = GetPosition(m_position);
    NTempest::C3Vector positionY = GetPosition(m_position);
    NTempest::C3Vector positionX = GetPosition(m_position);
    int                intFacing = static_cast<int>(GetFacing(m_facing));
    SErrDisplayErrorFmt(
        0x85100000,
        __FILE__,
        __LINE__,
        0,
        1,
        "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
        "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
        "elapsedMS > 0",
        m_guid,
        positionX.x,
        positionY.y,
        positionZ.z,
        facing,
        static_cast<int>(intPositionX.x),
        static_cast<int>(intPositionY.y),
        static_cast<int>(intPositionZ.z),
        intFacing
    );
  }

  m_moveFlags &= ~0x00800000;

  while (timeUsed < elapsedMS) {
    unsigned long     updateTime = lastUpdate + timeUsed;
    CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
    CPlayerMoveEvent *event = globals->m_localMoveQueue.m_events.Head();
    unsigned int      timeToUse = elapsedMS - timeUsed;

    if (event) {
      unsigned int untilEvent = event->timeStamp - updateTime;
      if (untilEvent < timeToUse) {
        timeToUse = untilEvent;
      }
      LogWrite(
          "0x%016I64X: first event time(0x%08X), time to consume(%u)\n",
          m_guid,
          event->timeStamp,
          timeToUse
      );
    } else {
      LogWrite("0x%016I64X: no events, consuming(%u)\n", m_guid, timeToUse);
    }

    LogWrite(
        "0x%016I64X: update time(0x%08X) last update(0x%08X) move start(0x%08X) fall start (0x%08X) time used(%u)\n",
        m_guid,
        updateTime,
        lastUpdate,
        m_moveStartTime,
        m_fallStartTime,
        timeUsed
    );

    if ((m_moveFlags & 0x40FF) && timeToUse) {
      unsigned int timeFallen = m_moveFlags & 0x4000 ? updateTime - m_fallStartTime : 0;
      ApplyMovement(updateTime + 1, timeFallen, updateTime + timeToUse - m_moveStartTime, timeToUse);
      m_moveFlags |= 0x00800000;
    }

    if (!UpdatePlayerMovement(updateTime + 1)) {
      LogWrite("0x%016I64X: No more movement and player not in motion\n", m_guid);
      globals->m_localMover = 0;
      break;
    }

    timeUsed += timeToUse;
  }

  OnMoveUpdate(m_guid, timeNow);
}

int MovementIdleMoveUnits(const void *packetData, void *param) {
  MovementLockMoversList(1);

  unsigned long timeNow = OsGetAsyncTimeMs();
  unsigned long lastUpdate = static_cast<CMovementGlobals *>(MovementGetGlobals())->m_lastUpdateTime;
  int           timeDiff = static_cast<int>(timeNow - lastUpdate);

  MovementMoveTransports(timeNow, static_cast<float>(timeDiff) * 0.001f);

  if (static_cast<CMovementGlobals *>(MovementGetGlobals())->m_localMover || CMovement::MoversOnList()) {
    FATALASSERT(timeDiff >= 0);

    if (timeDiff) {
      do {
        lastUpdate = static_cast<CMovementGlobals *>(MovementGetGlobals())->m_lastUpdateTime;

        if (timeDiff > 1000) {
          static_cast<CMovementGlobals *>(MovementGetGlobals())->m_lastUpdateTime = lastUpdate + 1000;
          timeDiff -= 1000;
        } else {
          static_cast<CMovementGlobals *>(MovementGetGlobals())->m_lastUpdateTime = timeNow;
          timeDiff = 0;
          CMovement::MoveUnits(timeNow, lastUpdate);
        }

        if (static_cast<CMovementGlobals *>(MovementGetGlobals())->m_localMover) {
          static_cast<CMovementGlobals *>(MovementGetGlobals())->m_localMover->MoveLocalPlayer(
              static_cast<CMovementGlobals *>(MovementGetGlobals())->m_lastUpdateTime,
              lastUpdate
          );
        }
      } while (timeDiff > 0);
    }
  } else {
    MovementUnlockMoversList(1);
    static_cast<CMovementGlobals *>(MovementGetGlobals())->m_lastUpdateTime = timeNow;
    return 1;
  }

  MovementUnlockMoversList(1);
  return 1;
}

void MovementInitialize(const char *logFileName, bool needLocalHeap) {
  CMovementGlobals *globals = NEW(CMovementGlobals);
  MovementSetGlobals(globals);
  FATALASSERT(globals);

  SStrCopy(globals->logFileName, logFileName, sizeof(globals->logFileName));
  globals->m_localMover = 0;

  if (needLocalHeap && s_localMoveHeap == static_cast<unsigned int>(-1)) {
    s_localMoveHeap = ObjectAllocAddHeap(sizeof(CPlayerMoveEvent), 1024, "CPlayerMoveEvent");
  }

  globals->m_lastUpdateTime = OsGetAsyncTimeMs();
}

void MovementDestroy() {
  CMovement::StopAllLogging();
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  globals->m_localMoveQueue.m_events.UnlinkAll();
  DEL(globals);
  MovementSetGlobals(0);
}

int MovementGetNumMovers() {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  return globals ? globals->numMovers : 0;
}

unsigned long MovementGetLastUpdate() {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  return globals ? globals->m_lastUpdateTime : 0;
}

void CMovement::GetMovingDirection(NTempest::C3Vector *direction) const {
  *direction = m_moveFlags & 2 ? -m_direction : m_direction;
}

void CMovement::GetStrafingDirection(NTempest::C3Vector *direction) const {
  direction->x = m_direction.y;
  direction->y = m_direction.x;
  direction->z = 0.0f;
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
  direction->x = (direction->x + moveDir.x) * 0.70710677f;
  direction->y = (direction->y + moveDir.y) * 0.70710677f;
  direction->z += moveDir.z;
}

void CMovement::GetMovingDirection2d(NTempest::C2Vector *direction) const {
  if (m_moveFlags & 2) {
    direction->x = -m_direction2d.x;
    direction->y = -m_direction2d.y;
  } else {
    *direction = m_direction2d;
  }
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

void CMovement::PlotLinearPosition(const NTempest::C3Vector &direction, float secsElapsed, NTempest::C3Vector *totalMove) {
  *totalMove = (direction * secsElapsed) * GetCurrentSpeed();
  LogWrite(
      "0x%016I64X: Want to move (%g,%g,%g) straight for %g secs\n",
      m_guid,
      totalMove->x,
      totalMove->y,
      totalMove->z,
      secsElapsed
  );
}

void CMovement::PlotNormalLinearPosition(float secsElapsed, NTempest::C3Vector *totalMove) {
  if (!(m_moveFlags & 3)) {
    NTempest::C3Vector intPositionZ = GetPosition(m_position);
    NTempest::C3Vector intPositionY = GetPosition(m_position);
    NTempest::C3Vector intPositionX = GetPosition(m_position);
    float              facing = GetFacing(m_facing);
    NTempest::C3Vector positionZ = GetPosition(m_position);
    NTempest::C3Vector positionY = GetPosition(m_position);
    NTempest::C3Vector positionX = GetPosition(m_position);
    int                intFacing = static_cast<int>(GetFacing(m_facing));
    SErrDisplayErrorFmt(
        0x85100000,
        __FILE__,
        __LINE__,
        0,
        1,
        "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
        "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
        "IsMoving()",
        m_guid,
        positionX.x,
        positionY.y,
        positionZ.z,
        facing,
        static_cast<int>(intPositionX.x),
        static_cast<int>(intPositionY.y),
        static_cast<int>(intPositionZ.z),
        intFacing
    );
  }

  NTempest::C3Vector direction;
  GetMovingDirection(&direction);
  PlotLinearPosition(direction, secsElapsed, totalMove);
}

void CMovement::PlotStrafeLinearPosition(float secsElapsed, NTempest::C3Vector *totalMove) {
  if (!(m_moveFlags & 0xC)) {
    NTempest::C3Vector intPositionZ = GetPosition(m_position);
    NTempest::C3Vector intPositionY = GetPosition(m_position);
    NTempest::C3Vector intPositionX = GetPosition(m_position);
    float              facing = GetFacing(m_facing);
    NTempest::C3Vector positionZ = GetPosition(m_position);
    NTempest::C3Vector positionY = GetPosition(m_position);
    NTempest::C3Vector positionX = GetPosition(m_position);
    int                intFacing = static_cast<int>(GetFacing(m_facing));
    SErrDisplayErrorFmt(
        0x85100000,
        __FILE__,
        __LINE__,
        0,
        1,
        "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
        "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
        "IsStrafing()",
        m_guid,
        positionX.x,
        positionY.y,
        positionZ.z,
        facing,
        static_cast<int>(intPositionX.x),
        static_cast<int>(intPositionY.y),
        static_cast<int>(intPositionZ.z),
        intFacing
    );
  }

  NTempest::C3Vector direction;
  GetStrafingDirection(&direction);
  PlotLinearPosition(direction, secsElapsed, totalMove);
}

void CMovement::PlotDiagonalLinearPosition(float secsElapsed, NTempest::C3Vector *totalMove) {
  if (!(m_moveFlags & 3)) {
    NTempest::C3Vector intPositionZ = GetPosition(m_position);
    NTempest::C3Vector intPositionY = GetPosition(m_position);
    NTempest::C3Vector intPositionX = GetPosition(m_position);
    float              facing = GetFacing(m_facing);
    NTempest::C3Vector positionZ = GetPosition(m_position);
    NTempest::C3Vector positionY = GetPosition(m_position);
    NTempest::C3Vector positionX = GetPosition(m_position);
    int                intFacing = static_cast<int>(GetFacing(m_facing));
    SErrDisplayErrorFmt(
        0x85100000,
        __FILE__,
        __LINE__,
        0,
        1,
        "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
        "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
        "IsMoving()",
        m_guid,
        positionX.x,
        positionY.y,
        positionZ.z,
        facing,
        static_cast<int>(intPositionX.x),
        static_cast<int>(intPositionY.y),
        static_cast<int>(intPositionZ.z),
        intFacing
    );
  }
  if (!(m_moveFlags & 0xC)) {
    NTempest::C3Vector intPositionZ = GetPosition(m_position);
    NTempest::C3Vector intPositionY = GetPosition(m_position);
    NTempest::C3Vector intPositionX = GetPosition(m_position);
    float              facing = GetFacing(m_facing);
    NTempest::C3Vector positionZ = GetPosition(m_position);
    NTempest::C3Vector positionY = GetPosition(m_position);
    NTempest::C3Vector positionX = GetPosition(m_position);
    int                intFacing = static_cast<int>(GetFacing(m_facing));
    SErrDisplayErrorFmt(
        0x85100000,
        __FILE__,
        __LINE__,
        0,
        1,
        "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
        "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
        "IsStrafing()",
        m_guid,
        positionX.x,
        positionY.y,
        positionZ.z,
        facing,
        static_cast<int>(intPositionX.x),
        static_cast<int>(intPositionY.y),
        static_cast<int>(intPositionZ.z),
        intFacing
    );
  }

  NTempest::C3Vector direction;
  GetDiagonalDirection(&direction);
  PlotLinearPosition(direction, secsElapsed, totalMove);
}

void CMovement::PlotUnitRotation(float elapsedSec) {
  if (!(m_moveFlags & 0x30)) {
    NTempest::C3Vector iz = GetPosition(m_position), iy = GetPosition(m_position), ix = GetPosition(m_position);
    float f = GetFacing(m_facing);
    NTempest::C3Vector pz = GetPosition(m_position), py = GetPosition(m_position), px = GetPosition(m_position);
    int fi = static_cast<int>(GetFacing(m_facing));
    SErrDisplayErrorFmt(
        0x85100000, __FILE__, __LINE__, 0, 1,
        "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
        "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
        "IsTurning()", m_guid, px.x, py.y, pz.z, f,
        static_cast<int>(ix.x), static_cast<int>(iy.y), static_cast<int>(iz.z), fi
    );
  }

  float turnRate = GetCurrentTurnRate();
  float facing = static_cast<float>(fmod(turnRate * elapsedSec + m_anchorFacing, 6.2831855));
  if (facing < 0.0f) {
    facing += 6.2831855f;
  }
  LogWrite("0x%016I64X: Turning for %g secs, from (%g) to (%g)\n", m_guid, elapsedSec, m_facing, facing);
  m_facing = facing;
}

void CMovement::PlotUnitPitch(float elapsedSec) {
  if (!(m_moveFlags & 0xC0)) {
    NTempest::C3Vector iz = GetPosition(m_position), iy = GetPosition(m_position), ix = GetPosition(m_position);
    float f = GetFacing(m_facing);
    NTempest::C3Vector pz = GetPosition(m_position), py = GetPosition(m_position), px = GetPosition(m_position);
    int fi = static_cast<int>(GetFacing(m_facing));
    SErrDisplayErrorFmt(
        0x85100000, __FILE__, __LINE__, 0, 1,
        "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
        "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
        "IsPitching()", m_guid, px.x, py.y, pz.z, f,
        static_cast<int>(ix.x), static_cast<int>(iy.y), static_cast<int>(iz.z), fi
    );
  }

  float pitch = GetCurrentPitchRate() * elapsedSec + m_anchorPitch;
  if (pitch < -1.5707964f) {
    pitch = -1.5707964f;
  } else if (pitch > 1.5707964f) {
    pitch = 1.5707964f;
  }
  LogWrite("0x%016I64X: Pitching for %g secs, from (%g) to (%g)\n", m_guid, elapsedSec, m_pitch, pitch);
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
  float overflowTime = 0.0f;

  if (m_anchorPitch + pitchChange > 1.5707964f) {
    overflowTime = (m_anchorPitch + pitchChange - 1.5707964f) * inversePitchRate;
    pitchChange = 1.5707964f - m_anchorPitch;
  } else if (m_anchorPitch + pitchChange < -1.5707964f) {
    overflowTime = (m_anchorPitch + pitchChange + 1.5707964f) * inversePitchRate;
    pitchChange = -1.5707964f - m_anchorPitch;
  }

  float remainingTurn = (secsElapsed - overflowTime) * turnRate;
  float horizontal = (NTempest::CMath::sin_(pitchChange) * pitchRadius + NTempest::CMath::sin_(remainingTurn) * turnRadius) * 0.5f;
  float turnOffset = NTempest::CMath::cos_(remainingTurn) * turnRadius - turnRadius;
  float pitchOffset = pitchRadius - NTempest::CMath::cos_(pitchChange) * pitchRadius;
  float baseX = horizontal * direction2d.x - turnOffset * direction2d.y;
  float baseY = turnOffset * direction2d.x + horizontal * direction2d.y;

  totalMove->x = baseX * m_cosAnchorPitch - pitchOffset * m_sinAnchorPitch;
  totalMove->y = baseY;
  totalMove->z = pitchOffset * m_cosAnchorPitch + baseX * m_sinAnchorPitch;
  if (NTempest::CMath::fabs_(overflowTime) >= 0.00000095367432f) {
    float overflow = overflowTime * currentSpeed;
    totalMove->z += overflowTime > 0.0f ? overflow : -overflow;
  }
}

void CMovement::PlotVertCircularPosition(const NTempest::C2Vector &direction2d, float secsElapsed, NTempest::C3Vector *totalMove) {
  float currentSpeed = GetCurrentSpeed();
  float pitchRate = GetCurrentPitchRate();
  float pitchRadius = currentSpeed / pitchRate;
  float pitchAngle = pitchRate * secsElapsed;
  float overflowTime = 0.0f;

  if (m_anchorPitch + pitchAngle > 1.5707964f) {
    overflowTime = (m_anchorPitch + pitchAngle - 1.5707964f) / pitchRate;
    pitchAngle = 1.5707964f - m_anchorPitch;
  } else if (m_anchorPitch + pitchAngle < -1.5707964f) {
    overflowTime = (m_anchorPitch + pitchAngle + 1.5707964f) / pitchRate;
    pitchAngle = -1.5707964f - m_anchorPitch;
  }

  float horizontal = NTempest::CMath::sin_(pitchAngle) * pitchRadius;
  float vertical = pitchRadius - NTempest::CMath::cos_(pitchAngle) * pitchRadius;
  float projected = horizontal * m_cosAnchorPitch - vertical * m_sinAnchorPitch;
  totalMove->x = projected * direction2d.x;
  totalMove->y = projected * direction2d.y;
  totalMove->z = vertical * m_cosAnchorPitch + horizontal * m_sinAnchorPitch;
  if (NTempest::CMath::fabs_(overflowTime) >= 0.00000095367432f) {
    float overflow = overflowTime * currentSpeed;
    totalMove->z += overflowTime > 0.0f ? overflow : -overflow;
  }
  LogWrite(
      "0x%016I64X: Want to move (%g,%g,%g) arcing for %g secs\n",
      m_guid,
      totalMove->x,
      totalMove->y,
      totalMove->z,
      secsElapsed
  );
}

void CMovement::PlotHorzCircularPosition(const NTempest::C2Vector &direction2d, float secsElapsed, NTempest::C3Vector *totalMove) {
  float currentSpeed = GetCurrentSpeed();
  float turnRate = GetCurrentTurnRate();
  float turnX = NTempest::CMath::sin_(turnRate * secsElapsed) * (currentSpeed / turnRate);
  float turnY = currentSpeed / turnRate - NTempest::CMath::cos_(turnRate * secsElapsed) * (currentSpeed / turnRate);

  totalMove->x = (turnX * direction2d.x - turnY * direction2d.y) * m_cosAnchorPitch;
  totalMove->y = (turnY * direction2d.x + turnX * direction2d.y) * m_cosAnchorPitch;
  float totalZ = currentSpeed * m_sinAnchorPitch * secsElapsed;
  totalMove->z = totalZ;
  LogWrite(
      "0x%016I64X: Want to move (%g,%g,%g) arcing for %g secs\n",
      m_guid,
      totalMove->x,
      totalMove->y,
      totalZ,
      secsElapsed
  );
}

void CMovement::PlotNormalCircularPosition(float secsElapsed, NTempest::C3Vector *totalMove) {
  if (!(m_moveFlags & 0x30)) {
    NTempest::C3Vector iz = GetPosition(m_position), iy = GetPosition(m_position), ix = GetPosition(m_position);
    float f = GetFacing(m_facing);
    NTempest::C3Vector pz = GetPosition(m_position), py = GetPosition(m_position), px = GetPosition(m_position);
    int fi = static_cast<int>(GetFacing(m_facing));
    SErrDisplayErrorFmt(
        0x85100000, __FILE__, __LINE__, 0, 1,
        "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
        "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
        "IsTurning()", m_guid, px.x, py.y, pz.z, f,
        static_cast<int>(ix.x), static_cast<int>(iy.y), static_cast<int>(iz.z), fi
    );
  }
  if (!(m_moveFlags & 3)) {
    NTempest::C3Vector iz = GetPosition(m_position), iy = GetPosition(m_position), ix = GetPosition(m_position);
    float f = GetFacing(m_facing);
    NTempest::C3Vector pz = GetPosition(m_position), py = GetPosition(m_position), px = GetPosition(m_position);
    int fi = static_cast<int>(GetFacing(m_facing));
    SErrDisplayErrorFmt(
        0x85100000, __FILE__, __LINE__, 0, 1,
        "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
        "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
        "IsMoving()", m_guid, px.x, py.y, pz.z, f,
        static_cast<int>(ix.x), static_cast<int>(iy.y), static_cast<int>(iz.z), fi
    );
  }

  NTempest::C2Vector direction;
  GetMovingDirection2d(&direction);
  PlotHorzCircularPosition(direction, secsElapsed, totalMove);
}

void CMovement::PlotStrafeCircularPosition(float secsElapsed, NTempest::C3Vector *totalMove) {
  if (!(m_moveFlags & 0x30)) {
    NTempest::C3Vector iz = GetPosition(m_position), iy = GetPosition(m_position), ix = GetPosition(m_position);
    float f = GetFacing(m_facing);
    NTempest::C3Vector pz = GetPosition(m_position), py = GetPosition(m_position), px = GetPosition(m_position);
    int fi = static_cast<int>(GetFacing(m_facing));
    SErrDisplayErrorFmt(
        0x85100000, __FILE__, __LINE__, 0, 1,
        "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
        "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
        "IsTurning()", m_guid, px.x, py.y, pz.z, f,
        static_cast<int>(ix.x), static_cast<int>(iy.y), static_cast<int>(iz.z), fi
    );
  }
  if (!(m_moveFlags & 0xC)) {
    NTempest::C3Vector iz = GetPosition(m_position), iy = GetPosition(m_position), ix = GetPosition(m_position);
    float f = GetFacing(m_facing);
    NTempest::C3Vector pz = GetPosition(m_position), py = GetPosition(m_position), px = GetPosition(m_position);
    int fi = static_cast<int>(GetFacing(m_facing));
    SErrDisplayErrorFmt(
        0x85100000, __FILE__, __LINE__, 0, 1,
        "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
        "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
        "IsStrafing()", m_guid, px.x, py.y, pz.z, f,
        static_cast<int>(ix.x), static_cast<int>(iy.y), static_cast<int>(iz.z), fi
    );
  }

  NTempest::C2Vector direction;
  GetStrafingDirection2d(&direction);
  PlotHorzCircularPosition(direction, secsElapsed, totalMove);
}

void CMovement::PlotDiagonalCircularPosition(float secsElapsed, NTempest::C3Vector *totalMove) {
  if (!(m_moveFlags & 0x30)) {
    NTempest::C3Vector iz = GetPosition(m_position), iy = GetPosition(m_position), ix = GetPosition(m_position);
    float f = GetFacing(m_facing);
    NTempest::C3Vector pz = GetPosition(m_position), py = GetPosition(m_position), px = GetPosition(m_position);
    int fi = static_cast<int>(GetFacing(m_facing));
    SErrDisplayErrorFmt(
        0x85100000, __FILE__, __LINE__, 0, 1,
        "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
        "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
        "IsTurning()", m_guid, px.x, py.y, pz.z, f,
        static_cast<int>(ix.x), static_cast<int>(iy.y), static_cast<int>(iz.z), fi
    );
  }
  if (!(m_moveFlags & 3)) {
    NTempest::C3Vector iz = GetPosition(m_position), iy = GetPosition(m_position), ix = GetPosition(m_position);
    float f = GetFacing(m_facing);
    NTempest::C3Vector pz = GetPosition(m_position), py = GetPosition(m_position), px = GetPosition(m_position);
    int fi = static_cast<int>(GetFacing(m_facing));
    SErrDisplayErrorFmt(
        0x85100000, __FILE__, __LINE__, 0, 1,
        "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
        "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
        "IsMoving()", m_guid, px.x, py.y, pz.z, f,
        static_cast<int>(ix.x), static_cast<int>(iy.y), static_cast<int>(iz.z), fi
    );
  }
  if (!(m_moveFlags & 0xC)) {
    NTempest::C3Vector iz = GetPosition(m_position), iy = GetPosition(m_position), ix = GetPosition(m_position);
    float f = GetFacing(m_facing);
    NTempest::C3Vector pz = GetPosition(m_position), py = GetPosition(m_position), px = GetPosition(m_position);
    int fi = static_cast<int>(GetFacing(m_facing));
    SErrDisplayErrorFmt(
        0x85100000, __FILE__, __LINE__, 0, 1,
        "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
        "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
        "IsStrafing()", m_guid, px.x, py.y, pz.z, f,
        static_cast<int>(ix.x), static_cast<int>(iy.y), static_cast<int>(iz.z), fi
    );
  }

  NTempest::C2Vector direction;
  GetDiagonalDirection2d(&direction);
  PlotHorzCircularPosition(direction, secsElapsed, totalMove);
}

void CMovement::PlotNormalPitchingCircularPosition(float secsElapsed, NTempest::C3Vector *totalMove) {
  if (!(m_moveFlags & 3)) {
    NTempest::C3Vector iz = GetPosition(m_position), iy = GetPosition(m_position), ix = GetPosition(m_position);
    float f = GetFacing(m_facing);
    NTempest::C3Vector pz = GetPosition(m_position), py = GetPosition(m_position), px = GetPosition(m_position);
    int fi = static_cast<int>(GetFacing(m_facing));
    SErrDisplayErrorFmt(
        0x85100000, __FILE__, __LINE__, 0, 1,
        "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
        "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
        "IsMoving()", m_guid, px.x, py.y, pz.z, f,
        static_cast<int>(ix.x), static_cast<int>(iy.y), static_cast<int>(iz.z), fi
    );
  }
  if (!(m_moveFlags & 0xC0)) {
    NTempest::C3Vector iz = GetPosition(m_position), iy = GetPosition(m_position), ix = GetPosition(m_position);
    float f = GetFacing(m_facing);
    NTempest::C3Vector pz = GetPosition(m_position), py = GetPosition(m_position), px = GetPosition(m_position);
    int fi = static_cast<int>(GetFacing(m_facing));
    SErrDisplayErrorFmt(
        0x85100000, __FILE__, __LINE__, 0, 1,
        "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
        "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
        "IsPitching()", m_guid, px.x, py.y, pz.z, f,
        static_cast<int>(ix.x), static_cast<int>(iy.y), static_cast<int>(iz.z), fi
    );
  }

  NTempest::C2Vector direction;
  GetMovingDirection2d(&direction);
  PlotVertCircularPosition(direction, secsElapsed, totalMove);
}

void CMovement::PlotDiagonalPitchingCircularPosition(float secsElapsed, NTempest::C3Vector *totalMove) {
  if (!(m_moveFlags & 3)) {
    NTempest::C3Vector iz = GetPosition(m_position), iy = GetPosition(m_position), ix = GetPosition(m_position);
    float f = GetFacing(m_facing);
    NTempest::C3Vector pz = GetPosition(m_position), py = GetPosition(m_position), px = GetPosition(m_position);
    int fi = static_cast<int>(GetFacing(m_facing));
    SErrDisplayErrorFmt(
        0x85100000, __FILE__, __LINE__, 0, 1,
        "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
        "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
        "IsMoving()", m_guid, px.x, py.y, pz.z, f,
        static_cast<int>(ix.x), static_cast<int>(iy.y), static_cast<int>(iz.z), fi
    );
  }
  if (!(m_moveFlags & 0xC0)) {
    NTempest::C3Vector iz = GetPosition(m_position), iy = GetPosition(m_position), ix = GetPosition(m_position);
    float f = GetFacing(m_facing);
    NTempest::C3Vector pz = GetPosition(m_position), py = GetPosition(m_position), px = GetPosition(m_position);
    int fi = static_cast<int>(GetFacing(m_facing));
    SErrDisplayErrorFmt(
        0x85100000, __FILE__, __LINE__, 0, 1,
        "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
        "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
        "IsPitching()", m_guid, px.x, py.y, pz.z, f,
        static_cast<int>(ix.x), static_cast<int>(iy.y), static_cast<int>(iz.z), fi
    );
  }

  NTempest::C2Vector direction;
  GetDiagonalDirection2d(&direction);
  PlotVertCircularPosition(direction, secsElapsed, totalMove);
}

void CMovement::PlotNormalSpiralPosition(float secsElapsed, NTempest::C3Vector *totalMove) {
  if (!(m_moveFlags & 3)) {
    NTempest::C3Vector iz = GetPosition(m_position), iy = GetPosition(m_position), ix = GetPosition(m_position);
    float f = GetFacing(m_facing);
    NTempest::C3Vector pz = GetPosition(m_position), py = GetPosition(m_position), px = GetPosition(m_position);
    int fi = static_cast<int>(GetFacing(m_facing));
    SErrDisplayErrorFmt(
        0x85100000, __FILE__, __LINE__, 0, 1,
        "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
        "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
        "IsMoving()", m_guid, px.x, py.y, pz.z, f,
        static_cast<int>(ix.x), static_cast<int>(iy.y), static_cast<int>(iz.z), fi
    );
  }
  if (!(m_moveFlags & 0xC0)) {
    NTempest::C3Vector iz = GetPosition(m_position), iy = GetPosition(m_position), ix = GetPosition(m_position);
    float f = GetFacing(m_facing);
    NTempest::C3Vector pz = GetPosition(m_position), py = GetPosition(m_position), px = GetPosition(m_position);
    int fi = static_cast<int>(GetFacing(m_facing));
    SErrDisplayErrorFmt(
        0x85100000, __FILE__, __LINE__, 0, 1,
        "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
        "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
        "IsPitching()", m_guid, px.x, py.y, pz.z, f,
        static_cast<int>(ix.x), static_cast<int>(iy.y), static_cast<int>(iz.z), fi
    );
  }

  NTempest::C2Vector direction;
  GetMovingDirection2d(&direction);
  PlotSpiralPosition(direction, secsElapsed, totalMove);
}

void CMovement::PlotDiagonalSpiralPosition(float secsElapsed, NTempest::C3Vector *totalMove) {
  if (!(m_moveFlags & 3)) {
    NTempest::C3Vector iz = GetPosition(m_position), iy = GetPosition(m_position), ix = GetPosition(m_position);
    float f = GetFacing(m_facing);
    NTempest::C3Vector pz = GetPosition(m_position), py = GetPosition(m_position), px = GetPosition(m_position);
    int fi = static_cast<int>(GetFacing(m_facing));
    SErrDisplayErrorFmt(
        0x85100000, __FILE__, __LINE__, 0, 1,
        "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
        "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
        "IsMoving()", m_guid, px.x, py.y, pz.z, f,
        static_cast<int>(ix.x), static_cast<int>(iy.y), static_cast<int>(iz.z), fi
    );
  }
  if (!(m_moveFlags & 0xC0)) {
    NTempest::C3Vector iz = GetPosition(m_position), iy = GetPosition(m_position), ix = GetPosition(m_position);
    float f = GetFacing(m_facing);
    NTempest::C3Vector pz = GetPosition(m_position), py = GetPosition(m_position), px = GetPosition(m_position);
    int fi = static_cast<int>(GetFacing(m_facing));
    SErrDisplayErrorFmt(
        0x85100000, __FILE__, __LINE__, 0, 1,
        "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
        "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
        "IsPitching()", m_guid, px.x, py.y, pz.z, f,
        static_cast<int>(ix.x), static_cast<int>(iy.y), static_cast<int>(iz.z), fi
    );
  }

  NTempest::C2Vector direction;
  GetDiagonalDirection2d(&direction);
  PlotSpiralPosition(direction, secsElapsed, totalMove);
}

int CMovement::PlotUnitMovement(unsigned int moveTime, NTempest::C3Vector *move) {
  if (!move) {
    if (this) {
      NTempest::C3Vector intPositionZ = GetPosition(m_position);
      NTempest::C3Vector intPositionY = GetPosition(m_position);
      NTempest::C3Vector intPositionX = GetPosition(m_position);
      float              facing = GetFacing(m_facing);
      NTempest::C3Vector positionZ = GetPosition(m_position);
      NTempest::C3Vector positionY = GetPosition(m_position);
      NTempest::C3Vector positionX = GetPosition(m_position);
      int                intFacing = static_cast<int>(GetFacing(m_facing));
      SErrDisplayErrorFmt(
          0x85100000,
          __FILE__,
          __LINE__,
          0,
          1,
          "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
          "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
          "move",
          m_guid,
          positionX.x,
          positionY.y,
          positionZ.z,
          facing,
          static_cast<int>(intPositionX.x),
          static_cast<int>(intPositionY.y),
          static_cast<int>(intPositionZ.z),
          intFacing
      );
    } else {
      SErrDisplayError(0x85100000, __FILE__, __LINE__, "move", 0, 1);
    }
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

  float        secsElapsed = static_cast<float>(moveTime) * 0.001f;
  unsigned int actionFlags = 0;
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

int CMovement::PlotUnitSplineMovement(unsigned long eventTime, NTempest::C3Vector *move) {
  if (!move) {
    if (this) {
      NTempest::C3Vector intPositionZ = GetPosition(m_position);
      NTempest::C3Vector intPositionY = GetPosition(m_position);
      NTempest::C3Vector intPositionX = GetPosition(m_position);
      float              facing = GetFacing(m_facing);
      NTempest::C3Vector positionZ = GetPosition(m_position);
      NTempest::C3Vector positionY = GetPosition(m_position);
      NTempest::C3Vector positionX = GetPosition(m_position);
      int                intFacing = static_cast<int>(GetFacing(m_facing));
      SErrDisplayErrorFmt(
          0x85100000,
          __FILE__,
          __LINE__,
          0,
          1,
          "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
          "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
          "move",
          m_guid,
          positionX.x,
          positionY.y,
          positionZ.z,
          facing,
          static_cast<int>(intPositionX.x),
          static_cast<int>(intPositionY.y),
          static_cast<int>(intPositionZ.z),
          intFacing
      );
    } else {
      SErrDisplayError(0x85100000, __FILE__, __LINE__, "move", 0, 1);
    }
  }
  if (!IsSplineMover()) {
    SErrDisplayErrorFmt(
        0x85100000,
        __FILE__,
        __LINE__,
        0,
        1,
        "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
        "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
        "IsSplineMover()",
        m_guid,
        GetPosition(m_position).x,
        GetPosition(m_position).y,
        GetPosition(m_position).z,
        GetFacing(m_facing),
        static_cast<int>(GetPosition(m_position).x),
        static_cast<int>(GetPosition(m_position).y),
        static_cast<int>(GetPosition(m_position).z),
        static_cast<int>(GetFacing(m_facing))
    );
  }
  if (!(m_moveFlags & 3)) {
    return 0;
  }

  if (!(m_spline->flags & 0x80000000)) {
    int   timeUsed = eventTime - m_spline->start;
    float time;
    if (!m_spline->time) {
      time = 1.0f;
    } else if (timeUsed < 0) {
      time = 0.0f;
    } else if (static_cast<unsigned int>(timeUsed) < m_spline->time) {
      time = static_cast<float>(timeUsed) / m_spline->time;
    } else {
      time = 1.0f;
      m_spline->flags |= 1;
    }

    NTempest::C34Matrix matrix;
    matrix.a0 = m_direction.x;
    matrix.a1 = m_direction.y;
    matrix.a2 = m_direction.z;
    m_spline->spline.Frame(time, matrix, NTempest::C3Spline::EVAL_ARCLENGTH);

    m_facing = static_cast<float>(atan2(matrix.a1, matrix.a0));
    if (m_facing < 0.0f) {
      m_facing += 6.2831855f;
    }
    m_direction.Set(matrix.a0, matrix.a1, matrix.a2);

    if (m_spline->flags & 0x200) {
      NTempest::C34Matrix futureMatrix;
      futureMatrix.a0 = m_direction.x;
      futureMatrix.a1 = m_direction.y;
      futureMatrix.a2 = m_direction.z;

      float futureTime = static_cast<float>(timeUsed + 1000) / m_spline->time;
      if (futureTime < 0.0f) {
        futureTime = 0.0f;
      } else if (futureTime > 1.0f) {
        futureTime = 1.0f;
      }
      m_spline->spline.Frame(futureTime, futureMatrix, NTempest::C3Spline::EVAL_ARCLENGTH);

      NTempest::C2Vector currentFacing(m_direction.x, m_direction.y);
      NTempest::C3Vector position = GetPosition(m_position);
      NTempest::C2Vector futureFacing(futureMatrix.d0 - position.x, futureMatrix.d1 - position.y);
      float              magnitude = currentFacing.Mag();
      if (NTempest::CMath::fabs_(magnitude) >= 0.00000023841858f) {
        currentFacing /= magnitude;
      }
      magnitude = futureFacing.Mag();
      if (NTempest::CMath::fabs_(magnitude) >= 0.00000023841858f) {
        futureFacing /= magnitude;
      }

      float roll = futureFacing.x * currentFacing.x + futureFacing.y * currentFacing.y;
      if (roll < -1.0f) {
        roll = -1.0f;
      } else if (roll > 1.0f) {
        roll = 1.0f;
      }
      if (futureFacing.y * currentFacing.x - currentFacing.y * futureFacing.x < 0.0f) {
        roll = static_cast<float>(acos(roll));
      } else {
        roll = -static_cast<float>(acos(roll));
      }
      roll += roll;
      if (roll < -1.5707964f) {
        roll = -1.5707964f;
      } else if (roll > 1.5707964f) {
        roll = 1.5707964f;
      }

      NTempest::C33Matrix rollMatrix = NTempest::C33Matrix::Rotation(roll, m_direction, 0);
      m_groundNormal = rollMatrix * NTempest::C3Vector(matrix.c0, matrix.c1, matrix.c2);
    }

    move->Set(matrix.d0 - m_anchorPosition.x, matrix.d1 - m_anchorPosition.y, matrix.d2 - m_anchorPosition.z);
  }
  return 1;
}

int CMovement::CheckInvalidPositionOrMove(const NTempest::C3Vector &move, unsigned int moveTime) {
  if (_isnan(m_position.x) || _isnan(m_position.y) || m_position.x - 2.0f < -17066.666f || m_position.x + 2.0f > 17066.666f ||
      m_position.y - 2.0f < -17066.666f || m_position.y + 2.0f > 17066.666f)
  {
    MovementFixOutOfBoundsUnit(m_guid);
    return 0;
  }

  NTempest::C3Vector moveVector(
      m_anchorPosition.x + move.x - m_position.x, m_anchorPosition.y + move.y - m_position.y, m_anchorPosition.z + move.z - m_position.z
  );
  float distanceSq = moveVector.x * moveVector.x + moveVector.y * moveVector.y;
  if (m_moveFlags & 0x02000000) {
    distanceSq += moveVector.z * moveVector.z;
  }

  if (moveTime) {
    float seconds = static_cast<float>(moveTime) * 0.001f;
    if (distanceSq / (seconds * seconds) <= 3600.0f) {
      return 1;
    }
  } else if (distanceSq < 0.00000095367432f) {
    return 1;
  }

  MovementFixOutOfBoundsUnit(m_guid);
  return 0;
}

void CMovement::ApplyAdjustedMove(
    unsigned long timeStemp, const NTempest::C3Vector &moveWanted, int wasAdjusted, unsigned int oldMoveFlags
) {
  if (wasAdjusted || (m_moveFlags & 0x1000)) {
    if (!m_spline || (m_spline->flags & 4) || (oldMoveFlags & 0x1000)) {
      UpdateAnchors(timeStemp);
      return;
    }

    m_spline->flags |= 2;
    m_direction = moveWanted;
    float length = m_direction.Mag();
    if (NTempest::CMath::fabs_(length) >= 0.00000023841858f) {
      m_direction.x /= length;
      m_direction.y /= length;
      m_direction.z /= length;
    }
    UpdateAnchors(timeStemp);
  } else if (m_spline && !(m_spline->flags & 4)) {
    m_spline->flags &= ~2U;
  }
}

void CMovement::SimpleRequestMove(unsigned int fallTime, const NTempest::C3Vector &moveVector) {
  m_position = m_anchorPosition + moveVector;
  if (m_jumpVelocity != 0.0f) {
    float distJumped = static_cast<float>(fallTime) * 0.001f * m_jumpVelocity * -0.5f;
    if (distJumped > 1.0f) {
      distJumped = 1.0f;
      StopFalling();
    }
    m_position.z = m_fallStartElevation + distJumped;
  }
}

void CMovement::ApplyMovement(unsigned long eventTime, unsigned int fallTime, unsigned int moveTime, unsigned int elapsed) {
  NTempest::C3Vector moveVector(0.0f);
  if (!m_spline || (m_spline->flags & 6)) {
    PlotUnitMovement(moveTime, &moveVector);
  } else {
    PlotUnitSplineMovement(eventTime + elapsed, &moveVector);
  }

  if (!(m_moveFlags & 0x400F) || !CheckInvalidPositionOrMove(moveVector, moveTime)) {
    return;
  }

  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  FATALASSERT(globals);
  unsigned int oldMoveFlags = m_moveFlags;
  int          wasJumping = m_jumpVelocity != 0.0f;
  int          moveAdjusted = 0;

  if (globals->ignoreObstacles) {
    m_position = m_anchorPosition + moveVector;
  } else if ((oldMoveFlags & 0x800) || (m_spline && !(m_spline->flags & 4) && (m_spline->flags & 0x200))) {
    SimpleRequestMove(fallTime + elapsed, moveVector);
  } else if (oldMoveFlags & 0x400F) {
    NTempest::C3Vector move = m_anchorPosition + moveVector - m_position;
    m_moveFlags = oldMoveFlags & 0xEFFFFFFF;
    if (!(oldMoveFlags & 0x02000000)) {
      move.z = 0.0f;
    }
    moveAdjusted = CollideRequestMove(eventTime, elapsed, move);
    ApplyAdjustedMove(eventTime + elapsed, moveVector, moveAdjusted, oldMoveFlags);
  }

  CallMoveEventHandlers(eventTime, moveAdjusted, oldMoveFlags, wasJumping);
  if (m_spline && !(m_spline->flags & 4) && (m_spline->flags & 0x200)) {
    UpdateAnchors(eventTime + elapsed);
  }
  if (m_position.z < -333.33334f) {
    MovementFixOutOfBoundsUnit(m_guid);
  }
}

CMovementData::CMovementData(const NTempest::C3Vector &position, float facing, const unsigned __int64 &guid)
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

int CMovementData::ForceSetTransport(unsigned __int64 guid) {
  if (guid == m_transportGUID || (guid && !MovementGameObjIsTransport(guid))) {
    return 0;
  }

  NTempest::C34Matrix transportMtx;
  float               transportFacing;
  if (guid) {
    MovementGetTransportMtx(guid, &transportMtx);
    transportFacing = MovementGetTransportFacing(guid);
    transportMtx = transportMtx.AffineInverse();
    m_position = m_position * transportMtx;
    m_anchorPosition = m_anchorPosition * transportMtx;
    m_facing -= transportFacing;
    m_anchorFacing -= transportFacing;
    // TWO_PI
    if (m_facing > 6.2831855f) {
      m_facing -= 6.2831855f;
    } else if (m_facing < 0.0f) {
      m_facing += 6.2831855f;
    }
    if (m_anchorFacing > 6.2831855f) {
      m_anchorFacing -= 6.2831855f;
    } else if (m_anchorFacing < 0.0f) {
      m_anchorFacing += 6.2831855f;
    }
    m_fallStartElevation = transportMtx.c2 * m_fallStartElevation + transportMtx.d2;
    MovementAddToTransport(this, guid);
  } else {
    MovementGetTransportMtx(m_transportGUID, &transportMtx);
    transportFacing = MovementGetTransportFacing(m_transportGUID);
    m_position = m_position * transportMtx;
    m_anchorPosition = m_anchorPosition * transportMtx;
    m_facing += transportFacing;
    m_anchorFacing += transportFacing;
    // TWO_PI
    if (m_facing > 6.2831855f) {
      m_facing -= 6.2831855f;
    } else if (m_facing < 0.0f) {
      m_facing += 6.2831855f;
    }
    if (m_anchorFacing > 6.2831855f) {
      m_anchorFacing -= 6.2831855f;
    } else if (m_anchorFacing < 0.0f) {
      m_anchorFacing += 6.2831855f;
    }
    m_fallStartElevation = transportMtx.c2 * m_fallStartElevation + transportMtx.d2;
    transportLink.Unlink();
  }

  MovementFixUpMoveHistory(m_guid, transportMtx);
  m_transportGUID = guid;
  if (IsLocalPlayer()) {
    MovementUpdateCameraYaw(guid);
    ProcessLocalMoveEvent(MSG_MOVE_HEARTBEAT);
  }
  return 1;
}

int CMovementData::SetTransport(unsigned __int64 guid) {
  if (!guid && m_transportGUID && MovementInsideTransport(m_transportGUID, m_position)) {
    return 0;
  }
  return ForceSetTransport(guid);
}

void CMovement::UpdateAnchors(unsigned long eventTime) {
  m_anchorPosition = m_position;
  m_anchorFacing = m_facing;
  m_anchorPitch = m_pitch;
  m_moveStartTime = eventTime;
  m_moveFlags |= 0x200;
  CalcDirection();
  MovementNotifyZoneMgr(m_guid);
}

int CMovement::StartMove(unsigned long eventTime, int forward) {
  m_moveFlags &= 0xFFE6FFFF;
  if ((m_moveFlags & 0x4000) && (m_moveFlags & 0xF)) {
    m_moveFlags |= forward ? 0x00080000 : 0x00100000;
    return 0;
  }

  if (m_moveFlags & 0x4000) {
    m_moveFlags |= 0x20000000;
  }
  UpdateAnchors(eventTime);
  m_moveFlags = (m_moveFlags & ~3U) | (forward ? 1U : 2U);
  m_moveFlags |= 0x08000000;
  return 1;
}

void CMovement::AddPlayerMoveEvent(unsigned long eventTime, int eventType, float facing) {
  FATALASSERT(static_cast<unsigned int>(eventType) < NUM_PMOVE_EVTS);
  RemoveSpline();
  FATALASSERT(s_localMoveHeap != static_cast<unsigned int>(-1));

  unsigned int memHandle;
  FATALASSERT(ObjectAlloc(s_localMoveHeap, &memHandle));
  CPlayerMoveEvent *event = new (ObjectPtr(memHandle)) CPlayerMoveEvent;
  FATALASSERT(event);

  event->timeStamp = eventTime;
  event->eventType = static_cast<PLAYER_MOVE_EVT>(eventType);
  event->memHandle = memHandle;
  event->facing = facing;

  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  CPlayerMoveEvent *existing = globals->m_localMoveQueue.m_events.Head();
  while (existing && static_cast<int>(eventTime - existing->timeStamp) >= 0) {
    existing = existing->Next();
  }

  globals->m_localMoveQueue.m_events.LinkNode(event, existing ? LIST_LINK_BEFORE : LIST_TAIL, existing);
  globals->m_localMover = this;
}

void CMovement::OnMoveStartLocal(unsigned long eventTime, int forward) {
  AddPlayerMoveEvent(eventTime, forward ? PMOVE_MOVE_START_FWD : PMOVE_MOVE_START_BWD, 0.0f);
}

void CMovement::OnMoveStart(unsigned long eventTime, int forward) {
  if (StartMove(static_cast<CMovementGlobals *>(MovementGetGlobals())->m_lastUpdateTime, forward)) {
    AddToMoversList();
  }
}

int CMovement::StartStrafe(unsigned long eventTime, int left) {
  m_moveFlags &= 0xFF9DFFFF;
  if ((m_moveFlags & 0x4000) && (m_moveFlags & 0xF)) {
    m_moveFlags |= left ? 0x00200000 : 0x00400000;
    return 0;
  }

  if (m_moveFlags & 0x4000) {
    m_moveFlags |= 0x20000000;
  }
  UpdateAnchors(eventTime);
  m_moveFlags = (m_moveFlags & ~0xCU) | (left ? 4U : 8U);
  m_moveFlags |= 0x08000000;
  return 1;
}

void CMovement::OnStrafeStartLocal(unsigned long eventTime, int left) {
  AddPlayerMoveEvent(eventTime, left ? PMOVE_STRAFE_START_LFT : PMOVE_STRAFE_START_RGT, 0.0f);
}

int CMovement::Jump(unsigned long eventTime) {
  if ((m_moveFlags & 0x4000) || (m_moveFlags & 0x02000000)) {
    return 0;
  }

  UpdateAnchors(eventTime);
  m_jumpVelocity -= 7.9555473f;
  m_fallStartElevation = m_position.z;
  m_fallStartTime = eventTime;
  m_moveFlags = (m_moveFlags & ~0x1000U) | 0x4000;
  return 1;
}

void CMovement::OnJumpLocal(unsigned long eventTime) {
  AddPlayerMoveEvent(eventTime, PMOVE_JUMP, 0.0f);
}

void CMovement::OnFallLocal(unsigned long eventTime) {
  AddPlayerMoveEvent(eventTime, PMOVE_FALL, 0.0f);
}

void CMovement::OnFall(unsigned long eventTime) {
  StartFalling(eventTime);
  AddToMoversList();
}

void CMovement::Halt(unsigned long eventTime) {
  if (m_moveFlags & 0x4000) {
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
    m_moveFlags = (m_moveFlags & 0xF7FCEFF0) | 0x08000000;
    UpdateAnchors(eventTime);
  } else {
    m_moveFlags |= 0x10000000;
  }
}

int CMovement::StopMove(unsigned long eventTime) {
  if (!(m_moveFlags & 3)) {
    if (m_moveFlags & 0x00180000) {
      m_moveFlags &= 0xFFE7FFFF;
      return 1;
    }
    return 0;
  }

  if (m_moveFlags & 0x4000) {
    m_moveFlags = (m_moveFlags & 0xFFE6FFFF) | 0x00010000;
    return 0;
  }

  m_moveFlags = (m_moveFlags & 0xF7FEEFFC) | 0x08000000;
  UpdateAnchors(eventTime);
  return 1;
}

void CMovement::ForceStopMove(unsigned long eventTime) {
  m_moveFlags = (m_moveFlags & 0xF7FEEFFC) | 0x08000000;
  UpdateAnchors(eventTime);
}

void CMovement::OnMoveStopLocal(unsigned long eventTime) {
  AddPlayerMoveEvent(eventTime, PMOVE_MOVE_STOP, 0.0f);
}

int CMovement::OnMoveStop(unsigned long eventTime) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  if (!StopMove(globals->m_lastUpdateTime) || (m_moveFlags & 0xC)) {
    return 0;
  }
  if (!(m_moveFlags & 0x40FF)) {
    RemoveFromMoversList();
  }
  return 1;
}

int CMovement::StopStrafe(unsigned long eventTime) {
  if (!(m_moveFlags & 0xC)) {
    if (m_moveFlags & 0x00620000) {
      m_moveFlags &= 0xFF9FFFFF;
      return 1;
    }
    return 0;
  }

  if (m_moveFlags & 0x4000) {
    m_moveFlags = (m_moveFlags & 0xFF9DFFFF) | 0x00020000;
    return 0;
  }

  m_moveFlags = (m_moveFlags & 0xF7FDEFF3) | 0x08000000;
  UpdateAnchors(eventTime);
  return 1;
}

void CMovement::OnStrafeStopLocal(unsigned long eventTime) {
  AddPlayerMoveEvent(eventTime, PMOVE_STRAFE_STOP, 0.0f);
}

void CMovement::StartTurn(unsigned long eventTime, int left) {
  UpdateAnchors(eventTime);
  m_moveFlags = (m_moveFlags & ~0x30U) | (left ? 0x10U : 0x20U);
}

void CMovement::OnTurnStartLocal(unsigned long eventTime, int left) {
  AddPlayerMoveEvent(eventTime, left ? PMOVE_TURN_START_LFT : PMOVE_TURN_START_RGT, 0.0f);
}

void CMovement::StopTurn(unsigned long eventTime) {
  if (m_moveFlags & 0x30) {
    UpdateAnchors(eventTime);
    m_moveFlags &= ~0x30U;
  }
}

void CMovement::OnTurnStopLocal(unsigned long eventTime) {
  AddPlayerMoveEvent(eventTime, PMOVE_TURN_STOP, 0.0f);
}

void CMovement::OnSetRunModeLocal(unsigned long eventTime, int run) {
  AddPlayerMoveEvent(eventTime, run ? PMOVE_SET_RUN_MODE : PMOVE_SET_WALK_MODE, 0.0f);
}

void CMovement::OnSetRunMode(unsigned long eventTime, int run) {
  SetRunMode(static_cast<CMovementGlobals *>(MovementGetGlobals())->m_lastUpdateTime, run);
}

void CMovement::SetRunMode(unsigned long eventTime, int run) {
  UpdateAnchors(eventTime);
  if (run) {
    m_moveFlags &= ~0x100U;
  } else {
    m_moveFlags |= 0x100;
  }
  m_moveFlags |= 0x08000000;
  LogWrite(
      "0x%016I64X: Switching to %s mode (0x%08X)\n",
      m_guid,
      (m_moveFlags & 0x100) ? "walk" : "run",
      m_moveStartTime
  );
}

void DisconnectLocalMover(CMovement *mover) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  if (globals && mover == globals->m_localMover) {
    globals->m_localMover = 0;
    globals->m_localMoveQueue.m_events.UnlinkAll();
  }
}

void CMovement::GetMoveStatus(CMovementStatus *status) const {
  status->transport = m_transportGUID;
  status->moveFlags = m_moveFlags & 0xFAFF0BFF;
  status->transRelPosition = m_position;
  status->transRelFacing = m_facing;
  status->worldPosition = GetPosition(m_position);
  status->worldFacing = GetFacing(m_facing);
  status->pitch = m_pitch;

  if (m_spline && !(m_spline->flags & 0x4)) {
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

  if (m_spline && !(m_spline->flags & 4)) {
    init->spline = *m_spline;
  }

  LogWrite("0x%016I64X: Send move (0x%X) ", m_guid, m_moveStartTime);
  switch (m_moveFlags & 3) {
    case 0:
      LogWrite("standing at ");
      break;
    case 1:
      LogWrite("moving forward from ");
      break;
    case 2:
      LogWrite("moving backward from ");
      break;
  }
  LogWrite("(%g,%g), ", m_anchorPosition.x, m_anchorPosition.y);
  switch (m_moveFlags & 0x30) {
    case 0:
      LogWrite("facing ");
      break;
    case 0x10:
      LogWrite("turning left from ");
      break;
    case 0x20:
      LogWrite("turning right from ");
      break;
  }
  LogWrite("(%g)\n", m_anchorFacing);
}

void CMovement::UpdateCurrentSpeed() {
  unsigned int moveFlags = m_moveFlags;
  if (!(moveFlags & 0x4000) || (moveFlags & 0x20000000)) {
    if (!(moveFlags & 0xF)) {
      m_currentSpeed = 0.0f;
      LogWrite("0x%016I64X: Updating current speed to zero\n", m_guid);
    } else if (!m_spline || (m_spline->flags & 4)) {
      float speed = m_runSpeed;
      unsigned int slowFlags = 0x20000102;
      if (moveFlags & 0x02000000) {
        speed = m_swimSpeed;
        slowFlags = 0x2000010E;
      }
      if ((moveFlags & slowFlags) && speed >= m_walkSpeed) {
        speed = m_walkSpeed;
      }
      m_currentSpeed = speed;
      m_moveFlags = moveFlags & ~0x20000000U;
      LogWrite("0x%016I64X: Updating current speed to (%g)\n", m_guid, m_currentSpeed);
    } else {
      float speed = m_spline->spline.cachedLength;
      speed /= static_cast<__int64>(m_spline->time);
      m_currentSpeed = speed * 1000.0f;
      LogWrite("0x%016I64X: Updating spline mover's current speed to (%g)\n", m_guid, m_currentSpeed);
    }
  }
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

int CMovement::SetCollisionBox(const NTempest::CAaBox &box, float scale) {
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

void CMovementData::CalcDirection() {
  float cosFacing;
  float sinFacing;
  NTempest::CMath::sincos_(m_anchorFacing, sinFacing, cosFacing);

  m_direction2d.x = cosFacing;
  m_direction2d.y = sinFacing;

  if (NTempest::CMath::fequal4_(m_anchorPitch, 0)) {
    m_direction = NTempest::C3Vector(cosFacing, sinFacing, 0.0f);
    m_sinAnchorPitch = 0.0f;
    m_cosAnchorPitch = 1.0f;
  } else {
    m_sinAnchorPitch = NTempest::CMath::sin_(m_anchorPitch);
    m_cosAnchorPitch = NTempest::CMath::cos_(m_anchorPitch);
    m_direction.x = m_cosAnchorPitch * cosFacing;
    m_direction.y = m_cosAnchorPitch * sinFacing;
    m_direction.z = m_sinAnchorPitch;
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
    MovementFixUpMoveHistory(m_guid, transportMtx.AffineInverse());
    m_transportGUID = 0;
  }

  if (update.transport && MovementGameObjIsTransport(update.transport)) {
    MovementAddToTransport(this, update.transport);
    MovementGetTransportMtx(update.transport, &transportMtx);
    MovementFixUpMoveHistory(m_guid, transportMtx);

    m_position = update.transRelPosition;
    m_anchorPosition = update.transRelPosition;
    m_facing = update.transRelFacing;
    m_anchorFacing = update.transRelFacing;
    m_transportGUID = update.transport;
  }
}

void CMovement::UpdateStatusInternal(unsigned long eventTime, const CMovementStatus &update) {
  FallLogWrite(
      "0x%016I64X: time (0x%08X) updating from (%g,%g,%g) to "
      "(%g,%g,%g)\n",
      m_guid, eventTime, m_position.x, m_position.y, m_position.z, update.transRelPosition.x, update.transRelPosition.y, update.transRelPosition.z
  );

  m_moveFlags = (m_moveFlags & 0x0500E400) ^ (update.moveFlags & 0xFAFF0BFF) | 0x08000000;
  m_position = update.worldPosition;
  m_anchorPosition = update.worldPosition;
  m_facing = update.worldFacing;
  m_anchorFacing = update.worldFacing;
  m_pitch = update.pitch;
  m_anchorPitch = update.pitch;
  CalcDirection();

  if (m_moveFlags & 0xFF) {
    if (!(m_moveFlags & 0x200)) {
      NTempest::C3Vector intPositionZ = GetPosition(m_position);
      NTempest::C3Vector intPositionY = GetPosition(m_position);
      NTempest::C3Vector intPositionX = GetPosition(m_position);
      float              facing = GetFacing(m_facing);
      NTempest::C3Vector positionZ = GetPosition(m_position);
      NTempest::C3Vector positionY = GetPosition(m_position);
      NTempest::C3Vector positionX = GetPosition(m_position);
      int                intFacing = static_cast<int>(GetFacing(m_facing));
      SErrDisplayErrorFmt(
          0x85100000,
          __FILE__,
          __LINE__,
          0,
          1,
          "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
          "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
          "m_moveFlags & MOVEFLAG_TIME_VALID",
          m_guid,
          positionX.x,
          positionY.y,
          positionZ.z,
          facing,
          static_cast<int>(intPositionX.x),
          static_cast<int>(intPositionY.y),
          static_cast<int>(intPositionZ.z),
          intFacing
      );
    }
    CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
    m_moveStartTime = globals->m_lastUpdateTime;
    FallLogWrite("0x%016I64X: setting move start time (0x%08X)\n", m_guid, m_moveStartTime);
  }

  MovementNotifyZoneMgr(m_guid);
}

void CMovement::UpdateStatus(unsigned long eventTime, const CMovementStatus &update) {
  UpdateStatusInternal(eventTime, update);
  SetIdleUpdates();
  if (m_position.z < -333.33334f) {
    MovementFixOutOfBoundsUnit(m_guid);
  }
}

void CMovement::UpdateStatusLocal(unsigned long eventTime, const CMovementStatus &update) {
  UpdateStatusInternal(eventTime, update);
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  globals->m_localMover = m_moveFlags & 0x40FF ? this : 0;
}

void CMovement::AddSpline() {
  m_moveFlags |= 0x04000000;
  if (!m_spline) {
    m_spline = NEW(CMoveSpline);
  }
  DisconnectLocalMover(this);
}

void CMovement::OnSpline(
    unsigned long eventTime, const NTempest::C3Vector *points, unsigned int count, unsigned long duration, unsigned int flags
) {
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
    NTempest::C3Vector intPositionZ = GetPosition(m_position);
    NTempest::C3Vector intPositionY = GetPosition(m_position);
    NTempest::C3Vector intPositionX = GetPosition(m_position);
    float              facing = GetFacing(m_facing);
    NTempest::C3Vector positionZ = GetPosition(m_position);
    NTempest::C3Vector positionY = GetPosition(m_position);
    NTempest::C3Vector positionX = GetPosition(m_position);
    int                intFacing = static_cast<int>(GetFacing(m_facing));
    SErrDisplayErrorFmt(
        0x85100000,
        __FILE__,
        __LINE__,
        0,
        1,
        "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
        "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
        "IsSpline()",
        m_guid,
        positionX.x,
        positionY.y,
        positionZ.z,
        facing,
        static_cast<int>(intPositionX.x),
        static_cast<int>(intPositionY.y),
        static_cast<int>(intPositionZ.z),
        intFacing
    );
  }
  m_spline->flags |= 0x10000;
  m_spline->face.spot = spot;
}

void CMovement::OnSplineDoneFace(const unsigned __int64 &guid) {
  if (!m_spline || (m_spline->flags & 4)) {
    NTempest::C3Vector intPositionZ = GetPosition(m_position);
    NTempest::C3Vector intPositionY = GetPosition(m_position);
    NTempest::C3Vector intPositionX = GetPosition(m_position);
    float              facing = GetFacing(m_facing);
    NTempest::C3Vector positionZ = GetPosition(m_position);
    NTempest::C3Vector positionY = GetPosition(m_position);
    NTempest::C3Vector positionX = GetPosition(m_position);
    int                intFacing = static_cast<int>(GetFacing(m_facing));
    SErrDisplayErrorFmt(
        0x85100000,
        __FILE__,
        __LINE__,
        0,
        1,
        "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
        "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
        "IsSpline()",
        m_guid,
        positionX.x,
        positionY.y,
        positionZ.z,
        facing,
        static_cast<int>(intPositionX.x),
        static_cast<int>(intPositionY.y),
        static_cast<int>(intPositionZ.z),
        intFacing
    );
  }
  m_spline->flags |= 0x20000;
  m_spline->face.guid = guid;
}

void CMovement::OnSplineDoneFace(float facing) {
  if (!m_spline || (m_spline->flags & 4)) {
    NTempest::C3Vector intPositionZ = GetPosition(m_position);
    NTempest::C3Vector intPositionY = GetPosition(m_position);
    NTempest::C3Vector intPositionX = GetPosition(m_position);
    float              currentFacing = GetFacing(m_facing);
    NTempest::C3Vector positionZ = GetPosition(m_position);
    NTempest::C3Vector positionY = GetPosition(m_position);
    NTempest::C3Vector positionX = GetPosition(m_position);
    int                intFacing = static_cast<int>(GetFacing(m_facing));
    SErrDisplayErrorFmt(
        0x85100000,
        __FILE__,
        __LINE__,
        0,
        1,
        "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
        "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
        "IsSpline()",
        m_guid,
        positionX.x,
        positionY.y,
        positionZ.z,
        currentFacing,
        static_cast<int>(intPositionX.x),
        static_cast<int>(intPositionY.y),
        static_cast<int>(intPositionZ.z),
        intFacing
    );
  }
  m_spline->flags |= 0x40000;
  m_spline->face.facing = facing;
}

void CMovementData::RemoveSpline() {
  m_moveFlags &= ~0x04000000u;
  if (m_spline) {
    delete m_spline;
    m_spline = 0;
  }
  RemoveFromMoversList();
}

void CMovement::SetUpdateInfo(unsigned long eventTime, const CClientMoveUpdate &init, int localPlayer) {
  FATALASSERT(CMath::fnotequal_(init.runSpeed,0));
  FATALASSERT(CMath::fnotequal_(init.walkSpeed,0));
  FATALASSERT(CMath::fnotequal_(init.swimSpeed,0));
  FATALASSERT(CMath::fnotequal_(init.turnRate,0));

  m_walkSpeed = init.walkSpeed;
  m_runSpeed = init.runSpeed;
  m_swimSpeed = init.swimSpeed;
  m_turnRate = init.turnRate;

  m_moveStartTime = static_cast<CMovementGlobals *>(MovementGetGlobals())->m_lastUpdateTime;
  FallLogWrite("0x%016I64X: setting move start time (0x%08X)\n", m_guid, m_moveStartTime);
  m_fallStartTime = static_cast<CMovementGlobals *>(MovementGetGlobals())->m_lastUpdateTime - init.timeFallen;

  if (init.status.moveFlags & 0x04000000) {
    AddSpline();
    *m_spline = init.spline;
  } else {
    RemoveSpline();
  }

  m_moveFlags |= static_cast<CMovementGlobals *>(MovementGetGlobals())->ignoreObstacles ? 0x08000000 : 0x08004000;
  if (localPlayer) {
    UpdateStatusLocal(static_cast<CMovementGlobals *>(MovementGetGlobals())->m_lastUpdateTime, init.status);
  } else {
    UpdateStatus(static_cast<CMovementGlobals *>(MovementGetGlobals())->m_lastUpdateTime, init.status);
  }

  m_moveFlags |= 0x200;
  m_fallStartElevation = CalcFallStartElevation(init.timeFallen);
  if (m_moveFlags & 0x8000) {
    OnCollideFalling(m_guid, static_cast<CMovementGlobals *>(MovementGetGlobals())->m_lastUpdateTime);
  }

  LogUpdateInfo(init);
}

void CMovement::LogUpdateInfo(const CClientMoveUpdate &init) {
  FallLogWrite(
      "0x%016I64X: ___INIT EVENT: time(0x%X) timeFallen(%u) "
      "pos(%g,%g,%g)\n",
      m_guid,
      static_cast<CMovementGlobals *>(MovementGetGlobals())->m_lastUpdateTime,
      init.timeFallen,
      init.status.worldPosition.x,
      init.status.worldPosition.y,
      init.status.worldPosition.z
  );
  LogWrite("0x%016I64X: Receive move (0x%X) ", m_guid, m_moveStartTime);
  switch (m_moveFlags & 3) {
    case 0:
      LogWrite("standing at ");
      break;
    case 1:
      LogWrite("moving forward from ");
      break;
    case 2:
      LogWrite("moving backward from ");
      break;
  }
  LogWrite("(%g,%g), ", m_anchorPosition.x, m_anchorPosition.y);
  switch (m_moveFlags & 0x30) {
    case 0:
      LogWrite("facing ");
      break;
    case 0x10:
      LogWrite("turning left from ");
      break;
    case 0x20:
      LogWrite("turning right from ");
      break;
  }
  LogWrite("(%g)\n", m_anchorFacing);
}

void CMovement::StartFallLogging() {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  ASSERT(globals);

  char fileName[260];
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

int CMovement::IsFallLoggingOn() {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  return globals && globals->fallingLog;
}

void __cdecl CMovement::FallLogWrite(const char *format, ...) {
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

int CMovement::IsLoggingOn() {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  ASSERT(globals);
  return globals->movementLog != 0;
}

void __cdecl CMovement::LogWrite(const char *format, ...) {
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

void __cdecl CMovement::BothLogWrite(const char *format, ...) {
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

int CMovement::OnRunSpeedChange(unsigned long eventTime, float speed) {
  if (!CMath::fnotequal_(speed,0)) {
    if (this) {
      NTempest::C3Vector intPositionZ = GetPosition(m_position);
      NTempest::C3Vector intPositionY = GetPosition(m_position);
      NTempest::C3Vector intPositionX = GetPosition(m_position);
      float              facing = GetFacing(m_facing);
      NTempest::C3Vector positionZ = GetPosition(m_position);
      NTempest::C3Vector positionY = GetPosition(m_position);
      NTempest::C3Vector positionX = GetPosition(m_position);
      int                intFacing = static_cast<int>(GetFacing(m_facing));
      SErrDisplayErrorFmt(
          0x85100000,
          __FILE__,
          __LINE__,
          0,
          1,
          "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
          "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
          "CMath::fnotequal_(speed,0)",
          m_guid,
          positionX.x,
          positionY.y,
          positionZ.z,
          facing,
          static_cast<int>(intPositionX.x),
          static_cast<int>(intPositionY.y),
          static_cast<int>(intPositionZ.z),
          intFacing
      );
    } else {
      SErrDisplayError(
          0x85100000,
          __FILE__,
          __LINE__,
          "CMath::fnotequal_(speed,0)",
          0,
          1
      );
    }
  }
  if (NTempest::CMath::fequal_(m_runSpeed, speed)) {
    return 0;
  }
  LogWrite(
      "0x%016I64X: Changing run speed from (%g) to (%g) (0x%X)\n", m_guid, m_runSpeed, speed,
      m_moveStartTime
  );
  m_runSpeed = speed;
  m_moveFlags |= 0x08000000;
  UpdateAnchors(static_cast<CMovementGlobals *>(MovementGetGlobals())->m_lastUpdateTime);
  return 1;
}

int CMovement::OnWalkSpeedChange(unsigned long eventTime, float speed) {
  if (!CMath::fnotequal_(speed,0)) {
    if (this) {
      NTempest::C3Vector intPositionZ = GetPosition(m_position);
      NTempest::C3Vector intPositionY = GetPosition(m_position);
      NTempest::C3Vector intPositionX = GetPosition(m_position);
      float              facing = GetFacing(m_facing);
      NTempest::C3Vector positionZ = GetPosition(m_position);
      NTempest::C3Vector positionY = GetPosition(m_position);
      NTempest::C3Vector positionX = GetPosition(m_position);
      int                intFacing = static_cast<int>(GetFacing(m_facing));
      SErrDisplayErrorFmt(
          0x85100000,
          __FILE__,
          __LINE__,
          0,
          1,
          "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
          "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
          "CMath::fnotequal_(speed,0)",
          m_guid,
          positionX.x,
          positionY.y,
          positionZ.z,
          facing,
          static_cast<int>(intPositionX.x),
          static_cast<int>(intPositionY.y),
          static_cast<int>(intPositionZ.z),
          intFacing
      );
    } else {
      SErrDisplayError(
          0x85100000,
          __FILE__,
          __LINE__,
          "CMath::fnotequal_(speed,0)",
          0,
          1
      );
    }
  }
  if (NTempest::CMath::fequal_(m_walkSpeed, speed)) {
    return 0;
  }
  LogWrite(
      "0x%016I64X: Changing walk speed from (%g) to (%g) (0x%X)\n", m_guid, m_walkSpeed, speed,
      m_moveStartTime
  );
  m_walkSpeed = speed;
  m_moveFlags |= 0x08000000;
  UpdateAnchors(static_cast<CMovementGlobals *>(MovementGetGlobals())->m_lastUpdateTime);
  return 1;
}

int CMovement::OnSwimSpeedChange(unsigned long eventTime, float speed) {
  if (!CMath::fnotequal_(speed,0)) {
    if (this) {
      NTempest::C3Vector intPositionZ = GetPosition(m_position);
      NTempest::C3Vector intPositionY = GetPosition(m_position);
      NTempest::C3Vector intPositionX = GetPosition(m_position);
      float              facing = GetFacing(m_facing);
      NTempest::C3Vector positionZ = GetPosition(m_position);
      NTempest::C3Vector positionY = GetPosition(m_position);
      NTempest::C3Vector positionX = GetPosition(m_position);
      int                intFacing = static_cast<int>(GetFacing(m_facing));
      SErrDisplayErrorFmt(
          0x85100000,
          __FILE__,
          __LINE__,
          0,
          1,
          "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
          "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
          "CMath::fnotequal_(speed,0)",
          m_guid,
          positionX.x,
          positionY.y,
          positionZ.z,
          facing,
          static_cast<int>(intPositionX.x),
          static_cast<int>(intPositionY.y),
          static_cast<int>(intPositionZ.z),
          intFacing
      );
    } else {
      SErrDisplayError(
          0x85100000,
          __FILE__,
          __LINE__,
          "CMath::fnotequal_(speed,0)",
          0,
          1
      );
    }
  }
  if (NTempest::CMath::fequal_(m_swimSpeed, speed)) {
    return 0;
  }
  LogWrite(
      "0x%016I64X: Changing swim speed from (%g) to (%g) (0x%X)\n", m_guid, m_swimSpeed, speed,
      m_moveStartTime
  );
  m_swimSpeed = speed;
  m_moveFlags |= 0x08000000;
  UpdateAnchors(static_cast<CMovementGlobals *>(MovementGetGlobals())->m_lastUpdateTime);
  return 1;
}

int CMovement::OnTurnRateChange(unsigned long eventTime, float rate) {
  if (!CMath::fnotequal_(rate,0)) {
    if (this) {
      NTempest::C3Vector intPositionZ = GetPosition(m_position);
      NTempest::C3Vector intPositionY = GetPosition(m_position);
      NTempest::C3Vector intPositionX = GetPosition(m_position);
      float              facing = GetFacing(m_facing);
      NTempest::C3Vector positionZ = GetPosition(m_position);
      NTempest::C3Vector positionY = GetPosition(m_position);
      NTempest::C3Vector positionX = GetPosition(m_position);
      int                intFacing = static_cast<int>(GetFacing(m_facing));
      SErrDisplayErrorFmt(
          0x85100000,
          __FILE__,
          __LINE__,
          0,
          1,
          "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
          "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
          "CMath::fnotequal_(rate,0)",
          m_guid,
          positionX.x,
          positionY.y,
          positionZ.z,
          facing,
          static_cast<int>(intPositionX.x),
          static_cast<int>(intPositionY.y),
          static_cast<int>(intPositionZ.z),
          intFacing
      );
    } else {
      SErrDisplayError(
          0x85100000,
          __FILE__,
          __LINE__,
          "CMath::fnotequal_(rate,0)",
          0,
          1
      );
    }
  }
  if (NTempest::CMath::fequal_(m_turnRate, rate)) {
    return 0;
  }
  LogWrite(
      "0x%016I64X: Changing turn rate from (%g) to (%g) (0x%X)\n", m_guid, m_turnRate, rate, m_moveStartTime
  );
  m_turnRate = rate;
  UpdateAnchors(static_cast<CMovementGlobals *>(MovementGetGlobals())->m_lastUpdateTime);
  return 1;
}

void CMovement::OnSetFacingLocal(unsigned long eventTime, float facing) {
  if (m_transportGUID) {
    facing -= MovementGetTransportFacing(m_transportGUID);
    if (facing >= 6.2831855f) {
      facing -= 6.2831855f;
    } else if (facing < 0.0f) {
      facing += 6.2831855f;
    }
  }

  AddPlayerMoveEvent(eventTime, PMOVE_SET_FACING, facing);
}

void CMovement::OnSetRawFacingLocal(unsigned long eventTime, float facing) {
  AddPlayerMoveEvent(eventTime, PMOVE_SET_FACING, facing);
}

void CMovement::OnSetFacing(unsigned long eventTime, float facing) {
  if (m_transportGUID) {
    facing -= MovementGetTransportFacing(m_transportGUID);
  }
  if (facing >= 6.2831855f) {
    facing -= 6.2831855f;
  } else if (facing < 0.0f) {
    facing += 6.2831855f;
  }

  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  FATALASSERT(globals);
  SetFacing(globals->m_lastUpdateTime, facing);
  if (!(m_moveFlags & 0x40FF)) {
    RemoveFromMoversList();
  }
}

void CMovement::Teleport(unsigned long eventTime, const NTempest::C3Vector &position, float facing) {
  LogWrite(
      "0x%016I64X: Teleporting (0x%X) from (%g,%g,%g) (%g) to (%g,%g,%g)\n", m_guid, eventTime, m_position.x, m_position.y,
      m_position.z, m_facing, position.x, position.y, position.z
  );
  m_position = position;
  m_facing = facing;
  m_pitch = 0.0f;
  ForceSetTransport(0);
  UpdateAnchors(eventTime);
  m_moveFlags = (m_moveFlags & 0xF5FCEF00) | 0x08000000;
  if (m_spline && !(m_spline->flags & 4)) {
    m_spline->flags |= 4;
  }

  if (!(m_moveFlags & 0x800) && (!m_spline || (m_spline->flags & 4) || !(m_spline->flags & 0x200))) {
    CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
    FATALASSERT(globals);
    if (!globals->ignoreObstacles && !(m_moveFlags & 0x800) && (!m_spline || (m_spline->flags & 4) || !(m_spline->flags & 0x200))) {
      m_moveFlags |= 0x4000;
    }
    m_fallStartTime = eventTime;
    m_fallStartElevation = position.z;
    FallLogWrite(
        "0x%016I64X: Started to fall from teleport at (0x%08X) from (%g elevation)\n", m_guid, eventTime,
        static_cast<double>(position.z)
    );
    OnMoveUpdate(m_guid, eventTime);
  }
}

void CMovement::OnTeleport(unsigned long eventTime, const NTempest::C3Vector &position, float facing) {
  DisconnectLocalMover(this);
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  FATALASSERT(globals);
  Teleport(globals->m_lastUpdateTime, position, facing);
  AddToMoversList();
}

void CMovement::OnTeleportLocal(unsigned long eventTime, const NTempest::C3Vector &position, float facing) {
  RemoveSpline();
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  FATALASSERT(globals);
  Teleport(globals->m_lastUpdateTime, position, facing);
  globals->m_localMover = this;
}

void CMovement::OnStrafeStart(unsigned long eventTime, int left) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  if (StartStrafe(globals->m_lastUpdateTime, left)) {
    AddToMoversList();
  }
}

int CMovement::OnStrafeStop(unsigned long eventTime) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  if (!StopStrafe(globals->m_lastUpdateTime) || (m_moveFlags & 3)) {
    return 0;
  }
  if (!(m_moveFlags & 0x40FF)) {
    RemoveFromMoversList();
  }
  return 1;
}

void CMovement::OnJump(unsigned long eventTime) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  if (Jump(globals->m_lastUpdateTime)) {
    AddToMoversList();
  }
}

void CMovement::OnTurnStart(unsigned long eventTime, int left) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  StartTurn(globals->m_lastUpdateTime, left);
  AddToMoversList();
}

void CMovement::OnTurnStop(unsigned long eventTime) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  StopTurn(globals->m_lastUpdateTime);
  if (!(m_moveFlags & 0x40FF)) {
    RemoveFromMoversList();
  }
}

void CMovement::OnPitchStart(unsigned long eventTime, int up) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  StartPitch(globals->m_lastUpdateTime, up);
  AddToMoversList();
}

void CMovement::OnPitchStop(unsigned long eventTime) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  StopPitch(globals->m_lastUpdateTime);
  if (!(m_moveFlags & 0x40FF)) {
    RemoveFromMoversList();
  }
}

void CMovement::OnSwimStart(unsigned long eventTime) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  StartSwim(globals->m_lastUpdateTime);
  if (!(m_moveFlags & 0x40FF)) {
    RemoveFromMoversList();
  }
}

void CMovement::OnSwimStop(unsigned long eventTime) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  StopSwim(globals->m_lastUpdateTime);
  AddToMoversList();
}

void CMovement::OnSetPitch(unsigned long eventTime, float pitch) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  SetPitch(globals->m_lastUpdateTime, pitch);
  if (!(m_moveFlags & 0x40FF)) {
    RemoveFromMoversList();
  }
}

void CMovement::OnDisableGravity(unsigned long eventTime) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  if (!globals->ignoreObstacles) {
    unsigned int oldMoveFlags = m_moveFlags;
    StopFalling();
    HandlePendingActions(eventTime);
    CallMoveEventHandlers(eventTime, 0, oldMoveFlags, 0);
  }
}

void CMovement::OnEnableGravity(unsigned long eventTime) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  if (!globals->ignoreObstacles) {
    StartFalling(eventTime);
  }
}

void CMovement::CollisionStateChanged() {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  if ((m_moveFlags & 0x800) || (m_spline && !(m_spline->flags & 4) && (m_spline->flags & 0x200))) {
    OnDisableGravity(globals->m_lastUpdateTime);
    if (!(m_moveFlags & 0x40FF)) {
      RemoveFromMoversList();
    }
  } else {
    OnEnableGravity(globals->m_lastUpdateTime);
    AddToMoversList();
  }
}

void CMovement::CollisionStateChangedLocal(unsigned long eventTime) {
  if ((m_moveFlags & 0x800) || (m_spline && !(m_spline->flags & 4) && (m_spline->flags & 0x200))) {
    OnDisableGravity(eventTime);
    if (!(m_moveFlags & 0xF)) {
      CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
      globals->m_localMover = 0;
    }
  } else {
    OnEnableGravity(eventTime);
    CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
    globals->m_localMover = this;
  }
}

void CMovement::ToggleCollision(unsigned long eventTime) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  UpdateAnchors(globals->m_lastUpdateTime);
  m_moveFlags ^= 0x800;
  globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  CollisionStateChangedLocal(globals->m_lastUpdateTime);
}

void CMovement::EnableCollision(unsigned long eventTime, int enable) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  UpdateAnchors(globals->m_lastUpdateTime);
  if (enable) {
    m_moveFlags &= ~0x800u;
  } else {
    m_moveFlags |= 0x800;
  }
  CollisionStateChanged();
}

void CMovement::SetFacing(unsigned long eventTime, float facing) {
  if (NTempest::CMath::fabs_(facing - m_facing) >= 0.00000095367432f) {
    m_facing = facing;
    if (!(m_moveFlags & 0x4000)) {
      UpdateAnchors(eventTime);
    }
  }
  m_moveFlags &= ~0x30U;
}

void CMovement::OnSetPitchLocal(unsigned long eventTime, float pitch) {
  AddPlayerMoveEvent(eventTime, PMOVE_SET_PITCH, pitch);
}

void CMovement::SetPitch(unsigned long eventTime, float pitch) {
  if ((m_moveFlags & 0x02000000) && NTempest::CMath::fabs_(pitch - m_pitch) >= 0.00000095367432f) {
    m_pitch = pitch;
    UpdateAnchors(eventTime);
  }
  m_moveFlags &= ~0xC0U;
}

void MovementEnableCollision(int enable) {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  FATALASSERT(globals);
  globals->ignoreObstacles = enable == 0;
}

void CMovement::AddToMoversList() {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  if (globals) {
    FATALASSERT(globals->m_localMover != this);
    MovementLockMoversList(1);
    if (!moveLink.IsLinked()) {
      globals->movers.LinkNode(this, LIST_HEAD, 0);
      ++globals->numMovers;
    }
    MovementUnlockMoversList(1);
  }
}

void CMovementData::RemoveFromMoversList() {
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  if (globals) {
    MovementLockMoversList(1);
    if (moveLink.IsLinked()) {
      globals->movers.UnlinkNode(this);
      --globals->numMovers;
    }
    MovementUnlockMoversList(1);
  }
}

unsigned int CMovement::FallTime() const {
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

int CMovement::GetMoveEventMsgId(unsigned int oldMoveFlags, int wasJumping) {
  unsigned int changed = oldMoveFlags ^ m_moveFlags;
  if (changed & 3) {
    if (m_moveFlags & 1) {
      return MSG_MOVE_START_FORWARD;
    }
    return m_moveFlags & 2 ? MSG_MOVE_START_BACKWARD : MSG_MOVE_STOP;
  }
  if (changed & 0xC) {
    if (m_moveFlags & 4) {
      return MSG_MOVE_START_STRAFE_LEFT;
    }
    return m_moveFlags & 8 ? MSG_MOVE_START_STRAFE_RIGHT : MSG_MOVE_STOP_STRAFE;
  }
  if (m_jumpVelocity == 0.0f || wasJumping) {
    if (changed & 0x02000000) {
      return m_moveFlags & 0x02000000 ? MSG_MOVE_START_SWIM : MSG_MOVE_STOP_SWIM;
    }
    return MSG_MOVE_STOP;
  }
  return MSG_MOVE_JUMP;
}

void CMovement::OnSwimStartLocal(unsigned long eventTime) {
  AddPlayerMoveEvent(eventTime, PMOVE_MOVE_START_SWIM, 0.0f);
}

void CMovement::OnSwimStopLocal(unsigned long eventTime) {
  AddPlayerMoveEvent(eventTime, PMOVE_MOVE_STOP_SWIM, 0.0f);
}

void CMovement::StartSwimLocal(unsigned long eventTime) {
  StartSwim(eventTime);
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  FATALASSERT(globals);
  globals->m_localMover = this;
}

void CMovement::StopSwimLocal(unsigned long eventTime) {
  StopSwim(eventTime);
  CMovementGlobals *globals = static_cast<CMovementGlobals *>(MovementGetGlobals());
  FATALASSERT(globals);
  globals->m_localMover = this;
}

void CMovement::StartSwim(unsigned long eventTime) {
  UpdateAnchors(eventTime);
  m_moveFlags |= 0x0A000000;
}

void CMovement::StopSwim(unsigned long eventTime) {
  UpdateAnchors(eventTime);
  m_moveFlags = (m_moveFlags & 0xF5FFFF3F) | 0x08000000;
  m_anchorPitch = 0.0f;
  m_pitch = 0.0f;
}

void CMovement::OnPitchStartLocal(unsigned long eventTime, int up) {
  AddPlayerMoveEvent(eventTime, up ? PMOVE_PITCH_START_UP : PMOVE_PITCH_START_DOWN, 0.0f);
}

void CMovement::OnPitchStopLocal(unsigned long eventTime) {
  AddPlayerMoveEvent(eventTime, PMOVE_PITCH_STOP, 0.0f);
}

void CMovement::StartPitch(unsigned long eventTime, int up) {
  if (m_moveFlags & 0x02000000) {
    UpdateAnchors(eventTime);
    m_moveFlags = (m_moveFlags & ~0xC0U) | (up ? 0x40U : 0x80U);
  }
}

void CMovement::StopPitch(unsigned long eventTime) {
  if ((m_moveFlags & 0x020000C0) == 0x020000C0 || ((m_moveFlags & 0x02000000) && (m_moveFlags & 0xC0))) {
    UpdateAnchors(eventTime);
    m_moveFlags &= ~0xC0U;
  }
}
