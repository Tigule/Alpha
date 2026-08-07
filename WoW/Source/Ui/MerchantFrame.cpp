#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

#include "GameUI.h"
#include "MerchantFrame.h"

#include "DB/DBClient/DBCacheInstances.h"
#include "DB/DBClient/DBClient.h"
#include "DB/WowLocale.h"
#include "Object/ItemStats.h"
#include "Object/ObjectClient/Item_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "UIUtil/Cursor.h"

#include <FrameScript/FrameScript.h>

#include <lauxlib.h>
#include <lua.h>
#include <storm.h>
#include <string.h>

void CursorModelSetSequence(CURSORANIMATIONS sequence);

DWORDLONG  CGMerchantInfo::m_merchant;
VendorItem CGMerchantInfo::m_items[128];
int        CGMerchantInfo::m_itemCount;
UINT       CGMerchantInfo::m_callbackCount;

void MerchantItemStatsCallback(int id, const DWORDLONG &guid, LPVOID, bool) {
  CGMerchantInfo::DecrementCallbackCount();
}

void CGMerchantInfo::EnterWorld() {
  memset(m_items, 0, sizeof(m_items));
  m_merchant = 0;
  m_itemCount = 0;
}

void CGMerchantInfo::LeaveWorld() {
  CloseMerchant();
}

void CGMerchantInfo::SetMerchant(DWORDLONG merchantGUID, VendorItem *items, int count) {
  if (m_itemCount) {
    for (int index = 0; index < 128; ++index) {
      if (m_items[index].m_itemType) {
        g_itemDBCache.CancelCallback(m_items[index].m_itemType, MerchantItemStatsCallback, 0);
      }
    }
  }

  CGGameUI::SetInteractTarget(merchantGUID, MAX_SHOP_DISTANCE_SQUARED);
  m_merchant = merchantGUID;
  memset(m_items, 0, sizeof(m_items));
  memcpy(m_items, items, sizeof(VendorItem) * count);
  m_itemCount = count;
  m_callbackCount = 0;
  FrameScript_SignalEvent(293);
}

void CGMerchantInfo::CloseMerchant() {
  if (m_merchant) {
    FrameScript_SignalEvent(295);
    CGGameUI::ClearInteractTarget(m_merchant);
    m_merchant = 0;
    m_itemCount = 0;
    if (CGGameUI::GetCursorVirtualItem()) {
      CGGameUI::ClearCursor(1);
    }
  }
}

void CGMerchantInfo::UpdateItemQuantity(DWORDLONG vendor, DWORD muid, int newQuantity) {
  if (vendor == m_merchant) {
    int index = 0;
    while (m_items[index].m_muid != muid) {
      if (++index >= 128) {
        FrameScript_SignalEvent(294);
        return;
      }
    }

    m_items[index].m_quantity = newQuantity;
    FrameScript_SignalEvent(294);
  }
}

const ItemStats *CGMerchantInfo::GetItemStats(UINT itemID) {
  if (!itemID) {
    return 0;
  }
  const ItemStats_C *stats = g_itemDBCache.GetRecord(itemID, GetMerchant(), MerchantItemStatsCallback, 0);
  if (!stats) {
    ++m_callbackCount;
  }
  return stats;
}

void CGMerchantInfo::DecrementCallbackCount() {
  if (m_callbackCount) {
    --m_callbackCount;
  }

  if (!m_callbackCount) {
    FrameScript_SignalEvent(294);
  }
}

static int Script_CloseMerchant(lua_State *) {
  CGMerchantInfo::CloseMerchant();
  return 0;
}

static int Script_GetMerchantNumItems(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGMerchantInfo::GetNumItems()));
  return 1;
}

static int Script_GetMerchantItemInfo(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetMerchantItemInfo(index)");
  }
  const VendorItem *item = CGMerchantInfo::GetItem(static_cast<int>(lua_tonumber(L, 1)) - 1);
  if (!item || !CGMerchantInfo::GetMerchant() || !item->m_itemType) {
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnumber(L, 0.0);
    lua_pushnumber(L, 1.0);
    lua_pushnumber(L, 0.0);
    lua_pushnumber(L, 1.0);
    return 6;
  }

  const ItemStats *stats = CGMerchantInfo::GetItemStats(item->m_itemType);
  if (stats) {
    lua_pushstring(L, stats->m_displayName[CURRENT_LANGUAGE]);
  } else {
    lua_pushnil(L);
  }
  LPCSTR path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
  LPCSTR separator = path && *path ? "\\" : "";
  char   buffer[MAX_PATH];
  SStrPrintf(buffer, sizeof(buffer), "%s%s", path, separator);
  SStrCopy(buffer + strlen(buffer), CGItem_C::GetInventoryArt(item->m_itemDisplayID), sizeof(buffer) - strlen(buffer));
  lua_pushstring(L, buffer);
  lua_pushnumber(L, static_cast<double>(item->m_price));
  lua_pushnumber(L, static_cast<double>(item->m_stackCount));
  lua_pushnumber(L, static_cast<double>(item->m_quantity));

  CGPlayer_C     *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  GAME_ERROR_TYPE reason = GERR_NONE;
  if (player && stats && !player->CanUseItem(stats, reason)) {
    lua_pushnil(L);
  } else {
    lua_pushnumber(L, 1.0);
  }
  return 6;
}

static int Script_GetMerchantItemLink(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetMerchantItemLink(index)");
  }
  const VendorItem *item = CGMerchantInfo::GetItem(static_cast<int>(lua_tonumber(L, 1)) - 1);
  if (!item || !CGMerchantInfo::GetMerchant() || !item->m_itemType) {
    return 0;
  }
  const ItemStats *stats = CGMerchantInfo::GetItemStats(item->m_itemType);
  if (!stats) {
    return 0;
  }
  char link[1024];
  SStrPrintf(link, sizeof(link), "|Hitem:%d|h[%s]|h", item->m_itemType, stats->m_displayName[CURRENT_LANGUAGE]);
  lua_pushstring(L, link);
  return 1;
}

static int Script_GetMerchantItemMaxStack(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetMerchantItemMaxStack(index)");
  }
  const VendorItem *item = CGMerchantInfo::GetItem(static_cast<int>(lua_tonumber(L, 1)) - 1);
  const ItemStats  *stats = item && CGMerchantInfo::GetMerchant() && item->m_stackCount <= 1 ? CGMerchantInfo::GetItemStats(item->m_itemType) : 0;
  lua_pushnumber(L, stats ? static_cast<double>(stats->m_stackable) : 1.0);
  return 1;
}

static int Script_PickupMerchantItem(lua_State *L) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }
  DWORDLONG cursorItem = CGGameUI::GetCursorItem();
  if (cursorItem) {
    player->SellItem(CGMerchantInfo::GetMerchant(), cursorItem, 0);
    CGGameUI::ClearCursor(0);
    return 0;
  }
  if (!lua_isnumber(L, 1)) {
    CGGameUI::ClearCursor(1);
    return 0;
  }
  int               index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  const VendorItem *item = CGMerchantInfo::GetItem(index);
  if (!item || !item->m_muid) {
    CGGameUI::ClearCursor(1);
    return 0;
  }
  if (CGGameUI::GetCursorVirtualItem() == item->m_itemType) {
    CGGameUI::ClearCursor(1);
  } else {
    CGGameUI::SetCursorVirtualItem(item->m_itemType, item->m_itemDisplayID, index, UICURSOR_MERCHANT);
  }
  return 0;
}

static int Script_BuyMerchantItem(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: BuyMerchantItem(index)");
  }
  CGGameUI::ClearCursor(1);
  int  index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  UINT quantity = lua_isnumber(L, 2) ? static_cast<UINT>(lua_tonumber(L, 2)) : 1;
  if (!quantity) {
    quantity = 1;
  }
  const VendorItem *item = CGMerchantInfo::GetItem(index);
  if (item && item->m_muid) {
    CGPlayer_C::XBuyItem(CGMerchantInfo::GetMerchant(), item->m_muid, quantity, 1);
  }
  return 0;
}

static int Script_ShowMerchantSellCursor(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: ShowMerchantSellCursor(index)");
  }
  const VendorItem *item = CGMerchantInfo::GetItem(static_cast<int>(lua_tonumber(L, 1)) - 1);
  if (item && item->m_itemType) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    CursorModelSetSequence(player && player->GetUnitData()->coinage >= static_cast<UINT>(item->m_price) ? BUY_CURSOR : BUY_ERROR_CURSOR);
  }
  return 0;
}

static FrameScript_Method s_ScriptFunctions[8] = {
    {          "CloseMerchant",           Script_CloseMerchant},
    {    "GetMerchantNumItems",     Script_GetMerchantNumItems},
    {    "GetMerchantItemInfo",     Script_GetMerchantItemInfo},
    {    "GetMerchantItemLink",     Script_GetMerchantItemLink},
    {"GetMerchantItemMaxStack", Script_GetMerchantItemMaxStack},
    {     "PickupMerchantItem",      Script_PickupMerchantItem},
    {        "BuyMerchantItem",         Script_BuyMerchantItem},
    { "ShowMerchantSellCursor",  Script_ShowMerchantSellCursor}
};

void MerchantRegisterScriptFunctions() {
  for (UINT i = 0; i < 8; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void MerchantUnregisterScriptFunctions() {
  for (UINT i = 0; i < 8; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
