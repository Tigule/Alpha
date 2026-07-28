#pragma once

#include "Ui/GameUI.h"

class CGItem_C;

enum BAG_RESULT {
  BAG_OK = 0,
  BAG_LEVEL_MISMATCH = 1,
  BAG_SKILL_MISMATCH = 2,
  BAG_SLOT_MISMATCH = 3,
  BAG_FULL = 4,
  BAG_NO_BAGS_IN_BAGS = 5,
  BAG_AMMO_ONLY = 6,
  BAG_PROFICIENCY_NEEDED = 7,
  BAG_NO_SLOTS_AVAILABLE = 8,
  BAG_CLASS_NOTALLOWED = 9,
  BAG_RACE_NOTALLOWED = 10,
  BAG_2HWEAPON_ITEMEXISTSINOFFHAND = 11,
  BAG_2HWEAPONBEINGWIELDED = 12,
  BAG_2HWEAPON_SKILLNOTFOUND = 13,
  BAG_ITEM_CLASS_MISMATCH = 14,
  BAG_ITEM_SUBTYPE_MISMATCH = 15,
  BAG_ITEM_MAX_COUNT_EXCEEDED = 16,
  BAG_SLOT_NOT_EMPTY = 17,
  BAG_CANT_STACK = 18,
  BAG_NOT_EQUIPPABLE = 19,
  BAG_CANT_SWAP = 20,
  BAG_SLOT_EMPTY = 21,
  BAG_ITEM_NOT_FOUND = 22,
  BAG_ITEM_ALREADY_BOUND = 23,
  BAG_DROP_TOO_FAR_AWAY = 24,
  BAG_ITEM_TOO_FEW_TO_SPLIT = 25,
  BAG_ITEM_SPLIT_FAILED = 26,
  BAG_CANT_CAST_ENCHANTMENT = 27,
  BAG_NOT_ENOUGH_GOLD = 28,
  BAG_NOT_A_CONTAINER = 29,
  BAG_NOT_EMPTY = 30,
  BAG_NOT_OWNER = 31,
  BAG_ONLY_ONE_QUIVER = 32,
  BAG_NOBANKSLOT = 33,
  BAG_NOBANKHERE = 34,
  BAG_ITEM_LOCKED = 35,
  BAG_NOT_WHILE_DEAD = 36,
  BAG_CLIENT_LOCKED_OUT = 37,
  BAG_ERROR = 38,
  BAG_ONLY_ONE_BOLT = 39,
  BAG_ONLY_ONE_AMMO = 40,
  BAG_CANT_WRAP_STACKABLE = 41,
  BAG_CANT_WRAP_EQUIPPED = 42,
  BAG_CANT_WRAP_WRAPPED = 43,
  BAG_CANT_WRAP_BOUND = 44,
  BAG_CANT_WRAP_UNIQUE = 45,
  BAG_CANT_WRAP_BAGS = 46,
  BAG_LOOT_GONE = 47,
  BAG_INV_FULL = 48,
  BAG_SOLD_OUT = 49,
  BAG_DONT_LIKE_YOU = 50,
  BAG_UNKNOWN_ITEM = 51,
  BAG_STACK_COUNT_EXCEEDED = 52,
  BAG_QUANTITY_ZERO = 53,
  BAG_DONT_HAVE_THAT_MANY = 54
};

class CGBag {
 public:
  CGBag(unsigned __int64 guid, unsigned int *slotCount, unsigned __int64 *slots, unsigned char isInventory)
      : m_slotCount(slotCount), m_slots(slots), m_guid(guid), m_isInventory(isInventory) {
  }

  unsigned __int64 GetItem(unsigned int slot) const {
    return slot < *m_slotCount ? m_slots[slot] : 0;
  }
  int GetIndexOfObject(unsigned __int64 guid) const {
    for (unsigned int index = 0; index < NumSlots(); ++index) {
      if (GetItem(index) == guid) {
        return index;
      }
    }
    return -1;
  }
  unsigned int NumItems() const;
  unsigned int NumSlots() const {
    return *m_slotCount;
  }
  int IsInventory() const {
    return m_isInventory;
  }
  unsigned __int64 GetGUID() const {
    return m_guid;
  }

 protected:
  unsigned int     *m_slotCount;
  unsigned __int64 *m_slots;
  unsigned __int64  m_guid;
  unsigned char     m_isInventory;
};

class CGBag_C : public CGBag {
 public:
  CGBag_C(unsigned __int64 guid, unsigned int *slotCount, unsigned __int64 *slots, unsigned char isInventory)
      : CGBag(guid, slotCount, slots, isInventory) {
  }
  ~CGBag_C() {
  }

  int                               GetItemTypeCount(int entryID, unsigned int flags) const;
  int                               GetWidth(unsigned int offset) const;
  int                               GetHeight(unsigned int offset) const;
  static GAME_ERROR_TYPE GetGameError(BAG_RESULT result);
  CGItem_C                         *FindItem(int(*func)(const CGItem_C *, void *), void *param, unsigned int flags) const;
  CGItem_C                         *FindItemOfType(int entryID, unsigned int flags) const;
  CGItem_C                         *FindItemOfType(int entryID, unsigned __int64 &bagGUID, unsigned int &slot, unsigned int flags) const;
  CGItem_C                         *FindItemOfClass(int classID, int subclassMask, unsigned int flags) const;
  CGItem_C *FindItemOfClass(int classID, int subclassMask, unsigned __int64 &bagGUID, unsigned int &slot, unsigned int flags) const;
  CGItem_C *
  FindItem(int(*func)(const CGItem_C *, void *), void *param, unsigned __int64 &bagGUID, unsigned int &slot, unsigned int flags) const;
};
