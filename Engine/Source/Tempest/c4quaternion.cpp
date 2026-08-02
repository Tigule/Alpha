#include <Base/Base.h>

#include "Tempest/c4quaternion.h"

#include "Tempest/c33matrix.h"
#include "Tempest/cmath.h"

#include <math.h>

namespace NTempest {

  static const unsigned long next[3] = {1, 2, 0};

  void C4Quaternion::FromRotationMatrix(const C33Matrix &rotation) {
    FromRotationMatrixInv(C33Matrix(rotation).Transpose());
  }

  void C4Quaternion::FromRotationMatrixInv(const C33Matrix &rotation) {
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

    long i = rotation.b1 > rotation.a0;
    if (rotation.c2 > matrix[4 * i]) {
      i = 2;
    }

    long   j = next[i];
    long   k = next[j];
    float  root = CMath::sqrt_(matrix[4 * i] - matrix[4 * j] - matrix[4 * k] + 1.0f);
    float *q_[3] = {&x, &y, &z};
    *q_[i] = 0.5f * root;
    root = 0.5f / root;
    w = (matrix[3 * k + j] - matrix[3 * j + k]) * root;
    *q_[j] = (matrix[3 * i + j] + matrix[3 * j + i]) * root;
    *q_[k] = (matrix[3 * i + k] + matrix[3 * k + i]) * root;
  }

  void C4Quaternion::FromAngleAxis(
      const float angle,
      const C3Vector &axis
  ) {
    ASSERT(CMath::fequal_(axis.Mag(), 1.0f));
    float halfAngle = angle * 0.5f;
    float sine = CMath::sin_(halfAngle);
    w = CMath::cos_(halfAngle);
    x = sine * axis.x;
    y = sine * axis.y;
    z = sine * axis.z;
  }

  void C4Quaternion::ToAngleAxis(
      float &angle,
      C3Vector &axis
  ) const {
    float len2 = x * x + y * y + z * z;
    if (len2 > 0.0f) {
      angle = 2.0f * static_cast<float>(acos(w));
      float inverseLength = 1.0f / CMath::sqrt_(len2);
      axis.x = x * inverseLength;
      axis.y = y * inverseLength;
      axis.z = z * inverseLength;
    } else {
      angle = 0.0f;
      axis.x = 1.0f;
      axis.y = 0.0f;
      axis.z = 0.0f;
    }
  }

  C4Quaternion C4Quaternion::Inverse() const {
    float norm = x * x + y * y + z * z + w * w;
    if (CMath::fabs_(norm) < 0.00000023841858f) {
      ASSERT(!"C4Quaternion::Inverse(): cannot invert an invalid (zero-norm) quaternion.");
      return C4Quaternion();
    }
    norm = 1.0f / norm;
    return C4Quaternion(
        w * norm,
        -x * norm,
        -y * norm,
        -z * norm
    );
  }

  C4Quaternion C4Quaternion::Exp() const {
    float angle = CMath::sqrt_(x * x + y * y + z * z);
    float s = CMath::sin_(angle);
    float coeff =
        CMath::fabs_(s) < 0.00000047683716f
        ? 1.0f
        : s / angle;
    return C4Quaternion(
        CMath::cos_(angle),
        coeff * x,
        coeff * y,
        coeff * z
    );
  }

  C4Quaternion C4Quaternion::Log() const {
    float coeff = 1.0f;
    if (CMath::fabs_(w) < 1.0f) {
      float angle = static_cast<float>(acos(w));
      float sine = CMath::sin_(angle);
      if (CMath::fabs_(sine) >= 0.00000047683716f) {
        coeff = angle / sine;
      }
    }
    return C4Quaternion(
        0.0f,
        coeff * x,
        coeff * y,
        coeff * z
    );
  }

  C4Quaternion C4Quaternion::Slerp(float ratio, const C4Quaternion &start, const C4Quaternion &end) {
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

  C4Quaternion C4Quaternion::Squad(
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

  void C4Quaternion::SquadInterm(
      const C4Quaternion &q0,
      const C4Quaternion &q1,
      const C4Quaternion &q2,
      C4Quaternion &a,
      C4Quaternion &b
  ) {
    ASSERT(q0.IsUnit());
    ASSERT(q1.IsUnit());
    ASSERT(q2.IsUnit());
    C4Quaternion p0 = q0.Conjugate() * q1;
    C4Quaternion p1 = q1.Conjugate() * q2;
    C4Quaternion log0 = p0.Log();
    C4Quaternion log1 = p1.Log();
    C4Quaternion tangent(
        0.25f * (log0.w - log1.w),
        0.25f * (log0.x - log1.x),
        0.25f * (log0.y - log1.y),
        0.25f * (log0.z - log1.z)
    );
    C4Quaternion inverseTangent(
        -tangent.w, -tangent.x, -tangent.y, -tangent.z
    );
    a = q1 * tangent.Exp();
    b = q1 * inverseTangent.Exp();
  }

  void C4Quaternion::SquadIntermMaxCompat(
      const C4Quaternion &q0,
      const C4Quaternion &q1,
      const C4Quaternion &q2,
      C4Quaternion &a,
      C4Quaternion &b
  ) {
    ASSERT(q0.IsUnit());
    ASSERT(q1.IsUnit());
    ASSERT(q2.IsUnit());
    C4Quaternion p0 = q0.Conjugate() * q1;
    C4Quaternion p1 = q1.Conjugate() * q2;
    C4Quaternion log0 = p0.Log();
    C4Quaternion log1 = p1.Log();
    C4Quaternion tangent(
        0.25f * (log0.w - log1.w),
        0.25f * (log0.x - log1.x),
        0.25f * (log0.y - log1.y),
        0.25f * (log0.z - log1.z)
    );
    a = q1 * tangent.Exp();
    b = a;
  }

  void C4Quaternion::SquadIntermTCB(
      const C4Quaternion &q0,
      const C4Quaternion &q1,
      const C4Quaternion &q2,
      float time0,
      float time1,
      float time2,
      float tension,
      float continuity,
      float bias,
      C4Quaternion &a,
      C4Quaternion &b
  ) {
    C4Quaternion qm;
    C4Quaternion qp;
    if (time0 <= time1) {
      C4Quaternion prev = q0;
      if (prev.x * q1.x + prev.y * q1.y
          + prev.z * q1.z + prev.w * q1.w < 0.0f) {
        prev = C4Quaternion(
            -prev.w, -prev.x, -prev.y, -prev.z
        );
      }
      qm = (prev.Conjugate() * q1).Log();
    }
    if (time1 <= time2) {
      C4Quaternion next = q2;
      if (q1.x * next.x + q1.y * next.y
          + q1.z * next.z + q1.w * next.w < 0.0f) {
        next = C4Quaternion(-next.w, -next.x, -next.y, -next.z);
      }
      qp = (q1.Conjugate() * next).Log();
    }
    if (time0 > time1) {
      qm = qp;
    }
    if (time1 > time2) {
      qp = qm;
    }

    float adjustMinus = 1.0f;
    float adjustPlus = 1.0f;
    if (time2 > time0) {
      float inverseHalfSpan = 1.0f / ((time2 - time0) * 0.5f);
      float deltaMinus = (time1 - time0) * inverseHalfSpan;
      float deltaPlus = (time2 - time1) * inverseHalfSpan;
      float absContinuity = CMath::fabs_(continuity);
      adjustMinus =
          (1.0f - deltaMinus) * absContinuity + deltaMinus;
      adjustPlus =
          (1.0f - deltaPlus) * absContinuity + deltaPlus;
    }

    float oneMinusTension = 1.0f - tension;
    float onePlusContinuity = 1.0f + continuity;
    float oneMinusContinuity = 1.0f - continuity;
    float onePlusBias = 1.0f + bias;
    float oneMinusBias = 1.0f - bias;
    float kdMinus =
        onePlusBias * onePlusContinuity * oneMinusTension
        * adjustPlus * 0.5f;
    float ksPlus =
        oneMinusBias * oneMinusContinuity * oneMinusTension
        * adjustPlus * 0.5f - 1.0f;
    float ksMinus =
        1.0f - oneMinusContinuity * onePlusBias
        * oneMinusTension * adjustMinus * 0.5f;
    float kdPlus =
        oneMinusBias * onePlusContinuity * oneMinusTension
        * adjustMinus * -0.5f;

    C4Quaternion qa(
        0.5f * (qp.w * ksPlus + qm.w * kdMinus),
        0.5f * (qp.x * ksPlus + qm.x * kdMinus),
        0.5f * (qp.y * ksPlus + qm.y * kdMinus),
        0.5f * (qp.z * ksPlus + qm.z * kdMinus)
    );
    C4Quaternion qb(
        0.5f * (qp.w * kdPlus + qm.w * ksMinus),
        0.5f * (qp.x * kdPlus + qm.x * ksMinus),
        0.5f * (qp.y * kdPlus + qm.y * ksMinus),
        0.5f * (qp.z * kdPlus + qm.z * ksMinus)
    );
    a = q1 * qa.Exp();
    b = q1 * qb.Exp();
  }

}  // namespace NTempest
