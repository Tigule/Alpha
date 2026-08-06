#ifndef WOW_SOURCE_UI_GAMEUI_H
#define WOW_SOURCE_UI_GAMEUI_H

#include "Object/Object.h"
#include "Net/NetClient/NetClient.h"
#include "LootFrame.h"
#include "PartyFrame.h"

#include <Event/EvtApi.h>

class CSimpleFrame;
class CSimpleTop;
class CGTooltip;
class CGCursor;
class CGSpellBook;
class CGBankInfo;
class CGObject_C;
class CinematicSequencesRec;
class CinematicCameraRec;
class CDataStore;
struct Sound;
struct CSpriteClickEvent;
struct CTerrainClickEvent;
struct CWorldClickEvent;
struct CObjectTrackEvent;
struct ATTACKROUNDINFO;
struct MIRRORTIMERDAMAGE;
struct SPELLLOG;
struct HMODEL__;
struct lua_State;

static int DebugAIStateHandler(LPVOID, NETMESSAGE, DWORD, CDataStore *);
static int Script_PickupPetAction(lua_State *L);
static int Script_TogglePetAutocast(lua_State *L);
static int Script_CastPetAction(lua_State *L);
class CMouseEvent;
class CSizeEvent;
class PetAction;
enum SYSMSG_TYPE;

struct CinematicData {
  const CinematicSequencesRec *sequence;
  Sound                       *sequenceMusic;
  int                          currentCamera;
  const CinematicCameraRec    *camera;
  Sound                       *cameraMusic;
  int                          zoneMusicPaused;
};

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

enum UICURSORTYPE {
  UICURSOR_EMPTY = 0,
  UICURSOR_ITEM = 1,
  UICURSOR_MONEY = 2,
  UICURSOR_SPELL = 3,
  UICURSOR_PET_SPELL = 4,
  UICURSOR_PET_ACTION = 5,
  UICURSOR_MERCHANT = 6,
  UICURSOR_LOOT = 7,
  UICURSOR_ACTIONBAR = 8
};

class CGGameUI {
 public:
  static void       ClearClientControls();
  static void       InitializeGame();
  static void       Initialize();
  static void       Shutdown();
  static void       RegisterFrameFactories();
  static void       Reload();
  static void       ShutdownGame();
  static void       UpdateActivePlayer();
  static CGTooltip *GetGameTooltip() {
    return m_gameTooltip;
  }
  static void      UnitNameUpdate(const DWORDLONG &guid);
  static void      UnitPortraitUpdate(const DWORDLONG &guid);
  static void      SetPartyLeader(DWORDLONG guid);
  static void      AddPartyMember(DWORDLONG guid, int connected);
  static void      RemoveAllPartyMembers();
  static void      SetLootMethod(LOOT_METHOD method, DWORDLONG master);
  static void      ClearLootSlot(BYTE slot);
  static void      OpenLoot(CGObject_C *object, int coins, LOOT_ACQUIRE lootType);
  static void      Target(const DWORDLONG &target, int usingNearest);
  static DWORDLONG ClosestObjectMatch(LPCSTR match, OBJECT_TYPE type);
  static void      TargetNearestEnemy(int reverse);
  static void      AssistByName(LPCSTR name);
  static void      FollowByName(LPCSTR name);
  static int       IsPartyMember(const DWORDLONG &guid);
  static void      EnablePartyMember(DWORDLONG guid, int enable);
  static DWORDLONG GetPartyMember(UINT index);
  static void      ClearTarget(DWORDLONG guid, int sendTarget);
  static void      ClearInteractTarget();
  static void      ClearInteractTarget(const DWORDLONG &target);
  static void      TargetIfNone(const DWORDLONG &target);
  static void      SetInteractTarget(const DWORDLONG &target, float maxDist);
  static void      CloseInteraction();
  static void      ResetCamera();
  static void      SysMsgDisplay(LPCSTR msg, SYSMSG_TYPE severity);
  static int       FilterMouseDown(const CMouseEvent &evt);
  static int       HandleMouseDown(const CMouseEvent &evt);
  static int       HandleMouseUp(const CMouseEvent &evt);
  static int       HandleDisplaySizeChanged(const CSizeEvent &evt);
  static void      ScaleUI(float scale, int force);
  static void      NamePlateClicked(DWORDLONG unit, MOUSEBUTTON button);
  static void      EnterWorld();
  static void      LeaveWorld();
  static void      UpdateInteractTarget();
  static DWORDLONG GetCursorItem();
  static void      SetCursorItem(DWORDLONG itemGUID, DWORDLONG containerGUID, UINT slot, int unlock, UINT stackSplit);
  static UINT      GetCursorMoney() {
    return m_cursorMoney;
  }
  static UINT GetCursorStackSplit() {
    return m_stackSplit;
  }
  static void          SetCursorMoney(UINT money);
  static void          SetCursorSpell(int spellId, int pet);
  static void          DropCursorSpell();
  static void          SetCursorPetAction(const PetAction &action);
  static void          DropCursorPetAction();
  static int           GetCursorSpell();
  static void          GetCursorItem(DWORDLONG &cursorItem, DWORDLONG &containerGUID, UINT &slot);
  static UINT          GetCursorVirtualItem();
  static UINT          GetCursorVirtualItem(UICURSORTYPE type);
  static void          SetCursorVirtualItem(UINT itemID, UINT displayID, UINT slot, UICURSORTYPE type);
  static void          GetCursorVirtualItem(UINT &cursorItem, UINT &slot);
  static UINT          GetCursorPetAction();
  static UICURSORTYPE  GetCursorType();
  static int           IsCursorEmpty();
  static int           IsCursorPetSpell();
  static void          UnlockItem(DWORDLONG itemGUID);
  static void          UnlockAllItems();
  static void          LockItem(DWORDLONG itemGUID);
  static void          CloseLoot(bool send, bool moving);
  static void          NewZoneFeedback(int areaID, LPCSTR zoneString, LPCSTR subZoneString);
  static void          SetMinimapZoneText(LPCSTR areaName);
  static void          ClearCursor(int unlock);
  static void          DeleteCursorItem();
  static void          PlayerCombatModeChanged(int newState);
  static void          StartCinematic(int cinematicID);
  static void          BeginCinematic();
  static void          BeginCinematicInternal(LPVOID);
  static int           StartCinematicCamera();
  static int           NextCinematic(LPVOID);
  static void          NextCinematicInternal(LPVOID);
  static int           StopCinematic(LPVOID);
  static void          StopCinematicInternal(LPVOID);
  static void          HideCursor();
  static void          ShowCursor();
  static void          ShowHealingFeedback(const DWORDLONG &guid, int amount);
  static void          ShowSpellMissFeedback(DWORDLONG victim, int reason);
  static void          OnClientControlChanged(int hasControl);
  static void          ShowCombatFeedback(const ATTACKROUNDINFO *info);
  static void          ShowCombatFeedback(const SPELLLOG &log);
  static void          ShowCombatFeedback(const DWORDLONG &guid, int amount, int damageClass, UINT flags);
  static void          ShowCombatFeedback(const MIRRORTIMERDAMAGE &log);
  static void          OnItemPush(DWORDLONG player, int slot, int itemID, int pushed, int display);
  static void          OpenPartyInvite(LPCSTR inviter);
  static void          OpenResurrectRequest(LPCSTR inviter);
  static void          CancelPartyInvite();
  static void          OpenGuildInvite(LPCSTR inviter, LPCSTR guildName);
  static void          CancelGuildInvite();
  static void          AddErrorMessage(LPCSTR string, int error);
  static void __cdecl  DisplayError(GAME_ERROR_TYPE errorType, ...);
  static LPCSTR        GetLastErrorString();
  static void          ShowAutoFollowChange(DWORDLONG newTarget, DWORDLONG oldTarget, int type);
  static int           HandleSpriteClick(const CSpriteClickEvent &evt);
  static int           HandleTerrainClick(const CTerrainClickEvent &evt);
  static int           HandleWorldClick(const CWorldClickEvent &evt);
  static void          HandleSpriteTrack(const CObjectTrackEvent &evt);
  static void          HandleScreenshot(int success);
  static void          HandleObjectTrackChange(DWORDLONG object, DWORDLONG oldGUID, float x, float y);
  static CSimpleFrame *GetUISimpleParent();
  static int           GetCurrentAreaID();
  static int           HasPlayerControl();

 private:
  static int  OnTerrainClick(const CTerrainClickEvent &evt);
  static int  OnSpriteLeftClick(DWORDLONG object, float x, float y);
  static int  OnSpriteRightClick(DWORDLONG object, float x, float y);
  static void UpdatePlayerAlpha(float alpha);
  static void ResetStaticVars();

 public:
  static void OnTargetContextAction();

  static const DWORDLONG &GetCurrentObjectTrack() {
    return m_currentObjectTrack;
  }

  static const DWORDLONG &GetInteractTarget() {
    return m_interactTarget;
  }

  static const DWORDLONG &GetLockedTarget() {
    return m_lockedTarget;
  }

  static const DWORDLONG &GetLastEnemyTarget() {
    return m_lastEnemyTarget;
  }

  static LPCSTR GetZoneText() {
    return m_zoneText;
  }

  static LPCSTR GetSubZoneText() {
    return m_subZoneText;
  }

  static LPCSTR GetMinimapZoneText() {
    return m_minimapZoneText;
  }

 private:
  friend int DebugAIStateHandler(LPVOID, NETMESSAGE, DWORD, CDataStore *);

  static void UpdateObjectHighlightColor(HMODEL__ *model, CGObject_C *object);
  friend class CGTooltip;
  friend class CGCursor;
  friend class CGSpellBook;
  friend class CGBankInfo;
  friend class CGActionBar;
  friend class CGPlayer_C;
  friend class CGMinimapFrame;
  friend class CGWorldMap;
  friend int Script_PickupPetAction(lua_State *L);
  friend int Script_TogglePetAutocast(lua_State *L);
  friend int Script_CastPetAction(lua_State *L);
  friend int Script_CursorHasItem(lua_State *L);
  friend int Script_CursorHasSpell(lua_State *L);
  friend int Script_CursorHasMoney(lua_State *L);
  friend int Script_HasFullControl(lua_State *L);
  friend int Script_DeleteCursorItem(lua_State *);
  friend int Script_TargetLastEnemy(lua_State *);

  static int Idle(LPCVOID data, LPVOID param);

  static CSimpleFrame *m_UISimpleParent;
  static CSimpleTop   *m_simpleTop;
  static bool          m_reloadUI;
  static UINT          m_stackSplit;
  static DWORDLONG     m_cursorItem;
  static DWORDLONG     m_cursorItemContainer;
  static UINT          m_cursorItemSlot;
  static UINT          m_cursorMoney;
  static int           m_cursorSpell;
  static UINT          m_cursorPetAction;
  static UINT          m_cursorVirtualID;
  static UINT          m_cursorVirtualDisplay;
  static UINT          m_cursorVirtualSlot;
  static int           m_cursorHasAction;
  static UICURSORTYPE  m_cursorItemType;
  static DWORDLONG     m_currentObjectTrack;
  static float         m_interactMaxDist;
  static DWORDLONG     m_interactTarget;
  static DWORDLONG     m_lockedTarget;
  static DWORDLONG     m_lastEnemyTarget;
  static char         *m_zoneText;
  static char         *m_subZoneText;
  static char         *m_minimapZoneText;
  static int           m_areaID;
  static int           m_hasControl;
  static int           m_screenWidth;
  static CGTooltip    *m_gameTooltip;
  static CinematicData m_cinematic;
  static char          s_lastErrorString[512];
};

#endif
