#include <Base/Base.h>

#include "Tempest/cmath.h"

namespace NTempest {

  float CMath::sinoid_(float x, const float oneOverPi) {
    float fraction;
    long integer;
    split_(x * oneOverPi - 0.5f, fraction, integer);
    float result = 1.0f - (6.0f - fraction * 4.0f) * fraction * fraction;
    return integer & 1 ? -result : result;
  }

  float CMath::cosoid_(float x, const float oneOverPi) {
    float fraction;
    long integer;
    split_(x * oneOverPi, fraction, integer);
    float result = 1.0f - (6.0f - fraction * 4.0f) * fraction * fraction;
    return integer & 1 ? -result : result;
  }

  long CMath::mulhw_(long x, long y) {
    return static_cast<long>((static_cast<__int64>(x) * y) >> 32);
  }

  unsigned long CMath::mulhwu_(unsigned long x, unsigned long y) {
    return static_cast<unsigned long>((static_cast<unsigned __int64>(x) * y) >> 32);
  }

  unsigned long CMath::div3_(unsigned long n) { return mulhwu_(0xAAAAAAAB, n) >> 1; }
  long CMath::div3_(long n) { return mulhw_(1431655766, n); }

  unsigned long CMath::div5_(unsigned long x) {
    unsigned long q = ((3 * x >> 4) + 3 * x) >> 8;
    return ((q + (3 * x >> 4) + 3 * x >> 16) + q + (3 * x >> 4) + 3 * x + 2) >> 4;
  }

  long CMath::div5_(long x) {
    long q = (((3 * x) >> 4) + 3 * x) >> 8;
    return (((q + ((3 * x) >> 4) + 3 * x) >> 16) + q + ((3 * x) >> 4) + 3 * x + 15) >> 4;
  }

  unsigned long CMath::div9_(unsigned long x) {
    unsigned long q = ((7 * x >> 6) + 7 * x) >> 12;
    return ((q + (7 * x >> 6) + 7 * x >> 24) + q + (7 * x >> 6) + 7 * x + 2) >> 6;
  }

  long CMath::div9_(long x) {
    long q = (((7 * x) >> 6) + 7 * x) >> 12;
    return (((q + ((7 * x) >> 6) + 7 * x) >> 24) + q + ((7 * x) >> 6) + 7 * x + 62) >> 6;
  }

  long CMath::min_(long a, long b, long c) { return a < b ? (a < c ? a : c) : (b < c ? b : c); }
  long CMath::med_(long a, long b, long c) {
    if (((a - b) ^ (c - b)) < 0) return b;
    if (((a - b) ^ (a - c)) < 0) return a;
    return c;
  }
  long CMath::max_(long a, long b, long c) { return a > b ? (a > c ? a : c) : (b > c ? b : c); }
  long CMath::span_(long a, long b, long c) { return max_(a, b, c) - min_(a, b, c); }
  long CMath::mean_(long a, long b, long c) { return div3_(a + b + c); }
  long CMath::min_(long a, long b, long c, long d, long e) { return min_(min_(a, b, c), d, e); }
  long CMath::med_(long a, long b, long c, long d, long e) {
    long ablo = a < b ? a : b, abhi = a > b ? a : b;
    long deLo = d < e ? d : e, deHi = d > e ? d : e;
    return med_(abhi < deHi ? abhi : deHi, c, ablo > deLo ? ablo : deLo);
  }
  long CMath::max_(long a, long b, long c, long d, long e) { return max_(max_(a, b, c), d, e); }
  long CMath::span_(long a, long b, long c, long d, long e) { return max_(a, b, c, d, e) - min_(a, b, c, d, e); }
  long CMath::mean_(long a, long b, long c, long d, long e) { return div5_(a + b + c + d + e); }
  long CMath::min_(long a, long b, long c, long d, long e, long f, long g, long h, long i) {
    return min_(min_(a, b, c, d, e), min_(f, g, h), i);
  }
  long CMath::med_(long a, long b, long c, long d, long e, long f, long g, long h, long i) {
    long v[9] = { a, b, c, d, e, f, g, h, i };
    for (unsigned long p = 0; p < 5; ++p)
      for (unsigned long q = p + 1; q < 9; ++q)
        if (v[q] < v[p]) { long t = v[p]; v[p] = v[q]; v[q] = t; }
    return v[4];
  }
  long CMath::max_(long a, long b, long c, long d, long e, long f, long g, long h, long i) {
    return max_(max_(a, b, c, d, e), max_(f, g, h), i);
  }
  long CMath::span_(long a, long b, long c, long d, long e, long f, long g, long h, long i) {
    return max_(a, b, c, d, e, f, g, h, i) - min_(a, b, c, d, e, f, g, h, i);
  }
  long CMath::mean_(long a, long b, long c, long d, long e, long f, long g, long h, long i) {
    return div9_(a + b + c + d + e + f + g + h + i);
  }

  void CMath::normalize_(double &x, double &y) {
    double inverse = 1.0 / sqrt_(x * x + y * y);
    x *= inverse; y *= inverse;
  }
  void CMath::normalize_(float &x, float &y) {
    float inverse = 1.0f / sqrt_(x * x + y * y);
    x *= inverse; y *= inverse;
  }
  void CMath::normalize_(double &x, double &y, double &z) {
    double inverse = 1.0 / sqrt_(x * x + y * y + z * z);
    x *= inverse; y *= inverse; z *= inverse;
  }
  void CMath::normalize_(float &x, float &y, float &z) {
    float inverse = 1.0f / sqrt_(x * x + y * y + z * z);
    x *= inverse; y *= inverse; z *= inverse;
  }

  float CMath::frsqrte_(float x, unsigned long magic) {
    unsigned long bits = *reinterpret_cast<unsigned long *>(&x);
    bits = magic - ((bits >> 1) & 0x3FFFFFFF);
    return *reinterpret_cast<float *>(&bits);
  }
  double CMath::frsqrte_(double x, unsigned long magic) {
    reinterpret_cast<unsigned long *>(&x)[1] = magic - ((reinterpret_cast<unsigned long *>(&x)[1] >> 1) & 0x3FFFFFFF);
    return x;
  }
  float CMath::frsqrte_(float *x, unsigned long magic) { *x = frsqrte_(*x, magic); return *x; }
  double CMath::frsqrte_(double *x, unsigned long magic) { *x = frsqrte_(*x, magic); return *x; }
  float CMath::fres_(float x, unsigned long magic) {
    unsigned long bits = magic - *reinterpret_cast<unsigned long *>(&x);
    return *reinterpret_cast<float *>(&bits);
  }
  double CMath::fres_(double x, unsigned long magic) {
    reinterpret_cast<unsigned long *>(&x)[1] = magic - reinterpret_cast<unsigned long *>(&x)[1];
    return x;
  }
  float CMath::fres_(float *x, unsigned long magic) { *x = fres_(*x, magic); return *x; }
  double CMath::fres_(double *x, unsigned long magic) { *x = fres_(*x, magic); return *x; }

  void CMath::split_(double x, double &xf, long &xi) {
    xi = static_cast<long>(x);
    if (x < 0.0) --xi;
    xf = x - xi;
  }
  void CMath::split_(float x, float &xf, long &xi) {
    xi = static_cast<long>(x);
    if (x < 0.0f) --xi;
    xf = x - xi;
  }
  void CMath::splitr_(double x, double &xf, double &xi) {
    xi = static_cast<long>(x);
    if (x < 0.0) xi -= 1.0;
    xf = x - xi;
  }
  void CMath::splitr_(float x, float &xf, float &xi) {
    xi = static_cast<float>(static_cast<long>(x));
    if (x < 0.0f) xi -= 1.0f;
    xf = x - xi;
  }

  float CMath::step_(float x, float a) { return x >= a ? 1.0f : 0.0f; }
  float CMath::pulse_(float x, float a, float b) { return step_(x, a) - step_(x, b); }
  float CMath::bstep_(float x, float a, float b) { return clamp_((x - a) / (b - a), 0.0f, 1.0f); }
  float CMath::smoothstep_(float x, float a, float b) {
    if (x < a) return 0.0f;
    if (x >= b) return 1.0f;
    float t = (x - a) / (b - a);
    return t * (3.0f - 2.0f * t) * t;
  }
  double CMath::gammai_(float x, float g) { return pow(x, g); }
  double CMath::gamma_(float x, float g) { return gammai_(x, 1.0f / g); }
  double CMath::bias_(float x, float g) { return pow(x, -log2_(static_cast<double>(g))); }
  double CMath::gain_(float x, float g) {
    return x < 0.5f ? bias_(x + x, 1.0f - g) * 0.5 : 1.0 - bias_(2.0f - x - x, 1.0f - g) * 0.5;
  }
  double CMath::sinc_(double x, double a) { double v = x * a * 3.141592653589793; return sin(v) / v; }
  double CMath::sinc_(double x) { double v = x * 3.141592653589793; return sin(v) / v; }
  float CMath::sinc_(float x, float a) { float v = x * a * 3.1415927f; return static_cast<float>(sin(v) / v); }
  float CMath::sinc_(float x) { float v = x * 3.1415927f; return static_cast<float>(sin(v) / v); }

}
