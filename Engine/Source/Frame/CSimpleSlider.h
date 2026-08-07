#ifndef ENGINE_SOURCE_FRAME_CSIMPLESLIDER_H
#define ENGINE_SOURCE_FRAME_CSIMPLESLIDER_H

#include "Frame/CSimpleFrame.h"

class CSimpleTexture;

enum SLIDER_ORIENTATION {
  SLIDER_HORIZONTAL = 0,
  SLIDER_VERTICAL = 1
};

class CSimpleSlider : public CSimpleFrame {
 public:
  CSimpleSlider(CSimpleFrame *parent = 0);
  virtual ~CSimpleSlider();

  virtual void LoadXML(const XMLNode *node, CStatus *status);
  virtual void LoadXML_Scripts(const XMLNode *node, CStatus *status);
  virtual void OnLayerUpdate(float elapsedSec);
  virtual BOOL OnLayerTrackUpdate(const CMouseEvent &evt);
  virtual void OnFrameSizeChanged(const NTempest::CRect &rect);
  virtual BOOL OnLayerMouseDown(CMouseEvent &evt);
  virtual BOOL OnLayerMouseUp(CMouseEvent &evt);

  void SetThumbTexture(CSimpleTexture *texture, int layer);
  void SetOrientation(SLIDER_ORIENTATION orientation);
  void SetMinMaxValues(float min, float max);
  void SetValue(float value);
  void SetValueStep(float step);

  float GetMinValue() const {
    return m_baseValue;
  }

  float GetMaxValue() const {
    return m_baseValue + m_range;
  }

  float GetValue() const {
    return m_value;
  }

  float GetValueStep() const {
    return m_valueStep;
  }

  SLIDER_ORIENTATION GetOrientation() const {
    return m_orientation;
  }

  BOOL IsHorizontal() const {
    return m_orientation == SLIDER_HORIZONTAL;
  }

  BOOL IsVertical() const {
    return m_orientation == SLIDER_VERTICAL;
  }

  static void RegisterScriptMethods();
  static void UnregisterScriptMethods();

  void SetOnValueChangedScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnValueChanged", GetName());
    SetEventScript(m_onValueChanged, source, description);
  }

  void RunOnValueChangedScript() {
    if (m_onValueChanged) {
      FrameScript_Execute(m_onValueChanged, this, "%f", m_value);
    }
  }

 protected:
  virtual BOOL LookupScriptMethod(lua_State *L, LPCSTR name);
  float        StepValue(float value) {
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

    return value;
  }

  static TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> s_scriptMethods;

  int                m_changed : 1;
  int                m_rangeSet : 1;
  int                m_valueSet : 1;
  int                m_buttonDown : 1;
  float              m_baseValue;
  float              m_range;
  float              m_value;
  float              m_valueStep;
  CSimpleTexture    *m_thumbTexture;
  SLIDER_ORIENTATION m_orientation;
  int                m_onValueChanged;
};

#endif
