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

#include <stddef.h>
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

#define MAX_CHAIR_SLOTS 5

static const char NONAME[7] = "NoName";

inline UINT CGGameObject_C_Type_Door::GetStartOpen() const {
  return m_owner->GetPropertyValue(CGameObjectDef::GetPropNum(m_owner->GetType(), 1));
}

inline UINT CGGameObject_C_Type_Door::GetAutoClose() const {
  return m_owner->GetPropertyValue(CGameObjectDef::GetPropNum(m_owner->GetType(), 3));
}

inline UINT CGGameObject_C_Type_Chair::GetNumSlots() const {
  return m_owner->GetPropertyValue(CGameObjectDef::GetPropNum(7, 11));
}

inline UINT CGGameObject_C_Type_Chair::GetHeight() const {
  return m_owner->GetPropertyValue(CGameObjectDef::GetPropNum(m_owner->GetType(), 12));
}

inline void CGGameObject_C::SetSolid(bool solid) {
  m_isSolid = solid;
}

inline NTempest::C3Vector CGGameObject::GetObjectPosition() const {
  return m_gameObj->m_position;
}

inline UINT CGGameObject::GetGameObjectFlags() const {
  return m_gameObj->m_flags;
}

inline float CGUnit::LinearDistanceSquared(const NTempest::C3Vector &position) const {
  return (GetPosition() - position).SquaredMag();
}

inline NTempest::CAaBox CGGameObject_C::GetCollideExtents() const {
  return m_collideExtents;
}

inline bool CGGameObject::GetDisabled() const {
  return GetGameObjectFlags() & 1;
}

inline bool CGGameObject::GetLocked() const {
  return (GetGameObjectFlags() >> 1) & 1;
}

inline bool CGGameObject_C::IsQuestObjectForMe() {
  return m_gameObj->m_dynamicFlags & 1;
}

enum {
  ANIMSTATE_CLOSED = 0,
  ANIMSTATE_OPENING = 1,
  ANIMSTATE_OPEN = 2,
  ANIMSTATE_CLOSING = 3,
  ANIMSTATE_DESTROYING = 4,
  ANIMSTATE_DESTROYED = 5,
  ANIMSTATE_REBUILDING = 6,
  ANIMSTATE_FIRSTCUSTOM = 7,
  ANIMSTATE_CUSTOM0 = 7,
  ANIMSTATE_CUSTOM1 = 8,
  ANIMSTATE_CUSTOM2 = 9,
  ANIMSTATE_CUSTOM3 = 10,
  NUM_GAMEOBJ_ANIMSTATES = 11
};

struct StateAnimInfo {
  UINT seq;
  BYTE reverse;
  BYTE setAtEnd;
  BYTE neverUseFallback;
};

static StateAnimInfo s_stateAnimInfo[NUM_GAMEOBJ_ANIMSTATES] = {
    { 1, 1, 0, 0},
    { 2, 0, 0, 1},
    { 3, 0, 1, 0},
    { 4, 1, 1, 0},
    { 5, 0, 0, 1},
    { 6, 0, 1, 0},
    { 7, 1, 1, 0},
    { 8, 0, 0, 1},
    { 9, 0, 0, 1},
    {10, 0, 0, 1},
    {11, 0, 0, 1}
};

static LPCSTR s_statusString[NUM_GAMEOBJ_ANIMSTATES] = {"Closed", "Opening", "Open", "Closing", "Custom0", "Custom1", "Custom2", "Custom3", 0, 0, 0};

static CGGameObject_C_Type_Null s_nullBaseObj;

void ClntObjMgrHideObject(DWORDLONG guid);
void ClntObjMgrShowObject(DWORDLONG guid);
void MovementAddTransport(CGGameObject_C *transport);
void MovementRemoveTransport(CGGameObject_C *transport);
void Spell_C_GetMinMaxPoints(const SpellRec *spell, int effectIndex, int *min, int *max, UINT level, BOOL isPet);
void Spell_C_GetMinMaxRange(int spellID, float *min, float *max);
bool Spell_C_CastSpell(int spellID, const CGItem_C *item);
bool Spell_C_HandleSpriteClick(CGObject_C *object);
void SpellCameraShakeCallback(LPCSTR eventName, const NTempest::C3Vector &position);
void SpellSoundEffectCallback(LPCSTR eventName, const NTempest::C3Vector &position);

static BOOL PageTextHandler(LPVOID param, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  FATALASSERT(msg);
  DWORDLONG gameObject;
  msg->Get(gameObject);
  CGObject_C *object = ClntObjMgrObjectPtr(gameObject, __FILE__, __LINE__);
  if (object) {
    CGItemText::SetItem(object->GetGUID(), 0);
  }
  return 1;
}

static BOOL CustomAnimHandler(LPVOID param, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  FATALASSERT(msg);
  DWORDLONG gameObject;
  UINT      customAnim;
  msg->Get(gameObject);
  msg->Get(customAnim);
  CGGameObject_C *object = static_cast<CGGameObject_C *>(ClntObjMgrObjectPtr(gameObject, __FILE__, __LINE__));
  if (object && customAnim < 4) {
    object->ActivateCustomAnim(customAnim);
  }
  return 1;
}

CGGameObject_C_TypeBase::CGGameObject_C_TypeBase(CGGameObject_C *owner) : m_owner(owner) {
  FATALASSERT(m_owner);
  m_interactDistance = MAX_LOOT_DISTANCE;
}

void CGGameObject_C_TypeBase::PostInit() {
  FATALASSERT(m_owner);
  m_owner->PostPostInit();
}

bool CGGameObject_C_TypeBase::CanUse() const {
  DWORDLONG activePlayer = ClntObjMgrGetActivePlayer();
  CGUnit_C *player = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(activePlayer, __FILE__, __LINE__));
  return player && (m_owner->GetType() == 6 || m_owner->ObjectReaction(player) != UNIT_REACTION_HOSTILE) && !m_owner->GetDisabled() &&
         (!m_owner->GetQuestOnly() || m_owner->IsQuestObjectForMe());
}

bool CGGameObject_C_TypeBase::CanUseNow(GAME_ERROR_TYPE *reason) const {
  DWORDLONG   activePlayer = ClntObjMgrGetActivePlayer();
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(activePlayer, __FILE__, __LINE__));
  if (!player || player->GetHealth() <= 0) {
    if (reason) {
      *reason = GERR_PLAYER_DEAD;
    }
    return 0;
  }

  if (m_owner->GetLocked()) {
    if (reason) {
      *reason = GERR_USE_LOCKED;
    }
    return 0;
  }

  float range = m_interactDistance;
  int   spellID = 0;
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

  if (player->LinearDistanceSquared(m_owner->GetPosition()) > range * range) {
    if (reason) {
      *reason = GERR_USE_TOO_FAR;
    }
    return 0;
  }
  return 1;
}

bool CGGameObject_C_TypeBase::Use(const DWORDLONG &) {
  FATALASSERT(CanUseNow());

  int       spellID = 0;
  CGItem_C *item = 0;
  int       openIndex = 0;
  if (m_owner->IsLocked(&spellID, 0, 0, &item, &openIndex)) {
    const LockRec *lock = m_owner->GetLockRec();
    FATALASSERT(lock);

    if (m_owner->IsValidOpenAction(lock->m_Action[0])) {
      switch (lock->m_Type[0]) {
        case 1: {
          const ItemStats_C *stats = g_itemDBCache.GetRecord(lock->m_Index[0], m_owner->GetGUID(), 0, 0);
          if (stats) {
            CGGameUI::DisplayError(GERR_USE_LOCKED_WITH_ITEM_S, stats->m_displayName[0]);
          }
          break;
        }
        case 2: {
          const LockTypeRec *lockType = g_lockTypeDB.GetRecord(lock->m_Index[0]);
          if (spellID) {
            CGGameUI::DisplayError(GERR_USE_LOCKED_WITH_SPELL_KNOWN_SI, lockType ? lockType->m_name_lang[CURRENT_LANGUAGE] : "UNKNOWN", lock->m_Skill[0]);
          } else {
            CGGameUI::DisplayError(GERR_USE_LOCKED_WITH_SPELL_S, lockType ? lockType->m_name_lang[CURRENT_LANGUAGE] : "UNKNOWN");
          }
          break;
        }
        default:
          CGGameUI::DisplayError(GERR_USE_CANT_OPEN);
          break;
      }
    } else if (m_owner->GetType() == 3 && !m_owner->GetState()) {
      CGGameUI::DisplayError(GERR_CHEST_IN_USE);
    }
    return 0;
  }

  if (spellID) {
    if (Spell_C_CastSpell(spellID, item)) {
      Spell_C_HandleSpriteClick(m_owner);
      return 1;
    }
    return 0;
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

void CGGameObject_C_TypeBase::HandleAnimEvent(LPCSTR, const NTempest::C3Vector &) {
}

LPCSTR CGGameObject_C_TypeBase::DebugStatus() {
  return "";
}

NTempest::C3Vector CGGameObject_C_TypeBase::GetPosition() const {
  return m_owner->m_gameObj->m_position;
}

float CGGameObject_C_TypeBase::GetFacing() const {
  return m_owner->m_gameObj->m_facing;
}

bool CGGameObject_C_Type_Null::CanUse() const {
  return 0;
}

bool CGGameObject_C_Type_Null::CanUseNow(GAME_ERROR_TYPE *) const {
  return 0;
}

LPCSTR CGGameObject_C_Type_Null::DebugStatus() {
  return "Unknown Object Type";
}

void CGGameObject_C_TypeAnimated::ModelJustLoaded() {
  UINT i;
  for (i = 0; i < sizeof(s_stateAnimInfo) / sizeof(s_stateAnimInfo[0]); ++i) {
    if (ModelHasSequenceId(m_owner->GetObjectModel(), s_stateAnimInfo[i].seq)) {
      m_useFallbackAnim[i] = 0;
      m_animPresent |= 1 << i;
    } else {
      m_useFallbackAnim[i] = !s_stateAnimInfo[i].neverUseFallback;
    }
  }
  UpdateAnimState(m_animState);
  m_owner->SetSolid(m_owner->GetCollideExtents().NotEmpty());
}

void CGGameObject_C_TypeAnimated::Disable(int) {
  CloseLoopingSound();
}

CGGameObject_C_TypeAnimated::~CGGameObject_C_TypeAnimated() {
  CloseLoopingSound();
}

void CGGameObject_C_TypeAnimated::PostInit() {
  CGGameObject_C_TypeBase::PostInit();
  int state = m_owner->GetState();
  UpdateState(state, state);
}

void CGGameObject_C_TypeAnimated::SetSequence() {
  const StateAnimInfo &info = s_stateAnimInfo[m_animState];
  HMODEL               model = m_owner->GetObjectModel();
  FATALASSERT(model);

  if (m_useFallbackAnim[m_animState]) {
    UINT fallbackState = m_animState < 4 ? 1 : 4;
    UINT sequence = (1 << fallbackState) & m_animPresent ? s_stateAnimInfo[fallbackState].seq : 0;
    ModelSetRandomSequenceFidget(model, sequence, 4);
    if (sequence) {
      ModelSetTimeScale(model, info.reverse ? -1.0f : 1.0f, 1);
      if (info.setAtEnd) {
        ModelForceSequenceTime(model, sequence, 0x7FFFFFFF, 0);
      }
    } else {
      ModelSetTimeScale(model, 1.0f, 1);
    }
  } else if ((1 << m_animState) & m_animPresent) {
    ModelSetRandomSequenceFidget(model, info.seq, 4);
    ModelSetTimeScale(model, 1.0f, 1);
  }
}

void CGGameObject_C_TypeAnimated::UpdateAnimState(UINT newState) {
  FATALASSERT(newState < (sizeof(s_stateAnimInfo) / sizeof(s_stateAnimInfo[0])));
  m_animState = newState;
  if (m_owner->GetObjectModel() && m_owner->IsObjectModelLoaded()) {
    SetSequence();
  }
}

void CGGameObject_C_TypeAnimated::UpdateState(int oldState, int newState) {
  CloseLoopingSound();
  switch (newState) {
    case 0:
      if (oldState == 1) {
        UpdateAnimState(1);
      } else {
        UpdateAnimState(2);
      }
      break;
    case 1:
      if (oldState == 0) {
        UpdateAnimState(3);
      } else if (oldState == 2) {
        UpdateAnimState(6);
      } else {
        UpdateAnimState(0);
      }
      break;
    case 2:
      if (oldState == 1) {
        UpdateAnimState(4);
      } else {
        UpdateAnimState(5);
      }
      break;
  }
}

void CGGameObject_C_TypeAnimated::HandleAnimEvent(LPCSTR eventName, const NTempest::C3Vector &position) {
  FATALASSERT(m_owner);

  UINT event = *reinterpret_cast<const UINT *>(eventName);
  switch (event) {
    case '0OG$':
    case '1OG$':
    case '2OG$':
    case '3OG$':
    case '4OG$':
    case '5OG$':
      PlayAnimatedSound(eventName[3] - '0', position);
      break;
    case '0CG$':
    case '1CG$':
    case '2CG$':
    case '3CG$':
      PlayAnimatedSound(eventName[3] - '*', position);
      break;
    case 'DNS$':
      SpellSoundEffectCallback(eventName + 4, position);
      break;
    case 'KHS$':
      SpellCameraShakeCallback(eventName + 4, position);
      break;
  }
}

void CGGameObject_C_TypeAnimated::PlayAnimatedSound(int index, const NTempest::C3Vector &position) {
  if (index == -1) {
    return;
  }

  const GameObjectDisplayInfoRec *displayInfo = g_gameObjectDisplayInfoDB.GetRecord(m_owner->GameObject()->m_displayID);
  if (!displayInfo) {
    return;
  }

  bool looping;
  if (!SoundInterfaceIsSoundLooping(displayInfo->m_Sound[index], looping)) {
    return;
  }

  if (looping) {
    CloseLoopingSound();
    m_loopingSound = SndInterfacePlayLoopedSound(displayInfo->m_Sound[index], position, 0);
  } else {
    SndInterfacePlaySound(displayInfo->m_Sound[index], position, -1, 1.0f);
  }
}

void CGGameObject_C_TypeAnimated::CloseLoopingSound() {
  if (m_loopingSound) {
    Sound::KillSound(m_loopingSound);
    m_loopingSound = 0;
  }
}

void CGGameObject_C_TypeAnimated::HandleAnimFinished() {
  switch (m_animState) {
    case 1:
      UpdateAnimState(2);
      break;
    case 4:
      UpdateAnimState(5);
      break;
    case 3:
    case 6:
      UpdateAnimState(0);
      break;
    case 0:
    case 2:
      if (!m_useFallbackAnim[m_animState] || !(m_animPresent & 2)) {
        SetSequence();
      }
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
    default:
      FATALERROR(("Unhandled anim state: (%d)", m_animState));
      break;
  }
}

void CGGameObject_C_TypeAnimated::ActivateCustomAnim(UINT anim) {
  FATALASSERT(anim < 4);
  UpdateAnimState(anim + 7);
}

LPCSTR CGGameObject_C_TypeAnimated::DebugStatus() {
  FATALASSERT(m_animState < (sizeof(s_statusString) / sizeof(s_statusString[0])));
  return s_statusString[m_animState];
}

CGGameObject_C_Type_Door::CGGameObject_C_Type_Door(CGGameObject_C *owner) : CGGameObject_C_TypeAnimated(owner) {
}

bool CGGameObject_C_Type_Door::IsAtRest() const {
  if (GetAutoClose()) {
    return GetStartOpen() ? m_animState != 0 : m_animState != 2;
  }
  return 1;
}

bool CGGameObject_C_Type_Door::CanUseNow(GAME_ERROR_TYPE *reason) const {
  if (!IsAtRest()) {
    if (reason) {
      *reason = GERR_USE_OBJECT_MOVING;
    }
    return 0;
  }
  return CGGameObject_C_TypeBase::CanUseNow(reason);
}

void CGGameObject_C_Type_Door::UpdateAnimState(UINT newState) {
  CGGameObject_C_TypeAnimated::UpdateAnimState(newState);
  m_owner->SetSolid(m_animState == 0);
}

CGGameObject_C_Type_Button::CGGameObject_C_Type_Button(CGGameObject_C *owner) : CGGameObject_C_TypeAnimated(owner) {
}

CGGameObject_C_Type_Chest::CGGameObject_C_Type_Chest(CGGameObject_C *owner) : CGGameObject_C_TypeAnimated(owner) {
}

CGGameObject_C_Type_Trap::CGGameObject_C_Type_Trap(CGGameObject_C *owner) : CGGameObject_C_TypeAnimated(owner) {
}

void CGGameObject_C_Type_AreaDamage::ModelJustLoaded() {
}

CGGameObject_C_Type_AreaDamage::CGGameObject_C_Type_AreaDamage(CGGameObject_C *owner) : CGGameObject_C_TypeAnimated(owner) {
  m_owner->m_isSolid = 0;
  m_interactDistance = 0.0f;
}

CGGameObject_C_Type_QuestGiver::CGGameObject_C_Type_QuestGiver(CGGameObject_C *owner) : CGGameObject_C_TypeAnimated(owner) {
  m_interactDistance = MAX_SHOP_DISTANCE;
}

void CGGameObject_C_Type_QuestGiver::StartInteraction() {
  UpdateAnimState(1);
}

void CGGameObject_C_Type_QuestGiver::CloseInteraction() {
  UpdateAnimState(3);
}

CGGameObject_C_Type_Binder::CGGameObject_C_Type_Binder(CGGameObject_C *owner) : CGGameObject_C_TypeBase(owner) {
  m_interactDistance = MAX_BIND_DISTANCE;
}

bool CGGameObject_C_Type_Generic::CanUse() const {
  return 0;
}

CGGameObject_C_Type_Generic::CGGameObject_C_Type_Generic(CGGameObject_C *owner) : CGGameObject_C_TypeBase(owner) {
}

bool CGGameObject_C_Type_Generic::CanHighlight() const {
  int prop = CGameObjectDef::GetPropNum(m_owner->GetType(), 18);
  return m_owner->GetPropertyValue(prop) != 0;
}

bool CGGameObject_C_Type_MapObj::CanHighlight() const {
  return 0;
}

bool CGGameObject_C_Type_MapObj::CanUse() const {
  return 0;
}

CGGameObject_C_Type_MapObj::CGGameObject_C_Type_MapObj(CGGameObject_C *owner) : CGGameObject_C_TypeBase(owner), m_objectId(0) {
  m_owner->m_isSolid = 0;
}

void CGGameObject_C_Type_MapObj::PostInit() {
  m_objectId = m_owner->CreateWorldObject(m_owner->GetType() == 15 ? m_owner->GetGUID() : 0);
}

CGGameObject_C_Type_MapObj::~CGGameObject_C_Type_MapObj() {
  if (m_objectId) {
    CWorld::ObjectDelete(m_objectId);
  }
}

NTempest::C3Vector CGGameObject_C_Type_MapObjTransport::GetPosition() const {
  return m_position;
}

float CGGameObject_C_Type_MapObjTransport::GetFacing() const {
  return m_facing;
}

CGGameObject_C_Type_MapObjTransport::CGGameObject_C_Type_MapObjTransport(CGGameObject_C *owner)
    : CGGameObject_C_Type_MapObj(owner), m_position(), m_facing(0.0f) {
  MovementAddTransport(m_owner);

  FLOAT shipSpeed = m_owner->GetPropertyValue(CGameObjectDef::GetPropNum(15, 37));
  int pathId[2] = {m_owner->GetPropertyValue(CGameObjectDef::GetPropNum(15, 35)), m_owner->GetPropertyValue(CGameObjectDef::GetPropNum(15, 36))};

  UINT                             count = g_taxiPathNodeDB.GetNumRecords();
  TSStackArray<NTempest::C3Vector> points(_alloca(count * sizeof(NTempest::C3Vector)), count, 0);
  for (UINT i = 0; i < 2; ++i) {
    for (UINT j = 0; j < count; ++j) {
      const TaxiPathNodeRec *node = g_taxiPathNodeDB.GetRecordByIndex(j);
      if (node && node->m_PathID == pathId[i]) {
        do {
          points.New(NTempest::C3Vector(node->m_LocX, node->m_LocY, node->m_LocZ));
          node = g_taxiPathNodeDB.GetRecordByIndex(++j);
        } while (node && node->m_PathID == pathId[i]);
        break;
      }
    }
    m_path[i].SetPoints(points.Ptr(), points.Count());
    m_tripTime[i] = NTempest::CMath::fint_n(m_path[i].Length() * (1.0f / shipSpeed) * 1000.0f);
    points.SetCount(0);
  }

  UpdateMovement(OsGetAsyncTimeMs(), 0.0f);
}

void CGGameObject_C_Type_MapObjTransport::Reenable() {
  m_objectId = m_owner->CreateWorldObject(m_owner->GetGUID());
  MovementAddTransport(m_owner);
}

void CGGameObject_C_Type_MapObjTransport::Disable(int shutdown) {
  DWORD eventTime = OsGetAsyncTimeMs();
  for (CMovementData *passenger = m_passengers.Head(), *passengernext_node;
       (int)passenger > 0 ? (passengernext_node = m_passengers.RawNext(passenger), 1) : 0; passenger = passengernext_node) {
    CMovement *movement = static_cast<CMovement *>(passenger);
    if (passenger->GetGUID() == ClntObjMgrGetActivePlayer()) {
      movement->OnFallLocal(eventTime);
    } else {
      movement->OnFall(eventTime);
    }
    passenger->ForceSetTransport(0);
  }

  MovementRemoveTransport(m_owner);
  if (!shutdown && m_objectId) {
    CWorld::ObjectDelete(m_objectId);
  }
}

void CGGameObject_C_Type_MapObjTransport::UpdateMovement(DWORD eventTime, float) {
  NTempest::C34Matrix matrix;
  UINT                time = (eventTime + m_owner->m_serverTimeOffset) % (m_tripTime[0] + m_tripTime[1] + 40000);

  if (time < 20000) {
    m_path[0].Frame(0.0f, matrix, NTempest::C3Spline::EVAL_ARCLENGTH);
  } else if (time < m_tripTime[0] + 20000) {
    float t = static_cast<float>(time - 20000) / m_tripTime[0];
    m_path[0].Frame(t, matrix, NTempest::C3Spline::EVAL_ARCLENGTH);
  } else if (time < m_tripTime[0] + 40000) {
    m_path[1].Frame(0.0f, matrix, NTempest::C3Spline::EVAL_ARCLENGTH);
  } else {
    float t = static_cast<float>(time - m_tripTime[0] - 40000) / m_tripTime[1];
    m_path[1].Frame(t, matrix, NTempest::C3Spline::EVAL_ARCLENGTH);
  }

  NTempest::C2Vector direction(-matrix.a0, -matrix.a1);
  direction.SafeNormalize();
  m_position = *matrix.Row3AsVec3();
  m_facing = NTempest::CMath::atan2_(direction.y, direction.x);

  if (m_objectId) {
    CWorld::ObjectUpdate(m_objectId, m_position, m_facing, 0);
  }

  ITERATELIST(CMovementData, m_passengers, passenger) {
    CGObject_C *unit = ClntObjMgrObjectPtr(passenger->GetGUID(), __FILE__, __LINE__);
    FATALASSERT(unit);
    unit->UpdateWorldObject();
  }
}

void CGGameObject_C_Type_MapObjTransport::AddPassenger(CMovementData *passenger) {
  FATALASSERT(passenger);
  m_passengers.LinkNode(passenger, LIST_TAIL, 0);
  CMovement::LogWrite(
      "0x%016I64X: Attaching to transport (0x%016I64X) at "
      "position(%g,%g).  Synced time is (0x%08X)",
      passenger->m_guid, m_owner->GetGUID(), m_position.x, m_position.y, m_position.z, OsGetAsyncTimeMs() + m_owner->m_serverTimeOffset
  );
}

BOOL CGGameObject_C_Type_MapObjTransport::IsPointInside(const NTempest::C3Vector &point) const {
  return CWorld::ObjectTestConvexVolume(m_objectId, point);
}

CGGameObject_C_Type_Chair::CGGameObject_C_Type_Chair(CGGameObject_C *owner) : CGGameObject_C_TypeBase(owner) {
}

bool CGGameObject_C_Type_Chair::CanUseNow(GAME_ERROR_TYPE *reason) const {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player || player->GetHealth() <= 0) {
    if (reason) {
      *reason = GERR_PLAYER_DEAD;
    }
    return 0;
  }

  for (UINT i = GetNumSlots(); i--;) {
    if (player->LinearDistanceSquared(m_slotPositions[i]) < MAX_SITCHAIRUSE_DISTANCE_SQUARED) {
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
  FATALASSERT(GetNumSlots() <= MAX_CHAIR_SLOTS);

  GenerateChairPoints(NTempest::C44Matrix(m_owner->GetMatrix()), GetNumSlots(), m_slotPositions);
}

bool CGGameObject_C_Type_SpellFocus::CanHighlight() const {
  return 1;
}

bool CGGameObject_C_Type_SpellFocus::CanUse() const {
  return 0;
}

CGGameObject_C_Type_SpellFocus::CGGameObject_C_Type_SpellFocus(CGGameObject_C *owner) : CGGameObject_C_TypeAnimated(owner) {
}

CGGameObject_C_Type_Text::CGGameObject_C_Type_Text(CGGameObject_C *owner) : CGGameObject_C_TypeAnimated(owner) {
  m_interactDistance = MAX_SHOP_DISTANCE;
}

bool CGGameObject_C_Type_Text::Use(const DWORDLONG &activator) {
  FATALASSERT(m_owner);
  CGItemText::SetItem(m_owner->GetGUID(), 0);
  return 1;
}

int CGGameObject_C::GetPageTextID(void (*)(int, const DWORDLONG &, LPVOID, bool)) const {
  return GetPropertyValue(CGameObjectDef::GetPropNum(GetType(), 15));
}

int CGGameObject_C::GetPageTextLanguage() const {
  return GetPropertyValue(CGameObjectDef::GetPropNum(GetType(), 16));
}

int CGGameObject_C::GetPageTextMaterial() const {
  CGGameObject_C *object = const_cast<CGGameObject_C *>(this);
  int             prop = CGameObjectDef::GetPropNum(object->GetType(), 17);
  return object->GetPropertyValue(prop);
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

CGGameObject_C_Type_Goober::CGGameObject_C_Type_Goober(CGGameObject_C *owner) : CGGameObject_C_TypeAnimated(owner) {
}

NTempest::C3Vector CGGameObject_C_Type_Transport::GetPosition() const {
  return m_position;
}

NTempest::C3Vector CGGameObject_C_Type_Transport::GetCurrentMoveVector() const {
  return m_currDirection * m_currSpeed;
}

bool CGGameObject_C_Type_Transport::CanUse() const {
  return 0;
}

CGGameObject_C_Type_Transport::CGGameObject_C_Type_Transport(CGGameObject_C *owner)
    : CGGameObject_C_TypeAnimated(owner), m_currKey(0), m_position(), m_currSpeed(0.0f), m_currDirection(0.0f, 0.0f, 1.0f) {
  MovementAddTransport(m_owner);

  int firstKey = FindAnimData(owner);
  if (firstKey == -1) {
    FATALERROR(("No key frames found for transport (entry ID: %d)", owner->GetEntryID()));
    return;
  }

  m_keys = g_transportAnimationDB.GetRecordByIndex(firstKey);
  m_numKeys = 1;
  int numRecords = g_transportAnimationDB.GetNumRecords();
  for (int i = firstKey + 1; i < numRecords; ++i) {
    const TransportAnimationRec *key = g_transportAnimationDB.GetRecordByIndex(i);
    if (key->m_TransportID != owner->GetEntryID()) {
      break;
    }
    ++m_numKeys;
  }

  m_position = m_owner->GetObjectPosition() + GetMovement(OsGetAsyncTimeMs());
}

void CGGameObject_C_Type_Transport::Reenable() {
  MovementAddTransport(m_owner);
}

void CGGameObject_C_Type_Transport::ModelJustLoaded() {
  CGGameObject_C_TypeAnimated::ModelJustLoaded();

  NTempest::CAaBox bounds(0.0f);
  ModelGetExtents(m_owner->GetObjectModel(), &bounds);
  m_interior.SetCount(6);
  m_interior[0].Set(NTempest::C3Vector(1.0f, 0.0f, 0.0f), NTempest::C3Vector(bounds.t.x, 0.0f, 0.0f));
  m_interior[1].Set(NTempest::C3Vector(0.0f, 1.0f, 0.0f), NTempest::C3Vector(0.0f, bounds.t.y, 0.0f));
  m_interior[2].Set(NTempest::C3Vector(0.0f, 0.0f, 1.0f), NTempest::C3Vector(0.0f, 0.0f, bounds.t.z));
  m_interior[3].Set(NTempest::C3Vector(-1.0f, 0.0f, 0.0f), NTempest::C3Vector(bounds.b.x, 0.0f, 0.0f));
  m_interior[4].Set(NTempest::C3Vector(0.0f, -1.0f, 0.0f), NTempest::C3Vector(0.0f, bounds.b.y, 0.0f));
  m_interior[5].Set(NTempest::C3Vector(0.0f, 0.0f, -1.0f), NTempest::C3Vector(0.0f, 0.0f, bounds.b.z));
}

void CGGameObject_C_Type_Transport::Disable(int) {
  DWORD eventTime = OsGetAsyncTimeMs();
  for (CMovementData *passenger = m_passengers.Head(), *passengernext_node;
       (int)passenger > 0 ? (passengernext_node = m_passengers.RawNext(passenger), 1) : 0; passenger = passengernext_node) {
    CMovement *movement = static_cast<CMovement *>(passenger);
    if (passenger->GetGUID() == ClntObjMgrGetActivePlayer()) {
      movement->OnFallLocal(eventTime);
    } else {
      movement->OnFall(eventTime);
    }
    passenger->ForceSetTransport(0);
  }
  MovementRemoveTransport(m_owner);
}

int CGGameObject_C_Type_Transport::FindAnimData(CGGameObject_C *owner) {
  for (int i = 0; i < g_transportAnimationDB.GetNumRecords(); ++i) {
    const TransportAnimationRec *key = g_transportAnimationDB.GetRecordByIndex(i);
    if (key->m_TransportID == owner->GetEntryID()) {
      return i;
    }
    if (key->m_TransportID > owner->GetEntryID()) {
      break;
    }
  }
  return -1;
}

void CGGameObject_C_Type_Transport::UpdateMovement(DWORD eventTime, float elapsed) {
  NTempest::C3Vector move = m_position;
  m_position = m_owner->GetObjectPosition() + GetMovement(eventTime);
  move = m_position - move;

  m_owner->UpdateMatrix();
  m_owner->UpdateWorldObject();

  float distance = move.Mag();
  m_currSpeed = distance / elapsed;
  if (NTempest::CMath::fnotequal_(distance, 0.0f)) {
    m_currDirection = move / distance;
  }

  ITERATELIST(CMovementData, m_passengers, passenger) {
    CGObject_C *unit = ClntObjMgrObjectPtr(passenger->GetGUID(), __FILE__, __LINE__);
    FATALASSERT(unit);
    unit->UpdateWorldObject();
  }
}

UINT CGGameObject_C_Type_Transport::NextKeyID() const {
  return m_currKey + 1 == m_numKeys ? 0 : m_currKey + 1;
}

NTempest::C3Vector CGGameObject_C_Type_Transport::GetMovement(UINT eventTime) {
  FATALASSERT(m_numKeys > 1);

  UINT                         time = (eventTime + m_owner->m_serverTimeOffset) % m_keys[m_numKeys - 1].m_TimeIndex;
  UINT                         nextKeyID = NextKeyID();
  const TransportAnimationRec *key = &m_keys[m_currKey];
  const TransportAnimationRec *nextKey = &m_keys[nextKeyID];
  while (time < static_cast<UINT>(key->m_TimeIndex) || time >= static_cast<UINT>(nextKey->m_TimeIndex)) {
    m_currKey = nextKeyID;
    nextKeyID = NextKeyID();
    key = &m_keys[m_currKey];
    nextKey = &m_keys[nextKeyID];
  }

  float                   ratio = static_cast<float>(time - key->m_TimeIndex) / static_cast<float>(nextKey->m_TimeIndex - key->m_TimeIndex);
  NTempest::C4Quaternion *rotation = &m_owner->m_gameObj->m_rotation;
  return NTempest::C33Matrix(*rotation) * NTempest::C3Vector(
                                              key->m_PosX * (1.0f - ratio) + nextKey->m_PosX * ratio,
                                              key->m_PosY * (1.0f - ratio) + nextKey->m_PosY * ratio,
                                              key->m_PosZ * (1.0f - ratio) + nextKey->m_PosZ * ratio
                                          );
}

void CGGameObject_C_Type_Transport::AddPassenger(CMovementData *passenger) {
  FATALASSERT(passenger);
  passenger->transportLink.Unlink();
  m_passengers.LinkNode(passenger, LIST_TAIL, 0);
}

BOOL CGGameObject_C_Type_Transport::IsPointInside(const NTempest::C3Vector &point) const {
  UINT numPlanes = m_interior.Count();
  for (UINT i = 0; i < numPlanes; ++i) {
    if (m_interior[i].DistSigned(point) > 0.0f) {
      return 0;
    }
  }
  return 1;
}

CGGameObject_C_Type_Camera::CGGameObject_C_Type_Camera(CGGameObject_C *owner) : CGGameObject_C_TypeBase(owner) {
}

bool CGGameObject_C_Type_DuelArbiter::CanHighlight() const {
  return 1;
}

bool CGGameObject_C_Type_DuelArbiter::CanUse() const {
  return 0;
}

CGGameObject_C_Type_DuelArbiter::CGGameObject_C_Type_DuelArbiter(CGGameObject_C *owner) : CGGameObject_C_TypeBase(owner) {
}

CGGameObject_C_Type_FishingNode::CGGameObject_C_Type_FishingNode(CGGameObject_C *owner) : CGGameObject_C_TypeAnimated(owner) {
  m_interactDistance = MAX_OBJ_INTEREST_RADIUS;
}

bool CGGameObject_C_Type_FishingNode::CanUse() const {
  DWORDLONG activePlayer = ClntObjMgrGetActivePlayer();
  CGUnit_C *player = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(activePlayer, __FILE__, __LINE__));
  if (!player || player->GetChannelObject() != m_owner->GetGUID()) {
    return 0;
  }
  return CGGameObject_C_TypeBase::CanUse();
}

CGGameObject_C_Type_Ritual::CGGameObject_C_Type_Ritual(CGGameObject_C *owner) : CGGameObject_C_TypeAnimated(owner) {
}

bool CGGameObject_C_Type_Ritual::CanUseNow(GAME_ERROR_TYPE *reason) const {
  return 1;
}

static void GameObjectStatsCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted) {
  CGGameObject_C *object = static_cast<CGGameObject_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (object) {
    const GameObjectStats_C *stats = g_gameObjectDBCache.GetRecord(id, 0, 0, 0);
    if (stats) {
      object->LoadBaseObject(stats);
    }
  }
}

static void AnimEventCallback(LPCSTR eventName, const NTempest::C3Vector &position, LPVOID param) {
  FATALASSERT(param);
  CGGameObject_C_TypeBase *obj = static_cast<CGGameObject_C *>(param)->m_baseObj;
  FATALASSERT(obj);
  obj->HandleAnimEvent(eventName, position);
}

static BOOL AnimFinishedCallback(LPVOID param) {
  FATALASSERT(param);
  CGGameObject_C_TypeBase *obj = static_cast<CGGameObject_C *>(param)->m_baseObj;
  FATALASSERT(obj);
  obj->HandleAnimFinished();
  return 1;
}

static BOOL OnUpdateState(DWORDLONG guid, UINT offset, UINT bytes, LPCVOID prevValue, LPVOID param) {
  CGGameObject_C *gameObj = static_cast<CGGameObject_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  FATALASSERT(gameObj);
  FATALASSERT(gameObj->m_baseObj);
  gameObj->m_baseObj->UpdateState(*static_cast<const int *>(prevValue), gameObj->GetState());
  return 1;
}

void CGGameObject_C::SetStorage(DWORD *storage) {
  CGObject_C::SetStorage(storage);
  CGGameObject::SetStorage(storage + CGObject::TotalFields());
}

void CGGameObject_C::UpdateMatrix() {
  m_matrix.Identity();
  m_matrix.Translate(GetPosition());
  m_matrix.Rotate(m_gameObj->m_rotation);
  m_matrix.Scale(GetScale());
  NTempest::CAaBox localExtents;
  if (GetObjectModel()) {
    ModelGetCollisionExtents(GetObjectModel(), &localExtents);
  }
  CWorldMath::TransformAABox(m_matrix, localExtents, m_collideExtents);
}

CGGameObject_C::CGGameObject_C(DWORD *storage, DWORD eventTime, CClientObjCreate *init)
    : CGObject_C(storage, eventTime, init),
      CGGameObject(storage + CGObject::TotalFields()),
      m_baseObj(0),
      m_stats(0),
      m_isSolid(0) {
  ClntObjMgrHideObject(GetGUID());
  m_gameObj->m_position = init->move.status.worldPosition;
  m_gameObj->m_facing = init->move.status.worldFacing;
  m_serverTimeOffset = init->move.timeFallen - eventTime;
}

CGGameObject_C::~CGGameObject_C() {
  if (m_baseObj) {
    DEL(m_baseObj);
  }
}

BOOL CGGameObject_C::UpdateModelLoadStatus() {
  if (!CGObject_C::UpdateModelLoadStatus()) {
    return 0;
  }
  NTempest::CAaBox localExtents;
  ModelGetCollisionExtents(GetObjectModel(), &localExtents);
  CWorldMath::TransformAABox(m_matrix, localExtents, m_collideExtents);
  m_baseObj->ModelJustLoaded();
  return 1;
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

void CGGameObject_C::PostInit(const CClientObjCreate &init) {
  CGObject_C::PostInit(init);
  const GameObjectStats_C *stats = g_gameObjectDBCache.GetRecord(GetEntryID(), GetGUID(), GameObjectStatsCallback, 0);
  if (stats) {
    LoadBaseObject(stats);
  }
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
    const GameObjectStats_C *stats = g_gameObjectDBCache.GetRecord(GetEntryID(), GetGUID(), GameObjectStatsCallback, 0);
    if (stats) {
      LoadBaseObject(stats);
    }
  }
}

void CGGameObject_C::SetMirrorHandlers() {
  ClntObjMgrSetObjMirrorHandler(
      GetGUID(), OffsetOf(ID_GAMEOBJECT) + offsetof(CGGameObjectData, m_state), sizeof(((CGGameObjectData *)0)->m_state), OnUpdateState, 0, HANDLER_PRIORITY_NORMAL
  );
}

void CGGameObject_C::UnsetMirrorHandlers() {
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_GAMEOBJECT) + offsetof(CGGameObjectData, m_state), OnUpdateState, 0);
}

BOOL CGGameObject_C::SetBlock(UINT, DWORD) {
  FATALASSERT(0);
  return 1;
}

void CGGameObject_C::SetData(LPCVOID data, UINT bytes) {
  FATALASSERT(bytes <= sizeof(*m_gameObj));
  memcpy(m_gameObj, data, bytes);
}

UINT CGGameObject_C::OffsetOf(OBJECT_TYPE_ID type) {
  switch (type) {
    case ID_OBJECT:
      return 0;
    case ID_GAMEOBJECT:
      return CGObject::TotalFields() * sizeof(DWORD);
  }
  FATALASSERT(0);
  return -1;
}

void CGGameObject_C::LoadBaseObject(const GameObjectStats *stats) {
  VALIDATEBEGIN;
  VALIDATE(stats);
  VALIDATEENDVOID;
  SetMirrorHandlers();
  m_stats = stats;

  switch (stats->m_typeID) {
    case 0:
      m_baseObj = NEW(CGGameObject_C_Type_Door)(this);
      break;
    case 1:
      m_baseObj = NEW(CGGameObject_C_Type_Button)(this);
      break;
    case 3:
      m_baseObj = NEW(CGGameObject_C_Type_Chest)(this);
      break;
    case 6:
      m_baseObj = NEW(CGGameObject_C_Type_Trap)(this);
      break;
    case 12:
      m_baseObj = NEW(CGGameObject_C_Type_AreaDamage)(this);
      break;
    case 2:
      m_baseObj = NEW(CGGameObject_C_Type_QuestGiver)(this);
      break;
    case 4:
      m_baseObj = NEW(CGGameObject_C_Type_Binder)(this);
      break;
    case 5:
      m_baseObj = NEW(CGGameObject_C_Type_Generic)(this);
      break;
    case 7:
      m_baseObj = NEW(CGGameObject_C_Type_Chair)(this);
      break;
    case 8:
      m_baseObj = NEW(CGGameObject_C_Type_SpellFocus)(this);
      break;
    case 9:
      m_baseObj = NEW(CGGameObject_C_Type_Text)(this);
      break;
    case 10:
      m_baseObj = NEW(CGGameObject_C_Type_Goober)(this);
      break;
    case 11:
      m_baseObj = NEW(CGGameObject_C_Type_Transport)(this);
      break;
    case 13:
      m_baseObj = NEW(CGGameObject_C_Type_Camera)(this);
      break;
    case 14:
      m_baseObj = NEW(CGGameObject_C_Type_MapObj)(this);
      break;
    case 15:
      m_baseObj = NEW(CGGameObject_C_Type_MapObjTransport)(this);
      break;
    case 16:
      m_baseObj = NEW(CGGameObject_C_Type_DuelArbiter)(this);
      break;
    case 17:
      m_baseObj = NEW(CGGameObject_C_Type_FishingNode)(this);
      break;
    case 18:
      m_baseObj = NEW(CGGameObject_C_Type_Ritual)(this);
      break;
    default:
      m_baseObj = &s_nullBaseObj;
      SysMsgPrintf(SYSMSG_WARNING, 2, "BADBASEGAMEOBJECT|%d", stats->m_typeID);
      break;
  }

  m_baseObj->PostInit();
}

UNIT_REACTION CGGameObject_C::ObjectReaction(const CGUnit_C *unit) const {
  if (m_gameObj->m_factionTemplate) {
    return CGUnit_C::UnitReaction(m_gameObj->m_factionTemplate, unit, -1);
  }
  return UNIT_REACTION_NEUTRAL;
}

UINT CGGameObject_C::CreateWorldObject(DWORDLONG guid) {
  NTempest::C3Vector position = m_gameObj->m_position;
  return CWorld::ObjectCreate(GetModelFileNameInternal(), position, GetFacing(), 0, 0, guid);
}

LPCSTR CGGameObject_C::GetModelFileNameInternal() const {
  int displayID = m_gameObj->m_displayID;
  if (!displayID) {
    return 0;
  }

  const GameObjectDisplayInfoRec *displayInfo = g_gameObjectDisplayInfoDB.GetRecord(displayID);
  if (displayInfo) {
    return displayInfo->m_modelName;
  }

  SysMsgPrintf(SYSMSG_FATAL, 2, "NOOBJECTFILENAME|%d|%d|Game", displayID, m_obj->m_entryID);
  return NONAME;
}

LPCSTR CGGameObject_C::GetModelFileName() const {
  LPCSTR modelName = GetModelFileNameInternal();
  if (!modelName) {
    return 0;
  }
  LPCSTR extension = SStrChrR(modelName, '.');
  if (extension && !SStrCmpI(extension, ".wmo", 0x7FFFFFFF)) {
    return 0;
  }
  return modelName;
}

LPCSTR CGGameObject_C::GetName() const {
  return m_stats ? m_stats->m_name[0] : "";
}

LPCSTR CGGameObject_C::GetTypeName() const {
  return m_stats ? CGameObjectDef::NameFromTypeId(m_stats->m_typeID) : "UNKNOWN";
}

LPCSTR CGGameObject_C::GetDebugStatus() const {
  FATALASSERT(m_baseObj);
  return m_baseObj->DebugStatus();
}

int CGGameObject_C::GetType() const {
  return m_stats ? m_stats->m_typeID : -1;
}

UINT CGGameObject_C::GetPropertyValue(UINT index) const {
  return m_stats && index < 10 ? m_stats->m_propValue[index] : 0;
}

const LockRec *CGGameObject_C::GetLockRec() const {
  int lockID = GetPropertyValue(CGameObjectDef::GetPropNum(GetType(), 4));
  if (!lockID) {
    return 0;
  }
  return g_lockDB.GetRecord(lockID);
}

bool CGGameObject_C::IsValidOpenAction(int action) const {
  if ((action != 4 && GetState() == 2) || (action == 4 && GetState() != 2)) {
    return false;
  }
  if ((action == 0 || action == 1 || action == 3) && GetState() != 1) {
    return false;
  }
  if (action == 0) {
    if (GetLocked()) {
      return false;
    }
  } else if (action == 1) {
    if (!GetLocked()) {
      return false;
    }
  } else if (action == 2 && GetState() != 0) {
    return false;
  }
  return true;
}

bool CGGameObject_C::IsValidTargetForSpell(const DWORDLONG &caster, int spellID) const {
  const LockRec *lock = GetLockRec();
  if (!lock) {
    return 0;
  }

  const SpellRec *spell = g_spellDB.GetRecord(spellID);
  if (!spell) {
    return 0;
  }

  const int *lockType = lock->m_Type;
  const int *lockIndex = lock->m_Index;
  const int *lockAction = lock->m_Action;
  for (int effectIndex = 0; effectIndex < sizeof(spell->m_effect) / sizeof(spell->m_effect[0]); ++effectIndex) {
    if (spell->m_effect[effectIndex] == 33) {
      for (int i = 0; i < 4; ++i) {
        if (lockType[i] == 2 && spell->m_effectMiscValue[effectIndex] == lockIndex[i] && IsValidOpenAction(lockAction[i])) {
          return 1;
        }
      }
    }
    if (spell->m_effect[effectIndex] == 59) {
      CGObject_C *item = ClntObjMgrObjectPtr(caster, __FILE__, __LINE__);
      if (item && item->IsA(ID_ITEM)) {
        for (int i = 0; i < 4; ++i) {
          if (lockType[i] == 1 && item->GetEntryID() == lockIndex[i] && IsValidOpenAction(lockAction[i])) {
            return 1;
          }
        }
      }
    }
  }
  return 0;
}

bool CGGameObject_C::IsLocked(int *spellID, int *spellSkill, int *lockSkill, CGItem_C **itemPtr, int *openIndex) const {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return false;
  }

  const LockRec *lock = GetLockRec();
  if (!lock) {
    return false;
  }

  bool locked = false;
  for (int i = 0; i < sizeof(lock->m_Type) / sizeof(lock->m_Type[0]); ++i) {
    int type = lock->m_Type[i];
    if (!type) {
      continue;
    }

    if (type == 2) {
      locked = true;
      if (!IsValidOpenAction(lock->m_Action[i])) {
        continue;
      }

      for (UINT j = 0; j < CGSpellBook::m_unlockSpells.Count(); ++j) {
        const SpellRec *srec = g_spellDB.GetRecord(CGSpellBook::m_unlockSpells[j]);
        FATALASSERT(srec);
        for (int effect = 0; effect < sizeof(srec->m_effect) / sizeof(srec->m_effect[0]); ++effect) {
          if (srec->m_effect[effect] != 33 || srec->m_effectMiscValue[effect] != lock->m_Index[i]) {
            continue;
          }

          int min;
          int max;
          Spell_C_GetMinMaxPoints(srec, effect, &min, &max, 0, 0);
          if (spellID) {
            *spellID = CGSpellBook::m_unlockSpells[j];
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
    } else if (type == 1) {
      locked = true;
      if (!IsValidOpenAction(lock->m_Action[i])) {
        continue;
      }

      CGItem_C *item = player->Inventory()->FindItemOfType(lock->m_Index[i], 0);
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

BOOL CGGameObject_C::CanHighlight() const {
  FATALASSERT(m_baseObj);
  return m_baseObj->CanHighlight();
}

BOOL CGGameObject_C::FloatingTooltip() const {
  return GetPropertyValue(CGameObjectDef::GetPropNum(GetType(), 19)) != 0;
}

void CGGameObject_C::OnRightClick() {
  FATALASSERT(m_baseObj);
  if (m_baseObj->CanUse()) {
    GAME_ERROR_TYPE reason;
    if (m_baseObj->CanUseNow(&reason)) {
      m_baseObj->Use(ClntObjMgrGetActivePlayer());
    } else {
      CGGameUI::DisplayError(reason);
    }
  }
}

bool CGGameObject_C::CanChangeCursor() const {
  FATALASSERT(m_baseObj);
  return m_baseObj->CanChangeCursor();
}

bool CGGameObject_C::CanUse() const {
  FATALASSERT(m_baseObj);
  return m_baseObj->CanUse();
}

bool CGGameObject_C::CanUseNow() const {
  FATALASSERT(m_baseObj && m_baseObj->CanUse());
  return m_baseObj->CanUseNow(0);
}

void CGGameObject_C::ObjectPostAnimate(const NTempest::C34Matrix &, const NTempest::C3Vector &, const NTempest::C3Vector &) {
  if (GetObjectModel()) {
    ModelShowCollision(GetObjectModel(), CWorld::enables & CWorld::Enable_Collision);
  }
}

void CGGameObject_C::Initialize() {
  ClientServices_SetMessageHandler(SMSG_GAMEOBJECT_PAGETEXT, PageTextHandler, 0);
  ClientServices_SetMessageHandler(SMSG_GAMEOBJECT_CUSTOM_ANIM, CustomAnimHandler, 0);
}

void CGGameObject_C::Shutdown() {
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

void CGGameObject_C::ActivateCustomAnim(UINT anim) {
  if (m_baseObj) {
    m_baseObj->ActivateCustomAnim(anim);
  }
}

BOOL CGGameObject_C::IsTransport() const {
  FATALASSERT(m_stats);
  UINT type = *reinterpret_cast<const UINT *>(m_stats);
  return type == 11 || type == 15;
}

bool CGGameObject_C_TypeBase::CanHighlight() const {
  return CanUse();
}

bool CGGameObject_C_TypeBase::CanChangeCursor() const {
  return CanUse();
}

void CGGameObject_C_TypeBase::HandleAnimFinished() {
}

void CGGameObject_C_TypeBase::ActivateCustomAnim(UINT) {
}

void CGGameObject_C_TypeBase::AddPassenger(CMovementData *) {
}

NTempest::C3Vector CGGameObject_C_TypeBase::GetCurrentMoveVector() const {
  return NTempest::C3Vector();
}

BOOL CGGameObject_C_TypeBase::IsPointInside(const NTempest::C3Vector &) const {
  return 0;
}

void CGGameObject_C_TypeBase::Reenable() {
}

void CGGameObject_C_TypeBase::Disable(int) {
}

void CGGameObject_C_TypeBase::PostReenable() {
}

void CGGameObject_C_TypeBase::UpdateMovement(DWORD, float) {
}

void CGGameObject_C_TypeBase::ModelJustLoaded() {
}

void CGGameObject_C_TypeBase::StartInteraction() {
}

void CGGameObject_C_TypeBase::CloseInteraction() {
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

BOOL CGGameObject_C::IsPointInside(const NTempest::C3Vector &point) const {
  FATALASSERT(m_baseObj);
  return m_baseObj->IsPointInside(point);
}

BOOL CGGameObject_C::IsSolidSelectable() const {
  return m_isSolid || CanHighlight();
}

BOOL CGGameObject_C::IsSolidCollidable() const {
  return m_isSolid;
}

LPCSTR CGGameObject_C::GetObjectName() const {
  return GetName();
}

NTempest::C3Vector CGGameObject_C::GetCurrentMoveVector() const {
  FATALASSERT(m_baseObj);
  return m_baseObj->GetCurrentMoveVector();
}

NTempest::C34Matrix CGGameObject_C::GetMatrix() const {
  return m_matrix;
}

void CGGameObject_C::GetWorldMatrix(NTempest::C34Matrix *worldMatrix) const {
  *worldMatrix = GetMatrix();
}
