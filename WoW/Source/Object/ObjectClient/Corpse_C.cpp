#include <Base/Base.h>
#include <Gx/Gx.h>
#include <MapDefs.h>
#include <WorldClient/World.h>
#include "Net/NetClient/NetClient.h"
#include <Frame/CSimpleTop.h>
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "SoundInterface/SoundInterface.h"
#include "UIUtil/InputControl.h"
#include "Ui/WorldFrame.h"
#include "Ui/GameUI.h"

#include "Corpse_C.h"

#include "Component/CharacterCustomization.h"
#include "Component/Component.h"
#include "DB/DBClient/AutoCode/CreatureDisplayInfoRec.h"
#include "DB/DBClient/AutoCode/CreatureModelDataRec.h"
#include "DB/DBClient/AutoCode/ItemDisplayInfoRec.h"
#include "Game/GameClient/GuildClient.h"
#include "Net/NetClient/NetClient.h"
#include "Object/ObjectClient/AnimCompiles.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Ui/WorldFrame.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <Base/CDataAllocator.h>
#include <Base/CDataStore.h>
#include <Base/Handle.h>
#include <Base/Status.h>
#include <Model/IModel.h>
#include <Services/SysMessage.h>
#include <WorldClient/World.h>

struct CORPSEANIMDATA {
  DWORDLONG guid;
};

void ClntObjMgrShowObject(DWORDLONG guid);

inline BYTE CGCorpse::GetRaceID() const {
  return m_corpse->m_raceID;
}

inline BYTE CGCorpse::GetSex() const {
  return m_corpse->m_sex;
}

inline BYTE CGCorpse::GetSkinID() const {
  return m_corpse->m_skinID;
}

inline BYTE CGCorpse::GetFaceID() const {
  return m_corpse->m_faceID;
}

inline BYTE CGCorpse::GetHairStyleID() const {
  return m_corpse->m_hairStyleID;
}

inline BYTE CGCorpse::GetHairColorID() const {
  return m_corpse->m_hairColorID;
}

inline BYTE CGCorpse::GetFacialHairStyleID() const {
  return m_corpse->m_facialHairStyleID;
}

static TInstanceAllocator<CORPSEANIMDATA> s_freeAnimData(20);
static const char                         NONAME[7] = "NoName";

static BOOL DrownAnimCallback(LPVOID param) {
  CORPSEANIMDATA *animData = (CORPSEANIMDATA *)param;
  CGObject_C     *object = ClntObjMgrObjectPtr(animData->guid, __FILE__, __LINE__);
  if (object) {
    ((CGCorpse_C *)object)->OnDeathAnimEnd();
  }
  return 1;
}

void CGCorpse_C::SetStorage(DWORD *storage) {
  CGObject_C::SetStorage(storage);
  CGCorpse::SetStorage(storage + CGObject::TotalFields());
}

CGCorpse_C::CGCorpse_C(DWORD *storage, DWORD eventTime, CClientObjCreate *init)
    : CGObject_C(storage, eventTime, init), CGCorpse(storage + CGObject::TotalFields()), m_animData(0) {
  m_corpse->m_position = init->move.status.worldPosition;
  m_corpse->m_facing = init->move.status.worldFacing;
  InitComponents();
}

CGCorpse_C::~CGCorpse_C() {
  if (m_geosetHandle) {
    HandleClose(m_geosetHandle);
  }
  m_geosetHandle = 0;

  if (m_texComponent) {
    HandleClose(m_texComponent);
  }
  m_texComponent = 0;

  if (m_animData) {
    s_freeAnimData.Put(m_animData);
  }
}

void CGCorpse_C::PostInit(const CClientObjCreate &init) {
  CGObject_C::PostInit(init);
  ClntObjMgrShowObject(GetGUID());

  AddComponents();
  if (m_geosetHandle) {
    CharCustomizationCommitItemGeosets(m_geosetHandle, 0);
    Animate();
  }

  if (GetObjectModel()) {
    AddWorldObject();
    if (IsUnderWater()) {
      m_animData = s_freeAnimData.Get(0);
      m_animData->guid = GetGUID();
      ModelSetSeqFinishedHandler(GetObjectModel(), ANIM_DROWNED, DrownAnimCallback, m_animData);
      ObjectModelSetSequence(GetObjectModel(), ANIM_DROWNED, 0, 0);
    } else {
      ObjectModelSetSequence(GetObjectModel(), ANIM_DEATH, 0, 0);
      ModelForceCurrentSequenceTime(GetObjectModel(), INT_MAX, 0);
    }
  }
}

void CGCorpse_C::Disable(int shutdown) {
  CGWorldFrame::RegisterObjectFadeoutModel(this, m_texComponent, m_alpha);
  RemoveWorldObject();
  CGObject_C::Disable(shutdown);
}

void CGCorpse_C::Reenable() {
  CGObject_C::Reenable();
  AddWorldObject();
  DoFade(255, 0);
}

BOOL CGCorpse_C::SetBlock(UINT i, DWORD data) {
  if (i < CGObject::TotalFields()) {
    return CGObject_C::SetBlock(i, data);
  }

  i -= CGObject::TotalFields();
  FATALASSERT(i < (CGCorpse::GetDataSize()/sizeof(DWORD)));
  ((DWORD *)&m_corpse)[i] = data;
  return 1;
}

void CGCorpse_C::SetData(LPCVOID data, UINT bytes) {
  FATALASSERT(bytes < sizeof(*m_corpse));
  memcpy(m_corpse, data, bytes);
}

UINT CGCorpse_C::OffsetOf(OBJECT_TYPE_ID type) {
  switch (type) {
    case ID_OBJECT:
      return 0;
    case ID_CORPSE:
      return CGObject::TotalFields() * sizeof(DWORD);
    default:
      FATALASSERT(0);
      return -1;
  }
}

LPCSTR CGCorpse_C::GetModelFileName() const {
  const CreatureDisplayInfoRec *displayInfo = g_creatureDisplayInfoDB.GetRecord(m_corpse->m_displayID);
  if (!displayInfo) {
    SysMsgPrintf(SYSMSG_WARNING, 16, "INVALIDPLAYERDISPLAYID|%d|%d|%d", m_corpse->m_displayID, m_corpse->m_raceID, m_corpse->m_sex);
    return NONAME;
  }

  const CreatureModelDataRec *modelData = g_creatureModelDataDB.GetRecord(displayInfo->m_modelID);
  if (!modelData) {
    SysMsgPrintf(SYSMSG_WARNING, 16, "INVALIDPLAYERMODELRECORD|%d|%d|%d", displayInfo->m_modelID, m_corpse->m_raceID, m_corpse->m_sex);
    return NONAME;
  }

  return modelData->m_ModelName;
}

BOOL CGCorpse_C::ShouldRender(DWORD worldStatus) {
  if (m_texComponent && !TexComponentCheckSections(m_texComponent, 0)) {
    worldStatus &= ~1u;
  }

  BOOL shouldRender = CGObject_C::ShouldRender(worldStatus);
  if (m_texComponent && shouldRender) {
    CommitTexture(0);
  }
  return shouldRender;
}

void CGCorpse_C::CommitTexture(int force) {
  char    errorString[512];
  CStatus status;
  TexComponentCommitSections(&status, m_texComponent, force);
  if (!status.IsEmpty()) {
    status.GetErrorStr(errorString, sizeof(errorString), STATUS_INFO);
    FATALERROR(("corpse GUID 0x%I64X: %s", GetGUID(), errorString));
  }
}

void CGCorpse_C::InitPreferredGeosets() {
  memset(m_preferredGeosets, 0, sizeof(m_preferredGeosets));
  m_preferredGeosets[CHARGEOSET_HAIR] = CharCustomizationGetHairGeoset(GetRaceID(), GetSex(), GetHairStyleID());

  BEARDSTYLEDATA beardStyleData;
  BOOL           hasFacialInfo = CharCustomizationGetBeardStyle(GetRaceID(), GetSex(), GetFacialHairStyleID(), &beardStyleData);
  m_preferredGeosets[CHARGEOSET_EAR] = 2;
  if (hasFacialInfo) {
    m_preferredGeosets[CHARGEOSET_BEARD] = beardStyleData.beardGeoset;
    m_preferredGeosets[CHARGEOSET_SIDEBURN] = beardStyleData.sideBurnGeoset;
    m_preferredGeosets[CHARGEOSET_MOUSTACHE] = beardStyleData.moustacheGeoset;
  }
}

void CGCorpse_C::InitComponents() {
  const CreatureDisplayInfoRec *displayInfo = g_creatureDisplayInfoDB.GetRecord(m_corpse->m_displayID);
  if (!displayInfo || !g_creatureModelDataDB.GetRecord(displayInfo->m_modelID)) {
    return;
  }

  HMODEL model = GetObjectModel();
  FATALASSERT(model);

  HTEXTURE skinTexture = CharCustomizationSetSkin(model, GetRaceID(), GetSex(), GetSkinID(), 0);
  if (!skinTexture) {
    FATALERROR(
        ("Error, skinID %d on corpse (race/sex is %d/%d) cannot be loaded, is it a missing file?", GetSkinID(), GetRaceID(),
         GetSex())
    );
  }

  UINT textureLayerHolds[NUM_TEXLAYERS];
  CharCustomizationGetTextureLayerHolds(GetRaceID(), GetSex(), textureLayerHolds, NUM_TEXLAYERS);

  m_texComponent = TexComponentCreate(skinTexture, GetRaceID(), GetSex(), GetSkinID(), 0, 0);
  FATALASSERT(m_texComponent);
  HandleClose(skinTexture);

  CharCustomizationSetFaceTexture(model, m_texComponent, GetRaceID(), GetSex(), GetFaceID(), GetSkinID(), 0);
  CharCustomizationSetHairTexture(model, m_texComponent, GetRaceID(), GetSex(), GetHairStyleID(), GetHairColorID());
  CharCustomizationSetFacialTexture(
      model, m_texComponent, GetRaceID(), GetSex(), GetFacialHairStyleID(), GetHairColorID()
  );

  BEARDSTYLEDATA facialData;
  BOOL           hasFacialData = CharCustomizationGetBeardStyle(GetRaceID(), GetSex(), GetFacialHairStyleID(), &facialData);

  m_geosetHandle = CharCustomizationCreateGeosetHandle(model);
  FATALASSERT(m_geosetHandle);
  InitPreferredGeosets();
  if (hasFacialData) {
    CharCustomizationInitBaseCharacter(m_geosetHandle, facialData.beardGeoset, facialData.sideBurnGeoset, facialData.moustacheGeoset, 2);
  } else {
    CharCustomizationInitBaseCharacter(
        m_geosetHandle, g_defaultGeosetIDOffsets[1], g_defaultGeosetIDOffsets[2], g_defaultGeosetIDOffsets[3], 2
    );
  }
  CharCustomizationResetHairGeoset(m_geosetHandle, GetRaceID(), GetSex(), GetHairStyleID());
}

void CGCorpse_C::AddComponents() {
  for (UINT slot = 0; slot < sizeof(m_corpse->m_items) / sizeof(m_corpse->m_items[0]); ++slot) {
    if (m_corpse->m_items[slot]) {
      AddComponent(m_corpse->m_items[slot] & 0xFFFFFF, m_corpse->m_items[slot] >> 24, slot, 0);
    }
  }
}

void CGCorpse_C::AddComponent(int displayID, UINT inventoryType, int slot, int commit) {
  FATALASSERT(inventoryType < INDEX_NUMSLOTS);
  if (slot == 17) {
    return;
  }

  if (m_texComponent) {
    if ((1 << slot) & 0x403F8) {
      CStatus status;
      TexComponentAdd(&status, GetSex(), m_texComponent, g_itemDisplayInfoDB.GetRecord(displayID), inventoryType, 1);
    }

    const ItemDisplayInfoRec *displayInfo = g_itemDisplayInfoDB.GetRecord(displayID);
    if (displayInfo && (displayInfo->m_flags & 1) && slot == 18 && inventoryType == 19 && m_corpse->m_guildID) {
      int eStyle;
      int eColor;
      int bStyle;
      int bColor;
      int background;
      if (GuildGetGuildTabard(m_corpse->m_guildID, 0, eStyle, eColor, bStyle, bColor, background)) {
        ComponentApplyTabardTexture(m_texComponent, eStyle, eColor, bStyle, bColor, background);
      }
    }

    CharCustomizationAddItemGeosets(
        m_geosetHandle, g_itemDisplayInfoDB.GetRecord(displayID), inventoryType, m_texComponent, GetRaceID(), commit == 0
    );
  }

  if (commit) {
    CharCustomizationCommitItemGeosets(m_geosetHandle, 0);
    Animate();
  }

  if (!slot) {
    HeadGeosetHideCharGeosets(m_geosetHandle, g_itemDisplayInfoDB.GetRecord(displayID), GetRaceID(), m_preferredGeosets, NUM_CHARGEOSETS);
  }

  ObjComponentAdd(
      GetSex(), GetRaceID(), 1, GetObjectModel(), g_itemDisplayInfoDB.GetRecord(displayID), inventoryType, 0, 0, 0, 0, slot
  );
}

void CGCorpse_C::OnLeftClick() {
}

void CGCorpse_C::OnRightClick() {
  if (m_corpse->m_owner == ClntObjMgrGetActivePlayer()) {
    CDataStore msg;
    msg.Put(CMSG_RECLAIM_CORPSE);
    msg.Put(GetGUID());
    msg.Finalize();
    ClientServices_Send(&msg);
  }
}

void CGCorpse_C::GetWorldMatrix(NTempest::C34Matrix *worldMatrix) const {
  ModelForceStandingMatrix(
      GetObjectModel(), GetPosition(), GetGroundNormal(), GetRenderFacing(), GetScale() * GetRenderScale(), 2, 1.0f, worldMatrix
  );
}

bool CGCorpse_C::IsUnderWater() const {
  UINT               liquidStatus = 15;
  float              surfaceColPt = 0.0f;
  NTempest::C3Vector waterDir;
  int                deep;
  if (CWorld::QueryObjectLiquid(GetWorldObject(), liquidStatus, surfaceColPt, waterDir, deep)) {
    if (surfaceColPt - GetPosition().z > 0.66666669f) {
      return true;
    }
  }

  return false;
}

void CGCorpse_C::OnDeathAnimEnd() {
  HMODEL model = GetObjectModel();
  if (model) {
    ObjectModelSetSequence(model, ANIM_DROWNED, 0, 0);
  }
}
