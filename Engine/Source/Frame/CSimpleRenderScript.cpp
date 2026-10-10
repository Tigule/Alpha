#include <Base/Base.h>

#include "Frame/CSimpleTop.h"
#include "Frame/CSimpleRender.h"

#include "FrameXML/LoadXML.h"

#include <lauxlib.h>
#include <lua.h>
#include <storm.h>

static int CSimpleTexture_GetAlpha(lua_State *L) {
  CSimpleTexture *object = (CSimpleTexture *)FrameScript_GetObjectThis(L);

  NTempest::CImVector color;
  object->GetVertexColor(color);
  lua_pushnumber(L, (double)color.a / 255.0);
  return 1;
}

static int CSimpleTexture_GetName(lua_State *L) {
  CSimpleTexture *object = (CSimpleTexture *)FrameScript_GetObjectThis(L);
  LPCSTR          name = object->GetName();
  if (name && *name) {
    lua_pushstring(L, name);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int CSimpleTexture_SetVertexColor(lua_State *L) {
  CSimpleTexture     *object = (CSimpleTexture *)FrameScript_GetObjectThis(L);
  NTempest::CImVector color;
  float               red = lua_tonumber(L, 2);
  float               green = lua_tonumber(L, 3);
  float               blue = lua_tonumber(L, 4);
  float               alpha = 1.0f;
  if (lua_isnumber(L, 5)) {
    alpha = lua_tonumber(L, 5);
  }
  color.Set(alpha, red, green, blue);
  object->SetVertexColor(color);
  return 0;
}

static int CSimpleTexture_SetAlpha(lua_State *L) {
  CSimpleTexture *object = (CSimpleTexture *)FrameScript_GetObjectThis(L);
  if (lua_isnumber(L, 2)) {
    NTempest::CImVector color;
    object->GetVertexColor(color);
    color.a = lua_tonumber(L, 2) * 255.0;
    object->SetVertexColor(color);
    return 0;
  }
  luaL_error(L, "Usage: SetAlpha(alpha)");
  return 0;
}

static int CSimpleTexture_Show(lua_State *L) {
  CSimpleTexture *object = (CSimpleTexture *)FrameScript_GetObjectThis(L);
  object->Show();
  return 0;
}

static int CSimpleTexture_Hide(lua_State *L) {
  CSimpleTexture *object = (CSimpleTexture *)FrameScript_GetObjectThis(L);
  object->Hide();
  return 0;
}

static int CSimpleTexture_IsVisible(lua_State *L) {
  CSimpleTexture *object = (CSimpleTexture *)FrameScript_GetObjectThis(L);
  if (object->IsVisible()) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int CSimpleTexture_SetTexture(lua_State *L) {
  CSimpleTexture *object = (CSimpleTexture *)FrameScript_GetObjectThis(L);

  if (lua_isstring(L, 2)) {
    object->SetTexture(lua_tostring(L, 2), 0);
  } else {
    NTempest::CImVector color;
    float               red = lua_tonumber(L, 2);
    float               green = lua_tonumber(L, 3);
    float               blue = lua_tonumber(L, 4);
    float               alpha = 1.0f;

    if (lua_isnumber(L, 5)) {
      alpha = lua_tonumber(L, 5);
    }

    color.Set(alpha, red, green, blue);
    object->SetTexture(color);
  }

  return 0;
}

static int CSimpleTexture_SetTexCoord(lua_State *L) {
  CSimpleTexture *object = (CSimpleTexture *)FrameScript_GetObjectThis(L);

  NTempest::CRect rect;
  rect.l = lua_tonumber(L, 2);
  rect.r = lua_tonumber(L, 3);
  rect.t = lua_tonumber(L, 4);
  rect.b = lua_tonumber(L, 5);
  object->SetTexCoord(rect);
  return 0;
}

static int CSimpleTexture_SetPoint(lua_State *L) {
  CSimpleTexture *object = (CSimpleTexture *)FrameScript_GetObjectThis(L);
  if (lua_isstring(L, 2) && lua_isstring(L, 3)) {
    char          message[128];
    CLayoutFrame *relativeFrame;
    float         offsetY = 0.0f;
    float         offsetX = 0.0f;
    FRAMEPOINT    point;
    FRAMEPOINT    relativePoint;
    if (!StringToFramePoint(lua_tostring(L, 2), point)) {
      luaL_error(L, "Unknown frame point");
      return 0;
    }
    relativePoint = point;
    LPCSTR relativeName = lua_tostring(L, 3);
    relativeFrame = object->GetLayoutFrameByName(relativeName);
    if (!relativeFrame) {
      SStrPrintf(message, sizeof(message), "Couldn't find frame named '%s'", relativeName);
      luaL_error(L, message);
      return 0;
    }
    if (lua_isstring(L, 4)) {
      if (!StringToFramePoint(lua_tostring(L, 4), relativePoint)) {
        luaL_error(L, "Unknown frame point");
        return 0;
      }
      if (lua_isnumber(L, 5) && lua_isnumber(L, 6)) {
        offsetX = 0.8f * (lua_tonumber(L, 5) * 0.0009765625f);
        offsetY = 0.8f * (lua_tonumber(L, 6) * 0.0009765625f);
      }
    }
    object->SetPoint(point, relativeFrame, relativePoint, offsetX, offsetY, 1);
    return 0;
  }
  luaL_error(
      L,
      "Usage: SetPoint(\"point\" \"frame\" [, relativePoint] "
      "[, offsetX, offsetY])"
  );
  return 0;
}

static int CSimpleTexture_ClearAllPoints(lua_State *L) {
  CSimpleTexture *object = (CSimpleTexture *)FrameScript_GetObjectThis(L);
  object->ClearAllPoints(1);
  return 0;
}

static int CSimpleTexture_GetWidth(lua_State *L) {
  CSimpleTexture *object = (CSimpleTexture *)FrameScript_GetObjectThis(L);

  float width = object->GetWidth();
  if (width == 0.0f) {
    NTempest::CRect rect;
    object->GetRect(&rect);
    width = rect.r - rect.l;
  }

  lua_pushnumber(L, 1.25f * (width * 1024.0f));
  return 1;
}

static int CSimpleTexture_SetWidth(lua_State *L) {
  CSimpleTexture *object = (CSimpleTexture *)FrameScript_GetObjectThis(L);

  if (lua_isnumber(L, 2)) {
    object->SetWidth(0.8f * (lua_tonumber(L, 2) * 0.0009765625f));
    return 0;
  }

  luaL_error(L, "Usage: SetWidth(width)");
  return 0;
}

static int CSimpleTexture_GetHeight(lua_State *L) {
  CSimpleTexture *object = (CSimpleTexture *)FrameScript_GetObjectThis(L);

  float height = object->GetHeight();
  if (height == 0.0f) {
    NTempest::CRect rect;
    object->GetRect(&rect);
    height = rect.b - rect.t;
  }

  lua_pushnumber(L, 1.25f * (height * 1024.0f));
  return 1;
}

static int CSimpleTexture_SetHeight(lua_State *L) {
  CSimpleTexture *object = (CSimpleTexture *)FrameScript_GetObjectThis(L);

  if (lua_isnumber(L, 2)) {
    object->SetHeight(0.8f * (lua_tonumber(L, 2) * 0.0009765625f));
    return 0;
  }

  luaL_error(L, "Usage: SetHeight(height)");
  return 0;
}

static FrameScript_Method SimpleTextureMethods[] = {
    {      "GetAlpha",       CSimpleTexture_GetAlpha},
    {       "GetName",        CSimpleTexture_GetName},
    {"SetVertexColor", CSimpleTexture_SetVertexColor},
    {      "SetAlpha",       CSimpleTexture_SetAlpha},
    {          "Show",           CSimpleTexture_Show},
    {          "Hide",           CSimpleTexture_Hide},
    {     "IsVisible",      CSimpleTexture_IsVisible},
    {    "SetTexture",     CSimpleTexture_SetTexture},
    {   "SetTexCoord",    CSimpleTexture_SetTexCoord},
    {      "SetPoint",       CSimpleTexture_SetPoint},
    {"ClearAllPoints", CSimpleTexture_ClearAllPoints},
    {      "GetWidth",       CSimpleTexture_GetWidth},
    {      "SetWidth",       CSimpleTexture_SetWidth},
    {     "GetHeight",      CSimpleTexture_GetHeight},
    {     "SetHeight",      CSimpleTexture_SetHeight}
};

TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> CSimpleTexture::s_scriptMethods;

static int CSimpleFontString_SetAlphaGradient(lua_State *L) {
  CSimpleFontString *object = (CSimpleFontString *)FrameScript_GetObjectThis(L);

  if (lua_isnumber(L, 2) && lua_isnumber(L, 3)) {
    int start = lua_tonumber(L, 2);
    int length = lua_tonumber(L, 3);
    if (object->SetAlphaGradient(start, length)) {
      lua_pushnumber(L, 1.0);
    } else {
      lua_pushnil(L);
    }

    return 1;
  }

  luaL_error(L, "Usage: SetAlphaGradient(start, length)");
  return 0;
}

static int CSimpleFontString_SetText(lua_State *L) {
  CSimpleFontString *object = (CSimpleFontString *)FrameScript_GetObjectThis(L);

  object->SetText(lua_tostring(L, 2));
  return 0;
}

static int CSimpleFontString_GetText(lua_State *L) {
  CSimpleFontString *object = (CSimpleFontString *)FrameScript_GetObjectThis(L);

  lua_pushstring(L, object->GetText());
  return 1;
}

static int CSimpleFontString_GetName(lua_State *L) {
  CSimpleFontString *object = (CSimpleFontString *)FrameScript_GetObjectThis(L);
  LPCSTR             name = object->GetName();
  if (name && *name) {
    lua_pushstring(L, name);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int CSimpleFontString_SetVertexColor(lua_State *L) {
  CSimpleFontString  *object = (CSimpleFontString *)FrameScript_GetObjectThis(L);
  NTempest::CImVector color;
  float               red = lua_tonumber(L, 2);
  float               green = lua_tonumber(L, 3);
  float               blue = lua_tonumber(L, 4);
  float               alpha = 1.0f;
  if (lua_isnumber(L, 5)) {
    alpha = lua_tonumber(L, 5);
  }
  color.Set(alpha, red, green, blue);
  object->SetVertexColor(color);
  return 0;
}

static int CSimpleFontString_SetAlpha(lua_State *L) {
  CSimpleFontString *object = (CSimpleFontString *)FrameScript_GetObjectThis(L);
  if (lua_isnumber(L, 2)) {
    NTempest::CImVector color;
    object->GetVertexColor(color);
    color.a = lua_tonumber(L, 2) * 255.0;
    object->SetVertexColor(color);
    return 0;
  }
  luaL_error(L, "Usage: SetAlpha(alpha)");
  return 0;
}

static int CSimpleFontString_SetTextHeight(lua_State *L) {
  CSimpleFontString *object = (CSimpleFontString *)FrameScript_GetObjectThis(L);

  if (lua_isnumber(L, 2)) {
    object->SetTextHeight(0.8f * ((float)lua_tonumber(L, 2) * 0.0009765625f));
    return 0;
  }

  luaL_error(L, "Usage: SetTextHeight(pixelHeight)");
  return 0;
}

static int CSimpleFontString_Show(lua_State *L) {
  CSimpleFontString *object = (CSimpleFontString *)FrameScript_GetObjectThis(L);
  object->Show();
  return 0;
}

static int CSimpleFontString_Hide(lua_State *L) {
  CSimpleFontString *object = (CSimpleFontString *)FrameScript_GetObjectThis(L);
  object->Hide();
  return 0;
}

static int CSimpleFontString_IsVisible(lua_State *L) {
  CSimpleFontString *object = (CSimpleFontString *)FrameScript_GetObjectThis(L);
  if (object->IsVisible()) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int CSimpleFontString_SetWidth(lua_State *L) {
  CSimpleFontString *object = (CSimpleFontString *)FrameScript_GetObjectThis(L);

  if (lua_isnumber(L, 2)) {
    object->SetWidth(0.8f * ((float)lua_tonumber(L, 2) * 0.0009765625f));
    return 0;
  }

  luaL_error(L, "Usage: SetWidth(pixelWidth)");
  return 0;
}

static int CSimpleFontString_GetWidth(lua_State *L) {
  CSimpleFontString *object = (CSimpleFontString *)FrameScript_GetObjectThis(L);

  lua_pushnumber(L, 1.25f * (object->GetWidth() * 1024.0f));
  return 1;
}

static int CSimpleFontString_SetTextColor(lua_State *L) {
  CSimpleFontString  *object = (CSimpleFontString *)FrameScript_GetObjectThis(L);
  NTempest::CImVector color;
  float               red = lua_tonumber(L, 2);
  float               green = lua_tonumber(L, 3);
  float               blue = lua_tonumber(L, 4);
  float               alpha = 1.0f;
  if (lua_isnumber(L, 5)) {
    alpha = lua_tonumber(L, 5);
  }
  color.Set(alpha, red, green, blue);
  object->SetVertexColor(color);
  return 0;
}

static int CSimpleFontString_SetHeight(lua_State *L) {
  CSimpleFontString *object = (CSimpleFontString *)FrameScript_GetObjectThis(L);

  if (lua_isnumber(L, 2)) {
    object->SetHeight(0.8f * ((float)lua_tonumber(L, 2) * 0.0009765625f));
    return 0;
  }

  luaL_error(L, "Usage: SetHeight(pixelHeight)");
  return 0;
}

static int CSimpleFontString_GetHeight(lua_State *L) {
  CSimpleFontString *object = (CSimpleFontString *)FrameScript_GetObjectThis(L);

  lua_pushnumber(L, 1.25f * (object->GetHeight() * 1024.0f));
  return 1;
}

static int CSimpleFontString_SetJustifyH(lua_State *L) {
  CSimpleFontString *object = (CSimpleFontString *)FrameScript_GetObjectThis(L);

  UINT flag;
  if (lua_isstring(L, 2) && StringToJustify(lua_tostring(L, 2), flag)) {
    object->SetHorizontalAlignment(flag);
    return 0;
  }

  luaL_error(L, "Usage(SetJustifyH(\"justify\")");
  return 0;
}

static int CSimpleFontString_SetJustifyV(lua_State *L) {
  CSimpleFontString *object = (CSimpleFontString *)FrameScript_GetObjectThis(L);

  UINT flag;
  if (lua_isstring(L, 2) && StringToJustify(lua_tostring(L, 2), flag)) {
    object->SetVerticalAlignment(flag);
    return 0;
  }

  luaL_error(L, "Usage(SetJustifyV(\"justify\")");
  return 0;
}

void CSimpleTexture::RegisterScriptMethods() {
  FrameScript_Object::FillScriptMethodTable(SimpleTextureMethods, sizeof(SimpleTextureMethods) / sizeof(SimpleTextureMethods[0]), s_scriptMethods);
}

static int CSimpleFontString_SetPoint(lua_State *L) {
  CSimpleFontString *object = (CSimpleFontString *)FrameScript_GetObjectThis(L);
  if (lua_isstring(L, 2) && lua_isstring(L, 3)) {
    char          message[128];
    CLayoutFrame *relativeFrame;
    float         offsetY = 0.0f;
    float         offsetX = 0.0f;
    FRAMEPOINT    point;
    FRAMEPOINT    relativePoint;
    if (!StringToFramePoint(lua_tostring(L, 2), point)) {
      luaL_error(L, "Unknown frame point");
      return 0;
    }
    relativePoint = point;
    LPCSTR relativeName = lua_tostring(L, 3);
    relativeFrame = object->GetLayoutFrameByName(relativeName);
    if (!relativeFrame) {
      SStrPrintf(message, sizeof(message), "Couldn't find frame named '%s'", relativeName);
      luaL_error(L, message);
      return 0;
    }
    if (lua_isstring(L, 4)) {
      if (!StringToFramePoint(lua_tostring(L, 4), relativePoint)) {
        luaL_error(L, "Unknown frame point");
        return 0;
      }
      if (lua_isnumber(L, 5) && lua_isnumber(L, 6)) {
        offsetX = 0.8f * (lua_tonumber(L, 5) * 0.0009765625f);
        offsetY = 0.8f * (lua_tonumber(L, 6) * 0.0009765625f);
      }
    }
    object->SetPoint(point, relativeFrame, relativePoint, offsetX, offsetY, 1);
    return 0;
  }
  luaL_error(
      L,
      "Usage: SetPoint(\"point\" \"frame\" [, relativePoint] "
      "[, offsetX, offsetY])"
  );
  return 0;
}

static int CSimpleFontString_ClearAllPoints(lua_State *L) {
  CSimpleFontString *object = (CSimpleFontString *)FrameScript_GetObjectThis(L);
  object->ClearAllPoints(1);
  return 0;
}

void CSimpleTexture::UnregisterScriptMethods() {
  FrameScript_Object::EmptyScriptMethodTable(s_scriptMethods);
}

BOOL CSimpleTexture::LookupScriptMethod(lua_State *L, LPCSTR name) {
  return FrameScript_Object::LookupScriptMethod(L, name, s_scriptMethods);
}

static FrameScript_Method SimpleFontStringMethods[] = {
    {         "GetName",          CSimpleFontString_GetName},
    {  "SetVertexColor",   CSimpleFontString_SetVertexColor},
    {        "SetAlpha",         CSimpleFontString_SetAlpha},
    {"SetAlphaGradient", CSimpleFontString_SetAlphaGradient},
    {            "Show",             CSimpleFontString_Show},
    {            "Hide",             CSimpleFontString_Hide},
    {       "IsVisible",        CSimpleFontString_IsVisible},
    {         "SetText",          CSimpleFontString_SetText},
    {         "GetText",          CSimpleFontString_GetText},
    {    "SetTextColor",     CSimpleFontString_SetTextColor},
    {   "SetTextHeight",    CSimpleFontString_SetTextHeight},
    {        "SetWidth",         CSimpleFontString_SetWidth},
    {        "GetWidth",         CSimpleFontString_GetWidth},
    {       "SetHeight",        CSimpleFontString_SetHeight},
    {       "GetHeight",        CSimpleFontString_GetHeight},
    {        "SetPoint",         CSimpleFontString_SetPoint},
    {  "ClearAllPoints",   CSimpleFontString_ClearAllPoints},
    {     "SetJustifyH",      CSimpleFontString_SetJustifyH},
    {     "SetJustifyV",      CSimpleFontString_SetJustifyV}
};

TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> CSimpleFontString::s_scriptMethods;

void CSimpleFontString::RegisterScriptMethods() {
  FrameScript_Object::FillScriptMethodTable(
      SimpleFontStringMethods, sizeof(SimpleFontStringMethods) / sizeof(SimpleFontStringMethods[0]), s_scriptMethods
  );
}

void CSimpleFontString::UnregisterScriptMethods() {
  FrameScript_Object::EmptyScriptMethodTable(s_scriptMethods);
}

BOOL CSimpleFontString::LookupScriptMethod(lua_State *L, LPCSTR name) {
  return FrameScript_Object::LookupScriptMethod(L, name, s_scriptMethods);
}
