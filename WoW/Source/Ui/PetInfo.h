#ifndef WOW_SOURCE_UI_PETINFO_H
#define WOW_SOURCE_UI_PETINFO_H

class PetAction {
 public:
  PetAction(unsigned int action = 0) : m_action(action) {
  }

  void SetAction(unsigned int);
  unsigned int GetAction() const;
  void SetActionTypeAndID(unsigned int, unsigned int);
  unsigned int GetActionTypeAndID() const;
  void SetActionType(unsigned int);
  int GetActionType() const;
  void SetActionID(unsigned int);
  int GetActionID() const;
  void SetAutocastAllowed(unsigned char);
  unsigned char GetAutocastAllowed() const;
  void SetAutocastEnabled(unsigned char);
  unsigned char GetAutocastEnabled() const;
  unsigned char operator==(const PetAction &) const;

  operator unsigned int &() {
    return m_action;
  }

  operator const unsigned int &() const {
    return m_action;
  }

 private:
  unsigned int m_action;
};

class CGPetInfo {
 public:
  static void InitializeGame();
  static void EnterWorld();
  static void LeaveWorld();
  static void ShutdownGame();
  static void HideGrid();
  static void SetPet(unsigned __int64 pet, unsigned long expirationTime);
  static void SetPetModeAndOrders(unsigned int petMode);
  static unsigned __int64 GetPet() {
    return m_pet;
  }
  static unsigned long GetExpirationTime() {
    return m_expirationTime;
  }
  static void SetPetMode(unsigned int mode);
  static unsigned int    GetPetMode() {
    return m_petMode & 0xFF;
  }
  static void SetPetOrders(unsigned int orders);
  static unsigned int    GetPetOrders() {
    return m_petMode >> 8 & 0xFF;
  }
  static void ClearActions();
  static void SetAction(unsigned int index, PetAction &action, int save);
  static void UpdateCooldowns();
  static const PetAction *GetAction(unsigned int index) {
    return m_pet && index < 10 ? &m_actions[index] : 0;
  }
  static void ToggleAutocast(unsigned int index);
  static void            PutSpellInSlot(int spell, unsigned int slot) {
    PutActionInSlot(static_cast<unsigned int>(spell) | 0x01000000, slot);
  }
  static void PutActionInSlot(PetAction &action, unsigned int slot);
  static void            PutActionInSlot(unsigned int action, unsigned int slot) {
    PetAction petAction(action);
    PutActionInSlot(petAction, slot);
  }
  static const char *GetModeToken(unsigned int id);
  static const char *GetOrdersToken(unsigned int id);
  static void ShowGrid();
  static void SendPetAction(const PetAction &action, const unsigned __int64 &target);
  static void PetPassiveMode();
  static void PetDefensiveMode();
  static void PetAggressiveMode();
  static void PetWait();
  static void PetFollow();
  static void PetAttackTarget(const unsigned __int64 &targetGUID);
  static void PetDismiss();
  static void PetAbandon();
  static void PetRename(const char *newName);

 protected:
  static unsigned __int64 m_pet;
  static unsigned int     m_petMode;
  static PetAction        m_actions[10];
  static unsigned long    m_expirationTime;
};

#endif
