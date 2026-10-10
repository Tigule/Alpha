#include <Base/Base.h>

#include "Frame/CSimpleTop.h"
#include "Frame/CSimpleStatusBar.h"

#include "Tempest/cimvector.h"

#include <lauxlib.h>
#include <lua.h>

static int CSimpleStatusBar_SetMinMaxValues(lua_State *L) {
  CSimpleStatusBar *object = (CSimpleStatusBar *)FrameScript_GetObjectThis(L);

  if (lua_isnumber(L, 2) && lua_isnumber(L, 3)) {
    float min = lua_tonumber(L, 2);
    float max = lua_tonumber(L, 3);
    object->SetMinMaxValues(min, max);
    return 0;
  }

  luaL_error(L, "Usage: SetMinMaxValues(min, max)");
  return 0;
}

static int CSimpleStatusBar_GetMinMaxValues(lua_State *L) {
  CSimpleStatusBar *object = (CSimpleStatusBar *)FrameScript_GetObjectThis(L);

  lua_pushnumber(L, object->GetMinValue());
  lua_pushnumber(L, object->GetMaxValue());
  return 2;
}

static int CSimpleStatusBar_SetValue(lua_State *L) {
  CSimpleStatusBar *object = (CSimpleStatusBar *)FrameScript_GetObjectThis(L);

  if (lua_isnumber(L, 2)) {
    float value = lua_tonumber(L, 2);
    object->SetValue(value);
    return 0;
  }

  luaL_error(L, "Usage: SetValue(value)");
  return 0;
}

static int CSimpleStatusBar_GetValue(lua_State *L) {
  CSimpleStatusBar *object = (CSimpleStatusBar *)FrameScript_GetObjectThis(L);

  lua_pushnumber(L, object->GetValue());
  return 1;
}

static int CSimpleStatusBar_SetStatusBarColor(lua_State *L) {
  CSimpleStatusBar *object = (CSimpleStatusBar *)FrameScript_GetObjectThis(L);

  NTempest::CImVector color;
  float               red = lua_tonumber(L, 2);
  float               green = lua_tonumber(L, 3);
  float               blue = lua_tonumber(L, 4);
  float               alpha = 1.0f;
  if (lua_isnumber(L, 5)) {
    alpha = lua_tonumber(L, 5);
  }

  color.Set(alpha, red, green, blue);
  object->SetStatusBarColor(color);
  return 0;
}

static FrameScript_Method SimpleStatusBarMethods[5] = {
    {  "SetMinMaxValues",   CSimpleStatusBar_SetMinMaxValues},
    {  "GetMinMaxValues",   CSimpleStatusBar_GetMinMaxValues},
    {         "SetValue",          CSimpleStatusBar_SetValue},
    {         "GetValue",          CSimpleStatusBar_GetValue},
    {"SetStatusBarColor", CSimpleStatusBar_SetStatusBarColor}
};

TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> CSimpleStatusBar::s_scriptMethods;

void CSimpleStatusBar::RegisterScriptMethods() {
  FrameScript_Object::FillScriptMethodTable(SimpleStatusBarMethods, 5, s_scriptMethods);
}

void CSimpleStatusBar::UnregisterScriptMethods() {
  FrameScript_Object::EmptyScriptMethodTable(s_scriptMethods);
}

BOOL CSimpleStatusBar::LookupScriptMethod(lua_State *L, LPCSTR name) {
  if (FrameScript_Object::LookupScriptMethod(L, name, s_scriptMethods)) {
    return 1;
  }

  return CSimpleFrame::LookupScriptMethod(L, name);
}
