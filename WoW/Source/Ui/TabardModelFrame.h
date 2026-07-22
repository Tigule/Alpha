#ifndef WOW_SOURCE_UI_TABARDMODELFRAME_H
#define WOW_SOURCE_UI_TABARDMODELFRAME_H

#include "Ui/CharacterModelBase.h"

#include <FrameScript/FrameScript.h>

struct HTEXCOMPONENT__;
class CGPlayer_C;

class CGTabardModelFrame : public CGCharacterModelBase {
 public:
  virtual ~CGTabardModelFrame();

  static CSimpleFrame *__fastcall Create(CSimpleFrame *parent) {
    return NEW(CGTabardModelFrame)(parent);
  }

  static void __fastcall RegisterScriptMethods();
  static void __fastcall UnregisterScriptMethods();

  void SaveTabard();
  int  CanSaveTabard();
  void CycleVariation(unsigned int index, int delta);

  virtual void         InitializeModel(HMODEL model);
  virtual unsigned int GetUniquePaperDollModel() {
    return 1;
  }

  int GetVariation(unsigned int index) const {
    FATALASSERT(index < 5);
    return m_variations[index];
  }

 protected:
  virtual int LookupScriptMethod(lua_State *L, const char *name);
  void        UpdateTabard();

  static TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> s_scriptMethods;

 private:
  CGTabardModelFrame(CSimpleFrame *parent);
  void InitializeTabardColors(const CGPlayer_C *playerPtr);

  int              m_variations[5];
  HTEXCOMPONENT__ *m_charComponent;
};

#endif
