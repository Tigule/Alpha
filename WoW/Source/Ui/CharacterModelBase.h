#ifndef WOW_SOURCE_UI_CHARACTERMODELBASE_H
#define WOW_SOURCE_UI_CHARACTERMODELBASE_H
#include <Frame/CSimpleModel.h>
#include <storm.h>
class CGCharacterModelBase : public CSimpleModel {
 public:
  virtual ~CGCharacterModelBase();
  CGCharacterModelBase(CSimpleFrame *parent);

  static CSimpleFrame *Create(CSimpleFrame *parent);
  static void          RegisterScriptMethods();
  static void          UnregisterScriptMethods();

  virtual void UpdateModel();
  virtual void InitializeModel(HMODEL model) {
  }
  virtual bool GetUniquePaperDollModel() {
    return false;
  }
  void SetUnit(DWORDLONG unitGUID);
  void SetRotationScale(float rotationScale) {
    m_rotationScale = rotationScale;
  }

 protected:
  virtual BOOL LookupScriptMethod(lua_State *L, LPCSTR name);
  void         ConfigureCamera();

  static TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> s_scriptMethods;

 private:
  DWORDLONG m_unit;
  float     m_rotationScale;
};

inline CSimpleFrame *CGCharacterModelBase::Create(CSimpleFrame *parent) {
  return NEW(CGCharacterModelBase)(parent);
}

#endif
