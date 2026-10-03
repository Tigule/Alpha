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

#include "Item_C.h"

#include "Object/ObjectClient/Player_C.h"
#include "Magic/MagicClient/Spell_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Ui/ActionBarFrame.h"
#include "Ui/ContainerFrame.h"
#include "Ui/GameUI.h"
#include "Ui/ItemTextFrame.h"
#include "Ui/LootFrame.h"
#include "Ui/PaperDollInfoFrame.h"
#include "Ui/QuestLog.h"
#include "Ui/TradeFrame.h"
#include "Ui/Tutorial.h"
#include "Net/NetClient/NetClient.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <Base/CDataStore.h>
#include <Base/Status.h>
#include <DB/DBClient/DBClient.h>
#include <DB/DBClient/DBCacheInstances.h>
#include <DB/DBClient/AutoCode/ItemDisplayInfoRec.h>
#include <DB/DBClient/AutoCode/ItemGroupSoundsRec.h>
#include <DB/DBClient/AutoCode/MaterialRec.h>
#include <Services/SysMessage.h>
#include <Services/Texture.h>
#include <Object/ItemStats.h>
#include <Os/OsTime.h>
#include <stpl.h>
#include <Tempest/cimvector.h>
#include <stddef.h>

extern const int g_ITEMTYPEARRAY[];

bool             Spell_C_CastSpell(int spellID, const CGItem_C *item);
void             Spell_C_CancelSpell(bool failed, bool notifyServer, SPELL_FAILED_REASON reason);
const DWORDLONG &Spell_C_GetCurrentCaster();
void             ClntObjMgrHideObject(DWORDLONG guid);
void             ClntObjMgrShowObject(DWORDLONG guid);

struct TradeSkillInfo;
struct TradeSkillSubClassInfo;
struct CraftInfo;
struct CraftSkillLineInfo;

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

struct INVENTORYART : public TSHashObject<INVENTORYART, HASHKEY_NONE> {
  char *textureName;

  INVENTORYART() : textureName(0) {
  }

  INVENTORYART(const INVENTORYART &other) : textureName(0) {
    SetArt(other.textureName);
  }

  const INVENTORYART &operator=(const INVENTORYART &other) {
    SetArt(other.textureName);
    return *this;
  }

  ~INVENTORYART() {
    Clear();
  }

  void Clear() {
    if (textureName) {
      SMemFree(textureName, __FILE__, __LINE__, 0);
    }
    textureName = 0;
  }

  void SetArt(LPCSTR art) {
    if (art && *art) {
      Clear();
      textureName = SStrDupA(art, __FILE__, __LINE__);
    }
  }
};

static TSHashTable<INVENTORYART, HASHKEY_NONE> s_inventoryTextures;
static HASHKEY_NONE                            s_nullHashKey;

static BOOL OnUpdateOwner(DWORDLONG guid, UINT offset, UINT bytes, LPCVOID prevValue, LPVOID param) {
  CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  FATALASSERT(item);
  DWORDLONG currOwner = item->GetOwner();
  if (!*static_cast<const DWORDLONG *>(prevValue) && currOwner) {
    ClntObjMgrHideObject(guid);
    item->RemoveWorldObject();
  } else if (*static_cast<const DWORDLONG *>(prevValue) && !currOwner) {
    ClntObjMgrShowObject(guid);
    item->AddWorldObject();
  }
  if (*static_cast<const DWORDLONG *>(prevValue) == ClntObjMgrGetActivePlayer() && currOwner != ClntObjMgrGetActivePlayer()) {
    CGActionBar::UpdateItem(item->GetEntryID());
    CGTradeInfo::RemovePlayerItem(item->GetGUID());
  }
  if (*static_cast<const DWORDLONG *>(prevValue) == ClntObjMgrGetActivePlayer() || currOwner == ClntObjMgrGetActivePlayer()) {
    CGTradeSkillInfo::RefreshList(0);
    CGCraftInfo::RefreshList();
    CGQuestLog::Update(0);
  }
  return 1;
}

static BOOL OnUpdateStackCount(DWORDLONG guid, UINT offset, UINT bytes, LPCVOID prevValue, LPVOID param) {
  CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  FATALASSERT(item);
  if (item->GetOwner() == ClntObjMgrGetActivePlayer()) {
    CGGameUI::UnlockItem(item->GetGUID());
    CGTradeSkillInfo::RefreshList(0);
    CGCraftInfo::RefreshList();
    CGActionBar::UpdateItem(item->GetEntryID());
    CGQuestLog::Update(0);
    CGTradeInfo::UpdatePlayerItem(item->GetGUID());
    CGContainerInfo::UpdateItem(item->GetGUID());
    CGCharacterInfo::UpdateItem(item->GetGUID());
  }
  return 1;
}

static BOOL OnUpdateEnchantments(DWORDLONG guid, UINT, UINT, LPCVOID, LPVOID) {
  CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (item) {
    item->UpdateEnchantments();
  }
  return 1;
}

static void ItemIDChangedCacheCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted) {
  CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (item && item->GetOwner() == ClntObjMgrGetActivePlayer()) {
    CGContainerInfo::UpdateContents(item->GetContainedIn());
  }
}

static BOOL OnUpdateItemID(DWORDLONG guid, UINT offset, UINT bytes, LPCVOID prevValue, LPVOID param) {
  CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  FATALASSERT(item);
  FATALASSERT(prevValue);
  if (item->GetOwner() == ClntObjMgrGetActivePlayer()) {
    if (g_itemDBCache.GetRecord(item->GetEntryID(), item->GetGUID(), ItemIDChangedCacheCallback, 0)) {
      CGContainerInfo::UpdateContents(item->GetContainedIn());
    }
  }
  return 1;
}

static void AddInventoryArtHash(UINT displayID, LPCSTR fileName) {
  if (!s_inventoryTextures.Ptr(displayID, s_nullHashKey)) {
    INVENTORYART *entry = s_inventoryTextures.New(displayID, s_nullHashKey, 0, 0);
    entry->SetArt(fileName);
  }
}

static LPCSTR GetInventoryArtHash(UINT displayID) {
  INVENTORYART *entry = s_inventoryTextures.Ptr(displayID, s_nullHashKey);
  return entry ? entry->textureName : 0;
}

void CGItem_C::SetStorage(DWORD *storage) {
  CGObject_C::SetStorage(storage);
  CGItem::SetStorage(storage + CGObject::TotalFields());
}

CGItem_C::CGItem_C(DWORD *storage, DWORD eventTime, CClientObjCreate *init)
    : CGObject_C(storage, eventTime, init), CGItem(storage + CGObject::TotalFields()), m_flags(0), m_expirationTime(0), m_soundsRec(0) {
  if (m_item->m_owner) {
    ClntObjMgrHideObject(GetGUID());
  } else {
    AddWorldObject();
  }

  m_itemInfo.m_classID = static_cast<BYTE>(GetClassID());
  m_itemInfo.m_subclassID = static_cast<BYTE>(GetSubtypeID());
  m_itemInfo.m_material = static_cast<BYTE>(GetMaterial());
  m_itemInfo.m_inventoryType = static_cast<BYTE>(GetInventoryType());
  m_itemInfo.m_sheatheType = static_cast<BYTE>(GetSheatheType());

  memset(m_enchantmentExpiration, 0, sizeof(m_enchantmentExpiration));
}

void CGItem_C::InstallObjMirrorHandlers() {
  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_ITEM) + offsetof(CGItemData, m_owner), sizeof(m_item->m_owner), OnUpdateOwner, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_ITEM) + offsetof(CGItemData, m_stackCount), sizeof(m_item->m_stackCount), OnUpdateStackCount, 0, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(GetGUID(), OffsetOf(ID_ITEM) + offsetof(CGItemData, m_enchantment), sizeof(m_item->m_enchantment), OnUpdateEnchantments, 0, HANDLER_PRIORITY_NORMAL);
}

static void LoadItemCacheCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted) {
  CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (!item) {
    return;
  }

  CGPlayer_C *owner = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(item->GetOwner(), __FILE__, __LINE__));

  if (!granted) {
    if (owner) {
      owner->DecrementPendingItemStats();
    }
  } else {
    CGContainerInfo::UpdateContents(item->GetContainedIn());
    item->PostInitWithStats();

    if (owner) {
      const ItemStats *stats = g_itemDBCache.GetRecord(item->GetEntryID(), 0, 0, 0);
      owner->FixComponenting(item);
      owner->ItemReceived(stats);
      owner->UpdateReadyAnim(stats);
    }
  }
}

void CGItem_C::PostInit(const CClientObjCreate &init) {
  CGObject_C::PostInit(init);

  if (g_itemDBCache.GetRecord(GetEntryID(), GetGUID(), LoadItemCacheCallback, 0)) {
    PostInitWithStats();
  } else {
    CGPlayer_C *owner = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(GetOwner(), __FILE__, __LINE__));
    if (owner) {
      owner->IncrementPendingItemStats();
    }
  }
}

void CGItem_C::PostInitWithStats() {
  const ItemStats *stat = g_itemDBCache.GetRecord(GetEntryID(), 0, 0, 0);
  FATALASSERT(stat);

  InstallObjMirrorHandlers();
  InstallItemIDMirrorHandler();

  const ItemDisplayInfoRec *displayInfo = g_itemDisplayInfoDB.GetRecord(GetDisplayID());
  if (displayInfo) {
    m_soundsRec = g_itemGroupSoundsDB.GetRecord(displayInfo->m_groupSoundIndex);
  } else {
    m_soundsRec = 0;
  }

  if (GetOwner() == ClntObjMgrGetActivePlayer()) {
    CGPlayer_C::UpdatePendingItemExpiration(GetGUID());
    CGTradeSkillInfo::RefreshList(0);
    CGCraftInfo::RefreshList();
    CGActionBar::UpdateItem(GetEntryID());
    CGQuestLog::Update(0);
    CGContainerInfo::UpdateItem(GetGUID());

    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(GetOwner(), __FILE__, __LINE__));
    if (player) {
      if (stat) {
        player->ItemReceived(stat);
      }
      if (player->GetBag()->GetIndexOfObject(GetGUID()) >= 23 && player->GetBag()->GetIndexOfObject(GetGUID()) <= 38) {
        CGTutorial::TriggerTutorial(TUTORIAL_ITEMS);
        if (CanBeUsed()) {
          CGTutorial::TriggerTutorial(TUTORIAL_USABLE_ITEMS);
        }
        if (GetBag()) {
          CGTutorial::TriggerTutorial(TUTORIAL_BAGS);
        }
      }
    }
  }

  m_itemInfo.m_classID = static_cast<BYTE>(GetClassID());
  m_itemInfo.m_subclassID = static_cast<BYTE>(GetSubtypeID());
  m_itemInfo.m_material = static_cast<BYTE>(GetMaterial());
  m_itemInfo.m_inventoryType = static_cast<BYTE>(GetInventoryType());
  m_itemInfo.m_sheatheType = static_cast<BYTE>(GetSheatheType());
}

void CGItem_C::Disable(int shutdown) {
  if (CGLootInfo::GetObject() == GetGUID()) {
    CGGameUI::CloseLoot(1, 0);
  }

  if (Spell_C_GetCurrentCaster() == GetGUID()) {
    Spell_C_CancelSpell(1, 0, SPELL_FAILED_ERROR);
  }

  UninstallItemIDMirrorHandler();
  RemoveWorldObject();

  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_ITEM) + offsetof(CGItemData, m_owner), OnUpdateOwner, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_ITEM) + offsetof(CGItemData, m_stackCount), OnUpdateStackCount, 0);

  if (GetGUID() == CGGameUI::GetCursorItem()) {
    CGGameUI::ClearCursor(1);
  }
  if (GetGUID() == CGItemText::GetItem()) {
    CGItemText::SetItem(0, 0);
  }

  BOOL updateUI = !shutdown && GetOwner() == ClntObjMgrGetActivePlayer();
  CGObject_C::Disable(shutdown);
  if (updateUI) {
    CGTradeSkillInfo::RefreshList(0);
    CGCraftInfo::RefreshList();
    CGActionBar::UpdateItem(GetEntryID());
    CGTradeInfo::RemovePlayerItem(GetGUID());
    CGQuestLog::Update(0);
  }
}

void CGItem_C::Reenable() {
  CGObject_C::Reenable();
  if (!m_item->m_owner) {
    AddWorldObject();
    InstallObjMirrorHandlers();
    InstallItemIDMirrorHandler();
  }
}

CGItem_C::~CGItem_C() {
}

LPCSTR CGItem_C::GetInventoryArt(int displayID) {
  LPCSTR inventoryArt = GetInventoryArtHash(displayID);
  if (inventoryArt) {
    return inventoryArt;
  }

  const ItemDisplayInfoRec *displayInfo = g_itemDisplayInfoDB.GetRecord(displayID);
  if (displayInfo && displayInfo->m_inventoryIcon && *displayInfo->m_inventoryIcon) {
    char buffer[MAX_PATH];
    inventoryArt = displayInfo->m_inventoryIcon;
    TEXFILETYPE type = TextureDiscoverFileType(inventoryArt);
    if (type == TEXFILETYPE_TGA) {
      TexturePickAlternateFilename(inventoryArt, type, buffer, sizeof(buffer));
      inventoryArt = buffer;
    }

    AddInventoryArtHash(displayID, inventoryArt);
    return GetInventoryArtHash(displayID);
  }

  SysMsgPrintf(SYSMSG_ERROR, 2, "NOINVENTORYICON|%d", displayID);
  return "INV_Misc_QuestionMark";
}

LPCSTR CGItem_C::GetInventoryArt() const {
  return GetInventoryArt(GetDisplayID());
}

LPCSTR CGItem_C::GetModelFileName() const {
  return 0;
}

BOOL CGItem_C::CanBeUsed() {
  const ItemStats_C *stats = g_itemDBCache.GetRecord(GetEntryID(), ClntObjMgrGetActivePlayer(), 0, 0);
  if (!stats) {
    return 0;
  }
  UINT index;
  for (index = 0; index < 5; ++index) {
    if (stats->m_spellID[index] && !stats->m_spellTrigger[index]) {
      break;
    }
  }
  return index < 5;
}

int CGItem_C::GetUseSpell() {
  int                spellID = 0;
  const ItemStats_C *stats = g_itemDBCache.GetRecord(GetEntryID(), 0, 0, 0);
  if (!stats) {
    return 0;
  }
  for (UINT index = 0; index < 5; ++index) {
    if (stats->m_spellID[index] && !stats->m_spellTrigger[index]) {
      spellID = stats->m_spellID[index];
      break;
    }
  }
  return spellID;
}

bool CGItem_C::Use() {
  if (IsA(ID_CONTAINER) || IsLocked()) {
    return false;
  }

  const ItemStats *stats = g_itemDBCache.GetRecord(GetEntryID(), 0, 0, 0);
  if (!stats) {
    return false;
  }

  CGPlayer_C     *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  GAME_ERROR_TYPE reason = GERR_NUM_TYPES;
  if (player && !player->CanUseItem(stats, reason)) {
    if (reason == GERR_CANT_EQUIP_LEVEL_I) {
      CGGameUI::DisplayError(GERR_CANT_EQUIP_LEVEL_I, stats->m_requiredLevel);
    } else {
      CGGameUI::DisplayError(reason);
    }
    return false;
  }

  if (stats->m_pageText) {
    CGItemText::SetItem(GetGUID(), 0);
    return false;
  }
  if (stats->m_startQuestID) {
    player->QueryQuest(GetGUID(), stats->m_startQuestID);
    return false;
  }
  if (stats->m_flags & ITEM_FLAG_HAS_LOOT) {
    player->OpenLootItem(this);
    return false;
  }
  if (stats->m_flags & ITEM_FLAG_IS_WRAPPER) {
    if (IsWrapped()) {
      player->OpenWrappedItem(this);
    } else {
      CGPlayer_C::StartGiftWrap(this);
    }
    return false;
  }
  if (stats->m_flags & ITEM_FLAG_PETITION) {
    player->RequestPetitionSignatures(GetGUID());
    return false;
  }

  if (!CanBeUsed()) {
    return false;
  }
  if (GetInventoryType()) {
    if (GetContainedIn() != ClntObjMgrGetActivePlayer()) {
      CGGameUI::DisplayError(GERR_MUST_EQUIP_ITEM);
      return false;
    }
    if (player && player->GetBag()->GetIndexOfObject(GetGUID()) >= 23) {
      CGGameUI::DisplayError(GERR_MUST_EQUIP_ITEM);
      return false;
    }
  }

  return Spell_C_CastSpell(GetUseSpell(), this);
}

BOOL CGItem_C::SetBlock(UINT i, DWORD data) {
  if (i < CGObject::TotalFields()) {
    return CGObject_C::SetBlock(i, data);
  }

  i -= CGObject::TotalFields();
  FATALASSERT(i < (CGItem::GetDataSize()/sizeof(DWORD)));
  reinterpret_cast<DWORD *>(&m_item)[i] = data;
  return 1;
}

void CGItem_C::SetData(LPCVOID data, UINT bytes) {
  FATALASSERT(bytes <= sizeof(*m_item));
  memcpy(m_item, data, bytes);
}

UINT CGItem_C::OffsetOf(OBJECT_TYPE_ID type) {
  switch (type) {
    case ID_OBJECT:
      return 0;
    case ID_ITEM:
      return CGObject::TotalFields() * sizeof(DWORD);
    default:
      FATALASSERT(0);
      return static_cast<UINT>(-1);
  }
}

void CGItem_C::Initialize() {
  if (ClientDBStringLookup(SLOOKUP_INVENTORYICONBUTTONGEOMETRY)) {
    CStatus status;

    SysMsgAdd(status, 0x10);
  }
}

void CGItem_C::Shutdown() {
  s_inventoryTextures.Clear();
}

BOOL CGItem_C::IsMetal() const {
  return IsMetal(GetMaterial());
}

BOOL CGItem_C::IsMetal(UINT material) {
  const MaterialRec *rec = g_materialDB.GetRecord(material);
  return rec && (rec->m_flags & 1);
}

void CGItem_C::SetTranslated() {
  m_item->m_dynamicFlags |= 2;
}

void CGItem_C::InstallItemIDMirrorHandler() {
  ClntObjMgrSetObjMirrorHandler(
      GetGUID(), OffsetOf(ID_OBJECT) + offsetof(CGObjectData, m_entryID), sizeof(m_obj->m_entryID), OnUpdateItemID, 0, HANDLER_PRIORITY_NORMAL
  );
}

void CGItem_C::UninstallItemIDMirrorHandler() {
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), OffsetOf(ID_OBJECT) + offsetof(CGObjectData, m_entryID), OnUpdateItemID, 0);
}

void CGItem_C::UpdateEnchantments() const {
  CGUnit_C *owner = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(GetOwner(), __FILE__, __LINE__));
  if (owner) {
    owner->UpdateObjComponentVisuals(this, m_item->m_enchantment, 5);
  }
}

void CGItem_C::UpdateExpirationTime(int timeLeft) {
  if (timeLeft > 0) {
    m_expirationTime = OsGetAsyncTimeMs() + 1000 * timeLeft;
  } else {
    m_expirationTime = 0;
  }
}

int CGItem_C::GetExpirationTimeLeft() {
  if (m_expirationTime) {
    DWORD now = OsGetAsyncTimeMs();
    if (static_cast<long>(now - m_expirationTime) < 0) {
      return m_expirationTime - now;
    }
  }
  return 0;
}

void CGItem_C::UpdateEnchantmentTime(int slot, int timeLeft) {
  FATALASSERT((slot >= 0) && (slot < NUM_ITEM_ENCHANTMENTS));
  if (timeLeft > 0) {
    m_enchantmentExpiration[slot] = OsGetAsyncTimeMs() + 1000 * timeLeft;
  } else {
    m_enchantmentExpiration[slot] = 0;
  }
}

int CGItem_C::GetEnchantmentTimeLeft(int slot) {
  FATALASSERT((slot >= 0) && (slot < NUM_ITEM_ENCHANTMENTS));
  if (m_enchantmentExpiration[slot]) {
    DWORD now = OsGetAsyncTimeMs();
    if (static_cast<long>(now - m_enchantmentExpiration[slot]) < 0) {
      return m_enchantmentExpiration[slot] - now;
    }
  }
  return 0;
}

BOOL CGItem_C::GetSelectionHighlightColor(NTempest::CImVector *outPtr) const {
  FATALASSERT(outPtr);
  *outPtr = NTempest::CImVector(0xFFFFFFFF);
  return 1;
}

void CGItem_C::OnRightClick() {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player || player->GetHealth() <= 0) {
    return;
  }

  CDataStore msg;
  msg.Put(CMSG_AUTOSTORE_GROUND_ITEM);
  msg.Put(GetGUID());
  msg.Finalize();
  ClientServices_Send(&msg);
}

int CGItem_C::GetPageTextID(void (*func)(int, const DWORDLONG &, LPVOID, bool)) const {
  const ItemStats_C *stats = g_itemDBCache.GetRecord(GetEntryID(), GetGUID(), func, 0);
  return stats ? stats->m_pageText : 0;
}

LPCSTR CGItem_C::GetObjectName() const {
  const ItemStats_C *stats = g_itemDBCache.GetRecord(GetEntryID(), 0, 0, 0);
  return stats ? stats->m_displayName[0] : 0;
}

int CGItem_C::GetMaxCount() const {
  const ItemStats_C *stats = g_itemDBCache.GetRecord(GetEntryID(), 0, 0, 0);
  return stats ? stats->m_maxCount : 1;
}

int CGItem_C::GetClassID() const {
  const ItemStats_C *stats = g_itemDBCache.GetRecord(GetEntryID(), 0, 0, 0);
  return stats ? stats->m_class : 0;
}

int CGItem_C::GetSubtypeID() const {
  const ItemStats_C *stats = g_itemDBCache.GetRecord(GetEntryID(), 0, 0, 0);
  return stats ? stats->m_subclass : 0;
}

UINT CGItem_C::GetInventoryType() const {
  const ItemStats_C *stats = g_itemDBCache.GetRecord(GetEntryID(), 0, 0, 0);
  return stats ? stats->m_inventoryType : 0;
}

int CGItem_C::GetDisplayID() const {
  const ItemStats_C *stats = g_itemDBCache.GetRecord(GetEntryID(), 0, 0, 0);
  return stats ? stats->m_displayInfoID : 0;
}

bool CGItem_C::IsExotic() const {
  return GetItemStaticFlag(ITEM_FLAG_EXOTIC) != 0;
}

BOOL CGItem_C::GetItemStaticFlag(ITEM_STATIC_FLAGS flags) const {
  const ItemStats_C *stats = g_itemDBCache.GetRecord(GetEntryID(), 0, 0, 0);
  return stats ? (stats->m_flags & flags) == flags : 0;
}

int CGItem_C::GetMaterial() const {
  const ItemStats_C *stats = g_itemDBCache.GetRecord(GetEntryID(), 0, 0, 0);
  return stats ? stats->m_material : 0;
}

int CGItem_C::GetSheatheType() const {
  const ItemStats_C *stats = g_itemDBCache.GetRecord(GetEntryID(), 0, 0, 0);
  return stats ? stats->m_sheatheType : 0;
}

BOOL CGItem_C::CanGoInSlot(UINT slot) const {
  return slot >= 23 || (g_ITEMTYPEARRAY[GetInventoryType()] & (1 << slot)) != 0;
}

int CGItem_C::GetSheatheInvisible() const {
  return GetSheatheType() > 4;
}

const ItemStats *CGItem_C::GetStats() const {
  return g_itemDBCache.GetRecord(GetEntryID(), 0, 0, 0);
}

bool CGItem_C::IsWrapper() const {
  const ItemStats_C *stats = g_itemDBCache.GetRecord(GetEntryID(), 0, 0, 0);
  return stats ? (stats->m_flags & ITEM_FLAG_IS_WRAPPER) != 0 : false;
}
