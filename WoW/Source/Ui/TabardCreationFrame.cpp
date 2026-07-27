#include "GameUI.h"
#include "Game/GameClient/GuildClient.h"

#include <FrameScript/FrameScript.h>

#include <lua.h>

static const float MAX_SHOP_DISTANCE = 5.5555553f;
static const float MAX_SHOP_DISTANCE_SQUARED = MAX_SHOP_DISTANCE * MAX_SHOP_DISTANCE;

class CGTabardCreationFrame {
 public:
  static void EnterWorld();
  static void LeaveWorld();
  static void Open(const unsigned __int64 &vendor);
  static void Close();
  static void ClearVendor() {
    m_vendor = 0;
  }
  static unsigned __int64 GetVendor();

 private:
  static unsigned __int64 m_vendor;
};

unsigned __int64 CGTabardCreationFrame::m_vendor;

void CGTabardCreationFrame::EnterWorld() {
  m_vendor = 0;
}

void CGTabardCreationFrame::LeaveWorld() {
  Close();
}

unsigned __int64 CGTabardCreationFrame::GetVendor() {
  return m_vendor;
}

void CGTabardCreationFrame::Open(const unsigned __int64 &vendor) {
  m_vendor = vendor;
  CGGameUI::SetInteractTarget(vendor, MAX_SHOP_DISTANCE_SQUARED);
  FrameScript_SignalEvent(357);
}

void CGTabardCreationFrame::Close() {
  if (m_vendor) {
    FrameScript_SignalEvent(358);
    CGGameUI::ClearInteractTarget(m_vendor);
    m_vendor = 0;
  }
}

static int Script_CloseTabardCreation(lua_State *__formal) {
  CGTabardCreationFrame::Close();
  return 0;
}

static int Script_GetTabardCreationCost(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(GuildGetTabardCost()));
  return 1;
}

static int Script_TabardFrameClosed(lua_State *L) {
  CGTabardCreationFrame::ClearVendor();
  return 0;
}

static FrameScript_Method s_ScriptFunctions[3] = {
    {  "CloseTabardCreation",   Script_CloseTabardCreation},
    {"GetTabardCreationCost", Script_GetTabardCreationCost},
    {    "TabardFrameClosed",     Script_TabardFrameClosed}
};

void TabardCreationRegisterScriptFunctions() {
  for (unsigned int i = 0; i < 3; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void TabardCreationUnregisterScriptFunctions() {
  for (unsigned int i = 0; i < 3; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
