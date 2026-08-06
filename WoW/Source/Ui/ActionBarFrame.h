#ifndef WOW_SOURCE_UI_ACTIONBARFRAME_H
#define WOW_SOURCE_UI_ACTIONBARFRAME_H

#define NUM_ACTION_BUTTONS 120

class CGActionBar {
 public:
  static void InitializeGame();
  static void EnterWorld();
  static void ShutdownGame();
  static void ShowGrid();
  static void HideGrid();
  static void UpdateBonusBar();
  static void SetAction(int id, int action);
  static void RemoveAction(int id);
  static void ReplaceSpell(int oldSpell, int newSpell);
  static void RemoveSpell(int spellID);
  static void UpdateSelection();
  static void UpdateCooldowns();
  static void UpdateUsable();
  static int  IsSpell(int id) {
    return id >= 0 && id < NUM_ACTION_BUTTONS && m_slotActions[id] > 0;
  }
  static int IsItem(int id) {
    return id >= 0 && id < NUM_ACTION_BUTTONS && m_slotActions[id] < 0;
  }
  static int GetSpell(int id) {
    return IsSpell(id) ? m_slotActions[id] : 0;
  }
  static int GetItem(int id) {
    return IsItem(id) ? -m_slotActions[id] : 0;
  }
  static int HasAction(int id) {
    return id >= 0 && id < NUM_ACTION_BUTTONS && m_slotActions[id] != 0;
  }
  static int    IsAttackAction(int id);
  static int    IsUsableAction(int id, int &noMana);
  static int    IsCurrentAction(int id);
  static int    IsToggledAction(int id);
  static void   UpdateItem(int entryID);
  static void   AddAction(int action);
  static void   UseAction(int id, int checkCursor);
  static void   PickupAction(int id);
  static void   PutActionInSlot(int id);
  static LPCSTR GetAttackTexture();
  static LPCSTR GetTexture(int id);
  static int    GetCount(int id);
  static void   GetCooldown(int id, DWORD &startTime, UINT &duration, UINT &enable);
  static void   PrecacheButtonArt(int id);
  static void   SlotChanged(int id);
  static UINT   GetBonusBarOffset() {
    return m_bonusPage;
  }

 private:
  static int  m_slotActions[NUM_ACTION_BUTTONS];
  static UINT m_bonusPage;
};

#endif
