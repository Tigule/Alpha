#include "Object/ObjectClient/Unit_C.h"
#include "DB/DBClient/AutoCode/SkillLineAbilityRec.h"

#include <Base/Status.h>
#include <Base/CDataAllocator.h>
#include <Services/SysMessage.h>
#include <Services/Texture.h>
#include <Os/W32/OsSound.h>
#include <Os/W32/Debugging.h>
#include <Base/CDataStore.h>
#include <FrameScript/FrameScript.h>
#include <Gx/Gx.h>

#include <math.h>
#include <malloc.h>
#include <string.h>
#include <Tempest/c44matrix.h>
#include <Tempest/caabox.h>
#include <Tempest/cimvector.h>

#include "DB/DBClient/AutoCode/CreatureDisplayInfoRec.h"
#include "DB/DBClient/AutoCode/CreatureDisplayInfoExtraRec.h"
#include "DB/DBClient/AutoCode/ChrRacesRec.h"
#include "DB/DBClient/AutoCode/CreatureModelDataRec.h"
#include "DB/DBClient/AutoCode/CreatureSoundDataRec.h"
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
#include "DB/DBClient/AutoCode/SpellVisualRec.h"
#include "DB/DBClient/AutoCode/SpellVisualKitRec.h"
#include "DB/DBClient/AutoCode/SpellVisualEffectNameRec.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "DB/DBClient/DBClient.h"
#include "Client.h"
#include "Console/ConsoleVar.h"
#include "Console/ConsoleClient.h"
#include "Component/CharacterCustomization.h"
#include "Component/Component.h"
#include "Game/GameClient/PlayerName.h"
#include "Object/CreatureStats.h"
#include "Object/ObjectClient/Item_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "SoundInterface/SoundInterface.h"
#include "UIUtil/InputControl.h"
#include "Ui/GameUI.h"
#include "Ui/NamePlateFrame.h"
#include "Ui/ReputationInfo.h"
#include "Ui/SpellBookFrame.h"
#include "Ui/WorldFrame.h"
#include "WorldClient/World.h"
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
void __fastcall UnitEffectsInitialize();
void __fastcall UnitEffectsShutdown();
void __fastcall SpellVisualClearLightning(LightningObject *lightning);
void __fastcall SpellVisualGetLightning(CGUnit_C *unitPtr, SpellVisualKitRec *kitRec, int spellID, LightningObject **objects, int numObjects);
void __fastcall UpdatePortraitTexture(const unsigned __int64 &guid);
void            UnitFootprintNewBloodSplat(UnitBloodRec *rec, unsigned int unitSize, NTempest::C3Vector &position);

int __fastcall OnPickNextStandHandler(void *param, CGUnit_C *ptr) {
  typedef void (CGUnit_C::*HandlerProc)();
  HandlerProc handler;
  void      **vtable = *reinterpret_cast<void ***>(ptr);
  memcpy(&handler, &vtable[0x13C / sizeof(void *)], sizeof(handler));
  (ptr->*handler)();
  return 1;
}

CGUnit_C::~CGUnit_C() {
  UnsetAuraMirrorHandlers();
  RemoveUnitNamePlate();
  KillSpellLoopedSound();
  ClearActiveAttachmentInfo();
  if (m_resEffectModel) {
    HandleClose(m_resEffectModel);
    m_resEffectModel = 0;
  }
  if (m_interactIconModel) {
    HandleClose(m_interactIconModel);
    m_interactIconModel = 0;
  }
  ClearWeaponTrailHandles();
}

int __fastcall DeathAnimEndHandler(void *param, CGUnit_C *ptr) {
  ptr->DeathAnimEndHandler();
  return 1;
}

int __fastcall PickNextRunHandler(void *param, CGUnit_C *ptr) {
  ptr->PickNextRunHandler();
  return 1;
}

int __fastcall WoundAnimEndHandler(void *param, CGUnit_C *ptr) {
  ptr->WoundAnimEndHandler();
  return 1;
}

int __fastcall SpellAnimEndHandler(void *param, CGUnit_C *ptr) {
  ptr->SpellAnimEndHandler();
  return 1;
}

int __fastcall NPCAnimEndHandler(void *param, CGUnit_C *ptr) {
  ptr->NPCAnimEndHandler();
  return 1;
}

int __fastcall JumpTakeOffFinishedHandler(void *param, CGUnit_C *ptr) {
  ptr->JumpTakeOffFinishedHandler();
  return 1;
}

int __fastcall JumpLandFinishedHandler(void *param, CGUnit_C *ptr) {
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

static INTERACTICONTYPE s_questIconInfo[5] = {
    INTERACTICON_NONE, INTERACTICON_NONE, INTERACTICON_FUTURE, INTERACTICON_COMPLETION, INTERACTICON_NORMAL
};

struct BLOODSPLATNODE : public TSLinkedNode<BLOODSPLATNODE> {
  unsigned int       time;
  NTempest::C3Vector position;
};

static TSList<BLOODSPLATNODE, TSGetLink<BLOODSPLATNODE> > s_bloodSplatList;

struct NAMEPLATEDESC : public TSHashObject<NAMEPLATEDESC, CHashKeyGUID> {
  TSLink<NAMEPLATEDESC> m_sortLink;
  float                 screenSortOrder;
  CGUnit_C             *unit;
  CGNamePlateFrame     *namePlate;
  NTempest::C2Vector    screenCoords;

  ~NAMEPLATEDESC();
};

struct FREENAMEPLATE : public TSLinkedNode<FREENAMEPLATE> {
  CGNamePlateFrame *namePlate;

  FREENAMEPLATE() : namePlate(0) {
  }
  ~FREENAMEPLATE() {
    DEL(namePlate);
  }
};

static int                                              s_drawNameplates = 1;
static TSExplicitList<NAMEPLATEDESC, 32>                s_namePlateList;
static const float                                      MAX_NAMEPLATE_DIST = 30.0f;
static const float                                      MAX_NAMEPLATE_DIST_SQ = MAX_NAMEPLATE_DIST * MAX_NAMEPLATE_DIST;
static TSList<FREENAMEPLATE, TSGetLink<FREENAMEPLATE> > s_freeNamePlateList;
static TSHashTable<NAMEPLATEDESC, CHashKeyGUID>         s_monsterNamePlateList;
static CGWorldFrame                                    *s_namePlateWorldFrame;

int __fastcall RangedWeaponAnimEndHandler(void *param, CGUnit_C *ptr) {
  ptr->RangedWeaponAnimEndHandler();
  return 1;
}

typedef TSList<SPELLEFFECTDESC, TSGetLink<SPELLEFFECTDESC> > SpellEffectList;
typedef void(__fastcall *SpellProcHandler)(
    SPELLPROC_ACTION         action,
    SpellEffectList         &list,
    CGUnit_C                *unit,
    HMODEL                   charModel,
    const SpellVisualKitRec *rec,
    SPELLEFFECTDESC         *newDesc,
    unsigned int             spellID,
    float                    elapsed
);

void __fastcall                   SpellVisualsHandleCastStop(int id, CGUnit_C *caster, unsigned char status, unsigned char reason);
void __fastcall                   UnitEffectClearSpellPrecast(CGObject_C *object, int spellID);
void __fastcall                   SndInterfaceAssociateSoundWithObject(Sound *sound, CGObject_C *objectPtr);
const ItemSubClassRec *__fastcall SDBItemSubclassGetSubClassRec(unsigned int classID, unsigned int subClassID);
int __fastcall                    SheatheTypeToSheathePoint(int sheatheType, int invSlot);

enum SAVEDSHEATHATTACHPOINTS {
  SHEATHATTACH_NONE = 0,
  SHEATHATTACH_MAINHAND = 1,
  SHEATHATTACH_LARGEWEAPONLEFT = 3,
  SHEATHATTACH_LARGEWEAPONRIGHT = 4,
  SHEATHATTACH_NUM_SAVESSHEATHATTACHPOINTS = 8
};

static SAVEDSHEATHATTACHPOINTS s_sheathePoints[2][9] = {
    {SHEATHATTACH_NONE,                   SHEATHATTACH_MAINHAND, static_cast<SAVEDSHEATHATTACHPOINTS>(4), static_cast<SAVEDSHEATHATTACHPOINTS>(5),
     static_cast<SAVEDSHEATHATTACHPOINTS>(7), SHEATHATTACH_NONE, SHEATHATTACH_NONE, SHEATHATTACH_NONE, SHEATHATTACH_NONE},
    {SHEATHATTACH_NONE, static_cast<SAVEDSHEATHATTACHPOINTS>(2),            SHEATHATTACH_LARGEWEAPONLEFT, static_cast<SAVEDSHEATHATTACHPOINTS>(6),
     static_cast<SAVEDSHEATHATTACHPOINTS>(7), SHEATHATTACH_NONE, SHEATHATTACH_NONE, SHEATHATTACH_NONE, SHEATHATTACH_NONE}
};

static const int s_savedSheathToAttachPoints[8] = {-1, 26, 27, 30, 31, 32, 33, 28};
void __fastcall  WeaponTrailClose(int trail);
int __fastcall   WeaponTrailCreate(HMODEL model);
void __fastcall  WeaponTrailSetDrawing(int trail, const NTempest::CImVector &color, int fadeOutRate, unsigned int duration);
int __fastcall   Spell_C_GetCastTime(int id, int isPet);
int __fastcall   UnitEffectGetSpecialVisual(UNITEFFECTSPECIALS effectNumber);
void __fastcall  UnitCombatLogAuraAddedOrRemoved(CGUnit_C *unitPtr, int spellID, bool added, int auraSlot);
int __fastcall   GetObjComponentInfo(
    int     race,
    int     sex,
    int     displayID,
    int     inventoryType,
    bool    useMonsterComponent,
    bool    forceAlternate,
    HMODEL *models,
    int    *attachmentPoints
);
void __fastcall              CreatureQueryCallback(int id, const unsigned __int64 &guid, void *arg, bool granted);
void __fastcall              Script_SendUnitSignal(const unsigned __int64 &guid, int signal);
bool __fastcall              IsShapeshiftSpell(const SpellRec *rec);
void __fastcall              SndInterfacePlaySpellSound(int soundID, CGUnit_C *obj);
void __fastcall              SpellVisualsPlayCameraShakeID(unsigned int shakeID, const NTempest::C3Vector &position);
void __fastcall              UnitEffectAddMissile(const MISSILESTRUCT &desc, int durationOffset);
GEOCOMPONENTLINKS __fastcall UnitEffectGetLinkPointFromAttachment(UNITEFFECTATTACHPPOINT attach);
HMODEL __fastcall            UnitEffectCreateAuraModel(unsigned int effectID);
unsigned int __fastcall      UnitEffectIsAuraWorldObject(unsigned int effectID, unsigned int &isWorldObj);
unsigned long __fastcall     UnitEffectCreateWorldModelAura(unsigned int effect, const NTempest::C3Vector &location, float facing);
int __fastcall               OnFirstAuraSequenceFinished(void *param);
bool __fastcall              AnimSheathesWeapon(unsigned int anim);
int __fastcall               GetObjAnimFlags(int unitAnimFlags);
unsigned int __fastcall      SpellGetRangedPrecastHoldAnim(unsigned int loadAnim);
unsigned int __fastcall      PlayerNameGetUnitNameMode();
void __fastcall              SpellVisualsPlayCastKit(CGUnit_C *caster, SpellVisualKitRec *kitRec, int spellID, unsigned int isCastEffect);
unsigned int __fastcall      Object_C_AnimHasHitEvent(int anim);
float __fastcall             CalculateFacingTo(const NTempest::C3Vector &position, const NTempest::C3Vector &destination);
void __fastcall              UnitEffectOneShot(
    const SpellVisualEffectNameRec *effect,
    CGObject_C                     *object,
    UNITEFFECTATTACHPPOINT          attachPoint,
    int                             spellID,
    unsigned int                    isCastEffect,
    unsigned int                    forceEffectOnMount
);

struct AuraDecayNode : public TSLinkedNode<AuraDecayNode> {
  AuraVisual             visual;
  unsigned __int64       unit;
  UNITEFFECTATTACHPPOINT attach;

  ~AuraDecayNode() {
    visual.Clear();
  }
};

int __fastcall OnAuraDecayFinished(void *param);

class CGQuestInfo {
 public:
  static const unsigned __int64 &__fastcall GetQuestGiver();
  static void __fastcall                    QuestGiverFinished();
};
int __fastcall AuraMirrorHandler(unsigned __int64 guid, unsigned int offset, unsigned int bytes, const void *prevValue, void *param);
#define DECLARE_UNIT_MIRROR_HANDLER(name) int __fastcall name(unsigned __int64, unsigned int, unsigned int, const void *, void *)
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

static int                s_invSlotToObjAttachSlot[19] = {0, -1, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2, 3, 4, -1};
static const unsigned int s_hands[2] = {3, 2};
static unsigned int       s_canHideslots[5] = {0x00622000, 0, 0, 0xFFFFFFFF, 1};

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

static const unsigned int                               s_standStateAnimState[9] = {3, 51, 3, 54, 56, 57, 58, 1, 60};
static TInstanceAllocator<ACTIVEAURAINFO>               s_auraInfoFreeList(100);
static TSList<AuraDecayNode, TSGetLink<AuraDecayNode> > s_activeAuraDecays;
static TInstanceAllocator<AuraDecayNode>                s_auraDecayFreeList(100);
static TInstanceAllocator<SPELLEFFECTDESC>              s_spellEffectFreeList(100);
static TInstanceAllocator<ANIMQUEUENODE>                s_animQueueFreeList(100);

int __fastcall LootAnimEndHandler(void *param, CGUnit_C *ptr) {
  typedef void (CGUnit_C::*HandlerProc)();
  HandlerProc handler;
  void      **vtable = *reinterpret_cast<void ***>(ptr);
  memcpy(&handler, &vtable[0x108 / sizeof(void *)], sizeof(handler));
  (ptr->*handler)();
  return 1;
}

int __fastcall SheatheAnimEndHandler(void *param, CGUnit_C *ptr) {
  ptr->SheatheAnimEndHandler();
  return 1;
}

int __fastcall SitSleepAnimEndHandler(void *param, CGUnit_C *ptr) {
  ptr->SitSleepAnimEndHandler();
  return 1;
}

int __fastcall RangedPrecastEndHandler(void *param, CGUnit_C *ptr) {
  ptr->RangedPrecastEndHandler();
  return 1;
}

int __fastcall ThrowAnimEndHandler(void *param, CGUnit_C *ptr) {
  ptr->ThrowAnimEndHandler();
  return 1;
}

int __fastcall AttackAnimEndHandler(void *param, CGUnit_C *ptr) {
  ptr->AttackAnimEndHandler();
  return 1;
}

int __fastcall DodgeAnimEndHandler(void *param, CGUnit_C *ptr) {
  ptr->DodgeAnimEndHandler();
  return 1;
}

int InvSlotToObjAttachSlot(int invSlot) {
  return invSlot <= 18 ? s_invSlotToObjAttachSlot[invSlot] : -1;
}

static void __fastcall PurgeExpiredNodes(TSList<SPELLEFFECTDESC, TSGetLink<SPELLEFFECTDESC> > &list, float elapsed) {
  SPELLEFFECTDESC *desc = list.Head();
  while (desc) {
    SPELLEFFECTDESC *next = desc->Next();
    desc->curTime += static_cast<unsigned int>(elapsed * 1000.0f);
    if (desc->endTime && desc->curTime > desc->endTime) {
      desc->~SPELLEFFECTDESC();
      s_spellEffectFreeList.PutData(desc, 0, 0);
    }
    desc = next;
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

void __fastcall SpellProcChainHandler(
    SPELLPROC_ACTION action,
    SpellEffectList &,
    CGUnit_C *unit,
    HMODEL,
    const SpellVisualKitRec *rec,
    SPELLEFFECTDESC         *newDesc,
    unsigned int             spellID,
    float
) {
  if (action == SPELLPROCADD) {
    int savedChannelSpellID = *reinterpret_cast<int *>(reinterpret_cast<unsigned char *>(unit) + 2488);
    if (static_cast<int>(spellID) == savedChannelSpellID) {
      newDesc->lightningObjs[0] = 0;
      newDesc->lightningObjs[1] = 0;
      newDesc->lightningObjs[2] = 0;
      SpellVisualGetLightning(unit, const_cast<SpellVisualKitRec *>(rec), spellID, newDesc->lightningObjs, 3);
    }
    unit->ClearSavedChannelSpellTargets();
  } else if (action == SPELLPROCREMOVE) {
    newDesc->ClearLightningObjects();
    newDesc->~SPELLEFFECTDESC();
    s_spellEffectFreeList.PutData(newDesc, 0, 0);
  }
}

void __fastcall SpellProcColorHandler(
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
    unsigned int now = GetTickCount();
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

static float __fastcall GetSpellEffectDescScale(SPELLEFFECTDESC *desc) {
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

static float __fastcall GetDesiredRenderScale(SpellEffectList &list) {
  float currentScale = 1.0f;
  for (SPELLEFFECTDESC *desc = list.Head(); desc; desc = desc->Next()) {
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

void __fastcall SpellProcScaleHandler(
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
    unsigned int now = GetTickCount();
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
    *reinterpret_cast<float *>(reinterpret_cast<unsigned char *>(unit) + 12) = GetDesiredRenderScale(list);
  } else if (action == SPELLPROCUPDATE) {
    float oldScale = list.Head() ? GetDesiredRenderScale(list) : 1.0f;
    PurgeExpiredNodes(list, elapsed);
    float newScale = list.Head() ? GetDesiredRenderScale(list) : 1.0f;
    if (newScale != oldScale) {
      *reinterpret_cast<float *>(reinterpret_cast<unsigned char *>(unit) + 12) = newScale;
    }
  }
}

void __fastcall SpellProcEmissiveHandler(
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
    newDesc->~SPELLEFFECTDESC();
    s_spellEffectFreeList.PutData(newDesc, 0, 0);
  } else if (action == SPELLPROCUPDATE) {
    PurgeExpiredNodes(list, elapsed);
  }
}

void __fastcall SpellProcEclipseHandler(
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
    newDesc->startTime = GetTickCount();
    int castTime = Spell_C_GetCastTime(spellID, 0);
    newDesc->endTime = newDesc->startTime + castTime;
    newDesc->fadeInTime = newDesc->startTime + static_cast<unsigned int>(castTime * rec->m_characterParam[1]);
  } else if (action == SPELLPROCREMOVE) {
    newDesc->~SPELLEFFECTDESC();
    s_spellEffectFreeList.PutData(newDesc, 0, 0);
  } else if (action == SPELLPROCUPDATE) {
    PurgeExpiredNodes(list, elapsed);
  }
}

void __fastcall SpellProcStandWalkAnimHandler(
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
    newDesc->~SPELLEFFECTDESC();
    s_spellEffectFreeList.PutData(newDesc, 0, 0);
  } else if (action == SPELLPROCUPDATE) {
    PurgeExpiredNodes(list, elapsed);
  }

  int standAnim = 0;
  int walkAnim = 0;
  for (SPELLEFFECTDESC *desc = list.Head(); desc; desc = desc->Next()) {
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
static EmotesRec                          *s_talkEmotes[TALKANIM_NUMTALKANIMS];
static const unsigned char                 s_standStateStartsSheathe[9] = {0, 1, 1, 1, 1, 1, 1, 1, 1};
static const unsigned int                  s_standStateEndEmote[9] = {0, 4, 8, 6, 8, 8, 8, 0, 14};
static const unsigned int                  s_standStateStartEmote[9] = {0, 3, 7, 5, 9, 10, 11, 12, 13};

void __fastcall SpellProcWeaponTrailHandler(
    SPELLPROC_ACTION action,
    SpellEffectList &,
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
    newDesc->~SPELLEFFECTDESC();
    s_spellEffectFreeList.PutData(newDesc, 0, 0);
  }
}

static BLOODSPLATNODE *__fastcall NewBloodSplatNode() {
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

static NTempest::CImVector COLOR_GOLD(0xFFFFDE00);

int __fastcall OnPickNextStandHandler(void *param, CGUnit_C *ptr);
int __fastcall DeathAnimEndHandler(void *param, CGUnit_C *ptr);
int __fastcall PickNextRunHandler(void *param, CGUnit_C *ptr);
int __fastcall WoundAnimEndHandler(void *param, CGUnit_C *ptr);
int __fastcall SpellAnimEndHandler(void *param, CGUnit_C *ptr);
int __fastcall NPCAnimEndHandler(void *param, CGUnit_C *ptr);
int __fastcall JumpTakeOffFinishedHandler(void *param, CGUnit_C *ptr);
int __fastcall JumpLandFinishedHandler(void *param, CGUnit_C *ptr);
int __fastcall RangedWeaponAnimEndHandler(void *param, CGUnit_C *ptr);
int __fastcall LootAnimEndHandler(void *param, CGUnit_C *ptr);
int __fastcall SheatheAnimEndHandler(void *param, CGUnit_C *ptr);
int __fastcall SitSleepAnimEndHandler(void *param, CGUnit_C *ptr);
int __fastcall RangedPrecastEndHandler(void *param, CGUnit_C *ptr);
int __fastcall ThrowAnimEndHandler(void *param, CGUnit_C *ptr);
int __fastcall AttackAnimEndHandler(void *param, CGUnit_C *ptr);
int __fastcall DodgeAnimEndHandler(void *param, CGUnit_C *ptr);

static void ClearQuestIconHandles(int reinitialize) {
    // TODO: implement
}

void __fastcall InitTalkEmotes() {
  for (int index = g_emotesDB.GetNumRecords(); index;) {
    EmotesRec *emote = g_emotesDB.GetRecordByIndex(--index);
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

void __fastcall CreatureQueryCallback(int id, const unsigned __int64 &guid, void *, bool) {
  unsigned __int64       noGuid = 0;
  const CreatureStats_C *stats = g_creatureDBCache.GetRecord(id, noGuid, 0, 0);
  if (stats) {
    CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
    if (unitPtr) {
      unitPtr->m_stats = const_cast<CreatureStats_C *>(stats);
    }
    CGGameUI::UnitNameUpdate(guid);
  }
}

static void __fastcall NameQueryCallback(int id, const unsigned __int64 &guid, void *arg, bool granted) {
  if (granted) {
    CGObject_C *object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
    if (object) {
      CGGameUI::UnitNameUpdate(guid);
    }
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

int __fastcall SpellFizzleTimer(const void *__formal, void *userData) {
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(userData);
  FATALASSERT(unitPtr);
  unitPtr->EndSpellEffects(2);
  return 1;
}

static int __fastcall RangedStandTimerHandler(const void *__formal, void *userData) {
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(userData);
  FATALASSERT(unitPtr);
  unitPtr->OnRangedStandTimer();
  return 1;
}

void CGUnit_C::RemoveBloodPool() {
  reinterpret_cast<unsigned int *>(this)[313] &= ~0x10000u;
}

void CGUnit_C::AddBloodPool() {
  if (m_bloodRec) {
    reinterpret_cast<unsigned int *>(this)[313] |= 0x10000u;
  }
}

static void AnimEventCallback(const char* eventName, const NTempest::C3Vector& position, void* param) {
    // TODO: implement
}

static void MountedAnimEventCallback(const char* eventName, const NTempest::C3Vector& position, void* param) {
    // TODO: implement
}

int __fastcall UnitFlagUpdateHandler(unsigned __int64 unit, unsigned int, unsigned int, const void *oldValue, void *) {
  FATALASSERT(oldValue);
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  FATALASSERT(unitPtr);
  unitPtr->OnFlagChanged(*static_cast<const unsigned int *>(oldValue));
  return 1;
}

int __fastcall UnitLevelUpdateHandler(unsigned __int64 guid, unsigned int offset, unsigned int bytes, const void *oldValue, void *param) {
  int       oldLevel = *static_cast<const int *>(oldValue);
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (unitPtr->GetUnitData()->level != oldLevel) {
    unitPtr->OnLevelChange();
  }
  return 1;
}

int __fastcall UnitModeUpdateHandler(unsigned __int64 guid, unsigned int offset, unsigned int bytes, const void *oldValue, void *param) {
  FATALASSERT(oldValue);
  CGUnit_C              *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  const CreatureStats_C *stats = g_creatureDBCache.GetRecord(unitPtr->m_obj->m_entryID, guid, CreatureQueryCallback, 0);
  if (stats) {
    unitPtr->m_stats = const_cast<CreatureStats_C *>(stats);
    CGGameUI::UnitNameUpdate(guid);
  }
  return 1;
}

int __fastcall UnitHealthUpdateHandler(unsigned __int64 unit, unsigned int offset, unsigned int bytes, const void *oldValue, void *param) {
  FATALASSERT(oldValue);
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  int       oldHealth = *static_cast<const int *>(oldValue);
  FATALASSERT((unitPtr->GetType() & HIER_TYPE_UNIT) == HIER_TYPE_UNIT);
  FATALASSERT(unitPtr->GetUnitData()->maxHealth);

  const CGUnitData *unitData = unitPtr->GetUnitData();
  if (static_cast<double>(unitData->health) / static_cast<double>(unitData->maxHealth) < 0.2 && unitData->health > 0) {
    unitPtr->AddBloodPool();
  } else {
    unitPtr->RemoveBloodPool();
  }
  unitPtr->UpdateDisplayHealth();

  void **vtable = *reinterpret_cast<void ***>(unitPtr);
  typedef void (CGUnit_C::*UnitStateFn)();
  if (unitData->health <= 0 && oldHealth > 0) {
    if (unit == ClntObjMgrGetActivePlayer()) {
      CGInputControl::GetActive()->UpdatePlayer(GetTickCount());
    }

    UnitStateFn onDeath;
    memcpy(&onDeath, &vtable[0xC0 / 4], sizeof(onDeath));
    (unitPtr->*onDeath)();

    const unsigned int *self = reinterpret_cast<const unsigned int *>(unitPtr);
    if (!self[448]) {
      CGGameUI::ClearTarget(unit, 1);
      if (!(self[314] & 0x2000)) {
        UnitStateFn onDeathAnimate;
        memcpy(&onDeathAnimate, &vtable[0xC4 / 4], sizeof(onDeathAnimate));
        (unitPtr->*onDeathAnimate)();
      }
    }
  } else if (unitData->health > 0 && oldHealth <= 0) {
    if (unit == ClntObjMgrGetActivePlayer()) {
      CGInputControl::GetActive()->UpdatePlayer(GetTickCount());
    }
    reinterpret_cast<unsigned int *>(unitPtr)[313] &= ~1u;
    UnitStateFn restoreUnit;
    memcpy(&restoreUnit, &vtable[0x10C / 4], sizeof(restoreUnit));
    (unitPtr->*restoreUnit)();
  }
  return 1;
}

int __fastcall UnitCharmedUpdateHandler(unsigned __int64 unit, unsigned int offset, unsigned int bytes, const void *oldValue, void *param) {
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  if (unitPtr) {
    unitPtr->OnCharmedChanged();
  }
  return 1;
}

int __fastcall DisplayIDUpdateHandler(unsigned __int64 unit, unsigned int offset, unsigned int bytes, const void *oldValue, void *param) {
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  if (unitPtr) {
    unitPtr->UpdateDisplayInfo();
  }
  return 1;
}

int __fastcall StandStateUpdateHandler(unsigned __int64 unit, unsigned int offset, unsigned int bytes, const void *oldValue, void *param) {
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
  unsigned int snapOnClear;
  float        maxDistance;
};

static const TRACKTYPEINFO s_trackTypeInfo[3] = {
    {0, 100000.0f},
    {0, 100000.0f},
    {1,     30.0f}
};

int __fastcall NPCFlagsHandler(unsigned __int64 unit, unsigned int offset, unsigned int bytes, const void *oldValue, void *param) {
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  if (unitPtr) {
    unitPtr->NPCFlagChanged(*static_cast<const unsigned char *>(oldValue));
  }
  return 1;
}

int __fastcall WeaponModeUpdateHandler(unsigned __int64 unit, unsigned int offset, unsigned int bytes, const void *oldValue, void *param) {
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  if (unitPtr) {
    unitPtr->WeaponModeChanged();
  }
  return 1;
}

int __fastcall PetNameChangeHandler(unsigned __int64 unit, unsigned int, unsigned int, const void *, void *) {
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

int __fastcall VirtualItemChangeHandler(unsigned __int64 unit, unsigned int offset, unsigned int bytes, const void *oldValue, void *param) {
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  if (unitPtr) {
    unitPtr->VirtualComponentChanged(reinterpret_cast<int>(param), *static_cast<const int *>(oldValue));
  }
  return 1;
}

int __fastcall DynamicFlagsChangeHandler(unsigned __int64 unit, unsigned int offset, unsigned int bytes, const void *oldValue, void *param) {
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  if (unitPtr) {
    unitPtr->OnDynamicFlagsChanged(*static_cast<const unsigned int *>(oldValue));
  }
  return 1;
}

int __fastcall EmoteStateChangeHandler(unsigned __int64 unit, unsigned int offset, unsigned int bytes, const void *oldValue, void *param) {
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  if (unitPtr) {
    unitPtr->ClearTorsoAnimation(0);
  }
  return 1;
}

ACTIVEATTACHMENTINFO::~ACTIVEATTACHMENTINFO() {
  Clear();
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

int __fastcall ChannelSpellChangeHandler(unsigned __int64 unit, unsigned int offset, unsigned int bytes, const void *oldValue, void *param) {
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  if (unitPtr) {
    unitPtr->OnChannelSpellChanged(*static_cast<const unsigned int *>(oldValue));
  }
  return 1;
}

static int OnUnitMoveEvent(NETMESSAGE msgId, unsigned long eventTime, unsigned __int64 guid, CDataStore* msg) {
    // TODO: implement
    return 0;
}

static int OnUnitMoveEventActive(void* param, NETMESSAGE msgId, unsigned long eventTime, CDataStore* msg) {
    // TODO: implement
    return 0;
}

static int OnUnitMoveEventNoActive(void* param, NETMESSAGE msgId, unsigned long eventTime, CDataStore* msg) {
    // TODO: implement
    return 0;
}

static int OnMonsterMoveEvent(void* param, NETMESSAGE msgId, unsigned long eventTime, CDataStore* msg) {
    // TODO: implement
    return 0;
}

static int OnForceMoveChange(void*, NETMESSAGE msgId, unsigned long eventTime, CDataStore* msg) {
    // TODO: implement
    return 0;
}

static int OnUnitMountCancelledEvent(void* param, NETMESSAGE msgId, unsigned long eventTime, CDataStore* msg) {
    // TODO: implement
    return 0;
}

static int OnSpecialMountAnim(void* param, NETMESSAGE msgId, unsigned long eventTime, CDataStore* msg) {
    // TODO: implement
    return 0;
}

static int OnUnitReaction(void*, NETMESSAGE msgId, unsigned long, CDataStore* msg) {
    // TODO: implement
    return 0;
}

int __fastcall AuraMirrorHandler(unsigned __int64 guid, unsigned int offset, unsigned int bytes, const void *prevValue, void *param) {
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
    // TODO: implement
    return 0;
}

static int ChannelObjectMirrorHandler(unsigned __int64 guid, unsigned int, unsigned int, const void*, void*) {
    // TODO: implement
    return 0;
}

static void PlayerNameGuildCallback(int guildID, const unsigned __int64& guid, void* arg, unsigned char granted) {
    // TODO: implement
}

void __fastcall OnPendingMoveStateChange(unsigned __int64 unit, int msgId, unsigned long eventTime) {
  CGUnit_C *mover = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  FATALASSERT(mover);
  mover->OnPendingMoveStateChange(static_cast<NETMESSAGE>(msgId));
}

void __fastcall OnCollideRedirected(unsigned __int64 unit, unsigned long eventTime) {
  if (unit == CGUnit_C::m_activeMover) {
    CGUnit_C *mover = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
    FATALASSERT(mover);
    mover->SendRedirectionMessage();
  }
}

void __fastcall OnCollideStuck(unsigned __int64 unit, unsigned long eventTime) {
  if (unit == CGUnit_C::m_activeMover) {
    CGUnit_C *mover = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
    FATALASSERT(mover);

    CDataStore msg;
    mover->BuildMovementUpdate(MSG_MOVE_COLLIDE_STUCK, &msg);
    msg.Finalize();
    ClientServices_Send(&msg);
  }
}

void __fastcall OnCollideFallLand(unsigned __int64 unit, unsigned long eventTime) {
  CGUnit_C *mover = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  FATALASSERT(mover);
  mover->OnCollideFallLand(eventTime);
}

void __fastcall OnCollideFalling(unsigned __int64 unit, unsigned long eventTime) {
  CGUnit_C *mover = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  FATALASSERT(mover);
  mover->OnCollideFalling(eventTime);
}

void __fastcall OnMoveUpdate(unsigned __int64 unit, unsigned long eventTime) {
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
    unitptr->m_movement.m_waterSurfaceElev = surfaceColPt - unitptr->GetObjectHeight() * 0.75f;
  }

  if (unit == CGUnit_C::m_activeMover) {
    unsigned int moveFlags = unitptr->m_movement.m_moveFlags;
    if ((moveFlags & 0x01000000) || ((unitptr->GetType() & TYPE_PLAYER) && !unitptr->m_movement.m_transportGUID &&
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

int __fastcall UnitGetObjectPosition(const unsigned __int64 &guid, NTempest::C3Vector *position) {
  CGObject_C *object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
  if (!object) {
    return 0;
  }

  *position = object->GetPosition();
  return 1;
}

float __fastcall UnitCalculateFacingTo(NTempest::C3Vector &position, NTempest::C3Vector &destination) {
  return CalculateFacingTo(position, destination);
}

void __fastcall UnitUpdateMovementAnim(const unsigned __int64& unit) {
    // TODO: implement
}

void __fastcall UnitNotifyStopped(const unsigned __int64 &, bool) {
}

void CGUnit_C::SetStorage(unsigned long *storage) {
  CGObject_C::SetStorage(storage);
  m_unit = reinterpret_cast<CGUnitData *>(storage + 6);
}

CGUnit_C::CGUnit_C(unsigned long *storage, unsigned long eventTime, CClientObjCreate *init)
    : CGObject_C(storage, eventTime, init),
      m_unitVTable(0),
      m_unitUnknown(0),
      m_unit(reinterpret_cast<CGUnitData *>(storage + 6)),
      m_unitPadding(0),
      m_movement(init->move.status.worldPosition, init->move.status.worldFacing, *reinterpret_cast<unsigned __int64 *>(storage)),
      m_questCountKilled(-1),
      m_questCountNeeded(-1),
      m_resEffectModel(0),
      m_meleeTargetDeathHold(0),
      m_precastSheatheHoldTimer(0),
      m_customAttackSound(0),
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
      m_spellPrecastingAnim(ANIM_STAND),
      m_spellCastingAnim(ANIM_STAND),
      m_deferredPrecastAnim(static_cast<ANIMENUMERATION>(-1)),
      m_animatingAura(0),
      m_emoteID(-1),
      m_spellCastingEffectKit(-1),
      m_spellCastingSoundID(0),
      m_spellCastingCameraShakeID(0),
      m_lastSentFacing(0.0f),
      m_lastSentPitch(0.0f),
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
      m_deferredSheatheReason(SHEATHEREASON_0),
      m_savedChannelSpellID(0),
      m_channelSpellEffect(0),
      m_shapeShiftPoof(0),
      m_fishingLineObject(0) {
  memset(m_callbackList, 0, sizeof(m_callbackList));
  memset(&m_combat, 0, sizeof(m_combat));
  memset(m_savedFacingDeltas, 0, sizeof(m_savedFacingDeltas));
  memset(m_preferredGeosets, 0, sizeof(m_preferredGeosets));
  memset(m_auraFlags, 0, sizeof(m_auraFlags));
  memset(m_attachments, 0, sizeof(m_attachments));
  memset(m_deferredAttachments, 0, sizeof(m_deferredAttachments));
  memset(m_weaponTrails, 0, sizeof(m_weaponTrails));
  m_handAnim[0] = static_cast<ANIMENUMERATION>(-1);
  m_handAnim[1] = static_cast<ANIMENUMERATION>(-1);
  SetClientInitData(eventTime, *init, 0);
  RefreshDataPointers();
  AddWorldObject();
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
  HTEXTURE       skinTexture;
  HTEXCOMPONENT &texComponent = *reinterpret_cast<HTEXCOMPONENT *>(reinterpret_cast<unsigned int *>(this) + 477);

  if (*displayTextureName) {
    char preBakeName[256];
    SStrPrintf(preBakeName, sizeof(preBakeName), "%s%s", "Textures\\BakedNpcTextures\\", displayTextureName);
    skinTexture = CharCustomizationLoadSkin(charModel, preBakeName, race, sex, SkinVariationID(), 1);
    if (texComponent) {
      HandleClose(texComponent);
    }
    texComponent = 0;
    CharCustomizationSetHairTexture(charModel, 0, race, sex, HairStyleID(), HairColorID());
  } else {
    skinTexture = CharCustomizationSetSkin(charModel, race, sex, SkinVariationID(), 1);
    if (!skinTexture) {
      FATALERROR(
          ("Error, skinID %d on character %s (race/sex is %d/%d)cannot be loaded, is it a missing file?", SkinVariationID(), GetUnitName(), race, sex)
      );
    }
    if (texComponent) {
      HandleClose(texComponent);
    }
    texComponent = TexComponentCreate(skinTexture, race, sex, SkinVariationID(), 1, 0);
    FATALASSERT(texComponent);
    CharCustomizationSetFaceTexture(charModel, texComponent, race, sex, FaceID(), SkinVariationID(), 1);
    CharCustomizationSetHairTexture(charModel, texComponent, race, sex, HairStyleID(), HairColorID());
    CharCustomizationSetFacialTexture(charModel, texComponent, race, sex, FacialHairID(), HairColorID());
  }

  BEARDSTYLEDATA beardStyleData = {1, 1, 1};
  int            hasFacialInfo = CharCustomizationGetBeardStyle(race, sex, FacialHairID(), &beardStyleData);

  HCHARGEOSET &geosetHandle = *reinterpret_cast<HCHARGEOSET *>(reinterpret_cast<unsigned int *>(this) + 476);
  if (geosetHandle) {
    HandleClose(geosetHandle);
  }
  geosetHandle = CharCustomizationCreateGeosetHandle(charModel);
  FATALASSERT(geosetHandle);
  InitPreferredGeosets();
  CharCustomizationInitBaseCharacter(
      geosetHandle, hasFacialInfo ? beardStyleData.beardGeoset : 1, hasFacialInfo ? beardStyleData.sideBurnGeoset : 1,
      hasFacialInfo ? beardStyleData.moustacheGeoset : 1, 2
  );
  CharCustomizationResetHairGeoset(geosetHandle, race, sex, HairStyleID());

  if (skinTexture) {
    HandleClose(skinTexture);
  }
  HandleClose(charModel);
}

void CGUnit_C::PostInit(const CClientObjCreate &init) {
  PostSetClientInitData(init.move);
  m_baseRadius = m_unit->boundingRadius;
  CGObject_C::PostInit(init);
  SetupFootprints();
  RefreshAuraVisuals();
  InitializeExtendedDisplay();
  UpdateDisplayHealth();
  SetAuraMirrorHandlers();
  SetSmoothFacing(GetFacing());
  UpdateBaseAnimation(0);

  for (unsigned int slot = 0; slot < 56; ++slot) {
    if (m_unit->auras[slot]) {
      AddAuraEffect(slot, 0);
      RefreshAuraVisuals();
    }
  }
}

void CGUnit_C::PostMovementUpdate(CClientMoveUpdate &update) {
  PostSetClientInitData(update);
  UpdateBaseAnimation(0);
}

void CGUnit_C::SetupFootprints() {
  FATALASSERT(m_modelData);
  m_footprintTextureID = m_modelData->m_footprintTextureID;
  m_footprintSize.x = m_modelData->m_footprintTextureWidth / 36.0f;
  m_footprintSize.y = m_modelData->m_footprintTextureLength / 36.0f;
  m_footprintParticleScale = m_modelData->m_footprintParticleScale;
}

void __fastcall CGUnit_C::InitializeTextureVariations(
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
  const float *self = reinterpret_cast<const float *>(this);
  return self[55] + self[55] >= const_cast<CMovement &>(m_movement).GetCurrentSpeed();
}

unsigned int CGUnit_C::GetAnimationState() {
  unsigned int *self = reinterpret_cast<unsigned int *>(this);
  if (m_unit->health <= 0 && ((self[314] & 0x2000) || !(reinterpret_cast<const unsigned char *>(this)[1252] & 4))) {
    typedef void (CGUnit_C::*SetTorsoAnimStateProc)(unsigned int);
    SetTorsoAnimStateProc setTorsoAnimState;
    void                **vtable = *reinterpret_cast<void ***>(this);
    memcpy(&setTorsoAnimState, &vtable[0x118 / sizeof(void *)], sizeof(setTorsoAnimState));
    (this->*setTorsoAnimState)(0);
    return 1;
  }

  if ((GetType() & TYPE_PLAYER)) {
    unsigned int playerState = static_cast<CGPlayer_C *>(this)->GetPlayerAnimState();
    if (playerState) {
      return playerState;
    }
  }

  unsigned int moveFlags = self[32];
  if (moveFlags & 0x8000) {
    return 43;
  }
  if (reinterpret_cast<float *>(this)[62] != 0.0f) {
    return 41;
  }
  if (moveFlags & 0x2000000) {
    if (moveFlags & 2) {
      return 24;
    }
    if (moveFlags & 0xC) {
      return ((~moveFlags & 4) | 0x58) >> 2;
    }
    return ((moveFlags & 0xF) != 0) + 20;
  }
  if (moveFlags & 2) {
    if (moveFlags & 0xC) {
      return ((~moveFlags & 4) | 0x40) >> 2;
    }
    return 7;
  }
  if ((moveFlags & 0xF) == 0xF) {
    return IsWalking() ? 12 : 14;
  }
  if (moveFlags & 4) {
    return IsWalking() ? 8 : 10;
  }
  if (moveFlags & 8) {
    return IsWalking() ? 9 : 11;
  }
  if ((moveFlags & 1) && m_movement.GetCurrentSpeed() > 0.0f) {
    return 6 - (IsWalking() != 0);
  }

  if (GetGUID() == ClntObjMgrGetActivePlayer() && (self[634] & 0x400) && !m_unit->standState) {
    return 31;
  }
  if (!(m_unit->flags & 0x20000) && reinterpret_cast<CCombat *>(reinterpret_cast<unsigned char *>(this) + 0x4C0)->IsAttacking()) {
    return 31;
  }
  if (self[308]) {
    return 31;
  }
  if (m_unit->weaponMode == WEAPONMODE_RANGED && self[600]) {
    return 31;
  }
  return s_standStateAnimState[m_unit->standState];
}

void CGUnit_C::GenericAnimEndHandler(ANIMENUMERATION animID, void *param) {
  if (reinterpret_cast<const unsigned int *>(this)[442] == 46 && GetCurrentTorsoAnim() == animID) {
    ClearTorsoAnimation(0x40);
  } else if (g_seqInformation[animID].handler) {
    g_seqInformation[animID].handler(param, this);
  }
}

static int __fastcall IsSitStandSleepTransition(unsigned int animState);

int CGUnit_C::PlayBaseAnimation(int newAnimState, int newAnim, int forceNoFidget, bool &checkImpacts) {
  checkImpacts = false;
  int    result = 0;
  HMODEL charModel = GetCharacterModel(0);
  FATALASSERT(charModel);

  int flag = 0;
  if (s_animInfo[newAnimState].flags & 0x4000) {
    flag = 8;
  }
  if (forceNoFidget) {
    flag |= 1;
  }

  if (!GetCurrentTorsoAnimState() || (s_animInfo[newAnimState].flags & 8)) {
    CheckPendingMissileRelease(0);
    CheckPendingVictimFeedback();
    if (GetCurrentTorsoAnimState() == 46) {
      typedef void (CGUnit_C::*ClearEmoteStateProc)(unsigned int);
      ClearEmoteStateProc clearEmoteState;
      void              **vtable = *reinterpret_cast<void ***>(this);
      memcpy(&clearEmoteState, &vtable[0x164 / sizeof(void *)], sizeof(clearEmoteState));
      (this->*clearEmoteState)(0);
    }

    switch (GetCurrentTorsoAnimState()) {
      case 16:
      case 17:
      case 18:
      case 19:
      case 46:
      case 49:
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

    result = ObjectModelSetSequence(charModel, newAnim, GetObjAnimFlags(flag | 2), 0);
    if (GetCurrentTorsoAnimState()) {
      StoreSequenceEndCallbacks(GetCurrentTorsoAnim());
    }
    if (newAnimState == 41) {
      unsigned int fallTime = m_movement.FallTime();
      ModelForceSequenceTime(charModel, newAnim, fallTime, 0);
      if (fallTime < 300) {
        typedef void (CGUnit_C::*PlayUnitSoundProc)(UNITSOUNDTYPE, int) const;
        PlayUnitSoundProc playUnitSound;
        void            **vtable = *reinterpret_cast<void ***>(this);
        memcpy(&playUnitSound, &vtable[0xF4 / sizeof(void *)], sizeof(playUnitSound));
        (this->*playUnitSound)(static_cast<UNITSOUNDTYPE>(14), 1);
      }
    }
  } else {
    if (s_animInfo[GetCurrentBaseAnimState()].flags & 8) {
      SetTorsoAnimation(GetCurrentTorsoAnimState(), 0, 0x40);
    }
    if (newAnimState == 3 && (s_animInfo[GetCurrentTorsoAnimState()].flags & 2)) {
      SetBaseAnim(GetCurrentTorsoAnim());
      flag |= 4;
    }

    if (ModelLockObjectSequence(charModel, 4, 1)) {
      result = ObjectModelSetSequence(charModel, newAnim, GetObjAnimFlags(flag), 0);
      ModelLockObjectSequence(charModel, 4, 0);
      checkImpacts = true;
    } else if (s_animInfo[newAnimState].basePriority > s_animInfo[GetCurrentTorsoAnimState()].basePriority) {
      result = ObjectModelSetSequence(charModel, newAnim, GetObjAnimFlags(flag | 2), 0);
      CheckPendingImpactKit();
    }
  }

  UpdateMovementAnimSpeed(0, newAnimState);
  HandleClose(charModel);
  return result;
}

void CGUnit_C::SetStrafeRotation() {
  unsigned int       moveFlags = reinterpret_cast<unsigned int *>(this)[32];
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

  HMODEL model = reinterpret_cast<HMODEL *>(this)[4];
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
    HMODEL model = reinterpret_cast<HMODEL *>(this)[4];
    ModelRemoveObjectFaceDir(model, 5);
    ModelRemoveObjectFaceDir(model, 4);
    ModelRemoveObjectFaceDir(model, 6);
  }
}

bool CGUnit_C::BaseAnimLocksHead() const {
  const unsigned int *self = reinterpret_cast<const unsigned int *>(this);
  if (s_animInfo[self[440]].flags & 1) {
    return true;
  }
  return (g_seqInformation[self[441]].flags & 2) != 0;
}

bool CGUnit_C::TorsoAnimOverridesBase() const {
  const unsigned int *self = reinterpret_cast<const unsigned int *>(this);
  return (s_animInfo[self[442]].flags & 0x8000) || (g_seqInformation[GetCurrentTorsoAnim()].flags & 8);
}

void CGUnit_C::UpdateMountAnimation(unsigned int newState, unsigned int flags) {
  FATALASSERT(newState < 64);

  unsigned int *self = reinterpret_cast<unsigned int *>(this);
  if ((m_flags & 0x10) && (self[444] != newState || (flags & 0x100))) {
    self[444] = newState;
    unsigned int newAnim = ChooseAnimation(newState);
    HMODEL       model = reinterpret_cast<HMODEL *>(this)[4];
    ObjectModelSetSequence(model, newAnim, GetObjAnimFlags(2), 0);
    UpdateMovementAnimSpeed(1, newState);

    unsigned int animFlags = s_animInfo[newState].flags;
    if (((flags & 0x100) && (animFlags & 0x400)) || (animFlags & 0x40) || ((animFlags & 0x800) && !(m_animFlags & 4))) {
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
  unsigned int *self = reinterpret_cast<unsigned int *>(this);
  if (IsPreemptableWoundAnimState(self[442])) {
    self[445] = GetTickCount();
    unsigned int anim = GetCurrentTorsoAnim();
    if (!ModelGetSequenceDuration(theModel, anim, &self[446])) {
      goto sequence_failed;
    }
    FATALASSERT(self[446]);
  }

  if (!(s_animInfo[self[440]].flags & 4) || TorsoAnimOverridesBase()) {
    if (TorsoAnimOverridesBase()) {
      ModelRemoveObjectFaceDir(reinterpret_cast<HMODEL *>(this)[4], 5);
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
  typedef void (CGUnit_C::*SetTorsoAnimStateProc)(unsigned int);
  SetTorsoAnimStateProc setTorsoAnimState;
  void                **vtable = *reinterpret_cast<void ***>(this);
  memcpy(&setTorsoAnimState, &vtable[0x118 / sizeof(void *)], sizeof(setTorsoAnimState));
  (this->*setTorsoAnimState)(0);
  return 0;
}

float CGUnit_C::GetAnimTimeScale(unsigned int sequence, unsigned int duration, unsigned int flags) {
  HMODEL theModel = GetCharacterModel(0);
  FATALASSERT(theModel);

  float        timeScale = 1.0f;
  unsigned int seqInfoDuration;
  if (ModelGetSequenceDuration(theModel, sequence, &seqInfoDuration)) {
    unsigned int *self = reinterpret_cast<unsigned int *>(this);
    self[311] = seqInfoDuration;
    self[312] = GetTickCount();
    self[310] = duration + self[312];
    if (seqInfoDuration && duration && seqInfoDuration != duration && !(flags & 0x20) && (!(flags & 0x10) || seqInfoDuration > duration)) {
      timeScale = static_cast<float>(seqInfoDuration) / static_cast<float>(duration);
    }
  }

  HandleClose(theModel);
  return timeScale;
}

void CGUnit_C::StoreSequenceEndCallbacks(int anim) {
  ANIMENDDATA *data = m_animEndCallbackList.New();
  data->guid = GetGUID();
  data->anim = static_cast<ANIMENUMERATION>(anim);
}

void CGUnit_C::ProcessAnimEndCallbacks() {
  unsigned int count = m_animEndCallbackList.Count();
  while (count) {
    --count;
    ANIMENDDATA &data = m_animEndCallbackList[count];
    GenericAnimEndHandler(data.anim, &data);
  }
  m_animEndCallbackList.SetCount(0);
}

int CGUnit_C::SetTorsoAnimation(unsigned int state, unsigned long duration, unsigned int flags) {
  FATALASSERT(state < 64);

  unsigned int *self = reinterpret_cast<unsigned int *>(this);
  void        **vtable = *reinterpret_cast<void ***>(this);
  typedef int (CGUnit_C::*GetStandStateAnimProc)(HMODEL);
  typedef void (CGUnit_C::*SetAnimStateProc)(unsigned int);
  GetStandStateAnimProc getStandStateAnim;
  SetAnimStateProc      setTorsoAnimState;
  SetAnimStateProc      setBaseAnimState;
  memcpy(&getStandStateAnim, &vtable[0x184 / sizeof(void *)], sizeof(getStandStateAnim));
  memcpy(&setTorsoAnimState, &vtable[0x118 / sizeof(void *)], sizeof(setTorsoAnimState));
  memcpy(&setBaseAnimState, &vtable[0x110 / sizeof(void *)], sizeof(setBaseAnimState));

  if (!state) {
    if (!self[442]) {
      return 1;
    }

    HMODEL tempCharModel = reinterpret_cast<HMODEL>(self[422]);
    if (!tempCharModel || (this->*getStandStateAnim)(tempCharModel) <= 0) {
      CheckLevelUpAnimFlag(self[442], 0);
      SetTorsoAnim(GetStandStateAnim(0));
      (this->*setTorsoAnimState)(0);
      self[310] = 0;
      (this->*setBaseAnimState)(0);
      SetTorsoAnim(ChooseAnimation(self[440]));

      if (GetCurrentTorsoAnim() == ANIM_STEALTHSTAND ||
          (!GetCurrentTorsoAnim() &&
           (reinterpret_cast<CCombatClient *>(reinterpret_cast<unsigned char *>(this) + 0x4C0)->IsAttacking() || !m_unit->health)))
      {
        flags |= 1;
      }

      return SetTorsoSequence(GetAnimTimeScale(GetCurrentTorsoAnim(), duration, flags), flags);
    }

    state = 37;
  }

  CheckLevelUpAnimFlag(self[442], state);
  if (g_seqInformation[self[441]].flags & 4) {
    return 0;
  }
  if (self[442] == state && !(s_animInfo[state].flags & 0x20)) {
    return 0;
  }
  if (!(s_animInfo[self[440]].flags & 0x80) && (s_animInfo[self[440]].flags & 8) && !(flags & 0x40)) {
    return 0;
  }
  if (self[442] && s_animInfo[self[442]].basePriority > s_animInfo[state].basePriority + s_animInfo[state].priorityOffset && !(flags & 0x40)) {
    return 0;
  }

  unsigned int anim = ChooseAnimation(state);
  if (anim == static_cast<unsigned int>(-1)) {
    return 1;
  }
  if (anim == ANIM_HOLDBOW) {
    SetRangedWeaponPullAnim(0);
  }
  SetTorsoAnim(anim);
  (this->*setTorsoAnimState)(state);
  return SetTorsoSequence(GetAnimTimeScale(GetCurrentTorsoAnim(), duration, flags), flags);
}

int CGUnit_C::ClearTorsoAnimation(unsigned int flags) {
  int           anim;
  unsigned int *self = reinterpret_cast<unsigned int *>(this);

  if (IsSpellAuraAnimActive(anim)) {
    self[391] = anim;
    SetTorsoAnimation(63, 0, flags);
  } else if (IsSpellChannelAnimActive(anim)) {
    self[391] = anim;
    SetTorsoAnimation(62, 0, flags);
  } else if (m_unit->channelSpell) {
    PlayEmoteAnimation(m_unit->channelSpell, flags);
  } else {
    if (self[293] && self[442] == 46) {
      unsigned int  index = self[293] - 1;
      unsigned int *emote = reinterpret_cast<unsigned int *>(self[294] + 8 * index);
      *emote += GetTickCount();
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

  int *handAnim = reinterpret_cast<int *>(this) + 618;
  handAnim[0] = -1;
  handAnim[1] = -1;
  if (!(reinterpret_cast<const unsigned char *>(this)[1258] & 1)) {
    UpdateSheatheRangedReasons(1);
  }
}

void CGUnit_C::UpdateMoveInfo(unsigned long eventTime, CClientMoveUpdate &update) {
  m_movement.SetUpdateInfo(eventTime, update, GetGUID() == ClntObjMgrGetActivePlayer());
}

void CGUnit_C::SetClientInitData(unsigned long eventTime, CClientObjCreate &init, unsigned int partialUpdateOfActivePlayer) {
  m_combat.SetClientInitData(init);
  if (!partialUpdateOfActivePlayer) {
    UpdateMoveInfo(eventTime, init.move);
  }
}

void CGUnit_C::PostSetClientInitData(const CClientMoveUpdate &update) {
  m_movement.UpdateTransportStatus(update.status);
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
    typedef unsigned __int64 (CGUnit_C::*GetLookAtTargetProc)() const;
    GetLookAtTargetProc getLookAtTarget;
    void              **vtable = *reinterpret_cast<void ***>(this);
    memcpy(&getLookAtTarget, &vtable[0xA4 / sizeof(void *)], sizeof(getLookAtTarget));
    targetGUID = (this->*getLookAtTarget)();
  } else {
    targetGUID = *reinterpret_cast<const unsigned __int64 *>(reinterpret_cast<const unsigned char *>(m_unit) + 40);
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

int __fastcall OnAuraDecayFinished(void *param) {
  AuraDecayNode *decay = static_cast<AuraDecayNode *>(param);
  CGObject_C    *object = ClntObjMgrObjectPtr(decay->unit, __FILE__, __LINE__);

  if (object && !decay->visual.IsWorldModel()) {
    GEOCOMPONENTLINKS linkPoint = UnitEffectGetLinkPointFromAttachment(decay->attach);
    FATALASSERT(linkPoint != static_cast<GEOCOMPONENTLINKS>(-1));

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
  decay->~AuraDecayNode();
  s_auraDecayFreeList.PutData(decay, 0, 0);
  return 0;
}

void CGUnit_C::UpdateBaseAnimation(unsigned int flags) {
  UpdateBaseAnimation(m_currentBaseAnimState, flags);
}

void CGUnit_C::FinishAuraDecays() {
  AuraDecayNode *decay = s_activeAuraDecays.Head();
  while (decay) {
    AuraDecayNode *next = decay->Next();
    if (decay->unit == GetGUID()) {
      OnAuraDecayFinished(decay);
    }
    decay = next;
  }
}

unsigned int CGUnit_C::IsSpellAuraAnimActive(int &anim) {
  int slot = reinterpret_cast<const int *>(this)[393];
  if (slot == -1) {
    return 0;
  }

  SpellRec          *spell = g_spellDB.GetRecord(m_unit->auras[slot]);
  SpellVisualRec    *visual = spell ? g_spellVisualDB.GetRecord(spell->m_spellVisualID) : 0;
  SpellVisualKitRec *kit = visual ? g_spellVisualKitDB.GetRecord(visual->m_stateKit) : 0;
  if (!kit) {
    return 0;
  }

  anim = kit->m_anim;
  return anim > 0;
}

void CGUnit_C::UpdateBaseAnimation(unsigned int newState, unsigned int flags) {
  FATALASSERT(newState < 64);
  if (!newState) {
    return;
  }

  typedef void (CGUnit_C::*SetBaseAnimStateProc)(unsigned int);
  SetBaseAnimStateProc setBaseAnimState;
  void               **vtable = *reinterpret_cast<void ***>(this);
  memcpy(&setBaseAnimState, &vtable[0x11C / sizeof(void *)], sizeof(setBaseAnimState));

  if (TorsoAnimOverridesBase() && !(s_animInfo[newState].flags & 8)) {
    (this->*setBaseAnimState)(newState);
    return;
  }

  if ((m_unit->flags & 0x2000) && (s_animInfo[newState].flags & 0x2000)) {
    UpdateMountAnimation(newState, flags);
    return;
  }

  unsigned int *self = reinterpret_cast<unsigned int *>(this);
  if (!m_unit->emoteState && !self[448] && newState != 1) {
    return;
  }

  unsigned int newAnim = ChooseAnimation(newState);
  ApplyStrafeRotation(newState);
  unsigned int force = flags & 0x100;
  if (!force && GetCurrentBaseAnimState() == newState && GetCurrentBaseAnim() == newAnim) {
    return;
  }

  FATALASSERT((GetCurrentBaseAnimState() != 1) || (GetCurrentTorsoAnimState() != 27));
  if (m_flags & 4) {
    CheckPendingThrownWeaponReattach(0);
  }

  int  forceNoFidget = newAnim == 120 || (!newAnim && m_combat.IsAttacking());
  bool checkImpacts;
  if (PlayBaseAnimation(newState, newAnim, forceNoFidget, checkImpacts)) {
    SetBaseAnim(newAnim);
    (this->*setBaseAnimState)(newState);
    LookAtTarget();
    if (checkImpacts) {
      CheckPendingImpactKit();
    }

    unsigned int currentFlags = s_animInfo[GetCurrentBaseAnimState()].flags;
    if ((flags & 0x80) || (force && (currentFlags & 0x400)) || (currentFlags & 0x40) || ((currentFlags & 0x800) && !(m_animFlags & 4))) {
      ModelForceSequenceTime(reinterpret_cast<HMODEL *>(this)[4], newAnim, 0x7FFFFFFF, 0);
    }
  }
}

unsigned int CGUnit_C::IsSpellChannelAnimActive(int &anim) {
  SpellRec          *spell = g_spellDB.GetRecord(m_unit->channelSpell);
  SpellVisualRec    *visual = spell ? g_spellVisualDB.GetRecord(spell->m_spellVisualID) : 0;
  SpellVisualKitRec *kit = visual ? g_spellVisualKitDB.GetRecord(visual->m_channelKit) : 0;
  if (!kit) {
    return 0;
  }

  anim = kit->m_anim;
  return anim > 0;
}

ACTIVEAURAINFO *CGUnit_C::FindActiveAuraInfo(int slot) {
  ACTIVEAURAINFO *active = m_activeAuraInfo.Head();
  while (active && active->slot != slot) {
    active = active->Next();
  }
  return active;
}

void CGUnit_C::RefreshAuraVisuals() {
  SpellVisualKitRec *highestPrioritiesByKit[12];
  SpellRec          *highestPrioritySpellFoundByKit[12];
  ACTIVEAURAINFO    *highestSpellWithAnim = 0;
  ACTIVEAURAINFO    *highestPriorityAura = 0;

  memset(highestPrioritiesByKit, 0, sizeof(highestPrioritiesByKit));
  memset(highestPrioritySpellFoundByKit, 0, sizeof(highestPrioritySpellFoundByKit));

  for (ACTIVEAURAINFO *curr = m_activeAuraInfo.Head(); curr; curr = curr->Next()) {
    FATALASSERT(curr->stateKitRec);

    SpellRec *spellRec = g_spellDB.GetRecord(m_unit->auras[curr->slot]);
    if (!spellRec) {
      continue;
    }

    SpellVisualKitRec *kitRec = curr->stateKitRec;
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

  unsigned int *self = reinterpret_cast<unsigned int *>(this);
  if ((self[442] == 38 || self[442] == 63) && (!highestSpellWithAnim || static_cast<int>(self[393]) != highestPriorityAura->slot)) {
    self[393] = -1;
    if (self[442] == 63) {
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
    self[391] = highestSpellWithAnim->stateKitRec->m_anim;
    self[393] = highestSpellWithAnim->slot;
    if (!SetTorsoAnimation(63, 0, 0)) {
      SetTorsoAnimation(0, 0, 0);
    }
  }
}

void CGUnit_C::AddPendingShapeshiftEffect(int oldSpell) {
  SpellRec *spell = g_spellDB.GetRecord(oldSpell);
  if (spell && IsShapeshiftSpell(spell)) {
    *reinterpret_cast<SpellRec **>(reinterpret_cast<unsigned char *>(this) + 0x9D4) = spell;
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

unsigned int __fastcall VisualHasDecay(HMODEL model) {
  return model && ModelHasSequenceId(model, 2);
}

void CGUnit_C::RemoveAuraVisual(UNITEFFECTATTACHPPOINT attach) {
  GEOCOMPONENTLINKS linkPoint = UnitEffectGetLinkPointFromAttachment(attach);
  FATALASSERT(linkPoint != static_cast<GEOCOMPONENTLINKS>(-1));

  AuraVisual &visual = m_auraVisual[attach];
  if (!visual.HasArt()) {
    return;
  }

  FATALASSERT(visual.GetModel());
  if (visual.IsWorldModel()) {
    HMODEL model = visual.GetModel();
    if (VisualHasDecay(model)) {
      AuraDecayNode *decay = new (s_auraDecayFreeList.GetData(0, typeid(AuraDecayNode).raw_name(), -2)) AuraDecayNode;
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
    AuraDecayNode *decay = new (s_auraDecayFreeList.GetData(0, typeid(AuraDecayNode).raw_name(), -2)) AuraDecayNode;
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
    active->~ACTIVEAURAINFO();
    s_auraInfoFreeList.PutData(active, 0, 0);
  }

  for (unsigned int attach = 0; attach < sizeof(m_auraVisual) / sizeof(m_auraVisual[0]); ++attach) {
    if (m_auraVisual[attach].GetSpellID() == static_cast<unsigned int>(previousSpell)) {
      RemoveAuraVisual(static_cast<UNITEFFECTATTACHPPOINT>(attach));
    }
  }

  if (GetGUID() != ClntObjMgrGetActivePlayer()) {
    return;
  }

  SpellRec *spellRec = g_spellDB.GetRecord(previousSpell);
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

void CGUnit_C::AddAuraEffect(unsigned int slot, unsigned int startNow) {
  FATALASSERT(slot < sizeof(m_unit->auras) / sizeof(m_unit->auras[0]));
  FATALASSERT(reinterpret_cast<unsigned int *>(this)[4]);

  if (FindActiveAuraInfo(slot)) {
    return;
  }

  int spellID = m_unit->auras[slot];
  if (!spellID) {
    return;
  }

  SpellRec *spellRec = g_spellDB.GetRecord(spellID);
  if (!spellRec) {
    SysMsgPrintf(SYSMSG_ERROR, 2, "NOSPELLIDFOUND|%d", spellID);
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

  SpellVisualRec *spellVisualRec = g_spellVisualDB.GetRecord(spellRec->m_spellVisualID);
  if (!spellVisualRec) {
    SysMsgPrintf(SYSMSG_ERROR, 2, "SPELLVISUALIDNOTFOUND|%d", spellRec->m_spellVisualID);
    return;
  }

  if (!startNow) {
    SpellVisualKitRec *castRec = g_spellVisualKitDB.GetRecord(spellVisualRec->m_castKit);
    if (castRec && Object_C_AnimHasHitEvent(castRec->m_anim)) {
      return;
    }
  }

  SpellVisualKitRec *impactRec = g_spellVisualKitDB.GetRecord(spellVisualRec->m_impactKit);
  if (impactRec) {
    SetImpactKitEffect(spellID, this, impactRec, 1);
  }

  SpellVisualKitRec *stateRec = g_spellVisualKitDB.GetRecord(spellVisualRec->m_stateKit);
  if (!stateRec) {
    return;
  }

  ACTIVEAURAINFO *active = new (s_auraInfoFreeList.GetData(0, typeid(ACTIVEAURAINFO).raw_name(), -2)) ACTIVEAURAINFO;
  m_activeAuraInfo.LinkNode(active, LIST_HEAD, 0);
  active->slot = slot;
  active->stateKitRec = stateRec;

  NTempest::C3Vector pos;
  GetPosition(pos);
  SndInterfacePlaySound(stateRec->m_soundID, pos, -1, 1.0f);
  AddKitAuras(stateRec, spellRec);
  AddSpellProcAuraEffect(slot, stateRec);

  unsigned int *self = reinterpret_cast<unsigned int *>(this);
  bool          higherPriority = self[393] == -1;
  if (!higherPriority) {
    SpellRec *current = g_spellDB.GetRecord(m_unit->auras[self[393]]);
    higherPriority = !current || spellRec->m_spellPriority > current->m_spellPriority;
  }
  if ((self[393] == -1 || higherPriority) && stateRec->m_anim > 0) {
    self[391] = stateRec->m_anim;
    self[393] = slot;
    SetTorsoAnimation(63, 0, 0);
  }
}

int __fastcall OnFirstAuraSequenceFinished(void *param) {
  ModelSetRandomSequenceFidget(reinterpret_cast<HMODEL>(param), 1, 0);
  return 1;
}

void CGUnit_C::MaybeAttachAura(UNITEFFECTATTACHPPOINT attach, unsigned int effect, unsigned int spellID, int priority, unsigned int permanent) {
  AuraVisual &visual = m_auraVisual[attach];
  if (visual.HasArt() && visual.GetSpellID() == spellID) {
    return;
  }
  if (visual.HasArt()) {
    SpellRec *currentSpell = g_spellDB.GetRecord(visual.GetSpellID());
    if (currentSpell && currentSpell->m_spellPriority > priority) {
      return;
    }
  }

  RemoveAuraVisual(attach);
  unsigned int isWorldObj;
  if (!UnitEffectIsAuraWorldObject(effect, isWorldObj)) {
    return;
  }

  if (isWorldObj) {
    NTempest::C3Vector location = GetPosition();
    unsigned long      object = UnitEffectCreateWorldModelAura(effect, location, GetFacing());
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

void CGUnit_C::SetAuraMirrorHandler(unsigned int slot, int(__fastcall *handler)(unsigned __int64, unsigned int, unsigned int, const void *, void *)) {
  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 4 * slot + 200, 4, handler, 0, HANDLER_PRIORITY_HIGH);
}

void CGUnit_C::UnsetAuraMirrorHandler(
    unsigned int slot,
    int(__fastcall *handler)(unsigned __int64, unsigned int, unsigned int, const void *, void *)
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
  unsigned long currentTime = GetTickCount();
  unsigned int  newFlags = m_unit->flags;
  unsigned int  xorBits = oldFlags ^ newFlags;

  if (xorBits & 0x02000000) {
    UpdateBaseAnimation(GetAnimationState(), 0x100);
  }

  if (xorBits & 0x400) {
    if (oldFlags & 0x400) {
      SetTorsoAnimation(44, 0, 0);
    } else if (m_currentBaseAnimState == 44) {
      SetTorsoAnimation(45, 0, 0);
    }
  }

  if (xorBits & 0x200000) {
    SetSheatheReason(SHEATHEREASON_2, (newFlags & 0x200000) != 0, 0);
  }

  if (xorBits & 0x4000) {
    CGGameUI::UnitNameUpdate(GetGUID());
  }

  if (xorBits & 0x18000) {
    UpdatePlayerNameColor();
  }

  if (xorBits & 0x80000) {
    FrameScript_SignalEvent((oldFlags & 0x80000) ? 186 : 187);
  }

  if (xorBits & 0x800) {
    FrameScript_SignalEvent((oldFlags & 0x800) ? 312 : 313);
  }

  if ((xorBits & 0x100000) && GetGUID() == ClntObjMgrGetActivePlayer() && (newFlags & 0x100000)) {
    SignalDisplayHealthUpdate();
  }

  (void)currentTime;
}

void CGUnit_C::HandleAnimEvent(const char *eventName, const NTempest::C3Vector &pos) {
  FATALASSERT(eventName);
  unsigned long code;
  memcpy(&code, eventName, sizeof(code));

  switch (code) {
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
      HandleFootfallAnimEvent(pos);
      return;

    case 0x48544224:  // $BTH
      SetRangedWeaponPullAnim(0);
      return;

    case 0x47475724:  // $WGG
      SetTorsoAnimation(13, 0, 0);
      return;

    case 0x474E5724:  // $WNG
      SetTorsoAnimation(10, 0, 0);
      return;

    case 0x4C485324:  // $SHL
    case 0x52485324:  // $SHR
      SetRangedWeaponReleaseAnim();
      return;

    case 0x4C534324:  // $CSL
    case 0x50574224:  // $BWP
    case 0x52534324:  // $CSR
    case 0x54534324:  // $CST
      if (m_currentBaseAnimState == 38) {
        HandleCastAnimEvent();
      }
      CheckPendingMissileRelease(&pos);
      return;
  }

  SysMsgPrintf(SYSMSG_WARNING, 16, "OBSOLETEANIMEVENT|%s", eventName);
}

const char *CGUnit_C::GetModelFileName() const {
  CreatureDisplayInfoRec *displayInfo = g_creatureDisplayInfoDB.GetRecord(m_unit->displayID);
  if (!displayInfo) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "NOCREATUREDISPLAYIDFOUND|%d", m_unit->displayID);
    return "NoName";
  }

  CreatureModelDataRec *modelData = g_creatureModelDataDB.GetRecord(displayInfo->m_modelID);
  if (!modelData) {
    SysMsgPrintf(SYSMSG_WARNING, 0x10, "INVALIDDISPLAYMODELRECORD|%d|%d", displayInfo->m_modelID, displayInfo->m_ID);
    return "NoName";
  }

  return modelData->m_ModelName;
}

const char *CGUnit_C::GetObjectName() const {
  return GetUnitName();
}

int CGUnit_C::CanBeLooted(unsigned long currentTime) const {
  const unsigned int *self = reinterpret_cast<const unsigned int *>(this);
  return m_unit->health <= 0 && (self[314] & 0x2000) && static_cast<int>(currentTime - self[435]) >= 0 && (m_unit->dynamicFlags & 1);
}

void CGUnit_C::OnDynamicFlagsChanged(unsigned int oldValue) {
  unsigned int newValue = m_unit->dynamicFlags;
  if (((oldValue ^ newValue) & 1) == 0) {
    return;
  }

  unsigned int effect = UnitEffectGetSpecialVisual(static_cast<UNITEFFECTSPECIALS>(0));
  if (newValue & 1) {
    if (m_unit->health <= 0 && static_cast<int>(effect) >= 0) {
      MaybeAttachAura(UNITEFFECT_ATTACHBASE, effect, 0, 100, 1);
    }
    return;
  }

  const unsigned char *self = reinterpret_cast<const unsigned char *>(this);
  for (unsigned int attach = 0; attach < NUM_UNITEFFECT_ATTACHPOINTS; ++attach) {
    const unsigned int attachedEffect = *reinterpret_cast<const unsigned int *>(self + 0x390 + 16 * attach);
    if (attachedEffect == effect) {
      RemoveAuraVisual(static_cast<UNITEFFECTATTACHPPOINT>(attach));
    }
  }
}

static int __fastcall MoveHeartBeatHandler(const void *packetData, void *param) {
  CGUnit_C       *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(CGUnit_C::m_activeMover, __FILE__, __LINE__));
  CMovementStatus status;
  if (unit) {
    unit->m_movement.GetMoveStatus(&status);
  }
  if (unit && !(status.moveFlags & 0x10000000)) {
    unit->SendMovementUpdate(MSG_MOVE_HEARTBEAT);
    SysMsgAdd("MOVEMENT|Movement heartbeat", SYSMSG_INFO, 1);
  }

  s_moveHeartBeatTimer = ClientSetTimer(500, MoveHeartBeatHandler, 0);
  return 1;
}

HTEXCOMPONENT CGUnit_C::GetTexComponent() const {
  return reinterpret_cast<HTEXCOMPONENT const *>(this)[477];
}

void __fastcall CGUnit_C::SetActiveMover(const unsigned __int64 &guid) {
  if (m_activeMover) {
    CGUnit_C *mover = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(m_activeMover, __FILE__, __LINE__));
    if (mover) {
      CGInputControl::GetActive()->UpdatePlayer(GetTickCount());
    }
  }
  m_activeMover = guid;
}

void CGUnit_C::BuildMovementUpdate(NETMESSAGE messageId, CDataStore *msg) const {
  CMovementStatus status;
  m_movement.GetMoveStatus(&status);

  msg->Put(static_cast<unsigned int>(messageId));
  msg->Put(status.transport);
  msg->Put(status.transRelPosition.x);
  msg->Put(status.transRelPosition.y);
  msg->Put(status.transRelPosition.z);
  msg->Put(status.transRelFacing);
  msg->Put(status.worldPosition.x);
  msg->Put(status.worldPosition.y);
  msg->Put(status.worldPosition.z);
  msg->Put(status.worldFacing);
  msg->Put(status.pitch);
  msg->Put(status.moveFlags & 0xFAFF0BFF);
}

void CGUnit_C::SendMovementUpdate(NETMESSAGE messageId) {
  m_lastSentFacing = GetFacing();

  CMovementStatus status;
  m_movement.GetMoveStatus(&status);
  m_lastSentPitch = status.pitch;

  CDataStore msg;
  BuildMovementUpdate(messageId, &msg);
  msg.Finalize();
  ClientServices_Send(&msg);
}

float CGUnit_C::GetDisplayFacing() const {
  return m_movement.GetFacing(m_displayFacing);
}

float CGUnit_C::GetSmoothFacing() const {
  return m_movement.GetFacing(reinterpret_cast<const float *>(this)[429]);
}

void CGUnit_C::UpdateSmoothFacing() {
  float         facing = m_movement.m_facing;
  float        *selfFloat = reinterpret_cast<float *>(this);
  unsigned int *self = reinterpret_cast<unsigned int *>(this);

  CGObject_C *activeMover = ClntObjMgrObjectPtr(m_activeMover, __FILE__, __LINE__);
  if (this == activeMover) {
    unsigned int unitFlags = m_unit->flags;
    if ((unitFlags & 0x1000000) ||
        ((m_obj->m_type & TYPE_PLAYER) && !m_unit->charmedBy && ((unitFlags & 2) || !(unitFlags & 0xC00004)) && !(unitFlags & 1)))
    {
      selfFloat[429] = facing;
      CGInputControl *input = CGInputControl::GetActive();
      if ((m_movement.m_moveFlags & 0x30) || input->CameraCanTurnPlayer()) {
        self[314] |= 8;
      } else {
        self[314] &= ~8u;
      }
      if ((m_movement.m_moveFlags & 0x30) || input->IsMouseDragMoving()) {
        self[314] |= 0x10;
      } else {
        self[314] &= ~0x10u;
      }
      return;
    }
  }

  if (!(m_movement.m_moveFlags & 0xF) && !m_unit->shapeshiftForm && !m_unit->channelSpell) {
    if (self[313] & 1) {
      facing = selfFloat[434];
    } else if (!((m_unit->flags & 8) && (m_obj->m_type & TYPE_PLAYER))) {
      unsigned __int64 target = 0;
      if ((m_unit->flags & 0x20000) || !m_combat.IsAttacking()) {
        void **vtable = *reinterpret_cast<void ***>(this);
        typedef unsigned __int64 (CGUnit_C::*GetFacingTargetFn)();
        GetFacingTargetFn getFacingTarget;
        memcpy(&getFacingTarget, &vtable[176 / 4], sizeof(getFacingTarget));
        target = (this->*getFacingTarget)();
        if (!target && GetGUID() == CGGameUI::GetInteractTarget()) {
          target = ClntObjMgrGetActivePlayer();
        }
      } else {
        target = m_combat.IsAttacking();
      }

      CGObject_C *targetObject = ClntObjMgrObjectPtr(target, __FILE__, __LINE__);
      if (targetObject) {
        CGUnit_C *targetUnit = static_cast<CGUnit_C *>(targetObject);
        facing = CalculateFacingTo(m_movement.m_position, targetUnit->m_movement.m_position);
      }
    }
  }

  float deltaFacing = facing - selfFloat[429];
  if (fabs(deltaFacing) <= 0.01f) {
    selfFloat[429] = facing;
    memset(selfFloat + 430, 0, 4 * sizeof(float));
    return;
  }

  if (deltaFacing > 3.1415927f) {
    deltaFacing -= 6.2831855f;
  } else if (deltaFacing < -3.1415927f) {
    deltaFacing += 6.2831855f;
  }

  float *savedFacingDeltas = selfFloat + 430;
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

  selfFloat[429] += deltaFacing * 0.5f;
  if (selfFloat[429] >= 6.2831855f) {
    selfFloat[429] -= 6.2831855f;
  } else if (selfFloat[429] < 0.0f) {
    selfFloat[429] += 6.2831855f;
  }
}

void CGUnit_C::SetSmoothFacing(float facing) {
  float *self = reinterpret_cast<float *>(this);
  self[429] = facing;
  while (self[429] >= 6.2831855f) {
    self[429] -= 6.2831855f;
  }
  memset(self + 430, 0, 4 * sizeof(float));
}

static void __fastcall UpdateLocalPlayerFallState(int falling) {
  CGUnit_C::StopMoveHeartbeatTimer();
  if (falling) {
    s_moveHeartBeatTimer = ClientSetTimer(500, MoveHeartBeatHandler, 0);
  }
}

void CGUnit_C::OnTeleportLocalNoUpdate(unsigned long eventTime, const NTempest::C3Vector &position, float facing) {
  CMovementStatus status;
  m_movement.GetMoveStatus(&status);
  status.worldPosition = position;
  status.worldFacing = facing;
  m_movement.UpdateStatusLocal(eventTime, status);
  UpdateBaseAnimation(0);
}

void CGUnit_C::UpdateSwimmingStatus(unsigned long eventTime, int inWater, float depth) {
  if (m_movement.m_moveFlags & 0x800) {
    return;
  }

  if (inWater) {
    if (!(m_movement.m_moveFlags & 0x02000000) && depth > 0.0f) {
      if (GetObjectHeight() * 0.75f < depth) {
        m_movement.StartSwim(eventTime);
      }
      if (m_movement.m_moveFlags & 0x4000) {
        PlaySplashSound(GetPosition());
      }
    }
  } else if (m_movement.m_moveFlags & 0x02000000) {
    m_movement.StopSwim(eventTime);
  }
}

void CGUnit_C::SendRedirectionMessage() {
  if (m_movement.m_lastReDirectionSent.SquaredMag() < 0.00000023841858f ||
      NTempest::C3Vector::Dot(m_movement.m_reDirection, m_movement.m_lastReDirectionSent) <= 0.99984771f)
  {
    CDataStore msg;
    BuildMovementUpdate(MSG_MOVE_COLLIDE_REDIRECT, &msg);
    msg.Put(m_movement.m_reDirection.x);
    msg.Put(m_movement.m_reDirection.y);
    msg.Put(m_movement.m_reDirection.z);
    msg.Finalize();
    ClientServices_Send(&msg);
    m_movement.m_lastReDirectionSent = m_movement.m_reDirection;
  }
}

void __fastcall CGUnit_C::StopMoveHeartbeatTimer() {
  if (s_moveHeartBeatTimer) {
    ClientKillTimer(s_moveHeartBeatTimer, MoveHeartBeatHandler, "MoveHeartBeatHandler");
  }
  s_moveHeartBeatTimer = 0;
}

void __fastcall CGUnit_C::StartMoveHeartbeatTimer() {
  StopMoveHeartbeatTimer();
  s_moveHeartBeatTimer = ClientSetTimer(500, MoveHeartBeatHandler, 0);
}

unsigned int __fastcall CGUnit_C::OffsetOf(OBJECT_TYPE_ID type) {
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
    // TODO: implement
}

static unsigned char ShowBreathCallback(CVar* h, const char* oldValue, const char* newValue, void* arg) {
    // TODO: implement
    return 0;
}

void __fastcall CGUnit_C::Initialize() {
  UnitEffectsInitialize();
}

void __fastcall CGUnit_C::PostShutdown() {
  UnitEffectsShutdown();
}

int __fastcall ViolenceGetLevel();

void __fastcall CGUnit_C::NamePlateShow(int show) {
  s_drawNameplates = show;
  if (!show) {
    RemoveAllNamePlates();
  }
}

int CGUnit_C::GetCreatureType() {
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

int CGUnit_C::IsUnderWater() const {
  float              surface = 0.0f;
  unsigned int       liquid = 0;
  NTempest::C3Vector waterDir;
  int                deep = 15;
  float              depth = 0.0f;
  if (CWorld::QueryObjectLiquid(GetWorldObject(), liquid, surface, waterDir, deep)) {
    NTempest::C3Vector position = GetPosition();
    depth = surface - position.z;
  }
  return depth > reinterpret_cast<const float *>(this)[60] * 0.5f;
}

unsigned int CGUnit_C::ChooseAnimation(unsigned int state) const {
  FATALASSERT(state != 0);
  const unsigned int *self = reinterpret_cast<const unsigned int *>(this);

  switch (state) {
    case 1:
      if (!(m_animFlags & 0x400)) {
        return 1;
      }
      return IsUnderWater() ? 131 : 1;
    case 2:
      return 2;
    case 4:
      return GetStopSequence();
    case 5:
    case 8:
    case 9:
    case 12:
    case 13:
      return GetWalkStateAnim();
    case 6:
    case 10:
    case 11:
    case 14:
    case 15:
      return GetRunSequence();
    case 7:
    case 16:
    case 17:
      return (m_animFlags & 0x400) ? 13 : GetWalkStateAnim();
    case 18:
      return 11;
    case 19:
      return 12;
    case 20:
      return 41;
    case 21:
      return 42;
    case 22:
      return (m_animFlags & 0x100) ? 43 : 41;
    case 23:
      return (m_animFlags & 0x80) ? 44 : 41;
    case 24:
      return (m_animFlags & 0x200) ? 45 : 41;
    case 26:
      return 7;
    case 27: {
      typedef unsigned int (CGUnit_C::*GetStandSequenceProc)() const;
      GetStandSequenceProc getStandSequence;
      void               **vtable = *reinterpret_cast<void ***>(const_cast<CGUnit_C *>(this));
      memcpy(&getStandSequence, &vtable[0x120 / sizeof(void *)], sizeof(getStandSequence));
      return (this->*getStandSequence)();
    }
    case 28:
      return 10;
    case 29:
      return 14;
    case 30:
    case 32:
      return DetermineAttackerSequence(static_cast<COMBATHAND>(0));
    case 31: {
      FATALASSERT(m_readySequence != 0xFFFFFFFF);
      unsigned int sequence = m_readySequence;
      if (ModelHasSequenceId(reinterpret_cast<HMODEL *>(const_cast<CGUnit_C *>(this))[4], sequence)) {
        return sequence;
      }

      typedef const char *(CGUnit_C::*GetObjectTypeNameProc)() const;
      GetObjectTypeNameProc getObjectTypeName;
      void                **vtable = *reinterpret_cast<void ***>(const_cast<CGUnit_C *>(this));
      memcpy(&getObjectTypeName, &vtable[0x28 / sizeof(void *)], sizeof(getObjectTypeName));
      ReportMissingAnimation(sequence, (this->*getObjectTypeName)());
      return GetStandStateAnim(0);
    }
    case 33:
    case 34:
      return DetermineAttackerSequence(static_cast<COMBATHAND>(1));
    case 35:
      return DetermineParrySequence();
    case 36:
      return 30;
    case 37:
      return self[390];
    case 38:
    case 62:
    case 63:
      return self[391];
    case 40:
      return 24;
    case 41:
      return 37;
    case 42:
      return 39;
    case 43:
      return 40;
    case 44:
    case 45:
      return 50;
    case 46:
      return GetEmoteAnimation(self[394]);
    case 47:
      return self[277];
    case 48:
      return 91;
    case 49:
      return 94;
    case 50:
      return 96;
    case 51:
      return 97;
    case 52:
      return 98;
    case 53:
      return 99;
    case 54:
      return 100;
    case 55:
      return 101;
    case 56:
      return 102;
    case 57:
      return 103;
    case 58:
      return 104;
    case 59:
      return 114;
    case 60:
      return 115;
    case 61:
      return 116;
    default:
      return GetStandStateAnim(0);
  }
}

void CGUnit_C::OnBadAttackFacing(unsigned __int64 victimGUID) {
  reinterpret_cast<unsigned int *>(this)[388] |= 2;
}

int __fastcall GetForcedAnimIndex(const char *token) {
  for (unsigned int i = 0; i < 8; ++i) {
    if (!SStrCmp(s_forcedAnimations[i].name, token, 0x7FFFFFFF)) {
      return i;
    }
  }
  return -1;
}

int __fastcall ParseForcedAnimCommandLine(const char *arguments, unsigned int *fidgetIndex) {
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

  HMODEL model = reinterpret_cast<HMODEL *>(this)[4];
  FATALASSERT(model);
  ANIMENUMERATION anim = s_forcedAnimations[animIndex].anim;
  if (!ModelGetNumSequenceFidgets(model, anim)) {
    return;
  }

  unsigned int &flags = reinterpret_cast<unsigned int *>(this)[314];
  flags |= 1;
  if (s_forcedAnimations[animIndex].flag) {
    flags |= 2;
  } else {
    flags &= ~2u;
  }
  ModelSetSequenceFidget(model, anim, animVariation, 0);

  switch (animIndex) {
    case 1:
      PlayUnitSound(static_cast<UNITSOUNDTYPE>(4), 0);
      break;
    case 4:
    case 6:
      PlayUnitSound(static_cast<UNITSOUNDTYPE>(s_forcedAnimations[animIndex].flag != 0), 1);
      break;
    case 5:
    case 7:
      PlayUnitSound(static_cast<UNITSOUNDTYPE>((s_forcedAnimations[animIndex].flag != 0) + 2), 1);
      break;
  }
}

void CGUnit_C::ResetForcedAnimation() {
  reinterpret_cast<unsigned int *>(this)[314] &= ~3u;
  UpdateBaseAnimation(GetAnimationState(), 0x100);
}

void CGUnit_C::ForceUpdateBaseAnimation() {
  UpdateBaseAnimation(GetAnimationState(), 0x100);
}

void CGUnit_C::OnBadAttackPosition(unsigned __int64 victimGUID, float range) {
}

void __fastcall ClearSpecialEffects(HMODEL model) {
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
  int castingSpell = *reinterpret_cast<const int *>(reinterpret_cast<const unsigned char *>(this) + 0x698);
  if (castingSpell && castingSpell == spellID) {
    unsigned int timer = *reinterpret_cast<const unsigned int *>(reinterpret_cast<const unsigned char *>(this) + 0x6FC);
    ClientKillTimer(timer, SpellFizzleTimer, "SpellFizzleTimer");
    EndSpellEffects(status);
  }
}

void CGUnit_C::EndSpellEffects(unsigned char status) {
  int castingSpell = *reinterpret_cast<const int *>(reinterpret_cast<const unsigned char *>(this) + 0x698);
  *reinterpret_cast<unsigned int *>(reinterpret_cast<unsigned char *>(this) + 0x6FC) = 0;

  if (castingSpell) {
    UnitEffectClearSpellPrecast(this, castingSpell);
    SetCastingSpell(0, true, false);
  }

  KillSpellLoopedSound();
  unsigned int torsoState = *reinterpret_cast<const unsigned int *>(reinterpret_cast<const unsigned char *>(this) + 0x6E8);
  if (torsoState == 37) {
    unsigned int torsoAnim = GetCurrentTorsoAnim();
    if (torsoAnim == 46 || torsoAnim == 49) {
      RangedWeaponAnimEndHandler();
    } else if (torsoAnim == 107) {
      ThrowAnimEndHandler();
    } else {
      SpellVisualKitRec *kit = GetRangedSpellAnim(castingSpell, true);
      if (status || !kit || !kit->m_anim) {
        ClearTorsoAnimation(0x40);
      }
    }
  }

  FATALASSERT(!*reinterpret_cast<const unsigned int *>(reinterpret_cast<const unsigned char *>(this) + 0x6FC));
}

NTempest::C3Vector CGUnit_C::GetPosition() const {
  CMovementStatus status;
  m_movement.GetMoveStatus(&status);
  return status.worldPosition;
}

void CGUnit_C::GetPosition(NTempest::C3Vector &vec) const {
  vec = GetPosition();
}

float CGUnit_C::GetFacing() const {
  CMovementStatus status;
  m_movement.GetMoveStatus(&status);
  return status.worldFacing;
}

void CGUnit_C::AddDeathHold() {
  ++reinterpret_cast<unsigned int *>(this)[448];
}

NTempest::C3Vector CGUnit_C::GetGroundNormal() const {
  return NTempest::C3Vector(0.0f, 0.0f, 1.0f);
}

void CGUnit_C::DelDeathHold() {
  unsigned int *self = reinterpret_cast<unsigned int *>(this);
  if (!self[448]) {
    return;
  }

  self[493] = m_unit->health;
  SignalDisplayHealthUpdate();
  --self[448];
  if (!m_unit->health) {
    self[289] = GetTickCount();
  }

  if (!self[448]) {
    m_deathHoldBufferIndices.SetCount(0);
    m_deathHoldBuffer.SetCount(0);
    if (m_unit->health <= 0) {
      if (!(self[314] & 0x2000)) {
        Reenable();
        CGGameUI::ClearTarget(GetGUID(), 1);
      }
      if (m_unit->dynamicFlags & 1) {
        int effect = UnitEffectGetSpecialVisual(static_cast<UNITEFFECTSPECIALS>(0));
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
        static_cast<GAME_ERROR_TYPE>(224), stats->m_name[FrameScript_GetPluralIndex(m_questCountNeeded)], m_questCountKilled, m_questCountNeeded
    );
  } else {
    OsOutputDebugString("Error\n");
  }
  m_questCountNeeded = -1;
}

static bool __fastcall IsCombatSwingSpell(int spellID) {
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

void CGUnit_C::ProcessLocalMoveEvent(NETMESSAGE msgId) {
  if (!(m_movement.m_moveFlags & 0xF) && (msgId == MSG_MOVE_STOP || msgId == MSG_MOVE_STOP_STRAFE)) {
    UpdateBaseAnimation(4, 0);
  } else if (msgId == MSG_MOVE_SET_FACING) {
    if (fabs(GetFacing() - m_lastSentFacing) < 0.1f) {
      return;
    }
  } else if (msgId != MSG_MOVE_SET_PITCH) {
    UpdateBaseAnimation(0);
  }

  CMovementStatus status;
  m_movement.GetMoveStatus(&status);
  if (msgId != MSG_MOVE_SET_PITCH || fabs(status.pitch - m_lastSentPitch) >= 0.1f) {
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
      if (!(m_movement.m_moveFlags & 0x40FF)) {
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
  if (!(m_movement.m_moveFlags & 0xF) && (msgId == MSG_MOVE_STOP || msgId == MSG_MOVE_STOP_STRAFE)) {
    UpdateBaseAnimation(4, 0);
  } else {
    UpdateBaseAnimation(0);
  }

  if (GetGUID() == m_activeMover) {
    unsigned int moveFlags = m_movement.m_moveFlags;
    if ((moveFlags & 0x01000000) ||
        ((GetType() & TYPE_PLAYER) && !m_movement.m_transportGUID && ((moveFlags & 2) || !(moveFlags & 0x00C00004)) && !(moveFlags & 1)))
    {
      SendMovementUpdate(msgId);
    }
  }
}

void CGUnit_C::OnCollideFalling(unsigned long eventTime) {
  UpdateBaseAnimation(0);
  if (GetGUID() == m_activeMover && m_movement.m_jumpVelocity == 0.0f) {
    UpdateLocalPlayerFallState(1);
  }
}

void CGUnit_C::OnCollideFallLand(unsigned long eventTime) {
  if (m_movement.m_moveFlags & 0xF) {
    UpdateBaseAnimation(0);
  } else if (!IsInStandSitTransition()) {
    UpdateBaseAnimation(42, 0);
  }

  PlayUnitSound(UNITSOUND_JUMP_END, 1);

  if (GetGUID() == m_activeMover && !(m_movement.m_moveFlags & 0x40FF)) {
    UpdateLocalPlayerFallState(0);
    SendMovementUpdate(MSG_MOVE_HEARTBEAT);
  }

  if (GetGUID() == ClntObjMgrGetActivePlayer() && (m_flags & 0x2000)) {
    CGGameUI::UpdateActivePlayer();
  }
}

unsigned int CGUnit_C::IsSlotComponented(unsigned int offset, int ignoreUsingRangedWeapon) {
  if (offset >= 23) {
    return 0;
  }
  if (ignoreUsingRangedWeapon || offset == 15 || offset == 16) {
    return 1;
  }
  if (offset != 17) {
    return 1;
  }
  return m_unit->weaponMode == WEAPONMODE_RANGED;
}

float CGUnit_C::DetermineWalkRunTimeScale(int currentState) {
  if (!(reinterpret_cast<unsigned int *>(this)[32] & 0xF)) {
    return 1.0f;
  }

  float  movementSpeed = m_movement.GetCurrentSpeed();
  HMODEL model = reinterpret_cast<HMODEL *>(this)[4];
  float  animMovementSpeed = 0.0f;
  ModelGetSequenceMoveSpeed(model, ChooseAnimation(currentState), &animMovementSpeed);
  if (fabs(animMovementSpeed) < 0.00000023841858f) {
    return 0.0f;
  }

  float modelScale = GetScale();
  FATALASSERT(modelScale > 0.0f);
  return movementSpeed / (fabs(animMovementSpeed) * modelScale);
}

void CGUnit_C::UpdateMovementAnimSpeed(int forMount, int currentState) {
  unsigned int *self = reinterpret_cast<unsigned int *>(this);
  if (currentState == -1) {
    currentState = self[440];
  }
  if ((m_unit->flags & 0x2000) && forMount) {
    currentState = self[444];
  }

  float scale = 1.0f;
  if (s_animInfo[currentState].flags & 0x100) {
    scale = fabs(DetermineWalkRunTimeScale(currentState));
  }
  unsigned int animFlags = s_animInfo[currentState].flags;
  if ((animFlags & 0x40) || ((animFlags & 0x800) && !(m_animFlags & 4))) {
    scale = -scale;
  }
  ModelSetTimeScale(reinterpret_cast<HMODEL *>(this)[4], scale, 0);
}

void CGUnit_C::AttachVirtualComponent(unsigned int slot, bool deferApply) {
  FATALASSERT(!(m_obj->m_type & TYPE_PLAYER));
  FATALASSERT(slot < 3);

  int          invSlot;
  unsigned int showHidden = 0;
  if (slot == 0) {
    invSlot = 15;
    showHidden = (m_unit->flags & 0x200000) != 0;
  } else if (slot == 1) {
    invSlot = 16;
  } else {
    invSlot = 17;
    showHidden = m_unit->weaponMode != WEAPONMODE_RANGED;
  }

  void **vtable = *reinterpret_cast<void ***>(this);
  typedef const VirtualItemInfo *(CGUnit_C::*GetVirtualItemInfoFn)(unsigned int, unsigned int);
  GetVirtualItemInfoFn getVirtualItemInfo;
  memcpy(&getVirtualItemInfo, &vtable[296 / 4], sizeof(getVirtualItemInfo));
  const VirtualItemInfo *itemInfo = (this->*getVirtualItemInfo)(slot, 0);
  if (!itemInfo) {
    return;
  }

  const ItemSubClassRec *subClass = SDBItemSubclassGetSubClassRec(itemInfo->m_classID, itemInfo->m_subclassID);
  bool                   forceAlternate = subClass && (subClass->m_flags & 0x20) != 0;
  int                    sheathedAttachmentPoint = SheatheTypeToSheathePoint(itemInfo->m_sheatheType, invSlot);

  typedef int (CGUnit_C::*GetVirtualItemDisplayFn)(unsigned int);
  GetVirtualItemDisplayFn getVirtualItemDisplay;
  memcpy(&getVirtualItemDisplay, &vtable[300 / 4], sizeof(getVirtualItemDisplay));
  int displayID = (this->*getVirtualItemDisplay)(slot);

  AddObjectComponentBySlot(
      invSlot, displayID, itemInfo->m_inventoryType, forceAlternate, deferApply, false, sheathedAttachmentPoint, showHidden != 0
  );
}

void CGUnit_C::AttachVirtualMonsterWeapons() {
  if (m_obj->m_type & TYPE_PLAYER) {
    return;
  }

  for (unsigned int slot = 0; slot < 3; ++slot) {
    if (m_unit->virtualItemDisplay[slot]) {
      AttachVirtualComponent(slot, slot == 2);
    }
  }

  SetSheatheReason(static_cast<SHEATHEREASONS>(0), m_unit->weaponMode == WEAPONMODE_MELEE, 1);
  if (m_unit->virtualItemDisplay[2]) {
    SetSheatheReason(static_cast<SHEATHEREASONS>(5), m_unit->weaponMode == WEAPONMODE_RANGED && reinterpret_cast<unsigned int *>(this)[617], 1);
  }
}

void CGUnit_C::RenderDebugPathing() {
  static NTempest::CImVector green(0xFF00FF00);
  static NTempest::CImVector white(0xFFFFFFFF);

  TSGrowableArray<NTempest::C3Vector> &points =
      *reinterpret_cast<TSGrowableArray<NTempest::C3Vector> *>(reinterpret_cast<unsigned char *>(this) + 0x714);
  unsigned int &numPathNodes = *reinterpret_cast<unsigned int *>(reinterpret_cast<unsigned char *>(this) + 0x728);

  GxRsPush();
  if (m_modelData && !(m_modelData->m_flags & 4) && (m_modelData->m_flags & 0x200)) {
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
    const NTempest::C3Vector &serverLoc = *reinterpret_cast<const NTempest::C3Vector *>(reinterpret_cast<const unsigned char *>(this) + 0x708);
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

bool __fastcall CGUnit_C::FactionHasReputation(int faction) {
  FactionRec *rec = g_factionDB.GetRecord(faction);
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

  unsigned int flags = m_unit->flags;
  unsigned int unitFlags = unit->m_unit->flags;
  if (!(flags & 8)) {
    return true;
  }

  if (!(unitFlags & 8)) {
    const MapRec *map = g_mapDB.GetRecord(CGPlayer_C::GetNewContinentID());
    return map && map->m_PVP != 0;
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

  if (player1 && (player1->GetType() & TYPE_PLAYER) && player2 && (player2->GetType() & TYPE_PLAYER)) {
    const unsigned char *playerData1 = *reinterpret_cast<const unsigned char *const *>(reinterpret_cast<const unsigned char *>(player1) + 2528);
    const unsigned char *playerData2 = *reinterpret_cast<const unsigned char *const *>(reinterpret_cast<const unsigned char *>(player2) + 2528);
    unsigned int         duelTeam1 = *reinterpret_cast<const unsigned int *>(playerData1 + 1776);
    unsigned int         duelTeam2 = *reinterpret_cast<const unsigned int *>(playerData2 + 1776);
    if (duelTeam1 || duelTeam2) {
      return *reinterpret_cast<const unsigned __int64 *>(playerData1 + 568) == *reinterpret_cast<const unsigned __int64 *>(playerData2 + 568) &&
             duelTeam1 == duelTeam2;
    }
  }

  return UnitReaction(GetFactionTemplate(), unit, -1) >= UNIT_REACTION_NEUTRAL;
}

bool CGUnit_C::CanAttack(const CGUnit_C *unit) const {
  unsigned int flags = m_unit->flags;
  unsigned int unitFlags = unit->m_unit->flags;
  if (((flags & 8) && (unitFlags & 0x100)) || (!(flags & 8) && (unitFlags & 0x200)) || ((flags & 0x100) && (unitFlags & 8)) ||
      ((flags & 0x200) && !(unitFlags & 8)))
  {
    return false;
  }

  if (!(flags & 8) || !(unitFlags & 8)) {
    return UnitReaction(unit) < UNIT_REACTION_AMIABLE || (!(flags & 8) && unit->UnitReaction(this) < UNIT_REACTION_AMIABLE);
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

  if (player1 && (player1->GetType() & TYPE_PLAYER) && player2 && (player2->GetType() & TYPE_PLAYER)) {
    const unsigned char *playerData1 = *reinterpret_cast<const unsigned char *const *>(reinterpret_cast<const unsigned char *>(player1) + 2528);
    const unsigned char *playerData2 = *reinterpret_cast<const unsigned char *const *>(reinterpret_cast<const unsigned char *>(player2) + 2528);
    unsigned int         duelTeam1 = *reinterpret_cast<const unsigned int *>(playerData1 + 1776);
    unsigned int         duelTeam2 = *reinterpret_cast<const unsigned int *>(playerData2 + 1776);

    if (duelTeam1 || duelTeam2) {
      if (*reinterpret_cast<const unsigned __int64 *>(playerData1 + 568) != *reinterpret_cast<const unsigned __int64 *>(playerData2 + 568)) {
        return false;
      }
      return duelTeam1 != duelTeam2;
    }
  }

  return UnitReaction(unit) < UNIT_REACTION_AMIABLE || (!(flags & 8) && unit->UnitReaction(this) < UNIT_REACTION_AMIABLE);
}

bool CGUnit_C::CanCooperate(const CGUnit_C *unit) const {
  if (unit == this) {
    return false;
  }

  const FactionTemplateRec *faction = g_factionTemplateDB.GetRecord(GetFactionTemplate());
  const FactionTemplateRec *unitFaction = g_factionTemplateDB.GetRecord(unit->GetFactionTemplate());
  return faction && unitFaction && faction->m_factionGroup == unitFaction->m_factionGroup;
}

unsigned int CGUnit_C::IsUnitInGroup(CGUnit_C *unit) {
  if (unit == this) {
    return 1;
  }

  if ((m_unit->flags & 8) && (unit->m_unit->flags & 8)) {
    CGUnit_C               *player1 = this;
    const unsigned __int64 &owner1 = m_unit->charmedBy ? m_unit->charmedBy : m_unit->createdBy;
    if (owner1) {
      player1 = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(owner1, __FILE__, __LINE__));
    }

    CGUnit_C               *player2 = unit;
    const unsigned __int64 &owner2 = unit->m_unit->charmedBy ? unit->m_unit->charmedBy : unit->m_unit->createdBy;
    if (owner2) {
      player2 = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(owner2, __FILE__, __LINE__));
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
  PlayerNameTriggerColorUpdate(reinterpret_cast<HPLAYERNAME__ **>(this)[420]);
}

static int __fastcall IsInSitSleepPosition(unsigned int animState) {
  return animState < 64 ? (s_animInfo[animState].statePreempts >> 19) & 1 : 0;
}

void CGUnit_C::AddWorldDamageText(unsigned int damage, int normalCombatDamage) {
  if (damage) {
    char buffer[32];
    SStrPrintf(buffer, sizeof(buffer), "%d", damage);
    HPLAYERNAME__ *name = *reinterpret_cast<HPLAYERNAME__ **>(reinterpret_cast<unsigned char *>(this) + 0x690);
    PlayerNameCreateText(name, WT_DAMAGE, buffer, normalCombatDamage ? 0 : &COLOR_GOLD);
  }
}

void CGUnit_C::AddWorldCritText(unsigned int damage, int normalCombatDamage) {
  if (damage) {
    char buffer[32];
    SStrPrintf(buffer, sizeof(buffer), "%d", damage);
    HPLAYERNAME__ *name = *reinterpret_cast<HPLAYERNAME__ **>(reinterpret_cast<unsigned char *>(this) + 0x690);
    PlayerNameCreateText(name, WT_CRIT, buffer, normalCombatDamage ? 0 : &COLOR_GOLD);
  }
}

void CGUnit_C::AddWorldXPGainText(int xpGain) {
  char buf[64];
  char buffer[64] = "";
  SStrCopy(buf, FrameScript_GetText("XP", -1, GENDER_NOT_APPLICABLE), sizeof(buf));
  SStrPrintf(buffer, sizeof(buffer), "%s: %d", buf, xpGain);
  HPLAYERNAME__ *name = *reinterpret_cast<HPLAYERNAME__ **>(reinterpret_cast<unsigned char *>(this) + 0x690);
  PlayerNameCreateText(name, WT_XPGAIN, buffer, 0);
}

void CGUnit_C::RemoveInteractIcon() {
  if (m_interactIconModel) {
    HMODEL charModel = GetCharacterModel(0);
    if (charModel) {
      ModelRemoveLink(charModel, 18, m_interactIconModel);
      ModelRemoveLink(charModel, 29, m_interactIconModel);
      HandleClose(charModel);
    }
    HandleClose(m_interactIconModel);
    m_interactIconModel = 0;
  }
}

HMODEL CGUnit_C::DuplicateCharacterModel(unsigned int flags) {
  HMODEL characterModel = GetCharacterModel(0);
  HMODEL duplicate = ModelDuplicate(characterModel, flags);
  HandleClose(characterModel);
  return duplicate;
}

UNIT_REACTION __fastcall CGUnit_C::UnitReaction(int factionID, const CGUnit_C *unit, int trueSight) {
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
    if (ownerUnit && ownerUnit->GetGUID() == ClntObjMgrGetActivePlayer()) {
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

  if (player1 && (player1->GetType() & TYPE_PLAYER) && player2 && (player2->GetType() & TYPE_PLAYER)) {
    const unsigned char *playerData1 = *reinterpret_cast<const unsigned char *const *>(reinterpret_cast<const unsigned char *>(player1) + 2528);
    const unsigned char *playerData2 = *reinterpret_cast<const unsigned char *const *>(reinterpret_cast<const unsigned char *>(player2) + 2528);
    unsigned int         duelTeam1 = *reinterpret_cast<const unsigned int *>(playerData1 + 1776);
    unsigned int         duelTeam2 = *reinterpret_cast<const unsigned int *>(playerData2 + 1776);

    if (duelTeam1 || duelTeam2) {
      if (*reinterpret_cast<const unsigned __int64 *>(playerData1 + 568) == *reinterpret_cast<const unsigned __int64 *>(playerData2 + 568)) {
        return duelTeam1 == duelTeam2 ? UNIT_REACTION_FRIENDLY : UNIT_REACTION_HOSTILE;
      }
      return UNIT_REACTION_HATED;
    }

    if (player1->GetGUID() == ClntObjMgrGetActivePlayer() && CGGameUI::IsPartyMember(player2->GetGUID())) {
      return UNIT_REACTION_FRIENDLY;
    }
    if (player2->GetGUID() == ClntObjMgrGetActivePlayer() && CGGameUI::IsPartyMember(player1->GetGUID())) {
      return UNIT_REACTION_FRIENDLY;
    }

    if ((*reinterpret_cast<const unsigned int *>(playerData1 + 1368) & 1) && (*reinterpret_cast<const unsigned int *>(playerData2 + 1368) & 1)) {
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
      const CGUnit_C *ownerUnit = this;
      if (owner1) {
        ownerUnit = static_cast<const CGUnit_C *>(ClntObjMgrObjectPtr(owner1, __FILE__, __LINE__));
      }
      if (ownerUnit && ownerUnit->GetGUID() == ClntObjMgrGetActivePlayer()) {
        return CGReputationInfo::IsAtWar(targetFaction->m_faction) ? UNIT_REACTION_HOSTILE : UNIT_REACTION_FRIENDLY;
      }
    }
  }

  return UnitReaction(GetFactionTemplate(), unit, -1);
}

HMODEL CGUnit_C::GetCharacterModel(int *mountedPtr) const {
  if (m_flags & 0x10) {
    if (mountedPtr) {
      *mountedPtr = 1;
    }
    FATALASSERT(m_tempCharModel);
    return static_cast<HMODEL>(HandleDuplicate(m_tempCharModel));
  }

  if (mountedPtr) {
    *mountedPtr = 0;
  }
  return static_cast<HMODEL>(HandleDuplicate(GetObjectModel()));
}

bool CGUnit_C::CanInteract(const CGUnit_C *unit) const {
  return unit->GetUnitData()->npcFlags && unit->UnitReaction(this) >= UNIT_REACTION_NEUTRAL && UnitReaction(unit) >= UNIT_REACTION_NEUTRAL;
}

HMODEL CGUnit_C::GetMountedModel() {
  return (m_flags & 0x10) ? GetCharacterModel(0) : 0;
}

unsigned int CGUnit_C::GetRunSequence() const {
  if (!(m_unit->flags & 0x2000)) {
    return 5;
  }
  unsigned int moveFlags = reinterpret_cast<const unsigned int *>(this)[32];
  if ((moveFlags & 0x33) != 0x33) {
    return 5;
  }
  if ((moveFlags & 0x10) && (m_animFlags & 0x40)) {
    return 93;
  }
  if (!(moveFlags & 0x20)) {
    return 5;
  }
  return (m_animFlags & 0x20) ? 92 : 5;
}

unsigned int CGUnit_C::GetStopSequence() const {
  HMODEL model = reinterpret_cast<HMODEL const *>(this)[4];
  if (ModelHasSequenceId(model, 3)) {
    return 3;
  }
  unsigned int sequence = GetStandStateAnim(0);
  return ModelHasSequenceId(model, sequence) ? sequence : 0;
}

void CGUnit_C::UpdateRenderFacing() {
  m_displayFacing = GetFacing();
}

float CGUnit_C::GetRenderFacing() const {
  return m_movement.GetFacing(m_displayFacing);
}

void CGUnit_C::PostAnimate(CGWorldFrame *worldFrame) {
  if (m_unit->weaponMode == WEAPONMODE_RANGED) {
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
  for (NAMEPLATEDESC *existing = s_namePlateList.Head(); existing; existing = s_namePlateList.Next(existing)) {
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

void __fastcall CGUnit_C::RemoveAllNamePlates() {
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

void __fastcall CGUnit_C::UpdateUnitNameplates(CGWorldFrame *worldFrame) {
  s_namePlateWorldFrame = worldFrame;
  if (!worldFrame) {
    while (NAMEPLATEDESC *desc = s_namePlateList.Head()) {
      s_monsterNamePlateList.Delete(desc);
    }
    s_monsterNamePlateList.Clear();
  }
}

void __fastcall CGUnit_C::ResortAllUnitNameplates(CGWorldFrame *worldFrame) {
  FATALASSERT(worldFrame);
  for (NAMEPLATEDESC *desc = s_namePlateList.Head(); desc;) {
    NAMEPLATEDESC *next = s_namePlateList.Next(desc);
    if (!desc->namePlate || !CalculateScreenSortOrder(worldFrame, desc)) {
      s_monsterNamePlateList.Delete(desc);
    } else {
      desc->namePlate->SetPoint(FRAMEPOINT_CENTER, worldFrame, FRAMEPOINT_BOTTOMLEFT, desc->screenCoords.x, desc->screenCoords.y, 1);
    }
    desc = next;
  }
}

unsigned int CGUnit_C::GetPlayerNameAttachmentPoint() {
  if (!(m_flags & 0x10)) {
    return 18;
  }
  return (m_flags & 0x1000) ? 18 : 29;
}

const char *CGUnit_C::GetUnitName() const {
  unsigned __int64 guid = m_obj->m_guid;
  if (m_obj->m_type & 0x10) {
    const NameCache *name = g_nameDBCache.GetRecord(guid, guid, NameQueryCallback, 0);
    if (name) {
      return name->m_name;
    }
  } else if (m_unit->petNumber) {
    const PetNameCache *name = g_petNameCache.GetRecord(m_unit->petNumber, guid, NameQueryCallback, 0);
    if (name) {
      if (name->m_timestamp == m_unit->petNameTimestamp) {
        return name->m_name;
      }

      g_petNameCache.Invalidate(m_unit->petNumber);
      g_petNameCache.GetRecord(m_unit->petNumber, guid, NameQueryCallback, 0);
    }
  }

  return m_stats ? m_stats->m_name[0] : "Unknown Being";
}

static unsigned char IsDayTime() {
    // TODO: implement
    return 0;
}

static unsigned char IsMountSpell(const SpellRec* rec) {
    // TODO: implement
    return 0;
}

void CGUnit_C::PendingPrecastInterrupt(int spellID) {
  int &interruptedSpell = *reinterpret_cast<int *>(reinterpret_cast<unsigned char *>(this) + 0x69C);
  if (interruptedSpell && !spellID) {
    SpellVisualsHandleCastStop(interruptedSpell, this, 2, 0);
  }
  interruptedSpell = spellID;
}

void CGUnit_C::StoreXPGain(int XP) {
  int *self = reinterpret_cast<int *>(this);
  self[421] += XP;
  unsigned int animFlags = self[314];
  self[313] |= 0x80;
  if (animFlags & 0x2000) {
    ShowPlayerXPGained();
  }
}

void CGUnit_C::UpdateInteractIcon(QUEST_GIVER_STATUS status) {
  FATALASSERT(status < QUEST_GIVER_NUMITEMS);
  unsigned int *self = reinterpret_cast<unsigned int *>(this);
  if (s_questIconInfo[status] != s_questIconInfo[self[449]]) {
    UpdateInteractIcon(s_questIconInfo[status]);
  }
  self[449] = status;
}

void CGUnit_C::UpdateInteractIcon(INTERACTICONTYPE which) {
  RemoveInteractIcon();
  if (which) {
    FATALASSERT(which < INTERACTICON_NUMITEMS);
    unsigned int index = which - 1;
    FATALASSERT(s_interactIconModelInfo[index].model);
    m_interactIconModel = ModelDuplicate(s_interactIconModelInfo[index].model, 0);
    if (m_interactIconModel) {
      typedef int (CGUnit_C::*ShouldRenderUnitNameProc)(unsigned int) const;
      ShouldRenderUnitNameProc shouldRenderUnitName;
      void                   **vtable = *reinterpret_cast<void ***>(this);
      memcpy(&shouldRenderUnitName, &vtable[0x130 / sizeof(void *)], sizeof(shouldRenderUnitName));
      int renderName = (this->*shouldRenderUnitName)(PlayerNameGetUnitNameMode());
      ModelSetSequence(m_interactIconModel, renderName != 0, 0);

      HMODEL charModel = GetCharacterModel(0);
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
  node->time = GetTickCount() + 500;

  float               y = NTempest::CRandom::reals_(g_rndSeed) / 6.0f;
  float               x = NTempest::CRandom::reals_(g_rndSeed) / 6.0f;
  NTempest::C44Matrix direction;
  NTempest::C3Vector  rot(0.0f, 0.0f, 1.0f);
  direction.Rotate(-GetDisplayFacing(), rot, 1);

  NTempest::C3Vector jitter(2.0f / 3.0f, 0.0f, 0.0f);
  if (linkPoint == BLOODSPURT_BACK) {
    jitter.x = -2.0f / 3.0f;
  }
  jitter = jitter * direction;

  GetPosition(rot);
  node->position = rot + NTempest::C3Vector(x, y, 0.0f) + jitter;
  m_bloodSplatNodes.LinkNode(node, LIST_TAIL, 0);
}

void CGUnit_C::HandleBloodPool(unsigned int currentTime) {
  UnitBloodRec *bloodRec = GetBloodRecord();
  if (bloodRec && static_cast<int>(currentTime - m_nextAllowableBloodPool) >= 0) {
    m_nextAllowableBloodPool = currentTime + 2000 + NTempest::CMath::mulhwu_(3000, NTempest::CRandom::uint32_(g_rndSeed));

    NTempest::C3Vector jitter(NTempest::CRandom::reals_(g_rndSeed) / 6.0f, NTempest::CRandom::reals_(g_rndSeed) / 6.0f, 0.0f);
    jitter += GetPosition();
    UnitFootprintNewBloodSplat(bloodRec, GetUnitSize(), jitter);
  }
}

UnitBloodRec *CGUnit_C::GetBloodRecord() {
  if (!m_bloodRec) {
    return 0;
  }
  return g_unitBloodDB.GetRecord(m_bloodRec->m_Violencelevel[ViolenceGetLevel()]);
}

void CGUnit_C::HandleCastAnimEvent() {
  int &castKit = reinterpret_cast<int *>(this)[395];
  if (castKit) {
    SpellVisualKitRec *kitRec = g_spellVisualKitDB.GetRecord(castKit);
    if (kitRec) {
      SpellVisualsPlayCastKit(this, kitRec, 0, 1);
    }
    castKit = 0;
  }
}

void CGUnit_C::OnCharmedChanged() {
  unsigned __int64 guid = GetGUID();
  CGGameUI::UnitNameUpdate(guid);
  HPLAYERNAME__ *unitNameHandle = *reinterpret_cast<HPLAYERNAME__ **>(reinterpret_cast<unsigned char *>(this) + 0x690);
  if (unitNameHandle) {
    PlayerNameTriggerColorUpdate(unitNameHandle);
  }
  guid = GetGUID();
  Script_SendUnitSignal(guid, 27);
}

void CGUnit_C::CheckPendingMissileRelease(const NTempest::C3Vector *position) {
  unsigned int *self = reinterpret_cast<unsigned int *>(this);
  if (self[396]) {
    SndInterfacePlaySpellSound(self[396], this);
    self[396] = 0;
  }

  if (self[397]) {
    NTempest::C3Vector effectPosition = position ? *position : GetPosition();
    SpellVisualsPlayCameraShakeID(self[397], effectPosition);
    self[397] = 0;
  }

  if (self[398]) {
    if (self[409]) {
      if (!(GetType() & TYPE_PLAYER)) {
        ThrownMissileReleased();
      } else {
        typedef unsigned long **(CGUnit_C::*GetDataProc)();
        GetDataProc getData;
        void      **vtable = *reinterpret_cast<void ***>(this);
        memcpy(&getData, &vtable[3], sizeof(getData));
        unsigned long  **data = (this->*getData)();
        unsigned __int64 itemGUID = 0;
        if (**data > 17) {
          itemGUID = *reinterpret_cast<unsigned __int64 *>(reinterpret_cast<unsigned char *>(data[1]) + 136);
        }
        CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(itemGUID, __FILE__, __LINE__));
        if (item) {
          if (item->GetInventoryType() == 25) {
            ThrownMissileReleased();
          } else {
            self[313] |= 0x800;
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
    self[402] = *reinterpret_cast<unsigned int *>(&missilePosition.x);
    self[403] = *reinterpret_cast<unsigned int *>(&missilePosition.y);
    self[404] = *reinterpret_cast<unsigned int *>(&missilePosition.z);

    int durationOffset = 0;
    if (self[442] == 38) {
      durationOffset = GetTickCount() - self[424];
    }
    UnitEffectAddMissile(*reinterpret_cast<MISSILESTRUCT *>(self + 398), durationOffset < 0 ? 0 : durationOffset);
    self[398] = 0;
  }

  HandleCastAnimEvent();
  CheckPendingImpactKit();
}

void CGUnit_C::ThrownMissileReleased() {
  HMODEL model = GetCharacterModel(0);
  FATALASSERT(model);
  SetRangedWeaponPullAnim(1);
  HandleClose(model);
  m_animFlags |= 0x20000;
}

void CGUnit_C::CheckPendingThrownWeaponReattach(unsigned int force) {
  typedef int (CGUnit_C::*GetVirtualItemDisplayIDFn)(unsigned int);
  typedef VirtualItemInfo *(CGUnit_C::*GetVirtualItemFn)(unsigned int, unsigned int);
  void                    **vtable = *reinterpret_cast<void ***>(this);
  GetVirtualItemDisplayIDFn getVirtualItemDisplayID;
  GetVirtualItemFn          getVirtualItem;
  memcpy(&getVirtualItemDisplayID, &vtable[0x12C / 4], sizeof(getVirtualItemDisplayID));
  memcpy(&getVirtualItem, &vtable[0x128 / 4], sizeof(getVirtualItem));

  int              displayID = (this->*getVirtualItemDisplayID)(2);
  VirtualItemInfo *itemInfo = (this->*getVirtualItem)(2, 0);
  unsigned int    &flags = *reinterpret_cast<unsigned int *>(reinterpret_cast<unsigned char *>(this) + 0x4E4);
  if ((force || (flags & 0x20000)) && displayID && itemInfo) {
    const ItemSubClassRec *subClass = SDBItemSubclassGetSubClassRec(itemInfo->m_classID, itemInfo->m_subclassID);
    unsigned int           forceAlternate = subClass && (subClass->m_flags & 0x20);
    AddObjectComponentBySlot(17, displayID, itemInfo->m_inventoryType, forceAlternate, 0, 0, -1, 0);
    SetAttachmentHidden(4, 0);
  }
  flags &= ~0x20000u;
}

void CGUnit_C::OnStopRender() {
  CheckPendingVictimFeedback();
  CheckPendingImpactKit();
  CheckPendingMissileRelease(0);
  RemoveUnitNamePlate();
}

void CGUnit_C::CheckRendering() {
  int wasRendering = (m_flags & 0x40000) != 0;
  if (m_flags & 0x20000) {
    m_flags |= 0x40000;
  } else {
    m_flags &= ~0x40000;
  }
  m_flags &= ~0x20000;

  if (wasRendering && !(m_flags & 0x40000)) {
    OnStopRender();
  }
}

void CGUnit_C::UpdateDisplay(unsigned long now) {
  CheckRendering();
  if (m_worldObject) {
    UpdateDisplayInfo();
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

  void **vtable = *reinterpret_cast<void ***>(this);
  typedef void (CGUnit_C::*CleanupUnitArtworkFn)(int, int);
  CleanupUnitArtworkFn cleanupUnitArtwork;
  memcpy(&cleanupUnitArtwork, &vtable[0x150 / 4], sizeof(cleanupUnitArtwork));
  (this->*cleanupUnitArtwork)(playerModelChanged, wasPlayerModel);

  RefreshDataPointers();

  typedef void (CGUnit_C::*ArtworkFn)();
  ArtworkFn reinitializeUnitArtwork;
  memcpy(&reinitializeUnitArtwork, &vtable[0x154 / 4], sizeof(reinitializeUnitArtwork));
  (this->*reinitializeUnitArtwork)();

  InitializeExtendedDisplay();
  AttachVirtualMonsterWeapons();
  InitializeNPCItems();

  ArtworkFn postReinitializeArtwork;
  memcpy(&postReinitializeArtwork, &vtable[0x158 / 4], sizeof(postReinitializeArtwork));
  (this->*postReinitializeArtwork)();

  void *geosetHandle = *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(this) + 0x770);
  if (geosetHandle) {
    CharCustomizationCommitItemGeosets(reinterpret_cast<HCHARGEOSET>(geosetHandle), 0);
  }

  SpellRec *pendingShapeshift = *reinterpret_cast<SpellRec **>(reinterpret_cast<unsigned char *>(this) + 0x9D4);
  if (pendingShapeshift) {
    SpellVisualRec    *visual = g_spellVisualDB.GetRecord(pendingShapeshift->m_spellVisualID);
    SpellVisualKitRec *stateKit = visual ? g_spellVisualKitDB.GetRecord(visual->m_stateKit) : 0;
    if (stateKit) {
      PlayImpactKit(pendingShapeshift->m_ID, stateKit);
    }
    *reinterpret_cast<SpellRec **>(reinterpret_cast<unsigned char *>(this) + 0x9D4) = 0;
  }

  ReinitializePaperdollModel();
  UpdatePortraitTexture(GetGUID());
}

int CGUnit_C::DisplayInfoNeedsUpdate(int &playerModelChanged, int &wasPlayerModel) const {
  FATALASSERT(m_modelData);
  playerModelChanged = 0;
  wasPlayerModel = 0;

  CreatureDisplayInfoRec *displayInfo = g_creatureDisplayInfoDB.GetRecord(m_unit->displayID);
  if (!displayInfo) {
    SysMsgPrintf(SYSMSG_ERROR, 2, "NOUNITDISPLAYID|%d|%s", m_unit->displayID, GetUnitName());
    return 0;
  }
  if (displayInfo == m_displayInfo) {
    return 0;
  }

  CreatureModelDataRec *modelData = g_creatureModelDataDB.GetRecord(displayInfo->m_modelID);
  if (modelData == m_modelData) {
    CreatureSoundDataRec *soundData = g_creatureSoundDataDB.GetRecord(displayInfo->m_soundID);
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
      GetGUID(), OffsetOf(ID_UNIT) + 160, 4, VirtualItemChangeHandler, reinterpret_cast<void *>(1), HANDLER_PRIORITY_NORMAL
  );
  ClntObjMgrSetObjMirrorHandler(
      GetGUID(), OffsetOf(ID_UNIT) + 164, 4, VirtualItemChangeHandler, reinterpret_cast<void *>(2), HANDLER_PRIORITY_NORMAL
  );
  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 684, 4, DynamicFlagsChangeHandler, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 688, 4, EmoteStateChangeHandler, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 692, 4, ChannelSpellChangeHandler, 0, HANDLER_PRIORITY_NORMAL);
}

IMPACTEFFECTDESC::~IMPACTEFFECTDESC() {
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
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 160, VirtualItemChangeHandler, reinterpret_cast<void *>(1));
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 164, VirtualItemChangeHandler, reinterpret_cast<void *>(2));
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 684, DynamicFlagsChangeHandler, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 688, EmoteStateChangeHandler, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + 692, ChannelSpellChangeHandler, 0);
}

void CGUnit_C::StandStateChanged(unsigned int oldState) {
  if (IsInReenable()) {
    return;
  }

  unsigned int newState = m_unit->standState;
  OnStandStateChanged(oldState, newState);
  SetSheatheReason(SHEATHEREASON_2, s_standStateStartsSheathe[newState], 0);
  FATALASSERT(newState < 9);
  FATALASSERT(oldState < 9);

  if ((IsInStandSitTransition() || IsInSitSleepPosition()) && (!newState || oldState)) {
    PlayEmoteAnimation(s_standStateEndEmote[oldState], 0);
  }
  if (newState || !(reinterpret_cast<unsigned int *>(this)[32] & 0x40FF)) {
    PlayEmoteAnimation(s_standStateStartEmote[newState], 0);
  }
  if (reinterpret_cast<unsigned int *>(this)[442] == 46) {
    ClearTorsoAnimation(64);
  }
}

SPELLEFFECTDESC::~SPELLEFFECTDESC() {
  ClearLightningObjects();
}

void CGUnit_C::NPCFlagChanged(unsigned int oldNPCFlags) {
  unsigned int newNPCFlags = m_unit->npcFlags;
  unsigned int changed = oldNPCFlags ^ newNPCFlags;
  if (changed & 8) {
    RemoveInteractIcon();
  }
  if (changed & 0x10) {
    RemoveInteractIcon();
    if (newNPCFlags & 0x10) {
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
    if (newNPCFlags & 2) {
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
    if (newNPCFlags & 4) {
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (player) {
        player->UpdateTaxiStatus(this);
      }
    }
  }
}

void CGUnit_C::WeaponModeChanged() {
  if (GetGUID() == ClntObjMgrGetActivePlayer()) {
    typedef void (CGUnit_C::*SetWeaponModeFn)(WEAPONMODE);
    SetWeaponModeFn setWeaponMode;
    void          **vtable = *reinterpret_cast<void ***>(this);
    memcpy(&setWeaponMode, &vtable[0x194 / 4], sizeof(setWeaponMode));
    (this->*setWeaponMode)(static_cast<WEAPONMODE>(-1));
    if (!SheatheAnimPlaying() || (reinterpret_cast<const unsigned char *>(this)[0x4EA] & 1)) {
      UpdateSheatheRangedReasons(1);
    }
  } else {
    static const unsigned char s_standStateStartsSheathe[9] = {1, 0, 0, 0, 0, 0, 0, 0, 0};
    if (s_standStateStartsSheathe[m_unit->standState]) {
      MaybeStartSheatheAnim();
    }
  }
}

void CGUnit_C::UpdateSheatheRangedReasons(unsigned int suppressSound) {
  if (m_unit->weaponMode == WEAPONMODE_MELEE) {
    SetSheatheReason(SHEATHEREASON_0, 1, suppressSound);
  } else if (m_unit->weaponMode == WEAPONMODE_RANGED) {
    CheckPendingThrownWeaponReattach(1);
    SetSheatheReason(SHEATHEREASON_5, 1, suppressSound);
    SetSheatheReason(SHEATHEREASON_0, 0, 0);
  } else {
    SetSheatheReason(SHEATHEREASON_0, 0, 0);
    SetSheatheReason(SHEATHEREASON_5, 0, 0);
  }
}

void CGUnit_C::HandlePrecastStart(unsigned int precast) {
  if (precast) {
    SetSheatheReason(SHEATHEREASON_PRECAST, 1, 1);
  }
}

void CGUnit_C::HandlePrecastStop(int spellID, unsigned int force) {
  SpellRec          *spellRec = g_spellDB.GetRecord(spellID);
  SpellVisualRec    *visualRec = spellRec ? g_spellVisualDB.GetRecord(spellRec->m_spellVisualID) : 0;
  SpellVisualKitRec *kitRec = visualRec ? g_spellVisualKitDB.GetRecord(visualRec->m_castKit) : 0;
  if (kitRec && kitRec->m_anim != -1 && !force) {
    m_precastSheatheHoldTimer = GetTickCount() + 1000;
  } else {
    SetSheatheReason(SHEATHEREASON_PRECAST, 0, 1);
    m_precastSheatheHoldTimer = -1;
  }
}

ANIMQUEUENODE *CGUnit_C::GetNewAnimNode(int leaveUnlinked) {
  void          *storage = s_animQueueFreeList.GetData(0, typeid(ANIMQUEUENODE).raw_name(), -2);
  ANIMQUEUENODE *node = storage ? new (storage) ANIMQUEUENODE : 0;

  if (!leaveUnlinked) {
    m_animQueue.LinkNode(node, LIST_TAIL, 0);
  }

  return node;
}

void CGUnit_C::RecycleAnimNode(ANIMQUEUENODE *node) {
  if (node) {
    node->~ANIMQUEUENODE();
    s_animQueueFreeList.PutData(node, 0, 0);
  }
}

static unsigned char NodesSame(const ANIMQUEUENODE* current, const ANIMQUEUENODE* next) {
    // TODO: implement
    return 0;
}

bool CGUnit_C::SetSpellPreCastingAnimation(ANIMENUMERATION anim) {
  HMODEL charModel = GetCharacterModel(0);
  FATALASSERT(charModel);

  bool result = true;
  if (ModelIsLoaded(charModel, 1)) {
    unsigned int sequence = anim;
    while (!ModelHasSequenceId(charModel, sequence)) {
      if (sequence == 31) {
        result = false;
        sequence = 0;
        break;
      }
      sequence = 31;
    }
    HandleClose(charModel);
    reinterpret_cast<unsigned int *>(this)[390] = sequence;
    return result;
  }

  reinterpret_cast<unsigned int *>(this)[392] = anim;
  return false;
}

void CGUnit_C::SetSpellImpactKit(const SpellVisualKitRec *impactKit) {
  if (impactKit && impactKit->m_anim >= 1) {
    reinterpret_cast<unsigned int *>(this)[277] = impactKit->m_anim;
    SetTorsoAnimation(47, 0, 0);
  }
}

int CGUnit_C::PlayEmoteAnimation(unsigned int emoteID, int flags) {
  EmotesRec *emote = g_emotesDB.GetRecord(emoteID);
  if (!emote || m_unit->standState == 3 || (emote->m_EmoteFlags & 2) || (reinterpret_cast<const unsigned int *>(this)[32] & 0x2000000)) {
    return 0;
  }
  return SetEmoteAnimation(emoteID, flags);
}

void CGUnit_C::RequestTalkEmote(TALKANIMATION talkAnim) {
  FATALASSERT(talkAnim < TALKANIM_NUMTALKANIMS);

  EmotesRec *emote = s_talkEmotes[talkAnim];
  if (emote && !(reinterpret_cast<unsigned int *>(this)[32] & 0x2000000) && SetEmoteAnimation(emote->m_ID, 0) && (emote->m_EmoteFlags & 0x800)) {
    SetSheatheReason(SHEATHEREASON_6, 1, 1);
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

void CGUnit_C::ChangeStandState(unsigned int standState) {
  FATALASSERT(!(standState >= 4 && standState <= 6));
  FATALASSERT(standState < 9);

  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_STANDSTATECHANGE));
  msg.Put(standState);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGUnit_C::RegisterScript() {
  SetMirrorHandlers();
  SetAuraMirrorHandlers();
}

void CGUnit_C::UnregisterScript() {
  UnsetMirrorHandlers();
  UnsetAuraMirrorHandlers();
}

int CGUnit_C::SetEmoteAnimation(unsigned int emoteID, int flags) {
  reinterpret_cast<unsigned int *>(this)[394] = emoteID;
  return SetTorsoAnimation(46, 0, flags);
}

bool __fastcall AnimSheathesWeapon(unsigned int anim) {
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
      *reinterpret_cast<TSGrowableArray<QUESTGIVEREMOTENODE> *>(reinterpret_cast<unsigned char *>(this) + 1164);
  queue.SetCount(0);

  EMOTESPECPROCS proc = EMOTESPECPROC_0;
  while (num) {
    const QUESTGIVEREMOTENODE &node = list[--num];
    if (node.emoteID && EmoteProcType(node.emoteID, proc) && proc == EMOTESPECPROC_0) {
      *queue.New() = node;
    }
  }

  if (queue.Count()) {
    queue[queue.Count() - 1].delay += GetTickCount();
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
  unsigned int *preferredGeosets = reinterpret_cast<unsigned int *>(this) + 478;
  memset(preferredGeosets, 0, 15 * sizeof(*preferredGeosets));
  preferredGeosets[CGS_HAIR] = HairStyleID();

  BEARDSTYLEDATA beardStyleData = {1, 1, 1};
  int            hasFacialInfo = CharCustomizationGetBeardStyle(GetDisplayRace(), GetDisplaySex(), FacialHairID(), &beardStyleData);
  preferredGeosets[CGS_EARS] = 2;
  if (hasFacialInfo) {
    preferredGeosets[CGS_FACIAL_BEARD] = beardStyleData.beardGeoset;
    preferredGeosets[CGS_FACIAL_SIDEBURN] = beardStyleData.sideBurnGeoset;
    preferredGeosets[CGS_FACIAL_MOUSTACHE] = beardStyleData.moustacheGeoset;
  }
}

void CGUnit_C::InitializeNPCItems() {
  if ((m_obj->m_type & TYPE_PLAYER) || !reinterpret_cast<unsigned int *>(this)[476] || !m_displayInfoExtra) {
    return;
  }

  static const int s_inventoryTypes[10] = {1, 3, 4, 5, 6, 7, 8, 9, 10, 19};
  static const int s_inventorySlots[10] = {0, 2, 3, 4, 5, 6, 7, 8, 9, 18};

  HMODEL charModel = GetCharacterModel(0);
  FATALASSERT(charModel);
  HCHARGEOSET geosetHandle = reinterpret_cast<HCHARGEOSET *>(this)[476];
  FATALASSERT(geosetHandle);
  HTEXCOMPONENT texComponent = reinterpret_cast<HTEXCOMPONENT *>(this)[477];
  unsigned int *preferredGeosets = reinterpret_cast<unsigned int *>(this) + 478;

  for (unsigned int i = 0; i < 10; ++i) {
    int displayID = m_displayInfoExtra->m_NPCItemDisplay[i];
    if (!displayID) {
      continue;
    }

    AddObjectComponentBySlot(s_inventorySlots[i], displayID, s_inventoryTypes[i], false, false, false, -1, false);

    ItemDisplayInfoRec *displayInfoRec = g_itemDisplayInfoDB.GetRecord(displayID);
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
  unsigned __int64 guid = GetGUID();
  Script_SendUnitSignal(guid, 16);
}

void CGUnit_C::UpdateDisplayHealth() {
  unsigned int *self = reinterpret_cast<unsigned int *>(this);
  if (!self[448]) {
    self[493] = m_unit->health;
    SignalDisplayHealthUpdate();
  }
}

bool CGUnit_C::IsSpellKnown(int spellID) const {
  return GetGUID() == ClntObjMgrGetActivePlayer() ? CGSpellBook::IsSpellKnown(spellID) : CGSpellBook::IsPetSpellKnown(spellID);
}

const SkillLineAbilityRec *CGUnit_C::LookupAbility(int spellID) const {
  const CGUnitData *unitData = GetUnitData();
  for (int index = 0; index < g_skillLineAbilityDB.GetNumRecords(); ++index) {
    const SkillLineAbilityRec *ability = g_skillLineAbilityDB.GetRecordByIndex(index);
    if (ability->m_spell != spellID) {
      continue;
    }
    int raceMask = ability->m_excludeRace ? ~ability->m_raceMask : ability->m_raceMask;
    int classMask = ability->m_excludeClass ? ~ability->m_classMask : ability->m_classMask;
    if (raceMask && !(raceMask & (1 << (unitData->race - 1)))) {
      continue;
    }
    if (classMask && !(classMask & (1 << (unitData->classId - 1)))) {
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
  SpellRec *spell = g_spellDB.GetRecord(spellID);
  if ((!spell || !IsShapeshiftSpell(spell)) && target && impactKit) {
    if (immediate) {
      target->PlayImpactKit(spellID, impactKit);
    } else {
      IMPACTEFFECTDESC *desc = new IMPACTEFFECTDESC;
      if (desc) {
        desc->Set(GetGUID(), target->GetGUID(), impactKit, spellID);
      }
      typedef TSList<IMPACTEFFECTDESC, TSGetLink<IMPACTEFFECTDESC> > ImpactList;
      ImpactList &impactEffects = *reinterpret_cast<ImpactList *>(reinterpret_cast<unsigned char *>(this) + 0x7BC);
      impactEffects.LinkNode(desc, LIST_TAIL, 0);
    }
  }
}

void CGUnit_C::PlayImpactKit(int spellID, const SpellVisualKitRec *impactKit) {
  FATALASSERT(impactKit);
  UnitEffectOneShot(g_spellVisualEffectNameDB.GetRecord(impactKit->m_headEffect), this, UNITEFFECT_ATTACHHEAD, 0, true, false);
  UnitEffectOneShot(g_spellVisualEffectNameDB.GetRecord(impactKit->m_chestEffect), this, UNITEFFECT_ATTACHCHEST, 0, true, false);
  UnitEffectOneShot(g_spellVisualEffectNameDB.GetRecord(impactKit->m_baseEffect), this, UNITEFFECT_ATTACHBASE, 0, true, false);

  NTempest::C3Vector position;
  GetPosition(position);
  SpellVisualsPlayCameraShakeID(impactKit->m_shakeID, position);
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
  const unsigned int *self = reinterpret_cast<const unsigned int *>(this);
  if (((m_unit->flags & 0x2000) && (reinterpret_cast<const unsigned char *>(this)[1252] & 0x10) && self[444] == 6) || self[440] == 6) {
    UpdateBaseAnimation(6, 0x100);
  }
}

void CGUnit_C::CheckPendingImpactKit() {
  typedef TSList<IMPACTEFFECTDESC, TSGetLink<IMPACTEFFECTDESC> > ImpactList;
  ImpactList       &impactEffects = *reinterpret_cast<ImpactList *>(reinterpret_cast<unsigned char *>(this) + 0x7BC);
  IMPACTEFFECTDESC *head;
  while ((head = impactEffects.Head()) != 0) {
    head->Unlink();
    FATALASSERT(head->impactKit);
    FATALASSERT(head->victim);
    CGUnit_C *victim = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(head->victim, __FILE__, __LINE__));
    if (victim) {
      victim->PlayImpactKit(head->spellID, head->impactKit);
    }
    delete head;
  }
}

void CGUnit_C::SpellAnimEndHandler() {
  unsigned int state = reinterpret_cast<const unsigned int *>(this)[442];
  if (state == 38 || state == 47 || state == 62 || state == 63) {
    ClearTorsoAnimation(0x40);
  }
  HandleCastAnimEvent();
}

void CGUnit_C::AddSpellProcAuraEffect(int auraslot, const SpellVisualKitRec *rec) {
  if (!rec) {
    return;
  }

  unsigned int proc = rec->m_characterParam[0];
  if (proc >= 11 || ((1 << proc) & 0x640)) {
    return;
  }

  HMODEL charModel = GetCharacterModel(0);
  if (!charModel) {
    return;
  }

  void            *storage = s_spellEffectFreeList.GetData(0, typeid(SPELLEFFECTDESC).raw_name(), -2);
  SPELLEFFECTDESC *newDesc = storage ? new (storage) SPELLEFFECTDESC : 0;
  FATALASSERT(newDesc);
  newDesc->kitPtr = const_cast<SpellVisualKitRec *>(rec);
  newDesc->isOneShot = 0;

  unsigned int     spellID;
  SpellEffectList &list = *reinterpret_cast<SpellEffectList *>(reinterpret_cast<unsigned char *>(this) + 1988 + proc * 12);
  if (auraslot == -1) {
    spellID = m_unit->channelSpell;
    SPELLEFFECTDESC **channelEffect = reinterpret_cast<SPELLEFFECTDESC **>(reinterpret_cast<unsigned char *>(this) + 2512);
    FATALASSERT(!*channelEffect);
    *channelEffect = newDesc;
  } else {
    spellID = m_unit->auras[auraslot];
    list.LinkNode(newDesc, LIST_TAIL, 0);
  }

  SpellProcHandler handler = s_spellProcHandlerFunctions[proc];
  if (handler) {
    handler(SPELLPROCADD, list, this, charModel, rec, newDesc, spellID, 0.0f);
  }
  HandleClose(charModel);
}

void CGUnit_C::RemoveSpellProcAuraEffect(ACTIVEAURAINFO *rec) {
  unsigned int             proc = static_cast<unsigned int>(-1);
  SPELLEFFECTDESC         *desc = 0;
  const SpellVisualKitRec *kitRec = 0;

  if (!rec) {
    SPELLEFFECTDESC **channelEffect = reinterpret_cast<SPELLEFFECTDESC **>(reinterpret_cast<unsigned char *>(this) + 2512);
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
    FATALASSERT(rec->slot < 56);
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
    s_spellEffectFreeList.PutData(desc, 0, 0);
    return;
  }

  HMODEL charModel = GetCharacterModel(0);
  if (!charModel) {
    desc->ClearLightningObjects();
    desc->Unlink();
    s_spellEffectFreeList.PutData(desc, 0, 0);
    return;
  }

  SpellProcHandler handler = s_spellProcHandlerFunctions[proc];
  if (handler) {
    SpellEffectList &list = *reinterpret_cast<SpellEffectList *>(reinterpret_cast<unsigned char *>(this) + 1988 + proc * 12);
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

  if (!m_unit->emoteState) {
    reinterpret_cast<unsigned int *>(this)[435] = GetTickCount();
    HMODEL charModel = GetCharacterModel(0);
    FATALASSERT(charModel);
    unsigned int anim = (m_animFlags & 0x400) && IsUnderWater() ? 132 : 6;
    ObjectModelSetSequence(charModel, anim, 2, 0);
    HandleClose(charModel);
  }
}

SPELLEFFECTDESC *CGUnit_C::FindSpellEffectProcDesc(const SpellVisualKitRec *rec) {
  unsigned int proc = rec->m_characterParam[0];
  if (proc >= 11) {
    return 0;
  }
  SpellEffectList &list = *reinterpret_cast<SpellEffectList *>(reinterpret_cast<unsigned char *>(this) + 1988 + proc * 12);
  for (SPELLEFFECTDESC *desc = list.Head(); desc; desc = desc->Next()) {
    if (desc->kitPtr == rec) {
      return desc;
    }
  }
  return 0;
}

int CGUnit_C::JumpTakeOffFinishedHandler() {
  ObjectModelSetSequence(reinterpret_cast<HMODEL *>(this)[4], ANIM_JUMP, GetObjAnimFlags(2), 0);
  return 1;
}

int CGUnit_C::JumpLandFinishedHandler() {
  typedef void (CGUnit_C::*SetBaseAnimStateProc)(unsigned int);
  SetBaseAnimStateProc setBaseAnimState;
  void               **vtable = *reinterpret_cast<void ***>(this);
  memcpy(&setBaseAnimState, &vtable[0x110 / sizeof(void *)], sizeof(setBaseAnimState));
  (this->*setBaseAnimState)(0);
  return 1;
}

void CGUnit_C::SitSleepAnimEndHandler() {
  typedef void (CGUnit_C::*SetBaseAnimStateProc)(unsigned int);
  SetBaseAnimStateProc setBaseAnimState;
  void               **vtable = *reinterpret_cast<void ***>(this);
  memcpy(&setBaseAnimState, &vtable[0x110 / sizeof(void *)], sizeof(setBaseAnimState));
  (this->*setBaseAnimState)(0);
}

void CGUnit_C::InternalProcessSpellProcEffects(SPELLPROC_ACTION action, float elapsed) {
  HMODEL charModel = GetCharacterModel(0);
  if (!charModel) {
    return;
  }

  for (unsigned int proc = 0; proc < 11; ++proc) {
    SpellProcHandler handler = s_spellProcHandlerFunctions[proc];
    SpellEffectList &list = *reinterpret_cast<SpellEffectList *>(reinterpret_cast<unsigned char *>(this) + 1988 + proc * 12);
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
  int *current = reinterpret_cast<int *>(reinterpret_cast<unsigned char *>(this) + 0x848);
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
  int *current = reinterpret_cast<int *>(reinterpret_cast<unsigned char *>(this) + 0x848);
  current[0] -= color.r;
  current[1] -= color.g;
  current[2] -= color.b;

  HMODEL model = GetCharacterModel(0);
  if (model) {
    FATALASSERT(current[0] >= 0 && current[1] >= 0 && current[2] >= 0);
    ModelSetEmissiveColor(model, color, 0);
  }
}

int __fastcall CGUnit_C::GetAnimPriority(int state) {
  FATALASSERT(state >= 0);
  FATALASSERT(state < sizeof(s_animInfo) / sizeof(s_animInfo[0]));
  return s_animInfo[state].basePriority;
}

int CGUnit_C::IsInStandSitTransition() {
  if (m_animFlags & 0x40000) {
    return 1;
  }
  return IsSitStandSleepTransition(m_currentBaseAnimState);
}

static int __fastcall IsSitStandSleepTransition(unsigned int animState) {
  return animState < 64 ? (s_animInfo[animState].statePreempts >> 16) & 1 : 0;
}

int CGUnit_C::IsInSitSleepPosition() {
  if (m_animFlags & 0x40000) {
    return 0;
  }

  unsigned int animState = m_currentBaseAnimState;
  return animState < 64 ? (s_animInfo[animState].statePreempts >> 19) & 1 : 0;
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
  if (unit == GetGUID()) {
    return AFFILIATION_YOURSELF;
  }
  return AFFILIATION_OTHER;
}

int CGUnit_C::GetSpellRank(int spellID) {
  FATALASSERT(!IsA(TYPE_PLAYER));

  SpellRec *spell = g_spellDB.GetRecord(spellID);
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
    reinterpret_cast<unsigned int *>(this)[314] |= 0x4000;
  } else {
    PerformLevelUpAnim(1);
  }
}

void CGUnit_C::PerformLevelUpAnim(int force) {
  unsigned int &flags = reinterpret_cast<unsigned int *>(this)[314];
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
  HPLAYERNAME__ *name = *reinterpret_cast<HPLAYERNAME__ **>(reinterpret_cast<unsigned char *>(this) + 0x690);
  PlayerNameCreateText(name, s_worldTextInfo[type].type, buf, 0);
}

void CGUnit_C::AddWorldText(MISS_REASON reason) {
  static WORLDTEXTMISSTYPE s_worldMissTextReasons[10] = {WORLDTEXTMISS_PHYSICAL,  WORLDTEXTMISS_PHYSICAL, WORLDTEXTMISS_RESIST, WORLDTEXTMISS_DODGED,
                                                         WORLDTEXTMISS_PARRIED,   WORLDTEXTMISS_BLOCKED,  WORLDTEXTMISS_EVADED, WORLDTEXTMISS_IMMUNE,
                                                         WORLDTEXTMISS_DEFLECTED, WORLDTEXTMISS_ABSORBED};

  FATALASSERT(static_cast<unsigned int>(reason) < 10);
  AddWorldText(s_worldMissTextReasons[reason]);
}

void __fastcall CGUnit_C_RenderBowStrings(NTempest::C3Vector &c) {
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

void CGUnit_C::DrawBowString(NTempest::C3Vector &cameraPos) {
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
    ModelSetSequence(model, 3, 0);
    HandleClose(model);
  }
}

void CGUnit_C::SetBaseAnim(unsigned int newAnim) {
  SetSheatheReason(SHEATHEREASON_3, AnimSheathesWeapon(newAnim), 0);
  FATALASSERT(newAnim != NUM_OBJECTANIMATIONS);
  reinterpret_cast<unsigned int *>(this)[441] = newAnim;
}

void CGUnit_C::SetTorsoAnim(unsigned int newAnim) {
  SetSheatheReason(SHEATHEREASON_4, AnimSheathesWeapon(newAnim), 0);
  reinterpret_cast<unsigned int *>(this)[443] = newAnim;
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
  if (m_unit->weaponMode != 2) {
    return 0;
  }

  HMODEL characterModel = GetCharacterModel(0);
  if (!characterModel) {
    return 0;
  }

  typedef const void *(CGUnit_C::*GetItemStatsProc)(int, unsigned int);
  GetItemStatsProc getItemStats;
  void           **vtable = *reinterpret_cast<void ***>(this);
  memcpy(&getItemStats, &vtable[74], sizeof(getItemStats));
  const unsigned char *itemStats = static_cast<const unsigned char *>((this->*getItemStats)(2, 0));

  unsigned int linkPoint;
  if (!itemStats || itemStats[3] == 25) {
    HandleClose(characterModel);
    return 0;
  }
  if (itemStats[3] == 15) {
    linkPoint = 2;
  } else if (itemStats[3] == 26) {
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
  unsigned int &timer = *reinterpret_cast<unsigned int *>(reinterpret_cast<unsigned char *>(this) + 0x960);
  if (timer) {
    ClientKillTimer(timer, RangedStandTimerHandler, "RangedStandTimerHandler");
    timer = 0;
  }
}

void CGUnit_C::OnRangedStandTimer() {
  unsigned char *self = reinterpret_cast<unsigned char *>(this);
  *reinterpret_cast<unsigned int *>(self + 0x960) = 0;
  DetermineReadySequence(1);
  if (*reinterpret_cast<unsigned int *>(self + 0x6E0) != 1) {
    typedef void (CGUnit_C::*UpdateBaseAnimationFn)(unsigned int);
    void                **vtable = *reinterpret_cast<void ***>(this);
    UpdateBaseAnimationFn updateBaseAnimation;
    memcpy(&updateBaseAnimation, &vtable[0x110 / 4], sizeof(updateBaseAnimation));
    (this->*updateBaseAnimation)(0x100);
  }
}

void CGUnit_C::StopRangedAttackPrecast() {
  if (m_unit->weaponMode != 2) {
    HMODEL model = GetRangedWeaponModel();
    if (model) {
      ModelSetSequence(model, 0, 0);
      HandleClose(model);
    }
  }
}

void CGUnit_C::SetRangedWeaponPullAnim(int duration) {
  if (m_unit->weaponMode == WEAPONMODE_RANGED) {
    HMODEL model = GetRangedWeaponModel();
    if (model) {
      unsigned int sequenceDuration;
      if (ModelSetRandomSequenceFidget(model, 2, 0) && ModelGetSequenceDuration(model, 2, &sequenceDuration)) {
        if (duration) {
          ModelSetTimeScale(model, static_cast<float>(sequenceDuration) / static_cast<float>(duration), 0);
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

void CGUnit_C::DDWRITELOG(const char *buffer) {
  unsigned int current = m_deathHoldBuffer.Count();
  unsigned int bytes = SStrLen(buffer) + 1;
  m_deathHoldBuffer.SetCount(current + bytes);
  memcpy(&m_deathHoldBuffer[current], buffer, bytes);
  *m_deathHoldBufferIndices.New() = current;
}

void CGUnit_C::DDADDLOG(unsigned __int64 guid, const char *string, const char *file, unsigned int line) {
  char buffer[512];
  SStrPrintf(buffer, sizeof(buffer), "[DDADD 0x%016I64X (%d)]: %s (%s:%d)", guid, reinterpret_cast<unsigned int *>(this)[448], string, file, line);
  DDWRITELOG(buffer);
  AddDeathHold();
}

void CGUnit_C::DDDELLOG(unsigned __int64 guid, const char *string, const char *file, unsigned int line) {
  char buffer[512];
  SStrPrintf(buffer, sizeof(buffer), "[DDDEL 0x%016I64X (%d)]: %s (%s:%d)", guid, reinterpret_cast<unsigned int *>(this)[448], string, file, line);
  DDWRITELOG(buffer);
  DelDeathHold();
}

void CGUnit_C::DDGENLOG(unsigned __int64 guid, const char *string, const char *file, unsigned int line) {
  char buffer[512];
  SStrPrintf(buffer, sizeof(buffer), "[DDGEN 0x%016I64X (%d)]: %s (%s:%d)", guid, reinterpret_cast<unsigned int *>(this)[448], string, file, line);
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
  unsigned int &flags = *reinterpret_cast<unsigned int *>(reinterpret_cast<unsigned char *>(this) + 0x4E4);
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
    if (anim != static_cast<unsigned int>(-1) && SetSpellPreCastingAnimation(static_cast<ANIMENUMERATION>(anim))) {
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
    if (activePlayer->GetGUID() == GetGUID()) {
      SStrPrintf(labelString, sizeof(labelString), " (Local Player)");
    } else {
      typedef unsigned __int64(__fastcall * GetLocalTargetFn)(CGUnit_C *, void *);
      GetLocalTargetFn getLocalTarget = reinterpret_cast<GetLocalTargetFn>((*reinterpret_cast<void ***>(activePlayer))[41]);
      if (getLocalTarget(activePlayer, 0) == GetGUID()) {
        SStrPrintf(labelString, sizeof(labelString), " (Local Player's Target)");
      }
    }
  }

  DDGenerateLogString(handle, stringBuffer, "======================================");
  DDGenerateLogString(handle, stringBuffer, "Death Holds for unit %s%s (0x%016I64X)", GetUnitName(), labelString, GetGUID());

  NTempest::C3Vector pos;
  GetPosition(pos);
  DDGenerateLogString(handle, stringBuffer, "Unit at %g, %g, %g", pos.x, pos.y, pos.z);

  const unsigned int *self = reinterpret_cast<const unsigned int *>(this);
  DDGenerateLogString(handle, stringBuffer, "Current Torso Anim State: %d", self[442]);
  DDGenerateLogString(handle, stringBuffer, "Current Torso Anim: %d", GetCurrentTorsoAnim());
  DDGenerateLogString(handle, stringBuffer, "Current Base Anim State: %d", self[440]);
  DDGenerateLogString(handle, stringBuffer, "Current Base Anim: %d", self[441]);

  if (!self[448]) {
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

  IMPACTEFFECTDESC *impact = reinterpret_cast<IMPACTEFFECTDESC *>(self[496]);
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

int CGUnit_C::SetCastingSpell(int spellID, unsigned int force, unsigned int precastAnimSuccessful) {
  int &castingSpell = *reinterpret_cast<int *>(reinterpret_cast<unsigned char *>(this) + 0x698);
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
    SpellRec *spellRec = g_spellDB.GetRecord(spellID);
    if (!spellRec) {
      return;
    }

    TSGrowableArray<unsigned __int64> &targets =
        *reinterpret_cast<TSGrowableArray<unsigned __int64> *>(reinterpret_cast<unsigned char *>(this) + 2492);
    if (targets.Count() && (spellRec->m_attributesEx & 0x4000)) {
      SaveTrackingTarget(targets[0], TRACKTYPE_SPELLCHANNEL, 0);
    }

    SpellVisualRec    *visualRec = g_spellVisualDB.GetRecord(spellRec->m_spellVisualID);
    SpellVisualKitRec *channelKit = visualRec ? g_spellVisualKitDB.GetRecord(visualRec->m_channelKit) : 0;
    if (!channelKit) {
      return;
    }

    if (!(spellRec->m_attributes & 0x40000)) {
      SetSheatheReason(SHEATHEREASON_8, 1, 0);
    }
    AddSpellProcAuraEffect(-1, channelKit);
    AddKitAuras(channelKit, spellRec);
    PlaySpellLoopedSound(channelKit->m_soundID);
    if (channelKit->m_anim && channelKit->m_anim != -1) {
      reinterpret_cast<unsigned int *>(this)[391] = channelKit->m_anim;
      SetTorsoAnimation(62, 0, 0);
    }
    return;
  }

  SetSheatheReason(SHEATHEREASON_8, 0, 0);
  SpellRec *oldSpellRec = g_spellDB.GetRecord(oldSpell);
  if (GetTrackingTarget() && oldSpellRec && (oldSpellRec->m_attributesEx & 0x4000)) {
    ClearTrackingTarget(0);
  }
  if (reinterpret_cast<unsigned int *>(this)[442] == 62) {
    ClearTorsoAnimation(0);
  }
  RefreshAuraVisuals();
  KillSpellLoopedSound();
}

void CGUnit_C::ClearSavedChannelSpellTargets() {
  TSGrowableArray<unsigned __int64> &targets = *reinterpret_cast<TSGrowableArray<unsigned __int64> *>(reinterpret_cast<unsigned char *>(this) + 2492);
  targets.Clear();
}

void CGUnit_C::SetStandStateAnim(int standAnim) {
  int *self = reinterpret_cast<int *>(this);
  if (self[596] != standAnim) {
    self[596] = standAnim;
    if (self[440] == 3) {
      UpdateBaseAnimation(3, 0x100);
    }
  }
}

void CGUnit_C::SetWalkStateAnim(int walkAnim) {
  int *self = reinterpret_cast<int *>(this);
  if (self[595] != walkAnim) {
    FATALASSERT(walkAnim < NUM_OBJECTANIMATIONS);
    self[595] = walkAnim;
    if (self[440] == 5) {
      UpdateBaseAnimation(5, 0x100);
    }
  }
}

int CGUnit_C::GetStandStateAnim(HMODEL model) const {
  if (!model) {
    model = reinterpret_cast<HMODEL const *>(this)[4];
  }

  unsigned int standStateAnim = reinterpret_cast<const unsigned int *>(this)[596];
  return ModelHasSequenceId(model, standStateAnim) ? standStateAnim : 0;
}

int CGUnit_C::GetWalkStateAnim() const {
  unsigned int sequence = reinterpret_cast<const unsigned int *>(this)[595];
  return ModelHasSequenceId(reinterpret_cast<HMODEL const *>(this)[4], sequence) ? sequence : 4;
}

SpellVisualRec *CGUnit_C::GetAppropriateSpellVisual(SpellRec *spellRec, SpellVisualRec &filled) {
  FATALASSERT(spellRec);

  SpellVisualRec *itemVisual = 0;
  SpellVisualRec *spellVisual = g_spellVisualDB.GetRecord(spellRec->m_spellVisualID);
  if (spellRec->m_attributes & 2) {
    typedef int (CGUnit_C::*GetVirtualItemDisplayIDProc)(unsigned int);
    GetVirtualItemDisplayIDProc getVirtualItemDisplayID;
    void                      **vtable = *reinterpret_cast<void ***>(this);
    memcpy(&getVirtualItemDisplayID, &vtable[75], sizeof(getVirtualItemDisplayID));
    int                 displayID = (this->*getVirtualItemDisplayID)(2);
    ItemDisplayInfoRec *displayInfo = g_itemDisplayInfoDB.GetRecord(displayID);
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

void ACTIVEATTACHMENTINFO::Hide(CGUnit_C *unitPtr, HMODEL charModel, HMODEL paperDollModel, unsigned int hide) {
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

void CGUnit_C::SetHandsState(HMODEL model) {
  const unsigned int *self = reinterpret_cast<const unsigned int *>(this);
  if (!(self[313] & 4)) {
    ResetFingersSeq(model, 8, 17);
    return;
  }

  typedef VirtualItemInfo *(CGUnit_C::*GetVirtualItemFn)(unsigned int, unsigned int);
  GetVirtualItemFn getVirtualItem;
  void           **vtable = *reinterpret_cast<void ***>(this);
  memcpy(&getVirtualItem, &vtable[0x128 / 4], sizeof(getVirtualItem));

  if (m_unit->weaponMode == 2) {
    VirtualItemInfo *item = (this->*getVirtualItem)(2, 0);
    if (item) {
      if (reinterpret_cast<const unsigned char *>(item)[3] == 25) {
        SetHandState(model, item, 8, 12);
      } else {
        SetHandState(model, item, 13, 17);
      }
    }
  } else {
    VirtualItemInfo *left = (this->*getVirtualItem)(0, 0);
    VirtualItemInfo *right = (this->*getVirtualItem)(1, 0);
    SetHandState(model, left, 8, 12);
    SetHandState(model, right, 13, 17);
  }
}

void CGUnit_C::VirtualComponentChanged(int slot, int oldValue) {
  if (oldValue) {
    RemoveObjectComponentByInvSlot(slot, 0, 1);
  }
  AttachVirtualComponent(slot, 0);
  if (slot == 2) {
    SetSheatheReason(static_cast<SHEATHEREASONS>(5), m_unit->weaponMode == WEAPONMODE_RANGED, 1);
  }
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
  forceAlternate = forceAlternate || invSlot == 16;
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
    if ((*found && (*found)->modelInfo[0].model) ||
        ((*found = CreateAttachmentInfo(invSlot, displayID, inventoryType, forceAlternate, sheathe, sheathedAttachmentPoint, showHidden)) != 0))
    {
      if ((1u << inventoryType) & s_canHideslots[0]) {
        unsigned int *self = reinterpret_cast<unsigned int *>(this);
        if (self[611 + attachmentSlot]) {
          WeaponTrailClose(self[611 + attachmentSlot]);
        }
        self[611 + attachmentSlot] = 0;
        HMODEL model = (*found)->modelInfo[0].model;
        if (model && ModelIsLoaded(model, 1)) {
          self[611 + attachmentSlot] = WeaponTrailCreate(model);
        }
      }
      if (!deferApply && !showHidden) {
        ApplyAttachmentInfo(characterModel, sheathe, attachmentSlot, false);
      }
    }
  }
  HandleClose(characterModel);
}

bool CGUnit_C::UpdateVisibilitySlots(HMODEL characterModel, int attachmentSlot, ACTIVEATTACHMENTINFO **&found, int displayID, bool deferApply) {
  unsigned int          *self = reinterpret_cast<unsigned int *>(this);
  ACTIVEATTACHMENTINFO **active = reinterpret_cast<ACTIVEATTACHMENTINFO **>(&self[601 + attachmentSlot]);
  found = active;
  ACTIVEATTACHMENTINFO *current = *active;
  if (current && current->displayInfo->GetID() == displayID) {
    ClearDeferredAttachment(characterModel, attachmentSlot);
    return deferApply;
  }

  ACTIVEATTACHMENTINFO *deferred = reinterpret_cast<ACTIVEATTACHMENTINFO *>(self[606 + attachmentSlot]);
  if (deferred && deferred->displayInfo->GetID() == displayID) {
    *active = deferred;
    self[606 + attachmentSlot] = reinterpret_cast<unsigned int>(current);
    ClearDeferredAttachment(characterModel, attachmentSlot);
    return deferApply;
  }

  ClearDeferredAttachment(characterModel, attachmentSlot);
  current = *active;
  *active = reinterpret_cast<ACTIVEATTACHMENTINFO *>(self[606 + attachmentSlot]);
  self[606 + attachmentSlot] = reinterpret_cast<unsigned int>(current);
  ClearDeferredAttachment(characterModel, attachmentSlot);
  return false;
}

void CGUnit_C::ClearWeaponTrailHandles() {
  for (int i = 0; i < 5; ++i) {
    if (m_weaponTrails[i]) {
      WeaponTrailClose(m_weaponTrails[i]);
      m_weaponTrails[i] = 0;
    }
  }
}

void CGUnit_C::ClearDeferredAttachment(HMODEL charModel, int slot) {
  unsigned int         *self = reinterpret_cast<unsigned int *>(this);
  ACTIVEATTACHMENTINFO *info = reinterpret_cast<ACTIVEATTACHMENTINFO *>(self[606 + slot]);
  if (info) {
    info->ClearAttachmentFromModel(charModel, reinterpret_cast<HMODEL>(self[616]));
    info->Clear();
    FATALASSERT(!info->modelInfo[0].model);
    FATALASSERT(!info->modelInfo[1].model);
    delete info;
  }
  self[606 + slot] = 0;
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
  const unsigned int *self = reinterpret_cast<const unsigned int *>(this);
  bool                useMonsterComponent = (GetType() & TYPE_PLAYER) || self[477] || self[476] || self[221];
  if (!GetObjComponentInfo(
          GetDisplayRace(), GetDisplaySex(), displayID, inventoryType, useMonsterComponent, forceAlternate, models, attachmentPoints
      ))
  {
    return 0;
  }

  ACTIVEATTACHMENTINFO *info = new ACTIVEATTACHMENTINFO;
  info->inventoryType = inventoryType;
  info->flags = 0;
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
    info->modelInfo[i].currentLink = -1;
    models[i] = 0;
  }
  return info;
}

void CGUnit_C::RemoveObjectComponentByInvSlot(int invSlot, bool deferDeleteFromModel, bool removeRecord) {
  int attachmentSlot = InvSlotToObjAttachSlot(invSlot);
  if (attachmentSlot < 0) {
    return;
  }

  unsigned int *self = reinterpret_cast<unsigned int *>(this);
  if (self[611 + attachmentSlot]) {
    WeaponTrailClose(self[611 + attachmentSlot]);
    self[611 + attachmentSlot] = 0;
  }

  if (!self[601 + attachmentSlot] && !self[606 + attachmentSlot]) {
    return;
  }

  HMODEL model = GetCharacterModel(0);
  FATALASSERT(model);
  ClearDeferredAttachment(model, attachmentSlot);
  unsigned int current = self[601 + attachmentSlot];
  self[601 + attachmentSlot] = self[606 + attachmentSlot];
  self[606 + attachmentSlot] = current;
  if (removeRecord || !deferDeleteFromModel) {
    ClearDeferredAttachment(model, attachmentSlot);
  }
  HandleClose(model);
}

void CGUnit_C::ClearActiveAttachmentInfo() {
  HMODEL model = GetCharacterModel(0);
  for (int i = 0; i < 5; ++i) {
    if (m_attachments[i]) {
      m_attachments[i]->ClearAttachmentFromModel(model, m_paperDollModel);
      m_attachments[i]->Clear();
      delete m_attachments[i];
      m_attachments[i] = 0;
    }
    if (m_deferredAttachments[i]) {
      m_deferredAttachments[i]->ClearAttachmentFromModel(model, m_paperDollModel);
      m_deferredAttachments[i]->Clear();
      delete m_deferredAttachments[i];
      m_deferredAttachments[i] = 0;
    }
  }
  if (model) {
    HandleClose(model);
  }
}

unsigned int CGUnit_C::SheatheObjComponent(int slot, unsigned int sheathe) {
  int attachmentSlot = InvSlotToObjAttachSlot(slot);
  if (attachmentSlot < 0) {
    return 0;
  }

  unsigned int *self = reinterpret_cast<unsigned int *>(this);
  if (!self[601 + attachmentSlot]) {
    return 0;
  }

  HMODEL charModel = GetCharacterModel(0);
  FATALASSERT(charModel);
  ClearDeferredAttachment(charModel, attachmentSlot);
  unsigned int result = ApplyAttachmentInfo(charModel, sheathe, attachmentSlot, 0);
  HandleClose(charModel);
  return result;
}

bool CGUnit_C::ApplyAttachmentInfo(HMODEL characterModel, bool sheathe, int attachmentSlot, bool) {
  FATALASSERT(attachmentSlot < 5);
  FATALASSERT(characterModel);

  unsigned int         *self = reinterpret_cast<unsigned int *>(this);
  ACTIVEATTACHMENTINFO *info = reinterpret_cast<ACTIVEATTACHMENTINFO *>(self[601 + attachmentSlot]);
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
  if (attachmentSlot != 4 && !((1 << attachmentSlot) & 0xC)) {
    sheathe = false;
  }

  HMODEL paperDollModel = reinterpret_cast<HMODEL>(self[616]);
  info->ClearAttachmentFromModel(characterModel, paperDollModel);
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

    int linked = attachmentSlot > 1 ? ModelAddLink(characterModel, modelInfo.currentLink, modelInfo.model, 1.0f)
                                    : AddAttachment(characterModel, modelInfo.currentLink, modelInfo.model, 1.0f);
    if (linked) {
      ModelSetVertexAlpha(modelInfo.model, reinterpret_cast<const unsigned char *>(this)[44], 1);
    } else {
      modelInfo.currentLink = -1;
      failed = true;
    }

    if (paperDollModel && attachmentSlot != 4) {
      HMODEL duplicate = ModelDuplicate(modelInfo.model, 0);
      if (duplicate) {
        ModelSetVertexAlpha(duplicate, 255, 1);
        ModelAddLink(paperDollModel, modelInfo.currentLink, duplicate, 1.0f);
        HandleClose(duplicate);
      }
    }
  }

  info->flags &= ~5u;
  if (!failed && attachmentSlot >= 2) {
    info->flags |= 4;
    SetHandsState(characterModel);
  }
  return !failed;
}

void CGUnit_C::SetAttachmentHidden(int attachmentSlot, unsigned int hide) {
  if (attachmentSlot < 0 || attachmentSlot >= 5) {
    return;
  }

  unsigned int         *self = reinterpret_cast<unsigned int *>(this);
  ACTIVEATTACHMENTINFO *info = reinterpret_cast<ACTIVEATTACHMENTINFO *>(self[601 + attachmentSlot]);
  if (!info) {
    return;
  }

  HMODEL charModel = GetCharacterModel(0);
  info->Hide(this, charModel, reinterpret_cast<HMODEL>(self[616]), hide);
  HandleClose(charModel);
}

void CGUnit_C::ReinitializePaperdollModel() {
  if (reinterpret_cast<unsigned int *>(this)[616]) {
    unsigned __int64 guid = GetGUID();
    Script_SendUnitSignal(guid, 182);
  }
}

void CGUnit_C::CreatePaperdollModel() {
  HMODEL      &paperDollModel = reinterpret_cast<HMODEL *>(this)[616];
  unsigned int sequence = 0;
  if (paperDollModel) {
    sequence = ModelGetPrimarySequence(paperDollModel);
    HandleClose(paperDollModel);
  }

  paperDollModel = DuplicateCharacterModel(1);
  HCHARGEOSET geosetHandle = reinterpret_cast<HCHARGEOSET *>(this)[476];
  CharCustomizationSetPaperDollGeoset(geosetHandle, paperDollModel);
  FATALASSERT(paperDollModel);
  ClearSpecialEffects(paperDollModel);
  if (!ModelSetSequence(paperDollModel, 0, 5)) {
    ConsolePrintf("UNITNOSTAND|%s", GetUnitName());
  }
  ModelSetSequence(paperDollModel, 0, sequence, 0);
}

int CGUnit_C::ShouldDelayLevelupAnim() {
  unsigned int state = reinterpret_cast<const unsigned int *>(this)[442];
  FATALASSERT(state < 64);
  return ShouldDelayLevelupAnim(state);
}

int CGUnit_C::ShouldDelayLevelupAnim(unsigned int state) {
  FATALASSERT(state < 64);
  return s_animInfo[state].flags & 0x200;
}

HMODEL CGUnit_C::GetPaperDollModel(unsigned int duplicateModel) {
  HMODEL &paperDollModel = reinterpret_cast<HMODEL *>(this)[616];
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

SpellVisualKitRec *CGUnit_C::GetRangedSpellAnim(int id, unsigned int castKit) {
  SpellRec *spellRec = g_spellDB.GetRecord(id);
  if (!spellRec) {
    return 0;
  }

  SpellVisualRec  visRecData;
  SpellVisualRec *visualRec = GetAppropriateSpellVisual(spellRec, visRecData);
  if (!visualRec) {
    return 0;
  }

  int kitID = castKit ? visualRec->m_castKit : visualRec->m_precastKit;
  return g_spellVisualKitDB.GetRecord(kitID);
}

unsigned int CGUnit_C::GetCurrentTorsoAnim() const {
  const unsigned int *animation = reinterpret_cast<const unsigned int *>(reinterpret_cast<const unsigned char *>(this));
  return animation[442] ? animation[443] : animation[441];
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

void CGUnit_C::SetSheatheReason(SHEATHEREASONS reason, unsigned int on, unsigned int suppressSound) {
  if (reason >= SHEATHEREASON_NUMREASONS) {
    return;
  }

  unsigned char *self = reinterpret_cast<unsigned char *>(this);
  unsigned int  &reasons = *reinterpret_cast<unsigned int *>(self + 0x9A4);
  unsigned int   oldReasons = reasons;
  unsigned int   reasonFlag = 1u << reason;
  reasons = on ? reasons | reasonFlag : reasons & ~reasonFlag;

  unsigned int &flags = *reinterpret_cast<unsigned int *>(self + 0x4E8);
  if ((oldReasons == 0) != (reasons == 0) || (flags & 0x800)) {
    unsigned int playSound = !suppressSound && ((oldReasons ^ reasons) & 1);
    unsigned int sheathe = flags & 0x800 ? reinterpret_cast<unsigned char *>(m_unit)[0x29B] == WEAPONMODE_MELEE : oldReasons == 0;
    SheatheOrUnsheatheItems(reason, sheathe, playSound);
  }

  if (reasons) {
    SheatheObjComponent(4, reinterpret_cast<unsigned char *>(m_unit)[0x29B] != WEAPONMODE_RANGED);
  }
  flags &= ~0x800u;
}

void CGUnit_C::SheatheOrUnsheatheItems(SHEATHEREASONS reason, unsigned int sheathe, unsigned int playSound) {
  unsigned char *self = reinterpret_cast<unsigned char *>(this);
  *reinterpret_cast<SHEATHEREASONS *>(self + 0x9B4) = reason;
  unsigned int &flags = *reinterpret_cast<unsigned int *>(self + 0x9B0);
  flags = sheathe ? 3 : 1;
  if (playSound) {
    flags |= 4;
  }
}

void CGUnit_C::MaybeStartSheatheAnim() {
  unsigned char *self = reinterpret_cast<unsigned char *>(this);
  if (SheatheAnimPlaying()) {
    if (self[0x4EA] & 1) {
      HandleSheatheAnimEvent(1, 1);
    }
    return;
  }

  static const unsigned int s_slots[2] = {0, 1};
  int                       found = 0;
  int                      *handAnim = reinterpret_cast<int *>(self + 0x9A8);
  typedef VirtualItemInfo *(CGUnit_C::*GetVirtualItemFn)(unsigned int, unsigned int);
  void           **vtable = *reinterpret_cast<void ***>(this);
  GetVirtualItemFn getVirtualItem;
  memcpy(&getVirtualItem, &vtable[0x128 / 4], sizeof(getVirtualItem));
  for (unsigned int i = 0; i < 2; ++i) {
    const unsigned char *info = reinterpret_cast<unsigned char *>((this->*getVirtualItem)(s_slots[i], 0));
    if (info) {
      found = 1;
      handAnim[i] = (((1u << info[4]) & 0x88) != 0) + 89;
    }
  }

  if (!found) {
    HandleSheatheAnimEvent(0, 0);
  } else if (!SetSheathingSequence()) {
    HandleSheatheAnimEvent(1, 1);
  }
}

unsigned int CGUnit_C::SheatheAnimPlaying() {
  const int *handAnim = reinterpret_cast<const int *>(reinterpret_cast<const unsigned char *>(this) + 0x9A8);
  return handAnim[0] != -1 || handAnim[1] != -1;
}

void CGUnit_C::HandleSheatheAnimEvent(bool clearSheatheAnim, bool suppressSound) {
  HMODEL theModel = GetCharacterModel(0);
  FATALASSERT(theModel);
  ModelSetSequence(theModel, 3, 0);
  ModelSetSequence(theModel, 2, 0);
  ModelSetSequence(theModel, 3, 4, 6);
  ModelSetSequence(theModel, 2, 4, 6);
  HandleClose(theModel);

  int *handAnim = reinterpret_cast<int *>(reinterpret_cast<unsigned char *>(this) + 0x9A8);
  handAnim[0] = -1;
  handAnim[1] = -1;
  if (!(reinterpret_cast<unsigned char *>(this)[0x4EA] & 1)) {
    UpdateSheatheRangedReasons(suppressSound);
  }
}

bool CGUnit_C::SetSheathingSequence() {
  HMODEL theModel = GetCharacterModel(0);
  FATALASSERT(theModel);
  unsigned int &flags = *reinterpret_cast<unsigned int *>(reinterpret_cast<unsigned char *>(this) + 0x4E8);
  flags &= ~0x10000u;

  bool success = true;
  int *handAnim = reinterpret_cast<int *>(reinterpret_cast<unsigned char *>(this) + 0x9A8);
  for (unsigned int i = 0; i < 2; ++i) {
    if (handAnim[i] != -1) {
      success = success && ObjectModelSetBoneSequence(theModel, handAnim[i], s_hands[i], 0);
      ModelSetSequence(theModel, s_hands[i], 1);
    }
  }
  HandleClose(theModel);
  return success;
}

int __fastcall SheatheTypeToSheathePoint(int sheatheType, int invSlot) {
  unsigned int ranged = invSlot != 15;
  return s_savedSheathToAttachPoints[s_sheathePoints[ranged][sheatheType]];
}

void CGUnit_C::SetWeaponMode(WEAPONMODE mode) {
  FATALASSERT(mode < WEAPONMODE_NUMMODES);

  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_SETWEAPONMODE));
  msg.Put(mode);
  msg.Finalize();
  ClientServices_Send(&msg);

  typedef void (CGUnit_C::*SetLastWeaponModeSentFn)(int);
  void                  **vtable = *reinterpret_cast<void ***>(this);
  SetLastWeaponModeSentFn setLastWeaponModeSent;
  memcpy(&setLastWeaponModeSent, &vtable[0x194 / 4], sizeof(setLastWeaponModeSent));
  (this->*setLastWeaponModeSent)(mode);
}

void CGUnit_C::AddSpellProcOneShotEffect(int spellID, const SpellVisualKitRec *rec) {
  if (!rec) {
    return;
  }

  unsigned int proc = rec->m_characterParam[0];
  if (proc >= 11 || ((1 << proc) & 0x640)) {
    return;
  }

  HMODEL charModel = GetCharacterModel(0);
  if (!charModel) {
    return;
  }

  void            *storage = s_spellEffectFreeList.GetData(0, typeid(SPELLEFFECTDESC).raw_name(), -2);
  SPELLEFFECTDESC *newDesc = storage ? new (storage) SPELLEFFECTDESC : 0;
  FATALASSERT(newDesc);
  newDesc->kitPtr = const_cast<SpellVisualKitRec *>(rec);
  newDesc->isOneShot = 1;

  SpellEffectList &list = *reinterpret_cast<SpellEffectList *>(reinterpret_cast<unsigned char *>(this) + 1988 + proc * 12);
  list.LinkNode(newDesc, LIST_TAIL, 0);
  SpellProcHandler handler = s_spellProcHandlerFunctions[proc];
  if (handler) {
    handler(SPELLPROCADD, list, this, charModel, rec, newDesc, spellID, 0.0f);
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
  if (m_movement.m_moveFlags & 0x2400) {
    CMovement::LogWrite("0x%016I64X: Immobilized\n", GetGUID());
  } else {
    m_movement.OnMoveStartLocal(eventTime, forward);
  }
}

void CGUnit_C::OnMoveStopLocal(unsigned long eventTime) {
  m_movement.OnMoveStopLocal(eventTime);
}

void CGUnit_C::OnStrafeStartLocal(unsigned long eventTime, int left) {
  if (!(m_unit->flags & 0x2000)) {
    OnMovementInitiated(0);
    if (m_movement.m_moveFlags & 0x2400) {
      CMovement::LogWrite("0x%016I64X: Immobilized\n", GetGUID());
    } else {
      m_movement.OnStrafeStartLocal(eventTime, left);
    }
  }
}

void CGUnit_C::OnStrafeStopLocal(unsigned long eventTime) {
  if (!(m_movement.m_moveFlags & 0x2000)) {
    m_movement.OnStrafeStopLocal(eventTime);
  }
}

void CGUnit_C::OnTurnStartLocal(unsigned long eventTime, int left) {
  OnMovementInitiated(0);
  if (m_unit->flags & 0x40000) {
    CMovement::LogWrite("0x%016I64X: Stunned\n", GetGUID());
  } else {
    m_movement.OnTurnStartLocal(eventTime, left);
  }
}

void CGUnit_C::OnTurnStopLocal(unsigned long eventTime) {
  m_movement.OnTurnStopLocal(eventTime);
}

void CGUnit_C::OnPitchStartLocal(unsigned long eventTime, int up) {
  m_movement.OnPitchStartLocal(eventTime, up);
}
void CGUnit_C::OnPitchStopLocal(unsigned long eventTime) {
  m_movement.OnPitchStopLocal(eventTime);
}

void CGUnit_C::OnSetFacingLocal(unsigned long eventTime, float facing) {
  OnMovementInitiated(1);
  m_movement.OnSetFacingLocal(eventTime, facing);
  if (m_unit->standState == 3 || m_unit->standState == 2) {
    ChangeStandState(0);
  }
}

void CGUnit_C::OnSetPitchLocal(unsigned long eventTime, float pitch) {
  m_movement.OnSetPitchLocal(eventTime, pitch);
}

void CGUnit_C::OnJumpLocal(unsigned long eventTime) {
  m_movement.OnJumpLocal(eventTime);
}

void CGUnit_C::OnAllSpeedChangeLocal(unsigned long eventTime, float speed) {
  m_movement.OnRunSpeedChange(eventTime, speed);
  m_movement.OnWalkSpeedChange(eventTime, speed);
  m_movement.OnSwimSpeedChange(eventTime, speed);
  UpdateBaseAnimation(0);
  UpdateMovementAnimSpeed(1, -1);

  CDataStore msg;
  BuildMovementUpdate(MSG_MOVE_SET_ALL_SPEED_CHEAT, &msg);
  msg.Put(speed);
  FATALASSERT(!msg.IsFinal());
  ClientServices_Send(&msg);
}

void CGUnit_C::OnTurnRateChangeLocal(unsigned long eventTime, float rate) {
  m_movement.OnTurnRateChange(eventTime, rate);

  CDataStore msg;
  BuildMovementUpdate(MSG_MOVE_SET_TURN_RATE_CHEAT, &msg);
  msg.Put(rate);
  FATALASSERT(!msg.IsFinal());
  ClientServices_Send(&msg);
}

void CGUnit_C::OnSetRunModeLocal(unsigned long eventTime, int run) {
  m_movement.OnSetRunModeLocal(eventTime, run);
}

void CGUnit_C::ToggleRunModeLocal(unsigned long eventTime) {
  m_movement.OnSetRunModeLocal(eventTime, (m_movement.m_moveFlags & 0x100) == 0);
}

void __fastcall ProcessLocalMoveEvent(unsigned int msgId) {
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(CGUnit_C::m_activeMover, __FILE__, __LINE__));
  FATALASSERT(unit);
  unit->ProcessLocalMoveEvent(static_cast<NETMESSAGE>(msgId));
}

void CGUnit_C::EnableWeaponTrail(const NTempest::CImVector &color, int fadeOutRate, unsigned int duration) {
  int *trails = reinterpret_cast<int *>(this) + 611;
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

SPELLEFFECTDESC::SPELLEFFECTDESC() : color(0) {
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

void CGUnit_C::SaveTrackingTarget(unsigned __int64 target, TRACKTYPE type, unsigned int snapToTargetOnClear) {
  FATALASSERT(!target || type <= TRACKTYPE_NUMTRACKTYPES);
  if (GetGUID() != ClntObjMgrGetActivePlayer() || ((!target && !s_trackingTarget) || target == GetGUID())) {
    return;
  }

  if (target && type == TRACKTYPE_FOLLOW && m_unit->channelSpell) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(280));
    return;
  }

  unsigned long currentTime = GetTickCount();
  if (target) {
    CGObject_C *object = ClntObjMgrObjectPtr(target, __FILE__, __LINE__);
    if (!object || !(object->GetType() & TYPE_UNIT)) {
      target = 0;
    } else {
      NTempest::C3Vector position;
      NTempest::C3Vector targetPosition;
      GetPosition(position);
      object->GetPosition(targetPosition);
      float dx = targetPosition.x - position.x;
      float dy = targetPosition.y - position.y;
      float dz = targetPosition.z - position.z;
      float maxDistance = s_trackTypeInfo[type].maxDistance;
      if (dx * dx + dy * dy + dz * dz > maxDistance * maxDistance) {
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(267));
        target = 0;
      } else if (!s_trackingTarget) {
        if (reinterpret_cast<const unsigned int *>(this)[32] & 0x100) {
          s_trackingFlags &= ~4u;
        } else {
          s_trackingFlags |= 4;
        }
      }
      s_trackingInterpStartTime = GetTickCount();
      s_trackLastCheckTime = s_trackingInterpStartTime;
    }
  } else if (s_trackingTarget) {
    unsigned int objectFlags = reinterpret_cast<const unsigned int *>(this)[32];
    if ((((s_trackingFlags >> 2) ^ static_cast<unsigned int>(~(objectFlags >> 8))) & 1) != 0) {
      OnSetRunModeLocal(currentTime, (s_trackingFlags >> 2) & 1);
    }

    if (snapToTargetOnClear || s_trackTypeInfo[s_trackingType].snapOnClear) {
      CGObject_C *object = ClntObjMgrObjectPtr(s_trackingTarget, __FILE__, __LINE__);
      if (object) {
        NTempest::C3Vector position;
        NTempest::C3Vector targetPosition;
        GetPosition(position);
        object->GetPosition(targetPosition);
        float facing = CalculateFacingTo(position, targetPosition);
        s_trackingFlags |= 2;
        OnSetFacingLocal(currentTime, facing);
        s_trackingFlags &= ~2u;
      }
    }

    if (s_trackTypeInfo[s_trackingType].snapOnClear && reinterpret_cast<const unsigned char *>(this)[128]) {
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

void CGUnit_C::ClearTrackingTarget(unsigned int snapToTargetOnClear) {
  if (s_trackingTarget && GetGUID() == ClntObjMgrGetActivePlayer()) {
    SaveTrackingTarget(0, s_trackingType, snapToTargetOnClear);
  }
}

unsigned __int64 CGUnit_C::GetTrackingTarget() {
  return GetGUID() == ClntObjMgrGetActivePlayer() ? s_trackingTarget : 0;
}

NAMEPLATEDESC::~NAMEPLATEDESC() {
  RecycleNameplateFrame(namePlate);
  namePlate = 0;
}

void CGUnit_C::OnMovementInitiated(unsigned int facingOnly) {
  if (TrackingTargetMoving() && !(s_trackingFlags & 2)) {
    SaveTrackingTarget(0, s_trackingType, 0);
  }
  if ((m_unit->flags & 0x400) && !facingOnly && GetGUID() == ClntObjMgrGetActivePlayer()) {
    CGGameUI::CloseLoot(1, 1);
  }
}

unsigned __int64 CGUnit_C::TrackingTargetMoving() {
  if (GetGUID() == ClntObjMgrGetActivePlayer()) {
    return s_trackingTarget;
  }
  return 0;
}

int __fastcall GetObjAnimFlags(int unitAnimFlags) {
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
