#include "Object/ObjectClient/Bag_C.h"

#include "Object/ObjectClient/Item_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"

struct GetItemTypeCountData {
  int entryID;
  int count;
};

struct FindItemClassData {
  int classID;
  int subclassMask;
};

static int GetItemTypeCountCallback(const CGItem_C *item, void *param) {
  GetItemTypeCountData *data = static_cast<GetItemTypeCountData *>(param);
  if (item->GetEntryID() == data->entryID || data->entryID == -1) {
    data->count += item->GetStackCount();
  }
  return 0;
}

static int FindItemIDCallback(const CGItem_C *item, void *param) {
  return item->GetEntryID() == *static_cast<int *>(param);
}

int CGBag_C::GetWidth(unsigned int offset) const {
  unsigned int width = NumSlots() - offset;
  return width > 4 ? 4 : width;
}

int CGBag_C::GetHeight(unsigned int offset) const {
  return (NumSlots() - offset + 3) / 4;
}

CGItem_C *CGBag_C::FindItemOfType(int entryID, unsigned int flags) const {
  unsigned __int64 bagGUID;
  unsigned int     slot;
  return FindItemOfType(entryID, bagGUID, slot, flags);
}

CGItem_C *CGBag_C::FindItemOfType(int entryID, unsigned __int64 &bagGUID, unsigned int &slot, unsigned int flags) const {
  return FindItem(FindItemIDCallback, &entryID, bagGUID, slot, flags);
}

int CGBag_C::GetItemTypeCount(int entryID, unsigned int flags) const {
  GetItemTypeCountData data;
  data.entryID = entryID;
  data.count = 0;
  FindItem(GetItemTypeCountCallback, &data, flags);
  return data.count;
}

static int FindItemClassCallback(const CGItem_C *item, void *param) {
  FindItemClassData *data = static_cast<FindItemClassData *>(param);
  return item->GetClassID() == data->classID && (data->subclassMask & (1 << item->GetSubtypeID()));
}

CGItem_C *CGBag_C::FindItemOfClass(int classID, int subclassMask, unsigned int flags) const {
  unsigned __int64 bagGUID;
  unsigned int     slot;
  FindItemClassData data;
  data.classID = classID;
  data.subclassMask = subclassMask;
  return FindItem(FindItemClassCallback, &data, bagGUID, slot, flags);
}

CGItem_C *CGBag_C::FindItemOfClass(int classID, int subclassMask, unsigned __int64 &bagGUID, unsigned int &slot, unsigned int flags) const {
  FindItemClassData data;
  data.classID = classID;
  data.subclassMask = subclassMask;
  return FindItem(FindItemClassCallback, &data, bagGUID, slot, flags);
}

CGItem_C *CGBag_C::FindItem(int(*func)(const CGItem_C *, void *), void *param, unsigned int flags) const {
  unsigned __int64 bagGUID;
  unsigned int     slot;
  return FindItem(func, param, bagGUID, slot, flags);
}

CGItem_C *CGBag_C::FindItem(
    int(*func)(const CGItem_C *, void *),
    void             *param,
    unsigned __int64 &bagGUID,
    unsigned int     &slot,
    unsigned int      flags
) const {
  FATALASSERT(func);

  unsigned int index;

  if (IsInventory() && !(flags & 7)) {
    flags |= 7;
  }

  for (index = 0; index < NumSlots(); ++index) {
    if (IsInventory() &&
        !((index <= 18 && (flags & 1)) ||
          (index >= 19 && index <= 22 && (flags & 2)) ||
          (index >= 23 && index <= 38 && (flags & 4)) ||
          (index >= 39 && index <= 62 && (flags & 8)))) {
      continue;
    }

    CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(GetItem(index), __FILE__, __LINE__));
    if (!item || item->IsDisabled()) {
      continue;
    }

    if (func(item, param)) {
      bagGUID = GetGUID();
      slot = index;
      return item;
    }

    CGBag_C *bag = item->GetBag();
    if (bag && !(flags & 0x10)) {
      CGItem_C *found = bag->FindItem(func, param, bagGUID, slot, flags);
      if (found) {
        return found;
      }
    }
  }

  bagGUID = 0;
  slot = 0;
  return 0;
}

GAME_ERROR_TYPE CGBag_C::GetGameError(BAG_RESULT result) {
  static const GAME_ERROR_TYPE errors[55] = {
      static_cast<GAME_ERROR_TYPE>(297), static_cast<GAME_ERROR_TYPE>(1),   static_cast<GAME_ERROR_TYPE>(2),   static_cast<GAME_ERROR_TYPE>(5),
      static_cast<GAME_ERROR_TYPE>(6),   static_cast<GAME_ERROR_TYPE>(8),   static_cast<GAME_ERROR_TYPE>(9),   static_cast<GAME_ERROR_TYPE>(4),
      static_cast<GAME_ERROR_TYPE>(10),  static_cast<GAME_ERROR_TYPE>(3),   static_cast<GAME_ERROR_TYPE>(3),   static_cast<GAME_ERROR_TYPE>(27),
      static_cast<GAME_ERROR_TYPE>(27),  static_cast<GAME_ERROR_TYPE>(140), static_cast<GAME_ERROR_TYPE>(11),  static_cast<GAME_ERROR_TYPE>(11),
      static_cast<GAME_ERROR_TYPE>(12),  static_cast<GAME_ERROR_TYPE>(15),  static_cast<GAME_ERROR_TYPE>(14),  static_cast<GAME_ERROR_TYPE>(13),
      static_cast<GAME_ERROR_TYPE>(15),  static_cast<GAME_ERROR_TYPE>(16),  static_cast<GAME_ERROR_TYPE>(17),  static_cast<GAME_ERROR_TYPE>(34),
      static_cast<GAME_ERROR_TYPE>(31),  static_cast<GAME_ERROR_TYPE>(18),  static_cast<GAME_ERROR_TYPE>(19),  static_cast<GAME_ERROR_TYPE>(137),
      static_cast<GAME_ERROR_TYPE>(20),  static_cast<GAME_ERROR_TYPE>(21),  static_cast<GAME_ERROR_TYPE>(7),   static_cast<GAME_ERROR_TYPE>(22),
      static_cast<GAME_ERROR_TYPE>(23),  static_cast<GAME_ERROR_TYPE>(24),  static_cast<GAME_ERROR_TYPE>(25),  static_cast<GAME_ERROR_TYPE>(26),
      static_cast<GAME_ERROR_TYPE>(113), static_cast<GAME_ERROR_TYPE>(114), static_cast<GAME_ERROR_TYPE>(297), static_cast<GAME_ERROR_TYPE>(250),
      static_cast<GAME_ERROR_TYPE>(251), static_cast<GAME_ERROR_TYPE>(254), static_cast<GAME_ERROR_TYPE>(253), static_cast<GAME_ERROR_TYPE>(256),
      static_cast<GAME_ERROR_TYPE>(255), static_cast<GAME_ERROR_TYPE>(258), static_cast<GAME_ERROR_TYPE>(259), static_cast<GAME_ERROR_TYPE>(116),
      static_cast<GAME_ERROR_TYPE>(0),   static_cast<GAME_ERROR_TYPE>(30),  static_cast<GAME_ERROR_TYPE>(29),  static_cast<GAME_ERROR_TYPE>(17),
      static_cast<GAME_ERROR_TYPE>(12),  static_cast<GAME_ERROR_TYPE>(297), static_cast<GAME_ERROR_TYPE>(18)
  };

  if (static_cast<unsigned int>(result) >= 55) {
    return static_cast<GAME_ERROR_TYPE>(297);
  }
  return errors[result];
}
