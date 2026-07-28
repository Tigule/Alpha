#include "Object.h"

#include "Base/CDataStore.h"
#include "Base/UnrealConstants.h"
#include "Os/OsTime.h"

#include <windows.h>

#include <float.h>
#include <math.h>

void CClientMoveUpdate::Skip(CDataStore *packet) {
  void *unused;
  packet->GetDataInSitu(unused, 44);
  unsigned int flags = 0;
  packet->Get(flags);
  packet->GetDataInSitu(unused, 20);
  if (flags & 0x04000000) {
    flags = 0;
    packet->Get(flags);
    unsigned int bytes = 0;
    if (flags & 0x00010000) {
      bytes = 12;
    }
    if (flags & 0x00020000) {
      bytes += 8;
    }
    if (flags & 0x00040000) {
      bytes += 4;
    }
    packet->GetDataInSitu(unused, bytes + 8);
    unsigned int pointCount = 0;
    packet->Get(pointCount);
    packet->GetDataInSitu(unused, 12 * pointCount);
  }
}

namespace NTempest {

  CDataStore &operator<<(CDataStore &s_, const C3Vector &d_) {
    s_ << d_.x;
    s_ << d_.y;
    s_ << d_.z;
    return s_;
  }

}  // namespace NTempest

CDataStore &operator<<(CDataStore &packet, const CClientMoveUpdate &update) {
  packet << update.status.transport << update.status.transRelPosition.x << update.status.transRelPosition.y << update.status.transRelPosition.z
         << update.status.transRelFacing;
  packet << update.status.worldPosition;
  packet << update.status.worldFacing << update.status.pitch << update.status.moveFlags << update.timeFallen << update.walkSpeed << update.runSpeed
         << update.swimSpeed << update.turnRate;

  if (update.status.moveFlags & 0x04000000) {
    packet << update.spline.flags;
    if (update.spline.flags & 0x00010000) {
      packet << update.spline.face.spot.x << update.spline.face.spot.y << update.spline.face.spot.z;
    }
    if (update.spline.flags & 0x00020000) {
      packet << update.spline.face.guid;
    }
    if (update.spline.flags & 0x00040000) {
      packet << update.spline.face.facing;
    }
    packet << static_cast<unsigned long>(OsGetAsyncTimeMs() - update.spline.start) << update.spline.time;
    unsigned int pointCount = update.spline.spline.NumPoints();
    packet << pointCount;
    for (unsigned int i = 0; i < pointCount; ++i) {
      const NTempest::C3Vector &point = update.spline.spline.Point(i);
      packet << point.x << point.y << point.z;
    }
  }

  return packet;
}

CDataStore &operator>>(CDataStore &packet, CClientMoveUpdate &update) {
  packet.Get(update.status.transport);
  packet.Get(update.status.transRelPosition.x);
  packet.Get(update.status.transRelPosition.y);
  packet.Get(update.status.transRelPosition.z);
  packet.Get(update.status.transRelFacing);
  packet.Get(update.status.worldPosition.x);
  packet.Get(update.status.worldPosition.y);
  packet.Get(update.status.worldPosition.z);
  packet.Get(update.status.worldFacing);
  packet.Get(update.status.pitch);
  packet.Get(update.status.moveFlags);
  packet.Get(update.timeFallen);
  packet.Get(update.walkSpeed);
  packet.Get(update.runSpeed);
  packet.Get(update.swimSpeed);
  packet.Get(update.turnRate);

  if (update.status.moveFlags & 0x04000000) {
    packet.Get(update.spline.flags);
    if (update.spline.flags & 0x00010000) {
      packet.Get(update.spline.face.spot.x);
      packet.Get(update.spline.face.spot.y);
      packet.Get(update.spline.face.spot.z);
    }
    if (update.spline.flags & 0x00020000) {
      packet.Get(update.spline.face.guid);
    }
    if (update.spline.flags & 0x00040000) {
      packet.Get(update.spline.face.facing);
    }
    unsigned long elapsed;
    packet.Get(elapsed);
    update.spline.start = OsGetAsyncTimeMs() - elapsed;
    packet.Get(update.spline.time);
    unsigned int pointCount = 0;
    packet.Get(pointCount);
    if (pointCount) {
      void *points;
      packet.GetDataInSitu(points, 12 * pointCount);
      update.spline.spline.SetPoints(static_cast<const NTempest::C3Vector *>(points), pointCount);
    }
  }

  return packet;
}

bool IsAngleWithinRange(float a, float b, float fieldofView) {
  fieldofView = static_cast<float>(fabs(fieldofView));
  while (a < 0.0f) {
    a += TWO_PI;
  }
  while (b < 0.0f) {
    b += TWO_PI;
  }
  a = static_cast<float>(fmod(a, TWO_PI));
  b = static_cast<float>(fmod(b, TWO_PI));
  if (fabs(a - b) < fieldofView) {
    return 1;
  }
  if (a >= b) {
    b += TWO_PI;
  } else {
    a += TWO_PI;
  }
  return fabs(a - b) < fieldofView;
}

float CalculateFacingTo(const NTempest::C3Vector &position, const NTempest::C3Vector &destination) {
  float diffX = destination.x - position.x;
  float diffY = destination.y - position.y;

  if (fabs(diffX) >= 2.3841858e-7f) {
    if (fabs(diffY) >= 2.3841858e-7f) {
      return static_cast<float>(atan2(diffY, diffX));
    }
    return destination.x >= position.x ? 0.0f : PI;
  }

  return diffY >= 0.0f ? 0.5f * PI : 1.5f * PI;
}
