#include <windows.h>
#include <dsound.h>
#include <storm.h>
#include <stdio.h>
#include <time.h>

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
