#include "Frame/CSimpleSlider.h"

#include "Event/CMouseEvent.h"
#include "Frame/CSimpleRender.h"
#include "FrameXML/LoadXML.h"
#include "FrameXML/XMLTree.h"

#include <Base/Status.h>
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

void CSimpleSlider::LoadXML(const XMLNode *node, CStatus *status) {
  CSimpleFrame::LoadXML(node, status);

  unsigned int layer = 3;
  const char  *value = node->GetAttributeByName("drawLayer");
  if (value && *value) {
    StringToDrawLayer(value, layer);
  }

  for (const XMLNode *child = node->GetChild(); child; child = child->GetSibling()) {
    if (!SStrCmpI(child->GetName(), "ThumbTexture", INT_MAX)) {
      SetThumbTexture(LoadXML_Texture(child, this, status), 3);
    }
  }

  value = node->GetAttributeByName("minValue");
  if (value && *value) {
    float min = SStrToFloat(value);
    value = node->GetAttributeByName("maxValue");
    if (value && *value) {
      float max = SStrToFloat(value);
      SetMinMaxValues(min, max);

      value = node->GetAttributeByName("valueStep");
      if (value && *value) {
        SetValueStep(SStrToFloat(value));
      }

      value = node->GetAttributeByName("defaultValue");
      if (value && *value) {
        SetValue(SStrToFloat(value));
      }
    }
  }

  value = node->GetAttributeByName("orientation");
  if (value && *value) {
    if (!SStrCmpI(value, "HORIZONTAL", INT_MAX)) {
      SetOrientation(SLIDER_HORIZONTAL);
    } else if (!SStrCmpI(value, "VERTICAL", INT_MAX)) {
      SetOrientation(SLIDER_VERTICAL);
    } else {
      status->Add(STATUS_WARNING, "Unknown orientation: %s", value);
    }
  }
}

void CSimpleSlider::LoadXML_Scripts(const XMLNode *node, CStatus *status) {
  CSimpleFrame::LoadXML_Scripts(node, status);

  for (const XMLNode *script = node->GetChild(); script; script = script->GetSibling()) {
    if (!SStrCmpI(script->GetName(), "OnValueChanged", INT_MAX)) {
      char description[1024];
      SStrPrintf(description, sizeof(description), "%s:OnValueChanged", GetName());
      SetEventScript(m_onValueChanged, script->GetBody(), description);
    }
  }
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

void CSimpleSlider::SetOrientation(SLIDER_ORIENTATION orientation) {
  m_orientation = orientation;
  if (m_thumbTexture) {
    m_thumbTexture->ClearAllPoints(1);
  }
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

void CSimpleSlider::OnLayerUpdate(float elapsedSec) {
  CSimpleFrame::OnLayerUpdate(elapsedSec);

  if (m_changed && m_thumbTexture && m_rangeSet && m_valueSet) {
    float offset = (m_value - m_baseValue) / m_range;
    if (m_orientation == SLIDER_VERTICAL) {
      float area = m_rect.b - m_rect.t - m_thumbTexture->GetHeight();
      m_thumbTexture->SetPoint(FRAMEPOINT_TOP, this, FRAMEPOINT_TOP, 0.0f, -(area * offset / m_layoutScale), 1);
    } else {
      float area = m_rect.r - m_rect.l - m_thumbTexture->GetWidth();
      m_thumbTexture->SetPoint(FRAMEPOINT_LEFT, this, FRAMEPOINT_LEFT, area * offset / m_layoutScale, 0.0f, 1);
    }
    m_changed = 0;
  }
}

int CSimpleSlider::OnLayerTrackUpdate(const CMouseEvent &evt) {
  if (m_buttonDown) {
    float area;
    float offset;
    if (m_orientation == SLIDER_VERTICAL) {
      area = m_rect.b - m_rect.t - m_thumbTexture->GetHeight();
      offset = m_rect.b - m_thumbTexture->GetHeight() * 0.5f - evt.y;
    } else {
      area = m_rect.r - m_rect.l - m_thumbTexture->GetWidth();
      offset = evt.x - (m_thumbTexture->GetWidth() * 0.5f + m_rect.l);
    }
    SetValue(m_range * (offset / area) + m_baseValue);
  }
  return CSimpleFrame::OnLayerTrackUpdate(evt);
}

void CSimpleSlider::OnFrameSizeChanged(const NTempest::CRect &rect) {
  CSimpleFrame::OnFrameSizeChanged(rect);
  m_changed = 1;
}

int CSimpleSlider::OnLayerMouseDown(CMouseEvent &evt) {
  m_buttonDown = 1;
  OnLayerTrackUpdate(evt);
  return CSimpleFrame::OnLayerMouseDown(evt);
}

int CSimpleSlider::OnLayerMouseUp(CMouseEvent &evt) {
  m_buttonDown = 0;
  return CSimpleFrame::OnLayerMouseUp(evt);
}
