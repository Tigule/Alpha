#ifndef WOW_SOURCE_OBJECT_MOVEMENTDATA_H
#define WOW_SOURCE_OBJECT_MOVEMENTDATA_H

#include <stdio.h>
#include <stpl.h>

#include "Object/Object.h"
#include "Tempest/c2vector.h"
#include "Tempest/c3vector.h"
#include "Tempest/cfacet.h"
#include "Tempest/cimvector.h"

class CMovement;
struct CWalkableSurface;
struct CRedirect {
  CRedirect();
  ~CRedirect();
  void Reset();

  NTempest::C3Vector hitPoint;
  NTempest::C3Vector surfaceNorm[2];
  unsigned __int64   gameObjHit;
  unsigned int       flags;
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

void __fastcall  OnPendingMoveStateChange(unsigned __int64 unit, int msgId, unsigned long eventTime);
void __fastcall  OnCollideRedirected(unsigned __int64 unit, unsigned long eventTime);
void __fastcall  OnCollideStuck(unsigned __int64 unit, unsigned long eventTime);
void __fastcall  OnCollideFallLand(unsigned __int64 unit, unsigned long eventTime);
void __fastcall  OnCollideFalling(unsigned __int64 unit, unsigned long eventTime);
void __fastcall  UnitNotifyStopped(const unsigned __int64 &guid, bool moveComplete);
int __fastcall   UnitGetObjectPosition(const unsigned __int64 &guid, NTempest::C3Vector *position);
float __fastcall UnitCalculateFacingTo(NTempest::C3Vector &position, NTempest::C3Vector &destination);
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

void __fastcall CollisionInfoSetWatchGUID(const unsigned __int64 &guid);
void __fastcall CollisionInfoReset();
void __fastcall CollisionInfoSetFaces(const unsigned __int64 &guid, const TSGrowableArray<NTempest::CFacet> &faces);
void __fastcall CollisionInfoColorFace(unsigned int faceId, FACET_COLOR color);
void __fastcall CollisionInfoSetFallBox(const NTempest::C3Vector &position, float boxHalfDepth, float boxHeight);
void __fastcall CollisionInfoAddBox(const NTempest::C3Vector &boxMin, const NTempest::C3Vector &boxMax);
void __fastcall CollisionInfoAddVector(const NTempest::C3Vector &position, const NTempest::C3Vector &vector);
int __fastcall  ToggleCollisionInfo();
void __fastcall RenderCollisionInfo();
void __fastcall ProcessLocalMoveEvent(unsigned int msgId);

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

struct CPlayerMoveEvent : public TSLinkedNode<CPlayerMoveEvent> {
  unsigned long   timeStamp;
  PLAYER_MOVE_EVT eventType;
  unsigned int    memHandle;
  float           facing;
};

class CPlayerMoveQueue {
 public:
  TSList<CPlayerMoveEvent, TSGetLink<CPlayerMoveEvent> > m_events;
};

class CMovementData {
 public:
  CMovementData(const NTempest::C3Vector &position, float facing, const unsigned __int64 &guid);
  ~CMovementData();
  NTempest::C3Vector GetPosition(const NTempest::C3Vector &position) const;
  float              GetFacing(float facing) const;
  int                ForceSetTransport(unsigned __int64 guid);
  int                SetTransport(unsigned __int64 guid);
  void               RemoveFromMoversList();

  friend void __fastcall OnMoveUpdate(unsigned __int64 unit, unsigned long eventTime);
  friend class CGUnit_C;

  TSLink<CMovementData> moveLink;
  TSLink<CMovementData> transportLink;

 protected:
  void CalcDirection();
  void RemoveSpline();

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

class CMovementGlobals {
 public:
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
  TSExplicitList<CMovementData, 0> movers;
  int                              numMovers;
  unsigned int                     ignoreObstacles : 1;
  CMovement                       *currentLoading;
  CMovement                       *m_localMover;
  CPlayerMoveQueue                 m_localMoveQueue;
  unsigned int                     m_lastUpdateTime;
};

class CMovement : public CMovementData {
 public:
  CMovement(const NTempest::C3Vector &position, float facing, const unsigned __int64 &guid);
  void SetUpdateInfo(unsigned long eventTime, CClientMoveUpdate &init, int localPlayer);
  int  IsLocalPlayer();
  void GetMoveStatus(CMovementStatus *status) const;
  void UpdateTransportStatus(const CMovementStatus &update);
  void UpdateStatusLocal(unsigned long eventTime, const CMovementStatus &update);

  float GetCurrentTurnRate() const;
  float GetCurrentPitchRate() const;

  void SetServerInitData(float const runSpeed, float const walkSpeed, float const swimSpeed, float const turnRate);

  static void __fastcall StartLogging();
  static void __fastcall StopLogging();
  static int __fastcall  ToggleLogging();
  static int __fastcall  IsLoggingOn();
  static void __cdecl    LogWrite(const char *format, ...);

  static void __fastcall StartFallLogging();
  static void __fastcall StopFallLogging();
  static int __fastcall  ToggleFallLogging();
  static int __fastcall  IsFallLoggingOn();
  static void __cdecl    FallLogWrite(const char *format, ...);

  static void __fastcall StopAllLogging();
  static int __fastcall  MoversOnList();
  static void __fastcall MoveUnits(unsigned long timeNow, unsigned long lastUpdate);
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
  void                   OnSetPitchLocal(unsigned long eventTime, float pitch);
  void                   OnSwimStartLocal(unsigned long eventTime);
  void                   OnSwimStopLocal(unsigned long eventTime);
  int                    OnRunSpeedChange(unsigned long eventTime, float speed);
  int                    OnWalkSpeedChange(unsigned long eventTime, float speed);
  int                    OnSwimSpeedChange(unsigned long eventTime, float speed);
  int                    OnTurnRateChange(unsigned long eventTime, float rate);
  void                   StartSwimLocal(unsigned long eventTime);
  void                   StopSwimLocal(unsigned long eventTime);

 private:
  friend class CGUnit_C;

  void AddPlayerMoveEvent(unsigned long eventTime, int eventType, float facing);
  int  UpdatePlayerMovement(unsigned long timeNow);
  void ApplyMovement(unsigned long eventTime, unsigned int fallTime, unsigned int moveTime, unsigned int elapsed);
  int  PlotUnitMovement(unsigned int moveTime, NTempest::C3Vector *move);
  int  PlotUnitSplineMovement(unsigned long eventTime, NTempest::C3Vector *move);
  void GetMovingDirection(NTempest::C3Vector *direction);
  void GetStrafingDirection(NTempest::C3Vector *direction);
  void GetDiagonalDirection(NTempest::C3Vector *direction);
  void GetMovingDirection2d(NTempest::C2Vector *direction);
  void GetStrafingDirection2d(NTempest::C2Vector *direction);
  void GetDiagonalDirection2d(NTempest::C2Vector *direction);
  void PlotLinearPosition(NTempest::C3Vector &direction, float secsElapsed, NTempest::C3Vector *totalMove);
  void PlotHorzCircularPosition(NTempest::C2Vector &direction2d, float secsElapsed, NTempest::C3Vector *totalMove);
  void PlotVertCircularPosition(NTempest::C2Vector &direction2d, float secsElapsed, NTempest::C3Vector *totalMove);
  void PlotSpiralPosition(NTempest::C2Vector &direction2d, float secsElapsed, NTempest::C3Vector *totalMove);
  void PlotUnitRotation(float elapsedSec);
  void PlotUnitPitch(float elapsedSec);
  int  CheckInvalidPositionOrMove(NTempest::C3Vector &move, unsigned int moveTime);
  void ApplyAdjustedMove(unsigned long timeStemp, NTempest::C3Vector &moveWanted, int wasAdjusted, unsigned int oldMoveFlags);
  void SimpleRequestMove(unsigned int fallTime, NTempest::C3Vector &moveVector);
  int  CollideRequestMove(unsigned long lastUpdateTime, unsigned int timeElapsed, NTempest::C3Vector &moveVector);
  void CallMoveEventHandlers(unsigned long eventTime, int moveAdjusted, unsigned int oldMoveFlags, int wasJumping);
  int  GetMoveEventMsgId(unsigned int oldMoveFlags, int wasJumping);
  void SaveMoveState(CMoveState *state);
  void RestoreMoveState(CMoveState &state);
  void
  Redirect(unsigned long timeStamp, NTempest::C3Vector &unitMoveVector, NTempest::C3Vector &platformNorm, CRedirect &hitInfoX, CRedirect &hitInfoY);
  void  Redirect(unsigned long timeStamp, NTempest::C3Vector &unitMoveVector, NTempest::C3Vector &platformNorm, CRedirect &hitInfo);
  void  AttemptRedirect(unsigned long timeStamp, NTempest::C3Vector &unitMove, NTempest::C3Vector &newDirection);
  void  Obstruct(unsigned long timeStamp, NTempest::C3Vector &unitMove, NTempest::C3Vector &platformNorm, NTempest::C3Vector &facetNormHit);
  void  Halt(unsigned long eventTime);
  void  UpdateAnchors(unsigned long eventTime);
  void  UpdateCurrentSpeed();
  float GetCurrentSpeed();
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
  void  StartSwim(unsigned long eventTime);
  void  StopSwim(unsigned long eventTime);
  void  StartFalling(unsigned long eventTime);
  void  StopFalling();
  void  ProcessFallReset(unsigned long eventTime);
  void  CheckFallenFar(unsigned long eventTime);
  void  ProcessFalling(unsigned long eventTime);
  int   HandlePendingActions(unsigned long eventTime);
  void  GetMoveFacets(float distance, unsigned int timeToFall, NTempest::C3Vector &unitMove);
  void  ShowCollisionBox(NTempest::C3Vector &unitMove, unsigned int oldMoveFlags);
  void  SetOrientation();
  NTempest::C3Vector CalcAverageSurfaceNormal(NTempest::C4Plane *box);
  unsigned int       Swim(unsigned long eventTime, unsigned int timeToMove, NTempest::C3Vector &moveWanted, NTempest::C3Vector &unitMoveWanted);
  float              CollideWithWaterSurface(NTempest::C3Vector &unitMove, NTempest::C3Vector &unitMoveWanted, float distanceWanted);
  float              ExtrudeFlyBoxUp(NTempest::C3Vector &unitMove, NTempest::C3Vector &unitMoveWanted, float distanceWanted);
  float              ExtrudeFlyBoxDown(NTempest::C3Vector &unitMove, NTempest::C3Vector &unitMoveWanted, float distanceWanted);
  void               FlyRedirect(NTempest::C3Vector &unitMoveWanted, CRedirect &hitInfoX, CRedirect &hitInfoY, CRedirect &hitInfoZ);
  void               FlyRedirect(NTempest::C3Vector &unitMoveWanted, CRedirect &hitInfoX, CRedirect &hitInfoY);
  unsigned int ProjectileFall(unsigned long eventTime, unsigned int timeToMove, NTempest::C3Vector &moveWanted, NTempest::C2Vector &unitMoveWanted);
  float        ExtrudeProjectileBoxUpHill(NTempest::C3Vector &unitMove, float distanceWanted, unsigned __int64 *gameObjHit);
  float        ExtrudeProjectileBoxDownHill(
      unsigned long       timeStamp,
      NTempest::C3Vector &unitMove,
      float               distanceWanted,
      NTempest::C2Vector &unitMoveWanted,
      unsigned __int64   *gameObjHit
  );
  unsigned int
  TraceSurface(unsigned long eventTime, unsigned int timeToMove, float distance, NTempest::C2Vector &unitMove, NTempest::C2Vector &unitMoveWanted);
  unsigned int Slide(unsigned int fallenSoFar, unsigned int timeIncrement);
  unsigned int Fall(unsigned int fallenSoFar, unsigned int timeIncrement);
  float        FindCeilingDistanceAbove(float distanceToJump);
  float        FindGroundDistanceBelow(float distanceToFall, unsigned __int64 *gameObjHit);
  void         ExtrudeDownNegXFacet(float distance, NTempest::C4Plane *sides, NTempest::C4Plane *startPlane);
  void         ExtrudeDownPosXFacet(float distance, NTempest::C4Plane *sides, NTempest::C4Plane *startPlane);
  void         ExtrudeDownNegYFacet(float distance, NTempest::C4Plane *sides, NTempest::C4Plane *startPlane);
  void         ExtrudeDownPosYFacet(float distance, NTempest::C4Plane *sides, NTempest::C4Plane *startPlane);
  float        ExtrudeSlideBoxDownHill(NTempest::C3Vector &unitMove, float distanceWanted, CRedirect *hitInfo);
  void         ExtrudeBoxSideZ(NTempest::C3Vector &moveVector, float bottom, NTempest::C4Plane *boxSides);
  void         ExtrudeBoxSideY(NTempest::C3Vector &moveVector, float bottom, NTempest::C4Plane *boxSides);
  void         ExtrudeBoxSideX(NTempest::C3Vector &moveVector, float bottom, NTempest::C4Plane *boxSides);
  int          ExtrudePyramidSideX(NTempest::C3Vector &unitMove, float distance, NTempest::C4Plane *boxSides);
  int          ExtrudePyramidSideY(NTempest::C3Vector &unitMove, float distance, NTempest::C4Plane *boxSides);
  float        ExtrudeCollisionShape(
      unsigned long       timeStamp,
      NTempest::C3Vector &moveVector,
      float               distanceWanted,
      NTempest::C2Vector &unitMoveWanted,
      NTempest::C3Vector &platformNorm
  );
  int   TestStepUp(NTempest::C3Vector &destination);
  float AttemptMove(
      unsigned long       eventTime,
      NTempest::C3Vector &move,
      float               distance2d,
      NTempest::C2Vector &unitMove,
      NTempest::C2Vector &unitMoveWanted,
      NTempest::C4Plane  &ground
  );
  float CalcFallSurfaceProjection(
      NTempest::C3Vector &position,
      unsigned long       moveStartTime,
      unsigned int        timeLeft,
      NTempest::C3Vector  hitPoint,
      float               distanceAway,
      NTempest::C3Vector &moveNormal,
      NTempest::C4Plane  *platform
  );
  int  IsTooLow(NTempest::C3Vector &position, unsigned long moveStartTime, CWalkableSurface *surface, float distanceMoved, float currSpeedInv);
  void ClipFacetsWithOneAnother(NTempest::C4Plane &startPlane, TSGrowableArray<CWalkableSurface> *surfacePool);
  int  NextSurfaceIsWalkable(
      CWalkableSurface                  *surface,
      unsigned long                      eventTime,
      float                              distanceMoved,
      float                              currSpeedInv,
      TSGrowableArray<CWalkableSurface> *surfacePool
  );
  CWalkableSurface *GetNextSurface(
      NTempest::C3Vector                &position,
      unsigned int                       surfaceId,
      unsigned long                      eventTime,
      float                              distanceMoved,
      float                              currSpeedInv,
      NTempest::C4Plane                 &currentCeiling,
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
      NTempest::C3Vector &unitMoveVector,
      NTempest::C4Plane  *box,
      unsigned int        numSides,
      NTempest::C4Plane  &startPlane,
      int                 hitType,
      float              *closestDist,
      CRedirect          *hitInfo
  );
  int          DetermineHitType(int hitType, NTempest::C3Vector &unitMove, float distance, unsigned int facetId, CRedirect *hitInfo);
  void         DeterminePyramidHitType(NTempest::C3Vector &unitMove, float distance, unsigned int facetId, CRedirect *hitInfo);
  int          DetermineBoxHitType(NTempest::C3Vector &unitMove, float distance, unsigned int facetId, float baseHeight, CRedirect *hitInfo);
  int          IsJumpingUp(unsigned long eventTime);
  int          FallFromTransport();
  float        RelDistanceFallen(unsigned long currentTime, float updateFallTimeSecs);
  float        RelDistanceFallen(unsigned int fallTimeMS);
  unsigned int FallTime() const;
  void         UpdateStatusInternal(unsigned long eventTime, const CMovementStatus &update);
  void         SetIdleUpdates();
  void         AddToMoversList();
  void         AddSpline();
  float        CalcFallStartElevation(unsigned int timeFallen);
};

void *__fastcall MovementGetGlobals();
void __fastcall  MovementSetGlobals(void *ptr);
void __fastcall  MovementLockMoversList(int forWriting);
void __fastcall  MovementUnlockMoversList(int fromWriting);
void __fastcall  MovementDestroy();
void __fastcall  MovementInitialize(const char *logFileName, bool needLocalHeap);
int __fastcall   MovementIdleMoveUnits(const void *packetData, void *param);
void *__fastcall MovementTryLock(unsigned __int64 guid);
void __fastcall  MovementUnlock(void *obj);
void __fastcall  MovementUpdateProxMap(void *obj);
void __fastcall  MovementMoveTransports(unsigned long eventTime, float elapsed);

void __fastcall  DisconnectLocalMover(CMovement *mover);
void __fastcall  MovementGetTransportMtx(unsigned __int64 transportGUID, NTempest::C34Matrix *transportMtx);
float __fastcall MovementGetTransportFacing(unsigned __int64 transportGUID);
void __fastcall  MovementAddToTransport(CMovementData *mover, unsigned __int64 transportGUID);
void __fastcall  MovementFixUpMoveHistory(unsigned __int64 mover, const NTempest::C34Matrix &fixup);
int __fastcall   MovementGameObjIsTransport(unsigned __int64 transportGUID);
void __fastcall  MovementNotifyZoneMgr(unsigned __int64 guid);
void __fastcall  MovementFixOutOfBoundsUnit(unsigned __int64 guid);
void __stdcall   MovementSetGravityRate(float metersPerSecSqd);
void __stdcall   MovementSetTerminalVelocity(float metersPerSec);
float            MovementGetTerminalVelocity();

#endif
