#ifndef WOW_SOURCE_OBJECT_ITEM_H
#define WOW_SOURCE_OBJECT_ITEM_H

#define NUM_ITEM_ENCHANTMENTS 5
#define NUM_ITEM_SPELLS       5

struct ItemEnchantment {
  int id;
  int expiration;
  int chargesRemaining;

  ItemEnchantment(int id = 0, int expiration = 0, int chargesRemaining = 0);

  ItemEnchantment &operator=(const ItemEnchantment &other) {
    id = other.id;
    expiration = other.expiration;
    chargesRemaining = other.chargesRemaining;
    return *this;
  }

  BYTE operator==(const ItemEnchantment &other) {
    return id == other.id && expiration == other.expiration && chargesRemaining == other.chargesRemaining;
  }

  BYTE operator!=(const ItemEnchantment &other) {
    return !(*this == other);
  }


};

inline ItemEnchantment::ItemEnchantment(int id, int expiration, int chargesRemaining)
    : id(id), expiration(expiration), chargesRemaining(chargesRemaining) {
}

struct CGItemData {
  DWORDLONG       m_owner;
  DWORDLONG       m_containedIn;
  DWORDLONG       m_creator;
  UINT            m_stackCount;
  int             m_expiration;
  int             m_spellCharges[5];
  short           m_staticFlags;
  short           m_dynamicFlags;
  ItemEnchantment m_enchantment[NUM_ITEM_ENCHANTMENTS];
  int             pad;
};
class ItemGroupSoundsRec;
class CGItemText;
class ItemStats;
class SpellCast;

enum ITEM_STATIC_FLAGS {
  ITEM_FLAG_NO_PICKUP = 1,
  ITEM_FLAG_CONJURED = 2,
  ITEM_FLAG_HAS_LOOT = 4,
  ITEM_FLAG_EXOTIC = 8,
  ITEM_FLAG_DEPRECATED = 16,
  ITEM_FLAG_OBSOLETE = 32,
  ITEM_FLAG_PLAYERCAST = 64,
  ITEM_FLAG_NO_EQUIPCOOLDOWN = 128,
  ITEM_FLAG_INTBONUSINSTEAD = 256,
  ITEM_FLAG_IS_WRAPPER = 512,
  ITEM_FLAG_USES_RESOURCES = 1024,
  ITEM_FLAG_MULTI_DROP = 2048,
  ITEM_FLAG_BRIEFSPELLEFFECTS = 4096,
  ITEM_FLAG_PETITION = 8192,
  MAX_ITEM_FLAG = 32768,
  ITEM_FLAG_NUM = 14
};

enum ITEM_DYNAMIC_FLAGS {
  ITEM_DFLAG_BOUND = 1,
  ITEM_DFLAG_TRANSLATED = 2,
  ITEM_DFLAG_UNLOCKED = 4,
  ITEM_DFLAG_WRAPPED = 8
};

class CGItem {
  friend class CGItemText;
  friend class CGPlayer_C;

 public:
  int GetStackCount() const {
    return m_item->m_stackCount;
  }
  DWORDLONG GetOwner() const {
    return m_item->m_owner;
  }
  DWORDLONG GetContainedIn() const {
    return m_item->m_containedIn;
  }
  DWORDLONG GetCreator() const {
    return m_item->m_creator;
  }
  UINT GetItemStaticFlags() const;
  UINT GetItemDynamicFlags() const;
  bool IsBound() const {
    return (m_item->m_dynamicFlags & ITEM_DFLAG_BOUND) != 0;
  }
  bool IsTranslated() const {
    return (m_item->m_dynamicFlags & ITEM_DFLAG_TRANSLATED) != 0;
  }
  bool IsUnlocked() const {
    return (m_item->m_dynamicFlags & ITEM_DFLAG_UNLOCKED) != 0;
  }
  bool IsWrapped() const {
    return (m_item->m_dynamicFlags & ITEM_DFLAG_WRAPPED) != 0;
  }
  UINT                   GetExpiration() const;
  int                    GetItemDynamicFlag(ITEM_DYNAMIC_FLAGS flag) const;
  int                    GetSpellCharges(int slot) const;
  const ItemEnchantment *GetEnchantment(int index) const;
  int                    GetEnchantmentID(int slot) const;
  int                    GetEnchantmentExpiration(int index) const;
  int                    GetEnchantmentCharges(int index) const;
  int GetPetitionID() const {
    return !(m_item->m_staticFlags & ITEM_FLAG_PETITION) ? 0 : m_item->m_enchantment[0].id;
  }
  int                    GetNumPetitionSignatures() const;
  BYTE                  *GetData(UINT index);
  static __forceinline UINT GetDataSize() {
    return TotalFields() * sizeof(DWORD);
  }
  static UINT               GetBaseOffset();
  static __forceinline UINT TotalFields() {
    return 36;
  }
  static UINT GetUpdateMaskBytes();
  static UINT GetUpdateMaskBlocks();

  void                   SetStorage(DWORD *storage) {
    m_item = (CGItemData *)storage;
  }

 protected:
  explicit CGItem(DWORD *storage) {
    SetStorage(storage);
  }

  ~CGItem() {
  }

  const CGItemData *Item() const {
    return m_item;
  }

  CGItemData *Item() {
    return m_item;
  }

  CGItemData *m_item;
};

inline int CGItem::GetSpellCharges(int slot) const {
  FATALASSERT(slot >= 0);
  FATALASSERT(slot < NUM_ITEM_SPELLS);
  return m_item->m_spellCharges[slot];
}

inline int CGItem::GetEnchantmentID(int slot) const {
  FATALASSERT(slot >= 0);
  FATALASSERT(slot < NUM_ITEM_ENCHANTMENTS);
  return GetEnchantment(slot)->id;
}

#endif
