#include "WowSvcs/WowSvcsClient/FriendList.h"

#include "Console/ConsoleCommand.h"
#include "DB/DBClient/AutoCode/AreaTableRec.h"
#include "DB/DBClient/AutoCode/ChrClassesRec.h"
#include "DB/DBClient/AutoCode/ChrRacesRec.h"
#include "DB/WowLocale.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <Base/CDataStore.h>
#include <FrameScript/FrameScript.h>

extern FrameScript_Method s_FriendListScriptFunctions[19];

#include <ctype.h>
#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <string.h>

FriendList *g_friendList;

struct WhoListEntry {
  char name[48];
  char guild[96];
  int  level;
  int  raceID;
  int  classID;
  int  areaID;
  int  partyStatus;
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

static int __fastcall FriendListStatusHandler(void*, NETMESSAGE, unsigned long, CDataStore* msg) {
    // TODO: implement
    return 0;
}

static void FriendListNameCallbackWithSort(int id, const unsigned __int64& guid, void* arg, unsigned char granted) {
    // TODO: implement
}

static void IgnoreListNameCallback(int id, const unsigned __int64& guid, void* arg, unsigned char granted) {
    // TODO: implement
}

FriendList::~FriendList() {
}

static int __fastcall FriendListHandler(void*, NETMESSAGE, unsigned long, CDataStore* msg) {
    // TODO: implement
    return 0;
}

static int __fastcall CCommand_Friends(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int __fastcall CCommand_AddFriend(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int __fastcall CCommand_RemoveFriend(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int __fastcall WhoisResponseHandler(void*, NETMESSAGE, unsigned long, CDataStore* msg) {
    // TODO: implement
    return 0;
}

void FriendList::RemoveFriend(unsigned int index) {
  Friend *entry = GetFriend(index);
  if (!entry) {
    return;
  }
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_DEL_FRIEND));
  msg.Put(entry->guid);
  msg.Finalize();
  ClientServices_Send(&msg);
}

static int __fastcall ReverseWhoisResponseHandler(void*, NETMESSAGE, unsigned long, CDataStore* msg) {
    // TODO: implement
    return 0;
}

static int __fastcall CCommand_Whois(const char*, const char* args) {
    // TODO: implement
    return 0;
}

static int __fastcall CCommand_RWhois(const char*, const char* args) {
    // TODO: implement
    return 0;
}

static int __fastcall Script_GetNumFriends(lua_State *L) {
  lua_pushnumber(L, g_friendList ? g_friendList->GetNumFriends() : 0);
  return 1;
}

static int __fastcall Script_GetFriendInfo(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetFriendInfo(index)");
  }
  FriendList::Friend *entry = g_friendList ? g_friendList->GetFriend(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1) : 0;
  if (!entry) {
    return 0;
  }
  lua_pushstring(L, entry->m_name ? entry->m_name : "");
  lua_pushnumber(L, entry->m_level);
  ChrClassesRec *classRec = g_chrClassesDB.GetRecord(entry->m_class);
  lua_pushstring(L, classRec ? classRec->m_name_lang[CURRENT_LANGUAGE] : "");
  AreaTableRec *areaRec = g_areaTableDB.GetRecord(entry->m_area);
  lua_pushstring(L, areaRec ? areaRec->m_AreaName_lang[CURRENT_LANGUAGE] : "");
  lua_pushnumber(L, entry->m_connected != 0);
  lua_pushstring(L, "");
  return 6;
}

static int __fastcall Script_SetSelectedFriend(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: SetSelectedFriend(index)");
  }
  if (g_friendList) {
    g_friendList->SetFriendSelectionIndex(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  }
  return 0;
}

static int __fastcall Script_GetSelectedFriend(lua_State *L) {
  lua_pushnumber(L, g_friendList ? g_friendList->GetFriendSelectionIndex() + 1 : 0);
  return 1;
}

void FriendList::DelIgnore(const char *name) {
  if (!name || !*name) {
    return;
  }
  for (unsigned int i = 0; i < GetNumIgnores(); ++i) {
    NameCache *entry = const_cast<NameCache *>(g_nameDBCache.GetRecord(m_ignore[i], m_ignore[i], 0, 0));
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

static int __fastcall Script_AddFriend(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: AddFriend(\"name\")");
  }
  if (g_friendList) {
    g_friendList->AddFriend(lua_tostring(L, 1));
  }
  return 0;
}

static int __fastcall Script_RemoveFriend(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: RemoveFriend(index)");
  }
  if (g_friendList) {
    g_friendList->RemoveFriend(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  }
  return 0;
}

static int __fastcall Script_ShowFriends(lua_State *L) {
  if (g_friendList) {
    g_friendList->ShowFriends();
  }
  return 0;
}

static int __fastcall Script_SendWho(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: SendWho(\"filter\")");
  }
  if (g_friendList) {
    g_friendList->SendWho(lua_tostring(L, 1));
  }
  return 0;
}

static int __fastcall Script_GetNumIgnores(lua_State *L) {
  lua_pushnumber(L, g_friendList ? g_friendList->GetNumIgnores() : 0);
  return 1;
}

static int __fastcall Script_GetIgnoreName(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetIgnoreName(index)");
  }
  unsigned __int64 guid = g_friendList ? g_friendList->GetIgnore(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1) : 0;
  NameCache       *entry = guid ? const_cast<NameCache *>(g_nameDBCache.GetRecord(guid, guid, 0, 0)) : 0;
  if (entry) {
    lua_pushstring(L, entry->m_name);
    return 1;
  }
  return 0;
}

static int __fastcall Script_SetSelectedIgnore(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: SetSelectedIgnore(index)");
  }
  if (g_friendList) {
    g_friendList->SetIgnoreSelectionIndex(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  }
  return 0;
}

static int __fastcall Script_GetSelectedIgnore(lua_State *L) {
  lua_pushnumber(L, g_friendList ? g_friendList->GetIgnoreSelectionIndex() + 1 : 0);
  return 1;
}

static int __fastcall Script_AddOrDelIgnore(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: AddOrDelIgnore(\"name\")");
  }
  if (g_friendList) {
    g_friendList->AddOrDelIgnore(lua_tostring(L, 1));
  }
  return 0;
}

static int __fastcall Script_AddIgnore(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: AddIgnore(\"name\")");
  }
  if (g_friendList) {
    g_friendList->AddIgnore(lua_tostring(L, 1));
  }
  return 0;
}

static int __fastcall Script_DelIgnore(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: DelIgnore(\"name\")");
  }
  if (g_friendList) {
    g_friendList->DelIgnore(lua_tostring(L, 1));
  }
  return 0;
}

static int __fastcall Script_GetNumWhoResults(lua_State *L) {
  lua_pushnumber(L, s_numWhos);
  lua_pushnumber(L, s_totalNumWhos);
  return 2;
}

static int __fastcall Script_GetWhoInfo(lua_State *L) {
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
  ChrRacesRec *race = g_chrRacesDB.GetRecord(entry.raceID);
  lua_pushstring(L, race ? race->m_name_lang[CURRENT_LANGUAGE] : "");
  ChrClassesRec *playerClass = g_chrClassesDB.GetRecord(entry.classID);
  lua_pushstring(L, playerClass ? playerClass->m_name_lang[CURRENT_LANGUAGE] : "");
  AreaTableRec *area = g_areaTableDB.GetRecord(entry.areaID);
  lua_pushstring(L, area ? area->m_AreaName_lang[CURRENT_LANGUAGE] : "");
  lua_pushstring(L, entry.partyStatus == 0 ? "" : (entry.partyStatus == 1 ? "LFG" : "FULL"));
  return 7;
}

static int __fastcall Script_SetWhoToUI(lua_State *L) {
  s_whoToUI = lua_toboolean(L, 1);
  return 0;
}

static int __cdecl QSortWho(const void *a, const void *b) {
  const WhoListEntry *entry1 = static_cast<const WhoListEntry *>(a);
  const WhoListEntry *entry2 = static_cast<const WhoListEntry *>(b);
  return SStrCmpI(entry1->name, entry2->name, 0x7FFFFFFF);
}

static int __fastcall Script_SortWho(lua_State *L) {
  qsort(s_whoList, s_numWhos, sizeof(WhoListEntry), QSortWho);
  return 0;
}

void __fastcall FriendList::RegisterScriptFunctions() {
  for (int i = 0; i < 19; ++i) {
    FrameScript_RegisterFunction(s_FriendListScriptFunctions[i].name, s_FriendListScriptFunctions[i].method);
  }
}

void __fastcall FriendList::UnregisterScriptFunctions() {
  for (int i = 0; i < 19; ++i) {
    FrameScript_UnregisterFunction(s_FriendListScriptFunctions[i].name);
  }
}

static void PrintWho(const char* name, const char* guild, int level, int classID, int raceID, int areaID) {
    // TODO: implement
}

static int __fastcall OnWhoList(void*, NETMESSAGE msgId, unsigned long eventTime, CDataStore* msg) {
    // TODO: implement
    return 0;
}

static int __fastcall OnIgnoreList(void*, NETMESSAGE, unsigned long, CDataStore* msg) {
    // TODO: implement
    return 0;
}

void __fastcall FriendList::Initialize() {
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

void __fastcall FriendList::Destroy() {
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

unsigned int FriendList::GetNumFriends() const {
  unsigned int count = 0;
  while (count < 50 && m_friends[count].guid) {
    ++count;
  }
  return count;
}

FriendList::Friend *FriendList::GetFriend(unsigned int index) {
  return index < GetNumFriends() ? &m_friends[index] : 0;
}

void FriendList::SetFriendSelectionIndex(unsigned int index) {
  Friend *entry = GetFriend(index);
  m_selectedFriend = entry ? entry->guid : 0;
}

int FriendList::GetFriendSelectionIndex() const {
  for (unsigned int i = 0; i < GetNumFriends(); ++i) {
    if (m_friends[i].guid == m_selectedFriend) {
      return i;
    }
  }
  return -1;
}

unsigned int FriendList::GetNumIgnores() const {
  unsigned int count = 0;
  while (count < 25 && m_ignore[count]) {
    ++count;
  }
  return count;
}

unsigned __int64 FriendList::GetIgnore(unsigned int index) const {
  return index < GetNumIgnores() ? m_ignore[index] : 0;
}

void FriendList::SetIgnoreSelectionIndex(unsigned int index) {
  m_selectedIgnore = GetIgnore(index);
}

int FriendList::GetIgnoreSelectionIndex() const {
  for (unsigned int i = 0; i < GetNumIgnores(); ++i) {
    if (m_ignore[i] == m_selectedIgnore) {
      return i;
    }
  }
  return -1;
}

void FriendList::ShowFriends() {
  FrameScript_SignalEvent(250);
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

static int QSortFriends(const void* a, const void* b) {
    // TODO: implement
    return 0;
}

static int QSortIgnore(const void* a, const void* b) {
    // TODO: implement
    return 0;
}

char *__fastcall StripQuotes(char *string) {
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
        AreaTableRec *rec = g_areaTableDB.GetRecordByIndex(index);
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
        ChrRacesRec *rec = g_chrRacesDB.GetRecordByIndex(index);
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
        ChrClassesRec *rec = g_chrClassesDB.GetRecordByIndex(index);
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
    NameCache *entry = const_cast<NameCache *>(g_nameDBCache.GetRecord(m_ignore[i], m_ignore[i], 0, 0));
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
