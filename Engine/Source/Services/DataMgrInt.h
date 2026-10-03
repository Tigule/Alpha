inline void CAngle::Calc() {
  NTempest::CMath::sincos_(m_data, m_sin, m_cos);
}

inline CAngle::CAngle(float angle) {
  TManaged<float>::Set_(ClampTo2Pi(angle));
  Calc();
}

inline CAngle::CAngle() : TManaged<float>() {
  Calc();
}

inline void CAngle::Set_(const float &angle) {
  const float wrapped = ClampTo2Pi(angle);
  TManaged<float>::Set_(wrapped);
  Calc();
}
