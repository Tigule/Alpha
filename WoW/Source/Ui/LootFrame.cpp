#include "LootFrame.h"

#include "DB/DBClient/DBCacheInstances.h"
#include "DB/DBClient/DBClient.h"
#include "DB/DBClient/AutoCode/ItemDisplayInfoRec.h"
#include "FrameScript/FrameScript.h"
#include "Object/ItemStats.h"
#include "Object/ObjectClient/Object_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "SoundInterface/SoundInterface.h"
#include "Ui/GameUI.h"

#include <Frame/CSimpleRender.h>
#include <lauxlib.h>
#include <lua.h>
#include <string.h>

#ifdef GetObject
#undef GetObject
#endif

class CGUnit_C;

void SetPortraitTexture(CSimpleTexture *texture, const CGUnit_C *unit);
void CurrencyBreakdown(int money, int *coins);
const char *CurrencyAbbreviation(int coinType);

static char buffer[260];
static char moneyBuf[128];

unsigned __int64 CGLootInfo::m_object;
int              CGLootInfo::m_coins;
CGLootSlot       CGLootInfo::m_loot[16];
LOOT_ACQUIRE     CGLootInfo::m_lootType;
unsigned int     CGLootInfo::m_itemsPending;

void CGLootInfo::InitializeGame() {
  m_object = 0;
}

void CGLootInfo::ShutdownGame() {
}

void CGLootInfo::EnterWorld() {
}

void CGLootInfo::LeaveWorld() {
  CGGameUI::CloseLoot(1, 0);
}

void CGLootInfo::SetObject(CGObject_C *object, int coins, LOOT_ACQUIRE lootType) {
  if (m_object) {
    for (unsigned int index = 0; index < 16; ++index) {
      if (m_loot[index].pending) {
        g_itemDBCache.CancelCallback(m_loot[index].itemID, LootButtonItemStatsCallback, 0);
      }
    }
    m_object = 0;
    FrameScript_SignalEvent(252);
  }

  if (object) {
    m_object = object->GetGUID();
    m_lootType = lootType;
    memset(m_loot, 0, sizeof(m_loot));
    m_coins = coins;
    m_itemsPending = 0;

    unsigned int itemCount = 0;
    for (unsigned int slot = 0; slot < 16; ++slot) {
      unsigned int itemID = CGPlayer_C::GetLootItem(slot);
      if (itemID) {
        CGLootSlot &loot = m_loot[itemCount++];
        loot.itemID = itemID;
        loot.itemDisplayID = CGPlayer_C::GetLootItemDisplayID(slot);
        loot.quantity = CGPlayer_C::GetLootItemQuantity(slot);
        loot.slot = slot;
      }
    }

    for (unsigned int index = 0; index < itemCount; ++index) {
      CGLootSlot &loot = m_loot[index];
      if (g_itemDBCache.GetRecord(loot.itemID, m_object, LootButtonItemStatsCallback, 0)) {
        loot.pending = 0;
      } else {
        loot.pending = 1;
        ++m_itemsPending;
      }
    }

    if (!m_itemsPending) {
      FrameScript_SignalEvent(250);
    }
  }
}

void CGLootInfo::ClearSlot(unsigned char _slot) {
  unsigned int index;

  for (index = 0; index < 16; ++index) {
    if (m_loot[index].itemID && m_loot[index].slot == _slot) {
      if (m_loot[index].pending) {
        g_itemDBCache.CancelCallback(m_loot[index].itemID, LootButtonItemStatsCallback, 0);
      }

      memset(&m_loot[index], 0, sizeof(m_loot[index]));
      if (m_coins) {
        ++index;
      }
      FrameScript_SignalEvent(251, "%d", index + 1);
      if (!HasLoot()) {
        CGGameUI::CloseLoot(1, 0);
      }
      return;
    }
  }
}

int CGLootInfo::GetNumItems() {
  if (!m_object) {
    return 0;
  }

  int count = 0;

  for (unsigned int index = 0; index < 16; ++index) {
    if (m_loot[index].itemID > 0) {
      count = index + 1;
    }
  }

  if (m_coins) {
    ++count;
  }

  return count;
}

int CGLootInfo::GetLootItem(unsigned int slot) {
  if (!m_object) {
    return 0;
  }
  if (m_coins) {
    if (!slot) {
      return 0;
    }
    --slot;
  }
  FATALASSERT(slot < (sizeof(m_loot) / sizeof(m_loot[0])));
  return m_loot[slot].itemID;
}

int CGLootInfo::GetLootQuantity(unsigned int slot) {
  if (!m_object) {
    return 0;
  }
  if (m_coins) {
    if (!slot) {
      return 0;
    }
    --slot;
  }
  FATALASSERT(slot < (sizeof(m_loot) / sizeof(m_loot[0])));
  return m_loot[slot].quantity;
}

int CGLootInfo::GetLootQuality(unsigned int slot) {
  if (!m_object) {
    return 0;
  }
  if (m_coins) {
    if (!slot) {
      return 0;
    }
    --slot;
  }
  FATALASSERT(slot < (sizeof(m_loot) / sizeof(m_loot[0])));
  const unsigned __int64 noGuid = 0;
  const ItemStats_C *stats = g_itemDBCache.GetRecord(m_loot[slot].itemID, noGuid, 0, 0);
  return stats && stats->m_inventoryType ? stats->m_overallQualityID : -1;
}

int CGLootInfo::GetLootCoin(unsigned int slot) {
  return m_object && m_coins > 0 && !slot ? m_coins : 0;
}

const char *CGLootInfo::GetLootSlotTexture(unsigned int slot) {
  if (!m_object) {
    return 0;
  }

  const char *path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
  SStrPrintf(buffer, sizeof(buffer), "%s%s", path, *path ? "\\" : "");

  if (m_coins) {
    if (!slot) {
      if (m_coins < 0) {
        return 0;
      }
      const char *icon;
      if (m_coins < 10) {
        icon = "INV_Misc_Coin_05";
      } else if (m_coins < 100) {
        icon = "INV_Misc_Coin_06";
      } else if (m_coins < 1000) {
        icon = "INV_Misc_Coin_03";
      } else if (m_coins < 10000) {
        icon = "INV_Misc_Coin_04";
      } else if (m_coins < 100000) {
        icon = "INV_Misc_Coin_01";
      } else {
        icon = "INV_Misc_Coin_02";
      }
      SStrPack(buffer, icon, sizeof(buffer));
      return buffer;
    }
    --slot;
  }

  FATALASSERT(slot < (sizeof(m_loot) / sizeof(m_loot[0])));
  if (!m_loot[slot].itemID) {
    return 0;
  }
  const ItemDisplayInfoRec *displayInfo = g_itemDisplayInfoDB.GetRecord(m_loot[slot].itemDisplayID);
  SStrPack(
      buffer, displayInfo && displayInfo->m_inventoryIcon && *displayInfo->m_inventoryIcon ? displayInfo->m_inventoryIcon : "INV_Misc_QuestionMark",
      sizeof(buffer)
  );
  return buffer;
}

const char *CGLootInfo::GetLootSlotText(unsigned int slot) {
  if (!m_object) {
    return 0;
  }
  if (m_coins) {
    if (!slot) {
      if (m_coins < 0) {
        return 0;
      }
      char buf[64];
      char coinName[32];
      int  coins[3];
      CurrencyBreakdown(m_coins, coins);
      int first = 1;
      for (int coin = 2; coin >= 0; --coin) {
        if (coins[coin]) {
          SStrCopy(coinName, FrameScript_GetText(CurrencyAbbreviation(coin), -1, GENDER_NOT_APPLICABLE), sizeof(coinName));
          if (first) {
            SStrPrintf(moneyBuf, sizeof(moneyBuf), "%d %s%s", coins[coin], coinName, coin > 0 ? "\n" : "");
            first = 0;
          } else {
            SStrPrintf(buf, sizeof(buf), "%d %s%s", coins[coin], coinName, coin > 0 ? "\n" : "");
            SStrPack(moneyBuf, buf, sizeof(moneyBuf));
          }
        }
      }
      return moneyBuf;
    }
    --slot;
  }

  FATALASSERT(slot < (sizeof(m_loot) / sizeof(m_loot[0])));
  if (!m_loot[slot].itemID) {
    return 0;
  }
  const ItemStats_C *stats = g_itemDBCache.GetRecord(m_loot[slot].itemID, m_object, 0, 0);
  return stats ? stats->m_displayName[0] : 0;
}

const char *CGLootInfo::GetLootSlotLink(unsigned int slot, char *link, unsigned int size) {
  if (!m_object) {
    return 0;
  }
  if (m_coins) {
    if (!slot) {
      return 0;
    }
    --slot;
  }
  FATALASSERT(slot < (sizeof(m_loot) / sizeof(m_loot[0])));
  int itemID = m_loot[slot].itemID;
  if (!itemID) {
    return 0;
  }
  const ItemStats_C *stats = g_itemDBCache.GetRecord(itemID, m_object, 0, 0);
  if (!stats) {
    return 0;
  }
  SStrPrintf(link, size, "|Hitem:%d|h[%s]|h", itemID, stats->m_displayName[0]);
  return link;
}

LOOT_ACQUIRE CGLootInfo::GetLootType() {
  return m_lootType;
}

void CGLootInfo::CoinsCleared() {
  if (m_coins > 0) {
    m_coins = -1;
    FrameScript_SignalEvent(251, "%d", 1);
  }
}

int CGLootInfo::LootSlot(unsigned int slot, int force) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 1;
  }

  unsigned int origSlot = slot;
  if (m_coins) {
    if (!slot) {
      if (m_coins > 0) {
        SndInterfacePlayInterfaceSound("LOOTWINDOWCOINSOUND");
        player->LootMoney();
        m_coins = -1;
        FrameScript_SignalEvent(251, "%d", 1);
        if (!HasLoot()) {
          CGGameUI::CloseLoot(1, 0);
        }
      }
      return 1;
    }
    --slot;
  }

  FATALASSERT(slot < (sizeof(m_loot) / sizeof(m_loot[0])));
  if (m_loot[slot].itemID) {
    const ItemStats_C *stats = g_itemDBCache.GetRecord(m_loot[slot].itemID, m_object, 0, 0);
    FATALASSERT(stats);
    if (stats->m_bonding == 1 && !force) {
      FrameScript_SignalEvent(268, "%u", origSlot + 1);
      return 1;
    }
    SndInterfacePlayItemSound(ITEMSOUND_PICKUP, stats->m_displayInfoID);
    player->AutoStoreLootItem(m_loot[slot].slot);
  }
  return 1;
}

int CGLootInfo::HasLoot() {
  FATALASSERT(m_object);

  if (m_coins > 0) {
    return 1;
  }

  for (unsigned int index = 0; index < 16; ++index) {
    if (m_loot[index].itemID) {
      return 1;
    }
  }

  return 0;
}

void CGLootInfo::LootButtonItemStatsCallback(int id, const unsigned __int64 &, void *, bool) {
  for (unsigned int index = 0; index < 16; ++index) {
    if (m_loot[index].pending && m_loot[index].itemID == id) {
      m_loot[index].pending = 0;
    }
  }

  if (!--m_itemsPending) {
    FrameScript_SignalEvent(250);
  }
}

static int Script_SetLootPortrait(lua_State *L) {
  CSimpleTexture *texture = 0;
  if (lua_type(L, 1) == LUA_TTABLE) {
    lua_rawgeti(L, 1, 0);
    texture = static_cast<CSimpleTexture *>(lua_touserdata(L, -1));
    lua_pop(L, 1);
  } else {
    return luaL_error(L, "Attempt to find 'this' in non-table object (used '.' instead of ':' ?)");
  }
  FATALASSERT(texture);

  CGObject_C *object = ClntObjMgrObjectPtr(CGLootInfo::GetObject(), __FILE__, __LINE__);
  if (object && (object->GetType() & TYPE_UNIT)) {
    SetPortraitTexture(texture, static_cast<CGUnit_C *>(object));
    lua_pushnumber(L, 1.0);
  } else {
    texture->SetTexture(0);
    lua_pushnil(L);
  }
  return 1;
}

static int Script_GetNumLootItems(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGLootInfo::GetNumItems()));
  return 1;
}

static int Script_GetLootSlotInfo(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetLootSlotInfo(slot)");
  }
  unsigned int slot = static_cast<unsigned int>(lua_tonumber(L, 1)) - 1;
  lua_pushstring(L, CGLootInfo::GetLootSlotTexture(slot));
  lua_pushstring(L, CGLootInfo::GetLootSlotText(slot));
  lua_pushnumber(L, static_cast<double>(CGLootInfo::GetLootQuantity(slot)));
  lua_pushnumber(L, static_cast<double>(CGLootInfo::GetLootQuality(slot)));
  return 4;
}

static int Script_GetLootSlotLink(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetLootSlotLink(slot)");
  }
  char link[1024];
  lua_pushstring(L, CGLootInfo::GetLootSlotLink(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1, link, sizeof(link)));
  return 1;
}

static int Script_LootSlotIsItem(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: LootSlotIsItem(slot)");
  }
  if (CGLootInfo::GetLootItem(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1) > 0) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_LootSlotIsCoin(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: LootSlotIsCoin(slot)");
  }
  if (CGLootInfo::GetLootCoin(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1) > 0) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_LootSlot(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: LootSlot(slot [, force])");
  }
  unsigned int slot = static_cast<unsigned int>(lua_tonumber(L, 1)) - 1;
  int          force = lua_isnumber(L, 2) ? static_cast<int>(lua_tonumber(L, 2)) : 0;
  lua_pushnumber(L, static_cast<double>(CGLootInfo::LootSlot(slot, force)));
  return 1;
}

static int Script_CloseLoot(lua_State *L) {
  CGGameUI::CloseLoot(1, 0);
  int displayError = 0;
  if (lua_isnumber(L, 1)) {
    displayError = static_cast<int>(lua_tonumber(L, 1));
  } else if (lua_isstring(L, 1)) {
    displayError = lua_toboolean(L, 1);
  }
  if (displayError) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(122));
  }
  return 0;
}

static int Script_IsFishingLoot(lua_State *L) {
  if (CGLootInfo::GetLootType() == LOOT_ACQUIRE_FISHING) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static FrameScript_Method s_ScriptFunctions[9] = {
    {"SetLootPortrait", Script_SetLootPortrait},
    {"GetNumLootItems", Script_GetNumLootItems},
    {"GetLootSlotInfo", Script_GetLootSlotInfo},
    {"GetLootSlotLink", Script_GetLootSlotLink},
    { "LootSlotIsItem",  Script_LootSlotIsItem},
    { "LootSlotIsCoin",  Script_LootSlotIsCoin},
    {       "LootSlot",        Script_LootSlot},
    {      "CloseLoot",       Script_CloseLoot},
    {  "IsFishingLoot",   Script_IsFishingLoot}
};

void LootInfoRegisterScriptFunctions() {
  for (unsigned int i = 0; i < 9; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void LootInfoUnregisterScriptFunctions() {
  for (unsigned int i = 0; i < 9; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
