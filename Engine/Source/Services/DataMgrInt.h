inline CAngle::CAngle(float angle) : TManaged<float>(ClampTo2Pi(angle)) {
  m_sin = sinf(m_data);
  m_cos = cosf(m_data);
}

inline void CAngle::Set_(const float &angle) {
  const float wrapped = ClampTo2Pi(angle);
  TManaged<float>::Set_(wrapped);
  m_sin = sinf(m_data);
  m_cos = cosf(m_data);
}
