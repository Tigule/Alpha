#include "GameUI.h"
#include "ActionBarFrame.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "DB/DBClient/DBClient.h"
#include "DB/WowLocale.h"
#include "Object/ItemStats.h"
#include "Object/ObjectClient/Bag_C.h"
#include "Object/ObjectClient/Item_C.h"
#include "Object/ObjectClient/Object_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "UIUtil/Cursor.h"

#include <FrameScript/FrameScript.h>
#include <Frame/CSimpleRender.h>

#include <lauxlib.h>
#include <lua.h>
#include <storm.h>
#include <string.h>

int __fastcall Spell_C_GetItemCooldown(int itemID, unsigned int *duration, unsigned long *startTime, unsigned int *enable);

class CGContainerInfo {
 public:
  static void __fastcall             EnterWorld();
  static void __fastcall             LeaveWorld();
  static void __fastcall             UpdateContainers();
  static void __fastcall             UpdateContents(unsigned __int64 guid);
  static void __fastcall             UpdateCooldowns();
  static unsigned __int64 __fastcall GetContainer(int index);
  static void __fastcall             OpenContainer(unsigned __int64 container);
  static void __fastcall             UpdateItem(unsigned __int64 item);

 protected:
  static unsigned __int64 m_containers[10];
};

unsigned __int64 CGContainerInfo::m_containers[10];

static int __fastcall UpdateContainerContents(unsigned __int64 guid, unsigned int offset, unsigned int, const void *prevValue, void *) {
  CGContainerInfo::UpdateContents(guid);
  CGObject_C *object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
  if (object) {
    CGBag_C         *bag = object->GetBag();
    unsigned __int64 item = bag->GetItem((offset - 8) / 8);
    if (*static_cast<const unsigned __int64 *>(prevValue) != item) {
      CGGameUI::UnlockItem(item);
    }
  }
  return 1;
}

static int __fastcall UpdateInvContents(unsigned __int64 guid, unsigned int offset, unsigned int, const void *prevValue, void *) {
  CGContainerInfo::UpdateContents(guid);
  CGObject_C *object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
  if (object) {
    CGBag_C         *bag = object->GetBag();
    unsigned __int64 item = bag->GetItem(offset / 8);
    if (*static_cast<const unsigned __int64 *>(prevValue) != item) {
      CGGameUI::UnlockItem(item);
    }
  }
  return 1;
}

static int __fastcall InvUpdateHandler(unsigned __int64, unsigned int, unsigned int, const void *, void *) {
  CGContainerInfo::UpdateContainers();
  return 1;
}

void __fastcall CGContainerInfo::EnterWorld() {
  unsigned __int64 player = ClntObjMgrGetActivePlayer();
  unsigned int     playerOffset = CGPlayer_C::OffsetOf(ID_PLAYER);
  ClntObjMgrSetObjMirrorHandler(player, playerOffset + 152, 32, InvUpdateHandler, 0, HANDLER_PRIORITY_NORMAL);
  for (unsigned int offset = 184; offset <= 304; offset += 8) {
    ClntObjMgrSetObjMirrorHandler(player, playerOffset + offset, 8, UpdateInvContents, 0, HANDLER_PRIORITY_NORMAL);
  }
  ClntObjMgrSetObjMirrorHandler(player, playerOffset + 504, 48, InvUpdateHandler, 0, HANDLER_PRIORITY_NORMAL);
  memset(m_containers, 0, sizeof(m_containers));
  UpdateContainers();
}

void __fastcall CGContainerInfo::LeaveWorld() {
  unsigned int containerOffset = CGUnit_C::OffsetOf(ID_CONTAINER);
  for (int i = 0; i < 10; ++i) {
    unsigned __int64 container = m_containers[i];
    if (container) {
      CGObject_C *object = ClntObjMgrObjectPtr(container, __FILE__, __LINE__);
      if (object) {
        CGBag_C *bag = object->GetBag();
        for (unsigned int slot = 0; slot < bag->NumSlots(); ++slot) {
          ClntObjMgrUnsetObjMirrorHandler(container, containerOffset + 8 * slot + 8, UpdateContainerContents, 0);
        }
      }
      FrameScript_SignalEvent(306, "%d", i + 1);
    }
  }

  unsigned __int64 player = ClntObjMgrGetActivePlayer();
  unsigned int     playerOffset = CGPlayer_C::OffsetOf(ID_PLAYER);
  for (unsigned int offset = 184; offset <= 304; offset += 8) {
    ClntObjMgrUnsetObjMirrorHandler(player, playerOffset + offset, UpdateInvContents, 0);
  }
  ClntObjMgrUnsetObjMirrorHandler(player, playerOffset + 152, InvUpdateHandler, 0);
  ClntObjMgrUnsetObjMirrorHandler(player, playerOffset + 504, InvUpdateHandler, 0);
}

void __fastcall CGContainerInfo::UpdateContainers() {
  CGObject_C *object = ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__);
  if (!object) {
    return;
  }

  CGBag_C     *inventory = object->GetBag();
  unsigned int containerOffset = CGUnit_C::OffsetOf(ID_CONTAINER);
  for (unsigned int index = 0; index < 10; ++index) {
    unsigned int     slot = index < 4 ? index + 19 : index + 59;
    unsigned __int64 container = inventory->GetItem(slot);
    if (container == m_containers[index]) {
      continue;
    }

    if (m_containers[index]) {
      CGObject_C *oldObject = ClntObjMgrObjectPtr(m_containers[index], __FILE__, __LINE__);
      if (oldObject) {
        CGBag_C *oldBag = oldObject->GetBag();
        for (unsigned int item = 0; item < oldBag->NumSlots(); ++item) {
          ClntObjMgrUnsetObjMirrorHandler(m_containers[index], containerOffset + 8 * item + 8, UpdateContainerContents, 0);
        }
      }
      FrameScript_SignalEvent(306, "%d", index + 1);
    }

    if (container) {
      CGObject_C *newObject = ClntObjMgrObjectPtr(container, __FILE__, __LINE__);
      if (newObject) {
        CGBag_C *newBag = newObject->GetBag();
        for (unsigned int item = 0; item < newBag->NumSlots(); ++item) {
          ClntObjMgrSetObjMirrorHandler(container, containerOffset + 8 * item + 8, 8, UpdateContainerContents, 0, HANDLER_PRIORITY_NORMAL);
        }
      }
    }
    m_containers[index] = container;
  }
}

void __fastcall CGContainerInfo::UpdateContents(unsigned __int64 guid) {
  CGActionBar::UpdateUsable();
  if (guid == ClntObjMgrGetActivePlayer()) {
    FrameScript_SignalEvent(305, "%d", 0);
    return;
  }
  for (unsigned int index = 0; index < 10; ++index) {
    if (m_containers[index] == guid) {
      FrameScript_SignalEvent(305, "%d", index + 1);
      return;
    }
  }
}

void __fastcall CGContainerInfo::UpdateCooldowns() {
  FrameScript_SignalEvent(307);
}

unsigned __int64 __fastcall CGContainerInfo::GetContainer(int index) {
  if (!index) {
    return ClntObjMgrGetActivePlayer();
  }
  return index > 0 && index <= 10 ? m_containers[index - 1] : 0;
}

void __fastcall CGContainerInfo::OpenContainer(unsigned __int64 container) {
  if (container == ClntObjMgrGetActivePlayer()) {
    FrameScript_SignalEvent(304, "%d", 0);
    return;
  }

  for (unsigned int index = 0; index < 10; ++index) {
    if (m_containers[index] == container) {
      FrameScript_SignalEvent(304, "%d", index + 1);
    }
  }
}

void __fastcall CGContainerInfo::UpdateItem(unsigned __int64 item) {
  CGItem_C *itemPtr = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(item, __FILE__, __LINE__));
  if (itemPtr) {
    if (itemPtr->GetOwner() == ClntObjMgrGetActivePlayer()) {
      UpdateContents(itemPtr->GetContainedIn());
    }
  }
}

static int __fastcall Script_GetContainerNumSlots(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetContainerNumSlots(index)");
  }
  int index = static_cast<int>(lua_tonumber(L, 1));
  if (!index) {
    lua_pushnumber(L, 16.0);
    return 1;
  }
  unsigned __int64 guid = CGContainerInfo::GetContainer(index);
  CGObject_C      *object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
  CGBag_C         *bag = object ? object->GetBag() : 0;
  lua_pushnumber(L, bag ? static_cast<double>(bag->NumSlots()) : 0.0);
  return 1;
}

static int __fastcall Script_GetContainerItemInfo(lua_State *L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
    return luaL_error(L, "Usage: GetContainerItemInfo(index, slot)");
  }
  int              index = static_cast<int>(lua_tonumber(L, 1));
  unsigned int     slot = static_cast<unsigned int>(lua_tonumber(L, 2)) - 1;
  unsigned __int64 guid = CGContainerInfo::GetContainer(index);
  CGObject_C      *object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
  CGBag_C         *bag = object ? object->GetBag() : 0;
  if (!index) {
    slot += 23;
  }
  CGItem_C *item = bag ? static_cast<CGItem_C *>(ClntObjMgrObjectPtr(bag->GetItem(slot), __FILE__, __LINE__)) : 0;
  if (!item) {
    return 0;
  }
  const char *path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
  const char *separator = path && *path ? "\\" : "";
  char        buffer[260];
  SStrPrintf(buffer, sizeof(buffer), "%s%s%s", path, separator, item->GetInventoryArt());
  lua_pushstring(L, buffer);
  lua_pushnumber(L, static_cast<double>(item->GetStackCount()));
  item->IsUnlocked() ? lua_pushnil(L) : lua_pushnumber(L, 1.0);
  unsigned __int64 noGuid = 0;
  const ItemStats *stats = g_itemDBCache.GetRecord(item->GetEntryID(), noGuid, 0, 0);
  lua_pushnumber(L, stats ? static_cast<double>(stats->m_overallQualityID) : -2.0);
  return 4;
}

static int __fastcall Script_GetContainerItemLink(lua_State *L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
    return luaL_error(L, "Usage: GetContainerItemLink(index, slot)");
  }
  int              index = static_cast<int>(lua_tonumber(L, 1));
  unsigned int     slot = static_cast<unsigned int>(lua_tonumber(L, 2)) - 1;
  unsigned __int64 guid = CGContainerInfo::GetContainer(index);
  CGObject_C      *object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
  CGBag_C         *bag = object ? object->GetBag() : 0;
  if (!index) {
    slot += 23;
  }
  CGItem_C *item = bag ? static_cast<CGItem_C *>(ClntObjMgrObjectPtr(bag->GetItem(slot), __FILE__, __LINE__)) : 0;
  if (!item) {
    return 0;
  }
  unsigned __int64 noGuid = 0;
  const ItemStats *stats = g_itemDBCache.GetRecord(item->GetEntryID(), noGuid, 0, 0);
  if (!stats) {
    return 0;
  }
  char link[1024];
  SStrPrintf(link, sizeof(link), "|Hitem:%d|h[%s]|h", item->GetEntryID(), stats->m_displayName[CURRENT_LANGUAGE]);
  lua_pushstring(L, link);
  return 1;
}

static int __fastcall Script_GetContainerItemCooldown(lua_State *L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
    return luaL_error(L, "Usage: GetContainerItemCooldown(index, slot)");
  }
  int              index = static_cast<int>(lua_tonumber(L, 1));
  unsigned int     slot = static_cast<unsigned int>(lua_tonumber(L, 2)) - 1;
  unsigned __int64 guid = CGContainerInfo::GetContainer(index);
  CGObject_C      *object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
  CGBag_C         *bag = object ? object->GetBag() : 0;
  if (!index) {
    slot += 23;
  }
  CGItem_C     *item = bag ? static_cast<CGItem_C *>(ClntObjMgrObjectPtr(bag->GetItem(slot), __FILE__, __LINE__)) : 0;
  unsigned int  duration = 0;
  unsigned long startTime = 0;
  unsigned int  enable = 0;
  if (item) {
    Spell_C_GetItemCooldown(item->GetEntryID(), &duration, &startTime, &enable);
  }
  lua_pushnumber(L, static_cast<double>(startTime) * 0.001);
  lua_pushnumber(L, static_cast<double>(duration) * 0.001);
  lua_pushnumber(L, static_cast<double>(enable));
  return 3;
}

static int __fastcall Script_PickupContainerItem(lua_State *L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
    return luaL_error(L, "Usage: PickupContainerItem(index, slot)");
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }
  int              index = static_cast<int>(lua_tonumber(L, 1));
  unsigned int     slot = static_cast<unsigned int>(lua_tonumber(L, 2)) - 1;
  unsigned __int64 container = CGContainerInfo::GetContainer(index);
  CGObject_C      *object = ClntObjMgrObjectPtr(container, __FILE__, __LINE__);
  CGBag_C         *bag = object ? object->GetBag() : 0;
  if (!index) {
    slot += 23;
  }
  unsigned __int64 item = bag ? bag->GetItem(slot) : 0;
  unsigned __int64 cursorItem;
  unsigned __int64 cursorContainer;
  unsigned int     cursorSlot;
  CGGameUI::GetCursorItem(cursorItem, cursorContainer, cursorSlot);
  if (cursorItem) {
    if (cursorItem == item) {
      CGGameUI::ClearCursor(1);
    } else if (CGGameUI::GetCursorStackSplit()) {
      player->SplitItem(cursorItem, cursorContainer, cursorSlot, container, slot, CGGameUI::GetCursorStackSplit());
      CGGameUI::ClearCursor(0);
    } else {
      CGGameUI::LockItem(item);
      player->SwapItems(cursorItem, cursorContainer, cursorSlot, container, slot, 0);
      CGGameUI::ClearCursor(0);
    }
  } else if (item) {
    CGGameUI::SetCursorItem(item, container, slot, 1, 0);
    CGGameUI::LockItem(item);
  } else {
    CGGameUI::ClearCursor(1);
  }
  return 0;
}

static int __fastcall Script_SplitContainerItem(lua_State *L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
    return luaL_error(L, "Usage: SplitContainerItem(index, slot, amount)");
  }
  int              index = static_cast<int>(lua_tonumber(L, 1));
  unsigned int     slot = static_cast<unsigned int>(lua_tonumber(L, 2)) - 1;
  int              split = static_cast<int>(lua_tonumber(L, 3));
  unsigned __int64 container = CGContainerInfo::GetContainer(index);
  CGObject_C      *object = ClntObjMgrObjectPtr(container, __FILE__, __LINE__);
  CGBag_C         *bag = object ? object->GetBag() : 0;
  if (!index) {
    slot += 23;
  }
  CGItem_C *item = bag ? static_cast<CGItem_C *>(ClntObjMgrObjectPtr(bag->GetItem(slot), __FILE__, __LINE__)) : 0;
  if (item && item->IsUnlocked() && split >= 1 && split <= item->GetStackCount()) {
    CGGameUI::ClearCursor(1);
    CGGameUI::SetCursorItem(item->GetGUID(), container, slot, 1, split == item->GetStackCount() ? 0 : split);
    CGGameUI::LockItem(item->GetGUID());
  }
  return 0;
}

static int __fastcall Script_UseContainerItem(lua_State *L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
    return luaL_error(L, "Usage: UseContainerItem(index, slot)");
  }
  int index = static_cast<int>(lua_tonumber(L, 1));
  if (index >= 4) {
    return 0;
  }
  unsigned int     slot = static_cast<unsigned int>(lua_tonumber(L, 2)) - 1;
  unsigned __int64 container = CGContainerInfo::GetContainer(index);
  CGObject_C      *object = ClntObjMgrObjectPtr(container, __FILE__, __LINE__);
  CGBag_C         *bag = object ? object->GetBag() : 0;
  if (!index) {
    slot += 23;
  }
  CGItem_C *item = bag ? static_cast<CGItem_C *>(ClntObjMgrObjectPtr(bag->GetItem(slot), __FILE__, __LINE__)) : 0;
  if (item && item->IsUnlocked()) {
    CGGameUI::ClearCursor(1);
    if (!item->Use()) {
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (player) {
        player->AutoEquipItem(container, slot, 0);
      }
    }
  }
  return 0;
}

static int __fastcall Script_ShowContainerSellCursor(lua_State *L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
    return luaL_error(L, "Usage: ShowContainerSellCursor(index, slot)");
  }
  CursorModelSetSequence(BUY_CURSOR);
  return 0;
}

static int __fastcall Script_SetBagPortaitTexture(lua_State *L) {
  if (lua_type(L, 1) != LUA_TTABLE) {
    return luaL_error(L, "Attempt to find 'this' in non-table object (used '.' instead of ':' ?)");
  }
  lua_rawgeti(L, 1, 0);
  CSimpleTexture *texture = static_cast<CSimpleTexture *>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  FATALASSERT(texture);
  if (!lua_isnumber(L, 2)) {
    return luaL_error(L, "Usage: SetBagPortraitTexture(texture, slot)");
  }
  int              index = static_cast<int>(lua_tonumber(L, 2));
  unsigned __int64 container = CGContainerInfo::GetContainer(index);
  CGItem_C        *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(container, __FILE__, __LINE__));
  if (item) {
    const char *path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
    const char *separator = path && *path ? "\\" : "";
    char        buffer[260];
    SStrPrintf(buffer, sizeof(buffer), "%s%s%s.blp", path, separator, item->GetInventoryArt());
    texture->SetTexture(buffer, 0);
  }
  return 0;
}

static int __fastcall Script_GetBagName(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetBagName(index)");
  }
  int index = static_cast<int>(lua_tonumber(L, 1));
  if (!index) {
    lua_pushstring(L, FrameScript_GetText("BACKPACK_TOOLTIP", -1, GENDER_NOT_APPLICABLE));
    return 1;
  }
  CGItem_C        *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(CGContainerInfo::GetContainer(index), __FILE__, __LINE__));
  unsigned __int64 noGuid = 0;
  const ItemStats *stats = item ? g_itemDBCache.GetRecord(item->GetEntryID(), noGuid, 0, 0) : 0;
  stats ? lua_pushstring(L, stats->m_displayName[CURRENT_LANGUAGE]) : lua_pushnil(L);
  return 1;
}

static FrameScript_Method s_ScriptFunctions[10] = {
    {    "GetContainerNumSlots",     Script_GetContainerNumSlots},
    {    "GetContainerItemInfo",     Script_GetContainerItemInfo},
    {    "GetContainerItemLink",     Script_GetContainerItemLink},
    {"GetContainerItemCooldown", Script_GetContainerItemCooldown},
    {     "PickupContainerItem",      Script_PickupContainerItem},
    {      "SplitContainerItem",       Script_SplitContainerItem},
    {        "UseContainerItem",         Script_UseContainerItem},
    { "ShowContainerSellCursor",  Script_ShowContainerSellCursor},
    {    "SetBagPortaitTexture",     Script_SetBagPortaitTexture},
    {              "GetBagName",               Script_GetBagName}
};

void __fastcall ContainerRegisterScriptFunctions() {
  for (unsigned int i = 0; i < 10; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void __fastcall ContainerUnregisterScriptFunctions() {
  for (unsigned int i = 0; i < 10; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
