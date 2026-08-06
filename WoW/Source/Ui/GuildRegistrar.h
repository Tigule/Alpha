#ifndef WOW_SOURCE_UI_GUILDREGISTRAR_H
#define WOW_SOURCE_UI_GUILDREGISTRAR_H

struct PetitionVendorItem {
  UINT m_muid;
  UINT m_itemID;
  UINT m_itemDisplayID;
  int  m_price;
  int  m_flags;
};

class CGGuildRegistrar {
 public:
  static void      EnterWorld();
  static void      LeaveWorld();
  static void      SetRegistrar(DWORDLONG registrar, const PetitionVendorItem *petition);
  static void      CloseRegistrar();
  static DWORDLONG GetRegistrar() {
    return m_registrar;
  }
  static UINT GetGuildCharterCost();
  static void BuyGuildCharter(LPCSTR guildName);

 protected:
  static DWORDLONG          m_registrar;
  static PetitionVendorItem m_petition;
};

#endif
