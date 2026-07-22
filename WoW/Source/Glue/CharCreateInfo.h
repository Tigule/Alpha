#pragma once

#include <stpl.h>

#include "Base/Base.h"
#include "Component/Component.h"
#include "Model/IModel.h"

class CSimpleModel;

extern const char *g_glueBgObjNames[2];

struct CustomizationSelections {
  uint classID;
  uint outfit;
  uint skinColor;
  uint hairColor;
  uint hairStyle;
  uint facialStyle;
  uint face;
};

struct CHARCREATEINFO {
  void Initialize();
  void Shutdown();
  void UpdateOutfit(int increment, uint race, uint sex);
  void ResetOutfitSelection(uint raceID, uint sex);
  void CommitGeoset(uint sex);
  void UpdateCharacterInfo(uint race, uint sex);
  void ChangeHairGeosets(uint race, uint sex);
  void UpdateEquipment(int doNotCommitGeosets, uint race, uint sex);
  void ChangeSkinTexture(int doNotCommitGeosets, uint race, uint sex);
  void ChangeFaceTexture(uint race, uint sex);
  void ChangeFacialHairTexture(uint race, uint sex);
  void RefreshVisibleGeosets(uint sex) {
  }
  void ChangeFacialHairGeosets(uint sex, uint beardGeoset, uint sideburnGeoset, uint moustacheGeoset);
  void ChangeScalpHairTexture(uint race, uint sex);
  void UpdateGeosets(uint beardGeoset, uint sideBurnGeoset, uint moustacheGeoset, uint sex);
  void FindRange(uint group, uint *start, uint *end);
  void CommitTexture(int race, int sex);

  HMODEL                  characterModel[2];
  HCHARGEOSET             geosetHandle[2];
  HTEXCOMPONENT           characterComponent[2];
  CustomizationSelections selections[2];
  float                   cameraHeight[2][2];
  float                   cameraRadius[2][2];
  float                   targetHeight[2][2];
  uint                    currentGeosets[3][15];
};

class CCharCreateInfo {
 public:
  static void __fastcall        CreateCharacter(const char *name);
  static void __fastcall        CycleCharCustomization(uint index, int delta);
  static const char *__fastcall GetClassNameByIndex(uint index);
  static float __fastcall       GetCharFacing() {
    return m_charFacing;
  }
  static uint __fastcall GetNumCharCustomizations(uint index);
  static uint __fastcall GetNumClasses() {
    return m_classIndex.Count();
  }
  static uint __fastcall GetNumRaces() {
    return m_raceIndex.Count();
  }
  static const char *__fastcall GetRaceNameByIndex(uint index);
  static uint __fastcall        GetSelectedClassID();
  static uint __fastcall        GetSelectedClassIndex() {
    return m_selectedClass;
  }
  static uint __fastcall GetSelectedRaceID();
  static uint __fastcall GetSelectedRaceIndex() {
    return m_selectedRace;
  }
  static uint __fastcall                      GetSelectedSexID();
  static uint __fastcall                      GetNumOutfits(uint raceID, uint classID, uint sexID);
  static class CharStartOutfitRec *__fastcall GetOutfit(uint raceID, uint classID, uint sexID, uint outfitID);
  static void __fastcall                      Initialize();
  static void __fastcall                      RandomizeCharCustomization();
  static void __fastcall                      ResetCharCustomizeInfo();
  static void __fastcall                      SetCharCustomizeFrame(CSimpleModel *frame);
  static void __fastcall                      SetCharCustomizeModel(const char *filename);
  static void __fastcall                      SetCharFacing(float facing);
  static void __fastcall                      SetSelectedClass(uint index);
  static void __fastcall                      SetSelectedRace(uint index, int updateModel);
  static void __fastcall                      SetSelectedSex(uint sex);
  static void __fastcall                      Shutdown();
  static void __fastcall                      UpdateAvailableClasses();

  static void __fastcall UpdateAllCharacterInfo(int race, uint sex);
  static void __fastcall InitializeCharacterInfo(uint sex, int doNotCommitGeosets);
  static void __fastcall UpdateCharacterInfo(uint sex);
  static void __fastcall UpdateGeosets(uint sex);
  static void __fastcall UpdateEquipment(int doNotUpdateGeosets, uint sex);
  static void __fastcall ChangeSkinTexture(int doNotCommitGeosets, uint sex);
  static void __fastcall ChangeFaceTexture(uint sex);
  static void __fastcall ChangeFacialHairTexture(uint sex);
  static void __fastcall ChangeScalpHairTexture(uint sex);
  static void __fastcall ChangeHairGeosets(uint sex);
  static void __fastcall ChangeFacialHairGeosets(uint sex);
  static void __fastcall CommitCurrentGeoset(uint sex);

 private:
  static CSimpleModel         *m_charCustomizeFrame;
  static TSFixedArray<uint>    m_factionIndex;
  static TSFixedArray<uint>    m_raceIndex;
  static int                   m_selectedRace;
  static TSGrowableArray<uint> m_classIndex;
  static int                   m_selectedClass;
  static uint                  m_selectedSex;
  static float                 m_charFacing;
  static CHARCREATEINFO        m_charInfo;
};

void __fastcall CharCreateRegisterScriptFunctions();
void __fastcall CharCreateUnregisterScriptFunctions();
void __fastcall ReportMissingComponentTextures(uint race, uint sex);
