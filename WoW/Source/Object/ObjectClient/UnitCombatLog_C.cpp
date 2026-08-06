#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

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
  UINT                                   totalVictimStatesByEntity[9];
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
  void LogVictim(const SPELLLOG &info);
  void LogVictim(const ATTACKROUNDINFO &info);
  void LogUnitGUID(DWORDLONG guid, TSHashTable<UNITHASHOBJ, CHashKeyGUID> &theTable);
};

struct COMBATMESSAGEPRONOUNS {
  char attackerName[48];
  char victimName[48];
};

struct ENCHANTMENTLOGDESC {
  ENCHANTMENTLOGDESC() : valid(false) {
  }

  ENCHANTMENTLOGDESC(const ENCHANTMENTLOGDESC &other) : valid(other.valid), log(other.log) {
  }

  bool           valid;
  ENCHANTMENTLOG log;
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

void UnitCombatLogEnableFileLog(int enable);
void UnitCombatLogShowXPGained(const DWORDLONG &victim, int xp);
void UnitCombatLogEnchantment(const ENCHANTMENTLOG &log);

static HSLOG                               s_logHandle;
static HSLOG                               s_generalLogHandle;
static UINT                                s_flags;
static const CGPlayer_C                   *s_activePlayer;
static TSGrowableArray<char>               s_charArray;
static TSGrowableArray<ENCHANTMENTLOGDESC> s_logDesc;
static UINT                                s_logStartTime;
static UINT                                s_lastLogTime;
static COMBATLOGDESC s_unitCombatData[AFFILIATION_NUMAFFILIATIONS] = {"You", "Your Pet", "Party Members", "Enemy", "Your Charmer"};
static LPCSTR        formatString =
    "(%d)%s Hit (%g%%/%g%%) %s for %d points of %s damage(-/+/mDone/mTaken/actual/scaler) "
    "%d/%d/%g/%g/%d/%g ( %g/%g - %g(max:%g)) )%s%s%s";

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
  totalMisses = 0;
  totalHits = 0;
  memset(totalVictimStatesByEntity, 0, sizeof(totalVictimStatesByEntity));
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
}

void COMBATLOGDESC::LogUnitGUID(DWORDLONG guid, TSHashTable<UNITHASHOBJ, CHashKeyGUID> &theTable) {
  CHashKeyGUID key(guid);
  UINT         hash = static_cast<UINT>(guid);
  UNITHASHOBJ *unit = theTable.Ptr(hash, key);
  if (!unit) {
    unit = theTable.New(hash, key, 0, 0);
  }
  ++unit->count;
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

static SLASH_COMMAND_ID s_affiliationLogType[AFFILIATION_NUMAFFILIATIONS] = {
    SLASH_CMD_COMBAT_LOG_SELF, SLASH_CMD_COMBAT_LOG_PARTY, SLASH_CMD_COMBAT_LOG_PARTY, SLASH_CMD_COMBAT_LOG_ENEMY, SLASH_CMD_COMBAT_LOG_PARTY
};

static float GetLogDistance(UNITAFFILIATION aff, bool suppressUnaffiliated) {
  if (aff >= AFFILIATION_NUMAFFILIATIONS || (suppressUnaffiliated && aff == AFFILIATION_OTHER)) {
    return 0.0f;
  }

  LPCSTR cvarName = 0;
  if (aff == AFFILIATION_PARTYMEMBER) {
    cvarName = "CombatLogPartyRange";
  } else if (aff == AFFILIATION_OTHER) {
    cvarName = "CombatLogRange";
  }

  if (!cvarName) {
    return 100000.0f;
  }
  CVar *cvar = CVar::Lookup(cvarName);
  return cvar ? cvar->GetFloat() : 0.0f;
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
  if (!(allowedAffiliationFlags & (1 << aAff))) {
    return 0;
  }

  unitPtr = ClntObjMgrObjectPtr(attacker, __FILE__, __LINE__);
  if (!unitPtr) {
    return 0;
  }

  CVar *cvar = CVar::Lookup("CombatDeathLogRange");
  float dist = useDeathRange && cvar ? cvar->GetFloat() : GetLogDistance(aAff, suppressIfUnaffiliated);
  return (s_activePlayer->GetPosition() - unitPtr->GetPosition()).SquaredMag() <= dist * dist;
}

static int ShouldLog(
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
  if (objectPtr->GetType() & TYPE_UNIT) {
    attackerPtr = static_cast<CGUnit_C *>(objectPtr);
  }
  if (subjectPtr->GetType() & TYPE_UNIT) {
    victimPtr = static_cast<CGUnit_C *>(subjectPtr);
  }

  float objectRangeSquared = GetLogDistance(aAff, suppressIfAllUnaffiliated);
  float subjectRangeSquared = GetLogDistance(vAff, suppressIfAllUnaffiliated);
  objectRangeSquared *= objectRangeSquared;
  subjectRangeSquared *= subjectRangeSquared;
  NTempest::C3Vector objectDiff = s_activePlayer->GetPosition() - objectPtr->GetPosition();
  float              objectSquaredMag = objectDiff.SquaredMag();
  float              subjectSquaredMag = (s_activePlayer->GetPosition() - subjectPtr->GetPosition()).SquaredMag();

  if (aAff == AFFILIATION_PARTYMEMBER && objectSquaredMag > objectRangeSquared) {
    return 0;
  }
  if (vAff == AFFILIATION_PARTYMEMBER && subjectSquaredMag > subjectRangeSquared) {
    return 0;
  }
  return objectSquaredMag < objectRangeSquared || subjectSquaredMag < subjectRangeSquared;
}

static bool IsSpellTeach(const SpellRec *rec) {
  for (UINT effect = 0; effect < 3; ++effect) {
    if (rec->m_effect[effect] == 36) {
      return 1;
    }
  }
  return 0;
}

static bool IsSpellAbility(const SpellRec *rec) {
  return (static_cast<UINT>(rec->m_attributes) >> 4) & 1;
}

static BYTE IsSpellHarmful(const SpellRec *rec) {
  for (UINT effect = 0; effect < 3; ++effect) {
    if (rec->m_effect[effect] == 2 || rec->m_effect[effect] == 58 || rec->m_effect[effect] == 17 || rec->m_effect[effect] == 31 ||
        rec->m_effect[effect] == 62)
    {
      return 1;
    }
  }
  return 0;
}

static BYTE IsSpellOpenLock(const SpellRec *rec) {
  for (UINT effect = 0; effect < 3; ++effect) {
    if (rec->m_effect[effect] == 33 || rec->m_effect[effect] == 59) {
      return 1;
    }
  }
  return 0;
}

static bool IsSpellQuiet(const SpellRec *rec) {
  return (static_cast<UINT>(rec->m_attributes) >> 7) & 1;
}

bool        IsSpellAura(const SpellRec *rec);
void        UnitCombatLogSpellMissed(UINT missReason, UINT spellID, DWORDLONG caster, DWORDLONG victim);
static void ItemEnchantmentCacheCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted);

static void LogEnchantmentRequest(const ENCHANTMENTLOG &log) {
  UINT index = 0;
  while (index < s_logDesc.Count() && s_logDesc[index].valid) {
    ++index;
  }

  ENCHANTMENTLOGDESC *desc = index == s_logDesc.Count() ? s_logDesc.New() : &s_logDesc[index];
  desc->valid = true;
  desc->log = log;
}

static void CloseDebugLogHandle() {
  if (s_logHandle) {
    SLogClose(s_logHandle);
  }
  s_logHandle = 0;
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

static void UnitCombatLogSpellTeach(const SpellRec *rec, DWORDLONG caster, DWORDLONG target) {
  CGObject_C *casterObject = ClntObjMgrObjectPtr(caster, __FILE__, __LINE__);
  CGObject_C *targetObject = ClntObjMgrObjectPtr(target, __FILE__, __LINE__);
  if (!rec || !casterObject || !targetObject || !(casterObject->GetType() & TYPE_UNIT) || !(targetObject->GetType() & TYPE_UNIT)) {
    return;
  }
  GeneralLogPrintf(
      SLASH_CMD_COMBAT_LOG_SELF, "%s teaches %s to %s.", static_cast<CGUnit_C *>(casterObject)->GetUnitName(), rec->m_name_lang[CURRENT_LANGUAGE],
      static_cast<CGUnit_C *>(targetObject)->GetUnitName()
  );
}

static void HandleTerseVictimLogging(DWORDLONG attacker, DWORDLONG victim, UINT spellID) {
  CGObject_C     *attackerObject = ClntObjMgrObjectPtr(attacker, __FILE__, __LINE__);
  CGObject_C     *victimObject = ClntObjMgrObjectPtr(victim, __FILE__, __LINE__);
  const SpellRec *spell = g_spellDB.GetRecord(spellID);
  if (attackerObject && victimObject && spell && (attackerObject->GetType() & TYPE_UNIT) && (victimObject->GetType() & TYPE_UNIT)) {
    GeneralLogPrintf(
        SLASH_CMD_COMBAT_LOG_PARTY, "%s's %s affects %s.", static_cast<CGUnit_C *>(attackerObject)->GetUnitName(),
        spell->m_name_lang[CURRENT_LANGUAGE], static_cast<CGUnit_C *>(victimObject)->GetUnitName()
    );
  }
}

static void HandleGeneralHealLogging(const DamageData &dmg, UINT spellID, DWORDLONG attacker, DWORDLONG victim) {
  CGObject_C     *attackerObject = ClntObjMgrObjectPtr(attacker, __FILE__, __LINE__);
  CGObject_C     *victimObject = ClntObjMgrObjectPtr(victim, __FILE__, __LINE__);
  const SpellRec *spell = g_spellDB.GetRecord(spellID);
  if (attackerObject && victimObject && spell && (attackerObject->GetType() & TYPE_UNIT) && (victimObject->GetType() & TYPE_UNIT)) {
    GeneralLogPrintf(
        SLASH_CMD_COMBAT_LOG_SELF, "%s's %s heals %s for %d.", static_cast<CGUnit_C *>(attackerObject)->GetUnitName(),
        spell->m_name_lang[CURRENT_LANGUAGE], static_cast<CGUnit_C *>(victimObject)->GetUnitName(), -dmg.totalDamage
    );
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
  if (!attackerPtr || !victimPtr) {
    return;
  }
  const SpellRec *spell = spellID ? g_spellDB.GetRecord(spellID) : 0;
  LPCSTR          spellName = spell ? spell->m_name_lang[CURRENT_LANGUAGE] : 0;
  LPCSTR          victimName = victimPtr->GetUnitName();
  if (spellName) {
    GeneralLogPrintf(
        s_affiliationLogType[aAff], "%s's %s hits %s for %d%s.", attackerPtr->GetUnitName(), spellName, victimName, dmg.totalDamage,
        critted ? " (critical)" : ""
    );
  } else {
    GeneralLogPrintf(
        s_affiliationLogType[aAff], "%s hits %s for %d%s.", attackerPtr->GetUnitName(), victimName, dmg.totalDamage, critted ? " (critical)" : ""
    );
  }
}

static void HandleSpellLogTerse(CGUnit_C *attackerPtr, UNITAFFILIATION aAff, LPCSTR spellNameString) {
  if (attackerPtr && spellNameString) {
    GeneralLogPrintf(s_affiliationLogType[aAff], "%s casts %s.", attackerPtr->GetUnitName(), spellNameString);
  }
}

static void HandleGeneralCombatLoggingMissed(
    const ATTACKROUNDINFO &info,
    CGUnit_C              *attackerPtr,
    CGUnit_C              *victimPtr,
    UNITAFFILIATION        aAff,
    UNITAFFILIATION        vAff
) {
  if (attackerPtr && victimPtr) {
    LPCSTR attackerName = attackerPtr->GetUnitName();
    GeneralLogPrintf(s_affiliationLogType[aAff], "%s misses %s.", attackerName, victimPtr->GetUnitName());
  }
}

static void
HandleGeneralCombatEvadeLogging(const ATTACKROUNDINFO info, CGUnit_C *attackerPtr, CGUnit_C *victimPtr, UNITAFFILIATION aAff, UNITAFFILIATION vAff) {
  if (attackerPtr && victimPtr) {
    LPCSTR attackerName = attackerPtr->GetUnitName();
    GeneralLogPrintf(s_affiliationLogType[aAff], "%s's attack was evaded by %s.", attackerName, victimPtr->GetUnitName());
  }
}

static void HandleGeneralCombatLogging(const ATTACKROUNDINFO &info) {
  CGObject_C     *attackerObjPtr;
  CGObject_C     *victimObjPtr;
  CGUnit_C       *attackerPtr;
  CGUnit_C       *victimPtr;
  UNITAFFILIATION aAff;
  UNITAFFILIATION vAff;
  if (!ShouldLog(info.attacker, aAff, attackerObjPtr, attackerPtr, info.victim, vAff, victimObjPtr, victimPtr, 0)) {
    return;
  }
  if (info.newVictimState == VS_EVADE) {
    HandleGeneralCombatEvadeLogging(info, attackerPtr, victimPtr, aAff, vAff);
  } else if (!info.dmg.totalDamage) {
    HandleGeneralCombatLoggingMissed(info, attackerPtr, victimPtr, aAff, vAff);
  } else {
    HandleGeneralCombatOrSpellHitLogging(1, info.dmg, 0, attackerPtr, victimPtr, (info.flags & 2) != 0, 0, 0, aAff, vAff);
  }
}

static void FormatSpellMissString(char *string, UINT size, const SPELLMISSLOG &log) {
  const SpellRec *spell = g_spellDB.GetRecord(log.spellID);
  SStrPrintf(string, size, "%s missed (reason %u).", spell ? spell->m_name_lang[CURRENT_LANGUAGE] : "Spell", log.reason);
}

static void FormatSpellString(char *string, UINT size, const SPELLLOG &log) {
  const SpellRec *spell = g_spellDB.GetRecord(log.spellID);
  SStrPrintf(string, size, "%s hit for %d.", spell ? spell->m_name_lang[CURRENT_LANGUAGE] : "Spell", log.dmg.totalDamage);
}

static void Capitalize(char *string) {
  if (string && *string && islower(*string)) {
    *string -= 32;
  }
}

static void NormalHitHandler(COMBATMESSAGEPRONOUNS &pronouns, const ATTACKROUNDINFO &info, char *buffer, UINT size) {
  FATALASSERT(buffer);
  FATALASSERT(size);
  const ResistancesRec *damageClass = GetDamageClassRecord(info.dmg.damageType[0]);
  LPCSTR                damageType = damageClass ? damageClass->m_name_lang[CURRENT_LANGUAGE] : "";

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
  int totalDamage = (info.flags & 0x4000) ? 0 : info.dmg.totalDamage;
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

static void WriteMessage(LPCSTR message) {
  if (message && *message && (s_flags & 2) && s_logHandle) {
    SLogWrite(s_logHandle, "%s", message);
  }
}

void UnitCombatDebugLogEnable(int enable);

static int CCommand_PlayerCombatLogDebug(LPCSTR, LPCSTR arguments) {
  UnitCombatDebugLogEnable(arguments && SStrToInt(arguments));
  return 1;
}

static int DebugCombatLogHandler(LPCSTR command, LPCSTR arguments) {
  UnitCombatDebugLogEnable(arguments && SStrToInt(arguments));
  return 1;
}

static void GeneratePronouns(COMBATMESSAGEPRONOUNS &pronouns, const ATTACKROUNDINFO &info) {
  CGObject_C *attacker = ClntObjMgrObjectPtr(info.attacker, __FILE__, __LINE__);
  CGObject_C *victim = ClntObjMgrObjectPtr(info.victim, __FILE__, __LINE__);
  SStrCopy(
      pronouns.attackerName, attacker && (attacker->GetType() & TYPE_UNIT) ? static_cast<CGUnit_C *>(attacker)->GetUnitName() : "",
      sizeof(pronouns.attackerName)
  );
  SStrCopy(
      pronouns.victimName, victim && (victim->GetType() & TYPE_UNIT) ? static_cast<CGUnit_C *>(victim)->GetUnitName() : "",
      sizeof(pronouns.victimName)
  );
}

static COMBATMESSAGETYPE DetermineResultType(const ATTACKROUNDINFO &info) {
  switch (info.newVictimState) {
    case VS_BLOCK:
      return COMBATMESSAGETYPE_NORMALBLOCK;
    case VS_PARRY:
      return COMBATMESSAGETYPE_NORMALPARRY;
    case VS_DODGE:
      return COMBATMESSAGETYPE_NORMALDODGE;
    case VS_EVADE:
      return COMBATMESSAGETYPE_NORMALEVADE;
    case VS_IMMUNE:
      return COMBATMESSAGETYPE_NORMALIMMUNE;
    default:
      return info.dmg.totalDamage ? COMBATMESSAGETYPE_NORMALHIT : COMBATMESSAGETYPE_NORMALMISS;
  }
}

static void OutputCombatMessage(const ATTACKROUNDINFO &info) {
  COMBATMESSAGEPRONOUNS pronouns;
  GeneratePronouns(pronouns, info);
  char buffer[256];
  switch (DetermineResultType(info)) {
    case COMBATMESSAGETYPE_NORMALHIT:
      NormalHitHandler(pronouns, info, buffer, sizeof(buffer));
      break;
    case COMBATMESSAGETYPE_NORMALBLOCK:
      NormalBlockHandler(pronouns, info, buffer, sizeof(buffer));
      break;
    case COMBATMESSAGETYPE_NORMALPARRY:
      NormalParryHandler(pronouns, info, buffer, sizeof(buffer));
      break;
    case COMBATMESSAGETYPE_NORMALDODGE:
      NormalDodgeHandler(pronouns, info, buffer, sizeof(buffer));
      break;
    case COMBATMESSAGETYPE_NORMALEVADE:
      NormalEvadeHandler(pronouns, info, buffer, sizeof(buffer));
      break;
    case COMBATMESSAGETYPE_NORMALIMMUNE:
      NormalImmuneHandler(pronouns, info, buffer, sizeof(buffer));
      break;
    default:
      NormalMissHandler(pronouns, info, buffer, sizeof(buffer));
      break;
  }
  ConsoleWrite(buffer, DEFAULT_COLOR);
  WriteMessage(buffer);
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
  float critRate = unit.spellCritsAttempted ? static_cast<float>(unit.spellCritsSucceeded) / unit.spellCritsAttempted * 100.0f : 0.0f;
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
  float critRate = attacker.totalAttemptsByEntity ? static_cast<float>(attacker.criticalHits) / attacker.totalAttemptsByEntity * 100.0f : 0.0f;
  WriteString(1, s_charArray, "%d/%d crits/attempts, %02f%% crit rate\r\n", attacker.criticalHits, attacker.totalHits, critRate);
}

static void WriteVictimStates(const COMBATLOGDESC &victim, LPCSTR name, UINT attempts, UINT successes) {
  WriteString(1, s_charArray, "%s %s Attempts/Success/Failure: %d/%d/%d\r\n", victim.m_name, name, attempts, successes, attempts - successes);
  float successRate = attempts ? static_cast<float>(successes) * 100.0f / attempts : 0.0f;
  WriteString(1, s_charArray, "%s Percentage %s successes: %g%%\r\n", victim.m_name, name, successRate);
}

static float RoundTo(float roundThis, float toThis) {
  if (toThis == 0.0f) {
    return roundThis;
  }

  float negate = 1.0f;
  if (roundThis < 0.0f) {
    negate = -1.0f;
    roundThis = -roundThis;
  }
  if (toThis < 0.0f) {
    toThis = -toThis;
  }

  UINT count = static_cast<UINT>(roundThis / toThis);
  if (fmod(roundThis, toThis) >= toThis * 0.5f) {
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
  UINT grossDamage = desc.totalDamageDoneByEntity + desc.totalDamageReducedByVictim;
  WriteString(1, s_charArray, "  Gross damage suffered by victim: %d (%g per second)\r\n", grossDamage, RoundTo(grossDamage * perSecond, 0.3f));
  WriteString(
      1, s_charArray, "  Net damage suffered by victim: %d (%g per second)\r\n", desc.totalDamageDoneByEntity,
      RoundTo(desc.totalDamageDoneByEntity * perSecond, 0.3f)
  );
  UINT percent = grossDamage ? 100 * desc.totalDamageReducedByVictim / grossDamage : 0;
  WriteString(1, s_charArray, "  Percent damage reduction: %d\r\n", percent);
}

static void LogResults() {
  s_charArray.SetCount(0);
  UINT currentTime = OsGetAsyncTimeMs();
  UINT elapsedTime = s_lastLogTime - s_logStartTime;
  if (s_lastLogTime == s_logStartTime) {
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

static void UnitCombatLogEnchantmentRemoved(const ENCHANTMENTLOG &log, bool isCallback) {
  ENCHANTMENTLOG copy(log);
  copy.flags |= 1;
  DWORDLONG noGuid = 0;
  if (isCallback || g_itemDBCache.GetRecord(copy.itemID, noGuid, ItemEnchantmentCacheCallback, 0)) {
    UnitCombatLogEnchantment(copy);
  } else {
    LogEnchantmentRequest(copy);
  }
}

static void UnitCombatLogEnchantmentAdded(const ENCHANTMENTLOG &log, bool isCallback) {
  ENCHANTMENTLOG copy(log);
  copy.flags &= ~1;
  DWORDLONG noGuid = 0;
  if (isCallback || g_itemDBCache.GetRecord(copy.itemID, noGuid, ItemEnchantmentCacheCallback, 0)) {
    UnitCombatLogEnchantment(copy);
  } else {
    LogEnchantmentRequest(copy);
  }
}

static void ItemEnchantmentCacheCallback(int id, const DWORDLONG &, LPVOID, bool) {
  DWORDLONG noGuid = 0;
  if (!g_itemDBCache.GetRecord(id, noGuid, 0, 0)) {
    return;
  }

  for (UINT index = s_logDesc.Count(); index;) {
    ENCHANTMENTLOGDESC &desc = s_logDesc[--index];
    if (!desc.valid || desc.log.itemID != id) {
      continue;
    }

    if (desc.log.flags & 1) {
      UnitCombatLogEnchantmentRemoved(desc.log, true);
    } else {
      UnitCombatLogEnchantmentAdded(desc.log, true);
    }
    desc.valid = false;
  }
}

static void ClearUnitDataStructs() {
  for (UINT i = 0; i < AFFILIATION_NUMAFFILIATIONS; ++i) {
    s_unitCombatData[i].Clear();
  }
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
  UnitCombatLogEnableFileLog(!s_logHandle);
  return 0;
}

void UnitCombatLogInitialize() {
  ConsoleCommandRegister("playercombatlogdebug", CCommand_PlayerCombatLogDebug, GAME, "Enables logging of combat");
  FrameScript_RegisterFunction("ToggleCombatLogFileWrite", Script_ToggleCombatLogFileWrite);
  CVar::Register("CombatLogPartyRange", 0, 0, "0", 0, DEFAULT, false, 0);
  CVar::Register("CombatLogRange", 0, 0, "40", 0, DEFAULT, false, 0);
  CVar::Register("CombatDeathLogRange", 0, 0, "60", 0, DEFAULT, false, 0);
  CVar::Register("CombatLogPeriodicSpells", 0, 0, "0", 0, DEFAULT, false, 0);
}

void UnitCombatLogShutdown() {
  ConsoleCommandUnregister("playercombatlogdebug");
  s_flags = 0;
  CloseDebugLogHandle();
  FrameScript_UnregisterFunction("ToggleCombatLogFileWrite");
}

void UnitCombatDebugLogEnable(int enable) {
  CDataStore msg;
  msg.Put(static_cast<int>(CMSG_ENABLEDEBUGCOMBATLOGGING));
  msg.Put(enable);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void UnitCombatLogCastGo(UINT spellID, DWORDLONG casterUnit, DWORDLONG target) {
  if (!s_activePlayer) {
    return;
  }
  const SpellRec *rec = g_spellDB.GetRecord(spellID);
  if (!rec || IsSpellQuiet(rec)) {
    return;
  }
  if (IsSpellTeach(rec)) {
    UnitCombatLogSpellTeach(rec, casterUnit, target);
    return;
  }
  CGObject_C *caster = ClntObjMgrObjectPtr(casterUnit, __FILE__, __LINE__);
  CGObject_C *victim = ClntObjMgrObjectPtr(target, __FILE__, __LINE__);
  if (!caster || !(caster->GetType() & TYPE_UNIT)) {
    return;
  }
  LPCSTR casterName = static_cast<CGUnit_C *>(caster)->GetUnitName();
  LPCSTR spellName = rec->m_name_lang[CURRENT_LANGUAGE];
  if (victim && (victim->GetType() & TYPE_UNIT)) {
    GeneralLogPrintf(SLASH_CMD_COMBAT_LOG_SELF, "%s casts %s on %s.", casterName, spellName, static_cast<CGUnit_C *>(victim)->GetUnitName());
  } else {
    GeneralLogPrintf(SLASH_CMD_COMBAT_LOG_SELF, "%s casts %s.", casterName, spellName);
  }
}

void UnitCombatLogCastStart(UINT spellID, DWORDLONG caster) {
  if (!s_activePlayer) {
    return;
  }

  CGObject_C     *casterObjPtr = ClntObjMgrObjectPtr(caster, __FILE__, __LINE__);
  UNITAFFILIATION aAff;
  if (!ShouldLogAttacker(caster, aAff, casterObjPtr, 0, 0, -1) || !casterObjPtr || !(casterObjPtr->GetType() & TYPE_UNIT)) {
    return;
  }

  const SpellRec *rec = g_spellDB.GetRecord(spellID);
  if (!rec || IsSpellQuiet(rec) || (caster == ClntObjMgrGetActivePlayer() && !IsSpellAura(rec)) || IsSpellTeach(rec)) {
    return;
  }

  LPCSTR casterName = static_cast<CGUnit_C *>(casterObjPtr)->GetUnitName();
  LPCSTR spellName = rec->m_name_lang[CURRENT_LANGUAGE];
  UINT   selfCasting = caster == ClntObjMgrGetActivePlayer();
  LPCSTR templateTag;
  if (selfCasting) {
    templateTag = IsSpellAbility(rec) ? "SPELLPERFORMSELFSTART" : "SPELLCASTSELFSTART";
  } else {
    templateTag = IsSpellAbility(rec) ? "SPELLPERFORMOTHERSTART" : "SPELLCASTOTHERSTART";
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

  CGObject_C     *attackerObjPtr;
  CGObject_C     *victimObjPtr;
  UNITAFFILIATION aAff;
  UNITAFFILIATION vAff;
  if (!ShouldLogAttacker(log.attacker, aAff, attackerObjPtr, 0, 0, -1) || !ShouldLogAttacker(log.victim, vAff, victimObjPtr, 0, 0, -1) ||
      !(attackerObjPtr->GetType() & TYPE_UNIT) || !(victimObjPtr->GetType() & TYPE_UNIT))
  {
    return;
  }

  const SpellRec *spellRec = g_spellDB.GetRecord(log.spellID);
  LPCSTR          spellName = spellRec ? spellRec->m_name_lang[CURRENT_LANGUAGE] : "Unknown Spell";
  LPCSTR          attackerName = static_cast<CGUnit_C *>(attackerObjPtr)->GetUnitName();
  LPCSTR          victimName = static_cast<CGUnit_C *>(victimObjPtr)->GetUnitName();
  if ((log.flags & 0x20) && (aAff != AFFILIATION_OTHER || vAff != AFFILIATION_OTHER)) {
    s_unitCombatData[vAff].LogVictim(log);
    s_unitCombatData[aAff].LogAttack(log);
  }
  char outputString[512];
  SStrPrintf(outputString, sizeof(outputString), "%s's %s hits %s for %d.", attackerName, spellName, victimName, log.dmg.totalDamage);
  GeneralLogPrintf(s_affiliationLogType[aAff], "%s", outputString);
  WriteMessage(outputString);
}

void UnitCombatLog(const SPELLMISSLOG &log) {
  if ((s_flags & 2) && (log.flags & 8)) {
    UnitCombatLogSpellMissed(log.reason, log.spellID, log.attacker, log.victim);
  }
}

void UnitCombatLog(const MIRRORTIMERDAMAGE &log) {
  if (static_cast<UINT>(log.damage) > 2 || !log.amount || !log.victim) {
    return;
  }

  CGObject_C     *objPtr;
  UNITAFFILIATION aff;
  if (!ShouldLogAttacker(log.victim, aff, objPtr, 1, 0, -1) || !(objPtr->GetType() & TYPE_UNIT)) {
    return;
  }

  UINT   other = aff != AFFILIATION_YOURSELF;
  LPCSTR templateTag;
  if (log.damage == UNIT_MIRROR_TIMER_EXHAUSTION) {
    templateTag = other ? "VSENVEXHAUSTIONOTHER" : "VSENVEXHAUSTIONSELF";
  } else if (log.damage == UNIT_MIRROR_TIMER_BREATH) {
    templateTag = other ? "VSENVBREATHOTHER" : "VSENVBREATHSELF";
  } else {
    return;
  }

  LPCSTR format = FrameScript_GetText(templateTag, -1, GENDER_NOT_APPLICABLE);
  if (!format || !*format) {
    ReportError(templateTag);
    return;
  }
  if (other) {
    GeneralLogPrintf(s_affiliationLogType[aff], format, static_cast<CGUnit_C *>(objPtr)->GetUnitName(), log.amount);
  } else {
    GeneralLogPrintf(s_affiliationLogType[aff], format, log.amount);
  }
}

void UnitCombatLog(const ENVIRONMENTALDAMAGE &log) {
  if (!log.victim || !log.amount) {
    return;
  }

  CGObject_C     *objPtr;
  UNITAFFILIATION aff;
  if (!ShouldLogAttacker(log.victim, aff, objPtr, 1, 0, -1) || !(objPtr->GetType() & TYPE_UNIT)) {
    return;
  }

  char buffer[64];
  SStrPrintf(buffer, sizeof(buffer), "VSENVIRONMENTALDAMAGE_%d_%s", log.school, aff == AFFILIATION_YOURSELF ? "SELF" : "OTHER");
  LPCSTR format = FrameScript_GetText(buffer, -1, GENDER_NOT_APPLICABLE);
  if (!format || !*format) {
    ReportError(buffer);
    return;
  }

  if (aff == AFFILIATION_YOURSELF) {
    GeneralLogPrintf(s_affiliationLogType[aff], format, log.amount);
  } else {
    GeneralLogPrintf(s_affiliationLogType[aff], format, static_cast<CGUnit_C *>(objPtr)->GetUnitName(), log.amount);
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
  if (added) {
    if (auraSlot < 32 || auraSlot >= 40) {
      token = aAff ? "AURAADDEDOTHERHELPFUL" : "AURAADDEDSELFHELPFUL";
    } else {
      token = aAff ? "AURAADDEDOTHERHARMFUL" : "AURAADDEDSELFHARMFUL";
    }
  } else {
    token = aAff ? "AURAREMOVEDOTHER" : "AURAREMOVEDSELF";
  }

  LPCSTR format = FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE);
  if (!format || !*format) {
    ReportError(token);
    return;
  }

  LPCSTR spellName = spellRec->m_name_lang[CURRENT_LANGUAGE];
  char   string[128];
  if (!aAff) {
    SStrPrintf(string, sizeof(string), format, spellName);
  } else if (added) {
    SStrPrintf(string, sizeof(string), format, unitPtr->GetUnitName(), spellName);
  } else {
    SStrPrintf(string, sizeof(string), format, spellName, unitPtr->GetUnitName());
  }

  GeneralLogPrintf(s_affiliationLogType[aAff], string);
  if (s_flags & 2) {
    ConsoleWrite(string, DEFAULT_COLOR);
    WriteMessage(string);
  }
}

void UnitCombatLogSpellMissed(UINT missReason, UINT spellID, DWORDLONG caster, DWORDLONG victim) {
  CGObject_C     *attackerObjPtr;
  CGObject_C     *victimObjPtr;
  UNITAFFILIATION aAff;
  UNITAFFILIATION vAff;
  if (missReason >= 10 || !ShouldLogAttacker(caster, aAff, attackerObjPtr, 0, 0, -1) || !ShouldLogAttacker(victim, vAff, victimObjPtr, 0, 0, -1)) {
    return;
  }

  const SpellRec *spell = g_spellDB.GetRecord(spellID);
  CGUnit_C       *attackerPtr = static_cast<CGUnit_C *>(attackerObjPtr);
  CGUnit_C       *victimPtr = static_cast<CGUnit_C *>(victimObjPtr);
  LPCSTR          casterName = attackerPtr->GetUnitName();
  LPCSTR          victimName = victimPtr->GetUnitName();
  LPCSTR          spellName = spell ? spell->m_name_lang[CURRENT_LANGUAGE] : "";

  LPCSTR reasonToken;
  switch (missReason) {
    case 2:
      reasonToken = "SPELLRESIST";
      break;
    case 3:
      reasonToken = "SPELLDODGED";
      break;
    case 4:
      reasonToken = "SPELLPARRIED";
      break;
    case 5:
      reasonToken = "SPELLBLOCKED";
      break;
    case 6:
      reasonToken = "SPELLEVADED";
      break;
    case 7:
      reasonToken = "SPELLIMMUNE";
      break;
    case 8:
      reasonToken = "SPELLDEFLECTED";
      break;
    case 9:
      reasonToken = "SPELLABSORB";
      break;
    default:
      reasonToken = "SPELLMISS";
      break;
  }

  char templateTagBuffer[40];
  SStrPrintf(
      templateTagBuffer, sizeof(templateTagBuffer), "%s%s%s", reasonToken, aAff == AFFILIATION_YOURSELF ? "SELF" : "OTHER",
      vAff == AFFILIATION_YOURSELF ? "SELF" : "OTHER"
  );
  LPCSTR templateTag = templateTagBuffer;
  LPCSTR format = FrameScript_GetText(templateTag, -1, GENDER_NOT_APPLICABLE);

  char output[128];
  if (format && *format) {
    SStrPrintf(output, sizeof(output), format, casterName, spellName, victimName);
  } else {
    SStrPrintf(output, sizeof(output), "%s's %s missed %s.", casterName, spellName, victimName);
  }
  GeneralLogPrintf(s_affiliationLogType[aAff], "%s", output);
  WriteMessage(output);
}

void UnitCombatLogUnitDead(DWORDLONG unit) {
  if (!s_activePlayer) {
    return;
  }

  UNITAFFILIATION aAff;
  CGObject_C     *unitObjPtr;
  if (!ShouldLogAttacker(unit, aAff, unitObjPtr, 0, 1, -1) || (static_cast<CGUnit_C *>(unitObjPtr)->GetUnitData()->flags & 0x80)) {
    return;
  }

  LPCSTR unitName = static_cast<CGUnit_C *>(unitObjPtr)->GetUnitName();
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
    if (!s_logHandle) {
      SLogCreate("Logs.Client\\PlayerCombatLog.txt", 0, &s_logHandle);
    }
  } else {
    if (s_logHandle) {
      SLogClose(s_logHandle);
    }
    s_logHandle = 0;
  }
}

void UnitCombatLogSetActivePlayer(const CGPlayer_C *playerPtr) {
  s_activePlayer = playerPtr;
}

void UnitCombatLogXPGain(const DWORDLONG &victim, CDataStore *msg, UINT count) {
  FATALASSERT(msg);

  for (UINT i = 0; i < count; ++i) {
    DWORDLONG guid;
    int       xp;
    msg->Get(guid);
    msg->Get(xp);
    CGObject_C *playerPtr = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
    if (!playerPtr || !xp) {
      break;
    }
    FATALASSERT(playerPtr->GetType() & TYPE_PLAYER);
    if (playerPtr->GetGUID() == ClntObjMgrGetActivePlayer()) {
      CGObject_C *victimPtr = ClntObjMgrObjectPtr(victim, __FILE__, __LINE__);
      if (victimPtr && (victimPtr->GetType() & TYPE_UNIT)) {
        UnitCombatLogShowXPGained(victim, xp);
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

  CGObject_C     *dummy;
  UNITAFFILIATION aAff;
  DWORDLONG       casterGUID = caster->GetGUID();
  if (!ShouldLogAttacker(casterGUID, aAff, dummy, 0, 0, -1)) {
    return;
  }

  const SpellRec *spellRec = g_spellDB.GetRecord(spellID);
  if (!spellRec || IsSpellQuiet(spellRec)) {
    return;
  }

  LPCSTR casterName = caster->GetUnitName();
  UINT   selfCasting = casterGUID == ClntObjMgrGetActivePlayer();
  LPCSTR templateTag;
  if (IsSpellAbility(spellRec)) {
    templateTag = selfCasting ? "SPELLFAILPERFORMSELF" : "SPELLFAILPERFORMOTHER";
  } else {
    templateTag = selfCasting ? "SPELLFAILCASTSELF" : "SPELLFAILCASTOTHER";
  }

  LPCSTR format = FrameScript_GetText(templateTag, -1, GENDER_NOT_APPLICABLE);
  if (!format || !*format) {
    ReportError(templateTag);
    return;
  }

  LPCSTR spellName = spellRec->m_name_lang[CURRENT_LANGUAGE];
  char   output[256];
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
  UNITAFFILIATION aAff;
  UNITAFFILIATION vAff;
  if (!ShouldLogAttacker(log.attacker, aAff, attackerObjPtr, 0, 0, -1) || !ShouldLogAttacker(log.victim, vAff, victimObjPtr, 0, 0, -1) ||
      !(attackerObjPtr->GetType() & TYPE_UNIT) || !(victimObjPtr->GetType() & TYPE_UNIT))
  {
    return;
  }

  const SpellRec *spellRec = g_spellDB.GetRecord(log.spell);
  if (!spellRec || IsSpellQuiet(spellRec)) {
    return;
  }

  CGUnit_C *caster = static_cast<CGUnit_C *>(attackerObjPtr);
  CGUnit_C *victim = static_cast<CGUnit_C *>(victimObjPtr);
  char      output[256];
  SStrPrintf(
      output, sizeof(output), "%s (castlevel %d) %s the \"%s\" aura of %s (%s) (%g%%/%g%%)", caster->GetUnitName(), log.castLevel,
      (log.flags & 2) ? "resists" : "fails to resist", spellRec->m_name_lang[CURRENT_LANGUAGE], victim->GetUnitName(),
      (log.flags & 1) ? "DAMAGE" : "HEARTBEAT", log.resistRollNeeded, log.resistRoll
  );
  ConsoleWrite(output, DEFAULT_COLOR);
  WriteMessage(output);
}

void UnitCombatLogEnchantment(const ENCHANTMENTLOG &log) {
  if (!s_activePlayer || !log.attacker) {
    return;
  }

  CGObject_C     *attackerObjPtr;
  UNITAFFILIATION aAff;
  if (!ShouldLogAttacker(log.attacker, aAff, attackerObjPtr, 0, 0, -1) || !(attackerObjPtr->GetType() & TYPE_UNIT)) {
    return;
  }

  const SpellItemEnchantmentRec *enchantment = g_spellItemEnchantmentDB.GetRecord(log.enchantment);
  LPCSTR                         enchantmentName = enchantment ? enchantment->m_name_lang[CURRENT_LANGUAGE] : "Unknown Enchantment";
  const ItemStats_C             *item = g_itemDBCache.GetRecord(log.itemID, 0, 0, 0);
  if (!item) {
    return;
  }
  LPCSTR itemName = item->m_displayName[0] ? item->m_displayName[0] : "";
  LPCSTR attackerName = static_cast<CGUnit_C *>(attackerObjPtr)->GetUnitName();

  LPCSTR          templateTag;
  CGObject_C     *victimObjPtr = 0;
  UNITAFFILIATION vAff = AFFILIATION_OTHER;
  if (log.flags & 1) {
    templateTag = aAff == AFFILIATION_YOURSELF ? "ITEMENCHANTMENTREMOVESELF" : "ITEMENCHANTMENTREMOVEOTHER";
  } else {
    if (!ShouldLogAttacker(log.victim, vAff, victimObjPtr, 0, 0, -1) || !(victimObjPtr->GetType() & TYPE_UNIT)) {
      return;
    }
    if (aAff == AFFILIATION_YOURSELF) {
      templateTag = vAff == AFFILIATION_YOURSELF ? "ITEMENCHANTMENTADDSELFSELF" : "ITEMENCHANTMENTADDSELFOTHER";
    } else {
      templateTag = vAff == AFFILIATION_YOURSELF ? "ITEMENCHANTMENTADDOTHERSELF" : "ITEMENCHANTMENTADDOTHEROTHER";
    }
  }

  LPCSTR format = FrameScript_GetText(templateTag, -1, GENDER_NOT_APPLICABLE);
  if (!format || !*format) {
    ReportError(templateTag);
    return;
  }

  char   output[256];
  LPCSTR victimName = victimObjPtr ? static_cast<CGUnit_C *>(victimObjPtr)->GetUnitName() : "";
  if (log.flags & 1) {
    if (aAff == AFFILIATION_YOURSELF) {
      SStrPrintf(output, sizeof(output), format, enchantmentName, itemName);
    } else {
      SStrPrintf(output, sizeof(output), format, enchantmentName, attackerName, itemName);
    }
  } else if (aAff == AFFILIATION_YOURSELF) {
    if (vAff == AFFILIATION_YOURSELF) {
      SStrPrintf(output, sizeof(output), format, enchantmentName, itemName);
    } else {
      SStrPrintf(output, sizeof(output), format, enchantmentName, victimName, itemName);
    }
  } else if (vAff == AFFILIATION_YOURSELF) {
    SStrPrintf(output, sizeof(output), format, attackerName, enchantmentName, itemName);
  } else {
    SStrPrintf(output, sizeof(output), format, attackerName, enchantmentName, victimName, itemName);
  }
  GeneralLogPrintf(s_affiliationLogType[aAff], "%s", output);
}

void UnitCombatLogString(LPCSTR buffer) {
  if (buffer && *buffer) {
    GeneralLogPrintf(SLASH_CMD_COMBAT_LOG_ENEMY, "%s", buffer);
  }
}

void UnitCombatLogFactionChanged(int faction, int delta) {
  const FactionRec *rec = g_factionDB.GetRecord(faction);
  if (!rec || !delta) {
    return;
  }

  LPCSTR token = delta < 0 ? "FACTION_STANDING_DECREASED" : "FACTION_STANDING_INCREASED";
  LPCSTR format = FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE);
  if (format) {
    if (delta <= 0) {
      delta = -delta;
    }
    GeneralLogPrintf(SLASH_CMD_COMBAT_LOG_SELF, format, rec->m_name_lang[CURRENT_LANGUAGE], delta);
  } else {
    GeneralLogPrintf(SLASH_CMD_COMBAT_LOG_MISC_INFO, "Error, cannot find string <%s>", token);
  }
}

void UnitCombatLogPartyKill(const PARTYKILLLOG &log) {
  if (!s_activePlayer || !log.killer || log.killer == ClntObjMgrGetActivePlayer()) {
    return;
  }

  CGObject_C     *objPtr;
  UNITAFFILIATION aff;
  if (!ShouldLogAttacker(log.killer, aff, objPtr, 1, 0, -1) || aff != AFFILIATION_PARTYMEMBER || !(objPtr->GetType() & TYPE_UNIT)) {
    return;
  }

  CGObject_C *victimObjPtr = ClntObjMgrObjectPtr(log.victim, __FILE__, __LINE__);
  if (!victimObjPtr || !(victimObjPtr->GetType() & TYPE_UNIT)) {
    return;
  }

  LPCSTR format = FrameScript_GetText("PARTYKILLOTHER", -1, GENDER_NOT_APPLICABLE);
  if (!format) {
    GeneralLogPrintf(SLASH_CMD_COMBAT_LOG_MISC_INFO, "Error, cannot find string <%s>", "PARTYKILLOTHER");
    return;
  }
  GeneralLogPrintf(
      SLASH_CMD_COMBAT_LOG_PARTY, format, static_cast<CGUnit_C *>(victimObjPtr)->GetUnitName(), static_cast<CGUnit_C *>(objPtr)->GetUnitName()
  );
}

void UnitCombatLogShowXPGained(const DWORDLONG &victim, int xp) {
  CGUnit_C *victimPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(victim, __FILE__, __LINE__));
  if (victimPtr && (victimPtr->GetUnitData()->flags & 8)) {
    GeneralLogPrintf(
        SLASH_CMD_COMBAT_LOG_SELF, FrameScript_GetText("COMBATLOG_XPGAIN_FIRSTPERSON", -1, GENDER_NOT_APPLICABLE), victimPtr->GetUnitName(), xp
    );
  }
}
