#include <Base/SFileExtras.h>
#include <DB/DBClient/DBClient.h>
#include <DB/DBClient/AutoCode/CreatureDisplayInfoRec.h>
#include <DB/DBClient/AutoCode/CreatureDisplayInfoExtraRec.h>
#include <DB/DBClient/AutoCode/CreatureFamilyRec.h>
#include <DB/DBClient/AutoCode/CreatureModelDataRec.h>
#include <DB/DBClient/AutoCode/CreatureSoundDataRec.h>
#include <DB/DBClient/AutoCode/CreatureTypeRec.h>
#include <DB/DBClient/AutoCode/CharStartOutfitRec.h>
#include <DB/DBClient/AutoCode/ChrRacesRec.h>
#include <DB/DBClient/AutoCode/ChrClassesRec.h>
#include <DB/DBClient/AutoCode/CinematicCameraRec.h>
#include <DB/DBClient/AutoCode/CinematicSequencesRec.h>
#include <DB/DBClient/AutoCode/EmotesRec.h>
#include <DB/DBClient/AutoCode/EmotesTextRec.h>
#include <DB/DBClient/AutoCode/EmotesTextDataRec.h>
#include <DB/DBClient/AutoCode/FactionRec.h>
#include <DB/DBClient/AutoCode/FactionGroupRec.h>
#include <DB/DBClient/AutoCode/FactionTemplateRec.h>
#include <DB/DBClient/AutoCode/ItemDisplayInfoRec.h>
#include <DB/DBClient/AutoCode/TabardBackgroundTexturesRec.h>
#include <DB/DBClient/AutoCode/TabardEmblemTexturesRec.h>
#include <DB/DBClient/AutoCode/PaperDollItemFrameRec.h>
#include <DB/DBClient/AutoCode/CharVariationsRec.h>
#include <DB/DBClient/AutoCode/ItemSubClassRec.h>
#include <DB/DBClient/AutoCode/SkillLineRec.h>
#include <DB/DBClient/AutoCode/SkillLineAbilityRec.h>
#include <DB/DBClient/AutoCode/SpellRec.h>
#include <DB/DBClient/AutoCode/SpellIconRec.h>
#include <DB/DBClient/AutoCode/SpellFocusObjectRec.h>
#include <DB/DBClient/AutoCode/SpellRangeRec.h>
#include <DB/DBClient/AutoCode/SpellRadiusRec.h>
#include <DB/DBClient/AutoCode/SpellVisualRec.h>
#include <DB/DBClient/AutoCode/SpellVisualEffectNameRec.h>
#include <DB/DBClient/AutoCode/SpellVisualKitRec.h>
#include <DB/DBClient/AutoCode/CharBaseInfoRec.h>
#include <DB/DBClient/AutoCode/ChrProficiencyRec.h>
#include <DB/DBClient/AutoCode/MaterialRec.h>
#include <DB/DBClient/AutoCode/WeaponImpactSoundsRec.h>
#include <DB/DBClient/AutoCode/SpellCastTimesRec.h>
#include <DB/DBClient/AutoCode/SpellChainEffectsRec.h>
#include <DB/DBClient/AutoCode/SpellDurationRec.h>
#include <DB/DBClient/AutoCode/SpellEffectNamesRec.h>
#include <DB/DBClient/AutoCode/SpellAuraNamesRec.h>
#include <DB/DBClient/AutoCode/SpellDispelTypeRec.h>
#include <DB/DBClient/AutoCode/AreaTriggerRec.h>
#include <DB/DBClient/AutoCode/SpellVisualAnimNameRec.h>
#include <DB/DBClient/AutoCode/SpellVisualPrecastTransitionsRec.h>
#include <DB/DBClient/AutoCode/TerrainTypeRec.h>
#include <DB/DBClient/AutoCode/TerrainTypeSoundsRec.h>
#include <DB/DBClient/AutoCode/AreaPOIRec.h>
#include <DB/DBClient/AutoCode/AreaTableRec.h>
#include <DB/DBClient/AutoCode/AttackAnimTypesRec.h>
#include <DB/DBClient/AutoCode/AttackAnimKitsRec.h>
#include <DB/DBClient/AutoCode/SoundEntriesRec.h>
#include <DB/DBClient/AutoCode/WeaponSwingSounds2Rec.h>
#include <DB/DBClient/AutoCode/UISoundLookupsRec.h>
#include <DB/DBClient/AutoCode/LanguagesRec.h>
#include <DB/DBClient/AutoCode/LanguageWordsRec.h>
#include <DB/DBClient/AutoCode/LockRec.h>
#include <DB/DBClient/AutoCode/LockTypeRec.h>
#include <DB/DBClient/AutoCode/VocalUISoundsRec.h>
#include <DB/DBClient/AutoCode/MapRec.h>
#include <DB/DBClient/AutoCode/FootprintTexturesRec.h>
#include <DB/DBClient/AutoCode/CharacterFacialHairStylesRec.h>
#include <DB/DBClient/AutoCode/CharacterCreateCamerasRec.h>
#include <DB/DBClient/AutoCode/ZoneMusicRec.h>
#include <DB/DBClient/AutoCode/SpellEffectCameraShakesRec.h>
#include <DB/DBClient/AutoCode/HelmetGeosetVisDataRec.h>
#include <DB/DBClient/AutoCode/StringLookupsRec.h>
#include <DB/DBClient/AutoCode/GameObjectDisplayInfoRec.h>
#include <DB/DBClient/AutoCode/PageTextMaterialRec.h>
#include <DB/DBClient/AutoCode/ItemGroupSoundsRec.h>
#include <DB/DBClient/AutoCode/FootstepTerrainLookupRec.h>
#include <DB/DBClient/AutoCode/ResistancesRec.h>
#include <DB/DBClient/AutoCode/TaxiNodesRec.h>
#include <DB/DBClient/AutoCode/SheatheSoundLookupsRec.h>
#include <DB/DBClient/AutoCode/TaxiPathRec.h>
#include <DB/DBClient/AutoCode/TaxiPathNodeRec.h>
#include <DB/DBClient/AutoCode/SoundProviderPreferencesRec.h>
#include <DB/DBClient/AutoCode/SoundSamplePreferencesRec.h>
#include <DB/DBClient/AutoCode/DeathThudLookupsRec.h>
#include <DB/DBClient/AutoCode/ItemClassRec.h>
#include <DB/DBClient/AutoCode/NPCSoundsRec.h>
#include <DB/DBClient/AutoCode/AreaMIDIAmbiencesRec.h>
#include <DB/DBClient/AutoCode/UnitBloodRec.h>
#include <DB/DBClient/AutoCode/UnitBloodLevelsRec.h>
#include <DB/DBClient/AutoCode/SoundWaterTypeRec.h>
#include <DB/DBClient/AutoCode/SpellItemEnchantmentRec.h>
#include <DB/DBClient/AutoCode/EmoteAnimsRec.h>
#include <DB/DBClient/AutoCode/SpellShapeshiftFormRec.h>
#include <DB/DBClient/AutoCode/BankBagSlotPricesRec.h>
#include <DB/DBClient/AutoCode/TransportAnimationRec.h>
#include <DB/DBClient/AutoCode/CharTextureVariationsV2Rec.h>
#include <DB/DBClient/AutoCode/CharHairGeosetsRec.h>
#include <DB/DBClient/AutoCode/WorldMapAreaRec.h>
#include <DB/DBClient/AutoCode/WorldMapContinentRec.h>
#include <DB/DBClient/AutoCode/SoundCharacterMacroLinesRec.h>
#include <DB/DBClient/AutoCode/WorldSafeLocsRec.h>
#include <DB/DBClient/AutoCode/QuestSortRec.h>
#include <DB/DBClient/AutoCode/QuestInfoRec.h>
#include <DB/DBClient/AutoCode/NamesProfanityRec.h>
#include <DB/DBClient/AutoCode/NamesReservedRec.h>
#include <DB/DBClient/AutoCode/ItemVisualsRec.h>
#include <DB/DBClient/AutoCode/ItemVisualEffectsRec.h>
#include <DB/DBClient/AutoCode/WMOAreaTableRec.h>
#include <DB/DBClient/AutoCode/CameraShakesRec.h>
#include <DB/DBClient/AutoCode/GroundEffectDoodadRec.h>
#include <DB/DBClient/AutoCode/GroundEffectTextureRec.h>

#include <stpl.h>

WowClientDB<GroundEffectTextureRec>           g_groundEffectTextureDB;
WowClientDB<GroundEffectDoodadRec>            g_groundEffectDoodadDB;
WowClientDB<CameraShakesRec>                  g_cameraShakesDB;
WowClientDB<CreatureDisplayInfoRec>           g_creatureDisplayInfoDB;
WowClientDB<CreatureDisplayInfoExtraRec>      g_creatureDisplayInfoExtraDB;
WowClientDB<CreatureFamilyRec>                g_creatureFamilyDB;
WowClientDB<CreatureModelDataRec>             g_creatureModelDataDB;
WowClientDB<CreatureSoundDataRec>             g_creatureSoundDataDB;
WowClientDB<CreatureTypeRec>                  g_creatureTypeDB;
WowClientDB<CharStartOutfitRec>               g_charStartOutfitDB;
WowClientDB<ChrRacesRec>                      g_chrRacesDB;
WowClientDB<ChrClassesRec>                    g_chrClassesDB;
WowClientDB<CinematicCameraRec>               g_cinematicCameraDB;
WowClientDB<CinematicSequencesRec>            g_cinematicSequencesDB;
WowClientDB<FactionRec>                       g_factionDB;
WowClientDB<FactionGroupRec>                  g_factionGroupDB;
WowClientDB<FactionTemplateRec>               g_factionTemplateDB;
WowClientDB<ItemDisplayInfoRec>               g_itemDisplayInfoDB;
WowClientDB<TabardBackgroundTexturesRec>      g_tabardBackgroundTexturesDB;
WowClientDB<TabardEmblemTexturesRec>          g_tabardEmblemTexturesDB;
WowClientDB<PaperDollItemFrameRec>            g_paperDollItemFrameDB;
WowClientDB<CharVariationsRec>                g_charVariationsDB;
WowClientDB<ItemSubClassRec>                  g_itemSubClassDB;
WowClientDB<SkillLineRec>                     g_skillLineDB;
WowClientDB<SkillLineAbilityRec>              g_skillLineAbilityDB;
WowClientDB<SpellRec>                         g_spellDB;
WowClientDB<SpellIconRec>                     g_spellIconDB;
WowClientDB<SpellRangeRec>                    g_spellRangeDB;
WowClientDB<SpellRadiusRec>                   g_spellRadiusDB;
WowClientDB<SpellVisualRec>                   g_spellVisualDB;
WowClientDB<SpellVisualEffectNameRec>         g_spellVisualEffectNameDB;
WowClientDB<SpellVisualKitRec>                g_spellVisualKitDB;
WowClientDB<CharBaseInfoRec>                  g_charBaseInfoDB;
WowClientDB<ChrProficiencyRec>                g_chrProficiencyDB;
WowClientDB<MaterialRec>                      g_materialDB;
WowClientDB<WeaponImpactSoundsRec>            g_weaponImpactSoundsDB;
WowClientDB<SpellCastTimesRec>                g_spellCastTimesDB;
WowClientDB<SpellDurationRec>                 g_spellDurationDB;
WowClientDB<SpellEffectNamesRec>              g_spellEffectNamesDB;
WowClientDB<SpellAuraNamesRec>                g_spellAuraNamesDB;
WowClientDB<SpellDispelTypeRec>               g_spellDispelTypeDB;
WowClientDB<AreaTriggerRec>                   g_areaTriggerDB;
WowClientDB<SpellVisualAnimNameRec>           g_spellVisualAnimNameDB;
WowClientDB<TerrainTypeRec>                   g_terrainTypeDB;
WowClientDB<TerrainTypeSoundsRec>             g_terrainTypeSoundsDB;
WowClientDB<AreaPOIRec>                       g_areaPOIDB;
WowClientDB<AreaTableRec>                     g_areaTableDB;
WowClientDB<AttackAnimKitsRec>                g_attackAnimKitsDB;
WowClientDB<AttackAnimTypesRec>               g_attackAnimTypesDB;
WowClientDB<SoundEntriesRec>                  g_soundEntriesDB;
WowClientDB<WeaponSwingSounds2Rec>            g_weaponSwingSounds2DB;
WowClientDB<UISoundLookupsRec>                g_uISoundLookupsDB;
WowClientDB<LanguagesRec>                     g_languagesDB;
WowClientDB<LanguageWordsRec>                 g_languageWordsDB;
WowClientDB<LockRec>                          g_lockDB;
WowClientDB<LockTypeRec>                      g_lockTypeDB;
WowClientDB<VocalUISoundsRec>                 g_vocalUISoundsDB;
WowClientDB<MapRec>                           g_mapDB;
WowClientDB<FootprintTexturesRec>             g_footprintTexturesDB;
WowClientDB<CharacterFacialHairStylesRec>     g_characterFacialHairStylesDB;
WowClientDB<CharacterCreateCamerasRec>        g_characterCreateCamerasDB;
WowClientDB<ZoneMusicRec>                     g_zoneMusicDB;
WowClientDB<SpellEffectCameraShakesRec>       g_spellEffectCameraShakesDB;
WowClientDB<HelmetGeosetVisDataRec>           g_helmetGeosetVisDataDB;
WowClientDB<StringLookupsRec>                 g_stringLookupsDB;
WowClientDB<GameObjectDisplayInfoRec>         g_gameObjectDisplayInfoDB;
WowClientDB<PageTextMaterialRec>              g_pageTextMaterialDB;
WowClientDB<ItemGroupSoundsRec>               g_itemGroupSoundsDB;
WowClientDB<FootstepTerrainLookupRec>         g_footstepTerrainLookupDB;
WowClientDB<ResistancesRec>                   g_resistancesDB;
WowClientDB<TaxiNodesRec>                     g_taxiNodesDB;
WowClientDB<SheatheSoundLookupsRec>           g_sheatheSoundLookupsDB;
WowClientDB<TaxiPathRec>                      g_taxiPathDB;
WowClientDB<TaxiPathNodeRec>                  g_taxiPathNodeDB;
WowClientDB<SoundProviderPreferencesRec>      g_soundProviderPreferencesDB;
WowClientDB<SoundSamplePreferencesRec>        g_soundSamplePreferencesDB;
WowClientDB<DeathThudLookupsRec>              g_deathThudLookupsDB;
WowClientDB<ItemClassRec>                     g_itemClassDB;
WowClientDB<NPCSoundsRec>                     g_nPCSoundsDB;
WowClientDB<AreaMIDIAmbiencesRec>             g_areaMIDIAmbiencesDB;
WowClientDB<UnitBloodRec>                     g_unitBloodDB;
WowClientDB<UnitBloodLevelsRec>               g_unitBloodLevelsDB;
WowClientDB<SoundWaterTypeRec>                g_soundWaterTypeDB;
WowClientDB<SpellItemEnchantmentRec>          g_spellItemEnchantmentDB;
WowClientDB<EmoteAnimsRec>                    g_emoteAnimsDB;
WowClientDB<EmotesRec>                        g_emotesDB;
WowClientDB<EmotesTextRec>                    g_emotesTextDB;
WowClientDB<EmotesTextDataRec>                g_emotesTextDataDB;
WowClientDB<SpellFocusObjectRec>              g_spellFocusObjectDB;
WowClientDB<SpellVisualPrecastTransitionsRec> g_spellVisualPrecastTransitionsDB;
WowClientDB<SpellShapeshiftFormRec>           g_spellShapeshiftFormDB;
WowClientDB<WMOAreaTableRec>                  g_wMOAreaTableDB;
WowClientDB<BankBagSlotPricesRec>             g_bankBagSlotPricesDB;
WowClientDB<TransportAnimationRec>            g_transportAnimationDB;
WowClientDB<SpellChainEffectsRec>             g_spellChainEffectsDB;
WowClientDB<CharTextureVariationsV2Rec>       g_charTextureVariationsV2DB;
WowClientDB<CharHairGeosetsRec>               g_charHairGeosetsDB;
WowClientDB<WorldMapAreaRec>                  g_worldMapAreaDB;
WowClientDB<WorldMapContinentRec>             g_worldMapContinentDB;
WowClientDB<SoundCharacterMacroLinesRec>      g_soundCharacterMacroLinesDB;
WowClientDB<WorldSafeLocsRec>                 g_worldSafeLocsDB;
WowClientDB<QuestSortRec>                     g_questSortDB;
WowClientDB<QuestInfoRec>                     g_questInfoDB;
WowClientDB<NamesProfanityRec>                g_namesProfanityDB;
WowClientDB<NamesReservedRec>                 g_namesReservedDB;
WowClientDB<ItemVisualsRec>                   g_itemVisualsDB;
WowClientDB<ItemVisualEffectsRec>             g_itemVisualEffectsDB;

static unsigned int                          s_physicalDamageClassID = -1;
static unsigned int                          s_firstNonPhysicalDamageClass = -1;
static TSFixedArray<const ResistancesRec *>  s_damageTypeRecordIDs;
static TSFixedArray<unsigned int>            s_terrainSoundType;
static TSFixedArray<const ItemSubClassRec *> s_weaponSubClasses;
static ItemClassRec                         *s_weaponClassRecPtr;
static ItemSubClassRec                      *s_unarmedWeaponSubclass;
static SoundProviderPreferencesRec          *s_defaultOutdoorProviderPrefs;
static SoundProviderPreferencesRec          *s_defaultIndoorProviderPrefs;

enum {
  NUM_WEAPONPARRYSEQS = 4,
  NUM_WEAPONREADYSEQS = 6,
  NUM_WEAPONATTACKSEQS = 6
};

void __fastcall        StaticDBLoadAll();
void __fastcall        CheckDamageClassConsistency();
void __fastcall        InitTerrainSoundTypeIDs();
void __fastcall        InitWeaponSubclasses();
void __fastcall        InitSoundProviderPreferences();
static void __fastcall LocateWeaponSubclass();

void __fastcall SDBItemSubclassInitialize();
void __fastcall SDBItemSubclassDestroy();

void __fastcall CheckDamageClassConsistency() {
  unsigned int numDamageClasses = g_resistancesDB.GetNumRecords();

  s_physicalDamageClassID = -1;
  s_firstNonPhysicalDamageClass = -1;

  if (numDamageClasses != 6) {
    FATALERROR(("Error, the DamageClass table doesn't have the right # of entries!"));
  }
  int nonphysicalToPhysical = 0;
  int physicalToNonPhysical = 0;
  int nonPhysicalFound = 0;

  s_damageTypeRecordIDs.SetCount(numDamageClasses);

  for (unsigned int i = 0; i < numDamageClasses; ++i) {
    const ResistancesRec *rec = g_resistancesDB.GetRecordByIndex(i);

    if (rec->m_Flags & 1) {
      if (s_physicalDamageClassID != -1) {
        FATALERROR(("Error, two physical damage types found in DamageClass table!"));
      }
      s_physicalDamageClassID = i;
      if (nonPhysicalFound) {
        nonphysicalToPhysical = 1;
      }
    }
    if (rec->m_ID != i) {
      FATALERROR(("Error, the DamageClass table isn't numbered consecutively!"));
    }
    if (!(rec->m_Flags & 1)) {
      if (!nonPhysicalFound) {
        s_firstNonPhysicalDamageClass = i;
      }
      nonPhysicalFound = 1;
      if (s_physicalDamageClassID != -1) {
        physicalToNonPhysical = 1;
      }
    }
    s_damageTypeRecordIDs[i] = rec;
  }

  if (physicalToNonPhysical && nonphysicalToPhysical) {
    FATALERROR(("Error, nonphysical damage types in DamageClass table are not contiguous!"));
  }
  if (s_physicalDamageClassID == -1) {
    FATALERROR(("Error, no physical damage class found in DamageClass table!"));
  }
}

void __fastcall InitTerrainSoundTypeIDs() {
  unsigned int numTerrainTypes = g_terrainTypeDB.GetMaxID() + 1;
  unsigned int i;
  s_terrainSoundType.SetCount(numTerrainTypes);
  for (i = 0; i < numTerrainTypes; ++i) {
    const TerrainTypeRec *rec;
    s_terrainSoundType[i] = 0;
    rec = g_terrainTypeDB.GetRecord(i);
    if (rec) {
      s_terrainSoundType[i] = rec->m_SoundID;
    }
  }
}

static void __fastcall LocateWeaponSubclass() {
  int i;
  s_weaponClassRecPtr = 0;
  for (i = g_itemClassDB.GetNumRecords(); i;) {
    ItemClassRec *rec = g_itemClassDB.GetRecordByIndex(--i);
    ASSERT(rec);

    if (rec->m_flags & 1) {
      if (s_weaponClassRecPtr)
        ASSERT(!"Error, only one entry in the ItemClass can be a weapon!");
      s_weaponClassRecPtr = rec;
    }
  }
  if (!s_weaponClassRecPtr)
    ASSERT(!"Error, at least one entry in the ItemClass has to be a weapon!");
}

void __fastcall InitWeaponSubclasses() {
  int numWeaponSubclasses = 0;
  int weaponClass;
  int i;
  if (s_weaponSubClasses.Count())
    s_weaponSubClasses.Clear();
  s_unarmedWeaponSubclass = 0;
  LocateWeaponSubclass();
  ASSERT(s_weaponClassRecPtr);
  weaponClass = s_weaponClassRecPtr->m_classID;

  for (i = g_itemSubClassDB.GetNumRecords(); i;) {
    ItemSubClassRec *rec = g_itemSubClassDB.GetRecordByIndex(--i);
    if (rec->m_classID == weaponClass && numWeaponSubclasses <= rec->m_subClassID + 1) {
      numWeaponSubclasses = rec->m_subClassID + 1;
    }
  }
  s_weaponSubClasses.SetCount(numWeaponSubclasses);
  for (i = g_itemSubClassDB.GetNumRecords(); i;) {
    ItemSubClassRec *rec = g_itemSubClassDB.GetRecordByIndex(--i);

    if (rec->m_classID == weaponClass) {
      s_weaponSubClasses[rec->m_subClassID] = rec;

      if (rec->m_flags & 0x4) {
        if (s_unarmedWeaponSubclass)
          ASSERT(!"Error, only one weapon subclass can be unarmed!");
        s_unarmedWeaponSubclass = rec;
      }
    }
  }
  if (!s_unarmedWeaponSubclass)
    ASSERT(!"Error, there has to be one weapon subclass that's marked as unarmed!");
  for (i = s_weaponSubClasses.Count(); i;) {
    --i;

    if (!s_weaponSubClasses[i])
      ASSERT(!"Error, weapon subclasses in the ItemSubClass table have to be contiguous!");

    ASSERT(s_weaponSubClasses[i]->m_weaponParrySeq <= NUM_WEAPONPARRYSEQS);
    ASSERT(s_weaponSubClasses[i]->m_weaponReadySeq <= NUM_WEAPONREADYSEQS);
    ASSERT(s_weaponSubClasses[i]->m_weaponAttackSeq <= NUM_WEAPONATTACKSEQS);
  }
}

void __fastcall InitSoundProviderPreferences() {
  int i;

  s_defaultOutdoorProviderPrefs = 0;
  s_defaultIndoorProviderPrefs = 0;

  for (i = g_soundProviderPreferencesDB.GetNumRecords(); i;) {
    SoundProviderPreferencesRec *rec = g_soundProviderPreferencesDB.GetRecordByIndex(--i);
    ASSERT(rec);
    if (rec->m_Flags & 1) {
      ASSERT(!s_defaultOutdoorProviderPrefs);
      s_defaultOutdoorProviderPrefs = rec;
    } else if (rec->m_Flags & 2) {
      ASSERT(!s_defaultIndoorProviderPrefs);
      s_defaultIndoorProviderPrefs = rec;
    }
  }
}

void __fastcall ClientDBInitialize() {
  StaticDBLoadAll();
  CheckDamageClassConsistency();
  InitTerrainSoundTypeIDs();
  InitWeaponSubclasses();
  InitSoundProviderPreferences();
  SDBItemSubclassInitialize();
}

void __fastcall ClientDBShutdown() {
  s_damageTypeRecordIDs.Clear();
  s_terrainSoundType.Clear();
  SDBItemSubclassDestroy();
}

const char *__fastcall ClientDBStringLookup(STRINGLOOKUP lookup) {
  const StringLookupsRec *record;

  ASSERT(lookup < NUM_STRINGLOOKUPS);

  record = g_stringLookupsDB.GetRecord(lookup);
  return record ? record->m_String : 0;
}

unsigned int __fastcall GetPhysicalDamageClassID() {
  ASSERT(s_physicalDamageClassID != -1);
  return s_physicalDamageClassID;
}

unsigned int __fastcall GetFirstNonPhysicalID() {
  ASSERT(s_firstNonPhysicalDamageClass != -1);
  return s_firstNonPhysicalDamageClass;
}

const ResistancesRec *__fastcall GetDamageClassRecord(unsigned int record) {
  return g_resistancesDB.GetRecord(record);
}

unsigned int __fastcall ClientDBLookupTerrainSoundID(unsigned int terrainType) {
  return terrainType < s_terrainSoundType.Count() ? s_terrainSoundType.Ptr()[terrainType] : 0;
}

WEAPONPARRYSEQ
__fastcall ClientDBGetWeaponSubclassParrySeq(unsigned int subclassID) {
  ASSERT(subclassID < s_weaponSubClasses.Count());
  ASSERT(s_weaponSubClasses[subclassID]);
  return static_cast<WEAPONPARRYSEQ>(s_weaponSubClasses[subclassID]->m_weaponParrySeq);
}

WEAPONREADYSEQ
__fastcall ClientDBGetWeaponSubclassReadySeq(unsigned int subclassID) {
  ASSERT(subclassID < s_weaponSubClasses.Count());
  ASSERT(s_weaponSubClasses[subclassID]);
  return static_cast<WEAPONREADYSEQ>(s_weaponSubClasses[subclassID]->m_weaponReadySeq);
}

WEAPONATTACKSEQ
__fastcall ClientDBGetWeaponSubclassWeaponSeq(unsigned int subclassID) {
  ASSERT(subclassID < s_weaponSubClasses.Count());
  ASSERT(s_weaponSubClasses[subclassID]);
  return static_cast<WEAPONATTACKSEQ>(s_weaponSubClasses[subclassID]->m_weaponAttackSeq);
}

unsigned int __fastcall ClientDBGetNumWeaponSubclasses() {
  return s_weaponSubClasses.Count();
}

int __fastcall ClientDBWeaponSubclassSetsFingerSeq(unsigned int subclassID) {
  ASSERT(subclassID < s_weaponSubClasses.Count());
  ASSERT(s_weaponSubClasses[subclassID]);
  return s_weaponSubClasses[subclassID]->m_flags & 0x2;
}

unsigned int __fastcall ClientDBGetUnarmedWeapon() {
  ASSERT(s_unarmedWeaponSubclass);
  return s_unarmedWeaponSubclass->m_subClassID;
}

const SoundProviderPreferencesRec *__fastcall ClientDBGetDefaultIndoorProviderPrefs() {
  return s_defaultIndoorProviderPrefs;
}

const SoundProviderPreferencesRec *__fastcall ClientDBGetDefaultOutdoorProviderPrefs() {
  return s_defaultOutdoorProviderPrefs;
}

void __fastcall StaticDBLoadAll() {
  g_groundEffectTextureDB.Load();
  g_groundEffectDoodadDB.Load();
  g_cameraShakesDB.Load();
  g_creatureDisplayInfoDB.Load();
  g_creatureDisplayInfoExtraDB.Load();
  g_creatureFamilyDB.Load();
  g_creatureModelDataDB.Load();
  g_creatureSoundDataDB.Load();
  g_creatureTypeDB.Load();
  g_charStartOutfitDB.Load();
  g_chrClassesDB.Load();
  g_chrRacesDB.Load();
  g_cinematicCameraDB.Load();
  g_cinematicSequencesDB.Load();
  g_factionDB.Load();
  g_factionGroupDB.Load();
  g_factionTemplateDB.Load();
  g_itemDisplayInfoDB.Load();
  g_tabardBackgroundTexturesDB.Load();
  g_tabardEmblemTexturesDB.Load();
  g_paperDollItemFrameDB.Load();
  g_charVariationsDB.Load();
  g_itemSubClassDB.Load();
  g_skillLineDB.Load();
  g_skillLineAbilityDB.Load();
  g_spellDB.Load();
  g_spellIconDB.Load();
  g_spellRadiusDB.Load();
  g_spellRangeDB.Load();
  g_spellVisualDB.Load();
  g_spellVisualEffectNameDB.Load();
  g_spellVisualKitDB.Load();
  g_charBaseInfoDB.Load();
  g_chrProficiencyDB.Load();
  g_materialDB.Load();
  g_weaponImpactSoundsDB.Load();
  g_spellCastTimesDB.Load();
  g_spellDurationDB.Load();
  g_spellEffectNamesDB.Load();
  g_spellAuraNamesDB.Load();
  g_spellDispelTypeDB.Load();
  g_areaTriggerDB.Load();
  g_spellVisualAnimNameDB.Load();
  g_terrainTypeDB.Load();
  g_terrainTypeSoundsDB.Load();
  g_areaPOIDB.Load();
  g_areaTableDB.Load();
  g_attackAnimKitsDB.Load();
  g_attackAnimTypesDB.Load();
  g_soundEntriesDB.Load();
  g_weaponSwingSounds2DB.Load();
  g_uISoundLookupsDB.Load();
  g_languagesDB.Load();
  g_languageWordsDB.Load();
  g_lockDB.Load();
  g_lockTypeDB.Load();
  g_vocalUISoundsDB.Load();
  g_mapDB.Load();
  g_footprintTexturesDB.Load();
  g_characterFacialHairStylesDB.Load();
  g_characterCreateCamerasDB.Load();
  g_zoneMusicDB.Load();
  g_spellEffectCameraShakesDB.Load();
  g_helmetGeosetVisDataDB.Load();
  g_stringLookupsDB.Load();
  g_gameObjectDisplayInfoDB.Load();
  g_pageTextMaterialDB.Load();
  g_itemGroupSoundsDB.Load();
  g_footstepTerrainLookupDB.Load();
  g_resistancesDB.Load();
  g_taxiNodesDB.Load();
  g_sheatheSoundLookupsDB.Load();
  g_taxiPathDB.Load();
  g_taxiPathNodeDB.Load();
  g_soundProviderPreferencesDB.Load();
  g_soundSamplePreferencesDB.Load();
  g_deathThudLookupsDB.Load();
  g_itemClassDB.Load();
  g_nPCSoundsDB.Load();
  g_areaMIDIAmbiencesDB.Load();
  g_unitBloodDB.Load();
  g_unitBloodLevelsDB.Load();
  g_soundWaterTypeDB.Load();
  g_spellItemEnchantmentDB.Load();
  g_emoteAnimsDB.Load();
  g_emotesDB.Load();
  g_emotesTextDB.Load();
  g_emotesTextDataDB.Load();
  g_spellFocusObjectDB.Load();
  g_spellVisualPrecastTransitionsDB.Load();
  g_spellShapeshiftFormDB.Load();
  g_wMOAreaTableDB.Load();
  g_bankBagSlotPricesDB.Load();
  g_transportAnimationDB.Load();
  g_spellChainEffectsDB.Load();
  g_charTextureVariationsV2DB.Load();
  g_charHairGeosetsDB.Load();
  g_worldMapAreaDB.Load();
  g_worldMapContinentDB.Load();
  g_soundCharacterMacroLinesDB.Load();
  g_worldSafeLocsDB.Load();
  g_questSortDB.Load();
  g_questInfoDB.Load();
  g_namesProfanityDB.Load();
  g_namesReservedDB.Load();
  g_itemVisualsDB.Load();
  g_itemVisualEffectsDB.Load();
}
