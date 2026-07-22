#include "Tempest/c4quaternion.h"

#include "Tempest/c33matrix.h"
#include "Tempest/cmath.h"

#include <math.h>

namespace NTempest {

  static const unsigned long next[3] = {1, 2, 0};

  void C4Quaternion::FromRotationMatrix(C33Matrix &rotation) {
    FromRotationMatrixInv(rotation.Transpose());
  }

  void C4Quaternion::FromRotationMatrixInv(C33Matrix &rotation) {
    const float *matrix = &rotation.a0;
    float        trace = rotation.a0 + rotation.b1 + rotation.c2;

    if (trace > 0.0f) {
      float root = CMath::sqrt_(trace + 1.0f);
      w = 0.5f * root;
      root = 0.5f / root;
      x = (rotation.c1 - rotation.b2) * root;
      y = (rotation.a2 - rotation.c0) * root;
      z = (rotation.b0 - rotation.a1) * root;
      return;
    }

    long k = rotation.b1 > rotation.a0;
    if (rotation.c2 > matrix[4 * k]) {
      k = 2;
    }

    long   i = next[k];
    long   j = next[i];
    float  root = CMath::sqrt_(matrix[4 * k] - matrix[4 * i] - matrix[4 * j] + 1.0f);
    float *q[3] = {&x, &y, &z};
    *q[k] = 0.5f * root;
    root = 0.5f / root;
    w = (matrix[3 * j + i] - matrix[3 * i + j]) * root;
    *q[i] = (matrix[3 * k + i] + matrix[3 * i + k]) * root;
    *q[j] = (matrix[3 * k + j] + matrix[3 * j + k]) * root;
  }

  C4Quaternion __fastcall C4Quaternion::Slerp(float ratio, const C4Quaternion &start, const C4Quaternion &end) {
    float sign = 1.0f;
    float dot = start.x * end.x + start.y * end.y + start.z * end.z + start.w * end.w;
    if (dot < 0.0f) {
      sign = -1.0f;
      dot = -dot;
    }

    float sine = CMath::sqrt_(CMath::fabs_(1.0f - dot * dot));
    if (CMath::fabs_(sine) < 0.00000047683716f) {
      return start;
    }

    float angle = static_cast<float>(atan2(sine, dot));
    float startScale = CMath::sin_((1.0f - ratio) * angle) / sine;
    float endScale = CMath::sin_(ratio * angle) / sine * sign;
    return C4Quaternion(
        startScale * start.w + endScale * end.w, startScale * start.x + endScale * end.x, startScale * start.y + endScale * end.y,
        startScale * start.z + endScale * end.z
    );
  }

  C4Quaternion __fastcall C4Quaternion::Squad(
      float               ratio,
      const C4Quaternion &start,
      const C4Quaternion &end,
      const C4Quaternion &outTangent,
      const C4Quaternion &inTangent
  ) {
    C4Quaternion value = Slerp(ratio, start, end);
    C4Quaternion tangent = Slerp(ratio, outTangent, inTangent);
    return Slerp(2.0f * ratio * (1.0f - ratio), value, tangent);
  }

}  // namespace NTempest
