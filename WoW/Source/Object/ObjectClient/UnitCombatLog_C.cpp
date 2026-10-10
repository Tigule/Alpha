#include <Base/Base.h>
#include <Gx/Gx.h>
#include <MapDefs.h>
#include <WorldClient/World.h>
#include "Net/NetClient/NetClient.h"
#include <Frame/CSimpleTop.h>
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "SoundInterface/SoundInterface.h"
#include "UIUtil/InputControl.h"
#include "Ui/WorldFrame.h"
#include "Ui/GameUI.h"

#include "Unit_C.h"
#include "Player_C.h"

#include <Base/CDataStore.h>

#include "DB/DBClient/AutoCode/SpellRec.h"
#include "DB/DBClient/AutoCode/SpellItemEnchantmentRec.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "DB/DBClient/AutoCode/FactionRec.h"
#include "DB/DBClient/AutoCode/ResistancesRec.h"
#include "DB/DBClient/DBClient.h"
#include "DB/WowLocale.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Ui/ChatFrame.h"
#include "Ui/GameUI.h"
#include "Ui/Tutorial.h"
#include "Console/ConsoleClient.h"
#include "Console/ConsoleCommand.h"
#include "Console/ConsoleVar.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <FrameScript/FrameScript.h>
#include <Os/OsTime.h>
#include <storm.h>

struct UNITHASHOBJ : public TSHashObject<UNITHASHOBJ, CHashKeyGUID> {
  UNITHASHOBJ() : count(0) {
  }
  UINT count;
};

struct COMBATLOGDESC {
  UINT                                   totalDamageDoneByEntity;
  UINT                                   totalDamageReducedByVictim;
  UINT                                   totalAttemptsByEntity;
  UINT                                   totalMisses;
  UINT                                   totalHits;
  UINT                                   totalVictimStatesByEntity[NUM_VICTIMSTATES];
  UINT                                   parryAttempts;
  UINT                                   dodgeAttempts;
  UINT                                   blockAttempts;
  UINT                                   totalTimeDelayed;
  UINT                                   criticalHits;
  UINT                                   spellCritsAttempted;
  UINT                                   spellCritsSucceeded;
  UINT                                   spellCritsSuffered;
  int                                    totalHealthHealed;
  int                                    totalReflectedDamageSuffered;
  int                                    totalDamageSuffered;
  int                                    totalHealingProvided;
  int                                    totalReflectedDamageProvided;
  int                                    totalDamageProvided;
  float                                  totalSpellDamageReducedByVictim;
  float                                  totalSpellDamageReduced;
  TSHashTable<UNITHASHOBJ, CHashKeyGUID> victims;
  TSHashTable<UNITHASHOBJ, CHashKeyGUID> attackers;
  char                                   m_name[48];

  COMBATLOGDESC(LPCSTR name);
  ~COMBATLOGDESC();
  void Clear();
  void LogAttack(const ATTACKROUNDINFO &info);
  void LogAttack(const SPELLLOG &info);
  void LogVictim(const ATTACKROUNDINFO &info);
  void LogVictim(const SPELLLOG &info);
  void LogUnitGUID(DWORDLONG guid, TSHashTable<UNITHASHOBJ, CHashKeyGUID> &theTable);
};

struct COMBATMESSAGEPRONOUNS {
  char attackerName[48];
  char victimName[48];
};

struct ENCHANTMENTLOGDESC {
  bool           valid;
  ENCHANTMENTLOG log;

  ENCHANTMENTLOGDESC() : valid(false) {
  }

  ENCHANTMENTLOGDESC(const ENCHANTMENTLOGDESC &other) : valid(other.valid), log(other.log) {
  }
};

enum COMBATMESSAGETYPE {
  COMBATMESSAGETYPE_NORMALHIT = 0,
  COMBATMESSAGETYPE_NORMALMISS = 1,
  COMBATMESSAGETYPE_NORMALBLOCK = 2,
  COMBATMESSAGETYPE_NORMALPARRY = 3,
  COMBATMESSAGETYPE_NORMALDODGE = 4,
  COMBATMESSAGETYPE_NORMALEVADE = 5,
  COMBATMESSAGETYPE_NORMALIMMUNE = 6,
  NUM_COMBATMESSAGETYPES = 7,
  COMBATMESSAGETYPE_UNKNOWN = -1
};

struct RESULTTYPEHANDLERDESC {
  void (*handler)(COMBATMESSAGEPRONOUNS &, const ATTACKROUNDINFO &, char *, UINT);
  UINT flags;
};

void          UnitCombatLogEnableFileLog(int enable);
void          UnitCombatDebugLogEnable(int enable);
void          UnitCombatLogShowXPGained(const DWORDLONG &victim, int xp);
void          UnitCombatLogEnchantment(const ENCHANTMENTLOG &log);
void          UnitCombatLogSpellMissed(UINT missReason, UINT spellID, DWORDLONG caster, DWORDLONG victim);
bool          IsSpellAura(const SpellRec *rec);
LPCSTR        GetSpellAuraEffectName(int effectID);
static void   ItemEnchantmentCacheCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted);
static int    Script_ToggleCombatLogFileWrite(lua_State *L);

static HSLOG                               s_logHandle;
static HSLOG                               s_generalLogHandle;
static TSGrowableArray<char>               s_charArray;
static UINT                                s_flags;
static UINT                                s_logStartTime;
static UINT                                s_lastLogTime;
static const CGPlayer_C                   *s_activePlayer;
static TSGrowableArray<ENCHANTMENTLOGDESC> s_logDesc;
static COMBATLOGDESC s_unitCombatData[AFFILIATION_NUMAFFILIATIONS] = {"You", "Your Pet", "Party Members", "Enemy", "Your Charmer"};

static const char *s_attackStrings[AFFILIATION_NUMAFFILIATIONS][AFFILIATION_NUMAFFILIATIONS] = {
    {0, 0, 0, "COMBATHITSELFOTHER", 0},
    {0, 0, 0, "COMBATHITOTHEROTHER", 0},
    {0, 0, 0, "COMBATHITOTHEROTHER", 0},
    {"COMBATHITOTHERSELF", "COMBATHITOTHEROTHER", "COMBATHITOTHEROTHER", "COMBATHITOTHEROTHER", 0},
    {0, 0, 0, 0, 0}
};

static const char *s_attackAbsorbStrings[AFFILIATION_NUMAFFILIATIONS][AFFILIATION_NUMAFFILIATIONS] = {
    {0, 0, 0, "COMBATHITABSORBSELFOTHER", 0},
    {0, 0, 0, "COMBATHITABSORBOTHEROTHER", 0},
    {0, 0, 0, "COMBATHITABSORBOTHEROTHER", 0},
    {"COMBATHITABSORBOTHERSELF", "COMBATHITABSORBOTHEROTHER", "COMBATHITABSORBOTHEROTHER", "COMBATHITABSORBOTHEROTHER", 0},
    {0, 0, 0, 0, 0}
};

static const char *s_attackCritStrings[AFFILIATION_NUMAFFILIATIONS][AFFILIATION_NUMAFFILIATIONS] = {
    {0, 0, 0, "COMBATHITCRITSELFOTHER", 0},
    {0, 0, 0, "COMBATHITCRITOTHEROTHER", 0},
    {0, 0, 0, "COMBATHITCRITOTHEROTHER", 0},
    {"COMBATHITCRITOTHERSELF", "COMBATHITCRITOTHEROTHER", "COMBATHITCRITOTHEROTHER", "COMBATHITCRITOTHEROTHER", 0},
    {0, 0, 0, 0, 0}
};

static const char *s_spellHitStrings[AFFILIATION_NUMAFFILIATIONS][AFFILIATION_NUMAFFILIATIONS] = {
    {0, 0, 0, "SPELLLOGSELFOTHER", 0},
    {0, 0, 0, "SPELLLOGOTHEROTHER", 0},
    {0, 0, 0, "SPELLLOGOTHEROTHER", 0},
    {"SPELLLOGOTHERSELF", "SPELLLOGOTHEROTHER", "SPELLLOGOTHEROTHER", "SPELLLOGOTHEROTHER", 0},
    {0, 0, 0, 0, 0}
};

static const char *s_spellHitAbsorbStrings[AFFILIATION_NUMAFFILIATIONS][AFFILIATION_NUMAFFILIATIONS] = {
    {0, 0, 0, "SPELLLOGABSORBSELFOTHER", 0},
    {0, 0, 0, "SPELLLOGABSORBOTHEROTHER", 0},
    {0, 0, 0, "SPELLLOGABSORBOTHEROTHER", 0},
    {"SPELLLOGABSORBOTHERSELF", "SPELLLOGABSORBOTHEROTHER", "SPELLLOGABSORBOTHEROTHER", "SPELLLOGABSORBOTHEROTHER", 0},
    {0, 0, 0, 0, 0}
};

static const char *s_spellHitCritStrings[AFFILIATION_NUMAFFILIATIONS][AFFILIATION_NUMAFFILIATIONS] = {
    {0, 0, 0, "SPELLLOGCRITSELFOTHER", 0},
    {0, 0, 0, "SPELLLOGCRITOTHEROTHER", 0},
    {0, 0, 0, "SPELLLOGCRITOTHEROTHER", 0},
    {"SPELLLOGCRITOTHERSELF", "SPELLLOGCRITOTHEROTHER", "SPELLLOGCRITOTHEROTHER", "SPELLLOGCRITOTHEROTHER", 0},
    {0, 0, 0, 0, 0}
};

static const char *s_healStrings[AFFILIATION_NUMAFFILIATIONS][AFFILIATION_NUMAFFILIATIONS] = {
    {"HEALEDSELFSELF", "HEALEDSELFOTHER", "HEALEDSELFOTHER", "HEALEDSELFOTHER", 0},
    {"HEALEDOTHERSELF", "HEALEDOTHEROTHER", "HEALEDOTHEROTHER", "HEALEDOTHEROTHER", 0},
    {"HEALEDOTHERSELF", "HEALEDOTHEROTHER", "HEALEDOTHEROTHER", "HEALEDOTHEROTHER", 0},
    {"HEALEDOTHERSELF", "HEALEDOTHEROTHER", "HEALEDOTHEROTHER", "HEALEDOTHEROTHER", 0},
    {0, 0, 0, 0, 0}
};

static const char *s_dispelStrings[AFFILIATION_NUMAFFILIATIONS][AFFILIATION_NUMAFFILIATIONS] = {
    {"DISPELLEDSELFSELF", "DISPELLEDSELFOTHER", "DISPELLEDSELFOTHER", "DISPELLEDSELFOTHER", 0},
    {"DISPELLEDOTHERSELF", "DISPELLEDOTHEROTHER", "DISPELLEDOTHEROTHER", "DISPELLEDOTHEROTHER", 0},
    {"DISPELLEDOTHERSELF", "DISPELLEDOTHEROTHER", "DISPELLEDOTHEROTHER", "DISPELLEDOTHEROTHER", 0},
    {"DISPELLEDOTHERSELF", "DISPELLEDOTHEROTHER", "DISPELLEDOTHEROTHER", "DISPELLEDOTHEROTHER", 0},
    {0, 0, 0, 0, 0}
};

static const char *s_immuneStrings[AFFILIATION_NUMAFFILIATIONS][AFFILIATION_NUMAFFILIATIONS] = {
    {"IMMUNESELFSELF", "IMMUNESELFOTHER", "IMMUNESELFOTHER", "IMMUNESELFOTHER", 0},
    {"IMMUNEOTHERSELF", "IMMUNEOTHEROTHER", "IMMUNEOTHEROTHER", "IMMUNEOTHEROTHER", 0},
    {"IMMUNEOTHERSELF", "IMMUNEOTHEROTHER", "IMMUNEOTHEROTHER", "IMMUNEOTHEROTHER", 0},
    {"IMMUNEOTHERSELF", "IMMUNEOTHEROTHER", "IMMUNEOTHEROTHER", "IMMUNEOTHEROTHER", 0},
    {0, 0, 0, 0, 0}
};

static const UINT s_requiredHitStrings[AFFILIATION_NUMAFFILIATIONS][AFFILIATION_NUMAFFILIATIONS] = {
    {0, 2, 3, 2, 0},
    {1, 3, 3, 3, 0},
    {1, 3, 3, 3, 0},
    {1, 3, 3, 3, 0},
    {0, 0, 0, 0, 0}
};

static const char *s_missStrings[AFFILIATION_NUMAFFILIATIONS][AFFILIATION_NUMAFFILIATIONS] = {
    {0, 0, 0, "MISSEDSELFOTHER", 0},
    {0, 0, 0, "MISSEDPETOTHER", 0},
    {0, 0, 0, "MISSEDOTHEROTHER", 0},
    {"MISSEDOTHERSELF", "MISSEDOTHERPET", "MISSEDOTHERPARTY", "MISSEDOTHEROTHER", 0},
    {0, 0, 0, 0, 0}
};

static const UINT s_requiredMissStrings[AFFILIATION_NUMAFFILIATIONS][AFFILIATION_NUMAFFILIATIONS] = {
    {0, 0, 0, 2, 0},
    {0, 0, 0, 3, 0},
    {0, 0, 0, 3, 0},
    {1, 3, 3, 3, 0},
    {0, 0, 0, 0, 0}
};

static const UINT s_requiredVictimStateStrings[AFFILIATION_NUMAFFILIATIONS][AFFILIATION_NUMAFFILIATIONS] = {
    {0, 0, 0, 2, 0},
    {0, 0, 0, 3, 0},
    {0, 0, 0, 3, 0},
    {1, 3, 3, 3, 0},
    {0, 0, 0, 0, 0}
};

static const char *s_victimStateStringParry[AFFILIATION_NUMAFFILIATIONS][AFFILIATION_NUMAFFILIATIONS] = {
    {0, 0, 0, "VSPARRYSELFOTHER", 0},
    {0, 0, 0, "VSPARRYOTHEROTHER", 0},
    {0, 0, 0, "VSPARRYOTHEROTHER", 0},
    {"VSPARRYOTHERSELF", "VSPARRYOTHEROTHER", "VSPARRYOTHEROTHER", "VSPARRYOTHEROTHER", 0},
    {0, 0, 0, 0, 0}
};

static const char *s_victimStateStringBlock[AFFILIATION_NUMAFFILIATIONS][AFFILIATION_NUMAFFILIATIONS] = {
    {0, 0, 0, "VSBLOCKSELFOTHER", 0},
    {0, 0, 0, "VSBLOCKOTHEROTHER", 0},
    {0, 0, 0, "VSBLOCKOTHEROTHER", 0},
    {"VSBLOCKOTHERSELF", "VSBLOCKOTHEROTHER", "VSBLOCKOTHEROTHER", "VSBLOCKOTHEROTHER", 0},
    {0, 0, 0, 0, 0}
};

static const char *s_victimStateStringImmune[AFFILIATION_NUMAFFILIATIONS][AFFILIATION_NUMAFFILIATIONS] = {
    {0, 0, 0, "VSIMMUNESELFOTHER", 0},
    {0, 0, 0, "VSIMMUNEOTHEROTHER", 0},
    {0, 0, 0, "VSIMMUNEOTHEROTHER", 0},
    {"VSIMMUNEOTHERSELF", "VSIMMUNEOTHEROTHER", "VSIMMUNEOTHEROTHER", "VSIMMUNEOTHEROTHER", 0},
    {0, 0, 0, 0, 0}
};

static const char *s_victimStateStringDeflect[AFFILIATION_NUMAFFILIATIONS][AFFILIATION_NUMAFFILIATIONS] = {
    {0, 0, 0, "VSDEFLECTSELFOTHER", 0},
    {0, 0, 0, "VSDEFLECTOTHEROTHER", 0},
    {0, 0, 0, "VSDEFLECTOTHEROTHER", 0},
    {"VSDEFLECTOTHERSELF", "VSDEFLECTOTHEROTHER", "VSDEFLECTOTHEROTHER", "VSDEFLECTOTHEROTHER", 0},
    {0, 0, 0, 0, 0}
};

static const char *s_victimStateStringDodge[AFFILIATION_NUMAFFILIATIONS][AFFILIATION_NUMAFFILIATIONS] = {
    {0, 0, 0, "VSDODGESELFOTHER", 0},
    {0, 0, 0, "VSDODGEOTHEROTHER", 0},
    {0, 0, 0, "VSDODGEOTHEROTHER", 0},
    {"VSDODGEOTHERSELF", "VSDODGEOTHEROTHER", "VSDODGEOTHEROTHER", "VSDODGEOTHEROTHER", 0},
    {0, 0, 0, 0, 0}
};

static const char *s_victimStateStringEvade[AFFILIATION_NUMAFFILIATIONS][AFFILIATION_NUMAFFILIATIONS] = {
    {0, 0, 0, "VSEVADESELFOTHER", 0},
    {0, 0, 0, "VSEVADEOTHEROTHER", 0},
    {0, 0, 0, "VSEVADEOTHEROTHER", 0},
    {"VSEVADEOTHERSELF", "VSEVADEOTHEROTHER", "VSEVADEOTHEROTHER", "VSEVADEOTHEROTHER", 0},
    {0, 0, 0, 0, 0}
};

static int s_showVictimStates[NUM_VICTIMSTATES] = {0, 0, 1, 1, 0, 1, 1, 1, 1};

static const char *s_spellMissStrings[AFFILIATION_NUMAFFILIATIONS][AFFILIATION_NUMAFFILIATIONS] = {
    {0, 0, 0, "SPELLMISSSELFOTHER", 0},
    {0, 0, 0, "SPELLMISSPETOTHER", 0},
    {0, 0, 0, "SPELLMISSOTHEROTHER", 0},
    {"SPELLMISSOTHERSELF", "SPELLMISSOTHERPET", "SPELLMISSOTHEROTHER", "SPELLMISSOTHEROTHER", 0},
    {0, 0, 0, 0, 0}
};

static const UINT s_requiredSpellMissStrings[AFFILIATION_NUMAFFILIATIONS][AFFILIATION_NUMAFFILIATIONS] = {
    {0, 0, 0, 2, 0},
    {0, 0, 0, 3, 0},
    {0, 0, 0, 3, 0},
    {1, 1, 3, 3, 0},
    {0, 0, 0, 0, 0}
};

static const char *s_spellResistStrings[AFFILIATION_NUMAFFILIATIONS][AFFILIATION_NUMAFFILIATIONS] = {
    {0, 0, 0, "SPELLRESISTSELFOTHER", 0},
    {0, 0, 0, "SPELLRESISTPETOTHER", 0},
    {0, 0, 0, "SPELLRESISTOTHEROTHER", 0},
    {"SPELLRESISTOTHERSELF", "SPELLRESISTOTHERPET", "SPELLRESISTOTHEROTHER", "SPELLRESISTOTHEROTHER", 0},
    {0, 0, 0, 0, 0}
};

static const char *s_spellImmuneStrings[AFFILIATION_NUMAFFILIATIONS][AFFILIATION_NUMAFFILIATIONS] = {
    {0, 0, 0, "SPELLIMMUNESELFOTHER", 0},
    {0, 0, 0, "SPELLIMMUNEPETOTHER", 0},
    {0, 0, 0, "SPELLIMMUNEOTHEROTHER", 0},
    {"SPELLIMMUNEOTHERSELF", "SPELLIMMUNEOTHERPET", "SPELLIMMUNEOTHEROTHER", "SPELLIMMUNEOTHEROTHER", 0},
    {0, 0, 0, 0, 0}
};

static const char *s_spellEvadedStrings[AFFILIATION_NUMAFFILIATIONS][AFFILIATION_NUMAFFILIATIONS] = {
    {0, 0, 0, "SPELLEVADEDSELFOTHER", 0},
    {0, 0, 0, "SPELLEVADEDPETOTHER", 0},
    {0, 0, 0, "SPELLEVADEDOTHEROTHER", 0},
    {"SPELLEVADEDOTHERSELF", "SPELLEVADEDOTHERPET", "SPELLEVADEDOTHEROTHER", "SPELLEVADEDOTHEROTHER", 0},
    {0, 0, 0, 0, 0}
};

static const char *s_spellDodgedStrings[AFFILIATION_NUMAFFILIATIONS][AFFILIATION_NUMAFFILIATIONS] = {
    {0, 0, 0, "SPELLDODGEDSELFOTHER", 0},
    {0, 0, 0, "SPELLDODGEDPETOTHER", 0},
    {0, 0, 0, "SPELLDODGEDOTHEROTHER", 0},
    {"SPELLDODGEDOTHERSELF", "SPELLDODGEDOTHERPET", "SPELLDODGEDOTHEROTHER", "SPELLDODGEDOTHEROTHER", 0},
    {0, 0, 0, 0, 0}
};

static const char *s_spellParriedStrings[AFFILIATION_NUMAFFILIATIONS][AFFILIATION_NUMAFFILIATIONS] = {
    {0, 0, 0, "SPELLPARRIEDSELFOTHER", 0},
    {0, 0, 0, "SPELLPARRIEDPETOTHER", 0},
    {0, 0, 0, "SPELLPARRIEDOTHEROTHER", 0},
    {"SPELLPARRIEDOTHEROTHER", "SPELLPARRIEDOTHEROTHER", "SPELLPARRIEDOTHEROTHER", "SPELLPARRIEDOTHEROTHER", 0},
    {0, 0, 0, 0, 0}
};

static const char *s_spellBlockedStrings[AFFILIATION_NUMAFFILIATIONS][AFFILIATION_NUMAFFILIATIONS] = {
    {0, 0, 0, "SPELLBLOCKEDSELFOTHER", 0},
    {0, 0, 0, "SPELLBLOCKEDOTHEROTHER", 0},
    {0, 0, 0, "SPELLBLOCKEDOTHEROTHER", 0},
    {"SPELLBLOCKEDOTHERSELF", "SPELLBLOCKEDOTHEROTHER", "SPELLBLOCKEDOTHEROTHER", "SPELLBLOCKEDOTHEROTHER", 0},
    {0, 0, 0, 0, 0}
};

static const char *s_spellDeflectedStrings[AFFILIATION_NUMAFFILIATIONS][AFFILIATION_NUMAFFILIATIONS] = {
    {0, 0, 0, "SPELLDEFLECTEDSELFOTHER", 0},
    {0, 0, 0, "SPELLDEFLECTEDOTHEROTHER", 0},
    {0, 0, 0, "SPELLDEFLECTEDOTHEROTHER", 0},
    {"SPELLDEFLECTEDOTHERSELF", "SPELLDEFLECTEDOTHEROTHER", "SPELLDEFLECTEDOTHEROTHER", "SPELLDEFLECTEDOTHEROTHER", 0},
    {0, 0, 0, 0, 0}
};

static SLASH_COMMAND_ID s_affiliationLogType[AFFILIATION_NUMAFFILIATIONS] = {
    SLASH_CMD_COMBAT_LOG_SELF, SLASH_CMD_COMBAT_LOG_PARTY, SLASH_CMD_COMBAT_LOG_PARTY, SLASH_CMD_COMBAT_LOG_ENEMY, SLASH_CMD_COMBAT_LOG_PARTY
};

struct {
  LPCSTR cvarname;
  LPCSTR defaultvalue;
} s_affMappingCVarsNames[AFFILIATION_NUMAFFILIATIONS] = {{0, 0}, {0, 0}, {"CombatLogPartyRange", "0"}, {"CombatLogRange", "40"}, {0, 0}};

static const struct {
  int   hitRollValid;
  char *actionString;
  char *optionalString;
} s_missTypesInfo[] = {
    {1, "unavoidable", " (hit)"},
    {1, "missed", " (physical)"},
    {1, "missed", " (resist)"},
    {0, "immune", " (immune)"},
    {1, "evaded", " (evade)"},
    {1, "dodged", ""},
    {1, "parried", ""},
    {1, "blocked", ""},
    {0, "immune", " (temp)"},
    {0, "deflected", ""}
};

static bool IsSpellTeach(const SpellRec *rec) {
  for (UINT effect = 0; effect < 3; ++effect) {
    if (rec->m_effect[effect] == 36) {
      return 1;
    }
  }
  return 0;
}

static bool IsSpellAbility(const SpellRec *rec) {
  return ((UINT)rec->m_attributes >> 4) & 1;
}

static bool IsSpellHarmful(const SpellRec *rec) {
  for (UINT effect = 0; effect < 3; ++effect) {
    if (rec->m_effect[effect] == 2 || rec->m_effect[effect] == 58 || rec->m_effect[effect] == 17 || rec->m_effect[effect] == 31 ||
        rec->m_effect[effect] == 62)
    {
      return 1;
    }
  }
  return 0;
}

static bool IsSpellOpenLock(const SpellRec *rec) {
  for (UINT effect = 0; effect < 3; ++effect) {
    if (rec->m_effect[effect] == 33 || rec->m_effect[effect] == 59) {
      return 1;
    }
  }
  return 0;
}

static bool IsSpellQuiet(const SpellRec *rec) {
  return ((UINT)rec->m_attributes >> 7) & 1;
}

static float GetLogDistance(UNITAFFILIATION aff, bool suppressUnaffiliated) {
  if (aff < AFFILIATION_NUMAFFILIATIONS && (!suppressUnaffiliated || aff != AFFILIATION_OTHER)) {
    if (s_affMappingCVarsNames[aff].cvarname && *s_affMappingCVarsNames[aff].cvarname) {
      CVar *cvar = CVar::Lookup(s_affMappingCVarsNames[aff].cvarname);
      return cvar ? cvar->GetFloat() : 0.0f;
    }
    return 100000.0f;
  }
  return 0.0f;
}

static bool ShouldLogAttacker(
    DWORDLONG        attacker,
    UNITAFFILIATION &aAff,
    CGObject_C     *&unitPtr,
    bool             suppressIfUnaffiliated,
    bool             useDeathRange,
    int              allowedAffiliationFlags
) {
  if (!s_activePlayer) {
    return 0;
  }

  aAff = s_activePlayer->GetGUIDAffiliation(attacker);
  if (!((1 << aAff) & allowedAffiliationFlags)) {
    return 0;
  }

  unitPtr = ClntObjMgrObjectPtr(attacker, __FILE__, __LINE__);
  if (!unitPtr) {
    return 0;
  }

  CVar *cvar = CVar::Lookup("CombatDeathLogRange");
  float dist;
  if (useDeathRange && cvar) {
    dist = cvar->GetFloat();
  } else {
    dist = GetLogDistance(aAff, suppressIfUnaffiliated);
  }
  return (s_activePlayer->GetPosition() - unitPtr->GetPosition()).SquaredMag() < dist * dist;
}

static BOOL ShouldLog(
    DWORDLONG        object,
    UNITAFFILIATION &aAff,
    CGObject_C     *&objectPtr,
    CGUnit_C       *&attackerPtr,
    DWORDLONG        subject,
    UNITAFFILIATION &vAff,
    CGObject_C     *&subjectPtr,
    CGUnit_C       *&victimPtr,
    bool             suppressIfAllUnaffiliated
) {
  if (!s_activePlayer) {
    return 0;
  }

  aAff = s_activePlayer->GetGUIDAffiliation(object);
  vAff = s_activePlayer->GetGUIDAffiliation(subject);
  objectPtr = ClntObjMgrObjectPtr(object, __FILE__, __LINE__);
  subjectPtr = ClntObjMgrObjectPtr(subject, __FILE__, __LINE__);
  if (!objectPtr || !subjectPtr) {
    return 0;
  }

  attackerPtr = 0;
  victimPtr = 0;
  if (objectPtr && objectPtr->IsA(TYPE_UNIT)) {
    attackerPtr = (CGUnit_C *)objectPtr;
  }
  if (subjectPtr && subjectPtr->IsA(TYPE_UNIT)) {
    victimPtr = (CGUnit_C *)subjectPtr;
  }

  float objRangeSquared = GetLogDistance(aAff, suppressIfAllUnaffiliated);
  float subRangeSquared = GetLogDistance(vAff, suppressIfAllUnaffiliated);
  objRangeSquared *= objRangeSquared;
  subRangeSquared *= subRangeSquared;
  NTempest::C3Vector objectDiff = s_activePlayer->GetPosition() - objectPtr->GetPosition();
  NTempest::C3Vector diff = s_activePlayer->GetPosition() - subjectPtr->GetPosition();
  float              objectSquaredMag = objectDiff.SquaredMag();
  float              subjectSquaredMag = diff.SquaredMag();

  if (aAff == AFFILIATION_PARTYMEMBER && objectSquaredMag > objRangeSquared) {
    return 0;
  }
  if (vAff == AFFILIATION_PARTYMEMBER && subjectSquaredMag > subRangeSquared) {
    return 0;
  }
  return objectSquaredMag < objRangeSquared || subjectSquaredMag < subRangeSquared;
}

static void __cdecl GeneralLogPrintf(SLASH_COMMAND_ID type, LPCSTR format, ...) {
  char    buffer[512];
  va_list arguments;
  va_start(arguments, format);
  SStrVPrintf(buffer, sizeof(buffer), format, arguments);
  va_end(arguments);

  CGChat::AddChatMessage(buffer, type, 0, 0, 0, 0, 0);
  if (s_generalLogHandle) {
    SLogWrite(s_generalLogHandle, "%s", buffer);
  }
}

static void ReportError(LPCSTR string) {
  if (string && *string) {
    GeneralLogPrintf(SLASH_CMD_COMBAT_LOG_ERROR, "Warning, string %s not found in stringfile.", string);
  }
}

static void HandleTerseVictimLogging(DWORDLONG attacker, DWORDLONG victim, UINT spellID) {
  if (!s_activePlayer) {
    return;
  }

  CGObject_C     *attackerObjPtr;
  CGObject_C     *victimObjPtr;
  CGUnit_C       *attackerPtr;
  CGUnit_C       *victimPtr;
  UNITAFFILIATION aAff;
  UNITAFFILIATION vAff;
  if (!ShouldLog(attacker, aAff, attackerObjPtr, attackerPtr, victim, vAff, victimObjPtr, victimPtr, 0)) {
    return;
  }
  if (!victimPtr || !attackerPtr) {
    return;
  }

  const SpellRec *rec = g_spellDB.GetRecord(spellID);
  if (!rec || IsSpellQuiet(rec)) {
    return;
  }

  LPCSTR spellName = rec->m_name_lang[CURRENT_LANGUAGE];
  LPCSTR victimName = victimPtr->GetUnitName();
  LPCSTR attackerName = attackerPtr->GetUnitName();
  LPCSTR tag = s_dispelStrings[aAff][vAff];
  if (!tag) {
    return;
  }

  LPCSTR format = FrameScript_GetText(tag, -1, GENDER_NOT_APPLICABLE);
  if (format && *format) {
    if (aAff == AFFILIATION_YOURSELF) {
      if (vAff == AFFILIATION_YOURSELF) {
        GeneralLogPrintf(s_affiliationLogType[aAff], format, spellName);
      } else {
        GeneralLogPrintf(s_affiliationLogType[aAff], format, spellName, victimName);
      }
    } else {
      if (vAff == AFFILIATION_YOURSELF) {
        GeneralLogPrintf(s_affiliationLogType[aAff], format, attackerName, spellName);
      } else {
        GeneralLogPrintf(s_affiliationLogType[aAff], format, attackerName, spellName, victimName);
      }
    }
  } else {
    ReportError(tag);
  }
}

static void HandleGeneralHealLogging(const DamageData &dmg, UINT spellID, DWORDLONG attacker, DWORDLONG victim) {
  if (!s_activePlayer) {
    return;
  }

  CGObject_C     *attackerObjPtr;
  CGObject_C     *victimObjPtr;
  CGUnit_C       *attackerPtr;
  CGUnit_C       *victimPtr;
  UNITAFFILIATION aAff;
  UNITAFFILIATION vAff;
  if (!ShouldLog(attacker, aAff, attackerObjPtr, attackerPtr, victim, vAff, victimObjPtr, victimPtr, 0)) {
    return;
  }
  if (!victimPtr || !attackerPtr) {
    return;
  }

  const SpellRec *rec = g_spellDB.GetRecord(spellID);
  if (!rec || IsSpellQuiet(rec)) {
    return;
  }

  LPCSTR spellName = rec->m_name_lang[CURRENT_LANGUAGE];
  LPCSTR victimName = victimPtr->GetUnitName();
  LPCSTR attackerName = attackerPtr->GetUnitName();
  LPCSTR tag = s_healStrings[aAff][vAff];
  if (!tag) {
    return;
  }

  LPCSTR format = FrameScript_GetText(tag, -1, GENDER_NOT_APPLICABLE);
  if (format && *format) {
    if (aAff == AFFILIATION_YOURSELF) {
      if (vAff == AFFILIATION_YOURSELF) {
        GeneralLogPrintf(s_affiliationLogType[aAff], format, spellName, dmg.totalDamage);
      } else {
        GeneralLogPrintf(s_affiliationLogType[aAff], format, spellName, victimName, dmg.totalDamage);
      }
    } else {
      if (vAff == AFFILIATION_YOURSELF) {
        GeneralLogPrintf(s_affiliationLogType[aAff], format, attackerName, spellName, dmg.totalDamage);
      } else {
        GeneralLogPrintf(s_affiliationLogType[aAff], format, attackerName, spellName, victimName, dmg.totalDamage);
      }
    }
  } else {
    ReportError(tag);
  }
}

static void HandleSpellLogTerse(CGUnit_C *attackerPtr, UNITAFFILIATION aAff, LPCSTR spellNameString) {
  LPCSTR tag;
  switch (aAff) {
    case AFFILIATION_YOURSELF:
      tag = "SPELLTERSE_SELF";
      break;
    case AFFILIATION_YOURPET:
    case AFFILIATION_PARTYMEMBER:
    case AFFILIATION_OTHER:
    case AFFILIATION_YOURCONTROLLER:
      tag = "SPELLTERSE_OTHER";
      break;
    default:
      return;
  }

  LPCSTR format = FrameScript_GetText(tag, -1, GENDER_NOT_APPLICABLE);
  if (format && *format) {
    if (aAff == AFFILIATION_YOURSELF) {
      GeneralLogPrintf(s_affiliationLogType[aAff], format, spellNameString);
    } else {
      GeneralLogPrintf(s_affiliationLogType[aAff], format, attackerPtr->GetUnitName(), spellNameString);
    }
  } else {
    ReportError(tag);
  }
}

static void HandleGeneralCombatOrSpellHitLogging(
    int               combat,
    const DamageData &dmg,
    UINT              spellID,
    CGUnit_C         *attackerPtr,
    CGUnit_C         *victimPtr,
    int               critted,
    UINT              specialSpellID,
    UINT              specialSpellDamage,
    UNITAFFILIATION   aAff,
    UNITAFFILIATION   vAff
) {
  if (combat && specialSpellID && specialSpellDamage) {
    return;
  }

  const SpellRec *rec = g_spellDB.GetRecord(spellID);
  if (!combat && (!rec || IsSpellQuiet(rec))) {
    return;
  }

  LPCSTR spellName;
  if (rec && !combat) {
    spellName = rec->m_name_lang[CURRENT_LANGUAGE];
  } else {
    spellName = "UKNOWNSPELL";
  }
  LPCSTR victimName = victimPtr->GetUnitName();
  LPCSTR attackerName = attackerPtr->GetUnitName();
  int    absorbed = 0;
  int    damage = 0;
  for (UINT i = 0; i < 5; ++i) {
    if (dmg.damageType[i] < 0) {
      break;
    }
    damage += dmg.damage[i];
    absorbed += dmg.absorbed[i];
  }

  LPCSTR tag;
  if (combat) {
    if (!damage) {
      return;
    }
    if (absorbed) {
      tag = s_attackAbsorbStrings[aAff][vAff];
    } else if (critted) {
      tag = s_attackCritStrings[aAff][vAff];
    } else {
      tag = s_attackStrings[aAff][vAff];
    }
  } else {
    if (absorbed) {
      tag = s_spellHitAbsorbStrings[aAff][vAff];
    } else if (critted) {
      tag = s_spellHitCritStrings[aAff][vAff];
    } else {
      tag = s_spellHitStrings[aAff][vAff];
    }
  }
  if (!tag || !*tag) {
    return;
  }

  LPCSTR format = FrameScript_GetText(tag, -1, GENDER_NOT_APPLICABLE);
  UINT   required = s_requiredHitStrings[aAff][vAff];
  if (format && *format) {
    if (combat) {
      if (required == 0) {
        GeneralLogPrintf(s_affiliationLogType[aAff], format, damage, absorbed);
      } else if (required == 1) {
        GeneralLogPrintf(s_affiliationLogType[aAff], format, attackerName, damage, absorbed);
      } else if (required == 2) {
        GeneralLogPrintf(s_affiliationLogType[aAff], format, victimName, damage, absorbed);
      } else {
        GeneralLogPrintf(s_affiliationLogType[aAff], format, attackerName, victimName, damage, absorbed);
      }
    } else {
      if (required == 0) {
        GeneralLogPrintf(s_affiliationLogType[aAff], format, spellName, damage, absorbed);
      } else if (required == 1) {
        GeneralLogPrintf(s_affiliationLogType[aAff], format, attackerName, spellName, damage, absorbed);
      } else if (required == 2) {
        GeneralLogPrintf(s_affiliationLogType[aAff], format, spellName, victimName, damage, absorbed);
      } else {
        GeneralLogPrintf(s_affiliationLogType[aAff], format, attackerName, spellName, victimName, damage);
      }
    }

    if (combat && specialSpellID && specialSpellDamage) {
      if (aAff == AFFILIATION_YOURSELF) {
        tag = "COMBATSPECIALSELF";
      } else {
        tag = "COMBATSPECIALOTHER";
      }
      format = FrameScript_GetText(tag, -1, GENDER_NOT_APPLICABLE);
      if (format && *format) {
        const SpellRec *specSpellRec = g_spellDB.GetRecord(specialSpellID);
        if (specSpellRec && !IsSpellQuiet(specSpellRec)) {
          spellName = specSpellRec->m_name_lang[CURRENT_LANGUAGE];
          if (aAff == AFFILIATION_YOURSELF) {
            GeneralLogPrintf(s_affiliationLogType[aAff], format, spellName, specialSpellDamage);
          } else {
            GeneralLogPrintf(s_affiliationLogType[aAff], format, attackerName, spellName, specialSpellDamage);
          }
        }
      } else {
        ReportError(tag);
      }
    }
  } else {
    ReportError(tag);
  }
}

static void HandleGeneralCombatLoggingMissed(
    const ATTACKROUNDINFO &info,
    CGUnit_C              *attackerPtr,
    CGUnit_C              *victimPtr,
    UNITAFFILIATION        aAff,
    UNITAFFILIATION        vAff
) {
  if ((info.flags & 0x200) && (info.flags & 0x8000)) {
    return;
  }
  if (info.flags & 0x1000) {
    return;
  }

  LPCSTR victimName = victimPtr->GetUnitName();
  LPCSTR attackerName = attackerPtr->GetUnitName();
  LPCSTR tag = s_missStrings[aAff][vAff];
  if (!tag || !*tag) {
    return;
  }

  LPCSTR format = FrameScript_GetText(tag, -1, GENDER_NOT_APPLICABLE);
  if (format && *format) {
    UINT required = s_requiredMissStrings[aAff][vAff];
    if (required == 0) {
      GeneralLogPrintf(s_affiliationLogType[aAff], format);
    } else if (required == 1) {
      GeneralLogPrintf(s_affiliationLogType[aAff], format, attackerName);
    } else if (required == 2) {
      GeneralLogPrintf(s_affiliationLogType[aAff], format, victimName);
    } else {
      GeneralLogPrintf(s_affiliationLogType[aAff], format, attackerName, victimName);
    }
  } else {
    ReportError(tag);
  }
}

static void
HandleGeneralCombatEvadeLogging(const ATTACKROUNDINFO info, CGUnit_C *attackerPtr, CGUnit_C *victimPtr, UNITAFFILIATION aAff, UNITAFFILIATION vAff) {
  if (!s_showVictimStates[info.newVictimState]) {
    return;
  }
  if (info.flags & 0x1000) {
    return;
  }
  if (!victimPtr || !victimPtr->IsA(TYPE_UNIT) || !attackerPtr || !attackerPtr->IsA(TYPE_UNIT)) {
    return;
  }

  LPCSTR victimName = victimPtr->GetUnitName();
  LPCSTR attackerName = attackerPtr->GetUnitName();
  LPCSTR tag = 0;
  switch (info.newVictimState) {
    case VS_EVADE:
      tag = s_victimStateStringEvade[aAff][vAff];
      break;
    case VS_DODGE:
      tag = s_victimStateStringDodge[aAff][vAff];
      break;
    case VS_PARRY:
      tag = s_victimStateStringParry[aAff][vAff];
      break;
    case VS_DEFLECT:
      tag = s_victimStateStringDeflect[aAff][vAff];
      break;
    case VS_BLOCK:
      tag = s_victimStateStringBlock[aAff][vAff];
      break;
    case VS_IMMUNE:
      tag = s_victimStateStringImmune[aAff][vAff];
      break;
    default:
      FATALASSERT(!"Error should never reach here");
      break;
  }
  if (!tag || !*tag) {
    return;
  }

  LPCSTR format = FrameScript_GetText(tag, -1, GENDER_NOT_APPLICABLE);
  if (format && *format) {
    UINT required = s_requiredVictimStateStrings[aAff][vAff];
    if (required == 1) {
      GeneralLogPrintf(s_affiliationLogType[aAff], format, attackerName);
    } else if (required == 2) {
      GeneralLogPrintf(s_affiliationLogType[aAff], format, victimName);
    } else {
      GeneralLogPrintf(s_affiliationLogType[aAff], format, attackerName, victimName);
    }
  } else {
    ReportError(tag);
  }
}

static void HandleGeneralCombatLogging(const ATTACKROUNDINFO &info) {
  if (!s_activePlayer) {
    return;
  }
  if (info.flags & 0x2000) {
    return;
  }

  UNITAFFILIATION aAff = s_activePlayer->GetGUIDAffiliation(info.attacker);
  UNITAFFILIATION vAff = s_activePlayer->GetGUIDAffiliation(info.victim);
  CGObject_C     *attackerObjPtr;
  CGObject_C     *victimObjPtr;
  CGUnit_C       *attackerPtr;
  CGUnit_C       *victimPtr;
  if (!ShouldLog(info.attacker, aAff, attackerObjPtr, attackerPtr, info.victim, vAff, victimObjPtr, victimPtr, 0)) {
    return;
  }
  if (!victimPtr || !attackerPtr) {
    return;
  }

  if (info.flags & 1) {
    HandleGeneralCombatLoggingMissed(info, attackerPtr, victimPtr, aAff, vAff);
  } else if (info.newVictimState == VS_WOUND) {
    HandleGeneralCombatOrSpellHitLogging(
        1, info.dmg, 0, attackerPtr, victimPtr, info.flags & 8, info.spellAddedDamage, info.spellDamageAdded, aAff, vAff
    );
  } else {
    HandleGeneralCombatEvadeLogging(info, attackerPtr, victimPtr, aAff, vAff);
  }
}

static void FormatSpellMissString(char *string, UINT size, const SPELLMISSLOG &log) {
  if (log.flags & 0x10) {
    const SpellRec *rec = g_spellDB.GetRecord(log.spellID);
    SStrPrintf(
        string, size, "(%s) Proc failed with roll %g%%/%g%% (required/needed)", rec ? rec->m_name_lang[CURRENT_LANGUAGE] : "Unknown Spell",
        log.hitRollNeeded, log.hitRoll
    );
    return;
  }
  if (!log.reason) {
    return;
  }

  char hitString[128] = "";
  if (log.reason >= sizeof(s_missTypesInfo) / sizeof(s_missTypesInfo[0])) {
    return;
  }
  if (s_missTypesInfo[log.reason].hitRollValid) {
    SStrPrintf(hitString, sizeof(hitString), " (%g%%/%g%%)", log.hitRollNeeded, log.hitRoll);
  }

  const SpellRec *rec = g_spellDB.GetRecord(log.spellID);
  CGObject_C     *attackerPtr = ClntObjMgrObjectPtr(log.attacker, __FILE__, __LINE__);
  CGObject_C     *victimPtr = ClntObjMgrObjectPtr(log.victim, __FILE__, __LINE__);
  if (victimPtr && victimPtr->IsA(TYPE_UNIT) && attackerPtr && attackerPtr->IsA(TYPE_UNIT)) {
    SStrPrintf(
        string, size, "(%s) The attack of %s on %s was %s%s%s", rec ? rec->m_name_lang[CURRENT_LANGUAGE] : "Unknown Spell",
        ((CGUnit_C *)attackerPtr)->GetUnitName(), ((CGUnit_C *)victimPtr)->GetUnitName(),
        s_missTypesInfo[log.reason].actionString, hitString, s_missTypesInfo[log.reason].optionalString
    );
  }
}

static void FormatSpellString(char *string, UINT size, const SPELLLOG &log) {
  CGUnit_C   *attackerPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(log.attacker, __FILE__, __LINE__));
  CGObject_C *victimPtr = ClntObjMgrObjectPtr(log.victim, __FILE__, __LINE__);
  LPCSTR      actionString;
  if (log.flags & 2) {
    actionString = "healed";
  } else if (log.flags & 0x80) {
    actionString = "energized";
  } else if (log.flags & 8) {
    actionString = "reflected damage to";
  } else if (log.flags & 0x400) {
    actionString = "cast spell on";
  } else {
    actionString = "hit";
  }

  char critString[64] = "";
  if (log.flags & 0x40) {
    SStrPrintf(critString, sizeof(critString), " (CRIT: %g%%/%g%%)", log.critRollNeededFloat, log.critRollFloat);
  }

  LPCSTR          spellName = "";
  const SpellRec *rec = g_spellDB.GetRecord(log.spellID);
  if (rec) {
    spellName = rec->m_name_lang[CURRENT_LANGUAGE];
  }

  char auraString[128] = "";
  if ((log.flags & 4) && log.auraEffectID < 89) {
    SStrPrintf(auraString, sizeof(auraString), " (AURA: %s)", GetSpellAuraEffectName(log.auraEffectID));
  }

  char chanceString[128] = "";
  if (log.flags & 0x10) {
    SStrPrintf(chanceString, sizeof(chanceString), "(%g%%/%g%%)", log.hitRollNeededFloat, log.hitRollFloat);
  }

  char                  damageTypeString[32] = "(UNKNOWNTYPE)";
  const ResistancesRec *damageClass = GetDamageClassRecord(log.damageType);
  if (damageClass && !(log.flags & 0x482)) {
    SStrPrintf(damageTypeString, sizeof(damageTypeString), "(%s)", damageClass->m_name_lang[CURRENT_LANGUAGE]);
  }

  if (victimPtr && victimPtr->IsA(TYPE_UNIT) && attackerPtr && attackerPtr->IsA(TYPE_UNIT)) {
    SStrPrintf(
        string, size,
        "(%s) %s %s %s %s for %d points (type/min/max/base/scaled/net/bonus/modTaken/modDone) (%s/%d/%d/%g/%g/%g/%g) (DR: "
        "%g/%g(coeff=%g))%s%s",
        spellName, attackerPtr->GetUnitName(), actionString, chanceString, ((CGUnit_C *)victimPtr)->GetUnitName(), log.dmg.totalDamage,
        damageTypeString, log.dmg.minDamage[0], log.dmg.maxDamage[0], log.scaledDamage, log.dmg.damageFloat[0], log.modDamageTaken,
        log.modDamageDone, log.scaledArmorReduction, log.maxDamageReduction, log.resistanceCoefficient, critString, auraString
    );
  }
}

static LPCSTR formatString =
    "(%d)%s Hit (%g%%/%g%%) %s for %d points of %s damage(-/+/mDone/mTaken/actual/scaler) "
    "%d/%d/%g/%g/%d/%g ( %g/%g - %g(max:%g)) )%s%s%s";

static void NormalHitHandler(COMBATMESSAGEPRONOUNS &pronouns, const ATTACKROUNDINFO &info, char *buffer, UINT size) {
  FATALASSERT(buffer);
  FATALASSERT(size);
  LPCSTR                damageType = "";
  const ResistancesRec *damageClass = GetDamageClassRecord(info.dmg.damageType[0]);
  if (damageClass) {
    damageType = damageClass->m_name_lang[CURRENT_LANGUAGE];
  }

  char critString[128] = "";
  if (info.flags & 8) {
    SStrPrintf(critString, sizeof(critString), " (CRIT: %g%%/%g%%)", info.critRollNeededFloat, info.critRollFloat);
  }
  char stunString[128] = "";
  if (info.flags & 0x10) {
    SStrPrintf(
        stunString, sizeof(stunString), " (STUN: %g%%/%g%%%s%s)", info.stunRollNeededFloat, info.stunRollFloat, (info.flags & 0x800) ? " HIT" : "",
        (info.flags & 0x100) ? " CLDN" : ""
    );
  }
  char offHandString[128] = "";
  if (info.flags & 0x200) {
    SStrPrintf(offHandString, sizeof(offHandString), " (OFFHAND: %g%%/%g%%", info.dualWieldHitRollNeededFloat, info.dualWieldHitRollFloat);
  }
  int totalDamage = info.dmg.totalDamage;
  if (info.flags & 0x4000) {
    totalDamage = 0;
  }
  SStrPrintf(
      buffer, size, formatString, info.sinceLastSwing, pronouns.attackerName, info.hitRollNeededFloat, info.hitRollFloat, pronouns.victimName,
      totalDamage, damageType, info.dmg.minDamage[0], info.dmg.maxDamage[0], info.modDamageDone, info.modDamageTaken, totalDamage, info.DPSScaler,
      info.scaledDamage, info.dmg.damageFloat[0], info.scaledArmorReduction, info.maxDamageReduction, critString, stunString, offHandString
  );
}

static void NormalMissHandler(COMBATMESSAGEPRONOUNS &pronouns, const ATTACKROUNDINFO &info, char *buffer, UINT size) {
  FATALASSERT(buffer);
  FATALASSERT(size);
  if (info.flags & 0x8000) {
    SStrPrintf(buffer, size, "(%d) offhand failed (%g%%/%g%%)", info.sinceLastSwing, info.dualWieldHitRollNeededFloat, info.dualWieldHitRollFloat);
    return;
  }
  char buff[128] = "";
  if (info.flags & 0x200) {
    SStrPrintf(buff, sizeof(buff), " OFFHAND: (%g%%/%g%%)", info.dualWieldHitRollNeededFloat, info.dualWieldHitRollFloat);
  }
  SStrPrintf(
      buffer, size, "(%d)%s (%g%%/%g%%) Missed %s%s", info.sinceLastSwing, pronouns.attackerName, info.hitRollNeededFloat, info.hitRollFloat,
      pronouns.victimName, buff
  );
}

static void NormalBlockHandler(COMBATMESSAGEPRONOUNS &pronouns, const ATTACKROUNDINFO &info, char *buffer, UINT size) {
  FATALASSERT(buffer);
  FATALASSERT(size);
  GetDamageClassRecord(info.dmg.damageType[0]);
  SStrPrintf(
      buffer, size, "(%d)The attack of %s on %s (%g%%/%g%%,) is blocked (%g%%/%g%%)", info.sinceLastSwing, pronouns.attackerName, pronouns.victimName,
      info.hitRollNeededFloat, info.hitRollFloat, info.blockRollNeededFloat, info.blockRollFloat
  );
}

static void NormalParryHandler(COMBATMESSAGEPRONOUNS &pronouns, const ATTACKROUNDINFO &info, char *buffer, UINT size) {
  FATALASSERT(buffer);
  FATALASSERT(size);
  SStrPrintf(
      buffer, size, "(%d)The attack of %s on %s (%g%%/%g%%,) is parried (%g%%/%g%%) by %s", info.sinceLastSwing, pronouns.attackerName,
      pronouns.victimName, info.hitRollNeededFloat, info.hitRollFloat, info.parryRollNeededFloat, info.parryRollFloat, pronouns.victimName
  );
}

static void NormalDodgeHandler(COMBATMESSAGEPRONOUNS &pronouns, const ATTACKROUNDINFO &info, char *buffer, UINT size) {
  FATALASSERT(buffer);
  FATALASSERT(size);
  SStrPrintf(
      buffer, size, "(%d)The attack of %s on %s (%g%%/%g%%,) is dodged (%g%%/%g%%) by %s", info.sinceLastSwing, pronouns.attackerName,
      pronouns.victimName, info.hitRollNeededFloat, info.hitRollFloat, info.dodgeRollNeededFloat, info.dodgeRollFloat, pronouns.victimName
  );
}

static void NormalImmuneHandler(COMBATMESSAGEPRONOUNS &pronouns, const ATTACKROUNDINFO &info, char *buffer, UINT size) {
  FATALASSERT(buffer);
  FATALASSERT(size);
  SStrPrintf(
      buffer, size, "(%d)The attack of %s on %s (%g%%/%g%%,) failed, victim is immune", info.sinceLastSwing, pronouns.attackerName,
      pronouns.victimName, info.hitRollNeededFloat, info.hitRollFloat
  );
}

static void NormalEvadeHandler(COMBATMESSAGEPRONOUNS &pronouns, const ATTACKROUNDINFO &info, char *buffer, UINT size) {
  FATALASSERT(buffer);
  FATALASSERT(size);
  SStrPrintf(
      buffer, size, "(%d)The attack of %s on %s (%g%%/%g%%,) is evaded by %s", info.sinceLastSwing, pronouns.attackerName, pronouns.victimName,
      info.hitRollNeededFloat, info.hitRollFloat, pronouns.victimName
  );
}

static const RESULTTYPEHANDLERDESC s_resultTypeHandler[NUM_COMBATMESSAGETYPES] = {
    {NormalHitHandler,    1},
    {NormalMissHandler,   1},
    {NormalBlockHandler,  1},
    {NormalParryHandler,  3},
    {NormalDodgeHandler,  3},
    {NormalEvadeHandler,  3},
    {NormalImmuneHandler, 3}
};

static void Capitalize(char *string) {
  if (string && *string && islower(*string)) {
    *string -= 32;
  }
}

static void GeneratePronouns(COMBATMESSAGEPRONOUNS &pronouns, const ATTACKROUNDINFO &info) {
  FATALASSERT(info.attacker);
  FATALASSERT(info.victim);
  FATALASSERT(info.attacker != info.victim);
  DWORDLONG   activePlayer = ClntObjMgrGetActivePlayer();
  CGObject_C *attackerPtr = ClntObjMgrObjectPtr(info.attacker, __FILE__, __LINE__);
  CGObject_C *victimPtr = ClntObjMgrObjectPtr(info.victim, __FILE__, __LINE__);
  char        buf[32];
  LPCSTR      text = FrameScript_GetText("YOU", -1, GENDER_NOT_APPLICABLE);
  SStrCopy(buf, text, sizeof(buf));
  if (activePlayer == info.attacker) {
    SStrPrintf(pronouns.attackerName, sizeof(pronouns.attackerName), buf);
  } else if (attackerPtr && attackerPtr->IsA(TYPE_UNIT)) {
    SStrCopy(pronouns.attackerName, ((CGUnit_C *)attackerPtr)->GetUnitName(), sizeof(pronouns.attackerName));
  } else {
    SStrCopy(pronouns.attackerName, "NONAME", sizeof(pronouns.attackerName));
  }
  if (activePlayer == info.victim) {
    SStrPrintf(pronouns.victimName, sizeof(pronouns.victimName), buf);
  } else if (victimPtr && victimPtr->IsA(TYPE_UNIT)) {
    SStrCopy(pronouns.victimName, ((CGUnit_C *)victimPtr)->GetUnitName(), sizeof(pronouns.victimName));
  } else {
    SStrCopy(pronouns.victimName, "NONAME", sizeof(pronouns.victimName));
  }
}

static COMBATMESSAGETYPE DetermineResultType(const ATTACKROUNDINFO &info) {
  switch (info.newVictimState) {
    case VS_WOUND:
      return COMBATMESSAGETYPE_NORMALHIT;
    case VS_NONE:
      return COMBATMESSAGETYPE_NORMALMISS;
    case VS_EVADE:
      return COMBATMESSAGETYPE_NORMALEVADE;
    case VS_DODGE:
      return COMBATMESSAGETYPE_NORMALDODGE;
    case VS_PARRY:
      return COMBATMESSAGETYPE_NORMALPARRY;
    case VS_BLOCK:
      return COMBATMESSAGETYPE_NORMALBLOCK;
    case VS_IMMUNE:
      return COMBATMESSAGETYPE_NORMALIMMUNE;
    default:
      return COMBATMESSAGETYPE_UNKNOWN;
  }
}

static void CloseDebugLogHandle() {
  if (s_logHandle) {
    SLogClose(s_logHandle);
  }
  s_logHandle = 0;
}

static void WriteMessage(LPCSTR message) {
  if (message && *message && (s_flags & 2)) {
    SLogWrite(s_logHandle, "%s", message);
  }
}

static void OutputCombatMessage(const ATTACKROUNDINFO &info) {
  COMBATMESSAGEPRONOUNS pronouns;
  GeneratePronouns(pronouns, info);
  COMBATMESSAGETYPE resultType = DetermineResultType(info);
  if (resultType == NUM_COMBATMESSAGETYPES || resultType == COMBATMESSAGETYPE_UNKNOWN) {
    return;
  }
  if (s_resultTypeHandler[resultType].flags & 1) {
    Capitalize(pronouns.attackerName);
  }
  if (s_resultTypeHandler[resultType].flags & 2) {
    Capitalize(pronouns.victimName);
  }
  FATALASSERT(s_resultTypeHandler[resultType].handler);
  char buffer[256];
  s_resultTypeHandler[resultType].handler(pronouns, info, buffer, sizeof(buffer));
  ConsoleWrite(buffer, DEFAULT_COLOR);
  WriteMessage(buffer);
}

static void UnitCombatLogSpellTeach(const SpellRec *rec, DWORDLONG caster, DWORDLONG target) {
  if (!s_activePlayer) {
    return;
  }

  UNITAFFILIATION aAff = s_activePlayer->GetGUIDAffiliation(caster);
  UNITAFFILIATION vAff = s_activePlayer->GetGUIDAffiliation(target);
  CGObject_C     *attackerObjPtr;
  CGObject_C     *victimObjPtr;
  CGUnit_C       *attackerPtr;
  CGUnit_C       *victimPtr;
  if (!ShouldLog(caster, aAff, attackerObjPtr, attackerPtr, target, vAff, victimObjPtr, victimPtr, 0)) {
    return;
  }
  if (!victimPtr || !attackerPtr) {
    return;
  }

  LPCSTR templateTag;
  if (aAff == AFFILIATION_YOURSELF) {
    if (vAff == AFFILIATION_YOURSELF) {
      templateTag = "SPELLTEACHSELFSELF";
    } else {
      templateTag = "SPELLTEACHSELFOTHER";
    }
  } else {
    if (vAff == AFFILIATION_YOURSELF) {
      templateTag = "SPELLTEACHOTHERSELF";
    } else {
      templateTag = "SPELLTEACHOTHEROTHER";
    }
  }

  LPCSTR spellName = rec->m_name_lang[CURRENT_LANGUAGE];
  LPCSTR format = FrameScript_GetText(templateTag, -1, GENDER_NOT_APPLICABLE);
  if (format && *format) {
    LPCSTR victimName = victimPtr->GetUnitName();
    LPCSTR attackerName = attackerPtr->GetUnitName();
    if (aAff == AFFILIATION_YOURSELF) {
      if (vAff == AFFILIATION_YOURSELF) {
        GeneralLogPrintf(SLASH_CMD_COMBAT_LOG_MISC_INFO, format, spellName);
      } else {
        GeneralLogPrintf(SLASH_CMD_COMBAT_LOG_MISC_INFO, format, victimName, spellName);
      }
    } else {
      if (vAff == AFFILIATION_YOURSELF) {
        GeneralLogPrintf(SLASH_CMD_COMBAT_LOG_MISC_INFO, format, attackerName, spellName);
      } else {
        GeneralLogPrintf(SLASH_CMD_COMBAT_LOG_MISC_INFO, format, attackerName, victimName, spellName);
      }
    }
  } else {
    ReportError(templateTag);
  }
}

static void LogEnchantmentRequest(const ENCHANTMENTLOG &log) {
  UINT index = 0;
  for (; index < s_logDesc.Count(); ++index) {
    if (!s_logDesc[index].valid) {
      break;
    }
  }

  ENCHANTMENTLOGDESC *desc;
  if (index == s_logDesc.Count()) {
    desc = s_logDesc.New();
  } else {
    desc = &s_logDesc[index];
  }
  desc->valid = true;
  desc->log = log;
}

static void UnitCombatLogEnchantmentRemoved(const ENCHANTMENTLOG &log, bool isCallback) {
  FATALASSERT(log.attacker);
  CGObject_C     *attackerObjPtr;
  UNITAFFILIATION aAff;
  if (!ShouldLogAttacker(log.attacker, aAff, attackerObjPtr, 1, 0, -1)) {
    return;
  }

  LPCSTR                         attackerName = ((CGUnit_C *)attackerObjPtr)->GetUnitName();
  const SpellItemEnchantmentRec *rec = g_spellItemEnchantmentDB.GetRecord(log.enchantment);
  LPCSTR                         enchantmentName;
  if (rec) {
    enchantmentName = rec->m_name_lang[CURRENT_LANGUAGE];
  } else {
    enchantmentName = "Unknown Enchantment";
  }

  LPCSTR templateTag;
  if (aAff == AFFILIATION_YOURSELF) {
    templateTag = "ITEMENCHANTMENTREMOVESELF";
  } else {
    templateTag = "ITEMENCHANTMENTREMOVEOTHER";
  }

  const ItemStats_C *item = g_itemDBCache.GetRecord(log.itemID, 0, ItemEnchantmentCacheCallback, 0);
  if (!item) {
    if (!isCallback) {
      LogEnchantmentRequest(log);
    }
    return;
  }

  LPCSTR itemName = item->m_displayName[CURRENT_LANGUAGE];
  LPCSTR format = FrameScript_GetText(templateTag, -1, GENDER_NOT_APPLICABLE);
  char   output[256];
  if (!format || !*format) {
    SStrPrintf(output, sizeof(output), "Warning, string %s not found in stringfile.", templateTag);
  } else {
    if (aAff == AFFILIATION_YOURSELF) {
      SStrPrintf(output, sizeof(output), format, enchantmentName, itemName);
    } else {
      SStrPrintf(output, sizeof(output), format, enchantmentName, attackerName, itemName);
    }
  }
  GeneralLogPrintf(s_affiliationLogType[aAff], output);
}

static void UnitCombatLogEnchantmentAdded(const ENCHANTMENTLOG &log, bool isCallback) {
  CGObject_C     *attackerObjPtr;
  CGObject_C     *victimObjPtr;
  CGUnit_C       *attackerPtr;
  CGUnit_C       *victimPtr;
  UNITAFFILIATION aAff;
  UNITAFFILIATION vAff;
  if (!ShouldLog(log.attacker, aAff, attackerObjPtr, attackerPtr, log.victim, vAff, victimObjPtr, victimPtr, 1)) {
    return;
  }
  if (!victimPtr || !attackerPtr) {
    return;
  }

  LPCSTR                         casterName = attackerPtr->GetUnitName();
  LPCSTR                         victimName = victimPtr->GetUnitName();
  const SpellItemEnchantmentRec *rec = g_spellItemEnchantmentDB.GetRecord(log.enchantment);
  LPCSTR                         enchantmentName;
  if (rec) {
    enchantmentName = rec->m_name_lang[CURRENT_LANGUAGE];
  } else {
    enchantmentName = "Unknown Enchantment";
  }

  const ItemStats_C *item = g_itemDBCache.GetRecord(log.itemID, 0, ItemEnchantmentCacheCallback, 0);
  if (!item) {
    if (!isCallback) {
      LogEnchantmentRequest(log);
    }
    return;
  }

  LPCSTR itemName = item->m_displayName[CURRENT_LANGUAGE];
  LPCSTR templateTag;
  if (aAff == AFFILIATION_YOURSELF) {
    if (vAff == AFFILIATION_YOURSELF) {
      templateTag = "ITEMENCHANTMENTADDSELFSELF";
    } else {
      templateTag = "ITEMENCHANTMENTADDSELFOTHER";
    }
  } else {
    if (vAff == AFFILIATION_YOURSELF) {
      templateTag = "ITEMENCHANTMENTADDOTHERSELF";
    } else {
      templateTag = "ITEMENCHANTMENTADDOTHEROTHER";
    }
  }

  LPCSTR format = FrameScript_GetText(templateTag, -1, GENDER_NOT_APPLICABLE);
  char   output[256];
  if (!format || !*format) {
    SStrPrintf(output, sizeof(output), "Warning, string %s not found in stringfile.", templateTag);
  } else {
    if (aAff == AFFILIATION_YOURSELF) {
      if (vAff == AFFILIATION_YOURSELF) {
        SStrPrintf(output, sizeof(output), format, enchantmentName, itemName);
      } else {
        SStrPrintf(output, sizeof(output), format, enchantmentName, victimName, itemName);
      }
    } else {
      if (vAff == AFFILIATION_YOURSELF) {
        SStrPrintf(output, sizeof(output), format, casterName, enchantmentName, itemName);
      } else {
        SStrPrintf(output, sizeof(output), format, casterName, enchantmentName, victimName, itemName);
      }
    }
  }
  GeneralLogPrintf(s_affiliationLogType[aAff], output);
}

static void ItemEnchantmentCacheCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted) {
  if (!g_itemDBCache.GetRecord(id, 0, 0, 0)) {
    return;
  }

  for (UINT index = s_logDesc.Count(); index--;) {
    if (s_logDesc[index].valid && s_logDesc[index].log.itemID == id) {
      if (s_logDesc[index].log.flags & 1) {
        UnitCombatLogEnchantmentRemoved(s_logDesc[index].log, 1);
      } else {
        UnitCombatLogEnchantmentAdded(s_logDesc[index].log, 1);
      }
      s_logDesc[index].valid = false;
    }
  }
}

static void WriteString(int writeToConsole, TSGrowableArray<char> &array, LPCSTR format, ...) {
  char    buff[256];
  va_list args;
  va_start(args, format);
  SStrVPrintf(buff, sizeof(buff), format, args);
  va_end(args);
  buff[sizeof(buff) - 1] = 0;
  UINT chars = SStrLen(buff);
  array.Add(chars, buff);
  if (writeToConsole) {
    ConsoleWrite(buff, DEFAULT_COLOR);
  }
}

static void WriteSpellInfo(const COMBATLOGDESC &unit) {
  float critRate = unit.spellCritsAttempted ? (float)unit.spellCritsSucceeded / unit.spellCritsAttempted * 100.0f : 0.0f;
  WriteString(
      1, s_charArray, "%s: %d/%d crits/attempts (suffered %d), %02f%% crit rate\r\n", unit.m_name, unit.spellCritsSucceeded, unit.spellCritsAttempted,
      unit.spellCritsSuffered, critRate
  );
  WriteString(1, s_charArray, "%s: Received %d HP of healing\r\n", unit.m_name, unit.totalHealthHealed);
  int totalDamageSuffered = unit.totalReflectedDamageSuffered + unit.totalDamageSuffered;
  WriteString(
      1, s_charArray, "%s: %d/%d/%d points of reflected/normal/total damage received\r\n", unit.m_name, unit.totalReflectedDamageSuffered,
      unit.totalDamageSuffered, totalDamageSuffered
  );
  WriteString(1, s_charArray, "%s: Provided %d HP of healing\r\n", unit.m_name, unit.totalHealingProvided);
  WriteString(
      1, s_charArray, "%s: %d/%d/%d points of reflected/normal/total damage given\r\n", unit.m_name, unit.totalReflectedDamageProvided,
      unit.totalDamageProvided, unit.totalReflectedDamageProvided + unit.totalDamageProvided
  );
  WriteString(
      1, s_charArray, "%s: Total spell damage reduced by self/victim: %g/%g\r\n", unit.m_name, unit.totalSpellDamageReduced,
      unit.totalSpellDamageReducedByVictim
  );
  float reduced = totalDamageSuffered ? unit.totalSpellDamageReduced / totalDamageSuffered * 100.0f : 0.0f;
  WriteString(1, s_charArray, "%s: percent damage reduced: %g\r\n", unit.m_name, reduced);
}

static void WriteAttemptsHitsMisses(const COMBATLOGDESC &attacker) {
  WriteString(
      1, s_charArray, "%s Attempts/Hits/Misses on victim: %d/%d/%d\r\n", attacker.m_name, attacker.totalAttemptsByEntity, attacker.totalHits,
      attacker.totalMisses
  );
  UINT hitPercent = attacker.totalAttemptsByEntity ? 100 * attacker.totalHits / attacker.totalAttemptsByEntity : 0;
  WriteString(1, s_charArray, "%s percentage hits: %d\r\n", attacker.m_name, hitPercent);
  float critRate = attacker.totalAttemptsByEntity ? (float)attacker.criticalHits / attacker.totalAttemptsByEntity * 100.0f : 0.0f;
  WriteString(1, s_charArray, "%d/%d crits/attempts, %02f%% crit rate\r\n", attacker.criticalHits, attacker.totalHits, critRate);
}

static void WriteVictimStates(const COMBATLOGDESC &victim, LPCSTR name, UINT attempts, UINT successes) {
  WriteString(1, s_charArray, "%s %s Attempts/Success/Failure: %d/%d/%d\r\n", victim.m_name, name, attempts, successes, attempts - successes);
  float successRate = attempts ? (float)successes * 100.0f / attempts : 0.0f;
  WriteString(1, s_charArray, "%s Percentage %s successes: %g%%\r\n", victim.m_name, name, successRate);
}

static float RoundTo(float roundThis, float toThis) {
  if (toThis == 0.0f) {
    return roundThis;
  }

  float negate = 1.0f;
  if (roundThis < 0.0f) {
    negate = -1.0f;
    roundThis *= -1.0f;
  }
  if (toThis < 0.0f) {
    toThis *= -1.0f;
  }

  UINT count = roundThis / toThis;
  if (fmod(roundThis, toThis) >= toThis * 0.5) {
    ++count;
  }
  return count * negate * toThis;
}

static void WriteDamageTallies(const COMBATLOGDESC &desc, float seconds) {
  float perSecond = 1.0f / seconds;
  float reducedPerSecond = RoundTo(desc.totalDamageReducedByVictim * perSecond, 0.5f);
  WriteString(
      1, s_charArray, "  Total damage reduced by the armor of victim: %d (%g per second)\r\n", desc.totalDamageReducedByVictim, reducedPerSecond
  );
  UINT netDamage = desc.totalDamageDoneByEntity;
  UINT grossDamage = desc.totalDamageReducedByVictim + netDamage;
  WriteString(1, s_charArray, "  Gross damage suffered by victim: %d (%g per second)\r\n", grossDamage, RoundTo(grossDamage * perSecond, 0.3f));
  WriteString(1, s_charArray, "  Net damage suffered by victim: %d (%g per second)\r\n", netDamage, RoundTo(netDamage * perSecond, 0.3f));
  UINT percent = grossDamage ? 100 * desc.totalDamageReducedByVictim / grossDamage : 0;
  WriteString(1, s_charArray, "  Percent damage reduction: %d\r\n", percent);
}

static void LogResults() {
  s_charArray.SetCount(0);
  UINT currentTime = OsGetAsyncTimeMs();
  int  elapsedTime = s_lastLogTime - s_logStartTime;
  if (!elapsedTime) {
    elapsedTime = 1;
  }
  float seconds = elapsedTime * 0.001f;
  WriteString(1, s_charArray, "Combat Summary:\r\n");
  WriteString(1, s_charArray, "===============\r\n");
  WriteString(1, s_charArray, "Start Time: %d\r\n", s_logStartTime);
  WriteString(1, s_charArray, "End Time  : %d\r\n", currentTime);
  WriteString(1, s_charArray, "Elapsed time: %g seconds\r\n", seconds);
  for (UINT i = 0; i < AFFILIATION_NUMAFFILIATIONS; ++i) {
    WriteString(1, s_charArray, "Tallies for %s:\r\n", s_unitCombatData[i].m_name);
    WriteDamageTallies(s_unitCombatData[i], seconds);
  }
  for (UINT j = 0; j < AFFILIATION_NUMAFFILIATIONS; ++j) {
    WriteString(1, s_charArray, "--------------------\r\n");
    WriteAttemptsHitsMisses(s_unitCombatData[j]);
    WriteVictimStates(s_unitCombatData[j], "Parry", s_unitCombatData[j].parryAttempts, s_unitCombatData[j].totalVictimStatesByEntity[VS_PARRY]);
    WriteVictimStates(s_unitCombatData[j], "Dodge", s_unitCombatData[j].dodgeAttempts, s_unitCombatData[j].totalVictimStatesByEntity[VS_DODGE]);
    WriteVictimStates(s_unitCombatData[j], "Block", s_unitCombatData[j].blockAttempts, s_unitCombatData[j].totalVictimStatesByEntity[VS_BLOCK]);
    WriteString(1, s_charArray, "Spell Info:\r\n");
    WriteSpellInfo(s_unitCombatData[j]);
  }
  WriteString(1, s_charArray, "===============\r\n");
  const char terminator = 0;
  s_charArray.Add(1, &terminator);
}

static void ClearUnitDataStructs() {
  for (UINT i = 0; i < AFFILIATION_NUMAFFILIATIONS; ++i) {
    s_unitCombatData[i].Clear();
  }
}

COMBATLOGDESC::COMBATLOGDESC(LPCSTR name) {
  if (name && *name) {
    SStrPrintf(m_name, sizeof(m_name), name);
  } else {
    m_name[0] = 0;
  }
}

COMBATLOGDESC::~COMBATLOGDESC() {
  Clear();
}

void COMBATLOGDESC::Clear() {
  victims.Clear();
  attackers.Clear();
  totalDamageDoneByEntity = 0;
  totalDamageReducedByVictim = 0;
  totalAttemptsByEntity = 0;
  parryAttempts = 0;
  dodgeAttempts = 0;
  blockAttempts = 0;
  totalTimeDelayed = 0;
  criticalHits = 0;
  spellCritsAttempted = 0;
  spellCritsSucceeded = 0;
  spellCritsSuffered = 0;
  totalHealthHealed = 0;
  totalReflectedDamageSuffered = 0;
  totalDamageSuffered = 0;
  totalHealingProvided = 0;
  totalReflectedDamageProvided = 0;
  totalDamageProvided = 0;
  totalSpellDamageReducedByVictim = 0.0f;
  totalSpellDamageReduced = 0.0f;
  totalMisses = 0;
  totalHits = 0;
  for (UINT i = 0; i < NUM_VICTIMSTATES; ++i) {
    totalVictimStatesByEntity[i] = 0;
  }
}

void COMBATLOGDESC::LogAttack(const ATTACKROUNDINFO &info) {
  FATALASSERT(info.attacker);
  FATALASSERT(info.victim);
  FATALASSERT(info.attacker != info.victim);
  totalDamageDoneByEntity += info.dmg.totalDamage;
  totalDamageReducedByVictim += info.armorReduction;
  ++totalAttemptsByEntity;
  if (info.flags & 1) {
    ++totalMisses;
  } else {
    ++totalHits;
  }
  LogUnitGUID(info.victim, victims);
  LogUnitGUID(info.attacker, attackers);
  if (info.flags & 8) {
    ++criticalHits;
  }
}

void COMBATLOGDESC::LogAttack(const SPELLLOG &info) {
  FATALASSERT(info.attacker);
  FATALASSERT(info.victim);
  if (info.flags & 1) {
    ++spellCritsAttempted;
    if (info.flags & 0x40) {
      ++spellCritsSucceeded;
    }
  }
  if (info.flags & 0x82) {
    totalHealingProvided += info.dmg.totalDamage;
  } else if (info.flags & 8) {
    totalReflectedDamageProvided += info.dmg.totalDamage;
  } else {
    totalDamageProvided += info.dmg.totalDamage;
  }
  totalSpellDamageReducedByVictim += info.scaledArmorReduction;
}

void COMBATLOGDESC::LogVictim(const SPELLLOG &info) {
  FATALASSERT(info.attacker);
  FATALASSERT(info.victim);
  if (info.flags & 0x40) {
    ++spellCritsSuffered;
  }
  if (info.flags & 0x82) {
    totalHealthHealed += info.dmg.totalDamage;
  } else if (info.flags & 8) {
    totalReflectedDamageSuffered += info.dmg.totalDamage;
  } else {
    totalDamageSuffered += info.dmg.totalDamage;
  }
  totalSpellDamageReduced += info.scaledArmorReduction;
}

void COMBATLOGDESC::LogVictim(const ATTACKROUNDINFO &info) {
  FATALASSERT(info.attacker);
  FATALASSERT(info.victim);
  FATALASSERT(info.attacker != info.victim);
  FATALASSERT(info.newVictimState < NUM_VICTIMSTATES);
  ++totalVictimStatesByEntity[info.newVictimState];
  if (info.flags & 0x40) {
    ++parryAttempts;
  }
  if (info.flags & 0x20) {
    ++dodgeAttempts;
  }
  if (info.flags & 0x80) {
    ++blockAttempts;
  }
}

void COMBATLOGDESC::LogUnitGUID(DWORDLONG guid, TSHashTable<UNITHASHOBJ, CHashKeyGUID> &theTable) {
  UNITHASHOBJ *unit = theTable.Ptr(guid, guid);
  if (unit) {
    ++unit->count;
    return;
  }
  unit = theTable.New(guid, guid, 0, 0);
  ++unit->count;
}

void UnitDebugCombatLogOnEnable(int enable) {
  if (enable) {
    CloseDebugLogHandle();
    SLogCreate("PlayerCombatLog.txt", 0, &s_logHandle);
    s_charArray.SetChunkSize(256);
    s_charArray.ReserveSpace(256);
    ClearUnitDataStructs();
    s_flags &= ~1U;
    s_logStartTime = OsGetAsyncTimeMs();
    s_lastLogTime = s_logStartTime;
    s_flags |= 2;
  } else {
    if (s_flags & 2) {
      LogResults();
      WriteMessage(s_charArray.Ptr());
    }
    CloseDebugLogHandle();
    s_flags &= ~2U;
  }
}

static int Script_ToggleCombatLogFileWrite(lua_State *L) {
  UnitCombatLogEnableFileLog(!s_generalLogHandle);
  return 0;
}

static FrameScript_Method s_ScriptFunctions[] = {
    {"ToggleCombatLogFileWrite", Script_ToggleCombatLogFileWrite}
};

static BOOL DebugCombatLogHandler(LPCSTR command, LPCSTR arguments) {
  UnitCombatDebugLogEnable(arguments && SStrToInt(arguments));
  return 1;
}

void UnitCombatLogInitialize() {
  ConsoleCommandRegister("playercombatlogdebug", DebugCombatLogHandler, GAME, "Enables logging of combat");
  UINT i;
  for (i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
  for (i = 0; i < AFFILIATION_NUMAFFILIATIONS; ++i) {
    if (s_affMappingCVarsNames[i].cvarname || s_affMappingCVarsNames[i].defaultvalue) {
      CVar::Register(s_affMappingCVarsNames[i].cvarname, "", 0, s_affMappingCVarsNames[i].defaultvalue, 0, DEFAULT, false, 0);
    }
  }
  CVar::Register("CombatDeathLogRange", "", 0, "60", 0, DEFAULT, false, 0);
  CVar::Register("CombatLogPeriodicSpells", "", 0, "0", 0, DEFAULT, false, 0);
}

void UnitCombatLogShutdown() {
  ConsoleCommandUnregister("playercombatlogdebug");
  s_flags = 0;
  CloseDebugLogHandle();
  s_logDesc.Clear();
  for (UINT i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}

void UnitCombatDebugLogEnable(int enable) {
  CDataStore msg;
  msg.Put(CMSG_ENABLEDEBUGCOMBATLOGGING);
  msg.Put(enable);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void UnitCombatLog(const ATTACKROUNDINFO &roundInfo) {
  ATTACKROUNDINFO info(roundInfo);
  FATALASSERT(info.attacker);
  FATALASSERT(info.victim);
  FATALASSERT(info.attacker != info.victim);
  if (!s_activePlayer) {
    return;
  }
  if ((s_flags & 2) && (info.flags & 0x2000) && !(info.flags & 0x1000)) {
    if (!(s_flags & 1)) {
      s_flags |= 1;
      s_logStartTime = OsGetAsyncTimeMs();
      s_lastLogTime = s_logStartTime;
      ClearUnitDataStructs();
    }
    if (info.newVictimState == VS_DEFLECT) {
      info.newVictimState = (info.flags & 0x40000) ? VS_PARRY : VS_BLOCK;
    }
    OutputCombatMessage(info);
    s_lastLogTime = OsGetAsyncTimeMs();
    UNITAFFILIATION aAff = s_activePlayer->GetGUIDAffiliation(info.attacker);
    UNITAFFILIATION vAff = s_activePlayer->GetGUIDAffiliation(info.victim);
    FATALASSERT(aAff < AFFILIATION_NUMAFFILIATIONS);
    FATALASSERT(vAff < AFFILIATION_NUMAFFILIATIONS);
    if (aAff != AFFILIATION_OTHER || vAff != AFFILIATION_OTHER) {
      s_unitCombatData[vAff].LogVictim(info);
      s_unitCombatData[aAff].LogAttack(info);
    }
  }
  if (!(info.flags & 0x4000)) {
    HandleGeneralCombatLogging(info);
  }
}

void UnitCombatLog(const SPELLLOG &log) {
  if (!s_activePlayer) {
    return;
  }

  if (log.flags & 0x200) {
    const SpellRec *rec = g_spellDB.GetRecord(log.spellID);
    if (!rec || IsSpellQuiet(rec)) {
      return;
    }

    LPCSTR          spellName = rec->m_name_lang[CURRENT_LANGUAGE];
    UNITAFFILIATION aAff;
    CGObject_C     *attackerObjPtr;
    bool            result = ShouldLogAttacker(log.attacker, aAff, attackerObjPtr, 0, 0, -1);
    CGUnit_C       *attackerPtr = (CGUnit_C *)attackerObjPtr;
    if (result) {
      HandleSpellLogTerse(attackerPtr, aAff, spellName);
    }
    if (attackerPtr && (log.flags & 0x20)) {
      char buffer[512];
      SStrPrintf(buffer, sizeof(buffer), "%s) %s casts %s", spellName, attackerPtr->GetUnitName(), spellName);
      ConsolePrintf(buffer);
      WriteMessage(buffer);
    }
    return;
  }

  UNITAFFILIATION aAff = s_activePlayer->GetGUIDAffiliation(log.attacker);
  UNITAFFILIATION vAff = s_activePlayer->GetGUIDAffiliation(log.victim);
  CGObject_C     *attackerObjPtr;
  CGObject_C     *victimObjPtr;
  CGUnit_C       *attackerPtr;
  CGUnit_C       *victimPtr;
  if (!ShouldLog(log.attacker, aAff, attackerObjPtr, attackerPtr, log.victim, vAff, victimObjPtr, victimPtr, 0)) {
    return;
  }
  if (!victimPtr || !attackerPtr) {
    return;
  }

  if (log.flags & 0x20) {
    char outputString[512];
    FormatSpellString(outputString, sizeof(outputString), log);
    if (aAff != AFFILIATION_OTHER || vAff != AFFILIATION_OTHER) {
      s_unitCombatData[vAff].LogVictim(log);
      s_unitCombatData[aAff].LogAttack(log);
    }
    ConsoleWrite(outputString, DEFAULT_COLOR);
    WriteMessage(outputString);
    return;
  }

  if (log.flags & 0x100) {
    CVar *cvar = CVar::Lookup("CombatLogPeriodicSpells");
    if (!cvar || !cvar->GetInt()) {
      return;
    }
  }

  if (log.flags & 0x400) {
    HandleTerseVictimLogging(log.attacker, log.victim, log.spellID);
  } else if (log.flags & 0x82) {
    HandleGeneralHealLogging(log.dmg, log.spellID, log.attacker, log.victim);
  } else {
    HandleGeneralCombatOrSpellHitLogging(0, log.dmg, log.spellID, attackerPtr, victimPtr, log.flags & 0x40, 0, 0, aAff, vAff);
  }
}

void UnitCombatLog(const SPELLMISSLOG &log) {
  if ((s_flags & 2) && (log.flags & 8)) {
    char outputString[512];
    FormatSpellMissString(outputString, sizeof(outputString), log);
    ConsoleWrite(outputString, DEFAULT_COLOR);
    WriteMessage(outputString);
  }
}

static const char *s_affStrings[AFFILIATION_NUMAFFILIATIONS][NUM_UNIT_MIRROR_TIMERS] = {
    {"VSENVEXHAUSTIONSELF",  "VSENVBREATHSELF",  0},
    {"VSENVEXHAUSTIONOTHER", "VSENVBREATHOTHER", 0},
    {"VSENVEXHAUSTIONOTHER", "VSENVBREATHOTHER", 0},
    {0,                      0,                  0},
    {"VSENVEXHAUSTIONOTHER", "VSENVBREATHOTHER", 0}
};

void UnitCombatLog(const MIRRORTIMERDAMAGE &log) {
  if (log.damage < 0 || log.damage >= NUM_UNIT_MIRROR_TIMERS || !log.amount || !log.victim) {
    return;
  }

  CGObject_C     *objPtr;
  UNITAFFILIATION aff;
  if (!ShouldLogAttacker(log.victim, aff, objPtr, 1, 0, -1) || !objPtr->IsA(TYPE_UNIT)) {
    return;
  }
  if (!s_affStrings[aff][log.damage]) {
    return;
  }

  LPCSTR format = FrameScript_GetText(s_affStrings[aff][log.damage], -1, GENDER_NOT_APPLICABLE);
  if (format && *format) {
    if (aff == AFFILIATION_YOURSELF) {
      GeneralLogPrintf(s_affiliationLogType[aff], format, log.amount);
    } else {
      GeneralLogPrintf(s_affiliationLogType[aff], format, ((CGUnit_C *)objPtr)->GetUnitName(), log.amount);
    }
  } else {
    ReportError(s_affStrings[aff][log.damage]);
  }
}

void UnitCombatLog(const ENVIRONMENTALDAMAGE &log) {
  if (!log.victim || !log.amount) {
    return;
  }

  CGObject_C     *objPtr;
  UNITAFFILIATION aff;
  if (!ShouldLogAttacker(log.victim, aff, objPtr, 1, 0, -1) || !objPtr->IsA(TYPE_UNIT)) {
    return;
  }

  const ResistancesRec *rec = g_resistancesDB.GetRecord(log.school);
  if (!rec) {
    return;
  }

  char buffer[64];
  SStrPrintf(buffer, sizeof(buffer), "VSENVIRONMENTALDAMAGE_%d_%s", rec->m_ID, aff == AFFILIATION_YOURSELF ? "SELF" : "OTHER");
  LPCSTR format = FrameScript_GetText(buffer, -1, GENDER_NOT_APPLICABLE);
  if (format && *format) {
    if (aff == AFFILIATION_YOURSELF) {
      GeneralLogPrintf(s_affiliationLogType[aff], format, log.amount);
    } else {
      GeneralLogPrintf(s_affiliationLogType[aff], format, ((CGUnit_C *)objPtr)->GetUnitName(), log.amount);
    }
  } else {
    ReportError(buffer);
  }
}

void UnitCombatLogCastGo(UINT spellID, DWORDLONG casterUnit, DWORDLONG target) {
  if (!s_activePlayer) {
    return;
  }

  const SpellRec *rec = g_spellDB.GetRecord(spellID);
  if (!rec || IsSpellOpenLock(rec) || IsSpellQuiet(rec)) {
    return;
  }
  if (IsSpellTeach(rec)) {
    UnitCombatLogSpellTeach(rec, casterUnit, target);
    return;
  }

  CGObject_C     *attackerObjPtr;
  CGObject_C     *victimObjPtr;
  CGUnit_C       *attackerPtr;
  CGUnit_C       *victimPtr;
  UNITAFFILIATION aAff;
  UNITAFFILIATION vAff;
  if (!ShouldLog(casterUnit, aAff, attackerObjPtr, attackerPtr, target, vAff, victimObjPtr, victimPtr, 0)) {
    return;
  }
  if (!attackerPtr || !vAff) {
    return;
  }
  if (casterUnit == ClntObjMgrGetActivePlayer() && (target == casterUnit || IsSpellHarmful(rec))) {
    return;
  }
  if (casterUnit == ClntObjMgrGetActivePlayer() && !IsSpellAura(rec)) {
    return;
  }

  LPCSTR casterName = attackerPtr->GetUnitName();
  LPCSTR spellName = rec->m_name_lang[CURRENT_LANGUAGE];
  LPCSTR targetName;
  if (victimPtr) {
    targetName = victimPtr->GetUnitName();
  } else {
    if (!victimObjPtr->IsA(TYPE_OBJECT)) {
      return;
    }
    const ItemStats_C *item = g_itemDBCache.GetRecord(victimObjPtr->GetEntryID(), 0, 0, 0);
    if (!item || !*item->m_displayName[0]) {
      return;
    }
    targetName = item->m_displayName[0];
  }
  if (!targetName) {
    return;
  }

  bool   selfCasting = casterUnit == s_activePlayer->GetGUID();
  LPCSTR templateTag;
  if (IsSpellAbility(rec)) {
    if (selfCasting) {
      if (target != casterUnit) {
        templateTag = "SPELLPERFORMGOSELFTARGETTED";
      } else {
        templateTag = "SPELLPERFORMGOSELF";
      }
    } else {
      if (target != casterUnit) {
        templateTag = "SPELLPERFORMGOTARGETTED";
      } else {
        templateTag = "SPELLPERFORMGO";
      }
    }
  } else {
    if (selfCasting) {
      if (target != casterUnit) {
        templateTag = "SPELLCASTGOSELFTARGETTED";
      } else {
        templateTag = "SPELLCASTGOSELF";
      }
    } else {
      if (target != casterUnit) {
        templateTag = "SPELLCASTGOOTHERTARGETTED";
      } else {
        templateTag = "SPELLCASTGOOTHER";
      }
    }
  }

  LPCSTR format = FrameScript_GetText(templateTag, -1, GENDER_NOT_APPLICABLE);
  if (!format) {
    ReportError(templateTag);
    return;
  }

  char output[256];
  if (selfCasting) {
    if (target != casterUnit) {
      SStrPrintf(output, sizeof(output), format, spellName, targetName);
    } else {
      SStrPrintf(output, sizeof(output), format, spellName);
    }
  } else {
    if (target != casterUnit) {
      SStrPrintf(output, sizeof(output), format, casterName, spellName, targetName);
    } else {
      SStrPrintf(output, sizeof(output), format, casterName, spellName);
    }
  }
  GeneralLogPrintf(s_affiliationLogType[aAff], output);
  if (s_flags & 2) {
    ConsoleWrite(output, DEFAULT_COLOR);
    WriteMessage(output);
  }
}

void UnitCombatLogCastStart(UINT spellID, DWORDLONG caster) {
  if (!s_activePlayer) {
    return;
  }

  CGObject_C     *casterObjPtr = ClntObjMgrObjectPtr(caster, __FILE__, __LINE__);
  UNITAFFILIATION aAff;
  if (!ShouldLogAttacker(caster, aAff, casterObjPtr, 0, 0, -1) || !casterObjPtr || !casterObjPtr->IsA(TYPE_UNIT)) {
    return;
  }

  const SpellRec *rec = g_spellDB.GetRecord(spellID);
  if (!rec || IsSpellQuiet(rec)) {
    return;
  }

  bool aura = IsSpellAura(rec);
  if (caster == ClntObjMgrGetActivePlayer() && !aura) {
    return;
  }
  if (IsSpellTeach(rec)) {
    return;
  }

  LPCSTR casterName = ((CGUnit_C *)casterObjPtr)->GetUnitName();
  LPCSTR spellName = rec->m_name_lang[CURRENT_LANGUAGE];
  bool   selfCasting = caster == ClntObjMgrGetActivePlayer();
  bool   ability = IsSpellAbility(rec);
  LPCSTR templateTag;
  if (selfCasting) {
    if (ability) {
      templateTag = "SPELLPERFORMSELFSTART";
    } else {
      templateTag = "SPELLCASTSELFSTART";
    }
  } else {
    if (ability) {
      templateTag = "SPELLPERFORMOTHERSTART";
    } else {
      templateTag = "SPELLCASTOTHERSTART";
    }
  }

  LPCSTR format = FrameScript_GetText(templateTag, -1, GENDER_NOT_APPLICABLE);
  if (!format || !*format) {
    ReportError(templateTag);
    return;
  }

  char output[256];
  if (selfCasting) {
    SStrPrintf(output, sizeof(output), format, spellName);
  } else {
    SStrPrintf(output, sizeof(output), format, casterName, spellName);
  }
  if (s_flags & 2) {
    ConsoleWrite(output, DEFAULT_COLOR);
    WriteMessage(output);
  }
}

void UnitCombatLogAuraAddedOrRemoved(CGUnit_C *unitPtr, int spellID, bool added, int auraSlot) {
  const SpellRec *spellRec = g_spellDB.GetRecord(spellID);
  CGObject_C     *dummy;
  UNITAFFILIATION aAff;
  if (!unitPtr || !spellRec || (spellRec->m_attributes & 0xC0) || IsSpellQuiet(spellRec) ||
      !ShouldLogAttacker(unitPtr->GetGUID(), aAff, dummy, 0, 0, -1))
  {
    return;
  }

  LPCSTR token;
  if (!added) {
    if (aAff == AFFILIATION_YOURSELF) {
      token = "AURAREMOVEDSELF";
    } else {
      token = "AURAREMOVEDOTHER";
    }
  } else if (auraSlot >= 32 && auraSlot < 40) {
    if (aAff == AFFILIATION_YOURSELF) {
      token = "AURAADDEDSELFHARMFUL";
    } else {
      token = "AURAADDEDOTHERHARMFUL";
    }
  } else {
    if (aAff == AFFILIATION_YOURSELF) {
      token = "AURAADDEDSELFHELPFUL";
    } else {
      token = "AURAADDEDOTHERHELPFUL";
    }
  }

  LPCSTR format = FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE);
  if (!format || !*format) {
    ReportError(token);
    return;
  }

  char string[128];
  if (added) {
    if (aAff == AFFILIATION_YOURSELF) {
      SStrPrintf(string, sizeof(string), format, spellRec->m_name_lang[CURRENT_LANGUAGE]);
    } else {
      SStrPrintf(string, sizeof(string), format, unitPtr->GetUnitName(), spellRec->m_name_lang[CURRENT_LANGUAGE]);
    }
  } else {
    if (aAff == AFFILIATION_YOURSELF) {
      SStrPrintf(string, sizeof(string), format, spellRec->m_name_lang[CURRENT_LANGUAGE]);
    } else {
      SStrPrintf(string, sizeof(string), format, spellRec->m_name_lang[CURRENT_LANGUAGE], unitPtr->GetUnitName());
    }
  }

  GeneralLogPrintf(s_affiliationLogType[aAff], string);
  if (s_flags & 2) {
    ConsoleWrite(string, DEFAULT_COLOR);
    WriteMessage(string);
  }
}

void UnitCombatLogSpellMissed(UINT missReason, UINT spellID, DWORDLONG caster, DWORDLONG victim) {
  if (!missReason || !s_activePlayer) {
    return;
  }

  UNITAFFILIATION aAff = s_activePlayer->GetGUIDAffiliation(caster);
  UNITAFFILIATION vAff = s_activePlayer->GetGUIDAffiliation(victim);
  CGObject_C     *attackerObjPtr;
  CGObject_C     *victimObjPtr;
  CGUnit_C       *attackerPtr;
  CGUnit_C       *victimPtr;
  if (!ShouldLog(caster, aAff, attackerObjPtr, attackerPtr, victim, vAff, victimObjPtr, victimPtr, 0)) {
    return;
  }
  if (!victimPtr || !attackerPtr) {
    return;
  }

  const SpellRec *rec = g_spellDB.GetRecord(spellID);
  if (!rec || IsSpellQuiet(rec)) {
    return;
  }

  LPCSTR spellName = rec->m_name_lang[CURRENT_LANGUAGE];
  LPCSTR casterName = attackerPtr->GetUnitName();
  LPCSTR victimName = victimPtr->GetUnitName();
  LPCSTR templateTag;
  switch (missReason) {
    case 2:
      templateTag = s_spellResistStrings[aAff][vAff];
      break;
    case 3:
    case 8:
      templateTag = s_spellImmuneStrings[aAff][vAff];
      break;
    case 4:
      templateTag = s_spellEvadedStrings[aAff][vAff];
      break;
    case 5:
      templateTag = s_spellDodgedStrings[aAff][vAff];
      break;
    case 6:
      templateTag = s_spellParriedStrings[aAff][vAff];
      break;
    case 7:
      templateTag = s_spellBlockedStrings[aAff][vAff];
      break;
    case 9:
      templateTag = s_spellDeflectedStrings[aAff][vAff];
      break;
    default:
      templateTag = s_spellMissStrings[aAff][vAff];
      break;
  }
  if (!templateTag || !*templateTag) {
    return;
  }

  LPCSTR format = FrameScript_GetText(templateTag, -1, GENDER_NOT_APPLICABLE);
  UINT   required = s_requiredSpellMissStrings[aAff][vAff];
  char   output[128] = "";
  if (!format || !*format) {
    ReportError(templateTag);
    return;
  }

  if (required == 1) {
    SStrPrintf(output, sizeof(output), format, casterName, spellName);
  } else if (required == 2) {
    SStrPrintf(output, sizeof(output), format, spellName, victimName);
  } else if (required == 3) {
    SStrPrintf(output, sizeof(output), format, casterName, spellName, victimName);
  } else {
    return;
  }
  GeneralLogPrintf(s_affiliationLogType[aAff], output);
}

void UnitCombatLogUnitDead(DWORDLONG unit) {
  if (!s_activePlayer) {
    return;
  }

  UNITAFFILIATION aAff;
  CGObject_C     *unitObjPtr;
  if (!ShouldLogAttacker(unit, aAff, unitObjPtr, 0, 1, -1) || (((CGUnit_C *)unitObjPtr)->GetUnitFlags() & 0x80)) {
    return;
  }

  LPCSTR unitName = ((CGUnit_C *)unitObjPtr)->GetUnitName();
  LPCSTR templateTag = aAff ? "UNITDIESOTHER" : "UNITDIESSELF";
  LPCSTR format = FrameScript_GetText(templateTag, -1, GENDER_NOT_APPLICABLE);
  if (!format || !*format) {
    ReportError(templateTag);
    return;
  }

  char output[128];
  if (aAff) {
    SStrPrintf(output, sizeof(output), format, unitName);
  } else {
    SStrPrintf(output, sizeof(output), "%s", format);
  }
  GeneralLogPrintf(s_affiliationLogType[aAff], output);
}

void UnitCombatLogEnableFileLog(int enable) {
  if (enable) {
    if (!s_generalLogHandle) {
      SLogCreate("Logs.Client\\PlayerCombatLog.txt", 0, &s_generalLogHandle);
    }
    CGChat::AddChatMessage(FrameScript_GetText("COMBATLOGENABLED", -1, GENDER_NOT_APPLICABLE), SLASH_CMD_COMBAT_LOG_MISC_INFO, 0, 0, 0, 0, 0);
  } else {
    CGChat::AddChatMessage(FrameScript_GetText("COMBATLOGDISABLED", -1, GENDER_NOT_APPLICABLE), SLASH_CMD_COMBAT_LOG_MISC_INFO, 0, 0, 0, 0, 0);
    if (s_generalLogHandle) {
      SLogClose(s_generalLogHandle);
    }
    s_generalLogHandle = 0;
  }
}

void UnitCombatLogSetActivePlayer(const CGPlayer_C *playerPtr) {
  s_activePlayer = playerPtr;
}

void UnitCombatLogXPGain(const DWORDLONG &victim, CDataStore *msg, UINT count) {
  for (UINT i = 0; i < count; ++i) {
    DWORDLONG guid;
    int       xp;
    msg->Get(guid);
    msg->Get(xp);
    CGObject_C *playerPtr = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
    if (!playerPtr || !xp) {
      break;
    }
    FATALASSERT(playerPtr->IsA(TYPE_PLAYER));
    if (playerPtr->GetGUID() == ClntObjMgrGetActivePlayer()) {
      CGObject_C *victimPtr = ClntObjMgrObjectPtr(victim, __FILE__, __LINE__);
      if (victimPtr && victimPtr->IsA(TYPE_UNIT)) {
        ((CGUnit_C *)victimPtr)->StoreXPGain(xp);
      }
    }
  }
}

void UnitCombatLogSpellFail(CGUnit_C *caster, int spellID, LPCSTR message) {
  if (!s_activePlayer || !caster || !spellID) {
    return;
  }
  if (!message) {
    message = "";
  }

  UNITAFFILIATION aAff = s_activePlayer->GetGUIDAffiliation(caster->GetGUID());
  CGObject_C     *dummy;
  if (!ShouldLogAttacker(caster->GetGUID(), aAff, dummy, 0, 0, -1)) {
    return;
  }

  LPCSTR          casterName = caster->GetUnitName();
  const SpellRec *spellRec = g_spellDB.GetRecord(spellID);
  if (!spellRec || IsSpellQuiet(spellRec)) {
    return;
  }

  bool   selfCasting = caster->GetGUID() == s_activePlayer->GetGUID();
  LPCSTR templateTag;
  if (IsSpellAbility(spellRec)) {
    if (selfCasting) {
      templateTag = "SPELLFAILPERFORMSELF";
    } else {
      templateTag = "SPELLFAILPERFORMOTHER";
    }
  } else {
    if (selfCasting) {
      templateTag = "SPELLFAILCASTSELF";
    } else {
      templateTag = "SPELLFAILCASTOTHER";
    }
  }

  LPCSTR spellName = spellRec->m_name_lang[CURRENT_LANGUAGE];
  LPCSTR format = FrameScript_GetText(templateTag, -1, GENDER_NOT_APPLICABLE);
  if (!format || !*format) {
    ReportError(templateTag);
    return;
  }

  char output[256];
  if (selfCasting) {
    SStrPrintf(output, sizeof(output), format, spellName, message);
  } else {
    SStrPrintf(output, sizeof(output), format, casterName, spellName, message);
  }
  GeneralLogPrintf(SLASH_CMD_COMBAT_LOG_ERROR, output);
  if (s_flags & 2) {
    ConsoleWrite(output, DEFAULT_COLOR);
    WriteMessage(output);
  }
}

void UnitCombatLogHeartbeatResist(const RESISTLOG &log) {
  if (!s_activePlayer || !(s_flags & 2)) {
    return;
  }

  CGObject_C     *attackerObjPtr;
  CGObject_C     *victimObjPtr;
  CGUnit_C       *caster;
  CGUnit_C       *victim;
  UNITAFFILIATION aAff;
  UNITAFFILIATION vAff;
  if (!ShouldLog(log.attacker, aAff, attackerObjPtr, caster, log.victim, vAff, victimObjPtr, victim, 1)) {
    return;
  }
  if (!caster || !victim) {
    return;
  }

  const SpellRec *spellRec = g_spellDB.GetRecord(log.spell);
  if (!spellRec || IsSpellQuiet(spellRec)) {
    return;
  }

  LPCSTR casterName = caster->GetUnitName();
  LPCSTR victimName = victim->GetUnitName();
  char   output[256];
  SStrPrintf(
      output, sizeof(output), "%s (castlevel %d) %s the \"%s\" aura of %s (%s) (%g%%/%g%%)", victimName, log.castLevel,
      (log.flags & 2) ? "resists" : "fails to resist", spellRec->m_name_lang[0], casterName, (log.flags & 1) ? "DAMAGE" : "HEARTBEAT",
      log.resistRollNeeded, log.resistRoll
  );
  ConsoleWrite(output, DEFAULT_COLOR);
  WriteMessage(output);
}

void UnitCombatLogEnchantment(const ENCHANTMENTLOG &log) {
  if (!s_activePlayer) {
    return;
  }
  if (log.flags & 1) {
    UnitCombatLogEnchantmentRemoved(log, 0);
  } else {
    UnitCombatLogEnchantmentAdded(log, 0);
  }
}

void UnitCombatLogString(LPCSTR buffer) {
  if (buffer && *buffer) {
    GeneralLogPrintf(SLASH_CMD_COMBAT_LOG_SELF, "%s", buffer);
  }
}

void UnitCombatLogFactionChanged(int faction, int delta) {
  const FactionRec *rec = g_factionDB.GetRecord(faction);
  if (!rec || !delta) {
    return;
  }

  LPCSTR token = delta < 0 ? "FACTION_STANDING_DECREASED" : "FACTION_STANDING_INCREASED";
  LPCSTR format = FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE);
  if (!format) {
    GeneralLogPrintf(SLASH_CMD_COMBAT_LOG_MISC_INFO, "Error, cannot find string <%s>", token);
    return;
  }
  if (delta <= 0) {
    delta = -delta;
  }
  GeneralLogPrintf(SLASH_CMD_COMBAT_LOG_SELF, format, rec->m_name_lang[CURRENT_LANGUAGE], delta);
}

void UnitCombatLogPartyKill(const PARTYKILLLOG &log) {
  if (log.killer == ClntObjMgrGetActivePlayer()) {
    CGTutorial::TriggerTutorial(TUTORIAL_LOOTING);
  }
  if (!s_activePlayer || s_activePlayer->GetGUID() == log.killer) {
    return;
  }

  CGObject_C     *objPtr;
  UNITAFFILIATION aff;
  if (!ShouldLogAttacker(log.killer, aff, objPtr, 1, 0, -1) || aff != AFFILIATION_PARTYMEMBER) {
    return;
  }

  CGObject_C *victimPtr = ClntObjMgrObjectPtr(log.victim, __FILE__, __LINE__);
  LPCSTR      format = FrameScript_GetText("PARTYKILLOTHER", -1, GENDER_NOT_APPLICABLE);
  if (!format) {
    GeneralLogPrintf(SLASH_CMD_COMBAT_LOG_MISC_INFO, "Error, cannot find string <%s>", "PARTYKILLOTHER");
    return;
  }
  if (victimPtr && victimPtr->IsA(TYPE_UNIT)) {
    GeneralLogPrintf(
        SLASH_CMD_COMBAT_LOG_PARTY, format, ((CGUnit_C *)victimPtr)->GetUnitName(), ((CGUnit_C *)objPtr)->GetUnitName()
    );
  }
}

void UnitCombatLogShowXPGained(const DWORDLONG &victim, int xp) {
  CGObject_C *victimPtr = ClntObjMgrObjectPtr(victim, __FILE__, __LINE__);
  if (victimPtr && victimPtr->IsA(TYPE_UNIT)) {
    GeneralLogPrintf(
        SLASH_CMD_COMBAT_LOG_SELF, FrameScript_GetText("COMBATLOG_XPGAIN_FIRSTPERSON", -1, GENDER_NOT_APPLICABLE),
        ((CGUnit_C *)victimPtr)->GetUnitName(), xp
    );
  }
}
