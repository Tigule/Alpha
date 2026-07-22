#include "Glue/GlueScriptEvents.h"

#include <FrameScript/FrameScript.h>
#include <Services/AsyncFileRead.h>

#include "Glue/CGlueMgr.h"
#include "SoundInterface/SoundInterface.h"
#include "WowSvcs/WowSvcsClient/ClientConnection.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <lauxlib.h>
#include <lua.h>
#include <storm.h>

int __fastcall OsLaunchURL(const char *url);

static const char REGKEY[11] = "WoW\\Client";
static const char REGVAL_ACCOUNTNAME[12] = "AccountName";

static int __fastcall Script_GetBuildInfo(lua_State *L);
static int __fastcall Script_SetCurrentScreen(lua_State *L);
static int __fastcall Script_QuitGame(lua_State *__formal);
static int __fastcall Script_PlayGlueMusic(lua_State *L);
static int __fastcall Script_LaunchURL(lua_State *L);
static int __fastcall Script_LaunchAccountCreate(lua_State *__formal);
static int __fastcall Script_GetLastAccountName(lua_State *L);
static int __fastcall Script_DefaultServerLogin(lua_State *L);
static int __fastcall Script_StatusDialogClick(lua_State *__formal);
static int __fastcall Script_GetServerName(lua_State *L);
static int __fastcall Script_DisconnectFromServer(lua_State *__formal);
static int __fastcall Script_IsConnectedToServer(lua_State *L);
static int __fastcall Script_GetRealmList(lua_State *__formal);
static int __fastcall Script_GetNumRealms(lua_State *L);
static int __fastcall Script_GetRealmInfo(lua_State *L);
static int __fastcall Script_ChangeRealm(lua_State *L);
static int __fastcall Script_EnterWorld(lua_State *__formal);

static const FrameScript_Method s_ScriptFunctions[17] = {
    {        "GetBuildInfo",         Script_GetBuildInfo},
    {    "SetCurrentScreen",     Script_SetCurrentScreen},
    {            "QuitGame",             Script_QuitGame},
    {       "PlayGlueMusic",        Script_PlayGlueMusic},
    {           "LaunchURL",            Script_LaunchURL},
    { "LaunchAccountCreate",  Script_LaunchAccountCreate},
    {  "GetLastAccountName",   Script_GetLastAccountName},
    {  "DefaultServerLogin",   Script_DefaultServerLogin},
    {   "StatusDialogClick",    Script_StatusDialogClick},
    {       "GetServerName",        Script_GetServerName},
    {"DisconnectFromServer", Script_DisconnectFromServer},
    { "IsConnectedToServer",  Script_IsConnectedToServer},
    {        "GetRealmList",         Script_GetRealmList},
    {        "GetNumRealms",         Script_GetNumRealms},
    {        "GetRealmInfo",         Script_GetRealmInfo},
    {         "ChangeRealm",          Script_ChangeRealm},
    {          "EnterWorld",           Script_EnterWorld}
};

const char *g_glueScriptEvents[11];

void __fastcall GlueScriptEventsInitialize() {
  g_glueScriptEvents[0] = "SET_GLUE_SCREEN";
  g_glueScriptEvents[1] = "START_GLUE_MUSIC";
  g_glueScriptEvents[2] = "DISCONNECTED_FROM_SERVER";
  g_glueScriptEvents[3] = "OPEN_STATUS_DIALOG";
  g_glueScriptEvents[4] = "UPDATE_STATUS_DIALOG";
  g_glueScriptEvents[5] = "CLOSE_STATUS_DIALOG";
  g_glueScriptEvents[6] = "CHARACTER_LIST_UPDATE";
  g_glueScriptEvents[7] = "UPDATE_SELECTED_CHARACTER";
  g_glueScriptEvents[8] = "OPEN_REALM_LIST";
  g_glueScriptEvents[9] = "UPDATE_SELECTED_RACE";
  g_glueScriptEvents[10] = "SELECT_LAST_CHARACTER";
}

static int __fastcall Script_GetBuildInfo(lua_State *L) {
  lua_pushstring(L, FrameScript_GetText("ALPHA_BUILD", -1, GENDER_NOT_APPLICABLE));
  lua_pushstring(L, FrameScript_GetText("ASSERTIONS_ENABLED_BUILD", -1, GENDER_NOT_APPLICABLE));
  lua_pushstring(L, "5.3");
  lua_pushstring(L, "3368");
  lua_pushstring(L, "Dec 11 2003");
  return 5;
}

static int __fastcall Script_SetCurrentScreen(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: SetCurrentScreen(\"screen\")");
    return 0;
  }

  CGlueMgr::UpdateCurrentScreen(lua_tostring(L, 1));
  AsyncFileReadWaitAll();
  return 0;
}

static int __fastcall Script_QuitGame(lua_State *__formal) {
  CGlueMgr::QuitGame();
  return 0;
}

static int __fastcall Script_PlayGlueMusic(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: PlayGlueMusic(\"filename\")");
    return 0;
  }

  SndInterfaceSetGlueMusic(lua_tostring(L, 1));
  return 0;
}

static int __fastcall Script_LaunchURL(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: LaunchURL(\"URL\")");
    return 0;
  }

  OsLaunchURL(lua_tostring(L, 1));
  return 0;
}

static int __fastcall Script_LaunchAccountCreate(lua_State *__formal) {
  OsLaunchURL(FrameScript_GetText("ACCOUNT_CREATE_URL", -1, GENDER_NOT_APPLICABLE));
  return 0;
}

static int __fastcall Script_GetLastAccountName(lua_State *L) {
  char accountName[64] = "";

  SRegLoadString(REGKEY, REGVAL_ACCOUNTNAME, 0, accountName, sizeof(accountName));
  lua_pushstring(L, accountName);
  return 1;
}

static int __fastcall Script_DefaultServerLogin(lua_State *L) {
  if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
    luaL_error(L, "Usage: DefaultServerLogin(\"accountName\", \"password\")");
    return 0;
  }

  CGlueMgr::DefaultServerLogin();
  return 0;
}

static int __fastcall Script_StatusDialogClick(lua_State *__formal) {
  CGlueMgr::StatusDialogClick();
  return 0;
}

static int __fastcall Script_GetServerName(lua_State *L) {
  lua_pushstring(L, ClientServices_GetSelectedRealmName());
  return 1;
}

static int __fastcall Script_DisconnectFromServer(lua_State *__formal) {
  if (ClientServices_IsConnected()) {
    CGlueMgr::ExpectDisconnect(0);
    ClientServices_Disconnect();
  }

  return 0;
}

static int __fastcall Script_IsConnectedToServer(lua_State *L) {
  if (ClientServices_IsConnected()) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }

  return 1;
}

static int __fastcall Script_GetRealmList(lua_State *__formal) {
  CGlueMgr::GetRealmList();
  return 0;
}

static int __fastcall Script_GetNumRealms(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(ClientServices_GetRealmListCount()));
  return 1;
}

static int __fastcall Script_GetRealmInfo(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetRealmInfo(index)");
    return 0;
  }

  int               index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  const REALM_INFO *realm = ClientServices_GetRealmInfoByIndex(index);
  if (realm) {
    lua_pushstring(L, realm->name);
    lua_pushnumber(L, static_cast<double>(realm->players));

    if (!SStrCmpI(realm->name, ClientServices_GetSelectedRealmName(), 0x7FFFFFFF)) {
      lua_pushnumber(L, 1.0);
    } else {
      lua_pushnil(L);
    }
  } else {
    lua_pushnil(L);
    lua_pushnumber(L, 0.0);
    lua_pushnil(L);
  }

  return 3;
}

static int __fastcall Script_ChangeRealm(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: ChangeRealm(index)");
    return 0;
  }

  int               index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  const REALM_INFO *realm = ClientServices_GetRealmInfoByIndex(index);
  if (!realm) {
    luaL_error(L, "Bad realm index in ChangeRealm");
    return 0;
  }

  if (SStrCmpI(realm->address, ClientServices_GetSelectedRealmAddress(), 0x7FFFFFFF)) {
    CGlueMgr::ChangeRealm(realm);
  }

  return 0;
}

static int __fastcall Script_EnterWorld(lua_State *__formal) {
  CGlueMgr::EnterWorld();
  return 0;
}

void __fastcall GlueScriptEventsRegisterFunctions() {
  for (unsigned int i = 0; i < 17; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void __fastcall GlueScriptEventsUnregisterFunctions() {
  for (unsigned int i = 0; i < 17; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
