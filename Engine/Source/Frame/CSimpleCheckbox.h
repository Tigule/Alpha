#ifndef ENGINE_SOURCE_FRAME_CSIMPLECHECKBOX_H
#define ENGINE_SOURCE_FRAME_CSIMPLECHECKBOX_H

#include "Frame/CSimpleButton.h"

class CSimpleTexture;

class CSimpleCheckbox : public CSimpleButton {
 public:
  CSimpleCheckbox(CSimpleFrame *parent = 0);
  virtual ~CSimpleCheckbox();
  virtual void LoadXML(const XMLNode *node, CStatus *status);
  BOOL SetCheckedTexture(LPCSTR texFile);
  void SetCheckedTexture(CSimpleTexture *texture);
  BOOL SetDisabledCheckedTexture(LPCSTR texFile);
  void SetDisabledCheckedTexture(CSimpleTexture *texture);
  virtual void Enable(int enabled);
  void SetChecked(int state, int force);

  int GetChecked() {
    return m_checked;
  }

  virtual void OnClick(MOUSEBUTTON click);
  static void RegisterScriptMethods();
  static void UnregisterScriptMethods();

 protected:
  virtual BOOL LookupScriptMethod(lua_State *L, LPCSTR name);
  static TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> s_scriptMethods;
  int             m_checked;
  CSimpleTexture *m_checkedTexture;
  CSimpleTexture *m_disabledTexture;
};

#endif
