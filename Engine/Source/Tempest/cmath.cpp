#include "../Base/Base.h"

#include "Tempest/c22matrix.h"
#include "Tempest/c2ivector.h"
#include "Tempest/c2vector.h"
#include "Tempest/c33matrix.h"
#include "Tempest/c34matrix.h"
#include "Tempest/c3ivector.h"
#include "Tempest/c3ray.h"
#include "Tempest/c3segment.h"
#include "Tempest/c3spline.h"
#include "Tempest/c3vector.h"
#include "Tempest/c44matrix.h"
#include "Tempest/c4ivector.h"
#include "Tempest/c4plane.h"
#include "Tempest/c4quaternion.h"
#include "Tempest/c4quaternioncompressed.h"
#include "Tempest/c4vector.h"
#include "Tempest/caabox.h"
#include "Tempest/caasphere.h"
#include "Tempest/ccone.h"
#include "Tempest/cdyntable.h"
#include "Tempest/cfacet.h"
#include "Tempest/cimvector.h"
#include "Tempest/cirange.h"
#include "Tempest/cirect.h"
#include "Tempest/cmath.h"
#include "Tempest/cmemblock.h"
#include "Tempest/cobbox.h"
#include "Tempest/cpriorityq.h"
#include "Tempest/crandom.h"
#include "Tempest/crange.h"
#include "Tempest/crect.h"
#include "Tempest/tempest_intersect.h"

static const double expmul_[64] = {
    1.0000000000000000, 1.0108892860517005, 1.0218971486541166, 1.0330248790212284, 1.0442737824274138, 1.0556451783605572, 1.0671404006768237,
    1.0787607977571199, 1.0905077326652577, 1.1023825833078409, 1.1143867425958924, 1.1265216186082418, 1.1387886347566916, 1.1511892299529827,
    1.1637248587775775, 1.1763969916502812, 1.1892071150027210, 1.2021567314527031, 1.2152473599804690, 1.2284805361068700, 1.2418578120734840,
    1.2553807570246911, 1.2690509571917332, 1.2828700160787783, 1.2968395546510096, 1.3109612115247644, 1.3252366431597413, 1.3396675240533029,
    1.3542555469368927, 1.3690024229745905, 1.3839098819638320, 1.3989796725383112, 1.4142135623730951, 1.4296133383919700, 1.4451808069770467,
    1.4609177941806470, 1.4768261459394993, 1.4929077282912648, 1.5091644275934228, 1.5255981507445384, 1.5422108254079407, 1.5590044002378369,
    1.5759808451078865, 1.5931421513422670, 1.6104903319492543, 1.6280274218573478, 1.6457554781539649, 1.6636765803267364, 1.6817928305074290,
    1.7001063537185235, 1.7186192981224779, 1.7373338352737062, 1.7562521603732995, 1.7753764925265212, 1.7947090750031072, 1.8142521755003989,
    1.8340080864093424, 1.8539791250833855, 1.8741676341103000, 1.8945759815869656, 1.9152065613971474, 1.9360617934922943, 1.9571441241754002,
    1.9784560263879509
};

static const double logmul_[64] = {
    1.0000000000000000,  0.98461538461538467, 0.96969696969696972, 0.95522388059701491, 0.94117647058823528, 0.92753623188405798, 0.91428571428571426,
    0.90140845070422537, 0.88888888888888884, 0.87671232876712324, 0.86486486486486491, 0.85333333333333339, 0.84210526315789469, 0.83116883116883122,
    0.82051282051282048, 0.81012658227848100, 0.80000000000000004, 0.79012345679012341, 0.78048780487804881, 0.77108433734939763, 0.76190476190476186,
    0.75294117647058822, 0.74418604651162790, 0.73563218390804597, 0.72727272727272729, 0.71910112359550560, 0.71111111111111114, 0.70329670329670335,
    0.69565217391304346, 0.68817204301075274, 0.68085106382978722, 0.67368421052631577, 0.66666666666666663, 0.65979381443298968, 0.65306122448979587,
    0.64646464646464652, 0.64000000000000001, 0.63366336633663367, 0.62745098039215685, 0.62135922330097082, 0.61538461538461542, 0.60952380952380958,
    0.60377358490566035, 0.59813084112149528, 0.59259259259259256, 0.58715596330275233, 0.58181818181818179, 0.57657657657657657, 0.57142857142857140,
    0.56637168141592920, 0.56140350877192979, 0.55652173913043479, 0.55172413793103448, 0.54700854700854706, 0.54237288135593220, 0.53781512605042014,
    0.53333333333333333, 0.52892561983471076, 0.52459016393442626, 0.52032520325203258, 0.51612903225806450, 0.51200000000000001, 0.50793650793650791,
    0.50393700787401574
};

static const double logadd_[64] = {0.0000000000000000,  0.022367813028454510, 0.044394119358453436, 0.066089190457772437, 0.087462841250339401,
                                   0.10852445677816905, 0.12928301694496647,  0.14974711950468206,  0.16992500144231237,  0.18982455888001723,
                                   0.20945336562894978, 0.22881869049588088,  0.24792751344358549,  0.26678654069490138,  0.28540221886224837,
                                   0.30378074817710293, 0.32192809488736235,  0.33985000288462475,  0.35755200461808367,  0.37503943134692475,
                                   0.39231742277876031, 0.40939093613770178,  0.42626475470209796,  0.44294349584872827,  0.45943161863729726,
                                   0.47573343096639775, 0.49185309632967472,  0.50779464019869625,  0.52356195605701283,  0.53915881110803143,
                                   0.55458885167763738, 0.56985560833094784,  0.58496250072115619,  0.59991284218712770,  0.61470984411520824,
                                   0.62935662007960957, 0.64385618977472470,  0.65821148275179475,  0.67242534197149562,  0.68650052718321841,
                                   0.70043971814109218, 0.71424551766612265,  0.72792045456319920,  0.74146698640114694,  0.75488750216346856,
                                   0.76818432477692633, 0.78135971352465960,  0.79441586635010597,  0.80735492205760406,  0.82017896241518773,
                                   0.83289001416474162, 0.84549005094437524,  0.85798099512757209,  0.87036471958340456,  0.88264304936184124,
                                   0.89481776330794349, 0.90689059560851848,  0.91886323727459451,  0.93073733756288624,  0.94251450533923986,
                                   0.95419631038687525, 0.96578428466208699,  0.97727992349991644,  0.98868468677216581};

namespace NTempest {

  DWORD CMath::sqrt_(DWORD a) {
    DWORD estimate;
    if (a <= 0xFF) {
      estimate = a / 12 + 1;
    } else if (a <= 0xFFFF) {
      estimate = a / 200 + 21;
    } else {
      estimate = a / 26743 + 444;
    }

    LONG difference;
    do {
      difference = (LONG)(estimate - a / estimate) / 2;
      estimate = (LONG)(a / estimate + estimate) / 2;
    } while (difference);
    return estimate;
  }

  float CMath::atanoid_(float x, const float _pi_2) {
    BOOL negative = 0;
    BOOL reciprocal = 0;

    if (x < -x) {
      negative = 1;
      x = -x;
    }

    if (x > 1.0f) {
      x = 1.0f / x;
      reciprocal = 1;
    }

    float x2 = x * x;
    float result = x + ((0.25906625f - x * 0.04955592f) * x2 + (0.016148888f - x * 0.44026104f)) * x2;
    if (reciprocal) {
      result = _pi_2 - result;
    }
    if (negative) {
      result = -result;
    }
    return result;
  }

  double CMath::logoid_(double x, const double _a, const double _b, const double _c, const double _d, const double _ln2) {
    if (x <= 1.0e-307) {
      return 0.0;
    }
#if !BUILD_PORTABLE
    DWORD *words = (DWORD *)&x;
    DWORD  exponent = words[1];
    words[1] = exponent & 0xFFFFF | 0x3FF00000;
    exponent = (exponent >> 20 & 0x7FF) - 1023;
    return ((LONG)exponent + ((_a * x + _b) * (x * x) + (x * _c + _d))) * _ln2;
#else
    UINT exponent;
    memcpy(&exponent, (BYTE *)&x + 4, sizeof(exponent));
    UINT high = exponent & 0xFFFFF | 0x3FF00000;
    memcpy((BYTE *)&x + 4, &high, sizeof(high));
    exponent = (exponent >> 20 & 0x7FF) - 1023;
    return ((int)exponent + ((_a * x + _b) * (x * x) + (x * _c + _d))) * _ln2;
#endif
  }

  double CMath::logoid2_(double x, const double _a, const double _b, const double _c, const double _d) {
    if (x <= 1.0e-307) {
      return 0.0;
    }
#if !BUILD_PORTABLE
    DWORD *words = (DWORD *)&x;
    DWORD  exponent = words[1];
    words[1] = exponent & 0xFFFFF | 0x3FF00000;
    exponent = (exponent >> 20 & 0x7FF) - 1023;
    return (LONG)exponent + ((_a * x + _b) * (x * x) + (x * _c + _d));
#else
    UINT exponent;
    memcpy(&exponent, (BYTE *)&x + 4, sizeof(exponent));
    UINT high = exponent & 0xFFFFF | 0x3FF00000;
    memcpy((BYTE *)&x + 4, &high, sizeof(high));
    exponent = (exponent >> 20 & 0x7FF) - 1023;
    return (int)exponent + ((_a * x + _b) * (x * x) + (x * _c + _d));
#endif
  }

  double CMath::logoid10_(double x, const double _a, const double _b, const double _c, const double _d, const double _ln10) {
    if (x <= 1.0e-307) {
      return 0.0;
    }
#if !BUILD_PORTABLE
    DWORD *words = (DWORD *)&x;
    DWORD  exponent = words[1];
    words[1] = exponent & 0xFFFFF | 0x3FF00000;
    exponent = (exponent >> 20 & 0x7FF) - 1023;
    return ((LONG)exponent + ((_a * x + _b) * (x * x) + (x * _c + _d))) * _ln10;
#else
    UINT exponent;
    memcpy(&exponent, (BYTE *)&x + 4, sizeof(exponent));
    UINT high = exponent & 0xFFFFF | 0x3FF00000;
    memcpy((BYTE *)&x + 4, &high, sizeof(high));
    exponent = (exponent >> 20 & 0x7FF) - 1023;
    return ((int)exponent + ((_a * x + _b) * (x * x) + (x * _c + _d))) * _ln10;
#endif
  }

  double CMath::log2_(double y) {
#if !BUILD_PORTABLE
    double q = y;
    DWORD  high = ((DWORD *)&q)[1];
    ((DWORD *)&q)[1] = high & 0xFFFFF | 0x3FF00000;
    DWORD  index = high >> 11 & 0x1F8;
    double v = q * *(const double *)((const BYTE *)logmul_ + index) - 1.0;
    double v2 = v * v;
    if (!(y > 1.0e-307)) {
    underflow:
      return -HUGE_VAL;
    }
    return v * (((0.2883070248990067 - v * 0.22949900023246153) * v2 + (0.48089833404997356 - v * 0.36067132973951144)) * v2 +
                (1.442695040888937 - v * 0.7213475204127876)) +
           ((LONG)((high >> 20 & 0x7FF) - 1023) + *(const double *)((const BYTE *)logadd_ + index));
#else
    double q = y;
    UINT   high;
    memcpy(&high, (BYTE *)&q + 4, sizeof(high));
    UINT   mantissa = high & 0xFFFFF | 0x3FF00000;
    memcpy((BYTE *)&q + 4, &mantissa, sizeof(mantissa));
    UINT   index = high >> 11 & 0x1F8;
    double v = q * *(const double *)((const BYTE *)logmul_ + index) - 1.0;
    double v2 = v * v;
    if (!(y > 1.0e-307)) {
      return -HUGE_VAL;
    }
    return v * (((0.2883070248990067 - v * 0.22949900023246153) * v2 + (0.48089833404997356 - v * 0.36067132973951144)) * v2 +
                (1.442695040888937 - v * 0.7213475204127876)) +
           ((int)((high >> 20 & 0x7FF) - 1023) + *(const double *)((const BYTE *)logadd_ + index));
#endif
  }

  double CMath::exp2_(double x) {
#if !BUILD_PORTABLE
    LONG   exponent = (LONG)(x + 1023.0) - 1;
    double base;
    ((DWORD *)&base)[0] = 0;
    double q = x + 1023.0 - (DWORD)exponent;
    DWORD  high = ((DWORD *)&q)[1];
    DWORD  index = high & 0xFC000;
    ((DWORD *)&q)[1] = high ^ index;
    ((DWORD *)&base)[1] = exponent << 20;
    double q2 = q * q;
    base *= *(const double *)((const BYTE *)expmul_ + (index >> 11));
    if ((DWORD)exponent > 0x7FF) {
    illegal:
      return exponent > 0 ? (float)HUGE_VAL : 0.0f;
    }
#else
    int    exponent = (int)(x + 1023.0) - 1;
    double base;
    UINT   word = 0;
    memcpy(&base, &word, sizeof(word));
    double q = x + 1023.0 - (UINT)exponent;
    UINT   high;
    memcpy(&high, (BYTE *)&q + 4, sizeof(high));
    UINT   index = high & 0xFC000;
    high ^= index;
    memcpy((BYTE *)&q + 4, &high, sizeof(high));
    word = (UINT)exponent << 20;
    memcpy((BYTE *)&base + 4, &word, sizeof(word));
    double q2 = q * q;
    base *= *(const double *)((const BYTE *)expmul_ + (index >> 11));
    if ((UINT)exponent > 0x7FF) {
      return exponent > 0 ? (float)HUGE_VAL : 0.0f;
    }
#endif
    return (((q * 0.0026811946517564956 + 0.005830032491308937) * q2 + (q * 0.06087614283854009 + 0.23603244393034895)) * q2 +
            (q * 0.6948749415195616 + 0.9997052445684839)) *
           base;
  }

  bool CMath::xsectunitsphere_(double x, double y, double z, double dx, double dy, double dz, const double _r2) {
    double distance2 = x * x + y * y + z * z;
    x *= dx;
    double direction = -(y * dy + z * dz);
    if (distance2 < _r2) {
    xsect:
      return true;
    }
    if (x > direction) {
    noxsect:
      return false;
    }
    direction -= x;
    return distance2 - direction * direction < _r2;
  }

  bool CMath::solvequad_(double a, double b, double c, double &r1, double &r2) {
    double q = a * c * 4.0;
    double discriminant = b * b;
    if (discriminant > q) {
      double root = sqrt_(discriminant - q);
      q = -0.5 * (b > 0.0 ? b + root : b - root);
      double div = 1.0 / (a * q);
      double first = q * q * div;
      double rb = a * c * div;
      if (first < rb) {
        r1 = first;
        r2 = rb;
      } else {
        r2 = first;
        r1 = rb;
      }
      return true;
    }
    return false;
  }

  bool CMath::solvequad_(float a, float b, float c, float &r1, float &r2) {
    float q = a * c * 4.0f;
    float discriminant = b * b;
    if (discriminant > q) {
      float root = sqrt_(discriminant - q);
      q = -0.5f * (b > 0.0f ? b + root : b - root);
      float div = 1.0f / (a * q);
      float first = q * q * div;
      float rb = a * c * div;
      if (first < rb) {
        r1 = first;
        r2 = rb;
      } else {
        r2 = first;
        r1 = rb;
      }
      return true;
    }
    return false;
  }

  void CMath::invertarray_(double *a, DWORD n) {
#if !BUILD_PORTABLE
    DWORD count = div3_(n);
    DWORD i;
    n -= count * 3;
    double fp1, fp2, fp3, fp4, fp5, fp6, fp7, fp8, fp9;
    if (count < 1)
      goto p3;
    fp1 = a[0];
    ((DWORD *)a)[1] = 0x7FDE61EE - ((DWORD *)a)[1];
    fp2 = a[0];
    fp4 = a[1];
    ((DWORD *)a)[3] = 0x7FDE61EE - ((DWORD *)a)[3];
    fp2 = fp2 * (2.0 - fp1 * fp2);
    fp5 = a[1];
    fp7 = a[2];
    ((DWORD *)a)[5] = 0x7FDE61EE - ((DWORD *)a)[5];
    fp2 = fp2 * (2.0 - fp1 * fp2);
    fp5 = fp5 * (2.0 - fp4 * fp5);
    fp8 = a[2];
    fp2 = fp2 * (2.0 - fp1 * fp2);
    fp5 = fp5 * (2.0 - fp4 * fp5);
    fp8 = fp8 * (2.0 - fp7 * fp8);
    for (i = 1; i < count; ++i) {
      fp3 = 2.0 - fp1 * fp2;
      fp5 = fp5 * (2.0 - fp4 * fp5);
      fp8 = fp8 * (2.0 - fp7 * fp8);
      fp1 = a[3];
      ((DWORD *)a)[7] = 0x7FDE61EE - ((DWORD *)a)[7];
      fp6 = 2.0 - fp4 * fp5;
      fp4 = a[4];
      ((DWORD *)a)[9] = 0x7FDE61EE - ((DWORD *)a)[9];
      a[0] = fp2 * fp3;
      fp8 = fp8 * (2.0 - fp7 * fp8);
      fp2 = a[3];
      fp9 = 2.0 - fp7 * fp8;
      fp7 = a[5];
      a[1] = fp5 * fp6;
      fp2 = fp2 * (2.0 - fp1 * fp2);
      fp5 = a[4];
      a[2] = fp8 * fp9;
      fp2 = fp2 * (2.0 - fp1 * fp2);
      fp5 = fp5 * (2.0 - fp4 * fp5);
      ((DWORD *)a)[11] = 0x7FDE61EE - ((DWORD *)a)[11];
      fp8 = a[5];
      a += 3;
      fp2 = fp2 * (2.0 - fp1 * fp2);
      fp5 = fp5 * (2.0 - fp4 * fp5);
      fp8 = fp8 * (2.0 - fp7 * fp8);
    }
    fp5 = fp5 * (2.0 - fp4 * fp5);
    fp8 = fp8 * (2.0 - fp7 * fp8);
    a[0] = fp2 * (2.0 - fp1 * fp2);
    fp8 = fp8 * (2.0 - fp7 * fp8);
    a[1] = fp5 * (2.0 - fp4 * fp5);
    a[2] = fp8 * (2.0 - fp7 * fp8);
    a += 3;
  p3:
    if (n >= 1) {
      a[0] = 1.0 / a[0];
      if (n != 1)
        a[1] = 1.0 / a[1];
    }
#else
    DWORD count = div3_(n);
    DWORD i;
    n -= count * 3;
    for (i = 0; i < count * 3; ++i) {
      double x = *a;
      double y = x;
      UINT   high;
      memcpy(&high, (BYTE *)&y + 4, sizeof(high));
      high = 0x7FDE61EE - high;
      memcpy((BYTE *)&y + 4, &high, sizeof(high));
      y = y * (2.0 - x * y);
      y = y * (2.0 - x * y);
      y = y * (2.0 - x * y);
      y = y * (2.0 - x * y);
      *a++ = y;
    }
    if (n >= 1) {
      a[0] = 1.0 / a[0];
      if (n != 1)
        a[1] = 1.0 / a[1];
    }
#endif
  }

  void CMath::sqrtarray_(double *a, DWORD n) {
#if !BUILD_PORTABLE
    DWORD count = div3_(n);
    DWORD i;
    n -= count * 3;
    double fp0, fp1, fp2, fp3, fp4, fp5, fp6, fp7, fp8, fp9, fpA, fpB, fpC;
    if (count < 1)
      goto p3;
    fpA = a[0];
    fp1 = fpA * 0.5;
    ((DWORD *)a)[1] = 0x5FE732F7 - ((((DWORD *)a)[1] >> 1) & 0x3FFFFFFF);
    fp2 = a[0];
    fpB = a[1];
    ((DWORD *)a)[3] = 0x5FE732F7 - ((((DWORD *)a)[3] >> 1) & 0x3FFFFFFF);
    fp5 = a[1];
    fpC = a[2];
    fp4 = fpB * 0.5;
    ((DWORD *)a)[5] = 0x5FE732F7 - ((((DWORD *)a)[5] >> 1) & 0x3FFFFFFF);
    fp2 = fp2 * (1.5 - fp2 * fp2 * fp1);
    fp8 = a[2];
    fp7 = fpC * 0.5;
    fp5 = fp5 * (1.5 - fp5 * fp5 * fp4);
    fp2 = fp2 * (1.5 - fp2 * fp2 * fp1);
    fp8 = fp8 * (1.5 - fp8 * fp8 * fp7);
    fp3 = 1.5 - fp2 * fp2 * fp1;
    fp5 = fp5 * (1.5 - fp5 * fp5 * fp4);
    fp9 = fp8 * fp8;
    for (i = 1; i < count; ++i) {
      fp2 = fp2 * fp3;
      fp8 = fp8 * (1.5 - fp9 * fp7);
      fp5 = fp5 * (1.5 - fp5 * fp5 * fp4);
      fp3 = fp2 * fpA * (1.5 - fp2 * fp2 * fp1);
      fp8 = fp8 * (1.5 - fp8 * fp8 * fp7);
      fpA = a[3];
      ((DWORD *)a)[7] = 0x5FE732F7 - ((((DWORD *)a)[7] >> 1) & 0x3FFFFFFF);
      fp2 = a[3];
      fp6 = fp5 * fpB * (1.5 - fp5 * fp5 * fp4);
      fpB = a[4];
      a[0] = fp3;
      fp1 = fpA * 0.5;
      ((DWORD *)a)[9] = 0x5FE732F7 - ((((DWORD *)a)[9] >> 1) & 0x3FFFFFFF);
      fp5 = a[4];
      fp0 = fp8 * fpC * (1.5 - fp8 * fp8 * fp7);
      fpC = a[5];
      a[1] = fp6;
      fp4 = fpB * 0.5;
      ((DWORD *)a)[11] = 0x5FE732F7 - ((((DWORD *)a)[11] >> 1) & 0x3FFFFFFF);
      fp8 = a[5];
      a += 3;
      fp2 = fp2 * (1.5 - fp2 * fp2 * fp1);
      fp7 = fpC * 0.5;
      a[-1] = fp0;
      fp5 = fp5 * (1.5 - fp5 * fp5 * fp4);
      fp2 = fp2 * (1.5 - fp2 * fp2 * fp1);
      fp8 = fp8 * (1.5 - fp8 * fp8 * fp7);
      fp3 = 1.5 - fp2 * fp2 * fp1;
      fp5 = fp5 * (1.5 - fp5 * fp5 * fp4);
      fp9 = fp8 * fp8;
    }
    fp2 = fp2 * fp3;
    fp8 = fp8 * (1.5 - fp9 * fp7);
    fp5 = fp5 * (1.5 - fp5 * fp5 * fp4);
    fp8 = fp8 * (1.5 - fp8 * fp8 * fp7);
    a[0] = fp2 * (1.5 - fp2 * fp2 * fp1) * fpA;
    a[1] = fp5 * (1.5 - fp5 * fp5 * fp4) * fpB;
    a[2] = fp8 * (1.5 - fp8 * fp8 * fp7) * fpC;
    a += 3;
  p3:
    if (n >= 1) {
      a[0] = sqrt_(a[0]);
      if (n != 1)
        a[1] = sqrt_(a[1]);
    }
#else
    DWORD count = div3_(n);
    DWORD i;
    n -= count * 3;
    for (i = 0; i < count * 3; ++i) {
      double x = *a;
      double half = x * 0.5;
      double y = x;
      UINT   high;
      memcpy(&high, (BYTE *)&y + 4, sizeof(high));
      high = 0x5FE732F7 - ((high >> 1) & 0x3FFFFFFF);
      memcpy((BYTE *)&y + 4, &high, sizeof(high));
      y = y * (1.5 - y * y * half);
      y = y * (1.5 - y * y * half);
      y = y * (1.5 - y * y * half);
      *a++ = y * (1.5 - y * y * half) * x;
    }
    if (n >= 1) {
      a[0] = sqrt_(a[0]);
      if (n != 1)
        a[1] = sqrt_(a[1]);
    }
#endif
  }

  void CMath::sqrtinvarray_(double *a, DWORD n) {
#if !BUILD_PORTABLE
    DWORD count = div3_(n);
    DWORD i;
    n -= count * 3;
    double fp0, fp1, fp2, fp3, fp4, fp5, fp6, fp7, fp8, fp9;
    if (count < 1)
      goto p3;
    fp1 = a[0];
    ((DWORD *)a)[1] = 0x5FE732F7 - ((((DWORD *)a)[1] >> 1) & 0x3FFFFFFF);
    fp2 = a[0];
    fp4 = a[1];
    ((DWORD *)a)[3] = 0x5FE732F7 - ((((DWORD *)a)[3] >> 1) & 0x3FFFFFFF);
    fp1 = fp1 * 0.5;
    fp5 = a[1];
    fp4 = fp4 * 0.5;
    fp7 = a[2];
    ((DWORD *)a)[5] = 0x5FE732F7 - ((((DWORD *)a)[5] >> 1) & 0x3FFFFFFF);
    fp2 = fp2 * (1.5 - fp2 * fp2 * fp1);
    fp8 = a[2];
    fp7 = fp7 * 0.5;
    fp5 = fp5 * (1.5 - fp5 * fp5 * fp4);
    fp2 = fp2 * (1.5 - fp2 * fp2 * fp1);
    fp8 = fp8 * (1.5 - fp8 * fp8 * fp7);
    fp3 = 1.5 - fp2 * fp2 * fp1;
    fp5 = fp5 * (1.5 - fp5 * fp5 * fp4);
    fp9 = fp8 * fp8;
    for (i = 1; i < count; ++i) {
      fp2 = fp2 * fp3;
      fp8 = fp8 * (1.5 - fp9 * fp7);
      fp3 = 1.5 - fp2 * fp2 * fp1;
      fp1 = a[3];
      ((DWORD *)a)[7] = 0x5FE732F7 - ((((DWORD *)a)[7] >> 1) & 0x3FFFFFFF);
      fp5 = fp5 * (1.5 - fp5 * fp5 * fp4);
      fp0 = fp2 * fp3;
      fp6 = 1.5 - fp5 * fp5 * fp4;
      fp4 = a[4];
      ((DWORD *)a)[9] = 0x5FE732F7 - ((((DWORD *)a)[9] >> 1) & 0x3FFFFFFF);
      fp8 = fp8 * (1.5 - fp8 * fp8 * fp7);
      fp2 = a[3];
      a[0] = fp0;
      fp6 = fp5 * fp6;
      fp9 = 1.5 - fp8 * fp8 * fp7;
      fp1 = fp1 * 0.5;
      fp5 = a[4];
      fp7 = a[5];
      fp0 = fp8 * fp9;
      a[1] = fp6;
      fp4 = fp4 * 0.5;
      ((DWORD *)a)[11] = 0x5FE732F7 - ((((DWORD *)a)[11] >> 1) & 0x3FFFFFFF);
      fp8 = a[5];
      a += 3;
      fp2 = fp2 * (1.5 - fp2 * fp2 * fp1);
      a[-1] = fp0;
      fp7 = fp7 * 0.5;
      fp5 = fp5 * (1.5 - fp5 * fp5 * fp4);
      fp2 = fp2 * (1.5 - fp2 * fp2 * fp1);
      fp8 = fp8 * (1.5 - fp8 * fp8 * fp7);
      fp3 = 1.5 - fp2 * fp2 * fp1;
      fp5 = fp5 * (1.5 - fp5 * fp5 * fp4);
      fp9 = fp8 * fp8;
    }
    fp2 = fp2 * fp3;
    fp8 = fp8 * (1.5 - fp9 * fp7);
    fp5 = fp5 * (1.5 - fp5 * fp5 * fp4);
    a[0] = fp2 * (1.5 - fp2 * fp2 * fp1);
    fp8 = fp8 * (1.5 - fp8 * fp8 * fp7);
    a[1] = fp5 * (1.5 - fp5 * fp5 * fp4);
    a[2] = fp8 * (1.5 - fp8 * fp8 * fp7);
    a += 3;
  p3:
    if (n >= 1) {
      a[0] = sqrtinv_(a[0]);
      if (n != 1)
        a[1] = sqrtinv_(a[1]);
    }
#else
    DWORD count = div3_(n);
    DWORD i;
    n -= count * 3;
    for (i = 0; i < count * 3; ++i) {
      double half = *a * 0.5;
      double y = *a;
      UINT   high;
      memcpy(&high, (BYTE *)&y + 4, sizeof(high));
      high = 0x5FE732F7 - ((high >> 1) & 0x3FFFFFFF);
      memcpy((BYTE *)&y + 4, &high, sizeof(high));
      y = y * (1.5 - y * y * half);
      y = y * (1.5 - y * y * half);
      y = y * (1.5 - y * y * half);
      y = y * (1.5 - y * y * half);
      *a++ = y;
    }
    if (n >= 1) {
      a[0] = sqrtinv_(a[0]);
      if (n != 1)
        a[1] = sqrtinv_(a[1]);
    }
#endif
  }

  double CMath::spline_(double x, double *k, DWORD n) {
    ASSERT(n >= 4);
    n -= 3;
    x = clamp_(x, 0.0, 1.0) * n;
    DWORD span = x;
    if (span > n)
      span = n;
    x -= span;
    double k0 = k[span];
    double k1 = k[span + 1];
    double k2 = k[span + 2];
    double k3 = k[span + 3];
    double c3 = k3 * 0.5 - k2 * 1.5 + k1 * 1.5 - k0 * 0.5;
    double c2 = k2 * 2.0 - k3 * 0.5 - k1 * 2.5 + k0;
    double c1 = k2 * 0.5 - k0 * 0.5;
    double c0 = k1;

    return ((c3 * x + c2) * x + c1) * x + c0;
  }

  float CMath::spline_(float x, float *k, DWORD n) {
    ASSERT(n >= 4);
    n -= 3;
    x = clamp_(x, 0.0f, 1.0f) * n;
    DWORD span = x;
    if (span > n)
      span = n;
    x -= span;
    float k0 = k[span];
    float k1 = k[span + 1];
    float k2 = k[span + 2];
    float k3 = k[span + 3];
    float c3 = k3 * 0.5f - k2 * 1.5f + k1 * 1.5f - k0 * 0.5f;
    float c2 = k2 * 2.0f - k3 * 0.5f - k1 * 2.5f + k0;
    float c1 = k2 * 0.5f - k0 * 0.5f;
    float c0 = k1;

    return ((c3 * x + c2) * x + c1) * x + c0;
  }

}  // namespace NTempest
