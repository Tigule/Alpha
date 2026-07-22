#include "c3spline.h"

#include "Tempest/c34matrix.h"
#include "Tempest/c44matrix.h"
#include "Tempest/cmath.h"

#include <math.h>

namespace NTempest {

  void C3Spline::SetPoints(const C3Vector *pts, unsigned int count) {
    ISetPoints(pts, count);
    IValidateCache();
    cachedLength = ILength();
  }

  void C3Spline::Pos(float t, C3Vector &pos, EvalType ptype) const {
    ASSERT(points.Count());
    if (t <= 0.0f) {
      pos = points[0];
    } else if (t >= 1.0f) {
      pos = points[points.Count() - 1];
    } else if (ptype == EVAL_ARCLENGTH) {
      IPosArclength(t, pos);
    } else {
      IPosParametric(t, pos);
    }
  }

  void C3Spline::Vel(float t, C3Vector &vel, EvalType ptype) const {
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    if (ptype == EVAL_ARCLENGTH) {
      IVelArclength(t, vel);
    } else {
      IVelParametric(t, vel);
    }
  }

  void C3Spline::ISetPoints(const C3Vector *pts, unsigned int count) {
    points.SetCount(count);
    for (unsigned int i = 0; i < count; ++i) {
      points[i] = pts[i];
    }
  }

}  // namespace NTempest

static float __fastcall EvaluatePolynomial(unsigned int degree, float t, const float *coefficients) {
  float result = coefficients[0];
  for (unsigned int i = 1; i <= degree; ++i) {
    result = result * t + coefficients[i];
  }
  return result;
}

namespace NTempest {

  void C3Spline::Evaluate(unsigned int segment, float t, const C44Matrix &coeffs, C3Vector &pos) const {
    pos.x = 0.0f;
    pos.y = 0.0f;
    pos.z = 0.0f;
    const float *coefficient = &coeffs.a0;
    for (unsigned int i = 0; i < 4; ++i, coefficient += 4) {
      float           weight = EvaluatePolynomial(3, t, coefficient);
      const C3Vector &point = points[segment + i];
      pos.x += weight * point.x;
      pos.y += weight * point.y;
      pos.z += weight * point.z;
    }
  }

  void C3Spline::EvaluateDer1(unsigned int segment, float t, const C34Matrix &coeffs, C3Vector &der) const {
    der.x = 0.0f;
    der.y = 0.0f;
    der.z = 0.0f;
    const float *coefficient = &coeffs.a0;
    for (unsigned int i = 0; i < 4; ++i, coefficient += 3) {
      float           weight = EvaluatePolynomial(2, t, coefficient);
      const C3Vector &point = points[segment + i];
      der.x += weight * point.x;
      der.y += weight * point.y;
      der.z += weight * point.z;
    }
  }

  float C3Spline::SegLength(unsigned int segment, const C44Matrix &coeffs) const {
    C3Vector curPos;
    C3Vector nextPos;
    float    length = 0.0f;
    float    t = 0.0f;
    Evaluate(segment, t, coeffs, curPos);
    for (unsigned int i = 0; i < 20; ++i) {
      t += 0.05f;
      Evaluate(segment, t, coeffs, nextPos);
      float x = nextPos.x - curPos.x;
      float y = nextPos.y - curPos.y;
      float z = nextPos.z - curPos.z;
      length += CMath::sqrt_(x * x + y * y + z * z);
      curPos = nextPos;
    }
    return length;
  }

  void C3Spline::ArclengthSegT(float s, const C44Matrix &coeffs, unsigned int segCount, unsigned int &seg, float &t) const {
    if (segCount == 1) {
      seg = 0;
      t = s;
      return;
    }
    float target = cachedLength * s;
    float accumulated = 0.0f;
    seg = 0;
    while (seg + 1 < segCount && target >= accumulated + cachedSegLength[seg]) {
      accumulated += cachedSegLength[seg++];
    }
    ASSERT(cachedSegLength[seg] > 0.0f);
    t = (target - accumulated) / cachedSegLength[seg];
  }

  void C3Spline::ParametricSegT(float wholeT, unsigned int segCount, unsigned int &segment, float &t) const {
    float tPerSeg;
    float fSegCount = static_cast<float>(segCount);
    segment = CMath::ftol_0_256_(fSegCount * wholeT);
    tPerSeg = 1.0f / fSegCount;
    t = (wholeT - static_cast<float>(segment) * tPerSeg) * fSegCount;
  }

  static C44Matrix s_bezierCoeffs(-1.0f, 3.0f, -3.0f, 1.0f, 3.0f, -6.0f, 3.0f, 0.0f, -3.0f, 3.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);

  void C3Spline_Bezier3::ParametricSegT(float wholeT, unsigned int &segment, float &t) const {
    C3Spline::ParametricSegT(wholeT, points.Count() / 3, segment, t);
  }

  void C3Spline_Bezier3::ArclengthSegT(float s, unsigned int &seg, float &t) const {
    C3Spline::ArclengthSegT(s, s_bezierCoeffs, points.Count() / 3, seg, t);
  }

  void C3Spline_Bezier3::IPosParametric(float t, C3Vector &pos) const {
    unsigned int segment;
    float        segmentT;
    ParametricSegT(t, segment, segmentT);
    Evaluate(segment, segmentT, pos);
  }

  void C3Spline_Bezier3::IPosArclength(float t, C3Vector &pos) const {
    unsigned int segment;
    float        segmentT;
    ArclengthSegT(t, segment, segmentT);
    Evaluate(segment, segmentT, pos);
  }

  void C3Spline_Bezier3::IVelParametric(float t, C3Vector &vel) const {
    unsigned int segment;
    float        segmentT;
    ParametricSegT(t, segment, segmentT);
    EvaluateDer1(segment, segmentT, vel);
  }

  void C3Spline_Bezier3::IVelArclength(float t, C3Vector &vel) const {
    unsigned int segment;
    float        segmentT;
    ArclengthSegT(t, segment, segmentT);
    EvaluateDer1(segment, segmentT, vel);
  }

  void C3Spline_Bezier3::IFrameArclength(float t, C34Matrix &frame) const {
    IPosArclength(t, *reinterpret_cast<C3Vector *>(&frame.d0));
    C3Vector facing;
    IVelArclength(t, facing);
    if (facing.SquaredMag() >= 0.0001f) {
      facing.Normalize();
    }
    frame.a0 = facing.x;
    frame.a1 = facing.y;
    frame.a2 = facing.z;
    C3Vector side(-facing.y, facing.x, 0.0f);
    side.Normalize();
    frame.b0 = side.x;
    frame.b1 = side.y;
    frame.b2 = side.z;
    frame.c0 = -side.y * facing.z;
    frame.c1 = side.x * facing.z;
    frame.c2 = side.y * facing.x - side.x * facing.y;
  }

  void C3Spline_Bezier3::EvaluateDer1(unsigned int segment, float t, C3Vector &der) const {
    static C34Matrix s_bezierDer1Coeffs(-3.0f, 6.0f, -3.0f, 9.0f, -12.0f, 3.0f, -9.0f, 6.0f, 0.0f, 3.0f, 0.0f, 0.0f);
    C3Spline::EvaluateDer1(segment * 3, t, s_bezierDer1Coeffs, der);
  }

  void C3Spline_Bezier3::Evaluate(unsigned int segment, float t, C3Vector &pos) const {
    C3Spline::Evaluate(segment * 3, t, s_bezierCoeffs, pos);
  }

  float C3Spline_Bezier3::SegLength(unsigned int segment) const {
    return C3Spline::SegLength(segment * 3, s_bezierCoeffs);
  }

  float C3Spline_Bezier3::ILength() const {
    float length = 0.0f;
    for (unsigned int i = 0; i < cachedSegLength.Count(); ++i) {
      length += cachedSegLength[i];
    }
    return length;
  }

  void C3Spline_Bezier3::IValidateCache() const {
    unsigned int segmentCount = points.Count() / 3;
    const_cast<TSGrowableArray<float> &>(cachedSegLength).SetCount(segmentCount);
    for (unsigned int i = 0; i < segmentCount; ++i) {
      cachedSegLength[i] = SegLength(i);
    }
  }

  void C3Spline_Bezier3::ISetPoints(const C3Vector *pts, unsigned int count) {
    unsigned int segmentCount = count / 3;
    unsigned int pointCount = segmentCount * 3 + (segmentCount != 0 ? 1 : 0);
    ASSERT(pointCount <= count);
    cachedSegLength.SetCount(segmentCount);
    C3Spline::ISetPoints(pts, pointCount);
  }

  static C44Matrix s_catmullRomCoeffs(-0.5f, 1.5f, -1.5f, 0.5f, 1.0f, -2.5f, 2.0f, -0.5f, -0.5f, 0.0f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);

  void C3Spline_CatmullRom::Evaluate(unsigned int segment, float t, C3Vector &pos) const {
    if (splineMode == SPLINE_MODE_CATMULLROM) {
      C3Spline::Evaluate(segment, t, s_catmullRomCoeffs, pos);
    } else {
      const C3Vector &start = points[segment + 1];
      const C3Vector &end = points[segment + 2];
      pos.x = start.x + (end.x - start.x) * t;
      pos.y = start.y + (end.y - start.y) * t;
      pos.z = start.z + (end.z - start.z) * t;
    }
  }

  void C3Spline_CatmullRom::EvaluateDer1(unsigned int segment, float t, C3Vector &der) const {
    static C34Matrix s_catmullRomDer1Coeffs(-1.5f, 3.0f, -1.5f, 3.0f, -5.0f, 2.0f, -1.5f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f);
    C3Spline::EvaluateDer1(segment, t, s_catmullRomDer1Coeffs, der);
  }

  float C3Spline_CatmullRom::SegLength(unsigned int segment) const {
    return C3Spline::SegLength(segment, s_catmullRomCoeffs);
  }

  void C3Spline_CatmullRom::ArclengthSegT(float s, unsigned int &seg, float &t) const {
    C3Spline::ArclengthSegT(s, s_catmullRomCoeffs, points.Count() - 3, seg, t);
  }

  void C3Spline_CatmullRom::ParametricSegT(float wholeT, unsigned int &segment, float &t) const {
    C3Spline::ParametricSegT(wholeT, points.Count() - 3, segment, t);
  }

  float C3Spline_CatmullRom::ILength() const {
    float length = 0.0f;
    for (unsigned int i = 0; i < cachedSegLength.Count(); ++i) {
      length += cachedSegLength[i];
    }
    return length;
  }

  void C3Spline_CatmullRom::IValidateCache() const {
    unsigned int segmentCount = points.Count() - 3;
    const_cast<TSGrowableArray<float> &>(cachedSegLength).SetCount(segmentCount);
    for (unsigned int i = 0; i < segmentCount; ++i) {
      cachedSegLength[i] = SegLength(i);
    }
  }

  void C3Spline_CatmullRom::IPosArclength(float t, C3Vector &pos) const {
    unsigned int segment;
    float        segmentT;
    ArclengthSegT(t, segment, segmentT);
    Evaluate(segment, segmentT, pos);
  }

  void C3Spline_CatmullRom::IPosParametric(float t, C3Vector &pos) const {
    unsigned int segment;
    float        segmentT;
    ParametricSegT(t, segment, segmentT);
    Evaluate(segment, segmentT, pos);
  }

  void C3Spline_CatmullRom::IVelArclength(float t, C3Vector &vel) const {
    unsigned int segment;
    float        segmentT;
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    ArclengthSegT(t, segment, segmentT);
    EvaluateDer1(segment, segmentT, vel);
  }

  void C3Spline_CatmullRom::IVelParametric(float t, C3Vector &vel) const {
    unsigned int segment;
    float        segmentT;
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    ParametricSegT(t, segment, segmentT);
    EvaluateDer1(segment, segmentT, vel);
  }

  void C3Spline_CatmullRom::IFrameArclength(float t, C34Matrix &frame) const {
    IPosArclength(t, *reinterpret_cast<C3Vector *>(&frame.d0));
    C3Vector facing;
    IVelArclength(t, facing);
    if (facing.SquaredMag() >= 0.0001f) {
      facing.Normalize();
    }
    frame.a0 = facing.x;
    frame.a1 = facing.y;
    frame.a2 = facing.z;
    C3Vector side(-facing.y, facing.x, 0.0f);
    side.Normalize();
    frame.b0 = side.x;
    frame.b1 = side.y;
    frame.b2 = side.z;
    frame.c0 = -side.y * facing.z;
    frame.c1 = side.x * facing.z;
    frame.c2 = side.y * facing.x - side.x * facing.y;
  }

  void C3Spline_CatmullRom::ISetPoints(const C3Vector *pts, unsigned int count) {
    FATALASSERT(count > 3);
    cachedSegLength.SetCount(count - 3);
    C3Spline::ISetPoints(pts, count);
  }

}  // namespace NTempest
