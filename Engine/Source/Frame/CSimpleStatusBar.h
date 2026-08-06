#ifndef ENGINE_SOURCE_FRAME_CSIMPLESTATUSBAR_H
#define ENGINE_SOURCE_FRAME_CSIMPLESTATUSBAR_H

#include "Frame/CSimpleFrame.h"

class CSimpleTexture;

namespace NTempest {
  class CImVector;
}

class CSimpleStatusBar : public CSimpleFrame {
 public:
  CSimpleStatusBar(CSimpleFrame *parent = 0);
  virtual ~CSimpleStatusBar();

  virtual void LoadXML(const XMLNode *node, CStatus *status);
  virtual void LoadXML_Scripts(const XMLNode *node, CStatus *status);
  virtual void OnLayerUpdate(float elapsedSec);

  void         SetBarTexture(CSimpleTexture *texture, int layer);
  int          SetBarTexture(LPCSTR texFile, int layer);
  void         SetMinMaxValues(float min, float max);
  virtual void SetValue(float value);

  float GetValue() const {
    return m_value;
  }

  float GetMinValue() const {
    return m_minValue;
  }

  float GetMaxValue() const {
    return m_maxValue;
  }

  virtual void  SetStatusBarColor(const NTempest::CImVector &color);
  virtual float GetAnimValue() const;

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
  virtual int LookupScriptMethod(lua_State *L, LPCSTR name);

  static TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> s_scriptMethods;

  int             m_changed : 1;
  int             m_rangeSet : 1;
  int             m_valueSet : 1;
  float           m_minValue;
  float           m_maxValue;
  float           m_value;
  CSimpleTexture *m_barTexture;
  int             m_onValueChanged;
};

#endif
