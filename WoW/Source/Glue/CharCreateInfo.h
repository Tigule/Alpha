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
  static void CreateCharacter(const char *name);
  static void CycleCharCustomization(uint index, int delta);
  static const char *GetClassNameByIndex(uint index);
  static float GetCharFacing() {
    return m_charFacing;
  }
  static uint GetNumCharCustomizations(uint index);
  static uint GetNumClasses() {
    return m_classIndex.Count();
  }
  static uint GetNumRaces() {
    return m_raceIndex.Count();
  }
  static const char *GetRaceNameByIndex(uint index);
  static uint GetSelectedClassID();
  static uint GetSelectedClassIndex() {
    return m_selectedClass;
  }
  static uint GetSelectedRaceID();
  static uint GetSelectedRaceIndex() {
    return m_selectedRace;
  }
  static uint GetSelectedSexID();
  static uint GetNumOutfits(uint raceID, uint classID, uint sexID);
  static class CharStartOutfitRec *GetOutfit(uint raceID, uint classID, uint sexID, uint outfitID);
  static void Initialize();
  static void RandomizeCharCustomization();
  static void ResetCharCustomizeInfo();
  static void SetCharCustomizeFrame(CSimpleModel *frame);
  static void SetCharCustomizeModel(const char *filename);
  static void SetCharFacing(float facing);
  static void SetSelectedClass(uint index);
  static void SetSelectedRace(uint index, int updateModel);
  static void SetSelectedSex(uint sex);
  static void Shutdown();
  static void UpdateAvailableClasses();

  static void UpdateAllCharacterInfo(int race, uint sex);
  static void InitializeCharacterInfo(uint sex, int doNotCommitGeosets);
  static void UpdateCharacterInfo(uint sex);
  static void UpdateGeosets(uint sex);
  static void UpdateEquipment(int doNotUpdateGeosets, uint sex);
  static void ChangeSkinTexture(int doNotCommitGeosets, uint sex);
  static void ChangeFaceTexture(uint sex);
  static void ChangeFacialHairTexture(uint sex);
  static void ChangeScalpHairTexture(uint sex);
  static void ChangeHairGeosets(uint sex);
  static void ChangeFacialHairGeosets(uint sex);
  static void CommitCurrentGeoset(uint sex);

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

void CharCreateRegisterScriptFunctions();
void CharCreateUnregisterScriptFunctions();
void ReportMissingComponentTextures(uint race, uint sex);
