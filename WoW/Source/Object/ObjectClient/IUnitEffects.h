#pragma once

#include <Tempest/c3vector.h>

class CGUnit_C;

enum MISS_REASON {
  MISS_REASON_NONE = 0
};

struct MISSILESTRUCT {
  MISSILESTRUCT()
      : caster(0),
        spellID(0),
        target(0),
        speed(0.0f),
        ammoDisplayID(0),
        inventoryType(0),
        missileEffect(0),
        missileVictimEffect(0),
        missilePathType(0),
        hits(0),
        reason(MISS_REASON_NONE),
        sound(0) {
  }

  ~MISSILESTRUCT() {
  }

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

void __fastcall UnitEffectAddMissile(const MISSILESTRUCT &desc, int durationOffset);
