#include "Item_C.h"

#include "Object/ObjectClient/Player_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Ui/GameUI.h"
#include "Ui/ItemTextFrame.h"

#include <Base/Status.h>
#include <DB/DBClient/DBClient.h>
#include <DB/DBClient/DBCacheInstances.h>
#include <DB/DBClient/AutoCode/ItemDisplayInfoRec.h>
#include <DB/DBClient/AutoCode/MaterialRec.h>
#include <Services/SysMessage.h>
#include <Services/Texture.h>
#include <Object/ItemStats.h>
#include <stpl.h>

bool __fastcall Spell_C_CastSpell(int spellID, const CGItem_C *item);
void __fastcall ClntObjMgrHideObject(unsigned __int64 guid);

void CGItem_C::SetStorage(unsigned long *storage) {
  CGObject_C::SetStorage(storage);
  m_item = reinterpret_cast<CGItemData *>(storage + 6);
}

CGItem_C::CGItem_C(unsigned long *storage, unsigned long eventTime, CClientObjCreate *init)
    : CGObject_C(storage, eventTime, init), m_flags(0), m_expirationTime(0), m_soundsRec(0) {
  m_item = reinterpret_cast<CGItemData *>(storage + 6);

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

CGItem_C::~CGItem_C() {
}

struct INVENTORYART : public TSHashObject<INVENTORYART, HASHKEY_NONE> {
  char *textureName;
};

static TSHashTable<INVENTORYART, HASHKEY_NONE> s_inventoryTextures;
static HASHKEY_NONE                            s_nullInventoryArtKey;

static void __fastcall AddInventoryArtHash(unsigned int displayID, const char *fileName) {
  INVENTORYART *entry = s_inventoryTextures.New(displayID, s_nullInventoryArtKey, 0, 0);
  entry->textureName = SStrDupA(fileName, __FILE__, __LINE__);
}

static const char *__fastcall GetInventoryArtHash(unsigned int displayID) {
  INVENTORYART *entry = s_inventoryTextures.Ptr(displayID, s_nullInventoryArtKey);
  return entry ? entry->textureName : 0;
}

const char *__fastcall CGItem_C::GetInventoryArt(int displayID) {
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

int CGItem_C::GetDisplayID() const {
  const unsigned __int64 noGuid = 0;
  const ItemStats_C     *stats = g_itemDBCache.GetRecord(m_obj->m_entryID, noGuid, 0, 0);
  return stats ? stats->m_displayInfoID : 0;
}

unsigned int CGItem_C::GetInventoryType() const {
  const unsigned __int64 noGuid = 0;
  const ItemStats_C     *stats = g_itemDBCache.GetRecord(m_obj->m_entryID, noGuid, 0, 0);
  return stats ? stats->m_inventoryType : 0;
}

int CGItem_C::GetSheatheType() {
  ItemStats *stats = GetStats();
  return stats ? stats->m_sheatheType : 0;
}

ItemStats *CGItem_C::GetStats() {
  const unsigned __int64 noGuid = 0;
  return const_cast<ItemStats_C *>(g_itemDBCache.GetRecord(m_obj->m_entryID, noGuid, 0, 0));
}

void CGItem_C::UpdateExpirationTime(int timeLeft) {
  if (timeLeft <= 0) {
    m_expirationTime = 0;
  } else {
    m_expirationTime = GetTickCount() + 1000 * timeLeft;
  }
}

int CGItem_C::GetExpirationTimeLeft() {
  if (m_expirationTime) {
    unsigned long now = GetTickCount();
    if (static_cast<long>(now - m_expirationTime) < 0) {
      return m_expirationTime - now;
    }
  }
  return 0;
}

void CGItem_C::UpdateEnchantmentTime(int slot, int timeLeft) {
  FATALASSERT((slot >= 0) && (slot < 5));
  if (timeLeft <= 0) {
    m_enchantmentExpiration[slot] = 0;
  } else {
    m_enchantmentExpiration[slot] = GetTickCount() + 1000 * timeLeft;
  }
}

void CGItem_C::SetTranslated() {
  m_item->m_dynamicFlags |= 2;
}

void CGItem_C::UpdateEnchantments() {
  CGUnit_C *owner = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(m_item->m_owner, __FILE__, __LINE__));
  if (owner) {
    owner->UpdateObjComponentVisuals(this, m_item->m_enchantment, 5);
  }
}

int CGItem_C::GetClassID() const {
  const unsigned __int64 noGuid = 0;
  const ItemStats_C     *stats = g_itemDBCache.GetRecord(m_obj->m_entryID, noGuid, 0, 0);
  return stats ? stats->m_class : 0;
}

int CGItem_C::GetSubtypeID() const {
  const unsigned __int64 noGuid = 0;
  const ItemStats_C     *stats = g_itemDBCache.GetRecord(m_obj->m_entryID, noGuid, 0, 0);
  return stats ? stats->m_subclass : 0;
}

int CGItem_C::GetSheatheType() const {
  const unsigned __int64 noGuid = 0;
  const ItemStats_C     *stats = g_itemDBCache.GetRecord(m_obj->m_entryID, noGuid, 0, 0);
  return stats ? stats->m_sheatheType : 0;
}

const char *CGItem_C::GetInventoryArt() const {
  return GetInventoryArt(GetDisplayID());
}

const char *CGItem_C::GetModelFileName() const {
  return 0;
}

int CGItem_C::CanBeUsed() {
  const unsigned __int64 player = ClntObjMgrGetActivePlayer();
  const ItemStats_C     *stats = g_itemDBCache.GetRecord(GetEntryID(), player, 0, 0);
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
  const unsigned __int64 noGuid = 0;
  const ItemStats_C     *stats = g_itemDBCache.GetRecord(GetEntryID(), noGuid, 0, 0);
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

  const unsigned __int64 noGuid = 0;
  const ItemStats_C     *stats = g_itemDBCache.GetRecord(GetEntryID(), noGuid, 0, 0);
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
    const unsigned __int64 guid = GetGUID();
    CGItemText::SetItem(guid, 0);
    return false;
  }
  if (stats->m_startQuestID) {
    if (player) {
      const unsigned __int64 guid = GetGUID();
      player->QueryQuest(guid, stats->m_startQuestID);
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

void __fastcall CGItem_C::Initialize() {
  if (ClientDBStringLookup(SLOOKUP_INVENTORYICONBUTTONGEOMETRY)) {
    CStatus status;

    SysMsgAdd(status, 0x10);
  }
}

void __fastcall CGItem_C::Shutdown() {
  s_inventoryTextures.Clear();
}

int CGItem_C::IsMetal() {
  return IsMetal(GetMaterial());
}

int __fastcall CGItem_C::IsMetal(unsigned int material) {
  MaterialRec *rec = g_materialDB.GetRecord(material);
  return rec && (rec->m_flags & 1);
}

int CGItem_C::GetItemStaticFlag(ITEM_STATIC_FLAGS flags) const {
  const unsigned __int64 noGuid = 0;
  const ItemStats_C     *stats = g_itemDBCache.GetRecord(GetEntryID(), noGuid, 0, 0);
  return stats && (stats->m_flags & flags) == flags;
}

int CGItem_C::GetMaterial() {
  return m_itemInfo.m_material;
}
