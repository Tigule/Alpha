#pragma once

#include "Tempest/c3vector.h"

#include <stpl.h>

class CSplineParticleEmitter;
class CGGameObject_C_Type_MapObjTransport;
class CGUnit_C;
class CMovement;

class C24Matrix {
 public:
  enum {
    eComponents = 8
  };

  C24Matrix() : a0(0.0f), a1(0.0f), b0(0.0f), b1(0.0f), c0(0.0f), c1(0.0f), d0(0.0f), d1(0.0f) {
  }

  C24Matrix(float value) : a0(value), a1(value), b0(value), b1(value), c0(value), c1(value), d0(value), d1(value) {
  }

  C24Matrix(float a0, float a1, float b0, float b1, float c0, float c1, float d0, float d1)
      : a0(a0), a1(a1), b0(b0), b1(b1), c0(c0), c1(c1), d0(d0), d1(d1) {
  }

  ~C24Matrix() {
  }

  const float *Access() const {
    return &a0;
  }

  float *Access() {
    return &a0;
  }

  const float *operator[](UINT row) const {
    return &a0 + row * 2;
  }

  float *operator[](UINT row) {
    return &a0 + row * 2;
  }

  float a0;
  float a1;
  float b0;
  float b1;
  float c0;
  float c1;
  float d0;
  float d1;
};

namespace NTempest {

  class C34Matrix;
  class C44Matrix;

  class C3Spline {
   public:
    enum EvalType {
      EVAL_PARAMETRIC = 0,
      EVAL_ARCLENGTH = 1,
      EVAL_COUNT = 2
    };

    enum {
      DEFAULT_STEPS = 20
    };

    UINT NumPoints() const {
      return points.Count();
    }
    const C3Vector &Point(UINT pointSub) const {
      return points[pointSub];
    }
    void SetPoints(const TSGrowableArray<C3Vector> &pts) {
      SetPoints(pts.Ptr(), pts.Count());
    }
    void  SetPoints(const C3Vector *pts, UINT count);
    void  SetPoint(UINT pointSub, const C3Vector &point);
    void  Pos(float t, C3Vector &pos, EvalType ptype) const;
    void  Vel(float t, C3Vector &vel, EvalType ptype) const;
    void  Frame(float t, C34Matrix &frame, EvalType ptype) const;
    float Length() const {
      ValidateCache();
      return cachedLength;
    }

   protected:
    friend class ::CSplineParticleEmitter;
    virtual float ILength() const = 0;
    float         ILength(UINT segmentCount) const;
    virtual void  IValidateCache() const = 0;
    virtual void  IPosArclength(float t, C3Vector &result) const = 0;
    virtual void  IPosParametric(float t, C3Vector &result) const = 0;
    virtual void  IVelArclength(float t, C3Vector &result) const = 0;
    virtual void  IVelParametric(float t, C3Vector &result) const = 0;
    virtual void  IFrameArclength(float t, C34Matrix &result) const = 0;
    virtual void  ISetPoints(const C3Vector *pts, UINT count);

    void  Evaluate(UINT segment, float t, const C44Matrix &coeffs, C3Vector &pos) const;
    void  EvaluateDer1(UINT segment, float t, const C34Matrix &coeffs, C3Vector &der) const;
    void  EvaluateDer2(UINT segment, float t, const C24Matrix &coeffs, C3Vector &der) const;
    void  Curvature(UINT segment, float t, const C34Matrix &der1coeffs, const C24Matrix &der2coeffs, C3Vector &centerOfCurvature) const;
    float SegLength(UINT segment, const C44Matrix &coeffs) const;
    void  ValidateCache() const;

    void ParametricSegT(float wholeT, UINT segCount, UINT &segment, float &t) const;
    void ArclengthSegT(float s, const C44Matrix &coeffs, UINT segCount, UINT &seg, float &t) const;

   private:
    friend class ::CGGameObject_C_Type_MapObjTransport;
    friend class ::CGUnit_C;
    friend class ::CMovement;
    friend class C3Spline_Bezier3;
    friend class C3Spline_CatmullRom;

    mutable float             cachedLength;
    TSGrowableArray<C3Vector> points;

   protected:
    mutable TSGrowableArray<float> cachedSegLength;
  };

  class C3Spline_Bezier3 : public C3Spline {
   public:
    C3Spline_Bezier3() {
    }
    C3Spline_Bezier3(const C3Vector *pts, UINT count) {
      SetPoints(pts, count);
    }
    C3Spline_Bezier3(const C3Spline_Bezier3 &spline) : C3Spline(spline) {
    }

   protected:
    virtual float ILength() const;
    virtual void  IValidateCache() const;
    virtual void  IPosArclength(float t, C3Vector &pos) const;
    virtual void  IPosParametric(float t, C3Vector &pos) const;
    virtual void  IVelArclength(float t, C3Vector &vel) const;
    virtual void  IVelParametric(float t, C3Vector &vel) const;
    virtual void  IFrameArclength(float t, C34Matrix &frame) const;
    virtual void  ISetPoints(const C3Vector *pts, UINT count);

   private:
    UINT SegCount() const {
      return points.Count() / 3;
    }
    void  ParametricSegT(float wholeT, UINT &segment, float &t) const;
    void  ArclengthSegT(float s, UINT &seg, float &t) const;
    void  Evaluate(UINT segment, float t, C3Vector &pos) const;
    void  EvaluateDer1(UINT segment, float t, C3Vector &der) const;
    float SegLength(UINT segment) const;
  };

  class C3Spline_CatmullRom : public C3Spline {
   public:
    enum SPLINE_MODE {
      MODE_LINEAR = 0,
      MODE_CATMULLROM = 1
    };

    C3Spline_CatmullRom() : splineMode(MODE_CATMULLROM) {
    }
    C3Spline_CatmullRom(const C3Spline_CatmullRom &spline) : C3Spline(spline), splineMode(spline.splineMode) {
    }
    void SetSplineMode(SPLINE_MODE mode) {
      splineMode = mode;
    }

    void Curvature(float t, C3Vector &centerOfCurvature) const;

   protected:
    virtual float ILength() const;
    virtual void  IValidateCache() const;
    virtual void  IPosArclength(float t, C3Vector &pos) const;
    virtual void  IPosParametric(float t, C3Vector &pos) const;
    virtual void  IVelArclength(float t, C3Vector &vel) const;
    virtual void  IVelParametric(float t, C3Vector &vel) const;
    virtual void  IFrameArclength(float t, C34Matrix &frame) const;
    virtual void  ISetPoints(const C3Vector *pts, UINT count);

   private:
    UINT SegCount() const {
      return points.Count() - 3;
    }
    void  ParametricSegT(float wholeT, UINT &segment, float &t) const;
    void  ArclengthSegT(float s, UINT &seg, float &t) const;
    void  Evaluate(UINT segment, float t, C3Vector &pos) const;
    void  EvaluateDer1(UINT segment, float t, C3Vector &der) const;
    void  EvaluateDer2(UINT segment, float t, C3Vector &der) const;
    float SegLength(UINT segment) const;

   protected:
    SPLINE_MODE splineMode;
  };

}  // namespace NTempest
