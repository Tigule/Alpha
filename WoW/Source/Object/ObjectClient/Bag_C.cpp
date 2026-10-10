#include <Base/Base.h>
#include <Gx/Gx.h>
#include <MapDefs.h>
#include <WorldClient/World.h>
#include "Net/NetClient/NetClient.h"
#include <Frame/CSimpleTop.h>
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "SoundInterface/SoundInterface.h"
#include "UIUtil/InputControl.h"
#include "Ui/WorldFrame.h"
#include "Ui/GameUI.h"

#include "Object/ObjectClient/Bag_C.h"

#include "Object/ObjectClient/Item_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"

struct GetItemTypeCountData {
  int entryID;
  int count;
};

struct FindItemClassData {
  FindItemClassData(int classID, int subclassMask) : classID(classID), subclassMask(subclassMask) {
  }

  int classID;
  int subclassMask;
};

static const UINT inventoryFlags = 7;

static BOOL GetItemTypeCountCallback(const CGItem_C *item, LPVOID param) {
  GetItemTypeCountData *data = (GetItemTypeCountData *)param;
  if (item->GetEntryID() == data->entryID || data->entryID == -1) {
    data->count += item->GetStackCount();
  }
  return 0;
}

static BOOL FindItemIDCallback(const CGItem_C *item, LPVOID param) {
  return item->GetEntryID() == *(int *)param;
}

int CGBag_C::GetWidth(UINT offset) const {
  UINT width = NumSlots() - offset;
  return width > 4 ? 4 : width;
}

int CGBag_C::GetHeight(UINT offset) const {
  return (NumSlots() - offset + 3) / 4;
}

CGItem_C *CGBag_C::FindItemOfType(int entryID, UINT flags) const {
  DWORDLONG bagGUID;
  UINT      slot;
  return FindItem(FindItemIDCallback, &entryID, bagGUID, slot, flags);
}

CGItem_C *CGBag_C::FindItemOfType(int entryID, DWORDLONG &bagGUID, UINT &slot, UINT flags) const {
  return FindItem(FindItemIDCallback, &entryID, bagGUID, slot, flags);
}

int CGBag_C::GetItemTypeCount(int entryID, UINT flags) const {
  GetItemTypeCountData data;
  data.entryID = entryID;
  data.count = 0;
  FindItem(GetItemTypeCountCallback, &data, flags);
  return data.count;
}

static BOOL FindItemClassCallback(const CGItem_C *item, LPVOID param) {
  FindItemClassData *data = (FindItemClassData *)param;
  return item->GetClassID() == data->classID && (!data->subclassMask || (data->subclassMask & (1 << item->GetSubtypeID())));
}

CGItem_C *CGBag_C::FindItemOfClass(int classID, int subclassMask, UINT flags) const {
  DWORDLONG         bagGUID;
  UINT              slot;
  FindItemClassData data(classID, subclassMask);
  return FindItem(FindItemClassCallback, &data, bagGUID, slot, flags);
}

CGItem_C *CGBag_C::FindItemOfClass(int classID, int subclassMask, DWORDLONG &bagGUID, UINT &slot, UINT flags) const {
  FindItemClassData data(classID, subclassMask);
  return FindItem(FindItemClassCallback, &data, bagGUID, slot, flags);
}

CGItem_C *CGBag_C::FindItem(BOOL (*func)(const CGItem_C *, LPVOID), LPVOID param, UINT flags) const {
  DWORDLONG bagGUID;
  UINT      slot;
  return FindItem(func, param, bagGUID, slot, flags);
}

CGItem_C *CGBag_C::FindItem(BOOL (*func)(const CGItem_C *, LPVOID), LPVOID param, DWORDLONG &bagGUID, UINT &slot, UINT flags) const {
  FATALASSERT(func);

  UINT index;

  if (IsInventory() && !(flags & inventoryFlags)) {
    flags |= inventoryFlags;
  }

  for (index = 0; index < NumSlots(); ++index) {
    if (IsInventory() && !((index <= EQUIPPED_LAST && (flags & 1)) || (index >= INVSLOT_BAGFIRST && index <= INVSLOT_BAGLAST && (flags & 2)) ||
                           (index >= BACKPACK_FIRST && index <= BACKPACK_LAST && (flags & 4)) ||
                           (index >= BANKGENERIC_FIRST && index <= BANKGENERIC_LAST && (flags & 8))))
    {
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

    if (item->GetBag() && !(flags & 0x10)) {
      CGItem_C *found = item->GetBag()->FindItem(func, param, bagGUID, slot, flags);
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
  switch (result) {
    case BAG_OK:
      return GERR_NONE;
    case BAG_LEVEL_MISMATCH:
      return GERR_CANT_EQUIP_LEVEL_I;
    case BAG_SKILL_MISMATCH:
      return GERR_CANT_EQUIP_SKILL;
    case BAG_PROFICIENCY_NEEDED:
      return GERR_PROFICIENCY_NEEDED;
    case BAG_SLOT_MISMATCH:
      return GERR_WRONG_SLOT;
    case BAG_INV_FULL:
      return GERR_INV_FULL;
    case BAG_NO_BAGS_IN_BAGS:
      return GERR_BAG_IN_BAG;
    case BAG_AMMO_ONLY:
      return GERR_AMMO_ONLY;
    case BAG_NO_SLOTS_AVAILABLE:
    case BAG_2HWEAPON_ITEMEXISTSINOFFHAND:
    case BAG_2HWEAPONBEINGWIELDED:
    case BAG_SLOT_NOT_EMPTY:
      return GERR_NO_SLOT_AVAILABLE;
    case BAG_CLASS_NOTALLOWED:
    case BAG_RACE_NOTALLOWED:
      return GERR_CANT_EQUIP_EVER;
    case BAG_2HWEAPON_SKILLNOTFOUND:
      return GERR_2HSKILLNOTFOUND;
    case BAG_ITEM_CLASS_MISMATCH:
    case BAG_ITEM_SUBTYPE_MISMATCH:
      return GERR_WRONG_BAG_TYPE;
    case BAG_ITEM_MAX_COUNT_EXCEEDED:
      return GERR_ITEM_MAX_COUNT;
    case BAG_NOT_EQUIPPABLE:
      return GERR_NOT_EQUIPPABLE;
    case BAG_CANT_SWAP:
      return GERR_CANT_SWAP;
    case BAG_SLOT_EMPTY:
      return GERR_SLOT_EMPTY;
    case BAG_ITEM_NOT_FOUND:
    case BAG_UNKNOWN_ITEM:
      return GERR_ITEM_NOT_FOUND;
    case BAG_ITEM_ALREADY_BOUND:
      return GERR_DROP_BOUND_ITEM;
    case BAG_DROP_TOO_FAR_AWAY:
      return GERR_OUT_OF_RANGE;
    case BAG_ITEM_TOO_FEW_TO_SPLIT:
      return GERR_TOO_FEW_TO_SPLIT;
    case BAG_ITEM_SPLIT_FAILED:
      return GERR_SPLIT_FAILED;
    case BAG_NOT_ENOUGH_GOLD:
      return GERR_NOT_ENOUGH_GOLD;
    case BAG_NOT_A_CONTAINER:
      return GERR_NOT_A_BAG;
    case BAG_NOT_EMPTY:
      return GERR_DESTROY_NONEMPTY_BAG;
    case BAG_NOT_OWNER:
      return GERR_NOT_OWNER;
    case BAG_ONLY_ONE_QUIVER:
      return GERR_ONLY_ONE_QUIVER;
    case BAG_ONLY_ONE_BOLT:
      return GERR_ONLY_ONE_BOLT;
    case BAG_ONLY_ONE_AMMO:
      return GERR_ONLY_ONE_AMMO;
    case BAG_NOBANKSLOT:
      return GERR_NO_BANK_SLOT;
    case BAG_NOBANKHERE:
      return GERR_NO_BANK_HERE;
    case BAG_ITEM_LOCKED:
      return GERR_ITEM_LOCKED;
    case BAG_NOT_WHILE_DEAD:
      return GERR_PLAYER_DEAD;
    case BAG_CLIENT_LOCKED_OUT:
      return GERR_CLIENT_LOCKED_OUT;
    case BAG_CANT_WRAP_STACKABLE:
      return GERR_CANT_WRAP_STACKABLE;
    case BAG_CANT_WRAP_EQUIPPED:
      return GERR_CANT_WRAP_EQUIPPED;
    case BAG_CANT_WRAP_WRAPPED:
      return GERR_CANT_WRAP_WRAPPED;
    case BAG_CANT_WRAP_BOUND:
      return GERR_CANT_WRAP_BOUND;
    case BAG_CANT_WRAP_UNIQUE:
      return GERR_CANT_WRAP_UNIQUE;
    case BAG_CANT_WRAP_BAGS:
      return GERR_CANT_WRAP_BAGS;
    case BAG_LOOT_GONE:
      return GERR_LOOT_GONE;
    case BAG_SOLD_OUT:
    case BAG_DONT_HAVE_THAT_MANY:
      return GERR_VENDOR_SOLD_OUT;
    case BAG_CANT_STACK:
    case BAG_STACK_COUNT_EXCEEDED:
      return GERR_CANT_STACK;
    default:
      return GERR_BAG_FULL;
  }
}
