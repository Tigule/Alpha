#include <Base/Base.h>

#include "Frame/CSimpleTop.h"
#include "Frame/CSimpleHTML.h"

#include <lauxlib.h>
#include <lua.h>

static int CSimpleHTML_SetText(lua_State *L) {
  CSimpleHTML *object = (CSimpleHTML *)FrameScript_GetObjectThis(L);

  object->SetText(lua_tostring(L, 2), 0);
  return 0;
}

static int CSimpleHTML_SetTextColor(lua_State *L) {
  CSimpleHTML *object = (CSimpleHTML *)FrameScript_GetObjectThis(L);

  float red = lua_tonumber(L, 2);
  float green = lua_tonumber(L, 3);
  float blue = lua_tonumber(L, 4);
  float alpha = 1.0f;
  if (lua_isnumber(L, 5)) {
    alpha = lua_tonumber(L, 5);
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
