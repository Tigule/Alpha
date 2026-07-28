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
struct MirrorSkillInfo {
  unsigned short m_skillLineID;
  unsigned short m_skillRank;
  unsigned short m_skillMaxRank;
  short          m_skillModifier;
  unsigned short m_skillStep;
  unsigned short m_padding;
};

struct CQuestLogData {
  int          m_questID;
  int          m_questGiverID;
  int          m_questRewarderID;
  unsigned int m_questFlags;
  int          m_questFailureTime;
  int          m_qtyMonsterToKill;
};

struct CGPlayerData {
  unsigned __int64 invSlots[69];
  unsigned __int64 selection;
  unsigned __int64 farsightObject;
  unsigned __int64 duelArbiter;
  unsigned int     numInvSlots;
  unsigned int     guildID;
  unsigned int     guildRank;
  unsigned char    skinID;
  unsigned char    faceID;
  unsigned char    hairStyleID;
  unsigned char    hairColorID;
  int               XP;
  int               nextLevelXP;
  MirrorSkillInfo   skillInfo[64];
  unsigned char     playerFlags;
  unsigned char     facialHairStyleID;
  unsigned char     numBankSlots;
  unsigned char     padByte;
  CQuestLogData     questLog[16];
  int               characterPoints[2];
  unsigned int      trackCreatureMask;
  unsigned int      trackResourceMask;
  unsigned int      chatFilters;
  unsigned int      duelTeam;
  float             blockPercentage;
  float             dodgePercentage;
  float             parryPercentage;
  int               baseMana;
  int               guildTimeStamp;
};

class CGPlayer {
  friend int Spell_C_GetManaCost(int id, int isPet);

 public:
  static unsigned int GetDataSize();
  static unsigned int GetBaseOffset();
  static __forceinline unsigned int TotalFields() {
    return 634;
  }
  static unsigned int GetUpdateMaskBytes();
  static unsigned int GetUpdateMaskBlocks();

  unsigned int GetGuildID() const {
    return m_plyr->guildID;
  }
  unsigned int GetGuildRank() const {
    return m_plyr->guildRank;
  }
  int GetXP() const;
  int GetNextLevelXP() const;
  unsigned short GetMirrorSkillID(int index) const {
    return m_plyr->skillInfo[index].m_skillLineID;
  }
  unsigned short GetMirrorSkillRank(int index) const {
    return m_plyr->skillInfo[index].m_skillRank;
  }
  unsigned short GetMirrorSkillMaxRank(int index) const {
    return m_plyr->skillInfo[index].m_skillMaxRank;
  }
  short GetMirrorSkillModifier(int index) const {
    return m_plyr->skillInfo[index].m_skillModifier;
  }
  unsigned short GetMirrorSkillStep(int index) const {
    return m_plyr->skillInfo[index].m_skillStep;
  }
  const CQuestLogData *GetQuestLogData(int index) const {
    return &m_plyr->questLog[index];
  }
  unsigned __int64 GetSelection() const;
  int GetCharacterPoints(int index) const;
  unsigned int GetCreatureTracking() const {
    return m_plyr->trackCreatureMask;
  }
  unsigned int GetResourceTracking() const {
    return m_plyr->trackResourceMask;
  }
  unsigned char GetSkin() const {
    return m_plyr->skinID;
  }
  unsigned char GetFace() const {
    return m_plyr->faceID;
  }
  unsigned char GetHairStyle() const {
    return m_plyr->hairStyleID;
  }
  unsigned char GetHairColorID() const {
    return m_plyr->hairColorID;
  }
  unsigned char GetFacialHair() const {
    return m_plyr->facialHairStyleID;
  }
  unsigned __int64 GetFarsightFocus() const {
    return m_plyr->farsightObject;
  }
  unsigned int GetPlayerFlags() const {
    return m_plyr->playerFlags;
  }
  int GetPVPEnabled() const;
  int IsPartyLeader() const;
  const unsigned __int64 &GetDuelArbiter() const {
    return m_plyr->duelArbiter;
  }
  unsigned char IsDueling() const;
  unsigned int GetDuelTeam() const {
    return m_plyr->duelTeam;
  }
  unsigned char GetNumBankSlots() const {
    return m_plyr->numBankSlots;
  }
  int GetBaseMana() const;
  unsigned char *GetData(unsigned int index);

  void SetStorage(unsigned long *storage) {
    m_plyr = reinterpret_cast<CGPlayerData *>(storage);
  }

 protected:
  explicit CGPlayer(unsigned long *storage) : m_plyr(reinterpret_cast<CGPlayerData *>(storage)) {
  }

  ~CGPlayer() {
  }

  CGPlayerData *Player() {
    return m_plyr;
  }

  const CGPlayerData *Player() const {
    return m_plyr;
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
  friend class CGUnit_C;
  friend bool Spell_C_HaveSpellTokens(CGPlayer_C *player, const SpellRec *spell, bool report);
  friend bool Spell_C_HaveEquippedSpellItems(CGPlayer_C *player, const SpellRec *spell, bool checkAmmo, bool report);

 public:
  CGPlayer_C(unsigned long *storage, unsigned long eventTime, CClientObjCreate *init);
  ~CGPlayer_C();
  virtual void Disable(int shutdown);
  virtual void Reenable();
  virtual int  ShouldRender(unsigned long worldStatus);
  virtual void PreAnimate(CGWorldFrame *worldFrame);
  virtual void GetAFKText(char *buffer, int size) const;
  virtual void GetDNDText(char *buffer, int size) const;
  virtual void GetGMText(char *buffer, int size) const;
  virtual unsigned __int64 GetLocalTarget() const;
  virtual void HandleSpellEventSound();
  virtual void CombatLoggingFlagChanged();
  virtual unsigned __int64 GetUnitBeingLooted() const;
  virtual void OnAttackStart(unsigned __int64 victim);
  virtual void OnAttackStop(unsigned __int64 previousTarget, int nowDead);
  virtual void OnDeath();
  virtual void OnDeathAnimate();
  virtual void OnBadAttackFacing(unsigned __int64 victim);
  virtual void OnBadAttackTarget(unsigned __int64 victim);
  virtual void OnBadAttackPosition(unsigned __int64 victim, float range);
  virtual void OnNotStanding(unsigned __int64 victim);
  virtual void UnitHit(VICTIMSTATES state, unsigned __int64 attacker);
  virtual void OnAttackerStateChange(const ATTACKROUNDINFO &roundInfo);
  virtual void HandleMirrorTimerDamage(const MIRRORTIMERDAMAGE &log);
  virtual void PlayUnitSound(UNITSOUNDTYPE soundType, int alwaysPlay) const;
  virtual void PlayFoleySound() const;

 protected:
  virtual unsigned int GetImpactType() const;

 public:
  virtual const VirtualItemInfo *GetDefendingItem() const;
  virtual void PlayDeathThudCameraShake() const;
  virtual void LootAnimEndHandler();
  virtual void SetTorsoAnimState(unsigned int newState);
  virtual void SetBaseAnimState(unsigned int newState);

 protected:
  virtual unsigned int DetermineWoundSequence() const;

 public:
  virtual const VirtualItemInfo *GetVirtualItem(unsigned int slot, bool ignoreDisarmFlag) const;
  virtual int GetVirtualItemDisplayID(unsigned int slot) const;
  virtual int ShouldRenderUnitName(unsigned int mode) const;
  virtual void CommitTexture(int force);
  virtual unsigned int UpdateUnitNameString(unsigned int localPlayerFlags, unsigned int otherUnitsFlags, char *buffer, unsigned int bufferSize) const;
  virtual float GetMountScale() const;
  virtual void OnMount();
  virtual void OnDismount();
  virtual bool CanBeMounted();

 protected:
  virtual void CleanupUnitArtwork(int playerModelChanged, int wasPlayerModel);
  virtual void ReinitializeUnitArtwork();
  virtual void PostReinitializeArtwork();

 public:
  virtual void OnStandStateChanged(unsigned int oldState, unsigned int newState);
  virtual void ChangeStandState(unsigned int standState);
  virtual void SetEmoteState(unsigned int emoteID);
  virtual UNITAFFILIATION GetGUIDAffiliation(unsigned __int64 unit) const;
  virtual int GetSpellRank(int spellID) const;
  unsigned int GetDisplayRace() const {
    return CGUnit_C::GetDisplayRace();
  }
  unsigned int GetDisplaySex() const {
    return CGUnit_C::GetDisplaySex();
  }
  virtual bool GetDefenseSkillRank(int &base, int &modifier) const;
  virtual bool GetAttackSkillRank(int hand, int &base, int &modifier) const;
  virtual void OnLevelChange();
  virtual float GetBlockChance() const;
  virtual float GetDodgeChance() const;
  virtual float GetParryChance() const;
  virtual int GetSpellCastingTime(int spellID) const;
  virtual void UpdateObjComponentVisuals(const CGItem_C *item, const ItemEnchantment *enchantments, int num);
  virtual void ClearItemVisuals(ACTIVEATTACHMENTINFO *info);
  virtual void SetItemVisuals(ACTIVEATTACHMENTINFO *info, const ItemVisualsRec *rec, bool force);
  virtual void SetLastWeaponModeSent(int mode);

  void                               SetStorage(unsigned long *storage);
  void                               PostInit(const CClientObjCreate &init);
  void                               GuildInfoLoaded(const TSGrowableArray<unsigned int> &guildList);
  static ITEMEXPIRATION *GetPendingItemExpirationNode(const unsigned __int64 &itemGUID);
  static void Initialize();
  static void InstallGMHandlers();
  static void UninstallGMHandlers();
  static void GMIdle();
  static void StartGhosting(const char *name);
  static void StartGhosting(unsigned __int64 guid);
  static void StopGhosting();
  static void SetRealActivePlayer(unsigned __int64 guid);
  static void SetActive(const CGPlayer_C *playerPtr);
  static unsigned __int64 GetActive() {
    return ClntObjMgrGetActivePlayer();
  }
  static unsigned __int64 GetRealActivePlayer();
  static unsigned int GetNewContinentID();
  static unsigned int OffsetOf(OBJECT_TYPE_ID type);
  static unsigned int GetProficiency(unsigned char type);
  static void UpdateTaxiStatusAll();
  static void UpdateBindStatusAll();
  static unsigned int GetLootItem(unsigned int slot);
  static unsigned int GetLootItemDisplayID(unsigned int slot);
  static unsigned int GetLootItemQuantity(unsigned int slot);
  void                               OnLootGameObject(const unsigned __int64 &gameObject, bool lootAnim);
  const unsigned __int64             &GetUnitLootingSent() const;
  void                               ClearLootingUnitSent();
  void                               SetLootCloseSentFlag();
  static void TogglePlayerBounds();
  static void AddDeferredDamage(int normal, unsigned int flags, unsigned int damage, unsigned __int64 victim);
  static void AddDeferredSpellMiss(unsigned __int64 victim, MISS_REASON reason, int spellID);
  static void ProcessDeferredDamage();
  static void ProcessDeferredSpellMiss();
  static void XBuyItem(unsigned __int64 merchant, unsigned int itemID, unsigned char quantity, bool autoEquip);
  static void XBuyItemInSlot(
      unsigned __int64 merchant,
      unsigned int itemID,
      unsigned char quantity,
      unsigned __int64 container,
      unsigned char slot
  );
  static void XBuyItemInBag(unsigned __int64 merchant, unsigned int itemID, unsigned char quantity, unsigned __int64 container);
  static void UpdatePendingItemExpiration(const unsigned __int64 &itemGUID);
  static void Shutdown();
  void                               TrySheathingWeapon();
  void                               SheatheWeapon(bool sheathe);
  void                               SetFarSightFocus(CGObject_C *obj);
  void ToggleFarSight();
  void ClearFarSight();
  int  IsInFarSight() {
    return (m_flags & 0x800) != 0;
  }
  void BotMove(unsigned long now, NTempest::C3Vector *points, int count, unsigned long duration, unsigned int flags);
  int BotSpline();
  CGUnit_C                *GetPossessedUnit();
  int                      CanLoot(CGUnit_C *unitPtr);
  static bool IsGiftWrapping();
  static void CancelGiftWrap();
  void                   SetCombatMode(int state);
  void                   ReadItemResult(NETMESSAGE msgID, CDataStore *msg);
  void                   ReceiveResurrectRequest(const char *name);
  void                   InspectPlayer(const unsigned __int64 &guid);
  void                   AcceptResurrectRequest(int accept);

 private:
  void SetInventoryMirrorHandler(unsigned int slot, int(*handler)(unsigned __int64, unsigned int, unsigned int, const void *, void *));
  void UnsetInventoryMirrorHandler(unsigned int slot, int(*handler)(unsigned __int64, unsigned int, unsigned int, const void *, void *));
  void SetPlayerMirrorHandlers();
  void UnsetPlayerMirrorHandlers();

 public:
  void SetActiveMirrorHandlers();

 private:
  void UnsetActiveMirrorHandlers();

 public:
  virtual void PostReenable();
  void         ResetCombatModeTimer(int newCombat);
  void         ToggleSheathe(bool ignoreAnim);
  void         KillExitCombatModeSheatheTimer();
  void         StartSheatheAnim(INVENTORY_SLOTS slot, int hip, int both);
  int          CanEngageTarget(const CGUnit_C *unitPtr);
  void         OnSpellFailed(const SpellRec *spellRec, unsigned int reason);
  void                SaveTabard(int eStyle, int eColor, int bStyle, int bColor, int bg, unsigned __int64 vendor) const;
  bool                OnGuildChanged();
  virtual const char *GetModelFileName() const;

 private:
  void                InitPreferredGeosets();
  void                InitComponents();
  CGPlayer_C &operator=(const CGPlayer_C &);

 public:
  void                AddComponent(int displayID, unsigned int inventoryType, int slot, int commit);
  void                RemoveComponent(int slot, bool commitItemGeosets, bool defer, bool removeRecord);
  void                AttachObjComponent(unsigned __int64 item, unsigned int slot, bool defer, bool sheathe, int sheatheAttachmentSlot);
  unsigned char       FindSlotIndex(unsigned __int64 obj);
  unsigned char       FindItemSlot(unsigned __int64 containerGUID, CGItem_C *item);
  int                 SwapInventorySlots(int slotA, int slotB);
  void MoveItem(
      unsigned __int64 item,
      unsigned __int64 itemContainer,
      unsigned int slot,
      unsigned __int64 newContainer,
      unsigned int newSlot
  );
  void SwapItems(unsigned __int64 cursorItem, unsigned __int64 cursorContainer, int cursorSlot, unsigned __int64 containerB, int slotB, int force);
  void SplitItem(unsigned __int64 cursorItem, unsigned __int64 cursorContainer, int cursorSlot, unsigned __int64 containerB, int slotB, int quantity);
  void DropItemInCursor(unsigned __int64 cursorItem, unsigned __int64 cursorItemPack, unsigned int cursorSlot);
  void AutoStoreItemInBag(
      unsigned __int64 cursorItem,
      unsigned __int64 cursorContainer,
      int              cursorSlot,
      unsigned __int64 containerB,
      int              ignoreOwnershipRules
  );
  void                                  AutoEquipItem(unsigned __int64 container, unsigned int slot, int force);
  static void SellItem(unsigned __int64 merchant, unsigned __int64 item, unsigned int amount);
  void                                  AutoEquipCursorItem(int force);
  void                                  ClearPendingEquip(unsigned int index, int equip);
  int                                   HasEquipped(int classID, int subclassID);
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
  void                                  PutLootInSlot(unsigned __int64 container, unsigned char containerSlot, unsigned char lootSlot);
  void                                  PutLootInBag(unsigned __int64 container, unsigned char lootSlot);
  void                                  LootMoney();
  void                                  OpenWrappedItem(CGItem_C *item);
  static void StartGiftWrap(CGItem_C *wrapper);
  void                                  GiftWrap(CGItem_C *item);
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
  int                                   SetBlock(unsigned int index, unsigned long data);
  void                                  SetData(const void *data, unsigned int bytes);
  int                                   OnTerrainClick(const CTerrainClickEvent &__formal);
  unsigned int                          GetPlayerAnimState();
  int                                   OnAttackBreakHandler();
  int                                   ReportBagItemSubtypeMismatch(unsigned char bagSlot) const;
  void                                  SaveDeathMessage(unsigned __int64 guid);
  void                                  CheckKillerFeedback();
  void                                  OnUnitDeath(unsigned __int64 guid);
  void                                  OnObjectDestruct(unsigned __int64 guid);
  static void                           OnItemDelete(unsigned __int64 item, unsigned __int64 listener);
  void                                  OnItemDelete(unsigned __int64 item);
  void                                  PlayerFlagsChanged(unsigned char oldFlags);
  static void SaveBindPoint(CDataStore *msg);
  void                                  HandleMountResult(unsigned int result);
  void                                  HandleDismountResult(unsigned int result);
  void                                  OnTaxiNodeStatus(CDataStore *msg);
  void                                  ShowTaxiNodes(CDataStore *msg);
  void                                  StartTaxi(unsigned __int64 vendor, unsigned int startNode, unsigned int destNode);
  void                                  HandleActivateTaxiReply(unsigned int code);
  bool                                  CanTrack(const CGGameObject_C *object);
  bool                                  CanTrack(const CGUnit_C *unit);
  void                                  PlayMacroSound(int category) const;
  void                                  PlayVocalMacro(int category);
  CGItem_C                             *GetSoulstone() const;
  void                                  UseSoulstone() const;
  void                                  HandleRepopRequest();
  int                                   OnPetitionShowList(CDataStore *msg);
  void                                  BuyPetition(const unsigned __int64 &petitionUnit, CGPetition *petition);
  void                                  TurnInGuildCharter();
  void                                  SendTextEmote(const EmotesTextRec *rec, const unsigned __int64 &target) const;
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
  void                                  AddKnownSpell(int spellID, int slot, int learned, int addToBook);
  void                                  DelKnownSpell(int spellID);
  void                                  DeleteWornItems() const;
  unsigned int                          GetFramesSinceUpdate();
  void                                  SkipUpdate();
  void                                  UpdateText();
  void                                  UpdateBindStatus(CGUnit_C *unit);
  void                                  UpdateQuestStatus(const unsigned __int64 &guid);
  void                                  UpdateQuestStatus(CGUnit_C *unit);
  static void                           UpdateQuestStatusAll();
  void                                  UpdateTaxiStatus(CGUnit_C *unit);
  int                                   LootUnit(CGUnit_C *unit);
  void                                  ShopFromMerchant(const unsigned __int64 &merchant);
  int                                   IsQuestUnit(CGUnit_C *unit);
  void                                  TalkToQuestUnit(const unsigned __int64 &unit);
  int                                   QueryTaxiNodes(const unsigned __int64 &unit);
  void                                  TalkToTrainer(const unsigned __int64 &trainerUnit);
  void                                  TalkToBinder(const unsigned __int64 &binder);
  void                                  TalkToBanker(const unsigned __int64 &banker);
  void                                  TalkToNpcPetition(const unsigned __int64 &vendor);
  void                                  TrainerBuySpell(const unsigned __int64 &trainer, int spellID);
  void                                  TalkToTabardVendor(const unsigned __int64 &tabardUnit);
  void                                  ReadItem(unsigned char packSlot, unsigned char slot);
  void                                  ReadItem(unsigned __int64 containerGUID, unsigned char slot);
  int                                   DeathBindDistanceCompare(const NTempest::C3Vector &bindStonePosition);
  static const NTempest::C3Vector &GetBindPoint();
  int                                   GetLanguageSkill(unsigned int language, unsigned int &skill);
  unsigned int                          GetDefaultLanguage();
  const TSGrowableArray<int>           *GetTradeSkills(int skillLine) const;
  const TSGrowableArray<int>           *GetCraftSkills(SPELL_CAST_UI_TYPE type) const;
  int                                   GetCraftSkillActivator(SPELL_CAST_UI_TYPE type) const;
  int                                   GetSkillIndex(int skillID) const;
  int                                   GetSkillRank(int skillID) const;
  void                                  CheckWeaponDefenseRankChange() const;
  void                                  CheckWeaponDefenseRankChange(COMBATHAND hand) const;
  int                                   ValidateSlot(unsigned int slotID, unsigned __int64 cursorItem);
  bool                                  GetExpandedSkillRank(int skillID, int &rank, int &modifier) const;
  bool                                  GetPackAndSlot(CGItem_C *item, unsigned char &packSlot, unsigned char &slot);
  CGBag_C                              *Inventory();
  const CGBag_C                        *Inventory() const;
  virtual CGBag_C                      *GetBag() {
    return &m_inventory;
  }
  virtual const CGBag_C *GetBag() const {
    return &m_inventory;
  }
  virtual void ItemReceived(const ItemStats *stats) const;
  void IncrementPendingItemStats();
  void DecrementPendingItemStats();
  void FixComponenting(CGItem_C *item);

  int IsInCombatMode() const {
    return (m_flags & 0x400) != 0;
  }

 private:
  void KillCombatModeTimer();
  unsigned int GetCombatModeTimerInterval() const;

 protected:
  void CheckWeaponRankChange() const;
  void CheckDefenseRankChange() const;
  void SetGuildMirrorHandler();
  void UnsetGuildMirrorHandler();
  void SetBankMirrorHandlers();
  void UnsetBankMirrorHandlers();

  unsigned int m_framesSinceUpdate;
  unsigned int m_flags;
  int          m_lastWeaponModeSent;

 protected:
  friend class CGGameUI;
  friend class CGWorldFrame;

  int GetWeaponSpell(COMBATHAND hand) const;

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

const CreatureModelDataRec *Player_C_GetModelName(unsigned int race, unsigned int sex);
unsigned int Player_C_GetDisplayId(unsigned int race, unsigned int sex);
int Player_C_AppFocusMovementHandler(int focus);
int Player_C_ZoneUpdateHandler(const void *eventData, void *arg);
int Player_C_SetPlayerRender(int enable);
void Player_C_ClearGuildIDs();
void PlayerClientInitialize();
void PlayerClientShutdown();
