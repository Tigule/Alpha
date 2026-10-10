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

int s_qsortLevel;

struct ProficiencyInfo {
  int minLevel;
  int slot;
};

static int __cdecl            QSortCompareByCategoryAndLevel(LPCVOID a, LPCVOID b);
static const CharBaseInfoRec *GetCharBaseInfo(int raceID, int classID);
static const ItemSubClassRec *FindItemSubClassRecord(int classID, int subClassID);
static int __cdecl            QSortCompareProficiency(LPCVOID a, LPCVOID b);

static BOOL PlayerCharacterPointsUpdateHandler(DWORDLONG, UINT, UINT, LPCVOID, LPVOID) {
  FrameScript_SignalEvent(247);
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
    ClntObjMgrSetObjMirrorHandler(
        player, CGPlayer_C::OffsetOf(ID_PLAYER) + offsetof(CGPlayerData, characterPoints), sizeof(((CGPlayerData *)0)->characterPoints),
        PlayerCharacterPointsUpdateHandler, 0, HANDLER_PRIORITY_NORMAL
    );
  }
}

void CGCharacterInfo::RemoveMirrorHandlers(DWORDLONG player) {
  if (player) {
    ClntObjMgrUnsetObjMirrorHandler(player, CGPlayer_C::OffsetOf(ID_PLAYER) + offsetof(CGPlayerData, characterPoints), PlayerCharacterPointsUpdateHandler, 0);
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
  if (!player || (!CGGameUI::HasPlayerControl() && !player->IsOnTaxi())) {
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
    if (itemGUID == cursorItem) {
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

  DWORDLONG targetBag;
  if (slot == 255) {
    targetBag = player->GetGUID();
  } else {
    targetBag = player->CGPlayer_C::GetBag()->GetItem(slot);
  }
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
  const SkillLineRec *skill1 = *(const SkillLineRec *const *)a;
  const SkillLineRec *skill2 = *(const SkillLineRec *const *)b;
  if (skill1->m_skillType == skill2->m_skillType) {
    if (skill1->m_minCharLevel <= s_qsortLevel && skill2->m_minCharLevel <= s_qsortLevel) {
      if (skill1->m_categoryID == skill2->m_categoryID) {
        return SStrCmp(skill1->m_displayName_lang[CURRENT_LANGUAGE], skill2->m_displayName_lang[CURRENT_LANGUAGE], 4);
      }
      return skill1->m_categoryID > skill2->m_categoryID ? 1 : -1;
    }
    if (skill1->m_minCharLevel <= s_qsortLevel) {
      return -1;
    }
    if (skill2->m_minCharLevel <= s_qsortLevel) {
      return 1;
    }
    if (skill1->m_minCharLevel == skill2->m_minCharLevel) {
      return SStrCmp(skill1->m_displayName_lang[CURRENT_LANGUAGE], skill2->m_displayName_lang[CURRENT_LANGUAGE], 4);
    }
    return skill1->m_minCharLevel > skill2->m_minCharLevel ? 1 : -1;
  }
  return skill1->m_skillType > skill2->m_skillType ? 1 : -1;
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
    if (i < 64 && playerPtr->GetMirrorSkillID(i) > 0) {
      const SkillLineRec *rec = g_skillLineDB.GetRecord(playerPtr->GetMirrorSkillID(i));
      skillInfo[count] = rec;
      if (rec && playerPtr->GetMirrorSkillMaxRank(i) > 0 && rec->m_categoryID < 4) {
        if (!rec->m_skillType) {
          ++numClassSkills;
        }
        ++count;
      }
    }
  }
  s_qsortLevel = playerPtr->GetLevel();
  qsort(skillInfo, count, sizeof(SkillLineRec *), QSortCompareByCategoryAndLevel);
  m_profOffset = numClassSkills + 1;
  m_specialOffset = 0;
  m_racialOffset = 0;
  m_secondaryOffset = 0;
  UINT numProfs = OrderProficiencies(m_profOffset + 1);
  if (numProfs) {
    ++numProfs;
  }
  m_skillInfoList[0].isProf = 0;
  m_skillInfoList[0].skillID = 0;
  UINT skillCount = 1;
  for (i = 0; i < count; ++i) {
    if (!m_specialOffset && skillInfo[i]->m_skillType == 1) {
      m_specialOffset = numProfs + skillCount;
      ++skillCount;
    }
    if (!m_racialOffset && skillInfo[i]->m_skillType == 2) {
      m_racialOffset = numProfs + skillCount;
      if (!m_specialOffset) {
        m_specialOffset = numProfs + skillCount;
      }
      ++skillCount;
    }
    if (!m_secondaryOffset && (skillInfo[i]->m_skillType == 3 || skillInfo[i]->m_skillType == 4)) {
      m_secondaryOffset = numProfs + skillCount;
      if (!m_racialOffset) {
        m_racialOffset = numProfs + skillCount;
      }
      if (!m_specialOffset) {
        m_specialOffset = numProfs + skillCount;
      }
      ++skillCount;
    }
    UINT index = (skillInfo[i]->m_skillType ? numProfs : 0) + skillCount;
    m_skillInfoList[index].isProf = 0;
    m_skillInfoList[index].skillID = skillInfo[i]->m_ID;
    ++skillCount;
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
  m_numSkills = skillCount + numProfs;
}

static const CharBaseInfoRec *GetCharBaseInfo(int raceID, int classID) {
  int i;
  for (i = 0; i < g_charBaseInfoDB.GetNumRecords(); ++i) {
    const CharBaseInfoRec *rec = g_charBaseInfoDB.GetRecordByIndex(i);
    if (rec->m_classID == classID && rec->m_raceID == raceID) {
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
  const ProficiencyInfo *info1 = (const ProficiencyInfo *)a;
  const ProficiencyInfo *info2 = (const ProficiencyInfo *)b;
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
      UINT proficiency = CGPlayer_C::GetProficiency(rec->m_classID);
      if (proficiency && (proficiency & (1 << rec->m_subClassID)) &&
          (rec->m_classID != 4 || !(proficiency & (1 << rec->m_postrequisiteProficiency))))
      {
        if (rec->m_verboseName_lang[CURRENT_LANGUAGE] && *rec->m_verboseName_lang[CURRENT_LANGUAGE]) {
          m_skillInfoList[offset + numProfs].isProf = 1;
          m_skillInfoList[offset + numProfs].profLevel = 0;
          SStrCopy(m_skillInfoList[offset + numProfs].profName, rec->m_verboseName_lang[CURRENT_LANGUAGE], sizeof(m_skillInfoList[offset + numProfs].profName));
          ++numProfs;
          FATALASSERT(numProfs < 24);
        }
      }
    }
  }
  CGPlayer_C *playerPtr = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (playerPtr) {
    const CharBaseInfoRec *base = GetCharBaseInfo(playerPtr->GetRace(), playerPtr->GetClass());
    if (base) {
      const ChrProficiencyRec *proficiencyRec = g_chrProficiencyDB.GetRecord(base->m_proficiency);
      if (proficiencyRec) {
        int count = 0;
        int j;
        for (j = 0; j < 16; ++j) {
          if (proficiencyRec->m_proficiency_minLevel[j] > playerPtr->GetLevel() && proficiencyRec->m_proficiency_acquireMethod[i] == 1) {
            orderedSlots[count].minLevel = proficiencyRec->m_proficiency_minLevel[j];
            orderedSlots[count].slot = j;
            ++count;
          }
        }
        qsort(orderedSlots, count, sizeof(ProficiencyInfo), QSortCompareProficiency);
        for (i = 0; i < count; ++i) {
          UINT proficiency = CGPlayer_C::GetProficiency(proficiencyRec->m_proficiency_itemClass[orderedSlots[i].slot]);
          UINT bit;
          for (bit = 0; bit < 32; ++bit) {
            if ((proficiencyRec->m_proficiency_itemSubClassMask[orderedSlots[i].slot] & (1 << bit)) && !(proficiency & (1 << bit))) {
              const ItemSubClassRec *rec = FindItemSubClassRecord(proficiencyRec->m_proficiency_itemClass[orderedSlots[i].slot], bit);
              if (rec && *rec->m_verboseName_lang[CURRENT_LANGUAGE]) {
                m_skillInfoList[offset + numProfs].isProf = 1;
                m_skillInfoList[offset + numProfs].profLevel = proficiencyRec->m_proficiency_minLevel[orderedSlots[i].slot];
                SStrCopy(
                    m_skillInfoList[offset + numProfs].profName, rec->m_verboseName_lang[CURRENT_LANGUAGE],
                    sizeof(m_skillInfoList[offset + numProfs].profName)
                );
                ++numProfs;
              }
            }
            FATALASSERT(numProfs < 24);
          }
        }
      }
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
  if (index >= 0 && (UINT)index < 93) {
    return &m_skillInfoList[index];
  }
  return 0;
}

static BOOL GetSlotFromLua(lua_State *L, int &slot, int index) {
  if (lua_isnumber(L, index)) {
    int value = (int)lua_tonumber(L, index) - 1;
    if (value >= 0 && value <= 22 || value >= 39 && value <= 62 || value >= 63 && value <= 68) {
      slot = value;
      return 1;
    }
  }
  return 0;
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
  CGUnit_C *unit = Script_GetUnitFromName(lua_tostring(L, 1));
  if (unit && unit->IsA(ID_PLAYER)) {
    CGItem_C *item =
        static_cast<CGItem_C *>(ClntObjMgrObjectPtr(((CGPlayer_C *)unit)->CGPlayer_C::GetBag()->GetItem(slot), __FILE__, __LINE__));
    if (item) {
      LPCSTR path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
      LPCSTR separator = *path ? "\\" : "";
      char   buffer[MAX_PATH];
      SStrPrintf(buffer, sizeof(buffer), "%s%s%s", path, separator, item->GetInventoryArt());
      lua_pushstring(L, buffer);
      return 1;
    }
  }
  lua_pushnil(L);
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
  CGUnit_C *unit = Script_GetUnitFromName(lua_tostring(L, 1));
  if (unit && unit->IsA(ID_PLAYER)) {
    CGItem_C *item =
        static_cast<CGItem_C *>(ClntObjMgrObjectPtr(((CGPlayer_C *)unit)->CGPlayer_C::GetBag()->GetItem(slot), __FILE__, __LINE__));
    if (item) {
      if (item->IsA(ID_CONTAINER) && item->GetClassID() == 11) {
        lua_pushnumber(L, max(item->GetBag()->GetItemTypeCount(-1, 0), 0));
        return 1;
      }
      lua_pushnumber(L, item->GetStackCount());
      return 1;
    }
  }
  lua_pushnumber(L, 1.0);
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
  CGUnit_C *unit = Script_GetUnitFromName(lua_tostring(L, 1));
  if (unit && unit->IsA(ID_PLAYER)) {
    CGItem_C *item =
        static_cast<CGItem_C *>(ClntObjMgrObjectPtr(((CGPlayer_C *)unit)->CGPlayer_C::GetBag()->GetItem(slot), __FILE__, __LINE__));
    if (item) {
      const ItemStats *stats = g_itemDBCache.GetRecord(item->GetEntryID(), 0, 0, 0);
      lua_pushnumber(L, stats && stats->m_inventoryType ? stats->m_overallQualityID : -1);
      return 1;
    }
  }
  lua_pushnil(L);
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
  CGUnit_C *unit = Script_GetUnitFromName(lua_tostring(L, 1));
  if (unit && unit->IsA(ID_PLAYER)) {
    CGItem_C *item =
        static_cast<CGItem_C *>(ClntObjMgrObjectPtr(((CGPlayer_C *)unit)->CGPlayer_C::GetBag()->GetItem(slot), __FILE__, __LINE__));
    if (item) {
      DWORD startTime = 0;
      UINT  duration = 0;
      Spell_C_GetItemCooldown(item->GetEntryID(), &duration, &startTime, 0);
      lua_pushnumber(L, (double)startTime * 0.001);
      lua_pushnumber(L, (double)duration * 0.001);
      lua_pushnumber(L, 1.0);
      return 3;
    }
  }
  lua_pushnumber(L, 0.0);
  lua_pushnumber(L, 0.0);
  lua_pushnumber(L, 0.0);
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
  CGUnit_C *unit = Script_GetUnitFromName(lua_tostring(L, 1));
  if (unit && unit->IsA(ID_PLAYER)) {
    CGItem_C *item =
        static_cast<CGItem_C *>(ClntObjMgrObjectPtr(((CGPlayer_C *)unit)->CGPlayer_C::GetBag()->GetItem(slot), __FILE__, __LINE__));
    if (item) {
      const ItemStats *stats = g_itemDBCache.GetRecord(item->GetEntryID(), 0, 0, 0);
      if (stats) {
        char link[1024];
        SStrPrintf(link, sizeof(link), "|Hitem:%d|h[%s]|h", item->GetEntryID(), stats->m_displayName[0]);
        lua_pushstring(L, link);
        return 1;
      }
    }
  }
  return 0;
}

static int Script_PickupInventoryItem(lua_State *L) {
  int slot = 0;
  if (!GetSlotFromLua(L, slot, 1)) {
    luaL_error(L, "Invalid inventory slot in PickupInventoryItem");
    return 0;
  }
  CGCharacterInfo::PickupItem(slot);
  return 0;
}

static int Script_UseInventoryItem(lua_State *L) {
  int slot = 0;
  if (!GetSlotFromLua(L, slot, 1)) {
    luaL_error(L, "Invalid inventory slot in UseInventoryItem");
    return 0;
  }
  CGCharacterInfo::UseItem(slot);
  return 0;
}

static int Script_IsInventoryItemLocked(lua_State *L) {
  int slot = 0;
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
  lua_pushnumber(L, CGCharacterInfo::GetNumClassSkills());
  lua_pushnumber(L, CGCharacterInfo::GetNumSpecSkills());
  lua_pushnumber(L, CGCharacterInfo::GetNumRacialSkills());
  lua_pushnumber(L, CGCharacterInfo::GetNumSecondarySkills());
  lua_pushnumber(L, CGCharacterInfo::GetNumProficiencies());
  return 5;
}

static int Script_GetSkillByIndex(lua_State *L) {
  if (lua_isstring(L, 1)) {
    int offset = 0;
    if (CGCharacterInfo::GetSkillOffsetFromString(lua_tostring(L, 1), offset) && lua_isnumber(L, 2)) {
      offset += (int)lua_tonumber(L, 2);
      const SkillInfo *info = CGCharacterInfo::GetSkillInfoByIndex(offset);
      if (info) {
        if (info->isProf) {
          lua_pushstring(L, info->profName);
          lua_pushnumber(L, info->profLevel);
          lua_pushnumber(L, 0.0);
          lua_pushnumber(L, 0.0);
          lua_pushnumber(L, 0.0);
        } else {
          const SkillLineRec *skill = g_skillLineDB.GetRecord(info->skillID);
          if (skill) {
            lua_pushstring(L, skill->m_displayName_lang[CURRENT_LANGUAGE]);
            lua_pushnumber(L, skill->m_minCharLevel);
            CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
            if (player) {
              int skillIndex = player->GetSkillIndex(info->skillID);
              lua_pushnumber(L, player->GetMirrorSkillRank(skillIndex));
              lua_pushnumber(L, player->GetMirrorSkillModifier(skillIndex));
              lua_pushnumber(L, player->GetMirrorSkillMaxRank(skillIndex));
            } else {
              lua_pushnumber(L, 0.0);
              lua_pushnumber(L, 0.0);
              lua_pushnumber(L, 0.0);
            }
          }
        }
        return 5;
      }
    }
  }
  luaL_error(L, "Invalid skill index in GetSkillByIndex(\"skillType\", index)");
  return 0;
}

static int Script_PutItemInBag(lua_State *L) {
  int slot = 0;
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
  int slot = 0;
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
  CSimpleTexture *texture = (CSimpleTexture *)FrameScript_GetObjectThis(L);
  texture->SetTexture(0);

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
  CGPlayer_C *player = unit && unit->IsA(ID_PLAYER) ? (CGPlayer_C *)unit : 0;
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
  CGUnit_C *unit = Script_GetUnitFromName(lua_tostring(L, 1));
  if (unit && unit->IsA(ID_PLAYER)) {
    CGPlayer_C *player = (CGPlayer_C *)unit;
    int         guildID = player->GetGuildID();
    if (guildID) {
      const GuildStats_C *guild = g_guildInfoCache.GetRecord(guildID, player->GetGUID(), GuildNameCallback, 0);
      if (guild) {
        lua_pushstring(L, guild->m_guildName);
        lua_pushnumber(L, player->GetGuildRank());
        return 2;
      }
    }
  }
  lua_pushnil(L);
  lua_pushnumber(L, 0.0);
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
