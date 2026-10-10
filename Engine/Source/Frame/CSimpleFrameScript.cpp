#include <Base/Base.h>

#include "Frame/CSimpleTop.h"
#include "Frame/CSimpleFrame.h"

#include "Frame/CBackdropGenerator.h"
#include "FrameXML/LoadXML.h"

#include <lauxlib.h>
#include <lua.h>

static int CSimpleFrame_GetParent(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  CSimpleFrame *parent = object->m_parent;
  LPCSTR        name;

  if (parent && (name = parent->GetName()) != 0 && *name) {
    lua_getglobal(L, name);
  } else {
    lua_pushnil(L);
  }

  return 1;
}

static int CSimpleFrame_GetName(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  LPCSTR name = object->GetName();
  if (name && *name) {
    lua_pushstring(L, name);
  } else {
    lua_pushnil(L);
  }

  return 1;
}

static int CSimpleFrame_GetFrameLevel(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  lua_pushnumber(L, object->GetFrameLevel());
  return 1;
}

static int CSimpleFrame_SetFrameLevel(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  if (lua_isnumber(L, 2)) {
    object->SetFrameLevel(lua_tonumber(L, 2), 0);
    return 0;
  }

  luaL_error(L, "Usage: SetFrameLevel(level)");
  return 0;
}

static int CSimpleFrame_RegisterEvent(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  if (lua_isstring(L, 2)) {
    object->RegisterScriptEvent(lua_tostring(L, 2));
    return 0;
  }

  luaL_error(L, "Usage: RegisterEvent(\"event\")");
  return 0;
}

static int CSimpleFrame_UnregisterEvent(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  if (lua_isstring(L, 2)) {
    object->UnregisterScriptEvent(lua_tostring(L, 2));
    return 0;
  }

  luaL_error(L, "Usage: UnregisterEvent(\"event\")");
  return 0;
}

static int CSimpleFrame_SetAlpha(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  if (lua_isnumber(L, 2)) {
    double alpha = lua_tonumber(L, 2);
    if (alpha >= 0.0 && alpha <= 1.0) {
      object->SetAlpha(alpha * 255.0);
      return 0;
    }

    luaL_error(L, "Alpha must be in the range of 0.0 to 1.0");
    return 0;
  }

  luaL_error(L, "Usage: SetAlpha(alpha)");
  return 0;
}

static int CSimpleFrame_GetAlpha(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  lua_pushnumber(L, (double)object->GetAlpha() / 255.0);
  return 1;
}

static int CSimpleFrame_SetID(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  if (lua_isnumber(L, 2)) {
    object->m_id = lua_tonumber(L, 2);
    return 0;
  }

  luaL_error(L, "Usage: SetID(ID)");
  return 0;
}

static int CSimpleFrame_GetID(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  lua_pushnumber(L, object->m_id);
  return 1;
}

static int CSimpleFrame_EnableDrawLayer(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  UINT layer = 2;
  if (lua_isstring(L, 2)) {
    StringToDrawLayer(lua_tostring(L, 2), layer);
  }

  object->EnableDrawLayer(layer);
  return 0;
}

static int CSimpleFrame_DisableDrawLayer(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  UINT layer = 2;
  if (lua_isstring(L, 2)) {
    StringToDrawLayer(lua_tostring(L, 2), layer);
  }

  object->DisableDrawLayer(layer);
  return 0;
}

static int CSimpleFrame_Show(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  object->Show();
  return 0;
}

static int CSimpleFrame_Hide(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  object->Hide();
  return 0;
}

static int CSimpleFrame_IsVisible(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  if (object->IsVisible()) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }

  return 1;
}

static int CSimpleFrame_IsShown(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  if (object->m_shown) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }

  return 1;
}

static int CSimpleFrame_Raise(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  object->Raise();
  return 0;
}

static int CSimpleFrame_Lower(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  object->Lower();
  return 0;
}

static int CSimpleFrame_GetCenter(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  lua_pushnumber(L, 1.25f * (object->CenterX() * 1024.0f));
  lua_pushnumber(L, 1.25f * (object->CenterY() * 1024.0f));
  return 2;
}

static int CSimpleFrame_GetWidth(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  float width = object->GetWidth();
  if (width == 0.0f) {
    NTempest::CRect rect;

    object->GetRect(&rect);
    width = rect.r - rect.l;
  }

  lua_pushnumber(L, 1.25f * (width * 1024.0f));
  return 1;
}

static int CSimpleFrame_SetWidth(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  if (lua_isnumber(L, 2)) {
    object->SetWidth(0.8f * (lua_tonumber(L, 2) * 0.0009765625f));
    return 0;
  }

  luaL_error(L, "Usage: SetWidth(width)");
  return 0;
}

static int CSimpleFrame_GetHeight(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  float height = object->GetHeight();
  if (height == 0.0f) {
    NTempest::CRect rect;

    object->GetRect(&rect);
    height = rect.b - rect.t;
  }

  lua_pushnumber(L, 1.25f * (height * 1024.0f));
  return 1;
}

static int CSimpleFrame_SetHeight(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  if (lua_isnumber(L, 2)) {
    object->SetHeight(0.8f * (lua_tonumber(L, 2) * 0.0009765625f));
    return 0;
  }

  luaL_error(L, "Usage: SetHeight(height)");
  return 0;
}

static int CSimpleFrame_SetPoint(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  if (lua_isstring(L, 2) && lua_isstring(L, 3)) {
    FRAMEPOINT    point;
    FRAMEPOINT    relativePoint;
    CLayoutFrame *relativeFrame;
    float         offsetX = 0.0f;
    float         offsetY = 0.0f;

    if (!StringToFramePoint(lua_tostring(L, 2), point)) {
      luaL_error(L, "Unknown frame point");
      return 0;
    }

    relativePoint = point;

    LPCSTR relativeName = lua_tostring(L, 3);
    relativeFrame = object->GetLayoutFrameByName(relativeName);
    if (!relativeFrame) {
      char message[128];

      SStrPrintf(message, sizeof(message), "Couldn't find frame named '%s'", relativeName);
      luaL_error(L, message);
      return 0;
    }

    if (relativeFrame == object) {
      char message[128];

      SStrPrintf(message, sizeof(message), "Error: %s is anchored to itself", relativeName);
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

static int CSimpleFrame_SetAllPoints(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  if (lua_isstring(L, 2)) {
    LPCSTR        relativeName = lua_tostring(L, 2);
    CLayoutFrame *relativeFrame = object->GetLayoutFrameByName(relativeName);

    if (!relativeFrame) {
      char message[128];

      SStrPrintf(message, sizeof(message), "Couldn't find frame named '%s'", relativeName);
      luaL_error(L, message);
      return 0;
    }

    object->SetAllPoints(relativeFrame, 1);
    return 0;
  }

  luaL_error(L, "Usage: SetAllPoints(\"frame\")");
  return 0;
}

static int CSimpleFrame_ClearAllPoints(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  object->ClearAllPoints(1);
  return 0;
}

static int CSimpleFrame_RegisterForDrag(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  CSimpleFrame *frame = object;
  UINT          buttons = 0;
  int           index = 2;

  while (lua_isstring(L, index)) {
    LPCSTR button = lua_tostring(L, index);
    buttons |= !button || !*button                           ? MOUSE_BUTTON_NONE
             : !SStrCmpI(button, "LeftButton", 0x7FFFFFFF)   ? MOUSE_BUTTON_LEFT
             : !SStrCmpI(button, "MiddleButton", 0x7FFFFFFF) ? MOUSE_BUTTON_MIDDLE
             : !SStrCmpI(button, "RightButton", 0x7FFFFFFF)  ? MOUSE_BUTTON_RIGHT
                                                             : MOUSE_BUTTON_NONE;
    ++index;
  }

  frame->RegisterForDrag(buttons);
  return 0;
}

static int CSimpleFrame_EnableMouse(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  int enable;
  if (lua_isnumber(L, 2)) {
    enable = lua_tonumber(L, 2) != 0.0;
  } else if (lua_isstring(L, 2)) {
    enable = StringToBOOL(lua_tostring(L, 2));
  } else {
    luaL_error(L, "Usage: EnableMouse(0|1)");
    return 0;
  }

  if (enable) {
    object->EnableEvent(SIMPLE_EVENT_MOUSE, -1);
  } else {
    object->DisableEvent(SIMPLE_EVENT_MOUSE);
  }

  return 0;
}

static int CSimpleFrame_EnableKeyboard(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  int enable;
  if (lua_isnumber(L, 2)) {
    enable = lua_tonumber(L, 2) != 0.0;
  } else if (lua_isstring(L, 2)) {
    enable = StringToBOOL(lua_tostring(L, 2));
  } else {
    luaL_error(L, "Usage: EnableKeyboard(0|1)");
    return 0;
  }

  if (enable) {
    object->EnableEvent(SIMPLE_EVENT_KEY, -1);
    object->EnableEvent(SIMPLE_EVENT_CHAR, -1);
  } else {
    object->DisableEvent(SIMPLE_EVENT_KEY);
    object->DisableEvent(SIMPLE_EVENT_CHAR);
  }

  return 0;
}

static int CSimpleFrame_SetBackdropColor(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  NTempest::CImVector color;
  float               red = lua_tonumber(L, 2);
  float               green = lua_tonumber(L, 3);
  float               blue = lua_tonumber(L, 4);
  float               alpha = 1.0f;

  if (lua_isnumber(L, 5)) {
    alpha = lua_tonumber(L, 5);
  }

  color.Set(alpha, red, green, blue);
  if (object->m_backdrop) {
    object->m_backdrop->SetVertexColor(color);
  }

  return 0;
}

static int CSimpleFrame_SetBackdropBorderColor(lua_State *L) {
  CSimpleFrame *object = (CSimpleFrame *)FrameScript_GetObjectThis(L);

  NTempest::CImVector color;
  float               red = lua_tonumber(L, 2);
  float               green = lua_tonumber(L, 3);
  float               blue = lua_tonumber(L, 4);
  float               alpha = 1.0f;

  if (lua_isnumber(L, 5)) {
    alpha = lua_tonumber(L, 5);
  }

  color.Set(alpha, red, green, blue);
  if (object->m_backdrop) {
    object->m_backdrop->SetBorderVertexColor(color);
  }

  return 0;
}

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

BOOL CSimpleFrame::LookupScriptMethod(lua_State *L, LPCSTR name) {
  return FrameScript_Object::LookupScriptMethod(L, name, s_scriptMethods);
}
