#include "Frame/CSimpleSlider.h"

#include <lauxlib.h>
#include <lua.h>

#define GET_SIMPLE_SLIDER_THIS(L, object)                         \
  CSimpleSlider *object = 0;                                      \
  if (lua_type(L, 1) == LUA_TTABLE) {                             \
    lua_rawgeti(L, 1, 0);                                         \
    object = static_cast<CSimpleSlider *>(lua_touserdata(L, -1)); \
    lua_pop(L, 1);                                                \
  } else {                                                        \
    luaL_error(                                                   \
        L,                                                        \
        "Attempt to find 'this' in non-table object (used '.' "   \
        "instead of ':' ?)"                                       \
    );                                                            \
  }                                                               \
  ASSERT(object)

static int CSimpleSlider_SetMinMaxValues(lua_State *L) {
  GET_SIMPLE_SLIDER_THIS(L, object);

  if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
    luaL_error(L, "Usage: SetMinMaxValues(min, max)");
  }

  float min = static_cast<float>(lua_tonumber(L, 2));
  float max = static_cast<float>(lua_tonumber(L, 3));
  object->SetMinMaxValues(min, max);
  return 0;
}

static int CSimpleSlider_GetMinMaxValues(lua_State *L) {
  GET_SIMPLE_SLIDER_THIS(L, object);

  lua_pushnumber(L, object->GetMinValue());
  lua_pushnumber(L, object->GetMaxValue());
  return 2;
}

static int CSimpleSlider_SetValue(lua_State *L) {
  GET_SIMPLE_SLIDER_THIS(L, object);

  if (!lua_isnumber(L, 2)) {
    luaL_error(L, "Usage: SetValue(value)");
  }

  float value = static_cast<float>(lua_tonumber(L, 2));
  object->SetValue(value);
  return 0;
}

static int CSimpleSlider_GetValue(lua_State *L) {
  GET_SIMPLE_SLIDER_THIS(L, object);

  lua_pushnumber(L, object->GetValue());
  return 1;
}

static int CSimpleSlider_SetValueStep(lua_State *L) {
  GET_SIMPLE_SLIDER_THIS(L, object);

  if (!lua_isnumber(L, 2)) {
    luaL_error(L, "Usage: SetValueStep(value)");
  }

  float value = static_cast<float>(lua_tonumber(L, 2));
  object->SetValueStep(value);
  return 0;
}

static int CSimpleSlider_GetValueStep(lua_State *L) {
  GET_SIMPLE_SLIDER_THIS(L, object);

  lua_pushnumber(L, object->GetValueStep());
  return 1;
}

#undef GET_SIMPLE_SLIDER_THIS

static FrameScript_Method SimpleSliderMethods[] = {
    {"SetMinMaxValues", CSimpleSlider_SetMinMaxValues},
    {"GetMinMaxValues", CSimpleSlider_GetMinMaxValues},
    {       "SetValue",        CSimpleSlider_SetValue},
    {       "GetValue",        CSimpleSlider_GetValue},
    {   "SetValueStep",    CSimpleSlider_SetValueStep},
    {   "GetValueStep",    CSimpleSlider_GetValueStep}
};

TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> CSimpleSlider::s_scriptMethods;

void CSimpleSlider::RegisterScriptMethods() {
  FrameScript_Object::FillScriptMethodTable(SimpleSliderMethods, 6, s_scriptMethods);
}

void CSimpleSlider::UnregisterScriptMethods() {
  FrameScript_Object::EmptyScriptMethodTable(s_scriptMethods);
}

int CSimpleSlider::LookupScriptMethod(lua_State *L, const char *name) {
  if (FrameScript_Object::LookupScriptMethod(L, name, s_scriptMethods)) {
    return 1;
  }

  return CSimpleFrame::LookupScriptMethod(L, name);
}
