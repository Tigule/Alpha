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
  unsigned __int64   gameObjHit;
  unsigned char      flags;
};

struct CMoveState {
  NTempest::C3Vector position;
  float              facing;
  float              pitch;
  unsigned int       moveFlags;
  NTempest::C3Vector anchorPosition;
  float              anchorFacing;
  float              anchorPitch;
  unsigned long      moveStartTime;
  NTempest::C3Vector direction;
  NTempest::C2Vector direction2d;
  float              cosAnchorPitch;
  float              sinAnchorPitch;
  NTempest::C3Vector reDirection;
  unsigned long      fallStartTime;
  float              fallStartElevation;
  float              jumpVelocity;
};

namespace NTempest {
  class C34Matrix;
  class C4Plane;
}  // namespace NTempest

void OnPendingMoveStateChange(unsigned __int64 unit, int msgId, unsigned long eventTime);
void OnCollideRedirected(unsigned __int64 unit, unsigned long eventTime);
void OnCollideStuck(unsigned __int64 unit, unsigned long eventTime);
void OnCollideFallLand(unsigned __int64 unit, unsigned long eventTime);
void OnCollideFalling(unsigned __int64 unit, unsigned long eventTime);
void UnitNotifyStopped(const unsigned __int64 &guid, bool moveComplete);
int UnitGetObjectPosition(const unsigned __int64 &guid, NTempest::C3Vector *position);
float UnitCalculateFacingTo(const NTempest::C3Vector &position, const NTempest::C3Vector &destination);
enum FACET_COLOR {
  FACET_UNTESTED = 0,
  FACET_TESTED_UNTOUCHED = 1,
  FACET_TESTED_TOUCHED = 2,
  FACET_BLOCKING = 3,
  NUM_FACET_COLORS = 4
};

extern TSGrowableArray<unsigned short>      g_debugBoxIndices;
extern TSGrowableArray<NTempest::C3Vector>  g_debugBoxNormals;
extern TSGrowableArray<unsigned short>      g_debugIndices;
extern TSGrowableArray<NTempest::C3Vector>  g_debugNormalVerts;
extern TSGrowableArray<NTempest::C3Vector>  g_debugVerts;
extern TSGrowableArray<unsigned short>      g_debugNormalIndices;
extern TSGrowableArray<NTempest::C3Vector>  g_debugBoxVerts;
extern TSGrowableArray<NTempest::CImVector> g_debugVertColors;

void CollisionInfoSetWatchGUID(const unsigned __int64 &guid);
void CollisionInfoReset();
void CollisionInfoSetFaces(const unsigned __int64 &guid, const TSGrowableArray<NTempest::CFacet> &faces);
void CollisionInfoColorFace(unsigned int faceId, FACET_COLOR color);
void CollisionInfoSetFallBox(const NTempest::C3Vector &position, float boxHalfDepth, float boxHeight);
void CollisionInfoAddBox(const NTempest::C3Vector &boxMin, const NTempest::C3Vector &boxMax);
void CollisionInfoAddVector(const NTempest::C3Vector &position, const NTempest::C3Vector &vector);
int ToggleCollisionInfo();
void RenderCollisionInfo();
void ProcessLocalMoveEvent(unsigned int msgId);

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
  unsigned long   timeStamp;
  PLAYER_MOVE_EVT eventType;
  unsigned int    memHandle;
  float           facing;
};

class CPlayerMoveQueue {
 public:
  void              Enqueue(CPlayerMoveEvent *event);
  CPlayerMoveEvent *Root();
  void              Dequeue();
  unsigned char     HasEntries();
  void              DiscardAll();

 protected:
  friend class CMovement;
  friend void MovementDestroy();
  friend void DisconnectLocalMover(CMovement *);

  LISTDECL(CPlayerMoveEvent, m_events);
};

class CMovementData {
 public:
  CMovementData(const unsigned __int64 &guid);
  CMovementData(const NTempest::C3Vector &position, float facing, const unsigned __int64 &guid);
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
  unsigned long      GetMoveStartTime() const {
    return m_moveStartTime;
  }
  float GetRunSpeed() const;
  float GetWalkSpeed() const;
  float GetSwimSpeed() const;
  float GetTurnRate() const;
  unsigned int GetMoveFlags() const {
    return m_moveFlags;
  }
  unsigned __int64 GetGUID() const {
    return m_guid;
  }
  int              IsInMotion() const;
  int              IsMovingOrTurning() const;
  int              IsMovingAndTurning() const;
  int              IsMovingOrFalling() const;
  int              IsMovingOrStrafing() const;
  int              IsMovingStrafingOrFalling() const;
  int              IsMovingAndStrafing() const;
  int              IsMovingTurningOrStrafing() const;
  int              IsMoving() const;
  int              IsMovingForward() const;
  int              IsMovingBackwards() const;
  int              IsTurning() const;
  int              IsTurningOrFalling() const;
  int              IsTurningLeft() const;
  int              IsTurningRight() const;
  int              IsTurningOrPitching() const;
  int              IsTurningAndPitching() const;
  int              IsStrafingLeft() const;
  int              IsStrafingRight() const;
  int              IsStrafing() const;
  int              IsFalling() const;
  int              IsJumping() const;
  int              HasFallenFar() const;
  int              IsWalking() const;
  int              Moved() const;
  int              TimeIsValid() const;
  int              IsImmobilized() const;
  int              IsRooted() const;
  int              IsSwimming() const;
  int              IsSwimmingOrFalling() const;
  int              IsPitching() const;
  int              IsPitchingUp() const;
  int              IsPitchingDown() const;
  int              IsMovingStrafingOrSwimming() const;
  int              IsMovingStrafingFallingOrSwimming() const;
  int              IsSplineMover() const {
    return (m_moveFlags & 0x04000000) != 0;
  }
  int              IgnoresCollision() const;
  int              IsHalted() const;
  int              WasNudged() const;
  float              GetCollisionBoxHeight() const {
    return m_collisionBoxHeight;
  }
  void               SetWaterSurfaceElevation(float elevation);
  int                ForceSetTransport(unsigned __int64 guid);
  int                SetTransport(unsigned __int64 guid);
  void               RemoveFromMoversList();

  friend void OnMoveUpdate(unsigned __int64 unit, unsigned long eventTime);
  friend class CGUnit_C;
  friend class CGPlayer_C;
  friend class CGInputControl;
  friend class CGGameObject_C_Type_Chair;
  friend class CGGameObject_C_Type_MapObjTransport;
  friend class CGGameObject_C_Type_Transport;
  friend int MoveHeartBeatHandler(const void *packetData, void *param);
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
  NTempest::C3Vector      m_position;
  float                   m_facing;
  float                   m_pitch;
  NTempest::C3Vector      m_groundNormal;
  const unsigned __int64 &m_guid;
  unsigned __int64        m_transportGUID;
  unsigned int            m_moveFlags;
  NTempest::C3Vector      m_anchorPosition;
  float                   m_anchorFacing;
  float                   m_anchorPitch;
  unsigned long           m_moveStartTime;
  NTempest::C3Vector      m_direction;
  NTempest::C2Vector      m_direction2d;
  float                   m_cosAnchorPitch;
  float                   m_sinAnchorPitch;
  NTempest::C3Vector      m_reDirection;
  NTempest::C3Vector      m_lastReDirectionSent;
  unsigned long           m_fallStartTime;
  float                   m_fallStartElevation;
  float                   m_currentSpeed;
  float                   m_walkSpeed;
  float                   m_runSpeed;
  float                   m_swimSpeed;
  float                   m_turnRate;
  float                   m_collisionBoxHalfDepth;
  float                   m_collisionBoxHeight;
  float                   m_stepUpHeight;
  float                   m_jumpVelocity;
  CMoveSpline            *m_spline;
  float                   m_waterSurfaceElev;
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

  char                             logFileName[260];
  FILE                            *movementLog;
  FILE                            *fallingLog;
  LISTDECLEX(CMovementData, moveLink, movers);
  int                              numMovers;
  unsigned int                     ignoreObstacles : 1;
  CMovement                       *currentLoading;
  CMovement                       *m_localMover;
  CPlayerMoveQueue                 m_localMoveQueue;
  unsigned long                    m_lastUpdateTime;
};

class CMovement : public CMovementData {
 public:
  CMovement(const unsigned __int64 &guid);
  void SetUpdateInfo(unsigned long eventTime, const CClientMoveUpdate &init, int localPlayer);
  void GetMoveStatus(CMovementStatus *status) const;
  void UpdateTransportStatus(const CMovementStatus &update);
  void UpdateStatus(unsigned long eventTime, const CMovementStatus &update);
  void UpdateStatusLocal(unsigned long eventTime, const CMovementStatus &update);
  void OnTeleportLocal(unsigned long eventTime, const NTempest::C3Vector &position, float facing);
  int  SetCollisionBox(const NTempest::CAaBox &box, float scale);

  float GetCurrentTurnRate() const;
  float GetCurrentPitchRate() const;

  void SetServerInitData(float const runSpeed, float const walkSpeed, float const swimSpeed, float const turnRate);

  static void StartLogging();
  static void StopLogging();
  static int ToggleLogging();
  static int IsLoggingOn();
  static void __cdecl    LogWrite(const char *format, ...);
  static void __cdecl    BothLogWrite(const char *format, ...);

  static void StartFallLogging();
  static void StopFallLogging();
  static int ToggleFallLogging();
  static int IsFallLoggingOn();
  static void __cdecl    FallLogWrite(const char *format, ...);

  static void StopAllLogging();
  static int MoversOnList();
  static void MoveUnits(unsigned long timeNow, unsigned long lastUpdate);
  void                   MoveUnit(unsigned long timeNow, unsigned long lastUpdate, void *obj);
  void                   MoveLocalPlayer(unsigned long timeNow, unsigned long lastUpdate);
  void                   OnMoveStartLocal(unsigned long eventTime, int forward);
  void                   OnMoveStopLocal(unsigned long eventTime);
  void                   OnStrafeStartLocal(unsigned long eventTime, int left);
  void                   OnStrafeStopLocal(unsigned long eventTime);
  void                   OnJumpLocal(unsigned long eventTime);
  void                   OnFallLocal(unsigned long eventTime);
  void                   OnTurnStartLocal(unsigned long eventTime, int left);
  void                   OnTurnStopLocal(unsigned long eventTime);
  void                   OnPitchStartLocal(unsigned long eventTime, int up);
  void                   OnPitchStopLocal(unsigned long eventTime);
  void                   OnSetRunModeLocal(unsigned long eventTime, int run);
  void                   OnSetFacingLocal(unsigned long eventTime, float facing);
  void                   OnSetRawFacingLocal(unsigned long eventTime, float facing);
  void                   OnSetPitchLocal(unsigned long eventTime, float pitch);
  void                   OnSwimStartLocal(unsigned long eventTime);
  void                   OnSwimStopLocal(unsigned long eventTime);
  void                   OnMoveStart(unsigned long eventTime, int forward);
  int                    OnMoveStop(unsigned long eventTime);
  void                   OnStrafeStart(unsigned long eventTime, int left);
  int                    OnStrafeStop(unsigned long eventTime);
  void                   OnJump(unsigned long eventTime);
  void                   OnFall(unsigned long eventTime);
  void                   OnTurnStart(unsigned long eventTime, int left);
  void                   OnTurnStop(unsigned long eventTime);
  void                   OnPitchStart(unsigned long eventTime, int up);
  void                   OnPitchStop(unsigned long eventTime);
  void                   OnSetRunMode(unsigned long eventTime, int run);
  void                   OnTeleport(unsigned long eventTime, const NTempest::C3Vector &position, float facing);
  void                   OnSetFacing(unsigned long eventTime, float facing);
  void                   OnSetPitch(unsigned long eventTime, float pitch);
  void                   OnSwimStart(unsigned long eventTime);
  void                   OnSwimStop(unsigned long eventTime);
  void                   EnableCollision(unsigned long eventTime, int enable);
  void                   OnSpline(
      unsigned long eventTime, const NTempest::C3Vector *points, unsigned int count, unsigned long duration, unsigned int flags
  );
  void                   OnSplineDoneFace(float facing);
  void                   OnSplineDoneFace(const unsigned __int64 &guid);
  void                   OnSplineDoneFace(const NTempest::C3Vector &spot);
  int                    OnRunSpeedChange(unsigned long eventTime, float speed);
  int                    OnWalkSpeedChange(unsigned long eventTime, float speed);
  int                    OnSwimSpeedChange(unsigned long eventTime, float speed);
  int                    OnTurnRateChange(unsigned long eventTime, float rate);
  void                   OnCollideRedirServer(
      unsigned long eventTime, const NTempest::C3Vector &position, float facing, const NTempest::C3Vector &redirection
  );
  void                   OnStuckServer(unsigned long eventTime);
  float                  GetCurrentSpeed();
  unsigned int           GetExportMoveFlags() const;
  unsigned int           GetLocalMoveFlags() const;
  int                    IsSplineFlyer() const;
  float                  FallDistance() const;
  unsigned int           FallTime() const;
  float                  GetFallStartElevation() const;
  void                   BuildMovementUpdate(CDataStore *msg) const;
  void                   SetRawPosition(const NTempest::C3Vector &position);
  void                   SetRawFacing(const float facing);
  void                   Mobilize();
  void                   Immobilize();
  void                   Root();
  void                   UnRoot();
  void                   ToggleCollision(unsigned long eventTime);
  int                    IsSpline() const;
  void                   GetUpdateInfo(CClientMoveUpdate *init) const;
  void                   BuildFullZoneUpdate(CDataStore *msg);
  void                   UnpackFullZoneUpdate(CDataStore *msg);
  static int             SkipFullZoneUpdate(CDataStore *msg);
  void                   PutHandoffData(CDataStore *msg);
  void                   GetHandoffData(CDataStore *msg);
  static void            SkipHandoffData(CDataStore *msg);
  void                   SetIdleUpdates();
  int                    CollideRequestMove(
      unsigned long lastUpdateTime, unsigned int timeElapsed, const NTempest::C3Vector &moveVector
  );
  int                    GetMoveEventMsgId(unsigned int oldMoveFlags, int wasJumping);
  void                   UpdateLastSentRedirection();

 private:
  friend class CGUnit_C;

  CMovement &operator=(const CMovement &);
  void AddPlayerMoveEvent(unsigned long eventTime, int eventType, float facing);
  int  UpdatePlayerMovement(unsigned long timeNow);
  void ApplyMovement(unsigned long eventTime, unsigned int fallTime, unsigned int moveTime, unsigned int elapsed);
  int  PlotUnitMovement(unsigned int moveTime, NTempest::C3Vector *move);
  int  PlotUnitSplineMovement(unsigned long eventTime, NTempest::C3Vector *move);
  void GetMovingDirection(NTempest::C3Vector *direction) const;
  void GetStrafingDirection(NTempest::C3Vector *direction) const;
  void GetDiagonalDirection(NTempest::C3Vector *direction) const;
  void GetMovingDirection2d(NTempest::C2Vector *direction) const;
  void GetStrafingDirection2d(NTempest::C2Vector *direction) const;
  void GetDiagonalDirection2d(NTempest::C2Vector *direction) const;
  void GetDirection(NTempest::C3Vector *direction) const;
  void PlotLinearPosition(const NTempest::C3Vector &direction, float secsElapsed, NTempest::C3Vector *totalMove);
  void PlotHorzCircularPosition(const NTempest::C2Vector &direction2d, float secsElapsed, NTempest::C3Vector *totalMove);
  void PlotVertCircularPosition(const NTempest::C2Vector &direction2d, float secsElapsed, NTempest::C3Vector *totalMove);
  void PlotSpiralPosition(const NTempest::C2Vector &direction2d, float secsElapsed, NTempest::C3Vector *totalMove);
  void PlotUnitRotation(float elapsedSec);
  void PlotUnitPitch(float elapsedSec);
  void PlotNormalLinearPosition(float secsElapsed, NTempest::C3Vector *totalMove);
  void PlotStrafeLinearPosition(float secsElapsed, NTempest::C3Vector *totalMove);
  void PlotDiagonalLinearPosition(float secsElapsed, NTempest::C3Vector *totalMove);
  void PlotNormalCircularPosition(float secsElapsed, NTempest::C3Vector *totalMove);
  void PlotStrafeCircularPosition(float secsElapsed, NTempest::C3Vector *totalMove);
  void PlotDiagonalCircularPosition(float secsElapsed, NTempest::C3Vector *totalMove);
  void PlotNormalPitchingCircularPosition(float secsElapsed, NTempest::C3Vector *totalMove);
  void PlotDiagonalPitchingCircularPosition(float secsElapsed, NTempest::C3Vector *totalMove);
  void PlotNormalSpiralPosition(float secsElapsed, NTempest::C3Vector *totalMove);
  void PlotDiagonalSpiralPosition(float secsElapsed, NTempest::C3Vector *totalMove);
  int  CheckInvalidPositionOrMove(const NTempest::C3Vector &move, unsigned int moveTime);
  void ApplyAdjustedMove(unsigned long timeStemp, const NTempest::C3Vector &moveWanted, int wasAdjusted, unsigned int oldMoveFlags);
  void SimpleRequestMove(unsigned int fallTime, const NTempest::C3Vector &moveVector);
  void CallMoveEventHandlers(unsigned long eventTime, int moveAdjusted, unsigned int oldMoveFlags, int wasJumping);
  void SaveMoveState(CMoveState *state) const;
  void RestoreMoveState(const CMoveState &state);
  void LogUpdateInfo(const CClientMoveUpdate &init);
  void
  Redirect(
      unsigned long timeStamp,
      const NTempest::C3Vector &unitMoveVector,
      const NTempest::C3Vector &platformNorm,
      const CRedirect &hitInfoX,
      const CRedirect &hitInfoY
  );
  void  Redirect(
      unsigned long timeStamp,
      const NTempest::C3Vector &unitMoveVector,
      const NTempest::C3Vector &platformNorm,
      const CRedirect &hitInfo
  );
  void  AttemptRedirect(unsigned long timeStamp, const NTempest::C3Vector &unitMove, const NTempest::C3Vector &newDirection);
  void  Obstruct(
      unsigned long timeStamp,
      const NTempest::C3Vector &unitMove,
      const NTempest::C3Vector &platformNorm,
      const NTempest::C3Vector &facetNormHit
  );
  void  Halt(unsigned long eventTime);
  void  UpdateAnchors(unsigned long eventTime);
  void  UpdateCurrentSpeed();
  int   StartMove(unsigned long eventTime, int forward);
  int   StartStrafe(unsigned long eventTime, int left);
  int   StopMove(unsigned long eventTime);
  void  ForceStopMove(unsigned long eventTime);
  int   StopStrafe(unsigned long eventTime);
  int   Jump(unsigned long eventTime);
  void  StartTurn(unsigned long eventTime, int left);
  void  StopTurn(unsigned long eventTime);
  void  StartPitch(unsigned long eventTime, int up);
  void  StopPitch(unsigned long eventTime);
  void  SetRunMode(unsigned long eventTime, int run);
  void  SetFacing(unsigned long eventTime, float facing);
  void  SetPitch(unsigned long eventTime, float pitch);
  void  Teleport(unsigned long eventTime, const NTempest::C3Vector &position, float facing);
  void  StartSwim(unsigned long eventTime);
  void  StopSwim(unsigned long eventTime);
  void  StartSwimLocal(unsigned long eventTime);
  void  StopSwimLocal(unsigned long eventTime);
  void  ForceStopStrafe(unsigned long eventTime);
  int   ForceJump(unsigned long eventTime);
  void  OnDisableGravity(unsigned long eventTime);
  void  OnEnableGravity(unsigned long eventTime);
  void  CollisionStateChanged();
  void  CollisionStateChangedLocal(unsigned long eventTime);
  void  StartFalling(unsigned long eventTime);
  void  StopFalling();
  void  ProcessFallReset(unsigned long eventTime);
  void  CheckFallenFar(unsigned long eventTime);
  void  ProcessFalling(unsigned long eventTime);
  int   HandlePendingActions(unsigned long eventTime);
  void  GetMoveFacets(float distance, unsigned int timeToFall, const NTempest::C3Vector &unitMove);
  void  ShowCollisionBox(const NTempest::C3Vector &unitMove, unsigned int oldMoveFlags);
  void  SetOrientation();
  NTempest::C3Vector CalcAverageSurfaceNormal(const NTempest::C4Plane *box, unsigned int count);
  unsigned int       Swim(
      unsigned long eventTime,
      unsigned int timeToMove,
      const NTempest::C3Vector &moveWanted,
      const NTempest::C3Vector &unitMoveWanted
  );
  float              CollideWithWaterSurface(
      const NTempest::C3Vector &unitMove, const NTempest::C3Vector &unitMoveWanted, float distanceWanted
  );
  float              ExtrudeFlyBoxUp(
      const NTempest::C3Vector &unitMove, const NTempest::C3Vector &unitMoveWanted, float distanceWanted
  );
  float              ExtrudeFlyBoxDown(
      const NTempest::C3Vector &unitMove, const NTempest::C3Vector &unitMoveWanted, float distanceWanted
  );
  void               FlyRedirect(
      const NTempest::C3Vector &unitMoveWanted,
      const CRedirect &hitInfoX,
      const CRedirect &hitInfoY,
      const CRedirect &hitInfoZ
  );
  void               FlyRedirect(
      const NTempest::C3Vector &unitMoveWanted, const CRedirect &hitInfoX, const CRedirect &hitInfoY
  );
  unsigned int ProjectileFall(
      unsigned long eventTime,
      unsigned int timeToMove,
      const NTempest::C3Vector &moveWanted,
      const NTempest::C2Vector &unitMoveWanted
  );
  float        ExtrudeProjectileBoxUpHill(
      const NTempest::C3Vector &unitMove, float distanceWanted, unsigned __int64 *gameObjHit
  );
  float        ExtrudeProjectileBoxDownHill(
      unsigned long       timeStamp,
      const NTempest::C3Vector &unitMove,
      float               distanceWanted,
      const NTempest::C2Vector &unitMoveWanted,
      unsigned __int64   *gameObjHit
  );
  float        ExtrudeAlignedDownHill(
      unsigned long timeStamp,
      const NTempest::C3Vector &moveVector,
      float distanceWanted,
      const NTempest::C2Vector &unitMoveWanted,
      const NTempest::C3Vector &platformNorm
  );
  float        ExtrudeAlignedUpHill(
      unsigned long timeStamp,
      const NTempest::C3Vector &moveVector,
      float distanceWanted,
      const NTempest::C2Vector &unitMoveWanted,
      const NTempest::C3Vector &platformNorm
  );
  float        ExtrudeUnalignedDownHill(
      unsigned long timeStamp,
      const NTempest::C3Vector &moveVector,
      float distanceWanted,
      const NTempest::C2Vector &unitMoveWanted,
      const NTempest::C3Vector &platformNorm
  );
  float        ExtrudeUnalignedUpHill(
      unsigned long timeStamp,
      const NTempest::C3Vector &moveVector,
      float distanceWanted,
      const NTempest::C2Vector &unitMoveWanted,
      const NTempest::C3Vector &platformNorm
  );
  unsigned int
  TraceSurface(
      unsigned long eventTime,
      unsigned int timeToMove,
      float distance,
      const NTempest::C2Vector &unitMove,
      const NTempest::C2Vector &unitMoveWanted
  );
  unsigned int Slide(unsigned int fallenSoFar, unsigned int timeIncrement);
  unsigned int Fall(unsigned int fallenSoFar, unsigned int timeIncrement);
  float        FindCeilingDistanceAbove(float distanceToJump);
  float        FindGroundDistanceBelow(float distanceToFall, unsigned __int64 *gameObjHit);
  void         ExtrudeDownNegXFacet(float distance, NTempest::C4Plane *sides, NTempest::C4Plane *startPlane);
  void         ExtrudeDownPosXFacet(float distance, NTempest::C4Plane *sides, NTempest::C4Plane *startPlane);
  void         ExtrudeDownNegYFacet(float distance, NTempest::C4Plane *sides, NTempest::C4Plane *startPlane);
  void         ExtrudeDownPosYFacet(float distance, NTempest::C4Plane *sides, NTempest::C4Plane *startPlane);
  float        ExtrudeSlideBoxDownHill(const NTempest::C3Vector &unitMove, float distanceWanted, CRedirect *hitInfo);
  void         ExtrudeBoxSideZ(const NTempest::C3Vector &moveVector, float bottom, NTempest::C4Plane *boxSides);
  void         ExtrudeBoxSideY(const NTempest::C3Vector &moveVector, float bottom, NTempest::C4Plane *boxSides);
  void         ExtrudeBoxSideX(const NTempest::C3Vector &moveVector, float bottom, NTempest::C4Plane *boxSides);
  int          ExtrudePyramidSideX(const NTempest::C3Vector &unitMove, float distance, NTempest::C4Plane *boxSides);
  int          ExtrudePyramidSideY(const NTempest::C3Vector &unitMove, float distance, NTempest::C4Plane *boxSides);
  float        ExtrudeCollisionShape(
      unsigned long       timeStamp,
      const NTempest::C3Vector &moveVector,
      float               distanceWanted,
      const NTempest::C2Vector &unitMoveWanted,
      const NTempest::C3Vector &platformNorm
  );
  int   TestStepUp(const NTempest::C3Vector &destination);
  float AttemptMove(
      unsigned long       eventTime,
      const NTempest::C3Vector &move,
      float               distance2d,
      const NTempest::C2Vector &unitMove,
      const NTempest::C2Vector &unitMoveWanted,
      const NTempest::C4Plane  &ground
  );
  float CalcFallSurfaceProjection(
      const NTempest::C3Vector &position,
      unsigned long       moveStartTime,
      unsigned int        timeLeft,
      NTempest::C3Vector  hitPoint,
      float               distanceAway,
      const NTempest::C3Vector &moveNormal,
      NTempest::C4Plane  *platform
  );
  int  IsTooLow(
      const NTempest::C3Vector &position,
      unsigned long moveStartTime,
      CWalkableSurface *surface,
      float distanceMoved,
      float currSpeedInv
  );
  void ClipFacetsWithOneAnother(const NTempest::C4Plane &startPlane, TSGrowableArray<CWalkableSurface> *surfacePool);
  int  NextSurfaceIsWalkable(
      CWalkableSurface                  *surface,
      unsigned long                      eventTime,
      float                              distanceMoved,
      float                              currSpeedInv,
      TSGrowableArray<CWalkableSurface> *surfacePool
  );
  CWalkableSurface *GetNextSurface(
      const NTempest::C3Vector          &position,
      unsigned int                       surfaceId,
      unsigned long                      eventTime,
      float                              distanceMoved,
      float                              currSpeedInv,
      const NTempest::C4Plane           &currentCeiling,
      TSGrowableArray<CWalkableSurface> *surfacePool
  );
  void CheckSurfaceObstacles(
      CWalkableSurface                  *surface,
      unsigned long                      eventTime,
      float                             *distanceLeft,
      float                              distanceMoved,
      float                              currSpeedInv,
      TSGrowableArray<CWalkableSurface> *surfacePool,
      CRedirect                         *hitInfo
  );
  void FindObstacles(
      const NTempest::C3Vector &unitMoveVector,
      NTempest::C4Plane  *box,
      unsigned int        numSides,
      const NTempest::C4Plane &startPlane,
      int                 hitType,
      float              *closestDist,
      CRedirect          *hitInfo
  );
  int          DetermineHitType(
      int hitType, const NTempest::C3Vector &unitMove, float distance, unsigned int facetId, CRedirect *hitInfo
  );
  void         DeterminePyramidHitType(
      const NTempest::C3Vector &unitMove, float distance, unsigned int facetId, CRedirect *hitInfo
  );
  int          DetermineBoxHitType(
      const NTempest::C3Vector &unitMove, float distance, unsigned int facetId, float baseHeight, CRedirect *hitInfo
  );
  int          IsJumpingUp(unsigned long eventTime);
  int          IsRedirected() const;
  int          IsSliding() const;
  int          FallFromTransport();
  float        RelDistanceFallen(unsigned long currentTime, float updateFallTimeSecs);
  float        RelDistanceFallen(unsigned int fallTimeMS);
  void         UpdateStatusInternal(unsigned long eventTime, const CMovementStatus &update);
  void         AddToMoversList();
  void         AddSpline();
  float        CalcFallStartElevation(unsigned int timeFallen);
};

void *MovementGetGlobals();
void MovementSetGlobals(void *ptr);
void MovementLockMoversList(int forWriting);
void MovementUnlockMoversList(int fromWriting);
void MovementDestroy();
void MovementInitialize(const char *logFileName, bool needLocalHeap);
int MovementIdleMoveUnits(const void *packetData, void *param);
void *MovementTryLock(unsigned __int64 guid);
void MovementUnlock(void *obj);
void MovementUpdateProxMap(void *obj);
void MovementMoveTransports(unsigned long eventTime, float elapsed);

void DisconnectLocalMover(CMovement *mover);
void MovementGetTransportMtx(unsigned __int64 transportGUID, NTempest::C34Matrix *transportMtx);
NTempest::C3Vector MovementGetTransportVector(unsigned __int64 transportGUID);
float MovementGetTransportFacing(unsigned __int64 transportGUID);
int MovementInsideTransport(unsigned __int64 transportGUID, const NTempest::C3Vector &position);
void MovementAddToTransport(CMovementData *mover, unsigned __int64 transportGUID);
void MovementFixUpMoveHistory(unsigned __int64 mover, const NTempest::C34Matrix &fixup);
void MovementUpdateCameraYaw(unsigned __int64 transportGUID);
int MovementGameObjIsTransport(unsigned __int64 transportGUID);
void MovementNotifyZoneMgr(unsigned __int64 guid);
void MovementFixOutOfBoundsUnit(unsigned __int64 guid);
void  MovementSetGravityRate(float metersPerSecSqd);
void  MovementSetTerminalVelocity(float metersPerSec);
float MovementGetTerminalVelocity();

#endif
