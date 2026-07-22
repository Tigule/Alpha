#include "Glue/CharSelectInfo.h"

#include <Base/Status.h>
#include <Frame/CSimpleModel.h>
#include <Frame/SimpleFrameRegistry.h>
#include <FrameScript/FrameScript.h>
#include <Services/SysMessage.h>

#include "DB/DBClient/AutoCode/AreaTableRec.h"
#include "DB/DBClient/AutoCode/ChrClassesRec.h"
#include "DB/DBClient/AutoCode/ChrRacesRec.h"
#include "DB/DBClient/AutoCode/CreatureDisplayInfoRec.h"
#include "DB/DBClient/AutoCode/CreatureModelDataRec.h"
#include "DB/DBClient/AutoCode/ItemDisplayInfoRec.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "Component/CharacterCustomization.h"
#include "Game/GameClient/GuildClient.h"
#include "Glue/CharCreateInfo.h"
#include "Glue/CGlueMgr.h"
#include "Object/ObjectClient/AnimCompiles.h"
#include "Object/ObjectClient/Player_C.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <lauxlib.h>
#include <lua.h>

#define MAX_CHARACTERS_PER_REALM 10

static TSGrowableArray<CHARINFO> s_charList;
static const char                REGKEY[11] = "WoW\\Client";
static const char                REGVAL_LASTCHARACTER[14] = "LastCharacter";
static const char                REGVAL_LASTACCOUNT[12] = "LastAccount";
static const char                REGVAL_LASTREALM[10] = "LastRealm";

int           CCharSelectInfo::m_selectionIndex;
CSimpleModel *CCharSelectInfo::m_modelFrame;

static int __fastcall Script_SetCharSelectModelFrame(lua_State *L);
static int __fastcall Script_SetCharSelectBackground(lua_State *L);
static int __fastcall Script_GetCharacterListUpdate(lua_State *__formal);
static int __fastcall Script_GetNumCharacters(lua_State *L);
static int __fastcall Script_GetCharacterInfo(lua_State *L);
static int __fastcall Script_SelectCharacter(lua_State *L);
static int __fastcall Script_DeleteCharacter(lua_State *L);

static const FrameScript_Method s_ScriptFunctions[7] = {
    {"SetCharSelectModelFrame", Script_SetCharSelectModelFrame},
    {"SetCharSelectBackground", Script_SetCharSelectBackground},
    { "GetCharacterListUpdate",  Script_GetCharacterListUpdate},
    {       "GetNumCharacters",        Script_GetNumCharacters},
    {       "GetCharacterInfo",        Script_GetCharacterInfo},
    {        "SelectCharacter",         Script_SelectCharacter},
    {        "DeleteCharacter",         Script_DeleteCharacter}
};

CHARINFO::~CHARINFO() {
  if (m_characterModel) {
    HandleClose(m_characterModel);
  }
  if (m_characterComponent) {
    HandleClose(m_characterComponent);
  }
  if (m_petModel) {
    HandleClose(m_petModel);
  }
}

static void __fastcall SetFingersSeq(HMODEL model, unsigned int sequence, unsigned int startFinger, unsigned int lastFinger) {
  unsigned int finger;
  for (finger = startFinger; finger <= lastFinger; ++finger) {
    if (ModelLockObjectSequence(model, finger, 0)) {
      if (ModelSetSequence(model, sequence, finger, 4)) {
        ModelLockObjectSequence(model, finger, 1);
      }
    }
  }
}

static void __fastcall ResetFingersSeq(HMODEL model, unsigned int startFinger, unsigned int lastFinger) {
  unsigned int finger;
  unsigned int sequence = ModelGetPrimarySequence(model);
  for (finger = startFinger; finger <= lastFinger; ++finger) {
    if (ModelLockObjectSequence(model, finger, 0)) {
      ModelSetSequence(model, sequence, finger, 4);
    }
  }
}

static void __fastcall SetHandState(HMODEL model, int invType, unsigned int startFinger, unsigned int lastFinger) {
  if (invType == INDEX_SHIELD_TYPE) {
    ResetFingersSeq(model, startFinger, lastFinger);
  } else {
    SetFingersSeq(model, 15, startFinger, lastFinger);
  }
}

void __fastcall SetHandsState(HMODEL model, int itemSlot, int itemInventoryType) {
  if (!itemSlot) {
    return;
  }
  if (itemInventoryType == INDEX_RANGED_TYPE) {
    SetHandState(model, itemInventoryType, 8, 12);
  } else if (itemInventoryType > INDEX_RANGED_TYPE && itemInventoryType <= INDEX_2HWEAPON_TYPE) {
    SetHandState(model, itemInventoryType, 13, 17);
  }
}

void CHARINFO::ChangeSkinTexture() {
  unsigned int              preferredGeosets[NUM_CHARGEOSETS];
  CStatus                   status;
  BEARDSTYLEDATA            facialData = {1, 1, 1};
  int                       hasFacialInfo;
  HCHARGEOSET               geosetHandle;
  const ItemDisplayInfoRec *displayInfoRec;
  int                       i;

  if (m_characterComponent) {
    HandleClose(m_characterComponent);
    m_characterComponent = 0;
  }

  HTEXTURE skinTexture = CharCustomizationSetSkin(m_characterModel, m_characterInfo.raceID, m_characterInfo.sexID, m_characterInfo.skinID, 0);
  if (skinTexture) {
    m_characterComponent = TexComponentCreate(skinTexture, m_characterInfo.raceID, m_characterInfo.sexID, m_characterInfo.skinID, 0, 1);
    HandleClose(skinTexture);
  }

  if (m_characterComponent) {
    CharCustomizationSetFaceTexture(
        m_characterModel, m_characterComponent, m_characterInfo.raceID, m_characterInfo.sexID, m_characterInfo.faceID, m_characterInfo.skinID, 0
    );
    CharCustomizationSetFacialTexture(
        m_characterModel, m_characterComponent, m_characterInfo.raceID, m_characterInfo.sexID, m_characterInfo.facialHairStyleID,
        m_characterInfo.hairColorID
    );
  }

  hasFacialInfo = CharCustomizationGetBeardStyle(m_characterInfo.raceID, m_characterInfo.sexID, m_characterInfo.facialHairStyleID, &facialData);
  geosetHandle = CharCustomizationCreateGeosetHandle(m_characterModel);
  if (!geosetHandle) {
    return;
  }

  CharCustomizationInitBaseCharacter(
      geosetHandle, hasFacialInfo ? facialData.beardGeoset : 1, hasFacialInfo ? facialData.sideBurnGeoset : 1,
      hasFacialInfo ? facialData.moustacheGeoset : 1, 2
  );
  CharCustomizationResetHairGeoset(geosetHandle, m_characterInfo.raceID, m_characterInfo.sexID, m_characterInfo.hairStyleID);
  CharCustomizationSetHairTexture(
      m_characterModel, m_characterComponent, m_characterInfo.raceID, m_characterInfo.sexID, m_characterInfo.hairStyleID, m_characterInfo.hairColorID
  );

  memset(preferredGeosets, 0, sizeof(preferredGeosets));
  if (hasFacialInfo) {
    preferredGeosets[CGS_HAIR] = CharCustomizationGetHairGeoset(m_characterInfo.raceID, m_characterInfo.sexID, m_characterInfo.hairStyleID);
    preferredGeosets[CGS_FACIAL_BEARD] = facialData.beardGeoset;
    preferredGeosets[CGS_FACIAL_SIDEBURN] = facialData.sideBurnGeoset;
    preferredGeosets[CGS_FACIAL_MOUSTACHE] = facialData.moustacheGeoset;
    preferredGeosets[CGS_EARS] = 2;
  }

  for (i = 0; i < 20; ++i) {
    if (i == 17 || !m_characterInfo.inventoryItemDisplayID[i]) {
      continue;
    }

    displayInfoRec = g_itemDisplayInfoDB.GetRecord(m_characterInfo.inventoryItemDisplayID[i]);
    if (!displayInfoRec) {
      SysMsgPrintf(SYSMSG_WARNING, 0x10, "ITEMDISPLAYNOTFOUND|%d", m_characterInfo.inventoryItemDisplayID[i]);
      continue;
    }

    int inventoryType = m_characterInfo.inventoryItemType[i];
    ObjComponentAdd(
        m_characterInfo.sexID, m_characterInfo.raceID, 1, m_characterModel, displayInfoRec, inventoryType, (1 << i) & 0x10000, 0, 0, 0, 0
    );
    SetHandsState(m_characterModel, i, inventoryType);

    if ((1 << i) & 0x403F8) {
      if (m_characterComponent) {
        TexComponentAdd(&status, m_characterInfo.sexID, m_characterComponent, displayInfoRec, inventoryType, 1);
      } else {
        ReportMissingComponentTextures(m_characterInfo.raceID, m_characterInfo.sexID);
      }
    }

    if (!i) {
      HeadGeosetHideCharGeosets(geosetHandle, displayInfoRec, m_characterInfo.raceID, preferredGeosets, NUM_CHARGEOSETS);
    }
    CharCustomizationAddItemGeosets(geosetHandle, displayInfoRec, inventoryType, m_characterComponent, m_characterInfo.raceID, 1);
  }

  CharCustomizationCommitItemGeosets(geosetHandle, 1);
  CommitTexture(1);
  CharCustomizationCommitGeosets(geosetHandle);
  HandleClose(geosetHandle);
}

void CHARINFO::CommitTexture(int force) {
  char    errorString[512];
  CStatus status;

  TexComponentCommitSections(&status, m_characterComponent, force);

  if (!status.IsEmpty()) {
    status.GetErrorStr(errorString, sizeof(errorString), STATUS_INFO);
    FATALERROR(("Race %d Sex %d: %s", m_characterInfo.raceID, m_characterInfo.sexID, errorString));
  }
}

void CHARINFO::UpdateTabardTexture() {
  if (GuildGetGuildTabard(m_characterInfo.guildID, CCharSelectInfo::GuildCallback, m_eStyle, m_eColor, m_bStyle, m_bColor, m_background) &&
      m_characterComponent)
  {
    ComponentApplyTabardTexture(m_characterComponent, m_eStyle, m_eColor, m_bStyle, m_bColor, m_background);
    CommitTexture(0);
  }
}

void CHARINFO::UpdateCharacterInfo(const char *modelName, HMODEL backgroundModel) {
  const CreatureDisplayInfoRec *displayInfo = 0;
  const CreatureModelDataRec   *modelData = 0;

  if (m_characterModel) {
    HandleClose(m_characterModel);
  }
  if (m_petModel) {
    HandleClose(m_petModel);
  }
  if (m_characterComponent) {
    HandleClose(m_characterComponent);
  }

  m_characterModel = 0;
  m_petModel = 0;
  m_characterComponent = 0;

  CModelCreate createData;
  CStatus      status;

  createData.flags = 0x10286E;
  createData.sequenceNames = g_animationNames;
  createData.numSequences = NUM_OBJECTANIMATIONS;
  createData.boneNames = 0;
  createData.numBones = 0;
  createData.cameraNames = 0;
  createData.numCameras = 0;

  m_characterModel = ModelCreate(modelName, &createData, &status);
  SysMsgAdd(status, 4);

  if (!m_characterModel) {
    return;
  }

  ModelSetSequence(m_characterModel, ANIM_STAND, 4);

  if (m_characterInfo.petDisplayInfoID) {
    displayInfo = g_creatureDisplayInfoDB.GetRecord(m_characterInfo.petDisplayInfoID);
    if (displayInfo) {
      modelData = g_creatureModelDataDB.GetRecord(displayInfo->m_modelID);
    }
  }

  if (modelData) {
    CStatus petStatus;

    m_petModel = ModelCreate(modelData->m_ModelName, &createData, &petStatus);
    SysMsgAdd(petStatus, 4);
  }

  if (backgroundModel) {
    if (ModelAddLink(backgroundModel, 0, m_characterModel, 1.0f)) {
      ChangeSkinTexture();
      UpdateTabardTexture();
    }

    if (m_petModel) {
      ModelAddLink(backgroundModel, 1, m_petModel, 1.0f);
      ModelSetSequence(m_petModel, ANIM_STAND, 4);
      CGUnit_C::InitializeTextureVariations(displayInfo, m_petModel, modelData);
    }
  }
}

void __fastcall CCharSelectInfo::Initialize() {
}

void __fastcall CCharSelectInfo::Shutdown() {
  s_charList.Clear();
}

void __fastcall CCharSelectInfo::SetModelFrame(CSimpleModel *frame) {
  m_modelFrame = frame;
}

void __fastcall CCharSelectInfo::SetBackgroundModel(const char *filename) {
  CModelCreate createData;

  if (!m_modelFrame || !filename || !*filename) {
    return;
  }

  createData.sequenceNames = 0;
  createData.numSequences = 0;
  createData.cameraNames = 0;
  createData.numCameras = 0;
  createData.flags = 4;
  createData.boneNames = g_glueBgObjNames;
  createData.numBones = 2;
  m_modelFrame->SetModel(filename, &createData, 0);
}

void __fastcall CCharSelectInfo::ClearCharacterModel() {
  if (m_modelFrame) {
    HMODEL model = m_modelFrame->GetModel();

    if (model) {
      ModelClearLink(model, 0);
    }
  }
}

void __fastcall CCharSelectInfo::ClearPetModel() {
  if (m_modelFrame) {
    HMODEL model = m_modelFrame->GetModel();

    if (model) {
      ModelClearLink(model, 1);
    }
  }
}

CHARACTER_INFO *__fastcall CCharSelectInfo::GetSelectedCharacterInfo() {
  if (m_selectionIndex < 0 || m_selectionIndex >= static_cast<int>(s_charList.Count())) {
    return 0;
  }

  return &s_charList[m_selectionIndex].m_characterInfo;
}

void __fastcall CCharSelectInfo::EnumerateCharactersCallback(CHARACTER_INFO &info, void *__formal) {
  ASSERT(s_charList.Count() < MAX_CHARACTERS_PER_REALM);

  CHARINFO *charInfo = s_charList.New();

  if (!info.experienceLevel) {
    info.experienceLevel = 1;
  }

  charInfo->m_characterInfo = info;

  int displayID = charInfo->m_characterInfo.inventoryItemDisplayID[18];

  if (displayID && charInfo->m_characterInfo.inventoryItemType[18] == 19) {
    const ItemDisplayInfoRec *rec = g_itemDisplayInfoDB.GetRecord(displayID);

    if (rec && rec->m_flags & 1) {
      charInfo->UpdateTabardTexture();
      return;
    }
  }
}

void __fastcall CCharSelectInfo::GuildCallback(int guildID, const unsigned __int64 &guid, void *arg, bool granted) {
  if (guildID && granted) {
    unsigned __int64 guildGuid = 0;
    unsigned int     index;

    ASSERT(g_guildInfoCache.GetRecord(guildID, guildGuid, 0, 0));

    for (index = 0; index < s_charList.Count(); ++index) {
      if (s_charList[index].m_characterInfo.guildID == guildID) {
        s_charList[index].UpdateTabardTexture();
      }
    }
  }
}

void __fastcall CCharSelectInfo::UpdateCharacterList() {
  s_charList.SetCount(0);
  ClientServices_EnumerateCharacters(EnumerateCharactersCallback, 0);

  if (!s_charList.Count()) {
    SelectCharacter(-1);
    FrameScript_SignalEvent(6);
    return;
  }

  char          realm[64] = "";
  char          accountName[64] = "";
  unsigned long lastChar;

  SRegLoadString(REGKEY, REGVAL_LASTACCOUNT, 0, accountName, sizeof(accountName));
  SRegLoadString(REGKEY, REGVAL_LASTREALM, 0, realm, sizeof(realm));

  if (SStrCmpI(accountName, CGlueMgr::GetCurrentAccount(), 0x7FFFFFFF) || SStrCmpI(realm, ClientServices_GetSelectedRealmAddress(), 0x7FFFFFFF)) {
    SelectCharacter(0);
  } else {
    if (!SRegLoadValue(REGKEY, REGVAL_LASTCHARACTER, 0, &lastChar)) {
      lastChar = 0;
    }

    SelectCharacter(lastChar);
  }

  FrameScript_SignalEvent(6);
}

void __fastcall CCharSelectInfo::UpdateCharacterInfo() {
  if (m_selectionIndex < 0 || m_selectionIndex >= static_cast<int>(s_charList.Count())) {
    return;
  }

  CHARINFO                   *info = &s_charList[m_selectionIndex];
  const CreatureModelDataRec *rec = Player_C_GetModelName(info->m_characterInfo.raceID, info->m_characterInfo.sexID);

  ASSERT(rec);

  if (!rec->m_ModelName) {
    return;
  }

  ClearCharacterModel();
  ClearPetModel();

  HMODEL backgroundModel = m_modelFrame ? m_modelFrame->GetModel() : 0;

  info = &s_charList[m_selectionIndex];
  info->UpdateCharacterInfo(rec->m_ModelName, backgroundModel);
}

void __fastcall CCharSelectInfo::ChangeSkinTexture() {
  if (m_selectionIndex < 0 || m_selectionIndex >= static_cast<int>(s_charList.Count())) {
    return;
  }
  s_charList[m_selectionIndex].ChangeSkinTexture();
}

void __fastcall CCharSelectInfo::SelectCharacter(int index) {
  index = index < 0 ? 0 : index;

  if (index >= static_cast<int>(s_charList.Count())) {
    index = 0;
  }

  m_selectionIndex = index;

  UpdateCharacterInfo();
  FrameScript_SignalEvent(7, "%d", m_selectionIndex + 1);
}

int __fastcall CCharSelectInfo::GetNumCharacters() {
  return s_charList.Count();
}

static int __fastcall Script_SetCharSelectModelFrame(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: SetCharSelectModelFrame(\"frameName\")");
    return 0;
  }

  CSimpleFrame *frame = SimpleFrameRegistryGetEntry(lua_tostring(L, 1), 0);
  if (frame) {
    CCharSelectInfo::SetModelFrame(static_cast<CSimpleModel *>(frame));
  }

  return 0;
}

static int __fastcall Script_SetCharSelectBackground(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: SetCharSelectBackground(\"filename\")");
    return 0;
  }

  CCharSelectInfo::SetBackgroundModel(lua_tostring(L, 1));
  return 0;
}

static int __fastcall Script_GetCharacterListUpdate(lua_State *__formal) {
  CCharSelectInfo::ClearCharacterModel();
  CCharSelectInfo::ClearPetModel();
  CGlueMgr::GetCharacterList();
  return 0;
}

static int __fastcall Script_GetNumCharacters(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CCharSelectInfo::GetNumCharacters()));
  return 1;
}

static int __fastcall Script_GetCharacterInfo(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetCharacterInfo(index)");
    return 0;
  }

  int index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  if (index < 0 || index >= CCharSelectInfo::GetNumCharacters()) {
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnumber(L, 0.0);
    lua_pushnil(L);
    lua_pushnil(L);
    return 6;
  }

  CHARINFO &info = s_charList[index];
  lua_pushstring(L, info.m_characterInfo.name);

  const ChrRacesRec *race = g_chrRacesDB.GetRecord(info.m_characterInfo.raceID);
  lua_pushstring(L, race ? race->m_name_lang[CURRENT_LANGUAGE] : "");

  const ChrClassesRec *playerClass = g_chrClassesDB.GetRecord(info.m_characterInfo.classID);
  lua_pushstring(L, playerClass ? playerClass->m_name_lang[CURRENT_LANGUAGE] : "");
  lua_pushnumber(L, static_cast<double>(info.m_characterInfo.experienceLevel));

  const AreaTableRec *area = g_areaTableDB.GetRecord(info.m_characterInfo.zoneID);
  if (area) {
    lua_pushstring(L, area->m_AreaName_lang[CURRENT_LANGUAGE]);
  } else {
    lua_pushnil(L);
  }
  lua_pushstring(L, race ? race->m_clientFileString : "");
  return 6;
}

static int __fastcall Script_SelectCharacter(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: SelectCharacter(index)");
    return 0;
  }

  int index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  if (index < -1 || index >= CCharSelectInfo::GetNumCharacters()) {
    index = -1;
  }

  CCharSelectInfo::SelectCharacter(index);
  return 0;
}

static int __fastcall Script_DeleteCharacter(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: DeleteCharacter(index)");
    return 0;
  }

  int index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  if (index >= 0 && index < CCharSelectInfo::GetNumCharacters()) {
    CGlueMgr::DeleteCharacter(s_charList[index].m_characterInfo.guid);
  }

  return 0;
}

void __fastcall CharSelectRegisterScriptFunctions() {
  for (unsigned int i = 0; i < 7; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void __fastcall CharSelectUnregisterScriptFunctions() {
  for (unsigned int i = 0; i < 7; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
