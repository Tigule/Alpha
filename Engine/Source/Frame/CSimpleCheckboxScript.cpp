#include <Base/Base.h>

#include "Frame/CSimpleTop.h"
#include "Frame/CSimpleCheckbox.h"
#include "FrameXML/LoadXML.h"

#include <lauxlib.h>
#include <lua.h>

static int CSimpleCheckbox_SetChecked(lua_State *L) {
  CSimpleCheckbox *object = (CSimpleCheckbox *)FrameScript_GetObjectThis(L);

  int state = 0;
  if (lua_isnumber(L, 2)) {
    state = lua_tonumber(L, 2);
  } else if (lua_isstring(L, 2)) {
    state = StringToBOOL(lua_tostring(L, 2));
  }

  object->SetChecked(state, 0);
  return 0;
}

static int CSimpleCheckbox_GetChecked(lua_State *L) {
  CSimpleCheckbox *object = (CSimpleCheckbox *)FrameScript_GetObjectThis(L);

  if (object->GetChecked()) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int CSimpleCheckbox_SetCheckedTexture(lua_State *L) {
  CSimpleCheckbox *object = (CSimpleCheckbox *)FrameScript_GetObjectThis(L);

  if (lua_isstring(L, 2)) {
    object->SetCheckedTexture(lua_tostring(L, 2));
    return 0;
  }

  luaL_error(L, "Usage: SetCheckedTexture(\"texture\")");
  return 0;
}

static int CSimpleCheckbox_SetDisabledCheckedTexture(lua_State *L) {
  CSimpleCheckbox *object = (CSimpleCheckbox *)FrameScript_GetObjectThis(L);

  if (lua_isstring(L, 2)) {
    object->SetDisabledCheckedTexture(lua_tostring(L, 2));
    return 0;
  }

  luaL_error(L, "Usage: SetDisabledCheckedTexture(\"texture\")");
  return 0;
}

static FrameScript_Method SimpleCheckboxMethods[] = {
    {               "SetChecked",                CSimpleCheckbox_SetChecked},
    {               "GetChecked",                CSimpleCheckbox_GetChecked},
    {        "SetCheckedTexture",         CSimpleCheckbox_SetCheckedTexture},
    {"SetDisabledCheckedTexture", CSimpleCheckbox_SetDisabledCheckedTexture}
};

TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> CSimpleCheckbox::s_scriptMethods;

void CSimpleCheckbox::RegisterScriptMethods() {
  FrameScript_Object::FillScriptMethodTable(SimpleCheckboxMethods, 4, s_scriptMethods);
}

void CSimpleCheckbox::UnregisterScriptMethods() {
  FrameScript_Object::EmptyScriptMethodTable(s_scriptMethods);
}

BOOL CSimpleCheckbox::LookupScriptMethod(lua_State *L, LPCSTR name) {
  if (FrameScript_Object::LookupScriptMethod(L, name, s_scriptMethods)) {
    return 1;
  }

  return CSimpleButton::LookupScriptMethod(L, name);
}
