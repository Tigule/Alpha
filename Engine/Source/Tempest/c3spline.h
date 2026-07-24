#pragma once

#include "Tempest/c3vector.h"

#include <stpl.h>

class CSplineParticleEmitter;

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
    ~C3Spline() {
    }

    C3Spline &operator=(const C3Spline &spline);
    void SetPoints(const C3Vector *pts, unsigned int count);
    void Pos(float t, C3Vector &pos, EvalType ptype) const;
    void Vel(float t, C3Vector &vel, EvalType ptype) const;
    void Frame(float t, C34Matrix &frame, EvalType ptype) const;

   protected:
    friend class ::CSplineParticleEmitter;
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
    float SegLength(unsigned int segment, const C44Matrix &coeffs) const;

    void ParametricSegT(float wholeT, unsigned int segCount, unsigned int &segment, float &t) const;
    void ArclengthSegT(float s, const C44Matrix &coeffs, unsigned int segCount, unsigned int &seg, float &t) const;

   public:
    mutable float                  cachedLength;

   protected:
    TSGrowableArray<C3Vector>      points;
    mutable TSGrowableArray<float> cachedSegLength;
  };

  class C3Spline_Bezier3 : public C3Spline {
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

    unsigned int NumPoints() const {
      return points.Count();
    }

    const C3Vector &Point(unsigned int index) const {
      return points[index];
    }

    void SetSplineMode(SPLINE_MODE mode) {
      splineMode = mode;
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
    void  ParametricSegT(float wholeT, unsigned int &segment, float &t) const;
    void  ArclengthSegT(float s, unsigned int &seg, float &t) const;
    void  Evaluate(unsigned int segment, float t, C3Vector &pos) const;
    void  EvaluateDer1(unsigned int segment, float t, C3Vector &der) const;
    float SegLength(unsigned int segment) const;

    SPLINE_MODE splineMode;
  };

}  // namespace NTempest
