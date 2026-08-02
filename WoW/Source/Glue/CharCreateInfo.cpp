#include <WowConst.h>

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
#include "Object/ObjectClient/Object_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"
#include "Tempest/cmath.h"
#include "Tempest/crandom.h"

#include <string.h>
#include <math.h>

#include <lauxlib.h>
#include <lua.h>

unsigned int CharCustomizationNumHairColors(unsigned int raceID, unsigned int sexID);
unsigned int CharCustomizationNumHairStyles(unsigned int raceID, unsigned int sexID);
unsigned int CharCustomizationNumBeardStyles(unsigned int raceID, unsigned int sexID);

static const char *s_sexName[4] = {"male", "female", "sex unspecified", "unknown sex specification"};

static TEXCOMPONENT_SECTIONS s_removeSections[8] = {TCS_UPPERARM,   TCS_LOWERARM, TCS_HAND,     TCS_UPPERTORSO,
                                                    TCS_LOWERTORSO, TCS_LEGUPPER, TCS_LEGLOWER, TCS_FEET};

static uint s_startingLayer[8] = {1, 1, 1, 1, 1, 1, 1, 1};
static const float s_cameraTargetZ = 0.97222221f;
static const float s_cameraOrbitRadius = 11.666667f;
static const float s_cameraOrbitHeight = 8.333333f;

extern const int *const g_ITEMTYPEARRAY;
void SetHandsState(HMODEL model, int itemSlot, int itemInventoryType);

CSimpleModel         *CCharCreateInfo::m_charCustomizeFrame;
TSFixedArray<uint>    CCharCreateInfo::m_factionIndex;
TSFixedArray<uint>    CCharCreateInfo::m_raceIndex;
int                   CCharCreateInfo::m_selectedRace = -1;
TSGrowableArray<uint> CCharCreateInfo::m_classIndex;
int                   CCharCreateInfo::m_selectedClass;
uint                  CCharCreateInfo::m_selectedSex;
float                 CCharCreateInfo::m_charFacing;
CHARCREATEINFO        CCharCreateInfo::m_charInfo;

static uint RandomSelection(uint numChoices) {
  return NTempest::CMath::mulhwu_(numChoices, NTempest::CRandom::uint32_(g_rndSeed));
}

static int Script_SetCharCustomizeFrame(lua_State *L);
static int Script_SetCharCustomizeBackground(lua_State *L);
static int Script_ResetCharCustomize(lua_State *__formal);
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
static int Script_UpdateCustomizationBackground(lua_State *__formal);
static int Script_HasCharCustomization(lua_State *L);
static int Script_CycleCharCustomization(lua_State *L);
static int Script_RandomizeCharCustomization(lua_State *__formal);
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

void CHARCREATEINFO::UpdateOutfit(int increment, uint race, uint sex) {
  FATALASSERT(increment <= 1 && increment >= -1);
  FATALASSERT(sex < 2);

  uint numOutfits = CCharCreateInfo::GetNumOutfits(race, selections[sex].classID, sex);
  if (numOutfits) {
    selections[sex].outfit = (increment + numOutfits + selections[sex].outfit) % numOutfits;
    ChangeFaceTexture(race, sex);
    ChangeFacialHairTexture(race, sex);
    ChangeScalpHairTexture(race, sex);
  }
}

void CHARCREATEINFO::ResetOutfitSelection(uint raceID, uint sex) {
  FATALASSERT(sex < 2);

  uint numOutfits = CCharCreateInfo::GetNumOutfits(raceID, selections[sex].classID, sex);
  if (numOutfits) {
    selections[sex].outfit %= numOutfits;
  } else {
    selections[sex].outfit = 0;
  }
}

void CHARCREATEINFO::CommitGeoset(uint sex) {
  FATALASSERT(sex < 2);
  CharCustomizationCommitGeosets(geosetHandle[sex]);
}

void ReportMissingComponentTextures(uint race, uint sex) {
  const ChrRacesRec *raceInfo;
  const char        *raceName;

  sex = min(max(static_cast<int>(sex), 0), 3);
  raceInfo = g_chrRacesDB.GetRecord(race);
  raceName = raceInfo ? raceInfo->m_name_lang[CURRENT_LANGUAGE] : "unknown race";

  SysMsgPrintf(SYSMSG_WARNING, 0x10, "MODELHASNOCOMPONENTABLETEXTURES|%s|%d|%s|%d", raceName, race, s_sexName[sex], sex);
}

void CHARCREATEINFO::FindRange(uint group, uint *start, uint *end) {
  FATALASSERT(start);
  FATALASSERT(end);
  *start = group * 100 + 1;
  *end = group * 100 + 99;
}

void CHARCREATEINFO::CommitTexture(int race, int sex) {
  if (sex < 2) {
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
  uint                   numFactions;
  uint                   i;
  uint                   count = 0;
  const FactionGroupRec *group;
  uint                   numRaces = 0;

  memset(m_charInfo.currentGeosets, 0, sizeof(m_charInfo.currentGeosets));

  for (i = 0; i < 2; ++i) {
    m_charInfo.characterModel[i] = 0;
    m_charInfo.geosetHandle[i] = 0;
    m_charInfo.characterComponent[i] = 0;
  }

  for (i = 0; i < static_cast<uint>(g_chrRacesDB.GetNumRecords()); ++i) {
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
      uint factionCount = m_factionIndex.Count();
      uint raceIndex;

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
  uint sex;

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

void CCharCreateInfo::SetCharCustomizeModel(const char *filename) {
  CModelCreate createData;

  if (!m_charCustomizeFrame || !filename || !*filename) {
    return;
  }

  createData.sequenceNames = 0;
  createData.numSequences = 0;
  createData.cameraNames = 0;
  createData.numCameras = 0;
  createData.flags = 4;
  createData.boneNames = g_glueBgObjNames;
  createData.numBones = 2;
  m_charCustomizeFrame->SetModel(filename, &createData, 0);
}

uint CCharCreateInfo::GetNumOutfits(uint raceID, uint classID, uint sexID) {
  uint count = 0;
  for (int i = 0; i < g_charStartOutfitDB.GetNumRecords(); ++i) {
    const CharStartOutfitRec *outfit = g_charStartOutfitDB.GetRecordByIndex(i);
    if (outfit->m_raceID == raceID && outfit->m_classID == classID && outfit->m_sexID == sexID) {
      ++count;
    }
  }
  return count;
}

const CharStartOutfitRec *CCharCreateInfo::GetOutfit(uint raceID, uint classID, uint sexID, uint outfitID) {
  for (int i = 0; i < g_charStartOutfitDB.GetNumRecords(); ++i) {
    const CharStartOutfitRec *outfit = g_charStartOutfitDB.GetRecordByIndex(i);
    if (outfit->m_raceID == raceID && outfit->m_classID == classID && outfit->m_sexID == sexID && outfit->m_outfitID == outfitID) {
      return outfit;
    }
  }
  return 0;
}

void CCharCreateInfo::ResetCharCustomizeInfo() {
  if (m_charCustomizeFrame && ModelIsLoaded(m_charCustomizeFrame->GetModel(), 0) && m_raceIndex.Count()) {
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
    NTempest::C3Vector facingVector(cos(facing), sin(facing), 0.0f);
    ModelApplyObjectFaceDir(model, 0, facingVector);
  }
}

const char *CCharCreateInfo::GetRaceNameByIndex(uint index) {
  if (index >= m_raceIndex.Count()) {
    return 0;
  }

  const ChrRacesRec *race = g_chrRacesDB.GetRecord(m_raceIndex[index]);
  return race ? race->m_name_lang[CURRENT_LANGUAGE] : 0;
}

void CCharCreateInfo::UpdateAvailableClasses() {
  if (static_cast<uint>(m_selectedRace) >= m_raceIndex.Count()) {
    return;
  }

  uint numRecords;
  numRecords = g_chrClassesDB.GetNumRecords();
  m_classIndex.SetCount(numRecords);

  uint count = 0;
  for (uint i = 0; i < static_cast<uint>(g_charBaseInfoDB.GetNumRecords()); ++i) {
    const CharBaseInfoRec *rec = g_charBaseInfoDB.GetRecordByIndex(i);
    if (rec->m_raceID == m_raceIndex[m_selectedRace]) {
      m_classIndex[count++] = rec->m_classID;
    }
  }

  m_classIndex.SetCount(count);
}

const char *CCharCreateInfo::GetClassNameByIndex(uint index) {
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

uint CCharCreateInfo::GetSelectedRaceID() {
  if (static_cast<uint>(m_selectedRace) < m_raceIndex.Count()) {
    return m_raceIndex[m_selectedRace];
  }

  return 0;
}

uint CCharCreateInfo::GetSelectedSexID() {
  return m_selectedSex;
}

uint CCharCreateInfo::GetSelectedClassID() {
  if (static_cast<uint>(m_selectedClass) < m_classIndex.Count()) {
    return m_classIndex[m_selectedClass];
  }

  return 0;
}

void CCharCreateInfo::UpdateAllCharacterInfo(int race, uint sex) {
  FATALASSERT(sex < 2);
  InitializeCharacterInfo(sex, 1);
  m_charInfo.CommitTexture(race, sex);
  CommitCurrentGeoset(sex);
}

void CCharCreateInfo::InitializeCharacterInfo(uint sex, int doNotCommitGeosets) {
  FATALASSERT(sex < UNITSEX_LAST);
  uint race = GetSelectedRaceID();
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

void CHARCREATEINFO::UpdateCharacterInfo(uint race, uint sex) {
  FATALASSERT(sex < 2);

  if (!characterModel[sex]) {
    const CreatureModelDataRec *modelInfo = Player_C_GetModelName(race, sex);
    FATALASSERT(modelInfo && modelInfo->m_ModelName && *modelInfo->m_ModelName);

    if (geosetHandle[sex]) {
      HandleClose(geosetHandle[sex]);
    }
    if (characterModel[sex]) {
      HandleClose(characterModel[sex]);
    }
    if (characterComponent[sex]) {
      HandleClose(characterComponent[sex]);
    }

    geosetHandle[sex] = 0;
    characterModel[sex] = ObjectModelCreate(modelInfo->m_ModelName, static_cast<OBJECT_TYPE>(25), 0x100800);
    characterComponent[sex] = 0;
    geosetHandle[sex] = CharCustomizationCreateGeosetHandle(characterModel[sex]);
    FATALASSERT(geosetHandle[sex]);
    ModelSetSequence(characterModel[sex], 0, 0);
  }
}

void CHARCREATEINFO::UpdateEquipment(int doNotCommitGeosets, uint race, uint sex) {
  uint                itemInventoryTypes[20];
  uint                itemDisplayIDs[20];
  CStatus             status;

  FATALASSERT(sex < 2);
  if (characterModel[sex]) {
    ModelClearAllLinks(characterModel[sex]);
  }
  CharCustomizationClearItemGeosets(geosetHandle[sex]);
  TexComponentRemoveSections(characterComponent[sex], s_removeSections, s_startingLayer, 8);
  TexComponentRemoveAllHolds(characterComponent[sex]);

  const CharStartOutfitRec *outfit = CCharCreateInfo::GetOutfit(race, selections[sex].classID, sex, selections[sex].outfit);
  if (outfit) {
    int itemCount = 0;
    for (int i = 0; i < 12; ++i) {
      if (outfit->m_ItemID[i] == -1 || outfit->m_DisplayItemID[i] == -1 || outfit->m_InventoryType[i] == -1 ||
          outfit->m_InventoryType[i] == INDEX_RANGED_TYPE || outfit->m_InventoryType[i] == INDEX_THROWN_TYPE ||
          outfit->m_InventoryType[i] == INDEX_RANGEDRIGHT_TYPE)
      {
        continue;
      }

      itemDisplayIDs[itemCount] = outfit->m_DisplayItemID[i];
      itemInventoryTypes[itemCount] = outfit->m_InventoryType[i];
      ++itemCount;

      const ItemDisplayInfoRec *displayInfoRec =
          g_itemDisplayInfoDB.GetRecord(outfit->m_DisplayItemID[i]);
      if (!displayInfoRec) {
        SysMsgPrintf(SYSMSG_WARNING, 0x10, "ITEMDISPLAYNOTFOUND|%d", outfit->m_DisplayItemID[i]);
        continue;
      }

      int itemSlotNum = -1;
      switch (outfit->m_InventoryType[i]) {
        case INDEX_NON_EQUIP_TYPE:
          continue;
        case INDEX_WEAPON_TYPE:
        case INDEX_2HWEAPON_TYPE:
        case INDEX_WEAPONMAINHAND_TYPE:
          itemSlotNum = 15;
          break;
        case INDEX_SHIELD_TYPE:
        case INDEX_WEAPONOFFHAND_TYPE:
        case INDEX_HOLDABLE_TYPE:
          itemSlotNum = 16;
          break;
      }

      ObjComponentAdd(sex, race, 1, characterModel[sex], displayInfoRec, outfit->m_InventoryType[i], 0, 0, 0, 0, 0);

      if (g_ITEMTYPEARRAY[outfit->m_InventoryType[i]] & 0x403F8) {
        if (characterComponent[sex]) {
          TexComponentAdd(&status, sex, characterComponent[sex], displayInfoRec, outfit->m_InventoryType[i], 1);
        } else {
          ReportMissingComponentTextures(race, sex);
        }
      }

      if (itemSlotNum != -1) {
        SetHandsState(characterModel[sex], itemSlotNum, outfit->m_InventoryType[i]);
      }
      CharCustomizationAddItemGeosets(geosetHandle[sex], displayInfoRec, outfit->m_InventoryType[i], characterComponent[sex], race, 1);
    }
    CharCustomizationCommitItemGeosets(geosetHandle[sex], 1);
  }

  if (!doNotCommitGeosets) {
    CommitTexture(race, sex);
    CharCustomizationCommitGeosets(geosetHandle[sex]);
  }
}

void CHARCREATEINFO::ChangeSkinTexture(int doNotCommitGeosets, uint race, uint sex) {
  FATALASSERT(sex < 2);

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

void CHARCREATEINFO::ChangeFaceTexture(uint race, uint sex) {
  FATALASSERT(sex < 2);
  if (characterModel[sex] && characterComponent[sex]) {
    CharCustomizationSetFaceTexture(characterModel[sex], characterComponent[sex], race, sex, selections[sex].face, selections[sex].skinColor, 0);
  }
}

void CHARCREATEINFO::ChangeFacialHairTexture(uint race, uint sex) {
  FATALASSERT(sex < 2);
  if (characterModel[sex] && characterComponent[sex]) {
    CharCustomizationSetFacialTexture(
        characterModel[sex], characterComponent[sex], race, sex, selections[sex].facialStyle, selections[sex].hairColor
    );
  }
}

void CHARCREATEINFO::ChangeFacialHairGeosets(uint sex, uint beardGeoset, uint sideburnGeoset, uint moustacheGeoset) {
  FATALASSERT(sex < 2);
  FATALASSERT(geosetHandle[sex]);
  CharCustomizationShowGeoset(geosetHandle[sex], CHARGEOSET_BEARD, beardGeoset);
  CharCustomizationShowGeoset(geosetHandle[sex], CHARGEOSET_SIDEBURN, sideburnGeoset);
  CharCustomizationShowGeoset(geosetHandle[sex], CHARGEOSET_MOUSTACHE, moustacheGeoset);
}

void CHARCREATEINFO::ChangeScalpHairTexture(uint race, uint sex) {
  if (characterModel[sex] && characterComponent[sex]) {
    CharCustomizationSetHairTexture(characterModel[sex], characterComponent[sex], race, sex, selections[sex].hairStyle, selections[sex].hairColor);
  }
}

void CHARCREATEINFO::ChangeHairGeosets(uint race, uint sex) {
  CharCustomizationResetHairGeoset(geosetHandle[sex], race, sex, selections[sex].hairStyle);
}

void CHARCREATEINFO::UpdateGeosets(uint beardGeoset, uint sideBurnGeoset, uint moustacheGeoset, uint sex) {
  FATALASSERT(sex < 2);
  CharCustomizationInitBaseCharacter(geosetHandle[sex], beardGeoset, sideBurnGeoset, moustacheGeoset, 2);
}

void CCharCreateInfo::CommitCurrentGeoset(uint sex) {
  FATALASSERT(sex < UNITSEX_LAST);
  m_charInfo.CommitGeoset(sex);
}

void CCharCreateInfo::UpdateCharacterInfo(uint sex) {
  FATALASSERT(sex < UNITSEX_LAST);
  uint race = GetSelectedRaceID();
  if (race) {
    m_charInfo.UpdateCharacterInfo(race, sex);
    ChangeSkinTexture(0, sex);
    ChangeFaceTexture(sex);
    ChangeFacialHairTexture(sex);
    ChangeScalpHairTexture(sex);
  }
}

void CCharCreateInfo::UpdateEquipment(int doNotUpdateGeosets, uint sex) {
  FATALASSERT(sex < UNITSEX_LAST);
  uint race = GetSelectedRaceID();
  if (race) {
    m_charInfo.UpdateEquipment(doNotUpdateGeosets, race, sex);
  }
}

void CCharCreateInfo::ChangeSkinTexture(int doNotCommitGeosets, uint sex) {
  FATALASSERT(sex < UNITSEX_LAST);
  uint race = GetSelectedRaceID();
  if (race) {
    m_charInfo.ChangeSkinTexture(doNotCommitGeosets, race, sex);
  }
}

void CCharCreateInfo::ChangeFaceTexture(uint sex) {
  FATALASSERT(sex < UNITSEX_LAST);
  uint race = GetSelectedRaceID();
  if (race) {
    m_charInfo.ChangeFaceTexture(race, sex);
  }
}

void CCharCreateInfo::ChangeFacialHairTexture(uint sex) {
  FATALASSERT(sex < UNITSEX_LAST);
  uint race = GetSelectedRaceID();
  if (race) {
    m_charInfo.ChangeFacialHairTexture(race, sex);
  }
}

void CCharCreateInfo::ChangeFacialHairGeosets(uint sex) {
  FATALASSERT(sex < UNITSEX_LAST);
  uint race = GetSelectedRaceID();
  if (race) {
    BEARDSTYLEDATA facialData;
    CharCustomizationGetBeardStyle(race, sex, m_charInfo.selections[sex].facialStyle, &facialData);
    m_charInfo.ChangeFacialHairGeosets(sex, facialData.beardGeoset, facialData.sideBurnGeoset, facialData.moustacheGeoset);
  }
}

void CCharCreateInfo::ChangeScalpHairTexture(uint sex) {
  FATALASSERT(sex < UNITSEX_LAST);
  uint race = GetSelectedRaceID();
  if (race) {
    m_charInfo.ChangeScalpHairTexture(race, sex);
  }
}

void CCharCreateInfo::ChangeHairGeosets(uint sex) {
  FATALASSERT(sex < UNITSEX_LAST);
  uint race = GetSelectedRaceID();
  if (race) {
    m_charInfo.ChangeHairGeosets(race, sex);
  }
}

void CCharCreateInfo::UpdateGeosets(uint sex) {
  FATALASSERT(sex < UNITSEX_LAST);
  uint race = GetSelectedRaceID();
  if (race) {
    BEARDSTYLEDATA facialData;
    CharCustomizationGetBeardStyle(race, sex, m_charInfo.selections[sex].facialStyle, &facialData);
    m_charInfo.UpdateGeosets(facialData.beardGeoset, facialData.sideBurnGeoset, facialData.moustacheGeoset, sex);
  }
}

void CCharCreateInfo::SetSelectedRace(uint index, int updateModel) {
  if (index >= m_raceIndex.Count()) {
    return;
  }

  m_selectedRace = index;
  UpdateAvailableClasses();
  if (!updateModel) {
    return;
  }

  m_charInfo.Shutdown();
  memset(m_charInfo.currentGeosets, 0, sizeof(m_charInfo.currentGeosets));
  {
    for (uint sex = 0; sex < 2; ++sex) {
      m_charInfo.characterModel[sex] = 0;
      m_charInfo.geosetHandle[sex] = 0;
      m_charInfo.characterComponent[sex] = 0;
    }
  }

  m_selectedClass = 0;
  uint classID = m_classIndex[0];
  index = m_raceIndex[index];
  memset(m_charInfo.selections, 0, sizeof(m_charInfo.selections));

  {
    for (uint sex = 0; sex < 2; ++sex) {
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

  for (int i = g_characterCreateCamerasDB.GetNumRecords(); i; --i) {
    const CharacterCreateCamerasRec *camera = g_characterCreateCamerasDB.GetRecordByIndex(i - 1);
    FATALASSERT(camera);
    if (camera->m_Race == static_cast<int>(index) && static_cast<uint>(camera->m_Sex) < UNITSEX_LAST && static_cast<uint>(camera->m_Camera) < 2) {
      m_charInfo.cameraHeight[camera->m_Sex][camera->m_Camera] = camera->m_Height * 0.027777778f;
      m_charInfo.cameraRadius[camera->m_Sex][camera->m_Camera] = camera->m_Radius * 0.027777778f * 0.5f;
      m_charInfo.targetHeight[camera->m_Sex][camera->m_Camera] = camera->m_Target * 0.027777778f;
    }
  }

  UpdateAllCharacterInfo(index, m_selectedSex);

  HMODEL model = m_charCustomizeFrame ? m_charCustomizeFrame->GetModel() : 0;
  if (model && ModelClearLink(model, 0)) {
    ModelAddLink(model, 0, m_charInfo.characterModel[m_selectedSex], 1.0f);
    NTempest::C3Vector facingVector(cos(m_charFacing), sin(m_charFacing), 0.0f);
    ModelApplyObjectFaceDir(model, 0, facingVector);
  }
}

void CCharCreateInfo::SetSelectedSex(uint sex) {
  if (sex < UNITSEX_LAST && sex != m_selectedSex) {
    m_selectedSex = sex;
    uint race = GetSelectedRaceID();
    m_charInfo.UpdateOutfit(0, race, sex);
    UpdateAllCharacterInfo(race, sex);

    HMODEL model = m_charCustomizeFrame ? m_charCustomizeFrame->GetModel() : 0;
    if (model && ModelClearLink(model, 0)) {
      ModelAddLink(model, 0, m_charInfo.characterModel[sex], 1.0f);
      NTempest::C3Vector facingVector(cos(m_charFacing), sin(m_charFacing), 0.0f);
      ModelApplyObjectFaceDir(model, 0, facingVector);
    }
  }
}

void CCharCreateInfo::SetSelectedClass(uint index) {
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

uint CCharCreateInfo::GetNumCharCustomizations(uint index) {
  uint race = GetSelectedRaceID();
  if (!race) {
    return 0;
  }

  int numVariations = 0;
  switch (index) {
    case 0:
      CharCustomizationGetNumSkinTextures(race, m_selectedSex, &numVariations, 0);
      return numVariations;
    case 1:
      CharCustomizationNumFaces(race, m_selectedSex, &numVariations, 0);
      return numVariations;
    case 2:
      return CharCustomizationNumHairStyles(race, m_selectedSex);
    case 3:
      return CharCustomizationNumHairColors(race, m_selectedSex);
    case 4:
      return CharCustomizationNumBeardStyles(race, m_selectedSex);
  }

  return 0;
}

void CCharCreateInfo::CycleCharCustomization(uint index, int delta) {
  if (!delta) {
    return;
  }

  uint race = GetSelectedRaceID();
  if (!race) {
    return;
  }

  uint                     sex = m_selectedSex;
  CustomizationSelections &selection = m_charInfo.selections[sex];
  uint                     seqTime = ModelGetSequenceTime(m_charInfo.characterModel[sex], 0);
  switch (index) {
    case 0: {
      int numSkinColors;
      CharCustomizationGetNumSkinTextures(race, sex, &numSkinColors, 0);
      if (numSkinColors >= 2) {
        selection.skinColor += delta < 0 ? numSkinColors - 1 : 1;
        selection.skinColor %= numSkinColors;
        ChangeSkinTexture(1, sex);
        ChangeFaceTexture(sex);
        ChangeFacialHairTexture(sex);
        ChangeScalpHairTexture(sex);
        m_charInfo.CommitTexture(race, sex);
        m_charInfo.RefreshVisibleGeosets(sex);
      }
      break;
    }

    case 1: {
      int pcVars;
      CharCustomizationNumFaces(race, sex, &pcVars, 0);
      if (pcVars >= 2) {
        selection.face += delta < 0 ? pcVars - 1 : 1;
        selection.face %= pcVars;
        ChangeFaceTexture(sex);
        m_charInfo.CommitTexture(race, sex);
      }
      break;
    }

    case 2: {
      uint numHairStyles = CharCustomizationNumHairStyles(race, sex);
      if (numHairStyles >= 2) {
        selection.hairStyle += delta < 0 ? numHairStyles - 1 : 1;
        selection.hairStyle %= numHairStyles;
        ChangeScalpHairTexture(sex);
        ChangeHairGeosets(sex);
        CommitCurrentGeoset(sex);
        m_charInfo.CommitTexture(race, sex);
      }
      break;
    }

    case 3: {
      uint numHairColors = CharCustomizationNumHairColors(race, sex);
      if (numHairColors >= 2) {
        selection.hairColor += delta < 0 ? numHairColors - 1 : 1;
        selection.hairColor %= numHairColors;
        ChangeFaceTexture(sex);
        ChangeFacialHairTexture(sex);
        ChangeScalpHairTexture(sex);
        m_charInfo.CommitTexture(race, sex);
      }
      break;
    }

    case 4: {
      uint numFacialStyles = CharCustomizationNumBeardStyles(race, sex);
      if (numFacialStyles >= 2) {
        selection.facialStyle += delta < 0 ? numFacialStyles - 1 : 1;
        selection.facialStyle %= numFacialStyles;
        ChangeFacialHairTexture(sex);
        ChangeFacialHairGeosets(sex);
        CommitCurrentGeoset(sex);
        m_charInfo.CommitTexture(race, sex);
      }
      break;
    }

    case 5:
      m_charInfo.UpdateOutfit(delta, race, sex);
      ChangeSkinTexture(1, sex);
      m_charInfo.CommitTexture(race, sex);
      m_charInfo.RefreshVisibleGeosets(sex);
      break;
  }

  ModelForceSequenceTime(m_charInfo.characterModel[sex], 0, seqTime, 0);
}

void CCharCreateInfo::RandomizeCharCustomization() {
  uint race = GetSelectedRaceID();
  if (!race) {
    return;
  }

  uint sex = m_selectedSex;
  uint seqTime = ModelGetSequenceTime(m_charInfo.characterModel[sex], 0);
  int  skinColors;
  int  PCFaceColors;
  CharCustomizationGetNumSkinTextures(race, sex, &skinColors, 0);
  CharCustomizationNumFaces(race, sex, &PCFaceColors, 0);

  CustomizationSelections &selection = m_charInfo.selections[sex];
  selection.outfit = 0;
  selection.skinColor = RandomSelection(skinColors);
  selection.hairColor = RandomSelection(CharCustomizationNumHairColors(race, sex));
  selection.hairStyle = RandomSelection(CharCustomizationNumHairStyles(race, sex));
  selection.facialStyle = RandomSelection(CharCustomizationNumBeardStyles(race, sex));
  selection.face = RandomSelection(PCFaceColors);

  UpdateAllCharacterInfo(race, sex);
  ModelForceSequenceTime(m_charInfo.characterModel[sex], 0, seqTime, 0);
}

void CCharCreateInfo::CreateCharacter(const char *name) {
  CHAR_NAME_RESULT result = CHAR_NAME_RESULT_START;

  if (!name || (result = ClientServices_CharacterValidateName(name)) != CHAR_NAME_SUCCESS) {
    const char *token = ClientServices_GetErrorToken(result);
    const char *text = FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE);
    FrameScript_SignalEvent(3, "%s%s", "OKAY", text);
    return;
  }

  CHARACTER_CREATE_INFO createInfo;
  SStrCopy(createInfo.name, name, sizeof(createInfo.name));
  createInfo.raceID = static_cast<unsigned char>(GetSelectedRaceID());
  createInfo.sexID = static_cast<unsigned char>(m_selectedSex);
  createInfo.classID = static_cast<unsigned char>(m_charInfo.selections[m_selectedSex].classID);
  createInfo.outfitID = static_cast<unsigned char>(m_charInfo.selections[m_selectedSex].outfit);
  createInfo.skinID = static_cast<unsigned char>(m_charInfo.selections[m_selectedSex].skinColor);
  createInfo.hairColorID = static_cast<unsigned char>(m_charInfo.selections[m_selectedSex].hairColor);
  createInfo.hairStyleID = static_cast<unsigned char>(m_charInfo.selections[m_selectedSex].hairStyle);
  createInfo.facialHairStyleID = static_cast<unsigned char>(m_charInfo.selections[m_selectedSex].facialStyle);
  createInfo.faceID = static_cast<unsigned char>(m_charInfo.selections[m_selectedSex].face);
  CGlueMgr::CreateCharacter(&createInfo);
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

static int Script_ResetCharCustomize(lua_State *__formal) {
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
    for (uint i = 0; i < static_cast<uint>(g_factionGroupDB.GetNumRecords()); ++i) {
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
  uint count = CCharCreateInfo::GetNumRaces();
  for (uint i = 0; i < count; ++i) {
    lua_pushstring(L, CCharCreateInfo::GetRaceNameByIndex(i));
  }
  return count;
}

static int Script_GetClassesForRace(lua_State *L) {
  uint count = CCharCreateInfo::GetNumClasses();
  for (uint i = 0; i < count; ++i) {
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
  CCharCreateInfo::SetSelectedRace(static_cast<uint>(lua_tonumber(L, 1)) - 1, 0);
  return 0;
}

static int Script_SetSelectedSex(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: SetSelectedSex(index)");
    return 0;
  }
  CCharCreateInfo::SetSelectedSex(static_cast<uint>(lua_tonumber(L, 1)) - 1);
  return 0;
}

static int Script_SetSelectedClass(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: SetSelectedClass(index)");
    return 0;
  }
  CCharCreateInfo::SetSelectedClass(static_cast<uint>(lua_tonumber(L, 1)) - 1);
  return 0;
}

static int Script_UpdateCustomizationBackground(lua_State *__formal) {
  CCharCreateInfo::SetSelectedRace(CCharCreateInfo::GetSelectedRaceIndex(), 1);
  return 0;
}

static int Script_HasCharCustomization(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: HasCharCustomization(index)");
    return 0;
  }

  if (CCharCreateInfo::GetNumCharCustomizations(static_cast<uint>(lua_tonumber(L, 1)) - 1) > 1) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_CycleCharCustomization(lua_State *L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
    luaL_error(L, "Usage: CycleCharCustomization(index, delta)");
    return 0;
  }
  CCharCreateInfo::CycleCharCustomization(static_cast<int>(lua_tonumber(L, 1)) - 1, static_cast<int>(lua_tonumber(L, 2)));
  return 0;
}

static int Script_RandomizeCharCustomization(lua_State *__formal) {
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
  CCharCreateInfo::CreateCharacter(lua_tostring(L, 1));
  return 0;
}

void CharCreateRegisterScriptFunctions() {
  for (uint i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void CharCreateUnregisterScriptFunctions() {
  for (uint i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
