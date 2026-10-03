#include <Base/Base.h>

#include "Frame/CSimpleStatusBar.h"

#include "Frame/CSimpleRender.h"
#include "FrameXML/LoadXML.h"
#include "FrameXML/XMLTree.h"

#include "Tempest/cimvector.h"

CSimpleStatusBar::CSimpleStatusBar(CSimpleFrame *parent)
    : CSimpleFrame(parent), m_changed(0), m_rangeSet(0), m_valueSet(0), m_barTexture(0), m_onValueChanged(0) {
}

CSimpleStatusBar::~CSimpleStatusBar() {
  SetBarTexture(static_cast<CSimpleTexture *>(0), 2);

  SetOnValueChangedScript(0);
}

void CSimpleStatusBar::LoadXML(const XMLNode *node, CStatus *status) {
  CSimpleFrame::LoadXML(node, status);

  UINT   layer = 2;
  LPCSTR value = node->GetAttributeByName("drawLayer");
  if (value && *value) {
    StringToDrawLayer(value, layer);
  }

  for (const XMLNode *child = node->GetChild(); child; child = child->GetSibling()) {
    if (!SStrCmpI(child->GetName(), "BarTexture", 0x7FFFFFFF)) {
      SetBarTexture(LoadXML_Texture(child, this, status), layer);
    } else if (!SStrCmpI(child->GetName(), "BarColor", 0x7FFFFFFF)) {
      NTempest::CImVector color;
      color.Set(0UL);
      if (LoadXML_Color(child, color, status)) {
        SetStatusBarColor(color);
      }
    }
  }

  value = node->GetAttributeByName("minValue");
  if (value && *value) {
    float min = SStrToFloat(value);
    value = node->GetAttributeByName("maxValue");
    if (value && *value) {
      float max = SStrToFloat(value);
      SetMinMaxValues(min, max);

      value = node->GetAttributeByName("defaultValue");
      if (value && *value) {
        SetValue(SStrToFloat(value));
      }
    }
  }
}

void CSimpleStatusBar::LoadXML_Scripts(const XMLNode *node, CStatus *status) {
  CSimpleFrame::LoadXML_Scripts(node, status);

  for (const XMLNode *script = node->GetChild(); script; script = script->GetSibling()) {
    if (!SStrCmpI(script->GetName(), "OnValueChanged", 0x7FFFFFFF)) {
      SetOnValueChangedScript(script->GetBody());
    }
  }
}

BOOL CSimpleStatusBar::SetBarTexture(LPCSTR texFile, int layer) {
  int okay = 1;

  if (m_barTexture) {
    m_barTexture->SetTexture(texFile, 0);
  } else {
    CSimpleTexture *texture = NEW(CSimpleTexture)(0, 2, 1);
    if (texture->SetTexture(texFile, 0)) {
      texture->SetAllPoints(this, 1);
      SetBarTexture(texture, layer);
    } else {
      DEL(texture);
      okay = 0;
    }
  }

  return okay;
}

void CSimpleStatusBar::SetBarTexture(CSimpleTexture *texture, int layer) {
  if (m_barTexture) {
    DEL(m_barTexture);
  }

  if (texture) {
    texture->SetFrame(this, layer, 1);
    texture->m_TexCoordModifiesPosition = 1;
  }

  m_barTexture = texture;
  m_changed = 1;
}

void CSimpleStatusBar::SetMinMaxValues(float min, float max) {
  ASSERT(min <= max);

  m_minValue = min;
  m_maxValue = max;
  m_changed = 1;
  m_rangeSet = 1;

  if (m_valueSet) {
    float value = m_value;
    m_valueSet = 0;
    SetValue(value);
  }
}

void CSimpleStatusBar::SetValue(float value) {
  ASSERT(m_rangeSet);

  value = __min(m_maxValue, __max(m_minValue, value));

  if (!m_valueSet || value != m_value) {
    m_value = value;
    m_changed = 1;
    m_valueSet = 1;

    RunOnValueChangedScript();
  }
}

float CSimpleStatusBar::GetAnimValue() const {
  float range = m_maxValue - m_minValue;
  if (range > 0.0f) {
    return (m_value - m_minValue) / range;
  }
  return 0.0f;
}

void CSimpleStatusBar::OnLayerUpdate(float elapsedSec) {
  CSimpleFrame::OnLayerUpdate(elapsedSec);

  if (m_changed && m_barTexture && m_rangeSet && m_valueSet) {
    NTempest::CRect texRect;
    texRect.r = GetAnimValue();
    texRect.t = 0.0f;
    texRect.b = 1.0f;
    m_barTexture->SetTexCoord(texRect);
    m_changed = 0;
  }
}

