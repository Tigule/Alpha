#include <Base/Base.h>

#include "Frame/CSimpleTop.h"
#include "Frame/CSimpleScrollFrame.h"

#include <lauxlib.h>
#include <lua.h>

#define GET_SIMPLE_SCROLL_FRAME_THIS(L, object)                        \
  CSimpleScrollFrame *object;                                          \
  if (lua_type(L, 1) != LUA_TTABLE) {                                  \
    luaL_error(                                                        \
        L,                                                             \
        "Attempt to find 'this' in non-table object (used '.' "        \
        "instead of ':' ?)"                                            \
    );                                                                 \
    object = 0;                                                        \
  } else {                                                             \
    lua_rawgeti(L, 1, 0);                                              \
    object = static_cast<CSimpleScrollFrame *>(lua_touserdata(L, -1)); \
    lua_pop(L, 1);                                                     \
    ASSERT(object);                                                    \
  }

static int CSimpleScrollFrame_SetHorizontalScroll(lua_State *L) {
  GET_SIMPLE_SCROLL_FRAME_THIS(L, object);

  if (lua_isnumber(L, 2)) {
    float offset = static_cast<float>(0.8f * (lua_tonumber(L, 2) * 0.0009765625f));
    object->SetHorizontalScroll(offset);
    return 0;
  }

  luaL_error(L, "Usage: SetHorizontalScroll(offset)");
  return 0;
}

static int CSimpleScrollFrame_SetVerticalScroll(lua_State *L) {
  GET_SIMPLE_SCROLL_FRAME_THIS(L, object);

  if (lua_isnumber(L, 2)) {
    float offset = static_cast<float>(0.8f * (lua_tonumber(L, 2) * 0.0009765625f));
    object->SetVerticalScroll(offset);
    return 0;
  }

  luaL_error(L, "Usage: SetVerticalScroll(offset)");
  return 0;
}

static int CSimpleScrollFrame_GetHorizontalScroll(lua_State *L) {
  GET_SIMPLE_SCROLL_FRAME_THIS(L, object);

  lua_pushnumber(L, 1.25f * (object->GetHorizontalScroll() * 1024.0f));
  return 1;
}

static int CSimpleScrollFrame_GetVerticalScroll(lua_State *L) {
  GET_SIMPLE_SCROLL_FRAME_THIS(L, object);

  lua_pushnumber(L, 1.25f * (object->GetVerticalScroll() * 1024.0f));
  return 1;
}

static int CSimpleScrollFrame_GetHorizontalScrollRange(lua_State *L) {
  GET_SIMPLE_SCROLL_FRAME_THIS(L, object);

  lua_pushnumber(L, 1.25f * (object->GetHorizontalScrollRange() * 1024.0f));
  return 1;
}

static int CSimpleScrollFrame_GetVerticalScrollRange(lua_State *L) {
  GET_SIMPLE_SCROLL_FRAME_THIS(L, object);

  lua_pushnumber(L, 1.25f * (object->GetVerticalScrollRange() * 1024.0f));
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
