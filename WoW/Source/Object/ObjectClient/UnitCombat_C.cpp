#include <WowConst.h>
#include <MapDefs.h>

#include "Object/ObjectClient/Unit_C.h"

#include <math.h>

#include <Base/CDataStore.h>
#include <Os/OsTime.h>
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
#include "Object/ItemStats.h"
#include "Object/ObjectClient/Player_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "SoundInterface/SoundInterface.h"
#include "UIUtil/Camera.h"
#include "Ui/GameUI.h"
#include "Ui/WorldFrame.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

extern CVar *g_combatModeMaxDistance;

void UnitCombatLogShowXPGained(const DWORDLONG &victim, int xp);
void UnitCombatLogXPGain(const DWORDLONG &victim, CDataStore *msg, UINT count);
void UnitCombatLog(const ATTACKROUNDINFO &roundInfo);
void UnitCombatLog(const SPELLLOG &log);
void UnitCombatLog(const SPELLMISSLOG &log);
void UnitCombatLog(const MIRRORTIMERDAMAGE &log);
void UnitCombatLog(const ENVIRONMENTALDAMAGE &log);
void UnitCombatLogHeartbeatResist(const RESISTLOG &log);
void UnitCombatLogEnchantment(const ENCHANTMENTLOG &log);
void UnitCombatLogPartyKill(const PARTYKILLLOG &log);
void UnitCombatLogInitialize();
void UnitCombatLogShutdown();

struct CHANCES {
  UINT seq;
  UINT frequency;
};

struct WEAPONHANDCHANCES {
  UINT                     total;
  TSGrowableArray<CHANCES> chances;

  WEAPONHANDCHANCES() : total(0) {
  }
};

NODEDECL(HITSPRITE) {
  HITSPRITE();
  HITSPRITE(const HITSPRITE &);
  ~HITSPRITE();

  UINT   start;
  UINT   duration;
  HMODEL model;
};

HITSPRITE::~HITSPRITE() {
  if (model) {
    HandleClose(model);
  }
}

struct ANIMKIT : public TSHashObject<ANIMKIT, HASHKEY_NONE> {
  WEAPONHANDCHANCES chancesArray[NUMHANDS];
};

static TSHashTable<ANIMKIT, HASHKEY_NONE> s_animKitTable;
static HASHKEY_NONE                       s_nullHashKey;
static BYTE                               s_didHitConnect[NUM_VICTIMSTATES] = {0, 1, 0, 0, 1, 1, 0, 0, 1};
static ANIM_STATE                         s_attackAnimHitStates[NUMHANDS] = {ANIM_STATE_ATTACK_HIT, ANIM_STATE_ATTACKOFF_HIT};
static ANIM_STATE                         s_attackAnimMissStates[NUMHANDS] = {ANIM_STATE_ATTACK_MISS, ANIM_STATE_ATTACKOFF_MISS};
static ANIMENUMERATION                    s_unarmedSequences[NUMHANDS] = {ANIM_ATTACKUNARMED, ANIM_ATTACKUNARMEDOFF};
static struct {
  LPCSTR          animName;
  ANIMENUMERATION anim;
} s_attackerAnimLookups[7] = {
    { "1H_Main_Swing",            ANIM_ATTACK1H},
    {"1H_Main_Pierce",      ANIM_ATTACK1HPIERCE},
    {    "2HL_Pierce", ANIM_ATTACK2HLOOSEPIERCE},
    {     "2HL_Swing",       ANIM_ATTACK2HLOOSE},
    {     "2HT_Swing",       ANIM_ATTACK2HTIGHT},
    {    "OffH_Swing",           ANIM_ATTACKOFF},
    {   "OffH_Pierce",     ANIM_ATTACKOFFPIERCE}
};

void UnitEffectOneShot(
    UNITEFFECTSPECIALS        effectNumber,
    DWORDLONG                 target,
    const NTempest::C3Vector *attachPos,
    float                     facing,
    float                     scale,
    bool                      forceEffectOnMount
);
int    UnitEffectGetSpecialVisual(UNITEFFECTSPECIALS effectNumber);
HMODEL UnitEffectCreateAuraModel(UINT effectID);
void   SndInterfacePlayWeaponSwooshSound(WEAPONSWING_SOUNDTYPES soundType, int criticalHit, const NTempest::C3Vector &position, int missed);
UINT   SpellGetRangedPrecastHoldAnim(UINT loadAnim);

static UINT FindAnimation(UINT ID) {
  const AttackAnimTypesRec *rec = g_attackAnimTypesDB.GetRecord(ID);
  if (!rec) {
    return INVALID_ANIMATION;
  }
  for (UINT index = 0; index < 7; ++index) {
    if (!SStrCmpI(rec->m_AnimName, s_attackerAnimLookups[index].animName, 0x7FFFFFFF)) {
      return s_attackerAnimLookups[index].anim;
    }
  }
  return INVALID_ANIMATION;
}

static void LoadAnimKitTable() {
  UINT count = g_attackAnimKitsDB.GetNumRecords();
  while (count) {
    const AttackAnimKitsRec *rec = g_attackAnimKitsDB.GetRecordByIndex(--count);
    FATALASSERT(rec);
    FATALASSERT(rec->m_AnimTypeID >= 0);
    FATALASSERT(rec->m_AnimFrequency >= 0);

    ANIMKIT *kit = s_animKitTable.Ptr(rec->m_ItemSubclassID, s_nullHashKey);
    if (!kit) {
      kit = s_animKitTable.New(rec->m_ItemSubclassID, s_nullHashKey, 0, 0);
    }
    FATALASSERT(rec->m_WhichHand < NUMHANDS);
    WEAPONHANDCHANCES &chancesStruct = kit->chancesArray[rec->m_WhichHand];
    CHANCES           *chance = chancesStruct.chances.New();
    chance->frequency = rec->m_AnimFrequency;
    chance->seq = FindAnimation(rec->m_AnimTypeID);
    chancesStruct.total += rec->m_AnimFrequency;
  }
}

void ATTACKROUNDINFO::PI(CDataStore &msg, int debug) const {
}

void ATTACKROUNDINFO::UI(CDataStore &msg) {
  msg.Get(flags);
  msg.Get(attacker);
  msg.Get(victim);
  msg.Get(dmg.totalDamage);

  BYTE d;
  msg.Get(d);
  for (UINT i = 0; i < d; ++i) {
    msg.Get(dmg.damageType[i]);
    msg.Get(dmg.damageFloat[i]);
    msg.Get(dmg.damage[i]);
    msg.Get(dmg.absorbed[i]);
  }

  msg.Get(reinterpret_cast<UINT &>(newVictimState));
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
    for (UINT i = 0; i < 5; ++i) {
      msg.Get(dmg.minDamage[i]);
      msg.Get(dmg.maxDamage[i]);
    }
    msg.Get(netDamageMultiplier);
    msg.Get(scaledDamage);
    msg.Get(scaledArmorReduction);
    msg.Get(maxDamageReduction);
    msg.Get(stunRollFloat);
    msg.Get(stunRollNeededFloat);
    msg.Get(DPSScaler);
    msg.Get(modDamageTaken);
    msg.Get(modDamageDone);
    msg.Get(sinceLastSwing);
  }

  msg.Get(procSpell);
  FATALASSERT(attacker);
  FATALASSERT(victim);
}

void SPELLLOG::PI(CDataStore &msg, int debug) const {
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

void SPELLMISSLOG::PI(CDataStore &msg, int debug) const {
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

void RESISTLOG::PI(CDataStore &msg, int debug) const {
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

void ENCHANTMENTLOG::PI(CDataStore &msg, int debug) const {
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

void ENVIRONMENTALDAMAGE::PI(CDataStore &msg, int debug) const {
}

void ENVIRONMENTALDAMAGE::UI(CDataStore &msg) {
  msg.Get(victim);
  msg.Get(school);
  msg.Get(amount);
}

void MIRRORTIMERDAMAGE::PI(CDataStore &msg, int debug) const {
}

void MIRRORTIMERDAMAGE::UI(CDataStore &msg) {
  msg.Get(victim);
  msg.Get(damage);
  msg.Get(amount);
}

void PARTYKILLLOG::PI(CDataStore &msg, int debug) const {
}

void PARTYKILLLOG::UI(CDataStore &msg) {
  msg.Get(killer);
  msg.Get(victim);
}

BOOL OnUnitCombatEvent(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  FATALASSERT(msg);

  switch (msgId) {
    case SMSG_ATTACKERSTATEUPDATEDEBUGINFO: {
      ATTACKROUNDINFO attackInfo;
      attackInfo.UI(*msg);
      UnitCombatLog(attackInfo);
      return 1;
    }
    case SMSG_ATTACKERSTATEUPDATEDEBUGINFOSPELL: {
      SPELLLOG log(0, 0, 0);
      log.UI(*msg);
      UnitCombatLog(log);
      CGGameUI::ShowCombatFeedback(log);
      return 1;
    }
    case SMSG_ATTACKERSTATEUPDATEDEBUGINFOSPELLMISS: {
      SPELLMISSLOG log(0, 0, 0);
      log.UI(*msg);
      UnitCombatLog(log);
      return 1;
    }
    case SMSG_ATTACKSTART: {
      DWORDLONG attacker;
      DWORDLONG victim;
      msg->Get(attacker);
      msg->Get(victim);
      CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(attacker, __FILE__, __LINE__));
      if (unit) {
        unit->m_combat.ClearAttackSent();
        unit->OnAttackStart(victim);
      }
      return 1;
    }
    case SMSG_ATTACKSTOP: {
      DWORDLONG attacker;
      DWORDLONG victim;
      UINT      nowDead;
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
      CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(attackInfo.attacker, __FILE__, __LINE__));
      if (unit) {
        unit->m_combat.ClearAttackSent();
        unit->OnAttackerStateChange(attackInfo);
        unit->m_hitInformation.attackFlags = 0;
        unit->SetDebugHitRolls(attackInfo);
        UnitCombatLog(attackInfo);
      }
      return 0;
    }
    case SMSG_ATTACKSWING_NOTINRANGE:
    case SMSG_ATTACKSWING_BADFACING:
    case SMSG_ATTACKSWING_NOTSTANDING:
    case SMSG_ATTACKSWING_DEADTARGET:
    case SMSG_ATTACKSWING_CANT_ATTACK: {
      DWORDLONG attacker;
      DWORDLONG victim;
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
      DWORDLONG guid;
      UINT      count;
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

BOOL OnUnitDamageDone(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  DWORDLONG attacker;
  DWORDLONG guid;
  int       damage;
  int       normalCombatDamage;
  UINT      flags;
  int       spellID;

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

BOOL OnUnitCombatEvent(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
BOOL OnUnitDamageDone(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
BOOL OnUnitDamageTaken(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);

BOOL OnUnitDamageTaken(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  DWORDLONG guid;
  UINT      flags;
  BYTE      damage;
  int       amount;

  msg->Get(guid);
  msg->Get(damage);
  msg->Get(amount);
  msg->Get(flags);
  if (amount > 0) {
    CGGameUI::ShowCombatFeedback(guid, amount, damage, flags);
  }
  return 1;
}

void CGUnit_C::SetDebugHitRolls(const ATTACKROUNDINFO &info) {
  m_hitInformation.attackFlags |= 8;
  m_hitInformation.attackInfo = info;
}

BOOL CGUnit_C::IsPreemptableWoundAnimState(UINT state) {
  return state == ANIM_STATE_WOUND || state == ANIM_STATE_CRITICALWOUND || state == ANIM_STATE_PARRY || state == ANIM_STATE_DODGE ||
         state == ANIM_STATE_BLOCK;
}

BOOL CGUnit_C::IsAttackAnimState(UINT state) {
  return state == ANIM_STATE_ATTACK_MISS || state == ANIM_STATE_ATTACK_HIT || state == ANIM_STATE_ATTACKOFF_HIT || state == ANIM_STATE_ATTACKOFF_MISS;
}

bool CGUnit_C::QueueVictimAnim(VICTIMSTATES newState, int unitDead, int criticalHit, UINT victimRoundDuration) {
  if (((1 << newState) & 0x2E) && IsPreemptableWoundAnimState(m_currentTorsoAnimState)) {
    UINT midpoint = m_currentWoundStartTime + (m_currentWoundAnimDuration >> 1);
    int  elapsed = OsGetAsyncTimeMs() - midpoint;
    return !m_currentWoundAnimDuration || elapsed <= 0;
  }

  if (m_currentTorsoAnimState == ANIM_STATE_SPELLPRECAST || m_currentTorsoAnimState == ANIM_STATE_SPELLCAST) {
    return 0;
  }

  if (ObjectIsRendering()) {
    ATTACKROUNDINFO dummy;
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

void CGUnit_C::SetVictimAnimation(VICTIMSTATES newState, int unitDead, int criticalHit, UINT victimRoundDuration, int processNow) {
  FATALASSERT(newState < NUM_VICTIMSTATES);

  if (!processNow && QueueVictimAnim(newState, unitDead, criticalHit, victimRoundDuration)) {
    return;
  }

  UINT sequence = 0;
  switch (newState) {
    case VS_NONE:
      break;
    case VS_WOUND:
    case VS_INTERRUPT:
      PlayUnitSound(criticalHit ? UNITSOUNDTYPE_INJURYCRITICAL : UNITSOUNDTYPE_INJURY, 0);
      sequence = criticalHit ? ANIM_STATE_CRITICALWOUND : ANIM_STATE_WOUND;
      break;
    case VS_DODGE:
    case VS_EVADE:
    case VS_DEFLECT:
      sequence = ANIM_STATE_DODGE;
      break;
    case VS_PARRY:
      sequence = ANIM_STATE_PARRY;
      break;
    case VS_BLOCK:
      sequence = ANIM_STATE_BLOCK;
      break;
    default:
      FATALASSERT(!"bad enum value");
      break;
  }

  if (s_didHitConnect[newState]) {
    PendingPrecastInterrupt(0);
  }

  if (!unitDead || m_unit->health > 0 || m_deathHolds) {
    if (!sequence) {
      m_flags |= 2;
      UpdateBaseAnimation(0);
      return;
    }
  } else {
    if (!(m_animFlags & 0x2000)) {
      OnDeathAnimate();
    }
    sequence = ANIM_STATE_DEAD;
    if (s_didHitConnect[newState]) {
      PlayUnitSound(UNITSOUNDTYPE_DEATH, 0);
    }
  }

  if (m_currentTorsoAnimState == ANIM_STATE_SPELLPRECAST) {
    UINT anim = SpellGetRangedPrecastHoldAnim(GetCurrentTorsoAnim());
    if (anim != RESET_ANIMATION_INDICES0) {
      SetSpellPreCastingAnimation(static_cast<ANIMENUMERATION>(anim));
    }
  }

  SetTorsoAnimation(sequence, victimRoundDuration, 16);
}

BOOL CGUnit_C::SetAttackerAnimation(const ATTACKROUNDINFO *roundInfo, int processNow) {
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

  ANIM_STATE state = (roundInfo->flags & 1) ? s_attackAnimMissStates[hand] : s_attackAnimHitStates[hand];
  if (!SetTorsoAnimation(state, 0, 0)) {
    if (victimPtr) {
      victimPtr->DoVictimFeedback(roundInfo, 1);
    }
    return m_currentTorsoAnimState == ANIM_STATE_SPELLCAST;
  }

  if (victimPtr) {
    AddVictimDeathHold(victimPtr);
  }
  PlayUnitSound((roundInfo->flags & 8) ? UNITSOUNDTYPE_EXERTIONCRITICAL : UNITSOUNDTYPE_EXERTION, 0);

  UINT   sequence = ChooseAnimation(state);
  UINT   seqDuration;
  HMODEL model = m_model;
  if (ModelGetSequenceDuration(model, sequence, &seqDuration)) {
    UINT random = NTempest::CMath::mulhwu_(NTempest::CRandom::uint32_(g_rndSeed), 16);
    UINT scaledDuration = NTempest::CMath::fuint_n((random + 85.0f) * seqDuration * 0.01f);
    UINT attackTime = m_unit->attackRoundBaseTime[(roundInfo->flags >> 9) & 1];
    if (scaledDuration >= attackTime) {
      scaledDuration = attackTime;
    }
    ModelSetObjectTimeScale(model, 0, static_cast<float>(seqDuration) / static_cast<float>(scaledDuration), 0);
  }
  return 1;
}

UINT CGUnit_C::GetAttackerAnimEx(COMBATHAND hand, const VirtualItemInfo *itemInfo) const {
  FATALASSERT(hand < NUMHANDS);
  FATALASSERT(itemInfo);

  ANIMKIT *kit = s_animKitTable.Ptr(itemInfo->m_subclassID, s_nullHashKey);
  if (!kit) {
    return INVALID_ANIMATION;
  }

  const WEAPONHANDCHANCES *chancesStruct = &kit->chancesArray[hand];
  UINT                     count = chancesStruct->chances.Count();
  if (!count) {
    return INVALID_ANIMATION;
  }

  UINT dice = NTempest::CRandom::dice_(chancesStruct->total + 1, g_rndSeed);
  UINT accumulated = 0;
  for (UINT i = 0; i < count; ++i) {
    accumulated += chancesStruct->chances[i].frequency;
    if (dice <= accumulated) {
      return chancesStruct->chances[i].seq;
    }
  }
  return INVALID_ANIMATION;
}

UINT CGUnit_C::DetermineAttackerSequence(COMBATHAND hand) const {
  FATALASSERT(hand < NUMHANDS);
  static const ANIMENUMERATION s_anims[WEAPONATTACKSEQ_RIFLE + 1] = {
      ANIM_ATTACK2HTIGHT, ANIM_ATTACK2HLOOSE, ANIM_ATTACKBOW, ANIM_ATTACK1H, ANIM_ATTACKRIFLE
  };

  const VirtualItemInfo *itemInfo = GetVirtualItem(g_monsterHands[hand], 0);

  if (!itemInfo || itemInfo->m_classID != 2) {
    UINT sequence = s_unarmedSequences[hand];
    if (!ModelHasSequenceId(m_model, sequence)) {
      PrintAttackSeqErrorMsg(sequence, ANIM_ATTACK1H);
      return ANIM_ATTACK1H;
    }
    return sequence;
  }

  UINT sequence = GetAttackerAnimEx(hand, itemInfo);
  if (sequence == INVALID_ANIMATION || !ModelHasSequenceId(m_model, sequence)) {
    WEAPONATTACKSEQ weaponSeq = ClientDBGetWeaponSubclassWeaponSeq(itemInfo->m_subclassID);
    if (weaponSeq <= WEAPONATTACKSEQ_BOW || weaponSeq == WEAPONATTACKSEQ_RIFLE) {
      sequence = s_anims[weaponSeq];
    } else {
      sequence = hand == COMBAT_MAINHAND ? ANIM_ATTACK1H : ANIM_ATTACKOFF;
    }
  }

  if (!ModelHasSequenceId(m_model, sequence)) {
    PrintAttackSeqErrorMsg(sequence, ANIM_ATTACKUNARMED);
    return ANIM_ATTACKUNARMED;
  }
  return sequence;
}

UINT CGUnit_C::DetermineParrySequence() const {
  static const ANIMENUMERATION s_anims[NUM_WEAPONPARRYSEQS] = {ANIM_PARRY2HTIGHT, ANIM_PARRY2HLOOSE, ANIM_PARRY1H, ANIM_STAND};

  const VirtualItemInfo *itemInfo = GetVirtualItem(VIRTUAL_MONSTER_SLOT_MAINHAND, 0);
  if (!itemInfo || itemInfo->m_classID != 2) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "NOWEAPONPARRY|%d|0x%016I64X", m_obj->m_entryID, m_obj->m_guid);
    return ANIM_PARRYUNARMED;
  }

  WEAPONPARRYSEQ seq = ClientDBGetWeaponSubclassParrySeq(itemInfo->m_subclassID);
  FATALASSERT(seq < (sizeof(s_anims) / sizeof(s_anims[0])));
  ANIMENUMERATION anim = s_anims[seq];
  return anim ? anim : GetStandStateAnim(0);
}

void CGUnit_C::SetFingersSeq(HMODEL charModel, UINT sequence, UINT startFinger, UINT lastFinger) {
  FATALASSERT(charModel);
  for (UINT finger = startFinger; finger <= lastFinger; ++finger) {
    if (ModelLockObjectSequence(charModel, finger, 0)) {
      if (ModelSetSequence(charModel, sequence, finger, 4)) {
        ModelLockObjectSequence(charModel, finger, 1);
      }
    }
  }
}

void CGUnit_C::ResetFingersSeq(HMODEL charModel, UINT startFinger, UINT lastFinger) {
  FATALASSERT(charModel);
  for (UINT finger = startFinger; finger <= lastFinger; ++finger) {
    if (ModelLockObjectSequence(charModel, finger, 0)) {
      ModelMatchSequence(charModel, finger, 4, 6);
    }
  }
}

void CGUnit_C::SetHandState(HMODEL model, const VirtualItemInfo *item, UINT startFinger, UINT lastFinger) {
  if (item && !m_sheatheReasons && ClientDBWeaponSubclassSetsFingerSeq(item->m_subclassID)) {
    SetFingersSeq(model, ANIM_HANDS_CLOSED, startFinger, lastFinger);
    if (m_paperDollModel) {
      SetFingersSeq(m_paperDollModel, ANIM_HANDS_CLOSED, startFinger, lastFinger);
    }
  } else {
    ResetFingersSeq(model, startFinger, lastFinger);
    if (m_paperDollModel) {
      ResetFingersSeq(m_paperDollModel, startFinger, lastFinger);
    }
  }
}

void CGUnit_C::SetHandsState(HMODEL model) {
  if (!(m_flags & 4)) {
    ResetFingersSeq(model, 8, 17);
    return;
  }

  if (m_unit->weaponMode == WEAPONMODE_RANGEDMODE) {
    const VirtualItemInfo *item = GetVirtualItem(VIRTUAL_MONSTER_SLOT_RANGED, 0);
    if (item) {
      if (item->m_inventoryType == INDEX_THROWN_TYPE) {
        SetHandState(model, item, 8, 12);
      } else {
        SetHandState(model, item, 13, 17);
      }
    }
  } else {
    const VirtualItemInfo *left = GetVirtualItem(VIRTUAL_MONSTER_SLOT_MAINHAND, 0);
    const VirtualItemInfo *right = GetVirtualItem(VIRTUAL_MONSTER_SLOT_OFFHAND, 0);
    SetHandState(model, left, 8, 12);
    SetHandState(model, right, 13, 17);
  }
}

void CGUnit_C::DetermineReadySequence(bool forceNormal) {
  if (!(m_flags & 4)) {
    m_readySequence = ANIM_READY2HTIGHT;
    return;
  }

  UINT weaponMode = m_unit->weaponMode;
  if (forceNormal) {
    weaponMode = WEAPONMODE_NORMALMODE;
  }

  const VirtualItemInfo *itemInfo = 0;

  switch (weaponMode) {
    case WEAPONMODE_NORMALMODE:
      itemInfo = GetVirtualItem(VIRTUAL_MONSTER_SLOT_MAINHAND, 0);
      break;

    case WEAPONMODE_RANGEDMODE:
      itemInfo = GetVirtualItem(VIRTUAL_MONSTER_SLOT_RANGED, 0);
      break;

    case WEAPONMODE_SHEATHEDMODE:
      break;
  }

  ANIMENUMERATION sequence = ANIM_READYUNARMED;
  if (itemInfo && itemInfo->m_classID == 2) {
    static const ANIMENUMERATION s_anims[NUM_WEAPONREADYSEQS] = {ANIM_READY2HTIGHT, ANIM_READY2HLOOSE, ANIM_READY1H,
                                                                 ANIM_READYBOW,     ANIM_READYRIFLE,   ANIM_READYTHROWN};
    WEAPONREADYSEQ               readySeq = ClientDBGetWeaponSubclassReadySeq(itemInfo->m_subclassID);
    FATALASSERT(readySeq < NUM_WEAPONREADYSEQS);
    sequence = s_anims[readySeq];
  }

  if (m_readySequence != static_cast<UINT>(sequence)) {
    m_readySequence = sequence;
    if (m_currentBaseAnimState == ANIM_STATE_ATTACK_READY) {
      UpdateBaseAnimation(0);
    }
  }
}

void CGUnit_C::UpdateReadyAnim(const ItemStats *stats) {
  if (stats && stats->m_class == 2) {
    DetermineReadySequence(false);
    if (m_currentTorsoAnimState == ANIM_STATE_ATTACK_READY) {
      UpdateBaseAnimation(0);
    }
  }
}

UINT CGUnit_C::DetermineWoundSequence() const {
  return 9;
}

void CGUnit_C::HandleCombatAnimEvent(LPCSTR eventName, DWORD value, const NTempest::C3Vector &position) {
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
        UINT index = SStrToInt(eventName + 3);
        if (index < 4) {
          victimPtr->SetCustomAttackSound(m_soundData->m_customAttack[index], position);
        }
      }
      // Fall through.
    case 0x48414324:  // $CAH
      if (m_currentBaseAnimState == ANIM_STATE_SPELLCAST) {
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
      CGUnit_C::PlayDeathThudCameraShake();
      break;

    case 0x50574224: {  // $BWP
      m_flags = m_flags & ~0xC00u | 0x400;
      UINT currentTime = OsGetAsyncTimeMsPrecise();
      if (m_animEndTime > currentTime) {
        UINT duration = m_animStartTime + m_animBaseDuration - currentTime;
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

void CGUnit_C::OnAttackSwing(DWORDLONG victimGUID, UINT clientTimeStamp) {
  if (m_combat.StopBeenSent() || (!m_combat.IsAttacking() && !m_combat.AttackBeenSent())) {
    CDataStore msg;
    msg.Put(static_cast<int>(CMSG_ATTACKSWING));
    msg.Put(victimGUID);
    msg.Finalize();
    ClientServices_Send(&msg);
    m_combat.SetAttackSent(victimGUID);
  }
}

void CGUnit_C::StopAttack() {
  CDataStore message;
  message.Put(static_cast<int>(CMSG_ATTACKSTOP));
  message.Finalize();
  ClientServices_Send(&message);
  m_combat.SetStopSent(1);
}

void CGUnit_C::OnAttackStart(DWORDLONG victim) {
  FATALASSERT(victim);

  m_combat.SetAttacking(victim);
  m_combat.ClearAttackSent();
  m_hitInformation.attackFlags = 0;

  CGUnit_C *victimPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(victim, __FILE__, __LINE__));
  if (victimPtr) {
    victimPtr->OnGetAttacked(GetGUID());
  }
}

void CGUnit_C::OnAttackStop(DWORDLONG previousTarget, int nowDead) {
  m_hitInformation.attackFlags = 0;
  m_combat.StopAttack();
  m_combat.ClearAttackSent();
  m_combat.SetStopSent(0);

  if (!(g_seqInformation[m_currentTorsoAnimState].flags & 0x10)) {
    UpdateBaseAnimation(0);
  }

  m_hitInformation.attackFlags = 0;
  if (previousTarget && nowDead) {
    CGUnit_C *victimPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(previousTarget, __FILE__, __LINE__));
    if (victimPtr) {
      m_flags |= 1;
      NTempest::C3Vector position;
      NTempest::C3Vector victimPosition;
      GetPosition(position);
      victimPtr->GetPosition(victimPosition);
      m_forcedDisplayFacing = UnitCalculateFacingTo(position, victimPosition);
    }
  }
}

void CGUnit_C::OnAttackerStateChange(const ATTACKROUNDINFO &roundInfo) {
  m_hitInformation.attackFlags = 0;

  CGUnit_C *victimPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(roundInfo.victim, __FILE__, __LINE__));
  if (victimPtr) {
    victimPtr->AdjustVictimState(const_cast<ATTACKROUNDINFO *>(&roundInfo));
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
    UINT *combat = reinterpret_cast<UINT *>(&m_combat);
    combat[0] = 0;
    combat[1] = 0;
    combat[2] = 0;
    combat[3] = 0;
  }

  if (roundInfo.victim == ClntObjMgrGetActivePlayer() && !CGGameUI::GetLockedTarget()) {
    CGGameUI::Target(GetGUID(), 0);
  }
}

void CGUnit_C::HandleMirrorTimerDamage(const MIRRORTIMERDAMAGE &log) {
  UnitCombatLog(log);
}

void CGUnit_C::PlayDeathThudCameraShake() const {
  CGWorldFrame *worldFrame = CGWorldFrame::GetActive();
  if (worldFrame) {
    CGCamera *camera = worldFrame->Camera();
    if (camera) {
      FATALASSERT(m_modelData);
      int shakeSize = m_modelData->m_deathThudShakeSize;
      if (shakeSize) {
        camera->AddShake(shakeSize, GetPosition());
      }
    }
  }
}

void CGUnit_C::DoVictimFeedback(const ATTACKROUNDINFO *roundInfo, int showAnimation) {
  FATALASSERT(roundInfo);

  CGUnit_C *attackerPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(roundInfo->attacker, __FILE__, __LINE__));
  if (!(roundInfo->flags & 0x1000) && attackerPtr) {
    attackerPtr->SetMeleeDeathHold(0);
  }

  switch (roundInfo->newVictimState) {
    case VS_PARRY:
      PlayParrySound(0, roundInfo, GetPosition());
      break;
    case VS_BLOCK:
      PlayParrySound(1, roundInfo, GetPosition());
      break;
    case VS_IMMUNE:
      SndInterfacePlayImmuneSound(GetPosition());
      break;
    default:
      if (roundInfo->flags & 0x10000) {
        SndInterfacePlayAbsorbedSound(GetPosition());
      }
      break;
  }

  if (roundInfo->flags & 2) {
    if (showAnimation) {
      SetVictimAnimation(roundInfo->newVictimState, roundInfo->flags & 4, roundInfo->flags & 8, roundInfo->victimRoundDuration, 0);
    }
    if (roundInfo->dmg.totalDamage && (roundInfo->newVictimState == VS_WOUND || roundInfo->newVictimState == VS_INTERRUPT)) {
      ShowBloodSpurt(attackerPtr, roundInfo->flags & 0x400);
    }
  }

  if (!(m_flags & 2)) {
    m_flags |= 2;
    if (m_unit->health <= 0) {
      UpdateBaseAnimation(0);
    }
  }

  if (m_flags & 0x8000) {
    if (m_customAttackSound == -1) {
      if (roundInfo->newVictimState == VS_DEFLECT) {
        SndInterfacePlayDeflectedSound(GetPosition());
      } else {
        COMBATHAND hand = (roundInfo->flags & 0x200) ? COMBAT_OFFHAND : COMBAT_MAINHAND;
        PlayImpactSound(roundInfo->attacker, roundInfo->flags & 8, hand);
      }
    } else {
      PlayCustomAttackSound(m_customAttackSound, m_customAttackPosition);
      m_customAttackSound = -1;
    }
    m_flags &= ~0x8000u;
  }

  PerformSpellProcImpact(roundInfo->procSpell);
  if (!(roundInfo->flags & 0x1000)) {
    CGGameUI::ShowCombatFeedback(roundInfo);
  }
  if (attackerPtr) {
    attackerPtr->PerformLevelUpAnim(0);
  }
  if (!(roundInfo->flags & 0x1000)) {
    ShowWorldText(roundInfo);
  }
}

void CGUnit_C::AdjustVictimState(ATTACKROUNDINFO *roundInfo) {
  if (roundInfo->newVictimState == VS_PARRY) {
    const VirtualItemInfo *item = GetAttackingWeapon(COMBAT_MAINHAND);
    if (!item || !m_unit->virtualItemDisplay[VIRTUAL_MONSTER_SLOT_MAINHAND]) {
      roundInfo->flags |= 0x40000;
      roundInfo->newVictimState = VS_DEFLECT;
    }
  } else if (roundInfo->newVictimState == VS_BLOCK) {
    const VirtualItemInfo *item = GetVirtualItem(VIRTUAL_MONSTER_SLOT_OFFHAND, 1);
    if (!item || !m_unit->virtualItemDisplay[VIRTUAL_MONSTER_SLOT_OFFHAND]) {
      roundInfo->flags &= ~0x40000u;
      roundInfo->newVictimState = VS_DEFLECT;
    }
  }
}

MISS_REASON CGUnit_C::AdjustVictimState(MISS_REASON reason) {
  if (reason == MISS_PARRIED) {
    if (!GetAttackingWeapon(COMBAT_MAINHAND) || !m_unit->virtualItemDisplay[VIRTUAL_MONSTER_SLOT_MAINHAND]) {
      return MISS_DEFLECTED;
    }
  } else if (reason == MISS_BLOCKED) {
    const VirtualItemInfo *item = GetVirtualItem(VIRTUAL_MONSTER_SLOT_OFFHAND, 1);
    if (!item || !m_unit->virtualItemDisplay[VIRTUAL_MONSTER_SLOT_OFFHAND]) {
      return MISS_DEFLECTED;
    }
  }
  return reason;
}

void CGUnit_C::ShowWorldText(const ATTACKROUNDINFO *roundInfo) {
  DWORDLONG activePlayer = ClntObjMgrGetActivePlayer();
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
  const SpellRec *spellRec = g_spellDB.GetRecord(spell);
  if (!spellRec) {
    return;
  }

  const SpellVisualRec *visualRec = g_spellVisualDB.GetRecord(spellRec->m_spellVisualID);
  if (!visualRec) {
    return;
  }

  const SpellVisualKitRec *impactKit = g_spellVisualKitDB.GetRecord(visualRec->m_impactKit);
  if (impactKit) {
    SetImpactKitEffect(spell, this, impactKit, 1);
  }
}

void CGUnit_C::ShowBloodSpurt(CGUnit_C *attacker, int crushingBlow) {
  const UnitBloodRec *bloodRec = GetBloodRecord();
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

  const SpellVisualEffectNameRec *effectRec = g_spellVisualEffectNameDB.GetRecord(effectID);
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
  ProcessQuestItemMessages();
  ShowPlayerXPGained();
  FATALASSERT(!m_deathHolds);
  FATALASSERT(!IsDeathFlagSet());
  m_animFlags |= 0x2000;
  CheckPendingVictimFeedback();
  PurgeAnimNodes(0);

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->OnUnitDeath(GetGUID());
  }

  PlayUnitSound(UNITSOUNDTYPE_DEATH, 0);
  UpdateBaseAnimation(0);
}

void CGUnit_C::InitializeResEffectModel() {
  if (IsA(ID_PLAYER)) {
    CGUnit_C *activePlayer = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (activePlayer->CanAssist(this)) {
      ClearResEffectModel();

      static int effectVisualID = UnitEffectGetSpecialVisual(SPECIALEFFECT_RES_EFFECT);

      m_resEffectModel = UnitEffectCreateAuraModel(effectVisualID);
      AttachResEffectModel();
    }
  }
}

void CGUnit_C::ClearResEffectModel() {
  DetatchResEffectModel();
  if (m_resEffectModel) {
    HandleClose(m_resEffectModel);
  }
  m_resEffectModel = 0;
}

void CGUnit_C::AttachResEffectModel() {
  if (m_resEffectModel) {
    HMODEL model = GetCharacterModel(0);
    ModelAddLink(model, ATTACH_UNITEFFECT_BASE, m_resEffectModel, 1.0f);
    HandleClose(model);
  }
}

void CGUnit_C::DetatchResEffectModel() {
  if (m_resEffectModel) {
    HMODEL model = GetCharacterModel(0);
    ModelRemoveLink(model, ATTACH_UNITEFFECT_BASE, m_resEffectModel);
    HandleClose(model);
  }
}

void CGUnit_C::ShowPlayerXPGained() {
  if (m_accumulatedXPDrop) {
    UnitCombatLogShowXPGained(GetGUID(), m_accumulatedXPDrop);

    CGUnit_C *player = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      player->AddWorldXPGainText(m_accumulatedXPDrop);
    }
    m_accumulatedXPDrop = 0;
  }
}

const VirtualItemInfo *CGUnit_C::GetParryingItem(bool ignoreMainHand) const {
  const VirtualItemInfo *item = GetVirtualItem(VIRTUAL_MONSTER_SLOT_MAINHAND, 0);
  if (!ignoreMainHand && item && item->m_classID == 2) {
    return item;
  }

  item = GetVirtualItem(VIRTUAL_MONSTER_SLOT_OFFHAND, 0);
  if (item && item->m_classID != 2 && item->m_classID != 4) {
    return 0;
  }
  return item;
}

const VirtualItemInfo *CGUnit_C::GetDefendingItem() const {
  return 0;
}

const VirtualItemInfo *CGUnit_C::GetAttackingWeapon(COMBATHAND hand) const {
  const VirtualItemInfo *item = GetVirtualItem(hand != COMBAT_MAINHAND, 0);
  return WeaponAttached(hand) && item && item->m_classID == 2 ? item : 0;
}

int CGUnit_C::GetUnitSize() const {
  FATALASSERT(m_modelData);
  return m_modelData->m_sizeClass;
}

void CGUnit_C::WoundAnimEndHandler() {
  if (m_currentBaseAnimState != ANIM_STATE_DEAD) {
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
  ANIMQUEUENODE *node = m_currentDamageInfo;
  if (!node) {
    return;
  }

  BYTE     *nodeData = reinterpret_cast<BYTE *>(node);
  DWORDLONG victimGUID = *reinterpret_cast<DWORDLONG *>(nodeData + 32);
  CGUnit_C *victimPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(victimGUID, __FILE__, __LINE__));
  FATALASSERT(victimPtr != this);

  m_currentDamageInfo = 0;
  if (victimPtr) {
    DoVictimFeedback(reinterpret_cast<ATTACKROUNDINFO *>(nodeData + 16), 1);
  }
  if (*reinterpret_cast<UINT *>(nodeData + 88) & 0x1000) {
    if (GetGUID() == ClntObjMgrGetActivePlayer()) {
      CGPlayer_C::ProcessDeferredDamage();
      CGPlayer_C::ProcessDeferredSpellMiss();
    }
  }
  RecycleAnimNode(node);
}

void UnitCombatClientInitialize() {
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

void UnitCombatClientShutdown() {
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

void CGUnit_C::OnCombatModeTimer() {
  DWORDLONG lockedTarget = CGGameUI::GetLockedTarget();
  CGUnit_C *victimPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(lockedTarget, __FILE__, __LINE__));
  FATALASSERT(victimPtr);

  NTempest::C3Vector victimPosition;
  NTempest::C3Vector position;
  victimPtr->GetPosition(victimPosition);
  position = GetPosition();
  float rangeSquared = (victimPosition - position).SquaredMag();
  float maxRange = g_combatModeMaxDistance->GetFloat();

  if (m_unit->health > 0 && !(m_unit->flags & 0x2000) && victimPtr->m_unit->health > 0 && CanAttack(victimPtr) && rangeSquared <= maxRange * maxRange)
  {
    float attackRange = victimPtr->m_unit->weaponReach + victimPtr->m_unit->combatReach + m_unit->weaponReach + m_unit->combatReach + 1.3333334f;
    bool  inPosition = false;
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
        if (!m_castingSpell && !m_combat.IsAttacking() && !m_combat.AttackBeenSent()) {
          m_flags &= ~0xC0u;
          AttackUnit(victimPtr);
        }
      } else {
        OnBadAttackFacing(lockedTarget);
      }
    } else {
      OnBadAttackPosition(lockedTarget, attackRange);
    }

    if ((!inPosition || rangeSquared > attackRange * attackRange) && (m_combat.IsAttacking() || m_combat.AttackBeenSent())) {
      CGUnit_C::StopAttack();
    }

    CGPlayer_C *player;
    if (GetGUID() == ClntObjMgrGetActivePlayer()) {
      player = static_cast<CGPlayer_C *>(this);
    } else {
      player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    }
    if (player) {
      player->ResetCombatModeTimer(0);
    }
    return;
  }

  if (GetType() & TYPE_PLAYER) {
    static_cast<CGPlayer_C *>(this)->SetCombatMode(0);
  }
  if (m_combat.IsAttacking() || m_combat.AttackBeenSent()) {
    CGUnit_C::StopAttack();
  }
}

void CGUnit_C::AttackUnit(CGUnit_C *newVictim) {
  DWORDLONG currentVictim = m_combat.IsAttacking();
  if (currentVictim && currentVictim != newVictim->GetGUID()) {
    CGUnit_C::StopAttack();
    currentVictim = 0;
  }

  CGGameUI::Target(newVictim->GetGUID(), 0);

  CGPlayer_C *player;
  if (GetGUID() == ClntObjMgrGetActivePlayer()) {
    player = static_cast<CGPlayer_C *>(this);
  } else {
    player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  }
  if (!player || !player->CanEngageTarget(newVictim)) {
    return;
  }

  const SpellRec *spell = m_castingSpell ? g_spellDB.GetRecord(m_castingSpell) : 0;
  if (spell && (spell->m_interruptFlags & 8)) {
    return;
  }

  if (!m_combat.IsAttacking() && !m_combat.AttackBeenSent()) {
    OnAttackSwing(currentVictim ? currentVictim : newVictim->GetGUID(), OsGetAsyncTimeMs());
  }
}

void CGUnit_C::AddVictimDeathHold(CGUnit_C *victimPtr) {
  SetMeleeDeathHold(victimPtr);

  char buff[256];
  SStrPrintf(buff, sizeof(buff), "(%s) adding melee death hold on (%s)", GetUnitName(), victimPtr->GetUnitName());
  victimPtr->DDADDLOG(GetGUID(), buff, __FILE__, __LINE__);
}

void CGUnit_C::SetMeleeDeathHold(const CGUnit_C *victimPtr) {
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
