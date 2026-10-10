#include <Base/Base.h>
#include <Gx/Gx.h>
#include <WowConst.h>
#include <MapDefs.h>

#include "Object.h"

#include "Base/CDataStore.h"
#include "Base/UnrealConstants.h"
#include "Os/OsTime.h"

#include <windows.h>

#include <float.h>
#include <math.h>

void CClientMoveUpdate::Skip(CDataStore *packet) {
  UINT   flags = CMovementStatus::Skip(packet);
  LPVOID unused;
  packet->GetDataInSitu(unused, 20);
  if (flags & 0x04000000) {
    CMoveSpline::Skip(packet);
  }
}

inline CDataStore &operator<<(CDataStore &s_, const CMovementStatus &d_) {
  s_ << d_.transport << d_.transRelPosition << d_.transRelFacing << d_.worldPosition << d_.worldFacing << d_.pitch << d_.moveFlags;
  return s_;
}

inline CDataStore &operator>>(CDataStore &s_, CMovementStatus &d_) {
  s_ >> d_.transport >> d_.transRelPosition >> d_.transRelFacing >> d_.worldPosition >> d_.worldFacing >> d_.pitch >> d_.moveFlags;
  return s_;
}

CDataStore &operator<<(CDataStore &packet, const CClientMoveUpdate &update) {
  packet << update.status;
  packet << update.timeFallen << update.walkSpeed << update.runSpeed << update.swimSpeed << update.turnRate;
  if (update.status.moveFlags & 0x04000000) {
    packet << update.spline;
  }
  return packet;
}

CDataStore &operator>>(CDataStore &packet, CClientMoveUpdate &update) {
  packet >> update.status;
  packet >> update.timeFallen >> update.walkSpeed >> update.runSpeed >> update.swimSpeed >> update.turnRate;
  if (update.status.moveFlags & 0x04000000) {
    packet >> update.spline;
  }
  return packet;
}

bool IsAngleWithinRange(float a, float b, float fieldofView) {
  fieldofView = fabsf(fieldofView);

  while (a < 0.0f) {
    a += TWO_PI;
  }
  while (b < 0.0f) {
    b += TWO_PI;
  }

  a = fmod(a, TWO_PI);
  b = fmod(b, TWO_PI);

  if (fabsf(a - b) < fieldofView) {
    return 1;
  }

  if (a < b) {
    a += TWO_PI;
  } else {
    b += TWO_PI;
  }

  return fabsf(a - b) < fieldofView;
}

float CalculateFacingTo(const NTempest::C3Vector &position, const NTempest::C3Vector &destination) {
  NTempest::C3Vector diff = destination - position;
  if (fabsf(diff.x) < 2.3841858e-7f) {
    return (diff.y < 0.0f ? 1.5f : 0.5f) * PI;
  }

  if (fabsf(diff.y) < 2.3841858e-7f) {
    return destination.x < position.x ? PI : 0.0f;
  }

  return atan2(diff.y, diff.x);
}
