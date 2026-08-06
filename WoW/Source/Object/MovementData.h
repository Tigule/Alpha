#ifndef WOW_SOURCE_OBJECT_MOVEMENTDATA_H
#define WOW_SOURCE_OBJECT_MOVEMENTDATA_H

#include <stdio.h>
#include <stpl.h>

#include "Object/Object.h"
#include "Tempest/c2vector.h"
#include "Tempest/c3vector.h"
#include "Tempest/caabox.h"
#include "Tempest/cfacet.h"
#include "Tempest/cimvector.h"

class CMovement;
struct CWalkableSurface;
struct CRedirect {
  CRedirect();
  void Reset();

  NTempest::C3Vector hitPoint;
  NTempest::C3Vector surfaceNorm[2];
  DWORDLONG          gameObjHit;
  BYTE               flags;
};

struct CMoveState {
  NTempest::C3Vector position;
  float              facing;
  float              pitch;
  UINT               moveFlags;
  NTempest::C3Vector anchorPosition;
  float              anchorFacing;
  float              anchorPitch;
  DWORD              moveStartTime;
  NTempest::C3Vector direction;
  NTempest::C2Vector direction2d;
  float              cosAnchorPitch;
  float              sinAnchorPitch;
  NTempest::C3Vector reDirection;
  DWORD              fallStartTime;
  float              fallStartElevation;
  float              jumpVelocity;
};

namespace NTempest {
  class C34Matrix;
  class C4Plane;
}  // namespace NTempest

void  OnPendingMoveStateChange(DWORDLONG unit, int msgId, DWORD eventTime);
void  OnCollideRedirected(DWORDLONG unit, DWORD eventTime);
void  OnCollideStuck(DWORDLONG unit, DWORD eventTime);
void  OnCollideFallLand(DWORDLONG unit, DWORD eventTime);
void  OnCollideFalling(DWORDLONG unit, DWORD eventTime);
void  UnitNotifyStopped(const DWORDLONG &guid, bool moveComplete);
int   UnitGetObjectPosition(const DWORDLONG &guid, NTempest::C3Vector *position);
float UnitCalculateFacingTo(const NTempest::C3Vector &position, const NTempest::C3Vector &destination);
enum FACET_COLOR {
  FACET_UNTESTED = 0,
  FACET_TESTED_UNTOUCHED = 1,
  FACET_TESTED_TOUCHED = 2,
  FACET_BLOCKING = 3,
  NUM_FACET_COLORS = 4
};

extern TSGrowableArray<WORD>                g_debugBoxIndices;
extern TSGrowableArray<NTempest::C3Vector>  g_debugBoxNormals;
extern TSGrowableArray<WORD>                g_debugIndices;
extern TSGrowableArray<NTempest::C3Vector>  g_debugNormalVerts;
extern TSGrowableArray<NTempest::C3Vector>  g_debugVerts;
extern TSGrowableArray<WORD>                g_debugNormalIndices;
extern TSGrowableArray<NTempest::C3Vector>  g_debugBoxVerts;
extern TSGrowableArray<NTempest::CImVector> g_debugVertColors;

void CollisionInfoSetWatchGUID(const DWORDLONG &guid);
void CollisionInfoReset();
void CollisionInfoSetFaces(const DWORDLONG &guid, const TSGrowableArray<NTempest::CFacet> &faces);
void CollisionInfoColorFace(UINT faceId, FACET_COLOR color);
void CollisionInfoSetFallBox(const NTempest::C3Vector &position, float boxHalfDepth, float boxHeight);
void CollisionInfoAddBox(const NTempest::C3Vector &boxMin, const NTempest::C3Vector &boxMax);
void CollisionInfoAddVector(const NTempest::C3Vector &position, const NTempest::C3Vector &vector);
int  ToggleCollisionInfo();
void RenderCollisionInfo();
void ProcessLocalMoveEvent(UINT msgId);

enum PLAYER_MOVE_EVT {
  PMOVE_MOVE_START_FWD = 0x00,
  PMOVE_MOVE_START_BWD = 0x01,
  PMOVE_MOVE_STOP = 0x02,
  PMOVE_STRAFE_START_LFT = 0x03,
  PMOVE_STRAFE_START_RGT = 0x04,
  PMOVE_STRAFE_STOP = 0x05,
  PMOVE_FALL = 0x06,
  PMOVE_JUMP = 0x07,
  PMOVE_TURN_START_LFT = 0x08,
  PMOVE_TURN_START_RGT = 0x09,
  PMOVE_TURN_STOP = 0x0A,
  PMOVE_PITCH_START_UP = 0x0B,
  PMOVE_PITCH_START_DOWN = 0x0C,
  PMOVE_PITCH_STOP = 0x0D,
  PMOVE_SET_RUN_MODE = 0x0E,
  PMOVE_SET_WALK_MODE = 0x0F,
  PMOVE_SET_FACING = 0x10,
  PMOVE_SET_PITCH = 0x11,
  PMOVE_MOVE_START_SWIM = 0x12,
  PMOVE_MOVE_STOP_SWIM = 0x13,
  NUM_PMOVE_EVTS = 0x14
};

NODEDECL(CPlayerMoveEvent) {
  DWORD           timeStamp;
  PLAYER_MOVE_EVT eventType;
  UINT            memHandle;
  float           facing;
};

class CPlayerMoveQueue {
 public:
  void              Enqueue(CPlayerMoveEvent *event);
  CPlayerMoveEvent *Root();
  void              Dequeue();
  BYTE              HasEntries();
  void              DiscardAll();

 protected:
  friend class CMovement;
  friend void MovementDestroy();
  friend void DisconnectLocalMover(CMovement *);

  LISTDECL(CPlayerMoveEvent, m_events);
};

class CMovementData {
 public:
  CMovementData(const DWORDLONG &guid);
  CMovementData(const NTempest::C3Vector &position, float facing, const DWORDLONG &guid);
  ~CMovementData();
  NTempest::C3Vector GetPosition() const {
    return GetPosition(m_position);
  }
  NTempest::C3Vector GetPosition(const NTempest::C3Vector &position) const;
  NTempest::C3Vector GetRawPosition() const;
  float              GetFacing() const;
  float              GetFacing(float facing) const;
  float              GetRawFacing() const;
  float              GetPitch() const;
  NTempest::C3Vector GetAnchorPosition() const;
  float              GetAnchorFacing() const;
  float              GetAnchorPitch() const;
  NTempest::C3Vector GetGroundNormal() const;
  NTempest::C3Vector GetRedirection() const;
  NTempest::C3Vector GetLastSentRedirection() const;
  DWORD              GetMoveStartTime() const {
    return m_moveStartTime;
  }
  float GetRunSpeed() const;
  float GetWalkSpeed() const;
  float GetSwimSpeed() const;
  float GetTurnRate() const;
  UINT  GetMoveFlags() const {
    return m_moveFlags;
  }
  DWORDLONG GetGUID() const {
    return m_guid;
  }
  int IsInMotion() const;
  int IsMovingOrTurning() const;
  int IsMovingAndTurning() const;
  int IsMovingOrFalling() const;
  int IsMovingOrStrafing() const;
  int IsMovingStrafingOrFalling() const;
  int IsMovingAndStrafing() const;
  int IsMovingTurningOrStrafing() const;
  int IsMoving() const;
  int IsMovingForward() const;
  int IsMovingBackwards() const;
  int IsTurning() const;
  int IsTurningOrFalling() const;
  int IsTurningLeft() const;
  int IsTurningRight() const;
  int IsTurningOrPitching() const;
  int IsTurningAndPitching() const;
  int IsStrafingLeft() const;
  int IsStrafingRight() const;
  int IsStrafing() const;
  int IsFalling() const;
  int IsJumping() const;
  int HasFallenFar() const;
  int IsWalking() const;
  int Moved() const;
  int TimeIsValid() const;
  int IsImmobilized() const;
  int IsRooted() const;
  int IsSwimming() const;
  int IsSwimmingOrFalling() const;
  int IsPitching() const;
  int IsPitchingUp() const;
  int IsPitchingDown() const;
  int IsMovingStrafingOrSwimming() const;
  int IsMovingStrafingFallingOrSwimming() const;
  int IsSplineMover() const {
    return (m_moveFlags & 0x04000000) != 0;
  }
  int   IgnoresCollision() const;
  int   IsHalted() const;
  int   WasNudged() const;
  float GetCollisionBoxHeight() const {
    return m_collisionBoxHeight;
  }
  void SetWaterSurfaceElevation(float elevation);
  int  ForceSetTransport(DWORDLONG guid);
  int  SetTransport(DWORDLONG guid);
  void RemoveFromMoversList();

  friend void OnMoveUpdate(DWORDLONG unit, DWORD eventTime);
  friend class CGUnit_C;
  friend class CGPlayer_C;
  friend class CGInputControl;
  friend class CGGameObject_C_Type_Chair;
  friend class CGGameObject_C_Type_MapObjTransport;
  friend class CGGameObject_C_Type_Transport;
  friend int MoveHeartBeatHandler(LPCVOID packetData, LPVOID param);
  friend int Player_C_AppFocusMovementHandler(int focus);

  LINKDECLEX(CMovementData, moveLink);
  LINKDECLEX(CMovementData, transportLink);

 protected:
  void CalcDirection();
  void RemoveSpline();
  int  IsLocalPlayer();

 private:
  CMovementData &operator=(const CMovementData &);

 protected:
  NTempest::C3Vector m_position;
  float              m_facing;
  float              m_pitch;
  NTempest::C3Vector m_groundNormal;
  const DWORDLONG   &m_guid;
  DWORDLONG          m_transportGUID;
  UINT               m_moveFlags;
  NTempest::C3Vector m_anchorPosition;
  float              m_anchorFacing;
  float              m_anchorPitch;
  DWORD              m_moveStartTime;
  NTempest::C3Vector m_direction;
  NTempest::C2Vector m_direction2d;
  float              m_cosAnchorPitch;
  float              m_sinAnchorPitch;
  NTempest::C3Vector m_reDirection;
  NTempest::C3Vector m_lastReDirectionSent;
  DWORD              m_fallStartTime;
  float              m_fallStartElevation;
  float              m_currentSpeed;
  float              m_walkSpeed;
  float              m_runSpeed;
  float              m_swimSpeed;
  float              m_turnRate;
  float              m_collisionBoxHalfDepth;
  float              m_collisionBoxHeight;
  float              m_stepUpHeight;
  float              m_jumpVelocity;
  CMoveSpline       *m_spline;
  float              m_waterSurfaceElev;
};

struct CMovementGlobals {
  CMovementGlobals() : movementLog(0), fallingLog(0), numMovers(0), ignoreObstacles(0), currentLoading(0), m_localMover(0), m_lastUpdateTime(0) {
  }

  ~CMovementGlobals() {
    if (movementLog) {
      fclose(movementLog);
    }
    movementLog = 0;
    if (fallingLog) {
      fclose(fallingLog);
    }
    fallingLog = 0;
  }

  char  logFileName[260];
  FILE *movementLog;
  FILE *fallingLog;
  LISTDECLEX(CMovementData, moveLink, movers);
  int              numMovers;
  UINT             ignoreObstacles : 1;
  CMovement       *currentLoading;
  CMovement       *m_localMover;
  CPlayerMoveQueue m_localMoveQueue;
  DWORD            m_lastUpdateTime;
};

class CMovement : public CMovementData {
 public:
  CMovement(const DWORDLONG &guid);
  void SetUpdateInfo(DWORD eventTime, const CClientMoveUpdate &init, int localPlayer);
  void GetMoveStatus(CMovementStatus *status) const;
  void UpdateTransportStatus(const CMovementStatus &update);
  void UpdateStatus(DWORD eventTime, const CMovementStatus &update);
  void UpdateStatusLocal(DWORD eventTime, const CMovementStatus &update);
  void OnTeleportLocal(DWORD eventTime, const NTempest::C3Vector &position, float facing);
  int  SetCollisionBox(const NTempest::CAaBox &box, float scale);

  float GetCurrentTurnRate() const;
  float GetCurrentPitchRate() const;

  void SetServerInitData(float const runSpeed, float const walkSpeed, float const swimSpeed, float const turnRate);

  static void         StartLogging();
  static void         StopLogging();
  static int          ToggleLogging();
  static int          IsLoggingOn();
  static void __cdecl LogWrite(LPCSTR format, ...);
  static void __cdecl BothLogWrite(LPCSTR format, ...);

  static void         StartFallLogging();
  static void         StopFallLogging();
  static int          ToggleFallLogging();
  static int          IsFallLoggingOn();
  static void __cdecl FallLogWrite(LPCSTR format, ...);

  static void StopAllLogging();
  static int  MoversOnList();
  static void MoveUnits(DWORD timeNow, DWORD lastUpdate);
  void        MoveUnit(DWORD timeNow, DWORD lastUpdate, LPVOID obj);
  void        MoveLocalPlayer(DWORD timeNow, DWORD lastUpdate);
  void        OnMoveStartLocal(DWORD eventTime, int forward);
  void        OnMoveStopLocal(DWORD eventTime);
  void        OnStrafeStartLocal(DWORD eventTime, int left);
  void        OnStrafeStopLocal(DWORD eventTime);
  void        OnJumpLocal(DWORD eventTime);
  void        OnFallLocal(DWORD eventTime);
  void        OnTurnStartLocal(DWORD eventTime, int left);
  void        OnTurnStopLocal(DWORD eventTime);
  void        OnPitchStartLocal(DWORD eventTime, int up);
  void        OnPitchStopLocal(DWORD eventTime);
  void        OnSetRunModeLocal(DWORD eventTime, int run);
  void        OnSetFacingLocal(DWORD eventTime, float facing);
  void        OnSetRawFacingLocal(DWORD eventTime, float facing);
  void        OnSetPitchLocal(DWORD eventTime, float pitch);
  void        OnSwimStartLocal(DWORD eventTime);
  void        OnSwimStopLocal(DWORD eventTime);
  void        OnMoveStart(DWORD eventTime, int forward);
  int         OnMoveStop(DWORD eventTime);
  void        OnStrafeStart(DWORD eventTime, int left);
  int         OnStrafeStop(DWORD eventTime);
  void        OnJump(DWORD eventTime);
  void        OnFall(DWORD eventTime);
  void        OnTurnStart(DWORD eventTime, int left);
  void        OnTurnStop(DWORD eventTime);
  void        OnPitchStart(DWORD eventTime, int up);
  void        OnPitchStop(DWORD eventTime);
  void        OnSetRunMode(DWORD eventTime, int run);
  void        OnTeleport(DWORD eventTime, const NTempest::C3Vector &position, float facing);
  void        OnSetFacing(DWORD eventTime, float facing);
  void        OnSetPitch(DWORD eventTime, float pitch);
  void        OnSwimStart(DWORD eventTime);
  void        OnSwimStop(DWORD eventTime);
  void        EnableCollision(DWORD eventTime, int enable);
  void        OnSpline(DWORD eventTime, const NTempest::C3Vector *points, UINT count, DWORD duration, UINT flags);
  void        OnSplineDoneFace(float facing);
  void        OnSplineDoneFace(const DWORDLONG &guid);
  void        OnSplineDoneFace(const NTempest::C3Vector &spot);
  int         OnRunSpeedChange(DWORD eventTime, float speed);
  int         OnWalkSpeedChange(DWORD eventTime, float speed);
  int         OnSwimSpeedChange(DWORD eventTime, float speed);
  int         OnTurnRateChange(DWORD eventTime, float rate);
  void        OnCollideRedirServer(DWORD eventTime, const NTempest::C3Vector &position, float facing, const NTempest::C3Vector &redirection);
  void        OnStuckServer(DWORD eventTime);
  float       GetCurrentSpeed();
  UINT        GetExportMoveFlags() const;
  UINT        GetLocalMoveFlags() const;
  int         IsSplineFlyer() const;
  float       FallDistance() const;
  UINT        FallTime() const;
  float       GetFallStartElevation() const;
  void        BuildMovementUpdate(CDataStore *msg) const;
  void        SetRawPosition(const NTempest::C3Vector &position);
  void        SetRawFacing(const float facing);
  void        Mobilize();
  void        Immobilize();
  void        Root();
  void        UnRoot();
  void        ToggleCollision(DWORD eventTime);
  int         IsSpline() const;
  void        GetUpdateInfo(CClientMoveUpdate *init) const;
  void        BuildFullZoneUpdate(CDataStore *msg);
  void        UnpackFullZoneUpdate(CDataStore *msg);
  static int  SkipFullZoneUpdate(CDataStore *msg);
  void        PutHandoffData(CDataStore *msg);
  void        GetHandoffData(CDataStore *msg);
  static void SkipHandoffData(CDataStore *msg);
  void        SetIdleUpdates();
  int         CollideRequestMove(DWORD lastUpdateTime, UINT timeElapsed, const NTempest::C3Vector &moveVector);
  int         GetMoveEventMsgId(UINT oldMoveFlags, int wasJumping);
  void        UpdateLastSentRedirection();

 private:
  friend class CGUnit_C;

  CMovement &operator=(const CMovement &);
  void       AddPlayerMoveEvent(DWORD eventTime, int eventType, float facing);
  int        UpdatePlayerMovement(DWORD timeNow);
  void       ApplyMovement(DWORD eventTime, UINT fallTime, UINT moveTime, UINT elapsed);
  int        PlotUnitMovement(UINT moveTime, NTempest::C3Vector *move);
  int        PlotUnitSplineMovement(DWORD eventTime, NTempest::C3Vector *move);
  void       GetMovingDirection(NTempest::C3Vector *direction) const;
  void       GetStrafingDirection(NTempest::C3Vector *direction) const;
  void       GetDiagonalDirection(NTempest::C3Vector *direction) const;
  void       GetMovingDirection2d(NTempest::C2Vector *direction) const;
  void       GetStrafingDirection2d(NTempest::C2Vector *direction) const;
  void       GetDiagonalDirection2d(NTempest::C2Vector *direction) const;
  void       GetDirection(NTempest::C3Vector *direction) const;
  void       PlotLinearPosition(const NTempest::C3Vector &direction, float secsElapsed, NTempest::C3Vector *totalMove);
  void       PlotHorzCircularPosition(const NTempest::C2Vector &direction2d, float secsElapsed, NTempest::C3Vector *totalMove);
  void       PlotVertCircularPosition(const NTempest::C2Vector &direction2d, float secsElapsed, NTempest::C3Vector *totalMove);
  void       PlotSpiralPosition(const NTempest::C2Vector &direction2d, float secsElapsed, NTempest::C3Vector *totalMove);
  void       PlotUnitRotation(float elapsedSec);
  void       PlotUnitPitch(float elapsedSec);
  void       PlotNormalLinearPosition(float secsElapsed, NTempest::C3Vector *totalMove);
  void       PlotStrafeLinearPosition(float secsElapsed, NTempest::C3Vector *totalMove);
  void       PlotDiagonalLinearPosition(float secsElapsed, NTempest::C3Vector *totalMove);
  void       PlotNormalCircularPosition(float secsElapsed, NTempest::C3Vector *totalMove);
  void       PlotStrafeCircularPosition(float secsElapsed, NTempest::C3Vector *totalMove);
  void       PlotDiagonalCircularPosition(float secsElapsed, NTempest::C3Vector *totalMove);
  void       PlotNormalPitchingCircularPosition(float secsElapsed, NTempest::C3Vector *totalMove);
  void       PlotDiagonalPitchingCircularPosition(float secsElapsed, NTempest::C3Vector *totalMove);
  void       PlotNormalSpiralPosition(float secsElapsed, NTempest::C3Vector *totalMove);
  void       PlotDiagonalSpiralPosition(float secsElapsed, NTempest::C3Vector *totalMove);
  int        CheckInvalidPositionOrMove(const NTempest::C3Vector &move, UINT moveTime);
  void       ApplyAdjustedMove(DWORD timeStemp, const NTempest::C3Vector &moveWanted, int wasAdjusted, UINT oldMoveFlags);
  void       SimpleRequestMove(UINT fallTime, const NTempest::C3Vector &moveVector);
  void       CallMoveEventHandlers(DWORD eventTime, int moveAdjusted, UINT oldMoveFlags, int wasJumping);
  void       SaveMoveState(CMoveState *state) const;
  void       RestoreMoveState(const CMoveState &state);
  void       LogUpdateInfo(const CClientMoveUpdate &init);
  void       Redirect(
      DWORD                     timeStamp,
      const NTempest::C3Vector &unitMoveVector,
      const NTempest::C3Vector &platformNorm,
      const CRedirect          &hitInfoX,
      const CRedirect          &hitInfoY
  );
  void Redirect(DWORD timeStamp, const NTempest::C3Vector &unitMoveVector, const NTempest::C3Vector &platformNorm, const CRedirect &hitInfo);
  void AttemptRedirect(DWORD timeStamp, const NTempest::C3Vector &unitMove, const NTempest::C3Vector &newDirection);
  void Obstruct(DWORD timeStamp, const NTempest::C3Vector &unitMove, const NTempest::C3Vector &platformNorm, const NTempest::C3Vector &facetNormHit);
  void Halt(DWORD eventTime);
  void UpdateAnchors(DWORD eventTime);
  void UpdateCurrentSpeed();
  int  StartMove(DWORD eventTime, int forward);
  int  StartStrafe(DWORD eventTime, int left);
  int  StopMove(DWORD eventTime);
  void ForceStopMove(DWORD eventTime);
  int  StopStrafe(DWORD eventTime);
  int  Jump(DWORD eventTime);
  void StartTurn(DWORD eventTime, int left);
  void StopTurn(DWORD eventTime);
  void StartPitch(DWORD eventTime, int up);
  void StopPitch(DWORD eventTime);
  void SetRunMode(DWORD eventTime, int run);
  void SetFacing(DWORD eventTime, float facing);
  void SetPitch(DWORD eventTime, float pitch);
  void Teleport(DWORD eventTime, const NTempest::C3Vector &position, float facing);
  void StartSwim(DWORD eventTime);
  void StopSwim(DWORD eventTime);
  void StartSwimLocal(DWORD eventTime);
  void StopSwimLocal(DWORD eventTime);
  void ForceStopStrafe(DWORD eventTime);
  int  ForceJump(DWORD eventTime);
  void OnDisableGravity(DWORD eventTime);
  void OnEnableGravity(DWORD eventTime);
  void CollisionStateChanged();
  void CollisionStateChangedLocal(DWORD eventTime);
  void StartFalling(DWORD eventTime);
  void StopFalling();
  void ProcessFallReset(DWORD eventTime);
  void CheckFallenFar(DWORD eventTime);
  void ProcessFalling(DWORD eventTime);
  int  HandlePendingActions(DWORD eventTime);
  void GetMoveFacets(float distance, UINT timeToFall, const NTempest::C3Vector &unitMove);
  void ShowCollisionBox(const NTempest::C3Vector &unitMove, UINT oldMoveFlags);
  void SetOrientation();
  NTempest::C3Vector CalcAverageSurfaceNormal(const NTempest::C4Plane *box, UINT count);
  UINT               Swim(DWORD eventTime, UINT timeToMove, const NTempest::C3Vector &moveWanted, const NTempest::C3Vector &unitMoveWanted);
  float              CollideWithWaterSurface(const NTempest::C3Vector &unitMove, const NTempest::C3Vector &unitMoveWanted, float distanceWanted);
  float              ExtrudeFlyBoxUp(const NTempest::C3Vector &unitMove, const NTempest::C3Vector &unitMoveWanted, float distanceWanted);
  float              ExtrudeFlyBoxDown(const NTempest::C3Vector &unitMove, const NTempest::C3Vector &unitMoveWanted, float distanceWanted);
  void  FlyRedirect(const NTempest::C3Vector &unitMoveWanted, const CRedirect &hitInfoX, const CRedirect &hitInfoY, const CRedirect &hitInfoZ);
  void  FlyRedirect(const NTempest::C3Vector &unitMoveWanted, const CRedirect &hitInfoX, const CRedirect &hitInfoY);
  UINT  ProjectileFall(DWORD eventTime, UINT timeToMove, const NTempest::C3Vector &moveWanted, const NTempest::C2Vector &unitMoveWanted);
  float ExtrudeProjectileBoxUpHill(const NTempest::C3Vector &unitMove, float distanceWanted, DWORDLONG *gameObjHit);
  float ExtrudeProjectileBoxDownHill(
      DWORD                     timeStamp,
      const NTempest::C3Vector &unitMove,
      float                     distanceWanted,
      const NTempest::C2Vector &unitMoveWanted,
      DWORDLONG                *gameObjHit
  );
  float ExtrudeAlignedDownHill(
      DWORD                     timeStamp,
      const NTempest::C3Vector &moveVector,
      float                     distanceWanted,
      const NTempest::C2Vector &unitMoveWanted,
      const NTempest::C3Vector &platformNorm
  );
  float ExtrudeAlignedUpHill(
      DWORD                     timeStamp,
      const NTempest::C3Vector &moveVector,
      float                     distanceWanted,
      const NTempest::C2Vector &unitMoveWanted,
      const NTempest::C3Vector &platformNorm
  );
  float ExtrudeUnalignedDownHill(
      DWORD                     timeStamp,
      const NTempest::C3Vector &moveVector,
      float                     distanceWanted,
      const NTempest::C2Vector &unitMoveWanted,
      const NTempest::C3Vector &platformNorm
  );
  float ExtrudeUnalignedUpHill(
      DWORD                     timeStamp,
      const NTempest::C3Vector &moveVector,
      float                     distanceWanted,
      const NTempest::C2Vector &unitMoveWanted,
      const NTempest::C3Vector &platformNorm
  );
  UINT  TraceSurface(DWORD eventTime, UINT timeToMove, float distance, const NTempest::C2Vector &unitMove, const NTempest::C2Vector &unitMoveWanted);
  UINT  Slide(UINT fallenSoFar, UINT timeIncrement);
  UINT  Fall(UINT fallenSoFar, UINT timeIncrement);
  float FindCeilingDistanceAbove(float distanceToJump);
  float FindGroundDistanceBelow(float distanceToFall, DWORDLONG *gameObjHit);
  void  ExtrudeDownNegXFacet(float distance, NTempest::C4Plane *const sides, NTempest::C4Plane *startPlane);
  void  ExtrudeDownPosXFacet(float distance, NTempest::C4Plane *const sides, NTempest::C4Plane *startPlane);
  void  ExtrudeDownNegYFacet(float distance, NTempest::C4Plane *const sides, NTempest::C4Plane *startPlane);
  void  ExtrudeDownPosYFacet(float distance, NTempest::C4Plane *const sides, NTempest::C4Plane *startPlane);
  float ExtrudeSlideBoxDownHill(const NTempest::C3Vector &unitMove, float distanceWanted, CRedirect *hitInfo);
  void  ExtrudeBoxSideZ(const NTempest::C3Vector &moveVector, float bottom, NTempest::C4Plane *const boxSides);
  void  ExtrudeBoxSideY(const NTempest::C3Vector &moveVector, float bottom, NTempest::C4Plane *const boxSides);
  void  ExtrudeBoxSideX(const NTempest::C3Vector &moveVector, float bottom, NTempest::C4Plane *const boxSides);
  int   ExtrudePyramidSideX(const NTempest::C3Vector &unitMove, float distance, NTempest::C4Plane *const boxSides);
  int   ExtrudePyramidSideY(const NTempest::C3Vector &unitMove, float distance, NTempest::C4Plane *const boxSides);
  float ExtrudeCollisionShape(
      DWORD                     timeStamp,
      const NTempest::C3Vector &moveVector,
      float                     distanceWanted,
      const NTempest::C2Vector &unitMoveWanted,
      const NTempest::C3Vector &platformNorm
  );
  int   TestStepUp(const NTempest::C3Vector &destination);
  float AttemptMove(
      DWORD                     eventTime,
      const NTempest::C3Vector &move,
      float                     distance2d,
      const NTempest::C2Vector &unitMove,
      const NTempest::C2Vector &unitMoveWanted,
      const NTempest::C4Plane  &ground
  );
  float CalcFallSurfaceProjection(
      const NTempest::C3Vector &position,
      DWORD                     moveStartTime,
      UINT                      timeLeft,
      NTempest::C3Vector        hitPoint,
      float                     distanceAway,
      const NTempest::C3Vector &moveNormal,
      NTempest::C4Plane        *platform
  );
  int  IsTooLow(const NTempest::C3Vector &position, DWORD moveStartTime, CWalkableSurface *surface, float distanceMoved, float currSpeedInv);
  void ClipFacetsWithOneAnother(const NTempest::C4Plane &startPlane, TSGrowableArray<CWalkableSurface> *surfacePool);
  int  NextSurfaceIsWalkable(
      CWalkableSurface                  *surface,
      DWORD                              eventTime,
      float                              distanceMoved,
      float                              currSpeedInv,
      TSGrowableArray<CWalkableSurface> *surfacePool
  );
  CWalkableSurface *GetNextSurface(
      const NTempest::C3Vector          &position,
      UINT                               surfaceId,
      DWORD                              eventTime,
      float                              distanceMoved,
      float                              currSpeedInv,
      const NTempest::C4Plane           &currentCeiling,
      TSGrowableArray<CWalkableSurface> *surfacePool
  );
  void CheckSurfaceObstacles(
      CWalkableSurface                  *surface,
      DWORD                              eventTime,
      float                             *distanceLeft,
      float                              distanceMoved,
      float                              currSpeedInv,
      TSGrowableArray<CWalkableSurface> *surfacePool,
      CRedirect                         *hitInfo
  );
  void FindObstacles(
      const NTempest::C3Vector &unitMoveVector,
      NTempest::C4Plane        *box,
      UINT                      numSides,
      const NTempest::C4Plane  &startPlane,
      int                       hitType,
      float                    *closestDist,
      CRedirect                *hitInfo
  );
  int   DetermineHitType(int hitType, const NTempest::C3Vector &unitMove, float distance, UINT facetId, CRedirect *hitInfo);
  void  DeterminePyramidHitType(const NTempest::C3Vector &unitMove, float distance, UINT facetId, CRedirect *hitInfo);
  int   DetermineBoxHitType(const NTempest::C3Vector &unitMove, float distance, UINT facetId, float baseHeight, CRedirect *hitInfo);
  int   IsJumpingUp(DWORD eventTime);
  int   IsRedirected() const;
  int   IsSliding() const;
  int   FallFromTransport();
  float RelDistanceFallen(DWORD currentTime, float updateFallTimeSecs);
  float RelDistanceFallen(UINT fallTimeMS);
  void  UpdateStatusInternal(DWORD eventTime, const CMovementStatus &update);
  void  AddToMoversList();
  void  AddSpline();
  float CalcFallStartElevation(UINT timeFallen);
};

LPVOID MovementGetGlobals();
void   MovementSetGlobals(LPVOID ptr);
void   MovementLockMoversList(int forWriting);
void   MovementUnlockMoversList(int fromWriting);
void   MovementDestroy();
void   MovementInitialize(LPCSTR logFileName, bool needLocalHeap);
int    MovementIdleMoveUnits(LPCVOID packetData, LPVOID param);
LPVOID MovementTryLock(DWORDLONG guid);
void   MovementUnlock(LPVOID obj);
void   MovementUpdateProxMap(LPVOID obj);
void   MovementMoveTransports(DWORD eventTime, float elapsed);

void               DisconnectLocalMover(CMovement *mover);
void               MovementGetTransportMtx(DWORDLONG transportGUID, NTempest::C34Matrix *transportMtx);
NTempest::C3Vector MovementGetTransportVector(DWORDLONG transportGUID);
float              MovementGetTransportFacing(DWORDLONG transportGUID);
int                MovementInsideTransport(DWORDLONG transportGUID, const NTempest::C3Vector &position);
void               MovementAddToTransport(CMovementData *mover, DWORDLONG transportGUID);
void               MovementFixUpMoveHistory(DWORDLONG mover, const NTempest::C34Matrix &fixup);
void               MovementUpdateCameraYaw(DWORDLONG transportGUID);
int                MovementGameObjIsTransport(DWORDLONG transportGUID);
void               MovementNotifyZoneMgr(DWORDLONG guid);
void               MovementFixOutOfBoundsUnit(DWORDLONG guid);
void               MovementSetGravityRate(float metersPerSecSqd);
void               MovementSetTerminalVelocity(float metersPerSec);
float              MovementGetTerminalVelocity();

#endif
