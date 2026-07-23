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

struct CWalkableSurface {
  CWalkableSurface() : closeDist(0.0f), farDist(0.0f), firstPtOfContact(0.0f), lastPtOfContact(0.0f), facetId(0), highestElevation(0.0f) {
  }

  float              closeDist;
  float              farDist;
  NTempest::C3Vector firstPtOfContact;
  NTempest::C3Vector lastPtOfContact;
  unsigned int       facetId;
  float              highestElevation;
};

class CClippedTriangle {
 public:
  CClippedTriangle() : numVerts(0) {
  }

  void Init(const NTempest::C3Vector *vertices) {
    numVerts = 3;
    verts[0] = vertices[0];
    verts[1] = vertices[1];
    verts[2] = vertices[2];
  }

  NTempest::C3Vector *Ptr() {
    return verts;
  }

  NTempest::C3Vector &operator[](unsigned int index) {
    FATALASSERT(index < numVerts);
    return verts[index];
  }

  NTempest::C3Vector &Last() {
    FATALASSERT(numVerts);
    return verts[numVerts - 1];
  }

  void SetCount(unsigned int count) {
    FATALASSERT(count <= 9);
    numVerts = count;
  }

  unsigned int Count() const {
    return numVerts;
  }

  void Add(const NTempest::C3Vector &vertex) {
    FATALASSERT(numVerts < 9);
    verts[numVerts++] = vertex;
  }

 private:
  NTempest::C3Vector verts[9];
  unsigned int       numVerts;
};

static CWFacetData                       s_facetData;
static TSGrowableArray<CWalkableSurface> surfacePool;

static float s_gravityRate = 19.291105f;
static float s_terminalVelocity = 60.148003f;

float CMovement::CalcFallStartElevation(unsigned int timeFallen) {
  float jumpVelocity = m_jumpVelocity;
  float fallTime = timeFallen * 0.001f;
  float velocity = s_gravityRate * fallTime;

  if (velocity + jumpVelocity <= s_terminalVelocity) {
    return (jumpVelocity + velocity * 0.5f) * fallTime + m_position.z;
  }

  float terminalTime = (s_terminalVelocity - jumpVelocity) / s_gravityRate;
  return (fallTime - terminalTime + terminalTime * 0.5f) * s_terminalVelocity + m_position.z;
}

void __stdcall MovementSetGravityRate(float metersPerSecSqd) {
  s_gravityRate = metersPerSecSqd * 1.0936f;
}

void __stdcall MovementSetTerminalVelocity(float metersPerSec) {
  s_terminalVelocity = metersPerSec * 1.0936f;
}

float MovementGetTerminalVelocity() {
  return s_terminalVelocity * 0.91439998f;
}

CRedirect::CRedirect() {
  Reset();
}

CRedirect::~CRedirect() {
}

void CRedirect::Reset() {
  hitPoint = NTempest::C3Vector(0.0f);
  surfaceNorm[0] = NTempest::C3Vector(0.0f);
  surfaceNorm[1] = NTempest::C3Vector(0.0f);
  gameObjHit = 0;
  flags = 0;
}

static int PolygonIntersectsPlane(NTempest::C4Plane &plane, CClippedTriangle &poly) {
  int allPositive = 1;
  int allNegative = 1;
  for (unsigned int i = 0; i < poly.Count(); ++i) {
    float distance = plane.DistSigned(poly[i]);
    if (NTempest::CMath::fabs_(distance) < 0.0013888889f) {
      return 1;
    }
    allPositive &= distance > 0.0013888889f;
    allNegative &= distance < -0.0013888889f;
  }
  return !allPositive && !allNegative;
}

static float ClipPolygonToPlane(NTempest::C4Plane &plane, CClippedTriangle *poly) {
  CClippedTriangle input = *poly;
  poly->SetCount(0);
  float penetrationDepth = -FLT_MAX;
  if (!input.Count()) {
    return 0.0f;
  }

  for (unsigned int i = 0; i < input.Count(); ++i) {
    unsigned int nextIndex = i + 1;
    if (nextIndex == input.Count()) {
      nextIndex = 0;
    }

    NTempest::C3Vector current = input[i];
    NTempest::C3Vector next = input[nextIndex];
    float              dist1 = plane.DistSigned(current);
    float              dist2 = plane.DistSigned(next);
    if (-dist1 > penetrationDepth) {
      penetrationDepth = -dist1;
    }

    if ((dist1 < 0.0f || dist2 <= 0.0013888889f) && (dist1 <= 0.0013888889f || dist2 < 0.0f)) {
      if (dist1 > 0.0013888889f || dist2 > 0.0013888889f) {
        float              t = dist1 / (dist1 - dist2);
        NTempest::C3Vector intersection = current + (next - current) * t;
        if (dist1 <= 0.0f && dist2 > 0.0f) {
          if (!poly->Count() || poly->Last() != current) {
            poly->Add(current);
          }
          poly->Add(intersection);
        } else if (dist1 > 0.0f && dist2 <= 0.0f) {
          poly->Add(intersection);
          if (nextIndex) {
            poly->Add(next);
          }
        }
      } else {
        if (!poly->Count() || poly->Last() != current) {
          poly->Add(current);
        }
        if (nextIndex || !poly->Count() || (*poly)[0] != next) {
          poly->Add(next);
        }
      }
    }
  }

  if (poly->Count() < 3) {
    poly->SetCount(0);
  }
  return penetrationDepth;
}

static int ClipPolygonToPolyhedron(NTempest::C4Plane *boxSides, unsigned int numSides, CClippedTriangle *poly, float *penetrationDepth) {
  *penetrationDepth = FLT_MAX;
  for (unsigned int side = 0; side < numSides; ++side) {
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

static float DistFromPlaneAlongVector(NTempest::C3Vector &point, NTempest::C4Plane &plane, NTempest::C3Vector &unitVector) {
  float directDist = plane.DistSigned(point);
  float cosTheta = NTempest::C3Vector::Dot(plane.n, unitVector);
  FATALASSERT(NTempest::CMath::fnotequal_(cosTheta, 0.0f));
  return directDist / cosTheta;
}

static void LogHitInfoFlags(unsigned int flags) {
    // TODO: implement
}

void CMovement::Redirect(
    unsigned long       timeStamp,
    NTempest::C3Vector &unitMoveVector,
    NTempest::C3Vector &platformNorm,
    CRedirect          &hitInfoX,
    CRedirect          &hitInfoY
) {
  if (!hitInfoX.flags && !hitInfoY.flags) {
    return;
  }

  unsigned int typeX = hitInfoX.flags & 0x1F;
  unsigned int typeY = hitInfoY.flags & 0x1F;
  int          corner =
      (((typeX == 8 || typeX == 16) && !typeY) || ((typeY == 8 || typeY == 16) && !typeX) || (typeX == 4 && !typeY) || (typeY == 4 && !typeX) ||
       (typeX == 4 && typeY == 4));
  if (corner) {
    NTempest::C3Vector *normal;
    if (!hitInfoX.flags) {
      normal = &hitInfoY.surfaceNorm[0];
    } else if (
        !hitInfoY.flags ||
        NTempest::C3Vector::Dot(hitInfoX.surfaceNorm[0], unitMoveVector) < NTempest::C3Vector::Dot(hitInfoY.surfaceNorm[0], unitMoveVector)
    )
    {
      normal = &hitInfoX.surfaceNorm[0];
    } else {
      normal = &hitInfoY.surfaceNorm[0];
    }
    Obstruct(timeStamp, unitMoveVector, platformNorm, *normal);
  } else if ((hitInfoX.flags & 0x1B) && (hitInfoY.flags & 0x1B)) {
    Halt(timeStamp);
  } else {
    NTempest::C3Vector normal(hitInfoX.flags & 0x1B ? 0.0f : 1.0f, hitInfoX.flags & 0x1B ? 1.0f : 0.0f, 0.0f);
    if (NTempest::C3Vector::Dot(normal, unitMoveVector) < 0.0f) {
      normal = normal * -1.0f;
    }
    AttemptRedirect(timeStamp, unitMoveVector, normal);
  }
}

void CMovement::Redirect(unsigned long timeStamp, NTempest::C3Vector &unitMoveVector, NTempest::C3Vector &platformNorm, CRedirect &hitInfo) {
  unsigned int hitType = hitInfo.flags & 0x1F;
  if (!hitInfo.flags) {
    return;
  }
  if (hitType == 4 || hitType == 8 || hitType == 16) {
    Obstruct(timeStamp, unitMoveVector, platformNorm, hitInfo.surfaceNorm[0]);
  } else {
    Halt(timeStamp);
  }
}

void CMovement::AttemptRedirect(unsigned long timeStamp, NTempest::C3Vector &unitMove, NTempest::C3Vector &newDirection) {
  NTempest::C2Vector newDirection2d(newDirection.x, newDirection.y);
  float              length = static_cast<float>(sqrt(newDirection2d.x * newDirection2d.x + newDirection2d.y * newDirection2d.y));
  if (NTempest::CMath::fabs_(length) >= 0.00000023841858f) {
    newDirection2d.x /= length;
    newDirection2d.y /= length;
  }

  if (m_moveFlags & 0x1000) {
    if (NTempest::CMath::fabs_(m_reDirection.x - newDirection2d.x) < 0.00000095367432f &&
        NTempest::CMath::fabs_(m_reDirection.y - newDirection2d.y) < 0.00000095367432f)
    {
      return;
    }

    float oldCross = unitMove.y * m_reDirection.x - unitMove.x * m_reDirection.y;
    float newCross = unitMove.y * newDirection2d.x - unitMove.x * newDirection2d.y;
    float oldDot = unitMove.x * m_reDirection.x + unitMove.y * m_reDirection.y;
    float newDot = unitMove.x * newDirection2d.x + unitMove.y * newDirection2d.y;
    if ((oldCross < 0.0f) != (newCross < 0.0f) && newDot - oldDot < 0.00000095367432f) {
      m_reDirection.x = newDirection2d.x;
      m_reDirection.y = newDirection2d.y;
      m_reDirection.z = 0.0f;
    } else {
      Halt(timeStamp);
    }
  } else {
    m_reDirection.x = newDirection2d.x;
    m_reDirection.y = newDirection2d.y;
    m_reDirection.z = 0.0f;
    m_moveFlags |= 0x1000;
  }
}

void CMovement::Obstruct(unsigned long timeStamp, NTempest::C3Vector &unitMove, NTempest::C3Vector &platformNorm, NTempest::C3Vector &facetNormHit) {
  float normalLength = facetNormHit.Mag();
  if (normalLength < 0.00000095367432f || NTempest::C3Vector::Dot(facetNormHit, unitMove) > -0.0013888889f || !(m_moveFlags & 0xF)) {
    Halt(timeStamp);
    return;
  }

  NTempest::C3Vector planeIntersect = NTempest::C3Vector::Cross(platformNorm, facetNormHit);
  float              length = planeIntersect.Mag();
  if (NTempest::CMath::fabs_(length) >= 0.00000023841858f) {
    planeIntersect.x /= length;
    planeIntersect.y /= length;
    planeIntersect.z /= length;
  }
  float cosTheta = NTempest::C3Vector::Dot(planeIntersect, unitMove);
  if (cosTheta < 0.0f) {
    planeIntersect = planeIntersect * -1.0f;
    cosTheta = -cosTheta;
  }
  if (planeIntersect.SquaredMag() < 0.00000095367432f || (NTempest::CMath::fabs_(cosTheta) < 0.017452406f && (m_moveFlags & 0x4000))) {
    Halt(timeStamp);
    return;
  }
  AttemptRedirect(timeStamp, unitMove, planeIntersect);
}

void CMovement::CallMoveEventHandlers(unsigned long eventTime, int moveAdjusted, unsigned int oldMoveFlags, int wasJumping) {
  if (m_spline && !(m_spline->flags & 4) && (m_moveFlags & 3) && (m_spline->flags & 1)) {
    ForceStopMove(eventTime);
    if (m_spline->flags & 0x40000) {
      m_facing = m_spline->face.facing;
    } else if (m_spline->flags & 0x20000) {
      NTempest::C3Vector spot;
      if (UnitGetObjectPosition(m_spline->face.guid, &spot)) {
        m_facing = UnitCalculateFacingTo(m_position, spot);
      }
    } else if (m_spline->flags & 0x10000) {
      m_facing = UnitCalculateFacingTo(m_position, m_spline->face.spot);
    }
    m_spline->flags |= 4;
  }

  if (((oldMoveFlags ^ m_moveFlags) & 0xF) != 0) {
    OnPendingMoveStateChange(m_guid, GetMoveEventMsgId(oldMoveFlags, wasJumping), eventTime);
    if (!(m_moveFlags & 0xF)) {
      UnitNotifyStopped(m_guid, m_spline && (m_spline->flags & 1));
    }
  } else if (m_moveFlags & 0x10000000) {
    if (!(oldMoveFlags & 0x10000000)) {
      OnCollideStuck(m_guid, eventTime);
    }
  } else if (moveAdjusted && (m_moveFlags & 0xF)) {
    OnCollideRedirected(m_guid, eventTime);
  }

  if (m_moveFlags & 0x4000) {
    if ((m_moveFlags & 0x8000) && !(oldMoveFlags & 0x8000)) {
      OnCollideFalling(m_guid, eventTime);
    }
  } else if (oldMoveFlags & 0x4000) {
    OnCollideFallLand(m_guid, eventTime);
  }
}

float CMovement::RelDistanceFallen(unsigned long currentTime, float updateFallTimeSecs) {
  float lastFallTime = 0.0f;
  float fallStartElevation = m_position.z;
  if ((m_moveFlags & 0x4000) && currentTime != m_fallStartTime) {
    lastFallTime = static_cast<float>(currentTime - m_fallStartTime) * 0.001f;
    FATALASSERT(lastFallTime >= 0.0f);
    fallStartElevation = m_fallStartElevation;
  }

  float fallTime = lastFallTime + updateFallTimeSecs;
  float velocity = s_gravityRate * fallTime;
  float distance;
  if (velocity + m_jumpVelocity <= s_terminalVelocity) {
    distance = (m_jumpVelocity + velocity * 0.5f) * fallTime;
  } else {
    float terminalTime = (s_terminalVelocity - m_jumpVelocity) / s_gravityRate;
    distance = (fallTime - terminalTime + terminalTime * 0.5f) * s_terminalVelocity;
  }
  return m_position.z - (fallStartElevation - distance);
}

float CMovement::RelDistanceFallen(unsigned int fallTimeMS) {
  float fallTime = static_cast<float>(fallTimeMS) * 0.001f;
  float velocity = s_gravityRate * fallTime;
  float distance;
  if (velocity + m_jumpVelocity <= s_terminalVelocity) {
    distance = (m_jumpVelocity + velocity * 0.5f) * fallTime;
  } else {
    float terminalTime = (s_terminalVelocity - m_jumpVelocity) / s_gravityRate;
    distance = (fallTime - terminalTime + terminalTime * 0.5f) * s_terminalVelocity;
  }
  float startElevation = m_moveFlags & 0x4000 ? m_fallStartElevation : m_position.z;
  return m_position.z - (startElevation - distance);
}

int CMovement::IsJumpingUp(unsigned long eventTime) {
  if (m_jumpVelocity == 0.0f) {
    return 0;
  }
  float velocity = static_cast<float>(eventTime - m_fallStartTime) * 0.001f * s_gravityRate + m_jumpVelocity;
  if (velocity > s_terminalVelocity) {
    velocity = s_terminalVelocity;
  }
  return velocity < 0.0f;
}

int CMovement::FallFromTransport() {
  if (!transportLink.m_next) {
    return 0;
  }
  FATALASSERT(m_transportGUID);
  MovementGetTransportVector(m_transportGUID);
  return SetTransport(0) != 0;
}

void CMovement::StartFalling(unsigned long eventTime) {
  FallFromTransport();
  m_fallStartElevation = m_position.z;
  m_moveFlags |= 0x4000;
  m_fallStartTime = eventTime;
}

void CMovement::StopFalling() {
  m_moveFlags = (m_moveFlags & 0xF6FF2FFF) | 0x08000000;
  m_jumpVelocity = 0.0f;
  CalcDirection();
}

void CMovement::ProcessFallReset(unsigned long eventTime) {
  m_jumpVelocity = 0.0f;
  m_fallStartTime = eventTime;
  m_fallStartElevation = m_position.z;
}

void CMovement::CheckFallenFar(unsigned long eventTime) {
  if (m_moveFlags & 0x8000) {
    return;
  }
  if (m_jumpVelocity == 0.0f) {
    if (eventTime - m_fallStartTime < 500) {
      return;
    }
  } else if (m_fallStartElevation - 0.11111111f < m_position.z) {
    return;
  }
  m_moveFlags |= 0x8000;
}

void CMovement::ProcessFalling(unsigned long eventTime) {
  if (!(m_moveFlags & 0x4000)) {
    StartFalling(eventTime);
  }
  CheckFallenFar(eventTime);
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

void CMovement::ExtrudeDownNegXFacet(float distance, NTempest::C4Plane *sides, NTempest::C4Plane *startPlane) {
  startPlane->Set(0.87964189f, 0.0f, 0.4756366f, -m_position.z * 0.4756366f - m_position.x * 0.87964189f);
  sides[0].Set(-0.87964189f, 0.0f, -0.4756366f, m_position.x * 0.87964189f + (m_position.z - distance) * 0.4756366f);
  sides[1].Set(0.0f, 0.0f, 1.0f, -m_position.z - m_stepUpHeight);
  sides[2].Set(-1.0f, 0.0f, 0.0f, m_position.x - m_collisionBoxHalfDepth);
  sides[3].Set(0.70710677f, 0.70710677f, 0.0f, -(m_position.x + m_position.y) * 0.70710677f);
  sides[4].Set(0.70710677f, -0.70710677f, 0.0f, (m_position.y - m_position.x) * 0.70710677f);
}

void CMovement::ExtrudeDownPosXFacet(float distance, NTempest::C4Plane *sides, NTempest::C4Plane *startPlane) {
  startPlane->Set(-0.87964189f, 0.0f, 0.4756366f, -m_position.z * 0.4756366f + m_position.x * 0.87964189f);
  sides[0].Set(0.87964189f, 0.0f, -0.4756366f, -m_position.x * 0.87964189f + (m_position.z - distance) * 0.4756366f);
  sides[1].Set(0.0f, 0.0f, 1.0f, -m_position.z - m_stepUpHeight);
  sides[2].Set(1.0f, 0.0f, 0.0f, -m_position.x - m_collisionBoxHalfDepth);
  sides[3].Set(-0.70710677f, 0.70710677f, 0.0f, (m_position.x - m_position.y) * 0.70710677f);
  sides[4].Set(-0.70710677f, -0.70710677f, 0.0f, (m_position.x + m_position.y) * 0.70710677f);
}

void CMovement::ExtrudeDownNegYFacet(float distance, NTempest::C4Plane *sides, NTempest::C4Plane *startPlane) {
  startPlane->Set(0.0f, 0.87964189f, 0.4756366f, -m_position.y * 0.87964189f - m_position.z * 0.4756366f);
  sides[0].Set(0.0f, -0.87964189f, -0.4756366f, m_position.y * 0.87964189f + (m_position.z - distance) * 0.4756366f);
  sides[1].Set(0.0f, 0.0f, 1.0f, -m_position.z - m_stepUpHeight);
  sides[2].Set(0.0f, -1.0f, 0.0f, m_position.y - m_collisionBoxHalfDepth);
  sides[3].Set(0.70710677f, 0.70710677f, 0.0f, -(m_position.x + m_position.y) * 0.70710677f);
  sides[4].Set(-0.70710677f, 0.70710677f, 0.0f, (m_position.x - m_position.y) * 0.70710677f);
}

void CMovement::ExtrudeDownPosYFacet(float distance, NTempest::C4Plane *sides, NTempest::C4Plane *startPlane) {
  startPlane->Set(0.0f, -0.87964189f, 0.4756366f, m_position.y * 0.87964189f - m_position.z * 0.4756366f);
  sides[0].Set(0.0f, 0.87964189f, -0.4756366f, -m_position.y * 0.87964189f + (m_position.z - distance) * 0.4756366f);
  sides[1].Set(0.0f, 0.0f, 1.0f, -m_position.z - m_stepUpHeight);
  sides[2].Set(0.0f, 1.0f, 0.0f, -m_position.y - m_collisionBoxHalfDepth);
  sides[3].Set(0.70710677f, -0.70710677f, 0.0f, (m_position.y - m_position.x) * 0.70710677f);
  sides[4].Set(-0.70710677f, -0.70710677f, 0.0f, (m_position.x + m_position.y) * 0.70710677f);
}

float CMovement::FindCeilingDistanceAbove(float distanceToJump) {
  NTempest::C3Vector unitMove(0.0f, 0.0f, 1.0f);
  NTempest::C4Plane  startPlane;
  startPlane.Set(0.0f, 0.0f, 1.0f, -m_position.z - m_collisionBoxHeight);

  NTempest::C4Plane boxSides[6];
  boxSides[0].Set(0.0f, 0.0f, -1.0f, m_position.z + m_stepUpHeight);
  boxSides[1].Set(0.0f, 0.0f, 1.0f, -m_position.z - m_collisionBoxHeight - distanceToJump);
  boxSides[2].Set(0.0f, 1.0f, 0.0f, m_position.y - m_collisionBoxHalfDepth);
  boxSides[3].Set(0.0f, -1.0f, 0.0f, -m_position.y - m_collisionBoxHalfDepth);
  boxSides[4].Set(1.0f, 0.0f, 0.0f, -m_position.x - m_collisionBoxHalfDepth);
  boxSides[5].Set(-1.0f, 0.0f, 0.0f, m_position.x - m_collisionBoxHalfDepth);

  float distanceJumped = distanceToJump;
  FindObstacles(unitMove, boxSides, 6, startPlane, 0, &distanceJumped, 0);
  return distanceJumped > 0.0f ? distanceJumped : 0.0f;
}

void CMovement::SetOrientation() {
  NTempest::C4Plane boxSides[6];
  boxSides[0].Set(1.0f, 0.0f, 0.0f, -m_position.x - m_collisionBoxHalfDepth);
  boxSides[1].Set(-1.0f, 0.0f, 0.0f, m_position.x - m_collisionBoxHalfDepth);
  boxSides[2].Set(0.0f, 1.0f, 0.0f, -m_position.y - m_collisionBoxHalfDepth);
  boxSides[3].Set(0.0f, -1.0f, 0.0f, m_position.y - m_collisionBoxHalfDepth);
  boxSides[4].Set(0.0f, 0.0f, 1.0f, -m_position.z - m_collisionBoxHeight);
  boxSides[5].Set(0.0f, 0.0f, -1.0f, m_position.z);
  m_groundNormal = CalcAverageSurfaceNormal(boxSides);
}

static int AddNormal(NTempest::C3Vector &normal, unsigned int maxNormals, NTempest::C3Vector *normalList, unsigned int *numNormals) {
  if (*numNormals == maxNormals) {
    return 0;
  }
  for (unsigned int i = 0; i < *numNormals; ++i) {
    if (NTempest::C3Vector::Dot(normalList[i], normal) > 0.99984771f) {
      return 1;
    }
  }
  normalList[(*numNormals)++] = normal;
  return 1;
}

static int GetSlidingDirection(unsigned __int64 guid, NTempest::C3Vector *normalList, unsigned int numNormals, NTempest::C3Vector *direction) {
  CMovement::LogWrite("0x%016I64X: Getting slide direction from %u normals\n", guid, numNormals);
  direction->Set(0.0f, 0.0f, 0.0f);
  if (numNormals == 1) {
    NTempest::C2Vector downCrossSurfaceNorm(-normalList[0].y, normalList[0].x);
    float magnitude = static_cast<float>(sqrt(downCrossSurfaceNorm.x * downCrossSurfaceNorm.x + downCrossSurfaceNorm.y * downCrossSurfaceNorm.y));
    if (NTempest::CMath::fabs_(magnitude) >= 0.00000023841858f) {
      downCrossSurfaceNorm.x /= magnitude;
      downCrossSurfaceNorm.y /= magnitude;
    }
    NTempest::C3Vector down(downCrossSurfaceNorm.x, downCrossSurfaceNorm.y, 0.0f);
    *direction = NTempest::C3Vector::Cross(down, normalList[0]);
  } else if (numNormals == 2) {
    *direction = NTempest::C3Vector::Cross(normalList[0], normalList[1]);
    if (NTempest::CMath::fabs_(direction->Mag()) >= 0.00000023841858f) {
      direction->Normalize();
    }
  } else if (numNormals == 3) {
    NTempest::C3Vector intermed1 = NTempest::C3Vector::Cross(normalList[0], normalList[1]);
    if (NTempest::CMath::fabs_(intermed1.Mag()) >= 0.00000023841858f) {
      intermed1.Normalize();
    }
    NTempest::C3Vector intermed2 = NTempest::C3Vector::Cross(normalList[0], normalList[2]);
    if (NTempest::CMath::fabs_(intermed2.Mag()) >= 0.00000023841858f) {
      intermed2.Normalize();
    }
    *direction = NTempest::C3Vector::Cross(intermed1, intermed2);
    if (NTempest::CMath::fabs_(direction->Mag()) >= 0.00000023841858f) {
      direction->Normalize();
    }
  } else if (numNormals == 4) {
    NTempest::C3Vector intermed1 = NTempest::C3Vector::Cross(normalList[0], normalList[1]);
    if (NTempest::CMath::fabs_(intermed1.Mag()) >= 0.00000023841858f) {
      intermed1.Normalize();
    }
    NTempest::C3Vector intermed2 = NTempest::C3Vector::Cross(normalList[2], normalList[3]);
    if (NTempest::CMath::fabs_(intermed2.Mag()) >= 0.00000023841858f) {
      intermed2.Normalize();
    }
    *direction = NTempest::C3Vector::Cross(intermed1, intermed2);
    if (NTempest::CMath::fabs_(direction->Mag()) >= 0.00000023841858f) {
      direction->Normalize();
    }
  } else {
    CMovement::LogWrite("0x%016I64X: TOO MANY NORMALS, failed to get slide direction!!!\n", guid);
  }

  if (NTempest::CMath::fabs_(direction->z) <= 0.76604444f) {
    NTempest::C3Vector minIncline(0.0f, 0.0f, 1.0f);
    float              highestZ = 0.0f;
    for (unsigned int i = 0; i < numNormals; ++i) {
      if (normalList[i].z > highestZ) {
        minIncline.x = normalList[i].x;
        minIncline.y = normalList[i].y;
        highestZ = normalList[i].z;
      }
    }
    NTempest::C2Vector downCrossSurfaceNorm(-minIncline.y, minIncline.x);
    float magnitude = static_cast<float>(sqrt(downCrossSurfaceNorm.x * downCrossSurfaceNorm.x + downCrossSurfaceNorm.y * downCrossSurfaceNorm.y));
    if (NTempest::CMath::fabs_(magnitude) >= 0.00000023841858f) {
      downCrossSurfaceNorm.x /= magnitude;
      downCrossSurfaceNorm.y /= magnitude;
    }
    NTempest::C3Vector down(downCrossSurfaceNorm.x, downCrossSurfaceNorm.y, 0.0f);
    *direction = NTempest::C3Vector::Cross(down, minIncline);
  }

  if (direction->z > 0.0f) {
    *direction = -*direction;
  }
  CMovement::LogWrite("0x%016I64X: New slide dir (%g,%g,%g)\n", guid, direction->x, direction->y, direction->z);
  return NTempest::CMath::fabs_(direction->z) > 0.76604444f;
}

float CMovement::FindGroundDistanceBelow(float distanceToFall, unsigned __int64 *gameObjHit) {
  NTempest::C3Vector unitMove(0.0f, 0.0f, -1.0f);
  NTempest::C3Vector surfNormals[4];
  unsigned int       numHits = 0;
  float              shortestDist = distanceToFall;
  int                foundWalkable = 0;
  *gameObjHit = 0;

  for (unsigned int side = 0; side < 4; ++side) {
    NTempest::C4Plane boxPlanes[5];
    NTempest::C4Plane startPlane;
    if (side == 0) {
      ExtrudeDownNegXFacet(distanceToFall, boxPlanes, &startPlane);
    } else if (side == 1) {
      ExtrudeDownPosXFacet(distanceToFall, boxPlanes, &startPlane);
    } else if (side == 2) {
      ExtrudeDownNegYFacet(distanceToFall, boxPlanes, &startPlane);
    } else {
      ExtrudeDownPosYFacet(distanceToFall, boxPlanes, &startPlane);
    }

    float     distanceFallen = distanceToFall;
    CRedirect hitInfo;
    FindObstacles(unitMove, boxPlanes, 5, startPlane, 1, &distanceFallen, &hitInfo);
    if (!hitInfo.flags) {
      continue;
    }

    if (NTempest::CMath::fabs_(shortestDist - distanceFallen) < 0.013888889f) {
      if (distanceFallen < shortestDist) {
        shortestDist = distanceFallen;
      }
      AddNormal(hitInfo.surfaceNorm[0], 4, surfNormals, &numHits);
      if (hitInfo.flags & 0x80) {
        AddNormal(hitInfo.surfaceNorm[1], 4, surfNormals, &numHits);
      }
      foundWalkable |= hitInfo.surfaceNorm[0].z > 0.64278764f;
      *gameObjHit = hitInfo.gameObjHit;
    } else if (distanceFallen < shortestDist) {
      shortestDist = distanceFallen;
      numHits = 0;
      AddNormal(hitInfo.surfaceNorm[0], 4, surfNormals, &numHits);
      if (hitInfo.flags & 0x80) {
        AddNormal(hitInfo.surfaceNorm[1], 4, surfNormals, &numHits);
      }
      foundWalkable = hitInfo.surfaceNorm[0].z > 0.64278764f;
      *gameObjHit = hitInfo.gameObjHit;
    }
  }

  if (!foundWalkable && NTempest::CMath::fabs_(distanceToFall - shortestDist) >= 0.0013888889f) {
    FATALASSERT(numHits > 0);
    if (GetSlidingDirection(m_guid, surfNormals, numHits, &m_reDirection)) {
      m_moveFlags |= 0x01000000;
    } else {
      m_moveFlags &= ~0x01000000U;
    }
  }
  return shortestDist > 0.0f ? shortestDist : 0.0f;
}

float CMovement::CollideWithWaterSurface(NTempest::C3Vector &unitMove, NTempest::C3Vector &unitMoveWanted, float distanceWanted) {
  float verticalMove = distanceWanted * unitMove.z;
  if (NTempest::CMath::fabs_(verticalMove) < 0.00000095367432f || m_position.z + verticalMove < m_waterSurfaceElev) {
    m_moveFlags &= ~0x1000U;
    return distanceWanted;
  }

  m_reDirection.Set(unitMoveWanted.x, unitMoveWanted.y, 0.0f);
  float magnitude = m_reDirection.Mag();
  if (NTempest::CMath::fabs_(magnitude) >= 0.00000023841858f) {
    m_reDirection *= 1.0f / magnitude;
  }
  m_moveFlags |= 0x1000;
  return (m_waterSurfaceElev - m_position.z) / verticalMove * distanceWanted;
}

float CMovement::ExtrudeFlyBoxUp(NTempest::C3Vector &unitMove, NTempest::C3Vector &unitMoveWanted, float distanceWanted) {
  NTempest::C4Plane  boxPlanes[5][6];
  CRedirect          hitInfoZ;
  CRedirect          hitInfoY;
  CRedirect          hitInfoX;
  NTempest::C4Plane  startPlanes[3];
  NTempest::C3Vector moveVector = unitMove * distanceWanted;
  float              pyramidHgt = m_collisionBoxHalfDepth * 1.849399f;
  ExtrudeBoxSideX(moveVector, pyramidHgt, boxPlanes[0]);
  ExtrudeBoxSideY(moveVector, pyramidHgt, boxPlanes[1]);
  ExtrudeBoxSideZ(moveVector, pyramidHgt, boxPlanes[2]);
  int pyrSideX = ExtrudePyramidSideX(unitMove, distanceWanted, boxPlanes[3]);
  int pyrSideY = ExtrudePyramidSideY(unitMove, distanceWanted, boxPlanes[4]);

  float distanceX = FLT_MAX;
  float distanceY = FLT_MAX;
  float distanceZ = FLT_MAX;
  for (int side = 0; side < 3; ++side) {
    startPlanes[side] = boxPlanes[side][0];
    startPlanes[side].d += 0.027777778f;
  }
  FindObstacles(unitMove, boxPlanes[0], 6, startPlanes[0], 0, &distanceX, &hitInfoX);
  FindObstacles(unitMove, boxPlanes[1], 6, startPlanes[1], 0, &distanceY, &hitInfoY);
  FindObstacles(unitMove, boxPlanes[2], 6, startPlanes[2], 0, &distanceZ, &hitInfoZ);
  if (pyrSideX) {
    FindObstacles(unitMove, boxPlanes[3], 5, boxPlanes[3][0], 1, &distanceX, &hitInfoX);
  }
  if (pyrSideY) {
    FindObstacles(unitMove, boxPlanes[4], 5, boxPlanes[4][0], 1, &distanceY, &hitInfoY);
  }

  float distance = distanceX;
  if (distanceY < distance) {
    distance = distanceY;
  }
  if (distanceZ < distance) {
    distance = distanceZ;
  }
  if (NTempest::CMath::fabs_(distanceX - distance) >= 0.0013888889f) {
    hitInfoX.Reset();
  }
  if (NTempest::CMath::fabs_(distanceY - distance) >= 0.0013888889f) {
    hitInfoY.Reset();
  }
  if (NTempest::CMath::fabs_(distanceZ - distance) >= 0.0013888889f) {
    hitInfoZ.Reset();
  }
  if (((m_moveFlags & 0x1000) && NTempest::CMath::fabs_(m_direction.z) >= 0.00000023841858f) || hitInfoX.flags || hitInfoY.flags || hitInfoZ.flags) {
    FlyRedirect(unitMoveWanted, hitInfoX, hitInfoY, hitInfoZ);
  } else {
    distance = CollideWithWaterSurface(unitMove, unitMoveWanted, distanceWanted);
  }
  return distance > 0.0f ? distance : 0.0f;
}

float CMovement::ExtrudeProjectileBoxUpHill(NTempest::C3Vector &unitMove, float distanceWanted, unsigned __int64 *gameObjHit) {
  NTempest::C4Plane  boxPlanes[5][6];
  CRedirect          hitInfo;
  NTempest::C4Plane  startPlanes[3];
  NTempest::C3Vector moveVector = unitMove * distanceWanted;
  float              pyramidHgt = m_collisionBoxHalfDepth * 1.849399f;
  ExtrudeBoxSideX(moveVector, pyramidHgt, boxPlanes[0]);
  ExtrudeBoxSideY(moveVector, pyramidHgt, boxPlanes[1]);
  ExtrudeBoxSideZ(moveVector, pyramidHgt, boxPlanes[2]);
  int   pyrSideX = ExtrudePyramidSideX(unitMove, distanceWanted, boxPlanes[3]);
  int   pyrSideY = ExtrudePyramidSideY(unitMove, distanceWanted, boxPlanes[4]);
  float distance = FLT_MAX;
  for (int side = 0; side < 3; ++side) {
    startPlanes[side] = boxPlanes[side][0];
    startPlanes[side].d += 0.027777778f;
    FindObstacles(unitMove, boxPlanes[side], 6, startPlanes[side], 0, &distance, &hitInfo);
  }
  if (pyrSideX) {
    FindObstacles(unitMove, boxPlanes[3], 5, boxPlanes[3][0], 1, &distance, &hitInfo);
    if (hitInfo.flags & 0x40) {
      *gameObjHit = hitInfo.gameObjHit;
    }
  }
  if (pyrSideY) {
    FindObstacles(unitMove, boxPlanes[4], 5, boxPlanes[4][0], 1, &distance, &hitInfo);
    if (hitInfo.flags & 0x40) {
      *gameObjHit = hitInfo.gameObjHit;
    }
  }
  if (distance < distanceWanted && hitInfo.flags && hitInfo.surfaceNorm[0].z > 0.64278764f) {
    StopFalling();
  }
  return distance > 0.0f ? distance : 0.0f;
}

float CMovement::ExtrudeSlideBoxDownHill(NTempest::C3Vector &unitMove, float distanceWanted, CRedirect *hitInfo) {
  NTempest::C4Plane  boxPlanes[4][6];
  NTempest::C4Plane  startPlanes[2];
  NTempest::C3Vector moveVector = unitMove * distanceWanted;
  float              pyramidHgt = m_collisionBoxHalfDepth * 1.849399f;
  ExtrudeBoxSideX(moveVector, pyramidHgt, boxPlanes[0]);
  ExtrudeBoxSideY(moveVector, pyramidHgt, boxPlanes[1]);
  int   pyrSideX = ExtrudePyramidSideX(unitMove, distanceWanted, boxPlanes[2]);
  int   pyrSideY = ExtrudePyramidSideY(unitMove, distanceWanted, boxPlanes[3]);
  float distance = FLT_MAX;
  for (int side = 0; side < 2; ++side) {
    startPlanes[side] = boxPlanes[side][0];
    startPlanes[side].d += 0.027777778f;
    FindObstacles(unitMove, boxPlanes[side], 6, startPlanes[side], 0, &distance, hitInfo);
  }
  if (pyrSideX) {
    FindObstacles(unitMove, boxPlanes[2], 5, boxPlanes[2][0], 1, &distance, hitInfo);
  }
  if (pyrSideY) {
    FindObstacles(unitMove, boxPlanes[3], 5, boxPlanes[3][0], 1, &distance, hitInfo);
  }
  return distance > 0.0f ? distance : 0.0f;
}

static float GetDist2d(NTempest::C3Vector &unitMove, float closestDist3D) {
  float horizontal = static_cast<float>(sqrt(unitMove.x * unitMove.x + unitMove.y * unitMove.y));
  return horizontal * closestDist3D;
}

static NTempest::C3Vector ComputeFlyRedirection(NTempest::C3Vector *normals, unsigned int numNormals, NTempest::C3Vector &unitMoveWanted) {
  NTempest::C3Vector finalDirection(0.0f);
  float              mostObtuse = 1.0f;
  for (unsigned int i = 0; i < numNormals; ++i) {
    NTempest::C3Vector orthogonal = NTempest::C3Vector::Cross(unitMoveWanted, normals[i]);
    if (NTempest::CMath::fabs_(orthogonal.x) < 0.00000095367432f && NTempest::CMath::fabs_(orthogonal.y) < 0.00000095367432f &&
        NTempest::CMath::fabs_(orthogonal.z) < 0.00000095367432f)
    {
      continue;
    }
    orthogonal.Normalize();
    NTempest::C3Vector newDirection = NTempest::C3Vector::Cross(normals[i], orthogonal);
    if (NTempest::C3Vector::Dot(newDirection, unitMoveWanted) < 0.0f) {
      newDirection = -newDirection;
    }
    float cosTheta = NTempest::C3Vector::Dot(newDirection, unitMoveWanted);
    if (cosTheta < mostObtuse) {
      mostObtuse = cosTheta;
      finalDirection = newDirection;
    }
  }
  return finalDirection;
}

void CMovement::FlyRedirect(NTempest::C3Vector &unitMoveWanted, CRedirect &hitInfoX, CRedirect &hitInfoY, CRedirect &hitInfoZ) {
  if (!hitInfoX.flags && !hitInfoY.flags && !hitInfoZ.flags) {
    return;
  }

  NTempest::C3Vector normals[6];
  unsigned int       numNormals = 0;
  if (hitInfoX.flags) {
    if (hitInfoX.flags & 3) {
      NTempest::C3Vector normal(unitMoveWanted.x < 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f);
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
      NTempest::C3Vector normal(0.0f, unitMoveWanted.y < 0.0f ? 1.0f : -1.0f, 0.0f);
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
      NTempest::C3Vector normal(0.0f, 0.0f, -1.0f);
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
  if (NTempest::CMath::fabs_(m_reDirection.SquaredMag()) >= 0.00000095367432f) {
    m_moveFlags |= 0x1000;
  }
}

void CMovement::FlyRedirect(NTempest::C3Vector &unitMoveWanted, CRedirect &hitInfoX, CRedirect &hitInfoY) {
  if (!hitInfoX.flags && !hitInfoY.flags) {
    return;
  }

  NTempest::C3Vector normals[4];
  unsigned int       numNormals = 0;
  if (hitInfoX.flags) {
    if (hitInfoX.flags & 3) {
      NTempest::C3Vector normal(unitMoveWanted.x < 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f);
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
      NTempest::C3Vector normal(0.0f, unitMoveWanted.y < 0.0f ? 1.0f : -1.0f, 0.0f);
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
  if (NTempest::CMath::fabs_(m_reDirection.SquaredMag()) >= 0.00000095367432f) {
    m_moveFlags |= 0x1000;
  }
}

float CMovement::ExtrudeFlyBoxDown(NTempest::C3Vector &unitMove, NTempest::C3Vector &unitMoveWanted, float distanceWanted) {
  NTempest::C4Plane  boxPlanes[4][6];
  CRedirect          hitInfoY;
  CRedirect          hitInfoX;
  NTempest::C4Plane  startPlanes[2];
  NTempest::C3Vector moveVector = unitMove * distanceWanted;
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
  FindObstacles(unitMove, boxPlanes[0], 6, startPlanes[0], 0, &distanceX, &hitInfoX);
  FindObstacles(unitMove, boxPlanes[1], 6, startPlanes[1], 0, &distanceY, &hitInfoY);
  if (pyrSideX) {
    FindObstacles(unitMove, boxPlanes[2], 5, boxPlanes[2][0], 1, &distanceX, &hitInfoX);
  }
  if (pyrSideY) {
    FindObstacles(unitMove, boxPlanes[3], 5, boxPlanes[3][0], 1, &distanceY, &hitInfoY);
  }
  float distance = distanceX < distanceY ? distanceX : distanceY;
  if (NTempest::CMath::fabs_(distanceX - distanceY) >= 0.0013888889f) {
    if (distanceX < distanceY) {
      hitInfoY.Reset();
    } else {
      hitInfoX.Reset();
    }
  }
  FlyRedirect(unitMoveWanted, hitInfoX, hitInfoY);
  return distance > 0.0f ? distance : 0.0f;
}

float CMovement::ExtrudeProjectileBoxDownHill(
    unsigned long       timeStamp,
    NTempest::C3Vector &unitMove,
    float               distanceWanted,
    NTempest::C2Vector &unitMoveWanted,
    unsigned __int64   *gameObjHit
) {
  NTempest::C4Plane  boxPlanes[4][6];
  CRedirect          hitInfoY;
  CRedirect          hitInfoX;
  NTempest::C4Plane  startPlanes[2];
  NTempest::C3Vector moveVector = unitMove * distanceWanted;
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
  FindObstacles(unitMove, boxPlanes[0], 6, startPlanes[0], 0, &distanceX, &hitInfoX);
  FindObstacles(unitMove, boxPlanes[1], 6, startPlanes[1], 0, &distanceY, &hitInfoY);
  hitInfoX.gameObjHit = 0;
  hitInfoY.gameObjHit = 0;
  if (pyrSideX) {
    FindObstacles(unitMove, boxPlanes[2], 5, boxPlanes[2][0], 1, &distanceX, &hitInfoX);
    if (hitInfoX.gameObjHit) {
      *gameObjHit = hitInfoX.gameObjHit;
    }
  }
  if (pyrSideY) {
    FindObstacles(unitMove, boxPlanes[3], 5, boxPlanes[3][0], 1, &distanceY, &hitInfoY);
    if (hitInfoY.gameObjHit) {
      *gameObjHit = hitInfoY.gameObjHit;
    }
  }

  float distance;
  if (distanceX < distanceY) {
    distance = distanceX;
    if (NTempest::CMath::fabs_(distanceX - distanceY) >= 0.0013888889f) {
      hitInfoY.Reset();
    }
  } else {
    distance = distanceY;
    if (NTempest::CMath::fabs_(distanceX - distanceY) >= 0.0013888889f) {
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

  NTempest::C3Vector platformNorm(-unitMove.x * unitMove.z, -unitMove.y * unitMove.z, unitMove.x * unitMove.x + unitMove.y * unitMove.y);
  if (platformNorm.z < 0.0f) {
    platformNorm = -platformNorm;
  }
  NTempest::C3Vector unitMoveWanted3d(unitMoveWanted.x, unitMoveWanted.y, 0.0f);
  Redirect(timeStamp, unitMoveWanted3d, platformNorm, hitInfoX, hitInfoY);
  return distance;
}

unsigned int CMovement::Slide(unsigned int fallenSoFar, unsigned int timeIncrement) {
  if (!timeIncrement || (m_transportGUID && FallFromTransport())) {
    return 0;
  }

  float distToFall = RelDistanceFallen(fallenSoFar + timeIncrement);
  LogWrite(
      "0x%016I64X: ====| Starting new slide: fallenSoFar (%u) increment (%u) distance to fall (%g)\n", m_guid, fallenSoFar, timeIncrement, distToFall
  );
  if (NTempest::CMath::fabs_(distToFall) < 0.00000023841858f) {
    return 0;
  }

  float              scale = 1.0f / -m_reDirection.z;
  NTempest::C3Vector unitMove(m_reDirection.x * scale, m_reDirection.y * scale, m_reDirection.z * scale);
  NTempest::C3Vector moveWanted = unitMove * distToFall;
  float              distance = moveWanted.Mag();
  FATALASSERT(NTempest::CMath::fnotequal_(distance, 0.0f));

  float distanceSlid = distance;
  if (s_facetData.facets.Count()) {
    CRedirect hitInfo;
    distanceSlid = ExtrudeSlideBoxDownHill(unitMove, distance, &hitInfo);
    if (distanceSlid < distance) {
      m_moveFlags &= ~0x01000000U;
      m_position += unitMove * distanceSlid;
      float timeUsed = distanceSlid / distance * (static_cast<float>(timeIncrement) * 0.001f) * 1000.0f;
      if (timeUsed <= 0.0f) {
        timeIncrement = static_cast<unsigned int>(-static_cast<long>(-timeUsed + 0.5f));
      } else {
        timeIncrement = static_cast<unsigned int>(timeUsed + 0.5f);
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

  LogWrite(
      "0x%016I64X: ====| Wanted to slide (%g) (%g,%g,%g), slid (%g) to (%g,%g,%g)\n", m_guid, distance, moveWanted.x, moveWanted.y, moveWanted.z,
      distanceSlid, m_position.x, m_position.y, m_position.z
  );
  SetOrientation();
  return timeIncrement;
}

unsigned int CMovement::Fall(unsigned int fallenSoFar, unsigned int timeIncrement) {
  if (!timeIncrement || (m_transportGUID && FallFromTransport())) {
    return 0;
  }

  unsigned int fallEndTime = fallenSoFar + timeIncrement;
  LogWrite("0x%016I64X: ====| Starting new fall: fallenSoFar (%u) increment (%u)\n", m_guid, fallenSoFar, timeIncrement);
  float distToFall = RelDistanceFallen(fallEndTime);
  float distFallen = distToFall;

  if (s_facetData.facets.Count()) {
    if (distToFall >= 0.0f) {
      unsigned __int64 gameObjHit = 0;
      distFallen = FindGroundDistanceBelow(distToFall, &gameObjHit);
      m_position.z -= distFallen;
      if (NTempest::CMath::fabs_(distToFall - distFallen) < 0.00000095367432f) {
        if (!(m_moveFlags & 0x01000000)) {
          CheckFallenFar(fallEndTime + m_fallStartTime);
        }
      } else {
        float timeUsed = distFallen / distToFall * (static_cast<float>(timeIncrement) * 0.001f) * 1000.0f;
        if (timeUsed <= 0.0f) {
          timeIncrement = static_cast<unsigned int>(-static_cast<long>(-timeUsed + 0.5f));
        } else {
          timeIncrement = static_cast<unsigned int>(timeUsed + 0.5f);
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
      if (NTempest::CMath::fabs_(distToFall - distFallen) < 0.00000095367432f) {
        CheckFallenFar(fallEndTime + m_fallStartTime);
      } else {
        float timeUsed = distFallen / distToFall * (static_cast<float>(timeIncrement) * 0.001f) * 1000.0f;
        if (timeUsed <= 0.0f) {
          timeIncrement = static_cast<unsigned int>(-static_cast<long>(-timeUsed + 0.5f));
        } else {
          timeIncrement = static_cast<unsigned int>(timeUsed + 0.5f);
        }
        ProcessFallReset(fallenSoFar + timeIncrement + m_fallStartTime);
      }
    }
  } else {
    m_position.z -= distToFall;
    CheckFallenFar(fallEndTime + m_fallStartTime);
  }

  LogWrite("0x%016I64X: ====| Wanted to fall (%g), fell (%g) to Z(%g)\n", m_guid, distToFall, distFallen, m_position.z);
  m_groundNormal.Set(0.0f, 0.0f, 1.0f);
  return timeIncrement;
}

void CMovement::GetMoveFacets(float distance, unsigned int timeToFall, NTempest::C3Vector &unitMove) {
  NTempest::C3Vector moveDirection = unitMove;
  if (!(m_moveFlags & 0x02000000)) {
    moveDirection.z = 0.0f;
    if (NTempest::CMath::fabs_(moveDirection.x) >= 0.00000023841858f && NTempest::CMath::fabs_(moveDirection.y) >= 0.00000023841858f) {
      float ooMag = 1.0f / static_cast<float>(sqrt(moveDirection.x * moveDirection.x + moveDirection.y * moveDirection.y));
      moveDirection.x *= ooMag;
      moveDirection.y *= ooMag;
    }
  }

  NTempest::C3Vector  position = m_position;
  NTempest::C3Vector  moveVector = moveDirection * distance;
  NTempest::C34Matrix worldToTransport;
  if (m_transportGUID) {
    NTempest::C34Matrix transportToWorld;
    MovementGetTransportMtx(m_transportGUID, &transportToWorld);
    worldToTransport = transportToWorld.AffineInverse();
    position *= transportToWorld;
  }

  NTempest::C3Vector boxBottom(position.x - m_collisionBoxHalfDepth, position.y - m_collisionBoxHalfDepth, position.z);
  NTempest::C3Vector boxTop(position.x + m_collisionBoxHalfDepth, position.y + m_collisionBoxHalfDepth, position.z + m_collisionBoxHeight);
  NTempest::CAaBox   start(boxBottom, boxTop);
  NTempest::CAaBox   end(boxBottom + moveVector, boxTop + moveVector);
  NTempest::CAaBox   axisAlign(NTempest::C3Vector::Min(start.b, end.b), NTempest::C3Vector::Max(start.t, end.t));

  if (!(m_moveFlags & 0x02000000)) {
    axisAlign.t.z += m_stepUpHeight;
    axisAlign.b.z -= RelDistanceFallen(timeToFall);
  }

  unsigned int queryFlags = (m_guid & 0xF000000000000000ui64) ? 8465 : 273;
  CWorld::GetFacets(axisAlign, &s_facetData, queryFlags);
  CollisionInfoSetFaces(m_guid, s_facetData.facets);

  if (m_transportGUID) {
    for (unsigned int i = 0; i < s_facetData.facets.Count(); ++i) {
      NTempest::CFacet &facet = s_facetData.facets[i];
      facet.vertices[0] *= worldToTransport;
      facet.vertices[1] *= worldToTransport;
      facet.vertices[2] *= worldToTransport;
      NTempest::C3Vector normal(
          worldToTransport.a0 * facet.plane.n.x + worldToTransport.b0 * facet.plane.n.y + worldToTransport.c0 * facet.plane.n.z,
          worldToTransport.a1 * facet.plane.n.x + worldToTransport.b1 * facet.plane.n.y + worldToTransport.c1 * facet.plane.n.z,
          worldToTransport.a2 * facet.plane.n.x + worldToTransport.b2 * facet.plane.n.y + worldToTransport.c2 * facet.plane.n.z
      );
      facet.plane.n = normal;
      facet.plane.d = -NTempest::C3Vector::Dot(normal, facet.vertices[0]);
    }
  }
}

static int __cdecl FacetCompare(const void *elem1, const void *elem2) {
  const CWalkableSurface *left = static_cast<const CWalkableSurface *>(elem1);
  const CWalkableSurface *right = static_cast<const CWalkableSurface *>(elem2);
  if (NTempest::CMath::fabs_(left->closeDist - right->closeDist) >= 0.0013888889f) {
    return left->closeDist > right->closeDist ? 1 : -1;
  }
  if (NTempest::CMath::fabs_(left->firstPtOfContact.z - right->firstPtOfContact.z) >= 0.001f) {
    return left->firstPtOfContact.z < right->firstPtOfContact.z ? 1 : -1;
  }
  if (NTempest::CMath::fabs_(left->farDist - right->farDist) < 0.0013888889f) {
    return left->lastPtOfContact.z < right->lastPtOfContact.z ? 1 : -1;
  }
  if (left->farDist < right->farDist) {
    return s_facetData.facets[right->facetId].plane.SolveForZ(left->lastPtOfContact.x, left->lastPtOfContact.y) > left->lastPtOfContact.z ? 1 : -1;
  }
  return s_facetData.facets[left->facetId].plane.SolveForZ(right->lastPtOfContact.x, right->lastPtOfContact.y) < right->lastPtOfContact.z ? 1 : -1;
}

static void EnqueueFacets(
    NTempest::C4Plane                 &slopeTestPlane,
    NTempest::C2Vector                &position,
    NTempest::C2Vector                &unitMove,
    TSGrowableArray<NTempest::CFacet> &facets,
    TSGrowableArray<CWalkableSurface> *surfacePool
) {
  surfacePool->SetCount(0);
  CClippedTriangle  poly;
  NTempest::C4Plane startPlane(unitMove.x, unitMove.y, 0.0f, -(position.x * unitMove.x + position.y * unitMove.y));
  unsigned int      next;
  startPlane.Set(-startPlane.n.x, -startPlane.n.y, -startPlane.n.z, -startPlane.d);
  unsigned int count = facets.Count();
  for (unsigned int i = 0; i < count; ++i) {
    NTempest::CFacet &facet = facets[i];
    if (facet.plane.n.z < 0.00000095367432f) {
      continue;
    }
    poly.Init(facet.vertices);
    ClipPolygonToPlane(startPlane, &poly);
    unsigned int numClippedVerts = poly.Count();
    FATALASSERT(!numClippedVerts || numClippedVerts > 2);
    float              closestDist = FLT_MAX;
    float              farthestDist = -FLT_MAX;
    NTempest::C3Vector closest;
    NTempest::C3Vector farthest;
    NTempest::C3Vector intersection;
    int                hitTri = 0;
    for (next = 0; next < numClippedVerts; ++next) {
      intersection = poly[(next + 1) % numClippedVerts] - poly[next];
      if (NTempest::CMath::fabs_(intersection.Mag()) < 0.00000095367432f) {
        continue;
      }
      if (NTempest::CMath::fabs_(NTempest::C3Vector::Dot(slopeTestPlane.n, intersection)) < 0.00000095367432f) {
        continue;
      }
      intersection = poly[next] + intersection * (-slopeTestPlane.DistSigned(poly[next]) / NTempest::C3Vector::Dot(slopeTestPlane.n, intersection));
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

    if (!hitTri || NTempest::CMath::fabs_(closestDist - farthestDist) < 0.00000095367432f || farthestDist < 0.00000095367432f) {
      continue;
    }

    float highestZ = -FLT_MAX;
    for (next = 0; next < 3; ++next) {
      intersection = facet.vertices[(next + 1) % 3] - facet.vertices[next];
      if (NTempest::CMath::fabs_(intersection.Mag()) < 0.00000095367432f ||
          NTempest::CMath::fabs_(NTempest::C3Vector::Dot(slopeTestPlane.n, intersection)) < 0.00000095367432f)
      {
        continue;
      }
      intersection = facet.vertices[next] +
                     intersection * (-slopeTestPlane.DistSigned(facet.vertices[next]) / NTempest::C3Vector::Dot(slopeTestPlane.n, intersection));
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

void CMovement::ShowCollisionBox(NTempest::C3Vector &unitMove, unsigned int oldMoveFlags) {
  if (oldMoveFlags & 0x02004000) {
    CollisionInfoSetFallBox(m_position, m_collisionBoxHalfDepth, m_collisionBoxHeight);
    return;
  }

  NTempest::C3Vector boxMin(m_position.x - m_collisionBoxHalfDepth, m_position.y - m_collisionBoxHalfDepth, m_position.z - m_stepUpHeight);
  NTempest::C3Vector boxMax(m_position.x + m_collisionBoxHalfDepth, m_position.y + m_collisionBoxHalfDepth, m_position.z + m_collisionBoxHeight);
  CollisionInfoAddBox(boxMin, boxMax);

  NTempest::C3Vector vectorPosition = m_position;
  vectorPosition.z += (m_collisionBoxHeight + m_stepUpHeight) * 0.5f;
  NTempest::C3Vector &moveDirection = m_moveFlags & 0x1000 ? m_reDirection : unitMove;
  CollisionInfoAddVector(vectorPosition, moveDirection);
}

int CMovement::CollideRequestMove(unsigned long lastUpdateTime, unsigned int timeElapsed, NTempest::C3Vector &moveVector) {
  float distance = moveVector.Mag();
  LogWrite("0x%016I64X: ====| Starting new move: elapsed (%u), requested vector (%g,%g)\n", m_guid, timeElapsed, distance, moveVector.z);
  if (!timeElapsed) {
    return 0;
  }

  float elapsedSecs = static_cast<float>(timeElapsed) * 0.001f;
  float currSpeed = distance / elapsedSecs;
  if (!(m_moveFlags & 0x30)) {
    float expectedSpeed = GetCurrentSpeed();
    LogWrite("0x%016I64X: requested move (%g), should be (%g) for (%u ms)\n", m_guid, distance, expectedSpeed * elapsedSecs, timeElapsed);
  }

  NTempest::C3Vector moveDirWanted;
  if (distance >= 0.00000095367432f) {
    moveDirWanted = moveVector * (1.0f / distance);
  } else {
    moveDirWanted.Set(0.0f, 0.0f, -1.0f);
  }

  unsigned int oldMoveFlags = m_moveFlags;
  unsigned int timeUsed = 0;
  unsigned int zeroMoves = 0;
  int          moveModified = 0;
  CMoveState   state;

  while (timeUsed < timeElapsed) {
    unsigned int timeLeft = timeElapsed - timeUsed;
    float        distanceLeft = static_cast<float>(timeLeft) * 0.001f * currSpeed;
    unsigned int fallenSoFar = 0;
    if (m_moveFlags & 0x4000) {
      fallenSoFar = lastUpdateTime + timeUsed - m_fallStartTime;
    }

    NTempest::C3Vector &unitMove = m_moveFlags & 0x01000000 ? m_reDirection : moveDirWanted;
    unsigned int        moveEndFallTime = fallenSoFar + timeElapsed;
    GetMoveFacets(distanceLeft, moveEndFallTime, unitMove);
    SaveMoveState(&state);

    int          wasRedirected = (m_moveFlags & 0x1000) != 0;
    unsigned int timeJustUsed;
    LogWrite("0x%016I64X: lastUpdateTime (0x%08X), timeUsed (%d)\n", m_guid, lastUpdateTime, timeUsed);

    if (m_moveFlags & 0x02000000) {
      NTempest::C3Vector moveWanted = unitMove * distanceLeft;
      timeJustUsed = Swim(lastUpdateTime + timeUsed, timeLeft, moveWanted, moveDirWanted);
    } else if (m_moveFlags & 0x4000) {
      if (m_moveFlags & 0x01000000) {
        timeJustUsed = Slide(fallenSoFar, timeLeft);
      } else if (m_moveFlags & 0xF) {
        NTempest::C3Vector moveWanted = unitMove * distanceLeft;
        NTempest::C2Vector moveDirWanted2d(moveDirWanted.x, moveDirWanted.y);
        timeJustUsed = ProjectileFall(lastUpdateTime + timeUsed, timeLeft, moveWanted, moveDirWanted2d);
      } else {
        timeJustUsed = Fall(fallenSoFar, timeLeft);
      }
    } else {
      NTempest::C2Vector unitMove2d(unitMove.x, unitMove.y);
      NTempest::C2Vector moveDirWanted2d(moveDirWanted.x, moveDirWanted.y);
      timeJustUsed = TraceSurface(lastUpdateTime + timeUsed, timeLeft, distanceLeft, unitMove2d, moveDirWanted2d);
    }

    LogWrite("0x%016I64X: move consumed (%d) ms\n", m_guid, timeJustUsed);
    if (timeJustUsed + 1 >= timeLeft) {
      if (wasRedirected) {
        m_moveFlags &= ~0x1000U;
        moveModified = 1;
        UpdateAnchors(lastUpdateTime + timeUsed);
      }
      timeUsed += timeJustUsed;
      break;
    }

    if (!wasRedirected) {
      moveModified = 1;
      if (!timeJustUsed) {
        ++zeroMoves;
      }
      if (zeroMoves >= 5) {
        LogWrite("0x%016I64X: unit executed five zero-time moves\n", m_guid);
        StopFalling();
        unsigned long eventTime = lastUpdateTime + timeUsed;
        HandlePendingActions(eventTime);
        Halt(eventTime);
      }
      if (!(m_moveFlags & 0x400F) || (m_moveFlags & 0x10000000)) {
        timeUsed += timeJustUsed;
        break;
      }
      timeUsed += timeJustUsed;
      if (timeUsed > timeElapsed) {
        timeUsed = timeElapsed;
      }
    } else {
      RestoreMoveState(state);
    }

    if (!(m_moveFlags & 0x1000)) {
      continue;
    }

    unsigned int redirectedTimeLeft = timeElapsed - timeUsed;
    float        speedFactor = NTempest::C3Vector::Dot(moveDirWanted, m_reDirection);
    if (speedFactor < 0.0f) {
      speedFactor = 0.0f;
    } else if (speedFactor > 1.0f) {
      speedFactor = 1.0f;
    }
    float redirectedDistance = speedFactor * static_cast<float>(redirectedTimeLeft) * 0.001f * currSpeed;
    GetMoveFacets(redirectedDistance, moveEndFallTime, m_reDirection);

    LogWrite("0x%016I64X: (redir) lastUpdateTime (0x%08X), timeUsed (%d)\n", m_guid, lastUpdateTime, timeUsed);
    unsigned int redirectedTimeUsed = timeJustUsed;
    if (m_moveFlags & 0x02000000) {
      NTempest::C3Vector moveWanted = m_reDirection * redirectedDistance;
      redirectedTimeUsed = Swim(lastUpdateTime + timeUsed, redirectedTimeLeft, moveWanted, moveDirWanted);
    } else if (m_moveFlags & 0x4000) {
      if (m_moveFlags & 0xF) {
        NTempest::C3Vector moveWanted = m_reDirection * redirectedDistance;
        NTempest::C2Vector moveDirWanted2d(moveDirWanted.x, moveDirWanted.y);
        redirectedTimeUsed = ProjectileFall(lastUpdateTime + timeUsed, redirectedTimeLeft, moveWanted, moveDirWanted2d);
        if ((m_moveFlags & 0x4000) && !redirectedTimeUsed) {
          Halt(lastUpdateTime + timeUsed);
        }
      }
    } else {
      NTempest::C2Vector redirectedMove(m_reDirection.x, m_reDirection.y);
      NTempest::C2Vector moveDirWanted2d(moveDirWanted.x, moveDirWanted.y);
      redirectedTimeUsed = TraceSurface(lastUpdateTime + timeUsed, redirectedTimeLeft, redirectedDistance, redirectedMove, moveDirWanted2d);
    }

    LogWrite("0x%016I64X: redir move consumed (%d) ms\n", m_guid, redirectedTimeUsed);
    if (redirectedTimeUsed < redirectedTimeLeft) {
      moveModified = 1;
    }
    if (!redirectedTimeUsed) {
      ++zeroMoves;
    }
    if (zeroMoves >= 5) {
      StopFalling();
      unsigned long eventTime = lastUpdateTime + timeUsed;
      HandlePendingActions(eventTime);
      Halt(eventTime);
    }
    if (!(m_moveFlags & 0x400F) || (m_moveFlags & 0x10000000)) {
      timeUsed += redirectedTimeUsed;
      break;
    }
    timeUsed += redirectedTimeUsed;
  }

  SetOrientation();
  LogWrite("0x%016I64X: ====| Completed move request\n", m_guid);
  ShowCollisionBox(moveDirWanted, oldMoveFlags);
  return moveModified;
}

float CMovement::CalcFallSurfaceProjection(
    NTempest::C3Vector &position,
    unsigned long       moveStartTime,
    unsigned int        timeLeft,
    NTempest::C3Vector  hitPoint,
    float               distanceAway,
    NTempest::C3Vector &moveNormal,
    NTempest::C4Plane  *platform
) {
  if (NTempest::CMath::fabs_(distanceAway) >= 0.0013888889f && timeLeft) {
    float distToFall = RelDistanceFallen(moveStartTime, static_cast<float>(timeLeft) * 0.001f);
    hitPoint.z -= distToFall;
    NTempest::C3Vector fallVector = hitPoint - position;
    fallVector.Normalize();
    NTempest::C3Vector platformNorm = NTempest::C3Vector::Cross(fallVector, moveNormal);
    if (platformNorm.z < 0.0f) {
      platformNorm = -platformNorm;
    }
    platform->Set(platformNorm, position);
    LogWrite(
        "0x%016I64X: moveNormal(%g,%g,%g) fallVector(%g,%g,%g) hitPoint(%g,%g,%g)\n", m_guid, moveNormal.x, moveNormal.y, moveNormal.z, fallVector.x,
        fallVector.y, fallVector.z, hitPoint.x, hitPoint.y, hitPoint.z
    );
    float cosTheta = platformNorm.z;
    if (cosTheta < -1.0f) {
      cosTheta = -1.0f;
    } else if (cosTheta > 1.0f) {
      cosTheta = 1.0f;
    }
    LogWrite(
        "0x%016I64X: unit position(%g,%g,%g), platform (%g,%g,%g,%g) (%g deg slope), fall (%g)\n", m_guid, position.x, position.y, position.z,
        platform->n.x, platform->n.y, platform->n.z, platform->d, acos(cosTheta) * 57.29578, distToFall
    );
    return distToFall;
  }
  NTempest::C3Vector platformNorm(0.0f, 0.0f, 1.0f);
  platform->Set(platformNorm, position);
  LogWrite("0x%016I64X: unit position(%g,%g,%g), flat platform\n", m_guid, position.x, position.y, position.z);
  return 0.0f;
}

float CMovement::AttemptMove(
    unsigned long       eventTime,
    NTempest::C3Vector &move,
    float               distance2d,
    NTempest::C2Vector &unitMove,
    NTempest::C2Vector &unitMoveWanted,
    NTempest::C4Plane  &ground
) {
  float distance = move.Mag();
  if (NTempest::CMath::fabs_(distance) < 0.00000023841858f) {
    LogWrite("0x%016I64X: insignificant distance to move (%g), not moving\n", m_guid, distance);
    return 0.0f;
  }

  float nearestObstacleDist = ExtrudeCollisionShape(eventTime, move, distance, unitMoveWanted, ground.n);
  if (nearestObstacleDist >= distance2d) {
    LogWrite(
        "0x%016I64X: No obstacles hit, moving (%g,%g,%g) from (%g,%g,%g) to ", m_guid, move.x, move.y, move.z, m_position.x, m_position.y,
        m_position.z
    );
    m_position += move;
    LogWrite("(%g,%g,%g)\n", m_position.x, m_position.y, m_position.z);
    LogWrite("0x%016I64X: Nearest obstacle (%g) away, wanted to move (%g)\n", m_guid, nearestObstacleDist, distance2d);
    return distance2d;
  }

  NTempest::C2Vector adjustedMove(nearestObstacleDist * unitMove.x, nearestObstacleDist * unitMove.y);
  LogWrite("0x%016I64X: Obstacle hit (%g) away, from (%g,%g,%g) ", m_guid, nearestObstacleDist, m_position.x, m_position.y, m_position.z);
  m_position.x += adjustedMove.x;
  m_position.y += adjustedMove.y;
  float newElevation = ground.SolveForZ(m_position.x, m_position.y);
  float adjustedElevation = newElevation - m_position.z;
  LogWrite("moving (%g,%g,%g) to ", adjustedMove.x, adjustedMove.y, adjustedElevation);
  if (adjustedElevation < 0.0f) {
    float maxDrop = -(nearestObstacleDist * 1.1917536f + 0.0013888889f);
    if (maxDrop > adjustedElevation) {
      adjustedElevation = maxDrop;
    }
  }
  m_position.z += adjustedElevation;
  LogWrite("(%g,%g,%g)\n", m_position.x, m_position.y, m_position.z);
  if (NTempest::IsUnitVector(m_reDirection) && (m_moveFlags & 0xF)) {
    m_moveFlags |= 0x1000;
    LogWrite("0x%016I64X: Setting redirected (D:\\build\\buildWoW\\WoW\\Source\\Object\\Collide.cpp: %d)\n", m_guid, 2470);
  } else {
    Halt(eventTime);
  }
  return nearestObstacleDist;
}

int CMovement::IsTooLow(
    NTempest::C3Vector &position,
    unsigned long       moveStartTime,
    CWalkableSurface   *surface,
    float               distanceMoved,
    float               currSpeedInv
) {
  if (distanceMoved >= surface->farDist) {
    LogWrite("0x%016I64X: distanceMoved(%g) beyond surface(%g,%g)\n", m_guid, distanceMoved, surface->closeDist, surface->farDist);
  }
  float minElevation = position.z - (surface->farDist - distanceMoved) * 1.1917536f;
  LogWrite(
      "0x%016I64X: Checking if face is below: LPOC(%g), minElevation(%g), unit(%g)\n", m_guid, surface->lastPtOfContact.z, minElevation, position.z
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
    if (timeToFall < 0) {
      LogWrite("0x%016I64X: timeToFall(%d) less than zero\n", m_guid, timeToFall);
    }
    minElevation = position.z - RelDistanceFallen(timeToFall);
    LogWrite(
        "0x%016I64X: Checking if face is below: FPOC(%g), minElevation(%g), unit(%g)\n", m_guid, surface->firstPtOfContact.z, minElevation, position.z
    );
  } else {
    minElevation = position.z;
  }
  return minElevation - 0.0013888889f > surface->firstPtOfContact.z;
}

static void InsertSurface(CWalkableSurface &toBeInserted, unsigned int startIndex, TSGrowableArray<CWalkableSurface> *surfacePool) {
  unsigned int numSurfaces = surfacePool->Count();
  surfacePool->SetCount(numSurfaces + 1);
  unsigned int insertId;
  for (insertId = startIndex; insertId < numSurfaces; ++insertId) {
    CWalkableSurface &surface = (*surfacePool)[insertId];
    int               insert = 0;
    if (NTempest::CMath::fabs_(toBeInserted.closeDist - surface.closeDist) >= 0.0013888889f) {
      insert = toBeInserted.closeDist <= surface.closeDist;
    } else if (NTempest::CMath::fabs_(toBeInserted.firstPtOfContact.z - surface.firstPtOfContact.z) >= 0.001f) {
      insert = toBeInserted.firstPtOfContact.z >= surface.firstPtOfContact.z;
    } else if (NTempest::CMath::fabs_(toBeInserted.farDist - surface.farDist) < 0.0013888889f) {
      insert = toBeInserted.lastPtOfContact.z >= surface.lastPtOfContact.z;
    } else if (toBeInserted.farDist < surface.farDist) {
      NTempest::C4Plane &surfacePlane = s_facetData.facets[surface.facetId].plane;
      insert = surfacePlane.SolveForZ(toBeInserted.lastPtOfContact.x, toBeInserted.lastPtOfContact.y) <= toBeInserted.lastPtOfContact.z;
    } else {
      NTempest::C4Plane &insertedPlane = s_facetData.facets[toBeInserted.facetId].plane;
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
  newSurface->highestElevation = beingSplit->highestElevation;
  newSurface->closeDist = splitBy->farDist;
  newSurface->firstPtOfContact = splitBy->lastPtOfContact;
  newSurface->facetId = beingSplit->facetId;
  NTempest::C4Plane &plane = s_facetData.facets[newSurface->facetId].plane;
  newSurface->firstPtOfContact.z = plane.SolveForZ(newSurface->firstPtOfContact.x, newSurface->firstPtOfContact.y);
  newSurface->farDist = beingSplit->farDist;
  newSurface->lastPtOfContact = beingSplit->lastPtOfContact;
  beingSplit->farDist = newSurface->closeDist;
  beingSplit->lastPtOfContact = newSurface->firstPtOfContact;
}

void CMovement::ClipFacetsWithOneAnother(NTempest::C4Plane &startPlane, TSGrowableArray<CWalkableSurface> *surfacePool) {
  CWalkableSurface   newSurface2;
  CWalkableSurface   newSurface1;
  NTempest::C3Vector projected;
  NTempest::C3Vector intersection;
  float              firstElev;
  NTempest::C4Plane *surfPlane;
  unsigned int       numSurfaces = surfacePool->Count();
  if (numSurfaces < 2) {
    return;
  }
  for (unsigned int surfaceId = 0; surfaceId < numSurfaces - 1; ++surfaceId) {
    for (unsigned int nextSurfaceId = surfaceId + 1; nextSurfaceId < numSurfaces; ++nextSurfaceId) {
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
      if (NTempest::CMath::fabs_(surface.farDist - nextSurface.farDist) < 0.0013888889f) {
        continue;
      }

      intersection = nextSurface.lastPtOfContact - nextSurface.firstPtOfContact;
      if (NTempest::CMath::fabs_(intersection.Mag()) >= 0.00000095367432f &&
          NTempest::CMath::fabs_(NTempest::C3Vector::Dot(surfPlane->n, intersection)) >= 0.00000095367432f)
      {
        intersection = nextSurface.firstPtOfContact +
                       intersection * (-surfPlane->DistSigned(nextSurface.firstPtOfContact) / NTempest::C3Vector::Dot(surfPlane->n, intersection));
      } else {
        intersection.Set(0.0f, 0.0f, 0.0f);
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

int CMovement::NextSurfaceIsWalkable(
    CWalkableSurface                  *surface,
    unsigned long                      eventTime,
    float                              distanceMoved,
    float                              currSpeedInv,
    TSGrowableArray<CWalkableSurface> *surfacePool
) {
  FATALASSERT(surface);
  LogWrite("0x%016I64X: ------>Checking next walkable surface\n", m_guid);
  NTempest::C3Vector position = surface->lastPtOfContact;
  NTempest::C3Vector above = position;
  above.z += m_collisionBoxHeight;
  NTempest::C4Plane &plane = s_facetData.facets[surface->facetId].plane;
  NTempest::C4Plane  currentCeiling(-plane.n.x, -plane.n.y, -plane.n.z, NTempest::C3Vector::Dot(plane.n, above));
  unsigned int       numSurfaces = surfacePool->Count();
  for (unsigned int surfaceId = 0; surfaceId < numSurfaces; ++surfaceId) {
    CWalkableSurface *next = GetNextSurface(position, surfaceId, eventTime, distanceMoved, currSpeedInv, currentCeiling, surfacePool);
    if (!next) {
      continue;
    }
    float              stepHeight = next->firstPtOfContact.z - m_position.z;
    NTempest::C4Plane &nextPlane = s_facetData.facets[next->facetId].plane;
    float              cosTheta = nextPlane.n.z;
    if (cosTheta < -1.0f) {
      cosTheta = -1.0f;
    } else if (cosTheta > 1.0f) {
      cosTheta = 1.0f;
    }
    LogWrite("0x%016I64X: Next surface: step hgt(%g), incline(%g degrees)", m_guid, stepHeight, acos(cosTheta) * 57.29578f);
    if (stepHeight > m_stepUpHeight) {
      LogWrite(" - Surface was too high\n");
      LogWrite("0x%016I64X: ------>Done checking next walkable surface\n", m_guid);
      return 0;
    }
    if (nextPlane.n.z > 0.64278764f) {
      LogWrite(" - Surface is walkable\n");
      LogWrite("0x%016I64X: ------>Done checking next walkable surface\n", m_guid);
      return 1;
    }
    LogWrite(" - Surface is too steep, continuing\n");
    position = next->lastPtOfContact;
    above = position;
    above.z += m_collisionBoxHeight;
    currentCeiling.Set(-nextPlane.n.x, -nextPlane.n.y, -nextPlane.n.z, NTempest::C3Vector::Dot(nextPlane.n, above));
  }
  LogWrite("0x%016I64X: Ran out of facets, failing\n", m_guid);
  return 0;
}

static void LogSurface(unsigned __int64 guid, CWalkableSurface &surface) {
  float cosTheta = s_facetData.facets[surface.facetId].plane.n.z;
  if (cosTheta < -1.0f) {
    cosTheta = -1.0f;
  } else if (cosTheta > 1.0f) {
    cosTheta = 1.0f;
  }
  CMovement::LogWrite(
      "0x%016I64X: facet close(%g) far(%g) FPOC(%g,%g,%g) LPOC(%g,%g,%g) incline(%g degrees)\n", guid, surface.closeDist, surface.farDist,
      surface.firstPtOfContact.x, surface.firstPtOfContact.y, surface.firstPtOfContact.z, surface.lastPtOfContact.x, surface.lastPtOfContact.y,
      surface.lastPtOfContact.z, acos(cosTheta) * 57.29578f
  );
  CMovement::LogWrite(
      "0x%016I64X: plane eq(%g, %g, %g, %g)\n", guid, s_facetData.facets[surface.facetId].plane.n.x, s_facetData.facets[surface.facetId].plane.n.y,
      s_facetData.facets[surface.facetId].plane.n.z, s_facetData.facets[surface.facetId].plane.d
  );
}

CWalkableSurface *CMovement::GetNextSurface(
    NTempest::C3Vector                &position,
    unsigned int                       surfaceId,
    unsigned long                      eventTime,
    float                              distanceMoved,
    float                              currSpeedInv,
    NTempest::C4Plane                 &currentCeiling,
    TSGrowableArray<CWalkableSurface> *surfacePool
) {
  if (surfaceId >= surfacePool->Count()) {
    return 0;
  }
  CWalkableSurface *surface = &(*surfacePool)[surfaceId];
  LogSurface(m_guid, *surface);
  NTempest::C4Plane &plane = s_facetData.facets[surface->facetId].plane;
  if (plane.n.z <= 0.0f) {
    LogWrite("0x%016I64X: facet faces downward (z: %g), skipping\n", m_guid, plane.n.z);
    return 0;
  }
  if (distanceMoved + 0.0013888889f > surface->farDist) {
    LogWrite("0x%016I64X: facet already passed\n", m_guid);
    return 0;
  }
  if (IsTooLow(position, eventTime, surface, distanceMoved, currSpeedInv)) {
    LogWrite("0x%016I64X: facet too low\n", m_guid);
    return 0;
  }
  float startDistFromTop = currentCeiling.DistSigned(surface->firstPtOfContact);
  float endDistFromTop = currentCeiling.DistSigned(surface->lastPtOfContact);
  if (startDistFromTop >= 0.0013888889f || endDistFromTop >= 0.0013888889f) {
    return surface;
  }
  LogWrite("0x%016I64X: facet too high, start(%g) end(%g)\n", m_guid, startDistFromTop, endDistFromTop);
  return 0;
}

int CMovement::HandlePendingActions(unsigned long eventTime) {
  unsigned short oldMoveFlags = static_cast<unsigned short>(m_moveFlags);

  if (m_moveFlags & 0x00010000) {
    m_moveFlags &= ~3U;
    LogWrite("0x%016I64X: Executing pending stop at (%g,%g,%g)\n", m_guid, m_position.x, m_position.y, m_position.z);
  }
  if (m_moveFlags & 0x00020000) {
    m_moveFlags &= ~0xCU;
    LogWrite("0x%016I64X: Executing pending unstrafe at (%g,%g,%g)\n", m_guid, m_position.x, m_position.y, m_position.z);
  }
  if (!(m_moveFlags & 0xF)) {
    m_moveFlags &= ~0x1000U;
  }
  if (m_moveFlags & 0x00180000) {
    LogWrite("0x%016I64X: Executing pending move start at (%g,%g,%g)\n", m_guid, m_position.x, m_position.y, m_position.z);
    StartMove(eventTime, (m_moveFlags & 0x00080000) != 0);
  }
  if (m_moveFlags & 0x00600000) {
    LogWrite("0x%016I64X: Executing pending strafe start at (%g,%g,%g)\n", m_guid, m_position.x, m_position.y, m_position.z);
    StartStrafe(eventTime, (m_moveFlags & 0x00200000) != 0);
  }
  if (m_moveFlags & 0x00040000) {
    StartFalling(eventTime);
  }

  m_moveFlags &= 0xFF80FFFF;
  return (oldMoveFlags ^ static_cast<unsigned short>(m_moveFlags)) & 0x400F;
}

void CMovement::CheckSurfaceObstacles(
    CWalkableSurface                  *surface,
    unsigned long                      eventTime,
    float                             *distanceLeft,
    float                              distanceMoved,
    float                              currSpeedInv,
    TSGrowableArray<CWalkableSurface> *surfacePool,
    CRedirect                         *hitInfo
) {
  float cosTheta = surface->firstPtOfContact.z - m_position.z;
  if (cosTheta > m_stepUpHeight) {
    LogWrite(
        "0x%016I64X: Facet too high (facet: %g, unit: %g, step hgt: %g), obstructing\n", m_guid, surface->firstPtOfContact.z, m_position.z,
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
    LogWrite(
        "0x%016I64X: Step up blocked (facet: %g, unit: %g, step hgt: %g), obstructing\n", m_guid, surface->firstPtOfContact.z, m_position.z,
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
    LogWrite("0x%016I64X: Facet too steep (%g degree slope), but moving down-hill\n", m_guid, acos(cosTheta) * 57.29578f);
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
    LogWrite("0x%016I64X: Facet too steep (%g degree slope), but low enough to step over\n", m_guid, acos(cosTheta) * 57.29578f);
    return;
  }

  cosTheta = s_facetData.facets[surface->facetId].plane.n.z;
  if (cosTheta < -1.0f) {
    cosTheta = -1.0f;
  } else if (cosTheta > 1.0f) {
    cosTheta = 1.0f;
  }
  LogWrite(
      "0x%016I64X: Facet too steep (%g degree slope), highest elev (%g), unit Z(%g) obstructing\n", m_guid, acos(cosTheta) * 57.29578f,
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

unsigned int CMovement::Swim(unsigned long eventTime, unsigned int timeToMove, NTempest::C3Vector &moveWanted, NTempest::C3Vector &unitMoveWanted) {
  if (!timeToMove || (m_transportGUID && FallFromTransport())) {
    return 0;
  }

  LogWrite("0x%016I64X: ====| Starting new swim: time to move (%u)\n", m_guid, timeToMove);
  NTempest::C3Vector move = moveWanted;
  float              distance = move.Mag();
  if (NTempest::CMath::fabs_(distance) < 0.00000023841858f) {
    return 0;
  }

  float              inverseDistance = 1.0f / distance;
  NTempest::C3Vector unitMove = move * inverseDistance;
  float              distanceMoved;
  if (s_facetData.facets.Count()) {
    if (move.z < 0.0f) {
      distanceMoved = ExtrudeFlyBoxDown(unitMove, unitMoveWanted, distance);
    } else {
      distanceMoved = ExtrudeFlyBoxUp(unitMove, unitMoveWanted, distance);
    }
  } else {
    distanceMoved = CollideWithWaterSurface(unitMove, unitMoveWanted, distance);
  }

  if (distanceMoved < distance) {
    move = unitMove * distanceMoved;
  } else {
    distanceMoved = distance;
  }
  m_position += move;

  if (IsLocalPlayer() && m_position.z - 0.0013888889f > m_waterSurfaceElev) {
    float        timeUsed = inverseDistance * distanceMoved * static_cast<float>(timeToMove);
    unsigned int eventOffset;
    if (timeUsed <= 0.0f) {
      eventOffset = static_cast<unsigned int>(-static_cast<long>(-timeUsed + 0.5f));
    } else {
      eventOffset = static_cast<unsigned int>(timeUsed);
    }
    StopSwimLocal(eventTime + eventOffset);
    ProcessLocalMoveEvent(204);
  }

  LogWrite(
      "0x%016I64X: ====| Swam (%g) (%g,%g,%g) to (%g,%g,%g)\n", m_guid, distanceMoved, move.x, move.y, move.z, m_position.x, m_position.y,
      m_position.z
  );
  m_groundNormal.Set(0.0f, 0.0f, 1.0f);
  if (distanceMoved == distance) {
    return timeToMove;
  }

  float timeUsed = NTempest::CMath::fabs_(distanceMoved) * inverseDistance * (static_cast<float>(timeToMove) * 0.001f) * 1000.0f;
  if (timeUsed <= 0.0f) {
    return static_cast<unsigned int>(-static_cast<long>(-timeUsed + 0.5f));
  }
  return static_cast<unsigned int>(timeUsed + 0.5f);
}

unsigned int
CMovement::ProjectileFall(unsigned long eventTime, unsigned int timeToMove, NTempest::C3Vector &moveWanted, NTempest::C2Vector &unitMoveWanted) {
  if (!timeToMove || (m_transportGUID && FallFromTransport())) {
    return 0;
  }

  LogWrite(
      "0x%016I64X: ====| Starting new projectile fall: time to move (%u), time fallen so far (%d)\n", m_guid, timeToMove, eventTime - m_fallStartTime
  );
  float              updateFallTimeSecs = static_cast<float>(timeToMove) * 0.001f;
  float              relativeFall = RelDistanceFallen(eventTime, updateFallTimeSecs);
  NTempest::C3Vector move(moveWanted.x, moveWanted.y, -relativeFall);
  float              distance = move.Mag();
  if (NTempest::CMath::fabs_(distance) < 0.00000023841858f) {
    return 0;
  }

  float              inverseDistance = 1.0f / distance;
  NTempest::C3Vector unitMove = move * inverseDistance;
  LogWrite("0x%016I64X: ====| Wanted to projectile fall (%g) (%g,%g,%g)\n", m_guid, distance, move.x, move.y, move.z);

  unsigned __int64 gameObjHit = 0;
  float            distanceMoved = FLT_MAX;
  if (s_facetData.facets.Count()) {
    if (relativeFall >= 0.0f) {
      distanceMoved = ExtrudeProjectileBoxDownHill(eventTime, unitMove, distance, unitMoveWanted, &gameObjHit);
    } else {
      distanceMoved = ExtrudeProjectileBoxUpHill(unitMove, distance, &gameObjHit);
    }
  }

  unsigned int timeUsed = timeToMove;
  if (distanceMoved < distance) {
    move = unitMove * distanceMoved;
    m_position += move;
    float adjustedTime = NTempest::CMath::fabs_(distanceMoved) * inverseDistance * updateFallTimeSecs * 1000.0f;
    if (adjustedTime <= 0.0f) {
      timeUsed = static_cast<unsigned int>(-static_cast<long>(-adjustedTime + 0.5f));
    } else {
      timeUsed = static_cast<unsigned int>(adjustedTime + 0.5f);
    }

    if (m_moveFlags & 0x4000) {
      if (relativeFall < 0.0f) {
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
  LogWrite(
      "0x%016I64X: ====| Projectile fell (%g) (%g,%g,%g) to (%g,%g,%g)\n", m_guid, distanceMoved, move.x, move.y, move.z, m_position.x, m_position.y,
      m_position.z
  );
  m_groundNormal.Set(0.0f, 0.0f, 1.0f);
  return timeUsed;
}

unsigned int CMovement::TraceSurface(
    unsigned long       eventTime,
    unsigned int        timeToMove,
    float               distance,
    NTempest::C2Vector &unitMove,
    NTempest::C2Vector &unitMoveWanted
) {
  FATALASSERT(!(m_moveFlags & 0x4000));
  FATALASSERT(!(m_moveFlags & 0x01000000));
  FATALASSERT(!(m_moveFlags & 0x02000000));
  FATALASSERT(!(m_moveFlags & 0x10000000));
  FATALASSERT(m_moveFlags & 0xF);
  if (NTempest::CMath::fabs_(distance) < 0.00000095367432f) {
    return timeToMove;
  }

  CRedirect          hitInfo;
  NTempest::C4Plane  slopeTestPlane(unitMove.y, -unitMove.x, 0.0f, -(unitMove.y * m_position.x - unitMove.x * m_position.y));
  NTempest::C3Vector normal;
  NTempest::C4Plane  currentCeiling;
  NTempest::C4Plane  startPlane;
  float              absDistMoved = 0.0f;
  float              currentSpeedInv = static_cast<float>(timeToMove) * 0.001f / distance;
  float              cosTheta;
  NTempest::C2Vector moveVector(unitMove.x * distance, unitMove.y * distance);
  unsigned int       numSurfaces;
  NTempest::C3Vector above;
  NTempest::C4Plane  currentPlatform;
  unsigned int       surfaceId;
  float              distanceMoved;
  CWalkableSurface  *surface;
  unsigned long      currEventTime;
  float              segmentDist;
  unsigned int       timeUsed;
  float              distMoved;
  float              adjustedDist;
  NTempest::C3Vector fullMoveVector;

  LogWrite(
      "0x%016I64X: *** Requesting new move vector(%g,%g), current time (0x%08X), elapsed (%u), distance (%g)\n", m_guid, moveVector.x, moveVector.y,
      eventTime, timeToMove, distance
  );
  surfacePool.SetCount(0);
  EnqueueFacets(slopeTestPlane, *reinterpret_cast<NTempest::C2Vector *>(&m_position), unitMove, s_facetData.facets, &surfacePool);

  NTempest::C3Vector r(unitMove.x, unitMove.y, 0.0f);
  startPlane.Set(r, m_position);
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
    if (NTempest::CMath::fabs_(surface->closeDist - distanceMoved - adjustedDist) < 0.0013888889f && hitInfo.flags) {
      fullMoveVector *= adjustedDist;
      fullMoveVector.z = currentPlatform.SolveForZ(m_position.x + fullMoveVector.x, m_position.y + fullMoveVector.y) - m_position.z;
      if (fullMoveVector.z < -(adjustedDist * 1.1917536f + 0.0013888889f)) {
        fullMoveVector.z = -(adjustedDist * 1.1917536f + 0.0013888889f);
      }
      segmentDist = adjustedDist;
      LogWrite(
          "0x%016I64X: Completing move at start of facet, attempting to move (%g,%g,%g)\n", m_guid, fullMoveVector.x, fullMoveVector.y,
          fullMoveVector.z
      );
    } else if (segmentDist < adjustedDist) {
      fullMoveVector *= segmentDist;
      fullMoveVector.z = surface->lastPtOfContact.z - m_position.z;
      if (fullMoveVector.z < -(segmentDist * 1.1917536f + 0.0013888889f)) {
        fullMoveVector.z = -(segmentDist * 1.1917536f + 0.0013888889f);
      }
      LogWrite("0x%016I64X: moving to end of facet, attempting to move (%g,%g,%g)\n", m_guid, fullMoveVector.x, fullMoveVector.y, fullMoveVector.z);
    } else {
      fullMoveVector *= adjustedDist;
      fullMoveVector.z =
          s_facetData.facets[surface->facetId].plane.SolveForZ(m_position.x + fullMoveVector.x, m_position.y + fullMoveVector.y) - m_position.z;
      if (fullMoveVector.z < -(adjustedDist * 1.1917536f + 0.0013888889f)) {
        fullMoveVector.z = -(adjustedDist * 1.1917536f + 0.0013888889f);
      }
      segmentDist = adjustedDist;
      LogWrite("0x%016I64X: Completing move on facet, attempting to move (%g,%g,%g)\n", m_guid, fullMoveVector.x, fullMoveVector.y, fullMoveVector.z);
    }

    distMoved = AttemptMove(currEventTime, fullMoveVector, segmentDist, unitMove, unitMoveWanted, s_facetData.facets[surface->facetId].plane);
    int wasRedirected = m_moveFlags & 0x1000;
    distanceMoved += distMoved;
    absDistMoved += NTempest::CMath::fabs_(distMoved);
    distance -= NTempest::CMath::fabs_(segmentDist);
    if (!wasRedirected && !(m_moveFlags & 0x1000) && (m_moveFlags & 0xF) && hitInfo.flags) {
      fullMoveVector.Set(unitMoveWanted.x, unitMoveWanted.y, 0.0f);
      Redirect(currEventTime, fullMoveVector, currentPlatform.n, hitInfo);
    }

    timeUsed = absDistMoved * currentSpeedInv * 1000.0f <= 0.0f
                   ? static_cast<unsigned int>(-static_cast<int>(-(absDistMoved * currentSpeedInv * 1000.0f) + 0.5f))
                   : static_cast<unsigned int>(absDistMoved * currentSpeedInv * 1000.0f + 0.5f);
    if ((!wasRedirected && (m_moveFlags & 0x1000)) || NTempest::CMath::fabs_(distMoved - segmentDist) >= 0.00000095367432f ||
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
    LogWrite(
        "0x%016I64X: setting platform (%g,%g,%g,%g) (%g deg slope)\n", m_guid, currentPlatform.n.x, currentPlatform.n.y, currentPlatform.n.z,
        currentPlatform.d, acos(cosTheta) * 57.29578f
    );
    above = m_position;
    above.z += m_collisionBoxHeight;
    normal = -currentPlatform.n;
    currentCeiling.Set(normal, above);
    ++surfaceId;
  }

  if (distance > 0.00000095367432f || timeUsed != timeToMove) {
    LogWrite("0x%016I64X: no more facets, falling\n", m_guid);
    ProcessFalling(eventTime + timeUsed);
  }
  return timeUsed < timeToMove ? timeUsed : timeToMove;
}

int CMovement::DetermineHitType(int hitType, NTempest::C3Vector &unitMove, float distance, unsigned int facetId, CRedirect *hitInfo) {
  FATALASSERT(hitType < 3);
  if (hitType == 0) {
    return DetermineBoxHitType(unitMove, distance, facetId, m_collisionBoxHalfDepth * 1.849399f, hitInfo);
  }
  if (hitType == 1) {
    DeterminePyramidHitType(unitMove, distance, facetId, hitInfo);
  } else {
    return DetermineBoxHitType(unitMove, distance, facetId, m_stepUpHeight, hitInfo);
  }
  FATALASSERT(hitInfo->flags != 0);
  return 1;
}

void CMovement::DeterminePyramidHitType(NTempest::C3Vector &unitMove, float distance, unsigned int facetId, CRedirect *hitInfo) {
  NTempest::C3Vector newPosition = m_position + unitMove * distance;
  int                hitPyrTrailingX = unitMove.x >= 0.0f;
  int                hitPyrTrailingY = unitMove.y >= 0.0f;

  NTempest::C3Vector pyrNormX(0.87964189f, 0.0f, -0.4756366f);
  NTempest::C3Vector pyrNormY(0.0f, 0.87964189f, -0.4756366f);
  NTempest::C4Plane  pyrSides[4];
  pyrSides[!hitPyrTrailingX].Set(pyrNormX, newPosition);
  pyrNormX.x = -pyrNormX.x;
  pyrSides[hitPyrTrailingX].Set(pyrNormX, newPosition);
  pyrSides[2 + !hitPyrTrailingY].Set(pyrNormY, newPosition);
  pyrNormY.y = -pyrNormY.y;
  pyrSides[2 + hitPyrTrailingY].Set(pyrNormY, newPosition);

  NTempest::C4Plane topBottom[2];
  topBottom[0].Set(0.0f, 0.0f, 1.0f, -newPosition.z - m_collisionBoxHalfDepth * 1.849399f);
  topBottom[1].Set(0.0f, 0.0f, -1.0f, newPosition.z);

  NTempest::CFacet &facet = s_facetData.facets[facetId];
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

static float FindClosestVertDist(NTempest::C4Plane &front, NTempest::C4Plane *sides, CClippedTriangle &poly) {
  CClippedTriangle testPoly = poly;
  float            penetrationDepth;
  if (!ClipPolygonToPolyhedron(sides, 2, &testPoly, &penetrationDepth)) {
    return FLT_MAX;
  }

  float minDist = FLT_MAX;
  for (unsigned int i = 0; i < testPoly.Count(); ++i) {
    float distance = front.DistSigned(testPoly[i]);
    if (distance < minDist) {
      minDist = distance;
    }
  }
  return minDist;
}

static void DetermineBoxParallelHitType(NTempest::C4Plane *boxSides, CClippedTriangle &clippedPoly, CRedirect *hitInfo) {
  float xDist = FindClosestVertDist(boxSides[0], &boxSides[2], clippedPoly);
  float yDist = FindClosestVertDist(boxSides[2], &boxSides[0], clippedPoly);
  if (NTempest::CMath::fabs_(xDist - yDist) < 0.00000095367432f) {
    hitInfo->flags |= 3;
  } else if (xDist >= yDist) {
    hitInfo->flags |= 2;
  } else {
    hitInfo->flags |= 1;
  }
}

int CMovement::DetermineBoxHitType(NTempest::C3Vector &unitMove, float distance, unsigned int facetId, float baseHeight, CRedirect *hitInfo) {
  NTempest::C3Vector newPosition = m_position + unitMove * distance;
  int                hitBoxTrailingX = unitMove.x >= 0.0f;
  int                hitBoxTrailingY = unitMove.y >= 0.0f;

  NTempest::C4Plane boxSides[4];
  boxSides[!hitBoxTrailingX].Set(1.0f, 0.0f, 0.0f, -newPosition.x - m_collisionBoxHalfDepth);
  boxSides[hitBoxTrailingX].Set(-1.0f, 0.0f, 0.0f, newPosition.x - m_collisionBoxHalfDepth);
  boxSides[2 + !hitBoxTrailingY].Set(0.0f, 1.0f, 0.0f, -newPosition.y - m_collisionBoxHalfDepth);
  boxSides[2 + hitBoxTrailingY].Set(0.0f, -1.0f, 0.0f, newPosition.y - m_collisionBoxHalfDepth);

  NTempest::C4Plane topBottom[2];
  topBottom[0].Set(0.0f, 0.0f, 1.0f, -newPosition.z - m_collisionBoxHeight);
  topBottom[1].Set(0.0f, 0.0f, -1.0f, newPosition.z + baseHeight);

  NTempest::CFacet &facet = s_facetData.facets[facetId];
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

NTempest::C3Vector CMovement::CalcAverageSurfaceNormal(NTempest::C4Plane *box) {
  NTempest::C3Vector average(0.0f);
  unsigned int       numNormals = 0;
  for (unsigned int facetId = 0; facetId < s_facetData.facets.Count(); ++facetId) {
    NTempest::CFacet &facet = s_facetData.facets[facetId];
    if (facet.plane.n.z <= 0.017452406f) {
      continue;
    }

    CClippedTriangle clippedPoly;
    clippedPoly.Init(facet.vertices);
    float penetrationDepth;
    if (ClipPolygonToPolyhedron(box, 6, &clippedPoly, &penetrationDepth) && penetrationDepth >= 0.013888889f) {
      average += facet.plane.n;
      ++numNormals;
    }
  }

  if (!numNormals) {
    return NTempest::C3Vector(0.0f, 0.0f, 1.0f);
  }
  average *= 1.0f / static_cast<float>(numNormals);
  average.Normalize();
  return average;
}

void CMovement::FindObstacles(
    NTempest::C3Vector &unitMoveVector,
    NTempest::C4Plane  *box,
    unsigned int        numSides,
    NTempest::C4Plane  &startPlane,
    int                 hitType,
    float              *closestDist,
    CRedirect          *hitInfo
) {
  for (unsigned int facetId = 0; facetId < s_facetData.facets.Count(); ++facetId) {
    NTempest::CFacet &facet = s_facetData.facets[facetId];
    if (NTempest::C3Vector::Dot(facet.plane.n, unitMoveVector) > -0.017452406f) {
      CollisionInfoColorFace(facetId, FACET_TESTED_UNTOUCHED);
      continue;
    }

    CClippedTriangle clippedPoly;
    clippedPoly.Init(facet.vertices);
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

    NTempest::C3Vector closest(0.0f);
    float              minDist = FLT_MAX;
    for (unsigned int i = 0; i < clippedPoly.Count(); ++i) {
      float distance = DistFromPlaneAlongVector(clippedPoly[i], startPlane, unitMoveVector);
      if (distance < minDist) {
        minDist = distance;
        closest = clippedPoly[i];
      }
    }

    if (hitInfo && hitInfo->flags && NTempest::CMath::fabs_(minDist - *closestDist) < 0.0013888889f) {
      float dx = closest.x - hitInfo->hitPoint.x;
      float dy = closest.y - hitInfo->hitPoint.y;
      float sqDist = dx * dx + dy * dy;
      if (facetId < s_facetData.gameObjects.Count() && s_facetData.gameObjects[facetId]) {
        hitInfo->gameObjHit = s_facetData.gameObjects[facetId];
      }

      if (NTempest::CMath::fabs_(sqDist) < 0.000048225309f) {
        float newDot = NTempest::C3Vector::Dot(facet.plane.n, unitMoveVector);
        float oldDot = NTempest::C3Vector::Dot(hitInfo->surfaceNorm[0], unitMoveVector);
        if (NTempest::CMath::fabs_(newDot - oldDot) >= 0.00000095367432f) {
          hitInfo->flags |= 0x80;
          if (newDot >= oldDot) {
            hitInfo->surfaceNorm[1] = facet.plane.n;
          } else {
            hitInfo->surfaceNorm[1] = hitInfo->surfaceNorm[0];
            hitInfo->surfaceNorm[0] = facet.plane.n;
          }
        }
      } else if (DetermineHitType(hitType, unitMoveVector, minDist, facetId, hitInfo)) {
        *closestDist = minDist;
        CollisionInfoColorFace(facetId, FACET_BLOCKING);
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

    unsigned int savedFlags = hitInfo->flags;
    hitInfo->flags = 0;
    if (!DetermineHitType(hitType, unitMoveVector, minDist, facetId, hitInfo)) {
      hitInfo->flags = savedFlags;
      continue;
    }

    hitInfo->surfaceNorm[0] = facet.plane.n;
    hitInfo->gameObjHit = facetId < s_facetData.gameObjects.Count() ? s_facetData.gameObjects[facetId] : 0;
    hitInfo->hitPoint = closest;
    *closestDist = minDist;
    CollisionInfoColorFace(facetId, FACET_BLOCKING);
  }
}

float CMovement::ExtrudeCollisionShape(
    unsigned long       timeStamp,
    NTempest::C3Vector &moveVector,
    float               distanceWanted,
    NTempest::C2Vector &unitMoveWanted,
    NTempest::C3Vector &platformNorm
) {
  NTempest::C3Vector unitMove = moveVector;
  if (unitMove.Mag() >= 0.00000023841858f) {
    unitMove.Normalize();
  }
  NTempest::C4Plane boxPlanes[3][6];
  NTempest::C4Plane startPlanes[3];
  CRedirect         hitInfoX;
  CRedirect         hitInfoY;
  CRedirect         hitInfoZ;
  float             distances[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
  float             bottom = m_stepUpHeight;
  ExtrudeBoxSideX(moveVector, bottom, boxPlanes[0]);
  ExtrudeBoxSideY(moveVector, bottom, boxPlanes[1]);
  ExtrudeBoxSideZ(moveVector, bottom, boxPlanes[2]);
  CRedirect *hitInfo[3] = {&hitInfoX, &hitInfoY, &hitInfoZ};
  for (int side = 0; side < 3; ++side) {
    startPlanes[side] = boxPlanes[side][0];
    startPlanes[side].d += 0.027777778f;
    FindObstacles(unitMove, boxPlanes[side], 6, startPlanes[side], 0, &distances[side], hitInfo[side]);
  }
  float distance = distances[0];
  if (distances[1] < distance) {
    distance = distances[1];
  }
  if (distances[2] < distance) {
    distance = distances[2];
  }
  if (NTempest::CMath::fabs_(distances[0] - distance) >= 0.0013888889f) {
    hitInfoX.Reset();
  }
  if (NTempest::CMath::fabs_(distances[1] - distance) >= 0.0013888889f) {
    hitInfoY.Reset();
  }
  if (NTempest::CMath::fabs_(distances[2] - distance) >= 0.0013888889f) {
    hitInfoZ.Reset();
  }
  if (distance < distanceWanted) {
    NTempest::C3Vector wanted(unitMoveWanted.x, unitMoveWanted.y, 0.0f);
    if (hitInfoX.flags || hitInfoY.flags) {
      Redirect(timeStamp, wanted, platformNorm, hitInfoX, hitInfoY);
    } else {
      Redirect(timeStamp, wanted, platformNorm, hitInfoZ);
    }
  }
  return distance > 0.0f ? distance : 0.0f;
}

int CMovement::TestStepUp(NTempest::C3Vector &destination) {
  float stepHeight = destination.z - m_position.z;
  if (stepHeight <= 0.0f) {
    return 1;
  }
  if (FindCeilingDistanceAbove(stepHeight) < stepHeight) {
    return 0;
  }
  NTempest::C3Vector moveVector = destination - m_position;
  float              distWanted = moveVector.Mag();
  if (distWanted < 0.00000023841858f) {
    return 1;
  }
  NTempest::C3Vector unitMove = moveVector;
  unitMove.Normalize();
  NTempest::C4Plane xBoxPlanes[6];
  NTempest::C4Plane yBoxPlanes[6];
  NTempest::C4Plane startPlanes[2];
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

void CMovement::ExtrudeBoxSideZ(NTempest::C3Vector &moveVector, float bottom, NTempest::C4Plane *boxSides) {
  float              height[2] = {bottom, m_collisionBoxHeight};
  NTempest::C3Vector moveDir[2] = {-moveVector, moveVector};
  int                posZ = moveVector.z >= 0.0f;
  if (moveDir[0].Mag() >= 0.00000023841858f) {
    moveDir[0].Normalize();
    moveDir[1].Normalize();
  }

  float              faceZ = m_position.z + height[posZ];
  NTempest::C3Vector leftPoint(m_position.x - m_collisionBoxHalfDepth, m_position.y - m_collisionBoxHalfDepth, faceZ);
  NTempest::C3Vector rghtPoint(m_position.x + m_collisionBoxHalfDepth, m_position.y + m_collisionBoxHalfDepth, faceZ);
  NTempest::C3Vector face[4] = {
      leftPoint, NTempest::C3Vector(rghtPoint.x, leftPoint.y, faceZ), rghtPoint, NTempest::C3Vector(leftPoint.x, rghtPoint.y, faceZ)
  };
  NTempest::C4Plane basePlane(moveDir[0], face[0]);
  boxSides[0] = basePlane;
  NTempest::C3Vector endPoint = face[0] + moveVector;
  boxSides[1].Set(moveDir[1], endPoint);

  NTempest::C3Vector center = m_position + moveVector * 0.5f;
  center.z = faceZ + moveVector.z * 0.5f;
  for (int side = 0; side < 4; ++side) {
    NTempest::C3Vector edge = face[(side + 1) & 3] - face[side];
    NTempest::C3Vector normal = NTempest::C3Vector::Cross(edge, moveVector);
    if (normal.Mag() >= 0.00000023841858f) {
      normal.Normalize();
    }
    boxSides[side + 2].Set(normal, face[side]);
    if (boxSides[side + 2].DistSigned(center) > 0.0f) {
      boxSides[side + 2] = -boxSides[side + 2];
    }
  }
}

void CMovement::ExtrudeBoxSideY(NTempest::C3Vector &moveVector, float bottom, NTempest::C4Plane *boxSides) {
  float              depth[2] = {-m_collisionBoxHalfDepth, m_collisionBoxHalfDepth};
  NTempest::C3Vector moveDir[2] = {-moveVector, moveVector};
  int                posY = moveVector.y >= 0.0f;
  if (moveDir[0].Mag() >= 0.00000023841858f) {
    moveDir[0].Normalize();
    moveDir[1].Normalize();
  }

  float              faceY = m_position.y + depth[posY];
  NTempest::C3Vector botPoint(m_position.x - m_collisionBoxHalfDepth, faceY, m_position.z + bottom);
  NTempest::C3Vector topPoint(m_position.x + m_collisionBoxHalfDepth, faceY, m_position.z + m_collisionBoxHeight);
  NTempest::C3Vector face[4] = {
      botPoint, NTempest::C3Vector(topPoint.x, faceY, botPoint.z), topPoint, NTempest::C3Vector(botPoint.x, faceY, topPoint.z)
  };
  NTempest::C4Plane basePlane(moveDir[0], face[0]);
  boxSides[0] = basePlane;
  NTempest::C3Vector endPoint = face[0] + moveVector;
  boxSides[1].Set(moveDir[1], endPoint);

  NTempest::C3Vector center = m_position + moveVector * 0.5f;
  center.y = faceY + moveVector.y * 0.5f;
  center.z += (bottom + m_collisionBoxHeight) * 0.5f;
  for (int side = 0; side < 4; ++side) {
    NTempest::C3Vector edge = face[(side + 1) & 3] - face[side];
    NTempest::C3Vector normal = NTempest::C3Vector::Cross(edge, moveVector);
    if (normal.Mag() >= 0.00000023841858f) {
      normal.Normalize();
    }
    boxSides[side + 2].Set(normal, face[side]);
    if (boxSides[side + 2].DistSigned(center) > 0.0f) {
      boxSides[side + 2] = -boxSides[side + 2];
    }
  }
}

void CMovement::ExtrudeBoxSideX(NTempest::C3Vector &moveVector, float bottom, NTempest::C4Plane *boxSides) {
  float              depth[2] = {-m_collisionBoxHalfDepth, m_collisionBoxHalfDepth};
  NTempest::C3Vector moveDir[2] = {-moveVector, moveVector};
  int                posX = moveVector.x >= 0.0f;
  if (moveDir[0].Mag() >= 0.00000023841858f) {
    moveDir[0].Normalize();
    moveDir[1].Normalize();
  }

  float              faceX = m_position.x + depth[posX];
  NTempest::C3Vector botPoint(faceX, m_position.y - m_collisionBoxHalfDepth, m_position.z + bottom);
  NTempest::C3Vector topPoint(faceX, m_position.y + m_collisionBoxHalfDepth, m_position.z + m_collisionBoxHeight);
  NTempest::C3Vector face[4] = {
      botPoint, NTempest::C3Vector(faceX, topPoint.y, botPoint.z), topPoint, NTempest::C3Vector(faceX, botPoint.y, topPoint.z)
  };
  NTempest::C4Plane basePlane(moveDir[0], face[0]);
  boxSides[0] = basePlane;
  NTempest::C3Vector endPoint = face[0] + moveVector;
  boxSides[1].Set(moveDir[1], endPoint);

  NTempest::C3Vector center = m_position + moveVector * 0.5f;
  center.x = faceX + moveVector.x * 0.5f;
  center.z += (bottom + m_collisionBoxHeight) * 0.5f;
  for (int side = 0; side < 4; ++side) {
    NTempest::C3Vector edge = face[(side + 1) & 3] - face[side];
    NTempest::C3Vector normal = NTempest::C3Vector::Cross(edge, moveVector);
    if (normal.Mag() >= 0.00000023841858f) {
      normal.Normalize();
    }
    boxSides[side + 2].Set(normal, face[side]);
    if (boxSides[side + 2].DistSigned(center) > 0.0f) {
      boxSides[side + 2] = -boxSides[side + 2];
    }
  }
}

int CMovement::ExtrudePyramidSideX(NTempest::C3Vector &unitMove, float distance, NTempest::C4Plane *boxSides) {
  if (NTempest::CMath::fabs_(unitMove.x) < 0.00000023841858f && NTempest::CMath::fabs_(unitMove.z) < 0.00000023841858f) {
    return 0;
  }
  NTempest::C3Vector moveVector = unitMove * distance;
  NTempest::C3Vector moveDir[2] = {-unitMove, unitMove};
  int                posX = unitMove.x >= 0.0f;
  float              depth[2] = {-m_collisionBoxHalfDepth, m_collisionBoxHalfDepth};
  NTempest::C3Vector posZNorm(0.0f, 0.0f, 1.0f);
  NTempest::C3Vector baseVector(depth[posX], 0.0f, m_collisionBoxHalfDepth * 1.849399f);
  NTempest::C3Vector posPyramidEdge = m_position + baseVector;
  posPyramidEdge.y += m_collisionBoxHalfDepth;
  NTempest::C3Vector negPyramidEdge = posPyramidEdge;
  negPyramidEdge.y -= 2.0f * m_collisionBoxHalfDepth;
  NTempest::C3Vector face[3] = {m_position, negPyramidEdge, posPyramidEdge};
  NTempest::C3Vector faceNorm = NTempest::C3Vector::Cross(face[1] - face[0], face[2] - face[0]);
  faceNorm.Normalize();
  NTempest::C3Vector center = (face[0] + face[1] + face[2]) * (1.0f / 3.0f) + moveVector * 0.5f;
  boxSides[0].Set(-faceNorm, face[0]);
  NTempest::C3Vector endPoint = face[0] + moveVector;
  boxSides[1].Set(faceNorm, endPoint);
  for (int side = 0; side < 3; ++side) {
    NTempest::C3Vector edge = face[(side + 1) % 3] - face[side];
    NTempest::C3Vector normal = NTempest::C3Vector::Cross(edge, moveVector);
    if (normal.Mag() >= 0.00000023841858f) {
      normal.Normalize();
    }
    boxSides[side + 2].Set(normal, face[side]);
    if (boxSides[side + 2].DistSigned(center) > 0.0f) {
      boxSides[side + 2] = -boxSides[side + 2];
    }
  }
  return 1;
}

int CMovement::ExtrudePyramidSideY(NTempest::C3Vector &unitMove, float distance, NTempest::C4Plane *boxSides) {
  if (NTempest::CMath::fabs_(unitMove.y) < 0.00000023841858f && NTempest::CMath::fabs_(unitMove.z) < 0.00000023841858f) {
    return 0;
  }
  NTempest::C3Vector moveVector = unitMove * distance;
  NTempest::C3Vector moveDir[2] = {-unitMove, unitMove};
  int                posY = unitMove.y >= 0.0f;
  float              depth[2] = {-m_collisionBoxHalfDepth, m_collisionBoxHalfDepth};
  NTempest::C3Vector posZNorm(0.0f, 0.0f, 1.0f);
  NTempest::C3Vector baseVector(0.0f, depth[posY], m_collisionBoxHalfDepth * 1.849399f);
  NTempest::C3Vector posPyramidEdge = m_position + baseVector;
  posPyramidEdge.x += m_collisionBoxHalfDepth;
  NTempest::C3Vector negPyramidEdge = posPyramidEdge;
  negPyramidEdge.x -= 2.0f * m_collisionBoxHalfDepth;
  NTempest::C3Vector face[3] = {m_position, posPyramidEdge, negPyramidEdge};
  NTempest::C3Vector faceNorm = NTempest::C3Vector::Cross(face[1] - face[0], face[2] - face[0]);
  faceNorm.Normalize();
  NTempest::C3Vector center = (face[0] + face[1] + face[2]) * (1.0f / 3.0f) + moveVector * 0.5f;
  boxSides[0].Set(-faceNorm, face[0]);
  NTempest::C3Vector endPoint = face[0] + moveVector;
  boxSides[1].Set(faceNorm, endPoint);
  for (int side = 0; side < 3; ++side) {
    NTempest::C3Vector edge = face[(side + 1) % 3] - face[side];
    NTempest::C3Vector normal = NTempest::C3Vector::Cross(edge, moveVector);
    if (normal.Mag() >= 0.00000023841858f) {
      normal.Normalize();
    }
    boxSides[side + 2].Set(normal, face[side]);
    if (boxSides[side + 2].DistSigned(center) > 0.0f) {
      boxSides[side + 2] = -boxSides[side + 2];
    }
  }
  return 1;
}
