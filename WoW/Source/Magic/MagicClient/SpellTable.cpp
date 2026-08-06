#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

#include "DB/DBClient/AutoCode/CharBaseInfoRec.h"
#include "DB/DBClient/AutoCode/ChrClassesRec.h"
#include "DB/DBClient/AutoCode/ChrRacesRec.h"
#include "DB/DBClient/AutoCode/SkillLineAbilityRec.h"
#include "DB/DBClient/AutoCode/SkillLineRec.h"
#include "DB/DBClient/AutoCode/SpellRec.h"

#include <stpl.h>

class SkillLineTable {
 public:
  void                       Initialize();
  const SkillLineAbilityRec *Lookup(UINT raceID, UINT classID, UINT spellID);
  const SkillLineAbilityRec *LookupPet(int skillLineID, UINT spellID);

 private:
  bool MatchRaceClass(int raceID, int classID, int raceMask, int classMask, int excludeRace, int excludeClass);
  bool AddAbility(int raceID, int classID, TSFixedArray<const SkillLineAbilityRec *> &abilities, const SkillLineAbilityRec *rec);

  TSFixedArray<TSFixedArray<const SkillLineAbilityRec *> > m_abilities;
};

static SkillLineTable *s_skillLineTable;

bool SkillLineTable::MatchRaceClass(int raceID, int classID, int raceMask, int classMask, int excludeRace, int excludeClass) {
  if (excludeRace) {
    raceMask = ~raceMask;
  }
  if (excludeClass) {
    classMask = ~classMask;
  }

  if (raceMask && !(raceMask & (1 << (raceID - 1)))) {
    return false;
  }
  if (classMask && !(classMask & (1 << (classID - 1)))) {
    return false;
  }
  return true;
}

bool SkillLineTable::AddAbility(int raceID, int classID, TSFixedArray<const SkillLineAbilityRec *> &abilities, const SkillLineAbilityRec *rec) {
  const SkillLineRec *skillLine = g_skillLineDB.GetRecord(rec->m_skillLine);

  if (!skillLine) {
    return false;
  }
  if (!MatchRaceClass(raceID, classID, skillLine->m_raceMask, skillLine->m_classMask, skillLine->m_excludeRace, skillLine->m_excludeClass)) {
    return false;
  }
  if (!MatchRaceClass(raceID, classID, rec->m_raceMask, rec->m_classMask, rec->m_excludeRace, rec->m_excludeClass)) {
    return false;
  }

  abilities[rec->m_spell] = rec;
  return true;
}

void SkillLineTable::Initialize() {
  int numClasses = g_chrClassesDB.GetMaxID() + 1;

  m_abilities.SetCount(numClasses * (g_chrRacesDB.GetMaxID() + 1));

  for (int i = 0; i < g_charBaseInfoDB.GetNumRecords(); ++i) {
    const CharBaseInfoRec *rec = g_charBaseInfoDB.GetRecordByIndex(i);

    if (g_chrRacesDB.GetRecord(rec->m_raceID)) {
      TSFixedArray<const SkillLineAbilityRec *> &abilities = m_abilities[rec->m_raceID * numClasses + rec->m_classID];

      abilities.SetCount(g_spellDB.GetMaxID() + 1);
      memset(abilities.Ptr(), 0, abilities.Count() * sizeof(SkillLineAbilityRec *));

      for (int j = 0; j < g_skillLineAbilityDB.GetNumRecords(); ++j) {
        AddAbility(rec->m_raceID, rec->m_classID, abilities, g_skillLineAbilityDB.GetRecordByIndex(j));
      }
    }
  }
}

const SkillLineAbilityRec *SkillLineTable::Lookup(UINT raceID, UINT classID, UINT spellID) {
  UINT                                       numClasses = g_chrClassesDB.GetMaxID() + 1;
  TSFixedArray<const SkillLineAbilityRec *> &abilities = m_abilities[raceID * numClasses + classID];

  if (!abilities.Count()) {
    return 0;
  }

  return abilities[spellID];
}

const SkillLineAbilityRec *SkillLineTable::LookupPet(int skillLineID, UINT spellID) {
  for (int i = 0; i < g_skillLineAbilityDB.GetNumRecords(); ++i) {
    const SkillLineAbilityRec *ability = g_skillLineAbilityDB.GetRecordByIndex(i);
    if (ability->m_skillLine == skillLineID && ability->m_spell == spellID) {
      return ability;
    }
  }
  return 0;
}

void SpellTableInitialize() {
  ASSERT(!s_skillLineTable);

  s_skillLineTable = new SkillLineTable;
  s_skillLineTable->Initialize();
}

void SpellTableDestroy() {
  if (s_skillLineTable) {
    delete s_skillLineTable;
    s_skillLineTable = 0;
  }
}

const SkillLineAbilityRec *SpellTableLookupAbility(UINT raceID, UINT classID, UINT spellID) {
  if (!s_skillLineTable) {
    return 0;
  }

  return s_skillLineTable->Lookup(raceID, classID, spellID);
}

const SkillLineAbilityRec *SpellTableLookupPetAbility(int skillLineID, UINT spellID) {
  if (!s_skillLineTable) {
    return 0;
  }

  return s_skillLineTable->LookupPet(skillLineID, spellID);
}
