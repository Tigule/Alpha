#ifndef WOW_SOURCE_UI_PETINFO_H
#define WOW_SOURCE_UI_PETINFO_H

class PetAction {
 public:
  PetAction(unsigned int action = 0) : m_action(action) {
  }

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
  static void __fastcall  InitializeGame();
  static void __fastcall  EnterWorld();
  static void __fastcall  LeaveWorld();
  static void __fastcall  ShutdownGame();
  static void __fastcall  HideGrid();
  static void __fastcall  SetPet(unsigned __int64 pet, unsigned long expirationTime);
  static void __fastcall  SetPetModeAndOrders(unsigned int petMode);
  static unsigned __int64 GetPet() {
    return m_pet;
  }
  static unsigned long GetExpirationTime() {
    return m_expirationTime;
  }
  static void __fastcall SetPetMode(unsigned int mode);
  static unsigned int    GetPetMode() {
    return m_petMode & 0xFF;
  }
  static void __fastcall SetPetOrders(unsigned int orders);
  static unsigned int    GetPetOrders() {
    return m_petMode >> 8 & 0xFF;
  }
  static void __fastcall ClearActions();
  static void __fastcall SetAction(unsigned int index, PetAction &action, int save);
  static void __fastcall UpdateCooldowns();
  static PetAction      *GetAction(unsigned int index) {
    return index < 10 ? &m_actions[index] : 0;
  }
  static void __fastcall ToggleAutocast(unsigned int index);
  static void            PutSpellInSlot(int spell, unsigned int slot) {
    PutActionInSlot(static_cast<unsigned int>(spell) | 0x01000000, slot);
  }
  static void __fastcall PutActionInSlot(PetAction &action, unsigned int slot);
  static void            PutActionInSlot(unsigned int action, unsigned int slot) {
    PetAction petAction(action);
    PutActionInSlot(petAction, slot);
  }
  static const char *__fastcall GetModeToken(unsigned int id);
  static const char *__fastcall GetOrdersToken(unsigned int id);
  static void __fastcall        ShowGrid();
  static void __fastcall        SendPetAction(PetAction &action, const unsigned __int64 &target);
  static void __fastcall        PetPassiveMode();
  static void __fastcall        PetDefensiveMode();
  static void __fastcall        PetAggressiveMode();
  static void __fastcall        PetWait();
  static void __fastcall        PetFollow();
  static void __fastcall        PetAttackTarget(const unsigned __int64 &targetGUID);
  static void __fastcall        PetDismiss();
  static void __fastcall        PetAbandon();
  static void __fastcall        PetRename(const char *newName);

 protected:
  static unsigned __int64 m_pet;
  static unsigned int     m_petMode;
  static PetAction        m_actions[10];
  static unsigned long    m_expirationTime;
};

#endif
