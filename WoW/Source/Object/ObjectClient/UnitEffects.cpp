#include "Unit_C.h"
#include "IUnitEffects.h"

#include "Client.h"
#include "Component/CharacterCustomization.h"
#include "Component/Component.h"
#include "Console/ConsoleVar.h"
#include "DB/DBClient/AutoCode/ItemDisplayInfoRec.h"
#include "DB/DBClient/AutoCode/SpellRec.h"
#include "DB/DBClient/AutoCode/SpellVisualRec.h"
#include "DB/DBClient/AutoCode/SpellVisualKitRec.h"
#include "DB/DBClient/AutoCode/ChrRacesRec.h"
#include "DB/DBClient/AutoCode/SpellVisualEffectNameRec.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "SoundInterface/SoundInterface.h"
#include "UIUtil/Camera.h"
#include "WorldClient/World.h"

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

int __fastcall    DeathHoldEventTimerHandler(const void *packetData, void *param);
void __fastcall   SpellVisualsPlayCameraShakeID(unsigned int shakeID, const NTempest::C3Vector &position);
HMODEL __fastcall InitializeModel(const char *fileName, void(__fastcall *callback)(const char *, const NTempest::C3Vector &, void *), void *param);
static void __fastcall DecorateEffectFilename(const char *fileName, int raceSexSpecific, CGObject_C *object, char *buffer, unsigned int size);
static void __fastcall SpellUnitAnimEventCallback(const char *eventName, const NTempest::C3Vector &position, void *param);
static int __fastcall  OneShotEndHandler(void *param);
static void __fastcall SpellAreaAnimEventCallback(const char *eventName, const NTempest::C3Vector &position, void *param);
static int __fastcall  PurgeTimerHandler(const void *timerData, void *userData);
void __fastcall        UnitEffectOneShot(
    SpellVisualEffectNameRec       *effectRec,
    NTempest::C3Vector             &location,
    TSStackArray<unsigned __int64> *objects,
    float                           facing,
    float                           scale
);
void __fastcall PreloadModel(int effectID, CStatus *status);
void __fastcall PreloadModelsByKit(int record, CStatus *status);

static const char *s_sequenceNames[3] = {"Stand", "Hold", "Decay"};
static const char *s_objNames[1] = {"$DTH"};

HMODEL __fastcall CreateModel(const char *fileName, CStatus *status) {
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

void __fastcall PreloadModel(int effectID, CStatus *status) {
  const SpellVisualEffectNameRec *effect = g_spellVisualEffectNameDB.GetRecord(effectID);
  if (effect) {
    HMODEL model = CreateModel(effect->m_fileName, status);
    if (model) {
      HandleClose(model);
    }
  }
}

void __fastcall PreloadModelsByKit(int record, CStatus *status) {
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
  for (unsigned int i = 0; i < 3; ++i) {
    PreloadModel(kit->m_specialEffect[i], status);
  }
}

HMODEL __fastcall InitializeModel(const char *fileName, void(__fastcall *callback)(const char *, const NTempest::C3Vector &, void *), void *param) {
  CStatus status;
  HMODEL  model = CreateModel(fileName, &status);
  if (callback) {
    ModelSetEventCallback(model, callback, param, 0);
  }
  ModelSetSequence(model, 0, 0);
  SysMsgAdd(status, 16);
  return model;
}

class NODEBASE {
 public:
  NODEBASE() : model(0), flags(0), deathHoldTimer(0) {
  }
  virtual void ReleaseDeathHolds() = 0;
  ~NODEBASE();

  void         ClearDeathHoldTimer();
  unsigned int CheckModelLoadStatus();

  TSLink<NODEBASE> node;
  HMODEL__        *model;
  unsigned int     flags;
  unsigned int     deathHoldTimer;
};

class ONESHOTEFFECTNODE : public NODEBASE {
 public:
  ONESHOTEFFECTNODE() : objectModel(0), objectModelAttachmentPoint(0), objectGUID(0), spellID(0), isCastEffect(0) {
  }
  virtual void ReleaseDeathHolds();
  void         CheckModelLoadStatus();
  ~ONESHOTEFFECTNODE();

  HMODEL__        *objectModel;
  unsigned int     objectModelAttachmentPoint;
  unsigned __int64 objectGUID;
  int              spellID;
  unsigned int     isCastEffect;
};

class ONESHOTSTANDALONEEFFECTNODE : public NODEBASE {
 public:
  ONESHOTSTANDALONEEFFECTNODE() : facing(0.0f), scale(1.0f), worldObject(0), expireTime(0) {
  }
  virtual void ReleaseDeathHolds();
  void         CheckModelLoadStatus();
  ~ONESHOTSTANDALONEEFFECTNODE();

  NTempest::C3Vector             position;
  TSFixedArray<unsigned __int64> objects;
  float                          facing;
  float                          scale;
  unsigned long                  worldObject;
  int                            expireTime;
};

struct MISSILENODE : public TSLinkedNode<MISSILENODE> {
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
        missReason(MISS_REASON_NONE),
        flags(0),
        sound(0) {
  }
  ~MISSILENODE();
  void CheckModelLoadStatus();

  HMODEL             model;
  unsigned __int64   caster;
  NTempest::C3Vector startPosition;
  NTempest::C3Vector position;
  NTempest::C3Vector endPosition;
  NTempest::C3Vector normal;
  unsigned __int64   target;
  unsigned int       startTime;
  unsigned int       travelTime;
  NTempest::C3Vector facing;
  unsigned int       spellID;
  unsigned int       victimEffect;
  unsigned int       pathType;
  unsigned int       miss;
  MISS_REASON        missReason;
  int                flags;
  Sound             *sound;
};

struct UNITONESHOTEFFECTDESC : public TSHashObject<UNITONESHOTEFFECTDESC, CHashKeyGUID> {
  TSExplicitList<ONESHOTEFFECTNODE, 4> m_effects;
};

static TSHashTable<UNITONESHOTEFFECTDESC, CHashKeyGUID> s_oneShotEffects;
static TSExplicitList<ONESHOTSTANDALONEEFFECTNODE, 4>   s_standAloneEffects;
static TInstanceAllocator<ONESHOTSTANDALONEEFFECTNODE>  s_freeStandaloneEffects(40);
static TSList<MISSILENODE, TSGetLink<MISSILENODE> >     s_missiles;
static TInstanceAllocator<MISSILENODE>                  s_freeMissiles(10);
static CVar                                            *s_showEffectsStandalone;
static int                                              s_specialEffects[43];
static unsigned int                                     s_purgeTimer;
static int                                              s_purgeTime;

static const GEOCOMPONENTLINKS g_attachmentPoints[12] = {
    static_cast<GEOCOMPONENTLINKS>(19), static_cast<GEOCOMPONENTLINKS>(20), static_cast<GEOCOMPONENTLINKS>(21), static_cast<GEOCOMPONENTLINKS>(22),
    static_cast<GEOCOMPONENTLINKS>(-1), static_cast<GEOCOMPONENTLINKS>(17), static_cast<GEOCOMPONENTLINKS>(34), static_cast<GEOCOMPONENTLINKS>(23),
    static_cast<GEOCOMPONENTLINKS>(24), static_cast<GEOCOMPONENTLINKS>(25), static_cast<GEOCOMPONENTLINKS>(16), static_cast<GEOCOMPONENTLINKS>(15)
};

GEOCOMPONENTLINKS __fastcall UnitEffectGetLinkPointFromAttachment(UNITEFFECTATTACHPPOINT attach) {
  FATALASSERT(attach >= 0);
  FATALASSERT(static_cast<unsigned int>(attach) < sizeof(g_attachmentPoints) / sizeof(g_attachmentPoints[0]));
  return g_attachmentPoints[attach];
}

HMODEL __fastcall UnitEffectCreateAuraModel(unsigned int effectID) {
  SpellVisualEffectNameRec *effectRec = g_spellVisualEffectNameDB.GetRecord(effectID);
  if (effectRec) {
    return InitializeModel(effectRec->m_fileName, 0, 0);
  }

  SysMsgPrintf(SYSMSG_WARNING, 2, "SPELLEFFECTIDNOTFOUND|%d", effectID);
  return 0;
}

unsigned int __fastcall UnitEffectIsAuraWorldObject(unsigned int effectID, unsigned int &isWorldObj) {
  SpellVisualEffectNameRec *effectRec = g_spellVisualEffectNameDB.GetRecord(effectID);
  if (!effectRec) {
    return 0;
  }

  isWorldObj = (effectRec->m_VisualEffectNameFlags & 8) != 0;
  return 1;
}

unsigned long __fastcall UnitEffectCreateWorldModelAura(unsigned int effect, const NTempest::C3Vector &location, float facing) {
  if (!effect) {
    return 0;
  }

  SpellVisualEffectNameRec *effectRec = g_spellVisualEffectNameDB.GetRecord(effect);
  HMODEL                    model = InitializeModel(effectRec->m_fileName, 0, 0);
  if (!model) {
    return 0;
  }

  NTempest::C44Matrix tempMat;
  tempMat.Translate(location);
  NTempest::C3Vector axis(0.0f, 0.0f, 1.0f);
  tempMat.Rotate(facing, axis, 1);
  unsigned long object = CWorld::AddDoodad(effectRec->m_fileName, model, tempMat, 6);
  HandleClose(model);
  return object;
}

int __fastcall GetMissileTargetLocation(unsigned __int64 caster, unsigned int spellID) {
  SpellRec *spellRec = g_spellDB.GetRecord(spellID);
  if (!spellRec) {
    return 1;
  }
  CGObject_C     *casterObject = caster ? ClntObjMgrObjectPtr(caster, __FILE__, __LINE__) : 0;
  CGUnit_C       *casterUnit = casterObject && (casterObject->GetType() & TYPE_UNIT) ? static_cast<CGUnit_C *>(casterObject) : 0;
  SpellVisualRec  filled;
  SpellVisualRec *visual =
      casterUnit ? casterUnit->GetAppropriateSpellVisual(spellRec, filled) : g_spellVisualDB.GetRecord(spellRec->m_spellVisualID);
  return visual ? visual->m_missileDestinationAttachment : 1;
}

void __fastcall GetMissileTargetPosition(CGObject_C *target, int hitLocation, NTempest::C3Vector &position) {
  FATALASSERT(target);
  FATALASSERT(hitLocation <= 2);
  FATALASSERT(target->GetType() & TYPE_UNIT);

  HMODEL model = target->GetCharacterModel(0);
  FATALASSERT(model);
  NTempest::C3Vector modelPosition;
  if (ModelGetObjectPosition(model, 0, &modelPosition)) {
    position = target->GetPosition() + modelPosition;
  } else {
    position = target->GetPosition();
  }
  HandleClose(model);
}

static void RecycleMissileNode(MISSILENODE *node) {
  s_missiles.UnlinkNode(node);
  node->~MISSILENODE();
  s_freeMissiles.PutData(node, 0, 0);
}

static int MoveMissile(MISSILENODE *node) {
  CGObject_C *target = node->target ? ClntObjMgrObjectPtr(node->target, __FILE__, __LINE__) : 0;
  if (target && (target->GetType() & TYPE_UNIT)) {
    GetMissileTargetPosition(target, GetMissileTargetLocation(node->caster, node->spellID), node->endPosition);
  }

  unsigned int elapsed = GetTickCount() - node->startTime;
  if (elapsed >= node->travelTime) {
    if (target && (target->GetType() & TYPE_UNIT)) {
      CGUnit_C *unit = static_cast<CGUnit_C *>(target);
      if (!node->miss) {
        SpellVisualKitRec *kit = g_spellVisualKitDB.GetRecord(node->victimEffect);
        if (kit) {
          unit->PlayImpactKit(node->spellID, kit);
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
  endPos.z -= 5.0f;
  NTempest::C3Vector scanStart = node->position;
  scanStart.z += 5.0f;
  NTempest::C3Segment seg(scanStart, endPos);
  NTempest::C4Plane   facet;
  float               segT;
  if (CWorld::GetFacet(seg, segT, facet, 273)) {
    float ground = scanStart.z + (endPos.z - scanStart.z) * segT;
    if (node->pathType == 1) {
      node->position.z = ground;
      node->normal = facet.n;
    } else if (node->position.z - ground < 0.3f) {
      node->position.z = ground + 0.3f;
    }
  } else {
    node->normal.Set(0.0f, 0.0f, 1.0f);
  }

  NTempest::C3Vector movement = node->endPosition - node->startPosition;
  float              distance = movement.Mag();
  node->facing.z = CalculateFacingTo(node->startPosition, node->endPosition);
  node->facing.x = -atan2(node->endPosition.z - node->startPosition.z, distance);
  if (node->sound) {
    NTempest::C3Vector velocity = node->endPosition - node->position;
    node->sound->SetPosition(node->position, &velocity);
  }
  return 1;
}

static void RenderMissiles(CGCamera *camera) {
  if (!camera) {
    return;
  }
  MISSILENODE *node = s_missiles.Head();
  while (node) {
    MISSILENODE *nodenext_node = s_missiles.RawNext(node);
    node->CheckModelLoadStatus();
    if (MoveMissile(node)) {
      NTempest::C3Vector axis(0.0f, 0.0f, 1.0f);
      ModelAnimate(node->model, node->position, node->facing.z, axis, 1.0f, camera->Position(), camera->Forward());
      ModelAddToScene(node->model, 6);
    }
    node = nodenext_node;
  }
}

void __fastcall UnitEffectAddMissile(const MISSILESTRUCT &desc, int durationOffset) {
  NTempest::C3Vector endPos;
  CGObject_C        *target = desc.target ? ClntObjMgrObjectPtr(desc.target, __FILE__, __LINE__) : 0;
  if (desc.target && !target) {
    return;
  }
  unsigned __int64 caster = desc.caster ? desc.caster->GetGUID() : 0;
  if (target) {
    GetMissileTargetPosition(target, GetMissileTargetLocation(caster, desc.spellID), endPos);
  } else {
    endPos = desc.destination;
  }

  char                      modelName[MAX_PATH];
  HMODEL                    model = 0;
  unsigned int              dummy1 = 0;
  SpellVisualEffectNameRec *effectRec = g_spellVisualEffectNameDB.GetRecord(desc.missileEffect);
  if (effectRec && effectRec->m_fileName && *effectRec->m_fileName) {
    DecorateEffectFilename(effectRec->m_fileName, 0, desc.caster, modelName, sizeof(modelName));
    model = InitializeModel(modelName, 0, 0);
  } else if (desc.ammoDisplayID) {
    model = ObjComponentBuildAmmoModel(g_itemDisplayInfoDB.GetRecord(desc.ammoDisplayID), desc.inventoryType, dummy1);
  }
  if (!model) {
    return;
  }

  MISSILENODE *node = static_cast<MISSILENODE *>(s_freeMissiles.GetData(0, typeid(MISSILENODE).raw_name(), -2));
  if (node) {
    new (node) MISSILENODE;
  }
  s_missiles.LinkNode(node, LIST_HEAD, 0);
  node->model = model;
  node->caster = caster;
  node->startPosition = desc.startPosition;
  node->pathType = desc.missilePathType;
  if (node->pathType == 1) {
    NTempest::C3Vector scanStart = node->startPosition;
    scanStart.z += 5.0f;
    NTempest::C3Vector scanEnd = node->startPosition;
    scanEnd.z -= 25.0f;
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
  node->startTime = GetTickCount();
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

int __fastcall DeathHoldEventTimerHandler(const void *packetData, void *param) {
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

unsigned int NODEBASE::CheckModelLoadStatus() {
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
  if (unitPtr) {
    unitPtr->DDADDLOG(unitPtr->GetGUID(), "UnitEffectOneShot", __FILE__, __LINE__);
  }
}

void ONESHOTEFFECTNODE::CheckModelLoadStatus() {
  if (NODEBASE::CheckModelLoadStatus() && ModelAnimHasObjectId(model, 0)) {
    CGObject_C *object = ClntObjMgrObjectPtr(objectGUID, __FILE__, __LINE__);
    if (object && (object->GetType() & TYPE_UNIT)) {
      AddUnitDeathHold(static_cast<CGUnit_C *>(object));
    }
  }
}

void ONESHOTSTANDALONEEFFECTNODE::CheckModelLoadStatus() {
  if (NODEBASE::CheckModelLoadStatus() && ModelAnimHasObjectId(model, 0)) {
    for (unsigned int index = objects.Count(); index; --index) {
      CGObject_C *object = ClntObjMgrObjectPtr(objects[index - 1], __FILE__, __LINE__);
      if (object && (object->GetType() & TYPE_UNIT)) {
        AddUnitDeathHold(static_cast<CGUnit_C *>(object));
      }
    }
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
    for (unsigned int index = 0; index < objects.Count(); ++index) {
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
    unsigned int duration;
    if (ModelGetSequenceDuration(model, 0, &duration)) {
      unsigned int finishTime = startTime + travelTime;
      unsigned int currentTime = GetTickCount();
      if (finishTime > currentTime) {
        ModelSetTimeScale(model, static_cast<float>(duration) / static_cast<float>(finishTime - currentTime), 1);
      }
    }
  }
}

static void __fastcall DecorateEffectFilename(const char *fileName, int raceSexSpecific, CGObject_C *object, char *buffer, unsigned int size) {
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

  CGUnit_C    *unit = static_cast<CGUnit_C *>(object);
  unsigned int sex = unit->GetDisplaySex();
  unsigned int race = unit->GetDisplayRace();
  FATALASSERT(sex < UNITSEX_LAST);
  ChrRacesRec *raceRec = g_chrRacesDB.GetRecord(race);
  FATALASSERT(raceRec);

  static const char *const sexNames[UNITSEX_LAST] = {"Male", "Female", "NOSEX"};
  SStrPrintf(buffer, size, "%s%s%s%s", scratchBuffer, raceRec->m_clientFileString, sexNames[sex], extensionString);
}

void __fastcall UnitEffectsInitialize() {
  s_showEffectsStandalone = CVar::Register("showEffectsStandalone", 0, 0, "1", 0, 5, false, 0);
}

void __fastcall UnitEffectsShutdown() {
  while (reinterpret_cast<long>(s_missiles.Head()) > 0) {
    MISSILENODE *node = s_missiles.Head();
    node->~MISSILENODE();
    s_freeMissiles.PutData(node, 0, 0);
  }

  while (reinterpret_cast<long>(s_standAloneEffects.Head()) > 0) {
    ONESHOTSTANDALONEEFFECTNODE *node = s_standAloneEffects.Head();
    node->~ONESHOTSTANDALONEEFFECTNODE();
    s_freeStandaloneEffects.PutData(node, 0, 0);
  }

  if (s_purgeTimer) {
    ClientKillTimer(s_purgeTimer, PurgeTimerHandler, "PurgeTimerHandler");
    s_purgeTimer = 0;
  }
}

void __fastcall UnitEffectUpdate(CGCamera *camera) {
  if (s_showEffectsStandalone->GetInt()) {
    RenderMissiles(camera);
  }

  UNITONESHOTEFFECTDESC *effectDesc = s_oneShotEffects.Head();
  while (reinterpret_cast<long>(effectDesc) > 0) {
    ONESHOTEFFECTNODE *node = effectDesc->m_effects.Head();
    while (reinterpret_cast<long>(node) > 0) {
      node->CheckModelLoadStatus();
      node = effectDesc->m_effects.RawNext(node);
    }
    effectDesc = s_oneShotEffects.RawNext(effectDesc);
  }

  ONESHOTSTANDALONEEFFECTNODE *standalone = s_standAloneEffects.Head();
  while (reinterpret_cast<long>(standalone) > 0) {
    standalone->CheckModelLoadStatus();
    standalone = s_standAloneEffects.RawNext(standalone);
  }
}

static void __fastcall SpellSoundEffectCallback(const char *eventName, const NTempest::C3Vector &position) {
  if (eventName && *eventName) {
    SndInterfacePlaySound(SStrToInt(eventName), position, -1, 1.0f);
  }
}

static void __fastcall SpellCameraShakeCallback(const char *eventName, const NTempest::C3Vector &position) {
  if (eventName && *eventName) {
    SpellVisualsPlayCameraShakeID(SStrToInt(eventName), position);
  }
}

static void __fastcall SpellAreaAnimEventCallback(const char *eventName, const NTempest::C3Vector &position, void *param) {
  ONESHOTSTANDALONEEFFECTNODE *node = static_cast<ONESHOTSTANDALONEEFFECTNODE *>(param);
  unsigned int                 event = *reinterpret_cast<const unsigned int *>(eventName);

  switch (event) {
    case 0x4B485324:  // $SHK
      SpellCameraShakeCallback(eventName + 1, position);
      break;
    case 0x48544424:  // $DTH
      if (node) {
        node->ReleaseDeathHolds();
      }
      break;
    case 0x444E5324:    // $SND
    case 0x58444E53: {  // SNDX
      NTempest::C3Vector soundPos = position;
      if (node) {
        soundPos += node->position;
      }
      SpellSoundEffectCallback(eventName + 1, soundPos);
      break;
    }
    case 0x54494824:  // $HIT
      if (node) {
        for (unsigned int index = 0; index < node->objects.Count(); ++index) {
          CGObject_C *object = ClntObjMgrObjectPtr(node->objects[index], __FILE__, __LINE__);
          if (object && (object->GetType() & TYPE_UNIT)) {
            static_cast<CGUnit_C *>(object)->PlayDeathThud();
          }
        }
      }
      break;
    default:
      SysMsgPrintf(SYSMSG_WARNING, 16, "UNKNOWNANIMEVENT|%s|ONESHOTSTANDALONEEFFECTNODE|SpellAreaAnimEventCallback", eventName);
      break;
  }
}

static void __fastcall SpellUnitAnimEventCallback(const char *eventName, const NTempest::C3Vector &position, void *param) {
  ONESHOTEFFECTNODE *node = static_cast<ONESHOTEFFECTNODE *>(param);
  unsigned int       event = *reinterpret_cast<const unsigned int *>(eventName);

  CGObject_C *object = node ? ClntObjMgrObjectPtr(node->objectGUID, __FILE__, __LINE__) : 0;
  CGUnit_C   *unit = object && (object->GetType() & TYPE_UNIT) ? static_cast<CGUnit_C *>(object) : 0;

  switch (event) {
    case 0x50504324:  // $CPP
    case 0x48414324:  // $ACH
    case 0x53534324:  // $CSS
      if (unit) {
        NTempest::C3Vector eventPosition = position;
        unit->HandleCombatAnimEvent(eventName, event, eventPosition);
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
      if (object) {
        soundPos += object->GetPosition();
      }
      SpellSoundEffectCallback(eventName + 1, soundPos);
      break;
    }
    case 0x54494824:  // $HIT
      if (unit) {
        unit->PlayDeathThud();
      }
      break;
    default:
      SysMsgPrintf(SYSMSG_WARNING, 16, "UNKNOWNANIMEVENT|%s|ONESHOTEFFECTNODE|SpellUnitAnimEventCallback", eventName);
      break;
  }
}

static int __fastcall OneShotEndHandler(void *param) {
  FATALASSERT(param);

  ONESHOTEFFECTNODE *node = static_cast<ONESHOTEFFECTNODE *>(param);
  if (node->isCastEffect) {
    delete node;
  }
  return 0;
}

void __fastcall UnitEffectOneShot(
    const SpellVisualEffectNameRec *effect,
    CGObject_C                     *object,
    UNITEFFECTATTACHPPOINT          attachPoint,
    int                             spellID,
    unsigned int                    isCastEffect,
    unsigned int                    forceEffectOnMount
) {
  if (!effect) {
    return;
  }

  FATALASSERT(static_cast<unsigned int>(attachPoint) < sizeof(g_attachmentPoints) / sizeof(g_attachmentPoints[0]));

  unsigned __int64       guid = object->GetGUID();
  CHashKeyGUID           key(guid);
  UNITONESHOTEFFECTDESC *unitEffectDesc = s_oneShotEffects.Ptr(static_cast<unsigned int>(guid), key);
  if (!unitEffectDesc) {
    unitEffectDesc = s_oneShotEffects.New(static_cast<unsigned int>(guid), key, 0, 0);
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
    linkPoint = static_cast<GEOCOMPONENTLINKS>(15);
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

  ONESHOTEFFECTNODE *newEffectNode = new ONESHOTEFFECTNODE;
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
    newEffectNode->objectGUID = guid;
    if (!(effect->m_VisualEffectNameFlags & 4)) {
      ModelSetSeqFinishedHandler(model, 0, OneShotEndHandler, newEffectNode);
      HandleClose(objectModel);
      return;
    }
  }

  if (model) {
    HandleClose(model);
  }
  HandleClose(heldObjectModel);
  unitEffectDesc->m_effects.DeleteNode(newEffectNode);
  HandleClose(objectModel);
}

static void __fastcall CheckReinitTimer(int current, unsigned int duration) {
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

static int __fastcall PurgeTimerHandler(const void *timerData, void *userData) {
  s_purgeTimer = 0;

  int                          current = GetTickCount();
  int                          next = 0x7FFFFFFF;
  int                          found = 0;
  ONESHOTSTANDALONEEFFECTNODE *node = s_standAloneEffects.Head();
  while (node) {
    ONESHOTSTANDALONEEFFECTNODE *nextNode = s_standAloneEffects.RawNext(node);
    if (node->expireTime > current) {
      if (next >= node->expireTime) {
        next = node->expireTime;
      }
      ++found;
    } else {
      s_standAloneEffects.UnlinkNode(node);
      node->~ONESHOTSTANDALONEEFFECTNODE();
      s_freeStandaloneEffects.PutData(node, 0, 0);
    }
    node = nextNode;
  }

  if (found) {
    CheckReinitTimer(current, next - current);
  }
  return 1;
}

void __fastcall UnitEffectOneShot(
    SpellVisualEffectNameRec       *effectRec,
    NTempest::C3Vector             &location,
    TSStackArray<unsigned __int64> *objects,
    float                           facing,
    float                           scale
) {
  if (!effectRec || !s_showEffectsStandalone->GetInt()) {
    return;
  }

  ONESHOTSTANDALONEEFFECTNODE *unitEffectDesc =
      static_cast<ONESHOTSTANDALONEEFFECTNODE *>(s_freeStandaloneEffects.GetData(0, typeid(ONESHOTSTANDALONEEFFECTNODE).raw_name(), -2));
  if (unitEffectDesc) {
    new (unitEffectDesc) ONESHOTSTANDALONEEFFECTNODE;
  }

  HMODEL model = InitializeModel(effectRec->m_fileName, SpellAreaAnimEventCallback, unitEffectDesc);
  if (!model) {
    unitEffectDesc->~ONESHOTSTANDALONEEFFECTNODE();
    s_freeStandaloneEffects.PutData(unitEffectDesc, 0, 0);
    return;
  }

  unsigned int duration = 0;
  if (!ModelIsLoaded(model, 1) || !ModelGetSequenceDuration(model, 0, &duration) || !duration) {
    HandleClose(model);
    unitEffectDesc->~ONESHOTSTANDALONEEFFECTNODE();
    s_freeStandaloneEffects.PutData(unitEffectDesc, 0, 0);
    return;
  }

  int currentTime = GetTickCount();
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

void __fastcall UnitEffectClearSpellPrecast(CGObject_C *object, int spellID) {
  if (!object) {
    return;
  }

  unsigned __int64       guid = object->GetGUID();
  CHashKeyGUID           key(guid);
  UNITONESHOTEFFECTDESC *effectDesc = s_oneShotEffects.Ptr(static_cast<unsigned int>(guid), key);
  if (!effectDesc) {
    return;
  }

  ONESHOTEFFECTNODE *nodenext_node;
  for (ONESHOTEFFECTNODE *node = effectDesc->m_effects.Head(); node; node = nodenext_node) {
    nodenext_node = effectDesc->m_effects.RawNext(node);
    if (node->spellID == spellID && !node->isCastEffect) {
      effectDesc->m_effects.DeleteNode(node);
    }
  }
}

int __fastcall UnitEffectGetSpecialVisual(UNITEFFECTSPECIALS effectNumber) {
  return s_specialEffects[effectNumber];
}

void __fastcall UnitEffectOneShot(
    UNITEFFECTSPECIALS        effectNumber,
    unsigned __int64          target,
    const NTempest::C3Vector *attachPos,
    float                     facing,
    float                     scale,
    bool                      forceEffectOnMount
) {
  if (target && effectNumber < 43) {
    CGObject_C *object = ClntObjMgrObjectPtr(target, __FILE__, __LINE__);
    if (object) {
      int                       effectID = s_specialEffects[effectNumber];
      SpellVisualEffectNameRec *effectRec = g_spellVisualEffectNameDB.GetRecord(effectID);
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

void __fastcall UnitEffectPreloadSpellEffects(int spellID) {
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
