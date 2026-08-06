#pragma once

#include <Base/Base.h>

class SoundProviderPreferencesRec;
class ResistancesRec;
class WMOAreaTableRec;

enum WEAPONPARRYSEQ {
  WEAPONPARRYSEQ_2HTIGHT,
  WEAPONPARRYSEQ_2HLOOSE,
  WEAPONPARRYSEQ_1H,
  WEAPONPARRYSEQ_STAND,
  NUM_WEAPONPARRYSEQS
};

enum WEAPONREADYSEQ {
  WEAPONREADYSEQ_2HTIGHT,
  WEAPONREADYSEQ_2HLOOSE,
  WEAPONREADYSEQ_1H,
  WEAPONREADYSEQ_BOW,
  WEAPONREADYSEQ_RIFLE,
  WEAPONREADYSEQ_THROWN,
  NUM_WEAPONREADYSEQS
};

enum WEAPONATTACKSEQ {
  WEAPONATTACKSEQ_2HTIGHT,
  WEAPONATTACKSEQ_2HLOOSE,
  WEAPONATTACKSEQ_BOW,
  WEAPONATTACKSEQ_1H,
  WEAPONATTACKSEQ_RIFLE,
  WEAPONATTACKSEQ_THROWN,
  NUM_WEAPONATTACKSEQS
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

LPCSTR                ClientDBStringLookup(STRINGLOOKUP lookup);
UINT                  GetPhysicalDamageClassID();
UINT                  GetFirstNonPhysicalID();
const ResistancesRec *GetDamageClassRecord(UINT record);
UINT                  ClientDBLookupTerrainSoundID(UINT terrainType);
WEAPONPARRYSEQ
ClientDBGetWeaponSubclassParrySeq(UINT subclassID);
WEAPONREADYSEQ
ClientDBGetWeaponSubclassReadySeq(UINT subclassID);
WEAPONATTACKSEQ
ClientDBGetWeaponSubclassWeaponSeq(UINT subclassID);
int                                ClientDBWeaponSubclassSetsFingerSeq(UINT subclassID);
UINT                               ClientDBGetUnarmedWeapon();
const SoundProviderPreferencesRec *ClientDBGetDefaultIndoorProviderPrefs();
const SoundProviderPreferencesRec *ClientDBGetDefaultOutdoorProviderPrefs();
UINT                               ClientDBGetNumWeaponSubclasses();
LPCSTR                             SDBWMOAreaTableLookup(int wmoID, int nameSetID, int wmoGroupID);
bool                               SDBWMOAreaTableLookup(int wmoID, int nameSetID, int wmoGroupID, const WMOAreaTableRec *&rec);
