#include <string.h>

#include "GameUI.h"
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

void __fastcall         Trade_C_CancelTrade();
int __fastcall          Trade_C_UseCursorItem();
int __fastcall          Trade_C_GetProposedEnchantment(unsigned int player, int &spellID, int &slot);
unsigned int __fastcall Trade_C_GetPlayerTradeGold();
unsigned int __fastcall Trade_C_GetTargetTradeGold();
unsigned __int64 __fastcall Trade_C_GetTradeTarget();
bool __fastcall         Trade_C_AddItem(unsigned __int64 item, unsigned __int64 itemContainer, unsigned int itemSlot, unsigned int tradeSlot);
void __fastcall         Trade_C_RemoveItem(unsigned int slot);
void __fastcall         Trade_C_AcceptTrade();
void __fastcall         Trade_C_UnacceptTrade();
void __fastcall         Trade_C_AddMoney(unsigned int money);
void __fastcall         Trade_C_RemoveMoney(unsigned int money);
void __fastcall         TradeItemStatsCallback(int id, const unsigned __int64 &guid, void *arg, bool granted);

static const float MAX_TRADE_DISTANCE = 11.111111f;
static const float MAX_TRADE_DISTANCE_SQUARED = MAX_TRADE_DISTANCE * MAX_TRADE_DISTANCE;

struct TradeItemData {
  int              itemID;
  int              displayID;
  int              enchantment;
  int              count;
  unsigned __int64 creator;
};

enum TRADE_STATUS {
  TRADE_STATUS_PLAYER_BUSY = 0,
  TRADE_STATUS_PROPOSED = 1,
  TRADE_STATUS_INITIATED = 2,
  TRADE_STATUS_CANCELLED = 3,
  TRADE_STATUS_ACCEPTED = 4,
  TRADE_STATUS_ALREADY_TRADING = 5,
  TRADE_STATUS_PLAYER_NOT_FOUND = 6,
  TRADE_STATUS_STATE_CHANGED = 7,
  TRADE_STATUS_COMPLETE = 8,
  TRADE_STATUS_UNACCEPTED = 9,
  TRADE_STATUS_TOO_FAR_AWAY = 10,
  TRADE_STATUS_WRONG_FACTION = 11,
  TRADE_STATUS_FAILED = 12,
  TRADE_STATUS_DEAD = 13,
  TRADE_STATUS_PETITION = 14,
  TRADE_STATUS_PLAYER_IGNORED = 15
};

class CGTradeInfo {
 public:
  static void __fastcall             EnterWorld();
  static void __fastcall             LeaveWorld();
  static unsigned __int64 __fastcall GetTradePartner();
  static void __fastcall             SetTradePartner(unsigned __int64 partner);
  static void __fastcall             Update(TradeItemData *items);
  static void __fastcall             PlayerAccept(int accept);
  static void __fastcall             TargetAccept(int accept);
  static void __fastcall             HandleTradeMessage(TRADE_STATUS status, BAG_RESULT bagResult, int myFailure, int itemID);
  static int __fastcall              SetPlayerItem(int index, unsigned __int64 guid, unsigned __int64 bag, unsigned int slot);
  static void __fastcall             GetPlayerItemInfo(int index, unsigned __int64 &guid, unsigned __int64 &bag, unsigned int &slot);
  static int __fastcall              GetTargetTradeItem(int index);
  static int __fastcall GetTargetTradeItemCount(int index) {
    return index >= 0 && index < 8 ? m_targetItemCount[index] : 0;
  }
  static int __fastcall GetTargetTradeItemEnachantment(int index) {
    return index >= 0 && index < 8 ? m_targetItemEnchantment[index] : 0;
  }
  static unsigned __int64 __fastcall GetTargetTradeItemCreator(int index) {
    return index >= 0 && index < 8 ? m_targetItemCreator[index] : 0;
  }
  static int __fastcall GetPlayerEnchantSlot() {
    return m_playerEnchantSlot;
  }
  static int __fastcall GetTargetEnchantSlot() {
    return m_targetEnchantSlot;
  }

 protected:
  static unsigned __int64 m_tradingPlayer;
  static int              m_playerAccepted;
  static int              m_targetAccepted;
  static unsigned __int64 m_playerItems[8];
  static unsigned __int64 m_playerItemBag[8];
  static unsigned int     m_playerItemSlot[8];
  static int              m_targetItems[8];
  static int              m_targetItemCount[8];
  static int              m_targetItemEnchantment[8];
  static unsigned __int64 m_targetItemCreator[8];
  static int              m_playerEnchantSlot;
  static int              m_targetEnchantSlot;
  static unsigned int     m_playerMoney;
  static unsigned int     m_targetMoney;
};

unsigned __int64 CGTradeInfo::m_tradingPlayer;
int              CGTradeInfo::m_playerAccepted;
int              CGTradeInfo::m_targetAccepted;
unsigned __int64 CGTradeInfo::m_playerItems[8];
unsigned __int64 CGTradeInfo::m_playerItemBag[8];
unsigned int     CGTradeInfo::m_playerItemSlot[8];
int              CGTradeInfo::m_targetItems[8];
int              CGTradeInfo::m_targetItemCount[8];
int              CGTradeInfo::m_targetItemEnchantment[8];
unsigned __int64 CGTradeInfo::m_targetItemCreator[8];
int              CGTradeInfo::m_playerEnchantSlot = -1;
int              CGTradeInfo::m_targetEnchantSlot = -1;
unsigned int     CGTradeInfo::m_playerMoney;
unsigned int     CGTradeInfo::m_targetMoney;

int __fastcall CGTradeInfo::GetTargetTradeItem(int index) {
  return index >= 0 && index < 8 ? m_targetItems[index] : 0;
}

void __fastcall CGTradeInfo::HandleTradeMessage(TRADE_STATUS status, BAG_RESULT bagResult, int myFailure, int itemID) {
  switch (status) {
    case TRADE_STATUS_INITIATED:
      SetTradePartner(Trade_C_GetTradeTarget());
      break;
    case TRADE_STATUS_CANCELLED:
      FrameScript_SignalEvent(267);
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
    default:
      break;
  }
}

void __fastcall TradeItemStatsCallback(int, const unsigned __int64 &, void *, bool granted) {
  if (granted) {
    FrameScript_SignalEvent(298);
  }
}

void __fastcall CGTradeInfo::EnterWorld() {
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

void __fastcall CGTradeInfo::LeaveWorld() {
  Trade_C_CancelTrade();
}

void __fastcall CGTradeInfo::PlayerAccept(int accept) {
  if (m_playerAccepted != accept) {
    m_playerAccepted = accept;
    FrameScript_SignalEvent(298);
  }
}

void __fastcall CGTradeInfo::TargetAccept(int accept) {
  if (m_targetAccepted != accept) {
    m_targetAccepted = accept;
    FrameScript_SignalEvent(299);
  }
}

void __fastcall CGTradeInfo::Update(TradeItemData *items) {
  PlayerAccept(0);
  TargetAccept(0);

  for (int index = 0; index < 8; ++index) {
    if (m_targetItems[index] != items[index].itemID || m_targetItemEnchantment[index] != items[index].enchantment ||
        m_targetItemCount[index] != items[index].count)
    {
      m_targetItems[index] = items[index].itemID;
      m_targetItemEnchantment[index] = items[index].enchantment;
      m_targetItemCount[index] = items[index].count;
      m_targetItemCreator[index] = items[index].creator;
      FrameScript_SignalEvent(300, "%d", index + 1);
    }
  }

  int spellID;
  int slot = -1;
  Trade_C_GetProposedEnchantment(0, spellID, slot);
  if (m_targetEnchantSlot != slot) {
    m_targetEnchantSlot = slot;
    FrameScript_SignalEvent(301, "%d", slot + 1);
  }

  Trade_C_GetProposedEnchantment(1, spellID, slot);
  if (m_playerEnchantSlot != slot) {
    m_playerEnchantSlot = slot;
    FrameScript_SignalEvent(300, "%d", slot + 1);
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

unsigned __int64 __fastcall CGTradeInfo::GetTradePartner() {
  return m_tradingPlayer;
}

void __fastcall CGTradeInfo::SetTradePartner(unsigned __int64 partner) {
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
    unsigned __int64 bag;
    unsigned int     slot;
    CGGameUI::GetCursorItem(item, bag, slot);
    if (item && bag) {
      Trade_C_AddItem(item, bag, slot, 0);
      CGGameUI::ClearCursor(0);
      m_playerItems[0] = item;
      m_playerItemBag[0] = bag;
      m_playerItemSlot[0] = slot;
    }
  }
  FrameScript_SignalEvent(296);
}

int __fastcall CGTradeInfo::SetPlayerItem(int index, unsigned __int64 guid, unsigned __int64 bag, unsigned int slot) {
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

void __fastcall CGTradeInfo::GetPlayerItemInfo(int index, unsigned __int64 &guid, unsigned __int64 &bag, unsigned int &slot) {
  if (index >= 0 && index < 8) {
    guid = m_playerItems[index];
    bag = m_playerItemBag[index];
    slot = m_playerItemSlot[index];
  } else {
    guid = 0;
    bag = 0;
    slot = 0;
  }
}

static int __fastcall Script_CloseTrade(lua_State *__formal) {
  Trade_C_CancelTrade();
  return 0;
}

static int __fastcall Script_ClickTradeButton(lua_State *L) {
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
  unsigned __int64 cursorBag;
  unsigned int     cursorSlot;
  CGGameUI::GetCursorItem(cursorItem, cursorBag, cursorSlot);
  unsigned __int64 item;
  unsigned __int64 bag;
  unsigned int     slot;
  CGTradeInfo::GetPlayerItemInfo(index, item, bag, slot);
  if (cursorItem == item) {
    CGGameUI::ClearCursor(1);
  } else {
    if (cursorItem) {
      CGTradeInfo::SetPlayerItem(index, cursorItem, cursorBag, cursorSlot);
    } else {
      CGTradeInfo::SetPlayerItem(index, 0, 0, 0);
    }
    CGGameUI::SetCursorItem(item, bag, slot, 0, 0);
  }
  return 0;
}

static int __fastcall Script_ClickTargetTradeButton(lua_State *L) {
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

static int __fastcall Script_GetTradeTargetItemInfo(lua_State *L) {
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
  CGTradeInfo::GetTargetEnchantSlot() == index ? lua_pushnumber(L, 1.0) : lua_pushnil(L);
  lua_pushnumber(L, static_cast<double>(CGTradeInfo::GetTargetTradeItemEnachantment(index)));
  lua_pushnil(L);
  return 6;
}

static int __fastcall Script_GetTradeTargetItemLink(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetTradeTargetItemLink(index)");
  }
  int              itemID = CGTradeInfo::GetTargetTradeItem(static_cast<int>(lua_tonumber(L, 1)) - 1);
  unsigned __int64 noGuid = 0;
  const ItemStats *stats = itemID ? g_itemDBCache.GetRecord(itemID, noGuid, 0, 0) : 0;
  if (!stats) {
    return 0;
  }
  char link[1024];
  SStrPrintf(link, sizeof(link), "|Hitem:%d|h[%s]|h", itemID, stats->m_displayName[CURRENT_LANGUAGE]);
  lua_pushstring(L, link);
  return 1;
}

static int __fastcall Script_GetTradePlayerItemInfo(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetTradePlayerItemInfo(index)");
  }
  int              index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  unsigned __int64 guid;
  unsigned __int64 bag;
  unsigned int     slot;
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
  unsigned __int64 player = ClntObjMgrGetActivePlayer();
  const ItemStats *stats = g_itemDBCache.GetRecord(item->GetEntryID(), player, TradeItemStatsCallback, 0);
  stats ? lua_pushstring(L, stats->m_displayName[CURRENT_LANGUAGE]) : lua_pushnil(L);
  const char *path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
  const char *separator = path && *path ? "\\" : "";
  char        buffer[260];
  SStrPrintf(buffer, sizeof(buffer), "%s%s", path, separator);
  SStrCopy(buffer + strlen(buffer), item->GetInventoryArt(), sizeof(buffer) - strlen(buffer));
  lua_pushstring(L, buffer);
  lua_pushnumber(L, static_cast<double>(item->GetStackCount()));
  CGTradeInfo::GetPlayerEnchantSlot() == index ? lua_pushnumber(L, 1.0) : lua_pushnil(L);
  lua_pushnumber(L, 0.0);
  return 5;
}

static int __fastcall Script_GetTradePlayerItemLink(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetTradePlayerItemLink(index)");
  }
  unsigned __int64 guid;
  unsigned __int64 bag;
  unsigned int     slot;
  CGTradeInfo::GetPlayerItemInfo(static_cast<int>(lua_tonumber(L, 1)) - 1, guid, bag, slot);
  CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (!item) {
    return 0;
  }
  const ItemStats *stats = g_itemDBCache.GetRecord(item->GetEntryID(), guid, 0, 0);
  if (!stats) {
    return 0;
  }
  char link[1024];
  SStrPrintf(link, sizeof(link), "|Hitem:%d|h[%s]|h", item->GetEntryID(), stats->m_displayName[CURRENT_LANGUAGE]);
  lua_pushstring(L, link);
  return 1;
}

static int __fastcall Script_AcceptTrade(lua_State *__formal) {
  Trade_C_AcceptTrade();
  return 0;
}

static int __fastcall Script_CancelTradeAccept(lua_State *__formal) {
  Trade_C_UnacceptTrade();
  return 0;
}

static int __fastcall Script_GetPlayerTradeMoney(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGTradeInfo::GetTradePartner() ? Trade_C_GetPlayerTradeGold() : 0));
  return 1;
}

static int __fastcall Script_GetTargetTradeMoney(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(Trade_C_GetTargetTradeGold()));
  return 1;
}

static int __fastcall Script_PickupTradeMoney(lua_State *L) {
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

static int __fastcall Script_AddTradeMoney(lua_State *__formal) {
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

void __fastcall TradeInfoRegisterScriptFunctions() {
  for (unsigned int i = 0; i < 13; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void __fastcall TradeInfoUnregisterScriptFunctions() {
  for (unsigned int i = 0; i < 13; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
