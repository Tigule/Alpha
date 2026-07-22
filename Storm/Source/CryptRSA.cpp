#include <storm.h>

BigNum &BigNum::PowMod(const BigNum &b, const BigNum &c, const BigNum &d) {
  SBigPowMod(m_data, b.m_data, c.m_data, d.m_data);
  return *this;
}

void *BigNum::ToBinaryBuffer(void *data, unsigned int bytes) const {
  unsigned int actual;

  SBigToBinaryBuffer(m_data, data, bytes, &actual);
  if (actual < bytes) {
    memset((BYTE *)data + actual, 0, bytes - actual);
  }
  return data;
}

void BigNum::FromBinary(const void *data, unsigned int bytes) {
  SBigFromBinary(m_data, data, bytes);
}

namespace Crypt {

  void RSA::Prepare(const void *modulus, unsigned long mLength, const void *exponent, unsigned long eLength) {
    m_modulus.FromBinary(modulus, mLength);
    m_exponent.FromBinary(exponent, eLength);
  }

  void RSA::Process(unsigned char *data, unsigned long length) {
    BigNum src;

    src.FromBinary(data, length);
    BigNum dst;
    dst.PowMod(src, m_exponent, m_modulus);
    dst.ToBinaryBuffer(data, length);
  }

}  // namespace Crypt
