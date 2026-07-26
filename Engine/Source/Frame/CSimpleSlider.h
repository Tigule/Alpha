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
  CSimpleSlider(CSimpleFrame *parent);
  virtual ~CSimpleSlider();

  virtual void LoadXML(const XMLNode *node, CStatus *status);
  virtual void LoadXML_Scripts(const XMLNode *node, CStatus *status);

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

  int IsHorizontal() const {
    return m_orientation == SLIDER_HORIZONTAL;
  }

  int IsVertical() const {
    return m_orientation == SLIDER_VERTICAL;
  }

  static void __fastcall RegisterScriptMethods();
  static void __fastcall UnregisterScriptMethods();

  void SetOnValueChangedScript(const char *source) {
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
  virtual int LookupScriptMethod(lua_State *L, const char *name);
  virtual void OnLayerUpdate(float elapsedSec);
  virtual int  OnLayerTrackUpdate(const CMouseEvent &evt);
  virtual void OnFrameSizeChanged(const NTempest::CRect &rect);
  virtual int  OnLayerMouseDown(CMouseEvent &evt);
  virtual int  OnLayerMouseUp(CMouseEvent &evt);
  float StepValue(float value) {
    if (m_valueStep != 0.0f) {
      float delta = value - m_baseValue;
      float halfStep = m_valueStep * 0.5f;
      int steps;

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
