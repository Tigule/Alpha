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

#include "DynamicObject_C.h"

#include "DB/DBClient/AutoCode/SpellRec.h"
#include "DB/DBClient/AutoCode/SpellVisualEffectNameRec.h"
#include "DB/DBClient/AutoCode/SpellVisualKitRec.h"
#include "DB/DBClient/AutoCode/SpellVisualRec.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Object/ObjectClient/Player_C.h"
#include "SoundInterface/SoundInterface.h"

#include <Model/IModel.h>
#include <Os/W32/OsSound.h>
#include <Services/SysMessage.h>
#include <Tempest/caasphere.h>

void            ClntObjMgrShowObject(DWORDLONG guid);
void            SpellVisualsBlizzardDestroy(BlizzardObject *&blizzard);
BlizzardObject *SpellVisualsBlizzardCreate(const NTempest::C3Vector &pos, float radius, int spellID, const SpellVisualKitRec *kitRec);
void            SpellVisualsPlayCameraShakeID(UINT shakeID, const NTempest::C3Vector &position);
void            SpellCameraShakeCallback(LPCSTR eventName, const NTempest::C3Vector &position);
void            SpellSoundEffectCallback(LPCSTR eventName, const NTempest::C3Vector &position);

static const char NONAME[7] = "NoName";

inline DYNAMIC_OBJECT_TYPE CGDynamicObject::GetDynamicType() {
  return (DYNAMIC_OBJECT_TYPE)m_dynamicObj->m_type;
}

inline int CGDynamicObject::GetSpellID() const {
  return m_dynamicObj->m_spellID;
}

inline float CGDynamicObject::GetRadius() const {
  return m_dynamicObj->m_radius;
}

inline DWORDLONG CGDynamicObject::GetCaster() const {
  return m_dynamicObj->m_caster;
}

void CGDynamicObject_C::SetStorage(DWORD *storage) {
  CGObject_C::SetStorage(storage);
  CGDynamicObject::SetStorage(storage + CGObject::TotalFields());
}

CGDynamicObject_C::CGDynamicObject_C(DWORD *storage, DWORD eventTime, CClientObjCreate *init)
    : CGObject_C(storage, eventTime, init), CGDynamicObject(storage + CGObject::TotalFields()), m_dynamicScale(1.0f), m_blizzardObject(0), m_sound(0) {
  m_dynamicObj->m_position = init->move.status.worldPosition;
  m_dynamicObj->m_facing = init->move.status.worldFacing;
  m_haveStandSequence = 0;
  m_haveHoldSequence = 0;
  m_dynamicScale = 1.0f;
}

CGDynamicObject_C::~CGDynamicObject_C() {
  if (!IsDisabled()) {
    RemoveWorldObject();
  }

  if (m_blizzardObject) {
    SpellVisualsBlizzardDestroy(m_blizzardObject);
  }

  ClearSound();
}

BOOL CGDynamicObject_C::UpdateModelLoadStatus() {
  if (!CGObject_C::UpdateModelLoadStatus()) {
    return 0;
  }

  m_haveStandSequence = ModelHasSequenceId(GetObjectModel(), 0) != 0;
  m_haveHoldSequence = ModelHasSequenceId(GetObjectModel(), 1) != 0;

  if (GetDynamicType() != DYNAMIC_OBJECT_FARSIGHT_FOCUS && GetDynamicType() != DYNAMIC_OBJECT_AREA_SPELL) {
    NTempest::CAaSphere bounds;
    bounds.r = 0.0f;
    ModelGetBounds(GetObjectModel(), &bounds);
    if (bounds.r > 0.001) {
      m_dynamicScale = m_dynamicObj->m_radius / bounds.r;
    } else {
      const SpellVisualEffectNameRec *effectRec = GetVisualEffectNameRec();
      if (effectRec && effectRec->m_areaEffectSize > 0.0f) {
        m_dynamicScale = m_dynamicObj->m_radius / effectRec->m_areaEffectSize;
      }
    }
  }

  if (m_haveHoldSequence && !m_haveStandSequence) {
    ObjectModelSetSequence(GetObjectModel(), 1, 0, 0);
  }
  return 1;
}

static void AnimEventCallback(LPCSTR eventName, const NTempest::C3Vector &position, LPVOID param) {
  ((CGDynamicObject_C *)param)->HandleAnimEvent(eventName, position);
}

static BOOL AnimFinishedCallback(LPVOID param) {
  if (param) {
    ((CGDynamicObject_C *)param)->AnimFinished();
  }
  return 1;
}

void CGDynamicObject_C::PostInit(const CClientObjCreate &init) {
  CGObject_C::PostInit(init);
  ClntObjMgrShowObject(GetGUID());
  if (GetObjectModel()) {
    AddWorldObject();
    ModelSetEventCallback(GetObjectModel(), AnimEventCallback, this, 0);
    ModelSetSeqFinishedHandler(GetObjectModel(), AnimFinishedCallback, this);
  }
  ObjectVisKitProc();

  if (GetDynamicType() == DYNAMIC_OBJECT_FARSIGHT_FOCUS && GetCaster() == ClntObjMgrGetActivePlayer()) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player && GetGUID() == player->GetFarsightFocus()) {
      player->SetFarSightFocus(this);
    }
  }
}

void CGDynamicObject_C::ObjectVisKitProc() {
  const SpellRec *spellRec = g_spellDB.GetRecord(GetSpellID());
  if (!spellRec) {
    return;
  }

  const SpellVisualRec *visualRec = g_spellVisualDB.GetRecord(spellRec->m_spellVisualID);
  if (!visualRec || !visualRec->m_hasAreaEffect) {
    return;
  }

  const SpellVisualKitRec *kitRec = g_spellVisualKitDB.GetRecord(visualRec->m_areaKit);
  if (!kitRec) {
    return;
  }

  if (kitRec->m_characterProcedure == 9) {
    m_blizzardObject = SpellVisualsBlizzardCreate(GetPosition(), GetRadius(), GetSpellID(), kitRec);
  }

  if (kitRec->m_soundID) {
    ClearSound();
    m_sound = SndInterfaceCreateSound(kitRec->m_soundID, 2.0f, -1, true);
    if (m_sound) {
      SndInterfaceAssociateSoundWithObject(m_sound, this);
    }
  }
}

void CGDynamicObject_C::ClearSound() {
  if (m_sound) {
    m_sound->Stop(3.0f);
    m_sound = 0;
  }
}

void CGDynamicObject_C::Disable(int shutdown) {
  CGObject_C::Disable(shutdown);
  RemoveWorldObject();
  ClearSound();
}

void CGDynamicObject_C::Reenable() {
  CGObject_C::Reenable();
  ObjectVisKitProc();
  AddWorldObject();
}

BOOL CGDynamicObject_C::SetBlock(UINT, DWORD) {
  FATALASSERT(0);
  return 1;
}

void CGDynamicObject_C::SetData(LPCVOID data, UINT bytes) {
  FATALASSERT(bytes <= sizeof(*m_dynamicObj));
  memcpy(m_dynamicObj, data, bytes);
}

UINT CGDynamicObject_C::OffsetOf(OBJECT_TYPE_ID type) {
  switch (type) {
    case ID_OBJECT:
      return 0;
    case ID_DYNAMICOBJECT:
      return CGObject::TotalFields() * sizeof(DWORD);
    default:
      FATALASSERT(0);
      return -1;
  }
}

const SpellVisualEffectNameRec *CGDynamicObject_C::GetVisualEffectNameRec() const {
  const SpellRec *spellRec = g_spellDB.GetRecord(GetSpellID());
  if (!spellRec) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "NOSPELLIDFOUND|%d", GetSpellID());
    return 0;
  }

  const SpellVisualRec *visualRec = g_spellVisualDB.GetRecord(spellRec->m_spellVisualID);
  if (!visualRec) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "SPELLVISUALIDNOTFOUND|%d|%d", spellRec->m_spellVisualID, GetSpellID());
    return 0;
  }

  if (!visualRec->m_hasAreaEffect) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "SPELLEFFECTNOAREAEFFECT|%d", spellRec->m_spellVisualID);
    return 0;
  }

  const SpellVisualEffectNameRec *effectRec = g_spellVisualEffectNameDB.GetRecord(visualRec->m_areaModel);
  if (!effectRec) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "SPELLEFFECTIDNOTFOUND|%d", visualRec->m_areaModel);
    return 0;
  }
  return effectRec;
}

LPCSTR CGDynamicObject_C::GetModelFileName() const {
  const SpellVisualEffectNameRec *effectRec = GetVisualEffectNameRec();
  if (effectRec) {
    return effectRec->m_fileName;
  }

  SysMsgPrintf(SYSMSG_FATAL, 2, "NOOBJECTFILENAME|%d|Dynamic", m_obj->m_entryID);
  return NONAME;
}

void CGDynamicObject_C::HandleAnimEvent(LPCSTR eventName, const NTempest::C3Vector &position) {
  switch (*(const UINT *)eventName) {
    case 'DNS$':
      SpellSoundEffectCallback(eventName + 4, position);
      break;
    case 'KHS$':
      SpellCameraShakeCallback(eventName + 4, position);
      break;
    default:
      SysMsgPrintf(SYSMSG_WARNING, 16, "UNKNOWNANIMEVENT|%s|CGDynamicObject_C|CGDynamicObject_C::HandleAnimEvent", eventName);
      break;
  }
}

void CGDynamicObject_C::AnimFinished() {
  if (!m_haveHoldSequence) {
    ObjectModelSetSequence(GetObjectModel(), 0, 0, 0);
  } else {
    ObjectModelSetSequence(GetObjectModel(), 1, 0, 0);
  }
}
