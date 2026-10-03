#ifndef WOW_SOURCE_OBJECT_OBJECTCLIENT_ITEM_C_H
#define WOW_SOURCE_OBJECT_OBJECTCLIENT_ITEM_C_H

#include "Object/ObjectClient/Object_C.h"

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
    m_item = reinterpret_cast<CGItemData *>(storage);
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

class CGItem_C : public CGObject_C, public CGItem {
  friend class CGItemText;
  friend class CGPlayer_C;
  friend bool Spell_C_CastSpell(int spellID, const CGItem_C *item);
  friend void SendCast(SpellCast *cast);

 public:
  CGItem_C(DWORD *storage, DWORD eventTime, CClientObjCreate *init);
  ~CGItem_C();

  void SetStorage(DWORD *storage);

  void           PostInit(const CClientObjCreate &init);
  void           PostInitWithStats();
  void PostMovementUpdate() {
  }
  virtual void   Disable(int shutdown);
  virtual void   Reenable();
  virtual void   OnRightClick();
  BOOL           CanBeUsed();
  int            GetUseSpell();
  bool           Use();
  LPCSTR         GetInventoryArt() const;
  static LPCSTR  GetInventoryArt(int displayID);
  BOOL           SetBlock(UINT i, DWORD data);
  void           SetData(LPCVOID data, UINT bytes);
  static UINT    OffsetOf(OBJECT_TYPE_ID type);
  virtual LPCSTR GetModelFileName() const;
  static void    Initialize();
  static void    Shutdown();
  BOOL           IsMetal() const;
  static BOOL    IsMetal(UINT material);
  void             Lock() {
    m_flags |= 1U;
  }

  void Unlock() {
    m_flags &= ~1U;
  }
  BOOL                   IsLocked() {
    return m_flags & 1;
  }
  void SetTranslated();

  const VirtualItemInfo *GetVirtualInfo();
  int                    GetMaxCount() const;
  int                    GetClassID() const;
  int                    GetSubtypeID() const;
  UINT                   GetInventoryType() const;
  int                    GetDisplayID() const;
  bool                   IsExotic() const;
  BOOL                   GetItemStaticFlag(ITEM_STATIC_FLAGS flags) const;
  int                    GetMaterial() const;
  int                    GetSheatheType() const;
  BOOL                   CanGoInSlot(UINT slot) const;
  int                    GetSheatheInvisible() const;
  bool                   IsWrapper() const;
  void                   UpdateExpirationTime(int timeLeft);
  int                    GetExpirationTimeLeft();
  void                   UpdateEnchantmentTime(int slot, int timeLeft);
  int                    GetEnchantmentTimeLeft(int slot);
  const ItemStats       *GetStats() const;

 protected:
  void InstallObjMirrorHandlers();
  void InstallItemIDMirrorHandler();
  void UninstallItemIDMirrorHandler();

 public:
  void UpdateEnchantments() const;

 private:
  CGItem_C &operator=(const CGItem_C &);

  UINT            m_flags;
  VirtualItemInfo m_itemInfo;
  DWORD           m_expirationTime;
  DWORD           m_enchantmentExpiration[5];

 public:
  virtual BOOL GetSelectionHighlightColor(NTempest::CImVector *outPtr) const;

  const ItemGroupSoundsRec *GetGroupSoundRec() const {
    return m_soundsRec;
  }

 private:
  const ItemGroupSoundsRec *m_soundsRec;

 public:
  virtual int    GetPageTextID(void (*func)(int, const DWORDLONG &, LPVOID, bool)) const;
  virtual LPCSTR GetObjectName() const;
};

#endif
