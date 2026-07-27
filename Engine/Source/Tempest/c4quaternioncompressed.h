#pragma once

#include "Tempest/c4quaternion.h"

namespace NTempest {

  class C4QuaternionCompressed {
   private:
    float GetX() const {
      return static_cast<float>(static_cast<int>(static_cast<unsigned __int64>(m_data) >> 32) >> 10) * 0.00000047683716f;
    }

    float GetY() const {
      return static_cast<float>(static_cast<int>(static_cast<unsigned int>(static_cast<unsigned __int64>(m_data) >> 10)) >> 11) * 0.00000095367432f;
    }

    float GetZ() const {
      return static_cast<float>(static_cast<int>(static_cast<unsigned int>(m_data) << 11) >> 11) * 0.00000095367432f;
    }

    float GetW(float x, float y, float z) const {
      float magnitude = x * x + y * y + z * z;
      return CMath::fabs_(magnitude - 1.0f) < 0.00000095367432f ? 0.0f : CMath::sqrt_(1.0f - magnitude);
    }

   public:
    C4QuaternionCompressed() : m_data(0) {
    }

    C4QuaternionCompressed(__int64 data) : m_data(data) {
    }

    C4QuaternionCompressed(const C4QuaternionCompressed &source) : m_data(source.m_data) {
    }

    C4QuaternionCompressed(const C4Quaternion &source) {
      Set(source);
    }

    C4QuaternionCompressed &operator=(const C4QuaternionCompressed &source) {
      m_data = source.m_data;
      return *this;
    }

    C4QuaternionCompressed &operator=(const C4Quaternion &source) {
      Set(source);
      return *this;
    }

    void Set(const C4Quaternion &source);
    operator C4Quaternion() const;

    __int64 Raw() const {
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

    static C4Quaternion Slerp(
        float ratio,
        const C4QuaternionCompressed &start,
        const C4QuaternionCompressed &end
    ) {
      return C4Quaternion::Slerp(ratio, static_cast<C4Quaternion>(start), static_cast<C4Quaternion>(end));
    }

    static C4Quaternion Squad(
        float ratio,
        const C4QuaternionCompressed &start,
        const C4QuaternionCompressed &end,
        const C4QuaternionCompressed &outTangent,
        const C4QuaternionCompressed &inTangent
    ) {
      return C4Quaternion::Squad(
          ratio,
          static_cast<C4Quaternion>(start),
          static_cast<C4Quaternion>(end),
          static_cast<C4Quaternion>(outTangent),
          static_cast<C4Quaternion>(inTangent)
      );
    }

   private:
    __int64 m_data;
  };

}  // namespace NTempest
