#pragma once

#include "Object/ObjectClient/Bag_C.h"
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"

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
  DWORDLONG       invSlots[PLAYER_INVENTORY_SLOTS];
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
  UINT GetGuildID() const {
    return m_plyr->guildID;
  }
  UINT GetGuildRank() const {
    return m_plyr->guildRank;
  }
  int GetXP() const {
    return m_plyr->XP;
  }
  int GetNextLevelXP() const {
    return m_plyr->nextLevelXP;
  }
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
    return (index >= 0 && index <= sizeof(m_plyr->questLog) / sizeof(m_plyr->questLog[0])) ? &m_plyr->questLog[index] : 0;
  }
  DWORDLONG GetSelection() const {
    return m_plyr->selection;
  }
  int       GetCharacterPoints(int index) const;
  UINT      GetCreatureTracking() const {
    return m_plyr->trackCreatureMask;
  }
  UINT GetResourceTracking() const {
    return m_plyr->trackResourceMask;
  }
  UINT GetPlayerFlags() const {
    return m_plyr->playerFlags;
  }
  int              GetPVPEnabled() const;
  BOOL             IsPartyLeader() const;
  BYTE GetNumBankSlots() const {
    return m_plyr->numBankSlots;
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
  BYTE IsDueling() const;
  const DWORDLONG &GetDuelArbiter() const {
    return m_plyr->duelArbiter;
  }
  UINT GetDuelTeam() const {
    return m_plyr->duelTeam;
  }
  int   GetBaseMana() const;
  BYTE *GetData(UINT index);

  static UINT               GetDataSize();
  static UINT               GetBaseOffset();
  static __forceinline UINT TotalFields() {
    return 634;
  }
  static UINT GetUpdateMaskBytes();
  static UINT GetUpdateMaskBlocks();


  void SetStorage(DWORD *storage) {
    m_plyr = (CGPlayerData *)storage;
  }

 protected:
  explicit CGPlayer(DWORD *storage) : m_plyr((CGPlayerData *)storage) {
  }

  ~CGPlayer() {
  }

  const CGPlayerData *Player() const {
    return m_plyr;
  }

  CGPlayerData *Player() {
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
  friend bool Spell_C_CastSpell(int spellID, const CGItem_C *item);
  friend bool Spell_C_HaveSpellTokens(CGPlayer_C *player, const SpellRec *spell, bool report);
  friend bool Spell_C_HaveEquippedSpellItems(CGPlayer_C *player, const SpellRec *spell, bool checkAmmo, bool report);
  friend class CGGameUI;
  friend class CGWorldFrame;

 public:
  CGPlayer_C(DWORD *storage, DWORD eventTime, CClientObjCreate *init);
  ~CGPlayer_C();

  void         SetStorage(DWORD *storage);
  void         SetActiveMirrorHandlers();
  void         PostInit(const CClientObjCreate &init);
  virtual void PostReenable();
  virtual void Disable(int shutdown);
  virtual void Reenable();
  static void  SellItem(DWORDLONG merchant, DWORDLONG item, UINT amount);
  static DWORDLONG       GetActive() {
    return ClntObjMgrGetActivePlayer();
  }
  static void       SetActive(const CGPlayer_C *playerPtr);
  static void       TogglePlayerBounds();
  static UINT       GetProficiency(BYTE type);
  int               SwapInventorySlots(int slotA, int slotB);
  BOOL              ReportBagItemSubtypeMismatch(BYTE bagSlot) const;
  BOOL              OnTerrainClick(const CTerrainClickEvent &);
  void              OnUnitDeath(DWORDLONG guid);
  void              OnObjectDestruct(DWORDLONG guid);
  void              SaveDeathMessage(DWORDLONG guid);
  void              CheckKillerFeedback();
  virtual void      OnAttackStart(DWORDLONG victim);
  virtual void      OnAttackStop(DWORDLONG previousTarget, int nowDead);
  BOOL              CanEngageTarget(const CGUnit_C *unitPtr);
  virtual void      CombatLoggingFlagChanged();
  void              PlayerFlagsChanged(BYTE oldFlags);
  virtual void      SetEmoteState(UINT emoteID);
  void              SendTextEmote(const EmotesTextRec *rec, const DWORDLONG &target) const;
  virtual void      SetTorsoAnimState(UINT newState);
  virtual void      SetBaseAnimState(UINT newState);
  virtual BOOL      ShouldRenderUnitName(UINT mode) const;
  virtual void      CommitTexture(int force);
  virtual UINT      UpdateUnitNameString(UINT localPlayerFlags, UINT otherUnitsFlags, char *buffer, UINT bufferSize) const;
  virtual void      GetAFKText(char *buffer, int size) const;
  virtual void      GetDNDText(char *buffer, int size) const;
  virtual void      GetGMText(char *buffer, int size) const;
  virtual void      OnBadAttackFacing(DWORDLONG victim);
  virtual void      OnBadAttackPosition(DWORDLONG victim, float range);
  virtual void      OnBadAttackTarget(DWORDLONG victim);
  virtual void      OnNotStanding(DWORDLONG victim);
  virtual void      OnDeath();
  virtual void      OnDeathAnimate();
  void              HandleRepopRequest();
  void              MoveItem(DWORDLONG item, DWORDLONG itemContainer, UINT slot, DWORDLONG newContainer, UINT newSlot);
  void              SwapItems(DWORDLONG cursorItem, DWORDLONG cursorContainer, int cursorSlot, DWORDLONG containerB, int slotB, int force);
  void              SplitItem(DWORDLONG cursorItem, DWORDLONG cursorContainer, int cursorSlot, DWORDLONG containerB, int slotB, int quantity);
  void              DropItemInCursor(DWORDLONG cursorItem, DWORDLONG cursorItemPack, UINT cursorSlot);
  void              AutoStoreItemInBag(DWORDLONG cursorItem, DWORDLONG cursorContainer, int cursorSlot, DWORDLONG containerB, int ignoreOwnershipRules);
  void              AutoEquipCursorItem(int force);
  void              AutoEquipItem(DWORDLONG container, UINT slot, int force);
  void              AutoStoreLootItem(BYTE slot);
  void              PutLootInSlot(DWORDLONG container, BYTE containerSlot, BYTE lootSlot);
  void              PutLootInBag(DWORDLONG container, BYTE lootSlot);
  BYTE              FindSlotIndex(DWORDLONG obj);
  void              ClearPendingEquip(UINT index, int equip);
  BOOL              HasEquipped(int classID, int subclassID);
  int               LootUnit(CGUnit_C *unit);
  BOOL              OnLootResponse(UINT eventTime, CDataStore *msg);
  BOOL              OnLootReleaseResponse(CDataStore *msg);
  BOOL              OnLootRemoved(CDataStore *msg);
  BOOL              OnLootMoneyNotify(CDataStore *msg);
  BOOL              OnLootClearMoney(CDataStore *msg);
  BOOL              OnLootItemNotify(CDataStore *msg);
  virtual void      LootAnimEndHandler();
  BOOL              CanLoot(CGUnit_C *unitPtr);
  virtual DWORDLONG GetUnitBeingLooted() const;
  const DWORDLONG  &GetUnitLootingSent() const;
  UINT              GetPlayerAnimState();
  void              SheatheWeapon(bool sheathe);
  void              TrySheathingWeapon();
  void              AttachObjComponent(DWORDLONG item, UINT slot, bool defer, bool sheathe, int sheatheAttachmentSlot);
  void              AddComponent(int displayID, UINT inventoryType, int slot, int commit);
  void              RemoveComponent(int slot, bool commitItemGeosets, bool defer, bool removeRecord);
  void              LootMoney();
  BOOL              CanUseItem(const ItemStats *stats, GAME_ERROR_TYPE &reason);
  BOOL              InviteToGroup(DWORDLONG target);
  void              InviteToGroup(LPCSTR target);
  int               Uninvite(DWORDLONG target);
  void              Uninvite(LPCSTR target);
  BOOL              SetNewLeader(DWORDLONG target);
  void              SetNewLeader(LPCSTR target);
  void              AcceptGroup();
  void              DeclineGroup();
  void              LeaveGroup();
  void              SetLootMethod(LOOT_METHOD method, DWORDLONG master);
  void              AcceptGuild();
  void              DeclineGuild();
  BOOL              SetBlock(UINT index, DWORD data);
  void              SetData(LPCVOID data, UINT bytes);
  static UINT       OffsetOf(OBJECT_TYPE_ID type);
  virtual LPCSTR    GetModelFileName() const;
  static void       Initialize();
  static void       Shutdown();
  static void       XBuyItem(DWORDLONG merchant, UINT itemID, BYTE quantity, bool autoEquip);
  static void       XBuyItemInSlot(DWORDLONG merchant, UINT itemID, BYTE quantity, DWORDLONG container, BYTE slot);
  static void       XBuyItemInBag(DWORDLONG merchant, UINT itemID, BYTE quantity, DWORDLONG container);
  BOOL              OnVendorInventory(CDataStore *msg);
  BOOL              OnQuestGiverListQuests(CDataStore *msg);
  BOOL              OnQuestGiverInvalidQuest(CDataStore *msg);
  BOOL              OnQuestGiverSendQuest(CDataStore *msg);
  BOOL              OnQuestGiverRequestItems(CDataStore *msg);
  BOOL              OnQuestGiverChooseReward(CDataStore *msg);
  BOOL              OnQuestGiverQuestComplete(CDataStore *msg);
  BOOL              OnQuestGiverQuestFailed(CDataStore *msg);
  BOOL              OnQuestGiverStatus(CDataStore *msg);
  BOOL              OnTrainerList(CDataStore *msg);
  BOOL              OnBuyFailed(CDataStore *msg);
  BOOL              OnBuySucceeded(CDataStore *msg);
  BOOL              OnSellResponse(CDataStore *msg);
  void              QueryQuest(const DWORDLONG &questGiver, int questID);
  void              AcceptQuest(const DWORDLONG &questGiver, int questID);
  void              CompleteQuest(const DWORDLONG &questGiver, int questID);
  void              GiveQuestItems(const DWORDLONG &questGiver, int questID);
  void              GetQuestReward(const DWORDLONG &questGiver, int questID, int itemChoice);
  void              CancelQuest(const DWORDLONG &questGiver);
  void              QuestLogRemoveQuest(int entry);
  void              QuestLogSwapQuest(int entry1, int entry2);
  void              UpdateQuestStatus(const DWORDLONG &guid);
  void              UpdateQuestStatus(CGUnit_C *unit);
  static void       UpdateQuestStatusAll();
  void              UpdateTaxiStatus(CGUnit_C *unit);
  static void       UpdateTaxiStatusAll();
  void              UpdateBindStatus(CGUnit_C *unit);
  static void       UpdateBindStatusAll();
  void              TrainerBuySpell(const DWORDLONG &trainer, int spellID);
  void              OnSpellFailed(const SpellRec *spellRec, UINT reason);
  void              RequestPetitionSignatures(DWORDLONG item);
  void              BuyPetition(const DWORDLONG &petitionUnit, CGPetition *petition);
  void              TurnInGuildCharter();
  BOOL              OnPetitionShowSignatures(CDataStore *msg);
  BOOL              OnPetitionShowList(CDataStore *msg);
  BOOL              OnSignedResults(CDataStore *msg);
  BOOL              OnTurnInPetitionResults(CDataStore *msg);
  void              PlayMacroSound(int category) const;
  virtual void      PlayUnitSound(UNITSOUNDTYPE soundType, int alwaysPlay) const;
  virtual void      PlayFoleySound() const;
  virtual void      HandleSpellEventSound();
  void              PlayVocalMacro(int category);
  virtual void      PlayDeathThudCameraShake() const;

 protected:
  virtual UINT GetImpactType() const;

 public:
  void                        DeleteWornItems() const;
  UINT                        GetFramesSinceUpdate();
  void                        SkipUpdate();
  void                        UpdateText();
  void                        InspectPlayer(const DWORDLONG &guid);
  void                        ReceiveResurrectRequest(LPCSTR name);
  void                        AcceptResurrectRequest(int accept);
  virtual int                 GetSpellCastingTime(int spellID) const;
  void                        AddKnownSpell(int spellID, int slot, int learned, int addToBook);
  void                        DelKnownSpell(int spellID);
  const TSGrowableArray<int> *GetTradeSkills(int skillLine) const;
  const TSGrowableArray<int> *GetCraftSkills(SPELL_CAST_UI_TYPE type) const;
  int                         GetCraftSkillActivator(SPELL_CAST_UI_TYPE type) const;
  int                         GetSkillIndex(int skillID) const;
  bool                        GetExpandedSkillRank(int skillID, int &rank, int &modifier) const;
  int                         GetSkillRank(int skillID) const;
  virtual int                 GetSpellRank(int spellID) const;
  virtual bool                GetDefenseSkillRank(int &base, int &modifier) const;
  virtual bool                GetAttackSkillRank(int hand, int &base, int &modifier) const;
  static UINT                 GetNewContinentID();
  int                         ValidateSlot(UINT slotID, DWORDLONG cursorItem);
  static ITEMEXPIRATION      *GetPendingItemExpirationNode(const DWORDLONG &itemGUID);
  static void                 UpdatePendingItemExpiration(const DWORDLONG &itemGUID);
  virtual void                UpdateObjComponentVisuals(const CGItem_C *item, const ItemEnchantment *enchantments, int num);
  virtual void                ClearItemVisuals(ACTIVEATTACHMENTINFO *info);
  virtual void                SetItemVisuals(ACTIVEATTACHMENTINFO *info, const ItemVisualsRec *rec, bool force);
  BOOL                        OnAttackBreakHandler();
  BOOL                        OnAttackIconPressed();
  void                        SetCombatMode(int state);

  BOOL IsInCombatMode() const {
    return (m_flags & 0x400) != 0;
  }
  virtual void      OnAttackerStateChange(const ATTACKROUNDINFO &roundInfo);
  virtual void      HandleMirrorTimerDamage(const MIRRORTIMERDAMAGE &log);
  void              KillExitCombatModeSheatheTimer();
  virtual DWORDLONG GetLocalTarget() const;
  virtual void      UnitHit(VICTIMSTATES state, DWORDLONG attacker);
  void              ResetCombatModeTimer(int newCombat);

 private:
  void KillCombatModeTimer();
  UINT GetCombatModeTimerInterval() const;

 public:
  static void  OnItemDelete(DWORDLONG item, DWORDLONG listener);
  void         OnItemDelete(DWORDLONG item);
  void         SetLootCloseSentFlag();
  void         ToggleSheathe(bool ignoreAnim);
  void         StartSheatheAnim(INVENTORY_SLOTS slot, int hip, int both);
  virtual void SetLastWeaponModeSent(int mode);

 protected:
  UINT m_framesSinceUpdate;
  UINT m_flags;
  int  m_lastWeaponModeSent;

  TSHashTable<TRADESKILLLINE, HASHKEY_NONE> m_tradeSkillLines;
  TSGrowableArray<int>                      m_craftSpells[4];
  int                                       m_craftActivators[4];
  HMODEL                                    m_components[NUM_INVENTORY_SLOTS][36];
  TexComponentInfo                          m_texComponentInfo[NUM_INVENTORY_SLOTS];

  virtual UINT DetermineWoundSequence() const;

 private:
  void        SetInventoryMirrorHandler(UINT slot, int (*handler)(DWORDLONG, UINT, UINT, LPCVOID, LPVOID));
  void        UnsetInventoryMirrorHandler(UINT slot, int (*handler)(DWORDLONG, UINT, UINT, LPCVOID, LPVOID));
  void        SetPlayerMirrorHandlers();
  void        UnsetPlayerMirrorHandlers();
  void        UnsetActiveMirrorHandlers();
  void        InitPreferredGeosets();
  void        InitComponents();
  CGPlayer_C &operator=(const CGPlayer_C &);

 public:
  BOOL IsQuestUnit(CGUnit_C *unit);
  void ShopFromMerchant(const DWORDLONG &merchant);
  void TalkToQuestUnit(const DWORDLONG &unit);
  void TalkToTrainer(const DWORDLONG &trainerUnit);
  void TalkToBinder(const DWORDLONG &binder);
  void TalkToBanker(const DWORDLONG &banker);
  void TalkToTabardVendor(const DWORDLONG &tabardUnit);
  void TalkToNpcPetition(const DWORDLONG &vendor);

 protected:
  DWORDLONG m_lootingUnit;
  DWORDLONG m_lootingUnitSent;

 public:
  void SaveTabard(int eStyle, int eColor, int bStyle, int bColor, int bg, DWORDLONG vendor) const;
  bool OnGuildChanged();
  void GuildInfoLoaded(const TSGrowableArray<UINT> &guildList);

 protected:
  void SetGuildMirrorHandler();
  void UnsetGuildMirrorHandler();

  CGBag_C m_inventory;

 public:
  virtual CGBag_C                 *GetBag() {
    return &m_inventory;
  }
  virtual const CGBag_C *GetBag() const {
    return &m_inventory;
  }
  CGBag_C *Inventory() {
    return &m_inventory;
  }
  const CGBag_C *Inventory() const {
    return &m_inventory;
  }
  virtual const VirtualItemInfo *GetVirtualItem(UINT slot, bool ignoreDisarmFlag) const;
  virtual int                    GetVirtualItemDisplayID(UINT slot) const;
  virtual const VirtualItemInfo *GetDefendingItem() const;
  void                           ReadItem(BYTE packSlot, BYTE slot);
  void                           ReadItem(DWORDLONG containerGUID, BYTE slot);
  void                           ReadItemResult(NETMESSAGE msgID, CDataStore *msg);
  virtual void                   OnMount();
  virtual void                   OnDismount();
  void                           HandleMountResult(UINT result);
  void                           HandleDismountResult(UINT result);
  virtual float                  GetMountScale() const;
  virtual bool                   CanBeMounted();
  BOOL                           GetLanguageSkill(UINT language, UINT &skill);
  UINT                           GetDefaultLanguage();
  virtual BOOL                   ShouldRender(DWORD worldStatus);
  virtual void                   PreAnimate(CGWorldFrame *worldFrame);
  void                           OnTaxiNodeStatus(CDataStore *msg);
  void                           ShowTaxiNodes(CDataStore *msg);
  int                            QueryTaxiNodes(const DWORDLONG &unit);
  void                           StartTaxi(DWORDLONG vendor, UINT startNode, UINT destNode);
  void                           HandleActivateTaxiReply(UINT code);
  bool                           CanTrack(const CGUnit_C *unit);
  bool                           CanTrack(const CGGameObject_C *object);

 protected:
  virtual void CleanupUnitArtwork(int playerModelChanged, BOOL wasPlayerModel);
  virtual void ReinitializeUnitArtwork();
  virtual void PostReinitializeArtwork();

 public:
  BOOL                             DeathBindDistanceCompare(const NTempest::C3Vector &bindStonePosition);
  static void                      SaveBindPoint(CDataStore *msg);
  static const NTempest::C3Vector &GetBindPoint();
  virtual void                     ChangeStandState(UINT standState);
  virtual void                     OnStandStateChanged(UINT oldState, UINT newState);
  UINT                    GetDisplayRace() const {
    return m_unit->race;
  }
  UINT GetDisplaySex() const {
    return m_unit->sex;
  }
  void                    OnLootGameObject(const DWORDLONG &gameObject, bool lootAnim);
  void                    ClearLootingUnitSent();
  CGItem_C               *GetSoulstone() const;
  void                    UseSoulstone() const;
  void                    FixComponenting(CGItem_C *item);
  virtual void            ItemReceived(const ItemStats *stats) const;
  virtual UNITAFFILIATION GetGUIDAffiliation(DWORDLONG unit) const;
  virtual void            OnLevelChange();
  void                    CheckWeaponDefenseRankChange(COMBATHAND hand) const;
  void                    CheckWeaponDefenseRankChange() const;
  bool                    GetPackAndSlot(CGItem_C *item, BYTE &packSlot, BYTE &slot);
  void                    OpenLootItem(CGItem_C *item);
  void                    OpenWrappedItem(CGItem_C *item);
  static UINT             GetLootItem(UINT slot);
  static UINT             GetLootItemDisplayID(UINT slot);
  static UINT             GetLootItemQuantity(UINT slot);
  virtual float           GetBlockChance() const;
  virtual float           GetDodgeChance() const;
  virtual float           GetParryChance() const;

 protected:
  void CheckWeaponRankChange() const;
  void CheckDefenseRankChange() const;

  int GetWeaponSpell(COMBATHAND hand) const;

  DWORDLONG m_lastKillerGUID;

  void SetBankMirrorHandlers();
  void UnsetBankMirrorHandlers();

 public:
  BOOL        OnSplitMoneyNotify(CDataStore *msg);
  void        IncrementPendingItemStats();
  void        DecrementPendingItemStats();
  static void StartGiftWrap(CGItem_C *wrapper);
  static void CancelGiftWrap();
  static bool IsGiftWrapping();
  BYTE        FindItemSlot(DWORDLONG containerGUID, CGItem_C *item);
  void        GiftWrap(CGItem_C *item);
  void        BotMove(DWORD now, NTempest::C3Vector *points, int count, DWORD duration, UINT flags);
  int         BotSpline();
  void        SetFarSightFocus(CGObject_C *obj);
  void        ToggleFarSight();
  void        ClearFarSight();
  BOOL             IsInFarSight() {
    return (m_flags & 0x800) != 0;
  }
  CGUnit_C        *GetPossessedUnit();
  static void      InstallGMHandlers();
  static void      UninstallGMHandlers();
  static void      StartGhosting(LPCSTR name);
  static void      StartGhosting(DWORDLONG guid);
  static void      StopGhosting();
  static void      GMIdle();
  static void      SetRealActivePlayer(DWORDLONG guid);
  static DWORDLONG GetRealActivePlayer();

 protected:
  int m_pendingItemStats;

 public:
  static void AddDeferredDamage(int normal, UINT flags, UINT damage, DWORDLONG victim);
  static void AddDeferredSpellMiss(DWORDLONG victim, MISS_REASON reason, int spellID);
  static void ProcessDeferredDamage();
  static void ProcessDeferredSpellMiss();
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
