#ifndef WOW_SOURCE_UI_MERCHANTFRAME_H
#define WOW_SOURCE_UI_MERCHANTFRAME_H

class ItemStats;

struct VendorItem {
  UINT m_muid;
  UINT m_itemType;
  UINT m_itemDisplayID;
  int  m_quantity;
  int  m_price;
  int  m_durability;
  int  m_stackCount;
};

class CGMerchantInfo {
 public:
  static void      EnterWorld();
  static void      LeaveWorld();
  static void      SetMerchant(DWORDLONG merchantGUID, VendorItem *items, int count);
  static DWORDLONG GetMerchant() {
    return m_merchant;
  }
  static void CloseMerchant();
  static void UpdateItemQuantity(DWORDLONG vendor, DWORD muid, int newQuantity);
  static int  GetNumItems() {
    return m_itemCount;
  }
  static const VendorItem *GetItem(int index) {
    return index >= 0 && index < m_itemCount ? &m_items[index] : 0;
  }
  static const ItemStats *GetItemStats(UINT itemID);
  static void             DecrementCallbackCount();

 protected:
  static DWORDLONG  m_merchant;
  static VendorItem m_items[128];
  static int        m_itemCount;
  static UINT       m_callbackCount;
};

#endif
