#include <WowConst.h>
#include <MapDefs.h>

#include "Unit_C.h"
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
#include "Ui/WorldFrame.h"
#include "WorldClient/World.h"
#include <Event/EvtInt.h>
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

BOOL        DeathHoldEventTimerHandler(LPCVOID packetData, LPVOID param);
void        SpellVisualsPlayCameraShakeID(UINT shakeID, const NTempest::C3Vector &position);
HMODEL      InitializeModel(LPCSTR fileName, void (*callback)(LPCSTR, const NTempest::C3Vector &, LPVOID), LPVOID param);
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
void PreloadModel(int effectID, CStatus *status);
void PreloadModelsByKit(int record, CStatus *status);

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

static LPCSTR s_sequenceNames[3] = {"Stand", "Hold", "Decay"};
static LPCSTR s_objNames[1] = {"$DTH"};

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

HMODEL CreateModel(LPCSTR fileName, CStatus *status) {
  CModelCreate createData;
  createData.flags = 0x2006;
  createData.sequenceNames = s_sequenceNames;
  createData.numSequences = 3;
  createData.boneNames = s_objNames;
  createData.numBones = 1;
  createData.cameraNames = 0;
  createData.numCameras = 0;
  return ModelCreate(fileName, &createData, status);
}

void PreloadModel(int effectID, CStatus *status) {
  const SpellVisualEffectNameRec *effect = g_spellVisualEffectNameDB.GetRecord(effectID);
  if (effect) {
    HMODEL model = CreateModel(effect->m_fileName, status);
    if (model) {
      HandleClose(model);
    }
  }
}

void PreloadModelsByKit(int record, CStatus *status) {
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
    case 0x444E5324:  // $SND
    case 0x58444E53:  // SNDX
      break;
    case 0x4B485324:  // $SHK
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
  virtual void ReleaseDeathHolds() = 0;
  ~NODEBASE();

  void ClearDeathHoldTimer();
  void SetDeathHoldTimer(UINT duration);
  bool CheckModelLoadStatus();

  LINKDECLEX(NODEBASE, node);
  HMODEL__ *model;
  UINT      flags;
  UINT      deathHoldTimer;
};

class ONESHOTEFFECTNODE : public NODEBASE {
 public:
  ONESHOTEFFECTNODE() : objectModel(0), objectModelAttachmentPoint(0), objectGUID(0), spellID(0), isCastEffect(0) {
  }
  virtual void ReleaseDeathHolds();
  void         CheckModelLoadStatus();
  ~ONESHOTEFFECTNODE();

  HMODEL__ *objectModel;
  UINT      objectModelAttachmentPoint;
  DWORDLONG objectGUID;
  int       spellID;
  BYTE      isCastEffect;
};

class ONESHOTSTANDALONEEFFECTNODE : public NODEBASE {
 public:
  ONESHOTSTANDALONEEFFECTNODE() : facing(0.0f), scale(1.0f), worldObject(0), expireTime(0) {
  }
  virtual void ReleaseDeathHolds();
  void         CheckModelLoadStatus();
  ~ONESHOTSTANDALONEEFFECTNODE();

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

  MISSILENODE()
      : model(0),
        caster(0),
        target(0),
        startTime(0),
        travelTime(0),
        spellID(0),
        victimEffect(0),
        pathType(0),
        miss(0),
        missReason(MISS_NONE),
        flags(0),
        sound(0) {
  }
  ~MISSILENODE();
  void CheckModelLoadStatus();

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
  Sound             *sound;
};

const float MISSILENODE::HEIGHT_SCAN_RANGE = 5.0f;
const float MISSILENODE::MIN_HEIGHT = 0.3f;

struct UNITONESHOTEFFECTDESC : public TSHashObject<UNITONESHOTEFFECTDESC, CHashKeyGUID> {
  UNITONESHOTEFFECTDESC() {
  }
  UNITONESHOTEFFECTDESC(const UNITONESHOTEFFECTDESC &);
  ~UNITONESHOTEFFECTDESC() {
  }

  LISTDECLEX(ONESHOTEFFECTNODE, node, m_effects);
};

static TSHashTable<UNITONESHOTEFFECTDESC, CHashKeyGUID> s_oneShotEffects;
static LISTDECLEX(ONESHOTSTANDALONEEFFECTNODE, node, s_standAloneEffects);
static TInstanceAllocator<ONESHOTSTANDALONEEFFECTNODE> s_freeStandaloneEffects(40);
static LISTDECL(MISSILENODE, s_missiles);
static TInstanceAllocator<MISSILENODE> s_freeMissiles(10);
static CVar                           *s_showEffectsStandalone;
UINT                                   g_specialSpellIDs[43];
static UINT                            s_purgeTimer;
static int                             s_purgeTime;

static const GEOCOMPONENTLINKS g_attachmentPoints[12] = {
    ATTACH_UNITEFFECT_BASE, ATTACH_UNITEFFECT_HEAD, ATTACH_UNITEFFECT_SPELLLEFTHAND, ATTACH_UNITEFFECT_SPELLRIGHTHAND, ATTACH_NONE,
    ATTACH_BREATH,          ATTACH_TORSOSPELL,      ATTACH_UNITEFFECT_SPECIAL1,      ATTACH_UNITEFFECT_SPECIAL2,       ATTACH_UNITEFFECT_SPECIAL3,
    ATTACH_TORSOBLOODBACK,  ATTACH_TORSOBLOODFRONT
};

static void SpellUnitAnimEventCallback(LPCSTR eventName, const NTempest::C3Vector &position, LPVOID param) {
  ONESHOTEFFECTNODE *node = static_cast<ONESHOTEFFECTNODE *>(param);
  UINT               event = *reinterpret_cast<const UINT *>(eventName);

  switch (event) {
    case 0x50504324:  // $CPP
    case 0x48414324:  // $ACH
    case 0x53534324:  // $CSS
      if (node) {
        CGObject_C *object = ClntObjMgrObjectPtr(node->objectGUID, __FILE__, __LINE__);
        if (object && (object->GetType() & TYPE_UNIT)) {
          static_cast<CGUnit_C *>(object)->HandleCombatAnimEvent(eventName, event, position);
        }
      }
      break;
    case 0x48544424:  // $DTH
      if (node) {
        node->ReleaseDeathHolds();
      }
      break;
    case 0x444E5324:    // $SND
    case 0x58444E53: {  // SNDX
      NTempest::C3Vector soundPos = position;
      if (node && node->objectGUID) {
        CGObject_C *object = ClntObjMgrObjectPtr(node->objectGUID, __FILE__, __LINE__);
        if (object) {
          soundPos += object->GetPosition();
        }
      }
      SpellSoundEffectCallback(eventName + 4, soundPos);
      break;
    }
    case 0x4B485324:  // $SHK
      SpellCameraShakeCallback(eventName + 4, position);
      break;
    case 0x54494824:  // $HIT
      if (node) {
        CGObject_C *object = ClntObjMgrObjectPtr(node->objectGUID, __FILE__, __LINE__);
        if (object && (object->GetType() & TYPE_UNIT)) {
          static_cast<CGUnit_C *>(object)->SpellEventHit();
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
    if (object && (object->GetType() & TYPE_UNIT)) {
      static_cast<CGUnit_C *>(object)->DDDELLOG(object->GetGUID(), "ONESHOTEFFECTNODE::ReleaseDeathHolds", __FILE__, __LINE__);
    }
  }
}

static void SpellAreaAnimEventCallback(LPCSTR eventName, const NTempest::C3Vector &position, LPVOID param) {
  ONESHOTSTANDALONEEFFECTNODE *node = static_cast<ONESHOTSTANDALONEEFFECTNODE *>(param);
  UINT                         event = *reinterpret_cast<const UINT *>(eventName);

  switch (event) {
    case 0x4B485324:  // $SHK
      SpellCameraShakeCallback(eventName + 4, position);
      break;
    case 0x48544424:  // $DTH
      if (node) {
        node->ReleaseDeathHolds();
      }
      break;
    case 0x444E5324:    // $SND
    case 0x58444E53: {  // SNDX
      SpellSoundEffectCallback(eventName + 4, position);
      break;
    }
    case 0x54494824:  // $HIT
      if (node) {
        for (UINT index = 0; index < node->objects.Count(); ++index) {
          CGObject_C *object = ClntObjMgrObjectPtr(node->objects[index], __FILE__, __LINE__);
          if (object && (object->GetType() & TYPE_UNIT)) {
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

HMODEL InitializeModel(LPCSTR fileName, void (*callback)(LPCSTR, const NTempest::C3Vector &, LPVOID), LPVOID param) {
  CStatus status;
  HMODEL  model = CreateModel(fileName, &status);
  if (callback) {
    ModelSetEventCallback(model, callback, param, 0);
  }
  ModelSetSequence(model, 0, 0);
  SysMsgAdd(status, 16);
  return model;
}

static void RenderModel(HMODEL__ *model, const NTempest::C3Vector &position, const NTempest::C44Matrix &orientation, CGCamera *camera, float scale) {
  if (!model || !camera || !ModelAdvanceTime(model)) {
    return;
  }

  NTempest::C3Vector  cameraPos = camera->Position();
  NTempest::C3Vector  cameraVector = camera->Up();
  NTempest::C3Vector  relativePosition = position - cameraPos;
  NTempest::C3Vector  rotationAxis(0.0f, 0.0f, 1.0f);
  NTempest::C34Matrix basis(
      orientation.a0, orientation.a1, orientation.a2, orientation.b0, orientation.b1, orientation.b2, orientation.c0, orientation.c1, orientation.c2,
      orientation.d0, orientation.d1, orientation.d2
  );

  if (ModelTestSphere(model, relativePosition, 0.0f, rotationAxis, scale, 0)) {
    ModelAnimate(model, basis, scale, cameraPos, cameraVector);
    ModelProcessEvents(model, basis);
    ModelAddToScene(model, 0);
  } else {
    ModelProcessEvents(model, basis);
  }
}

SPELL_VISUAL_ATTACHMENT GetMissileTargetLocation(DWORDLONG caster, UINT spellID) {
  const SpellRec *spellRec = g_spellDB.GetRecord(spellID);
  if (!spellRec) {
    return SPELL_VISUAL_ATTACH_CHEST;
  }
  CGObject_C           *casterObject = caster ? ClntObjMgrObjectPtr(caster, __FILE__, __LINE__) : 0;
  CGUnit_C             *casterUnit = casterObject && (casterObject->GetType() & TYPE_UNIT) ? static_cast<CGUnit_C *>(casterObject) : 0;
  SpellVisualRec        visRecData;
  const SpellVisualRec *visual =
      casterUnit ? casterUnit->GetAppropriateSpellVisual(spellRec, visRecData) : g_spellVisualDB.GetRecord(spellRec->m_spellVisualID);
  return visual ? static_cast<SPELL_VISUAL_ATTACHMENT>(visual->m_missileDestinationAttachment) : SPELL_VISUAL_ATTACH_CHEST;
}

static void RecycleMissileNode(MISSILENODE *node) {
  s_missiles.UnlinkNode(node);
  s_freeMissiles.Put(node);
}

void GetMissileTargetPosition(CGObject_C *target, SPELL_VISUAL_ATTACHMENT hitLocation, NTempest::C3Vector &position) {
  FATALASSERT(target);
  FATALASSERT(hitLocation <= 2);
  FATALASSERT(target->GetType() & TYPE_UNIT);

  HMODEL model = target->GetCharacterModel(0);
  FATALASSERT(model);
  if (hitLocation != 2) {
    UINT eventObject = hitLocation ? 25 : 24;
    if (ModelGetEventObjectPosition(model, eventObject, 0, &position)) {
      position += CGWorldFrame::GetActiveCamera()->Position();
      HandleClose(model);
      return;
    }
    target->ReportMissingEventObject(eventObject, 0);
  }
  position = target->GetPosition();
  HandleClose(model);
}

static bool MoveMissile(MISSILENODE *node) {
  CGObject_C *target = node->target ? ClntObjMgrObjectPtr(node->target, __FILE__, __LINE__) : 0;
  if (target && (target->GetType() & TYPE_UNIT)) {
    GetMissileTargetPosition(target, GetMissileTargetLocation(node->caster, node->spellID), node->endPosition);
  }

  UINT elapsed = OsGetAsyncTimeMs() - node->startTime;
  if (elapsed >= node->travelTime) {
    if (target && (target->GetType() & TYPE_UNIT)) {
      CGUnit_C *unit = static_cast<CGUnit_C *>(target);
      if (!node->miss) {
        unit->SetVictimAnimation(VS_WOUND, unit->GetUnitData()->health <= 0, 0, 1000, 0);
        const SpellVisualKitRec *kit = g_spellVisualKitDB.GetRecord(node->victimEffect);
        if (kit) {
          unit->PlayImpactKit(node->spellID, kit);
        }
      } else {
        MISS_REASON reason = node->missReason;
        if (reason == MISS_BLOCKED && (!unit->GetVirtualItem(1, 1) || !unit->GetVirtualItemDisplayID(1))) {
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
      }
      unit->DDDELLOG(node->caster, "MoveMissile", __FILE__, __LINE__);
      node->target = 0;
    }
    RecycleMissileNode(node);
    return 0;
  }

  float ratio = static_cast<float>(elapsed) / static_cast<float>(node->travelTime);
  node->position = node->startPosition + (node->endPosition - node->startPosition) * ratio;

  NTempest::C3Vector endPos = node->position;
  endPos.z -= MISSILENODE::HEIGHT_SCAN_RANGE;
  NTempest::C3Vector scanStart = node->position;
  scanStart.z += MISSILENODE::HEIGHT_SCAN_RANGE;
  NTempest::C3Segment seg(scanStart, endPos);
  NTempest::C4Plane   facet;
  float               segT;
  if (CWorld::GetFacet(seg, segT, facet, 273)) {
    float ground = scanStart.z + (endPos.z - scanStart.z) * segT;
    if (node->pathType == 1) {
      node->position.z = ground;
      node->normal = facet.n;
    } else if (node->position.z - ground < MISSILENODE::MIN_HEIGHT) {
      node->position.z = ground + MISSILENODE::MIN_HEIGHT;
    }
  } else {
    node->normal.Set(0.0f, 0.0f, 1.0f);
  }

  NTempest::C3Vector movement = node->endPosition - node->startPosition;
  float              distance = movement.Mag();
  node->facing.z = CalculateFacingTo(node->startPosition, node->endPosition);
  node->facing.x = -atan2(node->endPosition.z - node->startPosition.z, distance);
  if (node->sound) {
    NTempest::C3Vector vel = node->endPosition - node->position;
    node->sound->SetPosition(node->position, &vel);
  }
  return 1;
}

static void AddUnitDeathHold(CGUnit_C *unitPtr) {
  if (unitPtr && unitPtr->IsA(TYPE_UNIT)) {
    unitPtr->DDADDLOG(unitPtr->GetGUID(), "UnitEffectOneShot", __FILE__, __LINE__);
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

static void RenderMissiles(CGCamera *camera) {
  if (!camera) {
    return;
  }
  MISSILENODE *node = s_missiles.Head();
  while (reinterpret_cast<long>(node) > 0) {
    MISSILENODE *nodenext_node = s_missiles.RawNext(node);
    node->CheckModelLoadStatus();
    if (MoveMissile(node)) {
      NTempest::C44Matrix orientation;
      if (!node->pathType) {
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
    node = nodenext_node;
  }
}

BOOL DeathHoldEventTimerHandler(LPCVOID packetData, LPVOID param) {
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

void ONESHOTEFFECTNODE::CheckModelLoadStatus() {
  if (NODEBASE::CheckModelLoadStatus() && ModelAnimHasObjectId(model, 0)) {
    if (objectGUID) {
      AddUnitDeathHold(static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(objectGUID, __FILE__, __LINE__)));
    }
  }
}

void ONESHOTSTANDALONEEFFECTNODE::CheckModelLoadStatus() {
  if (NODEBASE::CheckModelLoadStatus() && ModelAnimHasObjectId(model, 0)) {
    for (UINT index = objects.Count(); index; --index) {
      AddUnitDeathHold(static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(objects[index - 1], __FILE__, __LINE__)));
    }
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

  if (!raceSexSpecific || !(object->GetType() & TYPE_UNIT)) {
    SStrCopy(buffer, fileName, size);
    return;
  }

  char scratchBuffer[MAX_PATH];
  char extensionString[5] = "";
  SStrCopy(scratchBuffer, fileName, sizeof(scratchBuffer));
  char *extension = SStrChrR(scratchBuffer, '.');
  if (extension && *extension) {
    SStrCopy(extensionString, extension, sizeof(extensionString));
    *extension = 0;
  }

  const CGUnit_C *unit = static_cast<const CGUnit_C *>(object);
  UINT            sex = unit->GetDisplaySex();
  UINT            race = unit->GetDisplayRace();
  FATALASSERT(sex < UNITSEX_LAST);
  const ChrRacesRec *raceRec = g_chrRacesDB.GetRecord(race);
  FATALASSERT(raceRec);

  static LPCSTR const sexNames[UNITSEX_LAST] = {"Male", "Female", "NOSEX"};
  SStrPrintf(buffer, size, "%s%s%s%s", scratchBuffer, raceRec->m_clientFileString, sexNames[sex], extensionString);
}

NODEBASE::~NODEBASE() {
  if (model) {
    HandleClose(model);
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

void ONESHOTSTANDALONEEFFECTNODE::ReleaseDeathHolds() {
  ClearDeathHoldTimer();

  if (!(flags & 3)) {
    flags |= 1;
    for (UINT index = 0; index < objects.Count(); ++index) {
      CGObject_C *object = ClntObjMgrObjectPtr(objects[index], __FILE__, __LINE__);
      if (object && (object->GetType() & TYPE_UNIT)) {
        static_cast<CGUnit_C *>(object)->DDDELLOG(object->GetGUID(), "ONESHOTSTANDALONEEFFECTNODE::ReleaseDeathHolds", __FILE__, __LINE__);
      }
    }
  }
}

ONESHOTSTANDALONEEFFECTNODE::~ONESHOTSTANDALONEEFFECTNODE() {
  ReleaseDeathHolds();
  if (worldObject) {
    CWorld::RemoveObject(worldObject);
  }
}

MISSILENODE::~MISSILENODE() {
  if (model) {
    HandleClose(model);
  }
  if (sound) {
    sound->Stop(2.0f);
  }
  if (target) {
    CGObject_C *object = ClntObjMgrObjectPtr(target, __FILE__, __LINE__);
    if (object && (object->GetType() & TYPE_UNIT)) {
      static_cast<CGUnit_C *>(object)->DDDELLOG(caster, "MISSILENODE immaturely freed", __FILE__, __LINE__);
    }
  }
}

void MISSILENODE::CheckModelLoadStatus() {
  if ((flags & 1) && !(flags & 2) && ModelIsLoaded(model, 1)) {
    flags |= 2;
    UINT duration;
    if (ModelGetSequenceDuration(model, 0, &duration)) {
      UINT finishTime = startTime + travelTime;
      UINT currentTime = OsGetAsyncTimeMs();
      if (finishTime > currentTime) {
        ModelSetTimeScale(model, static_cast<float>(duration) / static_cast<float>(finishTime - currentTime), 1);
      }
    }
  }
}

void UnitEffectsInitialize() {
  s_showEffectsStandalone = CVar::Register("showEffectsStandalone", 0, 0, "1", 0, 5, false, 0);
  LoadUnitDefs();
}

void UnitEffectsShutdown() {
  while (reinterpret_cast<long>(s_missiles.Head()) > 0) {
    MISSILENODE *node = s_missiles.Head();
    s_freeMissiles.Put(node);
  }

  while (reinterpret_cast<long>(s_standAloneEffects.Head()) > 0) {
    ONESHOTSTANDALONEEFFECTNODE *node = s_standAloneEffects.Head();
    s_freeStandaloneEffects.Put(node);
  }

  if (s_purgeTimer) {
    ClientKillTimer(s_purgeTimer, PurgeTimerHandler, "PurgeTimerHandler");
    s_purgeTimer = 0;
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

static void CheckReinitTimer(int current, UINT duration) {
  int triggerTime = current + duration;
  if (s_purgeTimer) {
    if (triggerTime >= s_purgeTime) {
      return;
    }
    ClientKillTimer(s_purgeTimer, PurgeTimerHandler, "PurgeTimerHandler");
  }

  s_purgeTimer = ClientSetTimer(duration, PurgeTimerHandler, 0);
  s_purgeTime = triggerTime;
}

static BOOL PurgeTimerHandler(LPCVOID timerData, LPVOID userData) {
  s_purgeTimer = 0;

  int                          current = static_cast<const EvtContext *>(timerData)->GetCurrTime();
  int                          next = 0x7FFFFFFF;
  int                          found = 0;
  ONESHOTSTANDALONEEFFECTNODE *node = s_standAloneEffects.Head();
  while (reinterpret_cast<long>(node) > 0) {
    ONESHOTSTANDALONEEFFECTNODE *nextNode = s_standAloneEffects.RawNext(node);
    if (node->expireTime > current) {
      if (next >= node->expireTime) {
        next = node->expireTime;
      }
      ++found;
    } else {
      s_standAloneEffects.UnlinkNode(node);
      s_freeStandaloneEffects.Put(node);
    }
    node = nextNode;
  }

  if (found) {
    CheckReinitTimer(current, next - current);
  }
  return 1;
}

void UnitEffectClear(CGObject_C *object) {
  if (object) {
    CHashKeyGUID           key(object->GetGUID());
    UNITONESHOTEFFECTDESC *desc = s_oneShotEffects.Ptr(static_cast<UINT>(object->GetGUID()), key);
    if (desc) {
      s_oneShotEffects.Delete(desc);
    }
  }
}

void UnitEffectClearSpellPrecast(CGObject_C *object, int spellID) {
  if (!object) {
    return;
  }

  CHashKeyGUID           key(object->GetGUID());
  UNITONESHOTEFFECTDESC *effectDesc = s_oneShotEffects.Ptr(static_cast<UINT>(object->GetGUID()), key);
  if (!effectDesc) {
    return;
  }

  ITERATELIST(ONESHOTEFFECTNODE, effectDesc->m_effects, node) {
    if (node->spellID == spellID && !node->isCastEffect) {
      ITERATE_DELETE
    }
  }
}

int UnitEffectGetSpecialVisual(UNITEFFECTSPECIALS effectNumber) {
  return g_specialSpellIDs[effectNumber];
}

void SpellCameraShakeCallback(LPCSTR eventName, const NTempest::C3Vector &position) {
  if (eventName && *eventName) {
    const SpellEffectCameraShakesRec *shakes = g_spellEffectCameraShakesDB.GetRecord(SStrToInt(eventName));
    if (shakes) {
      for (UINT i = 0; i < 3; ++i) {
        CGWorldFrame::GetActiveCamera()->AddShake(shakes->m_CameraShake[i], position);
      }
    }
  }
}

void SpellSoundEffectCallback(LPCSTR eventName, const NTempest::C3Vector &position) {
  if (eventName && *eventName) {
    SndInterfacePlaySound(SStrToUnsigned(eventName), position, -1, 1.0f);
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

  FATALASSERT(static_cast<UINT>(attachPoint) < sizeof(g_attachmentPoints) / sizeof(g_attachmentPoints[0]));

  CHashKeyGUID           key(object->GetGUID());
  UNITONESHOTEFFECTDESC *unitEffectDesc = s_oneShotEffects.Ptr(static_cast<UINT>(object->GetGUID()), key);
  if (!unitEffectDesc) {
    unitEffectDesc = s_oneShotEffects.New(static_cast<UINT>(object->GetGUID()), key, 0, 0);
  }

  HMODEL objectModel;
  if ((object->GetType() & TYPE_UNIT) && forceEffectOnMount) {
    objectModel = static_cast<CGUnit_C *>(object)->GetMountedModel();
  } else {
    objectModel = object->GetCharacterModel(0);
  }
  if (!objectModel) {
    return;
  }

  GEOCOMPONENTLINKS linkPoint = g_attachmentPoints[attachPoint];
  if (!ModelHasLinkPoint(objectModel, linkPoint) && attachPoint == UNITEFFECT_ATTACHCHEST) {
    linkPoint = ATTACH_TORSOBLOODFRONT;
  }
  if (!ModelHasLinkPoint(objectModel, linkPoint)) {
    HandleClose(objectModel);
    return;
  }

  HMODEL heldObjectModel = static_cast<HMODEL>(HandleDuplicate(objectModel));
  if (!heldObjectModel) {
    SysMsgPrintf(SYSMSG_WARNING, 16, "OBJECTCANTDUPLICATEMODEL|%d", object->GetEntryID());
    HandleClose(objectModel);
    return;
  }

  char fileName[MAX_PATH];
  DecorateEffectFilename(effect->m_fileName, effect->m_VisualEffectNameFlags & 8, object, fileName, sizeof(fileName));

  ONESHOTEFFECTNODE *newEffectNode = NEW(ONESHOTEFFECTNODE);
  unitEffectDesc->m_effects.LinkNode(newEffectNode, LIST_HEAD, 0);
  newEffectNode->spellID = spellID;
  newEffectNode->isCastEffect = isCastEffect;

  HMODEL model = InitializeModel(fileName, SpellUnitAnimEventCallback, newEffectNode);
  if (model && ModelAddLink(objectModel, linkPoint, model, 1.0f)) {
    if (effect->m_VisualEffectNameFlags & 1) {
      newEffectNode->flags |= 2;
    }
    newEffectNode->model = model;
    newEffectNode->objectModel = heldObjectModel;
    newEffectNode->objectModelAttachmentPoint = linkPoint;
    newEffectNode->objectGUID = object->GetGUID();
    if (!(effect->m_VisualEffectNameFlags & 4)) {
      ModelSetSeqFinishedHandler(model, 0, OneShotEndHandler, newEffectNode);
      HandleClose(objectModel);
      return;
    }
    DEL(newEffectNode);
    HandleClose(objectModel);
    return;
  }

  if (model) {
    HandleClose(model);
  }
  HandleClose(heldObjectModel);
  DEL(newEffectNode);
  HandleClose(objectModel);
}

GEOCOMPONENTLINKS UnitEffectGetLinkPointFromAttachment(UNITEFFECTATTACHPPOINT attach) {
  FATALASSERT(attach >= 0);
  FATALASSERT(attach < sizeof(g_attachmentPoints) / sizeof(g_attachmentPoints[0]));
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
  s_standAloneEffects.LinkNode(unitEffectDesc, LIST_HEAD, 0);
  unitEffectDesc->model = model;
  unitEffectDesc->position = location;
  unitEffectDesc->facing = facing;
  unitEffectDesc->scale = scale;
  unitEffectDesc->expireTime = currentTime + duration;

  NTempest::C44Matrix tempMat;
  tempMat.Translate(location);
  NTempest::C3Vector axis(0.0f, 0.0f, 1.0f);
  tempMat.Rotate(facing, axis, 1);
  tempMat.Scale(scale);
  unitEffectDesc->worldObject = CWorld::AddDoodad(effectRec->m_fileName, model, tempMat, 6);
  if (effectRec->m_VisualEffectNameFlags & 1) {
    unitEffectDesc->flags |= 2;
  }

  if (objects) {
    unitEffectDesc->objects.Set(objects->Count(), objects->Ptr());
  } else {
    unitEffectDesc->objects.SetCount(0);
  }
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
  if (!effect) {
    return 0;
  }

  const SpellVisualEffectNameRec *effectRec = g_spellVisualEffectNameDB.GetRecord(effect);
  HMODEL                          model = InitializeModel(effectRec->m_fileName, 0, 0);
  if (!model) {
    return 0;
  }

  NTempest::C44Matrix tempMat;
  tempMat.Translate(location);
  NTempest::C3Vector axis(0.0f, 0.0f, 1.0f);
  tempMat.Rotate(facing, axis, 1);
  DWORD object = CWorld::AddDoodad(effectRec->m_fileName, model, tempMat, 6);
  HandleClose(model);
  return object;
}

void UnitEffectAddMissile(const MISSILESTRUCT &desc, int durationOffset) {
  NTempest::C3Vector endPos;
  CGObject_C        *target = desc.target ? ClntObjMgrObjectPtr(desc.target, __FILE__, __LINE__) : 0;
  if (desc.target && !target) {
    return;
  }
  DWORDLONG caster = desc.caster ? desc.caster->GetGUID() : 0;
  if (target) {
    GetMissileTargetPosition(target, GetMissileTargetLocation(caster, desc.spellID), endPos);
  } else {
    endPos = desc.destination;
  }

  char                            modelName[MAX_PATH];
  HMODEL                          model = 0;
  UINT                            dummy1 = 0;
  const SpellVisualEffectNameRec *effectRec = g_spellVisualEffectNameDB.GetRecord(desc.missileEffect);
  if (effectRec && effectRec->m_fileName && *effectRec->m_fileName) {
    DecorateEffectFilename(effectRec->m_fileName, 0, desc.caster, modelName, sizeof(modelName));
    model = InitializeModel(modelName, 0, 0);
  } else if (desc.ammoDisplayID) {
    model = ObjComponentBuildAmmoModel(g_itemDisplayInfoDB.GetRecord(desc.ammoDisplayID), desc.inventoryType, dummy1);
  }
  if (!model) {
    return;
  }

  MISSILENODE *node = s_freeMissiles.Get(0);
  s_missiles.LinkNode(node, LIST_HEAD, 0);
  node->model = model;
  node->caster = caster;
  node->startPosition = desc.startPosition;
  node->pathType = desc.missilePathType;
  if (node->pathType == 1) {
    NTempest::C3Vector scanStart = node->startPosition;
    scanStart.z += MISSILENODE::HEIGHT_SCAN_RANGE;
    NTempest::C3Vector scanEnd = node->startPosition;
    scanEnd.z -= MISSILENODE::HEIGHT_SCAN_RANGE * 5.0f;
    NTempest::C3Segment seg(scanStart, scanEnd);
    NTempest::C4Plane   facet;
    float               segT;
    if (CWorld::GetFacet(seg, segT, facet, 273)) {
      node->startPosition.z = scanStart.z + (scanEnd.z - scanStart.z) * segT;
    } else {
      node->pathType = 0;
    }
  }
  node->position = node->startPosition;
  node->endPosition = endPos;
  node->target = desc.target;
  node->startTime = OsGetAsyncTimeMs();
  node->spellID = desc.spellID;
  node->victimEffect = desc.missileVictimEffect;
  node->miss = !desc.hits;
  node->missReason = desc.reason;
  node->sound = SndInterfacePlayLoopedSound(desc.sound, node->position, 0);

  float distance = (node->endPosition - node->startPosition).Mag();
  int   duration = static_cast<int>(distance / desc.speed * 1000.0f - 0.5f);
  if (durationOffset < duration) {
    duration -= durationOffset;
  }
  node->travelTime = duration < 0 ? 0 : duration;
  if (node->travelTime) {
    node->flags |= 1;
  }
  if (target && (target->GetType() & TYPE_UNIT)) {
    static_cast<CGUnit_C *>(target)->DDADDLOG(node->caster, "UnitEffectAddMissile", __FILE__, __LINE__);
  }
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
      int                             effectID = g_specialSpellIDs[effectNumber];
      const SpellVisualEffectNameRec *effectRec = g_spellVisualEffectNameDB.GetRecord(effectID);
      if (effectRec) {
        if (effectRec->m_specialAttachPoint == 4) {
          NTempest::C3Vector position = object->GetPosition();
          if (attachPos) {
            position = *attachPos;
          }
          UnitEffectOneShot(effectRec, position, 0, facing, scale);
        } else {
          FATALASSERT(NTempest::CMath::fequal_(scale, 1.0f));
          UnitEffectOneShot(effectRec, object, static_cast<UNITEFFECTATTACHPPOINT>(effectRec->m_specialAttachPoint), 0, 1, forceEffectOnMount);
        }
      }
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
