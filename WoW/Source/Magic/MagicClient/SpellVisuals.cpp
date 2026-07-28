#include "DB/DBClient/AutoCode/SpellAuraNamesRec.h"
#include "DayNight.h"
#include "DB/DBClient/AutoCode/SpellEffectCameraShakesRec.h"
#include "DB/DBClient/AutoCode/SpellChainEffectsRec.h"
#include "DB/DBClient/AutoCode/SpellDurationRec.h"
#include "DB/DBClient/AutoCode/SpellRec.h"
#include "DB/DBClient/AutoCode/SpellVisualEffectNameRec.h"
#include "DB/DBClient/AutoCode/SpellVisualAnimNameRec.h"
#include "DB/DBClient/AutoCode/SpellVisualKitRec.h"
#include "DB/DBClient/AutoCode/SpellVisualPrecastTransitionsRec.h"
#include "DB/DBClient/AutoCode/SpellVisualRec.h"
#include "Client.h"
#include "Object/ObjectClient/AnimCompiles.h"
#include "Object/ObjectClient/Unit_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Services/Lightning.h"
#include "Services/SysMessage.h"
#include "Services/Texture.h"
#include <Os/OsTime.h>
#include "Gx/Gx.h"
#include "Model/IModel.h"
#include "SoundInterface/SoundInterface.h"
#include "Tempest/c34matrix.h"
#include "Tempest/caabox.h"
#include "Tempest/cmath.h"
#include "Tempest/caasphere.h"
#include "Ui/WorldFrame.h"
#include "WorldClient/World.h"

#include <Base/Status.h>
#include <Base/CDataAllocator.h>
#include <stpl.h>
#include <string.h>

class CDataStore;

struct SPELLVISUALNODE : public TSHashObject<SPELLVISUALNODE, HASHKEY_NONE> {
  SPELLVISUALNODE();
  SPELLVISUALNODE(const SPELLVISUALNODE &);

  unsigned int    m_effects[15];
  ANIMENUMERATION m_anims[2];
};

SPELLVISUALNODE::SPELLVISUALNODE() {
  memset(m_effects, 0, sizeof(m_effects));
  m_anims[0] = RESET_ANIMATION_INDICES0;
  m_anims[1] = RESET_ANIMATION_INDICES0;
}

NODEDECL(BlizzardObject) {
  NODEDECL(Shard) {
    NTempest::C3Vector pos;
    HMODEL             hModel;
    unsigned long      startTime;

    void Init() {
      hModel = 0;
    }
  };

  static LISTDECL(Shard, shardPool);

  HMODEL                           shardModel;
  unsigned long                    hWorldObject;
  NTempest::C3Vector               groundPos;
  float                            radius;
  float                            numEmitted;
  float                            emissionRate;
  unsigned int                     dead;
  NTempest::CAaSphere              boundSphere;
  LISTDECL(Shard, shards);

  static int ShardSeqFinished(void *param);
  static Shard          *AllocShard();
  static void            FreeShard(Shard *&shard);
  void                   UpdateBounds();
  static void WorldObjectRender(void *param, const NTempest::C44Matrix &mtx);
  void                   Init(const NTempest::C3Vector &pos, const char *modelName, float pRadius, float pEmissionRate);
  void                   Destroy();
  void                   Update();
  void                   Render(const NTempest::C44Matrix &mtx);
};

NODEDECL(LightningObject) {
 public:
  struct Bolt {
    enum {
      NULL_SUB = 0xFFFF
    };

    unsigned short srcGuidSub;
    unsigned short dstGuidSub;
    unsigned int   birthTime;
    unsigned int   deathTime;
    BoltID         boltID;
  };

  LightningObject();
  ~LightningObject();
  void         AddRef();
  void         DelRef();
  bool Tick(unsigned int currentTime);

  TSGrowableArray<unsigned __int64> guids;
  TSGrowableArray<Bolt>             bolts;
  unsigned int                      deathTime;
  float                             avgSegLen;
  float                             width;
  float                             noiseScale;
  float                             texCoordScale;
  float                             duration;
  HTEXTURE                          texture;
  int                               spellID;
  unsigned int                      forever;

 private:
  int refCount;
};

struct EclipseObject {
  NTempest::CImVector color;
  unsigned int        startTime;
  unsigned int        fadeInTime;
  unsigned int        fadeOutTime;
  unsigned int        endTime;

  void Update(unsigned int currentTime);
};

NODEDECL(FishingLineObject) {
  unsigned __int64    object;
  unsigned __int64    caster;
  NTempest::CImVector color;
  bool                visible;

  void Render();
  void RenderLine(const NTempest::C3Vector &p0, const NTempest::C3Vector &p1, const NTempest::CImVector &color);
};

static void ShardEventCallback(const char *eventName, const NTempest::C3Vector &position, void *param);
static void FreeBlizzard(BlizzardObject *bliz);
static inline void             RenderFishingLines();
static bool GetFishingLineStartPos(HMODEL model, NTempest::C3Vector &pos);
SPELL_VISUAL_ATTACHMENT GetMissileTargetLocation(unsigned __int64 caster, unsigned int spellID);
void GetMissileTargetPosition(CGObject_C *target, SPELL_VISUAL_ATTACHMENT hitLocation, NTempest::C3Vector &position);
int Spell_C_GetCastTime(int id, int isPet);

LISTDECL(BlizzardObject::Shard, BlizzardObject::shardPool);

static LISTDECL(FishingLineObject, s_fishingLineObjects);
static TInstanceAllocator<FishingLineObject>                    s_freeFishingObjects(20);
static TSCArray<unsigned short, 201>                            s_fishingLineIndices;
static TSCArray<float, 201>                                     s_segmentPoints;

LightningObject::LightningObject() : refCount(1) {
}

int BlizzardObject::ShardSeqFinished(void *param) {
  return 0;
}

BlizzardObject::Shard *BlizzardObject::AllocShard() {
  if (!shardPool.Head()) {
    shardPool.NewNode(2, 0, 0);
  }

  Shard *shard = shardPool.Head();
  shardPool.UnlinkNode(shard);
  shard->Init();
  return shard;
}

void BlizzardObject::FreeShard(Shard *&shard) {
  shardPool.LinkNode(shard, 2, 0);
  shard = 0;
}

void BlizzardObject::WorldObjectRender(void *param, const NTempest::C44Matrix &mtx) {
  static_cast<BlizzardObject *>(param)->Render(mtx);
}

void BlizzardObject::Init(const NTempest::C3Vector &pos, const char *modelName, float pRadius, float pEmissionRate) {
  NTempest::C44Matrix mtx;
  NTempest::CAaBox    aaBox;
  NTempest::CAaSphere sphere;
  NTempest::C3Vector  corner;
  float               bounds;

  dead = 0;
  numEmitted = 0.0f;
  groundPos = pos;
  radius = pRadius;
  emissionRate = pEmissionRate;
  shardModel = ObjectModelCreate(modelName, TYPE_OBJECT, 0x100800);

  mtx.d0 = groundPos.x;
  mtx.d1 = groundPos.y;
  mtx.d2 = groundPos.z;
  hWorldObject = CWorld::AddDoodad(0, 0, mtx, 2);
  CWorld::SetObjectRenderCallback(hWorldObject, WorldObjectRender, this);

  sphere.c = NTempest::C3Vector(0.0f, 0.0f, 0.0f);
  sphere.r = 0.0f;
  bounds = ModelGetBounds(shardModel, &sphere) ? sphere.r + pRadius : pRadius;
  corner = NTempest::C3Vector(-bounds, -bounds, -bounds);
  aaBox.b = corner;
  aaBox.t = NTempest::C3Vector(bounds, bounds, bounds);
  CWorld::UpdateObject(hWorldObject, mtx, aaBox);
}

void BlizzardObject::Destroy() {
  HandleClose(shardModel);
  shardModel = 0;
}

static void ShardEventCallback(const char *eventName, const NTempest::C3Vector &position, void *param) {
  static unsigned int counter;

  ++counter;
  if ((counter & 1) && *reinterpret_cast<const unsigned int *>(eventName) == 0x444E5324) {
    SndInterfacePlayInterfaceSound(eventName + 4);
  }
}

static inline void RenderFishingLines() {
  ITERATELIST(FishingLineObject, s_fishingLineObjects, object) {
    object->Render();
  }
}

void BlizzardObject::Update() {
  NTempest::C3Vector a;
  NTempest::C3Vector b;
  float              groundT;
  Shard             *shard;

  if (dead) {
    if (!shards.Head()) {
      CWorld::RemoveObject(hWorldObject);
      Destroy();
      FreeBlizzard(this);
      return;
    }
  } else {
    numEmitted += CWorld::GetTickTimeSec() * emissionRate;
    while (numEmitted >= 1.0f) {
      if (ModelIsLoaded(shardModel, 1)) {
        shard = AllocShard();
        shards.LinkNode(shard, 2, 0);
        shard->pos = groundPos + NTempest::CRandom::C3Vector_(g_rndSeed) * (NTempest::CRandom::real_(g_rndSeed) * radius);
        shard->pos.z = groundPos.z + 50.0f;

        a = shard->pos;
        b = NTempest::C3Vector(shard->pos.x, shard->pos.y, groundPos.z - 50.0f);
        groundT = 1.0f;
        if (!CWorld::Intersect(&a, &b, 0.0f, &shard->pos, &groundT, 273)) {
          shard->pos = groundPos;
        }

        shard->hModel = ModelDuplicate(shardModel, 0);
        ModelSetSequence(shard->hModel, 0, 0);
        ModelSetEventCallback(shard->hModel, ShardEventCallback, 0, 0);
        ModelSetSeqFinishedHandler(shard->hModel, ShardSeqFinished, shard);
        shard->startTime = CWorld::GetCurTimeMs() + 2 * NTempest::CMath::ftol_0_256_(NTempest::CRandom::real_(g_rndSeed) * 255.0f);
      }
      numEmitted -= 1.0f;
    }
  }

  shard = shards.Head();
  while (shard) {
    Shard *next = shards.Next(shard);
    if (CWorld::GetCurTimeMs() >= shard->startTime && !ModelAdvanceTime(shard->hModel)) {
      HandleClose(shard->hModel);
      FreeShard(shard);
    }
    shard = next;
  }
}

void BlizzardObject::Render(const NTempest::C44Matrix &mtx) {
  NTempest::C34Matrix transform;

  ITERATELIST(Shard, shards, shard) {
    if (CWorld::GetCurTimeMs() >= shard->startTime) {
      transform.Translate(shard->pos - CWorld::GetCamPos());
      ModelAnimate(shard->hModel, transform, 1.0f, CWorld::GetCamPos(), CWorld::GetCamTarget() - CWorld::GetCamPos());
      transform = NTempest::C34Matrix(mtx.a0, mtx.a1, mtx.a2, mtx.b0, mtx.b1, mtx.b2, mtx.c0, mtx.c1, mtx.c2, mtx.d0, mtx.d1, mtx.d2);
      ModelProcessEvents(shard->hModel, transform);
      ModelAddToScene(shard->hModel, 0);
    }
  }
}

static bool GetFishingLineStartPos(HMODEL model, NTempest::C3Vector &pos) {
  FATALASSERT(model);

  HMODEL       attached = 0;
  unsigned int size = 1;
  if (!ModelGetLinkPoint(model, 1, &attached, &size) || !attached) {
    return false;
  }

  bool result = ModelGetEventObjectPosition(attached, 2, 0, &pos) != 0;
  HandleClose(attached);
  return result;
}

void FishingLineObject::Render() {
  if (!visible) {
    return;
  }

  CGObject_C *gameObject = ClntObjMgrObjectPtr(object, __FILE__, __LINE__);
  if (!gameObject || !gameObject->IsObjectModelLoaded()) {
    return;
  }

  CGObject_C *casterObject = ClntObjMgrObjectPtr(caster, __FILE__, __LINE__);
  if (!casterObject) {
    return;
  }

  HMODEL objectModel = gameObject->GetCharacterModel(0);
  if (!objectModel) {
    return;
  }

  HMODEL casterModel = casterObject->GetCharacterModel(0);
  if (!casterModel) {
    HandleClose(objectModel);
    return;
  }

  NTempest::C3Vector casterAnchorPos;
  NTempest::C3Vector objectAnchorPos;
  if (GetFishingLineStartPos(casterModel, casterAnchorPos)) {
    if (!ModelGetEventObjectPosition(objectModel, 2, 0, &objectAnchorPos)) {
      objectAnchorPos = gameObject->GetPosition() - CGWorldFrame::GetActiveCamera()->Position();
    }
    RenderLine(casterAnchorPos, objectAnchorPos, color);
  }

  HandleClose(objectModel);
  HandleClose(casterModel);
}

void FishingLineObject::RenderLine(const NTempest::C3Vector &p0, const NTempest::C3Vector &p1, const NTempest::CImVector &color) {
  NTempest::C3Vector points[201];
  NTempest::C3Vector point0 = p0;
  NTempest::C3Vector xyIncr = (p1 - p0) * 0.005f;
  float              maxDip = (p0 - p1).Mag() * 0.05f;

  for (unsigned int i = 0; i < 201; ++i) {
    points[i].x = point0.x;
    points[i].y = point0.y;
    points[i].z = point0.z + maxDip * s_segmentPoints[i];
    point0 += xyIncr;
  }

  GxVertexShaderSelect(GxVS_PassThru);
  GxRsPush();
  GxRsSet(GxRs_DepthTest, 1);
  GxRsSet(GxRs_DepthWrite, 1);
  GxPrimLockVertexPtrs(201, points, sizeof(NTempest::C3Vector), 0, 0, &color, 0, 0, 0, 0, 0, 0, 0);
  GxPrimDrawElements(GxPrim_LineStrip, s_fishingLineIndices.Count(), s_fishingLineIndices.Ptr());
  GxPrimUnlockVertexPtrs();
  GxRsPop();
}

static const char *modelNames[4] = {
    "Spells\\Blizzard_Impact_Base.mdx", "Spells\\RainOfFire_Impact_Base.mdx", "Spells\\CallLightning_Impact.mdx",
    "Spells\\FlamestrikeSmall_Impact_Base.mdx"
};
static LISTDECL(BlizzardObject, s_blizzardPool);
static LISTDECL(BlizzardObject, s_blizzard);
static TSGrowableArray<const SpellAuraNamesRec *>           s_auraNames;
static TSFixedArray<ANIMENUMERATION>                        s_precastAnimTransitions;
static LISTDECL(LightningObject, s_lightning);
static CLightningManager                                   *s_lightningManager;
static EclipseObject                                        s_eclipseObject;

class SpellCast {
 public:
  SpellCast() {
    caster = 0;
    spellID = 0;
    castTime = 0;
    targets = 0;
    castEndTime = 0;
    unitTarget = 0;
    itemTarget = 0;
    ammoItem = 0;
    spellLevel = 0;
    spellIndex = 0;
    reflector = 0;
    overrideRank = -1;
    flags = 0;
    selectedTarget = 0;
  }

  ~SpellCast() {
  }

  void BuildFullZoneUpdate(CDataStore *msg);
  void UnpackFullZoneUpdate(CDataStore *msg);

  unsigned __int64   caster;
  unsigned __int64   casterUnit;
  int                spellID;
  unsigned short     targets;
  unsigned __int64   unitTarget;
  unsigned __int64   itemTarget;
  unsigned __int64   selectedTarget;
  NTempest::C3Vector sourceLocation;
  NTempest::C3Vector destLocation;
  float              destFacing;
  unsigned int       destZoneID;
  unsigned int       castTime;
  unsigned int       castEndTime;
  int                spellIndex;
  unsigned int       spellLevel;
  unsigned __int64   ammoItem;
  unsigned __int64   reflector;
  char               targetString[128];
  int                overrideRank;
  unsigned short     flags;
};

static void CreateLightningObj(
    const CGUnit_C          *unitPtr,
    const unsigned __int64  *guids,
    int                      numGuids,
    int                      spellID,
    const SpellVisualKitRec *kitRec,
    LightningObject        **objects,
    int                      maxObjects
);

void UnitEffectOneShot(
    const SpellVisualEffectNameRec *effect,
    CGObject_C                     *object,
    UNITEFFECTATTACHPPOINT          attachPoint,
    int                             spellID,
    bool                            isCastEffect,
    bool                            forceEffectOnMount
);
void UnitEffectOneShot(
    const SpellVisualEffectNameRec *effect,
    const NTempest::C3Vector &location,
    const TSStackArray<unsigned __int64> *objects,
    float facing,
    float scale
);
void SpellVisualsProcedure(
    CGUnit_C                             *caster,
    const SpellVisualKitRec              *kitRec,
    unsigned int                          spellID,
    const TSStackArray<unsigned __int64> *targets,
    const TSStackArray<MISS_REASON>      *missReasons
);
bool IsSpellAura(const SpellRec *rec);
bool Object_C_AnimHasHitEvent(int anim);
void UnitCombatLogSpellMissed(
    unsigned int reason, unsigned int spellID,
    unsigned __int64 caster, unsigned __int64 target);
void SpellVisualsPlayCameraShakeID(unsigned int shakeID, const NTempest::C3Vector &position);
void UnitCombatLogCastStart(unsigned int spellID, unsigned __int64 caster);

void EclipseObject::Update(unsigned int currentTime) {
  if (startTime == endTime) {
    return;
  }

  float amount = 0.0f;
  if (currentTime < fadeInTime) {
    amount = static_cast<float>(currentTime - startTime) / static_cast<float>(fadeInTime - startTime);
  } else if (currentTime < fadeOutTime) {
    amount = 1.0f;
  } else if (currentTime < endTime) {
    amount = 1.0f - static_cast<float>(currentTime - fadeOutTime) / static_cast<float>(endTime - fadeOutTime);
  } else {
    endTime = 0;
    startTime = 0;
  }
  DayNightSetEclipse(color, amount);
}

static BlizzardObject *AllocBlizzard() {
  if (!s_blizzardPool.Head()) {
    s_blizzardPool.NewNode(2, 0, 0);
  }

  BlizzardObject *bliz = s_blizzardPool.Head();
  s_blizzard.LinkNode(bliz, 2, 0);
  return bliz;
}

static void FreeBlizzard(BlizzardObject *bliz) {
  s_blizzardPool.LinkNode(bliz, 2, 0);
}

static void InitializeAuraNames() {
  int index;

  s_auraNames.SetCount(89);
  memset(s_auraNames.Ptr(), 0, 89 * sizeof(SpellAuraNamesRec *));

  for (index = g_spellAuraNamesDB.GetNumRecords() - 1; index >= 0; --index) {
    const SpellAuraNamesRec *auraName = g_spellAuraNamesDB.GetRecordByIndex(index);
    unsigned int             enumID = auraName->m_EnumID;

    if (enumID < s_auraNames.Count()) {
      ASSERT(!s_auraNames[enumID]);
      s_auraNames[enumID] = auraName;
    }
  }

  for (index = static_cast<int>(s_auraNames.Count()) - 1; index >= 0; --index) {
    ASSERT(s_auraNames[index]);
  }
}

void PlayOneShotEffect(CGObject_C *object, int effectID, UNITEFFECTATTACHPPOINT attach, int spellID, bool isCastEffect) {
  if (effectID) {
    FATALASSERT(object);
    FATALASSERT(attach < NUM_UNITEFFECTATTACHPOINTS);
    const SpellVisualEffectNameRec *effectRec = g_spellVisualEffectNameDB.GetRecord(effectID);
    UnitEffectOneShot(effectRec, object, attach, spellID, isCastEffect, 0);
  }
}

static void InitializeFishingLineIntervals() {
  float        current = 0.0f;
  unsigned int index;

  for (index = 0; index < s_segmentPoints.MaxCount(); ++index) {
    s_segmentPoints[index] = -NTempest::CMath::sin_(3.1415927f * (current < 0.0f ? 0.0f : (current > 1.0f ? 1.0f : current)));
    current += 0.005f;
  }
}

static void InitializeFishingLineIndices() {
  unsigned int index;

  for (index = 0; index < s_fishingLineIndices.MaxCount(); ++index) {
    s_fishingLineIndices[index] = static_cast<unsigned short>(index);
  }
}

void SpellVisualsInitialize() {
  TSGrowableArray<int> animCheck;
  int                  i;
  unsigned int         map;
  SpellVisualKitRec   *constKit;
  unsigned int         found;

  InitializeAuraNames();
  InitializeFishingLineIntervals();
  InitializeFishingLineIndices();

  animCheck.SetCount(g_spellVisualAnimNameDB.GetNumRecords());
  for (i = 0; i < static_cast<int>(animCheck.Count()); ++i) {
    animCheck[i] = -1;
  }

  for (i = 0; i < g_spellVisualKitDB.GetNumRecords(); ++i) {
    constKit = const_cast<SpellVisualKitRec *>(g_spellVisualKitDB.GetRecordByIndex(i));
    int animNameIndex;
    found = 0;

    if (constKit->m_anim <= 0) {
      constKit->m_anim = -1;
      continue;
    }

    for (animNameIndex = 0; animNameIndex < g_spellVisualAnimNameDB.GetNumRecords(); ++animNameIndex) {
      const SpellVisualAnimNameRec *animName = g_spellVisualAnimNameDB.GetRecordByIndex(animNameIndex);

      if (animName->m_AnimID != constKit->m_anim) {
        continue;
      }

      animCheck[animNameIndex] = animName->m_AnimID;
      for (map = ANIM_STAND; map < NUM_OBJECTANIMATIONS; ++map) {
        if (!SStrCmp(g_animationNames[map], animName->m_name, 0x7FFFFFFF)) {
          animCheck[animNameIndex] = -1;
          constKit->m_anim = map;
          found = 1;
          break;
        }
      }

      if (found) {
        break;
      }
    }
  }

  for (i = static_cast<int>(animCheck.Count()) - 1; i >= 0; --i) {
    if (animCheck[i] != -1) {
      SysMsgPrintf(SYSMSG_WARNING, 2, "Anim name not found in AnimCompiles.h for id %d", animCheck[i]);
    }
  }

  s_precastAnimTransitions.SetCount(NUM_OBJECTANIMATIONS);
  for (i = 0; i < static_cast<int>(s_precastAnimTransitions.Count()); ++i) {
    s_precastAnimTransitions[i] = INVALID_ANIMATION;
  }

  for (i = g_spellVisualPrecastTransitionsDB.GetNumRecords() - 1; i >= 0; --i) {
    const SpellVisualPrecastTransitionsRec *transition = g_spellVisualPrecastTransitionsDB.GetRecordByIndex(i);
    ANIMENUMERATION                         source = INVALID_ANIMATION;
    ANIMENUMERATION                         destination = INVALID_ANIMATION;
    int                                     animation;

    ASSERT(transition);
    if (!transition->m_PrecastLoadAnimName[0] || !transition->m_PrecastHoldAnimName[0]) {
      continue;
    }

    for (animation = ANIM_STAND; animation < NUM_OBJECTANIMATIONS; ++animation) {
      if (!SStrCmp(g_animationNames[animation], transition->m_PrecastLoadAnimName, 0x7FFFFFFF)) {
        source = static_cast<ANIMENUMERATION>(animation);
        break;
      }
    }

    for (animation = ANIM_STAND; animation < NUM_OBJECTANIMATIONS; ++animation) {
      if (!SStrCmp(g_animationNames[animation], transition->m_PrecastHoldAnimName, 0x7FFFFFFF)) {
        destination = static_cast<ANIMENUMERATION>(animation);
        break;
      }
    }

    if (source != INVALID_ANIMATION && destination != INVALID_ANIMATION) {
      s_precastAnimTransitions[source] = destination;
    }
  }

  s_lightningManager = NEW(CLightningManager);
}

void SpellVisualsShutdown() {
  while (s_lightning.Head()) {
    s_lightning.DeleteNode(s_lightning.Head());
  }

  DELIFUSED(s_lightningManager);
  s_auraNames.Clear();
}

void SpellVisualsPlayCastKit(CGUnit_C *caster, const SpellVisualKitRec *kitRec, int spellID, bool isCastEffect) {
  PlayOneShotEffect(caster, kitRec->m_headEffect, UNITEFFECT_ATTACHHEAD, spellID, isCastEffect);
  PlayOneShotEffect(caster, kitRec->m_leftHandEffect, UNITEFFECT_ATTACHLEFTHAND, spellID, isCastEffect);
  PlayOneShotEffect(caster, kitRec->m_rightHandEffect, UNITEFFECT_ATTACHRIGHTHAND, spellID, isCastEffect);
  PlayOneShotEffect(caster, kitRec->m_baseEffect, UNITEFFECT_ATTACHBASE, spellID, isCastEffect);
  PlayOneShotEffect(caster, kitRec->m_breathEffect, UNITEFFECT_ATTACHBREATH, spellID, isCastEffect);
  PlayOneShotEffect(caster, kitRec->m_chestEffect, UNITEFFECT_ATTACHCHEST, spellID, isCastEffect);
  PlayOneShotEffect(caster, kitRec->m_specialEffect[0], static_cast<UNITEFFECTATTACHPPOINT>(7), spellID, isCastEffect);
  PlayOneShotEffect(caster, kitRec->m_specialEffect[1], static_cast<UNITEFFECTATTACHPPOINT>(8), spellID, isCastEffect);
  PlayOneShotEffect(caster, kitRec->m_specialEffect[2], static_cast<UNITEFFECTATTACHPPOINT>(9), spellID, isCastEffect);
  SpellVisualsProcedure(caster, kitRec, spellID, 0, 0);
}

void
SpellVisualsHandleCastStart(int id, const SpellCast &cast, CGUnit_C *caster, unsigned int duration, unsigned int animDuration, bool wasProc) {
  FATALASSERT(caster);

  UnitCombatLogCastStart(id, caster->GetGUID());

  const SpellRec *spellRec = g_spellDB.GetRecord(id);
  if (!spellRec) {
    SysMsgPrintf(SYSMSG_ERROR, 2, "NOSPELLIDFOUND|%d", id);
    return;
  }

  unsigned int   instant = caster->GetSpellRank(id) > 0 || (spellRec->m_attributes & 2);
  SpellVisualRec visRecData;
  if (!caster->GetAppropriateSpellVisual(spellRec, visRecData)) {
    SysMsgPrintf(SYSMSG_ERROR, 2, "SPELLVISUALIDNOTFOUND|%d", id);
    return;
  }

  const SpellVisualKitRec *visualRec = caster->GetRangedSpellAnim(id, 0);
  if (!visualRec) {
    return;
  }

  FATALASSERT(caster->IsA(TYPE_UNIT));
  if (wasProc) {
    return;
  }

  if (visualRec->m_soundID && duration) {
    SndInterfacePlaySpellSound(visualRec->m_soundID, caster);
  }
  NTempest::C3Vector position = caster->GetPosition();
  SpellVisualsPlayCameraShakeID(visualRec->m_shakeID, position);

  if (!instant) {
    SpellVisualsPlayCastKit(caster, visualRec, id, 0);
  }

  int animSet = 0;
  if (!instant && caster->SetSpellPreCastingAnimation(static_cast<ANIMENUMERATION>(visualRec->m_anim))) {
    unsigned int specialAnim = visualRec->m_anim == 105 || visualRec->m_anim == 106;
    animSet = caster->SetTorsoAnimation(37, specialAnim ? animDuration : 0, specialAnim ? 0x20 : 0);
  }

  caster->SetCastingSpell(id, duration, animSet);
  if (!instant && (spellRec->m_attributes & 0x400000) && (cast.targets & 2)) {
    caster->SaveTrackingTarget(cast.unitTarget, TRACKTYPE_SPELLPRECAST, 0);
  }
}

void SpellVisualsHandleCastStop(int id, CGUnit_C *caster, unsigned char status, unsigned char reason) {
  FATALASSERT(caster);

  if (status == 2 && reason == 18) {
    caster->PendingPrecastInterrupt(id);
  } else {
    caster->StopRangedAttackPrecast();
    caster->ClearTrackingTarget(status == 0);
    caster->StopSpellFizzleTimer(id, status);
  }
}

NTempest::C3Vector GetSpellChainEffectSource(const CGUnit_C &unit) {
  NTempest::C3Vector outVect;
  HMODEL             model = unit.GetCharacterModel(0);

  if (model) {
    outVect = NTempest::C3Vector(0.0f, 0.0f, 0.0f);
    if (!ModelGetObjectPosition(model, 0, &outVect)) {
      outVect = unit.GetPosition() - CGWorldFrame::GetActiveCamera()->Position();
      outVect.z += 1.5f;
    }
    HandleClose(model);
    outVect += CGWorldFrame::GetActiveCamera()->Position();
  } else {
    outVect = unit.GetPosition();
  }

  return outVect;
}

LightningObject::~LightningObject() {
  unsigned int index;

  ASSERT(s_lightningManager);
  for (index = 0; index < bolts.Count(); ++index) {
    if (bolts[index].boltID != BADBOLT) {
      s_lightningManager->Remove(bolts[index].boltID);
    }
  }

  bolts.Clear();
  guids.Clear();
  if (texture) {
    HandleClose(texture);
    texture = 0;
  }
}

bool LightningObject::Tick(unsigned int currentTime) {
  for (unsigned int i = 0; i < bolts.Count(); ++i) {
    Bolt &bolt = bolts[i];
    if (bolt.srcGuidSub == static_cast<unsigned short>(-1) || bolt.dstGuidSub == static_cast<unsigned short>(-1)) {
      continue;
    }

    CGObject_C *srcObj = ClntObjMgrObjectPtr(guids[bolt.srcGuidSub], __FILE__, __LINE__);
    CGObject_C *dstObj = ClntObjMgrObjectPtr(guids[bolt.dstGuidSub], __FILE__, __LINE__);
    CGUnit_C   *srcUnit = srcObj && (srcObj->GetType() & TYPE_UNIT) ? static_cast<CGUnit_C *>(srcObj) : 0;
    CGUnit_C   *dstUnit = dstObj && (dstObj->GetType() & TYPE_UNIT) ? static_cast<CGUnit_C *>(dstObj) : 0;

    if (forever || (currentTime >= bolt.birthTime && currentTime < bolt.deathTime)) {
      if (srcObj && dstObj) {
        NTempest::C3Vector sourcePosition;
        NTempest::C3Vector destPosition;

        if (srcUnit) {
          if (i) {
            GetMissileTargetPosition(srcUnit, GetMissileTargetLocation(guids[bolt.srcGuidSub], spellID), sourcePosition);
          } else {
            sourcePosition = GetSpellChainEffectSource(*srcUnit);
          }
        } else {
          sourcePosition = srcObj->GetPosition();
        }

        if (dstUnit) {
          GetMissileTargetPosition(dstUnit, GetMissileTargetLocation(guids[bolt.dstGuidSub], spellID), destPosition);
        } else {
          destPosition = dstObj->GetPosition();
        }

        if (bolt.boltID == BADBOLT) {
          bolt.boltID = s_lightningManager->Add(
              sourcePosition, destPosition, avgSegLen, width, NTempest::CImVector(static_cast<unsigned long>(-1)), noiseScale, texCoordScale,
              duration, texture, 0, 0
          );
        } else {
          s_lightningManager->Move(bolt.boltID, &sourcePosition, &destPosition);
        }
      } else {
        forever = 0;
        bolt.deathTime = currentTime;
      }
    }

    if (!forever && currentTime >= bolt.deathTime && bolt.boltID != BADBOLT) {
      if (i && srcUnit) {
        srcUnit->DDDELLOG(guids[0], "LightningObject::Tick", __FILE__, __LINE__);
      }
      if (dstUnit) {
        dstUnit->DDDELLOG(guids[0], "LightningObject::Tick", __FILE__, __LINE__);
      }
      s_lightningManager->Remove(bolt.boltID);
      bolt.boltID = BADBOLT;
    }
  }

  return forever || currentTime < deathTime;
}

void SpellVisualsProc_Eclipse(CGUnit_C *caster, const SpellVisualKitRec *kitRec, unsigned int spellID) {
  FATALASSERT(kitRec->m_characterParam[1] >= 0.0f && kitRec->m_characterParam[1] <= 1.0f);

  unsigned int duration = Spell_C_GetCastTime(spellID, 0);
  s_eclipseObject.color = NTempest::CImVector(static_cast<unsigned long>(kitRec->m_characterParam[0]) | 0xFF000000ul);
  unsigned int fadeDuration = static_cast<unsigned int>(duration * kitRec->m_characterParam[1]);
  s_eclipseObject.startTime = OsGetAsyncTimeMs();
  s_eclipseObject.fadeInTime = s_eclipseObject.startTime + fadeDuration;
  s_eclipseObject.fadeOutTime = s_eclipseObject.startTime + duration;
  s_eclipseObject.endTime = s_eclipseObject.startTime + duration + 100;
}

static void CreateLightningObj(
    const CGUnit_C          *unitPtr,
    const unsigned __int64  *guids,
    int                      numGuids,
    int                      spellID,
    const SpellVisualKitRec *kitRec,
    LightningObject        **objects,
    int                      maxObjects
) {
  if (!kitRec || !unitPtr || !guids || !numGuids || !spellID) {
    return;
  }

  const SpellChainEffectsRec *rec = g_spellChainEffectsDB.GetRecord(static_cast<int>(kitRec->m_characterParam[0]));
  if (!rec) {
    return;
  }

  CStatus      status;
  unsigned int currentTime = OsGetAsyncTimeMs();
  unsigned int boltCount = NTempest::CMath::ftol_0_256_(kitRec->m_characterParam[1]);
  if (!boltCount) {
    return;
  }
  FATALASSERT(boltCount <= 3);

  int added = 0;
  for (unsigned int boltIndex = 0; boltIndex < boltCount; ++boltIndex) {
    LightningObject *lightning = new LightningObject;
    s_lightning.LinkNode(lightning, LIST_TAIL, 0);
    if (added < maxObjects) {
      objects[added++] = lightning;
    }

    lightning->guids.SetCount(numGuids + 1);
    lightning->bolts.SetCount(numGuids);
    lightning->guids[0] = unitPtr->GetGUID();
    lightning->deathTime = currentTime + rec->m_SegDuration * numGuids;
    lightning->avgSegLen = rec->m_AvgSegLen;
    lightning->width = rec->m_Width;
    lightning->noiseScale = rec->m_NoiseScale;
    lightning->texCoordScale = rec->m_TexCoordScale;
    lightning->duration = rec->m_SegDuration * 0.001f;
    lightning->forever = NTempest::CMath::ftol_0_256_(kitRec->m_characterParam[2]) != 0;
    lightning->texture = TextureCreate(rec->m_Texture, CGxTexFlags(GxTex_Linear, 1, 1, 0, 0, 0, 1), &status, 0);
    lightning->spellID = spellID;
    lightning->AddRef();

    unsigned int srcGuidSub = 0;
    for (int i = 0; i < numGuids; ++i) {
      LightningObject::Bolt &bolt = lightning->bolts[i];
      bolt.srcGuidSub = static_cast<unsigned short>(-1);
      bolt.dstGuidSub = static_cast<unsigned short>(-1);
      bolt.boltID = BADBOLT;

      CGObject_C *target = ClntObjMgrObjectPtr(guids[i], __FILE__, __LINE__);
      if (target && (target->GetType() & (TYPE_UNIT | TYPE_GAMEOBJECT))) {
        lightning->guids[i + 1] = guids[i];
        bolt.birthTime = currentTime + i * rec->m_SegDelay;
        bolt.deathTime = bolt.birthTime + rec->m_SegDuration;
        bolt.srcGuidSub = static_cast<unsigned short>(srcGuidSub);
        bolt.dstGuidSub = static_cast<unsigned short>(i + 1);

        if (!lightning->forever && !(target->GetType() & TYPE_GAMEOBJECT)) {
          CGUnit_C *targetUnit = static_cast<CGUnit_C *>(target);
          if (i > 0 && i < numGuids - 1) {
            targetUnit->DDADDLOG(unitPtr->GetGUID(), "CreateLightningObj", __FILE__, __LINE__);
          }
          targetUnit->DDADDLOG(unitPtr->GetGUID(), "CreateLightningObj", __FILE__, __LINE__);
        }
        srcGuidSub = bolt.dstGuidSub;
      } else {
        lightning->guids[i + 1] = 0;
        bolt.birthTime = lightning->deathTime;
        bolt.deathTime = lightning->deathTime;
      }
    }
  }
}

void SpellVisualsProcedureDispatch(
    int                                   proc,
    CGUnit_C                             *caster,
    const SpellVisualKitRec              *kitRec,
    unsigned int                          spellID,
    const TSStackArray<unsigned __int64> *targets,
    const TSStackArray<MISS_REASON>      *missReasons
) {
  if (proc == 0) {
    if (targets) {
      CreateLightningObj(caster, targets->Ptr(), targets->Count(), spellID, kitRec, 0, 0);
    }
  } else if (proc == 6 && !missReasons) {
    SpellVisualsProc_Eclipse(caster, kitRec, spellID);
  }
}

void SpellVisualsProcedure(
    CGUnit_C                             *caster,
    const SpellVisualKitRec              *kitRec,
    unsigned int                          spellID,
    const TSStackArray<unsigned __int64> *targets,
    const TSStackArray<MISS_REASON>      *missReasons
) {
  if (caster && kitRec) {
    SpellVisualsProcedureDispatch(kitRec->m_characterProcedure, caster, kitRec, spellID, targets, missReasons);
  }
}

static void PlayImpactKit(CGUnit_C* target, const SpellVisualKitRec* impactKit) {
  UnitEffectOneShot(
      g_spellVisualEffectNameDB.GetRecord(impactKit->m_headEffect),
      target,
      UNITEFFECT_ATTACHHEAD,
      0,
      true,
      false);
  UnitEffectOneShot(
      g_spellVisualEffectNameDB.GetRecord(impactKit->m_chestEffect),
      target,
      UNITEFFECT_ATTACHCHEST,
      0,
      true,
      false);
  UnitEffectOneShot(
      g_spellVisualEffectNameDB.GetRecord(impactKit->m_baseEffect),
      target,
      UNITEFFECT_ATTACHBASE,
      0,
      true,
      false);

  NTempest::C3Vector position;
  target->GetPosition(position);
  SpellVisualsPlayCameraShakeID(impactKit->m_shakeID, position);
  target->SetSpellImpactKit(impactKit);
  if (impactKit->m_soundID) {
    SndInterfacePlaySpellSound(impactKit->m_soundID, target);
  }
}

unsigned int SpellGetRangedPrecastHoldAnim(unsigned int loadAnim) {
  return loadAnim < s_precastAnimTransitions.Count() ? s_precastAnimTransitions[loadAnim] : INVALID_ANIMATION;
}

void SpellVisualsTick(float elapsed) {
  unsigned long currentTime = OsGetAsyncTimeMs();

  LightningObject *lightning = s_lightning.Head();
  while (lightning) {
    LightningObject *next = s_lightning.Next(lightning);
    if (!lightning->Tick(currentTime)) {
      s_lightning.UnlinkNode(lightning);
      lightning->DelRef();
    }
    lightning = next;
  }

  s_lightningManager->Update(elapsed);
  s_eclipseObject.Update(currentTime);

  ITERATELIST(BlizzardObject, s_blizzard, blizzard) {
    blizzard->Update();
  }
}

void SpellVisualsRender() {
  s_lightningManager->Render(CGWorldFrame::GetActiveCamera()->Position());
  RenderFishingLines();
}

void SpellVisualsPlayCameraShakeID(unsigned int shakeID, const NTempest::C3Vector &position) {
  const SpellEffectCameraShakesRec *rec = g_spellEffectCameraShakesDB.GetRecord(shakeID);
  if (!rec) {
    return;
  }

  for (unsigned int i = 0; i < 3; ++i) {
    if (rec->m_CameraShake[i]) {
      CGWorldFrame::GetActiveCamera()->AddShake(rec->m_CameraShake[i], position);
    }
  }
}

void SpellVisualGetLightning(
    const CGUnit_C *unitPtr, const SpellVisualKitRec *kitRec, int spellID, LightningObject **objects, int numObjects
) {
  if (unitPtr) {
    const TSGrowableArray<unsigned __int64> &targets = unitPtr->GetSavedChannelSpellTargets();
    CreateLightningObj(unitPtr, targets.Ptr(), targets.Count(), spellID, kitRec, objects, numObjects);
  }
}

void SpellVisualClearLightning(LightningObject *lightning) {
  lightning->deathTime = OsGetAsyncTimeMs();
  lightning->forever = 0;
  lightning->DelRef();
}

BlizzardObject *SpellVisualsBlizzardCreate(const NTempest::C3Vector &pos, float radius, int spellID, const SpellVisualKitRec *kitRec) {
  unsigned int nameSub;

  BlizzardObject *blizzard = AllocBlizzard();

  nameSub = NTempest::CMath::ftol_0_256_(kitRec->m_characterParam[0]);
  FATALASSERT(nameSub < sizeof(modelNames) / sizeof(modelNames[0]));
  blizzard->Init(pos, modelNames[nameSub], radius, kitRec->m_characterParam[1]);
  return blizzard;
}

void SpellVisualsBlizzardDestroy(BlizzardObject *&blizzard) {
  blizzard->dead = 1;
  blizzard = 0;
}

FishingLineObject* SpellVisualsFishingLineCreate(const SpellVisualKitRec* kitRec, const unsigned __int64& gameObj, const unsigned __int64& caster) {
  if (!kitRec) {
    return 0;
  }
  FishingLineObject *object = s_freeFishingObjects.Get(0);
  if (object) {
    new (object) FishingLineObject;
  }
  object->object = gameObj;
  object->caster = caster;
  object->color.Set(
      static_cast<unsigned int>(kitRec->m_characterParam[3]) |
      0xFF000000);
  object->visible = 0;
  s_fishingLineObjects.LinkNode(object, LIST_TAIL, 0);
  return object;
}

void SpellVisualsFishingLineDestroy(FishingLineObject* object) {
  if (object) {
    s_fishingLineObjects.UnlinkNode(object);
    s_freeFishingObjects.Put(object);
  }
}

void SpellVisualFishingLineSetVisible(FishingLineObject* obj) {
  if (obj) {
    obj->visible = 1;
  }
}

void LightningObject::AddRef() {
  ++refCount;
}

void LightningObject::DelRef() {
  FATALASSERT(refCount > 0);
  if (!--refCount) {
    delete this;
  }
}

bool IsShapeshiftSpell(const SpellRec *rec) {
  for (unsigned int effect = 0; effect < 3; ++effect) {
    if (rec->m_effect[effect] == 6 && (rec->m_effectAura[effect] == 36 || rec->m_effectAura[effect] == 56)) {
      return true;
    }
  }
  return false;
}
void SpellVisualsPlayKit(CGUnit_C* target, unsigned int id) {
  const SpellVisualKitRec *kitRec = g_spellVisualKitDB.GetRecord(id);
  if (kitRec) {
    PlayImpactKit(target, kitRec);
  }
}

static unsigned char GetSpellRecords(CGUnit_C* caster, int spellID, const SpellRec*& srec, SpellVisualRec& visRecData, const SpellVisualRec*& visRec, const SpellVisualKitRec*& kitRec) {
  srec = g_spellDB.GetRecord(spellID);
  if (!srec) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "NOSPELLIDFOUND|%d", spellID);
    return 0;
  }
  visRec = caster->GetAppropriateSpellVisual(
      const_cast<SpellRec *>(srec),
      visRecData);
  if (!visRec) {
    SysMsgPrintf(
        SYSMSG_WARNING,
        2,
        "SPELLVISUALIDNOTFOUND|%d",
        spellID);
    return 0;
  }
  kitRec = caster->GetRangedSpellAnim(spellID, 1);
  return 1;
}

static void PlayOneShotEffect(
    const NTempest::C3Vector &location,
    int effectID,
    const TSStackArray<unsigned __int64> &objects
) {
  UnitEffectOneShot(
      g_spellVisualEffectNameDB.GetRecord(effectID),
      location,
      &objects,
      0.0f,
      1.0f);
}

static void PlayCastAnim(
    CGUnit_C *caster,
    const SpellRec *srec,
    const SpellVisualRec *visRec,
    const SpellVisualKitRec *kitRec,
    const TSStackArray<unsigned __int64> &targets,
    int &torsoAnimSet
) {
  FATALASSERT(caster->GetType() & TYPE_UNIT);
  if (kitRec->m_anim) {
    ANIMENUMERATION finalAnim;
    bool animSuccessful = caster->SetSpellCastingAnimation(
        static_cast<ANIMENUMERATION>(kitRec->m_anim),
        visRec->m_castKit,
        kitRec->m_soundID,
        kitRec->m_shakeID,
        finalAnim);
    caster->AddSpellProcOneShotEffect(srec->m_ID, kitRec);
    NTempest::C3Vector position;
    caster->GetPosition(position);
    SpellVisualsPlayCameraShakeID(kitRec->m_shakeID, position);
    int oldCastingSpell = caster->SetCastingSpell(srec->m_ID, 0, 0);
    if (animSuccessful) {
      torsoAnimSet = caster->SetTorsoAnimation(38, 0, 0);
    }
    if (torsoAnimSet) {
      caster->HandlePrecastStop(srec->m_ID, true);
      caster->SetSheatheReason(
          SHEATHEREASON_PRECAST,
          (srec->m_attributes & 0x40000) == 0,
          false);
    } else {
      caster->ClearSpellCastAnimInfo();
    }
    if (!oldCastingSpell) {
      caster->SetCastingSpell(0, 1, 0);
    }
    if (torsoAnimSet &&
        Object_C_AnimHasHitEvent(torsoAnimSet)) {
      caster->AddHitAnimHolds(srec->m_ID, targets);
    }
    caster->SetCastingSpell(0, 0, 0);
  } else {
    SndInterfacePlaySpellSound(kitRec->m_soundID, caster);
    NTempest::C3Vector position;
    caster->GetPosition(position);
    SpellVisualsPlayCameraShakeID(kitRec->m_shakeID, position);
  }
  if (!torsoAnimSet &&
      caster->GetCurrentTorsoAnim() == 37) {
    caster->ClearTorsoAnimation(0);
  }
}

void HandleMissileEffects(
    CGUnit_C *caster,
    const SpellRec *srec,
    const SpellVisualRec *visRec,
    int ammoDisplayID,
    int ammoInventoryType,
    const SpellCast &cast,
    const TSStackArray<unsigned __int64> &targets,
    const TSStackArray<MISS_REASON> *missReasons,
    bool wasProc
) {
  if (!caster || !targets.Count() ||
      (!visRec->m_hasMissile && !ammoDisplayID)) {
    return;
  }

  if (cast.targets & 0x802) {
    unsigned int i;
    for (i = 0; i < targets.Count(); ++i) {
      CGObject_C *target =
          ClntObjMgrObjectPtr(targets[i], __FILE__, __LINE__);
      if (target) {
        MISS_REASON reason = missReasons
                                 ? (*missReasons)[i]
                                 : MISS_PHYSICAL;
        NTempest::C3Vector destination;
        target->GetPosition(destination);
        caster->StoreSpellMissileEffect(
            targets[i], destination, srec->m_speed,
            ammoDisplayID, ammoInventoryType, visRec,
            missReasons == 0, reason, srec->m_ID, wasProc);
      }
    }
  } else if (cast.targets & 0x40) {
    const unsigned __int64 noTarget = 0;
    caster->StoreSpellMissileEffect(
        noTarget, cast.destLocation, srec->m_speed,
        ammoDisplayID, ammoInventoryType, visRec,
        missReasons == 0, MISS_PHYSICAL, srec->m_ID, wasProc);
  }
}

void SpellVisualsHandleSpellStart(
    int spellID,
    const SpellCast &cast,
    CGGameObject_C *caster,
    const TSStackArray<unsigned __int64> &targets,
    bool ignoreAreaEffect,
    bool hits
) {
  FATALASSERT(caster);
  const SpellRec *srec = g_spellDB.GetRecord(spellID);
  if (!srec) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "NOSPELLIDFOUND|%d", spellID);
    return;
  }
  const SpellVisualRec *visRec =
      g_spellVisualDB.GetRecord(srec->m_spellVisualID);
  if (!visRec) {
    SysMsgPrintf(
        SYSMSG_WARNING, 2,
        "SPELLVISUALIDNOTFOUND|%d", spellID);
    return;
  }

  if (targets.Count() && srec->m_speed <= 0.0f && hits) {
    const SpellVisualKitRec *impactKit =
        g_spellVisualKitDB.GetRecord(visRec->m_impactKit);
    if (impactKit) {
      unsigned int i;
      for (i = 0; i < targets.Count(); ++i) {
        CGObject_C *target =
            ClntObjMgrObjectPtr(targets[i], __FILE__, __LINE__);
        if (target && (target->GetType() & TYPE_UNIT)) {
          PlayImpactKit(static_cast<CGUnit_C *>(target), impactKit);
        }
      }
    }
  }
  if (!ignoreAreaEffect && (cast.targets & 0x40)) {
    PlayOneShotEffect(cast.destLocation, visRec->m_areaModel, targets);
  }
}

void SpellVisualsHandleSpellStartHits(
    int spellID,
    const SpellCast &cast,
    CGUnit_C *caster,
    const TSStackArray<unsigned __int64> &targets,
    int ammoDisplayID,
    int ammoInventoryType,
    int flags
) {
  bool wasProc = (flags & 1) != 0;
  SpellVisualRec visRecData;
  const SpellRec *srec;
  const SpellVisualRec *visRec;
  const SpellVisualKitRec *kitRec;
  if (!GetSpellRecords(
          caster, spellID, srec, visRecData, visRec, kitRec)) {
    return;
  }

  if (!(flags & 8) && !wasProc && (cast.targets & 0x40)) {
    PlayOneShotEffect(cast.destLocation, visRec->m_areaModel, targets);
  }
  int torsoAnimSet = 0;
  if (!wasProc && kitRec) {
    PlayCastAnim(
        caster, srec, visRec, kitRec, targets, torsoAnimSet);
  }
  if (kitRec && !wasProc) {
    SpellVisualsProcedure(
        caster,
        const_cast<SpellVisualKitRec *>(kitRec),
        spellID,
        const_cast<TSStackArray<unsigned __int64> *>(&targets),
        0);
  }

  if (srec->m_speed > 0.0f) {
    HandleMissileEffects(
        caster, srec, visRec, ammoDisplayID,
        ammoInventoryType, cast, targets, 0, wasProc);
  } else {
    caster->MaybeSaveChannelSpellTargets(spellID, targets);
    const SpellVisualKitRec *impactKit =
        g_spellVisualKitDB.GetRecord(visRec->m_impactKit);
    if (impactKit && !IsSpellAura(srec)) {
      unsigned int i;
      for (i = 0; i < targets.Count(); ++i) {
        CGObject_C *target =
            ClntObjMgrObjectPtr(targets[i], __FILE__, __LINE__);
        if (target && (target->GetType() & TYPE_UNIT)) {
          caster->SetImpactKitEffect(
              spellID,
              static_cast<CGUnit_C *>(target),
              impactKit,
              torsoAnimSet == 0);
        }
      }
    }
  }
}

void SpellVisualsHandleSpellStartMisses(
    int spellID,
    const SpellCast &cast,
    CGUnit_C *caster,
    const TSStackArray<unsigned __int64> &targets,
    TSStackArray<MISS_REASON> &missReasons,
    int ammoDisplayID,
    int ammoInventoryType,
    int flags
) {
  if (!targets.Count()) {
    return;
  }
  bool ignoreAreaEffect = (flags & 8) != 0;
  bool wasProc = (flags & 1) != 0;
  SpellVisualRec visRecData;
  const SpellRec *srec;
  const SpellVisualRec *visRec;
  const SpellVisualKitRec *kitRec;
  if (!GetSpellRecords(
          caster, spellID, srec, visRecData, visRec, kitRec)) {
    return;
  }

  if (caster->GetCurrentTorsoAnim() == 37) {
    caster->ClearTorsoAnimation(0);
  }
  if (!ignoreAreaEffect && !wasProc &&
      (cast.targets & 0x40)) {
    PlayOneShotEffect(cast.destLocation, visRec->m_areaModel, targets);
  }
  int torsoAnimSet = 0;
  if (!wasProc && kitRec) {
    PlayCastAnim(
        caster, srec, visRec, kitRec, targets, torsoAnimSet);
  }
  if (kitRec && !wasProc) {
    SpellVisualsProcedure(
        caster,
        const_cast<SpellVisualKitRec *>(kitRec),
        spellID,
        const_cast<TSStackArray<unsigned __int64> *>(&targets),
        &missReasons);
  }

  unsigned int i;
  for (i = 0; i < targets.Count(); ++i) {
    CGObject_C *target =
        ClntObjMgrObjectPtr(targets[i], __FILE__, __LINE__);
    if (target && (target->GetType() & TYPE_UNIT)) {
      missReasons[i] =
          static_cast<CGUnit_C *>(target)->AdjustVictimState(
              missReasons[i]);
    }
  }

  if (srec->m_speed > 0.0f) {
    HandleMissileEffects(
        caster, srec, visRec, ammoDisplayID,
        ammoInventoryType, cast, targets, &missReasons, wasProc);
    return;
  }

  for (i = 0; i < targets.Count(); ++i) {
    CGObject_C *targetObject =
        ClntObjMgrObjectPtr(targets[i], __FILE__, __LINE__);
    if (targetObject && (targetObject->GetType() & TYPE_UNIT) &&
        caster->GetGUID() == ClntObjMgrGetActivePlayer()) {
      CGUnit_C *target = static_cast<CGUnit_C *>(targetObject);
      if (srec->m_attributes & 0x404) {
        CGPlayer_C::AddDeferredSpellMiss(
            targets[i],
            target->AdjustVictimState(missReasons[i]),
            spellID);
      } else {
        target->AddWorldText(missReasons[i]);
        UnitCombatLogSpellMissed(
            missReasons[i], spellID, caster->GetGUID(),
            target->GetGUID());
      }
    }
  }
}

const char* GetSpellAuraEffectName(int effectID) {
  if (effectID < static_cast<int>(s_auraNames.Count())) {
    return s_auraNames[effectID]->m_name_lang[0];
  }
  return "INVALID_SPELL_AURA_EFFECT";
}

const char* GetSpellAuraEffectToken(int effectID) {
  if (effectID < static_cast<int>(s_auraNames.Count())) {
    return s_auraNames[effectID]->m_globalstrings_tag;
  }
  return "INVALID_SPELL_AURA_EFFECT";
}
