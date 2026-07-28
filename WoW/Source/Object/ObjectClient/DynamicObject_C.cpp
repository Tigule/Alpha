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

void SpellVisualsBlizzardDestroy(BlizzardObject *&blizzard);
BlizzardObject *SpellVisualsBlizzardCreate(const NTempest::C3Vector &pos, float radius, int spellID, const SpellVisualKitRec *kitRec);
void SpellVisualsPlayCameraShakeID(unsigned int shakeID, const NTempest::C3Vector &position);
void SpellCameraShakeCallback(const char *eventName, const NTempest::C3Vector &position);
void SpellSoundEffectCallback(const char *eventName, const NTempest::C3Vector &position);

void CGDynamicObject_C::SetStorage(unsigned long *storage) {
  CGObject_C::SetStorage(storage);
  CGDynamicObject::SetStorage(storage + CGObject::TotalFields());
}

CGDynamicObject_C::CGDynamicObject_C(unsigned long *storage, unsigned long eventTime, CClientObjCreate *init)
    : CGObject_C(storage, eventTime, init),
      CGDynamicObject(storage + CGObject::TotalFields()),
      m_blizzardObject(0),
      m_sound(0) {
  m_dynamicScale = 1.0f;
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

int CGDynamicObject_C::UpdateModelLoadStatus() {
  if (!CGObject_C::UpdateModelLoadStatus()) {
    return 0;
  }

  HMODEL model = GetObjectModel();
  m_haveStandSequence = ModelHasSequenceId(model, 0) != 0;
  m_haveHoldSequence = ModelHasSequenceId(model, 1) != 0;

  if (m_dynamicObj->m_type != 2 && m_dynamicObj->m_type != 1) {
    NTempest::CAaSphere bounds;
    bounds.r = 0.0f;
    ModelGetBounds(model, &bounds);
    if (bounds.r > 0.001f) {
      m_dynamicScale = m_dynamicObj->m_radius / bounds.r;
    } else {
      const SpellVisualEffectNameRec *effectRec = GetVisualEffectNameRec();
      if (effectRec && effectRec->m_areaEffectSize > 0.0f) {
        m_dynamicScale = m_dynamicObj->m_radius / effectRec->m_areaEffectSize;
      }
    }
  }

  if (m_haveHoldSequence && !m_haveStandSequence) {
    ObjectModelSetSequence(model, 1, 0, 0);
  }
  return 1;
}

static void AnimEventCallback(const char *eventName, const NTempest::C3Vector &position, void *param) {
  static_cast<CGDynamicObject_C *>(param)->HandleAnimEvent(eventName, position);
}

static int AnimFinishedCallback(void *param) {
  if (param) {
    static_cast<CGDynamicObject_C *>(param)->AnimFinished();
  }
  return 1;
}

void CGDynamicObject_C::PostInit(const CClientObjCreate &init) {
  CGObject_C::PostInit(init);
  ClntObjMgrHideObject(GetGUID());
  HMODEL model = GetObjectModel();
  if (model) {
    AddWorldObject();
    ModelSetEventCallback(model, AnimEventCallback, this, 0);
    ModelSetSeqFinishedHandler(model, AnimFinishedCallback, this);
  }
  ObjectVisKitProc();

  unsigned __int64 activePlayer = ClntObjMgrGetActivePlayer();
  if (m_dynamicObj->m_type == 2 && m_dynamicObj->m_caster == activePlayer) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(activePlayer, __FILE__, __LINE__));
    if (player && player->GetFarsightFocus() == GetGUID()) {
      player->SetFarSightFocus(this);
    }
  }
}

void CGDynamicObject_C::ObjectVisKitProc() {
  const SpellRec *spellRec = g_spellDB.GetRecord(m_dynamicObj->m_spellID);
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
    NTempest::C3Vector position = GetPosition();
    m_blizzardObject = SpellVisualsBlizzardCreate(position, m_dynamicObj->m_radius, m_dynamicObj->m_spellID, kitRec);
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

int CGDynamicObject_C::SetBlock(unsigned int, unsigned long) {
  FATALASSERT(0);
  return 1;
}

void CGDynamicObject_C::SetData(const void *data, unsigned int bytes) {
  FATALASSERT(bytes <= sizeof(*m_dynamicObj));
  memcpy(m_dynamicObj, data, bytes);
}

unsigned int CGDynamicObject_C::OffsetOf(OBJECT_TYPE_ID type) {
  if (type == ID_OBJECT) {
    return 0;
  }
  FATALASSERT(type == ID_DYNAMICOBJECT);
  return CGObject::TotalFields() * sizeof(unsigned long);
}

const SpellVisualEffectNameRec *CGDynamicObject_C::GetVisualEffectNameRec() const {
  const SpellRec *spellRec = g_spellDB.GetRecord(m_dynamicObj->m_spellID);
  if (!spellRec) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "NOSPELLIDFOUND|%d", m_dynamicObj->m_spellID);
    return 0;
  }

  const SpellVisualRec *visualRec = g_spellVisualDB.GetRecord(spellRec->m_spellVisualID);
  if (!visualRec) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "SPELLVISUALIDNOTFOUND|%d|%d", spellRec->m_spellVisualID, m_dynamicObj->m_spellID);
    return 0;
  }

  if (!visualRec->m_hasAreaEffect) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "SPELLEFFECTNOAREAEFFECT|%d", spellRec->m_spellVisualID);
    return 0;
  }

  const SpellVisualEffectNameRec *effectRec = g_spellVisualEffectNameDB.GetRecord(visualRec->m_areaModel);
  if (!effectRec) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "SPELLEFFECTIDNOTFOUND|%d", visualRec->m_areaModel);
  }
  return effectRec;
}

const char *CGDynamicObject_C::GetModelFileName() const {
  const SpellVisualEffectNameRec *effectRec = GetVisualEffectNameRec();
  if (effectRec) {
    return effectRec->m_fileName;
  }

  SysMsgPrintf(SYSMSG_FATAL, 2, "NOOBJECTFILENAME|%d|Dynamic", m_dynamicObj->m_spellID);
  return "NoName";
}

void CGDynamicObject_C::HandleAnimEvent(const char *eventName, const NTempest::C3Vector &position) {
  unsigned int event = *reinterpret_cast<const unsigned int *>(eventName);
  if (event == 0x444E5324) {  // $SND
    SpellSoundEffectCallback(eventName + 4, position);
  } else if (event == 0x4B485324) {  // $SHK
    SpellCameraShakeCallback(eventName + 4, position);
  } else {
    SysMsgPrintf(SYSMSG_WARNING, 16, "UNKNOWNANIMEVENT|%s|CGDynamicObject_C|CGDynamicObject_C::HandleAnimEvent", eventName);
  }
}

void CGDynamicObject_C::AnimFinished() {
  if (m_haveHoldSequence) {
    ObjectModelSetSequence(GetObjectModel(), 1, 0, 0);
  } else {
    ObjectModelSetSequence(GetObjectModel(), 0, 0, 0);
  }
}
