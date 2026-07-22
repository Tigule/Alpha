#include "Frame/CSimpleStatusBar.h"

#include "Tempest/cimvector.h"

#include <lauxlib.h>
#include <lua.h>

#define GET_SIMPLE_STATUS_BAR_THIS(L, object)                        \
  CSimpleStatusBar *object = 0;                                      \
  if (lua_type(L, 1) == LUA_TTABLE) {                                \
    lua_rawgeti(L, 1, 0);                                            \
    object = static_cast<CSimpleStatusBar *>(lua_touserdata(L, -1)); \
    lua_pop(L, 1);                                                   \
  } else {                                                           \
    luaL_error(                                                      \
        L,                                                           \
        "Attempt to find 'this' in non-table object (used '.' "      \
        "instead of ':' ?)"                                          \
    );                                                               \
  }                                                                  \
  ASSERT(object)

static int __fastcall CSimpleStatusBar_SetMinMaxValues(lua_State *L) {
  GET_SIMPLE_STATUS_BAR_THIS(L, object);

  if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
    luaL_error(L, "Usage: SetMinMaxValues(min, max)");
  }

  float min = static_cast<float>(lua_tonumber(L, 2));
  float max = static_cast<float>(lua_tonumber(L, 3));
  object->SetMinMaxValues(min, max);
  return 0;
}

static int __fastcall CSimpleStatusBar_GetMinMaxValues(lua_State *L) {
  GET_SIMPLE_STATUS_BAR_THIS(L, object);

  lua_pushnumber(L, object->GetMinValue());
  lua_pushnumber(L, object->GetMaxValue());
  return 2;
}

static int __fastcall CSimpleStatusBar_SetValue(lua_State *L) {
  GET_SIMPLE_STATUS_BAR_THIS(L, object);

  if (!lua_isnumber(L, 2)) {
    luaL_error(L, "Usage: SetValue(value)");
  }

  float value = static_cast<float>(lua_tonumber(L, 2));
  object->SetValue(value);
  return 0;
}

static int __fastcall CSimpleStatusBar_GetValue(lua_State *L) {
  GET_SIMPLE_STATUS_BAR_THIS(L, object);

  lua_pushnumber(L, object->GetValue());
  return 1;
}

static int __fastcall CSimpleStatusBar_SetStatusBarColor(lua_State *L) {
  GET_SIMPLE_STATUS_BAR_THIS(L, object);

  float red = static_cast<float>(lua_tonumber(L, 2));
  float green = static_cast<float>(lua_tonumber(L, 3));
  float blue = static_cast<float>(lua_tonumber(L, 4));
  float alpha = 1.0f;
  if (lua_isnumber(L, 5)) {
    alpha = static_cast<float>(lua_tonumber(L, 5));
  }

  NTempest::CImVector color;
  color.Set(alpha, red, green, blue);
  object->SetStatusBarColor(color);
  return 0;
}

#undef GET_SIMPLE_STATUS_BAR_THIS

static FrameScript_Method SimpleStatusBarMethods[5] = {
    {  "SetMinMaxValues",   CSimpleStatusBar_SetMinMaxValues},
    {  "GetMinMaxValues",   CSimpleStatusBar_GetMinMaxValues},
    {         "SetValue",          CSimpleStatusBar_SetValue},
    {         "GetValue",          CSimpleStatusBar_GetValue},
    {"SetStatusBarColor", CSimpleStatusBar_SetStatusBarColor}
};

TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> CSimpleStatusBar::s_scriptMethods;

void __fastcall CSimpleStatusBar::RegisterScriptMethods() {
  FrameScript_Object::FillScriptMethodTable(SimpleStatusBarMethods, 5, s_scriptMethods);
}

void __fastcall CSimpleStatusBar::UnregisterScriptMethods() {
  FrameScript_Object::EmptyScriptMethodTable(s_scriptMethods);
}

int CSimpleStatusBar::LookupScriptMethod(lua_State *L, const char *name) {
  if (FrameScript_Object::LookupScriptMethod(L, name, s_scriptMethods)) {
    return 1;
  }

  return CSimpleFrame::LookupScriptMethod(L, name);
}
