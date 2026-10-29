#pragma once

class BigNum {
  BigData *m_data;

 public:
  BigNum() { SBigNew(&m_data); }
  BigNum(const BigNum &copy) { SBigNew(&m_data); SBigCopy(m_data, *copy.m_data); }
  BigNum(UINT value);
  BigNum(LPCSTR value);
  ~BigNum() { SBigDel(m_data); }
  BigNum &operator=(const BigNum &copy) { SBigCopy(m_data, *copy.m_data); return *this; }
  BigNum &operator=(UINT value);

  BigNum &operator=(LPCSTR value);

  BigNum &Add(const BigNum &a, const BigNum &b);
  BigNum &Sub(const BigNum &a, const BigNum &b);
  BigNum &Mul(const BigNum &a, const BigNum &b);
  BigNum &Div(const BigNum &a, const BigNum &b);
  BigNum &Mod(const BigNum &a, const BigNum &b);
  BigNum &And(const BigNum &a, const BigNum &b);
  BigNum &Or(const BigNum &a, const BigNum &b);
  BigNum &Xor(const BigNum &a, const BigNum &b);
  BigNum &Not(const BigNum &a);
  BigNum &Dec(const BigNum &a);
  BigNum &Inc(const BigNum &a);
  BigNum &Shl(const BigNum &a, UINT bits);
  BigNum &Shr(const BigNum &a, UINT bits);

  BigNum  operator+(const BigNum &value);
  BigNum  operator-(const BigNum &value);
  BigNum  operator*(const BigNum &value);
  BigNum  operator/(const BigNum &value);
  BigNum  operator%(const BigNum &value);
  BigNum  operator&(const BigNum &value);
  BigNum  operator|(const BigNum &value);
  BigNum  operator^(const BigNum &value);
  BigNum  operator<<(UINT bits);
  BigNum  operator>>(UINT bits);
  BigNum  operator~();
  BigNum  operator--(int);
  BigNum &operator--();
  BigNum  operator++(int);
  BigNum &operator++();
  BigNum &operator+=(const BigNum &value);
  BigNum &operator-=(const BigNum &value);
  BigNum &operator*=(const BigNum &value);
  BigNum &operator/=(const BigNum &value);
  BigNum &operator%=(const BigNum &value);
  BigNum &operator&=(const BigNum &value);
  BigNum &operator|=(const BigNum &value);
  BigNum &operator^=(const BigNum &value);
  BigNum &operator<<=(UINT bits);
  BigNum &operator>>=(UINT bits);

  int     Compare(const BigNum &value);
  int     operator==(const BigNum &value);
  int     operator!=(const BigNum &value);
  int     operator<=(const BigNum &value);
  int     operator>=(const BigNum &value);
  int     operator<(const BigNum &value);
  int     operator>(const BigNum &value);
  BigNum &FindPrime(UINT bits, const BigNum &minimum, const BigNum &maximum);

  BigNum &FindPrime(UINT bits, const BigNum &seed);
  BigNum &Gcd(const BigNum &a, const BigNum &b);
  BigNum &Gcd(const BigNum &value);
  BigNum &InvMod(const BigNum &a, const BigNum &b);
  BigNum &InvMod(const BigNum &value);
  BigNum &MulMod(const BigNum &a, const BigNum &b, const BigNum &modulus);
  BigNum &MulMod(const BigNum &a, const BigNum &b);
  BigNum &Pow(const BigNum &value, UINT exponent);
  BigNum &Pow(UINT exponent);
  BigNum &PowMod(const BigNum &b, const BigNum &c, const BigNum &d) { SBigPowMod(m_data, *b.m_data, *c.m_data, *d.m_data); return *this; }
  BigNum &PowMod(const BigNum &value, const BigNum &modulus);
  BigNum &Rand(const BigNum &maximum, BigNum *seed);
  BigNum &Square(const BigNum &value);
  BigNum &Square();

  char   *ToStr(char *buffer, UINT bytes) const;
  LPVOID  ToBinaryBuffer(LPVOID data, UINT bytes) const {
    UINT actual;
    SBigToBinaryBuffer(*m_data, data, bytes, &actual);
    if (actual < bytes)
      memset((BYTE *)data + actual, 0, bytes - actual);
    return data; }
          operator UINT();
  void    FromBinary(LPCVOID data, UINT bytes) { SBigFromBinary(m_data, data, bytes); }
  BOOL    IsEven();
  BOOL    IsOdd();
  BOOL    IsOne();
  BOOL    IsPrime();
  BOOL    IsZero();
  BigNum &Set2Exp(UINT exponent);
  BigNum &SetOne();
  BigNum &SetZero();
};

namespace Crypt {
  class RSA {
   private:
    BigNum m_modulus;
    BigNum m_exponent;

   public:
    void Prepare(LPCVOID modulus, DWORD mLength, LPCVOID exponent, DWORD eLength);
    void Process(BYTE *data, DWORD size);
  };
}

class SSignatureData;

extern "C" void  SSignatureVerifyStream_Begin(SSignatureData **token, DWORD modulusSize, DWORD pubExponentSize);
extern "C" DWORD SSignatureVerifyStream_GetSignatureLength(SSignatureData *token);
extern "C" void  SSignatureVerifyStream_ProvideData(SSignatureData *token, const BYTE *data, DWORD size);
extern "C" int   SSignatureVerifyStream_Finish(SSignatureData *token, const BYTE *modulus, const BYTE *pubExponent);
extern "C" int SSignatureVerify(const BYTE *data, DWORD size, const BYTE *modulus, DWORD modulusSize, const BYTE *pubExponent, DWORD pubExponentSize);
extern "C" int SSignatureGenerate(
    BYTE       *data,
    DWORD      &size,
    const BYTE *modulus,
    DWORD       modulusSize,
    const BYTE *privExponent,
    DWORD       privExponentSize,
    const BYTE *pubExponent,
    DWORD       pubExponentSize
);
