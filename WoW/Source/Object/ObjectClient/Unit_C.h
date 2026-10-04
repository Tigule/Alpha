#pragma once

#include <Model/IModel.h>

#include "Object/ObjectClient/AnimCompiles.h"
#include "Object/ObjectClient/IUnitEffects.h"
#include "Object/ObjectClient/Object_C.h"
#include "Object/MovementData.h"
#include "Object/Unit.h"
#include "Object/UnitCombat.h"
#include "WowServices/WDataStore.h"

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
class CDataStore;

void UnitUpdateMovementAnim(const DWORDLONG &unit);
BOOL UnitHealthUpdateHandler(DWORDLONG unit, UINT offset, UINT bytes, LPCVOID oldValue, LPVOID param);
void OnMoveUpdate(DWORDLONG unit, DWORD eventTime);
BOOL MoveHeartBeatHandler(LPCVOID packetData, LPVOID param);
BOOL OnUnitCombatEvent(LPVOID param, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
int  Player_C_AppFocusMovementHandler(int focus);
BOOL OnUpdateInventoryComponent(DWORDLONG guid, UINT offset, UINT bytes, LPCVOID prevValue, LPVOID param);

enum UNITEFFECTSPECIALS {
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
  NUM_UNITEFFECTSPECIALS = 43,
  SPECIALEFFECT_NONE = -1
};

enum ANIM_STATE {
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
  ANIM_STATE_LAST_STRAFE = ANIM_STATE_DIAG_BACKWARDS_RIGHT,
  INVALID_ANIM_STATE = -1
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
  NUM_UNITEFFECTATTACHPOINTS = 12,
  UNITEFFECT_INVALID = -1
};

enum GEOCOMPONENTLINKS {
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
  NUM_ATTACH_SLOTS = 36,
  ATTACH_NONE = -1
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
  WEAPONSWING_LIGHT = 0,
  WEAPONSWING_MEDIUM = 1,
  WEAPONSWING_HEAVY = 2,
  NUM_WEAPONSWINGSOUNDTYPES = 3,
  WEAPONSWING_UNUSED = -1
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
  QUESTGIVEREMOTENODE() : delay(0), emoteID(0) {
  }

  UINT delay;
  UINT emoteID;
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

  ~SPELLEFFECTDESC() {
    ClearLightningObjects();
  }
  SPELLEFFECTDESC();
  void ClearLightningObjects();

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

  void Clear();
  ~ACTIVEATTACHMENTINFO() {
    Clear();
  }
  ACTIVEATTACHMENTINFO() : inventoryType(0), flags(0), invSlot(-1), sheathAttachmentSlot(-1), displayInfo(0), enchantmentVisual(0) {
  }
  void Hide(CGUnit_C *unitPtr, HMODEL charModel, HMODEL paperDollModel, bool hide);
  void ClearAttachmentFromModel(HMODEL charModel, HMODEL paperDollModel);
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
 private:
  int  flags;
  UINT spellID;
  UINT effectID;
  union {
    HMODEL theModel;
    DWORD  obj;
  };

 public:
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
  HMODEL Model() const {
    return theModel;
  }
  void Clear();
  void Set(AuraVisual &visual) {
    Clear();
    flags = visual.flags;
    spellID = visual.spellID;
    effectID = visual.effectID;
    theModel = visual.theModel;
    visual.flags &= ~1;
    visual.theModel = 0;
  }
  void SetModel(HMODEL model);
  void SetWorldObject(DWORD object);
  void SetPermanent(bool permanent) {
    if (permanent) {
      flags |= 4;
    } else {
      flags &= ~4;
    }
  }
  HMODEL GetModel();
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
  friend BOOL MoveHeartBeatHandler(LPCVOID packetData, LPVOID param);
  friend int  Player_C_AppFocusMovementHandler(int focus);

 public:
  UINT GetUnitFlags() const {
    return m_unit->flags;
  }
  BYTE GetUnitNPCFlags() const {
    return m_unit->npcFlags;
  }
  BYTE IsAlive() const;
  BYTE IsDead() const {
    return m_unit->health <= 0;
  }
  int GetHealth() const {
    return m_unit->health;
  }
  float GetHealthPercent() const {
    return static_cast<float>(m_unit->health) / m_unit->maxHealth;
  }
  int GetPower(POWER_TYPE powerType) const {
    return powerType == -2 ? m_unit->health : m_unit->power[powerType];
  }
  int GetMaxPower(POWER_TYPE powerType) const {
    return powerType == -2 ? m_unit->maxHealth : m_unit->maxPower[powerType];
  }
  float GetPowerPercent(POWER_TYPE powerType) const;
  POWER_TYPE GetDisplayPower() const {
    return static_cast<POWER_TYPE>(m_unit->displayPower);
  }
  __forceinline int GetMaxHealth() const {
    return m_unit->maxHealth;
  }
  UINT GetMoney() const {
    return m_unit->coinage;
  }
  int GetLevel() const {
    return m_unit->level;
  }
  UINT GetMinDamage() const {
    return m_unit->minDamage;
  }
  UINT GetMaxDamage() const {
    return m_unit->maxDamage;
  }
  BOOL IsCombatLoggingActive() const;
  int GetCurrentStat(UINT statNumber) const {
    FATALASSERT(statNumber < (sizeof(m_unit->stats) / sizeof(m_unit->stats[0])));
    return m_unit->stats[statNumber];
  }
  int GetEffectiveStat(UINT stat) const;
  int GetBaseStat(UINT statNumber) const {
    FATALASSERT(statNumber < (sizeof(m_unit->baseStats) / sizeof(m_unit->baseStats[0])));
    return m_unit->baseStats[statNumber];
  }
  int GetResistance(UINT resistance) const {
    FATALASSERT(resistance < (sizeof(m_unit->resistances) / sizeof(m_unit->resistances[0])));
    return m_unit->resistances[resistance];
  }
  int GetEffectiveResistance(UINT resistance) const {
    FATALASSERT(resistance < (sizeof(m_unit->resistances) / sizeof(m_unit->resistances[0])));
    return m_unit->resistances[resistance] < 0 ? 0 : m_unit->resistances[resistance];
  }
  int GetResistanceBuffModPositive(UINT resistance) const {
    FATALASSERT(resistance < (sizeof(m_unit->resistanceBuffModsPositive) / sizeof(m_unit->resistanceBuffModsPositive[0])));
    return m_unit->resistanceBuffModsPositive[resistance];
  }
  int GetResistanceBuffModNegative(UINT resistance) const {
    FATALASSERT(resistance < (sizeof(m_unit->resistanceBuffModsNegative) / sizeof(m_unit->resistanceBuffModsNegative[0])));
    return m_unit->resistanceBuffModsNegative[resistance];
  }
  int GetResistanceItemMod(UINT school) const;
  UINT GetRace() const {
    return m_unit->race;
  }
  UINT GetClass() const {
    return m_unit->classId;
  }
  UNIT_SEX GetSex() const {
    return static_cast<UNIT_SEX>(m_unit->sex);
  }
  int GetModDamageDone(UINT school) const {
    return m_unit->modDamageDone[school];
  }
  int      GetModDamageTaken(UINT school) const;
  int      GetModCreatureDamageDone(UINT creatureType) const;
  const DWORDLONG &GetCharm() const {
    return m_unit->charm;
  }
  const DWORDLONG &GetSummon() const {
    return m_unit->summon;
  }
  const DWORDLONG &GetControlledGUID() const {
    return m_unit->charm ? m_unit->charm : m_unit->summon;
  }
  const DWORDLONG &GetCharmedBy() const {
    return m_unit->charmedBy;
  }
  BYTE             IsCharmedBy(const DWORDLONG &guid) const;
  BYTE IsCharmed() const {
    return m_unit->charmedBy != 0;
  }
  const DWORDLONG &GetSummonedBy() const {
    return m_unit->summonedBy;
  }
  BYTE             IsSummonedBy(const DWORDLONG &guid) const;
  BYTE             IsSummoned() const;
  const DWORDLONG &GetCreatedBy() const {
    return m_unit->createdBy;
  }
  BYTE             IsCreatedBy(const DWORDLONG &guid) const;
  BYTE             IsCreated() const;
  int GetCreatedBySpell() const {
    return m_unit->createdBySpell;
  }
  const DWORDLONG &GetControlGUID() const {
    return m_unit->charmedBy ? m_unit->charmedBy : m_unit->summonedBy;
  }
  const DWORDLONG &GetOwnerGUID() const {
    return m_unit->charmedBy ? m_unit->charmedBy : m_unit->createdBy;
  }
  BYTE IsPossessedBy(const DWORDLONG &guid) const {
    return IsPossessed() && guid == GetOwnerGUID();
  }
  BYTE IsPossessed() const {
    return (GetUnitFlags() >> 24) & 1;
  }
  float GetBoundingRadius() const;
  float GetCombatReach() const {
    return m_unit->weaponReach + m_unit->combatReach;
  }
  int GetDisplayID() const {
    return m_unit->displayID;
  }
  UINT                   GetMonsterItemDisplay(UINT slot) const;
  const VirtualItemInfo *GetMonsterItemInfo(UINT slot) const;
  UINT GetShapeshiftForm() const {
    return m_unit->shapeshiftForm;
  }
  UINT                   GetShapeshiftBit() const;
  BYTE                   IsChannelling() const;
  int GetChannelSpell() const {
    return m_unit->channelSpell;
  }
  DWORDLONG GetChannelObject() const {
    return m_unit->channelObject;
  }
  int ModCastSpeed() const {
    return m_unit->modCastingSpeed;
  }
  DWORDLONG GetComboTarget() const {
    return m_unit->comboTarget;
  }
  UINT GetComboPoints() const {
    return m_unit->comboPoints;
  }
  NTempest::C3Vector     GetPosition() const {
    return m_move.GetPosition();
  }
  void               GetPosition(NTempest::C3Vector &position) const;
  NTempest::C3Vector GetRawPosition() const;
  float              GetFacing() const;
  float              GetRawFacing() const;
  NTempest::C3Vector GetAnchorPosition() const;
  void               GetAnchorPosition(NTempest::C3Vector &position) const;
  float              GetAnchorFacing() const;
  float GetPitch() const {
    return m_move.GetPitch();
  }
  NTempest::C3Vector GetGroundNormal() const;
  float              GetRunSpeed() const;
  float              GetWalkSpeed() const;
  float              GetSwimSpeed() const;
  float              GetTurnRate() const;
  UINT                   GetMoveFlags() const {
    return m_move.GetMoveFlags();
  }
  BOOL IsInMotion() const;
  BOOL IsMovingOrTurning() const;
  BOOL IsMovingOrFalling() const;
  BOOL IsMoving() const;
  BOOL IsMovingOrStrafing() const;
  BOOL IsMovingTurningOrStrafing() const;
  BOOL IsMovingStrafingOrFalling() const;
  BOOL IsMovingForward() const;
  BOOL IsMovingBackwards() const;
  BOOL IsWalking() const;
  BOOL IsRunning() const;
  BOOL IsTurning() const;
  BOOL IsTurningLeft() const;
  BOOL IsTurningRight() const;
  BOOL IsStrafingLeft() const;
  BOOL IsStrafingRight() const;
  BOOL IsStrafing() const;
  BOOL IsFalling() const;
  BOOL IsImmobilized() const;
  int  Moved() const;
  DWORD GetMoveStartTime() const {
    return m_move.GetMoveStartTime();
  }
  NTempest::C3Vector GetRedirection() const;
  int                MoveTimeIsValid() const;
  BOOL               IsSwimming() const;
  BOOL               IsSwimmingOrFalling() const;
  BOOL               IsMovingStrafingOrSwimming() const;
  BOOL               IsMovingStrafingFallingOrSwimming() const;
  float GetCollisionBoxHeight() const {
    return m_move.GetCollisionBoxHeight();
  }
  int   IgnoresCollision() const;
  BOOL  IsHalted() const;
  void  BuildMovementUpdate(CDataStore *msg) const;
  float LinearDistanceSquared(const NTempest::C3Vector &position) const;
  int GetAura(int index) const {
    return m_unit->auras[index];
  }
  BYTE GetAuraFlags(int index) const {
    return (m_unit->auraFlags[index / 2] >> (4 * (index % 2))) & 0xF;
  }
  UINT GetAuraState() const {
    return m_unit->auraState;
  }
  BYTE  HasAuraState(UINT state) const;
  BYTE               IsDisconnected() const {
    return GetUnitFlags() & 1;
  }
  BYTE               IsSpawning() const {
    return (GetUnitFlags() >> 1) & 1;
  }
  BYTE IsClientLocked() const {
    return !IsSpawning() && (GetUnitFlags() & 0xC00004);
  }
  BYTE IsOnTaxi() const {
    return (GetUnitFlags() >> 20) & 1;
  }
  BYTE               IsPlayerControlled() const {
    return (GetUnitFlags() >> 3) & 1;
  }
  BYTE IsPlusMob() const {
    return (GetUnitFlags() >> 6) & 1;
  }
  BYTE IsBeastmaster() const;
  BYTE               IsImmunePC() const {
    return (GetUnitFlags() >> 8) & 1;
  }
  BYTE               IsImmuneNPC() const {
    return (GetUnitFlags() >> 9) & 1;
  }
  BYTE               IsLooting() const {
    return (GetUnitFlags() >> 10) & 1;
  }
  BYTE IsInCombat() const;
  BYTE               IsMounted() const {
    return (GetUnitFlags() >> 13) & 1;
  }
  BYTE      IsPureMountActive() const {
    return (GetUnitFlags() >> 12) & 1;
  }
  BYTE IsPureMountMounted() const;
  BYTE IsFeignDeath() const {
    return (GetUnitFlags() >> 14) & 1;
  }
  BYTE IsStealthed() const {
    return (GetUnitFlags() >> 15) & 1;
  }
  BYTE IsInvisible() const {
    return (GetUnitFlags() >> 16) & 1;
  }
  BYTE IsConfused() const;
  BYTE IsFleeing() const;
  BYTE IsAffectingCombat() const {
    return (GetUnitFlags() >> 19) & 1;
  }
  BYTE IsMerchant() const {
    return m_unit->npcFlags & 1;
  }
  BYTE IsQuestGiver() const {
    return m_unit->npcFlags & 2;
  }
  BYTE IsTaxiNode() const {
    return m_unit->npcFlags & 4;
  }
  BYTE IsTrainer() const {
    return m_unit->npcFlags & 8;
  }
  BYTE IsBinder() const {
    return m_unit->npcFlags & 0x10;
  }
  BYTE IsBanker() const {
    return m_unit->npcFlags & 0x20;
  }
  BYTE IsNpcPetition() const {
    return m_unit->npcFlags & 0x80;
  }
  BYTE IsTabardVendor() const {
    return m_unit->npcFlags & 0x40;
  }
  BYTE IsGuildRegistrar() const {
    return IsNpcPetition() && IsTabardVendor();
  }
  BYTE IsNPC() const {
    return IsMerchant() || IsQuestGiver() || IsTaxiNode() || IsTrainer() || IsBinder() || IsBanker() || IsNpcPetition() || IsTabardVendor();
  }
  int  GetMountDisplayID() const;
  DWORDLONG GetTarget() const {
    return m_unit->target;
  }
  UINT GetStandState() const {
    return m_unit->standState;
  }
  int  StandStateValid(UNITSTANDSTATE newState) const;
  BYTE IsSitting() const;
  BYTE IsSleeping() const;
  UINT GetEmoteState() const;
  UINT      GetPetNumber() const {
    return m_unit->petNumber;
  }
  UINT      GetPetNameTimestamp() const {
    return m_unit->petNameTimestamp;
  }
  BYTE       *GetData(UINT offset);
  static UINT GetDataSize();
  static UINT GetBaseOffset();
  static __forceinline UINT TotalFields() {
    return 184;
  }
  static UINT GetUpdateMaskBytes();
  static UINT GetUpdateMaskBlocks();
  void      SetStorage(DWORD *storage) {
    m_unit = reinterpret_cast<CGUnitData *>(storage);
  }
  UINT GetAttackRoundTime(COMBATHAND hand) const {
    FATALASSERT(hand<NUMHANDS);
    return m_unit->attackRoundBaseTime[hand];
  }
  WEAPONMODE GetWeaponMode() const {
    return static_cast<WEAPONMODE>(m_unit->weaponMode);
  }
  BYTE                    IsUsingRangedWeapon() const;
  BYTE                    GetSheathed() const;
  virtual UNITAFFILIATION GetGUIDAffiliation(DWORDLONG unit) const;
  void                    SetWaterSurfaceElevation(float elevation);

 protected:
  CGUnit(DWORD *storage, const NTempest::C3Vector &position, float facing, const DWORDLONG &guid)
      : m_unit(reinterpret_cast<CGUnitData *>(storage)), m_move(position, facing, guid) {
  }

  CGUnit(const CGUnit &);
  ~CGUnit() {
  }
  const CGUnitData *Unit() const;

  CGUnitData *Unit();

  CGUnitData   *m_unit;
  CMovementData m_move;

 private:
  CGUnit &operator=(const CGUnit &);
};

class CGUnit_C : public CGObject_C, public CGUnit {
  friend BOOL OnUpdateInventoryComponent(DWORDLONG guid, UINT offset, UINT bytes, LPCVOID prevValue, LPVOID param);
  friend class CGInputControl;
  friend class CGObject_C;
  friend class CGPlayer_C;
  friend struct ACTIVEATTACHMENTINFO;
  friend BOOL UnitHealthUpdateHandler(DWORDLONG unit, UINT offset, UINT bytes, LPCVOID oldValue, LPVOID param);
  friend BOOL OnUnitCombatEvent(LPVOID param, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
  friend void SetPortraitTexture(CSimpleTexture *texture, const CGUnit_C *unit);
  friend void CreatureQueryCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted);
  friend BOOL UnitModeUpdateHandler(DWORDLONG guid, UINT offset, UINT bytes, LPCVOID oldValue, LPVOID param);
  friend BOOL OnQuestUpdate(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);

  friend void MovementFixOutOfBoundsUnit(DWORDLONG guid);

 public:
  CGUnit_C(DWORD *storage, DWORD eventTime, CClientObjCreate *init);
  ~CGUnit_C();
  void InitializeExtendedDisplay();

  void         SetStorage(DWORD *storage);
  void         PostInit(const CClientObjCreate &init);
  void         PostMovementUpdate(const CClientMoveUpdate &update);
  virtual void Disable(int shutdown);
  virtual void Reenable();
  virtual void PostReenable();
  void         UpdateUnitCollisionBox(HMODEL model, LPCSTR modelFileName);
  virtual BOOL IsSolidSelectable() const;
  virtual BOOL IsSolidCollidable() const;
  virtual BOOL CanHighlight() const;
  virtual BOOL CanBeTargetted() const;
  virtual void OnLeftClick();
  virtual void OnRightClick();

  virtual LPCSTR GetObjectName() const;
  void           SetCreatureStats(const CreatureStats_C *stats);

 protected:
  virtual BOOL ShouldFadeIn() const;
  void         UpdateUnitAlpha();

 public:
  bool IsClientControlled() const {
    return IsPossessed() || (IsA(ID_PLAYER) && !IsCharmed() && !IsClientLocked() && !IsDisconnected());
  }

  static void Initialize();
  static void Shutdown();
  void        UnitInitializeModel(HMODEL model);
  void        UnitInitializeMountModel(HMODEL model);
  void        UnitUninitializeModel(HMODEL model);
  static void PostShutdown();
  static void NamePlateShow(int show);
  static void RemoveAllNamePlates();
  static void UpdateUnitNameplates(CGWorldFrame *worldFrame);
  static void ResortAllUnitNameplates(CGWorldFrame *worldFrame);

  virtual void      GetAFKText(char *buffer, int size) const;
  virtual void      GetDNDText(char *buffer, int size) const;
  virtual void      GetGMText(char *buffer, int size) const;
  int               GetCreatureType() const;
  void              SetLocalTarget(DWORDLONG target);
  virtual DWORDLONG GetLocalTarget() const;
  void              HandleAnimEvent(LPCSTR eventName, const NTempest::C3Vector &pos);
  void              HandleMountedAnimEvent(LPCSTR eventName, const NTempest::C3Vector &pos);
  virtual void      HandleSpellEventSound();
  virtual void      CombatLoggingFlagChanged();
  virtual DWORDLONG GetUnitBeingLooted() const;
  BOOL              CanBeLooted(DWORD currentTime) const;
  static void       SetActiveMover(const DWORDLONG &guid);

  static DWORDLONG GetActiveMover() {
    return m_activeMover;
  }

 protected:
  static DWORDLONG m_activeMover;

 public:
  virtual NTempest::C3Vector GetPosition() const;
  virtual void               GetPosition(NTempest::C3Vector &vec) const;
  virtual float              GetFacing() const;
  virtual NTempest::C3Vector GetGroundNormal() const;
  float                      GetBoundingRadius() const;
  bool                       IsTurningState() const;
  void                       SetMirrorHandlers();
  void                       UnsetMirrorHandlers();
  BOOL                       OnMoveEvent(NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
  void                       OnMonsterMove(DWORD eventTime, CDataStore *msg);
  BOOL                       OnForceMoveChange(DWORD eventTime, NETMESSAGE msgID, CDataStore *msg);
  void                       OnMoveStopLocalNoUpdate(DWORD eventTime);
  void                       OnStrafeStartLocalNoUpdate(DWORD eventTime, int left);
  void                       OnStrafeStopLocalNoUpdate(DWORD eventTime);
  void                       OnSetRunModeLocalNoUpdate(DWORD eventTime, int run);
  void                       OnSetFacingLocalNoUpdate(DWORD eventTime, float facing);
  void                       OnSetFacingGUIDLocalNoUpdate(DWORD eventTime, const DWORDLONG &guid);
  void                       OnTeleportLocalNoUpdate(DWORD eventTime, const NTempest::C3Vector &position, float facing);
  void                       OnTeleportNoUpdate(DWORD eventTime, const NTempest::C3Vector &position, float facing);
  void                       OnEnableCollisionLocalNoUpdate(DWORD eventTime);
  void                       OnDisableCollisionLocalNoUpdate(DWORD eventTime);
  void                       OnToggleCollisionLocal(DWORD eventTime);
  void                       OnPendingMoveStateChange(NETMESSAGE msgId);
  void                       OnCollideFalling(DWORD eventTime);
  void                       OnCollideFallLand(DWORD eventTime);
  void                       BuildMovementUpdate(NETMESSAGE messageId, CDataStore *msg) const;
  void                       SendMovementUpdate(NETMESSAGE messageId);
  void                       UpdateSmoothFacing();
  float                      GetSmoothFacing() const;
  float                      GetRawSmoothFacing() const {
    return m_smoothFacing;
  }
  void        SetSmoothFacing(float facing);
  void        UpdateSwimmingStatus(DWORD eventTime, int inWater, float depth);
  void        SendRedirectionMessage();
  static void StopMoveHeartbeatTimer();
  static void StartMoveHeartbeatTimer();

 protected:
  void UpdateDisplayFacing();
  BOOL ShouldShuffle() const;

 public:
  float        GetDisplayFacing() const;
  void         OnRestoreHealth();
  void         UpdateMoveInfo(DWORD eventTime, const CClientMoveUpdate &update);
  void         SetClientInitData(DWORD eventTime, const CClientObjCreate &init, bool partialUpdateOfActivePlayer);
  void         PostSetClientInitData(const CClientMoveUpdate &update);
  BOOL         IsWalking() const;
  virtual void GetWorldMatrix(NTempest::C34Matrix *worldMatrix) const;

 protected:
  void GetSwimMatrix(NTempest::C34Matrix *worldMatrix) const;

 public:
  void                OnAttackSwing(DWORDLONG victimGUID, UINT clientTimeStamp);
  virtual void        StopAttack();
  virtual void        OnAttackStart(DWORDLONG victim);
  virtual void        OnAttackStop(DWORDLONG previousTarget, int nowDead);
  virtual void        OnDeath();
  void                SaveQuestAddItemMessage(int killed, int needed);
  void                ProcessQuestItemMessages();
  bool                DoNotLogDeath() const;
  void                AdjustVictimState(ATTACKROUNDINFO *roundInfo);
  MISS_REASON         AdjustVictimState(MISS_REASON reason);
  virtual void        OnDeathAnimate();
  void                InitializeResEffectModel();
  void                ClearResEffectModel();
  void                AttachResEffectModel();
  void                DetatchResEffectModel();
  void                ShowPlayerXPGained();
  virtual void        OnGetAttacked(DWORDLONG attacker);
  virtual void        OnBadAttackFacing(DWORDLONG victimGUID);
  virtual void        OnBadAttackTarget(DWORDLONG victim);
  virtual void        OnBadAttackPosition(DWORDLONG victimGUID, float range);
  virtual void        OnNotStanding(DWORDLONG victim);
  void                OnEncounter(AI_REACTION reaction);
  void                QueueBloodSplat(BLOODSPURTLOCATION linkPoint);
  void                HandleBloodPool(UINT currentTime);
  BOOL                HasBloodRec() const;
  const UnitBloodRec *GetBloodRecord();
  void                AddBloodPool();
  void                RemoveBloodPool();
  virtual void        UnitHit(VICTIMSTATES state, DWORDLONG attacker);
  void                GetResistanceAndBuffs(int r, int &realResistance, int &effectiveResistance, int &buffPositive, int &buffNegative) const;
  BOOL                IsUnderWater() const;
  void                SetForcedAnimation(LPCSTR string);
  void                ResetForcedAnimation();
  void                ForceUpdateBaseAnimation();
  void                SetVictimAnimation(VICTIMSTATES newState, int unitDead, int criticalHit, UINT victimRoundDuration, int processNow);
  bool                QueueVictimAnim(VICTIMSTATES newState, int unitDead, int criticalHit, UINT victimRoundDuration);
  DWORDLONG IsAttacking() const {
    return m_combat.IsAttacking();
  }
  DWORDLONG    IsAttackingNow() const;
  void         ClearAttackSent();
  virtual void OnAttackerStateChange(const ATTACKROUNDINFO &roundInfo);
  virtual void HandleMirrorTimerDamage(const MIRRORTIMERDAMAGE &log);
  void         DoVictimFeedback(const ATTACKROUNDINFO *roundInfo, int showAnimation);
  void         ShowWorldText(const ATTACKROUNDINFO *roundInfo);
  void         PerformSpellProcImpact(int spell);
  void         AddVictimDeathHold(CGUnit_C *victimPtr);
  void         SetMeleeDeathHold(const CGUnit_C *victimPtr);
  void         ClearMeleeDeathHold();

 private:
  void PrintAttackSeqErrorMsg(UINT sequence, UINT fallBack) const;

  int       m_questCountKilled;
  int       m_questCountNeeded;
  HMODEL    m_resEffectModel;
  DWORDLONG m_meleeTargetDeathHold;
  int       m_precastSheatheHoldTimer;

 protected:
  virtual BOOL   QueueAnim(ANIMQUEUETYPE type, const ATTACKROUNDINFO *roundInfo);
  ANIMQUEUENODE *ProcessAnimQueue();
  void           PurgeAnimNodes(bool doNotProcess);
  virtual void   ProcessDiscardedAnim(ANIMQUEUENODE *node, bool doNotProcess);
  virtual void   ProcessAnim(ANIMQUEUENODE *node);
  ANIMQUEUENODE *GetNewAnimNode(int leaveUnlinked);
  void           RecycleAnimNode(ANIMQUEUENODE *node);
  void           CheckPendingVictimFeedback();
  void           CheckPendingMissileRelease(const NTempest::C3Vector *position);

 public:
  virtual void PlayUnitSound(UNITSOUNDTYPE soundType, int alwaysPlay) const;
  virtual void PlayFoleySound() const;
  void         PlayParrySound(bool ignoreMainHand, const ATTACKROUNDINFO *roundInfo, const NTempest::C3Vector &position) const;
  void         PlayImpactSound(DWORDLONG attacker, int criticalHit, COMBATHAND hand) const;
  void         PlayCustomAttackSound(int sound, const NTempest::C3Vector &position);
  void         SetCustomAttackSound(int sound, const NTempest::C3Vector &position);
  void         PlayDeathThud() const;
  void         PlaySplashSound(const NTempest::C3Vector &position);

 protected:
  virtual UINT GetImpactType() const;

  int                m_customAttackSound;
  NTempest::C3Vector m_customAttackPosition;
  UINT               m_splashSoundID;

 public:
  const VirtualItemInfo         *GetParryingItem(bool ignoreMainHand) const;
  virtual const VirtualItemInfo *GetDefendingItem() const;
  const VirtualItemInfo         *GetAttackingWeapon(COMBATHAND hand) const;
  bool                           GetWeaponSwingType(bool mainHand, WEAPONSWING_SOUNDTYPES &type);
  virtual void                   PlayDeathThudCameraShake() const;
  int                            GetUnitSize() const;
  void                           PlayStandSound() const;
  void                           WoundAnimEndHandler();
  void                           DodgeAnimEndHandler();
  void                           AttackAnimEndHandler();
  virtual void                   LootAnimEndHandler();
  void                           DeathAnimEndHandler();
  void                           NPCAnimEndHandler();
  void                           RangedWeaponAnimEndHandler();
  void                           ThrowAnimEndHandler();
  int                            JumpTakeOffFinishedHandler();
  int                            JumpLandFinishedHandler();
  void                           GenericAnimEndHandler(ANIMENUMERATION animID, LPVOID param);
  void                           InstallSeqEndHandler(HMODEL model, UINT animID);
  virtual void                   RestoreUnit();
  virtual LPCSTR                 GetModelFileName() const;
  virtual void                   UpdateBaseAnimation(UINT flags);
  void                           UpdateBaseAnimation(UINT newState, UINT flags);
  void                           UpdateMountAnimation(UINT newState, UINT flags);
  bool                           BaseAnimLocksHead() const;
  bool                           TorsoAnimLocksHead() const;
  bool                           TorsoAnimOverridesBase() const;
  bool                           UnitHeadLocked() const;
  virtual void                   StartSpellFizzleTimer(int spellID, UINT castingTime, int animSet);
  void                           SpellDelayed(int delay);
  void                           StopSpellFizzleTimer(int spellID, BYTE status);
  void                           StopRangedAttackPrecast();
  void                           EndSpellEffects(BYTE status);
  void                           MaybeSaveChannelSpellTargets(int spellID, const TSStackArray<DWORDLONG> &targets);
  bool                           CheckAndReportSpellInhibitFlags(const SpellRec *spell, const CGItem_C *item);
  void                           PendingPrecastInterrupt(int spellID);
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

  void                 PlaySpellLoopedSound(int soundID);
  virtual void         SetTorsoAnimState(UINT newState);
  virtual void         SetBaseAnimState(UINT newState);
  void                 SetBaseAnim(UINT newAnim);
  void                 SetTorsoAnim(UINT newAnim);
  int                  SetTorsoAnimation(UINT state, DWORD duration, UINT flags);
  int                  ClearTorsoAnimation(UINT flags);
  BOOL                 IsPreemptableWoundAnimState(UINT state);
  BOOL                 IsAttackAnimState(UINT state);
  BOOL                 ShouldDelayLevelupAnim();
  BOOL                 ShouldDelayLevelupAnim(UINT state);
  static bool          FactionHasReputation(int faction);
  int                  GetFactionTemplate() const;
  UNIT_REACTION        UnitReaction(const CGUnit_C *unit) const;
  static UNIT_REACTION UnitReaction(int factionID, const CGUnit_C *unit, int trueSight);
  bool                 IsFriend(const CGUnit_C *unit) const;
  bool                 IsPeaceful(const CGUnit_C *unit) const;
  bool                 IsEnemy(const CGUnit_C *unit) const;
  bool                 CanAssist(const CGUnit_C *unit) const;
  bool                 CanInteract(const CGUnit_C *unit) const;
  bool                 CanInteract(const CGGameObject_C *object) const;
  bool                 CanAttack(const CGUnit_C *unit) const;
  bool                 CanAttackNow(const CGUnit_C *unit) const;
  bool                 CanCooperate(const CGUnit_C *unit) const;
  bool                 IsUnitInGroup(const CGUnit_C *unit) const;
  BOOL                 IsStunned() const;
  BOOL                 IsPacified() const;
  BOOL                 IsDisarmed() const;
  BOOL                 IsPlayingDeathAnim() const;
  BOOL                 IsPlayingLayDownAnim() const;
  BOOL                 IsPlayingSleepAnim() const;
  BOOL                 IsPlayingGetUpAnim() const;
  void                 UpdateDisplay(DWORD now);
  void                 CheckRendering();
  void                 OnStopRender();
  void                 LookAtTarget();

 private:
  void LookAtTarget(CGUnit_C *target);

 public:
  void UpdateLookAtTarget();
  void SetDead();

 private:
  void RemoveObjectLookAt();
  void ApplyObjectCameraSpaceLookAt(const NTempest::C3Vector &target);

  UINT m_disengageLookAtTimer;

 protected:
  UINT  GetAnimationState();
  int   PlayBaseAnimation(int newAnimState, int newAnim, int forceNoFidget, bool &checkImpacts);
  void  ApplyStrafeRotation(UINT newState);
  void  SetStrafeRotation();
  BOOL  SetTorsoSequence(float timeScale, int flags);
  float GetAnimTimeScale(UINT sequence, UINT duration, UINT flags);
  void  StoreSequenceEndCallbacks(int anim);
  void  ProcessAnimEndCallbacks();
  void  ClearAnimCallbackData();

  TSGrowableArray<ANIMENDDATA> m_animEndCallbackList;
  ANIMENDDATA                 *m_callbackList[135];

  void               ShowBloodSpurt(CGUnit_C *attacker, int crushingBlow);
  BLOODSPURTLOCATION DetermineBloodLinkPoint(CGUnit_C *attacker);

 public:
  BOOL               SetBlock(UINT i, DWORD data);
  void               SetData(LPCVOID data, UINT bytes);
  static UINT        OffsetOf(OBJECT_TYPE_ID type);
  BOOL               HasInteractIcon();
  void               RefreshInteractIcon();
  void               RemoveInteractIcon();
  void               UpdateInteractIcon(QUEST_GIVER_STATUS status);
  void               UpdateInteractIcon(INTERACTICONTYPE which);
  QUEST_GIVER_STATUS GetQuestGiverStatus();
  void               SetQuestGiverStatus(QUEST_GIVER_STATUS status);
  void               EnableWeaponTrail(const NTempest::CImVector &color, int fadeOutRate, UINT duration);
  int                GetDebugStateInfo(ATTACKROUNDINFO *attackInfo);
  void               SetDebugHitRolls(const ATTACKROUNDINFO &info);
  void               ClearDebugFlags();
  void               SetUnitBadFacing();
  BOOL               IsBadFacing();
  BOOL               IsDeathFlagSet() const {
    return (m_animFlags & 0x2000) != 0;
  }
  void RemoveForceDisplayFacingFlag();
  void DetermineReadySequence(bool forceNormal);
  void UpdateReadyAnim(const ItemStats *stats);
  UINT GetRunSequence() const;
  UINT GetStopSequence() const;
  UINT GetReadySequence() const {
    FATALASSERT(m_readySequence != 0xffffffff);
    return m_readySequence;
  }
  UINT GetRangedReadySequence() const;
  void AddDeathHold();
  void DelDeathHold();
  UINT GetDeathHolds() const;
  void AddDamageDone(UINT damage, int normalCombatDamage, UINT flags, DWORDLONG attacker, int spellID);
  void SpellEventHit();
  bool IsSlotComponented(UINT offset, int ignoreUsingRangedWeapon);
  void ProcessLocalMoveEvent(NETMESSAGE msgId);

 protected:
  void  SetFingersSeq(HMODEL charModel, UINT sequence, UINT startFinger, UINT lastFinger);
  void  ResetFingersSeq(HMODEL charModel, UINT startFinger, UINT lastFinger);
  void  SetHandState(HMODEL model, const VirtualItemInfo *item, UINT startFinger, UINT lastFinger);
  void  SetHandsState(HMODEL model);
  void  OnTeleport(DWORD eventTime, const CMovementStatus &update);
  void  OnMoveStart(DWORD eventTime, const CMovementStatus &update, int forward);
  void  OnMoveStop(DWORD eventTime, const CMovementStatus &update);
  void  OnStrafeStart(DWORD eventTime, const CMovementStatus &update, int left);
  void  OnStrafeStop(DWORD eventTime, const CMovementStatus &update);
  void  OnJump(DWORD eventTime, const CMovementStatus &update);
  void  OnTurnStart(DWORD eventTime, const CMovementStatus &update, int left);
  void  OnTurnStop(DWORD eventTime, const CMovementStatus &update);
  void  OnPitchStart(DWORD eventTime, const CMovementStatus &update, int up);
  void  OnPitchStop(DWORD eventTime, const CMovementStatus &update);
  void  OnSetRunMode(DWORD eventTime, const CMovementStatus &update, int run);
  void  OnSetFacing(DWORD eventTime, const CMovementStatus &update);
  void  OnSetPitch(DWORD eventTime, const CMovementStatus &update);
  void  OnToggleCollision(DWORD eventTime, const CMovementStatus &update);
  void  OnRunSpeedChange(DWORD eventTime, const CMovementStatus &update, CDataStore *msg);
  void  OnWalkSpeedChange(DWORD eventTime, const CMovementStatus &update, CDataStore *msg);
  void  OnSwimSpeedChange(DWORD eventTime, const CMovementStatus &update, CDataStore *msg);
  void  OnTurnRateChange(DWORD eventTime, const CMovementStatus &update, CDataStore *msg);
  void  OnTeleportAck(DWORD eventTime, const CMovementStatus &update);
  void  OnSwimStart(DWORD eventTime, const CMovementStatus &update);
  void  OnSwimStop(DWORD eventTime, const CMovementStatus &update);
  void  OnMoveHeartBeat(DWORD eventTime, const CMovementStatus &update);
  void  InitializeSequenceFlags();
  float DetermineWalkRunTimeScale(int currentState);

 private:
  BOOL SetAttackerAnimation(const ATTACKROUNDINFO *roundInfo, int processNow);
  void AddDamageTimer(DWORDLONG attacker, VICTIMSTATES state, int unitDead, UINT duration, float delay, int criticalHit, int crushingBlow);

 public:
  void CheckPendingThrownWeaponReattach(bool force);
  BOOL ShouldReattachThrownWeapon() const;
  void SetReattachThrownWeapon(int reattach);
  UINT GetCurrentBaseAnimState() const {
    return m_currentBaseAnimState;
  }
  UINT GetCurrentTorsoAnimState() const {
    return m_currentTorsoAnimState;
  }
  UINT GetCurrentBaseAnim() const {
    return m_currentBaseAnim;
  }
  UINT GetCurrentTorsoAnim() const;

 protected:
  UINT                        GetFlags() const;
  int                         GotRangedWeaponRelease() const;
  UINT                        ChooseAnimation(UINT state) const;
  int                         CurrentAnimIncludesHit() const;
  UINT                        GetAttackerAnimEx(COMBATHAND hand, const VirtualItemInfo *itemInfo) const;
  UINT                        DetermineAttackerSequence(COMBATHAND hand) const;
  UINT                        DetermineParrySequence() const;
  virtual UINT                DetermineWoundSequence() const;
  const CreatureSoundDataRec *GetSoundData() const;
  const CreatureSoundDataRec *GetMountSoundDataRec() const;

  const CreatureStats_C             *m_stats;
  const CreatureDisplayInfoRec      *m_displayInfo;
  const CreatureDisplayInfoExtraRec *m_displayInfoExtra;
  const CreatureModelDataRec        *m_modelData;
  const CreatureSoundDataRec        *m_soundData;
  const CreatureSoundDataRec        *m_mountedSoundData;
  const UnitBloodLevelsRec          *m_bloodRec;

 public:
  void HandleCombatAnimEvent(LPCSTR eventName, DWORD value, const NTempest::C3Vector &position);

 protected:
  void HandlePlayStandSound(DWORD code, LPCSTR eventName);
  void FootstepAnimEventHit(const NTempest::C3Vector &position, BOOL isLeftFoot);
  void HandleFootstepAnimEvent(const NTempest::C3Vector &position);
  void HandleFootfallAnimEvent(const NTempest::C3Vector &position);
  void PlayFidgetSound(UINT fidgetNumber);
  void SetupFootprints();
  BOOL IsSplashing(const NTempest::C3Vector &position);
  void MarkSwimAnimations();
  void MarkFootstepAnimations(HMODEL model);
  void QueryModelStats();
  void QueryMountModelStats();

 private:
  CGUnit_C &operator=(const CGUnit_C &);

 public:
  static void              InitializeTextureVariations(const CreatureDisplayInfoRec *displayInfo, HMODEL theModel, const CreatureModelDataRec *modelData);
  bool                     SetSpellPreCastingAnimation(ANIMENUMERATION anim);
  bool                     SetSpellCastingAnimation(ANIMENUMERATION anim, UINT castKit, UINT soundID, int shakeID, ANIMENUMERATION &result);
  void                     ClearSpellCastAnimInfo();
  void                     SetSpellImpactKit(const SpellVisualKitRec *impactKit);
  const SpellVisualKitRec *GetRangedSpellAnim(int id, bool castKit);
  void                     HandleCastAnimEvent();
  void                     OnAuraChanged(UINT slot, int previousValue);
  virtual void             OnFlagChanged(UINT oldFlags);

 protected:
  void            SetAuraMirrorHandlers();
  void            UnsetAuraMirrorHandlers();
  void            SetAuraMirrorHandler(UINT slot, int (*handler)(DWORDLONG, UINT, UINT, LPCVOID, LPVOID));
  void            UnsetAuraMirrorHandler(UINT slot, int (*handler)(DWORDLONG, UINT, UINT, LPCVOID, LPVOID));
  void            RemoveAuraEffect(UINT slot, int previousSpell);
  void            AddAuraEffect(UINT slot, bool startNow);
  void            RemoveAuraVisual(UNITEFFECTATTACHPPOINT attach);
  void            MaybeAttachAura(UNITEFFECTATTACHPPOINT attach, UINT effect, UINT spellID, int priority, bool permanent);
  void            FinishAuraDecays();
  bool            IsSpellAuraAnimActive(int &anim) const;
  bool            IsSpellChannelAnimActive(int &anim) const;
  ACTIVEAURAINFO *FindActiveAuraInfo(int slot);
  void            RefreshAuraVisuals();
  void            AddPendingShapeshiftEffect(int oldSpell);
  void            AddKitAuras(const SpellVisualKitRec *kitRec, const SpellRec *spellRec);
  void            ClearChannelAuraInfo();

  AuraVisual      m_auraVisual[12];
  LISTDECL(ACTIVEAURAINFO, m_activeAuraInfo);
  ANIMENUMERATION m_pendingImpactAnim;

 public:
  void                           UpdateMovementAnimSpeed(int forMount, int currentState);
  virtual BOOL                   GetSelectionHighlightColor(NTempest::CImVector *outPtr) const;
  void                           PerformLevelUpAnim(int force);
  void                           CheckLevelUpAnimFlag(int oldState, int newState);
  virtual const VirtualItemInfo *GetVirtualItem(UINT slot, bool ignoreDisarmFlag) const;
  virtual int                    GetVirtualItemDisplayID(UINT slot) const;
  void                           SetRangedWeaponPullAnim(int duration);
  void                           SetRangedWeaponReleaseAnim();
  void                           ThrownMissileReleased();
  void                           AttachVirtualComponent(UINT slot, bool deferApply);
  void                           AttachVirtualMonsterWeapons();
  void                           VirtualComponentChanged(int slot, int oldValue);
  void                           DetachVirtualComponent(int vslot, bool defer, bool removeRecord);
  void                           SetDebugPathPosition(const NTempest::C3Vector &position);
  void                           RenderDebugPathing();
  void                           InitializeUnitName();
  void                           ShutdownWorldName();
  void                           UpdatePlayerNameWorldText();
  void                           UpdatePlayerNameColor();
  virtual BOOL                   ShouldRenderUnitName(UINT mode) const;
  void                           TriggerPlayerNameUpdate();
  virtual void                   CommitTexture(int force);
  LPCSTR                         GetUnitName() const;
  LPCSTR                         GetUnitTitle() const;
  virtual UINT                   UpdateUnitNameString(UINT localPlayerFlags, UINT otherUnitsFlags, char *buffer, UINT bufferSize) const;
  void                           AddWorldDamageText(UINT damage, int normalCombatDamage);
  void                           AddWorldCritText(UINT damage, int normalCombatDamage);
  void                           AddWorldText(WORLDTEXTMISSTYPE type);
  void                           AddWorldText(MISS_REASON reason);
  void                           AddWorldXPGainText(int xpGain);
  void                           PlayerNameVisibilityChanged(int nameVisible);
  void                           StoreXPGain(int XP);
  virtual void                   OnPickNextStandHandler();
  void                           PickNextRunHandler();
  void                           SpellAnimEndHandler();
  void                           RangedPrecastEndHandler();
  BOOL                           SetCastingSpell(int spellID, bool force, bool precastAnimSuccessful);
  int GetCastingSpell() const {
    return m_castingSpell;
  }
  void               KillSpellLoopedSound();
  void               KillCreatureLoopSound();
  void               InitializeLoopSound();
  UNITEFFECTSPECIALS DetermineBreathEffect(UINT *duration);
  void               BreathHandler(int forceOnMount);
  void               ProcessBreathParticles(int currentTime);
  virtual HMODEL     GetCharacterModel(int *mountedPtr) const;
  HMODEL             GetMountedModel() const;

  HMODEL        DuplicateCharacterModel(UINT flags) const;
  void          ClearMountAnimState();
  void          CreateUnitMount();
  virtual float GetMountScale() const;
  void          DestroyUnitMount(int doNotUpdateAnim);
  void          UpdateUnitMountInfo(int immediate, UINT changedFlags);
  int           UnitMountShowing() const;
  HMODEL        GetMountModel();
  void          DestroyFadingMounts();
  void          CreateFadeOutMount();
  void          CreateFadeInMount();
  void          UpdateFadingMountModel(const NTempest::C3Vector &cameraPos, const NTempest::C3Vector &cameraTarg);
  void          OnMountCancelled();
  virtual void  UpdatePlayerName();
  UINT          GetPlayerNameAttachmentPoint();
  virtual void  OnSpecialMountAnim();
  virtual void  OnMount();
  virtual void  OnDismount();
  void          OnCharmedChanged();
  virtual bool  CanBeMounted();
  void          ClearTempCharModel();
  void          SetTempCharModel(HMODEL model);

 protected:
  void GetFootprintInfo(UINT *id, NTempest::C2Vector *size);

  HMODEL m_tempCharModel;

 public:
  virtual BOOL  UpdateModelLoadStatus();
  virtual BOOL  UpdateAttachmentLoadStatus();
  virtual BOOL  UpdateTexComponentLoadStatus();
  virtual BOOL  ShouldRender(DWORD worldStatus);
  virtual void  UpdateRenderFacing();
  virtual float GetRenderFacing() const;
  virtual void  PreRender(int currentTime, float elapsed);
  virtual void  PreAnimate(CGWorldFrame *worldFrame);
  virtual void  PostAnimate(CGWorldFrame *worldFrame);
  virtual void  ObjectPostAnimate(const NTempest::C34Matrix &matrix, const NTempest::C3Vector &cameraPos, const NTempest::C3Vector &cameraTarg);
  virtual void  RenderTargetSelection() const;
  void          BuildSelectionRotMatrix(NTempest::C44Matrix &matrix) const;
  void          SetTexComponentLoaded(int loaded);
  BOOL          IsTexComponentLoaded() const;
  void          ReinitializeWeaponTrails();

 private:
  float GetBaseRadius() const;
  void  UpdateBaseRadius(HMODEL model);
  void  AddUnitNamePlate(CGWorldFrame *worldFrame);
  void  InsertSortedNamePlate(struct NAMEPLATEDESC *desc);
  void  RemoveUnitNamePlate();

 public:
  int  PlayNPCSound(NPCSOUNDS sound, UINT index);
  void OnNPCHello();
  void OnNPCGoodbye();
  void DDADDLOG(DWORDLONG guid, LPCSTR string, LPCSTR file, UINT line);
  void DDDELLOG(DWORDLONG guid, LPCSTR string, LPCSTR file, UINT line);
  void DDGENLOG(DWORDLONG guid, LPCSTR string, LPCSTR file, UINT line);
  void DumpGeneralDeathHoldLog(HSLOG handle, TSGrowableArray<char> *stringBuffer) const;

 protected:
  void DDWRITELOG(LPCSTR buffer);

  TSGrowableArray<char> m_deathHoldBuffer;
  TSGrowableArray<UINT> m_deathHoldBufferIndices;
  int                   m_lastDeathTime;
  int                   m_nextDeathHoldCheckTime;

 public:
  BOOL DisplayInfoNeedsUpdate(int &playerModelChanged, int &wasPlayerModel) const;
  void UpdateDisplayInfo();

 protected:
  virtual void CleanupUnitArtwork(int playerModelChanged, BOOL wasPlayerModel);
  void         RefreshDataPointers();
  virtual void ReinitializeUnitArtwork();
  virtual void PostReinitializeArtwork();

 public:
  void         StandStateChanged(UINT oldState);
  void         NPCFlagChanged(UINT oldNPCFlags);
  virtual void OnStandStateChanged(UINT oldState, UINT newState);
  void         SitSleepAnimEndHandler();
  BOOL         IsPlayingSittingOrStandingAnim() const;
  virtual void ChangeStandState(UINT standState);
  int          PlayEmoteAnimation(UINT emoteID, int flags);
  void         RequestTalkEmote(TALKANIMATION talkAnim);
  UINT         GetEmoteAnimation(UINT emoteID) const;
  virtual void SetEmoteState(UINT emoteID);
  void         SetEmoteQueue(TSStackArray<QUESTGIVEREMOTENODE> &list);
  void         SetEmoteQueue(const QUESTGIVEREMOTENODE *list, UINT num);

 protected:
  int SetEmoteAnimation(UINT emoteID, int flags);

  TSGrowableArray<QUESTGIVEREMOTENODE> m_emoteQueue;

  void ProcessEmoteQueue();
  BOOL EmoteProcType(UINT emoteID, EMOTESPECPROCS &proc) const;

 public:
  void RegisterScript();
  void UnregisterScript();

 protected:
  HMODEL             m_interactIconModel;
  LISTDECL(BLOODSPLATNODE, m_bloodSplatNodes);
  UINT               m_nextAllowableBloodPool;
  LISTDECL(ANIMQUEUENODE, m_animQueue);
  CCombatClient      m_combat;
  ANIMQUEUENODE     *m_currentDamageInfo;
  UINT               m_readySequence;
  UINT               m_animEndTime;
  UINT               m_animBaseDuration;
  UINT               m_animStartTime;
  UINT               m_flags;
  UINT               m_animFlags;
  UINT               m_footprintTextureID;
  UINT               m_terrain;
  NTempest::C2Vector m_footprintSize;
  float              m_footprintParticleScale;
  DEBUGHITROLLINFO   m_hitInformation;
  ANIMENUMERATION    m_spellPrecastingAnim;
  ANIMENUMERATION    m_spellCastingAnim;
  ANIMENUMERATION    m_deferredPrecastAnim;
  int                m_animatingAura;
  UINT               m_emoteID;
  UINT               m_spellCastingEffectKit;
  UINT               m_spellCastingSoundID;
  int                m_spellCastingCameraShakeID;
  MISSILESTRUCT      m_spellMissileStruct;
  float              m_lastSentFacing;
  float              m_lastSentPitch;
  HPLAYERNAME        m_unitNameHandle;
  int                m_accumulatedXPDrop;
  int                m_castingSpell;
  int                m_interruptedSpell;
  int                m_lastSpellCastAnimTime;
  int                m_nextBreath;
  int                m_nextMountBreath;
  int                m_scriptRegistered;
  float              m_displayFacing;

 public:
  enum {
    NUM_SAVED_FACING_DELTAS = 4
  };

 protected:
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

 public:
  HCHARGEOSET GetGeosetHandle() const;
  HTEXCOMPONENT GetTexComponent() const {
    return m_texComponent;
  }
  BOOL        IsModelComponentable() const;
  UINT        GetDisplayRace() const;
  UINT        GetDisplaySex() const;
  LPCSTR      GetDisplayTextureName() const;
  UINT        SkinVariationID() const;
  UINT        FaceID() const;
  UINT        HairStyleID() const;
  UINT        HairColorID() const;
  UINT        FacialHairID() const;
  const UINT *GetPreferredGeosets() const;
  UINT        GetNumPreferredGeosets() const;
  void        InitPreferredGeosets();
  void        InitializeNPCItems();

 protected:
  HCHARGEOSET   m_geosetHandle;
  HTEXCOMPONENT m_texComponent;
  UINT          m_preferredGeosets[15];

 public:
  int                     GetDisplayHealth() const;
  void                    SignalDisplayHealthUpdate() const;
  void                    UpdateDisplayHealth();
  BOOL                    IsInStandSitTransition();
  BOOL                    IsInSitSleepPosition();
  virtual UNITAFFILIATION GetGUIDAffiliation(DWORDLONG unit) const;
  virtual int             GetSpellRank(int spellID) const;
  int          GetSpellLevel(int spellID) const {
    return static_cast<UINT>(GetSpellRank(spellID)) / 5;
  }
  virtual bool  GetDefenseSkillRank(int &base, int &modifier) const;
  virtual bool  GetAttackSkillRank(int hand, int &base, int &modifier) const;
  virtual void  OnLevelChange();
  virtual float GetBlockChance() const;
  virtual float GetDodgeChance() const;
  virtual float GetParryChance() const;

 protected:
  int m_displayHealth;

 public:
  const SkillLineAbilityRec *LookupAbility(int spellID) const;
  bool                       IsSpellKnown(int spellID) const;
  bool                       IsSpellSuperceded(int spellID) const;
  int                        GetSpellSkillLine(int spellID) const;
  virtual int                GetSpellCastingTime(int spellID) const;
  void                       SetImpactKitEffect(int spellID, CGUnit_C *target, const SpellVisualKitRec *impactKit, int immediate);
  void                       PlayImpactKit(int spellID, const SpellVisualKitRec *impactKit);
  void                       CheckPendingImpactKit();
  void                       AddSpellProcAuraEffect(int auraslot, const SpellVisualKitRec *rec);
  void                       AddSpellProcOneShotEffect(int spellID, const SpellVisualKitRec *rec);
  void                       RemoveSpellProcAuraEffect(ACTIVEAURAINFO *rec);
  SPELLEFFECTDESC           *FindSpellEffectProcDesc(const SpellVisualKitRec *rec);
  void                       RefreshSpellProcEffects();
  void                       UpdateSpellProcEffects(float elapsedTime);
  void                       AddEmissiveColor(const NTempest::CImVector &color);
  void                       RemoveEmissiveColor(const NTempest::CImVector &color);
  void                       SetStandStateAnim(int standAnim);
  void                       SetWalkStateAnim(int walkAnim);
  int                        GetStandStateAnim(HMODEL model) const;
  int                        GetWalkStateAnim() const;
  const SpellVisualRec      *GetAppropriateSpellVisual(const SpellRec *spellRec, SpellVisualRec &filled) const;
  SPELLEFFECTDESC           *GetActiveEffect(LIST(SPELLEFFECTDESC) & list);
  void                       AddHitAnimHolds(int spellID, const TSStackArray<DWORDLONG> &targets);
  void                       CheckPendingSpellAnimHits();
  void                       SpellAnimHit(int spellID);

 private:
  void InternalProcessSpellProcEffects(SPELLPROC_ACTION action, float elapsed);

  LISTDECL(IMPACTEFFECTDESC, m_impactEffectsDesc);
  LISTDECL(SPELLEFFECTDESC, m_spellEffectLists[11]);
  NTempest::C3iVector        m_currentEmissive;
  int                        m_pendingHitSpellID;
  TSGrowableArray<DWORDLONG> m_pendingHitAnimVictims;
  int                        m_auraFlags[56];
  int                        m_walkStateAnim;
  int                        m_standStateAnim;
  float                      m_baseRadius;

 public:
  static int GetAnimPriority(int state);
  void       OnDynamicFlagsChanged(UINT oldValue);
  void       OnChannelSpellChanged(UINT oldSpell);
  void       ClearSavedChannelSpellTargets();
  int  GetSavedChannelSpellID() const {
    return m_savedChannelSpellID;
  }
  const TSGrowableArray<DWORDLONG> &GetSavedChannelSpellTargets() const {
    return m_savedChannelSpellTargets;
  }
  bool IsBeingStalked() const {
    return (m_unit->dynamicFlags >> 1) & 1;
  }
  bool GetLootPermission() const;
  void DrawBowString(const NTempest::C3Vector &cameraPos);
  void ShowHandArrow(int show);
  void SetShowHandArrowFlag(int show);
  bool GetShowingHandArrow() const {
    return (m_flags >> 19) & 1;
  }
  void SetShowingHandArrowFlag(int showing);
  void      SetAmmoDisplay(UINT displayID, UINT inventoryType) {
    m_ammoDisplayID = displayID;
    m_ammoInvType = inventoryType;
  }
  HMODEL GetRangedWeaponModel();
  void   ClearRangedStandTimer();
  void   OnRangedStandTimer();
  void   SetRangedStandTimer();

 protected:
  UINT m_ammoDisplayID;
  UINT m_ammoInvType;
  UINT m_rangedStandTimer;

 public:
  virtual void UpdateObjComponentVisuals(const CGItem_C *item, const ItemEnchantment *enchantments, int num);
  virtual void ClearItemVisuals(ACTIVEATTACHMENTINFO *info);
  virtual void SetItemVisuals(ACTIVEATTACHMENTINFO *info, const ItemVisualsRec *rec, bool force);

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

  ACTIVEATTACHMENTINFO *CreateAttachmentInfo(
      int  invSlot,
      int  displayID,
      int  inventoryType,
      bool forceAlternate,
      bool sheathe,
      int  sheathedAttachmentPoint,
      bool showHidden
  );

  void RemoveObjectComponentByInvSlot(int invSlot, bool deferDeleteFromModel, bool removeRecord);
  bool SheatheObjComponent(int slot, bool sheathe);
  void ClearActiveAttachmentInfo();
  void SetAttachmentHidden(int attachmentSlot, bool hide);
  bool ApplyAttachmentInfo(HMODEL characterModel, bool sheathe, int attachmentSlot, bool force);
  bool UpdateVisibilitySlots(HMODEL characterModel, int attachmentSlot, ACTIVEATTACHMENTINFO **&found, int displayID, bool deferApply);
  bool WeaponAttached(COMBATHAND hand) const;
  void ClearWeaponTrailHandles();
  void ClearDeferredAttachment(HMODEL charModel, int slot);
  void RefreshAttachmentInfo(HMODEL model);

  ACTIVEATTACHMENTINFO *m_attachments[5];
  ACTIVEATTACHMENTINFO *m_deferredAttachments[5];
  int                   m_weaponTrails[5];

 public:
  void   ReinitializePaperdollModel();
  void   CreatePaperdollModel();
  HMODEL GetPaperDollModel(bool duplicateModel);
  void   DestroyPaperdollModel();

 protected:
  HMODEL m_paperDollModel;

 public:
  void         CheckDeferredSheathing();
  void         SetSheatheReason(SHEATHEREASONS reason, bool on, bool suppressSound);
  void         SheatheOrUnsheatheItems(SHEATHEREASONS reason, bool sheathe, bool playSound);
  void         MaybeStartSheatheAnim();
  void         DisableWeaponTrails();
  void         HandleSheatheAnimEvent(bool clearSheatheAnim, bool suppressSound);
  void         SheatheAnimEndHandler();
  bool         IsItemSwapFlagSet() const;
  void         SetItemSwapFlag(bool set);
  void         SetWeaponMode(WEAPONMODE mode);
  void         WeaponModeChanged();
  void         UpdateSheatheRangedReasons(bool suppressSound);
  virtual void SetLastWeaponModeSent(int mode);
  void         HandlePrecastStart(bool precast);
  void         HandlePrecastStop(int spellID, bool force);

 protected:
  bool SetSheathingSequence();
  void HandleRemotePlayerSheathing();
  void HandleLocalPlayerSheathing();
  bool SheatheAnimPlaying() const;
  bool SheatheAnimEventEncountered() const {
    return (m_animFlags >> 16) & 1;
  }
  void SetSheatheEventEncountered(bool encountered);

  int             m_sheatheReasons;
  ANIMENUMERATION m_handAnim[2];
  UINT            m_deferredSheatheFlags;
  SHEATHEREASONS  m_deferredSheatheReason;

 public:
  void      ClearTrackingTarget(bool snapToTargetOnClear);
  void      SaveTrackingTarget(DWORDLONG target, TRACKTYPE type, bool snapToTargetOnClear);
  DWORDLONG GetTrackingTarget() const;
  bool      TrackingTargetMoving() const;
  void      HandleFollowTarget();
  void      OnMovementInitiated(bool facingOnly);
  void      OnMoveStartLocal(DWORD eventTime, int forward);
  void      OnMoveStopLocal(DWORD eventTime);
  void      OnStrafeStartLocal(DWORD eventTime, int left);
  void      OnStrafeStopLocal(DWORD eventTime);
  void      OnTurnStartLocal(DWORD eventTime, int left);
  void      OnTurnStopLocal(DWORD eventTime);
  void      OnSetFacingLocal(DWORD eventTime, float facing);
  void      OnSetRawFacingLocal(DWORD eventTime, float facing);
  void      OnPitchStartLocal(DWORD eventTime, int up);
  void      OnPitchStopLocal(DWORD eventTime);
  void      OnSetPitchLocal(DWORD eventTime, float pitch);
  void      OnJumpLocal(DWORD eventTime);
  void      OnSwimStartLocal(DWORD eventTime);
  void      OnSwimStopLocal(DWORD eventTime);
  void      OnRunSpeedChangeLocal(DWORD eventTime, NETMESSAGE msgID, float speed);
  void      OnWalkSpeedChangeLocal(DWORD eventTime, float speed);
  void      OnSwimSpeedChangeLocal(DWORD eventTime, NETMESSAGE msgID, float speed);
  void      OnAllSpeedChangeLocal(DWORD eventTime, float speed);
  void      OnTurnRateChangeLocal(DWORD eventTime, float rate);
  void      OnSetRunModeLocal(DWORD eventTime, int run);
  void      ToggleRunModeLocal(DWORD eventTime);
  bool      IsShapeShifted() const;
  void      AttackUnit(CGUnit_C *newVictim);
  void      OnCombatModeTimer();

 protected:
  int                        m_savedChannelSpellID;
  TSGrowableArray<DWORDLONG> m_savedChannelSpellTargets;
  SPELLEFFECTDESC           *m_channelSpellEffect;
  const SpellRec            *m_shapeShiftPoof;

 public:
  void ClearFishingObject();
  void ProcessChannelObject();

 protected:
  FishingLineObject *m_fishingLineObject;
};

void CGUnit_C_RenderBowStrings(const NTempest::C3Vector &c);

void ClearSpecialEffects(HMODEL model);
