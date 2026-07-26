#pragma once

#include "Tempest/cmath.h"

namespace NTempest {

  class CRgb565;
  class CArgb1555;
  class CArgb4444;
  class C3Vector;

  class CImVector {
   public:
    CImVector() {
      *reinterpret_cast<unsigned long *>(this) = 0;
    }

    CImVector(unsigned char alpha, unsigned char red, unsigned char green, unsigned char blue) {
      Set(alpha, red, green, blue);
    }

    CImVector(unsigned long n) {
      *reinterpret_cast<unsigned long *>(this) = n;
    }

    CImVector(unsigned char red, unsigned char green, unsigned char blue) {
      Set(0, red, green, blue);
    }

    CImVector(const CImVector *value) {
      *reinterpret_cast<unsigned long *>(this) = *reinterpret_cast<const unsigned long *>(value);
    }

    CImVector(const CImVector &value) {
      *reinterpret_cast<unsigned long *>(this) = *reinterpret_cast<const unsigned long *>(&value);
    }

    ~CImVector() {
    }

    static unsigned long __fastcall MakeARGB(unsigned char alpha, unsigned char red, unsigned char green, unsigned char blue) {
      return (static_cast<unsigned long>(alpha) << 24) | (static_cast<unsigned long>(red) << 16) | (static_cast<unsigned long>(green) << 8) |
             static_cast<unsigned long>(blue);
    }

    unsigned long *IV_() const {
      return reinterpret_cast<unsigned long *>(const_cast<CImVector *>(this));
    }

    void Set(float alpha, float red, float green, float blue) {
      *IV_() = MakeARGB(
          static_cast<unsigned char>(CMath::fuint_n(alpha * 255.0f)), static_cast<unsigned char>(CMath::fuint_n(red * 255.0f)),
          static_cast<unsigned char>(CMath::fuint_n(green * 255.0f)), static_cast<unsigned char>(CMath::fuint_n(blue * 255.0f))
      );
    }

    void Set(unsigned long value) {
      *IV_() = value;
    }

    CImVector &operator=(unsigned long n) {
      *IV_() = n;
      return *this;
    }

    void Set(unsigned char alpha, unsigned char red, unsigned char green, unsigned char blue) {
      *IV_() = MakeARGB(alpha, red, green, blue);
    }

    void From565(unsigned char r5, unsigned char g6, unsigned char b5);

    CImVector &operator=(const CRgb565 &c);
    CImVector &operator=(const CArgb1555 &c);
    CImVector &operator=(const CArgb4444 &c);
    CImVector &operator=(const CImVector &c) {
      *IV_() = *c.IV_();
      return *this;
    }
    CImVector &operator=(const C3Vector &c);
               operator C3Vector() const;

   protected:
    unsigned long SetC_(unsigned long value, unsigned long mask, unsigned long shift);
    void Scale_(unsigned long scale);
    void ScaleRGB_(unsigned long scale);
    void Scale255RGB_(unsigned long scale);
    void Multiply_(const CImVector *source);
    void Blend_(unsigned long alpha, const CImVector *source);
    void BlendARGB_(unsigned long alpha, const CImVector *source);

    void Scale255_(unsigned long scale) {
      Set(
          0,
          static_cast<unsigned char>((scale * r + 255) >> 8),
          static_cast<unsigned char>((scale * g + 255) >> 8),
          static_cast<unsigned char>((scale * b + 255) >> 8)
      );
    }

    void MultiplyRGB_(const CImVector *s) {
      CImVector d(*this);
      CImVector sa(*s);

      Set(d.a, static_cast<unsigned char>((sa.r * d.r + 255) >> 8), static_cast<unsigned char>((sa.g * d.g + 255) >> 8),
          static_cast<unsigned char>((sa.b * d.b + 255) >> 8));
    }

    void BlendRGB_(unsigned long alpha, const CImVector *source) {
      CImVector destination(*this);
      Set(
          destination.a,
          static_cast<unsigned char>(destination.r + ((alpha * (source->r - destination.r)) >> 8)),
          static_cast<unsigned char>(destination.g + ((alpha * (source->g - destination.g)) >> 8)),
          static_cast<unsigned char>(destination.b + ((alpha * (source->b - destination.b)) >> 8))
      );
    }

    void Blend255_(unsigned long alpha, const CImVector *source) {
      if (alpha == 255) {
        *IV_() = *source->IV_();
        return;
      }

      CImVector destination(*this);
      Set(
          0,
          static_cast<unsigned char>(destination.r + ((alpha * (source->r - destination.r)) >> 8)),
          static_cast<unsigned char>(destination.g + ((alpha * (source->g - destination.g)) >> 8)),
          static_cast<unsigned char>(destination.b + ((alpha * (source->b - destination.b)) >> 8))
      );
    }

    void BlendRGB255_(unsigned long alpha, const CImVector *source) {
      if (alpha == 255) {
        SetRGB(source);
        return;
      }

      CImVector destination(*this);
      Set(
          destination.a,
          static_cast<unsigned char>(destination.r + ((alpha * (source->r - destination.r)) >> 8)),
          static_cast<unsigned char>(destination.g + ((alpha * (source->g - destination.g)) >> 8)),
          static_cast<unsigned char>(destination.b + ((alpha * (source->b - destination.b)) >> 8))
      );
    }

   public:
    void Get(float &alpha, float &red, float &green, float &blue) const;
    void Get(unsigned long &alpha, unsigned long &red, unsigned long &green, unsigned long &blue) const;
    void Get(unsigned long &red, unsigned long &green, unsigned long &blue) const;
    unsigned long Get() const;
    unsigned long GetRGB() const;
    void SetA(unsigned char alpha);
    void SetR(unsigned char red);
    void SetG(unsigned char green);
    void SetB(unsigned char blue);
    void Set(const CImVector *value);
    void Set(const CImVector &value);
    void Set(unsigned char red, unsigned char green, unsigned char blue);
    void SetRGB(unsigned long value);
    void SetRGB(const CImVector &value);
    void SetRGB(unsigned char red, unsigned char green, unsigned char blue);
    void From1555(unsigned char alpha, unsigned char red, unsigned char green, unsigned char blue);
    void From4444(unsigned char alpha, unsigned char red, unsigned char green, unsigned char blue);
    void FromARGB(unsigned char alpha, const CImVector &rgb);

    unsigned long operator~() const;
    operator unsigned long() const;

    void Scale(unsigned long scale);
    void ScaleRGB(unsigned long scale);
    void Scale255(unsigned long scale);
    void Scale255RGB(unsigned long scale);
    void ScaleA(unsigned long scale);
    void ScaleA255(unsigned long scale);
    void Multiply(const CImVector *source);
    void MultiplyRGB(const CImVector *s) {
      MultiplyRGB_(s);
    }

    void Blend(unsigned long alpha, unsigned long source);
    void Blend(unsigned long alpha, const CImVector *source);
    void Blend(unsigned long source);
    void Blend(const CImVector *source);
    void BlendRGB(unsigned long alpha, unsigned long source);
    void BlendRGB(unsigned long alpha, const CImVector *source);
    void BlendRGB(unsigned long source);
    void BlendRGB(const CImVector *source);
    void BlendARGB(unsigned long alpha, unsigned long source);
    void BlendARGB(unsigned long alpha, const CImVector *source);
    void Blend255(unsigned long alpha, unsigned long source);
    void Blend255(unsigned long alpha, const CImVector *source);
    void Blend255RGB(unsigned long alpha, unsigned long source);
    void Blend255RGB(unsigned long alpha, const CImVector *source);

    void SetRGB(const CImVector *source) {
      *IV_() ^= (*IV_() ^ *source->IV_()) & 0x00FFFFFF;
    }

    unsigned char &operator[](unsigned long index) {
      ASSERT(index < 4);
      return (&b)[index];
    }

    const unsigned char &operator[](unsigned long index) const {
      ASSERT(index < 4);
      return (&b)[index];
    }

    unsigned char b;
    unsigned char g;
    unsigned char r;
    unsigned char a;
  };

  class CRgb565 {
   public:
    CRgb565() {
    }

    CRgb565(unsigned short value) {
      *Value() = value;
    }

    CRgb565(unsigned char r5, unsigned char g6, unsigned char b5) {
      From565(r5, g6, b5);
    }

    ~CRgb565() {
    }

    void From565(unsigned char r5, unsigned char g6, unsigned char b5) {
      r = r5;
      g = g6;
      b = b5;
    }

    CRgb565 &operator=(const CImVector &c) {
      r = c.r >> 3;
      g = c.g >> 2;
      b = c.b >> 3;
      return *this;
    }

    CRgb565 &operator=(unsigned short value) {
      *Value() = value;
      return *this;
    }

    operator unsigned short() const {
      return *Value();
    }

    unsigned short *Value() {
      return reinterpret_cast<unsigned short *>(this);
    }

    const unsigned short *Value() const {
      return reinterpret_cast<const unsigned short *>(this);
    }

    unsigned short b : 5;
    unsigned short g : 6;
    unsigned short r : 5;
  };

  class CArgb1555 {
   public:
    CArgb1555() {
    }

    CArgb1555(unsigned short value) {
      *Value() = value;
    }

    CArgb1555(unsigned char alpha, unsigned char red, unsigned char green, unsigned char blue) {
      a = alpha;
      r = red;
      g = green;
      b = blue;
    }

    ~CArgb1555() {
    }

    void From565(unsigned char r5, unsigned char g6, unsigned char b5) {
      a = 1;
      r = r5;
      g = g6 >> 1;
      b = b5;
    }

    CArgb1555 &operator=(const CRgb565 &c) {
      From565(c.r, c.g, c.b);
      return *this;
    }

    CArgb1555 &operator=(const CImVector &c) {
      a = c.a >> 7;
      r = c.r >> 3;
      g = c.g >> 3;
      b = c.b >> 3;
      return *this;
    }

    CArgb1555 &operator=(unsigned short value) {
      *Value() = value;
      return *this;
    }

    operator unsigned short() const {
      return *Value();
    }

    unsigned short *Value() {
      return reinterpret_cast<unsigned short *>(this);
    }

    const unsigned short *Value() const {
      return reinterpret_cast<const unsigned short *>(this);
    }

    unsigned short b : 5;
    unsigned short g : 5;
    unsigned short r : 5;
    unsigned short a : 1;
  };

  class CArgb4444 {
   public:
    CArgb4444() {
    }

    CArgb4444(unsigned short value) {
      *Value() = value;
    }

    CArgb4444(unsigned char alpha, unsigned char red, unsigned char green, unsigned char blue) {
      a = alpha;
      r = red;
      g = green;
      b = blue;
    }

    ~CArgb4444() {
    }

    void From565(unsigned char r5, unsigned char g6, unsigned char b5) {
      a = 15;
      r = r5 >> 1;
      g = g6 >> 2;
      b = b5 >> 1;
    }

    CArgb4444 &operator=(const CRgb565 &c) {
      From565(c.r, c.g, c.b);
      return *this;
    }

    CArgb4444 &operator=(const CImVector &c) {
      a = c.a >> 4;
      r = c.r >> 4;
      g = c.g >> 4;
      b = c.b >> 4;
      return *this;
    }

    CArgb4444 &operator=(unsigned short value) {
      *Value() = value;
      return *this;
    }

    operator unsigned short() const {
      return *Value();
    }

    unsigned short *Value() {
      return reinterpret_cast<unsigned short *>(this);
    }

    const unsigned short *Value() const {
      return reinterpret_cast<const unsigned short *>(this);
    }

    unsigned short b : 4;
    unsigned short g : 4;
    unsigned short r : 4;
    unsigned short a : 4;
  };

  inline void CImVector::From565(unsigned char r5, unsigned char g6, unsigned char b5) {
    a = 255;
    r = r5 << 3;
    g = g6 << 2;
    b = b5 << 3;
  }

  inline CImVector &CImVector::operator=(const CRgb565 &c) {
    From565(c.r, c.g, c.b);
    return *this;
  }

  void __fastcall RGBtoHSV(const C3Vector &rgb, C3Vector &hsv);
  void __fastcall HSVtoRGB(const C3Vector &hsv, C3Vector &rgb);

}  // namespace NTempest
