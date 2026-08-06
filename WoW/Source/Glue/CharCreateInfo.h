#pragma once

#include <stpl.h>
#include <string.h>

#include "Base/Base.h"
#include "Component/Component.h"
#include "Model/IModel.h"

class CSimpleModel;

extern LPCSTR g_glueBgObjNames[2];

struct CustomizationSelections {
  CustomizationSelections() {
  }

  UINT classID;
  UINT outfit;
  UINT skinColor;
  UINT hairColor;
  UINT hairStyle;
  UINT facialStyle;
  UINT face;
};

struct CHARCREATEINFO {
  CHARCREATEINFO() {
    Initialize();
  }

  ~CHARCREATEINFO() {
    Shutdown();
  }

  void Initialize() {
    memset(selections, 0, sizeof(selections));
    memset(currentGeosets, 0, sizeof(currentGeosets));

    for (UINT sex = 0; sex < 2; ++sex) {
      characterModel[sex] = 0;
      geosetHandle[sex] = 0;
      characterComponent[sex] = 0;
    }
  }

  void Shutdown();
  void UpdateOutfit(int increment, UINT race, UINT sex);
  void ResetOutfitSelection(UINT raceID, UINT sex);
  void CommitGeoset(UINT sex);
  void UpdateCharacterInfo(UINT race, UINT sex);
  void ChangeHairGeosets(UINT race, UINT sex);
  void UpdateEquipment(int doNotCommitGeosets, UINT race, UINT sex);
  void ChangeSkinTexture(int doNotCommitGeosets, UINT race, UINT sex);
  void ChangeFaceTexture(UINT race, UINT sex);
  void ChangeFacialHairTexture(UINT race, UINT sex);
  void RefreshVisibleGeosets(UINT sex) {
  }
  void ChangeFacialHairGeosets(UINT sex, UINT beardGeoset, UINT sideburnGeoset, UINT moustacheGeoset);
  void ChangeScalpHairTexture(UINT race, UINT sex);
  void UpdateGeosets(UINT beardGeoset, UINT sideBurnGeoset, UINT moustacheGeoset, UINT sex);
  void FindRange(UINT group, UINT *start, UINT *end);
  void CommitTexture(int race, int sex);

  HMODEL                  characterModel[2];
  HCHARGEOSET             geosetHandle[2];
  HTEXCOMPONENT           characterComponent[2];
  CustomizationSelections selections[2];
  float                   cameraHeight[2][2];
  float                   cameraRadius[2][2];
  float                   targetHeight[2][2];
  UINT                    currentGeosets[3][15];
};

class CCharCreateInfo {
 public:
  static void   CreateCharacter(LPCSTR name);
  static void   CycleCharCustomization(UINT index, int delta);
  static LPCSTR GetClassNameByIndex(UINT index);
  static float  GetCharFacing() {
    return m_charFacing;
  }
  static UINT GetNumCharCustomizations(UINT index);
  static UINT GetNumClasses() {
    return m_classIndex.Count();
  }
  static UINT GetNumRaces() {
    return m_raceIndex.Count();
  }
  static LPCSTR GetRaceNameByIndex(UINT index);
  static UINT   GetSelectedClassID();
  static UINT   GetSelectedClassIndex() {
    return m_selectedClass;
  }
  static UINT GetSelectedRaceID();
  static UINT GetSelectedRaceIndex() {
    return m_selectedRace;
  }
  static UINT                            GetSelectedSexID();
  static UINT                            GetNumOutfits(UINT raceID, UINT classID, UINT sexID);
  static const class CharStartOutfitRec *GetOutfit(UINT raceID, UINT classID, UINT sexID, UINT outfitID);
  static void                            Initialize();
  static void                            RandomizeCharCustomization();
  static void                            ResetCharCustomizeInfo();
  static void                            SetCharCustomizeFrame(CSimpleModel *frame);
  static void                            SetCharCustomizeModel(LPCSTR filename);
  static void                            SetCharFacing(float facing);
  static void                            SetSelectedClass(UINT index);
  static void                            SetSelectedRace(UINT index, int updateModel);
  static void                            SetSelectedSex(UINT sex);
  static void                            Shutdown();
  static void                            UpdateAvailableClasses();

 protected:
  static void UpdateAllCharacterInfo(int race, UINT sex);
  static void InitializeCharacterInfo(UINT sex, int doNotCommitGeosets);
  static void UpdateCharacterInfo(UINT sex);
  static void UpdateGeosets(UINT sex);
  static void UpdateEquipment(int doNotUpdateGeosets, UINT sex);
  static void ChangeSkinTexture(int doNotCommitGeosets, UINT sex);
  static void ChangeFaceTexture(UINT sex);
  static void ChangeFacialHairTexture(UINT sex);
  static void ChangeScalpHairTexture(UINT sex);
  static void ChangeHairGeosets(UINT sex);
  static void ChangeFacialHairGeosets(UINT sex);
  static void CommitCurrentGeoset(UINT sex);

 private:
  static CSimpleModel         *m_charCustomizeFrame;
  static TSFixedArray<UINT>    m_factionIndex;
  static TSFixedArray<UINT>    m_raceIndex;
  static int                   m_selectedRace;
  static TSGrowableArray<UINT> m_classIndex;
  static int                   m_selectedClass;
  static UINT                  m_selectedSex;
  static float                 m_charFacing;
  static CHARCREATEINFO        m_charInfo;
};

void CharCreateRegisterScriptFunctions();
void CharCreateUnregisterScriptFunctions();
void ReportMissingComponentTextures(UINT race, UINT sex);
