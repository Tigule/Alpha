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

enum SPELL_VISUAL_ATTACHMENT {
  SPELL_VISUAL_ATTACH_HEAD = 0,
  SPELL_VISUAL_ATTACH_CHEST = 1,
  SPELL_VISUAL_ATTACH_BASE = 2,
  SPELL_VISUAL_ATTACH_LEFT_HAND = 3,
  SPELL_VISUAL_ATTACH_RIGHT_HAND = 4,
  SPELL_VISUAL_ATTACH_BREATH = 5,
  SPELL_VISUAL_ATTACH_SPECIAL1 = 6,
  SPELL_VISUAL_ATTACH_SPECIAL2 = 7,
  SPELL_VISUAL_ATTACH_SPECIAL3 = 8,
  NUM_SPELL_VISUAL_ATTACH = 9
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
