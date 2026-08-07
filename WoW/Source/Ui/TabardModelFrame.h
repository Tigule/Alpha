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

  void SaveTabard();
  BOOL CanSaveTabard();
  void CycleVariation(UINT index, int delta);

  virtual void InitializeModel(HMODEL model);
  virtual bool GetUniquePaperDollModel() {
    return true;
  }

  int GetVariation(UINT index) {
    FATALASSERT(index < TABARDVARS_NUMVARS);
    return m_variations[index];
  }

 protected:
  virtual ~CGTabardModelFrame() {
    if (m_charComponent) {
      HandleClose(m_charComponent);
    }
  }
  virtual BOOL LookupScriptMethod(lua_State *L, LPCSTR name);
  void         UpdateTabard();

  CGTabardModelFrame(const CGTabardModelFrame &);
  CGTabardModelFrame(CSimpleFrame *parent);
  void InitializeTabardColors(const CGPlayer_C *playerPtr);

  static TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> s_scriptMethods;

 private:
  int           m_variations[TABARDVARS_NUMVARS];
  HTEXCOMPONENT m_charComponent;
};

#endif
