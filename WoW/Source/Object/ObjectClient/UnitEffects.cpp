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
#include <Ftol.h>

#include "Unit_C.h"
#include "Player_C.h"
#include "IUnitEffects.h"

#include "Client.h"
#include "Component/CharacterCustomization.h"
#include "Component/Component.h"
#include "Console/ConsoleVar.h"
#include "DB/DBClient/AutoCode/ItemDisplayInfoRec.h"
#include "DB/DBClient/AutoCode/SpellRec.h"
#include "DB/DBClient/AutoCode/SpellEffectCameraShakesRec.h"
#include "DB/DBClient/AutoCode/SpellVisualRec.h"
#include "DB/DBClient/AutoCode/SpellVisualKitRec.h"
#include "DB/DBClient/AutoCode/ChrRacesRec.h"
#include "DB/DBClient/AutoCode/SpellVisualEffectNameRec.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "SoundInterface/SoundInterface.h"
#include "Ui/GameUI.h"
#include "UIUtil/Camera.h"
#include "UIUtil/InputControl.h"
#include "Ui/WorldFrame.h"
#include "WorldClient/World.h"
#include <Os/OsTime.h>

#include <Base/Handle.h>
#include <Base/CDataAllocator.h>
#include <Base/Status.h>
#include <Model/IModel.h>
#include <Os/W32/OsSound.h>
#include <Services/SysMessage.h>
#include <Tempest/c44matrix.h>
#include <Tempest/c3segment.h>
#include <Tempest/c4plane.h>
#include <stpl.h>
#include <storm.h>

using NTempest::CMath;

static BOOL        DeathHoldEventTimerHandler(LPCVOID packetData, LPVOID param);
void        SpellVisualsPlayCameraShakeID(UINT shakeID, const NTempest::C3Vector &position);
static HMODEL      InitializeModel(LPCSTR fileName, void (*callback)(LPCSTR, const NTempest::C3Vector &, LPVOID), LPVOID param);
static void DecorateEffectFilename(LPCSTR fileName, int raceSexSpecific, const CGObject_C *object, char *buffer, UINT size);
static void SpellUnitAnimEventCallback(LPCSTR eventName, const NTempest::C3Vector &position, LPVOID param);
void        SpellCameraShakeCallback(LPCSTR eventName, const NTempest::C3Vector &position);
void        SpellSoundEffectCallback(LPCSTR eventName, const NTempest::C3Vector &position);
void        UnitCombatLogSpellMissed(UINT missReason, UINT spellID, DWORDLONG caster, DWORDLONG victim);
static BOOL OneShotEndHandler(LPVOID param);
static void SpellAreaAnimEventCallback(LPCSTR eventName, const NTempest::C3Vector &position, LPVOID param);
static BOOL PurgeTimerHandler(LPCVOID timerData, LPVOID userData);
void        UnitEffectOneShot(
    const SpellVisualEffectNameRec *effectRec,
    const NTempest::C3Vector       &location,
    const TSStackArray<DWORDLONG>  *objects,
    float                           facing,
    float                           scale
);
static void PreloadModel(int effectID, CStatus *status);
static void PreloadModelsByKit(int record, CStatus *status);

class PERSISTENTUNITEFFECT : public CHandleObject {
 public:
  PERSISTENTUNITEFFECT();
  PERSISTENTUNITEFFECT(const PERSISTENTUNITEFFECT &);
  virtual ~PERSISTENTUNITEFFECT();

  void Clear();

  HMODEL             effectModel;
  HMODEL             objectModel;
  GEOCOMPONENTLINKS  linkPoint;
  NTempest::C3Vector position;
  LINKDECLEX(PERSISTENTUNITEFFECT, m_listLink);
};

static LPCSTR s_objNames[1] = {"$DTH"};

GEOCOMPONENTLINKS g_attachmentPoints[12] = {
    ATTACH_UNITEFFECT_BASE, ATTACH_UNITEFFECT_HEAD, ATTACH_UNITEFFECT_SPELLLEFTHAND, ATTACH_UNITEFFECT_SPELLRIGHTHAND, ATTACH_NONE,
    ATTACH_BREATH,          ATTACH_TORSOSPELL,      ATTACH_UNITEFFECT_SPECIAL1,      ATTACH_UNITEFFECT_SPECIAL2,       ATTACH_UNITEFFECT_SPECIAL3,
    ATTACH_TORSOBLOODBACK,  ATTACH_TORSOBLOODFRONT
};

static HMODEL CreateModel(LPCSTR fileName, CStatus *status) {
  CModelCreate createData;
  createData.cameraNames = 0;
  createData.numCameras = 0;
  createData.flags = 0x2006;
  createData.boneNames = s_objNames;
  createData.numBones = 1;
  createData.sequenceNames = &g_animationNames[FIRST_EFFECTANIMATION];
  createData.numSequences = NUM_EFFECTANIMATIONS;
  return ModelCreate(fileName, &createData, status);
}

static void PreloadModel(int effectID, CStatus *status) {
  const SpellVisualEffectNameRec *effect = g_spellVisualEffectNameDB.GetRecord(effectID);
  if (effect) {
    HMODEL model = CreateModel(effect->m_fileName, status);
    if (model) {
      HandleClose(model);
    }
  }
}

static void PreloadModelsByKit(int record, CStatus *status) {
  const SpellVisualKitRec *kit = g_spellVisualKitDB.GetRecord(record);
  if (!kit) {
    return;
  }

  PreloadModel(kit->m_headEffect, status);
  PreloadModel(kit->m_chestEffect, status);
  PreloadModel(kit->m_baseEffect, status);
  PreloadModel(kit->m_leftHandEffect, status);
  PreloadModel(kit->m_rightHandEffect, status);
  PreloadModel(kit->m_breathEffect, status);
  for (UINT i = 0; i < 3; ++i) {
    PreloadModel(kit->m_specialEffect[i], status);
  }
}

static void SpellAnimEventCallback(LPCSTR eventName, const NTempest::C3Vector &position, LPVOID param) {
  UINT event = *reinterpret_cast<const UINT *>(eventName);
  switch (event) {
    case 'DNS$':
    case 'XDNS':
      break;
    case 'KHS$':
      SpellCameraShakeCallback(eventName + 4, position);
      break;
    default:
      SysMsgPrintf(SYSMSG_WARNING, 16, "UNKNOWNANIMEVENT|%s|SpellAnimEventCallback|SpellAnimEventCallback", eventName);
      break;
  }
}

class NODEBASE {
 public:
  NODEBASE() : model(0), flags(0), deathHoldTimer(0) {
  }
  ~NODEBASE();
  virtual void ReleaseDeathHolds() = 0;

  void ClearDeathHoldTimer();
  void SetDeathHoldTimer(UINT duration);
  bool CheckModelLoadStatus();

  LINKDECLEX(NODEBASE, node);
  HMODEL model;
  UINT      flags;
  UINT      deathHoldTimer;
};

class ONESHOTEFFECTNODE : public NODEBASE {
 public:
  ONESHOTEFFECTNODE() : objectModel(0) {
  }
  ~ONESHOTEFFECTNODE();
  virtual void ReleaseDeathHolds();
  void         CheckModelLoadStatus();

  HMODEL objectModel;
  UINT      objectModelAttachmentPoint;
  DWORDLONG objectGUID;
  int       spellID;
  BYTE      isCastEffect;
};

class ONESHOTSTANDALONEEFFECTNODE : public NODEBASE {
 public:
  ONESHOTSTANDALONEEFFECTNODE() : facing(0.0f), scale(1.0f), worldObject(0) {
  }
  ~ONESHOTSTANDALONEEFFECTNODE();
  virtual void ReleaseDeathHolds();
  void         CheckModelLoadStatus();

  NTempest::C3Vector      position;
  TSFixedArray<DWORDLONG> objects;
  float                   facing;
  float                   scale;
  DWORD                   worldObject;
  int                     expireTime;
};

NODEDECL(MISSILENODE) {
  static const float HEIGHT_SCAN_RANGE;
  static const float MIN_HEIGHT;

  MISSILENODE() : model(0), caster(0), target(0), flags(0), sound(0) {
  }
  ~MISSILENODE() {
    if (model) {
      HandleClose(model);
    }
    if (sound) {
      sound->Stop(2.0f);
    }
    if (target) {
      CGObject_C *object = ClntObjMgrObjectPtr(target, __FILE__, __LINE__);
      if (object && object->IsA(TYPE_UNIT)) {
        static_cast<CGUnit_C *>(object)->DDDELLOG(caster, "MISSILENODE immaturely freed", __FILE__, __LINE__);
      }
    }
  }

  HMODEL             model;
  DWORDLONG          caster;
  NTempest::C3Vector startPosition;
  NTempest::C3Vector position;
  NTempest::C3Vector endPosition;
  NTempest::C3Vector normal;
  DWORDLONG          target;
  UINT               startTime;
  UINT               travelTime;
  NTempest::C3Vector facing;
  UINT               spellID;
  UINT               victimEffect;
  UINT               pathType;
  bool               miss;
  MISS_REASON        missReason;
  int                flags;
  void CheckModelLoadStatus();
  Sound             *sound;
};

const float MISSILENODE::HEIGHT_SCAN_RANGE = 5.0f;
const float MISSILENODE::MIN_HEIGHT = 0.3f;

struct UNITONESHOTEFFECTDESC : public TSHashObject<UNITONESHOTEFFECTDESC, CHashKeyGUID> {
  UNITONESHOTEFFECTDESC() {
  }
  UNITONESHOTEFFECTDESC(const UNITONESHOTEFFECTDESC &);
  ~UNITONESHOTEFFECTDESC() {
    m_effects.Clear();
  }

  LISTDECLEX(ONESHOTEFFECTNODE, node, m_effects);
};

static TSHashTable<UNITONESHOTEFFECTDESC, CHashKeyGUID> s_oneShotEffects;
static LISTDECLEX(ONESHOTSTANDALONEEFFECTNODE, node, s_standAloneEffects);
static LISTDECL(MISSILENODE, s_missiles);
static TInstanceAllocator<ONESHOTSTANDALONEEFFECTNODE> s_freeStandaloneEffects(40);
static TInstanceAllocator<MISSILENODE> s_freeMissiles(10);
static CVar                           *s_showEffectsStandalone;
UINT                                   g_specialSpellIDs[43];
static UINT                            s_timer;
static int                             s_timerExpiry;

static void SpellUnitAnimEventCallback(LPCSTR eventName, const NTempest::C3Vector &position, LPVOID param) {
  ONESHOTEFFECTNODE *node = static_cast<ONESHOTEFFECTNODE *>(param);
  UINT               event = *reinterpret_cast<const UINT *>(eventName);

  switch (event) {
    case 'DNS$':
    case 'XDNS': {
      NTempest::C3Vector soundPos = position;
      if (node->objectGUID) {
        CGObject_C *object = ClntObjMgrObjectPtr(node->objectGUID, __FILE__, __LINE__);
        if (object) {
          soundPos += object->GetPosition();
        }
      }
      SpellSoundEffectCallback(eventName + 4, soundPos);
      break;
    }
    case 'KHS$':
      SpellCameraShakeCallback(eventName + 4, position);
      break;
    case 'HTD$':
      if (node) {
        node->ReleaseDeathHolds();
      }
      break;
    case 'TIH$':
      if (node) {
        CGObject_C *object = ClntObjMgrObjectPtr(node->objectGUID, __FILE__, __LINE__);
        if (object && object->IsA(TYPE_UNIT)) {
          static_cast<CGUnit_C *>(object)->SpellEventHit();
        }
      }
      break;
    case 'PPC$':
    case 'HAC$':
    case 'SSC$':
      if (node) {
        CGObject_C *object = ClntObjMgrObjectPtr(node->objectGUID, __FILE__, __LINE__);
        if (object && object->IsA(TYPE_UNIT)) {
          static_cast<CGUnit_C *>(object)->HandleCombatAnimEvent(eventName, event, position);
        }
      }
      break;
    default:
      SysMsgPrintf(SYSMSG_WARNING, 16, "UNKNOWNANIMEVENT|%s|ONESHOTEFFECTNODE|SpellUnitAnimEventCallback", eventName);
      break;
  }
}

void ONESHOTEFFECTNODE::ReleaseDeathHolds() {
  ClearDeathHoldTimer();

  if (!(flags & 3)) {
    flags |= 1;
    CGObject_C *object = ClntObjMgrObjectPtr(objectGUID, __FILE__, __LINE__);
    if (object && object->IsA(TYPE_UNIT)) {
      static_cast<CGUnit_C *>(object)->DDDELLOG(object->GetGUID(), "ONESHOTEFFECTNODE::ReleaseDeathHolds", __FILE__, __LINE__);
    }
  }
}

void ONESHOTSTANDALONEEFFECTNODE::ReleaseDeathHolds() {
  ClearDeathHoldTimer();

  if (!(flags & 3)) {
    flags |= 1;
    for (UINT index = 0; index < objects.Count(); ++index) {
      CGObject_C *object = ClntObjMgrObjectPtr(objects[index], __FILE__, __LINE__);
      if (object && object->IsA(TYPE_UNIT)) {
        static_cast<CGUnit_C *>(object)->DDDELLOG(object->GetGUID(), "ONESHOTSTANDALONEEFFECTNODE::ReleaseDeathHolds", __FILE__, __LINE__);
      }
    }
  }
}

static void SpellAreaAnimEventCallback(LPCSTR eventName, const NTempest::C3Vector &position, LPVOID param) {
  ONESHOTSTANDALONEEFFECTNODE *node = static_cast<ONESHOTSTANDALONEEFFECTNODE *>(param);
  UINT                         event = *reinterpret_cast<const UINT *>(eventName);

  switch (event) {
    case 'DNS$':
    case 'XDNS':
      SpellSoundEffectCallback(eventName + 4, position + node->position);
      break;
    case 'KHS$':
      SpellCameraShakeCallback(eventName + 4, position);
      break;
    case 'HTD$':
      if (node) {
        node->ReleaseDeathHolds();
      }
      break;
    case 'TIH$':
      if (node) {
        for (UINT index = 0; index < node->objects.Count(); ++index) {
          CGObject_C *object = ClntObjMgrObjectPtr(node->objects[index], __FILE__, __LINE__);
          if (object && object->IsA(TYPE_UNIT)) {
            static_cast<CGUnit_C *>(object)->SpellEventHit();
          }
        }
      }
      break;
    default:
      SysMsgPrintf(SYSMSG_WARNING, 16, "UNKNOWNANIMEVENT|%s|ONESHOTSTANDALONEEFFECTNODE|SpellAreaAnimEventCallback", eventName);
      break;
  }
}

static BOOL OneShotEndHandler(LPVOID param) {
  FATALASSERT(param);

  ONESHOTEFFECTNODE *node = static_cast<ONESHOTEFFECTNODE *>(param);
  if (node->isCastEffect) {
    node->ReleaseDeathHolds();
    DEL(node);
  }
  return 0;
}

static HMODEL InitializeModel(LPCSTR fileName, void (*callback)(LPCSTR, const NTempest::C3Vector &, LPVOID), LPVOID param) {
  CStatus status;
  HMODEL  model = CreateModel(fileName, &status);
  if (callback) {
    ModelSetEventCallback(model, callback, param, 0);
  }
  ModelSetSequence(model, 0, 0);
  SysMsgAdd(status, 16);
  return model;
}

static void RenderModel(HMODEL model, const NTempest::C3Vector &position, const NTempest::C44Matrix &orientation, CGCamera *camera, float scale) {
  if (!model || !camera) {
    return;
  }

  NTempest::C3Vector cameraPos = camera->Position();
  NTempest::C3Vector cameraTarg = camera->Position() + camera->Forward();
  if (!ModelAdvanceTime(model)) {
    return;
  }

  if (!ModelTestSphere(model, position - cameraPos, 0.0f, NTempest::C3Vector(0.0f, 0.0f, 1.0f), scale, 0)) {
    ModelProcessEvents(model, orientation);
    return;
  }

  ModelAnimate(model, orientation, scale, cameraPos, cameraTarg - cameraPos);
  ModelProcessEvents(model, orientation);
  ModelAddToScene(model, 0);
}

SPELL_VISUAL_ATTACHMENT GetMissileTargetLocation(DWORDLONG caster, UINT spellID) {
  CGObject_C     *casterObject = ClntObjMgrObjectPtr(caster, __FILE__, __LINE__);
  const SpellRec *spellRec = g_spellDB.GetRecord(spellID);
  if (!spellRec) {
    return SPELL_VISUAL_ATTACH_CHEST;
  }

  SpellVisualRec        visRecData;
  const SpellVisualRec *visual;
  if (casterObject && casterObject->IsA(TYPE_UNIT)) {
    visual = static_cast<CGUnit_C *>(casterObject)->GetAppropriateSpellVisual(spellRec, visRecData);
  } else {
    visual = g_spellVisualDB.GetRecord(spellRec->m_spellVisualID);
  }
  if (!visual) {
    return SPELL_VISUAL_ATTACH_CHEST;
  }
  return static_cast<SPELL_VISUAL_ATTACHMENT>(visual->m_missileDestinationAttachment);
}

void GetMissileTargetPosition(CGObject_C *target, SPELL_VISUAL_ATTACHMENT hitLocation, NTempest::C3Vector &position) {
  FATALASSERT(target);
  FATALASSERT(hitLocation <= SPELL_VISUAL_ATTACH_BASE);
  FATALASSERT(target->IsA(TYPE_UNIT));

  HMODEL model = target->GetCharacterModel(0);
  FATALASSERT(model);
  switch (hitLocation) {
    default:
      if (ModelGetEventObjectPosition(model, 25, 0, &position)) {
        position += CGWorldFrame::GetActiveCamera()->Position();
        HandleClose(model);
        return;
      }
      target->ReportMissingEventObject(25, 0);
    case SPELL_VISUAL_ATTACH_BASE:
      position = target->GetPosition();
      break;
    case SPELL_VISUAL_ATTACH_HEAD:
      if (ModelGetEventObjectPosition(model, 24, 0, &position)) {
        position += CGWorldFrame::GetActiveCamera()->Position();
        HandleClose(model);
        return;
      }
      target->ReportMissingEventObject(24, 0);
      position = target->GetPosition();
      break;
  }
  HandleClose(model);
}

static bool MoveMissile(MISSILENODE *node) {
  CGObject_C *target = node->target ? ClntObjMgrObjectPtr(node->target, __FILE__, __LINE__) : 0;
  if (target) {
    GetMissileTargetPosition(target, GetMissileTargetLocation(node->caster, node->spellID), node->endPosition);
  }

  UINT elapsed = OsGetAsyncTimeMs() - node->startTime;
  if (elapsed >= node->travelTime) {
    if (target && target->IsA(TYPE_UNIT)) {
      CGUnit_C *unit = static_cast<CGUnit_C *>(target);
      if (node->miss) {
        MISS_REASON            reason = node->missReason;
        const VirtualItemInfo *info = unit->GetVirtualItem(1, 1);
        int                    displayID = unit->GetVirtualItemDisplayID(1);
        if (reason == MISS_BLOCKED && (!info || !displayID)) {
          reason = MISS_DEFLECTED;
        }
        if (reason != MISS_NONE) {
          UnitCombatLogSpellMissed(reason, node->spellID, node->caster, unit->GetGUID());
          CGGameUI::ShowSpellMissFeedback(unit->GetGUID(), reason);
        }
        if (node->caster == ClntObjMgrGetActivePlayer()) {
          unit->AddWorldText(reason);
        }
        switch (reason) {
          case MISS_EVADED:
            unit->SetVictimAnimation(VS_EVADE, 0, 0, 0, 0);
            break;
          case MISS_DODGED:
          case MISS_DEFLECTED:
            unit->SetVictimAnimation(VS_DODGE, 0, 0, 0, 0);
            break;
          case MISS_PARRIED:
            unit->SetVictimAnimation(VS_PARRY, 0, 0, 0, 0);
            break;
          case MISS_BLOCKED:
            unit->SetVictimAnimation(VS_BLOCK, 0, 0, 0, 0);
            break;
        }
      } else {
        unit->SetVictimAnimation(VS_WOUND, unit->GetHealth() <= 0, 0, 1000, 0);
        const SpellVisualKitRec *kit = g_spellVisualKitDB.GetRecord(node->victimEffect);
        if (kit) {
          unit->PlayImpactKit(node->spellID, kit);
        }
      }
      unit->DDDELLOG(node->caster, "MoveMissile", __FILE__, __LINE__);
      node->target = 0;
    }
    s_freeMissiles.Put(node);
    return 0;
  }

  float              ratio = static_cast<float>(elapsed) / static_cast<float>(node->travelTime);
  NTempest::C3Vector movement = (node->endPosition - node->startPosition) * ratio;
  node->position.x = node->startPosition.x + movement.x;
  node->position.y = node->startPosition.y + movement.y;

  NTempest::C3Segment seg(node->position, node->position);
  seg.start.z += MISSILENODE::HEIGHT_SCAN_RANGE;
  seg.end.z -= MISSILENODE::HEIGHT_SCAN_RANGE;
  float             segT = 1.0f;
  NTempest::C4Plane facet;
  if (CWorld::GetFacet(seg, segT, facet, 273)) {
    float ground = seg.start.z + (seg.end.z - seg.start.z) * segT;
    if (node->pathType == 0) {
      node->position.z = node->startPosition.z + movement.z;
      if (node->position.z - ground < MISSILENODE::MIN_HEIGHT) {
        node->position.z = ground + MISSILENODE::MIN_HEIGHT;
      }
    } else if (node->pathType == 1) {
      node->position.z = ground;
      node->normal = facet.n;
    }
  } else {
    node->position.z = node->startPosition.z + movement.z;
    node->normal = NTempest::C3Vector(0.0f, 0.0f, 1.0f);
  }

  node->facing.z = CalculateFacingTo(node->startPosition, node->endPosition);
  float distance = (node->endPosition - node->startPosition).Mag();
  if (node->sound) {
    NTempest::C3Vector vel = node->endPosition - node->position;
    node->sound->SetPosition(node->position, &vel);
  }
  node->facing.x = -atan2(node->endPosition.z - node->startPosition.z, distance);
  return 1;
}

NODEBASE::~NODEBASE() {
  if (model) {
    HandleClose(model);
  }
}

bool NODEBASE::CheckModelLoadStatus() {
  if ((flags & 4) || !ModelIsLoaded(model, 1)) {
    return 0;
  }

  flags |= 4;
  if (!ModelAnimHasObjectId(model, 0)) {
    flags |= 1;
  }
  return 1;
}

static void AddUnitDeathHold(CGUnit_C *unitPtr) {
  if (unitPtr && unitPtr->IsA(TYPE_UNIT)) {
    unitPtr->DDADDLOG(unitPtr->GetGUID(), "UnitEffectOneShot", __FILE__, __LINE__);
  }
}

void ONESHOTEFFECTNODE::CheckModelLoadStatus() {
  if (NODEBASE::CheckModelLoadStatus() && ModelAnimHasObjectId(model, 0)) {
    if (objectGUID) {
      AddUnitDeathHold(static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(objectGUID, __FILE__, __LINE__)));
    }
  }
}

void ONESHOTSTANDALONEEFFECTNODE::CheckModelLoadStatus() {
  if (NODEBASE::CheckModelLoadStatus() && ModelAnimHasObjectId(model, 0)) {
    for (UINT index = objects.Count(); index;) {
      --index;
      AddUnitDeathHold(static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(objects[index], __FILE__, __LINE__)));
    }
  }
}

void MISSILENODE::CheckModelLoadStatus() {
  if ((flags & 1) && !(flags & 2) && ModelIsLoaded(model, 1)) {
    flags |= 2;
    UINT duration;
    if (ModelGetSequenceDuration(model, 0, &duration)) {
      int finishTime = startTime + travelTime;
      int currentTime = OsGetAsyncTimeMs();
      if (finishTime > currentTime) {
        ModelSetTimeScale(model, static_cast<float>(duration) / (finishTime - currentTime), 0);
      }
    }
  }
}

static void RenderMissiles(CGCamera *camera) {
  if (!camera) {
    return;
  }
  for (MISSILENODE *node = s_missiles.Head(), *nodenext_node; (int)node > 0 ? (nodenext_node = s_missiles.RawNext(node), 1) : 0;
       node = nodenext_node) {
    node->CheckModelLoadStatus();
    if (MoveMissile(node)) {
      NTempest::C44Matrix orientation;
      if (!node->pathType) {
        orientation.Identity();
        orientation.Translate(node->position - camera->Position());
        orientation.Rotate(node->facing.z, NTempest::C3Vector(0.0f, 0.0f, 1.0f), false);
        orientation.Rotate(node->facing.x, NTempest::C3Vector(0.0f, 1.0f, 0.0f), false);
      } else if (node->pathType == 1) {
        NTempest::C34Matrix ori34;
        ModelGetStandingMatrix(node->model, node->position - camera->Position(), node->normal, node->facing.z, 1.0f, &ori34);
        orientation = NTempest::C44Matrix(ori34);
      }
      RenderModel(node->model, node->position, orientation, camera, 1.0f);
    }
  }
}

void PERSISTENTUNITEFFECT::Clear() {
  if (!effectModel && !objectModel) {
    return;
  }

  if (!objectModel && effectModel) {
    HandleClose(effectModel);
    effectModel = 0;
    return;
  }

  FATALASSERT(objectModel && effectModel);
  FATALASSERT(linkPoint < NUM_ATTACH_SLOTS);
  ModelRemoveLink(objectModel, linkPoint, effectModel);
  HandleClose(effectModel);
  HandleClose(objectModel);
  objectModel = 0;
  effectModel = 0;
}

static BOOL DeathHoldEventTimerHandler(LPCVOID packetData, LPVOID param) {
  FATALASSERT(param);

  NODEBASE *node = static_cast<NODEBASE *>(param);
  node->deathHoldTimer = 0;
  node->ReleaseDeathHolds();
  return 1;
}

void NODEBASE::ClearDeathHoldTimer() {
  if (deathHoldTimer) {
    ClientKillTimer(deathHoldTimer, DeathHoldEventTimerHandler, "DeathHoldEventTimerHandler");
    deathHoldTimer = 0;
  }
}

void NODEBASE::SetDeathHoldTimer(UINT duration) {
  ASSERT(!deathHoldTimer);
  deathHoldTimer = ClientSetTimer(duration >> 1, DeathHoldEventTimerHandler, this);
}

ONESHOTSTANDALONEEFFECTNODE::~ONESHOTSTANDALONEEFFECTNODE() {
  ReleaseDeathHolds();
  if (worldObject) {
    CWorld::RemoveObject(worldObject);
  }
}

ONESHOTEFFECTNODE::~ONESHOTEFFECTNODE() {
  ReleaseDeathHolds();

  if (objectModel) {
    FATALASSERT(objectModelAttachmentPoint < NUM_ATTACH_SLOTS);
    ModelRemoveLink(objectModel, objectModelAttachmentPoint, model);
    HandleClose(objectModel);
  }
}

static void DecorateEffectFilename(LPCSTR fileName, int raceSexSpecific, const CGObject_C *object, char *buffer, UINT size) {
  FATALASSERT(object);
  FATALASSERT(buffer);
  FATALASSERT(size);
  FATALASSERT(fileName);

  if (!*fileName) {
    fileName = "GimmeTheShaneCube";
  }

  if (!raceSexSpecific) {
    SStrCopy(buffer, fileName, size);
    return;
  }

  char scratchBuffer[MAX_PATH];
  SStrCopy(scratchBuffer, fileName, size);
  if (!object->IsA(TYPE_PLAYER)) {
    SStrCopy(buffer, scratchBuffer, size);
    return;
  }

  char  extensionString[5] = "";
  char *extension = SStrChrR(scratchBuffer, '.');
  if (extension && *extension) {
    SStrCopy(extensionString, extension, sizeof(extensionString));
    *extension = 0;
  }

  const CGPlayer_C *player = static_cast<const CGPlayer_C *>(object);
  int               sex = player->GetDisplaySex();
  int               race = player->GetDisplayRace();
  FATALASSERT(sex < UNITSEX_LAST);
  const ChrRacesRec *rec = g_chrRacesDB.GetRecord(race);
  FATALASSERT(rec);

  SStrPrintf(buffer, size, "%s%s%s%s", scratchBuffer, rec->m_clientFileString, g_sexString[sex], extensionString);
}

void UnitEffectsInitialize() {
  s_showEffectsStandalone = CVar::Register("showEffectsStandalone", 0, 0, "1", 0, 5, false, 0);
  LoadUnitDefs();
}

void UnitEffectsShutdown() {
  {
    for (MISSILENODE *node = s_missiles.Head(); node; node = s_missiles.Head()) {
      s_freeMissiles.Put(node);
    }
  }

  {
    for (ONESHOTSTANDALONEEFFECTNODE *node = s_standAloneEffects.Head(); node; node = s_standAloneEffects.Head()) {
      s_freeStandaloneEffects.Put(node);
    }
  }

  if (s_timer) {
    ClientKillTimer(s_timer, PurgeTimerHandler, "PurgeTimerHandler");
    s_timer = 0;
  }
}

void UnitEffectUpdate(CGCamera *camera) {
  if (s_showEffectsStandalone->GetInt()) {
    RenderMissiles(camera);
  }

  {
    ITERATELIST(UNITONESHOTEFFECTDESC, s_oneShotEffects, effectDesc) {
      ITERATELIST(ONESHOTEFFECTNODE, effectDesc->m_effects, node) {
        node->CheckModelLoadStatus();
      }
    }
  }

  {
    ITERATELIST(ONESHOTSTANDALONEEFFECTNODE, s_standAloneEffects, standalone) {
      standalone->CheckModelLoadStatus();
    }
  }
}

void UnitEffectOneShot(
    const SpellVisualEffectNameRec *effect,
    CGObject_C                     *object,
    UNITEFFECTATTACHPPOINT          attachPoint,
    int                             spellID,
    bool                            isCastEffect,
    bool                            forceEffectOnMount
) {
  if (!effect) {
    return;
  }

  FATALASSERT(attachPoint < (sizeof(g_attachmentPoints) / sizeof(g_attachmentPoints[0])));

  UNITONESHOTEFFECTDESC *unitEffectDesc;
  ONESHOTEFFECTNODE     *newEffectNode = 0;
  const DWORDLONG       &guid = object->GetGUID();
  unitEffectDesc = s_oneShotEffects.Ptr(guid, guid);
  if (!unitEffectDesc) {
    unitEffectDesc = s_oneShotEffects.New(guid, guid, 0, 0);
  }

  HMODEL objectModel;
  HMODEL model;
  HMODEL effectModel = 0;
  if (object->IsA(TYPE_UNIT)) {
    if (forceEffectOnMount) {
      model = static_cast<CGUnit_C *>(object)->GetMountedModel();
    } else {
      model = object->GetCharacterModel(0);
    }
  } else {
    model = static_cast<HMODEL>(HandleDuplicate(object->GetObjectModel()));
  }
  if (!model) {
    return;
  }

  GEOCOMPONENTLINKS linkPoint = g_attachmentPoints[attachPoint];
  if (!ModelHasLinkPoint(model, linkPoint)) {
    if (attachPoint != UNITEFFECT_ATTACHCHEST) {
      HandleClose(model);
      return;
    }
    linkPoint = ATTACH_TORSOBLOODFRONT;
    if (!ModelHasLinkPoint(model, linkPoint)) {
      HandleClose(model);
      return;
    }
  }

  objectModel = static_cast<HMODEL>(HandleDuplicate(model));
  if (!objectModel) {
    SysMsgPrintf(SYSMSG_WARNING, 16, "OBJECTCANTDUPLICATEMODEL|%d", object->GetEntryID());
  } else {
    char fileName[MAX_PATH];
    DecorateEffectFilename(effect->m_fileName, 0, object, fileName, sizeof(fileName));

    newEffectNode = unitEffectDesc->m_effects.NewNode(LIST_TAIL, 0, 0);
    newEffectNode->spellID = spellID;
    newEffectNode->isCastEffect = isCastEffect;

    effectModel = InitializeModel(fileName, SpellUnitAnimEventCallback, newEffectNode);
    if (effectModel && ModelAddLink(objectModel, linkPoint, effectModel, 1.0f)) {
      if (effect->m_VisualEffectNameFlags & 1) {
        newEffectNode->flags |= 2;
      }
      newEffectNode->objectModel = objectModel;
      newEffectNode->model = effectModel;
      newEffectNode->objectModelAttachmentPoint = linkPoint;
      newEffectNode->objectGUID = object->GetGUID();
      if (effect->m_VisualEffectNameFlags & 4) {
        DEL(newEffectNode);
      } else {
        ModelSetSeqFinishedHandler(effectModel, 0, OneShotEndHandler, newEffectNode);
      }
      HandleClose(model);
      return;
    }
  }

  if (objectModel) {
    HandleClose(objectModel);
  }
  if (effectModel) {
    HandleClose(effectModel);
  }
  DEL(newEffectNode);
  HandleClose(model);
}

static void CheckReinitTimer(int current, UINT duration) {
  int triggerTime = current + duration;
  if (s_timer) {
    if (triggerTime >= s_timerExpiry) {
      return;
    }
    ClientKillTimer(s_timer, PurgeTimerHandler, "PurgeTimerHandler");
  }

  s_timer = ClientSetTimer(duration, PurgeTimerHandler, 0);
  s_timerExpiry = triggerTime;
}

static BOOL PurgeTimerHandler(LPCVOID timerData, LPVOID userData) {
  s_timer = 0;

  int                          found = 0;
  int                          next = 0x7FFFFFFF;
  for (ONESHOTSTANDALONEEFFECTNODE *node = s_standAloneEffects.Head(), *nodenext_node;
       (int)node > 0 ? (nodenext_node = s_standAloneEffects.RawNext(node), 1) : 0; node = nodenext_node) {
    if (node->expireTime <= static_cast<int>(static_cast<const EVENT_DATA_TIMER *>(timerData)->currTime)) {
      s_freeStandaloneEffects.Put(node);
    } else {
      if (next >= node->expireTime) {
        next = node->expireTime;
      }
      ++found;
    }
  }

  if (found) {
    CheckReinitTimer(
        static_cast<const EVENT_DATA_TIMER *>(timerData)->currTime, next - static_cast<const EVENT_DATA_TIMER *>(timerData)->currTime
    );
  }
  return 1;
}

void UnitEffectOneShot(
    const SpellVisualEffectNameRec *effectRec,
    const NTempest::C3Vector       &location,
    const TSStackArray<DWORDLONG>  *objects,
    float                           facing,
    float                           scale
) {
  if (!effectRec || !s_showEffectsStandalone->GetInt()) {
    return;
  }

  ONESHOTSTANDALONEEFFECTNODE *unitEffectDesc = s_freeStandaloneEffects.Get(0);

  HMODEL model = InitializeModel(effectRec->m_fileName, SpellAreaAnimEventCallback, unitEffectDesc);
  if (!model) {
    s_freeStandaloneEffects.Put(unitEffectDesc);
    return;
  }

  UINT duration = 0;
  if (!ModelIsLoaded(model, 1) || !ModelGetSequenceDuration(model, 0, &duration) || !duration) {
    HandleClose(model);
    s_freeStandaloneEffects.Put(unitEffectDesc);
    return;
  }

  int currentTime = OsGetAsyncTimeMs();
  CheckReinitTimer(currentTime, duration);
  s_standAloneEffects.LinkNode(unitEffectDesc, LIST_TAIL, 0);
  unitEffectDesc->model = model;
  unitEffectDesc->position = location;
  unitEffectDesc->facing = facing;
  unitEffectDesc->scale = scale;
  unitEffectDesc->expireTime = currentTime + duration;

  NTempest::C44Matrix tempMat;
  tempMat.Translate(location);
  tempMat.Rotate(facing, NTempest::C3Vector(0.0f, 0.0f, 1.0f), 1);
  tempMat.Scale(scale);
  unitEffectDesc->worldObject = CWorld::AddDoodad(effectRec->m_fileName, model, tempMat, 6);
  if (effectRec->m_VisualEffectNameFlags & 1) {
    unitEffectDesc->flags |= 2;
  }

  if (objects) {
    unitEffectDesc->objects.Set(objects->Count(), objects->Ptr());
    for (UINT index = 0; index < objects->Count(); ++index) {
      ClntObjMgrObjectPtr((*objects)[index], __FILE__, __LINE__);
    }
  } else {
    unitEffectDesc->objects.SetCount(0);
  }
}

void UnitEffectClear(CGObject_C *object) {
  if (object) {
    const DWORDLONG       &guid = object->GetGUID();
    UNITONESHOTEFFECTDESC *desc = s_oneShotEffects.Ptr(guid, guid);
    if (desc) {
      s_oneShotEffects.Delete(desc);
    }
  }
}

void UnitEffectClearSpellPrecast(CGObject_C *object, int spellID) {
  if (!object) {
    return;
  }

  const DWORDLONG       &guid = object->GetGUID();
  UNITONESHOTEFFECTDESC *effectDesc = s_oneShotEffects.Ptr(guid, guid);
  if (!effectDesc) {
    return;
  }

  for (ONESHOTEFFECTNODE *node = effectDesc->m_effects.Head(), *nodenext_node;
       (int)node > 0 ? (nodenext_node = effectDesc->m_effects.RawNext(node), 1) : 0; node = nodenext_node) {
    if (node->spellID == spellID && !node->isCastEffect) {
      effectDesc->m_effects.DeleteNode(node);
    }
  }
}

int UnitEffectGetSpecialVisual(UNITEFFECTSPECIALS effectNumber) {
  return g_specialSpellIDs[effectNumber];
}

void UnitEffectOneShot(
    UNITEFFECTSPECIALS        effectNumber,
    DWORDLONG                 target,
    const NTempest::C3Vector *attachPos,
    float                     facing,
    float                     scale,
    bool                      forceEffectOnMount
) {
  if (target && effectNumber < 43) {
    CGObject_C *object = ClntObjMgrObjectPtr(target, __FILE__, __LINE__);
    if (object) {
      int effectID = g_specialSpellIDs[effectNumber];
      if (effectID != -1) {
        const SpellVisualEffectNameRec *effectRec = g_spellVisualEffectNameDB.GetRecord(effectID);
        if (effectRec) {
          if (effectRec->m_specialAttachPoint == 4) {
            NTempest::C3Vector position = object->GetPosition();
            if (attachPos) {
              position = *attachPos;
            }
            UnitEffectOneShot(effectRec, position, 0, facing, scale);
          } else {
            FATALASSERT(CMath::fequal_(scale, 1.0f));
            UnitEffectOneShot(effectRec, object, static_cast<UNITEFFECTATTACHPPOINT>(effectRec->m_specialAttachPoint), 0, 1, forceEffectOnMount);
          }
        }
      }
    }
  }
}

void SpellCameraShakeCallback(LPCSTR eventName, const NTempest::C3Vector &position) {
  const SpellEffectCameraShakesRec *shakes = g_spellEffectCameraShakesDB.GetRecord(SStrToInt(eventName));
  if (shakes) {
    CGWorldFrame *worldFrame = CGWorldFrame::GetActive();
    FATALASSERT(worldFrame);
    CGCamera *camera = worldFrame->Camera();
    FATALASSERT(camera);
    for (UINT i = 0; i < 3; ++i) {
      camera->AddShake(shakes->m_CameraShake[i], position);
    }
  }
}

void SpellSoundEffectCallback(LPCSTR eventName, const NTempest::C3Vector &position) {
  if (eventName && *eventName) {
    SndInterfacePlaySound(SStrToUnsigned(eventName), position, -1, 1.0f);
  }
}

GEOCOMPONENTLINKS UnitEffectGetLinkPointFromAttachment(UNITEFFECTATTACHPPOINT attach) {
  FATALASSERT(attach >= 0);
  FATALASSERT(attach < (sizeof(g_attachmentPoints) / sizeof(g_attachmentPoints[0])));
  return g_attachmentPoints[attach];
}

HMODEL UnitEffectCreateAuraModel(UINT effectID) {
  const SpellVisualEffectNameRec *effectRec = g_spellVisualEffectNameDB.GetRecord(effectID);
  if (!effectRec) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "SPELLEFFECTIDNOTFOUND|%d", effectID);
    return 0;
  }

  return InitializeModel(effectRec->m_fileName, SpellAnimEventCallback, 0);
}

bool UnitEffectIsAuraWorldObject(UINT effectID, bool &isWorldObj) {
  const SpellVisualEffectNameRec *effectRec = g_spellVisualEffectNameDB.GetRecord(effectID);
  if (!effectRec) {
    return 0;
  }

  isWorldObj = (effectRec->m_VisualEffectNameFlags & 8) != 0;
  return 1;
}

DWORD UnitEffectCreateWorldModelAura(UINT effect, const NTempest::C3Vector &location, float facing) {
  const SpellVisualEffectNameRec *effectRec = g_spellVisualEffectNameDB.GetRecord(effect);
  if (!effect) {
    return 0;
  }

  HMODEL model = InitializeModel(effectRec->m_fileName, 0, 0);
  if (!model) {
    return 0;
  }

  NTempest::C44Matrix tempMat;
  tempMat.Translate(location);
  tempMat.Rotate(facing, NTempest::C3Vector(0.0f, 0.0f, 1.0f), 1);
  DWORD object = CWorld::AddDoodad(effectRec->m_fileName, model, tempMat, 6);
  HandleClose(model);
  return object;
}

void UnitEffectAddMissile(const MISSILESTRUCT &desc, int durationOffset) {
  NTempest::C3Vector endPos;
  if (desc.target) {
    CGObject_C *target = ClntObjMgrObjectPtr(desc.target, __FILE__, __LINE__);
    if (!target) {
      return;
    }
    GetMissileTargetPosition(target, GetMissileTargetLocation(desc.caster->GetGUID(), desc.spellID), endPos);
  } else {
    endPos = desc.destination;
  }

  HMODEL                          model;
  const SpellVisualEffectNameRec *effectRec = g_spellVisualEffectNameDB.GetRecord(desc.missileEffect);
  if (effectRec && *effectRec->m_fileName) {
    char modelName[MAX_PATH];
    DecorateEffectFilename(effectRec->m_fileName, 0, desc.caster, modelName, sizeof(modelName));
    model = InitializeModel(modelName, 0, 0);
  } else {
    if (!desc.ammoDisplayID) {
      return;
    }
    UINT dummy1;
    model = ObjComponentBuildAmmoModel(g_itemDisplayInfoDB.GetRecord(desc.ammoDisplayID), desc.inventoryType, dummy1);
  }
  if (!model) {
    return;
  }

  MISSILENODE *node = s_freeMissiles.Get(0);
  s_missiles.LinkNode(node, LIST_TAIL, 0);
  node->model = model;
  node->caster = desc.caster ? desc.caster->GetGUID() : 0;
  node->startPosition = desc.startPosition;
  node->pathType = desc.missilePathType;
  if (node->pathType == 1) {
    NTempest::C3Segment seg;
    seg.start = node->startPosition;
    seg.start.z += MISSILENODE::HEIGHT_SCAN_RANGE;
    seg.end = node->startPosition;
    seg.end.z -= MISSILENODE::HEIGHT_SCAN_RANGE * 5.0f;
    float             segT = 1.0f;
    NTempest::C4Plane facet;
    if (CWorld::GetFacet(seg, segT, facet, 273)) {
      node->startPosition.z = seg.start.z + (seg.end.z - seg.start.z) * segT;
    } else {
      node->pathType = 0;
    }
  }
  node->position = node->startPosition;
  node->endPosition = endPos;
  node->target = desc.target;
  node->startTime = OsGetAsyncTimeMs();
  node->victimEffect = desc.missileVictimEffect;
  node->miss = !desc.hits;
  node->missReason = desc.reason;
  node->spellID = desc.spellID;
  node->sound = SndInterfacePlayLoopedSound(desc.sound, node->position, 0);

  int duration = Fast_ftol((node->endPosition - node->startPosition).Mag() / desc.speed * 1000.0f);
  if (durationOffset < duration) {
    duration -= durationOffset;
  }
  node->travelTime = duration < 0 ? 0 : duration;
  if (node->travelTime > 0) {
    node->flags |= 1;
  }
  if (desc.target) {
    CGObject_C *target = ClntObjMgrObjectPtr(desc.target, __FILE__, __LINE__);
    if (target && target->IsA(TYPE_UNIT)) {
      static_cast<CGUnit_C *>(target)->DDADDLOG(desc.caster ? desc.caster->GetGUID() : 0, "UnitEffectAddMissile", __FILE__, __LINE__);
    }
  }
}

void UnitEffectPreloadSpellEffects(int spellID) {
  const SpellRec *spell = g_spellDB.GetRecord(spellID);
  if (!spell) {
    return;
  }

  const SpellVisualRec *visual = g_spellVisualDB.GetRecord(spell->m_spellVisualID);
  if (!visual) {
    return;
  }

  CStatus status;
  if (visual->m_hasAreaEffect) {
    PreloadModel(visual->m_areaModel, &status);
  }
  if (visual->m_hasMissile) {
    PreloadModel(visual->m_missileModel, &status);
  }
  PreloadModelsByKit(visual->m_precastKit, &status);
  PreloadModelsByKit(visual->m_castKit, &status);
  PreloadModelsByKit(visual->m_impactKit, &status);
  PreloadModelsByKit(visual->m_stateKit, &status);
  PreloadModelsByKit(visual->m_channelKit, &status);
  SysMsgAdd(status, 16);
}
