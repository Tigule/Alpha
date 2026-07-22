#ifndef WOW_SOURCE_UI_ACTIONBARFRAME_H
#define WOW_SOURCE_UI_ACTIONBARFRAME_H

class CGActionBar {
 public:
  static void __fastcall InitializeGame();
  static void __fastcall EnterWorld();
  static void __fastcall ShutdownGame();
  static void __fastcall ShowGrid();
  static void __fastcall HideGrid();
  static void __fastcall UpdateBonusBar();
  static void __fastcall SetAction(int id, int action);
  static void __fastcall RemoveAction(int id);
  static void __fastcall ReplaceSpell(int oldSpell, int newSpell);
  static void __fastcall RemoveSpell(int spellID);
  static void __fastcall UpdateSelection();
  static void __fastcall UpdateCooldowns();
  static void __fastcall UpdateUsable();
  static int __fastcall  IsSpell(int id) {
    return id >= 0 && id < 120 && m_slotActions[id] > 0;
  }
  static int __fastcall IsItem(int id) {
    return id >= 0 && id < 120 && m_slotActions[id] < 0;
  }
  static int __fastcall GetSpell(int id) {
    return IsSpell(id) ? m_slotActions[id] : 0;
  }
  static int __fastcall GetItem(int id) {
    return IsItem(id) ? -m_slotActions[id] : 0;
  }
  static int __fastcall HasAction(int id) {
    return id >= 0 && id < 120 && m_slotActions[id] != 0;
  }
  static int __fastcall          IsAttackAction(int id);
  static int __fastcall          IsUsableAction(int id, int &noMana);
  static int __fastcall          IsCurrentAction(int id);
  static int __fastcall          IsToggledAction(int id);
  static void __fastcall         UpdateItem(int entryID);
  static void __fastcall         AddAction(int action);
  static void __fastcall         UseAction(int id, int checkCursor);
  static void __fastcall         PickupAction(int id);
  static void __fastcall         PutActionInSlot(int id);
  static const char *__fastcall  GetAttackTexture();
  static const char *__fastcall  GetTexture(int id);
  static int __fastcall          GetCount(int id);
  static void __fastcall         GetCooldown(int id, unsigned long &startTime, unsigned int &duration, unsigned int &enable);
  static void __fastcall         PrecacheButtonArt(int id);
  static unsigned int __fastcall GetBonusBarOffset() {
    return m_bonusPage;
  }

 private:
  static void __fastcall SlotChanged(int id);
  static int             m_slotActions[120];
  static unsigned int    m_bonusPage;
};

#endif
