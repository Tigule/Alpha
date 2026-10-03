#include <Base/Base.h>
#include <Gx/Gx.h>
#include <WowConst.h>
#include <MapDefs.h>

#include "MovementData.h"

#include "Tempest/c34matrix.h"
#include "Tempest/caabox.h"
#include "Tempest/c4plane.h"
#include "Tempest/cfacet.h"
#include "WorldClient/World.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

using namespace NTempest;

struct CWalkableSurface {
  float              closeDist;
  float              farDist;
  C3Vector firstPtOfContact;
  C3Vector lastPtOfContact;
  UINT               facetId;
  float              highestElevation;

  static BYTE HasHigherPriority(const CWalkableSurface &, const CWalkableSurface &);
};

class CClippedTriangle {
 public:
  CClippedTriangle() : numVerts(3) {
  }

  CClippedTriangle(const CClippedTriangle &triangle) : numVerts(triangle.numVerts) {
    memcpy(verts, triangle.verts, sizeof(C3Vector) * numVerts);
  }

  C3Vector *Ptr() {
    return verts;
  }

  C3Vector &operator[](UINT index) {
    FATALASSERT(index < numVerts);
    return verts[index];
  }

  const C3Vector &operator[](UINT index) const {
    FATALASSERT(index < numVerts);
    return verts[index];
  }

  const C3Vector &Last() const {
    FATALASSERT(numVerts > 0);
    return verts[numVerts - 1];
  }

  void SetCount(UINT count) {
    FATALASSERT(count < 9);
    numVerts = count;
  }

  UINT Count() const {
    return numVerts;
  }

  void Init(const C3Vector *vertices) {
    numVerts = 3;
    verts[0] = vertices[0];
    verts[1] = vertices[1];
    verts[2] = vertices[2];
  }

  void Add(const C3Vector &vertex) {
    FATALASSERT(numVerts < 9);
    verts[numVerts++] = vertex;
  }

  CClippedTriangle &operator=(const CClippedTriangle &triangle) {
    memcpy(verts, triangle.verts, sizeof(C3Vector) * triangle.numVerts);
    numVerts = triangle.numVerts;
    return *this;
  }

 private:
  C3Vector verts[9];
  UINT               numVerts;
};

enum {
  NUM_HITTYPES = 3
};

static CWFacetData                       s_facetData;
static TSGrowableArray<CWalkableSurface> surfacePool;

inline BYTE CWalkableSurface::HasHigherPriority(const CWalkableSurface &a, const CWalkableSurface &b) {
  if (!CMath::fequalz_(a.closeDist, b.closeDist, 0.0013888889f)) {
    return a.closeDist <= b.closeDist;
  }
  if (!CMath::fequalz_(a.firstPtOfContact.z, b.firstPtOfContact.z, 0.001f)) {
    return a.firstPtOfContact.z >= b.firstPtOfContact.z;
  }
  if (CMath::fequalz_(a.farDist, b.farDist, 0.0013888889f)) {
    return a.lastPtOfContact.z >= b.lastPtOfContact.z;
  }
  if (a.farDist < b.farDist) {
    const C4Plane &plane = s_facetData.facets[b.facetId].plane;
    float          elevation = plane.SolveForZ(a.lastPtOfContact.x, a.lastPtOfContact.y);
    return elevation <= a.lastPtOfContact.z;
  }
  const C4Plane &plane = s_facetData.facets[a.facetId].plane;
  float          elevation = plane.SolveForZ(b.lastPtOfContact.x, b.lastPtOfContact.y);
  return elevation >= b.lastPtOfContact.z;
}

static float s_gravityRate = 19.291105f;
static float s_terminalVelocity = 60.148003f;

static void LogHitInfoFlags(UINT flags);
static int GetSlidingDirection(DWORDLONG guid, const C3Vector *normalList, UINT numNormals, C3Vector *direction);
static C3Vector ComputeFlyRedirection(const C3Vector *normals, UINT numNormals, const C3Vector &unitMoveWanted);
static void InsertSurface(const CWalkableSurface &toBeInserted, UINT startIndex, TSGrowableArray<CWalkableSurface> *surfacePool);
static void SplitFacetWithFacet(CWalkableSurface *beingSplit, CWalkableSurface *splitBy, CWalkableSurface *newSurface);
static void LogSurface(DWORDLONG guid, const CWalkableSurface &surface);
static void EnqueueFacets(
    const C4Plane                 &slopeTestPlane,
    const C2Vector                &position,
    const C2Vector                &unitMove,
    const TSGrowableArray<CFacet> &facets,
    TSGrowableArray<CWalkableSurface>       *surfacePool
);
static float ClipPolygonToPlane(const C4Plane &plane, CClippedTriangle *poly);
static int __cdecl FacetCompare(LPCVOID elem1, LPCVOID elem2);
static BOOL PolygonIntersectsPlane(const C4Plane &plane, const CClippedTriangle &poly);
static BOOL ClipPolygonToPolyhedron(const C4Plane *boxSides, UINT numSides, CClippedTriangle *poly, float *penetrationDepth);
static void DetermineBoxParallelHitType(const C4Plane *boxSides, const CClippedTriangle &clippedPoly, CRedirect *hitInfo);
static float FindClosestVertDist(const C4Plane &front, const C4Plane *sides, const CClippedTriangle &poly);
static float DistFromPlaneAlongVector(const C3Vector &point, const C4Plane &plane, const C3Vector &unitVector);
static float GetDist2d(const C3Vector &unitMove, float closestDist3D);

float CMovement::CalcFallStartElevation(UINT timeFallen) {
  float jumpVelocity = m_jumpVelocity;
  float fallTime = timeFallen * 0.001f;
  float velocity = s_gravityRate * fallTime;

  if (velocity + jumpVelocity > s_terminalVelocity) {
    float terminalTime = (s_terminalVelocity - jumpVelocity) / s_gravityRate;
    return (fallTime - terminalTime + terminalTime * 0.5f) * s_terminalVelocity + m_position.z;
  }
  return (jumpVelocity + velocity * 0.5f) * fallTime + m_position.z;
}

void MovementSetGravityRate(float metersPerSecSqd) {
  s_gravityRate = metersPerSecSqd * 1.0936f;
}

void MovementSetTerminalVelocity(float metersPerSec) {
  s_terminalVelocity = metersPerSec * 1.0936f;
}

float MovementGetTerminalVelocity() {
  return s_terminalVelocity * 0.91439998f;
}

void CMovement::Redirect(
    DWORD                     timeStamp,
    const C3Vector &unitMoveVector,
    const C3Vector &platformNorm,
    const CRedirect          &hitInfoX,
    const CRedirect          &hitInfoY
) {
  if (!hitInfoX.flags && !hitInfoY.flags) {
    return;
  }

  FallLogWrite("0x%016I64X: Hit info flags X:", GetGUID());
  LogHitInfoFlags(hitInfoX.flags);
  FallLogWrite(" Y:");
  LogHitInfoFlags(hitInfoY.flags);
  FallLogWrite("\n");

  UINT typeX = hitInfoX.flags & 0x1F;
  UINT typeY = hitInfoY.flags & 0x1F;
  int  corner =
      (((typeX == 8 || typeX == 16) && !typeY) || ((typeY == 8 || typeY == 16) && !typeX) || (typeX == 4 && !typeY) || (typeY == 4 && !typeX) ||
       (typeX == 4 && typeY == 4));
  if (corner) {
    FallLogWrite("0x%016I64X: Corner of box hit obstacle, redirecting along obstacle\n", GetGUID());
    const C3Vector *newDirection;
    if (hitInfoX.flags && hitInfoY.flags) {
      if (C3Vector::Dot(hitInfoX.surfaceNorm[0], unitMoveVector) < C3Vector::Dot(hitInfoY.surfaceNorm[0], unitMoveVector)) {
        newDirection = &hitInfoX.surfaceNorm[0];
      } else {
        newDirection = &hitInfoY.surfaceNorm[0];
      }
    } else if (hitInfoX.flags) {
      newDirection = &hitInfoX.surfaceNorm[0];
    } else {
      newDirection = &hitInfoY.surfaceNorm[0];
    }
    Obstruct(timeStamp, unitMoveVector, platformNorm, *newDirection);
  } else if ((hitInfoX.flags & 0x1B) && (hitInfoY.flags & 0x1B)) {
    FallLogWrite("0x%016I64X: Both box sides hit obstacles, so stuck\n", GetGUID());
    Halt(timeStamp);
  } else {
    FallLogWrite("0x%016I64X: Hit obstacle corner, or multiple per box side, redirecting along box side\n", GetGUID());
    C3Vector newDirection;
    if (hitInfoX.flags & 0x1B) {
      newDirection.Set(0.0f, 1.0f, 0.0f);
    } else {
      newDirection.Set(1.0f, 0.0f, 0.0f);
    }
    if (C3Vector::Dot(newDirection, unitMoveVector) < 0.0f) {
      newDirection = -newDirection;
    }
    AttemptRedirect(timeStamp, unitMoveVector, newDirection);
  }
}

static void LogHitInfoFlags(UINT flags) {
  if (!flags) {
    CMovement::FallLogWrite(" (no hits)");
    return;
  }
  if (flags & 1) {
    CMovement::FallLogWrite(" (hit box side 1)");
  }
  if (flags & 2) {
    CMovement::FallLogWrite(" (hit box side 2)");
  }
  if (flags & 8) {
    CMovement::FallLogWrite(" (hit box corner 1)");
  }
  if (flags & 0x10) {
    CMovement::FallLogWrite(" (hit box corner 2)");
  }
  if (flags & 4) {
    CMovement::FallLogWrite(" (hit box shared corner)");
  }
  if (flags & 0x40) {
    CMovement::FallLogWrite(" (hit pyramid bottom)");
  }
  if (flags & 0x80) {
    CMovement::FallLogWrite(" (hit multiple surfaces)");
  }
}

void CMovement::Redirect(
    DWORD                     timeStamp,
    const C3Vector &unitMoveVector,
    const C3Vector &platformNorm,
    const CRedirect          &hitInfo
) {
  if (!hitInfo.flags) {
    return;
  }

  FallLogWrite("0x%016I64X: Hit info flags: ", GetGUID());
  LogHitInfoFlags(hitInfo.flags);
  FallLogWrite("\n");

  UINT hitType = hitInfo.flags & 0x1F;
  if (hitType == 4 || hitType == 8 || hitType == 16) {
    FallLogWrite("0x%016I64X: Box corner hit obstacle, redirecting along obstacle\n", GetGUID());
    Obstruct(timeStamp, unitMoveVector, platformNorm, hitInfo.surfaceNorm[0]);
  } else {
    FallLogWrite("0x%016I64X: Hit box face head-on, or hit obstacle corner, so stuck\n", GetGUID());
    Halt(timeStamp);
  }
}

void CMovement::AttemptRedirect(DWORD timeStamp, const C3Vector &unitMove, const C3Vector &newDirection) {
  C2Vector newDirection2d = newDirection;
  newDirection2d.SafeNormalize();

  if (!(m_moveFlags & 0x1000)) {
    m_reDirection = newDirection2d;
    m_moveFlags |= 0x1000;
    float cosTheta = C3Vector::Dot(m_reDirection, unitMove);
    CMath::clamp_x(cosTheta, -1.0f, 1.0f);
    FallLogWrite(
        "0x%016I64X: wanted (%g,%g,%g), setting redirection (%g,%g) (%g deg off)\n", m_guid, unitMove.x, unitMove.y, unitMove.z, m_reDirection.x,
        m_reDirection.y, CMath::acos_(cosTheta) * 57.29578f
    );
  } else if (CMath::fequal4_(m_reDirection.x, newDirection2d.x) && CMath::fequal4_(m_reDirection.y, newDirection2d.y)) {
    FallLogWrite("0x%016I64X: redirected direction unchanged\n", m_guid);
  } else {
    float newCross = newDirection2d.x * unitMove.y - newDirection2d.y * unitMove.x;
    float cosTheta1 = C2Vector::Dot(m_reDirection, unitMove);
    float cosTheta2 = C2Vector::Dot(newDirection2d, unitMove);
    if (((m_reDirection.x * unitMove.y - m_reDirection.y * unitMove.x < 0.0f) ^ (newCross >= 0.0f)) && cosTheta2 - cosTheta1 < 0.00000095367432f) {
      m_reDirection = newDirection2d;
      CMath::clamp_x(cosTheta1, -1.0f, 1.0f);
      CMath::clamp_x(cosTheta2, -1.0f, 1.0f);
      FallLogWrite(
          "0x%016I64X: Already redirected (%g,%g) (%g deg off), new direction (%g,%g) (%g deg off) is sharper\n", m_guid, m_reDirection.x,
          m_reDirection.y, CMath::acos_(cosTheta1) * 57.29578f, newDirection2d.x, newDirection2d.y, CMath::acos_(cosTheta2) * 57.29578f
      );
    } else {
      Halt(timeStamp);
      CMath::clamp_x(cosTheta1, -1.0f, 1.0f);
      cosTheta2 = CMath::clamp_(cosTheta1, -1.0f, 1.0f);
      FallLogWrite(
          "0x%016I64X: Already redirected (%g,%g) (%g deg off), new direction (%g,%g) (%g deg off) turns back into first "
          "obstacle, stopping\n",
          m_guid, m_reDirection.x, m_reDirection.y, CMath::acos_(cosTheta1) * 57.29578f, newDirection2d.x, newDirection2d.y, CMath::acos_(cosTheta2) * 57.29578f
      );
    }
  }
}

void CMovement::Obstruct(DWORD timeStamp, const C3Vector &unitMove, const C3Vector &platformNorm, const C3Vector &facetNormHit) {
  if (CMath::fequal4_(facetNormHit.Mag(), 0.0f)) {
    FallLogWrite("0x%016I64X: Obstacle hit is horizontal, can't redirect, stopping\n", GetGUID());
    Halt(timeStamp);
    return;
  }

  if (C3Vector::Dot(facetNormHit, unitMove) > -0.0013888889f) {
    FallLogWrite("0x%016I64X: Obstacle hit faces away from movement vector, stopping\n", GetGUID());
    Halt(timeStamp);
    return;
  }

  if (!(m_moveFlags & 0xF)) {
    this ? SErrDisplayErrorFmt(
               STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
               "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
               "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
               "IsMovingOrStrafing()", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
               static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
               static_cast<int>(GetFacing())
           )
         : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "IsMovingOrStrafing()", FALSE, 1);
  }

  C3Vector planeIntersect = C3Vector::Cross(platformNorm, facetNormHit);
  planeIntersect.SafeNormalize();
  float cosTheta = C3Vector::Dot(planeIntersect, unitMove);
  if (cosTheta < 0.0f) {
    planeIntersect = -planeIntersect;
  }
  if (CMath::fequal4_(planeIntersect.SquaredMag(), 0.0f) || (CMath::fabs_(cosTheta) < 0.017452406f && (m_moveFlags & 0x4000))) {
    FallLogWrite("0x%016I64X: new redirected direction is horizontal, stopping\n", GetGUID());
    Halt(timeStamp);
    return;
  }
  AttemptRedirect(timeStamp, unitMove, planeIntersect);
  if (m_moveFlags & 0xF) {
    FallLogWrite("0x%016I64X: Setting redirected (" __FILE__ ": %d)\n", GetGUID(), __LINE__);
  }
}

void CMovement::CallMoveEventHandlers(DWORD eventTime, int moveAdjusted, UINT oldMoveFlags, BOOL wasJumping) {
  if (IsSpline() && IsMoving() && (m_spline->flags & 1)) {
    ForceStopMove(eventTime);
    if (m_spline->flags & 0x40000) {
      m_facing = m_spline->face.facing;
    } else if (m_spline->flags & 0x20000) {
      C3Vector spot;
      if (UnitGetObjectPosition(m_spline->face.guid, &spot)) {
        m_facing = UnitCalculateFacingTo(m_position, spot);
      }
    } else if (m_spline->flags & 0x10000) {
      m_facing = UnitCalculateFacingTo(m_position, m_spline->face.spot);
    }
    m_spline->flags |= 4;
    moveAdjusted = 1;
  }

  if (moveAdjusted) {
    if ((m_moveFlags ^ oldMoveFlags) & 0xF) {
      OnPendingMoveStateChange(GetGUID(), GetMoveEventMsgId(oldMoveFlags, wasJumping), eventTime);
      if (!(m_moveFlags & 0xF)) {
        UnitNotifyStopped(GetGUID(), m_spline && (m_spline->flags & 1));
      }
    } else if (m_moveFlags & 0x10000000) {
      if (!(oldMoveFlags & 0x10000000)) {
        OnCollideStuck(GetGUID(), eventTime);
      }
    } else if (m_moveFlags & 0xF) {
      OnCollideRedirected(GetGUID(), eventTime);
    }
  }

  if (m_moveFlags & 0x4000) {
    if ((m_moveFlags & 0x8000) && !(oldMoveFlags & 0x8000)) {
      OnCollideFalling(GetGUID(), eventTime);
    }
  } else if (oldMoveFlags & 0x4000) {
    OnCollideFallLand(GetGUID(), eventTime);
  }

  if (!(m_moveFlags & 0x02000000) && (oldMoveFlags & 0x02000000) && IsLocalPlayer()) {
    ProcessLocalMoveEvent(204);
  }
}

float CMovement::RelDistanceFallen(DWORD currentTime, float updateFallTimeSecs) {
  float lastFallTime;
  float fallStartElevation;
  if ((m_moveFlags & 0x4000) && currentTime != m_fallStartTime) {
    lastFallTime = static_cast<int>(currentTime - m_fallStartTime) * 0.001f;
    if (!(lastFallTime >= 0.0f)) {
      this ? SErrDisplayErrorFmt(
                 STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
                 "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
                 "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
                 "lastFallTime >= 0.0f", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
                 static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
                 static_cast<int>(GetFacing())
             )
           : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "lastFallTime >= 0.0f", FALSE, 1);
    }
    fallStartElevation = m_fallStartElevation;
  } else {
    fallStartElevation = m_position.z;
    lastFallTime = 0.0f;
  }

  float jumpVelocity = m_jumpVelocity;
  float fallTime = lastFallTime + updateFallTimeSecs;
  float velocity = s_gravityRate * fallTime;
  float distance;
  if (velocity + jumpVelocity > s_terminalVelocity) {
    float terminalTime = (s_terminalVelocity - jumpVelocity) / s_gravityRate;
    distance = (fallTime - terminalTime + terminalTime * 0.5f) * s_terminalVelocity;
  } else {
    distance = (jumpVelocity + velocity * 0.5f) * fallTime;
  }
  return m_position.z - (fallStartElevation - distance);
}

float CMovement::RelDistanceFallen(UINT fallTimeMS) {
  float jumpVelocity = m_jumpVelocity;
  float fallTime = fallTimeMS * 0.001f;
  float velocity = s_gravityRate * fallTime;
  float distance;
  if (velocity + jumpVelocity > s_terminalVelocity) {
    float terminalTime = (s_terminalVelocity - jumpVelocity) / s_gravityRate;
    distance = (fallTime - terminalTime + terminalTime * 0.5f) * s_terminalVelocity;
  } else {
    distance = (jumpVelocity + velocity * 0.5f) * fallTime;
  }
  float startElevation = m_moveFlags & 0x4000 ? m_fallStartElevation : m_position.z;
  return m_position.z - (startElevation - distance);
}

BOOL CMovement::IsJumpingUp(DWORD eventTime) {
  if (m_jumpVelocity == 0.0f) {
    return 0;
  }
  float velocity = static_cast<int>(eventTime - m_fallStartTime) * 0.001f * s_gravityRate + m_jumpVelocity;
  if (velocity > s_terminalVelocity) {
    velocity = s_terminalVelocity;
  }
  FallLogWrite("0x%016I64X: Fall velocity at (0x%08X) is (%g)\n", GetGUID(), eventTime, static_cast<double>(velocity));
  return velocity < 0.0f;
}

BOOL CMovement::FallFromTransport() {
  if (!transportLink.IsLinked()) {
    return 0;
  }
  FATALASSERT(m_transportGUID);
  C3Vector moveVector = MovementGetTransportVector(m_transportGUID);
  return SetTransport(0) != 0;
}

void CMovement::StartFalling(DWORD eventTime) {
  FallFromTransport();
  m_moveFlags |= 0x4000;
  m_fallStartTime = eventTime;
  m_fallStartElevation = m_position.z;
  FallLogWrite("0x%016I64X: Started to fall at (0x%08X) from (%g elevation)\n", GetGUID(), eventTime, m_fallStartElevation);
}

void CMovement::StopFalling() {
  BothLogWrite("0x%016I64X: Landed (%g)\n", GetGUID(), static_cast<double>(m_position.z));
  m_moveFlags = (m_moveFlags & 0xF6FF2FFF) | 0x08000000;
  m_jumpVelocity = 0.0f;
  CalcDirection();
}

void CMovement::ProcessFallReset(DWORD eventTime) {
  BothLogWrite("0x%016I64X: Hit ceiling (%g)\n", GetGUID(), static_cast<double>(m_position.z));
  m_jumpVelocity = 0.0f;
  m_fallStartTime = eventTime;
  m_fallStartElevation = m_position.z;
}

void CMovement::CheckFallenFar(DWORD eventTime) {
  if (m_moveFlags & 0x8000) {
    return;
  }
  if (m_jumpVelocity != 0.0f) {
    if (m_fallStartElevation - 0.11111111f < m_position.z) {
      return;
    }
  } else if (eventTime - m_fallStartTime < 500) {
    return;
  }
  m_moveFlags |= 0x8000;
  FallLogWrite("0x%016I64X: Fallen far\n", GetGUID());
}

void CMovement::ProcessFalling(DWORD eventTime) {
  if (!(m_moveFlags & 0x4000)) {
    StartFalling(eventTime);
  }
  CheckFallenFar(eventTime);
}

void CMovement::ExtrudeDownNegXFacet(float distance, C4Plane *sides, C4Plane *startPlane) {
  C3Vector bottomPoint = m_position;
  bottomPoint.z -= distance;
  C3Vector topVector(0.87964189f, 0.0f, 0.4756366f);

  startPlane->Set(topVector, m_position);

  topVector.Set(-0.87964189f, 0.0f, -0.4756366f);
  C3Vector posYNorm(0.70710677f, 0.70710677f, 0.0f);
  C3Vector negYNorm(0.70710677f, -0.70710677f, 0.0f);
  sides[0].Set(topVector, bottomPoint);
  sides[1].Set(0.0f, 0.0f, 1.0f, -m_position.z - m_stepUpHeight);
  sides[2].Set(-1.0f, 0.0f, 0.0f, m_position.x - m_collisionBoxHalfDepth);
  sides[3].Set(posYNorm, m_position);
  sides[4].Set(negYNorm, m_position);
}

void CMovement::ExtrudeDownPosXFacet(float distance, C4Plane *sides, C4Plane *startPlane) {
  C3Vector bottomPoint = m_position;
  bottomPoint.z -= distance;
  C3Vector topVector(-0.87964189f, 0.0f, 0.4756366f);

  startPlane->Set(topVector, m_position);

  topVector.Set(0.87964189f, 0.0f, -0.4756366f);
  C3Vector posYNorm(-0.70710677f, 0.70710677f, 0.0f);
  C3Vector negYNorm(-0.70710677f, -0.70710677f, 0.0f);
  sides[0].Set(topVector, bottomPoint);
  sides[1].Set(0.0f, 0.0f, 1.0f, -m_position.z - m_stepUpHeight);
  sides[2].Set(1.0f, 0.0f, 0.0f, -m_position.x - m_collisionBoxHalfDepth);
  sides[3].Set(posYNorm, m_position);
  sides[4].Set(negYNorm, m_position);
}

void CMovement::ExtrudeDownNegYFacet(float distance, C4Plane *sides, C4Plane *startPlane) {
  C3Vector bottomPoint = m_position;
  bottomPoint.z -= distance;
  C3Vector topVector(0.0f, 0.87964189f, 0.4756366f);

  startPlane->Set(topVector, m_position);

  topVector.Set(0.0f, -0.87964189f, -0.4756366f);
  C3Vector posYNorm(0.70710677f, 0.70710677f, 0.0f);
  C3Vector negYNorm(-0.70710677f, 0.70710677f, 0.0f);
  sides[0].Set(topVector, bottomPoint);
  sides[1].Set(0.0f, 0.0f, 1.0f, -m_position.z - m_stepUpHeight);
  sides[2].Set(0.0f, -1.0f, 0.0f, m_position.y - m_collisionBoxHalfDepth);
  sides[3].Set(posYNorm, m_position);
  sides[4].Set(negYNorm, m_position);
}

void CMovement::ExtrudeDownPosYFacet(float distance, C4Plane *sides, C4Plane *startPlane) {
  C3Vector bottomPoint = m_position;
  bottomPoint.z -= distance;
  C3Vector topVector(0.0f, -0.87964189f, 0.4756366f);

  startPlane->Set(topVector, m_position);

  topVector.Set(0.0f, 0.87964189f, -0.4756366f);
  C3Vector posYNorm(0.70710677f, -0.70710677f, 0.0f);
  C3Vector negYNorm(-0.70710677f, -0.70710677f, 0.0f);
  sides[0].Set(topVector, bottomPoint);
  sides[1].Set(0.0f, 0.0f, 1.0f, -m_position.z - m_stepUpHeight);
  sides[2].Set(0.0f, 1.0f, 0.0f, -m_position.y - m_collisionBoxHalfDepth);
  sides[3].Set(posYNorm, m_position);
  sides[4].Set(negYNorm, m_position);
}

float CMovement::FindCeilingDistanceAbove(float distanceToJump) {
  float   top = -(m_collisionBoxHeight + m_position.z);
  C4Plane startPlane(0.0f, 0.0f, 1.0f, top);
  C4Plane boxSides[6];
  boxSides[0].Set(0.0f, 0.0f, -1.0f, m_stepUpHeight + m_position.z);
  boxSides[1].Set(0.0f, 0.0f, 1.0f, top - distanceToJump);
  boxSides[2].Set(0.0f, -1.0f, 0.0f, m_position.y - m_collisionBoxHalfDepth);
  boxSides[3].Set(0.0f, 1.0f, 0.0f, -m_position.y - m_collisionBoxHalfDepth);
  boxSides[4].Set(1.0f, 0.0f, 0.0f, -m_position.x - m_collisionBoxHalfDepth);
  boxSides[5].Set(-1.0f, 0.0f, 0.0f, m_position.x - m_collisionBoxHalfDepth);

  float    distanceJumped = distanceToJump;
  C3Vector unitMove(0.0f, 0.0f, 1.0f);
  FindObstacles(unitMove, boxSides, 6, startPlane, 2, &distanceJumped, 0);
  return distanceJumped > 0.0f ? distanceJumped : 0.0f;
}

void CMovement::SetOrientation() {
  C4Plane boxSides[6];
  boxSides[0].Set(1.0f, 0.0f, 0.0f, -m_position.x - m_collisionBoxHalfDepth);
  boxSides[1].Set(-1.0f, 0.0f, 0.0f, m_position.x - m_collisionBoxHalfDepth);
  boxSides[2].Set(0.0f, 1.0f, 0.0f, -m_position.y - m_collisionBoxHalfDepth);
  boxSides[3].Set(0.0f, -1.0f, 0.0f, m_position.y - m_collisionBoxHalfDepth);
  boxSides[4].Set(0.0f, 0.0f, 1.0f, -m_position.z - m_collisionBoxHeight);
  boxSides[5].Set(0.0f, 0.0f, -1.0f, m_position.z);
  m_groundNormal = CalcAverageSurfaceNormal(boxSides, 6);
}

static inline BOOL AddNormal(const C3Vector &normal, UINT maxNormals, C3Vector *normalList, UINT *numNormals) {
  if (*numNormals == maxNormals) {
    return 0;
  }
  for (UINT i = 0; i < *numNormals; ++i) {
    if (C3Vector::Dot(normal, normalList[i]) > 0.99984771f) {
      return 1;
    }
  }
  normalList[(*numNormals)++] = normal;
  return 1;
}

float CMovement::FindGroundDistanceBelow(float distanceToFall, DWORDLONG *gameObjHit) {
  float     distanceFallen = distanceToFall;
  CRedirect hitInfo;
  UINT      numHits = 0;
  C3Vector  surfNormals[4];
  C3Vector  unitMove(0.0f, 0.0f, -1.0f);
  C4Plane   boxPlanes[5];
  C4Plane   startPlane;
  int       foundWalkable = 0;

  FallLogWrite("0x%016I64X: Checking neg X side:\n", GetGUID());
  ExtrudeDownNegXFacet(distanceToFall, boxPlanes, &startPlane);
  FindObstacles(unitMove, boxPlanes, 5, startPlane, 1, &distanceFallen, &hitInfo);
  if (hitInfo.flags) {
    surfNormals[0] = hitInfo.surfaceNorm[0];
    numHits = 1;
    if (hitInfo.flags & 0x80) {
      AddNormal(hitInfo.surfaceNorm[1], 4, surfNormals, &numHits);
    }
    foundWalkable = !(hitInfo.surfaceNorm[0].z <= 0.64278764f);
    *gameObjHit = hitInfo.gameObjHit;
  }
  FallLogWrite("0x%016I64X: Hit info flags neg X: ", GetGUID());
  LogHitInfoFlags(hitInfo.flags);
  FallLogWrite("\n");

  FallLogWrite("0x%016I64X: Checking pos X side:\n", GetGUID());
  ExtrudeDownPosXFacet(distanceToFall, boxPlanes, &startPlane);
  float shortestDist = distanceFallen;
  hitInfo.Reset();
  distanceFallen = distanceToFall;
  FindObstacles(unitMove, boxPlanes, 5, startPlane, 1, &distanceFallen, &hitInfo);
  if (hitInfo.flags) {
    if (CMath::fequalz_(shortestDist, distanceFallen, 0.013888889f)) {
      if (!AddNormal(hitInfo.surfaceNorm[0], 4, surfNormals, &numHits)) {
        FallLogWrite("0x%016I64X: failed to add normal to slide set, too many normals\n", GetGUID());
      }
      if ((hitInfo.flags & 0x80) && !AddNormal(hitInfo.surfaceNorm[1], 4, surfNormals, &numHits)) {
        FallLogWrite("0x%016I64X: failed to add normal to slide set, too many normals\n", GetGUID());
      }
      if (shortestDist > distanceFallen) {
        shortestDist = distanceFallen;
      }
      foundWalkable |= !(hitInfo.surfaceNorm[0].z <= 0.64278764f);
    } else if (shortestDist > distanceFallen) {
      surfNormals[0] = hitInfo.surfaceNorm[0];
      numHits = 1;
      if (hitInfo.flags & 0x80) {
        AddNormal(hitInfo.surfaceNorm[1], 4, surfNormals, &numHits);
      }
      shortestDist = distanceFallen;
      foundWalkable = !(hitInfo.surfaceNorm[0].z <= 0.64278764f);
    }
    *gameObjHit = hitInfo.gameObjHit;
  }
  FallLogWrite("0x%016I64X: Hit info flags pos X: ", GetGUID());
  LogHitInfoFlags(hitInfo.flags);
  FallLogWrite("\n");

  FallLogWrite("0x%016I64X: Checking neg Y side:\n", GetGUID());
  ExtrudeDownNegYFacet(distanceToFall, boxPlanes, &startPlane);
  hitInfo.Reset();
  distanceFallen = distanceToFall;
  FindObstacles(unitMove, boxPlanes, 5, startPlane, 1, &distanceFallen, &hitInfo);
  if (hitInfo.flags) {
    if (CMath::fequalz_(shortestDist, distanceFallen, 0.013888889f)) {
      if (!AddNormal(hitInfo.surfaceNorm[0], 4, surfNormals, &numHits)) {
        FallLogWrite("0x%016I64X: failed to add normal to slide set, too many normals\n", GetGUID());
      }
      if ((hitInfo.flags & 0x80) && !AddNormal(hitInfo.surfaceNorm[1], 4, surfNormals, &numHits)) {
        FallLogWrite("0x%016I64X: failed to add normal to slide set, too many normals\n", GetGUID());
      }
      if (shortestDist > distanceFallen) {
        shortestDist = distanceFallen;
      }
      foundWalkable |= !(hitInfo.surfaceNorm[0].z <= 0.64278764f);
    } else if (shortestDist > distanceFallen) {
      surfNormals[0] = hitInfo.surfaceNorm[0];
      numHits = 1;
      if (hitInfo.flags & 0x80) {
        AddNormal(hitInfo.surfaceNorm[1], 4, surfNormals, &numHits);
      }
      shortestDist = distanceFallen;
      foundWalkable = !(hitInfo.surfaceNorm[0].z <= 0.64278764f);
    }
    *gameObjHit = hitInfo.gameObjHit;
  }
  FallLogWrite("0x%016I64X: Hit info flags neg Y: ", GetGUID());
  LogHitInfoFlags(hitInfo.flags);
  FallLogWrite("\n");

  FallLogWrite("0x%016I64X: Checking pos Y side:\n", GetGUID());
  ExtrudeDownPosYFacet(distanceToFall, boxPlanes, &startPlane);
  hitInfo.Reset();
  distanceFallen = distanceToFall;
  FindObstacles(unitMove, boxPlanes, 5, startPlane, 1, &distanceFallen, &hitInfo);
  if (hitInfo.flags) {
    if (CMath::fequalz_(shortestDist, distanceFallen, 0.013888889f)) {
      if (!AddNormal(hitInfo.surfaceNorm[0], 4, surfNormals, &numHits)) {
        FallLogWrite("0x%016I64X: failed to add normal to slide set, too many normals\n", GetGUID());
      }
      if ((hitInfo.flags & 0x80) && !AddNormal(hitInfo.surfaceNorm[1], 4, surfNormals, &numHits)) {
        FallLogWrite("0x%016I64X: failed to add normal to slide set, too many normals\n", GetGUID());
      }
      if (shortestDist > distanceFallen) {
        shortestDist = distanceFallen;
      }
      foundWalkable |= !(hitInfo.surfaceNorm[0].z <= 0.64278764f);
    } else if (shortestDist > distanceFallen) {
      surfNormals[0] = hitInfo.surfaceNorm[0];
      numHits = 1;
      if (hitInfo.flags & 0x80) {
        AddNormal(hitInfo.surfaceNorm[1], 4, surfNormals, &numHits);
      }
      shortestDist = distanceFallen;
      foundWalkable = !(hitInfo.surfaceNorm[0].z <= 0.64278764f);
    }
    *gameObjHit = hitInfo.gameObjHit;
  }
  FallLogWrite("0x%016I64X: Hit info flags pos Y: ", GetGUID());
  LogHitInfoFlags(hitInfo.flags);
  FallLogWrite("\n");

  if (!foundWalkable && !CMath::fequalz_(distanceToFall, shortestDist, 0.0013888889f)) {
    if (!(numHits > 0)) {
      this ? SErrDisplayErrorFmt(
                 STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
                 "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
                 "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
                 "numHits > 0", GetGUID(), GetPosition().x, GetPosition().y, GetPosition().z, GetFacing(),
                 static_cast<int>(GetPosition().x), static_cast<int>(GetPosition().y), static_cast<int>(GetPosition().z),
                 static_cast<int>(GetFacing())
             )
           : SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "numHits > 0", FALSE, 1);
    }
    if (GetSlidingDirection(GetGUID(), surfNormals, numHits, &m_reDirection)) {
      m_moveFlags |= 0x01000000;
    } else {
      m_moveFlags &= ~0x01000000;
    }
  }
  return shortestDist > 0.0f ? shortestDist : 0.0f;
}

static int GetSlidingDirection(DWORDLONG guid, const C3Vector *normalList, UINT numNormals, C3Vector *direction) {
  C3Vector intermed1;
  C3Vector intermed2;

  CMovement::FallLogWrite("0x%016I64X: Getting slide direction from %u normals\n", guid, numNormals);
  switch (numNormals) {
    case 2:
      CMovement::FallLogWrite(
          "0x%016I64X: Normals: (%g,%g,%g), (%g,%g,%g)\n", guid, normalList[0].x, normalList[0].y, normalList[0].z, normalList[1].x, normalList[1].y,
          normalList[1].z
      );
      *direction = C3Vector::Cross(normalList[0], normalList[1]);
      direction->SafeNormalize();
      break;
    case 3:
      CMovement::FallLogWrite(
          "0x%016I64X: Normals: (%g,%g,%g), (%g,%g,%g), (%g,%g,%g)\n", guid, normalList[0].x, normalList[0].y, normalList[0].z, normalList[1].x,
          normalList[1].y, normalList[1].z, normalList[2].x, normalList[2].y, normalList[2].z
      );
      intermed1 = C3Vector::Cross(normalList[0], normalList[1]);
      intermed1.SafeNormalize();
      intermed2 = C3Vector::Cross(normalList[0], normalList[2]);
      intermed2.SafeNormalize();
      *direction = C3Vector::Cross(intermed1, intermed2);
      direction->SafeNormalize();
      break;
    case 4:
      CMovement::FallLogWrite(
          "0x%016I64X: Normals: (%g,%g,%g), (%g,%g,%g), (%g,%g,%g), (%g,%g,%g)\n", guid, normalList[0].x, normalList[0].y, normalList[0].z,
          normalList[1].x, normalList[1].y, normalList[1].z, normalList[2].x, normalList[2].y, normalList[2].z, normalList[3].x, normalList[3].y,
          normalList[3].z
      );
      intermed1 = C3Vector::Cross(normalList[0], normalList[1]);
      intermed1.SafeNormalize();
      intermed2 = C3Vector::Cross(normalList[2], normalList[3]);
      intermed2.SafeNormalize();
      *direction = C3Vector::Cross(intermed1, intermed2);
      direction->SafeNormalize();
      break;
    case 1: {
      CMovement::FallLogWrite("0x%016I64X: Normal: (%g,%g,%g)\n", guid, normalList[0].x, normalList[0].y, normalList[0].z);
      C2Vector downCrossSurfaceNorm(-normalList[0].y, normalList[0].x);
      downCrossSurfaceNorm.SafeNormalize();
      *direction = C3Vector::Cross(downCrossSurfaceNorm, normalList[0]);
      break;
    }
    default:
      CMovement::FallLogWrite("0x%016I64X: TOO MANY NORMALS, failed to get slide direction!!!\n", guid);
      break;
  }

  if (CMath::fabs_(direction->z) <= 0.76604444f) {
    C3Vector minIncline(0.0f, 1.0f, 0.0f);
    for (UINT i = 0; i < numNormals; ++i) {
      if (normalList[i].z > minIncline.z) {
        minIncline = normalList[i];
      }
    }
    C2Vector downCrossSurfaceNorm(-minIncline.y, minIncline.x);
    downCrossSurfaceNorm.SafeNormalize();
    *direction = C3Vector::Cross(downCrossSurfaceNorm, minIncline);
    CMovement::FallLogWrite("0x%016I64X: Slope too flat, readjusting slide dir\n", guid);
  }

  if (direction->z > 0.0f) {
    *direction = -*direction;
  }
  CMovement::FallLogWrite("0x%016I64X: New slide dir (%g,%g,%g)\n", guid, direction->x, direction->y, direction->z);
  return CMath::fabs_(direction->z) > 0.76604444f;
}

float CMovement::CollideWithWaterSurface(const C3Vector &unitMove, const C3Vector &unitMoveWanted, float distanceWanted) {
  C3Vector moveVector = unitMove * distanceWanted;
  if (CMath::fequal4_(moveVector.z, 0.0f) || m_position.z + moveVector.z < m_waterSurfaceElev) {
    m_moveFlags &= ~0x1000;
    return distanceWanted;
  }
  m_reDirection = C2Vector(unitMoveWanted);
  m_reDirection.SafeNormalize();
  m_moveFlags |= 0x1000;
  return (m_waterSurfaceElev - m_position.z) / moveVector.z * distanceWanted;
}

float CMovement::ExtrudeFlyBoxUp(const C3Vector &unitMove, const C3Vector &unitMoveWanted, float distanceWanted) {
  C3Vector moveVector = unitMove * distanceWanted;
  C4Plane  boxPlanes[5][6];
  float    pyramidHgt = m_collisionBoxHalfDepth * 1.849399f;
  ExtrudeBoxSideX(moveVector, pyramidHgt, boxPlanes[0]);
  ExtrudeBoxSideY(moveVector, pyramidHgt, boxPlanes[1]);
  ExtrudeBoxSideZ(moveVector, pyramidHgt, boxPlanes[2]);
  int pyrSideX = ExtrudePyramidSideX(unitMove, distanceWanted, boxPlanes[3]);
  int pyrSideY = ExtrudePyramidSideY(unitMove, distanceWanted, boxPlanes[4]);

  CRedirect hitInfoX;
  CRedirect hitInfoY;
  CRedirect hitInfoZ;
  float     distanceX = FLT_MAX;
  float     distanceY = FLT_MAX;
  float     distanceZ = FLT_MAX;
  C4Plane   startPlanes[3];
  startPlanes[0] = boxPlanes[0][0];
  startPlanes[0].d += 0.027777778f;
  startPlanes[1] = boxPlanes[1][0];
  startPlanes[1].d += 0.027777778f;
  startPlanes[2] = boxPlanes[2][0];
  startPlanes[2].d += 0.027777778f;
  FindObstacles(unitMove, boxPlanes[0], 6, startPlanes[0], 0, &distanceX, &hitInfoX);
  FindObstacles(unitMove, boxPlanes[1], 6, startPlanes[1], 0, &distanceY, &hitInfoY);
  FindObstacles(unitMove, boxPlanes[2], 6, startPlanes[2], 0, &distanceZ, &hitInfoZ);
  if (pyrSideX) {
    FindObstacles(unitMove, boxPlanes[3], 5, boxPlanes[3][0], 1, &distanceX, &hitInfoX);
  }
  if (pyrSideY) {
    FindObstacles(unitMove, boxPlanes[4], 5, boxPlanes[4][0], 1, &distanceY, &hitInfoY);
  }

  float distance;
  if (distanceX < distanceY && distanceX < distanceZ) {
    distance = distanceX;
    if (!CMath::fequalz_(distanceX, distanceY, 0.0013888889f)) {
      hitInfoY.flags = 0;
    }
    if (!CMath::fequalz_(distanceX, distanceZ, 0.0013888889f)) {
      hitInfoZ.flags = 0;
    }
  } else if (distanceY < distanceX && distanceY < distanceZ) {
    distance = distanceY;
    if (!CMath::fequalz_(distanceY, distanceX, 0.0013888889f)) {
      hitInfoX.flags = 0;
    }
    if (!CMath::fequalz_(distanceY, distanceZ, 0.0013888889f)) {
      hitInfoZ.flags = 0;
    }
  } else {
    distance = distanceZ;
    if (!CMath::fequalz_(distanceZ, distanceX, 0.0013888889f)) {
      hitInfoX.flags = 0;
    }
    if (!CMath::fequalz_(distanceZ, distanceY, 0.0013888889f)) {
      hitInfoY.flags = 0;
    }
  }
  if (((m_moveFlags & 0x1000) && CMath::fnotequal_(m_reDirection.z, 0.0f)) || hitInfoX.flags || hitInfoY.flags || hitInfoZ.flags) {
    FlyRedirect(unitMoveWanted, hitInfoX, hitInfoY, hitInfoZ);
  } else {
    distance = CollideWithWaterSurface(unitMove, unitMoveWanted, distanceWanted);
  }
  return distance > 0.0f ? distance : 0.0f;
}

float CMovement::ExtrudeProjectileBoxUpHill(const C3Vector &unitMove, float distanceWanted, DWORDLONG *gameObjHit) {
  CRedirect hitInfo;
  C3Vector  moveVector = unitMove * distanceWanted;
  C4Plane   boxPlanes[5][6];
  float     pyramidHgt = m_collisionBoxHalfDepth * 1.849399f;
  ExtrudeBoxSideX(moveVector, pyramidHgt, boxPlanes[0]);
  ExtrudeBoxSideY(moveVector, pyramidHgt, boxPlanes[1]);
  ExtrudeBoxSideZ(moveVector, pyramidHgt, boxPlanes[2]);
  int     pyrSideX = ExtrudePyramidSideX(unitMove, distanceWanted, boxPlanes[3]);
  int     pyrSideY = ExtrudePyramidSideY(unitMove, distanceWanted, boxPlanes[4]);
  float   distance = FLT_MAX;
  C4Plane startPlanes[3];
  startPlanes[0] = boxPlanes[0][0];
  startPlanes[0].d += 0.027777778f;
  startPlanes[1] = boxPlanes[1][0];
  startPlanes[1].d += 0.027777778f;
  startPlanes[2] = boxPlanes[2][0];
  startPlanes[2].d += 0.027777778f;
  FallLogWrite("0x%016I64X: Checking X side:\n", GetGUID());
  FindObstacles(unitMove, boxPlanes[0], 6, startPlanes[0], 0, &distance, &hitInfo);
  FallLogWrite("0x%016I64X: Checking Y side:\n", GetGUID());
  FindObstacles(unitMove, boxPlanes[1], 6, startPlanes[1], 0, &distance, &hitInfo);
  FallLogWrite("0x%016I64X: Checking Z side:\n", GetGUID());
  FindObstacles(unitMove, boxPlanes[2], 6, startPlanes[2], 0, &distance, &hitInfo);
  if (pyrSideX) {
    FallLogWrite("0x%016I64X: Checking Pyramid X side:\n", GetGUID());
    FindObstacles(unitMove, boxPlanes[3], 5, boxPlanes[3][0], 1, &distance, &hitInfo);
    if (hitInfo.flags & 0x40) {
      *gameObjHit = hitInfo.gameObjHit;
    }
  }
  if (pyrSideY) {
    FallLogWrite("0x%016I64X: Checking Pyramid Y side:\n", GetGUID());
    FindObstacles(unitMove, boxPlanes[4], 5, boxPlanes[4][0], 1, &distance, &hitInfo);
    if (hitInfo.flags & 0x40) {
      *gameObjHit = hitInfo.gameObjHit;
    }
  }
  if (distance < distanceWanted && hitInfo.flags && !(hitInfo.surfaceNorm[0].z <= 0.64278764f)) {
    StopFalling();
  }
  return distance > 0.0f ? distance : 0.0f;
}

float CMovement::ExtrudeSlideBoxDownHill(const C3Vector &unitMove, float distanceWanted, CRedirect *hitInfo) {
  C3Vector moveVector = unitMove * distanceWanted;
  C4Plane  boxPlanes[4][6];
  float    pyramidHgt = m_collisionBoxHalfDepth * 1.849399f;
  ExtrudeBoxSideX(moveVector, pyramidHgt, boxPlanes[0]);
  ExtrudeBoxSideY(moveVector, pyramidHgt, boxPlanes[1]);
  int     pyrSideX = ExtrudePyramidSideX(unitMove, distanceWanted, boxPlanes[2]);
  int     pyrSideY = ExtrudePyramidSideY(unitMove, distanceWanted, boxPlanes[3]);
  float   distance = FLT_MAX;
  C4Plane startPlanes[2];
  startPlanes[0] = boxPlanes[0][0];
  startPlanes[0].d += 0.027777778f;
  startPlanes[1] = boxPlanes[1][0];
  startPlanes[1].d += 0.027777778f;

  FallLogWrite("0x%016I64X: Checking X side:\n", GetGUID());
  FindObstacles(unitMove, boxPlanes[0], 6, startPlanes[0], 0, &distance, hitInfo);
  FallLogWrite("0x%016I64X: Hit info flags X: ", GetGUID());
  LogHitInfoFlags(hitInfo->flags);
  FallLogWrite("\n");

  FallLogWrite("0x%016I64X: Checking Y side:\n", GetGUID());
  FindObstacles(unitMove, boxPlanes[1], 6, startPlanes[1], 0, &distance, hitInfo);
  FallLogWrite("0x%016I64X: Hit info flags Y: ", GetGUID());
  LogHitInfoFlags(hitInfo->flags);
  FallLogWrite("\n");

  if (pyrSideX) {
    FallLogWrite("0x%016I64X: Checking Pyramid X side:\n", GetGUID());
    FindObstacles(unitMove, boxPlanes[2], 5, boxPlanes[2][0], 1, &distance, hitInfo);
    FallLogWrite("0x%016I64X: Hit info flags Pyramid X: ", GetGUID());
    LogHitInfoFlags(hitInfo->flags);
    FallLogWrite("\n");
  }
  if (pyrSideY) {
    FallLogWrite("0x%016I64X: Checking Pyramid Y side:\n", GetGUID());
    FindObstacles(unitMove, boxPlanes[3], 5, boxPlanes[3][0], 1, &distance, hitInfo);
    FallLogWrite("0x%016I64X: Hit info flags Pyramid Y: ", GetGUID());
    LogHitInfoFlags(hitInfo->flags);
    FallLogWrite("\n");
  }
  return distance > 0.0f ? distance : 0.0f;
}

void CMovement::FlyRedirect(
    const C3Vector &unitMoveWanted,
    const CRedirect          &hitInfoX,
    const CRedirect          &hitInfoY,
    const CRedirect          &hitInfoZ
) {
  if (!hitInfoX.flags && !hitInfoY.flags && !hitInfoZ.flags) {
    return;
  }

  C3Vector normals[6];
  UINT               numNormals = 0;
  if (hitInfoX.flags) {
    if (hitInfoX.flags & 3) {
      C3Vector normal(unitMoveWanted.x >= 0.0f ? -1.0f : 1.0f, 0.0f, 0.0f);
      AddNormal(normal, 6, normals, &numNormals);
    } else {
      AddNormal(hitInfoX.surfaceNorm[0], 6, normals, &numNormals);
      if (hitInfoX.flags & 0x80) {
        AddNormal(hitInfoX.surfaceNorm[1], 6, normals, &numNormals);
      }
    }
  }
  if (hitInfoY.flags) {
    if (hitInfoY.flags & 3) {
      C3Vector normal(0.0f, unitMoveWanted.y >= 0.0f ? -1.0f : 1.0f, 0.0f);
      AddNormal(normal, 6, normals, &numNormals);
    } else {
      AddNormal(hitInfoY.surfaceNorm[0], 6, normals, &numNormals);
      if (hitInfoY.flags & 0x80) {
        AddNormal(hitInfoY.surfaceNorm[1], 6, normals, &numNormals);
      }
    }
  }
  if (hitInfoZ.flags) {
    if (hitInfoZ.flags & 3) {
      C3Vector normal(0.0f, 0.0f, -1.0f);
      AddNormal(normal, 6, normals, &numNormals);
    } else {
      AddNormal(hitInfoZ.surfaceNorm[0], 6, normals, &numNormals);
      if (hitInfoZ.flags & 0x80) {
        AddNormal(hitInfoZ.surfaceNorm[1], 6, normals, &numNormals);
      }
    }
  }

  FATALASSERT(numNormals > 0);
  FATALASSERT(numNormals <= 6);
  m_reDirection = ComputeFlyRedirection(normals, numNormals, unitMoveWanted);
  if (CMath::fnotequal4_(m_reDirection.SquaredMag(), 0.0f)) {
    m_moveFlags |= 0x1000;
  }
}

static C3Vector ComputeFlyRedirection(const C3Vector *normals, UINT numNormals, const C3Vector &unitMoveWanted) {
  C3Vector finalDirection(0.0f);
  float    mostObtuse = 1.0f;
  for (UINT i = 0; i < numNormals; ++i) {
    C3Vector orthogonal = C3Vector::Cross(unitMoveWanted, normals[i]);
    if (!CMath::fequal4_(orthogonal.x, 0.0f) || !CMath::fequal4_(orthogonal.y, 0.0f) || !CMath::fequal4_(orthogonal.z, 0.0f)) {
      orthogonal.Normalize();
      C3Vector newDirection = C3Vector::Cross(normals[i], orthogonal);
      if (C3Vector::Dot(newDirection, unitMoveWanted) < 0.0f) {
        newDirection = -newDirection;
      }
      float cosTheta = C3Vector::Dot(newDirection, unitMoveWanted);
      if (cosTheta < mostObtuse) {
        mostObtuse = cosTheta;
        finalDirection = newDirection;
      }
    }
  }
  return finalDirection;
}

void CMovement::FlyRedirect(const C3Vector &unitMoveWanted, const CRedirect &hitInfoX, const CRedirect &hitInfoY) {
  if (!hitInfoX.flags && !hitInfoY.flags) {
    return;
  }

  C3Vector normals[4];
  UINT               numNormals = 0;
  if (hitInfoX.flags) {
    if (hitInfoX.flags & 3) {
      C3Vector normal(unitMoveWanted.x >= 0.0f ? -1.0f : 1.0f, 0.0f, 0.0f);
      AddNormal(normal, 4, normals, &numNormals);
    } else {
      AddNormal(hitInfoX.surfaceNorm[0], 4, normals, &numNormals);
      if (hitInfoX.flags & 0x80) {
        AddNormal(hitInfoX.surfaceNorm[1], 4, normals, &numNormals);
      }
    }
  }
  if (hitInfoY.flags) {
    if (hitInfoY.flags & 3) {
      C3Vector normal(0.0f, unitMoveWanted.y >= 0.0f ? -1.0f : 1.0f, 0.0f);
      AddNormal(normal, 4, normals, &numNormals);
    } else {
      AddNormal(hitInfoY.surfaceNorm[0], 4, normals, &numNormals);
      if (hitInfoY.flags & 0x80) {
        AddNormal(hitInfoY.surfaceNorm[1], 4, normals, &numNormals);
      }
    }
  }

  FATALASSERT(numNormals > 0);
  FATALASSERT(numNormals <= 4);
  m_reDirection = ComputeFlyRedirection(normals, numNormals, unitMoveWanted);
  if (CMath::fnotequal4_(m_reDirection.SquaredMag(), 0.0f)) {
    m_moveFlags |= 0x1000;
  }
}

float CMovement::ExtrudeFlyBoxDown(const C3Vector &unitMove, const C3Vector &unitMoveWanted, float distanceWanted) {
  CRedirect hitInfoX;
  CRedirect hitInfoY;
  C4Plane   boxPlanes[4][6];
  float     pyramidHgt = m_collisionBoxHalfDepth * 1.849399f;
  C3Vector  moveVector = unitMove * distanceWanted;
  ExtrudeBoxSideX(moveVector, pyramidHgt, boxPlanes[0]);
  ExtrudeBoxSideY(moveVector, pyramidHgt, boxPlanes[1]);
  int     pyrSideX = ExtrudePyramidSideX(unitMove, distanceWanted, boxPlanes[2]);
  int     pyrSideY = ExtrudePyramidSideY(unitMove, distanceWanted, boxPlanes[3]);
  float   distanceX = FLT_MAX;
  float   distanceY = FLT_MAX;
  C4Plane startPlanes[2];
  startPlanes[0] = boxPlanes[0][0];
  startPlanes[0].d += 0.027777778f;
  startPlanes[1] = boxPlanes[1][0];
  startPlanes[1].d += 0.027777778f;
  FindObstacles(unitMove, boxPlanes[0], 6, startPlanes[0], 0, &distanceX, &hitInfoX);
  FindObstacles(unitMove, boxPlanes[1], 6, startPlanes[1], 0, &distanceY, &hitInfoY);
  if (pyrSideX) {
    FindObstacles(unitMove, boxPlanes[2], 5, boxPlanes[2][0], 1, &distanceX, &hitInfoX);
  }
  if (pyrSideY) {
    FindObstacles(unitMove, boxPlanes[3], 5, boxPlanes[3][0], 1, &distanceY, &hitInfoY);
  }

  float distance;
  if (distanceX < distanceY) {
    distance = distanceX;
    if (!CMath::fequalz_(distanceX, distanceY, 0.0013888889f)) {
      hitInfoY.flags = 0;
    }
  } else {
    distance = distanceY;
    if (!CMath::fequalz_(distanceY, distanceX, 0.0013888889f)) {
      hitInfoX.flags = 0;
    }
  }
  FlyRedirect(unitMoveWanted, hitInfoX, hitInfoY);
  return distance > 0.0f ? distance : 0.0f;
}

float CMovement::ExtrudeProjectileBoxDownHill(
    DWORD                     timeStamp,
    const C3Vector &unitMove,
    float                     distanceWanted,
    const C2Vector &unitMoveWanted,
    DWORDLONG                *gameObjHit
) {
  C4Plane  boxPlanes[4][6];
  CRedirect          hitInfoY;
  CRedirect          hitInfoX;
  C4Plane  startPlanes[2];
  C3Vector moveVector = unitMove * distanceWanted;
  float              pyramidHgt = m_collisionBoxHalfDepth * 1.849399f;
  ExtrudeBoxSideX(moveVector, pyramidHgt, boxPlanes[0]);
  ExtrudeBoxSideY(moveVector, pyramidHgt, boxPlanes[1]);
  int   pyrSideX = ExtrudePyramidSideX(unitMove, distanceWanted, boxPlanes[2]);
  int   pyrSideY = ExtrudePyramidSideY(unitMove, distanceWanted, boxPlanes[3]);
  float distanceX = FLT_MAX;
  float distanceY = FLT_MAX;
  startPlanes[0] = boxPlanes[0][0];
  startPlanes[0].d += 0.027777778f;
  startPlanes[1] = boxPlanes[1][0];
  startPlanes[1].d += 0.027777778f;
  FallLogWrite("0x%016I64X: Checking X side:\n", GetGUID());
  FindObstacles(unitMove, boxPlanes[0], 6, startPlanes[0], 0, &distanceX, &hitInfoX);
  FallLogWrite("0x%016I64X: Checking Y side:\n", GetGUID());
  FindObstacles(unitMove, boxPlanes[1], 6, startPlanes[1], 0, &distanceY, &hitInfoY);
  hitInfoX.gameObjHit = 0;
  hitInfoY.gameObjHit = 0;
  if (pyrSideX) {
    FallLogWrite("0x%016I64X: Checking Pyramid X side:\n", GetGUID());
    FindObstacles(unitMove, boxPlanes[2], 5, boxPlanes[2][0], 1, &distanceX, &hitInfoX);
    if (hitInfoX.gameObjHit) {
      *gameObjHit = hitInfoX.gameObjHit;
    }
  }
  if (pyrSideY) {
    FallLogWrite("0x%016I64X: Checking Pyramid Y side:\n", GetGUID());
    FindObstacles(unitMove, boxPlanes[3], 5, boxPlanes[3][0], 1, &distanceY, &hitInfoY);
    if (hitInfoY.gameObjHit) {
      *gameObjHit = hitInfoY.gameObjHit;
    }
  }

  float distance;
  if (distanceX < distanceY) {
    distance = distanceX;
    if (CMath::fabs_(distanceX - distanceY) >= 0.0013888889f) {
      hitInfoY.Reset();
    }
  } else {
    distance = distanceY;
    if (CMath::fabs_(distanceX - distanceY) >= 0.0013888889f) {
      hitInfoX.Reset();
    }
  }
  if (distance >= distanceWanted) {
    return distance;
  }
  if ((hitInfoY.flags &&
       ((hitInfoY.flags & 0x40) || hitInfoY.surfaceNorm[0].z > 0.64278764f || (hitInfoY.flags & 0x80) && hitInfoY.surfaceNorm[1].z > 0.64278764f)) ||
      (hitInfoX.flags &&
       ((hitInfoX.flags & 0x40) || hitInfoX.surfaceNorm[0].z > 0.64278764f || (hitInfoX.flags & 0x80) && hitInfoX.surfaceNorm[1].z > 0.64278764f)))
  {
    StopFalling();
    return distance;
  }
  if (!hitInfoX.flags && !hitInfoY.flags) {
    return distance;
  }

  C3Vector platformNorm(-unitMove.x * unitMove.z, -unitMove.y * unitMove.z, unitMove.x * unitMove.x + unitMove.y * unitMove.y);
  if (platformNorm.z < 0.0f) {
    platformNorm = -platformNorm;
  }
  C3Vector unitMoveWanted3d(unitMoveWanted.x, unitMoveWanted.y, 0.0f);
  Redirect(timeStamp, unitMoveWanted3d, platformNorm, hitInfoX, hitInfoY);
  return distance;
}

UINT CMovement::Slide(UINT fallenSoFar, UINT timeIncrement) {
  if (!timeIncrement || (m_transportGUID && FallFromTransport())) {
    return 0;
  }

  float distToFall = RelDistanceFallen(fallenSoFar + timeIncrement);
  FallLogWrite(
      "0x%016I64X: ====| Starting new slide: fallenSoFar (%u) increment (%u) distance to fall (%g)\n", GetGUID(), fallenSoFar, timeIncrement, distToFall
  );
  if (CMath::fabs_(distToFall) < 0.00000023841858f) {
    return 0;
  }

  float              scale = 1.0f / -m_reDirection.z;
  C3Vector unitMove(m_reDirection.x * scale, m_reDirection.y * scale, m_reDirection.z * scale);
  C3Vector moveWanted = unitMove * distToFall;
  float              distance = moveWanted.Mag();
  FATALASSERT(CMath::fnotequal_(distance, 0.0f));

  float distanceSlid = distance;
  if (s_facetData.facets.Count()) {
    CRedirect hitInfo;
    distanceSlid = ExtrudeSlideBoxDownHill(unitMove, distance, &hitInfo);
    if (distanceSlid < distance) {
      m_moveFlags &= ~0x01000000U;
      m_position += unitMove * distanceSlid;
      float timeUsed = distanceSlid / distance * (static_cast<float>(timeIncrement) * 0.001f) * 1000.0f;
      if (timeUsed <= 0.0f) {
        timeIncrement = static_cast<UINT>(-static_cast<long>(-timeUsed + 0.5f));
      } else {
        timeIncrement = static_cast<UINT>(timeUsed + 0.5f);
      }
      if (hitInfo.surfaceNorm[0].z > 0.64278764f) {
        StopFalling();
        HandlePendingActions(fallenSoFar + timeIncrement + m_fallStartTime);
      }
    } else {
      m_position += moveWanted;
    }
  } else {
    m_position += moveWanted;
    m_moveFlags &= ~0x01000000U;
  }

  FallLogWrite(
      "0x%016I64X: ====| Wanted to slide (%g) (%g,%g,%g), slid (%g) to (%g,%g,%g)\n", GetGUID(), distance, moveWanted.x, moveWanted.y, moveWanted.z,
      distanceSlid, m_position.x, m_position.y, m_position.z
  );
  SetOrientation();
  return timeIncrement;
}

UINT CMovement::Fall(UINT fallenSoFar, UINT timeIncrement) {
  if (!timeIncrement || (m_transportGUID && FallFromTransport())) {
    return 0;
  }

  UINT fallEndTime = fallenSoFar + timeIncrement;
  FallLogWrite("0x%016I64X: ====| Starting new fall: fallenSoFar (%u) increment (%u)\n", GetGUID(), fallenSoFar, timeIncrement);
  float distToFall = RelDistanceFallen(fallEndTime);
  float distFallen = distToFall;

  if (s_facetData.facets.Count()) {
    if (distToFall >= 0.0f) {
      DWORDLONG gameObjHit = 0;
      distFallen = FindGroundDistanceBelow(distToFall, &gameObjHit);
      m_position.z -= distFallen;
      if (CMath::fabs_(distToFall - distFallen) < 0.00000095367432f) {
        if (!(m_moveFlags & 0x01000000)) {
          CheckFallenFar(fallEndTime + m_fallStartTime);
        }
      } else {
        float timeUsed = distFallen / distToFall * (static_cast<float>(timeIncrement) * 0.001f) * 1000.0f;
        if (timeUsed <= 0.0f) {
          timeIncrement = static_cast<UINT>(-static_cast<long>(-timeUsed + 0.5f));
        } else {
          timeIncrement = static_cast<UINT>(timeUsed + 0.5f);
        }
        if (!(m_moveFlags & 0x01000000)) {
          StopFalling();
          HandlePendingActions(fallenSoFar + timeIncrement + m_fallStartTime);
        }
        if (gameObjHit) {
          SetTransport(gameObjHit);
        }
      }
    } else {
      distFallen = -FindCeilingDistanceAbove(-distToFall);
      m_position.z -= distFallen;
      if (CMath::fabs_(distToFall - distFallen) < 0.00000095367432f) {
        CheckFallenFar(fallEndTime + m_fallStartTime);
      } else {
        float timeUsed = distFallen / distToFall * (static_cast<float>(timeIncrement) * 0.001f) * 1000.0f;
        if (timeUsed <= 0.0f) {
          timeIncrement = static_cast<UINT>(-static_cast<long>(-timeUsed + 0.5f));
        } else {
          timeIncrement = static_cast<UINT>(timeUsed + 0.5f);
        }
        ProcessFallReset(fallenSoFar + timeIncrement + m_fallStartTime);
      }
    }
  } else {
    m_position.z -= distToFall;
    CheckFallenFar(fallEndTime + m_fallStartTime);
  }

  FallLogWrite("0x%016I64X: ====| Wanted to fall (%g), fell (%g) to Z(%g)\n", GetGUID(), distToFall, distFallen, m_position.z);
  m_groundNormal.Set(0.0f, 0.0f, 1.0f);
  return timeIncrement;
}

void CMovement::GetMoveFacets(float distance, UINT timeToFall, const C3Vector &unitMove) {
  C3Vector moveVector = unitMove;
  if (!(m_moveFlags & 0x02000000)) {
    moveVector.z = 0.0f;
    if (CMath::fabs_(moveVector.x) >= 0.00000023841858f && CMath::fabs_(moveVector.y) >= 0.00000023841858f) {
      float ooMag = 1.0f / static_cast<float>(sqrt(moveVector.x * moveVector.x + moveVector.y * moveVector.y));
      moveVector.x *= ooMag;
      moveVector.y *= ooMag;
    }
  }
  moveVector *= distance;

  C3Vector  position = m_position;
  C34Matrix worldToTransport;
  if (m_transportGUID) {
    C34Matrix transportToWorld;
    MovementGetTransportMtx(m_transportGUID, &transportToWorld);
    worldToTransport = transportToWorld.AffineInverse();
    position *= transportToWorld;
  }

  CAaBox start(
      C3Vector(position.x - m_collisionBoxHalfDepth, position.y - m_collisionBoxHalfDepth, position.z),
      C3Vector(position.x + m_collisionBoxHalfDepth, position.y + m_collisionBoxHalfDepth, position.z + m_collisionBoxHeight)
  );
  CAaBox end(start.b + moveVector, start.t + moveVector);
  CAaBox axisAlign(C3Vector::Min(start.b, end.b), C3Vector::Max(start.t, end.t));

  if (!(m_moveFlags & 0x02000000)) {
    axisAlign.t.z += m_stepUpHeight;
    axisAlign.b.z -= RelDistanceFallen(timeToFall);
  }

  UINT queryFlags = (GetGUID() & 0xF000000000000000ui64) ? 8465 : 273;
  CWorld::GetFacets(axisAlign, &s_facetData, queryFlags);
  CollisionInfoSetFaces(GetGUID(), s_facetData.facets);

  if (m_transportGUID) {
    for (UINT i = 0; i < s_facetData.facets.Count(); ++i) {
      CFacet &facet = s_facetData.facets[i];
      facet.vertices[0] *= worldToTransport;
      facet.vertices[1] *= worldToTransport;
      facet.vertices[2] *= worldToTransport;
      C3Vector normal(
          worldToTransport.a0 * facet.plane.n.x + worldToTransport.b0 * facet.plane.n.y + worldToTransport.c0 * facet.plane.n.z,
          worldToTransport.a1 * facet.plane.n.x + worldToTransport.b1 * facet.plane.n.y + worldToTransport.c1 * facet.plane.n.z,
          worldToTransport.a2 * facet.plane.n.x + worldToTransport.b2 * facet.plane.n.y + worldToTransport.c2 * facet.plane.n.z
      );
      facet.plane.n = normal;
      facet.plane.d = -C3Vector::Dot(normal, facet.vertices[0]);
    }
  }
}

void CMovement::ShowCollisionBox(const C3Vector &unitMove, UINT oldMoveFlags) {
  if (oldMoveFlags & 0x02004000) {
    CollisionInfoSetFallBox(m_position, m_collisionBoxHalfDepth, m_collisionBoxHeight);
    return;
  }

  C3Vector boxMin(m_position.x - m_collisionBoxHalfDepth, m_position.y - m_collisionBoxHalfDepth, m_position.z - m_stepUpHeight);
  C3Vector boxMax(m_position.x + m_collisionBoxHalfDepth, m_position.y + m_collisionBoxHalfDepth, m_position.z + m_collisionBoxHeight);
  CollisionInfoAddBox(boxMin, boxMax);

  C3Vector vectPos = m_position;
  vectPos.z += (m_collisionBoxHeight + m_stepUpHeight) * 0.5f;
  C3Vector &moveDirection = m_moveFlags & 0x1000 ? m_reDirection : unitMove;
  CollisionInfoAddVector(vectPos, moveDirection);
}

BOOL CMovement::CollideRequestMove(DWORD lastUpdateTime, UINT timeElapsed, const C3Vector &moveVector) {
  float distance = moveVector.Mag();
  FallLogWrite("0x%016I64X: ====| Starting new move: elapsed (%u), requested vector (%g,%g)\n", GetGUID(), timeElapsed, moveVector.x, moveVector.y);
  if (!timeElapsed) {
    return 0;
  }

  float elapsedSecs = static_cast<float>(timeElapsed) * 0.001f;
  float currSpeed = distance / elapsedSecs;
  if (!(m_moveFlags & 0x30)) {
    float expectedSpeed = GetCurrentSpeed();
    FallLogWrite("0x%016I64X: requested move (%g), should be (%g) for (%u ms)\n", GetGUID(), distance, expectedSpeed * elapsedSecs, timeElapsed);
  }

  C3Vector moveDirWanted;
  if (distance >= 0.00000095367432f) {
    moveDirWanted = moveVector * (1.0f / distance);
  } else {
    moveDirWanted.Set(0.0f, 0.0f, -1.0f);
  }

  UINT       oldMoveFlags = m_moveFlags;
  UINT       timeUsed = 0;
  UINT       zeroMoves = 0;
  int        moveModified = 0;
  CMoveState state;

  while (timeUsed < timeElapsed) {
    UINT  timeLeft = timeElapsed - timeUsed;
    float distanceLeft = static_cast<float>(timeLeft) * 0.001f * currSpeed;
    UINT  fallenSoFar = 0;
    if (m_moveFlags & 0x4000) {
      fallenSoFar = lastUpdateTime + timeUsed - m_fallStartTime;
    }

    C3Vector &unitMove = m_moveFlags & 0x01000000 ? m_reDirection : moveDirWanted;
    UINT                moveEndFallTime = fallenSoFar + timeElapsed;
    GetMoveFacets(distanceLeft, moveEndFallTime, unitMove);
    SaveMoveState(&state);

    BOOL wasRedirected = (m_moveFlags & 0x1000) != 0;
    UINT timeJustUsed;
    FallLogWrite("0x%016I64X: lastUpdateTime (0x%08X), timeUsed (%d)\n", GetGUID(), lastUpdateTime, timeUsed);

    if (m_moveFlags & 0x02000000) {
      C3Vector moveWanted = unitMove * distanceLeft;
      timeJustUsed = Swim(lastUpdateTime + timeUsed, timeLeft, moveWanted, moveDirWanted);
    } else if (m_moveFlags & 0x4000) {
      if (m_moveFlags & 0x01000000) {
        timeJustUsed = Slide(fallenSoFar, timeLeft);
      } else if (m_moveFlags & 0xF) {
        C3Vector moveWanted = unitMove * distanceLeft;
        C2Vector moveDirWanted2d(moveDirWanted.x, moveDirWanted.y);
        timeJustUsed = ProjectileFall(lastUpdateTime + timeUsed, timeLeft, moveWanted, moveDirWanted2d);
      } else {
        timeJustUsed = Fall(fallenSoFar, timeLeft);
      }
    } else {
      C2Vector unitMove2d(unitMove.x, unitMove.y);
      C2Vector moveDirWanted2d(moveDirWanted.x, moveDirWanted.y);
      timeJustUsed = TraceSurface(lastUpdateTime + timeUsed, timeLeft, distanceLeft, unitMove2d, moveDirWanted2d);
    }

    FallLogWrite("0x%016I64X: move consumed (%d) ms\n", GetGUID(), timeJustUsed);
    if (timeJustUsed + 1 >= timeLeft) {
      if (wasRedirected) {
        m_moveFlags &= ~0x1000U;
        moveModified = 1;
        UpdateAnchors(lastUpdateTime + timeUsed);
      }
      FallLogWrite(
          "0x%016I64X: unit moved full distance: wanted (%g) moved (%g)\n", GetGUID(), distanceLeft,
          static_cast<double>(timeUsed + timeJustUsed) * 0.001 * currSpeed
      );
      timeUsed += timeJustUsed;
      break;
    }

    if (!wasRedirected) {
      moveModified = 1;
      if (!timeJustUsed) {
        ++zeroMoves;
      }
      if (zeroMoves >= 5) {
        FallLogWrite("0x%016I64X: unit executed five zero-time moves\n", GetGUID());
        StopFalling();
        DWORD eventTime = lastUpdateTime + timeUsed;
        HandlePendingActions(eventTime);
        Halt(eventTime);
      }
      if (!(m_moveFlags & 0x400F) || (m_moveFlags & 0x10000000)) {
        FallLogWrite(
            "0x%016I64X: unit blocked, moved (%g) instead of (%g)\n", GetGUID(), static_cast<double>(timeUsed + timeJustUsed) * 0.001 * currSpeed,
            distanceLeft
        );
        break;
      }
      timeUsed += timeJustUsed;
      FallLogWrite(
          "0x%016I64X: unit redirected, moved (%g) instead of (%g)\n", GetGUID(), static_cast<double>(timeUsed) * 0.001 * currSpeed, distanceLeft
      );
      if (timeUsed > timeElapsed) {
        timeUsed = timeElapsed;
      }
    } else {
      RestoreMoveState(state);
      FallLogWrite("0x%016I64X: unit was and still is redirected\n", GetGUID());
    }

    if (!(m_moveFlags & 0x1000)) {
      continue;
    }

    FallLogWrite("0x%016I64X: attempting to redirect\n", GetGUID());
    UINT  redirectedTimeLeft = timeElapsed - timeUsed;
    float speedFactor = C3Vector::Dot(moveDirWanted, m_reDirection);
    if (speedFactor < 0.0f) {
      speedFactor = 0.0f;
    } else if (speedFactor > 1.0f) {
      speedFactor = 1.0f;
    }
    float redirectedDistance = speedFactor * static_cast<float>(redirectedTimeLeft) * 0.001f * currSpeed;
    GetMoveFacets(redirectedDistance, moveEndFallTime, m_reDirection);

    FallLogWrite("0x%016I64X: (redir) lastUpdateTime (0x%08X), timeUsed (%d)\n", GetGUID(), lastUpdateTime, timeUsed);
    UINT redirectedTimeUsed = timeJustUsed;
    if (m_moveFlags & 0x02000000) {
      C3Vector moveWanted = m_reDirection * redirectedDistance;
      redirectedTimeUsed = Swim(lastUpdateTime + timeUsed, redirectedTimeLeft, moveWanted, moveDirWanted);
    } else if (m_moveFlags & 0x4000) {
      if (m_moveFlags & 0xF) {
        C3Vector moveWanted = m_reDirection * redirectedDistance;
        C2Vector moveDirWanted2d(moveDirWanted.x, moveDirWanted.y);
        redirectedTimeUsed = ProjectileFall(lastUpdateTime + timeUsed, redirectedTimeLeft, moveWanted, moveDirWanted2d);
        if ((m_moveFlags & 0x4000) && !redirectedTimeUsed) {
          Halt(lastUpdateTime + timeUsed);
        }
      }
    } else {
      C2Vector redirectedMove(m_reDirection.x, m_reDirection.y);
      C2Vector moveDirWanted2d(moveDirWanted.x, moveDirWanted.y);
      redirectedTimeUsed = TraceSurface(lastUpdateTime + timeUsed, redirectedTimeLeft, redirectedDistance, redirectedMove, moveDirWanted2d);
    }

    FallLogWrite("0x%016I64X: redir move consumed (%d) ms\n", GetGUID(), redirectedTimeUsed);
    if (redirectedTimeUsed < redirectedTimeLeft) {
      moveModified = 1;
    }
    if (!redirectedTimeUsed) {
      ++zeroMoves;
    }
    if (zeroMoves >= 5) {
      StopFalling();
      DWORD eventTime = lastUpdateTime + timeUsed;
      HandlePendingActions(eventTime);
      Halt(eventTime);
    }
    if (!(m_moveFlags & 0x400F) || (m_moveFlags & 0x10000000)) {
      FallLogWrite(
          "0x%016I64X: unit blocked on redirect, moved (%g) instead of (%g)\n", GetGUID(),
          static_cast<double>(timeUsed + redirectedTimeUsed) * 0.001 * speedFactor * currSpeed, redirectedDistance
      );
      break;
    }
    timeUsed += redirectedTimeUsed;
  }

  SetOrientation();
  FallLogWrite("0x%016I64X: ====| Completed move request\n", GetGUID());
  ShowCollisionBox(moveDirWanted, oldMoveFlags);
  return moveModified;
}

float CMovement::CalcFallSurfaceProjection(
    const C3Vector &position,
    DWORD                     moveStartTime,
    UINT                      timeLeft,
    C3Vector        hitPoint,
    float                     distanceAway,
    const C3Vector &moveNormal,
    C4Plane        *platform
) {
  if (CMath::fabs_(distanceAway) >= 0.0013888889f && timeLeft) {
    float distToFall = RelDistanceFallen(moveStartTime, static_cast<float>(timeLeft) * 0.001f);
    hitPoint.z -= distToFall;
    C3Vector fallVector = hitPoint - position;
    fallVector.Normalize();
    C3Vector platformNorm = C3Vector::Cross(fallVector, moveNormal);
    if (platformNorm.z < 0.0f) {
      platformNorm = -platformNorm;
    }
    platform->Set(platformNorm, position);
    FallLogWrite(
        "0x%016I64X: moveNormal(%g,%g,%g) fallVector(%g,%g,%g) hitPoint(%g,%g,%g)\n", GetGUID(), moveNormal.x, moveNormal.y, moveNormal.z, fallVector.x,
        fallVector.y, fallVector.z, hitPoint.x, hitPoint.y, hitPoint.z
    );
    float cosTheta = platformNorm.z;
    if (cosTheta < -1.0f) {
      cosTheta = -1.0f;
    } else if (cosTheta > 1.0f) {
      cosTheta = 1.0f;
    }
    FallLogWrite(
        "0x%016I64X: unit position(%g,%g,%g), platform (%g,%g,%g,%g) (%g deg slope), fall (%g)\n", GetGUID(), position.x, position.y, position.z,
        platform->n.x, platform->n.y, platform->n.z, platform->d, acos(cosTheta) * 57.29578, distToFall
    );
    return distToFall;
  }
  platform->Set(C3Vector(0.0f, 0.0f, 1.0f), position);
  FallLogWrite("0x%016I64X: unit position(%g,%g,%g), flat platform\n", GetGUID(), position.x, position.y, position.z);
  return 0.0f;
}

float CMovement::AttemptMove(
    DWORD                     eventTime,
    const C3Vector &move,
    float                     distance2d,
    const C2Vector &unitMove,
    const C2Vector &unitMoveWanted,
    const C4Plane  &ground
) {
  float distance = move.Mag();
  if (CMath::fabs_(distance) < 0.00000023841858f) {
    FallLogWrite("0x%016I64X: insignificant distance to move (%g), not moving\n", GetGUID(), distance);
    return 0.0f;
  }

  float nearestObstacleDist = ExtrudeCollisionShape(eventTime, move, distance, unitMoveWanted, ground.n);
  if (nearestObstacleDist >= distance2d) {
    FallLogWrite(
        "0x%016I64X: No obstacles hit, moving (%g,%g,%g) from (%g,%g,%g) to ", GetGUID(), move.x, move.y, move.z, m_position.x, m_position.y,
        m_position.z
    );
    m_position += move;
    FallLogWrite("(%g,%g,%g)\n", m_position.x, m_position.y, m_position.z);
    FallLogWrite("0x%016I64X: Nearest obstacle (%g) away, wanted to move (%g)\n", GetGUID(), nearestObstacleDist, distance2d);
    return distance2d;
  }

  C2Vector adjustedMove(nearestObstacleDist * unitMove.x, nearestObstacleDist * unitMove.y);
  FallLogWrite("0x%016I64X: Obstacle hit (%g) away, from (%g,%g,%g) ", GetGUID(), nearestObstacleDist, m_position.x, m_position.y, m_position.z);
  m_position.x += adjustedMove.x;
  m_position.y += adjustedMove.y;
  float newElevation = ground.SolveForZ(m_position.x, m_position.y);
  FallLogWrite("moving (%g,%g,%g) to ", adjustedMove.x, adjustedMove.y, newElevation - m_position.z);
  float adjustedElevation = newElevation - m_position.z;
  if (!(adjustedElevation >= 0.0f)) {
    float maxDrop = -(nearestObstacleDist * 1.1917536f + 0.0013888889f);
    if (maxDrop > adjustedElevation) {
      adjustedElevation = maxDrop;
    }
  }
  m_position.z += adjustedElevation;
  FallLogWrite("(%g,%g,%g)\n", m_position.x, m_position.y, m_position.z);
  if (IsUnitVector(m_reDirection) && (m_moveFlags & 0xF)) {
    m_moveFlags |= 0x1000;
    FallLogWrite("0x%016I64X: Setting redirected (D:\\build\\buildWoW\\WoW\\Source\\Object\\Collide.cpp: %d)\n", GetGUID(), 2470);
  } else {
    Halt(eventTime);
  }
  return nearestObstacleDist;
}

BOOL CMovement::IsTooLow(
    const C3Vector &position,
    DWORD                     moveStartTime,
    CWalkableSurface         *surface,
    float                     distanceMoved,
    float                     currSpeedInv
) {
  float minElevation;
  if (!(distanceMoved < surface->farDist)) {
    if (this) {
      C3Vector intPositionZ = GetPosition(m_position);
      C3Vector intPositionY = GetPosition(m_position);
      C3Vector intPositionX = GetPosition(m_position);
      float              facing = GetFacing(m_facing);
      C3Vector positionZ = GetPosition(m_position);
      C3Vector positionY = GetPosition(m_position);
      C3Vector positionX = GetPosition(m_position);
      int                intFacing = static_cast<int>(GetFacing(m_facing));
      SErrDisplayErrorFmt(
          STORM_ERROR_ASSERTION, __FILE__, __LINE__, 0, 1,
          "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
          "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
          "distanceMoved < surface->farDist", GetGUID(), positionX.x, positionY.y, positionZ.z, facing, static_cast<int>(intPositionX.x),
          static_cast<int>(intPositionY.y), static_cast<int>(intPositionZ.z), intFacing
      );
    } else {
      FATALASSERT(distanceMoved < surface->farDist);
    }
  }
  minElevation = position.z - (surface->farDist - distanceMoved) * 1.1917536f;
  FallLogWrite(
      "0x%016I64X: Checking if face is below: LPOC(%g), minElevation(%g), unit(%g)\n", GetGUID(), surface->lastPtOfContact.z, minElevation, position.z
  );
  if (minElevation - 0.0013888889f <= surface->lastPtOfContact.z) {
    return 0;
  }

  float fallTime = (surface->farDist - distanceMoved) * currSpeedInv * 1000.0f;
  int   timeToFall = fallTime <= 0.0f ? -static_cast<int>(-fallTime + 0.5f) : static_cast<int>(fallTime + 0.5f);
  if (IsJumpingUp(moveStartTime + timeToFall)) {
    return 1;
  }

  if (surface->closeDist - 0.00000095367432f > distanceMoved) {
    fallTime = (surface->closeDist - distanceMoved) * currSpeedInv * 1000.0f;
    timeToFall = fallTime <= 0.0f ? -static_cast<int>(-fallTime + 0.5f) : static_cast<int>(fallTime + 0.5f);
    if (m_moveFlags & 0x4000) {
      timeToFall = moveStartTime + timeToFall - m_fallStartTime;
    }
    if (!(timeToFall >= 0)) {
      if (this) {
        C3Vector intPositionZ = GetPosition(m_position);
        C3Vector intPositionY = GetPosition(m_position);
        C3Vector intPositionX = GetPosition(m_position);
        float              facing = GetFacing(m_facing);
        C3Vector positionZ = GetPosition(m_position);
        C3Vector positionY = GetPosition(m_position);
        C3Vector positionX = GetPosition(m_position);
        int                intFacing = static_cast<int>(GetFacing(m_facing));
        SErrDisplayErrorFmt(
            STORM_ERROR_ASSERTION, __FILE__, __LINE__, 0, 1,
            "\"%s\", guid (0x%016I64X) loc (%g, %g, %g) facing (%g degrees)\n"
            "(0x%08X, 0x%08X, 0x%08X) (0x%08X)",
            "timeToFall >= 0", GetGUID(), positionX.x, positionY.y, positionZ.z, facing, static_cast<int>(intPositionX.x),
            static_cast<int>(intPositionY.y), static_cast<int>(intPositionZ.z), intFacing
        );
      } else {
        FATALASSERT(timeToFall >= 0);
      }
    }
    minElevation = position.z - RelDistanceFallen(timeToFall);
    FallLogWrite(
        "0x%016I64X: Checking if face is below: FPOC(%g), minElevation(%g), unit(%g)\n", GetGUID(), surface->firstPtOfContact.z, minElevation, position.z
    );
  } else {
    minElevation = position.z;
  }
  return minElevation - 0.0013888889f > surface->firstPtOfContact.z;
}

void CMovement::ClipFacetsWithOneAnother(const C4Plane &startPlane, TSGrowableArray<CWalkableSurface> *surfacePool) {
  CWalkableSurface   newSurface2;
  CWalkableSurface   newSurface1;
  C3Vector projected;
  C3Vector intersection;
  float              firstElev;
  C4Plane *surfPlane;
  UINT               numSurfaces = surfacePool->Count();
  if (numSurfaces < 2) {
    return;
  }
  for (UINT surfaceId = 0; surfaceId < numSurfaces - 1; ++surfaceId) {
    for (UINT nextSurfaceId = surfaceId + 1; nextSurfaceId < numSurfaces; ++nextSurfaceId) {
      FATALASSERT(numSurfaces < 10000);
      CWalkableSurface &surface = (*surfacePool)[surfaceId];
      CWalkableSurface &nextSurface = (*surfacePool)[nextSurfaceId];
      if (surface.farDist - 0.0013888889f < nextSurface.closeDist) {
        break;
      }
      if (surface.farDist + 0.0013888889f > nextSurface.farDist) {
        continue;
      }

      surfPlane = &s_facetData.facets[surface.facetId].plane;
      firstElev = surfPlane->DistSigned(nextSurface.firstPtOfContact);
      if (nextSurface.farDist <= surface.farDist) {
        projected = nextSurface.lastPtOfContact;
      } else {
        projected = surface.lastPtOfContact;
        projected.z = s_facetData.facets[nextSurface.facetId].plane.SolveForZ(projected.x, projected.y);
      }

      if (firstElev < 0.0013888889f && surfPlane->DistSigned(projected) < 0.0013888889f) {
        if (nextSurface.farDist - 0.0013888889f < surface.farDist) {
          continue;
        }
        SplitFacetWithFacet(&nextSurface, &surface, &newSurface1);
        InsertSurface(newSurface1, nextSurfaceId, surfacePool);
        ++numSurfaces;
        continue;
      }
      if (firstElev > -0.0013888889f && surfPlane->DistSigned(projected) > -0.0013888889f) {
        if (surface.farDist - 0.0013888889f < nextSurface.farDist) {
          continue;
        }
        SplitFacetWithFacet(&surface, &nextSurface, &newSurface1);
        InsertSurface(newSurface1, nextSurfaceId, surfacePool);
        ++numSurfaces;
        continue;
      }
      if (CMath::fabs_(surface.farDist - nextSurface.farDist) < 0.0013888889f) {
        continue;
      }

      C3Vector direction = nextSurface.lastPtOfContact - nextSurface.firstPtOfContact;
      intersection.Set(0.0f, 0.0f, 0.0f);
      float length = direction.Mag();
      if (CMath::fabs_(length) >= 0.00000095367432f) {
        direction *= 1.0f / length;
        C3Vector reverse = -direction;
        float              denominator = C3Vector::Dot(reverse, surfPlane->n);
        if (CMath::fabs_(denominator) >= 0.00000095367432f) {
          float distance = surfPlane->DistSigned(nextSurface.firstPtOfContact) / denominator;
          if (distance >= -0.00000095367432f && distance <= length + 0.00000095367432f) {
            intersection = nextSurface.firstPtOfContact + direction * distance;
          }
        }
      }
      firstElev = startPlane.DistSigned(intersection);

      newSurface2.closeDist = firstElev;
      newSurface2.farDist = nextSurface.farDist;
      newSurface2.firstPtOfContact = intersection;
      newSurface2.lastPtOfContact = nextSurface.lastPtOfContact;
      newSurface2.facetId = nextSurface.facetId;
      newSurface2.highestElevation = nextSurface.highestElevation;

      newSurface1.closeDist = firstElev;
      newSurface1.farDist = surface.farDist;
      newSurface1.firstPtOfContact = intersection;
      newSurface1.lastPtOfContact = surface.lastPtOfContact;
      newSurface1.facetId = surface.facetId;
      newSurface1.highestElevation = surface.highestElevation;

      surface.farDist = firstElev;
      surface.lastPtOfContact = intersection;
      nextSurface.farDist = firstElev;
      nextSurface.lastPtOfContact = intersection;
      InsertSurface(newSurface2, nextSurfaceId, surfacePool);
      InsertSurface(newSurface1, nextSurfaceId, surfacePool);
      numSurfaces += 2;
    }
  }
}

static void InsertSurface(const CWalkableSurface &toBeInserted, UINT startIndex, TSGrowableArray<CWalkableSurface> *surfacePool) {
  UINT numSurfaces = surfacePool->Count();
  surfacePool->SetCount(numSurfaces + 1);
  UINT insertId;
  for (insertId = startIndex; insertId < numSurfaces; ++insertId) {
    CWalkableSurface &surface = (*surfacePool)[insertId];
    int               insert = 0;
    if (CMath::fabs_(toBeInserted.closeDist - surface.closeDist) >= 0.0013888889f) {
      insert = toBeInserted.closeDist <= surface.closeDist;
    } else if (CMath::fabs_(toBeInserted.firstPtOfContact.z - surface.firstPtOfContact.z) >= 0.001f) {
      insert = toBeInserted.firstPtOfContact.z >= surface.firstPtOfContact.z;
    } else if (CMath::fabs_(toBeInserted.farDist - surface.farDist) < 0.0013888889f) {
      insert = toBeInserted.lastPtOfContact.z >= surface.lastPtOfContact.z;
    } else if (toBeInserted.farDist < surface.farDist) {
      C4Plane &surfacePlane = s_facetData.facets[surface.facetId].plane;
      insert = surfacePlane.SolveForZ(toBeInserted.lastPtOfContact.x, toBeInserted.lastPtOfContact.y) <= toBeInserted.lastPtOfContact.z;
    } else {
      C4Plane &insertedPlane = s_facetData.facets[toBeInserted.facetId].plane;
      insert = insertedPlane.SolveForZ(surface.lastPtOfContact.x, surface.lastPtOfContact.y) >= surface.lastPtOfContact.z;
    }
    if (insert) {
      memmove(surfacePool->Ptr() + insertId + 1, surfacePool->Ptr() + insertId, sizeof(CWalkableSurface) * (numSurfaces - insertId));
      break;
    }
  }
  (*surfacePool)[insertId] = toBeInserted;
}

static void SplitFacetWithFacet(CWalkableSurface *beingSplit, CWalkableSurface *splitBy, CWalkableSurface *newSurface) {
  newSurface->facetId = beingSplit->facetId;
  newSurface->closeDist = splitBy->farDist;
  newSurface->firstPtOfContact.x = splitBy->lastPtOfContact.x;
  newSurface->firstPtOfContact.y = splitBy->lastPtOfContact.y;
  newSurface->firstPtOfContact.z =
      s_facetData.facets[newSurface->facetId].plane.SolveForZ(newSurface->firstPtOfContact.x, newSurface->firstPtOfContact.y);
  newSurface->farDist = beingSplit->farDist;
  newSurface->lastPtOfContact = beingSplit->lastPtOfContact;
  newSurface->highestElevation = beingSplit->highestElevation;
  beingSplit->farDist = newSurface->closeDist;
  beingSplit->lastPtOfContact = newSurface->firstPtOfContact;
}

int CMovement::NextSurfaceIsWalkable(
    CWalkableSurface                  *surface,
    DWORD                              eventTime,
    float                              distanceMoved,
    float                              currSpeedInv,
    TSGrowableArray<CWalkableSurface> *surfacePool
) {
  FATALASSERT(surface);
  FallLogWrite("0x%016I64X: ------>Checking next walkable surface\n", GetGUID());
  C3Vector position = surface->lastPtOfContact;
  C3Vector above = position;
  above.z += m_collisionBoxHeight;
  C4Plane &plane = s_facetData.facets[surface->facetId].plane;
  C4Plane  currentCeiling(-plane.n.x, -plane.n.y, -plane.n.z, C3Vector::Dot(plane.n, above));
  UINT               numSurfaces = surfacePool->Count();
  for (UINT surfaceId = 0; surfaceId < numSurfaces; ++surfaceId) {
    CWalkableSurface *next = GetNextSurface(position, surfaceId, eventTime, distanceMoved, currSpeedInv, currentCeiling, surfacePool);
    if (!next) {
      continue;
    }
    float              stepHeight = next->firstPtOfContact.z - m_position.z;
    C4Plane &nextPlane = s_facetData.facets[next->facetId].plane;
    float              cosTheta = nextPlane.n.z;
    if (cosTheta < -1.0f) {
      cosTheta = -1.0f;
    } else if (cosTheta > 1.0f) {
      cosTheta = 1.0f;
    }
    FallLogWrite("0x%016I64X: Next surface: step hgt(%g), incline(%g degrees)", GetGUID(), stepHeight, acos(cosTheta) * 57.29578f);
    int walkable = -1;
    if (stepHeight > m_stepUpHeight) {
      FallLogWrite(" -- surface was too high\n");
      walkable = 0;
    } else if (nextPlane.n.z > 0.64278764f) {
      FallLogWrite(" -- surface is walkable\n");
      walkable = 1;
    }
    if (walkable >= 0) {
      FallLogWrite("0x%016I64X: ------>Done checking next walkable surface\n", GetGUID());
      return walkable;
    }
    FallLogWrite("\n");
    position = next->lastPtOfContact;
    above = position;
    above.z += m_collisionBoxHeight;
    currentCeiling.Set(-nextPlane.n.x, -nextPlane.n.y, -nextPlane.n.z, C3Vector::Dot(nextPlane.n, above));
  }
  FallLogWrite("0x%016I64X: Ran out of facets, failing\n", GetGUID());
  return 0;
}

CWalkableSurface *CMovement::GetNextSurface(
    const C3Vector          &position,
    UINT                               surfaceId,
    DWORD                              eventTime,
    float                              distanceMoved,
    float                              currSpeedInv,
    const C4Plane           &currentCeiling,
    TSGrowableArray<CWalkableSurface> *surfacePool
) {
  if (surfaceId >= surfacePool->Count()) {
    return 0;
  }
  CWalkableSurface *surface = &(*surfacePool)[surfaceId];
  LogSurface(GetGUID(), *surface);
  C4Plane &plane = s_facetData.facets[surface->facetId].plane;
  if (plane.n.z <= 0.0f) {
    FallLogWrite("0x%016I64X: facet faces downward (z: %g), skipping\n", GetGUID(), plane.n.z);
    return 0;
  }
  if (distanceMoved + 0.0013888889f > surface->farDist) {
    FallLogWrite("0x%016I64X: facet already passed\n", GetGUID());
    return 0;
  }
  if (IsTooLow(position, eventTime, surface, distanceMoved, currSpeedInv)) {
    FallLogWrite("0x%016I64X: facet too low\n", GetGUID());
    return 0;
  }
  float startDistFromTop = currentCeiling.DistSigned(surface->firstPtOfContact);
  float endDistFromTop = currentCeiling.DistSigned(surface->lastPtOfContact);
  if (startDistFromTop >= 0.0013888889f || endDistFromTop >= 0.0013888889f) {
    return surface;
  }
  FallLogWrite("0x%016I64X: facet too high, start(%g) end(%g)\n", GetGUID(), startDistFromTop, endDistFromTop);
  return 0;
}

static void LogSurface(DWORDLONG guid, const CWalkableSurface &surface) {
  float cosTheta = s_facetData.facets[surface.facetId].plane.n.z;
  if (cosTheta < -1.0f) {
    cosTheta = -1.0f;
  } else if (cosTheta > 1.0f) {
    cosTheta = 1.0f;
  }
  CMovement::FallLogWrite(
      "0x%016I64X: facet close(%g) far(%g) FPOC(%g,%g,%g) LPOC(%g,%g,%g) incline(%g degrees)\n", guid, surface.closeDist, surface.farDist,
      surface.firstPtOfContact.x, surface.firstPtOfContact.y, surface.firstPtOfContact.z, surface.lastPtOfContact.x, surface.lastPtOfContact.y,
      surface.lastPtOfContact.z, acos(cosTheta) * 57.29578f
  );
  CMovement::FallLogWrite(
      "0x%016I64X: plane eq(%g, %g, %g, %g)\n", guid, s_facetData.facets[surface.facetId].plane.n.x, s_facetData.facets[surface.facetId].plane.n.y,
      s_facetData.facets[surface.facetId].plane.n.z, s_facetData.facets[surface.facetId].plane.d
  );
}

int CMovement::HandlePendingActions(DWORD eventTime) {
  UINT oldMoveFlags = m_moveFlags;

  if (m_moveFlags & 0x00010000) {
    m_moveFlags &= ~3U;
    BothLogWrite("0x%016I64X: Executing pending stop at (%g,%g,%g)\n", GetGUID(), m_position.x, m_position.y, m_position.z);
  }
  if (m_moveFlags & 0x00020000) {
    m_moveFlags &= ~0xCU;
    BothLogWrite("0x%016I64X: Executing pending unstrafe at (%g,%g,%g)\n", GetGUID(), m_position.x, m_position.y, m_position.z);
  }
  if (!(m_moveFlags & 0xF)) {
    m_moveFlags &= ~0x1000U;
  }
  if (m_moveFlags & 0x00180000) {
    BothLogWrite("0x%016I64X: Executing pending move start at (%g,%g,%g)\n", GetGUID(), m_position.x, m_position.y, m_position.z);
    StartMove(eventTime, m_moveFlags & 0x00080000);
  }
  if (m_moveFlags & 0x00600000) {
    BothLogWrite("0x%016I64X: Executing pending strafe start at (%g,%g,%g)\n", GetGUID(), m_position.x, m_position.y, m_position.z);
    StartStrafe(eventTime, m_moveFlags & 0x00200000);
  }
  if (m_moveFlags & 0x00040000) {
    StartFalling(eventTime);
  }

  m_moveFlags &= 0xFF80FFFF;
  return (m_moveFlags ^ oldMoveFlags) & 0x400F;
}

void CMovement::CheckSurfaceObstacles(
    CWalkableSurface                  *surface,
    DWORD                              eventTime,
    float                             *distanceLeft,
    float                              distanceMoved,
    float                              currSpeedInv,
    TSGrowableArray<CWalkableSurface> *surfacePool,
    CRedirect                         *hitInfo
) {
  float cosTheta = surface->firstPtOfContact.z - m_position.z;
  if (cosTheta > m_stepUpHeight) {
    FallLogWrite(
        "0x%016I64X: Facet too high (facet: %g, unit: %g, step hgt: %g), obstructing\n", GetGUID(), surface->firstPtOfContact.z, m_position.z,
        m_stepUpHeight
    );
    *distanceLeft = surface->closeDist - distanceMoved;
    if (*distanceLeft < 0.0f) {
      *distanceLeft = 0.0f;
    }
    hitInfo->hitPoint = surface->firstPtOfContact;
    hitInfo->surfaceNorm[0] = s_facetData.facets[surface->facetId].plane.n;
    hitInfo->flags = 4;
    return;
  }

  if (cosTheta > 0.0013888889f && !TestStepUp(surface->firstPtOfContact)) {
    FallLogWrite(
        "0x%016I64X: Step up blocked (facet: %g, unit: %g, step hgt: %g), obstructing\n", GetGUID(), surface->firstPtOfContact.z, m_position.z,
        m_stepUpHeight
    );
    *distanceLeft = surface->closeDist - distanceMoved;
    hitInfo->hitPoint = surface->firstPtOfContact;
    hitInfo->surfaceNorm[0] = s_facetData.facets[surface->facetId].plane.n;
    hitInfo->flags = 4;
    return;
  }

  if (s_facetData.facets[surface->facetId].plane.n.z > 0.64278764f) {
    return;
  }
  if (surface->lastPtOfContact.z - surface->firstPtOfContact.z < -0.0013888889f) {
    cosTheta = s_facetData.facets[surface->facetId].plane.n.z;
    if (cosTheta < -1.0f) {
      cosTheta = -1.0f;
    } else if (cosTheta > 1.0f) {
      cosTheta = 1.0f;
    }
    FallLogWrite("0x%016I64X: Facet too steep (%g degree slope), but moving down-hill\n", GetGUID(), acos(cosTheta) * 57.29578f);
    m_moveFlags |= 0x40000;
    return;
  }

  if (surface->highestElevation - m_position.z <= m_stepUpHeight &&
      NextSurfaceIsWalkable(surface, eventTime, distanceMoved, currSpeedInv, surfacePool))
  {
    cosTheta = s_facetData.facets[surface->facetId].plane.n.z;
    if (cosTheta < -1.0f) {
      cosTheta = -1.0f;
    } else if (cosTheta > 1.0f) {
      cosTheta = 1.0f;
    }
    FallLogWrite("0x%016I64X: Facet too steep (%g degree slope), but low enough to step over\n", GetGUID(), acos(cosTheta) * 57.29578f);
    return;
  }

  cosTheta = s_facetData.facets[surface->facetId].plane.n.z;
  if (cosTheta < -1.0f) {
    cosTheta = -1.0f;
  } else if (cosTheta > 1.0f) {
    cosTheta = 1.0f;
  }
  FallLogWrite(
      "0x%016I64X: Facet too steep (%g degree slope), highest elev (%g), unit Z(%g) obstructing\n", GetGUID(), acos(cosTheta) * 57.29578f,
      surface->highestElevation, m_position.z
  );
  *distanceLeft = surface->closeDist - distanceMoved;
  if (*distanceLeft < 0.0f) {
    *distanceLeft = 0.0f;
  }
  hitInfo->hitPoint = surface->firstPtOfContact;
  hitInfo->surfaceNorm[0] = s_facetData.facets[surface->facetId].plane.n;
  hitInfo->flags = 4;
}

UINT CMovement::Swim(DWORD eventTime, UINT timeToMove, const C3Vector &moveWanted, const C3Vector &unitMoveWanted) {
  if (!timeToMove || (m_transportGUID && FallFromTransport())) {
    return 0;
  }

  FallLogWrite("0x%016I64X: ====| Starting new swim: time to move (%u)\n", GetGUID(), timeToMove);
  float    distanceMoved;
  C3Vector move = moveWanted;
  C3Vector unitMove = moveWanted;
  float    distance = unitMove.Mag();
  if (CMath::fequal_(distance, 0.0f)) {
    return 0;
  }

  unitMove /= distance;
  if (!s_facetData.facets.Count()) {
    distanceMoved = CollideWithWaterSurface(unitMove, unitMoveWanted, distance);
  } else if (moveWanted.z >= 0.0f) {
    distanceMoved = ExtrudeFlyBoxUp(unitMove, unitMoveWanted, distance);
  } else {
    distanceMoved = ExtrudeFlyBoxDown(unitMove, unitMoveWanted, distance);
  }

  if (distanceMoved >= distance) {
    m_position += move;
    distanceMoved = distance;
  } else {
    move = unitMove * distanceMoved;
    m_position += move;
  }

  if (IsLocalPlayer() && m_position.z - 0.0013888889f > m_waterSurfaceElev) {
    StopSwimLocal(eventTime + CMath::fint_((distanceMoved / distance) * timeToMove));
    ProcessLocalMoveEvent(204);
  }

  FallLogWrite(
      "0x%016I64X: ====| Swam (%g) (%g,%g,%g) to (%g,%g,%g)\n", GetGUID(), distanceMoved, move.x, move.y, move.z, m_position.x, m_position.y,
      m_position.z
  );
  m_groundNormal.Set(0.0f, 0.0f, 1.0f);
  if (distanceMoved == distance) {
    return timeToMove;
  }
  return CMath::fint_n((CMath::fabs_(distanceMoved) / distance) * (timeToMove * 0.001f) * 1000.0f);
}

UINT CMovement::ProjectileFall(DWORD eventTime, UINT timeToMove, const C3Vector &moveWanted, const C2Vector &unitMoveWanted) {
  if (!timeToMove || (m_transportGUID && FallFromTransport())) {
    return 0;
  }

  FallLogWrite(
      "0x%016I64X: ====| Starting new projectile fall: time to move (%u), time fallen so far (%d)\n", GetGUID(), timeToMove, eventTime - m_fallStartTime
  );
  float              distToFall = RelDistanceFallen(eventTime, static_cast<float>(timeToMove) * 0.001f);
  C3Vector move(moveWanted.x, moveWanted.y, -distToFall);
  float              distance = move.Mag();
  if (CMath::fabs_(distance) < 0.00000023841858f) {
    return 0;
  }

  float              inverseDistance = 1.0f / distance;
  C3Vector unitMove = move * inverseDistance;
  FallLogWrite("0x%016I64X: ====| Wanted to projectile fall (%g) (%g,%g,%g)\n", GetGUID(), distance, move.x, move.y, move.z);

  DWORDLONG gameObjHit = 0;
  float     distanceMoved = FLT_MAX;
  if (s_facetData.facets.Count()) {
    if (distToFall >= 0.0f) {
      distanceMoved = ExtrudeProjectileBoxDownHill(eventTime, unitMove, distance, unitMoveWanted, &gameObjHit);
    } else {
      distanceMoved = ExtrudeProjectileBoxUpHill(unitMove, distance, &gameObjHit);
    }
  }

  UINT timeUsed = timeToMove;
  if (distanceMoved < distance) {
    move = unitMove * distanceMoved;
    m_position += move;
    float adjustedTime = CMath::fabs_(distanceMoved) * inverseDistance * (static_cast<float>(timeToMove) * 0.001f) * 1000.0f;
    if (adjustedTime <= 0.0f) {
      timeUsed = static_cast<UINT>(-static_cast<long>(-adjustedTime + 0.5f));
    } else {
      timeUsed = static_cast<UINT>(adjustedTime + 0.5f);
    }

    if (m_moveFlags & 0x4000) {
      if (distToFall < 0.0f) {
        ProcessFallReset(eventTime + timeUsed);
      }
    } else if (gameObjHit) {
      SetTransport(gameObjHit);
    }
  } else {
    distanceMoved = distance;
    m_position += move;
    if (m_moveFlags & 0x4000) {
      CheckFallenFar(eventTime + timeUsed);
    } else if (gameObjHit) {
      SetTransport(gameObjHit);
    }
  }

  if (!(m_moveFlags & 0x4000)) {
    HandlePendingActions(eventTime + timeUsed);
  }
  FallLogWrite(
      "0x%016I64X: ====| Projectile fell (%g) (%g,%g,%g) to (%g,%g,%g)\n", GetGUID(), distanceMoved, move.x, move.y, move.z, m_position.x, m_position.y,
      m_position.z
  );
  m_groundNormal.Set(0.0f, 0.0f, 1.0f);
  return timeUsed;
}

UINT CMovement::TraceSurface(
    DWORD                     eventTime,
    UINT                      timeToMove,
    float                     distance,
    const C2Vector &unitMove,
    const C2Vector &unitMoveWanted
) {
  FATALASSERT(!(m_moveFlags & 0x4000));
  FATALASSERT(!(m_moveFlags & 0x01000000));
  FATALASSERT(!(m_moveFlags & 0x02000000));
  FATALASSERT(!(m_moveFlags & 0x10000000));
  FATALASSERT(m_moveFlags & 0xF);
  if (CMath::fabs_(distance) < 0.00000095367432f) {
    return timeToMove;
  }

  CRedirect          hitInfo;
  C4Plane  slopeTestPlane(unitMove.y, -unitMove.x, 0.0f, -(unitMove.y * m_position.x - unitMove.x * m_position.y));
  C3Vector normal;
  C4Plane  currentCeiling;
  C4Plane  startPlane;
  float              absDistMoved = 0.0f;
  float              currentSpeedInv = static_cast<float>(timeToMove) * 0.001f / distance;
  float              cosTheta;
  C2Vector moveVector(unitMove.x * distance, unitMove.y * distance);
  UINT               numSurfaces;
  C3Vector above;
  C4Plane  currentPlatform;
  UINT               surfaceId;
  float              distanceMoved;
  CWalkableSurface  *surface;
  DWORD              currEventTime;
  float              segmentDist;
  UINT               timeUsed;
  float              distMoved;
  float              adjustedDist;
  C3Vector fullMoveVector;

  FallLogWrite(
      "0x%016I64X: *** Requesting new move vector(%g,%g), current time (0x%08X), elapsed (%u), distance (%g)\n", GetGUID(), moveVector.x, moveVector.y,
      eventTime, timeToMove, distance
  );
  surfacePool.SetCount(0);
  EnqueueFacets(slopeTestPlane, *reinterpret_cast<C2Vector *>(&m_position), unitMove, s_facetData.facets, &surfacePool);

  startPlane.Set(C3Vector(unitMove.x, unitMove.y, 0.0f), m_position);
  ClipFacetsWithOneAnother(startPlane, &surfacePool);

  fullMoveVector = m_position;
  fullMoveVector.x += moveVector.x;
  fullMoveVector.y += moveVector.y;
  above = m_position;
  above.z += m_collisionBoxHeight;
  CalcFallSurfaceProjection(m_position, eventTime, timeToMove, fullMoveVector, distance, slopeTestPlane.n, &currentPlatform);
  normal = -currentPlatform.n;
  currentCeiling.Set(normal, above);

  numSurfaces = surfacePool.Count();
  surfaceId = 0;
  distanceMoved = 0.0f;
  timeUsed = 0;
  while (surfaceId < numSurfaces && timeUsed < timeToMove) {
    currEventTime = eventTime + timeUsed;
    surface = GetNextSurface(m_position, surfaceId, currEventTime, distanceMoved, currentSpeedInv, currentCeiling, &surfacePool);
    if (!surface) {
      ++surfaceId;
      continue;
    }
    if (surface->closeDist - distanceMoved >= m_collisionBoxHalfDepth) {
      ProcessFalling(currEventTime);
      break;
    }

    fullMoveVector.Set(unitMove.x, unitMove.y, 0.0f);
    hitInfo.Reset();
    adjustedDist = distance;
    CheckSurfaceObstacles(surface, currEventTime, &adjustedDist, distanceMoved, currentSpeedInv, &surfacePool, &hitInfo);
    FATALASSERT(!(m_moveFlags & 0x4000));
    if (SetTransport(s_facetData.gameObjects[surface->facetId]) || HandlePendingActions(currEventTime)) {
      return timeUsed < timeToMove ? timeUsed : timeToMove;
    }

    segmentDist = surface->farDist - distanceMoved;
    if (CMath::fabs_(surface->closeDist - distanceMoved - adjustedDist) < 0.0013888889f && hitInfo.flags) {
      fullMoveVector *= adjustedDist;
      fullMoveVector.z = currentPlatform.SolveForZ(m_position.x + fullMoveVector.x, m_position.y + fullMoveVector.y) - m_position.z;
      if (fullMoveVector.z < -(adjustedDist * 1.1917536f + 0.0013888889f)) {
        fullMoveVector.z = -(adjustedDist * 1.1917536f + 0.0013888889f);
      }
      segmentDist = adjustedDist;
      FallLogWrite(
          "0x%016I64X: Completing move at start of facet, attempting to move (%g,%g,%g)\n", GetGUID(), fullMoveVector.x, fullMoveVector.y,
          fullMoveVector.z
      );
    } else if (segmentDist < adjustedDist) {
      fullMoveVector *= segmentDist;
      fullMoveVector.z = surface->lastPtOfContact.z - m_position.z;
      if (fullMoveVector.z < -(segmentDist * 1.1917536f + 0.0013888889f)) {
        fullMoveVector.z = -(segmentDist * 1.1917536f + 0.0013888889f);
      }
      FallLogWrite(
          "0x%016I64X: moving to end of facet, attempting to move (%g,%g,%g)\n", GetGUID(), fullMoveVector.x, fullMoveVector.y, fullMoveVector.z
      );
    } else {
      fullMoveVector *= adjustedDist;
      fullMoveVector.z =
          s_facetData.facets[surface->facetId].plane.SolveForZ(m_position.x + fullMoveVector.x, m_position.y + fullMoveVector.y) - m_position.z;
      if (fullMoveVector.z < -(adjustedDist * 1.1917536f + 0.0013888889f)) {
        fullMoveVector.z = -(adjustedDist * 1.1917536f + 0.0013888889f);
      }
      segmentDist = adjustedDist;
      FallLogWrite(
          "0x%016I64X: Completing move on facet, attempting to move (%g,%g,%g)\n", GetGUID(), fullMoveVector.x, fullMoveVector.y, fullMoveVector.z
      );
    }

    distMoved = AttemptMove(currEventTime, fullMoveVector, segmentDist, unitMove, unitMoveWanted, s_facetData.facets[surface->facetId].plane);
    BOOL wasRedirected = m_moveFlags & 0x1000;
    distanceMoved += distMoved;
    absDistMoved += CMath::fabs_(distMoved);
    distance -= CMath::fabs_(segmentDist);
    if (!wasRedirected && !(m_moveFlags & 0x1000) && (m_moveFlags & 0xF) && hitInfo.flags) {
      fullMoveVector.Set(unitMoveWanted.x, unitMoveWanted.y, 0.0f);
      Redirect(currEventTime, fullMoveVector, currentPlatform.n, hitInfo);
    }

    timeUsed = absDistMoved * currentSpeedInv * 1000.0f <= 0.0f
                   ? static_cast<UINT>(-static_cast<int>(-(absDistMoved * currentSpeedInv * 1000.0f) + 0.5f))
                   : static_cast<UINT>(absDistMoved * currentSpeedInv * 1000.0f + 0.5f);
    if ((!wasRedirected && (m_moveFlags & 0x1000)) || CMath::fabs_(distMoved - segmentDist) >= 0.00000095367432f ||
        distance < 0.00000095367432f || (m_moveFlags & 0x10000000))
    {
      return timeUsed < timeToMove ? timeUsed : timeToMove;
    }

    currentPlatform = s_facetData.facets[surface->facetId].plane;
    cosTheta = currentPlatform.n.z;
    if (cosTheta < -1.0f) {
      cosTheta = -1.0f;
    } else if (cosTheta > 1.0f) {
      cosTheta = 1.0f;
    }
    FallLogWrite(
        "0x%016I64X: setting platform (%g,%g,%g,%g) (%g deg slope)\n", GetGUID(), currentPlatform.n.x, currentPlatform.n.y, currentPlatform.n.z,
        currentPlatform.d, acos(cosTheta) * 57.29578f
    );
    above = m_position;
    above.z += m_collisionBoxHeight;
    normal = -currentPlatform.n;
    currentCeiling.Set(normal, above);
    ++surfaceId;
  }

  if (distance > 0.00000095367432f || timeUsed != timeToMove) {
    FallLogWrite("0x%016I64X: no more facets, falling\n", GetGUID());
    ProcessFalling(eventTime + timeUsed);
  }
  return timeUsed < timeToMove ? timeUsed : timeToMove;
}

static void EnqueueFacets(
    const C4Plane                 &slopeTestPlane,
    const C2Vector                &position,
    const C2Vector                &unitMove,
    const TSGrowableArray<CFacet> &facets,
    TSGrowableArray<CWalkableSurface>       *surfacePool
) {
  surfacePool->SetCount(0);
  CClippedTriangle  poly;
  C4Plane startPlane(unitMove.x, unitMove.y, 0.0f, -(position.x * unitMove.x + position.y * unitMove.y));
  UINT              next;
  startPlane.Set(-startPlane.n.x, -startPlane.n.y, -startPlane.n.z, -startPlane.d);
  UINT count = facets.Count();
  for (UINT i = 0; i < count; ++i) {
    const CFacet &facet = facets[i];
    if (facet.plane.n.z < 0.00000095367432f) {
      continue;
    }
    poly.Init(facet.vertices);
    ClipPolygonToPlane(startPlane, &poly);
    UINT numClippedVerts = poly.Count();
    FATALASSERT(!numClippedVerts || numClippedVerts > 2);
    float              closestDist = FLT_MAX;
    float              farthestDist = -FLT_MAX;
    C3Vector closest;
    C3Vector farthest;
    C3Vector intersection;
    int                hitTri = 0;
    for (next = 0; next < numClippedVerts; ++next) {
      intersection = poly[(next + 1) % numClippedVerts] - poly[next];
      float length = intersection.Mag();
      if (CMath::fabs_(length) < 0.00000095367432f) {
        continue;
      }
      intersection *= 1.0f / length;
      C3Vector reverse = -intersection;
      float              denominator = C3Vector::Dot(reverse, slopeTestPlane.n);
      if (CMath::fabs_(denominator) < 0.00000095367432f) {
        continue;
      }
      float distance = slopeTestPlane.DistSigned(poly[next]) / denominator;
      if (distance < -0.00000095367432f || distance > length + 0.00000095367432f) {
        continue;
      }
      intersection = poly[next] + intersection * distance;
      intersection.z = facet.plane.SolveForZ(intersection.x, intersection.y);
      hitTri = 1;
      if (unitMove.x * intersection.x + unitMove.y * intersection.y - startPlane.d < closestDist) {
        closestDist = unitMove.x * intersection.x + unitMove.y * intersection.y - startPlane.d;
        closest = intersection;
      }
      if (unitMove.x * intersection.x + unitMove.y * intersection.y - startPlane.d > farthestDist) {
        farthestDist = unitMove.x * intersection.x + unitMove.y * intersection.y - startPlane.d;
        farthest = intersection;
      }
    }

    if (!hitTri || CMath::fabs_(closestDist - farthestDist) < 0.00000095367432f || farthestDist < 0.00000095367432f) {
      continue;
    }

    float highestZ = -FLT_MAX;
    for (next = 0; next < 3; ++next) {
      intersection = facet.vertices[(next + 1) % 3] - facet.vertices[next];
      float length = intersection.Mag();
      if (CMath::fabs_(length) < 0.00000095367432f) {
        continue;
      }
      intersection *= 1.0f / length;
      C3Vector reverse = -intersection;
      float              denominator = C3Vector::Dot(reverse, slopeTestPlane.n);
      if (CMath::fabs_(denominator) < 0.00000095367432f) {
        continue;
      }
      float distance = slopeTestPlane.DistSigned(facet.vertices[next]) / denominator;
      if (distance < -0.00000095367432f || distance > length + 0.00000095367432f) {
        continue;
      }
      intersection = facet.vertices[next] + intersection * distance;
      if (intersection.z > highestZ) {
        highestZ = intersection.z;
      }
    }

    CWalkableSurface surface;
    surface.closeDist = closestDist;
    surface.farDist = farthestDist;
    surface.firstPtOfContact = closest;
    surface.lastPtOfContact = farthest;
    surface.facetId = i;
    surface.highestElevation = highestZ;
    surfacePool->Add(&surface);
  }
  qsort(surfacePool->Ptr(), surfacePool->Count(), sizeof(CWalkableSurface), FacetCompare);
}

static float ClipPolygonToPlane(const C4Plane &plane, CClippedTriangle *poly) {
  FATALASSERT(poly);
  if (!poly->Count()) {
    return 0.0f;
  }

  static CClippedTriangle input;
  input = *poly;
  poly->SetCount(0);
  float penetrationDepth = -FLT_MAX;

  for (UINT i = 0; i < input.Count(); ++i) {
    UINT nextIndex = i + 1;
    if (nextIndex == input.Count()) {
      nextIndex = 0;
    }

    float dist1 = plane.DistSigned(input[i]);
    float dist2 = plane.DistSigned(input[nextIndex]);
    if (-dist1 > penetrationDepth) {
      penetrationDepth = -dist1;
    }

    if ((dist1 < 0.0f || dist2 <= 0.0013888889f) && (dist1 <= 0.0013888889f || dist2 < 0.0f)) {
      if (dist1 > 0.0013888889f || dist2 > 0.0013888889f) {
        C3Vector intersection = input[i] + (input[nextIndex] - input[i]) * (dist1 / (dist1 - dist2));
        if (dist1 <= 0.0f && dist2 > 0.0f) {
          if (!poly->Count() || poly->Last() != input[i]) {
            poly->Add(input[i]);
          }
          poly->Add(intersection);
        } else if (dist1 > 0.0f && dist2 <= 0.0f) {
          poly->Add(intersection);
          if (nextIndex) {
            poly->Add(input[nextIndex]);
          }
        }
      } else {
        if (!poly->Count() || poly->Last() != input[i]) {
          poly->Add(input[i]);
        }
        if (nextIndex || !poly->Count() || (*poly)[0] != input[nextIndex]) {
          poly->Add(input[nextIndex]);
        }
      }
    }
  }

  if (poly->Count() < 3) {
    poly->SetCount(0);
  }
  return penetrationDepth;
}

static int __cdecl FacetCompare(LPCVOID elem1, LPCVOID elem2) {
  return CWalkableSurface::HasHigherPriority(*static_cast<const CWalkableSurface *>(elem1), *static_cast<const CWalkableSurface *>(elem2)) ? -1 : 1;
}

int CMovement::DetermineHitType(int hitType, const C3Vector &unitMove, float distance, UINT facetId, CRedirect *hitInfo) {
  FATALASSERT(hitType < NUM_HITTYPES);
  switch (hitType) {
    case 0:
      return DetermineBoxHitType(unitMove, distance, facetId, m_collisionBoxHalfDepth * 1.849399f, hitInfo);
    case 1:
      DeterminePyramidHitType(unitMove, distance, facetId, hitInfo);
      break;
    case 2:
      return DetermineBoxHitType(unitMove, distance, facetId, m_stepUpHeight, hitInfo);
  }
  FATALASSERT(hitInfo->flags != 0);
  return 1;
}

void CMovement::DeterminePyramidHitType(const C3Vector &unitMove, float distance, UINT facetId, CRedirect *hitInfo) {
  C3Vector newPosition = m_position + unitMove * distance;
  int                hitPyrTrailingX = unitMove.x >= 0.0f;
  int                hitPyrTrailingY = unitMove.y >= 0.0f;

  C3Vector pyrNormX(0.87964189f, 0.0f, -0.4756366f);
  C3Vector pyrNormY(0.0f, 0.87964189f, -0.4756366f);
  C4Plane  pyrSides[4];
  pyrSides[!hitPyrTrailingX].Set(pyrNormX, newPosition);
  pyrNormX.x = -pyrNormX.x;
  pyrSides[hitPyrTrailingX].Set(pyrNormX, newPosition);
  pyrSides[2 + !hitPyrTrailingY].Set(pyrNormY, newPosition);
  pyrNormY.y = -pyrNormY.y;
  pyrSides[2 + hitPyrTrailingY].Set(pyrNormY, newPosition);

  C4Plane topBottom[2];
  topBottom[0].Set(0.0f, 0.0f, 1.0f, -newPosition.z - m_collisionBoxHalfDepth * 1.849399f);
  topBottom[1].Set(0.0f, 0.0f, -1.0f, newPosition.z);

  CFacet &facet = s_facetData.facets[facetId];
  CClippedTriangle  clippedPoly;
  clippedPoly.Init(facet.vertices);
  float unused;
  if (!ClipPolygonToPolyhedron(topBottom, 2, &clippedPoly, &unused)) {
    hitInfo->flags |= 0x40;
    return;
  }

  CClippedTriangle testPoly = clippedPoly;
  ClipPolygonToPolyhedron(&pyrSides[2], 2, &testPoly, &unused);
  int hitTrailingX = PolygonIntersectsPlane(pyrSides[0], testPoly);
  int hitLeadingX = PolygonIntersectsPlane(pyrSides[1], testPoly);

  testPoly = clippedPoly;
  ClipPolygonToPolyhedron(pyrSides, 2, &testPoly, &unused);
  int hitTrailingY = PolygonIntersectsPlane(pyrSides[2], testPoly);
  int hitLeadingY = PolygonIntersectsPlane(pyrSides[3], testPoly);

  if (hitTrailingX && hitTrailingY) {
    if (!hitLeadingX || !hitLeadingY) {
      hitInfo->flags |= 4;
    } else {
      hitInfo->flags |= 0x40;
    }
  } else if (hitTrailingX && hitLeadingY) {
    hitInfo->flags |= 8;
  } else if (hitLeadingX && hitTrailingY) {
    hitInfo->flags |= 16;
  } else if (hitTrailingX) {
    hitInfo->flags |= 1;
  } else if (hitTrailingY) {
    hitInfo->flags |= 2;
  } else {
    hitInfo->flags |= 0x40;
  }
}

static BOOL PolygonIntersectsPlane(const C4Plane &plane, const CClippedTriangle &poly) {
  int allPositive = 1;
  int allNegative = 1;
  for (UINT i = 0; i < poly.Count(); ++i) {
    float distance = plane.DistSigned(poly[i]);
    if (CMath::fabs_(distance) < 0.0013888889f) {
      return 1;
    }
    allPositive &= distance > 0.0013888889f;
    allNegative &= distance < -0.0013888889f;
  }
  return !allPositive && !allNegative;
}

static BOOL ClipPolygonToPolyhedron(const C4Plane *boxSides, UINT numSides, CClippedTriangle *poly, float *penetrationDepth) {
  *penetrationDepth = FLT_MAX;
  for (UINT side = 0; side < numSides; ++side) {
    float depth = ClipPolygonToPlane(boxSides[side], poly);
    if (!poly->Count()) {
      return 0;
    }
    if (side >= 2 && depth < *penetrationDepth) {
      *penetrationDepth = depth;
    }
  }
  return 1;
}

BOOL CMovement::DetermineBoxHitType(const C3Vector &unitMove, float distance, UINT facetId, float baseHeight, CRedirect *hitInfo) {
  C3Vector newPosition = m_position + unitMove * distance;
  int                hitBoxTrailingX = unitMove.x >= 0.0f;
  int                hitBoxTrailingY = unitMove.y >= 0.0f;

  C4Plane boxSides[4];
  boxSides[!hitBoxTrailingX].Set(1.0f, 0.0f, 0.0f, -newPosition.x - m_collisionBoxHalfDepth);
  boxSides[hitBoxTrailingX].Set(-1.0f, 0.0f, 0.0f, newPosition.x - m_collisionBoxHalfDepth);
  boxSides[2 + !hitBoxTrailingY].Set(0.0f, 1.0f, 0.0f, -newPosition.y - m_collisionBoxHalfDepth);
  boxSides[2 + hitBoxTrailingY].Set(0.0f, -1.0f, 0.0f, newPosition.y - m_collisionBoxHalfDepth);

  C4Plane topBottom[2];
  topBottom[0].Set(0.0f, 0.0f, 1.0f, -newPosition.z - m_collisionBoxHeight);
  topBottom[1].Set(0.0f, 0.0f, -1.0f, newPosition.z + baseHeight);

  CFacet &facet = s_facetData.facets[facetId];
  CClippedTriangle  clippedPoly;
  clippedPoly.Init(facet.vertices);
  float unused;
  if (!ClipPolygonToPolyhedron(topBottom, 2, &clippedPoly, &unused)) {
    return 0;
  }

  CClippedTriangle testPoly = clippedPoly;
  ClipPolygonToPolyhedron(&boxSides[2], 2, &testPoly, &unused);
  int hitTrailingX = PolygonIntersectsPlane(boxSides[0], testPoly);
  int hitLeadingX = PolygonIntersectsPlane(boxSides[1], testPoly);

  testPoly = clippedPoly;
  ClipPolygonToPolyhedron(boxSides, 2, &testPoly, &unused);
  int hitTrailingY = PolygonIntersectsPlane(boxSides[2], testPoly);
  int hitLeadingY = PolygonIntersectsPlane(boxSides[3], testPoly);

  if (hitTrailingX && hitTrailingY) {
    hitInfo->flags |= 4;
  } else if (hitTrailingX && hitLeadingY) {
    hitInfo->flags |= 8;
  } else if (hitLeadingX && hitTrailingY) {
    hitInfo->flags |= 16;
  } else if (hitTrailingX) {
    hitInfo->flags |= 1;
  } else if (hitTrailingY) {
    hitInfo->flags |= 2;
  } else {
    DetermineBoxParallelHitType(boxSides, clippedPoly, hitInfo);
  }
  return 1;
}

static void DetermineBoxParallelHitType(const C4Plane *boxSides, const CClippedTriangle &clippedPoly, CRedirect *hitInfo) {
  float xDist = FindClosestVertDist(boxSides[0], &boxSides[2], clippedPoly);
  float yDist = FindClosestVertDist(boxSides[2], &boxSides[0], clippedPoly);
  if (CMath::fabs_(xDist - yDist) < 0.00000095367432f) {
    hitInfo->flags |= 3;
  } else if (xDist >= yDist) {
    hitInfo->flags |= 2;
  } else {
    hitInfo->flags |= 1;
  }
}

static float FindClosestVertDist(const C4Plane &front, const C4Plane *sides, const CClippedTriangle &poly) {
  CClippedTriangle testPoly = poly;
  float            unused;
  if (!ClipPolygonToPolyhedron(sides, 2, &testPoly, &unused)) {
    return FLT_MAX;
  }

  float minDist = FLT_MAX;
  UINT  count = testPoly.Count();
  for (UINT i = 0; i < count; ++i) {
    float distance = front.DistSigned(testPoly[i]);
    if (distance < minDist) {
      minDist = distance;
    }
  }
  return minDist;
}

C3Vector CMovement::CalcAverageSurfaceNormal(const C4Plane *box, UINT count) {
  C3Vector average(0.0f);
  UINT               numNormals = 0;
  UINT               numFacets = s_facetData.facets.Count();
  float              penetrationDepth;
  for (UINT facetId = 0; facetId < numFacets; ++facetId) {
    CFacet &facet = s_facetData.facets[facetId];
    if (facet.plane.n.z <= 0.017452406f) {
      continue;
    }

    CClippedTriangle clippedPoly;
    clippedPoly.Init(facet.vertices);
    if (ClipPolygonToPolyhedron(box, count, &clippedPoly, &penetrationDepth) && penetrationDepth >= 0.013888889f) {
      average += facet.plane.n;
      ++numNormals;
    }
  }

  if (!numNormals) {
    return C3Vector(0.0f, 0.0f, 1.0f);
  }
  average *= 1.0f / static_cast<float>(numNormals);
  average.Normalize();
  return average;
}

void CMovement::FindObstacles(
    const C3Vector &unitMoveVector,
    C4Plane        *box,
    UINT                      numSides,
    const C4Plane  &startPlane,
    int                       hitType,
    float                    *closestDist,
    CRedirect                *hitInfo
) {
  UINT  numFacets = s_facetData.facets.Count();
  float minDist;
  BYTE  savedFlags;
  for (UINT facetId = 0; facetId < numFacets; ++facetId) {
    if (C3Vector::Dot(s_facetData.facets[facetId].plane.n, unitMoveVector) > -0.017452406f) {
      CollisionInfoColorFace(facetId, FACET_TESTED_UNTOUCHED);
      continue;
    }

    CClippedTriangle clippedPoly;
    clippedPoly.Init(s_facetData.facets[facetId].vertices);
    float penetrationDepth;
    if (!ClipPolygonToPolyhedron(box, numSides, &clippedPoly, &penetrationDepth)) {
      CollisionInfoColorFace(facetId, FACET_TESTED_UNTOUCHED);
      continue;
    }
    if (penetrationDepth < 0.013888889f) {
      CollisionInfoColorFace(facetId, FACET_TESTED_UNTOUCHED);
      continue;
    }
    CollisionInfoColorFace(facetId, FACET_TESTED_TOUCHED);

    C3Vector closest(0.0f);
    minDist = FLT_MAX;
    for (UINT i = 0; i < clippedPoly.Count(); ++i) {
      float distance = DistFromPlaneAlongVector(clippedPoly[i], startPlane, unitMoveVector);
      if (distance < minDist) {
        minDist = distance;
        closest = clippedPoly[i];
      }
    }

    if (hitInfo && hitInfo->flags && CMath::fabs_(minDist - *closestDist) < 0.0013888889f) {
      float dx = closest.x - hitInfo->hitPoint.x;
      float dy = closest.y - hitInfo->hitPoint.y;
      float sqDist = dx * dx + dy * dy;
      if (facetId < s_facetData.gameObjects.Count() && s_facetData.gameObjects[facetId]) {
        hitInfo->gameObjHit = s_facetData.gameObjects[facetId];
      }
      FallLogWrite("0x%016I64X: Hit another obstacle at (%g,%g)\n", GetGUID(), closest.x, closest.y);

      if (CMath::fabs_(sqDist) < 0.000048225309f) {
        float newDot = C3Vector::Dot(s_facetData.facets[facetId].plane.n, unitMoveVector);
        float oldDot = C3Vector::Dot(hitInfo->surfaceNorm[0], unitMoveVector);
        if (CMath::fabs_(newDot - oldDot) >= 0.00000095367432f) {
          hitInfo->flags |= 0x80;
          if (newDot >= oldDot) {
            hitInfo->surfaceNorm[1] = s_facetData.facets[facetId].plane.n;
          } else {
            FallLogWrite("0x%016I64X: Obstacle replaces previous edge\n", GetGUID());
            hitInfo->surfaceNorm[1] = hitInfo->surfaceNorm[0];
            hitInfo->surfaceNorm[0] = s_facetData.facets[facetId].plane.n;
          }
        }
      } else {
        FallLogWrite("0x%016I64X: Obstacle is multi-hit, sq dist(%g)\n", GetGUID(), static_cast<double>(sqDist));
        FallLogWrite("0x%016I64X: closest(%g,%g) hitInfo->hitPoint(%g,%g)\n", GetGUID(), closest.x, closest.y, hitInfo->hitPoint.x, hitInfo->hitPoint.y);
        if (DetermineHitType(hitType, unitMoveVector, minDist, facetId, hitInfo)) {
          *closestDist = minDist;
          CollisionInfoColorFace(facetId, FACET_BLOCKING);
        }
      }
      continue;
    }

    if (minDist >= *closestDist) {
      continue;
    }
    if (!hitInfo) {
      *closestDist = minDist;
      CollisionInfoColorFace(facetId, FACET_BLOCKING);
      continue;
    }

    savedFlags = hitInfo->flags;
    hitInfo->flags = 0;
    if (!DetermineHitType(hitType, unitMoveVector, minDist, facetId, hitInfo)) {
      hitInfo->flags = savedFlags;
      continue;
    }

    FallLogWrite("0x%016I64X: Hit new obstacle at (%g,%g)\n", GetGUID(), closest.x, closest.y);
    hitInfo->surfaceNorm[0] = s_facetData.facets[facetId].plane.n;
    hitInfo->gameObjHit = facetId < s_facetData.gameObjects.Count() ? s_facetData.gameObjects[facetId] : 0;
    hitInfo->hitPoint = closest;
    *closestDist = minDist;
    CollisionInfoColorFace(facetId, FACET_BLOCKING);
  }
}

static float DistFromPlaneAlongVector(const C3Vector &point, const C4Plane &plane, const C3Vector &unitVector) {
  float directDist = plane.DistSigned(point);
  float cosTheta = C3Vector::Dot(plane.n, unitVector);
  FATALASSERT(CMath::fnotequal_(cosTheta, 0.0f));
  return directDist / cosTheta;
}

float CMovement::ExtrudeAlignedDownHill(
    DWORD                     timeStamp,
    const C3Vector &moveVector,
    float                     distanceWanted,
    const C2Vector &unitMoveWanted,
    const C3Vector &platformNorm
) {
  C4Plane boxPlanes[6];
  if (CMath::fabs_(moveVector.x) < 0.00000095367432f) {
    ExtrudeBoxSideY(moveVector, m_stepUpHeight, boxPlanes);
  } else {
    ExtrudeBoxSideX(moveVector, m_stepUpHeight, boxPlanes);
  }

  C4Plane startPlane = boxPlanes[0];
  startPlane.d += 0.027777778f;

  C3Vector unitMove = moveVector;
  unitMove.Normalize();

  float     distance = FLT_MAX;
  CRedirect hitInfo;
  FallLogWrite("0x%016I64X: Checking XY side:\n", GetGUID());
  FindObstacles(unitMove, boxPlanes, 6, startPlane, 2, &distance, &hitInfo);

  if (distance < distanceWanted && hitInfo.flags) {
    Redirect(timeStamp, C3Vector(unitMoveWanted.x, unitMoveWanted.y, 0.0f), platformNorm, hitInfo);
  }
  return GetDist2d(unitMove, distance);
}

static float GetDist2d(const C3Vector &unitMove, float closestDist3D) {
  if (closestDist3D >= FLT_MAX) {
    return FLT_MAX;
  }
  float horizontal = C2Vector(closestDist3D * unitMove.x, closestDist3D * unitMove.y).Mag();
  return closestDist3D >= 0.0f ? horizontal : -horizontal;
}

float CMovement::ExtrudeAlignedUpHill(
    DWORD                     timeStamp,
    const C3Vector &moveVector,
    float                     distanceWanted,
    const C2Vector &unitMoveWanted,
    const C3Vector &platformNorm
) {
  C4Plane zBoxPlanes[6];
  ExtrudeBoxSideZ(moveVector, m_stepUpHeight, zBoxPlanes);

  C4Plane xyBoxPlanes[6];
  if (CMath::fabs_(moveVector.x) < 0.00000095367432f) {
    ExtrudeBoxSideY(moveVector, m_stepUpHeight, xyBoxPlanes);
  } else {
    ExtrudeBoxSideX(moveVector, m_stepUpHeight, xyBoxPlanes);
  }

  C4Plane startPlanes[2];
  startPlanes[0] = xyBoxPlanes[0];
  startPlanes[0].d += 0.027777778f;
  startPlanes[1] = zBoxPlanes[0];
  startPlanes[1].d += 0.027777778f;

  C3Vector unitMove = moveVector;
  unitMove.Normalize();

  float     distance = FLT_MAX;
  float     distanceZ = FLT_MAX;
  CRedirect hitInfoXY;
  CRedirect hitInfoZ;

  FallLogWrite("0x%016I64X: Checking XY side:\n", GetGUID());
  FindObstacles(unitMove, xyBoxPlanes, 6, startPlanes[0], 2, &distance, &hitInfoXY);
  FallLogWrite("0x%016I64X: Checking Z side:\n", GetGUID());
  FindObstacles(unitMove, zBoxPlanes, 6, startPlanes[1], 2, &distanceZ, &hitInfoZ);

  if (hitInfoXY.flags && distance - distanceZ < 0.0013888889f) {
    if (CMath::fabs_(distance - distanceZ) >= 0.0013888889f) {
      hitInfoZ.flags = 0;
    }
    if (distance < distanceWanted) {
      Redirect(timeStamp, C3Vector(unitMoveWanted.x, unitMoveWanted.y, 0.0f), platformNorm, hitInfoXY);
    }
  } else if (hitInfoZ.flags) {
    if (CMath::fabs_(distance - distanceZ) >= 0.0013888889f) {
      hitInfoXY.flags = 0;
    }
    distance = distanceZ;
    if (distanceZ < distanceWanted) {
      Redirect(timeStamp, C3Vector(unitMoveWanted.x, unitMoveWanted.y, 0.0f), platformNorm, hitInfoZ);
    }
  }
  return GetDist2d(unitMove, distance);
}

float CMovement::ExtrudeUnalignedDownHill(
    DWORD                     timeStamp,
    const C3Vector &moveVector,
    float                     distanceWanted,
    const C2Vector &unitMoveWanted,
    const C3Vector &platformNorm
) {
  C4Plane xBoxPlanes[6];
  C4Plane yBoxPlanes[6];
  ExtrudeBoxSideX(moveVector, m_stepUpHeight, xBoxPlanes);
  ExtrudeBoxSideY(moveVector, m_stepUpHeight, yBoxPlanes);

  C4Plane startPlanes[2];
  startPlanes[0] = xBoxPlanes[0];
  startPlanes[0].d += 0.027777778f;
  startPlanes[1] = yBoxPlanes[0];
  startPlanes[1].d += 0.027777778f;

  C3Vector unitMove = moveVector;
  unitMove.Normalize();

  float     distance = FLT_MAX;
  float     distanceY = FLT_MAX;
  CRedirect hitInfoX;
  CRedirect hitInfoY;

  FallLogWrite("0x%016I64X: Checking X side:\n", GetGUID());
  FindObstacles(unitMove, xBoxPlanes, 6, startPlanes[0], 2, &distance, &hitInfoX);
  FallLogWrite("0x%016I64X: Checking Y side:\n", GetGUID());
  FindObstacles(unitMove, yBoxPlanes, 6, startPlanes[1], 2, &distanceY, &hitInfoY);

  if (distance >= distanceY) {
    if (CMath::fabs_(distance - distanceY) >= 0.0013888889f) {
      hitInfoX.flags = 0;
    }
    distance = distanceY;
  } else {
    if (CMath::fabs_(distance - distanceY) >= 0.0013888889f) {
      hitInfoY.flags = 0;
    }
  }

  if (distance < distanceWanted && (hitInfoX.flags || hitInfoY.flags)) {
    Redirect(timeStamp, C3Vector(unitMoveWanted.x, unitMoveWanted.y, 0.0f), platformNorm, hitInfoX, hitInfoY);
  }
  return GetDist2d(unitMove, distance);
}

float CMovement::ExtrudeUnalignedUpHill(
    DWORD                     timeStamp,
    const C3Vector &moveVector,
    float                     distanceWanted,
    const C2Vector &unitMoveWanted,
    const C3Vector &platformNorm
) {
  C4Plane xBoxPlanes[6];
  C4Plane yBoxPlanes[6];
  C4Plane zBoxPlanes[6];
  ExtrudeBoxSideX(moveVector, m_stepUpHeight, xBoxPlanes);
  ExtrudeBoxSideY(moveVector, m_stepUpHeight, yBoxPlanes);
  ExtrudeBoxSideZ(moveVector, m_stepUpHeight, zBoxPlanes);

  C4Plane startPlanes[3];
  startPlanes[0] = xBoxPlanes[0];
  startPlanes[0].d += 0.027777778f;
  startPlanes[1] = yBoxPlanes[0];
  startPlanes[1].d += 0.027777778f;
  startPlanes[2] = zBoxPlanes[0];
  startPlanes[2].d += 0.027777778f;

  C3Vector unitMove = moveVector;
  unitMove.Normalize();

  C3Vector paramDist(FLT_MAX, FLT_MAX, FLT_MAX);
  CRedirect          hitInfoX;
  CRedirect          hitInfoY;
  CRedirect          hitInfoZ;

  FallLogWrite("0x%016I64X: Checking X side:\n", GetGUID());
  FindObstacles(unitMove, xBoxPlanes, 6, startPlanes[0], 2, &paramDist.x, &hitInfoX);
  FallLogWrite("0x%016I64X: Checking Y side:\n", GetGUID());
  FindObstacles(unitMove, yBoxPlanes, 6, startPlanes[1], 2, &paramDist.y, &hitInfoY);
  FallLogWrite("0x%016I64X: Checking Z side:\n", GetGUID());
  FindObstacles(unitMove, zBoxPlanes, 6, startPlanes[2], 2, &paramDist.z, &hitInfoZ);

  CRedirect *hitInfoA;
  CRedirect *hitInfoB;
  float      distance;
  int        wantRedirect = 0;

  if (!hitInfoX.flags || paramDist.x >= paramDist.y || paramDist.x - paramDist.z >= 0.0013888889f) {
    if (hitInfoY.flags && paramDist.y - paramDist.z < 0.0013888889f) {
      distance = paramDist.y;
      if (CMath::fabs_(paramDist.y - paramDist.x) >= 0.0013888889f) {
        hitInfoX.flags = 0;
      }
      if (CMath::fabs_(paramDist.y - paramDist.z) >= 0.0013888889f) {
        hitInfoZ.flags = 0;
      }
      if (paramDist.y < distanceWanted) {
        hitInfoA = &hitInfoX;
        hitInfoB = &hitInfoY;
        wantRedirect = 1;
      }
    } else if (hitInfoZ.flags) {
      distance = paramDist.z;
      if (CMath::fabs_(paramDist.z - paramDist.x) >= 0.0013888889f) {
        hitInfoX.flags = 0;
      }
      if (CMath::fabs_(paramDist.z - paramDist.y) >= 0.0013888889f) {
        hitInfoY.flags = 0;
      }
      if (paramDist.z < distanceWanted) {
        if (CMath::fabs_(unitMoveWanted.y) >= CMath::fabs_(unitMoveWanted.x)) {
          hitInfoA = &hitInfoY;
        } else {
          hitInfoA = &hitInfoX;
        }
        hitInfoB = &hitInfoZ;
        wantRedirect = 1;
      }
    } else {
      distance = FLT_MAX;
    }
  } else {
    distance = paramDist.x;
    if (CMath::fabs_(paramDist.x - paramDist.y) >= 0.0013888889f) {
      hitInfoY.flags = 0;
    }
    if (CMath::fabs_(paramDist.x - paramDist.z) >= 0.0013888889f) {
      hitInfoZ.flags = 0;
    }
    if (paramDist.x < distanceWanted) {
      hitInfoA = &hitInfoX;
      hitInfoB = &hitInfoY;
      wantRedirect = 1;
    }
  }

  if (wantRedirect) {
    Redirect(timeStamp, C3Vector(unitMoveWanted.x, unitMoveWanted.y, 0.0f), platformNorm, *hitInfoA, *hitInfoB);
  }
  return GetDist2d(unitMove, distance);
}

float CMovement::ExtrudeCollisionShape(
    DWORD                     timeStamp,
    const C3Vector &moveVector,
    float                     distanceWanted,
    const C2Vector &unitMoveWanted,
    const C3Vector &platformNorm
) {
  int aligned = CMath::fabs_(moveVector.x) < 0.00000095367432f || CMath::fabs_(moveVector.y) < 0.00000095367432f;
  if (moveVector.z < 0.00000095367432f) {
    return aligned ? ExtrudeAlignedDownHill(timeStamp, moveVector, distanceWanted, unitMoveWanted, platformNorm)
                   : ExtrudeUnalignedDownHill(timeStamp, moveVector, distanceWanted, unitMoveWanted, platformNorm);
  }
  return aligned ? ExtrudeAlignedUpHill(timeStamp, moveVector, distanceWanted, unitMoveWanted, platformNorm)
                 : ExtrudeUnalignedUpHill(timeStamp, moveVector, distanceWanted, unitMoveWanted, platformNorm);
}

BOOL CMovement::TestStepUp(const C3Vector &destination) {
  float stepHeight = destination.z - m_position.z;
  if (stepHeight <= 0.0f) {
    return 1;
  }
  if (FindCeilingDistanceAbove(stepHeight) < stepHeight) {
    return 0;
  }
  C3Vector moveVector = destination - m_position;
  float              distWanted = moveVector.Mag();
  if (distWanted < 0.00000023841858f) {
    return 1;
  }
  C3Vector unitMove = moveVector;
  unitMove.Normalize();
  C4Plane xBoxPlanes[6];
  C4Plane yBoxPlanes[6];
  C4Plane startPlanes[2];
  ExtrudeBoxSideX(moveVector, m_stepUpHeight, xBoxPlanes);
  ExtrudeBoxSideY(moveVector, m_stepUpHeight, yBoxPlanes);
  float     distance = FLT_MAX;
  CRedirect hitInfo;
  startPlanes[0] = xBoxPlanes[0];
  startPlanes[0].d += 0.027777778f;
  startPlanes[1] = yBoxPlanes[0];
  startPlanes[1].d += 0.027777778f;
  FindObstacles(unitMove, xBoxPlanes, 6, startPlanes[0], 0, &distance, &hitInfo);
  FindObstacles(unitMove, yBoxPlanes, 6, startPlanes[1], 0, &distance, &hitInfo);
  return distance >= distWanted;
}

void CMovement::ExtrudeBoxSideZ(const C3Vector &moveVector, float bottom, C4Plane boxSides[]) {
  int posZ = moveVector.z >= 0.0f;

  C4Plane basePlane;
  if (posZ) {
    basePlane.n.Set(0.0f, 0.0f, 1.0f);
    basePlane.d = -(m_collisionBoxHeight + m_position.z);
  } else {
    basePlane.n.Set(0.0f, 0.0f, -1.0f);
    basePlane.d = m_stepUpHeight + m_position.z;
  }

  float height[2] = {bottom, m_collisionBoxHeight};

  C3Vector moveDir[2] = {-moveVector, moveVector};

  boxSides[0].n = -basePlane.n;
  boxSides[0].d = -basePlane.d - 0.027777778f;

  boxSides[1].n = basePlane.n;
  boxSides[1].d = basePlane.d - (moveDir[posZ].z > 0.027777778f ? moveDir[posZ].z : 0.027777778f);

  C3Vector leftPoint(m_position.x - m_collisionBoxHalfDepth, m_position.y - m_collisionBoxHalfDepth, m_position.z + height[posZ]);
  C3Vector rghtPoint(m_position.x + m_collisionBoxHalfDepth, m_position.y + m_collisionBoxHalfDepth, m_position.z + height[posZ]);

  C3Vector posYNorm(0.0f, moveDir[posZ].z, -moveDir[posZ].y);
  C3Vector negYNorm(0.0f, -moveDir[posZ].z, moveDir[posZ].y);
  float              negYMag = negYNorm.Mag();
  if (CMath::fabs_(negYMag) >= 0.00000023841858f) {
    float invMag = 1.0f / negYMag;
    negYNorm *= invMag;
    posYNorm *= invMag;
  }

  C3Vector posXNorm(moveDir[posZ].z, 0.0f, -moveDir[posZ].x);
  C3Vector negXNorm(-moveDir[posZ].z, 0.0f, moveDir[posZ].x);
  float              posXMag = posXNorm.Mag();
  if (CMath::fabs_(posXMag) >= 0.00000023841858f) {
    float invMag = 1.0f / posXMag;
    posXNorm *= invMag;
    negXNorm *= invMag;
  }

  boxSides[2].n = negYNorm;
  boxSides[2].d = -C3Vector::Dot(leftPoint, negYNorm);
  boxSides[3].n = posYNorm;
  boxSides[3].d = -C3Vector::Dot(rghtPoint, posYNorm);
  boxSides[4].n = posXNorm;
  boxSides[4].d = -C3Vector::Dot(rghtPoint, posXNorm);
  boxSides[5].n = negXNorm;
  boxSides[5].d = -C3Vector::Dot(leftPoint, negXNorm);
}

void CMovement::ExtrudeBoxSideY(const C3Vector &moveVector, float bottom, C4Plane boxSides[]) {
  int posY = moveVector.y >= 0.0f;

  C4Plane basePlane;
  if (posY) {
    basePlane.n.Set(0.0f, 1.0f, 0.0f);
    basePlane.d = -(m_position.y + m_collisionBoxHalfDepth);
  } else {
    basePlane.n.Set(0.0f, -1.0f, 0.0f);
    basePlane.d = m_position.y - m_collisionBoxHalfDepth;
  }

  float depth[2] = {-m_collisionBoxHalfDepth, m_collisionBoxHalfDepth};

  C3Vector moveDir[2] = {-moveVector, moveVector};

  boxSides[0].n = -basePlane.n;
  boxSides[0].d = -basePlane.d - 0.027777778f;

  boxSides[1].n = basePlane.n;
  boxSides[1].d = basePlane.d - (moveDir[posY].y > 0.027777778f ? moveDir[posY].y : 0.027777778f);

  C3Vector topPoint(m_position.x + m_collisionBoxHalfDepth, m_position.y + depth[posY], m_position.z + m_collisionBoxHeight);
  C3Vector botPoint(m_position.x - m_collisionBoxHalfDepth, m_position.y + depth[posY], m_position.z + bottom);

  C3Vector posXNorm(moveDir[posY].y, moveDir[!posY].x, 0.0f);
  C3Vector negXNorm(moveDir[!posY].y, moveDir[posY].x, 0.0f);
  float              posXMag = posXNorm.Mag();
  if (CMath::fabs_(posXMag) >= 0.00000023841858f) {
    float invMag = 1.0f / posXMag;
    posXNorm *= invMag;
    negXNorm *= invMag;
  }

  C3Vector posZNorm(0.0f, moveDir[!posY].z, moveDir[posY].y);
  C3Vector negZNorm(0.0f, moveDir[posY].z, moveDir[!posY].y);
  float              posZMag = posZNorm.Mag();
  if (CMath::fabs_(posZMag) >= 0.00000023841858f) {
    float invMag = 1.0f / posZMag;
    posZNorm *= invMag;
    negZNorm *= invMag;
  }

  boxSides[2].n = posXNorm;
  boxSides[2].d = -C3Vector::Dot(topPoint, posXNorm);
  boxSides[3].n = negXNorm;
  boxSides[3].d = -C3Vector::Dot(botPoint, negXNorm);
  boxSides[4].n = posZNorm;
  boxSides[4].d = -C3Vector::Dot(topPoint, posZNorm);
  boxSides[5].n = negZNorm;
  boxSides[5].d = -C3Vector::Dot(botPoint, negZNorm);
}

void CMovement::ExtrudeBoxSideX(const C3Vector &moveVector, float bottom, C4Plane boxSides[]) {
  int posX = moveVector.x >= 0.0f;

  C4Plane basePlane;
  basePlane.n.Set(posX ? 1.0f : -1.0f, 0.0f, 0.0f);
  basePlane.d = (posX ? -m_position.x : m_position.x) - m_collisionBoxHalfDepth;

  float depth[2] = {-m_collisionBoxHalfDepth, m_collisionBoxHalfDepth};

  C3Vector moveDir[2] = {-moveVector, moveVector};

  boxSides[0].n = -basePlane.n;
  boxSides[0].d = -basePlane.d - 0.027777778f;

  boxSides[1].n = basePlane.n;
  boxSides[1].d = basePlane.d - (moveDir[posX].x > 0.027777778f ? moveDir[posX].x : 0.027777778f);

  C3Vector topPoint(m_position.x + depth[posX], m_position.y + m_collisionBoxHalfDepth, m_position.z + m_collisionBoxHeight);
  C3Vector botPoint(m_position.x + depth[posX], m_position.y - m_collisionBoxHalfDepth, m_position.z + bottom);

  C3Vector posYNorm(moveDir[!posX].y, moveDir[posX].x, 0.0f);
  C3Vector negYNorm(moveDir[posX].y, moveDir[!posX].x, 0.0f);
  float              negYMag = negYNorm.Mag();
  if (CMath::fabs_(negYMag) >= 0.00000023841858f) {
    float invMag = 1.0f / negYMag;
    negYNorm *= invMag;
    posYNorm *= invMag;
  }

  C3Vector posZNorm(moveDir[!posX].z, 0.0f, moveDir[posX].x);
  C3Vector negZNorm(moveDir[posX].z, 0.0f, moveDir[!posX].x);
  float              posZMag = posZNorm.Mag();
  if (CMath::fabs_(posZMag) >= 0.00000023841858f) {
    float invMag = 1.0f / posZMag;
    posZNorm *= invMag;
    negZNorm *= invMag;
  }

  boxSides[2].n = negYNorm;
  boxSides[2].d = -C3Vector::Dot(botPoint, negYNorm);
  boxSides[3].n = posYNorm;
  boxSides[3].d = -C3Vector::Dot(topPoint, posYNorm);
  boxSides[4].n = posZNorm;
  boxSides[4].d = -C3Vector::Dot(topPoint, posZNorm);
  boxSides[5].n = negZNorm;
  boxSides[5].d = -C3Vector::Dot(botPoint, negZNorm);
}

BOOL CMovement::ExtrudePyramidSideX(const C3Vector &unitMove, float distance, C4Plane boxSides[]) {
  if (CMath::fabs_(unitMove.x) < 0.00000023841858f && CMath::fabs_(unitMove.z) < 0.00000023841858f) {
    return 0;
  }
  C3Vector moveVector = unitMove * distance;
  C3Vector moveDir[2] = {-moveVector, moveVector};
  int                posX = unitMove.x >= 0.0f;
  float              depth[2] = {-m_collisionBoxHalfDepth, m_collisionBoxHalfDepth};

  float pyrNormX = posX ? 0.87964189f : -0.87964189f;

  C3Vector posPyramidEdge(depth[posX], m_collisionBoxHalfDepth, m_collisionBoxHalfDepth * 1.849399f);
  C3Vector negPyramidEdge(posPyramidEdge.x, -posPyramidEdge.y, posPyramidEdge.z);

  C3Vector posZNorm(moveDir[!posX].z, 0.0f, moveDir[posX].x);
  float              posZMag = posZNorm.Mag();
  if (CMath::fabs_(posZMag) >= 0.00000023841858f) {
    posZNorm *= 1.0f / posZMag;
  }

  boxSides[0].n.Set(-pyrNormX, 0.0f, 0.4756366f);
  boxSides[0].d = -C3Vector::Dot(boxSides[0].n, m_position);

  float              clampedDist = distance > 0.027777778f ? distance : 0.027777778f;
  C3Vector advanced(unitMove.x * clampedDist, unitMove.y * clampedDist, unitMove.z * clampedDist);
  advanced.Set(advanced.x + m_position.x, advanced.y + m_position.y, advanced.z + m_position.z);
  boxSides[1].n.Set(pyrNormX, 0.0f, -0.4756366f);
  boxSides[1].d = -C3Vector::Dot(boxSides[1].n, advanced);

  boxSides[2] = C4Plane(C3Vector(0.0f), moveVector, negPyramidEdge);
  if (boxSides[2].n.y > 0.0f) {
    boxSides[2] = -boxSides[2];
  }
  boxSides[2].d = -C3Vector::Dot(boxSides[2].n, m_position);

  boxSides[3] = C4Plane(C3Vector(0.0f), moveVector, posPyramidEdge);
  if (boxSides[3].n.y < 0.0f) {
    boxSides[3] = -boxSides[3];
  }
  boxSides[3].d = -C3Vector::Dot(boxSides[3].n, m_position);

  negPyramidEdge.Set(posPyramidEdge.x + m_position.x, posPyramidEdge.y + m_position.y, posPyramidEdge.z + m_position.z);
  boxSides[4].n = posZNorm;
  boxSides[4].d = -C3Vector::Dot(boxSides[4].n, negPyramidEdge);

  return 1;
}

BOOL CMovement::ExtrudePyramidSideY(const C3Vector &unitMove, float distance, C4Plane boxSides[]) {
  if (CMath::fabs_(unitMove.y) < 0.00000023841858f && CMath::fabs_(unitMove.z) < 0.00000023841858f) {
    return 0;
  }
  C3Vector moveVector = unitMove * distance;
  C3Vector moveDir[2] = {-moveVector, moveVector};
  int                posY = unitMove.y >= 0.0f;
  float              depth[2] = {-m_collisionBoxHalfDepth, m_collisionBoxHalfDepth};

  float pyrNormY = posY ? 0.87964189f : -0.87964189f;

  C3Vector posPyramidEdge(m_collisionBoxHalfDepth, depth[posY], m_collisionBoxHalfDepth * 1.849399f);
  C3Vector negPyramidEdge(-posPyramidEdge.x, posPyramidEdge.y, posPyramidEdge.z);

  C3Vector posZNorm(0.0f, moveDir[!posY].z, moveDir[posY].y);
  float              posZMag = posZNorm.Mag();
  if (CMath::fabs_(posZMag) >= 0.00000023841858f) {
    posZNorm *= 1.0f / posZMag;
  }

  boxSides[0].n.Set(0.0f, -pyrNormY, 0.4756366f);
  boxSides[0].d = -C3Vector::Dot(boxSides[0].n, m_position);

  float              clampedDist = distance > 0.027777778f ? distance : 0.027777778f;
  C3Vector advanced(unitMove.x * clampedDist, unitMove.y * clampedDist, unitMove.z * clampedDist);
  advanced.Set(advanced.x + m_position.x, advanced.y + m_position.y, advanced.z + m_position.z);
  boxSides[1].n.Set(0.0f, pyrNormY, -0.4756366f);
  boxSides[1].d = -C3Vector::Dot(boxSides[1].n, advanced);

  boxSides[2] = C4Plane(C3Vector(0.0f), moveVector, negPyramidEdge);
  if (boxSides[2].n.x > 0.0f) {
    boxSides[2] = -boxSides[2];
  }
  boxSides[2].d = -C3Vector::Dot(boxSides[2].n, m_position);

  boxSides[3] = C4Plane(C3Vector(0.0f), moveVector, posPyramidEdge);
  if (boxSides[3].n.x < 0.0f) {
    boxSides[3] = -boxSides[3];
  }
  boxSides[3].d = -C3Vector::Dot(boxSides[3].n, m_position);

  negPyramidEdge.Set(posPyramidEdge.x + m_position.x, posPyramidEdge.y + m_position.y, posPyramidEdge.z + m_position.z);
  boxSides[4].n = posZNorm;
  boxSides[4].d = -C3Vector::Dot(boxSides[4].n, negPyramidEdge);

  return 1;
}
