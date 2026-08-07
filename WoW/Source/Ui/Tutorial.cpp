#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

#include "Tutorial.h"

#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <Base/CDataStore.h>
#include <FrameScript/FrameScript.h>
#include <storm.h>

#include <lauxlib.h>
#include <lua.h>

static LPCSTR s_tutorialTokens[18] = {"QUESTGIVERS", "MOVEMENT",     "CAMERA", "TARGETING", "TARGETING_ENEMY", "COMBAT",  "LOOTING",
                                      "ITEMS",       "USABLE_ITEMS", "BAGS",   "FOOD",      "DRINK",           "TALENTS", "SKILLS",
                                      "ABILITIES",   "REPUTATION",   "TELLS",  "GROUPING"};

FBitField CGTutorial::m_tutorialFlags;

void CGTutorial::InitializeGame() {
  m_tutorialFlags.ClearAll();
  ClientServices_SetMessageHandler(SMSG_TUTORIAL_FLAGS, OnTutorialFlags, 0);
}

void CGTutorial::ShutdownGame() {
  ClientServices_ClearMessageHandler(SMSG_TUTORIAL_FLAGS);
}

void CGTutorial::TriggerTutorial(TUTORIAL tutorial) {
}

void CGTutorial::ClearTutorials() {
  m_tutorialFlags.SetAll();

  CDataStore msg;
  msg.Put(static_cast<UINT>(CMSG_TUTORIAL_CLEAR));
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGTutorial::ResetTutorials() {
  m_tutorialFlags.ClearAll();

  CDataStore msg;
  msg.Put(static_cast<UINT>(CMSG_TUTORIAL_RESET));
  msg.Finalize();
  ClientServices_Send(&msg);
}

BOOL CGTutorial::OnTutorialFlags(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  UINT   byteCount = msg->Size() - msg->Tell();
  LPVOID data;

  msg->GetDataInSitu(data, byteCount);
  m_tutorialFlags.Load(data, byteCount);
  return 1;
}

static int Script_TriggerTutorial(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: TriggerTutorial(\"tutorial\")");
  }

  LPCSTR token = lua_tostring(L, 1);
  UINT   tutorial;
  for (tutorial = 0; tutorial < NUM_TUTORIALS; ++tutorial) {
    if (!SStrCmpI(token, s_tutorialTokens[tutorial], 0x7FFFFFFF)) {
      break;
    }
  }
  if (tutorial == NUM_TUTORIALS) {
    return luaL_error(L, "Unknown tutorial token");
  }
  return 0;
}

static int Script_ClearTutorials(lua_State *L) {
  CGTutorial::ClearTutorials();
  return 0;
}

static int Script_ResetTutorials(lua_State *L) {
  CGTutorial::ResetTutorials();
  return 0;
}

static FrameScript_Method s_ScriptFunctions[3] = {
    {"TriggerTutorial", Script_TriggerTutorial},
    { "ClearTutorials",  Script_ClearTutorials},
    { "ResetTutorials",  Script_ResetTutorials}
};

void TutorialRegisterScriptFunctions() {
  for (UINT i = 0; i < 3; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void TutorialUnregisterScriptFunctions() {
  for (UINT i = 0; i < 3; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
