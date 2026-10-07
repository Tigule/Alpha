#include <Base/Base.h>

#include "Frame/CSimpleTop.h"
#include "Frame/CSimpleEditBox.h"

#include <lauxlib.h>
#include <lua.h>

static int CSimpleEditBox_Insert(lua_State *L) {
  CSimpleEditBox *object = static_cast<CSimpleEditBox *>(FrameScript_GetObjectThis(L));

  if (lua_isstring(L, 2)) {
    object->Insert(lua_tostring(L, 2), 0);
  }

  return 0;
}

static int CSimpleEditBox_SetText(lua_State *L) {
  CSimpleEditBox *object = static_cast<CSimpleEditBox *>(FrameScript_GetObjectThis(L));

  if (lua_gettop(L) != 2) {
    luaL_error(L, "Usage: SetText(\"text\")");
    return 0;
  }

  object->SetText(lua_tostring(L, 2));
  return 0;
}

static int CSimpleEditBox_GetText(lua_State *L) {
  CSimpleEditBox *object = static_cast<CSimpleEditBox *>(FrameScript_GetObjectThis(L));

  lua_pushstring(L, object->GetText());
  return 1;
}

static int CSimpleEditBox_AddHistoryLine(lua_State *L) {
  CSimpleEditBox *object = static_cast<CSimpleEditBox *>(FrameScript_GetObjectThis(L));

  if (lua_gettop(L) != 2) {
    luaL_error(L, "Usage: AddHistoryLine(\"text\")");
    return 0;
  }

  object->AddHistoryLine(lua_tostring(L, 2));
  return 0;
}

static int CSimpleEditBox_SetTextInsets(lua_State *L) {
  CSimpleEditBox *object = static_cast<CSimpleEditBox *>(FrameScript_GetObjectThis(L));

  if (lua_isnumber(L, 2) && lua_isnumber(L, 3) && lua_isnumber(L, 4) && lua_isnumber(L, 5)) {
    object->SetEditTextInsets(
        static_cast<float>(0.8f * (lua_tonumber(L, 3) * 0.0009765625f)), static_cast<float>(0.8f * (lua_tonumber(L, 2) * 0.0009765625f)),
        static_cast<float>(0.8f * (lua_tonumber(L, 4) * 0.0009765625f)), static_cast<float>(0.8f * (lua_tonumber(L, 5) * 0.0009765625f))
    );
    return 0;
  }

  luaL_error(L, "Usage: SetTextInsets(l, r, t, b)");
  return 0;
}

static int CSimpleEditBox_SetTextColor(lua_State *L) {
  CSimpleEditBox *object = static_cast<CSimpleEditBox *>(FrameScript_GetObjectThis(L));

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
  CSimpleEditBox *object = static_cast<CSimpleEditBox *>(FrameScript_GetObjectThis(L));

  CSimpleEditBox::SetKeyboardFocus(object);
  return 0;
}

static int CSimpleEditBox_ClearFocus(lua_State *L) {
  CSimpleEditBox *object = static_cast<CSimpleEditBox *>(FrameScript_GetObjectThis(L));

  CSimpleEditBox::ClearKeyboardFocus(object);
  return 0;
}

static FrameScript_Method SimpleEditBoxMethods[8] = {
    {        "Insert",         CSimpleEditBox_Insert},
    {       "SetText",        CSimpleEditBox_SetText},
    {       "GetText",        CSimpleEditBox_GetText},
    {  "SetTextColor",   CSimpleEditBox_SetTextColor},
    {"AddHistoryLine", CSimpleEditBox_AddHistoryLine},
    { "SetTextInsets",  CSimpleEditBox_SetTextInsets},
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

BOOL CSimpleEditBox::LookupScriptMethod(lua_State *L, LPCSTR name) {
  if (FrameScript_Object::LookupScriptMethod(L, name, s_scriptMethods)) {
    return 1;
  }

  return CSimpleFrame::LookupScriptMethod(L, name);
}
