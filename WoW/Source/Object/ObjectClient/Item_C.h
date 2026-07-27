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

  unsigned char operator==(const ItemEnchantment &other) {
    return id == other.id && expiration == other.expiration && chargesRemaining == other.chargesRemaining;
  }

  unsigned char operator!=(const ItemEnchantment &other) {
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
  unsigned __int64 m_owner;
  unsigned __int64 m_containedIn;
  unsigned __int64 m_creator;
  unsigned int     m_stackCount;
  int              m_expiration;
  int              m_spellCharges[5];
  short            m_staticFlags;
  short            m_dynamicFlags;
  ItemEnchantment  m_enchantment[5];
  int              pad;
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
  static unsigned int GetDataSize();
  static unsigned int GetBaseOffset();
  static unsigned int TotalFields();
  static unsigned int GetUpdateMaskBytes();
  static unsigned int GetUpdateMaskBlocks();

  int GetStackCount() const;
  unsigned __int64 GetOwner() const;
  unsigned __int64 GetContainedIn() const;
  unsigned __int64 GetCreator() const {
    return m_item->m_creator;
  }
  unsigned int GetItemStaticFlags() const;
  unsigned int GetItemDynamicFlags() const;
  bool IsBound() const;
  bool IsTranslated() const;
  bool IsUnlocked() const;
  bool IsWrapped() const;
  unsigned int GetExpiration() const;
  int GetItemDynamicFlag(ITEM_DYNAMIC_FLAGS flag) const;
  int GetSpellCharges(int index) const;
  const ItemEnchantment *GetEnchantment(int index) const;
  int GetEnchantmentID(int index) const;
  int GetEnchantmentExpiration(int index) const;
  int GetEnchantmentCharges(int index) const;
  int GetPetitionID() const;
  int GetNumPetitionSignatures() const;
  unsigned char *GetData(unsigned int index);
  void SetStorage(unsigned long *storage) {
    m_item = reinterpret_cast<CGItemData *>(storage);
  }

 protected:
  explicit CGItem(unsigned long *storage) {
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
  CGItem_C(unsigned long *storage, unsigned long eventTime, CClientObjCreate *init);
  ~CGItem_C();

  void         PostInit(const CClientObjCreate &init);
  void         PostInitWithStats();
  virtual void Disable(int shutdown);
  virtual void Reenable();
  static void Initialize();
  static void Shutdown();
  const char                   *GetInventoryArt() const;
  static const char *GetInventoryArt(int displayID);
  virtual const char           *GetModelFileName() const;
  int                           GetDisplayID() const;
  int                           CanBeUsed();
  int                           GetUseSpell();
  int                           GetClassID() const;
  int                           GetSubtypeID() const;
  int                           GetSheatheType() const;
  int                           IsMetal() const;
  static int IsMetal(unsigned int material);
  int                           GetItemStaticFlag(ITEM_STATIC_FLAGS flags) const;
  int                           GetMaterial() const;
  const ItemStats              *GetStats() const;
  void Lock() {
    m_flags |= 1U;
  }
  void         SetTranslated();
  void         UpdateEnchantments() const;
  void         PostMovementUpdate();
  void         UpdateExpirationTime(int timeLeft);
  int          GetExpirationTimeLeft();
  void         UpdateEnchantmentTime(int slot, int timeLeft);
  int          GetEnchantmentTimeLeft(int slot);
  unsigned int GetInventoryType() const;
  int          GetMaxCount() const;
  bool         IsExotic() const;
  int          CanGoInSlot(unsigned int slot) const;
  int          GetSheatheInvisible() const;
  bool         IsWrapper() const;
  bool         Use();

  void SetStorage(unsigned long *storage);
  int  SetBlock(unsigned int i, unsigned long data);
  void SetData(const void *data, unsigned int bytes);
  static unsigned int OffsetOf(OBJECT_TYPE_ID type);
  virtual int         GetSelectionHighlightColor(NTempest::CImVector *outPtr) const;
  virtual void        OnRightClick();
  virtual int GetPageTextID(
      void(*func)(int, const unsigned __int64 &, void *, bool)
  ) const;
  virtual const char *GetObjectName() const;

  const ItemGroupSoundsRec *GetGroupSoundRec() const {
    return m_soundsRec;
  }

  const VirtualItemInfo *GetVirtualInfo();
  int IsLocked();

  void Unlock() {
    m_flags &= ~1U;
  }

 protected:
  void InstallObjMirrorHandlers();
  void InstallItemIDMirrorHandler();
  void UninstallItemIDMirrorHandler();

 private:
  CGItem_C &operator=(const CGItem_C &);
  unsigned int        m_flags;
  VirtualItemInfo     m_itemInfo;
  unsigned long       m_expirationTime;
  unsigned long       m_enchantmentExpiration[5];
  const ItemGroupSoundsRec *m_soundsRec;
};

#endif
