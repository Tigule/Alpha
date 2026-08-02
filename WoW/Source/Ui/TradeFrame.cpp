#include <string.h>

#include "GameUI.h"
#include "TradeFrame.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "DB/DBClient/DBClient.h"
#include "DB/WowLocale.h"
#include "Object/ItemStats.h"
#include "Object/ObjectClient/Bag_C.h"
#include "Object/ObjectClient/Item_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"

#include <FrameScript/FrameScript.h>
#include <lauxlib.h>
#include <lua.h>
#include <storm.h>

void Trade_C_CancelTrade();
int Trade_C_UseCursorItem();
int Trade_C_GetProposedEnchantment(unsigned int player, int &spellID, int &slot);
unsigned int Trade_C_GetPlayerTradeGold();
unsigned int Trade_C_GetTargetTradeGold();
unsigned __int64 Trade_C_GetTradeTarget();
bool Trade_C_AddItem(unsigned __int64 item, unsigned __int64 itemContainer, unsigned int itemSlot, unsigned int tradeSlot);
void Trade_C_RemoveItem(unsigned int slot);
void Trade_C_AcceptTrade();
void Trade_C_UnacceptTrade();
void Trade_C_AddMoney(unsigned int money);
void Trade_C_RemoveMoney(unsigned int money);
void TradeItemStatsCallback(int id, const unsigned __int64 &guid, void *arg, bool granted);

static const float MAX_TRADE_DISTANCE = 11.111111f;
static const float MAX_TRADE_DISTANCE_SQUARED = MAX_TRADE_DISTANCE * MAX_TRADE_DISTANCE;

unsigned __int64 CGTradeInfo::m_tradingPlayer;
int              CGTradeInfo::m_playerAccepted;
int              CGTradeInfo::m_targetAccepted;
unsigned __int64 CGTradeInfo::m_playerItems[8];
unsigned __int64 CGTradeInfo::m_playerItemBag[8];
unsigned char    CGTradeInfo::m_playerItemSlot[8];
int              CGTradeInfo::m_targetItems[8];
int              CGTradeInfo::m_targetItemCount[8];
int              CGTradeInfo::m_targetItemEnchantment[8];
unsigned __int64 CGTradeInfo::m_targetItemCreator[8];
int              CGTradeInfo::m_playerEnchantSlot = -1;
int              CGTradeInfo::m_targetEnchantSlot = -1;
unsigned int     CGTradeInfo::m_playerMoney;
unsigned int     CGTradeInfo::m_targetMoney;

void CGTradeInfo::HandleTradeMessage(TRADE_STATUS status, BAG_RESULT bagResult, int myFailure, int itemID) {
  switch (status) {
    case TRADE_STATUS_INITIATED:
      SetTradePartner(Trade_C_GetTradeTarget());
      break;
    case TRADE_STATUS_CANCELLED:
      FrameScript_SignalEvent(267);
      UnlockTradeItems();
      SetTradePartner(0);
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(172));
      break;
    case TRADE_STATUS_ACCEPTED:
      PlayerAccept(1);
      TargetAccept(1);
      break;
    case TRADE_STATUS_STATE_CHANGED:
      TargetAccept(1);
      break;
    case TRADE_STATUS_COMPLETE:
      UnlockTradeItems();
      SetTradePartner(0);
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(173));
      break;
    case TRADE_STATUS_UNACCEPTED:
      PlayerAccept(0);
      TargetAccept(0);
      break;
    case TRADE_STATUS_TOO_FAR_AWAY:
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(171));
      break;
    case TRADE_STATUS_WRONG_FACTION:
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(229));
      break;
    case TRADE_STATUS_FAILED:
      UnlockTradeItems();
      CGGameUI::DisplayError(GetGameError(bagResult, myFailure));
      SetTradePartner(0);
      break;
    default:
      break;
  }
}

void TradeItemStatsCallback(int id, const unsigned __int64 &guid, void *, bool granted) {
  if (granted) {
    FrameScript_SignalEvent(298);
  }
}

void CGTradeInfo::EnterWorld() {
  m_playerEnchantSlot = -1;
  m_targetEnchantSlot = -1;
  memset(m_targetItemEnchantment, 0, sizeof(m_targetItemEnchantment));
  memset(m_targetItemCount, 0, sizeof(m_targetItemCount));
  memset(m_targetItems, 0, sizeof(m_targetItems));
  memset(m_playerItemBag, 0, sizeof(m_playerItemBag));
  memset(m_playerItems, 0, sizeof(m_playerItems));
  memset(m_playerItemSlot, 0, sizeof(m_playerItemSlot));
  m_tradingPlayer = 0;
  m_playerAccepted = 0;
  m_targetAccepted = 0;
  m_playerMoney = 0;
  m_targetMoney = 0;
}

void CGTradeInfo::LeaveWorld() {
  Trade_C_CancelTrade();
}

void CGTradeInfo::PlayerAccept(int accept) {
  if (m_playerAccepted != accept) {
    m_playerAccepted = accept;
    FrameScript_SignalEvent(298);
  }
}

void CGTradeInfo::TargetAccept(int accept) {
  if (m_targetAccepted != accept) {
    m_targetAccepted = accept;
    FrameScript_SignalEvent(299);
  }
}

void CGTradeInfo::Update(TradeItemData *items) {
  PlayerAccept(0);
  TargetAccept(0);

  for (int index = 0; index < 8; ++index) {
    if (m_targetItems[index] != items[index].entryID || m_targetItemEnchantment[index] != items[index].enchantmentID ||
        m_targetItemCount[index] != items[index].count)
    {
      m_targetItems[index] = items[index].entryID;
      m_targetItemEnchantment[index] = items[index].enchantmentID;
      m_targetItemCount[index] = items[index].count;
      m_targetItemCreator[index] = items[index].creator;
      FrameScript_SignalEvent(300, "%d", index + 1);
    }
  }

  int proposedEnchantmentSpellID;
  int proposedEnchantmentSlot = -1;
  Trade_C_GetProposedEnchantment(0, proposedEnchantmentSpellID, proposedEnchantmentSlot);
  if (m_targetEnchantSlot != proposedEnchantmentSlot) {
    m_targetEnchantSlot = proposedEnchantmentSlot;
    FrameScript_SignalEvent(301, "%d", proposedEnchantmentSlot + 1);
  }

  Trade_C_GetProposedEnchantment(1, proposedEnchantmentSpellID, proposedEnchantmentSlot);
  if (m_playerEnchantSlot != proposedEnchantmentSlot) {
    m_playerEnchantSlot = proposedEnchantmentSlot;
    FrameScript_SignalEvent(300, "%d", proposedEnchantmentSlot + 1);
  }

  unsigned int playerMoney = Trade_C_GetPlayerTradeGold();
  if (m_playerMoney != playerMoney) {
    m_playerMoney = playerMoney;
    FrameScript_SignalEvent(303);
    FrameScript_SignalEvent(49, "%s", "player");
  }
  unsigned int targetMoney = Trade_C_GetTargetTradeGold();
  if (m_targetMoney != targetMoney) {
    m_targetMoney = targetMoney;
    FrameScript_SignalEvent(302);
  }
}

void CGTradeInfo::SetTradePartner(unsigned __int64 partner) {
  if (!partner) {
    if (m_tradingPlayer) {
      FrameScript_SignalEvent(297);
      CGGameUI::ClearInteractTarget(m_tradingPlayer);
      m_tradingPlayer = 0;
    }
    Trade_C_CancelTrade();
    return;
  }

  CGGameUI::SetInteractTarget(partner, MAX_TRADE_DISTANCE_SQUARED);
  m_tradingPlayer = partner;
  m_playerEnchantSlot = -1;
  m_targetEnchantSlot = -1;
  memset(m_targetItemEnchantment, 0, sizeof(m_targetItemEnchantment));
  memset(m_targetItemCount, 0, sizeof(m_targetItemCount));
  memset(m_targetItems, 0, sizeof(m_targetItems));
  memset(m_playerItemBag, 0, sizeof(m_playerItemBag));
  memset(m_playerItems, 0, sizeof(m_playerItems));
  memset(m_playerItemSlot, 0, sizeof(m_playerItemSlot));
  memset(m_targetItemCreator, 0, sizeof(m_targetItemCreator));
  m_playerAccepted = 0;
  m_targetAccepted = 0;
  m_playerMoney = 0;
  m_targetMoney = 0;

  if (Trade_C_UseCursorItem()) {
    unsigned __int64 item;
    unsigned __int64 container;
    unsigned int     slot;
    CGGameUI::GetCursorItem(item, container, slot);
    if (item && container) {
      Trade_C_AddItem(item, container, slot, 0);
      CGGameUI::ClearCursor(0);
      m_playerItems[0] = item;
      m_playerItemBag[0] = container;
      m_playerItemSlot[0] = slot;
    }
  }
  FrameScript_SignalEvent(296);
}

int CGTradeInfo::SetPlayerItem(int index, unsigned __int64 guid, unsigned __int64 bag, unsigned char slot) {
  if (index < 0 || index >= 8) {
    return 0;
  }
  if (guid ? !Trade_C_AddItem(guid, bag, slot, index) : (Trade_C_RemoveItem(index), false)) {
    return 0;
  }
  m_playerItems[index] = guid;
  m_playerItemBag[index] = bag;
  m_playerItemSlot[index] = slot;
  FrameScript_SignalEvent(301, "%d", index + 1);
  return 1;
}

void CGTradeInfo::RemovePlayerItem(unsigned __int64 guid) {
  if (!guid) {
    return;
  }

  for (int index = 0; index < 8; ++index) {
    if (m_playerItems[index] == guid) {
      SetPlayerItem(index, 0, 0, 0);
      return;
    }
  }
}

void CGTradeInfo::UpdatePlayerItem(unsigned __int64 guid) {
  if (!guid) {
    return;
  }

  for (int index = 0; index < 8; ++index) {
    if (m_playerItems[index] == guid) {
      SetPlayerItem(index, m_playerItems[index], m_playerItemBag[index], m_playerItemSlot[index]);
      return;
    }
  }
}

void CGTradeInfo::UnlockTradeItems() {
  for (int index = 0; index < 8; ++index) {
    if (m_playerItems[index]) {
      CGGameUI::UnlockItem(m_playerItems[index]);
    }
  }
}

GAME_ERROR_TYPE CGTradeInfo::GetGameError(BAG_RESULT bagResult, int myFailure) {
  if (bagResult == static_cast<BAG_RESULT>(4)) {
    return static_cast<GAME_ERROR_TYPE>(175 - (myFailure != 0));
  }
  if (bagResult == static_cast<BAG_RESULT>(16)) {
    return static_cast<GAME_ERROR_TYPE>(177 - (myFailure != 0));
  }
  return CGBag_C::GetGameError(bagResult);
}

static int Script_CloseTrade(lua_State *__formal) {
  Trade_C_CancelTrade();
  return 0;
}

static int Script_ClickTradeButton(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: ClickTradeButton(slot)");
  }
  if (CGGameUI::GetCursorMoney()) {
    Trade_C_AddMoney(CGGameUI::GetCursorMoney());
    CGGameUI::ClearCursor(1);
    return 0;
  }
  int              index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  unsigned __int64 cursorItem;
  unsigned __int64 cursorContainer;
  unsigned int     cursorSlot;
  CGGameUI::GetCursorItem(cursorItem, cursorContainer, cursorSlot);
  unsigned __int64 item;
  unsigned __int64 bag;
  unsigned char    slot;
  CGTradeInfo::GetPlayerItemInfo(index, item, bag, slot);
  if (cursorItem == item) {
    CGGameUI::ClearCursor(1);
  } else {
    if (cursorItem) {
      CGTradeInfo::SetPlayerItem(index, cursorItem, cursorContainer, cursorSlot);
    } else {
      CGTradeInfo::SetPlayerItem(index, 0, 0, 0);
    }
    CGGameUI::SetCursorItem(item, bag, slot, 0, 0);
  }
  return 0;
}

static int Script_ClickTargetTradeButton(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: ClickTargetTradeButton(slot)");
  }
  if (CGGameUI::GetCursorMoney()) {
    Trade_C_AddMoney(CGGameUI::GetCursorMoney());
    CGGameUI::ClearCursor(1);
  } else {
    Trade_C_RemoveItem(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  }
  return 0;
}

static int Script_GetTradeTargetItemInfo(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetTradeTargetItemInfo(index)");
  }
  int              index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  int              itemID = CGTradeInfo::GetTargetTradeItem(index);
  unsigned __int64 creator = CGTradeInfo::GetTargetTradeItemCreator(index);
  const ItemStats *stats = itemID ? g_itemDBCache.GetRecord(itemID, creator, TradeItemStatsCallback, 0) : 0;
  if (!itemID || !stats) {
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnumber(L, 0.0);
    lua_pushnil(L);
    lua_pushnumber(L, 0.0);
    lua_pushnil(L);
    return 6;
  }
  lua_pushstring(L, stats->m_displayName[CURRENT_LANGUAGE]);
  const char *path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
  const char *separator = path && *path ? "\\" : "";
  char        buffer[260];
  SStrPrintf(buffer, sizeof(buffer), "%s%s", path, separator);
  SStrCopy(buffer + strlen(buffer), CGItem_C::GetInventoryArt(stats->m_displayInfoID), sizeof(buffer) - strlen(buffer));
  lua_pushstring(L, buffer);
  lua_pushnumber(L, static_cast<double>(CGTradeInfo::GetTargetTradeItemCount(index)));
  if (CGTradeInfo::GetTargetEnchantSlot() == index) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  lua_pushnumber(L, static_cast<double>(CGTradeInfo::GetTargetTradeItemEnachantment(index)));
  lua_pushnil(L);
  return 6;
}

static int Script_GetTradeTargetItemLink(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetTradeTargetItemLink(index)");
  }
  int              itemID = CGTradeInfo::GetTargetTradeItem(static_cast<int>(lua_tonumber(L, 1)) - 1);
  const ItemStats *stats = itemID ? g_itemDBCache.GetRecord(itemID, 0, 0, 0) : 0;
  if (!stats) {
    return 0;
  }
  char link[1024];
  SStrPrintf(link, sizeof(link), "|Hitem:%d|h[%s]|h", itemID, stats->m_displayName[CURRENT_LANGUAGE]);
  lua_pushstring(L, link);
  return 1;
}

static int Script_GetTradePlayerItemInfo(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetTradePlayerItemInfo(index)");
  }
  int              index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  unsigned __int64 guid;
  unsigned __int64 bag;
  unsigned char    slot;
  CGTradeInfo::GetPlayerItemInfo(index, guid, bag, slot);
  CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (!item) {
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnumber(L, 0.0);
    lua_pushnil(L);
    lua_pushnumber(L, 0.0);
    return 5;
  }
  const ItemStats *stats = g_itemDBCache.GetRecord(item->GetEntryID(), ClntObjMgrGetActivePlayer(), TradeItemStatsCallback, 0);
  if (stats) {
    lua_pushstring(L, stats->m_displayName[CURRENT_LANGUAGE]);
  } else {
    lua_pushnil(L);
  }
  const char *path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
  const char *separator = path && *path ? "\\" : "";
  char        buffer[260];
  SStrPrintf(buffer, sizeof(buffer), "%s%s", path, separator);
  SStrCopy(buffer + strlen(buffer), item->GetInventoryArt(), sizeof(buffer) - strlen(buffer));
  lua_pushstring(L, buffer);
  lua_pushnumber(L, static_cast<double>(item->GetStackCount()));
  if (CGTradeInfo::GetPlayerEnchantSlot() == index) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  lua_pushnumber(L, 0.0);
  return 5;
}

static int Script_GetTradePlayerItemLink(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetTradePlayerItemLink(index)");
  }
  unsigned __int64 guid;
  unsigned __int64 bag;
  unsigned char    slot;
  CGTradeInfo::GetPlayerItemInfo(static_cast<int>(lua_tonumber(L, 1)) - 1, guid, bag, slot);
  CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (!item) {
    return 0;
  }
  const ItemStats *stats = g_itemDBCache.GetRecord(item->GetEntryID(), 0, 0, 0);
  if (!stats) {
    return 0;
  }
  char link[1024];
  SStrPrintf(link, sizeof(link), "|Hitem:%d|h[%s]|h", item->GetEntryID(), stats->m_displayName[CURRENT_LANGUAGE]);
  lua_pushstring(L, link);
  return 1;
}

static int Script_AcceptTrade(lua_State *__formal) {
  Trade_C_AcceptTrade();
  return 0;
}

static int Script_CancelTradeAccept(lua_State *__formal) {
  Trade_C_UnacceptTrade();
  return 0;
}

static int Script_GetPlayerTradeMoney(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGTradeInfo::GetTradePartner() ? Trade_C_GetPlayerTradeGold() : 0));
  return 1;
}

static int Script_GetTargetTradeMoney(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(Trade_C_GetTargetTradeGold()));
  return 1;
}

static int Script_PickupTradeMoney(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: PickupTradeMoney(amount)");
  }
  unsigned int amount = static_cast<unsigned int>(lua_tonumber(L, 1));
  if (amount && amount <= Trade_C_GetPlayerTradeGold()) {
    Trade_C_RemoveMoney(amount);
    CGGameUI::SetCursorMoney(amount);
  }
  return 0;
}

static int Script_AddTradeMoney(lua_State *__formal) {
  if (CGGameUI::GetCursorMoney()) {
    Trade_C_AddMoney(CGGameUI::GetCursorMoney());
    CGGameUI::ClearCursor(1);
  }
  return 0;
}

static FrameScript_Method s_ScriptFunctions[13] = {
    {            "CloseTrade",             Script_CloseTrade},
    {      "ClickTradeButton",       Script_ClickTradeButton},
    {"ClickTargetTradeButton", Script_ClickTargetTradeButton},
    {"GetTradeTargetItemInfo", Script_GetTradeTargetItemInfo},
    {"GetTradeTargetItemLink", Script_GetTradeTargetItemLink},
    {"GetTradePlayerItemInfo", Script_GetTradePlayerItemInfo},
    {"GetTradePlayerItemLink", Script_GetTradePlayerItemLink},
    {           "AcceptTrade",            Script_AcceptTrade},
    {     "CancelTradeAccept",      Script_CancelTradeAccept},
    {   "GetPlayerTradeMoney",    Script_GetPlayerTradeMoney},
    {   "GetTargetTradeMoney",    Script_GetTargetTradeMoney},
    {      "PickupTradeMoney",       Script_PickupTradeMoney},
    {         "AddTradeMoney",          Script_AddTradeMoney}
};

void TradeInfoRegisterScriptFunctions() {
  for (unsigned int i = 0; i < 13; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void TradeInfoUnregisterScriptFunctions() {
  for (unsigned int i = 0; i < 13; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
