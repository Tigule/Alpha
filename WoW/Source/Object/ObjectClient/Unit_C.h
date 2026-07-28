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

void UnitUpdateMovementAnim(const unsigned __int64 &unit);
int UnitHealthUpdateHandler(unsigned __int64 unit, unsigned int offset, unsigned int bytes, const void *oldValue, void *param);
void OnMoveUpdate(unsigned __int64 unit, unsigned long eventTime);
int MoveHeartBeatHandler(const void *packetData, void *param);
int OnUnitCombatEvent(void *param, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
int Player_C_AppFocusMovementHandler(int focus);

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
  unsigned __int64 guid;
  ANIMENUMERATION  anim;
};
NODEDECL(IMPACTEFFECTDESC) {
  unsigned __int64   victim;
  unsigned __int64   attacker;
  const SpellVisualKitRec *impactKit;
  int                spellID;

  ~IMPACTEFFECTDESC();
  void Set(unsigned __int64 a, unsigned __int64 v, const SpellVisualKitRec *i, int s);
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
  unsigned int delay;
  unsigned int emoteID;
};
struct LightningObject;
struct FishingLineObject;

struct DEBUGHITROLLINFO {
  ATTACKROUNDINFO attackInfo;
  unsigned int    attackFlags;
  float           range;
};

enum PUREMOUNTFADEMODE {
  PUREMOUNTFADE_IN = 0,
  PUREMOUNTFADE_OUT = 1
};

NODEDECL(SPELLEFFECTDESC) {
  SpellVisualKitRec  *kitPtr;
  NTempest::CImVector color;
  float               scale;
  unsigned int        startTime;
  unsigned int        fadeInTime;
  unsigned int        fadeOutTime;
  unsigned int        endTime;
  unsigned int        curTime;
  float               period;
  int                 standAnim;
  int                 walkAnim;
  unsigned int        isOneShot;
  LightningObject    *lightningObjs[3];

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
  EMOTESPECPROC_0 = 0,
  EMOTESPECPROC_1 = 1,
  EMOTESPECPROC_2 = 2
};

struct ATTACHMENTMODELINFO {
  HMODEL model;
  int    attachmentPoint;
  int    currentLink;

  void ClearAttachmentFromModel(HMODEL charModel, HMODEL paperDollModel);
};

struct ACTIVEATTACHMENTINFO {
  int                 inventoryType;
  int                 flags;
  int                 invSlot;
  int                 sheathAttachmentSlot;
  const ItemDisplayInfoRec *displayInfo;
  const ItemVisualsRec     *enchantmentVisual;
  ATTACHMENTMODELINFO modelInfo[2];

  ACTIVEATTACHMENTINFO();
  ~ACTIVEATTACHMENTINFO();
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
  SHEATHEREASON_0 = 0,
  SHEATHEREASON_1 = 1,
  SHEATHEREASON_2 = 2,
  SHEATHEREASON_3 = 3,
  SHEATHEREASON_4 = 4,
  SHEATHEREASON_5 = 5,
  SHEATHEREASON_6 = 6,
  SHEATHEREASON_7 = 7,
  SHEATHEREASON_PRECAST = 7,
  SHEATHEREASON_8 = 8,
  SHEATHEREASON_NUMREASONS = 9
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

  void SetSpellID(unsigned int id) {
    spellID = id;
  }
  unsigned int GetSpellID() const {
    return spellID;
  }
  unsigned int HasArt() const {
    return flags & 1;
  }
  unsigned int IsWorldModel() const {
    return flags & 2;
  }
  unsigned int GetEffect() const {
    return effectID;
  }
  void SetEffect(unsigned int effect) {
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
  void SetWorldObject(unsigned long object);
  void SetPermanent(unsigned int permanent) {
    if (permanent) {
      flags |= 4;
    } else {
      flags &= ~4;
    }
  }
  HMODEL GetModel();

 private:
  int          flags;
  unsigned int spellID;
  unsigned int effectID;
  union {
    HMODEL        theModel;
    unsigned long obj;
  };
};

NODEDECL(ACTIVEAURAINFO) {
  int                slot;
  const SpellVisualKitRec *stateKitRec;
};

struct CGUnitData {
  unsigned __int64 charm;
  unsigned __int64 summon;
  unsigned __int64 charmedBy;
  unsigned __int64 summonedBy;
  unsigned __int64 createdBy;
  unsigned __int64 target;
  unsigned __int64 comboTarget;
  unsigned __int64 channelObject;
  int              health;
  int              power[4];
  int              maxHealth;
  int              maxPower[4];
  int              level;
  int              factionTemplate;
  unsigned char    race;
  unsigned char    classId;
  unsigned char    sex;
  unsigned char    displayPower;
  int              stats[5];
  int              baseStats[5];
  unsigned int     virtualItemDisplay[3];
  VirtualItemInfo  virtualItemInfo[3];
  unsigned int     flags;
  unsigned int     coinage;
  int              auras[56];
  unsigned char    auraFlags[28];
  unsigned int     auraState;
  int              modDamageDone[6];
  int              modDamageTaken[6];
  int              modCreatureDamageDone[8];
  unsigned int     attackRoundBaseTime[2];
  int              resistances[6];
  float            boundingRadius;
  float            combatReach;
  float            weaponReach;
  int              displayID;
  int              mountDisplayID;
  unsigned short   minDamage;
  unsigned short   maxDamage;
  int              resistanceBuffModsPositive[6];
  int              resistanceBuffModsNegative[6];
  int              resistanceItemMods[6];
  unsigned char    standState;
  unsigned char    npcFlags;
  unsigned char    shapeshiftForm;
  unsigned char    weaponMode;
  unsigned int     petNumber;
  unsigned int     petNameTimestamp;
  unsigned int     petExperience;
  unsigned int     petNextLevelExperience;
  unsigned int     dynamicFlags;
  unsigned int     emoteState;
  int              channelSpell;
  int              modCastingSpeed;
  int              createdBySpell;
  unsigned char    comboPoints;
  unsigned char    bytepad1;
  unsigned char    bytepad2;
  unsigned char    bytepad3;
  unsigned int     pad;
};

class CGUnit {
  friend class CGCamera;
  friend class CGGameObject_C_Type_Chair;
  friend class CGInputControl;
  friend void OnMoveUpdate(unsigned __int64 unit, unsigned long eventTime);
  friend int MoveHeartBeatHandler(const void *packetData, void *param);
  friend int Player_C_AppFocusMovementHandler(int focus);

 public:
  static unsigned int GetDataSize();
  static unsigned int GetBaseOffset();
  static unsigned int TotalFields();
  static unsigned int GetUpdateMaskBytes();
  static unsigned int GetUpdateMaskBlocks();
  virtual UNITAFFILIATION GetGUIDAffiliation(unsigned __int64 unit) const;

  unsigned int GetUnitFlags() const;
  unsigned char GetUnitNPCFlags() const;
  unsigned char IsAlive() const;
  unsigned char IsDead() const;
  int GetHealth() const;
  float GetHealthPercent() const;
  int GetPower(POWER_TYPE powerType) const;
  int GetMaxPower(POWER_TYPE powerType) const;
  float GetPowerPercent(POWER_TYPE powerType) const;
  POWER_TYPE GetDisplayPower() const;
  int GetMaxHealth() const;
  unsigned int GetMoney() const;
  int GetLevel() const;
  unsigned int GetMinDamage() const;
  unsigned int GetMaxDamage() const;
  int IsCombatLoggingActive() const;
  int GetCurrentStat(unsigned int stat) const;
  int GetEffectiveStat(unsigned int stat) const;
  int GetBaseStat(unsigned int stat) const;
  int GetResistance(unsigned int school) const;
  int GetEffectiveResistance(unsigned int school) const;
  int GetResistanceBuffModPositive(unsigned int school) const;
  int GetResistanceBuffModNegative(unsigned int school) const;
  int GetResistanceItemMod(unsigned int school) const;
  unsigned int GetRace() const;
  unsigned int GetClass() const;
  UNIT_SEX GetSex() const;
  int GetModDamageDone(unsigned int school) const;
  int GetModDamageTaken(unsigned int school) const;
  int GetModCreatureDamageDone(unsigned int creatureType) const;
  const unsigned __int64 &GetCharm() const;
  const unsigned __int64 &GetSummon() const;
  const unsigned __int64 &GetControlledGUID() const;
  const unsigned __int64 &GetCharmedBy() const;
  unsigned char IsCharmedBy(const unsigned __int64 &guid) const;
  unsigned char IsCharmed() const;
  const unsigned __int64 &GetSummonedBy() const;
  unsigned char IsSummonedBy(const unsigned __int64 &guid) const;
  unsigned char IsSummoned() const;
  const unsigned __int64 &GetCreatedBy() const;
  unsigned char IsCreatedBy(const unsigned __int64 &guid) const;
  unsigned char IsCreated() const;
  int GetCreatedBySpell() const;
  const unsigned __int64 &GetControlGUID() const;
  const unsigned __int64 &GetOwnerGUID() const;
  unsigned char IsPossessedBy(const unsigned __int64 &guid) const;
  unsigned char IsPossessed() const;
  float GetBoundingRadius() const;
  float GetCombatReach() const;
  int GetDisplayID() const;
  unsigned int GetMonsterItemDisplay(unsigned int slot) const;
  const VirtualItemInfo *GetMonsterItemInfo(unsigned int slot) const;
  unsigned int GetShapeshiftForm() const;
  unsigned int GetShapeshiftBit() const;
  unsigned char IsChannelling() const;
  int GetChannelSpell() const;
  unsigned __int64 GetChannelObject() const;
  int ModCastSpeed() const;
  unsigned __int64 GetComboTarget() const;
  unsigned int GetComboPoints() const;
  void GetPosition(NTempest::C3Vector &position) const;
  NTempest::C3Vector GetPosition() const;
  NTempest::C3Vector GetRawPosition() const;
  float GetFacing() const;
  float GetRawFacing() const;
  void GetAnchorPosition(NTempest::C3Vector &position) const;
  NTempest::C3Vector GetAnchorPosition() const;
  float GetAnchorFacing() const;
  float GetPitch() const;
  NTempest::C3Vector GetGroundNormal() const;
  float GetRunSpeed() const;
  float GetWalkSpeed() const;
  float GetSwimSpeed() const;
  float GetTurnRate() const;
  unsigned int GetMoveFlags() const {
    return m_move.GetMoveFlags();
  }
  int IsInMotion() const;
  int IsMovingOrTurning() const;
  int IsMovingOrFalling() const;
  int IsMoving() const;
  int IsMovingOrStrafing() const;
  int IsMovingTurningOrStrafing() const;
  int IsMovingStrafingOrFalling() const;
  int IsMovingForward() const;
  int IsMovingBackwards() const;
  int IsWalking() const;
  int IsRunning() const;
  int IsTurning() const;
  int IsTurningLeft() const;
  int IsTurningRight() const;
  int IsStrafingLeft() const;
  int IsStrafingRight() const;
  int IsStrafing() const;
  int IsFalling() const;
  int IsImmobilized() const;
  int Moved() const;
  unsigned long GetMoveStartTime() const {
    return m_move.GetMoveStartTime();
  }
  NTempest::C3Vector GetRedirection() const;
  int MoveTimeIsValid() const;
  int IsSwimming() const;
  int IsSwimmingOrFalling() const;
  int IsMovingStrafingOrSwimming() const;
  int IsMovingStrafingFallingOrSwimming() const;
  float GetCollisionBoxHeight() const;
  int IgnoresCollision() const;
  int IsHalted() const;
  void BuildMovementUpdate(CDataStore *msg) const;
  float LinearDistanceSquared(const NTempest::C3Vector &position) const;
  int GetAura(int index) const;
  unsigned char GetAuraFlags(int index) const;
  unsigned int GetAuraState() const;
  unsigned char HasAuraState(unsigned int state) const;
  unsigned char IsDisconnected() const;
  unsigned char IsSpawning() const;
  unsigned char IsClientLocked() const;
  unsigned char IsOnTaxi() const;
  unsigned char IsPlayerControlled() const;
  unsigned char IsPlusMob() const;
  unsigned char IsBeastmaster() const;
  unsigned char IsImmunePC() const;
  unsigned char IsImmuneNPC() const;
  unsigned char IsLooting() const;
  unsigned char IsInCombat() const;
  unsigned char IsMounted() const;
  unsigned char IsPureMountActive() const;
  unsigned char IsPureMountMounted() const;
  unsigned char IsFeignDeath() const;
  unsigned char IsStealthed() const;
  unsigned char IsInvisible() const;
  unsigned char IsConfused() const;
  unsigned char IsFleeing() const;
  unsigned char IsAffectingCombat() const;
  unsigned char IsMerchant() const;
  unsigned char IsQuestGiver() const;
  unsigned char IsTaxiNode() const;
  unsigned char IsTrainer() const;
  unsigned char IsBinder() const;
  unsigned char IsBanker() const;
  unsigned char IsNpcPetition() const;
  unsigned char IsTabardVendor() const;
  unsigned char IsGuildRegistrar() const;
  unsigned char IsNPC() const;
  int GetMountDisplayID() const;
  unsigned __int64 GetTarget() const;
  unsigned int GetStandState() const;
  int StandStateValid(UNITSTANDSTATE newState) const;
  unsigned char IsSitting() const;
  unsigned char IsSleeping() const;
  unsigned int GetEmoteState() const;
  unsigned int GetPetNumber() const;
  unsigned int GetPetNameTimestamp() const;
  unsigned char *GetData(unsigned int offset);
  void SetStorage(unsigned long *storage);
  unsigned int GetAttackRoundTime(COMBATHAND hand) const;
  WEAPONMODE GetWeaponMode() const;
  unsigned char IsUsingRangedWeapon() const;
  unsigned char GetSheathed() const;
  void SetWaterSurfaceElevation(float elevation);

 protected:
  CGUnit(const CGUnit &);
  CGUnit(
      unsigned long *storage,
      const NTempest::C3Vector &position,
      float facing,
      const unsigned __int64 &guid
  );
  ~CGUnit();

  CGUnitData *Unit();
  const CGUnitData *Unit() const;

 private:
  CGUnit &operator=(const CGUnit &);

 protected:
  CGUnitData *m_unit;
  CMovementData m_move;
};

class CGUnit_C : public CGObject_C, public CGUnit {
  friend class CGInputControl;
  friend class CGObject_C;
  friend struct ACTIVEATTACHMENTINFO;
  friend int UnitHealthUpdateHandler(
      unsigned __int64 unit,
      unsigned int offset,
      unsigned int bytes,
      const void *oldValue,
      void *param
  );
  friend int OnUnitCombatEvent(void *param, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);

 public:
  CGUnit_C(unsigned long *storage, unsigned long eventTime, CClientObjCreate *init);
  ~CGUnit_C();
  virtual void Disable(int shutdown);
  virtual void Reenable();
  virtual void PostReenable();
  virtual void PreRender(int currentTime, float elapsed);
  virtual void PreAnimate(CGWorldFrame *worldFrame);

  // CGUnit_C virtuals, in primary-vtable order.
  virtual void                   GetAFKText(char *buffer, int size) const;
  virtual void                   GetDNDText(char *buffer, int size) const;
  virtual void                   GetGMText(char *buffer, int size) const;
  virtual unsigned __int64       GetLocalTarget() const;
  virtual void                   HandleSpellEventSound();
  virtual void                   CombatLoggingFlagChanged();
  virtual unsigned __int64       GetUnitBeingLooted() const;
  virtual void                   StopAttack();
  virtual void                   OnAttackStart(unsigned __int64 victim);
  virtual void                   OnAttackStop(unsigned __int64 previousTarget, int nowDead);
  virtual void                   OnDeath();
  virtual void                   OnDeathAnimate();
  virtual void                   OnGetAttacked(unsigned __int64 attacker);
  virtual void                   OnBadAttackFacing(unsigned __int64 victimGUID);
  virtual void                   OnBadAttackTarget(unsigned __int64 victim);
  virtual void                   OnBadAttackPosition(unsigned __int64 victimGUID, float range);
  virtual void                   OnNotStanding(unsigned __int64 victim);
  virtual void                   UnitHit(VICTIMSTATES state, unsigned __int64 attacker);
  virtual void                   OnAttackerStateChange(const ATTACKROUNDINFO &roundInfo);
  virtual void                   HandleMirrorTimerDamage(const MIRRORTIMERDAMAGE &log);

 protected:
  virtual int                    QueueAnim(ANIMQUEUETYPE type, const ATTACKROUNDINFO *roundInfo);
  virtual void                   ProcessDiscardedAnim(ANIMQUEUENODE *node, bool doNotProcess);
  virtual void                   ProcessAnim(ANIMQUEUENODE *node);

 public:
  virtual void                   PlayUnitSound(UNITSOUNDTYPE soundType, int alwaysPlay) const;
  virtual void                   PlayFoleySound() const;

 protected:
  virtual unsigned int           GetImpactType() const;

 public:
  virtual const VirtualItemInfo *GetDefendingItem() const;
  virtual void                   PlayDeathThudCameraShake() const;
  virtual void                   LootAnimEndHandler();
  virtual void                   RestoreUnit();
  virtual void                   UpdateBaseAnimation(unsigned int flags);
  virtual void                   StartSpellFizzleTimer(int spellID, unsigned int castingTime, int animSet);
  virtual void                   SetTorsoAnimState(unsigned int newState);
  virtual void                   SetBaseAnimState(unsigned int newState);

 protected:
  virtual unsigned int           DetermineWoundSequence() const;

 public:
  virtual void                   OnFlagChanged(unsigned int oldFlags);
  virtual const VirtualItemInfo *GetVirtualItem(unsigned int slot, unsigned char ignoreDisarmFlag) const;
  virtual int                    GetVirtualItemDisplayID(unsigned int slot) const;
  virtual int                    ShouldRenderUnitName(unsigned int mode) const;
  virtual void                   CommitTexture(int force);
  virtual unsigned int           UpdateUnitNameString(unsigned int localPlayerFlags, unsigned int otherUnitsFlags, char *buffer, unsigned int bufferSize) const;
  virtual void                   OnPickNextStandHandler();
  virtual float                  GetMountScale() const;
  virtual void                   OnMount();
  virtual void                   OnDismount();
  virtual bool                   CanBeMounted();

 protected:
  virtual void                   CleanupUnitArtwork(int playerModelChanged, int wasPlayerModel);
  virtual void                   ReinitializeUnitArtwork();
  virtual void                   PostReinitializeArtwork();

 public:
  virtual void                   OnStandStateChanged(unsigned int oldState, unsigned int newState);
  virtual void                   ChangeStandState(unsigned int standState);
  virtual void                   SetEmoteState(unsigned int emoteID);
  virtual int                    GetSpellRank(int spellID) const;
  virtual bool                   GetDefenseSkillRank(int &base, int &modifier) const;
  virtual bool                   GetAttackSkillRank(int hand, int &base, int &modifier) const;
  virtual void                   OnLevelChange();
  virtual float                  GetBlockChance() const;
  virtual float                  GetDodgeChance() const;
  virtual float                  GetParryChance() const;
  virtual int                    GetSpellCastingTime(int spellID) const;
  virtual void                   UpdateObjComponentVisuals(const CGItem_C *item, const ItemEnchantment *enchantments, int num);
  virtual void                   ClearItemVisuals(ACTIVEATTACHMENTINFO *info);
  virtual void                   SetItemVisuals(ACTIVEATTACHMENTINFO *info, const ItemVisualsRec *rec, bool force);
  virtual void                   SetLastWeaponModeSent(int mode);

  void SetStorage(unsigned long *storage);
  void PostInit(const CClientObjCreate &init);
  void PostMovementUpdate(const CClientMoveUpdate &update);
  void UpdateUnitCollisionBox(HMODEL model, const char *modelFileName);
  void SetClientInitData(unsigned long eventTime, const CClientObjCreate &init, bool partialUpdateOfActivePlayer);
  void UpdateMoveInfo(unsigned long eventTime, const CClientMoveUpdate &update);

 protected:
  static unsigned __int64        m_activeMover;

 public:
  enum {
    NUM_SAVED_FACING_DELTAS = 4
  };

  static void Initialize();
  static void PostShutdown();
  static void Shutdown();
  static unsigned int OffsetOf(OBJECT_TYPE_ID type);
  static void SetActiveMover(const unsigned __int64 &guid);
  static void StopMoveHeartbeatTimer();
  static void StartMoveHeartbeatTimer();
  static int GetAnimPriority(int state);
  static void NamePlateShow(int show);
  int                            GetCreatureType() const;
  int                            CanBeLooted(unsigned long currentTime) const;
  static void UpdateUnitNameplates(CGWorldFrame *worldFrame);
  static void RemoveAllNamePlates();

  virtual const char        *GetObjectName() const;
 protected:
  virtual int                ShouldFadeIn() const;

 public:
  virtual int                GetSelectionHighlightColor(NTempest::CImVector *outPtr) const;
  virtual void               RenderTargetSelection() const;
  void                       BuildSelectionRotMatrix(NTempest::C44Matrix &matrix) const;
  const char                *GetUnitName() const;
  const char                *GetUnitTitle() const;
  void                       UpdatePlayerNameWorldText();
 private:
  void                       AddUnitNamePlate(CGWorldFrame *worldFrame);
  void                       InsertSortedNamePlate(struct NAMEPLATEDESC *desc);
  void                       RemoveUnitNamePlate();

 public:
  void                       DestroyFadingMounts();
  void                       UpdateFadingMountModel(
      const NTempest::C3Vector &cameraPos,
      const NTempest::C3Vector &cameraTarg
  );
  void                       InitializeUnitName();
  void                       CreateFadeInMount();
  static void ResortAllUnitNameplates(CGWorldFrame *worldFrame);
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
  virtual void               ObjectPostAnimate(
      const NTempest::C34Matrix &matrix,
      const NTempest::C3Vector &cameraPos,
      const NTempest::C3Vector &cameraTarg
  );
  virtual float              GetRenderFacing() const;
  virtual void               UpdateRenderFacing();
  virtual void               UpdatePlayerName();
  virtual void               PostAnimate(CGWorldFrame *worldFrame);
  virtual void               OnSpecialMountAnim();
  virtual int                ShouldRender(unsigned long worldStatus);
  virtual HMODEL             GetCharacterModel(int *mountedPtr) const;
  virtual const char        *GetModelFileName() const;
  virtual int                UpdateModelLoadStatus();
  void                       RequestTalkEmote(TALKANIMATION talkAnim);
  virtual UNITAFFILIATION    GetGUIDAffiliation(unsigned __int64 unit) const;

  const CGUnitData *GetUnitData() const {
    return m_unit;
  }

  HMODEL DuplicateCharacterModel(unsigned int flags) const;

 protected:
  void InitializeSequenceFlags();
  void MarkSwimAnimations();
  void QueryModelStats();
  void QueryMountModelStats();
  void GetSwimMatrix(NTempest::C34Matrix *worldMatrix) const;
  void UpdateDisplayFacing();
  int  ShouldShuffle() const;

 private:
  void UpdateBaseRadius(HMODEL model);
  CGUnit_C &operator=(const CGUnit_C &);

 public:
  friend void SetPortraitTexture(CSimpleTexture *texture, const CGUnit_C *unit);
  friend void CreatureQueryCallback(int id, const unsigned __int64 &guid, void *arg, bool granted);
  friend int UnitModeUpdateHandler(unsigned __int64 guid, unsigned int offset, unsigned int bytes, const void *oldValue, void *param);
  friend int OnQuestUpdate(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);

  static void
  InitializeTextureVariations(const CreatureDisplayInfoRec *displayInfo, HMODEL theModel, const CreatureModelDataRec *modelData);

 public:
  void               PostSetClientInitData(const CClientMoveUpdate &update);
  int                OnMoveEvent(NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
  void               OnMonsterMove(unsigned long eventTime, CDataStore *msg);
  int                OnForceMoveChange(unsigned long eventTime, NETMESSAGE msgID, CDataStore *msg);
  void               OnMoveStopLocalNoUpdate(unsigned long eventTime);
  void               OnSetRunModeLocalNoUpdate(unsigned long eventTime, int run);
  void               OnSetFacingLocalNoUpdate(unsigned long eventTime, float facing);
  void               OnSetFacingGUIDLocalNoUpdate(unsigned long eventTime, const unsigned __int64 &guid);
  void               OnTeleportNoUpdate(unsigned long eventTime, const NTempest::C3Vector &position, float facing);
  void               OnPendingMoveStateChange(NETMESSAGE msgId);
  void               OnMoveStartLocal(unsigned long eventTime, int forward);
  void               OnCollideFalling(unsigned long eventTime);
  void               OnCollideFallLand(unsigned long eventTime);
  void               OnMoveStopLocal(unsigned long eventTime);
  void               OnJumpLocal(unsigned long eventTime);
  void               ToggleRunModeLocal(unsigned long eventTime);
  void               OnTurnStopLocal(unsigned long eventTime);
  void               OnStrafeStopLocal(unsigned long eventTime);
  void               OnStrafeStartLocal(unsigned long eventTime, int left);
  void               OnTurnStartLocal(unsigned long eventTime, int left);
  void               OnPitchStartLocal(unsigned long eventTime, int up);
  void               OnPitchStopLocal(unsigned long eventTime);
  void               OnSetRunModeLocal(unsigned long eventTime, int run);
  void               OnSetFacingLocal(unsigned long eventTime, float facing);
  void               OnSetRawFacingLocal(unsigned long eventTime, float facing);
  void               OnSetPitchLocal(unsigned long eventTime, float pitch);
  void               OnRunSpeedChangeLocal(unsigned long eventTime, NETMESSAGE msgID, float speed);
  void               OnWalkSpeedChangeLocal(unsigned long eventTime, float speed);
  void               OnSwimSpeedChangeLocal(unsigned long eventTime, NETMESSAGE msgID, float speed);
  void               OnAllSpeedChangeLocal(unsigned long eventTime, float speed);
  void               OnTurnRateChangeLocal(unsigned long eventTime, float rate);
  void               OnMovementInitiated(bool facingOnly);
  void               OnTeleportLocalNoUpdate(unsigned long eventTime, const NTempest::C3Vector &position, float facing);
  void               UpdateSwimmingStatus(unsigned long eventTime, int inWater, float depth);
  void               SendRedirectionMessage();
  void               PlaySplashSound(const NTempest::C3Vector &position);
  void               ProcessLocalMoveEvent(NETMESSAGE msgId);
  void               BuildMovementUpdate(NETMESSAGE messageId, CDataStore *msg) const;
  void               SendMovementUpdate(NETMESSAGE messageId);
  void               StopSpellFizzleTimer(int spellID, unsigned char status);
  void               SpellDelayed(int delay);
  void               EndSpellEffects(unsigned char status);
  int                SetCastingSpell(int spellID, bool force, bool precastAnimSuccessful);
  bool               SetSpellCastingAnimation(ANIMENUMERATION anim, unsigned int castKit, unsigned int soundID, int shakeID, ANIMENUMERATION &result);
  void               ClearSpellCastAnimInfo();
  void               AddHitAnimHolds(int spellID, const TSStackArray<unsigned __int64> &targets);
  void               MaybeSaveChannelSpellTargets(int spellID, const TSStackArray<unsigned __int64> &targets);
  void               StoreSpellMissileEffect(
      const unsigned __int64 &target, const NTempest::C3Vector &destination,
      float speed, unsigned int ammoDisplayID, int inventoryType,
      const SpellVisualRec *rec, bool hits, MISS_REASON reason,
      unsigned int spellID, bool wasProc);
  int                GetCastingSpell() {
    return m_castingSpell;
  }
  void               HandlePrecastStart(bool precast);
  void               HandlePrecastStop(int spellID, bool force);
  void               CheckDeferredSheathing();
  void               SetSheatheReason(SHEATHEREASONS reason, bool on, bool suppressSound);
  const SpellVisualRec *GetAppropriateSpellVisual(const SpellRec *spellRec, SpellVisualRec &filled) const;
  unsigned int       GetCurrentTorsoAnim() const;

 protected:
  unsigned int       GetAnimationState();

 public:
  int                IsWalking() const;
  int                SetTorsoAnimation(unsigned int state, unsigned long duration, unsigned int flags);
  void               CheckLevelUpAnimFlag(int oldState, int newState);
  void               HandleCastAnimEvent();
  void               HandleCombatAnimEvent(const char *eventName, unsigned long value, const NTempest::C3Vector &position);
  void               HandleAnimEvent(const char *eventName, const NTempest::C3Vector &pos);
  void               HandleMountedAnimEvent(const char *eventName, const NTempest::C3Vector &pos);
  UNITEFFECTSPECIALS DetermineBreathEffect(unsigned int *duration);
  void               BreathHandler(int forceOnMount);
  void               ProcessBreathParticles(int currentTime);

 protected:
  void               HandlePlayStandSound(unsigned long code, const char *eventName);
  void               HandleFootfallAnimEvent(const NTempest::C3Vector &position);
  void               PlayFidgetSound(unsigned int fidgetNumber);

 public:
  void               PlayStandSound() const;
  void               PlayDeathThud() const;
  int                GetStandStateAnim(HMODEL model) const;
  void               SetTorsoAnim(unsigned int newAnim);

 protected:
  int                IsSplashing(const NTempest::C3Vector &position);

 public:
  bool               IsShapeShifted() const;
  int                IsUnderWater() const;
  unsigned int       GetRunSequence() const;
  unsigned int       GetStopSequence() const;
  int                GetWalkStateAnim() const;
  unsigned int       GetEmoteAnimation(unsigned int emoteID) const;
  void               SetRangedWeaponPullAnim(int duration);

 protected:
  float              GetAnimTimeScale(unsigned int sequence, unsigned int duration, unsigned int flags);
  int                SetTorsoSequence(float timeScale, int flags);

 public:
  bool               TorsoAnimOverridesBase() const;
  int                IsPreemptableWoundAnimState(unsigned int state);
  int                IsAttackAnimState(unsigned int state);
  bool               QueueVictimAnim(VICTIMSTATES newState, int unitDead, int criticalHit, unsigned int victimRoundDuration);

 protected:
  void               CheckPendingVictimFeedback();

 public:
  void               SetVictimAnimation(VICTIMSTATES newState, int unitDead, int criticalHit, unsigned int victimRoundDuration, int processNow);
  void               DoVictimFeedback(const ATTACKROUNDINFO *roundInfo, int showAnimation);
  void               AdjustVictimState(ATTACKROUNDINFO *roundInfo);
  MISS_REASON        AdjustVictimState(MISS_REASON reason);
  void               ShowWorldText(const ATTACKROUNDINFO *roundInfo);
  void               PerformSpellProcImpact(int spell);

 protected:
  void               ShowBloodSpurt(CGUnit_C *attacker, int crushingBlow);
  BLOODSPURTLOCATION DetermineBloodLinkPoint(CGUnit_C *attacker);

 public:
  void               SetMeleeDeathHold(const CGUnit_C *victimPtr);
  void               AddVictimDeathHold(CGUnit_C *victimPtr);
  void               ClearMeleeDeathHold();
  void               PlayParrySound(bool ignoreMainHand, const ATTACKROUNDINFO *roundInfo, const NTempest::C3Vector &position) const;
  void               PlayImpactSound(unsigned __int64 attacker, int criticalHit, COMBATHAND hand) const;
  void               PlayCustomAttackSound(int sound, const NTempest::C3Vector &position);
  void               SetCustomAttackSound(int sound, const NTempest::C3Vector &position);
  void               ShowHandArrow(int show);
  const VirtualItemInfo *GetParryingItem(bool ignoreMainHand) const;
  const VirtualItemInfo *GetAttackingWeapon(COMBATHAND hand) const;
  bool               GetWeaponSwingType(bool mainHand, WEAPONSWING_SOUNDTYPES &type);
  int                GetUnitSize() const;

 protected:
  void               CheckPendingMissileRelease(const NTempest::C3Vector *position);

 public:
  void               CheckPendingImpactKit();
  const SpellVisualKitRec *GetRangedSpellAnim(int id, bool castKit);
  int                ClearTorsoAnimation(unsigned int flags);

 protected:
  bool               IsSpellAuraAnimActive(int &anim) const;
  bool               IsSpellChannelAnimActive(int &anim) const;

 public:
  int                PlayEmoteAnimation(unsigned int emoteID, int flags);
  void               RangedWeaponAnimEndHandler();
  void               ThrowAnimEndHandler();
  void               GenericAnimEndHandler(ANIMENUMERATION animID, void *param);

 protected:
  void               StoreSequenceEndCallbacks(int anim);
  void               ProcessAnimEndCallbacks();

 public:
  void               DeathAnimEndHandler();
  void               ForceUpdateBaseAnimation();
  void               PickNextRunHandler();
  void               WoundAnimEndHandler();
  void               SpellAnimEndHandler();
  void               NPCAnimEndHandler();
  int                JumpTakeOffFinishedHandler();
  int                JumpLandFinishedHandler();
  void               SitSleepAnimEndHandler();
  void               RangedPrecastEndHandler();
  bool               SetSpellPreCastingAnimation(ANIMENUMERATION anim);
  void               AttackAnimEndHandler();
  void               DodgeAnimEndHandler();
  void               SetRangedWeaponReleaseAnim();
  void               DrawBowString(const NTempest::C3Vector &cameraPos);
  void               ThrownMissileReleased();
  void               CheckPendingThrownWeaponReattach(bool force);

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
  bool         IsSlotComponented(unsigned int offset, int ignoreUsingRangedWeapon);

 protected:
  bool         UpdateVisibilitySlots(HMODEL characterModel, int attachmentSlot, ACTIVEATTACHMENTINFO **&found, int displayID, bool deferApply);
  void         ClearWeaponTrailHandles();

 public:
  void         ReinitializeWeaponTrails();
  void         OnMountCancelled();
  void         OnEncounter(AI_REACTION reaction);
  HMODEL       GetMountModel();

 protected:
  const CreatureSoundDataRec *GetMountSoundDataRec() const;
  const CreatureSoundDataRec *GetSoundData() const;

 public:
  void         UnitInitializeMountModel(HMODEL model);
  void         CreateUnitMount();
  void         DestroyUnitMount(int doNotUpdateAnim);
  void         UpdateUnitMountInfo(int immediate, unsigned int changedFlags);
  void         CreateFadeOutMount();
  void         DisableWeaponTrails();

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
  unsigned int  GetDisplayRace() const;
  unsigned int  GetDisplaySex() const;
  HTEXCOMPONENT GetTexComponent() const;
  virtual int   UpdateAttachmentLoadStatus();
  virtual int   UpdateTexComponentLoadStatus();
  int           IsModelComponentable() const;
  const char   *GetDisplayTextureName() const;
  unsigned int  SkinVariationID() const;
  unsigned int  FaceID() const;
  unsigned int  HairStyleID() const;
  unsigned int  HairColorID() const;
  unsigned int  FacialHairID() const;
  void          InitPreferredGeosets();

 protected:
  void          SetAttachmentHidden(int attachmentSlot, bool hide);

 public:
  void          PlaySpellLoopedSound(int soundID);
  void          KillSpellLoopedSound();
  void          StopRangedAttackPrecast();
  HMODEL        GetRangedWeaponModel();
  HMODEL        GetMountedModel() const;
  void             PendingPrecastInterrupt(int spellID);
  void             SaveTrackingTarget(unsigned __int64 target, TRACKTYPE type, bool snapToTargetOnClear);
  void             ClearTrackingTarget(bool snapToTargetOnClear);
  unsigned __int64 GetTrackingTarget() const;
  bool             TrackingTargetMoving() const;
  void             HandleFollowTarget();
  void             SetWeaponMode(WEAPONMODE mode);
  void             ClearRangedStandTimer();
  void             SetAmmoDisplay(unsigned int displayID, unsigned int inventoryType) {
    m_ammoDisplayID = displayID;
    m_ammoInvType = inventoryType;
  }
  void             SetRangedStandTimer();
  void             DetermineReadySequence(bool forceNormal);
  void             OnCombatModeTimer();
  void             AttackUnit(CGUnit_C *newVictim);
  void             OnAttackSwing(unsigned __int64 victimGUID, unsigned int clientTimeStamp);
  void         SetDebugHitRolls(const ATTACKROUNDINFO &info);

 private:
  int          SetAttackerAnimation(const ATTACKROUNDINFO *roundInfo, int processNow);

 public:
  void         InitializeResEffectModel();
  void         ClearResEffectModel();
  void         AttachResEffectModel();
  void         DetatchResEffectModel();
  void         OnRangedStandTimer();

 protected:
  bool         SheatheAnimPlaying() const;

 public:
  void         MaybeStartSheatheAnim();
  void         UpdateSheatheRangedReasons(bool suppressSound);
  void         SheatheOrUnsheatheItems(SHEATHEREASONS reason, bool sheathe, bool playSound);

 protected:
  bool         SheatheObjComponent(int slot, bool sheathe);
  void         ClearDeferredAttachment(HMODEL charModel, int slot);
  bool         ApplyAttachmentInfo(HMODEL characterModel, bool sheathe, int attachmentSlot, bool force);
  void         SetHandsState(HMODEL model);
  void         SetFingersSeq(HMODEL charModel, unsigned int sequence, unsigned int startFinger, unsigned int lastFinger);
  void         ResetFingersSeq(HMODEL charModel, unsigned int startFinger, unsigned int lastFinger);
  void         SetHandState(HMODEL model, const VirtualItemInfo *item, unsigned int startFinger, unsigned int lastFinger);

 public:
  void         HandleSheatheAnimEvent(bool clearSheatheAnim, bool suppressSound);
  void         SheatheAnimEndHandler();

 protected:
  bool         SetSheathingSequence();

 public:
  void         UnitInitializeModel(HMODEL model);
  void         UnitUninitializeModel(HMODEL model);
  void         ShutdownWorldName();

 protected:
  void         MarkFootstepAnimations(HMODEL model);
  void         UpdateUnitAlpha();
  void         RefreshAttachmentInfo(HMODEL model);

 public:
  void         KillCreatureLoopSound();
  void         InitializeLoopSound();
  void         InstallSeqEndHandler(HMODEL model, unsigned int animID);
  void         ClearMountAnimState();
  void         ClearTempCharModel();
  void         SetTempCharModel(HMODEL model);

 protected:
  void         ClearAnimCallbackData();

 public:
  void         CheckPendingSpellAnimHits();
  void         SpellAnimHit(int spellID);
  void         UpdateMountAnimation(unsigned int newState, unsigned int flags);
  void         UpdateMovementAnimSpeed(int forMount, int currentState);
  void         UpdateBaseAnimation(unsigned int newState, unsigned int flags);
  void         SetBaseAnim(unsigned int newAnim);
  void         LookAtTarget();
  void         UpdateLookAtTarget();
  void         SetLocalTarget(unsigned __int64 target);
  bool         BaseAnimLocksHead() const;
  bool         TorsoAnimLocksHead() const;
  unsigned int GetCurrentBaseAnimState() {
    return m_currentBaseAnimState;
  }
  unsigned int GetCurrentTorsoAnimState() {
    return m_currentTorsoAnimState;
  }
  unsigned int GetCurrentBaseAnim() {
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
  int          GetSpellLevel(int spellID) {
    return GetSpellRank(spellID) / 5;
  }
  bool                            IsSpellKnown(int spellID) const;
  bool                            CheckAndReportSpellInhibitFlags(const SpellRec *spell, const CGItem_C *item);
  const SkillLineAbilityRec      *LookupAbility(int spellID) const;
  bool                            IsSpellSuperceded(int spellID) const;
  int                             GetSpellSkillLine(int spellID) const;
  static bool FactionHasReputation(int faction);
  UNIT_REACTION                   UnitReaction(const CGUnit_C *unit) const;
  static UNIT_REACTION UnitReaction(int factionID, const CGUnit_C *unit, int trueSight);
  bool                            CanAttack(const CGUnit_C *unit) const;
  bool                            CanAssist(const CGUnit_C *unit) const;
  bool                            CanCooperate(const CGUnit_C *unit) const;
  bool                            CanInteract(const CGUnit_C *unit) const;
  bool                            CanInteract(const CGGameObject_C *object) const;
  bool                            IsUnitInGroup(const CGUnit_C *unit) const;
  void                            SetMirrorHandlers();
  void                            UnsetMirrorHandlers();
  void                            ClearFishingObject();
  void                            ProcessChannelObject();

 protected:
  void                            SetAuraMirrorHandlers();
  void                            UnsetAuraMirrorHandlers();
  void SetAuraMirrorHandler(unsigned int slot, int(*handler)(unsigned __int64, unsigned int, unsigned int, const void *, void *));
  void UnsetAuraMirrorHandler(unsigned int slot, int(*handler)(unsigned __int64, unsigned int, unsigned int, const void *, void *));

 public:
  void OnAuraChanged(unsigned int slot, int previousValue);
  void SignalDisplayHealthUpdate() const;
  void UpdateDisplayHealth();
  void HandleBloodPool(unsigned int currentTime);
  void OnStopRender();
  void CheckRendering();
  void UpdateDisplay(unsigned long now);
  void RemoveBloodPool();
  void AddBloodPool();
  void QueueBloodSplat(BLOODSPURTLOCATION linkPoint);
  const UnitBloodRec *GetBloodRecord();

 protected:
  void             RemoveAuraEffect(unsigned int slot, int previousSpell);
  void             RefreshAuraVisuals();
  void             AddPendingShapeshiftEffect(int oldSpell);
  void             AddAuraEffect(unsigned int slot, bool startNow);

 public:
  int              ShouldDelayLevelupAnim();
  int              ShouldDelayLevelupAnim(unsigned int state);
  void             PerformLevelUpAnim(int force);
  void             OnCharmedChanged();
  void             UpdateDisplayInfo();
  int              DisplayInfoNeedsUpdate(int &playerModelChanged, int &wasPlayerModel) const;

 protected:
  void             RefreshDataPointers();

 public:
  void             InitializeExtendedDisplay();

 protected:
  void             SetupFootprints();

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
  void             EnableWeaponTrail(const NTempest::CImVector &color, int fadeOutRate, unsigned int duration);

 protected:
  void OnTeleport(unsigned long eventTime, const CMovementStatus &update);
  void OnMoveStart(unsigned long eventTime, const CMovementStatus &update, int forward);
  void OnMoveStop(unsigned long eventTime, const CMovementStatus &update);
  void OnStrafeStart(unsigned long eventTime, const CMovementStatus &update, int left);
  void OnStrafeStop(unsigned long eventTime, const CMovementStatus &update);
  void OnJump(unsigned long eventTime, const CMovementStatus &update);
  void OnTurnStart(unsigned long eventTime, const CMovementStatus &update, int left);
  void OnTurnStop(unsigned long eventTime, const CMovementStatus &update);
  void OnPitchStart(unsigned long eventTime, const CMovementStatus &update, int up);
  void OnPitchStop(unsigned long eventTime, const CMovementStatus &update);
  void OnSetRunMode(unsigned long eventTime, const CMovementStatus &update, int run);
  void OnSetFacing(unsigned long eventTime, const CMovementStatus &update);
  void OnSetPitch(unsigned long eventTime, const CMovementStatus &update);
  void OnToggleCollision(unsigned long eventTime, const CMovementStatus &update);
  void OnRunSpeedChange(unsigned long eventTime, const CMovementStatus &update, CDataStore *msg);
  void OnWalkSpeedChange(unsigned long eventTime, const CMovementStatus &update, CDataStore *msg);
  void OnSwimSpeedChange(unsigned long eventTime, const CMovementStatus &update, CDataStore *msg);
  void OnTurnRateChange(unsigned long eventTime, const CMovementStatus &update, CDataStore *msg);
  void OnTeleportAck(unsigned long eventTime, const CMovementStatus &update);
  void OnSwimStart(unsigned long eventTime, const CMovementStatus &update);
  void OnSwimStop(unsigned long eventTime, const CMovementStatus &update);
  void OnMoveHeartBeat(unsigned long eventTime, const CMovementStatus &update);

 private:
  void InternalProcessSpellProcEffects(SPELLPROC_ACTION action, float elapsed);

 public:
  SPELLEFFECTDESC *GetActiveEffect(LIST(SPELLEFFECTDESC) &list);
  void             ReinitializePaperdollModel();
  void             CreatePaperdollModel();
  HMODEL           GetPaperDollModel(bool duplicateModel);
  void             DestroyPaperdollModel();
  void             StandStateChanged(unsigned int oldState);
  void             NPCFlagChanged(unsigned int oldNPCFlags);
  void             RemoveInteractIcon();
  void             RefreshInteractIcon();
  void             UpdateInteractIcon(QUEST_GIVER_STATUS status);
  void             UpdateInteractIcon(INTERACTICONTYPE which);
  unsigned int     GetPlayerNameAttachmentPoint();
  void             SetEmoteQueue(const QUESTGIVEREMOTENODE *list, unsigned int num);
  void             SetEmoteQueue(TSStackArray<QUESTGIVEREMOTENODE> &list);

 protected:
  int              EmoteProcType(unsigned int emoteID, EMOTESPECPROCS &proc) const;

 public:
  void             AddWorldXPGainText(int xpGain);
  void             AddWorldDamageText(unsigned int damage, int normalCombatDamage);
  void             AddWorldCritText(unsigned int damage, int normalCombatDamage);
  void             AddWorldText(MISS_REASON reason);
  void             AddWorldText(WORLDTEXTMISSTYPE type);
  void             StoreXPGain(int XP);
  void             SaveQuestAddItemMessage(int killed, int needed);
  void             ProcessQuestItemMessages();
  void             AddDamageDone(unsigned int damage, int normalCombatDamage, unsigned int flags, unsigned __int64 attacker, int spellID);
  void             SpellEventHit();
  void             ShowPlayerXPGained();
  void             WeaponModeChanged();
  void             VirtualComponentChanged(int slot, int oldValue);
  void             AttachVirtualComponent(unsigned int slot, bool deferApply);
  void             DetachVirtualComponent(int slot, bool defer, bool removeRecord);

 protected:
  void             RemoveObjectComponentByInvSlot(int invSlot, bool deferDeleteFromModel, bool removeRecord);
  void             ClearActiveAttachmentInfo();

 public:
  void             OnDynamicFlagsChanged(unsigned int oldValue);
  void             OnChannelSpellChanged(unsigned int oldSpell);
  void             ClearSavedChannelSpellTargets();
  int              GetSavedChannelSpellID() const {
    return m_savedChannelSpellID;
  }
  const TSGrowableArray<unsigned __int64> &GetSavedChannelSpellTargets() const {
    return m_savedChannelSpellTargets;
  }

 protected:
  int              SetEmoteAnimation(unsigned int emoteID, int flags);

 public:
  void             DDDELLOG(unsigned __int64 guid, const char *string, const char *file, unsigned int line);
  void             DDADDLOG(unsigned __int64 guid, const char *string, const char *file, unsigned int line);
  void             AddDeathHold();
  void             DDGENLOG(unsigned __int64 guid, const char *string, const char *file, unsigned int line);
  void             DumpGeneralDeathHoldLog(HSLOG handle, TSGrowableArray<char> *stringBuffer) const;
  void             DelDeathHold();

 protected:
  void             MaybeAttachAura(UNITEFFECTATTACHPPOINT attach, unsigned int effect, unsigned int spellID, int priority, bool permanent);
  ACTIVEAURAINFO  *FindActiveAuraInfo(int slot);
  void             AddKitAuras(const SpellVisualKitRec *kitRec, const SpellRec *spellRec);
  void             RemoveAuraVisual(UNITEFFECTATTACHPPOINT attach);
  void             FinishAuraDecays();

 public:
  void             RegisterScript();
  void             UnregisterScript();
  void             TriggerPlayerNameUpdate();
  void             PlayerNameVisibilityChanged(int nameVisible);
  void             UpdatePlayerNameColor();
  void             SetForcedAnimation(const char *string);
  void             ResetForcedAnimation();
  void             OnNPCHello();
  void             OnNPCGoodbye();
  int              PlayNPCSound(NPCSOUNDS sound, unsigned int index);

  static unsigned __int64 GetActiveMover() {
    return m_activeMover;
  }
  bool                     IsClientControlled() const;
  float                    GetBoundingRadius() const;
  void                     OnRestoreHealth();
  bool                     DoNotLogDeath() const;
  unsigned __int64         IsAttacking() const;
  unsigned __int64         IsAttackingNow() const;
  void                     ClearAttackSent();
  bool                     CanAttackNow(const CGUnit_C *unit) const;
  int                      IsStunned() const;
  int                      IsPacified() const;
  int                      IsDisarmed() const;
  int                      IsPlayingDeathAnim() const;
  int                      IsPlayingLayDownAnim() const;
  int                      IsPlayingSleepAnim() const;
  int                      IsPlayingGetUpAnim() const;
  int                      HasBloodRec() const;
  void                     GetResistanceAndBuffs(int r, int &realResistance, int &effectiveResistance, int &buffPositive, int &buffNegative) const;
  bool                     IsFriend(const CGUnit_C *unit) const;
  bool                     IsPeaceful(const CGUnit_C *unit) const;
  bool                     IsEnemy(const CGUnit_C *unit) const;
  void                     SetDead();
  int                      SetBlock(unsigned int i, unsigned long data);
  void                     SetData(const void *data, unsigned int bytes);
  int                      HasInteractIcon();
  QUEST_GIVER_STATUS       GetQuestGiverStatus();
  void                     SetQuestGiverStatus(QUEST_GIVER_STATUS status);
  int                      GetDebugStateInfo(ATTACKROUNDINFO *attackInfo);
  void                     ClearDebugFlags();
  void                     SetUnitBadFacing();
  int                      IsBadFacing();
  int                      IsDeathFlagSet() const;
  void                     RemoveForceDisplayFacingFlag();
  unsigned int             GetReadySequence() const;
  unsigned int             GetRangedReadySequence() const;
  void                     UpdateReadyAnim(const ItemStats *stats);
  unsigned int             GetDeathHolds() const;
  void                     SetReattachThrownWeapon(int reattach);
  int                      ShouldReattachThrownWeapon() const;
  void                     SetDebugPathPosition(const NTempest::C3Vector &position);
  HCHARGEOSET              GetGeosetHandle() const;
  const unsigned int      *GetPreferredGeosets() const;
  unsigned int             GetNumPreferredGeosets() const;
  int                      GetDisplayHealth() const;
  bool                     IsTexComponentLoaded() const;
  void                     SetTexComponentLoaded(int loaded);
  bool                     IsBeingStalked() const;
  bool                     GetLootPermission() const;
  bool                     GetShowingHandArrow() const;
  void                     SetShowHandArrowFlag(int show);
  void                     SetShowingHandArrowFlag(int showing);
  bool                     IsItemSwapFlagSet() const;
  void                     SetItemSwapFlag(bool set);
  int                      UnitMountShowing() const;
  bool                     UnitHeadLocked() const;
  void                     SetCreatureStats(const CreatureStats_C *stats);
  void                     OnEnableCollisionLocalNoUpdate(unsigned long eventTime);
  void                     OnDisableCollisionLocalNoUpdate(unsigned long eventTime);
  void                     OnToggleCollisionLocal(unsigned long eventTime);
  void                     OnStrafeStartLocalNoUpdate(unsigned long eventTime, int left);
  void                     OnStrafeStopLocalNoUpdate(unsigned long eventTime);
  void                     OnSwimStartLocal(unsigned long eventTime);
  void                     OnSwimStopLocal(unsigned long eventTime);

 protected:
  unsigned int GetFlags() const;
  int          CurrentAnimIncludesHit() const;
  int          GotRangedWeaponRelease() const;
  void         FootstepAnimEventHit(const NTempest::C3Vector &position, int isLeftFoot);
  void         HandleFootstepAnimEvent(const NTempest::C3Vector &position);
  void         GetFootprintInfo(unsigned int *id, NTempest::C2Vector *size);
  bool         WeaponAttached(COMBATHAND hand) const;
  void         ClearChannelAuraInfo();
  void         HandleRemotePlayerSheathing();
  void         HandleLocalPlayerSheathing();
  bool         SheatheAnimEventEncountered() const;
  void         SetSheatheEventEncountered(bool encountered);

 private:
  void  AddDamageTimer(
      unsigned __int64 attacker,
      VICTIMSTATES state,
      int unitDead,
      unsigned int duration,
      float delay,
      int criticalHit,
      int crushingBlow
  );
  float GetBaseRadius() const;

  friend void MovementFixOutOfBoundsUnit(unsigned __int64 guid);

 private:
  int                                                    m_questCountKilled;
  int                                                    m_questCountNeeded;
  HMODEL                                                 m_resEffectModel;
  unsigned __int64                                       m_meleeTargetDeathHold;
  int                                                    m_precastSheatheHoldTimer;

 protected:
  int                                                    m_customAttackSound;
  NTempest::C3Vector                                     m_customAttackPosition;
  unsigned int                                           m_splashSoundID;

 private:
  unsigned int                                           m_disengageLookAtTimer;

 protected:
  TSGrowableArray<ANIMENDDATA>                           m_animEndCallbackList;
  ANIMENDDATA                                           *m_callbackList[135];
  const CreatureStats_C                                 *m_stats;
  const CreatureDisplayInfoRec                          *m_displayInfo;
  const CreatureDisplayInfoExtraRec                     *m_displayInfoExtra;
  const CreatureModelDataRec                            *m_modelData;
  const CreatureSoundDataRec                            *m_soundData;
  const CreatureSoundDataRec                            *m_mountedSoundData;
  const UnitBloodLevelsRec                              *m_bloodRec;
  AuraVisual                                             m_auraVisual[12];
  LISTDECL(ACTIVEAURAINFO, m_activeAuraInfo);
  ANIMENUMERATION                                        m_pendingImpactAnim;
  HMODEL                                                 m_tempCharModel;
  TSGrowableArray<char>                                  m_deathHoldBuffer;
  TSGrowableArray<unsigned int>                          m_deathHoldBufferIndices;
  int                                                    m_lastDeathTime;
  int                                                    m_nextDeathHoldCheckTime;
  TSGrowableArray<QUESTGIVEREMOTENODE>                   m_emoteQueue;
  HMODEL                                                 m_interactIconModel;
  LISTDECL(BLOODSPLATNODE, m_bloodSplatNodes);
  unsigned int                                           m_nextAllowableBloodPool;
  LISTDECL(ANIMQUEUENODE, m_animQueue);
  CCombatClient                                          m_combat;
  ANIMQUEUENODE                                         *m_currentDamageInfo;
  unsigned int                                           m_readySequence;
  unsigned int                                           m_animEndTime;
  unsigned int                                           m_animBaseDuration;
  unsigned int                                           m_animStartTime;
  unsigned int                                           m_flags;
  unsigned int                                           m_animFlags;
  unsigned int                                           m_footprintTextureID;
  unsigned int                                           m_terrain;
  NTempest::C2Vector                                     m_footprintSize;
  float                                                  m_footprintParticleScale;
  DEBUGHITROLLINFO                                       m_hitInformation;
  ANIMENUMERATION                                        m_spellPrecastingAnim;
  ANIMENUMERATION                                        m_spellCastingAnim;
  ANIMENUMERATION                                        m_deferredPrecastAnim;
  int                                                    m_animatingAura;
  unsigned int                                           m_emoteID;
  unsigned int                                           m_spellCastingEffectKit;
  unsigned int                                           m_spellCastingSoundID;
  int                                                    m_spellCastingCameraShakeID;
  MISSILESTRUCT                                          m_spellMissileStruct;
  float                                                  m_lastSentFacing;
  float                                                  m_lastSentPitch;
  HPLAYERNAME                                            m_unitNameHandle;
  int                                                    m_accumulatedXPDrop;
  int                                                    m_castingSpell;
  int                                                    m_interruptedSpell;
  int                                                    m_lastSpellCastAnimTime;
  int                                                    m_nextBreath;
  int                                                    m_nextMountBreath;
  int                                                    m_scriptRegistered;
  float                                                  m_displayFacing;
  float                                                  m_smoothFacing;
  float                                                  m_savedFacingDeltas[NUM_SAVED_FACING_DELTAS];
  float                                                  m_forcedDisplayFacing;
  unsigned int                                           m_deathTime;
  unsigned __int64                                       m_lastCombatTarget;
  unsigned __int64                                       m_targetUnit;
  unsigned int                                           m_currentBaseAnimState;
  unsigned int                                           m_currentBaseAnim;
  unsigned int                                           m_currentTorsoAnimState;
  unsigned int                                           m_currentTorsoAnim;
  unsigned int                                           m_currentMountAnimState;
  unsigned int                                           m_currentWoundStartTime;
  unsigned int                                           m_currentWoundAnimDuration;
  unsigned int                                           m_spellFizzleTimer;
  unsigned int                                           m_deathHolds;
  QUEST_GIVER_STATUS                                     m_questGiverStatus;
  NTempest::C3Vector                                     m_serverLoc;
  TSGrowableArray<NTempest::C3Vector>                    m_debugPathPoints;
  unsigned int                                           m_numDebugPathNodes;
  Sound                                                 *m_spellLoopedSound;
  Sound                                                 *m_creatureLoopSound;
  unsigned int                                           m_mountedFootprintID;
  NTempest::C2Vector                                     m_mountedFootprintSize;
  HMODEL                                                 m_fadingPureMountModel;
  PUREMOUNTFADEMODE                                      m_pureMountFadeMode;
  unsigned int                                           m_pureMountFadeStartTime;
  float                                                  m_fadingMountFacing;
  NTempest::C3Vector                                     m_fadingMountPos;
  float                                                  m_fadingMountScale;
  const NPCSoundsRec                                    *m_NPCSoundsRec;
  unsigned int                                           m_lastGlobalClickCount;
  unsigned int                                           m_pissedCount;
  unsigned int                                           m_numNPCPissedSounds;
  HCHARGEOSET                                            m_geosetHandle;
  HTEXCOMPONENT                                          m_texComponent;
  unsigned int                                           m_preferredGeosets[15];
  int                                                    m_displayHealth;

 private:
  LISTDECL(IMPACTEFFECTDESC, m_impactEffectsDesc);
  LISTDECL(SPELLEFFECTDESC, m_spellEffectLists[11]);
  NTempest::C3iVector                                    m_currentEmissive;
  int                                                    m_pendingHitSpellID;
  TSGrowableArray<unsigned __int64>                      m_pendingHitAnimVictims;
  int                                                    m_auraFlags[56];
  int                                                    m_walkStateAnim;
  int                                                    m_standStateAnim;
  float                                                  m_baseRadius;

 protected:
  unsigned int                                           m_ammoDisplayID;
  unsigned int                                           m_ammoInvType;
  unsigned int                                           m_rangedStandTimer;
  ACTIVEATTACHMENTINFO                                  *m_attachments[5];
  ACTIVEATTACHMENTINFO                                  *m_deferredAttachments[5];
  int                                                    m_weaponTrails[5];
  HMODEL                                                 m_paperDollModel;
  int                                                    m_sheatheReasons;
  ANIMENUMERATION                                        m_handAnim[2];
  unsigned int                                           m_deferredSheatheFlags;
  SHEATHEREASONS                                         m_deferredSheatheReason;
  int                                                    m_savedChannelSpellID;
  TSGrowableArray<unsigned __int64>                      m_savedChannelSpellTargets;
  SPELLEFFECTDESC                                       *m_channelSpellEffect;
  const SpellRec                                        *m_shapeShiftPoof;
  FishingLineObject                                     *m_fishingLineObject;

 protected:
  void           ProcessEmoteQueue();
  void           ApplyStrafeRotation(unsigned int newState);
  void           SetStrafeRotation();
  int            PlayBaseAnimation(int newAnimState, int newAnim, int forceNoFidget, bool &checkImpacts);
  float          DetermineWalkRunTimeScale(int currentState);
  ANIMQUEUENODE *ProcessAnimQueue();
  ANIMQUEUENODE *GetNewAnimNode(int leaveUnlinked);
  void           RecycleAnimNode(ANIMQUEUENODE *node);
  void           PurgeAnimNodes(bool doNotProcess);
  unsigned int   ChooseAnimation(unsigned int state) const;
  unsigned int   DetermineAttackerSequence(COMBATHAND hand) const;
  unsigned int   DetermineParrySequence() const;
  unsigned int   GetAttackerAnimEx(COMBATHAND hand, const VirtualItemInfo *itemInfo) const;
  void           DDWRITELOG(const char *buffer);

 private:
  void LookAtTarget(CGUnit_C *target);
  void ApplyObjectCameraSpaceLookAt(const NTempest::C3Vector &target);
  void RemoveObjectLookAt();
  void PrintAttackSeqErrorMsg(unsigned int sequence, unsigned int fallBack) const;
};

void CGUnit_C_RenderBowStrings(const NTempest::C3Vector &c);

void ClearSpecialEffects(HMODEL model);
