#pragma once

#include "Object/ObjectClient/Bag_C.h"
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Ui/PartyFrame.h"

#include <stpl.h>

struct ITEMEXPIRATION;
struct CTerrainClickEvent;
class ItemStats;
class CGGameObject_C;
class CGPetition;
class EmotesTextRec;
struct CGPlayerData;

class CGPlayer {
 protected:
  explicit CGPlayer(unsigned long *storage) : m_plyr(reinterpret_cast<CGPlayerData *>(storage)) {
  }

  void SetStorage(unsigned long *storage) {
    m_plyr = reinterpret_cast<CGPlayerData *>(storage);
  }

  CGPlayerData *m_plyr;
};

struct TRADESKILLLINE : public TSHashObject<TRADESKILLLINE, HASHKEY_NONE> {
  TSGrowableArray<int> spells;
};

struct TexComponentInfo {
  int          m_displayID;
  unsigned int m_inventoryType;
};

enum SPELL_CAST_UI_TYPE {
  SPELL_CAST_UI_NONE = 0,
  SPELL_CAST_UI_PET_TRAINING = 1,
  SPELL_CAST_UI_DISGUISES = 2,
  SPELL_CAST_UI_INSCRIBING = 3,
  NUM_SPELL_CAST_UI_TYPES = 4
};

class CGPlayer_C : public CGUnit_C, public CGPlayer {
  friend bool __fastcall Spell_C_HaveSpellTokens(CGPlayer_C *player, const SpellRec *spell, bool report);
  friend bool __fastcall Spell_C_HaveEquippedSpellItems(CGPlayer_C *player, const SpellRec *spell, bool checkAmmo, bool report);

 public:
  CGPlayer_C(unsigned long *storage, unsigned long eventTime, CClientObjCreate *init);
  virtual ~CGPlayer_C();

  void                               SetStorage(unsigned long *storage);
  void                               PostInit(const CClientObjCreate &init);
  static ITEMEXPIRATION *__fastcall  GetPendingItemExpirationNode(const unsigned __int64 &itemGUID);
  static void __fastcall             Initialize();
  static void __fastcall             InstallGMHandlers();
  static void __fastcall             UninstallGMHandlers();
  static void __fastcall             GMIdle();
  static void __fastcall             StartGhosting(const char *name);
  static void __fastcall             StartGhosting(unsigned __int64 guid);
  static void __fastcall             StopGhosting();
  static void __fastcall             SetRealActivePlayer(unsigned __int64 guid);
  static void __fastcall             SetActive(CGPlayer_C *playerPtr);
  static unsigned __int64 __fastcall GetActive() {
    return ClntObjMgrGetActivePlayer();
  }
  static unsigned __int64 __fastcall GetRealActivePlayer();
  static unsigned int __fastcall     GetNewContinentID();
  static unsigned int __fastcall     OffsetOf(OBJECT_TYPE_ID type);
  static unsigned int __fastcall     GetProficiency(unsigned char type);
  static void __fastcall             UpdateTaxiStatusAll();
  static void __fastcall             UpdateBindStatusAll();
  static unsigned int __fastcall     GetLootItem(unsigned int slot);
  static unsigned int __fastcall     GetLootItemDisplayID(unsigned int slot);
  static unsigned int __fastcall     GetLootItemQuantity(unsigned int slot);
  static void __fastcall             TogglePlayerBounds();
  static void __fastcall             AddDeferredDamage(int normal, unsigned int flags, unsigned int damage, unsigned __int64 victim);
  static void __fastcall             AddDeferredSpellMiss(unsigned __int64 victim, MISS_REASON reason, int spellID);
  static void __fastcall             ProcessDeferredDamage();
  static void __fastcall             ProcessDeferredSpellMiss();
  static void __fastcall             XBuyItem(unsigned __int64 merchant, unsigned int itemID, unsigned int quantity, unsigned int autoEquip);
  unsigned __int64                   GetLocalTarget() const;
  static void __fastcall             Shutdown();
  void                               TrySheathingWeapon();
  void                               SheatheWeapon(unsigned int sheathe);
  void                               SetFarSightFocus(CGObject_C *obj);
  unsigned __int64                   GetFarSightFocusGUID() const {
    const unsigned char *playerData = *reinterpret_cast<const unsigned char *const *>(reinterpret_cast<const unsigned char *>(this) + 0x9E0);
    return *reinterpret_cast<const unsigned __int64 *>(playerData + 560);
  }
  void ToggleFarSight();
  void ClearFarSight();
  int  IsInFarSight() const {
    return (m_flags & 0x800) != 0;
  }
  CGUnit_C                *GetPossessedUnit();
  int                      CanLoot(CGUnit_C *unitPtr);
  virtual unsigned __int64 GetUnitBeingLooted() const {
    return m_lootingUnit;
  }
  static bool __fastcall IsGiftWrapping();
  static void __fastcall CancelGiftWrap();
  void                   SetCombatMode(int state);
  void                   ReadItemResult(NETMESSAGE msgID, CDataStore *msg);
  void                   ReceiveResurrectRequest(const char *name);
  void                   InspectPlayer(const unsigned __int64 &guid);
  void                   AcceptResurrectRequest(int accept);
  void SetInventoryMirrorHandler(unsigned int slot, int(__fastcall *handler)(unsigned __int64, unsigned int, unsigned int, const void *, void *));
  void UnsetInventoryMirrorHandler(unsigned int slot, int(__fastcall *handler)(unsigned __int64, unsigned int, unsigned int, const void *, void *));
  void SetPlayerMirrorHandlers();
  void UnsetPlayerMirrorHandlers();
  void SetActiveMirrorHandlers();
  void UnsetActiveMirrorHandlers();
  virtual void PostReenable();
  void         KillCombatModeTimer();
  void         ResetCombatModeTimer(int newCombat);
  unsigned int GetCombatModeTimerInterval();
  void         ToggleSheathe(unsigned int ignoreAnim);
  int          CanEngageTarget(CGUnit_C *unitPtr);
  void         OnSpellFailed(const SpellRec *spellRec, unsigned int reason);
  unsigned int GetGuildID() const {
    const unsigned int *playerData = *reinterpret_cast<const unsigned int *const *>(reinterpret_cast<const unsigned char *>(this) + 2528);
    return playerData[145];
  }
  unsigned int GetGuildRank() const {
    const unsigned int *playerData = *reinterpret_cast<const unsigned int *const *>(reinterpret_cast<const unsigned char *>(this) + 2528);
    return playerData[146];
  }
  void                SaveTabard(int eStyle, int eColor, int bStyle, int bColor, int bg, unsigned __int64 vendor) const;
  bool                OnGuildChanged();
  virtual const char *GetModelFileName() const;
  void                InitPreferredGeosets();
  void                InitComponents();
  void                AddComponent(int displayID, unsigned int inventoryType, int slot, int commit);
  void                AttachObjComponent(unsigned __int64 item, unsigned int slot, bool defer, bool sheathe, int sheatheAttachmentSlot);
  unsigned int        FindSlotIndex(unsigned __int64 obj);
  void SwapItems(unsigned __int64 cursorItem, unsigned __int64 cursorContainer, int cursorSlot, unsigned __int64 containerB, int slotB, int force);
  void SplitItem(unsigned __int64 cursorItem, unsigned __int64 cursorContainer, int cursorSlot, unsigned __int64 containerB, int slotB, int quantity);
  void AutoStoreItemInBag(
      unsigned __int64 cursorItem,
      unsigned __int64 cursorContainer,
      int              cursorSlot,
      unsigned __int64 containerB,
      int              ignoreOwnershipRules
  );
  void                                  AutoEquipItem(unsigned __int64 container, unsigned int slot, int force);
  void                                  SellItem(unsigned __int64 merchant, unsigned __int64 item, unsigned int amount);
  void                                  AutoEquipCursorItem(int force);
  void                                  ClearPendingEquip(unsigned int index, int equip);
  int                                   OnAttackIconPressed();
  int                                   CanUseItem(const ItemStats *stats, GAME_ERROR_TYPE &reason);
  void                                  QueryQuest(const unsigned __int64 &questGiver, int questID);
  void                                  AcceptQuest(const unsigned __int64 &questGiver, int questID);
  void                                  CompleteQuest(const unsigned __int64 &questGiver, int questID);
  void                                  GiveQuestItems(const unsigned __int64 &questGiver, int questID);
  void                                  GetQuestReward(const unsigned __int64 &questGiver, int questID, int itemChoice);
  void                                  CancelQuest(const unsigned __int64 &questGiver);
  void                                  QuestLogRemoveQuest(int entry);
  void                                  QuestLogSwapQuest(int entry1, int entry2);
  void                                  OpenLootItem(CGItem_C *item);
  void                                  AutoStoreLootItem(unsigned char slot);
  void                                  LootMoney();
  void                                  OpenWrappedItem(CGItem_C *item);
  static void __fastcall                StartGiftWrap(CGItem_C *wrapper);
  void                                  RequestPetitionSignatures(unsigned __int64 item);
  int                                   InviteToGroup(unsigned __int64 target);
  void                                  InviteToGroup(const char *target);
  int                                   Uninvite(unsigned __int64 target);
  void                                  Uninvite(const char *target);
  int                                   SetNewLeader(unsigned __int64 target);
  void                                  SetNewLeader(const char *target);
  void                                  AcceptGroup();
  void                                  DeclineGroup();
  void                                  LeaveGroup();
  void                                  SetLootMethod(LOOT_METHOD method, unsigned __int64 master);
  void                                  AcceptGuild();
  void                                  DeclineGuild();
  int                                   OnTerrainClick(CTerrainClickEvent &__formal);
  unsigned int                          GetPlayerAnimState();
  int                                   OnAttackBreakHandler();
  int                                   ReportBagItemSubtypeMismatch(unsigned int bagSlot) const;
  void                                  SaveDeathMessage(unsigned __int64 guid);
  void                                  CheckKillerFeedback();
  static void __fastcall                SaveBindPoint(CDataStore *msg);
  void                                  HandleMountResult(unsigned int result);
  void                                  HandleDismountResult(unsigned int result);
  virtual float                         GetMountScale() const;
  void                                  OnTaxiNodeStatus(CDataStore *msg);
  void                                  ShowTaxiNodes(CDataStore *msg);
  void                                  StartTaxi(unsigned __int64 vendor, unsigned int startNode, unsigned int destNode);
  void                                  HandleActivateTaxiReply(unsigned int code);
  unsigned int                          CanTrack(CGGameObject_C *object);
  unsigned int                          CanTrack(CGUnit_C *unit);
  void                                  PlayVocalMacro(int category);
  CGItem_C                             *GetSoulstone() const;
  void                                  UseSoulstone() const;
  void                                  HandleRepopRequest();
  int                                   OnPetitionShowList(CDataStore *msg);
  void                                  BuyPetition(const unsigned __int64 &petitionUnit, CGPetition *petition);
  void                                  TurnInGuildCharter();
  void                                  SendTextEmote(EmotesTextRec *rec, const unsigned __int64 &target);
  int                                   OnPetitionShowSignatures(CDataStore *msg);
  int                                   OnSignedResults(CDataStore *msg);
  int                                   OnTurnInPetitionResults(CDataStore *msg);
  int                                   OnVendorInventory(CDataStore *msg);
  int                                   OnBuyFailed(CDataStore *msg);
  int                                   OnBuySucceeded(CDataStore *msg);
  int                                   OnSellResponse(CDataStore *msg);
  int                                   OnQuestGiverListQuests(CDataStore *msg);
  int                                   OnQuestGiverInvalidQuest(CDataStore *msg);
  int                                   OnQuestGiverSendQuest(CDataStore *msg);
  int                                   OnQuestGiverRequestItems(CDataStore *msg);
  int                                   OnQuestGiverChooseReward(CDataStore *msg);
  int                                   OnQuestGiverQuestComplete(CDataStore *msg);
  int                                   OnQuestGiverQuestFailed(CDataStore *msg);
  int                                   OnQuestGiverStatus(CDataStore *msg);
  int                                   OnTrainerList(CDataStore *msg);
  int                                   OnLootResponse(unsigned int eventTime, CDataStore *msg);
  int                                   OnLootReleaseResponse(CDataStore *msg);
  int                                   OnLootRemoved(CDataStore *msg);
  int                                   OnLootMoneyNotify(CDataStore *msg);
  int                                   OnLootClearMoney(CDataStore *msg);
  int                                   OnLootItemNotify(CDataStore *msg);
  int                                   OnSplitMoneyNotify(CDataStore *msg);
  void                                  LootAnimEndHandler();
  void                                  AddKnownSpell(int spellID, int slot, int learned, int addToBook);
  void                                  DelKnownSpell(int spellID);
  void                                  DeleteWornItems();
  void                                  UpdateBindStatus(CGUnit_C *unit);
  void                                  UpdateQuestStatus(const unsigned __int64 &guid);
  void                                  UpdateQuestStatus(CGUnit_C *unit);
  void                                  UpdateQuestStatusAll();
  void                                  UpdateTaxiStatus(CGUnit_C *unit);
  void                                  TalkToTrainer(const unsigned __int64 &trainerUnit);
  void                                  TrainerBuySpell(const unsigned __int64 &trainer, int spellID);
  void                                  TalkToTabardVendor(const unsigned __int64 &tabardUnit);
  void                                  ReadItem(unsigned int packSlot, unsigned int slot);
  void                                  ReadItem(unsigned __int64 containerGUID, unsigned char slot);
  int                                   DeathBindDistanceCompare(NTempest::C3Vector &bindStonePosition);
  static NTempest::C3Vector &__fastcall GetBindPoint();
  int                                   GetLanguageSkill(unsigned int language, unsigned int &skill);
  TSGrowableArray<int>                 *GetTradeSkills(int skillLine);
  TSGrowableArray<int>                 *GetCraftSkills(SPELL_CAST_UI_TYPE type);
  int                                   GetSkillIndex(int skillID);
  int                                   GetSkillRank(int skillID);
  bool                                  GetPackAndSlot(CGItem_C *item, unsigned char &packSlot, unsigned char &slot);
  virtual CGBag_C                      *GetBag() {
    return &m_inventory;
  }
  virtual const CGBag_C *GetBag() const {
    return &m_inventory;
  }

  int IsInCombatMode() const {
    return (m_flags & 0x400) != 0;
  }

 public:
  unsigned int m_framesSinceUpdate;
  unsigned int m_flags;
  int          m_lastWeaponModeSent;

 protected:
  friend class CGGameUI;
  friend class CGWorldFrame;

  TSHashTable<TRADESKILLLINE, HASHKEY_NONE> m_tradeSkillLines;
  TSGrowableArray<int>                      m_craftSpells[4];
  int                                       m_craftActivators[4];
  HMODEL                                    m_components[23][36];
  TexComponentInfo                          m_texComponentInfo[23];
  unsigned __int64                          m_lootingUnit;
  unsigned __int64                          m_lootingUnitSent;
  CGBag_C                                   m_inventory;
  unsigned __int64                          m_lastKillerGUID;
  int                                       m_pendingItemStats;
};
class CreatureModelDataRec;

const CreatureModelDataRec *__fastcall Player_C_GetModelName(unsigned int race, unsigned int sex);
unsigned int __fastcall                Player_C_GetDisplayId(unsigned int race, unsigned int sex);
int __fastcall                         Player_C_AppFocusMovementHandler(int focus);
int __fastcall                         Player_C_ZoneUpdateHandler(const void *eventData, void *arg);
int __fastcall                         Player_C_SetPlayerRender(int enable);
void __fastcall                        Player_C_ClearGuildIDs();
void __fastcall                        PlayerClientInitialize();
void __fastcall                        PlayerClientShutdown();
