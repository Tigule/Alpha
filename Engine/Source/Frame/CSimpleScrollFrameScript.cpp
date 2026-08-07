#include <Base/Base.h>

#include "Frame/CSimpleScrollFrame.h"

#include <lauxlib.h>
#include <lua.h>

#define GET_SIMPLE_SCROLL_FRAME_THIS(L, object)                        \
  CSimpleScrollFrame *object = 0;                                      \
  if (lua_type(L, 1) == LUA_TTABLE) {                                  \
    lua_rawgeti(L, 1, 0);                                              \
    object = static_cast<CSimpleScrollFrame *>(lua_touserdata(L, -1)); \
    lua_pop(L, 1);                                                     \
  } else {                                                             \
    luaL_error(                                                        \
        L,                                                             \
        "Attempt to find 'this' in non-table object (used '.' "        \
        "instead of ':' ?)"                                            \
    );                                                                 \
  }                                                                    \
  ASSERT(object)

static int CSimpleScrollFrame_SetHorizontalScroll(lua_State *L) {
  GET_SIMPLE_SCROLL_FRAME_THIS(L, object);

  if (!lua_isnumber(L, 2)) {
    luaL_error(L, "Usage: SetHorizontalScroll(offset)");
  }

  float offset = static_cast<float>(lua_tonumber(L, 2) * 0.0009765625f * 0.8f);
  object->SetHorizontalScroll(offset);
  return 0;
}

static int CSimpleScrollFrame_SetVerticalScroll(lua_State *L) {
  GET_SIMPLE_SCROLL_FRAME_THIS(L, object);

  if (!lua_isnumber(L, 2)) {
    luaL_error(L, "Usage: SetVerticalScroll(offset)");
  }

  float offset = static_cast<float>(lua_tonumber(L, 2) * 0.0009765625f * 0.8f);
  object->SetVerticalScroll(offset);
  return 0;
}

static int CSimpleScrollFrame_GetHorizontalScroll(lua_State *L) {
  GET_SIMPLE_SCROLL_FRAME_THIS(L, object);

  lua_pushnumber(L, object->GetHorizontalScroll() * 1024.0f * 1.25f);
  return 1;
}

static int CSimpleScrollFrame_GetVerticalScroll(lua_State *L) {
  GET_SIMPLE_SCROLL_FRAME_THIS(L, object);

  lua_pushnumber(L, object->GetVerticalScroll() * 1024.0f * 1.25f);
  return 1;
}

static int CSimpleScrollFrame_GetHorizontalScrollRange(lua_State *L) {
  GET_SIMPLE_SCROLL_FRAME_THIS(L, object);

  lua_pushnumber(L, object->GetHorizontalScrollRange() * 1024.0f * 1.25f);
  return 1;
}

static int CSimpleScrollFrame_GetVerticalScrollRange(lua_State *L) {
  GET_SIMPLE_SCROLL_FRAME_THIS(L, object);

  lua_pushnumber(L, object->GetVerticalScrollRange() * 1024.0f * 1.25f);
  return 1;
}

static int CSimpleScrollFrame_UpdateScrollChildRect(lua_State *L) {
  GET_SIMPLE_SCROLL_FRAME_THIS(L, object);

  object->UpdateScrollChildRect();
  return 0;
}

#undef GET_SIMPLE_SCROLL_FRAME_THIS

static FrameScript_Method SimpleScrollFrameMethods[7] = {
    {     "SetHorizontalScroll",      CSimpleScrollFrame_SetHorizontalScroll},
    {       "SetVerticalScroll",        CSimpleScrollFrame_SetVerticalScroll},
    {     "GetHorizontalScroll",      CSimpleScrollFrame_GetHorizontalScroll},
    {       "GetVerticalScroll",        CSimpleScrollFrame_GetVerticalScroll},
    {"GetHorizontalScrollRange", CSimpleScrollFrame_GetHorizontalScrollRange},
    {  "GetVerticalScrollRange",   CSimpleScrollFrame_GetVerticalScrollRange},
    {   "UpdateScrollChildRect",    CSimpleScrollFrame_UpdateScrollChildRect}
};

TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> CSimpleScrollFrame::s_scriptMethods;

void CSimpleScrollFrame::RegisterScriptMethods() {
  FrameScript_Object::FillScriptMethodTable(SimpleScrollFrameMethods, 7, s_scriptMethods);
}

void CSimpleScrollFrame::UnregisterScriptMethods() {
  FrameScript_Object::EmptyScriptMethodTable(s_scriptMethods);
}

BOOL CSimpleScrollFrame::LookupScriptMethod(lua_State *L, LPCSTR name) {
  if (FrameScript_Object::LookupScriptMethod(L, name, s_scriptMethods)) {
    return 1;
  }

  return CSimpleFrame::LookupScriptMethod(L, name);
}
