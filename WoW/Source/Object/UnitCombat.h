#pragma once

#include <stpl.h>

struct CClientObjCreate;

class CDataStore;

enum VICTIMSTATES {
  VS_NONE = 0,
  VS_WOUND = 1,
  VS_DODGE = 2,
  VS_PARRY = 3,
  VS_INTERRUPT = 4,
  VS_BLOCK = 5,
  VS_EVADE = 6,
  VS_IMMUNE = 7,
  VS_DEFLECT = 8,
  NUM_VICTIMSTATES = 9
};

enum UNIT_MIRROR_TIMER {
  UNIT_MIRROR_TIMER_EXHAUSTION = 0,
  UNIT_MIRROR_TIMER_BREATH = 1,
  UNIT_MIRROR_TIMER_FEIGNDEATH = 2,
  NUM_UNIT_MIRROR_TIMERS = 3
};

enum ANIMQUEUETYPE {
  ANIMQUEUE_NONE = 0,
  ANIMQUEUE_ATTACK = 1,
  ANIMQUEUE_WOUND = 2,
  ANIMQUEUE_SITDOWN = 3,
  ANIMQUEUE_SITUP = 4,
  ANIMQUEUE_SLEEPDOWN = 5,
  ANIMQUEUE_SLEEPUP = 6,
  ANIMQUEUE_SITCHAIR = 7,
  ANIMQUEUE_SITCHAIRUP = 8,
  ANIMQUEUE_SITCHAIRLOW = 9,
  ANIMQUEUE_SITCHAIRMEDIUM = 10,
  ANIMQUEUE_SITCHAIRHIGH = 11,
  ANIMQUEUE_DEAD = 12,
  ANIMQUEUE_KNEELDOWN = 13,
  ANIMQUEUE_KNEELUP = 14,
  ANIMQUEUE_NUMTYPES = 15
};

struct DamageData {
  int   totalDamage;
  int   damageType[5];
  UINT  minDamage[5];
  UINT  maxDamage[5];
  float damageFloat[5];
  int   damage[5];
  int   absorbed[5];

  void Clear();
};

inline void DamageData::Clear() {
  totalDamage = 0;
  memset(damageFloat, 0, sizeof(damageFloat));
  memset(damage, 0, sizeof(damage));
  memset(absorbed, 0, sizeof(absorbed));
  memset(minDamage, 0, sizeof(minDamage));
  memset(maxDamage, 0, sizeof(maxDamage));
  for (UINT i = 0; i < 5; ++i) {
    damageType[i] = -1;
  }
}

struct LOGBASE {
  LOGBASE() {
  }

  LOGBASE(const LOGBASE &) {
  }

  ~LOGBASE() {
  }

  virtual void PI(CDataStore &msg, int debug) const = 0;
  virtual void UI(CDataStore &msg) = 0;
};

struct DAMAGELOGBASE : public LOGBASE {
  DAMAGELOGBASE(const DAMAGELOGBASE &other);
  DAMAGELOGBASE(DWORDLONG attacker, DWORDLONG victim);

  DWORDLONG  attacker;
  DWORDLONG  victim;
  float      intellectBonus;
  float      DPSScaler;
  float      modDamageTaken;
  float      modDamageDone;
  float      scaledDamage;
  float      netDamageMultiplier;
  float      maxDamageReduction;
  float      scaledArmorReduction;
  float      hitRollFloat;
  float      hitRollNeededFloat;
  float      critRollFloat;
  float      critRollNeededFloat;
  UINT       flags;
  DamageData dmg;
};

struct ATTACKROUNDINFO : public DAMAGELOGBASE {
  UINT         armorReduction;
  VICTIMSTATES newVictimState;
  UINT         victimRoundDuration;
  float        dodgeRollFloat;
  float        dodgeRollNeededFloat;
  float        parryRollFloat;
  float        parryRollNeededFloat;
  float        blockRollFloat;
  float        blockRollNeededFloat;
  float        stunRollFloat;
  float        stunRollNeededFloat;
  UINT         delayTime;
  UINT         spellDamageAdded;
  UINT         spellAddedDamage;
  UINT         sinceLastSwing;
  float        dualWieldHitRollFloat;
  float        dualWieldHitRollNeededFloat;
  int          procSpell;

  ATTACKROUNDINFO();
  virtual void PI(CDataStore &msg, int debug) const;
  virtual void UI(CDataStore &msg);
};

struct SPELLLOG : public DAMAGELOGBASE {
  SPELLLOG(const SPELLLOG &);
  SPELLLOG(DWORDLONG attacker, DWORDLONG victim, int spellID);
  SPELLLOG(DWORDLONG attacker, UINT spellID);
  SPELLLOG(
      DWORDLONG attacker,
      DWORDLONG victim,
      UINT      spellID,
      float     intellectBonus,
      float     DPSScaler,
      UINT      damageType,
      UINT      auraEffectID,
      float     resistanceCoefficient
  );

  UINT  auraEffectID;
  UINT  spellID;
  UINT  damageType;
  float resistanceCoefficient;

  virtual void PI(CDataStore &msg, int debug) const;
  virtual void UI(CDataStore &msg);
};

struct SPELLMISSLOG : public LOGBASE {
  SPELLMISSLOG(const SPELLMISSLOG &);
  SPELLMISSLOG(DWORDLONG attacker, DWORDLONG victim, UINT spellID);

  DWORDLONG attacker;
  DWORDLONG victim;
  UINT      spellID;
  UINT      reason;
  float     hitRoll;
  float     hitRollNeeded;
  float     dodgeRoll;
  float     dodgeRollNeeded;
  float     parryRoll;
  float     parryRollNeeded;
  float     blockRoll;
  float     blockRollNeeded;
  UINT      flags;

  virtual void PI(CDataStore &msg, int debug) const;
  virtual void UI(CDataStore &msg);
};

struct RESISTLOG : public LOGBASE {
  RESISTLOG() {
  }

  RESISTLOG(const RESISTLOG &);

  DWORDLONG attacker;
  DWORDLONG victim;
  int       spell;
  float     resistRollNeeded;
  float     resistRoll;
  int       flags;
  int       castLevel;

  virtual void PI(CDataStore &msg, int debug) const;
  virtual void UI(CDataStore &msg);
};

struct ENCHANTMENTLOG : public LOGBASE {
  DWORDLONG attacker;
  DWORDLONG victim;
  int       enchantment;
  int       itemID;
  int       flags;

  virtual void PI(CDataStore &msg, int debug) const;
  virtual void UI(CDataStore &msg);
};

struct ENVIRONMENTALDAMAGE : public LOGBASE {
  ENVIRONMENTALDAMAGE() {
  }

  ENVIRONMENTALDAMAGE(const ENVIRONMENTALDAMAGE &);
  ENVIRONMENTALDAMAGE(DWORDLONG victim, int school, int amount);

  DWORDLONG victim;
  int       school;
  int       amount;

  virtual void PI(CDataStore &msg, int debug) const;
  virtual void UI(CDataStore &msg);
};

struct MIRRORTIMERDAMAGE : public LOGBASE {
  MIRRORTIMERDAMAGE() {
  }

  MIRRORTIMERDAMAGE(const MIRRORTIMERDAMAGE &);
  MIRRORTIMERDAMAGE(UNIT_MIRROR_TIMER damage, DWORDLONG victim, int amount);

  int       damage;
  DWORDLONG victim;
  int       amount;

  virtual void PI(CDataStore &msg, int debug) const;
  virtual void UI(CDataStore &msg);
};

struct PARTYKILLLOG : public LOGBASE {
  PARTYKILLLOG(DWORDLONG killer = 0, DWORDLONG victim = 0);

  PARTYKILLLOG(const PARTYKILLLOG &other) : killer(other.killer), victim(other.victim) {
  }

  DWORDLONG killer;
  DWORDLONG victim;

  virtual void PI(CDataStore &msg, int debug) const;
  virtual void UI(CDataStore &msg);
};

inline DAMAGELOGBASE::DAMAGELOGBASE(const DAMAGELOGBASE &other)
    : LOGBASE(other),
      attacker(other.attacker),
      victim(other.victim),
      intellectBonus(other.intellectBonus),
      DPSScaler(other.DPSScaler),
      modDamageTaken(other.modDamageTaken),
      modDamageDone(other.modDamageDone),
      scaledDamage(other.scaledDamage),
      netDamageMultiplier(other.netDamageMultiplier),
      maxDamageReduction(other.maxDamageReduction),
      scaledArmorReduction(other.scaledArmorReduction),
      hitRollFloat(other.hitRollFloat),
      hitRollNeededFloat(other.hitRollNeededFloat),
      critRollFloat(other.critRollFloat),
      critRollNeededFloat(other.critRollNeededFloat),
      flags(other.flags),
      dmg(other.dmg) {
}

inline DAMAGELOGBASE::DAMAGELOGBASE(DWORDLONG attacker, DWORDLONG victim)
    : attacker(attacker),
      victim(victim),
      intellectBonus(0.0f),
      DPSScaler(0.0f),
      modDamageTaken(0.0f),
      modDamageDone(0.0f),
      scaledDamage(0.0f),
      netDamageMultiplier(0.0f),
      maxDamageReduction(0.0f),
      scaledArmorReduction(0.0f),
      hitRollFloat(0.0f),
      hitRollNeededFloat(0.0f),
      critRollFloat(0.0f),
      critRollNeededFloat(0.0f),
      flags(0) {
  dmg.Clear();
}

inline ATTACKROUNDINFO::ATTACKROUNDINFO() : DAMAGELOGBASE(0, 0) {
  armorReduction = 0;
  newVictimState = VS_NONE;
  victimRoundDuration = 0;
  dodgeRollFloat = 0.0f;
  dodgeRollNeededFloat = 0.0f;
  parryRollFloat = 0.0f;
  parryRollNeededFloat = 0.0f;
  stunRollFloat = 0.0f;
  stunRollNeededFloat = 0.0f;
  delayTime = 0;
  spellDamageAdded = 0;
  spellAddedDamage = 0;
  sinceLastSwing = 0;
  dualWieldHitRollFloat = 0.0f;
  dualWieldHitRollNeededFloat = 0.0f;
  procSpell = 0;
}

inline SPELLLOG::SPELLLOG(DWORDLONG attacker, DWORDLONG victim, int spellID)
    : DAMAGELOGBASE(attacker, victim), auraEffectID(0), spellID(spellID), damageType(0), resistanceCoefficient(0.0f) {
}

inline SPELLMISSLOG::SPELLMISSLOG(DWORDLONG attacker, DWORDLONG victim, UINT spellID)
    : attacker(attacker), victim(victim), spellID(spellID), flags(0) {
}

inline ENVIRONMENTALDAMAGE::ENVIRONMENTALDAMAGE(DWORDLONG victim, int school, int amount) : victim(victim), school(school), amount(amount) {
}

inline MIRRORTIMERDAMAGE::MIRRORTIMERDAMAGE(UNIT_MIRROR_TIMER damage, DWORDLONG victim, int amount) : damage(damage), victim(victim), amount(amount) {
}

inline PARTYKILLLOG::PARTYKILLLOG(DWORDLONG killer, DWORDLONG victim) : killer(killer), victim(victim) {
}

NODEDECL(ANIMQUEUENODE) {
  ANIMQUEUETYPE   type;
  ATTACKROUNDINFO roundInfo;
};

class CCombat {
  friend class CGUnit_C;

 public:
  CCombat() : m_victim(0) {
  }

  DWORDLONG IsAttacking() const;
  void      SetAttacking(DWORDLONG victim);
  void      StopAttack() {
    m_victim = 0;
  }
  void GetClientInitData(CClientObjCreate *init) const;
  void SetClientInitData(const CClientObjCreate &init);

 protected:
  DWORDLONG m_victim;
};

class CCombatClient : public CCombat {
  friend class CGUnit_C;

 public:
  CCombatClient() : m_attackSent(0), m_stopSent(0) {
  }

  int AttackBeenSent() const {
    return m_attackSent;
  }

  void SetAttackSent(DWORDLONG victim);
  void SetAttacking(DWORDLONG victim) {
    CCombat::SetAttacking(victim);
  }
  void StopAttack() {
    CCombat::StopAttack();
    m_attackSent = 0;
  }
  void ClearAttackSent() {
    m_attackSent = 0;
  }
  void SetStopSent(int stopSent) {
    m_stopSent = stopSent;
  }

  int StopBeenSent() const {
    return m_stopSent;
  }

 protected:
  int m_attackSent;
  int m_stopSent;
};
