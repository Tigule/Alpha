#ifndef ENGINE_SOURCE_FRAME_CSIMPLECHECKBOX_H
#define ENGINE_SOURCE_FRAME_CSIMPLECHECKBOX_H

#include "Frame/CSimpleButton.h"

class CSimpleTexture;

class CSimpleCheckbox : public CSimpleButton {
 public:
  CSimpleCheckbox(CSimpleFrame *parent = 0);
  virtual ~CSimpleCheckbox();

  static void RegisterScriptMethods();
  static void UnregisterScriptMethods();

  virtual void LoadXML(const XMLNode *node, CStatus *status);
  virtual void Enable(int enabled);
  virtual void OnClick(MOUSEBUTTON click);

  void SetChecked(int state, int force);
  void SetCheckedTexture(CSimpleTexture *texture);
  int  SetCheckedTexture(LPCSTR texFile);
  void SetDisabledCheckedTexture(CSimpleTexture *texture);
  int  SetDisabledCheckedTexture(LPCSTR texFile);

  int GetChecked() {
    return m_checked;
  }

 protected:
  virtual int LookupScriptMethod(lua_State *L, LPCSTR name);

  static TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> s_scriptMethods;

  int             m_checked;
  CSimpleTexture *m_checkedTexture;
  CSimpleTexture *m_disabledTexture;
};

#endif
