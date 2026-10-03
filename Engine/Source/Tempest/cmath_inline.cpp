#include <Base/Base.h>

#include "Tempest/cmath.h"

namespace NTempest {

  float CMath::sinoid_(float x, const float oneOverPi) {
    float fraction;
    long  integer;
    split_(x * oneOverPi - 0.5f, fraction, integer);
    float result = 1.0f - (6.0f - fraction * 4.0f) * fraction * fraction;
    return integer & 1 ? -result : result;
  }

  float CMath::cosoid_(float x, const float oneOverPi) {
    float fraction;
    long  integer;
    split_(x * oneOverPi, fraction, integer);
    float result = 1.0f - (6.0f - fraction * 4.0f) * fraction * fraction;
    return integer & 1 ? -result : result;
  }

  long CMath::mulhw_(long x, long y) {
    return static_cast<long>((static_cast<LONGLONG>(x) * y) >> 32);
  }

  DWORD CMath::mulhwu_(DWORD x, DWORD y) {
    return static_cast<DWORD>((static_cast<DWORDLONG>(x) * y) >> 32);
  }

  DWORD CMath::div3_(DWORD n) {
    return mulhwu_(0xAAAAAAAB, n) >> 1;
  }
  long CMath::div3_(long n) {
    return mulhw_(1431655766, n);
  }

  DWORD CMath::div5_(DWORD x) {
    x *= 3;
    x += x >> 4;
    x += x >> 8;
    x += x >> 16;
    return (x + 2) >> 4;
  }

  long CMath::div5_(long x) {
    x *= 3;
    x += x >> 4;
    x += x >> 8;
    x += x >> 16;
    return (x + 15) >> 4;
  }

  DWORD CMath::div9_(DWORD x) {
    x *= 7;
    x += x >> 6;
    x += x >> 12;
    x += x >> 24;
    return (x + 2) >> 6;
  }

  long CMath::div9_(long x) {
    x *= 7;
    x += x >> 6;
    x += x >> 12;
    x += x >> 24;
    return (x + 62) >> 6;
  }

  long CMath::min_(long a, long b, long c) {
    a = min(a, b);
    return min(a, c);
  }
  long CMath::med_(long a, long b, long c) {
    if (((a - b) ^ (c - b)) < 0)
      return b;
    if (((a - b) ^ (a - c)) < 0)
      return a;
    return c;
  }
  long CMath::max_(long a, long b, long c) {
    a = max(a, b);
    return max(a, c);
  }
  long CMath::span_(long a, long b, long c) {
    if (((c - b) ^ (a - b)) < 0)
      return c - a > 0 ? c - a : -(c - a);
    if (((a - c) ^ (a - b)) < 0)
      return c - b > 0 ? c - b : -(c - b);
    return a - b > 0 ? a - b : -(a - b);
  }
  long CMath::mean_(long a, long b, long c) {
    return div3_(a + b + c);
  }
  long CMath::min_(long a, long b, long c, long d, long e) {
    a = min(a, b);
    a = min(a, d);
    a = min(a, e);
    return min(a, c);
  }
  long CMath::med_(long a, long b, long c, long d, long e) {
    long ablo = a < b ? a : b, abhi = a > b ? a : b;
    long deLo = d < e ? d : e, deHi = d > e ? d : e;
    return med_(abhi < deHi ? abhi : deHi, c, ablo > deLo ? ablo : deLo);
  }
  long CMath::max_(long a, long b, long c, long d, long e) {
    a = max(a, b);
    a = max(a, d);
    a = max(a, e);
    return max(a, c);
  }
  long CMath::span_(long a, long b, long c, long d, long e) {
    long ablo = a < b ? a : b, abhi = a > b ? a : b;
    long deLo = d < e ? d : e, deHi = d > e ? d : e;
    return span_(deLo < ablo ? deLo : ablo, c, abhi > deHi ? abhi : deHi);
  }
  long CMath::mean_(long a, long b, long c, long d, long e) {
    return div5_(a + b + c + d + e);
  }
  long CMath::min_(long a, long b, long c, long d, long e, long f, long g, long h, long i) {
    a = min(a, c);
    a = min(a, g);
    a = min(a, i);
    a = min(a, b);
    a = min(a, d);
    a = min(a, f);
    a = min(a, h);
    return min(a, e);
  }
  long CMath::med_(long a, long b, long c, long d, long e, long f, long g, long h, long i) {
    long t;
    if ((t = d - a) < 0) {
      a += t;
      d -= t;
    }
    if ((t = e - b) < 0) {
      b += t;
      e -= t;
    }
    if ((t = f - c) < 0) {
      c += t;
      f -= t;
    }
    if ((t = b - a) < 0) {
      a += t;
      b -= t;
    }
    if ((t = c - a) < 0) {
      a += t;
      c -= t;
    }
    if ((t = f - d) < 0) {
      d += t;
      f -= t;
    }
    if ((t = f - e) < 0) {
      e += t;
      f -= t;
    }
    if ((t = c - b) < 0) {
      b += t;
      c -= t;
    }
    if ((t = e - d) < 0) {
      d += t;
      e -= t;
    }
    if ((t = d - b) < 0) {
      b += t;
      d -= t;
    }
    if ((t = g - b) < 0) {
      b += t;
      g -= t;
    }
    if ((t = g - c) < 0) {
      c += t;
      g -= t;
    }
    if ((t = g - e) < 0) {
      e += t;
      g -= t;
    }
    if ((t = d - c) < 0) {
      c += t;
      d -= t;
    }
    if ((t = h - e) < 0) {
      e += t;
      h -= t;
    }
    if ((t = e - c) < 0) {
      c += t;
      e -= t;
    }
    if ((t = h - d) < 0) {
      d += t;
      h -= t;
    }
    if ((t = e - d) < 0) {
      d += t;
      e -= t;
    }
    if ((t = i - d) < 0) {
      d += t;
      i -= t;
    }
    if ((t = i - e) < 0) {
      e += t;
      i -= t;
    }
    return e;
  }
  long CMath::max_(long a, long b, long c, long d, long e, long f, long g, long h, long i) {
    a = max(a, c);
    a = max(a, g);
    a = max(a, i);
    a = max(a, b);
    a = max(a, d);
    a = max(a, f);
    a = max(a, h);
    return max(a, e);
  }
  long CMath::span_(long a, long b, long c, long d, long e, long f, long g, long h, long i) {
    long  lo;
    DWORD range;
    if (a > i) {
      lo = i;
      range = a - i;
    } else {
      lo = a;
      range = i - a;
      a = i;
    }
    if (static_cast<DWORD>(c - lo) > range) {
      if (c < lo)
        lo = c;
      else
        a = c;
      range = a - lo;
    }
    if (static_cast<DWORD>(g - lo) > range) {
      if (g < lo)
        lo = g;
      else
        a = g;
      range = a - lo;
    }
    if (static_cast<DWORD>(b - lo) > range) {
      if (b < lo)
        lo = b;
      else
        a = b;
      range = a - lo;
    }
    if (static_cast<DWORD>(d - lo) > range) {
      if (d < lo)
        lo = d;
      else
        a = d;
      range = a - lo;
    }
    if (static_cast<DWORD>(f - lo) > range) {
      if (f < lo)
        lo = f;
      else
        a = f;
      range = a - lo;
    }
    if (static_cast<DWORD>(h - lo) > range) {
      if (h < lo)
        lo = h;
      else
        a = h;
      range = a - lo;
    }
    if (static_cast<DWORD>(e - lo) > range) {
      if (e < lo)
        lo = e;
      else
        a = e;
      range = a - lo;
    }
    return range;
  }
  long CMath::mean_(long a, long b, long c, long d, long e, long f, long g, long h, long i) {
    return div9_(a + b + c + d + e + f + g + h + i);
  }

  void CMath::normalize_(double &x, double &y) {
    double inverse = hypotinv_(x, y);
    x *= inverse;
    y *= inverse;
  }
  void CMath::normalize_(float &x, float &y) {
    float inverse = hypotinv_(x, y);
    x *= inverse;
    y *= inverse;
  }
  void CMath::normalize_(double &x, double &y, double &z) {
    double inverse = hypotinv_(x, y, z);
    x *= inverse;
    y *= inverse;
    z *= inverse;
  }
  void CMath::normalize_(float &x, float &y, float &z) {
    float inverse = hypotinv_(x, y, z);
    x *= inverse;
    y *= inverse;
    z *= inverse;
  }

  float CMath::frsqrte_(float x, DWORD magic) {
    DWORD bits = *reinterpret_cast<DWORD *>(&x);
    bits = magic - ((bits >> 1) & 0x3FFFFFFF);
    return *reinterpret_cast<float *>(&bits);
  }
  double CMath::frsqrte_(double x, DWORD magic) {
    reinterpret_cast<DWORD *>(&x)[1] = magic - ((reinterpret_cast<DWORD *>(&x)[1] >> 1) & 0x3FFFFFFF);
    return x;
  }
  float CMath::frsqrte_(float *x, DWORD magic) {
    *reinterpret_cast<DWORD *>(x) = magic - ((*reinterpret_cast<DWORD *>(x) >> 1) & 0x3FFFFFFF);
    return *x;
  }
  double CMath::frsqrte_(double *x, DWORD magic) {
    reinterpret_cast<DWORD *>(x)[1] = magic - ((reinterpret_cast<DWORD *>(x)[1] >> 1) & 0x3FFFFFFF);
    return *x;
  }
  float CMath::fres_(float x, DWORD magic) {
    DWORD bits = magic - *reinterpret_cast<DWORD *>(&x);
    return *reinterpret_cast<float *>(&bits);
  }
  double CMath::fres_(double x, DWORD magic) {
    reinterpret_cast<DWORD *>(&x)[1] = magic - reinterpret_cast<DWORD *>(&x)[1];
    return x;
  }
  float CMath::fres_(float *x, DWORD magic) {
    *reinterpret_cast<DWORD *>(x) = magic - *reinterpret_cast<DWORD *>(x);
    return *x;
  }
  double CMath::fres_(double *x, DWORD magic) {
    reinterpret_cast<DWORD *>(x)[1] = magic - reinterpret_cast<DWORD *>(x)[1];
    return *x;
  }

  void CMath::split_(double xlr, double &xf, long &xi) {
    xi = static_cast<long>(xlr);
    xi = xlr < 0.0 ? xi - 1 : xi;
    xf = xlr - xi;
  }
  void CMath::split_(float xr, float &xf, long &xi) {
    xi = fint_(xr);
    xi = xr < 0.0f ? xi - 1 : xi;
    xf = xr - xi;
  }
  void CMath::splitr_(double xlr, double &xf, double &xi) {
    xi = static_cast<long>(xlr);
    xi = xlr < 0.0 ? xi - 1.0 : xi;
    xf = xlr - xi;
  }
  void CMath::splitr_(float xr, float &xf, float &xi) {
    xi = static_cast<float>(fint_(xr));
    xi = xr < 0.0f ? xi - 1.0f : xi;
    xf = xr - xi;
  }

  float CMath::step_(float x, float a) {
    return x < a ? 0.0f : 1.0f;
  }
  float CMath::pulse_(float x, float a, float b) {
    return step_(x, a) - step_(x, b);
  }
  float CMath::bstep_(float x, float a, float b) {
    return clamp_((x - a) / (b - a), 0.0f, 1.0f);
  }
  float CMath::smoothstep_(float x, float a, float b) {
    if (x < a)
      return 0.0f;
    if (x >= b)
      return 1.0f;
    float t = (x - a) / (b - a);
    return t * (3.0f - 2.0f * t) * t;
  }
  double CMath::gammai_(float x, float g) {
    return pow(x, g);
  }
  double CMath::gamma_(float x, float g) {
    return gammai_(x, 1.0f / g);
  }
  double CMath::bias_(float x, float g) {
    return pow(x, -log2_(static_cast<double>(g)));
  }
  double CMath::gain_(float x, float g) {
    return x < 0.5f ? bias_(x + x, 1.0f - g) * 0.5 : 1.0 - bias_(2.0f - (x + x), 1.0f - g) * 0.5;
  }
  double CMath::sinc_(double x, double a) {
    double v = x * a * 3.141592653589793;
    return sin(v) / v;
  }
  double CMath::sinc_(double x) {
    double v = x * 3.141592653589793;
    return sin(v) / v;
  }
  float CMath::sinc_(float x, float a) {
    float v = x * a * 3.1415927f;
    return static_cast<float>(sin(v) / v);
  }
  float CMath::sinc_(float x) {
    float v = x * 3.1415927f;
    return static_cast<float>(sin(v) / v);
  }

}  // namespace NTempest
