#ifndef WOW_SOURCE_UI_TABARDMODELFRAME_H
#define WOW_SOURCE_UI_TABARDMODELFRAME_H

#include "Component/Component.h"
#include "Ui/CharacterModelBase.h"

#include <FrameScript/FrameScript.h>

class CGPlayer_C;

#define TABARDVARS_NUMVARS 5

class CGTabardModelFrame : public CGCharacterModelBase {
 public:
  static CSimpleFrame *Create(CSimpleFrame *parent) {
    return NEW(CGTabardModelFrame)(parent);
  }

  static void RegisterScriptMethods();
  static void UnregisterScriptMethods();

  virtual bool GetUniquePaperDollModel() {
    return true;
  }
  virtual void InitializeModel(HMODEL model);

  void SaveTabard();
  BOOL CanSaveTabard();
  void CycleVariation(UINT index, int delta);

  int GetVariation(UINT index) {
    FATALASSERT(index < TABARDVARS_NUMVARS);
    return m_variations[index];
  }

 protected:
  CGTabardModelFrame(const CGTabardModelFrame &);
  CGTabardModelFrame(CSimpleFrame *parent);
  virtual ~CGTabardModelFrame() {
    if (m_charComponent) {
      HandleClose(m_charComponent);
    }
  }
  virtual BOOL LookupScriptMethod(lua_State *L, LPCSTR name);

  static TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> s_scriptMethods;

  void InitializeTabardColors(const CGPlayer_C *playerPtr);
  void UpdateTabard();

 private:
  int           m_variations[TABARDVARS_NUMVARS];
  HTEXCOMPONENT m_charComponent;
};

#endif
