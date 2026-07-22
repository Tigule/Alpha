#include "GameUI.h"
#include "Object/ObjectClient/Unit_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "Object/ObjectClient/Item_C.h"
#include "Object/ObjectClient/Bag_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "DB/DBClient/AutoCode/BankBagSlotPricesRec.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <Base/CDataStore.h>
#include <FrameScript/FrameScript.h>

#include <lauxlib.h>
#include <lua.h>

static const float MAX_SHOP_DISTANCE = 5.5555553f;
static const float MAX_SHOP_DISTANCE_SQUARED = MAX_SHOP_DISTANCE * MAX_SHOP_DISTANCE;

class CGBankInfo {
 public:
  static void __fastcall  EnterWorld();
  static void __fastcall  LeaveWorld();
  static void __fastcall  OpenBank(const unsigned __int64 &guid);
  static void __fastcall  CloseBank();
  static void __fastcall  OnCloseBank();
  static void __fastcall  PickupItem(int slot, int isBag, int slotIsButtonID);
  static void __fastcall  SplitItem(int slot, int split);
  static unsigned __int64 GetBanker() {
    return m_unit;
  }
  static unsigned __int64 m_unit;
};

unsigned __int64 CGBankInfo::m_unit;

static unsigned int __fastcall GetBankSlotCost(int bankSlot) {
  BankBagSlotPricesRec *record = g_bankBagSlotPricesDB.GetRecord(bankSlot);
  return record ? record->m_Cost : 0;
}

static unsigned int __fastcall GetPlayerBankSlots(CGPlayer_C *player) {
  if (!player) {
    return 0;
  }
  const unsigned char *const *playerData = reinterpret_cast<const unsigned char *const *>(reinterpret_cast<const unsigned char *>(player) + 2528);
  return (*playerData)[1370];
}

static int __fastcall Script_GetBankSlotCost(lua_State *L) {
  CGPlayer_C  *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  unsigned int cost = player ? GetBankSlotCost(GetPlayerBankSlots(player) + 1) : 0;
  lua_pushnumber(L, static_cast<double>(cost));
  return 1;
}

static int __fastcall Script_GetNumBankSlots(lua_State *L) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  int         slots = GetPlayerBankSlots(player);
  lua_pushnumber(L, static_cast<double>(slots));
  if (slots >= 6) {
    lua_pushnumber(L, 1.0);
    return 2;
  }
  return 1;
}

static int __fastcall Script_CloseBankFrame(lua_State *L) {
  CGBankInfo::CloseBank();
  return 0;
}

static int __fastcall Script_PickupBankGenericItem(lua_State *L) {
  if (lua_isnumber(L, 1)) {
    CGBankInfo::PickupItem(static_cast<int>(lua_tonumber(L, 1)) - 1, lua_isnumber(L, 2), 1);
  }
  return 0;
}

static int __fastcall Script_SplitBankGenericItem(lua_State *L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
    return luaL_error(L, "Usage: SplitBankGenericItem(slot, amount)");
  }
  CGBankInfo::SplitItem(static_cast<int>(lua_tonumber(L, 1)) - 1, static_cast<int>(lua_tonumber(L, 2)));
  return 0;
}

static int __fastcall ButtonIDToSlotID(int ID, int isBag) {
  return isBag ? ID + 59 : ID + 39;
}

static int __fastcall Script_BankButtonIDToInvSlotID(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return 0;
  }
  int ID = static_cast<int>(lua_tonumber(L, 1)) - 1;
  int isBag = lua_isnumber(L, 2);
  lua_pushnumber(L, static_cast<double>(ButtonIDToSlotID(ID, isBag) + 1));
  return 1;
}

static int __fastcall Script_PutItemInBankBag(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: Script_PutItemInBankBag(unit, slot)");
  }
  int slot = static_cast<int>(lua_tonumber(L, 1)) - 1;
  if (slot >= 63 && slot <= 68) {
    CGBankInfo::PickupItem(slot, 1, 0);
  }
  return 0;
}

static int __fastcall Script_ContainerIDToInventoryID(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return 0;
  }
  int ID = static_cast<int>(lua_tonumber(L, 1)) - 1;
  lua_pushnumber(L, static_cast<double>((ID < 4 ? ID + 20 : ID + 60)));
  return 1;
}

static int __fastcall Script_PurchaseSlot(lua_State *L) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }
  unsigned int slots = GetPlayerBankSlots(player);
  if (slots >= 6) {
    return 0;
  }
  if (player->GetUnitData()->coinage < GetBankSlotCost(slots + 1)) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(231));
    return 0;
  }

  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_BUY_BANK_SLOT));
  msg.Put(CGGameUI::GetInteractTarget());
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int __fastcall Script_PickupBagFromBankSlot(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: PickupBagFromBankSlot(invSlot)");
  }
  int slot = static_cast<int>(lua_tonumber(L, 1)) - 1;
  if (slot >= 63 && slot <= 68) {
    CGBankInfo::PickupItem(slot, 1, 0);
  }
  return 0;
}

static void __fastcall SignalBankSlotsChanged() {
  FrameScript_SignalEvent(329);
}

static int __fastcall BankUpdateHandler(unsigned __int64, unsigned int, unsigned int, const void *, void *) {
  SignalBankSlotsChanged();
  return 1;
}

void __fastcall CGBankInfo::PickupItem(int slot, int isBag, int slotIsButtonID) {
  if (!m_unit) {
    return;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return;
  }
  if (slotIsButtonID) {
    slot = ButtonIDToSlotID(slot, isBag);
  }

  unsigned __int64 cursorItem;
  unsigned __int64 cursorItemPack;
  unsigned int     cursorItemSlot;
  CGGameUI::GetCursorItem(cursorItem, cursorItemPack, cursorItemSlot);
  unsigned int virtualItem;
  unsigned int virtualSlot;
  CGGameUI::GetCursorVirtualItem(virtualItem, virtualSlot);

  CGBag_C         *inventory = reinterpret_cast<CGBag_C *>(reinterpret_cast<unsigned char *>(player) + 6200);
  unsigned __int64 slotItem = inventory->GetItem(slot);
  if (!cursorItem && !virtualItem) {
    if (slotItem) {
      CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(slotItem, __FILE__, __LINE__));
      if (!item || item->IsUnlocked()) {
        CGGameUI::SetCursorItem(slotItem, player->GetGUID(), slot, 0, 0);
        CGGameUI::LockItem(slotItem);
      }
    }
    return;
  }
  if (cursorItem == slotItem) {
    CGGameUI::ClearCursor(1);
    return;
  }
  if (cursorItem) {
    player->SwapItems(cursorItem, cursorItemPack, cursorItemSlot, player->GetGUID(), slot, 0);
    CGGameUI::ClearCursor(0);
  }
}

void __fastcall CGBankInfo::SplitItem(int slot, int split) {
  if (!m_unit) {
    return;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return;
  }
  slot = ButtonIDToSlotID(slot, 0);
  CGBag_C         *inventory = reinterpret_cast<CGBag_C *>(reinterpret_cast<unsigned char *>(player) + 6200);
  unsigned __int64 itemGUID = inventory->GetItem(slot);
  CGItem_C        *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(itemGUID, __FILE__, __LINE__));
  if (!item || !item->IsUnlocked() || split < 1 || split > item->GetStackCount()) {
    return;
  }
  CGGameUI::ClearCursor(1);
  CGGameUI::SetCursorItem(itemGUID, player->GetGUID(), slot, 0, split == item->GetStackCount() ? 0 : split);
  CGGameUI::LockItem(itemGUID);
}

void __fastcall CGBankInfo::CloseBank() {
  FrameScript_SignalEvent(328);
}

void __fastcall CGBankInfo::OpenBank(const unsigned __int64 &guid) {
  OnCloseBank();
  if (guid) {
    m_unit = guid;
    CGGameUI::SetInteractTarget(guid, MAX_SHOP_DISTANCE_SQUARED);
    FrameScript_SignalEvent(327);
  }
}

void __fastcall CGBankInfo::OnCloseBank() {
  if (m_unit) {
    CGGameUI::ClearInteractTarget(m_unit);
  }
  m_unit = 0;
}

void __fastcall CGBankInfo::EnterWorld() {
  unsigned __int64 player = ClntObjMgrGetActivePlayer();
  unsigned int     playerOffset = CGPlayer_C::OffsetOf(ID_PLAYER);
  ClntObjMgrSetObjMirrorHandler(player, playerOffset + 1370, 1, BankUpdateHandler, 0, HANDLER_PRIORITY_NORMAL);
  SignalBankSlotsChanged();
}

void __fastcall CGBankInfo::LeaveWorld() {
  unsigned __int64 player = ClntObjMgrGetActivePlayer();
  unsigned int     playerOffset = CGPlayer_C::OffsetOf(ID_PLAYER);
  ClntObjMgrUnsetObjMirrorHandler(player, playerOffset + 1370, BankUpdateHandler, 0);
}

static FrameScript_Method s_ScriptFunctions[10] = {
    {         "GetBankSlotCost",          Script_GetBankSlotCost},
    {         "GetNumBankSlots",          Script_GetNumBankSlots},
    {          "CloseBankFrame",           Script_CloseBankFrame},
    {   "PickupBankGenericItem",    Script_PickupBankGenericItem},
    {    "SplitBankGenericItem",     Script_SplitBankGenericItem},
    { "BankButtonIDToInvSlotID",  Script_BankButtonIDToInvSlotID},
    {        "PutItemInBankBag",         Script_PutItemInBankBag},
    {"ContainerIDToInventoryID", Script_ContainerIDToInventoryID},
    {            "PurchaseSlot",             Script_PurchaseSlot},
    {   "PickupBagFromBankSlot",    Script_PickupBagFromBankSlot}
};

void __fastcall BankRegisterScriptFunctions() {
  for (unsigned int i = 0; i < 10; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void __fastcall BankUnregisterScriptFunctions() {
  for (unsigned int i = 0; i < 10; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
