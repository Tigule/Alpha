#include "Object/ObjectClient/Player_C.h"

#include "Client.h"
#include "Component/CharacterCustomization.h"
#include "Component/Component.h"
#include "Console/ConsoleCommand.h"
#include "Console/ConsoleClient.h"
#include "Console/ConsoleVar.h"
#include "Base/CDataStore.h"
#include "DB/DBClient/AutoCode/AreaTriggerRec.h"
#include "DB/DBClient/AutoCode/AreaTableRec.h"
#include "DB/DBClient/AutoCode/ChrRacesRec.h"
#include "DB/DBClient/AutoCode/CreatureDisplayInfoRec.h"
#include "DB/DBClient/AutoCode/CreatureModelDataRec.h"
#include "DB/DBClient/AutoCode/EmotesTextRec.h"
#include "DB/DBClient/AutoCode/FactionRec.h"
#include "DB/DBClient/AutoCode/ItemSubClassRec.h"
#include "DB/DBClient/AutoCode/ItemDisplayInfoRec.h"
#include "DB/DBClient/AutoCode/ItemVisualEffectsRec.h"
#include "DB/DBClient/AutoCode/ItemVisualsRec.h"
#include "DB/DBClient/AutoCode/LockRec.h"
#include "DB/DBClient/AutoCode/SkillLineAbilityRec.h"
#include "DB/DBClient/AutoCode/SkillLineRec.h"
#include "DB/DBClient/AutoCode/SpellItemEnchantmentRec.h"
#include "DB/DBClient/AutoCode/SpellRec.h"
#include "DB/DBClient/AutoCode/TaxiNodesRec.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "DB/DBClient/DBClient.h"
#include "DB/WowLocale.h"
#include "Game/GameClient/TaxiMap.h"
#include "Game/GameClient/GuildClient.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Object/ObjectClient/Item_C.h"
#include "Object/ObjectClient/Bag_C.h"
#include "Object/ObjectClient/GameObject_C.h"
#include "Object/Petition.h"
#include "Game/GameClient/NameCache.h"
#include "Game/GameClient/PlayerName.h"
#include "SoundInterface/SoundInterface.h"
#include "Services/SysMessage.h"
#include "Services/AsyncFileRead.h"
#include "Ui/ActionBarFrame.h"
#include "Ui/ChatFrame.h"
#include "Ui/ClassTrainerFrame.h"
#include "Ui/GameUI.h"
#include "Ui/ItemTextFrame.h"
#include "Ui/PaperDollInfoFrame.h"
#include "Ui/PetInfo.h"
#include "Ui/PartyFrame.h"
#include "Ui/MinimapFrame.h"
#include "Ui/LootFrame.h"
#include "Ui/ReputationInfo.h"
#include "Ui/SpellBookFrame.h"
#include "Ui/Tutorial.h"
#include "Ui/TaxiMapFrame.h"
#include "Ui/WorldFrame.h"
#include "WorldClient/AreaList.h"
#include "WorldClient/World.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"
#include "WowSvcs/WowSvcsClient/FriendList.h"

#include <stpl.h>
#include <FrameScript/FrameScript.h>
#include <Model/IModel.h>
#include <Os/OsTime.h>
#include <Tempest/crect.h>
#include <ctype.h>
#include <malloc.h>

struct InitialSpellStruct {
  unsigned short spellID;
  short          slot;
};

class CGTradeSkillInfo {
 public:
  static void RefreshList(int resetFilters);
};

class CGCraftInfo {
 public:
  static void RefreshList();
};

const SkillLineAbilityRec *SpellTableLookupAbility(unsigned int raceID, unsigned int classID, unsigned int spellID);
void UnitDebugCombatLogOnEnable(int enable);
int InvSlotToObjAttachSlot(int invSlot);

struct ITEMSWAP {
  unsigned __int64 bagA;
  unsigned __int64 bagB;
  int              slotA;
  int              slotB;
  int              pendingID;
};

struct LootItem {
  unsigned int m_itemID;
  unsigned int m_displayID;
  unsigned int m_quantity;
};

struct RandomRollInfo {
  int min;
  int max;
  int result;
};

static const char coinToken[3][16] = {"COPPER", "SILVER", "GOLD"};
static const char NONAME[7] = "NoName";

struct ITEMEXPIRATION : public TSHashObject<ITEMEXPIRATION, CHashKeyGUID> {
  int timeLeft;
  int enchantmentTimeLeft[5];
};

NODEDECL(DEFERREDDAMAGE) {
  int              normal;
  unsigned int     flags;
  unsigned int     damage;
  unsigned __int64 victim;

  void Set(int normalCombatDamage, unsigned int damageFlags, unsigned int damageAmount, unsigned __int64 victimGUID) {
    normal = normalCombatDamage;
    flags = damageFlags;
    damage = damageAmount;
    victim = victimGUID;
  }
};

NODEDECL(DEFERREDSPELLMISS) {
  unsigned __int64 victim;
  MISS_REASON      reason;
  int              spellID;

  void Set(unsigned __int64 victimGUID, MISS_REASON missReason, int missedSpellID) {
    victim = victimGUID;
    reason = missReason;
    spellID = missedSpellID;
  }
};

struct PetitionVendorItem {
  unsigned int m_muid;
  unsigned int m_itemID;
  unsigned int m_itemDisplayID;
  int          m_price;
  int          m_flags;
};

struct VendorItem {
  unsigned int m_muid;
  unsigned int m_itemType;
  unsigned int m_itemDisplayID;
  int          m_quantity;
  int          m_price;
  int          m_durability;
  int          m_stackCount;
};

enum PETITION_ERROR {
  PETITION_SUCCESS = 0,
  PETITION_ALREADY_SIGNED = 1,
  PETITION_ALREADY_IN_GUILD = 2,
  PETITION_CHARTER_CREATOR = 3,
  PETITION_NOT_ENOUGH_SIGNATURES = 4,
  PETITION_UNKNOWN_ERROR = 5
};

class CGBuffBar {
 public:
  static void UpdateDuration(unsigned char slot, unsigned int duration);
};

class CGContainerInfo {
 public:
  static void OpenContainer(unsigned __int64 container);
  static void UpdateItem(unsigned __int64 item);
};

class CGTradeInfo {
 public:
  static void UpdatePlayerItem(unsigned __int64 item);
};

class CGBankInfo {
 public:
  static void OpenBank(const unsigned __int64 &guid);
};

static TSGrowableArray<InitialSpellStruct> s_initialSpells;
static TSCArray<int, 120>                  s_initialButtons;
static GAME_ERROR_TYPE s_tabardErrors[7] = {static_cast<GAME_ERROR_TYPE>(270), static_cast<GAME_ERROR_TYPE>(271), static_cast<GAME_ERROR_TYPE>(272),
                                            static_cast<GAME_ERROR_TYPE>(273), static_cast<GAME_ERROR_TYPE>(274), static_cast<GAME_ERROR_TYPE>(275),
                                            static_cast<GAME_ERROR_TYPE>(276)};
static PetitionVendorItem         petitionList[10];
static VendorItem                 s_lastVendorList[128];
static int                        currentAreaTrigger;
static unsigned int               s_tempCombatModeCooldown;
static unsigned int               s_attackBreakTimer;
static int                        s_bindSaved;
static NTempest::C3Vector         s_bindPosition;
static unsigned int               s_bindZoneID;
static TSFixedArray<unsigned int> s_weaponSubclassSpells;
static unsigned int               s_defenseSkillID;
static TSGrowableArray<ITEMSWAP>  s_pendingSwaps;
static unsigned __int64           s_lastVendorListReceived;
unsigned __int64                  s_giftWrapItem;
static unsigned __int64           s_lastBinderID;
static int                        s_lastQuestList[32];
static int                        s_lastQuestListType[32];
static int                        s_lastQuestLevel[32];
static unsigned __int64           s_lastQuestGiverListRevieved;
static unsigned int               s_combatModeTimer;
static CVar                      *s_namePlateRenderOwn;
static float                      s_attackBreakDistanceSquared = 100.0f;
static unsigned int               s_playerProficiencies[16];
static unsigned __int64           s_resurrectOffer;

void ModelShowBoundingSphere(HMODEL model);
static const char *s_actionsArray[18] = {
    "ALWAYSBLOCK",
    "ALWAYSPARRY",
    "ALWAYSDODGE",
    "ALWAYSCRIT",
    "ALWAYSSPELLCRIT",
    "ALWAYSPROC",
    "ALWAYSCOMBATSTUN",
    "ALWAYSHIT",
    "ALWAYSSPELLHIT",
    "ALWAYSMISS",
    "ALWAYSSPELLMISS",
    "ALWAYSATTEMPTDUALWIELD",
    "ALWAYSSPELLINTERRUPT",
    "NEVERSPELLINTERRUPT",
    "ALWAYSIMMUNE",
    "DEBUGCOMBAT_NEVERDODGEMELEE",
    "DEBUGCOMBAT_NEVERPARRYMELEE",
    "DEBUGCOMBAT_NEVERBLOCKMELEE"
};
static GAME_ERROR_TYPE s_playerBankErrors[3] = {
    static_cast<GAME_ERROR_TYPE>(230), static_cast<GAME_ERROR_TYPE>(231), static_cast<GAME_ERROR_TYPE>(232)
};
static GAME_ERROR_TYPE s_mountResultGameErrors[11] = {
    static_cast<GAME_ERROR_TYPE>(178), static_cast<GAME_ERROR_TYPE>(179), static_cast<GAME_ERROR_TYPE>(180), static_cast<GAME_ERROR_TYPE>(181),
    static_cast<GAME_ERROR_TYPE>(182), static_cast<GAME_ERROR_TYPE>(183), static_cast<GAME_ERROR_TYPE>(184), static_cast<GAME_ERROR_TYPE>(185),
    static_cast<GAME_ERROR_TYPE>(186), static_cast<GAME_ERROR_TYPE>(266), static_cast<GAME_ERROR_TYPE>(297)
};
static GAME_ERROR_TYPE s_dismountResultGameErrors[4] = {
    static_cast<GAME_ERROR_TYPE>(187), static_cast<GAME_ERROR_TYPE>(188), static_cast<GAME_ERROR_TYPE>(189), static_cast<GAME_ERROR_TYPE>(297)
};
static GAME_ERROR_TYPE s_taxiErrors[12] = {static_cast<GAME_ERROR_TYPE>(297), static_cast<GAME_ERROR_TYPE>(157), static_cast<GAME_ERROR_TYPE>(156),
                                           static_cast<GAME_ERROR_TYPE>(158), static_cast<GAME_ERROR_TYPE>(159), static_cast<GAME_ERROR_TYPE>(160),
                                           static_cast<GAME_ERROR_TYPE>(161), static_cast<GAME_ERROR_TYPE>(162), static_cast<GAME_ERROR_TYPE>(163),
                                           static_cast<GAME_ERROR_TYPE>(164), static_cast<GAME_ERROR_TYPE>(165), static_cast<GAME_ERROR_TYPE>(155)};
static TSHashTable<ITEMEXPIRATION, CHashKeyGUID>                s_pendingItemExpirations;
static LISTDECL(DEFERREDDAMAGE, s_deferredDamage);
static LISTDECL(DEFERREDSPELLMISS, s_deferredSpellMiss);
static int                                                      s_pendingCinematicID;
static const int                                                CHARACTER_POINTS_PER_LEVEL[2] = {10, 1};
static const int                                                CHARACTER_POINTS_PER_BONUS[2] = {0, 1};
static const int                                                LEVELS_PER_CHARACTER_POINT_BONUS[2] = {1, 5};
static const int                                                CHARACTER_POINT_LEVEL_THRESHOLDS[2] = {10, 1};
static const int                                                CHARACTER_POINT_BONUS_PER_THRESHOLD[2] = {5, 0};
static unsigned int                                             s_areaTriggerCheck_TimerEvent;
static int                                                      s_enableDeathHoldLog;
static int                                                      s_renderPlayer;
static TSGrowableArray<unsigned int>                            s_guildIDs;
static unsigned int                                             s_numLootItems;
static LootItem                                                 s_lootItems[16];
static int                                                      s_questFailedReason;
CVar                                                           *g_combatModeMaxDistance;

void UnitCombatLogSetActivePlayer(CGPlayer_C *playerPtr);
void UnitCombatLogSpellMissed(unsigned int missReason, unsigned int spellID, unsigned __int64 caster, unsigned __int64 victim);

enum CURSORANIMATIONS {
  POINT_CURSOR = 0,
  CAST_CURSOR = 1
};

void CursorSetCursorMode(CURSORANIMATIONS mode);

enum SPELL_FAILED_REASON {
  SPELL_FAILED_ERROR = 14
};

void Spell_C_CancelSpell(unsigned int failed, unsigned int notifyServer, SPELL_FAILED_REASON reason);
void Spell_C_CancelCombatSpell();
void Spell_C_SetCooldownLeft(
    int  spellID,
    int  itemID,
    int  category,
    int  recoveryLeft,
    int  categoryRecoveryLeft,
    bool needsEvent,
    int  isPet,
    int  startRecoveryTimeLeft
);
void PlayerInitializeSounds();
void PlayerShutdownSounds();
const ItemSubClassRec *SDBItemSubclassGetSubClassRec(unsigned int classID, unsigned int subClassID);
int SheatheTypeToSheathePoint(int sheatheType, int invSlot);
void Script_SendUnitSignal(const unsigned __int64 &guid, int signal);

class CGTabardCreationFrame {
 public:
  static void Open(const unsigned __int64 &vendor);
};

class CGGuildRegistrar {
 public:
  static void SetRegistrar(unsigned __int64 registrar, const PetitionVendorItem *petition);
};

class CGPetitionInfo {
 public:
  static void SetPetition(unsigned __int64 petition, int petitionID);
  static void SetSignatures(unsigned char count, unsigned __int64 *signers, int *choices);
};

class CGMerchantInfo {
 public:
  static void SetMerchant(unsigned __int64 merchantGUID, VendorItem *items, int count);
  static void UpdateItemQuantity(unsigned __int64 vendor, unsigned long muid, int newQuantity);
};

enum QUEST_STATE {
  QUEST_GREETING = 0,
  QUEST_DETAIL = 1,
  QUEST_PROGRESS = 2,
  QUEST_REWARD = 3,
  QUEST_STATE_NUM_TYPES = 4
};

class CGQuestInfo {
 public:
  static const unsigned __int64 &GetQuestGiver();
  static int GetLastChosenItem();
  static void ClearLastChosenItem();
  static void SetState(unsigned __int64 guid, QUEST_STATE state, const char *text, int quest);
  static void SetLogDescription(const char *desc);
  static void AddQuest(int quest, const char *desc, int questLevel, int turnIn);
  static void AddQuestInProgress(int quest, const char *desc, int questLevel);
  static void EndQuestList();
  static void AddReward(
      const char *title,
      int        *itemChoice,
      int        *choiceDisplay,
      int        *choiceAmount,
      int         numChoice,
      int        *itemReward,
      int        *itemDisplay,
      int        *itemAmount,
      int         numReward,
      int         money,
      int         autoLaunched
  );
  static void
  AddItemRequest(const char *title, int *items, int *itemAmount, int *itemDisplay, int numItems, int completed, int autoLaunched);
  static void QuestGiverFinished();
  static void ConfirmAcceptQuest(int questID, const char *questTitle, const unsigned __int64 &initiatedBy);
};

void CurrencyBreakdown(int money, int *coins);
const char *CurrencyAbbreviation(int coinType);

int OnPlayerEvent(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
int OnVendorEvent(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
int OnLootEvent(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
int OnLearnedSpell(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
int OnSupercededSpell(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
int OnInitialSpells(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
int OnActionButtons(void *__formal, NETMESSAGE, unsigned long, CDataStore *msg);
int OnPetSpells(void *__formal, NETMESSAGE, unsigned long, CDataStore *msg);
int OnGroupInvite(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
int OnGroupCancel(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
int OnGroupDecline(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
int OnGroupUninvite(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
int OnGroupNewLeader(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
int OnGroupDestroy(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
int OnGroupCommandResult(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
int OnGroupList(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
int OnQuestGiverEvent(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
int OnTrainerEvent(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
int OnProficiency(void *, NETMESSAGE, unsigned long, CDataStore *msg);
int OnResurrectRequest(void *, NETMESSAGE, unsigned long, CDataStore *msg);
int OnInspectNotify(void *, NETMESSAGE, unsigned long, CDataStore *msg);
int OnFactionUpdate(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
int OnReadItemResult(void *, NETMESSAGE msgID, unsigned long, CDataStore *msg);
int OnCancelCombat(void *, NETMESSAGE, unsigned long, CDataStore *);
int OnGuildInvite(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
int OnGuildDecline(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
int OnGuildInfo(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
int OnGuildRoster(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
int OnGuildEvent(void *, NETMESSAGE, unsigned long, CDataStore *msg);
int OnGuildCommandResult(void *, NETMESSAGE, unsigned long, CDataStore *msg);
int OnGuildEmblemError(void *, NETMESSAGE, unsigned long, CDataStore *msg);
int OnGuildEmblemActivate(void *, NETMESSAGE, unsigned long, CDataStore *msg);
int OnNpcPetitionEvent(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
int OnPlayEmote(void *, NETMESSAGE, unsigned long, CDataStore *msg);
int HandlePartyMemberStats(void *, NETMESSAGE, unsigned long, CDataStore *msg);
int OnQuestUpdate(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
int OnQuestConfirm(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
int OnMirrorTimerEvent(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);
int OnItemEvent(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);

int BootMeHandler(const char *command, const char *arguments);
int RepopPlayerHandler(const char *command, const char *arguments);
int WhoCommandHandler(const char *command, const char *arguments);
int BuyCommandHandler(const char *command, const char *arguments);
int UndressMeHandler(const char *command, const char *arguments);
int GodmodeHandler(const char *command, const char *arguments);
int CCommand_LevelUp(const char *command, const char *arguments);
int CCommand_SetFaction(const char *command, const char *arguments);
int CCommand_Invite(const char *command, const char *arguments);
int CCommand_Accept(const char *command, const char *arguments);
int CCommand_Decline(const char *command, const char *arguments);
int CCommand_Disband(const char *command, const char *arguments);
int CCommand_NewLeader(const char *command, const char *arguments);
int CCommand_Uninvite(const char *command, const char *arguments);
int CCommand_AcceptRes(const char *, const char *);
int CCommand_DeclineRes(const char *, const char *);
int CCommand_ShowPet(const char *, const char *);
int CCommand_TaxiShowNodes(const char *, const char *);
int CCommand_GuildCreate(const char *command, const char *arguments);
int CCommand_TogglePVP(const char *command, const char *arguments);
int CCommand_Cinematic(const char *command, const char *arguments);
int CCommand_ForceActionSet(const char *command, const char *arguments);
int CCommand_ForceActionUnset(const char *command, const char *arguments);
int CCommand_ForceActionOnOtherSet(const char *command, const char *arguments);
int CCommand_ForceActionOnOtherUnset(const char *command, const char *arguments);
int CCommand_ForceActionShowFlags(const char *command, const char *arguments);
int CCommand_ForceMonsterAnim(const char *__formal, const char *arguments);
int CCommand_ResetMonsterAnim(const char *, const char *);
int CCommand_DumpDeathHoldLogs(const char *, const char *);

int AreaTriggerCheck(const void *eventData, void *arg);
static void AreaTriggersInitialize();
static void AreaTriggersShutdown();

static int CountWeaponItemSubclasses(int *number);
static int FindFirstSetBit(unsigned int field, int *whichBitSet);
static void InitializeWeaponSubclassSpells();

int PlayerAttackBreakHandler(const void *data, unsigned __int64 guid, void *param) {
  if (ClntObjMgrGetPlayerType() != PLAYER_BOT) {
    FATALASSERT(s_attackBreakTimer);
    s_attackBreakTimer = 0;

    CGPlayer_C *playerPtr = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
    FATALASSERT(playerPtr);
    FATALASSERT(playerPtr->GetGUID() == ClntObjMgrGetActivePlayer());

    if (!playerPtr->OnAttackBreakHandler()) {
      playerPtr->CGUnit_C::StopAttack();
      return 1;
    }

    s_attackBreakTimer = ClientSetTimer(500, PlayerAttackBreakHandler, guid, param);
  }

  return 1;
}

static int PlayerCombatModeHandler(const void *data, unsigned __int64 guid, void *param) {
  s_combatModeTimer = 0;
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (unit) {
    unit->OnCombatModeTimer();
  }
  return 1;
}

int Player_C_ZoneUpdateHandler(const void *eventData, void *arg) {
  static unsigned int updateCount;

  if (++updateCount >= 10) {
    updateCount -= 10;
    unsigned __int64 guid = ClntObjMgrGetActivePlayer();
    CGPlayer_C      *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
    if (player) {
      AreaListRegisterLocation(player->GetPosition(), ClntObjMgrGetMapID(), player->GetWorldObject());
    }
  }

  return 1;
}

int RepopPlayerHandler(const char *command, const char *arguments) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->HandleRepopRequest();
  }
  return 1;
}

int WhoCommandHandler(const char *command, const char *arguments) {
  g_friendList->SendWho(arguments);
  return 1;
}

void ShowForceActionFlags(unsigned int *flags) {
  for (unsigned int target = 0; target < 2; ++target) {
    ConsoleWrite(target ? "Victim force flags:" : "Self force flags:", DEFAULT_COLOR);
    for (unsigned int action = 0; action < 18; ++action) {
      ConsolePrintf("[%d] %s: %s", action, s_actionsArray[action], flags[target] & (1 << action) ? "on" : "off");
    }
  }
}

void RandomRollNameQueryCallback(int, const unsigned __int64 &guid, void *arg, bool granted) {
  char            buf[256];
  RandomRollInfo *info = static_cast<RandomRollInfo *>(arg);
  FATALASSERT(info);

  if (granted) {
    const NameCache *name = g_nameDBCache.GetRecord(guid, guid, 0, 0);
    if (name) {
      SStrPrintf(
          buf, sizeof(buf), FrameScript_GetText("RANDOM_ROLL_RESULT", -1, GENDER_NOT_APPLICABLE), name->m_name, info->result, info->min, info->max
      );
      CGChat::AddChatMessage(buf, static_cast<SLASH_COMMAND_ID>(9), 0, 0, 0, 0, 0);
    }
  }

  FREE(info);
}

static int BankInvHandler(unsigned __int64 guid, unsigned int offset, unsigned int bytes, const void* prevValue, void* param) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (player) {
    unsigned int slot = offset >> 3;
    unsigned __int64 currGuid = player->GetBag()->GetItem(slot);
    if (*static_cast<const unsigned __int64 *>(prevValue) != currGuid) {
      CGGameUI::UnlockItem(currGuid);
    }
    FrameScript_SignalEvent(326);
  }
  return 1;
}

int OnPlayerEvent(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  FATALASSERT(msg);

  switch (msgId) {
    case SMSG_FORCEACTIONSHOW: {
      unsigned int flags[2];
      msg->Get(flags[0]);
      msg->Get(flags[1]);
      ShowForceActionFlags(flags);
      return 1;
    }

    case SMSG_GODMODE: {
      unsigned int enabled = 0;
      msg->Get(*reinterpret_cast<unsigned char *>(&enabled));
      ConsoleWrite(enabled ? "Godmode enabled" : "Godmode disabled", DEFAULT_COLOR);
      return 1;
    }

    case SMSG_TRIGGER_CINEMATIC: {
      int cinematicID;
      msg->Get(cinematicID);
      if (ClientServices_CharacterIsInGame()) {
        CGGameUI::StartCinematic(cinematicID);
      } else {
        s_pendingCinematicID = cinematicID;
      }
      return 1;
    }

    case SMSG_INVENTORY_CHANGE_FAILURE: {
      unsigned int result = 0;
      msg->Get(*reinterpret_cast<unsigned char *>(&result));
      if (result == BAG_OK) {
        return 1;
      }

      GAME_ERROR_TYPE error = CGBag_C::GetGameError(static_cast<BAG_RESULT>(result));
      int             itemID = 0;
      if (result == BAG_LEVEL_MISMATCH) {
        msg->Get(itemID);
      }

      unsigned __int64 item1;
      unsigned __int64 item2;
      unsigned int     containerBSlot = 0;
      msg->Get(item1);
      msg->Get(item2);
      msg->Get(*reinterpret_cast<unsigned char *>(&containerBSlot));

      if (result == BAG_LEVEL_MISMATCH) {
        CGGameUI::DisplayError(error, itemID);
      } else {
        int displayError = 1;
        if (result == BAG_ITEM_SUBTYPE_MISMATCH) {
          CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
          if (player && player->ReportBagItemSubtypeMismatch(containerBSlot)) {
            displayError = 0;
          }
        }
        if (displayError) {
          CGGameUI::DisplayError(error);
        }
      }

      if (item1) {
        CGGameUI::UnlockItem(item1);
      }
      if (item2) {
        CGGameUI::UnlockItem(item2);
      }
      return 1;
    }

    case SMSG_OPEN_CONTAINER: {
      unsigned __int64 container;
      msg->Get(container);
      CGContainerInfo::OpenContainer(container);
      return 1;
    }

    case SMSG_UPDATE_AURA_DURATION: {
      unsigned int slot = 0;
      unsigned int duration;
      msg->Get(*reinterpret_cast<unsigned char *>(&slot));
      msg->Get(duration);
      CGBuffBar::UpdateDuration(slot, duration);
      return 1;
    }

    case SMSG_HEALSPELL_ON_PLAYER: {
      int amount;
      msg->Get(amount);
      const unsigned __int64 player = ClntObjMgrGetActivePlayer();
      CGGameUI::ShowHealingFeedback(player, amount);
      return 1;
    }

    case SMSG_HEALSPELL_ON_PLAYERS_PET: {
      unsigned __int64 petGUID;
      int              amount;
      msg->Get(petGUID);
      msg->Get(amount);
      CGGameUI::ShowHealingFeedback(petGUID, amount);
      return 1;
    }

    case SMSG_BINDPOINTUPDATE:
      CGPlayer_C::SaveBindPoint(msg);
      return 1;

    case SMSG_BINDZONEREPLY: {
      int  mapID;
      int  areaID;
      char buf[128];
      char string[256];
      msg->Get(mapID);
      msg->Get(areaID);
      if (AreaListGetName(mapID, areaID & 0xFFFF, static_cast<unsigned int>(areaID) >> 16, buf, sizeof(buf), 0)) {
        SStrPrintf(string, sizeof(string), FrameScript_GetText("BIND_ZONE_DISPLAY", -1, GENDER_NOT_APPLICABLE), buf);
        CGChat::AddChatMessage(string, static_cast<SLASH_COMMAND_ID>(9), 0, 0, 0, 0, 0);
      }
      return 1;
    }

    case SMSG_PLAYERBOUND: {
      unsigned __int64 binderID;
      msg->Get(binderID);
      s_lastBinderID = binderID;
      SndInterfacePlaySound(0x475, -1);
      CGChat::AddChatMessage(FrameScript_GetText("DEATHBIND_SUCCESSFUL", -1, GENDER_NOT_APPLICABLE), static_cast<SLASH_COMMAND_ID>(9), 0, 0, 0, 0, 0);
      return 1;
    }

    case SMSG_DEATH_NOTIFY: {
      unsigned __int64 unit;
      msg->Get(unit);
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (player) {
        player->SaveDeathMessage(unit);
      }
      return 1;
    }

    case SMSG_ITEM_PUSH_RESULT: {
      unsigned __int64 player;
      int              slot;
      int              itemID;
      unsigned int     pushed = 0;
      int              displayText;
      msg->Get(player);
      msg->Get(slot);
      msg->Get(itemID);
      msg->Get(*reinterpret_cast<unsigned char *>(&pushed));
      msg->Get(displayText);
      CGGameUI::OnItemPush(player, slot, itemID, pushed, displayText);
      return 1;
    }

    case SMSG_MOUNTRESULT:
    case SMSG_DISMOUNTRESULT: {
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (player) {
        unsigned int result = 0;
        msg->Get(*reinterpret_cast<unsigned char *>(&result));
        if (msgId == SMSG_MOUNTRESULT) {
          player->HandleMountResult(result);
        } else {
          player->HandleDismountResult(result);
        }
      }
      return 1;
    }

    case SMSG_PET_NAME_INVALID:
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(221));
      return 1;

    case SMSG_SHOWTAXINODES: {
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (player) {
        player->ShowTaxiNodes(msg);
      }
      return 1;
    }

    case SMSG_TAXINODE_STATUS: {
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (player) {
        player->OnTaxiNodeStatus(msg);
      }
      return 1;
    }

    case SMSG_ACTIVATETAXIREPLY: {
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (player) {
        unsigned int result;
        msg->Get(result);
        player->HandleActivateTaxiReply(result);
      }
      return 1;
    }

    case SMSG_NEW_TAXI_PATH:
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(216));
      CGPlayer_C::UpdateTaxiStatusAll();
      return 1;

    case SMSG_PLAYERBINDERROR:
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(283));
      return 1;

    case SMSG_SHOW_BANK: {
      unsigned __int64 guid;
      msg->Get(guid);
      CGBankInfo::OpenBank(guid);
      return 1;
    }

    case SMSG_BUY_BANK_SLOT_RESULT: {
      unsigned int result;
      msg->Get(result);
      if (result < 3) {
        CGGameUI::DisplayError(s_playerBankErrors[result]);
      }
      return 1;
    }

    case SMSG_FISH_NOT_HOOKED:
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(289));
      return 1;

    case SMSG_FISH_ESCAPED:
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(290));
      return 1;

    case SMSG_LEVELUP_INFO: {
      int level;
      int healthDelta;
      int manaDelta;
      int points[2];
      msg->Get(level);
      msg->Get(healthDelta);
      msg->Get(manaDelta);

      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (!player) {
        return 1;
      }

      unsigned int classID = player->GetUnitData()->classId;
      for (unsigned int index = 0; index < 2; ++index) {
        points[index] = CHARACTER_POINTS_PER_LEVEL[index];
        if (classID == 1 && level % LEVELS_PER_CHARACTER_POINT_BONUS[index] == 0) {
          points[index] += CHARACTER_POINTS_PER_BONUS[index];
        }
        if (CHARACTER_POINT_BONUS_PER_THRESHOLD[index]) {
          points[index] += level / CHARACTER_POINT_LEVEL_THRESHOLDS[index] * CHARACTER_POINT_BONUS_PER_THRESHOLD[index];
        }
      }

      CGTutorial::TriggerTutorial(TUTORIAL_TALENTS);
      if (level >= 4) {
        CGTutorial::TriggerTutorial(TUTORIAL_SKILLS);
      }
      FrameScript_SignalEvent(195, "%d%d%d%d%d", level, healthDelta, manaDelta, points[0], points[1]);
      return 1;
    }

    case MSG_MINIMAP_PING: {
      unsigned __int64   sender;
      NTempest::C2Vector pos;
      msg->Get(sender);
      msg->Get(pos.x);
      msg->Get(pos.y);
      CGMinimapFrame::SetPingPosition(sender, pos);
      return 1;
    }

    case SMSG_PLAYER_MACRO: {
      unsigned __int64 playerGUID;
      int              category;
      msg->Get(playerGUID);
      msg->Get(category);
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(playerGUID, __FILE__, __LINE__));
      if (player) {
        player->PlayVocalMacro(category);
      }
      return 1;
    }

    case SMSG_EXPLORATION_EXPERIENCE: {
      int areaID;
      int experience;
      msg->Get(areaID);
      msg->Get(experience);
      const AreaTableRec *area = g_areaTableDB.GetRecord(areaID);
      if (experience > 0 && area) {
        const char *name = area->m_AreaName_lang[CURRENT_LANGUAGE];
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(285), name);
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(286), name, experience);
      }
      return 1;
    }

    case MSG_RANDOM_ROLL: {
      int              min;
      int              max;
      int              result;
      unsigned __int64 sender;
      char             buf[256];
      msg->Get(min);
      msg->Get(max);
      msg->Get(result);
      msg->Get(sender);

      RandomRollInfo *info = NEW(RandomRollInfo);
      info->min = min;
      info->max = max;
      info->result = result;
      const NameCache *name = g_nameDBCache.GetRecord(sender, sender, RandomRollNameQueryCallback, info);
      if (name) {
        SStrPrintf(buf, sizeof(buf), FrameScript_GetText("RANDOM_ROLL_RESULT", -1, GENDER_NOT_APPLICABLE), name->m_name, result, min, max);
        CGChat::AddChatMessage(buf, static_cast<SLASH_COMMAND_ID>(9), 0, 0, 0, 0, 0);
        FREE(info);
      }
      return 1;
    }

    default:
      return 0;
  }
}

int OnItemEvent(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  FATALASSERT(msg);

  unsigned __int64 itemGUID;
  msg->Get(itemGUID);
  CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(itemGUID, __FILE__, __LINE__));

  if (msgId == SMSG_ITEM_TIME_UPDATE) {
    int timeLeft;
    msg->Get(timeLeft);
    if (item) {
      item->UpdateExpirationTime(timeLeft);
    } else {
      ITEMEXPIRATION *itemNode = CGPlayer_C::GetPendingItemExpirationNode(itemGUID);
      FATALASSERT(itemNode);
      itemNode->timeLeft = timeLeft;
    }
    return 1;
  }

  if (msgId == SMSG_ITEM_ENCHANT_TIME_UPDATE) {
    int timeLeft;
    int slot;
    msg->Get(slot);
    msg->Get(timeLeft);
    if (item) {
      item->UpdateEnchantmentTime(slot, timeLeft);
    } else {
      ITEMEXPIRATION *itemNode = CGPlayer_C::GetPendingItemExpirationNode(itemGUID);
      FATALASSERT(itemNode);
      FATALASSERT(slot >= 0 && slot < sizeof(itemNode->enchantmentTimeLeft) / sizeof(itemNode->enchantmentTimeLeft[0]));
      itemNode->enchantmentTimeLeft[slot] = timeLeft;
    }
    return 1;
  }

  return 0;
}

int OnNpcPetitionEvent(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }

  switch (msgId) {
    case SMSG_PETITION_SHOWLIST:
      return player->OnPetitionShowList(msg);
    case SMSG_PETITION_SHOW_SIGNATURES:
      return player->OnPetitionShowSignatures(msg);
    case SMSG_PETITION_SIGN_RESULTS:
      return player->OnSignedResults(msg);
    case SMSG_TURN_IN_PETITION_RESULTS:
      return player->OnTurnInPetitionResults(msg);
    default:
      return 0;
  }
}

int HandlePartyMemberStats(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
  unsigned __int64 guid;
  int              maxPower;
  msg->Get(guid);

  CGPartyInfo::RemoteStats *stats = CGPartyInfo::GetRemoteStats(guid);
  if (stats) {
    maxPower = stats->maxPower;
    msg->Get(stats->health);
    msg->Get(stats->maxHealth);
    unsigned int powerType = 0;
    msg->Get(*reinterpret_cast<unsigned char *>(&powerType));
    stats->powerType = static_cast<POWER_TYPE>(powerType);
    msg->Get(stats->power);
    msg->Get(stats->maxPower);
    msg->Get(stats->classID);
    msg->Get(stats->level);
    msg->Get(stats->mapID);
    msg->Get(stats->areaID);
    msg->Get(stats->pos.x);
    msg->Get(stats->pos.y);
    msg->Get(stats->pos.z);

    Script_SendUnitSignal(guid, 16);
    Script_SendUnitSignal(guid, 21);
    Script_SendUnitSignal(guid, stats->powerType + 17);
    Script_SendUnitSignal(guid, stats->powerType + 22);
    if (maxPower != stats->maxPower) {
      FrameScript_SignalEvent(209);
    }
  }
  return 1;
}

int OnVendorEvent(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }

  switch (msgId) {
    case SMSG_LIST_INVENTORY:
      return player->OnVendorInventory(msg);
    case SMSG_SELL_ITEM:
      return player->OnSellResponse(msg);
    case SMSG_BUY_ITEM:
      return player->OnBuySucceeded(msg);
    case SMSG_BUY_FAILED:
      return player->OnBuyFailed(msg);
    default:
      return 0;
  }
}

int OnFactionUpdate(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  switch (msgId) {
    case SMSG_INITIALIZE_FACTIONS:
      CGReputationInfo::OnInitializeFactions(msg);
      break;

    case SMSG_SET_FACTION_VISIBLE:
      CGReputationInfo::OnSetFactionVisible(msg);
      break;

    case SMSG_SET_FACTION_STANDING:
      CGReputationInfo::OnSetFactionStanding(msg);
      break;

    default:
      FATALASSERT(!"Unhandled faction message");
      break;
  }

  return 1;
}

int OnQuestGiverEvent(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }

  switch (msgId) {
    case SMSG_QUESTGIVER_STATUS:
      return player->OnQuestGiverStatus(msg);
    case SMSG_QUESTGIVER_QUEST_LIST:
      return player->OnQuestGiverListQuests(msg);
    case SMSG_QUESTGIVER_QUEST_DETAILS:
      return player->OnQuestGiverSendQuest(msg);
    case SMSG_QUESTGIVER_REQUEST_ITEMS:
      return player->OnQuestGiverRequestItems(msg);
    case SMSG_QUESTGIVER_OFFER_REWARD:
      return player->OnQuestGiverChooseReward(msg);
    case SMSG_QUESTGIVER_QUEST_INVALID:
      return player->OnQuestGiverInvalidQuest(msg);
    case SMSG_QUESTGIVER_QUEST_COMPLETE:
      return player->OnQuestGiverQuestComplete(msg);
    case SMSG_QUESTGIVER_QUEST_FAILED:
      return player->OnQuestGiverQuestFailed(msg);
    case SMSG_QUESTLOG_FULL:
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(134));
      return 1;
    default:
      return 0;
  }
}

static int OnQuestItemLoot(QuestCache *quest, int itemID, int quantity) {
  ItemStats *stats;

  if (!quest) {
    return 0;
  }

  int index;
  for (index = 0; index < 4; ++index) {
    if (quest->m_itemToGet[index] == itemID) {
      break;
    }
  }
  if (index == 4) {
    return 0;
  }

  stats = const_cast<ItemStats_C *>(g_itemDBCache.GetRecord(itemID, 0, 0, 0));
  if (stats) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      int killed = player->GetBag()->GetItemTypeCount(itemID, 8);
      int needed = quest->m_itemToGetQuantity[index];
      if (killed >= needed) {
        return 1;
      }

      killed += quantity;
      if (killed > needed) {
        killed = needed;
      }
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(226), stats->m_displayName[FrameScript_GetPluralIndex(needed)], killed, needed);
    }
  }
  return 1;
}

static void QuestLootQuestQueryCallback(int id, const unsigned __int64 &, void *arg, bool granted) {
  int *item = static_cast<int *>(arg);
  if (granted) {
    QuestCache *quest = const_cast<QuestCache *>(g_questDBCache.GetRecord(id, 0, 0, 0));
    OnQuestItemLoot(quest, item[0], item[1]);
  }
  delete item;
}

int OnQuestUpdate(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  unsigned __int64 monsterGUID;
  CGPlayer_C      *player;
  int              monsterID = 0;
  int              quantity;
  int              questID = 0;
  int              numKilled;
  int              itemID;
  int              numNeeded;

  if (msgId != SMSG_QUESTUPDATE_ADD_ITEM) {
    msg->Get(questID);
  }

  QuestCache *quest = const_cast<QuestCache *>(g_questDBCache.GetRecord(questID, 0, 0, 0));
  if (!quest) {
    ConsolePrintf("Unkown questID (%d) in QuestUpdate", questID);
  }

  switch (msgId) {
    case SMSG_QUESTUPDATE_FAILED:
      if (quest) {
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(125), quest->m_logTitle);
      }
      return 1;

    case SMSG_QUESTUPDATE_COMPLETE:
      if (quest) {
        if (quest->m_areaDescription[0]) {
          CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(222), quest->m_areaDescription);
        } else {
          CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(223));
        }
      }
      return 1;

    case SMSG_QUESTUPDATE_ADD_KILL:
      msg->Get(monsterID);
      msg->Get(quantity);
      msg->Get(numNeeded);
      msg->Get(monsterGUID);

      if (monsterID < 0) {
        int index;
        for (index = 0; index < 4; ++index) {
          if (quest->m_monsterToKill[index] == monsterID) {
            break;
          }
        }

        if (index < 4) {
          CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(225), quest->m_getDescription[index], quantity, numNeeded);
          return 1;
        }

        GameObjectStats_C *stats = const_cast<GameObjectStats_C *>(g_gameObjectDBCache.GetRecord(monsterID & 0x7FFFFFFF, 0, 0, 0));
        if (stats) {
          CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(225), stats->m_name[FrameScript_GetPluralIndex(numNeeded)], quantity, numNeeded);
        }
        return 1;
      }

      {
        CGObject_C *victim = ClntObjMgrObjectPtr(monsterGUID, __FILE__, __LINE__);
        if (victim && victim->IsA(TYPE_UNIT)) {
          CGUnit_C *victimPtr = static_cast<CGUnit_C *>(victim);
          if (victimPtr->m_stats) {
            FATALASSERT(victimPtr->GetEntryID() == monsterID);
            victimPtr->SaveQuestAddItemMessage(quantity, numNeeded);
            return 1;
          }
        }
      }

      {
        CreatureStats_C *stats = const_cast<CreatureStats_C *>(g_creatureDBCache.GetRecord(monsterID, 0, 0, 0));
        if (stats) {
          CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(224), stats->m_name[FrameScript_GetPluralIndex(numNeeded)], quantity, numNeeded);
        }
      }
      return 1;

    case SMSG_QUESTUPDATE_ADD_ITEM: {
      msg->Get(itemID);
      msg->Get(numKilled);
      player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (!player) {
        return 1;
      }

      for (int index = 0; index < 16; ++index) {
        const CQuestLogData *questLog = player->GetQuestLogData(index);
        if (questLog->m_questID > 0) {
          int *item = static_cast<int *>(ALLOC(2 * sizeof(int)));
          item[0] = itemID;
          item[1] = numKilled;
          QuestCache *itemQuest = const_cast<QuestCache *>(
              g_questDBCache.GetRecord(questLog->m_questID, 0, reinterpret_cast<DBCACHECALLBACKPROC>(QuestLootQuestQueryCallback), item)
          );
          if (itemQuest) {
            delete item;
            if (OnQuestItemLoot(itemQuest, itemID, numKilled)) {
              break;
            }
          }
        }
      }
      return 1;
    }

    default:
      return 0;
  }
}

int OnQuestConfirm(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  char             questTitle[1024];
  unsigned __int64 initiatedBy;
  int              questID;

  if (msgId == SMSG_QUEST_CONFIRM_ACCEPT) {
    msg->Get(questID);
    msg->GetString(questTitle, sizeof(questTitle));
    msg->Get(initiatedBy);
    CGQuestInfo::ConfirmAcceptQuest(questID, questTitle, initiatedBy);
  }
  return 1;
}

int OnTrainerEvent(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  unsigned __int64 activePlayer = ClntObjMgrGetActivePlayer();
  if (!activePlayer) {
    return 0;
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(activePlayer, __FILE__, __LINE__));
  if (!player) {
    return 0;
  }

  if (msgId == SMSG_TRAINER_LIST) {
    return player->OnTrainerList(msg);
  }
  if (msgId == SMSG_TRAINER_BUY_FAILED) {
    unsigned __int64 trainer;
    int              reason;
    int              spellID;
    msg->Get(trainer);
    msg->Get(spellID);
    msg->Get(reason);

    if (reason == 1) {
      ConsolePrintf("Not enough money for trainer service %d", spellID);
    } else if (reason == 2) {
      ConsolePrintf("Not enough skill points for trainer service %d", spellID);
    } else if (!reason) {
      ConsolePrintf("Trainer service %d unavailable", spellID);
    }
    return 1;
  }
  return 0;
}

int OnLootEvent(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  CGPlayer_C *playerPtr = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(playerPtr);

  switch (msgId) {
    case SMSG_LOOT_RESPONSE:
      return playerPtr->OnLootResponse(eventTime, msg);
    case SMSG_LOOT_RELEASE_RESPONSE:
      return playerPtr->OnLootReleaseResponse(msg);
    case SMSG_LOOT_REMOVED:
      return playerPtr->OnLootRemoved(msg);
    case SMSG_LOOT_MONEY_NOTIFY:
      return playerPtr->OnLootMoneyNotify(msg);
    case SMSG_LOOT_ITEM_NOTIFY:
      return playerPtr->OnLootItemNotify(msg);
    case SMSG_LOOT_CLEAR_MONEY:
      return playerPtr->OnLootClearMoney(msg);
    case MSG_SPLIT_MONEY:
      return playerPtr->OnSplitMoneyNotify(msg);
    default:
      return 0;
  }
}

int BootMeHandler(const char *command, const char *arguments) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_BOOTME));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

int OnLearnedSpell(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  short          slot;
  unsigned short spell;

  msg->Get(spell);
  msg->Get(slot);

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->AddKnownSpell(spell, slot, 1, 1);
  } else {
    InitialSpellStruct *initialSpell = s_initialSpells.New();
    initialSpell->spellID = spell;
    initialSpell->slot = slot;
  }

  return 1;
}

int OnSupercededSpell(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  unsigned short oldSpell;
  unsigned short newSpell;

  msg->Get(oldSpell);
  msg->Get(newSpell);

  CGActionBar::ReplaceSpell(oldSpell, newSpell);
  CGSpellBook::ReplaceSpell(oldSpell, newSpell);

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->DelKnownSpell(oldSpell);
    player->AddKnownSpell(newSpell, 0, 0, 0);
  } else {
    unsigned int index;
    for (index = 0; index < s_initialSpells.Count(); ++index) {
      if (s_initialSpells[index].spellID == oldSpell) {
        s_initialSpells[index].spellID = newSpell;
        break;
      }
    }

    if (index == s_initialSpells.Count()) {
      InitialSpellStruct *initialSpell = s_initialSpells.New();
      initialSpell->spellID = newSpell;
      initialSpell->slot = 0;
    }
  }

  return 1;
}

int OnInitialSpells(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  int            recoveryTime;
  unsigned int   onHold;
  int            categoryRecoveryTime;
  unsigned short spellID;
  unsigned short itemID;
  unsigned short category;
  unsigned short count;
  unsigned int   initial;
  unsigned short index;

  msg->Get(*reinterpret_cast<unsigned char *>(&initial));
  msg->Get(count);
  s_initialSpells.SetCount(count);

  for (index = 0; index < count; ++index) {
    msg->Get(s_initialSpells[index].spellID);
    msg->Get(s_initialSpells[index].slot);
  }

  msg->Get(count);
  for (index = 0; index < count; ++index) {
    msg->Get(spellID);
    msg->Get(itemID);
    msg->Get(category);
    msg->Get(recoveryTime);
    msg->Get(categoryRecoveryTime);
    onHold = categoryRecoveryTime < 0;
    categoryRecoveryTime &= ~0x80000000;
    Spell_C_SetCooldownLeft(spellID, itemID, category, recoveryTime, categoryRecoveryTime, onHold, 0, 0);
  }

  return 1;
}

int OnActionButtons(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
  for (unsigned int index = 0; index < 120; ++index) {
    msg->Get(s_initialButtons[index]);
  }

  return 1;
}

int OnPetSpells(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
  unsigned __int64 petGUID;
  int              spellDuration;
  unsigned int     petMode;
  unsigned long    timelimit = 0;
  int              categoryDuration;
  unsigned short   category;
  unsigned int     onHold;
  unsigned int     count;
  unsigned int     index;

  CGPetInfo::ClearActions();
  msg->Get(petGUID);

  if (petGUID) {
    msg->Get(timelimit);
    msg->Get(petMode);
    CGPetInfo::SetPetModeAndOrders(petMode);

    for (index = 0; index < 10; ++index) {
      PetAction action(0);
      msg->Get(action);
      CGPetInfo::SetAction(index, action, 0);
    }

    msg->Get(*reinterpret_cast<unsigned char *>(&count));
    CGSpellBook::ClearPetSpells();
    for (index = 0; index < count; ++index) {
      unsigned short spellID;
      msg->Get(spellID);
      CGSpellBook::AddPetSpell(spellID);
    }

    msg->Get(*reinterpret_cast<unsigned char *>(&count));
    for (index = 0; index < count; ++index) {
      unsigned short spellID;
      msg->Get(spellID);
      msg->Get(category);
      msg->Get(spellDuration);
      msg->Get(categoryDuration);
      onHold = categoryDuration < 0;
      categoryDuration &= ~0x80000000;
      Spell_C_SetCooldownLeft(spellID, 0, category, spellDuration, categoryDuration, onHold, 1, 0);
    }

    CGSpellBook::SetKnowsPetSpells();
  } else {
    CGSpellBook::ClearPetSpells();
  }

  CGSpellBook::UpdateSpells();
  CGPetInfo::SetPet(petGUID, timelimit);
  CGClassTrainer::RefreshList();
  return 1;
}

int OnPlayEmote(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
  unsigned __int64 guid;
  int              emoteID;

  msg->Get(emoteID);
  msg->Get(guid);

  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (unit && unit->GetUnitData()->standState != 3) {
    unit->PlayEmoteAnimation(emoteID, 0);
  }

  return 1;
}

int OnGroupInvite(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  char name[48];
  msg->GetString(name, sizeof(name));
  CGGameUI::OpenPartyInvite(name);
  CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(52), name);
  return 1;
}

int OnGroupCancel(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  char string[256];
  char name[48];
  msg->GetString(name, sizeof(name));
  SStrPrintf(string, sizeof(string), "%s cancels the group invitation.", name);
  ConsoleWrite(string, DEFAULT_COLOR);
  CGGameUI::CancelPartyInvite();
  return 1;
}

int OnGroupDecline(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  char name[48];
  msg->GetString(name, sizeof(name));
  CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(60), name);
  return 1;
}

int OnGroupNewLeader(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  char name[48];
  msg->GetString(name, sizeof(name));

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player && SStrCmpI(player->GetUnitName(), name, sizeof(name))) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(55), name);
  } else {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(56));
  }

  return 1;
}

int OnGroupUninvite(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  CGPartyInfo::RemoveAll();
  CGPartyInfo::SetLeader(0);
  CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(62));
  return 1;
}

int OnGroupDestroy(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  CGPartyInfo::RemoveAll();
  CGPartyInfo::SetLeader(0);
  CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(59));
  return 1;
}

int OnGroupCommandResult(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  char name[48];
  int  command;
  int  result;

  msg->Get(command);
  msg->GetString(name, sizeof(name));
  msg->Get(result);

  if (result) {
    switch (result) {
      case 1:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(63), name);
        break;
      case 2:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(65), name);
        break;
      case 3:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(66));
        break;
      case 4:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(53), name);
        break;
      case 5:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(64));
        break;
      case 6:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(67));
        break;
      case 7:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(229));
        break;
      case 8:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(288), name);
        break;
    }
  } else if (command) {
    if (command == 2) {
      CGPartyInfo::RemoveAll();
      CGPartyInfo::SetLeader(0);
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(58));
    }
  } else {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(51), name);
  }

  return 1;
}

int OnGroupList(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  unsigned __int64 newMembers[5];
  char             string[32];
  unsigned __int64 oldMembers[5];
  int              wasInGroup;
  unsigned int     count;
  unsigned int     i;
  int              isLeader;
  unsigned __int64 guid;
  unsigned __int64 lootMaster;
  unsigned int     connected;
  unsigned int     lootMethod;

  msg->Get(count);
  memset(oldMembers, 0, sizeof(oldMembers));
  memset(newMembers, 0, sizeof(newMembers));
  wasInGroup = 0;
  isLeader = 0;

  for (i = 0; i < 5; ++i) {
    oldMembers[i] = CGPartyInfo::GetMember(i);
    if (oldMembers[i]) {
      wasInGroup = 1;
    }
  }

  CGPartyInfo::RemoveAll();

  for (i = 0; i < count; ++i) {
    msg->GetString(string, sizeof(string));
    ConsoleWrite(string, DEFAULT_COLOR);
    msg->Get(guid);
    msg->Get(*reinterpret_cast<unsigned char *>(&connected));

    if (guid == ClntObjMgrGetActivePlayer()) {
      if (!i) {
        CGPartyInfo::SetLeader(guid);
        isLeader = 1;
      }
      continue;
    }

    newMembers[i] = guid;
    if (wasInGroup) {
      unsigned int oldIndex;
      for (oldIndex = 0; oldIndex < 5; ++oldIndex) {
        if (oldMembers[oldIndex] == guid) {
          break;
        }
      }
      if (oldIndex == 5) {
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(61), string);
      }
    } else if (isLeader) {
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(61), string);
    }

    CGPartyInfo::AddMember(guid, connected != 0);
    if (!i) {
      CGPartyInfo::SetLeader(guid);
    }
  }

  for (i = 0; i < 5; ++i) {
    guid = oldMembers[i];
    if (!guid || guid == ClntObjMgrGetActivePlayer()) {
      continue;
    }

    unsigned int newIndex;
    for (newIndex = 0; newIndex < count; ++newIndex) {
      if (newMembers[newIndex] == guid) {
        break;
      }
    }

    if (newIndex == count) {
      const NameCache *name = g_nameDBCache.GetRecord(guid, guid, 0, 0);
      if (name) {
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(57), name->m_name);
      }
    }
  }

  if (count) {
    msg->Get(*reinterpret_cast<unsigned char *>(&lootMethod));
    msg->Get(lootMaster);
    CGPartyInfo::SetLootMethod(static_cast<LOOT_METHOD>(lootMethod), lootMaster);
  }

  return 1;
}

int OnGuildInvite(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  char guildName[96];
  char name[48];
  msg->GetString(name, sizeof(name));
  msg->GetString(guildName, sizeof(guildName));
  CGGameUI::OpenGuildInvite(name, guildName);
  CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(71), name, guildName);
  return 1;
}

int OnGuildDecline(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  char name[48];
  msg->GetString(name, sizeof(name));
  CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(77), name);
  return 1;
}

int OnGuildInfo(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  char         name[96];
  char         buf[128];
  char         temp[64];
  unsigned int year;
  unsigned int day;
  unsigned int numChars;
  unsigned int month;
  unsigned int numAccounts;

  msg->GetString(name, sizeof(name));
  msg->Get(numAccounts);
  msg->Get(numChars);
  msg->Get(year);
  msg->Get(month);
  msg->Get(day);

  SStrCopy(temp, FrameScript_GetText("GUILD_NAME_TEMPLATE", -1, GENDER_NOT_APPLICABLE), sizeof(temp));
  SStrPrintf(buf, sizeof(buf), temp, name);
  CGChat::AddChatMessage(buf, static_cast<SLASH_COMMAND_ID>(9), 0, 0, 0, 0, 0);

  SStrCopy(temp, FrameScript_GetText("GUILD_INFO_TEMPLATE", -1, GENDER_NOT_APPLICABLE), sizeof(temp));
  SStrPrintf(buf, sizeof(buf), temp, numChars, numAccounts, year, month, day);
  CGChat::AddChatMessage(buf, static_cast<SLASH_COMMAND_ID>(9), 0, 0, 0, 0, 0);

  return 1;
}

int OnGuildRoster(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  char         name[256];
  char         ranks[5][32];
  char         guildname[96];
  char         buf[128];
  char         temp[64];
  unsigned int guildRank;
  unsigned int numAccounts;
  unsigned int numChars;
  unsigned int i;

  msg->GetString(guildname, sizeof(guildname));
  msg->Get(numChars);
  msg->Get(numAccounts);

  SStrCopy(temp, FrameScript_GetText("GUILD_NAME_TEMPLATE", -1, GENDER_NOT_APPLICABLE), sizeof(temp));
  SStrPrintf(buf, sizeof(buf), temp, guildname);
  CGChat::AddChatMessage(buf, static_cast<SLASH_COMMAND_ID>(9), 0, 0, 0, 0, 0);

  for (i = 0; i < 5; ++i) {
    SStrPrintf(temp, sizeof(temp), "GUILD_RANK%d_DESC", i);
    SStrCopy(ranks[i], FrameScript_GetText(temp, -1, GENDER_NOT_APPLICABLE), sizeof(ranks[i]));
  }

  SStrCopy(temp, FrameScript_GetText("GUILD_MEMBER_TEMPLATE", -1, GENDER_NOT_APPLICABLE), sizeof(temp));
  for (i = 0; i < numChars; ++i) {
    msg->GetString(name, 0x7FFFFFFF);
    msg->Get(guildRank);
    SStrPrintf(buf, sizeof(buf), temp, name, ranks[guildRank]);
    CGChat::AddChatMessage(buf, static_cast<SLASH_COMMAND_ID>(9), 0, 0, 0, 0, 0);
  }

  SStrCopy(temp, FrameScript_GetText("GUILD_ROSTER_TEMPLATE", -1, GENDER_NOT_APPLICABLE), sizeof(temp));
  SStrPrintf(buf, sizeof(buf), temp, numChars, numAccounts);
  CGChat::AddChatMessage(buf, static_cast<SLASH_COMMAND_ID>(9), 0, 0, 0, 0, 0);
  return 1;
}

int OnGuildEmblemActivate(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
  unsigned __int64 vendor;
  msg->Get(vendor);
  CGTabardCreationFrame::Open(vendor);
  return 1;
}

int OnGuildEmblemError(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
  int error;
  msg->Get(error);
  if (static_cast<unsigned int>(error) < 7) {
    CGGameUI::DisplayError(s_tabardErrors[error]);
  }
  return 1;
}

int OnGuildEvent(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
  char            string[2][256];
  unsigned int    numStrings;
  unsigned int    event;
  GAME_ERROR_TYPE errorType;

  event = 0;
  msg->Get(*reinterpret_cast<unsigned char *>(&event));
  msg->Get(*reinterpret_cast<unsigned char *>(&numStrings));
  for (unsigned int index = 0; index < numStrings; ++index) {
    msg->GetString(string[index], sizeof(string[index]));
  }

  switch (event) {
    case 0:
      errorType = static_cast<GAME_ERROR_TYPE>(81);
      break;
    case 1:
      errorType = static_cast<GAME_ERROR_TYPE>(82);
      break;
    case 2:
      if (!string[0][0]) {
        return 1;
      }
      errorType = static_cast<GAME_ERROR_TYPE>(91);
      break;
    case 3:
      errorType = static_cast<GAME_ERROR_TYPE>(79);
      break;
    case 4:
      errorType = static_cast<GAME_ERROR_TYPE>(84);
      break;
    case 5:
      errorType = static_cast<GAME_ERROR_TYPE>(85);
      break;
    case 6:
      errorType = static_cast<GAME_ERROR_TYPE>(99);
      break;
    case 7:
      errorType = static_cast<GAME_ERROR_TYPE>(100);
      break;
    case 8:
      errorType = static_cast<GAME_ERROR_TYPE>(101);
      break;
    default:
      errorType = static_cast<GAME_ERROR_TYPE>(98);
      break;
  }

  if (numStrings == 1) {
    CGGameUI::DisplayError(errorType, string[0]);
  } else if (numStrings == 2) {
    CGGameUI::DisplayError(errorType, string[0], string[1]);
  } else {
    CGGameUI::DisplayError(errorType);
  }
  return 1;
}

int OnGuildCommandResult(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
  char name[96];
  int  result;
  int  command;

  msg->Get(command);
  msg->GetString(name, sizeof(name));
  msg->Get(result);

  if (result) {
    switch (result) {
      case 1:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(98));
        break;
      case 2:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(75));
        break;
      case 3:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(72), name);
        break;
      case 4:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(74));
        break;
      case 5:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(73), name);
        break;
      case 6:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(104));
        break;
      case 7:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(105), name);
        break;
      case 8:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(command == 2 ? 103 : 78));
        break;
      case 9:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(94));
        break;
      case 10:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(93), name);
        break;
      case 11:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(92), name);
        break;
      case 12:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(102));
        break;
    }
  } else {
    switch (command) {
      case 0:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(69), name);
        break;
      case 1:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(70), name);
        break;
      case 2:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(83), name);
        break;
      case 12:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(80), name);
        break;
    }
  }

  return 1;
}

void Player_C_RegisterGuildUpdate(unsigned int guildID) {
  s_guildIDs.Add(&guildID);
}

static void GuildCallback(int guildID, const unsigned __int64 &, void *, bool granted) {
  if (granted) {
    Player_C_RegisterGuildUpdate(guildID);
  }
}

const char *MirrorTimerToName(int timer) {
  switch (timer) {
    case 0:
      return "EXHAUSTION";
    case 1:
      return "BREATH";
    case 2:
      return "FEIGNDEATH";
    default:
      return "UNKNOWN";
  }
}

const char *MirrorTimerLabel(int timer, int spellID) {
  char            label[128];
  const SpellRec *spell = g_spellDB.GetRecord(spellID);
  if (spell) {
    return spell->m_name_lang[CURRENT_LANGUAGE];
  }

  SStrPrintf(label, sizeof(label), "%s_LABEL", MirrorTimerToName(timer));
  return FrameScript_GetText(label, -1, GENDER_NOT_APPLICABLE);
}

int OnMirrorTimerEvent(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  switch (msgId) {
    case SMSG_START_MIRROR_TIMER: {
      int          value;
      int          maxValue;
      int          scale;
      int          spellID;
      int          timer;
      unsigned int paused;
      msg->Get(timer);
      msg->Get(value);
      msg->Get(maxValue);
      msg->Get(scale);
      msg->Get(*reinterpret_cast<unsigned char *>(&paused));
      msg->Get(spellID);
      FrameScript_SignalEvent(346, "%s%d%d%d%d%s", MirrorTimerToName(timer), value, maxValue, scale, paused, MirrorTimerLabel(timer, spellID));
      break;
    }

    case SMSG_PAUSE_MIRROR_TIMER: {
      int          timer;
      unsigned int paused;
      msg->Get(timer);
      msg->Get(*reinterpret_cast<unsigned char *>(&paused));
      FrameScript_SignalEvent(347, "%s%d", MirrorTimerToName(timer), paused);
      break;
    }

    case SMSG_STOP_MIRROR_TIMER: {
      int timer;
      msg->Get(timer);
      FrameScript_SignalEvent(348, "%s", MirrorTimerToName(timer));
      break;
    }
  }

  return 1;
}

int CGPlayer_C::OnVendorInventory(CDataStore *msg) {
  unsigned __int64 vendorGuid;
  unsigned int     reason = 0xFF;
  unsigned int     count = 0;

  for (unsigned int index = 0; index < 128; ++index) {
    s_lastVendorList[index].m_muid = 0;
  }

  msg->Get(vendorGuid);
  msg->Get(*reinterpret_cast<unsigned char *>(&count));
  FATALASSERT(count <= 128);
  s_lastVendorListReceived = vendorGuid;

  if (count) {
    for (unsigned int index = 0; index < count; ++index) {
      msg->Get(s_lastVendorList[index].m_muid);
      msg->Get(s_lastVendorList[index].m_itemType);
      msg->Get(s_lastVendorList[index].m_itemDisplayID);
      msg->Get(s_lastVendorList[index].m_quantity);
      msg->Get(s_lastVendorList[index].m_price);
      msg->Get(s_lastVendorList[index].m_durability);
      msg->Get(s_lastVendorList[index].m_stackCount);
    }
  } else {
    msg->Get(*reinterpret_cast<unsigned char *>(&reason));
    switch (reason) {
      case 0:
        ConsoleWrite("Vendor has no inventory", DEFAULT_COLOR);
        break;
      case 1:
        ConsoleWrite("I don't think he likes you very much", DEFAULT_COLOR);
        break;
      case 2:
        ConsoleWrite("You are too far away", DEFAULT_COLOR);
        break;
      case 3:
        ConsoleWrite("Vendor is dead", DEFAULT_COLOR);
        break;
      case 4:
        ConsoleWrite("You can't shop while dead.", DEFAULT_COLOR);
        break;
    }
  }

  if (count || !reason) {
    CGMerchantInfo::SetMerchant(vendorGuid, s_lastVendorList, count);
  }
  return 1;
}

int CGPlayer_C::OnQuestGiverListQuests(CDataStore *msg) {
  char                initialText[8][64];
  char                greetText[256];
  unsigned __int64    questGiverGuid;
  QUESTGIVEREMOTENODE node = {0, 0};
  unsigned int        count = 0;

  msg->Get(questGiverGuid);
  msg->GetString(greetText, 0x7FFFFFFF);
  msg->Get(node.emoteID);
  msg->Get(node.delay);
  msg->Get(*reinterpret_cast<unsigned char *>(&count));

  memset(s_lastQuestList, 0, sizeof(s_lastQuestList));
  memset(s_lastQuestListType, 0, sizeof(s_lastQuestListType));
  s_lastQuestGiverListRevieved = questGiverGuid;
  memset(s_lastQuestLevel, 0, sizeof(s_lastQuestLevel));

  CGQuestInfo::SetState(questGiverGuid, QUEST_GREETING, greetText, 0);
  for (unsigned int index = 0; index < count; ++index) {
    msg->Get(s_lastQuestList[index]);
    msg->Get(s_lastQuestListType[index]);
    msg->Get(s_lastQuestLevel[index]);
    msg->GetString(initialText[index], 64);

    if (s_lastQuestListType[index] == 3 || s_lastQuestListType[index] == 4) {
      CGQuestInfo::AddQuestInProgress(s_lastQuestList[index], initialText[index], s_lastQuestLevel[index]);
    } else {
      CGQuestInfo::AddQuest(s_lastQuestList[index], initialText[index], s_lastQuestLevel[index], s_lastQuestListType[index] == 0);
    }
  }

  CGObject_C *object = ClntObjMgrObjectPtr(questGiverGuid, __FILE__, __LINE__);
  if (object && (object->GetType() & TYPE_UNIT)) {
    static_cast<CGUnit_C *>(object)->SetEmoteQueue(&node, 1);
  }
  CGQuestInfo::EndQuestList();
  return 1;
}

int CGPlayer_C::OnQuestGiverInvalidQuest(CDataStore *msg) {
  int failureReason;
  msg->Get(failureReason);

  if (failureReason == 1) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(128));
  } else if (failureReason == 15) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(129));
  } else {
    ConsoleWrite("Invalid quest!", DEFAULT_COLOR);
  }

  CGQuestInfo::QuestGiverFinished();
  return 1;
}

int CGPlayer_C::OnQuestGiverSendQuest(CDataStore *msg) {
  char             questText[1024];
  char             logDescription[512];
  char             questTitle[64];
  int              chooseRewardQty[6];
  int              rewardItemQty[4];
  int              chooseRewardDispID[6];
  int              chooseReward[6];
  int              rewardItem[4];
  int              rewardItemDispID[4];
  int              rewardMoney;
  int              questID;
  int              autoLaunched;
  unsigned __int64 questGiverGuid;
  int              rewardItemCount;
  int              chooseRewardCount;
  int              numEmotes;

  msg->Get(questGiverGuid);
  msg->Get(questID);
  msg->GetString(questTitle, 0x7FFFFFFF);
  msg->GetString(questText, 0x7FFFFFFF);
  msg->GetString(logDescription, 0x7FFFFFFF);
  msg->Get(autoLaunched);
  msg->Get(chooseRewardCount);

  memset(chooseReward, 0, sizeof(chooseReward));
  memset(chooseRewardDispID, 0, sizeof(chooseRewardDispID));
  for (int index = 0; index < chooseRewardCount; ++index) {
    msg->Get(chooseReward[index]);
    msg->Get(chooseRewardQty[index]);
    msg->Get(chooseRewardDispID[index]);
  }

  msg->Get(rewardItemCount);
  memset(rewardItemDispID, 0, sizeof(rewardItemDispID));
  memset(rewardItem, 0, sizeof(rewardItem));
  for (index = 0; index < rewardItemCount; ++index) {
    msg->Get(rewardItem[index]);
    msg->Get(rewardItemQty[index]);
    msg->Get(rewardItemDispID[index]);
  }

  msg->Get(rewardMoney);
  msg->Get(numEmotes);
  TSStackArray<QUESTGIVEREMOTENODE> emotes(_alloca(numEmotes * sizeof(QUESTGIVEREMOTENODE)), numEmotes, numEmotes);
  for (index = 0; index < numEmotes; ++index) {
    msg->Get(emotes[index].delay);
    msg->Get(emotes[index].emoteID);
  }

  CGObject_C *object = ClntObjMgrObjectPtr(questGiverGuid, __FILE__, __LINE__);
  if (object && (object->GetType() & TYPE_UNIT)) {
    static_cast<CGUnit_C *>(object)->SetEmoteQueue(emotes);
  }

  CGQuestInfo::SetState(questGiverGuid, QUEST_DETAIL, questText, questID);
  CGQuestInfo::SetLogDescription(logDescription);
  CGQuestInfo::AddReward(
      questTitle, chooseReward, chooseRewardDispID, chooseRewardQty, chooseRewardCount, rewardItem, rewardItemDispID, rewardItemQty, rewardItemCount,
      rewardMoney, autoLaunched
  );
  return 1;
}

int CGPlayer_C::OnQuestGiverRequestItems(CDataStore *msg) {
  char                questText[1024];
  char                questTitle[64];
  int                 itemDispID[6];
  int                 itemAmounts[6];
  int                 items[6];
  int                 hasitems;
  int                 hasfaction;
  int                 questID;
  int                 autoLaunched;
  int                 maskmatch;
  QUESTGIVEREMOTENODE node = {0, 0};
  unsigned __int64    questGiverGuid;
  int                 itemCount;

  msg->Get(questGiverGuid);
  msg->Get(questID);
  msg->GetString(questTitle, 0x7FFFFFFF);
  msg->GetString(questText, 0x7FFFFFFF);
  msg->Get(node.emoteID);
  msg->Get(node.delay);
  msg->Get(autoLaunched);
  msg->Get(itemCount);

  memset(items, 0, sizeof(items));
  for (int index = 0; index < itemCount; ++index) {
    msg->Get(items[index]);
    msg->Get(itemAmounts[index]);
    msg->Get(itemDispID[index]);
  }

  msg->Get(hasitems);
  msg->Get(hasfaction);
  msg->Get(maskmatch);

  CGQuestInfo::SetState(questGiverGuid, QUEST_PROGRESS, questText, questID);
  CGQuestInfo::AddItemRequest(questTitle, items, itemAmounts, itemDispID, itemCount, hasitems && hasfaction && maskmatch, autoLaunched);

  CGObject_C *object = ClntObjMgrObjectPtr(questGiverGuid, __FILE__, __LINE__);
  if (object && (object->GetType() & TYPE_UNIT)) {
    static_cast<CGUnit_C *>(object)->SetEmoteQueue(&node, 1);
  }
  return 1;
}

int CGPlayer_C::OnQuestGiverChooseReward(CDataStore *msg) {
  char             questText[1024];
  char             questTitle[64];
  int              chooseRewardQty[6];
  int              rewardItemQty[4];
  int              chooseRewardDispID[6];
  int              chooseReward[6];
  int              rewardItem[4];
  int              rewardItemDispID[4];
  int              rewardMoney;
  int              questID;
  int              autoLaunched;
  unsigned __int64 questGiverGuid;
  int              rewardItemCount;
  int              chooseRewardCount;
  int              emoteCount;

  msg->Get(questGiverGuid);
  msg->Get(questID);
  msg->GetString(questTitle, 0x7FFFFFFF);
  msg->GetString(questText, 0x7FFFFFFF);
  msg->Get(autoLaunched);
  msg->Get(emoteCount);

  TSStackArray<QUESTGIVEREMOTENODE> emotes(_alloca(emoteCount * sizeof(QUESTGIVEREMOTENODE)), emoteCount, emoteCount);
  for (int index = 0; index < emoteCount; ++index) {
    msg->Get(emotes[index].delay);
    msg->Get(emotes[index].emoteID);
  }

  CGObject_C *object = ClntObjMgrObjectPtr(questGiverGuid, __FILE__, __LINE__);
  if (object && (object->GetType() & TYPE_UNIT)) {
    static_cast<CGUnit_C *>(object)->SetEmoteQueue(emotes);
  }

  msg->Get(chooseRewardCount);
  memset(chooseReward, 0, sizeof(chooseReward));
  memset(chooseRewardDispID, 0, sizeof(chooseRewardDispID));
  for (index = 0; index < chooseRewardCount; ++index) {
    msg->Get(chooseReward[index]);
    msg->Get(chooseRewardQty[index]);
    msg->Get(chooseRewardDispID[index]);
  }

  msg->Get(rewardItemCount);
  memset(rewardItem, 0, sizeof(rewardItem));
  memset(rewardItemDispID, 0, sizeof(rewardItemDispID));
  for (index = 0; index < rewardItemCount; ++index) {
    msg->Get(rewardItem[index]);
    msg->Get(rewardItemQty[index]);
    msg->Get(rewardItemDispID[index]);
  }

  msg->Get(rewardMoney);
  CGQuestInfo::SetState(questGiverGuid, QUEST_REWARD, questText, questID);
  CGQuestInfo::AddReward(
      questTitle, chooseReward, chooseRewardDispID, chooseRewardQty, chooseRewardCount, rewardItem, rewardItemDispID, rewardItemQty, rewardItemCount,
      rewardMoney, autoLaunched
  );
  return 1;
}

void QuestCompleteCallback(int id, const unsigned __int64 &, void *, bool granted) {
  if (!granted) {
    return;
  }

  QuestCache *quest = const_cast<QuestCache *>(g_questDBCache.GetRecord(id, 0, 0, 0));
  if (!quest) {
    return;
  }

  if (!quest->m_rewardNextQuest) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(124), quest->m_logTitle);
  }
  if (!quest->m_questType) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      player->UpdateQuestStatusAll();
    }
  }
}

void QuestRewardItemCallback(int id, const unsigned __int64 &, void *, bool granted) {
  if (!granted) {
    return;
  }

  const ItemStats_C *item = g_itemDBCache.GetRecord(id, 0, 0, 0);
  if (item) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(131), item->m_displayName[0]);
  }
}

int CGPlayer_C::OnQuestGiverQuestComplete(CDataStore *msg) {
  char        buf[128];
  char        coinBuf[3][64];
  int         itemsQty[5];
  char        coinName[32];
  int         status;
  int         items[5];
  int         coins[3];
  int         money;
  QuestCache *quest;
  int         questID;
  int         xp;
  int         itemCount;

  msg->Get(questID);
  msg->Get(status);
  msg->Get(xp);
  msg->Get(money);
  msg->Get(itemCount);

  memset(items, 0, sizeof(items));
  for (int index = 0; index < itemCount; ++index) {
    msg->Get(items[index]);
    msg->Get(itemsQty[index]);
  }

  quest = const_cast<QuestCache *>(g_questDBCache.GetRecord(questID, 0, reinterpret_cast<DBCACHECALLBACKPROC>(QuestCompleteCallback), 0));
  if (quest && !quest->m_rewardNextQuest) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(124), quest->m_logTitle);
  }

  if (xp) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(130), xp);
    StoreXPGain(xp);
  }

  if (money) {
    CurrencyBreakdown(money, coins);
    for (int coin = 0; coin < 3; ++coin) {
      SStrCopy(coinName, FrameScript_GetText(coinToken[coin], -1, GENDER_NOT_APPLICABLE), sizeof(coinName));
      SStrPrintf(coinBuf[coin], sizeof(coinBuf[coin]), "%d %s", coins[coin], coinName);
    }

    const char *copper = coins[0] ? coinBuf[0] : "";
    const char *silver = coins[1] ? coinBuf[1] : "";
    const char *gold = coins[2] ? coinBuf[2] : "";
    const char *copperSeparator = coins[0] && (coins[1] || coins[2]) ? ", " : "";
    const char *silverSeparator = coins[2] && (coins[1] || coins[0]) ? ", " : "";
    SStrPrintf(buf, sizeof(buf), "%s%s%s%s%s", gold, silverSeparator, silver, copperSeparator, copper);
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(132), buf);
  }

  for (index = 0; index < itemCount; ++index) {
    const ItemStats_C *item = g_itemDBCache.GetRecord(items[index], GetGUID(), reinterpret_cast<DBCACHECALLBACKPROC>(QuestRewardItemCallback), 0);
    if (item) {
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(131), item->m_displayName[0]);
    }
  }

  if (CGQuestInfo::GetLastChosenItem()) {
    const ItemStats_C *item =
        g_itemDBCache.GetRecord(CGQuestInfo::GetLastChosenItem(), GetGUID(), reinterpret_cast<DBCACHECALLBACKPROC>(QuestRewardItemCallback), 0);
    if (item) {
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(131), item->m_displayName[0]);
    }
    CGQuestInfo::ClearLastChosenItem();
  }

  CGQuestInfo::QuestGiverFinished();
  if (quest && !quest->m_questType) {
    UpdateQuestStatusAll();
  }
  return 1;
}

void QuestFailedCallback(int id, const unsigned __int64 &, void *, bool granted) {
  if (!granted) {
    return;
  }

  QuestCache *quest = const_cast<QuestCache *>(g_questDBCache.GetRecord(id, 0, 0, 0));
  if (!quest) {
    return;
  }

  if (s_questFailedReason == 4 || s_questFailedReason == 48) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(126), quest->m_logTitle);
    CGGameUI::DisplayError(GAME_ERROR_NONE);
  } else if (s_questFailedReason == 16) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(127), quest->m_logTitle);
  } else {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(125), quest->m_logTitle);
  }
}

int CGPlayer_C::OnQuestGiverQuestFailed(CDataStore *msg) {
  int questFailedID;
  msg->Get(questFailedID);
  msg->Get(s_questFailedReason);

  QuestCache *quest = const_cast<QuestCache *>(
      g_questDBCache.GetRecord(questFailedID, CGQuestInfo::GetQuestGiver(), reinterpret_cast<DBCACHECALLBACKPROC>(QuestFailedCallback), 0)
  );
  if (quest) {
    if (s_questFailedReason == 4 || s_questFailedReason == 48) {
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(126), quest->m_logTitle);
      CGGameUI::DisplayError(GAME_ERROR_NONE);
    } else if (s_questFailedReason == 16) {
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(127), quest->m_logTitle);
    } else {
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(125), quest->m_logTitle);
    }
  }

  CGQuestInfo::QuestGiverFinished();
  return 1;
}

int CGPlayer_C::OnQuestGiverStatus(CDataStore *msg) {
  unsigned __int64 questGiverGuid;
  int              hasquest;
  msg->Get(questGiverGuid);
  msg->Get(hasquest);

  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(questGiverGuid, __FILE__, __LINE__));
  if (unit && (unit->GetUnitData()->npcFlags & 2)) {
    FATALASSERT(unit->IsA(TYPE_UNIT));
    unit->UpdateInteractIcon(static_cast<QUEST_GIVER_STATUS>(hasquest));
  }
  return 1;
}

int CGPlayer_C::OnTrainerList(CDataStore *msg) {
  char             greeting[512];
  int             *reqAbilities[3];
  unsigned char   *pointCosts[2];
  unsigned __int64 trainerGUID;
  int              trainerType;
  unsigned int     count;

  msg->Get(trainerGUID);
  msg->Get(trainerType);
  msg->Get(count);

  TSStackArray<unsigned char> usable(_alloca(count * sizeof(unsigned char)), count, count);
  TSStackArray<int>           reqAbility2(_alloca(count * sizeof(int)), count, count);
  TSStackArray<unsigned int>  moneyCost(_alloca(count * sizeof(unsigned int)), count, count);
  TSStackArray<int>           reqAbility0(_alloca(count * sizeof(int)), count, count);
  TSStackArray<unsigned char> pointCost0(_alloca(count * sizeof(unsigned char)), count, count);
  TSStackArray<unsigned int>  reqSkillStep(_alloca(count * sizeof(unsigned int)), count, count);
  TSStackArray<unsigned char> pointCost1(_alloca(count * sizeof(unsigned char)), count, count);
  TSStackArray<int>           reqAbility1(_alloca(count * sizeof(int)), count, count);
  TSStackArray<unsigned char> reqLevel(_alloca(count * sizeof(unsigned char)), count, count);
  TSStackArray<int>           spellID(_alloca(count * sizeof(int)), count, count);
  TSStackArray<unsigned int>  reqSkillRank(_alloca(count * sizeof(unsigned int)), count, count);
  TSStackArray<unsigned int>  reqSkillLine(_alloca(count * sizeof(unsigned int)), count, count);

  for (unsigned int index = 0; index < count; ++index) {
    msg->Get(spellID[index]);
    msg->Get(usable[index]);
    msg->Get(moneyCost[index]);
    msg->Get(pointCost0[index]);
    msg->Get(pointCost1[index]);
    msg->Get(reqLevel[index]);
    msg->Get(reqSkillLine[index]);
    msg->Get(reqSkillRank[index]);
    msg->Get(reqSkillStep[index]);
    msg->Get(reqAbility0[index]);
    msg->Get(reqAbility1[index]);
    msg->Get(reqAbility2[index]);
  }

  msg->GetString(greeting, 512);
  pointCosts[0] = pointCost0.Ptr();
  pointCosts[1] = pointCost1.Ptr();
  reqAbilities[0] = reqAbility0.Ptr();
  reqAbilities[1] = reqAbility1.Ptr();
  reqAbilities[2] = reqAbility2.Ptr();

  if (trainerGUID == CGClassTrainer::GetTrainer()) {
    CGClassTrainer::SetTrainer(trainerGUID, static_cast<TRAINER_TYPE>(trainerType));
    CGClassTrainer::AddServices(
        count, spellID.Ptr(), moneyCost.Ptr(), pointCosts, reqLevel.Ptr(), reqSkillLine.Ptr(), reqSkillRank.Ptr(), reqSkillStep.Ptr(), reqAbilities,
        usable.Ptr(), greeting
    );
  }
  return 1;
}

int CGPlayer_C::OnBuyFailed(CDataStore *msg) {
  char             buf[256];
  unsigned __int64 vendorGUID;
  unsigned int     muid;
  unsigned int     quantity = 0;
  unsigned int     reason = 0;

  msg->Get(vendorGUID);
  msg->Get(muid);
  msg->Get(*reinterpret_cast<unsigned char *>(&quantity));
  msg->Get(*reinterpret_cast<unsigned char *>(&reason));

  if (reason == 1 && vendorGUID == s_lastVendorListReceived) {
    for (unsigned int index = 0; index < 128; ++index) {
      if (s_lastVendorList[index].m_muid == muid) {
        s_lastVendorList[index].m_quantity = 0;
        CGMerchantInfo::UpdateItemQuantity(vendorGUID, muid, 0);
      }
    }
  }

  const char *error;
  switch (reason) {
    case 1:
    case 7:
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(30));
      error = "Sold out.";
      break;
    case 2:
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(32));
      error = "Not enough money";
      break;
    case 3:
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(17));
      error = "Item creation failed.";
      break;
    case 4:
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(29));
      error = "Merchant doesn't like you.";
      break;
    case 5:
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(31));
      error = "You are too far away.";
      break;
    case 6:
      error = "Your inventory is full.";
      break;
    case 8:
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(12));
      error = "You already have the maximum number allowed.";
      break;
    case 11:
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(17));
      error = "Vendor error.";
      break;
    default:
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(17));
      error = "";
      break;
  }

  SStrPrintf(buf, sizeof(buf), "Buy %d of %d failed: %s", quantity, muid, error);
  ConsoleWrite(buf, DEFAULT_COLOR);
  return 1;
}

int CGPlayer_C::OnBuySucceeded(CDataStore *msg) {
  unsigned __int64 vendorGUID;
  int              newQuantity;
  unsigned long    muid;
  msg->Get(vendorGUID);
  msg->Get(muid);
  msg->Get(newQuantity);

  if (vendorGUID == s_lastVendorListReceived) {
    for (unsigned int index = 0; index < 128; ++index) {
      if (s_lastVendorList[index].m_muid == muid) {
        s_lastVendorList[index].m_quantity = newQuantity;
      }
    }
    CGMerchantInfo::UpdateItemQuantity(vendorGUID, muid, newQuantity);
  }
  return 1;
}

int CGPlayer_C::OnSellResponse(CDataStore *msg) {
  unsigned __int64 vendorGUID;
  unsigned __int64 itemGUID;
  unsigned int     reason = 0;
  msg->Get(vendorGUID);
  msg->Get(itemGUID);
  msg->Get(*reinterpret_cast<unsigned char *>(&reason));

  if (reason) {
    switch (reason) {
      case 1:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(17));
        break;
      case 2:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(28));
        break;
      case 3:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(29));
        break;
      case 4:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(22));
        break;
    }
    if (itemGUID) {
      CGGameUI::UnlockItem(itemGUID);
    }
  }
  return 1;
}

static int GuildIDUpdateHandler(unsigned __int64 guid, unsigned int offset, unsigned int bytes, const void *prevValue, void *param) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (player) {
    player->OnGuildChanged();
  }
  return 1;
}

static int DuelTeamUpdateHandler(unsigned __int64 guid, unsigned int offset, unsigned int bytes, const void *prevValue, void *param) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (player) {
    player->UpdatePlayerName();
  }
  FrameScript_SignalEvent(27);
  return 1;
}

static int OnUpdateInventoryComponent(unsigned __int64 guid, unsigned int offset, unsigned int bytes, const void *prevValue, void *param) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (player) {
    unsigned int     slot = offset >> 3;
    unsigned __int64 currGuid = player->GetBag()->GetItem(slot);
    if (*static_cast<const unsigned __int64 *>(prevValue) != currGuid) {
      CGGameUI::UnlockItem(currGuid);
    }
    FrameScript_SignalEvent(326);
  }
  return 1;
}

static int OnUpdateMoney(unsigned __int64, unsigned int, unsigned int, const void *, void *) {
  CGActionBar::UpdateUsable();
  return 1;
}

static void QuestAcceptedCallback(int id, const unsigned __int64&, void*, unsigned char granted) {
  if (granted) {
    unsigned __int64 noGuid = 0;
    const QuestCache *quest = g_questDBCache.GetRecord(id, noGuid, 0, 0);
    if (quest) {
      CGGameUI::DisplayError(GERR_QUEST_ACCEPTED_S, quest->m_logTitle);
    }
  }
}

static int OnUpdateQuest(unsigned __int64 guid, unsigned int offset, unsigned int bytes, const void *prevValue, void *param) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (player) {
    player->UpdateQuestStatusAll();
  }
  return 1;
}

static int OnUpdateShapeshiftForm(unsigned __int64 guid, unsigned int offset, unsigned int bytes, const void* prevValue, void* param) {
  CGSpellBook::UpdateSelection();
  CGActionBar::UpdateSelection();
  CGActionBar::UpdateBonusBar();
  return 1;
}

static int OnUpdatePlayerFlags(unsigned __int64 guid, unsigned int offset, unsigned int bytes, const void *prevValue, void *param) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (player) {
    player->OnFlagChanged(*static_cast<const unsigned char *>(prevValue));
  }
  return 1;
}

static void GuildTimestampChanged(int id, const unsigned __int64 &guid, void *arg, bool granted) {
  if (granted) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
    if (player) {
      player->OnGuildChanged();
    }
  }
}

static int OnUpdateGuild(unsigned __int64 guid, unsigned int offset, unsigned int bytes, const void *prevValue, void *param) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (player) {
    g_guildInfoCache.Invalidate(player->GetGuildID());
    g_guildInfoCache.GetRecord(player->GetGuildID(), guid, GuildTimestampChanged, 0);
  }
  return 1;
}

static int CharmChangeHandler(unsigned __int64, unsigned int, unsigned int, const void *, void *) {
  CGActionBar::UpdateSelection();
  return 1;
}

static int PetChangeHandler(unsigned __int64 unit, unsigned int, unsigned int, const void *oldValue, void *) {
  FATALASSERT(unit == ClntObjMgrGetActivePlayer());
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  if (player) {
    CGPetInfo::SetPet(player->GetFarsightFocus(), 0);
  }
  return 1;
}

static int FarsightChangeHandler(unsigned __int64, unsigned int, unsigned int, const void *, void *) {
  FrameScript_SignalEvent(326);
  return 1;
}

static int SkillRankChangeHandler(unsigned __int64 player, unsigned int offset, unsigned int, const void *oldValue, void *) {
  unsigned int skillOffset = (offset - 602) / 12;
  FATALASSERT(skillOffset < 64);
  CGPlayer_C *playerPtr = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(player, __FILE__, __LINE__));
  if (playerPtr) {
    unsigned short oldRank = *static_cast<const unsigned short *>(oldValue);
    unsigned short newRank = playerPtr->GetMirrorSkillRank(skillOffset);
    if (newRank != oldRank) {
      ConsolePrintf("Skill %d increased from %d to %d", playerPtr->GetMirrorSkillID(skillOffset), oldRank, newRank);
      CGChat::UpdateLanguages();
    }
  }
  CGActionBar::UpdateUsable();
  CGCharacterInfo::UpdateAllSkillLines();
  return 1;
}

static int SkillMaxRankChangeHandler(unsigned __int64, unsigned int, unsigned int, const void *, void *) {
  CGActionBar::UpdateUsable();
  CGCharacterInfo::UpdateAllSkillLines();
  return 1;
}

static int SkillModifierChangeHandler(unsigned __int64, unsigned int, unsigned int, const void *, void *) {
  CGChat::UpdateLanguages();
  return 1;
}

static void AnimEventCallback(const char *eventName, const NTempest::C3Vector &position, void *param) {
  FATALASSERT(param);
  static_cast<CGPlayer_C *>(param)->HandleAnimEvent(eventName, position);
}

void CGPlayer_C::SetStorage(unsigned long *storage) {
  CGUnit_C::SetStorage(storage);
  CGPlayer::SetStorage(storage + 184);
}

CGPlayer_C::CGPlayer_C(unsigned long *storage, unsigned long eventTime, CClientObjCreate *init)
    : CGUnit_C(storage, eventTime, init),
      CGPlayer(storage + 184),
      m_framesSinceUpdate(-1),
      m_flags(0),
      m_lastWeaponModeSent(-1),
      m_lootingUnit(0),
      m_lootingUnitSent(0),
      m_inventory(GetGUID(), reinterpret_cast<unsigned int *>(storage + 328), reinterpret_cast<unsigned __int64 *>(storage + 184), 1),
      m_lastKillerGUID(0),
      m_pendingItemStats(0) {
  memset(&m_lootingUnit, 0, sizeof(m_lootingUnit) + sizeof(m_lootingUnitSent));
  memset(&m_lastKillerGUID, 0, sizeof(m_lastKillerGUID) + sizeof(m_pendingItemStats));

  if (!m_unit->displayID) {
    FATALERROR(("Error, player %s has displayID 0!", GetUnitName()));
  }
  HMODEL charModel = GetCharacterModel(0);
  FATALASSERT(charModel);

  CMovement::LogWrite("Creating player guid (0x%016I64X)\n", GetGUID());
  InitComponents();
  ModelSetEventCallback(charModel, AnimEventCallback, this, 0);
  HandleClose(charModel);

  memset(m_components, 0, sizeof(m_components));
  memset(m_texComponentInfo, 0, sizeof(m_texComponentInfo));
}

static int SetLocalPlayerInGame(const void *eventData, void *param) {
  ClntObjMgrSetCurrent(static_cast<ClntObjMgr *>(param));
  ClientServices_CharacterSetInGame(1);
  AsyncFileReadWaitAll();

  if (s_pendingCinematicID) {
    CGGameUI::StartCinematic(s_pendingCinematicID);
    s_pendingCinematicID = 0;
  } else {
    DisableLoadingScreen();
  }

  return 1;
}

void CGPlayer_C::SetInventoryMirrorHandler(
    unsigned int slot,
    int(*handler)(unsigned __int64, unsigned int, unsigned int, const void *, void *)
) {
  ClntObjMgrSetObjMirrorHandler(GetGUID(), CGPlayer_C::OffsetOf(ID_PLAYER) + 8 * slot, 8, handler, 0, HANDLER_PRIORITY_HIGH);
}

void CGPlayer_C::UnsetInventoryMirrorHandler(
    unsigned int slot,
    int(*handler)(unsigned __int64, unsigned int, unsigned int, const void *, void *)
) {
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), CGPlayer_C::OffsetOf(ID_PLAYER) + 8 * slot, handler, 0);
}

void CGPlayer_C::SetPlayerMirrorHandlers() {
  for (unsigned int slot = 0; slot < 23; ++slot) {
    if ((1 << slot) & 0x783FD) {
      SetInventoryMirrorHandler(slot, OnUpdateInventoryComponent);
    }
  }

  unsigned int playerOffset = CGPlayer_C::OffsetOf(ID_PLAYER);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), playerOffset + 580, 4, GuildIDUpdateHandler, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), playerOffset + 1776, 4, DuelTeamUpdateHandler, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), playerOffset + 1368, 1, OnUpdatePlayerFlags, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), playerOffset + 1796, 4, OnUpdateGuild, 0, HANDLER_PRIORITY_NORMAL);
}

void CGPlayer_C::UnsetPlayerMirrorHandlers() {
  for (unsigned int slot = 0; slot < 23; ++slot) {
    if ((1 << slot) & 0x783FD) {
      UnsetInventoryMirrorHandler(slot, OnUpdateInventoryComponent);
    }
  }

  unsigned int playerOffset = CGPlayer_C::OffsetOf(ID_PLAYER);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), playerOffset + 580, GuildIDUpdateHandler, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), playerOffset + 1776, DuelTeamUpdateHandler, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), playerOffset + 1368, OnUpdatePlayerFlags, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), playerOffset + 1796, OnUpdateGuild, 0);
}

static int SummonChangeHandler(unsigned __int64, unsigned int, unsigned int, const void *, void *) {
  CGCharacterInfo::UpdateAllSkillLines();
  return 1;
}

int CGPlayer_C::ShouldRender(unsigned long worldStatus) {
  if (s_renderPlayer || GetGUID() != ClntObjMgrGetActivePlayer()) {
    return CGUnit_C::ShouldRender(worldStatus);
  }

  NTempest::C3Vector groundNormal(0.0f, 0.0f, 1.0f);
  ModelProcessEvents(GetObjectModel(), GetPosition(), GetFacing(), groundNormal, 1.0f);
  UpdatePlayerNameWorldText();
  ObjectSetNotRendering();
  return 0;
}

int CGPlayer_C::ShouldRenderUnitName(unsigned int mode) const {
  if ((m_unit->flags & 0x18000) && CGGameUI::GetLockedTarget() != m_obj->m_guid) {
    return 0;
  }
  if (m_obj->m_guid == ClntObjMgrGetActivePlayer() && !s_namePlateRenderOwn->GetInt()) {
    return 0;
  }
  switch (mode) {
    case 1:
      return CGGameUI::GetLockedTarget() == m_obj->m_guid;
    case 2:
      return (m_obj->m_type & TYPE_PLAYER) || CGGameUI::GetLockedTarget() == m_obj->m_guid;
    case 3:
      return 1;
    default:
      return 0;
  }
}

void CGPlayer_C::CommitTexture(int force) {
  char    errorString[512];
  CStatus status;
  TexComponentCommitSections(&status, m_texComponent, force);
  if (!status.IsEmpty()) {
    status.GetErrorStr(errorString, sizeof(errorString), STATUS_INFO);
    NTempest::C3Vector pos = GetPosition();
    FATALERROR(("playerguid:  0x%I64X(%s) (%g,%g,%g): %s", GetGUID(), GetUnitName(), pos.x, pos.y, pos.z, errorString));
  }
}

void CGPlayer_C::SetActiveMirrorHandlers() {
  unsigned int playerOffset = CGPlayer_C::OffsetOf(ID_PLAYER);
  unsigned int unitOffset = CGUnit_C::OffsetOf(ID_UNIT);
  unsigned int component;

  for (component = 312; component <= 496; component += 8) {
    ClntObjMgrSetObjMirrorHandler(GetGUID(), playerOffset + component, 8, OnUpdateInventoryComponent, 0, HANDLER_PRIORITY_NORMAL);
  }
  for (component = 504; component <= 544; component += 8) {
    ClntObjMgrSetObjMirrorHandler(GetGUID(), playerOffset + component, 8, OnUpdateInventoryComponent, 0, HANDLER_PRIORITY_NORMAL);
  }
  for (component = 0; component < 384; component += 24) {
    ClntObjMgrSetObjMirrorHandler(GetGUID(), playerOffset + 1372 + component, 24, OnUpdateQuest, 0, HANDLER_PRIORITY_NORMAL);
  }

  ClntObjMgrSetObjMirrorHandler(GetGUID(), unitOffset + 196, 1, OnUpdateMoney, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), unitOffset + 16, 8, CharmChangeHandler, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), unitOffset, 16, SummonChangeHandler, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), unitOffset + 666, 1, FarsightChangeHandler, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), playerOffset + 560, 8, PetChangeHandler, 0, HANDLER_PRIORITY_NORMAL);

  for (component = 0; component < 768; component += 12) {
    ClntObjMgrSetObjMirrorHandler(GetGUID(), playerOffset + 602 + component, 2, SkillRankChangeHandler, 0, HANDLER_PRIORITY_NORMAL);
    ClntObjMgrSetObjMirrorHandler(GetGUID(), playerOffset + 604 + component, 2, SkillMaxRankChangeHandler, 0, HANDLER_PRIORITY_NORMAL);
    ClntObjMgrSetObjMirrorHandler(GetGUID(), playerOffset + 606 + component, 2, SkillModifierChangeHandler, 0, HANDLER_PRIORITY_NORMAL);
  }
}

void CGPlayer_C::UnsetActiveMirrorHandlers() {
  unsigned int playerOffset = CGPlayer_C::OffsetOf(ID_PLAYER);
  unsigned int unitOffset = CGUnit_C::OffsetOf(ID_UNIT);
  unsigned int component;

  for (component = 312; component <= 496; component += 8) {
    ClntObjMgrUnsetObjMirrorHandler(GetGUID(), playerOffset + component, OnUpdateInventoryComponent, 0);
  }
  for (component = 504; component <= 544; component += 8) {
    ClntObjMgrUnsetObjMirrorHandler(GetGUID(), playerOffset + component, OnUpdateInventoryComponent, 0);
  }
  for (component = 0; component < 384; component += 24) {
    ClntObjMgrUnsetObjMirrorHandler(GetGUID(), playerOffset + 1372 + component, OnUpdateQuest, 0);
  }

  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), unitOffset + 196, OnUpdateMoney, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), unitOffset + 16, CharmChangeHandler, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), unitOffset, SummonChangeHandler, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), playerOffset + 560, PetChangeHandler, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), unitOffset + 666, FarsightChangeHandler, 0);

  for (component = 0; component < 768; component += 12) {
    ClntObjMgrUnsetObjMirrorHandler(GetGUID(), playerOffset + 602 + component, SkillRankChangeHandler, 0);
    ClntObjMgrUnsetObjMirrorHandler(GetGUID(), playerOffset + 604 + component, SkillMaxRankChangeHandler, 0);
    ClntObjMgrUnsetObjMirrorHandler(GetGUID(), playerOffset + 606 + component, SkillModifierChangeHandler, 0);
  }
}

void CGPlayer_C::PostInit(const CClientObjCreate &init) {
  unsigned __int64 item;
  unsigned long    time1;
  unsigned int     sheathe;
  CGItem_C        *itemptr;
  int              linkPoint;

  CGUnit_C::PostInit(init);
  m_fadingMountScale = GetMountScale();
  GetUnitName();
  SetPlayerMirrorHandlers();

  time1 = OsGetAsyncTimeMs();
  for (unsigned int slot = 0; slot < 23; ++slot) {
    if (slot == 17 || !((1 << slot) & 0x783FD) || !IsSlotComponented(slot, 1)) {
      continue;
    }

    item = m_inventory.GetItem(slot);
    if (!item) {
      continue;
    }

    itemptr = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(item, __FILE__, __LINE__));
    FATALASSERT(itemptr);

    sheathe = 0;
    linkPoint = -1;
    if ((1 << slot) & 0x18000) {
      sheathe = m_unit->weaponMode == 1;
      linkPoint = SheatheTypeToSheathePoint(itemptr->GetSheatheType(), slot);
    }

    itemptr = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(item, __FILE__, __LINE__));
    FATALASSERT(itemptr);
    if (itemptr->GetStats()) {
      AttachObjComponent(item, slot, 0, sheathe, linkPoint);
      AddComponent(itemptr->GetDisplayID(), itemptr->GetInventoryType(), slot, 0);
      CharCustomizationAddItemGeosets(
          m_geosetHandle, g_itemDisplayInfoDB.GetRecord(itemptr->GetDisplayID()), itemptr->GetInventoryType(), m_texComponent, m_unit->race, 1
      );
    }
  }
  time1 = OsGetAsyncTimeMs() - time1;
  if (time1 > 15) {
    ConsolePrintf("CGPlayer_C::PostInit Setup Components(): %dms\n", time1);
  }

  item = m_inventory.GetItem(17);
  if (item) {
    AttachObjComponent(item, 17, 0, 0, -1);
    SetSheatheReason(SHEATHEREASON_5, m_unit->weaponMode == 2, 1);
  }

  CharCustomizationCommitItemGeosets(m_geosetHandle, 1);
  OnGuildChanged();
  if (m_geosetHandle) {
    CharCustomizationCommitGeosets(m_geosetHandle);
    Animate();
  }

  if (GetGUID() == ClntObjMgrGetActivePlayer()) {
    SetActiveMirrorHandlers();
    SndInterfaceInitializeVocalUISounds(m_unit->race, m_unit->sex);
    ClientSetTimer(500, SetLocalPlayerInGame, ClntObjMgrGetCurrent());
    CGSpellBook::ClearSpells();

    for (unsigned int index = 0; index < s_initialSpells.Count(); ++index) {
      AddKnownSpell(s_initialSpells[index].spellID, s_initialSpells[index].slot, 0, 1);
    }

    if (ClntObjMgrGetPlayerType() == PLAYER_NORMAL) {
      for (unsigned int index = 0; index < 120; ++index) {
        int action = s_initialButtons[index];
        if (action < 0 || IsSpellKnown(action)) {
          CGActionBar::SetAction(index, action);
        }
      }
    }
  }

  if (GetGUID() == ClntObjMgrGetActivePlayer() && ClntObjMgrGetPlayerType() == PLAYER_NORMAL) {
    CGGameUI::EnterWorld();
    CGGameUI::UpdateActivePlayer();
  }
  CGPartyInfo::EnableMember(GetGUID(), 1);
}
void CGPlayer_C::PostReenable() {
  CGUnit_C::PostReenable();
  CGPartyInfo::EnableMember(GetGUID(), 1);
}

void CGPlayer_C::OnMount() {
  CGObject_C *object = ClntObjMgrObjectPtr(CGGameUI::GetLockedTarget(), __FILE__, __LINE__);
  if (object) {
    object->UpdatePlayerName();
  }
}

void CGPlayer_C::OnDismount() {
  CGObject_C *object = ClntObjMgrObjectPtr(CGGameUI::GetLockedTarget(), __FILE__, __LINE__);
  if (object) {
    object->UpdatePlayerName();
  }
}

void CGPlayer_C::InspectPlayer(const unsigned __int64 &guid) {
  if (!guid) {
    return;
  }
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_INSPECT));
  msg.Put(guid);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::ReceiveResurrectRequest(const char *name) {
  if (name && *name && GetUnitData()->health <= 0) {
    CGGameUI::OpenResurrectRequest(name);
  }
}

void CGPlayer_C::AcceptResurrectRequest(int accept) {
  if (!s_resurrectOffer) {
    return;
  }
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_RESURRECT_RESPONSE));
  msg.Put(s_resurrectOffer);
  msg.Put(static_cast<unsigned char>(accept != 0));
  msg.Finalize();
  ClientServices_Send(&msg);
  s_resurrectOffer = 0;
}

const char *CGPlayer_C::GetModelFileName() const {
  CreatureDisplayInfoRec *displayInfo = g_creatureDisplayInfoDB.GetRecord(m_unit->displayID);
  if (!displayInfo) {
    SysMsgPrintf(SYSMSG_WARNING, 16, "INVALIDPLAYERDISPLAYID|%d|%d|%d", m_unit->displayID, m_unit->race, m_unit->sex);
    return "NoName";
  }

  CreatureModelDataRec *modelData = g_creatureModelDataDB.GetRecord(displayInfo->m_modelID);
  if (!modelData) {
    SysMsgPrintf(SYSMSG_WARNING, 16, "INVALIDPLAYERMODELRECORD|%d|%d|%d", displayInfo->m_modelID, m_unit->race, m_unit->sex);
    return "NoName";
  }

  return modelData->m_ModelName;
}

void CGPlayer_C::InitPreferredGeosets() {
  memset(m_preferredGeosets, 0, sizeof(m_preferredGeosets));
  m_preferredGeosets[CGS_HAIR] = CharCustomizationGetHairGeoset(m_unit->race, m_unit->sex, GetHairStyle());

  BEARDSTYLEDATA beardStyleData = {1, 1, 1};
  m_preferredGeosets[CGS_EARS] = 2;
  if (CharCustomizationGetBeardStyle(m_unit->race, m_unit->sex, GetFacialHair(), &beardStyleData)) {
    m_preferredGeosets[CGS_FACIAL_BEARD] = beardStyleData.beardGeoset;
    m_preferredGeosets[CGS_FACIAL_SIDEBURN] = beardStyleData.sideBurnGeoset;
    m_preferredGeosets[CGS_FACIAL_MOUSTACHE] = beardStyleData.moustacheGeoset;
  }
}

void CGPlayer_C::InitComponents() {
  FATALASSERT(m_modelData);
  if (!(m_modelData->m_flags & 4)) {
    return;
  }

  unsigned long time1 = OsGetAsyncTimeMs();
  HMODEL        charModel = GetCharacterModel(0);
  FATALASSERT(charModel);

  HTEXTURE skinTexture = CharCustomizationSetSkin(charModel, m_unit->race, m_unit->sex, GetSkin(), 0);
  if (!skinTexture) {
    FATALERROR(
        ("Error, skinID %d on character %s (race/sex is %d/%d) cannot be loaded, is it a missing file?", GetSkin(), GetUnitName(), m_unit->race,
         m_unit->sex)
    );
  }

  m_texComponent = TexComponentCreate(skinTexture, m_unit->race, m_unit->sex, GetSkin(), 0, 0);
  FATALASSERT(m_texComponent);
  HandleClose(skinTexture);

  CharCustomizationSetFaceTexture(charModel, m_texComponent, m_unit->race, m_unit->sex, GetFace(), GetSkin(), 0);
  CharCustomizationSetHairTexture(charModel, m_texComponent, m_unit->race, m_unit->sex, GetHairStyle(), GetHairColorID());
  CharCustomizationSetFacialTexture(charModel, m_texComponent, m_unit->race, m_unit->sex, GetFacialHair(), GetHairColorID());

  BEARDSTYLEDATA facialData = {1, 1, 1};
  int            hasFacialData = CharCustomizationGetBeardStyle(m_unit->race, m_unit->sex, GetFacialHair(), &facialData);

  m_geosetHandle = CharCustomizationCreateGeosetHandle(charModel);
  FATALASSERT(m_geosetHandle);
  InitPreferredGeosets();
  CharCustomizationInitBaseCharacter(
      m_geosetHandle, hasFacialData ? facialData.beardGeoset : 1, hasFacialData ? facialData.sideBurnGeoset : 1,
      hasFacialData ? facialData.moustacheGeoset : 1, 2
  );
  CharCustomizationResetHairGeoset(m_geosetHandle, m_unit->race, m_unit->sex, GetHairStyle());
  HandleClose(charModel);

  unsigned long elapsed = OsGetAsyncTimeMs() - time1;
  if (elapsed > 15) {
    ConsolePrintf("CGPlayer_C::InitComponents(): %dms\n", elapsed);
  }
}

void CGPlayer_C::AddComponent(int displayID, unsigned int inventoryType, int slot, int commit) {
  FATALASSERT(inventoryType < 27);

  if ((1 << slot) & 0x403F8) {
    m_texComponentInfo[slot].m_displayID = displayID;
    m_texComponentInfo[slot].m_inventoryType = inventoryType;
  }

  HTEXCOMPONENT             texComponent = m_texComponent;
  const ItemDisplayInfoRec *displayInfo = g_itemDisplayInfoDB.GetRecord(displayID);
  if (texComponent) {
    if ((1 << slot) & 0x403F8) {
      CStatus status;
      TexComponentAdd(&status, m_unit->sex, texComponent, displayInfo, inventoryType, 1);
      if (!status.IsEmpty()) {
        char buffer[512];
        status.GetErrorStr(buffer, sizeof(buffer), STATUS_INFO);
        NTempest::C3Vector pos;
        GetPosition(pos);
        FATALERROR(("player 0x%I64X(%s)(%g,%g,%g): %s", GetGUID(), GetUnitName(), pos.x, pos.y, pos.z, buffer));
      }
    }

    if (displayInfo) {
      if (slot == 18 && inventoryType == 19) {
        OnGuildChanged();
      }
      CharCustomizationAddItemGeosets(
          m_geosetHandle, displayInfo, inventoryType, texComponent, m_unit->race, commit == 0
      );
    }
  }

  if (commit) {
    CharCustomizationCommitItemGeosets(m_geosetHandle, 0, m_paperDollModel);
    Animate();
  }

  if (!slot) {
    HeadGeosetHideCharGeosets(
        m_geosetHandle, displayInfo, m_unit->race, m_preferredGeosets, 15
    );
  }
  CGUnit_C::m_flags &= ~0x100u;
}

void CGPlayer_C::TalkToTrainer(const unsigned __int64 &trainerUnit) {
  s_lastVendorListReceived = trainerUnit;
  CGClassTrainer::SetTrainer(0, TRAINER_TYPE_GENERAL);

  CDataStore hello;
  hello.Put(static_cast<unsigned int>(CMSG_TRAINER_LIST));
  hello.Put(trainerUnit);
  hello.Finalize();
  ClientServices_Send(&hello);
}

int CGPlayer_C::LootUnit(CGUnit_C *unit) {
  if (CanLoot(unit) && unit->CanBeLooted(OsGetAsyncTimeMs()) &&
      !(m_move.GetMoveFlags() & 0x40FF)) {
    CGGameUI::CloseLoot(1, 1);

    CDataStore lootMsg;
    lootMsg.Put(static_cast<unsigned int>(CMSG_LOOT));
    lootMsg.Put(unit->GetGUID());
    lootMsg.Finalize();
    ClientServices_Send(&lootMsg);

    m_lootingUnitSent = unit->GetGUID();
    UpdateBaseAnimation(44, 0);
  }
  return 1;
}

void CGPlayer_C::ShopFromMerchant(const unsigned __int64 &merchant) {
  unsigned __int64 cursorItem = CGGameUI::GetCursorItem();
  if (cursorItem) {
    SellItem(merchant, cursorItem, 0);
    CGGameUI::SetCursorItem(0, 0, 0, 0, 0);
    return;
  }

  CDataStore invMsg;
  invMsg.Put(static_cast<unsigned int>(CMSG_LIST_INVENTORY));
  invMsg.Put(merchant);
  invMsg.Finalize();
  ClientServices_Send(&invMsg);
}

int CGPlayer_C::IsQuestUnit(CGUnit_C *unit) {
  if (unit->GetUnitData()->npcFlags & 2) {
    return 1;
  }

  for (unsigned int index = 0; index < 16; ++index) {
    if (m_plyr->questLog[index].m_questRewarderID == unit->GetEntryID()) {
      return 1;
    }
  }
  return 0;
}

void CGPlayer_C::TalkToQuestUnit(const unsigned __int64 &unit) {
  CDataStore hello;
  hello.Put(static_cast<unsigned int>(CMSG_QUESTGIVER_HELLO));
  hello.Put(unit);
  hello.Finalize();
  ClientServices_Send(&hello);
}

int CGPlayer_C::QueryTaxiNodes(const unsigned __int64 &unit) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_TAXIQUERYAVAILABLENODES));
  msg.Put(unit);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

void CGPlayer_C::TalkToBinder(const unsigned __int64 &binder) {
  if (s_lastBinderID && binder == s_lastBinderID) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(283));
    return;
  }

  CDataStore hello;
  hello.Put(static_cast<unsigned int>(CMSG_BINDER_ACTIVATE));
  hello.Put(binder);
  hello.Finalize();
  ClientServices_Send(&hello);
}

void CGPlayer_C::TalkToBanker(const unsigned __int64 &banker) {
  CDataStore hello;
  hello.Put(static_cast<unsigned int>(CMSG_BANKER_ACTIVATE));
  hello.Put(banker);
  hello.Finalize();
  ClientServices_Send(&hello);
}

void CGPlayer_C::TalkToNpcPetition(const unsigned __int64 &vendor) {
  CDataStore hello;
  hello.Put(static_cast<unsigned int>(CMSG_PETITION_SHOWLIST));
  hello.Put(vendor);
  hello.Finalize();
  ClientServices_Send(&hello);
}

void CGPlayer_C::TalkToTabardVendor(const unsigned __int64 &tabardUnit) {
  CDataStore hello;
  hello.Put(static_cast<unsigned int>(MSG_TABARDVENDOR_ACTIVATE));
  hello.Put(tabardUnit);
  hello.Finalize();
  ClientServices_Send(&hello);
}

int CGPlayer_C::OnTerrainClick(CTerrainClickEvent &__formal) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }

  unsigned __int64 cursorItem;
  unsigned __int64 cursorItemPack;
  unsigned int     cursorSlot;
  CGGameUI::GetCursorItem(cursorItem, cursorItemPack, cursorSlot);
  CGGameUI::ClearCursor(0);

  if (ClntObjMgrObjectPtr(cursorItemPack, __FILE__, __LINE__) && cursorItem) {
    unsigned int       packSlot = FindSlotIndex(cursorItem);
    NTempest::C3Vector position = GetPosition();

    CDataStore msg;
    msg.Put(CMSG_DROP_ITEM);
    msg.Put(packSlot);
    msg.Put(cursorSlot);
    msg.Put(position.x);
    msg.Put(position.y);
    msg.Put(position.z);
    msg.Finalize();
    ClientServices_Send(&msg);
  }
  return 1;
}

void CGPlayer_C::SaveTabard(int eStyle, int eColor, int bStyle, int bColor, int bg, unsigned __int64 vendor) const {
  if (eStyle < 0 || eColor < 0 || bStyle < 0 || bColor < 0 || bg < 0 || eStyle >= 42 || eColor >= 4 || bStyle >= 2 || bColor >= 4 || bg >= 19) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(271));
    return;
  }

  const unsigned __int64 noGuid = 0;
  const GuildStats_C    *guild = g_guildInfoCache.GetRecord(GetGuildID(), noGuid, 0, 0);
  if (!guild) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(272));
    return;
  }
  if (guild->m_emblemStyle != -1 || guild->m_emblemColor != -1 || guild->m_borderStyle != -1 || guild->m_borderColor != -1 ||
      guild->m_backgroundColor != -1)
  {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(273));
    return;
  }
  if (GetGuildRank()) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(274));
    return;
  }
  if (GetUnitData()->coinage < GuildGetTabardCost()) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(32));
    return;
  }

  CDataStore msg;
  msg.Put(static_cast<unsigned int>(MSG_SAVE_GUILD_EMBLEM));
  msg.Put(eStyle);
  msg.Put(eColor);
  msg.Put(bStyle);
  msg.Put(bColor);
  msg.Put(bg);
  msg.Put(vendor);
  msg.Finalize();
  ClientServices_Send(&msg);
}

bool CGPlayer_C::OnGuildChanged() {
  PlayerNameTriggerNameRegenerate(m_unitNameHandle);
  FrameScript_SignalEvent(359);

  HTEXCOMPONENT component = GetTexComponent();
  if (!component) {
    return 0;
  }

  CGBag_C *inventory = GetBag();
  FATALASSERT(inventory);

  CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(inventory->GetItem(18), __FILE__, __LINE__));
  if (!item || item->GetInventoryType() != 19 || item->GetDisplayID() <= 0) {
    return 0;
  }

  const ItemDisplayInfoRec *displayInfo = g_itemDisplayInfoDB.GetRecord(item->GetDisplayID());
  if (!displayInfo || !(displayInfo->m_flags & 1)) {
    return 0;
  }

  unsigned __int64 guid = GetGUID();
  Script_SendUnitSignal(guid, 324);

  int eStyle;
  int eColor;
  int bStyle;
  int bColor;
  int background;
  if (GuildGetGuildTabard(GetGuildID(), GuildCallback, eStyle, eColor, bStyle, bColor, background) &&
      ComponentApplyTabardTexture(component, eStyle, eColor, bStyle, bColor, background))
  {
    return 1;
  }

  ComponentRemoveTabardTexture(GetUnitData()->sex, component, const_cast<ItemDisplayInfoRec *>(displayInfo), item->GetInventoryType());
  return 1;
}

int CGPlayer_C::CanEngageTarget(CGUnit_C *unitPtr) {
  FATALASSERT(unitPtr);
  if ((GetPosition() - unitPtr->GetPosition()).SquaredMag() < 10.45f * 10.45f) {
    return 1;
  }
  CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(112));
  return 0;
}

void CGPlayer_C::HandleRepopRequest() {
  if (GetUnitData()->health <= 0) {
    CDataStore msg;
    msg.Put(static_cast<unsigned int>(CMSG_REPOP_REQUEST));
    msg.Finalize();
    ClientServices_Send(&msg);
  }
}

static void SwapItemsStatsCallback(int id, const unsigned __int64& guid, void* arg, unsigned char granted) {
  unsigned __int64 noGuid = 0;
  const ItemStats_C *stats = g_itemDBCache.GetRecord(id, noGuid, 0, 0);
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  unsigned int count = s_pendingSwaps.Count();
  for (unsigned int index = 0; index < count; ++index) {
    ITEMSWAP &swap = s_pendingSwaps[index];
    if (swap.pendingID == id) {
      if (stats && player) {
        player->SwapItems(0, swap.bagA, swap.slotA, swap.bagB, swap.slotB, 0);
      }
      swap.bagA = 0;
      swap.bagB = 0;
      swap.slotA = -1;
      swap.slotB = -1;
      swap.pendingID = 0;
    }
  }
}

static unsigned int FindEmptySwapIndex() {
  unsigned int index;
  for (index = 0; index < s_pendingSwaps.Count(); ++index) {
    if (!s_pendingSwaps[index].bagA) {
      return index;
    }
  }

  s_pendingSwaps.SetCount(index + 1);
  s_pendingSwaps[index].bagA = 0;
  s_pendingSwaps[index].bagB = 0;
  s_pendingSwaps[index].slotA = -1;
  s_pendingSwaps[index].slotB = -1;
  s_pendingSwaps[index].pendingID = 0;
  return index;
}

void CGPlayer_C::SwapItems(
    unsigned __int64 cursorItem,
    unsigned __int64 cursorContainer,
    int              cursorSlot,
    unsigned __int64 containerB,
    int              slotB,
    int              force
) {
  FATALASSERT(cursorSlot <= 0xFF);
  FATALASSERT(slotB <= 0xFF);

  unsigned int cursorItemContainerSlot = FindSlotIndex(cursorContainer);
  unsigned int newContainerSlot = FindSlotIndex(containerB);
  CDataStore   msg;

  if (!cursorItem) {
    msg.Put(static_cast<unsigned int>(CMSG_PICKUP_ITEM));
    msg.Put(containerB);
    msg.Put(static_cast<unsigned char>(newContainerSlot));
    msg.Put(static_cast<unsigned char>(slotB));
  } else if (cursorContainer == GetGUID() && cursorSlot < 23 && containerB == GetGUID() && slotB < 23) {
    msg.Put(static_cast<unsigned int>(CMSG_SWAP_INV_ITEM));
    msg.Put(static_cast<unsigned char>(cursorSlot));
    msg.Put(static_cast<unsigned char>(slotB));
  } else {
    msg.Put(static_cast<unsigned int>(CMSG_SWAP_ITEM));
    msg.Put(static_cast<unsigned char>(newContainerSlot));
    msg.Put(static_cast<unsigned char>(slotB));
    msg.Put(static_cast<unsigned char>(cursorItemContainerSlot));
    msg.Put(static_cast<unsigned char>(cursorSlot));
  }
  msg.Finalize();
  ClientServices_Send(&msg);
  CGGameUI::ClearCursor(0);
}

void CGPlayer_C::SplitItem(
    unsigned __int64 cursorItem,
    unsigned __int64 cursorContainer,
    int              cursorSlot,
    unsigned __int64 containerB,
    int              slotB,
    int              quantity
) {
  FATALASSERT(cursorSlot <= 0xFF);
  FATALASSERT(slotB <= 0xFF);
  FATALASSERT(quantity <= 0xFF);
  FATALASSERT(quantity > 0);

  CDataStore msg;
  if (cursorContainer) {
    unsigned char cursorItemContainerSlot = static_cast<unsigned char>(FindSlotIndex(cursorContainer));
    unsigned char newContainerSlot = static_cast<unsigned char>(FindSlotIndex(containerB));

    msg.Put(static_cast<unsigned int>(CMSG_SPLIT_ITEM));
    msg.Put(cursorItemContainerSlot);
    msg.Put(static_cast<unsigned char>(cursorSlot));
    msg.Put(newContainerSlot);
    msg.Put(static_cast<unsigned char>(slotB));
    msg.Put(static_cast<unsigned char>(quantity));
    msg.Finalize();
    ClientServices_Send(&msg);
  }
}

void CGPlayer_C::AutoStoreItemInBag(
    unsigned __int64 cursorItem,
    unsigned __int64 cursorContainer,
    int              cursorSlot,
    unsigned __int64 containerB,
    int              ignoreOwnershipRules
) {
  FATALASSERT(cursorSlot <= 0xFF);
  if (!cursorItem) {
    return;
  }

  unsigned char cursorItemContainerSlot = static_cast<unsigned char>(FindSlotIndex(cursorContainer));
  unsigned char newContainerSlot = static_cast<unsigned char>(FindSlotIndex(containerB));
  if (newContainerSlot == 0xFF && containerB != GetGUID()) {
    return;
  }

  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_AUTOSTORE_BAG_ITEM));
  msg.Put(cursorItemContainerSlot);
  msg.Put(static_cast<unsigned char>(cursorSlot));
  msg.Put(newContainerSlot);
  msg.Finalize();
  ClientServices_Send(&msg);
}

unsigned int CGPlayer_C::FindSlotIndex(unsigned __int64 obj) {
  unsigned char slot = 0;
  while (slot < m_inventory.NumSlots()) {
    if (m_inventory.GetItem(slot) == obj) {
      return slot;
    }
    ++slot;
  }
  return 0xFF;
}

static void AutoEquipStatsCallback(int id, const unsigned __int64& guid, void* arg, unsigned char granted) {
  if (granted) {
    unsigned __int64 noGuid = 0;
    if (g_itemDBCache.GetRecord(id, noGuid, 0, 0)) {
      CGObject_C *item = ClntObjMgrObjectPtr(CGGameUI::GetCursorItem(), __FILE__, __LINE__);
      if (item && item->GetEntryID() == id) {
        CGPlayer_C *player =
            static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
        if (player) {
          player->AutoEquipCursorItem(0);
        }
      }
    }
  }
}

void CGPlayer_C::AutoEquipCursorItem(int force) {
  unsigned __int64 cursorItem;
  unsigned __int64 cursorItemPack;
  unsigned int     cursorItemSlot;
  CGGameUI::GetCursorItem(cursorItem, cursorItemPack, cursorItemSlot);
  if (!cursorItem) {
    return;
  }

  FATALASSERT(cursorItemSlot <= 0xFF);
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_AUTOEQUIP_ITEM));
  msg.Put(static_cast<unsigned char>(FindSlotIndex(cursorItemPack)));
  msg.Put(static_cast<unsigned char>(cursorItemSlot));
  msg.Finalize();
  ClientServices_Send(&msg);
  CGGameUI::ClearCursor(0);
}

void CGPlayer_C::AutoEquipItem(unsigned __int64 container, unsigned int slot, int force) {
  CGObject_C *containerObject = ClntObjMgrObjectPtr(container, __FILE__, __LINE__);
  CGBag      *bag = containerObject ? containerObject->GetBag() : 0;
  if (!bag || slot >= bag->NumSlots()) {
    return;
  }
  unsigned __int64 item = bag->GetItem(slot);
  if (!item) {
    return;
  }
  CGGameUI::SetCursorItem(item, container, slot, 0, 0);
  AutoEquipCursorItem(force);
}

void CGPlayer_C::AutoStoreLootItem(unsigned char slot) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_AUTOSTORE_LOOT_ITEM));
  msg.Put(slot);
  msg.Finalize();
  ClientServices_Send(&msg);
}
void CGPlayer_C::ClearPendingEquip(unsigned int index, int equip) {
  if (index >= s_pendingSwaps.Count()) {
    return;
  }

  ITEMSWAP swap = s_pendingSwaps[index];
  if (equip) {
    CGObject_C      *containerObject = ClntObjMgrObjectPtr(swap.bagA, __FILE__, __LINE__);
    CGBag           *container = containerObject ? containerObject->GetBag() : 0;
    unsigned __int64 item = container ? container->GetItem(swap.slotA) : 0;
    if (swap.bagB) {
      SwapItems(item, swap.bagA, swap.slotA, swap.bagB, swap.slotB, 1);
    } else {
      AutoEquipItem(swap.bagA, swap.slotA, 1);
    }
  }

  swap.bagA = 0;
  swap.bagB = 0;
  swap.slotA = -1;
  swap.slotB = -1;
  swap.pendingID = 0;
  s_pendingSwaps[index] = swap;
}
void CGPlayer_C::TogglePlayerBounds() {
  static int  boundsPresent;
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  HMODEL      model = player ? player->GetCharacterModel(0) : 0;
  if (model) {
    boundsPresent = !boundsPresent;
    if (boundsPresent) {
      ModelShowBoundingSphere(model);
    } else {
      ModelHideBounds(model);
    }
    HandleClose(model);
  }
}

void CGPlayer_C::SellItem(unsigned __int64 merchant, unsigned __int64 item, unsigned int amount) {
  CDataStore sellMsg;
  sellMsg.Put(static_cast<unsigned int>(CMSG_SELL_ITEM));
  sellMsg.Put(merchant);
  sellMsg.Put(item);
  sellMsg.Put(static_cast<unsigned char>(amount));
  sellMsg.Finalize();
  ClientServices_Send(&sellMsg);
}

void CGPlayer_C::SetActive(CGPlayer_C *playerPtr) {
  if (ClntObjMgrGetPlayerType() != PLAYER_BOT) {
    UnitCombatLogSetActivePlayer(playerPtr);
    unsigned __int64 guid = playerPtr ? playerPtr->GetGUID() : 0;
    CGUnit_C::SetActiveMover(guid);
  }
  ConsolePrintf("Local player guid (0x%016I64X)\n", ClntObjMgrGetActivePlayer());
}

unsigned int CGPlayer_C::OffsetOf(OBJECT_TYPE_ID type) {
  switch (type) {
    case ID_OBJECT:
      return 0;
    case ID_ITEM:
    case ID_UNIT:
      return 24;
    case ID_CONTAINER:
      return 144;
    case ID_PLAYER:
      return 736;
    default:
      FATALASSERT(0);
      return -1;
  }
}

unsigned int CGPlayer_C::GetProficiency(unsigned char type) {
  return type < 16 ? s_playerProficiencies[type] : 0;
}

void CGPlayer_C::XBuyItem(unsigned __int64 merchant, unsigned int itemID, unsigned int quantity, unsigned int autoEquip) {
  CDataStore buyMsg;
  buyMsg.Put(static_cast<unsigned int>(CMSG_BUY_ITEM));
  buyMsg.Put(merchant);
  buyMsg.Put(itemID);
  buyMsg.Put(static_cast<unsigned char>(quantity));
  buyMsg.Put(static_cast<unsigned char>(autoEquip));
  buyMsg.Finalize();
  ClientServices_Send(&buyMsg);
}

int BuyCommandHandler(const char *command, const char *arguments) {
  unsigned int itemID = SStrToInt(arguments);
  if (!itemID) {
    ConsoleWrite("Usage: buy <muid>, where <muid> is the item id", DEFAULT_COLOR);
    return 1;
  }

  if (!s_lastVendorListReceived) {
    ConsoleWrite("Must click a vendor first", DEFAULT_COLOR);
    return 1;
  }

  CGPlayer_C::XBuyItem(s_lastVendorListReceived, itemID, 1, 1);
  return 1;
}

int UndressMeHandler(const char *command, const char *arguments) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->DeleteWornItems();
  }
  return 1;
}

int GodmodeHandler(const char *command, const char *arguments) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_GODMODE));
  msg.Put(static_cast<unsigned char>(SStrToInt(arguments) != 0));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

int CCommand_LevelUp(const char *command, const char *arguments) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_LEVELUP_CHEAT));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

int CCommand_SetFaction(const char *command, const char *arguments) {
  CDataStore msg;
  int        level;
  int        i;

  if (!isdigit(*arguments) && *arguments != '-') {
    ConsolePrintf("Unknown faction (usage: setfaction <level> <faction name>)");
    return 1;
  }

  level = SStrToInt(arguments);
  while (*arguments) {
    if (!isdigit(*arguments) && !isspace(*arguments) && *arguments != '-') {
      break;
    }
    ++arguments;
  }

  const FactionRec *faction = 0;
  for (i = 0; i < g_factionDB.GetNumRecords(); ++i) {
    const FactionRec *candidate = g_factionDB.GetRecordByIndex(i);
    if (candidate->m_reputationIndex >= 0 && !SStrCmp(candidate->m_name_lang[CURRENT_LANGUAGE], arguments, SStrLen(arguments))) {
      faction = candidate;
      break;
    }
  }

  if (!faction || !faction->m_ID) {
    ConsolePrintf("Unknown faction (usage: setfaction <level> <faction name>)");
    return 1;
  }

  msg.Put(static_cast<unsigned int>(CMSG_SET_FACTION_CHEAT));
  msg.Put(faction->m_ID);
  msg.Put(level);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

int CCommand_Invite(const char *command, const char *arguments) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_GROUP_INVITE));
  msg.PutString(arguments);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

int CCommand_Accept(const char *command, const char *arguments) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_GROUP_ACCEPT));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

int CCommand_Decline(const char *command, const char *arguments) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_GROUP_DECLINE));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

int CCommand_Disband(const char *command, const char *arguments) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_GROUP_DISBAND));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

int CCommand_NewLeader(const char *command, const char *arguments) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_GROUP_SET_LEADER));
  msg.PutString(arguments);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

int CCommand_Uninvite(const char *command, const char *arguments) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_GROUP_UNINVITE));
  msg.PutString(arguments);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

int CCommand_AcceptRes(const char *, const char *) {
  if (s_resurrectOffer) {
    CDataStore msg;
    msg.Put(static_cast<unsigned int>(CMSG_RESURRECT_RESPONSE));
    msg.Put(s_resurrectOffer);
    msg.Put(static_cast<unsigned char>(1));
    msg.Finalize();
    ClientServices_Send(&msg);
    s_resurrectOffer = 0;
  }
  return 1;
}

int CCommand_DeclineRes(const char *, const char *) {
  if (s_resurrectOffer) {
    CDataStore msg;
    msg.Put(static_cast<unsigned int>(CMSG_RESURRECT_RESPONSE));
    msg.Put(s_resurrectOffer);
    msg.Put(static_cast<unsigned char>(0));
    msg.Finalize();
    ClientServices_Send(&msg);
    s_resurrectOffer = 0;
  }
  return 1;
}

int CCommand_ForceMonsterAnim(const char *__formal, const char *arguments) {
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(CGGameUI::GetLockedTarget(), __FILE__, __LINE__));
  if (unit) {
    unit->SetForcedAnimation(arguments);
  } else {
    ConsoleWrite("Error, unable to get locked target", DEFAULT_COLOR);
  }
  return 1;
}

int CCommand_ResetMonsterAnim(const char *, const char *) {
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(CGGameUI::GetLockedTarget(), __FILE__, __LINE__));
  if (unit) {
    unit->ResetForcedAnimation();
  }
  return 1;
}

int DumpDeathLogEnumHandler(unsigned __int64 object, void *param) {
  ASSERT(param);
  CGObject_C *objectPtr = ClntObjMgrObjectPtr(object, __FILE__, __LINE__);
  if (objectPtr && (objectPtr->GetType() & TYPE_UNIT)) {
    static_cast<CGUnit_C *>(objectPtr)->DumpGeneralDeathHoldLog(static_cast<HSLOG>(param), 0);
  }
  return 1;
}

int CCommand_DumpDeathHoldLogs(const char *, const char *) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    HSLOG log;
    SLogCreate("DeathHoldGeneralLog.txt", 0, &log);
    if (log) {
      ClntObjMgrEnumVisibleObjects(DumpDeathLogEnumHandler, log);
      SLogClose(log);
    }
  }
  return 1;
}

int CCommand_ShowPet(const char *, const char *) {
  char        buffer[256];
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    const CGUnitData *unit = player->GetUnitData();
    unsigned __int64  pet = unit->charm ? unit->charm : unit->summon;
    SStrPrintf(buffer, sizeof(buffer), "Current pet: 0x%016I64X\n", pet);
    ConsoleWrite(buffer, DEFAULT_COLOR);
  }
  return 1;
}

int CCommand_TaxiShowNodes(const char *, const char *) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_TAXISHOWNODES));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

int CCommand_GuildCreate(const char *command, const char *arguments) {
  if (arguments && *arguments) {
    CDataStore msg;
    msg.Put(static_cast<unsigned int>(CMSG_GUILD_CREATE));
    msg.PutString(arguments);
    msg.Finalize();
    ClientServices_Send(&msg);
  } else {
    ConsoleWriteA("You must specify a guild name!", ERROR_COLOR);
  }
  return 1;
}

void PrintForceActionUsage(const char *command) {
  ASSERT(command);
  ConsolePrintf("usage: %s [x] [0|1] where [x] is one of:", command);
  for (unsigned int i = 0; i < 18; ++i) {
    ConsolePrintf("  %d   %s", i, s_actionsArray[i]);
  }
}

void SendForceActionMessage(int set, int onSelf, unsigned int argument) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(onSelf ? CMSG_FORCEACTION : CMSG_FORCEACTIONONOTHER));
  msg.Put(argument);
  msg.Put(set);
  msg.Finalize();
  ClientServices_Send(&msg);
}

int CCommand_ForceActionSet(const char *command, const char *arguments) {
  if (arguments && *arguments) {
    unsigned int argument = SStrToUnsigned(arguments);
    if (argument < 18) {
      SendForceActionMessage(1, 1, argument);
    } else {
      ConsoleWrite("invalid argument", DEFAULT_COLOR);
    }
  } else {
    PrintForceActionUsage(command);
  }
  return 1;
}

int CCommand_ForceActionUnset(const char *command, const char *arguments) {
  if (arguments && *arguments) {
    unsigned int argument = SStrToUnsigned(arguments);
    if (argument < 18) {
      SendForceActionMessage(0, 1, argument);
    } else {
      ConsoleWrite("invalid argument", DEFAULT_COLOR);
    }
  } else {
    PrintForceActionUsage(command);
  }
  return 1;
}

int CCommand_ForceActionOnOtherSet(const char *command, const char *arguments) {
  if (arguments && *arguments) {
    unsigned int argument = SStrToUnsigned(arguments);
    if (argument < 18) {
      SendForceActionMessage(1, 0, argument);
    } else {
      ConsoleWrite("invalid argument", DEFAULT_COLOR);
    }
  } else {
    PrintForceActionUsage(command);
  }
  return 1;
}

int CCommand_ForceActionOnOtherUnset(const char *command, const char *arguments) {
  if (arguments && *arguments) {
    unsigned int argument = SStrToUnsigned(arguments);
    if (argument < 18) {
      SendForceActionMessage(0, 0, argument);
    } else {
      ConsoleWrite("invalid argument", DEFAULT_COLOR);
    }
  } else {
    PrintForceActionUsage(command);
  }
  return 1;
}

int CCommand_ForceActionShowFlags(const char *command, const char *arguments) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_FORCEACTIONSHOW));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

int CCommand_TogglePVP(const char *command, const char *arguments) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    unsigned int enable = !(player->GetUnitData()->flags & 1);
    CDataStore   msg;
    msg.Put(static_cast<unsigned int>(CMSG_ENABLE_PVP));
    msg.Put(static_cast<unsigned char>(enable));
    msg.Finalize();
    ClientServices_Send(&msg);
    ConsoleWrite(enable ? "PVP Enabled" : "PVP Disabled", DEFAULT_COLOR);
  }
  return 1;
}

int CCommand_Cinematic(const char *command, const char *arguments) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_TRIGGER_CINEMATIC_CHEAT));
  msg.Put(static_cast<unsigned int>(SStrToInt(arguments)));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

int OnProficiency(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
  unsigned int proficiencyMask;
  unsigned int proficiencyClass;
  msg->Get(*reinterpret_cast<unsigned char *>(&proficiencyClass));
  msg->Get(proficiencyMask);
  s_playerProficiencies[proficiencyClass] = proficiencyMask;
  ConsolePrintf("Proficiency in item class %d set to %08x", proficiencyClass, proficiencyMask);
  CGCharacterInfo::UpdateAllSkillLines();
  return 1;
}

void ResurrectNameQueryCallback(int, const unsigned __int64 &, void *, bool) {
  const NameCache *name = g_nameDBCache.GetRecord(s_resurrectOffer, s_resurrectOffer, 0, 0);
  if (name) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      player->ReceiveResurrectRequest(name->m_name);
    }
  } else {
    s_resurrectOffer = 0;
  }
}

int OnResurrectRequest(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
  unsigned __int64 guid;
  msg->Get(guid);
  s_resurrectOffer = guid;

  const NameCache *name = g_nameDBCache.GetRecord(guid, guid, ResurrectNameQueryCallback, 0);
  if (name) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      player->ReceiveResurrectRequest(name->m_name);
    }
  }

  return 1;
}

int OnInspectNotify(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
  unsigned __int64 guid;
  msg->Get(guid);
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (unit) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(136), unit->GetUnitName());
  }
  return 1;
}

void CGPlayer_C::UpdateQuestStatus(const unsigned __int64 &guid) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_QUESTGIVER_STATUS_QUERY));
  msg.Put(guid);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::UpdateQuestStatus(CGUnit_C *unit) {
  UpdateQuestStatus(unit->GetGUID());
}

int OnReadItemResult(void *, NETMESSAGE msgID, unsigned long, CDataStore *msg) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->ReadItemResult(msgID, msg);
  }
  return 1;
}

int OnCancelCombat(void *, NETMESSAGE, unsigned long, CDataStore *) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->SetCombatMode(0);
  }
  return 1;
}

int AreaTriggerCheck(const void *eventData, void *arg) {
  CGPlayer_C *playerPtr = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));

  if (playerPtr) {
    int                worldId = CGPlayer_C::GetNewContinentID();
    NTempest::C3Vector pos;
    playerPtr->GetPosition(pos);

    if (currentAreaTrigger) {
      const AreaTriggerRec *rec = g_areaTriggerDB.GetRecord(currentAreaTrigger);
      FATALASSERT(rec);

      if (worldId == rec->m_ContinentID) {
        NTempest::C3Vector center(rec->m_x, rec->m_y, rec->m_z);
        float              radius = rec->m_radius * 1.1f;
        if ((center - pos).SquaredMag() < radius * radius) {
          goto reset_timer;
        }
      }

      currentAreaTrigger = 0;
    }

    for (int index = 0; index < g_areaTriggerDB.GetNumRecords(); ++index) {
      const AreaTriggerRec *rec = g_areaTriggerDB.GetRecordByIndex(index);
      FATALASSERT(rec);

      if (rec->m_ContinentID == worldId) {
        NTempest::C3Vector center(rec->m_x, rec->m_y, rec->m_z);
        if ((center - pos).SquaredMag() < rec->m_radius * rec->m_radius) {
          CDataStore msg;
          msg.Put(static_cast<unsigned int>(CMSG_AREATRIGGER));
          msg.Put(rec->m_ID);
          msg.Finalize();
          ClientServices_Send(&msg);
          currentAreaTrigger = rec->m_ID;
          break;
        }
      }
    }
  }

reset_timer:
  ClientSetTimer(100, AreaTriggerCheck, 0);
  return 1;
}

static void AreaTriggersInitialize() {
  s_areaTriggerCheck_TimerEvent = ClientSetTimer(100, AreaTriggerCheck, 0);
}

static void AreaTriggersShutdown() {
  if (s_areaTriggerCheck_TimerEvent) {
    ClientKillTimer(s_areaTriggerCheck_TimerEvent, AreaTriggerCheck, "AreaTriggerCheck");
    s_areaTriggerCheck_TimerEvent = 0;
  }
}

void PlayerClientInitialize() {
  ClientServices_SetMessageHandler(SMSG_MOUNTRESULT, OnPlayerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_DISMOUNTRESULT, OnPlayerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_GODMODE, OnPlayerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_INVENTORY_CHANGE_FAILURE, OnPlayerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_HEALSPELL_ON_PLAYER, OnPlayerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_HEALSPELL_ON_PLAYERS_PET, OnPlayerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_OPEN_CONTAINER, OnPlayerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_ITEM_PUSH_RESULT, OnPlayerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_LIST_INVENTORY, OnVendorEvent, 0);
  ClientServices_SetMessageHandler(SMSG_BUY_FAILED, OnVendorEvent, 0);
  ClientServices_SetMessageHandler(SMSG_BUY_ITEM, OnVendorEvent, 0);
  ClientServices_SetMessageHandler(SMSG_SELL_ITEM, OnVendorEvent, 0);
  ClientServices_SetMessageHandler(SMSG_LOOT_RESPONSE, OnLootEvent, 0);
  ClientServices_SetMessageHandler(SMSG_LOOT_RELEASE_RESPONSE, OnLootEvent, 0);
  ClientServices_SetMessageHandler(SMSG_LOOT_REMOVED, OnLootEvent, 0);
  ClientServices_SetMessageHandler(SMSG_LOOT_MONEY_NOTIFY, OnLootEvent, 0);
  ClientServices_SetMessageHandler(SMSG_LOOT_ITEM_NOTIFY, OnLootEvent, 0);
  ClientServices_SetMessageHandler(SMSG_LOOT_CLEAR_MONEY, OnLootEvent, 0);
  ClientServices_SetMessageHandler(MSG_SPLIT_MONEY, OnLootEvent, 0);
  ClientServices_SetMessageHandler(SMSG_LEARNED_SPELL, OnLearnedSpell, 0);
  ClientServices_SetMessageHandler(SMSG_SUPERCEDED_SPELL, OnSupercededSpell, 0);
  ClientServices_SetMessageHandler(SMSG_INITIAL_SPELLS, OnInitialSpells, 0);
  ClientServices_SetMessageHandler(SMSG_ACTION_BUTTONS, OnActionButtons, 0);
  ClientServices_SetMessageHandler(SMSG_PET_SPELLS, OnPetSpells, 0);
  ClientServices_SetMessageHandler(SMSG_GROUP_INVITE, OnGroupInvite, 0);
  ClientServices_SetMessageHandler(SMSG_GROUP_CANCEL, OnGroupCancel, 0);
  ClientServices_SetMessageHandler(SMSG_GROUP_DECLINE, OnGroupDecline, 0);
  ClientServices_SetMessageHandler(SMSG_GROUP_UNINVITE, OnGroupUninvite, 0);
  ClientServices_SetMessageHandler(SMSG_GROUP_SET_LEADER, OnGroupNewLeader, 0);
  ClientServices_SetMessageHandler(SMSG_GROUP_DESTROYED, OnGroupDestroy, 0);
  ClientServices_SetMessageHandler(SMSG_PARTY_COMMAND_RESULT, OnGroupCommandResult, 0);
  ClientServices_SetMessageHandler(SMSG_GROUP_LIST, OnGroupList, 0);
  ClientServices_SetMessageHandler(SMSG_QUESTGIVER_QUEST_LIST, OnQuestGiverEvent, 0);
  ClientServices_SetMessageHandler(SMSG_QUESTGIVER_QUEST_INVALID, OnQuestGiverEvent, 0);
  ClientServices_SetMessageHandler(SMSG_QUESTGIVER_QUEST_DETAILS, OnQuestGiverEvent, 0);
  ClientServices_SetMessageHandler(SMSG_QUESTGIVER_REQUEST_ITEMS, OnQuestGiverEvent, 0);
  ClientServices_SetMessageHandler(SMSG_QUESTGIVER_OFFER_REWARD, OnQuestGiverEvent, 0);
  ClientServices_SetMessageHandler(SMSG_QUESTGIVER_QUEST_COMPLETE, OnQuestGiverEvent, 0);
  ClientServices_SetMessageHandler(SMSG_QUESTGIVER_QUEST_FAILED, OnQuestGiverEvent, 0);
  ClientServices_SetMessageHandler(SMSG_QUESTGIVER_STATUS, OnQuestGiverEvent, 0);
  ClientServices_SetMessageHandler(SMSG_QUESTLOG_FULL, OnQuestGiverEvent, 0);
  ClientServices_SetMessageHandler(SMSG_TRAINER_LIST, OnTrainerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_TRAINER_BUY_FAILED, OnTrainerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_SET_PROFICIENCY, OnProficiency, 0);
  ClientServices_SetMessageHandler(SMSG_RESURRECT_REQUEST, OnResurrectRequest, 0);
  ClientServices_SetMessageHandler(SMSG_INSPECT, OnInspectNotify, 0);
  ClientServices_SetMessageHandler(SMSG_INITIALIZE_FACTIONS, OnFactionUpdate, 0);
  ClientServices_SetMessageHandler(SMSG_SET_FACTION_VISIBLE, OnFactionUpdate, 0);
  ClientServices_SetMessageHandler(SMSG_SET_FACTION_STANDING, OnFactionUpdate, 0);
  ClientServices_SetMessageHandler(SMSG_READ_ITEM_OK, OnReadItemResult, 0);
  ClientServices_SetMessageHandler(SMSG_READ_ITEM_FAILED, OnReadItemResult, 0);
  ClientServices_SetMessageHandler(SMSG_CANCEL_COMBAT, OnCancelCombat, 0);
  ClientServices_SetMessageHandler(SMSG_TAXINODE_STATUS, OnPlayerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_SHOWTAXINODES, OnPlayerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_ACTIVATETAXIREPLY, OnPlayerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_GUILD_INVITE, OnGuildInvite, 0);
  ClientServices_SetMessageHandler(SMSG_GUILD_DECLINE, OnGuildDecline, 0);
  ClientServices_SetMessageHandler(SMSG_GUILD_INFO, OnGuildInfo, 0);
  ClientServices_SetMessageHandler(SMSG_GUILD_ROSTER, OnGuildRoster, 0);
  ClientServices_SetMessageHandler(SMSG_GUILD_EVENT, OnGuildEvent, 0);
  ClientServices_SetMessageHandler(SMSG_GUILD_COMMAND_RESULT, OnGuildCommandResult, 0);
  ClientServices_SetMessageHandler(MSG_SAVE_GUILD_EMBLEM, OnGuildEmblemError, 0);
  ClientServices_SetMessageHandler(MSG_TABARDVENDOR_ACTIVATE, OnGuildEmblemActivate, 0);
  ClientServices_SetMessageHandler(SMSG_PETITION_SHOWLIST, OnNpcPetitionEvent, 0);
  ClientServices_SetMessageHandler(SMSG_PETITION_SHOW_SIGNATURES, OnNpcPetitionEvent, 0);
  ClientServices_SetMessageHandler(SMSG_PETITION_SIGN_RESULTS, OnNpcPetitionEvent, 0);
  ClientServices_SetMessageHandler(SMSG_TURN_IN_PETITION_RESULTS, OnNpcPetitionEvent, 0);
  ClientServices_SetMessageHandler(SMSG_UPDATE_AURA_DURATION, OnPlayerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_DEATH_NOTIFY, OnPlayerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_BINDPOINTUPDATE, OnPlayerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_BINDZONEREPLY, OnPlayerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_EMOTE, OnPlayEmote, 0);
  ClientServices_SetMessageHandler(SMSG_PLAYERBOUND, OnPlayerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_PLAYERBINDERROR, OnPlayerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_NEW_TAXI_PATH, OnPlayerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_PET_NAME_INVALID, OnPlayerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_EXPLORATION_EXPERIENCE, OnPlayerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_PARTY_MEMBER_STATS, HandlePartyMemberStats, 0);
  ClientServices_SetMessageHandler(SMSG_QUESTUPDATE_FAILED, OnQuestUpdate, 0);
  ClientServices_SetMessageHandler(SMSG_QUESTUPDATE_COMPLETE, OnQuestUpdate, 0);
  ClientServices_SetMessageHandler(SMSG_QUESTUPDATE_ADD_KILL, OnQuestUpdate, 0);
  ClientServices_SetMessageHandler(SMSG_QUESTUPDATE_ADD_ITEM, OnQuestUpdate, 0);
  ClientServices_SetMessageHandler(SMSG_QUEST_CONFIRM_ACCEPT, OnQuestConfirm, 0);
  ClientServices_SetMessageHandler(SMSG_FORCEACTIONSHOW, OnPlayerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_SHOW_BANK, OnPlayerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_BUY_BANK_SLOT_RESULT, OnPlayerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_LEVELUP_INFO, OnPlayerEvent, 0);
  ClientServices_SetMessageHandler(MSG_MINIMAP_PING, OnPlayerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_START_MIRROR_TIMER, OnMirrorTimerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_PAUSE_MIRROR_TIMER, OnMirrorTimerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_STOP_MIRROR_TIMER, OnMirrorTimerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_TRIGGER_CINEMATIC, OnPlayerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_PLAYER_MACRO, OnPlayerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_ITEM_TIME_UPDATE, OnItemEvent, 0);
  ClientServices_SetMessageHandler(SMSG_ITEM_ENCHANT_TIME_UPDATE, OnItemEvent, 0);
  ClientServices_SetMessageHandler(MSG_RANDOM_ROLL, OnPlayerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_FISH_NOT_HOOKED, OnPlayerEvent, 0);
  ClientServices_SetMessageHandler(SMSG_FISH_ESCAPED, OnPlayerEvent, 0);

  CGPlayer_C::InstallGMHandlers();
  ConsoleCommandRegister("bootme", BootMeHandler, DEBUG, "Tell server to forcefully boot us");
  ConsoleCommandRegister("invite", CCommand_Invite, GAME, "Invite a player to join your group");
  ConsoleCommandRegister("accept", CCommand_Accept, GAME, "Accept a group invitation");
  ConsoleCommandRegister("decline", CCommand_Decline, GAME, "Decline a group invitation");
  ConsoleCommandRegister("disband", CCommand_Disband, GAME, "Leave your group");
  ConsoleCommandRegister("newleader", CCommand_NewLeader, GAME, "Set a new group leader");
  ConsoleCommandRegister("uninvite", CCommand_Uninvite, GAME, "Kick out a member of your group");
  ConsoleCommandRegister("repopme", RepopPlayerHandler, GAME, "Repops you when you're dead");
  ConsoleCommandRegister("who", WhoCommandHandler, GAME, "Display who else is on the server");
  ConsoleCommandRegister("buy", BuyCommandHandler, GAME, "Buy an item from the last vendor queried");
  ConsoleCommandRegister("undressme", UndressMeHandler, GAME, "Removes all worn inventory items");
  ConsoleCommandRegister("godmode", GodmodeHandler, GAME, "prevents melee damage by monsters");
  ConsoleCommandRegister("levelup", CCommand_LevelUp, DEBUG, "raise one level");
  ConsoleCommandRegister("setfaction", CCommand_SetFaction, DEBUG, "set your reputation with a faction");
  ConsoleCommandRegister("acceptres", CCommand_AcceptRes, GAME, "Accept resurrect request");
  ConsoleCommandRegister("declineres", CCommand_DeclineRes, GAME, "Decline resurrect request");
  ConsoleCommandRegister("showpet", CCommand_ShowPet, DEBUG, "Displays GUID of pet");
  ConsoleCommandRegister("TaxiShowNodes", CCommand_TaxiShowNodes, DEBUG, "show all available taxi nodes");
  ConsoleCommandRegister("guildcreate", CCommand_GuildCreate, GAME, "Create a guild. Usage: 'guildcreate <guild name>'");
  ConsoleCommandRegister("TogglePVP", CCommand_TogglePVP, GAME, "Enable or disable PVP for yourself");
  ConsoleCommandRegister("cinematic", CCommand_Cinematic, DEBUG, "Start an in-game cinematic");
  ConsoleCommandRegister("CombatDebugForceActionOn", CCommand_ForceActionSet, DEBUG, "Value Name");
  ConsoleCommandRegister("CombatDebugForceActionOff", CCommand_ForceActionUnset, DEBUG, "Value Name");
  ConsoleCommandRegister("CombatDebugForceActionOtherOn", CCommand_ForceActionOnOtherSet, DEBUG, "Value Name");
  ConsoleCommandRegister("CombatDebugForceActionOtherOff", CCommand_ForceActionOnOtherUnset, DEBUG, "Value Name");
  ConsoleCommandRegister("CombatDebugShowFlags", CCommand_ForceActionShowFlags, DEBUG, "Value Name");
  ConsoleCommandRegister("forceanim", CCommand_ForceMonsterAnim, DEBUG, "Force a monster animation");
  ConsoleCommandRegister("resetanim", CCommand_ResetMonsterAnim, DEBUG, "Force a monster animation");
  ConsoleCommandRegister("DumpDeathHoldLogs", CCommand_DumpDeathHoldLogs, DEBUG, "Value Name");

  memset(s_playerProficiencies, 0, sizeof(s_playerProficiencies));
  s_tempCombatModeCooldown = 0;
  s_attackBreakTimer = 0;
  s_combatModeTimer = 0;
  s_enableDeathHoldLog = 1;
  s_renderPlayer = 1;
  g_combatModeMaxDistance =
      CVar::Register("CombatModeMaxDistance", "Specifies the range outside of which combat mode is impossible", 0, "30.0f", 0, 3, false, 0);
  s_namePlateRenderOwn = CVar::Register("UnitNameRenderOwn", "Toggles rendering of local player's nameplate", 0, "0", 0, 1, false, 0);
  s_resurrectOffer = 0;
  PlayerInitializeSounds();
  AreaTriggersInitialize();
}

void PlayerClientShutdown() {
  ClientServices_ClearMessageHandler(SMSG_GODMODE);
  ClientServices_ClearMessageHandler(SMSG_INVENTORY_CHANGE_FAILURE);
  ClientServices_ClearMessageHandler(SMSG_HEALSPELL_ON_PLAYER);
  ClientServices_ClearMessageHandler(SMSG_HEALSPELL_ON_PLAYERS_PET);
  ClientServices_ClearMessageHandler(SMSG_OPEN_CONTAINER);
  ClientServices_ClearMessageHandler(SMSG_ITEM_PUSH_RESULT);
  ClientServices_ClearMessageHandler(SMSG_LIST_INVENTORY);
  ClientServices_ClearMessageHandler(SMSG_BUY_FAILED);
  ClientServices_ClearMessageHandler(SMSG_BUY_ITEM);
  ClientServices_ClearMessageHandler(SMSG_SELL_ITEM);
  ClientServices_ClearMessageHandler(SMSG_LOOT_RESPONSE);
  ClientServices_ClearMessageHandler(SMSG_LOOT_RELEASE_RESPONSE);
  ClientServices_ClearMessageHandler(SMSG_LOOT_REMOVED);
  ClientServices_ClearMessageHandler(SMSG_LOOT_MONEY_NOTIFY);
  ClientServices_ClearMessageHandler(SMSG_LOOT_ITEM_NOTIFY);
  ClientServices_ClearMessageHandler(SMSG_LOOT_CLEAR_MONEY);
  ClientServices_ClearMessageHandler(MSG_SPLIT_MONEY);
  ClientServices_ClearMessageHandler(SMSG_LEARNED_SPELL);
  ClientServices_ClearMessageHandler(SMSG_SUPERCEDED_SPELL);
  ClientServices_ClearMessageHandler(SMSG_INITIAL_SPELLS);
  ClientServices_ClearMessageHandler(SMSG_ACTION_BUTTONS);
  ClientServices_ClearMessageHandler(SMSG_PET_SPELLS);
  ClientServices_ClearMessageHandler(SMSG_GROUP_INVITE);
  ClientServices_ClearMessageHandler(SMSG_GROUP_CANCEL);
  ClientServices_ClearMessageHandler(SMSG_GROUP_DECLINE);
  ClientServices_ClearMessageHandler(SMSG_GROUP_UNINVITE);
  ClientServices_ClearMessageHandler(SMSG_GROUP_SET_LEADER);
  ClientServices_ClearMessageHandler(SMSG_GROUP_DESTROYED);
  ClientServices_ClearMessageHandler(SMSG_PARTY_COMMAND_RESULT);
  ClientServices_ClearMessageHandler(SMSG_GROUP_LIST);
  ClientServices_ClearMessageHandler(SMSG_QUESTGIVER_QUEST_LIST);
  ClientServices_ClearMessageHandler(SMSG_QUESTGIVER_QUEST_INVALID);
  ClientServices_ClearMessageHandler(SMSG_QUESTGIVER_QUEST_DETAILS);
  ClientServices_ClearMessageHandler(SMSG_QUESTGIVER_REQUEST_ITEMS);
  ClientServices_ClearMessageHandler(SMSG_QUESTGIVER_OFFER_REWARD);
  ClientServices_ClearMessageHandler(SMSG_QUESTGIVER_QUEST_COMPLETE);
  ClientServices_ClearMessageHandler(SMSG_QUESTGIVER_QUEST_FAILED);
  ClientServices_ClearMessageHandler(SMSG_QUESTGIVER_STATUS);
  ClientServices_ClearMessageHandler(SMSG_QUESTLOG_FULL);
  ClientServices_ClearMessageHandler(SMSG_TRAINER_LIST);
  ClientServices_ClearMessageHandler(SMSG_TRAINER_BUY_FAILED);
  ClientServices_ClearMessageHandler(SMSG_SET_PROFICIENCY);
  ClientServices_ClearMessageHandler(SMSG_RESURRECT_REQUEST);
  ClientServices_ClearMessageHandler(SMSG_INSPECT);
  ClientServices_ClearMessageHandler(SMSG_INITIALIZE_FACTIONS);
  ClientServices_ClearMessageHandler(SMSG_SET_FACTION_VISIBLE);
  ClientServices_ClearMessageHandler(SMSG_SET_FACTION_STANDING);
  ClientServices_ClearMessageHandler(SMSG_FORCE_SPEED_CHANGE);
  ClientServices_ClearMessageHandler(SMSG_FORCE_SWIM_SPEED_CHANGE);
  ClientServices_ClearMessageHandler(SMSG_FORCE_MOVE_ROOT);
  ClientServices_ClearMessageHandler(SMSG_FORCE_MOVE_UNROOT);
  ClientServices_ClearMessageHandler(SMSG_READ_ITEM_OK);
  ClientServices_ClearMessageHandler(SMSG_READ_ITEM_FAILED);
  ClientServices_ClearMessageHandler(SMSG_TAXINODE_STATUS);
  ClientServices_ClearMessageHandler(SMSG_SHOWTAXINODES);
  ClientServices_ClearMessageHandler(SMSG_ACTIVATETAXIREPLY);
  ClientServices_ClearMessageHandler(SMSG_CANCEL_COMBAT);
  ClientServices_ClearMessageHandler(SMSG_GUILD_INVITE);
  ClientServices_ClearMessageHandler(SMSG_GUILD_DECLINE);
  ClientServices_ClearMessageHandler(SMSG_GUILD_INFO);
  ClientServices_ClearMessageHandler(SMSG_GUILD_ROSTER);
  ClientServices_ClearMessageHandler(SMSG_GUILD_EVENT);
  ClientServices_ClearMessageHandler(SMSG_GUILD_COMMAND_RESULT);
  ClientServices_ClearMessageHandler(MSG_SAVE_GUILD_EMBLEM);
  ClientServices_ClearMessageHandler(MSG_TABARDVENDOR_ACTIVATE);
  ClientServices_ClearMessageHandler(SMSG_PETITION_SHOWLIST);
  ClientServices_ClearMessageHandler(SMSG_PETITION_SHOW_SIGNATURES);
  ClientServices_ClearMessageHandler(SMSG_PETITION_SIGN_RESULTS);
  ClientServices_ClearMessageHandler(SMSG_TURN_IN_PETITION_RESULTS);
  ClientServices_ClearMessageHandler(SMSG_MOUNTRESULT);
  ClientServices_ClearMessageHandler(SMSG_DISMOUNTRESULT);
  ClientServices_ClearMessageHandler(SMSG_UPDATE_AURA_DURATION);
  ClientServices_ClearMessageHandler(SMSG_DEATH_NOTIFY);
  ClientServices_ClearMessageHandler(SMSG_BINDPOINTUPDATE);
  ClientServices_ClearMessageHandler(SMSG_BINDZONEREPLY);
  ClientServices_ClearMessageHandler(SMSG_EMOTE);
  ClientServices_ClearMessageHandler(SMSG_PLAYERBOUND);
  ClientServices_ClearMessageHandler(SMSG_PLAYERBINDERROR);
  ClientServices_ClearMessageHandler(SMSG_NEW_TAXI_PATH);
  ClientServices_ClearMessageHandler(SMSG_PET_NAME_INVALID);
  ClientServices_ClearMessageHandler(SMSG_PARTY_MEMBER_STATS);
  ClientServices_ClearMessageHandler(SMSG_QUESTUPDATE_FAILED);
  ClientServices_ClearMessageHandler(SMSG_QUESTUPDATE_COMPLETE);
  ClientServices_ClearMessageHandler(SMSG_QUESTUPDATE_ADD_KILL);
  ClientServices_ClearMessageHandler(SMSG_QUESTUPDATE_ADD_ITEM);
  ClientServices_ClearMessageHandler(SMSG_QUEST_CONFIRM_ACCEPT);
  ClientServices_ClearMessageHandler(SMSG_FORCEACTIONSHOW);
  ClientServices_ClearMessageHandler(SMSG_SHOW_BANK);
  ClientServices_ClearMessageHandler(SMSG_BUY_BANK_SLOT_RESULT);
  ClientServices_ClearMessageHandler(SMSG_LEVELUP_INFO);
  ClientServices_ClearMessageHandler(MSG_MINIMAP_PING);
  ClientServices_ClearMessageHandler(SMSG_START_MIRROR_TIMER);
  ClientServices_ClearMessageHandler(SMSG_PAUSE_MIRROR_TIMER);
  ClientServices_ClearMessageHandler(SMSG_STOP_MIRROR_TIMER);
  ClientServices_ClearMessageHandler(SMSG_TRIGGER_CINEMATIC);
  ClientServices_ClearMessageHandler(SMSG_PLAYER_MACRO);
  ClientServices_ClearMessageHandler(SMSG_ITEM_TIME_UPDATE);
  ClientServices_ClearMessageHandler(SMSG_ITEM_ENCHANT_TIME_UPDATE);
  ClientServices_ClearMessageHandler(SMSG_EXPLORATION_EXPERIENCE);
  ClientServices_ClearMessageHandler(MSG_RANDOM_ROLL);
  ClientServices_ClearMessageHandler(SMSG_FISH_NOT_HOOKED);
  ClientServices_ClearMessageHandler(SMSG_FISH_ESCAPED);

  CGPlayer_C::UninstallGMHandlers();
  ConsoleCommandUnregister("CombatDebugForceActionOn");
  ConsoleCommandUnregister("CombatDebugForceActionOff");
  ConsoleCommandUnregister("CombatDebugForceActionOtherOn");
  ConsoleCommandUnregister("CombatDebugForceActionOtherOff");
  ConsoleCommandUnregister("CombatDebugShowFlags");
  ConsoleCommandUnregister("dumpTexture");
  ConsoleCommandUnregister("bootme");
  ConsoleCommandUnregister("setguild");
  ConsoleCommandUnregister("invite");
  ConsoleCommandUnregister("cancel");
  ConsoleCommandUnregister("accept");
  ConsoleCommandUnregister("decline");
  ConsoleCommandUnregister("disband");
  ConsoleCommandUnregister("newleader");
  ConsoleCommandUnregister("uninvite");
  ConsoleCommandUnregister("grouplist");
  ConsoleCommandUnregister("repopme");
  ConsoleCommandUnregister("buy");
  ConsoleCommandUnregister("undressme");
  ConsoleCommandUnregister("godmode");
  ConsoleCommandUnregister("levelup");
  ConsoleCommandUnregister("setfaction");
  ConsoleCommandUnregister("ignorelevel");
  ConsoleCommandUnregister("acceptres");
  ConsoleCommandUnregister("declineres");
  ConsoleCommandUnregister("showpet");
  ConsoleCommandUnregister("TaxiShowNodes");
  ConsoleCommandUnregister("guildcreate");
  ConsoleCommandUnregister("TogglePVP");
  ConsoleCommandUnregister("cinematic");
  ConsoleCommandUnregister("forceanim");
  ConsoleCommandUnregister("resetanim");
  ConsoleCommandUnregister("DumpDeathHoldLogs");

  if (ClntObjMgrGetPlayerType() != PLAYER_BOT) {
    if (s_attackBreakTimer) {
      ClientKillTimer(s_attackBreakTimer, reinterpret_cast<CLIENTTIMERHANDLER>(PlayerAttackBreakHandler), "PlayerAttackBreakHandler");
    }
    if (s_combatModeTimer) {
      ClientKillTimer(s_combatModeTimer, reinterpret_cast<CLIENTTIMERHANDLER>(PlayerCombatModeHandler), "PlayerCombatModeHandler");
    }
    s_attackBreakTimer = 0;
    s_combatModeTimer = 0;
  }

  PlayerShutdownSounds();
  AreaTriggersShutdown();
}

int Player_C_AppFocusMovementHandler(int focus) {
  if (!focus) {
    unsigned long    eventTime = OsGetAsyncTimeMs();
    unsigned __int64 guid = ClntObjMgrGetActivePlayer();
    CGPlayer_C      *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
    if (player) {
      CMovementStatus status;
      player->m_move.GetMoveStatus(&status);
      unsigned int moveFlags = status.moveFlags;
      if ((moveFlags & 0x01000000) ||
          ((player->GetType() & TYPE_PLAYER) && !status.transport && ((moveFlags & 2) || !(moveFlags & 0x00C00004)) && !(moveFlags & 1)))
      {
        player->OnMoveStopLocal(eventTime);
        player->OnStrafeStopLocal(eventTime);
      }
    }
  }

  return 1;
}

static int CountWeaponItemSubclasses(int *number) {
  int maxSubclass = 0;
  int found = 0;

  ASSERT(number);

  for (int index = g_itemSubClassDB.GetNumRecords() - 1; index >= 0; --index) {
    const ItemSubClassRec *rec = g_itemSubClassDB.GetRecordByIndex(index);
    if (rec->m_classID == 2) {
      found = 1;
      if (maxSubclass <= rec->m_subClassID) {
        maxSubclass = rec->m_subClassID;
      }
    }
  }

  *number = maxSubclass + 1;
  return found;
}

static int FindFirstSetBit(unsigned int field, int *whichBitSet) {
  int result = field && !((field - 1) & field);
  int firstBit = 0x7FFFFFFF;

  for (unsigned int index = 0; index < 32; ++index) {
    if ((field & (1u << index)) && static_cast<int>(index) < firstBit) {
      firstBit = index;
    }
  }

  if (!field) {
    firstBit = -1;
  }

  if (whichBitSet) {
    *whichBitSet = firstBit;
  }

  return result;
}

static void InitializeWeaponSubclassSpells() {
  int subclass;
  int numSubclasses;

  if (!CountWeaponItemSubclasses(&numSubclasses)) {
    return;
  }

  s_weaponSubclassSpells.SetCount(numSubclasses);
  for (subclass = 0; subclass < numSubclasses; ++subclass) {
    s_weaponSubclassSpells[subclass] = 0;
  }

  s_defenseSkillID = 0;
  for (int index = g_spellDB.GetNumRecords() - 1; index >= 0; --index) {
    const SpellRec *rec = g_spellDB.GetRecordByIndex(index);
    ASSERT(rec);

    if (!(rec->m_attributes & 0x40)) {
      continue;
    }

    if (!s_defenseSkillID && rec->m_effect[0] == 26) {
      s_defenseSkillID = rec->m_ID;
    }

    if (rec->m_equippedItemClass == 2) {
      if (FindFirstSetBit(rec->m_equippedItemSubclass, &subclass) && subclass < numSubclasses) {
        s_weaponSubclassSpells[subclass] = rec->m_ID;
      }
    }
  }
}

void CGPlayer_C::Initialize() {
  s_lastVendorListReceived = 0;
  s_giftWrapItem = 0;
  s_bindSaved = 0;
  InitializeWeaponSubclassSpells();
}

unsigned int CGPlayer_C::CanTrack(CGUnit_C *unit) {
  if (unit->GetUnitData()->flags & 2) {
    return 1;
  }

  int creatureType = unit->GetCreatureType();
  if (!creatureType) {
    return 0;
  }

  return (GetCreatureTracking() & (1 << (creatureType - 1))) != 0;
}

unsigned int CGPlayer_C::CanTrack(CGGameObject_C *object) {
  unsigned int trackMask = GetResourceTracking();
  const LockRec      *lock = object->GetLockRec();
  if (!lock) {
    return 0;
  }

  for (unsigned int index = 0; index < 4; ++index) {
    if (lock->m_Type[index] == 2 && (trackMask & (1 << (lock->m_Index[index] - 1)))) {
      return 1;
    }
  }
  return 0;
}

void CGPlayer_C::Shutdown() {
  s_initialSpells.Clear();
  s_pendingSwaps.Clear();
  s_weaponSubclassSpells.Clear();
  s_lastBinderID = 0;
}

void CGPlayer_C::AcceptGroup() {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_GROUP_ACCEPT));
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::DeclineGroup() {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_GROUP_DECLINE));
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::LeaveGroup() {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_GROUP_DISBAND));
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::SetLootMethod(LOOT_METHOD method, unsigned __int64 master) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_LOOT_METHOD));
  msg.Put(static_cast<unsigned int>(method));
  msg.Put(master);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::AcceptGuild() {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_GUILD_ACCEPT));
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::DeclineGuild() {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_GUILD_DECLINE));
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::DeleteWornItems() {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_UNDRESSPLAYER));
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::AddKnownSpell(int spellID, int slot, int learned, int addToBook) {
  if (spellID > g_spellDB.GetMaxID()) {
    SysMsgPrintf(SYSMSG_ERROR, 2, "NOSPELLIDFOUND|%d", spellID);
    return;
  }

  if (addToBook) {
    CGSpellBook::AddKnownSpell(spellID, slot, learned);
  }
  CGClassTrainer::RefreshList();

  const SpellRec *spell = g_spellDB.GetRecord(spellID);
  if (!spell) {
    return;
  }

  if (spell->m_attributes & 0x20) {
    const SkillLineAbilityRec *ability = SpellTableLookupAbility(GetUnitData()->race, GetUnitData()->classId, spellID);
    const SkillLineRec        *skillLine = ability ? g_skillLineDB.GetRecord(ability->m_skillLine) : 0;
    if (skillLine) {
      HASHKEY_NONE                               hashKey;
      TRADESKILLLINE *tradeSkillLine = m_tradeSkillLines.Ptr(skillLine->m_ID, hashKey);
      if (!tradeSkillLine) {
        tradeSkillLine = m_tradeSkillLines.New(skillLine->m_ID, hashKey, 0, 0);
      }
      *tradeSkillLine->spells.New() = spellID;
    }
    CGTradeSkillInfo::RefreshList(1);
    return;
  }

  SPELL_CAST_UI_TYPE craftType = static_cast<SPELL_CAST_UI_TYPE>(spell->m_castUI);
  if (craftType > SPELL_CAST_UI_NONE) {
    FATALASSERT(craftType < NUM_SPELL_CAST_UI_TYPES);
    unsigned int          index;
    for (index = 0; index < m_craftSpells[craftType].Count(); ++index) {
      if (m_craftSpells[craftType][index] == spellID) {
        break;
      }
    }
    if (index == m_craftSpells[craftType].Count()) {
      *m_craftSpells[craftType].New() = spellID;
    }
    CGCraftInfo::RefreshList();
  } else if (spell->m_effect[0] == 47) {
    craftType = static_cast<SPELL_CAST_UI_TYPE>(spell->m_effectMiscValue[0]);
    FATALASSERT(craftType < NUM_SPELL_CAST_UI_TYPES);
    m_craftActivators[craftType] = spellID;
  }
}

void CGPlayer_C::DelKnownSpell(int spellID) {
  int found = 0;

  if (spellID > g_spellDB.GetMaxID()) {
    SysMsgPrintf(SYSMSG_ERROR, 2, "NOSPELLIDFOUND|%d", spellID);
    return;
  }

  const SpellRec *spell = g_spellDB.GetRecord(spellID);
  if (spell && spell->m_castUI > SPELL_CAST_UI_NONE) {
    SPELL_CAST_UI_TYPE craftType = static_cast<SPELL_CAST_UI_TYPE>(spell->m_castUI);
    FATALASSERT(craftType < NUM_SPELL_CAST_UI_TYPES);
    unsigned int count = m_craftSpells[craftType].Count();
    for (unsigned int index = 0; index < count; ++index) {
      if (found) {
        m_craftSpells[craftType][index - 1] = m_craftSpells[craftType][index];
      } else if (m_craftSpells[craftType][index] == spellID) {
        found = 1;
      }
    }
    if (found) {
      m_craftSpells[craftType].SetCount(count - 1);
      CGCraftInfo::RefreshList();
    }
  }

  CGSpellBook::DelKnownSpell(spellID);
  CGActionBar::RemoveSpell(spellID);
}

ITEMEXPIRATION *CGPlayer_C::GetPendingItemExpirationNode(const unsigned __int64 &itemGUID) {
  CHashKeyGUID    key(itemGUID);
  unsigned int    hash = static_cast<unsigned int>(itemGUID);
  ITEMEXPIRATION *itemNode = s_pendingItemExpirations.Ptr(hash, key);
  if (!itemNode) {
    itemNode = s_pendingItemExpirations.New(hash, key, 0, 0);
    itemNode->timeLeft = 0;
    memset(itemNode->enchantmentTimeLeft, 0, sizeof(itemNode->enchantmentTimeLeft));
  }
  return itemNode;
}

TSGrowableArray<int> *CGPlayer_C::GetTradeSkills(int skillLine) {
  HASHKEY_NONE                               hashKey;
  TRADESKILLLINE *line = m_tradeSkillLines.Ptr(skillLine, hashKey);
  return line ? &line->spells : 0;
}

TSGrowableArray<int> *CGPlayer_C::GetCraftSkills(SPELL_CAST_UI_TYPE type) {
  if (type <= SPELL_CAST_UI_NONE || type >= NUM_SPELL_CAST_UI_TYPES) {
    return 0;
  }
  return &m_craftSpells[type];
}

int CGPlayer_C::GetCraftSkillActivator(SPELL_CAST_UI_TYPE type) const {
  if (static_cast<unsigned int>(type) >= NUM_SPELL_CAST_UI_TYPES) {
    return 0;
  }
  return m_craftActivators[type];
}

int CGPlayer_C::GetSkillIndex(int skillID) const {
  unsigned int   index;
  for (index = 0; index < 64; ++index) {
    if (GetMirrorSkillID(index) == skillID) {
      break;
    }
  }
  return index == 64 ? -1 : static_cast<int>(index);
}

int CGPlayer_C::GetSkillRank(int skillID) const {
  int index = GetSkillIndex(skillID);
  if (index < 0) {
    return 0;
  }
  int rank = GetMirrorSkillRank(index) + GetMirrorSkillModifier(index);
  return rank < 0 ? 0 : rank;
}

int CGPlayer_C::ValidateSlot(unsigned int slotID, unsigned __int64 cursorItem) {
  CGObject_C *object = ClntObjMgrObjectPtr(cursorItem, __FILE__, __LINE__);
  if (!object) {
    return 0;
  }

  CGItem_C *item = static_cast<CGItem_C *>(object);
  if (!(object->GetType() & TYPE_ITEM) ||
      (slotID >= 19 && slotID <= 22) ||
      (slotID >= 63 && slotID <= 68)) {
    return item->CanGoInSlot(slotID);
  }

  CGBag_C *bag = object->GetBag();
  for (unsigned int slot = 0; slot < bag->NumSlots(); ++slot) {
    if (bag->GetItem(slot)) {
      return 0;
    }
  }
  return item->CanGoInSlot(slotID);
}

UNITAFFILIATION CGPlayer_C::GetGUIDAffiliation(unsigned __int64 unit) const {
  UNITAFFILIATION affiliation = CGUnit_C::GetGUIDAffiliation(unit);
  if (affiliation == AFFILIATION_OTHER && CGGameUI::IsPartyMember(unit)) {
    affiliation = AFFILIATION_PARTYMEMBER;
  }
  return affiliation;
}

int CGPlayer_C::GetSpellRank(int spellID) const {
  SpellRec *spell = g_spellDB.GetRecord(spellID);
  if (!spell) {
    return 0;
  }

  const SkillLineAbilityRec *ability = SpellTableLookupAbility(GetUnitData()->race, GetUnitData()->classId, spellID);
  if (!ability) {
    return 0;
  }

  int rank = GetSkillRank(ability->m_skillLine);
  if (spell->m_attributes & 0x406) {
    int weaponSpell;
    if (spell->m_attributes & 2) {
      const VirtualItemInfo *item = GetVirtualItem(2, 0);
      if (!item) {
        return 0;
      }
      weaponSpell = s_weaponSubclassSpells[item->m_subclassID];
    } else {
      weaponSpell = GetWeaponSpell(COMBAT_MAINHAND);
    }
    rank = (rank + GetSpellRank(weaponSpell)) / 2;
  }

  if (spell->m_maxLevel > 0 && rank >= 5 * spell->m_maxLevel) {
    rank = 5 * spell->m_maxLevel;
  }
  return rank < 0 ? 0 : rank;
}

int CGPlayer_C::GetWeaponSpell(COMBATHAND hand) const {
  if (hand >= NUMHANDS) {
    return 0;
  }

  const VirtualItemInfo *item = GetVirtualItem(static_cast<unsigned int>(hand), 0);
  unsigned int subclass = item ? item->m_subclassID : ClientDBGetUnarmedWeapon();
  if (item && item->m_classID != 2) {
    return 0;
  }
  return subclass < s_weaponSubclassSpells.Count() ? s_weaponSubclassSpells[subclass] : 0;
}

unsigned int CGPlayer_C::GetNewContinentID() {
  return ClntObjMgrGetMapID();
}

int CGPlayer_C::OnAttackBreakHandler() {
  unsigned __int64 target = GetLocalTarget();
  if (!target) {
    return 0;
  }

  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(target, __FILE__, __LINE__));
  if (!unitPtr) {
    return 0;
  }

  FATALASSERT(m_flags & 0x00000010);
  if (m_flags & 0x00000020) {
    NTempest::C3Vector there;
    NTempest::C3Vector here;
    GetPosition(here);
    unitPtr->GetPosition(there);
    if ((here - there).SquaredMag() >= s_attackBreakDistanceSquared) {
      return 0;
    }
  }

  return 1;
}

void CGPlayer_C::LootMoney() {
  if (m_lootingUnit) {
    CDataStore msg;
    msg.Put(static_cast<unsigned int>(CMSG_LOOT_MONEY));
    msg.Finalize();
    ClientServices_Send(&msg);
  }
}

int CGPlayer_C::CanUseItem(const ItemStats *stats, GAME_ERROR_TYPE &reason) {
  reason = GERR_NUM_TYPES;
  if (!stats) {
    reason = static_cast<GAME_ERROR_TYPE>(17);
    return 0;
  }

  if (stats->m_requiredLevel > m_unit->level) {
    reason = static_cast<GAME_ERROR_TYPE>(1);
    return 0;
  }
  if (!(stats->m_allowableClass & (1 << (m_unit->classId - 1))) || !(stats->m_allowableRace & (1 << (m_unit->race - 1)))) {
    reason = static_cast<GAME_ERROR_TYPE>(3);
    return 0;
  }

  unsigned int proficiency = GetProficiency(static_cast<unsigned char>(stats->m_class));
  if (proficiency && !(proficiency & (1 << stats->m_subclass))) {
    reason = static_cast<GAME_ERROR_TYPE>(4);
    return 0;
  }
  if (stats->m_requiredSkill && GetSkillRank(stats->m_requiredSkill) < stats->m_requiredSkillRank) {
    reason = static_cast<GAME_ERROR_TYPE>(2);
    return 0;
  }
  return 1;
}

void CGPlayer_C::QueryQuest(const unsigned __int64 &questGiver, int questID) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_QUESTGIVER_QUERY_QUEST));
  msg.Put(questGiver);
  msg.Put(questID);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::AcceptQuest(const unsigned __int64 &questGiver, int questID) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_QUESTGIVER_ACCEPT_QUEST));
  msg.Put(questGiver);
  msg.Put(questID);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::CompleteQuest(const unsigned __int64 &questGiver, int questID) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_QUESTGIVER_COMPLETE_QUEST));
  msg.Put(questGiver);
  msg.Put(questID);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::GiveQuestItems(const unsigned __int64 &questGiver, int questID) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_QUESTGIVER_REQUEST_REWARD));
  msg.Put(questGiver);
  msg.Put(questID);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::GetQuestReward(const unsigned __int64 &questGiver, int questID, int itemChoice) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_QUESTGIVER_CHOOSE_REWARD));
  msg.Put(questGiver);
  msg.Put(questID);
  msg.Put(itemChoice);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::CancelQuest(const unsigned __int64 &questGiver) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_QUESTGIVER_CANCEL));
  msg.Put(questGiver);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::QuestLogRemoveQuest(int entry) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_QUESTLOG_REMOVE_QUEST));
  msg.Put(static_cast<unsigned char>(entry));
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::QuestLogSwapQuest(int entry1, int entry2) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_QUESTLOG_SWAP_QUEST));
  msg.Put(static_cast<unsigned char>(entry1));
  msg.Put(static_cast<unsigned char>(entry2));
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::TrainerBuySpell(const unsigned __int64 &trainer, int spellID) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_TRAINER_BUY_SPELL));
  msg.Put(trainer);
  msg.Put(spellID);
  msg.Finalize();
  ClientServices_Send(&msg);
}

int QuestUpdateProc(unsigned __int64 guid, void *__formal) {
  CGObject_C *object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
  if (object) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player && (object->IsA(TYPE_UNIT) || object->IsA(TYPE_GAMEOBJECT)) && !object->IsA(TYPE_ITEM) &&
        (!object->IsA(TYPE_GAMEOBJECT) || ((static_cast<CGGameObject_C *>(object)->GameObject()->m_flags & 4) &&
                                           static_cast<CGGameObject_C *>(object)->ObjectReaction(player) != UNIT_REACTION_HOSTILE)) &&
        (!object->IsA(TYPE_UNIT) || ((static_cast<CGUnit_C *>(object)->GetUnitData()->npcFlags & 2) &&
                                     static_cast<CGUnit_C *>(object)->UnitReaction(player) > UNIT_REACTION_HOSTILE)))
    {
      player->UpdateQuestStatus(guid);
    }
  }
  return 1;
}

void CGPlayer_C::UpdateQuestStatusAll() {
  ClntObjMgrEnumVisibleObjects(QuestUpdateProc, 0);
}

void CGPlayer_C::UpdateTaxiStatus(CGUnit_C *unit) {
  if (unit->UnitReaction(this) > UNIT_REACTION_HOSTILE) {
    CDataStore msg;
    msg.Put(static_cast<unsigned int>(CMSG_TAXINODE_STATUS_QUERY));
    msg.Put(unit->GetGUID());
    msg.Finalize();
    ClientServices_Send(&msg);
  }
}

int TaxiUpdateProc(unsigned __int64 guid, void *param) {
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (unit && (unit->GetType() & TYPE_UNIT) && (unit->GetUnitData()->npcFlags & 4)) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      player->UpdateTaxiStatus(unit);
    }
  }
  return 1;
}

void CGPlayer_C::UpdateTaxiStatusAll() {
  ClntObjMgrEnumVisibleObjects(TaxiUpdateProc, 0);
}

void CGPlayer_C::UpdateBindStatus(CGUnit_C *unit) {
  if (unit->UnitReaction(this) > UNIT_REACTION_HOSTILE && (unit->GetUnitData()->npcFlags & 0x10)) {
    NTempest::C3Vector position;
    unit->GetPosition(position);
    unit->UpdateInteractIcon(static_cast<INTERACTICONTYPE>(DeathBindDistanceCompare(position) ? INTERACTICON_NONE : INTERACTICON_BINDER));
  }
}

int BindUpdateProc(unsigned __int64 guid, void *param) {
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (unit && (unit->GetType() & TYPE_UNIT) && (unit->GetUnitData()->npcFlags & 0x10)) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      player->UpdateBindStatus(unit);
    }
  }
  return 1;
}

void CGPlayer_C::UpdateBindStatusAll() {
  ClntObjMgrEnumVisibleObjects(BindUpdateProc, 0);
}

void CGPlayer_C::TrySheathingWeapon() {
  if (!(m_flags & 0x400)) {
    unsigned char playerAnimState = m_unit->weaponMode;
    if (playerAnimState != 1 || (m_lastWeaponModeSent != -1 && m_lastWeaponModeSent != 1)) {
      SheatheWeapon(1);
    }
  }
}

void CGPlayer_C::ToggleSheathe(unsigned int ignoreAnim) {
  if (m_unit->health <= 0 || m_currentTorsoAnimState == 46 || m_castingSpell)
  {
    return;
  }

  if (ignoreAnim || !SheatheAnimPlaying()) {
    unsigned char weaponMode = m_unit->weaponMode;
    if (weaponMode == WEAPONMODE_MELEE || weaponMode == WEAPONMODE_RANGED) {
      SetWeaponMode(WEAPONMODE_SHEATHED);
    } else {
      SetWeaponMode(WEAPONMODE_MELEE);
      if (m_flags & 0x400) {
        SetCombatMode(0);
      }
    }

    static const unsigned char s_standStateStartsSheathe[12] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    if (s_standStateStartsSheathe[m_unit->standState]) {
      MaybeStartSheatheAnim();
    }
  }
}

int CGPlayer_C::OnLootResponse(unsigned int eventTime, CDataStore *msg) {
  CDataStore       lootRelease;
  unsigned __int64 objectGUID;
  CGObject_C      *lootobject;
  unsigned int     coins;
  unsigned int     accquired = 0;
  unsigned int     reason = 0;
  unsigned int     slot = 0;
  unsigned int     count = 0;

  msg->Get(objectGUID);
  msg->Get(*reinterpret_cast<unsigned char *>(&accquired));

  if ((!m_lootingUnitSent || objectGUID != m_lootingUnitSent) &&
      (m_lootingUnitSent || (accquired != LOOT_ACQUIRE_PICKPOCKET && accquired != LOOT_ACQUIRE_FISHING)))
  {
    if (accquired) {
      lootRelease.Put(static_cast<unsigned int>(CMSG_LOOT_RELEASE));
      lootRelease.Put(objectGUID);
      lootRelease.Finalize();
      ClientServices_Send(&lootRelease);
    }
    m_lootingUnitSent = 0;
    return 1;
  }

  if (accquired) {
    msg->Get(coins);
    msg->Get(*reinterpret_cast<unsigned char *>(&count));
    if (count > 16) {
      count = 16;
    }

    lootobject = ClntObjMgrObjectPtr(objectGUID, __FILE__, __LINE__);
    if (lootobject) {
      m_lootingUnit = objectGUID;
      s_numLootItems = count;
      memset(s_lootItems, 0, sizeof(s_lootItems));
      for (unsigned int index = 0; index < count; ++index) {
        msg->Get(*reinterpret_cast<unsigned char *>(&slot));
        msg->Get(s_lootItems[slot].m_itemID);
        msg->Get(s_lootItems[slot].m_quantity);
        msg->Get(s_lootItems[slot].m_displayID);
      }
      CGLootInfo::SetObject(lootobject, coins, static_cast<LOOT_ACQUIRE>(accquired));
    } else {
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(118));
    }
    return 1;
  }

  msg->Get(*reinterpret_cast<unsigned char *>(&reason));
  switch (reason) {
    case 4:
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(117));
      break;
    case 5:
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(119));
      break;
    case 6:
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(116));
      break;
    case 8:
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(120));
      break;
    case 9:
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(121));
      break;
    default:
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(118));
      break;
  }
  m_lootingUnitSent = 0;
  if (GetPlayerAnimState() == 44) {
    UpdateBaseAnimation(0, 0);
  }
  return 1;
}

unsigned int CGPlayer_C::GetLootItem(unsigned int slot) {
  FATALASSERT(slot < 16);
  return s_lootItems[slot].m_itemID;
}

unsigned int CGPlayer_C::GetLootItemDisplayID(unsigned int slot) {
  FATALASSERT(slot < 16);
  return s_lootItems[slot].m_displayID;
}

unsigned int CGPlayer_C::GetLootItemQuantity(unsigned int slot) {
  FATALASSERT(slot < 16);
  return s_lootItems[slot].m_quantity;
}

int CGPlayer_C::OnLootRemoved(CDataStore *msg) {
  unsigned int slot = 0;
  msg->Get(*reinterpret_cast<unsigned char *>(&slot));
  if (ClntObjMgrObjectPtr(m_lootingUnit, __FILE__, __LINE__)) {
    s_lootItems[slot].m_itemID = 0;
    s_lootItems[slot].m_displayID = 0;
    s_lootItems[slot].m_quantity = 0;
    CGLootInfo::ClearSlot(slot);
  }
  return 1;
}

int CGPlayer_C::OnLootMoneyNotify(CDataStore *msg) {
  char         string[128];
  char         buf[128];
  char         coinBuf[64];
  char         moneyBuf[64];
  char         coinName[32];
  int          coins[3];
  unsigned int money;

  msg->Get(money);
  if (money) {
    CurrencyBreakdown(money, coins);
    int first = 1;
    for (int coin = 2; coin >= 0; --coin) {
      if (coins[coin]) {
        SStrCopy(coinName, FrameScript_GetText(CurrencyAbbreviation(coin), -1, GENDER_NOT_APPLICABLE), sizeof(coinName));
        if (first) {
          SStrPrintf(moneyBuf, sizeof(moneyBuf), "%d %s", coins[coin], coinName);
          first = 0;
        } else {
          SStrPrintf(coinBuf, sizeof(coinBuf), ", %d %s", coins[coin], coinName);
          SStrPack(moneyBuf, coinBuf, sizeof(moneyBuf));
        }
      }
    }
    SStrCopy(buf, FrameScript_GetText("LOOT_MONEY", -1, GENDER_NOT_APPLICABLE), sizeof(buf));
    SStrPrintf(string, sizeof(string), buf, moneyBuf);
    CGChat::AddChatMessage(string, static_cast<SLASH_COMMAND_ID>(9), 0, 0, 0, 0, 0);
  }
  return 1;
}

int CGPlayer_C::OnLootClearMoney(CDataStore *msg) {
  CGLootInfo::CoinsCleared();
  return 1;
}

int CGPlayer_C::OnLootItemNotify(CDataStore *msg) {
  char             itemName[64];
  unsigned __int64 player;
  int              displayID;
  unsigned int     slot = 0;
  unsigned int     pushed = 0;
  msg->Get(player);
  msg->Get(*reinterpret_cast<unsigned char *>(&slot));
  msg->Get(*reinterpret_cast<unsigned char *>(&pushed));
  msg->Get(displayID);
  msg->GetString(itemName, 64);
  return 1;
}

int CGPlayer_C::OnLootReleaseResponse(CDataStore *msg) {
  unsigned __int64 packGUID;
  unsigned int     success = 0;
  msg->Get(packGUID);
  msg->Get(*reinterpret_cast<unsigned char *>(&success));
  if (success && packGUID == m_lootingUnit) {
    m_lootingUnit = 0;
    if (GetPlayerAnimState() == 44) {
      UpdateBaseAnimation(45, 0);
    }
    m_flags &= ~0x200;
    CGLootInfo::SetObject(0, 0, LOOT_ACQUIRE_FAILED);
  }
  return 1;
}

void CGPlayer_C::LootAnimEndHandler() {
  if (!m_lootingUnit && !(GetUnitData()->flags & 0x400)) {
    UpdateBaseAnimation(GetPlayerAnimState(), 0);
  }
}

void CGPlayer_C::ReadItem(unsigned int packSlot, unsigned int slot) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_READ_ITEM));
  msg.Put(static_cast<unsigned char>(packSlot));
  msg.Put(static_cast<unsigned char>(slot));
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::ReadItem(unsigned __int64 containerGUID, unsigned char slot) {
  typedef void (CGPlayer_C::*ReadPackItemProc)(unsigned int, unsigned int);
  ReadPackItemProc readPackItem = CGPlayer_C::ReadItem;

  if (containerGUID == GetGUID()) {
    (this->*readPackItem)(0xFF, slot);
    return;
  }

  for (unsigned int packSlot = 19; packSlot < 23; ++packSlot) {
    unsigned __int64 item = m_inventory.GetItem(packSlot);
    if (item == containerGUID) {
      (this->*readPackItem)(packSlot, slot);
      return;
    }
  }
}

int CGPlayer_C::CanLoot(CGUnit_C *unitPtr) {
  FATALASSERT(unitPtr);
  return (GetPosition() - unitPtr->GetPosition()).SquaredMag() <=
             (GetUnitData()->combatReach + GetUnitData()->boundingRadius + unitPtr->GetUnitData()->combatReach +
              unitPtr->GetUnitData()->boundingRadius + 1.333333373f) *
                 1.049999952f * 0.899999976f *
                 (GetUnitData()->combatReach + GetUnitData()->boundingRadius + unitPtr->GetUnitData()->combatReach +
                  unitPtr->GetUnitData()->boundingRadius + 1.333333373f) *
                 1.049999952f * 0.899999976f &&
         unitPtr->GetGUID() != m_lootingUnit && !m_lootingUnitSent && GetUnitData()->health > 0 && !(m_flags & 0x4000) &&
         !(GetUnitData()->flags & 0x40000);
}

unsigned int CGPlayer_C::GetPlayerAnimState() {
  if (GetGUID() == ClntObjMgrGetActivePlayer()) {
    if (!(m_flags & 0x200) && (m_lootingUnit || m_lootingUnitSent) &&
        GetAnimPriority(m_currentBaseAnimState) <= GetAnimPriority(44) && GetAnimPriority(m_currentTorsoAnimState) <= GetAnimPriority(44))
    {
      unsigned __int64 lootTarget = m_lootingUnit ? m_lootingUnit : m_lootingUnitSent;
      CGObject_C      *object = ClntObjMgrObjectPtr(lootTarget, __FILE__, __LINE__);
      if (!object || !(object->GetType() & TYPE_ITEM)) {
        return 44;
      }
    }
  } else if (!(m_flags & 0x200) && (GetUnitData()->flags & 0x400) && !(m_move.m_moveFlags & 0xFF)) {
    return 44;
  }

  return 0;
}

void CGPlayer_C::AttachObjComponent(unsigned __int64 item, unsigned int slot, bool defer, bool sheathe, int sheatheAttachmentSlot) {
  CGItem_C *itemptr = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(item, __FILE__, __LINE__));
  if (!itemptr || !((1 << slot) & 0x38005)) {
    return;
  }

  unsigned int showHidden = 0;
  if ((slot == 17 && m_unit->weaponMode != WEAPONMODE_RANGED) || (slot == 15 && (m_unit->flags & 0x200000))) {
    showHidden = 1;
  }

  const ItemStats       *stats = itemptr->GetStats();
  const ItemSubClassRec *subclass = stats ? SDBItemSubclassGetSubClassRec(stats->m_class, stats->m_subclass) : 0;
  unsigned int           forceAlternate = subclass && (subclass->m_flags & 0x20);
  AddObjectComponentBySlot(
      slot, itemptr->GetDisplayID(), itemptr->GetInventoryType(), forceAlternate != 0, defer != 0, sheathe != 0, sheatheAttachmentSlot,
      showHidden != 0
  );
  itemptr->UpdateEnchantments();
  CGWorldFrame *worldFrame = CGWorldFrame::GetActive();
  if (worldFrame && GetGUID() == ClntObjMgrGetActivePlayer()) {
    worldFrame->RefreshPlayerAlpha();
  }
}

const CreatureModelDataRec *Player_C_GetModelName(unsigned int race, unsigned int sex) {
  int                           displayID;
  const CreatureDisplayInfoRec *displayInfo;
  const CreatureModelDataRec   *model;

  ASSERT(sex < UNITSEX_LAST);

  displayID = Player_C_GetDisplayId(race, sex);
  displayInfo = g_creatureDisplayInfoDB.GetRecord(displayID);
  if (!displayInfo) {
    FATALERROR(("Error, unknown displayInfo %d specified for player race %d sex %d!", displayID, race, sex));
  }

  model = g_creatureModelDataDB.GetRecord(displayInfo->m_modelID);
  if (!model) {
    FATALERROR(("Error, unknown model record %d specified for player race %d sex %d!", displayInfo->m_modelID, race, sex));
  }

  return model;
}

unsigned int Player_C_GetDisplayId(unsigned int race, unsigned int sex) {
  const ChrRacesRec *raceRecord;

  ASSERT(sex < UNITSEX_LAST);

  raceRecord = g_chrRacesDB.GetRecord(race);
  if (!raceRecord) {
    FATALERROR(("Error, race %d not found in race table!", race));
  }

  switch (sex) {
    case UNITSEX_MALE:
      return raceRecord->m_MaleDisplayId;

    case UNITSEX_FEMALE:
      return raceRecord->m_FemaleDisplayId;

    case UNITSEX_NONE:
      FATALERROR(("Error, attempted to look up model for player with sex %d (UNITSEX_NONE), all players have sex! =D", sex));
      return 0;

    default:
      FATALERROR(("Error, unrecognized sex code %d!", sex));
      return 0;
  }
}

void CGPlayer_C::ReadItemResult(NETMESSAGE msgID, CDataStore *msg) {
  unsigned __int64 item;
  int              delay;
  unsigned int     subcode;

  msg->Get(item);
  if (msgID == SMSG_READ_ITEM_OK) {
    CGItem_C *itemPtr = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(item, __FILE__, __LINE__));
    if (itemPtr) {
      itemPtr->SetTranslated();
      CGItemText::SetItem(item, 1);
    }
    return;
  }

  msg->Get(*reinterpret_cast<unsigned char *>(&subcode));
  switch (subcode) {
    case 0:
      CGItemText::SetItem(item, 1);
      break;

    case 1:
      CGItemText::SetItem(item, 0);
      msg->Get(delay);
      FrameScript_SignalEvent(274, "%f", static_cast<float>(delay) * 0.001f);
      break;

    default:
      if (item == CGItemText::GetItem()) {
        FrameScript_SignalEvent(276);
      }
      break;
  }
}

CGPlayer_C::~CGPlayer_C() {
  CGPartyInfo::EnableMember(GetGUID(), 0);
  UnsetPlayerMirrorHandlers();
  if (GetGUID() == ClntObjMgrGetActivePlayer()) {
    UnsetActiveMirrorHandlers();
  }
  if (GetGUID() == ClntObjMgrGetActivePlayer()) {
    CGGameUI::LeaveWorld();
    CGPlayer_C::SetActive(0);
  }
}

void CGPlayer_C::Disable(int shutdown) {
  CGGameUI::EnablePartyMember(GetGUID(), 0);
  UnsetPlayerMirrorHandlers();
  if (GetGUID() == ClntObjMgrGetActivePlayer()) {
    UnsetActiveMirrorHandlers();
  }
  if (GetGUID() == ClntObjMgrGetActivePlayer()) {
    CGGameUI::LeaveWorld();
  }
  CGUnit_C::Disable(shutdown);
}

void CGPlayer_C::Reenable() {
  CGUnit_C::Reenable();
  SetPlayerMirrorHandlers();
  if (GetGUID() == ClntObjMgrGetActivePlayer()) {
    SetActiveMirrorHandlers();
  }
  if (GetGUID() == ClntObjMgrGetActivePlayer()) {
    CGGameUI::EnterWorld();
  }
}

void CGPlayer_C::HandleMountResult(unsigned int result) {
  if (result < 11) {
    CGGameUI::DisplayError(s_mountResultGameErrors[result]);
  }
}

void CGPlayer_C::HandleDismountResult(unsigned int result) {
  if (result < 4) {
    CGGameUI::DisplayError(s_dismountResultGameErrors[result]);
  }
}

float CGPlayer_C::GetMountScale() const {
  const ChrRacesRec *rec = g_chrRacesDB.GetRecord(m_unit->race);
  FATALASSERT(rec);
  FATALASSERT(rec->m_MountScale >= 0.0f);
  return rec->m_MountScale;
}

int CGPlayer_C::GetLanguageSkill(unsigned int language, unsigned int &skill) {
  skill = 0;
  if (GetGUID() != ClntObjMgrGetActivePlayer()) {
    return 0;
  }

  int spellID = CGSpellBook::GetLanguageSpell(language);
  if (!spellID) {
    return 0;
  }

  const SkillLineAbilityRec *ability = SpellTableLookupAbility(GetUnitData()->race, GetUnitData()->classId, spellID);
  if (!ability) {
    return 0;
  }

  int index = GetSkillIndex(ability->m_skillLine);
  if (index < 0 || !g_skillLineDB.GetRecord(ability->m_skillLine)) {
    return 0;
  }

  skill = GetMirrorSkillRank(index) + GetMirrorSkillModifier(index);
  return 1;
}

int Player_C_TogglePlayerRender() {
  s_renderPlayer = !s_renderPlayer;
  return s_renderPlayer;
}

int Player_C_SetPlayerRender(int enable) {
  int previous = s_renderPlayer;
  s_renderPlayer = enable;
  return previous;
}

void Player_C_ClearGuildIDs() {
  s_guildIDs.Clear();
}

void CGPlayer_C::SetCombatMode(int state) {
  if (GetGUID() != ClntObjMgrGetActivePlayer()) {
    return;
  }

  if (state && (m_unit->flags & 0x20000)) {
    return;
  }

  unsigned int oldState = m_flags & 0x400;
  if (state) {
    m_flags |= 0x400;
    ResetCombatModeTimer(oldState == 0);
  } else {
    m_flags &= 0xFFFFFB3F;
    KillCombatModeTimer();
    if (m_combat.IsAttacking() || m_combat.AttackBeenSent()) {
      CGUnit_C::StopAttack();
    }
    Spell_C_CancelCombatSpell();
  }

  if (oldState == static_cast<unsigned int>(state)) {
    return;
  }

  if (state && m_castingSpell) {
    const SpellRec *spell = g_spellDB.GetRecord(m_castingSpell);
    FATALASSERT(spell);
    if (spell->m_attributes & 2) {
      Spell_C_CancelSpell(1, 1, SPELL_FAILED_ERROR);
      SetWeaponMode(WEAPONMODE_SHEATHED);
    } else if (spell->m_interruptFlags & 8) {
      Spell_C_CancelSpell(1, 1, SPELL_FAILED_ERROR);
    }
  }

  CGGameUI::PlayerCombatModeChanged(state);

  if (state) {
    SndInterfacePlayVocalUISound(static_cast<VOCALUISOUNDS>(5));
  } else {
    if (static_cast<double>(m_unit->health) / m_unit->maxHealth < 0.5) {
      SndInterfacePlayVocalUISound(static_cast<VOCALUISOUNDS>(10));
    }
    if (!m_unit->displayPower) {
      if (static_cast<double>(m_unit->power[0]) / m_unit->maxPower[0] < 0.5) {
        SndInterfacePlayVocalUISound(static_cast<VOCALUISOUNDS>(11));
      }
    }
  }

  int hasLastWeaponMode = m_lastWeaponModeSent != -1 && m_lastWeaponModeSent != WEAPONMODE_SHEATHED;
  if (state && (m_unit->weaponMode == WEAPONMODE_RANGED || m_unit->weaponMode == WEAPONMODE_MELEE || hasLastWeaponMode)) {
    ToggleSheathe(1);
  }

  if (!m_castingSpell) {
    CGUnit_C::UpdateBaseAnimation(0);
  }

  if (m_rangedStandTimer) {
    DetermineReadySequence(1);
    CGUnit_C::UpdateBaseAnimation(0);
    ClearRangedStandTimer();
  }

  if (state && m_unit->standState) {
    CGPlayer_C::ChangeStandState(0);
  }
}

int CGPlayer_C::OnAttackIconPressed() {
  FATALASSERT(GetGUID() == ClntObjMgrGetActivePlayer());
  if (m_unit->health <= 0) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(144));
    return 0;
  }

  unsigned __int64 target = GetLocalTarget();
  if (!target) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(141));
    return 0;
  }

  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(target, __FILE__, __LINE__));
  if (!unit || unit->GetUnitData()->health <= 0 || !CanEngageTarget(unit)) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(142));
    return 0;
  }

  if (m_flags & 0x400) {
    SetCombatMode(0);
    return 0;
  }
  SetCombatMode(1);
  return 1;
}

void CGPlayer_C::KillCombatModeTimer() {
  if (s_combatModeTimer) {
    ClientKillTimer(s_combatModeTimer, reinterpret_cast<CLIENTTIMERHANDLER>(PlayerCombatModeHandler), "PlayerCombatModeHandler");
    s_combatModeTimer = 0;
  }
}

static int GuildIDUpdateHandler(unsigned __int64, unsigned int, unsigned int, const void *, void *);
static int DuelTeamUpdateHandler(unsigned __int64, unsigned int, unsigned int, const void *, void *);
static int OnUpdateInventoryComponent(unsigned __int64, unsigned int, unsigned int, const void *, void *);
static int OnUpdatePlayerFlags(unsigned __int64, unsigned int, unsigned int, const void *, void *);
static int OnUpdateGuild(unsigned __int64, unsigned int, unsigned int, const void *, void *);

void CGPlayer_C::ResetCombatModeTimer(int newCombat) {
  if (!s_combatModeTimer) {
    unsigned int timeout = newCombat ? 0 : GetCombatModeTimerInterval();
    s_combatModeTimer = ClientSetTimer(timeout, PlayerCombatModeHandler, GetGUID(), 0);
  }
}

unsigned int CGPlayer_C::GetCombatModeTimerInterval() {
  FATALASSERT(m_flags & 0x400);
  return 500;
}

void CGPlayer_C::OnTaxiNodeStatus(CDataStore *msg) {
  unsigned __int64 taxiGUID;
  unsigned int     status = 0;
  msg->Get(taxiGUID);
  msg->Get(*reinterpret_cast<unsigned char *>(&status));

  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(taxiGUID, __FILE__, __LINE__));
  if (unit && (unit->GetUnitData()->npcFlags & 4)) {
    unit->UpdateInteractIcon(static_cast<QUEST_GIVER_STATUS>(status ? 4 : 0));
  }
}

void CGPlayer_C::ShowTaxiNodes(CDataStore *msg) {
  NTempest::CRect  rect;
  unsigned __int64 unit = 0;
  __int64          known;
  __int64          flag;
  unsigned int     currentNode = 0;
  unsigned int     showWindow;

  msg->Get(showWindow);
  if (showWindow) {
    msg->Get(unit);
    msg->Get(currentNode);
  }
  msg->Get(known);
  msg->Get(flag);

  if (!known) {
    if (showWindow) {
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(166));
    } else {
      ConsoleWrite("No taxi nodes for you!", DEFAULT_COLOR);
    }
  } else if (showWindow) {
    if (TaxiMapUpdatePosition(currentNode, known, flag, rect)) {
      CGTaxiMap::SetupMap(unit, currentNode, known, flag, rect);
    }
  } else {
    for (unsigned int index = 0; index < 64; ++index) {
      if (known & (static_cast<__int64>(1) << index)) {
        TaxiNodesRec *node = g_taxiNodesDB.GetRecord(index + 1);
        if (node) {
          ConsolePrintf("[%02d]: %s", node->m_ID, node->m_Name_lang[CURRENT_LANGUAGE]);
        }
      }
    }
  }
}

void CGPlayer_C::StartTaxi(unsigned __int64 vendor, unsigned int startNode, unsigned int destNode) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_ACTIVATETAXI));
  msg.Put(vendor);
  msg.Put(startNode);
  msg.Put(destNode);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::HandleActivateTaxiReply(unsigned int code) {
  if (code < 12) {
    if (code) {
      CGGameUI::DisplayError(s_taxiErrors[code]);
    } else {
      CGTaxiMap::CloseMap();
    }
  }
}

unsigned __int64 CGPlayer_C::GetLocalTarget() const {
  const unsigned __int64 &lockedTarget = CGGameUI::GetLockedTarget();
  const unsigned __int64 combatTarget = m_combat.IsAttacking();

  if (lockedTarget && lockedTarget == combatTarget) {
    return lockedTarget;
  }

  return m_targetUnit;
}

int CGPlayer_C::DeathBindDistanceCompare(NTempest::C3Vector &bindStonePosition) {
  NTempest::C3Vector diff = s_bindPosition - bindStonePosition;
  float              bindRadiusCheckSquared = 10.0f * 1.5f;
  bindRadiusCheckSquared *= bindRadiusCheckSquared;
  return s_bindSaved && ClntObjMgrGetMapID() == s_bindZoneID && diff.SquaredMag() <= bindRadiusCheckSquared;
}

void CGPlayer_C::SaveBindPoint(CDataStore *msg) {
  msg->Get(s_bindPosition.x);
  msg->Get(s_bindPosition.y);
  msg->Get(s_bindPosition.z);
  msg->Get(s_bindZoneID);
  s_bindSaved = 1;
  UpdateBindStatusAll();
}

NTempest::C3Vector &CGPlayer_C::GetBindPoint() {
  return s_bindPosition;
}

void CGPlayer_C::OnSpellFailed(const SpellRec *spellRec, unsigned int reason) {
  ClearTrackingTarget(0);
  if (spellRec && (spellRec->m_ID != static_cast<unsigned int>(m_castingSpell) || reason != 59) && (spellRec->m_attributes & 2)) {
    DetermineReadySequence(0);
    CGUnit_C::UpdateBaseAnimation(0);
  }
}

void CGPlayer_C::PlayVocalMacro(int category) {
  CVar *masterSoundEffects = CVar::Lookup("MasterSoundEffects");
  CVar *enableGroupSpeech = CVar::Lookup("EnableGroupSpeech");
  if (masterSoundEffects && masterSoundEffects->GetInt() && enableGroupSpeech && enableGroupSpeech->GetInt() && category < 12) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      UNITAFFILIATION affiliation = player->GetGUIDAffiliation(GetGUID());
      RequestTalkEmote(TALKANIM_SHOUT);
      if (5 & (1 << affiliation)) {
        SoundInterfacePlayVocalMacro(this, category);
      }
    }
  }
}

static int SoulStoneCompare(const CGItem_C *item, void *__formal) {
  const unsigned __int64 noGuid = 0;
  const ItemStats_C     *stats = g_itemDBCache.GetRecord(item->GetEntryID(), noGuid, 0, 0);
  if (!stats) {
    return 0;
  }

  for (int spellIndex = 0; spellIndex < 5; ++spellIndex) {
    int spellID = stats->m_spellID[spellIndex];
    if (spellID < 0 || spellID > g_spellDB.GetMaxID() || stats->m_spellTrigger[spellIndex]) {
      continue;
    }

    const SpellRec *spell = g_spellDB.GetRecord(spellID);
    if (!spell) {
      continue;
    }
    for (int effectIndex = 0; effectIndex < 3; ++effectIndex) {
      if (spell->m_effect[effectIndex] == 18 && (spell->m_implicitTargetA[effectIndex] == 1 || spell->m_implicitTargetB[effectIndex] == 1)) {
        return 1;
      }
    }
  }
  return 0;
}

CGItem_C *CGPlayer_C::GetSoulstone() const {
  return m_inventory.FindItem(SoulStoneCompare, 0, 0);
}

void CGPlayer_C::UseSoulstone() const {
  CGItem_C *soulstone = GetSoulstone();
  if (soulstone) {
    soulstone->Use();
  }
}

void CGPlayer_C::SaveDeathMessage(unsigned __int64 guid) {
  m_lastKillerGUID = guid;
  CheckKillerFeedback();
}

void CGPlayer_C::CheckKillerFeedback() {
  if (!GetUnitData()->health && !m_deathHolds && m_lastKillerGUID) {
    CGUnit_C *killer = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(m_lastKillerGUID, __FILE__, __LINE__));
    if (killer) {
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(115), killer->GetUnitName());
    }
    m_lastKillerGUID = 0;
  }
}

bool CGPlayer_C::GetPackAndSlot(CGItem_C *item, unsigned char &packSlot, unsigned char &slot) {
  unsigned __int64 containerGUID = item->m_item->m_containedIn;
  if (containerGUID == GetGUID()) {
    packSlot = 0xFF;
    slot = static_cast<unsigned char>(FindSlotIndex(item->GetGUID()));
    return slot != 0xFF;
  }

  packSlot = static_cast<unsigned char>(FindSlotIndex(containerGUID));
  if (packSlot == 0xFF) {
    return false;
  }
  CGObject_C *containerObject = ClntObjMgrObjectPtr(containerGUID, __FILE__, __LINE__);
  CGBag      *bag = containerObject ? containerObject->GetBag() : 0;
  if (!bag) {
    return false;
  }
  for (unsigned int index = 0; index < bag->NumSlots(); ++index) {
    if (bag->GetItem(index) == item->GetGUID()) {
      slot = static_cast<unsigned char>(index);
      return true;
    }
  }
  return false;
}

void CGPlayer_C::OpenLootItem(CGItem_C *item) {
  unsigned char packSlot;
  unsigned char slot;
  if (!GetPackAndSlot(item, packSlot, slot)) {
    return;
  }
  m_lootingUnit = item->GetGUID();
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_OPEN_ITEM));
  msg.Put(packSlot);
  msg.Put(slot);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::OpenWrappedItem(CGItem_C *item) {
  unsigned char packSlot;
  unsigned char slot;
  if (!GetPackAndSlot(item, packSlot, slot)) {
    return;
  }
  SndInterfacePlayInterfaceSound("UnwrapGift");
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_OPEN_ITEM));
  msg.Put(packSlot);
  msg.Put(slot);
  msg.Finalize();
  ClientServices_Send(&msg);
}

int CGPlayer_C::InviteToGroup(unsigned __int64 target) {
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(target, __FILE__, __LINE__));
  if (!unit) {
    return 0;
  }
  InviteToGroup(unit->GetUnitName());
  return 1;
}

void CGPlayer_C::InviteToGroup(const char *target) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_GROUP_INVITE));
  msg.PutString(target);
  msg.Finalize();
  ClientServices_Send(&msg);
}

int CGPlayer_C::Uninvite(unsigned __int64 target) {
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(target, __FILE__, __LINE__));
  if (unit) {
    Uninvite(unit->GetUnitName());
  } else {
    CDataStore msg;
    msg.Put(static_cast<unsigned int>(CMSG_GROUP_UNINVITE_GUID));
    msg.Put(target);
    msg.Finalize();
    ClientServices_Send(&msg);
  }
  return 1;
}

void CGPlayer_C::Uninvite(const char *target) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_GROUP_UNINVITE));
  msg.PutString(target);
  msg.Finalize();
  ClientServices_Send(&msg);
}

int CGPlayer_C::SetNewLeader(unsigned __int64 target) {
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(target, __FILE__, __LINE__));
  if (!unit) {
    return 0;
  }
  SetNewLeader(unit->GetUnitName());
  return 1;
}

void CGPlayer_C::SetNewLeader(const char *target) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_GROUP_SET_LEADER));
  msg.PutString(target);
  msg.Finalize();
  ClientServices_Send(&msg);
}

int CGPlayer_C::ReportBagItemSubtypeMismatch(unsigned int bagSlot) const {
  unsigned char slot = static_cast<unsigned char>(bagSlot);
  if (slot == 0xFF) {
    return 0;
  }

  unsigned __int64 itemGUID = m_inventory.GetItem(slot);
  CGItem_C            *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(itemGUID, __FILE__, __LINE__));
  if (!item || item->GetClassID() != 11) {
    return 0;
  }

  const ItemSubClassRec *subclass = SDBItemSubclassGetSubClassRec(6, item->GetSubtypeID());
  if (!subclass) {
    return 0;
  }

  CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(253), subclass->m_verboseName_lang[CURRENT_LANGUAGE]);
  return 1;
}

int CGPlayer_C::OnSplitMoneyNotify(CDataStore *msg) {
  char             string[128];
  char             buf[128];
  char             shareMoneyBuf[64];
  char             totalMoneyBuf[64];
  char             coinBuf[64];
  char             coinName[32];
  int              sharecoins[3];
  int              totalcoins[3];
  unsigned int     total;
  unsigned __int64 player;
  unsigned int     share;

  msg->Get(player);
  msg->Get(share);
  msg->Get(total);
  if (share && total) {
    CurrencyBreakdown(share, sharecoins);
    CurrencyBreakdown(total, totalcoins);

    int first = 1;
    for (int coin = 2; coin >= 0; --coin) {
      if (sharecoins[coin]) {
        SStrCopy(coinName, FrameScript_GetText(CurrencyAbbreviation(coin), -1, GENDER_NOT_APPLICABLE), sizeof(coinName));
        if (first) {
          SStrPrintf(shareMoneyBuf, sizeof(shareMoneyBuf), "%d %s", sharecoins[coin], coinName);
          first = 0;
        } else {
          SStrPrintf(coinBuf, sizeof(coinBuf), ", %d %s", sharecoins[coin], coinName);
          SStrPack(shareMoneyBuf, coinBuf, sizeof(shareMoneyBuf));
        }
      }
    }

    first = 1;
    for (coin = 2; coin >= 0; --coin) {
      if (totalcoins[coin]) {
        SStrCopy(coinName, FrameScript_GetText(CurrencyAbbreviation(coin), -1, GENDER_NOT_APPLICABLE), sizeof(coinName));
        if (first) {
          SStrPrintf(totalMoneyBuf, sizeof(totalMoneyBuf), "%d %s", totalcoins[coin], coinName);
          first = 0;
        } else {
          SStrPrintf(coinBuf, sizeof(coinBuf), ", %d %s", totalcoins[coin], coinName);
          SStrPack(totalMoneyBuf, coinBuf, sizeof(totalMoneyBuf));
        }
      }
    }

    const NameCache *name = g_nameDBCache.GetRecord(player, player, 0, 0);
    if (name) {
      if (player == ClntObjMgrGetActivePlayer()) {
        SStrCopy(buf, FrameScript_GetText("SPLIT_MONEY_SPLIT_SELF", -1, GENDER_NOT_APPLICABLE), sizeof(buf));
        SStrPrintf(string, sizeof(string), buf, shareMoneyBuf);
      } else {
        SStrCopy(buf, FrameScript_GetText("SPLIT_MONEY_SPLIT", -1, GENDER_NOT_APPLICABLE), sizeof(buf));
        SStrPrintf(string, sizeof(string), buf, name->m_name, shareMoneyBuf, totalMoneyBuf);
      }
      CGChat::AddChatMessage(string, static_cast<SLASH_COMMAND_ID>(9), 0, 0, 0, 0, 0);
    }
  }
  return 1;
}

void CGPlayer_C::StartGiftWrap(CGItem_C *wrapper) {
  s_giftWrapItem = wrapper->GetGUID();
  CGGameUI::LockItem(s_giftWrapItem);
  CursorSetCursorMode(CAST_CURSOR);
}

void CGPlayer_C::CancelGiftWrap() {
  CGGameUI::UnlockItem(s_giftWrapItem);
  s_giftWrapItem = 0;
  CursorSetCursorMode(POINT_CURSOR);
}

bool CGPlayer_C::IsGiftWrapping() {
  return s_giftWrapItem != 0;
}

void CGPlayer_C::SheatheWeapon(unsigned int sheathe) {
  if (!m_inventory.GetItem(15) && !m_inventory.GetItem(16)) {
    return;
  }

  if (sheathe && (m_flags & 0x400)) {
    SetCombatMode(0);
  }

  if (GetGUID() == ClntObjMgrGetActivePlayer()) {
    CDataStore msg;
    msg.Put(static_cast<unsigned int>(CMSG_SHEATHE));
    msg.Put(static_cast<unsigned char>(sheathe));
    msg.Finalize();
    ClientServices_Send(&msg);
  }

  static const unsigned char s_standStateAllowsSheathing[12] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  if (s_standStateAllowsSheathing[m_unit->standState]) {
    MaybeStartSheatheAnim();
  }
}

void CGPlayer_C::SetFarSightFocus(CGObject_C *obj) {
  if (!(m_flags & 0x800)) {
    ToggleFarSight();
  }
}

void CGPlayer_C::ToggleFarSight() {
  unsigned __int64 focusGUID = GetFarSightFocusGUID();
  CGObject_C      *focus = ClntObjMgrObjectPtr(focusGUID, __FILE__, __LINE__);

  if (!focus || CGWorldFrame::GetActiveCamera()->GetTarget() == focus->GetGUID()) {
    m_flags &= ~0x800;
    CGGameUI::ResetCamera();
    CGUnit_C::SetActiveMover(GetGUID());
    return;
  }

  SetCombatMode(0);
  unsigned __int64 mover = 0;
  if (focus->GetType() & TYPE_UNIT) {
    CGUnit_C         *unit = static_cast<CGUnit_C *>(focus);
    const CGUnitData *unitData = unit->GetUnitData();
    unsigned __int64  controller = unitData->charmedBy ? unitData->charmedBy : unitData->createdBy;
    if ((unitData->flags & 0x01000000) && controller == GetGUID()) {
      mover = focus->GetGUID();
    }
  }

  CGUnit_C::SetActiveMover(mover);
  m_flags |= 0x800;
  CGWorldFrame::GetActive()->SetCameraTarget(focus);
}

void CGPlayer_C::ClearFarSight() {
  if (m_flags & 0x800) {
    ToggleFarSight();
  }
}

CGUnit_C *CGPlayer_C::GetPossessedUnit() {
  if (!(m_flags & 0x800)) {
    return 0;
  }

  unsigned __int64 focusGUID = GetFarSightFocusGUID();
  CGObject_C      *focus = ClntObjMgrObjectPtr(focusGUID, __FILE__, __LINE__);
  if (!focus || !(focus->GetType() & TYPE_UNIT)) {
    return 0;
  }

  CGUnit_C         *unit = static_cast<CGUnit_C *>(focus);
  const CGUnitData *unitData = unit->GetUnitData();
  unsigned __int64  controller = unitData->charmedBy ? unitData->charmedBy : unitData->createdBy;
  if (!(unitData->flags & 0x01000000) || controller != GetGUID()) {
    return 0;
  }
  return unit;
}

int CGPlayer_C::OnPetitionShowList(CDataStore *msg) {
  unsigned __int64 petitionNpcGUID;
  unsigned int     count = 0;
  msg->Get(petitionNpcGUID);
  msg->Get(*reinterpret_cast<unsigned char *>(&count));

  memset(petitionList, 0, sizeof(petitionList));
  for (unsigned int index = 0; index < count; ++index) {
    msg->Get(petitionList[index].m_muid);
    msg->Get(petitionList[index].m_itemID);
    msg->Get(petitionList[index].m_itemDisplayID);
    msg->Get(petitionList[index].m_price);
    msg->Get(petitionList[index].m_flags);
  }

  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(petitionNpcGUID, __FILE__, __LINE__));
  if (unit && (unit->GetUnitData()->npcFlags & 0x80) && (unit->GetUnitData()->npcFlags & 0x40) && (petitionList[0].m_flags & 1)) {
    CGGuildRegistrar::SetRegistrar(petitionNpcGUID, petitionList);
  }
  return 1;
}

void CGPlayer_C::BuyPetition(const unsigned __int64 &petitionUnit, CGPetition *petition) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_PETITION_BUY));
  msg.Put(petitionUnit);
  petition->Pack(&msg);
  msg.Finalize();
  ClientServices_Send(&msg);
}

int CGPlayer_C::OnPetitionShowSignatures(CDataStore *msg) {
  unsigned __int64 itemGUID;
  unsigned __int64 ownerGUID;
  int              petitionID;
  unsigned int     count = 0;
  unsigned int     i;

  msg->Get(itemGUID);
  msg->Get(ownerGUID);
  msg->Get(petitionID);
  msg->Get(*reinterpret_cast<unsigned char *>(&count));

  unsigned __int64 *signers = static_cast<unsigned __int64 *>(_alloca(sizeof(unsigned __int64) * count));
  int              *choices = static_cast<int *>(_alloca(sizeof(int) * count));
  for (i = 0; i < count; ++i) {
    msg->Get(signers[i]);
    msg->Get(choices[i]);
  }

  if (!g_friendList->IsIgnored(ownerGUID)) {
    CGPetitionInfo::SetPetition(itemGUID, petitionID);
    CGPetitionInfo::SetSignatures(count, signers, choices);
  }
  return 1;
}

void CGPlayer_C::RequestPetitionSignatures(unsigned __int64 item) {
  if (!item) {
    return;
  }
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_PETITION_SHOW_SIGNATURES));
  msg.Put(item);
  msg.Finalize();
  ClientServices_Send(&msg);
}

int CGPlayer_C::OnSignedResults(CDataStore *msg) {
  PETITION_ERROR results;
  msg->Get(*reinterpret_cast<int *>(&results));
  switch (results) {
    case PETITION_SUCCESS:
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(292));
      FrameScript_SignalEvent(374);
      break;
    case PETITION_ALREADY_SIGNED:
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(293));
      break;
    case PETITION_ALREADY_IN_GUILD:
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(294));
      break;
    case PETITION_CHARTER_CREATOR:
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(295));
      break;
    default:
      ConsoleWrite("Petition error", DEFAULT_COLOR);
      break;
  }
  return 1;
}

int CGPlayer_C::OnTurnInPetitionResults(CDataStore *msg) {
  PETITION_ERROR results;
  msg->Get(*reinterpret_cast<int *>(&results));
  switch (results) {
    case PETITION_SUCCESS:
      FrameScript_SignalEvent(362);
      break;
    case PETITION_ALREADY_IN_GUILD:
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(294));
      break;
    case PETITION_NOT_ENOUGH_SIGNATURES:
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(296));
      break;
    default:
      ConsoleWrite("Petition error", DEFAULT_COLOR);
      break;
  }
  return 1;
}

void GuildCharterTurnInCallback(int, const unsigned __int64 &, void *, bool granted) {
  if (granted) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      player->TurnInGuildCharter();
    }
  }
}

void CGPlayer_C::TurnInGuildCharter() {
  CGBag_C     *inventory = GetBag();
  CGItem_C    *item = 0;
  unsigned int j;

  for (j = 23; j <= 38; ++j) {
    unsigned __int64 guid = inventory->GetItem(j);
    item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
    if (item && item->IsA(TYPE_ITEM)) {
      const CGPetition *petition = g_petitionCache.GetRecord(item->GetEntryID(), guid, GuildCharterTurnInCallback, 0);
      if (!petition) {
        return;
      }
      if (petition->m_flags & 1) {
        break;
      }
    }
    item = 0;
  }

  int found = item != 0;
  if (!found) {
    for (j = 19; j <= 22 && !found; ++j) {
      CGObject_C *container = ClntObjMgrObjectPtr(inventory->GetItem(j), __FILE__, __LINE__);
      CGBag_C    *bag = container ? container->GetBag() : 0;
      if (!bag) {
        continue;
      }
      for (unsigned int slot = 0; slot < bag->NumSlots(); ++slot) {
        unsigned __int64 guid = bag->GetItem(slot);
        item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
        if (item && item->IsA(TYPE_ITEM)) {
          const CGPetition *petition = g_petitionCache.GetRecord(item->GetEntryID(), guid, GuildCharterTurnInCallback, 0);
          if (!petition) {
            return;
          }
          if (petition->m_flags & 1) {
            found = 1;
            break;
          }
        }
        item = 0;
      }
    }
  }

  if (!found) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(111));
    return;
  }

  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_TURN_IN_PETITION));
  msg.Put(item->GetGUID());
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::SendTextEmote(EmotesTextRec *rec, const unsigned __int64 &target) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_TEXT_EMOTE));
  msg.Put(static_cast<unsigned int>(rec->m_ID));
  msg.Put(target);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::AddDeferredDamage(int normal, unsigned int flags, unsigned int damage, unsigned __int64 victim) {
  DEFERREDDAMAGE *deferred = s_deferredDamage.NewNode(LIST_HEAD, 0, 0);
  deferred->Set(normal, flags, damage, victim);
}

void CGPlayer_C::AddDeferredSpellMiss(unsigned __int64 victim, MISS_REASON reason, int spellID) {
  DEFERREDSPELLMISS *deferred = s_deferredSpellMiss.NewNode(LIST_HEAD, 0, 0);
  deferred->Set(victim, reason, spellID);
}

void CGPlayer_C::ProcessDeferredDamage() {
  DEFERREDDAMAGE *deferred = s_deferredDamage.Head();
  while (deferred) {
    DEFERREDDAMAGE *next = s_deferredDamage.Next(deferred);
    CGUnit_C       *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(deferred->victim, __FILE__, __LINE__));
    if (unit) {
      if (deferred->flags & 2) {
        unit->AddWorldText(WORLDTEXTMISS_ABSORBED);
      } else if (deferred->damage) {
        if (deferred->flags & 1) {
          unit->AddWorldCritText(deferred->damage, deferred->normal);
        } else {
          unit->AddWorldDamageText(deferred->damage, deferred->normal);
        }
      }
      s_deferredDamage.UnlinkNode(deferred);
      deferred->~DEFERREDDAMAGE();
      SMemFree(deferred, 0, 0, 0);
    }
    deferred = next;
  }
}

void CGPlayer_C::ProcessDeferredSpellMiss() {
  DEFERREDSPELLMISS *deferred = s_deferredSpellMiss.Head();
  while (deferred) {
    DEFERREDSPELLMISS *next = s_deferredSpellMiss.Next(deferred);
    CGUnit_C          *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(deferred->victim, __FILE__, __LINE__));
    if (unit) {
      unit->AddWorldText(deferred->reason);
      UnitCombatLogSpellMissed(deferred->reason, deferred->spellID, ClntObjMgrGetActivePlayer(), deferred->victim);
      s_deferredSpellMiss.UnlinkNode(deferred);
      deferred->~DEFERREDSPELLMISS();
      SMemFree(deferred, 0, 0, 0);
    }
    deferred = next;
  }
}

void CGPlayer_C::OnLootGameObject(
    const unsigned __int64 &gameObject,
    bool lootAnim
) {
  m_lootingUnitSent = gameObject;
  if (lootAnim && m_currentTorsoAnimState != 37) {
    UpdateBaseAnimation(44, 0);
  }
}
void CGPlayer_C::GetAFKText(char *buffer, int size) const {
  if (m_plyr->playerFlags & 4) {
    SStrCopy(buffer, FrameScript_GetText("CHAT_FLAG_AFK", -1, GENDER_NOT_APPLICABLE), size);
  } else {
    buffer[0] = '\0';
  }
}

void CGPlayer_C::GetDNDText(char *buffer, int size) const {
  if (m_plyr->playerFlags & 8) {
    SStrCopy(buffer, FrameScript_GetText("CHAT_FLAG_DND", -1, GENDER_NOT_APPLICABLE), size);
  } else {
    buffer[0] = '\0';
  }
}

void CGPlayer_C::GetGMText(char *buffer, int size) const {
  if (m_plyr->playerFlags & 0x10) {
    SStrCopy(buffer, FrameScript_GetText("CHAT_FLAG_GM", -1, GENDER_NOT_APPLICABLE), size);
  } else {
    buffer[0] = '\0';
  }
}

void CGPlayer_C::GuildInfoLoaded(const TSGrowableArray<unsigned int> &guildList) {
  if (!m_plyr->guildID) {
    return;
  }

  for (unsigned int index = guildList.Count(); index; --index) {
    if (guildList[index - 1] == m_plyr->guildID) {
      OnGuildChanged();
      return;
    }
  }
}

void CGPlayer_C::PreAnimate(CGWorldFrame *worldFrame) {
  FATALASSERT(worldFrame);
  if (s_guildIDs.Count()) {
    GuildInfoLoaded(s_guildIDs);
  }
  CGUnit_C::PreAnimate(worldFrame);
}

unsigned int CGPlayer_C::UpdateUnitNameString(
    unsigned int localPlayerFlags,
    unsigned int otherUnitsFlags,
    char *buffer,
    unsigned int bufferSize
) const {
  unsigned int flags =
      m_obj->m_guid == ClntObjMgrGetActivePlayer() ? localPlayerFlags : otherUnitsFlags;
  return CGUnit_C::UpdateUnitNameString(flags, flags, buffer, bufferSize);
}

bool CGPlayer_C::GetExpandedSkillRank(int skillID, int &rank, int &modifier) const {
  int index = GetSkillIndex(skillID);
  if (index < 0) {
    return false;
  }

  rank = m_plyr->skillInfo[index].m_skillRank;
  modifier = m_plyr->skillInfo[index].m_skillModifier;
  return true;
}

bool CGPlayer_C::GetDefenseSkillRank(int &base, int &modifier) const {
  base = 0;
  modifier = 0;

  const SkillLineAbilityRec *ability =
      SpellTableLookupAbility(GetUnitData()->race, GetUnitData()->classId, s_defenseSkillID);
  return ability && ability->m_spell == static_cast<int>(s_defenseSkillID) &&
         GetExpandedSkillRank(ability->m_skillLine, base, modifier);
}

bool CGPlayer_C::GetAttackSkillRank(int hand, int &base, int &modifier) const {
  base = 0;
  modifier = 0;

  int weaponSpell = GetWeaponSpell(static_cast<COMBATHAND>(hand));
  const SkillLineAbilityRec *ability =
      SpellTableLookupAbility(GetUnitData()->race, GetUnitData()->classId, weaponSpell);
  return ability && ability->m_spell == weaponSpell &&
         GetExpandedSkillRank(ability->m_skillLine, base, modifier);
}

void CGPlayer_C::CombatLoggingFlagChanged() {
  UnitDebugCombatLogOnEnable(m_unit->flags & 0x800000);
}

void CGPlayer_C::OnAttackStart(unsigned __int64 victim) {
  CGUnit_C::OnAttackStart(victim);
  m_flags = (m_flags & ~0x30u) | 0x10;

  if (GetGUID() == ClntObjMgrGetActivePlayer() && ClntObjMgrGetPlayerType() != PLAYER_BOT) {
    if (s_attackBreakTimer) {
      ClientKillTimer(
          s_attackBreakTimer,
          reinterpret_cast<CLIENTTIMERHANDLER>(PlayerAttackBreakHandler),
          "PlayerAttackBreakHandler"
      );
    }
    s_attackBreakTimer =
        ClientSetTimer(500, PlayerAttackBreakHandler, GetGUID(), ClntObjMgrGetCurrent());
  }
}

void CGPlayer_C::OnAttackStop(unsigned __int64 previousTarget, int nowDead) {
  m_flags &= ~0x30u;
  if (GetGUID() == ClntObjMgrGetActivePlayer() && ClntObjMgrGetPlayerType() != PLAYER_BOT) {
    if (s_attackBreakTimer) {
      ClientKillTimer(
          s_attackBreakTimer,
          reinterpret_cast<CLIENTTIMERHANDLER>(PlayerAttackBreakHandler),
          "PlayerAttackBreakHandler"
      );
    }
    s_attackBreakTimer = 0;
  }
  CGUnit_C::OnAttackStop(previousTarget, nowDead);
}

void CGPlayer_C::OnBadAttackFacing(unsigned __int64 victim) {
  if (GetGUID() != ClntObjMgrGetActivePlayer()) {
    CGUnit_C::OnBadAttackFacing(victim);
    return;
  }

  if (!(m_flags & 0x40)) {
    char buffer[128];
    SStrCopy(
        buffer,
        FrameScript_GetText("ERR_WRONG_DIRECTION_FOR_ATTACK", -1, GENDER_NOT_APPLICABLE),
        sizeof(buffer)
    );
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(195));
    m_flags |= 0x40;
  }
}

void CGPlayer_C::OnBadAttackPosition(unsigned __int64 victim, float range) {
  if (GetGUID() != ClntObjMgrGetActivePlayer()) {
    CGUnit_C::OnBadAttackPosition(victim, range);
    return;
  }

  if (!(m_flags & 0x80)) {
    char buffer[128];
    SStrCopy(
        buffer,
        FrameScript_GetText("ERR_TOO_FAR_TO_ATTACK", -1, GENDER_NOT_APPLICABLE),
        sizeof(buffer)
    );
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(196));
    m_flags |= 0x80;
  }
}

unsigned __int64 CGPlayer_C::GetUnitBeingLooted() const {
  return m_lootingUnit;
}

void CGPlayer_C::OnBadAttackTarget(unsigned __int64) {
  SetCombatMode(0);
}

void CGPlayer_C::OnNotStanding(unsigned __int64) {
  CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(208));
}

void CGPlayer_C::UnitHit(VICTIMSTATES state, unsigned __int64 attacker) {
  if (state && GetGUID() == ClntObjMgrGetActivePlayer() && !CGGameUI::GetLockedTarget()) {
    CGGameUI::Target(attacker, 0);
  }
}

void CGPlayer_C::OnAttackerStateChange(const ATTACKROUNDINFO &roundInfo) {
  if (roundInfo.attacker == ClntObjMgrGetActivePlayer()) {
    m_flags |= 0x20;
  }
  CGUnit_C::OnAttackerStateChange(roundInfo);
}

void CGPlayer_C::HandleMirrorTimerDamage(const MIRRORTIMERDAMAGE &log) {
  CGUnit_C::HandleMirrorTimerDamage(log);
  if (GetGUID() == ClntObjMgrGetActivePlayer()) {
    CGGameUI::ShowCombatFeedback(log);
  }
}

void CGPlayer_C::PlayDeathThudCameraShake() const {
}

bool CGPlayer_C::CanBeMounted() {
  if (m_unit->flags & 0x100000) {
    return true;
  }

  bool allowed;
  return !CWorld::QueryMountAllowed(m_worldObject, allowed) || allowed;
}

void CGPlayer_C::ChangeStandState(unsigned int standState) {
  if (GetGUID() == ClntObjMgrGetActivePlayer()) {
    CGUnit_C::ChangeStandState(standState);
  }
}

void CGPlayer_C::OnStandStateChanged(unsigned int, unsigned int newState) {
  if (GetGUID() != ClntObjMgrGetActivePlayer()) {
    return;
  }

  if (newState) {
    FrameScript_SignalEvent(260);
    if (m_unit->flags & 0x400) {
      CGGameUI::CloseLoot(1, 1);
    }
    if (m_flags & 0x400) {
      SetCombatMode(0);
    }
  } else {
    FrameScript_SignalEvent(259);
  }
}

void CGPlayer_C::OnLevelChange() {
  if (GetGUID() == ClntObjMgrGetActivePlayer()) {
    CGSpellBook::UpdateSpells();
    CGClassTrainer::RefreshList();
    UpdateQuestStatusAll();
  }
  CGUnit_C::OnLevelChange();
}

float CGPlayer_C::GetBlockChance() const {
  return m_plyr->blockPercentage;
}

float CGPlayer_C::GetDodgeChance() const {
  return m_plyr->dodgePercentage;
}

float CGPlayer_C::GetParryChance() const {
  return m_plyr->parryPercentage;
}

void CGPlayer_C::UpdateObjComponentVisuals(const CGItem_C *itemPtr, const ItemEnchantment *enchantments, int num) {
  if (!itemPtr) {
    return;
  }

  if (m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
    CGTradeSkillInfo::RefreshList(0);
    CGActionBar::UpdateItem(itemPtr->GetEntryID());
    CGTradeInfo::UpdatePlayerItem(itemPtr->GetGUID());
    CGContainerInfo::UpdateItem(itemPtr->GetGUID());
    CGCharacterInfo::UpdateItem(itemPtr->GetGUID());
  }

  ItemDisplayInfoRec *displayInfo = g_itemDisplayInfoDB.GetRecord(itemPtr->GetDisplayID());
  if (!displayInfo) {
    return;
  }

  int invSlot = static_cast<unsigned char>(FindSlotIndex(itemPtr->GetGUID()));
  if (invSlot >= 69) {
    return;
  }

  int attachmentSlot = InvSlotToObjAttachSlot(invSlot);
  if (attachmentSlot < 0) {
    return;
  }

  ACTIVEATTACHMENTINFO *info = m_attachments[attachmentSlot];
  if (!info) {
    return;
  }

  ItemVisualsRec *visual = g_itemVisualsDB.GetRecord(displayInfo->m_itemVisual);
  if (visual) {
    return;
  }

  for (int i = 0; i < num; ++i) {
    if (!enchantments[i].id) {
      continue;
    }

    SpellItemEnchantmentRec *enchantment = g_spellItemEnchantmentDB.GetRecord(enchantments[i].id);
    if (enchantment && enchantment->m_itemVisual) {
      visual = g_itemVisualsDB.GetRecord(enchantment->m_itemVisual);
      if (visual) {
        break;
      }
    }
  }

  SetItemVisuals(info, visual, false);
}

void CGPlayer_C::ClearItemVisuals(ACTIVEATTACHMENTINFO *info) {
  if (!info || info->displayInfo->m_itemVisual || !info->enchantmentVisual) {
    return;
  }

  info->enchantmentVisual = 0;
  for (int i = 0; i < 2; ++i) {
    if (info->modelInfo[i].model) {
      ModelClearAllLinks(info->modelInfo[i].model);
    }

    if (info->modelInfo[i].currentLink >= 0) {
      HMODEL child = ComponentUtilGetChildModel(m_paperDollModel, info->modelInfo[i].currentLink);
      if (child) {
        ModelClearAllLinks(child);
        HandleClose(child);
      }
    }
  }
}

void CGPlayer_C::SetItemVisuals(ACTIVEATTACHMENTINFO *info, const ItemVisualsRec *rec, bool force) {
  if (!info || (info->displayInfo->m_itemVisual && !force) ||
      (rec && info->enchantmentVisual && info->enchantmentVisual->m_ID == rec->m_ID)) {
    return;
  }

  ClearItemVisuals(info);
  if (!rec) {
    return;
  }

  if (!info->displayInfo->m_itemVisual) {
    info->enchantmentVisual = const_cast<ItemVisualsRec *>(rec);
  }

  for (int modelIndex = 0; modelIndex < 2; ++modelIndex) {
    ATTACHMENTMODELINFO &modelInfo = info->modelInfo[modelIndex];
    HMODEL child = 0;
    if (modelInfo.currentLink >= 0) {
      child = ComponentUtilGetChildModel(m_paperDollModel, modelInfo.currentLink);
    }

    for (int visualIndex = 0; visualIndex < 5; ++visualIndex) {
      if (!modelInfo.model) {
        continue;
      }

      ItemVisualEffectsRec *effect = g_itemVisualEffectsDB.GetRecord(rec->m_Slot[visualIndex]);
      if (!effect) {
        continue;
      }

      ComponentUtilAddItemVisual(modelInfo.model, visualIndex, effect->m_Model);
      if (modelInfo.currentLink >= 0 && child) {
        ComponentUtilAddItemVisual(child, visualIndex, effect->m_Model);
      }
    }

    if (child) {
      HandleClose(child);
    }
  }
}

void CGPlayer_C::ItemReceived(const ItemStats *stats) const {
  if (!stats || m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
    return;
  }

  for (unsigned int spellIndex = 0; spellIndex < 5; ++spellIndex) {
    SpellRec *spell = g_spellDB.GetRecord(stats->m_spellID[spellIndex]);
    if (!spell || stats->m_spellTrigger[spellIndex]) {
      continue;
    }

    for (unsigned int effectIndex = 0; effectIndex < 3; ++effectIndex) {
      if (
          spell->m_effect[effectIndex] == 18 &&
          (spell->m_implicitTargetA[effectIndex] == 1 || spell->m_implicitTargetB[effectIndex] == 1)
      ) {
        FrameScript_SignalEvent(308);
        return;
      }
    }
  }
}
