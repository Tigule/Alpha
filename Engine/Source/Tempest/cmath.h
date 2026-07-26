#pragma once

#include <storm.h>
#include <float.h>
#include <math.h>

static float OneHalfOffset = 0.5f;

namespace NTempest {

  class CMath {
   protected:
    static unsigned long __fastcall left1_(unsigned long x) {
      return x << 1;
    }

   public:
    static double __fastcall logoid_(double x, double a, double b, double c, double d, double ln2);
    static double __fastcall logoid2_(double x, double a, double b, double c, double d);
    static double __fastcall logoid10_(double x, double a, double b, double c, double d, double ln10);
    static double __fastcall log2_(double x);
    static float __fastcall log2_(float x) {
      return static_cast<float>(log2_(static_cast<double>(x)));
    }
    static double __fastcall exp2_(double x);
    static float __fastcall exp2_(float x) {
      return static_cast<float>(exp2_(static_cast<double>(x)));
    }
    static float __fastcall log_(float x) {
      return static_cast<float>(log(x));
    }
    static double __fastcall log_(double x) {
      return log(x);
    }
    static float __fastcall log10_(float x) {
      return static_cast<float>(log10(x));
    }
    static double __fastcall log10_(double x) {
      return log10(x);
    }
    static float __fastcall exp_(float x) {
      return static_cast<float>(exp(x));
    }
    static double __fastcall exp_(double x) {
      return exp(x);
    }

    static short __fastcall ftol_round_n32768_32767_(float x) {
      return static_cast<short>(x + (x < 0.0f ? -0.5f : 0.5f));
    }
    static short __fastcall ftol_n32767_32767_(float x) {
      return static_cast<short>(x);
    }
    static unsigned char __fastcall ftol_round_0_256_(float x) {
      ASSERT(x >= -0.5f);
      ASSERT(x <= 255.4999f);
      x += 512.5f;
      return static_cast<unsigned char>(*reinterpret_cast<unsigned long *>(&x) >> 14);
    }
    static unsigned char __fastcall ftol_0_256_(float x) {
      ASSERT(x >= 0.0f);
      ASSERT(x <= 255.9999f);
      x += 512.0f;
      return static_cast<unsigned char>(*reinterpret_cast<unsigned long *>(&x) >> 14);
    }
    static unsigned char __fastcall ftol_0_1_(float x) {
      return static_cast<unsigned char>(x);
    }

    static __int64 __fastcall iabs_(__int64 x) { return x < 0 ? -x : x; }
    static long __fastcall iabs_(long x) { return x < 0 ? -x : x; }
    static short __fastcall iabs_(short x) { return x < 0 ? -x : x; }
    static char __fastcall iabs_(char x) { return x < 0 ? -x : x; }
    static __int64 __fastcall inabs_(__int64 x) { return x > 0 ? -x : x; }
    static long __fastcall inabs_(long x) { return x > 0 ? -x : x; }
    static short __fastcall inabs_(short x) { return x > 0 ? -x : x; }
    static char __fastcall inabs_(char x) { return x > 0 ? -x : x; }
    static float __fastcall fabs_(float x) { return static_cast<float>(fabs(x)); }
    static double __fastcall fabs_(double x) { return fabs(x); }
    static float __fastcall fnabs_(float x) { return x > 0.0f ? -x : x; }
    static double __fastcall fnabs_(double x) { return x > 0.0 ? -x : x; }
    static float __fastcall fmod_(float x, float y) { return static_cast<float>(fmod(x, y)); }
    static double __fastcall fmod_(double x, double y) { return fmod(x, y); }

    static bool __fastcall fequalz_(float x, float y, float e) { return fabs_(x - y) < e; }
    static bool __fastcall fequalz_(double x, double y, double e) { return fabs_(x - y) < e; }
    static bool __fastcall fequal_(float x, float y) { return fabs_(x - y) < 0.00000023841858f; }
    static bool __fastcall fequal_(double x, double y) { return fabs_(x - y) < 0.00000000000000044408921; }
    static bool __fastcall fequal4_(float x, float y) { return fabs_(x - y) < 0.00000095367432f; }
    static bool __fastcall fequal4_(double x, double y) { return fabs_(x - y) < 0.00000000000000177635684; }
    static bool __fastcall fequal8_(float x, float y) { return fabs_(x - y) < 0.00000190734863f; }
    static bool __fastcall fequal8_(double x, double y) { return fabs_(x - y) < 0.00000000000000355271368; }
    static bool __fastcall fnotequalz_(float x, float y, float e) { return !fequalz_(x, y, e); }
    static bool __fastcall fnotequalz_(double x, double y, double e) { return !fequalz_(x, y, e); }
    static bool __fastcall fnotequal_(float x, float y) { return !fequal_(x, y); }
    static bool __fastcall fnotequal_(double x, double y) { return !fequal_(x, y); }
    static bool __fastcall fnotequal4_(float x, float y) { return !fequal4_(x, y); }
    static bool __fastcall fnotequal4_(double x, double y) { return !fequal4_(x, y); }
    static bool __fastcall fnotequal8_(float x, float y) { return !fequal8_(x, y); }
    static bool __fastcall fnotequal8_(double x, double y) { return !fequal8_(x, y); }
    static float __fastcall fcleanupz_(float x, float y, float e) { return fequalz_(x, y, e) ? y : x; }
    static double __fastcall fcleanupz_(double x, double y, double e) { return fequalz_(x, y, e) ? y : x; }
    static float __fastcall fcleanup_(float x, float y) { return fequal_(x, y) ? y : x; }
    static double __fastcall fcleanup_(double x, double y) { return fequal_(x, y) ? y : x; }
    static float __fastcall fcleanup4_(float x, float y) { return fequal4_(x, y) ? y : x; }
    static double __fastcall fcleanup4_(double x, double y) { return fequal4_(x, y) ? y : x; }
    static float __fastcall fcleanup8_(float x, float y) { return fequal8_(x, y) ? y : x; }
    static double __fastcall fcleanup8_(double x, double y) { return fequal8_(x, y) ? y : x; }

    static unsigned long __fastcall fuint_n(float r) {
      ASSERT(r >= .0f);
      return static_cast<unsigned long>(r + 0.5f);
    }
    static unsigned long __fastcall fuint_(float r) { return static_cast<unsigned long>(r); }
    static unsigned long __fastcall fuint_pi(float r) { return static_cast<unsigned long>(r + 0.5f); }
    static long __fastcall fint_(float x) { return static_cast<long>(x); }
    static long __fastcall fint_n(float x) { return static_cast<long>(x + (x < 0.0f ? -0.5f : 0.5f)); }
    static long __fastcall fint_pi(float x) { return static_cast<long>(x + 0.5f); }
    static long __fastcall fint_mi(float x) { return static_cast<long>(x - 0.5f); }
    static long __fastcall fint_si(float x) { return static_cast<long>(x + (x < 0.0f ? -0.5f : 0.5f)); }
    static float __fastcall int32asreal_(long x) { return *reinterpret_cast<float *>(&x); }
    static long __fastcall realasint32_(float x) { return *reinterpret_cast<long *>(&x); }
    static double __fastcall int64aslreal_(__int64 x) { return *reinterpret_cast<double *>(&x); }
    static __int64 __fastcall lrealasint64_(double x) { return *reinterpret_cast<__int64 *>(&x); }

    static unsigned long __fastcall rotl_(unsigned long x, unsigned long n) { return x << n | x >> (32 - n); }
    static unsigned long __fastcall rotr_(unsigned long x, unsigned long n) { return x >> n | x << (32 - n); }
    static unsigned long __fastcall rotl1_(unsigned long x) { return rotl_(x, 1); }
    static unsigned long __fastcall rotl2_(unsigned long x) { return rotl_(x, 2); }
    static unsigned long __fastcall rotl3_(unsigned long x) { return rotl_(x, 3); }
    static unsigned long __fastcall rotl4_(unsigned long x) { return rotl_(x, 4); }
    static unsigned long __fastcall rotl5_(unsigned long x) { return rotl_(x, 5); }
    static unsigned long __fastcall rotl6_(unsigned long x) { return rotl_(x, 6); }
    static unsigned long __fastcall rotl7_(unsigned long x) { return rotl_(x, 7); }
    static unsigned long __fastcall rotl8_(unsigned long x) { return rotl_(x, 8); }
    static unsigned long __fastcall rotl9_(unsigned long x) { return rotl_(x, 9); }
    static unsigned long __fastcall rotl10_(unsigned long x) { return rotl_(x, 10); }
    static unsigned long __fastcall rotl11_(unsigned long x) { return rotl_(x, 11); }
    static unsigned long __fastcall rotl12_(unsigned long x) { return rotl_(x, 12); }
    static unsigned long __fastcall rotl13_(unsigned long x) { return rotl_(x, 13); }
    static unsigned long __fastcall rotl14_(unsigned long x) { return rotl_(x, 14); }
    static unsigned long __fastcall rotl15_(unsigned long x) { return rotl_(x, 15); }
    static unsigned long __fastcall rotl16_(unsigned long x) { return rotl_(x, 16); }
    static unsigned long __fastcall rotr1_(unsigned long x) { return rotr_(x, 1); }
    static unsigned long __fastcall rotr2_(unsigned long x) { return rotr_(x, 2); }
    static unsigned long __fastcall rotr3_(unsigned long x) { return rotr_(x, 3); }
    static unsigned long __fastcall rotr4_(unsigned long x) { return rotr_(x, 4); }
    static unsigned long __fastcall rotr5_(unsigned long x) { return rotr_(x, 5); }
    static unsigned long __fastcall rotr6_(unsigned long x) { return rotr_(x, 6); }
    static unsigned long __fastcall rotr7_(unsigned long x) { return rotr_(x, 7); }
    static unsigned long __fastcall rotr8_(unsigned long x) { return rotr_(x, 8); }
    static unsigned long __fastcall rotr9_(unsigned long x) { return rotr_(x, 9); }
    static unsigned long __fastcall rotr10_(unsigned long x) { return rotr_(x, 10); }
    static unsigned long __fastcall rotr11_(unsigned long x) { return rotr_(x, 11); }
    static unsigned long __fastcall rotr12_(unsigned long x) { return rotr_(x, 12); }
    static unsigned long __fastcall rotr13_(unsigned long x) { return rotr_(x, 13); }
    static unsigned long __fastcall rotr14_(unsigned long x) { return rotr_(x, 14); }
    static unsigned long __fastcall rotr15_(unsigned long x) { return rotr_(x, 15); }
    static unsigned long __fastcall rotr16_(unsigned long x) { return rotr_(x, 16); }

    static float __fastcall cos_(float x) { return static_cast<float>(cos(x)); }
    static double __fastcall cos_(double x) { return cos(x); }
    static float __fastcall sin_(float x) { return static_cast<float>(sin(x)); }
    static double __fastcall sin_(double x) { return sin(x); }
    static void __fastcall sincos_(float x, float &s, float &c) { s = sin_(x); c = cos_(x); }
    static void __fastcall sincos_(double x, double &s, double &c) { s = sin_(x); c = cos_(x); }
    static float __fastcall tan_(float x) { return static_cast<float>(tan(x)); }
    static double __fastcall tan_(double x) { return tan(x); }
    static float __fastcall acos_(float x) { return static_cast<float>(acos(x)); }
    static double __fastcall acos_(double x) { return acos(x); }
    static float __fastcall asin_(float x) { return static_cast<float>(asin(x)); }
    static double __fastcall asin_(double x) { return asin(x); }
    static float __fastcall atan_(float x) { return static_cast<float>(atan(x)); }
    static double __fastcall atan_(double x) { return atan(x); }
    static float __fastcall atan2_(float y, float x) { return static_cast<float>(atan2(y, x)); }
    static double __fastcall atan2_(double y, double x) { return atan2(y, x); }
    static float __fastcall sinoid_(float x, float oneOverPi);
    static float __fastcall cosoid_(float x, float oneOverPi);
    static float __fastcall atanoid_(float x, float piOverTwo);
    static float __fastcall pow_(float x, float y) { return static_cast<float>(pow(x, y)); }
    static double __fastcall pow_(double x, double y) { return pow(x, y); }

    static float __fastcall hypot_(float x, float y) { return sqrt_(x * x + y * y); }
    static double __fastcall hypot_(double x, double y) { return sqrt_(x * x + y * y); }
    static float __fastcall hypot_(float x, float y, float z) { return sqrt_(x * x + y * y + z * z); }
    static double __fastcall hypot_(double x, double y, double z) { return sqrt_(x * x + y * y + z * z); }
    static float __fastcall hypot_(float x, float y, float z, float w) { return sqrt_(x * x + y * y + z * z + w * w); }
    static double __fastcall hypot_(double x, double y, double z, double w) { return sqrt_(x * x + y * y + z * z + w * w); }
    static float __fastcall hypotinv_(float x, float y) { return sqrtinv_(x * x + y * y); }
    static double __fastcall hypotinv_(double x, double y) { return sqrtinv_(x * x + y * y); }
    static float __fastcall hypotinv_(float x, float y, float z) { return sqrtinv_(x * x + y * y + z * z); }
    static double __fastcall hypotinv_(double x, double y, double z) { return sqrtinv_(x * x + y * y + z * z); }
    static float __fastcall hypotinv_(float x, float y, float z, float w) { return sqrtinv_(x * x + y * y + z * z + w * w); }
    static double __fastcall hypotinv_(double x, double y, double z, double w) { return sqrtinv_(x * x + y * y + z * z + w * w); }
    static bool __fastcall solvequad_(float a, float b, float c, float &r1, float &r2);
    static bool __fastcall solvequad_(double a, double b, double c, double &r1, double &r2);
    static void __fastcall normalize_(float &x, float &y);
    static void __fastcall normalize_(double &x, double &y);
    static void __fastcall normalize_(float &x, float &y, float &z);
    static void __fastcall normalize_(double &x, double &y, double &z);
    static bool __fastcall xsectunitsphere_(double x, double y, double z, double dx, double dy, double dz, double r2);
    static bool __fastcall xsectunitcube_(double x, double y, double z, double dx, double dy, double dz) {
      double minimum = 0.0;
      double maximum = HUGE_VAL;
      double origins[3] = { x, y, z };
      double directions[3] = { dx, dy, dz };
      for (unsigned long i = 0; i < 3; ++i) {
        if (directions[i] == 0.0) {
          if (origins[i] < -1.0 || origins[i] > 1.0) {
            return false;
          }
        } else {
          double first = (-1.0 - origins[i]) / directions[i];
          double second = (1.0 - origins[i]) / directions[i];
          if (first > second) {
            double exchange = first;
            first = second;
            second = exchange;
          }
          if (first > minimum) minimum = first;
          if (second < maximum) maximum = second;
          if (minimum > maximum) return false;
        }
      }
      return maximum >= 0.0;
    }

    static float __fastcall frsqrte_(float x, unsigned long magic);
    static double __fastcall frsqrte_(double x, unsigned long magic);
    static float __fastcall frsqrte_(float *x, unsigned long magic);
    static double __fastcall frsqrte_(double *x, unsigned long magic);
    static float __fastcall fres_(float x, unsigned long magic);
    static double __fastcall fres_(double x, unsigned long magic);
    static float __fastcall fres_(float *x, unsigned long magic);
    static double __fastcall fres_(double *x, unsigned long magic);
    static long __fastcall mulhw_(long x, long y);
    static unsigned long __fastcall mulhwu_(unsigned long x, unsigned long y);
    static long __fastcall div3_(long x);
    static unsigned long __fastcall div3_(unsigned long x);
    static long __fastcall div5_(long x);
    static unsigned long __fastcall div5_(unsigned long x);
    static long __fastcall div9_(long x);
    static unsigned long __fastcall div9_(unsigned long x);
    static long __fastcall min_(long a, long b, long c);
    static long __fastcall med_(long a, long b, long c);
    static long __fastcall max_(long a, long b, long c);
    static long __fastcall span_(long a, long b, long c);
    static long __fastcall mean_(long a, long b, long c);
    static long __fastcall min_(long a, long b, long c, long d, long e);
    static long __fastcall med_(long a, long b, long c, long d, long e);
    static long __fastcall max_(long a, long b, long c, long d, long e);
    static long __fastcall span_(long a, long b, long c, long d, long e);
    static long __fastcall mean_(long a, long b, long c, long d, long e);
    static long __fastcall min_(long a, long b, long c, long d, long e, long f, long g, long h, long i);
    static long __fastcall med_(long a, long b, long c, long d, long e, long f, long g, long h, long i);
    static long __fastcall max_(long a, long b, long c, long d, long e, long f, long g, long h, long i);
    static long __fastcall span_(long a, long b, long c, long d, long e, long f, long g, long h, long i);
    static long __fastcall mean_(long a, long b, long c, long d, long e, long f, long g, long h, long i);

    static unsigned long __fastcall sqrt_(unsigned long x);
    static float __fastcall sqrt_(float x) {
      ASSERT(x >= .0f);
      return static_cast<float>(sqrt(x));
    }
    static double __fastcall sqrt_(double x) {
      ASSERT(x >= .0f);
      return sqrt(x);
    }
    static float __fastcall sqrt_(float x, float y) { return sqrt_(x * x + y * y); }
    static double __fastcall sqrt_(double x, double y) { return sqrt_(x * x + y * y); }
    static float __fastcall sqrtinv_(float x) { return 1.0f / sqrt_(x); }
    static double __fastcall sqrtinv_(double x) { return 1.0 / sqrt_(x); }
    static float __fastcall sqrtx_(float x) { return x * sqrt_(x); }
    static double __fastcall sqrtx_(double x) { return x * sqrt_(x); }
    static float __fastcall sqrtxinv_(float x) { return x * sqrtinv_(x); }
    static double __fastcall sqrtxinv_(double x) { return x * sqrtinv_(x); }
    static int __fastcall isnan_(double x) { return _isnan(x); }
    static int __fastcall isinf_(double x) { return !_finite(x); }
    static void __fastcall invertarray_(double *a, unsigned long n);
    static void __fastcall sqrtarray_(double *a, unsigned long n);
    static void __fastcall sqrtinvarray_(double *a, unsigned long n);
    static float __fastcall cbrt_(float x) { return pow_(x, 1.0f / 3.0f); }
    static double __fastcall cbrt_(double x) { return pow_(x, 1.0 / 3.0); }
    static unsigned long __fastcall cntlzw_(unsigned long x) {
      unsigned long n = 0;
      while (n < 32 && !(x & 0x80000000)) {
        ++n;
        x <<= 1;
      }
      return n;
    }
    static void __fastcall split_(float x, float &fraction, long &integer);
    static void __fastcall split_(double x, double &fraction, long &integer);
    static void __fastcall splitr_(float x, float &fraction, float &integer);
    static void __fastcall splitr_(double x, double &fraction, double &integer);
    static float __fastcall copysign_(float x, float y) { return static_cast<float>(_copysign(x, y)); }
    static double __fastcall copysign_(double x, double y) { return _copysign(x, y); }
    static long __fastcall iclamp_(long x, long low, long high) { return x < low ? low : x > high ? high : x; }
    static long __fastcall iclamp_(long x, unsigned long high) { return x < 0 ? 0 : static_cast<unsigned long>(x) > high ? high : x; }
    static void __fastcall iclamp_x(long &x, long low, long high) { x = iclamp_(x, low, high); }
    static void __fastcall iclamp_x(unsigned long &x, long low, long high) { x = iclamp_(x, low, high); }
    static void __fastcall iclamp_x(long &x, unsigned long high) { x = iclamp_(x, high); }
    static void __fastcall iclamp_x(unsigned long &x, unsigned long high) { if (x > high) x = high; }
    static float __fastcall clamp_(float x, float low, float high) { return x < low ? low : x > high ? high : x; }
    static double __fastcall clamp_(double x, double low, double high) { return x < low ? low : x > high ? high : x; }
    static void __fastcall clamp_x(float &x, float low, float high) { x = clamp_(x, low, high); }
    static void __fastcall clamp_x(double &x, double low, double high) { x = clamp_(x, low, high); }
    static float __fastcall step_(float x, float a);
    static float __fastcall pulse_(float x, float a, float b);
    static float __fastcall bstep_(float x, float a, float b);
    static float __fastcall smoothstep_(float x, float a, float b);
    static double __fastcall gammai_(float x, float g);
    static double __fastcall gamma_(float x, float g);
    static double __fastcall bias_(float x, float g);
    static double __fastcall gain_(float x, float g);
    static float __fastcall sinc_(float x);
    static double __fastcall sinc_(double x);
    static float __fastcall sinc_(float x, float a);
    static double __fastcall sinc_(double x, double a);
    static float __fastcall spline_(float x, float *k, unsigned long n);
    static double __fastcall spline_(double x, double *k, unsigned long n);
    static void Initialize();
    static void Terminate();
  };

}
