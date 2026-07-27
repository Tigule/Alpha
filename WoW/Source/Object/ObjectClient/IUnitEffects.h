#pragma once

#include <Tempest/c3vector.h>

class CGUnit_C;

extern unsigned int g_specialSpellIDs[43];

enum MISS_REASON {
  MISS_REASON_NONE = 0,
  MISS_PHYSICAL = 1,
  MISS_RESIST = 2,
  MISS_IMMUNE = 3,
  MISS_EVADED = 4,
  MISS_DODGED = 5,
  MISS_PARRIED = 6,
  MISS_BLOCKED = 7,
  MISS_TEMPIMMUNE = 8,
  MISS_DEFLECTED = 9,
  MISS_NUMMISSTYPES = 10
};

struct MISSILESTRUCT {
  CGUnit_C          *caster;
  unsigned int       spellID;
  unsigned __int64   target;
  NTempest::C3Vector startPosition;
  NTempest::C3Vector destination;
  float              speed;
  unsigned int       ammoDisplayID;
  int                inventoryType;
  unsigned int       missileEffect;
  unsigned int       missileVictimEffect;
  unsigned int       missilePathType;
  unsigned int       hits;
  MISS_REASON        reason;
  int                sound;
};

void UnitEffectAddMissile(const MISSILESTRUCT &desc, int durationOffset);
