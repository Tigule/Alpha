#include "Corpse_C.h"

#include "Component/CharacterCustomization.h"
#include "Component/Component.h"
#include "DB/DBClient/AutoCode/CreatureDisplayInfoRec.h"
#include "DB/DBClient/AutoCode/CreatureModelDataRec.h"
#include "DB/DBClient/AutoCode/ItemDisplayInfoRec.h"
#include "Game/GameClient/GuildClient.h"
#include "Net/NetClient/NetClient.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <Base/CDataAllocator.h>
#include <Base/CDataStore.h>
#include <Base/Handle.h>
#include <Base/Status.h>
#include <Model/IModel.h>
#include <Services/SysMessage.h>
#include <WorldClient/World.h>

struct CORPSEANIMDATA {
  unsigned __int64 guid;
};

void __fastcall ClntObjMgrShowObject(unsigned __int64 guid);

static TInstanceAllocator<CORPSEANIMDATA> s_freeAnimData(20);

static int __fastcall DrownAnimCallback(void *param) {
  CORPSEANIMDATA *animData = static_cast<CORPSEANIMDATA *>(param);
  CGObject_C     *object = ClntObjMgrObjectPtr(animData->guid, __FILE__, __LINE__);
  if (object) {
    static_cast<CGCorpse_C *>(object)->OnDeathAnimEnd();
  }
  return 1;
}

void CGCorpse_C::SetStorage(unsigned long *storage) {
  CGObject_C::SetStorage(storage);
  m_corpse = reinterpret_cast<CGCorpseData *>(storage + 6);
}

CGCorpse_C::CGCorpse_C(unsigned long *storage, unsigned long eventTime, CClientObjCreate *init)
    : CGObject_C(storage, eventTime, init), CGCorpse(storage), m_animData(0) {
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
    s_freeAnimData.PutData(m_animData, 0, 0);
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

  HMODEL model = GetObjectModel();
  if (!model) {
    return;
  }

  AddWorldObject();
  if (IsUnderWater()) {
    m_animData = static_cast<CORPSEANIMDATA *>(s_freeAnimData.GetData(0, typeid(CORPSEANIMDATA).raw_name(), -2));
    m_animData->guid = GetGUID();
    ModelSetSeqFinishedHandler(model, 132, DrownAnimCallback, m_animData);
    ObjectModelSetSequence(model, 132, 0, 0);
  } else {
    ObjectModelSetSequence(model, 1, 0, 0);
    ModelForceCurrentSequenceTime(model, INT_MAX, 0);
  }
}

void CGCorpse_C::Disable(int shutdown) {
  RemoveWorldObject();
  CGObject_C::Disable(shutdown);
}

void CGCorpse_C::Reenable() {
  CGObject_C::Reenable();
  AddWorldObject();
  DoFade(255, 0);
}

int CGCorpse_C::SetBlock(unsigned int i, unsigned long data) {
  if (i < OffsetOf(ID_CORPSE)) {
    return CGObject_C::SetBlock(i, data);
  }

  i -= OffsetOf(ID_CORPSE);
  FATALASSERT(i < sizeof(*m_corpse) / sizeof(unsigned long));
  reinterpret_cast<unsigned long *>(m_corpse)[i] = data;
  return 1;
}

void CGCorpse_C::SetData(const void *data, unsigned int bytes) {
  FATALASSERT(bytes < sizeof(*m_corpse));
  memcpy(m_corpse, data, bytes);
}

unsigned int __fastcall CGCorpse_C::OffsetOf(OBJECT_TYPE_ID type) {
  if (type == ID_OBJECT) {
    return 0;
  }
  FATALASSERT(type == ID_CORPSE);
  return 24;
}

const char *CGCorpse_C::GetModelFileName() const {
  CreatureDisplayInfoRec *displayInfo = g_creatureDisplayInfoDB.GetRecord(m_corpse->m_displayID);
  if (!displayInfo) {
    SysMsgPrintf(SYSMSG_WARNING, 16, "INVALIDPLAYERDISPLAYID|%d|%d|%d", m_corpse->m_displayID, m_corpse->m_raceID, m_corpse->m_sex);
    return "NoName";
  }

  CreatureModelDataRec *modelData = g_creatureModelDataDB.GetRecord(displayInfo->m_modelID);
  if (!modelData) {
    SysMsgPrintf(SYSMSG_WARNING, 16, "INVALIDPLAYERMODELRECORD|%d|%d|%d", displayInfo->m_modelID, m_corpse->m_raceID, m_corpse->m_sex);
    return "NoName";
  }

  return modelData->m_ModelName;
}

int CGCorpse_C::ShouldRender(unsigned long worldStatus) {
  if (m_texComponent && !reinterpret_cast<CTexComponent *>(m_texComponent)->CheckSections(0)) {
    worldStatus &= ~1u;
  }

  int shouldRender = CGObject_C::ShouldRender(worldStatus);
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
  m_preferredGeosets[CGS_HAIR] = CharCustomizationGetHairGeoset(m_corpse->m_raceID, m_corpse->m_sex, m_corpse->m_hairStyleID);

  BEARDSTYLEDATA beardStyleData = {1, 1, 1};
  int            hasFacialInfo = CharCustomizationGetBeardStyle(m_corpse->m_raceID, m_corpse->m_sex, m_corpse->m_facialHairStyleID, &beardStyleData);
  m_preferredGeosets[CGS_EARS] = 2;
  if (hasFacialInfo) {
    m_preferredGeosets[CGS_FACIAL_BEARD] = beardStyleData.beardGeoset;
    m_preferredGeosets[CGS_FACIAL_SIDEBURN] = beardStyleData.sideBurnGeoset;
    m_preferredGeosets[CGS_FACIAL_MOUSTACHE] = beardStyleData.moustacheGeoset;
  }
}

void CGCorpse_C::InitComponents() {
  CreatureDisplayInfoRec *displayInfo = g_creatureDisplayInfoDB.GetRecord(m_corpse->m_displayID);
  if (!displayInfo || !g_creatureModelDataDB.GetRecord(displayInfo->m_modelID)) {
    return;
  }

  HMODEL model = GetObjectModel();
  FATALASSERT(model);

  HTEXTURE skinTexture =
      CharCustomizationSetSkin(model, m_corpse->m_raceID, m_corpse->m_sex, m_corpse->m_skinID, 0);
  if (!skinTexture) {
    FATALERROR(
        ("Error, skinID %d on corpse (race/sex is %d/%d) cannot be loaded, is it a missing file?",
         m_corpse->m_skinID, m_corpse->m_raceID, m_corpse->m_sex)
    );
  }

  unsigned int textureLayerHolds[4];
  CharCustomizationGetTextureLayerHolds(
      m_corpse->m_raceID, m_corpse->m_sex, textureLayerHolds, 4
  );

  m_texComponent =
      TexComponentCreate(skinTexture, m_corpse->m_raceID, m_corpse->m_sex, m_corpse->m_skinID, 0, 0);
  FATALASSERT(m_texComponent);
  HandleClose(skinTexture);

  CharCustomizationSetFaceTexture(
      model, m_texComponent, m_corpse->m_raceID, m_corpse->m_sex, m_corpse->m_faceID,
      m_corpse->m_skinID, 0
  );
  CharCustomizationSetHairTexture(
      model, m_texComponent, m_corpse->m_raceID, m_corpse->m_sex, m_corpse->m_hairStyleID,
      m_corpse->m_hairColorID
  );
  CharCustomizationSetFacialTexture(
      model, m_texComponent, m_corpse->m_raceID, m_corpse->m_sex, m_corpse->m_facialHairStyleID,
      m_corpse->m_hairColorID
  );

  BEARDSTYLEDATA facialData = {1, 1, 1};
  int hasFacialData = CharCustomizationGetBeardStyle(
      m_corpse->m_raceID, m_corpse->m_sex, m_corpse->m_facialHairStyleID, &facialData
  );

  m_geosetHandle = CharCustomizationCreateGeosetHandle(model);
  FATALASSERT(m_geosetHandle);
  InitPreferredGeosets();
  CharCustomizationInitBaseCharacter(
      m_geosetHandle, hasFacialData ? facialData.beardGeoset : 1,
      hasFacialData ? facialData.sideBurnGeoset : 1,
      hasFacialData ? facialData.moustacheGeoset : 1, 2
  );
  CharCustomizationResetHairGeoset(
      m_geosetHandle, m_corpse->m_raceID, m_corpse->m_sex, m_corpse->m_hairStyleID
  );
}

void CGCorpse_C::AddComponents() {
  for (int slot = 0; slot < 19; ++slot) {
    unsigned int item = m_corpse->m_items[slot];
    if (item) {
      AddComponent(item & 0xFFFFFF, item >> 24, slot, 0);
    }
  }
}

void CGCorpse_C::AddComponent(int displayID, unsigned int inventoryType, int slot, int commit) {
  FATALASSERT(inventoryType < 27);
  if (slot == 17) {
    return;
  }

  const ItemDisplayInfoRec *displayInfo = g_itemDisplayInfoDB.GetRecord(displayID);
  if (m_texComponent) {
    if ((1 << slot) & 0x403F8) {
      CStatus status;
      TexComponentAdd(&status, m_corpse->m_sex, m_texComponent, displayInfo, inventoryType, 1);
    }

    if (displayInfo && (displayInfo->m_flags & 1) && slot == 18 && inventoryType == 19 &&
        m_corpse->m_guildID) {
      int eStyle;
      int eColor;
      int bStyle;
      int bColor;
      int background;
      if (GuildGetGuildTabard(
              m_corpse->m_guildID, 0, eStyle, eColor, bStyle, bColor, background
          )) {
        ComponentApplyTabardTexture(
            m_texComponent, eStyle, eColor, bStyle, bColor, background
        );
      }
    }

    CharCustomizationAddItemGeosets(
        m_geosetHandle, displayInfo, inventoryType, m_texComponent, m_corpse->m_raceID,
        commit == 0
    );
  }

  if (commit) {
    CharCustomizationCommitItemGeosets(m_geosetHandle, 0);
    Animate();
  }

  if (!slot) {
    HeadGeosetHideCharGeosets(
        m_geosetHandle, displayInfo, m_corpse->m_raceID, m_preferredGeosets, 15
    );
  }

  ObjComponentAdd(
      m_corpse->m_raceID, m_corpse->m_sex, 1, GetObjectModel(), displayInfo, inventoryType, 0, 0,
      0, 0, slot
  );
}

void CGCorpse_C::OnLeftClick() {
}

void CGCorpse_C::OnRightClick() {
  if (m_corpse->m_owner == ClntObjMgrGetActivePlayer()) {
    CDataStore msg;
    msg.Put(static_cast<unsigned int>(CMSG_RECLAIM_CORPSE));
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

unsigned int CGCorpse_C::IsUnderWater() {
  NTempest::C3Vector waterDir(0.0f);
  int                deep;
  unsigned int       liquidStatus = 15;
  float              surfaceColPt = 0.0f;
  if (!CWorld::QueryObjectLiquid(m_worldObject, liquidStatus, surfaceColPt, waterDir, deep)) {
    return 0;
  }

  return surfaceColPt - GetPosition().z > 0.66666669f;
}

void CGCorpse_C::OnDeathAnimEnd() {
  HMODEL model = GetObjectModel();
  if (model) {
    ObjectModelSetSequence(model, 132, 0, 0);
  }
}
