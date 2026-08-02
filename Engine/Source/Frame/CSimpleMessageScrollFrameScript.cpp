#include <Base/Base.h>

#include "Frame/CSimpleMessageScrollFrame.h"

#include <lauxlib.h>
#include <lua.h>

#define GET_SIMPLE_MESSAGE_SCROLL_FRAME_THIS(L, object)                       \
  CSimpleMessageScrollFrame *object = 0;                                      \
  if (lua_type(L, 1) == LUA_TTABLE) {                                         \
    lua_rawgeti(L, 1, 0);                                                     \
    object = static_cast<CSimpleMessageScrollFrame *>(lua_touserdata(L, -1)); \
    lua_pop(L, 1);                                                            \
  } else {                                                                    \
    luaL_error(                                                               \
        L,                                                                    \
        "Attempt to find 'this' in non-table object (used '.' instead "       \
        "of ':' ?)"                                                           \
    );                                                                        \
  }                                                                           \
  ASSERT(object)

static int CSimpleMessageScrollFrame_AddMessage(lua_State *L) {
  GET_SIMPLE_MESSAGE_SCROLL_FRAME_THIS(L, object);

  if (!lua_isstring(L, 2)) {
    return 0;
  }

  const char *message = lua_tostring(L, 2);
  if (!message || !*message) {
    return 0;
  }

  if (!lua_isnumber(L, 3) || !lua_isnumber(L, 4) || !lua_isnumber(L, 5)) {
    object->AddMessage(message, 0);
    return 0;
  }

  float red = static_cast<float>(lua_tonumber(L, 3));
  float green = static_cast<float>(lua_tonumber(L, 4));
  float blue = static_cast<float>(lua_tonumber(L, 5));
  float alpha = 1.0f;
  if (lua_isnumber(L, 6)) {
    alpha = static_cast<float>(lua_tonumber(L, 6));
  }

  NTempest::CImVector color;
  color.Set(alpha, red, green, blue);
  CSimpleFontStringAttributes attrib(object->GetTextAttributes());
  attrib.SetColor(color);
  object->AddMessage(message, &attrib);
  return 0;
}

static int CSimpleMessageScrollFrame_ScrollUp(lua_State *L) {
  GET_SIMPLE_MESSAGE_SCROLL_FRAME_THIS(L, object);
  object->ScrollUp();
  return 0;
}

static int CSimpleMessageScrollFrame_ScrollDown(lua_State *L) {
  GET_SIMPLE_MESSAGE_SCROLL_FRAME_THIS(L, object);
  object->ScrollDown();
  return 0;
}

static int CSimpleMessageScrollFrame_PageUp(lua_State *L) {
  GET_SIMPLE_MESSAGE_SCROLL_FRAME_THIS(L, object);
  object->PageUp();
  return 0;
}

static int CSimpleMessageScrollFrame_PageDown(lua_State *L) {
  GET_SIMPLE_MESSAGE_SCROLL_FRAME_THIS(L, object);
  object->PageDown();
  return 0;
}

static int CSimpleMessageScrollFrame_ScrollToTop(lua_State *L) {
  GET_SIMPLE_MESSAGE_SCROLL_FRAME_THIS(L, object);
  object->ScrollToTop();
  return 0;
}

static int CSimpleMessageScrollFrame_ScrollToBottom(lua_State *L) {
  GET_SIMPLE_MESSAGE_SCROLL_FRAME_THIS(L, object);
  object->ScrollToBottom();
  return 0;
}

static int CSimpleMessageScrollFrame_AtBottom(lua_State *L) {
  GET_SIMPLE_MESSAGE_SCROLL_FRAME_THIS(L, object);
  if (object->AtBottom()) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static FrameScript_Method SimpleMessageScrollFrameMethods[] = {
    {    "AddMessage",     CSimpleMessageScrollFrame_AddMessage},
    {      "ScrollUp",       CSimpleMessageScrollFrame_ScrollUp},
    {    "ScrollDown",     CSimpleMessageScrollFrame_ScrollDown},
    {        "PageUp",         CSimpleMessageScrollFrame_PageUp},
    {      "PageDown",       CSimpleMessageScrollFrame_PageDown},
    {   "ScrollToTop",    CSimpleMessageScrollFrame_ScrollToTop},
    {"ScrollToBottom", CSimpleMessageScrollFrame_ScrollToBottom},
    {      "AtBottom",       CSimpleMessageScrollFrame_AtBottom}
};

TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> CSimpleMessageScrollFrame::s_scriptMethods;

void CSimpleMessageScrollFrame::RegisterScriptMethods() {
  FrameScript_Object::FillScriptMethodTable(SimpleMessageScrollFrameMethods, 8, s_scriptMethods);
}

void CSimpleMessageScrollFrame::UnregisterScriptMethods() {
  FrameScript_Object::EmptyScriptMethodTable(s_scriptMethods);
}

int CSimpleMessageScrollFrame::LookupScriptMethod(lua_State *L, const char *name) {
  if (FrameScript_Object::LookupScriptMethod(L, name, s_scriptMethods)) {
    return 1;
  }

  return CSimpleFrame::LookupScriptMethod(L, name);
}
