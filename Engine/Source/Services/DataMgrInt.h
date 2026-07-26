inline void CAngle::Calc() {
  m_sin = sinf(m_data);
  m_cos = cosf(m_data);
}

inline CAngle::CAngle(float angle) : TManaged<float>(ClampTo2Pi(angle)) {
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
