#include <Base/Base.h>
#include <Gx/Gx.h>
#include <MapDefs.h>
#include "WorldClient/World.h"
#include <WowConst.h>
#include "Ui/LootFrame.h"
#include "Ui/PartyFrame.h"
#include "Net/NetClient/NetClient.h"

#include "Object/ObjectClient/Item_C.h"
#include "Spell_C.h"
#include "Object/ObjectClient/GameObject_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "Object/ItemStats.h"
#include "Console/ConsoleClient.h"
#include "DB/DBClient/AutoCode/SpellRec.h"
#include "DB/DBClient/AutoCode/SpellCastTimesRec.h"
#include "DB/DBClient/AutoCode/SpellRangeRec.h"
#include "DB/DBClient/AutoCode/SpellRadiusRec.h"
#include "DB/DBClient/AutoCode/SpellFocusObjectRec.h"
#include "DB/DBClient/AutoCode/SpellShapeshiftFormRec.h"
#include "DB/DBClient/AutoCode/ItemSubClassRec.h"
#include "DB/DBClient/AutoCode/GameObjectDisplayInfoRec.h"
#include "DB/DBClient/AutoCode/SkillLineRec.h"
#include "DB/DBClient/AutoCode/SkillLineAbilityRec.h"
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "Ui/ActionBarFrame.h"
#include "Ui/ContainerFrame.h"
#include "Ui/GameUI.h"
#include "Ui/PetInfo.h"
#include "Ui/SpellBookFrame.h"
#include "Ui/TradeFrame.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"
#include "SoundInterface/SoundInterface.h"

#include <Base/CDataStore.h>
#include <FrameScript/FrameScript.h>

#include <lauxlib.h>
#include <lua.h>
#include <Os/OsTime.h>
#include <stpl.h>

#include <math.h>
#include <malloc.h>

enum CURSORANIMATIONS {
  POINT_CURSOR = 0,
  CAST_CURSOR = 1,
  CAST_ERROR_CURSOR = 10
};

struct TradeSkillInfo;
struct TradeSkillSubClassInfo;
struct CraftInfo;
struct CraftSkillLineInfo;

class CGCraftInfo {
 public:
  static void               EnterWorld();
  static void               ShutdownGame();
  static void               Close();
  static void               SetSelection(int index);
  static int                GetSelectionIndex();
  static SPELL_CAST_UI_TYPE GetCraftType() {
    return m_craftType;
  }
  static int GetNumCrafts() {
    return m_filteredSkills;
  }
  static const CraftInfo *GetCraftInfo(UINT index) {
    return index < m_numSkills ? m_skills[index] : 0;
  }
  static UINT GetNumSkillLines() {
    return m_numSkillLines;
  }
  static CraftSkillLineInfo *GetSkillLine(UINT index) {
    return index < m_numSkillLines ? m_skillLines[index] : 0;
  }
  static int  GetSkillLineIndexFromCraft(UINT index);
  static void SetCraftType(SPELL_CAST_UI_TYPE type);
  static void RefreshList();
  static BOOL IsCollpasedHeader(UINT index);
  static int  GetCollapseFilter() {
    return m_collapseFilter;
  }
  static void SetCollapseFilter(int filter);

 private:
  friend int __cdecl QSortSkills(LPCVOID a, LPCVOID b);
  friend int __cdecl QSortPetSkills(LPCVOID a, LPCVOID b);
  friend int __cdecl QSortSkillLines(LPCVOID a, LPCVOID b);

 protected:
  static void FilterAndSortSkills();

 private:
  static SPELL_CAST_UI_TYPE                    m_craftType;
  static int                                   m_currentSelection;
  static UINT                                  m_numSkills;
  static UINT                                  m_numSkillLines;
  static UINT                                  m_filteredSkills;
  static int                                   m_collapseFilter;
  static TSGrowableArray<CraftInfo *>          m_skills;
  static TSGrowableArray<CraftSkillLineInfo *> m_skillLines;
};

class CGTradeSkillInfo {
  friend int __cdecl QSortSkills(LPCVOID a, LPCVOID b);
  friend int __cdecl QSortSubClasses(LPCVOID a, LPCVOID b);

 public:
  static void EnterWorld();
  static void LeaveWorld();
  static void ShutdownGame();
  static void Close();
  static void ClearItemCallbacks();
  static void DecrementPendingItem() {
    if (!m_itemsPending || !--m_itemsPending) {
      RefreshList(1);
    }
  }
  static void SetSelection(int index);
  static int  GetSelectionIndex();
  static int  GetSkillLine() {
    return m_skillLine;
  }
  static int GetNumTradeSkills() {
    return m_filteredSkills;
  }
  static const TradeSkillInfo *GetTradeSkillInfo(UINT index) {
    return index < m_filteredSkills ? m_skills[index] : 0;
  }
  static void SetSkillLine(int id);
  static void RefreshList(int resetFilters);
  static UINT GetNumSubClasses() {
    return m_numSubClasses;
  }
  static TradeSkillSubClassInfo *GetSubClass(UINT index) {
    return index < m_numSubClasses ? m_subClasses[index] : 0;
  }
  static int  GetSubClassIndexFromSkill(UINT index);
  static BOOL IsCollpasedHeader(UINT index);
  static int  GetSubClassFilter() {
    return m_subClassFilter;
  }
  static int GetInvTypeFilter() {
    return m_invTypeFilter;
  }
  static int GetCollapseFilter() {
    return m_collapseFilter;
  }
  static int GetAvailableSlots() {
    return m_availableSlots;
  }
  static void SetSubClassFilter(int filter);
  static void SetInvTypeFilter(int filter);
  static void SetCollapseFilter(int filter);

 protected:
  static void FilterAndSortSkills();

 private:
  static int                                       m_skillLine;
  static int                                       m_currentSelection;
  static UINT                                      m_itemsPending;
  static UINT                                      m_numSkills;
  static UINT                                      m_numSubClasses;
  static UINT                                      m_filteredSkills;
  static int                                       m_subClassFilter;
  static int                                       m_invTypeFilter;
  static int                                       m_collapseFilter;
  static TSGrowableArray<TradeSkillInfo *>         m_skills;
  static TSGrowableArray<TradeSkillSubClassInfo *> m_subClasses;
  static int                                       m_availableSlots;
};

class SpellCast {
 public:
  SpellCast() {
    caster = 0;
    spellID = 0;
    castTime = 0;
    targets = 0;
    castEndTime = 0;
    unitTarget = 0;
    itemTarget = 0;
    ammoItem = 0;
    spellLevel = 0;
    spellIndex = 0;
    reflector = 0;
    overrideRank = -1;
    flags = 0;
    selectedTarget = 0;
  }

  ~SpellCast() {
  }

  DWORDLONG          caster;
  DWORDLONG          casterUnit;
  int                spellID;
  WORD               targets;
  DWORDLONG          unitTarget;
  DWORDLONG          itemTarget;
  DWORDLONG          selectedTarget;
  NTempest::C3Vector sourceLocation;
  NTempest::C3Vector destLocation;
  float              destFacing;
  UINT               destZoneID;
  UINT               castTime;
  UINT               castEndTime;
  int                spellIndex;
  UINT               spellLevel;
  DWORDLONG          ammoItem;
  DWORDLONG          reflector;
  char               targetString[128];
  int                overrideRank;
  WORD               flags;

  void BuildFullZoneUpdate(CDataStore *msg);
  void UnpackFullZoneUpdate(CDataStore *msg);
};

NODEDECL(SPELLHISTORY) {
  int   spellID;
  int   itemID;
  DWORD recoveryStart;
  UINT  recoveryTime;
  int   category;
  DWORD categoryRecoveryStart;
  UINT  categoryRecoveryTime;
  bool  onHold;
  int   startRecoveryCategory;
  UINT  startRecoveryTime;
};

class SpellHistory {
 public:
  void AddHistory(
      int   spellID,
      int   itemID,
      DWORD recoveryStart,
      UINT  recoveryTime,
      int   category,
      DWORD categoryRecoveryStart,
      UINT  categoryRecoveryTime,
      bool  onHold,
      int   startRecoveryCategory,
      UINT  startRecoveryTime
  );
  BOOL GetCooldown(int spellID, int itemID, UINT *duration, DWORD *startTime, UINT *enable);
  BOOL IsOnHold(int spellID, int itemID);
  void RemoveHold(int spellID, DWORD startTime, bool clear);
  void ClearHistory();
  void GarbageCollect(DWORD timestamp);

 protected:
  LISTDECL(SPELLHISTORY, m_spellHistory);
  LISTDECL(SPELLHISTORY, m_freeList);
};

struct ITEMCOOLDOWNHASHNODE : public TSHashObject<ITEMCOOLDOWNHASHNODE, HASHKEY_NONE> {
  int   spellID;
  DWORD startTime;
  bool  needsEvent;
};

struct FindAmmoData {
  int  ammoType;
  BYTE exoticAmmo;
};

void CursorSetCursorMode(CURSORANIMATIONS mode);
void CursorModelSetSequence(CURSORANIMATIONS sequence);
void CursorResetCursor(int force);
void SpellPutCastTargets(SpellCast *cast, CDataStore *msg);
void SpellGetCastTargets(SpellCast *cast, CDataStore *msg);
void SpellVisualsHandleCastStop(int id, CGUnit_C *caster, BYTE status, BYTE reason);
void SpellVisualsHandleCastStart(int id, const SpellCast &cast, CGUnit_C *caster, UINT duration, UINT animDuration, bool wasProc);
void UnitCombatLogSpellFail(CGUnit_C *caster, int spellID, LPCSTR message);
void SpellVisualsPlayKit(CGUnit_C *target, UINT id);
void SpellVisualsHandleSpellStart(
    int                            spellID,
    const SpellCast               &cast,
    CGGameObject_C                *caster,
    const TSStackArray<DWORDLONG> &targets,
    bool                           ignoreAreaEffect,
    bool                           hits
);
void SpellVisualsHandleSpellStartHits(
    int                            spellID,
    const SpellCast               &cast,
    CGUnit_C                      *caster,
    const TSStackArray<DWORDLONG> &targets,
    int                            ammoDisplayID,
    int                            ammoInventoryType,
    int                            flags
);
void SpellVisualsHandleSpellStartMisses(
    int                            spellID,
    const SpellCast               &cast,
    CGUnit_C                      *caster,
    const TSStackArray<DWORDLONG> &targets,
    TSStackArray<MISS_REASON>     &missReasons,
    int                            ammoDisplayID,
    int                            ammoInventoryType,
    int                            flags
);
void                       UnitCombatLogCastGo(UINT spellID, DWORDLONG casterUnit, DWORDLONG target);
void                       UnitEffectPreloadSpellEffects(int spellID);
const ItemSubClassRec     *SDBItemSubclassGetSubClassRec(UINT classID, UINT subClassID);
const SkillLineAbilityRec *SpellTableLookupAbility(UINT raceID, UINT classID, UINT spellID);
DWORDLONG                  Script_GetGUIDFromName(LPCSTR name);

void                          Spell_C_SpellFailed(int spellID, BYTE reason, int arg1, int arg2);
static const ItemSubClassRec *FindAnyItemSubclassRec(int classID, UINT subclassMask);
static LPCSTR                 GetStringReason(BYTE reason);
static void                   SpellMissingItemCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted);
static void                   ItemCheckCooldownCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted);
bool                          Spell_C_HaveSpellTokens(CGPlayer_C *player, const SpellRec *rec, bool report);
bool                          Spell_C_HaveEquippedSpellItems(CGPlayer_C *player, const SpellRec *rec, bool checkAmmo, bool report);
static BOOL                   FindAmmoCallback(const CGItem_C *item, LPVOID param);
bool                          Spell_C_IsModal();
bool                          Spell_C_IsTargeting();
void                          Spell_C_CancelSpell(bool failed, bool notifyServer, SPELL_FAILED_REASON reason);
static void                   Spell_C_SetModal(int spellID, const CGItem_C *item);
static bool                   Spell_C_TargetSpell(CGUnit_C *caster, const SpellRec *srec);
static void                   SendCast(SpellCast *cast);
static bool                   RangeCheckSelected(CGPlayer_C *caster, const SpellRec *srec);
static bool                   RangeCheck(CGPlayer_C *caster, CGObject_C *target, int spellID);
static void                   GameObjectStatsCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted);
bool                          Spell_C_HandleSpriteClick(CGObject_C *object);
static BOOL                   CCommand_Cast(LPCSTR, LPCSTR arguments);
static BOOL                   CastResultHandler(LPVOID, NETMESSAGE, DWORD, CDataStore *msg);
static BOOL                   SpellDelayed(LPVOID, NETMESSAGE, DWORD, CDataStore *msg);
static BOOL                   SpellChannelStart(LPVOID, NETMESSAGE, DWORD, CDataStore *msg);
static BOOL                   SpellChannelUpdate(LPVOID, NETMESSAGE, DWORD, CDataStore *msg);
static BOOL                   SpellAddDynamicTarget(LPVOID, NETMESSAGE, DWORD, CDataStore *msg);
static BOOL                   SpellStartHandler(LPVOID, NETMESSAGE msgID, DWORD, CDataStore *msg);
static void                   SpellStart(DWORDLONG casterGUID, DWORDLONG casterUnit, int spellID, CDataStore *msg);
static void                   SpellGo(const DWORDLONG &casterGUID, const DWORDLONG &casterUnit, int spellID, CDataStore *msg);
static void                   SetItemCooldown(int itemID, int spellID, DWORD startTime, bool needsEvent);
static void                   ItemStatsCooldownCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted);
static BOOL                   SpellFailedHandler(LPVOID, NETMESSAGE, DWORD, CDataStore *msg);
static BOOL                   PetSpellFailedHandler(LPVOID, NETMESSAGE, DWORD, CDataStore *msg);
static BOOL                   SpellCooldownHandler(LPVOID, NETMESSAGE, DWORD eventTime, CDataStore *msg);
static BOOL                   ItemCooldownHandler(LPVOID, NETMESSAGE, DWORD eventTime, CDataStore *msg);
static BOOL                   CooldownEvent(LPVOID, NETMESSAGE msgID, DWORD timeReceived, CDataStore *msg);
static void                   Spell_C_CooldownEventTriggered(int spellID, DWORD receivedTime, BOOL isPet, int clear);
static void                   Spell_C_ClearCooldowns(BOOL isPet);
static BOOL                   CooldownCheat(LPVOID, NETMESSAGE, DWORD, CDataStore *msg);
static BOOL                   PetTameFailure(LPVOID, NETMESSAGE, DWORD, CDataStore *msg);
static BOOL                   PlaySpellVisualKit(LPVOID, NETMESSAGE, DWORD, CDataStore *msg);
static BOOL                   CCommand_Learn(LPCSTR command, LPCSTR arguments);
static BOOL                   CCommand_Cooldown(LPCSTR command, LPCSTR arguments);
static BOOL                   CCommand_CooldownPet(LPCSTR command, LPCSTR arguments);
static BOOL                   CCommand_UseSkill(LPCSTR command, LPCSTR arguments);
static BOOL                   CCommand_SetSkill(LPCSTR command, LPCSTR arguments);
static BOOL                   CCommand_CancelAura(LPCSTR, LPCSTR arguments);
static BOOL                   CCommand_SpellString(LPCSTR, LPCSTR arguments);

static TSHashTable<ITEMCOOLDOWNHASHNODE, HASHKEY_NONE> s_itemCooldowns;
static SpellHistory                                    s_spellHistory[2];
static SpellCast                                       s_spellCast;
static WORD                                            s_needTargets;
static int                                             s_modalSpellID;
static int                                             s_savedModalSpellID;
static DWORDLONG                                       s_modalItemID;
static DWORDLONG                                       s_savedModalItemID;
static BYTE                                            s_playerCast;
static char                                            s_spellTargetString[128];
static UINT                                            s_spellWorldModel;
static float                                           s_spellWorldModelFacing;
static bool                                            s_spellWorldModelHousing;

#include "Ui/WorldFrame.h"

void SpellHistory::AddHistory(
    int   spellID,
    int   itemID,
    DWORD recoveryStart,
    UINT  recoveryTime,
    int   category,
    DWORD categoryRecoveryStart,
    UINT  categoryRecoveryTime,
    bool  onHold,
    int   startRecoveryCategory,
    UINT  startRecoveryTime
) {
  if (!recoveryTime && !categoryRecoveryTime && !onHold && !startRecoveryTime) {
    return;
  }

  SPELLHISTORY *history = m_freeList.Head();
  if (history) {
    m_freeList.UnlinkNode(history);
    m_spellHistory.LinkNode(history, LIST_TAIL, 0);
  } else {
    history = m_spellHistory.NewNode(LIST_TAIL, 0, 0);
  }

  history->spellID = spellID;
  history->itemID = itemID;
  history->recoveryStart = recoveryStart;
  history->recoveryTime = recoveryTime;
  history->category = category;
  history->categoryRecoveryStart = categoryRecoveryStart;
  history->categoryRecoveryTime = categoryRecoveryTime;
  history->onHold = onHold;
  history->startRecoveryCategory = startRecoveryCategory;
  history->startRecoveryTime = startRecoveryTime;
}

BOOL SpellHistory::GetCooldown(int spellID, int itemID, UINT *duration, DWORD *startTime, UINT *enable) {
  if (enable) {
    *enable = 1;
  }

  const SpellRec *spell = g_spellDB.GetRecord(spellID);
  if (!spell) {
    return 0;
  }

  int category = spell->m_category;
  int startCategory = spell->m_startRecoveryCategory;
  if (itemID) {
    const ItemStats *stats = g_itemDBCache.GetRecord(itemID, 0, 0, 0);
    if (stats) {
      int index;
      for (index = 0; index < 5; ++index) {
        if (stats->m_spellID[index] == spellID && stats->m_spellCategory[index] > 0) {
          category = stats->m_spellCategory[index];
        }
      }
    }
  }

  DWORD now = OsGetAsyncTimeMs();
  DWORD latestEnd = now;
  ITERATELIST(SPELLHISTORY, m_spellHistory, history) {
    if (history->spellID == spellID && history->itemID == itemID && history->recoveryTime) {
      DWORD end = (history->onHold ? now : history->recoveryStart) + history->recoveryTime;
      if ((long)(end - latestEnd) >= 0) {
        if (duration) {
          *duration = history->recoveryTime;
        }
        if (startTime) {
          *startTime = history->onHold ? now : history->recoveryStart;
        }
        if (enable) {
          *enable = !history->onHold;
        }
        latestEnd = end;
      }
    }

    if (history->category == category && history->categoryRecoveryTime) {
      DWORD end = (history->onHold ? now : history->categoryRecoveryStart) + history->categoryRecoveryTime;
      if ((long)(end - latestEnd) >= 0) {
        if (duration) {
          *duration = history->categoryRecoveryTime;
        }
        if (startTime) {
          *startTime = history->onHold ? now : history->categoryRecoveryStart;
        }
        if (enable) {
          *enable = !history->onHold;
        }
        latestEnd = end;
      }
    }

    if (history->startRecoveryCategory == startCategory && history->startRecoveryTime) {
      DWORD end = (history->onHold ? now : history->recoveryStart) + history->startRecoveryTime;
      if ((long)(end - latestEnd) >= 0) {
        if (duration) {
          *duration = history->startRecoveryTime;
        }
        if (startTime) {
          *startTime = history->onHold ? now : history->recoveryStart;
        }
        if (enable) {
          *enable = !history->onHold;
        }
        latestEnd = end;
      }
    }
  }
  return latestEnd != now;
}

BOOL SpellHistory::IsOnHold(int spellID, int itemID) {
  const SpellRec *spell = g_spellDB.GetRecord(spellID);
  if (!spell) {
    return 0;
  }

  int category = spell->m_category;
  if (itemID) {
    const ItemStats *stats = g_itemDBCache.GetRecord(itemID, 0, 0, 0);
    if (stats) {
      for (int i = 0; i < 5; ++i) {
        if (stats->m_spellID[i] == spellID && stats->m_spellCategory[i] > 0) {
          category = stats->m_spellCategory[i];
        }
      }
    }
  }

  ITERATELIST(SPELLHISTORY, m_spellHistory, history) {
    if ((history->spellID == spellID && history->itemID == itemID && history->recoveryTime && history->onHold) ||
        (history->category == category && history->categoryRecoveryTime && history->onHold))
    {
      return 1;
    }
  }
  return 0;
}

void SpellHistory::RemoveHold(int spellID, DWORD startTime, bool clear) {
  SAFEITERATELIST(SPELLHISTORY, m_spellHistory, history) {
    if (history->spellID == spellID && history->onHold) {
      if (clear) {
        m_spellHistory.UnlinkNode(history);
        m_freeList.LinkNode(history, LIST_TAIL, 0);
      } else {
        history->recoveryStart = startTime;
        history->categoryRecoveryStart = startTime;
        history->onHold = false;
      }
    }
  }
}

void SpellHistory::ClearHistory() {
  SPELLHISTORY *history;
  do {
    history = m_spellHistory.Head();
    if (!history) {
      break;
    }
    m_spellHistory.UnlinkNode(history);
    m_freeList.LinkNode(history, LIST_TAIL, 0);
  } while (TRUE);
}

void SpellHistory::GarbageCollect(DWORD timestamp) {
  SAFEITERATELIST(SPELLHISTORY, m_spellHistory, node) {
    if (node->onHold) {
      continue;
    }

    if (node->recoveryTime) {
      DWORD endTime = node->recoveryStart + node->recoveryTime;
      if ((long)(timestamp - endTime) < 0) {
        continue;
      }
    }

    if (node->categoryRecoveryTime) {
      DWORD endTime = node->categoryRecoveryStart + node->categoryRecoveryTime;
      if ((long)(timestamp - endTime) < 0) {
        continue;
      }
    }

    m_spellHistory.UnlinkNode(node);
    m_freeList.LinkNode(node, LIST_TAIL, 0);
  }
}

void Spell_C_SpellFailed(int spellID, BYTE reason, int arg1, int arg2) {
  const SpellRec *spell = g_spellDB.GetRecord(spellID);
  GAME_ERROR_TYPE error = GERR_SPELL_FAILED_S;
  BOOL            isPet = 0;

  FrameScript_SignalEvent(370);
  if (spell) {
    SndInterfacePlaySpellFizzleSound(spellID, static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__)));
    if (reason == SPELL_FAILED_NOT_READY) {
      error = (spell->m_category == 10 || spell->m_category == 11)
                  ? GERR_FOOD_COOLDOWN
                  : (spell->m_category == 4 || spell->m_category == 9)
                        ? GERR_POTION_COOLDOWN
                        : (spell->m_attributes & 0x10) ? GERR_ABILITY_COOLDOWN : GERR_SPELL_COOLDOWN;
    } else if (reason == SPELL_FAILED_ITEM_NOT_READY) {
      error = GERR_ITEM_COOLDOWN;
    } else if (reason == SPELL_FAILED_HUNGER_SATIATED) {
      error = GERR_HUNGER_SATIATED;
    } else if (reason == SPELL_FAILED_THIRST_SATIATED) {
      error = GERR_THIRST_SATIATED;
    } else if (reason == SPELL_FAILED_TOTEMS) {
      error = GERR_SPELL_FAILED_TOTEMS;
    } else if (reason == SPELL_FAILED_REAGENTS) {
      error = GERR_SPELL_FAILED_REAGENTS;
    } else if (reason == SPELL_FAILED_EQUIPPED_ITEM) {
      error = GERR_SPELL_FAILED_EQUIPPED_ITEM;
    } else if (reason == SPELL_FAILED_EQUIPPED_ITEM_CLASS) {
      error = GERR_SPELL_FAILED_EQUIPPED_ITEM_CLASS_S;
    } else if (reason == SPELL_FAILED_BAD_IMPLICIT_TARGETS) {
      error = GERR_GENERIC_NO_TARGET;
    } else if (reason == SPELL_FAILED_OUT_OF_RANGE) {
      error = GERR_SPELL_OUT_OF_RANGE;
    } else if (reason == SPELL_FAILED_NEED_AMMO) {
      error = GERR_NOAMMO_S;
    } else if (reason == SPELL_FAILED_ONLY_SHAPESHIFT) {
      error = GERR_SPELL_FAILED_SHAPESHIFT_FORM_S;
    } else if (reason == SPELL_FAILED_BAD_TARGETS) {
      error = (spell->m_targets & 0x10) ? GERR_INVALID_ITEM_TARGET : GERR_INVALID_ATTACK_TARGET;
    } else if (reason == SPELL_FAILED_NOTUNSHEATHED) {
      error = GERR_SPELL_FAILED_NOTUNSHEATHED;
    }

    if (spell->m_effect[0] == 57 || (spell->m_effect[0] == 36 && spell->m_implicitTargetA[0] == 5)) {
      isPet = 1;
    }
  }

  CGPlayer_C *playerPtr = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (playerPtr) {
    playerPtr->OnSpellFailed(spell, reason);
  }
  if (reason == SPELL_FAILED_DONT_REPORT) {
    return;
  }

  char message[128];
  char token[64];
  message[0] = 0;
  if (isPet) {
    SStrPrintf(token, sizeof(token), "%s_PET", GetStringReason(reason));
    LPCSTR text = FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE);
    SStrCopy(message, text, sizeof(message));
  }
  if (!message[0]) {
    LPCSTR failure = GetStringReason(reason);
    LPCSTR text = FrameScript_GetText(failure, -1, GENDER_NOT_APPLICABLE);
    SStrCopy(message, text, sizeof(message));
  }

  char processedmessage[256];
  bool processed = false;
  switch (reason) {
    case SPELL_FAILED_EQUIPPED_ITEM_CLASS: {
      const ItemSubClassRec *subclassRec = FindAnyItemSubclassRec(arg1, arg2);
      if (subclassRec) {
        SStrPrintf(processedmessage, sizeof(processedmessage), message, subclassRec->m_displayName_lang[CURRENT_LANGUAGE]);
        processed = true;
      }
      break;
    }
    case SPELL_FAILED_NEED_AMMO:
    case SPELL_FAILED_NEED_AMMO_POUCH: {
      const ItemSubClassRec *subclassRec = FindAnyItemSubclassRec(11, 1 << arg1);
      if (subclassRec) {
        SStrPrintf(processedmessage, sizeof(processedmessage), message, subclassRec->m_displayName_lang[CURRENT_LANGUAGE]);
        processed = true;
      }
      break;
    }
    case SPELL_FAILED_NEED_EXOTIC_AMMO: {
      const ItemSubClassRec *subclassRec = FindAnyItemSubclassRec(6, 1 << arg1);
      if (subclassRec) {
        SStrPrintf(processedmessage, sizeof(processedmessage), message, subclassRec->m_displayName_lang[CURRENT_LANGUAGE]);
        processed = true;
      }
      break;
    }
    case SPELL_FAILED_REAGENTS:
    case SPELL_FAILED_TOTEMS: {
      const ItemStats *stats =
          g_itemDBCache.GetRecord(arg1, spellID | 0xB000000000000000ui64, SpellMissingItemCallback, (LPVOID)reason);
      if (!stats) {
        return;
      }
      SStrPrintf(processedmessage, sizeof(processedmessage), message, stats->m_displayName[0]);
      processed = true;
    }
    case SPELL_FAILED_REQUIRES_SPELL_FOCUS: {
      const SpellFocusObjectRec *focus = g_spellFocusObjectDB.GetRecord(arg1);
      if (focus) {
        SStrPrintf(processedmessage, sizeof(processedmessage), message, focus->m_name_lang[CURRENT_LANGUAGE]);
        processed = true;
      }
      break;
    }
    case SPELL_FAILED_ONLY_SHAPESHIFT: {
      if (!spell) {
        return;
      }
      char shapes[512];
      int  first = 1;
      UINT numEntries = g_spellShapeshiftFormDB.GetNumRecords();
      for (UINT i = 0; i < numEntries; ++i) {
        if (spell->m_shapeshiftMask & (1 << i)) {
          const SpellShapeshiftFormRec *form = g_spellShapeshiftFormDB.GetRecordByIndex(i);
          if (form && form->m_name_lang[CURRENT_LANGUAGE] && *form->m_name_lang[CURRENT_LANGUAGE]) {
            if (first) {
              SStrCopy(shapes, form->m_name_lang[CURRENT_LANGUAGE], sizeof(shapes));
              first = 0;
            } else {
              SStrPack(shapes, ", ", sizeof(shapes));
              SStrPack(shapes, form->m_name_lang[CURRENT_LANGUAGE], sizeof(shapes));
            }
          }
        }
      }
      if (first) {
        return;
      }
      SStrPrintf(processedmessage, sizeof(processedmessage), message, shapes);
      processed = true;
      break;
    }
    case SPELL_FAILED_NO_POWER: {
      static const GAME_ERROR_TYPE s_gerrEnums[4] = {GERR_OUT_OF_MANA, GERR_OUT_OF_RAGE, GERR_OUT_OF_FOCUS, GERR_OUT_OF_ENERGY};
      if (spell->m_powerType == -2) {
        CGGameUI::DisplayError(GERR_OUT_OF_HEALTH);
      } else {
        CGGameUI::DisplayError(s_gerrEnums[spell->m_powerType]);
      }
      UnitCombatLogSpellFail(playerPtr, spellID, CGGameUI::GetLastErrorString());
      return;
    }
  }

  if (processed) {
    UnitCombatLogSpellFail(playerPtr, spellID, processedmessage);
    CGGameUI::DisplayError(error, processedmessage);
    return;
  }

  UnitCombatLogSpellFail(playerPtr, spellID, message);
  CGGameUI::DisplayError(error, message);
  if (spellID == s_modalSpellID) {
    Spell_C_CancelSpell(true, true, (SPELL_FAILED_REASON)reason);
  } else if (!Spell_C_IsModal()) {
    FrameScript_SignalEvent(reason == SPELL_FAILED_INTERRUPTED || reason == SPELL_FAILED_INTERRUPTED_COMBAT ? 318 : 317);
  }
}

static const ItemSubClassRec *FindAnyItemSubclassRec(int classID, UINT subclassMask) {
  for (int i = 0; i < g_itemSubClassDB.GetNumRecords(); ++i) {
    const ItemSubClassRec *record = g_itemSubClassDB.GetRecordByIndex(i);
    if (record->m_classID == classID && (subclassMask & (1 << record->m_subClassID))) {
      return record;
    }
  }
  return 0;
}

static LPCSTR GetStringReason(BYTE reason) {
  switch (reason) {
    case SPELL_FAILED_AFFECTING_COMBAT:
      return "SPELL_FAILED_AFFECTING_COMBAT";
    case SPELL_FAILED_ALREADY_HAVE_CHARM:
      return "SPELL_FAILED_ALREADY_HAVE_CHARM";
    case SPELL_FAILED_ALREADY_HAVE_SUMMON:
      return "SPELL_FAILED_ALREADY_HAVE_SUMMON";
    case SPELL_FAILED_ALREADY_OPEN:
      return "SPELL_FAILED_ALREADY_OPEN";
    case SPELL_FAILED_AURA_BOUNCED:
      return "SPELL_FAILED_AURA_BOUNCED";
    case SPELL_FAILED_BAD_IMPLICIT_TARGETS:
      return "SPELL_FAILED_BAD_IMPLICIT_TARGETS";
    case SPELL_FAILED_BAD_TARGETS:
      return "SPELL_FAILED_BAD_TARGETS";
    case SPELL_FAILED_CANT_BE_CHARMED:
      return "SPELL_FAILED_CANT_BE_CHARMED";
    case SPELL_FAILED_CANT_STEALTH:
      return "SPELL_FAILED_CANT_STEALTH";
    case SPELL_FAILED_CASTER_AURASTATE:
      return "SPELL_FAILED_CASTER_AURASTATE";
    case SPELL_FAILED_CASTER_DEAD:
      return "SPELL_FAILED_CASTER_DEAD";
    case SPELL_FAILED_DONT_REPORT:
      return "SPELL_FAILED_DONT_REPORT";
    case SPELL_FAILED_EQUIPPED_ITEM:
      return "SPELL_FAILED_EQUIPPED_ITEM";
    case SPELL_FAILED_EQUIPPED_ITEM_CLASS:
      return "SPELL_FAILED_EQUIPPED_ITEM_CLASS";
    case SPELL_FAILED_ERROR:
      return "SPELL_FAILED_ERROR";
    case SPELL_FAILED_FIZZLE:
      return "SPELL_FAILED_FIZZLE";
    case SPELL_FAILED_HUNGER_SATIATED:
      return "SPELL_FAILED_HUNGER_SATIATED";
    case SPELL_FAILED_INTERRUPTED:
      return "SPELL_FAILED_INTERRUPTED";
    case SPELL_FAILED_INTERRUPTED_COMBAT:
      return "SPELL_FAILED_INTERRUPTED_COMBAT";
    case SPELL_FAILED_ITEM_ALREADY_ENCHANTED:
      return "SPELL_FAILED_ITEM_ALREADY_ENCHANTED";
    case SPELL_FAILED_ITEM_NOT_FOUND:
      return "SPELL_FAILED_ITEM_NOT_FOUND";
    case SPELL_FAILED_ITEM_NOT_READY:
      return "SPELL_FAILED_ITEM_NOT_READY";
    case SPELL_FAILED_LEVEL_REQUIREMENT:
      return "SPELL_FAILED_LEVEL_REQUIREMENT";
    case SPELL_FAILED_LINE_OF_SIGHT:
      return "SPELL_FAILED_LINE_OF_SIGHT";
    case SPELL_FAILED_LOWLEVEL:
      return "SPELL_FAILED_LOWLEVEL";
    case SPELL_FAILED_LOW_CASTLEVEL:
      return "SPELL_FAILED_LOW_CASTLEVEL";
    case SPELL_FAILED_MOVING:
      return "SPELL_FAILED_MOVING";
    case SPELL_FAILED_NEED_AMMO:
      return "SPELL_FAILED_NEED_AMMO";
    case SPELL_FAILED_NEED_AMMO_POUCH:
      return "SPELL_FAILED_NEED_AMMO_POUCH";
    case SPELL_FAILED_NEED_EXOTIC_AMMO:
      return "SPELL_FAILED_NEED_EXOTIC_AMMO";
    case SPELL_FAILED_NOPATH:
      return "SPELL_FAILED_NOPATH";
    case SPELL_FAILED_NOTSTANDING:
      return "SPELL_FAILED_NOTSTANDING";
    case SPELL_FAILED_NOT_BEHIND:
      return "SPELL_FAILED_NOT_BEHIND";
    case SPELL_FAILED_NOT_BEHIND_OR_SIDE:
      return "SPELL_FAILED_NOT_BEHIND_OR_SIDE";
    case SPELL_FAILED_NOT_HERE:
      return "SPELL_FAILED_NOT_HERE";
    case SPELL_FAILED_NOT_KNOWN:
      return "SPELL_FAILED_NOT_KNOWN";
    case SPELL_FAILED_NOT_MOUNTED:
      return "SPELL_FAILED_NOT_MOUNTED";
    case SPELL_FAILED_NOT_READY:
      return "SPELL_FAILED_NOT_READY";
    case SPELL_FAILED_NOT_SHAPESHIFT:
      return "SPELL_FAILED_NOT_SHAPESHIFT";
    case SPELL_FAILED_NOT_TRADING:
      return "SPELL_FAILED_NOT_TRADING";
    case SPELL_FAILED_NO_AMMO:
      return "SPELL_FAILED_NO_AMMO";
    case SPELL_FAILED_NO_CHARGES_REMAIN:
      return "SPELL_FAILED_NO_CHARGES_REMAIN";
    case SPELL_FAILED_NO_ENDURANCE:
      return "SPELL_FAILED_NO_ENDURANCE";
    case SPELL_FAILED_NO_PET:
      return "SPELL_FAILED_NO_PET";
    case SPELL_FAILED_NO_POWER:
      return "SPELL_FAILED_NO_POWER";
    case SPELL_FAILED_ONLY_ABOVEWATER:
      return "SPELL_FAILED_ONLY_ABOVEWATER";
    case SPELL_FAILED_ONLY_DAYTIME:
      return "SPELL_FAILED_ONLY_DAYTIME";
    case SPELL_FAILED_ONLY_INDOORS:
      return "SPELL_FAILED_ONLY_INDOORS";
    case SPELL_FAILED_ONLY_MOUNTED:
      return "SPELL_FAILED_ONLY_MOUNTED";
    case SPELL_FAILED_ONLY_NIGHTTIME:
      return "SPELL_FAILED_ONLY_NIGHTTIME";
    case SPELL_FAILED_ONLY_OUTDOORS:
      return "SPELL_FAILED_ONLY_OUTDOORS";
    case SPELL_FAILED_ONLY_SHAPESHIFT:
      return "SPELL_FAILED_ONLY_SHAPESHIFT";
    case SPELL_FAILED_ONLY_STEALTHED:
      return "SPELL_FAILED_ONLY_STEALTHED";
    case SPELL_FAILED_ONLY_UNDERWATER:
      return "SPELL_FAILED_ONLY_UNDERWATER";
    case SPELL_FAILED_OUT_OF_RANGE:
      return "SPELL_FAILED_OUT_OF_RANGE";
    case SPELL_FAILED_PACIFIED:
      return "SPELL_FAILED_PACIFIED";
    case SPELL_FAILED_REAGENTS:
      return "SPELL_FAILED_REAGENTS";
    case SPELL_FAILED_REQUIRES_SPELL_FOCUS:
      return "SPELL_FAILED_REQUIRES_SPELL_FOCUS";
    case SPELL_FAILED_SILENCED:
      return "SPELL_FAILED_SILENCED";
    case SPELL_FAILED_SPELL_IN_PROGRESS:
      return "SPELL_FAILED_SPELL_IN_PROGRESS";
    case SPELL_FAILED_SPELL_LEARNED:
      return "SPELL_FAILED_SPELL_LEARNED";
    case SPELL_FAILED_SPELL_UNAVAILABLE:
      return "SPELL_FAILED_SPELL_UNAVAILABLE";
    case SPELL_FAILED_STUNNED:
      return "SPELL_FAILED_STUNNED";
    case SPELL_FAILED_TARGETS_DEAD:
      return "SPELL_FAILED_TARGETS_DEAD";
    case SPELL_FAILED_TARGET_AFFECTING_COMBAT:
      return "SPELL_FAILED_TARGET_AFFECTING_COMBAT";
    case SPELL_FAILED_TARGET_AURASTATE:
      return "SPELL_FAILED_TARGET_AURASTATE";
    case SPELL_FAILED_TARGET_ENEMY:
      return "SPELL_FAILED_TARGET_ENEMY";
    case SPELL_FAILED_TARGET_ENRAGED:
      return "SPELL_FAILED_TARGET_ENRAGED";
    case SPELL_FAILED_TARGET_FRIENDLY:
      return "SPELL_FAILED_TARGET_FRIENDLY";
    case SPELL_FAILED_TARGET_IS_PLAYER:
      return "SPELL_FAILED_TARGET_IS_PLAYER";
    case SPELL_FAILED_TARGET_NOT_DEAD:
      return "SPELL_FAILED_TARGET_NOT_DEAD";
    case SPELL_FAILED_TARGET_NOT_IN_PARTY:
      return "SPELL_FAILED_TARGET_NOT_IN_PARTY";
    case SPELL_FAILED_TARGET_NO_POCKETS:
      return "SPELL_FAILED_TARGET_NO_POCKETS";
    case SPELL_FAILED_THIRST_SATIATED:
      return "SPELL_FAILED_THIRST_SATIATED";
    case SPELL_FAILED_TOO_CLOSE:
      return "SPELL_FAILED_TOO_CLOSE";
    case SPELL_FAILED_TOTEMS:
      return "SPELL_FAILED_TOTEMS";
    case SPELL_FAILED_TRY_AGAIN:
      return "SPELL_FAILED_TRY_AGAIN";
    case SPELL_FAILED_UNIT_NOT_ATSIDE:
      return "SPELL_FAILED_UNIT_NOT_ATSIDE";
    case SPELL_FAILED_UNIT_NOT_BEHIND:
      return "SPELL_FAILED_UNIT_NOT_BEHIND";
    case SPELL_FAILED_UNIT_NOT_INFRONT:
      return "SPELL_FAILED_UNIT_NOT_INFRONT";
    case SPELL_FAILED_NO_MOUNTS_ALLOWED:
      return "SPELL_FAILED_NO_MOUNTS_ALLOWED";
    case SPELL_FAILED_CHEST_IN_USE:
      return "SPELL_FAILED_CHEST_IN_USE";
    case SPELL_FAILED_NO_COMBO_POINTS:
      return "SPELL_FAILED_NO_COMBO_POINTS";
    case SPELL_FAILED_TARGET_NOT_PLAYER:
      return "SPELL_FAILED_TARGET_NOT_PLAYER";
    case SPELL_FAILED_TARGET_DUELING:
      return "SPELL_FAILED_TARGET_DUELING";
    case SPELL_FAILED_NOTUNSHEATHED:
      return "SPELL_FAILED_NOTUNSHEATHED";
    case SPELL_FAILED_NOT_FISHABLE:
      return "SPELL_FAILED_NOT_FISHABLE";
    default:
      return "SPELL_FAILED_UNKNOWN";
  }
}

static void SpellMissingItemCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted) {
  BYTE            reason = (DWORD)arg;
  char            message[128];
  char            processedmessage[256];
  GAME_ERROR_TYPE error = GERR_SPELL_FAILED_S;

  LPCSTR text = FrameScript_GetText(GetStringReason(reason), -1, GENDER_NOT_APPLICABLE);
  SStrCopy(message, text, sizeof(message));
  switch (reason) {
    case SPELL_FAILED_REAGENTS:
      error = GERR_SPELL_FAILED_REAGENTS;
      break;
    case SPELL_FAILED_TOTEMS:
      error = GERR_SPELL_FAILED_TOTEMS;
      break;
  }

  const ItemStats *stats = g_itemDBCache.GetRecord(id, 0, 0, 0);
  if (stats) {
    SStrPrintf(processedmessage, sizeof(processedmessage), message, stats->m_displayName[0]);
  } else {
    SStrPrintf(processedmessage, sizeof(processedmessage), message, "UNKNOWN");
  }
  CGGameUI::DisplayError(error, processedmessage);
}

void Spell_C_SetCooldownLeft(
    int  spellID,
    int  itemID,
    int  category,
    int  recoveryLeft,
    int  categoryRecoveryLeft,
    bool needsEvent,
    BOOL isPet,
    int  startRecoveryTimeLeft
) {
  DWORD now = OsGetAsyncTimeMs();
  int   index = 0;
  DWORD spellRecoveryStart = 0;
  UINT  spellRecoveryTime = 0;
  DWORD categoryRecoveryStart = 0;
  UINT  categoryRecoveryTime = 0;

  const SpellRec *srec = g_spellDB.GetRecord(spellID);
  if (!srec) {
    return;
  }

  const ItemStats *stats = 0;
  if (itemID) {
    stats = g_itemDBCache.GetRecord(itemID, 0, 0, 0);
    if (stats) {
      for (; index < NUM_ITEM_SPELLS; ++index) {
        if (stats->m_spellID[index] == spellID) {
          break;
        }
      }
      FATALASSERT(index < NUM_ITEM_SPELLS);
    }
  }

  int recoveryTime;
  if (recoveryLeft > 0) {
    recoveryTime = stats ? stats->m_spellCooldown[index] : -1;
    if (recoveryTime < 0) {
      recoveryTime = srec->m_recoveryTime;
    }
    if (recoveryTime <= recoveryLeft) {
      recoveryTime = recoveryLeft;
    }
    spellRecoveryStart = now - recoveryTime + recoveryLeft;
    spellRecoveryTime = recoveryTime;
  }

  if (categoryRecoveryLeft > 0) {
    recoveryTime = stats ? stats->m_spellCategoryCooldown[index] : -1;
    if (recoveryTime < 0) {
      recoveryTime = srec->m_categoryRecoveryTime;
    }
    if (recoveryTime <= categoryRecoveryLeft) {
      recoveryTime = categoryRecoveryLeft;
    }
    categoryRecoveryStart = now - recoveryTime + categoryRecoveryLeft;
    categoryRecoveryTime = recoveryTime;
  }

  s_spellHistory[isPet].AddHistory(
      spellID, itemID, spellRecoveryStart, spellRecoveryTime, category, categoryRecoveryStart, categoryRecoveryTime, needsEvent, 0, 0
  );
}

int Spell_C_GetSpellCooldown(int spell, BOOL isPet, UINT *duration, DWORD *startTime, UINT *enable) {
  return s_spellHistory[isPet].GetCooldown(spell, 0, duration, startTime, enable);
}

int Spell_C_GetItemCooldown(int itemID, UINT *duration, DWORD *startTime, UINT *enable) {
  const ItemStats *stats =
      g_itemDBCache.GetRecord(itemID, ClntObjMgrGetActivePlayer(), ItemCheckCooldownCallback, 0);
  if (!stats) {
    return 0;
  }

  UINT index;
  for (index = 0; index < 5; ++index) {
    if (stats->m_spellID[index] > 0 && !stats->m_spellTrigger[index]) {
      return s_spellHistory[0].GetCooldown(stats->m_spellID[index], itemID, duration, startTime, enable);
    }
  }
  return 0;
}

static void ItemCheckCooldownCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted) {
  if (granted && Spell_C_GetItemCooldown(id, 0, 0, 0)) {
    CGActionBar::UpdateCooldowns();
    CGSpellBook::UpdateCooldowns();
    CGContainerInfo::UpdateCooldowns();
  }
}

int Spell_C_NeedsCooldownEvent(const SpellRec *srec, BOOL isPet) {
  return s_spellHistory[isPet].IsOnHold(srec->GetID(), 0);
}

int Spell_C_NeedsCooldownEvent(int itemID) {
  const ItemStats *stats =
      g_itemDBCache.GetRecord(itemID, ClntObjMgrGetActivePlayer(), ItemCheckCooldownCallback, 0);
  if (!stats) {
    return 0;
  }
  for (UINT i = 0; i < 5; ++i) {
    if (stats->m_spellID[i] > 0 && !stats->m_spellTrigger[i]) {
      return s_spellHistory[0].IsOnHold(stats->m_spellID[i], itemID);
    }
  }
  return 0;
}

int Spell_C_GetSpellByName(LPCSTR name) {
  int num = g_spellDB.GetNumRecords();
  for (int i = 0; i < num; ++i) {
    const SpellRec *spell = g_spellDB.GetRecordByIndex(i);
    LPCSTR spellName = spell->m_name_lang[CURRENT_LANGUAGE];
    if (!SStrCmpI(spellName, name, 0x7FFFFFFF)) {
      return spell->m_ID;
    }
  }
  ConsoleWriteA("Unknown Spell '%s'", DEFAULT_COLOR, name);
  return -1;
}

int Spell_C_GetSpellLevel(int id, BOOL isPet) {
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (isPet) {
    if (!unit) {
      return 0;
    }

    unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit->GetControlledGUID(), __FILE__, __LINE__));
  }

  return unit ? unit->GetSpellLevel(id) : 0;
}

int Spell_C_GetManaCost(int id, BOOL isPet) {
  const SpellRec *spell = g_spellDB.GetRecord(id);
  if (!spell) {
    return -1;
  }
  if (spell->m_manaCostPct && !isPet) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      return player->m_plyr->baseMana * (spell->m_manaCostPct * 0.01f);
    }
  }
  int level = Spell_C_GetSpellLevel(id, isPet);
  return spell->m_manaCostPerLevel * level + spell->m_manaCost;
}

int Spell_C_GetManaCostPerSecond(int id, BOOL isPet) {
  const SpellRec *spellRec = g_spellDB.GetRecord(id);
  if (!spellRec) {
    return -1;
  }
  int level = Spell_C_GetSpellLevel(id, isPet);
  return spellRec->m_manaPerSecondPerLevel * level + spellRec->m_manaPerSecond;
}

int Spell_C_GetCastTime(int id, BOOL isPet) {
  const SpellRec *spellRec = g_spellDB.GetRecord(id);
  if (!spellRec) {
    return 0;
  }

  const SpellCastTimesRec *castTime = g_spellCastTimesDB.GetRecord(spellRec->m_castingTimeIndex);
  if (!castTime) {
    return 0;
  }

  int level = Spell_C_GetSpellLevel(id, isPet);
  return max(castTime->m_perLevel * level + castTime->m_base, castTime->m_minimum);
}

void Spell_C_GetMinMaxRange(int id, float *min, float *max) {
  *min = 0.0f;
  *max = 0.0f;

  const SpellRec *spell = g_spellDB.GetRecord(id);
  if (!spell) {
    return;
  }

  const SpellRangeRec *range = g_spellRangeDB.GetRecord(spell->m_rangeIndex);
  if (!range) {
    return;
  }

  if (spell->m_attributes & 0x404) {
    *max = MAX_OBJ_INTEREST_RADIUS;
  } else if (range->m_flags & 1) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      CGUnit_C *target = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(player->IsAttacking(), __FILE__, __LINE__));
      float     targetReach = target ? target->GetCombatReach() : range->m_rangeMax;
      *max = player->GetCombatReach() + targetReach + 1.3333334f;
      *min = 0.0f;
    }
  } else {
    *min = range->m_rangeMin;
    *max = range->m_rangeMax;
  }
}

void Spell_C_GetMinMaxPoints(const SpellRec *srec, int effectIndex, int *min, int *max, UINT level, BOOL isPet) {
  *min = 0;
  *max = 0;
  if (!srec) {
    return;
  }

  int dieSides = srec->m_effectDieSides[effectIndex];
  int casterLevel = level ? level : Spell_C_GetSpellLevel(srec->m_ID, isPet);
  if (srec->m_baseLevel > 0) {
    casterLevel -= srec->m_baseLevel;
  }
  if (casterLevel < 0) {
    casterLevel = 0;
  }

  float levelBonus = casterLevel * srec->m_effectRealPointsPerLevel[effectIndex];
  int   minBonus = levelBonus;
  int   maxBonus;
  if (levelBonus - floorf(levelBonus) >= 0.5f) {
    maxBonus = ceilf(levelBonus);
  } else {
    maxBonus = floorf(levelBonus);
  }

  *min = srec->m_effectBaseDice[effectIndex] + srec->m_effectDicePerLevel[effectIndex] * casterLevel;
  *min += minBonus + srec->m_effectBasePoints[effectIndex];
  *max = srec->m_effectBaseDice[effectIndex] * dieSides;
  *max += srec->m_effectDicePerLevel[effectIndex] * (casterLevel * dieSides);
  *max += maxBonus + srec->m_effectBasePoints[effectIndex];
}

int Spell_C_GetModalSpell() {
  return s_modalSpellID;
}

const DWORDLONG &Spell_C_GetModalItem() {
  return s_modalItemID;
}

bool Spell_C_IsModal() {
  return s_modalSpellID != 0;
}

const DWORDLONG &Spell_C_GetCurrentCaster() {
  return s_spellCast.caster;
}

const DWORDLONG &Spell_C_GetCurrentTarget() {
  return s_spellCast.unitTarget;
}

bool Spell_C_HaveSpellTokens(CGPlayer_C *player, const SpellRec *rec, bool report) {
  UINT index;
  for (index = 0; index < 2; ++index) {
    if (rec->m_totem[index] && !player->m_inventory.FindItemOfType(rec->m_totem[index], 0)) {
      if (report) {
        Spell_C_SpellFailed(rec->m_ID, SPELL_FAILED_TOTEMS, rec->m_totem[index], -1);
      }
      return false;
    }
  }

  for (index = 0; index < 8; ++index) {
    if (rec->m_reagent[index] && player->m_inventory.GetItemTypeCount(rec->m_reagent[index], 0) < rec->m_reagentCount[index]) {
      if (report) {
        Spell_C_SpellFailed(rec->m_ID, SPELL_FAILED_REAGENTS, rec->m_reagent[index], -1);
      }
      return false;
    }
  }
  return true;
}

bool Spell_C_HaveEquippedSpellItems(CGPlayer_C *player, const SpellRec *rec, bool checkAmmo, bool report) {
  bool usable = true;

  if (!(rec->m_targets & 0x10) && rec->m_equippedItemClass >= 0 && rec->m_equippedItemSubclass) {
    usable = false;
    CGItem_C *itemPtr = player->m_inventory.FindItemOfClass(rec->m_equippedItemClass, rec->m_equippedItemSubclass, 1);
    if (itemPtr) {
      int              ammoType = 0;
      const ItemStats *stats = g_itemDBCache.GetRecord(itemPtr->GetEntryID(), 0, 0, 0);
      if (stats) {
        ammoType = stats->m_ammunitionType;
      }

      if (checkAmmo && ammoType) {
        const ItemSubClassRec *subclassRec = SDBItemSubclassGetSubClassRec(6, ammoType);
        FATALASSERT(subclassRec);
        if (!(subclassRec->m_flags & 0x40)) {
          CGItem_C *quiverPtr = player->m_inventory.FindItemOfClass(11, 1 << ammoType, 18);
          if (!quiverPtr) {
            if (report) {
              Spell_C_SpellFailed(rec->m_ID, SPELL_FAILED_NEED_AMMO_POUCH, ammoType, -1);
            }
            return false;
          }

          FATALASSERT(quiverPtr->GetBag());
          FindAmmoData data;
          data.ammoType = ammoType;
          data.exoticAmmo = (rec->m_attributes & 8) != 0;
          if (!quiverPtr->GetBag()->FindItem(FindAmmoCallback, &data, 0)) {
            if (report) {
              Spell_C_SpellFailed(rec->m_ID, SPELL_FAILED_NEED_AMMO, ammoType, -1);
            }
            return false;
          }
        }
      }
      usable = true;
    } else if (report) {
      Spell_C_SpellFailed(rec->m_ID, SPELL_FAILED_EQUIPPED_ITEM_CLASS, rec->m_equippedItemClass, rec->m_equippedItemSubclass);
    }
  }

  return usable;
}

static BOOL FindAmmoCallback(const CGItem_C *item, LPVOID param) {
  FindAmmoData *data = (FindAmmoData *)param;
  if (item->GetClassID() == 6 && item->GetSubtypeID() == data->ammoType) {
    if (data->exoticAmmo) {
      if (item->IsExotic()) {
        return 1;
      }
    } else {
      if (!item->IsExotic()) {
        return 1;
      }
    }
  }
  return 0;
}

bool Spell_C_IsTargeting() {
  return s_needTargets != 0;
}

int Spell_C_GetTargettingSpell() {
  return s_needTargets ? s_spellCast.spellID : 0;
}

void Spell_C_StopTargeting() {
  if (Spell_C_IsTargeting()) {
    Spell_C_CancelSpell(true, true, SPELL_FAILED_ERROR);
  }
}

void Spell_C_CancelSpell(bool failed, bool notifyServer, SPELL_FAILED_REASON reason) {
  CGUnit_C *caster = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(s_spellCast.casterUnit, __FILE__, __LINE__));

  if (Spell_C_IsTargeting()) {
    s_needTargets = 0;
    CGSpellBook::UpdateSelection();
    CGActionBar::UpdateSelection();
  } else if (Spell_C_IsModal()) {
    if (notifyServer && !Spell_C_GetModalItem()) {
      CDataStore msg;
      msg.Put(CMSG_CANCEL_CAST);
      msg.Put(Spell_C_GetModalSpell());
      msg.Finalize();
      ClientServices_Send(&msg);
    }

    if (caster) {
      SpellVisualsHandleCastStop(Spell_C_GetModalSpell(), caster, 2, reason);
    }
  }

  Spell_C_SetModal(0, 0);
  if (s_spellWorldModel) {
    CWorld::ObjectDelete(s_spellWorldModel);
    s_spellWorldModel = 0;
  }
  CursorSetCursorMode(POINT_CURSOR);

  if (s_spellCast.casterUnit == ClntObjMgrGetActivePlayer()) {
    bool interrupted = reason == SPELL_FAILED_INTERRUPTED || reason == SPELL_FAILED_INTERRUPTED_COMBAT;
    if (failed) {
      FrameScript_SignalEvent(interrupted ? 318 : 317);
    } else {
      FrameScript_SignalEvent(316);
    }
    s_spellCast.casterUnit = 0;
  }
}

static void Spell_C_SetModal(int spellID, const CGItem_C *item) {
  if (spellID) {
    s_savedModalSpellID = s_modalSpellID;
    s_savedModalItemID = s_modalItemID;
    s_modalSpellID = spellID;
    s_modalItemID = item ? item->GetGUID() : 0;
  } else {
    s_modalSpellID = s_savedModalSpellID;
    s_modalItemID = s_savedModalItemID;
  }

  CGSpellBook::UpdateSelection();
  CGActionBar::UpdateSelection();
}

bool Spell_C_CastSpell(int spellID, const CGItem_C *item) {
  const SpellRec *spell = g_spellDB.GetRecord(spellID);
  if (!spell || (spell->m_attributes & 0x40)) {
    return false;
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return false;
  }

  if (player->GetChannelSpell()) {
    CDataStore msg;
    msg.Put(CMSG_CANCEL_CHANNELLING);
    msg.Put(player->GetChannelSpell());
    msg.Finalize();
    ClientServices_Send(&msg);
  }

  if (spell->m_effect[0] == 78) {
    player->OnAttackIconPressed();
    return false;
  }

  if (spell->m_effect[0] == 47) {
    if (!spell->m_effectMiscValue[0]) {
      const SkillLineAbilityRec *ability = SpellTableLookupAbility(player->GetRace(), player->GetClass(), spellID);
      const SkillLineRec        *skillLine = ability ? g_skillLineDB.GetRecord(ability->m_skillLine) : 0;
      if (!skillLine) {
        return false;
      }
      CGTradeSkillInfo::SetSkillLine(skillLine->m_ID);
      return false;
    }

    CGCraftInfo::SetCraftType((SPELL_CAST_UI_TYPE)spell->m_effectMiscValue[0]);
    return false;
  }

  if (spellID == s_modalSpellID) {
    SndInterfacePlayInterfaceSound("igPlayerInviteDecline");
    return false;
  }

  if (Spell_C_IsTargeting()) {
    Spell_C_SpellFailed(spell->m_ID, SPELL_FAILED_SPELL_IN_PROGRESS, -1, -1);
    return false;
  }

  BYTE playerCast = 1;
  if (item && !item->GetItemStaticFlag(ITEM_FLAG_PLAYERCAST)) {
    playerCast = 0;
  }

  if (Spell_C_IsModal()) {
    const SpellRec *srec = g_spellDB.GetRecord(s_modalSpellID);
    FATALASSERT(srec);
    if (!(srec->m_attributes & 0x404)) {
      Spell_C_SpellFailed(spell->m_ID, SPELL_FAILED_SPELL_IN_PROGRESS, -1, -1);
      return false;
    }
  }

  if (player->GetHealth() <= 0 && !(spell->m_attributes & 0x800000)) {
    Spell_C_SpellFailed(spell->m_ID, SPELL_FAILED_CASTER_DEAD, -1, -1);
    return false;
  }

  if (!Spell_C_HaveSpellTokens(player, spell, true) || !Spell_C_HaveEquippedSpellItems(player, spell, true, true)) {
    return false;
  }

  if ((spell->m_attributes & 2) && !player->m_inventory.GetItem(17)) {
    Spell_C_SpellFailed(spell->m_ID, SPELL_FAILED_EQUIPPED_ITEM, spell->m_equippedItemClass, spell->m_equippedItemSubclass);
    return false;
  }

  if ((spell->m_attributesEx & 0x500000) && !player->GetComboPoints()) {
    Spell_C_SpellFailed(spell->m_ID, SPELL_FAILED_NO_COMBO_POINTS, -1, -1);
    return false;
  }

  if (!player->CheckAndReportSpellInhibitFlags(spell, item) || !RangeCheckSelected(player, spell)) {
    return false;
  }

  if ((spell->m_attributes & 0x404 || spell->m_attributesEx & 0x200) && !player->IsInCombatMode() && !player->OnAttackIconPressed()) {
    return false;
  }

  if (item) {
    SndInterfacePlayItemSound(ITEMSOUND_USE, item);
  } else {
    SndInterfacePlayInterfaceSound(spell->m_attributes & 0x10 ? "GAMEABILITYACTIVATE" : "GAMESPELLACTIVATE");
  }

  player->PendingPrecastInterrupt(0);
  if ((spell->m_attributes & 0x100000) && !(spell->m_attributes & 0x404) && player->IsInCombatMode()) {
    player->SetCombatMode(0);
  }

  memset(&s_spellCast, 0, sizeof(s_spellCast));
  s_spellCast.spellID = spellID;
  s_spellCast.caster = item ? item->GetGUID() : player->GetGUID();
  s_spellCast.casterUnit = player->GetGUID();
  s_playerCast = playerCast;

  if ((player->GetSpellCastingTime(spellID) >= 0 || spell->m_attributes & 0x404) && (!item || (item->m_item->m_staticFlags & ITEM_FLAG_PLAYERCAST))) {
    Spell_C_SetModal(spellID, item);
  }

  UnitEffectPreloadSpellEffects(spellID);
  if (!Spell_C_TargetSpell(player, spell)) {
    if (s_needTargets & 0x80) {
      DWORDLONG target = CGGameUI::GetLockedTarget();
      s_needTargets = 0;
      if (target) {
        Spell_C_SpellFailed(spellID, SPELL_FAILED_BAD_TARGETS, -1, -1);
      } else {
        Spell_C_SpellFailed(spellID, SPELL_FAILED_BAD_IMPLICIT_TARGETS, -1, -1);
      }
      CGSpellBook::UpdateSelection();
      CGActionBar::UpdateSelection();
      return false;
    }

    CursorSetCursorMode(CAST_CURSOR);
    CursorModelSetSequence(CAST_ERROR_CURSOR);
    FATALASSERT(!s_spellWorldModel);
    s_spellWorldModelHousing = 0;
    s_spellWorldModelFacing = 0.0f;

    for (UINT effectIndex = 0; effectIndex < 3; ++effectIndex) {
      if (spell->m_effect[effectIndex] == 50 || spell->m_effect[effectIndex] == 76 || spell->m_effect[effectIndex] == 81) {
        if (spell->m_effect[effectIndex] == 81) {
          s_spellWorldModelHousing = 1;
        }

        const GameObjectStats_C *stats = g_gameObjectDBCache.GetRecord(
            spell->m_effectMiscValue[effectIndex], spell->m_ID | 0xB000000000000000ui64,
            (DBCACHECALLBACKPROC)GameObjectStatsCallback, (LPVOID)spell->m_ID
        );
        if (stats) {
          const GameObjectDisplayInfoRec *display = g_gameObjectDisplayInfoDB.GetRecord(stats->m_displayID);
          if (display) {
            NTempest::C3Vector pos = player->GetPosition();
            s_spellWorldModel = CWorld::ObjectCreate(display->m_modelName, pos, 0.0f, 0, 0, 0);
            CWorld::ObjectEnableCollision(s_spellWorldModel, 0);
          }
        }
        break;
      }
    }
  }
  CGSpellBook::UpdateSelection();
  CGActionBar::UpdateSelection();
  return true;
}

static bool Spell_C_TargetSpell(CGUnit_C *caster, const SpellRec *srec) {
  s_needTargets = (WORD)srec->m_targets;
  bool suppressTarget = false;

  switch (srec->m_implicitTargetA[0]) {
    case 1:
      if (s_needTargets & 0x400) {
        s_needTargets &= ~0x400;
      }
      break;
    case 35:
      s_needTargets |= 8;
      break;
    case 21:
      s_needTargets |= 0x100;
      break;
    case 6:
      s_needTargets |= 0x80;
      break;
    case 25:
      s_needTargets |= 2;
      break;
    case 23:
      s_needTargets |= 0x800;
      break;
    case 26:
      s_needTargets |= 0x4000;
      break;
    case 16:
      suppressTarget = true;
      break;
  }

  if ((s_needTargets & 0x2000) && s_spellTargetString[0]) {
    SStrPrintf(s_spellCast.targetString, sizeof(s_spellCast.targetString), "%s", s_spellTargetString);
    s_needTargets &= ~0x2000;
    s_spellCast.targets |= 0x2000;
    s_spellTargetString[0] = 0;
  }

  CGSpellBook::UpdateSelection();
  CGActionBar::UpdateSelection();
  if (!s_needTargets) {
    SendCast(&s_spellCast);
    return true;
  }
  if (suppressTarget) {
    return false;
  }

  return Spell_C_HandleSpriteClick(ClntObjMgrObjectPtr(CGGameUI::GetLockedTarget(), __FILE__, __LINE__));
}

static void SendCast(SpellCast *cast) {
  DWORDLONG castingItem = cast->caster != cast->casterUnit ? cast->caster : 0;

  if (s_spellWorldModel) {
    CWorld::ObjectDelete(s_spellWorldModel);
    s_spellWorldModel = 0;
  }
  CursorSetCursorMode(POINT_CURSOR);

  CGUnit_C *caster = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(cast->casterUnit, __FILE__, __LINE__));
  if (!caster) {
    return;
  }

  CDataStore castMsg;
  if (castingItem) {
    CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(castingItem, __FILE__, __LINE__));
    if (!item) {
      ConsoleWrite("Casting item not found", DEFAULT_COLOR);
      return;
    }

    CGObject_C *container = ClntObjMgrObjectPtr(item->GetContainedIn(), __FILE__, __LINE__);
    if (!container) {
      ConsoleWrite("Casting item's container not found", DEFAULT_COLOR);
      return;
    }

    int itemSlot = container->GetBag()->GetIndexOfObject(item->GetGUID());
    if (itemSlot < 0) {
      ConsoleWrite("Casting item not found in container", DEFAULT_COLOR);
      return;
    }

    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(item->GetOwner(), __FILE__, __LINE__));
    if (!player) {
      ConsoleWrite("Active player not found", DEFAULT_COLOR);
      return;
    }

    BYTE packSlot = player->FindSlotIndex(item->GetContainedIn());
    if (packSlot > 43 && packSlot != 0xFF) {
      ConsoleWrite("Object not in container belonging to active player", DEFAULT_COLOR);
      return;
    }

    const ItemStats *stats = g_itemDBCache.GetRecord(item->GetEntryID(), 0, 0, 0);
    if (!stats) {
      ConsoleWrite("Casting item doesn't have stats", DEFAULT_COLOR);
      return;
    }

    int spellIndex = -1;
    for (UINT i = 0; i < sizeof(stats->m_spellID) / sizeof(stats->m_spellID[0]); ++i) {
      if (cast->spellID == stats->m_spellID[i] && !stats->m_spellTrigger[i]) {
        spellIndex = i;
        break;
      }
    }
    if (spellIndex < 0) {
      ConsoleWrite("Casting item doesn't have spell used", DEFAULT_COLOR);
      return;
    }

    castMsg.Put(CMSG_USE_ITEM);
    castMsg.Put(packSlot);
    castMsg.Put((BYTE)itemSlot);
    castMsg.Put((BYTE)spellIndex);
  } else {
    castMsg.Put(CMSG_CAST_SPELL);
    castMsg.Put(cast->spellID);
  }

  SpellPutCastTargets(cast, &castMsg);
  castMsg.Finalize();
  ClientServices_Send(&castMsg);

  if (caster->GetGUID() == ClntObjMgrGetActivePlayer() && (s_playerCast || !caster->GetCastingSpell())) {
    SpellVisualsHandleCastStart(cast->spellID, *cast, caster, 1000000, 4000, 0);
  }

  if (!s_playerCast && cast->spellID == s_modalSpellID) {
    Spell_C_SetModal(0, 0);
  }

  const SpellRec *spell = g_spellDB.GetRecord(cast->spellID);
  if (spell->m_startRecoveryCategory || spell->m_startRecoveryTime) {
    s_spellHistory[0].AddHistory(
        cast->spellID, 0, OsGetAsyncTimeMs(), 0, 0, OsGetAsyncTimeMs(), 0, false, spell->m_startRecoveryCategory, spell->m_startRecoveryTime
    );
    CGActionBar::UpdateCooldowns();
    CGSpellBook::UpdateCooldowns();
  }
}

static bool RangeCheckSelected(CGPlayer_C *caster, const SpellRec *srec) {
  int targets;
  switch (srec->m_implicitTargetA[0]) {
    case 35:
      targets = 8;
      break;
    case 21:
      targets = 0x100;
      break;
    case 6:
      targets = 0x80;
      break;
    case 25:
      targets = 2;
      break;
    case 23:
      targets = 0x800;
      break;
    case 26:
      targets = 0x4000;
      break;
    default:
      return true;
  }

  CGObject_C *target = ClntObjMgrObjectPtr(CGGameUI::GetLockedTarget(), __FILE__, __LINE__);
  if (!target) {
    return true;
  }

  bool checkRange = false;
  switch (targets) {
    case 8:
      checkRange = target->IsA(TYPE_UNIT) && caster->IsUnitInGroup((CGUnit_C *)target);
      break;
    case 0x100:
      checkRange = target->IsA(TYPE_UNIT) && caster->CanAssist((CGUnit_C *)target);
      break;
    case 0x80:
      checkRange = target->IsA(TYPE_UNIT) && caster->CanAttack((CGUnit_C *)target);
      break;
    case 2:
      checkRange = true;
      break;
    case 0x800:
    case 0x4000:
      checkRange = target->IsA(TYPE_GAMEOBJECT);
      break;
  }

  if (checkRange && !RangeCheck(caster, target, srec->m_ID)) {
    return false;
  }
  return true;
}

inline float CGUnit::LinearDistanceSquared(const NTempest::C3Vector &position) const {
  return (GetPosition() - position).SquaredMag();
}

static bool RangeCheck(CGPlayer_C *caster, CGObject_C *target, int spellID) {
  float minRange;
  float maxRange;
  Spell_C_GetMinMaxRange(spellID, &minRange, &maxRange);

  float distance = caster->LinearDistanceSquared(target->GetPosition());
  if (distance < minRange * minRange || distance > maxRange * maxRange) {
    Spell_C_SpellFailed(spellID, SPELL_FAILED_OUT_OF_RANGE, -1, -1);
    return false;
  }

  return true;
}

static void GameObjectStatsCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted) {
  if ((int)arg != s_spellCast.spellID) {
    return;
  }
  const GameObjectStats_C *stats = g_gameObjectDBCache.GetRecord(id, 0, 0, 0);
  if (!stats) {
    return;
  }
  const GameObjectDisplayInfoRec *display = g_gameObjectDisplayInfoDB.GetRecord(stats->m_displayID);
  if (!display) {
    return;
  }
  CGObject_C *caster = ClntObjMgrObjectPtr(s_spellCast.casterUnit, __FILE__, __LINE__);
  if (!caster) {
    return;
  }

  NTempest::C3Vector pos = caster->GetPosition();
  s_spellWorldModel = CWorld::ObjectCreate(display->m_modelName, pos, 0.0f, 0, 0, 0);
  CWorld::ObjectEnableCollision(s_spellWorldModel, 0);
}

bool Spell_C_CastSpell(LPCSTR name) {
  if (!SStrCmpI(name, "none", 0x7FFFFFFF)) {
    Spell_C_CancelSpell(1, 1, SPELL_FAILED_ERROR);
    return false;
  }
  return Spell_C_CastSpell(Spell_C_GetSpellByName(name), 0);
}

bool Spell_C_HandleSpriteClick(const CSpriteClickEvent &evt) {
  return Spell_C_HandleSpriteClick(ClntObjMgrObjectPtr(evt.objectGUID, __FILE__, __LINE__));
}

bool Spell_C_HandleSpriteClick(CGObject_C *object) {
  if (!s_needTargets) {
    return false;
  }
  if (!object) {
    return false;
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);

  DWORDLONG guid = object->GetGUID();
  if (guid == s_spellCast.casterUnit) {
    const SpellRec *spell = g_spellDB.GetRecord(s_spellCast.spellID);
    if (spell->m_attributesEx & 0x80000) {
      return false;
    }
  }

  WORD oldNeedTargets = s_needTargets;
  WORD oldTargets = s_spellCast.targets;
  bool handled = false;

  if (object->IsA(TYPE_UNIT)) {
    CGUnit_C *unit = (CGUnit_C *)object;
    if (unit->GetHealth() <= 0 && !(oldNeedTargets & 0x400)) {
      handled = false;
    } else if (unit->GetHealth() > 0 && (oldNeedTargets & 0x400)) {
      handled = false;
    } else {
      if ((s_needTargets & 8) && player->IsUnitInGroup(unit)) {
        s_spellCast.targets |= 2;
        s_spellCast.unitTarget = object->GetGUID();
        s_needTargets &= ~8;
      } else if ((s_needTargets & 0x100) && player->CanAssist(unit)) {
        s_spellCast.targets |= 2;
        s_spellCast.unitTarget = object->GetGUID();
        s_needTargets &= ~0x100;
      } else if ((s_needTargets & 0x80) && player->CanAttack(unit)) {
        s_spellCast.targets |= 2;
        s_spellCast.unitTarget = object->GetGUID();
        s_needTargets &= ~0x80;
      } else if (s_needTargets & 2) {
        s_spellCast.unitTarget = object->GetGUID();
        s_spellCast.targets |= 2;
        s_needTargets &= ~2;
      } else {
        return handled;
      }

      if (unit->GetHealth() <= 0) {
        s_needTargets &= ~0x400;
      }
      handled = true;
    }
  } else if (object->IsA(TYPE_ITEM)) {
    if (oldNeedTargets & 0x4010) {
      s_spellCast.targets |= 0x10;
      s_needTargets &= ~0x4010;
      s_spellCast.itemTarget = guid;
      handled = true;
    }
  } else if (object->IsA(TYPE_GAMEOBJECT)) {
    if (oldNeedTargets & 0x4800) {
      s_spellCast.targets |= 0x800;
      s_needTargets &= ~0x4800;
      s_spellCast.unitTarget = guid;
      handled = true;
    }
  }

  if (handled) {
    if ((s_spellCast.targets & 2) && s_spellCast.unitTarget) {
      CGObject_C *target = ClntObjMgrObjectPtr(s_spellCast.unitTarget, __FILE__, __LINE__);
      if (target && !RangeCheck(player, target, s_spellCast.spellID)) {
        s_spellCast.unitTarget = 0;
        s_spellCast.targets = oldTargets;
        s_needTargets = oldNeedTargets;
        return false;
      }
    }

    if (!s_needTargets) {
      CGSpellBook::UpdateSelection();
      CGActionBar::UpdateSelection();
      SendCast(&s_spellCast);
    }
  }
  return handled;
}

bool Spell_C_CanTargetObject(const CGObject_C *objectPtr) {
  if ((s_needTargets & 0x4800) && objectPtr->IsA(TYPE_GAMEOBJECT) &&
      ((const CGGameObject_C *)objectPtr)->IsValidTargetForSpell(s_spellCast.caster, s_spellCast.spellID))
  {
    return true;
  }
  return false;
}

bool Spell_C_CanTargetObjects() {
  bool result = false;
  if (s_needTargets & 0x4800) {
    result = true;
  }
  return result;
}

bool Spell_C_CanTargetUnits() {
  bool result = false;
  if (s_needTargets & 0x58A) {
    result = true;
  }
  return result;
}

bool Spell_C_CanTargetMe() {
  bool result = false;
  if (s_needTargets & 0x50A) {
    const SpellRec *spell = g_spellDB.GetRecord(s_spellCast.spellID);
    if (!(spell->m_attributesEx & 0x80000)) {
      result = true;
    }
  }
  return result;
}

bool Spell_C_CanTargetParty() {
  bool result = false;
  if (s_needTargets & 0x408) {
    result = true;
  }
  return result;
}

bool Spell_C_CanTargetFriends() {
  bool result = false;
  if (s_needTargets & 0x500) {
    result = true;
  }
  return result;
}

bool Spell_C_CanTargetEnemies() {
  bool result = false;
  if (s_needTargets & 0x480) {
    result = true;
  }
  return result;
}

bool Spell_C_CanTargetDead() {
  bool result = false;
  if (s_needTargets & 0x400) {
    result = true;
  }
  return result;
}

bool Spell_C_CanTargetItems() {
  bool result = false;
  if (s_needTargets & 0x4010) {
    result = true;
  }
  return result;
}

bool Spell_C_HandleTerrainClick(const CTerrainClickEvent &evt) {
  if (!s_needTargets) {
    return 0;
  }

  bool handled = false;
  if (s_needTargets & 0x20) {
    s_spellCast.sourceLocation = evt.point;
    s_spellCast.targets |= 0x20;
    s_needTargets &= ~0x20;
    handled = true;
  } else if (s_needTargets & 0x40) {
    s_spellCast.destLocation = evt.point;
    s_spellCast.targets |= 0x40;
    s_needTargets &= ~0x40;
    handled = true;
  }

  CGSpellBook::UpdateSelection();
  CGActionBar::UpdateSelection();
  if (handled && !s_needTargets) {
    SendCast(&s_spellCast);
  }
  return handled;
}

bool Spell_C_CanTargetTerrain() {
  bool result = false;
  if (s_needTargets & 0x60) {
    result = true;
  }
  return result;
}

float Spell_C_GetSpellRadius() {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);

  const SpellRec       *spell = g_spellDB.GetRecord(s_spellCast.spellID);
  const SpellRadiusRec *radius = g_spellRadiusDB.GetRecord(spell->m_effectRadiusIndex[0]);
  float radius1 = (radius ? radius->m_radius : 0.0f) + (radius ? player->GetLevel() * radius->m_radiusPerLevel : 0.0f);
  radius = g_spellRadiusDB.GetRecord(spell->m_effectRadiusIndex[1]);
  float radius2 = (radius ? radius->m_radius : 0.0f) + (radius ? player->GetLevel() * radius->m_radiusPerLevel : 0.0f);
  return radius1 > radius2 ? radius1 : radius2;
}

bool Spell_C_HandleSpriteRay(const CSpriteClickEvent &evt, bool checkRange) {
  CGObject_C *object = ClntObjMgrObjectPtr(evt.objectGUID, __FILE__, __LINE__);
  if (!object) {
    return false;
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);

  if (object->GetGUID() == s_spellCast.casterUnit) {
    const SpellRec *spell = g_spellDB.GetRecord(s_spellCast.spellID);
    if (spell->m_attributesEx & 0x80000) {
      return false;
    }
  }

  bool valid = false;
  if (object->IsA(TYPE_UNIT)) {
    CGUnit_C *unit = (CGUnit_C *)object;
    if (!player) {
      return false;
    }

    if (unit->GetHealth() <= 0 && !(s_needTargets & 0x400)) {
      valid = false;
    } else if (unit->GetHealth() > 0 && (s_needTargets & 0x400)) {
      valid = false;
    } else if ((s_needTargets & 8) && player->IsUnitInGroup(unit)) {
      valid = true;
    } else if ((s_needTargets & 0x100) && player->CanAssist(unit)) {
      valid = true;
    } else if ((s_needTargets & 0x80) && player->CanAttack(unit)) {
      valid = true;
    } else if (s_needTargets & 0x62) {
      valid = true;
    }

    if (valid) {
      const SpellRec *spell = g_spellDB.GetRecord(s_spellCast.spellID);
      if (spell && spell->m_targetCreatureType) {
        if (!unit->GetCreatureType()) {
          valid = false;
        } else if (!(spell->m_targetCreatureType & (1 << (unit->GetCreatureType() - 1)))) {
          valid = false;
        }
      }
    }
  } else if (object->IsA(TYPE_ITEM)) {
    if (s_needTargets & 0x4010) {
      valid = true;
    }
  } else if (object->IsA(TYPE_GAMEOBJECT)) {
    if (s_needTargets & 0x4800) {
      valid = ((CGGameObject_C *)object)->IsValidTargetForSpell(s_spellCast.caster, s_spellCast.spellID);
    }
  }

  if (!valid) {
    return false;
  }

  if (!checkRange) {
    return true;
  }

  float distance = (player->CGUnit::GetPosition() - object->GetPosition()).SquaredMag();
  float minRange;
  float maxRange;
  Spell_C_GetMinMaxRange(s_spellCast.spellID, &minRange, &maxRange);
  return minRange * minRange <= distance && maxRange * maxRange >= distance;
}

bool Spell_C_HandleTerrainRay(const CTerrainClickEvent &evt, bool checkRange) {
  if (!Spell_C_CanTargetTerrain()) {
    return false;
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);
  if (!checkRange) {
    return true;
  }

  float distance = (player->CGUnit::GetPosition() - evt.point).SquaredMag();
  float minRange;
  float maxRange;
  Spell_C_GetMinMaxRange(s_spellCast.spellID, &minRange, &maxRange);
  return minRange * minRange <= distance && maxRange * maxRange >= distance;
}

bool Spell_C_WaitingForStringInput() {
  return (s_needTargets >> 13) & 1;
}

BOOL Spell_C_TargetTradeItem(int tradeIndex) {
  if (!(s_needTargets & 0x4010) || tradeIndex < 0 || tradeIndex >= 8 || !CGTradeInfo::GetTargetTradeItem(tradeIndex)) {
    return 0;
  }
  s_spellCast.targets |= 0x1000;
  s_needTargets &= ~0x4010;
  s_spellCast.itemTarget = tradeIndex;
  CGSpellBook::UpdateSelection();
  CGActionBar::UpdateSelection();
  if (!s_needTargets) {
    SendCast(&s_spellCast);
  }
  return 1;
}

UINT Spell_C_WorldObjectCursor() {
  return s_spellWorldModel;
}

float Spell_C_WorldObjectFacing() {
  if (!s_spellWorldModelHousing) {
    CGObject_C *caster = ClntObjMgrObjectPtr(s_spellCast.casterUnit, __FILE__, __LINE__);
    if (caster) {
      s_spellWorldModelFacing = caster->GetFacing();
    }
  }

  return s_spellWorldModelFacing;
}

bool Spell_C_WorldObjectHousing() {
  return s_spellWorldModelHousing;
}

void Spell_C_WorldObjectRotate() {
  s_spellWorldModelFacing += 1.5707964f;
  if (s_spellWorldModelFacing >= 6.2831855f) {
    s_spellWorldModelFacing -= 6.2831855f;
  }
}

static int Script_SpellIsTargeting(lua_State *L) {
  Spell_C_IsTargeting() ? lua_pushnumber(L, 1.0) : lua_pushnil(L);
  return 1;
}

static int Script_SpellCanTargetUnit(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: SpellCanTargetUnit(\"unit\")");
    return 0;
  }
  DWORDLONG guid = Script_GetGUIDFromName(lua_tostring(L, 1));
  if (guid) {
    CSpriteClickEvent evt;
    evt.objectGUID = guid;
    if (Spell_C_HandleSpriteRay(evt, true)) {
      lua_pushnumber(L, 1.0);
      return 1;
    }
  }
  lua_pushnil(L);
  return 1;
}

static int Script_SpellTargetUnit(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: SpellCanTargetUnit(\"unit\")");
    return 0;
  }
  DWORDLONG guid = Script_GetGUIDFromName(lua_tostring(L, 1));
  if (guid) {
    CSpriteClickEvent evt;
    evt.objectGUID = guid;
    Spell_C_HandleSpriteClick(evt);
  }
  return 0;
}

static int Script_SpellStopTargeting(lua_State *L) {
  BOOL targeting = Spell_C_IsTargeting();
  Spell_C_StopTargeting();
  targeting ? lua_pushnumber(L, 1.0) : lua_pushnil(L);
  return 1;
}

static FrameScript_Method s_ScriptFunctions[4] = {
    {  "SpellIsTargeting",   Script_SpellIsTargeting},
    {"SpellCanTargetUnit", Script_SpellCanTargetUnit},
    {   "SpellTargetUnit",    Script_SpellTargetUnit},
    {"SpellStopTargeting", Script_SpellStopTargeting}
};

void SpellRegisterScriptFunctions() {
  for (UINT i = 0; i < 4; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void SpellUnregisterScriptFunctions() {
  for (UINT i = 0; i < 4; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}

void Spell_C_CancelCombatSpell() {
  if (s_modalSpellID) {
    const SpellRec *spell = g_spellDB.GetRecord(s_modalSpellID);
    if (spell && (spell->m_attributes & 0x404)) {
      Spell_C_CancelSpell(false, true, SPELL_FAILED_ERROR);
    }
  }

  if (s_modalSpellID) {
    const SpellRec *spell = g_spellDB.GetRecord(s_modalSpellID);
    if (spell && (spell->m_attributes & 0x404)) {
      Spell_C_CancelSpell(false, true, SPELL_FAILED_ERROR);
    }
  }

  if (s_savedModalSpellID) {
    const SpellRec *spell = g_spellDB.GetRecord(s_savedModalSpellID);
    if (spell && (spell->m_attributes & 0x404)) {
      if (!s_savedModalItemID) {
        CDataStore msg;
        msg.Put(CMSG_CANCEL_CAST);
        msg.Put(s_savedModalSpellID);
        msg.Finalize();
        ClientServices_Send(&msg);
      }
      s_savedModalSpellID = 0;
      s_savedModalItemID = 0;
    }
  }
}

void Spell_C_CancelAura(int spellID) {
  const SpellRec *spell = g_spellDB.GetRecord(spellID);
  if (spell->m_attributesEx & 0x2000) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      player->ToggleFarSight();
    }
    if (!(spell->m_attributesEx & 4)) {
      return;
    }
  }

  CDataStore msg;
  msg.Put(CMSG_CANCEL_AURA);
  msg.Put(spellID);
  msg.Finalize();
  ClientServices_Send(&msg);
}

static UINT s_displayPowerMods[4] = {1, 10, 1, 1};

UINT Spell_C_GetPowerDisplayMod(POWER_TYPE type) {
  return type < 0 ? 1 : s_displayPowerMods[type];
}

void Spell_C_Initialize() {
  ClientServices_SetMessageHandler(SMSG_CAST_RESULT, CastResultHandler, 0);
  ClientServices_SetMessageHandler(SMSG_SPELL_START, SpellStartHandler, 0);
  ClientServices_SetMessageHandler(SMSG_SPELL_GO, SpellStartHandler, 0);
  ClientServices_SetMessageHandler(SMSG_SPELL_FAILURE, SpellFailedHandler, 0);
  ClientServices_SetMessageHandler(SMSG_PET_CAST_FAILED, PetSpellFailedHandler, 0);
  ClientServices_SetMessageHandler(SMSG_SPELL_COOLDOWN, SpellCooldownHandler, 0);
  ClientServices_SetMessageHandler(SMSG_ITEM_COOLDOWN, ItemCooldownHandler, 0);
  ClientServices_SetMessageHandler(SMSG_COOLDOWN_EVENT, CooldownEvent, 0);
  ClientServices_SetMessageHandler(SMSG_CLEAR_COOLDOWN, CooldownEvent, 0);
  ClientServices_SetMessageHandler(SMSG_COOLDOWN_CHEAT, CooldownCheat, 0);
  ClientServices_SetMessageHandler(SMSG_PET_TAME_FAILURE, PetTameFailure, 0);
  ClientServices_SetMessageHandler(SMSG_SPELL_DELAYED, SpellDelayed, 0);
  ClientServices_SetMessageHandler(MSG_CHANNEL_START, SpellChannelStart, 0);
  ClientServices_SetMessageHandler(MSG_CHANNEL_UPDATE, SpellChannelUpdate, 0);
  ClientServices_SetMessageHandler(MSG_ADD_DYNAMIC_TARGET, SpellAddDynamicTarget, 0);
  ClientServices_SetMessageHandler(SMSG_PLAY_SPELL_VISUAL, PlaySpellVisualKit, 0);

  s_needTargets = 0;
  s_modalSpellID = 0;
  s_modalItemID = 0;
  s_savedModalSpellID = 0;
  s_savedModalItemID = 0;

  ConsoleCommandRegister("cast", CCommand_Cast, GAME, "Cast spell <spellname");
  ConsoleCommandRegister("learn", CCommand_Learn, DEBUG, "Learn a spell (or -1 for all spells)");
  ConsoleCommandRegister("cooldown", CCommand_Cooldown, DEBUG, "Toggle cooldowns");
  ConsoleCommandRegister("cooldownPet", CCommand_CooldownPet, DEBUG, "Toggle cooldowns for your pet");
  ConsoleCommandRegister("useskill", CCommand_UseSkill, DEBUG, "Simulate usage of a spell without actually casting, to test skill rank-ups");
  ConsoleCommandRegister("setskill", CCommand_SetSkill, DEBUG, "Set skill to a specific level");
  ConsoleCommandRegister("cancelaura", CCommand_CancelAura, GAME, "Cancel an aura given the aura's index (not spell ID)");
  ConsoleCommandRegister("spellstring", CCommand_SpellString, GAME, "specify a spell string. Eventually there will be an editbox.");
}

static BOOL CCommand_Cast(LPCSTR, LPCSTR arguments) {
  Spell_C_CastSpell(arguments);
  return 1;
}

static BOOL CastResultHandler(LPVOID, NETMESSAGE, DWORD, CDataStore *msg) {
  int  spellID;
  BYTE status;
  BYTE reason = 0;
  msg->Get(spellID);
  msg->Get(status);
  if (status == 2) {
    msg->Get(reason);
    int arg1 = -1;
    int arg2 = -1;
    if (msg->Tell() < msg->Size()) {
      msg->Get(arg1);
    }
    if (msg->Tell() < msg->Size()) {
      msg->Get(arg2);
    }
    FATALASSERT(msg->Tell() >= msg->Size());
    Spell_C_SpellFailed(spellID, reason, arg1, arg2);
  }

  CGUnit_C *player = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    SpellVisualsHandleCastStop(spellID, player, status, reason);
  }

  if (spellID == s_savedModalSpellID) {
    s_savedModalSpellID = 0;
    s_savedModalItemID = 0;
  }
  if (spellID == s_modalSpellID) {
    Spell_C_CancelSpell(0, 0, SPELL_FAILED_ERROR);
    const SpellRec *spell = g_spellDB.GetRecord(spellID);
    if (spell && spell->m_modalNextSpell) {
      Spell_C_CastSpell(spell->m_modalNextSpell, 0);
    }
  } else if (!Spell_C_IsModal() && status != 2) {
    FrameScript_SignalEvent(0x13C);
  }
  return 1;
}

static BOOL SpellDelayed(LPVOID, NETMESSAGE, DWORD, CDataStore *msg) {
  DWORDLONG caster;
  DWORD     delay;
  msg->Get(caster);
  msg->Get(delay);
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(caster, __FILE__, __LINE__));
  if (unit) {
    unit->SpellDelayed(delay);
  }
  if (caster == ClntObjMgrGetActivePlayer()) {
    FrameScript_SignalEvent(0x13F, "%d", delay);
  }
  return 1;
}

static BOOL SpellChannelStart(LPVOID, NETMESSAGE, DWORD, CDataStore *msg) {
  int   spellID;
  DWORD time;
  msg->Get(spellID);
  msg->Get(time);
  if ((int)time > 0) {
    const SpellRec *spell = g_spellDB.GetRecord(spellID);
    LPCSTR          text = spell && (spell->m_attributesEx & 0x20000000) ? spell->m_name_lang[CURRENT_LANGUAGE]
                                                                         : FrameScript_GetText("CHANNELING", -1, GENDER_NOT_APPLICABLE);
    FrameScript_SignalEvent(0x140, "%d%s", time, text);
  }
  return 1;
}

static BOOL SpellChannelUpdate(LPVOID, NETMESSAGE, DWORD, CDataStore *msg) {
  DWORD time;
  msg->Get(time);
  FrameScript_SignalEvent(0x141, "%d", time);
  return 1;
}

static BOOL SpellAddDynamicTarget(LPVOID, NETMESSAGE, DWORD, CDataStore *msg) {
  DWORDLONG dynObjGUID;
  DWORDLONG targetGUID;
  msg->Get(dynObjGUID);
  msg->Get(targetGUID);
  ClntObjMgrObjectPtr(dynObjGUID, __FILE__, __LINE__);
  ClntObjMgrObjectPtr(targetGUID, __FILE__, __LINE__);
  return 1;
}

static BOOL SpellStartHandler(LPVOID, NETMESSAGE msgID, DWORD, CDataStore *msg) {
  DWORDLONG casterGUID;
  DWORDLONG casterUnit;
  int       spellID;
  msg->Get(casterGUID);
  msg->Get(casterUnit);
  msg->Get(spellID);
  if (msgID == SMSG_SPELL_START) {
    SpellStart(casterGUID, casterUnit, spellID, msg);
  } else {
    SpellGo(casterGUID, casterUnit, spellID, msg);
  }
  return 1;
}

inline void CGGameUI::TargetIfNone(const DWORDLONG &target) {
  if (!m_lockedTarget) {
    Target(target, 0);
  }
}

static void SpellStart(DWORDLONG casterGUID, DWORDLONG casterUnit, int spellID, CDataStore *msg) {
  WORD spellCastFlags;
  UINT castDelay;
  msg->Get(spellCastFlags);
  UnitEffectPreloadSpellEffects(spellID);
  msg->Get(castDelay);

  SpellCast cast;
  SpellGetCastTargets(&cast, msg);

  int ammoDisplayID;
  int ammoInventoryType;
  if (spellCastFlags & 0x10) {
    msg->Get(ammoDisplayID);
    msg->Get(ammoInventoryType);
  } else {
    ammoDisplayID = 0;
    ammoInventoryType = 0;
  }
  FATALASSERT(msg->IsRead());

  const SpellRec *srec = g_spellDB.GetRecord(spellID);
  if (!srec) {
    return;
  }
  CGUnit_C *caster = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(casterUnit, __FILE__, __LINE__));
  if (!caster) {
    return;
  }
  FATALASSERT(caster->IsA(TYPE_UNIT));
  caster->ClearRangedStandTimer();
  if (ammoDisplayID) {
    caster->SetAmmoDisplay(ammoDisplayID, ammoInventoryType);
  }

  if (caster->GetGUID() != ClntObjMgrGetActivePlayer()) {
    SpellVisualsHandleCastStart(spellID, cast, caster, castDelay, castDelay, (spellCastFlags & 1) != 0);
    if ((cast.targets & 2) && cast.unitTarget && cast.unitTarget == ClntObjMgrGetActivePlayer()) {
      CGUnit_C *player = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (player && !caster->CanAssist(player)) {
        CGGameUI::TargetIfNone(caster->GetGUID());
      }
    }
  } else {
    DWORDLONG target;
    if ((srec->m_attributes & 0x400000) && (cast.targets & 2) && cast.unitTarget) {
      target = cast.unitTarget;
    } else {
      target = CGGameUI::GetLockedTarget();
    }
    if (srec->m_attributes & 0x400000) {
      caster->SaveTrackingTarget(target, TRACKTYPE_SPELLPRECAST, 0);
    }
    if (castDelay > 0) {
      FrameScript_SignalEvent(0x13B, "%s%d", srec->m_name_lang[CURRENT_LANGUAGE], castDelay);
    }
  }
}

static void SpellGo(const DWORDLONG &casterGUID, const DWORDLONG &casterUnit, int spellID, CDataStore *msg) {
  WORD spellCastFlags;
  BYTE count;
  msg->Get(spellCastFlags);
  msg->Get(count);

  TSStackArray<DWORDLONG> targets(_alloca(count * sizeof(DWORDLONG)), count, count);
  UINT                    i;
  for (i = 0; i < count; ++i) {
    msg->Get(targets[i]);
    if (!(spellCastFlags & 1)) {
      UnitCombatLogCastGo(spellID, casterUnit, targets[i]);
    }
  }

  msg->Get(count);
  TSStackArray<DWORDLONG>   missTargets(_alloca(count * sizeof(DWORDLONG)), count, count);
  TSStackArray<MISS_REASON> missReasons(_alloca(count * sizeof(MISS_REASON)), count, count);
  for (i = 0; i < count; ++i) {
    BYTE reason = 0;
    msg->Get(reason);
    missReasons[i] = (MISS_REASON)reason;
    msg->Get(missTargets[i]);
  }

  SpellCast cast;
  SpellGetCastTargets(&cast, msg);

  int ammoDisplayID;
  int ammoInventoryType;
  if (spellCastFlags & 0x10) {
    msg->Get(ammoDisplayID);
    msg->Get(ammoInventoryType);
  } else {
    ammoDisplayID = 0;
    ammoInventoryType = 0;
  }
  FATALASSERT(msg->IsRead());

  const SpellRec *srec = g_spellDB.GetRecord(spellID);
  if (!srec) {
    return;
  }

  CGUnit_C *caster = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(casterUnit, __FILE__, __LINE__));
  if (!caster) {
    CGObject_C *casterObject = ClntObjMgrObjectPtr(casterGUID, __FILE__, __LINE__);
    if (casterObject && casterObject->IsA(TYPE_GAMEOBJECT)) {
      SpellVisualsHandleSpellStart(spellID, cast, (CGGameObject_C *)casterObject, targets, (spellCastFlags & 8) != 0, true);
      if (missTargets.Count() > 0) {
        SpellVisualsHandleSpellStart(spellID, cast, (CGGameObject_C *)casterObject, missTargets, (spellCastFlags & 8) != 0, false);
      }
    }
    return;
  }

  FATALASSERT(caster->IsA(TYPE_UNIT));
  DWORD currTime = OsGetAsyncTimeMs();
  bool      needsEvent = (srec->m_attributes & 0x2000000) != 0;
  if (srec->m_attributes & 2) {
    caster->SetRangedStandTimer();
  }
  SpellVisualsHandleSpellStartHits(spellID, cast, caster, targets, ammoDisplayID, ammoInventoryType, spellCastFlags);
  if (missTargets.Count() > 0) {
    SpellVisualsHandleSpellStartMisses(spellID, cast, caster, missTargets, missReasons, ammoDisplayID, ammoInventoryType, spellCastFlags);
  }

  if (caster->GetGUID() == ClntObjMgrGetActivePlayer()) {
    UINT effect;
    for (effect = 0; effect < 3; ++effect) {
      if (srec->m_effect[effect] == 33 || srec->m_effect[effect] == 59) {
        if (targets.Count() == 1) {
          CGObject_C *target = ClntObjMgrObjectPtr(targets[0], __FILE__, __LINE__);
          if (target && target->IsA(TYPE_GAMEOBJECT) && ((CGGameObject_C *)target)->GetType() == 3) {
            ((CGPlayer_C *)caster)->OnLootGameObject(targets[0], true);
          }
        }
        break;
      }
    }

    int         itemID = 0;
    CGObject_C *unitObject = ClntObjMgrObjectPtr(casterGUID, __FILE__, __LINE__);
    if (unitObject && unitObject->IsA(TYPE_ITEM)) {
      itemID = unitObject->GetEntryID();
    }

    if (casterGUID == casterUnit) {
      s_spellHistory[0].AddHistory(
          spellID, 0, currTime, srec->m_recoveryTime, srec->m_category, currTime, srec->m_categoryRecoveryTime, needsEvent, 0, 0
      );
      CGActionBar::UpdateCooldowns();
      CGSpellBook::UpdateCooldowns();
    } else if (itemID) {
      const ItemStats *stats = g_itemDBCache.GetRecord(itemID, casterGUID, ItemStatsCooldownCallback, (LPVOID)1);
      if (!stats) {
        SetItemCooldown(itemID, spellID, currTime, needsEvent);
        return;
      }

      UINT index;
      for (index = 0; index < 5; ++index) {
        if (stats->m_spellID[index] == spellID) {
          break;
        }
      }
      if (index < 5) {
        UINT recoveryTime = stats->m_spellCooldown[index] < 0 ? srec->m_recoveryTime : stats->m_spellCooldown[index];
        int  category = stats->m_spellCategory[index] <= 0 ? srec->m_category : stats->m_spellCategory[index];
        UINT categoryRecoveryTime = stats->m_spellCategoryCooldown[index] < 0 ? srec->m_categoryRecoveryTime : stats->m_spellCategoryCooldown[index];
        s_spellHistory[0].AddHistory(spellID, itemID, currTime, recoveryTime, category, currTime, categoryRecoveryTime, needsEvent, 0, 0);
      }
      CGActionBar::UpdateCooldowns();
      CGSpellBook::UpdateCooldowns();
      CGContainerInfo::UpdateCooldowns();
    }
  } else {
    if (caster->GetControlGUID() == ClntObjMgrGetActivePlayer()) {
      s_spellHistory[1].AddHistory(
          spellID, 0, currTime, srec->m_recoveryTime, srec->m_category, currTime, srec->m_categoryRecoveryTime, needsEvent,
          srec->m_startRecoveryCategory, srec->m_startRecoveryTime
      );
      CGPetInfo::UpdateCooldowns();
    }
  }

  static DWORD s_cleanupTime;
  if ((long)(currTime - s_cleanupTime) >= 0) {
    s_cleanupTime = currTime + 120000;
    for (i = 0; i < 2; ++i) {
      s_spellHistory[i].GarbageCollect(currTime);
    }
  }
}

static void SetItemCooldown(int itemID, int spellID, DWORD startTime, bool needsEvent) {
  HASHKEY_NONE          key;
  ITEMCOOLDOWNHASHNODE *cooldown = s_itemCooldowns.Ptr(itemID, key);
  if (!cooldown) {
    cooldown = s_itemCooldowns.New(itemID, key, 0, 0);
  }
  cooldown->spellID = spellID;
  cooldown->startTime = startTime;
  cooldown->needsEvent = needsEvent;
}

static void ItemStatsCooldownCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted) {
  HASHKEY_NONE          key;
  ITEMCOOLDOWNHASHNODE *cooldown = s_itemCooldowns.Ptr(id, key);
  if (!cooldown) {
    return;
  }

  const ItemStats *stats = g_itemDBCache.GetRecord(id, 0, 0, 0);
  if (stats) {
    UINT index;
    for (index = 0; index < NUM_ITEM_SPELLS; ++index) {
      if (stats->m_spellID[index] == cooldown->spellID) {
        break;
      }
    }
    if (index < NUM_ITEM_SPELLS) {
      const SpellRec *srec = g_spellDB.GetRecord(cooldown->spellID);
      FATALASSERT(srec);
      UINT selfCooldown = stats->m_spellCooldown[index] < 0 ? srec->m_recoveryTime : stats->m_spellCooldown[index];
      int  category = 0;
      UINT categoryRecoveryTime = 0;
      if (arg) {
        category = stats->m_spellCategory[index] <= 0 ? srec->m_category : stats->m_spellCategory[index];
        categoryRecoveryTime = stats->m_spellCategoryCooldown[index] < 0 ? srec->m_categoryRecoveryTime : stats->m_spellCategoryCooldown[index];
      }
      if (selfCooldown || categoryRecoveryTime) {
        s_spellHistory[0].AddHistory(
            cooldown->spellID, id, cooldown->startTime, selfCooldown, category, cooldown->startTime, categoryRecoveryTime, cooldown->needsEvent, 0, 0
        );
      }
    }
    CGActionBar::UpdateCooldowns();
    CGSpellBook::UpdateCooldowns();
    CGContainerInfo::UpdateCooldowns();
  }
  s_itemCooldowns.Delete(cooldown);
}

static BOOL SpellFailedHandler(LPVOID, NETMESSAGE, DWORD, CDataStore *msg) {
  DWORDLONG casterGUID;
  int       spellID;
  BYTE      reason;
  msg->Get(casterGUID);
  msg->Get(spellID);
  msg->Get(reason);

  CGUnit_C *caster = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(casterGUID, __FILE__, __LINE__));
  if (caster) {
    const SpellRec *spell = g_spellDB.GetRecord(spellID);
    if (spell && (spell->m_attributes & 2)) {
      caster->SetRangedStandTimer();
    }
    SndInterfacePlaySpellFizzleSound(spellID, caster);
    SpellVisualsHandleCastStop(spellID, caster, 2, reason);
  }
  return 1;
}

static BOOL PetSpellFailedHandler(LPVOID, NETMESSAGE, DWORD, CDataStore *msg) {
  int  spellID;
  BYTE reason;
  msg->Get(spellID);
  msg->Get(reason);
  const SpellRec *spell = g_spellDB.GetRecord(spellID);
  if (spell) {
    switch (reason) {
      case SPELL_FAILED_NOT_READY:
        CGGameUI::DisplayError((spell->m_attributes & 0x10) ? GERR_ABILITY_COOLDOWN : GERR_SPELL_COOLDOWN);
        break;
      case SPELL_FAILED_NO_POWER: {
        static const GAME_ERROR_TYPE s_gerrEnums[4] = {GERR_OUT_OF_MANA, GERR_OUT_OF_RAGE, GERR_OUT_OF_FOCUS, GERR_OUT_OF_ENERGY};
        if (spell->m_powerType == -2) {
          CGGameUI::DisplayError(GERR_OUT_OF_HEALTH);
        } else {
          CGGameUI::DisplayError(s_gerrEnums[spell->m_powerType]);
        }
        break;
      }
      case SPELL_FAILED_OUT_OF_RANGE:
        CGGameUI::DisplayError(GERR_SPELL_OUT_OF_RANGE);
        break;
      default:
        CGGameUI::DisplayError(GERR_SPELL_FAILED_S, FrameScript_GetText(GetStringReason(reason), -1, GENDER_NOT_APPLICABLE));
        break;
    }
  }
  return 1;
}

static BOOL SpellCooldownHandler(LPVOID, NETMESSAGE, DWORD eventTime, CDataStore *msg) {
  int       spellID;
  DWORDLONG guid;
  WORD      recoveryTime;
  msg->Get(spellID);
  msg->Get(guid);
  msg->Get(recoveryTime);

  BOOL isPet;
  if (guid == ClntObjMgrGetActivePlayer()) {
    isPet = 0;
  } else if (guid == CGPetInfo::GetPet()) {
    isPet = 1;
  } else {
    return 1;
  }

  const SpellRec *spell = g_spellDB.GetRecord(spellID);
  if (spell) {
    bool needsEvent = (spell->m_attributes & 0x02000000) != 0;
    s_spellHistory[isPet].AddHistory(
        spellID, 0, eventTime, recoveryTime ? recoveryTime : spell->m_recoveryTime, spell->m_category, eventTime, spell->m_categoryRecoveryTime,
        needsEvent, spell->m_startRecoveryCategory, spell->m_startRecoveryTime
    );
  }

  if (isPet) {
    CGPetInfo::UpdateCooldowns();
  } else {
    CGActionBar::UpdateCooldowns();
    CGSpellBook::UpdateCooldowns();
  }
  return 1;
}

static BOOL ItemCooldownHandler(LPVOID, NETMESSAGE, DWORD eventTime, CDataStore *msg) {
  DWORDLONG itemGUID;
  int       spellID;
  msg->Get(itemGUID);
  msg->Get(spellID);

  const SpellRec *spell = g_spellDB.GetRecord(spellID);
  if (spell) {
    CGObject_C *object = ClntObjMgrObjectPtr(itemGUID, __FILE__, __LINE__);
    if (object && object->IsA(TYPE_ITEM)) {
      s_spellHistory[0].AddHistory(spellID, object->GetEntryID(), eventTime, 30000, 0, 0, 0, false, 0, 0);
    }
  }
  CGActionBar::UpdateCooldowns();
  CGSpellBook::UpdateCooldowns();
  CGContainerInfo::UpdateCooldowns();
  return 1;
}

static BOOL CooldownEvent(LPVOID, NETMESSAGE msgID, DWORD timeReceived, CDataStore *msg) {
  int       spellID;
  DWORDLONG guid;
  msg->Get(spellID);
  msg->Get(guid);

  BOOL isPet;
  if (guid == ClntObjMgrGetActivePlayer()) {
    isPet = 0;
  } else if (guid == CGPetInfo::GetPet()) {
    isPet = 1;
  } else {
    return 1;
  }

  if (msgID == SMSG_COOLDOWN_CHEAT) {
    Spell_C_ClearCooldowns(isPet);
  } else {
    Spell_C_CooldownEventTriggered(spellID, timeReceived, isPet, msgID == SMSG_CLEAR_COOLDOWN);
  }
  return 1;
}

static void Spell_C_CooldownEventTriggered(int spellID, DWORD receivedTime, BOOL isPet, int clear) {
  s_spellHistory[isPet].RemoveHold(spellID, receivedTime, clear != 0);
  if (isPet) {
    CGPetInfo::UpdateCooldowns();
  } else {
    CGActionBar::UpdateCooldowns();
    CGSpellBook::UpdateCooldowns();
    CGContainerInfo::UpdateCooldowns();
  }
}

static void Spell_C_ClearCooldowns(BOOL isPet) {
  s_spellHistory[isPet].ClearHistory();
  if (isPet) {
    CGPetInfo::UpdateCooldowns();
  } else {
    CGActionBar::UpdateCooldowns();
    CGSpellBook::UpdateCooldowns();
    CGContainerInfo::UpdateCooldowns();
  }
}

static BOOL CooldownCheat(LPVOID, NETMESSAGE, DWORD, CDataStore *msg) {
  DWORDLONG guid;
  msg->Get(guid);
  if (guid == ClntObjMgrGetActivePlayer()) {
    Spell_C_ClearCooldowns(0);
  } else if (guid == CGPetInfo::GetPet()) {
    Spell_C_ClearCooldowns(1);
  }
  return 1;
}

static BOOL PetTameFailure(LPVOID, NETMESSAGE, DWORD, CDataStore *msg) {
  BYTE reason;
  msg->Get(reason);

  LPCSTR token;
  switch (reason) {
    case 1:
      token = "PETTAME_INVALIDCREATURE";
      break;
    case 2:
      token = "PETTAME_TOOMANY";
      break;
    case 3:
      token = "PETTAME_CREATUREALREADYOWNED";
      break;
    case 4:
      token = "PETTAME_NOTTAMEABLE";
      break;
    case 5:
      token = "PETTAME_ANOTHERSUMMONACTIVE";
      break;
    case 6:
      token = "PETTAME_UNITSCANTTAME";
      break;
    case 7:
      token = "PETTAME_NOPETAVAILABLE";
      break;
    case 8:
      token = "PETTAME_INTERNALERROR";
      break;
    case 9:
      token = "PETTAME_TOOHIGHLEVEL";
      break;
    default:
      token = "PETTAME_UNKNOWNERROR";
      break;
  }

  char   message[128];
  LPCSTR text = FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE);
  SStrCopy(message, text, sizeof(message));
  CGGameUI::DisplayError(GERR_TAME_FAILED, message);
  return 1;
}

static BOOL PlaySpellVisualKit(LPVOID, NETMESSAGE, DWORD, CDataStore *msg) {
  DWORDLONG target;
  UINT      id;
  msg->Get(target);
  msg->Get(id);
  CGObject_C *object = ClntObjMgrObjectPtr(target, __FILE__, __LINE__);
  if (object) {
    SpellVisualsPlayKit((CGUnit_C *)object, id);
  }
  return 1;
}

static BOOL CCommand_Learn(LPCSTR command, LPCSTR arguments) {
  int spellID;
  if (isdigit(*arguments)) {
    spellID = SStrToInt(arguments);
    if (spellID <= 0) {
      return 1;
    }
  } else {
    if (!SStrCmpI(arguments, "all", 0x7FFFFFFF)) {
      ConsolePrintf("meh.");
      return 1;
    }
    spellID = Spell_C_GetSpellByName(arguments);
  }
  if (spellID > 0) {
    CDataStore msg;
    msg.Put(CMSG_LEARN_SPELL);
    msg.Put(spellID);
    msg.Finalize();
    ClientServices_Send(&msg);
  }
  return 1;
}

static BOOL CCommand_Cooldown(LPCSTR command, LPCSTR arguments) {
  CDataStore msg;
  msg.Put(CMSG_COOLDOWN_CHEAT);
  msg.Put(ClntObjMgrGetActivePlayer());
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static BOOL CCommand_CooldownPet(LPCSTR command, LPCSTR arguments) {
  CDataStore msg;
  msg.Put(CMSG_COOLDOWN_CHEAT);
  msg.Put(CGPetInfo::GetPet());
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static BOOL CCommand_UseSkill(LPCSTR command, LPCSTR arguments) {
  int offset = 0;
  int id;
  if (isdigit(*arguments)) {
    id = SStrToInt(arguments);
    while (arguments[offset] && isdigit(arguments[offset])) {
      ++offset;
    }
    while (arguments[offset] && isspace(arguments[offset])) {
      ++offset;
    }
  } else {
    while (arguments[offset] && !isdigit(arguments[offset])) {
      ++offset;
    }
    char spellName[1024];
    SStrCopy(spellName, arguments, min(offset, 1023));
    spellName[min(offset - 1, 1023)] = 0;
    id = Spell_C_GetSpellByName(spellName);
  }

  int level = SStrToInt(arguments + offset);
  if (id < 0) {
    ConsolePrintf("Unknown spell %s", arguments);
  } else {
    CDataStore msg;
    msg.Put(CMSG_USE_SKILL_CHEAT);
    msg.Put(id);
    msg.Put(level);
    msg.Finalize();
    ClientServices_Send(&msg);
  }
  return 1;
}

static BOOL CCommand_SetSkill(LPCSTR command, LPCSTR arguments) {
  if (isdigit(*arguments)) {
    int level = SStrToInt(arguments);
    while (*arguments && (isdigit(*arguments) || isspace(*arguments))) {
      ++arguments;
    }

    int skillID = 0;
    for (int i = 0; i < g_skillLineDB.GetNumRecords(); ++i) {
      const SkillLineRec *skill = g_skillLineDB.GetRecordByIndex(i);
      if (!SStrCmpI(skill->m_displayName_lang[0], arguments, SStrLen(arguments))) {
        skillID = skill->m_ID;
        break;
      }
    }

    if (skillID) {
      CDataStore msg;
      msg.Put(CMSG_SET_SKILL_CHEAT);
      msg.Put(skillID);
      msg.Put(level);
      msg.Finalize();
      ClientServices_Send(&msg);
      return 1;
    }
  }

  ConsolePrintf("Unknown skill line (usage: setskill <level> <skill line name>)");
  return 1;
}

static BOOL CCommand_CancelAura(LPCSTR, LPCSTR arguments) {
  int        spellID = SStrToInt(arguments);
  CDataStore msg;
  msg.Put(CMSG_CANCEL_AURA);
  msg.Put(spellID);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static BOOL CCommand_SpellString(LPCSTR, LPCSTR arguments) {
  if (arguments && *arguments) {
    SStrPrintf(s_spellTargetString, sizeof(s_spellTargetString), "%s", arguments);
    if (Spell_C_IsTargeting()) {
      if (s_needTargets & 0x2000) {
        SStrPrintf(s_spellCast.targetString, sizeof(s_spellCast.targetString), "%s", s_spellTargetString);
        s_needTargets &= ~0x2000;
        s_spellCast.targets |= 0x2000;
        s_spellTargetString[0] = 0;
        CGSpellBook::UpdateSelection();
        CGActionBar::UpdateSelection();
      }
      if (!s_needTargets) {
        SendCast(&s_spellCast);
      }
    }
  }
  return 1;
}

void Spell_C_Destroy() {
  ClientServices_ClearMessageHandler(SMSG_CAST_RESULT);
  ClientServices_ClearMessageHandler(SMSG_SPELL_START);
  ClientServices_ClearMessageHandler(SMSG_SPELL_GO);
  ClientServices_ClearMessageHandler(SMSG_SPELL_FAILURE);
  ClientServices_ClearMessageHandler(SMSG_PET_CAST_FAILED);
  ClientServices_ClearMessageHandler(SMSG_SPELL_COOLDOWN);
  ClientServices_ClearMessageHandler(SMSG_ITEM_COOLDOWN);
  ClientServices_ClearMessageHandler(SMSG_COOLDOWN_EVENT);
  ClientServices_ClearMessageHandler(SMSG_CLEAR_COOLDOWN);
  ClientServices_ClearMessageHandler(SMSG_COOLDOWN_CHEAT);
  ClientServices_ClearMessageHandler(SMSG_PET_TAME_FAILURE);
  ClientServices_ClearMessageHandler(SMSG_SPELL_DELAYED);
  ClientServices_ClearMessageHandler(MSG_CHANNEL_START);
  ClientServices_ClearMessageHandler(MSG_CHANNEL_UPDATE);
  ClientServices_ClearMessageHandler(MSG_ADD_DYNAMIC_TARGET);
  ClientServices_ClearMessageHandler(SMSG_PLAY_SPELL_VISUAL);

  ConsoleCommandUnregister("cast");
  ConsoleCommandUnregister("learn");
  ConsoleCommandUnregister("cooldown");
  ConsoleCommandUnregister("cooldownPet");
  ConsoleCommandUnregister("useskill");
  ConsoleCommandUnregister("setskill");
  ConsoleCommandUnregister("cancelaura");
  ConsoleCommandUnregister("spellstring");

  s_itemCooldowns.Clear();
  for (UINT i = 0; i < 2; ++i) {
    s_spellHistory[i].ClearHistory();
  }
}

bool IsSpellAura(const SpellRec *rec) {
  for (UINT effect = 0; effect < 3; ++effect) {
    if (rec->m_effect[effect] == 6 || rec->m_effect[effect] == 35) {
      return 1;
    }
  }
  return 0;
}
