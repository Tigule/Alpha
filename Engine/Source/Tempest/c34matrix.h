#ifndef ENGINE_SOURCE_TEMPEST_C34MATRIX_H
#define ENGINE_SOURCE_TEMPEST_C34MATRIX_H

namespace NTempest {

  class C3Vector;
  class C4Quaternion;

  class C34Matrix {
   public:
    enum {
      eComponents = 12
    };

    C34Matrix() {
      a0 = 1.0f;
      a1 = 0.0f;
      a2 = 0.0f;
      b0 = 0.0f;
      b1 = 1.0f;
      b2 = 0.0f;
      c0 = 0.0f;
      c1 = 0.0f;
      c2 = 1.0f;
      d0 = 0.0f;
      d1 = 0.0f;
      d2 = 0.0f;
    }

    C34Matrix(float a0, float a1, float a2, float b0, float b1, float b2, float c0, float c1, float c2, float d0, float d1, float d2)
        : a0(a0), a1(a1), a2(a2), b0(b0), b1(b1), b2(b2), c0(c0), c1(c1), c2(c2), d0(d0), d1(d1), d2(d2) {
    }

    ~C34Matrix() {
    }

    C34Matrix &operator+=(const C34Matrix &a);
    C34Matrix &operator*=(const C34Matrix &a);
    C34Matrix &operator/=(float a);

    void Identity() {
      a0 = 1.0f;
      a1 = 0.0f;
      a2 = 0.0f;
      b0 = 0.0f;
      b1 = 1.0f;
      b2 = 0.0f;
      c0 = 0.0f;
      c1 = 0.0f;
      c2 = 1.0f;
      d0 = 0.0f;
      d1 = 0.0f;
      d2 = 0.0f;
    }

    static C34Matrix __fastcall Rotation(float angle, const C3Vector &axis, bool unit);

    C34Matrix AffineInverse() const;
    C34Matrix AffineInverse(float uniformScale) const;

    void Translate(const C3Vector &move);
    void Scale(const C3Vector &scale);
    void Scale(float scale);
    void Rotate(const C4Quaternion &rotation);
    void Rotate(float angle, const C3Vector &axis, bool unit);

    float a0;
    float a1;
    float a2;
    float b0;
    float b1;
    float b2;
    float c0;
    float c1;
    float c2;
    float d0;
    float d1;
    float d2;
  };

  C34Matrix __fastcall operator+(const C34Matrix &l, const C34Matrix &r);
  C34Matrix __fastcall operator*(const C34Matrix &l, const C34Matrix &r);
  C3Vector __fastcall  operator*(const C3Vector &l, const C34Matrix &r);
  C3Vector __fastcall  operator*=(C3Vector &l, const C34Matrix &r);
  C34Matrix __fastcall operator/(const C34Matrix &l, float a);

}  // namespace NTempest

#endif
