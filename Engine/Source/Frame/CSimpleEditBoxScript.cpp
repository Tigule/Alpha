#include "Frame/CSimpleEditBox.h"

#include <lauxlib.h>
#include <lua.h>

#define GET_SIMPLE_EDITBOX_THIS(L, object)                         \
  CSimpleEditBox *object = 0;                                      \
  if (lua_type(L, 1) == LUA_TTABLE) {                              \
    lua_rawgeti(L, 1, 0);                                          \
    object = static_cast<CSimpleEditBox *>(lua_touserdata(L, -1)); \
    lua_pop(L, 1);                                                 \
  } else {                                                         \
    luaL_error(                                                    \
        L,                                                         \
        "Attempt to find 'this' in non-table object (used '.' "    \
        "instead of ':' ?)"                                        \
    );                                                             \
  }                                                                \
  ASSERT(object)

static int CSimpleEditBox_Insert(lua_State *L) {
  GET_SIMPLE_EDITBOX_THIS(L, object);

  if (lua_isstring(L, 2)) {
    object->Insert(lua_tostring(L, 2), 0);
  }

  return 0;
}

static int CSimpleEditBox_SetText(lua_State *L) {
  GET_SIMPLE_EDITBOX_THIS(L, object);

  if (lua_gettop(L) != 2) {
    luaL_error(L, "Usage: SetText(\"text\")");
  }

  object->SetText(lua_tostring(L, 2));
  return 0;
}

static int CSimpleEditBox_GetText(lua_State *L) {
  GET_SIMPLE_EDITBOX_THIS(L, object);

  lua_pushstring(L, object->GetText());
  return 1;
}

static int CSimpleEditBox_AddHistoryLine(lua_State *L) {
  GET_SIMPLE_EDITBOX_THIS(L, object);

  if (lua_gettop(L) != 2) {
    luaL_error(L, "Usage: AddHistoryLine(\"text\")");
  }

  object->AddHistoryLine(lua_tostring(L, 2));
  return 0;
}

static int CSimpleEditBox_SetTextInsets(lua_State *L) {
  GET_SIMPLE_EDITBOX_THIS(L, object);

  if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4) || !lua_isnumber(L, 5)) {
    luaL_error(L, "Usage: SetTextInsets(l, r, t, b)");
  }

  object->SetEditTextInsets(
      static_cast<float>(lua_tonumber(L, 3) * 0.0009765625f * 0.8f), static_cast<float>(lua_tonumber(L, 2) * 0.0009765625f * 0.8f),
      static_cast<float>(lua_tonumber(L, 4) * 0.0009765625f * 0.8f), static_cast<float>(lua_tonumber(L, 5) * 0.0009765625f * 0.8f)
  );
  return 0;
}

static int CSimpleEditBox_SetTextColor(lua_State *L) {
  GET_SIMPLE_EDITBOX_THIS(L, object);

  NTempest::CImVector color;
  float               red = static_cast<float>(lua_tonumber(L, 2));
  float               green = static_cast<float>(lua_tonumber(L, 3));
  float               blue = static_cast<float>(lua_tonumber(L, 4));
  float               alpha = 1.0f;
  if (lua_isnumber(L, 5)) {
    alpha = static_cast<float>(lua_tonumber(L, 5));
  }

  color.Set(alpha, red, green, blue);
  object->SetCursorColor(color);
  object->SetTextColor(color);
  return 0;
}

static int CSimpleEditBox_SetFocus(lua_State *L) {
  GET_SIMPLE_EDITBOX_THIS(L, object);

  CSimpleEditBox::SetKeyboardFocus(object);
  return 0;
}

static int CSimpleEditBox_ClearFocus(lua_State *L) {
  GET_SIMPLE_EDITBOX_THIS(L, object);

  CSimpleEditBox::ClearKeyboardFocus(object);
  return 0;
}

#undef GET_SIMPLE_EDITBOX_THIS

static FrameScript_Method SimpleEditBoxMethods[8] = {
    {        "Insert",         CSimpleEditBox_Insert},
    {       "SetText",        CSimpleEditBox_SetText},
    {       "GetText",        CSimpleEditBox_GetText},
    {"AddHistoryLine", CSimpleEditBox_AddHistoryLine},
    { "SetTextInsets",  CSimpleEditBox_SetTextInsets},
    {  "SetTextColor",   CSimpleEditBox_SetTextColor},
    {      "SetFocus",       CSimpleEditBox_SetFocus},
    {    "ClearFocus",     CSimpleEditBox_ClearFocus}
};

TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> CSimpleEditBox::s_scriptMethods;

void CSimpleEditBox::RegisterScriptMethods() {
  FrameScript_Object::FillScriptMethodTable(SimpleEditBoxMethods, 8, s_scriptMethods);
}

void CSimpleEditBox::UnregisterScriptMethods() {
  FrameScript_Object::EmptyScriptMethodTable(s_scriptMethods);
}

int CSimpleEditBox::LookupScriptMethod(lua_State *L, const char *name) {
  if (FrameScript_Object::LookupScriptMethod(L, name, s_scriptMethods)) {
    return 1;
  }

  return CSimpleFrame::LookupScriptMethod(L, name);
}
