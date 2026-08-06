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

    CImVector(BYTE alpha, BYTE red, BYTE green, BYTE blue) {
      Set(alpha, red, green, blue);
    }

    CImVector(DWORD n = 0);

    CImVector(BYTE red, BYTE green, BYTE blue) {
      Set(0, red, green, blue);
    }

    CImVector(const CImVector *value) {
      *reinterpret_cast<DWORD *>(this) = *reinterpret_cast<const DWORD *>(value);
    }

    CImVector(const CImVector &value) {
      *reinterpret_cast<DWORD *>(this) = *reinterpret_cast<const DWORD *>(&value);
    }

    ~CImVector() {
    }

    static DWORD MakeARGB(BYTE alpha, BYTE red, BYTE green, BYTE blue) {
      return (static_cast<DWORD>(alpha) << 24) | (static_cast<DWORD>(red) << 16) | (static_cast<DWORD>(green) << 8) | static_cast<DWORD>(blue);
    }

    DWORD *IV_() const {
      return reinterpret_cast<DWORD *>(const_cast<CImVector *>(this));
    }

    void Set(float alpha, float red, float green, float blue) {
      *IV_() = MakeARGB(
          static_cast<BYTE>(CMath::fuint_n(alpha * 255.0f)), static_cast<BYTE>(CMath::fuint_n(red * 255.0f)),
          static_cast<BYTE>(CMath::fuint_n(green * 255.0f)), static_cast<BYTE>(CMath::fuint_n(blue * 255.0f))
      );
    }

    void Set(DWORD value) {
      *IV_() = value;
    }

    CImVector &operator=(DWORD n) {
      *IV_() = n;
      return *this;
    }

    void Set(BYTE alpha, BYTE red, BYTE green, BYTE blue) {
      *IV_() = MakeARGB(alpha, red, green, blue);
    }

    void From565(BYTE r5, BYTE g6, BYTE b5);

    CImVector &operator=(const CRgb565 &c);
    CImVector &operator=(const CArgb1555 &c);
    CImVector &operator=(const CArgb4444 &c);
    CImVector &operator=(const CImVector &c) {
      *IV_() = *c.IV_();
      return *this;
    }
    static DWORD MakeRGB(BYTE red, BYTE green, BYTE blue);
    static DWORD A_(DWORD value);
    DWORD        A_() const;
    static DWORD R_(DWORD value);
    DWORD        R_() const;
    static DWORD G_(DWORD value);
    DWORD        G_() const;
    static DWORD B_(DWORD value);
    DWORD        B_() const;
    static void  Get_(DWORD value, float &alpha, float &red, float &green, float &blue);
    static void  Get_(DWORD value, DWORD &alpha, DWORD &red, DWORD &green, DWORD &blue);
    static void  Get_(DWORD value, DWORD &red, DWORD &green, DWORD &blue);
    static DWORD Neg(DWORD value);
    void         Neg();
    static DWORD NegRGB(DWORD value);
    void         NegRGB();
    static DWORD Desaturate(DWORD value);
    void         Desaturate();
    static DWORD NegA(DWORD value);
    void         NegA();
    static DWORD NegR(DWORD value);
    void         NegR();
    static DWORD NegG(DWORD value);
    void         NegG();
    static DWORD NegB(DWORD value);
    void         NegB();
    static BYTE  Gray(DWORD value);
    BYTE         Gray() const;
    CImVector   &operator=(const C3Vector &c);
                 operator C3Vector() const;

   protected:
    DWORD       SetC_(DWORD value, DWORD mask, DWORD shift) const;
    static BYTE ScaleC(DWORD value, DWORD scale);
    static BYTE ScaleC255(DWORD value, DWORD scale);
    static BYTE BlendC(DWORD alpha, DWORD source, DWORD destination);
    void        Scale_(DWORD scale);
    void        ScaleRGB_(DWORD scale);
    void        Scale255RGB_(DWORD scale);
    void        Multiply_(const CImVector *source);
    void        Blend_(DWORD alpha, const CImVector *source);
    void        BlendARGB_(DWORD alpha, const CImVector *source);

    void Scale255_(DWORD scale) {
      Set(0, static_cast<BYTE>((scale * r + 255) >> 8), static_cast<BYTE>((scale * g + 255) >> 8), static_cast<BYTE>((scale * b + 255) >> 8));
    }

    void MultiplyRGB_(const CImVector *s) {
      CImVector d(*this);
      CImVector sa(*s);

      Set(d.a, static_cast<BYTE>((sa.r * d.r + 255) >> 8), static_cast<BYTE>((sa.g * d.g + 255) >> 8), static_cast<BYTE>((sa.b * d.b + 255) >> 8));
    }

    void BlendRGB_(DWORD alpha, const CImVector *source) {
      CImVector destination(*this);
      Set(destination.a, static_cast<BYTE>(destination.r + ((alpha * (source->r - destination.r)) >> 8)),
          static_cast<BYTE>(destination.g + ((alpha * (source->g - destination.g)) >> 8)),
          static_cast<BYTE>(destination.b + ((alpha * (source->b - destination.b)) >> 8)));
    }

    void Blend255_(DWORD alpha, const CImVector *source) {
      if (alpha == 255) {
        *IV_() = *source->IV_();
        return;
      }

      CImVector destination(*this);
      Set(0, static_cast<BYTE>(destination.r + ((alpha * (source->r - destination.r)) >> 8)),
          static_cast<BYTE>(destination.g + ((alpha * (source->g - destination.g)) >> 8)),
          static_cast<BYTE>(destination.b + ((alpha * (source->b - destination.b)) >> 8)));
    }

    void BlendRGB255_(DWORD alpha, const CImVector *source) {
      if (alpha == 255) {
        SetRGB(source);
        return;
      }

      CImVector destination(*this);
      Set(destination.a, static_cast<BYTE>(destination.r + ((alpha * (source->r - destination.r)) >> 8)),
          static_cast<BYTE>(destination.g + ((alpha * (source->g - destination.g)) >> 8)),
          static_cast<BYTE>(destination.b + ((alpha * (source->b - destination.b)) >> 8)));
    }

   public:
    void  Get(float &alpha, float &red, float &green, float &blue) const;
    void  Get(DWORD &alpha, DWORD &red, DWORD &green, DWORD &blue) const;
    void  Get(DWORD &red, DWORD &green, DWORD &blue) const;
    DWORD Get() const;
    DWORD GetRGB() const;
    void  SetA(BYTE alpha);
    void  SetR(BYTE red);
    void  SetG(BYTE green);
    void  SetB(BYTE blue);
    void  Set(const CImVector *value);
    void  Set(const CImVector &value);
    void  Set(BYTE red, BYTE green, BYTE blue);
    void  SetRGB(DWORD value);
    void  SetRGB(const CImVector &value);
    void  SetRGB(BYTE red, BYTE green, BYTE blue);
    void  From1555(BYTE alpha, BYTE red, BYTE green, BYTE blue);
    void  From4444(BYTE alpha, BYTE red, BYTE green, BYTE blue);
    void  FromARGB(BYTE alpha, const CImVector &rgb);

    DWORD operator~() const;
          operator DWORD() const;

    void Scale(DWORD scale);
    void ScaleRGB(DWORD scale);
    void Scale255(DWORD scale);
    void Scale255RGB(DWORD scale);
    void ScaleA(DWORD scale);
    void ScaleA255(DWORD scale);
    void Multiply(const CImVector *source);
    void MultiplyRGB(const CImVector *s) {
      MultiplyRGB_(s);
    }

    void Blend(DWORD alpha, DWORD source);
    void Blend(DWORD alpha, const CImVector *source);
    void Blend(DWORD source);
    void Blend(const CImVector *source);
    void BlendRGB(DWORD alpha, DWORD source);
    void BlendRGB(DWORD alpha, const CImVector *source);
    void BlendRGB(DWORD source);
    void BlendRGB(const CImVector *source);
    void BlendARGB(DWORD alpha, DWORD source);
    void BlendARGB(DWORD alpha, const CImVector *source);
    void Blend255(DWORD alpha, DWORD source);
    void Blend255(DWORD alpha, const CImVector *source);
    void Blend255RGB(DWORD alpha, DWORD source);
    void Blend255RGB(DWORD alpha, const CImVector *source);

    void SetRGB(const CImVector *source) {
      *IV_() ^= (*IV_() ^ *source->IV_()) & 0x00FFFFFF;
    }

    BYTE &operator[](DWORD index) {
      ASSERT(index < 4);
      return (&b)[index];
    }

    const BYTE &operator[](DWORD index) const {
      ASSERT(index < 4);
      return (&b)[index];
    }

   protected:
    static BYTE s_a1Table[];
    static BYTE s_a4Table[];

   public:
    BYTE b;
    BYTE g;
    BYTE r;
    BYTE a;
  };

  inline CImVector::CImVector(DWORD n) {
    *reinterpret_cast<DWORD *>(this) = n;
  }

  class CRgb565 {
   public:
    enum {
      eBlueMask = 0x001F,
      eGreenMask = 0x07C0,
      eRedMask = 0xF800,
      eNotBlueMask = ~eBlueMask,
      eNotGreenMask = ~eGreenMask,
      eNotRedMask = ~eRedMask
    };
    enum {
      eRedS = 11,
      eGreenS = 5,
      eBlueS = 0
    };

    CRgb565() {
    }

    CRgb565(WORD value) {
      *reinterpret_cast<WORD *>(this) = value;
    }

    CRgb565(BYTE r5, BYTE g6, BYTE b5) {
      From565(r5, g6, b5);
    }

    void From565(BYTE r5, BYTE g6, BYTE b5) {
      r = r5;
      g = g6;
      b = b5;
    }
    void           From888(UINT red, UINT green, UINT blue);
    void           From555(BYTE red, BYTE green, BYTE blue);
    void           From444(BYTE red, BYTE green, BYTE blue);
    void           FromARGB(BYTE alpha, const CRgb565 &rgb);
    CImVector      MakeArgb() const;
    static CRgb565 Blend(DWORD alpha, const CRgb565 &source, const CRgb565 &destination);

    CRgb565 &operator=(const CImVector &c) {
      r = c.r >> 3;
      g = c.g >> 2;
      b = c.b >> 3;
      return *this;
    }

    CRgb565 &operator=(WORD value) {
      *reinterpret_cast<WORD *>(this) = value;
      return *this;
    }
    CRgb565 &operator=(const CRgb565 &value) {
      return *this = static_cast<WORD>(value);
    }
    CRgb565 &operator=(const CArgb1555 &value);
    CRgb565 &operator=(const CArgb4444 &value);

    operator WORD() const {
      return *reinterpret_cast<const WORD *>(this);
    }

   private:
    static BYTE BlendC(DWORD alpha, DWORD source, DWORD destination);

   public:
    WORD b : 5;
    WORD g : 6;
    WORD r : 5;
  };

  class CArgb1555 {
   public:
    enum {
      eBlueMask = 0x001F,
      eGreenMask = 0x03E0,
      eRedMask = 0x7C00,
      eAlphaMask = 0x8000,
      eNotBlueMask = ~eBlueMask,
      eNotGreenMask = ~eGreenMask,
      eNotRedMask = ~eRedMask,
      eNotAlphaMask = ~eAlphaMask
    };
    enum {
      eAlphaS = 15,
      eRedS = 10,
      eGreenS = 5,
      eBlueS = 0
    };

    CArgb1555() {
    }

    CArgb1555(WORD value) {
      *reinterpret_cast<WORD *>(this) = value;
    }

    CArgb1555(BYTE alpha, BYTE red, BYTE green, BYTE blue) {
      a = alpha;
      r = red;
      g = green;
      b = blue;
    }

    void From565(BYTE r5, BYTE g6, BYTE b5) {
      a = 1;
      r = r5;
      g = g6 >> 1;
      b = b5;
    }
    void       From1555(BYTE alpha, BYTE red, BYTE green, BYTE blue);
    void       From4444(BYTE alpha, BYTE red, BYTE green, BYTE blue);
    void       From8888(BYTE alpha, BYTE red, BYTE green, BYTE blue);
    void       FromARGB(BYTE alpha, const CArgb1555 &rgb);
    CArgb1555 &operator=(const CArgb1555 &value) {
      return *this = static_cast<WORD>(value);
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

    CArgb1555 &operator=(WORD value) {
      *reinterpret_cast<WORD *>(this) = value;
      return *this;
    }

    operator WORD() const {
      return *reinterpret_cast<const WORD *>(this);
    }

    WORD b : 5;
    WORD g : 5;
    WORD r : 5;
    WORD a : 1;
  };

  class CArgb4444 {
   public:
    enum {
      eBlueMask = 0x000F,
      eGreenMask = 0x00F0,
      eRedMask = 0x0F00,
      eAlphaMask = 0xF000,
      eNotBlueMask = ~eBlueMask,
      eNotGreenMask = ~eGreenMask,
      eNotRedMask = ~eRedMask,
      eNotAlphaMask = ~eAlphaMask
    };
    enum {
      eAlphaS = 12,
      eRedS = 8,
      eGreenS = 4,
      eBlueS = 0
    };

    CArgb4444() {
    }

    CArgb4444(WORD value) {
      *reinterpret_cast<WORD *>(this) = value;
    }

    CArgb4444(BYTE alpha, BYTE red, BYTE green, BYTE blue) {
      a = alpha;
      r = red;
      g = green;
      b = blue;
    }

    void From565(BYTE r5, BYTE g6, BYTE b5) {
      a = 15;
      r = r5 >> 1;
      g = g6 >> 2;
      b = b5 >> 1;
    }
    void       From1555(BYTE alpha, BYTE red, BYTE green, BYTE blue);
    void       From4444(BYTE alpha, BYTE red, BYTE green, BYTE blue);
    void       From8888(BYTE alpha, BYTE red, BYTE green, BYTE blue);
    void       FromARGB(BYTE alpha, const CArgb4444 &rgb);
    CArgb4444 &operator=(const CArgb4444 &value) {
      return *this = static_cast<WORD>(value);
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

    CArgb4444 &operator=(const WORD value) {
      *reinterpret_cast<WORD *>(this) = value;
      return *this;
    }

    operator WORD() const {
      return *reinterpret_cast<const WORD *>(this);
    }

   protected:
    static BYTE s_a1Table[];

   public:
    WORD b : 4;
    WORD g : 4;
    WORD r : 4;
    WORD a : 4;
  };

  inline void CImVector::From565(BYTE r5, BYTE g6, BYTE b5) {
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
