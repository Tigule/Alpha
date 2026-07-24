#include "GameObject_C.h"

#include "DB/DBClient/AutoCode/LockRec.h"
#include "DB/DBClient/AutoCode/SpellRec.h"
#include "DB/DBClient/AutoCode/GameObjectDisplayInfoRec.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Object/GameObjectStats.h"
#include "Services/SysMessage.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

class CGameObjectDef {
 public:
  static int __fastcall GetPropNum(int typeId, int propId);
};

void __fastcall ClntObjMgrHideObject(unsigned __int64 guid);

static int PageTextHandler(void* param, NETMESSAGE msgId, unsigned long eventTime, CDataStore* msg) {
    // TODO: implement
    return 0;
}

static int CustomAnimHandler(void* param, NETMESSAGE msgId, unsigned long eventTime, CDataStore* msg) {
    // TODO: implement
    return 0;
}

NTempest::C3Vector CGGameObject_C_TypeBase::GetPosition() const {
  return m_owner->m_gameObj->m_position;
}

float CGGameObject_C_TypeBase::GetFacing() const {
  return m_owner->m_gameObj->m_facing;
}

int CGGameObject_C::GetPageTextLanguage() const {
  FATALASSERT(m_stats);
  return m_stats->m_propValue[1];
}

int CGGameObject_C::GetPageTextMaterial() const {
  CGGameObject_C *object = const_cast<CGGameObject_C *>(this);
  int             prop = CGameObjectDef::GetPropNum(object->GetType(), 17);
  return object->GetPropertyValue(prop);
}

CGGameObject_C::~CGGameObject_C() {
  if (m_baseObj) {
    DEL(m_baseObj);
  }
}

static void GameObjectStatsCallback(int id, const unsigned __int64& guid, void* arg, unsigned char granted) {
    // TODO: implement
}

static void AnimEventCallback(const char* eventName, const NTempest::C3Vector& position, void* param) {
    // TODO: implement
}

static int AnimFinishedCallback(void* param) {
    // TODO: implement
    return 0;
}

static int OnUpdateState(unsigned __int64 guid, unsigned int offset, unsigned int bytes, const void* prevValue, void* param) {
    // TODO: implement
    return 0;
}

NTempest::C3Vector CGGameObject_C::GetPosition() const {
  FATALASSERT(m_baseObj);
  return m_baseObj->GetPosition();
}

void CGGameObject_C::GetPosition(NTempest::C3Vector &vec) const {
  FATALASSERT(m_baseObj);
  vec = m_baseObj->GetPosition();
}

float CGGameObject_C::GetFacing() const {
  FATALASSERT(m_baseObj);
  return m_baseObj->GetFacing();
}

int CGGameObject_C::IsPointInside(const NTempest::C3Vector &point) const {
  FATALASSERT(m_baseObj);
  return m_baseObj->IsPointInside(const_cast<NTempest::C3Vector &>(point));
}

void CGGameObject_C::SetStorage(unsigned long *storage) {
  CGObject_C::SetStorage(storage);
  m_gameObj = reinterpret_cast<CGGameObjectData *>(storage + 6);
}

CGGameObject_C::CGGameObject_C(unsigned long *storage, unsigned long eventTime, CClientObjCreate *init)
    : CGObject_C(storage, eventTime, init), m_baseObj(0), m_stats(0), m_serverTimeOffset(init->move.timeFallen - eventTime), m_isSolid(0) {
  m_gameObj = reinterpret_cast<CGGameObjectData *>(storage + 6);

  ClntObjMgrHideObject(GetGUID());
  m_gameObj->m_position = init->move.status.worldPosition;
  m_gameObj->m_facing = init->move.status.worldFacing;
}

UNIT_REACTION CGGameObject_C::ObjectReaction(const CGUnit_C *unit) const {
  if (m_gameObj->m_factionTemplate) {
    return CGUnit_C::UnitReaction(m_gameObj->m_factionTemplate, unit, -1);
  }
  return UNIT_REACTION_NEUTRAL;
}

const char *CGGameObject_C::GetModelFileNameInternal() const {
  int displayID = m_gameObj->m_data[0];
  if (!displayID) {
    return 0;
  }

  GameObjectDisplayInfoRec *displayInfo = g_gameObjectDisplayInfoDB.GetRecord(displayID);
  if (!displayInfo) {
    SysMsgPrintf(SYSMSG_FATAL, 2, "NOOBJECTFILENAME|%d|%d|Game", displayID, GetEntryID());
    return "NoName";
  }

  return displayInfo->m_modelName;
}

const char *CGGameObject_C::GetModelFileName() const {
  const char *modelName = GetModelFileNameInternal();
  if (modelName) {
    const char *extension = SStrChrR(modelName, '.');
    if (extension && !SStrCmpI(extension, ".wmo", 0x7FFFFFFF)) {
      return 0;
    }
  }
  return modelName;
}

int CGGameObject_C::GetType() {
  return m_stats ? m_stats->m_typeID : -1;
}

unsigned int CGGameObject_C::GetPropertyValue(unsigned int index) {
  return m_stats && index < 10 ? m_stats->m_propValue[index] : 0;
}

LockRec *CGGameObject_C::GetLockRec() {
  int lockID = GetPropertyValue(CGameObjectDef::GetPropNum(GetType(), 4));
  return g_lockDB.GetRecord(lockID);
}

unsigned int CGGameObject_C::IsValidOpenAction(int action) {
  int state = m_gameObj->m_data[6];
  if (action == 4) {
    return state == 2;
  }
  if (state == 2 || ((action < 2 || action == 3) && state != 1)) {
    return 0;
  }
  if (!action) {
    return !(m_gameObj->m_data[1] & 2);
  }
  if (action == 1) {
    return (m_gameObj->m_data[1] & 2) != 0;
  }
  return action != 2 || !state;
}

unsigned int CGGameObject_C::IsValidTargetForSpell(const unsigned __int64 &caster, int spellID) {
  LockRec  *lock = GetLockRec();
  SpellRec *spell = g_spellDB.GetRecord(spellID);
  if (!lock || !spell) {
    return 0;
  }

  const int *lockAction = lock->m_Action;
  const int *lockType = lock->m_Type;
  int        effectIndex;
  for (effectIndex = 0; effectIndex < 3; ++effectIndex) {
    if (spell->m_effect[effectIndex] == 33) {
      int i;
      for (i = 0; i < 4; ++i) {
        if (lockType[i] == 2 && spell->m_effectMiscValue[effectIndex] == lock->m_Index[i] && IsValidOpenAction(lockAction[i])) {
          return 1;
        }
      }
    } else if (spell->m_effect[effectIndex] == 59) {
      CGObject_C *item = ClntObjMgrObjectPtr(caster, __FILE__, __LINE__);
      if (item && (item->GetType() & TYPE_ITEM)) {
        int i;
        for (i = 0; i < 4; ++i) {
          if (lockType[i] == 1 && item->GetEntryID() == lock->m_Index[i] && IsValidOpenAction(lockAction[i])) {
            return 1;
          }
        }
      }
    }
  }
  return 0;
}

void __fastcall CGGameObject_C::Shutdown() {
  ClientServices_ClearMessageHandler(SMSG_GAMEOBJECT_PAGETEXT);
  ClientServices_ClearMessageHandler(SMSG_GAMEOBJECT_CUSTOM_ANIM);
}

void CGGameObject_C::StartInteraction() {
  FATALASSERT(m_baseObj);
  m_baseObj->StartInteraction();
}

void CGGameObject_C::CloseInteraction() {
  FATALASSERT(m_baseObj);
  m_baseObj->CloseInteraction();
}

int CGGameObject_C::IsTransport() const {
  FATALASSERT(m_stats);
  unsigned int type = *reinterpret_cast<const unsigned int *>(m_stats);
  return type == 11 || type == 15;
}

int CGGameObject_C::CanHighlight() const {
  FATALASSERT(m_baseObj);
  return m_baseObj->CanHighlight();
}
