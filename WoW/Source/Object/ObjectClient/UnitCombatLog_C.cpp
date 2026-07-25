#include "Unit_C.h"

#include <Base/CDataStore.h>

#include "DB/DBClient/AutoCode/SpellRec.h"
#include "DB/DBClient/AutoCode/SpellItemEnchantmentRec.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "DB/DBClient/AutoCode/FactionRec.h"
#include "DB/WowLocale.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Ui/ChatFrame.h"
#include "Ui/GameUI.h"
#include "Console/ConsoleClient.h"
#include "Console/ConsoleCommand.h"
#include "Console/ConsoleVar.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <FrameScript/FrameScript.h>
#include <storm.h>

class CGPlayer_C;
struct COMBATLOGDESC;

struct COMBATMESSAGEPRONOUNS {
  char attackerName[48];
  char victimName[48];
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
  COMBATMESSAGETYPE_UNKNOWN = 255
};

void __fastcall UnitCombatLogEnableFileLog(int enable);
void __fastcall UnitCombatLogShowXPGained(const unsigned __int64 &victim, int xp);
void __fastcall UnitCombatLogEnchantment(ENCHANTMENTLOG &log);

static HSLOG        s_logHandle;
static HSLOG        s_generalLogHandle;
static unsigned int s_flags;
static CGPlayer_C  *s_activePlayer;

static SLASH_COMMAND_ID s_affiliationLogType[AFFILIATION_NUMAFFILIATIONS] = {
    static_cast<SLASH_COMMAND_ID>(26), static_cast<SLASH_COMMAND_ID>(27), static_cast<SLASH_COMMAND_ID>(27), static_cast<SLASH_COMMAND_ID>(25),
    static_cast<SLASH_COMMAND_ID>(27)
};

static float __fastcall GetLogDistance(UNITAFFILIATION aff, unsigned int suppressUnaffiliated) {
  if (aff >= AFFILIATION_NUMAFFILIATIONS || (suppressUnaffiliated && aff == AFFILIATION_OTHER)) {
    return 0.0f;
  }

  const char *cvarName = 0;
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

static unsigned int __fastcall ShouldLogAttacker(
    unsigned __int64 attacker,
    UNITAFFILIATION &aAff,
    CGObject_C     *&unitPtr,
    unsigned int     suppressIfUnaffiliated,
    unsigned int     useDeathRange,
    int              allowedAffiliationFlags
) {
  CGUnit_C *activePlayer = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!activePlayer) {
    return 0;
  }

  unitPtr = ClntObjMgrObjectPtr(attacker, __FILE__, __LINE__);
  if (!unitPtr) {
    return 0;
  }

  if (unitPtr->GetGUID() == activePlayer->GetGUID()) {
    aAff = AFFILIATION_YOURSELF;
  } else if (unitPtr->GetType() & TYPE_UNIT) {
    const CGUnitData *data = static_cast<const CGUnit_C *>(unitPtr)->GetUnitData();
    if (data->summonedBy == activePlayer->GetGUID() || data->createdBy == activePlayer->GetGUID() || data->charmedBy == activePlayer->GetGUID()) {
      aAff = AFFILIATION_YOURPET;
    } else if (activePlayer->GetUnitData()->charmedBy == unitPtr->GetGUID()) {
      aAff = AFFILIATION_YOURCONTROLLER;
    } else if (CGGameUI::IsPartyMember(unitPtr->GetGUID())) {
      aAff = AFFILIATION_PARTYMEMBER;
    } else {
      aAff = AFFILIATION_OTHER;
    }
  } else {
    aAff = CGGameUI::IsPartyMember(unitPtr->GetGUID()) ? AFFILIATION_PARTYMEMBER : AFFILIATION_OTHER;
  }
  if (!(allowedAffiliationFlags & (1 << aAff))) {
    return 0;
  }

  float dist;
  if (useDeathRange) {
    CVar *cvar = CVar::Lookup("CombatDeathLogRange");
    dist = cvar ? cvar->GetFloat() : GetLogDistance(aAff, suppressIfUnaffiliated);
  } else {
    dist = GetLogDistance(aAff, suppressIfUnaffiliated);
  }

  NTempest::C3Vector unitPosition = unitPtr->GetPosition();
  NTempest::C3Vector playerPosition = activePlayer->GetPosition();
  float              x = playerPosition.x - unitPosition.x;
  float              y = playerPosition.y - unitPosition.y;
  float              z = playerPosition.z - unitPosition.z;
  return x * x + y * y + z * z <= dist * dist;
}

static int ShouldLog(unsigned __int64 object, UNITAFFILIATION& aAff, CGObject_C*& objectPtr, CGUnit_C*& attackerPtr, unsigned __int64 subject, UNITAFFILIATION& vAff, CGObject_C*& subjectPtr, CGUnit_C*& victimPtr, unsigned char suppressIfAllUnaffiliated) {
  if (!ShouldLogAttacker(object, aAff, objectPtr, 0, 0, -1) ||
      !ShouldLogAttacker(subject, vAff, subjectPtr, 0, 0, -1) ||
      !(objectPtr->GetType() & TYPE_UNIT) || !(subjectPtr->GetType() & TYPE_UNIT)) {
    return 0;
  }
  if (suppressIfAllUnaffiliated && aAff == AFFILIATION_OTHER && vAff == AFFILIATION_OTHER) {
    return 0;
  }
  attackerPtr = static_cast<CGUnit_C *>(objectPtr);
  victimPtr = static_cast<CGUnit_C *>(subjectPtr);
  return 1;
}

static unsigned int __fastcall IsSpellTeach(SpellRec *rec) {
  for (unsigned int effect = 0; effect < 3; ++effect) {
    if (rec->m_effect[effect] == 36) {
      return 1;
    }
  }
  return 0;
}

static unsigned int __fastcall IsSpellAbility(SpellRec *rec) {
  return (rec->m_attributes >> 4) & 1;
}

static unsigned char IsSpellHarmful(const SpellRec* rec) {
  for (unsigned int effect = 0; effect < 3; ++effect) {
    if (rec->m_effect[effect] == 2 ||
        rec->m_effect[effect] == 58 ||
        rec->m_effect[effect] == 17 ||
        rec->m_effect[effect] == 31 ||
        rec->m_effect[effect] == 62) {
      return 1;
    }
  }
  return 0;
}

static unsigned char IsSpellOpenLock(const SpellRec* rec) {
  for (unsigned int effect = 0; effect < 3; ++effect) {
    if (rec->m_effect[effect] == 33 || rec->m_effect[effect] == 59) {
      return 1;
    }
  }
  return 0;
}

static unsigned int __fastcall IsSpellQuiet(SpellRec *rec) {
  return (rec->m_attributes >> 7) & 1;
}

bool __fastcall IsSpellAura(const SpellRec *rec);
void __fastcall UnitCombatLogSpellMissed(unsigned int missReason, unsigned int spellID, unsigned __int64 caster, unsigned __int64 victim);
static void __fastcall ItemEnchantmentCacheCallback(int id, const unsigned __int64 &guid, void *arg, bool granted);

static void LogEnchantmentRequest(const ENCHANTMENTLOG& log) {
  ENCHANTMENTLOG *copy = NEW(ENCHANTMENTLOG)(log);
  if (g_itemDBCache.GetRecord(log.itemID, log.attacker, ItemEnchantmentCacheCallback, copy)) {
    UnitCombatLogEnchantment(*copy);
    DEL(copy);
  }
}

static void CloseDebugLogHandle() {
  if (s_logHandle) {
    SLogClose(s_logHandle);
  }
  s_logHandle = 0;
}

static void __cdecl GeneralLogPrintf(SLASH_COMMAND_ID type, const char *format, ...) {
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

static void __fastcall ReportError(const char *string) {
  if (string && *string) {
    GeneralLogPrintf(static_cast<SLASH_COMMAND_ID>(28), "Warning, string %s not found in stringfile.", string);
  }
}

static void UnitCombatLogSpellTeach(const SpellRec* rec, unsigned __int64 caster, unsigned __int64 target) {
  CGObject_C *casterObject = ClntObjMgrObjectPtr(caster, __FILE__, __LINE__);
  CGObject_C *targetObject = ClntObjMgrObjectPtr(target, __FILE__, __LINE__);
  if (!rec || !casterObject || !targetObject ||
      !(casterObject->GetType() & TYPE_UNIT) || !(targetObject->GetType() & TYPE_UNIT)) {
    return;
  }
  GeneralLogPrintf(
      static_cast<SLASH_COMMAND_ID>(26), "%s teaches %s to %s.",
      static_cast<CGUnit_C *>(casterObject)->GetUnitName(),
      rec->m_name_lang[CURRENT_LANGUAGE],
      static_cast<CGUnit_C *>(targetObject)->GetUnitName()
  );
}

static void HandleTerseVictimLogging(unsigned __int64 attacker, unsigned __int64 victim, unsigned int spellID) {
  CGObject_C *attackerObject = ClntObjMgrObjectPtr(attacker, __FILE__, __LINE__);
  CGObject_C *victimObject = ClntObjMgrObjectPtr(victim, __FILE__, __LINE__);
  SpellRec *spell = g_spellDB.GetRecord(spellID);
  if (attackerObject && victimObject && spell &&
      (attackerObject->GetType() & TYPE_UNIT) && (victimObject->GetType() & TYPE_UNIT)) {
    GeneralLogPrintf(
        static_cast<SLASH_COMMAND_ID>(27), "%s's %s affects %s.",
        static_cast<CGUnit_C *>(attackerObject)->GetUnitName(),
        spell->m_name_lang[CURRENT_LANGUAGE],
        static_cast<CGUnit_C *>(victimObject)->GetUnitName()
    );
  }
}

static void HandleGeneralHealLogging(const DamageData& dmg, unsigned int spellID, unsigned __int64 attacker, unsigned __int64 victim) {
  CGObject_C *attackerObject = ClntObjMgrObjectPtr(attacker, __FILE__, __LINE__);
  CGObject_C *victimObject = ClntObjMgrObjectPtr(victim, __FILE__, __LINE__);
  SpellRec *spell = g_spellDB.GetRecord(spellID);
  if (attackerObject && victimObject && spell &&
      (attackerObject->GetType() & TYPE_UNIT) && (victimObject->GetType() & TYPE_UNIT)) {
    GeneralLogPrintf(
        static_cast<SLASH_COMMAND_ID>(26), "%s's %s heals %s for %d.",
        static_cast<CGUnit_C *>(attackerObject)->GetUnitName(),
        spell->m_name_lang[CURRENT_LANGUAGE],
        static_cast<CGUnit_C *>(victimObject)->GetUnitName(),
        -dmg.totalDamage
    );
  }
}

static void HandleGeneralCombatOrSpellHitLogging(int combat, const DamageData& dmg, unsigned int spellID, CGUnit_C* attackerPtr, CGUnit_C* victimPtr, int critted, unsigned int specialSpellID, unsigned int specialSpellDamage, UNITAFFILIATION aAff, UNITAFFILIATION vAff) {
  if (!attackerPtr || !victimPtr) {
    return;
  }
  SpellRec *spell = spellID ? g_spellDB.GetRecord(spellID) : 0;
  const char *spellName = spell ? spell->m_name_lang[CURRENT_LANGUAGE] : 0;
  if (spellName) {
    GeneralLogPrintf(s_affiliationLogType[aAff], "%s's %s hits %s for %d%s.",
        attackerPtr->GetUnitName(), spellName, victimPtr->GetUnitName(), dmg.totalDamage,
        critted ? " (critical)" : "");
  } else {
    GeneralLogPrintf(s_affiliationLogType[aAff], "%s hits %s for %d%s.",
        attackerPtr->GetUnitName(), victimPtr->GetUnitName(), dmg.totalDamage,
        critted ? " (critical)" : "");
  }
}

static void HandleSpellLogTerse(CGUnit_C* attackerPtr, UNITAFFILIATION aAff, const char* spellNameString) {
  if (attackerPtr && spellNameString) {
    GeneralLogPrintf(s_affiliationLogType[aAff], "%s casts %s.", attackerPtr->GetUnitName(), spellNameString);
  }
}

static void HandleGeneralCombatLoggingMissed(const ATTACKROUNDINFO& info, CGUnit_C* attackerPtr, CGUnit_C* victimPtr, UNITAFFILIATION aAff, UNITAFFILIATION vAff) {
  if (attackerPtr && victimPtr) {
    GeneralLogPrintf(s_affiliationLogType[aAff], "%s misses %s.", attackerPtr->GetUnitName(), victimPtr->GetUnitName());
  }
}

static void HandleGeneralCombatEvadeLogging(ATTACKROUNDINFO info, CGUnit_C* attackerPtr, CGUnit_C* victimPtr, UNITAFFILIATION aAff, UNITAFFILIATION vAff) {
  if (attackerPtr && victimPtr) {
    GeneralLogPrintf(s_affiliationLogType[aAff], "%s's attack was evaded by %s.", attackerPtr->GetUnitName(), victimPtr->GetUnitName());
  }
}

static void HandleGeneralCombatLogging(const ATTACKROUNDINFO& info) {
  CGObject_C *attackerObject;
  CGObject_C *victimObject;
  CGUnit_C *attackerPtr;
  CGUnit_C *victimPtr;
  UNITAFFILIATION aAff;
  UNITAFFILIATION vAff;
  if (!ShouldLog(info.attacker, aAff, attackerObject, attackerPtr, info.victim, vAff, victimObject, victimPtr, 0)) {
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

static void FormatSpellMissString(char* string, unsigned int size, const SPELLMISSLOG& log) {
  SpellRec *spell = g_spellDB.GetRecord(log.spellID);
  SStrPrintf(string, size, "%s missed (reason %u).", spell ? spell->m_name_lang[CURRENT_LANGUAGE] : "Spell", log.reason);
}

static void FormatSpellString(char* string, unsigned int size, const SPELLLOG& log) {
  SpellRec *spell = g_spellDB.GetRecord(log.spellID);
  SStrPrintf(string, size, "%s hit for %d.", spell ? spell->m_name_lang[CURRENT_LANGUAGE] : "Spell", log.dmg.totalDamage);
}

static void Capitalize(char* string) {
  if (string && *string && islower(*string)) {
    *string -= 32;
  }
}

static void NormalHitHandler(COMBATMESSAGEPRONOUNS& pronouns, const ATTACKROUNDINFO& info, char* buffer, unsigned int size) {
  SStrPrintf(buffer, size, "%s hits %s for %d.", pronouns.attackerName, pronouns.victimName, info.dmg.totalDamage);
}

static void NormalMissHandler(COMBATMESSAGEPRONOUNS& pronouns, const ATTACKROUNDINFO& info, char* buffer, unsigned int size) {
  SStrPrintf(buffer, size, "%s misses %s.", pronouns.attackerName, pronouns.victimName);
}

static void NormalBlockHandler(COMBATMESSAGEPRONOUNS& pronouns, const ATTACKROUNDINFO& info, char* buffer, unsigned int size) {
  SStrPrintf(buffer, size, "%s blocks %s's attack.", pronouns.victimName, pronouns.attackerName);
}

static void NormalParryHandler(COMBATMESSAGEPRONOUNS& pronouns, const ATTACKROUNDINFO& info, char* buffer, unsigned int size) {
  SStrPrintf(buffer, size, "%s parries %s's attack.", pronouns.victimName, pronouns.attackerName);
}

static void NormalDodgeHandler(COMBATMESSAGEPRONOUNS& pronouns, const ATTACKROUNDINFO& info, char* buffer, unsigned int size) {
  SStrPrintf(buffer, size, "%s dodges %s's attack.", pronouns.victimName, pronouns.attackerName);
}

static void NormalImmuneHandler(COMBATMESSAGEPRONOUNS& pronouns, const ATTACKROUNDINFO& info, char* buffer, unsigned int size) {
  SStrPrintf(buffer, size, "%s is immune to %s's attack.", pronouns.victimName, pronouns.attackerName);
}

static void NormalEvadeHandler(COMBATMESSAGEPRONOUNS& pronouns, const ATTACKROUNDINFO& info, char* buffer, unsigned int size) {
  SStrPrintf(buffer, size, "%s evades %s's attack.", pronouns.victimName, pronouns.attackerName);
}

static void __fastcall WriteMessage(const char *message) {
  if (message && *message && (s_flags & 2) && s_logHandle) {
    SLogWrite(s_logHandle, "%s", message);
  }
}

void __fastcall UnitCombatDebugLogEnable(int enable);

static int __fastcall CCommand_PlayerCombatLogDebug(const char *, const char *arguments) {
  UnitCombatDebugLogEnable(arguments && SStrToInt(arguments));
  return 1;
}

static int DebugCombatLogHandler(const char* command, const char* arguments) {
  UnitCombatDebugLogEnable(arguments && SStrToInt(arguments));
  return 1;
}

static void GeneratePronouns(COMBATMESSAGEPRONOUNS& pronouns, const ATTACKROUNDINFO& info) {
  CGObject_C *attacker = ClntObjMgrObjectPtr(info.attacker, __FILE__, __LINE__);
  CGObject_C *victim = ClntObjMgrObjectPtr(info.victim, __FILE__, __LINE__);
  SStrCopy(pronouns.attackerName,
      attacker && (attacker->GetType() & TYPE_UNIT) ? static_cast<CGUnit_C *>(attacker)->GetUnitName() : "",
      sizeof(pronouns.attackerName));
  SStrCopy(pronouns.victimName,
      victim && (victim->GetType() & TYPE_UNIT) ? static_cast<CGUnit_C *>(victim)->GetUnitName() : "",
      sizeof(pronouns.victimName));
}

static COMBATMESSAGETYPE DetermineResultType(const ATTACKROUNDINFO& info) {
  switch (info.newVictimState) {
    case VS_BLOCK: return COMBATMESSAGETYPE_NORMALBLOCK;
    case VS_PARRY: return COMBATMESSAGETYPE_NORMALPARRY;
    case VS_DODGE: return COMBATMESSAGETYPE_NORMALDODGE;
    case VS_EVADE: return COMBATMESSAGETYPE_NORMALEVADE;
    case VS_IMMUNE: return COMBATMESSAGETYPE_NORMALIMMUNE;
    default: return info.dmg.totalDamage ? COMBATMESSAGETYPE_NORMALHIT : COMBATMESSAGETYPE_NORMALMISS;
  }
}

static void OutputCombatMessage(const ATTACKROUNDINFO& info) {
  COMBATMESSAGEPRONOUNS pronouns;
  GeneratePronouns(pronouns, info);
  char buffer[256];
  switch (DetermineResultType(info)) {
    case COMBATMESSAGETYPE_NORMALHIT: NormalHitHandler(pronouns, info, buffer, sizeof(buffer)); break;
    case COMBATMESSAGETYPE_NORMALBLOCK: NormalBlockHandler(pronouns, info, buffer, sizeof(buffer)); break;
    case COMBATMESSAGETYPE_NORMALPARRY: NormalParryHandler(pronouns, info, buffer, sizeof(buffer)); break;
    case COMBATMESSAGETYPE_NORMALDODGE: NormalDodgeHandler(pronouns, info, buffer, sizeof(buffer)); break;
    case COMBATMESSAGETYPE_NORMALEVADE: NormalEvadeHandler(pronouns, info, buffer, sizeof(buffer)); break;
    case COMBATMESSAGETYPE_NORMALIMMUNE: NormalImmuneHandler(pronouns, info, buffer, sizeof(buffer)); break;
    default: NormalMissHandler(pronouns, info, buffer, sizeof(buffer)); break;
  }
  ConsoleWrite(buffer, DEFAULT_COLOR);
  WriteMessage(buffer);
}

static void WriteSpellInfo(const COMBATLOGDESC& unit) {
}

static void WriteAttemptsHitsMisses(const COMBATLOGDESC& attacker) {
}

static void WriteVictimStates(const COMBATLOGDESC& victim, const char* name, unsigned int attempts, unsigned int successes) {
}

static float RoundTo(float roundThis, float toThis) {
  if (toThis == 0.0f) {
    return roundThis;
  }

  float sign = 1.0f;
  if (roundThis < 0.0f) {
    sign = -1.0f;
    roundThis = -roundThis;
  }
  if (toThis < 0.0f) {
    toThis = -toThis;
  }

  unsigned int count = static_cast<unsigned int>(roundThis / toThis);
  if (fmod(roundThis, toThis) >= toThis * 0.5f) {
    ++count;
  }
  return count * sign * toThis;
}

static void WriteDamageTallies(const COMBATLOGDESC& desc, float seconds) {
}

static void LogResults() {
  if (s_logHandle) {
    SLogWrite(s_logHandle, "Combat logging stopped.");
  }
}

static void UnitCombatLogEnchantmentRemoved(const ENCHANTMENTLOG& log, unsigned char isCallback) {
  ENCHANTMENTLOG copy(log);
  copy.flags |= 1;
  if (isCallback || g_itemDBCache.GetRecord(copy.itemID, copy.attacker, 0, 0)) {
    UnitCombatLogEnchantment(copy);
  } else {
    LogEnchantmentRequest(copy);
  }
}

static void UnitCombatLogEnchantmentAdded(const ENCHANTMENTLOG& log, unsigned char isCallback) {
  ENCHANTMENTLOG copy(log);
  copy.flags &= ~1;
  if (isCallback || g_itemDBCache.GetRecord(copy.itemID, copy.attacker, 0, 0)) {
    UnitCombatLogEnchantment(copy);
  } else {
    LogEnchantmentRequest(copy);
  }
}

static void __fastcall ItemEnchantmentCacheCallback(int id, const unsigned __int64& guid, void* arg, bool granted) {
  ENCHANTMENTLOG *log = static_cast<ENCHANTMENTLOG *>(arg);
  if (granted && log) {
    UnitCombatLogEnchantment(*log);
  }
  DEL(log);
}

static void ClearUnitDataStructs() {
}

void __fastcall UnitDebugCombatLogOnEnable(int enable) {
  if (enable) {
    s_flags |= 2;
    if (!s_logHandle) {
      SLogCreate("Logs.Client\\PlayerCombatLog.txt", 0, &s_logHandle);
    }
  } else {
    LogResults();
    s_flags &= ~2U;
    CloseDebugLogHandle();
  }
}

static int __fastcall Script_ToggleCombatLogFileWrite(lua_State *L) {
  UnitCombatLogEnableFileLog(!s_logHandle);
  return 0;
}

void __fastcall UnitCombatLogInitialize() {
  ConsoleCommandRegister("playercombatlogdebug", CCommand_PlayerCombatLogDebug, GAME, "Enables logging of combat");
  FrameScript_RegisterFunction("ToggleCombatLogFileWrite", Script_ToggleCombatLogFileWrite);
  CVar::Register("CombatLogPartyRange", 0, 0, "0", 0, DEFAULT, false, 0);
  CVar::Register("CombatLogRange", 0, 0, "40", 0, DEFAULT, false, 0);
  CVar::Register("CombatDeathLogRange", 0, 0, "60", 0, DEFAULT, false, 0);
  CVar::Register("CombatLogPeriodicSpells", 0, 0, "0", 0, DEFAULT, false, 0);
}

void __fastcall UnitCombatLogShutdown() {
  ConsoleCommandUnregister("playercombatlogdebug");
  s_flags = 0;
  UnitCombatLogEnableFileLog(0);
  FrameScript_UnregisterFunction("ToggleCombatLogFileWrite");
}

void __fastcall UnitCombatDebugLogEnable(int enable) {
  CDataStore msg;
  msg.Put(static_cast<int>(CMSG_ENABLEDEBUGCOMBATLOGGING));
  msg.Put(enable);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void __fastcall UnitCombatLogCastGo(unsigned int spellID, unsigned __int64 casterUnit, unsigned __int64 target) {
  if (!s_activePlayer) {
    return;
  }
  SpellRec *rec = g_spellDB.GetRecord(spellID);
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
  const char *casterName = static_cast<CGUnit_C *>(caster)->GetUnitName();
  const char *spellName = rec->m_name_lang[CURRENT_LANGUAGE];
  if (victim && (victim->GetType() & TYPE_UNIT)) {
    GeneralLogPrintf(static_cast<SLASH_COMMAND_ID>(26), "%s casts %s on %s.", casterName, spellName, static_cast<CGUnit_C *>(victim)->GetUnitName());
  } else {
    GeneralLogPrintf(static_cast<SLASH_COMMAND_ID>(26), "%s casts %s.", casterName, spellName);
  }
}

void __fastcall UnitCombatLogCastStart(unsigned int spellID, unsigned __int64 caster) {
  if (!s_activePlayer) {
    return;
  }

  CGObject_C     *casterObjPtr = ClntObjMgrObjectPtr(caster, __FILE__, __LINE__);
  UNITAFFILIATION aAff;
  if (!ShouldLogAttacker(caster, aAff, casterObjPtr, 0, 0, -1) || !casterObjPtr || !(casterObjPtr->GetType() & TYPE_UNIT)) {
    return;
  }

  SpellRec *rec = g_spellDB.GetRecord(spellID);
  if (!rec || IsSpellQuiet(rec) || (caster == ClntObjMgrGetActivePlayer() && !IsSpellAura(rec)) || IsSpellTeach(rec)) {
    return;
  }

  const char  *casterName = static_cast<CGUnit_C *>(casterObjPtr)->GetUnitName();
  const char  *spellName = rec->m_name_lang[CURRENT_LANGUAGE];
  unsigned int selfCasting = caster == ClntObjMgrGetActivePlayer();
  const char  *templateTag;
  if (selfCasting) {
    templateTag = IsSpellAbility(rec) ? "SPELLPERFORMSELFSTART" : "SPELLCASTSELFSTART";
  } else {
    templateTag = IsSpellAbility(rec) ? "SPELLPERFORMOTHERSTART" : "SPELLCASTOTHERSTART";
  }

  const char *format = FrameScript_GetText(templateTag, -1, GENDER_NOT_APPLICABLE);
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

void __fastcall UnitCombatLog(ATTACKROUNDINFO &roundInfo) {
  ATTACKROUNDINFO info(roundInfo);
  CGObject_C     *attackerObjPtr;
  CGObject_C     *victimObjPtr;
  UNITAFFILIATION aAff;
  UNITAFFILIATION vAff;
  if (!ShouldLogAttacker(info.attacker, aAff, attackerObjPtr, 0, 0, -1) || !ShouldLogAttacker(info.victim, vAff, victimObjPtr, 0, 0, -1) ||
      !(attackerObjPtr->GetType() & TYPE_UNIT) || !(victimObjPtr->GetType() & TYPE_UNIT))
  {
    return;
  }

  const char *attackerName = static_cast<CGUnit_C *>(attackerObjPtr)->GetUnitName();
  const char *victimName = static_cast<CGUnit_C *>(victimObjPtr)->GetUnitName();
  char        outputString[256];
  if (info.dmg.totalDamage) {
    SStrPrintf(outputString, sizeof(outputString), "%s hits %s for %d.", attackerName, victimName, info.dmg.totalDamage);
  } else {
    SStrPrintf(outputString, sizeof(outputString), "%s attacks %s.", attackerName, victimName);
  }
  GeneralLogPrintf(s_affiliationLogType[aAff], "%s", outputString);
  WriteMessage(outputString);
}

void __fastcall UnitCombatLog(SPELLLOG &log) {
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

  SpellRec   *spellRec = g_spellDB.GetRecord(log.spellID);
  const char *spellName = spellRec ? spellRec->m_name_lang[CURRENT_LANGUAGE] : "Unknown Spell";
  const char *attackerName = static_cast<CGUnit_C *>(attackerObjPtr)->GetUnitName();
  const char *victimName = static_cast<CGUnit_C *>(victimObjPtr)->GetUnitName();
  char        outputString[512];
  SStrPrintf(outputString, sizeof(outputString), "%s's %s hits %s for %d.", attackerName, spellName, victimName, log.dmg.totalDamage);
  GeneralLogPrintf(s_affiliationLogType[aAff], "%s", outputString);
  WriteMessage(outputString);
}

void __fastcall UnitCombatLog(SPELLMISSLOG &log) {
  if ((s_flags & 2) && (log.flags & 8)) {
    UnitCombatLogSpellMissed(log.reason, log.spellID, log.attacker, log.victim);
  }
}

void __fastcall UnitCombatLog(MIRRORTIMERDAMAGE &log) {
  if (static_cast<unsigned int>(log.damage) > 2 || !log.amount || !log.victim) {
    return;
  }

  CGObject_C     *objPtr;
  UNITAFFILIATION aff;
  if (!ShouldLogAttacker(log.victim, aff, objPtr, 1, 0, -1) || !(objPtr->GetType() & TYPE_UNIT)) {
    return;
  }

  unsigned int other = aff != AFFILIATION_YOURSELF;
  const char  *templateTag;
  if (log.damage == UNIT_MIRROR_TIMER_EXHAUSTION) {
    templateTag = other ? "VSENVEXHAUSTIONOTHER" : "VSENVEXHAUSTIONSELF";
  } else if (log.damage == UNIT_MIRROR_TIMER_BREATH) {
    templateTag = other ? "VSENVBREATHOTHER" : "VSENVBREATHSELF";
  } else {
    return;
  }

  const char *format = FrameScript_GetText(templateTag, -1, GENDER_NOT_APPLICABLE);
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

void __fastcall UnitCombatLog(ENVIRONMENTALDAMAGE &log) {
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
  const char *format = FrameScript_GetText(buffer, -1, GENDER_NOT_APPLICABLE);
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

void __fastcall UnitCombatLogAuraAddedOrRemoved(CGUnit_C *unitPtr, int spellID, bool added, int auraSlot) {
  SpellRec       *spellRec = g_spellDB.GetRecord(spellID);
  CGObject_C     *dummy;
  UNITAFFILIATION aAff;
  if (!unitPtr || !spellRec || (spellRec->m_attributes & 0xC0) || IsSpellQuiet(spellRec) ||
      !ShouldLogAttacker(unitPtr->GetGUID(), aAff, dummy, 0, 0, -1))
  {
    return;
  }

  const char *token;
  if (added) {
    if (auraSlot < 32 || auraSlot >= 40) {
      token = aAff ? "AURAADDEDOTHERHELPFUL" : "AURAADDEDSELFHELPFUL";
    } else {
      token = aAff ? "AURAADDEDOTHERHARMFUL" : "AURAADDEDSELFHARMFUL";
    }
  } else {
    token = aAff ? "AURAREMOVEDOTHER" : "AURAREMOVEDSELF";
  }

  const char *format = FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE);
  if (!format || !*format) {
    ReportError(token);
    return;
  }

  const char *spellName = spellRec->m_name_lang[CURRENT_LANGUAGE];
  char        string[128];
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

void __fastcall UnitCombatLogSpellMissed(unsigned int missReason, unsigned int spellID, unsigned __int64 caster, unsigned __int64 victim) {
  CGObject_C     *attackerObjPtr;
  CGObject_C     *victimObjPtr;
  UNITAFFILIATION aAff;
  UNITAFFILIATION vAff;
  if (missReason >= 10 || !ShouldLogAttacker(caster, aAff, attackerObjPtr, 0, 0, -1) || !ShouldLogAttacker(victim, vAff, victimObjPtr, 0, 0, -1)) {
    return;
  }

  SpellRec   *spell = g_spellDB.GetRecord(spellID);
  CGUnit_C   *attackerPtr = static_cast<CGUnit_C *>(attackerObjPtr);
  CGUnit_C   *victimPtr = static_cast<CGUnit_C *>(victimObjPtr);
  const char *casterName = attackerPtr->GetUnitName();
  const char *victimName = victimPtr->GetUnitName();
  const char *spellName = spell ? spell->m_name_lang[CURRENT_LANGUAGE] : "";

  const char *reasonToken;
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
  const char *templateTag = templateTagBuffer;
  const char *format = FrameScript_GetText(templateTag, -1, GENDER_NOT_APPLICABLE);

  char output[128];
  if (format && *format) {
    SStrPrintf(output, sizeof(output), format, casterName, spellName, victimName);
  } else {
    SStrPrintf(output, sizeof(output), "%s's %s missed %s.", casterName, spellName, victimName);
  }
  GeneralLogPrintf(s_affiliationLogType[aAff], "%s", output);
  WriteMessage(output);
}

void __fastcall UnitCombatLogUnitDead(unsigned __int64 unit) {
  CGObject_C *object = ClntObjMgrObjectPtr(unit, __FILE__, __LINE__);
  if (!object || !(object->GetType() & TYPE_UNIT)) {
    return;
  }
  CGObject_C *dummy;
  UNITAFFILIATION affiliation;
  if (ShouldLogAttacker(unit, affiliation, dummy, 1, 1, -1)) {
    GeneralLogPrintf(s_affiliationLogType[affiliation], "%s dies.", static_cast<CGUnit_C *>(object)->GetUnitName());
  }
}

void __fastcall UnitCombatLogEnableFileLog(int enable) {
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

void __fastcall UnitCombatLogSetActivePlayer(CGPlayer_C *playerPtr) {
  s_activePlayer = playerPtr;
}

void __fastcall UnitCombatLogXPGain(const unsigned __int64 &victim, CDataStore *msg, unsigned int count) {
  FATALASSERT(msg);

  for (unsigned int i = 0; i < count; ++i) {
    unsigned __int64 guid;
    int              xp;
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

void __fastcall UnitCombatLogSpellFail(CGUnit_C *caster, int spellID, const char *message) {
  if (!s_activePlayer || !caster || !spellID) {
    return;
  }
  if (!message) {
    message = "";
  }

  CGObject_C      *dummy;
  UNITAFFILIATION  aAff;
  unsigned __int64 casterGUID = caster->GetGUID();
  if (!ShouldLogAttacker(casterGUID, aAff, dummy, 0, 0, -1)) {
    return;
  }

  SpellRec *spellRec = g_spellDB.GetRecord(spellID);
  if (!spellRec || IsSpellQuiet(spellRec)) {
    return;
  }

  const char  *casterName = caster->GetUnitName();
  unsigned int selfCasting = casterGUID == ClntObjMgrGetActivePlayer();
  const char  *templateTag;
  if (IsSpellAbility(spellRec)) {
    templateTag = selfCasting ? "SPELLFAILPERFORMSELF" : "SPELLFAILPERFORMOTHER";
  } else {
    templateTag = selfCasting ? "SPELLFAILCASTSELF" : "SPELLFAILCASTOTHER";
  }

  const char *format = FrameScript_GetText(templateTag, -1, GENDER_NOT_APPLICABLE);
  if (!format || !*format) {
    ReportError(templateTag);
    return;
  }

  const char *spellName = spellRec->m_name_lang[CURRENT_LANGUAGE];
  char        output[256];
  if (selfCasting) {
    SStrPrintf(output, sizeof(output), format, spellName, message);
  } else {
    SStrPrintf(output, sizeof(output), format, casterName, spellName, message);
  }
  GeneralLogPrintf(static_cast<SLASH_COMMAND_ID>(28), output);
  if (s_flags & 2) {
    ConsoleWrite(output, DEFAULT_COLOR);
    WriteMessage(output);
  }
}

void __fastcall UnitCombatLogHeartbeatResist(RESISTLOG &log) {
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

  SpellRec *spellRec = g_spellDB.GetRecord(log.spell);
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

void __fastcall UnitCombatLogEnchantment(ENCHANTMENTLOG &log) {
  if (!s_activePlayer || !log.attacker) {
    return;
  }

  CGObject_C     *attackerObjPtr;
  UNITAFFILIATION aAff;
  if (!ShouldLogAttacker(log.attacker, aAff, attackerObjPtr, 0, 0, -1) || !(attackerObjPtr->GetType() & TYPE_UNIT)) {
    return;
  }

  SpellItemEnchantmentRec *enchantment = g_spellItemEnchantmentDB.GetRecord(log.enchantment);
  const char              *enchantmentName = enchantment ? enchantment->m_name_lang[CURRENT_LANGUAGE] : "Unknown Enchantment";
  const ItemStats_C       *item = g_itemDBCache.GetRecord(log.itemID, 0, 0, 0);
  if (!item) {
    return;
  }
  const char *itemName = item->m_displayName[0] ? item->m_displayName[0] : "";
  const char *attackerName = static_cast<CGUnit_C *>(attackerObjPtr)->GetUnitName();

  const char     *templateTag;
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

  const char *format = FrameScript_GetText(templateTag, -1, GENDER_NOT_APPLICABLE);
  if (!format || !*format) {
    ReportError(templateTag);
    return;
  }

  char        output[256];
  const char *victimName = victimObjPtr ? static_cast<CGUnit_C *>(victimObjPtr)->GetUnitName() : "";
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

void __fastcall UnitCombatLogString(const char* buffer) {
  if (buffer && *buffer) {
    GeneralLogPrintf(static_cast<SLASH_COMMAND_ID>(25), "%s", buffer);
  }
}

void __fastcall UnitCombatLogFactionChanged(int faction, int delta) {
  FactionRec *rec = g_factionDB.GetRecord(faction);
  if (!rec || !delta) {
    return;
  }

  const char *token = delta < 0 ? "FACTION_STANDING_DECREASED" : "FACTION_STANDING_INCREASED";
  const char *format = FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE);
  if (format) {
    if (delta <= 0) {
      delta = -delta;
    }
    GeneralLogPrintf(static_cast<SLASH_COMMAND_ID>(26), format, rec->m_name_lang[CURRENT_LANGUAGE], delta);
  } else {
    GeneralLogPrintf(static_cast<SLASH_COMMAND_ID>(29), "Error, cannot find string <%s>", token);
  }
}

void __fastcall UnitCombatLogPartyKill(PARTYKILLLOG &log) {
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

  const char *format = FrameScript_GetText("PARTYKILLOTHER", -1, GENDER_NOT_APPLICABLE);
  if (!format) {
    GeneralLogPrintf(static_cast<SLASH_COMMAND_ID>(29), "Error, cannot find string <%s>", "PARTYKILLOTHER");
    return;
  }
  GeneralLogPrintf(
      static_cast<SLASH_COMMAND_ID>(27), format, static_cast<CGUnit_C *>(victimObjPtr)->GetUnitName(), static_cast<CGUnit_C *>(objPtr)->GetUnitName()
  );
}

void __fastcall UnitCombatLogShowXPGained(const unsigned __int64 &victim, int xp) {
  CGUnit_C *victimPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(victim, __FILE__, __LINE__));
  if (victimPtr && (victimPtr->GetUnitData()->flags & 8)) {
    GeneralLogPrintf(
        static_cast<SLASH_COMMAND_ID>(26), FrameScript_GetText("COMBATLOG_XPGAIN_FIRSTPERSON", -1, GENDER_NOT_APPLICABLE), victimPtr->GetUnitName(),
        xp
    );
  }
}
