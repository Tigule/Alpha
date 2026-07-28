#ifndef WOW_SOURCE_UI_MERCHANTFRAME_H
#define WOW_SOURCE_UI_MERCHANTFRAME_H

class ItemStats;

struct VendorItem {
  unsigned int m_muid;
  unsigned int m_itemType;
  unsigned int m_itemDisplayID;
  int          m_quantity;
  int          m_price;
  int          m_durability;
  int          m_stackCount;
};

class CGMerchantInfo {
 public:
  static void EnterWorld();
  static void LeaveWorld();
  static void SetMerchant(unsigned __int64 merchantGUID, VendorItem *items, int count);
  static unsigned __int64 GetMerchant() {
    return m_merchant;
  }
  static void CloseMerchant();
  static void UpdateItemQuantity(unsigned __int64 vendor, unsigned long muid, int newQuantity);
  static int GetNumItems() {
    return m_itemCount;
  }
  static const VendorItem *GetItem(int index) {
    return index >= 0 && index < m_itemCount ? &m_items[index] : 0;
  }
  static const ItemStats *GetItemStats(unsigned int itemID);
  static void DecrementCallbackCount();

 protected:
  static unsigned __int64 m_merchant;
  static VendorItem       m_items[128];
  static int              m_itemCount;
  static unsigned int     m_callbackCount;
};

#endif
