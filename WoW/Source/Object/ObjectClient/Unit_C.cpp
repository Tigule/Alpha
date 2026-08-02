#include "Object/ObjectClient/Unit_C.h"
#include "Magic/MagicClient/Spell_C.h"
#include "Object/ObjectClient/GameObject_C.h"
#include "DB/DBClient/AutoCode/SkillLineAbilityRec.h"

#include <Base/Status.h>
#include <Base/CDataAllocator.h>
#include <Services/SysMessage.h>
#include <Services/Texture.h>
#include <Os/W32/OsSound.h>
#include <Os/W32/Debugging.h>
#include <Os/OsTime.h>
#include <Base/CDataStore.h>
#include <FrameScript/FrameScript.h>
#include <Event/EvtApi.h>
#include <Gx/Gx.h>
#include <Model/CollisionData.h>

#include <math.h>
#include <float.h>
#include <malloc.h>
#include <string.h>
#include <Tempest/c44matrix.h>
#include <Tempest/c34matrix.h>
#include <Tempest/c3spline.h>
#include <Tempest/c4plane.h>
#include <Tempest/caasphere.h>
#include <Tempest/caabox.h>
#include <Tempest/cimvector.h>

#include "DB/DBClient/AutoCode/CreatureDisplayInfoRec.h"
#include "DB/DBClient/AutoCode/CreatureDisplayInfoExtraRec.h"
#include "DB/DBClient/AutoCode/ChrRacesRec.h"
#include "DB/DBClient/AutoCode/CreatureModelDataRec.h"
#include "DB/DBClient/AutoCode/CreatureSoundDataRec.h"
#include "DB/DBClient/AutoCode/NPCSoundsRec.h"
#include "DB/DBClient/AutoCode/UnitBloodLevelsRec.h"
#include "DB/DBClient/AutoCode/UnitBloodRec.h"
#include "DB/DBClient/AutoCode/EmotesRec.h"
#include "DB/DBClient/AutoCode/EmoteAnimsRec.h"
#include "DB/DBClient/AutoCode/FactionTemplateRec.h"
#include "DB/DBClient/AutoCode/FactionRec.h"
#include "DB/DBClient/AutoCode/ItemDisplayInfoRec.h"
#include "DB/DBClient/AutoCode/ItemSubClassRec.h"
#include "DB/DBClient/AutoCode/MapRec.h"
#include "DB/DBClient/AutoCode/SpellRec.h"
#include "DB/DBClient/AutoCode/SpellShapeshiftFormRec.h"
#include "DB/DBClient/AutoCode/SpellCastTimesRec.h"
#include "DB/DBClient/AutoCode/SpellVisualRec.h"
#include "DB/DBClient/AutoCode/SpellVisualKitRec.h"
#include "DB/DBClient/AutoCode/SpellVisualEffectNameRec.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "DB/DBClient/DBClient.h"
#include "Client.h"
#include "Game/GameTime.h"
#include "Console/ConsoleVar.h"
#include "Console/ConsoleClient.h"
#include "Component/CharacterCustomization.h"
#include "Component/Component.h"
#include "Game/GameClient/PlayerName.h"
#include "Object/CreatureStats.h"
#include "Object/GuildStats.h"
#include "Object/ObjectClient/Item_C.h"
#include "Object/ObjectClient/NPC_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "SoundInterface/SoundInterface.h"
#include "Ui/ActionBarFrame.h"
#include "UIUtil/Camera.h"
#include "UIUtil/InputControl.h"
#include "UIUtil/Tooltip.h"
#include "Ui/GameUI.h"
#include "Ui/NamePlateFrame.h"
#include "Ui/PartyFrame.h"
#include "Ui/PetInfo.h"
#include "Ui/QuestFrame.h"
#include "Ui/ReputationInfo.h"
#include "Ui/SpellBookFrame.h"
#include "Ui/WorldFrame.h"
#include "WorldClient/World.h"
#include "WorldClient/AreaList.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

enum REPLACEABLE_MATERIAL_IDS {
  TEX_COMPONENT_SKIN = 1,
  TEX_COMPONENT_OBJECT_SKIN = 2,
  TEX_COMPONENT_WEAPON_BLADE = 3,
  TEX_COMPONENT_WEAPON_HANDLE = 4,
  TEX_COMPONENT_ENVIRONMENT = 5,
  TEX_COMPONENT_CHAR_HAIR = 6,
  TEX_COMPONENT_CHAR_FACIAL_HAIR = 7,
  TEX_COMPONENT_SKIN_EXTRA = 8,
  TEX_COMPONENT_UI_SKIN = 9,
  TEX_COMPONENT_TAUREN_MANE = 10,
  TEX_COMPONENT_MONSTER_1 = 11,
  TEX_COMPONENT_MONSTER_2 = 12,
  TEX_COMPONENT_MONSTER_3 = 13,
  TEX_COMPONENT_ITEM_ICON = 14,
  NUM_REPLACEABLE_MATERIAL_IDS = 15
};

struct LightningObject;
void UnitEffectsInitialize();
void UnitEffectsShutdown();
void UnitSoundInitialize();
void UnitSoundShutdown();
void Spell_C_Initialize();
void Spell_C_Destroy();
void UnitCombatClientInitialize();
void UnitCombatClientShutdown();
void ShadowInit();
void ShadowDestroy();
void UnitFootprintInitialize();
void UnitFootprintShutdown();
void UnitFootprintNewSplat(
    unsigned int               textureID,
    const NTempest::C2Vector  &size,
    const NTempest::C3Vector  &position,
    float                      facing,
    int                        mirrorLength,
    unsigned int               terrain
);
void UnitFootprintPlayParticle(
    CGUnit_C                  *unit,
    const NTempest::C3Vector &position,
    unsigned int              terrainID,
    float                     scale
);
ANIMENUMERATION Object_C_GetAnimIndex(const char *animName);
void UnitEffectClear(CGObject_C *object);
void WeaponTrailDisableDrawing(int trail);
void BotClientLoseTarget(const CGUnit_C *unit);
void ScriptEventsRegisterUnit(CGUnit_C *unit);
void ScriptEventsUnregisterUnit(CGUnit_C *unit);
FishingLineObject *SpellVisualsFishingLineCreate(
    const SpellVisualKitRec *kitRec,
    const unsigned __int64  &gameObj,
    const unsigned __int64  &caster
);
void SpellVisualsFishingLineDestroy(FishingLineObject *object);
void SpellVisualFishingLineSetVisible(FishingLineObject *object);
void SpellVisualClearLightning(LightningObject *lightning);
void SpellVisualGetLightning(
    const CGUnit_C *unitPtr, const SpellVisualKitRec *kitRec, int spellID, LightningObject **objects, int numObjects
);
int SpellFizzleTimer(const void *data, void *userData);
void UpdatePortraitTexture(const unsigned __int64 &guid);
bool Spell_C_IsModal();
int Spell_C_GetSpellCooldown(int spellID, int isPet, unsigned int *duration, unsigned long *startTime, unsigned int *enable);
int Spell_C_GetItemCooldown(int itemID, unsigned int *duration, unsigned long *startTime, unsigned int *enable);
int Spell_C_GetManaCost(int spellID, int isPet);
void Spell_C_SpellFailed(int spellID, unsigned char reason, int arg1, int arg2);
const unsigned __int64 &Spell_C_GetCurrentTarget();
void Spell_C_CancelSpell(bool failed, bool notifyServer, SPELL_FAILED_REASON reason);
void UnitCombatLogUnitDead(unsigned __int64 unit);
void            UnitFootprintNewBloodSplat(const UnitBloodRec *rec, unsigned int unitSize, const NTempest::C3Vector &position);
static void PlayerNameGuildCallback(int guildID, const unsigned __int64 &guid, void *arg, bool granted);

NODEDECL(BLOODSPLATNODE) {
  unsigned int       m_triggerTime;
  NTempest::C3Vector m_position;
};

int OnPickNextStandHandler(void *param, CGUnit_C *ptr) {
  ptr->OnPickNextStandHandler();
  return 1;
}

void CGUnit_C::Disable(int shutdown) {
  ClearMeleeDeathHold();
  m_savedChannelSpellTargets.SetCount(0);
  m_animFlags &= ~0xC000U;
  m_emoteQueue.SetCount(0);

  while (BLOODSPLATNODE *node = m_bloodSplatNodes.Head()) {
    delete node;
  }

  KillSpellLoopedSound();
  KillCreatureLoopSound();
  m_pendingHitAnimVictims.SetCount(0);
  ProcessAnimEndCallbacks();
  CheckPendingVictimFeedback();
  CheckPendingMissileRelease(0);
  ClearRangedStandTimer();
  FinishAuraDecays();
  m_questGiverStatus = QUEST_GIVER_NONE;
  PurgeAnimNodes(false);
  RemoveBloodPool();
  RemoveInteractIcon();
  UnitEffectClear(this);
  UnsetMirrorHandlers();
  if (m_spellFizzleTimer) {
    ClientKillTimer(m_spellFizzleTimer, SpellFizzleTimer, "SpellFizzleTimer");
  }
  m_spellFizzleTimer = 0;
  RemoveUnitNamePlate();

  if (!CGPartyInfo::IsMember(GetGUID())) {
    CGGameUI::ClearTarget(GetGUID(), 1);
  }

  BotClientLoseTarget(this);
  DestroyFadingMounts();
  CGWorldFrame::RegisterObjectFadeoutModel(this, m_texComponent, m_alpha);
  RemoveWorldObject();
  FATALASSERT(!m_currentDamageInfo);
  m_move.CMovementData::~CMovementData();

  if (m_scriptRegistered) {
    ScriptEventsUnregisterUnit(this);
  }

  CGObject_C::Disable(shutdown);
  ClearWeaponTrailHandles();
  ProcessQuestItemMessages();
  ClearFishingObject();
  m_flags &= ~4U;
}

void CGUnit_C::Reenable() {
  CGObject_C::Reenable();
  SetMirrorHandlers();
  UpdateDisplayInfo();
  UpdateUnitAlpha();
  float facing = m_move.m_facing;
  SetSmoothFacing(facing);
  m_displayFacing = facing;
  AddWorldObject();

  if (m_unit->health > 0 && static_cast<float>(m_unit->health) / static_cast<float>(m_unit->maxHealth) < 0.2f) {
    AddBloodPool();
  }
}

void CGUnit_C::PostReenable() {
  CGObject_C::PostReenable();
  RefreshSpellProcEffects();
  InitializeLoopSound();
  RefreshInteractIcon();
  DetermineReadySequence(0);

  unsigned int flags = 0x100;
  if (m_unit->health > 0) {
    ClearResEffectModel();
  } else {
    InitializeResEffectModel();
    flags = 0x180;
  }

  UpdateBaseAnimation(flags);
  ClearTorsoAnimation(0);
  LookAtTarget();
  UpdateUnitMountInfo(1, 0xFFFFFFFF);
  RefreshInteractIcon();
  if (m_scriptRegistered) {
    ScriptEventsRegisterUnit(this);
  }
}

void CGUnit_C::DestroyFadingMounts() {
  if (m_fadingPureMountModel) {
    HandleClose(m_fadingPureMountModel);
    m_fadingPureMountModel = 0;
  }
}

void CGUnit_C::ClearFishingObject() {
  if (m_fishingLineObject) {
    SpellVisualsFishingLineDestroy(m_fishingLineObject);
  }
  m_fishingLineObject = 0;
}

void CGUnit_C::ProcessChannelObject() {
  CGObject_C *gameObj = ClntObjMgrObjectPtr(m_unit->channelObject, __FILE__, __LINE__);
  int         spellID = m_unit->channelSpell;
  if (gameObj && spellID) {
    if (m_fishingLineObject) {
      if (m_currentTorsoAnimState == ANIM_STATE_CHANNELSPELL) {
        SpellVisualFishingLineSetVisible(m_fishingLineObject);
      }
    } else {
      const SpellRec          *spell = g_spellDB.GetRecord(spellID);
      const SpellVisualRec    *visual = spell ? g_spellVisualDB.GetRecord(spell->m_spellVisualID) : 0;
      const SpellVisualKitRec *kit = visual ? g_spellVisualKitDB.GetRecord(visual->m_channelKit) : 0;
      if (kit && kit->m_characterProcedure == 10) {
        m_fishingLineObject = SpellVisualsFishingLineCreate(kit, m_unit->channelObject, GetGUID());
      }
    }
  } else {
    ClearFishingObject();
  }
}

int DeathAnimEndHandler(void *param, CGUnit_C *ptr) {
  ptr->DeathAnimEndHandler();
  return 1;
}

int PickNextRunHandler(void *param, CGUnit_C *ptr) {
  ptr->PickNextRunHandler();
  return 1;
}

int WoundAnimEndHandler(void *param, CGUnit_C *ptr) {
  ptr->WoundAnimEndHandler();
  return 1;
}

int SpellAnimEndHandler(void *param, CGUnit_C *ptr) {
  ptr->SpellAnimEndHandler();
  return 1;
}

int NPCAnimEndHandler(void *param, CGUnit_C *ptr) {
  ptr->NPCAnimEndHandler();
  return 1;
}

int JumpTakeOffFinishedHandler(void *param, CGUnit_C *ptr) {
  ptr->JumpTakeOffFinishedHandler();
  return 1;
}

int JumpLandFinishedHandler(void *param, CGUnit_C *ptr) {
  ptr->JumpLandFinishedHandler();
  return 1;
}

void CGUnit_C::UnitHit(VICTIMSTATES, unsigned __int64) {
}

struct INTERACTICONTYPEINFO {
  HMODEL       model;
  STRINGLOOKUP string;

  INTERACTICONTYPEINFO(STRINGLOOKUP lookup) : model(0), string(lookup) {
  }
};

static INTERACTICONTYPEINFO s_interactIconModelInfo[5] = {
    INTERACTICONTYPEINFO(SLOOKUP_QUESTGIVERINDICATORMODEL), INTERACTICONTYPEINFO(SLOOKUP_QUESTGIVERINDICATORMODELCOMPLETION),
    INTERACTICONTYPEINFO(SLOOKUP_QUESTGIVERINDICATORMODELFUTURE), INTERACTICONTYPEINFO(SLOOKUP_TAXINODEINDICATORMODEL),
    INTERACTICONTYPEINFO(SLOOKUP_BINDERINDICATORMODEL)
};

static const char *s_interactIconAnimNames[2] = {"stand", "StandHigh"};

static INTERACTICONTYPE s_questIconInfo[5] = {
    INTERACTICON_NONE, INTERACTICON_NONE, INTERACTICON_FUTURE, INTERACTICON_COMPLETION, INTERACTICON_NORMAL
};

static LISTDECL(BLOODSPLATNODE, s_bloodSplatList);

struct NAMEPLATEDESC : public TSHashObject<NAMEPLATEDESC, CHashKeyGUID> {
  LINKDECLEX(NAMEPLATEDESC, m_sortLink);
  float                 screenSortOrder;
  CGUnit_C             *unit;
  CGNamePlateFrame     *namePlate;
  NTempest::C2Vector    screenCoords;

  NAMEPLATEDESC() : screenSortOrder(0.0f), unit(0), screenCoords(0.0f) {
  }
  NAMEPLATEDESC(const NAMEPLATEDESC &);
  ~NAMEPLATEDESC();
};

NODEDECL(FREENAMEPLATE) {
  CGNamePlateFrame *namePlate;

  FREENAMEPLATE() : namePlate(0) {
  }
  ~FREENAMEPLATE();
};

FREENAMEPLATE::~FREENAMEPLATE() {
  DEL(namePlate);
}

static int                                              s_drawNameplates = 1;
static LISTDECLEX(NAMEPLATEDESC, m_sortLink, s_namePlateList);
static const float                                      MAX_NAMEPLATE_DIST = 20.0f;
static const float                                      MAX_NAMEPLATE_DIST_SQ = MAX_NAMEPLATE_DIST * MAX_NAMEPLATE_DIST;
static LISTDECL(FREENAMEPLATE, s_freeNamePlateList);
static TSHashTable<NAMEPLATEDESC, CHashKeyGUID>         s_monsterNamePlateList;
static CGWorldFrame                                    *s_namePlateWorldFrame;

int RangedWeaponAnimEndHandler(void *param, CGUnit_C *ptr) {
  ptr->RangedWeaponAnimEndHandler();
  return 1;
}

typedef LIST(SPELLEFFECTDESC) SpellEffectList;
typedef void(*SpellProcHandler)(
    SPELLPROC_ACTION         action,
    SpellEffectList         &list,
    CGUnit_C                *unit,
    HMODEL                   charModel,
    const SpellVisualKitRec *rec,
    SPELLEFFECTDESC         *newDesc,
    unsigned int             spellID,
    float                    elapsed
);

void SpellVisualsHandleCastStop(int id, CGUnit_C *caster, unsigned char status, unsigned char reason);
void UnitEffectClearSpellPrecast(CGObject_C *object, int spellID);
void SndInterfaceAssociateSoundWithObject(Sound *sound, CGObject_C *objectPtr);
const ItemSubClassRec *SDBItemSubclassGetSubClassRec(unsigned int classID, unsigned int subClassID);
int SheatheTypeToSheathePoint(int sheatheType, int invSlot);

enum SAVEDSHEATHATTACHPOINTS {
  SHEATHATTACH_NONE = 0,
  SHEATHATTACH_MAINHAND = 1,
  SHEATHATTACH_OFFHAND = 2,
  SHEATHATTACH_LARGEWEAPONLEFT = 3,
  SHEATHATTACH_LARGEWEAPONRIGHT = 4,
  SHEATHATTACH_HIPWEAPONLEFT = 5,
  SHEATHATTACH_HIPWEAPONRIGHT = 6,
  SHEATHATTACH_SHIELD = 7,
  SHEATHATTACH_NUM_SAVESSHEATHATTACHPOINTS = 8
};

static SAVEDSHEATHATTACHPOINTS s_sheathePoints[2][9] = {
    {SHEATHATTACH_NONE, SHEATHATTACH_MAINHAND, SHEATHATTACH_LARGEWEAPONRIGHT, SHEATHATTACH_HIPWEAPONLEFT,
     SHEATHATTACH_SHIELD, SHEATHATTACH_NONE, SHEATHATTACH_NONE, SHEATHATTACH_NONE, SHEATHATTACH_NONE},
    {SHEATHATTACH_NONE, SHEATHATTACH_OFFHAND, SHEATHATTACH_LARGEWEAPONLEFT, SHEATHATTACH_HIPWEAPONRIGHT,
     SHEATHATTACH_SHIELD, SHEATHATTACH_NONE, SHEATHATTACH_NONE, SHEATHATTACH_NONE, SHEATHATTACH_NONE}
};

static const int s_savedSheathToAttachPoints[8] = {-1, 26, 27, 30, 31, 32, 33, 28};
void WeaponTrailClose(int trail);
int WeaponTrailCreate(HMODEL model);
void WeaponTrailSetDrawing(int trail, const NTempest::CImVector &color, int fadeOutRate, unsigned int duration);
int Spell_C_GetCastTime(int id, int isPet);
int UnitEffectGetSpecialVisual(UNITEFFECTSPECIALS effectNumber);
void UnitEffectOneShot(
    UNITEFFECTSPECIALS effectNumber,
    unsigned __int64   target,
    const NTempest::C3Vector *attachPos,
    float              facing,
    float              scale,
    bool               forceEffectOnMount
);
void UnitCombatLogAuraAddedOrRemoved(CGUnit_C *unitPtr, int spellID, bool added, int auraSlot);
int GetObjComponentInfo(
    int     race,
    int     sex,
    int     displayID,
    int     inventoryType,
    bool    useMonsterComponent,
    bool    forceAlternate,
    HMODEL *models,
    int    *attachmentPoints
);
void CreatureQueryCallback(int id, const unsigned __int64 &guid, void *arg, bool granted);
void Script_SendUnitSignal(const unsigned __int64 &guid, int signal);
bool IsShapeshiftSpell(const SpellRec *rec);
void SndInterfacePlaySpellSound(int soundID, CGUnit_C *obj);
void SpellVisualsPlayCameraShakeID(unsigned int shakeID, const NTempest::C3Vector &position);
void UnitEffectAddMissile(const MISSILESTRUCT &desc, int durationOffset);
GEOCOMPONENTLINKS UnitEffectGetLinkPointFromAttachment(UNITEFFECTATTACHPPOINT attach);
HMODEL UnitEffectCreateAuraModel(unsigned int effectID);
bool UnitEffectIsAuraWorldObject(unsigned int effectID, bool &isWorldObj);
unsigned long UnitEffectCreateWorldModelAura(unsigned int effect, const NTempest::C3Vector &location, float facing);
int OnFirstAuraSequenceFinished(void *param);
bool AnimSheathesWeapon(unsigned int anim);
int GetObjAnimFlags(int unitAnimFlags);
unsigned int SpellGetRangedPrecastHoldAnim(unsigned int loadAnim);
unsigned int PlayerNameGetUnitNameMode();
void SpellVisualsPlayCastKit(CGUnit_C *caster, const SpellVisualKitRec *kitRec, int spellID, bool isCastEffect);
bool Object_C_AnimHasHitEvent(int anim);
float CalculateFacingTo(const NTempest::C3Vector &position, const NTempest::C3Vector &destination);
void UnitEffectOneShot(
    const SpellVisualEffectNameRec *effect,
    CGObject_C                     *object,
    UNITEFFECTATTACHPPOINT          attachPoint,
    int                             spellID,
    bool                            isCastEffect,
    bool                            forceEffectOnMount
);

NODEDECL(AuraDecayNode) {
  AuraVisual             visual;
  unsigned __int64       unit;
  UNITEFFECTATTACHPPOINT attach;

  ~AuraDecayNode();
};

AuraDecayNode::~AuraDecayNode() {
  visual.Clear();
}

int OnAuraDecayFinished(void *param);

int AuraMirrorHandler(unsigned __int64 guid, unsigned int offset, unsigned int bytes, const void *prevValue, void *param);
#define DECLARE_UNIT_MIRROR_HANDLER(name) int name(unsigned __int64, unsigned int, unsigned int, const void *, void *)
DECLARE_UNIT_MIRROR_HANDLER(UnitFlagUpdateHandler);
DECLARE_UNIT_MIRROR_HANDLER(UnitLevelUpdateHandler);
DECLARE_UNIT_MIRROR_HANDLER(UnitModeUpdateHandler);
DECLARE_UNIT_MIRROR_HANDLER(UnitHealthUpdateHandler);
DECLARE_UNIT_MIRROR_HANDLER(UnitCharmedUpdateHandler);
DECLARE_UNIT_MIRROR_HANDLER(DisplayIDUpdateHandler);
DECLARE_UNIT_MIRROR_HANDLER(StandStateUpdateHandler);
DECLARE_UNIT_MIRROR_HANDLER(NPCFlagsHandler);
DECLARE_UNIT_MIRROR_HANDLER(WeaponModeUpdateHandler);
DECLARE_UNIT_MIRROR_HANDLER(PetNameChangeHandler);
DECLARE_UNIT_MIRROR_HANDLER(VirtualItemChangeHandler);
DECLARE_UNIT_MIRROR_HANDLER(DynamicFlagsChangeHandler);
DECLARE_UNIT_MIRROR_HANDLER(EmoteStateChangeHandler);
DECLARE_UNIT_MIRROR_HANDLER(ChannelSpellChangeHandler);
#undef DECLARE_UNIT_MIRROR_HANDLER

static const REPLACEABLE_MATERIAL_IDS s_materialIDs[3] = {TEX_COMPONENT_MONSTER_1, TEX_COMPONENT_MONSTER_2, TEX_COMPONENT_MONSTER_3};

enum OBJATTACHMENTPOINTS {
  OBJATTACH_HELMET = 0,
  OBJATTACH_SHOULDERPAD = 1,
  OBJATTACH_MAINHAND = 2,
  OBJATTACH_OFFHAND = 3,
  OBJATTACH_RANGED = 4,
  OBJATTACH_NUM = 5
};

static int s_invSlotToObjAttachSlot[EQUIPPED_LAST + 1] = {
    OBJATTACH_HELMET,
    -1,
    OBJATTACH_SHOULDERPAD,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    OBJATTACH_MAINHAND,
    OBJATTACH_OFFHAND,
    OBJATTACH_RANGED,
    -1
};
static const unsigned int s_shoulderBones[NUMHANDS] = {3, 2};
static const unsigned int s_hands[NUMHANDS] = {INVSLOT_MAINHAND, INVSLOT_OFFHAND};
const VIRTUAL_MONSTER_SLOT g_monsterHands[NUMHANDS] = {
    VIRTUAL_MONSTER_SLOT_MAINHAND,
    VIRTUAL_MONSTER_SLOT_OFFHAND
};
static unsigned int s_canHideslots[OBJATTACH_NUM] = {0x00622000, 0, 0, 0xFFFFFFFF, 1};

static OBJATTACHMENTPOINTS s_handAttachments[NUMHANDS] = {
    OBJATTACH_MAINHAND,
    OBJATTACH_OFFHAND
};
static int flags = 3;

struct ForcedAnimationInfo {
  const char     *name;
  ANIMENUMERATION anim;
  unsigned int    flag;
};

static const ForcedAnimationInfo s_forcedAnimations[8] = {
    {            "stand",         ANIM_STAND, 0},
    {            "death",         ANIM_DEATH, 0},
    {             "walk",          ANIM_WALK, 0},
    {              "run",           ANIM_RUN, 0},
    {    "attackunarmed", ANIM_ATTACKUNARMED, 0},
    {            "wound",   ANIM_COMBATWOUND, 0},
    {"attackunarmedcrit", ANIM_ATTACKUNARMED, 1},
    {        "woundcrit",   ANIM_COMBATWOUND, 1},
};

static const ANIM_STATE                                 s_standStateAnims[9] = {
    ANIM_STATE_IDLE,
    ANIM_STATE_SITTING,
    ANIM_STATE_IDLE,
    ANIM_STATE_SLEEPING,
    ANIM_STATE_SITCHAIRLOW,
    ANIM_STATE_SITCHAIRMEDIUM,
    ANIM_STATE_SITCHAIRHIGH,
    ANIM_STATE_DEAD,
    ANIM_STATE_KNEELING
};
static TInstanceAllocator<ACTIVEAURAINFO>               s_auraInfoFreeList(100);
static TInstanceAllocator<ANIMENDDATA>                  s_animEndDataPool(1024);
static TSGrowableArray<unsigned int>                    g_unitSeqEndList;
static TSGrowableArray<unsigned int>                    g_mountSeqEndList;
static LISTDECL(AuraDecayNode, s_activeAuraDecays);
static TInstanceAllocator<AuraDecayNode>                s_auraDecayFreeList(100);
static TInstanceAllocator<SPELLEFFECTDESC>              s_spellEffectFreeList(100);
static TInstanceAllocator<ANIMQUEUENODE>                s_animQueueFreeList(100);
static TInstanceAllocator<IMPACTEFFECTDESC>             s_freeImpactEffectDescs(100);
static TInstanceAllocator<ACTIVEATTACHMENTINFO>         s_activeAttachmentFreeList(100);
static CVar                                             *s_showBreathCvar;
static unsigned int                                      s_currentGlobalClickCount;
static NTempest::CImVector                               s_targetFlashColor;
static unsigned int                                      s_lastTargetFlashTime;
static int                                               s_targetPulseDirection;

CGUnit_C::~CGUnit_C() {
  ClearTempCharModel();
  ProcessAnimEndCallbacks();
  m_savedChannelSpellTargets.Clear();

  if (GetType() & TYPE_PLAYER) {
    unsigned int guildID = static_cast<CGPlayer_C *>(this)->GetGuildID();
    if (guildID) {
      g_guildInfoCache.CancelCallback(guildID, PlayerNameGuildCallback, &m_unitNameHandle);
    }
  }

  ClearFishingObject();
  DestroyPaperdollModel();
  DestroyUnitMount(1);
  m_emoteQueue.Clear();
  m_pendingHitAnimVictims.Clear();
  ClearRangedStandTimer();
  ClearActiveAttachmentInfo();

  ASSERT(!m_impactEffectsDesc.Head());
  while (IMPACTEFFECTDESC *impact = m_impactEffectsDesc.Head()) {
    DEL(impact);
  }

  SetLocalTarget(0);
  if (m_model) {
    ModelSetEventCallback(m_model, 0, 0, 0);
  }

  unsigned int index;
  for (index = 0; index < sizeof(m_auraVisual) / sizeof(m_auraVisual[0]); ++index) {
    m_auraVisual[index].Clear();
  }

  while (ACTIVEAURAINFO *active = m_activeAuraInfo.Head()) {
    s_auraInfoFreeList.Put(active);
  }

  UnitUninitializeModel(m_model);
  ShutdownWorldName();
  if (m_geosetHandle) {
    HandleClose(m_geosetHandle);
    m_geosetHandle = 0;
  }
  if (m_texComponent) {
    HandleClose(m_texComponent);
    m_texComponent = 0;
  }

  PurgeAnimNodes(true);
  ASSERT(!m_currentDamageInfo);
  m_deathHoldBuffer.Clear();
  m_deathHoldBufferIndices.Clear();
  ClearWeaponTrailHandles();

  for (index = 0; index < sizeof(m_spellEffectLists) / sizeof(m_spellEffectLists[0]); ++index) {
    while (SPELLEFFECTDESC *effect = m_spellEffectLists[index].Head()) {
      s_spellEffectFreeList.Put(effect);
    }
  }

  if (m_channelSpellEffect) {
    s_spellEffectFreeList.Put(m_channelSpellEffect);
    m_channelSpellEffect = 0;
  }

  ClearAnimCallbackData();
}

int LootAnimEndHandler(void *param, CGUnit_C *ptr) {
  if (ptr->GetType() & TYPE_PLAYER) {
    static_cast<CGPlayer_C *>(ptr)->CGPlayer_C::LootAnimEndHandler();
  } else {
    ptr->CGUnit_C::LootAnimEndHandler();
  }
  return 1;
}

int SheatheAnimEndHandler(void *param, CGUnit_C *ptr) {
  ptr->SheatheAnimEndHandler();
  return 1;
}

int SitSleepAnimEndHandler(void *param, CGUnit_C *ptr) {
  ptr->SitSleepAnimEndHandler();
  return 1;
}

int RangedPrecastEndHandler(void *param, CGUnit_C *ptr) {
  ptr->RangedPrecastEndHandler();
  return 1;
}

int ThrowAnimEndHandler(void *param, CGUnit_C *ptr) {
  ptr->ThrowAnimEndHandler();
  return 1;
}

int AttackAnimEndHandler(void *param, CGUnit_C *ptr) {
  ptr->AttackAnimEndHandler();
  return 1;
}

int DodgeAnimEndHandler(void *param, CGUnit_C *ptr) {
  ptr->DodgeAnimEndHandler();
  return 1;
}

int InvSlotToObjAttachSlot(int invSlot) {
  return invSlot <= EQUIPPED_LAST ? s_invSlotToObjAttachSlot[invSlot] : -1;
}

static void PurgeExpiredNodes(LIST(SPELLEFFECTDESC) &list, float elapsed) {
  ITERATELIST(SPELLEFFECTDESC, list, desc) {
    desc->curTime += static_cast<unsigned int>(elapsed * 1000.0f);
    if (desc->endTime && desc->curTime > desc->endTime) {
      s_spellEffectFreeList.Put(desc);
    }
  }
}

float SPELLEFFECTDESC::CalcScalar() {
  float scalar = 1.0f;
  if (fadeInTime != startTime && curTime < fadeInTime) {
    scalar = static_cast<float>(curTime - startTime) / static_cast<int>(fadeInTime - startTime);
  }
  if (period != 0.0f) {
    scalar *= static_cast<float>(cos(static_cast<float>(curTime - startTime) / period * 3.1415927f) * 0.5 + 0.5);
  }
  if (fadeOutTime != endTime && curTime > fadeOutTime) {
    scalar *= 1.0f - static_cast<float>(curTime - fadeOutTime) / static_cast<int>(endTime - fadeOutTime);
  }
  return scalar;
}

void SpellProcChainHandler(
    SPELLPROC_ACTION action,
    SpellEffectList &list,
    CGUnit_C *unit,
    HMODEL,
    const SpellVisualKitRec *rec,
    SPELLEFFECTDESC         *newDesc,
    unsigned int             spellID,
  float
) {
  if (action == SPELLPROCADD) {
    int savedChannelSpellID = unit->GetSavedChannelSpellID();
    if (static_cast<int>(spellID) == savedChannelSpellID) {
      newDesc->lightningObjs[0] = 0;
      newDesc->lightningObjs[1] = 0;
      newDesc->lightningObjs[2] = 0;
      SpellVisualGetLightning(unit, rec, spellID, newDesc->lightningObjs, 3);
    }
    unit->ClearSavedChannelSpellTargets();
  } else if (action == SPELLPROCREMOVE) {
    newDesc->ClearLightningObjects();
    s_spellEffectFreeList.Put(newDesc);
  }
}

void SpellProcColorHandler(
    SPELLPROC_ACTION         action,
    SpellEffectList         &list,
    CGUnit_C                *unit,
    HMODEL                   charModel,
    const SpellVisualKitRec *rec,
    SPELLEFFECTDESC         *newDesc,
    unsigned int,
    float elapsed
) {
  if (action == SPELLPROCADD) {
    newDesc->color.Set(static_cast<unsigned int>(rec->m_characterParam[0]) | 0xFF000000);
    unsigned int now = OsGetAsyncTimeMs();
    newDesc->curTime = now;
    newDesc->startTime = now;
    newDesc->fadeInTime = now;
    newDesc->period = rec->m_characterParam[1] == 0.0f ? 0.0f : 1000.0f / rec->m_characterParam[1];
    if (newDesc->isOneShot) {
      newDesc->fadeOutTime = now + static_cast<unsigned int>(rec->m_characterParam[3] * 1000.0f);
      newDesc->endTime = newDesc->fadeOutTime + static_cast<unsigned int>(rec->m_characterParam[2] * 1000.0f);
    } else {
      newDesc->fadeOutTime = 0;
      newDesc->endTime = 0;
    }
  } else if (action == SPELLPROCREMOVE) {
    newDesc->fadeOutTime = newDesc->curTime;
    newDesc->endTime = newDesc->curTime + static_cast<unsigned int>(newDesc->kitPtr->m_characterParam[2] * 1000.0f);
  } else if (action == SPELLPROCREFRESH) {
    SPELLEFFECTDESC    *desc = list.Tail();
    NTempest::CImVector color(desc ? *desc->color.IV_() : 0xFFFFFFFF);
    ModelSetVertexColor(charModel, color.r, color.g, color.b, 0);
  } else if (action == SPELLPROCUPDATE) {
    bool                hadEffects = list.Head() != 0;
    NTempest::CImVector color(0xFFFFFFFF);
    PurgeExpiredNodes(list, elapsed);
    SPELLEFFECTDESC *desc = unit->GetActiveEffect(list);
    if (desc) {
      unsigned int alpha = static_cast<unsigned int>(desc->CalcScalar() * 255.0f);
      color.r = static_cast<unsigned char>(255 + (alpha * (static_cast<int>(desc->color.r) - 255) >> 8));
      color.g = static_cast<unsigned char>(255 + (alpha * (static_cast<int>(desc->color.g) - 255) >> 8));
      color.b = static_cast<unsigned char>(255 + (alpha * (static_cast<int>(desc->color.b) - 255) >> 8));
    }
    if (hadEffects) {
      ModelSetVertexColor(charModel, color.r, color.g, color.b, 0);
    }
  }
}

static float GetSpellEffectDescScale(SPELLEFFECTDESC *desc) {
  float scale = desc->scale;
  if (desc->fadeInTime != desc->startTime && desc->curTime < desc->fadeInTime) {
    NTempest::C3Vector points[4] = {
        NTempest::C3Vector(0.0f, 1.0f, 0.0f), NTempest::C3Vector(0.1f, 1.1f * scale, 0.0f), NTempest::C3Vector(0.2f, 1.1f * scale, 0.0f),
        NTempest::C3Vector(1.0f, scale, 0.0f)
    };
    NTempest::C3Spline_Bezier3 spline;
    spline.SetPoints(points, 4);
    float scalar = desc->CalcScalar();
    scalar = scalar < 0.0f ? 0.0f : (scalar > 1.0f ? 1.0f : scalar);
    NTempest::C3Vector pt;
    spline.Pos(scalar, pt, NTempest::C3Spline::EVAL_ARCLENGTH);
    scale = pt.y;
  } else if (desc->fadeOutTime != desc->endTime && desc->curTime > desc->fadeOutTime) {
    NTempest::C3Vector points[4] = {
        NTempest::C3Vector(0.0f, 1.0f, 0.0f), NTempest::C3Vector(0.25f, 0.875f * scale, 0.0f), NTempest::C3Vector(0.5f, 0.875f * scale, 0.0f),
        NTempest::C3Vector(1.0f, scale, 0.0f)
    };
    NTempest::C3Spline_Bezier3 spline;
    spline.SetPoints(points, 4);
    float scalar = desc->CalcScalar();
    scalar = scalar < 0.0f ? 0.0f : (scalar > 1.0f ? 1.0f : scalar);
    NTempest::C3Vector pt;
    spline.Pos(scalar, pt, NTempest::C3Spline::EVAL_ARCLENGTH);
    scale = pt.y;
  }
  return scale;
}

static float GetDesiredRenderScale(SpellEffectList &list) {
  float currentScale = 1.0f;
  ITERATELIST(SPELLEFFECTDESC, list, desc) {
    currentScale *= GetSpellEffectDescScale(desc);
  }
  if (currentScale < 0.75f) {
    currentScale = 0.75f;
  }
  if (currentScale > 2.0f) {
    currentScale = 2.0f;
  }
  return currentScale;
}

void SpellProcScaleHandler(
    SPELLPROC_ACTION action,
    SpellEffectList &list,
    CGUnit_C        *unit,
    HMODEL,
    const SpellVisualKitRec *rec,
    SPELLEFFECTDESC         *newDesc,
    unsigned int,
    float elapsed
) {
  if (action == SPELLPROCADD) {
    unsigned int now = OsGetAsyncTimeMs();
    newDesc->curTime = now;
    newDesc->startTime = now;
    newDesc->fadeInTime = now + 200;
    newDesc->endTime = 0;
    newDesc->fadeOutTime = 0;
    newDesc->period = 0.0f;
    newDesc->scale = rec->m_characterParam[0];
  } else if (action == SPELLPROCREMOVE) {
    newDesc->fadeOutTime = newDesc->curTime;
    newDesc->endTime = newDesc->curTime + 400;
  } else if (action == SPELLPROCREFRESH) {
    unit->SetRenderScale(GetDesiredRenderScale(list));
  } else if (action == SPELLPROCUPDATE) {
    float oldScale = list.Head() ? GetDesiredRenderScale(list) : 1.0f;
    PurgeExpiredNodes(list, elapsed);
    float newScale = list.Head() ? GetDesiredRenderScale(list) : 1.0f;
    if (newScale != oldScale) {
      unit->SetRenderScale(newScale);
    }
  }
}

void SpellProcEmissiveHandler(
    SPELLPROC_ACTION action,
    SpellEffectList &list,
    CGUnit_C        *unit,
    HMODEL,
    const SpellVisualKitRec *rec,
    SPELLEFFECTDESC         *newDesc,
    unsigned int,
    float elapsed
) {
  if (action == SPELLPROCADD) {
    newDesc->color.Set(static_cast<unsigned int>(rec->m_characterParam[0]) | 0xFF000000);
    unit->AddEmissiveColor(newDesc->color);
  } else if (action == SPELLPROCREMOVE) {
    unit->RemoveEmissiveColor(newDesc->color);
    s_spellEffectFreeList.Put(newDesc);
  } else if (action == SPELLPROCUPDATE) {
    PurgeExpiredNodes(list, elapsed);
  }
}

void SpellProcEclipseHandler(
    SPELLPROC_ACTION action,
    SpellEffectList &list,
    CGUnit_C *,
    HMODEL,
    const SpellVisualKitRec *rec,
    SPELLEFFECTDESC         *newDesc,
    unsigned int             spellID,
    float                    elapsed
) {
  if (action == SPELLPROCADD) {
    newDesc->color.Set(static_cast<unsigned int>(rec->m_characterParam[0]) | 0xFF000000);
    newDesc->startTime = OsGetAsyncTimeMs();
    int castTime = Spell_C_GetCastTime(spellID, 0);
    newDesc->endTime = newDesc->startTime + castTime;
    newDesc->fadeInTime = newDesc->startTime + static_cast<unsigned int>(castTime * rec->m_characterParam[1]);
  } else if (action == SPELLPROCREMOVE) {
    s_spellEffectFreeList.Put(newDesc);
  } else if (action == SPELLPROCUPDATE) {
    PurgeExpiredNodes(list, elapsed);
  }
}

void SpellProcStandWalkAnimHandler(
    SPELLPROC_ACTION action,
    SpellEffectList &list,
    CGUnit_C        *unit,
    HMODEL,
    const SpellVisualKitRec *rec,
    SPELLEFFECTDESC         *newDesc,
    unsigned int,
    float elapsed
) {
  static const int standStateAnims[] = {0, 120, 1};
  static const int walkStateAnims[] = {4, 119};

  if (action == SPELLPROCADD) {
    newDesc->standAnim = static_cast<int>(rec->m_characterParam[0]);
    newDesc->walkAnim = static_cast<int>(rec->m_characterParam[1]);
    FATALASSERT(newDesc->standAnim < 3);
    FATALASSERT(newDesc->walkAnim < 2);
    newDesc->endTime = 0;
  } else if (action == SPELLPROCREMOVE) {
    FATALASSERT(newDesc);
    s_spellEffectFreeList.Put(newDesc);
  } else if (action == SPELLPROCUPDATE) {
    PurgeExpiredNodes(list, elapsed);
  }

  int standAnim = 0;
  int walkAnim = 0;
  ITERATELIST(SPELLEFFECTDESC, list, desc) {
    if (standAnim < desc->standAnim) {
      standAnim = desc->standAnim;
    }
    if (walkAnim < desc->walkAnim) {
      walkAnim = desc->walkAnim;
    }
  }

  unit->SetStandStateAnim(standStateAnims[standAnim]);
  unit->SetWalkStateAnim(walkStateAnims[walkAnim]);
  if (standAnim == 2 && action == SPELLPROCADD) {
    unit->UpdateBaseAnimation(4, 0);
  }
}

unsigned __int64                           CGUnit_C::m_activeMover;
static unsigned int                        s_moveHeartBeatTimer;
static TRACKTYPE                           s_trackingType;
static unsigned __int64                    s_trackingTarget;
static unsigned int                        s_trackingFlags;
static unsigned long                       s_trackingInterpStartTime;
static unsigned long                       s_trackLastCheckTime;
static TSGrowableArray<NTempest::C3Vector> s_bowStringVerts;
static TSGrowableArray<unsigned short>     s_bowStringIndices;
static const EmotesRec                    *s_talkEmotes[TALKANIM_NUMTALKANIMS];
static const bool                          s_mustSheathe[9] = {0, 1, 1, 1, 1, 1, 1, 1, 1};
static const ANIMQUEUETYPE                  s_animStandUpTransitions[9] = {
    ANIMQUEUE_NONE,      ANIMQUEUE_SITUP,       ANIMQUEUE_SITCHAIRUP,
    ANIMQUEUE_SLEEPUP,   ANIMQUEUE_SITCHAIRUP,  ANIMQUEUE_SITCHAIRUP,
    ANIMQUEUE_SITCHAIRUP, ANIMQUEUE_NONE,        ANIMQUEUE_KNEELUP
};
static const ANIMQUEUETYPE                  s_animStandDownTransitions[9] = {
    ANIMQUEUE_NONE,          ANIMQUEUE_SITDOWN,        ANIMQUEUE_SITCHAIR,
    ANIMQUEUE_SLEEPDOWN,     ANIMQUEUE_SITCHAIRLOW,    ANIMQUEUE_SITCHAIRMEDIUM,
    ANIMQUEUE_SITCHAIRHIGH,  ANIMQUEUE_DEAD,           ANIMQUEUE_KNEELDOWN
};

void SpellProcWeaponTrailHandler(
    SPELLPROC_ACTION action,
    SpellEffectList &list,
    CGUnit_C *unit,
    HMODEL,
    const SpellVisualKitRec *rec,
    SPELLEFFECTDESC         *newDesc,
    unsigned int,
    float
) {
  if (action == SPELLPROCADD) {
    NTempest::CImVector color((static_cast<unsigned int>(rec->m_characterParam[3]) << 24) | static_cast<unsigned int>(rec->m_characterParam[0]));
    unit->EnableWeaponTrail(color, static_cast<int>(rec->m_characterParam[1]), static_cast<unsigned int>(rec->m_characterParam[2]));
  } else if (action == SPELLPROCREMOVE) {
    FATALASSERT(newDesc);
    s_spellEffectFreeList.Put(newDesc);
  }
}

static BLOODSPLATNODE *NewBloodSplatNode() {
  BLOODSPLATNODE *node = s_bloodSplatList.Head();
  if (node) {
    node->Unlink();
    return node;
  }
  return NEW(BLOODSPLATNODE);
}

struct UnitAnimationInfo {
  unsigned int state;
  const char  *name;
  unsigned int flags;
  int          basePriority;
  int          priorityOffset;
  unsigned int statePreempts;

  UnitAnimationInfo(unsigned int theState, const char *theName, unsigned int theFlags, int theBasePriority, int thePriorityOffset)
      : state(theState), name(theName), flags(theFlags), basePriority(theBasePriority), priorityOffset(thePriorityOffset) {
  }
};

static UnitAnimationInfo s_animInfo[64] = {
    UnitAnimationInfo(0, "ANIM_STATE_NONE", 0x20000, 0, 0),
    UnitAnimationInfo(1, "ANIM_STATE_DEAD", 1033, 1000, 0),
    UnitAnimationInfo(2, "ANIM_STATE_SPELL", 0, 50, 0),
    UnitAnimationInfo(3, "ANIM_STATE_IDLE", 139264, 0, 0),
    UnitAnimationInfo(4, "ANIM_STATE_STOP", 139264, 0, 0),
    UnitAnimationInfo(5, "ANIM_STATE_WALK", 139524, 5, 0),
    UnitAnimationInfo(6, "ANIM_STATE_RUN", 139524, 5, 0),
    UnitAnimationInfo(7, "ANIM_STATE_WALK_BACKWARDS", 141572, 5, 0),
    UnitAnimationInfo(8, "ANIM_STATE_STRAFE_WALK_LEFT", 139524, 5, 0),
    UnitAnimationInfo(9, "ANIM_STATE_STRAFE_WALK_RIGHT", 139524, 5, 0),
    UnitAnimationInfo(10, "ANIM_STATE_STRAFE_RUN_LEFT", 139524, 5, 0),
    UnitAnimationInfo(11, "ANIM_STATE_STRAFE_RUN_RIGHT", 139524, 5, 0),
    UnitAnimationInfo(12, "ANIM_STATE_DIAG_WALK_LEFT", 139524, 10, 0),
    UnitAnimationInfo(13, "ANIM_STATE_DIAG_WALK_RIGHT", 139524, 10, 0),
    UnitAnimationInfo(14, "ANIM_STATE_DIAG_RUN_LEFT", 139524, 10, 0),
    UnitAnimationInfo(15, "ANIM_STATE_DIAG_RUN_RIGHT", 139524, 10, 0),
    UnitAnimationInfo(16, "ANIM_STATE_DIAG_BACKWARDS_LEFT", 141572, 10, 0),
    UnitAnimationInfo(17, "ANIM_STATE_DIAG_BACKWARDS_RIGHT", 141572, 10, 0),
    UnitAnimationInfo(18, "ANIM_STATE_TURNING_LEFT", 139264, 5, 0),
    UnitAnimationInfo(19, "ANIM_STATE_TURNING_RIGHT", 139264, 5, 0),
    UnitAnimationInfo(20, "ANIM_STATE_SWIM_IDLE", 8196, 0, 0),
    UnitAnimationInfo(21, "ANIM_STATE_SWIM", 8452, 0, 0),
    UnitAnimationInfo(22, "ANIM_STATE_SWIM_STRAFE_LEFT", 8452, 0, 0),
    UnitAnimationInfo(23, "ANIM_STATE_SWIM_STRAFE_RIGHT", 8452, 0, 0),
    UnitAnimationInfo(24, "ANIM_STATE_SWIM_BACKWARDS", 8452, 0, 0),
    UnitAnimationInfo(25, "ANIM_STATE_KNEEL", 0, 0, 0),
    UnitAnimationInfo(26, "ANIM_STATE_RISE", 0, 0, 0),
    UnitAnimationInfo(27, "ANIM_STATE_WOUND", 33, 70, 1),
    UnitAnimationInfo(28, "ANIM_STATE_CRITICALWOUND", 33, 70, 1),
    UnitAnimationInfo(29, "ANIM_STATE_STUN", 9, 60, 11),
    UnitAnimationInfo(30, "ANIM_STATE_ATTACK_HIT", 8753, 80, 0),
    UnitAnimationInfo(31, "ANIM_STATE_ATTACK_READY", 139296, 0, 0),
    UnitAnimationInfo(32, "ANIM_STATE_ATTACK_MISS", 8753, 70, 10),
    UnitAnimationInfo(33, "ANIM_STATE_ATTACKOFF_HIT", 8753, 80, 0),
    UnitAnimationInfo(34, "ANIM_STATE_ATTACKOFF_MISS", 8753, 70, 10),
    UnitAnimationInfo(35, "ANIM_STATE_PARRY", 8224, 70, 1),
    UnitAnimationInfo(36, "ANIM_STATE_DODGE", 8224, 70, 1),
    UnitAnimationInfo(37, "ANIM_STATE_SPELLPRECAST", 43, 71, 0),
    UnitAnimationInfo(38, "ANIM_STATE_SPELLCAST", 262673, 72, 10),
    UnitAnimationInfo(39, "ANIM_STATE_NPC_OBSOLETE", 0, 50, 0),
    UnitAnimationInfo(40, "ANIM_STATE_BLOCK", 32, 75, 0),
    UnitAnimationInfo(41, "ANIM_STATE_JUMPING", 139265, 90, 0),
    UnitAnimationInfo(42, "ANIM_STATE_JUMP_LANDING", 139265, 90, 0),
    UnitAnimationInfo(43, "ANIM_STATE_FALLING", 139265, 90, 0),
    UnitAnimationInfo(44, "ANIM_STATE_LOOTBEGIN", 1160, 41, 0),
    UnitAnimationInfo(45, "ANIM_STATE_LOOTEND", 200, 40, 0),
    UnitAnimationInfo(46, "ANIM_STATE_EMOTE", 131105, 30, 0),
    UnitAnimationInfo(47, "ANIM_STATE_SPELLIMPACT", 131109, 81, 0),
    UnitAnimationInfo(48, "ANIM_STATE_MOUNTED", 16420, 100, 0),
    UnitAnimationInfo(49, "ANIM_STATE_SPECIALMOUNTANIM", 0x2000, 20, 0),
    UnitAnimationInfo(50, "ANIM_STATE_SITDOWN", 196613, 2, 0),
    UnitAnimationInfo(51, "ANIM_STATE_SITTING", 655365, 1, 0),
    UnitAnimationInfo(52, "ANIM_STATE_SITUP", 196613, 3, 0),
    UnitAnimationInfo(53, "ANIM_STATE_SLEEPDOWN", 196613, 2, 0),
    UnitAnimationInfo(54, "ANIM_STATE_SLEEPING", 655365, 1, 0),
    UnitAnimationInfo(55, "ANIM_STATE_SLEEPUP", 196613, 3, 0),
    UnitAnimationInfo(56, "ANIM_STATE_SITCHAIRLOW", 655364, 1, 0),
    UnitAnimationInfo(57, "ANIM_STATE_SITCHAIRMEDIUM", 655364, 1, 0),
    UnitAnimationInfo(58, "ANIM_STATE_SITCHAIRHIGH", 655364, 1, 0),
    UnitAnimationInfo(59, "ANIM_STATE_KNEELDOWN", 196613, 2, 0),
    UnitAnimationInfo(60, "ANIM_STATE_KNEELING", 655365, 1, 0),
    UnitAnimationInfo(61, "ANIM_STATE_KNEELUP", 196613, 3, 0),
    UnitAnimationInfo(62, "ANIM_STATE_CHANNELSPELL", 33313, 29, 0),
    UnitAnimationInfo(63, "ANIM_STATE_SPELLAURA", 33, 29, 0),
};

void CGUnit_C::PreRender(int currentTime, float elapsed) {
  m_animFlags |= 0x20000U;
  UpdateSpellProcEffects(elapsed);
  ProcessEmoteQueue();
  ProcessAnimEndCallbacks();
  ProcessBreathParticles(currentTime);
  CheckDeferredSheathing();
  HandleFollowTarget();
  ProcessChannelObject();

  if (m_precastSheatheHoldTimer != -1 && currentTime >= m_precastSheatheHoldTimer) {
    SetSheatheReason(SHEATHE_PRECAST, 0, 1);
    m_precastSheatheHoldTimer = -1;
  }

  if ((s_animInfo[m_currentBaseAnimState].flags & 0x20000) || (s_animInfo[m_currentTorsoAnimState].flags & 0x20000)) {
    ANIMQUEUENODE *node = ProcessAnimQueue();
    if (node) {
      ProcessAnim(node);
    }
  }
}

static NTempest::CImVector COLOR_GOLD(0xFFFFDE00);

int OnPickNextStandHandler(void *param, CGUnit_C *ptr);
int DeathAnimEndHandler(void *param, CGUnit_C *ptr);
int PickNextRunHandler(void *param, CGUnit_C *ptr);
int WoundAnimEndHandler(void *param, CGUnit_C *ptr);
int SpellAnimEndHandler(void *param, CGUnit_C *ptr);
int NPCAnimEndHandler(void *param, CGUnit_C *ptr);
int JumpTakeOffFinishedHandler(void *param, CGUnit_C *ptr);
int JumpLandFinishedHandler(void *param, CGUnit_C *ptr);
int RangedWeaponAnimEndHandler(void *param, CGUnit_C *ptr);
int LootAnimEndHandler(void *param, CGUnit_C *ptr);
int SheatheAnimEndHandler(void *param, CGUnit_C *ptr);
int SitSleepAnimEndHandler(void *param, CGUnit_C *ptr);
int RangedPrecastEndHandler(void *param, CGUnit_C *ptr);
int ThrowAnimEndHandler(void *param, CGUnit_C *ptr);
int AttackAnimEndHandler(void *param, CGUnit_C *ptr);
int DodgeAnimEndHandler(void *param, CGUnit_C *ptr);

static void ClearQuestIconHandles(int reinitialize) {
  for (unsigned int i = 0; i < 5; ++i) {
    if (s_interactIconModelInfo[i].model) {
      HandleClose(s_interactIconModelInfo[i].model);
    }

    if (reinitialize) {
      const char *modelName = ClientDBStringLookup(s_interactIconModelInfo[i].string);
      FATALASSERT(modelName && *modelName);

      CModelCreate createData;
      createData.boneNames = 0;
      createData.numBones = 0;
      createData.cameraNames = 0;
      createData.numCameras = 0;
      createData.flags = 0x200A;
      createData.sequenceNames = s_interactIconAnimNames;
      createData.numSequences = 2;
      s_interactIconModelInfo[i].model = ModelCreate(modelName, &createData, 0);
      FATALASSERT(s_interactIconModelInfo[i].model);
    } else {
      s_interactIconModelInfo[i].model = 0;
    }
  }
}

void InitTalkEmotes() {
  for (int index = g_emotesDB.GetNumRecords(); index;) {
    const EmotesRec *emote = g_emotesDB.GetRecordByIndex(--index);
    FATALASSERT(emote);

    if (emote->m_EmoteFlags & 8) {
      s_talkEmotes[TALKANIM_TALK] = emote;
    } else if (emote->m_EmoteFlags & 0x10) {
      s_talkEmotes[TALKANIM_QUESTION] = emote;
    } else if (emote->m_EmoteFlags & 0x20) {
      s_talkEmotes[TALKANIM_EXCLAMATION] = emote;
    } else if (emote->m_EmoteFlags & 0x40) {
      s_talkEmotes[TALKANIM_SHOUT] = emote;
    } else if (emote->m_EmoteFlags & 0x100) {
      s_talkEmotes[TALKANIM_LAUGH] = emote;
    }
  }
}

void CreatureQueryCallback(int id, const unsigned __int64 &guid, void *, bool) {
  const CreatureStats_C *stats = g_creatureDBCache.GetRecord(id, static_cast<unsigned __int64>(0), 0, 0);
  if (stats) {
    CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
    if (unitPtr) {
      unitPtr->m_stats = const_cast<CreatureStats_C *>(stats);
    }
    CGGameUI::UnitNameUpdate(guid);
  }
}

static void NameQueryCallback(int id, const unsigned __int64 &guid, void *arg, bool granted) {
  if (granted) {
    CGGameUI::UnitNameUpdate(guid);
  }
}

void CGUnit_C::ApplyObjectCameraSpaceLookAt(const NTempest::C3Vector &target) {
  HMODEL charModel = GetCharacterModel(0);
  FATALASSERT(charModel);
  if (!ModelApplyObjectLookAt(charModel, 6, target)) {
    SysMsgPrintf(SYSMSG_WARNING, 8, "UNITMISSINGBONE|%s|%d", GetUnitName(), 6);
  }
  HandleClose(charModel);
  m_animFlags |= 0x1000;
}

void CGUnit_C::RemoveObjectLookAt() {
  HMODEL charModel = GetCharacterModel(0);
  if (charModel) {
    ModelRemoveObjectLookAt(charModel, 6);
    HandleClose(charModel);
  }
  m_animFlags &= ~0x1000u;
}

int SpellFizzleTimer(const void *__formal, void *userData) {
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(userData);
  FATALASSERT(unitPtr);
  unitPtr->EndSpellEffects(2);
  return 1;
}

static int RangedStandTimerHandler(const void *__formal, void *userData) {
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(userData);
  FATALASSERT(unitPtr);
  unitPtr->OnRangedStandTimer();
  return 1;
}

void CGUnit_C::RemoveBloodPool() {
  m_flags &= ~0x10000u;
}

void CGUnit_C::AddBloodPool() {
  if (m_bloodRec) {
    m_flags |= 0x10000u;
  }
}

static void AnimEventCallback(const char* eventName, const NTempest::C3Vector& position, void* param) {
  CGUnit_C *unit = static_cast<CGUnit_C *>(param);
  if (unit) {
    unit->HandleAnimEvent(eventName, position);
  }
}

static void MountedAnimEventCallback(const char* eventName, const NTempest::C3Vector& position, void* param) {
  CGUnit_C *unit = static_cast<CGUnit_C *>(param);
  if (unit) {
    unit->HandleMountedAnimEvent(eventName, position);
  }
}

void CGUnit_C::HandleMountedAnimEvent(const char *eventName, const NTempest::C3Vector &position) {
  unsigned long eventCode = *reinterpret_cast<const unsigned long *>(eventName);
  if (eventCode == 0x47475724) {
    PlayUnitSound(UNITSOUNDTYPE_WINGGLIDE, 0);
  } else if (eventCode == 0x474E5724) {
    PlayUnitSound(UNITSOUNDTYPE_WINGFLAP, 0);
  } else if (eventCode == 0x48544224) {
    BreathHandler(1);
  } else {
    HandleAnimEvent(eventName, position);
  }
}

UNITEFFECTSPECIALS CGUnit_C::DetermineBreathEffect(unsigned int *duration) {
  ASSERT(duration);

  if (s_showBreathCvar->GetInt() && (!m_modelData || !(m_modelData->m_flags & 2))) {
    unsigned int       liquid = 0;
    float              surface = 0.0f;
    NTempest::C3Vector flowDir(0.0f);
    NTempest::C3Vector pos = GetPosition();
    int                deep = 0;

    if (ClntObjMgrGetPlayerType() ||
        !CWorld::QueryObjectLiquid(GetWorldObject(), liquid, surface, flowDir, deep)) {
      if (AreaListZoneHasBreathParticles(GetWorldObject(), CGPlayer_C::GetNewContinentID(), GetPosition())) {
        *duration = 2000;
        return SPECIALEFFECT_COLDBREATH;
      }
    } else if (GetScale() * GetObjectHeight() + 5.0f < surface - pos.z) {
      *duration = 4000;
      return SPECIALEFFECT_UNDERWATERBUBBLES;
    }
  }

  *duration = 10000;
  return SPECIALEFFECT_NONE;
}

void CGUnit_C::BreathHandler(int forceOnMount) {
  unsigned int duration;
  UNITEFFECTSPECIALS effect = DetermineBreathEffect(&duration);
  if (effect != SPECIALEFFECT_NONE) {
    UnitEffectOneShot(effect, GetGUID(), 0, 0.0f, 1.0f, forceOnMount != 0);
  }
}

void CGUnit_C::ProcessBreathParticles(int currentTime) {
  if (m_unit->health <= 0) {
    return;
  }

  if ((m_nextBreath == -1 || m_nextBreath <= currentTime) &&
      (m_currentBaseAnimState == ANIM_STATE_IDLE || !(m_flags & 0x2000))) {
    unsigned int       duration;
    UNITEFFECTSPECIALS effect = DetermineBreathEffect(&duration);
    m_nextBreath = currentTime + duration;
    if (effect != SPECIALEFFECT_NONE) {
      UnitEffectOneShot(effect, GetGUID(), 0, 0.0f, 1.0f, false);
    }
  }

  if ((m_nextMountBreath == -1 || m_nextMountBreath <= currentTime) &&
      (((m_unit->flags & 0x2000) && m_currentMountAnimState == ANIM_STATE_IDLE) || !(m_flags & 0x4000))) {
    unsigned int       duration;
    UNITEFFECTSPECIALS effect = DetermineBreathEffect(&duration);
    if (effect != SPECIALEFFECT_NONE) {
      UnitEffectOneShot(effect, GetGUID(), 0, 0.0f, 1.0f, true);
    }
    m_nextMountBreath = currentTime + duration;
    if (m_nextMountBreath == m_nextBreath) {
      m_nextMountBreath += duration >> 1;
    }
  }
}

static int GenericAnimEndHandler(void *param) {
  FATALASSERT(param);
  ANIMENDDATA *data = static_cast<ANIMENDDATA *>(param);
  FATALASSERT(data->animID < NUM_OBJECTANIMATIONS);
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(data->unit, __FILE__, __LINE__));
  if (unit) {
    unit->GenericAnimEndHandler(data->animID, data);
  }
  return 1;
}

int UnitFlagUpdateHandler(unsigned __int64 unit, unsigned int, unsigned int, const void *oldValue, void *) {
  FATALASSERT(oldValue);
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  FATALASSERT(unitPtr);
  unitPtr->OnFlagChanged(*static_cast<const unsigned int *>(oldValue));
  return 1;
}

int UnitLevelUpdateHandler(unsigned __int64 guid, unsigned int offset, unsigned int bytes, const void *oldValue, void *param) {
  int       oldLevel = *static_cast<const int *>(oldValue);
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (unitPtr->GetUnitData()->level != oldLevel) {
    unitPtr->OnLevelChange();
  }
  return 1;
}

int UnitModeUpdateHandler(unsigned __int64 guid, unsigned int offset, unsigned int bytes, const void *oldValue, void *param) {
  FATALASSERT(oldValue);
  CGUnit_C              *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  const CreatureStats_C *stats = g_creatureDBCache.GetRecord(unitPtr->m_obj->m_entryID, guid, CreatureQueryCallback, 0);
  if (stats) {
    unitPtr->m_stats = const_cast<CreatureStats_C *>(stats);
    CGGameUI::UnitNameUpdate(guid);
  }
  return 1;
}

int UnitHealthUpdateHandler(unsigned __int64 unit, unsigned int offset, unsigned int bytes, const void *oldValue, void *param) {
  FATALASSERT(oldValue);
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  int       oldHealth = *static_cast<const int *>(oldValue);
  FATALASSERT(unitPtr->IsA(HIER_TYPE_UNIT));
  FATALASSERT(unitPtr->GetMaxHealth());

  if (static_cast<float>(unitPtr->m_unit->health) / unitPtr->m_unit->maxHealth < 0.2f &&
      unitPtr->m_unit->health > 0) {
    unitPtr->AddBloodPool();
  } else {
    unitPtr->RemoveBloodPool();
  }
  unitPtr->UpdateDisplayHealth();

  if (unitPtr->m_unit->health <= 0 && oldHealth > 0) {
    if (unit == CGUnit_C::GetActiveMover()) {
      CGInputControl::GetActive()->UpdatePlayer(OsGetAsyncTimeMs());
    }

    unitPtr->OnDeath();

    if (!unitPtr->m_deathHolds) {
      CGGameUI::ClearTarget(unit, 0);
      if (!(unitPtr->m_animFlags & 0x2000)) {
        unitPtr->OnDeathAnimate();
      }
    }
  } else if (unitPtr->m_unit->health > 0 && oldHealth <= 0) {
    if (unit == CGUnit_C::GetActiveMover()) {
      CGInputControl::GetActive()->UpdatePlayer(OsGetAsyncTimeMs());
    }
    unitPtr->m_flags &= ~1u;
    unitPtr->RestoreUnit();
  }
  return 1;
}

int UnitCharmedUpdateHandler(unsigned __int64 unit, unsigned int offset, unsigned int bytes, const void *oldValue, void *param) {
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  if (unitPtr) {
    unitPtr->OnCharmedChanged();
  }
  return 1;
}

int DisplayIDUpdateHandler(unsigned __int64 unit, unsigned int offset, unsigned int bytes, const void *oldValue, void *param) {
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  if (unitPtr) {
    unitPtr->UpdateDisplayInfo();
  }
  return 1;
}

int StandStateUpdateHandler(unsigned __int64 unit, unsigned int offset, unsigned int bytes, const void *oldValue, void *param) {
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  if (unitPtr) {
    unitPtr->StandStateChanged(*static_cast<const unsigned char *>(oldValue));
  }
  return 1;
}

SEQFINISHINFO g_seqInformation[NUM_OBJECTANIMATIONS] = {
    {    OnPickNextStandHandler, 3,  0},
    {       DeathAnimEndHandler, 1,  6},
    {                         0, 0,  0},
    {    OnPickNextStandHandler, 3,  0},
    {                         0, 0,  0},
    {        PickNextRunHandler, 3,  0},
    {                         0, 0,  4},
    {                         0, 0,  0},
    {       WoundAnimEndHandler, 3,  0},
    {       WoundAnimEndHandler, 3,  0},
    {       WoundAnimEndHandler, 3,  0},
    {                         0, 0,  0},
    {                         0, 0,  0},
    {                         0, 0,  0},
    {       WoundAnimEndHandler, 3,  0},
    {                         0, 0,  0},
    {      AttackAnimEndHandler, 3,  0},
    {      AttackAnimEndHandler, 3,  0},
    {      AttackAnimEndHandler, 3,  0},
    {      AttackAnimEndHandler, 3,  0},
    {       WoundAnimEndHandler, 3,  0},
    {       WoundAnimEndHandler, 3,  0},
    {       WoundAnimEndHandler, 3,  0},
    {       WoundAnimEndHandler, 3,  0},
    {       WoundAnimEndHandler, 3,  0},
    {                         0, 0,  0},
    {                         0, 0,  0},
    {                         0, 0,  0},
    {                         0, 0,  0},
    {                         0, 0,  0},
    {       DodgeAnimEndHandler, 3,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {         NPCAnimEndHandler, 1,  0},
    {         NPCAnimEndHandler, 1,  0},
    {       WoundAnimEndHandler, 3,  0},
    {JumpTakeOffFinishedHandler, 3,  0},
    {                         0, 0,  0},
    {   JumpLandFinishedHandler, 3,  0},
    {                         0, 0,  0},
    {                         0, 0,  0},
    {                         0, 0,  0},
    {                         0, 0,  0},
    {                         0, 0,  0},
    {                         0, 0,  0},
    {RangedWeaponAnimEndHandler, 1,  0},
    {                         0, 0,  0},
    {                         0, 0,  0},
    {RangedWeaponAnimEndHandler, 1,  0},
    {        LootAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  1},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1, 16},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {      AttackAnimEndHandler, 3,  0},
    {      AttackAnimEndHandler, 3,  0},
    {      AttackAnimEndHandler, 3,  0},
    {      AttackAnimEndHandler, 3,  0},
    {     SheatheAnimEndHandler, 1,  0},
    {     SheatheAnimEndHandler, 1,  0},
    {                         0, 0,  0},
    {                         0, 0,  0},
    {                         0, 0,  0},
    {                         0, 0,  0},
    {       SpellAnimEndHandler, 1,  8},
    {    SitSleepAnimEndHandler, 1,  0},
    {                         0, 0,  0},
    {    SitSleepAnimEndHandler, 1,  0},
    {    SitSleepAnimEndHandler, 1,  0},
    {                         0, 0,  0},
    {    SitSleepAnimEndHandler, 1,  0},
    {                         0, 0,  0},
    {                         0, 0,  0},
    {                         0, 0,  0},
    {   RangedPrecastEndHandler, 1,  0},
    {   RangedPrecastEndHandler, 1,  0},
    {       ThrowAnimEndHandler, 1,  0},
    {                         0, 0,  0},
    {                         0, 0,  0},
    {                         0, 0,  0},
    {                         0, 0,  0},
    {   RangedPrecastEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1, 16},
    {    SitSleepAnimEndHandler, 1,  0},
    {                         0, 0,  0},
    {    SitSleepAnimEndHandler, 1,  0},
    {      AttackAnimEndHandler, 3,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  0},
    {       SpellAnimEndHandler, 1,  8},
    {       SpellAnimEndHandler, 1, 16},
    {       SpellAnimEndHandler, 1, 16},
    {                         0, 0,  0},
    {                         0, 0,  0},
    {                         0, 0,  0},
    {                         0, 0,  8},
    {                         0, 0,  0},
    {       SpellAnimEndHandler, 1,  0},
    {                         0, 0,  0},
    {       DeathAnimEndHandler, 1,  6},
    {                         0, 0,  4},
    {       SpellAnimEndHandler, 1,  0},
    {                         0, 1,  0}
};

struct TRACKTYPEINFO {
  bool  hasMovement;
  float disengageDistance;
};

static const TRACKTYPEINFO s_trackTypeInfo[3] = {
    {0, 100000.0f},
    {0, 100000.0f},
    {1,     30.0f}
};

static const float TRACKMOVETHRESHOLDSQ = 4.0f;
static const float TRACKRUNTHRESHOLDSQ = 16.0f;

int NPCFlagsHandler(unsigned __int64 unit, unsigned int offset, unsigned int bytes, const void *oldValue, void *param) {
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  if (unitPtr) {
    unitPtr->NPCFlagChanged(*static_cast<const unsigned char *>(oldValue));
  }
  return 1;
}

int WeaponModeUpdateHandler(unsigned __int64 unit, unsigned int offset, unsigned int bytes, const void *oldValue, void *param) {
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  if (unitPtr) {
    unitPtr->WeaponModeChanged();
  }
  return 1;
}

int PetNameChangeHandler(unsigned __int64 unit, unsigned int, unsigned int, const void *, void *) {
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  if (unitPtr) {
    const CGUnitData *unitData = unitPtr->GetUnitData();
    g_petNameCache.Invalidate(unitData->petNumber);
    unitPtr->GetUnitName();
    unsigned __int64 owner = unitData->charmedBy ? unitData->charmedBy : unitData->summonedBy;
    if (owner == ClntObjMgrGetActivePlayer()) {
      FrameScript_SignalEvent(309);
    }
  }
  return 1;
}

int VirtualItemChangeHandler(unsigned __int64 unit, unsigned int offset, unsigned int bytes, const void *oldValue, void *param) {
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  if (unitPtr) {
    unitPtr->VirtualComponentChanged(reinterpret_cast<int>(param), *static_cast<const int *>(oldValue));
  }
  return 1;
}

int DynamicFlagsChangeHandler(unsigned __int64 unit, unsigned int offset, unsigned int bytes, const void *oldValue, void *param) {
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  if (unitPtr) {
    unitPtr->OnDynamicFlagsChanged(*static_cast<const unsigned int *>(oldValue));
  }
  return 1;
}

int EmoteStateChangeHandler(unsigned __int64 unit, unsigned int offset, unsigned int bytes, const void *oldValue, void *param) {
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  if (unitPtr) {
    unitPtr->ClearTorsoAnimation(0);
  }
  return 1;
}

void ACTIVEATTACHMENTINFO::Clear() {
  for (int i = 0; i < 2; ++i) {
    if (modelInfo[i].model) {
      HandleClose(modelInfo[i].model);
    }
    modelInfo[i].model = 0;
  }
}

void ACTIVEATTACHMENTINFO::ClearAttachmentFromModel(HMODEL charModel, HMODEL paperDollModel) {
  if (charModel) {
    modelInfo[0].ClearAttachmentFromModel(charModel, paperDollModel);
    modelInfo[1].ClearAttachmentFromModel(charModel, paperDollModel);
    flags &= ~4u;
  }
}

int ChannelSpellChangeHandler(unsigned __int64 unit, unsigned int offset, unsigned int bytes, const void *oldValue, void *param) {
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  if (unitPtr) {
    unitPtr->OnChannelSpellChanged(*static_cast<const unsigned int *>(oldValue));
  }
  return 1;
}

int CGUnit_C::OnMoveEvent(NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  CMovementStatus update;
  msg->Get(update.transport);
  msg->Get(update.transRelPosition.x);
  msg->Get(update.transRelPosition.y);
  msg->Get(update.transRelPosition.z);
  msg->Get(update.transRelFacing);
  msg->Get(update.worldPosition.x);
  msg->Get(update.worldPosition.y);
  msg->Get(update.worldPosition.z);
  msg->Get(update.worldFacing);
  msg->Get(update.pitch);
  msg->Get(update.moveFlags);

  switch (msgId) {
    case MSG_MOVE_START_FORWARD: OnMoveStart(eventTime, update, 1); return 1;
    case MSG_MOVE_START_BACKWARD: OnMoveStart(eventTime, update, 0); return 1;
    case MSG_MOVE_STOP: OnMoveStop(eventTime, update); return 1;
    case MSG_MOVE_START_STRAFE_LEFT: OnStrafeStart(eventTime, update, 1); return 1;
    case MSG_MOVE_START_STRAFE_RIGHT: OnStrafeStart(eventTime, update, 0); return 1;
    case MSG_MOVE_STOP_STRAFE: OnStrafeStop(eventTime, update); return 1;
    case MSG_MOVE_JUMP: OnJump(eventTime, update); return 1;
    case MSG_MOVE_START_TURN_LEFT: OnTurnStart(eventTime, update, 1); return 1;
    case MSG_MOVE_START_TURN_RIGHT: OnTurnStart(eventTime, update, 0); return 1;
    case MSG_MOVE_STOP_TURN: OnTurnStop(eventTime, update); return 1;
    case MSG_MOVE_START_PITCH_UP: OnPitchStart(eventTime, update, 1); return 1;
    case MSG_MOVE_START_PITCH_DOWN: OnPitchStart(eventTime, update, 0); return 1;
    case MSG_MOVE_STOP_PITCH: OnPitchStop(eventTime, update); return 1;
    case MSG_MOVE_SET_RUN_MODE: OnSetRunMode(eventTime, update, 1); return 1;
    case MSG_MOVE_SET_WALK_MODE: OnSetRunMode(eventTime, update, 0); return 1;
    case MSG_MOVE_TELEPORT: OnTeleport(eventTime, update); return 1;
    case MSG_MOVE_TELEPORT_ACK: OnTeleportAck(eventTime, update); return 1;
    case MSG_MOVE_START_SWIM: OnSwimStart(eventTime, update); return 1;
    case MSG_MOVE_STOP_SWIM: OnSwimStop(eventTime, update); return 1;
    case MSG_MOVE_SET_RUN_SPEED: OnRunSpeedChange(eventTime, update, msg); return 1;
    case MSG_MOVE_SET_WALK_SPEED: OnWalkSpeedChange(eventTime, update, msg); return 1;
    case MSG_MOVE_SET_SWIM_SPEED: OnSwimSpeedChange(eventTime, update, msg); return 1;
    case MSG_MOVE_SET_TURN_RATE: OnTurnRateChange(eventTime, update, msg); return 1;
    case MSG_MOVE_TOGGLE_COLLISION_CHEAT: OnToggleCollision(eventTime, update); return 1;
    case MSG_MOVE_SET_FACING: OnSetFacing(eventTime, update); return 1;
    case MSG_MOVE_SET_PITCH: OnSetPitch(eventTime, update); return 1;
    case MSG_MOVE_ROOT:
      m_move.m_moveFlags = (m_move.m_moveFlags & 0xFF87DFFF) | 0x2000;
      return 1;
    case MSG_MOVE_UNROOT:
      m_move.m_moveFlags &= ~0x2000u;
      return 1;
    case MSG_MOVE_HEARTBEAT: OnMoveHeartBeat(eventTime, update); return 1;
    default: return 0;
  }
}

void CGUnit_C::OnMoveStopLocalNoUpdate(unsigned long eventTime) {
  if (static_cast<CMovement &>(m_move).OnMoveStop(eventTime)) {
    UpdateBaseAnimation(ANIM_STATE_STOP, 0);
  }
}

void CGUnit_C::OnSetRunModeLocalNoUpdate(unsigned long eventTime, int run) {
  static_cast<CMovement &>(m_move).OnSetRunMode(eventTime, run);
  UpdateBaseAnimation(0);
}

void CGUnit_C::OnSetFacingLocalNoUpdate(unsigned long eventTime, float facing) {
  static_cast<CMovement &>(m_move).OnSetFacing(eventTime, facing);
}

void CGUnit_C::OnSetFacingGUIDLocalNoUpdate(unsigned long eventTime, const unsigned __int64 &guid) {
  CGObject_C *object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
  if (object) {
    NTempest::C3Vector target = object->GetPosition();
    NTempest::C3Vector position = GetPosition();
    static_cast<CMovement &>(m_move).OnSetFacing(eventTime, CalculateFacingTo(position, target));
  }
}

void CGUnit_C::OnTeleportNoUpdate(unsigned long eventTime, const NTempest::C3Vector &position, float facing) {
  static_cast<CMovement &>(m_move).OnTeleport(eventTime, position, facing);
  UpdateBaseAnimation(ANIM_STATE_NONE, 0);
}

void CGUnit_C::OnMonsterMove(unsigned long eventTime, CDataStore *msg) {
  NTempest::C3Vector serverLoc;
  msg->Get(serverLoc.x);
  msg->Get(serverLoc.y);
  msg->Get(serverLoc.z);

  static float maxDelta = 4.0f;
  static float maxDeltaSquared = maxDelta * maxDelta;

  NTempest::C3Vector position = GetPosition();
  NTempest::C3Vector delta = serverLoc - position;
  if (delta.x * delta.x + delta.y * delta.y > maxDeltaSquared || NTempest::CMath::fabs_(delta.z) > maxDelta) {
    CMovement::LogWrite(
        "0x%016I64X: sync teleport from (%f,%f,%f) to (%f,%f,%f) - move flags: 0x%X", GetGUID(), position.x, position.y,
        position.z, serverLoc.x, serverLoc.y, serverLoc.z, m_move.m_moveFlags
    );
    OnTeleportNoUpdate(eventTime, serverLoc, GetFacing());
  }

  m_debugPathPoints.Clear();
  m_serverLoc = serverLoc;

  unsigned int moveIndex;
  unsigned char facingType;
  msg->Get(moveIndex);
  msg->Get(facingType);

  NTempest::C3Vector finalFacingSpot;
  unsigned __int64 finalFacingGUID = 0;
  float finalFacingAngle = 0.0f;
  switch (facingType) {
    case 1:
      OnMoveStopLocalNoUpdate(eventTime);
      return;
    case 3:
      msg->Get(finalFacingGUID);
      break;
    case 4:
      msg->Get(finalFacingAngle);
      break;
  }

  unsigned int flags;
  unsigned int moveTime;
  unsigned int numPoints;
  msg->Get(flags);
  msg->Get(moveTime);
  msg->Get(numPoints);
  FATALASSERT(numPoints > 0);

  TSStackArray<NTempest::C3Vector> points(
      _alloca(sizeof(NTempest::C3Vector) * (numPoints + 3)),
      numPoints + 3,
      2
  );
  position = GetPosition();
  float facing = GetFacing();
  points[0] = NTempest::C3Vector(
      position.x - static_cast<float>(cos(facing)),
      position.y - static_cast<float>(sin(facing)),
      position.z
  );
  points[1] = position;

  NTempest::C3Vector endPoint;
  if (flags & 0x200) {
    for (unsigned int i = 0; i < numPoints; ++i) {
      msg->Get(endPoint.x);
      msg->Get(endPoint.y);
      msg->Get(endPoint.z);
      points.New(endPoint);
    }
  } else {
    msg->Get(endPoint.x);
    msg->Get(endPoint.y);
    msg->Get(endPoint.z);
    for (unsigned int i = 1; i < numPoints; ++i) {
      unsigned int packedDeltas;
      msg->Get(packedDeltas);
      int xDelta = static_cast<int>(packedDeltas << 20) >> 20;
      int yDelta = static_cast<int>(packedDeltas << 8) >> 20;
      int zDelta = static_cast<signed char>(packedDeltas >> 24);
      points.New(NTempest::C3Vector(
          endPoint.x - static_cast<float>(xDelta) * 0.125f, endPoint.y - static_cast<float>(yDelta) * 0.125f,
          endPoint.z - static_cast<float>(zDelta) * 0.125f
      ));
    }
    points.New(endPoint);
  }

  points.New(points[points.Count() - 1]);
  if (facingType == 2) {
    finalFacingSpot = endPoint;
  }

  if (points.Count() > 4) {
    position = GetPosition();
    float closestDistance = FLT_MAX;
    unsigned int closest_index = 1;
    for (unsigned int i = 2; i < points.Count() - 1; ++i) {
      NTempest::C3Vector offset = position - points[i];
      float distance = offset.SquaredMag();
      if (distance <= closestDistance) {
        closestDistance = distance;
        closest_index = i;
      }
    }
    FATALASSERT(closest_index > 1);

    if (closest_index < points.Count() - 2) {
      NTempest::C3Vector direction = points[closest_index + 1] - points[closest_index];
      direction.Normalize();
      NTempest::C4Plane plane(direction, points[closest_index]);
      if (plane.DistSigned(position) >= 0.0f) {
        ++closest_index;
      }
    }

    if (closest_index != 2) {
      unsigned int newCount = points.Count() - (closest_index - 2);
      memmove(&points[2], &points[closest_index], sizeof(NTempest::C3Vector) * (newCount - 2));
      points.SetCount(newCount);
    }
  }

  if (points.Count() > 3) {
    NTempest::C3Vector destination = points[points.Count() - 1];
    position = GetPosition();
    float zDelta = position.z - destination.z;
    if (zDelta < 1.0f) {
      zDelta = 0.0f;
    }
    float squaredDistance =
        (position.x - destination.x) * (position.x - destination.x) +
        (position.y - destination.y) * (position.y - destination.y) + zDelta * zDelta;
    if (squaredDistance > 0.027777778f) {
      float distance = NTempest::CMath::sqrt_(squaredDistance);
      float speed = m_move.m_runSpeed * 2.0f;
      if (flags & 0x200) {
        speed = m_move.m_runSpeed * 10.0f;
      }
      if (moveTime) {
        float requestedSpeed = distance / (static_cast<float>(moveTime) * 0.001f);
        if (requestedSpeed < speed) {
          speed = requestedSpeed;
        }
      }
      if (speed > 0.00000095367432f) {
        int duration = static_cast<int>(distance / speed * 1000.0f);
        moveTime = duration > 1 ? duration : 1;

        m_debugPathPoints.Add(points.Count(), points.Ptr());
        m_numDebugPathNodes = points.Count() + 1;
        if (flags & 0x200) {
          position = GetPosition();
          m_debugPathPoints.Add(1, &position);

          NTempest::C3Spline_CatmullRom debugSpline;
          debugSpline.SetPoints(points.Ptr(), points.Count());
          float step = 1.0f / (debugSpline.cachedLength * 3.0f);
          for (float t = 0.0f; t < 1.0f; t += step) {
            NTempest::C34Matrix matrix;
            matrix.Identity();
            debugSpline.Frame(t, matrix, NTempest::C3Spline::EVAL_ARCLENGTH);
            NTempest::C3Vector sample(matrix.d0, matrix.d1, matrix.d2);
            m_debugPathPoints.Add(1, &sample);
          }
        }

        static_cast<CMovement &>(m_move).OnSpline(OsGetAsyncTimeMs(), points.Ptr(), points.Count(), moveTime, flags);
        switch (facingType) {
          case 2:
            static_cast<CMovement &>(m_move).OnSplineDoneFace(finalFacingSpot);
            break;
          case 3:
            static_cast<CMovement &>(m_move).OnSplineDoneFace(finalFacingGUID);
            break;
          case 4:
            static_cast<CMovement &>(m_move).OnSplineDoneFace(finalFacingAngle);
            break;
        }
        return;
      }
    }
  }

  OnMoveStopLocalNoUpdate(eventTime);
  switch (facingType) {
    case 2:
      position = GetPosition();
      OnSetFacingLocalNoUpdate(eventTime, CalculateFacingTo(position, finalFacingSpot));
      break;
    case 3:
      OnSetFacingGUIDLocalNoUpdate(eventTime, finalFacingGUID);
      break;
    case 4:
      OnSetFacingLocalNoUpdate(eventTime, finalFacingAngle);
      break;
  }
}

int CGUnit_C::OnForceMoveChange(unsigned long eventTime, NETMESSAGE msgID, CDataStore *msg) {
  switch (msgID) {
    case SMSG_FORCE_SPEED_CHANGE: {
      float speed;
      msg->Get(speed);
      OnRunSpeedChangeLocal(eventTime, CMSG_FORCE_SPEED_CHANGE_ACK, speed);
      return 1;
    }
    case SMSG_FORCE_SWIM_SPEED_CHANGE: {
      float speed;
      msg->Get(speed);
      OnSwimSpeedChangeLocal(eventTime, CMSG_FORCE_SWIM_SPEED_CHANGE_ACK, speed);
      return 1;
    }
    case SMSG_FORCE_MOVE_ROOT:
      m_move.m_moveFlags = (m_move.m_moveFlags & 0xFF87DFFF) | 0x2000;
      SendMovementUpdate(CMSG_FORCE_MOVE_ROOT_ACK);
      CGInputControl::GetActive()->UpdatePlayer(eventTime);
      return 1;
    case SMSG_FORCE_MOVE_UNROOT:
      m_move.m_moveFlags &= ~0x2000u;
      SendMovementUpdate(CMSG_FORCE_MOVE_UNROOT_ACK);
      CGInputControl::GetActive()->UpdatePlayer(eventTime);
      return 1;
    default:
      return 1;
  }
}

void CGUnit_C::OnMoveStart(unsigned long eventTime, const CMovementStatus &update, int forward) {
  if (m_move.m_moveFlags & 0x2400) {
    CMovement::LogWrite("0x%016I64X: Immobilized\n", GetGUID());
    return;
  }
  static_cast<CMovement &>(m_move).UpdateStatus(eventTime, update);
  static_cast<CMovement &>(m_move).OnMoveStart(eventTime, forward);
  UpdateBaseAnimation(0);
}

void CGUnit_C::OnStrafeStart(unsigned long eventTime, const CMovementStatus &update, int left) {
  if (m_move.m_moveFlags & 0x2400) {
    CMovement::LogWrite("0x%016I64X: Immobilized\n", GetGUID());
    return;
  }
  static_cast<CMovement &>(m_move).UpdateStatus(eventTime, update);
  static_cast<CMovement &>(m_move).OnStrafeStart(eventTime, left);
  UpdateBaseAnimation(0);
}

void CGUnit_C::OnStrafeStartLocalNoUpdate(unsigned long eventTime, int left) {
  static_cast<CMovement &>(m_move).OnStrafeStart(eventTime, left);
  UpdateBaseAnimation(0);
}

void CGUnit_C::OnMoveStop(unsigned long eventTime, const CMovementStatus &update) {
  static_cast<CMovement &>(m_move).UpdateStatus(eventTime, update);
  static_cast<CMovement &>(m_move).OnMoveStop(eventTime);
  UpdateBaseAnimation(ANIM_STATE_STOP, 0);
}

void CGUnit_C::OnStrafeStop(unsigned long eventTime, const CMovementStatus &update) {
  static_cast<CMovement &>(m_move).UpdateStatus(eventTime, update);
  static_cast<CMovement &>(m_move).OnStrafeStop(eventTime);
  UpdateBaseAnimation(ANIM_STATE_STOP, 0);
}

void CGUnit_C::OnStrafeStopLocalNoUpdate(unsigned long eventTime) {
  if (static_cast<CMovement &>(m_move).OnStrafeStop(eventTime)) {
    UpdateBaseAnimation(ANIM_STATE_STOP, 0);
  }
}

void CGUnit_C::OnJump(unsigned long eventTime, const CMovementStatus &update) {
  if (m_move.m_moveFlags & 0x2400) {
    CMovement::LogWrite("0x%016I64X: Immobilized\n", GetGUID());
    return;
  }
  static_cast<CMovement &>(m_move).UpdateStatus(eventTime, update);
  static_cast<CMovement &>(m_move).OnJump(eventTime);
  UpdateBaseAnimation(0);
}

void CGUnit_C::OnTurnStart(unsigned long eventTime, const CMovementStatus &update, int left) {
  if (m_unit->flags & 0x40000) {
    CMovement::LogWrite("0x%016I64X: Stunned\n", GetGUID());
    return;
  }
  static_cast<CMovement &>(m_move).UpdateStatus(eventTime, update);
  static_cast<CMovement &>(m_move).OnTurnStart(eventTime, left);
  UpdateBaseAnimation(0);
}

void CGUnit_C::OnTurnStop(unsigned long eventTime, const CMovementStatus &update) {
  static_cast<CMovement &>(m_move).UpdateStatus(eventTime, update);
  static_cast<CMovement &>(m_move).OnTurnStop(eventTime);
  UpdateBaseAnimation(0);
}

void CGUnit_C::OnPitchStart(unsigned long eventTime, const CMovementStatus &update, int up) {
  if (m_unit->flags & 0x40000) {
    CMovement::LogWrite("0x%016I64X: Stunned\n", GetGUID());
    return;
  }
  static_cast<CMovement &>(m_move).UpdateStatus(eventTime, update);
  static_cast<CMovement &>(m_move).OnPitchStart(eventTime, up);
  UpdateBaseAnimation(0);
}

void CGUnit_C::OnPitchStop(unsigned long eventTime, const CMovementStatus &update) {
  static_cast<CMovement &>(m_move).UpdateStatus(eventTime, update);
  static_cast<CMovement &>(m_move).OnPitchStop(eventTime);
  UpdateBaseAnimation(0);
}

void CGUnit_C::OnSwimStart(unsigned long eventTime, const CMovementStatus &update) {
  static_cast<CMovement &>(m_move).UpdateStatus(eventTime, update);
  static_cast<CMovement &>(m_move).OnSwimStart(eventTime);
  UpdateBaseAnimation(0);
}

void CGUnit_C::OnSwimStop(unsigned long eventTime, const CMovementStatus &update) {
  static_cast<CMovement &>(m_move).UpdateStatus(eventTime, update);
  static_cast<CMovement &>(m_move).OnSwimStop(eventTime);
  UpdateBaseAnimation(0);
}

void CGUnit_C::OnMoveHeartBeat(unsigned long eventTime, const CMovementStatus &update) {
  static_cast<CMovement &>(m_move).UpdateStatus(eventTime, update);
}

void CGUnit_C::OnRunSpeedChange(unsigned long eventTime, const CMovementStatus &update, CDataStore *msg) {
  int wasWalking = IsWalking();
  float speed;
  msg->Get(speed);
  static_cast<CMovement &>(m_move).UpdateStatus(eventTime, update);
  static_cast<CMovement &>(m_move).OnRunSpeedChange(eventTime, speed);
  if (wasWalking != IsWalking()) {
    UpdateBaseAnimation(0);
  }
  UpdateMovementAnimSpeed(1, INVALID_ANIM_STATE);
}

void CGUnit_C::OnWalkSpeedChange(unsigned long eventTime, const CMovementStatus &update, CDataStore *msg) {
  int wasWalking = IsWalking();
  float speed;
  msg->Get(speed);
  static_cast<CMovement &>(m_move).UpdateStatus(eventTime, update);
  static_cast<CMovement &>(m_move).OnWalkSpeedChange(eventTime, speed);
  if (wasWalking != IsWalking()) {
    UpdateBaseAnimation(0);
  }
  UpdateMovementAnimSpeed(1, INVALID_ANIM_STATE);
}

void CGUnit_C::OnSwimSpeedChange(unsigned long eventTime, const CMovementStatus &update, CDataStore *msg) {
  float speed;
  msg->Get(speed);
  static_cast<CMovement &>(m_move).UpdateStatus(eventTime, update);
  static_cast<CMovement &>(m_move).OnSwimSpeedChange(eventTime, speed);
  UpdateBaseAnimation(0);
  UpdateMovementAnimSpeed(1, INVALID_ANIM_STATE);
}

void CGUnit_C::OnTurnRateChange(unsigned long eventTime, const CMovementStatus &update, CDataStore *msg) {
  float rate;
  msg->Get(rate);
  static_cast<CMovement &>(m_move).UpdateStatus(eventTime, update);
  static_cast<CMovement &>(m_move).OnTurnRateChange(eventTime, rate);
}

void CGUnit_C::OnSetRunMode(unsigned long eventTime, const CMovementStatus &update, int run) {
  static_cast<CMovement &>(m_move).UpdateStatus(eventTime, update);
  OnSetRunModeLocalNoUpdate(eventTime, run);
}

void CGUnit_C::OnToggleCollision(unsigned long eventTime, const CMovementStatus &update) {
  FATALASSERT(GetGUID() != CGUnit_C::GetActiveMover());
  static_cast<CMovement &>(m_move).UpdateStatus(eventTime, update);
  static_cast<CMovement &>(m_move).EnableCollision(eventTime, !(m_move.m_moveFlags & 0x800));
}

void CGUnit_C::OnEnableCollisionLocalNoUpdate(unsigned long eventTime) {
  static_cast<CMovement &>(m_move).EnableCollision(eventTime, 1);
}

void CGUnit_C::OnDisableCollisionLocalNoUpdate(unsigned long eventTime) {
  static_cast<CMovement &>(m_move).EnableCollision(eventTime, 0);
}

void CGUnit_C::OnToggleCollisionLocal(unsigned long eventTime) {
  FATALASSERT(GetGUID() == CGUnit_C::GetActiveMover());
  static_cast<CMovement &>(m_move).ToggleCollision(eventTime);
  SendMovementUpdate(MSG_MOVE_TOGGLE_COLLISION_CHEAT);
}

void CGUnit_C::OnSetFacing(unsigned long eventTime, const CMovementStatus &update) {
  static_cast<CMovement &>(m_move).UpdateStatus(eventTime, update);
  static_cast<CMovement &>(m_move).OnSetFacing(eventTime, GetFacing());
}

void CGUnit_C::OnSetPitch(unsigned long eventTime, const CMovementStatus &update) {
  static_cast<CMovement &>(m_move).UpdateStatus(eventTime, update);
  static_cast<CMovement &>(m_move).OnSetPitch(eventTime, m_move.m_pitch);
}

void CGUnit_C::OnTeleport(unsigned long eventTime, const CMovementStatus &update) {
  static_cast<CMovement &>(m_move).UpdateStatus(eventTime, update);
  OnTeleportNoUpdate(eventTime, GetPosition(), GetFacing());
}

void CGUnit_C::OnTeleportAck(unsigned long eventTime, const CMovementStatus &update) {
  (void)ClntObjMgrGetPlayerType();
  NTempest::C3Vector oldPos;
  GetPosition(oldPos);
  FATALASSERT(GetGUID() == CGUnit_C::GetActiveMover());
  static_cast<CMovement &>(m_move).UpdateStatusLocal(eventTime, update);
  OnTeleportLocalNoUpdate(eventTime, GetPosition(), GetFacing());

  NTempest::C3Vector delta = m_move.GetPosition() - oldPos;
  if (delta.SquaredMag() > 900.0f) {
    CWorld::Preload(GetPosition());
    CGObject_C::UpdateAllWorldObjects();
  }

  CDataStore outBound;
  outBound.Put(MSG_MOVE_TELEPORT_ACK);
  outBound.Finalize();
  ClientServices_Send(&outBound);
}

static int OnUnitMoveEvent(NETMESSAGE msgId, unsigned long eventTime, unsigned __int64 guid, CDataStore *msg) {
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (unit) {
    return unit->OnMoveEvent(msgId, eventTime, msg);
  }

  CMovement::LogWrite("0x%016I64X: Skipping move (0x%X) for unknown guid\n", guid, eventTime);
  msg->Seek(msg->Size());
  return 0;
}

static int OnUnitMoveEventActive(void *param, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  FATALASSERT(msg);
  return OnUnitMoveEvent(msgId, eventTime, CGUnit_C::GetActiveMover(), msg);
}

static int OnUnitMoveEventNoActive(void *param, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  FATALASSERT(msg);

  unsigned __int64 guid;
  msg->Get(guid);
  if (guid != CGUnit_C::GetActiveMover()) {
    return OnUnitMoveEvent(msgId, eventTime, guid, msg);
  }

  msg->Seek(msg->Size());
  return 1;
}

static int OnMonsterMoveEvent(void* param, NETMESSAGE msgId, unsigned long eventTime, CDataStore* msg) {
  unsigned __int64 guid;
  msg->Get(guid);
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (unit) {
    unit->OnMonsterMove(eventTime, msg);
    return 1;
  }

  CMovement::LogWrite("0x%016I64X: Skipping queued moves (0x%X) for invalid guid\n", guid, eventTime);
  msg->Seek(msg->Size());
  return 0;
}

static int OnForceMoveChange(void *, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(CGUnit_C::GetActiveMover(), __FILE__, __LINE__));
  if (unit) {
    return unit->OnForceMoveChange(eventTime, msgId, msg);
  }
  return 1;
}

static int OnUnitMountCancelledEvent(void* param, NETMESSAGE msgId, unsigned long eventTime, CDataStore* msg) {
  ASSERT(msg);
  unsigned __int64 guid;
  msg->Get(guid);
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (unit) {
    unit->OnMountCancelled();
  }
  return 1;
}

static int OnSpecialMountAnim(void* param, NETMESSAGE msgId, unsigned long eventTime, CDataStore* msg) {
  ASSERT(msg);
  unsigned __int64 guid;
  msg->Get(guid);
  CGObject_C *object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
  if (object) {
    object->OnSpecialMountAnim();
  }
  return 1;
}

static int OnUnitReaction(void*, NETMESSAGE msgId, unsigned long, CDataStore* msg) {
  ASSERT(msgId == SMSG_AI_REACTION);
  unsigned __int64 unitGUID;
  int              reaction;
  msg->Get(unitGUID);
  msg->Get(reaction);
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unitGUID, __FILE__, __LINE__));
  if (unit) {
    unit->OnEncounter(static_cast<AI_REACTION>(reaction));
  }
  return 1;
}

int AuraMirrorHandler(unsigned __int64 guid, unsigned int offset, unsigned int bytes, const void *prevValue, void *param) {
  unsigned int slot = (offset - 200) >> 2;
  FATALASSERT(prevValue);
  int       previousValue = *static_cast<const int *>(prevValue);
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (unit) {
    unit->OnAuraChanged(slot, previousValue);
  }
  return 1;
}

static int TargetMirrorHandler(unsigned __int64 guid, unsigned int, unsigned int, const void*, void*) {
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  ASSERT(unit);
  if (guid != ClntObjMgrGetActivePlayer()) {
    unit->LookAtTarget();
  }
  return 1;
}

static int ChannelObjectMirrorHandler(unsigned __int64 guid, unsigned int, unsigned int, const void*, void*) {
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (unit) {
    unit->ClearFishingObject();
  }
  return 1;
}

static void PlayerNameGuildCallback(int guildID, const unsigned __int64& guid, void* arg, bool granted) {
  if (granted) {
    PlayerNameTriggerNameRegenerate(*static_cast<HPLAYERNAME *>(arg));
  }
}

void CGUnit_C::OnEncounter(AI_REACTION reaction) {
  ASSERT(reaction < NUM_AI_REACTIONS);
  if (reaction == AI_REACT_ALERT) {
    PlayUnitSound(UNITSOUNDTYPE_ALERT, 0);
  } else if (reaction == AI_REACT_HOSTILE) {
    PlayUnitSound(UNITSOUNDTYPE_AGGRO, 0);
  }
}

void CGUnit_C::OnSpecialMountAnim() {
  UpdateBaseAnimation(ANIM_STATE_SPECIALMOUNTANIM, 0);
}

void CGUnit_C::UnitInitializeMountModel(HMODEL model) {
  ModelSetEventCallback(model, MountedAnimEventCallback, this, 0);
  unsigned int index = g_mountSeqEndList.Count();
  while (index) {
    --index;
    InstallSeqEndHandler(model, g_mountSeqEndList[index]);
  }
}

HMODEL CGUnit_C::GetMountModel() {
  const CreatureDisplayInfoRec *displayInfo = g_creatureDisplayInfoDB.GetRecord(m_unit->mountDisplayID);
  ASSERT(displayInfo);
  const CreatureModelDataRec *modelData = g_creatureModelDataDB.GetRecord(displayInfo->m_modelID);
  ASSERT(modelData);

  HMODEL model = ObjectModelCreate(modelData->m_ModelName, static_cast<OBJECT_TYPE>(GetType()), 0x100800);
  MarkFootstepAnimations(model);
  InitializeTextureVariations(displayInfo, model, modelData);
  m_mountedFootprintID = modelData->m_footprintTextureID;
  m_mountedFootprintSize.x = modelData->m_footprintTextureWidth * 0.027777778f;
  m_mountedFootprintSize.y = modelData->m_footprintTextureLength * 0.027777778f;
  return model;
}

const CreatureSoundDataRec *CGUnit_C::GetMountSoundDataRec() const {
  const CreatureDisplayInfoRec *displayInfo = g_creatureDisplayInfoDB.GetRecord(m_unit->mountDisplayID);
  if (!displayInfo) {
    return 0;
  }

  const CreatureSoundDataRec *soundData = g_creatureSoundDataDB.GetRecord(displayInfo->m_soundID);
  if (!soundData) {
    const CreatureModelDataRec *modelData = g_creatureModelDataDB.GetRecord(displayInfo->m_modelID);
    if (modelData) {
      soundData = g_creatureSoundDataDB.GetRecord(modelData->m_soundID);
    }
  }
  return soundData;
}

const CreatureSoundDataRec *CGUnit_C::GetSoundData() const {
  return m_mountedSoundData ? m_mountedSoundData : m_soundData;
}

void CGUnit_C::CreateUnitMount() {
  if (m_flags & 0x10) {
    return;
  }

  HMODEL mountModel;
  if (m_fadingPureMountModel) {
    mountModel = m_fadingPureMountModel;
    ModelSetVertexAlpha(mountModel, 255, 1);
    m_fadingPureMountModel = 0;
  } else {
    mountModel = GetMountModel();
  }

  if (mountModel) {
    UnitInitializeMountModel(mountModel);
    HMODEL charModel = m_model;
    FATALASSERT(charModel);
    ModelRemoveObjectLookAt(charModel, 6);
    SetObjectModel(mountModel);
    ModelAddLink(mountModel, 0, charModel, GetRenderScale() / m_fadingMountScale);
    SetTempCharModel(charModel);
    SetStandStateAnim(m_standStateAnim);
    SetWalkStateAnim(m_walkStateAnim);
    m_mountedSoundData = GetMountSoundDataRec();
    m_flags |= 0x10;
    SetRenderScale(m_fadingMountScale);
    UpdateBaseAnimation(0);
  }
}

void CGUnit_C::DestroyUnitMount(int doNotUpdateAnim) {
  if (m_flags & 0x10) {
    HMODEL       charModel = 0;
    unsigned int numModels = 1;
    if (ModelGetLinkPoint(m_model, 0, &charModel, &numModels)) {
      FATALASSERT(charModel);
      HMODEL mountModel = m_model;
      FATALASSERT(mountModel);
      ModelClearLink(mountModel, 0);
      SetObjectModel(charModel);
      HandleClose(mountModel);
      SetStandStateAnim(m_standStateAnim);
      SetWalkStateAnim(m_walkStateAnim);

      AuraVisual *visual = m_auraVisual;
      for (unsigned int index = 0; index < 12; ++index, ++visual) {
        if (visual->HasArt() && !visual->IsWorldModel() && visual->GetModel()) {
          HMODEL auraModel = visual->GetModel();
          GEOCOMPONENTLINKS linkPoint =
              UnitEffectGetLinkPointFromAttachment(static_cast<UNITEFFECTATTACHPPOINT>(index));
          if (ModelAddLink(charModel, linkPoint, auraModel, 1.0f)) {
            if (ModelHasSequenceId(auraModel, 1)) {
              ModelSetSequence(auraModel, 1, 0);
              ModelSetSeqFinishedHandler(auraModel, 1, OnFirstAuraSequenceFinished, auraModel);
            }
          } else {
            visual->Clear();
          }
        }
      }

      SetRenderScale(GetRenderScale() / m_fadingMountScale);
      m_flags &= ~0x10U;
      m_mountedSoundData = 0;
      if (!doNotUpdateAnim) {
        UpdateBaseAnimation(0);
      }
      ClearMountAnimState();
      ClearTempCharModel();
    }
  } else if (!doNotUpdateAnim) {
    UpdateBaseAnimation(0);
  }
}

void CGUnit_C::UpdateUnitMountInfo(int immediate, unsigned int changedFlags) {
  unsigned int flags = m_unit->flags;
  if (flags & 0x3000) {
    if (changedFlags & 0x2000) {
      if ((flags & 0x2000) && (flags & 0x1000)) {
        CreateUnitMount();
        OnMount();
        DestroyFadingMounts();
      } else {
        if (!immediate && (flags & 0x1000)) {
          CreateFadeOutMount();
        }
        DestroyUnitMount(0);
        OnDismount();
      }
    }

    if (changedFlags & 0x1000) {
      flags = m_unit->flags;
      if (flags & 0x1000) {
        if (flags & 0x2000) {
          CreateUnitMount();
          OnMount();
          DestroyFadingMounts();
        } else {
          DestroyUnitMount(0);
          OnDismount();
          if (!immediate) {
            CreateFadeInMount();
          }
        }
      } else {
        DestroyUnitMount(0);
        DestroyFadingMounts();
        OnDismount();
      }
    }
  } else if ((m_flags & 0x10) || m_fadingPureMountModel) {
    if (!m_fadingPureMountModel || m_pureMountFadeMode == PUREMOUNTFADE_IN) {
      CreateFadeOutMount();
    }
    OnDismount();
    DestroyUnitMount(0);
  }
}

void CGUnit_C::CreateFadeOutMount() {
  int created = 0;
  if (!m_fadingPureMountModel) {
    if (m_flags & 0x10) {
      m_fadingPureMountModel = static_cast<HMODEL>(HandleDuplicate(m_model));
    } else {
      m_fadingPureMountModel = GetMountModel();
    }
    m_fadingMountFacing = GetFacing();
    m_fadingMountPos = GetPosition();
    if (m_fadingPureMountModel) {
      ObjectModelSetSequence(m_fadingPureMountModel, 0, 0, 0);
    }
    created = 1;
  }

  if (m_fadingPureMountModel && (created || m_pureMountFadeMode != PUREMOUNTFADE_OUT)) {
    m_pureMountFadeMode = PUREMOUNTFADE_OUT;
    m_pureMountFadeStartTime = OsGetAsyncTimeMs();
  }
}

void CGUnit_C::CreateFadeInMount() {
  int created = 0;
  if (!m_fadingPureMountModel) {
    m_fadingMountPos = GetPosition();
    m_fadingPureMountModel = GetMountModel();
    m_fadingMountFacing = GetFacing();
    m_fadingMountScale = GetMountScale();
    if (m_fadingPureMountModel) {
      ObjectModelSetSequence(m_fadingPureMountModel, 0, 0, 0);
      UnitInitializeMountModel(m_fadingPureMountModel);
    }
    created = 1;
  }

  if (m_fadingPureMountModel && (created || m_pureMountFadeMode != PUREMOUNTFADE_IN)) {
    m_pureMountFadeMode = PUREMOUNTFADE_IN;
    m_pureMountFadeStartTime = OsGetAsyncTimeMs();
  }
}

void CGUnit_C::OnMountCancelled() {
  if (m_unit->flags & 0x1000) {
    CreateFadeOutMount();
  }
}

void OnPendingMoveStateChange(unsigned __int64 unit, int msgId, unsigned long eventTime) {
  CGUnit_C *mover = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  FATALASSERT(mover);
  mover->OnPendingMoveStateChange(static_cast<NETMESSAGE>(msgId));
}

void OnCollideRedirected(unsigned __int64 unit, unsigned long eventTime) {
  if (unit == CGUnit_C::GetActiveMover()) {
    CGUnit_C *mover = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
    FATALASSERT(mover);
    mover->SendRedirectionMessage();
  }
}

void OnCollideStuck(unsigned __int64 unit, unsigned long eventTime) {
  if (unit == CGUnit_C::GetActiveMover()) {
    CGUnit_C *mover = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
    FATALASSERT(mover);

    CDataStore msg;
    mover->BuildMovementUpdate(MSG_MOVE_COLLIDE_STUCK, &msg);
    msg.Finalize();
    ClientServices_Send(&msg);
  }
}

void OnCollideFallLand(unsigned __int64 unit, unsigned long eventTime) {
  CGUnit_C *mover = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  FATALASSERT(mover);
  mover->OnCollideFallLand(eventTime);
}

void OnCollideFalling(unsigned __int64 unit, unsigned long eventTime) {
  CGUnit_C *mover = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  FATALASSERT(mover);
  mover->OnCollideFalling(eventTime);
}

void OnMoveUpdate(unsigned __int64 unit, unsigned long eventTime) {
  CGUnit_C *unitptr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  FATALASSERT(unitptr);

  unitptr->UpdateWorldObject();

  NTempest::C3Vector ripplePos;
  NTempest::C3Vector waterDir(0.0f);
  int                deep;
  unsigned int       liquidStatus = 15;
  float              depth = 0.0f;
  float              surfaceColPt = 0.0f;
  int                inWater = CWorld::QueryObjectLiquid(unitptr->GetWorldObject(), liquidStatus, surfaceColPt, waterDir, deep);
  if (inWater) {
    ripplePos = unitptr->GetPosition();
    depth = surfaceColPt - ripplePos.z;
    unitptr->m_move.m_waterSurfaceElev = surfaceColPt - unitptr->GetObjectHeight() * 0.75f;
  }

  if (unit == CGUnit_C::GetActiveMover()) {
    unsigned int moveFlags = unitptr->m_move.m_moveFlags;
    if ((moveFlags & 0x01000000) || ((unitptr->GetType() & TYPE_PLAYER) && !unitptr->m_move.m_transportGUID &&
                                     ((moveFlags & 2) || !(moveFlags & 0x00C00004)) && !(moveFlags & 1)))
    {
      unitptr->UpdateSwimmingStatus(eventTime, inWater, depth);
    }
  }

  if (inWater && !(liquidStatus & 3) && unitptr->GetObjectHeight() > depth) {
    ripplePos = unitptr->GetPosition();
    ripplePos.z = surfaceColPt;
    CWorld::WaterRipple(ripplePos, 0.73333335f, 1.0f, 0.16666667f, 6.6666665f, 0.055555556f);
  }
}

int UnitGetObjectPosition(const unsigned __int64 &guid, NTempest::C3Vector *position) {
  CGObject_C *object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
  if (!object) {
    return 0;
  }

  *position = object->GetPosition();
  return 1;
}

float UnitCalculateFacingTo(const NTempest::C3Vector &position, const NTempest::C3Vector &destination) {
  return CalculateFacingTo(position, destination);
}

void UnitUpdateMovementAnim(const unsigned __int64 &unit) {
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  if (unitPtr) {
    unitPtr->UpdateBaseAnimation(0);
    unitPtr->UpdateMovementAnimSpeed(1, INVALID_ANIM_STATE);
  }
}

void UnitNotifyStopped(const unsigned __int64 &guid, bool moveComplete) {
}

CGUnit::CGUnit(
    unsigned long *storage,
    const NTempest::C3Vector &position,
    float facing,
    const unsigned __int64 &guid
)
    : m_unit(reinterpret_cast<CGUnitData *>(storage)),
      m_move(position, facing, guid) {
}

CGUnit::~CGUnit() {
}

UNITAFFILIATION CGUnit::GetGUIDAffiliation(unsigned __int64) const {
  return AFFILIATION_OTHER;
}

void CGUnit_C::SetStorage(unsigned long *storage) {
  CGObject_C::SetStorage(storage);
  CGUnit::SetStorage(storage + CGObject::TotalFields());
}

CGUnit_C::CGUnit_C(unsigned long *storage, unsigned long eventTime, CClientObjCreate *init)
    : CGObject_C(storage, eventTime, init),
      CGUnit(
          storage + CGObject::TotalFields(),
          init->move.status.worldPosition,
          init->move.status.worldFacing,
          *reinterpret_cast<unsigned __int64 *>(storage)
      ),
      m_questCountKilled(-1),
      m_questCountNeeded(-1),
      m_resEffectModel(0),
      m_meleeTargetDeathHold(0),
      m_precastSheatheHoldTimer(-1),
      m_customAttackSound(-1),
      m_customAttackPosition(0.0f),
      m_splashSoundID(0),
      m_disengageLookAtTimer(0),
      m_stats(0),
      m_displayInfo(0),
      m_displayInfoExtra(0),
      m_modelData(0),
      m_soundData(0),
      m_mountedSoundData(0),
      m_bloodRec(0),
      m_pendingImpactAnim(ANIM_STAND),
      m_tempCharModel(0),
      m_lastDeathTime(0),
      m_nextDeathHoldCheckTime(0),
      m_interactIconModel(0),
      m_nextAllowableBloodPool(0),
      m_currentDamageInfo(0),
      m_readySequence(0),
      m_animEndTime(0),
      m_animBaseDuration(0),
      m_animStartTime(0),
      m_flags(2),
      m_animFlags(0),
      m_footprintTextureID(-1),
      m_terrain(0),
      m_footprintSize(0.2777778f),
      m_footprintParticleScale(1.0f),
      m_spellPrecastingAnim(RESET_ANIMATION_INDICES0),
      m_spellCastingAnim(ANIM_STAND),
      m_deferredPrecastAnim(RESET_ANIMATION_INDICES0),
      m_animatingAura(0),
      m_emoteID(-1),
      m_spellCastingEffectKit(-1),
      m_spellCastingSoundID(0),
      m_spellCastingCameraShakeID(0),
      m_lastSentFacing(-10.0f),
      m_lastSentPitch(FLT_MAX),
      m_unitNameHandle(0),
      m_accumulatedXPDrop(0),
      m_castingSpell(0),
      m_interruptedSpell(0),
      m_lastSpellCastAnimTime(-1),
      m_nextBreath(-1),
      m_nextMountBreath(0),
      m_scriptRegistered(0),
      m_displayFacing(init->move.status.worldFacing),
      m_smoothFacing(init->move.status.worldFacing),
      m_forcedDisplayFacing(0.0f),
      m_deathTime(0),
      m_lastCombatTarget(0),
      m_targetUnit(0),
      m_currentBaseAnimState(3),
      m_currentBaseAnim(0),
      m_currentTorsoAnimState(0),
      m_currentTorsoAnim(0),
      m_currentMountAnimState(0),
      m_currentWoundStartTime(0),
      m_currentWoundAnimDuration(0),
      m_spellFizzleTimer(0),
      m_deathHolds(0),
      m_questGiverStatus(QUEST_GIVER_NONE),
      m_serverLoc(0.0f),
      m_numDebugPathNodes(0),
      m_spellLoopedSound(0),
      m_creatureLoopSound(0),
      m_mountedFootprintID(0),
      m_mountedFootprintSize(0.0f),
      m_fadingPureMountModel(0),
      m_pureMountFadeMode(PUREMOUNTFADE_IN),
      m_pureMountFadeStartTime(0),
      m_fadingMountFacing(0.0f),
      m_fadingMountPos(0.0f),
      m_fadingMountScale(1.0f),
      m_NPCSoundsRec(0),
      m_lastGlobalClickCount(0),
      m_pissedCount(0),
      m_numNPCPissedSounds(0),
      m_geosetHandle(0),
      m_texComponent(0),
      m_displayHealth(0),
      m_pendingHitSpellID(0),
      m_walkStateAnim(4),
      m_standStateAnim(0),
      m_baseRadius(0.0f),
      m_ammoDisplayID(0),
      m_ammoInvType(0),
      m_rangedStandTimer(0),
      m_paperDollModel(0),
      m_sheatheReasons(0),
      m_deferredSheatheFlags(0),
      m_deferredSheatheReason(SHEATHE_PLAYEREXPLICIT),
      m_savedChannelSpellID(0),
      m_channelSpellEffect(0),
      m_shapeShiftPoof(0),
      m_fishingLineObject(0) {
  memset(&m_combat, 0, sizeof(m_combat));
  m_handAnim[COMBAT_MAINHAND] = RESET_ANIMATION_INDICES0;
  m_handAnim[COMBAT_OFFHAND] = RESET_ANIMATION_INDICES0;
  SetClientInitData(eventTime, *init, 0);
  memset(m_auraVisual, 0, sizeof(m_auraVisual));
  memset(m_attachments, 0, sizeof(m_attachments));
  memset(m_deferredAttachments, 0, sizeof(m_deferredAttachments));
  AddWorldObject();
  RefreshDataPointers();
  if (GetType() == HIER_TYPE_UNIT) {
    m_stats = const_cast<CreatureStats_C *>(
        g_creatureDBCache.GetRecord(m_obj->m_entryID, GetGUID(), CreatureQueryCallback, 0));
    m_NPCSoundsRec = g_nPCSoundsDB.GetRecord(GetSoundData()->m_NPCSoundID);
    m_numNPCPissedSounds = m_NPCSoundsRec ? SndInterfaceGetSoundVariations(m_NPCSoundsRec->m_SoundID[2]) : 0;
    InitializeExtendedDisplay();
  }
  if (static_cast<float>(m_unit->health) / static_cast<float>(m_unit->maxHealth) < 0.2f && m_unit->health > 0) {
    AddBloodPool();
  }
  MarkFootstepAnimations(m_model);
  InitializeTextureVariations(m_displayInfo, m_model, m_modelData);
  m_displayHealth = m_unit->health;
  memset(m_auraFlags, 0, sizeof(m_auraFlags));
  m_animatingAura = -1;
  memset(m_weaponTrails, 0, sizeof(m_weaponTrails));
  memset(m_callbackList, 0, sizeof(m_callbackList));
}

void CGUnit_C::InitializeExtendedDisplay() {
  if (!IsModelComponentable()) {
    return;
  }

  HMODEL charModel = GetCharacterModel(0);
  FATALASSERT(charModel);
  unsigned int   race = GetDisplayRace();
  unsigned int   sex = GetDisplaySex();
  const char    *displayTextureName = GetDisplayTextureName();
  HTEXTURE       texture;
  HTEXCOMPONENT &texComponent = m_texComponent;

  if (*displayTextureName) {
    char preBakeName[256];
    SStrPrintf(preBakeName, sizeof(preBakeName), "%s%s", "Textures\\BakedNpcTextures\\", displayTextureName);
    texture = CharCustomizationLoadSkin(charModel, preBakeName, race, sex, SkinVariationID(), 1);
    if (texComponent) {
      HandleClose(texComponent);
    }
    texComponent = 0;
    CharCustomizationSetHairTexture(charModel, 0, race, sex, HairStyleID(), HairColorID());
  } else {
    texture = CharCustomizationSetSkin(charModel, race, sex, SkinVariationID(), 1);
    if (!texture) {
      FATALERROR(
          ("Error, skinID %d on character %s (race/sex is %d/%d)cannot be loaded, is it a missing file?", SkinVariationID(), GetUnitName(), race, sex)
      );
    }
    if (texComponent) {
      HandleClose(texComponent);
    }
    texComponent = TexComponentCreate(texture, race, sex, SkinVariationID(), 1, 0);
    FATALASSERT(texComponent);
    CharCustomizationSetFaceTexture(charModel, texComponent, race, sex, FaceID(), SkinVariationID(), 1);
    CharCustomizationSetHairTexture(charModel, texComponent, race, sex, HairStyleID(), HairColorID());
    CharCustomizationSetFacialTexture(charModel, texComponent, race, sex, FacialHairID(), HairColorID());
  }

  BEARDSTYLEDATA facialData;
  int            hasFacialInfo = CharCustomizationGetBeardStyle(race, sex, FacialHairID(), &facialData);

  HCHARGEOSET &geosetHandle = m_geosetHandle;
  if (geosetHandle) {
    HandleClose(geosetHandle);
  }
  geosetHandle = CharCustomizationCreateGeosetHandle(charModel);
  FATALASSERT(geosetHandle);
  InitPreferredGeosets();
  CharCustomizationInitBaseCharacter(
      geosetHandle, hasFacialInfo ? facialData.beardGeoset : 1, hasFacialInfo ? facialData.sideBurnGeoset : 1,
      hasFacialInfo ? facialData.moustacheGeoset : 1, 2
  );
  CharCustomizationResetHairGeoset(geosetHandle, race, sex, HairStyleID());

  if (texture) {
    HandleClose(texture);
  }
  HandleClose(charModel);
}

void CGUnit_C::PostInit(const CClientObjCreate &init) {
  PostSetClientInitData(init.move);
  m_fadingMountScale = GetMountScale();
  CGObject_C::PostInit(init);
  UnitInitializeModel(m_model);
  InitializeLoopSound();

  const ChrRacesRec *race = g_chrRacesDB.GetRecord(m_unit->race);
  if (race) {
    m_splashSoundID = race->m_SplashSoundID;
  }

  SetSheatheReason(SHEATHE_PLAYEREXPLICIT, m_unit->weaponMode == WEAPONMODE_SHEATHEDMODE, 1);
  SetupFootprints();
  AttachVirtualMonsterWeapons();
  InitializeNPCItems();
  if (m_geosetHandle) {
    CharCustomizationCommitItemGeosets(m_geosetHandle, 0);
    CGObject_C::Animate();
  }

  unsigned int flags = 0x100;
  if (m_unit->health > 0) {
    ClearResEffectModel();
  } else {
    m_animFlags |= 0x2000;
    m_deathTime = OsGetAsyncTimeMs();
    flags = 0x180;
    InitializeResEffectModel();
  }

  m_flags |= 4;
  UpdateUnitMountInfo(1, 0xFFFFFFFF);
  DetermineReadySequence(0);
  CGUnit_C::UpdateBaseAnimation(flags);
  ClearTorsoAnimation(0);
  UpdateUnitAlpha();
  RefreshInteractIcon();
  SetMirrorHandlers();
  SetSmoothFacing(GetFacing());

  if (m_unit->dynamicFlags & 1) {
    int effect = UnitEffectGetSpecialVisual(SPECIALEFFECT_LOOTART);
    if (effect >= 0) {
      MaybeAttachAura(UNITEFFECT_ATTACHBASE, effect, 0, 100, 1);
    }
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player && GetGUID() == player->GetFarsightFocus()) {
    player->SetFarSightFocus(this);
  }

  for (unsigned int slot = 0; slot < 56; ++slot) {
    if (m_unit->auras[slot]) {
      AddAuraEffect(slot, 0);
      RefreshAuraVisuals();
    }
  }
}

void CGUnit_C::PostMovementUpdate(const CClientMoveUpdate &update) {
  PostSetClientInitData(update);
  UpdateBaseAnimation(0);
}

void CGUnit_C::UpdateUnitCollisionBox(HMODEL model, const char *modelFileName) {
  NTempest::CAaBox extents(0.0f);
  ModelGetCollisionExtents(model, &extents);
  if (!static_cast<CMovement &>(m_move).SetCollisionBox(extents, GetScale())) {
    if (NTempest::CMath::fabs_(GetScale()) < 0.00000095367432f) {
      SysMsgPrintf(SYSMSG_ERROR, 2, "ZEROSCALEUNIT|%d|%s", GetEntryID(), GetUnitName());
    } else {
      SysMsgPrintf(SYSMSG_ERROR, 2, "NOCOLLISIONBOX|%s", modelFileName);
    }
  }
}

void CGUnit_C::SetupFootprints() {
  FATALASSERT(m_modelData);
  m_footprintTextureID = m_modelData->m_footprintTextureID;
  m_footprintSize.x = m_modelData->m_footprintTextureWidth / 36.0f;
  m_footprintSize.y = m_modelData->m_footprintTextureLength / 36.0f;
  m_footprintParticleScale = m_modelData->m_footprintParticleScale;
}

void CGUnit_C::InitializeTextureVariations(
    const CreatureDisplayInfoRec *displayInfo,
    HMODEL                        theModel,
    const CreatureModelDataRec   *modelData
) {
  char textureFile[MAX_PATH];
  char texturePath[MAX_PATH];

  if (!theModel || !modelData || (modelData->m_flags & 4)) {
    return;
  }

  SStrCopy(texturePath, modelData->m_ModelName, sizeof(texturePath));
  char *lastSlash = SStrChrR(texturePath, '\\');
  if (lastSlash) {
    lastSlash[1] = 0;
  }

  EGxTexFilter filter = GxTex_LinearMipNearest;
  unsigned int maxAnisotropy = 1;
  if (CWorld::enables & CWorld::Enable_Anisotropic) {
    filter = GxTex_Anisotropic;
    maxAnisotropy = 1 << CWorld::texMaxAnisotropyLog2;
  } else if (CWorld::enables & CWorld::Enable_Trilinear) {
    filter = GxTex_LinearMipLinear;
  }

  CGxTexFlags texFlags(filter, 1, 1, 0, 0, 0, maxAnisotropy);
  CStatus     status;

  for (unsigned int index = 0; index < 3; ++index) {
    const char *variation = displayInfo->m_textureVariation[index];
    if (!*variation) {
      continue;
    }

    SStrPrintf(textureFile, sizeof(textureFile), "%s%s", texturePath, variation);
    HTEXTURE newTexture = TextureCreate(textureFile, texFlags, &status, 0);
    ASSERT(newTexture);
    SysMsgAdd(status, 1);
    ModelReplaceTexture(theModel, s_materialIDs[index], newTexture, 0);
    HandleClose(newTexture);
  }
}

int CGUnit_C::IsWalking() const {
  return const_cast<CMovement &>(static_cast<const CMovement &>(m_move)).GetCurrentSpeed() <=
         m_move.m_walkSpeed + m_move.m_walkSpeed;
}

unsigned int CGUnit_C::GetAnimationState() {
  int inAttackMode = 0;
  CGUnitData *unit;

  if (m_unit->health <= 0 && ((m_animFlags & 0x2000) || !(m_flags & 4))) {
    SetTorsoAnimState(ANIM_STATE_NONE);
    return ANIM_STATE_DEAD;
  }

  if (IsA(ID_PLAYER)) {
    unsigned int playerState = static_cast<CGPlayer_C *>(this)->GetPlayerAnimState();
    if (playerState) {
      return playerState;
    }
  }

  unsigned int moveFlags = m_move.m_moveFlags;
  if (moveFlags & 0x8000) {
    return ANIM_STATE_FALLING;
  }
  if (m_move.m_jumpVelocity != 0.0f) {
    return ANIM_STATE_JUMPING;
  }
  if (moveFlags & 0x2000000) {
    if (moveFlags & 2) {
      return ANIM_STATE_SWIM_BACKWARDS;
    }
    if (moveFlags & 0xC) {
      return (moveFlags & 4) ? ANIM_STATE_SWIM_STRAFE_LEFT : ANIM_STATE_SWIM_STRAFE_RIGHT;
    }
    return (moveFlags & 0xF) ? ANIM_STATE_SWIM : ANIM_STATE_SWIM_IDLE;
  }
  if (moveFlags & 2) {
    if (moveFlags & 0xC) {
      return (moveFlags & 4) ? ANIM_STATE_DIAG_BACKWARDS_LEFT : ANIM_STATE_DIAG_BACKWARDS_RIGHT;
    }
    return ANIM_STATE_WALK_BACKWARDS;
  }
  if ((moveFlags & 0xF) == 0xF) {
    return IsWalking() ? ANIM_STATE_DIAG_WALK_LEFT : ANIM_STATE_DIAG_RUN_LEFT;
  }
  if (moveFlags & 4) {
    return IsWalking() ? ANIM_STATE_STRAFE_WALK_LEFT : ANIM_STATE_STRAFE_RUN_LEFT;
  }
  if (moveFlags & 8) {
    return IsWalking() ? ANIM_STATE_STRAFE_WALK_RIGHT : ANIM_STATE_STRAFE_RUN_RIGHT;
  }
  if ((moveFlags & 1) && static_cast<CMovement &>(m_move).GetCurrentSpeed() > 0.0f) {
    return IsWalking() ? ANIM_STATE_WALK : ANIM_STATE_RUN;
  }

  if (GetGUID() == ClntObjMgrGetActivePlayer()) {
    inAttackMode = static_cast<CGPlayer_C *>(this)->m_flags & 0x400;
  }
  if (inAttackMode && !m_unit->standState) {
    goto attack_ready;
  }
  if (!(m_unit->flags & 0x20000) && m_combat.IsAttacking()) {
    goto attack_ready;
  }
  if (m_currentDamageInfo) {
    goto attack_ready;
  }
  unit = m_unit;
  if (unit->weaponMode == WEAPONMODE_RANGEDMODE && m_rangedStandTimer) {
    goto attack_ready;
  }
  return s_standStateAnims[unit->standState];

attack_ready:
  return ANIM_STATE_ATTACK_READY;
}

void CGUnit_C::GenericAnimEndHandler(ANIMENUMERATION animID, void *param) {
  if (m_currentTorsoAnimState == ANIM_STATE_EMOTE && GetCurrentTorsoAnim() == animID) {
    ClearTorsoAnimation(0x40);
  } else if (g_seqInformation[animID].handler) {
    g_seqInformation[animID].handler(param, this);
  }
}

int IsSitStandSleepTransition(unsigned int animState);

int CGUnit_C::PlayBaseAnimation(int newAnimState, int newAnim, int forceNoFidget, bool &checkImpacts) {
  checkImpacts = false;
  int    flag = 0;
  HMODEL charModel = GetCharacterModel(0);
  FATALASSERT(charModel);

  int sequenceFlags = 0;
  if (s_animInfo[newAnimState].flags & 0x4000) {
    sequenceFlags = 8;
  }
  if (forceNoFidget) {
    sequenceFlags |= 1;
  }

  if (!GetCurrentTorsoAnimState() || (s_animInfo[newAnimState].flags & 8)) {
    CheckPendingMissileRelease(0);
    CheckPendingVictimFeedback();
    if (GetCurrentTorsoAnimState() == ANIM_STATE_EMOTE) {
      SetEmoteState(0);
    }

    switch (GetCurrentTorsoAnimState()) {
      case ANIM_STATE_DIAG_BACKWARDS_LEFT:
      case ANIM_STATE_DIAG_BACKWARDS_RIGHT:
      case ANIM_STATE_TURNING_LEFT:
      case ANIM_STATE_TURNING_RIGHT:
      case ANIM_STATE_EMOTE:
      case ANIM_STATE_SPECIALMOUNTANIM:
      case 85:
      case 86:
      case 87:
      case 88: {
        char buffer[256];
        SStrPrintf(
            buffer, sizeof(buffer), "Warning, %s attack state %d preempted by state %d!", GetUnitName(), GetCurrentTorsoAnimState(), newAnimState
        );
        if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__)) {
          DDGENLOG(GetGUID(), buffer, __FILE__, __LINE__);
        }
        break;
      }
    }

    flag = ObjectModelSetSequence(charModel, newAnim, GetObjAnimFlags(sequenceFlags | 2), 0);
    if (GetCurrentTorsoAnimState()) {
      StoreSequenceEndCallbacks(GetCurrentTorsoAnim());
    }
    if (newAnimState == ANIM_STATE_JUMPING) {
      unsigned int fallTime = static_cast<CMovement &>(m_move).FallTime();
      ModelForceSequenceTime(charModel, newAnim, fallTime, 0);
      if (fallTime < 300) {
        PlayUnitSound(UNITSOUNDTYPE_JUMPSTART, 1);
      }
    }
  } else {
    if (s_animInfo[GetCurrentBaseAnimState()].flags & 8) {
      SetTorsoAnimation(GetCurrentTorsoAnimState(), 0, 0x40);
    }
    if (newAnimState == ANIM_STATE_IDLE && (s_animInfo[GetCurrentTorsoAnimState()].flags & 2)) {
      SetBaseAnim(GetCurrentTorsoAnim());
      sequenceFlags |= 4;
    }

    if (ModelLockObjectSequence(charModel, 4, 1)) {
      flag = ObjectModelSetSequence(charModel, newAnim, GetObjAnimFlags(sequenceFlags), 0);
      ModelLockObjectSequence(charModel, 4, 0);
      checkImpacts = true;
    } else if (s_animInfo[newAnimState].basePriority > s_animInfo[GetCurrentTorsoAnimState()].basePriority) {
      flag = ObjectModelSetSequence(charModel, newAnim, GetObjAnimFlags(sequenceFlags | 2), 0);
      CheckPendingImpactKit();
    }
  }

  UpdateMovementAnimSpeed(0, newAnimState);
  HandleClose(charModel);
  return flag;
}

void CGUnit_C::SetStrafeRotation() {
  unsigned int       moveFlags = m_move.m_moveFlags;
  NTempest::C3Vector waistTurn;
  NTempest::C3Vector spineTurn;

  if (moveFlags & 3) {
    waistTurn = NTempest::C3Vector(0.9238795f, 0.38268343f, 0.0f);
    spineTurn = NTempest::C3Vector(0.98078525f, 0.19509032f, 0.0f);
  } else {
    waistTurn = NTempest::C3Vector(0.70710677f, 0.70710677f, 0.0f);
    spineTurn = NTempest::C3Vector(0.9238795f, 0.38268343f, 0.0f);
  }

  if ((moveFlags & 2) == ((moveFlags >> 1) & 2)) {
    waistTurn.y = -waistTurn.y;
    spineTurn.y = -spineTurn.y;
  }

  HMODEL model = m_model;
  if (TorsoAnimOverridesBase()) {
    ModelRemoveObjectFaceDir(model, 5);
  } else {
    ModelApplyObjectFaceDir(model, 5, waistTurn);
  }
  ModelApplyObjectFaceDir(model, 4, spineTurn);
  spineTurn.y = -spineTurn.y;
  ModelApplyObjectFaceDir(model, 6, spineTurn);
}

void CGUnit_C::ApplyStrafeRotation(unsigned int newState) {
  if (newState >= 8 && newState <= 17) {
    SetStrafeRotation();
  } else if (GetCurrentBaseAnimState() >= 8 && GetCurrentBaseAnimState() <= 17) {
    HMODEL model = m_model;
    ModelRemoveObjectFaceDir(model, 5);
    ModelRemoveObjectFaceDir(model, 4);
    ModelRemoveObjectFaceDir(model, 6);
  }
}

bool CGUnit_C::BaseAnimLocksHead() const {
  if (s_animInfo[m_currentBaseAnimState].flags & 1) {
    return true;
  }
  return (g_seqInformation[m_currentBaseAnim].flags & 2) != 0;
}

bool CGUnit_C::TorsoAnimLocksHead() const {
  return (s_animInfo[m_currentTorsoAnimState].flags & 1) != 0;
}

bool CGUnit_C::TorsoAnimOverridesBase() const {
  return (s_animInfo[m_currentTorsoAnimState].flags & 0x8000) || (g_seqInformation[GetCurrentTorsoAnim()].flags & 8);
}

void CGUnit_C::UpdateMountAnimation(unsigned int newState, unsigned int flags) {
  FATALASSERT(newState < NUM_ANIMSTATES);

  if ((m_flags & 0x10) && (m_currentMountAnimState != newState || (flags & 0x100))) {
    m_currentMountAnimState = newState;
    unsigned int newAnim = ChooseAnimation(newState);
    HMODEL       model = m_model;
    ObjectModelSetSequence(model, newAnim, GetObjAnimFlags(2), 0);
    UpdateMovementAnimSpeed(1, INVALID_ANIM_STATE);

    if (((flags & 0x100) && (s_animInfo[newState].flags & 0x400)) ||
        (s_animInfo[newState].flags & 0x40) ||
        ((s_animInfo[newState].flags & 0x800) && !(m_animFlags & 4)))
    {
      ModelForceSequenceTime(model, newAnim, 0x7FFFFFFF, 0);
    }
  }
}

int CGUnit_C::SetTorsoSequence(float timeScale, int flags) {
  CheckPendingThrownWeaponReattach(0);
  CheckPendingVictimFeedback();
  CheckPendingMissileRelease(0);
  CheckPendingImpactKit();

  HMODEL theModel = GetCharacterModel(0);
  FATALASSERT(theModel);
  if (IsPreemptableWoundAnimState(m_currentTorsoAnimState)) {
    m_currentWoundStartTime = OsGetAsyncTimeMs();
    unsigned int anim = GetCurrentTorsoAnim();
    if (!ModelGetSequenceDuration(theModel, anim, &m_currentWoundAnimDuration)) {
      goto sequence_failed;
    }
    FATALASSERT(m_currentWoundAnimDuration);
  }

  if (!(s_animInfo[m_currentBaseAnimState].flags & 4) || TorsoAnimOverridesBase()) {
    if (TorsoAnimOverridesBase()) {
      ModelRemoveObjectFaceDir(m_model, 5);
    }
    if (ObjectModelSetSequence(theModel, GetCurrentTorsoAnim(), GetObjAnimFlags(flags | 2), 0)) {
      ModelSetTimeScale(theModel, timeScale, 0);
      HandleClose(theModel);
      return 1;
    }
  } else if (ObjectModelSetBoneSequence(theModel, GetCurrentTorsoAnim(), 4, flags)) {
    ModelSetObjectTimeScale(theModel, 4, timeScale, 0);
    HandleClose(theModel);
    return 1;
  }

sequence_failed:
  HandleClose(theModel);
  SetTorsoAnimState(0);
  return 0;
}

float CGUnit_C::GetAnimTimeScale(unsigned int sequence, unsigned int duration, unsigned int flags) {
  HMODEL theModel = GetCharacterModel(0);
  FATALASSERT(theModel);

  float        timeScale = 1.0f;
  unsigned int seqInfoDuration;
  if (ModelGetSequenceDuration(theModel, sequence, &seqInfoDuration)) {
    m_animBaseDuration = seqInfoDuration;
    m_animStartTime = OsGetAsyncTimeMsPrecise();
    m_animEndTime = duration + m_animStartTime;
    if (seqInfoDuration && duration && seqInfoDuration != duration && !(flags & 0x20) && (!(flags & 0x10) || seqInfoDuration > duration)) {
      timeScale = static_cast<float>(seqInfoDuration) / static_cast<float>(duration);
    }
  }

  HandleClose(theModel);
  return timeScale;
}

void CGUnit_C::StoreSequenceEndCallbacks(int anim) {
  ANIMENDDATA *data = m_animEndCallbackList.New();
  data->unit = GetGUID();
  data->animID = static_cast<ANIMENUMERATION>(anim);
}

void CGUnit_C::ProcessAnimEndCallbacks() {
  unsigned int count = m_animEndCallbackList.Count();
  while (count) {
    --count;
    ANIMENDDATA &data = m_animEndCallbackList[count];
    GenericAnimEndHandler(data.animID, &data);
  }
  m_animEndCallbackList.SetCount(0);
}

int CGUnit_C::SetTorsoAnimation(unsigned int state, unsigned long duration, unsigned int flags) {
  FATALASSERT(state < NUM_ANIMSTATES);

  if (!state) {
    if (!m_currentTorsoAnimState) {
      return 1;
    }

    if (!m_castingSpell || GetSpellCastingTime(m_castingSpell) <= 0) {
      CheckLevelUpAnimFlag(m_currentTorsoAnimState, 0);
      SetTorsoAnim(GetStandStateAnim(0));
      SetTorsoAnimState(0);
      m_animEndTime = 0;
      UpdateBaseAnimation(0);
      SetTorsoAnim(ChooseAnimation(m_currentBaseAnimState));

      if (GetCurrentTorsoAnim() == ANIM_STEALTHSTAND ||
          (!GetCurrentTorsoAnim() && (m_combat.IsAttacking() || !m_unit->health)))
      {
        return SetTorsoSequence(
            GetAnimTimeScale(GetCurrentTorsoAnim(), duration, flags | 1),
            flags | 1
        );
      }

      return SetTorsoSequence(GetAnimTimeScale(GetCurrentTorsoAnim(), duration, flags), flags);
    }

    state = 37;
  }

  CheckLevelUpAnimFlag(m_currentTorsoAnimState, state);
  if (g_seqInformation[m_currentBaseAnim].flags & 4) {
    return 0;
  }
  if (m_currentTorsoAnimState == state && !(s_animInfo[state].flags & 0x20)) {
    return 0;
  }
  if (!(s_animInfo[m_currentBaseAnimState].flags & 0x80) && (s_animInfo[m_currentBaseAnimState].flags & 8) && !(flags & 0x40)) {
    return 0;
  }
  if (m_currentTorsoAnimState &&
      s_animInfo[m_currentTorsoAnimState].basePriority > s_animInfo[state].basePriority + s_animInfo[state].priorityOffset &&
      !(flags & 0x40)) {
    return 0;
  }

  unsigned int anim = ChooseAnimation(state);
  if (anim == INVALID_ANIMATION) {
    return 1;
  }
  if (anim == ANIM_HOLDBOW) {
    SetRangedWeaponPullAnim(0);
  }
  SetTorsoAnim(anim);
  SetTorsoAnimState(state);
  return SetTorsoSequence(GetAnimTimeScale(GetCurrentTorsoAnim(), duration, flags), flags);
}

int CGUnit_C::ClearTorsoAnimation(unsigned int flags) {
  int           anim;

  if (IsSpellAuraAnimActive(anim)) {
    m_spellCastingAnim = static_cast<ANIMENUMERATION>(anim);
    SetTorsoAnimation(63, 0, flags);
  } else if (IsSpellChannelAnimActive(anim)) {
    m_spellCastingAnim = static_cast<ANIMENUMERATION>(anim);
    SetTorsoAnimation(62, 0, flags);
  } else if (m_unit->channelSpell) {
    PlayEmoteAnimation(m_unit->channelSpell, flags);
  } else {
    if (m_emoteQueue.Count() && m_currentTorsoAnimState == 46) {
      m_emoteQueue[m_emoteQueue.Count() - 1].delay += OsGetAsyncTimeMs();
    }
    SetTorsoAnimation(0, 0, flags);
  }

  return 1;
}

void CGUnit_C::SheatheAnimEndHandler() {
  HMODEL theModel = GetCharacterModel(0);
  FATALASSERT(theModel);
  ModelLockObjectSequence(theModel, 3, 0);
  ModelLockObjectSequence(theModel, 2, 0);
  ModelMatchSequence(theModel, 3, 4, 6);
  ModelMatchSequence(theModel, 2, 4, 6);
  HandleClose(theModel);

  m_handAnim[COMBAT_MAINHAND] = RESET_ANIMATION_INDICES0;
  m_handAnim[COMBAT_OFFHAND] = RESET_ANIMATION_INDICES0;
  if (!(m_animFlags & 0x10000)) {
    UpdateSheatheRangedReasons(1);
  }
}

void CGUnit_C::UpdateMoveInfo(unsigned long eventTime, const CClientMoveUpdate &update) {
  static_cast<CMovement &>(m_move).SetUpdateInfo(eventTime, update, GetGUID() == m_activeMover);
}

void CGUnit_C::SetClientInitData(unsigned long eventTime, const CClientObjCreate &init, bool partialUpdateOfActivePlayer) {
  m_combat.SetClientInitData(init);
  if (!partialUpdateOfActivePlayer) {
    UpdateMoveInfo(eventTime, init.move);
  }
}

void CGUnit_C::PostSetClientInitData(const CClientMoveUpdate &update) {
  static_cast<CMovement &>(m_move).UpdateTransportStatus(update.status);
}

void CGUnit_C::SetAuraMirrorHandlers() {
  for (unsigned int slot = 0; slot < 56; ++slot) {
    SetAuraMirrorHandler(slot, AuraMirrorHandler);
  }
}

void CGUnit_C::LookAtTarget(CGUnit_C *target) {
  FATALASSERT(target);

  float              facing = GetRenderFacing();
  NTempest::C3Vector facingDirection(cos(facing), sin(facing), 0.0f);
  NTempest::C3Vector targetPosition;
  HMODEL             targetModel = target->GetCharacterModel(0);
  int                targetFound = ModelGetObjectPosition(targetModel, 21, &targetPosition);
  HandleClose(targetModel);
  if (!targetFound) {
    target->ReportMissingAttachment(21, 0);
    return;
  }

  NTempest::C3Vector position;
  HMODEL             charModel = GetCharacterModel(0);
  int                positionFound = ModelGetObjectPosition(charModel, 6, &position);
  HandleClose(charModel);
  if (!positionFound) {
    ReportMissingBone(21, 0);
    return;
  }

  NTempest::C3Vector toTarget = targetPosition - position;
  if (fabs(toTarget.SquaredMag()) >= 0.00000023841858f) {
    toTarget.Normalize();
    if (NTempest::C3Vector::Dot(toTarget, facingDirection) <= 0.76604003f) {
      if (m_animFlags & 0x1000) {
        RemoveObjectLookAt();
      }
    } else {
      ApplyObjectCameraSpaceLookAt(targetPosition);
    }
  }
}

void CGUnit_C::UnsetAuraMirrorHandlers() {
  for (unsigned int slot = 0; slot < 56; ++slot) {
    UnsetAuraMirrorHandler(slot, AuraMirrorHandler);
  }
}

void CGUnit_C::LookAtTarget() {
  unsigned __int64 targetGUID;
  if (GetGUID() == ClntObjMgrGetActivePlayer()) {
    targetGUID = static_cast<const CGPlayer_C *>(this)->CGPlayer_C::GetLocalTarget();
  } else {
    targetGUID = m_unit->target;
  }

  if (!targetGUID || BaseAnimLocksHead()) {
    if (GetCurrentBaseAnimState() >= 8 && GetCurrentBaseAnimState() <= 17) {
      SetStrafeRotation();
    }
    return;
  }

  CGUnit_C *target = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(targetGUID, __FILE__, __LINE__));
  if (target && !m_combat.IsAttacking()) {
    LookAtTarget(target);
  }
}

void CGUnit_C::UpdateLookAtTarget() {
  unsigned __int64 targetGUID;
  if (GetGUID() == ClntObjMgrGetActivePlayer()) {
    targetGUID = static_cast<const CGPlayer_C *>(this)->CGPlayer_C::GetLocalTarget();
  } else {
    targetGUID = m_unit->target;
  }

  CGUnit_C *target = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(targetGUID, __FILE__, __LINE__));
  if (TorsoAnimLocksHead() || m_combat.IsAttacking()) {
    RemoveObjectLookAt();
  } else if (target && !BaseAnimLocksHead()) {
    LookAtTarget(target);
  } else if (m_animFlags & 0x1000) {
    RemoveObjectLookAt();
  }
}

int OnAuraDecayFinished(void *param) {
  AuraDecayNode *decay = static_cast<AuraDecayNode *>(param);
  CGObject_C    *object = ClntObjMgrObjectPtr(decay->unit, __FILE__, __LINE__);

  if (object && !decay->visual.IsWorldModel()) {
    GEOCOMPONENTLINKS linkPoint = UnitEffectGetLinkPointFromAttachment(decay->attach);
    FATALASSERT(linkPoint != ATTACH_NONE);

    CGUnit_C *unit = static_cast<CGUnit_C *>(object);
    HMODEL    charModel = unit->GetCharacterModel(0);
    FATALASSERT(charModel);
    HMODEL model = decay->visual.GetModel();
    if (model && !ModelRemoveLink(charModel, linkPoint, model)) {
      SysMsgPrintf(SYSMSG_WARNING, 16, "UNITMISSINGCONNECTION|%d|%d", unit->GetUnitData()->displayID, linkPoint);
    }
    HandleClose(charModel);
  }

  decay->Unlink();
  s_auraDecayFreeList.Put(decay);
  return 0;
}

void CGUnit_C::UpdateBaseAnimation(unsigned int flags) {
  if (IsMounted()) {
    CGUnit_C::UpdateBaseAnimation(ANIM_STATE_MOUNTED, flags);
  }
  CGUnit_C::UpdateBaseAnimation(GetAnimationState(), flags);
}

void CGUnit_C::SetBaseAnimState(unsigned int newState) {
  FATALASSERT(newState < NUM_ANIMSTATES);
  m_currentBaseAnimState = newState;
}

void CGUnit_C::SetEmoteState(unsigned int emoteID) {
}

void CGUnit_C::OnPickNextStandHandler() {
  if (m_animFlags & 1) {
    m_animFlags &= ~1U;
  }
  ForceUpdateBaseAnimation();
}

void CGUnit_C::LootAnimEndHandler() {
  if (!(m_unit->flags & 0x400)) {
    CGUnit_C::UpdateBaseAnimation(0);
  }
}

void CGUnit_C::OnDeath() {
  m_lastDeathTime = OsGetAsyncTimeMs();
  m_combat.m_victim = 0;
  m_combat.m_attackSent = 0;
  m_combat.m_stopSent = 0;
  if (m_castingSpell) {
    StopSpellFizzleTimer(m_castingSpell, 2);
  }
  if (Spell_C_IsModal() && Spell_C_GetCurrentTarget() == GetGUID()) {
    Spell_C_CancelSpell(1, 1, SPELL_FAILED_TARGETS_DEAD);
  }
  ClearTrackingTarget(0);
  UnitCombatLogUnitDead(GetGUID());
}

void CGUnit_C::RestoreUnit() {
  m_animFlags &= ~0x2000U;
  FATALASSERT(m_unit->health > 0);
  m_deathHolds = 0;
  PurgeAnimNodes(0);
  ClearResEffectModel();
  CGUnit_C::UpdateBaseAnimation(0);
  if (GetGUID() == ClntObjMgrGetActivePlayer()) {
    CGGameUI::UnlockAllItems();
    FrameScript_SignalEvent(0xFF);
  }
}

void CGUnit_C::SetTorsoAnimState(unsigned int newState) {
  FATALASSERT(newState < NUM_ANIMSTATES);
  if ((m_flags & 0x400) && newState != ANIM_STATE_SPELLPRECAST) {
    ShowHandArrow(0);
    m_flags &= ~0x400U;
  }
  unsigned int oldState = m_currentTorsoAnimState;
  if (oldState != newState) {
    ClearMeleeDeathHold();
    if ((s_animInfo[newState].flags ^ s_animInfo[oldState].flags) & 0x80) {
      ApplyStrafeRotation(m_currentBaseAnimState);
    }
    if (oldState == ANIM_STATE_SPELLCAST) {
      CheckPendingSpellAnimHits();
      m_deferredPrecastAnim = RESET_ANIMATION_INDICES0;
      SetSheatheReason(SHEATHE_SPELLS, 0, 0);
    } else if (oldState == ANIM_STATE_EMOTE) {
      SetSheatheReason(SHEATHE_TALKEMOTE, 0, 1);
    }
  }
  if (newState == ANIM_STATE_SPELLCAST) {
    m_lastSpellCastAnimTime = OsGetAsyncTimeMs();
  }
  m_currentTorsoAnimState = newState;
}

int CGUnit_C::GetSpellCastingTime(int spellID) const {
  const SpellRec *spell = g_spellDB.GetRecord(spellID);
  if (!spell) {
    return 0;
  }
  const SpellCastTimesRec *castTime = g_spellCastTimesDB.GetRecord(spell->m_castingTimeIndex);
  if (!castTime) {
    return 0;
  }
  int result = castTime->m_base + castTime->m_perLevel * (const_cast<CGUnit_C *>(this)->GetSpellRank(spellID) / 5);
  if (result < castTime->m_minimum) {
    result = castTime->m_minimum;
  }
  if (result > 0 && m_unit->modCastingSpeed) {
    result += result * m_unit->modCastingSpeed / 100;
  }
  if (spell->m_attributes & 2) {
    result = 0x7FFFFFFF;
  }
  return result > 0 ? result : 0;
}

int CGUnit_C::GetVirtualItemDisplayID(unsigned int slot) const {
  return m_unit->virtualItemDisplay[slot];
}

const VirtualItemInfo *CGUnit_C::GetVirtualItem(unsigned int slot, bool ignoreDisarmFlag) const {
  ASSERT(!IsA(TYPE_PLAYER));
  if (!GetVirtualItemDisplayID(slot)) {
    return 0;
  }
  const VirtualItemInfo *item = &m_unit->virtualItemInfo[slot];
  if (!ignoreDisarmFlag && (m_unit->flags & 0x200000) && item->m_classID == 2 && !slot) {
    return 0;
  }
  return item;
}

int CGUnit_C::ShouldRenderUnitName(unsigned int mode) const {
  if ((m_unit->flags & 0x18000) && CGGameUI::GetLockedTarget() != m_obj->m_guid) {
    return 0;
  }
  switch (mode) {
    case 1:
    case 2:
      return CGGameUI::GetLockedTarget() == m_obj->m_guid;
    case 3:
      return 1;
    default:
      return 0;
  }
}

void CGUnit_C::SetLastWeaponModeSent(int mode) {
}

void CGUnit_C::InstallSeqEndHandler(HMODEL model, unsigned int animID) {
  if (animID >= 135) {
    return;
  }
  if (!m_callbackList[animID]) {
    m_callbackList[animID] = s_animEndDataPool.Get(0);
    m_callbackList[animID]->unit = GetGUID();
    m_callbackList[animID]->animID = static_cast<ANIMENUMERATION>(animID);
  } else {
    FATALASSERT(m_callbackList[animID]->unit == GetGUID());
    FATALASSERT(m_callbackList[animID]->animID == ANIMENUMERATION(animID));
  }
  ModelSetSeqFinishedHandler(model, animID, ::GenericAnimEndHandler, m_callbackList[animID]);
}

void CGUnit_C::ClearAnimCallbackData() {
  for (unsigned int animID = 0; animID < NUM_OBJECTANIMATIONS; ++animID) {
    if (m_callbackList[animID]) {
      s_animEndDataPool.Put(m_callbackList[animID]);
      m_callbackList[animID] = 0;
    }
  }
}

void CGUnit_C::UnitInitializeModel(HMODEL model) {
  FATALASSERT(model);
  ModelSetEventCallback(model, AnimEventCallback, this, 0);
  unsigned int index = g_unitSeqEndList.Count();
  while (index) {
    --index;
    InstallSeqEndHandler(model, g_unitSeqEndList[index]);
  }
}

void CGUnit_C::UnitUninitializeModel(HMODEL model) {
  if (!model) {
    return;
  }
  ModelSetEventCallback(model, 0, 0, 0);
  unsigned int index = g_unitSeqEndList.Count();
  while (index) {
    --index;
    ModelSetSeqFinishedHandler(model, index, 0, 0);
  }
}

void CGUnit_C::ShutdownWorldName() {
  if (m_unitNameHandle) {
    HandleClose(reinterpret_cast<HOBJECT>(m_unitNameHandle));
  }
  m_unitNameHandle = 0;
}

void CGUnit_C::MarkFootstepAnimations(HMODEL model) {
  ModelMarkFootstepSequence(model, ANIM_WALK);
  ModelMarkFootstepSequence(model, ANIM_RUN);
  ModelMarkFootstepSequence(model, ANIM_RUN_LEANRIGHT);
  ModelMarkFootstepSequence(model, ANIM_RUN_LEANLEFT);
}

void CGUnit_C::UpdateUnitAlpha() {
  unsigned char alpha = m_displayInfo ? static_cast<unsigned char>(m_displayInfo->m_creatureModelAlpha) : 255;
  if (m_unit->flags & 0x18000) {
    alpha /= 3;
  }
  DoFade(alpha, 1000);
}

void CGUnit_C::RefreshAttachmentInfo(HMODEL model) {
  bool sheathed = m_unit->weaponMode == WEAPONMODE_SHEATHEDMODE;
  for (unsigned int slot = 0; slot < 5; ++slot) {
    if (m_attachments[slot]) {
      ApplyAttachmentInfo(model, sheathed, slot, true);
    }
  }
}

void CGUnit_C::CleanupUnitArtwork(int playerModelChanged, int wasPlayerModel) {
  if (SheatheAnimPlaying() && !(m_animFlags & 0x10000)) {
    HandleSheatheAnimEvent(1, 1);
  }
  CheckPendingVictimFeedback();
  CheckPendingMissileRelease(0);
  UnitEffectClear(this);
  if (m_texComponent) {
    HandleClose(m_texComponent);
    m_texComponent = 0;
  }
  if (m_geosetHandle) {
    HandleClose(m_geosetHandle);
    m_geosetHandle = 0;
  }

  bool mountShowing = (m_flags & 0x10) != 0;
  HMODEL model = mountShowing ? GetCharacterModel(0) : m_model;
  if (!model) {
    return;
  }
  ModelSetEventCallback(model, 0, 0, 0);
  UnitUninitializeModel(model);
  ShutdownWorldName();
  if (!mountShowing) {
    RemoveWorldObject();
  }
  SetTorsoAnimState(0);
  SetBaseAnimState(3);
  if (mountShowing) {
    if (m_model) {
      ModelClearLink(m_model, 0);
    }
  } else {
    SetObjectModel(0);
  }
  HandleClose(model);
  KillCreatureLoopSound();
}

void CGUnit_C::ReinitializeUnitArtwork() {
  FATALASSERT(m_displayInfo);
  FATALASSERT(m_modelData);
  FATALASSERT(m_soundData);
  FATALASSERT(m_bloodRec);

  bool mountShowing = (m_flags & 0x10) != 0;
  if (mountShowing) {
    FATALASSERT(m_worldObject);
  } else {
    if (m_model) {
      HandleClose(m_model);
    }
    SetObjectModel(0);
  }

  char modelFileName[260] = "";
  if (!InitModelFileName(modelFileName, sizeof(modelFileName))) {
    return;
  }

  HMODEL model = ObjectModelCreate(modelFileName, static_cast<OBJECT_TYPE>(GetType()), 0x100800);
  MarkFootstepAnimations(model);
  UpdateObjectHeight(model);
  UnitInitializeModel(model);
  if (mountShowing) {
    FATALASSERT(ModelHasLinkPoint(m_model, 0));
    ModelAddLink(m_model, 0, model, 1.0f);
  } else {
    SetObjectModel(model);
  }

  InitializeTextureVariations(m_displayInfo, model, m_modelData);
  SetupFootprints();
  CGUnit_C::UpdateBaseAnimation(0x100);
  UpdateUnitAlpha();
  if (mountShowing) {
    HandleClose(model);
  }

  HMODEL characterModel = GetCharacterModel(0);
  FATALASSERT(characterModel);
  for (unsigned int index = 0; index < 12; ++index) {
    AuraVisual &visual = m_auraVisual[index];
    if (visual.HasArt() && !visual.IsWorldModel()) {
      HMODEL auraModel = visual.GetModel();
      if (auraModel) {
        GEOCOMPONENTLINKS linkPoint = UnitEffectGetLinkPointFromAttachment(static_cast<UNITEFFECTATTACHPPOINT>(index));
        if (!ModelAddLink(characterModel, linkPoint, auraModel, 1.0f)) {
          visual.Clear();
        }
      }
    }
  }
  if (ModelIsLoaded(characterModel, 1)) {
    m_flags &= ~0x40U;
    RefreshAttachmentInfo(characterModel);
  } else {
    m_flags |= 0x40U;
  }
  HandleClose(characterModel);
  InitializeLoopSound();
}

void CGUnit_C::PostReinitializeArtwork() {
  CGGameUI::UnitPortraitUpdate(GetGUID());
}

void CGUnit_C::ClearMountAnimState() {
  m_currentMountAnimState = 0;
}

void CGUnit_C::ClearTempCharModel() {
  if (m_tempCharModel) {
    HandleClose(m_tempCharModel);
  }
  m_tempCharModel = 0;
}

void CGUnit_C::SetTempCharModel(HMODEL model) {
  if (m_tempCharModel) {
    HandleClose(m_tempCharModel);
  }
  m_tempCharModel = model;
}

int CGUnit_C::UpdateAttachmentLoadStatus() {
  int result = CGObject_C::UpdateAttachmentLoadStatus();
  if (result) {
    ReinitializeWeaponTrails();
    CGGameUI::UnitPortraitUpdate(GetGUID());
    return 1;
  }
  return result;
}

void CGUnit_C::FinishAuraDecays() {
  ITERATELIST(AuraDecayNode, s_activeAuraDecays, decay) {
    if (decay->unit == GetGUID()) {
      OnAuraDecayFinished(decay);
    }
  }
}

bool CGUnit_C::IsSpellAuraAnimActive(int &anim) const {
  int slot = m_animatingAura;
  if (slot == -1) {
    return 0;
  }

  const SpellRec          *spell = g_spellDB.GetRecord(m_unit->auras[slot]);
  const SpellVisualRec    *visual = spell ? g_spellVisualDB.GetRecord(spell->m_spellVisualID) : 0;
  const SpellVisualKitRec *kit = visual ? g_spellVisualKitDB.GetRecord(visual->m_stateKit) : 0;
  if (!kit) {
    return 0;
  }

  anim = kit->m_anim;
  return anim > 0;
}

void CGUnit_C::UpdateBaseAnimation(unsigned int newState, unsigned int flags) {
  FATALASSERT(newState < NUM_ANIMSTATES);
  if (!newState) {
    return;
  }

  if (TorsoAnimOverridesBase() && !(s_animInfo[newState].flags & 8)) {
    SetBaseAnimState(newState);
    return;
  }

  if (IsMounted() && (s_animInfo[newState].flags & 0x2000)) {
    UpdateMountAnimation(newState, flags);
    return;
  }

  if (!m_unit->health && !m_deathHolds && newState != ANIM_STATE_DEAD) {
    return;
  }

  unsigned int newAnim = ChooseAnimation(newState);
  ApplyStrafeRotation(newState);
  if (!(flags & 0x100) && GetCurrentBaseAnimState() == newState && GetCurrentBaseAnim() == newAnim) {
    return;
  }

  FATALASSERT(
      (GetCurrentBaseAnimState() != ANIM_STATE_DEAD) ||
      (GetCurrentTorsoAnimState() != ANIM_STATE_WOUND)
  );
  if (m_flags & 4) {
    CheckPendingThrownWeaponReattach(0);
  }

  int  forceNoFidget = newAnim == ANIM_STEALTHSTAND || (!newAnim && m_combat.IsAttacking());
  bool checkImpacts;
  if (PlayBaseAnimation(newState, newAnim, forceNoFidget, checkImpacts)) {
    SetBaseAnim(newAnim);
    SetBaseAnimState(newState);
    LookAtTarget();
    if (checkImpacts) {
      CheckPendingImpactKit();
    }

    if ((flags & 0x80) ||
        ((flags & 0x100) && (s_animInfo[GetCurrentBaseAnimState()].flags & 0x400)) ||
        (s_animInfo[GetCurrentBaseAnimState()].flags & 0x40) ||
        ((s_animInfo[GetCurrentBaseAnimState()].flags & 0x800) && !(m_animFlags & 4)))
    {
      ModelForceSequenceTime(m_model, newAnim, 0x7FFFFFFF, 0);
    }
  }
}

bool CGUnit_C::IsSpellChannelAnimActive(int &anim) const {
  const SpellRec          *spell = g_spellDB.GetRecord(m_unit->channelSpell);
  const SpellVisualRec    *visual = spell ? g_spellVisualDB.GetRecord(spell->m_spellVisualID) : 0;
  const SpellVisualKitRec *kit = visual ? g_spellVisualKitDB.GetRecord(visual->m_channelKit) : 0;
  if (!kit) {
    return 0;
  }

  anim = kit->m_anim;
  return anim > 0;
}

ACTIVEAURAINFO *CGUnit_C::FindActiveAuraInfo(int slot) {
  ACTIVEAURAINFO *active = m_activeAuraInfo.Head();
  while (active && active->auraSlot != slot) {
    active = active->Next();
  }
  return active;
}

void CGUnit_C::RefreshAuraVisuals() {
  const SpellVisualKitRec *highestPrioritiesByKit[12];
  const SpellRec          *highestPrioritySpellFoundByKit[12];
  ACTIVEAURAINFO    *highestSpellWithAnim = 0;
  ACTIVEAURAINFO    *highestPriorityAura = 0;

  memset(highestPrioritiesByKit, 0, sizeof(highestPrioritiesByKit));
  memset(highestPrioritySpellFoundByKit, 0, sizeof(highestPrioritySpellFoundByKit));

  ITERATELIST(ACTIVEAURAINFO, m_activeAuraInfo, curr) {
    FATALASSERT(curr->stateKitRec);

    const SpellRec *spellRec = g_spellDB.GetRecord(m_unit->auras[curr->auraSlot]);
    if (!spellRec) {
      continue;
    }

    const SpellVisualKitRec *kitRec = curr->stateKitRec;
    if (spellRec->m_spellPriority > -1) {
      highestPriorityAura = curr;
      if (kitRec->m_anim != -1) {
        highestSpellWithAnim = curr;
      }
    }

#define CHECK_HIGHEST_AURA(attach, field)                                                                                               \
  if (kitRec->field > 0 &&                                                                                                              \
      (!highestPrioritySpellFoundByKit[attach] || spellRec->m_spellPriority > highestPrioritySpellFoundByKit[attach]->m_spellPriority)) \
  {                                                                                                                                     \
    highestPrioritiesByKit[attach] = kitRec;                                                                                            \
    highestPrioritySpellFoundByKit[attach] = spellRec;                                                                                  \
  }
    CHECK_HIGHEST_AURA(UNITEFFECT_ATTACHBASE, m_baseEffect);
    CHECK_HIGHEST_AURA(UNITEFFECT_ATTACHHEAD, m_headEffect);
    CHECK_HIGHEST_AURA(UNITEFFECT_ATTACHLEFTHAND, m_leftHandEffect);
    CHECK_HIGHEST_AURA(UNITEFFECT_ATTACHRIGHTHAND, m_rightHandEffect);
    CHECK_HIGHEST_AURA(UNITEFFECT_ATTACHCHEST, m_chestEffect);
    CHECK_HIGHEST_AURA(UNITEFFECT_ATTACHBREATH, m_breathEffect);
#undef CHECK_HIGHEST_AURA
  }

  if ((m_currentTorsoAnimState == ANIM_STATE_SPELLCAST || m_currentTorsoAnimState == ANIM_STATE_SPELLAURA) &&
      (!highestSpellWithAnim || static_cast<int>(m_animatingAura) != highestPriorityAura->auraSlot)) {
    m_animatingAura = -1;
    if (m_currentTorsoAnimState == ANIM_STATE_SPELLAURA) {
      ClearTorsoAnimation(0);
    }
  }

  if (!highestPriorityAura) {
    return;
  }

#define ATTACH_HIGHEST_AURA(attach, field)                                                           \
  FATALASSERT(!highestPrioritiesByKit[attach] == !highestPrioritySpellFoundByKit[attach]);           \
  if (highestPrioritiesByKit[attach] && highestPrioritySpellFoundByKit[attach]) {                    \
    MaybeAttachAura(                                                                                 \
        attach, highestPrioritiesByKit[attach]->field, highestPrioritySpellFoundByKit[attach]->m_ID, \
        highestPrioritySpellFoundByKit[attach]->m_spellPriority, 0                                   \
    );                                                                                               \
  }
  ATTACH_HIGHEST_AURA(UNITEFFECT_ATTACHBASE, m_baseEffect);
  ATTACH_HIGHEST_AURA(UNITEFFECT_ATTACHHEAD, m_headEffect);
  ATTACH_HIGHEST_AURA(UNITEFFECT_ATTACHLEFTHAND, m_leftHandEffect);
  ATTACH_HIGHEST_AURA(UNITEFFECT_ATTACHRIGHTHAND, m_rightHandEffect);
  ATTACH_HIGHEST_AURA(UNITEFFECT_ATTACHCHEST, m_chestEffect);
  ATTACH_HIGHEST_AURA(UNITEFFECT_ATTACHBREATH, m_breathEffect);
#undef ATTACH_HIGHEST_AURA

  if (highestSpellWithAnim) {
    m_spellCastingAnim = static_cast<ANIMENUMERATION>(highestSpellWithAnim->stateKitRec->m_anim);
    m_animatingAura = highestSpellWithAnim->auraSlot;
    if (!SetTorsoAnimation(ANIM_STATE_SPELLAURA, 0, 0)) {
      SetTorsoAnimation(0, 0, 0);
    }
  }
}

void CGUnit_C::AddPendingShapeshiftEffect(int oldSpell) {
  const SpellRec *spell = g_spellDB.GetRecord(oldSpell);
  if (spell && IsShapeshiftSpell(spell)) {
    m_shapeShiftPoof = spell;
  }
}

void CGUnit_C::AddKitAuras(const SpellVisualKitRec *kitRec, const SpellRec *spellRec) {
  MaybeAttachAura(UNITEFFECT_ATTACHBASE, kitRec->m_baseEffect, spellRec->m_ID, spellRec->m_spellPriority, 0);
  MaybeAttachAura(UNITEFFECT_ATTACHHEAD, kitRec->m_headEffect, spellRec->m_ID, spellRec->m_spellPriority, 0);
  MaybeAttachAura(UNITEFFECT_ATTACHLEFTHAND, kitRec->m_leftHandEffect, spellRec->m_ID, spellRec->m_spellPriority, 0);
  MaybeAttachAura(UNITEFFECT_ATTACHRIGHTHAND, kitRec->m_rightHandEffect, spellRec->m_ID, spellRec->m_spellPriority, 0);
  MaybeAttachAura(UNITEFFECT_ATTACHCHEST, kitRec->m_chestEffect, spellRec->m_ID, spellRec->m_spellPriority, 0);
  MaybeAttachAura(UNITEFFECT_ATTACHBREATH, kitRec->m_breathEffect, spellRec->m_ID, spellRec->m_spellPriority, 0);
}

bool VisualHasDecay(HMODEL model) {
  return model && ModelHasSequenceId(model, 2);
}

void CGUnit_C::RemoveAuraVisual(UNITEFFECTATTACHPPOINT attach) {
  GEOCOMPONENTLINKS linkPoint = UnitEffectGetLinkPointFromAttachment(attach);
  FATALASSERT(linkPoint != ATTACH_NONE);

  AuraVisual &visual = m_auraVisual[attach];
  if (!visual.HasArt()) {
    return;
  }

  FATALASSERT(visual.GetModel());
  if (visual.IsWorldModel()) {
    HMODEL model = visual.GetModel();
    if (VisualHasDecay(model)) {
      AuraDecayNode *decay = s_auraDecayFreeList.Get(0);
      decay->visual.Set(visual);
      decay->unit = GetGUID();
      s_activeAuraDecays.LinkNode(decay, LIST_HEAD, 0);
      ModelSetSequence(model, 2, 0);
      ModelSetSeqFinishedHandler(model, 2, OnAuraDecayFinished, decay);
    } else {
      visual.Clear();
    }
    return;
  }

  HMODEL charModel = GetCharacterModel(0);
  FATALASSERT(charModel);
  HMODEL model = visual.GetModel();
  if (VisualHasDecay(model)) {
    ModelSetSequence(model, 2, 0);
    AuraDecayNode *decay = s_auraDecayFreeList.Get(0);
    decay->visual.SetModel(model);
    decay->unit = GetGUID();
    decay->attach = attach;
    s_activeAuraDecays.LinkNode(decay, LIST_HEAD, 0);
    ModelSetSeqFinishedHandler(decay->visual.GetModel(), 2, OnAuraDecayFinished, decay);
  } else if (!ModelRemoveLink(charModel, linkPoint, model)) {
    SysMsgPrintf(SYSMSG_WARNING, 16, "UNITMISSINGCONNECTION|%d|%d", m_unit->displayID, linkPoint);
  }
  visual.Clear();
  HandleClose(charModel);
}

void CGUnit_C::RemoveAuraEffect(unsigned int slot, int previousSpell) {
  ACTIVEAURAINFO *active = FindActiveAuraInfo(slot);
  if (active) {
    RemoveSpellProcAuraEffect(active);
    active->Unlink();
    s_auraInfoFreeList.Put(active);
  }

  for (unsigned int attach = 0; attach < sizeof(m_auraVisual) / sizeof(m_auraVisual[0]); ++attach) {
    if (m_auraVisual[attach].GetSpellID() == static_cast<unsigned int>(previousSpell)) {
      RemoveAuraVisual(static_cast<UNITEFFECTATTACHPPOINT>(attach));
    }
  }

  if (GetGUID() != ClntObjMgrGetActivePlayer()) {
    return;
  }

  const SpellRec *spellRec = g_spellDB.GetRecord(previousSpell);
  if (!spellRec) {
    SErrPrepareAppFatal(__FILE__, __LINE__);
    SErrDisplayAppFatal("Spell record not found.  Your spell.dbc file is probably out of date!");
  }

  for (unsigned int effect = 0; effect < 3; ++effect) {
    if (spellRec->m_effect[effect] == 76) {
      CVar *fov = CVar::Lookup("fov");
      if (fov) {
        fov->Set("90", true, false, false);
      }
    }
  }
}

void CGUnit_C::AddAuraEffect(unsigned int slot, bool startNow) {
  FATALASSERT(slot < sizeof(m_unit->auras) / sizeof(m_unit->auras[0]));
  FATALASSERT(m_model);

  if (FindActiveAuraInfo(slot)) {
    return;
  }

  if (!m_unit->auras[slot]) {
    return;
  }

  const SpellRec *spellRec = g_spellDB.GetRecord(m_unit->auras[slot]);
  if (!spellRec) {
    SysMsgPrintf(SYSMSG_ERROR, 2, "NOSPELLIDFOUND|%d", m_unit->auras[slot]);
    return;
  }

  if (GetGUID() == ClntObjMgrGetActivePlayer()) {
    for (unsigned int effect = 0; effect < 3; ++effect) {
      if (spellRec->m_effect[effect] == 76) {
        CVar *fov = CVar::Lookup("fov");
        if (fov) {
          char buf[20];
          SStrPrintf(buf, sizeof(buf), "%d", spellRec->m_effectMiscValue[effect]);
          fov->Set(buf, true, false, false);
        }
      }
    }
  }

  const SpellVisualRec *spellVisualRec = g_spellVisualDB.GetRecord(spellRec->m_spellVisualID);
  if (!spellVisualRec) {
    SysMsgPrintf(SYSMSG_ERROR, 2, "SPELLVISUALIDNOTFOUND|%d", spellRec->m_spellVisualID);
    return;
  }

  if (!startNow) {
    const SpellVisualKitRec *castRec = g_spellVisualKitDB.GetRecord(spellVisualRec->m_castKit);
    if (castRec && Object_C_AnimHasHitEvent(castRec->m_anim)) {
      return;
    }
  }

  const SpellVisualKitRec *impactRec = g_spellVisualKitDB.GetRecord(spellVisualRec->m_impactKit);
  if (impactRec) {
    SetImpactKitEffect(m_unit->auras[slot], this, impactRec, 1);
  }

  const SpellVisualKitRec *stateRec = g_spellVisualKitDB.GetRecord(spellVisualRec->m_stateKit);
  if (!stateRec) {
    return;
  }

  ACTIVEAURAINFO *active = s_auraInfoFreeList.Get(0);
  m_activeAuraInfo.LinkNode(active, LIST_HEAD, 0);
  active->auraSlot = slot;
  active->stateKitRec = stateRec;

  NTempest::C3Vector pos;
  GetPosition(pos);
  SndInterfacePlaySound(stateRec->m_soundID, pos, -1, 1.0f);
  AddKitAuras(stateRec, spellRec);
  AddSpellProcAuraEffect(slot, stateRec);

  bool          higherPriority = m_animatingAura == -1;
  if (!higherPriority) {
    const SpellRec *current = g_spellDB.GetRecord(m_unit->auras[m_animatingAura]);
    higherPriority = !current || spellRec->m_spellPriority > current->m_spellPriority;
  }
  if ((m_animatingAura == -1 || higherPriority) && stateRec->m_anim > 0) {
    m_spellCastingAnim = static_cast<ANIMENUMERATION>(stateRec->m_anim);
    m_animatingAura = slot;
    SetTorsoAnimation(ANIM_STATE_SPELLAURA, 0, 0);
  }
}

int OnFirstAuraSequenceFinished(void *param) {
  ModelSetRandomSequenceFidget(reinterpret_cast<HMODEL>(param), 1, 0);
  return 1;
}

void CGUnit_C::MaybeAttachAura(UNITEFFECTATTACHPPOINT attach, unsigned int effect, unsigned int spellID, int priority, bool permanent) {
  AuraVisual &visual = m_auraVisual[attach];
  if (visual.HasArt() && visual.GetSpellID() == spellID) {
    return;
  }
  if (visual.HasArt()) {
    const SpellRec *currentSpell = g_spellDB.GetRecord(visual.GetSpellID());
    if (currentSpell && currentSpell->m_spellPriority > priority) {
      return;
    }
  }

  RemoveAuraVisual(attach);
  bool isWorldObj;
  if (!UnitEffectIsAuraWorldObject(effect, isWorldObj)) {
    return;
  }

  if (isWorldObj) {
    unsigned long object = UnitEffectCreateWorldModelAura(effect, GetPosition(), GetFacing());
    if (!object) {
      return;
    }
    visual.SetWorldObject(object);
  } else {
    HMODEL model = UnitEffectCreateAuraModel(effect);
    if (!model) {
      return;
    }
    visual.SetModel(model);
    HandleClose(model);

    HMODEL charModel = GetCharacterModel(0);
    FATALASSERT(charModel);
    GEOCOMPONENTLINKS linkPoint = UnitEffectGetLinkPointFromAttachment(attach);
    HMODEL            auraModel = visual.GetModel();
    if (ModelAddLink(charModel, linkPoint, auraModel, 1.0f)) {
      ModelSetSequence(auraModel, 0, 0);
      ModelSetSeqFinishedHandler(auraModel, 0, OnFirstAuraSequenceFinished, auraModel);
    } else {
      visual.Clear();
    }
    HandleClose(charModel);
  }

  visual.SetSpellID(spellID);
  visual.SetEffect(effect);
  visual.SetPermanent(permanent);
}

void CGUnit_C::SetAuraMirrorHandler(unsigned int slot, int(*handler)(unsigned __int64, unsigned int, unsigned int, const void *, void *)) {
  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 4 * slot + 200, 4, handler, 0, HANDLER_PRIORITY_HIGH);
}

void CGUnit_C::UnsetAuraMirrorHandler(
    unsigned int slot,
    int(*handler)(unsigned __int64, unsigned int, unsigned int, const void *, void *)
) {
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 4 * slot + 200, handler, 0);
}

void CGUnit_C::OnAuraChanged(unsigned int slot, int previousValue) {
  FATALASSERT(slot < sizeof(m_unit->auras));
  if (previousValue) {
    RemoveAuraEffect(slot, previousValue);
    if (!m_unit->auras[slot]) {
      UnitCombatLogAuraAddedOrRemoved(this, previousValue, 0, slot);
      RefreshAuraVisuals();
      AddPendingShapeshiftEffect(previousValue);
    }
  }

  if (m_unit->auras[slot]) {
    AddPendingShapeshiftEffect(m_unit->auras[slot]);
    AddAuraEffect(slot, false);
    UnitCombatLogAuraAddedOrRemoved(this, m_unit->auras[slot], 1, slot);
    RefreshAuraVisuals();
  }
}

void CGUnit_C::OnFlagChanged(unsigned int oldFlags) {
  unsigned long currentTime = OsGetAsyncTimeMs();
  unsigned int  newFlags = m_unit->flags;
  unsigned int  xorBits = oldFlags ^ newFlags;

  if (xorBits & 0x02000000) {
    CombatLoggingFlagChanged();
  }

  if (m_unit->health > 0 && GetGUID() != ClntObjMgrGetActivePlayer() && (xorBits & 0x400)) {
    if (newFlags & 0x400) {
      UpdateBaseAnimation(ANIM_STATE_LOOTBEGIN, 0);
    } else if (m_currentBaseAnimState == ANIM_STATE_LOOTBEGIN) {
      UpdateBaseAnimation(ANIM_STATE_LOOTEND, 0);
    }
  }

  if ((xorBits & 0x00C40004) && GetGUID() == ClntObjMgrGetActivePlayer()) {
    if (xorBits & 0x00C00004) {
      int hasControl =
          (newFlags & 0x01000000) ||
          ((m_obj->m_type & TYPE_PLAYER) &&
           !m_unit->charmedBy &&
           ((newFlags & 2) || !(newFlags & 0x00C00004)) &&
           !(newFlags & 1));
      CGGameUI::OnClientControlChanged(hasControl);
    }
    CGInputControl::GetActive()->UpdatePlayer(currentTime);
  }

  if ((xorBits & 0x20000) && GetGUID() == ClntObjMgrGetActivePlayer()) {
    CGActionBar::UpdateUsable();
  }

  if (xorBits & 0x200000) {
    SetAttachmentHidden(OBJATTACH_MAINHAND, (newFlags & 0x200000) != 0);
    DetermineReadySequence(false);
  }

  if (xorBits & 0x4000) {
    Script_SendUnitSignal(GetGUID(), 16);
    if ((newFlags & 0x4000) && GetGUID() == CGGameUI::GetLockedTarget()) {
      CGPlayer_C *player = static_cast<CGPlayer_C *>(
          ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__)
      );
      if (player && (player->m_flags & 0x400)) {
        player->SetCombatMode(0);
      }
    }
  }

  if (xorBits & 0x18000) {
    UpdateUnitAlpha();
    PlayerNameTriggerNameRegenerate(m_unitNameHandle);
  }

  if ((xorBits & 0x8000) && GetGUID() == ClntObjMgrGetActivePlayer()) {
    CGActionBar::UpdateUsable();
  }

  if ((xorBits & 0x80000) && GetGUID() == ClntObjMgrGetActivePlayer()) {
    CGActionBar::UpdateUsable();
    FrameScript_SignalEvent((newFlags & 0x80000) ? 186 : 187);
  }

  if (xorBits & 0x800) {
    unsigned __int64 owner = m_unit->charmedBy ? m_unit->charmedBy : m_unit->summonedBy;
    if (owner == ClntObjMgrGetActivePlayer()) {
      FrameScript_SignalEvent((newFlags & 0x800) ? 312 : 313);
    }
  }

  if (xorBits & 0x3000) {
    UpdateUnitMountInfo(0, xorBits);
  }

  if (GetGUID() == ClntObjMgrGetActivePlayer() &&
      (xorBits & 0x100000) &&
      (newFlags & 0x100000)) {
    ClearTrackingTarget(false);
  }
}

void CGUnit_C::HandleAnimEvent(const char *eventName, const NTempest::C3Vector &pos) {
  unsigned long code;
  memcpy(&code, eventName, sizeof(code));

  switch (code) {
    case 0x304C4224:  // $BL0
    case 0x304C4624:  // $FL0
    case 0x304C5224:  // $RL0
    case 0x304C5324:  // $SL0
    case 0x304C5724:  // $WL0
    case 0x314C4224:  // $BL1
    case 0x314C4624:  // $FL1
    case 0x314C5224:  // $RL1
    case 0x314C5324:  // $SL1
    case 0x314C5724:  // $WL1
    case 0x324C4224:  // $BL2
    case 0x324C4624:  // $FL2
    case 0x324C5224:  // $RL2
    case 0x324C5324:  // $SL2
    case 0x324C5724:  // $WL2
    case 0x334C4224:  // $BL3
    case 0x334C4624:  // $FL3
    case 0x334C5224:  // $RL3
    case 0x334C5324:  // $SL3
    case 0x334C5724:  // $WL3
      FootstepAnimEventHit(pos, 1);
      return;

    case 0x30524224:  // $BR0
    case 0x30524624:  // $FR0
    case 0x30525224:  // $RR0
    case 0x30525324:  // $SR0
    case 0x30525724:  // $WR0
    case 0x31524224:  // $BR1
    case 0x31524624:  // $FR1
    case 0x31525224:  // $RR1
    case 0x31525324:  // $SR1
    case 0x31525724:  // $WR1
    case 0x32524224:  // $BR2
    case 0x32524624:  // $FR2
    case 0x32525224:  // $RR2
    case 0x32525324:  // $SR2
    case 0x32525724:  // $WR2
    case 0x33524224:  // $BR3
    case 0x33524624:  // $FR3
    case 0x33525224:  // $RR3
    case 0x33525324:  // $SR3
    case 0x33525724:  // $WR3
      FootstepAnimEventHit(pos, 0);
      return;

    case 0x30484124:  // $AH0
    case 0x31484124:  // $AH1
    case 0x32484124:  // $AH2
    case 0x33484124:  // $AH3
    case 0x48414324:  // $CAH
    case 0x48544424:  // $DTH
    case 0x50504324:  // $CPP
    case 0x53534324:  // $CSS
      HandleCombatAnimEvent(eventName, code, pos);
      return;

    case 0x31444624:  // $FD1
    case 0x32444624:  // $FD2
    case 0x33444624:  // $FD3
    case 0x34444624:  // $FD4
    case 0x35444624:  // $FD5
    case 0x36444624:  // $FD6
    case 0x37444624:  // $FD7
    case 0x38444624:  // $FD8
    case 0x39444624:  // $FD9
    case 0x58444624:  // $FDX
      HandlePlayStandSound(code, eventName);
      return;

    case 0x44534624:  // $FSD
      {
        unsigned int groundType;
        if (CWorld::QueryGroundType(m_worldObject, groundType)) {
          m_terrain = groundType;
        }
      }
      HandleFootfallAnimEvent(pos);
      return;

    case 0x48544224:  // $BTH
      BreathHandler(0);
      return;

    case 0x47475724:  // $WGG
      PlayUnitSound(UNITSOUNDTYPE_WINGGLIDE, 0);
      return;

    case 0x474E5724:  // $WNG
      PlayUnitSound(UNITSOUNDTYPE_WINGFLAP, 0);
      return;

    case 0x44525424:  // $TRD
      HandleSpellEventSound();
      return;

    case 0x4C485324:  // $SHL
    case 0x52485324:  // $SHR
      HandleSheatheAnimEvent(0, 0);
      return;

    case 0x4C534324:  // $CSL
    case 0x50574224:  // $BWP
    case 0x52534324:  // $CSR
    case 0x52574224:  // $BWR
    case 0x54534324:  // $CST
      if (m_currentTorsoAnimState == 38) {
        CheckPendingSpellAnimHits();
      }
      CheckPendingMissileRelease(&pos);
      return;
  }

  SysMsgPrintf(SYSMSG_WARNING, 16, "OBSOLETEANIMEVENT|%s", eventName);
}

void CGUnit_C::FootstepAnimEventHit(const NTempest::C3Vector &position, int isLeftFoot) {
  unsigned int       textureID;
  NTempest::C2Vector size(0.0f);
  GetFootprintInfo(&textureID, &size);
  UnitFootprintNewSplat(textureID, size, position, GetFacing(), !isLeftFoot, m_terrain);
  HandleFootstepAnimEvent(position);
}

void CGUnit_C::HandleFootstepAnimEvent(const NTempest::C3Vector &position) {
  FATALASSERT(m_modelData);
  if (!(m_move.m_moveFlags & 2) && !(m_modelData->m_flags & 1)) {
    UnitFootprintPlayParticle(this, position, m_terrain, GetScale() * m_footprintParticleScale);
  }

  CGWorldFrame *worldFrame = CGWorldFrame::GetActive();
  if (worldFrame) {
    CGCamera *camera = worldFrame->Camera();
    if (camera) {
      FATALASSERT(m_modelData);
      if (m_modelData->m_footstepShakeSize) {
        camera->AddShake(m_modelData->m_footstepShakeSize, position);
      }
    }
  }
}

void CGUnit_C::GetFootprintInfo(unsigned int *id, NTempest::C2Vector *size) {
  if ((m_unit->flags & 0x2000) && (m_flags & 0x10)) {
    FATALASSERT(id);
    FATALASSERT(size);
    *id = m_mountedFootprintID;
    *size = m_mountedFootprintSize;
  } else {
    *id = m_footprintTextureID;
    *size = m_footprintSize;
  }
}

const char *CGUnit_C::GetModelFileName() const {
  const CreatureDisplayInfoRec *displayInfo = g_creatureDisplayInfoDB.GetRecord(m_unit->displayID);
  if (!displayInfo) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "NOCREATUREDISPLAYIDFOUND|%d", m_unit->displayID);
    return "NoName";
  }

  const CreatureModelDataRec *modelData = g_creatureModelDataDB.GetRecord(displayInfo->m_modelID);
  if (!modelData) {
    SysMsgPrintf(SYSMSG_WARNING, 0x10, "INVALIDDISPLAYMODELRECORD|%d|%d", displayInfo->m_modelID, displayInfo->m_ID);
    return "NoName";
  }

  return modelData->m_ModelName;
}

int CGUnit_C::CanBeLooted(unsigned long currentTime) const {
  return m_unit->health <= 0 && (m_animFlags & 0x2000) && static_cast<int>(currentTime - m_deathTime) >= 0 && (m_unit->dynamicFlags & 1);
}

void CGUnit_C::OnDynamicFlagsChanged(unsigned int oldValue) {
  unsigned int newValue = m_unit->dynamicFlags;
  if (((oldValue ^ newValue) & 1) == 0) {
    return;
  }

  if (newValue & 1) {
    if (m_unit->health <= 0) {
      int effect = UnitEffectGetSpecialVisual(SPECIALEFFECT_LOOTART);
      if (effect >= 0) {
        MaybeAttachAura(UNITEFFECT_ATTACHBASE, effect, 0, 100, 1);
      }
    }
    return;
  }

  unsigned int effect = UnitEffectGetSpecialVisual(SPECIALEFFECT_LOOTART);
  for (unsigned int attach = 0; attach < NUM_UNITEFFECTATTACHPOINTS; ++attach) {
    const unsigned int attachedEffect = m_auraVisual[attach].GetEffect();
    if (attachedEffect == effect) {
      RemoveAuraVisual(static_cast<UNITEFFECTATTACHPPOINT>(attach));
    }
  }
}

int MoveHeartBeatHandler(const void *packetData, void *param) {
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(CGUnit_C::GetActiveMover(), __FILE__, __LINE__));
  if (unit && !(unit->m_move.m_moveFlags & 0x10000000)) {
    unit->SendMovementUpdate(MSG_MOVE_HEARTBEAT);
    SysMsgAdd("MOVEMENT|Movement heartbeat", SYSMSG_INFO, 1);
  }

  s_moveHeartBeatTimer = ClientSetTimer(500, MoveHeartBeatHandler, 0);
  return 1;
}

HTEXCOMPONENT CGUnit_C::GetTexComponent() const {
  return m_texComponent;
}

void CGUnit_C::SetActiveMover(const unsigned __int64 &guid) {
  if (m_activeMover) {
    CGUnit_C *mover = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(m_activeMover, __FILE__, __LINE__));
    if (mover) {
      mover->OnMoveStopLocal(OsGetAsyncTimeMs());
    }
  }
  m_activeMover = guid;
}

void CGUnit_C::BuildMovementUpdate(NETMESSAGE messageId, CDataStore *msg) const {
  msg->Put(static_cast<unsigned int>(messageId));
  const NTempest::C3Vector &position = m_move.GetPosition(m_move.m_position);
  msg->Put(m_move.m_transportGUID)
      .Put(m_move.m_position.x)
      .Put(m_move.m_position.y)
      .Put(m_move.m_position.z)
      .Put(m_move.m_facing)
      .Put(position.x)
      .Put(position.y)
      .Put(position.z)
      .Put(m_move.GetFacing(m_move.m_facing))
      .Put(m_move.m_pitch)
      .Put(m_move.m_moveFlags & 0xFAFF0BFF);
}

void CGUnit_C::SendMovementUpdate(NETMESSAGE messageId) {
  m_lastSentFacing = GetFacing();
  m_lastSentPitch = m_move.m_pitch;

  CDataStore msg;
  BuildMovementUpdate(messageId, &msg);
  msg.Finalize();
  ClientServices_Send(&msg);
}

float CGUnit_C::GetDisplayFacing() const {
  return m_move.GetFacing(m_displayFacing);
}

float CGUnit_C::GetSmoothFacing() const {
  return m_move.GetFacing(m_smoothFacing);
}

bool CGUnit_C::IsTurningState() const {
  unsigned int state = (m_unit->flags & 0x2000) ? m_currentMountAnimState : m_currentBaseAnimState;
  return state == 18 || state == 19;
}

int CGUnit_C::ShouldShuffle() const {
  unsigned __int64 unitBeingLooted = 0;
  if (m_obj->m_type & TYPE_PLAYER) {
    unitBeingLooted = static_cast<const CGPlayer_C *>(this)->CGPlayer_C::GetUnitBeingLooted();
  }

  return !(m_move.m_moveFlags & 0x02000000) &&
         ((m_unit->flags & 0x20000) || !const_cast<CCombatClient &>(m_combat).IsAttacking()) &&
         !unitBeingLooted && !m_unit->standState && !(s_animInfo[m_currentTorsoAnimState].flags & 8);
}

void CGUnit_C::UpdateDisplayFacing() {
  HMODEL model = GetObjectModel();

  if (m_currentBaseAnimState >= 8 && m_currentBaseAnimState <= 17) {
    m_displayFacing = m_smoothFacing;
    return;
  }

  float angleDelta = m_smoothFacing - m_displayFacing;
  if ((m_move.m_moveFlags & 0x400F) || fabs(angleDelta) < 0.00000023841858f) {
    ModelRemoveObjectFaceDir(model, 4);
    ModelRemoveObjectFaceDir(model, 6);
    if (IsTurningState()) {
      CGUnit_C::UpdateBaseAnimation(0);
    }
    m_displayFacing = m_smoothFacing;
    return;
  }

  if (angleDelta > 3.1415927f) {
    angleDelta -= 6.2831855f;
  } else if (angleDelta < -3.1415927f) {
    angleDelta += 6.2831855f;
  }

  float absHeadAngle = fabs(angleDelta);
  float absAngleToRetract = 0.0f;
  float absAngleConsumed = 0.0f;

  if (!(m_animFlags & 8)) {
    int elapsed = OsGetAsyncTimeMs() - m_move.m_moveStartTime;
    if (elapsed < 0) {
      elapsed = 0;
    }
    absAngleToRetract = static_cast<float>(elapsed) * 0.001f * m_move.m_turnRate * 2.0f;
    if (absAngleToRetract > absHeadAngle) {
      absAngleToRetract = absHeadAngle;
    }
  }

  float absAngleLeft = absHeadAngle - absAngleToRetract;
  if (fabs(absAngleLeft) < 0.00000023841858f) {
    ModelRemoveObjectFaceDir(model, 4);
    ModelRemoveObjectFaceDir(model, 6);
  } else {
    float maxHeadAngle = 0.69813168f;
    float torsoTurnRate = 0.25f;
    if (GetTrackingTarget()) {
      torsoTurnRate = 1.0f;
      maxHeadAngle = 0.0f;
    }

    absAngleLeft -= absAngleToRetract;
    absHeadAngle = torsoTurnRate * absAngleLeft;
    if (absHeadAngle > 0.69813168f) {
      absHeadAngle = 0.69813168f;
    }

    float signedAngle = angleDelta < 0.0f ? -absHeadAngle : absHeadAngle;
    NTempest::C3Vector torsoTurn(cos(signedAngle), sin(signedAngle), 0.0f);
    if (ModelApplyObjectFaceDir(model, 4, torsoTurn)) {
      absAngleConsumed = absHeadAngle;
    }

    absHeadAngle = absAngleLeft - absAngleConsumed;
    if (absHeadAngle > maxHeadAngle) {
      absHeadAngle = maxHeadAngle;
    }

    signedAngle = angleDelta < 0.0f ? -absHeadAngle : absHeadAngle;
    NTempest::C3Vector headTurn(cos(signedAngle), sin(signedAngle), 0.0f);
    if (ModelApplyObjectFaceDir(model, 6, headTurn)) {
      absAngleConsumed += absHeadAngle;
    }
    absAngleLeft -= absAngleConsumed;
  }

  if (ShouldShuffle()) {
    if (((m_animFlags & 0x10) && absAngleLeft > 0.00000023841858f) ||
        (!(m_animFlags & 0x18) && fabs(absAngleToRetract) >= 0.00000023841858f))
    {
      if (angleDelta >= 0.0f && m_currentBaseAnimState != ANIM_STATE_TURNING_LEFT) {
        CGUnit_C::UpdateBaseAnimation(ANIM_STATE_TURNING_LEFT, 0);
      } else if (angleDelta < 0.0f && m_currentBaseAnimState != ANIM_STATE_TURNING_RIGHT) {
        CGUnit_C::UpdateBaseAnimation(ANIM_STATE_TURNING_RIGHT, 0);
      }
    } else if (IsTurningState()) {
      CGUnit_C::UpdateBaseAnimation(0);
    }
  }

  float signedConsumed = angleDelta < 0.0f ? -absAngleConsumed : absAngleConsumed;
  m_displayFacing = m_smoothFacing - signedConsumed;
}

void CGUnit_C::UpdateSmoothFacing() {
  float facing = m_move.m_facing;

  CGObject_C *activeMover = ClntObjMgrObjectPtr(m_activeMover, __FILE__, __LINE__);
  if (this == activeMover) {
    unsigned int unitFlags = m_unit->flags;
    if ((unitFlags & 0x1000000) ||
        ((m_obj->m_type & TYPE_PLAYER) && !m_unit->charmedBy && ((unitFlags & 2) || !(unitFlags & 0xC00004)) && !(unitFlags & 1)))
    {
      m_smoothFacing = facing;
      CGInputControl *input = CGInputControl::GetActive();
      if ((m_move.m_moveFlags & 0x30) || input->CameraCanTurnPlayer()) {
        m_animFlags |= 8;
      } else {
        m_animFlags &= ~8u;
      }
      if ((m_move.m_moveFlags & 0x30) || input->IsMouseDragMoving()) {
        m_animFlags |= 0x10;
      } else {
        m_animFlags &= ~0x10u;
      }
      return;
    }
  }

  if (!(m_move.m_moveFlags & 0xF) && !m_unit->standState && !m_unit->emoteState) {
    if (m_flags & 1) {
      facing = m_forcedDisplayFacing;
    } else if (!((m_unit->flags & 8) && (m_obj->m_type & TYPE_PLAYER))) {
      unsigned __int64 target = 0;
      if ((m_unit->flags & 0x20000) || !m_combat.IsAttacking()) {
        if (GetType() & TYPE_PLAYER) {
          target = static_cast<const CGPlayer_C *>(this)->CGPlayer_C::GetUnitBeingLooted();
        }
        if (!target && GetGUID() == CGGameUI::GetInteractTarget()) {
          target = ClntObjMgrGetActivePlayer();
        }
      } else {
        target = m_combat.IsAttacking();
      }

      CGObject_C *targetObject = ClntObjMgrObjectPtr(target, __FILE__, __LINE__);
      if (targetObject) {
        CGUnit_C *targetUnit = static_cast<CGUnit_C *>(targetObject);
        facing = CalculateFacingTo(m_move.m_position, targetUnit->m_move.m_position);
      }
    }
  }

  float deltaFacing = facing - m_smoothFacing;
  if (fabs(deltaFacing) <= 0.01f) {
    m_smoothFacing = facing;
    m_savedFacingDeltas[0] = 0.0f;
    return;
  }

  if (deltaFacing > 3.1415927f) {
    deltaFacing -= 6.2831855f;
  } else if (deltaFacing < -3.1415927f) {
    deltaFacing += 6.2831855f;
  }

  float *savedFacingDeltas = m_savedFacingDeltas;
  if ((deltaFacing < 0.0f && savedFacingDeltas[0] > 0.0f) || (deltaFacing > 0.0f && savedFacingDeltas[0] < 0.0f)) {
    savedFacingDeltas[0] = 0.0f;
  }

  if (savedFacingDeltas[0] == 0.0f) {
    savedFacingDeltas[0] = deltaFacing;
    savedFacingDeltas[1] = deltaFacing;
    savedFacingDeltas[2] = deltaFacing;
    savedFacingDeltas[3] = deltaFacing;
  } else {
    memmove(savedFacingDeltas + 1, savedFacingDeltas, 3 * sizeof(float));
    savedFacingDeltas[0] = deltaFacing;
    float average = 0.25f * (savedFacingDeltas[0] + savedFacingDeltas[1] + savedFacingDeltas[2] + savedFacingDeltas[3]);
    if (deltaFacing < 0.0f) {
      if (deltaFacing > average) {
        deltaFacing = average;
      }
    } else if (deltaFacing < average) {
      deltaFacing = average;
    }
  }

  m_smoothFacing += deltaFacing * 0.5f;
  if (m_smoothFacing >= 6.2831855f) {
    m_smoothFacing -= 6.2831855f;
  } else if (m_smoothFacing < 0.0f) {
    m_smoothFacing += 6.2831855f;
  }
}

void CGUnit_C::SetSmoothFacing(float facing) {
  m_smoothFacing = facing;
  while (m_smoothFacing >= 6.2831855f) {
    m_smoothFacing -= 6.2831855f;
  }
  memset(m_savedFacingDeltas, 0, sizeof(m_savedFacingDeltas));
}

static void UpdateLocalPlayerFallState(int falling) {
  CGUnit_C::StopMoveHeartbeatTimer();
  if (falling) {
    s_moveHeartBeatTimer = ClientSetTimer(500, MoveHeartBeatHandler, 0);
  }
}

void CGUnit_C::OnTeleportLocalNoUpdate(unsigned long eventTime, const NTempest::C3Vector &position, float facing) {
  static_cast<CMovement &>(m_move).OnTeleportLocal(eventTime, position, facing);
  UpdateBaseAnimation(0);
  if (GetGUID() == CGWorldFrame::GetActiveCamera()->GetTarget()) {
    CGGameUI::ResetCamera();
  }
}

void CGUnit_C::UpdateSwimmingStatus(unsigned long eventTime, int inWater, float depth) {
  if (m_move.m_moveFlags & 0x800) {
    return;
  }

  if (inWater) {
    if (!(m_move.m_moveFlags & 0x02000000) && depth > 0.0f) {
      if (GetObjectHeight() * 0.75f < depth) {
        static_cast<CMovement &>(m_move).StartSwim(eventTime);
      }
      if (m_move.m_moveFlags & 0x4000) {
        PlaySplashSound(GetPosition());
      }
    }
  } else if (m_move.m_moveFlags & 0x02000000) {
    static_cast<CMovement &>(m_move).StopSwim(eventTime);
  }
}

void CGUnit_C::SendRedirectionMessage() {
  if (m_move.m_lastReDirectionSent.SquaredMag() < 0.00000023841858f ||
      NTempest::C3Vector::Dot(m_move.m_reDirection, m_move.m_lastReDirectionSent) <= 0.99984771f)
  {
    CDataStore msg;
    BuildMovementUpdate(MSG_MOVE_COLLIDE_REDIRECT, &msg);
    msg.Put(m_move.m_reDirection.x);
    msg.Put(m_move.m_reDirection.y);
    msg.Put(m_move.m_reDirection.z);
    msg.Finalize();
    ClientServices_Send(&msg);
    m_move.m_lastReDirectionSent = m_move.m_reDirection;
  }
}

void CGUnit_C::StopMoveHeartbeatTimer() {
  if (s_moveHeartBeatTimer) {
    ClientKillTimer(s_moveHeartBeatTimer, MoveHeartBeatHandler, "MoveHeartBeatHandler");
  }
  s_moveHeartBeatTimer = 0;
}

void CGUnit_C::StartMoveHeartbeatTimer() {
  StopMoveHeartbeatTimer();
  s_moveHeartBeatTimer = ClientSetTimer(500, MoveHeartBeatHandler, 0);
}

unsigned int CGUnit_C::OffsetOf(OBJECT_TYPE_ID type) {
  switch (type) {
    case ID_OBJECT:
      return 0;
    case ID_ITEM:
    case ID_UNIT:
      return 24;
    case ID_CONTAINER:
      return 144;
    default:
      FATALASSERT(0);
      return -1;
  }
}

static void ResequenceEmoteAnims() {
  for (int index = g_emoteAnimsDB.GetNumRecords(); index;) {
    EmoteAnimsRec *emoteAnim = const_cast<EmoteAnimsRec *>(g_emoteAnimsDB.GetRecordByIndex(--index));
    emoteAnim->m_ProcessedAnimIndex = Object_C_GetAnimIndex(emoteAnim->m_AnimName);
  }
}

static bool ShowBreathCallback(CVar *h, const char *oldValue, const char *newValue, void *) {
  ConsoleWrite(SStrToInt(newValue) ? "Breath display enabled" : "Breath display disabled", DEFAULT_COLOR);
  return true;
}

void CGUnit_C::Initialize() {
  ClientServices_SetMessageHandler(MSG_MOVE_START_FORWARD, OnUnitMoveEventNoActive, 0);
  ClientServices_SetMessageHandler(MSG_MOVE_START_BACKWARD, OnUnitMoveEventNoActive, 0);
  ClientServices_SetMessageHandler(MSG_MOVE_STOP, OnUnitMoveEventNoActive, 0);
  ClientServices_SetMessageHandler(MSG_MOVE_START_STRAFE_LEFT, OnUnitMoveEventNoActive, 0);
  ClientServices_SetMessageHandler(MSG_MOVE_START_STRAFE_RIGHT, OnUnitMoveEventNoActive, 0);
  ClientServices_SetMessageHandler(MSG_MOVE_STOP_STRAFE, OnUnitMoveEventNoActive, 0);
  ClientServices_SetMessageHandler(MSG_MOVE_JUMP, OnUnitMoveEventNoActive, 0);
  ClientServices_SetMessageHandler(MSG_MOVE_START_TURN_LEFT, OnUnitMoveEventNoActive, 0);
  ClientServices_SetMessageHandler(MSG_MOVE_START_TURN_RIGHT, OnUnitMoveEventNoActive, 0);
  ClientServices_SetMessageHandler(MSG_MOVE_STOP_TURN, OnUnitMoveEventNoActive, 0);
  ClientServices_SetMessageHandler(MSG_MOVE_SET_RUN_MODE, OnUnitMoveEventNoActive, 0);
  ClientServices_SetMessageHandler(MSG_MOVE_SET_WALK_MODE, OnUnitMoveEventNoActive, 0);
  ClientServices_SetMessageHandler(MSG_MOVE_TELEPORT, OnUnitMoveEventNoActive, 0);
  ClientServices_SetMessageHandler(MSG_MOVE_SET_FACING, OnUnitMoveEventNoActive, 0);
  ClientServices_SetMessageHandler(MSG_MOVE_SET_PITCH, OnUnitMoveEventNoActive, 0);
  ClientServices_SetMessageHandler(MSG_MOVE_TOGGLE_COLLISION_CHEAT, OnUnitMoveEventNoActive, 0);
  ClientServices_SetMessageHandler(MSG_MOVE_SET_RUN_SPEED, OnUnitMoveEventNoActive, 0);
  ClientServices_SetMessageHandler(MSG_MOVE_SET_WALK_SPEED, OnUnitMoveEventNoActive, 0);
  ClientServices_SetMessageHandler(MSG_MOVE_SET_SWIM_SPEED, OnUnitMoveEventNoActive, 0);
  ClientServices_SetMessageHandler(MSG_MOVE_SET_TURN_RATE, OnUnitMoveEventNoActive, 0);
  ClientServices_SetMessageHandler(MSG_MOVE_ROOT, OnUnitMoveEventNoActive, 0);
  ClientServices_SetMessageHandler(MSG_MOVE_UNROOT, OnUnitMoveEventNoActive, 0);
  ClientServices_SetMessageHandler(MSG_MOVE_START_SWIM, OnUnitMoveEventNoActive, 0);
  ClientServices_SetMessageHandler(MSG_MOVE_STOP_SWIM, OnUnitMoveEventNoActive, 0);
  ClientServices_SetMessageHandler(MSG_MOVE_START_PITCH_UP, OnUnitMoveEventNoActive, 0);
  ClientServices_SetMessageHandler(MSG_MOVE_START_PITCH_DOWN, OnUnitMoveEventNoActive, 0);
  ClientServices_SetMessageHandler(MSG_MOVE_STOP_PITCH, OnUnitMoveEventNoActive, 0);
  ClientServices_SetMessageHandler(MSG_MOVE_HEARTBEAT, OnUnitMoveEventNoActive, 0);
  ClientServices_SetMessageHandler(MSG_MOVE_TELEPORT_ACK, OnUnitMoveEventActive, 0);
  ClientServices_SetMessageHandler(SMSG_MONSTER_MOVE, OnMonsterMoveEvent, 0);
  ClientServices_SetMessageHandler(SMSG_FORCE_SPEED_CHANGE, ::OnForceMoveChange, 0);
  ClientServices_SetMessageHandler(SMSG_FORCE_SWIM_SPEED_CHANGE, ::OnForceMoveChange, 0);
  ClientServices_SetMessageHandler(SMSG_FORCE_MOVE_ROOT, ::OnForceMoveChange, 0);
  ClientServices_SetMessageHandler(SMSG_FORCE_MOVE_UNROOT, ::OnForceMoveChange, 0);
  ClientServices_SetMessageHandler(SMSG_PUREMOUNT_CANCELLED, OnUnitMountCancelledEvent, 0);
  ClientServices_SetMessageHandler(SMSG_MOUNTSPECIAL_ANIM, ::OnSpecialMountAnim, 0);
  ClientServices_SetMessageHandler(SMSG_AI_REACTION, OnUnitReaction, 0);

  UnitSoundInitialize();
  Spell_C_Initialize();
  UnitEffectsInitialize();
  UnitCombatClientInitialize();
  ClntObjMgrSetTypeMirrorHandler(HIER_TYPE_UNIT, 40, 8, TargetMirrorHandler, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetTypeMirrorHandler(HIER_TYPE_UNIT, 56, 8, ChannelObjectMirrorHandler, 0, HANDLER_PRIORITY_NORMAL);
  ShadowInit();
  UnitFootprintInitialize();
  NPC_C_Initialize();
  ClearQuestIconHandles(1);
  InitTalkEmotes();

  g_unitSeqEndList.Clear();
  g_mountSeqEndList.Clear();
  for (unsigned int animID = 0; animID < NUM_OBJECTANIMATIONS; ++animID) {
    if (g_seqInformation[animID].callbackFlags & 1) {
      *g_unitSeqEndList.New() = animID;
    }
    if (g_seqInformation[animID].callbackFlags & 2) {
      *g_mountSeqEndList.New() = animID;
    }
  }

  ResequenceEmoteAnims();
  s_showBreathCvar =
      CVar::Register("ShowBreath", "Toggle display of unit/player breath", 0, "1", ShowBreathCallback, GRAPHICS, false, 0);
}

void CGUnit_C::PostShutdown() {
  UnitEffectsShutdown();
}

void CGUnit_C::Shutdown() {
  ClientServices_ClearMessageHandler(MSG_MOVE_START_FORWARD);
  ClientServices_ClearMessageHandler(MSG_MOVE_START_BACKWARD);
  ClientServices_ClearMessageHandler(MSG_MOVE_STOP);
  ClientServices_ClearMessageHandler(MSG_MOVE_START_STRAFE_LEFT);
  ClientServices_ClearMessageHandler(MSG_MOVE_START_STRAFE_RIGHT);
  ClientServices_ClearMessageHandler(MSG_MOVE_STOP_STRAFE);
  ClientServices_ClearMessageHandler(MSG_MOVE_JUMP);
  ClientServices_ClearMessageHandler(MSG_MOVE_START_TURN_LEFT);
  ClientServices_ClearMessageHandler(MSG_MOVE_START_TURN_RIGHT);
  ClientServices_ClearMessageHandler(MSG_MOVE_STOP_TURN);
  ClientServices_ClearMessageHandler(MSG_MOVE_SET_RUN_MODE);
  ClientServices_ClearMessageHandler(MSG_MOVE_SET_WALK_MODE);
  ClientServices_ClearMessageHandler(MSG_MOVE_TELEPORT);
  ClientServices_ClearMessageHandler(MSG_MOVE_SET_FACING);
  ClientServices_ClearMessageHandler(MSG_MOVE_SET_PITCH);
  ClientServices_ClearMessageHandler(MSG_MOVE_TOGGLE_COLLISION_CHEAT);
  ClientServices_ClearMessageHandler(MSG_MOVE_SET_RUN_SPEED);
  ClientServices_ClearMessageHandler(MSG_MOVE_SET_WALK_SPEED);
  ClientServices_ClearMessageHandler(MSG_MOVE_SET_SWIM_SPEED);
  ClientServices_ClearMessageHandler(MSG_MOVE_SET_TURN_RATE);
  ClientServices_ClearMessageHandler(MSG_MOVE_TELEPORT_ACK);
  ClientServices_ClearMessageHandler(MSG_MOVE_ROOT);
  ClientServices_ClearMessageHandler(MSG_MOVE_UNROOT);
  ClientServices_ClearMessageHandler(MSG_MOVE_START_SWIM);
  ClientServices_ClearMessageHandler(MSG_MOVE_STOP_SWIM);
  ClientServices_ClearMessageHandler(MSG_MOVE_START_PITCH_UP);
  ClientServices_ClearMessageHandler(MSG_MOVE_START_PITCH_DOWN);
  ClientServices_ClearMessageHandler(MSG_MOVE_STOP_PITCH);
  ClientServices_ClearMessageHandler(MSG_MOVE_HEARTBEAT);
  ClientServices_ClearMessageHandler(SMSG_MONSTER_MOVE);
  ClientServices_ClearMessageHandler(SMSG_PUREMOUNT_CANCELLED);
  ClientServices_ClearMessageHandler(SMSG_MOUNTSPECIAL_ANIM);
  ClientServices_ClearMessageHandler(SMSG_AI_REACTION);

  UnitSoundShutdown();
  Spell_C_Destroy();
  UnitCombatClientShutdown();
  NPC_C_Destroy();
  ClntObjMgrUnsetTypeMirrorHandler(HIER_TYPE_UNIT, 40, TargetMirrorHandler);
  ClntObjMgrUnsetTypeMirrorHandler(HIER_TYPE_UNIT, 56, ChannelObjectMirrorHandler);
  ShadowDestroy();
  UnitFootprintShutdown();
  ClearQuestIconHandles(0);
  s_bowStringVerts.Clear();
  s_bowStringIndices.Clear();

  while (AuraDecayNode *decay = s_activeAuraDecays.Head()) {
    s_auraDecayFreeList.Put(decay);
  }

  g_unitSeqEndList.Clear();
  g_mountSeqEndList.Clear();
}

int ViolenceGetLevel();

void CGUnit_C::NamePlateShow(int show) {
  s_drawNameplates = show;
  if (!show) {
    RemoveAllNamePlates();
  }
}

int CGUnit_C::GetCreatureType() const {
  if (m_stats) {
    return m_stats->m_creatureType;
  }

  const ChrRacesRec *race = g_chrRacesDB.GetRecord(m_unit->race);
  if (!race || race->m_creatureType <= 0) {
    return 0;
  }
  return race->m_creatureType;
}

void CGUnit_C::SetLocalTarget(unsigned __int64 target) {
  if (m_targetUnit == target) {
    return;
  }

  if (GetGUID() == ClntObjMgrGetActivePlayer()) {
    CMovement::FallLogWrite("\nLocal target guid set (0x%016I64X)\n\n", target);
  }

  m_targetUnit = target;
  CDataStore msg;
  msg.Put(static_cast<int>(CMSG_SET_TARGET));
  msg.Put(target);
  msg.Finalize();
  ClientServices_Send(&msg);
  LookAtTarget();
}

int CGUnit_C::IsSplashing(const NTempest::C3Vector &position) {
  unsigned int        liquid = 15;
  float               surfaceColPt = 0.0f;
  NTempest::C3Vector  waterDir(0.0f, 0.0f, 0.0f);
  int                 deep;
  return CWorld::QueryObjectLiquid(GetWorldObject(), liquid, surfaceColPt, waterDir, deep)
      && (liquid & 3) != 2
      && surfaceColPt > position.z
      && surfaceColPt - position.z < 0.66666669f;
}

bool CGUnit_C::IsShapeShifted() const {
  if (!m_unit->shapeshiftForm) {
    return false;
  }
  const SpellShapeshiftFormRec *form =
      g_spellShapeshiftFormDB.GetRecord(m_unit->shapeshiftForm);
  return !(form->m_flags & 1);
}

int CGUnit_C::IsUnderWater() const {
  float              surfaceColPt = 0.0f;
  unsigned int       liquidStatus = 0;
  NTempest::C3Vector waterDir;
  int                deep = 15;
  float              depth = 0.0f;
  if (CWorld::QueryObjectLiquid(GetWorldObject(), liquidStatus, surfaceColPt, waterDir, deep)) {
    depth = surfaceColPt - GetPosition().z;
  }
  return depth > m_move.m_collisionBoxHeight * 0.5f;
}

unsigned int CGUnit_C::ChooseAnimation(unsigned int state) const {
  FATALASSERT(state != ANIM_STATE_NONE);

  switch (state) {
    case ANIM_STATE_MOUNTED:
      return ANIM_MOUNT;
    case ANIM_STATE_DEAD:
      return (m_animFlags & 0x400) && IsUnderWater() ? ANIM_DROWN : ANIM_DEATH;
    case ANIM_STATE_SPELL:
      return ANIM_SPELL;
    case ANIM_STATE_TURNING_LEFT:
      return ANIM_SHUFFLE_LEFT;
    case ANIM_STATE_TURNING_RIGHT:
      return ANIM_SHUFFLE_RIGHT;
    case ANIM_STATE_WALK_BACKWARDS:
    case ANIM_STATE_DIAG_BACKWARDS_LEFT:
    case ANIM_STATE_DIAG_BACKWARDS_RIGHT:
      return (m_animFlags & 4) ? ANIM_WALK_BACKWARDS : GetWalkStateAnim();
    case ANIM_STATE_WALK:
    case ANIM_STATE_STRAFE_WALK_LEFT:
    case ANIM_STATE_STRAFE_WALK_RIGHT:
    case ANIM_STATE_DIAG_WALK_LEFT:
    case ANIM_STATE_DIAG_WALK_RIGHT:
      return GetWalkStateAnim();
    case ANIM_STATE_RUN:
    case ANIM_STATE_STRAFE_RUN_LEFT:
    case ANIM_STATE_STRAFE_RUN_RIGHT:
    case ANIM_STATE_DIAG_RUN_LEFT:
    case ANIM_STATE_DIAG_RUN_RIGHT:
      return GetRunSequence();
    case ANIM_STATE_SWIM_BACKWARDS:
      return (m_animFlags & 0x200) ? ANIM_SWIM_BACKWARDS : ANIM_SWIMIDLE;
    case ANIM_STATE_SWIM_STRAFE_LEFT:
      return (m_animFlags & 0x100) ? ANIM_SWIM_LEFT : ANIM_SWIMIDLE;
    case ANIM_STATE_SWIM_STRAFE_RIGHT:
      return (m_animFlags & 0x80) ? ANIM_SWIM_RIGHT : ANIM_SWIMIDLE;
    case ANIM_STATE_SWIM_IDLE:
      return ANIM_SWIMIDLE;
    case ANIM_STATE_SWIM:
      return ANIM_SWIM;
    case ANIM_STATE_STOP:
      return GetStopSequence();
    case ANIM_STATE_RISE:
      return ANIM_RISE;
    case ANIM_STATE_DODGE:
      return ANIM_DODGE;
    case ANIM_STATE_CRITICALWOUND:
      return ANIM_COMBATCRITICAL;
    case ANIM_STATE_SPELLPRECAST:
      return m_spellPrecastingAnim;
    case ANIM_STATE_SPELLCAST:
    case ANIM_STATE_CHANNELSPELL:
    case ANIM_STATE_SPELLAURA:
      return m_spellCastingAnim;
    case ANIM_STATE_JUMPING:
      return ANIM_JUMPSTART;
    case ANIM_STATE_JUMP_LANDING:
      return ANIM_JUMPEND;
    case ANIM_STATE_FALLING:
      return ANIM_FALL;
    case ANIM_STATE_STUN:
      return ANIM_STUN;
    case ANIM_STATE_WOUND:
      return DetermineWoundSequence();
    case ANIM_STATE_ATTACK_HIT:
    case ANIM_STATE_ATTACK_MISS:
      return DetermineAttackerSequence(COMBAT_MAINHAND);
    case ANIM_STATE_ATTACKOFF_HIT:
    case ANIM_STATE_ATTACKOFF_MISS:
      return DetermineAttackerSequence(COMBAT_OFFHAND);
    case ANIM_STATE_PARRY:
      return DetermineParrySequence();
    case ANIM_STATE_BLOCK:
      return ANIM_PARRYSHIELD;
    case ANIM_STATE_LOOTBEGIN:
    case ANIM_STATE_LOOTEND:
      return ANIM_LOOT;
    case ANIM_STATE_EMOTE:
      return GetEmoteAnimation(m_emoteID);
    case ANIM_STATE_ATTACK_READY: {
      unsigned int sequence = GetReadySequence();
      if (ModelHasSequenceId(GetObjectModel(), sequence)) {
        return sequence;
      }

      ReportMissingAnimation(sequence, GetModelFileName());
      return GetStandStateAnim(0);
    }
    case ANIM_STATE_SPECIALMOUNTANIM:
      return ANIM_MOUNT_SPECIAL;
    case ANIM_STATE_SITDOWN:
      return ANIM_SITDOWN;
    case ANIM_STATE_SITTING:
      return ANIM_SITTING;
    case ANIM_STATE_SITUP:
      return ANIM_SITUP;
    case ANIM_STATE_SLEEPDOWN:
      return ANIM_SLEEPDOWN;
    case ANIM_STATE_SLEEPING:
      return ANIM_SLEEPING;
    case ANIM_STATE_SLEEPUP:
      return ANIM_SLEEPUP;
    case ANIM_STATE_SITCHAIRLOW:
      return ANIM_SITCHAIRLOW;
    case ANIM_STATE_SITCHAIRMEDIUM:
      return ANIM_SITCHAIRMEDIUM;
    case ANIM_STATE_SITCHAIRHIGH:
      return ANIM_SITCHAIRHIGH;
    case ANIM_STATE_KNEELDOWN:
      return ANIM_KNEELDOWN;
    case ANIM_STATE_KNEELING:
      return ANIM_KNEELING;
    case ANIM_STATE_KNEELUP:
      return ANIM_KNEELUP;
    case ANIM_STATE_SPELLIMPACT:
      return m_pendingImpactAnim;
    default:
      return GetStandStateAnim(0);
  }
}

void CGUnit_C::OnBadAttackFacing(unsigned __int64 victimGUID) {
  m_hitInformation.attackFlags |= 2;
}

int GetForcedAnimIndex(const char *token) {
  for (unsigned int i = 0; i < 8; ++i) {
    if (!SStrCmp(s_forcedAnimations[i].name, token, 0x7FFFFFFF)) {
      return i;
    }
  }
  return -1;
}

int ParseForcedAnimCommandLine(const char *arguments, unsigned int *fidgetIndex) {
  if (!arguments || !*arguments || !fidgetIndex) {
    return -1;
  }

  char         tokenBuffer[128];
  const char  *stringBuffer = arguments;
  unsigned int foundFidget = 0;
  int          anim = -1;
  int          token = 0;
  do {
    SStrTokenize(&stringBuffer, tokenBuffer, sizeof(tokenBuffer), " ", 0);
    if (!token) {
      anim = GetForcedAnimIndex(tokenBuffer);
      if (anim == -1) {
        return -1;
      }
      token = 1;
    } else if (token == 1) {
      foundFidget = SStrToInt(tokenBuffer);
      token = 2;
    }
  } while (*stringBuffer);

  *fidgetIndex = foundFidget;
  return anim;
}

void CGUnit_C::SetForcedAnimation(const char *string) {
  unsigned int animVariation;
  int          animIndex = ParseForcedAnimCommandLine(string, &animVariation);
  if (animIndex == -1) {
    return;
  }

  HMODEL model = m_model;
  FATALASSERT(model);
  ANIMENUMERATION anim = s_forcedAnimations[animIndex].anim;
  if (!ModelGetNumSequenceFidgets(model, anim)) {
    return;
  }

  unsigned int &flags = m_animFlags;
  flags |= 1;
  if (s_forcedAnimations[animIndex].flag) {
    flags |= 2;
  } else {
    flags &= ~2u;
  }
  ModelSetSequenceFidget(model, anim, animVariation, 0);

  switch (animIndex) {
    case 1:
      PlayUnitSound(UNITSOUNDTYPE_DEATH, 0);
      break;
    case 4:
    case 6:
      PlayUnitSound(
          s_forcedAnimations[animIndex].flag ? UNITSOUNDTYPE_EXERTIONCRITICAL : UNITSOUNDTYPE_EXERTION,
          1
      );
      break;
    case 5:
    case 7:
      PlayUnitSound(
          s_forcedAnimations[animIndex].flag ? UNITSOUNDTYPE_INJURYCRITICAL : UNITSOUNDTYPE_INJURY,
          1
      );
      break;
  }
}

void CGUnit_C::ResetForcedAnimation() {
  m_animFlags &= ~3u;
  ForceUpdateBaseAnimation();
}

void CGUnit_C::ForceUpdateBaseAnimation() {
  UpdateBaseAnimation(GetAnimationState(), 0x100);
}

void CGUnit_C::OnBadAttackPosition(unsigned __int64 victimGUID, float range) {
}

void ClearSpecialEffects(HMODEL model) {
  static unsigned int linkPoints[] = {19, 20, 21, 22, -1, 17, 34, 23, 24, 25, 16, 15};

  FATALASSERT(model);
  for (unsigned int i = 0; i < 12; ++i) {
    ModelClearLink(model, linkPoints[i]);
  }
  ModelSetVertexAlpha(model, 255, 1);
  ModelSetVertexColor(model, 255, 255, 255, 1);
  ModelRemoveObjectLookAt(model, 6);
  ModelRemoveObjectFaceDir(model, 4);
  ModelRemoveObjectFaceDir(model, 5);
}

void CGUnit_C::StopSpellFizzleTimer(int spellID, unsigned char status) {
  int castingSpell = m_castingSpell;
  if (castingSpell && spellID == castingSpell) {
    unsigned int timer = m_spellFizzleTimer;
    ClientKillTimer(timer, SpellFizzleTimer, "SpellFizzleTimer");
    EndSpellEffects(status);
  }
}

void CGUnit_C::StartSpellFizzleTimer(int spellID, unsigned int castingTime, int animSet) {
  StopSpellFizzleTimer(m_castingSpell, 2);
  m_spellFizzleTimer = ClientSetTimer(castingTime, SpellFizzleTimer, this);
  SetCastingSpell(spellID, 1, animSet != 0);
}

void CGUnit_C::SpellDelayed(int delay) {
  if (m_spellFizzleTimer && delay) {
    float remaining = EventGetRemainingTime(m_spellFizzleTimer);
    ClientKillTimer(m_spellFizzleTimer, SpellFizzleTimer, "SpellFizzleTimer");
    m_spellFizzleTimer =
        ClientSetTimer(delay + static_cast<unsigned int>(remaining * 1000.0f), SpellFizzleTimer, this);
  }
}

void CGUnit_C::EndSpellEffects(unsigned char status) {
  int castingSpell = m_castingSpell;
  m_spellFizzleTimer = 0;

  if (castingSpell) {
    UnitEffectClearSpellPrecast(this, castingSpell);
    SetCastingSpell(0, true, false);
  }

  KillSpellLoopedSound();
  unsigned int torsoState = m_currentTorsoAnimState;
  if (torsoState == 37) {
    unsigned int torsoAnim = GetCurrentTorsoAnim();
    if (torsoAnim == 46 || torsoAnim == 49) {
      RangedWeaponAnimEndHandler();
    } else if (torsoAnim == 107) {
      ThrowAnimEndHandler();
    } else {
      const SpellVisualKitRec *kit = GetRangedSpellAnim(castingSpell, true);
      if (status || !kit || !kit->m_anim) {
        ClearTorsoAnimation(0x40);
      }
    }
  }

  FATALASSERT(!m_spellFizzleTimer);
}

void CGUnit_C::AddDeathHold() {
  ++m_deathHolds;
}

unsigned __int64 CGUnit_C::GetUnitBeingLooted() const {
  return 0;
}

void CGUnit_C::GetSwimMatrix(NTempest::C34Matrix *worldMatrix) const {
  NTempest::C33Matrix rotation;
  rotation.FromEulerAnglesZYX(-(6.2831855f - GetRenderFacing()), 6.2831855f - m_move.m_pitch, 0.0f);

  *worldMatrix = NTempest::C34Matrix(
      rotation.a0, rotation.a1, rotation.a2,
      rotation.b0, rotation.b1, rotation.b2,
      rotation.c0, rotation.c1, rotation.c2,
      0.0f, 0.0f, 0.0f
  );
  worldMatrix->Scale(GetScale() * GetRenderScale());
  *worldMatrix->Row3AsVec3() = GetPosition();
}

void CGUnit_C::GetWorldMatrix(NTempest::C34Matrix *worldMatrix) const {
  HMODEL model = GetObjectModel();
  float  blendRatio;

  if (m_currentBaseAnim == 1 || m_currentBaseAnim == 99) {
    blendRatio = ModelGetPrimarySequenceCompletion(model);
  } else if (
      (m_move.m_spline && !(m_move.m_spline->flags & 4) && (m_move.m_spline->flags & 0x200)) ||
      m_currentBaseAnim == 100
  ) {
    blendRatio = 1.0f;
  } else if (m_currentBaseAnim == 101) {
    blendRatio = 1.0f - ModelGetPrimarySequenceCompletion(model);
  } else {
    if ((m_move.m_moveFlags & 0x02000000) && (m_move.m_moveFlags & 1)) {
      GetSwimMatrix(worldMatrix);
    } else {
      ModelGetStandingMatrix(model, GetPosition(), GetGroundNormal(), GetRenderFacing(), GetScale() * GetRenderScale(), worldMatrix);
    }
    return;
  }

  ModelForceStandingMatrix(
      model, GetPosition(), GetGroundNormal(), GetRenderFacing(), GetScale() * GetRenderScale(), 2, blendRatio, worldMatrix
  );
}

void CGUnit_C::DelDeathHold() {
  if (!m_deathHolds) {
    return;
  }

  m_displayHealth = m_unit->health;
  SignalDisplayHealthUpdate();
  --m_deathHolds;
  if (!m_unit->health) {
    m_lastDeathTime = OsGetAsyncTimeMs();
  }

  if (!m_deathHolds) {
    m_deathHoldBufferIndices.SetCount(0);
    m_deathHoldBuffer.SetCount(0);
    if (m_unit->health <= 0) {
      if (!(m_animFlags & 0x2000)) {
        OnDeathAnimate();
        CGGameUI::ClearTarget(GetGUID(), 1);
      }
      if (m_unit->dynamicFlags & 1) {
        int effect = UnitEffectGetSpecialVisual(SPECIALEFFECT_LOOTART);
        if (effect >= 0) {
          MaybeAttachAura(UNITEFFECT_ATTACHBASE, effect, 0, 100, 1);
        }
      }
    }
  }
}

void CGUnit_C::SaveQuestAddItemMessage(int killed, int needed) {
  FATALASSERT(needed != -1);
  ProcessQuestItemMessages();
  m_questCountNeeded = needed;
  m_questCountKilled = killed;
}

void CGUnit_C::ProcessQuestItemMessages() {
  if (m_questCountNeeded == -1) {
    return;
  }

  const CreatureStats_C *stats = g_creatureDBCache.GetRecord(GetEntryID(), 0, 0, 0);
  if (stats) {
    CGGameUI::DisplayError(
        GERR_QUEST_ADD_KILL_SII, stats->m_name[FrameScript_GetPluralIndex(m_questCountNeeded)], m_questCountKilled, m_questCountNeeded
    );
  } else {
    OsOutputDebugString("Error\n");
  }
  m_questCountNeeded = -1;
}

static bool IsCombatSwingSpell(int spellID) {
  const SpellRec *spellRec = g_spellDB.GetRecord(spellID);
  return spellRec && (spellRec->m_attributes & 0x404);
}

void CGUnit_C::AddDamageDone(unsigned int damage, int normalCombatDamage, unsigned int flags, unsigned __int64 attacker, int spellID) {
  if (attacker != ClntObjMgrGetActivePlayer()) {
    return;
  }

  CGUnit_C *attackerPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(attacker, __FILE__, __LINE__));
  FATALASSERT(attackerPtr);
  if (IsCombatSwingSpell(spellID) || ((flags & 4) && attackerPtr->m_combat.IsAttacking())) {
    CGPlayer_C::AddDeferredDamage(normalCombatDamage, flags, damage, GetGUID());
  } else if (flags & 2) {
    AddWorldText(WORLDTEXTMISS_ABSORBED);
  } else if (damage) {
    if (flags & 1) {
      AddWorldCritText(damage, normalCombatDamage);
    } else {
      AddWorldDamageText(damage, normalCombatDamage);
    }
  }
}

void CGUnit_C::SpellEventHit() {
  SetVictimAnimation(VS_WOUND, m_unit->health <= 0, 0, 1000, 0);
}

void CGUnit_C::ProcessLocalMoveEvent(NETMESSAGE msgId) {
  if (!(m_move.m_moveFlags & 0xF) && (msgId == MSG_MOVE_STOP || msgId == MSG_MOVE_STOP_STRAFE)) {
    UpdateBaseAnimation(ANIM_STATE_STOP, 0);
  } else if (msgId == MSG_MOVE_SET_FACING) {
    if (fabs(GetFacing() - m_lastSentFacing) < 0.1f) {
      return;
    }
  } else if (msgId != MSG_MOVE_SET_PITCH) {
    UpdateBaseAnimation(0);
  }

  if (msgId != MSG_MOVE_SET_PITCH || fabs(m_move.m_pitch - m_lastSentPitch) >= 0.1f) {
    SendMovementUpdate(msgId);
  }

  switch (msgId) {
    case MSG_MOVE_START_FORWARD:
    case MSG_MOVE_START_BACKWARD:
    case MSG_MOVE_START_STRAFE_LEFT:
    case MSG_MOVE_START_STRAFE_RIGHT:
    case MSG_MOVE_JUMP:
      StartMoveHeartbeatTimer();
      break;

    case MSG_MOVE_STOP:
    case MSG_MOVE_STOP_STRAFE:
      if (!(m_move.m_moveFlags & 0x40FF)) {
        StopMoveHeartbeatTimer();
      }
      if (m_flags & 8) {
        ChangeStandState(2);
        m_flags &= ~8;
      }
      break;

    case MSG_MOVE_START_SWIM:
      if (GetType() & TYPE_PLAYER) {
        static_cast<CGPlayer_C *>(this)->TrySheathingWeapon();
      }
      break;
  }
}

void CGUnit_C::OnPendingMoveStateChange(NETMESSAGE msgId) {
  if (!(m_move.m_moveFlags & 0xF) && (msgId == MSG_MOVE_STOP || msgId == MSG_MOVE_STOP_STRAFE)) {
    UpdateBaseAnimation(ANIM_STATE_STOP, 0);
  } else {
    UpdateBaseAnimation(0);
  }

  if (GetGUID() == m_activeMover) {
    unsigned int unitFlags = m_unit->flags;
    if ((unitFlags & 0x01000000) ||
        ((GetType() & TYPE_PLAYER) && !m_unit->charmedBy && ((unitFlags & 2) || !(unitFlags & 0x00C00004)) && !(unitFlags & 1)))
    {
      SendMovementUpdate(msgId);
    }
  }
}

void CGUnit_C::OnCollideFalling(unsigned long eventTime) {
  UpdateBaseAnimation(0);
  if (GetGUID() == m_activeMover && m_move.m_jumpVelocity == 0.0f) {
    UpdateLocalPlayerFallState(1);
  }
}

void CGUnit_C::OnCollideFallLand(unsigned long eventTime) {
  if (m_move.m_moveFlags & 0xF) {
    UpdateBaseAnimation(0);
  } else if (!IsInStandSitTransition()) {
    UpdateBaseAnimation(ANIM_STATE_JUMP_LANDING, 0);
  }

  PlayUnitSound(UNITSOUNDTYPE_JUMPEND, 1);

  if (GetGUID() == m_activeMover && !(m_move.m_moveFlags & 0x40FF)) {
    UpdateLocalPlayerFallState(0);
    SendMovementUpdate(MSG_MOVE_HEARTBEAT);
  }

  if (GetGUID() == ClntObjMgrGetActivePlayer() && (m_animFlags & 0x2000)) {
    CGGameUI::UpdateActivePlayer();
  }
}

bool CGUnit_C::IsSlotComponented(unsigned int offset, int ignoreUsingRangedWeapon) {
  if (offset >= 23) {
    return 0;
  }
  if (ignoreUsingRangedWeapon || offset == 15 || offset == 16) {
    return 1;
  }
  if (offset != 17) {
    return 1;
  }
  return m_unit->weaponMode == WEAPONMODE_RANGEDMODE;
}

float CGUnit_C::DetermineWalkRunTimeScale(int currentState) {
  if (!(m_move.m_moveFlags & 0xF)) {
    return 1.0f;
  }

  float  movementSpeed = static_cast<CMovement &>(m_move).GetCurrentSpeed();
  HMODEL model = m_model;
  float  animMovementSpeed = 0.0f;
  ModelGetSequenceMoveSpeed(model, ChooseAnimation(currentState), &animMovementSpeed);
  if (NTempest::CMath::fabs_(animMovementSpeed) < 0.00000023841858f) {
    return 0.0f;
  }
  float modelScale = GetScale();
  FATALASSERT(modelScale > 0.0f);
  return movementSpeed / (NTempest::CMath::fabs_(animMovementSpeed) * modelScale);
}

void CGUnit_C::UpdateMovementAnimSpeed(int forMount, int currentState) {
  if (currentState == INVALID_ANIM_STATE) {
    currentState = m_currentBaseAnimState;
  }
  if ((m_unit->flags & 0x2000) && forMount) {
    currentState = m_currentMountAnimState;
  }

  float scale = 1.0f;
  if (s_animInfo[currentState].flags & 0x100) {
    scale = fabs(DetermineWalkRunTimeScale(currentState));
  }
  unsigned int animFlags = s_animInfo[currentState].flags;
  if ((animFlags & 0x40) || ((animFlags & 0x800) && !(m_animFlags & 4))) {
    scale = -scale;
  }
  ModelSetTimeScale(m_model, scale, 0);
}

void CGUnit_C::AttachVirtualComponent(unsigned int slot, bool deferApply) {
  FATALASSERT(!IsA(TYPE_PLAYER));
  FATALASSERT(slot < NUM_VIRTUAL_MONSTER_SLOTS);

  int  invSlot;
  bool showHidden = false;
  switch (slot) {
    case VIRTUAL_MONSTER_SLOT_MAINHAND:
      invSlot = INVSLOT_MAINHAND;
      showHidden = (m_unit->flags & 0x200000) != 0;
      break;
    case VIRTUAL_MONSTER_SLOT_OFFHAND:
      invSlot = INVSLOT_OFFHAND;
      break;
    case VIRTUAL_MONSTER_SLOT_RANGED:
      invSlot = INVSLOT_RANGED;
      showHidden = m_unit->weaponMode != WEAPONMODE_RANGEDMODE;
      break;
    default:
      return;
  }

  const VirtualItemInfo *itemInfo = GetVirtualItem(slot, 0);
  if (!itemInfo) {
    return;
  }

  const ItemSubClassRec *subClass = SDBItemSubclassGetSubClassRec(itemInfo->m_classID, itemInfo->m_subclassID);
  bool                   forceAlternate = subClass && (subClass->m_flags & 0x20) != 0;
  int                    sheathedAttachmentPoint = SheatheTypeToSheathePoint(itemInfo->m_sheatheType, invSlot);
  int                    inventoryType = itemInfo->m_inventoryType;

  AddObjectComponentBySlot(
      invSlot,
      GetVirtualItemDisplayID(slot),
      inventoryType,
      forceAlternate,
      deferApply,
      false,
      sheathedAttachmentPoint,
      showHidden
  );
}

void CGUnit_C::AttachVirtualMonsterWeapons() {
  if (IsA(TYPE_PLAYER)) {
    return;
  }

  for (unsigned int slot = 0; slot < NUM_VIRTUAL_MONSTER_SLOTS; ++slot) {
    if (m_unit->virtualItemDisplay[slot]) {
      AttachVirtualComponent(slot, slot == VIRTUAL_MONSTER_SLOT_RANGED);
    }
  }

  SetSheatheReason(SHEATHE_PLAYEREXPLICIT, m_unit->weaponMode == WEAPONMODE_SHEATHEDMODE, 1);
  if (m_unit->virtualItemDisplay[VIRTUAL_MONSTER_SLOT_RANGED]) {
    SetSheatheReason(
        SHEATHE_RANGED,
        m_unit->weaponMode == WEAPONMODE_RANGEDMODE && m_sheatheReasons,
        true
    );
  }
}

void CGUnit_C::RenderDebugPathing() {
  static NTempest::CImVector green(0xFF00FF00);
  static NTempest::CImVector white(0xFFFFFFFF);

  TSGrowableArray<NTempest::C3Vector> &points =
      m_debugPathPoints;
  unsigned int &numPathNodes = m_numDebugPathNodes;

  GxRsPush();
  if (m_move.m_spline && !(m_move.m_spline->flags & 4) && (m_move.m_spline->flags & 0x200)) {
    HTEXTURE  greenTex = TextureCreateSolid(green, 0);
    HMODEL    sphere = ModelCreateSolidSphere(0.5f, greenTex);
    CGCamera *camera = CGWorldFrame::GetActiveCamera();
    FATALASSERT(camera);

    NTempest::C3Vector cameraPos;
    CGWorldFrame::GetCameraPosition(&cameraPos);
    for (unsigned int i = 0; i < numPathNodes; ++i) {
      NTempest::C3Vector zero(0.0f);
      NTempest::C3Vector up(0.0f, 0.0f, 1.0f);
      NTempest::C3Vector position = points[i] - cameraPos;
      ModelAnimate(sphere, position, 0.0f, up, 1.0f, zero, zero);
      ModelRender(sphere, 0, 0);
    }

    GxRsSet(GxRs_Blend, 0);
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_Texture0, TextureGetGxTex(greenTex, 1, 0));
    GxVertexShaderSelect(GxVS_PassThru);

    NTempest::C44Matrix worldToCamera;
    worldToCamera.Translate(-cameraPos);
    GxXformPush(GxXform_World, worldToCamera);

    const unsigned int        count = points.Count() - numPathNodes;
    const NTempest::C3Vector *linePoints = &points[numPathNodes];
    GxPrimLockVertexPtrs(count, linePoints, sizeof(*linePoints), 0, 0, &white, 0, 0, 0, 0, 0, 0, 0);
    unsigned short *indices = static_cast<unsigned short *>(_alloca(count * sizeof(*indices)));
    for (unsigned int index = 0; index < count; ++index) {
      indices[index] = static_cast<unsigned short>(index);
    }
    GxPrimDrawElements(GxPrim_LineStrip, count, indices);
    GxPrimUnlockVertexPtrs();
    GxXformPop(GxXform_World);
    HandleClose(sphere);
    HandleClose(greenTex);
  } else {
    const NTempest::C3Vector  halfExtents(0.5f, 0.5f, 1.0f);
    const NTempest::C3Vector &serverLoc = m_serverLoc;
    NTempest::CAaBox          worldBox(serverLoc - halfExtents, serverLoc + halfExtents);
    ProjectTex2d(worldBox, green, 0, 0.5f);

    for (unsigned int pointIndex = points.Count(); pointIndex; --pointIndex) {
      const NTempest::C3Vector &point = points[pointIndex - 1];
      worldBox.b = point - halfExtents;
      worldBox.t = point + halfExtents;
      NTempest::CImVector color(0xFFFF0000);
      ProjectTex2d(worldBox, color, 0, 0.5f);
    }
  }
  GxRsPop();
}

void CGUnit_C::PreAnimate(CGWorldFrame *worldFrame) {
  FATALASSERT(worldFrame);
  unsigned int currentTime = OsGetAsyncTimeMs();

  if (s_drawNameplates && !(m_obj->m_type & TYPE_PLAYER) && m_unit->health > 0) {
    CGCamera *camera = CGWorldFrame::GetActiveCamera();
    CGObject_C *target =
        static_cast<CGObject_C *>(ClntObjMgrObjectPtr(camera->GetTarget(), __FILE__, __LINE__));
    if (target) {
      NTempest::C3Vector targetPosition = target->GetPosition();
      NTempest::C3Vector position = GetPosition();
      if ((position - targetPosition).SquaredMag() < MAX_NAMEPLATE_DIST_SQ) {
        AddUnitNamePlate(worldFrame);
      }
    }
  }

  if (CGGameUI::GetLockedTarget() == GetGUID()) {
    CGPlayer_C *player =
        static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player && player->IsInCombatMode()) {
      m_flags |= 0x200;
      if (static_cast<int>(currentTime - s_lastTargetFlashTime - 500) >= 0) {
        s_lastTargetFlashTime = currentTime;
        s_targetPulseDirection = !s_targetPulseDirection;
      }

      int   timePassed = s_lastTargetFlashTime - currentTime + 500;
      float pulse = static_cast<float>(timePassed) * 0.002f;
      if (s_targetPulseDirection) {
        pulse = 1.0f - pulse;
      }
      s_targetFlashColor.g = static_cast<unsigned char>(pulse * 128.0f);
      PlayerNameTriggerColorUpdate(m_unitNameHandle);
    }
  } else if (m_flags & 0x200) {
    m_flags &= ~0x200U;
    PlayerNameTriggerColorUpdate(m_unitNameHandle);
  }

  for (BLOODSPLATNODE *node = m_bloodSplatNodes.Head(); node; ) {
    if (static_cast<int>(currentTime - node->m_triggerTime) < 0) {
      break;
    }

    BLOODSPLATNODE *next = node->Next();
    UnitFootprintNewBloodSplat(GetBloodRecord(), GetUnitSize(), node->m_position);
    node->Unlink();
    s_bloodSplatList.LinkNode(node, LIST_TAIL, 0);
    node = next;
  }

  if (m_flags & 8) {
    m_flags = (m_flags & ~8U) | 0x80000;
    if (m_castingSpell && m_ammoDisplayID) {
      const ItemDisplayInfoRec *displayInfo = g_itemDisplayInfoDB.GetRecord(m_ammoDisplayID);
      unsigned int        sequenceDuration;
      HMODEL              ammoModel = ObjComponentBuildAmmoModel(displayInfo, m_ammoInvType, sequenceDuration);
      if (ammoModel) {
        HMODEL charModel = GetCharacterModel(0);
        if (charModel) {
          ModelAddLink(charModel, 35, ammoModel, 1.0f);
          HandleClose(charModel);
        }
        HandleClose(ammoModel);
      }
    }
  }

  CGObject_C::PreAnimate(worldFrame);
}

void CGUnit_C::BuildSelectionRotMatrix(NTempest::C44Matrix &matrix) const {
  matrix = NTempest::C44Matrix();
  if (CGWorldFrame::GetActive()) {
    NTempest::C3Vector pos;
    CGWorldFrame::GetCameraPosition(&pos);
    matrix.Rotate(-CalculateFacingTo(pos, GetPosition()), NTempest::C3Vector(0.0f, 0.0f, 1.0f), 1);
  }
}

void CGUnit_C::ObjectPostAnimate(
    const NTempest::C34Matrix &,
    const NTempest::C3Vector &cameraPos,
    const NTempest::C3Vector &cameraTarg
) {
  if (m_fadingPureMountModel) {
    UpdateFadingMountModel(cameraPos, cameraTarg);
  }
}

void CGUnit_C::UpdateFadingMountModel(
    const NTempest::C3Vector &cameraPos,
    const NTempest::C3Vector &cameraTarg
) {
  FATALASSERT(m_fadingPureMountModel);

  int elapsed = OsGetAsyncTimeMs() - m_pureMountFadeStartTime;
  if (elapsed > 1000) {
    if (m_pureMountFadeMode == PUREMOUNTFADE_OUT) {
      HandleClose(m_fadingPureMountModel);
      m_fadingPureMountModel = 0;
      return;
    }
    elapsed = 1000;
  }

  float alpha = min(static_cast<float>(elapsed) * 0.001f, 1.0f);
  if (m_pureMountFadeMode == PUREMOUNTFADE_OUT) {
    alpha = 1.0f - alpha;
  }
  ModelSetVertexAlpha(m_fadingPureMountModel, static_cast<unsigned char>(alpha * 255.0f), 1);

  if (ModelAdvanceTime(m_fadingPureMountModel)) {
    NTempest::C3Vector rotationAxis(0.0f, 0.0f, 1.0f);
    NTempest::C3Vector cameraVector = cameraTarg - cameraPos;
    ModelAnimate(
        m_fadingPureMountModel,
        m_fadingMountPos,
        m_fadingMountFacing,
        rotationAxis,
        m_fadingMountScale,
        cameraPos,
        cameraVector
    );
    ModelAddToScene(m_fadingPureMountModel, 0);
  }
}

void CGUnit_C::UpdatePlayerName() {
  if (!m_unitNameHandle) {
    return;
  }

  HMODEL charModel = GetCharacterModel(0);
  FATALASSERT(charModel);

  NTempest::C3Vector namePosition(0.0f);
  unsigned int       objectID = 22 | ((m_unit->flags >> 13) & 1);
  if (!ModelAnimHasObjectId(charModel, objectID)) {
    objectID = 22;
  }

  ModelGetModelSpacePivot(charModel, objectID, &namePosition);
  PlayerNameChangeLocation(m_unitNameHandle, namePosition);
  PlayerNameTriggerColorUpdate(m_unitNameHandle);
  HandleClose(charModel);
}

void CGUnit_C::InitializeUnitName() {
  if (m_unitNameHandle) {
    HandleClose(reinterpret_cast<HOBJECT>(m_unitNameHandle));
  }
  m_unitNameHandle = PlayerNameCreate(this);
}

void CGUnit_C::InitializeSequenceFlags() {
  m_animFlags &= 0xFFF7FF9B;
  if (ModelHasSequenceId(m_model, 13)) {
    m_animFlags |= 4;
  }
  if (ModelHasSequenceId(m_model, 92)) {
    m_animFlags |= 0x20;
  }
  if (ModelHasSequenceId(m_model, 93)) {
    m_animFlags |= 0x40;
  }
  if (ModelHasSequenceId(m_model, 14)) {
    m_animFlags |= 0x80000;
  }
}

void CGUnit_C::MarkSwimAnimations() {
  m_animFlags &= 0xFFFFF87F;
  if (ModelHasSequenceId(m_model, 44)) {
    m_animFlags |= 0x80;
  }
  if (ModelHasSequenceId(m_model, 43)) {
    m_animFlags |= 0x100;
  }
  if (ModelHasSequenceId(m_model, 45)) {
    m_animFlags |= 0x200;
  }
  if (ModelHasSequenceId(m_model, 131)) {
    m_animFlags |= 0x400;
  }
}

void CGUnit_C::UpdateBaseRadius(HMODEL model) {
  NTempest::CAaSphere bounds;
  ModelGetBounds(model, &bounds);
  m_baseRadius = bounds.r * 0.5f;
}

void CGUnit_C::QueryModelStats() {
  if (m_combat.IsAttacking()) {
    DetermineReadySequence(0);
    ObjectModelSetSequence(m_model, m_readySequence, 0, GetModelFileName());
  }

  UpdateUnitCollisionBox(m_model, GetModelFileName());
  UpdateBaseRadius(m_model);
  MarkSwimAnimations();

  if (ModelHasLinkPoint(m_model, 29)) {
    m_flags &= ~0x1000U;
  } else {
    m_flags |= 0x1000;
  }

  if (ModelAnimHasObjectId(m_model, 18)) {
    m_flags |= 0x2000;
  } else {
    m_flags &= ~0x2000U;
  }
}

void CGUnit_C::QueryMountModelStats() {
  if (ModelAnimHasObjectId(m_model, 18)) {
    m_flags |= 0x4000;
  } else {
    m_flags &= ~0x4000U;
  }

  if (!ModelHasLinkPoint(m_model, 0)) {
    SysMsgPrintf(SYSMSG_ERROR, 0x10, "MOUNTDISPLAYIDNOMOUNTATTACHMENT|%d", m_unit->mountDisplayID);
  }
}

int CGUnit_C::UpdateModelLoadStatus() {
  if (!CGObject_C::UpdateModelLoadStatus()) {
    return 0;
  }

  if (m_castingSpell && m_deferredPrecastAnim != RESET_ANIMATION_INDICES0) {
    SetSpellPreCastingAnimation(m_deferredPrecastAnim);
    SetTorsoAnimation(37, 0, 0);
    m_deferredPrecastAnim = RESET_ANIMATION_INDICES0;
  }

  if (m_flags & 0x40) {
    HMODEL charModel = GetCharacterModel(0);
    RefreshAttachmentInfo(charModel);
    HandleClose(charModel);
    m_flags &= ~0x40U;
  }

  InitializeSequenceFlags();
  if (m_flags & 0x10) {
    QueryMountModelStats();
    UpdateMovementAnimSpeed(1, INVALID_ANIM_STATE);
  } else {
    QueryModelStats();
    UpdateMovementAnimSpeed(0, INVALID_ANIM_STATE);
  }

  CGGameUI::UnitPortraitUpdate(GetGUID());
  if (!m_unitNameHandle) {
    InitializeUnitName();
  }
  UpdatePlayerName();
  return 1;
}

void CGUnit_C::RenderTargetSelection() const {
  NTempest::C3Vector  position = GetPosition();
  float               radius = m_baseRadius * m_obj->m_scale;
  NTempest::C3Vector  extents(radius, radius, radius + radius);
  NTempest::CAaBox    worldBox(position - extents, position + extents);
  NTempest::CImVector color;
  GetSelectionHighlightColor(&color);

  NTempest::C44Matrix matrix;
  BuildSelectionRotMatrix(matrix);

  GxRsPush();
  SetCircleRenderStates();
  ProjectTex2d(worldBox, color, &matrix, 0.5f);
  GxRsPop();
}

int CGUnit_C::GetSelectionHighlightColor(NTempest::CImVector *outPtr) const {
  static const NTempest::CImVector reactionTypeColors[NUM_UNIT_REACTIONS] = {
      NTempest::CImVector(0xFFFF0000),
      NTempest::CImVector(0xFFFF0000),
      NTempest::CImVector(255, 255, 128, 0),
      NTempest::CImVector(255, 255, 255, 0),
      NTempest::CImVector(255, 0, 255, 0),
      NTempest::CImVector(255, 0, 255, 0),
      NTempest::CImVector(255, 0, 255, 0)
  };
  static const NTempest::CImVector playerReactionColor(255, 96, 96, 255);
  static const NTempest::CImVector partyReactionColor(255, 170, 170, 255);

  CGUnit_C *currentPlayer =
      static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(currentPlayer);
  FATALASSERT(outPtr);

  UNIT_REACTION reaction = UnitReaction(currentPlayer);
  if (!(m_obj->m_type & TYPE_PLAYER) || m_unit->charmedBy || reaction <= UNIT_REACTION_HOSTILE) {
    *outPtr = (m_flags & 0x200) ? s_targetFlashColor : reactionTypeColors[reaction];
  } else if (m_flags & 0x200) {
    *outPtr = s_targetFlashColor;
  } else if (m_obj->m_guid == currentPlayer->GetGUID() || !CGPartyInfo::IsMember(m_obj->m_guid)) {
    *outPtr = playerReactionColor;
  } else {
    *outPtr = partyReactionColor;
  }

  return 1;
}

void CGUnit_C::OnLeftClick() {
  if ((m_obj->m_type & TYPE_PLAYER) || !m_NPCSoundsRec || !m_numNPCPissedSounds) {
    ++s_currentGlobalClickCount;
    return;
  }

  if (m_lastGlobalClickCount != s_currentGlobalClickCount) {
    m_pissedCount = 0;
  }

  if (m_pissedCount < 4) {
    if (!PlayNPCSound(NPCSOUND_ACK, 0)) {
      return;
    }
    ++m_pissedCount;
  } else {
    unsigned int pissedIndex = m_pissedCount - 4;
    if (pissedIndex < m_numNPCPissedSounds) {
      if (!PlayNPCSound(NPCSOUND_PISSED, pissedIndex)) {
        return;
      }
      ++m_pissedCount;
    } else {
      if (!PlayNPCSound(NPCSOUND_ACK, 0)) {
        return;
      }
      m_pissedCount = 1;
    }
  }

  m_lastGlobalClickCount = ++s_currentGlobalClickCount;
}

void CGUnit_C::OnRightClick() {
  CGPlayer_C *player =
      static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player || player->m_unit->health <= 0) {
    return;
  }

  if (player->m_flags & 0x800) {
    CGUnit_C *possessed = player->GetPossessedUnit();
    if (m_unit->health > 0 && possessed && possessed->m_unit->health > 0 &&
        !(possessed->m_unit->flags & 0x2000) && possessed->CanAttack(this)) {
      CGPetInfo::PetAttackTarget(GetGUID());
    }
    return;
  }

  if (m_unit->health > 0) {
    if (player->CanInteract(this)) {
      unsigned int npcFlags = m_unit->npcFlags;
      if (npcFlags & 1) {
        player->ShopFromMerchant(GetGUID());
      } else if (npcFlags & 4) {
        player->QueryTaxiNodes(GetGUID());
      } else if (npcFlags & 8) {
        player->TalkToTrainer(GetGUID());
      } else if (npcFlags & 0x10) {
        player->TalkToBinder(GetGUID());
      } else if (npcFlags & 0x20) {
        player->TalkToBanker(GetGUID());
      } else if (player->IsQuestUnit(this)) {
        player->TalkToQuestUnit(GetGUID());
      } else if ((npcFlags & 0xC0) == 0x40) {
        player->TalkToTabardVendor(GetGUID());
      } else if (npcFlags & 0x80) {
        player->TalkToNpcPetition(GetGUID());
      }
    } else if (player->CanAttack(this)) {
      if (player->m_unit->standState) {
        CGGameUI::DisplayError(GERR_CANTATTACK_NOTSTANDING);
      } else if (player->m_unit->health > 0 && !(player->m_unit->flags & 0x2000) &&
                 m_unit->health > 0 && player->CanAttack(this)) {
        player->SetCombatMode(1);
      }
    }
  } else if (!(player->m_unit->flags & 0x2000)) {
    if (player->m_unit->standState) {
      CGGameUI::DisplayError(GERR_LOOT_NOTSTANDING);
    } else {
      player->LootUnit(this);
    }
  }
}

bool CGUnit_C::FactionHasReputation(int faction) {
  const FactionRec *rec = g_factionDB.GetRecord(faction);
  if (!rec) {
    return false;
  }

  FATALASSERT(rec->m_reputationIndex < 64);
  return rec->m_reputationIndex >= 0;
}

int CGUnit_C::GetFactionTemplate() const {
  const unsigned __int64 &owner = m_unit->charmedBy ? m_unit->charmedBy : m_unit->createdBy;
  CGUnit_C               *ownerUnit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(owner, __FILE__, __LINE__));

  return ownerUnit ? ownerUnit->m_unit->factionTemplate : m_unit->factionTemplate;
}

bool CGUnit_C::CanAssist(const CGUnit_C *unit) const {
  if (UnitReaction(unit) <= UNIT_REACTION_HOSTILE) {
    return false;
  }

  if (!(m_unit->flags & 8)) {
    return true;
  }

  if (unit->m_unit->flags & 8) {
    const CGUnit_C         *player1 = this;
    const unsigned __int64 &owner1 = m_unit->charmedBy ? m_unit->charmedBy : m_unit->createdBy;
    if (owner1) {
      player1 = static_cast<const CGUnit_C *>(ClntObjMgrObjectPtr(owner1, __FILE__, __LINE__));
    }

    const CGUnit_C         *player2 = unit;
    const unsigned __int64 &owner2 = unit->m_unit->charmedBy ? unit->m_unit->charmedBy : unit->m_unit->createdBy;
    if (owner2) {
      player2 = static_cast<const CGUnit_C *>(ClntObjMgrObjectPtr(owner2, __FILE__, __LINE__));
    }

    const CGPlayer_C *playerObject1 = static_cast<const CGPlayer_C *>(player1);
    const CGPlayer_C *playerObject2 = static_cast<const CGPlayer_C *>(player2);
    if (player1 && (player1->m_obj->m_type & TYPE_PLAYER) && player2 && (player2->m_obj->m_type & TYPE_PLAYER) &&
            playerObject1->GetDuelTeam() ||
        playerObject2->GetDuelTeam()) {
      return playerObject1->GetDuelArbiter() == playerObject2->GetDuelArbiter() &&
             playerObject1->GetDuelTeam() == playerObject2->GetDuelTeam();
    }

    return UnitReaction(GetFactionTemplate(), unit, -1) >= UNIT_REACTION_NEUTRAL;
  }

  const MapRec *map = g_mapDB.GetRecord(CGPlayer_C::GetNewContinentID());
  return map && map->m_PVP != 0;
}

bool CGUnit_C::CanAttack(const CGUnit_C *unit) const {
  unsigned int flags = m_unit->flags;
  if (((flags & 8) && (unit->m_unit->flags & 0x100)) || (!(flags & 8) && (unit->m_unit->flags & 0x200)) ||
      ((flags & 0x100) && (unit->m_unit->flags & 8)) || ((flags & 0x200) && !(unit->m_unit->flags & 8)))
  {
    return false;
  }

  if (!(flags & 8) || !(unit->m_unit->flags & 8)) {
    return UnitReaction(unit) < UNIT_REACTION_AMIABLE ||
           (!(m_unit->flags & 8) && unit->UnitReaction(this) < UNIT_REACTION_AMIABLE);
  }

  const CGUnit_C         *player1 = this;
  const unsigned __int64 &owner1 = m_unit->charmedBy ? m_unit->charmedBy : m_unit->createdBy;
  if (owner1) {
    player1 = static_cast<const CGUnit_C *>(ClntObjMgrObjectPtr(owner1, __FILE__, __LINE__));
  }

  const CGUnit_C         *player2 = unit;
  const unsigned __int64 &owner2 = unit->m_unit->charmedBy ? unit->m_unit->charmedBy : unit->m_unit->createdBy;
  if (owner2) {
    player2 = static_cast<const CGUnit_C *>(ClntObjMgrObjectPtr(owner2, __FILE__, __LINE__));
  }

  if (player1 && (player1->m_obj->m_type & TYPE_PLAYER) && player2 && (player2->m_obj->m_type & TYPE_PLAYER)) {
    const CGPlayer_C *playerObject1 = static_cast<const CGPlayer_C *>(player1);
    const CGPlayer_C *playerObject2 = static_cast<const CGPlayer_C *>(player2);
    unsigned int      duelTeam1 = playerObject1->GetDuelTeam();
    unsigned int      duelTeam2 = playerObject2->GetDuelTeam();

    if (duelTeam1 || duelTeam2) {
      if (playerObject1->GetDuelArbiter() != playerObject2->GetDuelArbiter()) {
        return false;
      }
      return duelTeam1 != duelTeam2;
    }
  }

  return UnitReaction(unit) < UNIT_REACTION_AMIABLE ||
         (!(m_unit->flags & 8) && unit->UnitReaction(this) < UNIT_REACTION_AMIABLE);
}

bool CGUnit_C::CanCooperate(const CGUnit_C *unit) const {
  if (unit == this) {
    return false;
  }

  const FactionTemplateRec *faction = g_factionTemplateDB.GetRecord(GetFactionTemplate());
  const FactionTemplateRec *unitFaction = g_factionTemplateDB.GetRecord(unit->GetFactionTemplate());
  return faction && unitFaction && faction->m_factionGroup == unitFaction->m_factionGroup;
}

bool CGUnit_C::IsUnitInGroup(const CGUnit_C *unit) const {
  if (unit == this) {
    return 1;
  }

  if ((m_unit->flags & 8) && (unit->m_unit->flags & 8)) {
    const CGUnit_C         *player1 = this;
    const unsigned __int64 &owner1 = m_unit->charmedBy ? m_unit->charmedBy : m_unit->createdBy;
    if (owner1) {
      player1 = static_cast<const CGUnit_C *>(ClntObjMgrObjectPtr(owner1, __FILE__, __LINE__));
    }

    const CGUnit_C         *player2 = unit;
    const unsigned __int64 &owner2 = unit->m_unit->charmedBy ? unit->m_unit->charmedBy : unit->m_unit->createdBy;
    if (owner2) {
      player2 = static_cast<const CGUnit_C *>(ClntObjMgrObjectPtr(owner2, __FILE__, __LINE__));
    }

    if (!player1 || !(player1->GetType() & TYPE_PLAYER) || !player2 || !(player2->GetType() & TYPE_PLAYER)) {
      return 1;
    }

    if (player1->GetGUID() == ClntObjMgrGetActivePlayer() && CGGameUI::IsPartyMember(player2->GetGUID())) {
      return 1;
    }
    if (player2->GetGUID() == ClntObjMgrGetActivePlayer() && CGGameUI::IsPartyMember(player1->GetGUID())) {
      return 1;
    }
  }

  return 0;
}

void CGUnit_C::UpdatePlayerNameColor() {
  PlayerNameTriggerColorUpdate(m_unitNameHandle);
}

void CGUnit_C::TriggerPlayerNameUpdate() {
  if (m_unitNameHandle) {
    PlayerNameTriggerNameRegenerate(m_unitNameHandle);
  }
}

void CGUnit_C::PlayerNameVisibilityChanged(int nameVisible) {
  if (m_interactIconModel) {
    ModelSetSequence(m_interactIconModel, nameVisible ? 1 : 0, 0);
  }
}

static int IsInSitSleepPosition(unsigned int animState) {
  return animState < 64 ? (s_animInfo[animState].flags >> 19) & 1 : 0;
}

void CGUnit_C::AddWorldDamageText(unsigned int damage, int normalCombatDamage) {
  if (damage) {
    char buffer[32];
    SStrPrintf(buffer, sizeof(buffer), "%d", damage);
    HPLAYERNAME__ *name = m_unitNameHandle;
    PlayerNameCreateText(name, WT_DAMAGE, buffer, normalCombatDamage ? 0 : &COLOR_GOLD);
  }
}

void CGUnit_C::AddWorldCritText(unsigned int damage, int normalCombatDamage) {
  if (damage) {
    char buffer[32];
    SStrPrintf(buffer, sizeof(buffer), "%d", damage);
    HPLAYERNAME__ *name = m_unitNameHandle;
    PlayerNameCreateText(name, WT_CRIT, buffer, normalCombatDamage ? 0 : &COLOR_GOLD);
  }
}

void CGUnit_C::AddWorldXPGainText(int xpGain) {
  char buf[64];
  char buffer[64] = "";
  SStrCopy(buf, FrameScript_GetText("XP", -1, GENDER_NOT_APPLICABLE), sizeof(buf));
  SStrPrintf(buffer, sizeof(buffer), "%s: %d", buf, xpGain);
  HPLAYERNAME__ *name = m_unitNameHandle;
  PlayerNameCreateText(name, WT_XPGAIN, buffer, 0);
}

void CGUnit_C::RemoveInteractIcon() {
  if (m_interactIconModel) {
    HMODEL charModel = CGObject_C::GetCharacterModel(0);
    if (charModel) {
      ModelRemoveLink(charModel, 18, m_interactIconModel);
      ModelRemoveLink(charModel, 29, m_interactIconModel);
      HandleClose(charModel);
    }
    HandleClose(m_interactIconModel);
    m_interactIconModel = 0;
  }
}

HMODEL CGUnit_C::DuplicateCharacterModel(unsigned int flags) const {
  HMODEL characterModel = GetCharacterModel(0);
  HMODEL duplicate = ModelDuplicate(characterModel, flags);
  HandleClose(characterModel);
  return duplicate;
}

UNIT_REACTION CGUnit_C::UnitReaction(int factionID, const CGUnit_C *unit, int trueSight) {
  const FactionTemplateRec *source = g_factionTemplateDB.GetRecord(factionID);
  const FactionTemplateRec *target = g_factionTemplateDB.GetRecord(unit->GetFactionTemplate());

  if (!source || !target) {
    return UNIT_REACTION_NEUTRAL;
  }

  if (FactionHasReputation(source->m_faction)) {
    const unsigned __int64 &owner = unit->m_unit->charmedBy ? unit->m_unit->charmedBy : unit->m_unit->createdBy;
    const CGUnit_C         *ownerUnit = unit;
    if (owner) {
      ownerUnit = static_cast<const CGUnit_C *>(ClntObjMgrObjectPtr(owner, __FILE__, __LINE__));
    }
    if (ownerUnit && ownerUnit->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
      return CGReputationInfo::GetFactionStandingReaction(source->m_faction);
    }
  }

  if (target->m_factionGroup & source->m_enemyGroup) {
    return UNIT_REACTION_HOSTILE;
  }
  unsigned int i;
  for (i = 0; i < 4 && source->m_enemies[i]; ++i) {
    if (source->m_enemies[i] == target->m_faction) {
      return UNIT_REACTION_HOSTILE;
    }
  }

  if (target->m_factionGroup & source->m_friendGroup) {
    return UNIT_REACTION_FRIENDLY;
  }
  for (i = 0; i < 4 && source->m_friend[i]; ++i) {
    if (source->m_friend[i] == target->m_faction) {
      return UNIT_REACTION_FRIENDLY;
    }
  }

  if (target->m_friendGroup & source->m_factionGroup) {
    return UNIT_REACTION_FRIENDLY;
  }
  for (i = 0; i < 4 && target->m_friend[i]; ++i) {
    if (target->m_friend[i] == source->m_faction) {
      return UNIT_REACTION_FRIENDLY;
    }
  }

  return UNIT_REACTION_NEUTRAL;
}

UNIT_REACTION CGUnit_C::UnitReaction(const CGUnit_C *unit) const {
  if (unit == this) {
    return UNIT_REACTION_FRIENDLY;
  }

  if ((m_unit->flags & 8) && (unit->m_unit->flags & 8)) {
    const CGUnit_C         *player1 = this;
    const unsigned __int64 &owner1 = m_unit->charmedBy ? m_unit->charmedBy : m_unit->createdBy;
    if (owner1) {
      player1 = static_cast<const CGUnit_C *>(ClntObjMgrObjectPtr(owner1, __FILE__, __LINE__));
    }

    const CGUnit_C         *player2 = unit;
    const unsigned __int64 &owner2 = unit->m_unit->charmedBy ? unit->m_unit->charmedBy : unit->m_unit->createdBy;
    if (owner2) {
      player2 = static_cast<const CGUnit_C *>(ClntObjMgrObjectPtr(owner2, __FILE__, __LINE__));
    }

    if (!player1 || !(player1->m_obj->m_type & TYPE_PLAYER) || !player2 || !(player2->m_obj->m_type & TYPE_PLAYER)) {
      return UNIT_REACTION_FRIENDLY;
    }

    const CGPlayer_C *playerObject1 = static_cast<const CGPlayer_C *>(player1);
    const CGPlayer_C *playerObject2 = static_cast<const CGPlayer_C *>(player2);
    unsigned int      duelTeam1 = playerObject1->GetDuelTeam();
    unsigned int      duelTeam2 = playerObject2->GetDuelTeam();

    if (duelTeam1 && duelTeam2 && playerObject1->GetDuelArbiter() == playerObject2->GetDuelArbiter()) {
      return duelTeam1 == duelTeam2 ? UNIT_REACTION_FRIENDLY : UNIT_REACTION_HOSTILE;
    }

    if (player1->m_obj->m_guid == ClntObjMgrGetActivePlayer() && CGGameUI::IsPartyMember(player2->m_obj->m_guid)) {
      return UNIT_REACTION_FRIENDLY;
    }
    if (player2->m_obj->m_guid == ClntObjMgrGetActivePlayer() && CGGameUI::IsPartyMember(player1->m_obj->m_guid)) {
      return UNIT_REACTION_FRIENDLY;
    }

    if ((playerObject1->GetPlayerFlags() & 1) && (playerObject2->GetPlayerFlags() & 1)) {
      return UNIT_REACTION_HOSTILE;
    }

    const MapRec *map = g_mapDB.GetRecord(CGPlayer_C::GetNewContinentID());
    if (!map || !map->m_PVP) {
      return UNIT_REACTION_FRIENDLY;
    }
    if (map->m_PVP > 1) {
      return UNIT_REACTION_HOSTILE;
    }
  }

  if (m_unit->flags & 8) {
    const FactionTemplateRec *targetFaction = g_factionTemplateDB.GetRecord(unit->GetFactionTemplate());
    if (targetFaction && FactionHasReputation(targetFaction->m_faction)) {
      const unsigned __int64 &owner = m_unit->charmedBy ? m_unit->charmedBy : m_unit->createdBy;
      const CGUnit_C         *ownerUnit = this;
      if (owner) {
        ownerUnit = static_cast<const CGUnit_C *>(ClntObjMgrObjectPtr(owner, __FILE__, __LINE__));
      }
      if (ownerUnit && ownerUnit->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        return CGReputationInfo::IsAtWar(targetFaction->m_faction) ? UNIT_REACTION_HOSTILE : UNIT_REACTION_FRIENDLY;
      }
    }
  }

  return UnitReaction(GetFactionTemplate(), unit, -1);
}

HMODEL CGUnit_C::GetCharacterModel(int *mountedPtr) const {
  if (!(m_flags & 0x10)) {
    if (mountedPtr) {
      *mountedPtr = 0;
    }
    return static_cast<HMODEL>(HandleDuplicate(GetObjectModel()));
  }

  if (mountedPtr) {
    *mountedPtr = 1;
  }
  FATALASSERT(m_tempCharModel);
  return static_cast<HMODEL>(HandleDuplicate(m_tempCharModel));
}

bool CGUnit_C::CanInteract(const CGUnit_C *unit) const {
  return unit->m_unit->npcFlags && unit->UnitReaction(this) >= UNIT_REACTION_NEUTRAL && UnitReaction(unit) >= UNIT_REACTION_NEUTRAL;
}

bool CGUnit_C::CanInteract(const CGGameObject_C *object) const {
  return object->ObjectReaction(this) >= UNIT_REACTION_NEUTRAL;
}

HMODEL CGUnit_C::GetMountedModel() const {
  if (!(m_flags & 0x10)) {
    return 0;
  }
  return static_cast<HMODEL>(HandleDuplicate(m_model));
}

unsigned int CGUnit_C::GetRunSequence() const {
  if (!IsMounted()) {
    return ANIM_RUN;
  }

  unsigned int moveFlags = m_move.m_moveFlags;
  if ((moveFlags & 0x33) != 0x33) {
    return ANIM_RUN;
  }
  if ((moveFlags & 0x10) && (m_animFlags & 0x40)) {
    return ANIM_RUN_LEANLEFT;
  }
  if ((moveFlags & 0x20) && (m_animFlags & 0x20)) {
    return ANIM_RUN_LEANRIGHT;
  }

  return ANIM_RUN;
}

unsigned int CGUnit_C::GetStopSequence() const {
  HMODEL model = m_model;
  if (ModelHasSequenceId(model, ANIM_STOP)) {
    return ANIM_STOP;
  }
  unsigned int sequence = GetStandStateAnim(0);
  return ModelHasSequenceId(model, sequence) ? sequence : ANIM_STAND;
}

unsigned int CGUnit_C::GetRangedReadySequence() const {
  if (m_readySequence != ANIM_READYBOW && m_readySequence != ANIM_READYRIFLE) {
    return GetStandStateAnim(0);
  }
  return m_readySequence;
}

void CGUnit_C::UpdateRenderFacing() {
  CGWorldFrame *worldFrame = CGWorldFrame::GetActive();
  CGCamera     *camera = worldFrame->Camera();
  if (GetGUID() != camera->m_target) {
    UpdateSmoothFacing();
  }
  UpdateDisplayFacing();
  SetAnimated(1);
}

float CGUnit_C::GetRenderFacing() const {
  return m_move.GetFacing(m_displayFacing);
}

void CGUnit_C::PostAnimate(CGWorldFrame *worldFrame) {
  if (m_unit->weaponMode == WEAPONMODE_RANGEDMODE) {
    NTempest::C3Vector cameraPos(0.0f);
    CGWorldFrame::GetCameraPosition(&cameraPos);
    DrawBowString(cameraPos);
  }
}

static CGNamePlateFrame *GetNewNameplateFrame(CSimpleFrame *parent) {
  FREENAMEPLATE *freeNamePlate = s_freeNamePlateList.Head();
  if (freeNamePlate) {
    CGNamePlateFrame *frame = freeNamePlate->namePlate;
    FATALASSERT(frame);
    freeNamePlate->namePlate = 0;
    s_freeNamePlateList.UnlinkNode(freeNamePlate);
    s_freeNamePlateList.DeleteNode(freeNamePlate);
    return frame;
  }
  return NEW(CGNamePlateFrame)(parent);
}

static int CalculateScreenSortOrder(CGWorldFrame *worldFrame, NAMEPLATEDESC *desc) {
  FATALASSERT(worldFrame);
  FATALASSERT(desc);
  FATALASSERT(desc->unit);

  NTempest::C3Vector namePlatePos;
  desc->unit->GetPosition(namePlatePos);
  namePlatePos.z += desc->unit->GetUnitData()->combatReach * desc->unit->GetScale();

  CGCamera          *camera = CGWorldFrame::GetActiveCamera();
  NTempest::C3Vector relative = namePlatePos - camera->Position();
  float              depth = NTempest::C3Vector::Dot(relative, camera->Forward());
  if (depth <= 0.01f) {
    return 0;
  }

  float vertical = depth * static_cast<float>(tan(camera->FOV() * 0.5f));
  float horizontal = vertical * camera->Aspect();
  desc->screenCoords.x = 0.4f + 0.4f * NTempest::C3Vector::Dot(relative, camera->Right()) / horizontal;
  desc->screenCoords.y = 0.3f - 0.3f * NTempest::C3Vector::Dot(relative, camera->Up()) / vertical;

  float dx = desc->screenCoords.x - 0.4f;
  float dy = desc->screenCoords.y - 0.3f;
  desc->screenSortOrder = dx * dx + dy * dy;
  return 1;
}

void CGUnit_C::AddUnitNamePlate(CGWorldFrame *worldFrame) {
  FATALASSERT(worldFrame);
  if (!s_drawNameplates) {
    return;
  }

  unsigned __int64 unit = GetGUID();
  CHashKeyGUID     key(unit);
  NAMEPLATEDESC   *desc = s_monsterNamePlateList.Ptr(static_cast<unsigned int>(unit), key);
  if (!desc) {
    desc = s_monsterNamePlateList.New(static_cast<unsigned int>(unit), key, 0, 0);
    desc->unit = this;
    desc->namePlate = GetNewNameplateFrame(worldFrame);
    FATALASSERT(desc->namePlate);
    desc->namePlate->Initialize(this);
    CalculateScreenSortOrder(worldFrame, desc);
    desc->namePlate->Show();
    InsertSortedNamePlate(desc);
  } else {
    FATALASSERT(desc->unit);
    FATALASSERT(this == desc->unit);
    if (CalculateScreenSortOrder(worldFrame, desc)) {
      InsertSortedNamePlate(desc);
    }
  }
  ResortAllUnitNameplates(worldFrame);
}

void CGUnit_C::InsertSortedNamePlate(NAMEPLATEDESC *desc) {
  FATALASSERT(desc);
  s_namePlateList.UnlinkNode(desc);
  ITERATELIST(NAMEPLATEDESC, s_namePlateList, existing) {
    if (existing->screenSortOrder >= desc->screenSortOrder) {
      s_namePlateList.LinkNode(desc, LIST_LINK_BEFORE, existing);
      return;
    }
  }
  s_namePlateList.LinkNode(desc, LIST_TAIL, 0);
}

static void RecycleNameplateFrame(CGNamePlateFrame *frame) {
  if (!frame) {
    return;
  }
  frame->Hide();
  FREENAMEPLATE *freeNamePlate = s_freeNamePlateList.NewNode(LIST_TAIL, 0, 0);
  freeNamePlate->namePlate = frame;
}

void CGUnit_C::RemoveAllNamePlates() {
  s_freeNamePlateList.Clear();
  while (NAMEPLATEDESC *desc = s_namePlateList.Head()) {
    DEL(desc->namePlate);
    desc->namePlate = 0;
    s_monsterNamePlateList.Delete(desc);
  }
  s_monsterNamePlateList.Clear();
}

void CGUnit_C::RemoveUnitNamePlate() {
  unsigned __int64 unit = GetGUID();
  CHashKeyGUID     key(unit);
  NAMEPLATEDESC   *desc = s_monsterNamePlateList.Ptr(static_cast<unsigned int>(unit), key);
  if (desc) {
    s_monsterNamePlateList.Delete(desc);
  }
}

void CGUnit_C::UpdateUnitNameplates(CGWorldFrame *worldFrame) {
  s_namePlateWorldFrame = worldFrame;
  if (!worldFrame) {
    while (NAMEPLATEDESC *desc = s_namePlateList.Head()) {
      s_monsterNamePlateList.Delete(desc);
    }
    s_monsterNamePlateList.Clear();
  }
}

void CGUnit_C::ResortAllUnitNameplates(CGWorldFrame *worldFrame) {
  FATALASSERT(worldFrame);
  s_namePlateList.UnlinkAll();
  ITERATELIST(NAMEPLATEDESC, s_monsterNamePlateList, curr) {
    CalculateScreenSortOrder(worldFrame, curr);
    FATALASSERT(curr->unit);
    curr->unit->InsertSortedNamePlate(curr);
  }
}

unsigned int CGUnit_C::GetPlayerNameAttachmentPoint() {
  if (!(m_flags & 0x10)) {
    return 18;
  }
  return (m_flags & 0x1000) ? 18 : 29;
}

const char *CGUnit_C::GetUnitName() const {
  if (m_obj->m_type & 0x10) {
    const NameCache *name = g_nameDBCache.GetRecord(m_obj->m_guid, m_obj->m_guid, NameQueryCallback, 0);
    if (name) {
      return name->m_name;
    }
  } else if (m_unit->petNumber) {
    const PetNameCache *name = g_petNameCache.GetRecord(m_unit->petNumber, m_obj->m_guid, NameQueryCallback, 0);
    if (name) {
      if (name->m_timestamp == m_unit->petNameTimestamp) {
        return name->m_name;
      }

      g_petNameCache.Invalidate(m_unit->petNumber);
      g_petNameCache.GetRecord(m_unit->petNumber, m_obj->m_guid, NameQueryCallback, 0);
    }
  }

  return m_stats ? m_stats->m_name[0] : "Unknown Being";
}

void CGUnit_C::UpdatePlayerNameWorldText() {
  PlayerNameUpdateWorldText(m_unitNameHandle);
}

int CGUnit_C::ShouldRender(unsigned long worldStatus) {
  if (m_animFlags & 0x8000) {
    UnitEffectOneShot(SPECIALEFFECT_LEVELUP, GetGUID(), 0, 0.0f, 1.0f, false);
  }
  m_animFlags &= ~0x8000U;

  if (!(worldStatus & 1)) {
    RemoveUnitNamePlate();
  }
  if (!(m_flags & 0x100)) {
    worldStatus &= ~1U;
  }

  int shouldRender = CGObject_C::ShouldRender(worldStatus);
  if (m_texComponent && shouldRender && TexComponentCheckSections(m_texComponent, 0)) {
    CommitTexture(0);
  }
  return shouldRender;
}

static bool IsDayTime() {
  unsigned int encoded;
  WowTime valMin;
  WowTime valMax;
  WowTime::WowEncodeTime(encoded, 0, 20, -1, -1, -1, -1, 0);
  WowTime::WowDecodeTime(encoded, &valMax);
  WowTime::WowEncodeTime(encoded, 30, 5, -1, -1, -1, -1, 0);
  WowTime::WowDecodeTime(encoded, &valMin);
  return g_clientGameTime.InRange(valMin, valMax);
}

static bool IsMountSpell(const SpellRec* rec) {
  for (unsigned int i = 0; i < 3; ++i) {
    if (rec->m_effectAura[i] == 34) {
      return 1;
    }
  }
  return 0;
}

int CGUnit_C::QueueAnim(ANIMQUEUETYPE type, const ATTACKROUNDINFO *roundInfo) {
  FATALASSERT(type < ANIMQUEUE_NUMTYPES);
  if (type == ANIMQUEUE_NONE) {
    return 0;
  }

  if (type == ANIMQUEUE_WOUND && IsAttackAnimState(m_currentTorsoAnimState)) {
    return 0;
  }
  if (m_animFlags & 0x2000) {
    return 0;
  }

  ANIMQUEUENODE *node = GetNewAnimNode(0);
  node->type = type;
  if (roundInfo) {
    node->roundInfo = *roundInfo;
  }
  if (type >= ANIMQUEUE_SITDOWN && type <= ANIMQUEUE_KNEELUP) {
    m_flags |= 0x40000;
  }
  return 1;
}

bool CGUnit_C::CheckAndReportSpellInhibitFlags(const SpellRec *spell, const CGItem_C *item) {
  FATALASSERT(spell);

  if (item) {
    if (Spell_C_GetItemCooldown(item->GetEntryID(), 0, 0, 0)) {
      Spell_C_SpellFailed(spell->m_ID, 21, -1, -1);
      return false;
    }
  } else {
    if (Spell_C_GetSpellCooldown(spell->m_ID, 0, 0, 0, 0)) {
      Spell_C_SpellFailed(spell->m_ID, 37, -1, -1);
      return false;
    }

    int availablePower =
        spell->m_powerType < 0
            ? m_unit->health
            : m_unit->power[spell->m_powerType];
    if (availablePower < Spell_C_GetManaCost(spell->m_ID, 0)) {
      Spell_C_SpellFailed(spell->m_ID, 44, -1, -1);
      return false;
    }
  }

  for (unsigned int effect = 0; effect < 3; ++effect) {
    if (spell->m_effect[effect] == 36 && (GetType() & TYPE_PLAYER) && IsSpellKnown(spell->m_effectTriggerSpell[effect])) {
      const SpellRec *learnedSpell = g_spellDB.GetRecord(spell->m_effectTriggerSpell[effect]);
      CGGameUI::DisplayError(
          GERR_SPELL_ALREADY_KNOWN_S,
          learnedSpell ? learnedSpell->m_name_lang[CURRENT_LANGUAGE] : "Unknown"
      );
      return false;
    }

    if (spell->m_effect[effect] == 28 && (m_unit->charm || m_unit->summon) && !(spell->m_attributesEx & 1)) {
      Spell_C_SpellFailed(spell->m_ID, 2, -1, -1);
      return false;
    }

    if ((spell->m_implicitTargetA[effect] == 5 || spell->m_implicitTargetB[effect] == 5) &&
        !(m_unit->charm || m_unit->summon)) {
      Spell_C_SpellFailed(spell->m_ID, 43, -1, -1);
      return false;
    }
  }

  if ((spell->m_auraInterruptFlags & 0x40) && !(m_unit->flags & 0x2000)) {
    Spell_C_SpellFailed(spell->m_ID, 48, -1, -1);
    return false;
  }
  if ((spell->m_attributesEx & 4) && (spell->m_channelInterruptFlags & 0x40) && !(m_unit->flags & 0x2000)) {
    Spell_C_SpellFailed(spell->m_ID, 48, -1, -1);
    return false;
  }
  if (!(spell->m_attributes & 0x800000) && (m_unit->flags & 0x2000)) {
    Spell_C_SpellFailed(spell->m_ID, 36, -1, -1);
    return false;
  }
  if (!(spell->m_attributes & 0x8000000) && m_unit->standState) {
    Spell_C_SpellFailed(spell->m_ID, 31, -1, -1);
    return false;
  }
  if ((spell->m_attributes & 0x1000) && !IsDayTime()) {
    Spell_C_SpellFailed(spell->m_ID, 46, -1, -1);
    return false;
  }
  if ((spell->m_attributes & 0x2000) && IsDayTime()) {
    Spell_C_SpellFailed(spell->m_ID, 49, -1, -1);
    return false;
  }

  unsigned int moveFlags = m_move.GetMoveFlags();
  if ((spell->m_auraInterruptFlags & 0x100) && !(moveFlags & 0x2000000) ||
      (spell->m_attributesEx & 4) && (spell->m_channelInterruptFlags & 0x100) && !(moveFlags & 0x2000000)) {
    Spell_C_SpellFailed(spell->m_ID, 53, -1, -1);
    return false;
  }
  if ((spell->m_auraInterruptFlags & 0x80) && (moveFlags & 0x2000000) ||
      (spell->m_attributesEx & 4) && (spell->m_channelInterruptFlags & 0x80) && (moveFlags & 0x2000000)) {
    Spell_C_SpellFailed(spell->m_ID, 45, -1, -1);
    return false;
  }
  if ((spell->m_interruptFlags & 1) && (moveFlags & 0x400F)) {
    Spell_C_SpellFailed(spell->m_ID, 26, -1, -1);
    return false;
  }

  unsigned int formMask = m_unit->shapeshiftForm ? 1 << (m_unit->shapeshiftForm - 1) : 0;
  if (spell->m_shapeshiftMask && !(spell->m_shapeshiftMask & formMask)) {
    const SpellShapeshiftFormRec *form = g_spellShapeshiftFormDB.GetRecord(m_unit->shapeshiftForm);
    if (!form || (form->m_flags & 1)) {
      Spell_C_SpellFailed(spell->m_ID, 51, -1, -1);
      return false;
    }
  }
  if (IsShapeShifted()) {
    if (!(spell->m_shapeshiftMask & formMask) && (spell->m_attributes & 0x10000)) {
      Spell_C_SpellFailed(spell->m_ID, 38, -1, -1);
      return false;
    }
  } else if (spell->m_shapeshiftMask && !(spell->m_shapeshiftMask & formMask)) {
    Spell_C_SpellFailed(spell->m_ID, 51, -1, -1);
    return false;
  }
  if ((spell->m_attributes & 0x20000) && !(m_unit->flags & 0x8000)) {
    Spell_C_SpellFailed(spell->m_ID, 52, -1, -1);
    return false;
  }
  if ((spell->m_attributes & 0x10000000) && (m_unit->flags & 0x80000)) {
    Spell_C_SpellFailed(spell->m_ID, 52, -1, -1);
    return false;
  }
  if (spell->m_casterAuraState && !(m_unit->auraState & (1 << (spell->m_casterAuraState - 1)))) {
    Spell_C_SpellFailed(spell->m_ID, 9, -1, -1);
    return false;
  }
  if (IsMountSpell(spell) && !CanBeMounted()) {
    Spell_C_SpellFailed(spell->m_ID, 80, -1, -1);
    return false;
  }
  return true;
}

void CGUnit_C::PendingPrecastInterrupt(int spellID) {
  int &interruptedSpell = m_interruptedSpell;
  if (interruptedSpell && !spellID) {
    SpellVisualsHandleCastStop(interruptedSpell, this, 2, 0);
  }
  interruptedSpell = spellID;
}

void CGUnit_C::StoreXPGain(int XP) {
  m_accumulatedXPDrop += XP;
  unsigned int animFlags = m_animFlags;
  m_flags |= 0x80;
  if (animFlags & 0x2000) {
    ShowPlayerXPGained();
  }
}

void CGUnit_C::UpdateInteractIcon(QUEST_GIVER_STATUS status) {
  FATALASSERT(status < QUEST_GIVER_NUMITEMS);
  if (s_questIconInfo[status] != s_questIconInfo[m_questGiverStatus]) {
    UpdateInteractIcon(s_questIconInfo[status]);
  }
  m_questGiverStatus = status;
}

void CGUnit_C::UpdateInteractIcon(INTERACTICONTYPE which) {
  RemoveInteractIcon();
  if (which) {
    FATALASSERT(which < INTERACTICON_NUMITEMS);
    unsigned int index = which - 1;
    FATALASSERT(s_interactIconModelInfo[index].model);
    m_interactIconModel = ModelDuplicate(s_interactIconModelInfo[index].model, 0);
    if (m_interactIconModel) {
      int renderName = GetType() & TYPE_PLAYER
          ? static_cast<const CGPlayer_C *>(this)->CGPlayer_C::ShouldRenderUnitName(PlayerNameGetUnitNameMode())
          : CGUnit_C::ShouldRenderUnitName(PlayerNameGetUnitNameMode());
      ModelSetSequence(m_interactIconModel, renderName != 0, 0);

      HMODEL charModel = CGObject_C::GetCharacterModel(0);
      if (charModel) {
        ModelAddLink(charModel, GetPlayerNameAttachmentPoint(), m_interactIconModel, 1.0f);
        HandleClose(charModel);
      }
    }
  }
}

void CGUnit_C::OnNPCHello() {
  PlayNPCSound(NPCSOUND_HELLO, 0);
}

void CGUnit_C::OnNPCGoodbye() {
  PlayNPCSound(NPCSOUND_GOODBYE, 0);
}

void CGUnit_C::QueueBloodSplat(BLOODSPURTLOCATION linkPoint) {
  BLOODSPLATNODE *node = NewBloodSplatNode();
  node->m_triggerTime = OsGetAsyncTimeMs() + 500;

  NTempest::C3Vector  jitter(
      NTempest::CRandom::reals_(g_rndSeed) / 6.0f,
      NTempest::CRandom::reals_(g_rndSeed) / 6.0f,
      0.0f
  );
  NTempest::C44Matrix direction;
  direction.Rotate(-GetDisplayFacing(), NTempest::C3Vector(0.0f, 0.0f, 1.0f), 1);

  NTempest::C3Vector rot(2.0f / 3.0f, 0.0f, 0.0f);
  if (linkPoint == BLOODSPURT_BACK) {
    rot.x = -2.0f / 3.0f;
  }
  rot = direction * rot;

  node->m_position = GetPosition() + jitter + rot;
  m_bloodSplatNodes.LinkNode(node, LIST_TAIL, 0);
}

void CGUnit_C::HandleBloodPool(unsigned int currentTime) {
  const UnitBloodRec *bloodRec = GetBloodRecord();
  if (bloodRec && static_cast<int>(currentTime - m_nextAllowableBloodPool) >= 0) {
    m_nextAllowableBloodPool = currentTime + 2000 + NTempest::CMath::mulhwu_(3000, NTempest::CRandom::uint32_(g_rndSeed));

    NTempest::C3Vector jitter(NTempest::CRandom::reals_(g_rndSeed) / 6.0f, NTempest::CRandom::reals_(g_rndSeed) / 6.0f, 0.0f);
    jitter += GetPosition();
    UnitFootprintNewBloodSplat(bloodRec, GetUnitSize(), jitter);
  }
}

const UnitBloodRec *CGUnit_C::GetBloodRecord() {
  if (!m_bloodRec) {
    return 0;
  }
  return g_unitBloodDB.GetRecord(m_bloodRec->m_Violencelevel[ViolenceGetLevel()]);
}

void CGUnit_C::HandleCastAnimEvent() {
  unsigned int &castKit = m_spellCastingEffectKit;
  if (castKit) {
    const SpellVisualKitRec *kitRec = g_spellVisualKitDB.GetRecord(castKit);
    if (kitRec) {
      SpellVisualsPlayCastKit(this, kitRec, 0, 1);
    }
    castKit = 0;
  }
}

void CGUnit_C::OnCharmedChanged() {
  CGGameUI::UnitNameUpdate(GetGUID());
  HPLAYERNAME__ *unitNameHandle = m_unitNameHandle;
  if (unitNameHandle) {
    PlayerNameTriggerColorUpdate(unitNameHandle);
  }
  Script_SendUnitSignal(GetGUID(), 27);
}

void CGUnit_C::CheckPendingMissileRelease(const NTempest::C3Vector *position) {
  if (m_spellCastingSoundID) {
    SndInterfacePlaySpellSound(m_spellCastingSoundID, this);
    m_spellCastingSoundID = 0;
  }

  if (m_spellCastingCameraShakeID) {
    NTempest::C3Vector effectPosition = position ? *position : GetPosition();
    SpellVisualsPlayCameraShakeID(m_spellCastingCameraShakeID, effectPosition);
    m_spellCastingCameraShakeID = 0;
  }

  if (m_spellMissileStruct.caster) {
    if (m_spellMissileStruct.ammoDisplayID) {
      if (!(GetType() & TYPE_PLAYER)) {
        ThrownMissileReleased();
      } else {
        CGBag_C         *bag = static_cast<CGPlayer_C *>(this)->CGPlayer_C::GetBag();
        unsigned __int64 itemGUID = bag->GetItem(17);
        CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(itemGUID, __FILE__, __LINE__));
        if (item) {
          if (item->GetInventoryType() == 25) {
            ThrownMissileReleased();
          } else {
            m_flags |= 0x800;
            SetRangedWeaponReleaseAnim();
          }
        }
      }
    }

    NTempest::C3Vector missilePosition;
    if (position) {
      missilePosition = *position;
    } else {
      missilePosition = GetPosition();
      missilePosition.z += 1.0f;
    }
    m_spellMissileStruct.startPosition = missilePosition;

    int durationOffset = 0;
    if (m_currentTorsoAnimState == 38) {
      durationOffset = OsGetAsyncTimeMs() - m_lastSpellCastAnimTime;
    }
    UnitEffectAddMissile(m_spellMissileStruct, durationOffset < 0 ? 0 : durationOffset);
    m_spellMissileStruct.caster = 0;
  }

  HandleCastAnimEvent();
  CheckPendingImpactKit();
}

void CGUnit_C::ThrownMissileReleased() {
  HMODEL model = GetCharacterModel(0);
  FATALASSERT(model);
  SetAttachmentHidden(OBJATTACH_RANGED, 1);
  HandleClose(model);
  m_flags |= 0x20000;
}

void CGUnit_C::CheckPendingThrownWeaponReattach(bool force) {
  int displayID = GetVirtualItemDisplayID(VIRTUAL_MONSTER_SLOT_RANGED);
  const VirtualItemInfo *itemInfo = GetVirtualItem(VIRTUAL_MONSTER_SLOT_RANGED, 0);
  if ((force || (m_flags & 0x20000)) && displayID && itemInfo) {
    const ItemSubClassRec *subClass = SDBItemSubclassGetSubClassRec(itemInfo->m_classID, itemInfo->m_subclassID);
    AddObjectComponentBySlot(
        INVSLOT_RANGED,
        displayID,
        itemInfo->m_inventoryType,
        subClass && (subClass->m_flags & 0x20),
        0,
        0,
        -1,
        0
    );
    SetAttachmentHidden(OBJATTACH_RANGED, 0);
  }
  m_flags &= ~0x20000u;
}

void CGUnit_C::OnStopRender() {
  CheckPendingVictimFeedback();
  CheckPendingImpactKit();
  CheckPendingMissileRelease(0);
  RemoveUnitNamePlate();
}

void CGUnit_C::CheckRendering() {
  bool wasRendering = (m_animFlags & 0x40000) != 0;
  if (m_animFlags & 0x20000) {
    m_animFlags |= 0x40000;
  } else {
    m_animFlags &= ~0x40000;
  }
  m_animFlags &= ~0x20000;

  if (wasRendering && !(m_animFlags & 0x40000)) {
    OnStopRender();
  }
}

void CGUnit_C::UpdateDisplay(unsigned long now) {
  CheckRendering();
  if (m_worldObject) {
    UpdateLookAtTarget();
    if (m_flags & 0x10000) {
      HandleBloodPool(now);
    }
    CWorld::TickObject(m_worldObject);
  } else {
    FATALASSERT(CGPlayer_C::GetActive() == GetGUID());
  }
}

void CGUnit_C::UpdateDisplayInfo() {
  int playerModelChanged;
  int wasPlayerModel;
  if (!DisplayInfoNeedsUpdate(playerModelChanged, wasPlayerModel)) {
    return;
  }

  CleanupUnitArtwork(playerModelChanged, wasPlayerModel);

  RefreshDataPointers();

  ReinitializeUnitArtwork();

  InitializeExtendedDisplay();
  AttachVirtualMonsterWeapons();
  InitializeNPCItems();

  PostReinitializeArtwork();

  if (m_geosetHandle) {
    CharCustomizationCommitItemGeosets(m_geosetHandle, 0);
  }

  if (m_shapeShiftPoof) {
    const SpellVisualRec    *visual = g_spellVisualDB.GetRecord(m_shapeShiftPoof->m_spellVisualID);
    const SpellVisualKitRec *impactKit = visual ? g_spellVisualKitDB.GetRecord(visual->m_impactKit) : 0;
    if (impactKit) {
      PlayImpactKit(m_shapeShiftPoof->m_ID, impactKit);
    }
    m_shapeShiftPoof = 0;
  }

  ReinitializePaperdollModel();
}

int CGUnit_C::DisplayInfoNeedsUpdate(int &playerModelChanged, int &wasPlayerModel) const {
  FATALASSERT(m_modelData);
  playerModelChanged = 0;
  wasPlayerModel = 0;

  const CreatureDisplayInfoRec *displayInfo = g_creatureDisplayInfoDB.GetRecord(m_unit->displayID);
  if (!displayInfo) {
    SysMsgPrintf(SYSMSG_ERROR, 2, "NOUNITDISPLAYID|%d|%s", m_unit->displayID, GetUnitName());
    return 0;
  }
  if (displayInfo == m_displayInfo) {
    return 0;
  }

  const CreatureModelDataRec *modelData = g_creatureModelDataDB.GetRecord(displayInfo->m_modelID);
  if (modelData == m_modelData) {
    const CreatureSoundDataRec *soundData = g_creatureSoundDataDB.GetRecord(displayInfo->m_soundID);
    return soundData != m_soundData;
  }

  if ((modelData->m_flags ^ m_modelData->m_flags) & 4) {
    playerModelChanged = 1;
    if (m_modelData->m_flags & 4) {
      wasPlayerModel = 1;
    }
  }
  return 1;
}

void CGUnit_C::RefreshDataPointers() {
  if (!m_unit->displayID) {
    FATALERROR(("Error, unit %d has displayID 0!", m_obj->m_entryID));
  }

  m_displayInfo = g_creatureDisplayInfoDB.GetRecord(m_unit->displayID);
  if (!m_displayInfo) {
    SysMsgPrintf(SYSMSG_ERROR, 2, "NOUNITDISPLAYID|%d|%s", m_unit->displayID, GetUnitName());
    m_displayInfo = g_creatureDisplayInfoDB.GetRecordByIndex(0);
    if (!m_displayInfo) {
      FATALERROR(("Error, NO creature display records found"));
    }
  }

  m_displayInfoExtra = g_creatureDisplayInfoExtraDB.GetRecord(m_displayInfo->m_extendedDisplayInfoID);
  m_modelData = g_creatureModelDataDB.GetRecord(m_displayInfo->m_modelID);
  FATALASSERT(m_modelData);

  m_soundData = g_creatureSoundDataDB.GetRecord(m_displayInfo->m_soundID);
  if (!m_soundData) {
    m_soundData = g_creatureSoundDataDB.GetRecord(m_modelData->m_soundID);
  }
  FATALASSERT(m_soundData);

  m_bloodRec = g_unitBloodLevelsDB.GetRecord(m_displayInfo->m_bloodID);
  if (!m_bloodRec) {
    m_bloodRec = g_unitBloodLevelsDB.GetRecord(m_modelData->m_bloodID);
    if (!m_bloodRec) {
      m_bloodRec = g_unitBloodLevelsDB.GetRecordByIndex(0);
    }
  }
  FATALASSERT(m_bloodRec);
}

void CGUnit_C::SetMirrorHandlers() {
  SetAuraMirrorHandlers();
  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 104, 4, UnitLevelUpdateHandler, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_OBJECT) + 12, 4, UnitModeUpdateHandler, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 64, 4, UnitHealthUpdateHandler, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 192, 4, UnitFlagUpdateHandler, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 16, 16, UnitCharmedUpdateHandler, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 580, 4, DisplayIDUpdateHandler, 0, HANDLER_PRIORITY_HIGH);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 664, 1, StandStateUpdateHandler, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 665, 1, NPCFlagsHandler, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 667, 1, WeaponModeUpdateHandler, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 672, 4, PetNameChangeHandler, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 156, 4, VirtualItemChangeHandler, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(
      GetGUID(),
      OffsetOf(ID_UNIT) + 160,
      4,
      VirtualItemChangeHandler,
      reinterpret_cast<void *>(VIRTUAL_MONSTER_SLOT_OFFHAND),
      HANDLER_PRIORITY_NORMAL
  );
  ClntObjMgrSetObjMirrorHandler(
      GetGUID(),
      OffsetOf(ID_UNIT) + 164,
      4,
      VirtualItemChangeHandler,
      reinterpret_cast<void *>(VIRTUAL_MONSTER_SLOT_RANGED),
      HANDLER_PRIORITY_NORMAL
  );
  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 684, 4, DynamicFlagsChangeHandler, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 688, 4, EmoteStateChangeHandler, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 692, 4, ChannelSpellChangeHandler, 0, HANDLER_PRIORITY_NORMAL);
}

IMPACTEFFECTDESC::~IMPACTEFFECTDESC() {
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(victim, __FILE__, __LINE__));
  ASSERT(!unit || unit->IsA(TYPE_UNIT));
  if (unit) {
    unit->DDDELLOG(attacker, "~IMPACTEFFECTDESC", __FILE__, __LINE__);
  }
}

void CGUnit_C::UnsetMirrorHandlers() {
  UnsetAuraMirrorHandlers();
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 104, UnitLevelUpdateHandler, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_OBJECT) + 12, UnitModeUpdateHandler, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 64, UnitHealthUpdateHandler, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 192, UnitFlagUpdateHandler, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 16, UnitCharmedUpdateHandler, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 580, DisplayIDUpdateHandler, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 664, StandStateUpdateHandler, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 665, NPCFlagsHandler, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 667, WeaponModeUpdateHandler, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 672, PetNameChangeHandler, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 156, VirtualItemChangeHandler, 0);
  ClntObjMgrUnsetObjMirrorHandler(
      GetGUID(),
      OffsetOf(ID_UNIT) + 160,
      VirtualItemChangeHandler,
      reinterpret_cast<void *>(VIRTUAL_MONSTER_SLOT_OFFHAND)
  );
  ClntObjMgrUnsetObjMirrorHandler(
      GetGUID(),
      OffsetOf(ID_UNIT) + 164,
      VirtualItemChangeHandler,
      reinterpret_cast<void *>(VIRTUAL_MONSTER_SLOT_RANGED)
  );
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 684, DynamicFlagsChangeHandler, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 688, EmoteStateChangeHandler, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 692, ChannelSpellChangeHandler, 0);
}

void CGUnit_C::StandStateChanged(unsigned int oldState) {
  if (IsInReenable()) {
    return;
  }

  unsigned char newState = m_unit->standState;
  OnStandStateChanged(oldState, newState);
  SetSheatheReason(SHEATHE_STANDSTATE, s_mustSheathe[newState], 0);
  FATALASSERT(newState < 9);
  FATALASSERT(oldState < 9);

  if ((IsInStandSitTransition() || IsInSitSleepPosition()) && (!newState || oldState)) {
    QueueAnim(s_animStandUpTransitions[oldState], 0);
  }
  if (newState || !(m_move.m_moveFlags & 0x40FF)) {
    QueueAnim(s_animStandDownTransitions[newState], 0);
  }
  if (m_currentTorsoAnimState == 46) {
    ClearTorsoAnimation(64);
  }
}

SPELLEFFECTDESC::~SPELLEFFECTDESC() {
  ClearLightningObjects();
}

void CGUnit_C::NPCFlagChanged(unsigned int oldNPCFlags) {
  unsigned int newFlags = m_unit->npcFlags;
  unsigned int changed = oldNPCFlags ^ newFlags;
  if (changed & 8) {
    RemoveInteractIcon();
  }
  if (changed & 0x10) {
    RemoveInteractIcon();
    if (newFlags & 0x10) {
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (player) {
        player->UpdateBindStatus(this);
      }
    }
  }
  if (changed & 0x20) {
    RemoveInteractIcon();
  }
  if (changed & 2) {
    UpdateInteractIcon(QUEST_GIVER_NONE);
    if (newFlags & 2) {
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (player) {
        player->UpdateQuestStatus(this);
      }
    } else if (GetGUID() == CGQuestInfo::GetQuestGiver()) {
      CGQuestInfo::QuestGiverFinished();
    }
  }
  if (changed & 4) {
    RemoveInteractIcon();
    if (newFlags & 4) {
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (player) {
        player->UpdateTaxiStatus(this);
      }
    }
  }
}

void CGUnit_C::WeaponModeChanged() {
  if (GetGUID() == ClntObjMgrGetActivePlayer()) {
    SetLastWeaponModeSent(-1);
    if (!SheatheAnimPlaying() || (m_animFlags & 0x10000)) {
      UpdateSheatheRangedReasons(1);
    }
  } else {
    if (g_standStateAllowsSheathing[m_unit->standState]) {
      MaybeStartSheatheAnim();
    }
  }
}

void CGUnit_C::UpdateSheatheRangedReasons(bool suppressSound) {
  if (m_unit->weaponMode == WEAPONMODE_SHEATHEDMODE) {
    SetSheatheReason(SHEATHE_PLAYEREXPLICIT, 1, suppressSound);
  } else {
    if (m_unit->weaponMode == WEAPONMODE_RANGEDMODE) {
      CheckPendingThrownWeaponReattach(1);
      SetSheatheReason(SHEATHE_RANGED, 1, suppressSound);
      SetSheatheReason(SHEATHE_PLAYEREXPLICIT, 0, 0);
      return;
    }
    SetSheatheReason(SHEATHE_PLAYEREXPLICIT, 0, 0);
  }
  SetSheatheReason(SHEATHE_RANGED, 0, 0);
}

void CGUnit_C::HandlePrecastStart(bool precast) {
  if (precast) {
    SetSheatheReason(SHEATHE_PRECAST, 1, 1);
  }
}

void CGUnit_C::HandlePrecastStop(int spellID, bool force) {
  const SpellRec          *spellRec = g_spellDB.GetRecord(spellID);
  const SpellVisualRec    *visualRec = spellRec ? g_spellVisualDB.GetRecord(spellRec->m_spellVisualID) : 0;
  const SpellVisualKitRec *kitRec = visualRec ? g_spellVisualKitDB.GetRecord(visualRec->m_castKit) : 0;
  if (kitRec && kitRec->m_anim != -1 && !force) {
    m_precastSheatheHoldTimer = OsGetAsyncTimeMs() + 1000;
  } else {
    SetSheatheReason(SHEATHE_PRECAST, 0, 1);
    m_precastSheatheHoldTimer = -1;
  }
}

ANIMQUEUENODE *CGUnit_C::GetNewAnimNode(int leaveUnlinked) {
  ANIMQUEUENODE *node = s_animQueueFreeList.Get(0);

  if (!leaveUnlinked) {
    m_animQueue.LinkNode(node, LIST_TAIL, 0);
  }

  return node;
}

void CGUnit_C::RecycleAnimNode(ANIMQUEUENODE *node) {
  if (node) {
    s_animQueueFreeList.Put(node);
  }
}

void CGUnit_C::ProcessDiscardedAnim(ANIMQUEUENODE *node, bool doNotProcess) {
  FATALASSERT(node);
  FATALASSERT(node->type < ANIMQUEUE_NUMTYPES);

  switch (node->type) {
    case ANIMQUEUE_ATTACK:
      if (!doNotProcess) {
        CGUnit_C *victim =
            static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(node->roundInfo.victim, __FILE__, __LINE__));
        if (victim) {
          victim->DoVictimFeedback(&node->roundInfo, 0);
        } else {
          CGUnit_C *activePlayer =
              static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
          if (activePlayer) {
            activePlayer->DDGENLOG(
                GetGUID(), "WARNING discarding attack queue with no victimptr!", __FILE__, __LINE__
            );
          }
        }
      }
      break;

    case ANIMQUEUE_SITDOWN:
    case ANIMQUEUE_SITUP:
    case ANIMQUEUE_SLEEPDOWN:
    case ANIMQUEUE_SLEEPUP:
    case ANIMQUEUE_SITCHAIR:
    case ANIMQUEUE_SITCHAIRUP:
    case ANIMQUEUE_SITCHAIRLOW:
    case ANIMQUEUE_SITCHAIRMEDIUM:
    case ANIMQUEUE_SITCHAIRHIGH:
    case ANIMQUEUE_KNEELDOWN:
    case ANIMQUEUE_KNEELUP:
      m_flags &= ~0x40000u;
      break;
  }

  RecycleAnimNode(node);
}

void CGUnit_C::ProcessAnim(ANIMQUEUENODE *node) {
  static const ANIM_STATE s_standStateAnims[ANIMQUEUE_NUMTYPES] = {
      INVALID_ANIM_STATE,
      INVALID_ANIM_STATE,
      INVALID_ANIM_STATE,
      ANIM_STATE_SITDOWN,
      ANIM_STATE_SITUP,
      ANIM_STATE_SLEEPDOWN,
      ANIM_STATE_SLEEPUP,
      ANIM_STATE_SITCHAIRLOW,
      INVALID_ANIM_STATE,
      ANIM_STATE_SITCHAIRLOW,
      ANIM_STATE_SITCHAIRMEDIUM,
      ANIM_STATE_SITCHAIRHIGH,
      ANIM_STATE_DEAD,
      ANIM_STATE_KNEELDOWN,
      ANIM_STATE_KNEELUP
  };

  FATALASSERT(node);
  FATALASSERT(node->type < ANIMQUEUE_NUMTYPES);

  CheckPendingVictimFeedback();

  switch (node->type) {
    case ANIMQUEUE_ATTACK:
      FATALASSERT(!m_currentDamageInfo);
      if (SetAttackerAnimation(&node->roundInfo, 1)) {
        m_currentDamageInfo = node;
        return;
      }
      break;

    case ANIMQUEUE_WOUND:
      SetVictimAnimation(
          node->roundInfo.newVictimState,
          node->roundInfo.flags & 4,
          node->roundInfo.flags & 8,
          node->roundInfo.victimRoundDuration,
          1
      );
      RecycleAnimNode(node);
      return;

    case ANIMQUEUE_SITDOWN:
    case ANIMQUEUE_SITUP:
    case ANIMQUEUE_SLEEPDOWN:
    case ANIMQUEUE_SLEEPUP:
    case ANIMQUEUE_SITCHAIR:
    case ANIMQUEUE_SITCHAIRLOW:
    case ANIMQUEUE_SITCHAIRMEDIUM:
    case ANIMQUEUE_SITCHAIRHIGH:
    case ANIMQUEUE_DEAD:
    case ANIMQUEUE_KNEELDOWN:
    case ANIMQUEUE_KNEELUP:
      FATALASSERT(s_standStateAnims[node->type] != INVALID_ANIM_STATE);
      UpdateBaseAnimation(s_standStateAnims[node->type], 0);
      m_flags &= ~0x40000u;
      break;

    case ANIMQUEUE_SITCHAIRUP:
      UpdateBaseAnimation(ANIM_STATE_NONE);
      m_flags &= ~0x40000u;
      break;
  }

  RecycleAnimNode(node);
}

void CGUnit_C::PurgeAnimNodes(bool doNotProcess) {
  ITERATELIST(ANIMQUEUENODE, m_animQueue, node) {
    ProcessDiscardedAnim(node, doNotProcess);
  }
}

void CGUnit_C::SpellAnimHit(int spellID) {
  for (unsigned int slot = 0; slot < 56; ++slot) {
    if (m_unit->auras[slot] == spellID && !m_auraFlags[slot]) {
      AddAuraEffect(slot, 1);
    }
  }
}

void CGUnit_C::CheckPendingSpellAnimHits() {
  for (unsigned int count = m_pendingHitAnimVictims.Count(); count; --count) {
    CGUnit_C *victim =
        static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(m_pendingHitAnimVictims[count - 1], __FILE__, __LINE__));
    if (victim && victim->IsA(TYPE_UNIT)) {
      victim->SpellAnimHit(m_pendingHitSpellID);
    }
  }
  m_pendingHitAnimVictims.Clear();
}

static bool NodesSame(const ANIMQUEUENODE* current, const ANIMQUEUENODE* next) {
  FATALASSERT(current);
  if (!next) {
    return 0;
  }
  if (current->type != next->type) {
    return 0;
  }
  if (current->type != ANIMQUEUE_ATTACK) {
    return 1;
  }
  return ((current->roundInfo.flags ^ ~next->roundInfo.flags) >> 9) & 1;
}

ANIMQUEUENODE *CGUnit_C::ProcessAnimQueue() {
  ANIMQUEUENODE *node = m_animQueue.Head();
  while (node) {
    ANIMQUEUENODE *next = node->Next();
    switch (node->type) {
      case ANIMQUEUE_SITDOWN:
      case ANIMQUEUE_SITUP:
      case ANIMQUEUE_SLEEPDOWN:
      case ANIMQUEUE_SLEEPUP:
      case ANIMQUEUE_SITCHAIR:
      case ANIMQUEUE_SITCHAIRUP:
      case ANIMQUEUE_SITCHAIRLOW:
      case ANIMQUEUE_SITCHAIRMEDIUM:
      case ANIMQUEUE_SITCHAIRHIGH:
      case ANIMQUEUE_KNEELDOWN:
      case ANIMQUEUE_KNEELUP:
        if (IsPlayingSittingOrStandingAnim()) {
          return 0;
        }
        node->Unlink();
        return node;

      default:
        if (NodesSame(node, next)) {
          ProcessDiscardedAnim(node, false);
          node = next;
          continue;
        }
        node->Unlink();
        return node;
    }
  }
  return 0;
}

int CGUnit_C::IsPlayingSittingOrStandingAnim() const {
  switch (m_currentBaseAnimState) {
    case 50:
    case 52:
    case 53:
    case 55:
      return 1;

    default:
      return 0;
  }
}

bool CGUnit_C::SetSpellPreCastingAnimation(ANIMENUMERATION anim) {
  HMODEL charModel = GetCharacterModel(0);
  FATALASSERT(charModel);

  bool returnValue = true;
  if (ModelIsLoaded(charModel, 1)) {
    unsigned int sequence = anim;
    while (!ModelHasSequenceId(charModel, sequence)) {
      if (sequence == 31) {
        returnValue = false;
        sequence = 0;
        break;
      }
      sequence = 31;
    }
    HandleClose(charModel);
    m_spellPrecastingAnim = static_cast<ANIMENUMERATION>(sequence);
    return returnValue;
  }

  m_deferredPrecastAnim = anim;
  return false;
}

void CGUnit_C::SetSpellImpactKit(const SpellVisualKitRec *impactKit) {
  if (impactKit && impactKit->m_anim >= 1) {
    m_pendingImpactAnim = static_cast<ANIMENUMERATION>(impactKit->m_anim);
    SetTorsoAnimation(47, 0, 0);
  }
}

int CGUnit_C::PlayEmoteAnimation(unsigned int emoteID, int flags) {
  const EmotesRec *emote = g_emotesDB.GetRecord(emoteID);
  if (!emote || m_unit->standState == 3 || (emote->m_EmoteFlags & 2) || (m_move.m_moveFlags & 0x2000000)) {
    return 0;
  }
  return SetEmoteAnimation(emoteID, flags);
}

void CGUnit_C::RequestTalkEmote(TALKANIMATION talkAnim) {
  FATALASSERT(talkAnim < TALKANIM_NUMTALKANIMS);

  const EmotesRec *emote = s_talkEmotes[talkAnim];
  if (emote && !(m_move.m_moveFlags & 0x2000000) && SetEmoteAnimation(emote->m_ID, 0) && (emote->m_EmoteFlags & 0x800)) {
    SetSheatheReason(SHEATHE_TALKEMOTE, 1, 1);
  }
}

unsigned int CGUnit_C::GetEmoteAnimation(unsigned int emoteID) const {
  const EmotesRec *emote = g_emotesDB.GetRecord(emoteID);
  if (emote) {
    const EmoteAnimsRec *anim = g_emoteAnimsDB.GetRecord(emote->m_EmoteAnimID);
    if (anim && anim->m_ProcessedAnimIndex != -1) {
      return anim->m_ProcessedAnimIndex;
    }
  }
  return GetStandStateAnim(0);
}

static SpellProcHandler const s_spellProcHandlerFunctions[11] = {
    SpellProcChainHandler,   SpellProcColorHandler,         SpellProcScaleHandler,       0, 0, SpellProcEmissiveHandler,
    SpellProcEclipseHandler, SpellProcStandWalkAnimHandler, SpellProcWeaponTrailHandler, 0, 0
};

int CGUnit_C::EmoteProcType(unsigned int emoteID, EMOTESPECPROCS &proc) const {
  const EmotesRec *rec = g_emotesDB.GetRecord(emoteID);
  if (!rec || ((1 << rec->m_EmoteSpecProc) & 7) == 0) {
    return 0;
  }
  proc = static_cast<EMOTESPECPROCS>(rec->m_EmoteSpecProc);
  return 1;
}

static int s_stateTransitions[UNIT_NUMSTANDSTATES][UNIT_NUMSTANDSTATES] = {
    {0, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 1, 1, 1, 1, 0, 1},
    {1, 0, 0, 0, 1, 1, 1, 0, 1},
    {1, 1, 0, 0, 1, 1, 1, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 0, 1, 1, 1, 1, 0, 0}
};

int CGUnit::StandStateValid(UNITSTANDSTATE newState) const {
  UNITSTANDSTATE oldState = static_cast<UNITSTANDSTATE>(m_unit->standState);
  FATALASSERT(oldState < UNIT_NUMSTANDSTATES);
  FATALASSERT(newState < UNIT_NUMSTANDSTATES);
  return s_stateTransitions[oldState][newState];
}

void CGUnit_C::ChangeStandState(unsigned int standState) {
  FATALASSERT(!(standState >= 4 && standState <= 6));
  FATALASSERT(standState < 9);

  if (!IsInStandSitTransition() && !m_castingSpell && !IsMounted() && StandStateValid(static_cast<UNITSTANDSTATE>(standState))) {
    CDataStore msg;
    msg.Put(static_cast<unsigned int>(CMSG_STANDSTATECHANGE));
    msg.Put(standState);
    msg.Finalize();
    ClientServices_Send(&msg);
  }
}

void CGUnit_C::RegisterScript() {
  if (!m_scriptRegistered) {
    ScriptEventsRegisterUnit(this);
  }
  ++m_scriptRegistered;
}

bool CGUnit_C::SetSpellCastingAnimation(
    ANIMENUMERATION anim,
    unsigned int effectKit,
    unsigned int soundID,
    int camShakeID,
    ANIMENUMERATION &finalAnim
) {
  if (anim == INVALID_ANIMATION || anim >= NUM_OBJECTANIMATIONS) {
    return false;
  }

  HMODEL charModel = GetCharacterModel(0);
  FATALASSERT(charModel);

  if ((anim >= ANIM_SPELL_SPECIAL1H &&
       anim <= ANIM_SPELL_SPECIAL2H) ||
      anim == ANIM_SPECIALUNARMED) {
    const VirtualItemInfo *item = GetVirtualItem(VIRTUAL_MONSTER_SLOT_MAINHAND, 0);
    if (!item || m_unit->weaponMode == WEAPONMODE_SHEATHEDMODE) {
      anim = ANIM_SPECIALUNARMED;
    } else {
      unsigned int numLinked = 0;
      if (ModelGetNumLinkedAtPoint(charModel, 1, &numLinked) &&
          numLinked) {
        anim = item->m_inventoryType == INDEX_2HWEAPON_TYPE
                   ? ANIM_SPELL_SPECIAL2H
                   : ANIM_SPELL_SPECIAL1H;
      } else {
        anim = ANIM_SPECIALUNARMED;
      }
    }
  }

  bool result = true;
  bool exit = false;
  while (!ModelHasSequenceId(charModel, anim) && !exit) {
    switch (anim) {
      case ANIM_ATTACK2HLOOSE:
      case ANIM_SPELL_SPECIAL1H:
      case ANIM_KICK:
        PrintAttackSeqErrorMsg(anim, ANIM_ATTACK1H);
        anim = ANIM_ATTACK1H;
        break;
      case ANIM_SPELL_CAST_DIRECTED:
      case ANIM_SPELL_CAST_OMNI:
      case ANIM_EMOTE_USE_STANDING:
        PrintAttackSeqErrorMsg(anim, ANIM_SPELLCAST);
        anim = ANIM_SPELLCAST;
        break;
      case ANIM_SPELL_SPECIAL2H:
        PrintAttackSeqErrorMsg(anim, ANIM_ATTACK2HLOOSE);
        anim = ANIM_ATTACK2HLOOSE;
        break;
      case ANIM_SPECIALUNARMED:
        PrintAttackSeqErrorMsg(anim, ANIM_ATTACKUNARMED);
        anim = ANIM_ATTACKUNARMED;
        break;
      default:
        result = false;
        exit = true;
        FATALASSERT(anim != INVALID_ANIMATION);
        break;
    }
  }

  HandleClose(charModel);
  finalAnim = anim;
  m_spellCastingCameraShakeID = camShakeID;
  m_spellCastingAnim = anim;
  m_spellCastingEffectKit = effectKit;
  m_spellCastingSoundID = soundID;
  return result;
}

void CGUnit_C::UnregisterScript() {
  if (m_scriptRegistered-- == 1) {
    ScriptEventsUnregisterUnit(this);
  }
}

int CGUnit_C::SetEmoteAnimation(unsigned int emoteID, int flags) {
  m_emoteID = emoteID;
  return SetTorsoAnimation(ANIM_STATE_EMOTE, 0, flags);
}

bool AnimSheathesWeapon(unsigned int anim) {
  FATALASSERT(anim < NUM_OBJECTANIMATIONS);
  return (g_seqInformation[anim].flags >> 4) & 1;
}

int CGUnit_C::IsModelComponentable() const {
  return m_displayInfoExtra != 0;
}

void CGUnit_C::SetEmoteQueue(TSStackArray<QUESTGIVEREMOTENODE> &list) {
  SetEmoteQueue(list.Ptr(), list.Count());
}

void CGUnit_C::SetEmoteQueue(const QUESTGIVEREMOTENODE *list, unsigned int num) {
  TSGrowableArray<QUESTGIVEREMOTENODE> &queue =
      m_emoteQueue;
  queue.SetCount(0);

  EMOTESPECPROCS proc = EMOTESPECPROC_NONE;
  while (num) {
    const QUESTGIVEREMOTENODE &node = list[--num];
    if (node.emoteID && EmoteProcType(node.emoteID, proc) && proc == EMOTESPECPROC_NONE) {
      *queue.New() = node;
    }
  }

  if (queue.Count()) {
    queue[queue.Count() - 1].delay += OsGetAsyncTimeMs();
  }
}

void CGUnit_C::ProcessEmoteQueue() {
  if (m_emoteQueue.Count() && m_currentTorsoAnimState != 46) {
    unsigned int index = m_emoteQueue.Count() - 1;
    int currentTime = OsGetAsyncTimeMs();
    if (currentTime >= static_cast<int>(m_emoteQueue[index].delay)) {
      unsigned int emoteID = m_emoteQueue[index].emoteID;
      m_emoteQueue.SetCount(index);
      if (!PlayEmoteAnimation(emoteID, 0) && m_emoteQueue.Count()) {
        m_emoteQueue[m_emoteQueue.Count() - 1].delay += currentTime;
      }
    }
  }
}

unsigned int CGUnit_C::GetDisplayRace() const {
  return m_displayInfoExtra ? m_displayInfoExtra->m_DisplayRaceID : m_unit->race;
}

unsigned int CGUnit_C::GetDisplaySex() const {
  return m_displayInfoExtra ? m_displayInfoExtra->m_DisplaySexID : m_unit->sex;
}

const char *CGUnit_C::GetDisplayTextureName() const {
  return m_displayInfoExtra ? m_displayInfoExtra->m_BakeName : 0;
}

unsigned int CGUnit_C::SkinVariationID() const {
  FATALASSERT(m_displayInfoExtra);
  return m_displayInfoExtra->m_SkinID;
}

unsigned int CGUnit_C::FaceID() const {
  FATALASSERT(m_displayInfoExtra);
  return m_displayInfoExtra->m_FaceID;
}

unsigned int CGUnit_C::HairStyleID() const {
  FATALASSERT(m_displayInfoExtra);
  return m_displayInfoExtra->m_HairStyleID;
}

unsigned int CGUnit_C::HairColorID() const {
  FATALASSERT(m_displayInfoExtra);
  return m_displayInfoExtra->m_HairColorID;
}

unsigned int CGUnit_C::FacialHairID() const {
  FATALASSERT(m_displayInfoExtra);
  return m_displayInfoExtra->m_FacialHairID;
}

void CGUnit_C::InitPreferredGeosets() {
  FATALASSERT(m_displayInfoExtra);
  unsigned int *preferredGeosets = m_preferredGeosets;
  memset(preferredGeosets, 0, 15 * sizeof(*preferredGeosets));
  preferredGeosets[CHARGEOSET_HAIR] = HairStyleID();

  BEARDSTYLEDATA beardStyleData;
  int            hasFacialInfo = CharCustomizationGetBeardStyle(GetDisplayRace(), GetDisplaySex(), FacialHairID(), &beardStyleData);
  preferredGeosets[CHARGEOSET_EAR] = 2;
  if (hasFacialInfo) {
    preferredGeosets[CHARGEOSET_BEARD] = beardStyleData.beardGeoset;
    preferredGeosets[CHARGEOSET_SIDEBURN] = beardStyleData.sideBurnGeoset;
    preferredGeosets[CHARGEOSET_MOUSTACHE] = beardStyleData.moustacheGeoset;
  }
}

void CGUnit_C::InitializeNPCItems() {
  if ((m_obj->m_type & TYPE_PLAYER) || !m_geosetHandle || !m_displayInfoExtra) {
    return;
  }

  static const int s_inventoryTypes[10] = {1, 3, 4, 5, 6, 7, 8, 9, 10, 19};
  static const int s_inventorySlots[10] = {0, 2, 3, 4, 5, 6, 7, 8, 9, 18};

  HMODEL charModel = GetCharacterModel(0);
  FATALASSERT(charModel);
  HCHARGEOSET geosetHandle = m_geosetHandle;
  FATALASSERT(geosetHandle);
  HTEXCOMPONENT texComponent = m_texComponent;
  unsigned int *preferredGeosets = m_preferredGeosets;

  for (unsigned int i = 0; i < 10; ++i) {
    int displayID = m_displayInfoExtra->m_NPCItemDisplay[i];
    if (!displayID) {
      continue;
    }

    AddObjectComponentBySlot(s_inventorySlots[i], displayID, s_inventoryTypes[i], false, false, false, -1, false);

    const ItemDisplayInfoRec *displayInfoRec = g_itemDisplayInfoDB.GetRecord(displayID);
    if (texComponent) {
      CStatus status;
      TexComponentAdd(&status, GetDisplaySex(), texComponent, displayInfoRec, s_inventoryTypes[i], 1);
      if (!status.IsEmpty()) {
        char buffer[512];
        status.GetErrorStr(buffer, sizeof(buffer), STATUS_INFO);
        NTempest::C3Vector pos;
        GetPosition(pos);
        FATALERROR(("unit: 0x%I64X(%d:%s)(%g,%g,%g): %s", GetGUID(), m_obj->m_entryID, GetUnitName(), pos.x, pos.y, pos.z, buffer));
      }
    }

    CharCustomizationAddItemGeosets(geosetHandle, displayInfoRec, s_inventoryTypes[i], texComponent, GetDisplayRace(), 1);
    if (!i) {
      HeadGeosetHideCharGeosets(geosetHandle, displayInfoRec, GetDisplayRace(), preferredGeosets, 15);
    }
  }

  HandleClose(charModel);
}

void CGUnit_C::SignalDisplayHealthUpdate() const {
  Script_SendUnitSignal(m_obj->m_guid, 16);
}

void CGUnit_C::UpdateDisplayHealth() {
  if (!m_deathHolds) {
    m_displayHealth = m_unit->health;
    SignalDisplayHealthUpdate();
  }
}

bool CGUnit_C::IsSpellKnown(int spellID) const {
  return m_obj->m_guid == ClntObjMgrGetActivePlayer() ? CGSpellBook::IsSpellKnown(spellID) : CGSpellBook::IsPetSpellKnown(spellID);
}

const SkillLineAbilityRec *CGUnit_C::LookupAbility(int spellID) const {
  for (int index = 0; index < g_skillLineAbilityDB.GetNumRecords(); ++index) {
    const SkillLineAbilityRec *ability = g_skillLineAbilityDB.GetRecordByIndex(index);
    if (ability->m_spell != spellID) {
      continue;
    }
    int raceMask = ability->m_excludeRace ? ~ability->m_raceMask : ability->m_raceMask;
    int classMask = ability->m_excludeClass ? ~ability->m_classMask : ability->m_classMask;
    if (raceMask && !(raceMask & (1 << (m_unit->race - 1)))) {
      continue;
    }
    if (classMask && !(classMask & (1 << (m_unit->classId - 1)))) {
      continue;
    }
    return ability;
  }
  return 0;
}

void CGUnit_C::OnBadAttackTarget(unsigned __int64 victim) {
}

bool CGUnit_C::IsSpellSuperceded(int spellID) const {
  const SkillLineAbilityRec *ability = LookupAbility(spellID);
  while (ability && ability->m_supercededBySpell > 0) {
    if (IsSpellKnown(ability->m_supercededBySpell)) {
      return 1;
    }
    ability = LookupAbility(ability->m_supercededBySpell);
  }
  return 0;
}

int CGUnit_C::GetSpellSkillLine(int spellID) const {
  const SkillLineAbilityRec *ability = LookupAbility(spellID);
  return ability ? ability->m_skillLine : 0;
}

void CGUnit_C::SetImpactKitEffect(int spellID, CGUnit_C *target, const SpellVisualKitRec *impactKit, int immediate) {
  const SpellRec *spell = g_spellDB.GetRecord(spellID);
  if ((!spell || !IsShapeshiftSpell(spell)) && target && impactKit) {
    if (immediate) {
      target->PlayImpactKit(spellID, impactKit);
    } else {
      IMPACTEFFECTDESC *desc = s_freeImpactEffectDescs.Get(0);
      desc->Set(GetGUID(), target->GetGUID(), impactKit, spellID);
      typedef LIST(IMPACTEFFECTDESC) ImpactList;
      ImpactList &impactEffects = m_impactEffectsDesc;
      impactEffects.LinkNode(desc, LIST_TAIL, 0);
    }
  }
}

void CGUnit_C::PlayImpactKit(int spellID, const SpellVisualKitRec *impactKit) {
  FATALASSERT(impactKit);
  UnitEffectOneShot(g_spellVisualEffectNameDB.GetRecord(impactKit->m_headEffect), this, UNITEFFECT_ATTACHHEAD, 0, true, false);
  UnitEffectOneShot(g_spellVisualEffectNameDB.GetRecord(impactKit->m_chestEffect), this, UNITEFFECT_ATTACHCHEST, 0, true, false);
  UnitEffectOneShot(g_spellVisualEffectNameDB.GetRecord(impactKit->m_baseEffect), this, UNITEFFECT_ATTACHBASE, 0, true, false);

  SpellVisualsPlayCameraShakeID(impactKit->m_shakeID, GetPosition());
  SetSpellImpactKit(impactKit);
  if (impactKit->m_soundID) {
    SndInterfacePlaySpellSound(impactKit->m_soundID, this);
  }
  AddSpellProcOneShotEffect(spellID, impactKit);
}

void CGUnit_C::NPCAnimEndHandler() {
  ClearTorsoAnimation(0x40);
}

void CGUnit_C::PickNextRunHandler() {
  if (((m_unit->flags & 0x2000) && (m_flags & 0x10) && m_currentMountAnimState == ANIM_STATE_RUN) ||
      m_currentBaseAnimState == ANIM_STATE_RUN) {
    UpdateBaseAnimation(ANIM_STATE_RUN, 0x100);
  }
}

void CGUnit_C::CheckPendingImpactKit() {
  typedef LIST(IMPACTEFFECTDESC) ImpactList;
  ImpactList       &impactEffects = m_impactEffectsDesc;
  IMPACTEFFECTDESC *head;
  while ((head = impactEffects.Head()) != 0) {
    head->Unlink();
    FATALASSERT(head->impactKit);
    FATALASSERT(head->victim);
    CGUnit_C *victim = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(head->victim, __FILE__, __LINE__));
    if (victim) {
      victim->PlayImpactKit(head->spellID, head->impactKit);
    }
    s_freeImpactEffectDescs.Put(head);
  }
}

void CGUnit_C::SpellAnimEndHandler() {
  unsigned int state = m_currentTorsoAnimState;
  if (state == 38 || state == 47 || state == 62 || state == 63) {
    ClearTorsoAnimation(0x40);
  }
  HandleCastAnimEvent();
}

void CGUnit_C::AddSpellProcAuraEffect(int auraslot, const SpellVisualKitRec *rec) {
  if (!rec) {
    return;
  }

  if (rec->m_characterParam[0] >= 11 || ((1 << static_cast<unsigned int>(rec->m_characterParam[0])) & 0x640)) {
    return;
  }

  HMODEL charModel = GetCharacterModel(0);
  if (!charModel) {
    return;
  }

  SPELLEFFECTDESC *newDesc = s_spellEffectFreeList.Get(0);
  FATALASSERT(newDesc);
  newDesc->kitPtr = rec;
  newDesc->isOneShot = 0;

  unsigned int     spellID;
  if (auraslot == -1) {
    spellID = m_unit->channelSpell;
    SPELLEFFECTDESC **channelEffect = &m_channelSpellEffect;
    FATALASSERT(!*channelEffect);
    *channelEffect = newDesc;
  } else {
    spellID = m_unit->auras[auraslot];
    m_spellEffectLists[static_cast<unsigned int>(rec->m_characterParam[0])].LinkNode(newDesc, LIST_TAIL, 0);
  }

  SpellProcHandler handler = s_spellProcHandlerFunctions[static_cast<unsigned int>(rec->m_characterParam[0])];
  if (handler) {
    handler(SPELLPROCADD, m_spellEffectLists[static_cast<unsigned int>(rec->m_characterParam[0])], this, charModel, rec, newDesc, spellID, 0.0f);
  }
  HandleClose(charModel);
}

void CGUnit_C::RemoveSpellProcAuraEffect(ACTIVEAURAINFO *rec) {
  unsigned int             proc = static_cast<unsigned int>(-1);
  SPELLEFFECTDESC         *desc = 0;
  const SpellVisualKitRec *kitRec = 0;

  if (!rec) {
    SPELLEFFECTDESC **channelEffect = &m_channelSpellEffect;
    desc = *channelEffect;
    *channelEffect = 0;
    if (!desc) {
      return;
    }
    kitRec = desc->kitPtr;
    if (kitRec) {
      proc = kitRec->m_characterParam[0];
    }
  } else {
    if (!rec->stateKitRec) {
      return;
    }
    FATALASSERT(rec->auraSlot < 56);
    kitRec = rec->stateKitRec;
    desc = FindSpellEffectProcDesc(kitRec);
    proc = kitRec->m_characterParam[0];
    rec->stateKitRec = 0;
  }

  if (!desc) {
    return;
  }

  if (proc >= 11 || ((1 << proc) & 0x640)) {
    desc->ClearLightningObjects();
    desc->Unlink();
    s_spellEffectFreeList.Put(desc);
    return;
  }

  HMODEL charModel = GetCharacterModel(0);
  if (!charModel) {
    desc->ClearLightningObjects();
    desc->Unlink();
    s_spellEffectFreeList.Put(desc);
    return;
  }

  SpellProcHandler handler = s_spellProcHandlerFunctions[proc];
  if (handler) {
    SpellEffectList &list = m_spellEffectLists[proc];
    handler(SPELLPROCREMOVE, list, this, charModel, kitRec, desc, 0, 0.0f);
  }
  HandleClose(charModel);
}

void CGUnit_C::OnNotStanding(unsigned __int64 victim) {
}

void CGUnit_C::DeathAnimEndHandler() {
  if (m_animFlags & 1) {
    m_animFlags &= ~1u;
    ForceUpdateBaseAnimation();
    return;
  }

  if (!m_unit->health) {
    m_deathTime = OsGetAsyncTimeMs();
    HMODEL charModel = GetCharacterModel(0);
    FATALASSERT(charModel);
    ANIMENUMERATION anim = (m_animFlags & 0x400) && IsUnderWater() ? ANIM_DROWNED : ANIM_DEAD;
    ObjectModelSetSequence(charModel, anim, 2, 0);
    HandleClose(charModel);
  }
}

SPELLEFFECTDESC *CGUnit_C::FindSpellEffectProcDesc(const SpellVisualKitRec *rec) {
  unsigned int proc = rec->m_characterParam[0];
  if (proc >= 11) {
    return 0;
  }
  SpellEffectList &list = m_spellEffectLists[proc];
  ITERATELIST(SPELLEFFECTDESC, list, desc) {
    if (desc->kitPtr == rec) {
      return desc;
    }
  }
  return 0;
}

int CGUnit_C::JumpTakeOffFinishedHandler() {
  ObjectModelSetSequence(m_model, ANIM_JUMP, GetObjAnimFlags(2), 0);
  return 1;
}

int CGUnit_C::JumpLandFinishedHandler() {
  CGUnit_C::UpdateBaseAnimation(0);
  return 1;
}

void CGUnit_C::SitSleepAnimEndHandler() {
  UpdateBaseAnimation(0);
}

void CGUnit_C::InternalProcessSpellProcEffects(SPELLPROC_ACTION action, float elapsed) {
  HMODEL charModel = GetCharacterModel(0);
  if (!charModel) {
    return;
  }

  for (unsigned int proc = 0; proc < 11; ++proc) {
    SpellProcHandler handler = s_spellProcHandlerFunctions[proc];
    SpellEffectList &list = m_spellEffectLists[proc];
    if (handler && list.Head()) {
      handler(action, list, this, charModel, 0, 0, 0, elapsed);
    }
  }
  HandleClose(charModel);
}

void CGUnit_C::RefreshSpellProcEffects() {
  InternalProcessSpellProcEffects(SPELLPROCREFRESH, 0.0f);
}

void CGUnit_C::UpdateSpellProcEffects(float elapsedTime) {
  InternalProcessSpellProcEffects(SPELLPROCUPDATE, elapsedTime);
}

void CGUnit_C::AddEmissiveColor(const NTempest::CImVector &color) {
  int *current = reinterpret_cast<int *>(&m_currentEmissive);
  current[0] += color.r;
  current[1] += color.g;
  current[2] += color.b;

  HMODEL model = GetCharacterModel(0);
  if (model) {
    NTempest::CImVector emissive(
        0xFF000000 | (static_cast<unsigned int>(current[0] < 255 ? current[0] : 255) << 16) |
        (static_cast<unsigned int>(current[1] < 255 ? current[1] : 255) << 8) | static_cast<unsigned int>(current[2] < 255 ? current[2] : 255)
    );
    ModelSetEmissiveColor(model, emissive, 0);
  }
}

void CGUnit_C::RemoveEmissiveColor(const NTempest::CImVector &color) {
  NTempest::C3iVector acc(color.r, color.g, color.b);
  m_currentEmissive -= acc;

  HMODEL model = GetCharacterModel(0);
  if (model) {
    FATALASSERT(m_currentEmissive.x >= 0 && m_currentEmissive.y >= 0 && m_currentEmissive.z >= 0);
    ModelSetEmissiveColor(model, color, 0);
  }
}

int CGUnit_C::GetAnimPriority(int state) {
  FATALASSERT(state >= 0);
  FATALASSERT(state < sizeof(s_animInfo) / sizeof(s_animInfo[0]));
  return s_animInfo[state].basePriority;
}

int CGUnit_C::IsInStandSitTransition() {
  if (m_flags & 0x40000) {
    return 1;
  }
  return IsSitStandSleepTransition(m_currentBaseAnimState);
}

int IsSitStandSleepTransition(unsigned int animState) {
  return animState < 64 ? (s_animInfo[animState].flags >> 16) & 1 : 0;
}

int CGUnit_C::IsInSitSleepPosition() {
  if (m_flags & 0x40000) {
    return 0;
  }

  unsigned int animState = m_currentBaseAnimState;
  return animState < 64 ? (s_animInfo[animState].flags >> 19) & 1 : 0;
}

void CGUnit_C::PrintAttackSeqErrorMsg(unsigned int sequence, unsigned int fallBack) const {
  ASSERT(sequence < NUM_OBJECTANIMATIONS);
  ASSERT(fallBack < NUM_OBJECTANIMATIONS);
  SysMsgPrintf(SYSMSG_ERROR, 8, "COMBATANIMFALLBACK|%s|%d|%d", GetUnitName(), sequence, fallBack);
}

UNITAFFILIATION CGUnit_C::GetGUIDAffiliation(unsigned __int64 unit) const {
  const unsigned __int64 &controller = m_unit->charmedBy ? m_unit->charmedBy : m_unit->createdBy;
  if (unit == controller) {
    return AFFILIATION_YOURCONTROLLER;
  }

  const unsigned __int64 &pet = m_unit->charm ? m_unit->charm : m_unit->summon;
  if (unit == pet) {
    return AFFILIATION_YOURPET;
  }
  if (unit == m_obj->m_guid) {
    return AFFILIATION_YOURSELF;
  }
  return AFFILIATION_OTHER;
}

int CGUnit_C::GetSpellRank(int spellID) const {
  ASSERT(!IsA(TYPE_PLAYER));

  const SpellRec *spell = g_spellDB.GetRecord(spellID);
  if (!spell) {
    return 0;
  }

  int rank = 5 * m_unit->level;
  if (spell->m_maxLevel > 0 && rank >= 5 * spell->m_maxLevel) {
    rank = 5 * spell->m_maxLevel;
  }

  return rank < 0 ? 0 : rank;
}

void CGUnit_C::OnLevelChange() {
  if (ShouldDelayLevelupAnim()) {
    m_animFlags |= 0x4000;
  } else {
    PerformLevelUpAnim(1);
  }
}

void CGUnit_C::PerformLevelUpAnim(int force) {
  unsigned int &flags = m_animFlags;
  if (force || (flags & 0x4000)) {
    flags |= 0x8000;
  }
  flags &= ~0x4000u;
}

void CGUnit_C::CheckLevelUpAnimFlag(int oldState, int newState) {
  if (ShouldDelayLevelupAnim(oldState) != ShouldDelayLevelupAnim(newState)) {
    PerformLevelUpAnim(0);
  }
}

void CGUnit_C::AddWorldText(WORLDTEXTMISSTYPE type) {
  struct WORLDTEXTINFO {
    const char   *string;
    WORLDTEXTTYPE type;

    WORLDTEXTINFO(const char *string_, WORLDTEXTTYPE type_) : string(string_), type(type_) {
    }
  };

  static WORLDTEXTINFO s_worldTextInfo[WORLDTEXTMISS_NUMTYPES] = {WORLDTEXTINFO("EVADE_CAPS", WT_MISS),     WORLDTEXTINFO("DODGE_CAPS", WT_MISS),
                                                                  WORLDTEXTINFO("PARRY_CAPS", WT_MISS),     WORLDTEXTINFO("BLOCK_CAPS", WT_MISS),
                                                                  WORLDTEXTINFO("DEFLECTED_CAPS", WT_MISS), WORLDTEXTINFO("IMMUNE_CAPS", WT_MISS),
                                                                  WORLDTEXTINFO("IMMUNE_CAPS", WT_MISS),    WORLDTEXTINFO("MISS_CAPS", WT_MISS),
                                                                  WORLDTEXTINFO("RESIST_CAPS", WT_MISS),    WORLDTEXTINFO("ABSORB_CAPS", WT_ABSORB)};

  FATALASSERT(type < WORLDTEXTMISS_NUMTYPES);
  char buf[64] = "";
  SStrCopy(buf, FrameScript_GetText(s_worldTextInfo[type].string, -1, GENDER_NOT_APPLICABLE), sizeof(buf));
  HPLAYERNAME__ *name = m_unitNameHandle;
  PlayerNameCreateText(name, s_worldTextInfo[type].type, buf, 0);
}

void CGUnit_C::AddWorldText(MISS_REASON reason) {
  static WORLDTEXTMISSTYPE s_worldMissTextReasons[10] = {
      WORLDTEXTMISS_NUMTYPES, WORLDTEXTMISS_PHYSICAL, WORLDTEXTMISS_RESIST, WORLDTEXTMISS_IMMUNE,    WORLDTEXTMISS_EVADED,
      WORLDTEXTMISS_DODGED,               WORLDTEXTMISS_PARRIED,  WORLDTEXTMISS_BLOCKED, WORLDTEXTMISS_TEMPIMMUNE, WORLDTEXTMISS_DEFLECTED
  };

  reason = AdjustVictimState(reason);
  if (reason < MISS_NUMMISSTYPES) {
    AddWorldText(s_worldMissTextReasons[reason]);
  }
}

void CGUnit_C_RenderBowStrings(const NTempest::C3Vector &c) {
  if (!s_bowStringIndices.Count()) {
    return;
  }

  NTempest::C44Matrix m;
  m.Translate(-c);
  GxXformPush(GxXform_World, m);
  GxRsPush();
  GxRsSet(GxRs_DepthTest, 1);
  GxRsSet(GxRs_DepthWrite, 1);

  static NTempest::CImVector s_white(0xFFFFFFFF);
  GxPrimLockVertexPtrs(s_bowStringVerts.Count(), s_bowStringVerts.Ptr(), sizeof(NTempest::C3Vector), 0, 0, &s_white, 0, 0, 0, 0, 0, 0, 0);
  GxPrimDrawElements(GxPrim_Lines, s_bowStringIndices.Count(), s_bowStringIndices.Ptr());
  GxPrimUnlockVertexPtrs();

  s_bowStringVerts.SetCount(0);
  s_bowStringIndices.SetCount(0);
  GxRsPop();
  GxXformPop(GxXform_World);
}

void CGUnit_C::DrawBowString(const NTempest::C3Vector &cameraPos) {
  unsigned int torsoAnim = GetCurrentTorsoAnim();
  if (torsoAnim != ANIM_ATTACKBOW && torsoAnim != ANIM_LOADBOW && torsoAnim != ANIM_HOLDBOW) {
    return;
  }

  if (torsoAnim == ANIM_HOLDBOW) {
    m_flags |= 0x400;
    if (!(m_flags & 0x80000)) {
      m_flags |= 8;
    }
  }

  HMODEL bowModel = GetRangedWeaponModel();
  if (!bowModel) {
    return;
  }

  HMODEL charModel = GetCharacterModel(0);
  if (charModel) {
    NTempest::C3Vector bowPoints[3];
    if (ModelGetEventObjectPosition(bowModel, 0, 0, &bowPoints[0]) && ModelGetEventObjectPosition(bowModel, 1, 0, &bowPoints[1])) {
      bowPoints[0] += cameraPos;
      bowPoints[1] += cameraPos;

      NTempest::C34Matrix s;
      GetWorldMatrix(&s);
      NTempest::C34Matrix worldToChar = s.AffineInverse(GetScale() * GetRenderScale());
      bowPoints[0] *= worldToChar;
      bowPoints[1] *= worldToChar;

      unsigned int pointCount = 2;
      if ((torsoAnim == ANIM_LOADBOW || torsoAnim == ANIM_HOLDBOW) && (torsoAnim == ANIM_HOLDBOW || (m_flags & 0x400)) &&
          ModelGetEventObjectPosition(charModel, 19, 1, &bowPoints[2]))
      {
        bowPoints[2] *= worldToChar;
        pointCount = 3;
      }

      unsigned short currentCount = static_cast<unsigned short>(s_bowStringVerts.Count());
      s_bowStringVerts.Add(pointCount, bowPoints);

      if (pointCount == 2) {
        unsigned short indices[2] = {currentCount, static_cast<unsigned short>(currentCount + 1)};
        s_bowStringIndices.Add(2, indices);
      } else {
        unsigned short indices[4] = {
            currentCount, static_cast<unsigned short>(currentCount + 2), static_cast<unsigned short>(currentCount + 2),
            static_cast<unsigned short>(currentCount + 1)
        };
        s_bowStringIndices.Add(4, indices);
      }
    }
    HandleClose(charModel);
  }
  HandleClose(bowModel);
}

void CGUnit_C::SetRangedWeaponReleaseAnim() {
  HMODEL model = GetRangedWeaponModel();
  if (model) {
    ModelSetRandomSequenceFidget(model, 3, 0);
    HandleClose(model);
  }
}

void CGUnit_C::SetBaseAnim(unsigned int newAnim) {
  SetSheatheReason(SHEATHE_BASEANIM, AnimSheathesWeapon(newAnim), 0);
  FATALASSERT(newAnim != NUM_OBJECTANIMATIONS);
  m_currentBaseAnim = newAnim;
}

void CGUnit_C::SetTorsoAnim(unsigned int newAnim) {
  SetSheatheReason(SHEATHE_TORSOANIM, AnimSheathesWeapon(newAnim), 0);
  m_currentTorsoAnim = newAnim;
}

static const char *const s_animQueueNames[15] = {"ANIMQUEUE_NONE",        "ANIMQUEUE_ATTACK",         "ANIMQUEUE_WOUND",
                                                 "ANIMQUEUE_SITDOWN",     "ANIMQUEUE_SITUP",          "ANIMQUEUE_SLEEPDOWN",
                                                 "ANIMQUEUE_SLEEPUP",     "ANIMQUEUE_SITCHAIR",       "ANIMQUEUE_SITCHAIRUP",
                                                 "ANIMQUEUE_SITCHAIRLOW", "ANIMQUEUE_SITCHAIRMEDIUM", "ANIMQUEUE_SITCHAIRHIGH",
                                                 "ANIMQUEUE_DEAD",        "ANIMQUEUE_KNEELDOWN",      "ANIMQUEUE_KNEELUP"};

void CGUnit_C::ShowHandArrow(int show) {
  HMODEL model = GetCharacterModel(0);
  if (!model) {
    return;
  }

  if (show) {
    if (!(m_flags & 0x80000)) {
      m_flags |= 8;
    }
  } else {
    ModelClearLink(model, 35);
    m_flags &= ~0x80000u;
  }
  HandleClose(model);
}

HMODEL CGUnit_C::GetRangedWeaponModel() {
  if (m_unit->weaponMode != WEAPONMODE_RANGEDMODE) {
    return 0;
  }

  HMODEL characterModel = GetCharacterModel(0);
  if (!characterModel) {
    return 0;
  }

  const VirtualItemInfo *itemStats = GetVirtualItem(VIRTUAL_MONSTER_SLOT_RANGED, 0);

  unsigned int linkPoint;
  if (!itemStats || itemStats->m_inventoryType == INDEX_THROWN_TYPE) {
    HandleClose(characterModel);
    return 0;
  }
  if (itemStats->m_inventoryType == INDEX_RANGED_TYPE) {
    linkPoint = 2;
  } else if (itemStats->m_inventoryType == INDEX_RANGEDRIGHT_TYPE) {
    linkPoint = 1;
  } else {
    HandleClose(characterModel);
    return 0;
  }

  HMODEL       handle = 0;
  unsigned int numModels = 1;
  if (ModelGetLinkPoint(characterModel, linkPoint, &handle, &numModels)) {
    HandleClose(characterModel);
    return handle;
  }

  return 0;
}

void CGUnit_C::ClearRangedStandTimer() {
  unsigned int &timer = m_rangedStandTimer;
  if (timer) {
    ClientKillTimer(timer, RangedStandTimerHandler, "RangedStandTimerHandler");
    timer = 0;
  }
}

void CGUnit_C::MaybeSaveChannelSpellTargets(
    int spellID,
    const TSStackArray<unsigned __int64> &targets
) {
  m_savedChannelSpellID = spellID;
  m_savedChannelSpellTargets.SetCount(targets.Count());
  unsigned int i;
  for (i = 0; i < targets.Count(); ++i) {
    m_savedChannelSpellTargets[i] = targets[i];
  }
}

void CGUnit_C::AddHitAnimHolds(
    int spellID,
    const TSStackArray<unsigned __int64> &targets
) {
  m_pendingHitSpellID = spellID;
  unsigned int oldCount = m_pendingHitAnimVictims.Count();
  m_pendingHitAnimVictims.SetCount(oldCount + targets.Count());
  unsigned int i;
  for (i = 0; i < targets.Count(); ++i) {
    m_pendingHitAnimVictims[oldCount + i] = targets[i];
  }
}

void CGUnit_C::ClearSpellCastAnimInfo() {
  m_spellCastingEffectKit = 0;
  m_spellCastingSoundID = 0;
}

void CGUnit_C::StoreSpellMissileEffect(
    const unsigned __int64 &target,
    const NTempest::C3Vector &destination,
    float speed,
    unsigned int ammoDisplayID,
    int inventoryType,
    const SpellVisualRec *rec,
    bool hits,
    MISS_REASON reason,
    unsigned int spellID,
    bool wasProc
) {
  FATALASSERT(rec);
  CheckPendingMissileRelease(0);
  unsigned int sound = rec->m_missileSound;
  m_spellMissileStruct.caster = this;
  m_spellMissileStruct.target = target;
  m_spellMissileStruct.destination = destination;
  m_spellMissileStruct.missileEffect = rec->m_missileModel;
  m_spellMissileStruct.speed = speed;
  m_spellMissileStruct.missilePathType = rec->m_missilePathType;
  m_spellMissileStruct.ammoDisplayID = ammoDisplayID;
  m_spellMissileStruct.missileVictimEffect = rec->m_impactKit;
  m_spellMissileStruct.spellID = spellID;
  m_spellMissileStruct.inventoryType = inventoryType;
  m_spellMissileStruct.hits = hits;
  m_spellMissileStruct.reason = reason;
  m_spellMissileStruct.sound = sound;
  if (wasProc || !m_currentTorsoAnimState) {
    CheckPendingMissileRelease(0);
  }
}

void CGUnit_C::SetRangedStandTimer() {
  ClearRangedStandTimer();
  m_rangedStandTimer = ClientSetTimer(3000, RangedStandTimerHandler, this);
}

void CGUnit_C::OnRangedStandTimer() {
  m_rangedStandTimer = 0;
  DetermineReadySequence(1);
  if (m_currentBaseAnimState != 1) {
    CGUnit_C::UpdateBaseAnimation(0x100);
  }
}

void CGUnit_C::StopRangedAttackPrecast() {
  if (m_unit->weaponMode != WEAPONMODE_RANGEDMODE) {
    HMODEL model = GetRangedWeaponModel();
    if (model) {
      ModelSetSequence(model, 0, 0);
      HandleClose(model);
    }
  }
}

void CGUnit_C::SetRangedWeaponPullAnim(int duration) {
  if (m_unit->weaponMode == WEAPONMODE_RANGEDMODE) {
    HMODEL model = GetRangedWeaponModel();
    if (model) {
      unsigned int seqDuration;
      if (ModelSetRandomSequenceFidget(model, 2, 0) && ModelGetSequenceDuration(model, 2, &seqDuration)) {
        if (duration) {
          ModelSetTimeScale(model, static_cast<float>(seqDuration) / static_cast<float>(duration), 0);
          HandleClose(model);
          return;
        }
        ModelForceSequenceTime(model, 2, 0x7FFFFFFF, 0);
      }
      HandleClose(model);
    }
  }
}

int CGUnit_C::UpdateTexComponentLoadStatus() {
  int sectionsReady = !m_texComponent || TexComponentCheckSections(m_texComponent, 0);
  if (m_flags & 0x100) {
    return 0;
  }
  if (!sectionsReady) {
    return 0;
  }
  m_flags |= 0x100;
  UpdatePortraitTexture(GetGUID());
  return 1;
}

void CGUnit_C::CommitTexture(int force) {
  char    errorString[512];
  CStatus status;
  TexComponentCommitSections(&status, m_texComponent, force);
  if (!status.IsEmpty()) {
    status.GetErrorStr(errorString, sizeof(errorString), STATUS_INFO);
    NTempest::C3Vector pos = GetPosition();
    FATALERROR(("creatureID %d (%s)(%g,%g,%g): %s", m_obj->m_entryID, GetUnitName(), pos.x, pos.y, pos.z, errorString));
  }
}

void CGUnit_C::DDWRITELOG(const char *buffer) {
  unsigned int current = m_deathHoldBuffer.Count();
  unsigned int bytes = SStrLen(buffer) + 1;
  m_deathHoldBuffer.SetCount(current + bytes);
  memcpy(&m_deathHoldBuffer[current], buffer, bytes);
  *m_deathHoldBufferIndices.New() = current;
}

void CGUnit_C::DDADDLOG(unsigned __int64 guid, const char *string, const char *file, unsigned int line) {
  char buffer[512];
  SStrPrintf(buffer, sizeof(buffer), "[DDADD 0x%016I64X (%d)]: %s (%s:%d)", guid, m_deathHolds, string, file, line);
  DDWRITELOG(buffer);
  AddDeathHold();
}

void CGUnit_C::DDDELLOG(unsigned __int64 guid, const char *string, const char *file, unsigned int line) {
  char buffer[512];
  SStrPrintf(buffer, sizeof(buffer), "[DDDEL 0x%016I64X (%d)]: %s (%s:%d)", guid, m_deathHolds, string, file, line);
  DDWRITELOG(buffer);
  DelDeathHold();
}

void CGUnit_C::DDGENLOG(unsigned __int64 guid, const char *string, const char *file, unsigned int line) {
  char buffer[512];
  SStrPrintf(buffer, sizeof(buffer), "[DDGEN 0x%016I64X (%d)]: %s (%s:%d)", guid, m_deathHolds, string, file, line);
  DDWRITELOG(buffer);
}

void DDGenerateLogString(HSLOG handle, TSGrowableArray<char> *stringBuffer, char *format, ...) {
  if (!handle && !stringBuffer) {
    return;
  }

  char    buf[1024];
  va_list arguments;
  va_start(arguments, format);
  SStrVPrintf(buf, sizeof(buf), format, reinterpret_cast<char *>(arguments));
  va_end(arguments);

  if (handle) {
    SLogWrite(handle, buf);
  }

  if (stringBuffer) {
    unsigned int length = SStrLen(buf);
    unsigned int newline = length > 1022 ? 1022 : length;
    buf[newline] = '\r';
    buf[newline + 1] = '\n';

    unsigned int count = length + 1;
    unsigned int current = stringBuffer->Count();
    stringBuffer->SetCount(current + count);
    memcpy(&(*stringBuffer)[current], buf, count);
  }
}

void CGUnit_C::RangedWeaponAnimEndHandler() {
  unsigned int &flags = m_flags;
  if (!(flags & 0x800)) {
    SetRangedWeaponReleaseAnim();
    flags |= 0x800;
  }
  DetermineReadySequence(0);
  ClearTorsoAnimation(0x40);
}

void CGUnit_C::ThrowAnimEndHandler() {
  CheckPendingThrownWeaponReattach(0);
  DetermineReadySequence(0);
  ClearTorsoAnimation(0x40);
}

void CGUnit_C::RangedPrecastEndHandler() {
  if (GetCurrentTorsoAnimState() == 37) {
    unsigned int anim = SpellGetRangedPrecastHoldAnim(GetCurrentTorsoAnim());
    if (anim != INVALID_ANIMATION && SetSpellPreCastingAnimation(static_cast<ANIMENUMERATION>(anim))) {
      if (anim == 109) {
        SetRangedWeaponPullAnim(0);
      }
      SetTorsoAnimation(37, 0, 0);
    }
  }
}

void CGUnit_C::DumpGeneralDeathHoldLog(HSLOG handle, TSGrowableArray<char> *stringBuffer) const {
  if (!handle && !stringBuffer) {
    return;
  }

  CGUnit_C *activePlayer = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  char      labelString[64] = "";
  if (activePlayer) {
    if (activePlayer->GetGUID() == m_obj->m_guid) {
      SStrPrintf(labelString, sizeof(labelString), " (Local Player)");
    } else {
      if (static_cast<CGPlayer_C *>(activePlayer)->CGPlayer_C::GetLocalTarget() == m_obj->m_guid) {
        SStrPrintf(labelString, sizeof(labelString), " (Local Player's Target)");
      }
    }
  }

  DDGenerateLogString(handle, stringBuffer, "======================================");
  DDGenerateLogString(handle, stringBuffer, "Death Holds for unit %s%s (0x%016I64X)", GetUnitName(), labelString, m_obj->m_guid);

  NTempest::C3Vector pos;
  GetPosition(pos);
  DDGenerateLogString(handle, stringBuffer, "Unit at %g, %g, %g", pos.x, pos.y, pos.z);

  DDGenerateLogString(handle, stringBuffer, "Current Torso Anim State: %d", m_currentTorsoAnimState);
  DDGenerateLogString(handle, stringBuffer, "Current Torso Anim: %d", GetCurrentTorsoAnim());
  DDGenerateLogString(handle, stringBuffer, "Current Base Anim State: %d", m_currentBaseAnimState);
  DDGenerateLogString(handle, stringBuffer, "Current Base Anim: %d", m_currentBaseAnim);

  if (!m_deathHolds) {
    DDGenerateLogString(handle, stringBuffer, "No outstanding death holds.");
    return;
  }

  for (unsigned int i = 0; i < m_deathHoldBufferIndices.Count(); ++i) {
    DDGenerateLogString(handle, stringBuffer, "%s", &m_deathHoldBuffer[m_deathHoldBufferIndices[i]]);
  }

  const unsigned int *animNode = reinterpret_cast<const unsigned int *>(m_animQueue.Head());
  if (animNode) {
    DDGenerateLogString(handle, stringBuffer, "The following anims are queued:");
    while (animNode) {
      unsigned int type = animNode[2];
      if (type >= 15) {
        DDGenerateLogString(handle, stringBuffer, "Unknown queue type: %d", type);
      } else {
        DDGenerateLogString(handle, stringBuffer, "%s", s_animQueueNames[type]);
      }
      animNode = reinterpret_cast<const unsigned int *>(animNode[1]);
    }
  } else {
    DDGenerateLogString(handle, stringBuffer, "No anims queued");
  }

  const IMPACTEFFECTDESC *impact = m_impactEffectsDesc.Head();
  if (impact) {
    DDGenerateLogString(handle, stringBuffer, "The following impact effects are queued:");
    while (impact) {
      const char *victimName = "UNKNOWNVICTIM";
      CGObject_C *victim = ClntObjMgrObjectPtr(impact->victim, __FILE__, __LINE__);
      if (victim) {
        victimName = static_cast<CGUnit_C *>(victim)->GetUnitName();
      }
      DDGenerateLogString(handle, stringBuffer, "victim %s (%016I64X) : spellvisual %d\n", victimName, impact->victim, impact->impactKit->m_ID);
      impact = impact->Next();
    }
  } else {
    DDGenerateLogString(handle, stringBuffer, "No impact effects are queued:");
  }
}

int CGUnit_C::SetCastingSpell(int spellID, bool force, bool precastAnimSuccessful) {
  int &castingSpell = m_castingSpell;
  if (castingSpell && !force) {
    return 1;
  }

  if (spellID != castingSpell) {
    if (spellID) {
      HandlePrecastStart(precastAnimSuccessful);
      castingSpell = spellID;
      return 0;
    }

    HandlePrecastStop(castingSpell, 0);
  }

  castingSpell = spellID;
  return 0;
}

void CGUnit_C::OnChannelSpellChanged(unsigned int oldSpell) {
  if (oldSpell) {
    for (unsigned int attach = 0; attach < 12; ++attach) {
      if (m_auraVisual[attach].GetSpellID() == oldSpell) {
        RemoveAuraVisual(static_cast<UNITEFFECTATTACHPPOINT>(attach));
      }
    }
    RemoveSpellProcAuraEffect(0);
  }

  int spellID = m_unit->channelSpell;
  if (spellID) {
    const SpellRec *spellRec = g_spellDB.GetRecord(spellID);
    if (!spellRec) {
      return;
    }

    TSGrowableArray<unsigned __int64> &targets =
        m_savedChannelSpellTargets;
    if (targets.Count() && (spellRec->m_attributesEx & 0x4000)) {
      SaveTrackingTarget(targets[0], TRACKTYPE_SPELLCHANNEL, 0);
    }

    const SpellVisualRec    *visualRec = g_spellVisualDB.GetRecord(spellRec->m_spellVisualID);
    const SpellVisualKitRec *channelKit = visualRec ? g_spellVisualKitDB.GetRecord(visualRec->m_channelKit) : 0;
    if (!channelKit) {
      return;
    }

    if (!(spellRec->m_attributes & 0x40000)) {
      SetSheatheReason(SHEATHE_CHANNELLING, 1, 0);
    }
    AddSpellProcAuraEffect(-1, channelKit);
    AddKitAuras(channelKit, spellRec);
    PlaySpellLoopedSound(channelKit->m_soundID);
    if (channelKit->m_anim && channelKit->m_anim != -1) {
      m_spellCastingAnim = static_cast<ANIMENUMERATION>(channelKit->m_anim);
      SetTorsoAnimation(62, 0, 0);
    }
    return;
  }

  SetSheatheReason(SHEATHE_CHANNELLING, 0, 0);
  const SpellRec *oldSpellRec = g_spellDB.GetRecord(oldSpell);
  if (GetTrackingTarget() && oldSpellRec && (oldSpellRec->m_attributesEx & 0x4000)) {
    ClearTrackingTarget(0);
  }
  if (m_currentTorsoAnimState == 62) {
    ClearTorsoAnimation(0);
  }
  RefreshAuraVisuals();
  KillSpellLoopedSound();
}

void CGUnit_C::ClearSavedChannelSpellTargets() {
  TSGrowableArray<unsigned __int64> &targets = m_savedChannelSpellTargets;
  targets.Clear();
}

void CGUnit_C::SetStandStateAnim(int standAnim) {
  if (standAnim != m_standStateAnim) {
    m_standStateAnim = standAnim;
    if (m_currentBaseAnimState == 3) {
      UpdateBaseAnimation(3, 0x100);
    }
  }
}

void CGUnit_C::SetWalkStateAnim(int walkAnim) {
  if (walkAnim != m_walkStateAnim) {
    FATALASSERT(walkAnim < NUM_OBJECTANIMATIONS);
    m_walkStateAnim = walkAnim;
    if (m_currentBaseAnimState == 5) {
      UpdateBaseAnimation(5, 0x100);
    }
  }
}

int CGUnit_C::GetStandStateAnim(HMODEL model) const {
  if (!model) {
    model = m_model;
  }

  unsigned int standStateAnim = m_standStateAnim;
  return ModelHasSequenceId(model, standStateAnim) ? standStateAnim : ANIM_STAND;
}

int CGUnit_C::GetWalkStateAnim() const {
  HMODEL model = m_model;
  if (ModelHasSequenceId(model, m_walkStateAnim)) {
    return m_walkStateAnim;
  }
  return ANIM_WALK;
}

const SpellVisualRec *CGUnit_C::GetAppropriateSpellVisual(const SpellRec *spellRec, SpellVisualRec &filled) const {
  FATALASSERT(spellRec);

  const SpellVisualRec *itemVisual = 0;
  const SpellVisualRec *spellVisual = g_spellVisualDB.GetRecord(spellRec->m_spellVisualID);
  if (spellRec->m_attributes & 2) {
    int displayID = GetVirtualItemDisplayID(2);
    const ItemDisplayInfoRec *displayInfo = g_itemDisplayInfoDB.GetRecord(displayID);
    if (displayInfo && displayInfo->m_spellVisualID) {
      itemVisual = g_spellVisualDB.GetRecord(displayInfo->m_spellVisualID);
    }
  }

  if (spellVisual) {
    memcpy(&filled, spellVisual, sizeof(filled));
  } else {
    if (!itemVisual) {
      return 0;
    }
    memset(&filled, 0, sizeof(filled));
  }

  if (itemVisual) {
    if (!filled.m_precastKit) {
      filled.m_precastKit = itemVisual->m_precastKit;
    }
    if (!filled.m_castKit) {
      filled.m_castKit = itemVisual->m_castKit;
    }
    if (!filled.m_impactKit) {
      filled.m_impactKit = itemVisual->m_impactKit;
    }
    if (!filled.m_hasMissile && itemVisual->m_hasMissile) {
      filled.m_hasMissile = 1;
      filled.m_missileModel = itemVisual->m_missileModel;
      filled.m_missilePathType = itemVisual->m_missilePathType;
      filled.m_missileDestinationAttachment = itemVisual->m_missileDestinationAttachment;
    }
    if (!filled.m_animEventSoundID) {
      filled.m_animEventSoundID = itemVisual->m_animEventSoundID;
    }
  }

  return &filled;
}

void AuraVisual::Clear() {
  if (flags & 1) {
    if (theModel) {
      if (flags & 2) {
        CWorld::RemoveObject(obj);
      } else {
        HandleClose(theModel);
      }
      theModel = 0;
    }
    flags = 0;
    spellID = 0;
  }
}

void ACTIVEATTACHMENTINFO::Hide(CGUnit_C *unitPtr, HMODEL charModel, HMODEL paperDollModel, bool hide) {
  FATALASSERT(charModel);
  if (hide == ((flags & 2) != 0)) {
    return;
  }

  if (hide) {
    flags |= 2;
  } else {
    flags &= ~2;
  }

  for (int i = 0; i < 2; ++i) {
    if (!modelInfo[i].model) {
      continue;
    }

    if (hide) {
      int link = flags & 1 ? sheathAttachmentSlot : modelInfo[i].attachmentPoint;
      ModelClearLink(charModel, link);
      if (paperDollModel) {
        ModelClearLink(paperDollModel, link);
      }
    } else {
      unitPtr->SheatheObjComponent(invSlot, flags & 1);
    }
  }
}

void ATTACHMENTMODELINFO::ClearAttachmentFromModel(HMODEL charModel, HMODEL paperDollModel) {
  if (currentLink >= 0) {
    if (paperDollModel) {
      ModelClearLink(paperDollModel, currentLink);
    }
    if (charModel) {
      ModelClearLink(charModel, currentLink);
    }
    currentLink = -1;
  }
}

void CGUnit_C::VirtualComponentChanged(int slot, int oldValue) {
  if (oldValue) {
    DetachVirtualComponent(slot, false, true);
  }
  AttachVirtualComponent(slot, 0);
  if (slot == 2) {
    SetSheatheReason(SHEATHE_RANGED, m_unit->weaponMode == WEAPONMODE_RANGEDMODE, 1);
  }
}

int CGUnit_C::CanHighlight() const {
  return !m_stats || !(m_stats->m_flags & 0x200);
}

void CGUnit_C::RefreshInteractIcon() {
  RemoveInteractIcon();
  CGPlayer_C *player =
      static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    if (m_unit->npcFlags & 2) {
      player->UpdateQuestStatus(this);
    }
    if (m_unit->npcFlags & 4) {
      player->UpdateTaxiStatus(this);
    }
    if (m_unit->npcFlags & 0x10) {
      player->UpdateBindStatus(this);
    }
  }
}

void CGUnit_C::DetachVirtualComponent(int vslot, bool defer, bool removeRecord) {
  FATALASSERT(!IsA(TYPE_PLAYER));
  FATALASSERT(vslot < NUM_VIRTUAL_MONSTER_SLOTS);

  int invSlot;
  switch (vslot) {
    case VIRTUAL_MONSTER_SLOT_MAINHAND:
      invSlot = INVSLOT_MAINHAND;
      break;
    case VIRTUAL_MONSTER_SLOT_OFFHAND:
      invSlot = INVSLOT_OFFHAND;
      break;
    case VIRTUAL_MONSTER_SLOT_RANGED:
      invSlot = INVSLOT_RANGED;
      break;
    default:
      return;
  }

  RemoveObjectComponentByInvSlot(invSlot, defer, removeRecord);
}

void CGUnit_C::AddObjectComponentBySlot(
    int  invSlot,
    int  displayID,
    int  inventoryType,
    bool forceAlternate,
    bool deferApply,
    bool sheathe,
    int  sheathedAttachmentPoint,
    bool showHidden
) {
  forceAlternate = forceAlternate || invSlot == INVSLOT_OFFHAND;
  int attachmentSlot = InvSlotToObjAttachSlot(invSlot);
  if (attachmentSlot < 0 || !displayID || !inventoryType) {
    return;
  }

  HMODEL characterModel = GetCharacterModel(0);
  if (!characterModel) {
    return;
  }

  ACTIVEATTACHMENTINFO **found;
  if (!UpdateVisibilitySlots(characterModel, attachmentSlot, found, displayID, deferApply)) {
    if (*found ||
        ((*found = CreateAttachmentInfo(invSlot, displayID, inventoryType, forceAlternate, sheathe, sheathedAttachmentPoint, showHidden)) != 0))
    {
      if ((1u << inventoryType) & s_canHideslots[0]) {
        if (m_weaponTrails[attachmentSlot]) {
          WeaponTrailClose(m_weaponTrails[attachmentSlot]);
        }
        m_weaponTrails[attachmentSlot] = 0;
        HMODEL model = (*found)->modelInfo[0].model;
        if (model && ModelIsLoaded(model, 1)) {
          m_weaponTrails[attachmentSlot] = WeaponTrailCreate(model);
        }
      }
      if (!deferApply && !showHidden) {
        ApplyAttachmentInfo(characterModel, sheathe, attachmentSlot, false);
      }
    }
  }
  HandleClose(characterModel);
}

bool CGUnit_C::WeaponAttached(COMBATHAND hand) const {
  OBJATTACHMENTPOINTS attachmentPoint = s_handAttachments[hand];
  FATALASSERT(hand < NUMHANDS);

  ACTIVEATTACHMENTINFO *info = m_attachments[attachmentPoint];
  return info && !(info->flags & flags) && (info->flags & 4);
}

bool CGUnit_C::UpdateVisibilitySlots(HMODEL characterModel, int attachmentSlot, ACTIVEATTACHMENTINFO **&found, int displayID, bool deferApply) {
  ACTIVEATTACHMENTINFO **active = &m_attachments[attachmentSlot];
  found = active;
  ACTIVEATTACHMENTINFO *current = *active;
  if (current && current->displayInfo->GetID() == displayID) {
    ClearDeferredAttachment(characterModel, attachmentSlot);
    return deferApply;
  }

  ACTIVEATTACHMENTINFO *deferred = m_deferredAttachments[attachmentSlot];
  if (deferred && deferred->displayInfo->GetID() == displayID) {
    *active = deferred;
    m_deferredAttachments[attachmentSlot] = current;
    ClearDeferredAttachment(characterModel, attachmentSlot);
    return deferApply;
  }

  ClearDeferredAttachment(characterModel, attachmentSlot);
  current = *active;
  *active = m_deferredAttachments[attachmentSlot];
  m_deferredAttachments[attachmentSlot] = current;
  ClearDeferredAttachment(characterModel, attachmentSlot);
  return false;
}

void CGUnit_C::ClearWeaponTrailHandles() {
  for (int i = 0; i < OBJATTACH_NUM; ++i) {
    if (m_weaponTrails[i]) {
      WeaponTrailClose(m_weaponTrails[i]);
      m_weaponTrails[i] = 0;
    }
  }
}

void CGUnit_C::ReinitializeWeaponTrails() {
  ACTIVEATTACHMENTINFO *attachment = m_attachments[OBJATTACH_MAINHAND];
  if (attachment && !m_weaponTrails[OBJATTACH_MAINHAND] && ((1u << attachment->inventoryType) & s_canHideslots[0])) {
    HMODEL model = attachment->modelInfo[0].model;
    if (model) {
      m_weaponTrails[OBJATTACH_MAINHAND] = WeaponTrailCreate(model);
    }
  }

  attachment = m_attachments[OBJATTACH_OFFHAND];
  if (attachment && !m_weaponTrails[OBJATTACH_OFFHAND] && ((1u << attachment->inventoryType) & s_canHideslots[0])) {
    HMODEL model = attachment->modelInfo[0].model;
    if (model) {
      m_weaponTrails[OBJATTACH_OFFHAND] = WeaponTrailCreate(model);
    }
  }
}

void CGUnit_C::ClearDeferredAttachment(HMODEL charModel, int slot) {
  ACTIVEATTACHMENTINFO *info = m_deferredAttachments[slot];
  if (info) {
    info->ClearAttachmentFromModel(charModel, m_paperDollModel);
    info = m_deferredAttachments[slot];
    info->Clear();
    s_activeAttachmentFreeList.Put(info);
  }
  m_deferredAttachments[slot] = 0;
}

ACTIVEATTACHMENTINFO *CGUnit_C::CreateAttachmentInfo(
    int  invSlot,
    int  displayID,
    int  inventoryType,
    bool forceAlternate,
    bool sheathe,
    int  sheathedAttachmentPoint,
    bool showHidden
) {
  HMODEL              models[2] = {0, 0};
  int                 attachmentPoints[2] = {0, 0};
  if (!GetObjComponentInfo(
          GetDisplayRace(), GetDisplaySex(), displayID, inventoryType,
          (GetType() & TYPE_PLAYER) || m_texComponent || m_geosetHandle || m_displayInfoExtra,
          forceAlternate, models, attachmentPoints
      ))
  {
    return 0;
  }

  ACTIVEATTACHMENTINFO *info = s_activeAttachmentFreeList.Get(0);
  info->inventoryType = inventoryType;
  info->invSlot = invSlot;
  info->sheathAttachmentSlot = sheathedAttachmentPoint;
  info->displayInfo = g_itemDisplayInfoDB.GetRecord(displayID);
  info->enchantmentVisual = 0;
  if (sheathe) {
    info->flags |= 1;
  }
  if (showHidden) {
    info->flags |= 2;
  }
  for (unsigned int i = 0; i < 2; ++i) {
    info->modelInfo[i].model = models[i];
    info->modelInfo[i].attachmentPoint = attachmentPoints[i];
    models[i] = 0;
  }
  return info;
}

void CGUnit_C::RemoveObjectComponentByInvSlot(int invSlot, bool deferDeleteFromModel, bool removeRecord) {
  int attachmentSlot = InvSlotToObjAttachSlot(invSlot);
  if (attachmentSlot < 0) {
    return;
  }

  if (m_weaponTrails[attachmentSlot]) {
    WeaponTrailClose(m_weaponTrails[attachmentSlot]);
    m_weaponTrails[attachmentSlot] = 0;
  }

  if (!m_attachments[attachmentSlot] && !m_deferredAttachments[attachmentSlot]) {
    return;
  }

  HMODEL model = GetCharacterModel(0);
  FATALASSERT(model);
  if (removeRecord || !deferDeleteFromModel) {
    ClearDeferredAttachment(model, attachmentSlot);
    ACTIVEATTACHMENTINFO *current = m_attachments[attachmentSlot];
    m_attachments[attachmentSlot] = m_deferredAttachments[attachmentSlot];
    m_deferredAttachments[attachmentSlot] = current;
    ClearDeferredAttachment(model, attachmentSlot);
  } else {
    ClearDeferredAttachment(model, attachmentSlot);
    ACTIVEATTACHMENTINFO *current = m_attachments[attachmentSlot];
    m_attachments[attachmentSlot] = m_deferredAttachments[attachmentSlot];
    m_deferredAttachments[attachmentSlot] = current;
  }
  HandleClose(model);
}

void CGUnit_C::ClearActiveAttachmentInfo() {
  HMODEL model = GetCharacterModel(0);
  for (int i = 0; i < OBJATTACH_NUM; ++i) {
    if (m_attachments[i]) {
      m_attachments[i]->ClearAttachmentFromModel(model, m_paperDollModel);
      m_attachments[i]->Clear();
      s_activeAttachmentFreeList.Put(m_attachments[i]);
      m_attachments[i] = 0;
    }
    if (m_deferredAttachments[i]) {
      m_deferredAttachments[i]->ClearAttachmentFromModel(model, m_paperDollModel);
      m_deferredAttachments[i]->Clear();
      s_activeAttachmentFreeList.Put(m_deferredAttachments[i]);
      m_deferredAttachments[i] = 0;
    }
  }
  if (model) {
    HandleClose(model);
  }
}

bool CGUnit_C::SheatheObjComponent(int slot, bool sheathe) {
  int attachmentSlot = InvSlotToObjAttachSlot(slot);
  if (attachmentSlot < 0) {
    return 0;
  }

  if (!m_attachments[attachmentSlot]) {
    return 0;
  }

  HMODEL charModel = GetCharacterModel(0);
  FATALASSERT(charModel);
  ClearDeferredAttachment(charModel, attachmentSlot);
  bool result = ApplyAttachmentInfo(charModel, sheathe, attachmentSlot, 0);
  HandleClose(charModel);
  return result;
}

bool CGUnit_C::ApplyAttachmentInfo(HMODEL characterModel, bool sheathe, int attachmentSlot, bool) {
  FATALASSERT(attachmentSlot < (sizeof(m_attachments) / sizeof(m_attachments[0])));
  FATALASSERT(characterModel);

  ACTIVEATTACHMENTINFO *info = m_attachments[attachmentSlot];
  if (!info) {
    return true;
  }

  if (sheathe) {
    info->flags |= 1;
  } else {
    info->flags &= ~1u;
  }
  if (info->flags & 2) {
    return true;
  }
  if (attachmentSlot != OBJATTACH_RANGED &&
      !((1 << attachmentSlot) & ((1 << OBJATTACH_MAINHAND) | (1 << OBJATTACH_OFFHAND)))) {
    sheathe = false;
  }

  info->ClearAttachmentFromModel(characterModel, m_paperDollModel);
  bool failed = false;
  for (int i = 0; i < 2; ++i) {
    ATTACHMENTMODELINFO &modelInfo = info->modelInfo[i];
    if (!modelInfo.model) {
      continue;
    }

    modelInfo.currentLink = modelInfo.attachmentPoint;
    if (sheathe) {
      modelInfo.currentLink = info->sheathAttachmentSlot;
    }
    if (modelInfo.currentLink < 0) {
      continue;
    }

    if (!(info->flags & 2)) {
      int linkFailed;
      if (attachmentSlot > OBJATTACH_SHOULDERPAD) {
        linkFailed = !ModelAddLink(characterModel, modelInfo.currentLink, modelInfo.model, 1.0f);
      } else {
        linkFailed = !AddAttachment(characterModel, modelInfo.currentLink, modelInfo.model, 1.0f);
      }
      if (linkFailed) {
        modelInfo.currentLink = -1;
        failed = true;
      } else {
        ModelSetVertexAlpha(modelInfo.model, m_alpha, 1);
      }
    }

    if (m_paperDollModel && attachmentSlot != OBJATTACH_RANGED && modelInfo.model) {
      HMODEL duplicate = ModelDuplicate(modelInfo.model, 0);
      if (duplicate) {
        ModelSetVertexAlpha(duplicate, 255, 1);
        ModelAddLink(m_paperDollModel, modelInfo.currentLink, duplicate, 1.0f);
        HandleClose(duplicate);
      }
    }
  }

  info->flags &= ~5u;
  if (!failed && attachmentSlot >= OBJATTACH_MAINHAND) {
    info->flags |= 4;
    SetHandsState(characterModel);
  }
  return !failed;
}

void CGUnit_C::SetAttachmentHidden(int attachmentSlot, bool hide) {
  if (static_cast<unsigned int>(attachmentSlot) >= OBJATTACH_NUM) {
    return;
  }

  ACTIVEATTACHMENTINFO *info = m_attachments[attachmentSlot];
  if (!info) {
    return;
  }

  HMODEL charModel = GetCharacterModel(0);
  info->Hide(this, charModel, m_paperDollModel, hide);
  HandleClose(charModel);
}

void CGUnit_C::ReinitializePaperdollModel() {
  if (m_paperDollModel) {
    Script_SendUnitSignal(GetGUID(), 182);
  }
}

void CGUnit_C::CreatePaperdollModel() {
  HMODEL      &paperDollModel = m_paperDollModel;
  int sequenceTime = 0;
  if (paperDollModel) {
    sequenceTime = ModelGetSequenceTime(paperDollModel, 0);
    HandleClose(paperDollModel);
  }

  paperDollModel = DuplicateCharacterModel(1);
  HCHARGEOSET geosetHandle = m_geosetHandle;
  CharCustomizationSetPaperDollGeoset(geosetHandle, paperDollModel);
  FATALASSERT(paperDollModel);
  ClearSpecialEffects(paperDollModel);
  if (!ModelSetSequence(paperDollModel, 0, 5)) {
    ConsolePrintf("UNITNOSTAND|%s", GetUnitName());
  }
  ModelForceSequenceTime(paperDollModel, 0, sequenceTime, 0);
}

int CGUnit_C::ShouldDelayLevelupAnim() {
  unsigned int state = m_currentTorsoAnimState;
  FATALASSERT(state < 64);
  return ShouldDelayLevelupAnim(state);
}

int CGUnit_C::ShouldDelayLevelupAnim(unsigned int state) {
  FATALASSERT(state < 64);
  return s_animInfo[state].flags & 0x200;
}

void CGUnit_C::DestroyPaperdollModel() {
  if (m_paperDollModel) {
    HandleClose(m_paperDollModel);
    m_paperDollModel = 0;
  }
}

HMODEL CGUnit_C::GetPaperDollModel(bool duplicateModel) {
  HMODEL &paperDollModel = m_paperDollModel;
  if (!paperDollModel) {
    CreatePaperdollModel();
  }
  if (!paperDollModel) {
    return 0;
  }
  if (duplicateModel) {
    return ModelDuplicate(paperDollModel, 0);
  }
  return static_cast<HMODEL>(HandleDuplicate(paperDollModel));
}

const SpellVisualKitRec *CGUnit_C::GetRangedSpellAnim(int id, bool castKit) {
  const SpellRec *spellRec = g_spellDB.GetRecord(id);
  if (!spellRec) {
    return 0;
  }

  SpellVisualRec  visRecData;
  const SpellVisualRec *visualRec = GetAppropriateSpellVisual(spellRec, visRecData);
  if (!visualRec) {
    return 0;
  }

  int kitID = castKit ? visualRec->m_castKit : visualRec->m_precastKit;
  return g_spellVisualKitDB.GetRecord(kitID);
}

unsigned int CGUnit_C::GetCurrentTorsoAnim() const {
  if (!m_currentTorsoAnimState) {
    return m_currentBaseAnim;
  }
  return m_currentTorsoAnim;
}

HMODEL AuraVisual::GetModel() {
  if (flags & 2) {
    FATALASSERT(obj);
    HMODEL model = CWorld::GetModel(obj);
    FATALASSERT(model);
    return model;
  }
  return theModel;
}

void CGUnit_C::SetSheatheReason(SHEATHEREASONS reason, bool on, bool suppressSound) {
  if (reason >= SHEATHE_NUMREASONS) {
    return;
  }

  int oldReasons = m_sheatheReasons;
  int reasonFlag = 1 << reason;
  int newReasons = on ? oldReasons | reasonFlag : oldReasons & ~reasonFlag;

  int sheathe = oldReasons == 0;
  m_sheatheReasons = newReasons;
  if (sheathe != (newReasons == 0) || (m_animFlags & 0x800)) {
    bool playSound = !suppressSound && ((oldReasons ^ newReasons) & 1);
    if (m_animFlags & 0x800) {
      sheathe = m_unit->weaponMode == WEAPONMODE_SHEATHEDMODE;
    }
    SheatheOrUnsheatheItems(reason, sheathe, playSound);
  }

  if (m_sheatheReasons) {
    SetAttachmentHidden(OBJATTACH_RANGED, m_unit->weaponMode != WEAPONMODE_RANGEDMODE);
  }
  m_animFlags &= ~0x800u;
}

void CGUnit_C::CheckDeferredSheathing() {
  if (!(m_deferredSheatheFlags & 1)) {
    return;
  }

  FATALASSERT(m_deferredSheatheReason < SHEATHE_NUMREASONS);

  bool sheathe = (m_deferredSheatheFlags & 2) != 0;
  HMODEL charModel = GetCharacterModel(0);
  FATALASSERT(charModel);

  for (unsigned int i = 0; i < NUMHANDS; ++i) {
    const VirtualItemInfo *info = GetVirtualItem(g_monsterHands[i], 1);
    if (!info) {
      RemoveObjectComponentByInvSlot(s_hands[i], false, true);
    } else {
      SheatheObjComponent(s_hands[i], sheathe);
      if (sheathe) {
        DisableWeaponTrails();
      }
      if (m_deferredSheatheFlags & 4) {
        SndInterfacePlaySheatheSound(info, sheathe, GetPosition());
      }
    }
  }

  m_deferredSheatheFlags = 0;
  SetHandsState(charModel);
  HandleClose(charModel);
  DetermineReadySequence(0);
}

void CGUnit_C::DisableWeaponTrails() {
  for (unsigned int i = 0; i < 5; ++i) {
    if (m_weaponTrails[i]) {
      WeaponTrailDisableDrawing(m_weaponTrails[i]);
    }
  }
}

void CGUnit_C::SheatheOrUnsheatheItems(SHEATHEREASONS reason, bool sheathe, bool playSound) {
  m_deferredSheatheReason = reason;
  m_deferredSheatheFlags = 1;
  if (sheathe) {
    m_deferredSheatheFlags = 3;
  }
  if (playSound) {
    m_deferredSheatheFlags |= 4;
  }
}

void CGUnit_C::MaybeStartSheatheAnim() {
  if (SheatheAnimPlaying()) {
    if (m_animFlags & 0x10000) {
      HandleSheatheAnimEvent(1, 1);
    }
    return;
  }

  bool                        found = false;
  const VIRTUAL_MONSTER_SLOT *slot = g_monsterHands;
  ANIMENUMERATION            *handAnim = m_handAnim;
  for (; slot < g_monsterHands + NUMHANDS; ++slot, ++handAnim) {
    const VirtualItemInfo *info = GetVirtualItem(*slot, 0);
    if (info) {
      found = true;
      unsigned char sheatheMask = 1 << info->m_sheatheType;
      *handAnim = (sheatheMask & 0x88) ? ANIM_HIPSHEATHE : ANIM_SHEATHE;
    }
  }

  if (!found) {
    HandleSheatheAnimEvent(0, 0);
  } else if (!SetSheathingSequence()) {
    HandleSheatheAnimEvent(1, 1);
  }
}

bool CGUnit_C::SheatheAnimPlaying() const {
  for (unsigned int hand = 0; hand < NUMHANDS; ++hand) {
    if (m_handAnim[hand] != RESET_ANIMATION_INDICES0) {
      return true;
    }
  }
  return false;
}

void CGUnit_C::HandleRemotePlayerSheathing() {
  if (g_standStateAllowsSheathing[m_unit->standState]) {
    MaybeStartSheatheAnim();
  }
}

void CGUnit_C::HandleLocalPlayerSheathing() {
  if (!SheatheAnimPlaying() || (m_animFlags & 0x10000)) {
    SetSheatheReason(
        SHEATHE_PLAYEREXPLICIT,
        m_unit->weaponMode == WEAPONMODE_SHEATHEDMODE,
        !SheatheAnimPlaying()
    );
  }
}

void CGUnit_C::HandleSheatheAnimEvent(bool clearSheatheAnim, bool suppressSound) {
  if (!(m_animFlags & 0x10000)) {
    UpdateSheatheRangedReasons(suppressSound);
  }
  if (clearSheatheAnim) {
    SheatheAnimEndHandler();
  }
  m_animFlags |= 0x10000U;
}

bool CGUnit_C::SetSheathingSequence() {
  HMODEL theModel = GetCharacterModel(0);
  FATALASSERT(theModel);
  m_animFlags &= ~0x10000u;

  bool success = true;
  ANIMENUMERATION *handAnim = m_handAnim;
  for (unsigned int i = 0; i < NUMHANDS; ++i, ++handAnim) {
    if (*handAnim != RESET_ANIMATION_INDICES0) {
      success = success && ObjectModelSetBoneSequence(theModel, *handAnim, s_shoulderBones[i], 0);
      ModelLockObjectSequence(theModel, s_shoulderBones[i], 1);
    }
  }
  HandleClose(theModel);
  return success;
}

int SheatheTypeToSheathePoint(int sheatheType, int invSlot) {
  unsigned int ranged = invSlot != INVSLOT_MAINHAND;
  return s_savedSheathToAttachPoints[s_sheathePoints[ranged][sheatheType]];
}

void CGUnit_C::SetWeaponMode(WEAPONMODE mode) {
  FATALASSERT(mode < WEAPONMODE_NUMMODES);

  CDataStore msg;
  msg.Put(CMSG_SETWEAPONMODE);
  msg.Put(static_cast<unsigned int>(mode));
  msg.Finalize();
  ClientServices_Send(&msg);

  SetLastWeaponModeSent(mode);
}

void CGUnit_C::AddSpellProcOneShotEffect(int spellID, const SpellVisualKitRec *rec) {
  if (!rec) {
    return;
  }

  if (rec->m_characterParam[0] >= 11 || ((1 << static_cast<unsigned int>(rec->m_characterParam[0])) & 0x640)) {
    return;
  }

  HMODEL charModel = GetCharacterModel(0);
  if (!charModel) {
    return;
  }

  SPELLEFFECTDESC *newDesc = s_spellEffectFreeList.Get(0);
  FATALASSERT(newDesc);
  newDesc->kitPtr = rec;
  newDesc->isOneShot = 1;

  m_spellEffectLists[static_cast<unsigned int>(rec->m_characterParam[0])].LinkNode(newDesc, LIST_TAIL, 0);
  SpellProcHandler handler = s_spellProcHandlerFunctions[static_cast<unsigned int>(rec->m_characterParam[0])];
  if (handler) {
    handler(SPELLPROCADD, m_spellEffectLists[static_cast<unsigned int>(rec->m_characterParam[0])], this, charModel, rec, newDesc, spellID, 0.0f);
  }
  HandleClose(charModel);
}

SPELLEFFECTDESC *CGUnit_C::GetActiveEffect(SpellEffectList &list) {
  SPELLEFFECTDESC *desc = list.Tail();
  while (desc && !desc->isOneShot) {
    desc = desc->Prev();
  }
  return desc ? desc : list.Tail();
}

void CGUnit_C::OnMoveStartLocal(unsigned long eventTime, int forward) {
  OnMovementInitiated(0);
  if (m_move.m_moveFlags & 0x2400) {
    CMovement::LogWrite("0x%016I64X: Immobilized\n", GetGUID());
  } else {
    static_cast<CMovement &>(m_move).OnMoveStartLocal(eventTime, forward);
  }
}

void CGUnit_C::OnMoveStopLocal(unsigned long eventTime) {
  static_cast<CMovement &>(m_move).OnMoveStopLocal(eventTime);
}

void CGUnit_C::OnStrafeStartLocal(unsigned long eventTime, int left) {
  if (!(m_unit->flags & 0x2000)) {
    OnMovementInitiated(0);
    if (m_move.m_moveFlags & 0x2400) {
      CMovement::LogWrite("0x%016I64X: Immobilized\n", GetGUID());
    } else {
      static_cast<CMovement &>(m_move).OnStrafeStartLocal(eventTime, left);
    }
  }
}

void CGUnit_C::OnStrafeStopLocal(unsigned long eventTime) {
  if (!(m_unit->flags & 0x2000)) {
    static_cast<CMovement &>(m_move).OnStrafeStopLocal(eventTime);
  }
}

void CGUnit_C::OnSwimStartLocal(unsigned long eventTime) {
  static_cast<CMovement &>(m_move).OnSwimStartLocal(eventTime);
}

void CGUnit_C::OnSwimStopLocal(unsigned long eventTime) {
  static_cast<CMovement &>(m_move).OnSwimStopLocal(eventTime);
}

void CGUnit_C::OnTurnStartLocal(unsigned long eventTime, int left) {
  OnMovementInitiated(0);
  if (m_unit->flags & 0x40000) {
    CMovement::LogWrite("0x%016I64X: Stunned\n", GetGUID());
  } else {
    static_cast<CMovement &>(m_move).OnTurnStartLocal(eventTime, left);
  }
}

void CGUnit_C::OnTurnStopLocal(unsigned long eventTime) {
  static_cast<CMovement &>(m_move).OnTurnStopLocal(eventTime);
}

void CGUnit_C::OnPitchStartLocal(unsigned long eventTime, int up) {
  static_cast<CMovement &>(m_move).OnPitchStartLocal(eventTime, up);
}
void CGUnit_C::OnPitchStopLocal(unsigned long eventTime) {
  static_cast<CMovement &>(m_move).OnPitchStopLocal(eventTime);
}

void CGUnit_C::OnSetFacingLocal(unsigned long eventTime, float facing) {
  OnMovementInitiated(1);
  static_cast<CMovement &>(m_move).OnSetFacingLocal(eventTime, facing);
  if (m_unit->standState == 3 || m_unit->standState == 2) {
    ChangeStandState(0);
  }
}

void CGUnit_C::OnSetRawFacingLocal(unsigned long eventTime, float facing) {
  OnMovementInitiated(1);
  static_cast<CMovement &>(m_move).OnSetRawFacingLocal(eventTime, facing);
  if (m_unit->standState == 3 || m_unit->standState == 2) {
    ChangeStandState(0);
  }
}

void CGUnit_C::OnSetPitchLocal(unsigned long eventTime, float pitch) {
  static_cast<CMovement &>(m_move).OnSetPitchLocal(eventTime, pitch);
}

void CGUnit_C::OnJumpLocal(unsigned long eventTime) {
  OnMovementInitiated(0);
  if (m_move.m_moveFlags & 0x2400) {
    CMovement::LogWrite("0x%016I64X: Immobilized\n", GetGUID());
  } else if (!(m_unit->flags & 0x2000) || (m_move.m_moveFlags & 0x40FF)) {
    static_cast<CMovement &>(m_move).OnJumpLocal(eventTime);
  } else {
    CDataStore msg;
    msg.Put(CMSG_MOUNTSPECIAL_ANIM);
    msg.Put(GetGUID());
    msg.Finalize();
    ClientServices_Send(&msg);
  }
}

void CGUnit_C::OnRunSpeedChangeLocal(unsigned long eventTime, NETMESSAGE msgID, float speed) {
  static_cast<CMovement &>(m_move).OnRunSpeedChange(eventTime, speed);
  UpdateBaseAnimation(0);
  UpdateMovementAnimSpeed(1, INVALID_ANIM_STATE);

  CDataStore msg;
  BuildMovementUpdate(msgID, &msg);
  msg.Put(speed);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGUnit_C::OnWalkSpeedChangeLocal(unsigned long eventTime, float speed) {
  static_cast<CMovement &>(m_move).OnWalkSpeedChange(eventTime, speed);
  UpdateMovementAnimSpeed(1, INVALID_ANIM_STATE);

  CDataStore msg;
  BuildMovementUpdate(MSG_MOVE_SET_WALK_SPEED_CHEAT, &msg);
  msg.Put(speed);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGUnit_C::OnSwimSpeedChangeLocal(unsigned long eventTime, NETMESSAGE msgID, float speed) {
  static_cast<CMovement &>(m_move).OnSwimSpeedChange(eventTime, speed);
  UpdateMovementAnimSpeed(1, INVALID_ANIM_STATE);

  CDataStore msg;
  BuildMovementUpdate(msgID, &msg);
  msg.Put(speed);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGUnit_C::OnAllSpeedChangeLocal(unsigned long eventTime, float speed) {
  static_cast<CMovement &>(m_move).OnRunSpeedChange(eventTime, speed);
  static_cast<CMovement &>(m_move).OnWalkSpeedChange(eventTime, speed);
  static_cast<CMovement &>(m_move).OnSwimSpeedChange(eventTime, speed);
  UpdateBaseAnimation(0);
  UpdateMovementAnimSpeed(1, INVALID_ANIM_STATE);

  CDataStore msg;
  BuildMovementUpdate(MSG_MOVE_SET_ALL_SPEED_CHEAT, &msg);
  msg.Put(speed);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGUnit_C::OnTurnRateChangeLocal(unsigned long eventTime, float rate) {
  static_cast<CMovement &>(m_move).OnTurnRateChange(eventTime, rate);

  CDataStore msg;
  BuildMovementUpdate(MSG_MOVE_SET_TURN_RATE_CHEAT, &msg);
  msg.Put(rate);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGUnit_C::OnSetRunModeLocal(unsigned long eventTime, int run) {
  static_cast<CMovement &>(m_move).OnSetRunModeLocal(eventTime, run);
}

void CGUnit_C::ToggleRunModeLocal(unsigned long eventTime) {
  OnSetRunModeLocal(eventTime, m_move.m_moveFlags & 0x100);
}

void ProcessLocalMoveEvent(unsigned int msgId) {
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(CGUnit_C::GetActiveMover(), __FILE__, __LINE__));
  FATALASSERT(unit);
  unit->ProcessLocalMoveEvent(static_cast<NETMESSAGE>(msgId));
}

void CGUnit_C::EnableWeaponTrail(const NTempest::CImVector &color, int fadeOutRate, unsigned int duration) {
  int *trails = m_weaponTrails;
  for (unsigned int i = 0; i < 5; ++i) {
    if (trails[i]) {
      WeaponTrailSetDrawing(trails[i], color, fadeOutRate, duration);
    }
  }
}

void SPELLEFFECTDESC::ClearLightningObjects() {
  for (unsigned int i = 0; i < 3; ++i) {
    if (lightningObjs[i]) {
      SpellVisualClearLightning(lightningObjs[i]);
    }
    lightningObjs[i] = 0;
  }
}

SPELLEFFECTDESC::SPELLEFFECTDESC() : color(0ul) {
  lightningObjs[0] = 0;
  lightningObjs[1] = 0;
  lightningObjs[2] = 0;
}

void AuraVisual::SetModel(HMODEL model) {
  Clear();
  theModel = reinterpret_cast<HMODEL>(HandleDuplicate(model));
  if (theModel) {
    flags |= 1;
  }
  flags &= ~2;
}

void AuraVisual::SetWorldObject(unsigned long object) {
  Clear();
  obj = object;
  if (obj) {
    flags |= 3;
  }
}

void CGUnit_C::SaveTrackingTarget(unsigned __int64 target, TRACKTYPE type, bool snapToTargetOnClear) {
  FATALASSERT(!target || type <= TRACKTYPE_NUMTRACKTYPES);
  if (GetGUID() != ClntObjMgrGetActivePlayer() || ((!target && !s_trackingTarget) || target == GetGUID())) {
    return;
  }

  if (target && type == TRACKTYPE_FOLLOW && m_unit->channelSpell) {
    CGGameUI::DisplayError(GERR_TOOBUSYTOFOLLOW);
    return;
  }

  unsigned long currentTime = OsGetAsyncTimeMs();
  if (target) {
    CGObject_C *object = ClntObjMgrObjectPtr(target, __FILE__, __LINE__);
    if (!object || !(object->GetType() & TYPE_UNIT)) {
      target = 0;
    } else {
      float disengageDistance = s_trackTypeInfo[type].disengageDistance;
      if ((object->GetPosition() - GetPosition()).SquaredMag() > disengageDistance * disengageDistance) {
        CGGameUI::DisplayError(GERR_AUTOFOLLOW_TOO_FAR);
        target = 0;
      } else if (!s_trackingTarget) {
        if (m_move.m_moveFlags & 0x100) {
          s_trackingFlags &= ~4u;
        } else {
          s_trackingFlags |= 4;
        }
      }
      s_trackingInterpStartTime = OsGetAsyncTimeMs();
      s_trackLastCheckTime = s_trackingInterpStartTime;
    }
  } else if (s_trackingTarget) {
    unsigned int objectFlags = m_move.m_moveFlags;
    if ((((s_trackingFlags >> 2) ^ static_cast<unsigned int>(~(objectFlags >> 8))) & 1) != 0) {
      OnSetRunModeLocal(currentTime, (s_trackingFlags >> 2) & 1);
    }

    if (snapToTargetOnClear || s_trackTypeInfo[s_trackingType].hasMovement) {
      CGObject_C *object = ClntObjMgrObjectPtr(s_trackingTarget, __FILE__, __LINE__);
      if (object) {
        float facing = CalculateFacingTo(GetPosition(), object->GetPosition());
        s_trackingFlags |= 2;
        OnSetFacingLocal(currentTime, facing);
        s_trackingFlags &= ~2u;
      }
    }

    if (s_trackTypeInfo[s_trackingType].hasMovement && (m_move.m_moveFlags & 0xFF)) {
      s_trackingFlags |= 2;
      OnMoveStopLocal(currentTime);
      OnTurnStopLocal(currentTime);
      OnStrafeStopLocal(currentTime);
      s_trackingFlags &= ~2u;
    }
    s_trackingFlags &= ~1u;
  }

  if (type != s_trackingType || target != s_trackingTarget) {
    CGGameUI::ShowAutoFollowChange(target, s_trackingTarget, type);
  }
  s_trackingTarget = target;
  s_trackingType = type;
}

void CGUnit_C::ClearTrackingTarget(bool snapToTargetOnClear) {
  if (s_trackingTarget && GetGUID() == ClntObjMgrGetActivePlayer()) {
    SaveTrackingTarget(0, s_trackingType, snapToTargetOnClear);
  }
}

unsigned __int64 CGUnit_C::GetTrackingTarget() const {
  return m_obj->m_guid == ClntObjMgrGetActivePlayer() ? s_trackingTarget : 0;
}

void CGUnit_C::HandleFollowTarget() {
  if (!GetTrackingTarget()) {
    return;
  }

  FATALASSERT(s_trackingType <= TRACKTYPE_NUMTRACKTYPES);

  CGUnit_C *target = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(s_trackingTarget, __FILE__, __LINE__));
  if (!target || target->m_unit->health <= 0 || target->m_unit->flags & 0x100000) {
    ClearTrackingTarget(0);
    return;
  }

  NTempest::C3Vector position;
  NTempest::C3Vector targetPosition;
  GetPosition(position);
  target->GetPosition(targetPosition);

  float dx = targetPosition.x - position.x;
  float dy = targetPosition.y - position.y;
  float dz = targetPosition.z - position.z;
  float distanceSq = dx * dx + dy * dy + dz * dz;
  float disengageDistance = s_trackTypeInfo[s_trackingType].disengageDistance;
  if (distanceSq > disengageDistance * disengageDistance) {
    ClearTrackingTarget(0);
    return;
  }

  unsigned long currentTime = OsGetAsyncTimeMs();
  target->GetPosition(targetPosition);
  GetPosition(position);
  float facingToTarget = CalculateFacingTo(position, targetPosition);
  float facing = GetFacing();
  float facingDiff = facingToTarget - facing;
  if (fabs(facingDiff) >= 0.001f) {
    s_trackingFlags |= 1;
    if (facingDiff < 0.0f) {
      facingDiff = fmod(facingDiff, 6.2831855f) + 6.2831855f;
    } else if (facingDiff > 6.2831855f) {
      facingDiff = fmod(facingDiff, 6.2831855f);
    }

    float maxChange =
        fmod(fabs((currentTime - s_trackLastCheckTime) * 0.001f * 3.1415927f), 6.2831855f);
    s_trackLastCheckTime = currentTime;
    float change = facingDiff <= 3.1415927f ? facingDiff : 6.2831855f - facingDiff;
    if (change >= maxChange) {
      change = maxChange;
    }

    float newFacing = facingDiff <= 3.1415927f ? facing + change : facing - change;
    s_trackingFlags |= 2;
    OnSetFacingLocal(currentTime, fmod(newFacing, 6.2831855f));
    s_trackingFlags &= ~2u;
  } else {
    s_trackingFlags &= ~1u;
  }

  if (s_trackTypeInfo[s_trackingType].hasMovement) {
    unsigned int moveFlags = m_move.m_moveFlags;
    if (moveFlags & 3) {
      if (distanceSq <= TRACKMOVETHRESHOLDSQ) {
        s_trackingFlags |= 2;
        OnMoveStopLocal(currentTime);
        s_trackingFlags &= ~2u;
      } else {
        int run = distanceSq > TRACKRUNTHRESHOLDSQ;
        if (run != ((~moveFlags >> 8) & 1)) {
          OnSetRunModeLocal(currentTime, run);
        }
      }
    } else if (distanceSq > TRACKMOVETHRESHOLDSQ) {
      s_trackingFlags |= 2;
      OnMoveStartLocal(currentTime, 1);
      s_trackingFlags &= ~2u;
    }
  }
}

NAMEPLATEDESC::~NAMEPLATEDESC() {
  RecycleNameplateFrame(namePlate);
  namePlate = 0;
}

void CGUnit_C::OnMovementInitiated(bool facingOnly) {
  if (GetTrackingTarget() && !(s_trackingFlags & 2)) {
    ClearTrackingTarget(false);
  }
  if ((m_unit->flags & 0x400) && !facingOnly && GetGUID() == ClntObjMgrGetActivePlayer()) {
    CGGameUI::CloseLoot(1, 1);
  }
}

bool CGUnit_C::TrackingTargetMoving() const {
  return (s_trackingFlags & 1) != 0;
}

int GetObjAnimFlags(int unitAnimFlags) {
  int flags = unitAnimFlags & 1;
  if (unitAnimFlags & 2) {
    flags |= 2;
  }
  if (unitAnimFlags & 4) {
    flags |= 4;
  }
  if (unitAnimFlags & 8) {
    flags |= 8;
  }
  return flags;
}

void IMPACTEFFECTDESC::Set(unsigned __int64 a, unsigned __int64 v, const SpellVisualKitRec *i, int s) {
  attacker = a;
  victim = v;
  impactKit = const_cast<SpellVisualKitRec *>(i);
  spellID = s;
}
void CGUnit_C::GetAFKText(char *buffer, int) const {
  buffer[0] = '\0';
}

void CGUnit_C::GetDNDText(char *buffer, int) const {
  buffer[0] = '\0';
}

void CGUnit_C::GetGMText(char *buffer, int) const {
  buffer[0] = '\0';
}

const char *CGUnit_C::GetUnitTitle() const {
  return m_stats && m_stats->m_title && m_stats->m_title[0] ? m_stats->m_title : 0;
}

unsigned int CGUnit_C::UpdateUnitNameString(
    unsigned int,
    unsigned int otherUnitsFlags,
    char *buffer,
    unsigned int bufferSize
) const {
  FATALASSERT(buffer);
  FATALASSERT(bufferSize);

  unsigned int added = 0;
  char temp[260];

  GetAFKText(temp, sizeof(temp));
  SStrPack(buffer, temp, bufferSize);
  GetDNDText(temp, sizeof(temp));
  SStrPack(buffer, temp, bufferSize);
  GetGMText(temp, sizeof(temp));
  SStrPack(buffer, temp, bufferSize);

  if (otherUnitsFlags & 1) {
    SStrPrintf(temp, sizeof(temp), "%s%s", "", GetUnitName());
    SStrPack(buffer, temp, bufferSize);
    added = 1;
  }

  if ((m_obj->m_type & TYPE_PLAYER) && (otherUnitsFlags & 2)) {
    const CGPlayer_C *player = static_cast<const CGPlayer_C *>(this);
    unsigned int guildID = player->GetGuildID();
    if (guildID) {
      const unsigned __int64 noGuid = 0;
      const GuildStats_C *guild =
          g_guildInfoCache.GetRecord(guildID, noGuid, PlayerNameGuildCallback, const_cast<HPLAYERNAME *>(&m_unitNameHandle));
      if (guild) {
        if (added) {
          SStrPack(buffer, "\n", bufferSize);
        }
        SStrPack(buffer, "<", bufferSize);
        SStrPack(buffer, guild->m_guildName, bufferSize);
        SStrPack(buffer, ">", bufferSize);
        ++added;
      }
    }
  }

  const char *title = GetUnitTitle();
  if (title && (otherUnitsFlags & 4)) {
    if (added) {
      SStrPack(buffer, "\n", bufferSize);
    }
    SStrPack(buffer, "<", bufferSize);
    SStrPrintf(temp, sizeof(temp), FrameScript_GetText("UNITNAME_TITLE", -1, GENDER_NOT_APPLICABLE), title);
    SStrPack(buffer, temp, bufferSize);
    SStrPack(buffer, ">", bufferSize);
    ++added;
  }

  if (otherUnitsFlags & 8) {
    char summonString[128] = {0};
    CGTooltip::GetSummonedByString(this, summonString, sizeof(summonString));
    if (summonString[0]) {
      if (added) {
        SStrPack(buffer, "\n", bufferSize);
      }
      SStrPack(buffer, "<", bufferSize);
      SStrPack(buffer, summonString, bufferSize);
      SStrPack(buffer, ">", bufferSize);
      ++added;
    }
  }

  return added;
}

unsigned __int64 CGUnit_C::GetLocalTarget() const {
  return m_targetUnit;
}

void CGUnit_C::HandleSpellEventSound() {
}

void CGUnit_C::CombatLoggingFlagChanged() {
}

float CGUnit_C::GetMountScale() const {
  return 1.0f;
}

void CGUnit_C::OnMount() {
}

void CGUnit_C::OnDismount() {
}

bool CGUnit_C::CanBeMounted() {
  return true;
}

void CGUnit_C::OnStandStateChanged(unsigned int, unsigned int) {
}

bool CGUnit_C::GetDefenseSkillRank(int &base, int &modifier) const {
  modifier = 0;
  base = 0;
  return false;
}

bool CGUnit_C::GetAttackSkillRank(int, int &base, int &modifier) const {
  modifier = 0;
  base = 0;
  return false;
}

float CGUnit_C::GetBlockChance() const {
  return 0.0f;
}

float CGUnit_C::GetDodgeChance() const {
  return 0.0f;
}

float CGUnit_C::GetParryChance() const {
  return 0.0f;
}

void CGUnit_C::UpdateObjComponentVisuals(const CGItem_C *, const ItemEnchantment *, int) {
}

void CGUnit_C::ClearItemVisuals(ACTIVEATTACHMENTINFO *) {
}

void CGUnit_C::SetItemVisuals(ACTIVEATTACHMENTINFO *, const ItemVisualsRec *, bool) {
}
