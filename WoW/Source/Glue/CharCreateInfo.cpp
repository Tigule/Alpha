#include <Base/Base.h>
#include <WowConst.h>
#include "Glue/CGlueMgr.h"
#include <Frame/CSimpleFrame.h>
#include "WowSvcs/WowSvcsClient/ClientServices.h"
#include "Glue/CharCreateInfo.h"
#include "Glue/CharSelectInfo.h"
#include <Frame/CSimpleTop.h>
#include <Frame/CSimpleModel.h>
#include "SoundInterface/SoundInterface.h"
#include <Gx/CGxDevice.h>
#include "Object/ObjectClient/Unit_C.h"

#include "Glue/CharCreateInfo.h"

#include "DB/DBClient/AutoCode/ChrRacesRec.h"
#include "DB/DBClient/AutoCode/ChrClassesRec.h"
#include "DB/DBClient/AutoCode/CharBaseInfoRec.h"
#include "DB/DBClient/AutoCode/CharacterCreateCamerasRec.h"
#include "DB/DBClient/AutoCode/CharStartOutfitRec.h"
#include "DB/DBClient/AutoCode/CreatureModelDataRec.h"
#include "DB/DBClient/AutoCode/ItemDisplayInfoRec.h"
#include "DB/DBClient/AutoCode/FactionGroupRec.h"
#include "DB/DBClient/AutoCode/FactionTemplateRec.h"
#include "Frame/CSimpleModel.h"
#include "Frame/SimpleFrameRegistry.h"
#include "FrameScript/FrameScript.h"
#include "Glue/CGlueMgr.h"
#include "Client.h"
#include "Services/SysMessage.h"
#include "Base/Status.h"
#include "Component/CharacterCustomization.h"
#include "Component/Component.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"
#include "Tempest/cmath.h"
#include "Tempest/crandom.h"
#include "Object/ObjectClient/Object_C.h"
#include "Object/ObjectClient/Player_C.h"

#include <string.h>
#include <math.h>

#include <lauxlib.h>
#include <lua.h>

UINT CharCustomizationNumHairColors(UINT raceID, UINT sexID);
UINT CharCustomizationNumHairStyles(UINT raceID, UINT sexID);
UINT CharCustomizationNumBeardStyles(UINT raceID, UINT sexID);

static LPCSTR s_sexName[4] = {"male", "female", "sex unspecified", "unknown sex specification"};

static TEXCOMPONENT_SECTIONS s_removeSections[8] = {TCS_UPPERARM,   TCS_LOWERARM, TCS_HAND,     TCS_UPPERTORSO,
                                                    TCS_LOWERTORSO, TCS_LEGUPPER, TCS_LEGLOWER, TCS_FEET};

static UINT        s_startingLayer[8] = {1, 1, 1, 1, 1, 1, 1, 1};
static const float s_cameraTargetZ = 0.97222221f;
static const float s_cameraOrbitRadius = 11.666667f;
static const float s_cameraOrbitHeight = 8.333333f;

extern const int g_ITEMTYPEARRAY[];
void                    SetHandsState(HMODEL model, int itemSlot, int itemInventoryType);

CSimpleModel         *CCharCreateInfo::m_charCustomizeFrame;
TSFixedArray<UINT>    CCharCreateInfo::m_factionIndex;
TSFixedArray<UINT>    CCharCreateInfo::m_raceIndex;
int                   CCharCreateInfo::m_selectedRace = -1;
TSGrowableArray<UINT> CCharCreateInfo::m_classIndex;
int                   CCharCreateInfo::m_selectedClass;
UINT                  CCharCreateInfo::m_selectedSex;
float                 CCharCreateInfo::m_charFacing;
CHARCREATEINFO        CCharCreateInfo::m_charInfo;

static UINT RandomSelection(UINT numChoices) {
  if (!numChoices) {
    return 0;
  }
  return NTempest::CRandom::dice_(numChoices, g_rndSeed);
}

static int Script_SetCharCustomizeFrame(lua_State *L);
static int Script_SetCharCustomizeBackground(lua_State *L);
static int Script_ResetCharCustomize(lua_State *);
static int Script_GetNameForRace(lua_State *L);
static int Script_GetFactionForRace(lua_State *L);
static int Script_GetAvailableRaces(lua_State *L);
static int Script_GetClassesForRace(lua_State *L);
static int Script_GetSelectedRace(lua_State *L);
static int Script_GetSelectedSex(lua_State *L);
static int Script_GetSelectedClass(lua_State *L);
static int Script_SetSelectedRace(lua_State *L);
static int Script_SetSelectedSex(lua_State *L);
static int Script_SetSelectedClass(lua_State *L);
static int Script_UpdateCustomizationBackground(lua_State *);
static int Script_HasCharCustomization(lua_State *L);
static int Script_CycleCharCustomization(lua_State *L);
static int Script_RandomizeCharCustomization(lua_State *);
static int Script_GetCharacterFacing(lua_State *L);
static int Script_SetCharacterFacing(lua_State *L);
static int Script_CreateCharacter(lua_State *L);

static FrameScript_Method s_ScriptFunctions[20] = {
    {        "SetCharCustomizeFrame",         Script_SetCharCustomizeFrame},
    {   "SetCharCustomizeBackground",    Script_SetCharCustomizeBackground},
    {           "ResetCharCustomize",            Script_ResetCharCustomize},
    {               "GetNameForRace",                Script_GetNameForRace},
    {            "GetFactionForRace",             Script_GetFactionForRace},
    {            "GetAvailableRaces",             Script_GetAvailableRaces},
    {            "GetClassesForRace",             Script_GetClassesForRace},
    {              "GetSelectedRace",               Script_GetSelectedRace},
    {               "GetSelectedSex",                Script_GetSelectedSex},
    {             "GetSelectedClass",              Script_GetSelectedClass},
    {              "SetSelectedRace",               Script_SetSelectedRace},
    {               "SetSelectedSex",                Script_SetSelectedSex},
    {             "SetSelectedClass",              Script_SetSelectedClass},
    {"UpdateCustomizationBackground", Script_UpdateCustomizationBackground},
    {         "HasCharCustomization",          Script_HasCharCustomization},
    {       "CycleCharCustomization",        Script_CycleCharCustomization},
    {   "RandomizeCharCustomization",    Script_RandomizeCharCustomization},
    {           "GetCharacterFacing",            Script_GetCharacterFacing},
    {           "SetCharacterFacing",            Script_SetCharacterFacing},
    {              "CreateCharacter",               Script_CreateCharacter}
};

void CHARCREATEINFO::UpdateOutfit(int increment, UINT race, UINT sex) {
  FATALASSERT((increment <= 1 ) && (increment >= -1));
  FATALASSERT(sex < MAX_PLAYER_SEXES);

  CustomizationSelections &selection = selections[sex];
  UINT                     numOutfits = CCharCreateInfo::GetNumOutfits(race, selection.classID, sex);
  if (numOutfits) {
    selection.outfit = (increment + numOutfits + selection.outfit) % numOutfits;
    ChangeFaceTexture(race, sex);
    ChangeFacialHairTexture(race, sex);
    ChangeScalpHairTexture(race, sex);
  }
}

void CHARCREATEINFO::ResetOutfitSelection(UINT raceID, UINT sex) {
  FATALASSERT(sex < MAX_PLAYER_SEXES);

  CustomizationSelections &selection = selections[sex];
  UINT                     numOutfits = CCharCreateInfo::GetNumOutfits(raceID, selection.classID, sex);
  if (numOutfits) {
    selection.outfit %= numOutfits;
  } else {
    selection.outfit = 0;
  }
}

void CHARCREATEINFO::CommitGeoset(UINT sex) {
  FATALASSERT(sex < MAX_PLAYER_SEXES);
  CharCustomizationCommitGeosets(geosetHandle[sex]);
}

void ReportMissingComponentTextures(UINT race, UINT sex) {
  const ChrRacesRec *raceInfo;
  LPCSTR             raceName;

  sex = min(max(static_cast<int>(sex), 0), 3);
  raceInfo = g_chrRacesDB.GetRecord(race);
  raceName = raceInfo ? raceInfo->m_name_lang[CURRENT_LANGUAGE] : "unknown race";

  SysMsgPrintf(SYSMSG_WARNING, 0x10, "MODELHASNOCOMPONENTABLETEXTURES|%s|%d|%s|%d", raceName, race, s_sexName[sex], sex);
}

void CHARCREATEINFO::FindRange(UINT group, UINT *start, UINT *end) {
  FATALASSERT(start);
  FATALASSERT(end);
  *start = group * 100 + 1;
  *end = group * 100 + 99;
}

void CHARCREATEINFO::CommitTexture(int race, int sex) {
  if (sex < MAX_PLAYER_SEXES) {
    char    errorString[512];
    CStatus status;
    TexComponentCommitSections(&status, characterComponent[sex], 1);
    if (!status.IsEmpty()) {
      status.GetErrorStr(errorString, sizeof(errorString), STATUS_INFO);
      FATALERROR(("Race %d Sex %d: %s", race, sex, errorString));
    }
  }
}

void CCharCreateInfo::Initialize() {
  m_charInfo.Initialize();

  UINT                   numFactions;
  UINT                   i;
  UINT                   count = 0;
  const FactionGroupRec *group;
  UINT                   numRaces = 0;

  for (i = 0; i < static_cast<UINT>(g_chrRacesDB.GetNumRecords()); ++i) {
    const ChrRacesRec *race = g_chrRacesDB.GetRecordByIndex(i);

    if (!(race->m_flags & 1)) {
      ++numRaces;
    }
  }

  m_raceIndex.SetCount(numRaces);
  numFactions = g_factionGroupDB.GetNumRecords();

  for (i = 0; i < numFactions; ++i) {
    group = g_factionGroupDB.GetRecordByIndex(i);

    if (group->m_maskID == 1 || group->m_maskID == 2) {
      UINT factionCount = m_factionIndex.Count();
      UINT raceIndex;

      m_factionIndex.SetCount(factionCount + 1);
      m_factionIndex[factionCount] = group->m_ID;

      for (raceIndex = 0; raceIndex < numRaces; ++raceIndex) {
        const ChrRacesRec        *race = g_chrRacesDB.GetRecordByIndex(raceIndex);
        const FactionTemplateRec *factionTemplate;

        if (race->m_flags & 1) {
          continue;
        }

        factionTemplate = g_factionTemplateDB.GetRecord(race->m_factionID);
        if (factionTemplate && ((1 << group->m_maskID) & factionTemplate->m_factionGroup)) {
          m_raceIndex[count++] = race->m_ID;
        }
      }
    }
  }

  ASSERT(count == numRaces);
}

void CHARCREATEINFO::Shutdown() {
  UINT sex;

  for (sex = 0; sex < 2; ++sex) {
    if (characterModel[sex]) {
      HandleClose(characterModel[sex]);
    }
    if (geosetHandle[sex]) {
      HandleClose(geosetHandle[sex]);
    }
    if (characterComponent[sex]) {
      HandleClose(characterComponent[sex]);
    }

    characterModel[sex] = 0;
    geosetHandle[sex] = 0;
    characterComponent[sex] = 0;
  }
}

void CCharCreateInfo::SetCharCustomizeFrame(CSimpleModel *frame) {
  m_charCustomizeFrame = frame;
}

void CCharCreateInfo::SetCharCustomizeModel(LPCSTR filename) {
  if (!m_charCustomizeFrame || !filename || !*filename) {
    return;
  }

  CModelCreate createData;
  createData.flags = 4;
  createData.boneNames = g_glueBgObjNames;
  createData.numBones = 2;
  m_charCustomizeFrame->SetModel(filename, &createData, 0);
}

UINT CCharCreateInfo::GetNumOutfits(UINT raceID, UINT classID, UINT sexID) {
  UINT count = 0;
  for (int i = 0; i < g_charStartOutfitDB.GetNumRecords(); ++i) {
    const CharStartOutfitRec *outfit = g_charStartOutfitDB.GetRecordByIndex(i);
    if (outfit->m_raceID == raceID && outfit->m_classID == classID && outfit->m_sexID == sexID) {
      ++count;
    }
  }
  return count;
}

const CharStartOutfitRec *CCharCreateInfo::GetOutfit(UINT raceID, UINT classID, UINT sexID, UINT outfitID) {
  for (int i = 0; i < g_charStartOutfitDB.GetNumRecords(); ++i) {
    const CharStartOutfitRec *outfit = g_charStartOutfitDB.GetRecordByIndex(i);
    if (outfit->m_raceID == raceID && outfit->m_classID == classID && outfit->m_sexID == sexID && outfit->m_outfitID == outfitID) {
      return outfit;
    }
  }
  return 0;
}

void CCharCreateInfo::ResetCharCustomizeInfo() {
  if (m_charCustomizeFrame && ModelClearLink(m_charCustomizeFrame->GetModel(), 0) && m_raceIndex.Count()) {
    m_selectedRace = RandomSelection(m_raceIndex.Count());
    UpdateAvailableClasses();
    if (m_classIndex.Count()) {
      m_selectedClass = 0;
      m_selectedSex = RandomSelection(2);
      SetSelectedRace(m_selectedRace, 1);
    }
  }
}

void CCharCreateInfo::SetCharFacing(float facing) {
  m_charFacing = facing;

  HMODEL model = m_charCustomizeFrame ? m_charCustomizeFrame->GetModel() : 0;
  if (model) {
    NTempest::C3Vector facingVector;
    NTempest::CMath::sincos_(facing, facingVector.y, facingVector.x);
    ModelApplyObjectFaceDir(model, 0, facingVector);
  }
}

LPCSTR CCharCreateInfo::GetRaceNameByIndex(UINT index) {
  if (index >= m_raceIndex.Count()) {
    return 0;
  }

  const ChrRacesRec *race = g_chrRacesDB.GetRecord(m_raceIndex[index]);
  return race ? race->m_name_lang[CURRENT_LANGUAGE] : 0;
}

void CCharCreateInfo::UpdateAvailableClasses() {
  if (static_cast<UINT>(m_selectedRace) >= m_raceIndex.Count()) {
    return;
  }

  m_classIndex.SetCount(g_chrClassesDB.GetNumRecords());

  UINT numRecords = g_charBaseInfoDB.GetNumRecords();
  UINT count = 0;
  for (UINT i = 0; i < numRecords; ++i) {
    const CharBaseInfoRec *rec = g_charBaseInfoDB.GetRecordByIndex(i);
    if (rec->m_raceID == m_raceIndex[m_selectedRace]) {
      m_classIndex[count++] = rec->m_classID;
    }
  }

  m_classIndex.SetCount(count);
}

LPCSTR CCharCreateInfo::GetClassNameByIndex(UINT index) {
  if (index >= m_classIndex.Count()) {
    return 0;
  }

  const ChrClassesRec *classInfo = g_chrClassesDB.GetRecord(m_classIndex[index]);
  return classInfo ? classInfo->m_name_lang[CURRENT_LANGUAGE] : 0;
}

void CCharCreateInfo::Shutdown() {
  m_factionIndex.Clear();
  m_raceIndex.Clear();
  m_classIndex.Clear();
  m_charInfo.Shutdown();
}

UINT CCharCreateInfo::GetSelectedRaceID() {
  if (static_cast<UINT>(m_selectedRace) >= m_raceIndex.Count()) {
    return 0;
  }

  return m_raceIndex[m_selectedRace];
}

UINT CCharCreateInfo::GetSelectedSexID() {
  return m_selectedSex;
}

UINT CCharCreateInfo::GetSelectedClassID() {
  if (static_cast<UINT>(m_selectedClass) >= m_classIndex.Count()) {
    return 0;
  }

  return m_classIndex[m_selectedClass];
}

void CCharCreateInfo::UpdateAllCharacterInfo(int race, UINT sex) {
  FATALASSERT(sex < MAX_PLAYER_SEXES);
  InitializeCharacterInfo(sex, 1);
  m_charInfo.CommitTexture(race, sex);
  CommitCurrentGeoset(sex);
}

void CCharCreateInfo::InitializeCharacterInfo(UINT sex, int doNotCommitGeosets) {
  FATALASSERT(sex < UNITSEX_LAST);
  UINT race = GetSelectedRaceID();
  if (race) {
    m_charInfo.UpdateCharacterInfo(race, sex);
    UpdateGeosets(sex);
    ChangeSkinTexture(doNotCommitGeosets, sex);
    ChangeFaceTexture(sex);
    ChangeFacialHairTexture(sex);
    ChangeScalpHairTexture(sex);
    ChangeHairGeosets(sex);
  }
}

void CHARCREATEINFO::UpdateCharacterInfo(UINT race, UINT sex) {
  FATALASSERT(sex < MAX_PLAYER_SEXES);

  if (!characterModel[sex]) {
    const CreatureModelDataRec *modelInfo = Player_C_GetModelName(race, sex);
    if (!modelInfo || !*modelInfo->m_ModelName) {
      FATALERROR(("Error, model name for player race %d and sex %d not found!", race, sex));
      return;
    }

    if (geosetHandle[sex]) {
      HandleClose(geosetHandle[sex]);
    }
    if (characterModel[sex]) {
      HandleClose(characterModel[sex]);
    }
    if (characterComponent[sex]) {
      HandleClose(characterComponent[sex]);
    }

    characterModel[sex] = 0;
    characterComponent[sex] = 0;
    geosetHandle[sex] = 0;

    characterModel[sex] = ObjectModelCreate(modelInfo->m_ModelName, static_cast<OBJECT_TYPE>(25), 0x100800);
    geosetHandle[sex] = CharCustomizationCreateGeosetHandle(characterModel[sex]);
    FATALASSERT(geosetHandle[sex]);
    ModelSetSequence(characterModel[sex], 0, 0);
  }
}

void CHARCREATEINFO::UpdateEquipment(int doNotCommitGeosets, UINT race, UINT sex) {
  FATALASSERT(sex < MAX_PLAYER_SEXES);
  if (characterModel[sex]) {
    ModelClearAllLinks(characterModel[sex]);
  }
  CharCustomizationClearItemGeosets(geosetHandle[sex]);
  TexComponentRemoveSections(characterComponent[sex], s_removeSections, s_startingLayer, 8);
  TexComponentRemoveAllHolds(characterComponent[sex]);

  CustomizationSelections  &selection = selections[sex];
  const CharStartOutfitRec *outfit = CCharCreateInfo::GetOutfit(race, selection.classID, sex, selection.outfit);

  UINT itemInventoryTypes[20];
  UINT itemDisplayIDs[20];
  int  itemCount = 0;

  if (outfit) {
    for (int i = 0; i < 12; ++i) {
      if (outfit->m_ItemID[i] == -1) {
        continue;
      }
      int displayID = outfit->m_DisplayItemID[i];
      if (displayID == -1) {
        continue;
      }
      int inventoryType = outfit->m_InventoryType[i];
      if (inventoryType == -1 || inventoryType == INDEX_RANGED_TYPE || inventoryType == INDEX_THROWN_TYPE || inventoryType == INDEX_RANGEDRIGHT_TYPE) {
        continue;
      }

      itemDisplayIDs[itemCount] = displayID;
      itemInventoryTypes[itemCount] = inventoryType;
      ++itemCount;

      const ItemDisplayInfoRec *displayInfoRec = g_itemDisplayInfoDB.GetRecord(displayID);
      if (!displayInfoRec) {
        SysMsgPrintf(SYSMSG_WARNING, 0x10, "ITEMDISPLAYNOTFOUND|%d", displayID);
        continue;
      }

      int itemSlotNum = -1;
      switch (inventoryType) {
        case INDEX_NON_EQUIP_TYPE:
          continue;
        case INDEX_WEAPON_TYPE:
        case INDEX_2HWEAPON_TYPE:
        case INDEX_WEAPONMAINHAND_TYPE:
          itemSlotNum = 15;
          break;
        case INDEX_RANGED_TYPE:
        case INDEX_THROWN_TYPE:
        case INDEX_RANGEDRIGHT_TYPE:
          itemSlotNum = 17;
          break;
        case INDEX_SHIELD_TYPE:
        case INDEX_WEAPONOFFHAND_TYPE:
        case INDEX_HOLDABLE_TYPE:
          itemSlotNum = 16;
          break;
      }

      ObjComponentAdd(sex, race, 1, characterModel[sex], displayInfoRec, inventoryType, 0, 0, 0, 0, 0);

      if (g_ITEMTYPEARRAY[inventoryType] & 0x403F8) {
        if (characterComponent[sex]) {
          CStatus status;
          TexComponentAdd(&status, sex, characterComponent[sex], displayInfoRec, inventoryType, 1);
        } else {
          ReportMissingComponentTextures(race, sex);
        }
      }

      if (itemSlotNum != -1) {
        SetHandsState(characterModel[sex], itemSlotNum, inventoryType);
      }
      CharCustomizationAddItemGeosets(geosetHandle[sex], displayInfoRec, inventoryType, characterComponent[sex], race, 1);
    }
    CharCustomizationCommitItemGeosets(geosetHandle[sex], 1);
  }

  if (!doNotCommitGeosets) {
    CommitTexture(race, sex);
    CharCustomizationCommitGeosets(geosetHandle[sex]);
  }
}

void CHARCREATEINFO::ChangeSkinTexture(int doNotCommitGeosets, UINT race, UINT sex) {
  FATALASSERT(sex < MAX_PLAYER_SEXES);

  if (characterComponent[sex]) {
    HandleClose(characterComponent[sex]);
    characterComponent[sex] = 0;
  }

  HTEXTURE skinTexture = CharCustomizationSetSkin(characterModel[sex], race, sex, selections[sex].skinColor, 0);
  if (skinTexture) {
    characterComponent[sex] = TexComponentCreate(skinTexture, race, sex, selections[sex].skinColor, 0, 1);
    HandleClose(skinTexture);
    UpdateEquipment(doNotCommitGeosets, race, sex);
  }
}

void CHARCREATEINFO::ChangeFaceTexture(UINT race, UINT sex) {
  FATALASSERT(sex < MAX_PLAYER_SEXES);
  if (characterModel[sex] && characterComponent[sex]) {
    CharCustomizationSetFaceTexture(characterModel[sex], characterComponent[sex], race, sex, selections[sex].face, selections[sex].skinColor, 0);
  }
}

void CHARCREATEINFO::ChangeFacialHairTexture(UINT race, UINT sex) {
  FATALASSERT(sex < MAX_PLAYER_SEXES);
  if (characterModel[sex] && characterComponent[sex]) {
    CharCustomizationSetFacialTexture(
        characterModel[sex], characterComponent[sex], race, sex, selections[sex].facialStyle, selections[sex].hairColor
    );
  }
}

void CHARCREATEINFO::ChangeFacialHairGeosets(UINT sex, UINT beardGeoset, UINT sideburnGeoset, UINT moustacheGeoset) {
  FATALASSERT(sex < MAX_PLAYER_SEXES);
  FATALASSERT(geosetHandle[sex]);
  CharCustomizationShowGeoset(geosetHandle[sex], CHARGEOSET_BEARD, beardGeoset);
  CharCustomizationShowGeoset(geosetHandle[sex], CHARGEOSET_SIDEBURN, sideburnGeoset);
  CharCustomizationShowGeoset(geosetHandle[sex], CHARGEOSET_MOUSTACHE, moustacheGeoset);
}

void CHARCREATEINFO::ChangeScalpHairTexture(UINT race, UINT sex) {
  if (characterModel[sex] && characterComponent[sex]) {
    CharCustomizationSetHairTexture(characterModel[sex], characterComponent[sex], race, sex, selections[sex].hairStyle, selections[sex].hairColor);
  }
}

void CHARCREATEINFO::ChangeHairGeosets(UINT race, UINT sex) {
  CharCustomizationResetHairGeoset(geosetHandle[sex], race, sex, selections[sex].hairStyle);
}

void CHARCREATEINFO::UpdateGeosets(UINT beardGeoset, UINT sideBurnGeoset, UINT moustacheGeoset, UINT sex) {
  FATALASSERT(sex < MAX_PLAYER_SEXES);
  CharCustomizationInitBaseCharacter(geosetHandle[sex], beardGeoset, sideBurnGeoset, moustacheGeoset, 2);
}

void CCharCreateInfo::CommitCurrentGeoset(UINT sex) {
  FATALASSERT(sex < UNITSEX_LAST);
  m_charInfo.CommitGeoset(sex);
}

void CCharCreateInfo::UpdateCharacterInfo(UINT sex) {
  FATALASSERT(sex < UNITSEX_LAST);
  UINT race = GetSelectedRaceID();
  if (race) {
    m_charInfo.UpdateCharacterInfo(race, sex);
    ChangeSkinTexture(0, sex);
    ChangeFaceTexture(sex);
    ChangeFacialHairTexture(sex);
    ChangeScalpHairTexture(sex);
  }
}

void CCharCreateInfo::UpdateEquipment(int doNotUpdateGeosets, UINT sex) {
  FATALASSERT(sex < UNITSEX_LAST);
  UINT race = GetSelectedRaceID();
  if (race) {
    m_charInfo.UpdateEquipment(doNotUpdateGeosets, race, sex);
  }
}

void CCharCreateInfo::ChangeSkinTexture(int doNotCommitGeosets, UINT sex) {
  FATALASSERT(sex < UNITSEX_LAST);
  UINT race = GetSelectedRaceID();
  if (race) {
    m_charInfo.ChangeSkinTexture(doNotCommitGeosets, race, sex);
  }
}

void CCharCreateInfo::ChangeFaceTexture(UINT sex) {
  FATALASSERT(sex < UNITSEX_LAST);
  UINT race = GetSelectedRaceID();
  if (race) {
    m_charInfo.ChangeFaceTexture(race, sex);
  }
}

void CCharCreateInfo::ChangeFacialHairTexture(UINT sex) {
  FATALASSERT(sex < UNITSEX_LAST);
  UINT race = GetSelectedRaceID();
  if (race) {
    m_charInfo.ChangeFacialHairTexture(race, sex);
  }
}

void CCharCreateInfo::ChangeFacialHairGeosets(UINT sex) {
  FATALASSERT(sex < UNITSEX_LAST);
  UINT race = GetSelectedRaceID();
  if (race) {
    BEARDSTYLEDATA facialData;
    CharCustomizationGetBeardStyle(race, sex, m_charInfo.selections[sex].facialStyle, &facialData);
    m_charInfo.ChangeFacialHairGeosets(sex, facialData.beardGeoset, facialData.sideBurnGeoset, facialData.moustacheGeoset);
  }
}

void CCharCreateInfo::ChangeScalpHairTexture(UINT sex) {
  FATALASSERT(sex < UNITSEX_LAST);
  UINT race = GetSelectedRaceID();
  if (race) {
    m_charInfo.ChangeScalpHairTexture(race, sex);
  }
}

void CCharCreateInfo::ChangeHairGeosets(UINT sex) {
  FATALASSERT(sex < UNITSEX_LAST);
  UINT race = GetSelectedRaceID();
  if (race) {
    m_charInfo.ChangeHairGeosets(race, sex);
  }
}

void CCharCreateInfo::UpdateGeosets(UINT sex) {
  FATALASSERT(sex < UNITSEX_LAST);
  UINT race = GetSelectedRaceID();
  if (race) {
    BEARDSTYLEDATA facialData;
    CharCustomizationGetBeardStyle(race, sex, m_charInfo.selections[sex].facialStyle, &facialData);
    m_charInfo.UpdateGeosets(facialData.beardGeoset, facialData.sideBurnGeoset, facialData.moustacheGeoset, sex);
  }
}

void CCharCreateInfo::SetSelectedRace(UINT index, int updateModel) {
  if (index >= m_raceIndex.Count()) {
    return;
  }

  m_selectedRace = index;
  UpdateAvailableClasses();
  if (!updateModel) {
    return;
  }

  m_charInfo.Shutdown();
  m_charInfo.Initialize();

  m_selectedClass = 0;
  UINT classID = m_classIndex[0];
  index = m_raceIndex[m_selectedRace];
  memset(m_charInfo.selections, 0, sizeof(m_charInfo.selections));

  {
    for (UINT sex = 0; sex < 2; ++sex) {
      int                      pcVars;
      int                      pcFaceVars;
      int                      npcFaceVars;
      CustomizationSelections &selection = m_charInfo.selections[sex];

      CharCustomizationGetNumSkinTextures(index, sex, &pcVars, 0);
      CharCustomizationNumFaces(index, sex, &pcFaceVars, &npcFaceVars);

      selection.classID = classID;
      selection.outfit = 0;
      selection.skinColor = RandomSelection(pcVars);
      selection.hairColor = RandomSelection(CharCustomizationNumHairColors(index, sex));
      selection.hairStyle = RandomSelection(CharCustomizationNumHairStyles(index, sex));
      selection.facialStyle = RandomSelection(CharCustomizationNumBeardStyles(index, sex));
      selection.face = RandomSelection(pcFaceVars);

      m_charInfo.cameraHeight[sex][0] = s_cameraOrbitHeight;
      m_charInfo.cameraHeight[sex][1] = s_cameraOrbitHeight;
      m_charInfo.cameraRadius[sex][0] = s_cameraOrbitRadius;
      m_charInfo.cameraRadius[sex][1] = s_cameraOrbitRadius;
      m_charInfo.targetHeight[sex][0] = s_cameraTargetZ;
      m_charInfo.targetHeight[sex][1] = s_cameraTargetZ;
    }
  }

  for (int i = g_characterCreateCamerasDB.GetNumRecords(); i--;) {
    const CharacterCreateCamerasRec *rec = g_characterCreateCamerasDB.GetRecordByIndex(i);
    FATALASSERT(rec);
    if (rec->m_Race == static_cast<int>(index)) {
      UINT sex = rec->m_Sex;
      UINT camera = rec->m_Camera;
      if (sex < UNITSEX_LAST && camera < 2) {
        m_charInfo.cameraHeight[sex][camera] = rec->m_Height * 0.027777778f;
        m_charInfo.cameraRadius[sex][camera] = 0.5f * (rec->m_Radius * 0.027777778f);
        m_charInfo.targetHeight[sex][camera] = rec->m_Target * 0.027777778f;
      }
    }
  }

  UpdateAllCharacterInfo(index, m_selectedSex);

  HMODEL model = m_charCustomizeFrame ? m_charCustomizeFrame->GetModel() : 0;
  if (model && ModelClearLink(model, 0)) {
    ModelAddLink(model, 0, m_charInfo.characterModel[m_selectedSex], 1.0f);
    NTempest::C3Vector facingVector;
    NTempest::CMath::sincos_(m_charFacing, facingVector.y, facingVector.x);
    ModelApplyObjectFaceDir(model, 0, facingVector);
  }
}

void CCharCreateInfo::SetSelectedSex(UINT sex) {
  if (sex < UNITSEX_LAST && sex != m_selectedSex) {
    m_selectedSex = sex;
    m_charInfo.UpdateOutfit(0, GetSelectedRaceID(), sex);
    UpdateAllCharacterInfo(GetSelectedRaceID(), sex);

    HMODEL model = m_charCustomizeFrame ? m_charCustomizeFrame->GetModel() : 0;
    if (model && ModelClearLink(model, 0)) {
      ModelAddLink(model, 0, m_charInfo.characterModel[sex], 1.0f);
      NTempest::C3Vector facingVector;
      NTempest::CMath::sincos_(m_charFacing, facingVector.y, facingVector.x);
      ModelApplyObjectFaceDir(model, 0, facingVector);
    }
  }
}

void CCharCreateInfo::SetSelectedClass(UINT index) {
  if (index < m_classIndex.Count() && index != m_selectedClass) {
    m_selectedClass = index;
    m_charInfo.selections[0].classID = m_classIndex[index];
    m_charInfo.selections[1].classID = m_classIndex[index];
    m_charInfo.ResetOutfitSelection(GetSelectedRaceID(), m_selectedSex);
    UpdateEquipment(1, m_selectedSex);
    UpdateCharacterInfo(m_selectedSex);
    m_charInfo.UpdateOutfit(0, GetSelectedRaceID(), m_selectedSex);
    m_charInfo.CommitTexture(GetSelectedRaceID(), m_selectedSex);
    CommitCurrentGeoset(m_selectedSex);
  }
}

UINT CCharCreateInfo::GetNumCharCustomizations(UINT index) {
  UINT race = GetSelectedRaceID();
  if (!race) {
    return 0;
  }

  int numVariations = 0;
  switch (index) {
    case 0:
      CharCustomizationGetNumSkinTextures(race, m_selectedSex, &numVariations, 0);
      break;
    case 1:
      CharCustomizationNumFaces(race, m_selectedSex, &numVariations, 0);
      break;
    case 2:
      numVariations = CharCustomizationNumHairStyles(race, m_selectedSex);
      break;
    case 3:
      numVariations = CharCustomizationNumHairColors(race, m_selectedSex);
      break;
    case 4:
      numVariations = CharCustomizationNumBeardStyles(race, m_selectedSex);
      break;
  }
  return numVariations;
}

void CCharCreateInfo::CycleCharCustomization(UINT index, int delta) {
  if (!delta) {
    return;
  }

  UINT race = GetSelectedRaceID();
  if (!race) {
    return;
  }

  UINT seqTime = ModelGetSequenceTime(m_charInfo.characterModel[m_selectedSex], 0);

  switch (index) {
    case 0: {
      int numSkinColors;
      CharCustomizationGetNumSkinTextures(race, m_selectedSex, &numSkinColors, 0);
      if (numSkinColors >= 2) {
        if (delta < 0) {
          m_charInfo.selections[m_selectedSex].skinColor += numSkinColors - 1;
        } else {
          m_charInfo.selections[m_selectedSex].skinColor++;
        }
        m_charInfo.selections[m_selectedSex].skinColor %= numSkinColors;

        ChangeSkinTexture(1, m_selectedSex);
        ChangeFaceTexture(m_selectedSex);
        ChangeFacialHairTexture(m_selectedSex);
        ChangeScalpHairTexture(m_selectedSex);
        m_charInfo.CommitTexture(race, m_selectedSex);
        CharCustomizationCommitGeosets(m_charInfo.geosetHandle[m_selectedSex]);
      }
      break;
    }

    case 1: {
      int pcVars;
      CharCustomizationNumFaces(race, m_selectedSex, &pcVars, 0);
      if (pcVars >= 2) {
        if (delta < 0) {
          m_charInfo.selections[m_selectedSex].face += pcVars - 1;
        } else {
          m_charInfo.selections[m_selectedSex].face++;
        }
        m_charInfo.selections[m_selectedSex].face %= pcVars;

        ChangeFaceTexture(m_selectedSex);

        m_charInfo.CommitTexture(race, m_selectedSex);
      }
      break;
    }

    case 2: {
      UINT numHairStyles = CharCustomizationNumHairStyles(race, m_selectedSex);
      if (numHairStyles >= 2) {
        if (delta < 0) {
          m_charInfo.selections[m_selectedSex].hairStyle += numHairStyles - 1;
        } else {
          m_charInfo.selections[m_selectedSex].hairStyle++;
        }
        m_charInfo.selections[m_selectedSex].hairStyle %= numHairStyles;

        ChangeScalpHairTexture(m_selectedSex);
        ChangeHairGeosets(m_selectedSex);
        CommitCurrentGeoset(m_selectedSex);
        m_charInfo.CommitTexture(race, m_selectedSex);
      }
      break;
    }

    case 3: {
      UINT numHairColors = CharCustomizationNumHairColors(race, m_selectedSex);
      if (numHairColors >= 2) {
        if (delta < 0) {
          m_charInfo.selections[m_selectedSex].hairColor += numHairColors - 1;
        } else {
          m_charInfo.selections[m_selectedSex].hairColor++;
        }
        m_charInfo.selections[m_selectedSex].hairColor %= numHairColors;

        ChangeFaceTexture(m_selectedSex);
        ChangeFacialHairTexture(m_selectedSex);
        ChangeScalpHairTexture(m_selectedSex);

        m_charInfo.CommitTexture(race, m_selectedSex);
      }
      break;
    }

    case 4: {
      UINT numFacialStyles = CharCustomizationNumBeardStyles(race, m_selectedSex);
      if (numFacialStyles >= 2) {
        if (delta < 0) {
          m_charInfo.selections[m_selectedSex].facialStyle += numFacialStyles - 1;
        } else {
          m_charInfo.selections[m_selectedSex].facialStyle++;
        }
        m_charInfo.selections[m_selectedSex].facialStyle %= numFacialStyles;

        ChangeFacialHairTexture(m_selectedSex);
        ChangeFacialHairGeosets(m_selectedSex);
        CommitCurrentGeoset(m_selectedSex);
        m_charInfo.CommitTexture(race, m_selectedSex);
      }
      break;
    }

    case 5:
      m_charInfo.UpdateOutfit(delta, race, m_selectedSex);

      ChangeSkinTexture(1, m_selectedSex);
      m_charInfo.CommitTexture(race, m_selectedSex);
      CharCustomizationCommitGeosets(m_charInfo.geosetHandle[m_selectedSex]);
      break;
  }

  ModelForceSequenceTime(m_charInfo.characterModel[m_selectedSex], 0, seqTime, 0);
}

void CCharCreateInfo::RandomizeCharCustomization() {
  UINT race = GetSelectedRaceID();
  if (!race) {
    return;
  }

  UINT seqTime = ModelGetSequenceTime(m_charInfo.characterModel[m_selectedSex], 0);

  int skinColors;
  CharCustomizationGetNumSkinTextures(race, m_selectedSex, &skinColors, 0);
  CustomizationSelections &selection = m_charInfo.selections[m_selectedSex];
  int                      PCFaceColors;
  CharCustomizationNumFaces(race, m_selectedSex, &PCFaceColors, 0);

  selection.outfit = 0;
  selection.skinColor = RandomSelection(skinColors);
  selection.hairColor = RandomSelection(CharCustomizationNumHairColors(race, m_selectedSex));
  selection.hairStyle = RandomSelection(CharCustomizationNumHairStyles(race, m_selectedSex));
  selection.facialStyle = RandomSelection(CharCustomizationNumBeardStyles(race, m_selectedSex));
  selection.face = RandomSelection(PCFaceColors);

  UpdateAllCharacterInfo(race, m_selectedSex);

  ModelForceSequenceTime(m_charInfo.characterModel[m_selectedSex], 0, seqTime, 0);
}

void CCharCreateInfo::CreateCharacter(LPCSTR name) {
  CHAR_NAME_RESULT result = CHAR_NAME_RESULT_START;
  if (name) {
    result = ClientServices_CharacterValidateName(name);
  }
  if (result != CHAR_NAME_SUCCESS) {
    FrameScript_SignalEvent(3, "%s%s", "OKAY", FrameScript_GetText(ClientServices_GetErrorToken(result), -1, GENDER_NOT_APPLICABLE));
  } else {
    CHARACTER_CREATE_INFO createInfo;
    SStrCopy(createInfo.name, name, sizeof(createInfo.name));
    createInfo.raceID = static_cast<BYTE>(GetSelectedRaceID());
    createInfo.sexID = static_cast<BYTE>(m_selectedSex);
    createInfo.classID = static_cast<BYTE>(m_charInfo.selections[m_selectedSex].classID);
    createInfo.outfitID = static_cast<BYTE>(m_charInfo.selections[m_selectedSex].outfit);
    createInfo.skinID = static_cast<BYTE>(m_charInfo.selections[m_selectedSex].skinColor);
    createInfo.hairColorID = static_cast<BYTE>(m_charInfo.selections[m_selectedSex].hairColor);
    createInfo.hairStyleID = static_cast<BYTE>(m_charInfo.selections[m_selectedSex].hairStyle);
    createInfo.facialHairStyleID = static_cast<BYTE>(m_charInfo.selections[m_selectedSex].facialStyle);
    createInfo.faceID = static_cast<BYTE>(m_charInfo.selections[m_selectedSex].face);
    CGlueMgr::CreateCharacter(&createInfo);
  }
}

static int Script_SetCharCustomizeFrame(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: SetCharCustomizeFrame(\"frameName\")");
    return 0;
  }

  CSimpleFrame *frame = SimpleFrameRegistryGetEntry(lua_tostring(L, 1), 0);
  if (frame) {
    CCharCreateInfo::SetCharCustomizeFrame(static_cast<CSimpleModel *>(frame));
  }

  return 0;
}

static int Script_SetCharCustomizeBackground(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: SetCharCustomizeBackground(\"filename\")");
    return 0;
  }

  CCharCreateInfo::SetCharCustomizeModel(lua_tostring(L, 1));
  return 0;
}

static int Script_ResetCharCustomize(lua_State *) {
  CCharCreateInfo::ResetCharCustomizeInfo();
  return 0;
}

static int Script_GetNameForRace(lua_State *L) {
  const ChrRacesRec *race = g_chrRacesDB.GetRecord(CCharCreateInfo::GetSelectedRaceID());
  if (race) {
    lua_pushstring(L, race->m_name_lang[CURRENT_LANGUAGE]);
    lua_pushstring(L, race->m_clientFileString);
  } else {
    lua_pushnil(L);
    lua_pushnil(L);
  }
  return 2;
}

static int Script_GetFactionForRace(lua_State *L) {
  const ChrRacesRec        *race = g_chrRacesDB.GetRecord(CCharCreateInfo::GetSelectedRaceID());
  const FactionTemplateRec *faction;
  faction = race ? g_factionTemplateDB.GetRecord(race->m_factionID) : 0;

  if (faction) {
    for (UINT i = 0; i < static_cast<UINT>(g_factionGroupDB.GetNumRecords()); ++i) {
      const FactionGroupRec *group = g_factionGroupDB.GetRecordByIndex(i);
      if (group && ((1 << group->m_maskID) & faction->m_factionGroup) && group->m_name_lang[CURRENT_LANGUAGE][0]) {
        lua_pushstring(L, group->m_name_lang[CURRENT_LANGUAGE]);
        lua_pushstring(L, group->m_internalName);
        return 2;
      }
    }
  }

  lua_pushnil(L);
  lua_pushnil(L);
  return 2;
}

static int Script_GetAvailableRaces(lua_State *L) {
  UINT count = CCharCreateInfo::GetNumRaces();
  for (UINT i = 0; i < count; ++i) {
    lua_pushstring(L, CCharCreateInfo::GetRaceNameByIndex(i));
  }
  return count;
}

static int Script_GetClassesForRace(lua_State *L) {
  UINT count = CCharCreateInfo::GetNumClasses();
  for (UINT i = 0; i < count; ++i) {
    lua_pushstring(L, CCharCreateInfo::GetClassNameByIndex(i));
  }
  return count;
}

static int Script_GetSelectedRace(lua_State *L) {
  const ChrRacesRec *race = g_chrRacesDB.GetRecord(CCharCreateInfo::GetSelectedRaceID());
  lua_pushnumber(L, CCharCreateInfo::GetSelectedRaceIndex() + 1);
  lua_pushstring(L, race ? race->m_name_lang[CURRENT_LANGUAGE] : 0);
  return 2;
}

static int Script_GetSelectedSex(lua_State *L) {
  lua_pushnumber(L, CCharCreateInfo::GetSelectedSexID() + 1);
  return 1;
}

static int Script_GetSelectedClass(lua_State *L) {
  const ChrClassesRec *classInfo = g_chrClassesDB.GetRecord(CCharCreateInfo::GetSelectedClassID());
  lua_pushnumber(L, CCharCreateInfo::GetSelectedClassIndex() + 1);
  lua_pushstring(L, classInfo ? classInfo->m_name_lang[CURRENT_LANGUAGE] : 0);
  return 2;
}

static int Script_SetSelectedRace(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: SetSelectedRace(index)");
    return 0;
  }
  UINT index = static_cast<UINT>(lua_tonumber(L, 1)) - 1;
  CCharCreateInfo::SetSelectedRace(index, 0);
  return 0;
}

static int Script_SetSelectedSex(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: SetSelectedSex(index)");
    return 0;
  }
  UINT index = static_cast<UINT>(lua_tonumber(L, 1)) - 1;
  CCharCreateInfo::SetSelectedSex(index);
  return 0;
}

static int Script_SetSelectedClass(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: SetSelectedClass(index)");
    return 0;
  }
  UINT index = static_cast<UINT>(lua_tonumber(L, 1)) - 1;
  CCharCreateInfo::SetSelectedClass(index);
  return 0;
}

static int Script_UpdateCustomizationBackground(lua_State *) {
  CCharCreateInfo::SetSelectedRace(CCharCreateInfo::GetSelectedRaceIndex(), 1);
  return 0;
}

static int Script_HasCharCustomization(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: HasCharCustomization(index)");
    return 0;
  }

  UINT index = static_cast<UINT>(lua_tonumber(L, 1)) - 1;
  if (CCharCreateInfo::GetNumCharCustomizations(index) > 1) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_CycleCharCustomization(lua_State *L) {
  if (lua_isnumber(L, 1) && lua_isnumber(L, 2)) {
    int index = static_cast<int>(lua_tonumber(L, 1)) - 1;
    int delta = static_cast<int>(lua_tonumber(L, 2));
    CCharCreateInfo::CycleCharCustomization(index, delta);
    return 0;
  }
  luaL_error(L, "Usage: CycleCharCustomization(index, delta)");
  return 0;
}

static int Script_RandomizeCharCustomization(lua_State *) {
  CCharCreateInfo::RandomizeCharCustomization();
  return 0;
}

static int Script_GetCharacterFacing(lua_State *L) {
  lua_pushnumber(L, CCharCreateInfo::GetCharFacing() * 57.29578f);
  return 1;
}

static int Script_SetCharacterFacing(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: SetCharacterFacing(degrees)");
    return 0;
  }
  CCharCreateInfo::SetCharFacing(static_cast<float>(lua_tonumber(L, 1)) * 0.017453292f);
  return 1;
}

static int Script_CreateCharacter(lua_State *L) {
  if (lua_isstring(L, 1)) {
    lua_tostring(L, 1);
  }
  CCharCreateInfo::CreateCharacter(lua_tostring(L, 1));
  return 0;
}

void CharCreateRegisterScriptFunctions() {
  for (UINT i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void CharCreateUnregisterScriptFunctions() {
  for (UINT i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
