#include <storm.h>

BigNum &BigNum::PowMod(const BigNum &b, const BigNum &c, const BigNum &d) {
  SBigPowMod(m_data, *b.m_data, *c.m_data, *d.m_data);
  return *this;
}

LPVOID BigNum::ToBinaryBuffer(LPVOID data, UINT bytes) const {
  UINT actual;

  SBigToBinaryBuffer(*m_data, data, bytes, &actual);
  if (actual < bytes) {
    memset((BYTE *)data + actual, 0, bytes - actual);
  }
  return data;
}

void BigNum::FromBinary(LPCVOID data, UINT bytes) {
  SBigFromBinary(m_data, data, bytes);
}

namespace Crypt {

  void RSA::Prepare(LPCVOID modulus, DWORD mLength, LPCVOID exponent, DWORD eLength) {
    m_modulus.FromBinary(modulus, mLength);
    m_exponent.FromBinary(exponent, eLength);
  }

  void RSA::Process(BYTE *data, DWORD length) {
    BigNum src;

    src.FromBinary(data, length);
    BigNum dst;
    dst.PowMod(src, m_exponent, m_modulus);
    dst.ToBinaryBuffer(data, length);
  }

}  // namespace Crypt
