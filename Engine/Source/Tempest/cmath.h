#pragma once

#include <storm.h>
#include <math.h>

static float OneHalfOffset = 0.5f;

namespace NTempest {

  class CMath {
   public:
    static unsigned long __fastcall fuint_n(float r) {
      if (!(r >= .0f)) {
        SErrDisplayErrorFmt(STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1, "\"%s\", %s = %f", "r >= .0f", "r", r);
      }

      return static_cast<unsigned long>(r + 0.5f);
    }
    static unsigned long __fastcall mulhwu_(unsigned long x, unsigned long y);

    static unsigned char __fastcall ftol_0_256_(float x) {
      // todo: new macro
      if (!(x >= 0.0f)) {
        SErrDisplayErrorFmt(STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1, "\"%s\", %s = %f", "x >= 0.0f", "x", x);
      }

      if (!(x <= 255.9999f)) {
        SErrDisplayErrorFmt(STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1, "\"%s\", %s = %f", "x <= 255.9999f", "x", x);
      }

      x += 512.0f;
      return static_cast<unsigned char>(*reinterpret_cast<unsigned long *>(&x) >> 14);
    }

    static unsigned char __fastcall ftol_round_0_256_(float x) {
      // todo: new macro
      if (!(x >= -0.5f)) {
        SErrDisplayErrorFmt(STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1, "\"%s\", %s = %f", "x >= -0.5f", "x", x);
      }

      if (!(x <= 255.4999f)) {
        SErrDisplayErrorFmt(STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1, "\"%s\", %s = %f", "x <= 255.4999f", "x", x);
      }

      x += 512.5f;
      return static_cast<unsigned char>(*reinterpret_cast<unsigned long *>(&x) >> 14);
    }

    static long __fastcall fint_mi(float x) {
      return static_cast<long>(floor(x));
    }

    static float __fastcall fabs_(float x) {
      return static_cast<float>(fabs(x));
    }

    static unsigned int __fastcall fequal_(float x, float y) {
      return fabs_(x - y) < 0.00000023841858f;
    }

    static unsigned int __fastcall fequal4_(float x, float y) {
      return fabs_(x - y) < 0.00000095367432f;
    }

    static unsigned int __fastcall fnotequal_(float x, float y) {
      return !fequal_(x, y);
    }

    static float __fastcall cos_(float x) {
      return static_cast<float>(cos(x));
    }

    static float __fastcall sin_(float x) {
      return static_cast<float>(sin(x));
    }

    static void __fastcall sincos_(float x, float &sinValue, float &cosValue) {
      sinValue = sin_(x);
      cosValue = cos_(x);
    }

    static float __fastcall sqrt_(float x) {
      // todo: new macro
      if (!(x >= .0f)) {
        SErrDisplayErrorFmt(STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1, "\"%s\", %s = %f", "x >= .0f", "x", x);
      }

      return static_cast<float>(sqrt(x));
    }
  };

}  // namespace NTempest
