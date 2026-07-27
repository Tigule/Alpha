#ifndef ENGINE_SOURCE_BASE_CUNREAL_H
#define ENGINE_SOURCE_BASE_CUNREAL_H

class CDataStore;

class unreal {
 protected:
  union {
    unsigned int bits;
    float fp;
  };

 public:
  unreal() {}
  unreal(const unreal &value) : bits(value.bits) {}
  ~unreal() {}

  static unreal fromBits(unsigned int value) {
    unreal result;
    result.bits = value;
    return result;
  }

  static unsigned int asBits(const unreal &value) {
    return value.bits;
  }

  static unreal fromFloat(float value) {
    unreal result;
    result.fp = value;
    return result;
  }

  static float asFloat(const unreal &value) {
    return value.fp;
  }

  static unreal fromInt(int value);
  static int asInt(const unreal &value);
  static unreal fromString(const char *value);
  static void asString(const unreal &value, char *buffer, int integerWidth, int fractionalPrecision);

  static unreal fromRatio(int numerator, int denominator);

  void multiplyBy2() {
    bits += (bits & 0x7F800000) ? 0x00800000 : 0;
  }

  void multiplyBy4() {
    bits += (bits & 0x7F800000) ? 0x01000000 : 0;
  }

  void multiplyBy8() {
    bits += (bits & 0x7F800000) ? 0x01800000 : 0;
  }

  void multiplyBy16() {
    bits += (bits & 0x7F800000) ? 0x02000000 : 0;
  }

  void multiplyBy32() {
    bits += (bits & 0x7F800000) ? 0x02800000 : 0;
  }

  void multiplyBy64() {
    bits += (bits & 0x7F800000) ? 0x03000000 : 0;
  }

  void multiplyBy128() {
    bits += (bits & 0x7F800000) ? 0x03800000 : 0;
  }

  void multiplyBy256() {
    bits += (bits & 0x7F800000) ? 0x04000000 : 0;
  }

  void multiplyBy512() {
    bits += (bits & 0x7F800000) ? 0x04800000 : 0;
  }

  void multiplyBy1024() {
    bits += (bits & 0x7F800000) ? 0x05000000 : 0;
  }

  void divideBy2() {
    bits = (bits - 0x00800000) & ~static_cast<unsigned int>(static_cast<int>(bits ^ (bits - 0x01000000)) >> 31);
  }

  void divideBy4() {
    bits = (bits - 0x01000000) & ~static_cast<unsigned int>(static_cast<int>(bits ^ (bits - 0x01800000)) >> 31);
  }

  void divideBy8() {
    bits = (bits - 0x01800000) & ~static_cast<unsigned int>(static_cast<int>(bits ^ (bits - 0x02000000)) >> 31);
  }

  void divideBy16() {
    bits = (bits - 0x02000000) & ~static_cast<unsigned int>(static_cast<int>(bits ^ (bits - 0x02800000)) >> 31);
  }

  void divideBy32() {
    bits = (bits - 0x02800000) & ~static_cast<unsigned int>(static_cast<int>(bits ^ (bits - 0x03000000)) >> 31);
  }

  void divideBy64() {
    bits = (bits - 0x03000000) & ~static_cast<unsigned int>(static_cast<int>(bits ^ (bits - 0x03800000)) >> 31);
  }

  void divideBy128() {
    bits = (bits - 0x03800000) & ~static_cast<unsigned int>(static_cast<int>(bits ^ (bits - 0x04000000)) >> 31);
  }

  void divideBy256() {
    bits = (bits - 0x04000000) & ~static_cast<unsigned int>(static_cast<int>(bits ^ (bits - 0x04800000)) >> 31);
  }

  void divideBy512() {
    bits = (bits - 0x04800000) & ~static_cast<unsigned int>(static_cast<int>(bits ^ (bits - 0x05000000)) >> 31);
  }

  void divideBy1024() {
    bits = (bits - 0x05000000) & ~static_cast<unsigned int>(static_cast<int>(bits ^ (bits - 0x05800000)) >> 31);
  }

  unreal &operator+=(const unreal &value);
  unreal &operator-=(const unreal &value);
  unreal &operator*=(const unreal &value);
  unreal &operator/=(const unreal &value);

  bool operator<(const unreal &value) const {
    return fp < value.fp;
  }

  bool operator>(const unreal &value) const {
    return fp > value.fp;
  }

  bool operator<=(const unreal &value) const {
    return fp <= value.fp;
  }

  bool operator>=(const unreal &value) const {
    return fp >= value.fp;
  }

  bool operator==(const unreal &value) const {
    return fp == value.fp;
  }

  bool operator!=(const unreal &value) const {
    return fp != value.fp;
  }

  friend unreal operator*(const unreal &a, const unreal &b);
  friend unreal operator/(const unreal &a, const unreal &b);
  friend unreal operator-(const unreal &a, const unreal &b);
  friend unreal operator+(const unreal &a, const unreal &b);
  friend unreal reciprocal(const unreal &value);
  friend unreal floor(const unreal &value);
  friend unreal ceil(const unreal &value);
  friend unreal trunc(const unreal &value);
  friend unreal fract(const unreal &value);
  friend unreal round(const unreal &value);
  friend unreal mod(const unreal &a, const unreal &b);
  friend unreal ln(const unreal &value);
  friend unreal e(const unreal &value);
  friend unreal pow(const unreal &value, unsigned int exponent);
  friend unreal pow(const unreal &value, const unreal &exponent);
  friend unreal sqrt(const unreal &value);
  friend unreal sqrtinv(const unreal &value);
  friend unreal sin(const unreal &value);
  friend unreal cos(const unreal &value);
  friend void sincos(const unreal &value, unreal *sine, unreal *cosine);
  friend unreal tan(const unreal &value);
  friend unreal acos(const unreal &value);
  friend unreal asin(const unreal &value);
  friend unreal atan(const unreal &value);
  friend unreal atan2(const unreal &y, const unreal &x);
  friend CDataStore &operator<<(CDataStore &store, const unreal &value);
  friend CDataStore &operator>>(CDataStore &store, unreal &value);
};

unreal operator*(const unreal &a, const unreal &b);
unreal operator/(const unreal &a, const unreal &b);
unreal operator-(const unreal &a, const unreal &b);
unreal operator+(const unreal &a, const unreal &b);

inline unreal &unreal::operator+=(const unreal &value) {
  *this = *this + value;
  return *this;
}

inline unreal &unreal::operator-=(const unreal &value) {
  *this = *this - value;
  return *this;
}

inline unreal &unreal::operator*=(const unreal &value) {
  *this = *this * value;
  return *this;
}

inline unreal &unreal::operator/=(const unreal &value) {
  *this = *this / value;
  return *this;
}

inline unreal unreal::fromRatio(int numerator, int denominator) {
  return fromInt(numerator) / fromInt(denominator);
}

unreal reciprocal(const unreal &value);
unreal floor(const unreal &value);
unreal ceil(const unreal &value);
unreal trunc(const unreal &value);
unreal fract(const unreal &value);
unreal round(const unreal &value);
unreal mod(const unreal &a, const unreal &b);
unreal ln(const unreal &value);
unreal e(const unreal &value);
unreal pow(const unreal &value, unsigned int exponent);
unreal pow(const unreal &value, const unreal &exponent);
unreal sqrt(const unreal &value);
unreal sqrtinv(const unreal &value);
unreal sin(const unreal &value);
unreal cos(const unreal &value);
void sincos(const unreal &value, unreal *sine, unreal *cosine);
unreal tan(const unreal &value);
unreal acos(const unreal &value);
unreal asin(const unreal &value);
unreal atan(const unreal &value);
unreal atan2(const unreal &y, const unreal &x);

CDataStore &operator<<(CDataStore &store, const unreal &value);
CDataStore &operator>>(CDataStore &store, unreal &value);

void UnrealInitialize();
void UnrealDestroy();

#endif
