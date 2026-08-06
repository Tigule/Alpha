#ifndef WOW_SOURCE_OBJECT_OBJECTCLIENT_ITEM_C_H
#define WOW_SOURCE_OBJECT_OBJECTCLIENT_ITEM_C_H

#include "Object/ObjectClient/Object_C.h"

struct ItemEnchantment {
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

  int id;
  int expiration;
  int chargesRemaining;
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
  ItemEnchantment m_enchantment[5];
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
  ITEM_FLAG_NUM = 14,
  MAX_ITEM_FLAG = 32768
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
  static UINT               GetDataSize();
  static UINT               GetBaseOffset();
  static __forceinline UINT TotalFields() {
    return 36;
  }
  static UINT GetUpdateMaskBytes();
  static UINT GetUpdateMaskBlocks();

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
  bool IsBound() const;
  bool IsTranslated() const {
    return (m_item->m_dynamicFlags & ITEM_DFLAG_TRANSLATED) != 0;
  }
  bool IsUnlocked() const {
    return (m_item->m_dynamicFlags & ITEM_DFLAG_BOUND) == 0;
  }
  bool                   IsWrapped() const;
  UINT                   GetExpiration() const;
  int                    GetItemDynamicFlag(ITEM_DYNAMIC_FLAGS flag) const;
  int                    GetSpellCharges(int index) const;
  const ItemEnchantment *GetEnchantment(int index) const;
  int                    GetEnchantmentID(int index) const;
  int                    GetEnchantmentExpiration(int index) const;
  int                    GetEnchantmentCharges(int index) const;
  int                    GetPetitionID() const;
  int                    GetNumPetitionSignatures() const;
  BYTE                  *GetData(UINT index);
  void                   SetStorage(DWORD *storage) {
    m_item = reinterpret_cast<CGItemData *>(storage);
  }

 protected:
  explicit CGItem(DWORD *storage) {
    SetStorage(storage);
  }

  ~CGItem() {
  }

  CGItemData *Item() {
    return m_item;
  }

  const CGItemData *Item() const {
    return m_item;
  }

  CGItemData *m_item;
};

class CGItem_C : public CGObject_C, public CGItem {
  friend class CGItemText;
  friend class CGPlayer_C;
  friend void SendCast(SpellCast *cast);

 public:
  CGItem_C(DWORD *storage, DWORD eventTime, CClientObjCreate *init);
  ~CGItem_C();

  void             PostInit(const CClientObjCreate &init);
  void             PostInitWithStats();
  virtual void     Disable(int shutdown);
  virtual void     Reenable();
  static void      Initialize();
  static void      Shutdown();
  LPCSTR           GetInventoryArt() const;
  static LPCSTR    GetInventoryArt(int displayID);
  virtual LPCSTR   GetModelFileName() const;
  int              GetDisplayID() const;
  int              CanBeUsed();
  int              GetUseSpell();
  int              GetClassID() const;
  int              GetSubtypeID() const;
  int              GetSheatheType() const;
  int              IsMetal() const;
  static int       IsMetal(UINT material);
  int              GetItemStaticFlag(ITEM_STATIC_FLAGS flags) const;
  int              GetMaterial() const;
  const ItemStats *GetStats() const;
  void             Lock() {
    m_flags |= 1U;
  }
  void SetTranslated();
  void UpdateEnchantments() const;
  void PostMovementUpdate();
  void UpdateExpirationTime(int timeLeft);
  int  GetExpirationTimeLeft();
  void UpdateEnchantmentTime(int slot, int timeLeft);
  int  GetEnchantmentTimeLeft(int slot);
  UINT GetInventoryType() const;
  int  GetMaxCount() const;
  bool IsExotic() const;
  int  CanGoInSlot(UINT slot) const;
  int  GetSheatheInvisible() const;
  bool IsWrapper() const;
  bool Use();

  void           SetStorage(DWORD *storage);
  int            SetBlock(UINT i, DWORD data);
  void           SetData(LPCVOID data, UINT bytes);
  static UINT    OffsetOf(OBJECT_TYPE_ID type);
  virtual int    GetSelectionHighlightColor(NTempest::CImVector *outPtr) const;
  virtual void   OnRightClick();
  virtual int    GetPageTextID(void (*func)(int, const DWORDLONG &, LPVOID, bool)) const;
  virtual LPCSTR GetObjectName() const;

  const ItemGroupSoundsRec *GetGroupSoundRec() const {
    return m_soundsRec;
  }

  const VirtualItemInfo *GetVirtualInfo();
  int                    IsLocked() {
    return m_flags & 1;
  }

  void Unlock() {
    m_flags &= ~1U;
  }

 protected:
  void InstallObjMirrorHandlers();
  void InstallItemIDMirrorHandler();
  void UninstallItemIDMirrorHandler();

 private:
  CGItem_C                 &operator=(const CGItem_C &);
  UINT                      m_flags;
  VirtualItemInfo           m_itemInfo;
  DWORD                     m_expirationTime;
  DWORD                     m_enchantmentExpiration[5];
  const ItemGroupSoundsRec *m_soundsRec;
};

#endif
