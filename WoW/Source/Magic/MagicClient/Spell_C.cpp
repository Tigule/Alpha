#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

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
#include "Ui/WorldFrame.h"
#include "WorldClient/World.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"
#include "SoundInterface/SoundInterface.h"

#include <Base/CDataStore.h>
#include <FrameScript/FrameScript.h>

extern FrameScript_Method s_SpellScriptFunctions[4];
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

class CGCraftInfo {
 public:
  static void SetCraftType(SPELL_CAST_UI_TYPE type);
};

class CGTradeSkillInfo {
 public:
  static void SetSkillLine(int id);
};

class SpellCast {
 public:
  void BuildFullZoneUpdate(CDataStore *msg);
  void UnpackFullZoneUpdate(CDataStore *msg);

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
  BYTE  needsEvent;
};

static SpellCast                                       s_spellCast;
static GAME_ERROR_TYPE                                 s_gerrEnums[4] = {GERR_OUT_OF_MANA, GERR_OUT_OF_RAGE, GERR_OUT_OF_FOCUS, GERR_OUT_OF_ENERGY};
static UINT                                            s_displayPowerMods[4] = {1, 10, 1, 1};
static WORD                                            s_needTargets;
static int                                             s_modalSpellID;
static int                                             s_savedModalSpellID;
static DWORDLONG                                       s_modalItemID;
static DWORDLONG                                       s_savedModalItemID;
static BYTE                                            s_playerCast;
static char                                            s_spellTargetString[128];
static UINT                                            s_spellWorldModel;
static float                                           s_spellWorldModelFacing;
static BYTE                                            s_spellWorldModelHousing;
static SpellHistory                                    s_spellHistory[2];
static DWORD                                           s_cleanupTime;
static TSHashTable<ITEMCOOLDOWNHASHNODE, HASHKEY_NONE> s_itemCooldowns;

void CursorSetCursorMode(CURSORANIMATIONS mode);
void CursorModelSetSequence(CURSORANIMATIONS sequence);
void CursorResetCursor(int force);
void SendCast(SpellCast *cast);
void SpellPutCastTargets(SpellCast *cast, CDataStore *msg);
void SpellGetCastTargets(SpellCast *cast, CDataStore *msg);
void Spell_C_SpellFailed(int spellID, BYTE reason, int arg1, int arg2);
void SpellVisualsHandleCastStop(int id, CGUnit_C *caster, BYTE status, BYTE reason);
void SpellVisualsHandleCastStart(int id, const SpellCast &cast, CGUnit_C *caster, UINT duration, UINT animDuration, bool wasProc);
void UnitCombatLogSpellFail(CGUnit_C *caster, int spellID, LPCSTR message);
void Spell_C_CancelSpell(bool failed, bool notifyServer, SPELL_FAILED_REASON reason);
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
bool                       Spell_C_IsTargeting();
bool                       Spell_C_HaveSpellTokens(CGPlayer_C *player, const SpellRec *rec, bool report);
bool                       Spell_C_HaveEquippedSpellItems(CGPlayer_C *player, const SpellRec *rec, bool checkAmmo, bool report);
bool                       RangeCheckSelected(CGPlayer_C *caster, const SpellRec *srec);
bool                       Spell_C_TargetSpell(CGUnit_C *caster, const SpellRec *srec);
void                       UnitEffectPreloadSpellEffects(int spellID);
const ItemSubClassRec     *SDBItemSubclassGetSubClassRec(UINT classID, UINT subClassID);
bool                       Spell_C_HandleSpriteClick(CGObject_C *object);
const SkillLineAbilityRec *SpellTableLookupAbility(UINT raceID, UINT classID, UINT spellID);

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

void SpellHistory::RemoveHold(int spellID, DWORD startTime, bool clear) {
  SPELLHISTORY *history = m_spellHistory.Head();
  while (history) {
    SPELLHISTORY *next = m_spellHistory.Next(history);
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
    history = next;
  }
}

void SpellHistory::ClearHistory() {
  while (SPELLHISTORY *history = m_spellHistory.Head()) {
    m_spellHistory.UnlinkNode(history);
    m_freeList.LinkNode(history, LIST_TAIL, 0);
  }
}

void SpellHistory::GarbageCollect(DWORD timestamp) {
  SPELLHISTORY *history = m_spellHistory.Head();
  while (history) {
    SPELLHISTORY *next = m_spellHistory.Next(history);
    if (!history->onHold && (!history->recoveryTime || static_cast<long>(timestamp - (history->recoveryStart + history->recoveryTime)) >= 0) &&
        (!history->categoryRecoveryTime || static_cast<long>(timestamp - (history->categoryRecoveryStart + history->categoryRecoveryTime)) >= 0))
    {
      m_spellHistory.UnlinkNode(history);
      m_freeList.LinkNode(history, LIST_TAIL, 0);
    }
    history = next;
  }
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
    if (((history->spellID == spellID && history->itemID == itemID && history->recoveryTime) ||
         (history->category == category && history->categoryRecoveryTime)) &&
        history->onHold)
    {
      return 1;
    }
  }
  return 0;
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
      DWORD start = history->onHold ? now : history->recoveryStart;
      DWORD end = start + history->recoveryTime;
      if (static_cast<long>(end - latestEnd) >= 0) {
        if (duration) {
          *duration = history->recoveryTime;
        }
        if (startTime) {
          *startTime = start;
        }
        if (enable) {
          *enable = !history->onHold;
        }
        latestEnd = end;
      }
    }

    if (history->category == category && history->categoryRecoveryTime) {
      DWORD start = history->onHold ? now : history->categoryRecoveryStart;
      DWORD end = start + history->categoryRecoveryTime;
      if (static_cast<long>(end - latestEnd) >= 0) {
        if (duration) {
          *duration = history->categoryRecoveryTime;
        }
        if (startTime) {
          *startTime = start;
        }
        if (enable) {
          *enable = !history->onHold;
        }
        latestEnd = end;
      }
    }

    if (history->startRecoveryCategory == startCategory && history->startRecoveryTime) {
      DWORD start = history->onHold ? now : history->recoveryStart;
      DWORD end = start + history->startRecoveryTime;
      if (static_cast<long>(end - latestEnd) >= 0) {
        if (duration) {
          *duration = history->startRecoveryTime;
        }
        if (startTime) {
          *startTime = start;
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
    case 0:
      return "SPELL_FAILED_AFFECTING_COMBAT";
    case 1:
      return "SPELL_FAILED_ALREADY_HAVE_CHARM";
    case 2:
      return "SPELL_FAILED_ALREADY_HAVE_SUMMON";
    case 3:
      return "SPELL_FAILED_ALREADY_OPEN";
    case 4:
      return "SPELL_FAILED_AURA_BOUNCED";
    case 5:
      return "SPELL_FAILED_BAD_IMPLICIT_TARGETS";
    case 6:
      return "SPELL_FAILED_BAD_TARGETS";
    case 7:
      return "SPELL_FAILED_CANT_BE_CHARMED";
    case 8:
      return "SPELL_FAILED_CANT_STEALTH";
    case 9:
      return "SPELL_FAILED_CASTER_AURASTATE";
    case 10:
      return "SPELL_FAILED_CASTER_DEAD";
    case 11:
      return "SPELL_FAILED_DONT_REPORT";
    case 12:
      return "SPELL_FAILED_EQUIPPED_ITEM";
    case 13:
      return "SPELL_FAILED_EQUIPPED_ITEM_CLASS";
    case 14:
      return "SPELL_FAILED_ERROR";
    case 15:
      return "SPELL_FAILED_FIZZLE";
    case 16:
      return "SPELL_FAILED_HUNGER_SATIATED";
    case 17:
      return "SPELL_FAILED_INTERRUPTED";
    case 18:
      return "SPELL_FAILED_INTERRUPTED_COMBAT";
    case 19:
      return "SPELL_FAILED_ITEM_ALREADY_ENCHANTED";
    case 20:
      return "SPELL_FAILED_ITEM_NOT_FOUND";
    case 21:
      return "SPELL_FAILED_ITEM_NOT_READY";
    case 22:
      return "SPELL_FAILED_LEVEL_REQUIREMENT";
    case 23:
      return "SPELL_FAILED_LINE_OF_SIGHT";
    case 24:
      return "SPELL_FAILED_LOWLEVEL";
    case 25:
      return "SPELL_FAILED_LOW_CASTLEVEL";
    case 26:
      return "SPELL_FAILED_MOVING";
    case 27:
      return "SPELL_FAILED_NEED_AMMO";
    case 28:
      return "SPELL_FAILED_NEED_AMMO_POUCH";
    case 29:
      return "SPELL_FAILED_NEED_EXOTIC_AMMO";
    case 30:
      return "SPELL_FAILED_NOPATH";
    case 31:
      return "SPELL_FAILED_NOTSTANDING";
    case 32:
      return "SPELL_FAILED_NOT_BEHIND";
    case 33:
      return "SPELL_FAILED_NOT_BEHIND_OR_SIDE";
    case 34:
      return "SPELL_FAILED_NOT_HERE";
    case 35:
      return "SPELL_FAILED_NOT_KNOWN";
    case 36:
      return "SPELL_FAILED_NOT_MOUNTED";
    case 37:
      return "SPELL_FAILED_NOT_READY";
    case 38:
      return "SPELL_FAILED_NOT_SHAPESHIFT";
    case 39:
      return "SPELL_FAILED_NOT_TRADING";
    case 40:
      return "SPELL_FAILED_NO_AMMO";
    case 41:
      return "SPELL_FAILED_NO_CHARGES_REMAIN";
    case 42:
      return "SPELL_FAILED_NO_ENDURANCE";
    case 43:
      return "SPELL_FAILED_NO_PET";
    case 44:
      return "SPELL_FAILED_NO_POWER";
    case 45:
      return "SPELL_FAILED_ONLY_ABOVEWATER";
    case 46:
      return "SPELL_FAILED_ONLY_DAYTIME";
    case 47:
      return "SPELL_FAILED_ONLY_INDOORS";
    case 48:
      return "SPELL_FAILED_ONLY_MOUNTED";
    case 49:
      return "SPELL_FAILED_ONLY_NIGHTTIME";
    case 50:
      return "SPELL_FAILED_ONLY_OUTDOORS";
    case 51:
      return "SPELL_FAILED_ONLY_SHAPESHIFT";
    case 52:
      return "SPELL_FAILED_ONLY_STEALTHED";
    case 53:
      return "SPELL_FAILED_ONLY_UNDERWATER";
    case 54:
      return "SPELL_FAILED_OUT_OF_RANGE";
    case 55:
      return "SPELL_FAILED_PACIFIED";
    case 56:
      return "SPELL_FAILED_REAGENTS";
    case 57:
      return "SPELL_FAILED_REQUIRES_SPELL_FOCUS";
    case 58:
      return "SPELL_FAILED_SILENCED";
    case 59:
      return "SPELL_FAILED_SPELL_IN_PROGRESS";
    case 60:
      return "SPELL_FAILED_SPELL_LEARNED";
    case 61:
      return "SPELL_FAILED_SPELL_UNAVAILABLE";
    case 62:
      return "SPELL_FAILED_STUNNED";
    case 63:
      return "SPELL_FAILED_TARGETS_DEAD";
    case 64:
      return "SPELL_FAILED_TARGET_AFFECTING_COMBAT";
    case 65:
      return "SPELL_FAILED_TARGET_AURASTATE";
    case 66:
      return "SPELL_FAILED_TARGET_ENEMY";
    case 67:
      return "SPELL_FAILED_TARGET_ENRAGED";
    case 68:
      return "SPELL_FAILED_TARGET_FRIENDLY";
    case 69:
      return "SPELL_FAILED_TARGET_IS_PLAYER";
    case 70:
      return "SPELL_FAILED_TARGET_NOT_DEAD";
    case 71:
      return "SPELL_FAILED_TARGET_NOT_IN_PARTY";
    case 72:
      return "SPELL_FAILED_TARGET_NO_POCKETS";
    case 73:
      return "SPELL_FAILED_THIRST_SATIATED";
    case 74:
      return "SPELL_FAILED_TOO_CLOSE";
    case 75:
      return "SPELL_FAILED_TOTEMS";
    case 76:
      return "SPELL_FAILED_TRY_AGAIN";
    case 77:
      return "SPELL_FAILED_UNIT_NOT_ATSIDE";
    case 78:
      return "SPELL_FAILED_UNIT_NOT_BEHIND";
    case 79:
      return "SPELL_FAILED_UNIT_NOT_INFRONT";
    case 80:
      return "SPELL_FAILED_NO_MOUNTS_ALLOWED";
    case 81:
      return "SPELL_FAILED_CHEST_IN_USE";
    case 82:
      return "SPELL_FAILED_NO_COMBO_POINTS";
    case 83:
      return "SPELL_FAILED_TARGET_NOT_PLAYER";
    case 84:
      return "SPELL_FAILED_TARGET_DUELING";
    case 85:
      return "SPELL_FAILED_NOTUNSHEATHED";
    case 86:
      return "SPELL_FAILED_NOT_FISHABLE";
    default:
      return "SPELL_FAILED_UNKNOWN";
  }
}

static void SpellMissingItemCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted) {
  BYTE reason = static_cast<BYTE>(reinterpret_cast<DWORD>(arg));
  char message[128];
  char processedmessage[256];

  SStrCopy(message, FrameScript_GetText(GetStringReason(reason), -1, GENDER_NOT_APPLICABLE), sizeof(message));
  const ItemStats *stats = g_itemDBCache.GetRecord(id, guid, 0, 0);
  SStrPrintf(processedmessage, sizeof(processedmessage), message, stats ? stats->m_displayName[0] : "UNKNOWN");
  CGGameUI::DisplayError(reason == 56 ? GERR_SPELL_FAILED_REAGENTS : reason == 75 ? GERR_SPELL_FAILED_TOTEMS : GERR_SPELL_FAILED_S, processedmessage);
}

void Spell_C_SpellFailed(int spellID, BYTE reason, int arg1, int arg2) {
  char            shapes[512];
  char            processedmessage[256];
  char            token[64];
  char            message[128];
  UINT            numEntries = 0;
  CGPlayer_C     *playerPtr = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  const SpellRec *spell = g_spellDB.GetRecord(spellID);
  BOOL            isPet = 0;
  int             first = 1;
  GAME_ERROR_TYPE error = GERR_SPELL_FAILED_S;

  FrameScript_SignalEvent(370);
  if (spell) {
    SndInterfacePlaySpellFizzleSound(spellID, playerPtr);
    switch (reason) {
      case 37:
        if (spell->m_category == 10 || spell->m_category == 11) {
          error = GERR_FOOD_COOLDOWN;
        } else if (spell->m_category == 4 || spell->m_category == 9) {
          error = GERR_POTION_COOLDOWN;
        } else {
          error = (spell->m_attributes & 0x10) ? GERR_ABILITY_COOLDOWN : GERR_SPELL_COOLDOWN;
        }
        break;
      case 21:
        error = GERR_ITEM_COOLDOWN;
        break;
      case 16:
        error = GERR_HUNGER_SATIATED;
        break;
      case 73:
        error = GERR_THIRST_SATIATED;
        break;
      case 75:
        error = GERR_SPELL_FAILED_TOTEMS;
        break;
      case 56:
        error = GERR_SPELL_FAILED_REAGENTS;
        break;
      case 12:
        error = GERR_SPELL_FAILED_EQUIPPED_ITEM;
        break;
      case 13:
        error = GERR_SPELL_FAILED_EQUIPPED_ITEM_CLASS_S;
        break;
      case 5:
        error = GERR_GENERIC_NO_TARGET;
        break;
      case 54:
        error = GERR_SPELL_OUT_OF_RANGE;
        break;
      case 27:
        error = GERR_NOAMMO_S;
        break;
      case 51:
        error = GERR_SPELL_FAILED_SHAPESHIFT_FORM_S;
        break;
      case 6:
        error = (spell->m_targets & 0x10) ? GERR_INVALID_ITEM_TARGET : GERR_INVALID_ATTACK_TARGET;
        break;
      case 85:
        error = GERR_SPELL_FAILED_NOTUNSHEATHED;
        break;
    }
    isPet = spell->m_effect[0] == 57 || (spell->m_effect[0] == 36 && spell->m_effectMiscValue[0] == 5);
  }

  if (playerPtr) {
    playerPtr->OnSpellFailed(spell, reason);
  }
  if (reason == 11) {
    return;
  }

  LPCSTR failureToken;
  switch (reason) {
    case 0:
      failureToken = "SPELL_FAILED_AFFECTING_COMBAT";
      break;
    case 1:
      failureToken = "SPELL_FAILED_ALREADY_HAVE_CHARM";
      break;
    case 2:
      failureToken = "SPELL_FAILED_ALREADY_HAVE_SUMMON";
      break;
    case 3:
      failureToken = "SPELL_FAILED_ALREADY_OPEN";
      break;
    case 4:
      failureToken = "SPELL_FAILED_AURA_BOUNCED";
      break;
    case 5:
      failureToken = "SPELL_FAILED_BAD_IMPLICIT_TARGETS";
      break;
    case 6:
      failureToken = "SPELL_FAILED_BAD_TARGETS";
      break;
    case 7:
      failureToken = "SPELL_FAILED_CANT_BE_CHARMED";
      break;
    case 8:
      failureToken = "SPELL_FAILED_CANT_STEALTH";
      break;
    case 9:
      failureToken = "SPELL_FAILED_CASTER_AURASTATE";
      break;
    case 10:
      failureToken = "SPELL_FAILED_CASTER_DEAD";
      break;
    case 11:
      failureToken = "SPELL_FAILED_DONT_REPORT";
      break;
    case 12:
      failureToken = "SPELL_FAILED_EQUIPPED_ITEM";
      break;
    case 13:
      failureToken = "SPELL_FAILED_EQUIPPED_ITEM_CLASS";
      break;
    case 14:
      failureToken = "SPELL_FAILED_ERROR";
      break;
    case 15:
      failureToken = "SPELL_FAILED_FIZZLE";
      break;
    case 16:
      failureToken = "SPELL_FAILED_HUNGER_SATIATED";
      break;
    case 17:
      failureToken = "SPELL_FAILED_INTERRUPTED";
      break;
    case 18:
      failureToken = "SPELL_FAILED_INTERRUPTED_COMBAT";
      break;
    case 19:
      failureToken = "SPELL_FAILED_ITEM_ALREADY_ENCHANTED";
      break;
    case 20:
      failureToken = "SPELL_FAILED_ITEM_NOT_FOUND";
      break;
    case 21:
      failureToken = "SPELL_FAILED_ITEM_NOT_READY";
      break;
    case 22:
      failureToken = "SPELL_FAILED_LEVEL_REQUIREMENT";
      break;
    case 23:
      failureToken = "SPELL_FAILED_LINE_OF_SIGHT";
      break;
    case 24:
      failureToken = "SPELL_FAILED_LOWLEVEL";
      break;
    case 25:
      failureToken = "SPELL_FAILED_LOW_CASTLEVEL";
      break;
    case 26:
      failureToken = "SPELL_FAILED_MOVING";
      break;
    case 27:
      failureToken = "SPELL_FAILED_NEED_AMMO";
      break;
    case 28:
      failureToken = "SPELL_FAILED_NEED_AMMO_POUCH";
      break;
    case 29:
      failureToken = "SPELL_FAILED_NEED_EXOTIC_AMMO";
      break;
    case 30:
      failureToken = "SPELL_FAILED_NOPATH";
      break;
    case 31:
      failureToken = "SPELL_FAILED_NOTSTANDING";
      break;
    case 32:
      failureToken = "SPELL_FAILED_NOT_BEHIND";
      break;
    case 33:
      failureToken = "SPELL_FAILED_NOT_BEHIND_OR_SIDE";
      break;
    case 34:
      failureToken = "SPELL_FAILED_NOT_HERE";
      break;
    case 35:
      failureToken = "SPELL_FAILED_NOT_KNOWN";
      break;
    case 36:
      failureToken = "SPELL_FAILED_NOT_MOUNTED";
      break;
    case 37:
      failureToken = "SPELL_FAILED_NOT_READY";
      break;
    case 38:
      failureToken = "SPELL_FAILED_NOT_SHAPESHIFT";
      break;
    case 39:
      failureToken = "SPELL_FAILED_NOT_TRADING";
      break;
    case 40:
      failureToken = "SPELL_FAILED_NO_AMMO";
      break;
    case 41:
      failureToken = "SPELL_FAILED_NO_CHARGES_REMAIN";
      break;
    case 42:
      failureToken = "SPELL_FAILED_NO_ENDURANCE";
      break;
    case 43:
      failureToken = "SPELL_FAILED_NO_PET";
      break;
    case 44:
      failureToken = "SPELL_FAILED_NO_POWER";
      break;
    case 45:
      failureToken = "SPELL_FAILED_ONLY_ABOVEWATER";
      break;
    case 46:
      failureToken = "SPELL_FAILED_ONLY_DAYTIME";
      break;
    case 47:
      failureToken = "SPELL_FAILED_ONLY_INDOORS";
      break;
    case 48:
      failureToken = "SPELL_FAILED_ONLY_MOUNTED";
      break;
    case 49:
      failureToken = "SPELL_FAILED_ONLY_NIGHTTIME";
      break;
    case 50:
      failureToken = "SPELL_FAILED_ONLY_OUTDOORS";
      break;
    case 51:
      failureToken = "SPELL_FAILED_ONLY_SHAPESHIFT";
      break;
    case 52:
      failureToken = "SPELL_FAILED_ONLY_STEALTHED";
      break;
    case 53:
      failureToken = "SPELL_FAILED_ONLY_UNDERWATER";
      break;
    case 54:
      failureToken = "SPELL_FAILED_OUT_OF_RANGE";
      break;
    case 55:
      failureToken = "SPELL_FAILED_PACIFIED";
      break;
    case 56:
      failureToken = "SPELL_FAILED_REAGENTS";
      break;
    case 57:
      failureToken = "SPELL_FAILED_REQUIRES_SPELL_FOCUS";
      break;
    case 58:
      failureToken = "SPELL_FAILED_SILENCED";
      break;
    case 59:
      failureToken = "SPELL_FAILED_SPELL_IN_PROGRESS";
      break;
    case 60:
      failureToken = "SPELL_FAILED_SPELL_LEARNED";
      break;
    case 61:
      failureToken = "SPELL_FAILED_SPELL_UNAVAILABLE";
      break;
    case 62:
      failureToken = "SPELL_FAILED_STUNNED";
      break;
    case 63:
      failureToken = "SPELL_FAILED_TARGETS_DEAD";
      break;
    case 64:
      failureToken = "SPELL_FAILED_TARGET_AFFECTING_COMBAT";
      break;
    case 65:
      failureToken = "SPELL_FAILED_TARGET_AURASTATE";
      break;
    case 66:
      failureToken = "SPELL_FAILED_TARGET_ENEMY";
      break;
    case 67:
      failureToken = "SPELL_FAILED_TARGET_ENRAGED";
      break;
    case 68:
      failureToken = "SPELL_FAILED_TARGET_FRIENDLY";
      break;
    case 69:
      failureToken = "SPELL_FAILED_TARGET_IS_PLAYER";
      break;
    case 70:
      failureToken = "SPELL_FAILED_TARGET_NOT_DEAD";
      break;
    case 71:
      failureToken = "SPELL_FAILED_TARGET_NOT_IN_PARTY";
      break;
    case 72:
      failureToken = "SPELL_FAILED_TARGET_NO_POCKETS";
      break;
    case 73:
      failureToken = "SPELL_FAILED_THIRST_SATIATED";
      break;
    case 74:
      failureToken = "SPELL_FAILED_TOO_CLOSE";
      break;
    case 75:
      failureToken = "SPELL_FAILED_TOTEMS";
      break;
    case 76:
      failureToken = "SPELL_FAILED_TRY_AGAIN";
      break;
    case 77:
      failureToken = "SPELL_FAILED_UNIT_NOT_ATSIDE";
      break;
    case 78:
      failureToken = "SPELL_FAILED_UNIT_NOT_BEHIND";
      break;
    case 79:
      failureToken = "SPELL_FAILED_UNIT_NOT_INFRONT";
      break;
    case 80:
      failureToken = "SPELL_FAILED_NO_MOUNTS_ALLOWED";
      break;
    case 81:
      failureToken = "SPELL_FAILED_CHEST_IN_USE";
      break;
    case 82:
      failureToken = "SPELL_FAILED_NO_COMBO_POINTS";
      break;
    case 83:
      failureToken = "SPELL_FAILED_TARGET_NOT_PLAYER";
      break;
    case 84:
      failureToken = "SPELL_FAILED_TARGET_DUELING";
      break;
    case 85:
      failureToken = "SPELL_FAILED_NOTUNSHEATHED";
      break;
    case 86:
      failureToken = "SPELL_FAILED_NOT_FISHABLE";
      break;
    default:
      failureToken = "SPELL_FAILED_UNKNOWN";
      break;
  }

  message[0] = 0;
  if (isPet) {
    SStrPrintf(token, sizeof(token), "%s_PET", failureToken);
    SStrCopy(message, FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE), sizeof(message));
  }
  if (!message[0]) {
    SStrCopy(message, FrameScript_GetText(failureToken, -1, GENDER_NOT_APPLICABLE), sizeof(message));
  }

  LPCSTR                 replacement = 0;
  const ItemSubClassRec *subclass = 0;
  shapes[0] = 0;
  processedmessage[0] = 0;
  switch (reason) {
    case 13:
      subclass = FindAnyItemSubclassRec(arg1, arg2);
      replacement = subclass ? subclass->m_displayName_lang[CURRENT_LANGUAGE] : 0;
      break;
    case 27:
    case 28:
      subclass = FindAnyItemSubclassRec(11, 1 << arg1);
      replacement = subclass ? subclass->m_displayName_lang[CURRENT_LANGUAGE] : 0;
      break;
    case 29:
      subclass = FindAnyItemSubclassRec(6, 1 << arg1);
      replacement = subclass ? subclass->m_displayName_lang[CURRENT_LANGUAGE] : 0;
      break;
    case 44:
      if (spell && spell->m_powerType == -2) {
        CGGameUI::DisplayError(GERR_OUT_OF_HEALTH);
      } else if (spell) {
        CGGameUI::DisplayError(s_gerrEnums[spell->m_powerType]);
      }
      UnitCombatLogSpellFail(playerPtr, spellID, CGGameUI::GetLastErrorString());
      return;
    case 51: {
      if (!spell) {
        return;
      }
      for (int i = 0; i < g_spellShapeshiftFormDB.GetNumRecords(); ++i) {
        const SpellShapeshiftFormRec *form = g_spellShapeshiftFormDB.GetRecordByIndex(i);
        if ((spell->m_shapeshiftMask & (1 << i)) && form->m_name_lang[CURRENT_LANGUAGE] && *form->m_name_lang[CURRENT_LANGUAGE]) {
          if (shapes[0]) {
            SStrPack(shapes, ", ", sizeof(shapes));
          }
          SStrPack(shapes, form->m_name_lang[CURRENT_LANGUAGE], sizeof(shapes));
        }
      }
      if (!shapes[0]) {
        return;
      }
      replacement = shapes;
      break;
    }
    case 56:
    case 75: {
      const ItemStats *stats =
          g_itemDBCache.GetRecord(arg1, static_cast<DWORDLONG>(0), SpellMissingItemCallback, reinterpret_cast<LPVOID>(static_cast<DWORD>(reason)));
      if (!stats) {
        return;
      }
      replacement = stats->m_displayName[0];
      break;
    }
    case 57: {
      const SpellFocusObjectRec *focus = g_spellFocusObjectDB.GetRecord(arg1);
      replacement = focus ? focus->m_name_lang[CURRENT_LANGUAGE] : 0;
      break;
    }
  }

  if (replacement) {
    SStrPrintf(processedmessage, sizeof(processedmessage), message, replacement);
  } else {
    SStrCopy(processedmessage, message, sizeof(processedmessage));
  }

  numEntries = SStrLen(processedmessage);
  first = numEntries == 0;
  UnitCombatLogSpellFail(playerPtr, spellID, first ? message : processedmessage);
  CGGameUI::DisplayError(error, first ? message : processedmessage);

  if (spellID == s_spellCast.spellID) {
    Spell_C_CancelSpell(reason, 0, static_cast<SPELL_FAILED_REASON>(reason));
  } else if (!Spell_C_IsTargeting()) {
    FrameScript_SignalEvent(reason == SPELL_FAILED_INTERRUPTED || reason == SPELL_FAILED_INTERRUPTED_COMBAT ? 318 : 317);
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
  UINT             spellRecoveryTime = 0;
  DWORD            categoryRecoveryStart = 0;
  UINT             categoryRecoveryTime = 0;
  const SpellRec  *srec = g_spellDB.GetRecord(spellID);
  DWORD            now = OsGetAsyncTimeMs();
  const ItemStats *stats = 0;
  DWORD            spellRecoveryStart = 0;

  if (!srec) {
    return;
  }

  int index = 0;
  if (itemID) {
    stats = g_itemDBCache.GetRecord(itemID, 0, 0, 0);
    if (stats) {
      while (stats->m_spellID[index] != spellID) {
        ++index;
        FATALASSERT(index < 5);
      }
    }
  }

  if (recoveryLeft > 0) {
    if (!stats || stats->m_spellCooldown[index] < 0) {
      spellRecoveryTime = srec->m_recoveryTime;
    } else {
      spellRecoveryTime = stats->m_spellCooldown[index];
    }
    if (spellRecoveryTime <= static_cast<UINT>(recoveryLeft)) {
      spellRecoveryTime = recoveryLeft;
    }
    spellRecoveryStart = now - spellRecoveryTime + recoveryLeft;
  }

  if (categoryRecoveryLeft > 0) {
    if (!stats || stats->m_spellCategoryCooldown[index] < 0) {
      categoryRecoveryTime = srec->m_categoryRecoveryTime;
    } else {
      categoryRecoveryTime = stats->m_spellCategoryCooldown[index];
    }
    if (categoryRecoveryTime <= static_cast<UINT>(categoryRecoveryLeft)) {
      categoryRecoveryTime = categoryRecoveryLeft;
    }
    categoryRecoveryStart = now - categoryRecoveryTime + categoryRecoveryLeft;
  }

  s_spellHistory[isPet].AddHistory(
      spellID, itemID, spellRecoveryStart, spellRecoveryTime, category, categoryRecoveryStart, categoryRecoveryTime, needsEvent, 0, 0
  );
}

static void ItemStatsCooldownCallback(int id, const DWORDLONG &guid, LPVOID, bool granted) {
  if (!granted) {
    return;
  }
  const ItemStats *stats = g_itemDBCache.GetRecord(id, guid, 0, 0);
  if (!stats) {
    return;
  }
  int index;
  for (index = 0; index < 5; ++index) {
    const SpellRec *srec = g_spellDB.GetRecord(stats->m_spellID[index]);
    if (srec && !stats->m_spellTrigger[index]) {
      int  category = stats->m_spellCategory[index];
      UINT selfCooldown = stats->m_spellCooldown[index] < 0 ? srec->m_recoveryTime : stats->m_spellCooldown[index];
      Spell_C_SetCooldownLeft(srec->m_ID, id, category, selfCooldown, stats->m_spellCategoryCooldown[index], true, 0, 0);
    }
  }
}

int Spell_C_GetSpellCooldown(int spell, BOOL isPet, UINT *duration, DWORD *startTime, UINT *enable) {
  return s_spellHistory[isPet].GetCooldown(spell, 0, duration, startTime, enable);
}

static void ItemCheckCooldownCallback(int id, const DWORDLONG &guid, LPVOID, bool granted) {
  if (granted) {
    CGSpellBook::UpdateCooldowns();
    CGActionBar::UpdateCooldowns();
  }
}

int Spell_C_GetItemCooldown(int itemID, UINT *duration, DWORD *startTime, UINT *enable) {
  const ItemStats *stats =
      g_itemDBCache.GetRecord(itemID, ClntObjMgrGetActivePlayer(), reinterpret_cast<DBCACHECALLBACKPROC>(ItemCheckCooldownCallback), 0);
  if (!stats) {
    return 0;
  }

  int index;
  for (index = 0; index < 5; ++index) {
    if (stats->m_spellID[index] > 0 && !stats->m_spellTrigger[index]) {
      return s_spellHistory[0].GetCooldown(stats->m_spellID[index], itemID, duration, startTime, enable);
    }
  }
  return 0;
}

int Spell_C_NeedsCooldownEvent(const SpellRec *srec, BOOL isPet) {
  return s_spellHistory[isPet].IsOnHold(srec->m_ID, 0);
}

int Spell_C_NeedsCooldownEvent(int itemID) {
  const ItemStats *stats =
      g_itemDBCache.GetRecord(itemID, ClntObjMgrGetActivePlayer(), reinterpret_cast<DBCACHECALLBACKPROC>(ItemCheckCooldownCallback), 0);
  if (!stats) {
    return 0;
  }
  for (int i = 0; i < 5; ++i) {
    if (stats->m_spellID[i] > 0 && !stats->m_spellTrigger[i]) {
      return s_spellHistory[0].IsOnHold(stats->m_spellID[i], itemID);
    }
  }
  return 0;
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

int Spell_C_GetSpellByName(LPCSTR name) {
  int num = g_spellDB.GetNumRecords();
  for (int i = 0; i < num; ++i) {
    const SpellRec *spell = g_spellDB.GetRecordByIndex(i);
    if (!SStrCmpI(spell->m_name_lang[CURRENT_LANGUAGE], name, 0x7FFFFFFF)) {
      return spell->m_ID;
    }
  }
  ConsoleWriteA("Unknown spell %s", DEFAULT_COLOR, name);
  return -1;
}

int Spell_C_GetSpellLevel(int id, BOOL isPet) {
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (isPet) {
    if (!unit) {
      return 0;
    }

    const CGUnitData *unitData = unit->GetUnitData();
    DWORDLONG         pet = unitData->charm ? unitData->charm : unitData->summon;
    unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(pet, __FILE__, __LINE__));
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
      return static_cast<int>(spell->m_manaCostPct * 0.01f * player->m_plyr->baseMana);
    }
  }
  return spell->m_manaCost + Spell_C_GetSpellLevel(id, isPet) * spell->m_manaCostPerLevel;
}

int Spell_C_GetManaCostPerSecond(int id, BOOL isPet) {
  const SpellRec *spellRec = g_spellDB.GetRecord(id);
  if (!spellRec) {
    return -1;
  }
  return spellRec->m_manaPerSecond + Spell_C_GetSpellLevel(id, isPet) * spellRec->m_manaPerSecondPerLevel;
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

  int result = castTime->m_base + Spell_C_GetSpellLevel(id, isPet) * castTime->m_perLevel;
  return result > castTime->m_minimum ? result : castTime->m_minimum;
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
    *max = 100.0f;
  } else if (range->m_flags & 1) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      CGUnit_C *target = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(player->IsAttacking(), __FILE__, __LINE__));
      float     targetReach = target ? target->GetUnitData()->combatReach + target->GetUnitData()->boundingRadius : range->m_rangeMax;
      *max = player->GetUnitData()->combatReach + player->GetUnitData()->boundingRadius + targetReach + 1.3333334f;
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
  int   minBonus = static_cast<int>(levelBonus);
  int   maxBonus = static_cast<int>(levelBonus - floor(levelBonus) < 0.5f ? floor(levelBonus) : ceil(levelBonus));

  *min =
      srec->m_effectBasePoints[effectIndex] + minBonus + srec->m_effectBaseDice[effectIndex] + casterLevel * srec->m_effectDicePerLevel[effectIndex];
  *max = srec->m_effectBasePoints[effectIndex] + maxBonus + dieSides * srec->m_effectBaseDice[effectIndex] +
         dieSides * casterLevel * srec->m_effectDicePerLevel[effectIndex];
}

void Spell_C_SetModal(int spellID, const CGItem_C *item) {
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

void SendCast(SpellCast *cast) {
  DWORDLONG castingItem = cast->caster == cast->casterUnit ? 0 : cast->caster;

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

    CGObject_C *container = ClntObjMgrObjectPtr(item->m_item->m_containedIn, __FILE__, __LINE__);
    if (!container || !container->GetBag()) {
      ConsoleWrite("Casting item's container not found", DEFAULT_COLOR);
      return;
    }

    UINT itemSlot = 0;
    while (itemSlot < container->GetBag()->NumSlots() && container->GetBag()->GetItem(itemSlot) != item->GetGUID()) {
      ++itemSlot;
    }
    if (itemSlot >= container->GetBag()->NumSlots()) {
      ConsoleWrite("Casting item not found in container", DEFAULT_COLOR);
      return;
    }

    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(item->m_item->m_owner, __FILE__, __LINE__));
    if (!player) {
      ConsoleWrite("Active player not found", DEFAULT_COLOR);
      return;
    }

    UINT packSlot = player->FindSlotIndex(item->m_item->m_containedIn);
    if (packSlot > 43 && packSlot != 0xFF) {
      ConsoleWrite("Object not in container belonging to active player", DEFAULT_COLOR);
      return;
    }

    const ItemStats *stats = g_itemDBCache.GetRecord(item->GetEntryID(), 0, 0, 0);
    if (!stats) {
      ConsoleWrite("Casting item doesn't have stats", DEFAULT_COLOR);
      return;
    }

    UINT spellIndex = 0;
    while (spellIndex < 5 && (stats->m_spellID[spellIndex] != cast->spellID || stats->m_spellTrigger[spellIndex])) {
      ++spellIndex;
    }
    if (spellIndex >= 5) {
      ConsoleWrite("Casting item doesn't have spell used", DEFAULT_COLOR);
      return;
    }

    castMsg.Put(static_cast<UINT>(CMSG_USE_ITEM));
    castMsg.Put(packSlot);
    castMsg.Put(itemSlot);
    castMsg.Put(spellIndex);
  } else {
    castMsg.Put(static_cast<UINT>(CMSG_CAST_SPELL));
    castMsg.Put(cast->spellID);
  }

  SpellPutCastTargets(cast, &castMsg);
  castMsg.Finalize();
  ClientServices_Send(&castMsg);

  if (caster->GetGUID() == ClntObjMgrGetActivePlayer() && (s_playerCast || !caster->GetCastingSpell())) {
    SpellVisualsHandleCastStart(cast->spellID, *cast, caster, 1000000, 4000, 0);
  }

  if (s_playerCast) {
    const SpellRec *spell = g_spellDB.GetRecord(cast->spellID);
    if (spell->m_startRecoveryCategory || spell->m_startRecoveryTime) {
      s_spellHistory[0].AddHistory(
          cast->spellID, 0, OsGetAsyncTimeMs(), 0, 0, OsGetAsyncTimeMs(), 0, false, spell->m_startRecoveryCategory, spell->m_startRecoveryTime
      );
      CGActionBar::UpdateCooldowns();
      CGSpellBook::UpdateCooldowns();
    }
  }

  if (cast->spellID == s_modalSpellID) {
    Spell_C_SetModal(0, 0);
  }
}

bool Spell_C_TargetSpell(CGUnit_C *caster, const SpellRec *srec) {
  s_needTargets = static_cast<WORD>(srec->m_targets);
  bool suppressTarget = false;

  switch (srec->m_implicitTargetA[0]) {
    case 1:
      if (s_needTargets & 0x400) {
        s_needTargets &= ~0x400;
      }
      break;
    case 6:
      s_needTargets |= 0x80;
      break;
    case 16:
      suppressTarget = true;
      break;
    case 21:
      s_needTargets |= 0x100;
      break;
    case 23:
      s_needTargets |= 0x800;
      break;
    case 25:
      s_needTargets |= 2;
      break;
    case 26:
      s_needTargets |= 0x4000;
      break;
    case 35:
      s_needTargets |= 8;
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

bool Spell_C_HaveSpellTokens(CGPlayer_C *player, const SpellRec *rec, bool report) {
  UINT index;
  for (index = 0; index < 2; ++index) {
    if (rec->m_totem[index] && !player->m_inventory.FindItemOfType(rec->m_totem[index], 0)) {
      if (report) {
        Spell_C_SpellFailed(rec->m_ID, 75, rec->m_totem[index], -1);
      }
      return false;
    }
  }

  for (index = 0; index < 8; ++index) {
    if (rec->m_reagent[index] && player->m_inventory.GetItemTypeCount(rec->m_reagent[index], 0) < rec->m_reagentCount[index]) {
      if (report) {
        Spell_C_SpellFailed(rec->m_ID, 56, rec->m_reagent[index], -1);
      }
      return false;
    }
  }
  return true;
}

struct FindAmmoData {
  int  ammoType;
  BYTE exoticAmmo;
};

static BOOL FindAmmoCallback(const CGItem_C *item, LPVOID param) {
  FindAmmoData *data = static_cast<FindAmmoData *>(param);
  if (item->GetClassID() != 6 || item->GetSubtypeID() != data->ammoType) {
    return 0;
  }
  return item->GetItemStaticFlag(ITEM_FLAG_EXOTIC) == static_cast<int>(data->exoticAmmo);
}

bool Spell_C_HaveEquippedSpellItems(CGPlayer_C *player, const SpellRec *rec, bool checkAmmo, bool report) {
  if (rec->m_attributesEx & 0x10 || rec->m_equippedItemClass < 0 || !rec->m_equippedItemSubclass) {
    return true;
  }

  CGItem_C *equipped = player->m_inventory.FindItemOfClass(rec->m_equippedItemClass, rec->m_equippedItemSubclass, 1);
  if (!equipped) {
    if (report) {
      Spell_C_SpellFailed(rec->m_ID, 13, rec->m_equippedItemClass, rec->m_equippedItemSubclass);
    }
    return false;
  }

  const ItemStats *stats = g_itemDBCache.GetRecord(equipped->GetEntryID(), 0, 0, 0);
  int              ammoType = stats ? stats->m_ammunitionType : 0;
  if (!checkAmmo || !ammoType) {
    return true;
  }

  const ItemSubClassRec *subclassRec = SDBItemSubclassGetSubClassRec(6, ammoType);
  FATALASSERT(subclassRec);
  if (subclassRec->m_flags & 0x40) {
    return true;
  }

  CGItem_C *quiver = player->m_inventory.FindItemOfClass(11, 1 << ammoType, 18);
  if (!quiver) {
    if (report) {
      Spell_C_SpellFailed(rec->m_ID, 28, ammoType, -1);
    }
    return false;
  }

  CGBag_C *bag = quiver->GetBag();
  FATALASSERT(bag);
  FindAmmoData data;
  data.ammoType = ammoType;
  data.exoticAmmo = (rec->m_attributes & 8) != 0;
  if (!bag->FindItem(FindAmmoCallback, &data, 0)) {
    if (report) {
      Spell_C_SpellFailed(rec->m_ID, 27, ammoType, -1);
    }
    return false;
  }
  return true;
}

bool RangeCheck(CGPlayer_C *caster, CGObject_C *target, int spellID) {
  float maxRange;
  float minRange;
  Spell_C_GetMinMaxRange(spellID, &minRange, &maxRange);

  if ((caster->GetPosition() - target->GetPosition()).SquaredMag() >= minRange * minRange &&
      (caster->GetPosition() - target->GetPosition()).SquaredMag() <= maxRange * maxRange)
  {
    return 1;
  }

  Spell_C_SpellFailed(spellID, 54, -1, -1);
  return 0;
}

bool RangeCheckSelected(CGPlayer_C *caster, const SpellRec *srec) {
  UINT checkRange;
  switch (srec->m_implicitTargetA[0]) {
    case 6:
      checkRange = 0x80;
      break;
    case 21:
      checkRange = 0x100;
      break;
    case 23:
      checkRange = 0x800;
      break;
    case 25:
      checkRange = 2;
      break;
    case 26:
      checkRange = 0x4000;
      break;
    case 35:
      checkRange = 8;
      break;
    default:
      return true;
  }

  CGObject_C *target = ClntObjMgrObjectPtr(CGGameUI::GetLockedTarget(), __FILE__, __LINE__);
  if (!target) {
    return true;
  }

  if (checkRange > 0x100) {
    if (!(target->GetType() & TYPE_GAMEOBJECT)) {
      return true;
    }
  } else if (checkRange == 0x100) {
    if (!(target->GetType() & TYPE_UNIT) || !caster->CanAssist(static_cast<CGUnit_C *>(target))) {
      return true;
    }
  } else if (checkRange == 8) {
    if (!(target->GetType() & TYPE_UNIT) || !caster->IsUnitInGroup(static_cast<CGUnit_C *>(target))) {
      return true;
    }
  } else if (checkRange == 0x80) {
    if (!(target->GetType() & TYPE_UNIT) || !caster->CanAttack(static_cast<CGUnit_C *>(target))) {
      return true;
    }
  }

  return RangeCheck(caster, target, srec->m_ID) != 0;
}

bool Spell_C_IsTargeting() {
  return s_needTargets != 0;
}

int Spell_C_GetTargettingSpell() {
  return s_needTargets ? s_spellCast.spellID : 0;
}

void Spell_C_StopTargeting() {
  Spell_C_CancelSpell(0, 0, SPELL_FAILED_ERROR);
}

void Spell_C_CancelSpell(bool failed, bool notifyServer, SPELL_FAILED_REASON reason) {
  CGUnit_C *caster = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(s_spellCast.caster, __FILE__, __LINE__));

  if (Spell_C_IsTargeting()) {
    s_needTargets = 0;
    CGSpellBook::UpdateSelection();
    CGActionBar::UpdateSelection();
  } else if (Spell_C_IsModal()) {
    if (notifyServer && !Spell_C_GetModalItem()) {
      CDataStore msg;
      msg.Put(static_cast<UINT>(CMSG_CANCEL_CAST));
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

  if (s_spellCast.caster == ClntObjMgrGetActivePlayer()) {
    if (failed) {
      FrameScript_SignalEvent(reason == SPELL_FAILED_INTERRUPTED || reason == SPELL_FAILED_INTERRUPTED_COMBAT ? 318 : 317);
    } else {
      FrameScript_SignalEvent(316);
    }
    s_spellCast.caster = 0;
  }
}

static void GameObjectStatsCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted) {
  if (reinterpret_cast<int>(arg) != s_spellCast.spellID) {
    return;
  }
  const GameObjectStats_C *stats = g_gameObjectDBCache.GetRecord(id, guid, 0, 0);
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

bool Spell_C_CastSpell(int spellID, const CGItem_C *item) {
  const SpellRec *spell = g_spellDB.GetRecord(spellID);
  if (!spell || (spell->m_attributes & 0x40)) {
    return false;
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return false;
  }

  if (player->GetUnitData()->channelSpell) {
    CDataStore msg;
    msg.Put(static_cast<int>(CMSG_CANCEL_CHANNELLING));
    msg.Put(player->GetUnitData()->channelSpell);
    msg.Finalize();
    ClientServices_Send(&msg);
  }

  if (spell->m_effect[0] == 78) {
    player->OnAttackIconPressed();
    return false;
  }

  if (spell->m_effect[0] == 47) {
    if (spell->m_effectMiscValue[0]) {
      CGCraftInfo::SetCraftType(SPELL_CAST_UI_INSCRIBING);
      return false;
    }

    const SkillLineAbilityRec *ability = SpellTableLookupAbility(player->GetUnitData()->race, player->GetUnitData()->classId, spellID);
    const SkillLineRec        *skillLine = ability ? g_skillLineDB.GetRecord(ability->m_skillLine) : 0;
    if (skillLine) {
      CGTradeSkillInfo::SetSkillLine(skillLine->m_ID);
    }
    return false;
  }

  if (spellID == s_modalSpellID) {
    SndInterfacePlayInterfaceSound("igPlayerInviteDecline");
    return false;
  }

  if (Spell_C_IsTargeting()) {
    Spell_C_SpellFailed(spellID, 59, -1, -1);
    return false;
  }

  UINT playerCast = 1;
  if (item) {
    playerCast = item->GetItemStaticFlag(ITEM_FLAG_PLAYERCAST);
  }

  if (Spell_C_IsModal()) {
    const SpellRec *modalSpell = g_spellDB.GetRecord(Spell_C_GetModalSpell());
    FATALASSERT(modalSpell);
    if (!(modalSpell->m_attributes & 0x404)) {
      Spell_C_SpellFailed(spellID, 59, -1, -1);
      return false;
    }
  }

  if (player->GetUnitData()->health <= 0 && !(spell->m_attributes & 0x800000)) {
    Spell_C_SpellFailed(spellID, 10, -1, -1);
    return false;
  }

  if (!Spell_C_HaveSpellTokens(player, spell, true) || !Spell_C_HaveEquippedSpellItems(player, spell, true, true) ||
      !player->CheckAndReportSpellInhibitFlags(spell, item) || !RangeCheckSelected(player, spell))
  {
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
  s_spellCast.overrideRank = -1;
  s_playerCast = playerCast;

  if ((player->GetSpellRank(spellID) >= 0 || spell->m_attributes & 0x404) && (!item || playerCast)) {
    Spell_C_SetModal(spellID, item);
  }

  UnitEffectPreloadSpellEffects(spellID);
  if (!Spell_C_TargetSpell(player, spell)) {
    if (s_needTargets & 0x80) {
      DWORDLONG target = CGGameUI::GetLockedTarget();
      s_needTargets = 0;
      Spell_C_SpellFailed(spellID, target ? 6 : 5, -1, -1);
      CGSpellBook::UpdateSelection();
      CGActionBar::UpdateSelection();
      return false;
    }

    CursorSetCursorMode(CAST_CURSOR);
    CursorModelSetSequence(CAST_ERROR_CURSOR);
    FATALASSERT(!s_spellWorldModel);
    s_spellWorldModelHousing = 0;
    s_spellWorldModelFacing = 0.0f;

    UINT effectIndex;
    for (effectIndex = 0; effectIndex < 3; ++effectIndex) {
      int effect = spell->m_effect[effectIndex];
      if (effect == 50 || effect == 76 || effect == 81) {
        break;
      }
    }

    if (effectIndex < 3) {
      if (spell->m_effect[effectIndex] == 81) {
        s_spellWorldModelHousing = 1;
      }

      const GameObjectStats_C *stats = g_gameObjectDBCache.GetRecord(
          spell->m_effectMiscValue[effectIndex], static_cast<UINT>(spellID) | 0xB000000000000000ui64,
          reinterpret_cast<DBCACHECALLBACKPROC>(GameObjectStatsCallback), reinterpret_cast<LPVOID>(spellID)
      );
      if (stats) {
        const GameObjectDisplayInfoRec *display = g_gameObjectDisplayInfoDB.GetRecord(stats->m_displayID);
        if (display) {
          NTempest::C3Vector pos = player->GetPosition();
          s_spellWorldModel = CWorld::ObjectCreate(display->m_modelName, pos, 0.0f, 0, 0, 0);
          CWorld::ObjectEnableCollision(s_spellWorldModel, 0);
        }
      }
    }
  }
  CGSpellBook::UpdateSelection();
  CGActionBar::UpdateSelection();
  return true;
}

bool Spell_C_CastSpell(LPCSTR name) {
  if (!SStrCmpI(name, "none", 0x7FFFFFFF)) {
    Spell_C_CancelSpell(1, 1, SPELL_FAILED_ERROR);
    return false;
  }
  return Spell_C_CastSpell(Spell_C_GetSpellByName(name), 0);
}

bool Spell_C_CanTargetObject(const CGObject_C *objectPtr) {
  return (s_needTargets & 0x4800) && (objectPtr->GetType() & TYPE_GAMEOBJECT) &&
         static_cast<const CGGameObject_C *>(objectPtr)->IsValidTargetForSpell(s_spellCast.caster, s_spellCast.spellID);
}

bool Spell_C_CanTargetObjects() {
  return (s_needTargets & 0x4800) != 0;
}

bool Spell_C_HandleSpriteClick(CGObject_C *object) {
  if (!s_needTargets || !object) {
    return 0;
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);

  if (object->GetGUID() == s_spellCast.casterUnit) {
    const SpellRec *spell = g_spellDB.GetRecord(s_spellCast.spellID);
    if (spell->m_attributesEx & 0x80000) {
      return 0;
    }
  }

  WORD oldTargets = s_spellCast.targets;
  WORD oldNeedTargets = s_needTargets;
  UINT handled = 0;

  if (object->GetType() & TYPE_UNIT) {
    CGUnit_C *unit = static_cast<CGUnit_C *>(object);
    if (unit->GetUnitData()->health > 0) {
      if (s_needTargets & 0x400) {
        return 0;
      }
    } else if (!(s_needTargets & 0x400)) {
      return 0;
    }

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
      s_spellCast.targets |= 2;
      s_spellCast.unitTarget = object->GetGUID();
      s_needTargets &= ~2;
    } else {
      return 0;
    }

    if (unit->GetUnitData()->health <= 0) {
      s_needTargets &= ~0x400;
    }
    handled = 1;
  } else if (object->GetType() & TYPE_ITEM) {
    if (!(s_needTargets & 0x4010)) {
      return 0;
    }
    s_spellCast.targets |= 0x10;
    s_spellCast.itemTarget = object->GetGUID();
    s_needTargets &= ~0x4010;
    handled = 1;
  } else if ((object->GetType() & TYPE_GAMEOBJECT) && (s_needTargets & 0x4800)) {
    s_spellCast.targets |= 0x800;
    s_spellCast.unitTarget = object->GetGUID();
    s_needTargets &= ~0x4800;
    handled = 1;
  } else {
    return 0;
  }

  if ((s_spellCast.targets & 2) && s_spellCast.unitTarget && ClntObjMgrObjectPtr(s_spellCast.unitTarget, __FILE__, __LINE__) &&
      !RangeCheck(player, ClntObjMgrObjectPtr(s_spellCast.unitTarget, __FILE__, __LINE__), s_spellCast.spellID))
  {
    s_spellCast.unitTarget = 0;
    s_spellCast.targets = oldTargets;
    s_needTargets = oldNeedTargets;
    return 0;
  }

  if (!s_needTargets) {
    CGSpellBook::UpdateSelection();
    CGActionBar::UpdateSelection();
    SendCast(&s_spellCast);
  }
  return handled;
}

bool Spell_C_HandleSpriteClick(const CSpriteClickEvent &evt) {
  return Spell_C_HandleSpriteClick(ClntObjMgrObjectPtr(evt.objectGUID, __FILE__, __LINE__));
}

bool Spell_C_CanTargetUnits() {
  return (s_needTargets & 0x58A) != 0;
}

bool Spell_C_CanTargetMe() {
  if (!(s_needTargets & 0x50A)) {
    return 0;
  }

  const SpellRec *spell = g_spellDB.GetRecord(s_spellCast.spellID);
  return spell && !(spell->m_attributesEx & 0x80000);
}

bool Spell_C_CanTargetParty() {
  return (s_needTargets & 0x408) != 0;
}

bool Spell_C_CanTargetFriends() {
  return (s_needTargets & 0x500) != 0;
}

bool Spell_C_CanTargetEnemies() {
  return (s_needTargets & 0x480) != 0;
}

bool Spell_C_CanTargetDead() {
  return (s_needTargets & 0x400) != 0;
}

bool Spell_C_CanTargetItems() {
  return (s_needTargets & 0x4010) != 0;
}

bool Spell_C_HandleTerrainClick(const CTerrainClickEvent &evt) {
  if (!s_needTargets) {
    return 0;
  }

  UINT handled = 0;
  if (s_needTargets & 0x20) {
    s_spellCast.sourceLocation = evt.point;
    s_spellCast.targets |= 0x20;
    s_needTargets &= ~0x20;
    handled = 1;
  } else if (s_needTargets & 0x40) {
    s_spellCast.destLocation = evt.point;
    s_spellCast.targets |= 0x40;
    s_needTargets &= ~0x40;
    handled = 1;
  }

  CGSpellBook::UpdateSelection();
  CGActionBar::UpdateSelection();
  if (handled && !s_needTargets) {
    SendCast(&s_spellCast);
  }
  return handled;
}

bool Spell_C_CanTargetTerrain() {
  return (s_needTargets & 0x60) != 0;
}

float Spell_C_GetSpellRadius() {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);

  const SpellRec       *spell = g_spellDB.GetRecord(s_spellCast.spellID);
  const SpellRadiusRec *radius = g_spellRadiusDB.GetRecord(spell->m_effectRadiusIndex[0]);
  float                 radius1 = radius ? radius->m_radius + player->GetUnitData()->level * radius->m_radiusPerLevel : 0.0f;
  radius = g_spellRadiusDB.GetRecord(spell->m_effectRadiusIndex[1]);
  float radius2 = radius ? radius->m_radius + player->GetUnitData()->level * radius->m_radiusPerLevel : 0.0f;
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

  if (object->GetType() & TYPE_UNIT) {
    CGUnit_C *unit = static_cast<CGUnit_C *>(object);
    if (unit->GetUnitData()->health > 0) {
      if (s_needTargets & 0x400) {
        return false;
      }
    } else if (!(s_needTargets & 0x400)) {
      return false;
    }

    bool valid = (s_needTargets & 0x8) && player->IsUnitInGroup(unit);
    if (!valid && (s_needTargets & 0x100)) {
      valid = player->CanAssist(unit);
    }
    if (!valid && (s_needTargets & 0x80)) {
      valid = player->CanAttack(unit);
    }
    if (!valid && (s_needTargets & 0x62)) {
      valid = true;
    }
    if (!valid) {
      return false;
    }

    const SpellRec *spell = g_spellDB.GetRecord(s_spellCast.spellID);
    if (spell && spell->m_targetCreatureType) {
      int type = unit->GetCreatureType();
      if (!type || !(spell->m_targetCreatureType & (1 << (type - 1)))) {
        return false;
      }
    }
  } else if (object->GetType() & TYPE_ITEM) {
    if (!(s_needTargets & 0x4010)) {
      return false;
    }
  } else if (object->GetType() & TYPE_GAMEOBJECT) {
    if (!(s_needTargets & 0x4800) || !static_cast<CGGameObject_C *>(object)->IsValidTargetForSpell(s_spellCast.caster, s_spellCast.spellID)) {
      return false;
    }
  } else {
    return false;
  }

  if (!checkRange) {
    return true;
  }

  float distance = (player->GetPosition() - object->GetPosition()).SquaredMag();
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

  float distance = (player->GetPosition() - evt.point).SquaredMag();
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
  if (!s_needTargets) {
    CGObject_C *caster = ClntObjMgrObjectPtr(s_spellCast.caster, __FILE__, __LINE__);
    if (caster) {
      s_spellWorldModelFacing = caster->GetFacing();
    }
  }

  return s_spellWorldModelFacing;
}

bool Spell_C_WorldObjectHousing() {
  return s_spellWorldModelHousing != 0;
}

void Spell_C_WorldObjectRotate() {
  s_spellWorldModelFacing += 1.5707964f;
  if (s_spellWorldModelFacing >= 6.2831855f) {
    s_spellWorldModelFacing -= 6.2831855f;
  }
}

static BOOL CCommand_Cast(LPCSTR, LPCSTR arguments) {
  Spell_C_CastSpell(arguments);
  return 1;
}

DWORDLONG Script_GetGUIDFromName(LPCSTR name);

static BOOL CastResultHandler(LPVOID, NETMESSAGE, DWORD, CDataStore *msg) {
  int  spellID;
  BYTE status;
  BYTE reason = 0;
  msg->Get(spellID);
  msg->Get(status);
  if (status == 2) {
    int arg1 = -1;
    int arg2 = -1;
    msg->Get(reason);
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

static void SpellStart(DWORDLONG casterGUID, DWORDLONG casterUnit, int spellID, CDataStore *msg) {
  WORD  spellCastFlags;
  DWORD castDelay;
  msg->Get(spellCastFlags);
  UnitEffectPreloadSpellEffects(spellID);
  msg->Get(castDelay);

  SpellCast cast;
  memset(&cast, 0, sizeof(cast));
  cast.overrideRank = -1;
  SpellGetCastTargets(&cast, msg);

  int ammoDisplayID = 0;
  int ammoInventoryType = 0;
  if (spellCastFlags & 0x10) {
    msg->Get(ammoDisplayID);
    msg->Get(ammoInventoryType);
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
  FATALASSERT(caster->GetType() & TYPE_UNIT);
  caster->ClearRangedStandTimer();
  if (ammoDisplayID) {
    caster->SetAmmoDisplay(ammoDisplayID, ammoInventoryType);
  }

  if (caster->GetGUID() == ClntObjMgrGetActivePlayer()) {
    DWORDLONG target = (srec->m_attributes & 0x400000) && (cast.targets & 2) && cast.unitTarget ? cast.unitTarget : CGGameUI::GetLockedTarget();
    if (srec->m_attributes & 0x400000) {
      caster->SaveTrackingTarget(target, TRACKTYPE_SPELLPRECAST, 0);
    }
    if (castDelay) {
      FrameScript_SignalEvent(0x13B, "%s%d", srec->m_name_lang[CURRENT_LANGUAGE], castDelay);
    }
  } else {
    SpellVisualsHandleCastStart(spellID, cast, caster, castDelay, castDelay, (spellCastFlags & 1) != 0);
    if ((cast.targets & 2) && cast.unitTarget == ClntObjMgrGetActivePlayer()) {
      CGUnit_C *player = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (player && !caster->CanAssist(player) && !CGGameUI::GetLockedTarget()) {
        CGGameUI::Target(caster->GetGUID(), 0);
      }
    }
  }
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
  if (static_cast<int>(time) > 0) {
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
    missReasons[i] = static_cast<MISS_REASON>(reason);
    msg->Get(missTargets[i]);
  }

  SpellCast cast;
  memset(&cast, 0, sizeof(cast));
  cast.overrideRank = -1;
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
  ASSERT(msg->IsRead());

  const SpellRec *srec = g_spellDB.GetRecord(spellID);
  if (!srec) {
    return;
  }

  CGObject_C *casterObject = ClntObjMgrObjectPtr(casterGUID, __FILE__, __LINE__);
  if (!casterObject) {
    CGObject_C *unitObject = ClntObjMgrObjectPtr(casterUnit, __FILE__, __LINE__);
    if (unitObject && (unitObject->GetType() & TYPE_GAMEOBJECT)) {
      CGGameObject_C *gameObject = static_cast<CGGameObject_C *>(unitObject);
      SpellVisualsHandleSpellStart(spellID, cast, gameObject, targets, (spellCastFlags & 8) != 0, true);
      if (missTargets.Count()) {
        SpellVisualsHandleSpellStart(spellID, cast, gameObject, missTargets, (spellCastFlags & 8) != 0, false);
      }
    }
    return;
  }

  FATALASSERT(casterObject->GetType() & TYPE_UNIT);
  CGUnit_C *caster = static_cast<CGUnit_C *>(casterObject);
  DWORD     currTime = OsGetAsyncTimeMs();
  bool      needsEvent = (srec->m_attributes & 0x2000000) != 0;
  if (srec->m_attributes & 2) {
    caster->SetRangedStandTimer();
  }
  SpellVisualsHandleSpellStartHits(spellID, cast, caster, targets, ammoDisplayID, ammoInventoryType, spellCastFlags);
  if (missTargets.Count()) {
    SpellVisualsHandleSpellStartMisses(spellID, cast, caster, missTargets, missReasons, ammoDisplayID, ammoInventoryType, spellCastFlags);
  }

  if (caster->GetGUID() == ClntObjMgrGetActivePlayer()) {
    UINT effect;
    for (effect = 0; effect < 3; ++effect) {
      if ((srec->m_effect[effect] == 33 || srec->m_effect[effect] == 59) && targets.Count() == 1) {
        CGObject_C *target = ClntObjMgrObjectPtr(targets[0], __FILE__, __LINE__);
        if (target && (target->GetType() & TYPE_GAMEOBJECT) && static_cast<CGGameObject_C *>(target)->GetType() == 3) {
          static_cast<CGPlayer_C *>(caster)->OnLootGameObject(targets[0], true);
        }
        break;
      }
    }

    int         itemID = 0;
    CGObject_C *unitObject = ClntObjMgrObjectPtr(casterUnit, __FILE__, __LINE__);
    if (unitObject && (unitObject->GetType() & TYPE_ITEM)) {
      itemID = unitObject->GetEntryID();
    }

    if (casterUnit == casterGUID) {
      s_spellHistory[0].AddHistory(
          spellID, 0, currTime, srec->m_recoveryTime, srec->m_category, currTime, srec->m_categoryRecoveryTime, needsEvent, 0, 0
      );
      CGActionBar::UpdateCooldowns();
      CGSpellBook::UpdateCooldowns();
    } else if (itemID) {
      const ItemStats *stats = g_itemDBCache.GetRecord(itemID, casterUnit, ItemStatsCooldownCallback, reinterpret_cast<LPVOID>(1));
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
    const CGUnitData *unitData = caster->GetUnitData();
    const DWORDLONG  &owner = unitData->charmedBy ? unitData->charmedBy : unitData->summonedBy;
    if (owner == ClntObjMgrGetActivePlayer()) {
      s_spellHistory[1].AddHistory(
          spellID, 0, currTime, srec->m_recoveryTime, srec->m_category, currTime, srec->m_categoryRecoveryTime, needsEvent,
          srec->m_startRecoveryCategory, srec->m_startRecoveryTime
      );
      CGPetInfo::UpdateCooldowns();
    }
  }

  if (static_cast<long>(currTime - s_cleanupTime) >= 0) {
    s_cleanupTime = currTime + 120000;
    for (i = 0; i < 2; ++i) {
      s_spellHistory[i].GarbageCollect(currTime);
    }
  }
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
    if (spell && (spell->m_attributesEx & 2)) {
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
  if (!spell) {
    return 1;
  }

  if (reason == 37) {
    CGGameUI::DisplayError((spell->m_attributes & 0x10) ? GERR_ABILITY_COOLDOWN : GERR_SPELL_COOLDOWN);
  } else if (reason == 44) {
    if (spell->m_powerType == -2) {
      CGGameUI::DisplayError(GERR_OUT_OF_HEALTH);
    } else {
      CGGameUI::DisplayError(s_gerrEnums[spell->m_powerType]);
    }
  } else if (reason == 54) {
    CGGameUI::DisplayError(GERR_SPELL_OUT_OF_RANGE);
  } else {
    CGGameUI::DisplayError(GERR_SPELL_FAILED_S, FrameScript_GetText(GetStringReason(reason), -1, GENDER_NOT_APPLICABLE));
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
  CGObject_C     *object = ClntObjMgrObjectPtr(itemGUID, __FILE__, __LINE__);
  if (spell && object && (object->GetType() & TYPE_ITEM)) {
    s_spellHistory[0].AddHistory(spellID, object->GetEntryID(), eventTime, 30000, 0, 0, 0, false, 0, 0);
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

  char message[128];
  SStrCopy(message, FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE), sizeof(message));
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
    SpellVisualsPlayKit(static_cast<CGUnit_C *>(object), id);
  }
  return 1;
}

static BOOL CCommand_Learn(LPCSTR command, LPCSTR arguments) {
  int spellID;
  if (isdigit(*arguments)) {
    spellID = SStrToInt(arguments);
  } else {
    if (!SStrCmpI(arguments, "all", 0x7FFFFFFF)) {
      ConsolePrintf("meh.");
      return 1;
    }
    spellID = Spell_C_GetSpellByName(arguments);
  }
  if (spellID > 0) {
    CDataStore msg;
    msg.Put(16);
    msg.Put(spellID);
    ClientServices_Send(&msg);
  }
  return 1;
}

static BOOL CCommand_Cooldown(LPCSTR command, LPCSTR arguments) {
  CDataStore msg;
  msg.Put(40);
  msg.Put(ClntObjMgrGetActivePlayer());
  ClientServices_Send(&msg);
  return 1;
}

static BOOL CCommand_CooldownPet(LPCSTR command, LPCSTR arguments) {
  CDataStore msg;
  msg.Put(40);
  msg.Put(CGPetInfo::GetPet());
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
  if (id >= 0) {
    CDataStore msg;
    msg.Put(41);
    msg.Put(id);
    msg.Put(level);
    msg.Finalize();
    ClientServices_Send(&msg);
  } else {
    ConsolePrintf("Unknown spell %s", arguments);
  }
  return 1;
}

static BOOL CCommand_SetSkill(LPCSTR command, LPCSTR arguments) {
  LPCSTR name = arguments;
  if (!isdigit(*name)) {
    ConsolePrintf("Unknown skill line");
    return 1;
  }
  int level = SStrToInt(name);
  while (*name && (isdigit(*name) || isspace(*name))) {
    ++name;
  }

  int skillID = 0;
  for (int i = 0; i < g_skillLineDB.GetNumRecords(); ++i) {
    const SkillLineRec *skill = g_skillLineDB.GetRecordByIndex(i);
    if (!SStrCmpI(skill->m_displayName_lang[CURRENT_LANGUAGE], name, SStrLen(name))) {
      skillID = skill->m_ID;
      break;
    }
  }
  if (!skillID) {
    ConsolePrintf("Unknown skill line");
    return 1;
  }

  CDataStore msg;
  msg.Put(457);
  msg.Put(skillID);
  msg.Put(level);
  ClientServices_Send(&msg);
  return 1;
}

static BOOL CCommand_CancelAura(LPCSTR, LPCSTR arguments) {
  CDataStore msg;
  msg.Put(297);
  msg.Put(SStrToInt(arguments));
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

static int Script_SpellIsTargeting(lua_State *L) {
  Spell_C_IsTargeting() ? lua_pushnumber(L, 1.0) : lua_pushnil(L);
  return 1;
}

static int Script_SpellCanTargetUnit(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: SpellCanTargetUnit(\"unit\")");
  }
  DWORDLONG         guid = Script_GetGUIDFromName(lua_tostring(L, 1));
  CSpriteClickEvent evt;
  evt.objectGUID = guid;
  evt.pos.x = 0.0f;
  evt.pos.y = 0.0f;
  if (guid && Spell_C_HandleSpriteRay(evt, true)) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_SpellTargetUnit(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: SpellCanTargetUnit(\"unit\")");
  }
  DWORDLONG guid = Script_GetGUIDFromName(lua_tostring(L, 1));
  if (guid) {
    CSpriteClickEvent evt;
    evt.objectGUID = guid;
    evt.pos.x = 0.0f;
    evt.pos.y = 0.0f;
    Spell_C_HandleSpriteClick(evt);
  }
  return 0;
}

static int Script_SpellStopTargeting(lua_State *L) {
  bool targeting = Spell_C_IsTargeting();
  Spell_C_StopTargeting();
  targeting ? lua_pushnumber(L, 1.0) : lua_pushnil(L);
  return 1;
}

void SpellRegisterScriptFunctions() {
  for (UINT i = 0; i < 4; ++i) {
    FrameScript_RegisterFunction(s_SpellScriptFunctions[i].name, s_SpellScriptFunctions[i].method);
  }
}

void SpellUnregisterScriptFunctions() {
  for (UINT i = 0; i < 4; ++i) {
    FrameScript_UnregisterFunction(s_SpellScriptFunctions[i].name);
  }
}

FrameScript_Method s_SpellScriptFunctions[4] = {
    {  "SpellIsTargeting",   Script_SpellIsTargeting},
    {"SpellCanTargetUnit", Script_SpellCanTargetUnit},
    {   "SpellTargetUnit",    Script_SpellTargetUnit},
    {"SpellStopTargeting", Script_SpellStopTargeting}
};

void Spell_C_CancelCombatSpell() {
  const SpellRec *spell = s_modalSpellID ? g_spellDB.GetRecord(s_modalSpellID) : 0;
  if (spell && (spell->m_attributes & 0x404)) {
    Spell_C_CancelSpell(0, 1, SPELL_FAILED_ERROR);
  }

  spell = s_modalSpellID ? g_spellDB.GetRecord(s_modalSpellID) : 0;
  if (spell && (spell->m_attributes & 0x404)) {
    Spell_C_CancelSpell(0, 1, SPELL_FAILED_ERROR);
  }

  spell = s_savedModalSpellID ? g_spellDB.GetRecord(s_savedModalSpellID) : 0;
  if (spell && (spell->m_attributes & 0x404)) {
    if (!s_savedModalItemID) {
      CDataStore msg;
      msg.Put(static_cast<UINT>(CMSG_CANCEL_CAST));
      msg.Put(s_savedModalSpellID);
      msg.Finalize();
      ClientServices_Send(&msg);
    }
    s_savedModalSpellID = 0;
    s_savedModalItemID = 0;
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
  msg.Put(static_cast<UINT>(CMSG_CANCEL_AURA));
  msg.Put(spellID);
  msg.Finalize();
  ClientServices_Send(&msg);
}

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
  s_spellHistory[0].ClearHistory();
  s_spellHistory[1].ClearHistory();
}

bool IsSpellAura(const SpellRec *rec) {
  for (UINT effect = 0; effect < 3; ++effect) {
    if (rec->m_effect[effect] == 6 || rec->m_effect[effect] == 35) {
      return 1;
    }
  }
  return 0;
}
