#include <Base/Base.h>

#include "c3spline.h"

#include "Tempest/c2vector.h"
#include "Tempest/c34matrix.h"
#include "Tempest/c44matrix.h"
#include "Tempest/cimvector.h"
#include "Tempest/cmath.h"

#include <math.h>

namespace NTempest {

  void C3Spline::ValidateCache() const {
    IValidateCache();
    cachedLength = ILength();
  }

  void C3Spline::SetPoint(UINT pointSub, const C3Vector &point) {
    points[pointSub] = point;
    ValidateCache();
  }

  void C3Spline::SetPoints(const C3Vector *pts, UINT count) {
    ISetPoints(pts, count);
    ValidateCache();
  }

  void C3Spline::Pos(float t, C3Vector &pos, EvalType ptype) const {
    if (t <= 0.0f) {
      pos = points[0];
    } else if (t >= 1.0f) {
      pos = points[points.Count() - 1];
    } else {
      switch (ptype) {
        case EVAL_ARCLENGTH:
          IPosArclength(t, pos);
          break;
        case EVAL_PARAMETRIC:
          IPosParametric(t, pos);
          break;
      }
    }
  }

  void C3Spline::Vel(float t, C3Vector &vel, EvalType ptype) const {
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    switch (ptype) {
      case EVAL_ARCLENGTH:
        IVelArclength(t, vel);
        break;
      case EVAL_PARAMETRIC:
        IVelParametric(t, vel);
        break;
    }
  }

  void C3Spline::Frame(float t, C34Matrix &frame, EvalType ptype) const {
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    switch (ptype) {
      case EVAL_ARCLENGTH:
        IFrameArclength(t, frame);
        break;
      case EVAL_PARAMETRIC:
        ASSERT(0);
        break;
    }
  }

  void C3Spline::ISetPoints(const C3Vector *pts, UINT count) {
    points.SetCount(count);
    memcpy(points.Ptr(), pts, count * sizeof(C3Vector));
  }

}  // namespace NTempest

static float EvaluatePolynomial(UINT degree, float t, const float *coefficients) {
  float result = coefficients[0];
  for (UINT i = 1; i <= degree; ++i) {
    result = result * t + coefficients[i];
  }
  return result;
}

namespace NTempest {

  void C3Spline::Evaluate(UINT segment, float t, const C44Matrix &coeffs, C3Vector &pos) const {
    pos.x = 0.0f;
    pos.y = 0.0f;
    pos.z = 0.0f;
    for (UINT i = 0; i < 4; ++i) {
      pos += points[segment + i] * EvaluatePolynomial(3, t, coeffs[i]);
    }
  }

  void C3Spline::EvaluateDer1(UINT segment, float t, const C34Matrix &coeffs, C3Vector &der) const {
    der.x = 0.0f;
    der.y = 0.0f;
    der.z = 0.0f;
    for (UINT i = 0; i < 4; ++i) {
      der += points[segment + i] * EvaluatePolynomial(2, t, coeffs[i]);
    }
  }

  void C3Spline::EvaluateDer2(UINT segment, float t, const C24Matrix &coeffs, C3Vector &der) const {
    der.x = 0.0f;
    der.y = 0.0f;
    der.z = 0.0f;
    for (UINT i = 0; i < 4; ++i) {
      der += points[segment + i] * EvaluatePolynomial(1, t, coeffs[i]);
    }
  }

  void C3Spline::Curvature(UINT segment, float t, const C34Matrix &der1coeffs, const C24Matrix &der2coeffs, C3Vector &centerOfCurvature) const {
    C3Vector velocity(0.0f);
    C3Vector acceleration(0.0f);
    EvaluateDer1(segment, t, der1coeffs, velocity);
    EvaluateDer2(segment, t, der2coeffs, acceleration);
    float    speed = velocity.Mag();
    C3Vector curveVec = C3Vector::Cross(velocity, acceleration);
    float    speedCubed = speed * speed * speed;
    centerOfCurvature = C3Vector(speedCubed / curveVec.x, speedCubed / curveVec.y, speedCubed / curveVec.z);
  }

  float C3Spline::SegLength(UINT segment, const C44Matrix &coeffs) const {
    float    t = 0.05f;
    float    length = 0.0f;
    C3Vector curPos;
    C3Vector nextPos;
    Evaluate(segment, 0.0f, coeffs, curPos);
    for (UINT i = 0; i < 20; ++i) {
      Evaluate(segment, t, coeffs, nextPos);
      float x = nextPos.x - curPos.x;
      float y = nextPos.y - curPos.y;
      float z = nextPos.z - curPos.z;
      length += CMath::sqrt_(x * x + y * y + z * z);
      curPos = nextPos;
      t += 0.05f;
    }
    return length;
  }

  float C3Spline::ILength(UINT segmentCount) const {
    float length = 0.0f;
    for (UINT i = 0; i < segmentCount; ++i) {
      length += cachedSegLength[i];
    }
    return length;
  }

  void C3Spline::ArclengthSegT(float s, const C44Matrix &coeffs, UINT segCount, UINT &seg, float &t) const {
    if (segCount == 1) {
      seg = 0;
      t = s;
      return;
    }
    float tLength = cachedLength * s;
    float totLength = 0.0f;
    UINT  segCountm1 = segCount - 1;
    seg = 0;
    do {
      if (tLength < totLength + cachedSegLength[seg]) {
        break;
      }
      totLength += cachedSegLength[seg];
      ++seg;
    } while (seg < segCountm1);
    ASSERT(cachedSegLength[seg] > 0.0f);
    t = (tLength - totLength) / cachedSegLength[seg];
  }

  void C3Spline::ParametricSegT(float wholeT, UINT segCount, UINT &segment, float &t) const {
    float tPerSeg;
    float fSegCount = static_cast<float>(segCount);
    tPerSeg = 1.0f / fSegCount;
    segment = CMath::ftol_0_256_(fSegCount * wholeT);
    t = (wholeT - static_cast<float>(segment) * tPerSeg) * fSegCount;
  }

  static C44Matrix s_bezierCoeffs(-1.0f, 3.0f, -3.0f, 1.0f, 3.0f, -6.0f, 3.0f, 0.0f, -3.0f, 3.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);

  void C3Spline_Bezier3::ParametricSegT(float wholeT, UINT &segment, float &t) const {
    C3Spline::ParametricSegT(wholeT, points.Count() / 3, segment, t);
  }

  void C3Spline_Bezier3::ArclengthSegT(float s, UINT &seg, float &t) const {
    C3Spline::ArclengthSegT(s, s_bezierCoeffs, points.Count() / 3, seg, t);
  }

  void C3Spline_Bezier3::IPosParametric(float t, C3Vector &pos) const {
    UINT  segment;
    float segmentT;
    ParametricSegT(t, segment, segmentT);
    Evaluate(segment, segmentT, pos);
  }

  void C3Spline_Bezier3::IPosArclength(float t, C3Vector &pos) const {
    UINT  segment;
    float segmentT;
    ArclengthSegT(t, segment, segmentT);
    Evaluate(segment, segmentT, pos);
  }

  void C3Spline_Bezier3::IVelParametric(float t, C3Vector &vel) const {
    UINT  segment;
    float segmentT;
    ParametricSegT(t, segment, segmentT);
    EvaluateDer1(segment, segmentT, vel);
  }

  void C3Spline_Bezier3::IVelArclength(float t, C3Vector &vel) const {
    UINT  segment;
    float segmentT;
    ArclengthSegT(t, segment, segmentT);
    EvaluateDer1(segment, segmentT, vel);
  }

  void C3Spline_Bezier3::IFrameArclength(float t, C34Matrix &frame) const {
    UINT  segment;
    float segt;
    ArclengthSegT(t, segment, segt);
    Evaluate(segment, segt, *frame.Row3AsVec3());

    C3Vector newFacing;
    EvaluateDer1(segment, segt, newFacing);
    float magnitude = newFacing.Mag();
    if (!(CMath::fabs_(magnitude) < 0.01f)) {
      newFacing *= 1.0f / magnitude;
      *frame.Row0AsVec3() = newFacing;
    }

    frame.Row1AsVec3()->Set(-frame.a1, frame.a0, 0.0f);
    C2Vector &binormal = *reinterpret_cast<C2Vector *>(frame.Row1AsVec3());
    binormal /= binormal.Mag();
    *frame.Row2AsVec3() = C3Vector::Cross(*frame.Row0AsVec3(), binormal);
  }

  void C3Spline_Bezier3::EvaluateDer1(UINT segment, float t, C3Vector &der) const {
    static C34Matrix s_bezierDer1Coeffs(-3.0f, 6.0f, -3.0f, 9.0f, -12.0f, 3.0f, -9.0f, 6.0f, 0.0f, 3.0f, 0.0f, 0.0f);
    C3Spline::EvaluateDer1(segment * 3, t, s_bezierDer1Coeffs, der);
  }

  void C3Spline_Bezier3::Evaluate(UINT segment, float t, C3Vector &pos) const {
    C3Spline::Evaluate(segment * 3, t, s_bezierCoeffs, pos);
  }

  float C3Spline_Bezier3::SegLength(UINT segment) const {
    return C3Spline::SegLength(segment * 3, s_bezierCoeffs);
  }

  float C3Spline_Bezier3::ILength() const {
    return C3Spline::ILength(points.Count() / 3);
  }

  void C3Spline_Bezier3::IValidateCache() const {
    UINT segCount = points.Count() / 3;
    for (UINT i = 0; i < segCount; ++i) {
      cachedSegLength[i] = SegLength(i);
    }
  }

  void C3Spline_Bezier3::ISetPoints(const C3Vector *pts, UINT count) {
    UINT segmentCount = count / 3;
    UINT nPoints = (segmentCount > 0) * 4 + (segmentCount - 1) * 3;
    ASSERT(nPoints <= count);
    cachedSegLength.SetCount(segmentCount);
    C3Spline::ISetPoints(pts, nPoints);
  }

  static C44Matrix s_catmullRomCoeffs(-0.5f, 1.0f, -0.5f, 0.0f, 1.5f, -2.5f, 0.0f, 1.0f, -1.5f, 2.0f, 0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.0f);
  static C34Matrix s_catmullRomDer1Coeffs(-1.5f, 2.0f, -0.5f, 4.5f, -5.0f, 0.0f, -4.5f, 4.0f, 0.5f, 1.5f, -1.0f, 0.0f);
  static C24Matrix s_catmullRomDer2Coeffs(-3.0f, 2.0f, 9.0f, -5.0f, -9.0f, 4.0f, 3.0f, -1.0f);

  void C3Spline_CatmullRom::Evaluate(UINT segment, float t, C3Vector &pos) const {
    if (splineMode == MODE_LINEAR) {
      pos = points[segment + 1] + (points[segment + 2] - points[segment + 1]) * t;
    } else {
      C3Spline::Evaluate(segment, t, s_catmullRomCoeffs, pos);
    }
  }

  void C3Spline_CatmullRom::EvaluateDer1(UINT segment, float t, C3Vector &der) const {
    C3Spline::EvaluateDer1(segment, t, s_catmullRomDer1Coeffs, der);
  }

  void C3Spline_CatmullRom::EvaluateDer2(UINT segment, float t, C3Vector &der) const {
    C3Spline::EvaluateDer2(segment, t, s_catmullRomDer2Coeffs, der);
  }

  void C3Spline_CatmullRom::Curvature(float t, C3Vector &centerOfCurvature) const {
    UINT segment;
    ArclengthSegT(t, segment, t);
    C3Spline::Curvature(segment, t, s_catmullRomDer1Coeffs, s_catmullRomDer2Coeffs, centerOfCurvature);
  }

  float C3Spline_CatmullRom::SegLength(UINT segment) const {
    return C3Spline::SegLength(segment, s_catmullRomCoeffs);
  }

  void C3Spline_CatmullRom::ArclengthSegT(float s, UINT &seg, float &t) const {
    C3Spline::ArclengthSegT(s, s_catmullRomCoeffs, points.Count() - 3, seg, t);
  }

  void C3Spline_CatmullRom::ParametricSegT(float wholeT, UINT &segment, float &t) const {
    C3Spline::ParametricSegT(wholeT, points.Count() - 3, segment, t);
  }

  float C3Spline_CatmullRom::ILength() const {
    return C3Spline::ILength(points.Count() - 3);
  }

  void C3Spline_CatmullRom::IValidateCache() const {
    UINT segCount = points.Count() - 3;
    for (UINT i = 0; i < segCount; ++i) {
      cachedSegLength[i] = SegLength(i);
    }
  }

  void C3Spline_CatmullRom::IPosArclength(float t, C3Vector &pos) const {
    UINT  segment;
    float segmentT;
    ArclengthSegT(t, segment, segmentT);
    Evaluate(segment, segmentT, pos);
  }

  void C3Spline_CatmullRom::IPosParametric(float t, C3Vector &pos) const {
    UINT  segment;
    float segmentT;
    ParametricSegT(t, segment, segmentT);
    Evaluate(segment, segmentT, pos);
  }

  void C3Spline_CatmullRom::IVelArclength(float t, C3Vector &vel) const {
    UINT  segment;
    float segt = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    ArclengthSegT(segt, segment, segt);
    EvaluateDer1(segment, segt, vel);
  }

  void C3Spline_CatmullRom::IVelParametric(float t, C3Vector &vel) const {
    UINT  segment;
    float segt = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    ParametricSegT(segt, segment, segt);
    EvaluateDer1(segment, segt, vel);
  }

  void C3Spline_CatmullRom::IFrameArclength(float t, C34Matrix &frame) const {
    UINT  segment;
    float segt;
    ArclengthSegT(t, segment, segt);
    Evaluate(segment, segt, *reinterpret_cast<C3Vector *>(&frame.d0));

    C3Vector linearFacing = points[segment + 2] - points[segment + 1];
    float    magnitude = linearFacing.Mag();
    if (CMath::fabs_(magnitude) >= 0.00000023841858f) {
      linearFacing *= 1.0f / magnitude;
    }

    if (splineMode != MODE_LINEAR) {
      C3Vector newFacing;
      EvaluateDer1(segment, segt, newFacing);
      magnitude = newFacing.Mag();
      if (CMath::fabs_(magnitude) >= 0.01f) {
        newFacing *= 1.0f / magnitude;
        if (C3Vector::Dot(newFacing, linearFacing) >= 0.5f) {
          frame.a0 = newFacing.x;
          frame.a1 = newFacing.y;
          frame.a2 = newFacing.z;
        } else {
          frame.a0 = linearFacing.x;
          frame.a1 = linearFacing.y;
          frame.a2 = linearFacing.z;
        }
      }
    } else {
      frame.a0 = linearFacing.x;
      frame.a1 = linearFacing.y;
      frame.a2 = linearFacing.z;
    }

    frame.b0 = -frame.a1;
    frame.b1 = frame.a0;
    frame.b2 = 0.0f;
    C2Vector &binormal = *reinterpret_cast<C2Vector *>(&frame.b0);
    magnitude = binormal.Mag();
    if (CMath::fabs_(magnitude) >= 0.00000023841858f) {
      binormal /= magnitude;
    }

    frame.c0 = -frame.b1 * frame.a2;
    frame.c1 = frame.b0 * frame.a2;
    frame.c2 = frame.b1 * frame.a0 - frame.b0 * frame.a1;
  }

  void C3Spline_CatmullRom::ISetPoints(const C3Vector *pts, UINT count) {
    FATALASSERT(count > 3);
    cachedSegLength.SetCount(count - 3);
    C3Spline::ISetPoints(pts, count);
  }

}  // namespace NTempest
