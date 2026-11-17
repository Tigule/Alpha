#pragma once

#include "Object/MovementData.h"
#include "Object/UnitConst.h"

enum {
  BONE_ARM_LEFT = 0,
  BONE_ARM_RIGHT = 1,
  BONE_SHOULDER_LEFT = 2,
  BONE_SHOULDER_RIGHT = 3,
  BONE_LOWER_SPINE = 4,
  BONE_WAIST = 5,
  BONE_HEAD = 6,
  BONE_JAW = 7,
  BONE_FINGER_FIRST = 8,
  BONE_FINGER_FIRSTR = 8,
  BONE_FINGER_INDEXR = 8,
  BONE_FINGER_MIDDLER = 9,
  BONE_FINGER_PINKYR = 10,
  BONE_FINGER_RINGR = 11,
  BONE_THUMBR = 12,
  BONE_FINGER_LASTR = 12,
  BONE_FINGER_FIRSTL = 13,
  BONE_FINGER_INDEXL = 13,
  BONE_FINGER_MIDDLEL = 14,
  BONE_FINGER_PINKYL = 15,
  BONE_FINGER_RINGL = 16,
  BONE_THUMBL = 17,
  BONE_FINGER_LASTL = 17,
  BONE_FINGER_LAST = 17,
  EMITTER_BREATH = 18,
  RIGHTHANDCAST = 19,
  LEFTHANDCAST = 20,
  OBJ_ATTACH_BREATH = 21,
  OBJ_ATTACH_PLAYERNAME = 22,
  OBJ_ATTACH_PLAYERNAMEMOUNTED = 23,
  EMITTER_HEAD_TARGET = 24,
  EMITTER_CHEST_TARGET = 25
};

enum {
  CAMERA_PORTRAIT = 0,
  CAMERA_PAPERDOLL = 1,
  NUM_OBJECT_CAMERAS = 2
};

enum POWER_TYPE {
  POWER_TYPE_MANA = 0,
  POWER_TYPE_RAGE = 1,
  POWER_TYPE_FOCUS = 2,
  POWER_TYPE_ENERGY = 3,
  NUM_POWER_TYPES = 4
};

enum UNITSTANDSTATE {
  UNIT_STANDING = 0,
  UNIT_SITTING = 1,
  UNIT_SITTINGCHAIR = 2,
  UNIT_SLEEPING = 3,
  UNIT_SITTINGCHAIRLOW = 4,
  UNIT_FIRSTCHAIRSIT = 4,
  UNIT_SITTINGCHAIRMEDIUM = 5,
  UNIT_SITTINGCHAIRHIGH = 6,
  UNIT_LASTCHAIRSIT = 6,
  UNIT_DEAD = 7,
  UNIT_KNEEL = 8,
  UNIT_NUMSTANDSTATES = 9,
  UNIT_NUMCHAIRSTATES = 3
};

extern bool g_standStateAllowsSheathing[12];

enum GAME_ERROR_TYPE {
  GERR_INV_FULL = 0,
  GERR_CANT_EQUIP_LEVEL_I = 1,
  GERR_CANT_EQUIP_SKILL = 2,
  GERR_CANT_EQUIP_EVER = 3,
  GERR_PROFICIENCY_NEEDED = 4,
  GERR_WRONG_SLOT = 5,
  GERR_BAG_FULL = 6,
  GERR_DESTROY_NONEMPTY_BAG = 7,
  GERR_BAG_IN_BAG = 8,
  GERR_AMMO_ONLY = 9,
  GERR_NO_SLOT_AVAILABLE = 10,
  GERR_WRONG_BAG_TYPE = 11,
  GERR_ITEM_MAX_COUNT = 12,
  GERR_NOT_EQUIPPABLE = 13,
  GERR_CANT_STACK = 14,
  GERR_CANT_SWAP = 15,
  GERR_SLOT_EMPTY = 16,
  GERR_ITEM_NOT_FOUND = 17,
  GERR_TOO_FEW_TO_SPLIT = 18,
  GERR_SPLIT_FAILED = 19,
  GERR_NOT_ENOUGH_GOLD = 20,
  GERR_NOT_A_BAG = 21,
  GERR_NOT_OWNER = 22,
  GERR_ONLY_ONE_QUIVER = 23,
  GERR_NO_BANK_SLOT = 24,
  GERR_NO_BANK_HERE = 25,
  GERR_ITEM_LOCKED = 26,
  GERR_2HANDED_EQUIPPED = 27,
  GERR_VENDOR_NOT_INTERESTED = 28,
  GERR_VENDOR_HATES_YOU = 29,
  GERR_VENDOR_SOLD_OUT = 30,
  GERR_VENDOR_TOO_FAR = 31,
  GERR_NOT_ENOUGH_MONEY = 32,
  GERR_RECEIVE_ITEM_S = 33,
  GERR_DROP_BOUND_ITEM = 34,
  GERR_TRADE_BOUND_ITEM = 35,
  GERR_TRADE_QUEST_ITEM = 36,
  GERR_TRADE_GROUND_ITEM = 37,
  GERR_TRADE_BAG = 38,
  GERR_SPELL_FAILED_S = 39,
  GERR_ITEM_COOLDOWN = 40,
  GERR_POTION_COOLDOWN = 41,
  GERR_FOOD_COOLDOWN = 42,
  GERR_SPELL_COOLDOWN = 43,
  GERR_ABILITY_COOLDOWN = 44,
  GERR_SPELL_ALREADY_KNOWN_S = 45,
  GERR_SKILL_GAINED_S = 46,
  GERR_SKILL_UP_SI = 47,
  GERR_LEARN_SPELL_S = 48,
  GERR_LEARN_ABILITY_S = 49,
  GERR_LEARN_RECIPE_S = 50,
  GERR_INVITE_PLAYER_S = 51,
  GERR_INVITED_TO_GROUP_S = 52,
  GERR_ALREADY_IN_GROUP_S = 53,
  GERR_PLAYER_BUSY_S = 54,
  GERR_NEW_LEADER_S = 55,
  GERR_NEW_LEADER_YOU = 56,
  GERR_LEFT_GROUP_S = 57,
  GERR_LEFT_GROUP_YOU = 58,
  GERR_GROUP_DISBANDED = 59,
  GERR_DECLINE_GROUP_S = 60,
  GERR_JOINED_GROUP_S = 61,
  GERR_UNINVITE_YOU = 62,
  GERR_BAD_PLAYER_NAME_S = 63,
  GERR_NOT_IN_GROUP = 64,
  GERR_TARGET_NOT_IN_GROUP_S = 65,
  GERR_GROUP_FULL = 66,
  GERR_NOT_LEADER = 67,
  GERR_PLAYER_DIED_S = 68,
  GERR_GUILD_CREATE_S = 69,
  GERR_GUILD_INVITE_S = 70,
  GERR_INVITED_TO_GUILD_SS = 71,
  GERR_ALREADY_IN_GUILD_S = 72,
  GERR_ALREADY_INVITED_TO_GUILD_S = 73,
  GERR_INVITED_TO_GUILD = 74,
  GERR_ALREADY_IN_GUILD = 75,
  GERR_GUILD_ACCEPT = 76,
  GERR_GUILD_DECLINE_S = 77,
  GERR_GUILD_PERMISSIONS = 78,
  GERR_GUILD_JOIN_S = 79,
  GERR_GUILD_FOUNDER_S = 80,
  GERR_GUILD_PROMOTE_SS = 81,
  GERR_GUILD_DEMOTE_SS = 82,
  GERR_GUILD_QUIT_S = 83,
  GERR_GUILD_LEAVE_S = 84,
  GERR_GUILD_REMOVE_SS = 85,
  GERR_GUILD_REMOVE_SELF = 86,
  GERR_GUILD_DISBAND_S = 87,
  GERR_GUILD_DISBAND_SELF = 88,
  GERR_GUILD_LEADER_S = 89,
  GERR_GUILD_LEADER_SELF = 90,
  GERR_GUILD_MOTD_S = 91,
  GERR_GUILD_PLAYER_NOT_FOUND_S = 92,
  GERR_GUILD_PLAYER_NOT_IN_GUILD_S = 93,
  GERR_GUILD_PLAYER_NOT_IN_GUILD = 94,
  GERR_GUILD_CANT_PROMOTE_S = 95,
  GERR_GUILD_CANT_DEMOTE_S = 96,
  GERR_GUILD_NOT_IN_A_GUILD = 97,
  GERR_GUILD_INTERNAL = 98,
  GERR_GUILD_LEADER_IS_S = 99,
  GERR_GUILD_LEADER_CHANGED_SS = 100,
  GERR_GUILD_DISBANDED = 101,
  GERR_GUILD_NOT_ALLIED = 102,
  GERR_GUILD_LEADER_LEAVE = 103,
  GERR_GUILD_NAME_INVALID = 104,
  GERR_GUILD_NAME_EXISTS_S = 105,
  GERR_GUILD_ENTER_NAME = 106,
  GERR_GUILD_NAME_TOO_SHORT = 107,
  GERR_GUILD_NAME_MIXED_LANGUAGES = 108,
  GERR_GUILD_NAME_PROFANE = 109,
  GERR_GUILD_NAME_RESERVED = 110,
  GERR_NO_GUILD_CHARTER = 111,
  GERR_OUT_OF_RANGE = 112,
  GERR_PLAYER_DEAD = 113,
  GERR_CLIENT_LOCKED_OUT = 114,
  GERR_KILLED_BY_S = 115,
  GERR_LOOT_LOCKED = 116,
  GERR_LOOT_TOO_FAR = 117,
  GERR_LOOT_DIDNT_KILL = 118,
  GERR_LOOT_BAD_FACING = 119,
  GERR_LOOT_NOTSTANDING = 120,
  GERR_LOOT_STUNNED = 121,
  GERR_LOOT_NO_UI = 122,
  GERR_QUEST_ACCEPTED_S = 123,
  GERR_QUEST_COMPLETE_S = 124,
  GERR_QUEST_FAILED_S = 125,
  GERR_QUEST_FAILED_BAG_FULL_S = 126,
  GERR_QUEST_FAILED_MAX_COUNT_S = 127,
  GERR_QUEST_FAILED_LOW_LEVEL = 128,
  GERR_QUEST_FAILED_MISSING_ITEMS = 129,
  GERR_QUEST_REWARD_EXP_I = 130,
  GERR_QUEST_REWARD_ITEM_S = 131,
  GERR_QUEST_REWARD_MONEY_S = 132,
  GERR_QUEST_MUST_CHOOSE = 133,
  GERR_QUEST_LOG_FULL = 134,
  GERR_COMBAT_DAMAGE_SSI = 135,
  GERR_INSPECT_S = 136,
  GERR_CANT_USE_ITEM = 137,
  GERR_MUST_EQUIP_ITEM = 138,
  GERR_PASSIVE_ABILITY = 139,
  GERR_2HSKILLNOTFOUND = 140,
  GERR_NO_ATTACK_TARGET = 141,
  GERR_INVALID_ATTACK_TARGET = 142,
  GERR_ATTACK_PACIFIED = 143,
  GERR_ATTACK_DEAD = 144,
  GERR_HUNGER_VERY_LOW = 145,
  GERR_HUNGER_LOW = 146,
  GERR_HUNGER_MED = 147,
  GERR_HUNGER_HIGH = 148,
  GERR_HUNGER_SATIATED = 149,
  GERR_THIRST_VERY_LOW = 150,
  GERR_THIRST_LOW = 151,
  GERR_THIRST_MED = 152,
  GERR_THIRST_HIGH = 153,
  GERR_THIRST_SATIATED = 154,
  GERR_TAXISAMENODE = 155,
  GERR_TAXINOSUCHPATH = 156,
  GERR_TAXIUNSPECIFIEDSERVERERROR = 157,
  GERR_TAXINOTENOUGHMONEY = 158,
  GERR_TAXITOOFARAWAY = 159,
  GERR_TAXINOVENDORNEARBY = 160,
  GERR_TAXINOTVISITED = 161,
  GERR_TAXIPLAYERBUSY = 162,
  GERR_TAXIPLAYERALREADYMOUNTED = 163,
  GERR_TAXIPLAYERSHAPESHIFTED = 164,
  GERR_TAXIPLAYERMOVING = 165,
  GERR_TAXINOPATHS = 166,
  GERR_NO_REPLY_TARGET = 167,
  GERR_GENERIC_NO_TARGET = 168,
  GERR_INITIATE_TRADE_S = 169,
  GERR_TRADE_REQUEST_S = 170,
  GERR_TRADE_TOO_FAR = 171,
  GERR_TRADE_CANCELLED = 172,
  GERR_TRADE_COMPLETE = 173,
  GERR_TRADE_BAG_FULL = 174,
  GERR_TRADE_TARGET_BAG_FULL = 175,
  GERR_TRADE_MAX_COUNT_EXCEEDED = 176,
  GERR_TRADE_TARGET_MAX_COUNT_EXCEEDED = 177,
  GERR_MOUNT_INVALIDMOUNTEE = 178,
  GERR_MOUNT_TOOFARAWAY = 179,
  GERR_MOUNT_ALREADYMOUNTED = 180,
  GERR_MOUNT_NOTMOUNTABLE = 181,
  GERR_MOUNT_NOTYOURPET = 182,
  GERR_MOUNT_OTHER = 183,
  GERR_MOUNT_LOOTING = 184,
  GERR_MOUNT_RACECANTMOUNT = 185,
  GERR_MOUNT_SHAPESHIFTED = 186,
  GERR_DISMOUNT_NOPET = 187,
  GERR_DISMOUNT_NOTMOUNTED = 188,
  GERR_DISMOUNT_NOTYOURPET = 189,
  GERR_SPELL_FAILED_TOTEMS = 190,
  GERR_SPELL_FAILED_REAGENTS = 191,
  GERR_SPELL_FAILED_EQUIPPED_ITEM = 192,
  GERR_SPELL_FAILED_EQUIPPED_ITEM_CLASS_S = 193,
  GERR_SPELL_FAILED_SHAPESHIFT_FORM_S = 194,
  GERR_BADATTACKFACING = 195,
  GERR_BADATTACKPOS = 196,
  GERR_CHEST_IN_USE = 197,
  GERR_USE_CANT_OPEN = 198,
  GERR_USE_LOCKED = 199,
  GERR_USE_LOCKED_WITH_ITEM_S = 200,
  GERR_USE_LOCKED_WITH_SPELL_S = 201,
  GERR_USE_LOCKED_WITH_SPELL_KNOWN_SI = 202,
  GERR_USE_TOO_FAR = 203,
  GERR_USE_BAD_ANGLE = 204,
  GERR_USE_OBJECT_MOVING = 205,
  GERR_USE_SPELL_FOCUS = 206,
  GERR_USE_DESTROYED = 207,
  GERR_CANTATTACK_NOTSTANDING = 208,
  GERR_SET_LOOT_FREEFORALL = 209,
  GERR_SET_LOOT_ROUNDROBIN = 210,
  GERR_SET_LOOT_MASTER = 211,
  GERR_NEW_LOOT_MASTER_S = 212,
  GERR_SPECIFY_MASTER_LOOTER = 213,
  GERR_TAME_FAILED = 214,
  GERR_CHAT_WHILE_DEAD = 215,
  GERR_NEWTAXIPATH = 216,
  GERR_NO_PET = 217,
  GERR_NOTYOURPET = 218,
  GERR_PET_NOT_RENAMEABLE = 219,
  GERR_NULL_PETNAME = 220,
  GERR_INVALID_PETNAME = 221,
  GERR_QUEST_OBJECTIVE_COMPLETE_S = 222,
  GERR_QUEST_UNKNOWN_COMPLETE = 223,
  GERR_QUEST_ADD_KILL_SII = 224,
  GERR_QUEST_ADD_FOUND_SII = 225,
  GERR_QUEST_ADD_ITEM_SII = 226,
  GERR_CANNOTCREATEDIRECTORY = 227,
  GERR_CANNOTCREATEFILE = 228,
  GERR_PLAYER_WRONG_FACTION = 229,
  GERR_BANKSLOT_FAILED_TOO_MANY = 230,
  GERR_BANKSLOT_INSUFFICIENT_FUNDS = 231,
  GERR_BANKSLOT_NOTBANKER = 232,
  GERR_FRIEND_DB_ERROR = 233,
  GERR_FRIEND_LIST_FULL = 234,
  GERR_FRIEND_ADDED_S = 235,
  GERR_FRIEND_ONLINE_S = 236,
  GERR_FRIEND_OFFLINE_S = 237,
  GERR_FRIEND_NOT_FOUND = 238,
  GERR_FRIEND_WRONG_FACTION = 239,
  GERR_FRIEND_REMOVED_S = 240,
  GERR_FRIEND_ERROR = 241,
  GERR_FRIEND_ALREADY_S = 242,
  GERR_FRIEND_SELF = 243,
  GERR_IGNORE_FULL = 244,
  GERR_IGNORE_SELF = 245,
  GERR_IGNORE_NOT_FOUND = 246,
  GERR_IGNORE_ALREADY_S = 247,
  GERR_IGNORE_ADDED_S = 248,
  GERR_IGNORE_REMOVED_S = 249,
  GERR_ONLY_ONE_BOLT = 250,
  GERR_ONLY_ONE_AMMO = 251,
  GERR_SPELL_FAILED_EQUIPPED_SPECIFIC_ITEM = 252,
  GERR_WRONG_BAG_TYPE_SUBCLASS = 253,
  GERR_CANT_WRAP_STACKABLE = 254,
  GERR_CANT_WRAP_EQUIPPED = 255,
  GERR_CANT_WRAP_WRAPPED = 256,
  GERR_CANT_WRAP_BOUND = 257,
  GERR_CANT_WRAP_UNIQUE = 258,
  GERR_CANT_WRAP_BAGS = 259,
  GERR_OUT_OF_MANA = 260,
  GERR_OUT_OF_RAGE = 261,
  GERR_OUT_OF_FOCUS = 262,
  GERR_OUT_OF_ENERGY = 263,
  GERR_OUT_OF_HEALTH = 264,
  GERR_LOOT_GONE = 265,
  GERR_MOUNT_FORCEDDISMOUNT = 266,
  GERR_AUTOFOLLOW_TOO_FAR = 267,
  GERR_UNIT_NOT_FOUND = 268,
  GERR_INVALID_FOLLOW_TARGET = 269,
  GERR_GUILDEMBLEM_SUCCESS = 270,
  GERR_GUILDEMBLEM_INVALID_TABARD_COLORS = 271,
  GERR_GUILDEMBLEM_NOGUILD = 272,
  GERR_GUILDEMBLEM_COLORSPRESENT = 273,
  GERR_GUILDEMBLEM_NOTGUILDMASTER = 274,
  GERR_GUILDEMBLEM_NOTENOUGHMONEY = 275,
  GERR_GUILDEMBLEM_INVALIDVENDOR = 276,
  GERR_SPELL_OUT_OF_RANGE = 277,
  GERR_COMMAND_NEEDS_TARGET = 278,
  GERR_NOAMMO_S = 279,
  GERR_TOOBUSYTOFOLLOW = 280,
  GERR_DUEL_REQUESTED = 281,
  GERR_DUEL_CANCELLED = 282,
  GERR_DEATHBINDALREADYBOUND = 283,
  GERR_NOEMOTEWHILERUNNING = 284,
  GERR_ZONE_EXPLORED = 285,
  GERR_ZONE_EXPLORED_XP = 286,
  GERR_INVALID_ITEM_TARGET = 287,
  GERR_IGNORING_YOU_S = 288,
  GERR_FISH_NOT_HOOKED = 289,
  GERR_FISH_ESCAPED = 290,
  GERR_SPELL_FAILED_NOTUNSHEATHED = 291,
  GERR_PETITION_SIGNED = 292,
  GERR_PETITION_ALREADY_SIGNED = 293,
  GERR_PETITION_IN_GUILD = 294,
  GERR_PETITION_CREATOR = 295,
  GERR_PETITION_NOT_ENOUGH_SIGNATURES = 296,
  GERR_NONE = 297,
  GERR_NUM_TYPES = 297
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

enum WEAPONMODE {
  WEAPONMODE_NORMALMODE = 0,
  WEAPONMODE_SHEATHEDMODE = 1,
  WEAPONMODE_RANGEDMODE = 2,
  WEAPONMODE_NUMMODES = 3
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
  friend BOOL Player_C_AppFocusMovementHandler(int focus);

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
    return (float)m_unit->health / m_unit->maxHealth;
  }
  int GetPower(POWER_TYPE powerType) const {
    return powerType == -2 ? m_unit->health : m_unit->power[powerType];
  }
  int GetMaxPower(POWER_TYPE powerType) const {
    return powerType == -2 ? m_unit->maxHealth : m_unit->maxPower[powerType];
  }
  float GetPowerPercent(POWER_TYPE powerType) const;
  POWER_TYPE GetDisplayPower() const {
    return (POWER_TYPE)m_unit->displayPower;
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
    return (UNIT_SEX)m_unit->sex;
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
  NTempest::C3Vector GetRawPosition() const {
    return m_move.GetRawPosition();
  }
  float              GetFacing() const;
  float              GetRawFacing() const;
  NTempest::C3Vector GetAnchorPosition() const;
  void               GetAnchorPosition(NTempest::C3Vector &position) const;
  float              GetAnchorFacing() const;
  float GetPitch() const {
    return m_move.GetPitch();
  }
  NTempest::C3Vector GetGroundNormal() const {
    return m_move.GetGroundNormal();
  }
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
    m_unit = (CGUnitData *)storage;
  }
  UINT GetAttackRoundTime(COMBATHAND hand) const {
    FATALASSERT(hand<NUMHANDS);
    return m_unit->attackRoundBaseTime[hand];
  }
  WEAPONMODE GetWeaponMode() const {
    return (WEAPONMODE)m_unit->weaponMode;
  }
  BYTE                    IsUsingRangedWeapon() const;
  bool                    GetSheathed() const;
  virtual UNITAFFILIATION GetGUIDAffiliation(DWORDLONG) const {
    return AFFILIATION_OTHER;
  }
  void                    SetWaterSurfaceElevation(float elevation);

 protected:
  CGUnit(DWORD *storage, const NTempest::C3Vector &position, float facing, const DWORDLONG &guid)
      : m_unit((CGUnitData *)storage), m_move(position, facing, guid) {
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
