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

bool Spell_C_IsTargeting();
bool Spell_C_CanTargetItems();
bool Spell_C_HandleSpriteClick(CGObject_C *object);
void Spell_C_StopTargeting();

class CGBankInfo {
 public:
  static void      EnterWorld();
  static void      LeaveWorld();
  static void      OpenBank(const DWORDLONG &guid);
  static void      CloseBank();
  static void      OnCloseBank();
  static void      PickupItem(int slot, BOOL isBag, int slotIsButtonID);
  static void      SplitItem(int slot, int split);
  static DWORDLONG GetBanker() {
    return m_unit;
  }
  static DWORDLONG m_unit;
};

DWORDLONG CGBankInfo::m_unit;

static UINT GetBankSlotCost(int bankSlot) {
  const BankBagSlotPricesRec *record = g_bankBagSlotPricesDB.GetRecord(bankSlot);
  return record ? record->m_Cost : 0;
}

static inline UINT GetPlayerBankSlots(CGPlayer_C *player) {
  return player ? player->GetNumBankSlots() : 0;
}

static int Script_GetBankSlotCost(lua_State *L) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  UINT        cost = player ? GetBankSlotCost(GetPlayerBankSlots(player) + 1) : 0;
  lua_pushnumber(L, static_cast<double>(cost));
  return 1;
}

static int Script_GetNumBankSlots(lua_State *L) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  int         slots = GetPlayerBankSlots(player);
  lua_pushnumber(L, static_cast<double>(slots));
  if (slots >= 6) {
    lua_pushnumber(L, 1.0);
    return 2;
  }
  return 1;
}

static int Script_CloseBankFrame(lua_State *L) {
  CGBankInfo::OnCloseBank();
  return 0;
}

static int Script_PickupBankGenericItem(lua_State *L) {
  if (lua_isnumber(L, 1)) {
    CGBankInfo::PickupItem(static_cast<int>(lua_tonumber(L, 1)) - 1, lua_isnumber(L, 2), 1);
  }
  return 0;
}

static int Script_SplitBankGenericItem(lua_State *L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
    luaL_error(L, "Usage: SplitBankGenericItem(slot, amount)");
    return 0;
  }
  CGBankInfo::SplitItem(static_cast<int>(lua_tonumber(L, 1)) - 1, static_cast<int>(lua_tonumber(L, 2)));
  return 0;
}

static int ButtonIDToSlotID(int ID, BOOL isBag) {
  if (isBag) {
    ID += 59;
  } else {
    ID += 39;
  }
  return ID;
}

static int Script_BankButtonIDToInvSlotID(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return 0;
  }
  int ID = static_cast<int>(lua_tonumber(L, 1)) - 1;
  lua_pushnumber(L, static_cast<double>(ButtonIDToSlotID(ID, lua_isnumber(L, 2)) + 1));
  return 1;
}

static int Script_PutItemInBankBag(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: Script_PutItemInBankBag(unit, slot)");
    return 0;
  }
  int         slot = static_cast<int>(lua_tonumber(L, 1)) - 1;
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (slot >= 63 && slot <= 68 && player && CGGameUI::HasPlayerControl()) {
    DWORDLONG cursorItem;
    DWORDLONG cursorItemPack;
    UINT      cursorItemSlot;
    CGGameUI::GetCursorItem(cursorItem, cursorItemPack, cursorItemSlot);
    UINT virtualItem;
    UINT virtualSlot;
    CGGameUI::GetCursorVirtualItem(virtualItem, virtualSlot);
    DWORDLONG slotItem = player->CGPlayer_C::GetBag()->GetItem(slot);
    if (!slotItem) {
      CGBankInfo::PickupItem(slot, 1, 0);
      return 0;
    }
    if (cursorItem == slotItem) {
      CGGameUI::ClearCursor(1);
      return 1;
    }
    if (cursorItem || virtualItem) {
      CGItem_C *slotItemPtr = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(slotItem, __FILE__, __LINE__));
      if (!slotItemPtr || !slotItemPtr->IsA(ID_CONTAINER)) {
        if (slotItem != cursorItem) {
          slotItemPtr = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(slotItem, __FILE__, __LINE__));
          if (!slotItemPtr || !slotItemPtr->IsLocked()) {
            if (player->ValidateSlot(slot, cursorItem)) {
              player->SwapItems(cursorItem, cursorItemPack, cursorItemSlot, player->GetGUID(), slot, 0);
            } else if (cursorItemPack != player->GetGUID() || cursorItemSlot >= 19) {
              player->AutoEquipCursorItem(0);
            }
            CGGameUI::ClearCursor(1);
            lua_pushnumber(L, 1.0);
            return 1;
          }
        }
      } else if ((cursorItemPack != slotItem || (cursorItemPack == player->GetGUID() && cursorItemSlot <= 23)) && cursorItem) {
        if (CGGameUI::GetCursorStackSplit()) {
          player->SplitItem(cursorItem, cursorItemPack, cursorItemSlot, slotItem, 255, CGGameUI::GetCursorStackSplit());
          CGGameUI::ClearCursor(0);
          return 0;
        }
        const CGItem_C *cursorItemPtr = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(cursorItem, __FILE__, __LINE__));
        if (player->ValidateSlot(slot, cursorItem) && cursorItemPtr && cursorItemPtr->IsA(ID_CONTAINER)) {
          player->SwapItems(cursorItem, cursorItemPack, cursorItemSlot, player->GetGUID(), slot, 0);
        } else {
          player->AutoStoreItemInBag(cursorItem, cursorItemPack, cursorItemSlot, slotItem, 0);
        }
        CGGameUI::ClearCursor(0);
        lua_pushnumber(L, 1.0);
        return 1;
      }
    }
    CGGameUI::ClearCursor(1);
  }
  return 0;
}

static int Script_ContainerIDToInventoryID(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return 0;
  }
  int ID = static_cast<int>(lua_tonumber(L, 1)) - 1;
  if (ID >= 4) {
    lua_pushnumber(L, static_cast<double>(ID + 60));
    return 1;
  }
  lua_pushnumber(L, static_cast<double>(ID + 20));
  return 1;
}

static void SignalBankSlotsChanged() {
  FrameScript_SignalEvent(329);
}

static int Script_PurchaseSlot(lua_State *L) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    if (player->GetNumBankSlots() >= 6) {
      SignalBankSlotsChanged();
      return 0;
    }
    if (player->GetMoney() < GetBankSlotCost(player->GetNumBankSlots() + 1)) {
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(231));
      return 0;
    }

    CDataStore msg;
    msg.Put(CMSG_BUY_BANK_SLOT);
    msg.Put(CGGameUI::GetInteractTarget());
    msg.Finalize();
    ClientServices_Send(&msg);
  }
  return 0;
}

static int Script_PickupBagFromBankSlot(lua_State *L) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player || !CGGameUI::HasPlayerControl()) {
    return 0;
  }
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: PickupBagFromBankSlot(invSlot)");
    return 0;
  }
  int slot = static_cast<int>(lua_tonumber(L, 1) - 1.0);
  if (slot >= 63 && slot <= 68) {
    DWORDLONG cursorItem;
    DWORDLONG cursorItemPack;
    UINT      cursorItemSlot;
    CGGameUI::GetCursorItem(cursorItem, cursorItemPack, cursorItemSlot);
    if (cursorItem) {
      CGGameUI::ClearCursor(1);
    }
    DWORDLONG itemGUID = player->CGPlayer_C::GetBag()->GetItem(slot);
    if (!itemGUID) {
      CGBankInfo::PickupItem(slot, 1, 0);
      return 0;
    }
    CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(itemGUID, __FILE__, __LINE__));
    FATALASSERT(item->IsA(TYPE_CONTAINER));
    if (!item->IsLocked()) {
      CGGameUI::SetCursorItem(itemGUID, player->GetGUID(), slot, 1, 0);
      CGGameUI::LockItem(itemGUID);
    }
  }
  return 0;
}

static BOOL BankUpdateHandler(DWORDLONG, UINT, UINT, LPCVOID, LPVOID) {
  SignalBankSlotsChanged();
  return 1;
}

void CGBankInfo::PickupItem(int slot, BOOL isBag, int slotIsButtonID) {
  if (!CGGameUI::m_hasControl) {
    return;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return;
  }
  if (slotIsButtonID) {
    slot = ButtonIDToSlotID(slot, isBag);
  }

  DWORDLONG cursorItem;
  DWORDLONG cursorItemPack;
  UINT      cursorItemSlot;
  CGGameUI::GetCursorItem(cursorItem, cursorItemPack, cursorItemSlot);
  UINT virtualItem;
  UINT virtualSlot;
  CGGameUI::GetCursorVirtualItem(virtualItem, virtualSlot);

  CGBag_C  *inventory = player->CGPlayer_C::GetBag();
  DWORDLONG slotItem = inventory->GetItem(slot);
  if (!cursorItem) {
    if (!virtualItem && slotItem) {
      CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(slotItem, __FILE__, __LINE__));
      if (!item || !item->IsLocked()) {
        if (Spell_C_IsTargeting() && Spell_C_CanTargetItems()) {
          Spell_C_HandleSpriteClick(item);
        } else {
          CGGameUI::SetCursorItem(slotItem, player->GetGUID(), slot, 1, 0);
          CGGameUI::LockItem(slotItem);
        }
      }
    }
    return;
  }
  if (slotItem != cursorItem) {
    if (CGGameUI::GetCursorStackSplit()) {
      player->SplitItem(cursorItem, cursorItemPack, cursorItemSlot, player->GetGUID(), slot, CGGameUI::GetCursorStackSplit());
      CGGameUI::ClearCursor(0);
      return;
    }
    CGItem_C *slotItemPtr = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(slotItem, __FILE__, __LINE__));
    if (slotItemPtr && slotItemPtr->IsLocked()) {
      return;
    }
    if (player->ValidateSlot(slot, cursorItem)) {
      player->SwapItems(cursorItem, cursorItemPack, cursorItemSlot, player->GetGUID(), slot, 0);
      return;
    }
    if (cursorItemPack != player->GetGUID() || cursorItemSlot >= 19) {
      player->AutoEquipCursorItem(0);
      return;
    }
  }
  CGGameUI::ClearCursor(1);
}

void CGBankInfo::SplitItem(int slot, int split) {
  if (!CGGameUI::m_hasControl) {
    return;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return;
  }
  int       invSlot = ButtonIDToSlotID(slot, 0);
  DWORDLONG cursorItem;
  DWORDLONG cursorItemPack;
  UINT      cursorItemSlot;
  CGGameUI::GetCursorItem(cursorItem, cursorItemPack, cursorItemSlot);
  UINT virtualItem;
  UINT virtualSlot;
  CGGameUI::GetCursorVirtualItem(virtualItem, virtualSlot);
  CGBag_C  *inventory = player->CGPlayer_C::GetBag();
  DWORDLONG itemGUID = inventory->GetItem(invSlot);
  CGItem_C *itemPtr = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(itemGUID, __FILE__, __LINE__));
  if (!itemPtr || itemPtr->IsLocked() || split < 1 || split > itemPtr->GetStackCount()) {
    return;
  }
  CGGameUI::ClearCursor(1);
  Spell_C_StopTargeting();
  if (split == itemPtr->GetStackCount()) {
    CGGameUI::SetCursorItem(itemGUID, player->GetGUID(), invSlot, 1, 0);
  } else {
    CGGameUI::SetCursorItem(itemGUID, player->GetGUID(), invSlot, 1, split);
  }
  CGGameUI::LockItem(itemGUID);
}

void CGBankInfo::OpenBank(const DWORDLONG &guid) {
  OnCloseBank();
  if (guid) {
    m_unit = guid;
    CGGameUI::SetInteractTarget(guid, MAX_SHOP_DISTANCE_SQUARED);
    FrameScript_SignalEvent(327);
  }
}

void CGBankInfo::CloseBank() {
  FrameScript_SignalEvent(328);
}

void CGBankInfo::OnCloseBank() {
  if (m_unit) {
    CGGameUI::ClearInteractTarget(m_unit);
  }
  m_unit = 0;
}

void CGBankInfo::EnterWorld() {
  DWORDLONG player = ClntObjMgrGetActivePlayer();
  ClntObjMgrSetObjMirrorHandler(player, CGPlayer_C::OffsetOf(ID_PLAYER) + offsetof(CGPlayerData, numBankSlots), sizeof(((CGPlayerData *)0)->numBankSlots), BankUpdateHandler, 0, HANDLER_PRIORITY_NORMAL);
  SignalBankSlotsChanged();
}

void CGBankInfo::LeaveWorld() {
  DWORDLONG player = ClntObjMgrGetActivePlayer();
  ClntObjMgrUnsetObjMirrorHandler(player, CGPlayer_C::OffsetOf(ID_PLAYER) + offsetof(CGPlayerData, numBankSlots), BankUpdateHandler, 0);
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

void BankRegisterScriptFunctions() {
  for (UINT i = 0; i < 10; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void BankUnregisterScriptFunctions() {
  for (UINT i = 0; i < 10; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
