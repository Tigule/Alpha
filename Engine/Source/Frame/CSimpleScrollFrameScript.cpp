#include <Base/Base.h>

#include "Frame/CSimpleTop.h"
#include "Frame/CSimpleScrollFrame.h"

#include <lauxlib.h>
#include <lua.h>

static int CSimpleScrollFrame_SetHorizontalScroll(lua_State *L) {
  CSimpleScrollFrame *object = (CSimpleScrollFrame *)FrameScript_GetObjectThis(L);

  if (lua_isnumber(L, 2)) {
    float offset = 0.8f * ((float)lua_tonumber(L, 2) * 0.0009765625f);
    object->SetHorizontalScroll(offset);
    return 0;
  }

  luaL_error(L, "Usage: SetHorizontalScroll(offset)");
  return 0;
}

static int CSimpleScrollFrame_SetVerticalScroll(lua_State *L) {
  CSimpleScrollFrame *object = (CSimpleScrollFrame *)FrameScript_GetObjectThis(L);

  if (lua_isnumber(L, 2)) {
    float offset = 0.8f * ((float)lua_tonumber(L, 2) * 0.0009765625f);
    object->SetVerticalScroll(offset);
    return 0;
  }

  luaL_error(L, "Usage: SetVerticalScroll(offset)");
  return 0;
}

static int CSimpleScrollFrame_GetHorizontalScroll(lua_State *L) {
  CSimpleScrollFrame *object = (CSimpleScrollFrame *)FrameScript_GetObjectThis(L);

  lua_pushnumber(L, 1.25f * (object->GetHorizontalScroll() * 1024.0f));
  return 1;
}

static int CSimpleScrollFrame_GetVerticalScroll(lua_State *L) {
  CSimpleScrollFrame *object = (CSimpleScrollFrame *)FrameScript_GetObjectThis(L);

  lua_pushnumber(L, 1.25f * (object->GetVerticalScroll() * 1024.0f));
  return 1;
}

static int CSimpleScrollFrame_GetHorizontalScrollRange(lua_State *L) {
  CSimpleScrollFrame *object = (CSimpleScrollFrame *)FrameScript_GetObjectThis(L);

  lua_pushnumber(L, 1.25f * (object->GetHorizontalScrollRange() * 1024.0f));
  return 1;
}

static int CSimpleScrollFrame_GetVerticalScrollRange(lua_State *L) {
  CSimpleScrollFrame *object = (CSimpleScrollFrame *)FrameScript_GetObjectThis(L);

  lua_pushnumber(L, 1.25f * (object->GetVerticalScrollRange() * 1024.0f));
  return 1;
}

static int CSimpleScrollFrame_UpdateScrollChildRect(lua_State *L) {
  CSimpleScrollFrame *object = (CSimpleScrollFrame *)FrameScript_GetObjectThis(L);

  object->UpdateScrollChildRect();
  return 0;
}

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
