#pragma once

#include <Model/IModel.h>

#include "Object/ObjectClient/AnimCompiles.h"
#include "Object/ObjectClient/IUnitEffects.h"
#include "Object/ObjectClient/Object_C.h"
#include "Object/MovementData.h"
#include "Object/Unit.h"
#include "Object/UnitCombat.h"
#include "Net/NetClient/NetClient.h"

#include <Tempest/c3ivector.h>

class CreatureDisplayInfoRec;
class CreatureModelDataRec;
class NPCSoundsRec;
struct Sound;
class UnitBloodRec;
class CGItem_C;
class CGGameObject_C;
class CGGameObject_C_Type_Chair;
class CGCamera;
class CGInputControl;
struct ItemEnchantment;
struct HPLAYERNAME__;
typedef HPLAYERNAME__ *HPLAYERNAME;
struct HCHARGEOSET__;
typedef HCHARGEOSET__ *HCHARGEOSET;
template <class T>
class TSStackArray;

void UnitUpdateMovementAnim(const DWORDLONG &unit);
int  UnitHealthUpdateHandler(DWORDLONG unit, UINT offset, UINT bytes, LPCVOID oldValue, LPVOID param);
void OnMoveUpdate(DWORDLONG unit, DWORD eventTime);
int  MoveHeartBeatHandler(LPCVOID packetData, LPVOID param);
int  OnUnitCombatEvent(LPVOID param, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
int  Player_C_AppFocusMovementHandler(int focus);
int  OnUpdateInventoryComponent(DWORDLONG guid, UINT offset, UINT bytes, LPCVOID prevValue, LPVOID param);

enum UNITEFFECTSPECIALS {
  SPECIALEFFECT_NONE = -1,
  SPECIALEFFECT_LOOTART = 0,
  SPECIALEFFECT_LEVELUP = 1,
  SPECIALEFFECT_FOOTSTEPSPRAYSNOW = 2,
  SPECIALEFFECT_FOOTSTEPSPRAYSNOWWALK = 3,
  SPECIALEFFECT_FOOTSTEPDIRT = 4,
  SPECIALEFFECT_FOOTSTEPDIRTWALK = 5,
  SPECIALEFFECT_COLDBREATH = 6,
  SPECIALEFFECT_UNDERWATERBUBBLES = 7,
  SPECIALEFFECT_COMBATBLOODSPURTFRONT = 8,
  SPECIALEFFECT_UNUSED = 9,
  SPECIALEFFECT_COMBATBLOODSPURTBACK = 10,
  SPECIALEFFECT_HITSPLATPHYSICALSMALL = 11,
  SPECIALEFFECT_HITSPLATPHYSICALBIG = 12,
  SPECIALEFFECT_HITSPLATHOLYSMALL = 13,
  SPECIALEFFECT_HITSPLATHOLYBIG = 14,
  SPECIALEFFECT_HITSPLATFIRESMALL = 15,
  SPECIALEFFECT_HITSPLATFIREBIG = 16,
  SPECIALEFFECT_HITSPLATNATURESMALL = 17,
  SPECIALEFFECT_HITSPLATNATUREBIG = 18,
  SPECIALEFFECT_HITSPLATFROSTSMALL = 19,
  SPECIALEFFECT_HITSPLATFROSTBIG = 20,
  SPECIALEFFECT_HITSPLATSHADOWSMALL = 21,
  SPECIALEFFECT_HITSPLATSHADOWBIG = 22,
  SPECIALEFFECT_COMBATBLOODSPURTFRONTLARGE = 23,
  SPECIALEFFECT_COMBATBLOODSPURTBACKLARGE = 24,
  SPECIALEFFECT_FIZZLEPHYSICAL = 25,
  SPECIALEFFECT_FIZZLEHOLY = 26,
  SPECIALEFFECT_FIZZLEFIRE = 27,
  SPECIALEFFECT_FIZZLENATURE = 28,
  SPECIALEFFECT_FIZZLEFROST = 29,
  SPECIALEFFECT_FIZZLESHADOW = 30,
  SPECIALEFFECT_COMBATBLOODSPURTGREENFRONT = 31,
  SPECIALEFFECT_COMBATBLOODSPURTGREENFRONTLARGE = 32,
  SPECIALEFFECT_COMBATBLOODSPURTGREENBACK = 33,
  SPECIALEFFECT_COMBATBLOODSPURTGREENBACKLARGE = 34,
  SPECIALEFFECT_FOOTSTEPSPRAYWATER = 35,
  SPECIALEFFECT_FOOTSTEPSPRAYWATERWALK = 36,
  SPECIALEFFECT_CHARACTERSHAPESHIFT = 37,
  SPECIALEFFECT_COMBATBLOODSPURTBLACKFRONT = 38,
  SPECIALEFFECT_COMBATBLOODSPURTBLACKFRONTLARGE = 39,
  SPECIALEFFECT_COMBATBLOODSPURTBLACKBACK = 40,
  SPECIALEFFECT_COMBATBLOODSPURTBLACKBACKLARGE = 41,
  SPECIALEFFECT_RES_EFFECT = 42,
  NUM_UNITEFFECTSPECIALS = 43
};

enum ANIM_STATE {
  INVALID_ANIM_STATE = -1,
  ANIM_STATE_NONE = 0,
  ANIM_STATE_DEAD = 1,
  ANIM_STATE_SPELL = 2,
  ANIM_STATE_IDLE = 3,
  ANIM_STATE_STOP = 4,
  ANIM_STATE_WALK = 5,
  ANIM_STATE_RUN = 6,
  ANIM_STATE_WALK_BACKWARDS = 7,
  ANIM_STATE_STRAFE_WALK_LEFT = 8,
  ANIM_STATE_STRAFE_WALK_RIGHT = 9,
  ANIM_STATE_STRAFE_RUN_LEFT = 10,
  ANIM_STATE_STRAFE_RUN_RIGHT = 11,
  ANIM_STATE_DIAG_WALK_LEFT = 12,
  ANIM_STATE_DIAG_WALK_RIGHT = 13,
  ANIM_STATE_DIAG_RUN_LEFT = 14,
  ANIM_STATE_DIAG_RUN_RIGHT = 15,
  ANIM_STATE_DIAG_BACKWARDS_LEFT = 16,
  ANIM_STATE_DIAG_BACKWARDS_RIGHT = 17,
  ANIM_STATE_TURNING_LEFT = 18,
  ANIM_STATE_TURNING_RIGHT = 19,
  ANIM_STATE_SWIM_IDLE = 20,
  ANIM_STATE_SWIM = 21,
  ANIM_STATE_SWIM_STRAFE_LEFT = 22,
  ANIM_STATE_SWIM_STRAFE_RIGHT = 23,
  ANIM_STATE_SWIM_BACKWARDS = 24,
  ANIM_STATE_KNEEL = 25,
  ANIM_STATE_RISE = 26,
  ANIM_STATE_WOUND = 27,
  ANIM_STATE_CRITICALWOUND = 28,
  ANIM_STATE_STUN = 29,
  ANIM_STATE_ATTACK_HIT = 30,
  ANIM_STATE_ATTACK_READY = 31,
  ANIM_STATE_ATTACK_MISS = 32,
  ANIM_STATE_ATTACKOFF_HIT = 33,
  ANIM_STATE_ATTACKOFF_MISS = 34,
  ANIM_STATE_PARRY = 35,
  ANIM_STATE_DODGE = 36,
  ANIM_STATE_SPELLPRECAST = 37,
  ANIM_STATE_SPELLCAST = 38,
  ANIM_STATE_NPC_OBSOLETE = 39,
  ANIM_STATE_BLOCK = 40,
  ANIM_STATE_JUMPING = 41,
  ANIM_STATE_JUMP_LANDING = 42,
  ANIM_STATE_FALLING = 43,
  ANIM_STATE_LOOTBEGIN = 44,
  ANIM_STATE_LOOTEND = 45,
  ANIM_STATE_EMOTE = 46,
  ANIM_STATE_SPELLIMPACT = 47,
  ANIM_STATE_MOUNTED = 48,
  ANIM_STATE_SPECIALMOUNTANIM = 49,
  ANIM_STATE_SITDOWN = 50,
  ANIM_STATE_SITTING = 51,
  ANIM_STATE_SITUP = 52,
  ANIM_STATE_SLEEPDOWN = 53,
  ANIM_STATE_SLEEPING = 54,
  ANIM_STATE_SLEEPUP = 55,
  ANIM_STATE_SITCHAIRLOW = 56,
  ANIM_STATE_SITCHAIRMEDIUM = 57,
  ANIM_STATE_SITCHAIRHIGH = 58,
  ANIM_STATE_KNEELDOWN = 59,
  ANIM_STATE_KNEELING = 60,
  ANIM_STATE_KNEELUP = 61,
  ANIM_STATE_CHANNELSPELL = 62,
  ANIM_STATE_SPELLAURA = 63,
  NUM_ANIMSTATES = 64,
  ANIM_STATE_FIRST_STRAFE = ANIM_STATE_STRAFE_WALK_LEFT,
  ANIM_STATE_LAST_STRAFE = ANIM_STATE_DIAG_BACKWARDS_RIGHT
};

enum WORLDTEXTMISSTYPE {
  WORLDTEXTMISS_EVADED = 0,
  WORLDTEXTMISS_DODGED = 1,
  WORLDTEXTMISS_PARRIED = 2,
  WORLDTEXTMISS_BLOCKED = 3,
  WORLDTEXTMISS_DEFLECTED = 4,
  WORLDTEXTMISS_IMMUNE = 5,
  WORLDTEXTMISS_TEMPIMMUNE = 6,
  WORLDTEXTMISS_PHYSICAL = 7,
  WORLDTEXTMISS_RESIST = 8,
  WORLDTEXTMISS_ABSORBED = 9,
  WORLDTEXTMISS_NUMTYPES = 10
};

enum BLOODSPURTLOCATION {
  BLOODSPURT_FRONT = 0,
  BLOODSPURT_BACK = 1,
  NUM_BLOODSPURTLOCATIONS = 2
};

enum UNITEFFECTATTACHPPOINT {
  UNITEFFECT_INVALID = -1,
  UNITEFFECT_ATTACHBASE = 0,
  UNITEFFECT_ATTACHHEAD = 1,
  UNITEFFECT_ATTACHLEFTHAND = 2,
  UNITEFFECT_ATTACHRIGHTHAND = 3,
  UNITEFFECT_ATTACHNONE = 4,
  UNITEFFECT_ATTACHBREATH = 5,
  UNITEFFECT_ATTACHCHEST = 6,
  UNITEFFECT_ATTACHSPECIAL1 = 7,
  UNITEFFECT_ATTACHSPECIAL2 = 8,
  UNITEFFECT_ATTACHSPECIAL3 = 9,
  UNITEFFECT_ATTACHCHESTBLOODBACK = 10,
  UNITEFFECT_ATTACHCHESTBLOODFRONT = 11,
  NUM_UNITEFFECTATTACHPOINTS = 12
};

enum GEOCOMPONENTLINKS {
  ATTACH_NONE = -1,
  ATTACH_SHIELD = 0,
  ATTACH_HANDR = 1,
  ATTACH_HANDL = 2,
  ATTACH_ELBOWR = 3,
  ATTACH_ELBOWL = 4,
  ATTACH_SHOULDERR = 5,
  ATTACH_SHOULDERL = 6,
  ATTACH_KNEER = 7,
  ATTACH_KNEEL = 8,
  ATTACH_HIPR = 9,
  ATTACH_HIPL = 10,
  ATTACH_HELM = 11,
  ATTACH_BACK = 12,
  ATTACH_SHOULDERFLAPR = 13,
  ATTACH_SHOULDERFLAPL = 14,
  ATTACH_TORSOBLOODFRONT = 15,
  ATTACH_TORSOBLOODBACK = 16,
  ATTACH_BREATH = 17,
  ATTACH_PLAYERNAME = 18,
  ATTACH_UNITEFFECT_BASE = 19,
  ATTACH_UNITEFFECT_HEAD = 20,
  ATTACH_UNITEFFECT_SPELLLEFTHAND = 21,
  ATTACH_UNITEFFECT_SPELLRIGHTHAND = 22,
  ATTACH_UNITEFFECT_SPECIAL1 = 23,
  ATTACH_UNITEFFECT_SPECIAL2 = 24,
  ATTACH_UNITEFFECT_SPECIAL3 = 25,
  ATTACH_SHEATH_MAINHAND = 26,
  ATTACH_SHEATH_OFFHAND = 27,
  ATTACH_SHEATH_SHIELD = 28,
  ATTACH_PLAYERNAMEMOUNTED = 29,
  ATTACH_LARGEWEAPONLEFT = 30,
  ATTACH_LARGEWEAPONRIGHT = 31,
  ATTACH_HIPWEAPONLEFT = 32,
  ATTACH_HIPWEAPONRIGHT = 33,
  ATTACH_TORSOSPELL = 34,
  ATTACH_HANDARROW = 35,
  NUM_ATTACH_SLOTS = 36
};

enum QUEST_GIVER_STATUS {
  QUEST_GIVER_NONE = 0,
  QUEST_GIVER_TRIVIAL = 1,
  QUEST_GIVER_FUTURE = 2,
  QUEST_GIVER_REWARD = 3,
  QUEST_GIVER_QUEST = 4,
  QUEST_GIVER_NUMITEMS = 5
};

enum INTERACTICONTYPE {
  INTERACTICON_NONE = 0,
  INTERACTICON_NORMAL = 1,
  INTERACTICON_COMPLETION = 2,
  INTERACTICON_FUTURE = 3,
  INTERACTICON_TAXINODE = 4,
  INTERACTICON_BINDER = 5,
  INTERACTICON_NUMITEMS = 6
};

enum TALKANIMATION {
  TALKANIM_TALK = 0,
  TALKANIM_QUESTION = 1,
  TALKANIM_EXCLAMATION = 2,
  TALKANIM_SHOUT = 3,
  TALKANIM_LAUGH = 4,
  TALKANIM_NUMTALKANIMS = 5
};

enum UNITAFFILIATION {
  AFFILIATION_YOURSELF = 0,
  AFFILIATION_YOURPET = 1,
  AFFILIATION_PARTYMEMBER = 2,
  AFFILIATION_OTHER = 3,
  AFFILIATION_YOURCONTROLLER = 4,
  AFFILIATION_NUMAFFILIATIONS = 5
};
enum COMBATHAND {
  COMBAT_MAINHAND = 0,
  COMBAT_OFFHAND = 1,
  NUMHANDS = 2
};

enum VIRTUAL_MONSTER_SLOT {
  VIRTUAL_MONSTER_SLOT_MAINHAND = 0,
  VIRTUAL_MONSTER_SLOT_OFFHAND = 1,
  VIRTUAL_MONSTER_SLOT_RANGED = 2,
  NUM_VIRTUAL_MONSTER_SLOTS = 3
};

extern const VIRTUAL_MONSTER_SLOT g_monsterHands[NUMHANDS];

enum WEAPONSWING_SOUNDTYPES {
  WEAPONSWING_UNUSED = -1,
  WEAPONSWING_LIGHT = 0,
  WEAPONSWING_MEDIUM = 1,
  WEAPONSWING_HEAVY = 2,
  NUM_WEAPONSWINGSOUNDTYPES = 3
};
class CreatureStats_C;
class CGNamePlateFrame;
class CGWorldFrame;
class CSimpleTexture;
class CreatureDisplayInfoExtraRec;
class CreatureSoundDataRec;
class UnitBloodLevelsRec;
class ItemDisplayInfoRec;
class ItemVisualsRec;
class ItemStats;
class SpellRec;
class SpellVisualRec;
class SpellVisualKitRec;
class SpellVisualEffectNameRec;
class SkillLineAbilityRec;
class CGUnit_C;
struct HTEXCOMPONENT__;
typedef HTEXCOMPONENT__ *HTEXCOMPONENT;
struct ACTIVEAURAINFO;
struct ANIMENDDATA {
  DWORDLONG       unit;
  ANIMENUMERATION animID;
};
NODEDECL(IMPACTEFFECTDESC) {
  DWORDLONG                victim;
  DWORDLONG                attacker;
  const SpellVisualKitRec *impactKit;
  int                      spellID;

  IMPACTEFFECTDESC() : victim(0), impactKit(0), spellID(0) {
  }
  IMPACTEFFECTDESC(const IMPACTEFFECTDESC &);
  ~IMPACTEFFECTDESC();
  void Set(DWORDLONG a, DWORDLONG v, const SpellVisualKitRec *i, int s);
};
struct ANIMQUEUENODE;
struct ATTACKROUNDINFO;
struct BLOODSPLATNODE;

enum NPCSOUNDS {
  NPCSOUND_HELLO = 0,
  NPCSOUND_GOODBYE = 1,
  NPCSOUND_PISSED = 2,
  NPCSOUND_ACK = 3,
  NUM_NPCSOUNDS = 4
};
struct QUESTGIVEREMOTENODE {
  UINT delay;
  UINT emoteID;

  QUESTGIVEREMOTENODE() {
  }
};
struct LightningObject;
struct FishingLineObject;

struct DEBUGHITROLLINFO {
  ATTACKROUNDINFO attackInfo;
  UINT            attackFlags;
  float           range;

  DEBUGHITROLLINFO() : attackFlags(0), range(0.0f) {
  }
  DEBUGHITROLLINFO(const DEBUGHITROLLINFO &);
};

enum PUREMOUNTFADEMODE {
  PUREMOUNTFADE_IN = 0,
  PUREMOUNTFADE_OUT = 1
};

NODEDECL(SPELLEFFECTDESC) {
  const SpellVisualKitRec *kitPtr;
  NTempest::CImVector      color;
  float                    scale;
  UINT                     startTime;
  UINT                     fadeInTime;
  UINT                     fadeOutTime;
  UINT                     endTime;
  UINT                     curTime;
  float                    period;
  int                      standAnim;
  int                      walkAnim;
  bool                     isOneShot;
  LightningObject         *lightningObjs[3];

  SPELLEFFECTDESC();
  ~SPELLEFFECTDESC();
  void  ClearLightningObjects();
  float CalcScalar();
};

enum SPELLPROC_ACTION {
  SPELLPROCADD = 0,
  SPELLPROCREMOVE = 1,
  SPELLPROCREFRESH = 2,
  SPELLPROCUPDATE = 3
};

enum EMOTESPECPROCS {
  EMOTESPECPROC_NONE = 0,
  EMOTESPECPROC_STANDSTATEHANDLER = 1,
  EMOTESPECPROC_EMOTESTATEHANDLER = 2,
  EMOTESPECPROC_NUMSPECPROCS = 3
};

struct ATTACHMENTMODELINFO {
  HMODEL model;
  int    attachmentPoint;
  int    currentLink;

  ATTACHMENTMODELINFO() : model(0), attachmentPoint(0), currentLink(-1) {
  }
  ~ATTACHMENTMODELINFO() {
    FATALASSERT(!model);
  }
  void ClearAttachmentFromModel(HMODEL charModel, HMODEL paperDollModel);
};

struct ACTIVEATTACHMENTINFO {
  int                       inventoryType;
  int                       flags;
  int                       invSlot;
  int                       sheathAttachmentSlot;
  const ItemDisplayInfoRec *displayInfo;
  const ItemVisualsRec     *enchantmentVisual;
  ATTACHMENTMODELINFO       modelInfo[2];

  __forceinline ACTIVEATTACHMENTINFO() : inventoryType(0), flags(0), invSlot(-1), sheathAttachmentSlot(-1), displayInfo(0), enchantmentVisual(0) {
  }
  __forceinline ~ACTIVEATTACHMENTINFO() {
  }
  void Clear();
  void ClearAttachmentFromModel(HMODEL charModel, HMODEL paperDollModel);
  void Hide(CGUnit_C *unitPtr, HMODEL charModel, HMODEL paperDollModel, bool hide);
};

enum UNITSOUNDTYPE {
  UNITSOUNDTYPE_EXERTION = 0,
  UNITSOUNDTYPE_EXERTIONCRITICAL = 1,
  UNITSOUNDTYPE_INJURY = 2,
  UNITSOUNDTYPE_INJURYCRITICAL = 3,
  UNITSOUNDTYPE_DEATH = 4,
  UNITSOUNDTYPE_STUN = 5,
  UNITSOUNDTYPE_STAND = 6,
  UNITSOUNDTYPE_DEATHTHUD = 7,
  UNITSOUNDTYPE_FOOTFALL = 8,
  UNITSOUNDTYPE_AGGRO = 9,
  UNITSOUNDTYPE_WINGFLAP = 10,
  UNITSOUNDTYPE_ALERT = 11,
  UNITSOUNDTYPE_INJURYCRUSHINGBLOW = 12,
  UNITSOUNDTYPE_WINGGLIDE = 13,
  UNITSOUNDTYPE_JUMPSTART = 14,
  UNITSOUNDTYPE_JUMPEND = 15,
  NUM_UNITSOUNDTYPES = 16
};

enum AI_REACTION {
  AI_REACT_ALERT = 0,
  AI_REACT_FRIENDLY = 1,
  AI_REACT_HOSTILE = 2,
  AI_REACT_AFRAID = 3,
  NUM_AI_REACTIONS = 4
};

enum WEAPONMODE {
  WEAPONMODE_NORMALMODE = 0,
  WEAPONMODE_SHEATHEDMODE = 1,
  WEAPONMODE_RANGEDMODE = 2,
  WEAPONMODE_NUMMODES = 3
};

enum SHEATHEREASONS {
  SHEATHE_PLAYEREXPLICIT = 0,
  SHEATHE_SPELLS = 1,
  SHEATHE_STANDSTATE = 2,
  SHEATHE_BASEANIM = 3,
  SHEATHE_TORSOANIM = 4,
  SHEATHE_RANGED = 5,
  SHEATHE_TALKEMOTE = 6,
  SHEATHE_PRECAST = 7,
  SHEATHE_CHANNELLING = 8,
  SHEATHE_NUMREASONS = 9
};

enum UNIT_REACTION {
  UNIT_REACTION_HATED = 0,
  UNIT_REACTION_HOSTILE = 1,
  UNIT_REACTION_UNFRIENDLY = 2,
  UNIT_REACTION_NEUTRAL = 3,
  UNIT_REACTION_AMIABLE = 4,
  UNIT_REACTION_FRIENDLY = 5,
  UNIT_REACTION_REVERED = 6,
  NUM_UNIT_REACTIONS = 7
};

enum TRACKTYPE {
  TRACKTYPE_SPELLPRECAST = 0,
  TRACKTYPE_SPELLCHANNEL = 1,
  TRACKTYPE_FOLLOW = 2,
  TRACKTYPE_NUMTRACKTYPES = 3
};

struct AuraVisual {
  AuraVisual() : flags(0), spellID(0), effectID(0), theModel(0) {
  }

  void SetSpellID(UINT id) {
    spellID = id;
  }
  UINT GetSpellID() const {
    return spellID;
  }
  bool HasArt() const {
    return flags & 1;
  }
  bool IsWorldModel() const {
    return flags & 2;
  }
  UINT GetEffect() {
    return effectID;
  }
  void SetEffect(UINT effect) {
    effectID = effect;
  }
  void Set(AuraVisual &visual) {
    Clear();
    flags = visual.flags;
    spellID = visual.spellID;
    effectID = visual.effectID;
    theModel = visual.theModel;
    visual.flags &= ~1;
    visual.theModel = 0;
  }
  void Clear();
  void SetModel(HMODEL model);
  void SetWorldObject(DWORD object);
  void SetPermanent(bool permanent) {
    if (permanent) {
      flags |= 4;
    } else {
      flags &= ~4;
    }
  }
  HMODEL Model() const {
    return theModel;
  }
  HMODEL GetModel();

 private:
  int  flags;
  UINT spellID;
  UINT effectID;
  union {
    HMODEL theModel;
    DWORD  obj;
  };
};

NODEDECL(ACTIVEAURAINFO) {
  int                      auraSlot;
  const SpellVisualKitRec *stateKitRec;

  ACTIVEAURAINFO() {
  }
  ACTIVEAURAINFO(const ACTIVEAURAINFO &);
  ~ACTIVEAURAINFO() {
  }
};

struct CGUnitData {
  DWORDLONG       charm;
  DWORDLONG       summon;
  DWORDLONG       charmedBy;
  DWORDLONG       summonedBy;
  DWORDLONG       createdBy;
  DWORDLONG       target;
  DWORDLONG       comboTarget;
  DWORDLONG       channelObject;
  int             health;
  int             power[4];
  int             maxHealth;
  int             maxPower[4];
  int             level;
  int             factionTemplate;
  BYTE            race;
  BYTE            classId;
  BYTE            sex;
  BYTE            displayPower;
  int             stats[5];
  int             baseStats[5];
  UINT            virtualItemDisplay[3];
  VirtualItemInfo virtualItemInfo[3];
  UINT            flags;
  UINT            coinage;
  int             auras[56];
  BYTE            auraFlags[28];
  UINT            auraState;
  int             modDamageDone[6];
  int             modDamageTaken[6];
  int             modCreatureDamageDone[8];
  UINT            attackRoundBaseTime[2];
  int             resistances[6];
  float           boundingRadius;
  float           combatReach;
  float           weaponReach;
  int             displayID;
  int             mountDisplayID;
  WORD            minDamage;
  WORD            maxDamage;
  int             resistanceBuffModsPositive[6];
  int             resistanceBuffModsNegative[6];
  int             resistanceItemMods[6];
  BYTE            standState;
  BYTE            npcFlags;
  BYTE            shapeshiftForm;
  BYTE            weaponMode;
  UINT            petNumber;
  UINT            petNameTimestamp;
  UINT            petExperience;
  UINT            petNextLevelExperience;
  UINT            dynamicFlags;
  UINT            emoteState;
  int             channelSpell;
  int             modCastingSpeed;
  int             createdBySpell;
  BYTE            comboPoints;
  BYTE            bytepad1;
  BYTE            bytepad2;
  BYTE            bytepad3;
  UINT            pad;
};

class CGUnit {
  friend class CGCamera;
  friend class CGGameObject_C_Type_Chair;
  friend class CGInputControl;
  friend void OnMoveUpdate(DWORDLONG unit, DWORD eventTime);
  friend int  MoveHeartBeatHandler(LPCVOID packetData, LPVOID param);
  friend int  Player_C_AppFocusMovementHandler(int focus);

 public:
  static UINT               GetDataSize();
  static UINT               GetBaseOffset();
  static __forceinline UINT TotalFields() {
    return 184;
  }
  static UINT             GetUpdateMaskBytes();
  static UINT             GetUpdateMaskBlocks();
  virtual UNITAFFILIATION GetGUIDAffiliation(DWORDLONG unit) const;

  UINT              GetUnitFlags() const;
  BYTE              GetUnitNPCFlags() const;
  BYTE              IsAlive() const;
  BYTE              IsDead() const;
  int               GetHealth() const;
  float             GetHealthPercent() const;
  int               GetPower(POWER_TYPE powerType) const;
  int               GetMaxPower(POWER_TYPE powerType) const;
  float             GetPowerPercent(POWER_TYPE powerType) const;
  POWER_TYPE        GetDisplayPower() const;
  __forceinline int GetMaxHealth() const {
    return m_unit->maxHealth;
  }
  UINT                   GetMoney() const;
  int                    GetLevel() const;
  UINT                   GetMinDamage() const;
  UINT                   GetMaxDamage() const;
  int                    IsCombatLoggingActive() const;
  int                    GetCurrentStat(UINT stat) const;
  int                    GetEffectiveStat(UINT stat) const;
  int                    GetBaseStat(UINT stat) const;
  int                    GetResistance(UINT school) const;
  int                    GetEffectiveResistance(UINT school) const;
  int                    GetResistanceBuffModPositive(UINT school) const;
  int                    GetResistanceBuffModNegative(UINT school) const;
  int                    GetResistanceItemMod(UINT school) const;
  UINT                   GetRace() const;
  UINT                   GetClass() const;
  UNIT_SEX               GetSex() const;
  int                    GetModDamageDone(UINT school) const;
  int                    GetModDamageTaken(UINT school) const;
  int                    GetModCreatureDamageDone(UINT creatureType) const;
  const DWORDLONG       &GetCharm() const;
  const DWORDLONG       &GetSummon() const;
  const DWORDLONG       &GetControlledGUID() const;
  const DWORDLONG       &GetCharmedBy() const;
  BYTE                   IsCharmedBy(const DWORDLONG &guid) const;
  BYTE                   IsCharmed() const;
  const DWORDLONG       &GetSummonedBy() const;
  BYTE                   IsSummonedBy(const DWORDLONG &guid) const;
  BYTE                   IsSummoned() const;
  const DWORDLONG       &GetCreatedBy() const;
  BYTE                   IsCreatedBy(const DWORDLONG &guid) const;
  BYTE                   IsCreated() const;
  int                    GetCreatedBySpell() const;
  const DWORDLONG       &GetControlGUID() const;
  const DWORDLONG       &GetOwnerGUID() const;
  BYTE                   IsPossessedBy(const DWORDLONG &guid) const;
  BYTE                   IsPossessed() const;
  float                  GetBoundingRadius() const;
  float                  GetCombatReach() const;
  int                    GetDisplayID() const;
  UINT                   GetMonsterItemDisplay(UINT slot) const;
  const VirtualItemInfo *GetMonsterItemInfo(UINT slot) const;
  UINT                   GetShapeshiftForm() const;
  UINT                   GetShapeshiftBit() const;
  BYTE                   IsChannelling() const;
  int                    GetChannelSpell() const;
  DWORDLONG              GetChannelObject() const;
  int                    ModCastSpeed() const;
  DWORDLONG              GetComboTarget() const;
  UINT                   GetComboPoints() const;
  void                   GetPosition(NTempest::C3Vector &position) const;
  NTempest::C3Vector     GetPosition() const;
  NTempest::C3Vector     GetRawPosition() const;
  float                  GetFacing() const;
  float                  GetRawFacing() const;
  void                   GetAnchorPosition(NTempest::C3Vector &position) const;
  NTempest::C3Vector     GetAnchorPosition() const;
  float                  GetAnchorFacing() const;
  float                  GetPitch() const;
  NTempest::C3Vector     GetGroundNormal() const;
  float                  GetRunSpeed() const;
  float                  GetWalkSpeed() const;
  float                  GetSwimSpeed() const;
  float                  GetTurnRate() const;
  UINT                   GetMoveFlags() const {
    return m_move.GetMoveFlags();
  }
  int   IsInMotion() const;
  int   IsMovingOrTurning() const;
  int   IsMovingOrFalling() const;
  int   IsMoving() const;
  int   IsMovingOrStrafing() const;
  int   IsMovingTurningOrStrafing() const;
  int   IsMovingStrafingOrFalling() const;
  int   IsMovingForward() const;
  int   IsMovingBackwards() const;
  int   IsWalking() const;
  int   IsRunning() const;
  int   IsTurning() const;
  int   IsTurningLeft() const;
  int   IsTurningRight() const;
  int   IsStrafingLeft() const;
  int   IsStrafingRight() const;
  int   IsStrafing() const;
  int   IsFalling() const;
  int   IsImmobilized() const;
  int   Moved() const;
  DWORD GetMoveStartTime() const {
    return m_move.GetMoveStartTime();
  }
  NTempest::C3Vector GetRedirection() const;
  int                MoveTimeIsValid() const;
  int                IsSwimming() const;
  int                IsSwimmingOrFalling() const;
  int                IsMovingStrafingOrSwimming() const;
  int                IsMovingStrafingFallingOrSwimming() const;
  float              GetCollisionBoxHeight() const;
  int                IgnoresCollision() const;
  int                IsHalted() const;
  void               BuildMovementUpdate(CDataStore *msg) const;
  float              LinearDistanceSquared(const NTempest::C3Vector &position) const;
  int                GetAura(int index) const;
  BYTE               GetAuraFlags(int index) const;
  UINT               GetAuraState() const;
  BYTE               HasAuraState(UINT state) const;
  BYTE               IsDisconnected() const;
  BYTE               IsSpawning() const;
  BYTE               IsClientLocked() const;
  BYTE               IsOnTaxi() const;
  BYTE               IsPlayerControlled() const;
  BYTE               IsPlusMob() const;
  BYTE               IsBeastmaster() const;
  BYTE               IsImmunePC() const;
  BYTE               IsImmuneNPC() const;
  BYTE               IsLooting() const;
  BYTE               IsInCombat() const;
  BYTE               IsMounted() const {
    return (m_unit->flags >> 13) & 1;
  }
  BYTE      IsPureMountActive() const;
  BYTE      IsPureMountMounted() const;
  BYTE      IsFeignDeath() const;
  BYTE      IsStealthed() const;
  BYTE      IsInvisible() const;
  BYTE      IsConfused() const;
  BYTE      IsFleeing() const;
  BYTE      IsAffectingCombat() const;
  BYTE      IsMerchant() const;
  BYTE      IsQuestGiver() const;
  BYTE      IsTaxiNode() const;
  BYTE      IsTrainer() const;
  BYTE      IsBinder() const;
  BYTE      IsBanker() const;
  BYTE      IsNpcPetition() const;
  BYTE      IsTabardVendor() const;
  BYTE      IsGuildRegistrar() const;
  BYTE      IsNPC() const;
  int       GetMountDisplayID() const;
  DWORDLONG GetTarget() const;
  UINT      GetStandState() const;
  int       StandStateValid(UNITSTANDSTATE newState) const;
  BYTE      IsSitting() const;
  BYTE      IsSleeping() const;
  UINT      GetEmoteState() const;
  UINT      GetPetNumber() const;
  UINT      GetPetNameTimestamp() const;
  BYTE     *GetData(UINT offset);
  void      SetStorage(DWORD *storage) {
    m_unit = reinterpret_cast<CGUnitData *>(storage);
  }
  UINT       GetAttackRoundTime(COMBATHAND hand) const;
  WEAPONMODE GetWeaponMode() const;
  BYTE       IsUsingRangedWeapon() const;
  BYTE       GetSheathed() const;
  void       SetWaterSurfaceElevation(float elevation);

 protected:
  CGUnit(const CGUnit &);
  CGUnit(DWORD *storage, const NTempest::C3Vector &position, float facing, const DWORDLONG &guid);
  ~CGUnit();

  CGUnitData       *Unit();
  const CGUnitData *Unit() const;

 private:
  CGUnit &operator=(const CGUnit &);

 protected:
  CGUnitData   *m_unit;
  CMovementData m_move;
};

class CGUnit_C : public CGObject_C, public CGUnit {
  friend int OnUpdateInventoryComponent(DWORDLONG guid, UINT offset, UINT bytes, LPCVOID prevValue, LPVOID param);
  friend class CGInputControl;
  friend class CGObject_C;
  friend class CGPlayer_C;
  friend struct ACTIVEATTACHMENTINFO;
  friend int UnitHealthUpdateHandler(DWORDLONG unit, UINT offset, UINT bytes, LPCVOID oldValue, LPVOID param);
  friend int OnUnitCombatEvent(LPVOID param, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);

 public:
  CGUnit_C(DWORD *storage, DWORD eventTime, CClientObjCreate *init);
  ~CGUnit_C();
  virtual void Disable(int shutdown);
  virtual void Reenable();
  virtual void PostReenable();
  virtual void PreRender(int currentTime, float elapsed);
  virtual void PreAnimate(CGWorldFrame *worldFrame);

  // CGUnit_C virtuals, in primary-vtable order.
  virtual void      GetAFKText(char *buffer, int size) const;
  virtual void      GetDNDText(char *buffer, int size) const;
  virtual void      GetGMText(char *buffer, int size) const;
  virtual DWORDLONG GetLocalTarget() const;
  virtual void      HandleSpellEventSound();
  virtual void      CombatLoggingFlagChanged();
  virtual DWORDLONG GetUnitBeingLooted() const;
  virtual void      StopAttack();
  virtual void      OnAttackStart(DWORDLONG victim);
  virtual void      OnAttackStop(DWORDLONG previousTarget, int nowDead);
  virtual void      OnDeath();
  virtual void      OnDeathAnimate();
  virtual void      OnGetAttacked(DWORDLONG attacker);
  virtual void      OnBadAttackFacing(DWORDLONG victimGUID);
  virtual void      OnBadAttackTarget(DWORDLONG victim);
  virtual void      OnBadAttackPosition(DWORDLONG victimGUID, float range);
  virtual void      OnNotStanding(DWORDLONG victim);
  virtual void      UnitHit(VICTIMSTATES state, DWORDLONG attacker);
  virtual void      OnAttackerStateChange(const ATTACKROUNDINFO &roundInfo);
  virtual void      HandleMirrorTimerDamage(const MIRRORTIMERDAMAGE &log);

 protected:
  virtual int  QueueAnim(ANIMQUEUETYPE type, const ATTACKROUNDINFO *roundInfo);
  virtual void ProcessDiscardedAnim(ANIMQUEUENODE *node, bool doNotProcess);
  virtual void ProcessAnim(ANIMQUEUENODE *node);

 public:
  virtual void PlayUnitSound(UNITSOUNDTYPE soundType, int alwaysPlay) const;
  virtual void PlayFoleySound() const;

 protected:
  virtual UINT GetImpactType() const;

 public:
  virtual const VirtualItemInfo *GetDefendingItem() const;
  virtual void                   PlayDeathThudCameraShake() const;
  virtual void                   LootAnimEndHandler();
  virtual void                   RestoreUnit();
  virtual void                   UpdateBaseAnimation(UINT flags);
  virtual void                   StartSpellFizzleTimer(int spellID, UINT castingTime, int animSet);
  virtual void                   SetTorsoAnimState(UINT newState);
  virtual void                   SetBaseAnimState(UINT newState);

 protected:
  virtual UINT DetermineWoundSequence() const;

 public:
  virtual void                   OnFlagChanged(UINT oldFlags);
  virtual const VirtualItemInfo *GetVirtualItem(UINT slot, bool ignoreDisarmFlag) const;
  virtual int                    GetVirtualItemDisplayID(UINT slot) const;
  virtual int                    ShouldRenderUnitName(UINT mode) const;
  virtual void                   CommitTexture(int force);
  virtual UINT                   UpdateUnitNameString(UINT localPlayerFlags, UINT otherUnitsFlags, char *buffer, UINT bufferSize) const;
  virtual void                   OnPickNextStandHandler();
  virtual float                  GetMountScale() const;
  virtual void                   OnMount();
  virtual void                   OnDismount();
  virtual bool                   CanBeMounted();

 protected:
  virtual void CleanupUnitArtwork(int playerModelChanged, int wasPlayerModel);
  virtual void ReinitializeUnitArtwork();
  virtual void PostReinitializeArtwork();

 public:
  virtual void  OnStandStateChanged(UINT oldState, UINT newState);
  virtual void  ChangeStandState(UINT standState);
  virtual void  SetEmoteState(UINT emoteID);
  virtual int   GetSpellRank(int spellID) const;
  virtual bool  GetDefenseSkillRank(int &base, int &modifier) const;
  virtual bool  GetAttackSkillRank(int hand, int &base, int &modifier) const;
  virtual void  OnLevelChange();
  virtual float GetBlockChance() const;
  virtual float GetDodgeChance() const;
  virtual float GetParryChance() const;
  virtual int   GetSpellCastingTime(int spellID) const;
  virtual void  UpdateObjComponentVisuals(const CGItem_C *item, const ItemEnchantment *enchantments, int num);
  virtual void  ClearItemVisuals(ACTIVEATTACHMENTINFO *info);
  virtual void  SetItemVisuals(ACTIVEATTACHMENTINFO *info, const ItemVisualsRec *rec, bool force);
  virtual void  SetLastWeaponModeSent(int mode);

  void SetStorage(DWORD *storage);
  void PostInit(const CClientObjCreate &init);
  void PostMovementUpdate(const CClientMoveUpdate &update);
  void UpdateUnitCollisionBox(HMODEL model, LPCSTR modelFileName);
  void SetClientInitData(DWORD eventTime, const CClientObjCreate &init, bool partialUpdateOfActivePlayer);
  void UpdateMoveInfo(DWORD eventTime, const CClientMoveUpdate &update);

 protected:
  static DWORDLONG m_activeMover;

 public:
  enum {
    NUM_SAVED_FACING_DELTAS = 4
  };

  static void Initialize();
  static void PostShutdown();
  static void Shutdown();
  static UINT OffsetOf(OBJECT_TYPE_ID type);
  static void SetActiveMover(const DWORDLONG &guid);
  static void StopMoveHeartbeatTimer();
  static void StartMoveHeartbeatTimer();
  static int  GetAnimPriority(int state);
  static void NamePlateShow(int show);
  int         GetCreatureType() const;
  int         CanBeLooted(DWORD currentTime) const;
  static void UpdateUnitNameplates(CGWorldFrame *worldFrame);
  static void RemoveAllNamePlates();

  virtual LPCSTR GetObjectName() const;

 protected:
  virtual int ShouldFadeIn() const;

 public:
  virtual int  GetSelectionHighlightColor(NTempest::CImVector *outPtr) const;
  virtual void RenderTargetSelection() const;
  void         BuildSelectionRotMatrix(NTempest::C44Matrix &matrix) const;
  LPCSTR       GetUnitName() const;
  LPCSTR       GetUnitTitle() const;
  void         UpdatePlayerNameWorldText();

 private:
  void AddUnitNamePlate(CGWorldFrame *worldFrame);
  void InsertSortedNamePlate(struct NAMEPLATEDESC *desc);
  void RemoveUnitNamePlate();

 public:
  void                       DestroyFadingMounts();
  void                       UpdateFadingMountModel(const NTempest::C3Vector &cameraPos, const NTempest::C3Vector &cameraTarg);
  void                       InitializeUnitName();
  void                       CreateFadeInMount();
  static void                ResortAllUnitNameplates(CGWorldFrame *worldFrame);
  virtual NTempest::C3Vector GetPosition() const;
  virtual void               GetPosition(NTempest::C3Vector &vec) const;
  virtual float              GetFacing() const;
  float                      GetDisplayFacing() const;
  float                      GetSmoothFacing() const;
  float                      GetRawSmoothFacing() const {
    return m_smoothFacing;
  }
  void                       UpdateSmoothFacing();
  void                       SetSmoothFacing(float facing);
  bool                       IsTurningState() const;
  virtual NTempest::C3Vector GetGroundNormal() const;
  virtual void               GetWorldMatrix(NTempest::C34Matrix *worldMatrix) const;
  virtual void   ObjectPostAnimate(const NTempest::C34Matrix &matrix, const NTempest::C3Vector &cameraPos, const NTempest::C3Vector &cameraTarg);
  virtual float  GetRenderFacing() const;
  virtual void   UpdateRenderFacing();
  virtual void   UpdatePlayerName();
  virtual void   PostAnimate(CGWorldFrame *worldFrame);
  virtual void   OnSpecialMountAnim();
  virtual int    ShouldRender(DWORD worldStatus);
  virtual HMODEL GetCharacterModel(int *mountedPtr) const;
  virtual LPCSTR GetModelFileName() const;
  virtual int    UpdateModelLoadStatus();
  void           RequestTalkEmote(TALKANIMATION talkAnim);
  virtual UNITAFFILIATION GetGUIDAffiliation(DWORDLONG unit) const;

  __forceinline const CGUnitData *GetUnitData() const {
    return m_unit;
  }

  HMODEL DuplicateCharacterModel(UINT flags) const;

 protected:
  void InitializeSequenceFlags();
  void MarkSwimAnimations();
  void QueryModelStats();
  void QueryMountModelStats();
  void GetSwimMatrix(NTempest::C34Matrix *worldMatrix) const;
  void UpdateDisplayFacing();
  int  ShouldShuffle() const;

 private:
  void      UpdateBaseRadius(HMODEL model);
  CGUnit_C &operator=(const CGUnit_C &);

 public:
  friend void SetPortraitTexture(CSimpleTexture *texture, const CGUnit_C *unit);
  friend void CreatureQueryCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted);
  friend int  UnitModeUpdateHandler(DWORDLONG guid, UINT offset, UINT bytes, LPCVOID oldValue, LPVOID param);
  friend int  OnQuestUpdate(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);

  static void InitializeTextureVariations(const CreatureDisplayInfoRec *displayInfo, HMODEL theModel, const CreatureModelDataRec *modelData);

 public:
  void PostSetClientInitData(const CClientMoveUpdate &update);
  int  OnMoveEvent(NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
  void OnMonsterMove(DWORD eventTime, CDataStore *msg);
  int  OnForceMoveChange(DWORD eventTime, NETMESSAGE msgID, CDataStore *msg);
  void OnMoveStopLocalNoUpdate(DWORD eventTime);
  void OnSetRunModeLocalNoUpdate(DWORD eventTime, int run);
  void OnSetFacingLocalNoUpdate(DWORD eventTime, float facing);
  void OnSetFacingGUIDLocalNoUpdate(DWORD eventTime, const DWORDLONG &guid);
  void OnTeleportNoUpdate(DWORD eventTime, const NTempest::C3Vector &position, float facing);
  void OnPendingMoveStateChange(NETMESSAGE msgId);
  void OnMoveStartLocal(DWORD eventTime, int forward);
  void OnCollideFalling(DWORD eventTime);
  void OnCollideFallLand(DWORD eventTime);
  void OnMoveStopLocal(DWORD eventTime);
  void OnJumpLocal(DWORD eventTime);
  void ToggleRunModeLocal(DWORD eventTime);
  void OnTurnStopLocal(DWORD eventTime);
  void OnStrafeStopLocal(DWORD eventTime);
  void OnStrafeStartLocal(DWORD eventTime, int left);
  void OnTurnStartLocal(DWORD eventTime, int left);
  void OnPitchStartLocal(DWORD eventTime, int up);
  void OnPitchStopLocal(DWORD eventTime);
  void OnSetRunModeLocal(DWORD eventTime, int run);
  void OnSetFacingLocal(DWORD eventTime, float facing);
  void OnSetRawFacingLocal(DWORD eventTime, float facing);
  void OnSetPitchLocal(DWORD eventTime, float pitch);
  void OnRunSpeedChangeLocal(DWORD eventTime, NETMESSAGE msgID, float speed);
  void OnWalkSpeedChangeLocal(DWORD eventTime, float speed);
  void OnSwimSpeedChangeLocal(DWORD eventTime, NETMESSAGE msgID, float speed);
  void OnAllSpeedChangeLocal(DWORD eventTime, float speed);
  void OnTurnRateChangeLocal(DWORD eventTime, float rate);
  void OnMovementInitiated(bool facingOnly);
  void OnTeleportLocalNoUpdate(DWORD eventTime, const NTempest::C3Vector &position, float facing);
  void UpdateSwimmingStatus(DWORD eventTime, int inWater, float depth);
  void SendRedirectionMessage();
  void PlaySplashSound(const NTempest::C3Vector &position);
  void ProcessLocalMoveEvent(NETMESSAGE msgId);
  void BuildMovementUpdate(NETMESSAGE messageId, CDataStore *msg) const;
  void SendMovementUpdate(NETMESSAGE messageId);
  void StopSpellFizzleTimer(int spellID, BYTE status);
  void SpellDelayed(int delay);
  void EndSpellEffects(BYTE status);
  int  SetCastingSpell(int spellID, bool force, bool precastAnimSuccessful);
  bool SetSpellCastingAnimation(ANIMENUMERATION anim, UINT castKit, UINT soundID, int shakeID, ANIMENUMERATION &result);
  void ClearSpellCastAnimInfo();
  void AddHitAnimHolds(int spellID, const TSStackArray<DWORDLONG> &targets);
  void MaybeSaveChannelSpellTargets(int spellID, const TSStackArray<DWORDLONG> &targets);
  void StoreSpellMissileEffect(
      const DWORDLONG          &target,
      const NTempest::C3Vector &destination,
      float                     speed,
      UINT                      ammoDisplayID,
      int                       inventoryType,
      const SpellVisualRec     *rec,
      bool                      hits,
      MISS_REASON               reason,
      UINT                      spellID,
      bool                      wasProc
  );
  int GetCastingSpell() const {
    return m_castingSpell;
  }
  void                  HandlePrecastStart(bool precast);
  void                  HandlePrecastStop(int spellID, bool force);
  void                  CheckDeferredSheathing();
  void                  SetSheatheReason(SHEATHEREASONS reason, bool on, bool suppressSound);
  const SpellVisualRec *GetAppropriateSpellVisual(const SpellRec *spellRec, SpellVisualRec &filled) const;
  UINT                  GetCurrentTorsoAnim() const;

 protected:
  UINT GetAnimationState();

 public:
  int                IsWalking() const;
  int                SetTorsoAnimation(UINT state, DWORD duration, UINT flags);
  void               CheckLevelUpAnimFlag(int oldState, int newState);
  void               HandleCastAnimEvent();
  void               HandleCombatAnimEvent(LPCSTR eventName, DWORD value, const NTempest::C3Vector &position);
  void               HandleAnimEvent(LPCSTR eventName, const NTempest::C3Vector &pos);
  void               HandleMountedAnimEvent(LPCSTR eventName, const NTempest::C3Vector &pos);
  UNITEFFECTSPECIALS DetermineBreathEffect(UINT *duration);
  void               BreathHandler(int forceOnMount);
  void               ProcessBreathParticles(int currentTime);

 protected:
  void HandlePlayStandSound(DWORD code, LPCSTR eventName);
  void HandleFootfallAnimEvent(const NTempest::C3Vector &position);
  void PlayFidgetSound(UINT fidgetNumber);

 public:
  void PlayStandSound() const;
  void PlayDeathThud() const;
  int  GetStandStateAnim(HMODEL model) const;
  void SetTorsoAnim(UINT newAnim);

 protected:
  int IsSplashing(const NTempest::C3Vector &position);

 public:
  bool IsShapeShifted() const;
  int  IsUnderWater() const;
  UINT GetRunSequence() const;
  UINT GetStopSequence() const;
  int  GetWalkStateAnim() const;
  UINT GetEmoteAnimation(UINT emoteID) const;
  void SetRangedWeaponPullAnim(int duration);

 protected:
  float GetAnimTimeScale(UINT sequence, UINT duration, UINT flags);
  int   SetTorsoSequence(float timeScale, int flags);

 public:
  bool TorsoAnimOverridesBase() const;
  int  IsPreemptableWoundAnimState(UINT state);
  int  IsAttackAnimState(UINT state);
  bool QueueVictimAnim(VICTIMSTATES newState, int unitDead, int criticalHit, UINT victimRoundDuration);

 protected:
  void CheckPendingVictimFeedback();

 public:
  void        SetVictimAnimation(VICTIMSTATES newState, int unitDead, int criticalHit, UINT victimRoundDuration, int processNow);
  void        DoVictimFeedback(const ATTACKROUNDINFO *roundInfo, int showAnimation);
  void        AdjustVictimState(ATTACKROUNDINFO *roundInfo);
  MISS_REASON AdjustVictimState(MISS_REASON reason);
  void        ShowWorldText(const ATTACKROUNDINFO *roundInfo);
  void        PerformSpellProcImpact(int spell);

 protected:
  void               ShowBloodSpurt(CGUnit_C *attacker, int crushingBlow);
  BLOODSPURTLOCATION DetermineBloodLinkPoint(CGUnit_C *attacker);

 public:
  void                   SetMeleeDeathHold(const CGUnit_C *victimPtr);
  void                   AddVictimDeathHold(CGUnit_C *victimPtr);
  void                   ClearMeleeDeathHold();
  void                   PlayParrySound(bool ignoreMainHand, const ATTACKROUNDINFO *roundInfo, const NTempest::C3Vector &position) const;
  void                   PlayImpactSound(DWORDLONG attacker, int criticalHit, COMBATHAND hand) const;
  void                   PlayCustomAttackSound(int sound, const NTempest::C3Vector &position);
  void                   SetCustomAttackSound(int sound, const NTempest::C3Vector &position);
  void                   ShowHandArrow(int show);
  const VirtualItemInfo *GetParryingItem(bool ignoreMainHand) const;
  const VirtualItemInfo *GetAttackingWeapon(COMBATHAND hand) const;
  bool                   GetWeaponSwingType(bool mainHand, WEAPONSWING_SOUNDTYPES &type);
  int                    GetUnitSize() const;

 protected:
  void CheckPendingMissileRelease(const NTempest::C3Vector *position);

 public:
  void                     CheckPendingImpactKit();
  const SpellVisualKitRec *GetRangedSpellAnim(int id, bool castKit);
  int                      ClearTorsoAnimation(UINT flags);

 protected:
  bool IsSpellAuraAnimActive(int &anim) const;
  bool IsSpellChannelAnimActive(int &anim) const;

 public:
  int  PlayEmoteAnimation(UINT emoteID, int flags);
  void RangedWeaponAnimEndHandler();
  void ThrowAnimEndHandler();
  void GenericAnimEndHandler(ANIMENUMERATION animID, LPVOID param);

 protected:
  void StoreSequenceEndCallbacks(int anim);
  void ProcessAnimEndCallbacks();

 public:
  void DeathAnimEndHandler();
  void ForceUpdateBaseAnimation();
  void PickNextRunHandler();
  void WoundAnimEndHandler();
  void SpellAnimEndHandler();
  void NPCAnimEndHandler();
  int  JumpTakeOffFinishedHandler();
  int  JumpLandFinishedHandler();
  void SitSleepAnimEndHandler();
  void RangedPrecastEndHandler();
  bool SetSpellPreCastingAnimation(ANIMENUMERATION anim);
  void AttackAnimEndHandler();
  void DodgeAnimEndHandler();
  void SetRangedWeaponReleaseAnim();
  void DrawBowString(const NTempest::C3Vector &cameraPos);
  void ThrownMissileReleased();
  void CheckPendingThrownWeaponReattach(bool force);

 protected:
  void AddObjectComponentBySlot(
      int  invSlot,
      int  displayID,
      int  inventoryType,
      bool forceAlternate,
      bool deferApply,
      bool sheathe,
      int  sheathedAttachmentPoint,
      bool showHidden
  );

 public:
  bool IsSlotComponented(UINT offset, int ignoreUsingRangedWeapon);

 protected:
  bool UpdateVisibilitySlots(HMODEL characterModel, int attachmentSlot, ACTIVEATTACHMENTINFO **&found, int displayID, bool deferApply);
  void ClearWeaponTrailHandles();

 public:
  void   ReinitializeWeaponTrails();
  void   OnMountCancelled();
  void   OnEncounter(AI_REACTION reaction);
  HMODEL GetMountModel();

 protected:
  const CreatureSoundDataRec *GetMountSoundDataRec() const;
  const CreatureSoundDataRec *GetSoundData() const;

 public:
  void UnitInitializeMountModel(HMODEL model);
  void CreateUnitMount();
  void DestroyUnitMount(int doNotUpdateAnim);
  void UpdateUnitMountInfo(int immediate, UINT changedFlags);
  void CreateFadeOutMount();
  void DisableWeaponTrails();

 protected:
  ACTIVEATTACHMENTINFO *CreateAttachmentInfo(
      int  invSlot,
      int  displayID,
      int  inventoryType,
      bool forceAlternate,
      bool sheathe,
      int  sheathedAttachmentPoint,
      bool showHidden
  );

 public:
  UINT          GetDisplayRace() const;
  UINT          GetDisplaySex() const;
  HTEXCOMPONENT GetTexComponent() const;
  virtual int   UpdateAttachmentLoadStatus();
  virtual int   UpdateTexComponentLoadStatus();
  int           IsModelComponentable() const;
  LPCSTR        GetDisplayTextureName() const;
  UINT          SkinVariationID() const;
  UINT          FaceID() const;
  UINT          HairStyleID() const;
  UINT          HairColorID() const;
  UINT          FacialHairID() const;
  void          InitPreferredGeosets();

 protected:
  void SetAttachmentHidden(int attachmentSlot, bool hide);

 public:
  void      PlaySpellLoopedSound(int soundID);
  void      KillSpellLoopedSound();
  void      StopRangedAttackPrecast();
  HMODEL    GetRangedWeaponModel();
  HMODEL    GetMountedModel() const;
  void      PendingPrecastInterrupt(int spellID);
  void      SaveTrackingTarget(DWORDLONG target, TRACKTYPE type, bool snapToTargetOnClear);
  void      ClearTrackingTarget(bool snapToTargetOnClear);
  DWORDLONG GetTrackingTarget() const;
  bool      TrackingTargetMoving() const;
  void      HandleFollowTarget();
  void      SetWeaponMode(WEAPONMODE mode);
  void      ClearRangedStandTimer();
  void      SetAmmoDisplay(UINT displayID, UINT inventoryType) {
    m_ammoDisplayID = displayID;
    m_ammoInvType = inventoryType;
  }
  void SetRangedStandTimer();
  void DetermineReadySequence(bool forceNormal);
  void OnCombatModeTimer();
  void AttackUnit(CGUnit_C *newVictim);
  void OnAttackSwing(DWORDLONG victimGUID, UINT clientTimeStamp);
  void SetDebugHitRolls(const ATTACKROUNDINFO &info);

 private:
  int SetAttackerAnimation(const ATTACKROUNDINFO *roundInfo, int processNow);

 public:
  void InitializeResEffectModel();
  void ClearResEffectModel();
  void AttachResEffectModel();
  void DetatchResEffectModel();
  void OnRangedStandTimer();

 protected:
  bool SheatheAnimPlaying() const;

 public:
  void MaybeStartSheatheAnim();
  void UpdateSheatheRangedReasons(bool suppressSound);
  void SheatheOrUnsheatheItems(SHEATHEREASONS reason, bool sheathe, bool playSound);

 protected:
  bool SheatheObjComponent(int slot, bool sheathe);
  void ClearDeferredAttachment(HMODEL charModel, int slot);
  bool ApplyAttachmentInfo(HMODEL characterModel, bool sheathe, int attachmentSlot, bool force);
  void SetHandsState(HMODEL model);
  void SetFingersSeq(HMODEL charModel, UINT sequence, UINT startFinger, UINT lastFinger);
  void ResetFingersSeq(HMODEL charModel, UINT startFinger, UINT lastFinger);
  void SetHandState(HMODEL model, const VirtualItemInfo *item, UINT startFinger, UINT lastFinger);

 public:
  void HandleSheatheAnimEvent(bool clearSheatheAnim, bool suppressSound);
  void SheatheAnimEndHandler();

 protected:
  bool SetSheathingSequence();

 public:
  void UnitInitializeModel(HMODEL model);
  void UnitUninitializeModel(HMODEL model);
  void ShutdownWorldName();

 protected:
  void MarkFootstepAnimations(HMODEL model);
  void UpdateUnitAlpha();
  void RefreshAttachmentInfo(HMODEL model);

 public:
  void KillCreatureLoopSound();
  void InitializeLoopSound();
  void InstallSeqEndHandler(HMODEL model, UINT animID);
  void ClearMountAnimState();
  void ClearTempCharModel();
  void SetTempCharModel(HMODEL model);

 protected:
  void ClearAnimCallbackData();

 public:
  void CheckPendingSpellAnimHits();
  void SpellAnimHit(int spellID);
  void UpdateMountAnimation(UINT newState, UINT flags);
  void UpdateMovementAnimSpeed(int forMount, int currentState);
  void UpdateBaseAnimation(UINT newState, UINT flags);
  void SetBaseAnim(UINT newAnim);
  void LookAtTarget();
  void UpdateLookAtTarget();
  void SetLocalTarget(DWORDLONG target);
  bool BaseAnimLocksHead() const;
  bool TorsoAnimLocksHead() const;
  UINT GetCurrentBaseAnimState() const {
    return m_currentBaseAnimState;
  }
  UINT GetCurrentTorsoAnimState() const {
    return m_currentTorsoAnimState;
  }
  UINT GetCurrentBaseAnim() const {
    return m_currentBaseAnim;
  }
  int          IsInStandSitTransition();
  int          IsInSitSleepPosition();
  int          IsPlayingSittingOrStandingAnim() const;
  int          GetFactionTemplate() const;
  virtual int  IsSolidSelectable() const;
  virtual int  IsSolidCollidable() const;
  virtual int  CanHighlight() const;
  virtual int  CanBeTargetted() const;
  virtual void OnLeftClick();
  virtual void OnRightClick();
  int          GetSpellLevel(int spellID) const {
    return GetSpellRank(spellID) / 5;
  }
  bool                       IsSpellKnown(int spellID) const;
  bool                       CheckAndReportSpellInhibitFlags(const SpellRec *spell, const CGItem_C *item);
  const SkillLineAbilityRec *LookupAbility(int spellID) const;
  bool                       IsSpellSuperceded(int spellID) const;
  int                        GetSpellSkillLine(int spellID) const;
  static bool                FactionHasReputation(int faction);
  UNIT_REACTION              UnitReaction(const CGUnit_C *unit) const;
  static UNIT_REACTION       UnitReaction(int factionID, const CGUnit_C *unit, int trueSight);
  bool                       CanAttack(const CGUnit_C *unit) const;
  bool                       CanAssist(const CGUnit_C *unit) const;
  bool                       CanCooperate(const CGUnit_C *unit) const;
  bool                       CanInteract(const CGUnit_C *unit) const;
  bool                       CanInteract(const CGGameObject_C *object) const;
  bool                       IsUnitInGroup(const CGUnit_C *unit) const;
  void                       SetMirrorHandlers();
  void                       UnsetMirrorHandlers();
  void                       ClearFishingObject();
  void                       ProcessChannelObject();

 protected:
  void SetAuraMirrorHandlers();
  void UnsetAuraMirrorHandlers();
  void SetAuraMirrorHandler(UINT slot, int (*handler)(DWORDLONG, UINT, UINT, LPCVOID, LPVOID));
  void UnsetAuraMirrorHandler(UINT slot, int (*handler)(DWORDLONG, UINT, UINT, LPCVOID, LPVOID));

 public:
  void                OnAuraChanged(UINT slot, int previousValue);
  void                SignalDisplayHealthUpdate() const;
  void                UpdateDisplayHealth();
  void                HandleBloodPool(UINT currentTime);
  void                OnStopRender();
  void                CheckRendering();
  void                UpdateDisplay(DWORD now);
  void                RemoveBloodPool();
  void                AddBloodPool();
  void                QueueBloodSplat(BLOODSPURTLOCATION linkPoint);
  const UnitBloodRec *GetBloodRecord();

 protected:
  void RemoveAuraEffect(UINT slot, int previousSpell);
  void RefreshAuraVisuals();
  void AddPendingShapeshiftEffect(int oldSpell);
  void AddAuraEffect(UINT slot, bool startNow);

 public:
  int  ShouldDelayLevelupAnim();
  int  ShouldDelayLevelupAnim(UINT state);
  void PerformLevelUpAnim(int force);
  void OnCharmedChanged();
  void UpdateDisplayInfo();
  int  DisplayInfoNeedsUpdate(int &playerModelChanged, int &wasPlayerModel) const;

 protected:
  void RefreshDataPointers();

 public:
  void InitializeExtendedDisplay();

 protected:
  void SetupFootprints();

 public:
  void             AttachVirtualMonsterWeapons();
  void             RenderDebugPathing();
  void             InitializeNPCItems();
  void             PlayImpactKit(int spellID, const SpellVisualKitRec *impactKit);
  void             SetSpellImpactKit(const SpellVisualKitRec *impactKit);
  void             AddSpellProcOneShotEffect(int spellID, const SpellVisualKitRec *rec);
  void             SetImpactKitEffect(int spellID, CGUnit_C *target, const SpellVisualKitRec *impactKit, int immediate);
  void             AddSpellProcAuraEffect(int auraslot, const SpellVisualKitRec *rec);
  void             RemoveSpellProcAuraEffect(ACTIVEAURAINFO *rec);
  SPELLEFFECTDESC *FindSpellEffectProcDesc(const SpellVisualKitRec *rec);
  void             RefreshSpellProcEffects();
  void             UpdateSpellProcEffects(float elapsedTime);
  void             AddEmissiveColor(const NTempest::CImVector &color);
  void             RemoveEmissiveColor(const NTempest::CImVector &color);
  void             SetStandStateAnim(int standAnim);
  void             SetWalkStateAnim(int walkAnim);
  void             EnableWeaponTrail(const NTempest::CImVector &color, int fadeOutRate, UINT duration);

 protected:
  void OnTeleport(DWORD eventTime, const CMovementStatus &update);
  void OnMoveStart(DWORD eventTime, const CMovementStatus &update, int forward);
  void OnMoveStop(DWORD eventTime, const CMovementStatus &update);
  void OnStrafeStart(DWORD eventTime, const CMovementStatus &update, int left);
  void OnStrafeStop(DWORD eventTime, const CMovementStatus &update);
  void OnJump(DWORD eventTime, const CMovementStatus &update);
  void OnTurnStart(DWORD eventTime, const CMovementStatus &update, int left);
  void OnTurnStop(DWORD eventTime, const CMovementStatus &update);
  void OnPitchStart(DWORD eventTime, const CMovementStatus &update, int up);
  void OnPitchStop(DWORD eventTime, const CMovementStatus &update);
  void OnSetRunMode(DWORD eventTime, const CMovementStatus &update, int run);
  void OnSetFacing(DWORD eventTime, const CMovementStatus &update);
  void OnSetPitch(DWORD eventTime, const CMovementStatus &update);
  void OnToggleCollision(DWORD eventTime, const CMovementStatus &update);
  void OnRunSpeedChange(DWORD eventTime, const CMovementStatus &update, CDataStore *msg);
  void OnWalkSpeedChange(DWORD eventTime, const CMovementStatus &update, CDataStore *msg);
  void OnSwimSpeedChange(DWORD eventTime, const CMovementStatus &update, CDataStore *msg);
  void OnTurnRateChange(DWORD eventTime, const CMovementStatus &update, CDataStore *msg);
  void OnTeleportAck(DWORD eventTime, const CMovementStatus &update);
  void OnSwimStart(DWORD eventTime, const CMovementStatus &update);
  void OnSwimStop(DWORD eventTime, const CMovementStatus &update);
  void OnMoveHeartBeat(DWORD eventTime, const CMovementStatus &update);

 private:
  void InternalProcessSpellProcEffects(SPELLPROC_ACTION action, float elapsed);

 public:
  SPELLEFFECTDESC *GetActiveEffect(LIST(SPELLEFFECTDESC) & list);
  void             ReinitializePaperdollModel();
  void             CreatePaperdollModel();
  HMODEL           GetPaperDollModel(bool duplicateModel);
  void             DestroyPaperdollModel();
  void             StandStateChanged(UINT oldState);
  void             NPCFlagChanged(UINT oldNPCFlags);
  void             RemoveInteractIcon();
  void             RefreshInteractIcon();
  void             UpdateInteractIcon(QUEST_GIVER_STATUS status);
  void             UpdateInteractIcon(INTERACTICONTYPE which);
  UINT             GetPlayerNameAttachmentPoint();
  void             SetEmoteQueue(const QUESTGIVEREMOTENODE *list, UINT num);
  void             SetEmoteQueue(TSStackArray<QUESTGIVEREMOTENODE> &list);

 protected:
  int EmoteProcType(UINT emoteID, EMOTESPECPROCS &proc) const;

 public:
  void AddWorldXPGainText(int xpGain);
  void AddWorldDamageText(UINT damage, int normalCombatDamage);
  void AddWorldCritText(UINT damage, int normalCombatDamage);
  void AddWorldText(MISS_REASON reason);
  void AddWorldText(WORLDTEXTMISSTYPE type);
  void StoreXPGain(int XP);
  void SaveQuestAddItemMessage(int killed, int needed);
  void ProcessQuestItemMessages();
  void AddDamageDone(UINT damage, int normalCombatDamage, UINT flags, DWORDLONG attacker, int spellID);
  void SpellEventHit();
  void ShowPlayerXPGained();
  void WeaponModeChanged();
  void VirtualComponentChanged(int slot, int oldValue);
  void AttachVirtualComponent(UINT slot, bool deferApply);
  void DetachVirtualComponent(int vslot, bool defer, bool removeRecord);

 protected:
  void RemoveObjectComponentByInvSlot(int invSlot, bool deferDeleteFromModel, bool removeRecord);
  void ClearActiveAttachmentInfo();

 public:
  void OnDynamicFlagsChanged(UINT oldValue);
  void OnChannelSpellChanged(UINT oldSpell);
  void ClearSavedChannelSpellTargets();
  int  GetSavedChannelSpellID() const {
    return m_savedChannelSpellID;
  }
  const TSGrowableArray<DWORDLONG> &GetSavedChannelSpellTargets() const {
    return m_savedChannelSpellTargets;
  }

 protected:
  int SetEmoteAnimation(UINT emoteID, int flags);

 public:
  void DDDELLOG(DWORDLONG guid, LPCSTR string, LPCSTR file, UINT line);
  void DDADDLOG(DWORDLONG guid, LPCSTR string, LPCSTR file, UINT line);
  void AddDeathHold();
  void DDGENLOG(DWORDLONG guid, LPCSTR string, LPCSTR file, UINT line);
  void DumpGeneralDeathHoldLog(HSLOG handle, TSGrowableArray<char> *stringBuffer) const;
  void DelDeathHold();

 protected:
  void            MaybeAttachAura(UNITEFFECTATTACHPPOINT attach, UINT effect, UINT spellID, int priority, bool permanent);
  ACTIVEAURAINFO *FindActiveAuraInfo(int slot);
  void            AddKitAuras(const SpellVisualKitRec *kitRec, const SpellRec *spellRec);
  void            RemoveAuraVisual(UNITEFFECTATTACHPPOINT attach);
  void            FinishAuraDecays();

 public:
  void RegisterScript();
  void UnregisterScript();
  void TriggerPlayerNameUpdate();
  void PlayerNameVisibilityChanged(int nameVisible);
  void UpdatePlayerNameColor();
  void SetForcedAnimation(LPCSTR string);
  void ResetForcedAnimation();
  void OnNPCHello();
  void OnNPCGoodbye();
  int  PlayNPCSound(NPCSOUNDS sound, UINT index);

  static DWORDLONG GetActiveMover() {
    return m_activeMover;
  }
  bool      IsClientControlled() const;
  float     GetBoundingRadius() const;
  void      OnRestoreHealth();
  bool      DoNotLogDeath() const;
  DWORDLONG IsAttacking() const {
    return m_combat.IsAttacking();
  }
  DWORDLONG          IsAttackingNow() const;
  void               ClearAttackSent();
  bool               CanAttackNow(const CGUnit_C *unit) const;
  int                IsStunned() const;
  int                IsPacified() const;
  int                IsDisarmed() const;
  int                IsPlayingDeathAnim() const;
  int                IsPlayingLayDownAnim() const;
  int                IsPlayingSleepAnim() const;
  int                IsPlayingGetUpAnim() const;
  int                HasBloodRec() const;
  void               GetResistanceAndBuffs(int r, int &realResistance, int &effectiveResistance, int &buffPositive, int &buffNegative) const;
  bool               IsFriend(const CGUnit_C *unit) const;
  bool               IsPeaceful(const CGUnit_C *unit) const;
  bool               IsEnemy(const CGUnit_C *unit) const;
  void               SetDead();
  int                SetBlock(UINT i, DWORD data);
  void               SetData(LPCVOID data, UINT bytes);
  int                HasInteractIcon();
  QUEST_GIVER_STATUS GetQuestGiverStatus();
  void               SetQuestGiverStatus(QUEST_GIVER_STATUS status);
  int                GetDebugStateInfo(ATTACKROUNDINFO *attackInfo);
  void               ClearDebugFlags();
  void               SetUnitBadFacing();
  int                IsBadFacing();
  int                IsDeathFlagSet() const {
    return (m_animFlags & 0x2000) != 0;
  }
  void RemoveForceDisplayFacingFlag();
  UINT GetReadySequence() const {
    FATALASSERT(m_readySequence != 0xffffffff);
    return m_readySequence;
  }
  UINT        GetRangedReadySequence() const;
  void        UpdateReadyAnim(const ItemStats *stats);
  UINT        GetDeathHolds() const;
  void        SetReattachThrownWeapon(int reattach);
  int         ShouldReattachThrownWeapon() const;
  void        SetDebugPathPosition(const NTempest::C3Vector &position);
  HCHARGEOSET GetGeosetHandle() const;
  const UINT *GetPreferredGeosets() const;
  UINT        GetNumPreferredGeosets() const;
  int         GetDisplayHealth() const;
  int         IsTexComponentLoaded() const;
  void        SetTexComponentLoaded(int loaded);
  bool        IsBeingStalked() const;
  bool        GetLootPermission() const;
  bool        GetShowingHandArrow() const;
  void        SetShowHandArrowFlag(int show);
  void        SetShowingHandArrowFlag(int showing);
  bool        IsItemSwapFlagSet() const;
  void        SetItemSwapFlag(bool set);
  int         UnitMountShowing() const;
  bool        UnitHeadLocked() const;
  void        SetCreatureStats(const CreatureStats_C *stats);
  void        OnEnableCollisionLocalNoUpdate(DWORD eventTime);
  void        OnDisableCollisionLocalNoUpdate(DWORD eventTime);
  void        OnToggleCollisionLocal(DWORD eventTime);
  void        OnStrafeStartLocalNoUpdate(DWORD eventTime, int left);
  void        OnStrafeStopLocalNoUpdate(DWORD eventTime);
  void        OnSwimStartLocal(DWORD eventTime);
  void        OnSwimStopLocal(DWORD eventTime);

 protected:
  UINT GetFlags() const;
  int  CurrentAnimIncludesHit() const;
  int  GotRangedWeaponRelease() const;
  void FootstepAnimEventHit(const NTempest::C3Vector &position, int isLeftFoot);
  void HandleFootstepAnimEvent(const NTempest::C3Vector &position);
  void GetFootprintInfo(UINT *id, NTempest::C2Vector *size);
  bool WeaponAttached(COMBATHAND hand) const;
  void ClearChannelAuraInfo();
  void HandleRemotePlayerSheathing();
  void HandleLocalPlayerSheathing();
  bool SheatheAnimEventEncountered() const;
  void SetSheatheEventEncountered(bool encountered);

 private:
  void  AddDamageTimer(DWORDLONG attacker, VICTIMSTATES state, int unitDead, UINT duration, float delay, int criticalHit, int crushingBlow);
  float GetBaseRadius() const;

  friend void MovementFixOutOfBoundsUnit(DWORDLONG guid);

 private:
  int       m_questCountKilled;
  int       m_questCountNeeded;
  HMODEL    m_resEffectModel;
  DWORDLONG m_meleeTargetDeathHold;
  int       m_precastSheatheHoldTimer;

 protected:
  int                m_customAttackSound;
  NTempest::C3Vector m_customAttackPosition;
  UINT               m_splashSoundID;

 private:
  UINT m_disengageLookAtTimer;

 protected:
  TSGrowableArray<ANIMENDDATA>       m_animEndCallbackList;
  ANIMENDDATA                       *m_callbackList[135];
  const CreatureStats_C             *m_stats;
  const CreatureDisplayInfoRec      *m_displayInfo;
  const CreatureDisplayInfoExtraRec *m_displayInfoExtra;
  const CreatureModelDataRec        *m_modelData;
  const CreatureSoundDataRec        *m_soundData;
  const CreatureSoundDataRec        *m_mountedSoundData;
  const UnitBloodLevelsRec          *m_bloodRec;
  AuraVisual                         m_auraVisual[12];
  LISTDECL(ACTIVEAURAINFO, m_activeAuraInfo);
  ANIMENUMERATION                      m_pendingImpactAnim;
  HMODEL                               m_tempCharModel;
  TSGrowableArray<char>                m_deathHoldBuffer;
  TSGrowableArray<UINT>                m_deathHoldBufferIndices;
  int                                  m_lastDeathTime;
  int                                  m_nextDeathHoldCheckTime;
  TSGrowableArray<QUESTGIVEREMOTENODE> m_emoteQueue;
  HMODEL                               m_interactIconModel;
  LISTDECL(BLOODSPLATNODE, m_bloodSplatNodes);
  UINT m_nextAllowableBloodPool;
  LISTDECL(ANIMQUEUENODE, m_animQueue);
  CCombatClient                       m_combat;
  ANIMQUEUENODE                      *m_currentDamageInfo;
  UINT                                m_readySequence;
  UINT                                m_animEndTime;
  UINT                                m_animBaseDuration;
  UINT                                m_animStartTime;
  UINT                                m_flags;
  UINT                                m_animFlags;
  UINT                                m_footprintTextureID;
  UINT                                m_terrain;
  NTempest::C2Vector                  m_footprintSize;
  float                               m_footprintParticleScale;
  DEBUGHITROLLINFO                    m_hitInformation;
  ANIMENUMERATION                     m_spellPrecastingAnim;
  ANIMENUMERATION                     m_spellCastingAnim;
  ANIMENUMERATION                     m_deferredPrecastAnim;
  int                                 m_animatingAura;
  UINT                                m_emoteID;
  UINT                                m_spellCastingEffectKit;
  UINT                                m_spellCastingSoundID;
  int                                 m_spellCastingCameraShakeID;
  MISSILESTRUCT                       m_spellMissileStruct;
  float                               m_lastSentFacing;
  float                               m_lastSentPitch;
  HPLAYERNAME                         m_unitNameHandle;
  int                                 m_accumulatedXPDrop;
  int                                 m_castingSpell;
  int                                 m_interruptedSpell;
  int                                 m_lastSpellCastAnimTime;
  int                                 m_nextBreath;
  int                                 m_nextMountBreath;
  int                                 m_scriptRegistered;
  float                               m_displayFacing;
  float                               m_smoothFacing;
  float                               m_savedFacingDeltas[NUM_SAVED_FACING_DELTAS];
  float                               m_forcedDisplayFacing;
  UINT                                m_deathTime;
  DWORDLONG                           m_lastCombatTarget;
  DWORDLONG                           m_targetUnit;
  UINT                                m_currentBaseAnimState;
  UINT                                m_currentBaseAnim;
  UINT                                m_currentTorsoAnimState;
  UINT                                m_currentTorsoAnim;
  UINT                                m_currentMountAnimState;
  UINT                                m_currentWoundStartTime;
  UINT                                m_currentWoundAnimDuration;
  UINT                                m_spellFizzleTimer;
  UINT                                m_deathHolds;
  QUEST_GIVER_STATUS                  m_questGiverStatus;
  NTempest::C3Vector                  m_serverLoc;
  TSGrowableArray<NTempest::C3Vector> m_debugPathPoints;
  UINT                                m_numDebugPathNodes;
  Sound                              *m_spellLoopedSound;
  Sound                              *m_creatureLoopSound;
  UINT                                m_mountedFootprintID;
  NTempest::C2Vector                  m_mountedFootprintSize;
  HMODEL                              m_fadingPureMountModel;
  PUREMOUNTFADEMODE                   m_pureMountFadeMode;
  UINT                                m_pureMountFadeStartTime;
  float                               m_fadingMountFacing;
  NTempest::C3Vector                  m_fadingMountPos;
  float                               m_fadingMountScale;
  const NPCSoundsRec                 *m_NPCSoundsRec;
  UINT                                m_lastGlobalClickCount;
  UINT                                m_pissedCount;
  UINT                                m_numNPCPissedSounds;
  HCHARGEOSET                         m_geosetHandle;
  HTEXCOMPONENT                       m_texComponent;
  UINT                                m_preferredGeosets[15];
  int                                 m_displayHealth;

 private:
  LISTDECL(IMPACTEFFECTDESC, m_impactEffectsDesc);
  LISTDECL(SPELLEFFECTDESC, m_spellEffectLists[11]);
  NTempest::C3iVector        m_currentEmissive;
  int                        m_pendingHitSpellID;
  TSGrowableArray<DWORDLONG> m_pendingHitAnimVictims;
  int                        m_auraFlags[56];
  int                        m_walkStateAnim;
  int                        m_standStateAnim;
  float                      m_baseRadius;

 protected:
  UINT                       m_ammoDisplayID;
  UINT                       m_ammoInvType;
  UINT                       m_rangedStandTimer;
  ACTIVEATTACHMENTINFO      *m_attachments[5];
  ACTIVEATTACHMENTINFO      *m_deferredAttachments[5];
  int                        m_weaponTrails[5];
  HMODEL                     m_paperDollModel;
  int                        m_sheatheReasons;
  ANIMENUMERATION            m_handAnim[2];
  UINT                       m_deferredSheatheFlags;
  SHEATHEREASONS             m_deferredSheatheReason;
  int                        m_savedChannelSpellID;
  TSGrowableArray<DWORDLONG> m_savedChannelSpellTargets;
  SPELLEFFECTDESC           *m_channelSpellEffect;
  const SpellRec            *m_shapeShiftPoof;
  FishingLineObject         *m_fishingLineObject;

 protected:
  void           ProcessEmoteQueue();
  void           ApplyStrafeRotation(UINT newState);
  void           SetStrafeRotation();
  int            PlayBaseAnimation(int newAnimState, int newAnim, int forceNoFidget, bool &checkImpacts);
  float          DetermineWalkRunTimeScale(int currentState);
  ANIMQUEUENODE *ProcessAnimQueue();
  ANIMQUEUENODE *GetNewAnimNode(int leaveUnlinked);
  void           RecycleAnimNode(ANIMQUEUENODE *node);
  void           PurgeAnimNodes(bool doNotProcess);
  UINT           ChooseAnimation(UINT state) const;
  UINT           DetermineAttackerSequence(COMBATHAND hand) const;
  UINT           DetermineParrySequence() const;
  UINT           GetAttackerAnimEx(COMBATHAND hand, const VirtualItemInfo *itemInfo) const;
  void           DDWRITELOG(LPCSTR buffer);

 private:
  void LookAtTarget(CGUnit_C *target);
  void ApplyObjectCameraSpaceLookAt(const NTempest::C3Vector &target);
  void RemoveObjectLookAt();
  void PrintAttackSeqErrorMsg(UINT sequence, UINT fallBack) const;
};

void CGUnit_C_RenderBowStrings(const NTempest::C3Vector &c);

void ClearSpecialEffects(HMODEL model);
