#include "Frame/CSimpleRender.h"

#include "FrameXML/LoadXML.h"

#include <lauxlib.h>
#include <lua.h>
#include <storm.h>

#define GET_SIMPLE_RENDER_THIS(L, TYPE, object)                         \
  TYPE *object = 0;                                                     \
  if (lua_type(L, 1) == LUA_TTABLE) {                                   \
    lua_rawgeti(L, 1, 0);                                               \
    object = static_cast<TYPE *>(lua_touserdata(L, -1));                \
    lua_pop(L, 1);                                                      \
  } else {                                                              \
    luaL_error(                                                         \
        L,                                                              \
        "Attempt to find 'this' in non-table object (used '.' instead " \
        "of ':' ?)"                                                     \
    );                                                                  \
  }                                                                     \
  ASSERT(object)

#define DEFINE_RENDER_GET_NAME(TYPE, FUNCTION) \
  int FUNCTION(lua_State *L) {      \
    GET_SIMPLE_RENDER_THIS(L, TYPE, object);   \
    const char *name = object->GetName();      \
    if (name && *name) {                       \
      lua_pushstring(L, name);                 \
    } else {                                   \
      lua_pushnil(L);                          \
    }                                          \
    return 1;                                  \
  }

#define DEFINE_RENDER_SET_VERTEX_COLOR(TYPE, FUNCTION)                  \
  int FUNCTION(lua_State *L) {                               \
    GET_SIMPLE_RENDER_THIS(L, TYPE, object);                            \
    NTempest::CImVector color;                                          \
    float               red = static_cast<float>(lua_tonumber(L, 2));   \
    float               green = static_cast<float>(lua_tonumber(L, 3)); \
    float               blue = static_cast<float>(lua_tonumber(L, 4));  \
    float               alpha = 1.0f;                                   \
    if (lua_isnumber(L, 5)) {                                           \
      alpha = static_cast<float>(lua_tonumber(L, 5));                   \
    }                                                                   \
    color.Set(alpha, red, green, blue);                                 \
    object->SetVertexColor(color);                                      \
    return 0;                                                           \
  }

#define DEFINE_RENDER_SET_ALPHA(TYPE, FUNCTION)                       \
  int FUNCTION(lua_State *L) {                             \
    GET_SIMPLE_RENDER_THIS(L, TYPE, object);                          \
    if (!lua_isnumber(L, 2)) {                                        \
      luaL_error(L, "Usage: SetAlpha(alpha)");                        \
    }                                                                 \
    NTempest::CImVector color;                                        \
    object->GetVertexColor(color);                                    \
    color.a = static_cast<unsigned char>(lua_tonumber(L, 2) * 255.0); \
    object->SetVertexColor(color);                                    \
    return 0;                                                         \
  }

#define DEFINE_RENDER_SHOW(TYPE, FUNCTION)   \
  int FUNCTION(lua_State *L) {    \
    GET_SIMPLE_RENDER_THIS(L, TYPE, object); \
    object->Show();                          \
    return 0;                                \
  }

#define DEFINE_RENDER_HIDE(TYPE, FUNCTION)   \
  int FUNCTION(lua_State *L) {    \
    GET_SIMPLE_RENDER_THIS(L, TYPE, object); \
    object->Hide();                          \
    return 0;                                \
  }

#define DEFINE_RENDER_IS_VISIBLE(TYPE, FUNCTION) \
  int FUNCTION(lua_State *L) {        \
    GET_SIMPLE_RENDER_THIS(L, TYPE, object);     \
    if (object->IsVisible()) {                   \
      lua_pushnumber(L, 1.0);                    \
    } else {                                     \
      lua_pushnil(L);                            \
    }                                            \
    return 1;                                    \
  }

#define DEFINE_RENDER_SET_POINT(TYPE, FUNCTION)                                             \
  int FUNCTION(lua_State *L) {                                                   \
    GET_SIMPLE_RENDER_THIS(L, TYPE, object);                                                \
    if (!lua_isstring(L, 2) || !lua_isstring(L, 3)) {                                       \
      luaL_error(                                                                           \
          L,                                                                                \
          "Usage: SetPoint(\"point\" \"frame\" [, relativePoint] "                          \
          "[, offsetX, offsetY])"                                                           \
      );                                                                                    \
    }                                                                                       \
    char          message[128];                                                             \
    CLayoutFrame *relativeFrame;                                                            \
    float         offsetY = 0.0f;                                                           \
    float         offsetX = 0.0f;                                                           \
    FRAMEPOINT    point;                                                                    \
    FRAMEPOINT    relativePoint;                                                            \
    if (!StringToFramePoint(lua_tostring(L, 2), point)) {                                   \
      luaL_error(L, "Unknown frame point");                                                 \
    }                                                                                       \
    relativePoint = point;                                                                  \
    const char *relativeName = lua_tostring(L, 3);                                          \
    relativeFrame = object->GetLayoutFrameByName(relativeName);                             \
    if (!relativeFrame) {                                                                   \
      SStrPrintf(message, sizeof(message), "Couldn't find frame named '%s'", relativeName); \
      luaL_error(L, message);                                                               \
    }                                                                                       \
    if (lua_isstring(L, 4)) {                                                               \
      if (!StringToFramePoint(lua_tostring(L, 4), relativePoint)) {                         \
        luaL_error(L, "Unknown frame point");                                               \
      }                                                                                     \
      if (lua_isnumber(L, 5) && lua_isnumber(L, 6)) {                                       \
        offsetX = static_cast<float>(lua_tonumber(L, 5) * 0.0009765625f * 0.8f);            \
        offsetY = static_cast<float>(lua_tonumber(L, 6) * 0.0009765625f * 0.8f);            \
      }                                                                                     \
    }                                                                                       \
    object->SetPoint(point, relativeFrame, relativePoint, offsetX, offsetY, 1);             \
    return 0;                                                                               \
  }

#define DEFINE_RENDER_CLEAR_ALL_POINTS(TYPE, FUNCTION) \
  int FUNCTION(lua_State *L) {              \
    GET_SIMPLE_RENDER_THIS(L, TYPE, object);           \
    object->ClearAllPoints(1);                         \
    return 0;                                          \
  }

int CSimpleTexture_GetAlpha(lua_State *L) {
  GET_SIMPLE_RENDER_THIS(L, CSimpleTexture, object);

  NTempest::CImVector color;
  object->GetVertexColor(color);
  lua_pushnumber(L, static_cast<double>(color.a) / 255.0);
  return 1;
}

DEFINE_RENDER_GET_NAME(CSimpleTexture, CSimpleTexture_GetName)
DEFINE_RENDER_SET_VERTEX_COLOR(CSimpleTexture, CSimpleTexture_SetVertexColor)
DEFINE_RENDER_SET_ALPHA(CSimpleTexture, CSimpleTexture_SetAlpha)
DEFINE_RENDER_SHOW(CSimpleTexture, CSimpleTexture_Show)
DEFINE_RENDER_HIDE(CSimpleTexture, CSimpleTexture_Hide)
DEFINE_RENDER_IS_VISIBLE(CSimpleTexture, CSimpleTexture_IsVisible)

int CSimpleTexture_SetTexture(lua_State *L) {
  GET_SIMPLE_RENDER_THIS(L, CSimpleTexture, object);

  if (lua_isstring(L, 2)) {
    object->SetTexture(lua_tostring(L, 2), 0);
  } else {
    NTempest::CImVector color;
    float               red = static_cast<float>(lua_tonumber(L, 2));
    float               green = static_cast<float>(lua_tonumber(L, 3));
    float               blue = static_cast<float>(lua_tonumber(L, 4));
    float               alpha = 1.0f;

    if (lua_isnumber(L, 5)) {
      alpha = static_cast<float>(lua_tonumber(L, 5));
    }

    color.Set(alpha, red, green, blue);
    object->SetTexture(color);
  }

  return 0;
}

int CSimpleTexture_SetTexCoord(lua_State *L) {
  GET_SIMPLE_RENDER_THIS(L, CSimpleTexture, object);

  NTempest::CRect rect;
  rect.l = static_cast<float>(lua_tonumber(L, 2));
  rect.r = static_cast<float>(lua_tonumber(L, 3));
  rect.t = static_cast<float>(lua_tonumber(L, 4));
  rect.b = static_cast<float>(lua_tonumber(L, 5));
  object->SetTexCoord(rect);
  return 0;
}

DEFINE_RENDER_SET_POINT(CSimpleTexture, CSimpleTexture_SetPoint)
DEFINE_RENDER_CLEAR_ALL_POINTS(CSimpleTexture, CSimpleTexture_ClearAllPoints)

int CSimpleTexture_GetWidth(lua_State *L) {
  GET_SIMPLE_RENDER_THIS(L, CSimpleTexture, object);

  float width = object->GetWidth();
  if (width == 0.0f) {
    NTempest::CRect rect;
    object->GetRect(&rect);
    width = rect.r - rect.l;
  }

  lua_pushnumber(L, width * 1024.0f * 1.25f);
  return 1;
}

int CSimpleTexture_SetWidth(lua_State *L) {
  GET_SIMPLE_RENDER_THIS(L, CSimpleTexture, object);

  if (!lua_isnumber(L, 2)) {
    luaL_error(L, "Usage: SetWidth(width)");
  }

  object->SetWidth(static_cast<float>(lua_tonumber(L, 2) * 0.0009765625f * 0.8f));
  return 0;
}

int CSimpleTexture_GetHeight(lua_State *L) {
  GET_SIMPLE_RENDER_THIS(L, CSimpleTexture, object);

  float height = object->GetHeight();
  if (height == 0.0f) {
    NTempest::CRect rect;
    object->GetRect(&rect);
    height = rect.b - rect.t;
  }

  lua_pushnumber(L, height * 1024.0f * 1.25f);
  return 1;
}

int CSimpleTexture_SetHeight(lua_State *L) {
  GET_SIMPLE_RENDER_THIS(L, CSimpleTexture, object);

  if (!lua_isnumber(L, 2)) {
    luaL_error(L, "Usage: SetHeight(height)");
  }

  object->SetHeight(static_cast<float>(lua_tonumber(L, 2) * 0.0009765625f * 0.8f));
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

int CSimpleFontString_SetAlphaGradient(lua_State *L) {
  GET_SIMPLE_RENDER_THIS(L, CSimpleFontString, object);

  if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
    luaL_error(L, "Usage: SetAlphaGradient(start, length)");
  }

  int start = static_cast<int>(lua_tonumber(L, 2));
  int length = static_cast<int>(lua_tonumber(L, 3));
  if (object->SetAlphaGradient(start, length)) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }

  return 1;
}

int CSimpleFontString_SetText(lua_State *L) {
  GET_SIMPLE_RENDER_THIS(L, CSimpleFontString, object);

  object->SetText(lua_tostring(L, 2));
  return 0;
}

int CSimpleFontString_GetText(lua_State *L) {
  GET_SIMPLE_RENDER_THIS(L, CSimpleFontString, object);

  const char *text = object->GetText();
  if (!text || !*text) {
    text = 0;
  }

  lua_pushstring(L, text);
  return 1;
}

DEFINE_RENDER_GET_NAME(CSimpleFontString, CSimpleFontString_GetName)
DEFINE_RENDER_SET_VERTEX_COLOR(CSimpleFontString, CSimpleFontString_SetVertexColor)
DEFINE_RENDER_SET_ALPHA(CSimpleFontString, CSimpleFontString_SetAlpha)

int CSimpleFontString_SetTextHeight(lua_State *L) {
  GET_SIMPLE_RENDER_THIS(L, CSimpleFontString, object);

  if (!lua_isnumber(L, 2)) {
    luaL_error(L, "Usage: SetTextHeight(pixelHeight)");
  }

  object->SetTextHeight(static_cast<float>(lua_tonumber(L, 2) * 0.0009765625f * 0.8f));
  return 0;
}

DEFINE_RENDER_SHOW(CSimpleFontString, CSimpleFontString_Show)
DEFINE_RENDER_HIDE(CSimpleFontString, CSimpleFontString_Hide)
DEFINE_RENDER_IS_VISIBLE(CSimpleFontString, CSimpleFontString_IsVisible)

int CSimpleFontString_SetWidth(lua_State *L) {
  GET_SIMPLE_RENDER_THIS(L, CSimpleFontString, object);

  if (!lua_isnumber(L, 2)) {
    luaL_error(L, "Usage: SetWidth(pixelWidth)");
  }

  object->SetWidth(static_cast<float>(lua_tonumber(L, 2) * 0.0009765625f * 0.8f));
  return 0;
}

int CSimpleFontString_GetWidth(lua_State *L) {
  GET_SIMPLE_RENDER_THIS(L, CSimpleFontString, object);

  lua_pushnumber(L, object->GetWidth() * 1024.0f * 1.25f);
  return 1;
}

DEFINE_RENDER_SET_VERTEX_COLOR(CSimpleFontString, CSimpleFontString_SetTextColor)

int CSimpleFontString_SetHeight(lua_State *L) {
  GET_SIMPLE_RENDER_THIS(L, CSimpleFontString, object);

  if (!lua_isnumber(L, 2)) {
    luaL_error(L, "Usage: SetHeight(pixelHeight)");
  }

  object->SetHeight(static_cast<float>(lua_tonumber(L, 2) * 0.0009765625f * 0.8f));
  return 0;
}

int CSimpleFontString_GetHeight(lua_State *L) {
  GET_SIMPLE_RENDER_THIS(L, CSimpleFontString, object);

  lua_pushnumber(L, object->GetHeight() * 1024.0f * 1.25f);
  return 1;
}

int CSimpleFontString_SetJustifyH(lua_State *L) {
  GET_SIMPLE_RENDER_THIS(L, CSimpleFontString, object);

  unsigned int flag;
  if (!lua_isstring(L, 2) || !StringToJustify(lua_tostring(L, 2), flag)) {
    luaL_error(L, "Usage(SetJustifyH(\"justify\")");
  }

  object->SetHorizontalAlignment(flag);
  return 0;
}

int CSimpleFontString_SetJustifyV(lua_State *L) {
  GET_SIMPLE_RENDER_THIS(L, CSimpleFontString, object);

  unsigned int flag;
  if (!lua_isstring(L, 2) || !StringToJustify(lua_tostring(L, 2), flag)) {
    luaL_error(L, "Usage(SetJustifyV(\"justify\")");
  }

  object->SetVerticalAlignment(flag);
  return 0;
}

void CSimpleTexture::RegisterScriptMethods() {
  FrameScript_Object::FillScriptMethodTable(SimpleTextureMethods, sizeof(SimpleTextureMethods) / sizeof(SimpleTextureMethods[0]), s_scriptMethods);
}

DEFINE_RENDER_SET_POINT(CSimpleFontString, CSimpleFontString_SetPoint)
DEFINE_RENDER_CLEAR_ALL_POINTS(CSimpleFontString, CSimpleFontString_ClearAllPoints)

void CSimpleTexture::UnregisterScriptMethods() {
  FrameScript_Object::EmptyScriptMethodTable(s_scriptMethods);
}

int CSimpleTexture::LookupScriptMethod(lua_State *L, const char *name) {
  return FrameScript_Object::LookupScriptMethod(L, name, s_scriptMethods);
}

#undef DEFINE_RENDER_CLEAR_ALL_POINTS
#undef DEFINE_RENDER_SET_POINT
#undef DEFINE_RENDER_IS_VISIBLE
#undef DEFINE_RENDER_HIDE
#undef DEFINE_RENDER_SHOW
#undef DEFINE_RENDER_SET_ALPHA
#undef DEFINE_RENDER_SET_VERTEX_COLOR
#undef DEFINE_RENDER_GET_NAME
#undef GET_SIMPLE_RENDER_THIS

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

int CSimpleFontString::LookupScriptMethod(lua_State *L, const char *name) {
  return FrameScript_Object::LookupScriptMethod(L, name, s_scriptMethods);
}
