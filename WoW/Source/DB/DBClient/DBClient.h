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

const char *__fastcall           ClientDBStringLookup(STRINGLOOKUP lookup);
unsigned int __fastcall          GetPhysicalDamageClassID();
unsigned int __fastcall          GetFirstNonPhysicalID();
const ResistancesRec *__fastcall GetDamageClassRecord(unsigned int record);
unsigned int __fastcall          ClientDBLookupTerrainSoundID(unsigned int terrainType);
WEAPONPARRYSEQ
__fastcall ClientDBGetWeaponSubclassParrySeq(unsigned int subclassID);
WEAPONREADYSEQ
__fastcall ClientDBGetWeaponSubclassReadySeq(unsigned int subclassID);
WEAPONATTACKSEQ
__fastcall                                    ClientDBGetWeaponSubclassWeaponSeq(unsigned int subclassID);
int __fastcall                                ClientDBWeaponSubclassSetsFingerSeq(unsigned int subclassID);
unsigned int __fastcall                       ClientDBGetUnarmedWeapon();
const SoundProviderPreferencesRec *__fastcall ClientDBGetDefaultIndoorProviderPrefs();
const SoundProviderPreferencesRec *__fastcall ClientDBGetDefaultOutdoorProviderPrefs();
unsigned int __fastcall                       ClientDBGetNumWeaponSubclasses();
const char *__fastcall                        SDBWMOAreaTableLookup(int wmoID, int nameSetID, int wmoGroupID);
bool __fastcall                               SDBWMOAreaTableLookup(int wmoID, int nameSetID, int wmoGroupID, const WMOAreaTableRec *&rec);
