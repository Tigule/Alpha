#pragma once

#include "Tempest/c3vector.h"

#include <stpl.h>

class CSplineParticleEmitter;

class C24Matrix {
 public:
  C24Matrix()
      : a0(0.0f), a1(0.0f), b0(0.0f), b1(0.0f),
        c0(0.0f), c1(0.0f), d0(0.0f), d1(0.0f) {
  }

  C24Matrix(
      float a0, float a1, float b0, float b1,
      float c0, float c1, float d0, float d1
  )
      : a0(a0), a1(a1), b0(b0), b1(b1),
        c0(c0), c1(c1), d0(d0), d1(d1) {
  }

  const float *operator[](unsigned int row) const {
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
      EVAL_ARCLENGTH = 1
    };

    C3Spline() : cachedLength(0.0f) {
    }
    C3Spline(const C3Spline &spline)
        : cachedLength(spline.cachedLength), points(spline.points), cachedSegLength(spline.cachedSegLength) {
    }
    ~C3Spline() {
    }

    C3Spline &operator=(const C3Spline &spline);
    unsigned int NumPoints() const {
      return points.Count();
    }
    const C3Vector &Point(unsigned int pointSub) const {
      return points[pointSub];
    }
    void SetPoints(const TSGrowableArray<C3Vector> &pts) {
      SetPoints(pts.Ptr(), pts.Count());
    }
    void SetPoints(const C3Vector *pts, unsigned int count);
    void SetPoint(unsigned int pointSub, const C3Vector &point);
    void Pos(float t, C3Vector &pos, EvalType ptype) const;
    void Vel(float t, C3Vector &vel, EvalType ptype) const;
    void Frame(float t, C34Matrix &frame, EvalType ptype) const;
    float Length() const {
      ValidateCache();
      return cachedLength;
    }

   protected:
    friend class ::CSplineParticleEmitter;
    virtual float ILength(unsigned int segmentCount) const;
    virtual float ILength() const = 0;
    virtual void  IValidateCache() const = 0;
    virtual void  IPosArclength(float t, C3Vector &result) const = 0;
    virtual void  IPosParametric(float t, C3Vector &result) const = 0;
    virtual void  IVelArclength(float t, C3Vector &result) const = 0;
    virtual void  IVelParametric(float t, C3Vector &result) const = 0;
    virtual void  IFrameArclength(float t, C34Matrix &result) const = 0;
    virtual void  ISetPoints(const C3Vector *pts, unsigned int count);

    void  Evaluate(unsigned int segment, float t, const C44Matrix &coeffs, C3Vector &pos) const;
    void  EvaluateDer1(unsigned int segment, float t, const C34Matrix &coeffs, C3Vector &der) const;
    void  EvaluateDer2(unsigned int segment, float t, const C24Matrix &coeffs, C3Vector &der) const;
    void  Curvature(
        unsigned int segment,
        float t,
        const C34Matrix &der1coeffs,
        const C24Matrix &der2coeffs,
        C3Vector &centerOfCurvature
    ) const;
    float SegLength(unsigned int segment, const C44Matrix &coeffs) const;
    void ValidateCache() const;

    void ParametricSegT(float wholeT, unsigned int segCount, unsigned int &segment, float &t) const;
    void ArclengthSegT(float s, const C44Matrix &coeffs, unsigned int segCount, unsigned int &seg, float &t) const;

   public:
    mutable float                  cachedLength;

   protected:
    TSGrowableArray<C3Vector>      points;
    mutable TSGrowableArray<float> cachedSegLength;
  };

  class C3Spline_Bezier3 : public C3Spline {
   public:
    C3Spline_Bezier3() {
    }
    C3Spline_Bezier3(const C3Vector *pts, unsigned int count) {
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
    virtual void  ISetPoints(const C3Vector *pts, unsigned int count);

   private:
    unsigned int SegCount() const {
      return points.Count() / 3;
    }
    void  ParametricSegT(float wholeT, unsigned int &segment, float &t) const;
    void  ArclengthSegT(float s, unsigned int &seg, float &t) const;
    void  Evaluate(unsigned int segment, float t, C3Vector &pos) const;
    void  EvaluateDer1(unsigned int segment, float t, C3Vector &der) const;
    float SegLength(unsigned int segment) const;
  };

  class C3Spline_CatmullRom : public C3Spline {
   public:
    enum SPLINE_MODE {
      MODE_LINEAR = 0,
      MODE_CATMULLROM = 1
    };

    C3Spline_CatmullRom() : splineMode(MODE_CATMULLROM) {
    }
    C3Spline_CatmullRom(const C3Spline_CatmullRom &spline)
        : C3Spline(spline), splineMode(spline.splineMode) {
    }
    unsigned int NumPoints() const {
      return points.Count();
    }

    const C3Vector &Point(unsigned int index) const {
      return points[index];
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
    virtual void  ISetPoints(const C3Vector *pts, unsigned int count);

   private:
    unsigned int SegCount() const {
      return points.Count() - 3;
    }
    void  ParametricSegT(float wholeT, unsigned int &segment, float &t) const;
    void  ArclengthSegT(float s, unsigned int &seg, float &t) const;
    void  Evaluate(unsigned int segment, float t, C3Vector &pos) const;
    void  EvaluateDer1(unsigned int segment, float t, C3Vector &der) const;
    void  EvaluateDer2(unsigned int segment, float t, C3Vector &der) const;
    float SegLength(unsigned int segment) const;

    SPLINE_MODE splineMode;
  };

}  // namespace NTempest
