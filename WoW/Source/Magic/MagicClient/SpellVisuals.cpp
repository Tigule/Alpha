#include <Base/Base.h>
#include <Gx/Gx.h>
#include <MapDefs.h>
#include "WorldClient/World.h"
#include <WowConst.h>
#include "Ui/LootFrame.h"
#include "Ui/PartyFrame.h"
#include "Net/NetClient/NetClient.h"

#include "DB/DBClient/AutoCode/SpellAuraNamesRec.h"
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
#include "Services/IParticleMisc.h"
#include "Services/Lightning.h"
#include "Services/SysMessage.h"
#include "Services/Texture.h"
#include <Os/OsTime.h>
#include "Model/IModel.h"
#include "SoundInterface/SoundInterface.h"
#include "Tempest/c34matrix.h"
#include "Tempest/caabox.h"
#include "Tempest/cmath.h"
#include "Tempest/caasphere.h"
#include "Ui/WorldFrame.h"

#include <Base/Status.h>
#include <Base/CDataAllocator.h>
#include <stpl.h>
#include <string.h>

class CDataStore;
void DayNightSetEclipse(NTempest::CImVector color, float amount);

struct SPELLVISUALNODE : public TSHashObject<SPELLVISUALNODE, HASHKEY_NONE> {
  SPELLVISUALNODE();
  SPELLVISUALNODE(const SPELLVISUALNODE &);

  UINT            m_effects[15];
  ANIMENUMERATION m_anims[2];
};

NODEDECL(BlizzardObject) {
  NODEDECL(Shard) {
    NTempest::C3Vector pos;
    HMODEL             hModel;
    DWORD              startTime;

    void Init() {
      hModel = 0;
    }
  };

  typedef Shard       *PShard;
  typedef const Shard *PCShard;
  typedef Shard        Shard_HuhHuhHuh_Huh;

  static LISTDECL(Shard, shardPool);

  static Shard *AllocShard();
  static void   FreeShard(Shard * &shard);
  static BOOL   ShardSeqFinished(LPVOID param);

  HMODEL              shardModel;
  DWORD               hWorldObject;
  NTempest::C3Vector  groundPos;
  float               radius;
  float               numEmitted;
  float               emissionRate;
  BYTE                dead;
  NTempest::CAaSphere boundSphere;
  LISTDECL(Shard, shards);

  void        UpdateBounds();
  static void WorldObjectRender(LPVOID param, const NTempest::C44Matrix &mtx);
  void        Init(const NTempest::C3Vector &pos, LPCSTR modelName, float pRadius, float pEmissionRate);
  void        Destroy();
  void        Update();
  void        Render(const NTempest::C44Matrix &mtx);
};

NODEDECL(LightningObject) {
 public:
  struct Bolt {
    enum {
      NULL_SUB = 0xFFFF
    };

    WORD   srcGuidSub;
    WORD   dstGuidSub;
    UINT   birthTime;
    UINT   deathTime;
    BoltID boltID;
  };

  TSGrowableArray<DWORDLONG> guids;
  TSGrowableArray<Bolt>      bolts;
  UINT                       deathTime;
  float                      avgSegLen;
  float                      width;
  float                      noiseScale;
  float                      texCoordScale;
  float                      duration;
  HTEXTURE                   texture;
  int                        spellID;
  BYTE                       forever;

  LightningObject();
  ~LightningObject();
  bool Tick(UINT currentTime);
  void AddRef();
  void DelRef();

 private:
  int refCount;
};

NODEDECL(FishingLineObject) {
  DWORDLONG           object;
  DWORDLONG           caster;
  NTempest::CImVector color;
  bool                visible;

  void Render();
  void RenderLine(const NTempest::C3Vector &p0, const NTempest::C3Vector &p1, const NTempest::CImVector &color);
};

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

  DWORDLONG          caster;
  DWORDLONG          casterUnit;
  int                spellID;
  WORD               targets;
  DWORDLONG          unitTarget;
  DWORDLONG          itemTarget;
  DWORDLONG          selectedTarget;
  NTempest::C3Vector sourceLocation;
  NTempest::C3Vector destLocation;
  float              destFacing;
  UINT               destZoneID;
  UINT               castTime;
  UINT               castEndTime;
  int                spellIndex;
  UINT               spellLevel;
  DWORDLONG          ammoItem;
  DWORDLONG          reflector;
  char               targetString[128];
  int                overrideRank;
  WORD               flags;

  void BuildFullZoneUpdate(CDataStore *msg);
  void UnpackFullZoneUpdate(CDataStore *msg);
};

static void             ShardEventCallback(LPCSTR eventName, const NTempest::C3Vector &position, LPVOID param);
static void             FreeBlizzard(BlizzardObject *bliz);
static void             RenderFishingLines();
static bool             GetFishingLineStartPos(HMODEL model, NTempest::C3Vector &pos);
SPELL_VISUAL_ATTACHMENT GetMissileTargetLocation(DWORDLONG caster, UINT spellID);
void                    GetMissileTargetPosition(CGObject_C *target, SPELL_VISUAL_ATTACHMENT hitLocation, NTempest::C3Vector &position);
int                     Spell_C_GetCastTime(int id, BOOL isPet);
static void             CreateLightningObj(
    const CGUnit_C          *unitPtr,
    const DWORDLONG         *guids,
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
    const NTempest::C3Vector       &location,
    const TSStackArray<DWORDLONG>  *objects,
    float                           facing,
    float                           scale
);
void SpellVisualsProcedure(
    CGUnit_C                        *caster,
    const SpellVisualKitRec         *kitRec,
    UINT                             spellID,
    const TSStackArray<DWORDLONG>   *targets,
    const TSStackArray<MISS_REASON> *missReasons
);
bool IsSpellAura(const SpellRec *rec);
bool Object_C_AnimHasHitEvent(int anim);
void UnitCombatLogSpellMissed(UINT reason, UINT spellID, DWORDLONG caster, DWORDLONG target);
void SpellVisualsPlayCameraShakeID(UINT shakeID, const NTempest::C3Vector &position);
void UnitCombatLogCastStart(UINT spellID, DWORDLONG caster);
void SpellSoundEffectCallback(LPCSTR eventName, const NTempest::C3Vector &position);

static NTempest::C3Vector GetSpellChainEffectSource(const CGUnit_C &unit);
static void               InitializeAuraNames();
static void InitializeFishingLineIntervals();
static void InitializeFishingLineIndices();
static void PlayOneShotEffect(CGObject_C *object, int effectID, UNITEFFECTATTACHPPOINT attach, int spellID, bool isCastEffect);
static void SpellVisualsProcedureDispatch(
    int                              proc,
    CGUnit_C                        *caster,
    const SpellVisualKitRec         *kitRec,
    UINT                             spellID,
    const TSStackArray<DWORDLONG>   *targets,
    const TSStackArray<MISS_REASON> *missReasons
);
static void            SpellVisualsProc_Eclipse(CGUnit_C *caster, const SpellVisualKitRec *kitRec, UINT spellID);
static void            PlayOneShotEffect(const NTempest::C3Vector &pos, int effectID, const TSStackArray<DWORDLONG> &objects);
static void            PlayImpactKit(CGUnit_C *target, const SpellVisualKitRec *impactKit);
static BlizzardObject *AllocBlizzard();
void                   HandleMissileEffects(
    CGUnit_C                        *caster,
    const SpellRec                  *srec,
    const SpellVisualRec            *visRec,
    int                              ammoDisplayID,
    int                              ammoInventoryType,
    const SpellCast                 &cast,
    const TSStackArray<DWORDLONG>   &targets,
    const TSStackArray<MISS_REASON> *missReasons,
    bool                             wasProc
);
static bool GetSpellRecords(
    CGUnit_C                 *caster,
    int                       spellID,
    const SpellRec          *&srec,
    SpellVisualRec           &visRecData,
    const SpellVisualRec    *&visRec,
    const SpellVisualKitRec *&kitRec
);
static void PlayCastAnim(
    CGUnit_C                      *caster,
    const SpellRec                *srec,
    const SpellVisualRec          *visRec,
    const SpellVisualKitRec       *kitRec,
    const TSStackArray<DWORDLONG> &targets,
    int                           &torsoAnimSet
);

LISTDECL(BlizzardObject::Shard, BlizzardObject::shardPool);

static TInstanceAllocator<FishingLineObject> s_freeFishingObjects(20);
static LISTDECL(FishingLineObject, s_fishingLineObjects);
static TSCArray<WORD, 201>  s_fishingLineIndices;
static TSCArray<float, 201> s_segmentPoints;

BlizzardObject::Shard *BlizzardObject::AllocShard() {
  if (!shardPool.Head()) {
    shardPool.NewNode(LIST_TAIL, 0, 0);
  }

  Shard *shard = shardPool.Head();
  shard->Unlink();
  shard->Init();
  return shard;
}

void BlizzardObject::FreeShard(Shard *&shard) {
  shard->Unlink();
  shardPool.LinkNode(shard, LIST_TAIL, 0);
  shard = 0;
}

void BlizzardObject::WorldObjectRender(LPVOID param, const NTempest::C44Matrix &mtx) {
  ((BlizzardObject *)param)->Render(mtx);
}

void BlizzardObject::Init(const NTempest::C3Vector &pos, LPCSTR modelName, float pRadius, float pEmissionRate) {
  dead = 0;
  numEmitted = 0.0f;
  groundPos = pos;
  radius = pRadius;
  emissionRate = pEmissionRate;
  shardModel = ObjectModelCreate(modelName, TYPE_OBJECT, 0x100800);

  NTempest::C44Matrix mtx;
  *mtx.Row3AsVec3() = groundPos;
  hWorldObject = CWorld::AddDoodad(0, 0, mtx, 2);
  CWorld::SetObjectRenderCallback(hWorldObject, WorldObjectRender, this);

  float               bounds = pRadius;
  NTempest::CAaSphere sphere;
  if (ModelGetBounds(shardModel, &sphere)) {
    bounds = sphere.r + pRadius;
  }
  NTempest::C3Vector corner(bounds, bounds, bounds);
  NTempest::CAaBox   aaBox(-corner, corner);
  CWorld::UpdateObject(hWorldObject, mtx, aaBox);
}

void BlizzardObject::Destroy() {
  HandleClose(shardModel);
  shardModel = 0;
}

void BlizzardObject::Update() {
  if (dead && !shards.Head()) {
    CWorld::RemoveObject(hWorldObject);
    Destroy();
    FreeBlizzard(this);
    return;
  }

  if (!dead) {
    numEmitted += CWorld::GetTickTimeSec() * emissionRate;
    while (numEmitted >= 1.0f) {
      if (ModelIsLoaded(shardModel, 1)) {
        Shard *shard = AllocShard();
        shards.LinkNode(shard, LIST_TAIL, 0);
        NTempest::C2Vector offset = radius * (NTempest::CRandom::real_(g_rndSeed) * NTempest::CRandom::C2Vector_(g_rndSeed));
        NTempest::C3Vector a(groundPos.x + offset.x, groundPos.y + offset.y, groundPos.z + 50.0f);
        NTempest::C3Vector b(groundPos.x + offset.x, groundPos.y + offset.y, groundPos.z);
        b.z -= 50.0f;
        float groundT = 1.0f;
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

  SAFEITERATELIST(Shard, shards, shard) {
    if (CWorld::GetCurTimeMs() >= shard->startTime && !ModelAdvanceTime(shard->hModel)) {
      HandleClose(shard->hModel);
      FreeShard(shard);
    }
  }
}

BOOL BlizzardObject::ShardSeqFinished(LPVOID param) {
  return 0;
}

static void ShardEventCallback(LPCSTR eventName, const NTempest::C3Vector &position, LPVOID param) {
  static int s_counter;

  ++s_counter;
  if ((s_counter & 1) && *(const UINT *)eventName == 'DNS$') {
    SpellSoundEffectCallback(eventName + 4, position);
  }
}

void BlizzardObject::Render(const NTempest::C44Matrix &mtx) {
  NTempest::C34Matrix transform;

  ITERATELIST(Shard, shards, shard) {
    if (CWorld::GetCurTimeMs() >= shard->startTime) {
      (CWorld::GetCamTarget() - CWorld::GetCamPos()).Mag();
      *transform.Row3AsVec3() = shard->pos - CWorld::GetCamPos();
      ModelAnimate(shard->hModel, transform, 1.0f, CWorld::GetCamPos(), CWorld::GetCamTarget() - CWorld::GetCamPos());
      ModelProcessEvents(shard->hModel, mtx);
      ModelAddToScene(shard->hModel, 0);
    }
  }
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

static bool GetFishingLineStartPos(HMODEL model, NTempest::C3Vector &pos) {
  FATALASSERT(model);

  HMODEL attached = 0;
  UINT   size = 1;
  if (!ModelGetLinkPoint(model, 1, &attached, &size) || !attached) {
    return false;
  }

  bool result = ModelGetEventObjectPosition(attached, 2, 0, &pos) != 0;
  HandleClose(attached);
  return result;
}

void FishingLineObject::RenderLine(const NTempest::C3Vector &p0, const NTempest::C3Vector &p1, const NTempest::CImVector &color) {
  NTempest::C3Vector point0 = p0;
  NTempest::C3Vector point1 = p1;
  float              maxDip = (point0 - point1).Mag() * 0.05f;
  NTempest::C3Vector xy = point0;
  NTempest::C3Vector xyIncr = (point1 - point0) * 0.005f;
  NTempest::C3Vector points[201];

  for (UINT i = 0; i < 201; ++i) {
    points[i].x = xy.x;
    points[i].y = xy.y;
    points[i].z = xy.z + maxDip * s_segmentPoints[i];
    xy += xyIncr;
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

static TSFixedArray<enum ANIMENUMERATION> s_precastAnimTransitions;
static LISTDECL(LightningObject, s_lightning);
static CLightningManager *s_lightningManager;
static LISTDECL(BlizzardObject, s_blizzardPool);
static LISTDECL(BlizzardObject, s_blizzard);
static TSGrowableArray<const SpellAuraNamesRec *> s_auraNames;

SPELLVISUALNODE::SPELLVISUALNODE() {
  int i;
  for (i = 0; i < 15; ++i) {
    m_effects[i] = 0;
  }
  for (i = 0; i < 2; ++i) {
    m_anims[i] = RESET_ANIMATION_INDICES0;
  }
}

static void FreeBlizzard(BlizzardObject *bliz) {
  s_blizzardPool.LinkNode(bliz, LIST_TAIL, 0);
}

void SpellVisualsInitialize() {
  TSGrowableArray<int> animCheck;
  int                  i;
  UINT                 map;
  SpellVisualKitRec   *constKit;
  bool                 found;

  InitializeAuraNames();
  InitializeFishingLineIntervals();
  InitializeFishingLineIndices();

  animCheck.SetCount(g_spellVisualAnimNameDB.GetNumRecords());
  for (i = 0; i < g_spellVisualAnimNameDB.GetNumRecords(); ++i) {
    animCheck[i] = -1;
  }

  for (i = 0; i < g_spellVisualKitDB.GetNumRecords(); ++i) {
    constKit = (SpellVisualKitRec *)g_spellVisualKitDB.GetRecordByIndex(i);
    if (constKit) {
      if (constKit->m_anim <= 0) {
        constKit->m_anim = -1;
      } else {
        found = false;
        for (int animNameIndex = 0; animNameIndex < g_spellVisualAnimNameDB.GetNumRecords() && !found; ++animNameIndex) {
          const SpellVisualAnimNameRec *animName = g_spellVisualAnimNameDB.GetRecordByIndex(animNameIndex);
          if (animName && animName->m_AnimID == constKit->m_anim) {
            animCheck[animNameIndex] = animName->m_AnimID;
            for (map = ANIM_STAND; map < NUM_OBJECTANIMATIONS; ++map) {
              if (!SStrCmp(g_animationNames[map], animName->m_name, 0x7FFFFFFF)) {
                animCheck[animNameIndex] = -1;
                constKit->m_anim = map;
                found = true;
                break;
              }
            }
          }
        }
      }
    }
  }

  for (i = animCheck.Count(); i--;) {
    if (animCheck[i] != -1) {
      SysMsgPrintf(SYSMSG_WARNING, 2, "Anim name not found in AnimCompiles.h for id %d", animCheck[i]);
    }
  }

  s_precastAnimTransitions.SetCount(NUM_OBJECTANIMATIONS);
  for (i = 0; i < NUM_OBJECTANIMATIONS; ++i) {
    s_precastAnimTransitions[i] = INVALID_ANIMATION;
  }

  for (i = g_spellVisualPrecastTransitionsDB.GetNumRecords(); i--;) {
    const SpellVisualPrecastTransitionsRec *rec = g_spellVisualPrecastTransitionsDB.GetRecordByIndex(i);
    UINT                                    source;
    UINT                                    destination;

    FATALASSERT(rec);
    if (!rec->m_PrecastLoadAnimName[0] || !rec->m_PrecastHoldAnimName[0]) {
      continue;
    }

    for (source = ANIM_STAND; source < NUM_OBJECTANIMATIONS; ++source) {
      if (!SStrCmp(g_animationNames[source], rec->m_PrecastLoadAnimName, 0x7FFFFFFF)) {
        break;
      }
    }
    if (source == NUM_OBJECTANIMATIONS) {
      continue;
    }

    for (destination = ANIM_STAND; destination < NUM_OBJECTANIMATIONS; ++destination) {
      if (!SStrCmp(g_animationNames[destination], rec->m_PrecastHoldAnimName, 0x7FFFFFFF)) {
        break;
      }
    }
    if (destination < NUM_OBJECTANIMATIONS) {
      s_precastAnimTransitions[source] = (ANIMENUMERATION)destination;
    }
  }

  s_lightningManager = NEW(CLightningManager);
}

static void InitializeAuraNames() {
  int i;

  s_auraNames.SetCount(89);
  memset(s_auraNames.Ptr(), 0, 89 * sizeof(SpellAuraNamesRec *));

  for (i = g_spellAuraNamesDB.GetNumRecords(); i--;) {
    const SpellAuraNamesRec *auraName = g_spellAuraNamesDB.GetRecordByIndex(i);
    int                      enumID = auraName->m_EnumID;

    if (enumID < (int)s_auraNames.Count()) {
      FATALASSERT(!s_auraNames[enumID]);
      s_auraNames[enumID] = auraName;
    }
  }

  for (i = s_auraNames.Count(); i--;) {
    FATALASSERT(s_auraNames[i]);
  }
}

static void InitializeFishingLineIntervals() {
  float current = 0.0f;
  int   index;

  for (index = 0; index < (int)s_segmentPoints.Count(); ++index) {
    s_segmentPoints[index] = -NTempest::CMath::sin_(PI * min(max(0.0f, current), 1.0f));
    current += 0.005f;
  }
}

static void InitializeFishingLineIndices() {
  int index;

  for (index = 0; index < (int)s_fishingLineIndices.Count(); ++index) {
    s_fishingLineIndices[index] = index;
  }
}

void SpellVisualsShutdown() {
  while (s_lightning.Head()) {
    s_lightning.DeleteNode(s_lightning.Head());
  }

  DEL(s_lightningManager);
  s_auraNames.Clear();
}

void SpellVisualsPlayCastKit(CGUnit_C *caster, const SpellVisualKitRec *kitRec, int spellID, bool isCastEffect) {
  PlayOneShotEffect(caster, kitRec->m_headEffect, UNITEFFECT_ATTACHHEAD, spellID, isCastEffect);
  PlayOneShotEffect(caster, kitRec->m_leftHandEffect, UNITEFFECT_ATTACHLEFTHAND, spellID, isCastEffect);
  PlayOneShotEffect(caster, kitRec->m_rightHandEffect, UNITEFFECT_ATTACHRIGHTHAND, spellID, isCastEffect);
  PlayOneShotEffect(caster, kitRec->m_baseEffect, UNITEFFECT_ATTACHBASE, spellID, isCastEffect);
  PlayOneShotEffect(caster, kitRec->m_breathEffect, UNITEFFECT_ATTACHBREATH, spellID, isCastEffect);
  PlayOneShotEffect(caster, kitRec->m_chestEffect, UNITEFFECT_ATTACHCHEST, spellID, isCastEffect);
  PlayOneShotEffect(caster, kitRec->m_specialEffect[0], UNITEFFECT_ATTACHSPECIAL1, spellID, isCastEffect);
  PlayOneShotEffect(caster, kitRec->m_specialEffect[1], UNITEFFECT_ATTACHSPECIAL2, spellID, isCastEffect);
  PlayOneShotEffect(caster, kitRec->m_specialEffect[2], UNITEFFECT_ATTACHSPECIAL3, spellID, isCastEffect);
  SpellVisualsProcedure(caster, kitRec, spellID, 0, 0);
}

static void PlayOneShotEffect(CGObject_C *object, int effectID, UNITEFFECTATTACHPPOINT attach, int spellID, bool isCastEffect) {
  if (effectID) {
    VALIDATEBEGIN;
    VALIDATE(object);
    VALIDATE(attach < NUM_UNITEFFECTATTACHPOINTS);
    VALIDATEENDVOID;
    const SpellVisualEffectNameRec *effectRec = g_spellVisualEffectNameDB.GetRecord(effectID);
    UnitEffectOneShot(effectRec, object, attach, spellID, isCastEffect, 0);
  }
}

void SpellVisualsHandleCastStart(int id, const SpellCast &cast, CGUnit_C *caster, UINT duration, UINT animDuration, bool wasProc) {
  VALIDATEBEGIN;
  VALIDATE(caster);
  VALIDATEENDVOID;

  UnitCombatLogCastStart(id, caster->GetGUID());

  const SpellRec *spellRec = g_spellDB.GetRecord(id);
  if (!spellRec) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "NOSPELLIDFOUND|%d", id);
    return;
  }

  bool           instant = caster->GetSpellCastingTime(id) <= 0 && !(spellRec->m_attributes & 2);
  SpellVisualRec visRecData;
  if (!caster->GetAppropriateSpellVisual(spellRec, visRecData)) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "SPELLVISUALIDNOTFOUND|%d", id);
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
  SpellVisualsPlayCameraShakeID(visualRec->m_shakeID, caster->GetPosition());

  if (!instant) {
    SpellVisualsPlayCastKit(caster, visualRec, id, 0);
  }

  int animSet = 0;
  if (!instant && caster->SetSpellPreCastingAnimation((ANIMENUMERATION)visualRec->m_anim)) {
    bool specialAnim = visualRec->m_anim == 105 || visualRec->m_anim == 106;
    animSet = caster->SetTorsoAnimation(ANIM_STATE_SPELLPRECAST, specialAnim ? animDuration : 0, specialAnim ? 0x20 : 0);
  }

  caster->StartSpellFizzleTimer(id, duration, animSet);
  if (!instant && (spellRec->m_attributes & 0x400000) && (cast.targets & 2)) {
    caster->SaveTrackingTarget(cast.unitTarget, TRACKTYPE_SPELLPRECAST, 0);
  }
}

void SpellVisualsHandleCastStop(int id, CGUnit_C *caster, BYTE status, BYTE reason) {
  VALIDATEBEGIN;
  VALIDATE(caster);
  VALIDATEENDVOID;

  if (status == 2 && reason == 18) {
    caster->PendingPrecastInterrupt(id);
  } else {
    caster->StopRangedAttackPrecast();
    caster->ClearTrackingTarget(status == 0);
    caster->StopSpellFizzleTimer(id, status);
  }
}

struct EclipseObject {
  NTempest::CImVector color;
  UINT                startTime;
  UINT                fadeInTime;
  UINT                fadeOutTime;
  UINT                endTime;

  void Update(UINT currentTime);
} EclipseObject_HuhHuhHuh_Huh;

static EclipseObject s_eclipseObject;

bool LightningObject::Tick(UINT currentTime) {
  for (UINT i = 0; i < bolts.Count(); ++i) {
    Bolt &bolt = bolts[i];
    if (bolt.srcGuidSub == Bolt::NULL_SUB || bolt.dstGuidSub == Bolt::NULL_SUB) {
      continue;
    }

    CGObject_C *srcObj = ClntObjMgrObjectPtr(guids[bolt.srcGuidSub], __FILE__, __LINE__);
    CGObject_C *dstObj = ClntObjMgrObjectPtr(guids[bolt.dstGuidSub], __FILE__, __LINE__);
    CGUnit_C   *srcUnit =
        srcObj && srcObj->IsA(TYPE_UNIT) ? static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guids[bolt.srcGuidSub], __FILE__, __LINE__)) : 0;
    CGUnit_C *dstUnit =
        dstObj && dstObj->IsA(TYPE_UNIT) ? static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guids[bolt.dstGuidSub], __FILE__, __LINE__)) : 0;

    if (forever || (currentTime >= bolt.birthTime && currentTime < bolt.deathTime)) {
      if (srcObj && dstObj) {
        NTempest::C3Vector sourcePosition(0.0f);

        if (srcUnit) {
          if (i) {
            GetMissileTargetPosition(srcUnit, GetMissileTargetLocation(guids[bolt.srcGuidSub], spellID), sourcePosition);
          } else {
            sourcePosition = GetSpellChainEffectSource(*srcUnit);
          }
        } else {
          sourcePosition = srcObj->GetPosition();
        }

        NTempest::C3Vector destPosition(0.0f);
        if (dstUnit) {
          GetMissileTargetPosition(dstUnit, GetMissileTargetLocation(guids[bolt.dstGuidSub], spellID), destPosition);
        } else {
          destPosition = dstObj->GetPosition();
        }

        if (bolt.boltID == BADBOLT) {
          bolt.boltID = s_lightningManager->Add(
              sourcePosition, destPosition, avgSegLen, width, NTempest::CImVector(-1), noiseScale, texCoordScale, duration,
              texture, 0, 0
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

static NTempest::C3Vector GetSpellChainEffectSource(const CGUnit_C &unit) {
  HMODEL model = unit.GetCharacterModel(0);
  if (!model) {
    return unit.GetPosition();
  }

  NTempest::C3Vector outVect(0.0f, 0.0f, 0.0f);
  if (!ModelGetEventObjectPosition(model, 20, 0, &outVect)) {
    outVect = unit.GetPosition() + NTempest::C3Vector(0.0f, 0.0f, 1.5f) - CGWorldFrame::GetActiveCamera()->Position();
  }
  HandleClose(model);
  return outVect + CGWorldFrame::GetActiveCamera()->Position();
}

void EclipseObject::Update(UINT currentTime) {
  if (startTime == endTime) {
    return;
  }

  float amount = 0.0f;
  if (currentTime < fadeInTime) {
    amount = (float)(currentTime - startTime) / (float)(fadeInTime - startTime);
  } else if (currentTime < fadeOutTime) {
    amount = 1.0f;
  } else if (currentTime < endTime) {
    amount = 1.0f - (float)(currentTime - fadeOutTime) / (float)(endTime - fadeOutTime);
  } else {
    endTime = 0;
    startTime = 0;
  }
  DayNightSetEclipse(color, amount);
}

void SpellVisualsProcedure(
    CGUnit_C                        *caster,
    const SpellVisualKitRec         *kitRec,
    UINT                             spellID,
    const TSStackArray<DWORDLONG>   *targets,
    const TSStackArray<MISS_REASON> *missReasons
) {
  if (caster && kitRec) {
    SpellVisualsProcedureDispatch(kitRec->m_characterProcedure, caster, kitRec, spellID, targets, missReasons);
  }
}

static void SpellVisualsProcedureDispatch(
    int                              proc,
    CGUnit_C                        *caster,
    const SpellVisualKitRec         *kitRec,
    UINT                             spellID,
    const TSStackArray<DWORDLONG>   *targets,
    const TSStackArray<MISS_REASON> *missReasons
) {
  switch (proc) {
    case 0:
      if (targets) {
        CreateLightningObj(caster, targets->Ptr(), targets->Count(), spellID, kitRec, 0, 0);
      }
      break;
    case 6:
      if (!missReasons) {
        SpellVisualsProc_Eclipse(caster, kitRec, spellID);
      }
      break;
  }
}

static void SpellVisualsProc_Eclipse(CGUnit_C *caster, const SpellVisualKitRec *kitRec, UINT spellID) {
  FATALASSERT(kitRec->m_characterParam[1] >= 0.0f && kitRec->m_characterParam[1] <= 1.0f);

  UINT duration = Spell_C_GetCastTime(spellID, 0);
  s_eclipseObject.color = NTempest::CImVector((DWORD)kitRec->m_characterParam[0] | 0xFF000000ul);
  UINT fadeDuration = duration * kitRec->m_characterParam[1];
  s_eclipseObject.startTime = OsGetAsyncTimeMs();
  s_eclipseObject.fadeInTime = s_eclipseObject.startTime + fadeDuration;
  s_eclipseObject.fadeOutTime = s_eclipseObject.startTime + duration;
  s_eclipseObject.endTime = s_eclipseObject.startTime + duration + 100;
}

static void CreateLightningObj(
    const CGUnit_C          *unitPtr,
    const DWORDLONG         *guids,
    int                      numGuids,
    int                      spellID,
    const SpellVisualKitRec *kitRec,
    LightningObject        **objects,
    int                      maxObjects
) {
  if (!kitRec || !unitPtr || !guids || !numGuids || !spellID) {
    return;
  }

  const SpellChainEffectsRec *rec = g_spellChainEffectsDB.GetRecord(kitRec->m_characterParam[0]);
  if (!rec) {
    return;
  }

  CStatus status;
  UINT    currentTime = OsGetAsyncTimeMs();
  UINT    boltCount = NTempest::CMath::ftol_0_256_(kitRec->m_characterParam[1]);
  if (!boltCount) {
    return;
  }
  FATALASSERT(boltCount <= 3);

  int added = 0;
  for (UINT boltIndex = 0; boltIndex < boltCount; ++boltIndex) {
    LightningObject *lightning = s_lightning.NewNode(LIST_TAIL, 0, 0);
    if (added < maxObjects) {
      objects[added++] = lightning;
    }

    lightning->guids.SetCount(numGuids + 1);
    lightning->bolts.SetCount(numGuids);
    lightning->guids[0] = unitPtr->GetGUID();
    lightning->deathTime = currentTime + lightning->bolts.Count() * rec->m_SegDuration;
    lightning->avgSegLen = rec->m_AvgSegLen;
    lightning->width = rec->m_Width;
    lightning->noiseScale = rec->m_NoiseScale;
    lightning->texCoordScale = rec->m_TexCoordScale;
    lightning->duration = rec->m_SegDuration * 0.001f;
    lightning->forever = NTempest::CMath::ftol_0_256_(kitRec->m_characterParam[2]) != 0;
    HTEXTURE texture = TextureCreate(rec->m_Texture, CGxTexFlags(GxTex_Linear, 1, 0, 0, 0, 0, 1), &status, 0);
    lightning->spellID = spellID;
    lightning->texture = texture;
    lightning->AddRef();

    UINT srcGuidSub = 0;
    for (int i = 0; i < numGuids; ++i) {
      LightningObject::Bolt &bolt = lightning->bolts[i];
      bolt.srcGuidSub = LightningObject::Bolt::NULL_SUB;
      bolt.dstGuidSub = LightningObject::Bolt::NULL_SUB;
      bolt.boltID = BADBOLT;

      CGObject_C *target = ClntObjMgrObjectPtr(guids[i], __FILE__, __LINE__);
      if (target && (target->IsA(TYPE_UNIT) || target->IsA(TYPE_GAMEOBJECT))) {
        lightning->guids[i + 1] = guids[i];
        bolt.birthTime = currentTime + i * rec->m_SegDelay;
        bolt.deathTime = bolt.birthTime + rec->m_SegDuration;
        bolt.srcGuidSub = srcGuidSub;
        bolt.dstGuidSub = i + 1;

        if (!lightning->forever && !target->IsA(TYPE_GAMEOBJECT)) {
          CGUnit_C *targetUnit = (CGUnit_C *)target;
          if (i > 0 && i < numGuids - 1) {
            targetUnit->DDADDLOG(unitPtr->GetGUID(), "CreateLightningObj", __FILE__, __LINE__);
          }
          targetUnit->DDADDLOG(unitPtr->GetGUID(), "CreateLightningObj", __FILE__, __LINE__);
        }
        srcGuidSub = bolt.dstGuidSub;
      } else {
        lightning->guids[i + 1] = 0;
        bolt.birthTime = bolt.deathTime = lightning->deathTime;
      }
    }
  }
}

LightningObject::LightningObject() : refCount(1) {
}

void SpellVisualsHandleSpellStart(
    int                            spellID,
    const SpellCast               &cast,
    CGGameObject_C                *caster,
    const TSStackArray<DWORDLONG> &targets,
    bool                           ignoreAreaEffect,
    bool                           hits
) {
  VALIDATEBEGIN;
  VALIDATE(caster);
  VALIDATEENDVOID;
  const SpellRec *srec = g_spellDB.GetRecord(spellID);
  if (!srec) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "NOSPELLIDFOUND|%d", spellID);
    return;
  }
  const SpellVisualRec *visRec = g_spellVisualDB.GetRecord(srec->m_spellVisualID);
  if (!visRec) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "SPELLVISUALIDNOTFOUND|%d", spellID);
    return;
  }

  if (targets.Count() && srec->m_speed <= 0.0f && hits) {
    const SpellVisualKitRec *impactKit = g_spellVisualKitDB.GetRecord(visRec->m_impactKit);
    if (impactKit) {
      UINT i;
      for (i = 0; i < targets.Count(); ++i) {
        CGObject_C *target = ClntObjMgrObjectPtr(targets[i], __FILE__, __LINE__);
        if (target && target->IsA(TYPE_UNIT)) {
          PlayImpactKit((CGUnit_C *)target, impactKit);
        }
      }
    }
  }
  if (!ignoreAreaEffect && (cast.targets & 0x40)) {
    PlayOneShotEffect(cast.destLocation, visRec->m_areaModel, targets);
  }
}

static void PlayOneShotEffect(const NTempest::C3Vector &pos, int effectID, const TSStackArray<DWORDLONG> &objects) {
  UnitEffectOneShot(g_spellVisualEffectNameDB.GetRecord(effectID), pos, &objects, 0.0f, 1.0f);
}

static void PlayImpactKit(CGUnit_C *target, const SpellVisualKitRec *impactKit) {
  UnitEffectOneShot(g_spellVisualEffectNameDB.GetRecord(impactKit->m_headEffect), target, UNITEFFECT_ATTACHHEAD, 0, true, false);
  UnitEffectOneShot(g_spellVisualEffectNameDB.GetRecord(impactKit->m_chestEffect), target, UNITEFFECT_ATTACHCHEST, 0, true, false);
  UnitEffectOneShot(g_spellVisualEffectNameDB.GetRecord(impactKit->m_baseEffect), target, UNITEFFECT_ATTACHBASE, 0, true, false);

  SpellVisualsPlayCameraShakeID(impactKit->m_shakeID, target->GetPosition());
  target->SetSpellImpactKit(impactKit);
  if (impactKit->m_soundID) {
    SndInterfacePlaySpellSound(impactKit->m_soundID, target);
  }
}

UINT SpellGetRangedPrecastHoldAnim(UINT loadAnim) {
  if (loadAnim >= s_precastAnimTransitions.Count()) {
    return INVALID_ANIMATION;
  }
  return s_precastAnimTransitions.Ptr()[loadAnim];
}

void SpellVisualsTick(float elapsed) {
  DWORD currentTime = OsGetAsyncTimeMs();

  {
    SAFEITERATELIST(LightningObject, s_lightning, lightning) {
      if (!lightning->Tick(currentTime)) {
        s_lightning.UnlinkNode(lightning);
        lightning->DelRef();
      }
    }
  }

  s_lightningManager->Update(elapsed);
  s_eclipseObject.Update(currentTime);

  SAFEITERATELIST(BlizzardObject, s_blizzard, blizzard) {
    blizzard->Update();
  }
}

void SpellVisualsRender() {
  s_lightningManager->Render(CGWorldFrame::GetActiveCamera()->Position());
  RenderFishingLines();
}

static void RenderFishingLines() {
  SAFEITERATELIST(FishingLineObject, s_fishingLineObjects, object) {
    object->Render();
  }
}

void SpellVisualsPlayCameraShakeID(UINT shakeID, const NTempest::C3Vector &position) {
  const SpellEffectCameraShakesRec *rec = g_spellEffectCameraShakesDB.GetRecord(shakeID);
  if (!rec) {
    return;
  }

  for (UINT i = 0; i < 3; ++i) {
    if (rec->m_CameraShake[i]) {
      CGWorldFrame::GetActiveCamera()->AddShake(rec->m_CameraShake[i], position);
    }
  }
}

void SpellVisualGetLightning(const CGUnit_C *unitPtr, const SpellVisualKitRec *kitRec, int spellID, LightningObject **objects, int numObjects) {
  if (unitPtr) {
    const TSGrowableArray<DWORDLONG> &targets = unitPtr->GetSavedChannelSpellTargets();
    CreateLightningObj(unitPtr, targets.Ptr(), targets.Count(), spellID, kitRec, objects, numObjects);
  }
}

void SpellVisualClearLightning(LightningObject *lightning) {
  lightning->deathTime = OsGetAsyncTimeMs();
  lightning->forever = 0;
  lightning->DelRef();
}

BlizzardObject *SpellVisualsBlizzardCreate(const NTempest::C3Vector &pos, float radius, int spellID, const SpellVisualKitRec *kitRec) {
  static LPCSTR modelNames[4] = {
      "Spells\\Blizzard_Impact_Base.mdx", "Spells\\RainOfFire_Impact_Base.mdx", "Spells\\CallLightning_Impact.mdx",
      "Spells\\FlamestrikeSmall_Impact_Base.mdx"
  };
  UINT nameSub;

  BlizzardObject *blizzard = AllocBlizzard();

  nameSub = NTempest::CMath::ftol_0_256_(kitRec->m_characterParam[0]);
  FATALASSERT(nameSub < (sizeof(modelNames) / sizeof(modelNames[0])));
  blizzard->Init(pos, modelNames[nameSub], radius, kitRec->m_characterParam[1]);
  return blizzard;
}

static BlizzardObject *AllocBlizzard() {
  if (!s_blizzardPool.Head()) {
    s_blizzardPool.NewNode(LIST_TAIL, 0, 0);
  }

  BlizzardObject *bliz = s_blizzardPool.Head();
  s_blizzard.LinkNode(bliz, LIST_TAIL, 0);
  return bliz;
}

void SpellVisualsBlizzardDestroy(BlizzardObject *&blizzard) {
  blizzard->dead = 1;
  blizzard = 0;
}

FishingLineObject *SpellVisualsFishingLineCreate(const SpellVisualKitRec *kitRec, const DWORDLONG &gameObj, const DWORDLONG &caster) {
  if (!kitRec) {
    return 0;
  }
  FishingLineObject *object = s_freeFishingObjects.Get(0);
  object->object = gameObj;
  object->caster = caster;
  object->color.Set((UINT)kitRec->m_characterParam[0] | 0xFF000000);
  object->visible = 0;
  s_fishingLineObjects.LinkNode(object, LIST_TAIL, 0);
  return object;
}

void SpellVisualsFishingLineDestroy(FishingLineObject *object) {
  if (object) {
    s_freeFishingObjects.Put(object);
  }
}

void SpellVisualFishingLineSetVisible(FishingLineObject *obj) {
  if (obj) {
    obj->visible = 1;
  }
}

LightningObject::~LightningObject() {
  UINT index;

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
  for (UINT effect = 0; effect < 3; ++effect) {
    if (rec->m_effect[effect] == 6 && (rec->m_effectAura[effect] == 36 || rec->m_effectAura[effect] == 56)) {
      return true;
    }
  }
  return false;
}

void SpellVisualsPlayKit(CGUnit_C *target, UINT id) {
  const SpellVisualKitRec *kitRec = g_spellVisualKitDB.GetRecord(id);
  if (kitRec) {
    PlayImpactKit(target, kitRec);
  }
}

void HandleMissileEffects(
    CGUnit_C                        *caster,
    const SpellRec                  *srec,
    const SpellVisualRec            *visRec,
    int                              ammoDisplayID,
    int                              ammoInventoryType,
    const SpellCast                 &cast,
    const TSStackArray<DWORDLONG>   &targets,
    const TSStackArray<MISS_REASON> *missReasons,
    bool                             wasProc
) {
  float missileSpeed = srec->m_speed;
  if (!caster || !targets.Count() || (!visRec->m_hasMissile && !ammoDisplayID)) {
    return;
  }

  if (cast.targets & 0x802) {
    UINT i;
    for (i = 0; i < targets.Count(); ++i) {
      CGObject_C *target = ClntObjMgrObjectPtr(targets[i], __FILE__, __LINE__);
      if (target) {
        MISS_REASON reason = missReasons ? (*missReasons)[i] : MISS_PHYSICAL;
        caster->StoreSpellMissileEffect(
            targets[i], target->GetPosition(), missileSpeed, ammoDisplayID, ammoInventoryType, visRec, missReasons == 0, reason, srec->m_ID, wasProc
        );
      }
    }
  } else if (cast.targets & 0x40) {
    caster->StoreSpellMissileEffect(
        0, cast.destLocation, missileSpeed, ammoDisplayID, ammoInventoryType, visRec, missReasons == 0, MISS_PHYSICAL, srec->m_ID, wasProc
    );
  }
}

void SpellVisualsHandleSpellStartHits(
    int                            spellID,
    const SpellCast               &cast,
    CGUnit_C                      *caster,
    const TSStackArray<DWORDLONG> &targets,
    int                            ammoDisplayID,
    int                            ammoInventoryType,
    int                            flags
) {
  bool                     ignoreAreaEffect = (flags & 8) != 0;
  bool                     wasProc = (flags & 1) != 0;
  SpellVisualRec           visRecData;
  const SpellRec          *srec;
  const SpellVisualRec    *visRec;
  const SpellVisualKitRec *kitRec;
  if (!GetSpellRecords(caster, spellID, srec, visRecData, visRec, kitRec)) {
    return;
  }

  if (!ignoreAreaEffect && !wasProc && (cast.targets & 0x40)) {
    PlayOneShotEffect(cast.destLocation, visRec->m_areaModel, targets);
  }
  int torsoAnimSet = 0;
  if (!wasProc && kitRec) {
    PlayCastAnim(caster, srec, visRec, kitRec, targets, torsoAnimSet);
  }
  if (kitRec && !wasProc) {
    SpellVisualsProcedure(caster, kitRec, spellID, &targets, 0);
  }

  if (srec->m_speed <= 0.0f) {
    caster->MaybeSaveChannelSpellTargets(spellID, targets);
    const SpellVisualKitRec *impactKit = g_spellVisualKitDB.GetRecord(visRec->m_impactKit);
    if (impactKit && !IsSpellAura(srec)) {
      UINT i;
      for (i = 0; i < targets.Count(); ++i) {
        CGObject_C *target = ClntObjMgrObjectPtr(targets[i], __FILE__, __LINE__);
        if (target && target->IsA(TYPE_UNIT)) {
          caster->SetImpactKitEffect(spellID, (CGUnit_C *)target, impactKit, torsoAnimSet == 0);
        }
      }
    }
  } else {
    HandleMissileEffects(caster, srec, visRec, ammoDisplayID, ammoInventoryType, cast, targets, 0, wasProc);
  }
}

static bool GetSpellRecords(
    CGUnit_C                 *caster,
    int                       spellID,
    const SpellRec          *&srec,
    SpellVisualRec           &visRecData,
    const SpellVisualRec    *&visRec,
    const SpellVisualKitRec *&kitRec
) {
  srec = g_spellDB.GetRecord(spellID);
  if (!srec) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "NOSPELLIDFOUND|%d", spellID);
    return 0;
  }
  visRec = caster->GetAppropriateSpellVisual((SpellRec *)srec, visRecData);
  if (!visRec) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "SPELLVISUALIDNOTFOUND|%d", spellID);
    return 0;
  }
  kitRec = caster->GetRangedSpellAnim(spellID, 1);
  return 1;
}

static void PlayCastAnim(
    CGUnit_C                      *caster,
    const SpellRec                *srec,
    const SpellVisualRec          *visRec,
    const SpellVisualKitRec       *kitRec,
    const TSStackArray<DWORDLONG> &targets,
    int                           &torsoAnimSet
) {
  FATALASSERT(caster->IsA(TYPE_UNIT));
  if (kitRec->m_anim) {
    ANIMENUMERATION finalAnim;
    bool            animValid = caster->SetSpellCastingAnimation(
        (ANIMENUMERATION)kitRec->m_anim, visRec->m_castKit, kitRec->m_soundID, kitRec->m_shakeID, finalAnim
    );
    caster->AddSpellProcOneShotEffect(srec->m_ID, kitRec);
    SpellVisualsPlayCameraShakeID(kitRec->m_shakeID, caster->GetPosition());
    int oldCastingSpell = caster->SetCastingSpell(srec->m_ID, 0, 0);
    if (animValid) {
      torsoAnimSet = caster->SetTorsoAnimation(ANIM_STATE_SPELLCAST, 0, 0);
    }
    if (torsoAnimSet) {
      caster->HandlePrecastStop(srec->m_ID, true);
      caster->SetSheatheReason(SHEATHE_SPELLS, (srec->m_attributes & 0x40000) == 0, false);
    } else {
      caster->ClearSpellCastAnimInfo();
    }
    if (!oldCastingSpell) {
      caster->SetCastingSpell(0, 1, 0);
    }
    if (torsoAnimSet && Object_C_AnimHasHitEvent(finalAnim)) {
      caster->AddHitAnimHolds(srec->m_ID, targets);
    }
    caster->SetCastingSpell(0, 0, 0);
  } else {
    SndInterfacePlaySpellSound(kitRec->m_soundID, caster);
    SpellVisualsPlayCameraShakeID(kitRec->m_shakeID, caster->GetPosition());
  }
  if (!torsoAnimSet && caster->GetCurrentTorsoAnimState() == 37) {
    caster->ClearTorsoAnimation(0);
  }
}

void SpellVisualsHandleSpellStartMisses(
    int                            spellID,
    const SpellCast               &cast,
    CGUnit_C                      *caster,
    const TSStackArray<DWORDLONG> &targets,
    TSStackArray<MISS_REASON>     &missReasons,
    int                            ammoDisplayID,
    int                            ammoInventoryType,
    int                            flags
) {
  if (!targets.Count()) {
    return;
  }
  bool                     ignoreAreaEffect = (flags & 8) != 0;
  bool                     wasProc = (flags & 1) != 0;
  SpellVisualRec           visRecData;
  const SpellRec          *srec;
  const SpellVisualRec    *visRec;
  const SpellVisualKitRec *kitRec;
  if (!GetSpellRecords(caster, spellID, srec, visRecData, visRec, kitRec)) {
    return;
  }

  if (caster->GetCurrentTorsoAnimState() == 37) {
    caster->ClearTorsoAnimation(0);
  }
  if (!ignoreAreaEffect && !wasProc && (cast.targets & 0x40)) {
    PlayOneShotEffect(cast.destLocation, visRec->m_areaModel, targets);
  }
  int dummy = 0;
  if (!wasProc && kitRec) {
    PlayCastAnim(caster, srec, visRec, kitRec, targets, dummy);
  }
  if (kitRec && !wasProc) {
    SpellVisualsProcedure(caster, kitRec, spellID, &targets, &missReasons);
  }

  for (int i = 0; i < (int)targets.Count(); ++i) {
    CGObject_C *target = ClntObjMgrObjectPtr(targets[i], __FILE__, __LINE__);
    if (target && target->IsA(TYPE_UNIT)) {
      missReasons[i] = ((CGUnit_C *)target)->AdjustVictimState(missReasons[i]);
    }
  }

  if (srec->m_speed <= 0.0f) {
    for (UINT i = 0; i < targets.Count(); ++i) {
      CGObject_C *targetObject = ClntObjMgrObjectPtr(targets[i], __FILE__, __LINE__);
      if (targetObject && targetObject->IsA(TYPE_UNIT) && caster->GetGUID() == ClntObjMgrGetActivePlayer()) {
        CGUnit_C *target = (CGUnit_C *)targetObject;
        if (srec->m_attributes & 0x404) {
          CGPlayer_C::AddDeferredSpellMiss(targets[i], target->AdjustVictimState(missReasons[i]), spellID);
        } else {
          target->AddWorldText(missReasons[i]);
          UnitCombatLogSpellMissed(missReasons[i], spellID, caster->GetGUID(), target->GetGUID());
        }
      }
    }
  } else {
    HandleMissileEffects(caster, srec, visRec, ammoDisplayID, ammoInventoryType, cast, targets, &missReasons, wasProc);
  }
}

LPCSTR GetSpellAuraEffectName(int effectID) {
  if (effectID >= (int)s_auraNames.Count()) {
    return "INVALID_SPELL_AURA_EFFECT";
  }
  return s_auraNames[effectID]->m_name_lang[0];
}

LPCSTR GetSpellAuraEffectToken(int effectID) {
  if (effectID >= (int)s_auraNames.Count()) {
    return "INVALID_SPELL_AURA_EFFECT";
  }
  return s_auraNames[effectID]->m_globalstrings_tag;
}
