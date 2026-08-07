#include <Base/Base.h>

#include "Frame/CSimpleHTML.h"

#include <lauxlib.h>
#include <lua.h>

#define GET_SIMPLE_HTML_THIS(L, object)                                 \
  CSimpleHTML *object = 0;                                              \
  if (lua_type(L, 1) == LUA_TTABLE) {                                   \
    lua_rawgeti(L, 1, 0);                                               \
    object = static_cast<CSimpleHTML *>(lua_touserdata(L, -1));         \
    lua_pop(L, 1);                                                      \
  } else {                                                              \
    luaL_error(                                                         \
        L,                                                              \
        "Attempt to find 'this' in non-table object (used '.' instead " \
        "of ':' ?)"                                                     \
    );                                                                  \
  }                                                                     \
  ASSERT(object)

int CSimpleHTML_SetText(lua_State *L) {
  GET_SIMPLE_HTML_THIS(L, object);

  object->SetText(lua_tostring(L, 2), 0);
  return 0;
}

int CSimpleHTML_SetTextColor(lua_State *L) {
  GET_SIMPLE_HTML_THIS(L, object);

  float red = static_cast<float>(lua_tonumber(L, 2));
  float green = static_cast<float>(lua_tonumber(L, 3));
  float blue = static_cast<float>(lua_tonumber(L, 4));
  float alpha = 1.0f;
  if (lua_isnumber(L, 5)) {
    alpha = static_cast<float>(lua_tonumber(L, 5));
  }

  NTempest::CImVector color;
  color.Set(alpha, red, green, blue);

  for (UINT i = 0; i < NUM_HTML_TEXT_TYPES; ++i) {
    CSimpleFontStringAttributes attrib(object->m_attrib[i]);
    attrib.SetColor(color);
    object->m_attrib[i] = attrib;
  }

  return 0;
}

static FrameScript_Method SimpleHTMLMethods[2] = {
    {     "SetText",      CSimpleHTML_SetText},
    {"SetTextColor", CSimpleHTML_SetTextColor}
};

TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> CSimpleHTML::s_scriptMethods;

void CSimpleHTML::RegisterScriptMethods() {
  FrameScript_Object::FillScriptMethodTable(SimpleHTMLMethods, 2, s_scriptMethods);
}

void CSimpleHTML::UnregisterScriptMethods() {
  FrameScript_Object::EmptyScriptMethodTable(s_scriptMethods);
}

BOOL CSimpleHTML::LookupScriptMethod(lua_State *L, LPCSTR name) {
  if (FrameScript_Object::LookupScriptMethod(L, name, s_scriptMethods)) {
    return 1;
  }

  return CSimpleFrame::LookupScriptMethod(L, name);
}
