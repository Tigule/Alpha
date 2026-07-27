#pragma once

class SoundProviderPreferencesRec;
class ResistancesRec;
class WMOAreaTableRec;

enum WEAPONPARRYSEQ {
  WEAPONPARRYSEQ_NONE = 0
};

enum WEAPONREADYSEQ {
  WEAPONREADYSEQ_NONE = 0
};

enum WEAPONATTACKSEQ {
  WEAPONATTACKSEQ_NONE = 0
};

enum STRINGLOOKUP {
  SLOOKUP_UNUSED = 0,
  SLOOKUP_DEFAULTCURSOR = 1,
  SLOOKUP_INVENTORYICONBUTTONGEOMETRY = 2,
  SLOOKUP_INVENTORYICONPATH = 3,
  SLOOKUP_QUESTGIVERINDICATORTEXTURE = 4,
  SLOOKUP_QUESTGIVERINDICATORMODEL = 5,
  SLOOKUP_QUESTGIVERINDICATORMODELFUTURE = 6,
  SLOOKUP_TAXINODEINDICATORMODEL = 7,
  SLOOKUP_BINDERINDICATORMODEL = 8,
  SLOOKUP_QUESTGIVERINDICATORMODELCOMPLETION = 9,
  NUM_STRINGLOOKUPS = 10
};

const char *ClientDBStringLookup(STRINGLOOKUP lookup);
unsigned int GetPhysicalDamageClassID();
unsigned int GetFirstNonPhysicalID();
const ResistancesRec *GetDamageClassRecord(unsigned int record);
unsigned int ClientDBLookupTerrainSoundID(unsigned int terrainType);
WEAPONPARRYSEQ
ClientDBGetWeaponSubclassParrySeq(unsigned int subclassID);
WEAPONREADYSEQ
ClientDBGetWeaponSubclassReadySeq(unsigned int subclassID);
WEAPONATTACKSEQ
ClientDBGetWeaponSubclassWeaponSeq(unsigned int subclassID);
int ClientDBWeaponSubclassSetsFingerSeq(unsigned int subclassID);
unsigned int ClientDBGetUnarmedWeapon();
const SoundProviderPreferencesRec *ClientDBGetDefaultIndoorProviderPrefs();
const SoundProviderPreferencesRec *ClientDBGetDefaultOutdoorProviderPrefs();
unsigned int ClientDBGetNumWeaponSubclasses();
const char *SDBWMOAreaTableLookup(int wmoID, int nameSetID, int wmoGroupID);
bool SDBWMOAreaTableLookup(int wmoID, int nameSetID, int wmoGroupID, const WMOAreaTableRec *&rec);
