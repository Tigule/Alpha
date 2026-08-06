#ifndef WOW_SOURCE_DB_DBCLIENT_DBCACHEINSTANCES_H
#define WOW_SOURCE_DB_DBCLIENT_DBCACHEINSTANCES_H

#include "DB/DBClient/DBCache.h"
#include "Game/GameClient/NameCache.h"
#include "Game/GameClient/PetNameCache.h"
#include "Game/GameClient/QuestCache.h"
#include "Object/CreatureStats.h"
#include "Object/GameObjectStats.h"
#include "Object/GuildStats.h"
#include "Object/ItemStats.h"
#include "Object/NPCText.h"
#include "Object/PageTextCache.h"
#include "Object/Petition.h"
#include "ObjectMgrClient/ObjectMgrClient.h"

extern DBCache<CreatureStats_C, int, HASHKEY_INT>   g_creatureDBCache;
extern DBCache<GameObjectStats_C, int, HASHKEY_INT> g_gameObjectDBCache;
extern DBCache<ItemStats_C, int, HASHKEY_INT>       g_itemDBCache;
extern DBCache<NPCText, int, HASHKEY_INT>           g_npcTextDBCache;
extern DBCache<NameCache, DWORDLONG, CHashKeyGUID>  g_nameDBCache;
extern DBCache<GuildStats_C, int, HASHKEY_INT>      g_guildInfoCache;
extern DBCache<QuestCache, int, HASHKEY_INT>        g_questDBCache;
extern DBCache<PageTextCache_C, int, HASHKEY_INT>   g_pageTextCache;
extern DBCache<PetNameCache, int, HASHKEY_INT>      g_petNameCache;
extern DBCache<CGPetition, int, HASHKEY_INT>        g_petitionCache;

void DBCache_Initialize();
void DBCache_Destroy();
void DBCache_RegisterHandlers();
void DBCache_ClearHandlers();

#endif
