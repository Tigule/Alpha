#ifndef WOW_SOURCE_OBJECT_OBJECTCLIENT_ITEM_C_H
#define WOW_SOURCE_OBJECT_OBJECTCLIENT_ITEM_C_H

#include "Object/ObjectClient/Object_C.h"
#include "Object/Item.h"

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
