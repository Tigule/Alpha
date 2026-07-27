#pragma once

#include "Ui/GameUI.h"

class CGItem_C;

enum BAG_RESULT {
  BAG_OK = 0,
  BAG_LEVEL_MISMATCH = 1,
  BAG_ITEM_SUBTYPE_MISMATCH = 15
};

class CGBag {
 public:
  CGBag(unsigned __int64 guid, unsigned int *slotCount, unsigned __int64 *slots, unsigned int isInventory)
      : m_slotCount(slotCount), m_slots(slots), m_guid(guid), m_isInventory(isInventory) {
  }

  unsigned __int64 GetItem(unsigned int slot) const {
    return slot < *m_slotCount ? m_slots[slot] : 0;
  }
  unsigned int NumSlots() const {
    return *m_slotCount;
  }
  unsigned __int64 GetGUID() const {
    return m_guid;
  }

 protected:
  unsigned int     *m_slotCount;
  unsigned __int64 *m_slots;
  unsigned __int64  m_guid;
  unsigned int      m_isInventory;
};

class CGBag_C : public CGBag {
 public:
  CGBag_C(unsigned __int64 guid, unsigned int *slotCount, unsigned __int64 *slots, unsigned int isInventory)
      : CGBag(guid, slotCount, slots, isInventory) {
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
