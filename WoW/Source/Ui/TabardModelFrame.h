#ifndef WOW_SOURCE_UI_TABARDMODELFRAME_H
#define WOW_SOURCE_UI_TABARDMODELFRAME_H

#include "Ui/CharacterModelBase.h"

#include <FrameScript/FrameScript.h>

struct HTEXCOMPONENT__;
class CGPlayer_C;

class CGTabardModelFrame : public CGCharacterModelBase {
 public:
  static CSimpleFrame *Create(CSimpleFrame *parent) {
    return NEW(CGTabardModelFrame)(parent);
  }

  static void RegisterScriptMethods();
  static void UnregisterScriptMethods();

  void SaveTabard();
  int  CanSaveTabard();
  void CycleVariation(unsigned int index, int delta);

  virtual void         InitializeModel(HMODEL model);
  virtual bool GetUniquePaperDollModel() {
    return true;
  }

  int GetVariation(unsigned int index) {
    FATALASSERT(index < 5);
    return m_variations[index];
  }

 protected:
  virtual ~CGTabardModelFrame();
  virtual int LookupScriptMethod(lua_State *L, const char *name);
  void        UpdateTabard();

  CGTabardModelFrame(const CGTabardModelFrame &);
  CGTabardModelFrame(CSimpleFrame *parent);
  void InitializeTabardColors(const CGPlayer_C *playerPtr);

  static TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> s_scriptMethods;

 private:
  int              m_variations[5];
  HTEXCOMPONENT__ *m_charComponent;
};

#endif
