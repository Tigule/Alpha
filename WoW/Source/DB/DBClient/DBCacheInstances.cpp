#include "DB/DBClient/DBCacheInstances.h"

#include <Base/CDataStore.h>

#include "WowSvcs/WowSvcsClient/ClientServices.h"

static void LoadDBCaches();

static int ReceiveCreature(void *, NETMESSAGE, unsigned long, CDataStore *msg);
static int ReceiveGameObject(void *, NETMESSAGE, unsigned long, CDataStore *msg);
static int ReceiveSingleItem(void *, NETMESSAGE, unsigned long, CDataStore *msg);
static int ReceiveMultipleItems(void *, NETMESSAGE, unsigned long, CDataStore *msg);
static int ReceiveNPCText(void *, NETMESSAGE, unsigned long, CDataStore *msg);
static int ReceiveName(void *, NETMESSAGE, unsigned long, CDataStore *msg);
static int ReceiveGuildInfo(void *, NETMESSAGE, unsigned long, CDataStore *msg);
static int ReceiveQuest(void *, NETMESSAGE, unsigned long, CDataStore *msg);
static int ReceivePageText(void *, NETMESSAGE, unsigned long, CDataStore *msg);
static int ReceivePetName(void *, NETMESSAGE, unsigned long, CDataStore *msg);
static int ReceivePetition(void *, NETMESSAGE, unsigned long, CDataStore *msg);

DBCache<CreatureStats_C, int, HASHKEY_INT> g_creatureDBCache(0x574D4F42, "creaturecache.wdb", CMSG_CREATURE_QUERY, MSG_NULL_ACTION, true, true);
DBCache<GameObjectStats_C, int, HASHKEY_INT>
                                       g_gameObjectDBCache(0x57474F42, "gameobjectcache.wdb", CMSG_GAMEOBJECT_QUERY, MSG_NULL_ACTION, true, true);
DBCache<ItemStats_C, int, HASHKEY_INT> g_itemDBCache(0x57494442, "itemcache.wdb", CMSG_ITEM_QUERY_SINGLE, CMSG_ITEM_QUERY_MULTIPLE, true, true);
DBCache<NPCText, int, HASHKEY_INT>     g_npcTextDBCache(0x574E5043, "npccache.wdb", CMSG_NPC_TEXT_QUERY, MSG_NULL_ACTION, true, true);
DBCache<NameCache, unsigned __int64, CHashKeyGUID> g_nameDBCache(0x574E414D, "namecache.wdb", CMSG_NAME_QUERY, MSG_NULL_ACTION, false, false);
DBCache<GuildStats_C, int, HASHKEY_INT>            g_guildInfoCache(0x57474C44, "guildcache.wdb", CMSG_GUILD_QUERY, MSG_NULL_ACTION, false, false);
DBCache<QuestCache, int, HASHKEY_INT>              g_questDBCache(0x57515354, "questcache.wdb", CMSG_QUEST_QUERY, MSG_NULL_ACTION, false, true);
DBCache<PageTextCache_C, int, HASHKEY_INT> g_pageTextCache(0x57505458, "pagetextcache.wdb", CMSG_PAGE_TEXT_QUERY, MSG_NULL_ACTION, true, true);
DBCache<PetNameCache, int, HASHKEY_INT>    g_petNameCache(0x57504E4D, "petnamecache.wdb", CMSG_PET_NAME_QUERY, MSG_NULL_ACTION, true, false);
DBCache<CGPetition, int, HASHKEY_INT>      g_petitionCache(0x5750544E, "petitioncache.wdb", CMSG_PETITION_QUERY, MSG_NULL_ACTION, true, false);

static void LoadDBCaches() {
  g_creatureDBCache.Load();
  g_gameObjectDBCache.Load();
  g_itemDBCache.Load();
  g_npcTextDBCache.Load();
  g_nameDBCache.Load();
  g_guildInfoCache.Load();
  g_questDBCache.Load();
  g_pageTextCache.Load();
  g_petNameCache.Load();
  g_petitionCache.Load();
}

static int ReceiveCreature(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
  g_creatureDBCache.AddItems(msg, true);
  return 1;
}

static int ReceiveGameObject(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
  g_gameObjectDBCache.AddItems(msg, true);
  return 1;
}

static int ReceiveSingleItem(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
  g_itemDBCache.AddItems(msg, true);
  return 1;
}

static int ReceiveMultipleItems(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
  g_itemDBCache.AddItems(msg, false);
  return 1;
}

static int ReceiveNPCText(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
  g_npcTextDBCache.AddItems(msg, true);
  return 1;
}

static int ReceiveName(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
  NameCache nc;

  msg->Get(nc.m_guid);
  msg->GetString(nc.m_name, sizeof(nc.m_name));
  msg->Get(nc.m_race);
  msg->Get(nc.m_sex);
  msg->Get(nc.m_class);

  if (nc.m_name[0]) {
    g_nameDBCache.AddItem(&nc, nc.m_guid);
    if ((nc.m_guid & 0xF000000000000000ui64) == 0x9000000000000000ui64) {
      g_nameDBCache.SetTemporary(nc.m_guid);
    }
  } else {
    g_nameDBCache.DenyItem(nc.m_guid);
  }

  return 1;
}

static int ReceiveGuildInfo(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
  GuildStats_C guildStats;

  guildStats.Unpack(msg);
  if (guildStats.m_guildName[0]) {
    g_guildInfoCache.AddItem(&guildStats, guildStats.m_guildID);
  } else {
    g_guildInfoCache.DenyItem(guildStats.m_guildID);
  }

  return 1;
}

static int ReceiveQuest(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
  QuestCache qc;

  qc.Unpack(msg);
  if (qc.m_logTitle[0]) {
    g_questDBCache.AddItem(&qc, qc.m_questId);
  } else {
    g_questDBCache.DenyItem(qc.m_questId);
  }

  return 1;
}

static int ReceivePageText(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
  g_pageTextCache.AddItems(msg, true);
  return 1;
}

static int ReceivePetName(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
  PetNameCache pnc;

  msg->Get(pnc.m_ID);
  msg->GetString(pnc.m_name, sizeof(pnc.m_name));
  msg->Get(pnc.m_timestamp);

  if (pnc.m_name[0]) {
    g_petNameCache.AddItem(&pnc, pnc.m_ID);
  } else {
    g_petNameCache.DenyItem(pnc.m_ID);
  }

  return 1;
}

static int ReceivePetition(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
  CGPetition pc;

  pc.Unpack(msg);
  if (pc.m_petitionID > 0) {
    g_petitionCache.AddItem(&pc, pc.m_petitionID);
  } else {
    g_petitionCache.DenyItem(-pc.m_petitionID);
  }

  return 1;
}

void DBCache_Initialize() {
  LoadDBCaches();
}

void DBCache_Destroy() {
}

void DBCache_RegisterHandlers() {
  ClientServices_SetMessageHandler(SMSG_CREATURE_QUERY_RESPONSE, ReceiveCreature, 0);
  ClientServices_SetMessageHandler(SMSG_GAMEOBJECT_QUERY_RESPONSE, ReceiveGameObject, 0);
  ClientServices_SetMessageHandler(SMSG_ITEM_QUERY_SINGLE_RESPONSE, ReceiveSingleItem, 0);
  ClientServices_SetMessageHandler(SMSG_ITEM_QUERY_MULTIPLE_RESPONSE, ReceiveMultipleItems, 0);
  ClientServices_SetMessageHandler(SMSG_NPC_TEXT_UPDATE, ReceiveNPCText, 0);
  ClientServices_SetMessageHandler(SMSG_NAME_QUERY_RESPONSE, ReceiveName, 0);
  ClientServices_SetMessageHandler(SMSG_GUILD_QUERY_RESPONSE, ReceiveGuildInfo, 0);
  ClientServices_SetMessageHandler(SMSG_QUEST_QUERY_RESPONSE, ReceiveQuest, 0);
  ClientServices_SetMessageHandler(SMSG_PAGE_TEXT_QUERY_RESPONSE, ReceivePageText, 0);
  ClientServices_SetMessageHandler(SMSG_PET_NAME_QUERY_RESPONSE, ReceivePetName, 0);
  ClientServices_SetMessageHandler(SMSG_PETITION_QUERY_RESPONSE, ReceivePetition, 0);
}

void DBCache_ClearHandlers() {
  ClientServices_ClearMessageHandler(SMSG_CREATURE_QUERY_RESPONSE);
  ClientServices_ClearMessageHandler(SMSG_GAMEOBJECT_QUERY_RESPONSE);
  ClientServices_ClearMessageHandler(SMSG_ITEM_QUERY_SINGLE_RESPONSE);
  ClientServices_ClearMessageHandler(SMSG_ITEM_QUERY_MULTIPLE_RESPONSE);
  ClientServices_ClearMessageHandler(SMSG_NPC_TEXT_UPDATE);
  ClientServices_ClearMessageHandler(SMSG_NAME_QUERY_RESPONSE);
  ClientServices_ClearMessageHandler(SMSG_GUILD_QUERY_RESPONSE);
  ClientServices_ClearMessageHandler(SMSG_QUEST_QUERY_RESPONSE);
  ClientServices_ClearMessageHandler(SMSG_PAGE_TEXT_QUERY_RESPONSE);
  ClientServices_ClearMessageHandler(SMSG_PET_NAME_QUERY_RESPONSE);
  ClientServices_ClearMessageHandler(SMSG_PETITION_QUERY_RESPONSE);
}
