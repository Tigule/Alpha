#include "Object/ObjectClient/Unit_C.h"

#include <math.h>

#include <Base/CDataStore.h>
#include <Services/SysMessage.h>

#include "Console/ConsoleVar.h"
#include "DB/DBClient/AutoCode/SpellRec.h"
#include "DB/DBClient/AutoCode/CreatureModelDataRec.h"
#include "DB/DBClient/AutoCode/CreatureSoundDataRec.h"
#include "DB/DBClient/AutoCode/SpellVisualKitRec.h"
#include "DB/DBClient/AutoCode/SpellVisualEffectNameRec.h"
#include "DB/DBClient/AutoCode/SpellVisualRec.h"
#include "DB/DBClient/AutoCode/UnitBloodRec.h"
#include "DB/DBClient/AutoCode/AttackAnimKitsRec.h"
#include "DB/DBClient/AutoCode/AttackAnimTypesRec.h"
#include "DB/DBClient/DBClient.h"
#include "Client.h"
#include "Object/ObjectClient/Player_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "SoundInterface/SoundInterface.h"
#include "Ui/GameUI.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

extern CVar *g_combatModeMaxDistance;

void __fastcall UnitCombatLogShowXPGained(const unsigned __int64 &victim, int xp);
void __fastcall UnitCombatLogXPGain(const unsigned __int64 &victim, CDataStore *msg, unsigned int count);
void __fastcall UnitCombatLog(ATTACKROUNDINFO &roundInfo);
void __fastcall UnitCombatLog(SPELLLOG &log);
void __fastcall UnitCombatLog(SPELLMISSLOG &log);
void __fastcall UnitCombatLog(MIRRORTIMERDAMAGE &log);
void __fastcall UnitCombatLog(ENVIRONMENTALDAMAGE &log);
void __fastcall UnitCombatLogHeartbeatResist(RESISTLOG &log);
void __fastcall UnitCombatLogEnchantment(ENCHANTMENTLOG &log);
void __fastcall UnitCombatLogPartyKill(PARTYKILLLOG &log);
void __fastcall UnitCombatLogInitialize();
void __fastcall UnitCombatLogShutdown();

struct CHANCES {
  unsigned int seq;
  unsigned int frequency;
};

struct WEAPONHANDCHANCES {
  unsigned int             total;
  TSGrowableArray<CHANCES> chances;

  WEAPONHANDCHANCES() : total(0) {
  }
};

struct ANIMKIT : public TSHashObject<ANIMKIT, HASHKEY_NONE> {
  WEAPONHANDCHANCES chancesArray[NUMHANDS];
};

static TSHashTable<ANIMKIT, HASHKEY_NONE> s_animKitTable;
static HASHKEY_NONE                       s_animKitKey;
static unsigned char                      s_didHitConnect[NUM_VICTIMSTATES] = {0, 1, 0, 0, 1, 1, 0, 0, 1};
static unsigned int                       s_attackAnimHitStates[NUMHANDS] = {32, 34};
static unsigned int                       s_attackAnimMissStates[NUMHANDS] = {30, 33};

void __fastcall UnitEffectOneShot(
    UNITEFFECTSPECIALS        effectNumber,
    unsigned __int64          target,
    const NTempest::C3Vector *attachPos,
    float                     facing,
    float                     scale,
    bool                      forceEffectOnMount
);
int __fastcall    UnitEffectGetSpecialVisual(UNITEFFECTSPECIALS effectNumber);
HMODEL __fastcall UnitEffectCreateAuraModel(unsigned int effectID);
void __fastcall SndInterfacePlayWeaponSwooshSound(WEAPONSWING_SOUNDTYPES soundType, int criticalHit, const NTempest::C3Vector &position, int missed);
unsigned int __fastcall SpellGetRangedPrecastHoldAnim(unsigned int loadAnim);

static unsigned int __fastcall FindAnimation(unsigned int ID) {
  static const struct {
    const char     *animName;
    ANIMENUMERATION anim;
  } s_anims[7] = {
      { "1H_Main_Swing", static_cast<ANIMENUMERATION>(17)},
      {"1H_Main_Pierce", static_cast<ANIMENUMERATION>(85)},
      {    "2HL_Pierce", static_cast<ANIMENUMERATION>(86)},
      {     "2HL_Swing", static_cast<ANIMENUMERATION>(19)},
      {     "2HT_Swing", static_cast<ANIMENUMERATION>(18)},
      {    "OffH_Swing", static_cast<ANIMENUMERATION>(87)},
      {   "OffH_Pierce", static_cast<ANIMENUMERATION>(88)}
  };

  const AttackAnimTypesRec *rec = g_attackAnimTypesDB.GetRecord(ID);
  if (!rec) {
    return static_cast<unsigned int>(-1);
  }
  for (unsigned int i = 0; i < 7; ++i) {
    if (!SStrCmpI(rec->m_AnimName, s_anims[i].animName, 0x7FFFFFFF)) {
      return s_anims[i].anim;
    }
  }
  return static_cast<unsigned int>(-1);
}

static void __fastcall LoadAnimKitTable() {
  unsigned int count = g_attackAnimKitsDB.GetNumRecords();
  while (count) {
    AttackAnimKitsRec *rec = g_attackAnimKitsDB.GetRecordByIndex(--count);
    FATALASSERT(rec);
    FATALASSERT(rec->m_AnimTypeID >= 0);
    FATALASSERT(rec->m_AnimFrequency >= 0);

    ANIMKIT *kit = s_animKitTable.Ptr(rec->m_ItemSubclassID, s_animKitKey);
    if (!kit) {
      kit = s_animKitTable.New(rec->m_ItemSubclassID, s_animKitKey, 0, 0);
    }
    FATALASSERT(rec->m_WhichHand < NUMHANDS);
    WEAPONHANDCHANCES &chancesStruct = kit->chancesArray[rec->m_WhichHand];
    CHANCES           *chance = chancesStruct.chances.New();
    chance->frequency = rec->m_AnimFrequency;
    chance->seq = FindAnimation(rec->m_AnimTypeID);
    chancesStruct.total += rec->m_AnimFrequency;
  }
}

void ATTACKROUNDINFO::PI(CDataStore &msg, int debug) {
}

void ATTACKROUNDINFO::UI(CDataStore &msg) {
  msg.Get(flags);
  msg.Get(attacker);
  msg.Get(victim);
  msg.Get(dmg.totalDamage);

  unsigned char damageCount;
  msg.Get(damageCount);
  for (unsigned int i = 0; i < damageCount; ++i) {
    msg.Get(dmg.damageType[i]);
    msg.Get(dmg.damageFloat[i]);
    msg.Get(dmg.damage[i]);
    msg.Get(dmg.absorbed[i]);
  }

  msg.Get(reinterpret_cast<unsigned int &>(newVictimState));
  msg.Get(victimRoundDuration);
  msg.Get(spellDamageAdded);
  msg.Get(spellAddedDamage);

  if (flags & 0x200) {
    msg.Get(dualWieldHitRollFloat);
    msg.Get(dualWieldHitRollNeededFloat);
  }

  if (flags & 0x2000) {
    msg.Get(armorReduction);
    msg.Get(hitRollFloat);
    msg.Get(hitRollNeededFloat);
    msg.Get(critRollFloat);
    msg.Get(critRollNeededFloat);
    msg.Get(dodgeRollFloat);
    msg.Get(dodgeRollNeededFloat);
    msg.Get(parryRollFloat);
    msg.Get(parryRollNeededFloat);
    msg.Get(blockRollFloat);
    msg.Get(blockRollNeededFloat);
    msg.Get(delayTime);
    for (unsigned int i = 0; i < 5; ++i) {
      msg.Get(dmg.minDamage[i]);
      msg.Get(dmg.maxDamage[i]);
    }
    msg.Get(scaledDamage);
    msg.Get(modDamageDone);
    msg.Get(netDamageMultiplier);
    msg.Get(maxDamageReduction);
    msg.Get(scaledArmorReduction);
    msg.Get(intellectBonus);
    msg.Get(DPSScaler);
    msg.Get(modDamageTaken);
    msg.Get(sinceLastSwing);
  }

  msg.Get(procSpell);
  FATALASSERT(attacker);
  FATALASSERT(victim);
}

void SPELLLOG::PI(CDataStore &msg, int debug) {
}

void SPELLLOG::UI(CDataStore &msg) {
  msg.Get(flags);
  msg.Get(attacker);
  msg.Get(victim);
  msg.Get(spellID);
  if (flags & 0x200) {
    FATALASSERT(attacker);
    return;
  }

  msg.Get(dmg.totalDamage);
  msg.Get(dmg.damageFloat[0]);
  msg.Get(dmg.damageType[0]);
  msg.Get(dmg.damage[0]);
  msg.Get(dmg.absorbed[0]);
  if (flags & 0x20) {
    msg.Get(dmg.minDamage[0]);
    msg.Get(dmg.maxDamage[0]);
    msg.Get(netDamageMultiplier);
    msg.Get(scaledDamage);
    msg.Get(critRollNeededFloat);
    msg.Get(critRollFloat);
    msg.Get(maxDamageReduction);
    msg.Get(scaledArmorReduction);
    msg.Get(auraEffectID);
    msg.Get(hitRollNeededFloat);
    msg.Get(hitRollFloat);
    msg.Get(damageType);
    msg.Get(resistanceCoefficient);
  }
  FATALASSERT(attacker);
  FATALASSERT(victim);
}

void SPELLMISSLOG::PI(CDataStore &msg, int debug) {
}

void SPELLMISSLOG::UI(CDataStore &msg) {
  msg.Get(flags);
  msg.Get(attacker);
  msg.Get(victim);
  msg.Get(spellID);
  msg.Get(reason);
  FATALASSERT(attacker);
  if (flags & 8) {
    msg.Get(hitRoll);
    msg.Get(hitRollNeeded);
    msg.Get(dodgeRoll);
    msg.Get(dodgeRollNeeded);
    msg.Get(parryRoll);
    msg.Get(parryRollNeeded);
    msg.Get(blockRoll);
    msg.Get(blockRollNeeded);
    FATALASSERT(victim);
  }
}

void RESISTLOG::PI(CDataStore &msg, int debug) {
}

void RESISTLOG::UI(CDataStore &msg) {
  msg.Get(attacker);
  msg.Get(victim);
  msg.Get(spell);
  msg.Get(resistRollNeeded);
  msg.Get(resistRoll);
  msg.Get(flags);
  msg.Get(castLevel);
  FATALASSERT(attacker);
  FATALASSERT(victim);
}

void ENCHANTMENTLOG::PI(CDataStore &msg, int debug) {
}

void ENCHANTMENTLOG::UI(CDataStore &msg) {
  msg.Get(flags);
  msg.Get(attacker);
  if (!(flags & 1)) {
    msg.Get(victim);
    FATALASSERT(victim);
  }
  msg.Get(enchantment);
  msg.Get(itemID);
  FATALASSERT(attacker);
}

void ENVIRONMENTALDAMAGE::PI(CDataStore &msg, int debug) {
}

void ENVIRONMENTALDAMAGE::UI(CDataStore &msg) {
  msg.Get(victim);
  msg.Get(school);
  msg.Get(amount);
}

void MIRRORTIMERDAMAGE::PI(CDataStore &msg, int debug) {
}

void MIRRORTIMERDAMAGE::UI(CDataStore &msg) {
  msg.Get(victim);
  msg.Get(damage);
  msg.Get(amount);
}

void PARTYKILLLOG::PI(CDataStore &msg, int debug) {
}

void PARTYKILLLOG::UI(CDataStore &msg) {
  msg.Get(killer);
  msg.Get(victim);
}

int __fastcall OnUnitCombatEvent(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  FATALASSERT(msg);

  switch (msgId) {
    case SMSG_ATTACKERSTATEUPDATEDEBUGINFO: {
      ATTACKROUNDINFO attackInfo;
      attackInfo.UI(*msg);
      UnitCombatLog(attackInfo);
      return 1;
    }
    case SMSG_ATTACKERSTATEUPDATEDEBUGINFOSPELL: {
      SPELLLOG log;
      log.UI(*msg);
      UnitCombatLog(log);
      return 1;
    }
    case SMSG_ATTACKERSTATEUPDATEDEBUGINFOSPELLMISS: {
      SPELLMISSLOG log;
      log.UI(*msg);
      UnitCombatLog(log);
      return 1;
    }
    case SMSG_ATTACKSTART: {
      unsigned __int64 victim;
      unsigned __int64 attacker;
      msg->Get(victim);
      msg->Get(attacker);
      CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(victim, __FILE__, __LINE__));
      if (unit) {
        unit->OnAttackStart(attacker);
      }
      return 1;
    }
    case SMSG_ATTACKSTOP: {
      unsigned __int64 attacker;
      unsigned __int64 victim;
      unsigned int     nowDead;
      msg->Get(attacker);
      msg->Get(victim);
      msg->Get(nowDead);
      CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(attacker, __FILE__, __LINE__));
      if (unit) {
        unit->OnAttackStop(victim, nowDead);
      }
      return 1;
    }
    case SMSG_ATTACKERSTATEUPDATE: {
      ATTACKROUNDINFO attackInfo;
      attackInfo.UI(*msg);
      CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(0, __FILE__, __LINE__));
      if (unit) {
        unit->SetDebugHitRolls(attackInfo);
        unit->OnAttackerStateChange(attackInfo);
        UnitCombatLog(attackInfo);
      }
      return 0;
    }
    case SMSG_ATTACKSWING_NOTINRANGE:
    case SMSG_ATTACKSWING_BADFACING:
    case SMSG_ATTACKSWING_NOTSTANDING:
    case SMSG_ATTACKSWING_DEADTARGET:
    case SMSG_ATTACKSWING_CANT_ATTACK: {
      unsigned __int64 attacker;
      unsigned __int64 victim;
      msg->Get(attacker);
      msg->Get(victim);
      CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(attacker, __FILE__, __LINE__));
      if (!unit) {
        return 0;
      }
      if (msgId == SMSG_ATTACKSWING_NOTINRANGE) {
        unit->OnBadAttackPosition(victim, 0.0f);
      } else if (msgId == SMSG_ATTACKSWING_BADFACING) {
        unit->OnBadAttackFacing(victim);
      } else if (msgId == SMSG_ATTACKSWING_NOTSTANDING) {
        unit->OnNotStanding(victim);
      } else {
        unit->OnBadAttackTarget(victim);
      }
      return 0;
    }
    case SMSG_RESISTLOG: {
      RESISTLOG log;
      log.UI(*msg);
      UnitCombatLogHeartbeatResist(log);
      return 1;
    }
    case SMSG_ENCHANTMENTLOG: {
      ENCHANTMENTLOG log;
      log.UI(*msg);
      UnitCombatLogEnchantment(log);
      return 1;
    }
    case SMSG_MIRRORTIMERDAMAGELOG: {
      MIRRORTIMERDAMAGE log;
      log.UI(*msg);
      CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(log.victim, __FILE__, __LINE__));
      if (unit) {
        unit->HandleMirrorTimerDamage(log);
      }
      return 1;
    }
    case SMSG_ENVIRONMENTALDAMAGELOG: {
      ENVIRONMENTALDAMAGE log;
      log.UI(*msg);
      UnitCombatLog(log);
      return 1;
    }
    case SMSG_LOG_XPGAIN: {
      unsigned __int64 guid;
      unsigned int     count;
      msg->Get(guid);
      msg->Get(count);
      UnitCombatLogXPGain(guid, msg, count);
      return 1;
    }
    case SMSG_PARTYKILLLOG: {
      PARTYKILLLOG log;
      log.UI(*msg);
      UnitCombatLogPartyKill(log);
      return 1;
    }
    default:
      FATALASSERT(!"Error, unrecognized message ID!");
      return 0;
  }
}

int __fastcall OnUnitDamageDone(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  unsigned __int64 attacker;
  unsigned __int64 guid;
  int              damage;
  int              normalCombatDamage;
  unsigned int     flags;
  int              spellID;

  msg->Get(attacker);
  msg->Get(damage);
  msg->Get(normalCombatDamage);
  msg->Get(flags);
  msg->Get(spellID);
  msg->Get(guid);

  CGUnit_C *attackerPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(attacker, __FILE__, __LINE__));
  if (attackerPtr) {
    attackerPtr->AddDamageDone(damage, normalCombatDamage, flags, guid, spellID);
  }
  return 1;
}

int __fastcall OnUnitCombatEvent(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
int __fastcall OnUnitDamageDone(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
int __fastcall OnUnitDamageTaken(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);

int __fastcall OnUnitDamageTaken(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  unsigned __int64 guid;
  unsigned int     flags;
  int              damage;
  unsigned char    damageClass;

  msg->Get(guid);
  msg->Get(damageClass);
  msg->Get(damage);
  msg->Get(flags);
  if (damage > 0) {
    CGGameUI::ShowCombatFeedback(guid, damage, damageClass, flags);
  }
  return 1;
}

void CGUnit_C::SetDebugHitRolls(ATTACKROUNDINFO &info) {
  unsigned int *self = reinterpret_cast<unsigned int *>(this);
  unsigned int *roundInfo = reinterpret_cast<unsigned int *>(&info);

  self[388] |= 8;
  memcpy(self + 322, roundInfo + 2, 66 * sizeof(unsigned int));
}

int CGUnit_C::IsPreemptableWoundAnimState(unsigned int state) {
  return state == 27 || state == 28 || state == 35 || state == 36 || state == 40;
}

int CGUnit_C::IsAttackAnimState(unsigned int state) {
  return state == 30 || state == 32 || state == 33 || state == 34;
}

unsigned int CGUnit_C::QueueVictimAnim(VICTIMSTATES newState, int unitDead, int criticalHit, unsigned int victimRoundDuration) {
  unsigned int *self = reinterpret_cast<unsigned int *>(this);
  if (((1 << newState) & 0x2E) && IsPreemptableWoundAnimState(self[442])) {
    unsigned int midpoint = self[445] + (self[446] >> 1);
    int          elapsed = GetTickCount() - midpoint;
    return !self[446] || elapsed <= 0;
  }

  if (self[442] == 37 || self[442] == 38) {
    return 0;
  }

  if (ObjectIsRendering()) {
    ATTACKROUNDINFO dummy;
    memset(reinterpret_cast<unsigned int *>(&dummy) + 2, 0, 66 * sizeof(unsigned int));
    dummy.newVictimState = newState;
    if (unitDead) {
      dummy.flags = 4;
    }
    if (criticalHit) {
      dummy.flags |= 8;
    }
    dummy.victimRoundDuration = victimRoundDuration;
    QueueAnim(ANIMQUEUE_WOUND, &dummy);
  }
  return 1;
}

void CGUnit_C::SetVictimAnimation(VICTIMSTATES newState, int unitDead, int criticalHit, unsigned int victimRoundDuration, int processNow) {
  FATALASSERT(newState < NUM_VICTIMSTATES);

  if (!processNow && QueueVictimAnim(newState, unitDead, criticalHit, victimRoundDuration)) {
    return;
  }

  unsigned int sequence = 0;
  switch (newState) {
    case VS_NONE:
      break;
    case VS_WOUND:
    case VS_INTERRUPT:
      UpdateBaseAnimation((criticalHit != 0) + 2, 0);
      sequence = (criticalHit != 0) + 27;
      break;
    case VS_DODGE:
    case VS_EVADE:
    case VS_DEFLECT:
      sequence = 36;
      break;
    case VS_PARRY:
      sequence = 35;
      break;
    case VS_BLOCK:
      sequence = 40;
      break;
    default:
      FATALASSERT(!"bad enum value");
      break;
  }

  unsigned int *self = reinterpret_cast<unsigned int *>(this);
  if (s_didHitConnect[newState]) {
    self[423] = 0;
  }

  if (!unitDead || m_unit->health > 0 || self[448]) {
    if (!sequence) {
      m_flags |= 2;
      UpdateBaseAnimation(0);
      return;
    }
  } else {
    if (!(m_animFlags & 0x2000)) {
      OnDeathAnimate();
    }
    sequence = 1;
    if (s_didHitConnect[newState]) {
      UpdateBaseAnimation(4, 0);
    }
  }

  if (self[442] == 37) {
    unsigned int anim = SpellGetRangedPrecastHoldAnim(GetCurrentTorsoAnim());
    if (anim != static_cast<unsigned int>(-1)) {
      self[392] = anim;
    }
  }

  SetTorsoAnimation(sequence, victimRoundDuration, 16);
}

int CGUnit_C::SetAttackerAnimation(ATTACKROUNDINFO *roundInfo, int processNow) {
  FATALASSERT(roundInfo);

  COMBATHAND hand = COMBAT_MAINHAND;
  if (roundInfo->flags & 0x200) {
    if (!roundInfo->dmg.totalDamage) {
      return 0;
    }
    hand = COMBAT_OFFHAND;
  }

  CGUnit_C *victimPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(roundInfo->victim, __FILE__, __LINE__));
  if (!processNow) {
    if (ObjectIsRendering()) {
      QueueAnim(ANIMQUEUE_ATTACK, roundInfo);
      return 1;
    }
    if (victimPtr) {
      victimPtr->DoVictimFeedback(roundInfo, 1);
    }
    return 1;
  }

  CGUnit_C *player = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    char buff[256];
    SStrPrintf(buff, sizeof(buff), "%s setting attacker anim", GetUnitName());
    player->DDGENLOG(GetGUID(), buff, __FILE__, __LINE__);
  }

  unsigned int state = (roundInfo->flags & 1) ? s_attackAnimHitStates[hand] : s_attackAnimMissStates[hand];
  if (!SetTorsoAnimation(state, 0, 0)) {
    if (victimPtr) {
      victimPtr->DoVictimFeedback(roundInfo, 1);
    }
    return reinterpret_cast<unsigned int *>(this)[442] == 38;
  }

  if (victimPtr) {
    SetMeleeDeathHold(victimPtr);
  }
  UpdateBaseAnimation((roundInfo->flags >> 3) & 1, 0);

  unsigned int sequence = ChooseAnimation(state);
  unsigned int seqDuration;
  HMODEL       model = reinterpret_cast<HMODEL *>(this)[4];
  if (ModelGetSequenceDuration(model, sequence, &seqDuration)) {
    unsigned int random = NTempest::CMath::mulhwu_(16, NTempest::CRandom::uint32_(g_rndSeed));
    unsigned int scaledDuration = static_cast<unsigned int>((random + 85.0f) * seqDuration * 0.01f);
    unsigned int attackTime = m_unit->attackRoundBaseTime[(roundInfo->flags >> 9) & 1];
    if (scaledDuration > attackTime) {
      scaledDuration = attackTime;
    }
    if (scaledDuration) {
      ModelSetTimeScale(model, static_cast<float>(seqDuration) / static_cast<float>(scaledDuration), 0);
    }
  }
  return 1;
}

unsigned int CGUnit_C::GetAttackerAnimEx(COMBATHAND hand, const VirtualItemInfo *itemInfo) const {
  FATALASSERT(hand < NUMHANDS);
  FATALASSERT(itemInfo);

  const unsigned char *item = reinterpret_cast<const unsigned char *>(itemInfo);
  ANIMKIT             *kit = s_animKitTable.Ptr(item[1], s_animKitKey);
  if (!kit) {
    return static_cast<unsigned int>(-1);
  }

  WEAPONHANDCHANCES &chancesStruct = kit->chancesArray[hand];
  unsigned int       count = chancesStruct.chances.Count();
  if (!count) {
    return static_cast<unsigned int>(-1);
  }

  unsigned int dice = NTempest::CMath::mulhwu_(chancesStruct.total + 1, NTempest::CRandom::uint32_(g_rndSeed));
  unsigned int accumulated = 0;
  for (unsigned int i = 0; i < count; ++i) {
    accumulated += chancesStruct.chances[i].frequency;
    if (dice <= accumulated) {
      return chancesStruct.chances[i].seq;
    }
  }
  return static_cast<unsigned int>(-1);
}

unsigned int CGUnit_C::DetermineAttackerSequence(COMBATHAND hand) const {
  FATALASSERT(hand < NUMHANDS);
  static const unsigned int s_slots[NUMHANDS] = {0, 1};
  static const unsigned int s_unarmed[NUMHANDS] = {16, 117};
  static const unsigned int s_weaponSeq[5] = {18, 19, 46, 17, 49};

  typedef VirtualItemInfo *(CGUnit_C::*GetVirtualItemProc)(unsigned int, unsigned int);
  GetVirtualItemProc getVirtualItem;
  void             **vtable = *reinterpret_cast<void ***>(const_cast<CGUnit_C *>(this));
  memcpy(&getVirtualItem, &vtable[0x128 / sizeof(void *)], sizeof(getVirtualItem));
  VirtualItemInfo *itemInfo = (const_cast<CGUnit_C *>(this)->*getVirtualItem)(s_slots[hand], 0);

  if (!itemInfo || *reinterpret_cast<unsigned char *>(itemInfo) != 2) {
    unsigned int sequence = s_unarmed[hand];
    if (!ModelHasSequenceId(reinterpret_cast<HMODEL const *>(this)[4], sequence)) {
      PrintAttackSeqErrorMsg(sequence, 17);
      return 17;
    }
    return sequence;
  }

  unsigned int sequence = GetAttackerAnimEx(hand, itemInfo);
  if (sequence == static_cast<unsigned int>(-1) || !ModelHasSequenceId(reinterpret_cast<HMODEL const *>(this)[4], sequence)) {
    WEAPONATTACKSEQ weaponSeq = ClientDBGetWeaponSubclassWeaponSeq(reinterpret_cast<unsigned char *>(itemInfo)[1]);
    if (weaponSeq <= 2 || weaponSeq == 4) {
      sequence = s_weaponSeq[weaponSeq];
    } else {
      sequence = hand == COMBAT_MAINHAND ? 17 : 87;
    }
  }

  if (!ModelHasSequenceId(reinterpret_cast<HMODEL const *>(this)[4], sequence)) {
    PrintAttackSeqErrorMsg(sequence, 16);
    return 16;
  }
  return sequence;
}

unsigned int CGUnit_C::DetermineParrySequence() const {
  static const unsigned int s_anims[4] = {22, 23, 21, 0};

  typedef VirtualItemInfo *(CGUnit_C::*GetVirtualItemProc)(unsigned int, unsigned int);
  GetVirtualItemProc getVirtualItem;
  void             **vtable = *reinterpret_cast<void ***>(const_cast<CGUnit_C *>(this));
  memcpy(&getVirtualItem, &vtable[0x128 / sizeof(void *)], sizeof(getVirtualItem));
  VirtualItemInfo *itemInfo = (const_cast<CGUnit_C *>(this)->*getVirtualItem)(0, 0);
  if (!itemInfo || *reinterpret_cast<unsigned char *>(itemInfo) != 2) {
    SysMsgPrintf(SYSMSG_ERROR, 2, "NOWEAPONPARRY|%d|0x%016I64X", 0, GetGUID());
    return 20;
  }

  WEAPONPARRYSEQ seq = ClientDBGetWeaponSubclassParrySeq(reinterpret_cast<unsigned char *>(itemInfo)[1]);
  FATALASSERT(seq < 4);
  unsigned int anim = s_anims[seq];
  return anim ? anim : GetStandStateAnim(0);
}

int CGUnit_C::QueueAnim(ANIMQUEUETYPE type, ATTACKROUNDINFO *roundInfo) {
  FATALASSERT(type < ANIMQUEUE_NUMTYPES);
  if (type == ANIMQUEUE_NONE) {
    return 0;
  }

  unsigned int *self = reinterpret_cast<unsigned int *>(this);
  if (type == ANIMQUEUE_WOUND && IsAttackAnimState(self[442])) {
    return 0;
  }
  if (m_animFlags & 0x2000) {
    return 0;
  }

  ANIMQUEUENODE *node = GetNewAnimNode(0);
  node->type = type;
  if (roundInfo) {
    memcpy(reinterpret_cast<unsigned int *>(node) + 6, reinterpret_cast<unsigned int *>(roundInfo) + 2, 66 * sizeof(unsigned int));
  }
  if (type >= ANIMQUEUE_SITDOWN && type <= ANIMQUEUE_KNEELUP) {
    m_flags |= 0x40000;
  }
  return 1;
}

void CGUnit_C::SetFingersSeq(HMODEL charModel, unsigned int sequence, unsigned int startFinger, unsigned int lastFinger) {
  FATALASSERT(charModel);
  for (unsigned int finger = startFinger; finger <= lastFinger; ++finger) {
    if (ModelLockObjectSequence(charModel, finger, 0)) {
      if (ModelSetSequence(charModel, sequence, finger, 4)) {
        ModelLockObjectSequence(charModel, finger, 1);
      }
    }
  }
}

void CGUnit_C::ResetFingersSeq(HMODEL charModel, unsigned int startFinger, unsigned int lastFinger) {
  FATALASSERT(charModel);
  for (unsigned int finger = startFinger; finger <= lastFinger; ++finger) {
    if (ModelLockObjectSequence(charModel, finger, 0)) {
      ModelMatchSequence(charModel, finger, 4, 6);
    }
  }
}

void CGUnit_C::SetHandState(HMODEL model, VirtualItemInfo *item, unsigned int startFinger, unsigned int lastFinger) {
  const unsigned char *itemBytes = reinterpret_cast<const unsigned char *>(item);
  unsigned int         sheatheReasons = *reinterpret_cast<const unsigned int *>(reinterpret_cast<const unsigned char *>(this) + 0x9A4);
  HMODEL               paperDollModel = *reinterpret_cast<HMODEL *>(reinterpret_cast<unsigned char *>(this) + 0x9A0);

  if (item && !sheatheReasons && ClientDBWeaponSubclassSetsFingerSeq(itemBytes[1])) {
    SetFingersSeq(model, 15, startFinger, lastFinger);
    if (paperDollModel) {
      SetFingersSeq(paperDollModel, 15, startFinger, lastFinger);
    }
  } else {
    ResetFingersSeq(model, startFinger, lastFinger);
    if (paperDollModel) {
      ResetFingersSeq(paperDollModel, startFinger, lastFinger);
    }
  }
}

void CGUnit_C::DetermineReadySequence(unsigned int forceNormal) {
  unsigned char *self = reinterpret_cast<unsigned char *>(this);
  unsigned int  &readySequence = *reinterpret_cast<unsigned int *>(self + 0x4D4);

  if (!(self[0x4E4] & 4)) {
    readySequence = 27;
    return;
  }

  unsigned int         weaponMode = forceNormal ? 0 : *(reinterpret_cast<unsigned char *>(m_unit) + 0x29B);
  const unsigned char *itemInfo = 0;
  typedef VirtualItemInfo *(CGUnit_C::*GetVirtualItemFn)(unsigned int, unsigned int);
  void           **vtable = *reinterpret_cast<void ***>(this);
  GetVirtualItemFn getVirtualItem;
  memcpy(&getVirtualItem, &vtable[0x128 / 4], sizeof(getVirtualItem));

  if (weaponMode == WEAPONMODE_RANGED) {
    itemInfo = reinterpret_cast<unsigned char *>((this->*getVirtualItem)(2, 0));
  } else if (weaponMode == WEAPONMODE_SHEATHED) {
    itemInfo = reinterpret_cast<unsigned char *>((this->*getVirtualItem)(0, 0));
  }

  ANIMENUMERATION sequence = static_cast<ANIMENUMERATION>(25);
  if (itemInfo && itemInfo[0] == 2) {
    static const ANIMENUMERATION s_anims[6] = {static_cast<ANIMENUMERATION>(27), static_cast<ANIMENUMERATION>(28), static_cast<ANIMENUMERATION>(26),
                                               static_cast<ANIMENUMERATION>(29), static_cast<ANIMENUMERATION>(48), static_cast<ANIMENUMERATION>(108)};
    WEAPONREADYSEQ               readySeq = ClientDBGetWeaponSubclassReadySeq(itemInfo[1]);
    FATALASSERT(readySeq < 6);
    sequence = s_anims[readySeq];
  }

  if (readySequence != static_cast<unsigned int>(sequence)) {
    readySequence = sequence;
    if (*reinterpret_cast<unsigned int *>(self + 0x6E0) == 31) {
      typedef void (CGUnit_C::*UpdateBaseAnimationFn)(unsigned int);
      UpdateBaseAnimationFn updateBaseAnimation;
      memcpy(&updateBaseAnimation, &vtable[0x110 / 4], sizeof(updateBaseAnimation));
      (this->*updateBaseAnimation)(0);
    }
  }
}

void CGUnit_C::HandleCombatAnimEvent(const char *eventName, unsigned long value, const NTempest::C3Vector &position) {
  CGUnit_C *victimPtr = 0;
  if (m_currentDamageInfo) {
    victimPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(m_currentDamageInfo->roundInfo.victim, __FILE__, __LINE__));
  }

  switch (value) {
    case 0x30484124:  // $AH0
    case 0x31484124:  // $AH1
    case 0x32484124:  // $AH2
    case 0x33484124:  // $AH3
      if (victimPtr && m_soundData) {
        unsigned int index = SStrToInt(eventName + 3);
        if (index < 4) {
          victimPtr->SetCustomAttackSound(m_soundData->m_customAttack[index], position);
        }
      }
      // Fall through.
    case 0x48414324:  // $CAH
      if (m_currentBaseAnimState == 38) {
        CheckPendingMissileRelease(&position);
      }
      if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__)) {
        char buff[256];
        SStrPrintf(buff, sizeof(buff), "%s attackHit event found", GetUnitName());
        DDGENLOG(GetGUID(), buff, __FILE__, __LINE__);
      }
      CheckPendingVictimFeedback();
      break;

    case 0x50504324:  // $CPP
      if (m_currentDamageInfo && victimPtr) {
        ATTACKROUNDINFO &roundInfo = m_currentDamageInfo->roundInfo;
        if ((roundInfo.flags & 2) && ((1 << roundInfo.newVictimState) & 0x14C)) {
          victimPtr->SetVictimAnimation(roundInfo.newVictimState, roundInfo.flags & 4, roundInfo.flags & 8, roundInfo.victimRoundDuration, 0);
          roundInfo.flags &= ~2u;
        }
      }
      break;

    case 0x48544424:  // $DTH
      PlayDeathThud();
      OnDeathAnimate();
      break;

    case 0x50574224: {  // $BWP
      m_flags = m_flags & ~0xC00u | 0x400;
      unsigned int currentTime = GetTickCount();
      if (m_animEndTime > currentTime) {
        unsigned int duration = m_animStartTime + m_animBaseDuration - currentTime;
        if (duration) {
          SetRangedWeaponPullAnim(duration);
        }
      }
      ShowHandArrow(1);
      break;
    }

    case 0x53534324: {  // $CSS
      WEAPONSWING_SOUNDTYPES type = WEAPONSWING_LIGHT;
      if (m_currentDamageInfo && GetWeaponSwingType(!(m_currentDamageInfo->roundInfo.flags & 0x200), type)) {
        SndInterfacePlayWeaponSwooshSound(type, m_currentDamageInfo->roundInfo.flags & 8, position, m_currentDamageInfo->roundInfo.flags & 1);
      }
      break;
    }
  }
}

void CGUnit_C::OnAttackSwing(unsigned __int64 victimGUID, unsigned int clientTimeStamp) {
  if (m_combat.StopBeenSent() || (!m_combat.IsAttacking() && !m_combat.AttackBeenSent())) {
    CDataStore msg;
    msg.Put(static_cast<int>(CMSG_ATTACKSWING));
    msg.Put(victimGUID);
    msg.Finalize();
    ClientServices_Send(&msg);
    m_combat.SetAttackSent(victimGUID);
  }
}

void CGUnit_C::OnAttackStart(unsigned __int64 victim) {
  FATALASSERT(victim);

  unsigned int *combat = reinterpret_cast<unsigned int *>(&m_combat);
  combat[0] = static_cast<unsigned int>(victim);
  combat[1] = static_cast<unsigned int>(victim >> 32);
  combat[2] = 0;
  reinterpret_cast<unsigned int *>(this)[388] = 0;

  CGUnit_C *victimPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(victim, __FILE__, __LINE__));
  if (victimPtr) {
    victimPtr->OnGetAttacked(GetGUID());
  }
}

void CGUnit_C::OnAttackStop(unsigned __int64 previousTarget, int nowDead) {
  unsigned int *self = reinterpret_cast<unsigned int *>(this);
  unsigned int *combat = reinterpret_cast<unsigned int *>(&m_combat);

  self[388] = 0;
  combat[2] = 0;
  combat[0] = 0;
  combat[1] = 0;
  combat[2] = 0;
  combat[3] = 0;

  if (!(g_seqInformation[self[442]].flags & 0x10)) {
    UpdateBaseAnimation(0);
  }

  self[388] = 0;
  if (previousTarget && nowDead) {
    CGUnit_C *victimPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(previousTarget, __FILE__, __LINE__));
    if (victimPtr) {
      m_flags |= 1;
      NTempest::C3Vector position;
      NTempest::C3Vector victimPosition;
      GetPosition(position);
      victimPtr->GetPosition(victimPosition);
      float facing = UnitCalculateFacingTo(position, victimPosition);
      self[434] = *reinterpret_cast<unsigned int *>(&facing);
    }
  }
}

void CGUnit_C::OnAttackerStateChange(ATTACKROUNDINFO &roundInfo) {
  reinterpret_cast<unsigned int *>(this)[388] = 0;

  CGUnit_C *victimPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(roundInfo.victim, __FILE__, __LINE__));
  if (victimPtr) {
    AdjustVictimState(&roundInfo);
  }

  if (roundInfo.flags & 0x1000) {
    QueueAnim(ANIMQUEUE_ATTACK, &roundInfo);
  } else {
    SetAttackerAnimation(&roundInfo, 0);
  }

  if (victimPtr) {
    victimPtr->m_flags &= ~2u;
    victimPtr->UnitHit(roundInfo.newVictimState, GetGUID());
    if ((roundInfo.flags & 2) && s_didHitConnect[roundInfo.newVictimState]) {
      victimPtr->m_flags |= 0x8000;
    }
  } else {
    unsigned int *combat = reinterpret_cast<unsigned int *>(&m_combat);
    combat[0] = 0;
    combat[1] = 0;
    combat[2] = 0;
    combat[3] = 0;
  }

  if (roundInfo.victim == ClntObjMgrGetActivePlayer() && !CGGameUI::GetLockedTarget()) {
    unsigned __int64 attacker = GetGUID();
    CGGameUI::Target(attacker, 0);
  }
}

void CGUnit_C::HandleMirrorTimerDamage(MIRRORTIMERDAMAGE &log) {
  UnitCombatLog(log);
}

void CGUnit_C::DoVictimFeedback(ATTACKROUNDINFO *roundInfo, int showAnimation) {
  FATALASSERT(roundInfo);

  CGUnit_C *attackerPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(roundInfo->attacker, __FILE__, __LINE__));
  if (!(roundInfo->flags & 0x1000) && attackerPtr) {
    attackerPtr->SetMeleeDeathHold(0);
  }

  NTempest::C3Vector position;
  switch (roundInfo->newVictimState) {
    case VS_PARRY:
      GetPosition(position);
      PlayParrySound(0, roundInfo, position);
      break;
    case VS_BLOCK:
      GetPosition(position);
      PlayParrySound(1, roundInfo, position);
      break;
    case VS_IMMUNE:
      GetPosition(position);
      SndInterfacePlayImmuneSound(position);
      break;
    default:
      if (roundInfo->flags & 0x10000) {
        GetPosition(position);
        SndInterfacePlayAbsorbedSound(position);
      }
      break;
  }

  if (roundInfo->flags & 2) {
    if (showAnimation) {
      AdjustVictimState(roundInfo);
    }
    if (roundInfo->spellDamageAdded && (roundInfo->newVictimState == VS_WOUND || roundInfo->newVictimState == VS_INTERRUPT)) {
      ShowBloodSpurt(attackerPtr, roundInfo->flags & 0x400);
    }
  }

  if (!(m_flags & 2)) {
    m_flags |= 2;
    if (m_unit->health <= 0) {
      OnDeathAnimate();
    }
  }

  if (m_flags & 0x8000) {
    if (m_customAttackSound == -1) {
      GetPosition(position);
      if (roundInfo->newVictimState == VS_DEFLECT) {
        SndInterfacePlayDeflectedSound(position);
      } else {
        PlayImpactSound(roundInfo->attacker, roundInfo->flags & 8, static_cast<COMBATHAND>((roundInfo->flags >> 9) & 1));
      }
    } else {
      PlayCustomAttackSound(m_customAttackSound, m_customAttackPosition);
      m_customAttackSound = -1;
    }
    m_flags &= ~0x8000u;
  }

  PerformSpellProcImpact(roundInfo->procSpell);
  if (!(roundInfo->flags & 0x1000)) {
    ShowWorldText(roundInfo);
  }
  if (attackerPtr) {
    attackerPtr->PerformLevelUpAnim(0);
  }
}

void CGUnit_C::AdjustVictimState(ATTACKROUNDINFO *roundInfo) {
  if (roundInfo->newVictimState == VS_PARRY) {
    VirtualItemInfo *item = GetAttackingWeapon(COMBAT_MAINHAND);
    if (!item || !m_unit->virtualItemDisplay[0]) {
      roundInfo->flags |= 0x40000;
      roundInfo->newVictimState = VS_DEFLECT;
    }
  } else if (roundInfo->newVictimState == VS_BLOCK) {
    VirtualItemInfo *item = &m_unit->virtualItemInfo[1];
    if (!item->m_classID || !m_unit->virtualItemDisplay[1]) {
      roundInfo->flags &= ~0x40000u;
      roundInfo->newVictimState = VS_DEFLECT;
    }
  }
}

MISS_REASON CGUnit_C::AdjustVictimState(MISS_REASON reason) {
  if (static_cast<int>(reason) == 6) {
    if (!GetAttackingWeapon(COMBAT_MAINHAND) || !m_unit->virtualItemDisplay[0]) {
      return static_cast<MISS_REASON>(9);
    }
  } else if (static_cast<int>(reason) == 7) {
    VirtualItemInfo *item = &m_unit->virtualItemInfo[1];
    if (!item->m_classID || !m_unit->virtualItemDisplay[1]) {
      return static_cast<MISS_REASON>(9);
    }
  }
  return reason;
}

void CGUnit_C::ShowWorldText(ATTACKROUNDINFO *roundInfo) {
  unsigned __int64 activePlayer = ClntObjMgrGetActivePlayer();
  if (activePlayer != roundInfo->attacker) {
    return;
  }

  CGUnit_C *victimPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(roundInfo->victim, __FILE__, __LINE__));
  if (!victimPtr || ((roundInfo->flags & 0x1000) && !(roundInfo->flags & 1) && ((1 << roundInfo->newVictimState) & 0xEC))) {
    return;
  }

  switch (roundInfo->newVictimState) {
    case VS_DEFLECT:
      victimPtr->AddWorldText(WORLDTEXTMISS_DEFLECTED);
      break;
    case VS_PARRY:
      victimPtr->AddWorldText(WORLDTEXTMISS_PARRIED);
      break;
    case VS_EVADE:
      victimPtr->AddWorldText(WORLDTEXTMISS_EVADED);
      break;
    case VS_DODGE:
      victimPtr->AddWorldText(WORLDTEXTMISS_DODGED);
      break;
    case VS_BLOCK:
      victimPtr->AddWorldText(WORLDTEXTMISS_BLOCKED);
      break;
    case VS_IMMUNE:
      victimPtr->AddWorldText(WORLDTEXTMISS_IMMUNE);
      break;
    default:
      if (roundInfo->dmg.totalDamage) {
        int normalCombatDamage = !(roundInfo->flags & 0x1000);
        if (roundInfo->flags & 8) {
          victimPtr->AddWorldCritText(roundInfo->dmg.totalDamage, normalCombatDamage);
        } else {
          victimPtr->AddWorldDamageText(roundInfo->dmg.totalDamage, normalCombatDamage);
        }
      } else if (activePlayer != roundInfo->victim && !(roundInfo->flags & 0x20000)) {
        victimPtr->AddWorldText((roundInfo->flags & 0x10000) ? WORLDTEXTMISS_ABSORBED : WORLDTEXTMISS_PHYSICAL);
      }
      break;
  }
}

void CGUnit_C::PerformSpellProcImpact(int spell) {
  SpellRec *spellRec = g_spellDB.GetRecord(spell);
  if (!spellRec) {
    return;
  }

  SpellVisualRec *visualRec = g_spellVisualDB.GetRecord(spellRec->m_spellVisualID);
  if (!visualRec) {
    return;
  }

  SpellVisualKitRec *impactKit = g_spellVisualKitDB.GetRecord(visualRec->m_impactKit);
  if (impactKit) {
    SetImpactKitEffect(spell, this, impactKit, 1);
  }
}

void CGUnit_C::ShowBloodSpurt(CGUnit_C *attacker, int crushingBlow) {
  UnitBloodRec *bloodRec = GetBloodRecord();
  if (!attacker || !bloodRec) {
    return;
  }

  BLOODSPURTLOCATION linkPoint = DetermineBloodLinkPoint(attacker);
  int                effectID;
  if (linkPoint == BLOODSPURT_BACK) {
    effectID = bloodRec->m_CombatBloodSpurtBack[crushingBlow != 0];
  } else {
    effectID = bloodRec->m_CombatBloodSpurtFront[crushingBlow != 0];
  }

  SpellVisualEffectNameRec *effectRec = g_spellVisualEffectNameDB.GetRecord(effectID);
  if (effectRec) {
    UnitEffectOneShot(static_cast<UNITEFFECTSPECIALS>(effectRec->m_specialID), GetGUID(), 0, 0.0f, 1.0f, 0);
  }
  QueueBloodSplat(linkPoint);
}

BLOODSPURTLOCATION CGUnit_C::DetermineBloodLinkPoint(CGUnit_C *attacker) {
  FATALASSERT(attacker);

  NTempest::C3Vector toAttacker = attacker->GetPosition() - GetPosition();
  return cos(GetFacing()) * toAttacker.x + sin(GetFacing()) * toAttacker.y < 0.0f ? BLOODSPURT_BACK : BLOODSPURT_FRONT;
}

void CGUnit_C::OnDeathAnimate() {
  InitializeResEffectModel();
  ClearMeleeDeathHold();
  FATALASSERT(!(m_flags & 0x2000));
  m_flags |= 0x2000;
  CheckPendingVictimFeedback();
  FinishAuraDecays();
  SetSheatheReason(SHEATHEREASON_4, 0, 0);
  ClearTrackingTarget(0);
}

void CGUnit_C::InitializeResEffectModel() {
  if (!(GetType() & TYPE_PLAYER)) {
    return;
  }

  CGUnit_C *activePlayer = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!activePlayer || !activePlayer->CanCooperate(this)) {
    return;
  }

  ClearResEffectModel();
  m_resEffectModel = UnitEffectCreateAuraModel(UnitEffectGetSpecialVisual(static_cast<UNITEFFECTSPECIALS>(42)));
  AttachResEffectModel();
}

void CGUnit_C::ClearResEffectModel() {
  DetatchResEffectModel();
  if (m_resEffectModel) {
    HandleClose(m_resEffectModel);
  }
  m_resEffectModel = 0;
}

void CGUnit_C::AttachResEffectModel() {
  if (!m_resEffectModel) {
    return;
  }

  HMODEL model = GetCharacterModel(0);
  if (model) {
    ModelAddLink(model, 19, m_resEffectModel, 1.0f);
    HandleClose(model);
  }
}

void CGUnit_C::DetatchResEffectModel() {
  if (!m_resEffectModel) {
    return;
  }

  HMODEL model = GetCharacterModel(0);
  if (model) {
    ModelRemoveLink(model, 19, m_resEffectModel);
    HandleClose(model);
  }
}

void CGUnit_C::ShowPlayerXPGained() {
  unsigned int &xpGain = *reinterpret_cast<unsigned int *>(reinterpret_cast<unsigned char *>(this) + 0x694);
  if (xpGain) {
    unsigned __int64 guid = GetGUID();
    UnitCombatLogShowXPGained(guid, xpGain);

    CGUnit_C *player = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      player->AddWorldXPGainText(xpGain);
    }
    xpGain = 0;
  }
}

int CGUnit_C::GetUnitSize() const {
  FATALASSERT(m_modelData);
  return m_modelData->m_sizeClass;
}

void CGUnit_C::WoundAnimEndHandler() {
  if (m_currentBaseAnimState != 1) {
    m_animFlags |= 2;
    ClearTorsoAnimation(64);
  }
}

void CGUnit_C::DodgeAnimEndHandler() {
  ClearTorsoAnimation(64);
}

void CGUnit_C::AttackAnimEndHandler() {
  CGUnit_C *player = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player && player->GetGUID() != GetGUID()) {
    char buff[256];
    SStrPrintf(buff, sizeof(buff), "%s: attack anim ends", GetUnitName());
    DDGENLOG(GetGUID(), buff, __FILE__, __LINE__);
  }
  CheckPendingVictimFeedback();
  ClearTorsoAnimation(64);
}

void CGUnit_C::CheckPendingVictimFeedback() {
  unsigned int  *self = reinterpret_cast<unsigned int *>(this);
  ANIMQUEUENODE *node = reinterpret_cast<ANIMQUEUENODE *>(self[308]);
  if (!node) {
    return;
  }

  unsigned char   *nodeData = reinterpret_cast<unsigned char *>(node);
  unsigned __int64 victimGUID = *reinterpret_cast<unsigned __int64 *>(nodeData + 32);
  CGUnit_C        *victimPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(victimGUID, __FILE__, __LINE__));
  FATALASSERT(victimPtr != this);

  self[308] = 0;
  if (victimPtr) {
    DoVictimFeedback(reinterpret_cast<ATTACKROUNDINFO *>(nodeData + 16), 1);
  }
  if (*reinterpret_cast<unsigned int *>(nodeData + 88) & 0x1000) {
    if (GetGUID() == ClntObjMgrGetActivePlayer()) {
      CGPlayer_C::ProcessDeferredDamage();
      CGPlayer_C::ProcessDeferredSpellMiss();
    }
  }
  RecycleAnimNode(node);
}

void __fastcall UnitCombatClientInitialize() {
  ClientServices_SetMessageHandler(SMSG_LOG_XPGAIN, OnUnitCombatEvent, 0);
  ClientServices_SetMessageHandler(SMSG_ATTACKERSTATEUPDATEDEBUGINFO, OnUnitCombatEvent, 0);
  ClientServices_SetMessageHandler(SMSG_ATTACKERSTATEUPDATEDEBUGINFOSPELL, OnUnitCombatEvent, 0);
  ClientServices_SetMessageHandler(SMSG_PARTYKILLLOG, OnUnitCombatEvent, 0);
  ClientServices_SetMessageHandler(SMSG_ATTACKERSTATEUPDATEDEBUGINFOSPELLMISS, OnUnitCombatEvent, 0);
  ClientServices_SetMessageHandler(SMSG_ATTACKSTART, OnUnitCombatEvent, 0);
  ClientServices_SetMessageHandler(SMSG_ATTACKSTOP, OnUnitCombatEvent, 0);
  ClientServices_SetMessageHandler(SMSG_ATTACKERSTATEUPDATE, OnUnitCombatEvent, 0);
  ClientServices_SetMessageHandler(SMSG_ATTACKSWING_NOTINRANGE, OnUnitCombatEvent, 0);
  ClientServices_SetMessageHandler(SMSG_ATTACKSWING_BADFACING, OnUnitCombatEvent, 0);
  ClientServices_SetMessageHandler(SMSG_ATTACKSWING_NOTSTANDING, OnUnitCombatEvent, 0);
  ClientServices_SetMessageHandler(SMSG_ATTACKSWING_DEADTARGET, OnUnitCombatEvent, 0);
  ClientServices_SetMessageHandler(SMSG_ATTACKSWING_CANT_ATTACK, OnUnitCombatEvent, 0);
  ClientServices_SetMessageHandler(SMSG_RESISTLOG, OnUnitCombatEvent, 0);
  ClientServices_SetMessageHandler(SMSG_ENCHANTMENTLOG, OnUnitCombatEvent, 0);
  ClientServices_SetMessageHandler(SMSG_MIRRORTIMERDAMAGELOG, OnUnitCombatEvent, 0);
  ClientServices_SetMessageHandler(SMSG_ENVIRONMENTALDAMAGELOG, OnUnitCombatEvent, 0);
  ClientServices_SetMessageHandler(SMSG_DAMAGE_DONE, OnUnitDamageDone, 0);
  ClientServices_SetMessageHandler(SMSG_DAMAGE_TAKEN, OnUnitDamageTaken, 0);
  UnitCombatLogInitialize();
  LoadAnimKitTable();
}

void __fastcall UnitCombatClientShutdown() {
  ClientServices_ClearMessageHandler(SMSG_LOG_XPGAIN);
  ClientServices_ClearMessageHandler(SMSG_ATTACKERSTATEUPDATEDEBUGINFO);
  ClientServices_ClearMessageHandler(SMSG_ATTACKERSTATEUPDATEDEBUGINFOSPELL);
  ClientServices_ClearMessageHandler(SMSG_PARTYKILLLOG);
  ClientServices_ClearMessageHandler(SMSG_ATTACKERSTATEUPDATEDEBUGINFOSPELLMISS);
  ClientServices_ClearMessageHandler(SMSG_ATTACKSTART);
  ClientServices_ClearMessageHandler(SMSG_ATTACKSTOP);
  ClientServices_ClearMessageHandler(SMSG_ATTACKERSTATEUPDATE);
  ClientServices_ClearMessageHandler(SMSG_ATTACKSWING_NOTINRANGE);
  ClientServices_ClearMessageHandler(SMSG_ATTACKSWING_BADFACING);
  ClientServices_ClearMessageHandler(SMSG_ATTACKSWING_NOTSTANDING);
  ClientServices_ClearMessageHandler(SMSG_ATTACKSWING_DEADTARGET);
  ClientServices_ClearMessageHandler(SMSG_ATTACKSWING_CANT_ATTACK);
  ClientServices_ClearMessageHandler(SMSG_RESISTLOG);
  ClientServices_ClearMessageHandler(SMSG_ENCHANTMENTLOG);
  ClientServices_ClearMessageHandler(SMSG_MIRRORTIMERDAMAGELOG);
  ClientServices_ClearMessageHandler(SMSG_ENVIRONMENTALDAMAGELOG);
  ClientServices_ClearMessageHandler(SMSG_DAMAGE_DONE);
  ClientServices_ClearMessageHandler(SMSG_DAMAGE_TAKEN);
  ClientServices_ClearMessageHandler(SMSG_DEBUG_PLAYER_RANGE);
  UnitCombatLogShutdown();
  s_animKitTable.Destroy();
}

void LOGBASE::PI(CDataStore &, int) {
}

void LOGBASE::UI(CDataStore &) {
}

void CGUnit_C::OnCombatModeTimer() {
  unsigned __int64 lockedTarget = CGGameUI::GetLockedTarget();
  CGUnit_C        *victimPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(lockedTarget, __FILE__, __LINE__));
  FATALASSERT(victimPtr);

  NTempest::C3Vector victimPosition;
  NTempest::C3Vector position;
  victimPtr->GetPosition(victimPosition);
  position = GetPosition();
  float rangeSquared = (victimPosition - position).SquaredMag();
  float maxRange = g_combatModeMaxDistance->GetFloat();

  if (m_unit->health > 0 && !(m_unit->flags & 0x2000) && victimPtr->m_unit->health > 0 && CanAttack(victimPtr) && rangeSquared <= maxRange * maxRange)
  {
    float attackRange =
        victimPtr->m_unit->combatReach + victimPtr->m_unit->boundingRadius + m_unit->combatReach + m_unit->boundingRadius + 1.3333334f;
    bool inPosition = false;
    if (rangeSquared <= attackRange * attackRange) {
      if (rangeSquared < 0.33333334f * 0.33333334f) {
        inPosition = true;
      } else {
        float desiredFacing = CalculateFacingTo(position, victimPosition);
        float facing = GetFacing();
        float minFacing = desiredFacing - 1.2217305f;
        float maxFacing = desiredFacing + 1.2217305f;
        while (minFacing < 0.0f) {
          minFacing += 6.2831855f;
          maxFacing += 6.2831855f;
        }
        while (facing < minFacing) {
          facing += 6.2831855f;
        }
        inPosition = facing <= maxFacing;
      }

      if (inPosition) {
        if (!*reinterpret_cast<const int *>(reinterpret_cast<const unsigned char *>(this) + 0x698) && !m_combat.IsAttacking() &&
            !m_combat.AttackBeenSent())
        {
          *reinterpret_cast<unsigned int *>(reinterpret_cast<unsigned char *>(this) + 0x4E4) &= ~0xC0u;
          AttackUnit(victimPtr);
        }
      } else {
        typedef void (CGUnit_C::*BadFacingFn)(unsigned __int64);
        BadFacingFn onBadAttackFacing;
        void      **vtable = *reinterpret_cast<void ***>(this);
        memcpy(&onBadAttackFacing, &vtable[0xCC / 4], sizeof(onBadAttackFacing));
        (this->*onBadAttackFacing)(lockedTarget);
      }
    } else {
      typedef void (CGUnit_C::*BadPositionFn)(unsigned __int64, float);
      BadPositionFn onBadAttackPosition;
      void        **vtable = *reinterpret_cast<void ***>(this);
      memcpy(&onBadAttackPosition, &vtable[0xD4 / 4], sizeof(onBadAttackPosition));
      (this->*onBadAttackPosition)(lockedTarget, attackRange);
    }

    if ((!inPosition || rangeSquared > attackRange * attackRange) && (m_combat.IsAttacking() || m_combat.AttackBeenSent())) {
      typedef void (CGUnit_C::*StopAttackFn)();
      StopAttackFn stopAttack;
      void       **vtable = *reinterpret_cast<void ***>(this);
      memcpy(&stopAttack, &vtable[0xB4 / 4], sizeof(stopAttack));
      (this->*stopAttack)();
    }

    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      player->ResetCombatModeTimer(0);
    }
    return;
  }

  if (GetType() & TYPE_PLAYER) {
    static_cast<CGPlayer_C *>(this)->SetCombatMode(0);
  }
  if (m_combat.IsAttacking() || m_combat.AttackBeenSent()) {
    typedef void (CGUnit_C::*StopAttackFn)();
    StopAttackFn stopAttack;
    void       **vtable = *reinterpret_cast<void ***>(this);
    memcpy(&stopAttack, &vtable[0xB4 / 4], sizeof(stopAttack));
    (this->*stopAttack)();
  }
}

void CGUnit_C::AttackUnit(CGUnit_C *newVictim) {
  unsigned __int64 currentVictim = m_combat.IsAttacking();
  if (currentVictim && currentVictim != newVictim->GetGUID()) {
    typedef void (CGUnit_C::*StopAttackFn)();
    StopAttackFn stopAttack;
    void       **vtable = *reinterpret_cast<void ***>(this);
    memcpy(&stopAttack, &vtable[0xB4 / 4], sizeof(stopAttack));
    (this->*stopAttack)();
    currentVictim = 0;
  }

  unsigned __int64 victim = newVictim->GetGUID();
  CGGameUI::Target(victim, 0);

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player || !player->CanEngageTarget(newVictim)) {
    return;
  }

  int       castingSpell = *reinterpret_cast<const int *>(reinterpret_cast<const unsigned char *>(this) + 0x698);
  SpellRec *spell = castingSpell ? g_spellDB.GetRecord(castingSpell) : 0;
  if (spell && (spell->m_attributesEx & 8)) {
    return;
  }

  victim = currentVictim ? currentVictim : newVictim->GetGUID();
  if (!m_combat.IsAttacking() && !m_combat.AttackBeenSent()) {
    OnAttackSwing(victim, GetTickCount());
  }
}

void CGUnit_C::AddVictimDeathHold(CGUnit_C *victimPtr) {
  SetMeleeDeathHold(victimPtr);

  char buff[256];
  SStrPrintf(buff, sizeof(buff), "(%s) adding melee death hold on (%s)", GetUnitName(), victimPtr->GetUnitName());
  victimPtr->DDADDLOG(GetGUID(), buff, __FILE__, __LINE__);
}

void CGUnit_C::SetMeleeDeathHold(CGUnit_C *victimPtr) {
  ClearMeleeDeathHold();
  m_meleeTargetDeathHold = victimPtr ? victimPtr->GetGUID() : 0;
}

void CGUnit_C::ClearMeleeDeathHold() {
  CGUnit_C *victimPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(m_meleeTargetDeathHold, __FILE__, __LINE__));
  m_meleeTargetDeathHold = 0;
  if (victimPtr) {
    char buff[256];
    SStrPrintf(buff, sizeof(buff), "(%s) releasing melee death hold on (%s)", GetUnitName(), victimPtr->GetUnitName());
    victimPtr->DDDELLOG(GetGUID(), buff, __FILE__, __LINE__);
  }
}
