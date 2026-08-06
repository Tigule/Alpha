#ifndef WOW_SOURCE_UI_LOOTFRAME_H
#define WOW_SOURCE_UI_LOOTFRAME_H

class CGObject_C;

#ifdef GetObject
#undef GetObject
#endif

enum LOOT_ACQUIRE {
  LOOT_ACQUIRE_FAILED = 0,
  LOOT_ACQUIRE_NORMAL = 1,
  LOOT_ACQUIRE_PICKPOCKET = 2,
  LOOT_ACQUIRE_FISHING = 3
};

struct CGLootSlot {
  int  pending;
  int  itemID;
  int  itemDisplayID;
  int  quantity;
  BYTE slot;
};

class CGLootInfo {
 public:
  static void             InitializeGame();
  static void             ShutdownGame();
  static void             EnterWorld();
  static void             LeaveWorld();
  static void             SetObject(CGObject_C *object, int coins, LOOT_ACQUIRE lootType);
  static const DWORDLONG &GetObject() {
    return m_object;
  }
  static void         ClearSlot(BYTE _slot);
  static int          GetNumItems();
  static int          GetLootItem(UINT slot);
  static int          GetLootQuantity(UINT slot);
  static int          GetLootQuality(UINT slot);
  static int          GetLootCoin(UINT slot);
  static LPCSTR       GetLootSlotTexture(UINT slot);
  static LPCSTR       GetLootSlotText(UINT slot);
  static LPCSTR       GetLootSlotLink(UINT slot, char *link, UINT size);
  static LOOT_ACQUIRE GetLootType();
  static int          LootSlot(UINT slot, int force);
  static void         CoinsCleared();

 protected:
  friend class CGGameUI;

  static DWORDLONG    m_object;
  static int          m_coins;
  static CGLootSlot   m_loot[16];
  static LOOT_ACQUIRE m_lootType;
  static UINT         m_itemsPending;

  static int HasLoot();

  static void LootButtonItemStatsCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted);
};

#endif
