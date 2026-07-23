#include "Object/ObjectClient/Item_C.h"
#include "Object/ObjectClient/GameObject_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "Object/ItemStats.h"
#include "Console/ConsoleClient.h"
#include "DB/DBClient/AutoCode/SpellRec.h"
#include "DB/DBClient/AutoCode/SpellCastTimesRec.h"
#include "DB/DBClient/AutoCode/SpellRangeRec.h"
#include "DB/DBClient/AutoCode/SpellRadiusRec.h"
#include "DB/DBClient/AutoCode/ItemSubClassRec.h"
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "Ui/ActionBarFrame.h"
#include "Ui/SpellBookFrame.h"
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

enum SPELL_FAILED_REASON {
  SPELL_FAILED_ERROR = 14,
  SPELL_FAILED_INTERRUPTED = 17,
  SPELL_FAILED_INTERRUPTED_COMBAT = 18
};

enum CURSORANIMATIONS {
  POINT_CURSOR = 0
};

struct SpellCast {
  unsigned __int64   caster;
  unsigned __int64   casterUnit;
  int                spellID;
  unsigned short     targets;
  unsigned __int64   unitTarget;
  unsigned __int64   itemTarget;
  unsigned __int64   selectedTarget;
  NTempest::C3Vector sourceLocation;
  NTempest::C3Vector destLocation;
  float              destFacing;
  unsigned int       destZoneID;
  unsigned int       castTime;
  unsigned int       castEndTime;
  int                spellIndex;
  unsigned int       spellLevel;
  unsigned __int64   ammoItem;
  unsigned __int64   reflector;
  char               targetString[128];
  int                overrideRank;
  unsigned short     flags;
};

struct SPELLHISTORY : public TSLinkedNode<SPELLHISTORY> {
  int           spellID;
  int           itemID;
  unsigned long recoveryStart;
  unsigned int  recoveryTime;
  int           category;
  unsigned long categoryRecoveryStart;
  unsigned int  categoryRecoveryTime;
  bool          onHold;
  int           startRecoveryCategory;
  unsigned int  startRecoveryTime;
};

class SpellHistory {
 public:
  void AddHistory(
      int           spellID,
      int           itemID,
      unsigned long recoveryStart,
      unsigned int  recoveryTime,
      int           category,
      unsigned long categoryRecoveryStart,
      unsigned int  categoryRecoveryTime,
      bool          onHold,
      int           startRecoveryCategory,
      unsigned int  startRecoveryTime
  );
  int GetCooldown(int spellID, int itemID, unsigned int *duration, unsigned long *startTime, unsigned int *enable);

 protected:
  TSList<SPELLHISTORY, TSGetLink<SPELLHISTORY> > m_spellHistory;
  TSList<SPELLHISTORY, TSGetLink<SPELLHISTORY> > m_freeList;
};

static SpellCast        s_spellCast;
static unsigned short   s_needTargets;
static int              s_modalSpellID;
static int              s_savedModalSpellID;
static unsigned __int64 s_modalItemID;
static unsigned __int64 s_savedModalItemID;
static unsigned int     s_playerCast;
static char             s_spellTargetString[128];
static unsigned int     s_spellWorldModel;
static float            s_spellWorldModelFacing;
static unsigned int     s_spellWorldModelHousing;
static SpellHistory     s_spellHistory[2];

void __fastcall CursorSetCursorMode(CURSORANIMATIONS mode);
void __fastcall CursorResetCursor(int force);
void            SendCast(SpellCast *cast);
void __fastcall SpellPutCastTargets(SpellCast *cast, CDataStore *msg);
void __fastcall Spell_C_SpellFailed(int spellID, unsigned int reason, int arg1, int arg2);
void __fastcall SpellVisualsHandleCastStop(int id, CGUnit_C *caster, unsigned char status, unsigned char reason);
void __fastcall
SpellVisualsHandleCastStart(int id, SpellCast &cast, CGUnit_C *caster, unsigned int duration, unsigned int animDuration, unsigned int wasProc);
void __fastcall                   UnitCombatLogSpellFail(CGUnit_C *caster, int spellID, const char *message);
void __fastcall                   Spell_C_CancelSpell(unsigned int failed, unsigned int notifyServer, SPELL_FAILED_REASON reason);
bool __fastcall                   Spell_C_IsTargeting();
bool __fastcall                   Spell_C_HaveSpellTokens(CGPlayer_C *player, const SpellRec *spell, bool report);
bool __fastcall                   Spell_C_HaveEquippedSpellItems(CGPlayer_C *player, const SpellRec *spell, bool checkAmmo, bool report);
bool __fastcall                   RangeCheckSelected(CGPlayer_C *caster, const SpellRec *srec);
bool __fastcall                   Spell_C_TargetSpell(CGUnit_C *caster, const SpellRec *srec);
void __fastcall                   UnitEffectPreloadSpellEffects(int spellID);
const ItemSubClassRec *__fastcall SDBItemSubclassGetSubClassRec(unsigned int classID, unsigned int subClassID);
bool __fastcall                   Spell_C_HandleSpriteClick(CGObject_C *object);

void SpellHistory::AddHistory(
    int           spellID,
    int           itemID,
    unsigned long recoveryStart,
    unsigned int  recoveryTime,
    int           category,
    unsigned long categoryRecoveryStart,
    unsigned int  categoryRecoveryTime,
    bool          onHold,
    int           startRecoveryCategory,
    unsigned int  startRecoveryTime
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

int SpellHistory::GetCooldown(int spellID, int itemID, unsigned int *duration, unsigned long *startTime, unsigned int *enable) {
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

  unsigned long now = OsGetAsyncTimeMs();
  unsigned long latestEnd = now;
  SPELLHISTORY *history;
  for (history = m_spellHistory.Head(); history; history = m_spellHistory.Next(history)) {
    if (history->spellID == spellID && history->itemID == itemID && history->recoveryTime) {
      unsigned long start = history->onHold ? now : history->recoveryStart;
      unsigned long end = start + history->recoveryTime;
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
      unsigned long start = history->onHold ? now : history->categoryRecoveryStart;
      unsigned long end = start + history->categoryRecoveryTime;
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
      unsigned long start = history->onHold ? now : history->recoveryStart;
      unsigned long end = start + history->startRecoveryTime;
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

static const ItemSubClassRec* FindAnyItemSubclassRec(int classID, unsigned int subclassMask) {
    // TODO: implement
    return 0;
}

static const char* GetStringReason(unsigned char reason) {
    // TODO: implement
    return 0;
}

static void SpellMissingItemCallback(int id, const unsigned __int64& guid, void* arg, unsigned char granted) {
    // TODO: implement
}

void __fastcall Spell_C_SpellFailed(int spellID, unsigned int reason, int arg1, int arg2) {
  char            shapes[512];
  char            processedmessage[256];
  char            token[64];
  char            message[128];
  unsigned int    numEntries = 0;
  CGPlayer_C     *playerPtr = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  SpellRec       *spell = g_spellDB.GetRecord(spellID);
  int             isPet = 0;
  int             first = 1;
  GAME_ERROR_TYPE error = static_cast<GAME_ERROR_TYPE>(39);

  FrameScript_SignalEvent(370);
  if (spell) {
    SndInterfacePlaySpellFizzleSound(spellID, playerPtr);
    switch (reason) {
      case 37:
        if (spell->m_powerType == 10 || spell->m_powerType == 11) {
          error = static_cast<GAME_ERROR_TYPE>(42);
        } else if (spell->m_powerType == 4 || spell->m_powerType == 9) {
          error = static_cast<GAME_ERROR_TYPE>(41);
        } else {
          error = static_cast<GAME_ERROR_TYPE>((spell->m_attributes & 0x10) ? 44 : 43);
        }
        break;
      case 21:
        error = static_cast<GAME_ERROR_TYPE>(40);
        break;
      case 16:
        error = static_cast<GAME_ERROR_TYPE>(149);
        break;
      case 73:
        error = static_cast<GAME_ERROR_TYPE>(154);
        break;
      case 75:
        error = static_cast<GAME_ERROR_TYPE>(190);
        break;
      case 56:
        error = static_cast<GAME_ERROR_TYPE>(191);
        break;
      case 12:
        error = static_cast<GAME_ERROR_TYPE>(192);
        break;
      case 13:
        error = static_cast<GAME_ERROR_TYPE>(193);
        break;
      case 5:
        error = static_cast<GAME_ERROR_TYPE>(168);
        break;
      case 54:
        error = static_cast<GAME_ERROR_TYPE>(277);
        break;
      case 27:
        error = static_cast<GAME_ERROR_TYPE>(279);
        break;
      case 51:
        error = static_cast<GAME_ERROR_TYPE>(194);
        break;
      case 6:
        error = static_cast<GAME_ERROR_TYPE>((spell->m_targets & 0x10) ? 287 : 142);
        break;
      case 85:
        error = static_cast<GAME_ERROR_TYPE>(291);
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

  const char *failureToken;
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

  shapes[0] = 0;
  processedmessage[0] = 0;
  if (reason == 13 || reason == 27 || reason == 28 || reason == 29 || reason == 51 || reason == 56 || reason == 57 || reason == 75) {
    SStrPrintf(processedmessage, sizeof(processedmessage), message, arg1, arg2);
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

static void SetItemCooldown(int itemID, int spellID, unsigned long startTime, unsigned char needsEvent) {
    // TODO: implement
}

void __fastcall Spell_C_SetCooldownLeft(
    int  spellID,
    int  itemID,
    int  category,
    int  recoveryLeft,
    int  categoryRecoveryLeft,
    bool needsEvent,
    int  isPet,
    int  startRecoveryTimeLeft
) {
  unsigned int     spellRecoveryTime = 0;
  unsigned long    categoryRecoveryStart = 0;
  unsigned int     categoryRecoveryTime = 0;
  SpellRec        *srec = g_spellDB.GetRecord(spellID);
  unsigned long    now = OsGetAsyncTimeMs();
  const ItemStats *stats = 0;
  unsigned long    spellRecoveryStart = 0;

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
    if (spellRecoveryTime <= static_cast<unsigned int>(recoveryLeft)) {
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
    if (categoryRecoveryTime <= static_cast<unsigned int>(categoryRecoveryLeft)) {
      categoryRecoveryTime = categoryRecoveryLeft;
    }
    categoryRecoveryStart = now - categoryRecoveryTime + categoryRecoveryLeft;
  }

  s_spellHistory[isPet].AddHistory(
      spellID, itemID, spellRecoveryStart, spellRecoveryTime, category, categoryRecoveryStart, categoryRecoveryTime, needsEvent, 0, 0
  );
}

static void __fastcall ItemStatsCooldownCallback(int id, const unsigned __int64 &, void *, bool granted) {
  if (!granted) {
    return;
  }
  const ItemStats *stats = g_itemDBCache.GetRecord(id, 0, 0, 0);
  if (!stats) {
    return;
  }
  int index;
  for (index = 0; index < 5; ++index) {
    const SpellRec *srec = g_spellDB.GetRecord(stats->m_spellID[index]);
    if (srec && !stats->m_spellTrigger[index]) {
      unsigned int selfCooldown = stats->m_spellCooldown[index] < 0 ? srec->m_recoveryTime : stats->m_spellCooldown[index];
      Spell_C_SetCooldownLeft(srec->m_ID, id, stats->m_spellCategory[index], selfCooldown, stats->m_spellCategoryCooldown[index], true, 0, 0);
    }
  }
}

int __fastcall Spell_C_GetSpellCooldown(int spell, int isPet, unsigned int *duration, unsigned long *startTime, unsigned int *enable) {
  return s_spellHistory[isPet].GetCooldown(spell, 0, duration, startTime, enable);
}

static void __fastcall ItemCheckCooldownCallback(int, const unsigned __int64 &, void *, bool granted) {
  if (granted) {
    CGSpellBook::UpdateCooldowns();
    CGActionBar::UpdateCooldowns();
  }
}

int __fastcall Spell_C_GetItemCooldown(int itemID, unsigned int *duration, unsigned long *startTime, unsigned int *enable) {
  unsigned __int64 guid = ClntObjMgrGetActivePlayer();
  const ItemStats *stats = g_itemDBCache.GetRecord(itemID, guid, reinterpret_cast<DBCACHECALLBACKPROC>(ItemCheckCooldownCallback), 0);
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

int __fastcall Spell_C_NeedsCooldownEvent(const SpellRec* srec, int isPet) {
    // TODO: implement
    return 0;
}

int __fastcall Spell_C_NeedsCooldownEvent(int itemID) {
    // TODO: implement
    return 0;
}

static void Spell_C_CooldownEventTriggered(int spellID, unsigned long receivedTime, int isPet, int clear) {
    // TODO: implement
}

static void Spell_C_ClearCooldowns(int isPet) {
    // TODO: implement
}

int __fastcall Spell_C_GetSpellByName(const char* name) {
    // TODO: implement
    return 0;
}

int __fastcall Spell_C_GetSpellLevel(int id, int isPet) {
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (isPet) {
    if (!unit) {
      return 0;
    }

    const CGUnitData *unitData = unit->GetUnitData();
    unsigned __int64  pet = unitData->charm ? unitData->charm : unitData->summon;
    unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(pet, __FILE__, __LINE__));
  }

  return unit ? unit->GetSpellLevel(id) : 0;
}

int __fastcall Spell_C_GetManaCost(int id, int isPet) {
    // TODO: implement
    return 0;
}

int __fastcall Spell_C_GetManaCostPerSecond(int id, int isPet) {
    // TODO: implement
    return 0;
}

int __fastcall Spell_C_GetCastTime(int id, int isPet) {
  SpellRec *spellRec = g_spellDB.GetRecord(id);
  if (!spellRec) {
    return 0;
  }

  SpellCastTimesRec *castTime = g_spellCastTimesDB.GetRecord(spellRec->m_castingTimeIndex);
  if (!castTime) {
    return 0;
  }

  int result = castTime->m_base + Spell_C_GetSpellLevel(id, isPet) * castTime->m_perLevel;
  return result > castTime->m_minimum ? result : castTime->m_minimum;
}

void __fastcall Spell_C_GetMinMaxRange(int id, float *min, float *max) {
  *min = 0.0f;
  *max = 0.0f;

  SpellRec *spell = g_spellDB.GetRecord(id);
  if (!spell) {
    return;
  }

  SpellRangeRec *range = g_spellRangeDB.GetRecord(spell->m_rangeIndex);
  if (!range) {
    return;
  }

  if (spell->m_attributes & 0x404) {
    *max = 100.0f;
  } else if (range->m_flags & 1) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      CGUnit_C *target = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(player->GetUnitData()->target, __FILE__, __LINE__));
      float     targetReach = target ? target->GetUnitData()->combatReach + target->GetUnitData()->boundingRadius : range->m_rangeMax;
      *max = player->GetUnitData()->combatReach + player->GetUnitData()->boundingRadius + targetReach + 1.3333334f;
      *min = 0.0f;
    }
  } else {
    *min = range->m_rangeMin;
    *max = range->m_rangeMax;
  }
}

void __fastcall Spell_C_GetMinMaxPoints(const SpellRec *srec, int effectIndex, int *min, int *max, unsigned int level, int isPet) {
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

void __fastcall Spell_C_SetModal(int spellID, const CGItem_C *item) {
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

int __fastcall Spell_C_GetModalSpell() {
  return s_modalSpellID;
}

const unsigned __int64 &__fastcall Spell_C_GetModalItem() {
  return s_modalItemID;
}

bool __fastcall Spell_C_IsModal() {
  return s_modalSpellID != 0;
}

const unsigned __int64 &__fastcall Spell_C_GetCurrentCaster() {
  return s_spellCast.caster;
}

const unsigned __int64 &__fastcall Spell_C_GetCurrentTarget() {
  return s_spellCast.unitTarget;
}

void SendCast(SpellCast *cast) {
  unsigned __int64 castingItem = cast->caster == cast->casterUnit ? 0 : cast->caster;

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

    unsigned int itemSlot = 0;
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

    unsigned int packSlot = player->FindSlotIndex(item->m_item->m_containedIn);
    if (packSlot > 43 && packSlot != 0xFF) {
      ConsoleWrite("Object not in container belonging to active player", DEFAULT_COLOR);
      return;
    }

    const ItemStats *stats = g_itemDBCache.GetRecord(item->GetEntryID(), 0, 0, 0);
    if (!stats) {
      ConsoleWrite("Casting item doesn't have stats", DEFAULT_COLOR);
      return;
    }

    unsigned int spellIndex = 0;
    while (spellIndex < 5 && (stats->m_spellID[spellIndex] != cast->spellID || stats->m_spellTrigger[spellIndex])) {
      ++spellIndex;
    }
    if (spellIndex >= 5) {
      ConsoleWrite("Casting item doesn't have spell used", DEFAULT_COLOR);
      return;
    }

    castMsg.Put(static_cast<unsigned int>(CMSG_USE_ITEM));
    castMsg.Put(packSlot);
    castMsg.Put(itemSlot);
    castMsg.Put(spellIndex);
  } else {
    castMsg.Put(static_cast<unsigned int>(CMSG_CAST_SPELL));
    castMsg.Put(cast->spellID);
  }

  SpellPutCastTargets(cast, &castMsg);
  castMsg.Finalize();
  ClientServices_Send(&castMsg);

  if (caster->GetGUID() == ClntObjMgrGetActivePlayer() && (s_playerCast || !caster->GetCastingSpell())) {
    SpellVisualsHandleCastStart(cast->spellID, *cast, caster, 1000000, 4000, 0);
  }

  if (s_playerCast) {
    SpellRec *spell = g_spellDB.GetRecord(cast->spellID);
    if (spell->m_startRecoveryCategory || spell->m_startRecoveryTime) {
      s_spellHistory[0].AddHistory(
          cast->spellID, 0, OsGetAsyncTimeMs(), 0, 0, OsGetAsyncTimeMs(), 0, false, spell->m_startRecoveryCategory, spell->m_startRecoveryTime
      );
      CGSpellBook::UpdateCooldowns();
      CGActionBar::UpdateCooldowns();
    }
  }

  if (cast->spellID == s_modalSpellID) {
    Spell_C_SetModal(0, 0);
  }
}

bool __fastcall Spell_C_TargetSpell(CGUnit_C *caster, const SpellRec *srec) {
  s_needTargets = static_cast<unsigned short>(srec->m_targets);
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

  return Spell_C_HandleSpriteClick(ClntObjMgrObjectPtr(caster->GetUnitData()->target, __FILE__, __LINE__));
}

bool __fastcall Spell_C_HaveSpellTokens(CGPlayer_C *player, const SpellRec *spell, bool report) {
  unsigned int index;
  for (index = 0; index < 2; ++index) {
    if (spell->m_totem[index] && !player->m_inventory.FindItemOfType(spell->m_totem[index], 0)) {
      if (report) {
        Spell_C_SpellFailed(spell->m_ID, 75, spell->m_totem[index], -1);
      }
      return false;
    }
  }

  for (index = 0; index < 8; ++index) {
    if (spell->m_reagent[index] && player->m_inventory.GetItemTypeCount(spell->m_reagent[index], 0) < spell->m_reagentCount[index]) {
      if (report) {
        Spell_C_SpellFailed(spell->m_ID, 56, spell->m_reagent[index], -1);
      }
      return false;
    }
  }
  return true;
}

struct FindAmmoData {
  int          ammoType;
  unsigned int exoticAmmo;
};

static int __fastcall FindAmmoCallback(const CGItem_C *item, void *param) {
  FindAmmoData *data = static_cast<FindAmmoData *>(param);
  if (item->GetClassID() != 6 || item->GetSubtypeID() != data->ammoType) {
    return 0;
  }
  return item->GetItemStaticFlag(ITEM_FLAG_EXOTIC) == static_cast<int>(data->exoticAmmo);
}

bool __fastcall Spell_C_HaveEquippedSpellItems(CGPlayer_C *player, const SpellRec *spell, bool checkAmmo, bool report) {
  if (spell->m_attributesEx & 0x10 || spell->m_equippedItemClass < 0 || !spell->m_equippedItemSubclass) {
    return true;
  }

  CGItem_C *equipped = player->m_inventory.FindItemOfClass(spell->m_equippedItemClass, spell->m_equippedItemSubclass, 1);
  if (!equipped) {
    if (report) {
      Spell_C_SpellFailed(spell->m_ID, 13, spell->m_equippedItemClass, spell->m_equippedItemSubclass);
    }
    return false;
  }

  const unsigned __int64 noGuid = 0;
  const ItemStats       *stats = g_itemDBCache.GetRecord(equipped->GetEntryID(), noGuid, 0, 0);
  int                    ammoType = stats ? stats->m_ammunitionType : 0;
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
      Spell_C_SpellFailed(spell->m_ID, 28, ammoType, -1);
    }
    return false;
  }

  CGBag_C *bag = quiver->GetBag();
  FATALASSERT(bag);
  FindAmmoData data;
  data.ammoType = ammoType;
  data.exoticAmmo = (spell->m_attributes & 8) != 0;
  if (!bag->FindItem(FindAmmoCallback, &data, 0)) {
    if (report) {
      Spell_C_SpellFailed(spell->m_ID, 27, ammoType, -1);
    }
    return false;
  }
  return true;
}

unsigned int __fastcall RangeCheck(CGPlayer_C *caster, CGObject_C *target, int spellID) {
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

bool __fastcall RangeCheckSelected(CGPlayer_C *caster, const SpellRec *srec) {
  unsigned int checkRange;
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

  CGObject_C *target = ClntObjMgrObjectPtr(caster->GetUnitData()->target, __FILE__, __LINE__);
  if (!target) {
    return true;
  }

  if (checkRange > 0x100) {
    if (!(target->GetType() & TYPE_GAMEOBJECT)) {
      return true;
    }
  } else if (checkRange == 0x100) {
    if (!(target->GetType() & TYPE_UNIT) || !caster->CanCooperate(static_cast<CGUnit_C *>(target))) {
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

bool __fastcall Spell_C_IsTargeting() {
  return s_needTargets != 0;
}

int __fastcall Spell_C_GetTargettingSpell() {
  return s_needTargets ? s_spellCast.spellID : 0;
}

void __fastcall Spell_C_StopTargeting() {
  Spell_C_CancelSpell(0, 0, SPELL_FAILED_ERROR);
}

void __fastcall Spell_C_CancelSpell(unsigned int failed, unsigned int notifyServer, SPELL_FAILED_REASON reason) {
  CGUnit_C *caster = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(s_spellCast.caster, __FILE__, __LINE__));

  if (Spell_C_IsTargeting()) {
    s_needTargets = 0;
    CGSpellBook::UpdateSelection();
    CGActionBar::UpdateSelection();
  } else if (Spell_C_IsModal()) {
    if (notifyServer && !Spell_C_GetModalItem()) {
      CDataStore msg;
      msg.Put(static_cast<unsigned int>(CMSG_CANCEL_CAST));
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

static void GameObjectStatsCallback(int id, const unsigned __int64& guid, void* arg, unsigned char granted) {
    // TODO: implement
}

bool __fastcall Spell_C_CastSpell(int spellID, const CGItem_C *item) {
  SpellRec *spell = g_spellDB.GetRecord(spellID);
  if (!spell || (spell->m_attributes & 0x40)) {
    return false;
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return false;
  }

  if (spell->m_castUI == 78) {
    player->OnAttackIconPressed();
    return false;
  }

  if (spellID == static_cast<int>(s_spellWorldModel)) {
    SndInterfacePlayInterfaceSound("igPlayerInviteDecline");
    return false;
  }

  if (Spell_C_IsTargeting()) {
    Spell_C_SpellFailed(spellID, 59, -1, -1);
    return false;
  }

  unsigned int playerCast = 1;
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
      !RangeCheckSelected(player, spell))
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
  s_spellCast.selectedTarget = player->GetUnitData()->target;
  s_spellCast.overrideRank = -1;
  s_playerCast = playerCast;

  if ((player->GetSpellRank(spellID) >= 0 || spell->m_attributes & 0x404) && (!item || playerCast)) {
    Spell_C_SetModal(spellID, item);
  }

  UnitEffectPreloadSpellEffects(spellID);
  Spell_C_TargetSpell(player, spell);
  CGSpellBook::UpdateSelection();
  CGActionBar::UpdateSelection();
  return true;
}

unsigned int __fastcall Spell_C_CanTargetObject(CGObject_C *objectPtr) {
  return (s_needTargets & 0x4800) && (objectPtr->GetType() & TYPE_GAMEOBJECT) &&
         static_cast<CGGameObject_C *>(objectPtr)->IsValidTargetForSpell(s_spellCast.caster, s_spellCast.spellID);
}

unsigned int __fastcall Spell_C_CanTargetObjects() {
  return (s_needTargets & 0x4800) != 0;
}

bool __fastcall Spell_C_HandleSpriteClick(CGObject_C *object) {
  if (!s_needTargets || !object) {
    return 0;
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);

  if (object->GetGUID() == s_spellCast.caster) {
    SpellRec *spell = g_spellDB.GetRecord(s_spellCast.spellID);
    if (spell->m_attributes & 0x80000) {
      return 0;
    }
  }

  unsigned short oldTargets = s_spellCast.targets;
  unsigned short oldNeedTargets = s_needTargets;
  unsigned int   handled = 0;

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
    } else if ((s_needTargets & 0x100) && player->CanCooperate(unit)) {
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
    CGActionBar::UpdateSelection();
    CGSpellBook::UpdateSelection();
    SendCast(&s_spellCast);
  }
  return handled;
}

bool __fastcall Spell_C_HandleSpriteClick(const CSpriteClickEvent &evt) {
  return Spell_C_HandleSpriteClick(ClntObjMgrObjectPtr(evt.objectGUID, __FILE__, __LINE__));
}

unsigned int __fastcall Spell_C_CanTargetUnits() {
  return (s_needTargets & 0x58A) != 0;
}

unsigned int __fastcall Spell_C_CanTargetMe() {
  if (!(s_needTargets & 0x50A)) {
    return 0;
  }

  const SpellRec *spell = g_spellDB.GetRecord(s_spellCast.spellID);
  return spell && !(spell->m_attributes & 0x80000);
}

unsigned int __fastcall Spell_C_CanTargetParty() {
  return (s_needTargets & 0x408) != 0;
}

unsigned int __fastcall Spell_C_CanTargetFriends() {
  return (s_needTargets & 0x500) != 0;
}

unsigned int __fastcall Spell_C_CanTargetEnemies() {
  return (s_needTargets & 0x480) != 0;
}

unsigned int __fastcall Spell_C_CanTargetDead() {
  return (s_needTargets & 0x400) != 0;
}

unsigned int __fastcall Spell_C_CanTargetItems() {
  return (s_needTargets & 0x4010) != 0;
}

unsigned int __fastcall Spell_C_HandleTerrainClick(CTerrainClickEvent &evt) {
  if (!s_needTargets) {
    return 0;
  }

  unsigned int handled = 0;
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

  CGActionBar::UpdateSelection();
  CGSpellBook::UpdateSelection();
  if (handled && !s_needTargets) {
    SendCast(&s_spellCast);
  }
  return handled;
}

unsigned int __fastcall Spell_C_CanTargetTerrain() {
  return (s_needTargets & 0x60) != 0;
}

float __fastcall Spell_C_GetSpellRadius() {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);

  SpellRec       *spell = g_spellDB.GetRecord(s_spellCast.spellID);
  SpellRadiusRec *radius = g_spellRadiusDB.GetRecord(spell->m_effectRadiusIndex[0]);
  float           radius1 = radius ? radius->m_radius + player->GetUnitData()->level * radius->m_radiusPerLevel : 0.0f;
  radius = g_spellRadiusDB.GetRecord(spell->m_effectRadiusIndex[1]);
  float radius2 = radius ? radius->m_radius + player->GetUnitData()->level * radius->m_radiusPerLevel : 0.0f;
  return radius1 > radius2 ? radius1 : radius2;
}

bool __fastcall Spell_C_HandleSpriteRay(const CSpriteClickEvent &evt, bool checkRange) {
  CGObject_C *object = ClntObjMgrObjectPtr(evt.objectGUID, __FILE__, __LINE__);
  if (!object) {
    return false;
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);

  if (object->GetGUID() == s_spellCast.casterUnit) {
    SpellRec *spell = g_spellDB.GetRecord(s_spellCast.spellID);
    if (spell->m_attributes & 0x80000) {
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

    SpellRec *spell = g_spellDB.GetRecord(s_spellCast.spellID);
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

bool __fastcall Spell_C_HandleTerrainRay(const CTerrainClickEvent &evt, bool checkRange) {
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

unsigned int __fastcall Spell_C_WaitingForStringInput() {
  return (s_needTargets >> 13) & 1;
}

int __fastcall Spell_C_TargetTradeItem(int tradeIndex) {
    // TODO: implement
    return 0;
}

unsigned int __fastcall Spell_C_WorldObjectCursor() {
  return s_spellWorldModel;
}

float __fastcall Spell_C_WorldObjectFacing() {
  if (!s_needTargets) {
    CGObject_C *caster = ClntObjMgrObjectPtr(s_spellCast.caster, __FILE__, __LINE__);
    if (caster) {
      s_spellWorldModelFacing = caster->GetFacing();
    }
  }

  return s_spellWorldModelFacing;
}

bool __fastcall Spell_C_WorldObjectHousing() {
  return s_spellWorldModelHousing != 0;
}

void __fastcall Spell_C_WorldObjectRotate() {
    // TODO: implement
}

static int CCommand_Cast(const char*, const char* arguments) {
    // TODO: implement
    return 0;
}

unsigned __int64 __fastcall Script_GetGUIDFromName(const char *name);

static int CastResultHandler(void*, NETMESSAGE, unsigned long, CDataStore* msg) {
    // TODO: implement
    return 0;
}

static void SpellStart(unsigned __int64 casterGUID, unsigned __int64 casterUnit, int spellID, CDataStore* msg) {
    // TODO: implement
}

static int SpellDelayed(void*, NETMESSAGE, unsigned long, CDataStore* msg) {
    // TODO: implement
    return 0;
}

static int SpellChannelStart(void*, NETMESSAGE, unsigned long, CDataStore* msg) {
    // TODO: implement
    return 0;
}

static int SpellChannelUpdate(void*, NETMESSAGE, unsigned long, CDataStore* msg) {
    // TODO: implement
    return 0;
}

static int SpellAddDynamicTarget(void*, NETMESSAGE, unsigned long, CDataStore* msg) {
    // TODO: implement
    return 0;
}

static void SpellGo(const unsigned __int64& casterGUID, const unsigned __int64& casterUnit, int spellID, CDataStore* msg) {
    // TODO: implement
}

static int SpellStartHandler(void*, NETMESSAGE msgID, unsigned long, CDataStore* msg) {
    // TODO: implement
    return 0;
}

static int SpellFailedHandler(void*, NETMESSAGE, unsigned long, CDataStore* msg) {
    // TODO: implement
    return 0;
}

static int PetSpellFailedHandler(void*, NETMESSAGE, unsigned long, CDataStore* msg) {
    // TODO: implement
    return 0;
}

static int SpellCooldownHandler(void*, NETMESSAGE, unsigned long eventTime, CDataStore* msg) {
    // TODO: implement
    return 0;
}

static int ItemCooldownHandler(void*, NETMESSAGE, unsigned long eventTime, CDataStore* msg) {
    // TODO: implement
    return 0;
}

static int CooldownEvent(void*, NETMESSAGE msgID, unsigned long timeReceived, CDataStore* msg) {
    // TODO: implement
    return 0;
}

static int CooldownCheat(void*, NETMESSAGE, unsigned long, CDataStore* msg) {
    // TODO: implement
    return 0;
}

static int PetTameFailure(void*, NETMESSAGE, unsigned long, CDataStore* msg) {
    // TODO: implement
    return 0;
}

static int PlaySpellVisualKit(void*, NETMESSAGE, unsigned long, CDataStore* msg) {
    // TODO: implement
    return 0;
}

static int CCommand_Learn(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_Cooldown(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_CooldownPet(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_UseSkill(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_SetSkill(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_CancelAura(const char*, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_SpellString(const char*, const char* arguments) {
    // TODO: implement
    return 0;
}

static int __fastcall Script_SpellIsTargeting(lua_State *L) {
  Spell_C_IsTargeting() ? lua_pushnumber(L, 1.0) : lua_pushnil(L);
  return 1;
}

static int __fastcall Script_SpellCanTargetUnit(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: SpellCanTargetUnit(unit)");
  }
  unsigned __int64 guid = Script_GetGUIDFromName(lua_tostring(L, 1));
  CGObject_C      *object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
  if (Spell_C_IsTargeting() && object && (object->GetType() & TYPE_UNIT)) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int __fastcall Script_SpellTargetUnit(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: SpellTargetUnit(unit)");
  }
  unsigned __int64 guid = Script_GetGUIDFromName(lua_tostring(L, 1));
  CGObject_C      *object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
  if (object) {
    Spell_C_HandleSpriteClick(object);
  }
  return 0;
}

static int __fastcall Script_SpellStopTargeting(lua_State *L) {
  bool targeting = Spell_C_IsTargeting();
  Spell_C_StopTargeting();
  targeting ? lua_pushnumber(L, 1.0) : lua_pushnil(L);
  return 1;
}

void __fastcall SpellRegisterScriptFunctions() {
  for (unsigned int i = 0; i < 4; ++i) {
    FrameScript_RegisterFunction(s_SpellScriptFunctions[i].name, s_SpellScriptFunctions[i].method);
  }
}

void __fastcall SpellUnregisterScriptFunctions() {
  for (unsigned int i = 0; i < 4; ++i) {
    FrameScript_UnregisterFunction(s_SpellScriptFunctions[i].name);
  }
}

FrameScript_Method s_SpellScriptFunctions[4] = {
    {  "SpellIsTargeting",   Script_SpellIsTargeting},
    {"SpellCanTargetUnit", Script_SpellCanTargetUnit},
    {   "SpellTargetUnit",    Script_SpellTargetUnit},
    {"SpellStopTargeting", Script_SpellStopTargeting}
};

void __fastcall Spell_C_CancelCombatSpell() {
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
      msg.Put(static_cast<unsigned int>(CMSG_CANCEL_CAST));
      msg.Put(s_savedModalSpellID);
      msg.Finalize();
      ClientServices_Send(&msg);
    }
    s_savedModalSpellID = 0;
    s_savedModalItemID = 0;
  }
}

void __fastcall Spell_C_CancelAura(int spellID) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_CANCEL_AURA));
  msg.Put(spellID);
  msg.Finalize();
  ClientServices_Send(&msg);
}

unsigned int __fastcall Spell_C_GetPowerDisplayMod(POWER_TYPE type) {
    // TODO: implement
    return 0;
}

void __fastcall Spell_C_Initialize() {
    // TODO: implement
}

void __fastcall Spell_C_Destroy() {
    // TODO: implement
}

bool __fastcall IsSpellAura(const SpellRec *rec) {
  for (unsigned int effect = 0; effect < 3; ++effect) {
    if (rec->m_effect[effect] == 6 || rec->m_effect[effect] == 35) {
      return 1;
    }
  }
  return 0;
}
