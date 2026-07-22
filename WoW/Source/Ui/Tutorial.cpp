#include "Tutorial.h"

#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <Base/CDataStore.h>
#include <FrameScript/FrameScript.h>
#include <storm.h>

#include <lauxlib.h>
#include <lua.h>

static const char *s_tutorialTokens[18] = {"QUESTGIVERS", "MOVEMENT",     "CAMERA", "TARGETING", "TARGETING_ENEMY", "COMBAT",  "LOOTING",
                                           "ITEMS",       "USABLE_ITEMS", "BAGS",   "FOOD",      "DRINK",           "TALENTS", "SKILLS",
                                           "ABILITIES",   "REPUTATION",   "TELLS",  "GROUPING"};

FBitField CGTutorial::m_tutorialFlags;

void __fastcall CGTutorial::InitializeGame() {
  m_tutorialFlags.ClearAll();
  ClientServices_SetMessageHandler(SMSG_TUTORIAL_FLAGS, OnTutorialFlags, 0);
}

void __fastcall CGTutorial::ShutdownGame() {
  ClientServices_ClearMessageHandler(SMSG_TUTORIAL_FLAGS);
}

void __fastcall CGTutorial::TriggerTutorial(TUTORIAL tutorial) {
}

void __fastcall CGTutorial::ClearTutorials() {
  m_tutorialFlags.SetAll();

  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_TUTORIAL_CLEAR));
  msg.Finalize();
  ClientServices_Send(&msg);
}

void __fastcall CGTutorial::ResetTutorials() {
  m_tutorialFlags.ClearAll();

  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_TUTORIAL_RESET));
  msg.Finalize();
  ClientServices_Send(&msg);
}

int __fastcall CGTutorial::OnTutorialFlags(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  unsigned int byteCount = msg->Size() - msg->Tell();
  void        *data;

  msg->GetDataInSitu(data, byteCount);
  m_tutorialFlags.SetData(data, byteCount);
  return 1;
}

static int __fastcall Script_TriggerTutorial(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: TriggerTutorial(\"tutorial\")");
  }

  const char  *token = lua_tostring(L, 1);
  unsigned int tutorial;
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

static int __fastcall Script_ClearTutorials(lua_State *L) {
  CGTutorial::ClearTutorials();
  return 0;
}

static int __fastcall Script_ResetTutorials(lua_State *L) {
  CGTutorial::ResetTutorials();
  return 0;
}

static FrameScript_Method s_ScriptFunctions[3] = {
    {"TriggerTutorial", Script_TriggerTutorial},
    { "ClearTutorials",  Script_ClearTutorials},
    { "ResetTutorials",  Script_ResetTutorials}
};

void __fastcall TutorialRegisterScriptFunctions() {
  for (unsigned int i = 0; i < 3; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void __fastcall TutorialUnregisterScriptFunctions() {
  for (unsigned int i = 0; i < 3; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
