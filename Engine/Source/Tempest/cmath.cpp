#include "Tempest/cmath.h"

double expmul_[64] = {
  1.0000000000000000, 1.0108892860517005, 1.0218971486541166, 1.0330248790212284,
  1.0442737824274138, 1.0556451783605572, 1.0671404006768237, 1.0787607977571199,
  1.0905077326652577, 1.1023825833078409, 1.1143867425958924, 1.1265216186082418,
  1.1387886347566916, 1.1511892299529827, 1.1637248587775775, 1.1763969916502812,
  1.1892071150027210, 1.2021567314527031, 1.2152473599804690, 1.2284805361068700,
  1.2418578120734840, 1.2553807570246911, 1.2690509571917332, 1.2828700160787783,
  1.2968395546510096, 1.3109612115247644, 1.3252366431597413, 1.3396675240533029,
  1.3542555469368927, 1.3690024229745905, 1.3839098819638320, 1.3989796725383112,
  1.4142135623730951, 1.4296133383919700, 1.4451808069770467, 1.4609177941806470,
  1.4768261459394993, 1.4929077282912648, 1.5091644275934228, 1.5255981507445384,
  1.5422108254079407, 1.5590044002378369, 1.5759808451078865, 1.5931421513422670,
  1.6104903319492543, 1.6280274218573478, 1.6457554781539649, 1.6636765803267364,
  1.6817928305074290, 1.7001063537185235, 1.7186192981224779, 1.7373338352737062,
  1.7562521603732995, 1.7753764925265212, 1.7947090750031072, 1.8142521755003989,
  1.8340080864093424, 1.8539791250833855, 1.8741676341103000, 1.8945759815869656,
  1.9152065613971474, 1.9360617934922943, 1.9571441241754002, 1.9784560263879509
};

double logmul_[64] = {
  1.0000000000000000, 0.98461538461538467, 0.96969696969696972, 0.95522388059701491,
  0.94117647058823528, 0.92753623188405798, 0.91428571428571426, 0.90140845070422537,
  0.88888888888888884, 0.87671232876712324, 0.86486486486486491, 0.85333333333333339,
  0.84210526315789469, 0.83116883116883122, 0.82051282051282048, 0.81012658227848100,
  0.80000000000000004, 0.79012345679012341, 0.78048780487804881, 0.77108433734939763,
  0.76190476190476186, 0.75294117647058822, 0.74418604651162790, 0.73563218390804597,
  0.72727272727272729, 0.71910112359550560, 0.71111111111111114, 0.70329670329670335,
  0.69565217391304346, 0.68817204301075274, 0.68085106382978722, 0.67368421052631577,
  0.66666666666666663, 0.65979381443298968, 0.65306122448979587, 0.64646464646464652,
  0.64000000000000001, 0.63366336633663367, 0.62745098039215685, 0.62135922330097082,
  0.61538461538461542, 0.60952380952380958, 0.60377358490566035, 0.59813084112149528,
  0.59259259259259256, 0.58715596330275233, 0.58181818181818179, 0.57657657657657657,
  0.57142857142857140, 0.56637168141592920, 0.56140350877192979, 0.55652173913043479,
  0.55172413793103448, 0.54700854700854706, 0.54237288135593220, 0.53781512605042014,
  0.53333333333333333, 0.52892561983471076, 0.52459016393442626, 0.52032520325203258,
  0.51612903225806450, 0.51200000000000001, 0.50793650793650791, 0.50393700787401574
};

double logadd_[64] = {
  0.0000000000000000, 0.022367813028454510, 0.044394119358453436, 0.066089190457772437,
  0.087462841250339401, 0.10852445677816905, 0.12928301694496647, 0.14974711950468206,
  0.16992500144231237, 0.18982455888001723, 0.20945336562894978, 0.22881869049588088,
  0.24792751344358549, 0.26678654069490138, 0.28540221886224837, 0.30378074817710293,
  0.32192809488736235, 0.33985000288462475, 0.35755200461808367, 0.37503943134692475,
  0.39231742277876031, 0.40939093613770178, 0.42626475470209796, 0.44294349584872827,
  0.45943161863729726, 0.47573343096639775, 0.49185309632967472, 0.50779464019869625,
  0.52356195605701283, 0.53915881110803143, 0.55458885167763738, 0.56985560833094784,
  0.58496250072115619, 0.59991284218712770, 0.61470984411520824, 0.62935662007960957,
  0.64385618977472470, 0.65821148275179475, 0.67242534197149562, 0.68650052718321841,
  0.70043971814109218, 0.71424551766612265, 0.72792045456319920, 0.74146698640114694,
  0.75488750216346856, 0.76818432477692633, 0.78135971352465960, 0.79441586635010597,
  0.80735492205760406, 0.82017896241518773, 0.83289001416474162, 0.84549005094437524,
  0.85798099512757209, 0.87036471958340456, 0.88264304936184124, 0.89481776330794349,
  0.90689059560851848, 0.91886323727459451, 0.93073733756288624, 0.94251450533923986,
  0.95419631038687525, 0.96578428466208699, 0.97727992349991644, 0.98868468677216581
};

namespace NTempest {

  unsigned long __fastcall CMath::sqrt_(unsigned long a) {
    unsigned long estimate;
    if (a <= 0xFF) {
      estimate = a / 12 + 1;
    } else if (a <= 0xFFFF) {
      estimate = a / 200 + 21;
    } else {
      estimate = a / 26743 + 444;
    }

    long difference;
    do {
      difference = static_cast<long>(estimate - a / estimate) / 2;
      estimate = (a / estimate + estimate) / 2;
    } while (difference);
    return estimate;
  }

  float __fastcall CMath::atanoid_(float x, float piOverTwo) {
    bool negative = x < 0.0f;
    if (negative) {
      x = -x;
    }

    bool reciprocal = x > 1.0f;
    if (reciprocal) {
      x = 1.0f / x;
    }

    float x2 = x * x;
    float result = x + ((0.25906625f - x * 0.04955592f) * x2 + 0.016148888f - x * 0.44026104f) * x2;
    if (reciprocal) {
      result = piOverTwo - result;
    }
    return negative ? -result : result;
  }

  double __fastcall CMath::logoid_(double x, double a, double b, double c, double d, double ln2) {
    if (x <= 1.0e-307) {
      return -HUGE_VAL;
    }
    unsigned long *words = reinterpret_cast<unsigned long *>(&x);
    long exponent = static_cast<long>((words[1] >> 20 & 0x7FF) - 1023);
    words[1] = words[1] & 0xFFFFF | 0x3FF00000;
    return ((a * x + b) * x * x + x * c + d + exponent) * ln2;
  }

  double __fastcall CMath::logoid2_(double x, double a, double b, double c, double d) {
    if (x <= 1.0e-307) {
      return -HUGE_VAL;
    }
    unsigned long *words = reinterpret_cast<unsigned long *>(&x);
    long exponent = static_cast<long>((words[1] >> 20 & 0x7FF) - 1023);
    words[1] = words[1] & 0xFFFFF | 0x3FF00000;
    return (a * x + b) * x * x + x * c + d + exponent;
  }

  double __fastcall CMath::logoid10_(double x, double a, double b, double c, double d, double ln10) {
    if (x <= 1.0e-307) {
      return -HUGE_VAL;
    }
    unsigned long *words = reinterpret_cast<unsigned long *>(&x);
    long exponent = static_cast<long>((words[1] >> 20 & 0x7FF) - 1023);
    words[1] = words[1] & 0xFFFFF | 0x3FF00000;
    return ((a * x + b) * x * x + x * c + d + exponent) * ln10;
  }

  double __fastcall CMath::log2_(double y) {
    if (y <= 1.0e-307) {
      return -HUGE_VAL;
    }

    unsigned long high = reinterpret_cast<unsigned long *>(&y)[1];
    unsigned long index = high >> 14 & 0x3F;
    double q = y;
    reinterpret_cast<unsigned long *>(&q)[1] = high & 0xFFFFF | 0x3FF00000;
    double v = q * logmul_[index] - 1.0;
    double v2 = v * v;
    return v * (((0.2883070248990067 - v * 0.2294990002324615) * v2 +
                 0.4808983340499736 - v * 0.3606713297395114) * v2 +
                1.442695040888937 - v * 0.7213475204127876) +
           static_cast<long>((high >> 20 & 0x7FF) - 1023) + logadd_[index];
  }

  double __fastcall CMath::exp2_(double x) {
    long exponent = static_cast<long>(x + 1023.0) - 1;
    double q = x + 1023.0 - static_cast<double>(exponent);
    unsigned long high = reinterpret_cast<unsigned long *>(&q)[1];
    unsigned long index = high >> 14 & 0x3F;
    reinterpret_cast<unsigned long *>(&q)[1] = high ^ (high & 0xFC000);

    if (static_cast<unsigned long>(exponent) <= 0x7FF) {
      double base = 0.0;
      reinterpret_cast<unsigned long *>(&base)[1] = exponent << 20;
      return base * expmul_[index] *
             (((q * 0.002681194651756496 + 0.005830032491308937) * q * q +
               q * 0.06087614283854009 + 0.2360324439303489) * q * q +
              q * 0.6948749415195616 + 0.9997052445684839);
    }
    return exponent <= 0 ? 0.0 : HUGE_VAL;
  }

  bool __fastcall CMath::xsectunitsphere_(double x, double y, double z, double dx, double dy, double dz, double r2) {
    double distance2 = x * x + y * y + z * z;
    double direction = x * dx + y * dy + z * dz;
    if (distance2 < r2) {
      return true;
    }
    return direction <= 0.0 && distance2 - direction * direction < r2;
  }

  bool __fastcall CMath::solvequad_(double a, double b, double c, double &r1, double &r2) {
    double discriminant = b * b - 4.0 * a * c;
    if (discriminant <= 0.0) {
      return false;
    }
    double root = sqrt_(discriminant);
    double q = -0.5 * (b > 0.0 ? b + root : b - root);
    double inverse = 1.0 / (a * q);
    double first = q * q * inverse;
    double second = a * c * inverse;
    if (first < second) {
      r1 = first;
      r2 = second;
    } else {
      r1 = second;
      r2 = first;
    }
    return true;
  }

  bool __fastcall CMath::solvequad_(float a, float b, float c, float &r1, float &r2) {
    float discriminant = b * b - 4.0f * a * c;
    if (discriminant <= 0.0f) {
      return false;
    }
    float root = sqrt_(discriminant);
    float q = -0.5f * (b > 0.0f ? b + root : b - root);
    float inverse = 1.0f / (a * q);
    float first = q * q * inverse;
    float second = a * c * inverse;
    if (first < second) {
      r1 = first;
      r2 = second;
    } else {
      r1 = second;
      r2 = first;
    }
    return true;
  }

  void __fastcall CMath::invertarray_(double *a, unsigned long n) {
    for (unsigned long i = 0; i < n; ++i) {
      a[i] = 1.0 / a[i];
    }
  }

  void __fastcall CMath::sqrtarray_(double *a, unsigned long n) {
    for (unsigned long i = 0; i < n; ++i) {
      a[i] = sqrt_(a[i]);
    }
  }

  void __fastcall CMath::sqrtinvarray_(double *a, unsigned long n) {
    for (unsigned long i = 0; i < n; ++i) {
      a[i] = 1.0 / sqrt_(a[i]);
    }
  }

  double __fastcall CMath::spline_(double x, double *k, unsigned long n) {
    if (n < 4) {
      SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "n >= 4", 0, 1);
    }
    unsigned long segments = n - 3;
    x = clamp_(x, 0.0, 1.0) * segments;
    unsigned long segment = static_cast<unsigned long>(x);
    if (segment > segments) segment = segments;
    double t = x - segment;
    return t * (((k[segment + 3] * 0.5 - k[segment + 2] * 1.5 + k[segment + 1] * 1.5 - k[segment] * 0.5) * t +
                 k[segment + 2] * 2.0 - k[segment + 3] * 0.5 - k[segment + 1] * 2.5 + k[segment]) * t +
                k[segment + 2] * 0.5 - k[segment] * 0.5) + k[segment + 1];
  }

  float __fastcall CMath::spline_(float x, float *k, unsigned long n) {
    if (n < 4) {
      SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "n >= 4", 0, 1);
    }
    unsigned long segments = n - 3;
    x = clamp_(x, 0.0f, 1.0f) * segments;
    unsigned long segment = static_cast<unsigned long>(x);
    if (segment > segments) segment = segments;
    float t = x - segment;
    return t * (((k[segment + 3] * 0.5f - k[segment + 2] * 1.5f + k[segment + 1] * 1.5f - k[segment] * 0.5f) * t +
                 k[segment + 2] * 2.0f - k[segment + 3] * 0.5f - k[segment + 1] * 2.5f + k[segment]) * t +
                k[segment + 2] * 0.5f - k[segment] * 0.5f) + k[segment + 1];
  }

}
