add_library(WowShared STATIC
    Source/Ui/ReputationInfo.cpp
    Source/Ui/SpellBookFrame.cpp
    Source/Ui/TabardCreationFrame.cpp
    Source/Ui/WorldFrame.cpp
    Source/Ui/ChatFrame.cpp
    Source/Ui/WorldMap.cpp
    Source/Ui/QuestFrame.cpp
    Source/Ui/PetInfo.cpp
    Source/Ui/MinimapFrame.cpp
    Source/Ui/FadingScreen.cpp
    Source/Ui/QuestLog.cpp
    Source/Ui/GameUI.cpp
    Source/Ui/DuelInfo.cpp
    Source/Ui/BuffFrame.cpp
    Source/Ui/PartyFrame.cpp
    Source/Ui/PaperDollInfoFrame.cpp
    Source/Ui/TradeSkillFrame.cpp
    Source/Ui/ItemTextFrame.cpp
    Source/Ui/PetitionInfo.cpp
    Source/Ui/NamePlateFrame.cpp
    Source/Ui/CharacterModelBase.cpp
    Source/Ui/Tutorial.cpp
    Source/Ui/TaxiMapFrame.cpp
    Source/Ui/BankFrame.cpp
    Source/Ui/UIBindings.cpp
    Source/Ui/GuildRegistrar.cpp
    Source/Ui/TradeFrame.cpp
    Source/Ui/TabardModelFrame.cpp
    Source/Ui/LootFrame.cpp
    Source/Ui/CraftInfo.cpp
    Source/Ui/MerchantFrame.cpp
    Source/Ui/ContainerFrame.cpp
    Source/Ui/QuestTextParser.cpp
    Source/Ui/ClassTrainerFrame.cpp
    Source/Ui/ActionBarFrame.cpp
    Source/Glue/GlueScriptEvents.cpp
    Source/Glue/CharCreateInfo.cpp
    Source/Glue/CGlueMgr.cpp
    Source/Glue/CharSelectInfo.cpp
    Source/SoundInterface/SoundInterfaceDoodad.cpp
    Source/SoundInterface/SoundInterfaceProviderPrefs.cpp
    Source/SoundInterface/SndDebugOutdoors.cpp
    Source/SoundInterface/SoundInterfaceWater.cpp
    Source/SoundInterface/SoundInterfaceGlueMusic.cpp
    Source/SoundInterface/SoundInterfaceMIDI.cpp
    Source/SoundInterface/SndDebug.cpp
    Source/SoundInterface/SndDebugIndoors.cpp
    Source/SoundInterface/SoundInterface.cpp
    Source/SoundInterface/ISoundInterface.cpp
    Source/SoundInterface/SoundInterfaceVocal.cpp
    Source/SoundInterface/SoundInterfaceZoneMusic.cpp
    Source/SoundInterface/SoundInterfaceZoneIntro.cpp
    Source/Net/NetInternal.cpp
    Source/Net/NetClient/NetClient.cpp
    Source/WorldCommon/WorldMath.cpp
    Source/WowServices/misc.cpp
    Source/WowServices/WowConnection.cpp
    Source/WowServices/WowConnectionNet.cpp
    Source/WowServices/Linux/WowConnectionNetSelect.cpp
    Source/WowServices/WDataStore.cpp
    Source/Magic/MagicClient/SpellVisuals.cpp
    Source/Magic/MagicClient/SpellTable.cpp
    Source/Magic/MagicClient/Spell_C.cpp
    Source/Magic/SpellNet.cpp
    Source/WowSvcs/WowSvcsClient/ClientServices.cpp
    Source/WowSvcs/WowSvcsClient/FriendList.cpp
    Source/ObjectAlloc/ObjectAlloc.cpp
    Source/WorldClient/MapLight.cpp
    Source/WorldClient/WorldScene.cpp
    Source/WorldClient/MapObjRead.cpp
    Source/WorldClient/MapLod.cpp
    Source/WorldClient/MapObjUpdate.cpp
    Source/WorldClient/MapChunk.cpp
    Source/WorldClient/MapWater.cpp
    Source/WorldClient/MapBaseObj.cpp
    Source/WorldClient/MapObjDef.cpp
    Source/WorldClient/MapQuery.cpp
    Source/WorldClient/MapObjGroup.cpp
    Source/WorldClient/MapRender.cpp
    Source/WorldClient/MapPurge.cpp
    Source/WorldClient/MapEntity.cpp
    Source/WorldClient/MapUnload.cpp
    Source/WorldClient/MapPrepare.cpp
    Source/WorldClient/MapChunkRender.cpp
    Source/WorldClient/SimpleDoodad.cpp
    Source/WorldClient/Map.cpp
    Source/WorldClient/MapUpdate.cpp
    Source/WorldClient/WorldParam.cpp
    Source/WorldClient/MapObj.cpp
    Source/WorldClient/AreaList.cpp
    Source/WorldClient/MapTexture.cpp
    Source/WorldClient/MapClip.cpp
    Source/WorldClient/MapLoad.cpp
    Source/WorldClient/DetailDoodad.cpp
    Source/WorldClient/MapArea.cpp
    Source/WorldClient/MapDoodadDef.cpp
    Source/WorldClient/World.cpp
    Source/WorldClient/MapObjRender.cpp
    Source/WorldClient/MapMem.cpp
    Source/Component/Component.cpp
    Source/Component/CharacterCustomization.cpp
    Source/Game/Loot.cpp
    Source/Game/GameTime.cpp
    Source/Game/GameClient/shadow.cpp
    Source/Game/GameClient/NameCache.cpp
    Source/Game/GameClient/PetNameCache.cpp
    Source/Game/GameClient/PlayerName.cpp
    Source/Game/GameClient/TaxiMap.cpp
    Source/Game/GameClient/GameTime_C.cpp
    Source/Game/GameClient/WeaponTrails.cpp
    Source/Game/GameClient/GuildClient.cpp
    Source/Game/GameClient/ViolenceLevels.cpp
    Source/Game/GameClient/QuestCache.cpp
    Source/Game/GameClient/WorldText.cpp
    Source/Game/GameClient/Minimap.cpp
    Source/Game/ValidateName.cpp
    Source/ClientGMCommands.cpp
    Source/ObjectMgrClient/ObjectMgrClient.cpp
    Source/Object/CreatureStats.cpp
    Source/Object/PageTextCache.cpp
    Source/Object/Collide.cpp
    Source/Object/GameObject.cpp
    Source/Object/mirror.cpp
    Source/Object/Object.cpp
    Source/Object/ObjectClient/UnitCombatLog_C.cpp
    Source/Object/ObjectClient/Trade_C.cpp
    Source/Object/ObjectClient/Movement_C.cpp
    Source/Object/ObjectClient/DynamicObject_C.cpp
    Source/Object/ObjectClient/Item_C.cpp
    Source/Object/ObjectClient/Container_C.cpp
    Source/Object/ObjectClient/PlayerSound_C.cpp
    Source/Object/ObjectClient/UnitSound_C.cpp
    Source/Object/ObjectClient/Unit_C.cpp
    Source/Object/ObjectClient/Corpse_C.cpp
    Source/Object/ObjectClient/UnitCombat_C.cpp
    Source/Object/ObjectClient/GameObject_C.cpp
    Source/Object/ObjectClient/Bag_C.cpp
    Source/Object/ObjectClient/UnitEffects.cpp
    Source/Object/ObjectClient/NPC_C.cpp
    Source/Object/ObjectClient/PlayerGM_C.cpp
    Source/Object/ObjectClient/Object_C.cpp
    Source/Object/ObjectClient/ZoneDebug.cpp
    Source/Object/ObjectClient/UnitFootprints.cpp
    Source/Object/ObjectClient/Player_C.cpp
    Source/Object/ObjectClient/IUnitEffects.cpp
    Source/Object/ObjectClient/GuildStats_C.cpp
    Source/Object/ObjectClient/CollideDisplay.cpp
    Source/Object/GameObjectStats.cpp
    Source/Object/Movement.cpp
    Source/Object/Combat.cpp
    Source/Object/Unit.cpp
    Source/Object/ItemStats.cpp
    Source/Object/NPCText.cpp
    Source/Object/GuildStats.cpp
    Source/Object/Petition.cpp
    Source/Object/CollideDebug.cpp
    Source/LoadingScreen.cpp
    Source/LogClient/Log.cpp
    Source/DB/DBClient/DBCacheInstances.cpp
    Source/DB/DBClient/StatConstants.cpp
    Source/DB/DBClient/AutoCode/AreaTriggerRec.cpp
    Source/DB/DBClient/AutoCode/CharStartOutfitRec.cpp
    Source/DB/DBClient/AutoCode/SpellEffectCameraShakesRec.cpp
    Source/DB/DBClient/AutoCode/ItemDisplayInfoRec.cpp
    Source/DB/DBClient/AutoCode/TaxiNodesRec.cpp
    Source/DB/DBClient/AutoCode/WorldSafeLocsRec.cpp
    Source/DB/DBClient/AutoCode/TabardEmblemTexturesRec.cpp
    Source/DB/DBClient/AutoCode/ItemSubClassRec.cpp
    Source/DB/DBClient/AutoCode/MapRec.cpp
    Source/DB/DBClient/AutoCode/AreaPOIRec.cpp
    Source/DB/DBClient/AutoCode/NPCSoundsRec.cpp
    Source/DB/DBClient/AutoCode/SoundProviderPreferencesRec.cpp
    Source/DB/DBClient/AutoCode/TransportAnimationRec.cpp
    Source/DB/DBClient/AutoCode/FactionGroupRec.cpp
    Source/DB/DBClient/AutoCode/EmotesRec.cpp
    Source/DB/DBClient/AutoCode/ItemVisualEffectsRec.cpp
    Source/DB/DBClient/AutoCode/CreatureSoundDataRec.cpp
    Source/DB/DBClient/AutoCode/SpellVisualRec.cpp
    Source/DB/DBClient/AutoCode/WorldMapAreaRec.cpp
    Source/DB/DBClient/AutoCode/GroundEffectDoodadRec.cpp
    Source/DB/DBClient/AutoCode/CameraShakesRec.cpp
    Source/DB/DBClient/AutoCode/SpellItemEnchantmentRec.cpp
    Source/DB/DBClient/AutoCode/FootprintTexturesRec.cpp
    Source/DB/DBClient/AutoCode/SpellVisualEffectNameRec.cpp
    Source/DB/DBClient/AutoCode/AreaTableRec.cpp
    Source/DB/DBClient/AutoCode/FactionRec.cpp
    Source/DB/DBClient/AutoCode/NamesReservedRec.cpp
    Source/DB/DBClient/AutoCode/ItemVisualsRec.cpp
    Source/DB/DBClient/AutoCode/CharBaseInfoRec.cpp
    Source/DB/DBClient/AutoCode/GameObjectDisplayInfoRec.cpp
    Source/DB/DBClient/AutoCode/LockTypeRec.cpp
    Source/DB/DBClient/AutoCode/WMOAreaTableRec.cpp
    Source/DB/DBClient/AutoCode/SkillLineRec.cpp
    Source/DB/DBClient/AutoCode/EmoteAnimsRec.cpp
    Source/DB/DBClient/AutoCode/UnitBloodRec.cpp
    Source/DB/DBClient/AutoCode/CinematicSequencesRec.cpp
    Source/DB/DBClient/AutoCode/ChrProficiencyRec.cpp
    Source/DB/DBClient/AutoCode/SpellAuraNamesRec.cpp
    Source/DB/DBClient/AutoCode/SpellIconRec.cpp
    Source/DB/DBClient/AutoCode/SpellRadiusRec.cpp
    Source/DB/DBClient/AutoCode/SpellShapeshiftFormRec.cpp
    Source/DB/DBClient/AutoCode/SpellDispelTypeRec.cpp
    Source/DB/DBClient/AutoCode/ItemGroupSoundsRec.cpp
    Source/DB/DBClient/AutoCode/CharacterCreateCamerasRec.cpp
    Source/DB/DBClient/AutoCode/CharTextureVariationsV2Rec.cpp
    Source/DB/DBClient/AutoCode/ChrClassesRec.cpp
    Source/DB/DBClient/AutoCode/SpellVisualAnimNameRec.cpp
    Source/DB/DBClient/AutoCode/SpellVisualKitRec.cpp
    Source/DB/DBClient/AutoCode/AttackAnimTypesRec.cpp
    Source/DB/DBClient/AutoCode/EmotesTextDataRec.cpp
    Source/DB/DBClient/AutoCode/AttackAnimKitsRec.cpp
    Source/DB/DBClient/AutoCode/UISoundLookupsRec.cpp
    Source/DB/DBClient/AutoCode/CharVariationsRec.cpp
    Source/DB/DBClient/AutoCode/PageTextMaterialRec.cpp
    Source/DB/DBClient/AutoCode/TerrainTypeSoundsRec.cpp
    Source/DB/DBClient/AutoCode/ChrRacesRec.cpp
    Source/DB/DBClient/AutoCode/LanguageWordsRec.cpp
    Source/DB/DBClient/AutoCode/WeaponImpactSoundsRec.cpp
    Source/DB/DBClient/AutoCode/ResistancesRec.cpp
    Source/DB/DBClient/AutoCode/SoundEntriesRec.cpp
    Source/DB/DBClient/AutoCode/CharHairGeosetsRec.cpp
    Source/DB/DBClient/AutoCode/DeathThudLookupsRec.cpp
    Source/DB/DBClient/AutoCode/LanguagesRec.cpp
    Source/DB/DBClient/AutoCode/MaterialRec.cpp
    Source/DB/DBClient/AutoCode/ZoneMusicRec.cpp
    Source/DB/DBClient/AutoCode/WorldMapContinentRec.cpp
    Source/DB/DBClient/AutoCode/UnitBloodLevelsRec.cpp
    Source/DB/DBClient/AutoCode/SoundWaterTypeRec.cpp
    Source/DB/DBClient/AutoCode/AreaMIDIAmbiencesRec.cpp
    Source/DB/DBClient/AutoCode/SoundSamplePreferencesRec.cpp
    Source/DB/DBClient/AutoCode/SpellRec.cpp
    Source/DB/DBClient/AutoCode/SheatheSoundLookupsRec.cpp
    Source/DB/DBClient/AutoCode/SpellFocusObjectRec.cpp
    Source/DB/DBClient/AutoCode/LockRec.cpp
    Source/DB/DBClient/AutoCode/TaxiPathRec.cpp
    Source/DB/DBClient/AutoCode/SpellCastTimesRec.cpp
    Source/DB/DBClient/AutoCode/FootstepTerrainLookupRec.cpp
    Source/DB/DBClient/AutoCode/TabardBackgroundTexturesRec.cpp
    Source/DB/DBClient/AutoCode/EmotesTextRec.cpp
    Source/DB/DBClient/AutoCode/TerrainTypeRec.cpp
    Source/DB/DBClient/AutoCode/FactionTemplateRec.cpp
    Source/DB/DBClient/AutoCode/SpellRangeRec.cpp
    Source/DB/DBClient/AutoCode/VocalUISoundsRec.cpp
    Source/DB/DBClient/AutoCode/PaperDollItemFrameRec.cpp
    Source/DB/DBClient/AutoCode/QuestInfoRec.cpp
    Source/DB/DBClient/AutoCode/BankBagSlotPricesRec.cpp
    Source/DB/DBClient/AutoCode/QuestSortRec.cpp
    Source/DB/DBClient/AutoCode/CreatureDisplayInfoExtraRec.cpp
    Source/DB/DBClient/AutoCode/HelmetGeosetVisDataRec.cpp
    Source/DB/DBClient/AutoCode/CreatureTypeRec.cpp
    Source/DB/DBClient/AutoCode/SpellDurationRec.cpp
    Source/DB/DBClient/AutoCode/VideoHardwareRec.cpp
    Source/DB/DBClient/AutoCode/SpellVisualPrecastTransitionsRec.cpp
    Source/DB/DBClient/AutoCode/CinematicCameraRec.cpp
    Source/DB/DBClient/AutoCode/ItemClassRec.cpp
    Source/DB/DBClient/AutoCode/SpellChainEffectsRec.cpp
    Source/DB/DBClient/AutoCode/SoundCharacterMacroLinesRec.cpp
    Source/DB/DBClient/AutoCode/SkillLineAbilityRec.cpp
    Source/DB/DBClient/AutoCode/CreatureModelDataRec.cpp
    Source/DB/DBClient/AutoCode/SpellEffectNamesRec.cpp
    Source/DB/DBClient/AutoCode/StringLookupsRec.cpp
    Source/DB/DBClient/AutoCode/NamesProfanityRec.cpp
    Source/DB/DBClient/AutoCode/CharacterFacialHairStylesRec.cpp
    Source/DB/DBClient/AutoCode/CreatureFamilyRec.cpp
    Source/DB/DBClient/AutoCode/WeaponSwingSounds2Rec.cpp
    Source/DB/DBClient/AutoCode/CreatureDisplayInfoRec.cpp
    Source/DB/DBClient/AutoCode/GroundEffectTextureRec.cpp
    Source/DB/DBClient/AutoCode/TaxiPathNodeRec.cpp
    Source/DB/DBClient/DBClient.cpp
    Source/DB/DBClient/sdbItemSubclass.cpp
    Source/DB/DBClient/DBCache.cpp
    Source/DB/DBClient/sdbWMOAreaTable.cpp
    Source/UIUtil/ScriptEvents.cpp
    Source/UIUtil/SmartScreenRects.cpp
    Source/UIUtil/PortraitButton.cpp
    Source/UIUtil/Camera.cpp
    Source/UIUtil/HealthBar.cpp
    Source/UIUtil/InputControl.cpp
    Source/UIUtil/Tooltip.cpp
    Source/UIUtil/CSimpleCamera.cpp
    Source/UIUtil/Cursor.cpp
    Source/Console/IConsole.cpp
    Source/Console/ConsoleClient.cpp
    Source/Console/ConsoleVar.cpp
    Source/Console/ConsoleCommand.cpp
    Source/Console/ConsoleDetect.cpp
    Source/ClientCommands.cpp
    Common/GameObjectDefs.cpp
    Common/WowTime.cpp
    Common/DayNight.cpp
    Common/LightsAndFog.cpp
    Common/AaBsp.cpp
    Source/CompFilenameDecoration/CompDecoration.cpp
    Common/TextureCacheCore.cpp
    Common/ComponentCore/ComponentUtilCore.cpp
)

if(MSVC_VERSION EQUAL 1200)
    set_source_files_properties(
        Source/Ui/GameUI.cpp
        PROPERTIES COMPILE_FLAGS "/GX-"
    )
    set_source_files_properties(
        Source/WorldCommon/WorldMath.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy- /GX-"
    )
    set_source_files_properties(
        Source/Ui/ItemTextFrame.cpp
        Source/Ui/QuestLog.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1"
    )
    set_source_files_properties(
        Source/LoadingScreen.cpp
        Source/WorldClient/MapArea.cpp
        Source/WorldClient/MapBaseObj.cpp
        Source/WorldClient/MapChunk.cpp
        Source/WorldClient/MapChunkRender.cpp
        Source/WorldClient/MapLight.cpp
        Source/WorldClient/MapMem.cpp
        Source/WorldClient/MapObjDef.cpp
        Source/WorldClient/MapRender.cpp
        Source/WorldClient/MapUnload.cpp
        Source/WorldClient/MapWater.cpp
        Source/WorldClient/WorldScene.cpp
        Source/Magic/MagicClient/SpellTable.cpp
        Source/Game/GameClient/PlayerName.cpp
        Source/Game/GameClient/TaxiMap.cpp
        Source/Game/GameClient/WorldText.cpp
        Source/Game/GameClient/WeaponTrails.cpp
        Source/Game/GameClient/ViolenceLevels.cpp
        Source/Object/ObjectClient/Item_C.cpp
        Source/Object/ObjectClient/Object_C.cpp
        Source/UIUtil/SmartScreenRects.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /GX-"
    )
    set_source_files_properties(
        Source/Object/Movement.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /GX-"
    )
    set_source_files_properties(
        Source/WorldClient/MapLight.cpp
        Source/WorldClient/MapDoodadDef.cpp
        Source/WorldClient/MapLoad.cpp
        Source/WorldClient/WorldScene.cpp
        Source/Game/GameClient/ViolenceLevels.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy- /GX-"
    )
    set_source_files_properties(
        Common/AaBsp.cpp
        Source/WorldClient/MapObj.cpp
        Source/WorldClient/MapObjGroup.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /GX-"
    )
    set_source_files_properties(
        Common/DayNight.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy- /GX-"
    )
    set_source_files_properties(
        Common/WowTime.cpp
        Source/Game/GameTime.cpp
        Source/Game/GameClient/GameTime_C.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy- /GX-"
    )
    set_source_files_properties(
        Source/Magic/MagicClient/SpellVisuals.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /GX-"
    )
    set_source_files_properties(
        Common/TextureCacheCore.cpp
        PROPERTIES
            COMPILE_FLAGS "/G6 /Ob1 /GX-"
            OBJECT_NAME TextureCache
    )
    set_source_files_properties(
        Source/SoundInterface/SoundInterface.cpp
        Source/SoundInterface/ISoundInterface.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Gr /Ob1 /Oy- /GX-"
    )
    set_source_files_properties(
        Source/UIUtil/ScriptEvents.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Gr /Ob1 /GX-"
    )
    set_source_files_properties(
        Source/Console/ConsoleCommand.cpp
        PROPERTIES COMPILE_FLAGS "/GX-"
    )
    set_source_files_properties(
        Source/SoundInterface/SoundInterfaceDoodad.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy- /GX-"
    )
    set_source_files_properties(
        Source/SoundInterface/SndDebug.cpp
        Source/SoundInterface/SndDebugIndoors.cpp
        Source/SoundInterface/SndDebugOutdoors.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Gr /Ob1 /GX-"
    )
    set_source_files_properties(
        Source/SoundInterface/SoundInterfaceGlueMusic.cpp
        Source/SoundInterface/SoundInterfaceProviderPrefs.cpp
        Source/SoundInterface/SoundInterfaceZoneMusic.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1"
    )
    set_source_files_properties(
        Source/Component/Component.cpp
        Source/Component/CharacterCustomization.cpp
        Source/Glue/CharCreateInfo.cpp
        Source/Glue/CharSelectInfo.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /GX-"
    )
    set_source_files_properties(
        Source/Component/Component.cpp
        PROPERTIES OBJECT_NAME Component
    )
    set_source_files_properties(
        Source/Component/CharacterCustomization.cpp
        PROPERTIES OBJECT_NAME CharacterCustomization
    )
    set_source_files_properties(
        Common/ComponentCore/ComponentUtilCore.cpp
        PROPERTIES
            COMPILE_FLAGS "/G6 /Ob1 /GX-"
            OBJECT_NAME ComponentUtil
    )
    set_source_files_properties(
        Source/Glue/CharCreateInfo.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Gr /Ob1 /GX-"
    )
    set_source_files_properties(
        Source/Glue/CharSelectInfo.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Gr /Ob1 /Oy- /GX-"
    )
    set_source_files_properties(
        Source/Glue/GlueScriptEvents.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Gr /Ob1 /GX-"
    )
    set_source_files_properties(
        Source/Console/ConsoleVar.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX-"
    )
    set_source_files_properties(
        Source/Console/ConsoleDetect.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy- /GX-"
    )
    set_source_files_properties(
        Source/Console/ConsoleClient.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Gr /Ob1 /Oy- /GX-"
    )
    set_source_files_properties(
        Source/ClientCommands.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /GX-"
    )
    set_source_files_properties(
        Source/DB/DBClient/AutoCode/VideoHardwareRec.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy-"
    )
    set_source_files_properties(
        Source/DB/DBClient/AutoCode/ItemVisualEffectsRec.cpp
        Source/DB/DBClient/AutoCode/ItemVisualsRec.cpp
        Source/DB/DBClient/AutoCode/WorldMapAreaRec.cpp
        Source/DB/DBClient/AutoCode/NamesReservedRec.cpp
        Source/DB/DBClient/AutoCode/NamesProfanityRec.cpp
        Source/DB/DBClient/AutoCode/QuestInfoRec.cpp
        Source/DB/DBClient/AutoCode/QuestSortRec.cpp
        Source/DB/DBClient/AutoCode/WorldSafeLocsRec.cpp
        Source/DB/DBClient/AutoCode/WorldMapContinentRec.cpp
        Source/DB/DBClient/AutoCode/SoundCharacterMacroLinesRec.cpp
        Source/DB/DBClient/AutoCode/CharHairGeosetsRec.cpp
        Source/DB/DBClient/AutoCode/CharTextureVariationsV2Rec.cpp
        Source/DB/DBClient/AutoCode/SpellChainEffectsRec.cpp
        Source/DB/DBClient/AutoCode/TransportAnimationRec.cpp
        Source/DB/DBClient/AutoCode/BankBagSlotPricesRec.cpp
        Source/DB/DBClient/AutoCode/WMOAreaTableRec.cpp
        Source/DB/DBClient/AutoCode/SpellShapeshiftFormRec.cpp
        Source/DB/DBClient/AutoCode/SpellVisualPrecastTransitionsRec.cpp
        Source/DB/DBClient/AutoCode/SpellFocusObjectRec.cpp
        Source/DB/DBClient/AutoCode/EmotesTextDataRec.cpp
        Source/DB/DBClient/AutoCode/EmotesTextRec.cpp
        Source/DB/DBClient/AutoCode/EmotesRec.cpp
        Source/DB/DBClient/AutoCode/EmoteAnimsRec.cpp
        Source/DB/DBClient/AutoCode/SpellItemEnchantmentRec.cpp
        Source/DB/DBClient/AutoCode/SoundWaterTypeRec.cpp
        Source/DB/DBClient/AutoCode/UnitBloodLevelsRec.cpp
        Source/DB/DBClient/AutoCode/UnitBloodRec.cpp
        Source/DB/DBClient/AutoCode/AreaMIDIAmbiencesRec.cpp
        Source/DB/DBClient/AutoCode/NPCSoundsRec.cpp
        Source/DB/DBClient/AutoCode/ItemClassRec.cpp
        Source/DB/DBClient/AutoCode/DeathThudLookupsRec.cpp
        Source/DB/DBClient/AutoCode/SoundSamplePreferencesRec.cpp
        Source/DB/DBClient/AutoCode/SoundProviderPreferencesRec.cpp
        Source/DB/DBClient/AutoCode/TaxiPathNodeRec.cpp
        Source/DB/DBClient/AutoCode/TaxiPathRec.cpp
        Source/DB/DBClient/AutoCode/SheatheSoundLookupsRec.cpp
        Source/DB/DBClient/AutoCode/TaxiNodesRec.cpp
        Source/DB/DBClient/AutoCode/ResistancesRec.cpp
        Source/DB/DBClient/AutoCode/FootstepTerrainLookupRec.cpp
        Source/DB/DBClient/AutoCode/ItemGroupSoundsRec.cpp
        Source/DB/DBClient/AutoCode/PageTextMaterialRec.cpp
        Source/DB/DBClient/AutoCode/GameObjectDisplayInfoRec.cpp
        Source/DB/DBClient/AutoCode/StringLookupsRec.cpp
        Source/DB/DBClient/AutoCode/HelmetGeosetVisDataRec.cpp
        Source/DB/DBClient/AutoCode/SpellEffectCameraShakesRec.cpp
        Source/DB/DBClient/AutoCode/ZoneMusicRec.cpp
        Source/DB/DBClient/AutoCode/CharacterCreateCamerasRec.cpp
        Source/DB/DBClient/AutoCode/CharacterFacialHairStylesRec.cpp
        Source/DB/DBClient/AutoCode/FootprintTexturesRec.cpp
        Source/DB/DBClient/AutoCode/MapRec.cpp
        Source/DB/DBClient/AutoCode/VocalUISoundsRec.cpp
        Source/DB/DBClient/AutoCode/LockTypeRec.cpp
        Source/DB/DBClient/AutoCode/LockRec.cpp
        Source/DB/DBClient/AutoCode/LanguageWordsRec.cpp
        Source/DB/DBClient/AutoCode/LanguagesRec.cpp
        Source/DB/DBClient/AutoCode/UISoundLookupsRec.cpp
        Source/DB/DBClient/AutoCode/WeaponSwingSounds2Rec.cpp
        Source/DB/DBClient/AutoCode/SoundEntriesRec.cpp
        Source/DB/DBClient/AutoCode/AttackAnimTypesRec.cpp
        Source/DB/DBClient/AutoCode/AttackAnimKitsRec.cpp
        Source/DB/DBClient/AutoCode/AreaTableRec.cpp
        Source/DB/DBClient/AutoCode/AreaPOIRec.cpp
        Source/DB/DBClient/AutoCode/TerrainTypeSoundsRec.cpp
        Source/DB/DBClient/AutoCode/TerrainTypeRec.cpp
        Source/DB/DBClient/AutoCode/SpellVisualAnimNameRec.cpp
        Source/DB/DBClient/AutoCode/AreaTriggerRec.cpp
        Source/DB/DBClient/AutoCode/CameraShakesRec.cpp
        Source/DB/DBClient/AutoCode/CharStartOutfitRec.cpp
        Source/DB/DBClient/AutoCode/CharBaseInfoRec.cpp
        Source/DB/DBClient/AutoCode/CharVariationsRec.cpp
        Source/DB/DBClient/AutoCode/ChrProficiencyRec.cpp
        Source/DB/DBClient/AutoCode/ChrClassesRec.cpp
        Source/DB/DBClient/AutoCode/ChrRacesRec.cpp
        Source/DB/DBClient/AutoCode/CinematicCameraRec.cpp
        Source/DB/DBClient/AutoCode/CinematicSequencesRec.cpp
        Source/DB/DBClient/AutoCode/CreatureDisplayInfoExtraRec.cpp
        Source/DB/DBClient/AutoCode/CreatureDisplayInfoRec.cpp
        Source/DB/DBClient/AutoCode/CreatureFamilyRec.cpp
        Source/DB/DBClient/AutoCode/CreatureModelDataRec.cpp
        Source/DB/DBClient/AutoCode/CreatureSoundDataRec.cpp
        Source/DB/DBClient/AutoCode/CreatureTypeRec.cpp
        Source/DB/DBClient/AutoCode/FactionGroupRec.cpp
        Source/DB/DBClient/AutoCode/FactionRec.cpp
        Source/DB/DBClient/AutoCode/FactionTemplateRec.cpp
        Source/DB/DBClient/AutoCode/GroundEffectDoodadRec.cpp
        Source/DB/DBClient/AutoCode/GroundEffectTextureRec.cpp
        Source/DB/DBClient/AutoCode/ItemDisplayInfoRec.cpp
        Source/DB/DBClient/AutoCode/MaterialRec.cpp
        Source/DB/DBClient/AutoCode/PaperDollItemFrameRec.cpp
        Source/DB/DBClient/AutoCode/SkillLineAbilityRec.cpp
        Source/DB/DBClient/AutoCode/SkillLineRec.cpp
        Source/DB/DBClient/AutoCode/SpellIconRec.cpp
        Source/DB/DBClient/AutoCode/SpellRadiusRec.cpp
        Source/DB/DBClient/AutoCode/SpellRangeRec.cpp
        Source/DB/DBClient/AutoCode/SpellRec.cpp
        Source/DB/DBClient/AutoCode/SpellCastTimesRec.cpp
        Source/DB/DBClient/AutoCode/SpellDurationRec.cpp
        Source/DB/DBClient/AutoCode/SpellDispelTypeRec.cpp
        Source/DB/DBClient/AutoCode/SpellAuraNamesRec.cpp
        Source/DB/DBClient/AutoCode/SpellEffectNamesRec.cpp
        Source/DB/DBClient/AutoCode/SpellVisualEffectNameRec.cpp
        Source/DB/DBClient/AutoCode/SpellVisualKitRec.cpp
        Source/DB/DBClient/AutoCode/SpellVisualRec.cpp
        Source/DB/DBClient/AutoCode/TabardBackgroundTexturesRec.cpp
        Source/DB/DBClient/AutoCode/TabardEmblemTexturesRec.cpp
        Source/DB/DBClient/AutoCode/WeaponImpactSoundsRec.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy-"
    )
    set_source_files_properties(
        Source/DB/DBClient/DBClient.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy-"
    )
    set_source_files_properties(
        Source/DB/DBClient/DBCache.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy- /GX-"
    )
    set_source_files_properties(
        Source/DB/DBClient/DBCacheInstances.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy- /GX-"
    )
    set_source_files_properties(
        Source/Game/ValidateName.cpp
        PROPERTIES
            COMPILE_FLAGS "/G6 /Ob1 /Oy-"
            OBJECT_NAME ValidateName_C
    )
    set_source_files_properties(
        Source/UIUtil/Camera.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy-"
    )
    set_source_files_properties(
        Source/UIUtil/InputControl.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy- /GX-"
    )
    set_source_files_properties(
        Source/Net/NetClient/NetClient.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /GX-"
    )
    set_source_files_properties(
        Source/WowServices/WowConnection.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy- /GX-"
    )
    set_source_files_properties(
        Source/WowServices/WowConnectionNet.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy- /GX-"
    )
    set_source_files_properties(
        Source/WowServices/Linux/WowConnectionNetSelect.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy- /GX-"
    )
    set_source_files_properties(
        Source/Glue/CGlueMgr.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy- /GX-"
    )
    set_source_files_properties(
        Source/ObjectMgrClient/ObjectMgrClient.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy- /GX-"
    )
    set_source_files_properties(
        Source/Object/mirror.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /GX-"
    )
    set_source_files_properties(
        Source/ObjectAlloc/ObjectAlloc.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob2 /Oy- /GX-"
    )
    set_source_files_properties(
        Source/Object/ObjectClient/Player_C.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1"
    )
    set_source_files_properties(
        Source/Object/ObjectClient/Unit_C.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy- /GX-"
    )
    set_source_files_properties(
        Source/Net/NetInternal.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy- /GX-"
    )
    set_source_files_properties(
        Source/WowSvcs/WowSvcsClient/ClientServices.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /GX-"
    )
    set_source_files_properties(
        Source/WowSvcs/WowSvcsClient/FriendList.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /GX-"
    )
    set_source_files_properties(
        Source/Object/ObjectClient/ZoneDebug.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy- /GX-"
    )
    set_source_files_properties(
        Source/WorldClient/WorldParam.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy-"
    )
    set_source_files_properties(
        Source/UIUtil/PortraitButton.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy-"
    )
    set_source_files_properties(
        Source/WorldClient/MapQuery.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy- /QIfist"
    )
    set_source_files_properties(
        Source/WorldClient/DetailDoodad.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /GX-"
    )
    set_source_files_properties(
        Source/WorldClient/SimpleDoodad.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy- /GX-"
    )
    set_source_files_properties(
        Source/WorldClient/World.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy- /GX-"
    )
    set_source_files_properties(
        Source/WorldClient/Map.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy- /GX-"
    )
    set_source_files_properties(
        Source/WorldClient/AreaList.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy- /GX-"
    )

    set_property(SOURCE
        Source/Ui/SpellBookFrame.cpp
        Source/Console/ConsoleVar.cpp
        Source/DB/DBClient/sdbItemSubclass.cpp
        Source/UIUtil/Tooltip.cpp
        APPEND_STRING PROPERTY COMPILE_FLAGS " /Ob2")

endif()

if(MSVC_VERSION EQUAL 1200)
    set_property(SOURCE
        Source/Glue/CharCreateInfo.cpp
        Source/Glue/CharSelectInfo.cpp
        Source/Glue/GlueScriptEvents.cpp
        Source/Magic/MagicClient/Spell_C.cpp
        Source/SoundInterface/SoundInterface.cpp
        Source/SoundInterface/SoundInterfaceVocal.cpp
        Source/UIUtil/Camera.cpp
        Source/UIUtil/InputControl.cpp
        Source/UIUtil/ScriptEvents.cpp
        Source/UIUtil/Tooltip.cpp
        Source/Ui/ActionBarFrame.cpp
        Source/Ui/BankFrame.cpp
        Source/Ui/BuffFrame.cpp
        Source/Ui/CharacterModelBase.cpp
        Source/Ui/ChatFrame.cpp
        Source/Ui/ClassTrainerFrame.cpp
        Source/Ui/ContainerFrame.cpp
        Source/Ui/CraftInfo.cpp
        Source/Ui/DuelInfo.cpp
        Source/Ui/GameUI.cpp
        Source/Ui/GuildRegistrar.cpp
        Source/Ui/ItemTextFrame.cpp
        Source/Ui/LootFrame.cpp
        Source/Ui/MerchantFrame.cpp
        Source/Ui/MinimapFrame.cpp
        Source/Ui/PaperDollInfoFrame.cpp
        Source/Ui/PartyFrame.cpp
        Source/Ui/PetInfo.cpp
        Source/Ui/PetitionInfo.cpp
        Source/Ui/QuestFrame.cpp
        Source/Ui/QuestLog.cpp
        Source/Ui/ReputationInfo.cpp
        Source/Ui/SpellBookFrame.cpp
        Source/Ui/TabardCreationFrame.cpp
        Source/Ui/TabardModelFrame.cpp
        Source/Ui/TaxiMapFrame.cpp
        Source/Ui/TradeFrame.cpp
        Source/Ui/TradeSkillFrame.cpp
        Source/Ui/Tutorial.cpp
        Source/Ui/UIBindings.cpp
        Source/Ui/WorldMap.cpp
        Source/WowSvcs/WowSvcsClient/FriendList.cpp
        APPEND_STRING PROPERTY COMPILE_FLAGS " /Gr")

    set_source_files_properties(
        Source/WowServices/WDataStore.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy- /GX-"
    )
endif()

target_link_libraries(WowShared Engine Storm)

target_include_directories(WowShared
    PUBLIC
    Common/
    Source/
)

if(WIN32)
    add_executable(Wow WIN32
        Source/Client.cpp
        Source/Client.rc
    )

    if(MSVC_VERSION EQUAL 1200)
        target_compile_options(Wow PRIVATE /GX- /GR- /Oy-)
        target_compile_definitions(Wow PRIVATE WOW_STORM_ENTRYPOINT=1)
        set_property(TARGET Wow APPEND_STRING PROPERTY LINK_FLAGS " /ENTRY:StormStaticEntryPoint")
    endif()
elseif(APPLE)
    add_executable(Wow
        Source/Mac/Source/MacClient.cpp
        Source/Mac/Source/AboutBox.cpp
        Source/Mac/Source/FastTiming.cpp
        Source/Mac/Source/GetApplicationInformation.cpp
        Source/Mac/Source/MacClient.cpp
        Source/Mac/Source/MacClientMenus.cpp
        Source/Mac/Source/MoreCocoa.cpp
        Source/Mac/Source/MoreFilesExtras.cpp
        Source/Mac/Source/TemporaryItems.cpp
        Source/Mac/Source/Utils.cpp
    )
elseif(UNIX)
    add_executable(Wow
        Source/LinuxClient.cpp
    )
endif()

target_link_libraries(Wow WowShared)


if(WIN32)
    add_executable(UpdateClient
        Source/UpdateClient/UpdateClient.cpp
    )

    target_link_libraries(UpdateClient Storm)


    add_executable(WowError
        Source/WowError/WowError.cpp
    )

    target_link_libraries(WowError Storm)


    add_executable(WowUpdateHelper
        Source/WowUpdateHelper/WowUpdateHelper.cpp
    )

    target_link_libraries(WowUpdateHelper Storm)


endif()
