#include <WowConst.h>
#include <MapDefs.h>

#include "Item_C.h"

#include "Object/ObjectClient/Player_C.h"
#include "Magic/MagicClient/Spell_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Ui/GameUI.h"
#include "Ui/ItemTextFrame.h"
#include "Ui/LootFrame.h"
#include "Ui/QuestLog.h"
#include "Ui/TradeFrame.h"
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

extern const int *const g_ITEMTYPEARRAY;

bool Spell_C_CastSpell(int spellID, const CGItem_C *item);
void Spell_C_CancelSpell(bool failed, bool notifyServer, SPELL_FAILED_REASON reason);
const unsigned __int64 &Spell_C_GetCurrentCaster();
void ClntObjMgrHideObject(unsigned __int64 guid);
void ClntObjMgrShowObject(unsigned __int64 guid);

class CGContainerInfo {
 public:
  static void UpdateContents(unsigned __int64 guid);
  static void UpdateItem(unsigned __int64 guid);
};

class CGActionBar {
 public:
  static void UpdateItem(int entryID);
};

class CGCraftInfo {
 public:
  static void RefreshList();
};

class CGTradeSkillInfo {
 public:
  static void RefreshList(int resetFilters);
};

static int OnUpdateEnchantments(
    unsigned __int64, unsigned int, unsigned int, const void *, void *
);
static int OnUpdateItemID(
    unsigned __int64, unsigned int, unsigned int, const void *, void *
);

static int OnUpdateOwner(unsigned __int64 guid, unsigned int offset, unsigned int bytes, const void* prevValue, void* param) {
  CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  FATALASSERT(item);
  FATALASSERT(prevValue);
  unsigned __int64 previousOwner = *static_cast<const unsigned __int64 *>(prevValue);
  unsigned __int64 currOwner = item->GetOwner();
  if (previousOwner && !currOwner) {
    ClntObjMgrShowObject(guid);
    item->AddWorldObject();
  } else if (!previousOwner && currOwner) {
    ClntObjMgrHideObject(guid);
    item->RemoveWorldObject();
  }
  if (previousOwner == ClntObjMgrGetActivePlayer() ||
      currOwner == ClntObjMgrGetActivePlayer()) {
    CGActionBar::UpdateItem(item->GetEntryID());
    CGContainerInfo::UpdateItem(guid);
  }
  return 1;
}

static int OnUpdateStackCount(unsigned __int64 guid, unsigned int offset, unsigned int bytes, const void* prevValue, void* param) {
  CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  FATALASSERT(item);
  if (item->GetOwner() == ClntObjMgrGetActivePlayer()) {
    CGGameUI::UnlockItem(guid);
    CGActionBar::UpdateItem(item->GetEntryID());
    CGContainerInfo::UpdateItem(guid);
  }
  return 1;
}

CGItem_C::~CGItem_C() {
}

void CGItem_C::InstallObjMirrorHandlers() {
  unsigned int offset = OffsetOf(ID_ITEM);
  ClntObjMgrSetObjMirrorHandler(
      GetGUID(), offset, 8, OnUpdateOwner, 0, HANDLER_PRIORITY_NORMAL
  );
  ClntObjMgrSetObjMirrorHandler(
      GetGUID(), offset + offsetof(CGItemData, m_stackCount), 4, OnUpdateStackCount, 0, HANDLER_PRIORITY_NORMAL
  );
  ClntObjMgrSetObjMirrorHandler(
      GetGUID(), offset + offsetof(CGItemData, m_enchantment), sizeof(m_item->m_enchantment), OnUpdateEnchantments, 0, HANDLER_PRIORITY_NORMAL
  );
}

void CGItem_C::InstallItemIDMirrorHandler() {
  ClntObjMgrSetObjMirrorHandler(
      GetGUID(), OffsetOf(ID_OBJECT) + offsetof(CGObjectData, m_entryID), sizeof(m_obj->m_entryID), OnUpdateItemID, 0, HANDLER_PRIORITY_NORMAL
  );
}

void CGItem_C::UninstallItemIDMirrorHandler() {
  ClntObjMgrUnsetObjMirrorHandler(
      GetGUID(), OffsetOf(ID_OBJECT) + offsetof(CGObjectData, m_entryID), OnUpdateItemID, 0
  );
}

struct INVENTORYART : public TSHashObject<INVENTORYART, HASHKEY_NONE> {
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

  void SetArt(const char *art) {
    if (art && *art) {
      Clear();
      textureName = SStrDupA(art, __FILE__, __LINE__);
    }
  }

  char *textureName;
};

static TSHashTable<INVENTORYART, HASHKEY_NONE> s_inventoryTextures;
static HASHKEY_NONE                            s_nullHashKey;

static int OnUpdateEnchantments(unsigned __int64 guid, unsigned int, unsigned int, const void*, void*) {
  CGItem_C *item = static_cast<CGItem_C *>(
      ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (item) {
    item->UpdateEnchantments();
  }
  return 1;
}

static void ItemIDChangedCacheCallback(int id, const unsigned __int64& guid, void* arg, bool granted) {
  CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (item && item->GetOwner() == ClntObjMgrGetActivePlayer()) {
    CGContainerInfo::UpdateContents(item->GetContainedIn());
  }
}

static int OnUpdateItemID(unsigned __int64 guid, unsigned int offset, unsigned int bytes, const void* prevValue, void* param) {
  CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  FATALASSERT(item);
  FATALASSERT(prevValue);
  if (item->GetOwner() == ClntObjMgrGetActivePlayer()) {
    const ItemStats_C *stats = g_itemDBCache.GetRecord(
        item->GetEntryID(), guid, ItemIDChangedCacheCallback, 0);
    if (stats) {
      CGContainerInfo::UpdateContents(item->GetContainedIn());
    }
  }
  return 1;
}

static void AddInventoryArtHash(unsigned int displayID, const char *fileName) {
  INVENTORYART *entry = s_inventoryTextures.New(displayID, s_nullHashKey, 0, 0);
  entry->SetArt(fileName);
}

static const char *GetInventoryArtHash(unsigned int displayID) {
  INVENTORYART *entry = s_inventoryTextures.Ptr(displayID, s_nullHashKey);
  return entry ? entry->textureName : 0;
}

void CGItem_C::SetStorage(unsigned long *storage) {
  CGObject_C::SetStorage(storage);
  CGItem::SetStorage(storage + CGObject::TotalFields());
}

CGItem_C::CGItem_C(unsigned long *storage, unsigned long eventTime, CClientObjCreate *init)
    : CGObject_C(storage, eventTime, init),
      CGItem(storage + CGObject::TotalFields()),
      m_flags(0),
      m_expirationTime(0),
      m_soundsRec(0) {
  if (m_item->m_owner) {
    ClntObjMgrHideObject(GetGUID());
  } else {
    AddWorldObject();
  }

  m_itemInfo.m_classID = static_cast<unsigned char>(GetClassID());
  m_itemInfo.m_subclassID = static_cast<unsigned char>(GetSubtypeID());
  m_itemInfo.m_material = static_cast<unsigned char>(GetMaterial());
  m_itemInfo.m_inventoryType = static_cast<unsigned char>(GetInventoryType());
  m_itemInfo.m_sheatheType = static_cast<unsigned char>(GetSheatheType());

  for (unsigned int i = 0; i < 5; ++i) {
    m_enchantmentExpiration[i] = 0;
  }
}

static void LoadItemCacheCallback(int id, const unsigned __int64& guid, void* arg, bool granted) {
  CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (!item) {
    return;
  }

  CGPlayer_C *owner = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(item->GetOwner(), __FILE__, __LINE__));

  if (granted) {
    CGContainerInfo::UpdateContents(item->GetContainedIn());
    item->PostInitWithStats();

    if (owner) {
      const ItemStats_C *stats = g_itemDBCache.GetRecord(item->GetEntryID(), 0, 0, 0);
      owner->FixComponenting(item);
      owner->ItemReceived(stats);
      owner->UpdateReadyAnim(stats);
    }
  } else if (owner) {
    owner->DecrementPendingItemStats();
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
  const ItemStats_C *stat = g_itemDBCache.GetRecord(GetEntryID(), 0, 0, 0);
  FATALASSERT(stat);

  InstallObjMirrorHandlers();
  InstallItemIDMirrorHandler();

  const ItemDisplayInfoRec *displayInfo = g_itemDisplayInfoDB.GetRecord(GetDisplayID());
  m_soundsRec =
      displayInfo ? g_itemGroupSoundsDB.GetRecord(displayInfo->m_groupSoundIndex) : 0;

  if (m_item->m_owner == ClntObjMgrGetActivePlayer()) {
    CGActionBar::UpdateItem(GetEntryID());
    CGContainerInfo::UpdateItem(GetGUID());
  }

  m_itemInfo.m_classID = static_cast<unsigned char>(GetClassID());
  m_itemInfo.m_subclassID = static_cast<unsigned char>(GetSubtypeID());
  m_itemInfo.m_material = static_cast<unsigned char>(GetMaterial());
  m_itemInfo.m_inventoryType = static_cast<unsigned char>(GetInventoryType());
  m_itemInfo.m_sheatheType = static_cast<unsigned char>(GetSheatheType());
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

  unsigned int offset = OffsetOf(ID_ITEM);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), offset, OnUpdateOwner, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), offset + 24, OnUpdateStackCount, 0);
  ClntObjMgrUnsetObjMirrorHandler(GetGUID(), offset + 56, OnUpdateEnchantments, 0);

  if (CGGameUI::GetCursorItem() == GetGUID()) {
    CGGameUI::ClearCursor(0);
  }
  if (CGItemText::GetItem() == GetGUID()) {
    CGItemText::SetItem(0, 0);
  }

  bool updateUI = !shutdown && m_item->m_owner == ClntObjMgrGetActivePlayer();
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

const char *CGItem_C::GetInventoryArt(int displayID) {
  const char *inventoryArt = GetInventoryArtHash(displayID);
  if (inventoryArt) {
    return inventoryArt;
  }

  const ItemDisplayInfoRec *displayInfo = g_itemDisplayInfoDB.GetRecord(displayID);
  if (displayInfo && displayInfo->m_inventoryIcon && *displayInfo->m_inventoryIcon) {
    char buffer[MAX_PATH];
    inventoryArt = displayInfo->m_inventoryIcon;
    TEXFILETYPE type = TextureDiscoverFileType(inventoryArt);
    if (type == TEXFILETYPE_TGA && TexturePickAlternateFilename(inventoryArt, type, buffer, sizeof(buffer))) {
      inventoryArt = buffer;
    }

    AddInventoryArtHash(displayID, inventoryArt);
    return GetInventoryArtHash(displayID);
  }

  SysMsgPrintf(SYSMSG_ERROR, 2, "NOINVENTORYICON|%d", displayID);
  return "INV_Misc_QuestionMark";
}

const char *CGItem_C::GetModelFileName() const {
  return 0;
}

int CGItem_C::CanBeUsed() {
  const ItemStats_C *stats = g_itemDBCache.GetRecord(GetEntryID(), ClntObjMgrGetActivePlayer(), 0, 0);
  if (!stats) {
    return 0;
  }
  for (int index = 0; index < 5; ++index) {
    if (stats->m_spellID[index] && !stats->m_spellTrigger[index]) {
      return 1;
    }
  }
  return 0;
}

int CGItem_C::GetUseSpell() {
  const ItemStats_C *stats = g_itemDBCache.GetRecord(GetEntryID(), 0, 0, 0);
  if (!stats) {
    return 0;
  }
  for (int index = 0; index < 5; ++index) {
    if (stats->m_spellID[index] && !stats->m_spellTrigger[index]) {
      return stats->m_spellID[index];
    }
  }
  return 0;
}

bool CGItem_C::Use() {
  if (m_obj->m_type & 4 || (m_flags & 1)) {
    return false;
  }

  const ItemStats_C *stats = g_itemDBCache.GetRecord(GetEntryID(), 0, 0, 0);
  if (!stats) {
    return false;
  }

  CGPlayer_C     *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  GAME_ERROR_TYPE reason = GERR_NUM_TYPES;
  if (player && !player->CanUseItem(stats, reason)) {
    CGGameUI::DisplayError(reason == static_cast<GAME_ERROR_TYPE>(1) ? static_cast<GAME_ERROR_TYPE>(1) : reason);
    return false;
  }

  if (stats->m_pageText) {
    CGItemText::SetItem(GetGUID(), 0);
    return false;
  }
  if (stats->m_startQuestID) {
    if (player) {
      player->QueryQuest(GetGUID(), stats->m_startQuestID);
    }
    return false;
  }

  if (stats->m_flags & 4) {
    if (player) {
      player->OpenLootItem(this);
    }
    return false;
  }
  if (stats->m_flags & 0x2000) {
    if (player) {
      player->RequestPetitionSignatures(GetGUID());
    }
    return false;
  }
  if (stats->m_flags & 0x200) {
    if (m_item->m_dynamicFlags & 8) {
      if (player) {
        player->OpenWrappedItem(this);
      }
    } else {
      CGPlayer_C::StartGiftWrap(this);
    }
    return false;
  }

  if (!CanBeUsed()) {
    return false;
  }
  if (GetInventoryType()) {
    const unsigned __int64 activePlayer = ClntObjMgrGetActivePlayer();
    if (m_item->m_containedIn != activePlayer || !player || player->FindSlotIndex(GetGUID()) >= 23) {
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(138));
      return false;
    }
  }

  return Spell_C_CastSpell(GetUseSpell(), this);
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

void CGItem_C::SetTranslated() {
  m_item->m_dynamicFlags |= 2;
}

void CGItem_C::UpdateEnchantments() const {
  CGUnit_C *owner = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(m_item->m_owner, __FILE__, __LINE__));
  if (owner) {
    owner->UpdateObjComponentVisuals(this, m_item->m_enchantment, 5);
  }
}

void CGItem_C::UpdateExpirationTime(int timeLeft) {
  if (timeLeft <= 0) {
    m_expirationTime = 0;
  } else {
    m_expirationTime = OsGetAsyncTimeMs() + 1000 * timeLeft;
  }
}

int CGItem_C::GetExpirationTimeLeft() {
  if (m_expirationTime) {
    unsigned long now = OsGetAsyncTimeMs();
    if (static_cast<long>(now - m_expirationTime) < 0) {
      return m_expirationTime - now;
    }
  }
  return 0;
}

int CGItem_C::GetEnchantmentTimeLeft(int slot) {
  FATALASSERT((slot >= 0) && (slot < 5));
  if (m_enchantmentExpiration[slot]) {
    unsigned long now = OsGetAsyncTimeMs();
    if (static_cast<long>(now - m_enchantmentExpiration[slot]) < 0) {
      return m_enchantmentExpiration[slot] - now;
    }
  }
  return 0;
}

void CGItem_C::UpdateEnchantmentTime(int slot, int timeLeft) {
  FATALASSERT((slot >= 0) && (slot < 5));
  if (timeLeft <= 0) {
    m_enchantmentExpiration[slot] = 0;
  } else {
    m_enchantmentExpiration[slot] = OsGetAsyncTimeMs() + 1000 * timeLeft;
  }
}

int CGItem_C::GetSheatheType() const {
  const ItemStats_C *stats = g_itemDBCache.GetRecord(m_obj->m_entryID, 0, 0, 0);
  return stats ? stats->m_sheatheType : 0;
}

const char *CGItem_C::GetInventoryArt() const {
  return GetInventoryArt(GetDisplayID());
}

int CGItem_C::GetClassID() const {
  const ItemStats_C *stats = g_itemDBCache.GetRecord(m_obj->m_entryID, 0, 0, 0);
  return stats ? stats->m_class : 0;
}

int CGItem_C::GetSubtypeID() const {
  const ItemStats_C *stats = g_itemDBCache.GetRecord(m_obj->m_entryID, 0, 0, 0);
  return stats ? stats->m_subclass : 0;
}

unsigned int CGItem_C::GetInventoryType() const {
  const ItemStats_C *stats = g_itemDBCache.GetRecord(m_obj->m_entryID, 0, 0, 0);
  return stats ? stats->m_inventoryType : 0;
}

int CGItem_C::GetDisplayID() const {
  const ItemStats_C *stats = g_itemDBCache.GetRecord(m_obj->m_entryID, 0, 0, 0);
  return stats ? stats->m_displayInfoID : 0;
}

int CGItem_C::GetItemStaticFlag(ITEM_STATIC_FLAGS flags) const {
  const ItemStats_C *stats = g_itemDBCache.GetRecord(m_obj->m_entryID, 0, 0, 0);
  return stats && (stats->m_flags & flags) == flags;
}

int CGItem_C::GetMaterial() const {
  const ItemStats_C *stats = g_itemDBCache.GetRecord(m_obj->m_entryID, 0, 0, 0);
  return stats ? stats->m_material : 0;
}

int CGItem_C::IsMetal() const {
  return IsMetal(GetMaterial());
}

int CGItem_C::IsMetal(unsigned int material) {
  const MaterialRec *rec = g_materialDB.GetRecord(material);
  return rec && (rec->m_flags & 1);
}

const ItemStats *CGItem_C::GetStats() const {
  return g_itemDBCache.GetRecord(m_obj->m_entryID, 0, 0, 0);
}

int CGItem_C::SetBlock(unsigned int i, unsigned long data) {
  if (i < OffsetOf(ID_ITEM)) {
    return CGObject_C::SetBlock(i, data);
  }

  i -= OffsetOf(ID_ITEM);
  FATALASSERT(i < sizeof(*m_item) / sizeof(unsigned long));
  reinterpret_cast<unsigned long *>(m_item)[i] = data;
  return 1;
}

void CGItem_C::SetData(const void *data, unsigned int bytes) {
  FATALASSERT(bytes <= sizeof(*m_item));
  memcpy(m_item, data, bytes);
}

unsigned int CGItem_C::OffsetOf(OBJECT_TYPE_ID type) {
  if (type == ID_OBJECT) {
    return 0;
  }
  FATALASSERT(type == ID_ITEM);
  return CGObject::TotalFields() * sizeof(unsigned long);
}

int CGItem_C::GetSelectionHighlightColor(NTempest::CImVector *outPtr) const {
  FATALASSERT(outPtr);
  *outPtr = NTempest::CImVector(0xFFFFFFFF);
  return 1;
}

void CGItem_C::OnRightClick() {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(
      ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__)
  );
  if (!player || player->GetUnitData()->health <= 0) {
    return;
  }

  CDataStore msg;
  msg.Put(static_cast<int>(CMSG_AUTOSTORE_GROUND_ITEM));
  msg.Put(GetGUID());
  msg.Finalize();
  ClientServices_Send(&msg);
}

int CGItem_C::GetPageTextID(
    void(*func)(int, const unsigned __int64 &, void *, bool)
) const {
  const ItemStats_C *stats = g_itemDBCache.GetRecord(m_obj->m_entryID, m_obj->m_guid, func, 0);
  return stats ? stats->m_pageText : 0;
}

const char *CGItem_C::GetObjectName() const {
  const ItemStats_C *stats = g_itemDBCache.GetRecord(m_obj->m_entryID, 0, 0, 0);
  return stats ? stats->m_displayName[0] : 0;
}

int CGItem_C::GetMaxCount() const {
  const ItemStats_C *stats = g_itemDBCache.GetRecord(m_obj->m_entryID, 0, 0, 0);
  return stats ? stats->m_maxCount : 1;
}

bool CGItem_C::IsExotic() const {
  return GetItemStaticFlag(ITEM_FLAG_EXOTIC) != 0;
}

int CGItem_C::CanGoInSlot(unsigned int slot) const {
  return slot >= 23 || (g_ITEMTYPEARRAY[GetInventoryType()] & (1 << slot)) != 0;
}

int CGItem_C::GetSheatheInvisible() const {
  return GetSheatheType() > 4;
}

bool CGItem_C::IsWrapper() const {
  const ItemStats_C *stats = g_itemDBCache.GetRecord(m_obj->m_entryID, 0, 0, 0);
  return stats && (stats->m_flags & ITEM_FLAG_IS_WRAPPER) != 0;
}
