#include "WowSvcs/WowSvcsClient/FriendList.h"

#include "Console/ConsoleCommand.h"
#include "Console/ConsoleClient.h"
#include "DB/DBClient/AutoCode/AreaTableRec.h"
#include "DB/DBClient/AutoCode/ChrClassesRec.h"
#include "DB/DBClient/AutoCode/ChrRacesRec.h"
#include "DB/WowLocale.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"
#include "Ui/GameUI.h"
#include "Ui/ChatFrame.h"

#include <Base/CDataStore.h>
#include <FrameScript/FrameScript.h>

extern FrameScript_Method s_FriendListScriptFunctions[19];

#include <ctype.h>
#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <string.h>

FriendList *g_friendList;

enum PARTY_STATUS {
  PARTY_STATUS_NOT_IN_PARTY = 0,
  PARTY_STATUS_IN_PARTY = 1,
  PARTY_STATUS_LFG = 2
};

struct WhoListEntry {
  char         name[48];
  char         guild[96];
  int          level;
  int          raceID;
  int          classID;
  int          areaID;
  PARTY_STATUS partyStatus;
};

static WhoListEntry s_whoList[50];
static unsigned int s_numWhos;
static unsigned int s_totalNumWhos;
static int          s_whoToUI;

enum WHO_SORT_TYPE {
  WHO_SORT_ZONE = 0,
  WHO_SORT_LEVEL = 1,
  WHO_SORT_CLASS = 2,
  WHO_SORT_GROUP = 3,
  WHO_SORT_NAME = 4,
  WHO_SORT_RACE = 5,
  WHO_SORT_GUILD = 6,
  NUM_WHO_SORT_TYPES = 7
};

struct WhoSortType {
  WHO_SORT_TYPE type;
  int           reverse;
};

static WhoSortType s_whoSortCriteria[NUM_WHO_SORT_TYPES];

FriendList::FriendList() : m_friendNamesPending(0), m_selectedFriend(0), m_ignoreNamesPending(0), m_selectedIgnore(0) {
  memset(m_friends, 0, sizeof(m_friends));
  memset(m_ignore, 0, sizeof(m_ignore));
}

static int FriendListStatusHandler(void*, NETMESSAGE, unsigned long, CDataStore* msg) {
  unsigned char result;
  unsigned __int64 guid;
  msg->Get(result);
  msg->Get(guid);
  if (g_friendList) {
    g_friendList->HandleStatus(static_cast<FRIEND_RESULT>(result), guid, msg);
  }
  return 1;
}

static void FriendListNameCallbackWithSort(int id, const unsigned __int64& guid, void* arg, bool granted) {
  GAME_ERROR_TYPE error = static_cast<GAME_ERROR_TYPE>(reinterpret_cast<unsigned int>(arg));
  if (granted) {
    unsigned __int64 cacheGuid = 0;
    const NameCache *name = g_nameDBCache.GetRecord(guid, cacheGuid, 0, 0);
    FATALASSERT(name);
    if (g_friendList) {
      g_friendList->SetName(guid, name->m_name);
      g_friendList->DecrementPendingFriendName();
    }
    if (error != GERR_NONE) {
      CGGameUI::DisplayError(error, name->m_name);
    }
  } else {
    if (error != GERR_NONE) {
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(290));
    }
    if (g_friendList) {
      g_friendList->SetName(guid, FrameScript_GetText("UNKNOWN", -1, GENDER_NOT_APPLICABLE));
      g_friendList->DecrementPendingFriendName();
    }
  }
}

static void IgnoreListNameCallback(int id, const unsigned __int64& guid, void* arg, bool granted) {
  GAME_ERROR_TYPE error = static_cast<GAME_ERROR_TYPE>(reinterpret_cast<unsigned int>(arg));
  if (granted) {
    unsigned __int64 cacheGuid = 0;
    const NameCache *name = g_nameDBCache.GetRecord(guid, cacheGuid, 0, 0);
    FATALASSERT(name);
    if (g_friendList) {
      g_friendList->DecrementPendingIgnoreName();
    }
    if (error != GERR_NONE) {
      CGGameUI::DisplayError(error, name->m_name);
    }
  } else {
    if (error != GERR_NONE) {
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(290));
    }
    if (g_friendList) {
      g_friendList->DelIgnore(guid);
      g_friendList->DecrementPendingIgnoreName();
    }
  }
}

FriendList::~FriendList() {
}

static int FriendListHandler(void*, NETMESSAGE, unsigned long, CDataStore* msg) {
  if (g_friendList) {
    g_friendList->AddFriends(msg);
  }
  return 1;
}

static int CCommand_Friends(const char* command, const char* arguments) {
  CDataStore msg;
  msg.Put(102);
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_AddFriend(const char* command, const char* arguments) {
  g_friendList->AddFriend(arguments);
  return 1;
}

static int CCommand_RemoveFriend(const char* command, const char* arguments) {
  g_friendList->RemoveFriend(arguments);
  return 1;
}

static int WhoisResponseHandler(void*, NETMESSAGE, unsigned long, CDataStore* msg) {
  char name[256];
  msg->GetString(name, 0x7FFFFFFF);
  ConsoleWrite(name, DEFAULT_COLOR);
  return 1;
}

void FriendList::RemoveFriend(const char *name) {
  for (unsigned int i = 0; i < 50; ++i) {
    if (m_friends[i].m_name &&
        !SStrCmpI(m_friends[i].m_name, name, 0x7FFFFFFF)) {
      RemoveFriend(m_friends[i].guid);
      return;
    }
  }
  CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(0xEE));
}

void FriendList::RemoveFriend(unsigned __int64 guid) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_DEL_FRIEND));
  msg.Put(guid);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void FriendList::RemoveFriend(unsigned int index) {
  const Friend *entry = GetFriend(index);
  if (!entry) {
    return;
  }
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_DEL_FRIEND));
  msg.Put(entry->guid);
  msg.Finalize();
  ClientServices_Send(&msg);
}

static int ReverseWhoisResponseHandler(void*, NETMESSAGE, unsigned long, CDataStore* msg) {
  unsigned int numAccounts = 0;
  msg->Get(numAccounts);
  if (numAccounts == static_cast<unsigned int>(-1)) {
    ConsoleWriteA("RWhoIs failed\n", ERROR_COLOR);
    return 1;
  }
  if (!numAccounts) {
    ConsoleWriteA("Not found\n", WARNING_COLOR);
    return 1;
  }

  for (unsigned int account = 0; account < numAccounts; ++account) {
    char accountName[64];
    msg->GetString(accountName, 0x7FFFFFFF);
    ConsolePrintf("Account: %s\n", accountName);

    int          numCharacters = 0;
    unsigned int selected = 0;
    msg->Get(numCharacters);
    msg->Get(selected);
    for (int character = 0; character < numCharacters; ++character) {
      char characterName[48];
      msg->GetString(characterName, 0x7FFFFFFF);
      ConsoleWriteA(
          "  %s\n",
          character == static_cast<int>(selected)
              ? WARNING_COLOR
              : DEFAULT_COLOR,
          characterName);
    }
  }
  return 1;
}

static int CCommand_Whois(const char*, const char* args) {
  CDataStore msg;
  msg.Put(100);
  msg.PutString(args);
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_RWhois(const char*, const char* args) {
  CDataStore msg;
  msg.Put(494);
  msg.PutString(args);
  ClientServices_Send(&msg);
  return 1;
}

static int Script_GetNumFriends(lua_State *L) {
  lua_pushnumber(L, g_friendList ? g_friendList->GetNumFriends() : 0);
  return 1;
}

static int Script_GetFriendInfo(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetFriendInfo(index)");
  }
  const FriendList::Friend *entry = g_friendList ? g_friendList->GetFriend(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1) : 0;
  if (!entry) {
    return 0;
  }
  lua_pushstring(L, entry->m_name ? entry->m_name : "");
  lua_pushnumber(L, entry->m_level);
  const ChrClassesRec *classRec = g_chrClassesDB.GetRecord(entry->m_class);
  lua_pushstring(L, classRec ? classRec->m_name_lang[CURRENT_LANGUAGE] : "");
  const AreaTableRec *areaRec = g_areaTableDB.GetRecord(entry->m_area);
  lua_pushstring(L, areaRec ? areaRec->m_AreaName_lang[CURRENT_LANGUAGE] : "");
  lua_pushnumber(L, entry->m_connected != 0);
  lua_pushstring(L, "");
  return 6;
}

static int Script_SetSelectedFriend(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: SetSelectedFriend(index)");
  }
  if (g_friendList) {
    g_friendList->SetFriendSelectionIndex(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  }
  return 0;
}

static int Script_GetSelectedFriend(lua_State *L) {
  lua_pushnumber(L, g_friendList ? g_friendList->GetFriendSelectionIndex() + 1 : 0);
  return 1;
}

void FriendList::DelIgnore(const char *name) {
  if (!name || !*name) {
    return;
  }
  for (unsigned int i = 0; i < GetNumIgnores(); ++i) {
    unsigned __int64 noGuid = 0;
    const NameCache *entry = g_nameDBCache.GetRecord(m_ignore[i], noGuid, 0, 0);
    if (entry && !SStrCmpI(entry->m_name, name, 0x7FFFFFFF)) {
      CDataStore msg;
      msg.Put(static_cast<unsigned int>(CMSG_DEL_IGNORE));
      msg.Put(m_ignore[i]);
      msg.Finalize();
      ClientServices_Send(&msg);
      return;
    }
  }
}

static int Script_AddFriend(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: AddFriend(\"name\")");
  }
  if (g_friendList) {
    g_friendList->AddFriend(lua_tostring(L, 1));
  }
  return 0;
}

static int Script_RemoveFriend(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: RemoveFriend(index)");
  }
  if (g_friendList) {
    g_friendList->RemoveFriend(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  }
  return 0;
}

static int Script_ShowFriends(lua_State *L) {
  if (g_friendList) {
    g_friendList->ShowFriends();
  }
  return 0;
}

static int Script_SendWho(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: SendWho(\"filter\")");
  }
  if (g_friendList) {
    g_friendList->SendWho(lua_tostring(L, 1));
  }
  return 0;
}

static int Script_GetNumIgnores(lua_State *L) {
  lua_pushnumber(L, g_friendList ? g_friendList->GetNumIgnores() : 0);
  return 1;
}

static int Script_GetIgnoreName(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetIgnoreName(index)");
  }
  unsigned __int64 guid = g_friendList->GetIgnore(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  unsigned __int64 noGuid = 0;
  const NameCache *entry = g_nameDBCache.GetRecord(guid, noGuid, 0, 0);
  lua_pushstring(L, entry ? entry->m_name : FrameScript_GetText("UNKNOWN", -1, GENDER_NOT_APPLICABLE));
  return 1;
}

static int Script_SetSelectedIgnore(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: SetSelectedIgnore(index)");
  }
  if (g_friendList) {
    g_friendList->SetIgnoreSelectionIndex(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  }
  return 0;
}

static int Script_GetSelectedIgnore(lua_State *L) {
  lua_pushnumber(L, g_friendList ? g_friendList->GetIgnoreSelectionIndex() + 1 : 0);
  return 1;
}

static int Script_AddOrDelIgnore(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: AddOrDelIgnore(\"name\")");
  }
  if (g_friendList) {
    g_friendList->AddOrDelIgnore(lua_tostring(L, 1));
  }
  return 0;
}

static int Script_AddIgnore(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: AddIgnore(\"name\")");
  }
  if (g_friendList) {
    g_friendList->AddIgnore(lua_tostring(L, 1));
  }
  return 0;
}

static int Script_DelIgnore(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: DelIgnore(\"name\")");
  }
  if (g_friendList) {
    g_friendList->DelIgnore(lua_tostring(L, 1));
  }
  return 0;
}

static int Script_GetNumWhoResults(lua_State *L) {
  lua_pushnumber(L, s_numWhos);
  lua_pushnumber(L, s_totalNumWhos);
  return 2;
}

static int Script_GetWhoInfo(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetWhoInfo(index)");
  }
  unsigned int index = static_cast<unsigned int>(lua_tonumber(L, 1)) - 1;
  if (index >= s_numWhos) {
    return 0;
  }
  WhoListEntry &entry = s_whoList[index];
  lua_pushstring(L, entry.name);
  lua_pushstring(L, entry.guild);
  lua_pushnumber(L, entry.level);
  const ChrRacesRec *race = g_chrRacesDB.GetRecord(entry.raceID);
  lua_pushstring(L, race ? race->m_name_lang[CURRENT_LANGUAGE] : "");
  const ChrClassesRec *playerClass = g_chrClassesDB.GetRecord(entry.classID);
  lua_pushstring(L, playerClass ? playerClass->m_name_lang[CURRENT_LANGUAGE] : "");
  const AreaTableRec *area = g_areaTableDB.GetRecord(entry.areaID);
  lua_pushstring(L, area ? area->m_AreaName_lang[CURRENT_LANGUAGE] : "");
  lua_pushstring(L, entry.partyStatus == 0 ? "" : (entry.partyStatus == 1 ? "LFG" : "FULL"));
  return 7;
}

static int Script_SetWhoToUI(lua_State *L) {
  s_whoToUI = lua_toboolean(L, 1);
  return 0;
}

static int __cdecl QSortWho(const void *a, const void *b) {
  const WhoListEntry *entry1 = static_cast<const WhoListEntry *>(a);
  const WhoListEntry *entry2 = static_cast<const WhoListEntry *>(b);
  return SStrCmpI(entry1->name, entry2->name, 0x7FFFFFFF);
}

static int Script_SortWho(lua_State *L) {
  qsort(s_whoList, s_numWhos, sizeof(WhoListEntry), QSortWho);
  return 0;
}

void FriendList::RegisterScriptFunctions() {
  for (int i = 0; i < 19; ++i) {
    FrameScript_RegisterFunction(s_FriendListScriptFunctions[i].name, s_FriendListScriptFunctions[i].method);
  }
}

void FriendList::UnregisterScriptFunctions() {
  for (int i = 0; i < 19; ++i) {
    FrameScript_UnregisterFunction(s_FriendListScriptFunctions[i].name);
  }
}

static void PrintWho(const char* name, const char* guild, int level, int classID, int raceID, int areaID) {
  const ChrRacesRec *race = g_chrRacesDB.GetRecord(raceID);
  const ChrClassesRec *playerClass = g_chrClassesDB.GetRecord(classID);
  const AreaTableRec *area = g_areaTableDB.GetRecord(areaID);
  const char *unknown = FrameScript_GetText("UNKNOWN", -1, GENDER_NOT_APPLICABLE);
  const char *raceName = race ? race->m_name_lang[CURRENT_LANGUAGE] : unknown;
  const char *className = playerClass ? playerClass->m_name_lang[CURRENT_LANGUAGE] : unknown;
  const char *areaName = area ? area->m_AreaName_lang[CURRENT_LANGUAGE] : unknown;
  const char *format = FrameScript_GetText(
      guild && *guild ? "WHO_LIST_GUILD_FORMAT" : "WHO_LIST_FORMAT",
      -1, GENDER_NOT_APPLICABLE);
  char line[256];
  if (guild && *guild) {
    SStrPrintf(line, sizeof(line), format, name, level, raceName, className, guild, areaName);
  } else {
    SStrPrintf(line, sizeof(line), format, name, level, raceName, className, areaName);
  }
  CGChat::AddChatMessage(line, static_cast<SLASH_COMMAND_ID>(1), 0, 0, 0, 0, 0);
}

static int OnWhoList(void*, NETMESSAGE msgId, unsigned long eventTime, CDataStore* msg) {
  FATALASSERT(msg);
  unsigned int count;
  msg->Get(count);
  msg->Get(s_totalNumWhos);
  s_numWhos = min(count, 50U);
  int toChat = !s_whoToUI && count <= 3;

  for (unsigned int i = 0; i < count; ++i) {
    WhoListEntry entry;
    memset(&entry, 0, sizeof(entry));
    msg->GetString(entry.name, sizeof(entry.name));
    msg->GetString(entry.guild, sizeof(entry.guild));
    msg->Get(entry.level);
    msg->Get(entry.classID);
    msg->Get(entry.raceID);
    msg->Get(entry.areaID);
    int partyStatus = 0;
    msg->Get(partyStatus);
    entry.partyStatus = static_cast<PARTY_STATUS>(partyStatus);
    if (i < 50) {
      s_whoList[i] = entry;
    }
    if (toChat) {
      PrintWho(entry.name, entry.guild, entry.level, entry.classID, entry.raceID, entry.areaID);
    }
  }

  qsort(s_whoList, s_numWhos, sizeof(WhoListEntry), QSortWho);
  if (toChat) {
    char line[256];
    const char *format = FrameScript_GetText("WHO_NUM_RESULTS", count, GENDER_NOT_APPLICABLE);
    SStrPrintf(line, sizeof(line), format, count);
    CGChat::AddChatMessage(line, static_cast<SLASH_COMMAND_ID>(1), 0, 0, 0, 0, 0);
  } else {
    FrameScript_SignalEvent(371);
  }
  return 1;
}

static int OnIgnoreList(void*, NETMESSAGE, unsigned long, CDataStore* msg) {
  if (g_friendList) {
    g_friendList->IgnoreList(msg);
  }
  return 1;
}

void FriendList::Initialize() {
  if (g_friendList) {
    return;
  }

  g_friendList = NEW(FriendList);

  ClientServices_SetMessageHandler(SMSG_WHO, OnWhoList, 0);
  ClientServices_SetMessageHandler(SMSG_WHOIS, WhoisResponseHandler, 0);
  ClientServices_SetMessageHandler(SMSG_RWHOIS, ReverseWhoisResponseHandler, 0);
  ClientServices_SetMessageHandler(SMSG_FRIEND_LIST, FriendListHandler, 0);
  ClientServices_SetMessageHandler(SMSG_FRIEND_STATUS, FriendListStatusHandler, 0);
  ClientServices_SetMessageHandler(SMSG_IGNORE_LIST, OnIgnoreList, 0);

  ConsoleCommandRegister("friends", CCommand_Friends, GAME, 0);
  ConsoleCommandRegister("addfriend", CCommand_AddFriend, GAME, 0);
  ConsoleCommandRegister("removefriend", CCommand_RemoveFriend, GAME, 0);
  ConsoleCommandRegister("whois", CCommand_Whois, DEBUG, "Ask the server to do an account/real name lookup on a character name");
  ConsoleCommandRegister("rwhois", CCommand_RWhois, DEBUG, "Ask the server to do an reverse lookup on an account's real name");

  for (int i = WHO_SORT_ZONE; i < NUM_WHO_SORT_TYPES; ++i) {
    s_whoSortCriteria[i].type = static_cast<WHO_SORT_TYPE>(i);
    s_whoSortCriteria[i].reverse = 0;
  }
}

void FriendList::Destroy() {
  if (g_friendList) {
    ClientServices_ClearMessageHandler(SMSG_WHO);
    ClientServices_ClearMessageHandler(SMSG_WHOIS);
    ClientServices_ClearMessageHandler(SMSG_RWHOIS);
    ClientServices_ClearMessageHandler(SMSG_FRIEND_LIST);
    ClientServices_ClearMessageHandler(SMSG_FRIEND_STATUS);
    ClientServices_ClearMessageHandler(SMSG_IGNORE_LIST);

    ConsoleCommandUnregister("whois");
    ConsoleCommandUnregister("rwhois");
    ConsoleCommandUnregister("friends");
    ConsoleCommandUnregister("addfriend");
    ConsoleCommandUnregister("removefriend");

    delete g_friendList;
    g_friendList = 0;
  }
}

unsigned int FriendList::GetNumFriends() {
  unsigned int count = 0;
  while (count < 50 && m_friends[count].guid) {
    ++count;
  }
  return count;
}

const FriendList::Friend *FriendList::GetFriend(unsigned int index) {
  return index < GetNumFriends() ? &m_friends[index] : 0;
}

void FriendList::SetFriendSelectionIndex(unsigned int index) {
  const Friend *entry = GetFriend(index);
  m_selectedFriend = entry ? entry->guid : 0;
}

int FriendList::GetFriendSelectionIndex() {
  for (unsigned int i = 0; i < GetNumFriends(); ++i) {
    if (m_friends[i].guid == m_selectedFriend) {
      return i;
    }
  }
  return -1;
}

unsigned int FriendList::GetNumIgnores() {
  unsigned int count = 0;
  while (count < 25 && m_ignore[count]) {
    ++count;
  }
  return count;
}

unsigned __int64 FriendList::GetIgnore(unsigned int index) {
  return index < GetNumIgnores() ? m_ignore[index] : 0;
}

void FriendList::SetIgnoreSelectionIndex(unsigned int index) {
  m_selectedIgnore = GetIgnore(index);
}

int FriendList::GetIgnoreSelectionIndex() {
  for (unsigned int i = 0; i < GetNumIgnores(); ++i) {
    if (m_ignore[i] == m_selectedIgnore) {
      return i;
    }
  }
  return -1;
}

void FriendList::ShowFriends() {
  char text[256];
  for (unsigned int i = 0; i < 50; ++i) {
    if (m_friends[i].m_name) {
      SStrPrintf(
          text,
          sizeof(text),
          m_friends[i].m_connected ? "%s - Online" : "%s - Offline",
          m_friends[i].m_name);
      CGChat::AddChatMessage(text, static_cast<SLASH_COMMAND_ID>(9), 0, 0, 0, 0, 0);
    }
  }
}

void FriendList::AddFriend(const char *name) {
  if (!name || !*name) {
    return;
  }
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_ADD_FRIEND));
  msg.PutString(name);
  msg.Finalize();
  ClientServices_Send(&msg);
}

static int __cdecl QSortFriends(const void* a, const void* b) {
  FATALASSERT(a);
  FATALASSERT(b);
  const FriendList::Friend *left = static_cast<const FriendList::Friend *>(a);
  const FriendList::Friend *right = static_cast<const FriendList::Friend *>(b);
  if (!left->guid && !right->guid) return 0;
  if (!left->guid) return 1;
  if (!right->guid) return -1;
  if (left->m_connected != right->m_connected) {
    return left->m_connected ? -1 : 1;
  }
  if (left->m_name && right->m_name) {
    return SStrCmpI(left->m_name, right->m_name, 0x7FFFFFFF);
  }
  if (!left->m_name && !right->m_name) return 0;
  return left->m_name ? -1 : 1;
}

static int __cdecl QSortIgnore(const void* a, const void* b) {
  FATALASSERT(a);
  FATALASSERT(b);
  unsigned __int64 left = *static_cast<const unsigned __int64 *>(a);
  unsigned __int64 right = *static_cast<const unsigned __int64 *>(b);
  if (!left) return right != 0;
  if (!right) return -1;
  unsigned __int64 noGuid = 0;
  const NameCache *leftName = g_nameDBCache.GetRecord(left, noGuid, 0, 0);
  const NameCache *rightName = g_nameDBCache.GetRecord(right, noGuid, 0, 0);
  if (leftName && rightName) return SStrCmpI(leftName->m_name, rightName->m_name, 0x7FFFFFFF);
  if (!leftName && !rightName) return 0;
  return leftName ? -1 : 1;
}

void FriendList::SortFriends() {
  qsort(m_friends, 50, sizeof(m_friends[0]), QSortFriends);
  FrameScript_SignalEvent(330);
}

void FriendList::SortIgnore() {
  qsort(m_ignore, 25, sizeof(m_ignore[0]), QSortIgnore);
  FrameScript_SignalEvent(331);
}

void FriendList::Removed(unsigned __int64 guid) {
  for (unsigned int i = 0; i < 50; ++i) {
    if (m_friends[i].guid == guid) {
      m_friends[i].guid = 0;
      FREEIFUSED(m_friends[i].m_name);
      return;
    }
  }
}

int FriendList::Added(unsigned __int64 guid) {
  unsigned int freeIndex = -1;
  unsigned int i = 0;

  for (; i < 50; ++i) {
    if (m_friends[i].guid == guid) {
      return i;
    }
    if (!m_friends[i].guid) {
      freeIndex = i;
      break;
    }
  }

  m_friends[freeIndex].guid = guid;
  m_friends[freeIndex].m_connected = 0;
  return freeIndex;
}

void FriendList::SetName(unsigned __int64 guid, const char *name) {
  for (unsigned int i = 0; i < 50; ++i) {
    if (m_friends[i].guid == guid) {
      FREEIFUSED(m_friends[i].m_name);
      m_friends[i].m_name = SStrDupA(name, __FILE__, __LINE__);
      return;
    }
  }
}

void FriendList::SetConnected(unsigned __int64 guid, bool connected) {
  for (unsigned int i = 0; i < 50; ++i) {
    if (m_friends[i].guid == guid) {
      m_friends[i].m_connected = connected;
      return;
    }
  }
}

void FriendList::DecrementPendingFriendName() {
  if (m_friendNamesPending && !--m_friendNamesPending) {
    SortFriends();
  }
}

void FriendList::DecrementPendingIgnoreName() {
  if (m_ignoreNamesPending && !--m_ignoreNamesPending) {
    SortIgnore();
  }
}

bool FriendList::IsIgnored(unsigned __int64 guid) {
  for (unsigned int i = 0; i < 25; ++i) {
    if (m_ignore[i] == guid) {
      return true;
    }
  }
  return false;
}

void FriendList::AddFriends(CDataStore *msg) {
  unsigned int i;
  for (i = 0; i < 50; ++i) {
    FREEIFUSED(m_friends[i].m_name);
    memset(&m_friends[i], 0, sizeof(m_friends[i]));
  }
  m_friendNamesPending = 0;

  unsigned char count;
  msg->Get(count);
  for (i = 0; i < count && i < 50; ++i) {
    unsigned char connected;
    msg->Get(m_friends[i].guid);
    msg->Get(connected);
    m_friends[i].m_connected = connected != 0;
    if (connected) {
      msg->Get(m_friends[i].m_area);
      msg->Get(m_friends[i].m_level);
      msg->Get(m_friends[i].m_class);
    }
    const NameCache *name = g_nameDBCache.GetRecord(
        m_friends[i].guid, m_friends[i].guid, FriendListNameCallbackWithSort,
        reinterpret_cast<void *>(297));
    if (name) {
      m_friends[i].m_name = SStrDupA(name->m_name, __FILE__, __LINE__);
    } else {
      ++m_friendNamesPending;
    }
  }
  if (!m_friendNamesPending) {
    SortFriends();
  }
}

void FriendList::IgnoreAdded(unsigned __int64 guid, int sort) {
  for (unsigned int i = 0; i < 25; ++i) {
    if (m_ignore[i] == guid) {
      return;
    }
    if (!m_ignore[i]) {
      m_ignore[i] = guid;
      if (sort) {
        SortIgnore();
      }
      return;
    }
  }
}

void FriendList::IgnoreRemoved(unsigned __int64 guid) {
  for (unsigned int i = 0; i < 25; ++i) {
    if (m_ignore[i] == guid) {
      m_ignore[i] = 0;
      SortIgnore();
      return;
    }
  }
}

void FriendList::DelIgnore(unsigned __int64 guid) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_DEL_IGNORE));
  msg.Put(guid);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void FriendList::IgnoreList(CDataStore *msg) {
  memset(m_ignore, 0, sizeof(m_ignore));
  m_ignoreNamesPending = 0;
  unsigned char count;
  msg->Get(count);
  FATALASSERT(count <= 25);
  for (unsigned int i = 0; i < count; ++i) {
    unsigned __int64 guid;
    msg->Get(guid);
    IgnoreAdded(guid, 0);
    if (!g_nameDBCache.GetRecord(
            guid, guid, IgnoreListNameCallback, reinterpret_cast<void *>(297))) {
      ++m_ignoreNamesPending;
    }
  }
  if (!m_ignoreNamesPending) {
    SortIgnore();
  }
}

void FriendList::HandleStatus(FRIEND_RESULT result, unsigned __int64 guid, CDataStore *msg) {
  unsigned int i;
  switch (result) {
    case FRIEND_ONLINE:
    case FRIEND_ADDED_ONLINE: {
      int area;
      int level;
      int playerClass;
      msg->Get(area);
      msg->Get(level);
      msg->Get(playerClass);
      for (i = 0; i < 50; ++i) {
        if (m_friends[i].guid == guid || !m_friends[i].guid) {
          m_friends[i].guid = guid;
          m_friends[i].m_connected = 1;
          m_friends[i].m_area = area;
          m_friends[i].m_level = level;
          m_friends[i].m_class = playerClass;
          break;
        }
      }
      break;
    }
    case FRIEND_OFFLINE:
      for (i = 0; i < 50; ++i) {
        if (m_friends[i].guid == guid) m_friends[i].m_connected = 0;
      }
      break;
    case FRIEND_REMOVED:
      for (i = 0; i < 50; ++i) {
        if (m_friends[i].guid == guid) {
          FREEIFUSED(m_friends[i].m_name);
          memset(&m_friends[i], 0, sizeof(m_friends[i]));
          break;
        }
      }
      break;
    case FRIEND_ADDED_OFFLINE:
    case FRIEND_ALREADY:
      for (i = 0; i < 50; ++i) {
        if (!m_friends[i].guid) {
          m_friends[i].guid = guid;
          break;
        }
      }
      break;
    case FRIEND_IGNORE_ADDED:
      IgnoreAdded(guid, 0);
      break;
    case FRIEND_IGNORE_REMOVED:
      IgnoreRemoved(guid);
      break;
    default:
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(290));
      return;
  }

  bool ignoreResult = result >= FRIEND_IGNORE_ALREADY;
  void (*callback)(int, const unsigned __int64 &, void *, bool) =
      ignoreResult ? IgnoreListNameCallback : FriendListNameCallbackWithSort;
  const NameCache *name = g_nameDBCache.GetRecord(
      guid, guid, callback, reinterpret_cast<void *>(297));
  if (name) {
    if (!ignoreResult) SetName(guid, name->m_name);
    ignoreResult ? SortIgnore() : SortFriends();
  } else if (ignoreResult) {
    ++m_ignoreNamesPending;
  } else {
    ++m_friendNamesPending;
  }
}

char *StripQuotes(char *string) {
  if (!string) {
    return 0;
  }

  int length = strlen(string);
  if (length < 2 || (string[0] != '"' && string[0] != '\'')) {
    return string;
  }
  if (string[length - 1] != '"' && string[length - 1] != '\'') {
    return string;
  }

  string[length - 1] = 0;
  return string + 1;
}

FrameScript_Method s_FriendListScriptFunctions[19] = {
    {    "GetNumFriends",     Script_GetNumFriends},
    {    "GetFriendInfo",     Script_GetFriendInfo},
    {"SetSelectedFriend", Script_SetSelectedFriend},
    {"GetSelectedFriend", Script_GetSelectedFriend},
    {        "AddFriend",         Script_AddFriend},
    {     "RemoveFriend",      Script_RemoveFriend},
    {      "ShowFriends",       Script_ShowFriends},
    {          "SendWho",           Script_SendWho},
    {    "GetNumIgnores",     Script_GetNumIgnores},
    {    "GetIgnoreName",     Script_GetIgnoreName},
    {"SetSelectedIgnore", Script_SetSelectedIgnore},
    {"GetSelectedIgnore", Script_GetSelectedIgnore},
    {   "AddOrDelIgnore",    Script_AddOrDelIgnore},
    {        "AddIgnore",         Script_AddIgnore},
    {        "DelIgnore",         Script_DelIgnore},
    { "GetNumWhoResults",  Script_GetNumWhoResults},
    {       "GetWhoInfo",        Script_GetWhoInfo},
    {       "SetWhoToUI",        Script_SetWhoToUI},
    {          "SortWho",           Script_SortWho}
};

void FriendList::SendWho(const char *str) {
  char         words[4][128];
  char         guild[96];
  char         filter[128];
  char         name[48];
  int          zones[10];
  char        *wordptrs[4];
  int          minLevel = 0;
  int          quoted = 0;
  int          maxLevel = 100;
  int          raceFilter = -1;
  int          classFilter = -1;
  unsigned int numZones = 0;
  int          w = 0;

  FATALASSERT(str);

  name[0] = 0;
  guild[0] = 0;
  SStrCopy(filter, str, sizeof(filter));
  unsigned int length = strlen(filter);
  filter[length] = ' ';
  filter[length + 1] = 0;

  char *c = filter;
  while (isspace(*c)) {
    ++c;
  }

  while (*c && w < 4) {
    char        *word = words[w];
    unsigned int chars = 0;
    while (*c) {
      if (*c == '"' || *c == '\'') {
        quoted = !quoted;
      }
      if (!quoted && isspace(*c)) {
        break;
      }
      if (chars < sizeof(words[w]) - 1) {
        word[chars++] = *c;
      }
      ++c;
    }
    while (isspace(*c)) {
      ++c;
    }
    word[chars] = 0;
    wordptrs[w] = StripQuotes(word);

    const char  *tag = FrameScript_GetText("WHO_TAG_NAME", -1, GENDER_NOT_APPLICABLE);
    unsigned int tagLength = strlen(tag);
    if (!SStrCmp(word, tag, tagLength)) {
      SStrCopy(name, StripQuotes(word + tagLength), sizeof(name));
      continue;
    }

    tag = FrameScript_GetText("WHO_TAG_GUILD", -1, GENDER_NOT_APPLICABLE);
    tagLength = strlen(tag);
    if (!SStrCmp(word, tag, tagLength)) {
      SStrCopy(guild, StripQuotes(word + tagLength), 48);
      continue;
    }

    tag = FrameScript_GetText("WHO_TAG_ZONE", -1, GENDER_NOT_APPLICABLE);
    tagLength = strlen(tag);
    if (!SStrCmp(word, tag, tagLength)) {
      const char *zone = StripQuotes(word + tagLength);
      for (int index = 0; index < g_areaTableDB.GetNumRecords() && numZones < 10; ++index) {
        const AreaTableRec *rec = g_areaTableDB.GetRecordByIndex(index);
        if (!rec->m_ParentAreaNum && SStrStrI(rec->m_AreaName_lang[CURRENT_LANGUAGE], zone)) {
          zones[numZones++] = rec->m_ID;
        }
      }
      if (!numZones) {
        zones[numZones++] = 0;
      }
      continue;
    }

    tag = FrameScript_GetText("WHO_TAG_RACE", -1, GENDER_NOT_APPLICABLE);
    tagLength = strlen(tag);
    if (!SStrCmp(word, tag, tagLength)) {
      const char *race = StripQuotes(word + tagLength);
      if (raceFilter == -1) {
        raceFilter = 0;
      }
      for (int index = 0; index < g_chrRacesDB.GetNumRecords(); ++index) {
        const ChrRacesRec *rec = g_chrRacesDB.GetRecordByIndex(index);
        if (SStrStrI(rec->m_name_lang[CURRENT_LANGUAGE], race)) {
          raceFilter |= 1 << rec->m_ID;
        }
      }
      continue;
    }

    tag = FrameScript_GetText("WHO_TAG_CLASS", -1, GENDER_NOT_APPLICABLE);
    tagLength = strlen(tag);
    if (!SStrCmp(word, tag, tagLength)) {
      const char *playerClass = StripQuotes(word + tagLength);
      if (classFilter == -1) {
        classFilter = 0;
      }
      for (int index = 0; index < g_chrClassesDB.GetNumRecords(); ++index) {
        const ChrClassesRec *rec = g_chrClassesDB.GetRecordByIndex(index);
        if (SStrStrI(rec->m_name_lang[CURRENT_LANGUAGE], playerClass)) {
          classFilter |= 1 << rec->m_ID;
        }
      }
      continue;
    }

    char *range = wordptrs[w];
    int   haveMin = 0;
    if (isdigit(*range)) {
      minLevel = SStrToInt(range);
      maxLevel = minLevel;
      haveMin = 1;
      while (isdigit(*range)) {
        ++range;
      }
    }
    if (*range == '-') {
      ++range;
      maxLevel = 100;
      if (isdigit(*range)) {
        maxLevel = SStrToInt(range);
      }
      if (haveMin || maxLevel != 100) {
        continue;
      }
    }

    ++w;
  }

  CDataStore msg;
  msg.Put(CMSG_WHO);
  msg.Put(minLevel);
  msg.Put(maxLevel);
  msg.PutString(name);
  msg.PutString(guild);
  msg.Put(raceFilter);
  msg.Put(classFilter);
  msg.Put(static_cast<unsigned char>(numZones));
  for (unsigned int i = 0; i < numZones; ++i) {
    msg.Put(zones[i]);
  }
  msg.Put(w);
  for (int j = 0; j < w; ++j) {
    msg.PutString(wordptrs[j]);
  }
  msg.Finalize();
  ClientServices_Send(&msg);
}

void FriendList::AddOrDelIgnore(const char *name) {
  if (!name || !*name) {
    return;
  }
  for (unsigned int i = 0; i < GetNumIgnores(); ++i) {
    unsigned __int64 noGuid = 0;
    const NameCache *entry = g_nameDBCache.GetRecord(m_ignore[i], noGuid, 0, 0);
    if (entry && !SStrCmpI(entry->m_name, name, 0x7FFFFFFF)) {
      DelIgnore(name);
      return;
    }
  }
  AddIgnore(name);
}

void FriendList::AddIgnore(const char *name) {
  if (!name || !*name) {
    return;
  }
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_ADD_IGNORE));
  msg.PutString(name);
  msg.Finalize();
  ClientServices_Send(&msg);
}
