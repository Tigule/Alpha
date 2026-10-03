#include <Base/Base.h>
#include <WowConst.h>

#include "WowSvcs/WowSvcsClient/FriendList.h"

#include "Console/ConsoleCommand.h"
#include "Console/ConsoleClient.h"
#include "Console/ConsoleVar.h"
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
#include <FrameXML/LoadXML.h>


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

static int Script_GetNumFriends(lua_State *L);
static int Script_GetFriendInfo(lua_State *L);
static int Script_SetSelectedFriend(lua_State *L);
static int Script_GetSelectedFriend(lua_State *L);
static int Script_AddFriend(lua_State *L);
static int Script_RemoveFriend(lua_State *L);
static int Script_ShowFriends(lua_State *L);
static int Script_SendWho(lua_State *L);
static int Script_GetNumIgnores(lua_State *L);
static int Script_GetIgnoreName(lua_State *L);
static int Script_SetSelectedIgnore(lua_State *L);
static int Script_GetSelectedIgnore(lua_State *L);
static int Script_AddOrDelIgnore(lua_State *L);
static int Script_AddIgnore(lua_State *L);
static int Script_DelIgnore(lua_State *L);
static int Script_GetNumWhoResults(lua_State *L);
static int Script_GetWhoInfo(lua_State *L);
static int Script_SetWhoToUI(lua_State *L);
static int Script_SortWho(lua_State *L);

static WhoListEntry s_whoList[50];
static UINT         s_numWhos;
static UINT         s_totalNumWhos;
static LPCSTR       s_partyStatusStrings[3] = {"no", "yes", "looking"};
static CVar        *s_whoChatThreshold;
static int          s_whoToUI;

static FrameScript_Method s_ScriptFunctions[19] = {
    {    "GetNumFriends",     Script_GetNumFriends},
    {    "GetFriendInfo",     Script_GetFriendInfo},
    {"SetSelectedFriend", Script_SetSelectedFriend},
    {"GetSelectedFriend", Script_GetSelectedFriend},
    {        "AddFriend",         Script_AddFriend},
    {     "RemoveFriend",      Script_RemoveFriend},
    {      "ShowFriends",       Script_ShowFriends},
    {    "GetNumIgnores",     Script_GetNumIgnores},
    {    "GetIgnoreName",     Script_GetIgnoreName},
    {"SetSelectedIgnore", Script_SetSelectedIgnore},
    {"GetSelectedIgnore", Script_GetSelectedIgnore},
    {   "AddOrDelIgnore",    Script_AddOrDelIgnore},
    {        "AddIgnore",         Script_AddIgnore},
    {        "DelIgnore",         Script_DelIgnore},
    {          "SendWho",           Script_SendWho},
    { "GetNumWhoResults",  Script_GetNumWhoResults},
    {       "GetWhoInfo",        Script_GetWhoInfo},
    {       "SetWhoToUI",        Script_SetWhoToUI},
    {          "SortWho",           Script_SortWho}
};

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

WhoSortType s_whoSortCriteria[NUM_WHO_SORT_TYPES];

FriendList::FriendList() {
  memset(m_friends, 0, sizeof(m_friends));
  memset(m_ignore, 0, sizeof(m_ignore));
  m_friendNamesPending = 0;
  m_selectedFriend = 0;
  m_ignoreNamesPending = 0;
  m_selectedIgnore = 0;
}

static BOOL FriendListStatusHandler(LPVOID, NETMESSAGE, DWORD, CDataStore *msg) {
  BYTE      res;
  DWORDLONG guid;
  msg->Get(res);
  msg->Get(guid);
  if (g_friendList) {
    g_friendList->HandleStatus(static_cast<FRIEND_RESULT>(res), guid, msg);
  }
  return 1;
}

static void FriendListNameCallbackWithSort(int id, const DWORDLONG &guid, LPVOID arg, bool granted) {
  GAME_ERROR_TYPE error = static_cast<GAME_ERROR_TYPE>(reinterpret_cast<UINT>(arg));
  if (!granted) {
    if (error != GERR_NONE) {
      CGGameUI::DisplayError(GERR_FRIEND_ERROR);
    }
    if (g_friendList) {
      g_friendList->SetName(guid, FrameScript_GetText("UNKNOWN", -1, GENDER_NOT_APPLICABLE));
      g_friendList->DecrementPendingFriendName();
    }
  } else {
    const NameCache *nc = g_nameDBCache.GetRecord(guid, 0, 0, 0);
    FATALASSERT(nc);
    if (g_friendList) {
      g_friendList->SetName(guid, nc->m_name);
      g_friendList->DecrementPendingFriendName();
    }
    if (error != GERR_NONE) {
      CGGameUI::DisplayError(error, nc->m_name);
    }
  }
}

static void IgnoreListNameCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted) {
  GAME_ERROR_TYPE error = static_cast<GAME_ERROR_TYPE>(reinterpret_cast<UINT>(arg));
  if (!granted) {
    if (error != GERR_NONE) {
      CGGameUI::DisplayError(GERR_FRIEND_ERROR);
    }
    if (g_friendList) {
      g_friendList->DelIgnore(guid);
      g_friendList->DecrementPendingIgnoreName();
    }
  } else {
    const NameCache *nc = g_nameDBCache.GetRecord(guid, 0, 0, 0);
    FATALASSERT(nc);
    if (g_friendList) {
      g_friendList->DecrementPendingIgnoreName();
    }
    if (error != GERR_NONE) {
      CGGameUI::DisplayError(error, nc->m_name);
    }
  }
}

FriendList::~FriendList() {
}

static BOOL FriendListHandler(LPVOID, NETMESSAGE, DWORD, CDataStore *msg) {
  g_friendList->AddFriends(msg);
  return 1;
}

static BOOL CCommand_Friends(LPCSTR command, LPCSTR arguments) {
  CDataStore msg;
  msg.Put(CMSG_FRIEND_LIST);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static BOOL CCommand_AddFriend(LPCSTR command, LPCSTR arguments) {
  g_friendList->AddFriend(arguments);
  return 1;
}

static BOOL CCommand_RemoveFriend(LPCSTR command, LPCSTR arguments) {
  g_friendList->RemoveFriend(arguments);
  return 1;
}

static BOOL WhoisResponseHandler(LPVOID, NETMESSAGE, DWORD, CDataStore *msg) {
  char name[256];
  msg->GetString(name, 0x7FFFFFFF);
  ConsoleWrite(name, DEFAULT_COLOR);
  return 1;
}

void FriendList::RemoveFriend(LPCSTR name) {
  for (UINT i = 0; i < 50; ++i) {
    if (m_friends[i].m_name && !SStrCmpI(m_friends[i].m_name, name, 0x7FFFFFFF)) {
      RemoveFriend(m_friends[i].guid);
      return;
    }
  }
  CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(0xEE));
}

void FriendList::RemoveFriend(DWORDLONG guid) {
  CDataStore msg;
  msg.Put(CMSG_DEL_FRIEND);
  msg.Put(guid);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void FriendList::RemoveFriend(UINT index) {
  if (index < 50) {
    DWORDLONG guid = m_friends[index].guid;
    if (guid) {
      RemoveFriend(guid);
    }
  }
}

static BOOL ReverseWhoisResponseHandler(LPVOID, NETMESSAGE, DWORD, CDataStore *msg) {
  UINT numAccounts = 0;
  msg->Get(numAccounts);
  if (numAccounts == static_cast<UINT>(-1)) {
    ConsoleWriteA("RWhoIs failed\n", ERROR_COLOR);
    return 1;
  }
  if (!numAccounts) {
    ConsoleWriteA("Not found\n", WARNING_COLOR);
    return 1;
  }

  for (int account = 0; account < static_cast<int>(numAccounts); ++account) {
    char accountName[64];
    msg->GetString(accountName, 0x7FFFFFFF);
    ConsolePrintf("Account: %s\n", accountName);

    int numCharacters = 0;
    msg->Get(numCharacters);
    int selected = 0;
    msg->Get(selected);
    for (int character = 0; character < numCharacters; ++character) {
      char characterName[48];
      msg->GetString(characterName, 0x7FFFFFFF);
      ConsoleWriteA("  %s\n", character == selected ? WARNING_COLOR : DEFAULT_COLOR, characterName);
    }
  }
  return 1;
}

static BOOL CCommand_Whois(LPCSTR, LPCSTR args) {
  CDataStore msg;
  msg.Put(CMSG_WHOIS);
  msg.PutString(args);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static BOOL CCommand_RWhois(LPCSTR, LPCSTR args) {
  CDataStore msg;
  msg.Put(CMSG_RWHOIS);
  msg.PutString(args);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static int Script_GetNumFriends(lua_State *L) {
  if (g_friendList) {
    lua_pushnumber(L, g_friendList->GetNumFriends());
  } else {
    lua_pushnumber(L, 0.0);
  }
  return 1;
}

static int Script_GetFriendInfo(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetFriendInfo(index)");
    return 0;
  }
  LPCSTR                    unknown = FrameScript_GetText("UNKNOWN", -1, GENDER_NOT_APPLICABLE);
  const FriendList::Friend *info = g_friendList->GetFriend(static_cast<UINT>(lua_tonumber(L, 1)) - 1);
  if (info) {
    lua_pushstring(L, info->m_name);
    lua_pushnumber(L, info->m_level);
    const ChrClassesRec *classRec = g_chrClassesDB.GetRecord(info->m_class);
    lua_pushstring(L, classRec ? classRec->m_name_lang[CURRENT_LANGUAGE] : unknown);
    const AreaTableRec *areaRec = g_areaTableDB.GetRecord(info->m_area);
    if (areaRec && areaRec->m_ParentAreaNum) {
      for (UINT i = 0; i < g_areaTableDB.GetNumRecords(); ++i) {
        const AreaTableRec *rec = g_areaTableDB.GetRecordByIndex(i);
        if (rec->m_AreaNumber == areaRec->m_ParentAreaNum) {
          areaRec = rec;
          break;
        }
      }
    }
    lua_pushstring(L, areaRec ? areaRec->m_AreaName_lang[CURRENT_LANGUAGE] : unknown);
    if (info->m_connected) {
      lua_pushnumber(L, 1.0);
    } else {
      lua_pushnil(L);
    }
    return 5;
  }
  lua_pushstring(L, unknown);
  lua_pushnumber(L, 1.0);
  lua_pushstring(L, unknown);
  lua_pushstring(L, unknown);
  lua_pushnil(L);
  return 5;
}

static int Script_SetSelectedFriend(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: SetSelectedFriend(index)");
    return 0;
  }
  g_friendList->SetFriendSelectionIndex(static_cast<UINT>(lua_tonumber(L, 1)) - 1);
  return 0;
}

static int Script_GetSelectedFriend(lua_State *L) {
  lua_pushnumber(L, g_friendList->GetFriendSelectionIndex() + 1);
  return 1;
}

void FriendList::DelIgnore(LPCSTR name) {
  for (UINT i = 0; i < 25; ++i) {
    if (!m_ignore[i])
      continue;
    const NameCache *entry = g_nameDBCache.GetRecord(m_ignore[i], 0, 0, 0);
    if (entry && !SStrCmpI(entry->m_name, name, 0x7FFFFFFF)) {
      DelIgnore(m_ignore[i]);
      return;
    }
  }
  CGGameUI::DisplayError(GERR_IGNORE_NOT_FOUND);
}

static int Script_AddFriend(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: AddFriend(\"name\")");
    return 0;
  }
  CCommand_AddFriend("addfriend", lua_tostring(L, 1));
  return 0;
}

static int Script_RemoveFriend(lua_State *L) {
  if (lua_isnumber(L, 1)) {
    g_friendList->RemoveFriend(static_cast<UINT>(lua_tonumber(L, 1)) - 1);
    return 0;
  }
  if (lua_isstring(L, 1)) {
    g_friendList->RemoveFriend(lua_tostring(L, 1));
    return 0;
  }
  luaL_error(L, "Usage: RemoveFriend([\"name\"] or [index])");
  return 0;
}

static int Script_ShowFriends(lua_State *L) {
  CCommand_Friends("friends", "");
  return 0;
}

static int Script_SendWho(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: SendWho(\"filter\")");
    return 0;
  }
  if (g_friendList) {
    g_friendList->SendWho(lua_tostring(L, 1));
  }
  return 0;
}

static int Script_GetNumIgnores(lua_State *L) {
  if (g_friendList) {
    lua_pushnumber(L, g_friendList->GetNumIgnores());
  } else {
    lua_pushnumber(L, 0.0);
  }
  return 1;
}

static int Script_GetIgnoreName(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetIgnoreName(index)");
    return 0;
  }
  DWORDLONG        guid = g_friendList->GetIgnore(static_cast<UINT>(lua_tonumber(L, 1)) - 1);
  const NameCache *entry = g_nameDBCache.GetRecord(guid, 0, 0, 0);
  lua_pushstring(L, entry ? entry->m_name : FrameScript_GetText("UNKNOWN", -1, GENDER_NOT_APPLICABLE));
  return 1;
}

static int Script_SetSelectedIgnore(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: SetSelectedIgnore(index)");
    return 0;
  }
  g_friendList->SetIgnoreSelectionIndex(static_cast<UINT>(lua_tonumber(L, 1)) - 1);
  return 0;
}

static int Script_GetSelectedIgnore(lua_State *L) {
  lua_pushnumber(L, g_friendList->GetIgnoreSelectionIndex() + 1);
  return 1;
}

static int Script_AddOrDelIgnore(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: AddOrDelIgnore(\"name\")");
    return 0;
  }
  if (g_friendList) {
    g_friendList->AddOrDelIgnore(lua_tostring(L, 1));
  }
  return 0;
}

static int Script_AddIgnore(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: AddIgnore(\"name\")");
    return 0;
  }
  if (g_friendList) {
    g_friendList->AddIgnore(lua_tostring(L, 1));
  }
  return 0;
}

static int Script_DelIgnore(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: DelIgnore(\"name\")");
    return 0;
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
    luaL_error(L, "Usage: GetWhoInfo(index)");
    return 0;
  }
  UINT index = static_cast<UINT>(lua_tonumber(L, 1)) - 1;
  if (index <= s_numWhos) {
    lua_pushstring(L, s_whoList[index].name);
    lua_pushstring(L, s_whoList[index].guild);
    lua_pushnumber(L, s_whoList[index].level);
    const ChrRacesRec *race = g_chrRacesDB.GetRecord(s_whoList[index].raceID);
    lua_pushstring(L, race ? race->m_name_lang[CURRENT_LANGUAGE] : FrameScript_GetText("UNKNOWN", -1, GENDER_NOT_APPLICABLE));
    const ChrClassesRec *playerClass = g_chrClassesDB.GetRecord(s_whoList[index].classID);
    lua_pushstring(L, playerClass ? playerClass->m_name_lang[CURRENT_LANGUAGE] : FrameScript_GetText("UNKNOWN", -1, GENDER_NOT_APPLICABLE));
    const AreaTableRec *area = g_areaTableDB.GetRecord(s_whoList[index].areaID);
    lua_pushstring(L, area ? area->m_AreaName_lang[CURRENT_LANGUAGE] : FrameScript_GetText("UNKNOWN", -1, GENDER_NOT_APPLICABLE));
    lua_pushstring(L, s_partyStatusStrings[s_whoList[index].partyStatus]);
  } else {
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnumber(L, 0.0);
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
  }
  return 7;
}

static int Script_SetWhoToUI(lua_State *L) {
  s_whoToUI = 0;
  if (lua_isnumber(L, 1)) {
    s_whoToUI = static_cast<int>(lua_tonumber(L, 1));
  } else if (lua_isstring(L, 1)) {
    s_whoToUI = StringToBOOL(lua_tostring(L, 1));
  }
  return 0;
}

static int __cdecl QSortWho(LPCVOID a, LPCVOID b) {
  FATALASSERT(a);
  FATALASSERT(b);
  WhoListEntry info1 = *static_cast<const WhoListEntry *>(a);
  WhoListEntry info2 = *static_cast<const WhoListEntry *>(b);

  for (UINT i = 0; i < NUM_WHO_SORT_TYPES; ++i) {
    int result = 0;
    switch (s_whoSortCriteria[i].type) {
      case WHO_SORT_NAME:
        result = SStrCmpI(info1.name, info2.name, 0x7FFFFFFF);
        break;
      case WHO_SORT_LEVEL:
        if (info1.level != info2.level) {
          result = info1.level < info2.level ? -1 : 1;
        }
        break;
      case WHO_SORT_CLASS: {
        const ChrClassesRec *rec1 = g_chrClassesDB.GetRecord(info1.classID);
        const ChrClassesRec *rec2 = g_chrClassesDB.GetRecord(info2.classID);
        if (rec1 && rec2) {
          result = SStrCmpI(rec1->m_name_lang[CURRENT_LANGUAGE], rec2->m_name_lang[CURRENT_LANGUAGE], 0x7FFFFFFF);
        }
        break;
      }
      case WHO_SORT_GROUP:
        if (info1.partyStatus != info2.partyStatus) {
          result = info1.partyStatus > info2.partyStatus ? -1 : 1;
        }
        break;
      case WHO_SORT_RACE: {
        const ChrRacesRec *rec1 = g_chrRacesDB.GetRecord(info1.raceID);
        const ChrRacesRec *rec2 = g_chrRacesDB.GetRecord(info2.raceID);
        if (rec1 && rec2) {
          result = SStrCmpI(rec1->m_name_lang[CURRENT_LANGUAGE], rec2->m_name_lang[CURRENT_LANGUAGE], 0x7FFFFFFF);
        }
        break;
      }
      case WHO_SORT_ZONE: {
        const AreaTableRec *rec1 = g_areaTableDB.GetRecord(info1.areaID);
        const AreaTableRec *rec2 = g_areaTableDB.GetRecord(info2.areaID);
        if (rec1 && rec2) {
          result = SStrCmpI(rec1->m_AreaName_lang[CURRENT_LANGUAGE], rec2->m_AreaName_lang[CURRENT_LANGUAGE], 0x7FFFFFFF);
        }
        break;
      }
      case WHO_SORT_GUILD:
        result = SStrCmpI(info1.guild, info2.guild, 0x7FFFFFFF);
        break;
    }
    if (result) {
      if (s_whoSortCriteria[i].reverse) {
        return -result;
      }
      return result;
    }
  }
  return 0;
}

static int Script_SortWho(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usgae: SortWho(\"type\")");
    return 0;
  }

  WHO_SORT_TYPE type = WHO_SORT_NAME;
  LPCSTR        str = lua_tostring(L, 1);
  if (!SStrCmpI(str, "name", 0x7FFFFFFF)) {
    type = WHO_SORT_NAME;
  } else if (!SStrCmpI(str, "level", 0x7FFFFFFF)) {
    type = WHO_SORT_LEVEL;
  } else if (!SStrCmpI(str, "class", 0x7FFFFFFF)) {
    type = WHO_SORT_CLASS;
  } else if (!SStrCmpI(str, "group", 0x7FFFFFFF)) {
    type = WHO_SORT_GROUP;
  } else if (!SStrCmpI(str, "race", 0x7FFFFFFF)) {
    type = WHO_SORT_RACE;
  } else if (!SStrCmpI(str, "zone", 0x7FFFFFFF)) {
    type = WHO_SORT_ZONE;
  } else if (!SStrCmpI(str, "guild", 0x7FFFFFFF)) {
    type = WHO_SORT_GUILD;
  }

  for (UINT i = 0; i < NUM_WHO_SORT_TYPES; ++i) {
    if (s_whoSortCriteria[i].type == type) {
      int reverse = s_whoSortCriteria[i].reverse;
      if (i == 0) {
        reverse = !reverse;
      }
      for (UINT j = i; j > 0; --j) {
        s_whoSortCriteria[j].type = s_whoSortCriteria[j - 1].type;
        s_whoSortCriteria[j].reverse = s_whoSortCriteria[j - 1].reverse;
      }
      s_whoSortCriteria[0].type = type;
      s_whoSortCriteria[0].reverse = reverse;
      break;
    }
  }

  qsort(s_whoList, s_numWhos, sizeof(WhoListEntry), QSortWho);
  FrameScript_SignalEvent(371);
  return 0;
}

void FriendList::RegisterScriptFunctions() {
  for (UINT i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void FriendList::UnregisterScriptFunctions() {
  for (UINT i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}

static void PrintWho(LPCSTR name, LPCSTR guild, int level, int classID, int raceID, int areaID) {
  const ChrRacesRec   *race = g_chrRacesDB.GetRecord(raceID);
  LPCSTR               raceName = race ? race->m_name_lang[CURRENT_LANGUAGE] : FrameScript_GetText("UNKNOWN", -1, GENDER_NOT_APPLICABLE);
  const ChrClassesRec *playerClass = g_chrClassesDB.GetRecord(classID);
  LPCSTR               className = playerClass ? playerClass->m_name_lang[CURRENT_LANGUAGE] : FrameScript_GetText("UNKNOWN", -1, GENDER_NOT_APPLICABLE);
  const AreaTableRec  *area = g_areaTableDB.GetRecord(areaID);
  LPCSTR               areaName = area ? area->m_AreaName_lang[CURRENT_LANGUAGE] : FrameScript_GetText("UNKNOWN", -1, GENDER_NOT_APPLICABLE);
  char                 fullLine[256];
  if (guild && *guild) {
    SStrPrintf(fullLine, sizeof(fullLine), FrameScript_GetText("WHO_LIST_GUILD_FORMAT", -1, GENDER_NOT_APPLICABLE), name, level, raceName, className, guild, areaName);
  } else {
    SStrPrintf(fullLine, sizeof(fullLine), FrameScript_GetText("WHO_LIST_FORMAT", -1, GENDER_NOT_APPLICABLE), name, level, raceName, className, areaName);
  }
  CGChat::AddChatMessage(fullLine, SLASH_CMD_SYSTEM, 0, 0, 0, 0, 0);
}

static BOOL OnWhoList(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  FATALASSERT(msg);
  DWORD count;
  DWORD totalCount;
  msg->Get(count);
  msg->Get(totalCount);
  s_numWhos = min(count, 50);
  s_totalNumWhos = totalCount;
  int toChat = 0;
  if (!s_whoToUI) {
    int threshold = s_whoChatThreshold ? s_whoChatThreshold->GetInt() : 3;
    if (threshold < 0 || threshold >= static_cast<int>(count)) {
      toChat = 1;
    }
  }

  for (DWORD i = 0; i < count; ++i) {
    char name[48];
    char guild[96];
    msg->GetString(name, sizeof(name));
    msg->GetString(guild, sizeof(guild));
    int level = 0;
    msg->Get(level);
    int classID = 0;
    msg->Get(classID);
    int raceID = 0;
    msg->Get(raceID);
    int areaID = 0;
    msg->Get(areaID);
    int partyStatus = 0;
    msg->Get(partyStatus);
    if (i < 50) {
      SStrCopy(s_whoList[i].name, name, sizeof(name));
      SStrCopy(s_whoList[i].guild, guild, sizeof(guild));
      s_whoList[i].level = level;
      s_whoList[i].classID = classID;
      s_whoList[i].raceID = raceID;
      s_whoList[i].areaID = areaID;
      s_whoList[i].partyStatus = static_cast<PARTY_STATUS>(partyStatus);
    }
    if (toChat) {
      PrintWho(name, guild, level, classID, raceID, areaID);
    }
  }

  qsort(s_whoList, s_numWhos, sizeof(WhoListEntry), QSortWho);
  if (toChat) {
    char buf[256];
    SStrPrintf(buf, sizeof(buf), FrameScript_GetText("WHO_NUM_RESULTS", count, GENDER_NOT_APPLICABLE), count);
    CGChat::AddChatMessage(buf, SLASH_CMD_SYSTEM, 0, 0, 0, 0, 0);
  } else {
    FrameScript_SignalEvent(371);
  }
  return 1;
}

static BOOL OnIgnoreList(LPVOID, NETMESSAGE, DWORD, CDataStore *msg) {
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

  for (UINT i = 0; i < NUM_WHO_SORT_TYPES; ++i) {
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

UINT FriendList::GetNumFriends() {
  UINT count = 0;
  while (count < 50 && m_friends[count].guid) {
    ++count;
  }
  return count;
}

const FriendList::Friend *FriendList::GetFriend(UINT index) {
  if (index > 50) {
    return 0;
  }
  return &m_friends[index];
}

void FriendList::SetFriendSelectionIndex(UINT index) {
  if (index > 50) {
    m_selectedFriend = 0;
    return;
  }
  m_selectedFriend = m_friends[index].guid;
}

int FriendList::GetFriendSelectionIndex() {
  if (m_selectedFriend) {
    for (UINT i = 0; i < 50; ++i) {
      if (m_friends[i].guid == m_selectedFriend) {
        return i;
      }
    }
  }
  return -1;
}

UINT FriendList::GetNumIgnores() {
  UINT count = 0;
  while (count < 25 && m_ignore[count]) {
    ++count;
  }
  return count;
}

DWORDLONG FriendList::GetIgnore(UINT index) {
  if (index > 25) {
    return 0;
  }
  return m_ignore[index];
}

void FriendList::SetIgnoreSelectionIndex(UINT index) {
  if (index > 25) {
    m_selectedIgnore = 0;
    return;
  }
  m_selectedIgnore = m_ignore[index];
}

int FriendList::GetIgnoreSelectionIndex() {
  if (m_selectedIgnore) {
    for (UINT i = 0; i < 25; ++i) {
      if (m_ignore[i] == m_selectedIgnore) {
        return i;
      }
    }
  }
  return -1;
}

void FriendList::ShowFriends() {
  char text[256];
  for (UINT i = 0; i < 50; ++i) {
    if (m_friends[i].m_name) {
      if (m_friends[i].m_connected) {
        SStrPrintf(text, sizeof(text), "%s - Online", m_friends[i].m_name);
      } else {
        SStrPrintf(text, sizeof(text), "%s - Offline", m_friends[i].m_name);
      }
      CGChat::AddChatMessage(text, static_cast<SLASH_COMMAND_ID>(9), 0, 0, 0, 0, 0);
    }
  }
}

void FriendList::AddFriend(LPCSTR name) {
  CDataStore msg;
  msg.Put(CMSG_ADD_FRIEND);
  msg.PutString(name);
  msg.Finalize();
  ClientServices_Send(&msg);
}

static int __cdecl QSortFriends(LPCVOID a, LPCVOID b) {
  FATALASSERT(a);
  FATALASSERT(b);
  const FriendList::Friend *left = static_cast<const FriendList::Friend *>(a);
  const FriendList::Friend *right = static_cast<const FriendList::Friend *>(b);
  if (!left->guid && !right->guid)
    return 0;
  if (!left->guid)
    return 1;
  if (!right->guid)
    return -1;
  if (left->m_connected == right->m_connected) {
    if (left->m_name && right->m_name) {
      return SStrCmpI(left->m_name, right->m_name, 0x7FFFFFFF);
    }
    if (!left->m_name && !right->m_name)
      return 0;
    return left->m_name ? -1 : 1;
  }
  return left->m_connected ? -1 : 1;
}

static int __cdecl QSortIgnore(LPCVOID a, LPCVOID b) {
  FATALASSERT(a);
  FATALASSERT(b);
  DWORDLONG left = *static_cast<const DWORDLONG *>(a);
  DWORDLONG right = *static_cast<const DWORDLONG *>(b);
  if (!left)
    return right != 0;
  if (!right)
    return -1;
  DWORDLONG        nc1 = 0;
  const NameCache *leftName = g_nameDBCache.GetRecord(left, nc1, 0, 0);
  const NameCache *rightName = g_nameDBCache.GetRecord(right, nc1, 0, 0);
  if (leftName && rightName)
    return SStrCmpI(leftName->m_name, rightName->m_name, 0x7FFFFFFF);
  if (!leftName && !rightName)
    return 0;
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

void FriendList::Removed(DWORDLONG guid) {
  for (UINT i = 0; i < 50; ++i) {
    if (m_friends[i].guid == guid) {
      m_friends[i].guid = 0;
      FREEIFUSED(m_friends[i].m_name);
      m_friends[i].m_name = 0;
      return;
    }
  }
}

int FriendList::Added(DWORDLONG guid) {
  UINT freeIndex = -1;
  UINT i = 0;

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

void FriendList::SetName(DWORDLONG guid, LPCSTR name) {
  for (UINT i = 0; i < 50; ++i) {
    if (m_friends[i].guid == guid) {
      if (m_friends[i].m_name) {
        FREE(m_friends[i].m_name);
      }
      m_friends[i].m_name = SStrDupA(name, __FILE__, __LINE__);
      return;
    }
  }
}

void FriendList::SetConnected(DWORDLONG guid, bool connected) {
  for (UINT i = 0; i < 50; ++i) {
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

bool FriendList::IsIgnored(DWORDLONG guid) {
  for (UINT i = 0; i < 25; ++i) {
    if (m_ignore[i] == guid) {
      return true;
    }
  }
  return false;
}

void FriendList::AddFriends(CDataStore *msg) {
  for (UINT i = 0; i < 50; ++i) {
    FREEIFUSED(m_friends[i].m_name);
    m_friends[i].m_name = 0;
  }

  BYTE count;
  msg->Get(count);
  for (BYTE f = 0; f < count && f < 50; ++f) {
    BYTE      status;
    DWORDLONG guid;
    msg->Get(guid);
    msg->Get(status);
    m_friends[f].guid = guid;
    m_friends[f].m_connected = status != 0;
    if (status) {
      msg->Get(m_friends[f].m_area);
      msg->Get(m_friends[f].m_level);
      msg->Get(m_friends[f].m_class);
    } else {
      m_friends[f].m_area = 0;
      m_friends[f].m_level = 0;
      m_friends[f].m_class = 0;
    }
    const NameCache *name = g_nameDBCache.GetRecord(m_friends[f].guid, m_friends[f].guid, FriendListNameCallbackWithSort, reinterpret_cast<LPVOID>(297));
    if (name) {
      m_friends[f].m_name = SStrDupA(name->m_name, __FILE__, __LINE__);
    } else {
      ++m_friendNamesPending;
    }
  }
  if (!m_friendNamesPending) {
    SortFriends();
  }
}

void FriendList::IgnoreAdded(DWORDLONG guid, int sort) {
  for (UINT i = 0; i < 25; ++i) {
    if (!m_ignore[i]) {
      m_ignore[i] = guid;
      if (sort) {
        if (!g_nameDBCache.GetRecord(guid, guid, IgnoreListNameCallback, reinterpret_cast<LPVOID>(297))) {
          ++m_ignoreNamesPending;
        }
        if (!m_ignoreNamesPending) {
          SortIgnore();
        }
      }
      return;
    }
  }
}

void FriendList::IgnoreRemoved(DWORDLONG guid) {
  for (UINT i = 0; i < 25; ++i) {
    if (m_ignore[i] == guid) {
      m_ignore[i] = 0;
      if (!m_ignoreNamesPending) {
        SortIgnore();
      }
      return;
    }
  }
}

void FriendList::DelIgnore(DWORDLONG guid) {
  CDataStore msg;
  msg.Put(CMSG_DEL_IGNORE);
  msg.Put(guid);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void FriendList::IgnoreList(CDataStore *msg) {
  memset(m_ignore, 0, sizeof(m_ignore));
  BYTE count;
  msg->Get(count);
  FATALASSERT(count <= (sizeof(m_ignore) / sizeof(m_ignore[0])));
  for (int i = 0; i < count; ++i) {
    DWORDLONG guid;
    msg->Get(guid);
    IgnoreAdded(guid, 0);
    if (!g_nameDBCache.GetRecord(guid, guid, IgnoreListNameCallback, reinterpret_cast<LPVOID>(297))) {
      ++m_ignoreNamesPending;
    }
  }
  if (!m_ignoreNamesPending) {
    SortIgnore();
  }
}

void FriendList::HandleStatus(FRIEND_RESULT res, DWORDLONG guid, CDataStore *msg) {
  GAME_ERROR_TYPE error;
  bool            sortFriends = false;
  bool            sortIgnore = false;
  switch (res) {
    case FRIEND_DB_ERROR:
      error = GERR_FRIEND_DB_ERROR;
      break;
    case FRIEND_LIST_FULL:
      error = GERR_FRIEND_LIST_FULL;
      break;
    case FRIEND_ADDED_ONLINE: {
      error = GERR_FRIEND_ADDED_S;
      int area;
      int level;
      int classID;
      msg->Get(area);
      msg->Get(level);
      msg->Get(classID);
      int index = Added(guid);
      if (index >= 0) {
        m_friends[index].m_area = area;
        m_friends[index].m_level = level;
        m_friends[index].m_class = classID;
      }
      SetConnected(guid, true);
      sortFriends = true;
      break;
    }
    case FRIEND_ADDED_OFFLINE:
      error = GERR_FRIEND_ADDED_S;
      Added(guid);
      SetConnected(guid, false);
      sortFriends = true;
      break;
    case FRIEND_ONLINE: {
      error = GERR_FRIEND_ONLINE_S;
      int area;
      int level;
      int classID;
      msg->Get(area);
      msg->Get(level);
      msg->Get(classID);
      for (UINT i = 0; i < 50; ++i) {
        if (m_friends[i].guid == guid) {
          m_friends[i].m_area = area;
          m_friends[i].m_level = level;
          m_friends[i].m_class = classID;
          break;
        }
      }
      SetConnected(guid, true);
      sortFriends = true;
      break;
    }
    case FRIEND_OFFLINE:
      error = GERR_FRIEND_OFFLINE_S;
      SetConnected(guid, false);
      sortFriends = true;
      break;
    case FRIEND_NOT_FOUND:
      error = GERR_FRIEND_NOT_FOUND;
      break;
    case FRIEND_ENEMY:
      error = GERR_FRIEND_WRONG_FACTION;
      break;
    case FRIEND_REMOVED:
      error = GERR_FRIEND_REMOVED_S;
      Removed(guid);
      sortFriends = true;
      break;
    case FRIEND_ALREADY:
      error = GERR_FRIEND_ALREADY_S;
      Added(guid);
      sortFriends = true;
      break;
    case FRIEND_SELF:
      error = GERR_FRIEND_SELF;
      break;
    case FRIEND_IGNORE_FULL:
      error = GERR_IGNORE_FULL;
      break;
    case FRIEND_IGNORE_SELF:
      error = GERR_IGNORE_SELF;
      break;
    case FRIEND_IGNORE_NOT_FOUND:
      error = GERR_IGNORE_NOT_FOUND;
      break;
    case FRIEND_IGNORE_ALREADY:
      error = GERR_IGNORE_ALREADY_S;
      sortIgnore = true;
      break;
    case FRIEND_IGNORE_ADDED:
      error = GERR_IGNORE_ADDED_S;
      IgnoreAdded(guid, 1);
      sortIgnore = true;
      break;
    case FRIEND_IGNORE_REMOVED:
      error = GERR_IGNORE_REMOVED_S;
      IgnoreRemoved(guid);
      sortIgnore = true;
      break;
    default:
      error = GERR_FRIEND_ERROR;
      break;
  }

  if (!sortFriends && !sortIgnore) {
    CGGameUI::DisplayError(error);
    return;
  }

  const NameCache *nc = g_nameDBCache.GetRecord(guid, guid, sortFriends ? FriendListNameCallbackWithSort : IgnoreListNameCallback, reinterpret_cast<LPVOID>(error));
  if (nc) {
    if (error == GERR_FRIEND_ADDED_S) {
      SetName(guid, nc->m_name);
    }
    if (sortFriends) {
      SortFriends();
    } else if (sortIgnore) {
      SortIgnore();
    }
    CGGameUI::DisplayError(error, nc->m_name);
  } else if (sortFriends) {
    ++m_friendNamesPending;
  } else if (sortIgnore) {
    ++m_ignoreNamesPending;
  }
}

static char *StripQuotes(char *string) {
  if (!string) {
    return 0;
  }

  int length = SStrLen(string);
  if (length < 2) {
    return string;
  }
  if ((string[0] == '"' || string[0] == '\'') && (string[length - 1] == '"' || string[length - 1] == '\'')) {
    string[length - 1] = 0;
    return string + 1;
  }
  return string;
}


void FriendList::SendWho(LPCSTR str) {
  char  words[4][128];
  char  guild[96];
  char  filter[128];
  char  name[48];
  int   zones[10];
  char *wordptrs[4];
  int   minLevel;
  int   maxLevel;
  int   raceFilter;
  int   classFilter;
  UINT  numZones;

  FATALASSERT(str);

  raceFilter = -1;
  classFilter = -1;
  minLevel = 0;
  maxLevel = 100;
  numZones = 0;
  name[0] = 0;
  guild[0] = 0;
  SStrCopy(filter, str, sizeof(filter));
  UINT length = SStrLen(filter);
  filter[length] = ' ';
  filter[length + 1] = 0;

  int   w = 0;
  int   i = 0;
  char *c = filter;
  while (isspace(*c)) {
    ++c;
  }

  int quoted = 0;
  while (*c) {
    if (*c == '"' || *c == '\'') {
      quoted = !quoted;
    }
    if (!quoted && isspace(*c)) {
      while (isspace(*c)) {
        ++c;
      }
      words[w][i] = 0;
      char *word = StripQuotes(words[w]);
      wordptrs[w] = word;

      LPCSTR tag = FrameScript_GetText("WHO_TAG_NAME", -1, GENDER_NOT_APPLICABLE);
      if (!SStrCmpI(words[w], tag, SStrLen(tag))) {
        word += SStrLen(tag);
        word = StripQuotes(word);
        SStrCopy(name, word, sizeof(name));
        --w;
      } else {
        tag = FrameScript_GetText("WHO_TAG_GUILD", -1, GENDER_NOT_APPLICABLE);
        if (!SStrCmpI(words[w], tag, SStrLen(tag))) {
          word += SStrLen(tag);
        word = StripQuotes(word);
          SStrCopy(guild, word, 48);
          --w;
        } else {
          tag = FrameScript_GetText("WHO_TAG_ZONE", -1, GENDER_NOT_APPLICABLE);
          if (!SStrCmpI(words[w], tag, SStrLen(tag))) {
            word += SStrLen(tag);
        word = StripQuotes(word);
            if (numZones < 10) {
              UINT numEntries = g_areaTableDB.GetNumRecords();
              for (UINT j = 0; j < numEntries; ++j) {
                const AreaTableRec *rec = g_areaTableDB.GetRecordByIndex(j);
                if (!rec->m_ParentAreaNum && numZones < 10 && SStrStrI(rec->m_AreaName_lang[CURRENT_LANGUAGE], word)) {
                  zones[numZones++] = rec->m_ID;
                }
              }
            }
            if (!numZones) {
              zones[numZones++] = 0;
            }
            --w;
          } else {
            tag = FrameScript_GetText("WHO_TAG_RACE", -1, GENDER_NOT_APPLICABLE);
            if (!SStrCmpI(words[w], tag, SStrLen(tag))) {
              word += SStrLen(tag);
        word = StripQuotes(word);
              if (raceFilter == -1) {
                raceFilter = 0;
              }
              UINT numEntries = g_chrRacesDB.GetNumRecords();
              for (UINT j = 0; j < numEntries; ++j) {
                const ChrRacesRec *rec = g_chrRacesDB.GetRecordByIndex(j);
                if (SStrStrI(rec->m_name_lang[CURRENT_LANGUAGE], word)) {
                  raceFilter |= 1 << rec->m_ID;
                }
              }
              --w;
            } else {
              tag = FrameScript_GetText("WHO_TAG_CLASS", -1, GENDER_NOT_APPLICABLE);
              if (!SStrCmpI(words[w], tag, SStrLen(tag))) {
                word += SStrLen(tag);
        word = StripQuotes(word);
                if (classFilter == -1) {
                  classFilter = 0;
                }
                UINT numEntries = g_chrClassesDB.GetNumRecords();
                for (UINT j = 0; j < numEntries; ++j) {
                  const ChrClassesRec *rec = g_chrClassesDB.GetRecordByIndex(j);
                  if (SStrStrI(rec->m_name_lang[CURRENT_LANGUAGE], word)) {
                    classFilter |= 1 << rec->m_ID;
                  }
                }
                --w;
              } else {
                bool matchedLevel = false;
                word = 0;
                if (isdigit(words[w][0])) {
                  minLevel = SStrToInt(words[w]);
                  maxLevel = minLevel;
                  matchedLevel = true;
                  word = &words[w][1];
                  while (1) {
                    if (!isdigit(*word) || *word == '-') {
                      break;
                    }
                    ++word;
                  }
                  if (*word == '-') {
                    ++word;
                    maxLevel = 100;
                  }
                } else if (words[w][0] == '-') {
                  word = &words[w][1];
                }
                if (word && isdigit(*word)) {
                  maxLevel = SStrToInt(word);
                  matchedLevel = true;
                }
                if (matchedLevel) {
                  --w;
                }
              }
            }
          }
        }
      }

      ++w;
      i = 0;
      if (w >= sizeof(wordptrs) / sizeof(wordptrs[0])) {
        break;
      }
    } else {
      words[w][i++] = *c++;
    }
  }

  CDataStore msg;
  msg.Put(CMSG_WHO);
  msg.Put(minLevel);
  msg.Put(maxLevel);
  msg.PutString(name);
  msg.PutString(guild);
  msg.Put(raceFilter);
  msg.Put(classFilter);
  msg.Put(numZones);
  for (UINT z = 0; z < numZones; ++z) {
    msg.Put(zones[z]);
  }
  msg.Put(w);
  for (int n = 0; n < w; ++n) {
    msg.PutString(wordptrs[n]);
  }
  msg.Finalize();
  ClientServices_Send(&msg);
}

void FriendList::AddOrDelIgnore(LPCSTR name) {
  for (UINT i = 0; i < 25; ++i) {
    if (!m_ignore[i])
      continue;
    const NameCache *entry = g_nameDBCache.GetRecord(m_ignore[i], 0, 0, 0);
    if (entry && !SStrCmpI(entry->m_name, name, 0x7FFFFFFF)) {
      CDataStore msg;
      msg.Put(CMSG_DEL_IGNORE);
      msg.Put(m_ignore[i]);
      msg.Finalize();
      ClientServices_Send(&msg);
      return;
    }
  }
  CDataStore msg;
  msg.Put(CMSG_ADD_IGNORE);
  msg.PutString(name);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void FriendList::AddIgnore(LPCSTR name) {
  for (UINT i = 0; i < 25; ++i) {
    if (!m_ignore[i])
      continue;
    const NameCache *entry = g_nameDBCache.GetRecord(m_ignore[i], 0, 0, 0);
    if (entry && !SStrCmpI(entry->m_name, name, 0x7FFFFFFF)) {
      CGGameUI::DisplayError(GERR_IGNORE_ALREADY_S, entry->m_name);
      return;
    }
  }
  CDataStore msg;
  msg.Put(CMSG_ADD_IGNORE);
  msg.PutString(name);
  msg.Finalize();
  ClientServices_Send(&msg);
}
