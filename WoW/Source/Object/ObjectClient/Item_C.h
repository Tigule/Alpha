#ifndef WOW_SOURCE_OBJECT_OBJECTCLIENT_ITEM_C_H
#define WOW_SOURCE_OBJECT_OBJECTCLIENT_ITEM_C_H

#include "Object/ObjectClient/Object_C.h"

struct ItemEnchantment {
  int id;
  int expiration;
  int chargesRemaining;
};

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
struct SpellCast;

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

class CGItem {
  friend class CGItemText;
  friend class CGPlayer_C;

 protected:
  CGItemData *m_item;
};

class CGItem_C : public CGObject_C, public CGItem {
  friend class CGItemText;
  friend class CGPlayer_C;
  friend void SendCast(SpellCast *cast);

 public:
  CGItem_C(unsigned long *storage, unsigned long eventTime, CClientObjCreate *init);
  virtual ~CGItem_C();

  void         PostInit(const CClientObjCreate &init);
  void         PostInitWithStats();
  virtual void Disable(int shutdown);
  virtual void Reenable();
  static void __fastcall        Initialize();
  static void __fastcall        Shutdown();
  static const char *__fastcall GetInventoryArt(int displayID);
  const char                   *GetInventoryArt() const;
  virtual const char           *GetModelFileName() const;
  int                           GetDisplayID() const;
  int                           CanBeUsed();
  int                           GetUseSpell();
  int                           GetClassID() const;
  int                           GetSubtypeID() const;
  int                           GetSheatheType() const;
  int                           IsMetal() const;
  static int __fastcall         IsMetal(unsigned int material);
  int                           GetItemStaticFlag(ITEM_STATIC_FLAGS flags) const;
  int                           GetMaterial() const;
  ItemStats                    *GetStats();
  int                           GetStackCount() const {
    return m_item->m_stackCount;
  }
  unsigned __int64 GetOwner() {
    return m_item->m_owner;
  }
  unsigned __int64 GetContainedIn() {
    return m_item->m_containedIn;
  }
  unsigned char IsTranslated() {
    return (m_item->m_dynamicFlags & 2) != 0;
  }
  unsigned int IsUnlocked() const {
    return (m_item->m_dynamicFlags & 1) == 0;
  }
  void Lock() {
    m_flags |= 1U;
  }
  void         SetTranslated();
  void         UpdateEnchantments();
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
  static unsigned int __fastcall OffsetOf(OBJECT_TYPE_ID type);
  virtual int         GetSelectionHighlightColor(NTempest::CImVector *outPtr) const;
  virtual void        OnRightClick();
  virtual int GetPageTextID(
      void(__fastcall *func)(int, const unsigned __int64 &, void *, bool)
  ) const;
  virtual const char *GetObjectName() const;

  ItemGroupSoundsRec *GetGroupSoundRec() const {
    return m_soundsRec;
  }

  void Unlock() {
    m_flags &= ~1U;
  }

 private:
  void InstallObjMirrorHandlers();
  void InstallItemIDMirrorHandler();
  void UninstallItemIDMirrorHandler();

  unsigned int        m_flags;
  VirtualItemInfo     m_itemInfo;
  unsigned long       m_expirationTime;
  unsigned long       m_enchantmentExpiration[5];
  ItemGroupSoundsRec *m_soundsRec;
};

#endif
