#include <Base/Base.h>

#include "Tempest/c4quaternion.h"

#include "Tempest/c33matrix.h"
#include "Tempest/cmath.h"

#include <math.h>

namespace NTempest {

  static const DWORD next[3] = {1, 2, 0};

  void C4Quaternion::FromRotationMatrix(const C33Matrix &r) {
    FromRotationMatrixInv(r.Transpose());
  }

  void C4Quaternion::FromRotationMatrixInv(const C33Matrix &r) {
    float trace = r.a0 + r.b1 + r.c2;

    if (trace > 0.0f) {
      float root = CMath::sqrt_(trace + 1.0f);
      w = 0.5f * root;
      root = 0.5f / root;
      x = (r.c1 - r.b2) * root;
      y = (r.a2 - r.c0) * root;
      z = (r.b0 - r.a1) * root;
    } else {
      const float (*r_)[3] = reinterpret_cast<const float (*)[3]>(&r);
      long i = 0;
      ASSERT(r_[0][0] == r.a0 && r_[1][1] == r.b1 && r_[2][2] == r.c2);
      if (r.b1 > r.a0) {
        i = 1;
      }
      if (r.c2 > r_[i][i]) {
        i = 2;
      }

      long   j = next[i];
      long   k = next[j];
      float  root = CMath::sqrt_(r_[i][i] - r_[j][j] - r_[k][k] + 1.0f);
      float *q_[3] = {&x, &y, &z};
      *q_[i] = 0.5f * root;
      root = 0.5f / root;
      w = (r_[k][j] - r_[j][k]) * root;
      *q_[j] = (r_[i][j] + r_[j][i]) * root;
      *q_[k] = (r_[i][k] + r_[k][i]) * root;
    }
  }

  void C4Quaternion::FromAngleAxis(const float angle, const C3Vector &axis) {
    float mag = axis.Mag();
    ASSERT(CMath::fequal_(mag, 1.0f));
    float halfAngle = angle * 0.5f;
    float sine = CMath::sin_(halfAngle);
    w = CMath::cos_(halfAngle);
    x = sine * axis.x;
    y = sine * axis.y;
    z = sine * axis.z;
  }

  void C4Quaternion::ToAngleAxis(float &angle, C3Vector &axis) const {
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
    if (CMath::fnotequal_(norm, 0.0f)) {
      norm = 1.0f / norm;
      return C4Quaternion(w * norm, -x * norm, -y * norm, -z * norm);
    }
    SErrDisplayError(
        STORM_ERROR_ASSERTION, __FILE__, __LINE__, "\"C4Quaternion::Inverse(): cannot invert an invalid (zero-norm) quaternion.\"", FALSE
    );
    return C4Quaternion();
  }

  C4Quaternion C4Quaternion::Exp() const {
    float angle = CMath::sqrt_(x * x + y * y + z * z);
    float s;
    float c;
    CMath::sincos_(angle, s, c);
    if (CMath::fabs_(s) >= 0.00000047683716f) {
      float coeff = s / angle;
      return C4Quaternion(c, coeff * x, coeff * y, coeff * z);
    }
    return C4Quaternion(c, x, y, z);
  }

  C4Quaternion C4Quaternion::Log() const {
    if (CMath::fabs_(w) < 1.0f) {
      float angle = static_cast<float>(acos(w));
      float sine = CMath::sin_(angle);
      if (CMath::fabs_(sine) >= 0.00000047683716f) {
        float coeff = angle / sine;
        return C4Quaternion(0.0f, coeff * x, coeff * y, coeff * z);
      }
    }
    return C4Quaternion(0.0f, x, y, z);
  }

  C4Quaternion C4Quaternion::Slerp(float t, const C4Quaternion &p, const C4Quaternion &q) {
    float sign = 1.0f;
    float c = p.x * q.x + p.y * q.y + p.z * q.z + p.w * q.w;
    if (c < 0.0f) {
      sign = -1.0f;
      c = -c;
    }

    float s = CMath::sqrt_(CMath::fabs_(1.0f - c * c));
    if (CMath::fabs_(s) < 0.00000047683716f) {
      return p;
    }

    float angle = static_cast<float>(atan2(s, c));
    float coef0 = CMath::sin_((1.0f - t) * angle) * (1.0f / s);
    float endScale = CMath::sin_(t * angle) * (1.0f / s) * sign;
    return C4Quaternion(endScale * q.w + coef0 * p.w, endScale * q.x + coef0 * p.x, endScale * q.y + coef0 * p.y, endScale * q.z + coef0 * p.z);
  }

  C4Quaternion C4Quaternion::Squad(float t, const C4Quaternion &p, const C4Quaternion &a, const C4Quaternion &b, const C4Quaternion &q) {
    return Slerp(2.0f * t * (1.0f - t), Slerp(t, p, q), Slerp(t, a, b));
  }

  void C4Quaternion::SquadInterm(const C4Quaternion &q0, const C4Quaternion &q1, const C4Quaternion &q2, C4Quaternion &a, C4Quaternion &b) {
    ASSERT(q0.IsUnit());
    ASSERT(q1.IsUnit());
    ASSERT(q2.IsUnit());
    C4Quaternion p0 = q0.UnitInverse() * q1;
    C4Quaternion p1 = q1.UnitInverse() * q2;
    C4Quaternion at = C4Quaternion(p0.Log() - p1.Log()) * 0.25f;
    C4Quaternion bt = -at;
    a = q1 * at.Exp();
    b = q1 * bt.Exp();
  }

  void C4Quaternion::SquadIntermMaxCompat(const C4Quaternion &q0, const C4Quaternion &q1, const C4Quaternion &q2, C4Quaternion &a, C4Quaternion &b) {
    ASSERT(q0.IsUnit());
    ASSERT(q1.IsUnit());
    ASSERT(q2.IsUnit());
    C4Quaternion p0 = q0.UnitInverse() * q1;
    C4Quaternion p1 = q1.UnitInverse() * q2;
    C4Quaternion at = C4Quaternion(p0.Log() - p1.Log()) * 0.25f;
    a = q1 * at.Exp();
    b = a;
  }

  void C4Quaternion::SquadIntermTCB(
      const C4Quaternion &q0,
      const C4Quaternion &q1,
      const C4Quaternion &q2,
      float               time0,
      float               time1,
      float               time2,
      float               tension,
      float               continuity,
      float               bias,
      C4Quaternion       &a,
      C4Quaternion       &b
  ) {
    C4Quaternion qm;
    C4Quaternion qp;
    if (time0 <= time1) {
      C4Quaternion prev = q0.x * q1.x + q0.y * q1.y + q0.z * q1.z + q0.w * q1.w < 0.0f ? C4Quaternion(-q0) : q0;
      C4Quaternion p0 = prev.UnitInverse() * q1;
      qm = p0.Log();
    }
    if (time1 <= time2) {
      C4Quaternion next = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w < 0.0f ? C4Quaternion(-q2) : q2;
      C4Quaternion p1 = q1.UnitInverse() * next;
      qp = p1.Log();
    }
    if (time0 > time1) {
      qm = qp;
    }
    if (time1 > time2) {
      qp = qm;
    }

    float prevRatio = 1.0f;
    float nextRatio = 1.0f;
    if (time2 > time0) {
      float inverseHalfSpan = 1.0f / ((time2 - time0) * 0.5f);
      prevRatio = (time1 - time0) * inverseHalfSpan;
      nextRatio = (time2 - time1) * inverseHalfSpan;
      float absCont = CMath::fabs_(continuity);
      prevRatio = (1.0f - prevRatio) * absCont + prevRatio;
      nextRatio = (1.0f - nextRatio) * absCont + nextRatio;
    }

    float kdm = (1.0f - tension) * (1.0f + continuity) * (1.0f + bias) * nextRatio * 0.5f;
    float kdp = (1.0f - tension) * (1.0f - continuity) * (1.0f - bias) * nextRatio * 0.5f - 1.0f;
    float ksm = 1.0f - (1.0f - tension) * (1.0f - continuity) * (1.0f + bias) * prevRatio * 0.5f;
    float ksp = (1.0f - tension) * (1.0f + continuity) * (1.0f - bias) * prevRatio * -0.5f;

    C4Quaternion qa(
        0.5f * (qp.w * kdp + qm.w * kdm), 0.5f * (qp.x * kdp + qm.x * kdm), 0.5f * (qp.y * kdp + qm.y * kdm), 0.5f * (qp.z * kdp + qm.z * kdm)
    );
    C4Quaternion qb(
        0.5f * (qp.w * ksp + qm.w * ksm), 0.5f * (qp.x * ksp + qm.x * ksm), 0.5f * (qp.y * ksp + qm.y * ksm), 0.5f * (qp.z * ksp + qm.z * ksm)
    );
    a = q1 * qa.Exp();
    b = q1 * qb.Exp();
  }

}  // namespace NTempest
