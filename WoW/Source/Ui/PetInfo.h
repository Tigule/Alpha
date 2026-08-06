#ifndef WOW_SOURCE_UI_PETINFO_H
#define WOW_SOURCE_UI_PETINFO_H

class PetAction {
 public:
  PetAction(UINT action = 0) : m_action(action) {
  }

  void SetAction(UINT);
  UINT GetAction() const;
  void SetActionTypeAndID(UINT, UINT);
  UINT GetActionTypeAndID() const;
  void SetActionType(UINT);
  int  GetActionType() const;
  void SetActionID(UINT);
  int  GetActionID() const;
  void SetAutocastAllowed(BYTE);
  BYTE GetAutocastAllowed() const;
  void SetAutocastEnabled(BYTE);
  BYTE GetAutocastEnabled() const;
  BYTE operator==(const PetAction &) const;

  operator UINT &() {
    return m_action;
  }

  operator const UINT &() const {
    return m_action;
  }

 private:
  UINT m_action;
};

class CGPetInfo {
 public:
  static void      InitializeGame();
  static void      EnterWorld();
  static void      LeaveWorld();
  static void      ShutdownGame();
  static void      HideGrid();
  static void      SetPet(DWORDLONG pet, DWORD expirationTime);
  static void      SetPetModeAndOrders(UINT petMode);
  static DWORDLONG GetPet() {
    return m_pet;
  }
  static DWORD GetExpirationTime() {
    return m_expirationTime;
  }
  static void SetPetMode(UINT mode);
  static UINT GetPetMode() {
    return m_petMode & 0xFF;
  }
  static void SetPetOrders(UINT orders);
  static UINT GetPetOrders() {
    return m_petMode >> 8 & 0xFF;
  }
  static void             ClearActions();
  static void             SetAction(UINT index, PetAction &action, int save);
  static void             UpdateCooldowns();
  static const PetAction *GetAction(UINT index) {
    return m_pet && index < 10 ? &m_actions[index] : 0;
  }
  static void ToggleAutocast(UINT index);
  static void PutSpellInSlot(int spell, UINT slot) {
    PutActionInSlot(static_cast<UINT>(spell) | 0x01000000, slot);
  }
  static void PutActionInSlot(PetAction &action, UINT slot);
  static void PutActionInSlot(UINT action, UINT slot) {
    PetAction petAction(action);
    PutActionInSlot(petAction, slot);
  }
  static LPCSTR GetModeToken(UINT id);
  static LPCSTR GetOrdersToken(UINT id);
  static void   ShowGrid();
  static void   SendPetAction(const PetAction &action, const DWORDLONG &target);
  static void   PetPassiveMode();
  static void   PetDefensiveMode();
  static void   PetAggressiveMode();
  static void   PetWait();
  static void   PetFollow();
  static void   PetAttackTarget(const DWORDLONG &targetGUID);
  static void   PetDismiss();
  static void   PetAbandon();
  static void   PetRename(LPCSTR newName);

 protected:
  static DWORDLONG m_pet;
  static UINT      m_petMode;
  static PetAction m_actions[10];
  static DWORD     m_expirationTime;
};

#endif
