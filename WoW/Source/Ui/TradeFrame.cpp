#include <Base/Base.h>
#include <Frame/CSimpleTop.h>
#include <Frame/CSimpleModel.h>
#include <WowConst.h>
#include <MapDefs.h>
#include <WorldClient/World.h>
#include "Net/NetClient/NetClient.h"
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "SoundInterface/SoundInterface.h"
#include "UIUtil/InputControl.h"
#include "WorldFrame.h"
#include "GameUI.h"

#include <string.h>

#include "GameUI.h"
#include "TradeFrame.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "DB/DBClient/DBClient.h"
#include "DB/WowLocale.h"
#include "Object/ItemStats.h"
#include "Object/ObjectClient/Bag_C.h"
#include "Object/ObjectClient/Item_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "WowSvcs/WowSvcsClient/FriendList.h"

#include <FrameScript/FrameScript.h>
#include <lauxlib.h>
#include <lua.h>
#include <storm.h>

void      Trade_C_CancelTrade();
BOOL      Trade_C_UseCursorItem();
BOOL      Trade_C_GetProposedEnchantment(UINT player, int &spellID, int &slot);
UINT      Trade_C_GetPlayerTradeGold();
UINT      Trade_C_GetTargetTradeGold();
DWORDLONG Trade_C_GetTradeTarget();
int       Trade_C_IsInitiator();
void      Trade_C_BeginTrade();
void      Trade_C_PlayerBusy();
void      Trade_C_PlayerIgnored();
bool      Trade_C_AddItem(DWORDLONG item, DWORDLONG itemContainer, UINT itemSlot, UINT tradeSlot);
void      Trade_C_RemoveItem(UINT slot);
void      Trade_C_AcceptTrade();
void      Trade_C_UnacceptTrade();
void      Trade_C_AddMoney(UINT money);
void      Trade_C_RemoveMoney(UINT money);
int       Spell_C_TargetTradeItem(int tradeIndex);
static void      TradeItemStatsCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted);

DWORDLONG CGTradeInfo::m_tradingPlayer;
int       CGTradeInfo::m_playerAccepted;
int       CGTradeInfo::m_targetAccepted;
DWORDLONG CGTradeInfo::m_playerItems[8];
DWORDLONG CGTradeInfo::m_playerItemBag[8];
BYTE      CGTradeInfo::m_playerItemSlot[8];
int       CGTradeInfo::m_targetItems[8];
int       CGTradeInfo::m_targetItemCount[8];
int       CGTradeInfo::m_targetItemEnchantment[8];
DWORDLONG CGTradeInfo::m_targetItemCreator[8];
int       CGTradeInfo::m_playerEnchantSlot = -1;
int       CGTradeInfo::m_targetEnchantSlot = -1;
UINT      CGTradeInfo::m_playerMoney;
UINT      CGTradeInfo::m_targetMoney;

static void TradeItemStatsCallback(int id, const DWORDLONG &guid, LPVOID, bool granted) {
  if (granted) {
    FrameScript_SignalEvent(298);
  }
}

void CGTradeInfo::EnterWorld() {
  m_tradingPlayer = 0;
  m_playerEnchantSlot = -1;
  m_targetEnchantSlot = -1;
  m_playerAccepted = 0;
  m_targetAccepted = 0;
  m_playerMoney = 0;
  m_targetMoney = 0;
  for (int index = 0; index < 8; ++index) {
    m_playerItems[index] = 0;
    m_playerItemBag[index] = 0;
    m_playerItemSlot[index] = 0;
    m_targetItems[index] = 0;
    m_targetItemCount[index] = 0;
    m_targetItemEnchantment[index] = 0;
  }
}

void CGTradeInfo::LeaveWorld() {
  Trade_C_CancelTrade();
}

void CGTradeInfo::PlayerAccept(int accept) {
  if (m_playerAccepted != accept) {
    m_playerAccepted = accept;
    FrameScript_SignalEvent(299, "%d%d", accept, m_targetAccepted);
    if (accept) {
      Trade_C_AcceptTrade();
    } else {
      Trade_C_UnacceptTrade();
    }
  }
}

void CGTradeInfo::TargetAccept(int accept) {
  if (m_targetAccepted != accept) {
    m_targetAccepted = accept;
    FrameScript_SignalEvent(299, "%d%d", m_playerAccepted, accept);
  }
}

void CGTradeInfo::Update(TradeItemData items[]) {
  PlayerAccept(0);
  TargetAccept(0);

  for (int index = 0; index < 8; ++index) {
    if (m_targetItems[index] != items[index].entryID || m_targetItemCount[index] != items[index].count ||
        m_targetItemEnchantment[index] != items[index].enchantmentID)
    {
      m_targetItems[index] = items[index].entryID;
      m_targetItemCount[index] = items[index].count;
      m_targetItemEnchantment[index] = items[index].enchantmentID;
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

  if (m_playerMoney != Trade_C_GetPlayerTradeGold()) {
    m_playerMoney = Trade_C_GetPlayerTradeGold();
    FrameScript_SignalEvent(303);
    FrameScript_SignalEvent(49, "%s", "player");
  }
  if (m_targetMoney != Trade_C_GetTargetTradeGold()) {
    m_targetMoney = Trade_C_GetTargetTradeGold();
    FrameScript_SignalEvent(302);
  }
}

void CGTradeInfo::SetTradePartner(DWORDLONG partner) {
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
  m_playerAccepted = 0;
  m_targetAccepted = 0;
  m_playerMoney = 0;
  m_targetMoney = 0;
  for (int index = 0; index < 8; ++index) {
    m_playerItems[index] = 0;
    m_playerItemBag[index] = 0;
    m_playerItemSlot[index] = 0;
    m_targetItems[index] = 0;
    m_targetItemCount[index] = 0;
    m_targetItemEnchantment[index] = 0;
  }

  if (Trade_C_UseCursorItem()) {
    DWORDLONG item;
    DWORDLONG container;
    UINT      slot;
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

void CGTradeInfo::HandleTradeMessage(TRADE_STATUS status, BAG_RESULT bagResult, int myFailure, int itemID) {
  switch (status) {
    case TRADE_STATUS_PLAYER_BUSY:
    case TRADE_STATUS_ALREADY_TRADING:
      if (Trade_C_IsInitiator()) {
        CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(Trade_C_GetTradeTarget(), __FILE__, __LINE__));
        if (unit) {
          CGGameUI::DisplayError(GERR_PLAYER_BUSY_S, unit->GetUnitName());
        }
      }
      break;
    case TRADE_STATUS_PLAYER_IGNORED:
      if (Trade_C_IsInitiator()) {
        CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(Trade_C_GetTradeTarget(), __FILE__, __LINE__));
        if (unit) {
          CGGameUI::DisplayError(GERR_IGNORING_YOU_S, unit->GetUnitName());
        }
      }
      break;
    case TRADE_STATUS_CANCELLED:
      FrameScript_SignalEvent(267);
      UnlockTradeItems();
      SetTradePartner(0);
      CGGameUI::DisplayError(GERR_TRADE_CANCELLED);
      break;
    case TRADE_STATUS_PLAYER_NOT_FOUND:
      CGGameUI::DisplayError(GERR_GENERIC_NO_TARGET);
      break;
    case TRADE_STATUS_TOO_FAR_AWAY:
      CGGameUI::DisplayError(GERR_TRADE_TOO_FAR);
      break;
    case TRADE_STATUS_PROPOSED:
      if (!Trade_C_IsInitiator()) {
        if (g_friendList->IsIgnored(Trade_C_GetTradeTarget())) {
          Trade_C_PlayerIgnored();
        } else {
          CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(Trade_C_GetTradeTarget(), __FILE__, __LINE__));
          if (unit) {
            if (unit->GetHealth() <= 0) {
              Trade_C_PlayerBusy();
            } else {
              Trade_C_BeginTrade();
            }
          }
        }
      }
      break;
    case TRADE_STATUS_INITIATED:
      SetTradePartner(Trade_C_GetTradeTarget());
      break;
    case TRADE_STATUS_ACCEPTED:
      TargetAccept(1);
      break;
    case TRADE_STATUS_UNACCEPTED:
      TargetAccept(0);
      break;
    case TRADE_STATUS_COMPLETE:
      UnlockTradeItems();
      SetTradePartner(0);
      CGGameUI::DisplayError(GERR_TRADE_COMPLETE);
      break;
    case TRADE_STATUS_FAILED:
      UnlockTradeItems();
      CGGameUI::DisplayError(GetGameError(bagResult, myFailure));
      SetTradePartner(0);
      break;
    case TRADE_STATUS_STATE_CHANGED:
      PlayerAccept(0);
      TargetAccept(0);
      break;
    case TRADE_STATUS_WRONG_FACTION:
      CGGameUI::DisplayError(GERR_PLAYER_WRONG_FACTION);
      break;
    default:
      break;
  }
}

BOOL CGTradeInfo::SetPlayerItem(int index, DWORDLONG guid, DWORDLONG bag, BYTE slot) {
  if (index >= 0 && index < 8) {
    if (!guid) {
      Trade_C_RemoveItem(index);
    } else if (!Trade_C_AddItem(guid, bag, slot, index)) {
      return 0;
    }
    m_playerItems[index] = guid;
    m_playerItemBag[index] = bag;
    m_playerItemSlot[index] = slot;
    FrameScript_SignalEvent(301, "%d", index + 1);
    return 1;
  }
  return 0;
}

void CGTradeInfo::RemovePlayerItem(DWORDLONG guid) {
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

void CGTradeInfo::UpdatePlayerItem(DWORDLONG guid) {
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
  GAME_ERROR_TYPE error;
  switch (bagResult) {
    case BAG_FULL:
      error = myFailure ? GERR_TRADE_BAG_FULL : GERR_TRADE_TARGET_BAG_FULL;
      break;
    case BAG_ITEM_MAX_COUNT_EXCEEDED:
      error = myFailure ? GERR_TRADE_MAX_COUNT_EXCEEDED : GERR_TRADE_TARGET_MAX_COUNT_EXCEEDED;
      break;
    default:
      error = CGBag_C::GetGameError(bagResult);
      break;
  }
  return error;
}

static int Script_CloseTrade(lua_State *) {
  CGTradeInfo::SetTradePartner(0);
  FrameScript_SignalEvent(303);
  return 0;
}

static int Script_ClickTradeButton(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: ClickTradeButton(index)");
    return 0;
  }
  if (CGGameUI::GetCursorMoney()) {
    Trade_C_AddMoney(CGGameUI::GetCursorMoney());
    CGGameUI::ClearCursor(1);
    return 0;
  }
  int       index = (int)lua_tonumber(L, 1) - 1;
  DWORDLONG cursorItem;
  DWORDLONG cursorContainer;
  UINT      cursorSlot;
  CGGameUI::GetCursorItem(cursorItem, cursorContainer, cursorSlot);
  CGItem_C *cursor = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(cursorItem, __FILE__, __LINE__));
  if (cursor) {
    if (cursor->IsBound()) {
      const ItemStats *stats = g_itemDBCache.GetRecord(cursor->GetEntryID(), 0, 0, 0);
      if (stats && (stats->m_bonding == 4 || stats->m_bonding == 5)) {
        CGGameUI::DisplayError(GERR_TRADE_QUEST_ITEM);
      } else {
        CGGameUI::DisplayError(GERR_TRADE_BOUND_ITEM);
      }
      return 0;
    }
    if (cursor->IsA(TYPE_CONTAINER)) {
      CGBag_C *cursorBag = cursor->GetBag();
      if (cursorBag && cursorBag->NumItems()) {
        CGGameUI::DisplayError(GERR_TRADE_BAG);
        return 0;
      }
    }
  }
  DWORDLONG item;
  DWORDLONG bag;
  BYTE      slot;
  CGTradeInfo::GetPlayerItemInfo(index, item, bag, slot);
  if (cursorItem == item) {
    CGGameUI::ClearCursor(1);
  } else {
    BOOL placed = CGTradeInfo::SetPlayerItem(index, cursorItem, cursorContainer, cursorSlot);
    CGGameUI::SetCursorItem(item, bag, slot, !placed, 0);
  }
  return 0;
}

static int Script_ClickTargetTradeButton(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: ClickTargetTradeButton(index)");
    return 0;
  }
  if (CGGameUI::GetCursorMoney()) {
    Trade_C_AddMoney(CGGameUI::GetCursorMoney());
    CGGameUI::ClearCursor(1);
  } else {
    int index = (int)lua_tonumber(L, 1) - 1;
    Spell_C_TargetTradeItem(index);
  }
  return 0;
}

static int Script_GetTradeTargetItemInfo(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetTradeTargetItemInfo(index)");
    return 0;
  }
  int index = (int)lua_tonumber(L, 1) - 1;
  int itemID = CGTradeInfo::GetTargetTradeItem(index);
  if (itemID) {
    const ItemStats *stats = g_itemDBCache.GetRecord(itemID, CGTradeInfo::GetTradePartner(), TradeItemStatsCallback, 0);
    if (stats) {
      lua_pushstring(L, stats->m_displayName[0]);
      LPCSTR path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
      LPCSTR separator = *path ? "\\" : "";
      char   buffer[MAX_PATH];
      SStrPrintf(buffer, sizeof(buffer), "%s%s", path, separator);
      SStrPack(buffer, CGItem_C::GetInventoryArt(stats->m_displayInfoID), sizeof(buffer));
      lua_pushstring(L, buffer);
      lua_pushnumber(L, CGTradeInfo::GetTargetTradeItemCount(index));
      if (CGTradeInfo::GetTargetEnchantSlot() == index) {
        lua_pushnumber(L, 1.0);
      } else {
        lua_pushnil(L);
      }
      lua_pushnumber(L, stats->m_inventoryType ? stats->m_overallQualityID : -1);
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      GAME_ERROR_TYPE reason;
      if (!player || player->CanUseItem(stats, reason)) {
        lua_pushnumber(L, 1.0);
      } else {
        lua_pushnil(L);
      }
      return 6;
    }
  }
  lua_pushnil(L);
  lua_pushnil(L);
  lua_pushnumber(L, 0.0);
  lua_pushnil(L);
  lua_pushnumber(L, 0.0);
  lua_pushnil(L);
  return 6;
}

static int Script_GetTradeTargetItemLink(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetTradeTargetItemLink(index)");
    return 0;
  }
  int              itemID = CGTradeInfo::GetTargetTradeItem((int)lua_tonumber(L, 1) - 1);
  if (itemID) {
    const ItemStats *stats = g_itemDBCache.GetRecord(itemID, 0, 0, 0);
    if (stats) {
      char link[1024];
      SStrPrintf(link, sizeof(link), "|Hitem:%d|h[%s]|h", itemID, stats->m_displayName[0]);
      lua_pushstring(L, link);
      return 1;
    }
  }
  return 0;
}

static int Script_GetTradePlayerItemInfo(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetTradePlayerItemInfo(index)");
    return 0;
  }
  int       index = (int)lua_tonumber(L, 1) - 1;
  CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(CGTradeInfo::GetPlayerTradeSlot(index), __FILE__, __LINE__));
  if (item) {
    const ItemStats *stats = g_itemDBCache.GetRecord(item->GetEntryID(), ClntObjMgrGetActivePlayer(), TradeItemStatsCallback, 0);
    if (stats) {
      lua_pushstring(L, stats->m_displayName[0]);
    } else {
      lua_pushnil(L);
    }
    LPCSTR path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
    LPCSTR separator = *path ? "\\" : "";
    char   buffer[MAX_PATH];
    SStrPrintf(buffer, sizeof(buffer), "%s%s", path, separator);
    SStrPack(buffer, item->GetInventoryArt(), sizeof(buffer));
    lua_pushstring(L, buffer);
    lua_pushnumber(L, item->GetStackCount());
    if (CGTradeInfo::GetPlayerEnchantSlot() == index) {
      lua_pushnumber(L, 1.0);
    } else {
      lua_pushnil(L);
    }
    lua_pushnumber(L, stats && stats->m_inventoryType ? stats->m_overallQualityID : -1);
  } else {
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnumber(L, 0.0);
    lua_pushnil(L);
    lua_pushnumber(L, 0.0);
  }
  return 5;
}

static int Script_GetTradePlayerItemLink(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetTradePlayerItemLink(index)");
    return 0;
  }
  int       index = (int)lua_tonumber(L, 1) - 1;
  CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(CGTradeInfo::GetPlayerTradeSlot(index), __FILE__, __LINE__));
  if (item) {
    const ItemStats *stats = g_itemDBCache.GetRecord(item->GetEntryID(), 0, 0, 0);
    if (stats) {
      char link[1024];
      SStrPrintf(link, sizeof(link), "|Hitem:%d|h[%s]|h", item->GetEntryID(), stats->m_displayName[0]);
      lua_pushstring(L, link);
      return 1;
    }
  }
  return 0;
}

static int Script_AcceptTrade(lua_State *) {
  CGTradeInfo::PlayerAccept(1);
  return 0;
}

static int Script_CancelTradeAccept(lua_State *) {
  CGTradeInfo::PlayerAccept(0);
  return 0;
}

static int Script_GetPlayerTradeMoney(lua_State *L) {
  lua_pushnumber(L, CGTradeInfo::GetTradePartner() ? Trade_C_GetPlayerTradeGold() : 0);
  return 1;
}

static int Script_GetTargetTradeMoney(lua_State *L) {
  lua_pushnumber(L, Trade_C_GetTargetTradeGold());
  return 1;
}

static int Script_PickupTradeMoney(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: PickupTradeMoney(amount)");
    return 0;
  }
  int amount = lua_tonumber(L, 1);
  if (amount > 0 && amount <= (int)Trade_C_GetPlayerTradeGold()) {
    Trade_C_RemoveMoney(amount);
    CGGameUI::SetCursorMoney(amount);
  }
  return 0;
}

static int Script_AddTradeMoney(lua_State *) {
  if (CGGameUI::GetCursorMoney()) {
    Trade_C_AddMoney(CGGameUI::GetCursorMoney());
    CGGameUI::SetCursorMoney(0);
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
  for (UINT i = 0; i < 13; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void TradeInfoUnregisterScriptFunctions() {
  for (UINT i = 0; i < 13; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
