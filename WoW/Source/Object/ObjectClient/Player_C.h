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
  WORD  m_skillLineID;
  WORD  m_skillRank;
  WORD  m_skillMaxRank;
  short m_skillModifier;
  WORD  m_skillStep;
  WORD  m_padding;
};

struct CQuestLogData {
  int  m_questID;
  int  m_questGiverID;
  int  m_questRewarderID;
  UINT m_questFlags;
  int  m_questFailureTime;
  int  m_qtyMonsterToKill;
};

struct CGPlayerData {
  DWORDLONG       invSlots[69];
  DWORDLONG       selection;
  DWORDLONG       farsightObject;
  DWORDLONG       duelArbiter;
  UINT            numInvSlots;
  UINT            guildID;
  UINT            guildRank;
  BYTE            skinID;
  BYTE            faceID;
  BYTE            hairStyleID;
  BYTE            hairColorID;
  int             XP;
  int             nextLevelXP;
  MirrorSkillInfo skillInfo[64];
  BYTE            playerFlags;
  BYTE            facialHairStyleID;
  BYTE            numBankSlots;
  BYTE            padByte;
  CQuestLogData   questLog[16];
  int             characterPoints[2];
  UINT            trackCreatureMask;
  UINT            trackResourceMask;
  UINT            chatFilters;
  UINT            duelTeam;
  float           blockPercentage;
  float           dodgePercentage;
  float           parryPercentage;
  int             baseMana;
  int             guildTimeStamp;
};

class CGPlayer {
  friend int Spell_C_GetManaCost(int id, BOOL isPet);

 public:
  static UINT               GetDataSize();
  static UINT               GetBaseOffset();
  static __forceinline UINT TotalFields() {
    return 634;
  }
  static UINT GetUpdateMaskBytes();
  static UINT GetUpdateMaskBlocks();

  UINT GetGuildID() const {
    return m_plyr->guildID;
  }
  UINT GetGuildRank() const {
    return m_plyr->guildRank;
  }
  int  GetXP() const;
  int  GetNextLevelXP() const;
  WORD GetMirrorSkillID(int index) const {
    return m_plyr->skillInfo[index].m_skillLineID;
  }
  WORD GetMirrorSkillRank(int index) const {
    return m_plyr->skillInfo[index].m_skillRank;
  }
  WORD GetMirrorSkillMaxRank(int index) const {
    return m_plyr->skillInfo[index].m_skillMaxRank;
  }
  short GetMirrorSkillModifier(int index) const {
    return m_plyr->skillInfo[index].m_skillModifier;
  }
  WORD GetMirrorSkillStep(int index) const {
    return m_plyr->skillInfo[index].m_skillStep;
  }
  const CQuestLogData *GetQuestLogData(int index) const {
    return &m_plyr->questLog[index];
  }
  DWORDLONG GetSelection() const;
  int       GetCharacterPoints(int index) const;
  UINT      GetCreatureTracking() const {
    return m_plyr->trackCreatureMask;
  }
  UINT GetResourceTracking() const {
    return m_plyr->trackResourceMask;
  }
  BYTE GetSkin() const {
    return m_plyr->skinID;
  }
  BYTE GetFace() const {
    return m_plyr->faceID;
  }
  BYTE GetHairStyle() const {
    return m_plyr->hairStyleID;
  }
  BYTE GetHairColorID() const {
    return m_plyr->hairColorID;
  }
  BYTE GetFacialHair() const {
    return m_plyr->facialHairStyleID;
  }
  DWORDLONG GetFarsightFocus() const {
    return m_plyr->farsightObject;
  }
  UINT GetPlayerFlags() const {
    return m_plyr->playerFlags;
  }
  int              GetPVPEnabled() const;
  BOOL             IsPartyLeader() const;
  const DWORDLONG &GetDuelArbiter() const {
    return m_plyr->duelArbiter;
  }
  BYTE IsDueling() const;
  UINT GetDuelTeam() const {
    return m_plyr->duelTeam;
  }
  BYTE GetNumBankSlots() const {
    return m_plyr->numBankSlots;
  }
  int   GetBaseMana() const;
  BYTE *GetData(UINT index);

  void SetStorage(DWORD *storage) {
    m_plyr = reinterpret_cast<CGPlayerData *>(storage);
  }

 protected:
  explicit CGPlayer(DWORD *storage) : m_plyr(reinterpret_cast<CGPlayerData *>(storage)) {
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
  int  m_displayID;
  UINT m_inventoryType;
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
  CGPlayer_C(DWORD *storage, DWORD eventTime, CClientObjCreate *init);
  ~CGPlayer_C();
  virtual void      Disable(int shutdown);
  virtual void      Reenable();
  virtual BOOL      ShouldRender(DWORD worldStatus);
  virtual void      PreAnimate(CGWorldFrame *worldFrame);
  virtual void      GetAFKText(char *buffer, int size) const;
  virtual void      GetDNDText(char *buffer, int size) const;
  virtual void      GetGMText(char *buffer, int size) const;
  virtual DWORDLONG GetLocalTarget() const;
  virtual void      HandleSpellEventSound();
  virtual void      CombatLoggingFlagChanged();
  virtual DWORDLONG GetUnitBeingLooted() const;
  virtual void      OnAttackStart(DWORDLONG victim);
  virtual void      OnAttackStop(DWORDLONG previousTarget, int nowDead);
  virtual void      OnDeath();
  virtual void      OnDeathAnimate();
  virtual void      OnBadAttackFacing(DWORDLONG victim);
  virtual void      OnBadAttackTarget(DWORDLONG victim);
  virtual void      OnBadAttackPosition(DWORDLONG victim, float range);
  virtual void      OnNotStanding(DWORDLONG victim);
  virtual void      UnitHit(VICTIMSTATES state, DWORDLONG attacker);
  virtual void      OnAttackerStateChange(const ATTACKROUNDINFO &roundInfo);
  virtual void      HandleMirrorTimerDamage(const MIRRORTIMERDAMAGE &log);
  virtual void      PlayUnitSound(UNITSOUNDTYPE soundType, int alwaysPlay) const;
  virtual void      PlayFoleySound() const;

 protected:
  virtual UINT GetImpactType() const;

 public:
  virtual const VirtualItemInfo *GetDefendingItem() const;
  virtual void                   PlayDeathThudCameraShake() const;
  virtual void                   LootAnimEndHandler();
  virtual void                   SetTorsoAnimState(UINT newState);
  virtual void                   SetBaseAnimState(UINT newState);

 protected:
  virtual UINT DetermineWoundSequence() const;

 public:
  virtual const VirtualItemInfo *GetVirtualItem(UINT slot, bool ignoreDisarmFlag) const;
  virtual int                    GetVirtualItemDisplayID(UINT slot) const;
  virtual BOOL                   ShouldRenderUnitName(UINT mode) const;
  virtual void                   CommitTexture(int force);
  virtual UINT                   UpdateUnitNameString(UINT localPlayerFlags, UINT otherUnitsFlags, char *buffer, UINT bufferSize) const;
  virtual float                  GetMountScale() const;
  virtual void                   OnMount();
  virtual void                   OnDismount();
  virtual bool                   CanBeMounted();

 protected:
  virtual void CleanupUnitArtwork(int playerModelChanged, BOOL wasPlayerModel);
  virtual void ReinitializeUnitArtwork();
  virtual void PostReinitializeArtwork();

 public:
  virtual void            OnStandStateChanged(UINT oldState, UINT newState);
  virtual void            ChangeStandState(UINT standState);
  virtual void            SetEmoteState(UINT emoteID);
  virtual UNITAFFILIATION GetGUIDAffiliation(DWORDLONG unit) const;
  virtual int             GetSpellRank(int spellID) const;
  UINT                    GetDisplayRace() const {
    return CGUnit_C::GetDisplayRace();
  }
  UINT GetDisplaySex() const {
    return CGUnit_C::GetDisplaySex();
  }
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

  void                   SetStorage(DWORD *storage);
  void                   PostInit(const CClientObjCreate &init);
  void                   GuildInfoLoaded(const TSGrowableArray<UINT> &guildList);
  static ITEMEXPIRATION *GetPendingItemExpirationNode(const DWORDLONG &itemGUID);
  static void            Initialize();
  static void            InstallGMHandlers();
  static void            UninstallGMHandlers();
  static void            GMIdle();
  static void            StartGhosting(LPCSTR name);
  static void            StartGhosting(DWORDLONG guid);
  static void            StopGhosting();
  static void            SetRealActivePlayer(DWORDLONG guid);
  static void            SetActive(const CGPlayer_C *playerPtr);
  static DWORDLONG       GetActive() {
    return ClntObjMgrGetActivePlayer();
  }
  static DWORDLONG GetRealActivePlayer();
  static UINT      GetNewContinentID();
  static UINT      OffsetOf(OBJECT_TYPE_ID type);
  static UINT      GetProficiency(BYTE type);
  static void      UpdateTaxiStatusAll();
  static void      UpdateBindStatusAll();
  static UINT      GetLootItem(UINT slot);
  static UINT      GetLootItemDisplayID(UINT slot);
  static UINT      GetLootItemQuantity(UINT slot);
  void             OnLootGameObject(const DWORDLONG &gameObject, bool lootAnim);
  const DWORDLONG &GetUnitLootingSent() const;
  void             ClearLootingUnitSent();
  void             SetLootCloseSentFlag();
  static void      TogglePlayerBounds();
  static void      AddDeferredDamage(int normal, UINT flags, UINT damage, DWORDLONG victim);
  static void      AddDeferredSpellMiss(DWORDLONG victim, MISS_REASON reason, int spellID);
  static void      ProcessDeferredDamage();
  static void      ProcessDeferredSpellMiss();
  static void      XBuyItem(DWORDLONG merchant, UINT itemID, BYTE quantity, bool autoEquip);
  static void      XBuyItemInSlot(DWORDLONG merchant, UINT itemID, BYTE quantity, DWORDLONG container, BYTE slot);
  static void      XBuyItemInBag(DWORDLONG merchant, UINT itemID, BYTE quantity, DWORDLONG container);
  static void      UpdatePendingItemExpiration(const DWORDLONG &itemGUID);
  static void      Shutdown();
  void             TrySheathingWeapon();
  void             SheatheWeapon(bool sheathe);
  void             SetFarSightFocus(CGObject_C *obj);
  void             ToggleFarSight();
  void             ClearFarSight();
  BOOL             IsInFarSight() {
    return (m_flags & 0x800) != 0;
  }
  void        BotMove(DWORD now, NTempest::C3Vector *points, int count, DWORD duration, UINT flags);
  int         BotSpline();
  CGUnit_C   *GetPossessedUnit();
  BOOL        CanLoot(CGUnit_C *unitPtr);
  static bool IsGiftWrapping();
  static void CancelGiftWrap();
  void        SetCombatMode(int state);
  void        ReadItemResult(NETMESSAGE msgID, CDataStore *msg);
  void        ReceiveResurrectRequest(LPCSTR name);
  void        InspectPlayer(const DWORDLONG &guid);
  void        AcceptResurrectRequest(int accept);

 private:
  void SetInventoryMirrorHandler(UINT slot, int (*handler)(DWORDLONG, UINT, UINT, LPCVOID, LPVOID));
  void UnsetInventoryMirrorHandler(UINT slot, int (*handler)(DWORDLONG, UINT, UINT, LPCVOID, LPVOID));
  void SetPlayerMirrorHandlers();
  void UnsetPlayerMirrorHandlers();

 public:
  void SetActiveMirrorHandlers();

 private:
  void UnsetActiveMirrorHandlers();

 public:
  virtual void   PostReenable();
  void           ResetCombatModeTimer(int newCombat);
  void           ToggleSheathe(bool ignoreAnim);
  void           KillExitCombatModeSheatheTimer();
  void           StartSheatheAnim(INVENTORY_SLOTS slot, int hip, int both);
  BOOL           CanEngageTarget(const CGUnit_C *unitPtr);
  void           OnSpellFailed(const SpellRec *spellRec, UINT reason);
  void           SaveTabard(int eStyle, int eColor, int bStyle, int bColor, int bg, DWORDLONG vendor) const;
  bool           OnGuildChanged();
  virtual LPCSTR GetModelFileName() const;

 private:
  void        InitPreferredGeosets();
  void        InitComponents();
  CGPlayer_C &operator=(const CGPlayer_C &);

 public:
  void        AddComponent(int displayID, UINT inventoryType, int slot, int commit);
  void        RemoveComponent(int slot, bool commitItemGeosets, bool defer, bool removeRecord);
  void        AttachObjComponent(DWORDLONG item, UINT slot, bool defer, bool sheathe, int sheatheAttachmentSlot);
  BYTE        FindSlotIndex(DWORDLONG obj);
  BYTE        FindItemSlot(DWORDLONG containerGUID, CGItem_C *item);
  int         SwapInventorySlots(int slotA, int slotB);
  void        MoveItem(DWORDLONG item, DWORDLONG itemContainer, UINT slot, DWORDLONG newContainer, UINT newSlot);
  void        SwapItems(DWORDLONG cursorItem, DWORDLONG cursorContainer, int cursorSlot, DWORDLONG containerB, int slotB, int force);
  void        SplitItem(DWORDLONG cursorItem, DWORDLONG cursorContainer, int cursorSlot, DWORDLONG containerB, int slotB, int quantity);
  void        DropItemInCursor(DWORDLONG cursorItem, DWORDLONG cursorItemPack, UINT cursorSlot);
  void        AutoStoreItemInBag(DWORDLONG cursorItem, DWORDLONG cursorContainer, int cursorSlot, DWORDLONG containerB, int ignoreOwnershipRules);
  void        AutoEquipItem(DWORDLONG container, UINT slot, int force);
  static void SellItem(DWORDLONG merchant, DWORDLONG item, UINT amount);
  void        AutoEquipCursorItem(int force);
  void        ClearPendingEquip(UINT index, int equip);
  BOOL        HasEquipped(int classID, int subclassID);
  BOOL        OnAttackIconPressed();
  BOOL        CanUseItem(const ItemStats *stats, GAME_ERROR_TYPE &reason);
  void        QueryQuest(const DWORDLONG &questGiver, int questID);
  void        AcceptQuest(const DWORDLONG &questGiver, int questID);
  void        CompleteQuest(const DWORDLONG &questGiver, int questID);
  void        GiveQuestItems(const DWORDLONG &questGiver, int questID);
  void        GetQuestReward(const DWORDLONG &questGiver, int questID, int itemChoice);
  void        CancelQuest(const DWORDLONG &questGiver);
  void        QuestLogRemoveQuest(int entry);
  void        QuestLogSwapQuest(int entry1, int entry2);
  void        OpenLootItem(CGItem_C *item);
  void        AutoStoreLootItem(BYTE slot);
  void        PutLootInSlot(DWORDLONG container, BYTE containerSlot, BYTE lootSlot);
  void        PutLootInBag(DWORDLONG container, BYTE lootSlot);
  void        LootMoney();
  void        OpenWrappedItem(CGItem_C *item);
  static void StartGiftWrap(CGItem_C *wrapper);
  void        GiftWrap(CGItem_C *item);
  void        RequestPetitionSignatures(DWORDLONG item);
  BOOL        InviteToGroup(DWORDLONG target);
  void        InviteToGroup(LPCSTR target);
  int         Uninvite(DWORDLONG target);
  void        Uninvite(LPCSTR target);
  BOOL        SetNewLeader(DWORDLONG target);
  void        SetNewLeader(LPCSTR target);
  void        AcceptGroup();
  void        DeclineGroup();
  void        LeaveGroup();
  void        SetLootMethod(LOOT_METHOD method, DWORDLONG master);
  void        AcceptGuild();
  void        DeclineGuild();
  BOOL        SetBlock(UINT index, DWORD data);
  void        SetData(LPCVOID data, UINT bytes);
  BOOL        OnTerrainClick(const CTerrainClickEvent &);
  UINT        GetPlayerAnimState();
  BOOL        OnAttackBreakHandler();
  BOOL        ReportBagItemSubtypeMismatch(BYTE bagSlot) const;
  void        SaveDeathMessage(DWORDLONG guid);
  void        CheckKillerFeedback();
  void        OnUnitDeath(DWORDLONG guid);
  void        OnObjectDestruct(DWORDLONG guid);
  static void OnItemDelete(DWORDLONG item, DWORDLONG listener);
  void        OnItemDelete(DWORDLONG item);
  void        PlayerFlagsChanged(BYTE oldFlags);
  static void SaveBindPoint(CDataStore *msg);
  void        HandleMountResult(UINT result);
  void        HandleDismountResult(UINT result);
  void        OnTaxiNodeStatus(CDataStore *msg);
  void        ShowTaxiNodes(CDataStore *msg);
  void        StartTaxi(DWORDLONG vendor, UINT startNode, UINT destNode);
  void        HandleActivateTaxiReply(UINT code);
  bool        CanTrack(const CGGameObject_C *object);
  bool        CanTrack(const CGUnit_C *unit);
  void        PlayMacroSound(int category) const;
  void        PlayVocalMacro(int category);
  CGItem_C   *GetSoulstone() const;
  void        UseSoulstone() const;
  void        HandleRepopRequest();
  BOOL        OnPetitionShowList(CDataStore *msg);
  void        BuyPetition(const DWORDLONG &petitionUnit, CGPetition *petition);
  void        TurnInGuildCharter();
  void        SendTextEmote(const EmotesTextRec *rec, const DWORDLONG &target) const;
  BOOL        OnPetitionShowSignatures(CDataStore *msg);
  BOOL        OnSignedResults(CDataStore *msg);
  BOOL        OnTurnInPetitionResults(CDataStore *msg);
  BOOL        OnVendorInventory(CDataStore *msg);
  BOOL        OnBuyFailed(CDataStore *msg);
  BOOL        OnBuySucceeded(CDataStore *msg);
  BOOL        OnSellResponse(CDataStore *msg);
  BOOL        OnQuestGiverListQuests(CDataStore *msg);
  BOOL        OnQuestGiverInvalidQuest(CDataStore *msg);
  BOOL        OnQuestGiverSendQuest(CDataStore *msg);
  BOOL        OnQuestGiverRequestItems(CDataStore *msg);
  BOOL        OnQuestGiverChooseReward(CDataStore *msg);
  BOOL        OnQuestGiverQuestComplete(CDataStore *msg);
  BOOL        OnQuestGiverQuestFailed(CDataStore *msg);
  BOOL        OnQuestGiverStatus(CDataStore *msg);
  BOOL        OnTrainerList(CDataStore *msg);
  BOOL        OnLootResponse(UINT eventTime, CDataStore *msg);
  BOOL        OnLootReleaseResponse(CDataStore *msg);
  BOOL        OnLootRemoved(CDataStore *msg);
  BOOL        OnLootMoneyNotify(CDataStore *msg);
  BOOL        OnLootClearMoney(CDataStore *msg);
  BOOL        OnLootItemNotify(CDataStore *msg);
  BOOL        OnSplitMoneyNotify(CDataStore *msg);
  void        AddKnownSpell(int spellID, int slot, int learned, int addToBook);
  void        DelKnownSpell(int spellID);
  void        DeleteWornItems() const;
  UINT        GetFramesSinceUpdate();
  void        SkipUpdate();
  void        UpdateText();
  void        UpdateBindStatus(CGUnit_C *unit);
  void        UpdateQuestStatus(const DWORDLONG &guid);
  void        UpdateQuestStatus(CGUnit_C *unit);
  static void UpdateQuestStatusAll();
  void        UpdateTaxiStatus(CGUnit_C *unit);
  int         LootUnit(CGUnit_C *unit);
  void        ShopFromMerchant(const DWORDLONG &merchant);
  BOOL        IsQuestUnit(CGUnit_C *unit);
  void        TalkToQuestUnit(const DWORDLONG &unit);
  int         QueryTaxiNodes(const DWORDLONG &unit);
  void        TalkToTrainer(const DWORDLONG &trainerUnit);
  void        TalkToBinder(const DWORDLONG &binder);
  void        TalkToBanker(const DWORDLONG &banker);
  void        TalkToNpcPetition(const DWORDLONG &vendor);
  void        TrainerBuySpell(const DWORDLONG &trainer, int spellID);
  void        TalkToTabardVendor(const DWORDLONG &tabardUnit);
  void        ReadItem(BYTE packSlot, BYTE slot);
  void        ReadItem(DWORDLONG containerGUID, BYTE slot);
  BOOL        DeathBindDistanceCompare(const NTempest::C3Vector &bindStonePosition);
  static const NTempest::C3Vector &GetBindPoint();
  BOOL                             GetLanguageSkill(UINT language, UINT &skill);
  UINT                             GetDefaultLanguage();
  const TSGrowableArray<int>      *GetTradeSkills(int skillLine) const;
  const TSGrowableArray<int>      *GetCraftSkills(SPELL_CAST_UI_TYPE type) const;
  int                              GetCraftSkillActivator(SPELL_CAST_UI_TYPE type) const;
  int                              GetSkillIndex(int skillID) const;
  int                              GetSkillRank(int skillID) const;
  void                             CheckWeaponDefenseRankChange() const;
  void                             CheckWeaponDefenseRankChange(COMBATHAND hand) const;
  int                              ValidateSlot(UINT slotID, DWORDLONG cursorItem);
  bool                             GetExpandedSkillRank(int skillID, int &rank, int &modifier) const;
  bool                             GetPackAndSlot(CGItem_C *item, BYTE &packSlot, BYTE &slot);
  CGBag_C                         *Inventory();
  const CGBag_C                   *Inventory() const;
  virtual CGBag_C                 *GetBag() {
    return &m_inventory;
  }
  virtual const CGBag_C *GetBag() const {
    return &m_inventory;
  }
  virtual void ItemReceived(const ItemStats *stats) const;
  void         IncrementPendingItemStats();
  void         DecrementPendingItemStats();
  void         FixComponenting(CGItem_C *item);

  BOOL IsInCombatMode() const {
    return (m_flags & 0x400) != 0;
  }

 private:
  void KillCombatModeTimer();
  UINT GetCombatModeTimerInterval() const;

 protected:
  void CheckWeaponRankChange() const;
  void CheckDefenseRankChange() const;
  void SetGuildMirrorHandler();
  void UnsetGuildMirrorHandler();
  void SetBankMirrorHandlers();
  void UnsetBankMirrorHandlers();

  UINT m_framesSinceUpdate;
  UINT m_flags;
  int  m_lastWeaponModeSent;

 protected:
  friend class CGGameUI;
  friend class CGWorldFrame;

  int GetWeaponSpell(COMBATHAND hand) const;

  TSHashTable<TRADESKILLLINE, HASHKEY_NONE> m_tradeSkillLines;
  TSGrowableArray<int>                      m_craftSpells[4];
  int                                       m_craftActivators[4];
  HMODEL                                    m_components[23][36];
  TexComponentInfo                          m_texComponentInfo[23];
  DWORDLONG                                 m_lootingUnit;
  DWORDLONG                                 m_lootingUnitSent;
  CGBag_C                                   m_inventory;
  DWORDLONG                                 m_lastKillerGUID;
  int                                       m_pendingItemStats;
};
class CreatureModelDataRec;

const CreatureModelDataRec *Player_C_GetModelName(UINT race, UINT sex);
UINT                        Player_C_GetDisplayId(UINT race, UINT sex);
int                         Player_C_AppFocusMovementHandler(int focus);
BOOL                        Player_C_ZoneUpdateHandler(LPCVOID eventData, LPVOID arg);
int                         Player_C_SetPlayerRender(int enable);
void                        Player_C_ClearGuildIDs();
void                        PlayerClientInitialize();
void                        PlayerClientShutdown();
