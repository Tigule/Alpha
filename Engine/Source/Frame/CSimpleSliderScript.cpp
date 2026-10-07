#include <Base/Base.h>

#include "Frame/CSimpleTop.h"
#include "Frame/CSimpleSlider.h"

#include <lauxlib.h>
#include <lua.h>

static int CSimpleSlider_SetMinMaxValues(lua_State *L) {
  CSimpleSlider *object = static_cast<CSimpleSlider *>(FrameScript_GetObjectThis(L));

  if (lua_isnumber(L, 2) && lua_isnumber(L, 3)) {
    float min = static_cast<float>(lua_tonumber(L, 2));
    float max = static_cast<float>(lua_tonumber(L, 3));
    object->SetMinMaxValues(min, max);
    return 0;
  }

  luaL_error(L, "Usage: SetMinMaxValues(min, max)");
  return 0;
}

static int CSimpleSlider_GetMinMaxValues(lua_State *L) {
  CSimpleSlider *object = static_cast<CSimpleSlider *>(FrameScript_GetObjectThis(L));

  lua_pushnumber(L, object->GetMinValue());
  lua_pushnumber(L, object->GetMaxValue());
  return 2;
}

static int CSimpleSlider_SetValue(lua_State *L) {
  CSimpleSlider *object = static_cast<CSimpleSlider *>(FrameScript_GetObjectThis(L));

  if (lua_isnumber(L, 2)) {
    float value = static_cast<float>(lua_tonumber(L, 2));
    object->SetValue(value);
    return 0;
  }

  luaL_error(L, "Usage: SetValue(value)");
  return 0;
}

static int CSimpleSlider_GetValue(lua_State *L) {
  CSimpleSlider *object = static_cast<CSimpleSlider *>(FrameScript_GetObjectThis(L));

  lua_pushnumber(L, object->GetValue());
  return 1;
}

static int CSimpleSlider_SetValueStep(lua_State *L) {
  CSimpleSlider *object = static_cast<CSimpleSlider *>(FrameScript_GetObjectThis(L));

  if (lua_isnumber(L, 2)) {
    float value = static_cast<float>(lua_tonumber(L, 2));
    object->SetValueStep(value);
    return 0;
  }

  luaL_error(L, "Usage: SetValueStep(value)");
  return 0;
}

static int CSimpleSlider_GetValueStep(lua_State *L) {
  CSimpleSlider *object = static_cast<CSimpleSlider *>(FrameScript_GetObjectThis(L));

  lua_pushnumber(L, object->GetValueStep());
  return 1;
}

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

BOOL CSimpleSlider::LookupScriptMethod(lua_State *L, LPCSTR name) {
  if (FrameScript_Object::LookupScriptMethod(L, name, s_scriptMethods)) {
    return 1;
  }

  return CSimpleFrame::LookupScriptMethod(L, name);
}
