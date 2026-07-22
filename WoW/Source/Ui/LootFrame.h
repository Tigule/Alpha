#ifndef WOW_SOURCE_UI_LOOTFRAME_H
#define WOW_SOURCE_UI_LOOTFRAME_H

class CGObject_C;

enum LOOT_ACQUIRE {
  LOOT_ACQUIRE_FAILED = 0,
  LOOT_ACQUIRE_NORMAL = 1,
  LOOT_ACQUIRE_PICKPOCKET = 2,
  LOOT_ACQUIRE_FISHING = 3
};

struct CGLootSlot {
  int          pending;
  int          itemID;
  int          itemDisplayID;
  int          quantity;
  unsigned int slot;
};

class CGLootInfo {
 public:
  static void __fastcall         InitializeGame();
  static void __fastcall         ShutdownGame();
  static void __fastcall         EnterWorld();
  static void __fastcall         LeaveWorld();
  static void __fastcall         SetObject(CGObject_C *object, int coins, LOOT_ACQUIRE lootType);
  static const unsigned __int64 &GetObject() {
    return m_object;
  }
  static void __fastcall         ClearSlot(unsigned int _slot);
  static int __fastcall          GetNumItems();
  static int __fastcall          GetLootItem(unsigned int slot);
  static int __fastcall          GetLootQuantity(unsigned int slot);
  static int __fastcall          GetLootQuality(unsigned int slot);
  static int __fastcall          GetLootCoin(unsigned int slot);
  static const char *__fastcall  GetLootSlotTexture(unsigned int slot);
  static const char *__fastcall  GetLootSlotText(unsigned int slot);
  static const char *__fastcall  GetLootSlotLink(unsigned int slot, char *link, unsigned int size);
  static LOOT_ACQUIRE __fastcall GetLootType();
  static int __fastcall          LootSlot(unsigned int slot, int force);
  static void __fastcall         CoinsCleared();

 protected:
  friend class CGGameUI;

  static unsigned __int64 m_object;
  static int              m_coins;
  static CGLootSlot       m_loot[16];
  static LOOT_ACQUIRE     m_lootType;
  static unsigned int     m_itemsPending;

  static int __fastcall HasLoot();

  static void __fastcall LootButtonItemStatsCallback(int id, const unsigned __int64 &guid, void *arg, bool granted);
};

#endif
