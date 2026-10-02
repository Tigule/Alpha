#include <Base/Base.h>
#include <Frame/CSimpleTop.h>
#include <Frame/CSimpleModel.h>
#include <WowConst.h>
#include <MapDefs.h>
#include <WorldClient/World.h>
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "SoundInterface/SoundInterface.h"
#include "WorldFrame.h"
#include "GameUI.h"

#include "PaperDollInfoFrame.h"

#include "Object/ObjectClient/Unit_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "DB/DBClient/AutoCode/CharBaseInfoRec.h"
#include "DB/DBClient/AutoCode/ChrProficiencyRec.h"
#include "DB/DBClient/AutoCode/ItemSubClassRec.h"
#include "DB/DBClient/AutoCode/PaperDollItemFrameRec.h"
#include "DB/DBClient/AutoCode/SkillLineRec.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "DB/DBClient/DBClient.h"
#include "Object/ItemStats.h"
#include "Object/ObjectClient/Item_C.h"
#include "Ui/GameUI.h"
#include "Ui/MerchantFrame.h"
#include "UIUtil/Cursor.h"

#include <Frame/CSimpleRender.h>
#include <FrameScript/FrameScript.h>
#include <lauxlib.h>
#include <lua.h>
#include <stdlib.h>
#include <string.h>

bool      Spell_C_IsTargeting();
bool      Spell_C_CanTargetItems();
bool      Spell_C_HandleSpriteClick(CGObject_C *object);
int       Spell_C_GetItemCooldown(int itemID, UINT *duration, DWORD *startTime, UINT *enable);
void      CursorModelSetSequence(CURSORANIMATIONS sequence);
CGUnit_C *Script_GetUnitFromName(LPCSTR name);
void      SetPortraitTexture(CSimpleTexture *texture, LPCSTR textureFile);
void      Script_SendUnitSignal(const DWORDLONG &guid, int signal);

SkillInfo             CGCharacterInfo::m_skillInfoList[93];
UINT                  CGCharacterInfo::m_profOffset;
UINT                  CGCharacterInfo::m_specialOffset;
UINT                  CGCharacterInfo::m_racialOffset;
UINT                  CGCharacterInfo::m_secondaryOffset;
UINT                  CGCharacterInfo::m_numSkills;
CGCharacterModelBase *CGCharacterInfo::m_paperDoll;

static UINT s_playerLevel;

struct ProficiencyInfo {
  int minLevel;
  int slot;
};

static int __cdecl            QSortCompareByCategoryAndLevel(LPCVOID a, LPCVOID b);
static const CharBaseInfoRec *GetCharBaseInfo(int raceID, int classID);
static const ItemSubClassRec *FindItemSubClassRecord(int classID, int subClassID);
static int __cdecl            QSortCompareProficiency(LPCVOID a, LPCVOID b);

static BOOL PlayerCharacterPointsUpdateHandler(DWORDLONG, UINT, UINT, LPCVOID, LPVOID) {
  CGCharacterInfo::UpdateAllSkillLines();
  return 1;
}

void CGCharacterInfo::InitializeGame() {
  for (UINT i = 0; i < 93; ++i) {
    m_skillInfoList[i].isProf = 0;
    m_skillInfoList[i].skillID = 0;
  }
}

void CGCharacterInfo::ShutdownGame() {
}

void CGCharacterInfo::EnterWorld() {
  InstallMirrorHandlers(ClntObjMgrGetActivePlayer());
  FrameScript_SignalEvent(180, "%s", "player");
  FrameScript_SignalEvent(181, "%s", "player");
  FrameScript_SignalEvent(183, "%s", "player");
  FrameScript_SignalEvent(326, "%s", "player");
  UpdateAllSkillLines();
}

void CGCharacterInfo::LeaveWorld() {
  RemoveMirrorHandlers(ClntObjMgrGetActivePlayer());
}

void CGCharacterInfo::InstallMirrorHandlers(DWORDLONG player) {
  if (player) {
    UINT playerOffset = CGPlayer_C::OffsetOf(ID_PLAYER);
    ClntObjMgrSetObjMirrorHandler(player, playerOffset + offsetof(CGPlayerData, characterPoints), sizeof(((CGPlayerData *)0)->characterPoints), PlayerCharacterPointsUpdateHandler, 0, HANDLER_PRIORITY_NORMAL);
  }
}

void CGCharacterInfo::RemoveMirrorHandlers(DWORDLONG player) {
  if (player) {
    UINT playerOffset = CGPlayer_C::OffsetOf(ID_PLAYER);
    ClntObjMgrUnsetObjMirrorHandler(player, playerOffset + offsetof(CGPlayerData, characterPoints), PlayerCharacterPointsUpdateHandler, 0);
  }
}

void CGCharacterInfo::UpdateItem(DWORDLONG item) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return;
  }

  int slot = player->CGPlayer_C::GetBag()->GetIndexOfObject(item);
  if (slot == -1) {
    CGItem_C *itemPtr = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(item, __FILE__, __LINE__));
    if (!itemPtr || itemPtr->GetContainedIn() == ClntObjMgrGetActivePlayer()) {
      return;
    }
    slot = player->CGPlayer_C::GetBag()->GetIndexOfObject(itemPtr->GetContainedIn());
  }

  if (slot >= 0 && slot < NUM_INVENTORY_SLOTS) {
    FrameScript_SignalEvent(183, "%s", "player");
  } else if ((slot >= BANKGENERIC_FIRST && slot <= BANKGENERIC_LAST) || (slot >= BANKBAG_FIRST && slot <= BANKBAG_LAST)) {
    FrameScript_SignalEvent(326, "%s", "player");
  }
}

void CGCharacterInfo::PickupItem(int slot) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player || (!CGGameUI::HasPlayerControl() && !((player->GetUnitData()->flags >> 20) & 1))) {
    return;
  }

  DWORDLONG cursorItem;
  DWORDLONG cursorItemPack;
  UINT      cursorItemSlot;
  CGGameUI::GetCursorItem(cursorItem, cursorItemPack, cursorItemSlot);
  UINT virtualItem;
  UINT virtualSlot;
  CGGameUI::GetCursorVirtualItem(virtualItem, virtualSlot);

  DWORDLONG itemGUID = player->CGPlayer_C::GetBag()->GetItem(slot);
  if (!cursorItem && !virtualItem) {
    if (itemGUID) {
      CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(itemGUID, __FILE__, __LINE__));
      if (!item || !item->IsLocked()) {
        if (Spell_C_IsTargeting() && Spell_C_CanTargetItems()) {
          Spell_C_HandleSpriteClick(item);
        } else {
          CGGameUI::SetCursorItem(itemGUID, player->GetGUID(), slot, 1, 0);
          CGGameUI::LockItem(itemGUID);
        }
      }
    }
    return;
  }

  if (cursorItem) {
    if (cursorItem == itemGUID) {
      CGGameUI::ClearCursor(1);
      return;
    }

    CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(itemGUID, __FILE__, __LINE__));
    if (item && item->IsLocked()) {
      return;
    }
    if (player->ValidateSlot(slot, cursorItem)) {
      player->SwapItems(cursorItem, cursorItemPack, cursorItemSlot, player->GetGUID(), slot, 0);
      CGGameUI::LockItem(itemGUID);
    } else if (cursorItemPack == player->GetGUID() && cursorItemSlot < INVSLOT_BAG0) {
      CGGameUI::ClearCursor(1);
    } else {
      player->AutoEquipCursorItem(0);
    }
    return;
  }

  if (CGGameUI::GetCursorType() == UICURSOR_MERCHANT && CGMerchantInfo::GetMerchant()) {
    const VendorItem *item = CGMerchantInfo::GetItem(virtualSlot);
    if (item) {
      CGPlayer_C::XBuyItemInSlot(CGMerchantInfo::GetMerchant(), item->m_itemType, 1, player->GetGUID(), slot);
      CGGameUI::ClearCursor(1);
    }
  }
}

void CGCharacterInfo::UseItem(int slot) {
  if (!CGGameUI::HasPlayerControl()) {
    return;
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return;
  }

  CGGameUI::ClearCursor(1);
  CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(player->CGPlayer_C::GetBag()->GetItem(slot), __FILE__, __LINE__));
  if (item) {
    item->Use();
  }
}

void CGCharacterInfo::PickupBag(int slot) {
  ASSERT(slot >= INVSLOT_BAG0);
  ASSERT(slot < NUM_INVENTORY_SLOTS);

  if (!CGGameUI::HasPlayerControl()) {
    return;
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return;
  }

  DWORDLONG cursorItem;
  DWORDLONG cursorItemPack;
  UINT      cursorItemSlot;
  CGGameUI::GetCursorItem(cursorItem, cursorItemPack, cursorItemSlot);
  if (cursorItem) {
    CGGameUI::ClearCursor(1);
  }

  DWORDLONG itemGUID = player->CGPlayer_C::GetBag()->GetItem(slot);
  if (!itemGUID) {
    PickupItem(slot);
    return;
  }

  CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(itemGUID, __FILE__, __LINE__));
  ASSERT(item->IsA(TYPE_CONTAINER));
  if (!item->IsLocked()) {
    CGGameUI::SetCursorItem(itemGUID, player->GetGUID(), slot, 1, 0);
    CGGameUI::LockItem(itemGUID);
  }
}

BOOL CGCharacterInfo::PutItemInBag(int slot) {
  if (slot != 255) {
    ASSERT(slot >= INVSLOT_BAG0);
    ASSERT(slot < NUM_INVENTORY_SLOTS);
  }
  if (!CGGameUI::HasPlayerControl()) {
    return 0;
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }

  DWORDLONG cursorItem;
  DWORDLONG cursorItemPack;
  UINT      cursorItemSlot;
  CGGameUI::GetCursorItem(cursorItem, cursorItemPack, cursorItemSlot);
  UINT virtualItem;
  UINT virtualSlot;
  CGGameUI::GetCursorVirtualItem(virtualItem, virtualSlot);

  DWORDLONG targetBag = slot == 255 ? player->GetGUID() : player->CGPlayer_C::GetBag()->GetItem(slot);
  if (!targetBag) {
    PickupItem(slot);
    return 0;
  }
  if (cursorItem == targetBag) {
    CGGameUI::ClearCursor(1);
    return 1;
  }
  if (!cursorItem && !virtualItem) {
    CGGameUI::ClearCursor(1);
    return 0;
  }
  if (cursorItemPack == targetBag &&
      (cursorItemPack != player->GetGUID() || (cursorItemSlot >= BACKPACK_FIRST && cursorItemSlot <= BACKPACK_LAST))) {
    CGGameUI::ClearCursor(1);
    return 1;
  }

  if (cursorItem) {
    if (slot != 255) {
      if (cursorItemPack == player->GetGUID() && cursorItemSlot >= INVSLOT_BAGFIRST && cursorItemSlot <= INVSLOT_BAGLAST) {
        CGGameUI::LockItem(targetBag);
        player->SwapItems(cursorItem, cursorItemPack, cursorItemSlot, player->GetGUID(), slot, 0);
        return 1;
      }

      CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(cursorItem, __FILE__, __LINE__));
      if (item && item->IsA(TYPE_CONTAINER)) {
        CGGameUI::LockItem(targetBag);
        player->SwapItems(cursorItem, cursorItemPack, cursorItemSlot, player->GetGUID(), slot, 0);
        return 1;
      }
    }

    UINT split = CGGameUI::GetCursorStackSplit();
    if (split) {
      player->SplitItem(cursorItem, cursorItemPack, cursorItemSlot, targetBag, 255, split);
    } else {
      player->AutoStoreItemInBag(cursorItem, cursorItemPack, cursorItemSlot, targetBag, 0);
    }
    CGGameUI::ClearCursor(0);
    return 1;
  }

  if (CGGameUI::GetCursorType() == UICURSOR_MERCHANT && CGMerchantInfo::GetMerchant()) {
    const VendorItem *item = CGMerchantInfo::GetItem(virtualSlot);
    CGPlayer_C::XBuyItemInSlot(CGMerchantInfo::GetMerchant(), item->m_itemType, 1, targetBag, 255);
  }
  CGGameUI::ClearCursor(1);
  return 0;
}

int CGCharacterInfo::PutItemInBackpack() {
  return PutItemInBag(255);
}

void CGCharacterInfo::UpdateAllSkillLines() {
  OrderSkillLines();
  FrameScript_SignalEvent(248);
}

static int __cdecl QSortCompareByCategoryAndLevel(LPCVOID a, LPCVOID b) {
  FATALASSERT(a);
  FATALASSERT(b);
  const SkillLineRec *skill1 = *static_cast<const SkillLineRec *const *>(a);
  const SkillLineRec *skill2 = *static_cast<const SkillLineRec *const *>(b);
  if (skill1->m_skillType != skill2->m_skillType) {
    return skill1->m_skillType > skill2->m_skillType ? 1 : -1;
  }
  if (skill1->m_minCharLevel <= static_cast<int>(s_playerLevel)) {
    if (skill2->m_minCharLevel <= static_cast<int>(s_playerLevel)) {
      if (skill1->m_categoryID != skill2->m_categoryID) {
        return skill1->m_categoryID > skill2->m_categoryID ? 1 : -1;
      }
      return SStrCmp(skill1->m_displayName_lang[CURRENT_LANGUAGE], skill2->m_displayName_lang[CURRENT_LANGUAGE], 4);
    }
    return -1;
  }
  if (skill2->m_minCharLevel <= static_cast<int>(s_playerLevel)) {
    return 1;
  }
  if (skill1->m_minCharLevel != skill2->m_minCharLevel) {
    return skill1->m_minCharLevel > skill2->m_minCharLevel ? 1 : -1;
  }
  return SStrCmp(skill1->m_displayName_lang[CURRENT_LANGUAGE], skill2->m_displayName_lang[CURRENT_LANGUAGE], 4);
}

void CGCharacterInfo::OrderSkillLines() {
  const SkillLineRec *skillInfo[64];
  UINT                count = 0;
  CGPlayer_C         *playerPtr = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!playerPtr) {
    return;
  }
  UINT i;
  UINT numClassSkills = 0;
  for (i = 0; i < 69; ++i) {
    m_skillInfoList[i].isProf = 0;
    m_skillInfoList[i].skillID = 0;
    if (i < 64) {
      UINT skillID = playerPtr->GetMirrorSkillID(i);
      if (skillID) {
        const SkillLineRec *rec = g_skillLineDB.GetRecord(skillID);
        if (rec && playerPtr->GetMirrorSkillMaxRank(i) && rec->m_categoryID < 4) {
          if (!rec->m_skillType) {
            ++numClassSkills;
          }
          skillInfo[count++] = rec;
        }
      }
    }
  }
  s_playerLevel = playerPtr->GetUnitData()->level;
  qsort(skillInfo, count, sizeof(SkillLineRec *), QSortCompareByCategoryAndLevel);
  m_profOffset = numClassSkills + 1;
  m_specialOffset = 0;
  m_racialOffset = 0;
  m_secondaryOffset = 0;
  UINT numProfs = OrderProficiencies(numClassSkills + 1);
  if (numProfs) {
    ++numProfs;
  }
  m_skillInfoList[0].isProf = 0;
  m_skillInfoList[0].skillID = 0;
  UINT skillCount = 1;
  UINT output = numProfs + 1;
  for (i = 0; i < count; ++i) {
    const SkillLineRec *rec = skillInfo[i];
    if (!m_specialOffset && rec->m_skillType == 1) {
      m_specialOffset = output++;
      ++skillCount;
    }
    if (!m_racialOffset && rec->m_skillType == 2) {
      m_racialOffset = output;
      if (!m_specialOffset) {
        m_specialOffset = output;
      }
      ++skillCount;
      ++output;
    }
    if (!m_secondaryOffset && (rec->m_skillType == 3 || rec->m_skillType == 4)) {
      m_secondaryOffset = output;
      if (!m_racialOffset) {
        m_racialOffset = output;
      }
      if (!m_specialOffset) {
        m_specialOffset = output;
      }
      ++skillCount;
      ++output;
    }
    UINT index = skillCount + (rec->m_skillType ? numProfs : 0);
    m_skillInfoList[index].isProf = 0;
    m_skillInfoList[index].skillID = rec->m_ID;
    ++skillCount;
    ++output;
  }
  if (!m_secondaryOffset) {
    m_secondaryOffset = skillCount;
  }
  if (!m_racialOffset) {
    m_racialOffset = skillCount;
  }
  if (!m_specialOffset) {
    m_specialOffset = skillCount;
  }
  m_numSkills = numProfs + skillCount;
}

static const CharBaseInfoRec *GetCharBaseInfo(int raceID, int classID) {
  int i;
  for (i = 0; i < g_charBaseInfoDB.GetNumRecords(); ++i) {
    const CharBaseInfoRec *rec = g_charBaseInfoDB.GetRecordByIndex(i);
    if (rec->m_raceID == raceID && rec->m_classID == classID) {
      return rec;
    }
  }
  return 0;
}

static const ItemSubClassRec *FindItemSubClassRecord(int classID, int subClassID) {
  int i;
  for (i = 0; i < g_itemSubClassDB.GetNumRecords(); ++i) {
    const ItemSubClassRec *rec = g_itemSubClassDB.GetRecordByIndex(i);
    if (rec->m_classID == classID && rec->m_subClassID == subClassID) {
      return rec;
    }
  }
  return 0;
}

static int __cdecl QSortCompareProficiency(LPCVOID a, LPCVOID b) {
  FATALASSERT(a);
  FATALASSERT(b);
  const ProficiencyInfo *info1 = static_cast<const ProficiencyInfo *>(a);
  const ProficiencyInfo *info2 = static_cast<const ProficiencyInfo *>(b);
  if (info1->minLevel == info2->minLevel) {
    return 0;
  }
  return info1->minLevel > info2->minLevel ? 1 : -1;
}

UINT CGCharacterInfo::OrderProficiencies(UINT offset) {
  ProficiencyInfo orderedSlots[16];
  UINT            numProfs = 0;
  int             i;
  for (i = 0; i < g_itemSubClassDB.GetNumRecords(); ++i) {
    const ItemSubClassRec *rec = g_itemSubClassDB.GetRecordByIndex(i);
    if (rec->m_classID >= 0 && rec->m_classID < 16 && (rec->m_displayName_lang[CURRENT_LANGUAGE] || *rec->m_displayName_lang[CURRENT_LANGUAGE])) {
      FATALASSERT(numProfs < 24);
      UINT proficiency = CGPlayer_C::GetProficiency(static_cast<BYTE>(rec->m_classID));
      UINT bit = 1 << rec->m_subClassID;
      if ((proficiency & bit) && (rec->m_classID != 4 || !(proficiency & (1 << rec->m_postrequisiteProficiency)))) {
        LPCSTR name = rec->m_verboseName_lang[CURRENT_LANGUAGE];
        if (name && *name) {
          SkillInfo *info = &m_skillInfoList[offset + numProfs];
          info->isProf = 1;
          info->profLevel = 0;
          SStrCopy(info->profName, name, sizeof(info->profName));
          ++numProfs;
          FATALASSERT(numProfs < 24);
        }
      }
    }
  }
  CGPlayer_C *playerPtr = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!playerPtr) {
    return numProfs;
  }
  const CharBaseInfoRec *base = GetCharBaseInfo(playerPtr->GetUnitData()->race, playerPtr->GetUnitData()->classId);
  if (!base) {
    return numProfs;
  }
  const ChrProficiencyRec *proficiencyRec = g_chrProficiencyDB.GetRecord(base->m_proficiency);
  if (!proficiencyRec) {
    return numProfs;
  }
  UINT orderedCount = 0;
  for (i = 0; i < 16; ++i) {
    if (proficiencyRec->m_proficiency_minLevel[i] > playerPtr->GetUnitData()->level && proficiencyRec->m_proficiency_acquireMethod[i] == 1) {
      orderedSlots[orderedCount].minLevel = proficiencyRec->m_proficiency_minLevel[i];
      orderedSlots[orderedCount].slot = i;
      ++orderedCount;
    }
  }
  qsort(orderedSlots, orderedCount, sizeof(ProficiencyInfo), QSortCompareProficiency);
  for (i = 0; i < static_cast<int>(orderedCount); ++i) {
    int  slot = orderedSlots[i].slot;
    int  itemClass = proficiencyRec->m_proficiency_itemClass[slot];
    UINT proficiency = CGPlayer_C::GetProficiency(static_cast<BYTE>(itemClass));
    UINT bit;
    for (bit = 0; bit < 32; ++bit) {
      if ((proficiencyRec->m_proficiency_itemSubClassMask[slot] & (1 << bit)) && !(proficiency & (1 << bit))) {
        const ItemSubClassRec *rec = FindItemSubClassRecord(itemClass, bit);
        if (rec) {
          LPCSTR name = rec->m_verboseName_lang[CURRENT_LANGUAGE];
          if (*name) {
            SkillInfo *info = &m_skillInfoList[offset + numProfs];
            info->isProf = 1;
            info->profLevel = proficiencyRec->m_proficiency_minLevel[slot];
            SStrCopy(info->profName, name, sizeof(info->profName));
            ++numProfs;
          }
        }
      }
      FATALASSERT(numProfs < 24);
    }
  }
  return numProfs;
}

int CGCharacterInfo::GetSkillOffsetFromString(LPCSTR string, int &offset) {
  if (!string || !*string) {
    return 0;
  }
  if (!SStrCmpI(string, "class", 0x7FFFFFFF)) {
    offset = 0;
    return 1;
  }
  if (!SStrCmpI(string, "spec", 0x7FFFFFFF)) {
    offset = m_specialOffset;
    return 1;
  }
  if (!SStrCmpI(string, "racial", 0x7FFFFFFF)) {
    offset = m_racialOffset;
    return 1;
  }
  if (!SStrCmpI(string, "secondary", 0x7FFFFFFF)) {
    offset = m_secondaryOffset;
    return 1;
  }
  if (!SStrCmpI(string, "proficiency", 0x7FFFFFFF)) {
    offset = m_profOffset;
    return 1;
  }
  return 0;
}

const SkillInfo *CGCharacterInfo::GetSkillInfoByIndex(int index) {
  return index >= 0 && static_cast<UINT>(index) < 93 ? &m_skillInfoList[index] : 0;
}

static BOOL GetSlotFromLua(lua_State *L, int &slot, int index) {
  if (!lua_isnumber(L, index)) {
    return 0;
  }
  slot = static_cast<int>(lua_tonumber(L, index)) - 1;
  return slot >= 0 && slot <= 22 || slot >= 39 && slot <= 62 || slot >= 63 && slot <= 68;
}

static int Script_GetInventorySlotInfo(lua_State *L) {
  LPCSTR string;
  int    numEntries;
  if (lua_isstring(L, 1)) {
    string = lua_tostring(L, 1);
    numEntries = g_paperDollItemFrameDB.GetNumRecords();
    for (int i = 0; i < numEntries; ++i) {
      const PaperDollItemFrameRec *record = g_paperDollItemFrameDB.GetRecordByIndex(i);
      if (record && !SStrCmpI(record->m_ItemButtonName, string, 0x7FFFFFFF)) {
        lua_pushnumber(L, record->m_SlotNumber);
        lua_pushstring(L, record->m_SlotIcon);
        return 2;
      }
    }
  }
  luaL_error(L, "Invalid inventory slot in GetInventorySlotInfo");
  return 0;
}

static int Script_GetInventoryItemTexture(lua_State *L) {
  int slot = 0;
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: GetInventoryItemTexture(unit, slot)");
    return 0;
  }
  if (!GetSlotFromLua(L, slot, 2)) {
    luaL_error(L, "Invalid inventory slot in GetInventoryItemTexture");
    return 0;
  }
  CGUnit_C   *unit = Script_GetUnitFromName(lua_tostring(L, 1));
  CGPlayer_C *player = unit && unit->GetType() & TYPE_PLAYER ? static_cast<CGPlayer_C *>(unit) : 0;
  CGBag_C    *bag = player ? player->CGPlayer_C::GetBag() : 0;
  CGItem_C   *item = bag ? static_cast<CGItem_C *>(ClntObjMgrObjectPtr(bag->GetItem(slot), __FILE__, __LINE__)) : 0;
  if (!item) {
    lua_pushnil(L);
    return 1;
  }
  LPCSTR path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
  LPCSTR separator = *path ? "\\" : "";
  char   buffer[MAX_PATH];
  SStrPrintf(buffer, sizeof(buffer), "%s%s%s", path, separator, item->GetInventoryArt());
  lua_pushstring(L, buffer);
  return 1;
}

static int Script_GetInventoryItemCount(lua_State *L) {
  int slot = 0;
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: GetInventoryItemCount(unit, slot)");
    return 0;
  }
  if (!GetSlotFromLua(L, slot, 2)) {
    luaL_error(L, "Invalid inventory slot in GetInventoryItemCount");
    return 0;
  }
  CGUnit_C   *unit = Script_GetUnitFromName(lua_tostring(L, 1));
  CGPlayer_C *player = unit && unit->GetType() & TYPE_PLAYER ? static_cast<CGPlayer_C *>(unit) : 0;
  CGBag_C    *bag = player ? player->CGPlayer_C::GetBag() : 0;
  CGItem_C   *item = bag ? static_cast<CGItem_C *>(ClntObjMgrObjectPtr(bag->GetItem(slot), __FILE__, __LINE__)) : 0;
  int         count = 1;
  if (item && item->GetType() & TYPE_CONTAINER && item->GetClassID() == 11) {
    count = item->GetBag()->GetItemTypeCount(-1, 0) > 0 ? item->GetBag()->GetItemTypeCount(-1, 0) : 0;
  } else if (item) {
    count = item->GetStackCount();
  }
  lua_pushnumber(L, static_cast<double>(count));
  return 1;
}

static int Script_GetInventoryItemQuality(lua_State *L) {
  int slot = 0;
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: GetInventoryItemQuality(unit, slot)");
    return 0;
  }
  if (!GetSlotFromLua(L, slot, 2)) {
    luaL_error(L, "Invalid inventory slot in GetInventoryItemQuality");
    return 0;
  }
  CGUnit_C   *unit = Script_GetUnitFromName(lua_tostring(L, 1));
  CGPlayer_C *player = unit && unit->GetType() & TYPE_PLAYER ? static_cast<CGPlayer_C *>(unit) : 0;
  CGBag_C    *bag = player ? player->CGPlayer_C::GetBag() : 0;
  CGItem_C   *item = bag ? static_cast<CGItem_C *>(ClntObjMgrObjectPtr(bag->GetItem(slot), __FILE__, __LINE__)) : 0;
  if (!item) {
    lua_pushnil(L);
    return 1;
  }
  const ItemStats *stats = g_itemDBCache.GetRecord(item->GetEntryID(), 0, 0, 0);
  lua_pushnumber(L, stats && stats->m_inventoryType ? static_cast<double>(stats->m_overallQualityID) : -1.0);
  return 1;
}

static int Script_GetInventoryItemCooldown(lua_State *L) {
  int slot = 0;
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: GetInventoryItemCooldown(unit, slot)");
    return 0;
  }
  if (!GetSlotFromLua(L, slot, 2)) {
    luaL_error(L, "Invalid inventory slot in GetInventoryItemCooldown");
    return 0;
  }
  CGUnit_C   *unit = Script_GetUnitFromName(lua_tostring(L, 1));
  CGPlayer_C *player = unit && unit->GetType() & TYPE_PLAYER ? static_cast<CGPlayer_C *>(unit) : 0;
  CGBag_C    *bag = player ? player->CGPlayer_C::GetBag() : 0;
  CGItem_C   *item = bag ? static_cast<CGItem_C *>(ClntObjMgrObjectPtr(bag->GetItem(slot), __FILE__, __LINE__)) : 0;
  if (!item) {
    lua_pushnumber(L, 0.0);
    lua_pushnumber(L, 0.0);
    lua_pushnumber(L, 0.0);
    return 3;
  }
  UINT        duration = 0;
  DWORD       startTime = 0;
  Spell_C_GetItemCooldown(item->GetEntryID(), &duration, &startTime, 0);
  lua_pushnumber(L, static_cast<double>(startTime) * 0.001);
  lua_pushnumber(L, static_cast<double>(duration) * 0.001);
  lua_pushnumber(L, 1.0);
  return 3;
}

static int Script_GetInventoryItemLink(lua_State *L) {
  int slot = 0;
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: GetInventoryItemLink(unit, slot)");
    return 0;
  }
  if (!GetSlotFromLua(L, slot, 2)) {
    luaL_error(L, "Invalid inventory slot in GetInventoryItemLink");
    return 0;
  }
  CGUnit_C        *unit = Script_GetUnitFromName(lua_tostring(L, 1));
  CGPlayer_C      *player = unit && unit->GetType() & TYPE_PLAYER ? static_cast<CGPlayer_C *>(unit) : 0;
  CGBag_C         *bag = player ? player->CGPlayer_C::GetBag() : 0;
  CGItem_C        *item = bag ? static_cast<CGItem_C *>(ClntObjMgrObjectPtr(bag->GetItem(slot), __FILE__, __LINE__)) : 0;
  const ItemStats *stats = item ? g_itemDBCache.GetRecord(item->GetEntryID(), 0, 0, 0) : 0;
  if (!stats) {
    return 0;
  }
  char link[1024];
  SStrPrintf(link, sizeof(link), "|Hitem:%d|h[%s]|h", item->GetEntryID(), stats->m_displayName[0]);
  lua_pushstring(L, link);
  return 1;
}

static int Script_PickupInventoryItem(lua_State *L) {
  int slot;
  if (!GetSlotFromLua(L, slot, 1)) {
    luaL_error(L, "Usage: PickupInventoryItem(slot)");
    return 0;
  }
  CGCharacterInfo::PickupItem(slot);
  return 0;
}

static int Script_UseInventoryItem(lua_State *L) {
  int slot;
  if (!GetSlotFromLua(L, slot, 1)) {
    luaL_error(L, "Usage: UseInventoryItem(slot)");
    return 0;
  }
  CGCharacterInfo::UseItem(slot);
  return 0;
}

static int Script_IsInventoryItemLocked(lua_State *L) {
  int slot;
  if (!GetSlotFromLua(L, slot, 1)) {
    luaL_error(L, "Invalid inventory slot in IsInventoryItemLocked");
    return 0;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  CGItem_C *item = player ? static_cast<CGItem_C *>(ClntObjMgrObjectPtr(player->CGPlayer_C::GetBag()->GetItem(slot), __FILE__, __LINE__)) : 0;
  if (item && item->IsLocked()) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_GetSkillLineInfo(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGCharacterInfo::GetNumClassSkills()));
  lua_pushnumber(L, static_cast<double>(CGCharacterInfo::GetNumSpecSkills()));
  lua_pushnumber(L, static_cast<double>(CGCharacterInfo::GetNumRacialSkills()));
  lua_pushnumber(L, static_cast<double>(CGCharacterInfo::GetNumSecondarySkills()));
  lua_pushnumber(L, static_cast<double>(CGCharacterInfo::GetNumProficiencies()));
  return 5;
}

static int Script_GetSkillByIndex(lua_State *L) {
  int offset = 0;
  if (!lua_isstring(L, 1) || !CGCharacterInfo::GetSkillOffsetFromString(lua_tostring(L, 1), offset) || !lua_isnumber(L, 2)) {
    return luaL_error(L, "Invalid skill index in GetSkillByIndex(\"skillType\", index)");
  }
  const SkillInfo *info = CGCharacterInfo::GetSkillInfoByIndex(static_cast<int>(lua_tonumber(L, 2)) + offset);
  if (!info) {
    return luaL_error(L, "Invalid skill index in GetSkillByIndex(\"skillType\", index)");
  }

  if (info->isProf) {
    lua_pushstring(L, info->profName);
    lua_pushnumber(L, static_cast<double>(info->profLevel));
    lua_pushnumber(L, 0.0);
    lua_pushnumber(L, 0.0);
    lua_pushnumber(L, 0.0);
  } else {
    const SkillLineRec *skill = g_skillLineDB.GetRecord(info->skillID);
    if (!skill) {
      return 5;
    }
    lua_pushstring(L, skill->m_displayName_lang[CURRENT_LANGUAGE]);
    lua_pushnumber(L, static_cast<double>(skill->m_minCharLevel));
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (!player) {
      lua_pushnumber(L, 0.0);
      lua_pushnumber(L, 0.0);
      lua_pushnumber(L, 0.0);
      return 5;
    }
    int skillIndex = player->GetSkillIndex(info->skillID);
    lua_pushnumber(L, static_cast<double>(player->GetMirrorSkillRank(skillIndex)));
    lua_pushnumber(L, static_cast<double>(player->GetMirrorSkillModifier(skillIndex)));
    lua_pushnumber(L, static_cast<double>(player->GetMirrorSkillMaxRank(skillIndex)));
  }
  return 5;
}

static int Script_PutItemInBag(lua_State *L) {
  int slot;
  if (!GetSlotFromLua(L, slot, 1) || slot < 19) {
    luaL_error(L, "Invalid bag slot in PutItemInBag");
    return 0;
  }
  if (CGCharacterInfo::PutItemInBag(slot)) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_PutItemInBackpack(lua_State *L) {
  if (CGCharacterInfo::PutItemInBackpack()) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_PickupBagFromSlot(lua_State *L) {
  int slot;
  if (!GetSlotFromLua(L, slot, 1) || slot < 19) {
    luaL_error(L, "Invalid bag slot in PickupBagFromSlot");
    return 0;
  }
  CGCharacterInfo::PickupBag(slot);
  return 0;
}

static int Script_CursorCanGoInSlot(lua_State *L) {
  int slot = 0;
  if (!GetSlotFromLua(L, slot, 1)) {
    luaL_error(L, "Invalid inventory slot in CursorCanGoInSlot");
    return 0;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player && player->ValidateSlot(slot, CGGameUI::GetCursorItem())) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_ShowInventorySellCursor(lua_State *L) {
  if (Spell_C_IsTargeting()) {
    return 0;
  }

  int slot = 0;
  if (!GetSlotFromLua(L, slot, 1)) {
    luaL_error(L, "Invalid inventory slot in ShowInventorySellCursor");
    return 0;
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(player->GetBag()->GetItem(slot), __FILE__, __LINE__));
    if (item && !item->IsLocked()) {
      CursorModelSetSequence(BUY_CURSOR);
    }
  }
  return 0;
}

static int Script_SetInventoryPortaitTexture(lua_State *L) {
  if (lua_type(L, 1) != LUA_TTABLE) {
    return luaL_error(L, "Attempt to find 'this' in non-table object (used '.' instead of ':' ?)");
  }
  lua_rawgeti(L, 1, 0);
  CSimpleTexture *texture = static_cast<CSimpleTexture *>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  FATALASSERT(texture);
  texture->SetTexture(static_cast<HTEXTURE__ *>(0));

  int slot = 0;
  if (!lua_isstring(L, 2)) {
    luaL_error(L, "Usage: SetInventoryPortaitTexture(texure, unit, slot)");
    return 0;
  }
  if (!GetSlotFromLua(L, slot, 3)) {
    luaL_error(L, "Invalid inventory slot in SetInventoryPortaitTexture");
    return 0;
  }
  CGUnit_C   *unit = Script_GetUnitFromName(lua_tostring(L, 1));
  CGPlayer_C *player = unit && unit->GetType() & TYPE_PLAYER ? static_cast<CGPlayer_C *>(unit) : 0;
  CGItem_C   *item = player ? static_cast<CGItem_C *>(ClntObjMgrObjectPtr(player->CGPlayer_C::GetBag()->GetItem(slot), __FILE__, __LINE__)) : 0;
  if (item) {
    LPCSTR path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
    LPCSTR separator = *path ? "\\" : "";
    char   buffer[MAX_PATH];
    SStrPrintf(buffer, sizeof(buffer), "%s%s%s", path, separator, item->GetInventoryArt());
    SetPortraitTexture(texture, buffer);
  }
  return 0;
}

static void GuildNameCallback(int guildID, const DWORDLONG &guid, LPVOID, bool granted) {
  if (granted) {
    Script_SendUnitSignal(guid, 324);
  }
}

static int Script_GetGuildInfo(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: GetGuildInfo(\"unit\")");
    return 0;
  }
  CGUnit_C           *unit = Script_GetUnitFromName(lua_tostring(L, 1));
  CGPlayer_C         *player = unit && unit->GetType() & TYPE_PLAYER ? static_cast<CGPlayer_C *>(unit) : 0;
  const GuildStats_C *guild =
      player && player->GetGuildID() ? g_guildInfoCache.GetRecord(player->GetGuildID(), player->GetGUID(), GuildNameCallback, 0) : 0;
  if (guild) {
    lua_pushstring(L, guild->m_guildName);
    lua_pushnumber(L, static_cast<double>(player->GetGuildRank()));
  } else {
    lua_pushnil(L);
    lua_pushnumber(L, 0.0);
  }
  return 2;
}

static FrameScript_Method s_ScriptFunctions[18] = {
    {      "GetInventorySlotInfo",       Script_GetInventorySlotInfo},
    {   "GetInventoryItemTexture",    Script_GetInventoryItemTexture},
    {     "GetInventoryItemCount",      Script_GetInventoryItemCount},
    {   "GetInventoryItemQuality",    Script_GetInventoryItemQuality},
    {  "GetInventoryItemCooldown",   Script_GetInventoryItemCooldown},
    {      "GetInventoryItemLink",       Script_GetInventoryItemLink},
    {       "PickupInventoryItem",        Script_PickupInventoryItem},
    {          "UseInventoryItem",           Script_UseInventoryItem},
    {     "IsInventoryItemLocked",      Script_IsInventoryItemLocked},
    {          "GetSkillLineInfo",           Script_GetSkillLineInfo},
    {           "GetSkillByIndex",            Script_GetSkillByIndex},
    {              "PutItemInBag",               Script_PutItemInBag},
    {         "PutItemInBackpack",          Script_PutItemInBackpack},
    {         "PickupBagFromSlot",          Script_PickupBagFromSlot},
    {         "CursorCanGoInSlot",          Script_CursorCanGoInSlot},
    {   "ShowInventorySellCursor",    Script_ShowInventorySellCursor},
    {"SetInventoryPortaitTexture", Script_SetInventoryPortaitTexture},
    {              "GetGuildInfo",               Script_GetGuildInfo}
};

void CharacterInfoRegisterScriptFunctions() {
  for (UINT i = 0; i < 18; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void CharacterInfoUnregisterScriptFunctions() {
  for (UINT i = 0; i < 18; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
