#pragma once

#include "Tempest/c4quaternion.h"

namespace NTempest {

  class C4QuaternionCompressed {
   private:
    LONGLONG m_data;

    float GetX() const {
      return static_cast<float>(static_cast<int>(m_data >> 42)) * 0.000000476837158203125f;
    }

    float GetY() const {
      return static_cast<float>(static_cast<int>((m_data << 22) >> 43)) * 0.00000095367431640625f;
    }

    float GetZ() const {
      return static_cast<float>(static_cast<int>((m_data << 43) >> 43)) * 0.00000095367431640625f;
    }

    float GetW(float x, float y, float z) const {
      float magnitude = x * x + y * y + z * z;
      return CMath::fabs_(magnitude - 1.0f) < 0.00000095367432f ? 0.0f : CMath::sqrt_(1.0f - magnitude);
    }

   public:
    C4QuaternionCompressed() : m_data(0) {
    }

    C4QuaternionCompressed(LONGLONG data) : m_data(data) {
    }

    C4QuaternionCompressed(const C4QuaternionCompressed &source) : m_data(source.m_data) {
    }

    C4QuaternionCompressed(const C4Quaternion &source) {
      Set(source);
    }

    C4QuaternionCompressed &operator=(const C4Quaternion &source) {
      Set(source);
      return *this;
    }

    C4QuaternionCompressed &operator=(const C4QuaternionCompressed &source) {
      m_data = source.m_data;
      return *this;
    }

    void Set(const C4Quaternion &source) {
      int sign = source.w < 0.0f ? -1 : 1;
      int x = CMath::fint_(source.x * 2097152.0f) * sign;
      int y = CMath::fint_(source.y * 1048576.0f) * sign;
      int z = CMath::fint_(source.z * 1048576.0f) * sign;
      m_data = (((static_cast<LONGLONG>(x) << 21) | (y & 0x1FFFFF)) << 21) | (z & 0x1FFFFF);
    }

    operator C4Quaternion() const {
      float x = GetX();
      float y = GetY();
      float z = GetZ();
      float w = GetW(x, y, z);
      return C4Quaternion(w, x, y, z);
    }

    LONGLONG Raw() const {
      return m_data;
    }

    void Identity() {
      m_data = 0;
    }

    bool IsIdentity() const {
      return m_data == 0;
    }

    void FromRotationMatrix(const C33Matrix &matrix) {
      C4Quaternion quaternion;
      quaternion.FromRotationMatrix(matrix);
      Set(quaternion);
    }

    void FromRotationMatrixInv(const C33Matrix &matrix) {
      C4Quaternion quaternion;
      quaternion.FromRotationMatrixInv(matrix);
      Set(quaternion);
    }

    static C4Quaternion Slerp(float ratio, const C4QuaternionCompressed &start, const C4QuaternionCompressed &end) {
      return C4Quaternion::Slerp(ratio, static_cast<C4Quaternion>(start), static_cast<C4Quaternion>(end));
    }

    static C4Quaternion Squad(
        float                         ratio,
        const C4QuaternionCompressed &start,
        const C4QuaternionCompressed &end,
        const C4QuaternionCompressed &outTangent,
        const C4QuaternionCompressed &inTangent
    ) {
      return C4Quaternion::Squad(
          ratio, static_cast<C4Quaternion>(start), static_cast<C4Quaternion>(outTangent), static_cast<C4Quaternion>(inTangent),
          static_cast<C4Quaternion>(end)
      );
    }

  };

}  // namespace NTempest
