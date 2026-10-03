#pragma once

#include <stpl.h>
#include <string.h>

#include "Base/Base.h"
#include "Component/Component.h"
#include "Model/IModel.h"

#define MAX_PLAYER_SEXES 2u

class CSimpleModel;

extern LPCSTR g_glueBgObjNames[2];

struct CustomizationSelections {
  UINT classID;
  UINT outfit;
  UINT skinColor;
  UINT hairColor;
  UINT hairStyle;
  UINT facialStyle;
  UINT face;

  CustomizationSelections() : outfit(0), skinColor(0), hairColor(0), hairStyle(0), facialStyle(0), face(0) {
  }
};

struct CHARCREATEINFO {
  HMODEL                  characterModel[2];
  HCHARGEOSET             geosetHandle[2];
  HTEXCOMPONENT           characterComponent[2];
  CustomizationSelections selections[2];
  float                   cameraHeight[2][2];
  float                   cameraRadius[2][2];
  float                   targetHeight[2][2];
  UINT                    currentGeosets[3][15];

  CHARCREATEINFO() {
    Initialize();
  }

  ~CHARCREATEINFO() {
    Shutdown();
  }

  void Initialize() {
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
};

class CCharCreateInfo {
 public:
  static void Initialize();
  static void Shutdown();
  static void SetCharCustomizeFrame(CSimpleModel *frame);
  static void SetCharCustomizeModel(LPCSTR filename);
  static void ResetCharCustomizeInfo();
  static float  GetCharFacing() {
    return m_charFacing;
  }
  static void SetCharFacing(float facing);
  static UINT GetNumRaces() {
    return m_raceIndex.Count();
  }
  static LPCSTR GetRaceNameByIndex(UINT index);
  static void   UpdateAvailableClasses();
  static UINT GetNumClasses() {
    return m_classIndex.Count();
  }
  static LPCSTR GetClassNameByIndex(UINT index);
  static UINT   GetSelectedRaceID();
  static UINT GetSelectedRaceIndex() {
    return m_selectedRace;
  }
  static UINT GetSelectedSexID();
  static UINT GetSelectedClassID();
  static UINT   GetSelectedClassIndex() {
    return m_selectedClass;
  }
  static UINT                            GetNumOutfits(UINT raceID, UINT classID, UINT sexID);
  static const class CharStartOutfitRec *GetOutfit(UINT raceID, UINT classID, UINT sexID, UINT outfitID);
  static void                            SetSelectedRace(UINT index, int updateModel);
  static void                            SetSelectedSex(UINT sex);
  static void                            SetSelectedClass(UINT index);
  static UINT                            GetNumCharCustomizations(UINT index);
  static void                            CycleCharCustomization(UINT index, int delta);
  static void                            RandomizeCharCustomization();
  static void                            CreateCharacter(LPCSTR name);

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
