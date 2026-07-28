#ifndef WOW_SOURCE_UI_GUILDREGISTRAR_H
#define WOW_SOURCE_UI_GUILDREGISTRAR_H

struct PetitionVendorItem {
  unsigned int m_muid;
  unsigned int m_itemID;
  unsigned int m_itemDisplayID;
  int          m_price;
  int          m_flags;
};

class CGGuildRegistrar {
 public:
  static void EnterWorld();
  static void LeaveWorld();
  static void SetRegistrar(unsigned __int64 registrar, const PetitionVendorItem *petition);
  static void CloseRegistrar();
  static unsigned __int64 GetRegistrar() {
    return m_registrar;
  }
  static unsigned int GetGuildCharterCost();
  static void BuyGuildCharter(const char *guildName);

 protected:
  static unsigned __int64   m_registrar;
  static PetitionVendorItem m_petition;
};

#endif
