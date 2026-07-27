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
  int          pending;
  int          itemID;
  int          itemDisplayID;
  int          quantity;
  unsigned int slot;
};

class CGLootInfo {
 public:
  static void InitializeGame();
  static void ShutdownGame();
  static void EnterWorld();
  static void LeaveWorld();
  static void SetObject(CGObject_C *object, int coins, LOOT_ACQUIRE lootType);
  static const unsigned __int64 &GetObject() {
    return m_object;
  }
  static void ClearSlot(unsigned char _slot);
  static int GetNumItems();
  static int GetLootItem(unsigned int slot);
  static int GetLootQuantity(unsigned int slot);
  static int GetLootQuality(unsigned int slot);
  static int GetLootCoin(unsigned int slot);
  static const char *GetLootSlotTexture(unsigned int slot);
  static const char *GetLootSlotText(unsigned int slot);
  static const char *GetLootSlotLink(unsigned int slot, char *link, unsigned int size);
  static LOOT_ACQUIRE GetLootType();
  static int LootSlot(unsigned int slot, int force);
  static void CoinsCleared();

 protected:
  friend class CGGameUI;

  static unsigned __int64 m_object;
  static int              m_coins;
  static CGLootSlot       m_loot[16];
  static LOOT_ACQUIRE     m_lootType;
  static unsigned int     m_itemsPending;

  static int HasLoot();

  static void LootButtonItemStatsCallback(int id, const unsigned __int64 &guid, void *arg, bool granted);
};

#endif
