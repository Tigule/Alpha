#include "GameObject_C.h"

#include "DB/DBClient/AutoCode/LockRec.h"
#include "DB/DBClient/AutoCode/LockTypeRec.h"
#include "DB/DBClient/AutoCode/SpellRec.h"
#include "DB/DBClient/AutoCode/GameObjectDisplayInfoRec.h"
#include "DB/DBClient/AutoCode/TaxiPathNodeRec.h"
#include "DB/DBClient/AutoCode/TransportAnimationRec.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "DB/WowLocale.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Object/GameObjectStats.h"
#include "Object/GameObject.h"
#include "Object/MovementData.h"
#include "Object/ObjectClient/Bag_C.h"
#include "Object/ObjectClient/Item_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "Services/SysMessage.h"
#include "SoundInterface/SoundInterface.h"
#include "Ui/ItemTextFrame.h"
#include "Ui/SpellBookFrame.h"
#include "Ui/WorldFrame.h"
#include "WorldClient/World.h"
#include "WorldCommon/WorldMath.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"
#include "Os/W32/OsSound.h"
#include "Os/OsTime.h"
#include "Model/CollisionData.h"
#include "Tempest/c2vector.h"
#include "Tempest/c33matrix.h"
#include "Tempest/c4quaternion.h"
#include "Tempest/cmath.h"

#include <math.h>
#include <malloc.h>

class CGameObjectDef {
 public:
  static int __fastcall GetPropNum(int typeId, int propId);
  static const char *__fastcall NameFromTypeId(int typeId);
};

struct StateAnimInfo {
  unsigned int  sequence;
  unsigned char reverse;
  unsigned char setAtEnd;
  unsigned char neverUseFallback;
  unsigned char padding;
};

static const StateAnimInfo s_stateAnimInfo[11] = {
    {1, 1, 0, 0, 0}, {2, 0, 0, 1, 0}, {3, 0, 1, 0, 0}, {4, 1, 1, 0, 0},
    {5, 0, 0, 1, 0}, {6, 0, 1, 0, 0}, {7, 1, 1, 0, 0}, {8, 0, 0, 1, 0},
    {9, 0, 0, 1, 0}, {10, 0, 0, 1, 0}, {11, 0, 0, 1, 0}
};

static const char *s_statusString[11] = {
    "Closed", "Opening", "Open", "Closing", "Custom0", "Custom1",
    "Custom2", "Custom3", 0, 0, 0
};

static const float MAX_SITCHAIRUSE_DISTANCE = 3.0f;
static const float MAX_SITCHAIRUSE_DISTANCE_SQUARED =
    MAX_SITCHAIRUSE_DISTANCE * MAX_SITCHAIRUSE_DISTANCE;
static const float MAX_LOOT_DISTANCE = 5.0f;
static const float MAX_BIND_DISTANCE = 10.0f;
static const float MAX_SHOP_DISTANCE = 5.5555553f;
static const float MAX_OBJ_INTEREST_RADIUS = 100.0f;

void __fastcall ClntObjMgrHideObject(unsigned __int64 guid);
void __fastcall ClntObjMgrShowObject(unsigned __int64 guid);
void __fastcall MovementAddTransport(CGGameObject_C *transport);
void __fastcall MovementRemoveTransport(CGGameObject_C *transport);
void __fastcall Spell_C_GetMinMaxPoints(
    const SpellRec *spell, int effectIndex, int *min, int *max, unsigned int level, int isPet);
void __fastcall Spell_C_GetMinMaxRange(int spellID, float *min, float *max);
bool __fastcall Spell_C_CastSpell(int spellID, const CGItem_C *item);
bool __fastcall Spell_C_HandleSpriteClick(CGObject_C *object);
void __fastcall SpellVisualsPlayCameraShakeID(
    unsigned int shakeID, const NTempest::C3Vector &position);

static int __fastcall PageTextHandler(void* param, NETMESSAGE msgId, unsigned long eventTime, CDataStore* msg) {
  FATALASSERT(msg);
  unsigned __int64 gameObject;
  msg->Get(gameObject);
  CGObject_C *object = ClntObjMgrObjectPtr(gameObject, __FILE__, __LINE__);
  if (object) {
    CGItemText::SetItem(object->GetGUID(), 1);
  }
  return 1;
}

static int __fastcall CustomAnimHandler(void* param, NETMESSAGE msgId, unsigned long eventTime, CDataStore* msg) {
  FATALASSERT(msg);
  unsigned __int64 gameObject;
  unsigned int anim;
  msg->Get(gameObject);
  msg->Get(anim);
  CGGameObject_C *object =
      static_cast<CGGameObject_C *>(ClntObjMgrObjectPtr(gameObject, __FILE__, __LINE__));
  if (object && anim < 4) {
    object->ActivateCustomAnim(anim);
  }
  return 1;
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

void CGGameObject_C::LoadBaseObject(const GameObjectStats *stats) {
  FATALASSERT(stats);
  SetMirrorHandlers();
  m_stats = const_cast<GameObjectStats *>(stats);

  switch (stats->m_typeID) {
    case 0:  m_baseObj = NEW(CGGameObject_C_Type_Door)(this); break;
    case 1:  m_baseObj = NEW(CGGameObject_C_Type_Button)(this); break;
    case 2:  m_baseObj = NEW(CGGameObject_C_Type_QuestGiver)(this); break;
    case 3:  m_baseObj = NEW(CGGameObject_C_Type_Chest)(this); break;
    case 4:  m_baseObj = NEW(CGGameObject_C_Type_Binder)(this); break;
    case 5:  m_baseObj = NEW(CGGameObject_C_Type_Generic)(this); break;
    case 6:  m_baseObj = NEW(CGGameObject_C_Type_Trap)(this); break;
    case 7:  m_baseObj = NEW(CGGameObject_C_Type_Chair)(this); break;
    case 8:  m_baseObj = NEW(CGGameObject_C_Type_SpellFocus)(this); break;
    case 9:  m_baseObj = NEW(CGGameObject_C_Type_Text)(this); break;
    case 10: m_baseObj = NEW(CGGameObject_C_Type_Goober)(this); break;
    case 11: m_baseObj = NEW(CGGameObject_C_Type_Transport)(this); break;
    case 12: m_baseObj = NEW(CGGameObject_C_Type_AreaDamage)(this); break;
    case 13: m_baseObj = NEW(CGGameObject_C_Type_Camera)(this); break;
    case 14: m_baseObj = NEW(CGGameObject_C_Type_MapObj)(this); break;
    case 15: m_baseObj = NEW(CGGameObject_C_Type_MapObjTransport)(this); break;
    case 16: m_baseObj = NEW(CGGameObject_C_Type_DuelArbiter)(this); break;
    case 17: m_baseObj = NEW(CGGameObject_C_Type_FishingNode)(this); break;
    case 18: m_baseObj = NEW(CGGameObject_C_Type_Ritual)(this); break;
    default:
      m_baseObj = &s_nullBaseObj;
      SysMsgPrintf(SYSMSG_WARNING, 2, "BADBASEGAMEOBJECT|%d", stats->m_typeID);
      break;
  }

  FATALASSERT(m_baseObj);
  m_baseObj->PostInit();
}

static void __fastcall GameObjectStatsCallback(int id, const unsigned __int64& guid, void* arg, bool granted) {
  CGGameObject_C *object =
      static_cast<CGGameObject_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (object) {
    unsigned __int64 cacheGuid = 0;
    const GameObjectStats_C *stats =
        g_gameObjectDBCache.GetRecord(id, cacheGuid, 0, 0);
    if (stats) {
      object->LoadBaseObject(stats);
    }
  }
}

static void __fastcall AnimEventCallback(const char* eventName, const NTempest::C3Vector& position, void* param) {
  FATALASSERT(param);
  CGGameObject_C *object = static_cast<CGGameObject_C *>(param);
  FATALASSERT(object->m_baseObj);
  object->m_baseObj->HandleAnimEvent(eventName, position);
}

static int __fastcall AnimFinishedCallback(void* param) {
  FATALASSERT(param);
  CGGameObject_C *object = static_cast<CGGameObject_C *>(param);
  FATALASSERT(object->m_baseObj);
  object->m_baseObj->HandleAnimFinished();
  return 1;
}

static int __fastcall OnUpdateState(unsigned __int64 guid, unsigned int offset, unsigned int bytes, const void* prevValue, void* param) {
  CGGameObject_C *object =
      static_cast<CGGameObject_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  FATALASSERT(object);
  FATALASSERT(object->m_baseObj);
  object->m_baseObj->UpdateState(
      *static_cast<const int *>(prevValue),
      object->GetState());
  return 1;
}

CGGameObject_C_TypeBase::CGGameObject_C_TypeBase()
    : m_owner(0), m_interactDistance(MAX_LOOT_DISTANCE) {
}

CGGameObject_C_TypeBase::CGGameObject_C_TypeBase(CGGameObject_C *owner)
    : m_owner(owner), m_interactDistance(MAX_LOOT_DISTANCE) {
  FATALASSERT(m_owner);
}

CGGameObject_C_TypeBase::~CGGameObject_C_TypeBase() {
}

unsigned int CGGameObject_C_TypeBase::CanHighlight() {
  return CanUse();
}

unsigned int CGGameObject_C_TypeBase::CanChangeCursor() {
  return CanUse();
}

unsigned int CGGameObject_C_TypeBase::CanUse() {
  unsigned __int64 activePlayer = ClntObjMgrGetActivePlayer();
  CGUnit_C *player = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(activePlayer, __FILE__, __LINE__));
  if (!player) {
    return 0;
  }

  const CGGameObjectData *data = m_owner->GetGameObjectData();
  if (m_owner->GetType() != 6 && m_owner->ObjectReaction(player) == UNIT_REACTION_HOSTILE) {
    return 0;
  }
  return !(data->m_data[1] & 1) && (!(data->m_data[1] & 4) || (data->m_dynamicFlags & 1));
}

unsigned int CGGameObject_C_TypeBase::CanUseNow(GAME_ERROR_TYPE *reason) {
  unsigned __int64 activePlayer = ClntObjMgrGetActivePlayer();
  CGPlayer_C *player = static_cast<CGPlayer_C *>(
      ClntObjMgrObjectPtr(activePlayer, __FILE__, __LINE__));
  if (!player || player->GetUnitData()->health <= 0) {
    if (reason) {
      *reason = GERR_PLAYER_DEAD;
    }
    return 0;
  }

  if (m_owner->GetGameObjectData()->m_data[1] & 2) {
    if (reason) {
      *reason = GERR_USE_LOCKED;
    }
    return 0;
  }

  float range = m_interactDistance;
  int spellID = 0;
  if (m_owner->IsLocked(&spellID, 0, 0, 0, 0) || !spellID) {
    if (m_owner->GetState() == 2) {
      if (reason) {
        *reason = GERR_USE_DESTROYED;
      }
      return 0;
    }
  } else {
    float minRange;
    Spell_C_GetMinMaxRange(spellID, &minRange, &range);
  }

  NTempest::C3Vector delta = player->GetPosition() - GetPosition();
  if (delta.SquaredMag() > range * range) {
    if (reason) {
      *reason = GERR_USE_TOO_FAR;
    }
    return 0;
  }
  return 1;
}

unsigned int CGGameObject_C_TypeBase::Use(const unsigned __int64 &) {
  FATALASSERT(CanUseNow(0));

  int       spellID = 0;
  CGItem_C *item = 0;
  int       openIndex = 0;
  if (m_owner->IsLocked(&spellID, 0, 0, &item, &openIndex)) {
    const LockRec *lock = m_owner->GetLockRec();
    FATALASSERT(lock);

    if (m_owner->IsValidOpenAction(lock->m_Action[0])) {
      if (lock->m_Type[0] == 1) {
        unsigned __int64 guid = m_owner->GetGUID();
        const ItemStats_C *stats =
            g_itemDBCache.GetRecord(lock->m_Index[0], guid, 0, 0);
        if (stats) {
          CGGameUI::DisplayError(GERR_USE_LOCKED_WITH_ITEM_S, stats->m_displayName[0]);
        }
      } else if (lock->m_Type[0] == 2) {
        LockTypeRec *lockType = g_lockTypeDB.GetRecord(lock->m_Index[0]);
        const char *name =
            lockType ? lockType->m_name_lang[CURRENT_LANGUAGE] : "UNKNOWN";
        if (spellID) {
          CGGameUI::DisplayError(
              GERR_USE_LOCKED_WITH_SPELL_KNOWN_SI,
              name,
              lock->m_Skill[0]);
        } else {
          CGGameUI::DisplayError(GERR_USE_LOCKED_WITH_SPELL_S, name);
        }
      } else {
        CGGameUI::DisplayError(GERR_USE_CANT_OPEN);
      }
    } else if (m_owner->GetType() == 3 && !m_owner->GetState()) {
      CGGameUI::DisplayError(GERR_CHEST_IN_USE);
    }
    return 0;
  }

  if (spellID) {
    if (!Spell_C_CastSpell(spellID, item)) {
      return 0;
    }
    Spell_C_HandleSpriteClick(m_owner);
    return 1;
  }

  CDataStore msg;
  msg.Put(CMSG_GAMEOBJ_USE);
  msg.Put(m_owner->GetGUID());
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

void CGGameObject_C_TypeBase::UpdateState(int, int) {
}

void CGGameObject_C_TypeBase::HandleAnimEvent(const char *, const NTempest::C3Vector &) {
}

void CGGameObject_C_TypeBase::HandleAnimFinished() {
}

const char *CGGameObject_C_TypeBase::DebugStatus() {
  return "";
}

void CGGameObject_C_TypeBase::ActivateCustomAnim(unsigned int) {
}

void CGGameObject_C_TypeBase::AddPassenger(CMovementData *) {
}

NTempest::C3Vector CGGameObject_C_TypeBase::GetCurrentMoveVector() const {
  return NTempest::C3Vector();
}

int CGGameObject_C_TypeBase::IsPointInside(const NTempest::C3Vector &) const {
  return 0;
}

void CGGameObject_C_TypeBase::PostInit() {
  FATALASSERT(m_owner);
  m_owner->PostPostInit();
}

void CGGameObject_C_TypeBase::Reenable() {
}

void CGGameObject_C_TypeBase::Disable(int) {
}

void CGGameObject_C_TypeBase::PostReenable() {
}

void CGGameObject_C_TypeBase::UpdateMovement(unsigned long, float) {
}

void CGGameObject_C_TypeBase::ModelJustLoaded() {
}

void CGGameObject_C_TypeBase::StartInteraction() {
}

void CGGameObject_C_TypeBase::CloseInteraction() {
}

unsigned int CGGameObject_C_Type_Null::CanUse() {
  return 0;
}

unsigned int CGGameObject_C_Type_Null::CanUseNow(GAME_ERROR_TYPE *) {
  return 0;
}

const char *CGGameObject_C_Type_Null::DebugStatus() {
  return "Unknown object type";
}

int CGGameObject::GetState() const {
  return m_gameObj->m_data[6];
}

void CGGameObject_C::ActivateCustomAnim(unsigned int anim) {
  FATALASSERT(m_baseObj);
  m_baseObj->ActivateCustomAnim(anim);
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
  return m_baseObj->IsPointInside(point);
}

void CGGameObject_C::SetStorage(unsigned long *storage) {
  CGObject_C::SetStorage(storage);
  m_gameObj = reinterpret_cast<CGGameObjectData *>(storage + 6);
}

void CGGameObject_C::PostInit(const CClientObjCreate &init) {
  CGObject_C::PostInit(init);
  unsigned __int64 guid = GetGUID();
  const GameObjectStats_C *stats =
      g_gameObjectDBCache.GetRecord(GetEntryID(), guid, GameObjectStatsCallback, 0);
  if (stats) {
    LoadBaseObject(stats);
  }
}

int CGGameObject_C::UpdateModelLoadStatus() {
  if (!CGObject_C::UpdateModelLoadStatus()) {
    return 0;
  }

  NTempest::CAaBox localExtents;
  memset(&localExtents, 0, sizeof(localExtents));
  ModelGetCollisionExtents(GetObjectModel(), &localExtents);
  CWorldMath::TransformAABox(m_matrix, localExtents, m_collideExtents);
  FATALASSERT(m_baseObj);
  m_baseObj->ModelJustLoaded();
  return 1;
}

void CGGameObject_C::Disable(int shutdown) {
  if (m_baseObj) {
    m_baseObj->Disable(shutdown);
    UnsetMirrorHandlers();
    CGWorldFrame::RegisterObjectFadeoutModel(this, 0, m_alpha);
    RemoveWorldObject();
  }
  CGObject_C::Disable(shutdown);
}

void CGGameObject_C::Reenable() {
  if (!m_baseObj) {
    ClntObjMgrHideObject(GetGUID());
  }
  CGObject_C::Reenable();
  if (m_baseObj) {
    m_baseObj->Reenable();
    SetMirrorHandlers();
    AddWorldObject();
  }
}

void CGGameObject_C::PostReenable() {
  CGObject_C::PostReenable();
  if (m_baseObj) {
    m_baseObj->PostReenable();
  } else {
    unsigned __int64 guid = GetGUID();
    const GameObjectStats_C *stats =
        g_gameObjectDBCache.GetRecord(GetEntryID(), guid, GameObjectStatsCallback, 0);
    if (stats) {
      LoadBaseObject(stats);
    }
  }
}

int CGGameObject_C::SetBlock(unsigned int, unsigned long) {
  FATALASSERT(0);
  return 1;
}

void CGGameObject_C::SetData(const void *data, unsigned int bytes) {
  FATALASSERT(bytes <= sizeof(*m_gameObj));
  memcpy(m_gameObj, data, bytes);
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

int CGGameObject_C::GetType() const {
  return m_stats ? m_stats->m_typeID : -1;
}

unsigned int CGGameObject_C::GetPropertyValue(unsigned int index) const {
  return m_stats && index < 10 ? m_stats->m_propValue[index] : 0;
}

LockRec *CGGameObject_C::GetLockRec() const {
  int lockID = GetPropertyValue(CGameObjectDef::GetPropNum(GetType(), 4));
  return g_lockDB.GetRecord(lockID);
}

bool CGGameObject_C::IsLocked(
    int *spellID,
    int *spellSkill,
    int *lockSkill,
    CGItem_C **itemPtr,
    int *openIndex) const {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(
      ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return false;
  }

  const LockRec *lock = GetLockRec();
  if (!lock) {
    return false;
  }

  bool locked = false;
  for (int i = 0; i < 4; ++i) {
    if (!lock->m_Type[i]) {
      continue;
    }

    if (lock->m_Type[i] == 2) {
      locked = true;
      if (!IsValidOpenAction(lock->m_Action[i])) {
        continue;
      }

      for (unsigned int j = 0; j < CGSpellBook::m_unlockSpells.Count(); ++j) {
        int spell = CGSpellBook::m_unlockSpells[j];
        const SpellRec *srec = g_spellDB.GetRecord(spell);
        FATALASSERT(srec);
        for (int effect = 0; effect < 3; ++effect) {
          if (srec->m_effect[effect] != 33 ||
              srec->m_effectMiscValue[effect] != lock->m_Index[i]) {
            continue;
          }

          int min;
          int max;
          Spell_C_GetMinMaxPoints(srec, effect, &min, &max, 0, 0);
          if (spellID) {
            *spellID = spell;
          }
          if (spellSkill) {
            *spellSkill = min;
          }
          if (lockSkill) {
            *lockSkill = lock->m_Skill[i];
          }
          if (min >= lock->m_Skill[i]) {
            if (openIndex) {
              *openIndex = i;
            }
            return false;
          }
        }
      }
    } else if (lock->m_Type[i] == 1) {
      locked = true;
      if (!IsValidOpenAction(lock->m_Action[i])) {
        continue;
      }

      CGItem_C *item = player->GetBag()->FindItemOfType(lock->m_Index[i], 0);
      if (item) {
        if (spellID) {
          *spellID = item->GetUseSpell();
        }
        if (itemPtr) {
          *itemPtr = item;
        }
        if (openIndex) {
          *openIndex = i;
        }
        return false;
      }
    }
  }
  return locked;
}

unsigned int CGGameObject_C::IsValidOpenAction(int action) const {
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

unsigned int CGGameObject_C::IsValidTargetForSpell(const unsigned __int64 &caster, int spellID) const {
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

void __fastcall CGGameObject_C::Initialize() {
  ClientServices_SetMessageHandler(SMSG_GAMEOBJECT_PAGETEXT, PageTextHandler, 0);
  ClientServices_SetMessageHandler(SMSG_GAMEOBJECT_CUSTOM_ANIM, CustomAnimHandler, 0);
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

int CGGameObject_C::IsSolidSelectable() const {
  return m_isSolid || CanHighlight();
}

int CGGameObject_C::IsSolidCollidable() const {
  return m_isSolid;
}

int CGGameObject_C::FloatingTooltip() const {
  return GetPropertyValue(CGameObjectDef::GetPropNum(GetType(), 19)) != 0;
}

void CGGameObject_C::OnRightClick() {
  FATALASSERT(m_baseObj);
  if (m_baseObj->CanUse()) {
    GAME_ERROR_TYPE reason;
    if (m_baseObj->CanUseNow(&reason)) {
      unsigned __int64 player = ClntObjMgrGetActivePlayer();
      m_baseObj->Use(player);
    } else {
      CGGameUI::DisplayError(reason);
    }
  }
}

int CGGameObject_C::CanChangeCursor() const {
  FATALASSERT(m_baseObj);
  return m_baseObj->CanChangeCursor();
}

int CGGameObject_C::CanUse() const {
  FATALASSERT(m_baseObj);
  return m_baseObj->CanUse();
}

int CGGameObject_C::CanUseNow() const {
  FATALASSERT(m_baseObj && m_baseObj->CanUse());
  return m_baseObj->CanUseNow(0);
}

const char *CGGameObject_C::GetName() const {
  return m_stats ? m_stats->m_name[0] : "";
}

const char *CGGameObject_C::GetTypeName() const {
  return m_stats ? CGameObjectDef::NameFromTypeId(m_stats->m_typeID) : "UNKNOWN";
}

const char *CGGameObject_C::GetDebugStatus() const {
  FATALASSERT(m_baseObj);
  return m_baseObj->DebugStatus();
}

const char *CGGameObject_C::GetObjectName() const {
  return GetName();
}

int CGGameObject_C::GetPageTextID(
    void(__fastcall *)(int, const unsigned __int64 &, void *, bool)) const {
  return GetPropertyValue(CGameObjectDef::GetPropNum(GetType(), 15));
}

NTempest::C3Vector CGGameObject_C::GetCurrentMoveVector() const {
  FATALASSERT(m_baseObj);
  return m_baseObj->GetCurrentMoveVector();
}

NTempest::C34Matrix CGGameObject_C::GetMatrix() const {
  return m_matrix;
}

void CGGameObject_C::GetWorldMatrix(NTempest::C34Matrix *worldMatrix) const {
  *worldMatrix = CGObject_C::GetMatrix();
}

void CGGameObject_C::ObjectPostAnimate(
    const NTempest::C34Matrix &,
    const NTempest::C3Vector &,
    const NTempest::C3Vector &) {
  if (GetObjectModel()) {
    ModelShowCollision(GetObjectModel(), CWorld::enables & CWorld::Enable_Collision);
  }
}

unsigned int CGGameObject_C::CreateWorldObject(unsigned __int64 guid) {
  NTempest::C3Vector position = m_gameObj->m_position;
  return CWorld::ObjectCreate(GetModelFileNameInternal(), position, GetFacing(), 0, 0, guid);
}

void CGGameObject_C::SetMirrorHandlers() {
  ClntObjMgrSetObjMirrorHandler(
      GetGUID(), OffsetOf(ID_GAMEOBJECT) + 24, 4, OnUpdateState, 0, HANDLER_PRIORITY_NORMAL);
}

void CGGameObject_C::UnsetMirrorHandlers() {
  ClntObjMgrUnsetObjMirrorHandler(
      GetGUID(), OffsetOf(ID_GAMEOBJECT) + 24, OnUpdateState, 0);
}

unsigned int __fastcall CGGameObject_C::OffsetOf(OBJECT_TYPE_ID type) {
  if (type == ID_OBJECT) {
    return 0;
  }
  FATALASSERT(type == ID_GAMEOBJECT);
  return 24;
}

void CGGameObject_C::PostPostInit() {
  UpdateMatrix();
  ClntObjMgrShowObject(GetGUID());
  if (GetObjectModel()) {
    AddWorldObject();
    ModelSetEventCallback(GetObjectModel(), AnimEventCallback, this, 0);
    ModelSetSeqFinishedHandler(GetObjectModel(), AnimFinishedCallback, this);
  }
}

void CGGameObject_C::UpdateMatrix() {
  NTempest::CAaBox localExtents;
  memset(&localExtents, 0, sizeof(localExtents));

  m_matrix = NTempest::C34Matrix();
  m_matrix.Translate(GetPosition());
  m_matrix.Rotate(*reinterpret_cast<NTempest::C4Quaternion *>(&m_gameObj->m_data[2]));
  m_matrix.Scale(GetScale());
  if (GetObjectModel()) {
    ModelGetCollisionExtents(GetObjectModel(), &localExtents);
  }
  CWorldMath::TransformAABox(m_matrix, localExtents, m_collideExtents);
}

CGGameObject_C_Type_Null CGGameObject_C::s_nullBaseObj;

CGGameObject_C_TypeAnimated::CGGameObject_C_TypeAnimated(CGGameObject_C *owner)
    : CGGameObject_C_TypeBase(owner), m_animState(0), m_loopingSound(0), m_animPresent(0) {
  memset(m_useFallbackAnim, 0, sizeof(m_useFallbackAnim));
}

CGGameObject_C_TypeAnimated::~CGGameObject_C_TypeAnimated() {
  CloseLoopingSound();
}

void CGGameObject_C_TypeAnimated::UpdateState(int oldState, int newState) {
  CloseLoopingSound();
  if (newState == 0) {
    UpdateAnimState(oldState == 1 ? 1 : 2);
  } else if (newState == 1) {
    if (oldState == 0) {
      UpdateAnimState(3);
    } else {
      UpdateAnimState(oldState == 2 ? 6 : 0);
    }
  } else if (newState == 2) {
    UpdateAnimState(oldState == 1 ? 4 : 5);
  }
}

void CGGameObject_C_TypeAnimated::HandleAnimEvent(
    const char *eventName, const NTempest::C3Vector &position) {
  FATALASSERT(m_owner);

  unsigned int event = *reinterpret_cast<const unsigned int *>(eventName);
  switch (event) {
    case 0x304F4724:  // $GO0
    case 0x314F4724:  // $GO1
    case 0x324F4724:  // $GO2
    case 0x334F4724:  // $GO3
    case 0x344F4724:  // $GO4
    case 0x354F4724:  // $GO5
      PlayAnimatedSound(eventName[3] - '0', position);
      break;
    case 0x30434724:  // $GC0
    case 0x31434724:  // $GC1
    case 0x32434724:  // $GC2
    case 0x33434724:  // $GC3
      PlayAnimatedSound(eventName[3] - '*', position);
      break;
    case 0x444E5324:  // $SND
      if (eventName[4]) {
        SndInterfacePlaySound(SStrToInt(eventName + 4), position, -1, 1.0f);
      }
      break;
    case 0x4B485324:  // $SHK
      if (eventName[4]) {
        SpellVisualsPlayCameraShakeID(SStrToInt(eventName + 4), position);
      }
      break;
  }
}

void CGGameObject_C_TypeAnimated::HandleAnimFinished() {
  FATALASSERT(m_animState < sizeof(s_stateAnimInfo) / sizeof(s_stateAnimInfo[0]));

  switch (m_animState) {
    case 0:
    case 2:
      if (!m_useFallbackAnim[m_animState] || !(m_animPresent & 2)) {
        SetSequence();
      }
      break;
    case 1:
      UpdateAnimState(2);
      break;
    case 3:
    case 6:
      UpdateAnimState(0);
      break;
    case 4:
      UpdateAnimState(5);
      break;
    case 5:
      if (!m_useFallbackAnim[5] || !(m_animPresent & 0x10)) {
        SetSequence();
      }
      break;
    case 7:
    case 8:
    case 9:
    case 10: {
      int state = m_owner->GetState();
      UpdateState(state, state);
      break;
    }
  }
}

const char *CGGameObject_C_TypeAnimated::DebugStatus() {
  FATALASSERT(m_animState < sizeof(s_statusString) / sizeof(s_statusString[0]));
  return s_statusString[m_animState];
}

void CGGameObject_C_TypeAnimated::ActivateCustomAnim(unsigned int anim) {
  FATALASSERT(anim < 4);
  UpdateAnimState(anim + 7);
}

void CGGameObject_C_TypeAnimated::UpdateAnimState(unsigned int newState) {
  FATALASSERT(newState < sizeof(s_stateAnimInfo) / sizeof(s_stateAnimInfo[0]));
  m_animState = newState;
  if (m_owner->GetObjectModel() && m_owner->IsObjectModelLoaded()) {
    SetSequence();
  }
}

void CGGameObject_C_TypeAnimated::PostInit() {
  CGGameObject_C_TypeBase::PostInit();
  int state = m_owner->GetState();
  UpdateState(state, state);
}

void CGGameObject_C_TypeAnimated::Disable(int shutdown) {
  CloseLoopingSound();
  CGGameObject_C_TypeBase::Disable(shutdown);
}

void CGGameObject_C_TypeAnimated::ModelJustLoaded() {
  HMODEL__ *model = m_owner->GetObjectModel();
  unsigned int i;
  for (i = 0; i < sizeof(s_stateAnimInfo) / sizeof(s_stateAnimInfo[0]); ++i) {
    if (ModelHasSequenceId(model, s_stateAnimInfo[i].sequence)) {
      m_useFallbackAnim[i] = 0;
      m_animPresent |= 1 << i;
    } else {
      m_useFallbackAnim[i] = !s_stateAnimInfo[i].neverUseFallback;
    }
  }
  UpdateAnimState(m_animState);
  m_owner->m_isSolid =
      m_owner->m_collideExtents.b.x < m_owner->m_collideExtents.t.x &&
      m_owner->m_collideExtents.b.y < m_owner->m_collideExtents.t.y &&
      m_owner->m_collideExtents.b.z < m_owner->m_collideExtents.t.z;
}

void CGGameObject_C_TypeAnimated::PlayAnimatedSound(
    int index, const NTempest::C3Vector &position) {
  if (index == -1) {
    return;
  }

  GameObjectDisplayInfoRec *displayInfo =
      g_gameObjectDisplayInfoDB.GetRecord(m_owner->GetGameObjectData()->m_data[0]);
  if (!displayInfo) {
    return;
  }

  bool looping;
  unsigned int soundID = displayInfo->m_Sound[index];
  if (!SoundInterfaceIsSoundLooping(soundID, looping)) {
    return;
  }

  if (looping) {
    CloseLoopingSound();
    m_loopingSound = SndInterfacePlayLoopedSound(soundID, position, 0);
  } else {
    SndInterfacePlaySound(soundID, position, -1, 1.0f);
  }
}

void CGGameObject_C_TypeAnimated::CloseLoopingSound() {
  if (m_loopingSound) {
    Sound::KillSound(m_loopingSound);
    m_loopingSound = 0;
  }
}

void CGGameObject_C_TypeAnimated::SetSequence() {
  HMODEL__ *model = m_owner->GetObjectModel();
  FATALASSERT(model);

  unsigned int sequence;
  if (!m_useFallbackAnim[m_animState]) {
    if (!(m_animPresent & (1 << m_animState))) {
      return;
    }
    sequence = s_stateAnimInfo[m_animState].sequence;
  } else {
    unsigned int fallbackState = m_animState < 4 ? 1 : 4;
    sequence = m_animPresent & (1 << fallbackState)
        ? s_stateAnimInfo[fallbackState].sequence
        : 0;
  }

  ModelSetRandomSequenceFidget(model, sequence, 4);
  if (!sequence) {
    ModelSetTimeScale(model, 1.0f, 0);
    return;
  }

  ModelSetTimeScale(model, s_stateAnimInfo[m_animState].reverse ? -1.0f : 1.0f, 0);
  if (s_stateAnimInfo[m_animState].setAtEnd) {
    ModelForceSequenceTime(model, sequence, 0x7FFFFFFF, 0);
  }
}

CGGameObject_C_Type_Door::CGGameObject_C_Type_Door(CGGameObject_C *owner)
    : CGGameObject_C_TypeAnimated(owner) {
}

unsigned int CGGameObject_C_Type_Door::CanUseNow(GAME_ERROR_TYPE *reason) {
  if (IsAtRest()) {
    return CGGameObject_C_TypeBase::CanUseNow(reason);
  }
  if (reason) {
    *reason = GERR_USE_OBJECT_MOVING;
  }
  return 0;
}

void CGGameObject_C_Type_Door::UpdateAnimState(unsigned int newState) {
  CGGameObject_C_TypeAnimated::UpdateAnimState(newState);
  m_owner->m_isSolid = m_animState == 0;
}

unsigned int CGGameObject_C_Type_Door::IsAtRest() {
  if (!GetAutoClose()) {
    return 1;
  }
  return GetStartOpen() ? m_animState != 0 : m_animState != 2;
}

unsigned int CGGameObject_C_Type_Door::GetStartOpen() {
  return m_owner->GetPropertyValue(CGameObjectDef::GetPropNum(m_owner->GetType(), 1));
}

unsigned int CGGameObject_C_Type_Door::GetAutoClose() {
  return m_owner->GetPropertyValue(CGameObjectDef::GetPropNum(m_owner->GetType(), 3));
}

CGGameObject_C_Type_Button::CGGameObject_C_Type_Button(CGGameObject_C *owner)
    : CGGameObject_C_TypeAnimated(owner) {
}

CGGameObject_C_Type_Chest::CGGameObject_C_Type_Chest(CGGameObject_C *owner)
    : CGGameObject_C_TypeAnimated(owner) {
}

CGGameObject_C_Type_Trap::CGGameObject_C_Type_Trap(CGGameObject_C *owner)
    : CGGameObject_C_TypeAnimated(owner) {
}

CGGameObject_C_Type_AreaDamage::CGGameObject_C_Type_AreaDamage(CGGameObject_C *owner)
    : CGGameObject_C_TypeAnimated(owner) {
  m_owner->m_isSolid = 0;
  m_interactDistance = 0.0f;
}

void CGGameObject_C_Type_AreaDamage::ModelJustLoaded() {
}

CGGameObject_C_Type_QuestGiver::CGGameObject_C_Type_QuestGiver(CGGameObject_C *owner)
    : CGGameObject_C_TypeAnimated(owner) {
  m_interactDistance = MAX_SHOP_DISTANCE;
}

void CGGameObject_C_Type_QuestGiver::StartInteraction() {
  UpdateAnimState(1);
}

void CGGameObject_C_Type_QuestGiver::CloseInteraction() {
  UpdateAnimState(3);
}

CGGameObject_C_Type_Binder::CGGameObject_C_Type_Binder(CGGameObject_C *owner)
    : CGGameObject_C_TypeBase(owner) {
  m_interactDistance = MAX_BIND_DISTANCE;
}

CGGameObject_C_Type_Generic::CGGameObject_C_Type_Generic(CGGameObject_C *owner)
    : CGGameObject_C_TypeBase(owner) {
}

unsigned int CGGameObject_C_Type_Generic::CanHighlight() {
  int prop = CGameObjectDef::GetPropNum(m_owner->GetType(), 18);
  return m_owner->GetPropertyValue(prop) != 0;
}

unsigned int CGGameObject_C_Type_Generic::CanUse() {
  return 0;
}

CGGameObject_C_Type_MapObj::CGGameObject_C_Type_MapObj(CGGameObject_C *owner)
    : CGGameObject_C_TypeBase(owner), m_objectId(0) {
  m_owner->m_isSolid = 0;
}

CGGameObject_C_Type_MapObj::~CGGameObject_C_Type_MapObj() {
  if (m_objectId) {
    CWorld::ObjectDelete(m_objectId);
  }
}

unsigned int CGGameObject_C_Type_MapObj::CanHighlight() {
  return 0;
}

unsigned int CGGameObject_C_Type_MapObj::CanUse() {
  return 0;
}

void CGGameObject_C_Type_MapObj::PostInit() {
  m_objectId = m_owner->CreateWorldObject(m_owner->GetType() == 15 ? m_owner->GetGUID() : 0);
}

CGGameObject_C_Type_MapObjTransport::CGGameObject_C_Type_MapObjTransport(CGGameObject_C *owner)
    : CGGameObject_C_Type_MapObj(owner), m_position(), m_facing(0.0f) {
  MovementAddTransport(m_owner);

  float speed = static_cast<float>(
      m_owner->GetPropertyValue(CGameObjectDef::GetPropNum(15, 37)));
  int pathID[2] = {
      m_owner->GetPropertyValue(CGameObjectDef::GetPropNum(15, 35)),
      m_owner->GetPropertyValue(CGameObjectDef::GetPropNum(15, 36))
  };

  unsigned int maxPoints = g_taxiPathNodeDB.GetNumRecords();
  TSStackArray<NTempest::C3Vector> points(
      _alloca(maxPoints * sizeof(NTempest::C3Vector)), maxPoints, 0);
  for (unsigned int path = 0; path < 2; ++path) {
    for (unsigned int i = 0; i < maxPoints; ++i) {
      TaxiPathNodeRec *node = g_taxiPathNodeDB.GetRecordByIndex(i);
      if (node && node->m_PathID == pathID[path]) {
        points.New(NTempest::C3Vector(node->m_LocX, node->m_LocY, node->m_LocZ));
      }
    }
    m_path[path].SetPoints(points.Ptr(), points.Count());
    m_tripTime[path] =
        static_cast<unsigned int>(m_path[path].cachedLength / speed * 1000.0f + 0.5f);
    points.SetCount(0);
  }

  UpdateMovement(OsGetAsyncTimeMs(), 0.0f);
}

CGGameObject_C_Type_MapObjTransport::~CGGameObject_C_Type_MapObjTransport() {
}

NTempest::C3Vector CGGameObject_C_Type_MapObjTransport::GetPosition() const {
  return m_position;
}

float CGGameObject_C_Type_MapObjTransport::GetFacing() const {
  return m_facing;
}

void CGGameObject_C_Type_MapObjTransport::AddPassenger(CMovementData *passenger) {
  FATALASSERT(passenger);
  m_passengers.LinkNode(passenger, LIST_TAIL, 0);
}

int CGGameObject_C_Type_MapObjTransport::IsPointInside(const NTempest::C3Vector &point) const {
  return CWorld::ObjectTestConvexVolume(m_objectId, point);
}

void CGGameObject_C_Type_MapObjTransport::Reenable() {
  m_objectId = m_owner->CreateWorldObject(m_owner->GetGUID());
  MovementAddTransport(m_owner);
}

void CGGameObject_C_Type_MapObjTransport::Disable(int shutdown) {
  unsigned long eventTime = OsGetAsyncTimeMs();
  for (CMovementData *passenger = m_passengers.Head(); passenger;) {
    CMovementData *next = m_passengers.RawNext(passenger);
    CMovement *movement = static_cast<CMovement *>(passenger);
    if (passenger->m_guid == ClntObjMgrGetActivePlayer()) {
      movement->OnFallLocal(eventTime);
    } else {
      movement->OnFall(eventTime);
    }
    passenger->ForceSetTransport(0);
    passenger = next;
  }

  MovementRemoveTransport(m_owner);
  if (!shutdown && m_objectId) {
    CWorld::ObjectDelete(m_objectId);
  }
}

void CGGameObject_C_Type_MapObjTransport::UpdateMovement(
    unsigned long eventTime, float) {
  NTempest::C34Matrix matrix;
  unsigned int time =
      (eventTime + m_owner->m_serverTimeOffset) %
      (m_tripTime[0] + m_tripTime[1] + 40000);

  if (time < 20000) {
    m_path[0].Frame(0.0f, matrix, NTempest::C3Spline::EVAL_ARCLENGTH);
  } else if (time < m_tripTime[0] + 20000) {
    float t = static_cast<float>(time - 20000) / m_tripTime[0];
    m_path[0].Frame(t, matrix, NTempest::C3Spline::EVAL_ARCLENGTH);
  } else if (time < m_tripTime[0] + 40000) {
    m_path[1].Frame(0.0f, matrix, NTempest::C3Spline::EVAL_ARCLENGTH);
  } else {
    float t =
        static_cast<float>(time - m_tripTime[0] - 40000) / m_tripTime[1];
    m_path[1].Frame(t, matrix, NTempest::C3Spline::EVAL_ARCLENGTH);
  }

  NTempest::C2Vector direction(-matrix.a0, -matrix.a1);
  float magnitude = direction.Mag();
  if (fabs(magnitude) >= 2.3841858e-7f) {
    direction.x /= magnitude;
    direction.y /= magnitude;
  }
  m_position = NTempest::C3Vector(matrix.d0, matrix.d1, matrix.d2);
  m_facing = static_cast<float>(atan2(direction.y, direction.x));

  if (m_objectId) {
    CWorld::ObjectUpdate(m_objectId, m_position, m_facing, 0);
  }

  for (CMovementData *passenger = m_passengers.Head(); passenger;
       passenger = m_passengers.RawNext(passenger)) {
    CGObject_C *unit =
        ClntObjMgrObjectPtr(passenger->m_guid, __FILE__, __LINE__);
    FATALASSERT(unit);
    unit->UpdateWorldObject();
  }
}

CGGameObject_C_Type_Chair::CGGameObject_C_Type_Chair(CGGameObject_C *owner)
    : CGGameObject_C_TypeBase(owner) {
  memset(m_slotPositions, 0, sizeof(m_slotPositions));
}

unsigned int CGGameObject_C_Type_Chair::CanUseNow(GAME_ERROR_TYPE *reason) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(
      ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player || player->GetUnitData()->health <= 0) {
    if (reason) {
      *reason = GERR_PLAYER_DEAD;
    }
    return 0;
  }

  NTempest::C3Vector playerPosition = player->m_movement.GetPosition();
  for (unsigned int i = 0; i < GetNumSlots(); ++i) {
    if ((playerPosition - m_slotPositions[i]).SquaredMag() <=
        MAX_SITCHAIRUSE_DISTANCE_SQUARED) {
      return 1;
    }
  }

  if (reason) {
    *reason = GERR_USE_TOO_FAR;
  }
  return 0;
}

void CGGameObject_C_Type_Chair::PostInit() {
  CGGameObject_C_TypeBase::PostInit();
  FATALASSERT(GetNumSlots());
  FATALASSERT(GetNumSlots() <= 5);

  NTempest::C34Matrix ownerMatrix = m_owner->CGObject_C::GetMatrix();
  NTempest::C44Matrix matrix(
      ownerMatrix.a0, ownerMatrix.a1, ownerMatrix.a2, 0.0f,
      ownerMatrix.b0, ownerMatrix.b1, ownerMatrix.b2, 0.0f,
      ownerMatrix.c0, ownerMatrix.c1, ownerMatrix.c2, 0.0f,
      ownerMatrix.d0, ownerMatrix.d1, ownerMatrix.d2, 1.0f);
  GenerateChairPoints(matrix, GetNumSlots(), m_slotPositions);
}

unsigned int CGGameObject_C_Type_Chair::GetNumSlots() {
  return m_owner->GetPropertyValue(CGameObjectDef::GetPropNum(m_owner->GetType(), 11));
}

unsigned int CGGameObject_C_Type_Chair::GetHeight() {
  return m_owner->GetPropertyValue(CGameObjectDef::GetPropNum(m_owner->GetType(), 12));
}

CGGameObject_C_Type_SpellFocus::CGGameObject_C_Type_SpellFocus(CGGameObject_C *owner)
    : CGGameObject_C_TypeAnimated(owner) {
}

unsigned int CGGameObject_C_Type_SpellFocus::CanHighlight() {
  return 1;
}

unsigned int CGGameObject_C_Type_SpellFocus::CanUse() {
  return 0;
}

CGGameObject_C_Type_Text::CGGameObject_C_Type_Text(CGGameObject_C *owner)
    : CGGameObject_C_TypeAnimated(owner) {
  m_interactDistance = MAX_SHOP_DISTANCE;
}

unsigned int CGGameObject_C_Type_Text::Use(const unsigned __int64 &activator) {
  FATALASSERT(m_owner);
  CGItemText::SetItem(m_owner->GetGUID(), 0);
  return 1;
}

void CGGameObject_C_Type_Text::PostInit() {
  CGGameObject_C_TypeAnimated::PostInit();
  FATALASSERT(m_owner);
  if (CGItemText::GetItem() == m_owner->GetGUID()) {
    CGItemText::SetItem(m_owner->GetGUID(), 1);
  }
}

void CGGameObject_C_Type_Text::StartInteraction() {
  UpdateAnimState(1);
}

void CGGameObject_C_Type_Text::CloseInteraction() {
  UpdateAnimState(3);
}

CGGameObject_C_Type_Goober::CGGameObject_C_Type_Goober(CGGameObject_C *owner)
    : CGGameObject_C_TypeAnimated(owner) {
}

CGGameObject_C_Type_Transport::CGGameObject_C_Type_Transport(CGGameObject_C *owner)
    : CGGameObject_C_TypeAnimated(owner), m_keys(0), m_numKeys(0), m_currKey(0),
      m_position(), m_currSpeed(0.0f), m_currDirection(0.0f, 0.0f, 1.0f) {
  MovementAddTransport(m_owner);

  int firstKey = FindAnimData(owner);
  FATALASSERT(firstKey != -1);
  m_keys = g_transportAnimationDB.GetRecordByIndex(firstKey);
  m_numKeys = 1;
  for (int i = firstKey + 1; i < g_transportAnimationDB.GetNumRecords(); ++i) {
    TransportAnimationRec *key = g_transportAnimationDB.GetRecordByIndex(i);
    if (key->m_TransportID != owner->GetEntryID()) {
      break;
    }
    ++m_numKeys;
  }

  m_position = m_owner->GetGameObjectData()->m_position +
      GetMovement(OsGetAsyncTimeMs());
}

CGGameObject_C_Type_Transport::~CGGameObject_C_Type_Transport() {
}

NTempest::C3Vector CGGameObject_C_Type_Transport::GetPosition() const {
  return m_position;
}

void CGGameObject_C_Type_Transport::AddPassenger(CMovementData *passenger) {
  FATALASSERT(passenger);
  m_passengers.LinkNode(passenger, LIST_TAIL, 0);
}

NTempest::C3Vector CGGameObject_C_Type_Transport::GetCurrentMoveVector() const {
  return m_currDirection * m_currSpeed;
}

unsigned int CGGameObject_C_Type_Transport::CanUse() {
  return 0;
}

int CGGameObject_C_Type_Transport::IsPointInside(const NTempest::C3Vector &point) const {
  unsigned int i;
  for (i = 0; i < m_interior.Count(); ++i) {
    if (NTempest::C3Vector::Dot(m_interior[i].n, point) + m_interior[i].d > 0.0f) {
      return 0;
    }
  }
  return 1;
}

void CGGameObject_C_Type_Transport::Reenable() {
}

void CGGameObject_C_Type_Transport::Disable(int) {
  unsigned long eventTime = OsGetAsyncTimeMs();
  for (CMovementData *passenger = m_passengers.Head(); passenger;) {
    CMovementData *next = m_passengers.RawNext(passenger);
    CMovement *movement = static_cast<CMovement *>(passenger);
    if (passenger->m_guid == ClntObjMgrGetActivePlayer()) {
      movement->OnFallLocal(eventTime);
    } else {
      movement->OnFall(eventTime);
    }
    passenger->ForceSetTransport(0);
    passenger = next;
  }
  MovementRemoveTransport(m_owner);
}

void CGGameObject_C_Type_Transport::UpdateMovement(
    unsigned long eventTime, float elapsed) {
  NTempest::C3Vector oldPosition = m_position;
  m_position =
      m_owner->GetGameObjectData()->m_position + GetMovement(eventTime);
  NTempest::C3Vector move = m_position - oldPosition;

  m_owner->UpdateMatrix();
  m_owner->UpdateWorldObject();

  float distance = move.Mag();
  m_currSpeed = distance / elapsed;
  if (fabs(distance) >= 2.3841858e-7f) {
    m_currDirection = move * (1.0f / distance);
  }

  for (CMovementData *passenger = m_passengers.Head(); passenger;
       passenger = m_passengers.RawNext(passenger)) {
    CGObject_C *unit =
        ClntObjMgrObjectPtr(passenger->m_guid, __FILE__, __LINE__);
    FATALASSERT(unit);
    unit->UpdateWorldObject();
  }
}

void CGGameObject_C_Type_Transport::ModelJustLoaded() {
  CGGameObject_C_TypeAnimated::ModelJustLoaded();

  NTempest::CAaBox bounds(0.0f);
  ModelGetExtents(m_owner->GetObjectModel(), &bounds);
  m_interior.SetCount(6);
  m_interior[0].Set(
      NTempest::C3Vector(1.0f, 0.0f, 0.0f),
      NTempest::C3Vector(bounds.t.x, 0.0f, 0.0f));
  m_interior[1].Set(
      NTempest::C3Vector(0.0f, 1.0f, 0.0f),
      NTempest::C3Vector(0.0f, bounds.t.y, 0.0f));
  m_interior[2].Set(
      NTempest::C3Vector(0.0f, 0.0f, 1.0f),
      NTempest::C3Vector(0.0f, 0.0f, bounds.t.z));
  m_interior[3].Set(
      NTempest::C3Vector(-1.0f, 0.0f, 0.0f),
      NTempest::C3Vector(bounds.b.x, 0.0f, 0.0f));
  m_interior[4].Set(
      NTempest::C3Vector(0.0f, -1.0f, 0.0f),
      NTempest::C3Vector(0.0f, bounds.b.y, 0.0f));
  m_interior[5].Set(
      NTempest::C3Vector(0.0f, 0.0f, -1.0f),
      NTempest::C3Vector(0.0f, 0.0f, bounds.b.z));
}

NTempest::C3Vector CGGameObject_C_Type_Transport::GetMovement(
    unsigned int eventTime) {
  FATALASSERT(m_numKeys > 1);

  unsigned int time =
      (eventTime + m_owner->m_serverTimeOffset) %
      m_keys[m_numKeys - 1].m_TimeIndex;
  unsigned int nextKeyID = NextKeyID();
  const TransportAnimationRec *key = &m_keys[m_currKey];
  const TransportAnimationRec *nextKey = &m_keys[nextKeyID];
  while (time < static_cast<unsigned int>(key->m_TimeIndex) ||
         time >= static_cast<unsigned int>(nextKey->m_TimeIndex)) {
    m_currKey = nextKeyID;
    nextKeyID = NextKeyID();
    key = &m_keys[m_currKey];
    nextKey = &m_keys[nextKeyID];
  }

  float ratio =
      static_cast<float>(time - key->m_TimeIndex) /
      static_cast<float>(nextKey->m_TimeIndex - key->m_TimeIndex);
  NTempest::C3Vector movement(
      key->m_PosX * (1.0f - ratio) + nextKey->m_PosX * ratio,
      key->m_PosY * (1.0f - ratio) + nextKey->m_PosY * ratio,
      key->m_PosZ * (1.0f - ratio) + nextKey->m_PosZ * ratio);
  NTempest::C4Quaternion *rotation =
      reinterpret_cast<NTempest::C4Quaternion *>(
          &m_owner->m_gameObj->m_data[2]);
  NTempest::C33Matrix matrix = *rotation;
  return matrix * movement;
}

int CGGameObject_C_Type_Transport::FindAnimData(CGGameObject_C *owner) {
  int entryID = owner->GetEntryID();
  for (int i = 0; i < g_transportAnimationDB.GetNumRecords(); ++i) {
    TransportAnimationRec *key = g_transportAnimationDB.GetRecordByIndex(i);
    if (key->m_TransportID == entryID) {
      return i;
    }
    if (key->m_TransportID > entryID) {
      break;
    }
  }
  return -1;
}

unsigned int CGGameObject_C_Type_Transport::NextKeyID() {
  return m_currKey + 1 == m_numKeys ? 0 : m_currKey + 1;
}

CGGameObject_C_Type_Camera::CGGameObject_C_Type_Camera(CGGameObject_C *owner)
    : CGGameObject_C_TypeBase(owner) {
}

CGGameObject_C_Type_DuelArbiter::CGGameObject_C_Type_DuelArbiter(CGGameObject_C *owner)
    : CGGameObject_C_TypeBase(owner) {
}

unsigned int CGGameObject_C_Type_DuelArbiter::CanHighlight() {
  return 1;
}

unsigned int CGGameObject_C_Type_DuelArbiter::CanUse() {
  return 1;
}

CGGameObject_C_Type_FishingNode::CGGameObject_C_Type_FishingNode(CGGameObject_C *owner)
    : CGGameObject_C_TypeAnimated(owner) {
  m_interactDistance = MAX_OBJ_INTEREST_RADIUS;
}

unsigned int CGGameObject_C_Type_FishingNode::CanUse() {
  unsigned __int64 activePlayer = ClntObjMgrGetActivePlayer();
  CGUnit_C *player = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(activePlayer, __FILE__, __LINE__));
  if (!player || player->GetUnitData()->summonedBy != m_owner->GetGUID()) {
    return 0;
  }
  return CGGameObject_C_TypeBase::CanUse();
}

CGGameObject_C_Type_Ritual::CGGameObject_C_Type_Ritual(CGGameObject_C *owner)
    : CGGameObject_C_TypeAnimated(owner) {
}

unsigned int CGGameObject_C_Type_Ritual::CanUseNow(GAME_ERROR_TYPE *reason) {
  return 1;
}
