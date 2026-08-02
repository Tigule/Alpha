#include <Base/Base.h>

#include "Services/ParticleSystem2.h"

#include <math.h>

void CParticleKey::Interpolate(float time, NTempest::CImVector &color, int &headCell, int &tailCell, float &scale) {
  float t = (time - m_startTime) * m_ooSegLength * 0.99f + 0.005f;

  color.a = NTempest::CMath::ftol_0_256_(m_startColor.a + m_deltaColor[0] * t);
  color.r = NTempest::CMath::ftol_0_256_(m_startColor.r + m_deltaColor[1] * t);
  color.g = NTempest::CMath::ftol_0_256_(m_startColor.g + m_deltaColor[2] * t);
  color.b = NTempest::CMath::ftol_0_256_(m_startColor.b + m_deltaColor[3] * t);
  scale = m_startScale + m_deltaScale * t;

  float rt = t;
  if (m_repeat != 1.0f) {
    rt = static_cast<float>(fmod(t * m_repeat, 1.0));
  }

  headCell = NTempest::CMath::ftol_0_256_(m_initialHead + m_deltaHead * rt);
  tailCell = NTempest::CMath::ftol_0_256_(m_initialTail + m_deltaTail * rt);
}

CParticleKey::CParticleKey()
    : m_startColor(static_cast<unsigned int>(-1)),
      m_initialHead(0),
      m_deltaHead(0),
      m_initialTail(0),
      m_deltaTail(0),
      m_startScale(1.0f),
      m_deltaScale(0.0f),
      m_startTime(0.0f),
      m_ooSegLength(1.0f),
      m_endTime(1.0f),
      m_endColor(static_cast<unsigned int>(-1)),
      m_headStart(0),
      m_headEnd(0),
      m_tailStart(0),
      m_tailEnd(0),
      m_endScale(1.0f),
      m_repeat(1.0f),
      m_normStartTime(0.0f),
      m_normEndTime(1.0f),
      m_lifeSpan(1.0f) {
  for (unsigned int i = 0; i < 4; ++i) {
    m_deltaColor[i] = 0;
  }
}

void CParticleKey::SetSegment(float normStartTime, float normEndTime) {
  ASSERT(normStartTime >= 0.0f);
  ASSERT(normStartTime < normEndTime);
  ASSERT(fabs(normStartTime - normEndTime) >= 2.3841858e-7f);
  m_normStartTime = normStartTime;
  m_normEndTime = normEndTime;
}

void CParticleKey::SetLifeSpan(float lifeSpan) {
  ASSERT(lifeSpan > 2.3841858e-7f);
  m_lifeSpan = lifeSpan;
  m_startTime = lifeSpan * m_normStartTime;
  m_endTime = lifeSpan * m_normEndTime;
  float timeDelta = m_endTime - m_startTime;
  ASSERT(fabs(timeDelta) >= 2.3841858e-7f);
  m_ooSegLength = 1.0f / timeDelta;
  ASSERT(fabs(m_ooSegLength) >= 2.3841858e-7f);
}

void CParticleKey::SetRepeat(float repeat) {
  ASSERT(repeat >= 1.0f);
  m_repeat = repeat;
}

void CParticleKey::SetColors(NTempest::CImVector start, NTempest::CImVector end) {
  m_startColor = start;
  m_endColor = end;
  m_deltaColor[0] = end.a - start.a;
  m_deltaColor[1] = end.r - start.r;
  m_deltaColor[2] = end.g - start.g;
  m_deltaColor[3] = end.b - start.b;
}

void CParticleKey::SetHeadCells(int start, int end) {
  m_headStart = start;
  m_headEnd = end;
  if (end < start) {
    m_initialHead = start + 1;
    m_deltaHead = end - start - 1;
  } else {
    m_initialHead = start;
    m_deltaHead = end - start + 1;
  }
}

void CParticleKey::SetTailCells(int start, int end) {
  m_tailStart = start;
  m_tailEnd = end;
  if (end < start) {
    m_initialTail = start + 1;
    m_deltaTail = end - start - 1;
  } else {
    m_initialTail = start;
    m_deltaTail = end - start + 1;
  }
}

void CParticleKey::SetScales(float start, float end) {
  m_startScale = start;
  m_endScale = end;
  m_deltaScale = end - start;
}

void CParticleKey::Segment(float &startTime, float &endTime) {
  startTime = m_startTime;
  endTime = m_endTime;
}

void CParticleKey::Repeat(float &repeat) {
  repeat = m_repeat;
}

void CParticleKey::LifeSpan(float &lifeSpan) {
  lifeSpan = m_lifeSpan;
}

void CParticleKey::Colors(NTempest::CImVector &start, NTempest::CImVector &end) {
  start = m_startColor;
  end = m_endColor;
}

void CParticleKey::HeadCells(int &start, int &end) {
  start = m_headStart;
  end = m_headEnd;
}

void CParticleKey::TailCells(int &start, int &end) {
  start = m_tailStart;
  end = m_tailEnd;
}

void CParticleKey::Scales(float &start, float &end) {
  start = m_startScale;
  end = m_endScale;
}
