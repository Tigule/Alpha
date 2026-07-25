#ifndef ENGINE_SOURCE_FRAME_CSIMPLESTATUSBAR_H
#define ENGINE_SOURCE_FRAME_CSIMPLESTATUSBAR_H

#include "Frame/CSimpleFrame.h"

class CSimpleTexture;

namespace NTempest {
  class CImVector;
}

class CSimpleStatusBar : public CSimpleFrame {
 public:
  CSimpleStatusBar(CSimpleFrame *parent);
  virtual ~CSimpleStatusBar();

  virtual void LoadXML(const XMLNode *node, CStatus *status);
  virtual void LoadXML_Scripts(const XMLNode *node, CStatus *status);

  void         SetBarTexture(CSimpleTexture *texture, int layer);
  int          SetBarTexture(const char *texFile, int layer);
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

  virtual void SetStatusBarColor(const NTempest::CImVector &color);

  static void __fastcall RegisterScriptMethods();
  static void __fastcall UnregisterScriptMethods();

 protected:
  virtual int LookupScriptMethod(lua_State *L, const char *name);

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
