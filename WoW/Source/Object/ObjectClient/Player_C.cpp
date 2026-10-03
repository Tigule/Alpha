#include <Base/Base.h>
#include <Gx/Gx.h>
#include <MapDefs.h>
#include <WorldClient/World.h>
#include "Net/NetClient/NetClient.h"
#include <Frame/CSimpleTop.h>
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "SoundInterface/SoundInterface.h"
#include "UIUtil/InputControl.h"
#include "Ui/WorldFrame.h"
#include "Ui/GameUI.h"

#include "Object/ObjectClient/Player_C.h"
#include "Magic/MagicClient/Spell_C.h"

#include "Client.h"
#include "Component/CharacterCustomization.h"
#include "Component/Component.h"
#include "Console/ConsoleCommand.h"
#include "Console/ConsoleClient.h"
#include "Console/ConsoleVar.h"
#include "Base/CDataAllocator.h"
#include "Base/CDataStore.h"
#include "DB/DBClient/AutoCode/AreaTriggerRec.h"
#include "DB/DBClient/AutoCode/AreaTableRec.h"
#include "DB/DBClient/AutoCode/ChrRacesRec.h"
#include "DB/DBClient/AutoCode/CreatureDisplayInfoRec.h"
#include "DB/DBClient/AutoCode/CreatureModelDataRec.h"
#include "DB/DBClient/AutoCode/EmotesRec.h"
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
#include "DB/DBClient/AutoCode/SpellCastTimesRec.h"
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
#include "UIUtil/InputControl.h"
#include "Ui/ClassTrainerFrame.h"
#include "Ui/ContainerFrame.h"
#include "Ui/GameUI.h"
#include "Ui/GuildRegistrar.h"
#include "Ui/ItemTextFrame.h"
#include "Ui/TabardCreationFrame.h"
#include "Ui/PaperDollInfoFrame.h"
#include "Ui/PetInfo.h"
#include "Ui/PartyFrame.h"
#include "Ui/QuestFrame.h"
#include "Ui/QuestLog.h"
#include "Ui/MinimapFrame.h"
#include "Ui/LootFrame.h"
#include "Ui/MerchantFrame.h"
#include "Ui/ReputationInfo.h"
#include "Ui/SpellBookFrame.h"
#include "Ui/Tutorial.h"
#include "Ui/TaxiMapFrame.h"
#include "Ui/TradeFrame.h"
#include "UIUtil/Tooltip.h"
#include "Ui/WorldFrame.h"
#include "WorldClient/AreaList.h"
#include "WorldClient/World.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"
#include "WowSvcs/WowSvcsClient/FriendList.h"

#include <stpl.h>
#include <FrameScript/FrameScript.h>
#include <Model/IModel.h>
#include <Os/OsTime.h>
#include <Os/W32/Debugging.h>
#include <Tempest/caasphere.h>
#include <Tempest/crect.h>
#include <ctype.h>
#include <malloc.h>

struct InitialSpellStruct {
  WORD  spellID;
  short slot;
};

struct TradeSkillInfo;
struct TradeSkillSubClassInfo;
struct CraftInfo;
struct CraftSkillLineInfo;
struct PetitionSignerInfo;
class CGBuffDesc;

class CGTradeSkillInfo {
 public:
  static void RefreshList(int resetFilters);

 private:
  static int                                       m_skillLine;
  static int                                       m_currentSelection;
  static UINT                                      m_itemsPending;
  static UINT                                      m_numSkills;
  static UINT                                      m_numSubClasses;
  static UINT                                      m_filteredSkills;
  static int                                       m_subClassFilter;
  static int                                       m_invTypeFilter;
  static int                                       m_collapseFilter;
  static TSGrowableArray<TradeSkillInfo *>         m_skills;
  static TSGrowableArray<TradeSkillSubClassInfo *> m_subClasses;
  static int                                       m_availableSlots;
};

class CGCraftInfo {
 public:
  static void RefreshList();

 private:
  static SPELL_CAST_UI_TYPE                    m_craftType;
  static int                                   m_currentSelection;
  static UINT                                  m_numSkills;
  static UINT                                  m_numSkillLines;
  static UINT                                  m_filteredSkills;
  static int                                   m_collapseFilter;
  static TSGrowableArray<CraftInfo *>          m_skills;
  static TSGrowableArray<CraftSkillLineInfo *> m_skillLines;
};

class CGBuffBar {
 public:
  static void UpdateDuration(BYTE slot, UINT duration);

 private:
  static CGBuffDesc m_buffs[56];
  static UINT       m_durations[56];
};

class CGBankInfo {
 public:
  static void      OpenBank(const DWORDLONG &guid);
  static DWORDLONG m_unit;
};

class CGPetitionInfo {
 public:
  static void SetPetition(DWORDLONG petition, int petitionID);
  static void SetSignatures(BYTE count, DWORDLONG *signers, int *choices);

 protected:
  static DWORDLONG                           m_petitionGUID;
  static int                                 m_petitionID;
  static TSGrowableArray<PetitionSignerInfo> m_signatures;
  static UINT                                m_numSignatures;
  static UINT                                m_pendingNames;
  static const CGPetition                   *m_petition;
};

const SkillLineAbilityRec *SpellTableLookupAbility(UINT raceID, UINT classID, UINT spellID);
void                       UnitDebugCombatLogOnEnable(int enable);
int                        InvSlotToObjAttachSlot(int invSlot);

struct ITEMSWAP {
  DWORDLONG bagA;
  DWORDLONG bagB;
  int       slotA;
  int       slotB;
  int       pendingID;

  ITEMSWAP() {
    Clear();
  }

  void Clear() {
    bagA = 0;
    bagB = 0;
    slotA = -1;
    slotB = -1;
    pendingID = 0;
  }
};

struct LootItem {
  UINT m_itemID;
  UINT m_displayID;
  UINT m_quantity;
};

struct RandomRollInfo {
  int min;
  int max;
  int result;
};

static const char  coinToken[3][16] = {"COPPER", "SILVER", "GOLD"};
static const char  NONAME[7] = "NoName";
static const UINT  DEATHBINDSOUNDID = 1141;
static int         BROADCASTTO = 5;
static const float AUTOMOVE_WALK_THRESHOLD_DISTANCE = 0.16666667f;
static const float AUTOMOVE_WALK_THRESHOLD_DISTANCE_SQ = AUTOMOVE_WALK_THRESHOLD_DISTANCE * AUTOMOVE_WALK_THRESHOLD_DISTANCE;
static const float AUTOMOVE_STOP_THRESHOLD_DISTANCE = 0.16666667f;
static const float AUTOMOVE_STOP_THRESHOLD_DISTANCE_SQ = AUTOMOVE_STOP_THRESHOLD_DISTANCE * AUTOMOVE_STOP_THRESHOLD_DISTANCE;
static TSGrowableArray<UINT> s_guildIDs;

struct ITEMEXPIRATION : public TSHashObject<ITEMEXPIRATION, CHashKeyGUID> {
  int timeLeft;
  int enchantmentTimeLeft[5];
};

NODEDECL(DEFERREDDAMAGE) {
  int       normal;
  UINT      flags;
  UINT      damage;
  DWORDLONG victim;

  void Set(int normalCombatDamage, UINT damageFlags, UINT damageAmount, DWORDLONG victimGUID) {
    normal = normalCombatDamage;
    flags = damageFlags;
    damage = damageAmount;
    victim = victimGUID;
  }
};

NODEDECL(DEFERREDSPELLMISS) {
  DWORDLONG   victim;
  MISS_REASON reason;
  int         spellID;

  void Set(DWORDLONG victimGUID, MISS_REASON missReason, int missedSpellID) {
    victim = victimGUID;
    reason = missReason;
    spellID = missedSpellID;
  }
};

enum PETITION_ERROR {
  PETITION_SUCCESS = 0,
  PETITION_ALREADY_SIGNED = 1,
  PETITION_ALREADY_IN_GUILD = 2,
  PETITION_CHARTER_CREATOR = 3,
  PETITION_NOT_ENOUGH_SIGNATURES = 4,
  PETITION_UNKNOWN_ERROR = 5
};

static TSGrowableArray<InitialSpellStruct> s_initialSpells;
static TSCArray<int, 120>                  s_initialButtons;
static GAME_ERROR_TYPE s_tabardErrors[7] = {GERR_GUILDEMBLEM_SUCCESS,       GERR_GUILDEMBLEM_INVALID_TABARD_COLORS, GERR_GUILDEMBLEM_NOGUILD,
                                            GERR_GUILDEMBLEM_COLORSPRESENT, GERR_GUILDEMBLEM_NOTGUILDMASTER,        GERR_GUILDEMBLEM_NOTENOUGHMONEY,
                                            GERR_GUILDEMBLEM_INVALIDVENDOR};
static PetitionVendorItem        petitionList[10];
static VendorItem                s_lastVendorList[128];
static int                       currentAreaTrigger;
static UINT                      s_tempCombatModeCooldown = 1000;
static UINT                      s_attackBreakTimer;
static int                       s_bindSaved;
static NTempest::C3Vector        s_bindPosition;
static UINT                      s_bindZoneID;
static TSFixedArray<UINT>        s_weaponSubclassSpells;
static UINT                      s_defenseSkillID;
static TSGrowableArray<ITEMSWAP> s_pendingSwaps;
static DWORDLONG                 s_lastVendorListReceived;
DWORDLONG                        s_lastTrainerRequest;
DWORDLONG                        s_giftWrapItem;
static DWORDLONG                 s_lastBinderID;
static int                       s_lastQuestList[32];
static int                       s_lastQuestListType[32];
static int                       s_lastQuestLevel[32];
static DWORDLONG                 s_lastQuestGiverListRevieved;
static UINT                      s_combatModeTimer;
static CVar                     *s_namePlateRenderOwn;
static float                     s_attackBreakDistanceSquared = 100.0f;
static UINT                      s_playerProficiencies[16];
static DWORDLONG                 s_resurrectOffer;
static const UINT                s_hands[NUMHANDS] = {INVSLOT_MAINHAND, INVSLOT_OFFHAND};
static const int                 s_clearLinkPointFlags[2][4] = {
    {26, 31, 32, -1},
    {27, 30, 33, 28}
};

BOOL IsSitStandSleepTransition(UINT animState);

LPCSTR CGUnit_C::GetObjectName() const {
  return GetUnitName();
}

NTempest::C3Vector CGUnit_C::GetPosition() const {
  return m_move.GetPosition();
}

void CGUnit_C::GetPosition(NTempest::C3Vector &vec) const {
  vec = m_move.GetPosition();
}

float CGUnit_C::GetFacing() const {
  return m_move.GetFacing();
}

NTempest::C3Vector CGUnit_C::GetGroundNormal() const {
  return m_move.m_groundNormal;
}

BOOL CGUnit_C::ShouldFadeIn() const {
  return !(m_unit->flags & 2);
}

BOOL CGUnit_C::IsSolidSelectable() const {
  return 1;
}

BOOL CGUnit_C::IsSolidCollidable() const {
  return 0;
}

BOOL CGUnit_C::CanBeTargetted() const {
  return CanHighlight();
}

void CGUnit_C::OnGetAttacked(DWORDLONG) {
}

void CGPlayer_C::SetBaseAnimState(UINT newState) {
  UINT oldState = m_currentBaseAnimState;
  CGUnit_C::SetBaseAnimState(newState);

  if (GetGUID() == ClntObjMgrGetActivePlayer() && oldState != newState && IsSitStandSleepTransition(newState) != IsSitStandSleepTransition(oldState))
  {
    CGInputControl::GetActive()->UpdatePlayer(OsGetAsyncTimeMs());
  }
}

void CGPlayer_C::SetEmoteState(UINT emoteID) {
  CDataStore msg;
  msg.Put(CMSG_EMOTE);
  msg.Put(static_cast<int>(emoteID));
  msg.Finalize();
  ClientServices_Send(&msg);
}

int CGPlayer_C::GetSpellCastingTime(int spellID) const {
  const SpellRec *srec = g_spellDB.GetRecord(spellID);
  if (!srec) {
    return 0;
  }
  const SpellCastTimesRec *castTime = g_spellCastTimesDB.GetRecord(srec->m_castingTimeIndex);
  if (!castTime) {
    return 0;
  }
  int baseCastingTime = castTime->m_base;
  int timePerLevel = castTime->m_perLevel;
  int result = baseCastingTime + timePerLevel * (const_cast<CGPlayer_C *>(this)->GetSpellRank(spellID) / 5);
  if (result < castTime->m_minimum) {
    result = castTime->m_minimum;
  }
  if (result > 0 && m_unit->modCastingSpeed) {
    result += result * m_unit->modCastingSpeed / 100;
  }
  if (srec->m_attributes & 2) {
    CGItem_C *rangedItem = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(m_inventory.GetItem(17), __FILE__, __LINE__));
    if (rangedItem) {
      const ItemStats_C *stats = g_itemDBCache.GetRecord(rangedItem->GetEntryID(), m_obj->m_guid, 0, 0);
      if (stats) {
        result += stats->m_delay;
      }
    }
  }
  return result > 0 ? result : 0;
}

void CGPlayer_C::CleanupUnitArtwork(int playerModelChanged, BOOL wasPlayerModel) {
  CGUnit_C::CleanupUnitArtwork(playerModelChanged, wasPlayerModel);
}

void CGPlayer_C::ReinitializeUnitArtwork() {
  CGUnit_C::ReinitializeUnitArtwork();
  FATALASSERT(!m_texComponent);
  FATALASSERT(!m_geosetHandle);
  for (UINT attachment = 0; attachment < 36; ++attachment) {
    if (attachment >= 5 && (attachment <= 6 || attachment == 11)) {
      for (UINT component = 0; component < NUM_INVENTORY_SLOTS; ++component) {
        if (m_components[component][attachment]) {
          AddAttachment(m_model, attachment, m_components[component][attachment], 1.0f);
        }
      }
    } else {
      for (UINT component = 0; component < NUM_INVENTORY_SLOTS; ++component) {
        if (m_components[component][attachment]) {
          ModelAddLink(m_model, attachment, m_components[component][attachment], 1.0f);
        }
      }
    }
  }
  if (m_unit->weaponMode == WEAPONMODE_SHEATHEDMODE) {
    for (UINT hand = 0; hand < 2; ++hand) {
      if (ClntObjMgrObjectPtr(m_inventory.GetItem(s_hands[hand]), __FILE__, __LINE__)) {
        SheatheObjComponent(s_hands[hand], 1);
      }
    }
  }
  InitComponents();
  if (m_geosetHandle) {
    CharCustomizationCommitItemGeosets(m_geosetHandle, 0, m_paperDollModel);
    Animate();
  }
}

void CGPlayer_C::PostReinitializeArtwork() {
  CGUnit_C::PostReinitializeArtwork();
  if (!m_texComponent) {
    return;
  }
  for (UINT slot = 0; slot < NUM_INVENTORY_SLOTS; ++slot) {
    if (m_texComponentInfo[slot].m_displayID && m_texComponentInfo[slot].m_inventoryType) {
      CStatus status;
      TexComponentAdd(
          &status,
          m_unit->sex,
          m_texComponent,
          g_itemDisplayInfoDB.GetRecord(m_texComponentInfo[slot].m_displayID),
          m_texComponentInfo[slot].m_inventoryType,
          1
      );
      if (!status.IsEmpty()) {
        char buffer[512];
        status.GetErrorStr(buffer, sizeof(buffer), STATUS_INFO);
        NTempest::C3Vector pos = GetPosition();
        FATALERROR(("Player: 0x%I64X(%s)(%g,%g,%g): %s", GetGUID(), GetUnitName(), pos.x, pos.y, pos.z, buffer));
      }
      CharCustomizationAddItemGeosets(
          m_geosetHandle,
          g_itemDisplayInfoDB.GetRecord(m_texComponentInfo[slot].m_displayID),
          m_texComponentInfo[slot].m_inventoryType,
          m_texComponent,
          m_unit->race,
          1
      );
    }
  }
  CGItem_C *head = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(GetBag()->GetItem(0), __FILE__, __LINE__));
  if (head) {
    HeadGeosetHideCharGeosets(m_geosetHandle, g_itemDisplayInfoDB.GetRecord(head->GetDisplayID()), m_unit->race, m_preferredGeosets, 15);
  }
}

void          ModelShowBoundingSphere(HMODEL model);
static LPCSTR s_actionsArray[18] = {
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
static const GAME_ERROR_TYPE s_playerBankErrors[3] = {
  GERR_BANKSLOT_FAILED_TOO_MANY,
  GERR_BANKSLOT_INSUFFICIENT_FUNDS,
  GERR_BANKSLOT_NOTBANKER
};
static GAME_ERROR_TYPE s_mountResultGameErrors[11] = {
    GERR_MOUNT_INVALIDMOUNTEE,
    GERR_MOUNT_TOOFARAWAY,
    GERR_MOUNT_ALREADYMOUNTED,
    GERR_MOUNT_NOTMOUNTABLE,
    GERR_MOUNT_NOTYOURPET,
    GERR_MOUNT_OTHER,
    GERR_MOUNT_LOOTING,
    GERR_MOUNT_RACECANTMOUNT,
    GERR_MOUNT_SHAPESHIFTED,
    GERR_MOUNT_FORCEDDISMOUNT,
    GERR_NONE
};
static GAME_ERROR_TYPE s_dismountResultGameErrors[4] = {GERR_DISMOUNT_NOPET, GERR_DISMOUNT_NOTMOUNTED, GERR_DISMOUNT_NOTYOURPET, GERR_NONE};
static GAME_ERROR_TYPE s_taxiErrors[12] = {
    GERR_NONE,
    GERR_TAXIUNSPECIFIEDSERVERERROR,
    GERR_TAXINOSUCHPATH,
    GERR_TAXINOTENOUGHMONEY,
    GERR_TAXITOOFARAWAY,
    GERR_TAXINOVENDORNEARBY,
    GERR_TAXINOTVISITED,
    GERR_TAXIPLAYERBUSY,
    GERR_TAXIPLAYERALREADYMOUNTED,
    GERR_TAXIPLAYERSHAPESHIFTED,
    GERR_TAXIPLAYERMOVING,
    GERR_TAXISAMENODE
};
static TSHashTable<ITEMEXPIRATION, CHashKeyGUID> s_pendingItemExpirations;
static LISTDECL(DEFERREDDAMAGE, s_deferredDamage);
static LISTDECL(DEFERREDSPELLMISS, s_deferredSpellMiss);
static TInstanceAllocator<DEFERREDDAMAGE>    s_freeDeferedDamage(100);
static TInstanceAllocator<DEFERREDSPELLMISS> s_freeDeferredSpellMiss(100);
static int                                   s_loginCinematicID;
static const int                             CHARACTER_POINTS_PER_LEVEL[2] = {10, 1};
static const int                             CHARACTER_POINTS_PER_BONUS[2] = {0, 1};
static const int                             LEVELS_PER_CHARACTER_POINT_BONUS[2] = {1, 5};
static const int                             CHARACTER_POINT_LEVEL_THRESHOLDS[2] = {10, 1};
static const int                             CHARACTER_POINT_BONUS_PER_THRESHOLD[2] = {5, 0};
static UINT                                  s_areaTriggerCheck_TimerEvent;
static int                                   s_enableDeathHoldLog;
static int                                   s_renderPlayer;
static UINT                                  s_numLootItems;
#define MAX_LOOT_ITEMS 16

static LootItem                              s_lootItems[MAX_LOOT_ITEMS];
static int                                   s_questFailedReason;
CVar                                        *g_combatModeMaxDistance;

void UnitCombatLogSetActivePlayer(const CGPlayer_C *playerPtr);
void UnitCombatLogSpellMissed(UINT missReason, UINT spellID, DWORDLONG caster, DWORDLONG victim);

enum CURSORANIMATIONS {
  POINT_CURSOR = 0,
  CAST_CURSOR = 1
};

void CursorSetCursorMode(CURSORANIMATIONS mode);

void Spell_C_CancelSpell(bool failed, bool notifyServer, SPELL_FAILED_REASON reason);
void Spell_C_CancelCombatSpell();
int  Spell_C_GetTargettingSpell();
void Spell_C_SetCooldownLeft(
    int  spellID,
    int  itemID,
    int  category,
    int  recoveryLeft,
    int  categoryRecoveryLeft,
    bool needsEvent,
    BOOL isPet,
    int  startRecoveryTimeLeft
);
void                   PlayerInitializeSounds();
void                   PlayerShutdownSounds();
const ItemSubClassRec *SDBItemSubclassGetSubClassRec(UINT classID, UINT subClassID);
int                    SheatheTypeToSheathePoint(int sheatheType, int invSlot);
void                   Script_SendUnitSignal(const DWORDLONG &guid, int signal);
UINT                   Trade_C_GetPlayerTradeGold();
void                   Trade_C_RemoveMoney(UINT money);

void   CurrencyBreakdown(int money, int coins[]);
LPCSTR CurrencyAbbreviation(int coinType);

static BOOL OnPlayerEvent(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
static void ShowForceActionFlags(UINT flags[]);
static BOOL OnVendorEvent(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
static BOOL OnLootEvent(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
static BOOL OnLearnedSpell(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
static BOOL OnSupercededSpell(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
static BOOL OnInitialSpells(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
static BOOL OnActionButtons(LPVOID, NETMESSAGE, DWORD, CDataStore *msg);
static BOOL OnPetSpells(LPVOID, NETMESSAGE, DWORD, CDataStore *msg);
static BOOL OnGroupInvite(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
static BOOL OnGroupCancel(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
static BOOL OnGroupDecline(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
static BOOL OnGroupUninvite(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
static BOOL OnGroupNewLeader(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
static BOOL OnGroupDestroy(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
static BOOL OnGroupCommandResult(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
static BOOL OnGroupList(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
static BOOL OnQuestGiverEvent(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
static BOOL OnTrainerEvent(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
static BOOL OnProficiency(LPVOID, NETMESSAGE, DWORD, CDataStore *msg);
static BOOL OnResurrectRequest(LPVOID, NETMESSAGE, DWORD, CDataStore *msg);
static BOOL OnInspectNotify(LPVOID, NETMESSAGE, DWORD, CDataStore *msg);
static BOOL OnFactionUpdate(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
static BOOL OnReadItemResult(LPVOID, NETMESSAGE msgID, DWORD, CDataStore *msg);
static BOOL OnCancelCombat(LPVOID, NETMESSAGE, DWORD, CDataStore *);
static BOOL OnGuildInvite(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
static BOOL OnGuildDecline(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
static BOOL OnGuildInfo(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
static BOOL OnGuildRoster(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
static BOOL OnGuildEvent(LPVOID, NETMESSAGE, DWORD, CDataStore *msg);
static BOOL OnGuildCommandResult(LPVOID, NETMESSAGE, DWORD, CDataStore *msg);
static BOOL OnGuildEmblemError(LPVOID, NETMESSAGE, DWORD, CDataStore *msg);
static BOOL OnGuildEmblemActivate(LPVOID, NETMESSAGE, DWORD, CDataStore *msg);
static BOOL OnNpcPetitionEvent(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
static BOOL OnPlayEmote(LPVOID, NETMESSAGE, DWORD, CDataStore *msg);
static BOOL HandlePartyMemberStats(LPVOID, NETMESSAGE, DWORD, CDataStore *msg);
static BOOL OnQuestUpdate(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
static BOOL OnQuestConfirm(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
static BOOL OnMirrorTimerEvent(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
static BOOL OnItemEvent(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);

static BOOL BootMeHandler(LPCSTR command, LPCSTR arguments);
static BOOL RepopPlayerHandler(LPCSTR command, LPCSTR arguments);
static BOOL WhoCommandHandler(LPCSTR command, LPCSTR arguments);
void CGPlayer_C::XBuyItemInSlot(DWORDLONG merchant, UINT itemID, BYTE quantity, DWORDLONG container, BYTE slot) {
  CDataStore buyMsg;
  buyMsg.Put(CMSG_BUY_ITEM_IN_SLOT);
  buyMsg.Put(merchant);
  buyMsg.Put(itemID);
  buyMsg.Put(container);
  buyMsg.Put(slot);
  buyMsg.Put(quantity);
  buyMsg.Finalize();
  ClientServices_Send(&buyMsg);
}

static BOOL BuyCommandHandler(LPCSTR command, LPCSTR arguments);
static BOOL UndressMeHandler(LPCSTR command, LPCSTR arguments);
static BOOL GodmodeHandler(LPCSTR command, LPCSTR arguments);
static BOOL CCommand_LevelUp(LPCSTR command, LPCSTR arguments);
static BOOL CCommand_SetFaction(LPCSTR command, LPCSTR arguments);
static BOOL CCommand_Invite(LPCSTR command, LPCSTR arguments);
static BOOL CCommand_Accept(LPCSTR command, LPCSTR arguments);
static BOOL CCommand_Decline(LPCSTR command, LPCSTR arguments);
static BOOL CCommand_Disband(LPCSTR command, LPCSTR arguments);
static BOOL CCommand_NewLeader(LPCSTR command, LPCSTR arguments);
static BOOL CCommand_Uninvite(LPCSTR command, LPCSTR arguments);
static BOOL CCommand_AcceptRes(LPCSTR, LPCSTR);
static BOOL CCommand_DeclineRes(LPCSTR, LPCSTR);
static BOOL CCommand_ShowPet(LPCSTR, LPCSTR);
static BOOL CCommand_TaxiShowNodes(LPCSTR, LPCSTR);
static BOOL CCommand_GuildCreate(LPCSTR command, LPCSTR arguments);
static BOOL CCommand_TogglePVP(LPCSTR command, LPCSTR arguments);
static BOOL CCommand_Cinematic(LPCSTR command, LPCSTR arguments);
static BOOL CCommand_ForceActionSet(LPCSTR command, LPCSTR arguments);
static BOOL CCommand_ForceActionUnset(LPCSTR command, LPCSTR arguments);
static BOOL CCommand_ForceActionOnOtherSet(LPCSTR command, LPCSTR arguments);
static BOOL CCommand_ForceActionOnOtherUnset(LPCSTR command, LPCSTR arguments);
static BOOL CCommand_ForceActionShowFlags(LPCSTR command, LPCSTR arguments);
static BOOL CCommand_ForceMonsterAnim(LPCSTR, LPCSTR arguments);
static BOOL CCommand_ResetMonsterAnim(LPCSTR, LPCSTR);
static BOOL CCommand_DumpDeathHoldLogs(LPCSTR, LPCSTR);

static BOOL        AreaTriggerCheck(LPCVOID eventData, LPVOID arg);
static void AreaTriggersInitialize();
static void AreaTriggersShutdown();

static BOOL CountWeaponItemSubclasses(int *number);
static BOOL FindFirstSetBit(UINT field, int *whichBitSet);
static void InitializeWeaponSubclassSpells();

static BOOL PlayerAttackBreakHandler(LPCVOID data, DWORDLONG guid, LPVOID param) {
  if (ClntObjMgrGetPlayerType() != PLAYER_BOT) {
    FATALASSERT(s_attackBreakTimer);
    s_attackBreakTimer = 0;

    CGPlayer_C *playerPtr = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
    FATALASSERT(playerPtr);
    FATALASSERT(playerPtr->GetGUID() == CGPlayer_C::GetActive());

    if (!playerPtr->OnAttackBreakHandler()) {
      playerPtr->StopAttack();
      return 1;
    }

    s_attackBreakTimer = ClientSetTimer(500, PlayerAttackBreakHandler, guid, param);
  }

  return 1;
}

static BOOL PlayerCombatModeHandler(LPCVOID data, DWORDLONG guid, LPVOID param) {
  s_combatModeTimer = 0;
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (unit) {
    unit->OnCombatModeTimer();
  }
  return 1;
}

BOOL Player_C_ZoneUpdateHandler(LPCVOID eventData, LPVOID arg) {
  static int ticks;

  if (++ticks < 10) {
    return 1;
  }

  ticks -= 10;
  DWORDLONG   guid = ClntObjMgrGetActivePlayer();
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (player) {
    AreaListRegisterLocation(player->GetPosition(), ClntObjMgrGetMapID(), player->GetWorldObject());
  }

  return 1;
}

static BOOL RepopPlayerHandler(LPCSTR command, LPCSTR arguments) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->HandleRepopRequest();
  }
  return 1;
}

static BOOL WhoCommandHandler(LPCSTR command, LPCSTR arguments) {
  g_friendList->SendWho(arguments);
  return 1;
}



static void RandomRollNameQueryCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted) {
  char            buf[256];
  RandomRollInfo *info = static_cast<RandomRollInfo *>(arg);
  FATALASSERT(arg);

  if (granted) {
    const NameCache *name = g_nameDBCache.GetRecord(guid, 0, 0, 0);
    if (name) {
      SStrPrintf(
          buf, sizeof(buf), FrameScript_GetText("RANDOM_ROLL_RESULT", -1, GENDER_NOT_APPLICABLE), name->m_name, info->result, info->min, info->max
      );
      CGChat::AddChatMessage(buf, SLASH_CMD_SYSTEM, 0, 0, 0, 0, 0);
    }
  }

  delete info;
}

static BOOL BankInvHandler(DWORDLONG guid, UINT offset, UINT bytes, LPCVOID prevValue, LPVOID param) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (player) {
    offset /= 8;
    DWORDLONG currGuid = player->Inventory()->GetItem(offset);
    if (*static_cast<const DWORDLONG *>(prevValue) != currGuid) {
      CGGameUI::UnlockItem(currGuid);
    }
    FrameScript_SignalEvent(326);
  }
  return 1;
}

static BOOL OnPlayerEvent(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  FATALASSERT(msg);

  switch (msgId) {
    case SMSG_TRIGGER_CINEMATIC: {
      int cinematicID;
      msg->Get(cinematicID);
      if (ClientServices_CharacterIsInGame()) {
        CGGameUI::StartCinematic(cinematicID);
      } else {
        s_loginCinematicID = cinematicID;
      }
      return 1;
    }

    case SMSG_GODMODE: {
      BYTE enabled = 0;
      msg->Get(enabled);
      ConsoleWrite(enabled ? "Godmode enabled" : "Godmode disabled", DEFAULT_COLOR);
      return 1;
    }

    case SMSG_FORCEACTIONSHOW: {
      UINT flags[2];
      msg->Get(flags[0]);
      msg->Get(flags[1]);
      ShowForceActionFlags(flags);
      return 1;
    }

    case SMSG_INVENTORY_CHANGE_FAILURE: {
      BYTE bagResult = 0;
      msg->Get(bagResult);
      BAG_RESULT result = static_cast<BAG_RESULT>(bagResult);
      if (result == BAG_OK) {
        return 1;
      }

      GAME_ERROR_TYPE error = CGBag_C::GetGameError(result);
      int             level = 0;
      if (result == BAG_LEVEL_MISMATCH) {
        msg->Get(level);
      }

      DWORDLONG item1;
      DWORDLONG item2;
      BYTE      containerBSlot;
      msg->Get(item1);
      msg->Get(item2);
      msg->Get(containerBSlot);

      switch (result) {
        case BAG_ITEM_SUBTYPE_MISMATCH: {
          CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
          if (player && player->ReportBagItemSubtypeMismatch(containerBSlot)) {
            break;
          }
        }
        default:
          CGGameUI::DisplayError(error);
          break;
        case BAG_LEVEL_MISMATCH:
          CGGameUI::DisplayError(error, level);
          break;
      }

      if (item1) {
        CGGameUI::UnlockItem(item1);
      }
      if (item2) {
        CGGameUI::UnlockItem(item2);
      }
      return 1;
    }

    case SMSG_HEALSPELL_ON_PLAYER: {
      int amount;
      msg->Get(amount);
      CGGameUI::ShowHealingFeedback(ClntObjMgrGetActivePlayer(), amount);
      return 1;
    }

    case SMSG_UPDATE_AURA_DURATION: {
      BYTE slot;
      int  duration;
      msg->Get(slot);
      msg->Get(duration);
      CGBuffBar::UpdateDuration(slot, duration);
      return 1;
    }

    case SMSG_OPEN_CONTAINER: {
      DWORDLONG container;
      msg->Get(container);
      CGContainerInfo::OpenContainer(container);
      return 1;
    }

    case SMSG_HEALSPELL_ON_PLAYERS_PET: {
      DWORDLONG petGUID;
      int       amount;
      msg->Get(petGUID);
      msg->Get(amount);
      CGGameUI::ShowHealingFeedback(petGUID, amount);
      return 1;
    }

    case SMSG_NEW_TAXI_PATH:
      CGGameUI::DisplayError(GERR_NEWTAXIPATH);
      CGPlayer_C::UpdateTaxiStatusAll();
      return 1;

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
      if (AreaListGetName(mapID, (areaID >> 16) & 0xFFFF, areaID & 0xFFFF, buf, sizeof(buf), 0)) {
        SStrPrintf(string, sizeof(string), FrameScript_GetText("BIND_ZONE_DISPLAY", -1, GENDER_NOT_APPLICABLE), buf);
        CGChat::AddChatMessage(string, SLASH_CMD_SYSTEM, 0, 0, 0, 0, 0);
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

    case SMSG_SHOWTAXINODES: {
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (player) {
        player->ShowTaxiNodes(msg);
      }
      return 1;
    }

    case SMSG_ACTIVATETAXIREPLY: {
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (player) {
        UINT error = 0;
        msg->Get(error);
        player->HandleActivateTaxiReply(error);
      }
      return 1;
    }

    case SMSG_ITEM_PUSH_RESULT: {
      DWORDLONG player;
      int       pushed;
      int       displayText;
      BYTE      slot;
      int       itemID;
      msg->Get(player);
      msg->Get(pushed);
      msg->Get(displayText);
      msg->Get(slot);
      msg->Get(itemID);
      CGGameUI::OnItemPush(player, slot, itemID, pushed, displayText);
      return 1;
    }

    case SMSG_MOUNTRESULT: {
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (player) {
        UINT error = 0;
        msg->Get(error);
        player->HandleMountResult(error);
      }
      return 1;
    }

    case SMSG_DISMOUNTRESULT: {
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (player) {
        UINT error = 0;
        msg->Get(error);
        player->HandleDismountResult(error);
      }
      return 1;
    }

    case SMSG_DEATH_NOTIFY: {
      DWORDLONG unit;
      msg->Get(unit);
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (player) {
        player->SaveDeathMessage(unit);
      }
      return 1;
    }

    case SMSG_PLAYERBINDERROR:
      CGGameUI::DisplayError(GERR_DEATHBINDALREADYBOUND);
      return 1;

    case SMSG_PLAYERBOUND: {
      DWORDLONG binderID;
      msg->Get(binderID);
      s_lastBinderID = binderID;
      SndInterfacePlaySound(0x475, -1);
      CGChat::AddChatMessage(FrameScript_GetText("DEATHBIND_SUCCESSFUL", -1, GENDER_NOT_APPLICABLE), SLASH_CMD_SYSTEM, 0, 0, 0, 0, 0);
      return 1;
    }

    case SMSG_PET_NAME_INVALID:
      CGGameUI::DisplayError(GERR_INVALID_PETNAME);
      return 1;

    case SMSG_SHOW_BANK: {
      DWORDLONG guid;
      msg->Get(guid);
      CGBankInfo::OpenBank(guid);
      return 1;
    }

    case SMSG_BUY_BANK_SLOT_RESULT: {
      UINT error;
      msg->Get(error);
      if (error < 3) {
        CGGameUI::DisplayError(s_playerBankErrors[error]);
      }
      return 1;
    }

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

      BYTE race = player->GetRace();
      for (UINT index = 0; index < 2; ++index) {
        points[index] = CHARACTER_POINTS_PER_LEVEL[index];
        if (race == 1 && level % LEVELS_PER_CHARACTER_POINT_BONUS[index] == 0) {
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
      DWORDLONG          sender;
      NTempest::C2Vector pos;
      msg->Get(sender);
      msg->Get(pos.x);
      msg->Get(pos.y);
      CGMinimapFrame::SetPingPosition(sender, pos);
      return 1;
    }

    case SMSG_PLAYER_MACRO: {
      DWORDLONG player;
      int       category;
      msg->Get(player);
      msg->Get(category);
      CGPlayer_C *playerPtr = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(player, __FILE__, __LINE__));
      if (playerPtr) {
        playerPtr->PlayVocalMacro(category);
      }
      return 1;
    }

    case SMSG_EXPLORATION_EXPERIENCE: {
      int areaID;
      int experience;
      msg->Get(areaID);
      msg->Get(experience);
      if (experience > 0) {
        const AreaTableRec *area = g_areaTableDB.GetRecord(areaID);
        if (area) {
          CGGameUI::DisplayError(GERR_ZONE_EXPLORED, area->m_AreaName_lang[CURRENT_LANGUAGE]);
          CGGameUI::DisplayError(GERR_ZONE_EXPLORED_XP, area->m_AreaName_lang[CURRENT_LANGUAGE], experience);
        }
      }
      return 1;
    }

    case MSG_RANDOM_ROLL: {
      int       min;
      DWORDLONG sender;
      int       max;
      int       result;
      char      buf[256];
      msg->Get(min);
      msg->Get(max);
      msg->Get(result);
      msg->Get(sender);

      RandomRollInfo *info = NEW(RandomRollInfo);
      const NameCache *name = g_nameDBCache.GetRecord(sender, sender, RandomRollNameQueryCallback, &info);
      if (name) {
        SStrPrintf(buf, sizeof(buf), FrameScript_GetText("RANDOM_ROLL_RESULT", -1, GENDER_NOT_APPLICABLE), name->m_name, result, min, max);
        CGChat::AddChatMessage(buf, SLASH_CMD_SYSTEM, 0, 0, 0, 0, 0);
        delete info;
      }
      return 1;
    }

    case SMSG_FISH_NOT_HOOKED:
      CGGameUI::DisplayError(GERR_FISH_NOT_HOOKED);
      return 1;

    case SMSG_FISH_ESCAPED:
      CGGameUI::DisplayError(GERR_FISH_ESCAPED);
      return 1;

    default:
      return 0;
  }
}

static void ShowForceActionFlags(UINT flags[]) {
  static LPCSTR flagName[2] = {"Self force flags:", "Victim force flags:"};
  for (UINT target = 0; target < 2; ++target) {
    ConsoleWrite(flagName[target], DEFAULT_COLOR);
    for (UINT action = 0; action < 18; ++action) {
      ConsolePrintf("[%d] %s: %s", action, s_actionsArray[action], flags[target] & (1 << action) ? "on" : "off");
    }
  }
}

static BOOL OnItemEvent(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  FATALASSERT(msg);

  DWORDLONG itemGUID;
  msg->Get(itemGUID);
  CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(itemGUID, __FILE__, __LINE__));

  switch (msgId) {
    case SMSG_ITEM_ENCHANT_TIME_UPDATE: {
      int timeLeft;
      int slot;
      msg->Get(slot);
      msg->Get(timeLeft);
      if (item) {
        item->UpdateEnchantmentTime(slot, timeLeft);
      } else {
        ITEMEXPIRATION *itemNode = CGPlayer_C::GetPendingItemExpirationNode(itemGUID);
        FATALASSERT(itemNode);
        FATALASSERT((slot >= 0) && (slot < (sizeof(itemNode->enchantmentTimeLeft) / sizeof(itemNode->enchantmentTimeLeft[0]))));
        itemNode->enchantmentTimeLeft[slot] = timeLeft;
      }
      return 1;
    }
    case SMSG_ITEM_TIME_UPDATE: {
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
    default:
      return 0;
  }
}

static BOOL OnNpcPetitionEvent(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  DWORDLONG activePlayer = ClntObjMgrGetActivePlayer();
  if (!activePlayer) {
    return 0;
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(activePlayer, __FILE__, __LINE__));
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

static BOOL HandlePartyMemberStats(LPVOID, NETMESSAGE, DWORD, CDataStore *msg) {
  DWORDLONG guid;
  int       maxPower;
  msg->Get(guid);

  CGPartyInfo::RemoteStats *stats = CGPartyInfo::GetRemoteStats(guid);
  if (stats) {
    maxPower = stats->maxPower;
    msg->Get(stats->health);
    msg->Get(stats->maxHealth);
    BYTE powerType = 0;
    msg->Get(powerType);
    stats->powerType = static_cast<POWER_TYPE>(powerType);
    msg->Get(stats->power);
    msg->Get(stats->maxPower);
    msg->Get(stats->level);
    msg->Get(stats->mapID);
    msg->Get(stats->areaID);
    msg->Get(stats->classID);
    *msg >> stats->pos;

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

static BOOL OnVendorEvent(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  DWORDLONG activePlayer = ClntObjMgrGetActivePlayer();
  if (!activePlayer) {
    return 0;
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(activePlayer, __FILE__, __LINE__));
  if (!player) {
    return 0;
  }

  switch (msgId) {
    case SMSG_LIST_INVENTORY:
      return player->OnVendorInventory(msg);
    case SMSG_BUY_FAILED:
      return player->OnBuyFailed(msg);
    case SMSG_BUY_ITEM:
      return player->OnBuySucceeded(msg);
    case SMSG_SELL_ITEM:
      return player->OnSellResponse(msg);
    default:
      return 0;
  }
}

static BOOL OnFactionUpdate(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
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

static BOOL OnQuestGiverEvent(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  DWORDLONG activePlayer = ClntObjMgrGetActivePlayer();
  if (!activePlayer) {
    return 0;
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(activePlayer, __FILE__, __LINE__));
  if (!player) {
    return 0;
  }

  switch (msgId) {
    case SMSG_QUESTGIVER_QUEST_LIST:
      return player->OnQuestGiverListQuests(msg);
    case SMSG_QUESTGIVER_QUEST_INVALID:
      return player->OnQuestGiverInvalidQuest(msg);
    case SMSG_QUESTGIVER_QUEST_DETAILS:
      return player->OnQuestGiverSendQuest(msg);
    case SMSG_QUESTGIVER_REQUEST_ITEMS:
      return player->OnQuestGiverRequestItems(msg);
    case SMSG_QUESTGIVER_OFFER_REWARD:
      return player->OnQuestGiverChooseReward(msg);
    case SMSG_QUESTGIVER_QUEST_COMPLETE:
      return player->OnQuestGiverQuestComplete(msg);
    case SMSG_QUESTGIVER_QUEST_FAILED:
      return player->OnQuestGiverQuestFailed(msg);
    case SMSG_QUESTGIVER_STATUS:
      return player->OnQuestGiverStatus(msg);
    case SMSG_QUESTLOG_FULL:
      CGGameUI::DisplayError(GERR_QUEST_LOG_FULL);
      return 1;
    default:
      return 0;
  }
}

static BOOL OnQuestItemLoot(const QuestCache *quest, int itemID, int quantity) {
  const ItemStats *stats;

  if (quest) {
    for (UINT index = 0; index < 4; ++index) {
      if (quest->m_itemToGet[index] == itemID) {
        stats = g_itemDBCache.GetRecord(itemID, 0, 0, 0);
        if (stats) {
          CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
          if (player) {
            int killed = player->CGPlayer_C::GetBag()->GetItemTypeCount(itemID, 8);
            if (killed >= quest->m_itemToGetQuantity[index]) {
              return 1;
            }

            UINT pluralIndex = FrameScript_GetPluralIndex(quest->m_itemToGetQuantity[index]);
            CGGameUI::DisplayError(
                GERR_QUEST_ADD_ITEM_SII,
                stats->m_displayName[pluralIndex],
                min(killed + quantity, quest->m_itemToGetQuantity[index]),
                quest->m_itemToGetQuantity[index]
            );
          }
        }
        return 1;
      }
    }
  }
  return 0;
}

static void QuestLootQuestQueryCallback(int id, const DWORDLONG &, LPVOID arg, bool granted) {
  int *item = static_cast<int *>(arg);
  if (granted) {
    OnQuestItemLoot(g_questDBCache.GetRecord(id, 0, 0, 0), item[0], item[1]);
  }
  delete item;
}

static BOOL OnQuestUpdate(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  int questID = 0;

  if (msgId != SMSG_QUESTUPDATE_ADD_ITEM) {
    msg->Get(questID);
  }

  const QuestCache *quest = g_questDBCache.GetRecord(questID, 0, 0, 0);
  if (!quest) {
    ConsoleWriteA("Unkown questID (%d) in QuestUpdate", DEFAULT_COLOR, questID);
  }

  switch (msgId) {
    case SMSG_QUESTUPDATE_FAILED:
      if (quest) {
        CGGameUI::DisplayError(GERR_QUEST_FAILED_S, quest->m_logTitle);
      }
      return 1;

    case SMSG_QUESTUPDATE_COMPLETE:
      if (quest) {
        if (quest->m_areaDescription && quest->m_areaDescription[0]) {
          CGGameUI::DisplayError(GERR_QUEST_OBJECTIVE_COMPLETE_S, quest->m_areaDescription);
        } else {
          CGGameUI::DisplayError(GERR_QUEST_UNKNOWN_COMPLETE);
        }
      }
      return 1;

    case SMSG_QUESTUPDATE_ADD_KILL: {
      int       numNeeded;
      int       numKilled;
      int       monsterID;
      DWORDLONG monsterGUID;
      msg->Get(monsterID);
      msg->Get(numKilled);
      msg->Get(numNeeded);
      msg->Get(monsterGUID);

      if (monsterID < 0) {
        for (UINT index = 0; index < 4; ++index) {
          if (quest->m_monsterToKill[index] == monsterID) {
            CGGameUI::DisplayError(GERR_QUEST_ADD_FOUND_SII, quest->m_getDescription[index], numKilled, numNeeded);
            return 1;
          }
        }

        const GameObjectStats_C *stats = g_gameObjectDBCache.GetRecord(monsterID & 0x7FFFFFFF, 0, 0, 0);
        if (stats) {
          UINT pluralIndex = FrameScript_GetPluralIndex(numNeeded);
          CGGameUI::DisplayError(GERR_QUEST_ADD_FOUND_SII, stats->m_name[pluralIndex], numKilled, numNeeded);
        }
        return 1;
      }

      CGUnit_C *victimPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(monsterGUID, __FILE__, __LINE__));
      if (victimPtr && victimPtr->IsA(ID_UNIT) && victimPtr->m_deathHolds) {
        FATALASSERT(victimPtr->GetEntryID() == monsterID);
        victimPtr->SaveQuestAddItemMessage(numKilled, numNeeded);
        return 1;
      }

      const CreatureStats_C *stats = g_creatureDBCache.GetRecord(monsterID, 0, 0, 0);
      if (stats) {
        UINT pluralIndex = FrameScript_GetPluralIndex(numNeeded);
        CGGameUI::DisplayError(GERR_QUEST_ADD_KILL_SII, stats->m_name[pluralIndex], numKilled, numNeeded);
      }
      return 1;
    }

    case SMSG_QUESTUPDATE_ADD_ITEM: {
      int quantity;
      int itemID;
      msg->Get(itemID);
      msg->Get(quantity);
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (!player) {
        return 1;
      }

      for (UINT index = 0; index < 16; ++index) {
        const CQuestLogData *questLog = player->GetQuestLogData(index);
        if (questLog && questLog->m_questID > 0) {
          int *item = static_cast<int *>(ALLOC(2 * sizeof(int)));
          item[0] = itemID;
          item[1] = quantity;
          const QuestCache *itemQuest = g_questDBCache.GetRecord(questLog->m_questID, 0, QuestLootQuestQueryCallback, item);
          if (itemQuest) {
            delete item;
            if (OnQuestItemLoot(itemQuest, itemID, quantity)) {
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

static BOOL OnQuestConfirm(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  char      questTitle[1024];
  DWORDLONG initiatedBy;
  int       questID;

  if (msgId == SMSG_QUEST_CONFIRM_ACCEPT) {
    msg->Get(questID);
    msg->GetString(questTitle, sizeof(questTitle));
    msg->Get(initiatedBy);
    CGQuestInfo::ConfirmAcceptQuest(questID, questTitle, initiatedBy);
  }
  return 1;
}

static BOOL OnTrainerEvent(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  DWORDLONG activePlayer = ClntObjMgrGetActivePlayer();
  if (!activePlayer) {
    return 0;
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(activePlayer, __FILE__, __LINE__));
  if (!player) {
    return 0;
  }

  switch (msgId) {
    case SMSG_TRAINER_BUY_FAILED: {
      DWORDLONG trainer;
      int       spellID;
      int       reason;
      msg->Get(trainer);
      msg->Get(spellID);
      msg->Get(reason);

      if (reason == 0) {
        ConsoleWriteA("Trainer service %d unavailable", DEFAULT_COLOR, spellID);
      } else if (reason == 1) {
        ConsoleWriteA("Not enough money for trainer service %d", DEFAULT_COLOR, spellID);
      } else if (reason == 2) {
        ConsoleWriteA("Not enough skill points for trainer service %d", DEFAULT_COLOR, spellID);
      }
      return 1;
    }
    case SMSG_TRAINER_LIST:
      return player->OnTrainerList(msg);
    default:
      return 0;
  }
}

static BOOL OnLootEvent(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
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
    case MSG_SPLIT_MONEY:
      return playerPtr->OnSplitMoneyNotify(msg);
    case SMSG_LOOT_ITEM_NOTIFY:
      return playerPtr->OnLootItemNotify(msg);
    case SMSG_LOOT_CLEAR_MONEY:
      return playerPtr->OnLootClearMoney(msg);
    default:
      return 0;
  }
}

static BOOL BootMeHandler(LPCSTR command, LPCSTR arguments) {
  CDataStore msg;
  msg.Put(CMSG_BOOTME);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static BOOL OnLearnedSpell(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  short slot;
  WORD  spell;

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

static BOOL OnSupercededSpell(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  WORD oldSpell;
  WORD newSpell;

  msg->Get(oldSpell);
  msg->Get(newSpell);

  CGActionBar::ReplaceSpell(oldSpell, newSpell);
  CGSpellBook::ReplaceSpell(oldSpell, newSpell);

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->DelKnownSpell(oldSpell);
    player->AddKnownSpell(newSpell, 0, 0, 0);
  } else {
    UINT index;
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

static BOOL OnInitialSpells(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  BYTE initial;
  WORD count;
  int  categoryRecoveryTime;
  int  recoveryTime;
  WORD itemID;
  WORD category;
  WORD spellID;
  bool onHold;
  WORD index;

  msg->Get(initial);
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
    onHold = (static_cast<UINT>(categoryRecoveryTime) >> 31) & 1;
    categoryRecoveryTime &= 0x7FFFFFFF;
    Spell_C_SetCooldownLeft(spellID, itemID, category, recoveryTime, categoryRecoveryTime, onHold, 0, 0);
  }

  return 1;
}

static BOOL OnActionButtons(LPVOID, NETMESSAGE, DWORD, CDataStore *msg) {
  for (UINT index = 0; index < 120; ++index) {
    msg->Get(s_initialButtons[index]);
  }

  return 1;
}

static BOOL OnPetSpells(LPVOID, NETMESSAGE, DWORD, CDataStore *msg) {
  DWORDLONG petGUID;
  DWORD     timelimit;

  CGPetInfo::ClearActions();
  timelimit = 0;
  msg->Get(petGUID);

  if (petGUID) {
    int  categoryDuration;
    int  spellDuration;
    UINT petMode;
    WORD category;
    BYTE count;
    WORD spellID;
    bool onHold;
    UINT index;

    msg->Get(timelimit);
    msg->Get(petMode);
    CGPetInfo::SetPetModeAndOrders(petMode);

    for (index = 0; index < 10; ++index) {
      PetAction action(0);
      msg->Get(action);
      CGPetInfo::SetAction(index, action, 0);
    }

    msg->Get(count);
    CGSpellBook::ClearPetSpells();
    for (index = 0; index < count; ++index) {
      WORD spellID;
      msg->Get(spellID);
      CGSpellBook::AddPetSpell(spellID);
    }

    msg->Get(count);
    for (index = 0; index < count; ++index) {
      msg->Get(spellID);
      msg->Get(category);
      msg->Get(spellDuration);
      msg->Get(categoryDuration);
      onHold = (categoryDuration & 0x80000000) != 0;
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

static BOOL OnPlayEmote(LPVOID, NETMESSAGE, DWORD, CDataStore *msg) {
  DWORDLONG guid;
  int       emoteID;

  msg->Get(emoteID);
  msg->Get(guid);

  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (unit && unit->GetStandState() != 3) {
    unit->PlayEmoteAnimation(emoteID, 0);
  }

  return 1;
}

static BOOL OnGroupInvite(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  char name[48];
  msg->GetString(name, sizeof(name));
  CGGameUI::OpenPartyInvite(name);
  CGGameUI::DisplayError(GERR_INVITED_TO_GROUP_S, name);
  return 1;
}

static BOOL OnGroupCancel(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  char string[256];
  char name[48];
  msg->GetString(name, sizeof(name));
  SStrPrintf(string, sizeof(string), "%s cancels the group invitation.", name);
  ConsoleWrite(string, DEFAULT_COLOR);
  CGGameUI::CancelPartyInvite();
  return 1;
}

static BOOL OnGroupDecline(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  char name[48];
  msg->GetString(name, sizeof(name));
  CGGameUI::DisplayError(GERR_DECLINE_GROUP_S, name);
  return 1;
}

static BOOL OnGroupNewLeader(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  char name[48];
  msg->GetString(name, sizeof(name));

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player && SStrCmpI(player->GetUnitName(), name, sizeof(name))) {
    CGGameUI::DisplayError(GERR_NEW_LEADER_S, name);
  } else {
    CGGameUI::DisplayError(GERR_NEW_LEADER_YOU);
  }

  return 1;
}

static BOOL OnGroupUninvite(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  CGGameUI::RemoveAllPartyMembers();
  CGGameUI::SetPartyLeader(0);
  CGGameUI::DisplayError(GERR_UNINVITE_YOU);
  return 1;
}

static BOOL OnGroupDestroy(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  CGGameUI::RemoveAllPartyMembers();
  CGGameUI::SetPartyLeader(0);
  CGGameUI::DisplayError(GERR_GROUP_DISBANDED);
  return 1;
}

static BOOL OnGroupCommandResult(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  char name[48];
  int  command;
  int  result;

  msg->Get(command);
  msg->GetString(name, sizeof(name));
  msg->Get(result);

  if (!result) {
    switch (command) {
      case 0:
        CGGameUI::DisplayError(GERR_INVITE_PLAYER_S, name);
        break;
      case 2:
        CGGameUI::RemoveAllPartyMembers();
        CGGameUI::SetPartyLeader(0);
        CGGameUI::DisplayError(GERR_LEFT_GROUP_YOU);
        break;
    }
  } else {
    switch (result) {
      case 1:
        CGGameUI::DisplayError(GERR_BAD_PLAYER_NAME_S, name);
        break;
      case 2:
        CGGameUI::DisplayError(GERR_TARGET_NOT_IN_GROUP_S, name);
        break;
      case 3:
        CGGameUI::DisplayError(GERR_GROUP_FULL);
        break;
      case 4:
        CGGameUI::DisplayError(GERR_ALREADY_IN_GROUP_S, name);
        break;
      case 5:
        CGGameUI::DisplayError(GERR_NOT_IN_GROUP);
        break;
      case 6:
        CGGameUI::DisplayError(GERR_NOT_LEADER);
        break;
      case 7:
        CGGameUI::DisplayError(GERR_PLAYER_WRONG_FACTION);
        break;
      case 8:
        CGGameUI::DisplayError(GERR_IGNORING_YOU_S, name);
        break;
    }
  }

  return 1;
}

static BOOL OnGroupList(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  DWORDLONG oldMembers[5];
  UINT      i;
  UINT      count;
  BOOL      isLeader;
  DWORDLONG newMembers[5];
  BOOL      wasInGroup;

  msg->Get(count);
  wasInGroup = 0;
  isLeader = 0;
  memset(oldMembers, 0, sizeof(oldMembers));
  memset(newMembers, 0, sizeof(newMembers));

  for (i = 0; i < 5; ++i) {
    oldMembers[i] = CGGameUI::GetPartyMember(i);
    if (oldMembers[i]) {
      wasInGroup = 1;
    }
  }

  CGGameUI::RemoveAllPartyMembers();

  for (i = 0; i < count; ++i) {
    DWORDLONG guid;
    BYTE      connected;
    char      string[32];
    msg->GetString(string, sizeof(string));
    ConsoleWrite(string, DEFAULT_COLOR);
    msg->Get(guid);
    msg->Get(connected);

    if (guid == ClntObjMgrGetActivePlayer()) {
      if (!i) {
        CGGameUI::SetPartyLeader(guid);
        isLeader = 1;
      }
      continue;
    }

    newMembers[i] = guid;
    if (isLeader && !wasInGroup) {
      CGGameUI::DisplayError(GERR_JOINED_GROUP_S, string);
    } else if (wasInGroup) {
      UINT oldIndex;
      for (oldIndex = 0; oldIndex < 5; ++oldIndex) {
        if (oldMembers[oldIndex] == guid) {
          break;
        }
      }
      if (oldIndex == 5) {
        CGGameUI::DisplayError(GERR_JOINED_GROUP_S, string);
      }
    }

    CGGameUI::AddPartyMember(guid, connected != 0);
    if (!i) {
      CGGameUI::SetPartyLeader(guid);
    }
  }

  for (i = 0; i < 5; ++i) {
    if (!oldMembers[i] || oldMembers[i] == ClntObjMgrGetActivePlayer()) {
      continue;
    }

    UINT newIndex;
    for (newIndex = 0; newIndex < count; ++newIndex) {
      if (newMembers[newIndex] == oldMembers[i]) {
        break;
      }
    }

    if (newIndex >= count) {
      const NameCache *name = g_nameDBCache.GetRecord(oldMembers[i], 0, 0, 0);
      if (name) {
        CGGameUI::DisplayError(GERR_LEFT_GROUP_S, name->m_name);
      }
    }
  }

  if (count > 0) {
    BYTE      lootMethod;
    DWORDLONG lootMaster;
    msg->Get(lootMethod);
    msg->Get(lootMaster);
    CGGameUI::SetLootMethod(static_cast<LOOT_METHOD>(lootMethod), lootMaster);
  }

  return 1;
}

static BOOL OnGuildInvite(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  char guildName[96];
  char name[48];
  msg->GetString(name, sizeof(name));
  msg->GetString(guildName, sizeof(guildName));
  CGGameUI::OpenGuildInvite(name, guildName);
  CGGameUI::DisplayError(GERR_INVITED_TO_GUILD_SS, name, guildName);
  return 1;
}

static BOOL OnGuildDecline(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  char name[48];
  msg->GetString(name, sizeof(name));
  CGGameUI::DisplayError(GERR_GUILD_DECLINE_S, name);
  return 1;
}

static BOOL OnGuildInfo(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  char name[96];
  char buf[128];
  char temp[64];
  UINT year;
  UINT day;
  UINT numChars;
  UINT month;
  UINT numAccounts;

  msg->GetString(name, sizeof(name));
  msg->Get(day);
  msg->Get(month);
  msg->Get(year);
  msg->Get(numChars);
  msg->Get(numAccounts);

  LPCSTR text = FrameScript_GetText("GUILD_NAME_TEMPLATE", -1, GENDER_NOT_APPLICABLE);
  SStrCopy(temp, text, sizeof(temp));
  SStrPrintf(buf, sizeof(buf), temp, name);
  CGChat::AddChatMessage(buf, SLASH_CMD_SYSTEM, 0, 0, 0, 0, 0);

  text = FrameScript_GetText("GUILD_INFO_TEMPLATE", -1, GENDER_NOT_APPLICABLE);
  SStrCopy(temp, text, sizeof(temp));
  SStrPrintf(buf, sizeof(buf), temp, month, day, year, numChars, numAccounts);
  CGChat::AddChatMessage(buf, SLASH_CMD_SYSTEM, 0, 0, 0, 0, 0);

  return 1;
}

static BOOL OnGuildRoster(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  char name[256];
  char ranks[5][32];
  char guildname[96];
  char buf[128];
  char temp[64];
  UINT guildRank;
  UINT numAccounts;
  UINT numChars;
  UINT i;

  msg->GetString(guildname, sizeof(guildname));
  msg->Get(numChars);
  msg->Get(numAccounts);

  LPCSTR text = FrameScript_GetText("GUILD_NAME_TEMPLATE", -1, GENDER_NOT_APPLICABLE);
  SStrCopy(temp, text, sizeof(temp));
  SStrPrintf(buf, sizeof(buf), temp, guildname);
  CGChat::AddChatMessage(buf, SLASH_CMD_SYSTEM, 0, 0, 0, 0, 0);

  for (i = 0; i <= 4; ++i) {
    SStrPrintf(temp, sizeof(temp), "GUILD_RANK%d_DESC", i);
    text = FrameScript_GetText(temp, -1, GENDER_NOT_APPLICABLE);
    SStrCopy(ranks[i], text, sizeof(ranks[i]));
  }

  text = FrameScript_GetText("GUILD_MEMBER_TEMPLATE", -1, GENDER_NOT_APPLICABLE);
  SStrCopy(temp, text, sizeof(temp));
  for (i = 0; i < numChars; ++i) {
    msg->GetString(name, 0x7FFFFFFF);
    msg->Get(guildRank);
    SStrPrintf(buf, sizeof(buf), temp, name, ranks[guildRank]);
    CGChat::AddChatMessage(buf, SLASH_CMD_SYSTEM, 0, 0, 0, 0, 0);
  }

  text = FrameScript_GetText("GUILD_ROSTER_TEMPLATE", -1, GENDER_NOT_APPLICABLE);
  SStrCopy(temp, text, sizeof(temp));
  SStrPrintf(buf, sizeof(buf), temp, numChars, numAccounts);
  CGChat::AddChatMessage(buf, SLASH_CMD_SYSTEM, 0, 0, 0, 0, 0);
  return 1;
}

static BOOL OnGuildEmblemActivate(LPVOID, NETMESSAGE, DWORD, CDataStore *msg) {
  DWORDLONG vendor;
  msg->Get(vendor);
  CGTabardCreationFrame::Open(vendor);
  return 1;
}

static BOOL OnGuildEmblemError(LPVOID, NETMESSAGE, DWORD, CDataStore *msg) {
  int error;
  msg->Get(error);
  if (error >= 0 && error < 7) {
    CGGameUI::DisplayError(s_tabardErrors[error]);
  }
  return 1;
}

static BOOL OnGuildEvent(LPVOID, NETMESSAGE, DWORD, CDataStore *msg) {
  char            string[2][256];
  BYTE            numStrings;
  GAME_ERROR_TYPE errorType;

  BYTE eventType = 0;
  msg->Get(eventType);
  int event = eventType;
  msg->Get(numStrings);
  for (BYTE index = 0; index < numStrings; ++index) {
    msg->GetString(string[index], sizeof(string[index]));
  }

  switch (event) {
    case 0:
      errorType = GERR_GUILD_PROMOTE_SS;
      break;
    case 1:
      errorType = GERR_GUILD_DEMOTE_SS;
      break;
    case 3:
      errorType = GERR_GUILD_JOIN_S;
      break;
    case 4:
      errorType = GERR_GUILD_LEAVE_S;
      break;
    case 6:
      errorType = GERR_GUILD_LEADER_IS_S;
      break;
    case 7:
      errorType = GERR_GUILD_LEADER_CHANGED_SS;
      break;
    case 5:
      errorType = GERR_GUILD_REMOVE_SS;
      break;
    case 8:
      errorType = GERR_GUILD_DISBANDED;
      break;
    case 2:
      if (!string[0][0]) {
        return 1;
      }
      errorType = GERR_GUILD_MOTD_S;
      break;
    default:
      errorType = GERR_GUILD_INTERNAL;
      break;
  }

  switch (numStrings) {
    default:
      CGGameUI::DisplayError(errorType);
      break;
    case 2:
      CGGameUI::DisplayError(errorType, string[0], string[1]);
      break;
    case 1:
      CGGameUI::DisplayError(errorType, string[0]);
      break;
  }
  return 1;
}

static BOOL OnGuildCommandResult(LPVOID, NETMESSAGE, DWORD, CDataStore *msg) {
  char name[96];
  int  result;
  int  command;

  msg->Get(command);
  msg->GetString(name, sizeof(name));
  msg->Get(result);

  if (!result) {
    switch (command) {
      case 0:
        CGGameUI::DisplayError(GERR_GUILD_CREATE_S, name);
        break;
      case 1:
        CGGameUI::DisplayError(GERR_GUILD_INVITE_S, name);
        break;
      case 2:
        CGGameUI::DisplayError(GERR_GUILD_QUIT_S, name);
        break;
      case 12:
        CGGameUI::DisplayError(GERR_GUILD_FOUNDER_S, name);
        break;
    }
  } else {
    switch (result) {
      case 1:
        CGGameUI::DisplayError(GERR_GUILD_INTERNAL);
        break;
      case 2:
        CGGameUI::DisplayError(GERR_ALREADY_IN_GUILD);
        break;
      case 3:
        CGGameUI::DisplayError(GERR_ALREADY_IN_GUILD_S, name);
        break;
      case 4:
        CGGameUI::DisplayError(GERR_INVITED_TO_GUILD);
        break;
      case 5:
        CGGameUI::DisplayError(GERR_ALREADY_INVITED_TO_GUILD_S, name);
        break;
      case 6:
        CGGameUI::DisplayError(GERR_GUILD_NAME_INVALID);
        break;
      case 7:
        CGGameUI::DisplayError(GERR_GUILD_NAME_EXISTS_S, name);
        break;
      case 8:
        if (command == 2) {
          CGGameUI::DisplayError(GERR_GUILD_LEADER_LEAVE);
        } else {
          CGGameUI::DisplayError(GERR_GUILD_PERMISSIONS);
        }
        break;
      case 9:
        CGGameUI::DisplayError(GERR_GUILD_PLAYER_NOT_IN_GUILD);
        break;
      case 10:
        CGGameUI::DisplayError(GERR_GUILD_PLAYER_NOT_IN_GUILD_S, name);
        break;
      case 11:
        CGGameUI::DisplayError(GERR_GUILD_PLAYER_NOT_FOUND_S, name);
        break;
      case 12:
        CGGameUI::DisplayError(GERR_GUILD_NOT_ALLIED);
        break;
    }
  }

  return 1;
}

static void Player_C_RegisterGuildUpdate(UINT guildID) {
  *s_guildIDs.New() = guildID;
}

static void GuildCallback(int guildID, const DWORDLONG &, LPVOID, bool granted) {
  if (granted) {
    Player_C_RegisterGuildUpdate(guildID);
  }
}

static LPCSTR MirrorTimerToName(int timer) {
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

static LPCSTR MirrorTimerLabel(int timer, int spellID) {
  char            label[128];
  const SpellRec *spell = g_spellDB.GetRecord(spellID);
  if (spell) {
    return spell->m_name_lang[CURRENT_LANGUAGE];
  }

  SStrPrintf(label, sizeof(label), "%s_LABEL", MirrorTimerToName(timer));
  return FrameScript_GetText(label, -1, GENDER_NOT_APPLICABLE);
}

static BOOL OnMirrorTimerEvent(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  switch (msgId) {
    case SMSG_START_MIRROR_TIMER: {
      int  value;
      int  maxValue;
      int  scale;
      int  spellID;
      int  timer;
      BYTE paused;
      msg->Get(timer);
      msg->Get(value);
      msg->Get(maxValue);
      msg->Get(scale);
      msg->Get(paused);
      msg->Get(spellID);
      FrameScript_SignalEvent(346, "%s%d%d%d%d%s", MirrorTimerToName(timer), value, maxValue, scale, paused, MirrorTimerLabel(timer, spellID));
      break;
    }

    case SMSG_PAUSE_MIRROR_TIMER: {
      int  timer;
      BYTE paused;
      msg->Get(timer);
      msg->Get(paused);
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

BOOL CGPlayer_C::OnVendorInventory(CDataStore *msg) {
  DWORDLONG vendorGuid;
  BYTE      reason = 0xFF;
  BYTE      count;

  for (UINT index = 0; index < 128; ++index) {
    s_lastVendorList[index].m_muid = 0;
  }

  msg->Get(vendorGuid);
  msg->Get(count);
  FATALASSERT(count <= 128);
  s_lastVendorListReceived = vendorGuid;

  if (count) {
    for (UINT index = 0; index < count; ++index) {
      msg->Get(s_lastVendorList[index].m_muid);
      msg->Get(s_lastVendorList[index].m_itemType);
      msg->Get(s_lastVendorList[index].m_itemDisplayID);
      msg->Get(s_lastVendorList[index].m_quantity);
      msg->Get(s_lastVendorList[index].m_price);
      msg->Get(s_lastVendorList[index].m_durability);
      msg->Get(s_lastVendorList[index].m_stackCount);
    }
  } else {
    msg->Get(reason);
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

BOOL CGPlayer_C::OnQuestGiverListQuests(CDataStore *msg) {
  char                initialText[8][64];
  char                greetText[256];
  DWORDLONG           questGiverGuid;
  QUESTGIVEREMOTENODE node;

  {
    BYTE count;
    msg->Get(questGiverGuid);
    msg->GetString(greetText, 0x7FFFFFFF);
    msg->Get(node.delay);
    msg->Get(node.emoteID);
    msg->Get(count);

    memset(s_lastQuestList, 0, sizeof(s_lastQuestList));
    memset(s_lastQuestListType, 0, sizeof(s_lastQuestListType));
    s_lastQuestGiverListRevieved = questGiverGuid;
    memset(s_lastQuestLevel, 0, sizeof(s_lastQuestLevel));

    CGQuestInfo::SetState(questGiverGuid, QUEST_GREETING, greetText, 0);
    for (UINT index = 0; index < count; ++index) {
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
  }
  return 1;
}

BOOL CGPlayer_C::OnQuestGiverInvalidQuest(CDataStore *msg) {
  int failureReason;
  msg->Get(failureReason);

  switch (failureReason) {
    case 1:
      CGGameUI::DisplayError(GERR_QUEST_FAILED_LOW_LEVEL);
      break;
    case 15:
      CGGameUI::DisplayError(GERR_QUEST_FAILED_MISSING_ITEMS);
      break;
    default:
      ConsoleWrite("Invalid quest!", DEFAULT_COLOR);
      break;
  }

  CGQuestInfo::QuestGiverFinished();
  return 1;
}

BOOL CGPlayer_C::OnQuestGiverSendQuest(CDataStore *msg) {
  char      questText[1024];
  char      logDescription[512];
  char      questTitle[64];
  int       chooseRewardQty[6];
  int       rewardItemQty[4];
  int       chooseRewardDispID[6];
  int       chooseReward[6];
  int       rewardItem[4];
  int       rewardItemDispID[4];
  int       rewardMoney;
  int       questID;
  int       autoLaunched;
  DWORDLONG questGiverGuid;
  int       rewardItemCount;
  int       chooseRewardCount;

  msg->Get(questGiverGuid);
  msg->Get(questID);
  msg->GetString(questTitle, 0x7FFFFFFF);
  msg->GetString(questText, 0x7FFFFFFF);
  msg->GetString(logDescription, 0x7FFFFFFF);
  msg->Get(autoLaunched);
  msg->Get(chooseRewardCount);

  memset(chooseReward, 0, sizeof(chooseReward));
  memset(chooseRewardDispID, 0, sizeof(chooseRewardDispID));
  int index;
  for (index = 0; index < chooseRewardCount; ++index) {
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
  {
    int numEmotes;
    msg->Get(numEmotes);
    TSStackArray<QUESTGIVEREMOTENODE> emotes(_alloca(numEmotes * sizeof(QUESTGIVEREMOTENODE)), numEmotes, numEmotes);
    for (index = 0; index < numEmotes; ++index) {
      msg->Get(emotes[index].emoteID);
      msg->Get(emotes[index].delay);
    }

    CGObject_C *object = ClntObjMgrObjectPtr(questGiverGuid, __FILE__, __LINE__);
    if (object && (object->GetType() & TYPE_UNIT)) {
      static_cast<CGUnit_C *>(object)->SetEmoteQueue(emotes);
    }

    CGQuestInfo::SetState(questGiverGuid, QUEST_OFFER, questText, questID);
    CGQuestInfo::SetLogDescription(logDescription);
    CGQuestInfo::AddReward(
        questTitle, chooseReward, chooseRewardDispID, chooseRewardQty, chooseRewardCount, rewardItem, rewardItemDispID, rewardItemQty,
        rewardItemCount, rewardMoney, autoLaunched
    );
  }
  return 1;
}

BOOL CGPlayer_C::OnQuestGiverRequestItems(CDataStore *msg) {
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
  QUESTGIVEREMOTENODE node;
  DWORDLONG           questGiverGuid;
  int                 itemCount;

  msg->Get(questGiverGuid);
  msg->Get(questID);
  msg->GetString(questTitle, 0x7FFFFFFF);
  msg->GetString(questText, 0x7FFFFFFF);
  msg->Get(node.delay);
  msg->Get(node.emoteID);
  msg->Get(autoLaunched);
  msg->Get(itemCount);

  memset(items, 0, sizeof(items));
  for (int index = 0; index < itemCount; ++index) {
    msg->Get(items[index]);
    msg->Get(itemAmounts[index]);
    msg->Get(itemDispID[index]);
  }

  msg->Get(maskmatch);
  msg->Get(hasitems);
  msg->Get(hasfaction);

  CGQuestInfo::SetState(questGiverGuid, QUEST_ACCEPTED, questText, questID);
  CGQuestInfo::AddItemRequest(questTitle, items, itemAmounts, itemDispID, itemCount, maskmatch && hasitems && hasfaction, autoLaunched);

  CGObject_C *object = ClntObjMgrObjectPtr(questGiverGuid, __FILE__, __LINE__);
  if (object && (object->GetType() & TYPE_UNIT)) {
    static_cast<CGUnit_C *>(object)->SetEmoteQueue(&node, 1);
  }
  return 1;
}

BOOL CGPlayer_C::OnQuestGiverChooseReward(CDataStore *msg) {
  char      questText[1024];
  char      questTitle[64];
  int       chooseRewardQty[6];
  int       rewardItemQty[4];
  int       chooseRewardDispID[6];
  int       chooseReward[6];
  int       rewardItem[4];
  int       rewardItemDispID[4];
  int       rewardMoney;
  int       questID;
  int       autoLaunched;
  DWORDLONG questGiverGuid;
  int       rewardItemCount;
  int       chooseRewardCount;

  msg->Get(questGiverGuid);
  msg->Get(questID);
  msg->GetString(questTitle, 0x7FFFFFFF);
  msg->GetString(questText, 0x7FFFFFFF);
  msg->Get(autoLaunched);

  int index;
  {
    int emoteCount;
    msg->Get(emoteCount);

    TSStackArray<QUESTGIVEREMOTENODE> emotes(_alloca(emoteCount * sizeof(QUESTGIVEREMOTENODE)), emoteCount, emoteCount);
    for (index = 0; index < emoteCount; ++index) {
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
        questTitle, chooseReward, chooseRewardDispID, chooseRewardQty, chooseRewardCount, rewardItem, rewardItemDispID, rewardItemQty,
        rewardItemCount, rewardMoney, autoLaunched
    );
  }
  return 1;
}

static void QuestCompleteCallback(int id, const DWORDLONG &, LPVOID, bool granted) {
  if (!granted) {
    return;
  }

  QuestCache *quest = const_cast<QuestCache *>(g_questDBCache.GetRecord(id, 0, 0, 0));
  if (!quest) {
    return;
  }

  if (!quest->m_rewardNextQuest) {
    CGGameUI::DisplayError(GERR_QUEST_COMPLETE_S, quest->m_logTitle);
  }
  if (!quest->m_questType) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      player->UpdateQuestStatusAll();
    }
  }
}

static void QuestRewardItemCallback(int id, const DWORDLONG &, LPVOID, bool granted) {
  if (!granted) {
    return;
  }

  const ItemStats_C *item = g_itemDBCache.GetRecord(id, 0, 0, 0);
  if (item) {
    CGGameUI::DisplayError(GERR_QUEST_REWARD_ITEM_S, item->m_displayName[0]);
  }
}

BOOL CGPlayer_C::OnQuestGiverQuestComplete(CDataStore *msg) {
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
  int index;
  for (index = 0; index < itemCount; ++index) {
    msg->Get(items[index]);
    msg->Get(itemsQty[index]);
  }

  quest = const_cast<QuestCache *>(g_questDBCache.GetRecord(questID, 0, reinterpret_cast<DBCACHECALLBACKPROC>(QuestCompleteCallback), 0));
  if (quest && !quest->m_rewardNextQuest) {
    CGGameUI::DisplayError(GERR_QUEST_COMPLETE_S, quest->m_logTitle);
  }

  if (xp) {
    CGGameUI::DisplayError(GERR_QUEST_REWARD_EXP_I, xp);
    AddWorldXPGainText(xp);
  }

  if (money) {
    CurrencyBreakdown(money, coins);
    for (int coin = 0; coin < 3; ++coin) {
      LPCSTR text = FrameScript_GetText(coinToken[coin], -1, GENDER_NOT_APPLICABLE);
      SStrCopy(coinName, text, sizeof(coinName));
      SStrPrintf(coinBuf[coin], sizeof(coinBuf[coin]), "%d %s", coins[coin], coinName);
    }

    SStrPrintf(
        buf, sizeof(buf), "%s%s%s%s%s", coins[2] ? coinBuf[2] : "", coins[2] && (coins[1] || coins[0]) ? ", " : "", coins[1] ? coinBuf[1] : "",
        coins[0] && (coins[1] || coins[2]) ? ", " : "", coins[0] ? coinBuf[0] : ""
    );
    CGGameUI::DisplayError(GERR_QUEST_REWARD_MONEY_S, buf);
  }

  for (index = 0; index < itemCount; ++index) {
    const ItemStats_C *item = g_itemDBCache.GetRecord(items[index], GetGUID(), reinterpret_cast<DBCACHECALLBACKPROC>(QuestRewardItemCallback), 0);
    if (item) {
      CGGameUI::DisplayError(GERR_QUEST_REWARD_ITEM_S, item->m_displayName[0]);
    }
  }

  if (CGQuestInfo::GetLastChosenItem()) {
    const ItemStats_C *item =
        g_itemDBCache.GetRecord(CGQuestInfo::GetLastChosenItem(), GetGUID(), reinterpret_cast<DBCACHECALLBACKPROC>(QuestRewardItemCallback), 0);
    if (item) {
      CGGameUI::DisplayError(GERR_QUEST_REWARD_ITEM_S, item->m_displayName[0]);
    }
    CGQuestInfo::ClearLastChosenItem();
  }

  CGQuestInfo::QuestGiverFinished();
  if (quest && !quest->m_questType) {
    UpdateQuestStatusAll();
  }
  return 1;
}

static void QuestFailedCallback(int id, const DWORDLONG &, LPVOID, bool granted) {
  if (!granted) {
    return;
  }

  const QuestCache *quest = g_questDBCache.GetRecord(id, 0, 0, 0);
  if (!quest) {
    return;
  }

  switch (s_questFailedReason) {
    case 4:
    case 48:
      CGGameUI::DisplayError(GERR_QUEST_FAILED_BAG_FULL_S, quest->m_logTitle);
      CGGameUI::DisplayError(GERR_INV_FULL);
      break;
    case 16:
      CGGameUI::DisplayError(GERR_QUEST_FAILED_MAX_COUNT_S, quest->m_logTitle);
      break;
    default:
      CGGameUI::DisplayError(GERR_QUEST_FAILED_S, quest->m_logTitle);
      break;
  }
}

BOOL CGPlayer_C::OnQuestGiverQuestFailed(CDataStore *msg) {
  int questFailedID;
  msg->Get(questFailedID);
  msg->Get(s_questFailedReason);

  QuestCache *quest = const_cast<QuestCache *>(
      g_questDBCache.GetRecord(questFailedID, CGQuestInfo::GetQuestGiver(), reinterpret_cast<DBCACHECALLBACKPROC>(QuestFailedCallback), 0)
  );
  if (quest) {
    switch (s_questFailedReason) {
      case 4:
      case 48:
        CGGameUI::DisplayError(GERR_QUEST_FAILED_BAG_FULL_S, quest->m_logTitle);
        CGGameUI::DisplayError(GERR_INV_FULL);
        break;
      case 16:
        CGGameUI::DisplayError(GERR_QUEST_FAILED_MAX_COUNT_S, quest->m_logTitle);
        break;
      default:
        CGGameUI::DisplayError(GERR_QUEST_FAILED_S, quest->m_logTitle);
        break;
    }
  }

  CGQuestInfo::QuestGiverFinished();
  return 1;
}

BOOL CGPlayer_C::OnQuestGiverStatus(CDataStore *msg) {
  DWORDLONG questGiverGuid;
  int       hasquest;
  msg->Get(questGiverGuid);
  msg->Get(hasquest);

  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(questGiverGuid, __FILE__, __LINE__));
  if (unit && (unit->m_unit->npcFlags & 2)) {
    FATALASSERT(unit->IsA(TYPE_UNIT));
    unit->UpdateInteractIcon(static_cast<QUEST_GIVER_STATUS>(hasquest));
  }
  return 1;
}

BOOL CGPlayer_C::OnTrainerList(CDataStore *msg) {
  char      greeting[512];
  int      *reqAbilities[3];
  BYTE     *pointCosts[2];
  DWORDLONG trainerGUID;
  int       trainerType;
  UINT      count;

  msg->Get(trainerGUID);
  msg->Get(trainerType);
  msg->Get(count);

  TSStackArray<BYTE> usable(_alloca(count * sizeof(BYTE)), count, count);
  TSStackArray<int>  reqAbility2(_alloca(count * sizeof(int)), count, count);
  TSStackArray<UINT> moneyCost(_alloca(count * sizeof(UINT)), count, count);
  TSStackArray<int>  reqAbility0(_alloca(count * sizeof(int)), count, count);
  TSStackArray<BYTE> pointCost0(_alloca(count * sizeof(BYTE)), count, count);
  TSStackArray<UINT> reqSkillStep(_alloca(count * sizeof(UINT)), count, count);
  TSStackArray<BYTE> pointCost1(_alloca(count * sizeof(BYTE)), count, count);
  TSStackArray<int>  reqAbility1(_alloca(count * sizeof(int)), count, count);
  TSStackArray<BYTE> reqLevel(_alloca(count * sizeof(BYTE)), count, count);
  TSStackArray<int>  spellID(_alloca(count * sizeof(int)), count, count);
  TSStackArray<UINT> reqSkillRank(_alloca(count * sizeof(UINT)), count, count);
  TSStackArray<UINT> reqSkillLine(_alloca(count * sizeof(UINT)), count, count);

  for (UINT index = 0; index < count; ++index) {
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

  if (trainerGUID == s_lastTrainerRequest) {
    CGClassTrainer::SetTrainer(trainerGUID, static_cast<TRAINER_TYPE>(trainerType));
    CGClassTrainer::AddServices(
        count, spellID.Ptr(), moneyCost.Ptr(), pointCosts, reqLevel.Ptr(), reqSkillLine.Ptr(), reqSkillRank.Ptr(), reqSkillStep.Ptr(), reqAbilities,
        usable.Ptr(), greeting
    );
  }
  return 1;
}

BOOL CGPlayer_C::OnBuyFailed(CDataStore *msg) {
  char      buf[256];
  DWORDLONG vendorGUID;
  UINT      muid;
  BYTE      quantity;
  BYTE      reason;

  msg->Get(vendorGUID);
  msg->Get(muid);
  msg->Get(quantity);
  msg->Get(reason);

  if (reason == 1 && vendorGUID == s_lastVendorListReceived) {
    for (UINT index = 0; index < 128; ++index) {
      if (s_lastVendorList[index].m_muid == muid) {
        s_lastVendorList[index].m_quantity = 0;
        CGMerchantInfo::UpdateItemQuantity(vendorGUID, muid, 0);
      }
    }
  }

  LPCSTR error;
  switch (reason) {
    case 1:
    case 7:
      CGGameUI::DisplayError(GERR_VENDOR_SOLD_OUT);
      error = "Sold out.";
      break;
    case 2:
      CGGameUI::DisplayError(GERR_NOT_ENOUGH_MONEY);
      error = "Not enough money";
      break;
    case 3:
      CGGameUI::DisplayError(GERR_ITEM_NOT_FOUND);
      error = "Item creation failed.";
      break;
    case 4:
      CGGameUI::DisplayError(GERR_VENDOR_HATES_YOU);
      error = "Merchant doesn't like you.";
      break;
    case 5:
      CGGameUI::DisplayError(GERR_VENDOR_TOO_FAR);
      error = "You are too far away.";
      break;
    case 6:
      error = "Your inventory is full.";
      break;
    case 8:
      CGGameUI::DisplayError(GERR_ITEM_MAX_COUNT);
      error = "You already have the maximum number allowed.";
      break;
    case 11:
      CGGameUI::DisplayError(GERR_ITEM_NOT_FOUND);
      error = "Vendor error.";
      break;
    default:
      CGGameUI::DisplayError(GERR_ITEM_NOT_FOUND);
      error = "NFI";
      break;
  }

  SStrPrintf(buf, sizeof(buf), "Buy %d of %d failed: %s", quantity, muid, error);
  ConsoleWrite(buf, DEFAULT_COLOR);
  return 1;
}

BOOL CGPlayer_C::OnBuySucceeded(CDataStore *msg) {
  DWORDLONG vendorGUID;
  int       newQuantity;
  DWORD     muid;
  msg->Get(vendorGUID);
  msg->Get(muid);
  msg->Get(newQuantity);

  if (vendorGUID == s_lastVendorListReceived) {
    for (UINT index = 0; index < 128; ++index) {
      if (s_lastVendorList[index].m_muid == muid) {
        s_lastVendorList[index].m_quantity = newQuantity;
      }
    }
    CGMerchantInfo::UpdateItemQuantity(vendorGUID, muid, newQuantity);
  }
  return 1;
}

BOOL CGPlayer_C::OnSellResponse(CDataStore *msg) {
  DWORDLONG vendorGUID;
  DWORDLONG itemGUID;
  BYTE      reason;
  msg->Get(vendorGUID);
  msg->Get(itemGUID);
  msg->Get(reason);

  if (reason) {
    GAME_ERROR_TYPE error = GERR_NONE;
    switch (reason) {
      case 1:
        error = GERR_ITEM_NOT_FOUND;
        break;
      case 2:
        error = GERR_VENDOR_NOT_INTERESTED;
        break;
      case 3:
        error = GERR_VENDOR_HATES_YOU;
        break;
      case 4:
        error = GERR_NOT_OWNER;
        break;
    }
    if (error != GERR_NONE) {
      CGGameUI::DisplayError(error);
    }
    if (itemGUID) {
      CGGameUI::UnlockItem(itemGUID);
    }
  }
  return 1;
}

static BOOL GuildIDUpdateHandler(DWORDLONG guid, UINT offset, UINT bytes, LPCVOID prevValue, LPVOID param) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (player) {
    player->OnGuildChanged();
  }
  return 1;
}

static BOOL DuelTeamUpdateHandler(DWORDLONG guid, UINT offset, UINT bytes, LPCVOID prevValue, LPVOID param) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (player) {
    player->UpdatePlayerName();
  }
  Script_SendUnitSignal(guid, 27);
  return 1;
}

void CGPlayer_C::KillExitCombatModeSheatheTimer() {
}

static BOOL OnUpdateInventoryComponent(DWORDLONG guid, UINT offset, UINT bytes, LPCVOID prevValue, LPVOID param) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (!player) {
    return 1;
  }

  UINT      slot = offset >> 3;
  DWORDLONG currGuid = player->CGPlayer_C::GetBag()->GetItem(slot);
  ConsolePrintf("Removed item, added item: %016I64X", currGuid);

  if (*static_cast<const DWORDLONG *>(prevValue) == currGuid) {
    return 1;
  }

  CGTradeSkillInfo::RefreshList(0);
  CGCraftInfo::RefreshList();
  CGQuestLog::Update(0);

  CGItem_C *oldItem = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(*static_cast<const DWORDLONG *>(prevValue), __FILE__, __LINE__));
  CGItem_C *newItem = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(currGuid, __FILE__, __LINE__));

  if (oldItem) {
    CGActionBar::UpdateItem(oldItem->GetEntryID());
  }
  if (newItem) {
    CGActionBar::UpdateItem(newItem->GetEntryID());
  }

  if (player->IsSlotComponented(slot, 0)) {
    if (player->m_unit->weaponMode == WEAPONMODE_NORMALMODE && ((1 << slot) & 0x18000)) {
      bool defer = false;

      bool startAnim = true;
      bool both = false;

      if (*static_cast<const DWORDLONG *>(prevValue) && currGuid) {
        if (slot == INVSLOT_MAINHAND) {
          if (*static_cast<const DWORDLONG *>(prevValue) == player->CGPlayer_C::GetBag()->GetItem(INVSLOT_OFFHAND)) {
            both = true;
          }
        } else if (slot == INVSLOT_OFFHAND) {
          if (currGuid == player->CGPlayer_C::GetBag()->GetItem(INVSLOT_MAINHAND)) {
            startAnim = false;
          }
        }
      }

      if (both) {
        player->m_animFlags |= 0x800;
        player->StartSheatheAnim(INVSLOT_MAINHAND, 1, 1);
        defer = true;
      } else if (startAnim) {
        int hip = 0;
        if ((oldItem && newItem && ((1 << oldItem->GetSheatheType()) & 0x88) && ((1 << newItem->GetSheatheType()) & 0x88)) ||
            (oldItem && ((1 << oldItem->GetSheatheType()) & 0x88)) || (newItem && ((1 << newItem->GetSheatheType()) & 0x88)))
        {
          hip = 1;
        }
        player->m_animFlags |= 0x800;
        player->StartSheatheAnim(static_cast<INVENTORY_SLOTS>(slot), hip, 0);
        defer = true;
      }

      player->RemoveComponent(slot, newItem == 0, defer, false);
      if (newItem) {
        player->AttachObjComponent(currGuid, slot, defer, false, SheatheTypeToSheathePoint(newItem->GetSheatheType(), slot));
        player->AddComponent(newItem->GetDisplayID(), newItem->GetInventoryType(), slot, 1);
      }
    } else {
      if (*static_cast<const DWORDLONG *>(prevValue)) {
        player->RemoveComponent(slot, newItem == 0, false, true);
      }

      if (newItem) {
        if ((1 << slot) & 0x18000) {
          HMODEL charModel = player->GetCharacterModel(0);
          FATALASSERT(charModel);

          for (UINT hand = 0; hand < NUMHANDS; ++hand) {
            if (slot == s_hands[hand]) {
              DWORDLONG itemGUID = player->CGPlayer_C::GetBag()->GetItem(s_hands[hand]);
              FATALASSERT(static_cast<CGItem_C *>(ClntObjMgrObjectPtr(itemGUID, __FILE__, __LINE__)) == newItem);
              for (UINT link = 0; link < 4; ++link) {
                if (s_clearLinkPointFlags[hand][link] != -1) {
                  ModelClearLink(charModel, s_clearLinkPointFlags[hand][link]);
                }
              }

              player->AttachObjComponent(currGuid, slot, false, true, SheatheTypeToSheathePoint(newItem->GetSheatheType(), s_hands[hand]));
              player->AddComponent(newItem->GetDisplayID(), newItem->GetInventoryType(), slot, 1);
            }
          }

          HandleClose(charModel);
        } else {
          player->AttachObjComponent(currGuid, slot, false, false, -1);
          player->AddComponent(newItem->GetDisplayID(), newItem->GetInventoryType(), slot, 1);
        }
      }

      if (player->m_geosetHandle) {
        CharCustomizationCommitGeosets(player->m_geosetHandle);
        player->Animate();
      }
    }
  }

  if (slot == INVSLOT_RANGED) {
    if (player->m_unit->weaponMode != WEAPONMODE_RANGEDMODE) {
      return 1;
    }
    if (!newItem) {
      player->SetWeaponMode(WEAPONMODE_NORMALMODE);
    }

    int spellID = Spell_C_GetTargettingSpell();
    if (player->m_castingSpell || spellID) {
      const SpellRec *spell = g_spellDB.GetRecord(spellID ? spellID : player->m_castingSpell);
      if (spell && (spell->m_attributes & 2)) {
        Spell_C_CancelSpell(true, true, SPELL_FAILED_ERROR);
      }
    }
  }

  if (slot == INVSLOT_RANGED || slot == INVSLOT_MAINHAND || slot == INVSLOT_OFFHAND) {
    UINT oldReadySequence = player->GetReadySequence();
    player->DetermineReadySequence(false);
    if (oldReadySequence != player->GetReadySequence()) {
      player->UpdateBaseAnimation(0);
    }
  }

  if (slot == INVSLOT_MAINHAND) {
    player->CheckWeaponDefenseRankChange(COMBAT_MAINHAND);
    CGSpellBook::UpdateSpells();
  } else if (slot == INVSLOT_OFFHAND) {
    player->CheckWeaponDefenseRankChange(COMBAT_OFFHAND);
  }

  return 1;
}

static BOOL OnUpdateMoney(DWORDLONG guid, UINT offset, UINT bytes, LPCVOID prevValue, LPVOID param) {
  SndInterfacePlayInterfaceSound("LOOTWINDOWCOINSOUND");
  CGClassTrainer::RefreshList();
  UINT tradeGold = Trade_C_GetPlayerTradeGold();
  if (tradeGold) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player && tradeGold > player->GetMoney()) {
      Trade_C_RemoveMoney(tradeGold - player->GetMoney());
    }
  }
  return 1;
}

static void QuestAcceptedCallback(int id, const DWORDLONG &, LPVOID, bool granted) {
  if (granted) {
    const QuestCache *quest = g_questDBCache.GetRecord(id, 0, 0, 0);
    if (quest) {
      CGGameUI::DisplayError(GERR_QUEST_ACCEPTED_S, quest->m_logTitle);
    }
  }
}

static BOOL OnUpdateQuest(DWORDLONG guid, UINT offset, UINT bytes, LPCVOID prevValue, LPVOID param) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (player) {
    if (player->GetGUID() == ClntObjMgrGetActivePlayer()) {
      CGPlayer_C::UpdateQuestStatusAll();
    }

    int                  index = (offset - 1372) / 24;
    const CQuestLogData *quest = player->GetQuestLogData(index);
    const CQuestLogData *prev = static_cast<const CQuestLogData *>(prevValue);
    if (quest && quest->m_questID && (!prev || !prev->m_questID)) {
      const QuestCache *questInfo = g_questDBCache.GetRecord(quest->m_questID, 0, QuestAcceptedCallback, 0);
      if (questInfo) {
        CGGameUI::DisplayError(GERR_QUEST_ACCEPTED_S, questInfo->m_logTitle);
      }
    }
    if (prev && prev->m_questID && (!quest || quest->m_questID != prev->m_questID)) {
      CGQuestLog::ClearQuest(prev->m_questID);
    }
  }
  return 1;
}

static BOOL OnUpdateShapeshiftForm(DWORDLONG guid, UINT offset, UINT bytes, LPCVOID prevValue, LPVOID param) {
  CGSpellBook::UpdateSelection();
  CGActionBar::UpdateSelection();
  CGActionBar::UpdateBonusBar();
  return 1;
}

static BOOL OnUpdatePlayerFlags(DWORDLONG guid, UINT offset, UINT bytes, LPCVOID prevValue, LPVOID param) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (player) {
    player->PlayerFlagsChanged(*static_cast<const BYTE *>(prevValue));
  }
  return 1;
}

static void GuildTimestampChanged(int id, const DWORDLONG &guid, LPVOID arg, bool granted) {
  if (granted) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
    if (player) {
      player->OnGuildChanged();
    }
  }
}

static BOOL OnUpdateGuild(DWORDLONG guid, UINT offset, UINT bytes, LPCVOID prevValue, LPVOID param) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (player) {
    g_guildInfoCache.Invalidate(player->GetGuildID());
    g_guildInfoCache.GetRecord(player->GetGuildID(), guid, GuildTimestampChanged, 0);
  }
  return 1;
}

static BOOL CharmChangeHandler(DWORDLONG unit, UINT, UINT, LPCVOID, LPVOID) {
  CGPlayer_C *playerPtr = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  CGGameUI::OnClientControlChanged(playerPtr->IsClientControlled());
  CGInputControl::GetActive()->UpdatePlayer(OsGetAsyncTimeMs());
  return 1;
}

static BOOL PetChangeHandler(DWORDLONG unit, UINT, UINT, LPCVOID oldValue, LPVOID) {
  CGPlayer_C *playerPtr = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  for (int i = 0; i < 2; ++i) {
    DWORDLONG oldGUID = static_cast<const DWORDLONG *>(oldValue)[i];
    DWORDLONG newGUID = i == 0 ? playerPtr->GetCharm() : playerPtr->GetSummon();
    if (oldGUID != newGUID) {
      CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(oldGUID, __FILE__, __LINE__));
      if (unitPtr) {
        unitPtr->UnregisterScript();
      }
      unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(newGUID, __FILE__, __LINE__));
      if (unitPtr) {
        unitPtr->RegisterScript();
      }
    }
  }
  FrameScript_SignalEvent(192);
  return 1;
}

static BOOL FarsightChangeHandler(DWORDLONG unit, UINT, UINT, LPCVOID oldValue, LPVOID) {
  FATALASSERT(unit == CGPlayer_C::GetActive());
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  if (player) {
    CGObject_C *focus = ClntObjMgrObjectPtr(player->GetFarsightFocus(), __FILE__, __LINE__);
    if (focus) {
      player->SetFarSightFocus(focus);
    } else {
      player->ClearFarSight();
    }
  }
  return 1;
}

static BOOL SkillRankChangeHandler(DWORDLONG player, UINT offset, UINT, LPCVOID oldValue, LPVOID) {
  WORD        oldRank = *static_cast<const WORD *>(oldValue);
  CGPlayer_C *playerPtr = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(player, __FILE__, __LINE__));
  offset = (offset - 602) / 12;
  FATALASSERT(offset < 64);
  if (playerPtr && playerPtr->GetMirrorSkillRank(offset) != oldRank) {
    playerPtr->CheckWeaponDefenseRankChange();
    ConsolePrintf("Skill %d increased from %d to %d", playerPtr->GetMirrorSkillID(offset), oldRank, playerPtr->GetMirrorSkillRank(offset));
    CGCharacterInfo::UpdateAllSkillLines();
    const SkillLineRec *skillLine = g_skillLineDB.GetRecord(playerPtr->GetMirrorSkillID(offset));
    if (skillLine && skillLine->m_categoryID != 4) {
      if (!oldRank) {
        CGGameUI::DisplayError(GERR_SKILL_GAINED_S, skillLine->m_displayName_lang[CURRENT_LANGUAGE]);
      } else {
        CGGameUI::DisplayError(GERR_SKILL_UP_SI, skillLine->m_displayName_lang[CURRENT_LANGUAGE], playerPtr->GetMirrorSkillRank(offset));
      }
    }
  }
  CGClassTrainer::RefreshList();
  CGTradeSkillInfo::RefreshList(0);
  return 1;
}

static BOOL SkillMaxRankChangeHandler(DWORDLONG player, UINT offset, UINT, LPCVOID, LPVOID) {
  CGClassTrainer::RefreshList();
  CGTradeSkillInfo::RefreshList(0);
  return 1;
}

static BOOL SkillModifierChangeHandler(DWORDLONG player, UINT offset, UINT, LPCVOID, LPVOID) {
  CGPlayer_C *playerPtr = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(player, __FILE__, __LINE__));
  if (playerPtr) {
    playerPtr->CheckWeaponDefenseRankChange();
  }
  CGCharacterInfo::UpdateAllSkillLines();
  return 1;
}

static void AnimEventCallback(LPCSTR eventName, const NTempest::C3Vector &position, LPVOID param) {
  FATALASSERT(param);
  if (param) {
    static_cast<CGPlayer_C *>(param)->HandleAnimEvent(eventName, position);
  }
}

void CGPlayer_C::SetStorage(DWORD *storage) {
  CGUnit_C::SetStorage(storage);
  CGPlayer::SetStorage(storage + CGUnit::TotalFields());
}

CGPlayer_C::CGPlayer_C(DWORD *storage, DWORD eventTime, CClientObjCreate *init)
    : CGUnit_C(storage, eventTime, init),
      CGPlayer(storage + CGUnit::TotalFields()),
      m_framesSinceUpdate(-1),
      m_flags(0),
      m_lastWeaponModeSent(-1),
      m_lootingUnit(0),
      m_lootingUnitSent(0),
      m_inventory(GetGUID(), &m_plyr->numInvSlots, m_plyr->invSlots, 1),
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

static BOOL SetLocalPlayerInGame(LPCVOID eventData, LPVOID param) {
  ClntObjMgrSetCurrent(static_cast<ClntObjMgr *>(param));
  ClientServices_CharacterSetInGame(1);
  AsyncFileReadWaitAll();

  if (s_loginCinematicID) {
    CGGameUI::StartCinematic(s_loginCinematicID);
    s_loginCinematicID = 0;
  } else {
    DisableLoadingScreen();
  }

  return 1;
}

void CGPlayer_C::SetInventoryMirrorHandler(UINT slot, int (*handler)(DWORDLONG, UINT, UINT, LPCVOID, LPVOID)) {
  ClntObjMgrSetObjMirrorHandler(GetGUID(), CGPlayer_C::OffsetOf(ID_PLAYER) + offsetof(CGPlayerData, invSlots) + slot * sizeof(DWORDLONG), sizeof(DWORDLONG), handler, 0, HANDLER_PRIORITY_HIGH);
}

void CGPlayer_C::UnsetInventoryMirrorHandler(UINT slot, int (*handler)(DWORDLONG, UINT, UINT, LPCVOID, LPVOID)) {
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), CGPlayer_C::OffsetOf(ID_PLAYER) + offsetof(CGPlayerData, invSlots) + slot * sizeof(DWORDLONG), handler, 0);
}

void CGPlayer_C::SetPlayerMirrorHandlers() {
  for (UINT slot = 0; slot < NUM_INVENTORY_SLOTS; ++slot) {
    if ((1 << slot) & 0x783FD) {
      SetInventoryMirrorHandler(slot, OnUpdateInventoryComponent);
    }
  }

  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_PLAYER) + offsetof(CGPlayerData, guildID), sizeof(((CGPlayerData *)0)->guildID), GuildIDUpdateHandler, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_PLAYER) + offsetof(CGPlayerData, duelTeam), sizeof(((CGPlayerData *)0)->duelTeam), DuelTeamUpdateHandler, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_PLAYER) + offsetof(CGPlayerData, playerFlags), sizeof(((CGPlayerData *)0)->playerFlags), OnUpdatePlayerFlags, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_PLAYER) + offsetof(CGPlayerData, guildTimeStamp), sizeof(((CGPlayerData *)0)->guildTimeStamp), OnUpdateGuild, 0, HANDLER_PRIORITY_NORMAL);
}

void CGPlayer_C::UnsetPlayerMirrorHandlers() {
  for (UINT slot = 0; slot < NUM_INVENTORY_SLOTS; ++slot) {
    if ((1 << slot) & 0x783FD) {
      UnsetInventoryMirrorHandler(slot, OnUpdateInventoryComponent);
    }
  }

  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_PLAYER) + offsetof(CGPlayerData, guildID), GuildIDUpdateHandler, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_PLAYER) + offsetof(CGPlayerData, duelTeam), DuelTeamUpdateHandler, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_PLAYER) + offsetof(CGPlayerData, playerFlags), OnUpdatePlayerFlags, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_PLAYER) + offsetof(CGPlayerData, guildTimeStamp), OnUpdateGuild, 0);
}

BOOL CGPlayer_C::ShouldRender(DWORD worldStatus) {
  if (s_renderPlayer || GetGUID() != ClntObjMgrGetActivePlayer()) {
    return CGUnit_C::ShouldRender(worldStatus);
  }

  ModelProcessEvents(GetObjectModel(), GetPosition(), GetFacing(), NTempest::C3Vector(0.0f, 0.0f, 1.0f), 1.0f);
  UpdatePlayerNameWorldText();
  ObjectSetNotRendering();
  return 0;
}

BOOL CGPlayer_C::ShouldRenderUnitName(UINT mode) const {
  if ((IsStealthed() || IsInvisible()) && CGGameUI::GetLockedTarget() != GetGUID()) {
    return 0;
  }
  if (GetGUID() == ClntObjMgrGetActivePlayer() && !s_namePlateRenderOwn->GetInt()) {
    return 0;
  }
  switch (mode) {
    case 0:
      return 0;
    case 1:
      return CGGameUI::GetLockedTarget() == GetGUID();
    case 2:
      return IsA(ID_PLAYER) || CGGameUI::GetLockedTarget() == GetGUID();
    case 3:
      return 1;
  }
  return 0;
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
  UINT component;

  for (component = BANKGENERIC_FIRST * sizeof(DWORDLONG); component <= BANKGENERIC_LAST * sizeof(DWORDLONG); component += sizeof(DWORDLONG)) {
    ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_PLAYER) + offsetof(CGPlayerData, invSlots) + component, sizeof(DWORDLONG), BankInvHandler, 0, HANDLER_PRIORITY_NORMAL);
  }
  for (component = BANKBAG_FIRST * sizeof(DWORDLONG); component <= BANKBAG_LAST * sizeof(DWORDLONG); component += sizeof(DWORDLONG)) {
    ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_PLAYER) + offsetof(CGPlayerData, invSlots) + component, sizeof(DWORDLONG), BankInvHandler, 0, HANDLER_PRIORITY_NORMAL);
  }
  for (component = 0; component < sizeof(((CGPlayerData *)0)->questLog); component += sizeof(CQuestLogData)) {
    ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_PLAYER) + offsetof(CGPlayerData, questLog) + component, sizeof(CQuestLogData), OnUpdateQuest, 0, HANDLER_PRIORITY_NORMAL);
  }

  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + offsetof(CGUnitData, coinage), 1, OnUpdateMoney, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + offsetof(CGUnitData, charmedBy), sizeof(((CGUnitData *)0)->charmedBy), CharmChangeHandler, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + offsetof(CGUnitData, charm), sizeof(((CGUnitData *)0)->charm) + sizeof(((CGUnitData *)0)->summon), PetChangeHandler, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + offsetof(CGUnitData, shapeshiftForm), sizeof(((CGUnitData *)0)->shapeshiftForm), OnUpdateShapeshiftForm, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_PLAYER) + offsetof(CGPlayerData, farsightObject), sizeof(((CGPlayerData *)0)->farsightObject), FarsightChangeHandler, 0, HANDLER_PRIORITY_NORMAL);

  for (component = 0; component < sizeof(((CGPlayerData *)0)->skillInfo); component += sizeof(MirrorSkillInfo)) {
    ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_PLAYER) + offsetof(CGPlayerData, skillInfo) + offsetof(MirrorSkillInfo, m_skillRank) + component, sizeof(((MirrorSkillInfo *)0)->m_skillRank), SkillRankChangeHandler, 0, HANDLER_PRIORITY_NORMAL);
    ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_PLAYER) + offsetof(CGPlayerData, skillInfo) + offsetof(MirrorSkillInfo, m_skillMaxRank) + component, sizeof(((MirrorSkillInfo *)0)->m_skillMaxRank), SkillMaxRankChangeHandler, 0, HANDLER_PRIORITY_NORMAL);
    ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_PLAYER) + offsetof(CGPlayerData, skillInfo) + offsetof(MirrorSkillInfo, m_skillModifier) + component, sizeof(((MirrorSkillInfo *)0)->m_skillModifier), SkillModifierChangeHandler, 0, HANDLER_PRIORITY_NORMAL);
  }
}

void CGPlayer_C::UnsetActiveMirrorHandlers() {
  UINT component;

  for (component = BANKGENERIC_FIRST * sizeof(DWORDLONG); component <= BANKGENERIC_LAST * sizeof(DWORDLONG); component += sizeof(DWORDLONG)) {
    ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_PLAYER) + offsetof(CGPlayerData, invSlots) + component, BankInvHandler, 0);
  }
  for (component = BANKBAG_FIRST * sizeof(DWORDLONG); component <= BANKBAG_LAST * sizeof(DWORDLONG); component += sizeof(DWORDLONG)) {
    ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_PLAYER) + offsetof(CGPlayerData, invSlots) + component, BankInvHandler, 0);
  }
  for (component = 0; component < sizeof(((CGPlayerData *)0)->questLog); component += sizeof(CQuestLogData)) {
    ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_PLAYER) + offsetof(CGPlayerData, questLog) + component, OnUpdateQuest, 0);
  }

  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + offsetof(CGUnitData, coinage), OnUpdateMoney, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + offsetof(CGUnitData, charmedBy), CharmChangeHandler, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + offsetof(CGUnitData, charm), PetChangeHandler, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_PLAYER) + offsetof(CGPlayerData, farsightObject), FarsightChangeHandler, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_UNIT) + offsetof(CGUnitData, shapeshiftForm), OnUpdateShapeshiftForm, 0);

  for (component = 0; component < sizeof(((CGPlayerData *)0)->skillInfo); component += sizeof(MirrorSkillInfo)) {
    ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_PLAYER) + offsetof(CGPlayerData, skillInfo) + offsetof(MirrorSkillInfo, m_skillRank) + component, SkillRankChangeHandler, 0);
    ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_PLAYER) + offsetof(CGPlayerData, skillInfo) + offsetof(MirrorSkillInfo, m_skillMaxRank) + component, SkillMaxRankChangeHandler, 0);
    ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_PLAYER) + offsetof(CGPlayerData, skillInfo) + offsetof(MirrorSkillInfo, m_skillModifier) + component, SkillModifierChangeHandler, 0);
  }
}

void CGPlayer_C::PostInit(const CClientObjCreate &init) {
  DWORD     time1;
  CGItem_C *itemptr;
  DWORDLONG item;
  bool      sheathe;
  int       linkPoint;

  CGUnit_C::PostInit(init);
  m_fadingMountScale = GetMountScale();
  GetUnitName();
  SetPlayerMirrorHandlers();

  time1 = OsGetAsyncTimeMsPrecise();
  for (int slot = 0; slot < NUM_INVENTORY_SLOTS; ++slot) {
    if (slot == INVSLOT_RANGED || !((1 << slot) & 0x783FD) || !IsSlotComponented(slot, 1)) {
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
      sheathe = m_unit->weaponMode == WEAPONMODE_SHEATHEDMODE;
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
  time1 = OsGetAsyncTimeMsPrecise() - time1;
  if (time1 > 15) {
    OsOutputDebugString("CGPlayer_C::PostInit Setup Components(): %dms\n", time1);
  }

  item = m_inventory.GetItem(INVSLOT_RANGED);
  if (item) {
    AttachObjComponent(item, INVSLOT_RANGED, 0, 0, -1);
    SetSheatheReason(SHEATHE_RANGED, m_unit->weaponMode == WEAPONMODE_RANGEDMODE, 1);
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
    g_itemDBCache.VerifyCache(this, 0, 0);
    CGSpellBook::ClearSpells();

    for (UINT index = 0; index < s_initialSpells.Count(); ++index) {
      AddKnownSpell(s_initialSpells[index].spellID, s_initialSpells[index].slot, 0, 1);
    }

    if (ClntObjMgrGetPlayerType() == PLAYER_NORMAL) {
      for (UINT index = 0; index < 120; ++index) {
        if (s_initialButtons[index] < 0 || IsSpellKnown(s_initialButtons[index])) {
          CGActionBar::SetAction(index, s_initialButtons[index]);
        }
      }
    }
  }

  if (GetGUID() == ClntObjMgrGetActivePlayer() && ClntObjMgrGetPlayerType() == PLAYER_NORMAL) {
    CGGameUI::EnterWorld();
    CGGameUI::UpdateActivePlayer();
  }
  CGGameUI::EnablePartyMember(GetGUID(), 1);
}
void CGPlayer_C::PostReenable() {
  CGUnit_C::PostReenable();
  CGGameUI::EnablePartyMember(GetGUID(), 1);
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

void CGPlayer_C::InspectPlayer(const DWORDLONG &guid) {
  if (!guid) {
    return;
  }
  CDataStore msg;
  msg.Put(CMSG_INSPECT);
  msg.Put(guid);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::ReceiveResurrectRequest(LPCSTR name) {
  if (name && *name && m_unit->health <= 0) {
    CGGameUI::OpenResurrectRequest(name);
  }
}

void CGPlayer_C::AcceptResurrectRequest(int accept) {
  if (!s_resurrectOffer) {
    return;
  }
  CDataStore msg;
  msg.Put(CMSG_RESURRECT_RESPONSE);
  msg.Put(s_resurrectOffer);
  msg.Put(static_cast<BYTE>(accept));
  msg.Finalize();
  ClientServices_Send(&msg);
  s_resurrectOffer = 0;
}

LPCSTR CGPlayer_C::GetModelFileName() const {
  const CreatureDisplayInfoRec *displayInfo = g_creatureDisplayInfoDB.GetRecord(m_unit->displayID);
  if (!displayInfo) {
    SysMsgPrintf(SYSMSG_WARNING, 16, "INVALIDPLAYERDISPLAYID|%d|%d|%d", m_unit->displayID, m_unit->race, m_unit->sex);
    return NONAME;
  }

  const CreatureModelDataRec *modelData = g_creatureModelDataDB.GetRecord(displayInfo->m_modelID);
  if (!modelData) {
    SysMsgPrintf(SYSMSG_WARNING, 16, "INVALIDPLAYERMODELRECORD|%d|%d|%d", displayInfo->m_modelID, m_unit->race, m_unit->sex);
    return NONAME;
  }

  return modelData->m_ModelName;
}

void CGPlayer_C::InitPreferredGeosets() {
  memset(m_preferredGeosets, 0, sizeof(m_preferredGeosets));
  m_preferredGeosets[CHARGEOSET_HAIR] = CharCustomizationGetHairGeoset(m_unit->race, m_unit->sex, GetHairStyle());

  BEARDSTYLEDATA beardStyleData;
  m_preferredGeosets[CHARGEOSET_EAR] = 2;
  if (CharCustomizationGetBeardStyle(m_unit->race, m_unit->sex, GetFacialHair(), &beardStyleData)) {
    m_preferredGeosets[CHARGEOSET_BEARD] = beardStyleData.beardGeoset;
    m_preferredGeosets[CHARGEOSET_SIDEBURN] = beardStyleData.sideBurnGeoset;
    m_preferredGeosets[CHARGEOSET_MOUSTACHE] = beardStyleData.moustacheGeoset;
  }
}

void CGPlayer_C::InitComponents() {
  FATALASSERT(m_modelData);
  if (!(m_modelData->m_flags & 4)) {
    return;
  }

  DWORD  time1 = OsGetAsyncTimeMs();
  HMODEL charModel = GetCharacterModel(0);
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

  BEARDSTYLEDATA facialData;
  BOOL           hasFacialData = CharCustomizationGetBeardStyle(m_unit->race, m_unit->sex, GetFacialHair(), &facialData);

  m_geosetHandle = CharCustomizationCreateGeosetHandle(charModel);
  FATALASSERT(m_geosetHandle);
  InitPreferredGeosets();
  CharCustomizationInitBaseCharacter(
      m_geosetHandle, hasFacialData ? facialData.beardGeoset : 1, hasFacialData ? facialData.sideBurnGeoset : 1,
      hasFacialData ? facialData.moustacheGeoset : 1, 2
  );
  CharCustomizationResetHairGeoset(m_geosetHandle, m_unit->race, m_unit->sex, GetHairStyle());
  HandleClose(charModel);

  DWORD elapsed = OsGetAsyncTimeMs() - time1;
  if (elapsed > 15) {
    ConsolePrintf("CGPlayer_C::InitComponents(): %dms\n", elapsed);
  }
}

void CGPlayer_C::RemoveComponent(int slot, bool commitItemGeosets, bool defer, bool removeRecord) {
  if (m_geosetHandle && !slot) {
    HeadGeosetUnhideCharGeosets(m_geosetHandle, m_preferredGeosets, 15);
    CharCustomizationResetHairGeoset(m_geosetHandle, m_unit->race, m_unit->sex, GetHairStyle());
  }

  if (m_texComponent) {
    if ((1 << slot) & 0x403F8) {
      TexComponentRemove(
          m_texComponent, g_itemDisplayInfoDB.GetRecord(m_texComponentInfo[slot].m_displayID), m_texComponentInfo[slot].m_inventoryType
      );
    }

    if (g_itemDisplayInfoDB.GetRecord(m_texComponentInfo[slot].m_displayID) && m_texComponentInfo[slot].m_displayID &&
        m_texComponentInfo[slot].m_inventoryType)
    {
      CharCustomizationRemoveItemGeosets(
          m_geosetHandle, g_itemDisplayInfoDB.GetRecord(m_texComponentInfo[slot].m_displayID), m_texComponentInfo[slot].m_inventoryType,
          m_texComponent
      );
    }
  }

  if (commitItemGeosets) {
    CharCustomizationCommitItemGeosets(m_geosetHandle, 0, m_paperDollModel);
    Animate();
  }

  m_texComponentInfo[slot].m_inventoryType = 0;
  m_texComponentInfo[slot].m_displayID = 0;
  RemoveObjectComponentByInvSlot(slot, defer, removeRecord);
  CGUnit_C::m_flags &= ~0x100u;
}

void CGPlayer_C::AddComponent(int displayID, UINT inventoryType, int slot, int commit) {
  FATALASSERT(inventoryType < 27);

  if ((1 << slot) & 0x403F8) {
    m_texComponentInfo[slot].m_displayID = displayID;
    m_texComponentInfo[slot].m_inventoryType = inventoryType;
  }

  const ItemDisplayInfoRec *displayInfo = g_itemDisplayInfoDB.GetRecord(displayID);
  if (m_texComponent) {
    if ((1 << slot) & 0x403F8) {
      CStatus status;
      TexComponentAdd(&status, m_unit->sex, m_texComponent, displayInfo, inventoryType, 1);
      if (!status.IsEmpty()) {
        char buffer[512];
        status.GetErrorStr(buffer, sizeof(buffer), STATUS_INFO);
        NTempest::C3Vector pos;
        GetPosition(pos);
        FATALERROR(("player 0x%I64X(%s)(%g,%g,%g): %s", GetGUID(), GetUnitName(), pos.x, pos.y, pos.z, buffer));
      }
    }

    if (displayInfo) {
      if (slot == INVSLOT_TABARD && inventoryType == INDEX_TABARD_TYPE) {
        OnGuildChanged();
      }
      CharCustomizationAddItemGeosets(m_geosetHandle, displayInfo, inventoryType, m_texComponent, m_unit->race, commit == 0);
    }
  }

  if (commit) {
    CharCustomizationCommitItemGeosets(m_geosetHandle, 0, m_paperDollModel);
    Animate();
  }

  if (!slot) {
    HeadGeosetHideCharGeosets(m_geosetHandle, displayInfo, m_unit->race, m_preferredGeosets, 15);
  }
  CGUnit_C::m_flags &= ~0x100u;
}

void CGPlayer_C::TalkToTrainer(const DWORDLONG &trainerUnit) {
  s_lastTrainerRequest = trainerUnit;
  CGClassTrainer::SetTrainer(0, TRAINER_TYPE_GENERAL);

  CDataStore hello;
  hello.Put(CMSG_TRAINER_LIST);
  hello.Put(trainerUnit);
  hello.Finalize();
  ClientServices_Send(&hello);
}

int CGPlayer_C::LootUnit(CGUnit_C *unit) {
  if (CanLoot(unit) && unit->CanBeLooted(OsGetAsyncTimeMs()) && !(m_move.GetMoveFlags() & 0x40FF)) {
    CGGameUI::CloseLoot(1, 0);

    CDataStore lootMsg;
    lootMsg.Put(CMSG_LOOT);
    lootMsg.Put(unit->GetGUID());
    lootMsg.Finalize();
    ClientServices_Send(&lootMsg);

    m_lootingUnitSent = unit->GetGUID();
    UpdateBaseAnimation(ANIM_STATE_LOOTBEGIN, 0);
  }
  return 1;
}

void CGPlayer_C::ShopFromMerchant(const DWORDLONG &merchant) {
  DWORDLONG cursorItem = CGGameUI::GetCursorItem();
  if (cursorItem) {
    SellItem(merchant, cursorItem, 0);
    CGGameUI::SetCursorItem(0, 0, 0, 0, 0);
    return;
  }

  CDataStore invMsg;
  invMsg.Put(CMSG_LIST_INVENTORY);
  invMsg.Put(merchant);
  invMsg.Finalize();
  ClientServices_Send(&invMsg);
}

BOOL CGPlayer_C::IsQuestUnit(CGUnit_C *unit) {
  if (unit->m_unit->npcFlags & 2) {
    return 1;
  }

  for (UINT index = 0; index < 16; ++index) {
    if (GetQuestLogData(index)->m_questRewarderID == unit->GetEntryID()) {
      return 1;
    }
  }
  return 0;
}

void CGPlayer_C::TalkToQuestUnit(const DWORDLONG &unit) {
  CDataStore hello;
  hello.Put(CMSG_QUESTGIVER_HELLO);
  hello.Put(unit);
  hello.Finalize();
  ClientServices_Send(&hello);
}

int CGPlayer_C::QueryTaxiNodes(const DWORDLONG &unit) {
  CDataStore msg;
  msg.Put(CMSG_TAXIQUERYAVAILABLENODES);
  msg.Put(unit);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

void CGPlayer_C::TalkToBinder(const DWORDLONG &binder) {
  if (s_lastBinderID && binder == s_lastBinderID) {
    CGGameUI::DisplayError(GERR_DEATHBINDALREADYBOUND);
    return;
  }

  CDataStore hello;
  hello.Put(CMSG_BINDER_ACTIVATE);
  hello.Put(binder);
  hello.Finalize();
  ClientServices_Send(&hello);
}

void CGPlayer_C::TalkToBanker(const DWORDLONG &banker) {
  CDataStore hello;
  hello.Put(CMSG_BANKER_ACTIVATE);
  hello.Put(banker);
  hello.Finalize();
  ClientServices_Send(&hello);
}

void CGPlayer_C::TalkToNpcPetition(const DWORDLONG &vendor) {
  CDataStore hello;
  hello.Put(CMSG_PETITION_SHOWLIST);
  hello.Put(vendor);
  hello.Finalize();
  ClientServices_Send(&hello);
}

void CGPlayer_C::TalkToTabardVendor(const DWORDLONG &tabardUnit) {
  if (CGTabardCreationFrame::GetVendor() == tabardUnit) {
    return;
  }
  CDataStore hello;
  hello.Put(MSG_TABARDVENDOR_ACTIVATE);
  hello.Put(tabardUnit);
  hello.Finalize();
  ClientServices_Send(&hello);
}

BOOL CGPlayer_C::OnTerrainClick(const CTerrainClickEvent &) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }

  DWORDLONG cursorItem;
  DWORDLONG cursorItemPack;
  UINT      cursorSlot;
  CGGameUI::GetCursorItem(cursorItem, cursorItemPack, cursorSlot);
  CGGameUI::ClearCursor(0);

  if (ClntObjMgrObjectPtr(cursorItem, __FILE__, __LINE__) && cursorItemPack) {
    BYTE packSlot = FindSlotIndex(cursorItemPack);
    CDataStore msg;
    NTempest::C3Vector position = GetPosition();
    msg.Put(CMSG_DROP_ITEM);
    msg.Put(packSlot);
    msg.Put(static_cast<BYTE>(cursorSlot));
    msg.Put(position.x);
    msg.Put(position.y);
    msg.Put(position.z);
    msg.Finalize();
    ClientServices_Send(&msg);
  }
  return 1;
}

void CGPlayer_C::OnObjectDestruct(DWORDLONG guid) {
  if (m_targetUnit == guid) {
    CGGameUI::ClearTarget(guid, 1);
    SetLocalTarget(0);
  }
}

void CGPlayer_C::SaveTabard(int eStyle, int eColor, int bStyle, int bColor, int bg, DWORDLONG vendor) const {
  if (eStyle >= 0 && eColor >= 0 && bStyle >= 0 && bColor >= 0 && bg >= 0 && eStyle < 42 && eColor < 4 && bStyle < 2 && bColor < 4 && bg < 19) {
    const GuildStats_C *guild = g_guildInfoCache.GetRecord(GetGuildID(), 0, 0, 0);
    if (!guild) {
      CGGameUI::DisplayError(GERR_GUILDEMBLEM_NOGUILD);
      return;
    }
    if (guild->m_emblemStyle == -1 && guild->m_emblemColor == -1 && guild->m_borderStyle == -1 && guild->m_borderColor == -1 &&
        guild->m_backgroundColor == -1)
    {
      if (GetGuildRank()) {
        CGGameUI::DisplayError(GERR_GUILDEMBLEM_NOTGUILDMASTER);
        return;
      }
      if (m_unit->coinage < GuildGetTabardCost()) {
        CGGameUI::DisplayError(GERR_NOT_ENOUGH_MONEY);
        return;
      }

      CDataStore msg;
      msg.Put(MSG_SAVE_GUILD_EMBLEM);
      msg.Put(eStyle);
      msg.Put(eColor);
      msg.Put(bStyle);
      msg.Put(bColor);
      msg.Put(bg);
      msg.Put(vendor);
      msg.Finalize();
      ClientServices_Send(&msg);
    } else {
      CGGameUI::DisplayError(GERR_GUILDEMBLEM_COLORSPRESENT);
    }
  } else {
    CGGameUI::DisplayError(GERR_GUILDEMBLEM_INVALID_TABARD_COLORS);
  }
}

bool CGPlayer_C::OnGuildChanged() {
  PlayerNameTriggerNameRegenerate(m_unitNameHandle);
  FrameScript_SignalEvent(359);

  if (!GetTexComponent()) {
    return 0;
  }

  CGBag_C *inventory = GetBag();
  FATALASSERT(inventory);

  CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(inventory->GetItem(18), __FILE__, __LINE__));
  if (!item || item->GetInventoryType() != INDEX_TABARD_TYPE || item->GetDisplayID() <= 0) {
    return 0;
  }

  if (!g_itemDisplayInfoDB.GetRecord(item->GetDisplayID()) || !(g_itemDisplayInfoDB.GetRecord(item->GetDisplayID())->m_flags & 1)) {
    return 0;
  }

  Script_SendUnitSignal(GetGUID(), 324);

  int eStyle;
  int eColor;
  int bStyle;
  int bColor;
  int background;
  if (GuildGetGuildTabard(GetGuildID(), GuildCallback, eStyle, eColor, bStyle, bColor, background) &&
      ComponentApplyTabardTexture(GetTexComponent(), eStyle, eColor, bStyle, bColor, background))
  {
    return 1;
  }

  ComponentRemoveTabardTexture(
      m_unit->sex, GetTexComponent(), const_cast<ItemDisplayInfoRec *>(g_itemDisplayInfoDB.GetRecord(item->GetDisplayID())), item->GetInventoryType()
  );
  return 1;
}

BOOL CGPlayer_C::CanEngageTarget(const CGUnit_C *unitPtr) {
  FATALASSERT(unitPtr);
  if ((GetPosition() - unitPtr->GetPosition()).SquaredMag() >= 10.45f * 10.45f) {
    CGGameUI::DisplayError(GERR_OUT_OF_RANGE);
    return 0;
  }
  return 1;
}

void CGPlayer_C::HandleRepopRequest() {
  if (m_unit->health <= 0) {
    CDataStore msg;
    msg.Put(CMSG_REPOP_REQUEST);
    msg.Finalize();
    ClientServices_Send(&msg);
  }
}

int CGPlayer_C::SwapInventorySlots(int slotA, int slotB) {
  if (slotA == slotB) {
    return 1;
  }
  FATALASSERT(slotA <= 0xff);
  FATALASSERT(slotB <= 0xff);

  CDataStore msg;
  msg.Put(CMSG_SWAP_INV_ITEM);
  msg.Put(static_cast<BYTE>(slotB));
  msg.Put(static_cast<BYTE>(slotA));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static void SwapItemsStatsCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted) {
  const ItemStats_C *stats = g_itemDBCache.GetRecord(id, 0, 0, 0);
  CGPlayer_C        *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  UINT               count = s_pendingSwaps.Count();
  for (UINT index = 0; index < count; ++index) {
    if (s_pendingSwaps[index].pendingID == id) {
      if (stats && player) {
        player->SwapItems(
            0,
            s_pendingSwaps[index].bagA,
            s_pendingSwaps[index].slotA,
            s_pendingSwaps[index].bagB,
            s_pendingSwaps[index].slotB,
            0
        );
      }
      s_pendingSwaps[index].Clear();
    }
  }
}

static UINT FindEmptySwapIndex() {
  UINT index;
  UINT count = s_pendingSwaps.Count();
  for (index = 0; index < count; ++index) {
    if (!s_pendingSwaps[index].bagA) {
      break;
    }
  }

  if (index == count) {
    s_pendingSwaps.SetCount(count + 1);
  }
  return index;
}

void CGPlayer_C::SwapItems(DWORDLONG cursorItem, DWORDLONG cursorContainer, int cursorSlot, DWORDLONG containerB, int slotB, int force) {
  FATALASSERT(cursorSlot <= 0xff);
  FATALASSERT(slotB <= 0xff);

  CDataStore msg;
  if (!cursorContainer) {
    BYTE newContainerSlot = FindSlotIndex(containerB);
    msg.Put(CMSG_PICKUP_ITEM);
    msg.Put(cursorItem);
    msg.Put(newContainerSlot);
    msg.Put(static_cast<BYTE>(slotB));
  } else {
    int cursorIsInv = cursorContainer == GetGUID() && cursorSlot < NUM_INVENTORY_SLOTS;
    int destinationEquipped = containerB == GetGUID() && slotB < NUM_INVENTORY_SLOTS;
    if ((!cursorIsInv && destinationEquipped) || (cursorIsInv && !destinationEquipped)) {
      DWORDLONG   equipContainer = cursorIsInv ? containerB : cursorContainer;
      UINT        equipSlot = cursorIsInv ? slotB : cursorSlot;
      CGObject_C *containerObject = ClntObjMgrObjectPtr(equipContainer, __FILE__, __LINE__);
      if (!containerObject || !containerObject->GetBag()) {
        return;
      }
      CGBag    *bag = containerObject->GetBag();
      CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(bag->GetItem(equipSlot), __FILE__, __LINE__));
      if (item && !(item->m_item->m_dynamicFlags & ITEM_DFLAG_BOUND) && !force) {
        const ItemStats_C *stats = g_itemDBCache.GetRecord(item->GetEntryID(), item->GetGUID(), SwapItemsStatsCallback, 0);
        if (!stats) {
          UINT index = FindEmptySwapIndex();
          s_pendingSwaps[index].bagA = cursorContainer;
          s_pendingSwaps[index].slotA = cursorSlot;
          s_pendingSwaps[index].bagB = containerB;
          s_pendingSwaps[index].slotB = slotB;
          s_pendingSwaps[index].pendingID = item->GetEntryID();
          return;
        }

        GAME_ERROR_TYPE reason;
        if (stats->m_bonding == 2 && CanUseItem(stats, reason)) {
          UINT index = FindEmptySwapIndex();
          s_pendingSwaps[index].bagA = cursorContainer;
          s_pendingSwaps[index].slotA = cursorSlot;
          s_pendingSwaps[index].bagB = containerB;
          s_pendingSwaps[index].slotB = slotB;
          s_pendingSwaps[index].pendingID = item->GetEntryID();
          FrameScript_SignalEvent(269, "%d", index);
          return;
        }
      }
    }

    if (cursorContainer == containerB && cursorContainer == GetGUID()) {
      msg.Put(CMSG_SWAP_INV_ITEM);
      msg.Put(static_cast<BYTE>(cursorSlot));
      msg.Put(static_cast<BYTE>(slotB));
    } else {
      BYTE cursorItemContainerSlot = FindSlotIndex(cursorContainer);
      BYTE newContainerSlot = FindSlotIndex(containerB);
      if (newContainerSlot == 0xFF && containerB != GetGUID()) {
        return;
      }
      msg.Put(CMSG_SWAP_ITEM);
      msg.Put(newContainerSlot);
      msg.Put(static_cast<BYTE>(slotB));
      msg.Put(cursorItemContainerSlot);
      msg.Put(static_cast<BYTE>(cursorSlot));
    }
  }

  msg.Finalize();
  ClientServices_Send(&msg);
  CGGameUI::ClearCursor(0);
}

void CGPlayer_C::SplitItem(DWORDLONG cursorItem, DWORDLONG cursorContainer, int cursorSlot, DWORDLONG containerB, int slotB, int quantity) {
  FATALASSERT(cursorSlot <= 0xff);
  FATALASSERT(slotB <= 0xff);
  FATALASSERT(quantity <= 0xff);
  FATALASSERT(quantity > 0);

  CDataStore msg;
  if (cursorContainer) {
    BYTE cursorItemContainerSlot = FindSlotIndex(cursorContainer);
    BYTE newContainerSlot = FindSlotIndex(containerB);

    msg.Put(CMSG_SPLIT_ITEM);
    msg.Put(cursorItemContainerSlot);
    msg.Put(static_cast<BYTE>(cursorSlot));
    msg.Put(newContainerSlot);
    msg.Put(static_cast<BYTE>(slotB));
    msg.Put(static_cast<BYTE>(quantity));
    msg.Finalize();
    ClientServices_Send(&msg);
  }
}

void CGPlayer_C::DropItemInCursor(DWORDLONG cursorItem, DWORDLONG cursorItemPack, UINT cursorSlot) {
  if (!ClntObjMgrObjectPtr(cursorItem, __FILE__, __LINE__)) {
    return;
  }
  BYTE packSlot = FindSlotIndex(cursorItemPack);
  CDataStore msg;
  NTempest::C3Vector position = GetPosition();
  msg.Put(CMSG_DROP_ITEM);
  msg.Put(packSlot);
  msg.Put(static_cast<BYTE>(cursorSlot));
  msg.Put(position.x);
  msg.Put(position.y);
  msg.Put(position.z);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::AutoStoreItemInBag(DWORDLONG cursorItem, DWORDLONG cursorContainer, int cursorSlot, DWORDLONG containerB, int ignoreOwnershipRules) {
  FATALASSERT(cursorSlot <= 0xff);
  CDataStore msg;
  if (cursorContainer) {
    BYTE cursorItemContainerSlot = FindSlotIndex(cursorContainer);
    BYTE newContainerSlot = FindSlotIndex(containerB);
    if (newContainerSlot == 0xFF && containerB != GetGUID()) {
      return;
    }

    msg.Put(CMSG_AUTOSTORE_BAG_ITEM);
    msg.Put(cursorItemContainerSlot);
    msg.Put(static_cast<BYTE>(cursorSlot));
    msg.Put(newContainerSlot);
  }
  msg.Finalize();
  ClientServices_Send(&msg);
}

BYTE CGPlayer_C::FindSlotIndex(DWORDLONG obj) {
  for (BYTE slot = 0; slot < m_inventory.NumSlots(); ++slot) {
    if (m_inventory.GetItem(slot) == obj) {
      return slot;
    }
  }
  return 0xFF;
}

BOOL CGPlayer_C::HasEquipped(int classID, int subclassID) {
  for (UINT slot = 0; slot < NUM_INVENTORY_SLOTS; ++slot) {
    CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(m_inventory.GetItem(slot), __FILE__, __LINE__));
    if (item && item->GetClassID() == classID && item->GetSubtypeID() == subclassID) {
      return 1;
    }
  }
  return 0;
}

BYTE CGPlayer_C::FindItemSlot(DWORDLONG containerGUID, CGItem_C *item) {
  CGObject_C *container = ClntObjMgrObjectPtr(containerGUID, __FILE__, __LINE__);
  FATALASSERT(container->GetBag());

  BYTE slot = 0;
  while (slot < container->GetBag()->NumSlots()) {
    if (container->GetBag()->GetItem(slot) == item->GetGUID()) {
      return slot;
    }
    ++slot;
  }
  return 0xFF;
}

static void AutoEquipStatsCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted) {
  if (granted) {
    if (g_itemDBCache.GetRecord(id, 0, 0, 0)) {
      CGObject_C *item = ClntObjMgrObjectPtr(CGGameUI::GetCursorItem(), __FILE__, __LINE__);
      if (item && item->GetEntryID() == id) {
        CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
        if (player) {
          player->AutoEquipCursorItem(0);
        }
      }
    }
  }
}

void CGPlayer_C::AutoEquipCursorItem(int force) {
  DWORDLONG cursorItem;
  DWORDLONG cursorItemPack;
  UINT      cursorItemSlot;
  CGGameUI::GetCursorItem(cursorItem, cursorItemPack, cursorItemSlot);
  if (!cursorItem) {
    return;
  }

  if (cursorItemPack == GetGUID() && cursorItemSlot < NUM_INVENTORY_SLOTS) {
    CGGameUI::ClearCursor(1);
    return;
  }

  CDataStore msg;
  if (cursorItemPack) {
    CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(cursorItem, __FILE__, __LINE__));
    if (!item) {
      return;
    }

    BYTE packSlot = FindSlotIndex(cursorItemPack);
    if (!(item->m_item->m_dynamicFlags & ITEM_DFLAG_BOUND) && !force) {
      const ItemStats_C *stats = g_itemDBCache.GetRecord(item->GetEntryID(), cursorItem, AutoEquipStatsCallback, 0);
      if (!stats) {
        return;
      }

      GAME_ERROR_TYPE reason;
      if (stats->m_bonding == 2 && CanUseItem(stats, reason)) {
        UINT      index = FindEmptySwapIndex();
        ITEMSWAP &swap = s_pendingSwaps[index];
        swap.bagA = cursorItemPack;
        swap.slotA = cursorItemSlot;
        swap.bagB = 0;
        swap.slotB = 0;
        swap.pendingID = item->GetEntryID();
        FrameScript_SignalEvent(270, "%d", index);
        return;
      }
    }

    msg.Put(CMSG_AUTOEQUIP_ITEM);
    msg.Put(packSlot);
    msg.Put(static_cast<BYTE>(cursorItemSlot));
  } else {
    msg.Put(CMSG_AUTOEQUIP_GROUND_ITEM);
    msg.Put(cursorItem);
  }
  msg.Finalize();
  ClientServices_Send(&msg);
  CGGameUI::ClearCursor(0);
}

void CGPlayer_C::AutoEquipItem(DWORDLONG container, UINT slot, int force) {
  CGObject_C *containerObject = ClntObjMgrObjectPtr(container, __FILE__, __LINE__);
  if (containerObject && containerObject->GetBag()) {
    DWORDLONG item = containerObject->GetBag()->GetItem(slot);
    if (item) {
      CGGameUI::SetCursorItem(item, container, slot, 1, 0);
      AutoEquipCursorItem(force);
    }
  }
}

void CGPlayer_C::PutLootInSlot(DWORDLONG container, BYTE containerSlot, BYTE lootSlot) {
  CDataStore msg;
  msg.Put(CMSG_STORE_LOOT_IN_SLOT);
  msg.Put(FindSlotIndex(container));
  msg.Put(containerSlot);
  msg.Put(lootSlot);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::AutoStoreLootItem(BYTE slot) {
  CDataStore msg;
  msg.Put(CMSG_AUTOSTORE_LOOT_ITEM);
  msg.Put(slot);
  msg.Finalize();
  ClientServices_Send(&msg);
}
void CGPlayer_C::ClearPendingEquip(UINT index, int equip) {
  if (index >= s_pendingSwaps.Count()) {
    return;
  }

  ITEMSWAP &swap = s_pendingSwaps[index];
  if (equip) {
    if (swap.bagB) {
      SwapItems(0, swap.bagA, swap.slotA, swap.bagB, swap.slotB, 1);
    } else {
      AutoEquipItem(swap.bagA, swap.slotA, 1);
    }
  } else {
    CGObject_C *containerObject = ClntObjMgrObjectPtr(swap.bagA, __FILE__, __LINE__);
    CGBag      *container = containerObject ? containerObject->GetBag() : 0;
    if (container) {
      CGGameUI::UnlockItem(container->GetItem(swap.slotA));
    }

    containerObject = ClntObjMgrObjectPtr(swap.bagB, __FILE__, __LINE__);
    container = containerObject ? containerObject->GetBag() : 0;
    if (container) {
      CGGameUI::UnlockItem(container->GetItem(swap.slotB));
    }
  }

  swap.Clear();
}
void CGPlayer_C::TogglePlayerBounds() {
  static int  boundsPresent;
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  HMODEL      model = player->GetCharacterModel(0);
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

void CGPlayer_C::SellItem(DWORDLONG merchant, DWORDLONG item, UINT amount) {
  CDataStore sellMsg;
  sellMsg.Put(CMSG_SELL_ITEM);
  sellMsg.Put(merchant);
  sellMsg.Put(item);
  sellMsg.Put(static_cast<BYTE>(amount));
  sellMsg.Finalize();
  ClientServices_Send(&sellMsg);
}

void CGPlayer_C::SetActive(const CGPlayer_C *playerPtr) {
  if (ClntObjMgrGetPlayerType() == PLAYER_NORMAL) {
    UnitCombatLogSetActivePlayer(playerPtr);
    CGUnit_C::SetActiveMover(playerPtr ? playerPtr->m_obj->m_guid : 0);
  }
  CMovement::LogWrite("Local player guid (0x%016I64X)\n", ClntObjMgrGetActivePlayer());
}

BOOL CGPlayer_C::SetBlock(UINT i, DWORD data) {
  FATALASSERT(0);
  return 1;
}

void CGPlayer_C::SetData(LPCVOID data, UINT bytes) {
  FATALASSERT(bytes <= sizeof(*m_plyr));
  memcpy(m_plyr, data, bytes);
}

UINT CGPlayer_C::OffsetOf(OBJECT_TYPE_ID type) {
  switch (type) {
    case ID_OBJECT:
      return 0;
    case ID_CONTAINER:
      return 144;
    case ID_ITEM:
    case ID_UNIT:
      return 24;
    case ID_PLAYER:
      return 736;
    default:
      FATALASSERT(0);
      return -1;
  }
}

UINT CGPlayer_C::GetProficiency(BYTE type) {
  return type < 16 ? s_playerProficiencies[type] : 0;
}

void CGPlayer_C::XBuyItem(DWORDLONG merchant, UINT itemID, BYTE quantity, bool autoEquip) {
  CDataStore buyMsg;
  buyMsg.Put(CMSG_BUY_ITEM);
  buyMsg.Put(merchant);
  buyMsg.Put(itemID);
  buyMsg.Put(quantity);
  buyMsg.Put(static_cast<BYTE>(autoEquip));
  buyMsg.Finalize();
  ClientServices_Send(&buyMsg);
}

static BOOL BuyCommandHandler(LPCSTR command, LPCSTR arguments) {
  UINT itemID = SStrToInt(arguments);
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

static BOOL UndressMeHandler(LPCSTR command, LPCSTR arguments) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->DeleteWornItems();
  }
  return 1;
}

static BOOL GodmodeHandler(LPCSTR command, LPCSTR arguments) {
  CDataStore msg;
  msg.Put(CMSG_GODMODE);
  msg << static_cast<BYTE>(SStrToInt(arguments) != 0);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static BOOL CCommand_LevelUp(LPCSTR command, LPCSTR arguments) {
  CDataStore msg;
  msg.Put(CMSG_LEVELUP_CHEAT);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static BOOL CCommand_SetFaction(LPCSTR command, LPCSTR arguments) {
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
    if (candidate->m_reputationIndex >= 0 && !SStrCmpI(candidate->m_name_lang[0], arguments, SStrLen(arguments))) {
      faction = candidate;
      break;
    }
  }

  if (!faction || !faction->m_ID) {
    ConsolePrintf("Unknown faction (usage: setfaction <level> <faction name>)");
    return 1;
  }

  CDataStore msg;
  msg.Put(CMSG_SET_FACTION_CHEAT);
  msg.Put(faction->m_ID);
  msg.Put(level);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static BOOL CCommand_Invite(LPCSTR command, LPCSTR arguments) {
  CDataStore msg;
  msg.Put(CMSG_GROUP_INVITE);
  msg.PutString(arguments);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static BOOL CCommand_Accept(LPCSTR command, LPCSTR arguments) {
  CDataStore msg;
  msg.Put(CMSG_GROUP_ACCEPT);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static BOOL CCommand_Decline(LPCSTR command, LPCSTR arguments) {
  CDataStore msg;
  msg.Put(CMSG_GROUP_DECLINE);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static BOOL CCommand_Disband(LPCSTR command, LPCSTR arguments) {
  CDataStore msg;
  msg.Put(CMSG_GROUP_DISBAND);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static BOOL CCommand_NewLeader(LPCSTR command, LPCSTR arguments) {
  CDataStore msg;
  msg.Put(CMSG_GROUP_SET_LEADER);
  msg.PutString(arguments);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static BOOL CCommand_Uninvite(LPCSTR command, LPCSTR arguments) {
  CDataStore msg;
  msg.Put(CMSG_GROUP_UNINVITE);
  msg.PutString(arguments);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static BOOL CCommand_AcceptRes(LPCSTR, LPCSTR) {
  if (s_resurrectOffer) {
    CDataStore msg;
    msg.Put(CMSG_RESURRECT_RESPONSE);
    msg.Put(s_resurrectOffer);
    msg.Put(static_cast<BYTE>(1));
    msg.Finalize();
    ClientServices_Send(&msg);
    s_resurrectOffer = 0;
  }
  return 1;
}

static BOOL CCommand_DeclineRes(LPCSTR, LPCSTR) {
  if (s_resurrectOffer) {
    CDataStore msg;
    msg.Put(CMSG_RESURRECT_RESPONSE);
    msg.Put(s_resurrectOffer);
    msg.Put(static_cast<BYTE>(0));
    msg.Finalize();
    ClientServices_Send(&msg);
    s_resurrectOffer = 0;
  }
  return 1;
}

static BOOL CCommand_ForceMonsterAnim(LPCSTR, LPCSTR arguments) {
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(CGGameUI::GetLockedTarget(), __FILE__, __LINE__));
  if (unit) {
    unit->SetForcedAnimation(arguments);
  } else {
    ConsoleWrite("Error, unable to get locked target", DEFAULT_COLOR);
  }
  return 1;
}

static BOOL CCommand_ResetMonsterAnim(LPCSTR, LPCSTR) {
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(CGGameUI::GetLockedTarget(), __FILE__, __LINE__));
  if (unit) {
    unit->ResetForcedAnimation();
  }
  return 1;
}

static BOOL DumpDeathLogEnumHandler(DWORDLONG object, LPVOID param) {
  ASSERT(param);
  CGObject_C *objectPtr = ClntObjMgrObjectPtr(object, __FILE__, __LINE__);
  if (objectPtr && objectPtr->IsA(ID_UNIT)) {
    static_cast<CGUnit_C *>(objectPtr)->DumpGeneralDeathHoldLog(static_cast<HSLOG>(param), 0);
  }
  return 1;
}

static BOOL CCommand_DumpDeathHoldLogs(LPCSTR, LPCSTR) {
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

static BOOL CCommand_ShowPet(LPCSTR, LPCSTR) {
  char        buffer[256];
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    SStrPrintf(buffer, sizeof(buffer), "Current pet: 0x%016I64X\n", player->GetControlledGUID());
    ConsoleWrite(buffer, DEFAULT_COLOR);
  }
  return 1;
}

static BOOL CCommand_TaxiShowNodes(LPCSTR, LPCSTR) {
  CDataStore msg;
  msg.Put(CMSG_TAXISHOWNODES);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static BOOL CCommand_GuildCreate(LPCSTR command, LPCSTR arguments) {
  if (arguments && *arguments) {
    CDataStore msg;
    msg.Put(CMSG_GUILD_CREATE);
    msg.PutString(arguments);
    msg.Finalize();
    ClientServices_Send(&msg);
  } else {
    ConsoleWriteA("You must specify a guild name!", ERROR_COLOR);
  }
  return 1;
}

static void PrintForceActionUsage(LPCSTR command) {
  ASSERT(command);
  ConsolePrintf("usage: %s [x] [0|1] where [x] is one of:", command);
  for (UINT i = 0; i < 18; ++i) {
    ConsolePrintf("  %d   %s", i, s_actionsArray[i]);
  }
}

static void SendForceActionMessage(int set, int onSelf, UINT argument) {
  CDataStore msg;
  if (onSelf) {
    msg.Put(CMSG_FORCEACTION);
  } else {
    msg.Put(CMSG_FORCEACTIONONOTHER);
  }
  msg.Put(argument);
  msg.Put(static_cast<UINT>(set));
  msg.Finalize();
  ClientServices_Send(&msg);
}

static BOOL CCommand_ForceActionSet(LPCSTR command, LPCSTR arguments) {
  if (arguments && *arguments) {
    UINT argument = SStrToUnsigned(arguments);
    if (argument >= 18) {
      ConsoleWrite("invalid argument", DEFAULT_COLOR);
    } else {
      SendForceActionMessage(1, 1, argument);
    }
  } else {
    PrintForceActionUsage(command);
  }
  return 1;
}

static BOOL CCommand_ForceActionUnset(LPCSTR command, LPCSTR arguments) {
  if (arguments && *arguments) {
    UINT argument = SStrToUnsigned(arguments);
    if (argument >= 18) {
      ConsoleWrite("invalid argument", DEFAULT_COLOR);
    } else {
      SendForceActionMessage(0, 1, argument);
    }
  } else {
    PrintForceActionUsage(command);
  }
  return 1;
}

static BOOL CCommand_ForceActionOnOtherSet(LPCSTR command, LPCSTR arguments) {
  if (arguments && *arguments) {
    UINT argument = SStrToUnsigned(arguments);
    if (argument >= 18) {
      ConsoleWrite("invalid argument", DEFAULT_COLOR);
    } else {
      SendForceActionMessage(1, 0, argument);
    }
  } else {
    PrintForceActionUsage(command);
  }
  return 1;
}

static BOOL CCommand_ForceActionOnOtherUnset(LPCSTR command, LPCSTR arguments) {
  if (arguments && *arguments) {
    UINT argument = SStrToUnsigned(arguments);
    if (argument >= 18) {
      ConsoleWrite("invalid argument", DEFAULT_COLOR);
    } else {
      SendForceActionMessage(0, 0, argument);
    }
  } else {
    PrintForceActionUsage(command);
  }
  return 1;
}

static BOOL CCommand_ForceActionShowFlags(LPCSTR command, LPCSTR arguments) {
  CDataStore msg;
  msg.Put(CMSG_FORCEACTIONSHOW);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static BOOL CCommand_TogglePVP(LPCSTR command, LPCSTR arguments) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    BYTE       enable = !(player->GetPlayerFlags() & 1);
    CDataStore msg;
    msg.Put(CMSG_ENABLE_PVP);
    msg.Put(enable);
    msg.Finalize();
    ClientServices_Send(&msg);
    ConsoleWrite(enable ? "PVP Enabled" : "PVP Disabled", DEFAULT_COLOR);
  }
  return 1;
}

static BOOL CCommand_Cinematic(LPCSTR command, LPCSTR arguments) {
  CDataStore msg;
  msg.Put(CMSG_TRIGGER_CINEMATIC_CHEAT);
  msg.Put(SStrToInt(arguments));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static BOOL OnProficiency(LPVOID, NETMESSAGE, DWORD, CDataStore *msg) {
  BYTE proficiencyClass;
  UINT proficiencyMask;
  msg->Get(proficiencyClass);
  msg->Get(proficiencyMask);
  s_playerProficiencies[proficiencyClass] = proficiencyMask;
  ConsolePrintf("Proficiency in item class %d set to %08x", proficiencyClass, proficiencyMask);
  CGCharacterInfo::UpdateAllSkillLines();
  return 1;
}

static void ResurrectNameQueryCallback(int id, const DWORDLONG &guid, LPVOID, bool) {
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

static BOOL OnResurrectRequest(LPVOID, NETMESSAGE, DWORD, CDataStore *msg) {
  DWORDLONG guid;
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

static BOOL OnInspectNotify(LPVOID, NETMESSAGE, DWORD, CDataStore *msg) {
  DWORDLONG guid;
  msg->Get(guid);
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (unit) {
    CGGameUI::DisplayError(GERR_INSPECT_S, unit->GetUnitName());
  }
  return 1;
}

void CGPlayer_C::UpdateQuestStatus(const DWORDLONG &guid) {
  CDataStore msg;
  msg.Put(CMSG_QUESTGIVER_STATUS_QUERY);
  msg.Put(guid);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::UpdateQuestStatus(CGUnit_C *unit) {
  UpdateQuestStatus(unit->GetGUID());
}

static BOOL OnReadItemResult(LPVOID, NETMESSAGE msgID, DWORD, CDataStore *msg) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->ReadItemResult(msgID, msg);
  }
  return 1;
}

static BOOL OnCancelCombat(LPVOID, NETMESSAGE, DWORD, CDataStore *) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->SetCombatMode(0);
  }
  return 1;
}

static BOOL AreaTriggerCheck(LPCVOID eventData, LPVOID arg) {
  CGPlayer_C *playerPtr = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));

  if (playerPtr) {
    int                worldId = CGPlayer_C::GetNewContinentID();
    NTempest::C3Vector pos = playerPtr->GetPosition();

    if (currentAreaTrigger) {
      const AreaTriggerRec *rec = g_areaTriggerDB.GetRecord(currentAreaTrigger);
      FATALASSERT(rec);

      if (worldId == rec->m_ContinentID) {
        if (NTempest::CAaSphere(NTempest::C3Vector(rec->m_x, rec->m_y, rec->m_z), rec->m_radius * 1.1f).Contains(pos)) {
          goto reset_timer;
        }
      }

      currentAreaTrigger = 0;
    }

    for (int index = 0; index < g_areaTriggerDB.GetNumRecords(); ++index) {
      const AreaTriggerRec *rec = g_areaTriggerDB.GetRecordByIndex(index);
      FATALASSERT(rec);

      if (rec->m_ContinentID == worldId) {
        if (NTempest::CAaSphere(NTempest::C3Vector(rec->m_x, rec->m_y, rec->m_z), rec->m_radius).Contains(pos)) {
          CDataStore msg;
          msg.Put(CMSG_AREATRIGGER);
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
  ConsoleCommandRegister("pet", CCommand_ShowPet, DEBUG, "Displays GUID of pet");
  ConsoleCommandRegister("TaxiShowNodes", CCommand_TaxiShowNodes, DEBUG, "show all available taxi nodes");
  ConsoleCommandRegister("guildcreate", CCommand_GuildCreate, GAME, "Create a guild. Usage: 'guildcreate <guild name>'");
  ConsoleCommandRegister("TogglePVP", CCommand_TogglePVP, GAME, "Enable or disable PVP for yourself");
  ConsoleCommandRegister("cinematic", CCommand_Cinematic, DEBUG, "Start an in-game cinematic");
  ConsoleCommandRegister("CombatDebugForceActionOn", CCommand_ForceActionSet, DEBUG, "");
  ConsoleCommandRegister("CombatDebugForceActionOff", CCommand_ForceActionUnset, DEBUG, "");
  ConsoleCommandRegister("CombatDebugForceActionOtherOn", CCommand_ForceActionOnOtherSet, DEBUG, "");
  ConsoleCommandRegister("CombatDebugForceActionOtherOff", CCommand_ForceActionOnOtherUnset, DEBUG, "");
  ConsoleCommandRegister("CombatDebugShowFlags", CCommand_ForceActionShowFlags, DEBUG, "");
  ConsoleCommandRegister("forceanim", CCommand_ForceMonsterAnim, DEBUG, "Force a monster animation");
  ConsoleCommandRegister("resetanim", CCommand_ResetMonsterAnim, DEBUG, "Force a monster animation");
  ConsoleCommandRegister("DumpDeathHoldLogs", CCommand_DumpDeathHoldLogs, DEBUG, "");

  memset(s_playerProficiencies, 0, sizeof(s_playerProficiencies));
  s_attackBreakTimer = 0;
  s_combatModeTimer = 0;
  s_enableDeathHoldLog = 0;
  s_renderPlayer = 1;
  g_combatModeMaxDistance =
      CVar::Register("CombatModeMaxDistance", "Specifies the range outside of which combat mode is impossible", 0, "30.0f", 0, 3, false, 0);
  s_namePlateRenderOwn = CVar::Register("UnitNameRenderOwn", "Toggles rendering of local player's nameplate", 0, "0", 0, 1, false, 0);
  s_resurrectOffer = 0;
  PlayerInitializeSounds();
  AreaTriggersInitialize();
  s_guildIDs.Clear();
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
  ConsoleCommandUnregister("pet");
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
  s_guildIDs.Clear();
}

int Player_C_AppFocusMovementHandler(int focus) {
  if (focus) {
    return 1;
  }

  DWORD       eventTime = OsGetAsyncTimeMs();
  DWORDLONG   guid = ClntObjMgrGetActivePlayer();
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (player) {
    if (player->IsClientControlled()) {
      player->OnMoveStopLocal(eventTime);
      player->OnStrafeStopLocal(eventTime);
    }
  }

  return 1;
}

static BOOL CountWeaponItemSubclasses(int *number) {
  ASSERT(number);

  int maxSubclass = 0;
  int found = 0;
  for (int index = g_itemSubClassDB.GetNumRecords(); index;) {
    --index;
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

static BOOL FindFirstSetBit(UINT field, int *whichBitSet) {
  int result = 1;
  if (!field || (field & (field - 1))) {
    result = 0;
  }
  int firstBit = 0x7FFFFFFF;

  for (UINT index = 0; index < 32; ++index) {
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
  for (UINT i = 0; i < numSubclasses; ++i) {
    s_weaponSubclassSpells[i] = 0;
  }

  s_defenseSkillID = 0;
  for (int index = g_spellDB.GetNumRecords(); index;) {
    --index;
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

bool CGPlayer_C::CanTrack(const CGUnit_C *unit) {
  if (unit->IsBeingStalked()) {
    return 1;
  }

  int creatureType = unit->GetCreatureType();
  if (!creatureType) {
    return 0;
  }

  if (GetCreatureTracking() & (1 << (creatureType - 1))) {
    return true;
  }
  return false;
}

bool CGPlayer_C::CanTrack(const CGGameObject_C *object) {
  UINT           trackMask = GetResourceTracking();
  const LockRec *lock = object->GetLockRec();
  if (!lock) {
    return 0;
  }

  for (UINT index = 0; index < 4; ++index) {
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

UINT CGPlayer_C::DetermineWoundSequence() const {
  HMODEL charModel = GetCharacterModel(0);
  FATALASSERT(charModel);
  UINT result;
  if (!(m_unit->flags & 0x20000) && const_cast<CCombatClient &>(m_combat).IsAttacking() && ModelHasSequenceId(charModel, 9)) {
    result = 9;
  } else if (ModelHasSequenceId(charModel, 8)) {
    result = 8;
  } else {
    FATALASSERT(m_readySequence != 0xffffffff);
    result = m_readySequence;
  }
  HandleClose(charModel);
  return result;
}

void CGPlayer_C::AcceptGroup() {
  CDataStore msg;
  msg.Put(CMSG_GROUP_ACCEPT);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::DeclineGroup() {
  CDataStore msg;
  msg.Put(CMSG_GROUP_DECLINE);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::LeaveGroup() {
  CDataStore msg;
  msg.Put(CMSG_GROUP_DISBAND);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::SetLootMethod(LOOT_METHOD method, DWORDLONG master) {
  CDataStore msg;
  msg.Put(CMSG_LOOT_METHOD);
  msg.Put(static_cast<UINT>(method));
  msg.Put(master);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::AcceptGuild() {
  CDataStore msg;
  msg.Put(CMSG_GUILD_ACCEPT);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::DeclineGuild() {
  CDataStore msg;
  msg.Put(CMSG_GUILD_DECLINE);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::DeleteWornItems() const {
  CDataStore msg;
  msg.Put(CMSG_UNDRESSPLAYER);
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
    const SkillLineAbilityRec *ability = SpellTableLookupAbility(m_unit->race, m_unit->classId, spellID);
    const SkillLineRec        *skillLine = ability ? g_skillLineDB.GetRecord(ability->m_skillLine) : 0;
    if (skillLine) {
      HASHKEY_NONE    hashKey;
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
    UINT index;
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
    for (UINT index = 0; index < m_craftSpells[craftType].Count(); ++index) {
      if (found) {
        m_craftSpells[craftType][index - 1] = m_craftSpells[craftType][index];
      } else if (m_craftSpells[craftType][index] == spellID) {
        found = 1;
      }
    }
    if (found) {
      m_craftSpells[craftType].SetCount(m_craftSpells[craftType].Count() - 1);
      CGCraftInfo::RefreshList();
    }
  }

  CGSpellBook::DelKnownSpell(spellID);
  CGActionBar::RemoveSpell(spellID);
}

ITEMEXPIRATION *CGPlayer_C::GetPendingItemExpirationNode(const DWORDLONG &itemGUID) {
  ITEMEXPIRATION *itemNode = s_pendingItemExpirations.Ptr(static_cast<UINT>(itemGUID), CHashKeyGUID(itemGUID));
  if (!itemNode) {
    itemNode = s_pendingItemExpirations.New(static_cast<UINT>(itemGUID), CHashKeyGUID(itemGUID), 0, 0);
    itemNode->timeLeft = 0;
    memset(itemNode->enchantmentTimeLeft, 0, sizeof(itemNode->enchantmentTimeLeft));
  }
  return itemNode;
}

void CGPlayer_C::UpdatePendingItemExpiration(const DWORDLONG &itemGUID) {
  ITEMEXPIRATION *itemNode = s_pendingItemExpirations.Ptr(static_cast<UINT>(itemGUID), CHashKeyGUID(itemGUID));
  if (itemNode) {
    CGItem_C *itemPtr = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(itemGUID, __FILE__, __LINE__));
    if (itemPtr) {
      if (itemNode->timeLeft) {
        itemPtr->UpdateExpirationTime(itemNode->timeLeft);
      }
      for (UINT i = 0; i < 5; ++i) {
        if (itemNode->enchantmentTimeLeft[i]) {
          itemPtr->UpdateEnchantmentTime(i, itemNode->enchantmentTimeLeft[i]);
        }
      }
      s_pendingItemExpirations.Delete(itemNode);
    }
  }
}

const TSGrowableArray<int> *CGPlayer_C::GetTradeSkills(int skillLine) const {
  HASHKEY_NONE          hashKey;
  const TRADESKILLLINE *line = m_tradeSkillLines.Ptr(skillLine, hashKey);
  return line ? &line->spells : 0;
}

const TSGrowableArray<int> *CGPlayer_C::GetCraftSkills(SPELL_CAST_UI_TYPE type) const {
  if (type == SPELL_CAST_UI_NONE || type >= NUM_SPELL_CAST_UI_TYPES) {
    return 0;
  }
  return &m_craftSpells[type];
}

int CGPlayer_C::GetCraftSkillActivator(SPELL_CAST_UI_TYPE type) const {
  if (static_cast<UINT>(type) >= NUM_SPELL_CAST_UI_TYPES) {
    return 0;
  }
  return m_craftActivators[type];
}

int CGPlayer_C::GetSkillIndex(int skillID) const {
  UINT index;
  for (index = 0; index < 64; ++index) {
    if (GetMirrorSkillID(index) == skillID) {
      break;
    }
  }
  return index == 64 ? -1 : static_cast<int>(index);
}

int CGPlayer_C::GetSkillRank(int skillID) const {
  int index = GetSkillIndex(skillID);
  if (index >= 0) {
    int rank = GetMirrorSkillRank(index) + GetMirrorSkillModifier(index);
    if (rank >= 0) {
      return rank;
    }
  }
  return 0;
}

int CGPlayer_C::ValidateSlot(UINT slotID, DWORDLONG cursorItem) {
  CGObject_C *object = ClntObjMgrObjectPtr(cursorItem, __FILE__, __LINE__);
  if (!object) {
    return 0;
  }

  if (object->IsA(ID_CONTAINER) && !(slotID >= 19 && slotID <= 22) && !(slotID >= 63 && slotID <= 68) && object->GetBag()->NumItems()) {
    return 0;
  }
  return static_cast<CGItem_C *>(object)->CanGoInSlot(slotID);
}

UNITAFFILIATION CGPlayer_C::GetGUIDAffiliation(DWORDLONG unit) const {
  UNITAFFILIATION affiliation = CGUnit_C::GetGUIDAffiliation(unit);
  if (affiliation == AFFILIATION_OTHER) {
    affiliation = CGGameUI::IsPartyMember(unit) ? AFFILIATION_PARTYMEMBER : AFFILIATION_OTHER;
  }
  return affiliation;
}

void CGPlayer_C::CheckWeaponDefenseRankChange(COMBATHAND) const {
  FrameScript_SignalEvent(0x136, "%s", "player");
}

void CGPlayer_C::CheckWeaponDefenseRankChange() const {
  CheckWeaponRankChange();
  CheckDefenseRankChange();
}

void CGPlayer_C::CheckWeaponRankChange() const {
  FrameScript_SignalEvent(0x136, "%s", "player");
}

void CGPlayer_C::CheckDefenseRankChange() const {
  FrameScript_SignalEvent(0x137, "%s", "player");
}

int CGPlayer_C::GetSpellRank(int spellID) const {
  const SpellRec *spell = g_spellDB.GetRecord(spellID);
  if (!spell) {
    return 0;
  }

  const SkillLineAbilityRec *ability = SpellTableLookupAbility(m_unit->race, m_unit->classId, spellID);
  if (!ability) {
    return 0;
  }

  int rank = GetSkillRank(ability->m_skillLine);
  if (spell->m_attributes & 0x406) {
    int weaponSpell;
    if (spell->m_attributes & 2) {
      const VirtualItemInfo *item = GetVirtualItem(VIRTUAL_MONSTER_SLOT_RANGED, 0);
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
  if (rank < 0) {
    rank = 0;
  }
  return rank;
}

int CGPlayer_C::GetWeaponSpell(COMBATHAND hand) const {
  if (hand >= NUMHANDS) {
    return 0;
  }

  const VirtualItemInfo *item = GetVirtualItem(g_monsterHands[hand], 0);
  UINT                   subclass;
  if (!item) {
    subclass = ClientDBGetUnarmedWeapon();
  } else {
    if (item->m_classID != 2) {
      return 0;
    }
    subclass = item->m_subclassID;
  }

  if (subclass < s_weaponSubclassSpells.Count()) {
    return s_weaponSubclassSpells.Ptr()[subclass];
  }
  return 0;
}

UINT CGPlayer_C::GetNewContinentID() {
  return ClntObjMgrGetMapID();
}

void CGPlayer_C::MoveItem(DWORDLONG item, DWORDLONG itemContainer, UINT slot, DWORDLONG newContainer, UINT newSlot) {
}

BOOL CGPlayer_C::OnAttackBreakHandler() {
  DWORDLONG target = m_combat.IsAttacking();
  if (!target) {
    return 0;
  }

  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(target, __FILE__, __LINE__));
  if (!unitPtr) {
    return 0;
  }

  FATALASSERT(m_flags & 0x00000010);
  if (m_flags & 0x00000020) {
    NTempest::C3Vector here = GetPosition();
    NTempest::C3Vector there = unitPtr->GetPosition();
    if ((here - there).SquaredMag() >= s_attackBreakDistanceSquared) {
      return 0;
    }
  }

  return 1;
}

void CGPlayer_C::LootMoney() {
  if (m_lootingUnit) {
    CDataStore msg;
    msg.Put(CMSG_LOOT_MONEY);
    msg.Finalize();
    ClientServices_Send(&msg);
  }
}

BOOL CGPlayer_C::CanUseItem(const ItemStats *stats, GAME_ERROR_TYPE &reason) {
  reason = GERR_NUM_TYPES;
  if (!stats) {
    reason = GERR_ITEM_NOT_FOUND;
    return 0;
  }

  if (stats->m_requiredLevel > m_unit->level) {
    reason = GERR_CANT_EQUIP_LEVEL_I;
    return 0;
  }
  if (!(stats->m_allowableClass & (1 << (m_unit->classId - 1))) || !(stats->m_allowableRace & (1 << (m_unit->race - 1)))) {
    reason = GERR_CANT_EQUIP_EVER;
    return 0;
  }

  UINT proficiency = GetProficiency(static_cast<BYTE>(stats->m_class));
  if (proficiency && !(proficiency & (1 << stats->m_subclass))) {
    reason = GERR_PROFICIENCY_NEEDED;
    return 0;
  }
  if (stats->m_requiredSkill && GetSkillRank(stats->m_requiredSkill) < stats->m_requiredSkillRank) {
    reason = GERR_CANT_EQUIP_SKILL;
    return 0;
  }
  return 1;
}

void CGPlayer_C::QueryQuest(const DWORDLONG &questGiver, int questID) {
  CDataStore msg;
  msg.Put(CMSG_QUESTGIVER_QUERY_QUEST);
  msg.Put(questGiver);
  msg.Put(questID);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::AcceptQuest(const DWORDLONG &questGiver, int questID) {
  CDataStore msg;
  msg.Put(CMSG_QUESTGIVER_ACCEPT_QUEST);
  msg.Put(questGiver);
  msg.Put(questID);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::CompleteQuest(const DWORDLONG &questGiver, int questID) {
  CDataStore msg;
  msg.Put(CMSG_QUESTGIVER_COMPLETE_QUEST);
  msg.Put(questGiver);
  msg.Put(questID);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::GiveQuestItems(const DWORDLONG &questGiver, int questID) {
  CDataStore msg;
  msg.Put(CMSG_QUESTGIVER_REQUEST_REWARD);
  msg.Put(questGiver);
  msg.Put(questID);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::GetQuestReward(const DWORDLONG &questGiver, int questID, int itemChoice) {
  CDataStore msg;
  msg.Put(CMSG_QUESTGIVER_CHOOSE_REWARD);
  msg.Put(questGiver);
  msg.Put(questID);
  msg.Put(itemChoice);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::CancelQuest(const DWORDLONG &questGiver) {
  CDataStore msg;
  msg.Put(CMSG_QUESTGIVER_CANCEL);
  msg.Put(questGiver);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::QuestLogRemoveQuest(int entry) {
  CDataStore msg;
  msg.Put(CMSG_QUESTLOG_REMOVE_QUEST);
  msg.Put(static_cast<BYTE>(entry));
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::QuestLogSwapQuest(int entry1, int entry2) {
  CDataStore msg;
  msg.Put(CMSG_QUESTLOG_SWAP_QUEST);
  msg.Put(static_cast<BYTE>(entry1));
  msg.Put(static_cast<BYTE>(entry2));
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::TrainerBuySpell(const DWORDLONG &trainer, int spellID) {
  CDataStore msg;
  msg.Put(CMSG_TRAINER_BUY_SPELL);
  msg.Put(trainer);
  msg.Put(spellID);
  msg.Finalize();
  ClientServices_Send(&msg);
}

static BOOL QuestUpdateProc(DWORDLONG guid, LPVOID) {
  CGObject_C *object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
  if (object) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player && (object->IsA(ID_UNIT) || object->IsA(ID_GAMEOBJECT)) && !object->IsA(ID_PLAYER) &&
        (!object->IsA(ID_GAMEOBJECT) || (static_cast<CGGameObject_C *>(object)->GetQuestOnly() &&
                                         static_cast<CGGameObject_C *>(object)->ObjectReaction(player) != UNIT_REACTION_HOSTILE)) &&
        (!object->IsA(ID_UNIT) || ((static_cast<CGUnit_C *>(object)->GetUnitNPCFlags() & 2) &&
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
    msg.Put(CMSG_TAXINODE_STATUS_QUERY);
    msg.Put(unit->GetGUID());
    msg.Finalize();
    ClientServices_Send(&msg);
  }
}

static BOOL TaxiUpdateProc(DWORDLONG guid, LPVOID param) {
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (unit && unit->IsA(ID_UNIT) && (unit->GetUnitNPCFlags() & 4)) {
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
  if (unit->UnitReaction(this) > UNIT_REACTION_HOSTILE && (unit->m_unit->npcFlags & 0x10)) {
    if (!DeathBindDistanceCompare(unit->GetPosition())) {
      unit->UpdateInteractIcon(INTERACTICON_BINDER);
    } else {
      unit->UpdateInteractIcon(INTERACTICON_NONE);
    }
  }
}

static BOOL BindUpdateProc(DWORDLONG guid, LPVOID param) {
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (unit && unit->IsA(ID_UNIT) && (unit->GetUnitNPCFlags() & 0x10)) {
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
    BYTE playerAnimState = m_unit->weaponMode;
    if (playerAnimState != WEAPONMODE_SHEATHEDMODE || (m_lastWeaponModeSent != -1 && m_lastWeaponModeSent != WEAPONMODE_SHEATHEDMODE)) {
      ToggleSheathe(1);
    }
  }
}

void CGPlayer_C::OnItemDelete(DWORDLONG item, DWORDLONG listener) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(listener, __FILE__, __LINE__));
  FATALASSERT(player);
  player->OnItemDelete(item);
}

void CGPlayer_C::OnItemDelete(DWORDLONG item) {
  FATALASSERT(item);
  int slot = m_inventory.GetIndexOfObject(item);
  if (slot != -1 && IsSlotComponented(slot, 1)) {
    RemoveComponent(slot, 1, 0, 1);
  }
}

void CGPlayer_C::ToggleSheathe(bool ignoreAnim) {
  if (m_unit->health > 0 && m_currentTorsoAnimState != ANIM_STATE_EMOTE && !m_castingSpell && (ignoreAnim || !SheatheAnimPlaying())) {
    BYTE weaponMode = m_unit->weaponMode;
    if (weaponMode == WEAPONMODE_RANGEDMODE || weaponMode == WEAPONMODE_SHEATHEDMODE) {
      SetWeaponMode(WEAPONMODE_NORMALMODE);
    } else {
      SetWeaponMode(WEAPONMODE_SHEATHEDMODE);
      if (m_flags & 0x400) {
        SetCombatMode(0);
      }
    }

    if (g_standStateAllowsSheathing[m_unit->standState]) {
      MaybeStartSheatheAnim();
    }
  }
}

BOOL CGPlayer_C::OnLootResponse(UINT eventTime, CDataStore *msg) {
  DWORDLONG   objectGUID;
  CGObject_C *lootobject;
  UINT        coins;
  BYTE        accquired;
  BYTE        reason;
  BYTE        slot;
  BYTE        count;

  msg->Get(objectGUID);
  msg->Get(accquired);

  if ((!m_lootingUnitSent || objectGUID != m_lootingUnitSent) &&
      (m_lootingUnitSent || (accquired != LOOT_ACQUIRE_PICKPOCKET && accquired != LOOT_ACQUIRE_FISHING)))
  {
    if (accquired) {
      CDataStore lootRelease;
      lootRelease.Put(CMSG_LOOT_RELEASE);
      lootRelease.Put(objectGUID);
      lootRelease.Finalize();
      ClientServices_Send(&lootRelease);
    }
    m_lootingUnitSent = 0;
    return 1;
  }

  if (accquired) {
    msg->Get(coins);
    msg->Get(count);
    if (count > 16) {
      count = 16;
    }

    lootobject = ClntObjMgrObjectPtr(objectGUID, __FILE__, __LINE__);
    if (lootobject) {
      m_lootingUnit = objectGUID;
      memset(s_lootItems, 0, sizeof(s_lootItems));
      for (UINT index = 0; index < count; ++index) {
        msg->Get(slot);
        msg->Get(s_lootItems[slot].m_itemID);
        msg->Get(s_lootItems[slot].m_quantity);
        msg->Get(s_lootItems[slot].m_displayID);
      }
      CGGameUI::OpenLoot(lootobject, coins, static_cast<LOOT_ACQUIRE>(accquired));
    } else {
      CGGameUI::DisplayError(GERR_LOOT_DIDNT_KILL);
    }
    return 1;
  }

  msg->Get(reason);
  switch (reason) {
    case 4:
      CGGameUI::DisplayError(GERR_LOOT_TOO_FAR);
      break;
    case 5:
      CGGameUI::DisplayError(GERR_LOOT_BAD_FACING);
      break;
    case 6:
      CGGameUI::DisplayError(GERR_LOOT_LOCKED);
      break;
    case 8:
      CGGameUI::DisplayError(GERR_LOOT_NOTSTANDING);
      break;
    case 9:
      CGGameUI::DisplayError(GERR_LOOT_STUNNED);
      break;
    default:
      CGGameUI::DisplayError(GERR_LOOT_DIDNT_KILL);
      break;
  }
  m_lootingUnitSent = 0;
  if (m_currentBaseAnimState == ANIM_STATE_LOOTBEGIN) {
    UpdateBaseAnimation(0, 0);
  }
  return 1;
}

UINT CGPlayer_C::GetLootItem(UINT slot) {
  FATALASSERT(slot < MAX_LOOT_ITEMS);
  return s_lootItems[slot].m_itemID;
}

UINT CGPlayer_C::GetLootItemDisplayID(UINT slot) {
  FATALASSERT(slot < MAX_LOOT_ITEMS);
  return s_lootItems[slot].m_displayID;
}

UINT CGPlayer_C::GetLootItemQuantity(UINT slot) {
  FATALASSERT(slot < MAX_LOOT_ITEMS);
  return s_lootItems[slot].m_quantity;
}

BOOL CGPlayer_C::OnLootRemoved(CDataStore *msg) {
  BYTE slot;
  msg->Get(slot);
  if (ClntObjMgrObjectPtr(m_lootingUnit, __FILE__, __LINE__)) {
    s_lootItems[slot].m_itemID = 0;
    s_lootItems[slot].m_displayID = 0;
    s_lootItems[slot].m_quantity = 0;
    CGGameUI::ClearLootSlot(slot);
  }
  return 1;
}

BOOL CGPlayer_C::OnLootMoneyNotify(CDataStore *msg) {
  char string[128];
  char buf[128];
  char coinBuf[64];
  char moneyBuf[64];
  char coinName[32];
  int  coins[3];
  UINT money;

  msg->Get(money);
  if (money) {
    CurrencyBreakdown(money, coins);
    int first = 1;
    for (int coin = 2; coin >= 0; --coin) {
      if (coins[coin]) {
        LPCSTR text = FrameScript_GetText(CurrencyAbbreviation(coin), -1, GENDER_NOT_APPLICABLE);
        SStrCopy(coinName, text, sizeof(coinName));
        if (first) {
          SStrPrintf(moneyBuf, sizeof(moneyBuf), "%d %s", coins[coin], coinName);
          first = 0;
        } else {
          SStrPrintf(coinBuf, sizeof(coinBuf), ", %d %s", coins[coin], coinName);
          SStrPack(moneyBuf, coinBuf, sizeof(moneyBuf));
        }
      }
    }
    LPCSTR text = FrameScript_GetText("LOOT_MONEY_SPLIT", -1, GENDER_NOT_APPLICABLE);
    SStrCopy(buf, text, sizeof(buf));
    SStrPrintf(string, sizeof(string), buf, moneyBuf);
    CGChat::AddChatMessage(string, SLASH_CMD_SYSTEM, 0, 0, 0, 0, 0);
  }
  return 1;
}

BOOL CGPlayer_C::OnLootClearMoney(CDataStore *msg) {
  CGLootInfo::CoinsCleared();
  return 1;
}

BOOL CGPlayer_C::OnLootItemNotify(CDataStore *msg) {
  char      itemName[64];
  DWORDLONG player;
  int       displayID;
  BYTE      pushed;
  BYTE      slot;
  msg->Get(player);
  msg->Get(pushed);
  msg->Get(slot);
  msg->Get(displayID);
  msg->GetString(itemName, 64);
  return 1;
}

BOOL CGPlayer_C::OnLootReleaseResponse(CDataStore *msg) {
  DWORDLONG packGUID;
  BYTE      success;
  msg->Get(packGUID);
  msg->Get(success);
  if (success && packGUID == m_lootingUnit) {
    m_lootingUnit = 0;
    if (m_currentBaseAnimState == ANIM_STATE_LOOTBEGIN) {
      UpdateBaseAnimation(ANIM_STATE_LOOTEND, 0);
    }
    m_flags &= ~0x200;
    CGGameUI::CloseLoot(false, false);
  }
  return 1;
}

void CGPlayer_C::LootAnimEndHandler() {
  if (!m_lootingUnit && !(m_unit->flags & 0x400)) {
    UpdateBaseAnimation(GetAnimationState(), 0);
  }
}

void CGPlayer_C::ReadItem(BYTE packSlot, BYTE slot) {
  CDataStore msg;
  msg.Put(CMSG_READ_ITEM);
  msg.Put(packSlot);
  msg.Put(slot);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::ReadItem(DWORDLONG containerGUID, BYTE slot) {
  typedef void (CGPlayer_C::*ReadPackItemProc)(BYTE, BYTE);
  ReadPackItemProc readPackItem = &CGPlayer_C::ReadItem;

  if (containerGUID == GetGUID()) {
    (this->*readPackItem)(0xFF, slot);
    return;
  }

  for (UINT packSlot = 19; packSlot < 23; ++packSlot) {
    DWORDLONG item = m_inventory.GetItem(packSlot);
    if (item == containerGUID) {
      (this->*readPackItem)(packSlot, slot);
      return;
    }
  }
}

BOOL CGPlayer_C::CanLoot(CGUnit_C *unitPtr) {
  FATALASSERT(unitPtr);
  return (GetPosition() - unitPtr->GetPosition()).SquaredMag() <=
             (m_unit->combatReach + m_unit->boundingRadius + unitPtr->m_unit->combatReach + unitPtr->m_unit->boundingRadius + 1.333333373f) *
                 1.049999952f * 0.899999976f *
                 (m_unit->combatReach + m_unit->boundingRadius + unitPtr->m_unit->combatReach + unitPtr->m_unit->boundingRadius + 1.333333373f) *
                 1.049999952f * 0.899999976f &&
         unitPtr->GetGUID() != m_lootingUnit && !m_lootingUnitSent && m_unit->health > 0 && !(m_flags & 0x4000) && !(m_unit->flags & 0x40000);
}

UINT CGPlayer_C::GetPlayerAnimState() {
  if (GetGUID() == ClntObjMgrGetActivePlayer()) {
    if (!(m_flags & 0x200) && (m_lootingUnit || m_lootingUnitSent) &&
        GetAnimPriority(m_currentBaseAnimState) <= GetAnimPriority(ANIM_STATE_LOOTBEGIN) &&
        GetAnimPriority(m_currentTorsoAnimState) <= GetAnimPriority(ANIM_STATE_LOOTBEGIN))
    {
      DWORDLONG   lootTarget = m_lootingUnit ? m_lootingUnit : m_lootingUnitSent;
      CGObject_C *object = ClntObjMgrObjectPtr(lootTarget, __FILE__, __LINE__);
      if (!object || !object->IsA(ID_ITEM)) {
        return ANIM_STATE_LOOTBEGIN;
      }
    }
  } else if (!(m_flags & 0x200)) {
    if (IsLooting() && !static_cast<BYTE>(m_move.m_moveFlags)) {
      return ANIM_STATE_LOOTBEGIN;
    }
  }

  return 0;
}

void CGPlayer_C::AttachObjComponent(DWORDLONG item, UINT slot, bool defer, bool sheathe, int sheatheAttachmentSlot) {
  CGItem_C *itemptr = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(item, __FILE__, __LINE__));
  if (!itemptr || !((1 << slot) & 0x38005)) {
    return;
  }

  bool showHidden = false;
  if ((slot == INVSLOT_RANGED && m_unit->weaponMode != WEAPONMODE_RANGEDMODE) || (slot == INVSLOT_MAINHAND && (m_unit->flags & 0x200000))) {
    showHidden = true;
  }

  int                    subtypeID = itemptr->GetSubtypeID();
  const ItemSubClassRec *subclass = SDBItemSubclassGetSubClassRec(itemptr->GetClassID(), subtypeID);
  bool                   forceAlternate = subclass && (subclass->m_flags & 0x20);
  AddObjectComponentBySlot(
      slot, itemptr->GetDisplayID(), itemptr->GetInventoryType(), forceAlternate, defer, sheathe, sheatheAttachmentSlot, showHidden
  );
  itemptr->UpdateEnchantments();
  CGWorldFrame *worldFrame = CGWorldFrame::GetActive();
  if (worldFrame && GetGUID() == ClntObjMgrGetActivePlayer()) {
    worldFrame->RefreshPlayerAlpha();
  }
}

void CGPlayer_C::SetTorsoAnimState(UINT newState) {
  UINT oldState = m_currentTorsoAnimState;
  CGUnit_C::SetTorsoAnimState(newState);
  if (oldState != newState && (m_handAnim[COMBAT_MAINHAND] != RESET_ANIMATION_INDICES0 || m_handAnim[COMBAT_OFFHAND] != RESET_ANIMATION_INDICES0)) {
    HandleSheatheAnimEvent(1, 1);
  }
}

const VirtualItemInfo *CGPlayer_C::GetVirtualItem(UINT slot, bool ignoreDisarmFlag) const {
  CGItem_C *item;

  switch (slot) {
    case VIRTUAL_MONSTER_SLOT_MAINHAND:
      item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(m_inventory.GetItem(INVSLOT_MAINHAND), __FILE__, __LINE__));
      if (!item || (item->m_itemInfo.m_classID == 2 && !ignoreDisarmFlag && (m_unit->flags & 0x200000))) {
        return 0;
      }
      break;

    case VIRTUAL_MONSTER_SLOT_OFFHAND:
      item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(m_inventory.GetItem(INVSLOT_OFFHAND), __FILE__, __LINE__));
      break;

    case VIRTUAL_MONSTER_SLOT_RANGED:
      item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(m_inventory.GetItem(INVSLOT_RANGED), __FILE__, __LINE__));
      break;

    default:
      return 0;
  }

  if (!item) {
    return 0;
  }

  return &item->m_itemInfo;
}

int CGPlayer_C::GetVirtualItemDisplayID(UINT slot) const {
  CGItem_C *item;

  switch (slot) {
    case VIRTUAL_MONSTER_SLOT_MAINHAND:
      item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(m_inventory.GetItem(INVSLOT_MAINHAND), __FILE__, __LINE__));
      if (!item || (item->m_itemInfo.m_classID == 2 && (m_unit->flags & 0x200000))) {
        return 0;
      }
      break;

    case VIRTUAL_MONSTER_SLOT_OFFHAND:
      item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(m_inventory.GetItem(INVSLOT_OFFHAND), __FILE__, __LINE__));
      break;

    case VIRTUAL_MONSTER_SLOT_RANGED:
      item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(m_inventory.GetItem(INVSLOT_RANGED), __FILE__, __LINE__));
      break;

    default:
      return 0;
  }

  if (!item) {
    return 0;
  }

  return item->GetDisplayID();
}

const VirtualItemInfo *CGPlayer_C::GetDefendingItem() const {
  const CGBag_C *inventory = GetBag();
  CGItem_C      *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(inventory->GetItem(4), __FILE__, __LINE__));
  if (item) {
    return &item->m_itemInfo;
  }
  return CGUnit_C::GetDefendingItem();
}

const CreatureModelDataRec *Player_C_GetModelName(UINT race, UINT sex) {
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

UINT Player_C_GetDisplayId(UINT race, UINT sex) {
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
  DWORDLONG item;
  BYTE      subcode;

  msg->Get(item);
  if (msgID == SMSG_READ_ITEM_OK) {
    CGItem_C *itemPtr = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(item, __FILE__, __LINE__));
    if (itemPtr) {
      itemPtr->SetTranslated();
      CGItemText::DisplayText(item, 1);
    }
    return;
  }

  msg->Get(subcode);
  switch (subcode) {
    case 0:
      CGItemText::DisplayText(item, 1);
      break;

    case 1: {
      CGItemText::DisplayText(item, 0);
      int delay;
      msg->Get(delay);
      FrameScript_SignalEvent(274, "%f", static_cast<float>(delay) * 0.001f);
      break;
    }

    default:
      if (item == CGItemText::GetItem()) {
        FrameScript_SignalEvent(276);
      }
      break;
  }
}

CGPlayer_C::~CGPlayer_C() {
  for (UINT component = 0; component < NUM_INVENTORY_SLOTS; ++component) {
    for (UINT attachment = 0; attachment < 36; ++attachment) {
      if (m_components[component][attachment]) {
        HandleClose(m_components[component][attachment]);
      }
    }
  }

  s_pendingItemExpirations.Clear();
  m_tradeSkillLines.Clear();
  for (UINT craftType = 0; craftType < 4; ++craftType) {
    m_craftSpells[craftType].Clear();
  }

  if (GetGUID() == ClntObjMgrGetActivePlayer()) {
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

    OnObjectDestruct(GetGUID());
    if (GetGUID() == ClntObjMgrGetActivePlayer()) {
      s_pendingItemExpirations.Clear();
      CGPlayer_C::SetActive(0);
    }
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

void CGPlayer_C::HandleMountResult(UINT result) {
  if (result < 11) {
    CGGameUI::DisplayError(s_mountResultGameErrors[result]);
  }
}

void CGPlayer_C::HandleDismountResult(UINT result) {
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

UINT CGPlayer_C::GetDefaultLanguage() {
  const ChrRacesRec *rec = g_chrRacesDB.GetRecord(GetRace());
  return rec ? rec->m_BaseLanguage : 0;
}

BOOL CGPlayer_C::GetLanguageSkill(UINT language, UINT &skill) {
  skill = 0;
  if (GetGUID() != ClntObjMgrGetActivePlayer()) {
    return 0;
  }

  int spellID = CGSpellBook::GetLanguageSpell(language);
  if (!spellID) {
    return 0;
  }

  const SkillLineAbilityRec *ability = SpellTableLookupAbility(m_unit->race, m_unit->classId, spellID);
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

BOOL Player_C_TogglePlayerRender() {
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

  UINT oldState = m_flags & 0x400;
  if (state) {
    m_flags |= 0x400;
    ResetCombatModeTimer(oldState == 0);
  } else {
    m_flags &= 0xFFFFFB3F;
    KillCombatModeTimer();
    if (m_combat.IsAttacking() || m_combat.AttackBeenSent()) {
      StopAttack();
    }
    Spell_C_CancelCombatSpell();
  }

  if (oldState == state) {
    return;
  }

  if (state && m_castingSpell) {
    const SpellRec *rec = g_spellDB.GetRecord(m_castingSpell);
    FATALASSERT(rec);
    if (rec->m_attributes & 2) {
      Spell_C_CancelSpell(1, 1, SPELL_FAILED_ERROR);
      SetWeaponMode(WEAPONMODE_NORMALMODE);
    } else if (rec->m_interruptFlags & 8) {
      Spell_C_CancelSpell(1, 1, SPELL_FAILED_ERROR);
    }
  }

  CGGameUI::PlayerCombatModeChanged(state);

  if (state) {
    CGTutorial::TriggerTutorial(TUTORIAL_COMBAT);
  } else {
    if (static_cast<float>(m_unit->health) / static_cast<float>(m_unit->maxHealth) < 0.5f) {
      CGTutorial::TriggerTutorial(TUTORIAL_FOOD);
    }
    if (!m_unit->displayPower) {
      if (static_cast<float>(m_unit->power[0]) / static_cast<float>(m_unit->maxPower[0]) < 0.5f) {
        CGTutorial::TriggerTutorial(TUTORIAL_DRINK);
      }
    }
  }

  bool hasLastWeaponMode = m_lastWeaponModeSent != -1 && m_lastWeaponModeSent != WEAPONMODE_NORMALMODE;
  if (state && (m_unit->weaponMode == WEAPONMODE_RANGEDMODE || m_unit->weaponMode == WEAPONMODE_SHEATHEDMODE || hasLastWeaponMode)) {
    ToggleSheathe(1);
  }

  if (!m_castingSpell) {
    UpdateBaseAnimation(0);
  }

  if (m_rangedStandTimer) {
    DetermineReadySequence(1);
    UpdateBaseAnimation(0);
    ClearRangedStandTimer();
  }

  if (state && m_unit->standState) {
    ChangeStandState(UNIT_STANDING);
  }
}

BOOL CGPlayer_C::OnAttackIconPressed() {
  FATALASSERT(GetGUID() == GetActive());
  if (!CGGameUI::m_hasControl) {
    return 0;
  }

  DWORDLONG target = CGGameUI::GetLockedTarget();
  if (m_flags & 0x400) {
    SetCombatMode(0);
    return 0;
  }

  if (m_unit->health <= 0) {
    CGGameUI::DisplayError(GERR_ATTACK_DEAD);
    return 0;
  }

  if (m_unit->flags & 0x20000) {
    CGGameUI::DisplayError(GERR_ATTACK_PACIFIED);
    return 0;
  }

  int useTarget = 0;
  if (target) {
    CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(target, __FILE__, __LINE__));
    useTarget = !unit || UnitReaction(unit) < UNIT_REACTION_AMIABLE;
  }
  if (!useTarget) {
    CGGameUI::TargetNearestEnemy(0);
    target = CGGameUI::GetLockedTarget();
  }
  if (!target) {
    CGGameUI::DisplayError(GERR_NO_ATTACK_TARGET);
    return 0;
  }

  FATALASSERT(g_combatModeMaxDistance);
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(target, __FILE__, __LINE__));
  if (!unit) {
    return 0;
  }

  if ((GetPosition() - unit->GetPosition()).SquaredMag() > g_combatModeMaxDistance->GetFloat() * g_combatModeMaxDistance->GetFloat()) {
    CGGameUI::DisplayError(GERR_OUT_OF_RANGE);
    return 0;
  }

  if (m_unit->health <= 0 || (m_unit->flags & 0x2000) || unit->m_unit->health <= 0 || !CanAttack(unit)) {
    CGGameUI::DisplayError(GERR_INVALID_ATTACK_TARGET);
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

static BOOL GuildIDUpdateHandler(DWORDLONG, UINT, UINT, LPCVOID, LPVOID);
static BOOL DuelTeamUpdateHandler(DWORDLONG, UINT, UINT, LPCVOID, LPVOID);
static BOOL        OnUpdateInventoryComponent(DWORDLONG, UINT, UINT, LPCVOID, LPVOID);
static BOOL OnUpdatePlayerFlags(DWORDLONG, UINT, UINT, LPCVOID, LPVOID);
static BOOL OnUpdateGuild(DWORDLONG, UINT, UINT, LPCVOID, LPVOID);

void CGPlayer_C::ResetCombatModeTimer(int newCombat) {
  if (!s_combatModeTimer) {
    UINT timeout = newCombat ? 0 : GetCombatModeTimerInterval();
    s_combatModeTimer = ClientSetTimer(timeout, PlayerCombatModeHandler, GetGUID(), 0);
  }
}

UINT CGPlayer_C::GetCombatModeTimerInterval() const {
  FATALASSERT(IsInCombatMode());
  return 500;
}

void CGPlayer_C::OnTaxiNodeStatus(CDataStore *msg) {
  DWORDLONG taxiGUID;
  BYTE      status;
  msg->Get(taxiGUID);
  msg->Get(status);

  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(taxiGUID, __FILE__, __LINE__));
  if (unit && (unit->m_unit->npcFlags & 4)) {
    if (status) {
      unit->UpdateInteractIcon(INTERACTICON_TAXINODE);
    } else {
      unit->UpdateInteractIcon(INTERACTICON_NONE);
    }
  }
}

void CGPlayer_C::ShowTaxiNodes(CDataStore *msg) {
  DWORDLONG unit = 0;
  LONGLONG  known;
  LONGLONG  flag;
  UINT      currentNode = 0;
  UINT      showWindow;

  msg->Get(showWindow);
  if (showWindow) {
    msg->Get(unit);
    msg->Get(currentNode);
  }
  msg->Get(flag);
  msg->Get(known);

  if (!flag) {
    if (showWindow) {
      CGGameUI::DisplayError(GERR_TAXINOPATHS);
    } else {
      ConsoleWrite("No taxi nodes for you!", DEFAULT_COLOR);
    }
  } else if (showWindow) {
    NTempest::CRect rect;
    if (TaxiMapUpdatePosition(currentNode, flag, known, rect)) {
      CGTaxiMap::SetupMap(unit, currentNode, flag, known, rect);
    }
  } else {
    for (UINT index = 0; index < 64; ++index) {
      if (flag & (static_cast<LONGLONG>(1) << index)) {
        const TaxiNodesRec *node = g_taxiNodesDB.GetRecord(index + 1);
        if (node) {
          ConsolePrintf("[%02d]: %s", node->m_ID, node->m_Name_lang[CURRENT_LANGUAGE]);
        }
      }
    }
  }
}

void CGPlayer_C::StartTaxi(DWORDLONG vendor, UINT startNode, UINT destNode) {
  CDataStore msg;
  msg.Put(CMSG_ACTIVATETAXI);
  msg.Put(vendor);
  msg.Put(startNode);
  msg.Put(destNode);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::HandleActivateTaxiReply(UINT code) {
  if (code < 12) {
    if (code) {
      CGGameUI::DisplayError(s_taxiErrors[code]);
    } else {
      CGTaxiMap::CloseMap();
    }
  }
}

DWORDLONG CGPlayer_C::GetLocalTarget() const {
  DWORDLONG lockedTarget = CGGameUI::GetLockedTarget();

  if (lockedTarget && lockedTarget == m_combat.IsAttacking()) {
    return lockedTarget;
  }

  return m_targetUnit;
}

BOOL CGPlayer_C::DeathBindDistanceCompare(const NTempest::C3Vector &bindStonePosition) {
  float              bindRadiusCheckSquared = MAX_BIND_DISTANCE * 1.5f;
  NTempest::C3Vector diff = s_bindPosition - bindStonePosition;
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

const NTempest::C3Vector &CGPlayer_C::GetBindPoint() {
  return s_bindPosition;
}

void CGPlayer_C::OnSpellFailed(const SpellRec *spellRec, UINT reason) {
  ClearTrackingTarget(0);
  if (spellRec && (spellRec->m_ID != static_cast<UINT>(m_castingSpell) || reason != SPELL_FAILED_SPELL_IN_PROGRESS) && (spellRec->m_attributes & 2)) {
    DetermineReadySequence(0);
    UpdateBaseAnimation(0);
  }
}

void CGPlayer_C::PlayMacroSound(int category) const {
  if (category < 12) {
    CDataStore msg;
    msg.Put(CMSG_PLAYER_MACRO);
    msg.Put(category);
    msg.Finalize();
    ClientServices_Send(&msg);
  }
}

void CGPlayer_C::PlayVocalMacro(int category) {
  CVar *masterSoundEffects = CVar::Lookup("MasterSoundEffects");
  bool  enabled = masterSoundEffects && masterSoundEffects->GetInt();
  CVar *enableGroupSpeech = CVar::Lookup("EnableGroupSpeech");
  if (enableGroupSpeech && enableGroupSpeech->GetInt() && enabled && category < 12) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      UNITAFFILIATION affiliation = player->GetGUIDAffiliation(GetGUID());
      RequestTalkEmote(TALKANIM_SHOUT);
      if ((1 << affiliation) & BROADCASTTO) {
        SoundInterfacePlayVocalMacro(this, category);
      }
    }
  }
}

static BOOL SoulStoneCompare(const CGItem_C *item, LPVOID) {
  const ItemStats_C *stats = g_itemDBCache.GetRecord(item->GetEntryID(), 0, 0, 0);
  if (!stats) {
    return 0;
  }

  for (UINT spellIndex = 0; spellIndex < 5; ++spellIndex) {
    const SpellRec *spell = g_spellDB.GetRecord(stats->m_spellID[spellIndex]);
    if (spell && !stats->m_spellTrigger[spellIndex]) {
      for (UINT effectIndex = 0; effectIndex < 3; ++effectIndex) {
        if (spell->m_effect[effectIndex] == 18 && (spell->m_implicitTargetA[effectIndex] == 1 || spell->m_implicitTargetB[effectIndex] == 1)) {
          return 1;
        }
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

void CGPlayer_C::SaveDeathMessage(DWORDLONG guid) {
  m_lastKillerGUID = guid;
  CheckKillerFeedback();
}

void CGPlayer_C::CheckKillerFeedback() {
  if (!m_unit->health && !m_deathHolds && m_lastKillerGUID) {
    CGUnit_C *killer = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(m_lastKillerGUID, __FILE__, __LINE__));
    if (killer) {
      CGGameUI::DisplayError(GERR_KILLED_BY_S, killer->GetUnitName());
    }
    m_lastKillerGUID = 0;
  }
}

bool CGPlayer_C::GetPackAndSlot(CGItem_C *item, BYTE &packSlot, BYTE &slot) {
  DWORDLONG containerGUID = item->m_item->m_containedIn;
  if (containerGUID == GetGUID()) {
    packSlot = 0xFF;
    slot = FindSlotIndex(item->GetGUID());
    return slot != 0xFF;
  }

  packSlot = FindSlotIndex(containerGUID);
  if (packSlot == 0xFF) {
    return false;
  }
  CGObject_C *containerObject = ClntObjMgrObjectPtr(containerGUID, __FILE__, __LINE__);
  CGBag      *bag = containerObject ? containerObject->GetBag() : 0;
  if (!bag) {
    return false;
  }
  for (UINT index = 0; index < bag->NumSlots(); ++index) {
    if (bag->GetItem(index) == item->GetGUID()) {
      slot = static_cast<BYTE>(index);
      return true;
    }
  }
  return false;
}

void CGPlayer_C::OpenLootItem(CGItem_C *item) {
  BYTE packSlot;
  BYTE slot;
  if (!GetPackAndSlot(item, packSlot, slot)) {
    return;
  }
  m_lootingUnitSent = item->GetGUID();
  CDataStore msg;
  msg.Put(CMSG_OPEN_ITEM);
  msg.Put(packSlot);
  msg.Put(slot);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::OpenWrappedItem(CGItem_C *item) {
  BYTE packSlot;
  BYTE slot;
  if (!GetPackAndSlot(item, packSlot, slot)) {
    return;
  }
  SndInterfacePlayInterfaceSound("UnwrapGift");
  CDataStore msg;
  msg.Put(CMSG_OPEN_ITEM);
  msg.Put(packSlot);
  msg.Put(slot);
  msg.Finalize();
  ClientServices_Send(&msg);
}

BOOL CGPlayer_C::InviteToGroup(DWORDLONG target) {
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(target, __FILE__, __LINE__));
  if (!unit) {
    return 0;
  }
  InviteToGroup(unit->GetUnitName());
  return 1;
}

void CGPlayer_C::InviteToGroup(LPCSTR target) {
  CDataStore msg;
  msg.Put(CMSG_GROUP_INVITE);
  msg.PutString(target);
  msg.Finalize();
  ClientServices_Send(&msg);
}

int CGPlayer_C::Uninvite(DWORDLONG target) {
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(target, __FILE__, __LINE__));
  if (!unit) {
    CDataStore msg;
    msg.Put(CMSG_GROUP_UNINVITE_GUID);
    msg.Put(target);
    msg.Finalize();
    ClientServices_Send(&msg);
  } else {
    Uninvite(unit->GetUnitName());
  }
  return 1;
}

void CGPlayer_C::Uninvite(LPCSTR target) {
  CDataStore msg;
  msg.Put(CMSG_GROUP_UNINVITE);
  msg.PutString(target);
  msg.Finalize();
  ClientServices_Send(&msg);
}

BOOL CGPlayer_C::SetNewLeader(DWORDLONG target) {
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(target, __FILE__, __LINE__));
  LPCSTR name;
  if (unit) {
    name = unit->GetUnitName();
  } else {
    const NameCache *nameEntry = g_nameDBCache.GetRecord(target, 0, 0, 0);
    if (!nameEntry) {
      return 0;
    }
    name = nameEntry->m_name;
  }
  SetNewLeader(name);
  return 1;
}

void CGPlayer_C::SetNewLeader(LPCSTR target) {
  CDataStore msg;
  msg.Put(CMSG_GROUP_SET_LEADER);
  msg.PutString(target);
  msg.Finalize();
  ClientServices_Send(&msg);
}

BOOL CGPlayer_C::ReportBagItemSubtypeMismatch(BYTE bagSlot) const {
  if (bagSlot == 0xFF) {
    return 0;
  }

  DWORDLONG itemGUID = m_inventory.GetItem(bagSlot);
  CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(itemGUID, __FILE__, __LINE__));
  if (!item || item->GetClassID() != 11) {
    return 0;
  }

  const ItemSubClassRec *subclass = SDBItemSubclassGetSubClassRec(6, item->GetSubtypeID());
  if (!subclass) {
    return 0;
  }

  CGGameUI::DisplayError(GERR_WRONG_BAG_TYPE_SUBCLASS, subclass->m_displayName_lang[CURRENT_LANGUAGE]);
  return 1;
}

BOOL CGPlayer_C::OnSplitMoneyNotify(CDataStore *msg) {
  char      string[128];
  char      buf[128];
  char      totalMoneyBuf[64];
  char      shareMoneyBuf[64];
  char      coinName[32];
  int       totalcoins[3];
  int       sharecoins[3];
  UINT      share;
  DWORDLONG player;
  UINT      total;

  msg->Get(player);
  msg->Get(total);
  msg->Get(share);
  if (total && share) {
    CurrencyBreakdown(total, totalcoins);
    CurrencyBreakdown(share, sharecoins);

    int first = 1;
    int coin;
    for (coin = 2; coin >= 0; --coin) {
      if (totalcoins[coin]) {
        LPCSTR text = FrameScript_GetText(CurrencyAbbreviation(coin), -1, GENDER_NOT_APPLICABLE);
        SStrCopy(coinName, text, sizeof(coinName));
        if (first) {
          SStrPrintf(totalMoneyBuf, sizeof(totalMoneyBuf), "%d %s", totalcoins[coin], coinName);
          first = 0;
        } else {
          char coinBuf[64];
          SStrPrintf(coinBuf, sizeof(coinBuf), ", %d %s", totalcoins[coin], coinName);
          SStrPack(totalMoneyBuf, coinBuf, sizeof(totalMoneyBuf));
        }
      }
    }

    first = 1;
    for (coin = 2; coin >= 0; --coin) {
      if (sharecoins[coin]) {
        LPCSTR text = FrameScript_GetText(CurrencyAbbreviation(coin), -1, GENDER_NOT_APPLICABLE);
        SStrCopy(coinName, text, sizeof(coinName));
        if (first) {
          SStrPrintf(shareMoneyBuf, sizeof(shareMoneyBuf), "%d %s", sharecoins[coin], coinName);
          first = 0;
        } else {
          char coinBuf[64];
          SStrPrintf(coinBuf, sizeof(coinBuf), ", %d %s", sharecoins[coin], coinName);
          SStrPack(shareMoneyBuf, coinBuf, sizeof(shareMoneyBuf));
        }
      }
    }

    const NameCache *name = g_nameDBCache.GetRecord(player, player, 0, 0);
    if (name) {
      if (player == ClntObjMgrGetActivePlayer()) {
        LPCSTR text = FrameScript_GetText("SPLIT_MONEY_SPLIT_SELF", -1, GENDER_NOT_APPLICABLE);
        SStrCopy(buf, text, sizeof(buf));
        SStrPrintf(string, sizeof(string), buf, totalMoneyBuf);
      } else {
        LPCSTR text = FrameScript_GetText("SPLIT_MONEY_SPLIT", -1, GENDER_NOT_APPLICABLE);
        SStrCopy(buf, text, sizeof(buf));
        SStrPrintf(string, sizeof(string), buf, name->m_name, totalMoneyBuf, shareMoneyBuf);
      }
      CGChat::AddChatMessage(string, SLASH_CMD_SYSTEM, 0, 0, 0, 0, 0);
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

void CGPlayer_C::GiftWrap(CGItem_C *item) {
  CGItem_C *wrapper = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(s_giftWrapItem, __FILE__, __LINE__));
  if (wrapper) {
    BYTE wrapperItemContainerSlot = FindSlotIndex(wrapper->m_item->m_containedIn);
    BYTE wrapperItemSlot = FindItemSlot(wrapper->m_item->m_containedIn, wrapper);
    BYTE itemContainerSlot = FindSlotIndex(item->m_item->m_containedIn);
    BYTE itemSlot = FindItemSlot(item->m_item->m_containedIn, item);

    CDataStore msg;
    msg.Put(CMSG_WRAP_ITEM);
    msg.Put(wrapperItemContainerSlot);
    msg.Put(wrapperItemSlot);
    msg.Put(itemContainerSlot);
    msg.Put(itemSlot);
    msg.Finalize();
    ClientServices_Send(&msg);
    CancelGiftWrap();
  }
}

void CGPlayer_C::SheatheWeapon(bool sheathe) {
  if (m_inventory.GetItem(INVSLOT_MAINHAND) || m_inventory.GetItem(INVSLOT_OFFHAND)) {
    if (sheathe && (m_flags & 0x400)) {
      SetCombatMode(0);
    }

    if (GetGUID() == ClntObjMgrGetActivePlayer()) {
      CDataStore msg;
      msg.Put(CMSG_SHEATHE);
      msg.Put(sheathe);
      msg.Finalize();
      ClientServices_Send(&msg);
    }

    if (g_standStateAllowsSheathing[m_unit->standState]) {
      MaybeStartSheatheAnim();
    }
  }
}

void CGPlayer_C::BotMove(DWORD now, NTempest::C3Vector *points, int count, DWORD duration, UINT flags) {
  static_cast<CMovement &>(m_move).OnSpline(now, points, count, duration, flags);
  OnSetRunModeLocal(now, 1);
  OnMoveStartLocal(now, 1);
}

int CGPlayer_C::BotSpline() {
  return static_cast<CMovement &>(m_move).IsSpline();
}

void CGPlayer_C::StartSheatheAnim(INVENTORY_SLOTS slot, int hip, int both) {
  FATALASSERT((slot == INVSLOT_MAINHAND) || (slot == INVSLOT_OFFHAND));

  if (SheatheAnimPlaying()) {
    if (!((m_animFlags >> 16) & 1)) {
      return;
    }
    HandleSheatheAnimEvent(1, 1);
    return;
  }

  if (both || slot == INVSLOT_MAINHAND) {
    m_handAnim[COMBAT_MAINHAND] = hip ? ANIM_HIPSHEATHE : ANIM_SHEATHE;
  }
  if (both || slot == INVSLOT_OFFHAND) {
    m_handAnim[COMBAT_OFFHAND] = hip ? ANIM_HIPSHEATHE : ANIM_SHEATHE;
  }

  if (!SetSheathingSequence()) {
    HandleSheatheAnimEvent(1, 1);
  }
}

void CGPlayer_C::SetLastWeaponModeSent(int mode) {
  if (ClntObjMgrGetActivePlayer() == GetGUID()) {
    m_lastWeaponModeSent = mode;
  }
}

void CGPlayer_C::SetFarSightFocus(CGObject_C *obj) {
  if (!(m_flags & 0x800)) {
    ToggleFarSight();
  }
}

void CGPlayer_C::ToggleFarSight() {
  CGObject_C *focus = ClntObjMgrObjectPtr(GetFarsightFocus(), __FILE__, __LINE__);
  if (focus && CGWorldFrame::GetActiveCamera()->GetTarget() != focus->GetGUID()) {
    SetCombatMode(0);
    if (focus->IsA(ID_UNIT) && (static_cast<CGUnit_C *>(focus)->m_unit->flags & 0x01000000) &&
        GetGUID() == (static_cast<CGUnit_C *>(focus)->m_unit->charmedBy ? static_cast<CGUnit_C *>(focus)->m_unit->charmedBy
                                                                         : static_cast<CGUnit_C *>(focus)->m_unit->createdBy))
    {
      CGUnit_C::SetActiveMover(focus->GetGUID());
    } else {
      CGUnit_C::SetActiveMover(0);
    }
    m_flags |= 0x800;
    CGWorldFrame::GetActive()->SetCameraTarget(focus);
  } else {
    m_flags &= ~0x800;
    CGGameUI::ResetCamera();
    CGUnit_C::SetActiveMover(GetGUID());
  }
}

void CGPlayer_C::ClearFarSight() {
  if (m_flags & 0x800) {
    ToggleFarSight();
  }
}

CGUnit_C *CGPlayer_C::GetPossessedUnit() {
  if (m_flags & 0x800) {
    CGObject_C *focus = ClntObjMgrObjectPtr(GetFarsightFocus(), __FILE__, __LINE__);
    if (focus && focus->IsA(ID_UNIT) && static_cast<CGUnit_C *>(focus)->IsPossessedBy(GetGUID())) {
      return static_cast<CGUnit_C *>(focus);
    }
  }
  return 0;
}

BOOL CGPlayer_C::OnPetitionShowList(CDataStore *msg) {
  DWORDLONG petitionNpcGUID;
  BYTE      count = 0;
  msg->Get(petitionNpcGUID);
  msg->Get(count);

  memset(petitionList, 0, sizeof(petitionList));
  for (UINT index = 0; index < count; ++index) {
    msg->Get(petitionList[index].m_muid);
    msg->Get(petitionList[index].m_itemID);
    msg->Get(petitionList[index].m_itemDisplayID);
    msg->Get(petitionList[index].m_price);
    msg->Get(petitionList[index].m_flags);
  }

  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(petitionNpcGUID, __FILE__, __LINE__));
  if (unit && (unit->m_unit->npcFlags & 0x80) && (unit->m_unit->npcFlags & 0x40) && (petitionList[0].m_flags & 1)) {
    CGGuildRegistrar::SetRegistrar(petitionNpcGUID, petitionList);
  }
  return 1;
}

void CGPlayer_C::BuyPetition(const DWORDLONG &petitionUnit, CGPetition *petition) {
  CDataStore msg;
  msg.Put(CMSG_PETITION_BUY);
  msg.Put(petitionUnit);
  petition->Pack(&msg);
  msg.Finalize();
  ClientServices_Send(&msg);
}

BOOL CGPlayer_C::OnPetitionShowSignatures(CDataStore *msg) {
  DWORDLONG itemGUID;
  DWORDLONG ownerGUID;
  int       petitionID;
  BYTE      count;
  BYTE      i;

  msg->Get(itemGUID);
  msg->Get(ownerGUID);
  msg->Get(petitionID);
  msg->Get(count);

  TSStackArray<DWORDLONG> signers(_alloca(sizeof(DWORDLONG) * count), count, count);
  TSStackArray<int>       choices(_alloca(sizeof(int) * count), count, count);
  for (i = 0; i < count; ++i) {
    msg->Get(signers[i]);
    msg->Get(choices[i]);
  }

  if (!g_friendList->IsIgnored(ownerGUID)) {
    CGPetitionInfo::SetPetition(itemGUID, petitionID);
    CGPetitionInfo::SetSignatures(count, signers.Ptr(), choices.Ptr());
  }
  return 1;
}

void CGPlayer_C::RequestPetitionSignatures(DWORDLONG item) {
  if (!item) {
    return;
  }
  CDataStore msg;
  msg.Put(CMSG_PETITION_SHOW_SIGNATURES);
  msg.Put(item);
  msg.Finalize();
  ClientServices_Send(&msg);
}

BOOL CGPlayer_C::OnSignedResults(CDataStore *msg) {
  PETITION_ERROR results;
  msg->Get(*reinterpret_cast<int *>(&results));
  if (results == PETITION_SUCCESS) {
    CGGameUI::DisplayError(GERR_PETITION_SIGNED);
    FrameScript_SignalEvent(374);
  } else if (results == PETITION_ALREADY_SIGNED) {
    CGGameUI::DisplayError(GERR_PETITION_ALREADY_SIGNED);
  } else if (results == PETITION_ALREADY_IN_GUILD) {
    CGGameUI::DisplayError(GERR_PETITION_IN_GUILD);
  } else if (results == PETITION_CHARTER_CREATOR) {
    CGGameUI::DisplayError(GERR_PETITION_CREATOR);
  } else {
    ConsoleWrite("Petition error", DEFAULT_COLOR);
  }
  return 1;
}

BOOL CGPlayer_C::OnTurnInPetitionResults(CDataStore *msg) {
  PETITION_ERROR results;
  msg->Get(*reinterpret_cast<int *>(&results));
  if (results == PETITION_SUCCESS) {
    FrameScript_SignalEvent(362);
  } else if (results == PETITION_ALREADY_IN_GUILD) {
    CGGameUI::DisplayError(GERR_PETITION_IN_GUILD);
  } else if (results == PETITION_NOT_ENOUGH_SIGNATURES) {
    CGGameUI::DisplayError(GERR_PETITION_NOT_ENOUGH_SIGNATURES);
  } else {
    ConsoleWrite("Petition error", DEFAULT_COLOR);
  }
  return 1;
}

static void GuildCharterTurnInCallback(int, const DWORDLONG &, LPVOID, bool granted) {
  if (granted) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      player->TurnInGuildCharter();
    }
  }
}

void CGPlayer_C::TurnInGuildCharter() {
  CGItem_C *item = 0;
  int       found = 0;
  UINT      j;

  for (UINT index = 23; index <= 38; ++index) {
    DWORDLONG guid = m_inventory.GetItem(index);
    if (guid) {
      item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
      if (item && item->GetItemStaticFlag(ITEM_FLAG_PETITION)) {
        const CGPetition *petition = g_petitionCache.GetRecord(item->GetPetitionID(), guid, GuildCharterTurnInCallback, 0);
        if (!petition) {
          return;
        }
        if (petition->m_flags & 1) {
          goto turn_in;
        }
      }
    }
  }

  for (index = 19; index <= 22; ++index) {
    CGObject_C *container = ClntObjMgrObjectPtr(m_inventory.GetItem(index), __FILE__, __LINE__);
    if (container) {
      for (j = 0; j < container->GetBag()->NumSlots(); ++j) {
        DWORDLONG guid = container->GetBag()->GetItem(index);
        if (guid) {
          item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
          if (item && item->GetItemStaticFlag(ITEM_FLAG_PETITION)) {
            const CGPetition *petition = g_petitionCache.GetRecord(item->GetPetitionID(), guid, GuildCharterTurnInCallback, 0);
            if (!petition) {
              return;
            }
            if (petition->m_flags & 1) {
              found = 1;
              break;
            }
          }
        }
      }
    }
  }

  if (!found) {
    CGGameUI::DisplayError(GERR_NO_GUILD_CHARTER);
    return;
  }

turn_in:
  CDataStore msg;
  msg.Put(CMSG_TURN_IN_PETITION);
  msg.Put(item->GetGUID());
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::SendTextEmote(const EmotesTextRec *rec, const DWORDLONG &target) const {
  if (!rec) {
    return;
  }

  const EmotesRec *emote = g_emotesDB.GetRecord(rec->m_emoteID);
  if (emote && (emote->m_EmoteFlags & 0x4000) && ((m_move.GetMoveFlags() & 0x4000) || (m_move.GetMoveFlags() & 0x33))) {
    CGGameUI::DisplayError(GERR_NOEMOTEWHILERUNNING);
    return;
  }

  CDataStore msg;
  msg.Put(CMSG_TEXT_EMOTE);
  msg.Put(rec->m_ID);
  msg.Put(target);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPlayer_C::AddDeferredDamage(int normal, UINT flags, UINT damage, DWORDLONG victim) {
  DEFERREDDAMAGE *deferred = s_freeDeferedDamage.Get(0);
  deferred->Set(normal, flags, damage, victim);
  s_deferredDamage.LinkNode(deferred, LIST_HEAD, 0);
}

void CGPlayer_C::AddDeferredSpellMiss(DWORDLONG victim, MISS_REASON reason, int spellID) {
  DEFERREDSPELLMISS *deferred = s_freeDeferredSpellMiss.Get(0);
  deferred->Set(victim, reason, spellID);
  s_deferredSpellMiss.LinkNode(deferred, LIST_HEAD, 0);
}

void CGPlayer_C::ProcessDeferredDamage() {
  DEFERREDDAMAGE *next;
  for (DEFERREDDAMAGE *deferred = s_deferredDamage.Head(); (int)deferred > 0; deferred = next) {
    next = deferred->RawNext();
    CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(deferred->victim, __FILE__, __LINE__));
    if (unit) {
      if (deferred->flags & 2) {
        unit->AddWorldText(WORLDTEXTMISS_ABSORBED);
      } else if (deferred->damage && (deferred->flags & 1)) {
        unit->AddWorldCritText(deferred->damage, deferred->normal);
      } else if (deferred->damage) {
        unit->AddWorldDamageText(deferred->damage, deferred->normal);
      }
      deferred->Unlink();
      s_freeDeferedDamage.Put(deferred);
    }
  }
}

void CGPlayer_C::ProcessDeferredSpellMiss() {
  DEFERREDSPELLMISS *next;
  for (DEFERREDSPELLMISS *deferred = s_deferredSpellMiss.Head(); (int)deferred > 0; deferred = next) {
    next = deferred->RawNext();
    CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(deferred->victim, __FILE__, __LINE__));
    if (unit) {
      unit->AddWorldText(deferred->reason);
      UnitCombatLogSpellMissed(deferred->reason, deferred->spellID, ClntObjMgrGetActivePlayer(), deferred->victim);
      deferred->Unlink();
      s_freeDeferredSpellMiss.Put(deferred);
    }
  }
}

void CGPlayer_C::OnLootGameObject(const DWORDLONG &gameObject, bool lootAnim) {
  m_lootingUnitSent = gameObject;
  if (lootAnim && m_currentTorsoAnimState != ANIM_STATE_SPELLPRECAST) {
    UpdateBaseAnimation(ANIM_STATE_LOOTBEGIN, 0);
  }
}
void CGPlayer_C::GetAFKText(char *buffer, int size) const {
  if (m_plyr->playerFlags & 4) {
    LPCSTR text = FrameScript_GetText("CHAT_FLAG_AFK", -1, GENDER_NOT_APPLICABLE);
    SStrCopy(buffer, text, size);
  } else {
    buffer[0] = '\0';
  }
}

void CGPlayer_C::GetDNDText(char *buffer, int size) const {
  if (m_plyr->playerFlags & 8) {
    LPCSTR text = FrameScript_GetText("CHAT_FLAG_DND", -1, GENDER_NOT_APPLICABLE);
    SStrCopy(buffer, text, size);
  } else {
    buffer[0] = '\0';
  }
}

void CGPlayer_C::GetGMText(char *buffer, int size) const {
  if (m_plyr->playerFlags & 0x10) {
    LPCSTR text = FrameScript_GetText("CHAT_FLAG_GM", -1, GENDER_NOT_APPLICABLE);
    SStrCopy(buffer, text, size);
  } else {
    buffer[0] = '\0';
  }
}

void CGPlayer_C::GuildInfoLoaded(const TSGrowableArray<UINT> &guildList) {
  if (!m_plyr->guildID) {
    return;
  }

  for (UINT index = guildList.Count(); index;) {
    if (guildList[--index] == m_plyr->guildID) {
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

UINT CGPlayer_C::UpdateUnitNameString(UINT localPlayerFlags, UINT otherUnitsFlags, char *buffer, UINT bufferSize) const {
  if (ClntObjMgrGetActivePlayer() == GetGUID()) {
    return CGUnit_C::UpdateUnitNameString(localPlayerFlags, localPlayerFlags, buffer, bufferSize);
  }
  return CGUnit_C::UpdateUnitNameString(otherUnitsFlags, otherUnitsFlags, buffer, bufferSize);
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
  modifier = 0;
  base = 0;

  const SkillLineAbilityRec *ability = SpellTableLookupAbility(m_unit->race, m_unit->classId, s_defenseSkillID);
  return ability && ability->m_spell == static_cast<int>(s_defenseSkillID) && GetExpandedSkillRank(ability->m_skillLine, base, modifier);
}

bool CGPlayer_C::GetAttackSkillRank(int hand, int &base, int &modifier) const {
  modifier = 0;
  base = 0;

  int                        weaponSpell = GetWeaponSpell(static_cast<COMBATHAND>(hand));
  const SkillLineAbilityRec *ability = SpellTableLookupAbility(m_unit->race, m_unit->classId, weaponSpell);
  return ability && ability->m_spell == weaponSpell && GetExpandedSkillRank(ability->m_skillLine, base, modifier);
}

void CGPlayer_C::CombatLoggingFlagChanged() {
  UnitDebugCombatLogOnEnable(m_unit->flags & 0x2000000);
}

void CGPlayer_C::PlayerFlagsChanged(BYTE oldFlags) {
  if (((oldFlags ^ m_plyr->playerFlags) & 0x10) == 0) {
    return;
  }

  TriggerPlayerNameUpdate();
  if (GetGUID() == ClntObjMgrGetActivePlayer()) {
    if (m_plyr->playerFlags & 0x10) {
      ConsolePrintf("Showing GM label to users");
    } else {
      ConsolePrintf("Not showing GM label to users");
    }
  }
}

void CGPlayer_C::OnDeath() {
  CGUnit_C::OnDeath();
  if (GetGUID() != ClntObjMgrGetActivePlayer()) {
    if (CGGameUI::IsPartyMember(GetGUID())) {
      CGGameUI::DisplayError(GERR_PLAYER_DIED_S, GetUnitName());
    }
  } else {
    CGGameUI::Target(0, 0);
    CGGameUI::ClearInteractTarget(CGGameUI::GetInteractTarget());
    CGGameUI::CloseLoot(1, 0);
    CGPlayer_C::UpdateQuestStatusAll();
    if (IsOnTaxi()) {
      HandleRepopRequest();
    }
  }
}

void CGPlayer_C::OnDeathAnimate() {
  CGUnit_C::OnDeathAnimate();
  CheckKillerFeedback();
  DWORDLONG guid = GetGUID();
  if (guid == ClntObjMgrGetActivePlayer() && !(m_move.m_moveFlags & 0x4000)) {
    CGGameUI::UpdateActivePlayer();
  }
}

void CGPlayer_C::OnUnitDeath(DWORDLONG) {
}

void CGPlayer_C::OnAttackStart(DWORDLONG victim) {
  CGUnit_C::OnAttackStart(victim);
  m_flags = (m_flags & ~0x30u) | 0x10;

  if (GetGUID() == ClntObjMgrGetActivePlayer() && ClntObjMgrGetPlayerType() != PLAYER_BOT) {
    if (s_attackBreakTimer) {
      ClientKillTimer(s_attackBreakTimer, reinterpret_cast<CLIENTTIMERHANDLER>(PlayerAttackBreakHandler), "PlayerAttackBreakHandler");
    }
    s_attackBreakTimer = ClientSetTimer(500, PlayerAttackBreakHandler, GetGUID(), ClntObjMgrGetCurrent());
  }
}

void CGPlayer_C::OnAttackStop(DWORDLONG previousTarget, int nowDead) {
  m_flags &= ~0x30u;
  if (GetGUID() == ClntObjMgrGetActivePlayer() && ClntObjMgrGetPlayerType() != PLAYER_BOT) {
    if (s_attackBreakTimer) {
      ClientKillTimer(s_attackBreakTimer, reinterpret_cast<CLIENTTIMERHANDLER>(PlayerAttackBreakHandler), "PlayerAttackBreakHandler");
    }
    s_attackBreakTimer = 0;
  }
  CGUnit_C::OnAttackStop(previousTarget, nowDead);
}

void CGPlayer_C::OnBadAttackFacing(DWORDLONG victim) {
  if (GetGUID() != ClntObjMgrGetActivePlayer()) {
    CGUnit_C::OnBadAttackFacing(victim);
    return;
  }

  if (!(m_flags & 0x40)) {
    char buf[128];
    LPCSTR text = FrameScript_GetText("ERR_WRONG_DIRECTION_FOR_ATTACK", -1, GENDER_NOT_APPLICABLE);
    SStrCopy(buf, text, sizeof(buf));
    CGGameUI::DisplayError(GERR_BADATTACKFACING);
    m_flags |= 0x40;
  }
}

void CGPlayer_C::OnBadAttackPosition(DWORDLONG victim, float range) {
  if (GetGUID() != ClntObjMgrGetActivePlayer()) {
    CGUnit_C::OnBadAttackPosition(victim, range);
    return;
  }

  if (!(m_flags & 0x80)) {
    char buf[128];
    LPCSTR text = FrameScript_GetText("ERR_TOO_FAR_TO_ATTACK", -1, GENDER_NOT_APPLICABLE);
    SStrCopy(buf, text, sizeof(buf));
    CGGameUI::DisplayError(GERR_BADATTACKPOS);
    m_flags |= 0x80;
  }
}

DWORDLONG CGPlayer_C::GetUnitBeingLooted() const {
  return m_lootingUnit;
}

void CGPlayer_C::OnBadAttackTarget(DWORDLONG) {
  SetCombatMode(0);
}

void CGPlayer_C::OnNotStanding(DWORDLONG) {
  CGGameUI::DisplayError(GERR_CANTATTACK_NOTSTANDING);
}

void CGPlayer_C::UnitHit(VICTIMSTATES state, DWORDLONG attacker) {
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
  bool allowed;
  return IsOnTaxi() || !CWorld::QueryMountAllowed(m_worldObject, allowed) || allowed;
}

void CGPlayer_C::ChangeStandState(UINT standState) {
  if (GetGUID() == ClntObjMgrGetActivePlayer()) {
    CGUnit_C::ChangeStandState(standState);
  }
}

void CGPlayer_C::OnStandStateChanged(UINT, UINT newState) {
  if (GetGUID() != ClntObjMgrGetActivePlayer()) {
    return;
  }

  if (!newState) {
    FrameScript_SignalEvent(259);
  } else {
    FrameScript_SignalEvent(260);
    if (IsLooting()) {
      CGGameUI::CloseLoot(1, 1);
    }
    if (m_flags & 0x400) {
      SetCombatMode(0);
    }
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

  if (GetGUID() == ClntObjMgrGetActivePlayer()) {
    CGTradeSkillInfo::RefreshList(0);
    CGActionBar::UpdateItem(itemPtr->GetEntryID());
    CGTradeInfo::UpdatePlayerItem(itemPtr->GetGUID());
    CGContainerInfo::UpdateItem(itemPtr->GetGUID());
    CGCharacterInfo::UpdateItem(itemPtr->GetGUID());
  }

  const ItemDisplayInfoRec *displayInfo = g_itemDisplayInfoDB.GetRecord(itemPtr->GetDisplayID());
  if (!displayInfo) {
    return;
  }

  int invSlot = FindSlotIndex(itemPtr->GetGUID());
  if (invSlot < 0 || invSlot >= 69) {
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

  const ItemVisualsRec *visual = g_itemVisualsDB.GetRecord(displayInfo->m_itemVisual);
  if (visual) {
    return;
  }

  for (int i = 0; i < num; ++i) {
    if (enchantments[i].id) {
      const SpellItemEnchantmentRec *enchantment = g_spellItemEnchantmentDB.GetRecord(enchantments[i].id);
      if (enchantment && enchantment->m_itemVisual) {
        visual = g_itemVisualsDB.GetRecord(enchantment->m_itemVisual);
      }
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
  if (!info || (info->displayInfo->m_itemVisual && !force) || (rec && info->enchantmentVisual && info->enchantmentVisual->m_ID == rec->m_ID)) {
    return;
  }

  ClearItemVisuals(info);
  if (!rec) {
    return;
  }

  if (!info->displayInfo->m_itemVisual) {
    info->enchantmentVisual = const_cast<ItemVisualsRec *>(rec);
  }

  ATTACHMENTMODELINFO *modelInfo = info->modelInfo;
  for (int modelIndex = 0; modelIndex < 2; ++modelIndex) {
    int    currentLink = modelInfo->currentLink;
    HMODEL child = 0;
    if (currentLink >= 0) {
      child = ComponentUtilGetChildModel(m_paperDollModel, currentLink);
    }

    for (UINT visualIndex = 0; visualIndex < 5; ++visualIndex) {
      if (!modelInfo->model) {
        continue;
      }

      int visualID = rec->m_Slot[visualIndex];
      if (!visualID) {
        continue;
      }

      const ItemVisualEffectsRec *effect = g_itemVisualEffectsDB.GetRecord(visualID);
      if (!effect) {
        continue;
      }

      ComponentUtilAddItemVisual(modelInfo->model, visualIndex, effect->m_Model);
      if (currentLink >= 0) {
        if (child) {
          ComponentUtilAddItemVisual(child, visualIndex, effect->m_Model);
        }
      }
    }

    if (child) {
      HandleClose(child);
    }
    ++modelInfo;
  }
}

void CGPlayer_C::IncrementPendingItemStats() {
  ++m_pendingItemStats;
}

void CGPlayer_C::DecrementPendingItemStats() {
  if (--m_pendingItemStats <= 0) {
    m_pendingItemStats = 0;
    if (m_geosetHandle) {
      CharCustomizationCommitItemGeosets(m_geosetHandle, 1);
      CharCustomizationCommitGeosets(m_geosetHandle);
    }
  }

  if (GetGUID() == ClntObjMgrGetActivePlayer()) {
    CGGameUI::UpdateActivePlayer();
    FrameScript_SignalEvent(183, "%s", "player");
  }
}

void CGPlayer_C::FixComponenting(CGItem_C *item) {
  for (int slot = 0; slot < NUM_INVENTORY_SLOTS; ++slot) {
    if (m_inventory.GetItem(slot) != item->GetGUID() || !((1 << slot) & 0x783FD)) {
      continue;
    }

    if (slot == INVSLOT_RANGED) {
      AttachObjComponent(item->GetGUID(), INVSLOT_RANGED, 0, 0, -1);
      SetSheatheReason(SHEATHE_RANGED, m_unit->weaponMode == WEAPONMODE_RANGEDMODE, 1);
      continue;
    }

    if (!IsSlotComponented(slot, 1)) {
      continue;
    }

    int linkPoint = -1;
    if ((1 << slot) & 0x18000) {
      linkPoint = SheatheTypeToSheathePoint(item->GetSheatheType(), slot);
    }

    AttachObjComponent(item->GetGUID(), slot, 0, m_unit->weaponMode == WEAPONMODE_SHEATHEDMODE, linkPoint);
    AddComponent(item->GetDisplayID(), item->GetInventoryType(), slot, 0);

    const ItemDisplayInfoRec *displayInfo = g_itemDisplayInfoDB.GetRecord(item->GetDisplayID());
    if (displayInfo) {
      CharCustomizationAddItemGeosets(m_geosetHandle, displayInfo, item->GetInventoryType(), m_texComponent, m_unit->race, 1);
    }
  }

  DecrementPendingItemStats();
}

void CGPlayer_C::ItemReceived(const ItemStats *stats) const {
  if (!stats || GetGUID() != ClntObjMgrGetActivePlayer()) {
    return;
  }

  for (UINT spellIndex = 0; spellIndex < 5; ++spellIndex) {
    if (!stats->m_spellID[spellIndex]) {
      continue;
    }
    const SpellRec *spell = g_spellDB.GetRecord(stats->m_spellID[spellIndex]);
    if (!spell || stats->m_spellTrigger[spellIndex]) {
      continue;
    }

    for (UINT effectIndex = 0; effectIndex < 3; ++effectIndex) {
      if (spell->m_effect[effectIndex] == 18 && (spell->m_implicitTargetA[effectIndex] == 1 || spell->m_implicitTargetB[effectIndex] == 1)) {
        FrameScript_SignalEvent(308);
        return;
      }
    }
  }
}
