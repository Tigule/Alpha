#include "Frame/CSimpleSlider.h"

#include "Frame/CSimpleRender.h"

#include <limits.h>

CSimpleSlider::CSimpleSlider(CSimpleFrame *parent)
    : CSimpleFrame(parent),
      m_changed(0),
      m_rangeSet(0),
      m_valueSet(0),
      m_buttonDown(0),
      m_valueStep(0.0f),
      m_thumbTexture(0),
      m_orientation(SLIDER_VERTICAL),
      m_onValueChanged(0) {
  EnableEvent(SIMPLE_EVENT_MOUSE, UINT_MAX);
}

CSimpleSlider::~CSimpleSlider() {
  SetThumbTexture(0, 3);

  char description[1024];
  SStrPrintf(description, sizeof(description), "%s:OnValueChanged", GetName());
  SetEventScript(m_onValueChanged, 0, description);
}

void CSimpleSlider::SetThumbTexture(CSimpleTexture *texture, int layer) {
  if (m_thumbTexture) {
    DEL(m_thumbTexture);
  }

  if (texture) {
    texture->SetFrame(this, layer, 1);
    texture->ClearAllPoints(1);
  }

  m_thumbTexture = texture;
  m_changed = 1;
}

void CSimpleSlider::SetMinMaxValues(float min, float max) {
  ASSERT(min <= max);

  m_baseValue = min;
  m_range = max - min;
  m_changed = 1;
  m_rangeSet = 1;

  if (m_valueSet) {
    SetValue(m_value);
  }
}

void CSimpleSlider::SetValue(float value) {
  if (!m_rangeSet) {
    return;
  }

  if (value < m_baseValue) {
    value = m_baseValue;
  }

  float max = m_baseValue + m_range;
  if (value > max) {
    value = max;
  }

  if (m_valueStep != 0.0f) {
    float delta = value - m_baseValue;
    float halfStep = m_valueStep * 0.5f;
    int   steps;

    if (delta > 0.0f) {
      steps = static_cast<int>((delta + halfStep) / m_valueStep);
    } else {
      steps = static_cast<int>((delta - halfStep) / m_valueStep);
    }

    value = steps * m_valueStep + m_baseValue;
  }

  if (!m_valueSet || value != m_value) {
    m_value = value;
    m_changed = 1;
    m_valueSet = 1;

    if (m_onValueChanged) {
      FrameScript_Execute(m_onValueChanged, this, "%f", value);
    }
  }
}

void CSimpleSlider::SetValueStep(float step) {
  ASSERT(step >= 0.0f);

  m_valueStep = step;

  if (m_rangeSet) {
    SetMinMaxValues(m_baseValue, m_baseValue + m_range);
  }
}
