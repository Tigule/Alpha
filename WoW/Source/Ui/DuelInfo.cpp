#include "WowSvcs/WowSvcsClient/ClientServices.h"
#include "ChatFrame.h"
#include "GameUI.h"
#include "SpellBookFrame.h"
#include "Object/ObjectClient/Object_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"

#include <Base/CDataStore.h>
#include <FrameScript/FrameScript.h>

#include <lauxlib.h>
#include <lua.h>

class CGItem_C;
unsigned __int64 __fastcall Script_GetGUIDFromName(const char *name);
bool __fastcall             Spell_C_CastSpell(int spellID, const CGItem_C *item);

class CGDuelInfo {
 public:
  static void __fastcall InitializeGame();
  static void __fastcall ShutdownGame();
  static void __fastcall StartDuel();
  static void __fastcall AcceptDuel();
  static void __fastcall CancelDuel();

 protected:
  static int __fastcall OnDuelRequested(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
  static int __fastcall OnDuelOutOfBounds(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
  static int __fastcall OnDuelInBounds(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
  static int __fastcall OnDuelComplete(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
  static int __fastcall OnDuelWinner(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);

  static unsigned __int64 m_arbiter;
};

unsigned __int64 CGDuelInfo::m_arbiter;

void __fastcall CGDuelInfo::InitializeGame() {
  ClientServices_SetMessageHandler(SMSG_DUEL_REQUESTED, OnDuelRequested, 0);
  ClientServices_SetMessageHandler(SMSG_DUEL_OUTOFBOUNDS, OnDuelOutOfBounds, 0);
  ClientServices_SetMessageHandler(SMSG_DUEL_INBOUNDS, OnDuelInBounds, 0);
  ClientServices_SetMessageHandler(SMSG_DUEL_COMPLETE, OnDuelComplete, 0);
  ClientServices_SetMessageHandler(SMSG_DUEL_WINNER, OnDuelWinner, 0);
}

void __fastcall CGDuelInfo::ShutdownGame() {
  ClientServices_ClearMessageHandler(SMSG_DUEL_REQUESTED);
  ClientServices_ClearMessageHandler(SMSG_DUEL_OUTOFBOUNDS);
  ClientServices_ClearMessageHandler(SMSG_DUEL_INBOUNDS);
  ClientServices_ClearMessageHandler(SMSG_DUEL_COMPLETE);
  ClientServices_ClearMessageHandler(SMSG_DUEL_WINNER);
}

void __fastcall CGDuelInfo::StartDuel() {
  Spell_C_CastSpell(CGSpellBook::GetDuelSpell(), 0);
}

void __fastcall CGDuelInfo::AcceptDuel() {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_DUEL_ACCEPTED));
  msg.Finalize();
  ClientServices_Send(&msg);
}

void __fastcall CGDuelInfo::CancelDuel() {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_DUEL_CANCELLED));
  msg.Finalize();
  ClientServices_Send(&msg);
}

int __fastcall CGDuelInfo::OnDuelRequested(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  unsigned __int64 requestedBy;
  msg->Get(m_arbiter);
  msg->Get(requestedBy);
  FrameScript_SignalEvent(363);
  return 1;
}

int __fastcall CGDuelInfo::OnDuelOutOfBounds(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  FrameScript_SignalEvent(364);
  return 1;
}

int __fastcall CGDuelInfo::OnDuelInBounds(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  FrameScript_SignalEvent(365);
  return 1;
}

int __fastcall CGDuelInfo::OnDuelComplete(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  unsigned int started;
  msg->Get(reinterpret_cast<unsigned char &>(started));
  m_arbiter = 0;
  FrameScript_SignalEvent(366);
  return 1;
}

int __fastcall CGDuelInfo::OnDuelWinner(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  unsigned int fled;
  char         message[1024];
  char         beaten[48];
  char         winner[48];
  msg->Get(reinterpret_cast<unsigned char &>(fled));
  msg->GetString(winner, sizeof(winner));
  msg->GetString(beaten, sizeof(beaten));
  const char *format = FrameScript_GetText(fled ? "DUEL_WINNER_RETREAT" : "DUEL_WINNER_KNOCKOUT", -1, GENDER_NOT_APPLICABLE);
  SStrPrintf(message, sizeof(message), format, winner, beaten);
  CGChat::AddChatMessage(message, static_cast<SLASH_COMMAND_ID>(9), 0, 0, 0, 0, 0);
  return 1;
}

static int __fastcall Script_StartDuel(lua_State *L) {
  CGDuelInfo::StartDuel();
  if (lua_isstring(L, 1)) {
    unsigned __int64 guid = CGGameUI::ClosestObjectMatch(lua_tostring(L, 1), TYPE_UNIT);
    CGObject_C      *object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
    if (object) {
      object->OnRightClick();
    }
  }
  return 0;
}

static int __fastcall Script_StartDuelUnit(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: StartDuelUnit(\"unit\")");
  }
  CGDuelInfo::StartDuel();
  unsigned __int64 guid = Script_GetGUIDFromName(lua_tostring(L, 1));
  CGObject_C      *object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
  if (object) {
    object->OnRightClick();
  }
  return 0;
}

static int __fastcall Script_AcceptDuel(lua_State *L) {
  CGDuelInfo::AcceptDuel();
  return 0;
}

static int __fastcall Script_CancelDuel(lua_State *L) {
  CGDuelInfo::CancelDuel();
  return 0;
}

static FrameScript_Method s_ScriptFunctions[4] = {
    {    "StartDuel",     Script_StartDuel},
    {"StartDuelUnit", Script_StartDuelUnit},
    {   "AcceptDuel",    Script_AcceptDuel},
    {   "CancelDuel",    Script_CancelDuel}
};

void __fastcall DuelInfoRegisterScriptFunctions() {
  for (unsigned int i = 0; i < 4; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void __fastcall DuelInfoUnregisterScriptFunctions() {
  for (unsigned int i = 0; i < 4; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
