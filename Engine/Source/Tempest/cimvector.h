#pragma once

#include "Tempest/cmath.h"

namespace NTempest {

  class CRgb565;
  class CArgb1555;
  class CArgb4444;
  class C3Vector;

  class CImVector {
   public:
    enum {
      eTransparent = 0,
      eOpaque8bit = 255,
      eOpaque = 256
    };
    enum {
      eBlueMask = 0x000000FF,
      eGreenMask = 0x0000FF00,
      eRedMask = 0x00FF0000,
      eAlphaMask = 0xFF000000,
      eNotBlueMask = ~eBlueMask,
      eNotGreenMask = ~eGreenMask,
      eNotRedMask = ~eRedMask,
      eNotAlphaMask = ~eAlphaMask
    };
    enum {
      eAlphaS = 24,
      eRedS = 16,
      eGreenS = 8,
      eBlueS = 0
    };

    CImVector(unsigned char alpha, unsigned char red, unsigned char green, unsigned char blue) {
      Set(alpha, red, green, blue);
    }

    CImVector(unsigned long n = 0);

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

    static unsigned long MakeARGB(unsigned char alpha, unsigned char red, unsigned char green, unsigned char blue) {
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
    static unsigned long MakeRGB(unsigned char red, unsigned char green, unsigned char blue);
    static unsigned long A_(unsigned long value);
    unsigned long A_() const;
    static unsigned long R_(unsigned long value);
    unsigned long R_() const;
    static unsigned long G_(unsigned long value);
    unsigned long G_() const;
    static unsigned long B_(unsigned long value);
    unsigned long B_() const;
    static void Get_(unsigned long value, float &alpha, float &red, float &green, float &blue);
    static void Get_(
        unsigned long value,
        unsigned long &alpha,
        unsigned long &red,
        unsigned long &green,
        unsigned long &blue);
    static void Get_(unsigned long value, unsigned long &red, unsigned long &green, unsigned long &blue);
    static unsigned long Neg(unsigned long value);
    void Neg();
    static unsigned long NegRGB(unsigned long value);
    void NegRGB();
    static unsigned long Desaturate(unsigned long value);
    void Desaturate();
    static unsigned long NegA(unsigned long value);
    void NegA();
    static unsigned long NegR(unsigned long value);
    void NegR();
    static unsigned long NegG(unsigned long value);
    void NegG();
    static unsigned long NegB(unsigned long value);
    void NegB();
    static unsigned char Gray(unsigned long value);
    unsigned char Gray() const;
    CImVector &operator=(const C3Vector &c);
               operator C3Vector() const;

   protected:
    unsigned long SetC_(unsigned long value, unsigned long mask, unsigned long shift) const;
    static unsigned char ScaleC(unsigned long value, unsigned long scale);
    static unsigned char ScaleC255(unsigned long value, unsigned long scale);
    static unsigned char BlendC(unsigned long alpha, unsigned long source, unsigned long destination);
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

   protected:
    static unsigned char s_a1Table[];
    static unsigned char s_a4Table[];

   public:
    unsigned char b;
    unsigned char g;
    unsigned char r;
    unsigned char a;
  };

  inline CImVector::CImVector(unsigned long n) {
    *reinterpret_cast<unsigned long *>(this) = n;
  }

  class CRgb565 {
   public:
    enum {
      eBlueMask = 0x001F,
      eGreenMask = 0x07C0,
      eRedMask = 0xF800
    };
    enum {
      eNotBlueMask = ~eBlueMask,
      eNotGreenMask = ~eGreenMask,
      eNotRedMask = ~eRedMask
    };

    CRgb565() {
    }

    CRgb565(unsigned short value) {
      *reinterpret_cast<unsigned short *>(this) = value;
    }

    CRgb565(unsigned char r5, unsigned char g6, unsigned char b5) {
      From565(r5, g6, b5);
    }

    void From565(unsigned char r5, unsigned char g6, unsigned char b5) {
      r = r5;
      g = g6;
      b = b5;
    }
    void From888(unsigned int red, unsigned int green, unsigned int blue);
    void From555(unsigned char red, unsigned char green, unsigned char blue);
    void From444(unsigned char red, unsigned char green, unsigned char blue);
    void FromARGB(unsigned char alpha, const CRgb565 &rgb);
    CImVector MakeArgb() const;
    static CRgb565 Blend(unsigned long alpha, const CRgb565 &source, const CRgb565 &destination);

    CRgb565 &operator=(const CImVector &c) {
      r = c.r >> 3;
      g = c.g >> 2;
      b = c.b >> 3;
      return *this;
    }

    CRgb565 &operator=(unsigned short value) {
      *reinterpret_cast<unsigned short *>(this) = value;
      return *this;
    }
    CRgb565 &operator=(const CRgb565 &value) {
      return *this = static_cast<unsigned short>(value);
    }
    CRgb565 &operator=(const CArgb1555 &value);
    CRgb565 &operator=(const CArgb4444 &value);

    operator unsigned short() const {
      return *reinterpret_cast<const unsigned short *>(this);
    }

   private:
    static unsigned char BlendC(unsigned long alpha, unsigned long source, unsigned long destination);

   public:
    unsigned short b : 5;
    unsigned short g : 6;
    unsigned short r : 5;
  };

  class CArgb1555 {
   public:
    enum {
      eBlueMask = 0x001F,
      eGreenMask = 0x03E0,
      eRedMask = 0x7C00,
      eAlphaMask = 0x8000
    };
    enum {
      eNotBlueMask = ~eBlueMask,
      eNotGreenMask = ~eGreenMask,
      eNotRedMask = ~eRedMask,
      eNotAlphaMask = ~eAlphaMask
    };

    CArgb1555() {
    }

    CArgb1555(unsigned short value) {
      *reinterpret_cast<unsigned short *>(this) = value;
    }

    CArgb1555(unsigned char alpha, unsigned char red, unsigned char green, unsigned char blue) {
      a = alpha;
      r = red;
      g = green;
      b = blue;
    }

    void From565(unsigned char r5, unsigned char g6, unsigned char b5) {
      a = 1;
      r = r5;
      g = g6 >> 1;
      b = b5;
    }
    void From1555(unsigned char alpha, unsigned char red, unsigned char green, unsigned char blue);
    void From4444(unsigned char alpha, unsigned char red, unsigned char green, unsigned char blue);
    void From8888(unsigned char alpha, unsigned char red, unsigned char green, unsigned char blue);
    void FromARGB(unsigned char alpha, const CArgb1555 &rgb);
    CArgb1555 &operator=(const CArgb1555 &value) {
      return *this = static_cast<unsigned short>(value);
    }
    CArgb1555 &operator=(const CArgb4444 &value);

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
      *reinterpret_cast<unsigned short *>(this) = value;
      return *this;
    }

    operator unsigned short() const {
      return *reinterpret_cast<const unsigned short *>(this);
    }

    unsigned short b : 5;
    unsigned short g : 5;
    unsigned short r : 5;
    unsigned short a : 1;
  };

  class CArgb4444 {
   public:
    enum {
      eBlueMask = 0x000F,
      eGreenMask = 0x00F0,
      eRedMask = 0x0F00,
      eAlphaMask = 0xF000
    };
    enum {
      eNotBlueMask = ~eBlueMask,
      eNotGreenMask = ~eGreenMask,
      eNotRedMask = ~eRedMask,
      eNotAlphaMask = ~eAlphaMask
    };

    CArgb4444() {
    }

    CArgb4444(unsigned short value) {
      *reinterpret_cast<unsigned short *>(this) = value;
    }

    CArgb4444(unsigned char alpha, unsigned char red, unsigned char green, unsigned char blue) {
      a = alpha;
      r = red;
      g = green;
      b = blue;
    }

    void From565(unsigned char r5, unsigned char g6, unsigned char b5) {
      a = 15;
      r = r5 >> 1;
      g = g6 >> 2;
      b = b5 >> 1;
    }
    void From1555(unsigned char alpha, unsigned char red, unsigned char green, unsigned char blue);
    void From4444(unsigned char alpha, unsigned char red, unsigned char green, unsigned char blue);
    void From8888(unsigned char alpha, unsigned char red, unsigned char green, unsigned char blue);
    void FromARGB(unsigned char alpha, const CArgb4444 &rgb);
    CArgb4444 &operator=(const CArgb4444 &value) {
      return *this = static_cast<unsigned short>(value);
    }
    CArgb4444 &operator=(const CArgb1555 &value);

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

    CArgb4444 &operator=(const unsigned short value) {
      *reinterpret_cast<unsigned short *>(this) = value;
      return *this;
    }

    operator unsigned short() const {
      return *reinterpret_cast<const unsigned short *>(this);
    }

   protected:
    static unsigned char s_a1Table[];

   public:
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

  void RGBtoHSV(const C3Vector &rgb, C3Vector &hsv);
  void HSVtoRGB(const C3Vector &hsv, C3Vector &rgb);

}  // namespace NTempest
