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

  char description[1024];
  SStrPrintf(description, sizeof(description), "%s:OnValueChanged", GetName());
  SetEventScript(m_onValueChanged, 0, description);
}

void CSimpleStatusBar::LoadXML(const XMLNode *node, CStatus *status) {
  CSimpleFrame::LoadXML(node, status);

  unsigned int layer = 2;
  const char  *value = node->GetAttributeByName("drawLayer");
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
      char description[1024];
      SStrPrintf(description, sizeof(description), "%s:OnValueChanged", GetName());
      SetEventScript(m_onValueChanged, script->GetBody(), description);
    }
  }
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

int CSimpleStatusBar::SetBarTexture(const char *texFile, int layer) {
  if (m_barTexture) {
    m_barTexture->SetTexture(texFile, 0);
    return 1;
  }

  CSimpleTexture *texture = NEW(CSimpleTexture)(0, 2, 1);
  if (texture->SetTexture(texFile, 0)) {
    texture->SetAllPoints(this, 1);
    SetBarTexture(texture, layer);
    return 1;
  }

  DEL(texture);
  return 0;
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

  if (value < m_minValue) {
    value = m_minValue;
  }

  if (value > m_maxValue) {
    value = m_maxValue;
  }

  if (!m_valueSet || value != m_value) {
    m_value = value;
    m_changed = 1;
    m_valueSet = 1;

    RunOnValueChangedScript();
  }
}

float CSimpleStatusBar::GetAnimValue() const {
  float range = m_maxValue - m_minValue;
  if (range <= 0.0f) {
    return 0.0f;
  }
  return (m_value - m_minValue) / range;
}

void CSimpleStatusBar::OnLayerUpdate(float elapsedSec) {
  CSimpleFrame::OnLayerUpdate(elapsedSec);

  if (m_changed && m_barTexture && m_rangeSet && m_valueSet) {
    NTempest::CRect texRect;
    memset(&texRect, 0, sizeof(texRect));
    texRect.r = GetAnimValue();
    texRect.b = 1.0f;
    m_barTexture->SetTexCoord(texRect);
    m_changed = 0;
  }
}

void CSimpleStatusBar::SetStatusBarColor(const NTempest::CImVector &color) {
  if (m_barTexture) {
    m_barTexture->SetVertexColor(color);
  }
}
