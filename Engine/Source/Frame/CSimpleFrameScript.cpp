#include <Base/Base.h>

#include "Frame/CSimpleFrame.h"

#include "Frame/CBackdropGenerator.h"
#include "FrameXML/LoadXML.h"

#include <lauxlib.h>
#include <lua.h>

#define GET_SIMPLE_FRAME_THIS(L, object)                                \
  CSimpleFrame *object = 0;                                             \
  if (lua_type(L, 1) == LUA_TTABLE) {                                   \
    lua_rawgeti(L, 1, 0);                                               \
    object = static_cast<CSimpleFrame *>(lua_touserdata(L, -1));        \
    lua_pop(L, 1);                                                      \
  } else {                                                              \
    luaL_error(                                                         \
        L,                                                              \
        "Attempt to find 'this' in non-table object (used '.' instead " \
        "of ':' ?)"                                                     \
    );                                                                  \
  }                                                                     \
  ASSERT(object)

int CSimpleFrame_GetParent(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  CSimpleFrame *parent = object->m_parent;
  const char   *name;

  if (parent && (name = parent->GetName()) != 0 && *name) {
    lua_getglobal(L, name);
  } else {
    lua_pushnil(L);
  }

  return 1;
}

int CSimpleFrame_GetName(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  const char *name = object->GetName();
  if (name && *name) {
    lua_pushstring(L, name);
  } else {
    lua_pushnil(L);
  }

  return 1;
}

int CSimpleFrame_GetFrameLevel(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  lua_pushnumber(L, object->GetFrameLevel());
  return 1;
}

int CSimpleFrame_SetFrameLevel(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  if (!lua_isnumber(L, 2)) {
    luaL_error(L, "Usage: SetFrameLevel(level)");
  }

  object->SetFrameLevel(static_cast<int>(lua_tonumber(L, 2)), 0);
  return 0;
}

int CSimpleFrame_RegisterEvent(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  if (!lua_isstring(L, 2)) {
    luaL_error(L, "Usage: RegisterEvent(\"event\")");
  }

  object->RegisterScriptEvent(lua_tostring(L, 2));
  return 0;
}

int CSimpleFrame_UnregisterEvent(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  if (!lua_isstring(L, 2)) {
    luaL_error(L, "Usage: UnregisterEvent(\"event\")");
  }

  object->UnregisterScriptEvent(lua_tostring(L, 2));
  return 0;
}

int CSimpleFrame_SetAlpha(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  if (!lua_isnumber(L, 2)) {
    luaL_error(L, "Usage: SetAlpha(alpha)");
  }

  double alpha = lua_tonumber(L, 2);
  if (alpha < 0.0 || alpha > 1.0) {
    luaL_error(L, "Alpha must be in the range of 0.0 to 1.0");
  }

  object->SetAlpha(static_cast<unsigned char>(alpha * 255.0));
  return 0;
}

int CSimpleFrame_GetAlpha(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  lua_pushnumber(L, static_cast<double>(object->GetAlpha()) / 255.0);
  return 1;
}

int CSimpleFrame_SetID(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  if (!lua_isnumber(L, 2)) {
    luaL_error(L, "Usage: SetID(ID)");
  }

  object->m_id = static_cast<int>(lua_tonumber(L, 2));
  return 0;
}

int CSimpleFrame_GetID(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  lua_pushnumber(L, object->m_id);
  return 1;
}

int CSimpleFrame_EnableDrawLayer(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  unsigned int layer = 2;
  if (lua_isstring(L, 2)) {
    StringToDrawLayer(lua_tostring(L, 2), layer);
  }

  object->EnableDrawLayer(layer);
  return 0;
}

int CSimpleFrame_DisableDrawLayer(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  unsigned int layer = 2;
  if (lua_isstring(L, 2)) {
    StringToDrawLayer(lua_tostring(L, 2), layer);
  }

  object->DisableDrawLayer(layer);
  return 0;
}

int CSimpleFrame_Show(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  object->Show();
  return 0;
}

int CSimpleFrame_Hide(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  object->Hide();
  return 0;
}

int CSimpleFrame_IsVisible(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  if (object->IsVisible()) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }

  return 1;
}

int CSimpleFrame_IsShown(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  if (object->m_shown) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }

  return 1;
}

int CSimpleFrame_Raise(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  object->Raise();
  return 0;
}

int CSimpleFrame_Lower(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  object->Lower();
  return 0;
}

int CSimpleFrame_GetCenter(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  lua_pushnumber(L, object->CenterX() * 1024.0f * 1.25f);
  lua_pushnumber(L, object->CenterY() * 1024.0f * 1.25f);
  return 2;
}

int CSimpleFrame_GetWidth(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  float width = object->GetWidth();
  if (width == 0.0f) {
    NTempest::CRect rect;

    object->GetRect(&rect);
    width = rect.r - rect.l;
  }

  lua_pushnumber(L, width * 1024.0f * 1.25f);
  return 1;
}

int CSimpleFrame_SetWidth(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  if (!lua_isnumber(L, 2)) {
    luaL_error(L, "Usage: SetWidth(width)");
  }

  object->SetWidth(static_cast<float>(lua_tonumber(L, 2) * 0.0009765625f * 0.8f));
  return 0;
}

int CSimpleFrame_GetHeight(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  float height = object->GetHeight();
  if (height == 0.0f) {
    NTempest::CRect rect;

    object->GetRect(&rect);
    height = rect.b - rect.t;
  }

  lua_pushnumber(L, height * 1024.0f * 1.25f);
  return 1;
}

int CSimpleFrame_SetHeight(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  if (!lua_isnumber(L, 2)) {
    luaL_error(L, "Usage: SetHeight(height)");
  }

  object->SetHeight(static_cast<float>(lua_tonumber(L, 2) * 0.0009765625f * 0.8f));
  return 0;
}

int CSimpleFrame_SetPoint(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  if (!lua_isstring(L, 2) || !lua_isstring(L, 3)) {
    luaL_error(
        L,
        "Usage: SetPoint(\"point\" \"frame\" [, relativePoint] "
        "[, offsetX, offsetY])"
    );
  }

  FRAMEPOINT    point;
  FRAMEPOINT    relativePoint;
  CLayoutFrame *relativeFrame;
  float         offsetX = 0.0f;
  float         offsetY = 0.0f;

  if (!StringToFramePoint(lua_tostring(L, 2), point)) {
    luaL_error(L, "Unknown frame point");
  }

  relativePoint = point;

  const char *relativeName = lua_tostring(L, 3);
  relativeFrame = object->GetLayoutFrameByName(relativeName);
  if (!relativeFrame) {
    char message[128];

    SStrPrintf(message, sizeof(message), "Couldn't find frame named '%s'", relativeName);
    luaL_error(L, message);
  }

  if (relativeFrame == object) {
    char message[128];

    SStrPrintf(message, sizeof(message), "Error: %s is anchored to itself", relativeName);
    luaL_error(L, message);
  }

  if (lua_isstring(L, 4)) {
    if (!StringToFramePoint(lua_tostring(L, 4), relativePoint)) {
      luaL_error(L, "Unknown frame point");
    }

    if (lua_isnumber(L, 5) && lua_isnumber(L, 6)) {
      offsetX = static_cast<float>(lua_tonumber(L, 5) * 0.0009765625f * 0.8f);
      offsetY = static_cast<float>(lua_tonumber(L, 6) * 0.0009765625f * 0.8f);
    }
  }

  object->SetPoint(point, relativeFrame, relativePoint, offsetX, offsetY, 1);
  return 0;
}

int CSimpleFrame_SetAllPoints(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  if (!lua_isstring(L, 2)) {
    luaL_error(L, "Usage: SetAllPoints(\"frame\")");
  }

  const char   *relativeName = lua_tostring(L, 2);
  CLayoutFrame *relativeFrame = object->GetLayoutFrameByName(relativeName);

  if (!relativeFrame) {
    char message[128];

    SStrPrintf(message, sizeof(message), "Couldn't find frame named '%s'", relativeName);
    luaL_error(L, message);
  }

  object->SetAllPoints(relativeFrame, 1);
  return 0;
}

int CSimpleFrame_ClearAllPoints(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  object->ClearAllPoints(1);
  return 0;
}

int CSimpleFrame_RegisterForDrag(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  CSimpleFrame *frame = object;
  unsigned int  buttons = 0;
  int           index = 2;

  while (lua_isstring(L, index)) {
    const char *button = lua_tostring(L, index);

    if (button && *button) {
      if (!SStrCmpI(button, "LeftButton", 0x7FFFFFFF)) {
        buttons |= MOUSE_BUTTON_LEFT;
      } else if (!SStrCmpI(button, "MiddleButton", 0x7FFFFFFF)) {
        buttons |= MOUSE_BUTTON_MIDDLE;
      } else if (!SStrCmpI(button, "RightButton", 0x7FFFFFFF)) {
        buttons |= MOUSE_BUTTON_RIGHT;
      }
    }

    ++index;
  }

  frame->RegisterForDrag(buttons);
  return 0;
}

int CSimpleFrame_EnableMouse(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  int enable;
  if (lua_isnumber(L, 2)) {
    enable = lua_tonumber(L, 2) != 0.0;
  } else {
    if (!lua_isstring(L, 2)) {
      luaL_error(L, "Usage: EnableMouse(0|1)");
    }

    enable = StringToBOOL(lua_tostring(L, 2));
  }

  if (enable) {
    object->EnableEvent(SIMPLE_EVENT_MOUSE, static_cast<unsigned int>(-1));
  } else {
    object->DisableEvent(SIMPLE_EVENT_MOUSE);
  }

  return 0;
}

int CSimpleFrame_EnableKeyboard(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  int enable;
  if (lua_isnumber(L, 2)) {
    enable = lua_tonumber(L, 2) != 0.0;
  } else {
    if (!lua_isstring(L, 2)) {
      luaL_error(L, "Usage: EnableKeyboard(0|1)");
    }

    enable = StringToBOOL(lua_tostring(L, 2));
  }

  if (enable) {
    object->EnableEvent(SIMPLE_EVENT_KEY, static_cast<unsigned int>(-1));
    object->EnableEvent(SIMPLE_EVENT_CHAR, static_cast<unsigned int>(-1));
  } else {
    object->DisableEvent(SIMPLE_EVENT_KEY);
    object->DisableEvent(SIMPLE_EVENT_CHAR);
  }

  return 0;
}

int CSimpleFrame_SetBackdropColor(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  NTempest::CImVector color;
  float               red = static_cast<float>(lua_tonumber(L, 2));
  float               green = static_cast<float>(lua_tonumber(L, 3));
  float               blue = static_cast<float>(lua_tonumber(L, 4));
  float               alpha = 1.0f;

  if (lua_isnumber(L, 5)) {
    alpha = static_cast<float>(lua_tonumber(L, 5));
  }

  color.Set(alpha, red, green, blue);
  if (object->m_backdrop) {
    object->m_backdrop->SetVertexColor(color);
  }

  return 0;
}

int CSimpleFrame_SetBackdropBorderColor(lua_State *L) {
  GET_SIMPLE_FRAME_THIS(L, object);

  NTempest::CImVector color;
  float               red = static_cast<float>(lua_tonumber(L, 2));
  float               green = static_cast<float>(lua_tonumber(L, 3));
  float               blue = static_cast<float>(lua_tonumber(L, 4));
  float               alpha = 1.0f;

  if (lua_isnumber(L, 5)) {
    alpha = static_cast<float>(lua_tonumber(L, 5));
  }

  color.Set(alpha, red, green, blue);
  if (object->m_backdrop) {
    object->m_backdrop->SetBorderVertexColor(color);
  }

  return 0;
}

#undef GET_SIMPLE_FRAME_THIS

static FrameScript_Method SimpleFrameMethods[] = {
    {             "GetParent",              CSimpleFrame_GetParent},
    {               "GetName",                CSimpleFrame_GetName},
    {         "GetFrameLevel",          CSimpleFrame_GetFrameLevel},
    {         "SetFrameLevel",          CSimpleFrame_SetFrameLevel},
    {         "RegisterEvent",          CSimpleFrame_RegisterEvent},
    {       "UnregisterEvent",        CSimpleFrame_UnregisterEvent},
    {              "SetAlpha",               CSimpleFrame_SetAlpha},
    {              "GetAlpha",               CSimpleFrame_GetAlpha},
    {                 "SetID",                  CSimpleFrame_SetID},
    {                 "GetID",                  CSimpleFrame_GetID},
    {       "EnableDrawLayer",        CSimpleFrame_EnableDrawLayer},
    {      "DisableDrawLayer",       CSimpleFrame_DisableDrawLayer},
    {                  "Show",                   CSimpleFrame_Show},
    {                  "Hide",                   CSimpleFrame_Hide},
    {             "IsVisible",              CSimpleFrame_IsVisible},
    {               "IsShown",                CSimpleFrame_IsShown},
    {                 "Raise",                  CSimpleFrame_Raise},
    {                 "Lower",                  CSimpleFrame_Lower},
    {             "GetCenter",              CSimpleFrame_GetCenter},
    {              "GetWidth",               CSimpleFrame_GetWidth},
    {              "SetWidth",               CSimpleFrame_SetWidth},
    {             "GetHeight",              CSimpleFrame_GetHeight},
    {             "SetHeight",              CSimpleFrame_SetHeight},
    {              "SetPoint",               CSimpleFrame_SetPoint},
    {          "SetAllPoints",           CSimpleFrame_SetAllPoints},
    {        "ClearAllPoints",         CSimpleFrame_ClearAllPoints},
    {       "RegisterForDrag",        CSimpleFrame_RegisterForDrag},
    {           "EnableMouse",            CSimpleFrame_EnableMouse},
    {        "EnableKeyboard",         CSimpleFrame_EnableKeyboard},
    {      "SetBackdropColor",       CSimpleFrame_SetBackdropColor},
    {"SetBackdropBorderColor", CSimpleFrame_SetBackdropBorderColor}
};

TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> CSimpleFrame::s_scriptMethods;

void CSimpleFrame::RegisterScriptMethods() {
  FrameScript_Object::FillScriptMethodTable(SimpleFrameMethods, sizeof(SimpleFrameMethods) / sizeof(SimpleFrameMethods[0]), s_scriptMethods);
}

void CSimpleFrame::UnregisterScriptMethods() {
  FrameScript_Object::EmptyScriptMethodTable(s_scriptMethods);
}

int CSimpleFrame::LookupScriptMethod(lua_State *L, const char *name) {
  return FrameScript_Object::LookupScriptMethod(L, name, s_scriptMethods);
}
