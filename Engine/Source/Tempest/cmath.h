#pragma once

#include <storm.h>
#include <float.h>
#include <math.h>

namespace NTempest {

  class CMath {
   protected:
    static unsigned long left1_(unsigned long x) {
      return x << 1;
    }

   public:
    static double logoid_(double x, const double a, const double b, const double c, const double d, const double ln2);
    static double logoid2_(double x, const double a, const double b, const double c, const double d);
    static double logoid10_(double x, const double a, const double b, const double c, const double d, const double ln10);
    static double log2_(double x);
    static float log2_(float x) {
      return static_cast<float>(log2_(static_cast<double>(x)));
    }
    static double exp2_(double x);
    static float exp2_(float x) {
      return static_cast<float>(exp2_(static_cast<double>(x)));
    }
    static float log_(float x) {
      return static_cast<float>(log(x));
    }
    static double log_(double x) {
      return log(x);
    }
    static float log10_(float x) {
      return static_cast<float>(log10(x));
    }
    static double log10_(double x) {
      return log10(x);
    }
    static float exp_(float x) {
      return static_cast<float>(exp(x));
    }
    static double exp_(double x) {
      return exp(x);
    }

    static short ftol_round_n32768_32767_(float x) {
      return static_cast<short>(x + (x < 0.0f ? -0.5f : 0.5f));
    }
    static short ftol_n32767_32767_(float x) {
      return static_cast<short>(x);
    }
    static unsigned char ftol_round_0_256_(float x) {
      ASSERT(x >= -0.5f);
      ASSERT(x <= 255.4999f);
      x += 512.5f;
      return static_cast<unsigned char>(*reinterpret_cast<unsigned long *>(&x) >> 14);
    }
    static unsigned char ftol_0_256_(float x) {
      ASSERT(x >= 0.0f);
      ASSERT(x <= 255.9999f);
      x += 512.0f;
      return static_cast<unsigned char>(*reinterpret_cast<unsigned long *>(&x) >> 14);
    }
    static unsigned char ftol_0_1_(float x) {
      return static_cast<unsigned char>(x);
    }

    static __int64 iabs_(__int64 x) { return x < 0 ? -x : x; }
    static long iabs_(long x) { return x < 0 ? -x : x; }
    static short iabs_(short x) { return x < 0 ? -x : x; }
    static char iabs_(char x) { return x < 0 ? -x : x; }
    static __int64 inabs_(__int64 x) { return x > 0 ? -x : x; }
    static long inabs_(long x) { return x > 0 ? -x : x; }
    static short inabs_(short x) { return x > 0 ? -x : x; }
    static char inabs_(char x) { return x > 0 ? -x : x; }
    static float fabs_(float x) { return static_cast<float>(fabs(x)); }
    static double fabs_(double x) { return fabs(x); }
    static float fnabs_(float x) { return x > 0.0f ? -x : x; }
    static double fnabs_(double x) { return x > 0.0 ? -x : x; }
    static float fmod_(float x, float y) { return static_cast<float>(fmod(x, y)); }
    static double fmod_(double x, double y) { return fmod(x, y); }

    static bool fequalz_(float x, float y, float e) { return fabs_(x - y) < e; }
    static bool fequalz_(double x, double y, double e) { return fabs_(x - y) < e; }
    static bool fequal_(float x, float y) { return fabs_(x - y) < 0.00000023841858f; }
    static bool fequal_(double x, double y) { return fabs_(x - y) < 0.00000000000000044408921; }
    static bool fequal4_(float x, float y) { return fabs_(x - y) < 0.00000095367432f; }
    static bool fequal4_(double x, double y) { return fabs_(x - y) < 0.00000000000000177635684; }
    static bool fequal8_(float x, float y) { return fabs_(x - y) < 0.00000190734863f; }
    static bool fequal8_(double x, double y) { return fabs_(x - y) < 0.00000000000000355271368; }
    static bool fnotequalz_(float x, float y, float e) { return !fequalz_(x, y, e); }
    static bool fnotequalz_(double x, double y, double e) { return !fequalz_(x, y, e); }
    static bool fnotequal_(float x, float y) { return !fequal_(x, y); }
    static bool fnotequal_(double x, double y) { return !fequal_(x, y); }
    static bool fnotequal4_(float x, float y) { return !fequal4_(x, y); }
    static bool fnotequal4_(double x, double y) { return !fequal4_(x, y); }
    static bool fnotequal8_(float x, float y) { return !fequal8_(x, y); }
    static bool fnotequal8_(double x, double y) { return !fequal8_(x, y); }
    static float fcleanupz_(float x, float y, float e) { return fequalz_(x, y, e) ? y : x; }
    static double fcleanupz_(double x, double y, double e) { return fequalz_(x, y, e) ? y : x; }
    static float fcleanup_(float x, float y) { return fequal_(x, y) ? y : x; }
    static double fcleanup_(double x, double y) { return fequal_(x, y) ? y : x; }
    static float fcleanup4_(float x, float y) { return fequal4_(x, y) ? y : x; }
    static double fcleanup4_(double x, double y) { return fequal4_(x, y) ? y : x; }
    static float fcleanup8_(float x, float y) { return fequal8_(x, y) ? y : x; }
    static double fcleanup8_(double x, double y) { return fequal8_(x, y) ? y : x; }

    static unsigned long fuint_n(float r) {
      ASSERT(r >= .0f);
      return static_cast<unsigned long>(r + 0.5f);
    }
    static unsigned long fuint_(float r) { return static_cast<unsigned long>(r); }
    static unsigned long fuint_pi(float r) { return static_cast<unsigned long>(r + 0.5f); }
    static long fint_(float x) { return static_cast<long>(x); }
    static long fint_n(float x) { return static_cast<long>(x + (x < 0.0f ? -0.5f : 0.5f)); }
    static long fint_pi(float x) { return static_cast<long>(x + 0.5f); }
    static long fint_mi(float x) { return static_cast<long>(x - 0.5f); }
    static long fint_si(float x) { return static_cast<long>(x + (x < 0.0f ? -0.5f : 0.5f)); }
    static float int32asreal_(long x) { return *reinterpret_cast<float *>(&x); }
    static long realasint32_(float x) { return *reinterpret_cast<long *>(&x); }
    static double int64aslreal_(__int64 x) { return *reinterpret_cast<double *>(&x); }
    static __int64 lrealasint64_(double x) { return *reinterpret_cast<__int64 *>(&x); }

    static unsigned long rotl_(unsigned long x, unsigned long n) { return x << n | x >> (32 - n); }
    static unsigned long rotr_(unsigned long x, unsigned long n) { return x >> n | x << (32 - n); }
    static unsigned long rotl1_(unsigned long x) { return rotl_(x, 1); }
    static unsigned long rotl2_(unsigned long x) { return rotl_(x, 2); }
    static unsigned long rotl3_(unsigned long x) { return rotl_(x, 3); }
    static unsigned long rotl4_(unsigned long x) { return rotl_(x, 4); }
    static unsigned long rotl5_(unsigned long x) { return rotl_(x, 5); }
    static unsigned long rotl6_(unsigned long x) { return rotl_(x, 6); }
    static unsigned long rotl7_(unsigned long x) { return rotl_(x, 7); }
    static unsigned long rotl8_(unsigned long x) { return rotl_(x, 8); }
    static unsigned long rotl9_(unsigned long x) { return rotl_(x, 9); }
    static unsigned long rotl10_(unsigned long x) { return rotl_(x, 10); }
    static unsigned long rotl11_(unsigned long x) { return rotl_(x, 11); }
    static unsigned long rotl12_(unsigned long x) { return rotl_(x, 12); }
    static unsigned long rotl13_(unsigned long x) { return rotl_(x, 13); }
    static unsigned long rotl14_(unsigned long x) { return rotl_(x, 14); }
    static unsigned long rotl15_(unsigned long x) { return rotl_(x, 15); }
    static unsigned long rotl16_(unsigned long x) { return rotl_(x, 16); }
    static unsigned long rotr1_(unsigned long x) { return rotr_(x, 1); }
    static unsigned long rotr2_(unsigned long x) { return rotr_(x, 2); }
    static unsigned long rotr3_(unsigned long x) { return rotr_(x, 3); }
    static unsigned long rotr4_(unsigned long x) { return rotr_(x, 4); }
    static unsigned long rotr5_(unsigned long x) { return rotr_(x, 5); }
    static unsigned long rotr6_(unsigned long x) { return rotr_(x, 6); }
    static unsigned long rotr7_(unsigned long x) { return rotr_(x, 7); }
    static unsigned long rotr8_(unsigned long x) { return rotr_(x, 8); }
    static unsigned long rotr9_(unsigned long x) { return rotr_(x, 9); }
    static unsigned long rotr10_(unsigned long x) { return rotr_(x, 10); }
    static unsigned long rotr11_(unsigned long x) { return rotr_(x, 11); }
    static unsigned long rotr12_(unsigned long x) { return rotr_(x, 12); }
    static unsigned long rotr13_(unsigned long x) { return rotr_(x, 13); }
    static unsigned long rotr14_(unsigned long x) { return rotr_(x, 14); }
    static unsigned long rotr15_(unsigned long x) { return rotr_(x, 15); }
    static unsigned long rotr16_(unsigned long x) { return rotr_(x, 16); }

    static float cos_(float x) { return static_cast<float>(cos(x)); }
    static double cos_(double x) { return cos(x); }
    static float sin_(float x) { return static_cast<float>(sin(x)); }
    static double sin_(double x) { return sin(x); }
    static void sincos_(float x, float &s, float &c) { s = sin_(x); c = cos_(x); }
    static void sincos_(double x, double &s, double &c) { s = sin_(x); c = cos_(x); }
    static float tan_(float x) { return static_cast<float>(tan(x)); }
    static double tan_(double x) { return tan(x); }
    static float acos_(float x) { return static_cast<float>(acos(x)); }
    static double acos_(double x) { return acos(x); }
    static float asin_(float x) { return static_cast<float>(asin(x)); }
    static double asin_(double x) { return asin(x); }
    static float atan_(float x) { return static_cast<float>(atan(x)); }
    static double atan_(double x) { return atan(x); }
    static float atan2_(float y, float x) { return static_cast<float>(atan2(y, x)); }
    static double atan2_(double y, double x) { return atan2(y, x); }
    static float sinoid_(float x, const float oneOverPi);
    static float cosoid_(float x, const float oneOverPi);
    static float atanoid_(float x, const float piOverTwo);
    static float pow_(float x, float y) { return static_cast<float>(pow(x, y)); }
    static double pow_(double x, double y) { return pow(x, y); }

    static float hypot_(float x, float y) { return sqrt_(x * x + y * y); }
    static double hypot_(double x, double y) { return sqrt_(x * x + y * y); }
    static float hypot_(float x, float y, float z) { return sqrt_(x * x + y * y + z * z); }
    static double hypot_(double x, double y, double z) { return sqrt_(x * x + y * y + z * z); }
    static float hypot_(float x, float y, float z, float w) { return sqrt_(x * x + y * y + z * z + w * w); }
    static double hypot_(double x, double y, double z, double w) { return sqrt_(x * x + y * y + z * z + w * w); }
    static float hypotinv_(float x, float y) { return sqrtinv_(x * x + y * y); }
    static double hypotinv_(double x, double y) { return sqrtinv_(x * x + y * y); }
    static float hypotinv_(float x, float y, float z) { return sqrtinv_(x * x + y * y + z * z); }
    static double hypotinv_(double x, double y, double z) { return sqrtinv_(x * x + y * y + z * z); }
    static float hypotinv_(float x, float y, float z, float w) { return sqrtinv_(x * x + y * y + z * z + w * w); }
    static double hypotinv_(double x, double y, double z, double w) { return sqrtinv_(x * x + y * y + z * z + w * w); }
    static bool solvequad_(float a, float b, float c, float &r1, float &r2);
    static bool solvequad_(double a, double b, double c, double &r1, double &r2);
    static void normalize_(float &x, float &y);
    static void normalize_(double &x, double &y);
    static void normalize_(float &x, float &y, float &z);
    static void normalize_(double &x, double &y, double &z);
    static bool xsectunitsphere_(double x, double y, double z, double dx, double dy, double dz, const double r2);
    static bool xsectunitcube_(double x, double y, double z, double dx, double dy, double dz) {
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

    static float frsqrte_(float x, unsigned long magic);
    static double frsqrte_(double x, unsigned long magic);
    static float frsqrte_(float *x, unsigned long magic);
    static double frsqrte_(double *x, unsigned long magic);
    static float fres_(float x, unsigned long magic);
    static double fres_(double x, unsigned long magic);
    static float fres_(float *x, unsigned long magic);
    static double fres_(double *x, unsigned long magic);
    static long mulhw_(long x, long y);
    static unsigned long mulhwu_(unsigned long x, unsigned long y);
    static long div3_(long x);
    static unsigned long div3_(unsigned long x);
    static long div5_(long x);
    static unsigned long div5_(unsigned long x);
    static long div9_(long x);
    static unsigned long div9_(unsigned long x);
    static long min_(long a, long b, long c);
    static long med_(long a, long b, long c);
    static long max_(long a, long b, long c);
    static long span_(long a, long b, long c);
    static long mean_(long a, long b, long c);
    static long min_(long a, long b, long c, long d, long e);
    static long med_(long a, long b, long c, long d, long e);
    static long max_(long a, long b, long c, long d, long e);
    static long span_(long a, long b, long c, long d, long e);
    static long mean_(long a, long b, long c, long d, long e);
    static long min_(long a, long b, long c, long d, long e, long f, long g, long h, long i);
    static long med_(long a, long b, long c, long d, long e, long f, long g, long h, long i);
    static long max_(long a, long b, long c, long d, long e, long f, long g, long h, long i);
    static long span_(long a, long b, long c, long d, long e, long f, long g, long h, long i);
    static long mean_(long a, long b, long c, long d, long e, long f, long g, long h, long i);

    static unsigned long sqrt_(unsigned long x);
    static float sqrt_(float x) {
      ASSERT(x >= .0f);
      return static_cast<float>(sqrt(x));
    }
    static double sqrt_(double x) {
      ASSERT(x >= .0f);
      return sqrt(x);
    }
    static float sqrt_(float x, float y) { return sqrt_(x * x + y * y); }
    static double sqrt_(double x, double y) { return sqrt_(x * x + y * y); }
    static float sqrtinv_(float x) { return 1.0f / sqrt_(x); }
    static double sqrtinv_(double x) { return 1.0 / sqrt_(x); }
    static float sqrtx_(float x) { return x * sqrt_(x); }
    static double sqrtx_(double x) { return x * sqrt_(x); }
    static float sqrtxinv_(float x) { return x * sqrtinv_(x); }
    static double sqrtxinv_(double x) { return x * sqrtinv_(x); }
    static int isnan_(double x) { return _isnan(x); }
    static int isinf_(double x) { return !_finite(x); }
    static void invertarray_(double *a, unsigned long n);
    static void sqrtarray_(double *a, unsigned long n);
    static void sqrtinvarray_(double *a, unsigned long n);
    static float cbrt_(float x) { return pow_(x, 1.0f / 3.0f); }
    static double cbrt_(double x) { return pow_(x, 1.0 / 3.0); }
    static unsigned long cntlzw_(unsigned long x) {
      unsigned long n = 0;
      while (n < 32 && !(x & 0x80000000)) {
        ++n;
        x <<= 1;
      }
      return n;
    }
    static void split_(float x, float &fraction, long &integer);
    static void split_(double x, double &fraction, long &integer);
    static void splitr_(float x, float &fraction, float &integer);
    static void splitr_(double x, double &fraction, double &integer);
    static float copysign_(float x, float y) { return static_cast<float>(_copysign(x, y)); }
    static double copysign_(double x, double y) { return _copysign(x, y); }
    static long iclamp_(long x, long low, long high) { return x < low ? low : x > high ? high : x; }
    static long iclamp_(long x, unsigned long high) { return x < 0 ? 0 : static_cast<unsigned long>(x) > high ? high : x; }
    static void iclamp_x(long &x, long low, long high) { x = iclamp_(x, low, high); }
    static void iclamp_x(unsigned long &x, long low, long high) { x = iclamp_(x, low, high); }
    static void iclamp_x(long &x, unsigned long high) { x = iclamp_(x, high); }
    static void iclamp_x(unsigned long &x, unsigned long high) { if (x > high) x = high; }
    static float clamp_(float x, float low, float high) { return x < low ? low : x > high ? high : x; }
    static double clamp_(double x, double low, double high) { return x < low ? low : x > high ? high : x; }
    static void clamp_x(float &x, float low, float high) { x = clamp_(x, low, high); }
    static void clamp_x(double &x, double low, double high) { x = clamp_(x, low, high); }
    static float step_(float x, float a);
    static float pulse_(float x, float a, float b);
    static float bstep_(float x, float a, float b);
    static float smoothstep_(float x, float a, float b);
    static double gammai_(float x, float g);
    static double gamma_(float x, float g);
    static double bias_(float x, float g);
    static double gain_(float x, float g);
    static float sinc_(float x);
    static double sinc_(double x);
    static float sinc_(float x, float a);
    static double sinc_(double x, double a);
    static float spline_(float x, float *k, unsigned long n);
    static double spline_(double x, double *k, unsigned long n);
    static void Initialize();
    static void Terminate();
  };

}
