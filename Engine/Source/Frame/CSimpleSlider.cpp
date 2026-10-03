#include <Base/Base.h>

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

  SetOnValueChangedScript(0);
}

void CSimpleSlider::LoadXML(const XMLNode *node, CStatus *status) {
  CSimpleFrame::LoadXML(node, status);

  UINT   layer = 3;
  LPCSTR value = node->GetAttributeByName("drawLayer");
  if (value && *value) {
    StringToDrawLayer(value, layer);
  }

  for (const XMLNode *child = node->GetChild(); child; child = child->GetSibling()) {
    if (!SStrCmpI(child->GetName(), "ThumbTexture", INT_MAX)) {
      SetThumbTexture(LoadXML_Texture(child, this, status), layer);
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
      SetOnValueChangedScript(script->GetBody());
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
    SetValue(GetValue());
  }
}

void CSimpleSlider::SetValue(float value) {
  if (!m_rangeSet) {
    return;
  }

  value = StepValue(__min(GetMaxValue(), __max(GetMinValue(), value)));

  if (!m_valueSet || value != m_value) {
    m_value = value;
    m_changed = 1;
    m_valueSet = 1;

    RunOnValueChangedScript();
  }
}

void CSimpleSlider::SetValueStep(float step) {
  ASSERT(step >= 0.0f);

  m_valueStep = step;

  if (m_rangeSet) {
    SetMinMaxValues(GetMinValue(), GetMaxValue());
  }
}

void CSimpleSlider::OnLayerUpdate(float elapsedSec) {
  CSimpleFrame::OnLayerUpdate(elapsedSec);

  if (m_changed && m_thumbTexture && m_rangeSet && m_valueSet) {
    float value = (GetValue() - GetMinValue()) / (GetMaxValue() - GetMinValue());
    if (IsHorizontal()) {
      float offset = (m_rect.r - m_rect.l - m_thumbTexture->GetWidth()) * value / m_layoutScale;
      m_thumbTexture->SetPoint(FRAMEPOINT_LEFT, this, FRAMEPOINT_LEFT, offset, 0.0f, 1);
    } else {
      float offset = (m_rect.b - m_rect.t - m_thumbTexture->GetHeight()) * value / m_layoutScale;
      m_thumbTexture->SetPoint(FRAMEPOINT_TOP, this, FRAMEPOINT_TOP, 0.0f, -offset, 1);
    }
    m_changed = 0;
  }
}

BOOL CSimpleSlider::OnLayerTrackUpdate(const CMouseEvent &evt) {
  if (m_buttonDown) {
    if (IsHorizontal()) {
      float area = m_rect.r - m_rect.l - m_thumbTexture->GetWidth();
      float offset = evt.x - (m_thumbTexture->GetWidth() * 0.5f + m_rect.l);
      SetValue((GetMaxValue() - GetMinValue()) * (offset / area) + GetMinValue());
    } else {
      float area = m_rect.b - m_rect.t - m_thumbTexture->GetHeight();
      float offset = m_rect.b - m_thumbTexture->GetHeight() * 0.5f - evt.y;
      SetValue((GetMaxValue() - GetMinValue()) * (offset / area) + GetMinValue());
    }
  }
  return CSimpleFrame::OnLayerTrackUpdate(evt);
}

void CSimpleSlider::OnFrameSizeChanged(const NTempest::CRect &rect) {
  CSimpleFrame::OnFrameSizeChanged(rect);
  m_changed = 1;
}

BOOL CSimpleSlider::OnLayerMouseDown(CMouseEvent &evt) {
  m_buttonDown = 1;
  OnLayerTrackUpdate(evt);
  return CSimpleFrame::OnLayerMouseDown(evt);
}

BOOL CSimpleSlider::OnLayerMouseUp(CMouseEvent &evt) {
  m_buttonDown = 0;
  return CSimpleFrame::OnLayerMouseUp(evt);
}
