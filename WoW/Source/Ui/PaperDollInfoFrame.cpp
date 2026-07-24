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
#include "UIUtil/Cursor.h"

#include <Frame/CSimpleRender.h>
#include <FrameScript/FrameScript.h>
#include <lauxlib.h>
#include <lua.h>
#include <stdlib.h>
#include <string.h>

int __fastcall       Spell_C_GetItemCooldown(int itemID, unsigned int *duration, unsigned long *startTime, unsigned int *enable);
void __fastcall      CursorModelSetSequence(CURSORANIMATIONS sequence);
CGUnit_C *__fastcall Script_GetUnitFromName(const char *name);

SkillInfo             CGCharacterInfo::m_skillInfoList[93];
unsigned int          CGCharacterInfo::m_profOffset;
unsigned int          CGCharacterInfo::m_specialOffset;
unsigned int          CGCharacterInfo::m_racialOffset;
unsigned int          CGCharacterInfo::m_secondaryOffset;
unsigned int          CGCharacterInfo::m_numSkills;
CGCharacterModelBase *CGCharacterInfo::m_paperDoll;

static unsigned int s_playerLevel;

struct ProficiencyInfo {
  int level;
  int index;
};

static int __cdecl            QSortCompareByCategoryAndLevel(const void *a, const void *b);
static const CharBaseInfoRec *GetCharBaseInfo(int raceID, int classID);
static const ItemSubClassRec *FindItemSubClassRecord(int classID, int subClassID);
static int __cdecl            QSortCompareProficiency(const void *a, const void *b);

static int __fastcall PlayerCharacterPointsUpdateHandler(unsigned __int64, unsigned int, unsigned int, const void *, void *) {
  CGCharacterInfo::UpdateAllSkillLines();
  return 1;
}

void __fastcall CGCharacterInfo::InitializeGame() {
  for (unsigned int i = 0; i < 93; ++i) {
    m_skillInfoList[i].isProf = 0;
    m_skillInfoList[i].skillID = 0;
  }
}

void __fastcall CGCharacterInfo::ShutdownGame() {
}

void __fastcall CGCharacterInfo::EnterWorld() {
  InstallMirrorHandlers(ClntObjMgrGetActivePlayer());
  FrameScript_SignalEvent(180, "%s", "player");
  FrameScript_SignalEvent(181, "%s", "player");
  FrameScript_SignalEvent(183, "%s", "player");
  FrameScript_SignalEvent(326, "%s", "player");
  UpdateAllSkillLines();
}

void __fastcall CGCharacterInfo::LeaveWorld() {
  RemoveMirrorHandlers(ClntObjMgrGetActivePlayer());
}

void __fastcall CGCharacterInfo::InstallMirrorHandlers(unsigned __int64 player) {
  if (player) {
    unsigned int playerOffset = CGPlayer_C::OffsetOf(ID_PLAYER);
    ClntObjMgrSetObjMirrorHandler(player, playerOffset + 1756, 8, PlayerCharacterPointsUpdateHandler, 0, HANDLER_PRIORITY_NORMAL);
  }
}

void __fastcall CGCharacterInfo::RemoveMirrorHandlers(unsigned __int64 player) {
  if (player) {
    unsigned int playerOffset = CGPlayer_C::OffsetOf(ID_PLAYER);
    ClntObjMgrUnsetObjMirrorHandler(player, playerOffset + 1756, PlayerCharacterPointsUpdateHandler, 0);
  }
}

void __fastcall CGCharacterInfo::UpdateItem(unsigned __int64 item) {
  CGItem_C *itemPtr = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(item, __FILE__, __LINE__));
  if (itemPtr) {
    if (itemPtr->GetOwner() == ClntObjMgrGetActivePlayer()) {
      FrameScript_SignalEvent(181, "%s", "player");
    }
  }
}

void __fastcall CGCharacterInfo::PickupItem(int slot) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  CGBag_C    *bag = player ? player->GetBag() : 0;
  if (!bag || slot < 0 || slot >= 23) {
    return;
  }
  unsigned __int64 item = bag->GetItem(slot);
  unsigned __int64 cursorItem;
  unsigned __int64 cursorBag;
  unsigned int     cursorSlot;
  CGGameUI::GetCursorItem(cursorItem, cursorBag, cursorSlot);
  if (cursorItem) {
    if (cursorItem == item) {
      CGGameUI::ClearCursor(1);
      return;
    }

    CGItem_C *slotItem = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(item, __FILE__, __LINE__));
    if (slotItem && !slotItem->IsUnlocked()) {
      return;
    }

    player->SwapItems(cursorItem, cursorBag, cursorSlot, player->GetGUID(), slot, 0);
    CGGameUI::LockItem(item);
  } else if (item) {
    CGGameUI::SetCursorItem(item, player->GetGUID(), slot, 1, 0);
    CGGameUI::LockItem(item);
  }
}

void __fastcall CGCharacterInfo::UseItem(int slot) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  CGBag_C    *bag = player ? player->GetBag() : 0;
  CGItem_C   *item = bag && slot >= 0 && slot < 19 ? static_cast<CGItem_C *>(ClntObjMgrObjectPtr(bag->GetItem(slot), __FILE__, __LINE__)) : 0;
  if (item && item->IsUnlocked()) {
    item->Use();
  }
}

void __fastcall CGCharacterInfo::PickupBag(int slot) {
  PickupItem(slot + 19);
}

int __fastcall CGCharacterInfo::PutItemInBag(int slot) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player || (slot != 255 && (slot < 0 || slot >= 4))) {
    return 0;
  }
  unsigned __int64 item;
  unsigned __int64 bag;
  unsigned int     itemSlot;
  CGGameUI::GetCursorItem(item, bag, itemSlot);
  unsigned __int64 targetBag = slot == 255 ? player->GetGUID() : player->GetBag()->GetItem(slot + 19);
  if (item && targetBag) {
    unsigned int split = CGGameUI::GetCursorStackSplit();
    if (split) {
      player->SplitItem(item, bag, itemSlot, targetBag, 255, split);
    } else {
      player->AutoStoreItemInBag(item, bag, itemSlot, targetBag, 0);
    }
    CGGameUI::ClearCursor(0);
    return 1;
  }
  return 0;
}

int __fastcall CGCharacterInfo::PutItemInBackpack() {
  return PutItemInBag(255);
}

void __fastcall CGCharacterInfo::UpdateAllSkillLines() {
  OrderSkillLines();
  FrameScript_SignalEvent(248);
}

static int __cdecl QSortCompareByCategoryAndLevel(const void *a, const void *b) {
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
      return SStrCmpI(skill1->m_displayName_lang[CURRENT_LANGUAGE], skill2->m_displayName_lang[CURRENT_LANGUAGE], 4);
    }
    return -1;
  }
  if (skill2->m_minCharLevel <= static_cast<int>(s_playerLevel)) {
    return 1;
  }
  if (skill1->m_minCharLevel != skill2->m_minCharLevel) {
    return skill1->m_minCharLevel > skill2->m_minCharLevel ? 1 : -1;
  }
  return SStrCmpI(skill1->m_displayName_lang[CURRENT_LANGUAGE], skill2->m_displayName_lang[CURRENT_LANGUAGE], 4);
}

void __fastcall CGCharacterInfo::OrderSkillLines() {
  SkillLineRec *skillInfo[64];
  unsigned int  count = 0;
  CGPlayer_C   *playerPtr = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!playerPtr) {
    return;
  }
  unsigned int   i;
  unsigned int   numClassSkills = 0;
  for (i = 0; i < 69; ++i) {
    m_skillInfoList[i].isProf = 0;
    m_skillInfoList[i].skillID = 0;
    if (i < 64) {
      unsigned int skillID = playerPtr->GetMirrorSkillID(i);
      if (skillID) {
        SkillLineRec *rec = g_skillLineDB.GetRecord(skillID);
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
  unsigned int numProfs = OrderProficiencies(numClassSkills + 2);
  unsigned int profSpan = numProfs ? numProfs + 1 : 0;
  m_skillInfoList[0].isProf = 0;
  m_skillInfoList[0].skillID = 0;
  unsigned int skillCount = 1;
  unsigned int output = profSpan + 1;
  for (i = 0; i < count; ++i) {
    SkillLineRec *rec = skillInfo[i];
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
    unsigned int index = skillCount + (rec->m_skillType ? profSpan : 0);
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
  m_numSkills = profSpan + skillCount;
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

static int __cdecl QSortCompareProficiency(const void *a, const void *b) {
  FATALASSERT(a);
  FATALASSERT(b);
  const ProficiencyInfo *info1 = static_cast<const ProficiencyInfo *>(a);
  const ProficiencyInfo *info2 = static_cast<const ProficiencyInfo *>(b);
  if (info1->level == info2->level) {
    return 0;
  }
  return info1->level > info2->level ? 1 : -1;
}

unsigned int __fastcall CGCharacterInfo::OrderProficiencies(unsigned int offset) {
  ProficiencyInfo orderedSlots[16];
  unsigned int    numProfs = 0;
  int             i;
  for (i = 0; i < g_itemSubClassDB.GetNumRecords(); ++i) {
    const ItemSubClassRec *rec = g_itemSubClassDB.GetRecordByIndex(i);
    if (rec->m_classID < 16 && rec->m_displayName_lang[CURRENT_LANGUAGE]) {
      unsigned int proficiency = CGPlayer_C::GetProficiency(static_cast<unsigned char>(rec->m_classID));
      unsigned int bit = 1 << rec->m_subClassID;
      if ((proficiency & bit) && (rec->m_classID != 4 || !(proficiency & (1 << rec->m_postrequisiteProficiency)))) {
        FATALASSERT(numProfs < 24);
        const char *name = rec->m_verboseName_lang[CURRENT_LANGUAGE];
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
  unsigned int orderedCount = 0;
  for (i = 0; i < 16; ++i) {
    if (proficiencyRec->m_proficiency_minLevel[i] > playerPtr->GetUnitData()->level && proficiencyRec->m_proficiency_acquireMethod[i] == 1) {
      orderedSlots[orderedCount].level = proficiencyRec->m_proficiency_minLevel[i];
      orderedSlots[orderedCount].index = i;
      ++orderedCount;
    }
  }
  qsort(orderedSlots, orderedCount, sizeof(ProficiencyInfo), QSortCompareProficiency);
  for (i = 0; i < static_cast<int>(orderedCount); ++i) {
    int          slot = orderedSlots[i].index;
    int          itemClass = proficiencyRec->m_proficiency_itemClass[slot];
    unsigned int proficiency = CGPlayer_C::GetProficiency(static_cast<unsigned char>(itemClass));
    unsigned int bit;
    for (bit = 0; bit < 32; ++bit) {
      if ((proficiencyRec->m_proficiency_itemSubClassMask[slot] & (1 << bit)) && !(proficiency & (1 << bit))) {
        const ItemSubClassRec *rec = FindItemSubClassRecord(itemClass, bit);
        if (rec) {
          const char *name = rec->m_verboseName_lang[CURRENT_LANGUAGE];
          if (name && *name) {
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

int __fastcall CGCharacterInfo::GetSkillOffsetFromString(const char *string, int &offset) {
  if (!SStrCmpI(string, "class", 0x7FFFFFFF)) {
    offset = 1;
    return GetNumClassSkills();
  }
  if (!SStrCmpI(string, "professions", 0x7FFFFFFF)) {
    offset = m_profOffset;
    return GetNumProficiencies();
  }
  if (!SStrCmpI(string, "secondary", 0x7FFFFFFF)) {
    offset = m_secondaryOffset;
    return GetNumSecondarySkills();
  }
  if (!SStrCmpI(string, "racial", 0x7FFFFFFF)) {
    offset = m_racialOffset;
    return GetNumRacialSkills();
  }
  if (!SStrCmpI(string, "special", 0x7FFFFFFF)) {
    offset = m_specialOffset;
    return GetNumSpecSkills();
  }
  offset = 0;
  return 0;
}

SkillInfo *__fastcall CGCharacterInfo::GetSkillInfoByIndex(int index) {
  return index >= 0 && static_cast<unsigned int>(index) < m_numSkills ? &m_skillInfoList[index] : 0;
}

static int __fastcall GetSlotFromLua(lua_State *L, int &slot, int index) {
  if (!lua_isnumber(L, index)) {
    return 0;
  }
  slot = static_cast<int>(lua_tonumber(L, index)) - 1;
  return slot >= 0 && slot <= 22 || slot >= 39 && slot <= 62 || slot >= 63 && slot <= 68;
}

static int __fastcall Script_GetInventorySlotInfo(lua_State *L) {
  const char *string;
  int         numEntries;
  if (lua_isstring(L, 1)) {
    string = lua_tostring(L, 1);
    numEntries = g_paperDollItemFrameDB.GetNumRecords();
    for (int i = 0; i < numEntries; ++i) {
      PaperDollItemFrameRec *record = g_paperDollItemFrameDB.GetRecordByIndex(i);
      if (record && !SStrCmpI(record->m_ItemButtonName, string, 0x7FFFFFFF)) {
        lua_pushnumber(L, record->m_SlotNumber);
        lua_pushstring(L, record->m_SlotIcon);
        return 2;
      }
    }
  }
  return luaL_error(L, "Invalid inventory slot in GetInventorySlotInfo");
}

static int __fastcall Script_GetInventoryItemTexture(lua_State *L) {
  int slot;
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: GetInventoryItemTexture(unit, slot)");
  }
  if (!GetSlotFromLua(L, slot, 2)) {
    return luaL_error(L, "Invalid inventory slot in GetInventoryItemTexture");
  }
  CGUnit_C   *unit = Script_GetUnitFromName(lua_tostring(L, 1));
  CGPlayer_C *player = unit && unit->GetType() & TYPE_PLAYER ? static_cast<CGPlayer_C *>(unit) : 0;
  CGBag_C    *bag = player ? player->GetBag() : 0;
  CGItem_C   *item = bag ? static_cast<CGItem_C *>(ClntObjMgrObjectPtr(bag->GetItem(slot), __FILE__, __LINE__)) : 0;
  if (!item) {
    lua_pushnil(L);
    return 1;
  }
  const char *path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
  const char *separator = path && *path ? "\\" : "";
  char        buffer[260];
  SStrPrintf(buffer, sizeof(buffer), "%s%s%s", path, separator, item->GetInventoryArt());
  lua_pushstring(L, buffer);
  return 1;
}

static int __fastcall Script_GetInventoryItemCount(lua_State *L) {
  int slot;
  if (!GetSlotFromLua(L, slot, 2)) {
    return luaL_error(L, "Usage: GetInventoryItemCount(unit, slot)");
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  CGBag_C    *bag = player ? player->GetBag() : 0;
  CGItem_C   *item = bag ? static_cast<CGItem_C *>(ClntObjMgrObjectPtr(bag->GetItem(slot), __FILE__, __LINE__)) : 0;
  lua_pushnumber(L, item ? static_cast<double>(item->GetStackCount()) : 0.0);
  return 1;
}

static int __fastcall Script_GetInventoryItemQuality(lua_State *L) {
  int slot;
  if (!GetSlotFromLua(L, slot, 2)) {
    return luaL_error(L, "Usage: GetInventoryItemQuality(unit, slot)");
  }
  CGPlayer_C      *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  CGBag_C         *bag = player ? player->GetBag() : 0;
  CGItem_C        *item = bag ? static_cast<CGItem_C *>(ClntObjMgrObjectPtr(bag->GetItem(slot), __FILE__, __LINE__)) : 0;
  unsigned __int64 noGuid = 0;
  const ItemStats *stats = item ? g_itemDBCache.GetRecord(item->GetEntryID(), noGuid, 0, 0) : 0;
  lua_pushnumber(L, stats ? static_cast<double>(stats->m_overallQualityID) : -1.0);
  return 1;
}

static int __fastcall Script_GetInventoryItemCooldown(lua_State *L) {
  int slot;
  if (!GetSlotFromLua(L, slot, 2)) {
    return luaL_error(L, "Usage: GetInventoryItemCooldown(unit, slot)");
  }
  CGPlayer_C   *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  CGBag_C      *bag = player ? player->GetBag() : 0;
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

static int __fastcall Script_GetInventoryItemLink(lua_State *L) {
  int slot;
  if (!GetSlotFromLua(L, slot, 2)) {
    return luaL_error(L, "Usage: GetInventoryItemLink(unit, slot)");
  }
  CGPlayer_C      *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  CGBag_C         *bag = player ? player->GetBag() : 0;
  CGItem_C        *item = bag ? static_cast<CGItem_C *>(ClntObjMgrObjectPtr(bag->GetItem(slot), __FILE__, __LINE__)) : 0;
  unsigned __int64 noGuid = 0;
  const ItemStats *stats = item ? g_itemDBCache.GetRecord(item->GetEntryID(), noGuid, 0, 0) : 0;
  if (!stats) {
    return 0;
  }
  char link[1024];
  SStrPrintf(link, sizeof(link), "|Hitem:%d|h[%s]|h", item->GetEntryID(), stats->m_displayName[CURRENT_LANGUAGE]);
  lua_pushstring(L, link);
  return 1;
}

static int __fastcall Script_PickupInventoryItem(lua_State *L) {
  int slot;
  if (!GetSlotFromLua(L, slot, 1)) {
    return luaL_error(L, "Usage: PickupInventoryItem(slot)");
  }
  CGCharacterInfo::PickupItem(slot);
  return 0;
}

static int __fastcall Script_UseInventoryItem(lua_State *L) {
  int slot;
  if (!GetSlotFromLua(L, slot, 1)) {
    return luaL_error(L, "Usage: UseInventoryItem(slot)");
  }
  CGCharacterInfo::UseItem(slot);
  return 0;
}

static int __fastcall Script_IsInventoryItemLocked(lua_State *L) {
  int slot;
  if (!GetSlotFromLua(L, slot, 1)) {
    return luaL_error(L, "Invalid inventory slot in IsInventoryItemLocked");
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  CGBag_C    *bag = player ? player->GetBag() : 0;
  CGItem_C   *item = bag ? static_cast<CGItem_C *>(ClntObjMgrObjectPtr(bag->GetItem(slot), __FILE__, __LINE__)) : 0;
  item && !item->IsUnlocked() ? lua_pushnumber(L, 1.0) : lua_pushnil(L);
  return 1;
}

static int __fastcall Script_GetSkillLineInfo(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGCharacterInfo::GetNumClassSkills()));
  lua_pushnumber(L, static_cast<double>(CGCharacterInfo::GetNumSpecSkills()));
  lua_pushnumber(L, static_cast<double>(CGCharacterInfo::GetNumRacialSkills()));
  lua_pushnumber(L, static_cast<double>(CGCharacterInfo::GetNumSecondarySkills()));
  lua_pushnumber(L, static_cast<double>(CGCharacterInfo::GetNumProficiencies()));
  return 5;
}

static int __fastcall Script_GetSkillByIndex(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetSkillByIndex(index)");
  }
  SkillInfo *info = CGCharacterInfo::GetSkillInfoByIndex(static_cast<int>(lua_tonumber(L, 1)) - 1);
  if (!info) {
    return 0;
  }
  if (info->isProf) {
    lua_pushstring(L, info->profName);
    lua_pushnumber(L, static_cast<double>(info->profLevel));
    lua_pushnil(L);
    lua_pushnil(L);
  } else {
    SkillLineRec *skill = g_skillLineDB.GetRecord(info->skillID);
    skill ? lua_pushstring(L, skill->m_displayName_lang[CURRENT_LANGUAGE]) : lua_pushnil(L);
    lua_pushnumber(L, 0.0);
    lua_pushnumber(L, 0.0);
    lua_pushnumber(L, 0.0);
  }
  return 4;
}

static int __fastcall Script_PutItemInBag(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: PutItemInBag(slot)");
  }
  lua_pushnumber(L, static_cast<double>(CGCharacterInfo::PutItemInBag(static_cast<int>(lua_tonumber(L, 1)) - 1)));
  return 1;
}

static int __fastcall Script_PutItemInBackpack(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGCharacterInfo::PutItemInBackpack()));
  return 1;
}

static int __fastcall Script_PickupBagFromSlot(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: PickupBagFromSlot(slot)");
  }
  CGCharacterInfo::PickupBag(static_cast<int>(lua_tonumber(L, 1)) - 1);
  return 0;
}

static int __fastcall Script_CursorCanGoInSlot(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: CursorCanGoInSlot(slot)");
  }
  CGGameUI::GetCursorItem() ? lua_pushnumber(L, 1.0) : lua_pushnil(L);
  return 1;
}

static int __fastcall Script_ShowInventorySellCursor(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: ShowInventorySellCursor(slot)");
  }
  CursorModelSetSequence(BUY_CURSOR);
  return 0;
}

static int __fastcall Script_SetInventoryPortaitTexture(lua_State *L) {
  if (lua_type(L, 1) != LUA_TTABLE) {
    return luaL_error(L, "Attempt to find 'this' in non-table object (used '.' instead of ':' ?)");
  }
  lua_rawgeti(L, 1, 0);
  CSimpleTexture *texture = static_cast<CSimpleTexture *>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  FATALASSERT(texture);
  int slot;
  if (!GetSlotFromLua(L, slot, 2)) {
    return luaL_error(L, "Usage: SetInventoryPortraitTexture(texture, slot)");
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  CGBag_C    *bag = player ? player->GetBag() : 0;
  CGItem_C   *item = bag ? static_cast<CGItem_C *>(ClntObjMgrObjectPtr(bag->GetItem(slot), __FILE__, __LINE__)) : 0;
  if (item) {
    const char *path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
    const char *separator = path && *path ? "\\" : "";
    char        buffer[260];
    SStrPrintf(buffer, sizeof(buffer), "%s%s%s.blp", path, separator, item->GetInventoryArt());
    texture->SetTexture(buffer, 0);
  }
  return 0;
}

void __fastcall GuildNameCallback(int, const unsigned __int64 &, void *, bool granted) {
  if (granted) {
    FrameScript_SignalEvent(183, "%s", "player");
  }
}

static int __fastcall Script_GetGuildInfo(lua_State *L) {
  CGPlayer_C         *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  unsigned __int64    noGuid = 0;
  const GuildStats_C *guild = player && player->GetGuildID() ? g_guildInfoCache.GetRecord(player->GetGuildID(), noGuid, GuildNameCallback, 0) : 0;
  guild ? lua_pushstring(L, guild->m_guildName) : lua_pushnil(L);
  lua_pushnil(L);
  lua_pushnumber(L, player ? static_cast<double>(player->GetGuildRank()) : 0.0);
  return 3;
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

void __fastcall CharacterInfoRegisterScriptFunctions() {
  for (unsigned int i = 0; i < 18; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void __fastcall CharacterInfoUnregisterScriptFunctions() {
  for (unsigned int i = 0; i < 18; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
