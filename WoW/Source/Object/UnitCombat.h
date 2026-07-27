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
  int          totalDamage;
  int          damageType[5];
  unsigned int minDamage[5];
  unsigned int maxDamage[5];
  float        damageFloat[5];
  int          damage[5];
  int          absorbed[5];

  void Clear();
};

struct LOGBASE {
  ~LOGBASE() {
  }

  virtual void PI(CDataStore &msg, int debug) = 0;
  virtual void UI(CDataStore &msg) = 0;
};

struct DAMAGELOGBASE : public LOGBASE {
  unsigned __int64 attacker;
  unsigned __int64 victim;
  float            intellectBonus;
  float            DPSScaler;
  float            modDamageTaken;
  float            modDamageDone;
  float            scaledDamage;
  float            netDamageMultiplier;
  float            maxDamageReduction;
  float            scaledArmorReduction;
  float            hitRollFloat;
  float            hitRollNeededFloat;
  float            critRollFloat;
  float            critRollNeededFloat;
  unsigned int     flags;
  DamageData       dmg;
};

struct ATTACKROUNDINFO : public DAMAGELOGBASE {
  unsigned int armorReduction;
  VICTIMSTATES newVictimState;
  unsigned int victimRoundDuration;
  float        dodgeRollFloat;
  float        dodgeRollNeededFloat;
  float        parryRollFloat;
  float        parryRollNeededFloat;
  float        blockRollFloat;
  float        blockRollNeededFloat;
  float        stunRollFloat;
  float        stunRollNeededFloat;
  unsigned int delayTime;
  unsigned int spellDamageAdded;
  unsigned int spellAddedDamage;
  unsigned int sinceLastSwing;
  float        dualWieldHitRollFloat;
  float        dualWieldHitRollNeededFloat;
  int          procSpell;

  ATTACKROUNDINFO();
  virtual void PI(CDataStore &msg, int debug);
  virtual void UI(CDataStore &msg);
};

struct SPELLLOG : public DAMAGELOGBASE {
  unsigned int auraEffectID;
  unsigned int spellID;
  unsigned int damageType;
  float        resistanceCoefficient;

  virtual void PI(CDataStore &msg, int debug);
  virtual void UI(CDataStore &msg);
};

struct SPELLMISSLOG : public LOGBASE {
  unsigned __int64 attacker;
  unsigned __int64 victim;
  unsigned int     spellID;
  unsigned int     reason;
  float            hitRoll;
  float            hitRollNeeded;
  float            dodgeRoll;
  float            dodgeRollNeeded;
  float            parryRoll;
  float            parryRollNeeded;
  float            blockRoll;
  float            blockRollNeeded;
  unsigned int     flags;

  virtual void PI(CDataStore &msg, int debug);
  virtual void UI(CDataStore &msg);
};

struct RESISTLOG : public LOGBASE {
  unsigned __int64 attacker;
  unsigned __int64 victim;
  int              spell;
  float            resistRollNeeded;
  float            resistRoll;
  int              flags;
  int              castLevel;

  virtual void PI(CDataStore &msg, int debug);
  virtual void UI(CDataStore &msg);
};

struct ENCHANTMENTLOG : public LOGBASE {
  unsigned __int64 attacker;
  unsigned __int64 victim;
  int              enchantment;
  int              itemID;
  int              flags;

  virtual void PI(CDataStore &msg, int debug);
  virtual void UI(CDataStore &msg);
};

struct ENVIRONMENTALDAMAGE : public LOGBASE {
  unsigned __int64 victim;
  int              school;
  int              amount;

  virtual void PI(CDataStore &msg, int debug);
  virtual void UI(CDataStore &msg);
};

struct MIRRORTIMERDAMAGE : public LOGBASE {
  int              damage;
  unsigned __int64 victim;
  int              amount;

  virtual void PI(CDataStore &msg, int debug);
  virtual void UI(CDataStore &msg);
};

struct PARTYKILLLOG : public LOGBASE {
  PARTYKILLLOG(unsigned __int64 killer = 0, unsigned __int64 victim = 0);

  PARTYKILLLOG(const PARTYKILLLOG &other) : killer(other.killer), victim(other.victim) {
  }

  unsigned __int64 killer;
  unsigned __int64 victim;

  virtual void PI(CDataStore &msg, int debug);
  virtual void UI(CDataStore &msg);
};

inline PARTYKILLLOG::PARTYKILLLOG(unsigned __int64 killer, unsigned __int64 victim) : killer(killer), victim(victim) {
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

  unsigned __int64 IsAttacking() const;
  void             SetAttacking(unsigned __int64 victim);
  void             StopAttack() {
    m_victim = 0;
  }
  void             GetClientInitData(CClientObjCreate *init) const;
  void             SetClientInitData(const CClientObjCreate &init);

 protected:
  unsigned __int64 m_victim;
};

class CCombatClient : public CCombat {
 friend class CGUnit_C;
 public:
  CCombatClient() : m_attackSent(0), m_stopSent(0) {
  }

  int AttackBeenSent() const {
    return m_attackSent;
  }

  void SetAttackSent(unsigned __int64 victim);
  void SetAttacking(unsigned __int64 victim) {
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
