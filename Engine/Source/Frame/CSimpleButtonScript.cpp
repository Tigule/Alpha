#include <Base/Base.h>

#include "Frame/CSimpleTop.h"
#include "Frame/CSimpleButton.h"
#include "FrameXML/LoadXML.h"

#include <lauxlib.h>
#include <lua.h>

static BOOL StringToButtonState(LPCSTR string, CSimpleButtonState &state) {
  struct {
    CSimpleButtonState state;
    LPCSTR             string;
  } array[3] = {
      {BUTTONSTATE_DISABLED, "DISABLED"},
      {  BUTTONSTATE_NORMAL,   "NORMAL"},
      {  BUTTONSTATE_PUSHED,   "PUSHED"}
  };

  for (UINT i = 0; i < 3; ++i) {
    if (!SStrCmpI(array[i].string, string, 0x7FFFFFFF)) {
      state = array[i].state;
      return 1;
    }
  }

  return 0;
}

static LPCSTR ButtonStateToString(CSimpleButtonState state) {
  switch (state) {
    case BUTTONSTATE_DISABLED:
      return "DISABLED";
    case BUTTONSTATE_NORMAL:
      return "NORMAL";
    case BUTTONSTATE_PUSHED:
      return "PUSHED";
    default:
      return "UNKNOWN";
  }
}

static int CSimpleButton_Enable(lua_State *L) {
  CSimpleButton *object = static_cast<CSimpleButton *>(FrameScript_GetObjectThis(L));

  object->Enable(1);
  return 0;
}

static int CSimpleButton_Disable(lua_State *L) {
  CSimpleButton *object = static_cast<CSimpleButton *>(FrameScript_GetObjectThis(L));

  object->Enable(0);
  return 0;
}

static int CSimpleButton_IsEnabled(lua_State *L) {
  CSimpleButton *object = static_cast<CSimpleButton *>(FrameScript_GetObjectThis(L));

  lua_pushnumber(L, object->IsEnabled() != 0);
  return 1;
}

static int CSimpleButton_GetButtonState(lua_State *L) {
  CSimpleButton *object = static_cast<CSimpleButton *>(FrameScript_GetObjectThis(L));

  lua_pushstring(L, ButtonStateToString(object->GetButtonState()));
  return 1;
}

static int CSimpleButton_SetButtonState(lua_State *L) {
  CSimpleButton *object = static_cast<CSimpleButton *>(FrameScript_GetObjectThis(L));

  CSimpleButtonState state = BUTTONSTATE_DISABLED;
  int                lock = 0;
  if (lua_isstring(L, 2) && StringToButtonState(lua_tostring(L, 2), state)) {
    if (lua_isnumber(L, 3)) {
      lock = static_cast<int>(lua_tonumber(L, 3)) > 0;
    } else if (lua_isstring(L, 3)) {
      lock = StringToBOOL(lua_tostring(L, 3));
    }

    object->SetButtonState(state, lock);
    return 0;
  }

  luaL_error(L, "Usage: SetButtonState(\"state\", lock)");
  return 0;
}

static int CSimpleButton_SetText(lua_State *L) {
  CSimpleButton *object = static_cast<CSimpleButton *>(FrameScript_GetObjectThis(L));

  if (lua_isstring(L, 2)) {
    LPCSTR text = lua_tostring(L, 2);
    object->SetTextString(text);
    object->SetDisabledTextString(text);
    object->SetHighlightTextString(text);
    return 0;
  }

  luaL_error(L, "Usage: SetText(\"text\")");
  return 0;
}

static int CSimpleButton_SetTextColor(lua_State *L) {
  CSimpleButton *object = static_cast<CSimpleButton *>(FrameScript_GetObjectThis(L));

  float red = static_cast<float>(lua_tonumber(L, 2));
  float green = static_cast<float>(lua_tonumber(L, 3));
  float blue = static_cast<float>(lua_tonumber(L, 4));
  float alpha = 1.0f;
  if (lua_isnumber(L, 5)) {
    alpha = static_cast<float>(lua_tonumber(L, 5));
  }

  NTempest::CImVector color;
  color.Set(alpha, red, green, blue);
  object->SetTextColor(color);
  return 0;
}

static int CSimpleButton_SetDisabledTextColor(lua_State *L) {
  CSimpleButton *object = static_cast<CSimpleButton *>(FrameScript_GetObjectThis(L));

  float red = static_cast<float>(lua_tonumber(L, 2));
  float green = static_cast<float>(lua_tonumber(L, 3));
  float blue = static_cast<float>(lua_tonumber(L, 4));
  float alpha = 1.0f;
  if (lua_isnumber(L, 5)) {
    alpha = static_cast<float>(lua_tonumber(L, 5));
  }

  NTempest::CImVector color;
  color.Set(alpha, red, green, blue);
  object->SetDisabledTextColor(color);
  return 0;
}

static int CSimpleButton_SetHighlightTextColor(lua_State *L) {
  CSimpleButton *object = static_cast<CSimpleButton *>(FrameScript_GetObjectThis(L));

  float red = static_cast<float>(lua_tonumber(L, 2));
  float green = static_cast<float>(lua_tonumber(L, 3));
  float blue = static_cast<float>(lua_tonumber(L, 4));
  float alpha = 1.0f;
  if (lua_isnumber(L, 5)) {
    alpha = static_cast<float>(lua_tonumber(L, 5));
  }

  NTempest::CImVector color;
  color.Set(alpha, red, green, blue);
  object->SetHighlightTextColor(color);
  return 0;
}

static int CSimpleButton_SetNormalTexture(lua_State *L) {
  CSimpleButton *object = static_cast<CSimpleButton *>(FrameScript_GetObjectThis(L));

  if (lua_isstring(L, 2)) {
    object->SetStateTexture(BUTTONSTATE_NORMAL, lua_tostring(L, 2));
  } else {
    object->SetStateTexture(BUTTONSTATE_NORMAL, static_cast<CSimpleTexture *>(0));
  }
  return 0;
}

static int CSimpleButton_SetPushedTexture(lua_State *L) {
  CSimpleButton *object = static_cast<CSimpleButton *>(FrameScript_GetObjectThis(L));

  if (lua_isstring(L, 2)) {
    object->SetStateTexture(BUTTONSTATE_PUSHED, lua_tostring(L, 2));
  } else {
    object->SetStateTexture(BUTTONSTATE_PUSHED, static_cast<CSimpleTexture *>(0));
  }
  return 0;
}

static int CSimpleButton_SetDisabledTexture(lua_State *L) {
  CSimpleButton *object = static_cast<CSimpleButton *>(FrameScript_GetObjectThis(L));

  if (lua_isstring(L, 2)) {
    object->SetStateTexture(BUTTONSTATE_DISABLED, lua_tostring(L, 2));
  } else {
    object->SetStateTexture(BUTTONSTATE_DISABLED, static_cast<CSimpleTexture *>(0));
  }
  return 0;
}

static int CSimpleButton_SetHighlightTexture(lua_State *L) {
  CSimpleButton *object = static_cast<CSimpleButton *>(FrameScript_GetObjectThis(L));

  if (lua_isstring(L, 2)) {
    EGxBlend blendMode = GxBlend_Add;
    if (lua_isstring(L, 3)) {
      StringToBlendMode(lua_tostring(L, 3), blendMode);
    }
    object->SetHighlight(lua_tostring(L, 2), blendMode);
  } else {
    object->SetHighlight(static_cast<CSimpleTexture *>(0), GxBlend_Add);
  }
  return 0;
}

static int CSimpleButton_GetText(lua_State *L) {
  CSimpleButton *object = static_cast<CSimpleButton *>(FrameScript_GetObjectThis(L));

  lua_pushstring(L, object->GetTextString());
  return 1;
}

static int CSimpleButton_GetTextWidth(lua_State *L) {
  CSimpleButton *object = static_cast<CSimpleButton *>(FrameScript_GetObjectThis(L));

  CSimpleFontString *text = object->GetText();
  float              width = text ? text->GetWidth() : 0.0f;
  lua_pushnumber(L, 1.25f * (width * 1024.0f));
  return 1;
}

static int CSimpleButton_GetTextHeight(lua_State *L) {
  CSimpleButton *object = static_cast<CSimpleButton *>(FrameScript_GetObjectThis(L));

  CSimpleFontString *text = object->GetText();
  float              height = text ? text->GetHeight() : 0.0f;
  lua_pushnumber(L, 1.25f * (height * 1024.0f));
  return 1;
}

static int CSimpleButton_RegisterForClicks(lua_State *L) {
  CSimpleButton *object = static_cast<CSimpleButton *>(FrameScript_GetObjectThis(L));

  CSimpleButton *button = object;
  UINT           buttons = 0;
  int            index = 2;
  while (lua_isstring(L, index)) {
    LPCSTR click = lua_tostring(L, index);

    if (click && *click) {
      if (!SStrCmpI(click, "LeftButtonDown", 0x7FFFFFFF)) {
        buttons |= 0x001;
      } else if (!SStrCmpI(click, "LeftButtonUp", 0x7FFFFFFF)) {
        buttons |= 0x100;
      } else if (!SStrCmpI(click, "MiddleButtonDown", 0x7FFFFFFF)) {
        buttons |= 0x002;
      } else if (!SStrCmpI(click, "MiddleButtonUp", 0x7FFFFFFF)) {
        buttons |= 0x200;
      } else if (!SStrCmpI(click, "RightButtonDown", 0x7FFFFFFF)) {
        buttons |= 0x004;
      } else if (!SStrCmpI(click, "RightButtonUp", 0x7FFFFFFF)) {
        buttons |= 0x400;
      } else if (!SStrCmpI(click, "Button4Down", 0x7FFFFFFF)) {
        buttons |= 0x008;
      } else if (!SStrCmpI(click, "Button4Up", 0x7FFFFFFF)) {
        buttons |= 0x800;
      } else if (!SStrCmpI(click, "Button5Down", 0x7FFFFFFF)) {
        buttons |= 0x010;
      } else if (!SStrCmpI(click, "Button5Up", 0x7FFFFFFF)) {
        buttons |= 0x1000;
      }
    }

    ++index;
  }

  button->SetClickAction(buttons);
  return 0;
}

static int CSimpleButton_Click(lua_State *L) {
  CSimpleButton *object = static_cast<CSimpleButton *>(FrameScript_GetObjectThis(L));

  MOUSEBUTTON button = MOUSE_BUTTON_LEFT;
  if (lua_isstring(L, 2)) {
    LPCSTR name = lua_tostring(L, 2);
    button = MOUSE_BUTTON_NONE;

    if (name && *name) {
      if (!SStrCmpI(name, "LeftButton", 0x7FFFFFFF)) {
        button = MOUSE_BUTTON_LEFT;
      } else if (!SStrCmpI(name, "MiddleButton", 0x7FFFFFFF)) {
        button = MOUSE_BUTTON_MIDDLE;
      } else if (!SStrCmpI(name, "RightButton", 0x7FFFFFFF)) {
        button = MOUSE_BUTTON_RIGHT;
      }
    }
  }

  object->OnClick(button);
  return 0;
}

static int CSimpleButton_LockHighlight(lua_State *L) {
  CSimpleButton *object = static_cast<CSimpleButton *>(FrameScript_GetObjectThis(L));

  object->LockHighlight(1);
  return 0;
}

static int CSimpleButton_UnlockHighlight(lua_State *L) {
  CSimpleButton *object = static_cast<CSimpleButton *>(FrameScript_GetObjectThis(L));

  object->LockHighlight(0);
  return 0;
}

static FrameScript_Method SimpleButtonMethods[20] = {
    {               "Enable",                CSimpleButton_Enable},
    {              "Disable",               CSimpleButton_Disable},
    {            "IsEnabled",             CSimpleButton_IsEnabled},
    {       "GetButtonState",        CSimpleButton_GetButtonState},
    {       "SetButtonState",        CSimpleButton_SetButtonState},
    {              "SetText",               CSimpleButton_SetText},
    {         "SetTextColor",          CSimpleButton_SetTextColor},
    { "SetDisabledTextColor",  CSimpleButton_SetDisabledTextColor},
    {"SetHighlightTextColor", CSimpleButton_SetHighlightTextColor},
    {     "SetNormalTexture",      CSimpleButton_SetNormalTexture},
    {     "SetPushedTexture",      CSimpleButton_SetPushedTexture},
    {   "SetDisabledTexture",    CSimpleButton_SetDisabledTexture},
    {  "SetHighlightTexture",   CSimpleButton_SetHighlightTexture},
    {              "GetText",               CSimpleButton_GetText},
    {         "GetTextWidth",          CSimpleButton_GetTextWidth},
    {        "GetTextHeight",         CSimpleButton_GetTextHeight},
    {    "RegisterForClicks",     CSimpleButton_RegisterForClicks},
    {                "Click",                 CSimpleButton_Click},
    {        "LockHighlight",         CSimpleButton_LockHighlight},
    {      "UnlockHighlight",       CSimpleButton_UnlockHighlight}
};

TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> CSimpleButton::s_scriptMethods;

void CSimpleButton::RegisterScriptMethods() {
  FrameScript_Object::FillScriptMethodTable(SimpleButtonMethods, 20, s_scriptMethods);
}

void CSimpleButton::UnregisterScriptMethods() {
  FrameScript_Object::EmptyScriptMethodTable(s_scriptMethods);
}

BOOL CSimpleButton::LookupScriptMethod(lua_State *L, LPCSTR name) {
  if (FrameScript_Object::LookupScriptMethod(L, name, s_scriptMethods)) {
    return 1;
  }

  return CSimpleFrame::LookupScriptMethod(L, name);
}
