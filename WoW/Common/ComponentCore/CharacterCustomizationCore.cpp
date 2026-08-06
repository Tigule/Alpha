#include <Component/CharacterCustomization.h>
#include <Component/Component.h>

#include <DB/DBClient/AutoCode/CharHairGeosetsRec.h>
#include <DB/DBClient/AutoCode/CharTextureVariationsV2Rec.h>
#include <DB/DBClient/AutoCode/CharVariationsRec.h>
#include <DB/DBClient/AutoCode/CharacterFacialHairStylesRec.h>
#include <DB/DBClient/AutoCode/ChrRacesRec.h>
#include <DB/DBClient/AutoCode/ItemDisplayInfoRec.h>

#include <Base/Status.h>
#include <Model/IModel.h>
#include <Services/Texture.h>
#include <WorldClient/World.h>

#include <malloc.h>
#include <string.h>

static TSFixedArray<TSFixedArray<VARIATIONS> >  s_characterVariations;
static TSFixedArray<CAMERAFILENAMES>            s_cameraFileNames;
static TSGrowableArray<CHARACTERRACEVARIATIONS> s_raceTextureFileNames;
static const UINT                               NUM_UNDERWEARHIDESECTIONS = 2;
static const UINT                               s_defaultGeosets[NUM_CHARGEOSETS] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0};
UINT                                            g_defaultGeosetIDOffsets[NUM_CHARGEOSETS] = {1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0};
static const UINT                               s_baseGeosets[NUM_CHARGEOSETS] = {1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0};
static CHARACTER_GEOSET_SECTIONS                s_clothingGeosetRanges[9] = {CHARGEOSET_GLOVE,  CHARGEOSET_BOOT,    CHARGEOSET_SLEEVES,
                                                                             CHARGEOSET_PANTS,  CHARGEOSET_DOUBLET, CHARGEOSET_PANTDOUBLET,
                                                                             CHARGEOSET_TABARD, CHARGEOSET_ROBE,    CHARGEOSET_LOINCLOTH};

static const UINT s_itemGeosetPriorities[INDEX_NUMSLOTS][9] = {
    {0, 0,  0, 0,  0,  0, 0, 0, 0},
    {0, 0,  0, 0,  0,  0, 0, 0, 0},
    {0, 0,  0, 0,  0,  0, 0, 0, 0},
    {0, 0,  0, 0,  0,  0, 0, 0, 0},
    {0, 0,  0, 0,  0,  0, 0, 0, 0},
    {0, 0, 10, 0, 10, 10, 0, 0, 0},
    {0, 0,  0, 0,  0,  0, 0, 0, 0},
    {0, 0,  0, 0,  0,  0, 0, 0, 0},
    {0, 0,  0, 0,  0,  0, 0, 0, 0},
    {0, 0,  0, 0,  0,  0, 0, 0, 0},
    {0, 0,  0, 0,  0,  0, 0, 0, 0},
    {0, 0,  0, 0,  0,  0, 0, 0, 0},
    {0, 0,  0, 0,  0,  0, 0, 0, 0},
    {0, 0,  0, 0,  0,  0, 0, 0, 0},
    {0, 0,  0, 0,  0,  0, 0, 0, 0},
    {0, 0,  0, 0,  0,  0, 0, 0, 0},
    {0, 0,  0, 0,  0,  0, 0, 0, 0},
    {0, 0,  0, 0,  0,  0, 0, 0, 0},
    {0, 0,  0, 0,  0,  0, 0, 0, 0},
    {0, 0,  0, 0,  0,  0, 0, 0, 0},
    {0, 0, 11, 0,  0,  0, 0, 0, 0},
    {0, 0,  0, 0,  0,  0, 0, 0, 0},
    {0, 0,  0, 0,  0,  0, 0, 0, 0},
    {0, 0,  0, 0,  0,  0, 0, 0, 0},
    {0, 0,  0, 0,  0,  0, 0, 0, 0},
    {0, 0,  0, 0,  0,  0, 0, 0, 0},
    {0, 0,  0, 0,  0,  0, 0, 0, 0}
};

static const int s_overridePriorities[INDEX_NUMSLOTS][9] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0}
};

static const UINT s_inventoryAndGeosetDisables[INDEX_NUMSLOTS][9] = {
    {0, 0, 0, 0, 0, 0,  0,   0, 0},
    {0, 0, 0, 0, 0, 0,  0,   0, 0},
    {0, 0, 0, 0, 0, 0,  0,   0, 0},
    {0, 0, 0, 0, 0, 0,  0,   0, 0},
    {0, 0, 0, 0, 0, 0,  0,   0, 0},
    {0, 0, 0, 0, 0, 0, 48, 122, 0},
    {0, 0, 0, 0, 0, 0,  0,   0, 0},
    {0, 0, 0, 0, 0, 0,  0, 106, 0},
    {0, 8, 0, 0, 0, 0,  0,   0, 0},
    {0, 0, 0, 0, 0, 0,  0,   0, 0},
    {4, 0, 0, 0, 0, 0,  0,   0, 0},
    {0, 0, 0, 0, 0, 0,  0,   0, 0},
    {0, 0, 0, 0, 0, 0,  0,   0, 0},
    {0, 0, 0, 0, 0, 0,  0,   0, 0},
    {0, 0, 0, 0, 0, 0,  0,   0, 0},
    {0, 0, 0, 0, 0, 0,  0,   0, 0},
    {0, 0, 0, 0, 0, 0,  0,   0, 0},
    {0, 0, 0, 0, 0, 0,  0,   0, 0},
    {0, 0, 0, 0, 0, 0,  0,   0, 0},
    {0, 0, 0, 0, 0, 0, 48,   0, 0},
    {0, 0, 0, 0, 0, 0, 48, 122, 0},
    {0, 0, 0, 0, 0, 0,  0,   0, 0},
    {0, 0, 0, 0, 0, 0,  0,   0, 0},
    {0, 0, 0, 0, 0, 0,  0,   0, 0},
    {0, 0, 0, 0, 0, 0,  0,   0, 0},
    {0, 0, 0, 0, 0, 0,  0,   0, 0},
    {0, 0, 0, 0, 0, 0,  0,   0, 0}
};

static UINT s_geosetGroupDisables[9] = {0, 0, 0, 0, 0, 0, 0, 2, 0};

struct HOLDINFO {
  CHARACTER_ITEM_GEOSETS geosetGroup;
  TEXCOMPONENT_SECTIONS  holdSection;
};

struct INVHOLDINFO {
  INVHOLDINFO(CHARACTER_ITEM_GEOSETS geoset0, TEXCOMPONENT_SECTIONS section0, CHARACTER_ITEM_GEOSETS geoset1, TEXCOMPONENT_SECTIONS section1) {
    holdInfo[0].geosetGroup = geoset0;
    holdInfo[0].holdSection = section0;
    holdInfo[1].geosetGroup = geoset1;
    holdInfo[1].holdSection = section1;
  }

  HOLDINFO holdInfo[2];
};

static const INVHOLDINFO s_itemTypeTextureHolds[INDEX_NUMSLOTS] = {
    INVHOLDINFO(INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION, INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION),
    INVHOLDINFO(INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION, INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION),
    INVHOLDINFO(INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION, INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION),
    INVHOLDINFO(INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION, INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION),
    INVHOLDINFO(CHARITEMGEOSETS_DOUBLET, TCS_LEGUPPER, INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION),
    INVHOLDINFO(CHARITEMGEOSETS_TABARD, TCS_LEGUPPER, CHARITEMGEOSETS_DOUBLET, TCS_LEGUPPER),
    INVHOLDINFO(INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION, INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION),
    INVHOLDINFO(INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION, INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION),
    INVHOLDINFO(INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION, INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION),
    INVHOLDINFO(INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION, INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION),
    INVHOLDINFO(INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION, INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION),
    INVHOLDINFO(INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION, INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION),
    INVHOLDINFO(INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION, INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION),
    INVHOLDINFO(INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION, INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION),
    INVHOLDINFO(INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION, INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION),
    INVHOLDINFO(INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION, INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION),
    INVHOLDINFO(INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION, INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION),
    INVHOLDINFO(INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION, INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION),
    INVHOLDINFO(INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION, INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION),
    INVHOLDINFO(CHARITEMGEOSETS_TABARD, TCS_LEGUPPER, INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION),
    INVHOLDINFO(INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION, INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION),
    INVHOLDINFO(INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION, INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION),
    INVHOLDINFO(INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION, INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION),
    INVHOLDINFO(INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION, INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION),
    INVHOLDINFO(INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION, INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION),
    INVHOLDINFO(INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION, INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION),
    INVHOLDINFO(INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION, INVALID_CHARITEMGEOSET, TCS_INVALIDSECTION)
};

class CharGeosetInfo {
 public:
  CharGeosetInfo();
  CharGeosetInfo(const CharGeosetInfo &rhs);

  void Clear();

  int ShowingSameGeosetsAs(const CharGeosetInfo &rhs);

  void ShowInventoryTypeTextureHolds(HTEXCOMPONENT component, UINT inventoryType, int adding) {
    FATALASSERT(component);
    FATALASSERT(inventoryType != INDEX_NON_EQUIP_TYPE);
    UINT i;
    for (i = 0; i < 2; ++i) {
      const HOLDINFO &hold = s_itemTypeTextureHolds[inventoryType].holdInfo[i];
      if (hold.geosetGroup != -1 && hold.holdSection != TCS_INVALIDSECTION && (flags[hold.geosetGroup] & 1) &&
          !(disabledByFlags[hold.geosetGroup] & (1u << inventoryType)) && currentGeosets[hold.geosetGroup] <= 1)
      {
        if (adding) {
          TexComponentAddHold(component, static_cast<INVENTORY_TYPES>(inventoryType), hold.holdSection);
        } else {
          TexComponentRemoveHold(component, static_cast<INVENTORY_TYPES>(inventoryType), hold.holdSection);
        }
      }
    }
  }

  void UpdateGeosetDisplay(const ItemDisplayInfoRec *displayInfoRec, UINT itemInventoryType, HTEXCOMPONENT component, UINT playerRace) {
    if (!displayInfoRec || !playerRace || playerRace > static_cast<UINT>(g_chrRacesDB.GetMaxID())) {
      return;
    }

    const ChrRacesRec *raceRec = g_chrRacesDB.GetRecord(playerRace);
    if (!raceRec || ((raceRec->m_flags & 2) && itemInventoryType == INDEX_FEET_TYPE)) {
      return;
    }

    UINT i;
    for (i = 0; i < 4; ++i) {
      CHARACTER_ITEM_GEOSETS group = g_geosetGroupsPerItem[itemInventoryType].geosetGroup[i];
      if (group == INVALID_CHARITEMGEOSET) {
        continue;
      }

      int geoset = displayInfoRec->m_geosetGroup[i];
      if (geoset) {
        inventoryTypeGeosets[group][itemInventoryType] = geoset + 1;
      }

      int priority = s_itemGeosetPriorities[itemInventoryType][group];
      if (priority >= highestPriority[group]) {
        highestPriority[group] = priority;
        if (geoset || s_overridePriorities[itemInventoryType][group]) {
          currentGeosets[group] = geoset + 1;
          geosetCurrentlyUsedBy[group] = itemInventoryType;
        }
      }
    }

    LPCSTR legLowerTexture = CompUtilGetTextureSectionName(displayInfoRec, TCS_LEGLOWER);
    if (itemInventoryType == INDEX_FEET_TYPE) {
      if (legLowerTexture && *legLowerTexture) {
        flags[3] |= 1;
        disabledByFlags[3] |= 1u << INDEX_FEET_TYPE;
      }
    } else if (itemInventoryType == INDEX_LEGS_TYPE && !flags[3] && legLowerTexture && *legLowerTexture) {
      flags[3] |= 1;
      disabledByFlags[3] |= 1u << INDEX_LEGS_TYPE;
    }

    UINT group;
    for (group = 0; group < 9; ++group) {
      if (s_geosetGroupDisables[group]) {
        UINT disabledGroup;
        for (disabledGroup = 0; disabledGroup < 9; ++disabledGroup) {
          if (currentGeosets[group] > 1 && (s_geosetGroupDisables[group] & (1u << disabledGroup))) {
            flags[disabledGroup] |= 2;
            disabledByFlags[disabledGroup] |= 1u << geosetCurrentlyUsedBy[group];
          }
        }
      }

      UINT disables = s_inventoryAndGeosetDisables[itemInventoryType][group];
      if (group >= 5 && group <= 7) {
        disables |= 1u << INDEX_FEET_TYPE;
      }
      if (currentGeosets[group] > 1 && disables) {
        UINT disabledGroup;
        for (disabledGroup = 0; disabledGroup < 9 && disables; ++disabledGroup) {
          if (disables & (1u << disabledGroup)) {
            flags[disabledGroup] |= 1;
            disabledByFlags[disabledGroup] |= 1u << geosetCurrentlyUsedBy[group];
            disables &= ~(1u << disabledGroup);
          }
        }
      }
    }

    if (component) {
      if (currentGeosets[7] > 1) {
        TexComponentAddHold(component, INDEX_FEET_TYPE, TCS_LEGLOWER);
      }
      if (currentGeosets[2] > 1 && !(flags[2] & 3) && currentGeosets[0] <= 1) {
        TexComponentAddHold(component, INDEX_HAND_TYPE, TCS_LOWERARM);
      } else {
        TexComponentRemoveHold(component, INDEX_HAND_TYPE, TCS_LOWERARM);
      }
      ShowInventoryTypeTextureHolds(component, itemInventoryType, 1);
    }
  }

  void RemoveGeosetInfo(const ItemDisplayInfoRec *displayInfoRec, UINT inventoryType, HTEXCOMPONENT component) {
    FATALASSERT(inventoryType != INDEX_NON_EQUIP_TYPE);

    UINT oldGeosets[9];
    memcpy(oldGeosets, currentGeosets, sizeof(oldGeosets));

    UINT group;
    for (group = 0; group < 9; ++group) {
      inventoryTypeGeosets[group][inventoryType] = 0;
      geosetCurrentlyUsedBy[group] = INDEX_NON_EQUIP_TYPE;
      highestPriority[group] = -1;

      UINT bestInventoryType = INDEX_NON_EQUIP_TYPE;
      UINT candidateInventoryType;
      for (candidateInventoryType = 0; candidateInventoryType < INDEX_NUMSLOTS; ++candidateInventoryType) {
        if (inventoryTypeGeosets[group][candidateInventoryType] &&
            static_cast<int>(s_itemGeosetPriorities[candidateInventoryType][group]) > highestPriority[group])
        {
          highestPriority[group] = s_itemGeosetPriorities[candidateInventoryType][group];
          bestInventoryType = candidateInventoryType;
        }
      }

      if (bestInventoryType != INDEX_NON_EQUIP_TYPE) {
        currentGeosets[group] = inventoryTypeGeosets[group][bestInventoryType];
        geosetCurrentlyUsedBy[group] = bestInventoryType;
        continue;
      }

      currentGeosets[group] = 1;

      UINT hideFlags = s_geosetGroupDisables[group];
      if (hideFlags) {
        UINT disabledGroup;
        for (disabledGroup = 0; disabledGroup < 9; ++disabledGroup) {
          if (hideFlags & (1u << disabledGroup)) {
            flags[disabledGroup] &= ~2u;
            currentGeosets[disabledGroup] = inventoryTypeGeosets[disabledGroup][geosetCurrentlyUsedBy[disabledGroup]];
            if (!currentGeosets[disabledGroup]) {
              currentGeosets[disabledGroup] = 1;
            }
            disabledByFlags[disabledGroup] &= ~(1u << inventoryType);
          }
        }
      }

      UINT thisGroupDisablesFlags = s_inventoryAndGeosetDisables[inventoryType][group];
      if (group >= 5 && group <= 7) {
        thisGroupDisablesFlags |= 1u << INDEX_FEET_TYPE;
      }
      if (thisGroupDisablesFlags) {
        UINT disabledGroup;
        for (disabledGroup = 0; disabledGroup < 9; ++disabledGroup) {
          if ((thisGroupDisablesFlags & (1u << disabledGroup)) && (flags[disabledGroup] & 1)) {
            currentGeosets[disabledGroup] = inventoryTypeGeosets[disabledGroup][geosetCurrentlyUsedBy[disabledGroup]];
            if (!currentGeosets[disabledGroup]) {
              currentGeosets[disabledGroup] = 1;
            }
            disabledByFlags[disabledGroup] &= ~(1u << inventoryType);
            if (!disabledByFlags[disabledGroup]) {
              flags[disabledGroup] &= ~1u;
            }
          }
        }
      }
    }

    if (inventoryType == INDEX_FEET_TYPE) {
      LPCSTR legLowerTexture = CompUtilGetTextureSectionName(displayInfoRec, TCS_LEGLOWER);
      if (legLowerTexture && *legLowerTexture) {
        flags[3] &= ~1u;
        currentGeosets[3] = inventoryTypeGeosets[3][INDEX_LEGS_TYPE];
        disabledByFlags[3] &= ~(1u << INDEX_FEET_TYPE);
      }
    }

    if (component) {
      if (currentGeosets[7] <= 1) {
        TexComponentRemoveHold(component, INDEX_FEET_TYPE, TCS_LEGLOWER);
      }
      if (oldGeosets[2] > 1 && currentGeosets[2] == 1 && !(flags[2] & 1) && currentGeosets[0] <= 1) {
        TexComponentRemoveHold(component, INDEX_HAND_TYPE, TCS_LOWERARM);
      }
      ShowInventoryTypeTextureHolds(component, inventoryType, 0);
    }
  }

  int  highestPriority[9];
  UINT currentGeosets[9];
  UINT geosetCurrentlyUsedBy[9];
  UINT disabledByFlags[9];
  UINT flags[9];
  UINT inventoryTypeGeosets[9][INDEX_NUMSLOTS];
};

CharGeosetInfo::CharGeosetInfo() {
  Clear();
}

CharGeosetInfo::CharGeosetInfo(const CharGeosetInfo &rhs) {
  UINT group;
  for (group = 0; group < 9; ++group) {
    highestPriority[group] = rhs.highestPriority[group];
    currentGeosets[group] = rhs.currentGeosets[group];
    geosetCurrentlyUsedBy[group] = rhs.geosetCurrentlyUsedBy[group];
    disabledByFlags[group] = rhs.disabledByFlags[group];
    flags[group] = 0;

    UINT inventoryType;
    for (inventoryType = 0; inventoryType < INDEX_NUMSLOTS; ++inventoryType) {
      inventoryTypeGeosets[group][inventoryType] = rhs.inventoryTypeGeosets[group][inventoryType];
    }
  }
}

int CharGeosetInfo::ShowingSameGeosetsAs(const CharGeosetInfo &rhs) {
  UINT group;
  for (group = 0; group < 9; ++group) {
    if ((flags[group] & 1) != (rhs.flags[group] & 1) || ((flags[group] ^ rhs.flags[group]) & 2) != 0 ||
        (!(flags[group] & 1) && currentGeosets[group] != rhs.currentGeosets[group]))
    {
      return 0;
    }
  }
  return 1;
}

void CharGeosetInfo::Clear() {
  UINT group;
  for (group = 0; group < 9; ++group) {
    highestPriority[group] = -1;
    currentGeosets[group] = 1;
    geosetCurrentlyUsedBy[group] = 0;
    disabledByFlags[group] = 0;
    flags[group] = 0;
    memset(inventoryTypeGeosets[group], 0, sizeof(inventoryTypeGeosets[group]));
  }
}

class CCharGeoset : public CHandleObject {
 public:
  enum {
    CHANGED = 1,
    HASSCALP = 2
  };

  CCharGeoset() : m_charModel(0), m_paperDollModel(0), m_flags(0) {
    memset(m_currentGeosets, 0, sizeof(m_currentGeosets));
  }

  virtual ~CCharGeoset() {
    if (m_charModel) {
      HandleClose(m_charModel);
    }
    if (m_paperDollModel) {
      HandleClose(m_paperDollModel);
    }
  }

  void ClearGeosets();

  void ShowGeosetSection(CHARACTER_GEOSET_SECTIONS section, UINT geosetNumber, int hideRemainder) {
    ASSERT(section < NUM_CHARGEOSETS);
    ASSERT(m_charModel);
    ASSERT(geosetNumber < 100);

    ShowGeosetSection(m_charModel, section, geosetNumber, hideRemainder);
    if (m_paperDollModel) {
      ShowGeosetSection(m_paperDollModel, section, geosetNumber, hideRemainder);
    }
    if (!geosetNumber) {
      geosetNumber = !s_defaultGeosets[section];
    }

    if (m_currentGeosets[section] != geosetNumber) {
      m_currentGeosets[section] = geosetNumber;
      m_flags |= CHANGED;
    }
  }

  void ShowGeosetSection(HMODEL model, CHARACTER_GEOSET_SECTIONS section, UINT geosetNumber, int hideRemainder) const {
    ASSERT(section < NUM_CHARGEOSETS);
    ASSERT(model);
    ASSERT(geosetNumber < 100);

    if (hideRemainder) {
      HideGeosetSection(model, section);
    }
    if (!geosetNumber) {
      geosetNumber = !s_defaultGeosets[section];
    }
    if (m_currentGeosets[section] != geosetNumber && m_currentGeosets[section] && !hideRemainder) {
      ModelHideGeosets(model, 100 * section + m_currentGeosets[section], 1);
    }
  }

  void HideGeosetSection(CHARACTER_GEOSET_SECTIONS section);

  void HideGeosetSection(HMODEL model, CHARACTER_GEOSET_SECTIONS section) const;

  void CommitGeosets(HMODEL model);

  void Commit();

  void CommitWorkingGeosetInfo() {
    if (!m_geosetInfo.ShowingSameGeosetsAs(m_workingGeosetInfo)) {
      UINT group;
      for (group = 0; group < 9; ++group) {
        UINT workingFlags = m_workingGeosetInfo.flags[group];
        if (!(workingFlags & 1) || !(m_geosetInfo.flags[group] & 1)) {
          if (!(workingFlags & 2) || !(m_geosetInfo.flags[group] & 2)) {
            if (workingFlags & 1) {
              HideGeosetSection(s_clothingGeosetRanges[group]);
              m_currentGeosets[s_clothingGeosetRanges[group]] = 0;
            } else {
              ShowGeosetSection(s_clothingGeosetRanges[group], workingFlags & 2 ? 1 : m_workingGeosetInfo.currentGeosets[group], 0);
            }
          }
        }
      }
    }
    m_geosetInfo = m_workingGeosetInfo;
  }

  void AddItemGeoset(const ItemDisplayInfoRec *displayInfoRec, UINT itemInventoryType, HTEXCOMPONENT component, UINT playerRace, int doNotCommit);

  void RemoveItemGeoset(const ItemDisplayInfoRec *displayInfoRec, UINT itemInventoryType, HTEXCOMPONENT component);

  void EnableHairGeosets(UINT race, UINT sex, UINT hairStyleID);

  HMODEL         m_charModel;
  HMODEL         m_paperDollModel;
  CharGeosetInfo m_geosetInfo;
  CharGeosetInfo m_workingGeosetInfo;
  int            m_flags;
  UINT           m_currentGeosets[NUM_CHARGEOSETS];
};

void CCharGeoset::ClearGeosets() {
  m_geosetInfo.Clear();
  m_workingGeosetInfo.Clear();
  UINT group;
  for (group = 0; group < 9; ++group) {
    ShowGeosetSection(s_clothingGeosetRanges[group], 0, 0);
  }
}

void CCharGeoset::HideGeosetSection(CHARACTER_GEOSET_SECTIONS section) {
  ASSERT(section < NUM_CHARGEOSETS);
  ASSERT(m_charModel);
  m_currentGeosets[section] = 0;
  HideGeosetSection(m_charModel, section);
  if (m_paperDollModel) {
    HideGeosetSection(m_paperDollModel, section);
  }
}

void CCharGeoset::HideGeosetSection(HMODEL model, CHARACTER_GEOSET_SECTIONS section) const {
  ASSERT(section < NUM_CHARGEOSETS);
  ASSERT(model);
  ModelHideGeosetsRange(model, 100 * section + 1, 100 * section + 99, 1);
}

void CCharGeoset::CommitGeosets(HMODEL model) {
  if (!model || !(m_flags & CHANGED)) {
    return;
  }
  UINT section;
  for (section = 0; section < NUM_CHARGEOSETS; ++section) {
    if (m_currentGeosets[section]) {
      ModelHideGeosets(model, 100 * section + m_currentGeosets[section], 0);
    }
  }
  if (m_flags & HASSCALP) {
    ModelHideGeosets(model, 1, 0);
  }
  ModelHideGeosets(model, 0, 0);
  ModelOptimizeVisibleGeosets(model);
}

void CCharGeoset::Commit() {
  CommitGeosets(m_charModel);
  CommitGeosets(m_paperDollModel);
  m_flags &= ~CHANGED;
}

void CCharGeoset::AddItemGeoset(
    const ItemDisplayInfoRec *displayInfoRec,
    UINT                      itemInventoryType,
    HTEXCOMPONENT             component,
    UINT                      playerRace,
    int                       doNotCommit
) {
  m_workingGeosetInfo.UpdateGeosetDisplay(displayInfoRec, itemInventoryType, component, playerRace);
  if (!doNotCommit) {
    CommitWorkingGeosetInfo();
  }
}

void CCharGeoset::RemoveItemGeoset(const ItemDisplayInfoRec *displayInfoRec, UINT itemInventoryType, HTEXCOMPONENT component) {
  m_workingGeosetInfo.RemoveGeosetInfo(displayInfoRec, itemInventoryType, component);
}

void CCharGeoset::EnableHairGeosets(UINT race, UINT sex, UINT hairStyleID) {
  int geoset = CharCustomizationGetHairGeoset(race, sex, hairStyleID);
  ShowGeosetSection(CHARGEOSET_HAIR, abs(geoset), 1);
  if (geoset < 0) {
    m_flags |= HASSCALP;
  } else {
    m_flags &= ~HASSCALP;
  }
}

static void InitializeCameraFileNames();
static void FillInMissingTextureFileNames();
static void ReadTextureFileNames(int numRaces);
static void InitializeTextureFileNames();
static void InitializeHairGeosets();
static void InitializeTextureHoldLayers();
static void InitializeFacialHairVariations();

static void InitializeCameraFileNames() {
  UINT i;

  s_cameraFileNames.SetCount(g_chrRacesDB.GetMaxID() + 1);

  for (i = 0; i < static_cast<UINT>(g_chrRacesDB.GetNumRecords()); ++i) {
    const ChrRacesRec *rec = g_chrRacesDB.GetRecordByIndex(i);
    UINT               sex;

    ASSERT(rec);

    for (sex = 0; sex < UNITSEX_LAST; ++sex) {
      LPCSTR raceString;

      if (sex == UNITSEX_NONE) {
        continue;
      }

      raceString = rec->m_clientFileString;
      ASSERT(raceString && *raceString);

      SStrPrintf(
          s_cameraFileNames[rec->m_ID].fileName[sex], MAX_PATH, "Character\\%s\\%s\\%s%sGlueCamera.mdl", raceString, g_sexString[sex], raceString,
          g_sexString[sex]
      );
    }
  }
}

static void FillInMissingTextureFileNames() {
  int race;

  for (race = s_raceTextureFileNames.Count(); race;) {
    int sex;

    --race;

    for (sex = 0; sex < UNITSEX_LAST; ++sex) {
      CHARACTERSEXVARIATIONS &sexVar = s_raceTextureFileNames[race].sex[sex];
      int                     maxVars[CHARTEXTUREVARIATIONS_NUM];
      int                     maxColor[CHARTEXTUREVARIATIONS_NUM];
      int                     section;

      memset(maxVars, 0, sizeof(maxVars));
      memset(maxColor, 0, sizeof(maxColor));

      for (section = 0; section < CHARTEXTURESECTION_NUM; ++section) {
        int variation = g_charTextureSectionMapping[section];
        int variations = sexVar.NumVariations(section);
        int sectionVariation;

        if (maxVars[variation] <= variations) {
          maxVars[variation] = variations;
        }

        for (sectionVariation = 0; sectionVariation < variations; ++sectionVariation) {
          int colorCount = sexVar.GetNames(section, sectionVariation).GetColorCount();

          if (maxColor[variation] <= colorCount) {
            maxColor[variation] = colorCount;
          }
        }
      }

      for (section = 0; section < CHARTEXTURESECTION_NUM; ++section) {
        int variation = g_charTextureSectionMapping[section];
        int sectionVariation;

        sexVar.GetSectionData(section).SetCount(maxVars[variation]);

        for (sectionVariation = 0; sectionVariation < maxVars[variation]; ++sectionVariation) {
          if (sectionVariation < sexVar.firstNPCVar[section] && sectionVariation > sexVar.lastNPCVar[section]) {
            sexVar.GetNames(section, sectionVariation).SetColorCount(maxColor[variation]);
          }
        }
      }
    }
  }
}

static void ReadTextureFileNames(int numRaces) {
  int i;

  i = g_charTextureVariationsV2DB.GetNumRecords();
  for (; i; --i) {
    const CharTextureVariationsV2Rec *rec = g_charTextureVariationsV2DB.GetRecordByIndex(i - 1);
    CHARACTERSEXVARIATIONS           &sexVar = s_raceTextureFileNames[rec->m_RaceID].sex[rec->m_SexID];
    CHARACTERVARIATIONS              *variation;

    ASSERT(rec->m_RaceID < numRaces);
    ASSERT(rec->m_SexID < UNITSEX_LAST);
    ASSERT(rec->m_SectionID < CHARTEXTURESECTION_NUM);

    if (rec->m_VariationID >= sexVar.GetSectionData(rec->m_SectionID).Count()) {
      sexVar.GetSectionData(rec->m_SectionID).SetCount(rec->m_VariationID + 1);
    }
    variation = &sexVar.GetNames(rec->m_SectionID, rec->m_VariationID);

    if (rec->m_IsNPC) {
      if (sexVar.firstNPCVar[rec->m_SectionID] >= rec->m_VariationID) {
        sexVar.firstNPCVar[rec->m_SectionID] = rec->m_VariationID;
      }

      if (sexVar.lastNPCVar[rec->m_SectionID] <= rec->m_VariationID) {
        sexVar.lastNPCVar[rec->m_SectionID] = rec->m_VariationID;
      }
    } else {
      ASSERT((sexVar.firstNPCVar[rec->m_SectionID] == -1) || (rec->m_VariationID < sexVar.firstNPCVar[rec->m_SectionID]));
    }

    if (rec->m_ColorID >= variation->GetColorCount()) {
      variation->SetColorCount(rec->m_ColorID + 1);
    }

    LPCSTR textureName = rec->m_TextureName;
    if (textureName && *textureName) {
      variation->GetColor(rec->m_ColorID).SetString("", textureName);
    }
  }
}

static void InitializeTextureFileNames() {
  int numRaces = g_chrRacesDB.GetMaxID() + 1;

  s_raceTextureFileNames.SetCount(numRaces);
  ReadTextureFileNames(numRaces);
  FillInMissingTextureFileNames();
}

static void InitializeHairGeosets() {
  int i;

  i = g_charHairGeosetsDB.GetNumRecords();
  for (; i; --i) {
    const CharHairGeosetsRec *rec = g_charHairGeosetsDB.GetRecordByIndex(i - 1);
    int                       geoset;

    ASSERT(rec->m_VariationID >= 0);

    geoset = rec->m_GeosetID;
    if (rec->m_Showscalp) {
      geoset = -rec->m_GeosetID;
    }

    if (static_cast<int>(s_characterVariations[rec->m_RaceID][rec->m_SexID].hairGeosets.Count()) < rec->m_VariationID + 1) {
      s_characterVariations[rec->m_RaceID][rec->m_SexID].hairGeosets.SetCount(rec->m_VariationID + 1);
    }

    s_characterVariations[rec->m_RaceID][rec->m_SexID].hairGeosets[rec->m_VariationID].theInt = geoset;
  }
}

static void InitializeTextureHoldLayers() {
  int i;

  i = g_charVariationsDB.GetNumRecords();
  for (; i; --i) {
    const CharVariationsRec *rec = g_charVariationsDB.GetRecordByIndex(i - 1);
    int                      race;

    ASSERT(rec);
    race = rec->m_RaceID;

    if (rec->m_SexID < UNITSEX_LAST) {
      int variation;

      for (variation = 0; variation < CHARTEXTUREVARIATIONS_NUM; ++variation) {
        s_characterVariations[race][rec->m_SexID].textureHolds[variation] = rec->m_TextureHoldLayer[variation];
      }
    }
  }
}

static void InitializeFacialHairVariations() {
  int               records = g_characterFacialHairStylesDB.GetNumRecords();
  int               MAX_PLAYER_RACE_ID = g_chrRacesDB.GetMaxID();
  TSStackArray<int> maxVariationID(_alloca(MAX_PLAYER_RACE_ID * 2 * sizeof(int)), MAX_PLAYER_RACE_ID * 2, MAX_PLAYER_RACE_ID * 2);
  int               j;
  int               i;

  j = 0;
  for (; j < MAX_PLAYER_RACE_ID * 2; ++j) {
    maxVariationID[j] = -1;
  }

  for (i = 0; i < records; ++i) {
    const CharacterFacialHairStylesRec *rec = g_characterFacialHairStylesDB.GetRecordByIndex(i);

    if (rec->m_RaceID <= MAX_PLAYER_RACE_ID && static_cast<UINT>(rec->m_SexID) < 2) {
      int index = rec->m_SexID + 2 * rec->m_RaceID - 2;

      if (rec->m_VariationID > maxVariationID[index]) {
        maxVariationID[index] = rec->m_VariationID;
      }
    }
  }

  for (i = 1; i <= MAX_PLAYER_RACE_ID; ++i) {
    int sex;

    for (sex = 0; sex < 2; ++sex) {
      int index = sex + 2 * i - 2;

      s_characterVariations[i][sex].facialVariations.facialGeosets.SetCount(maxVariationID[index] + 1);
    }
  }

  for (i = 0; i < records; ++i) {
    const CharacterFacialHairStylesRec *rec = g_characterFacialHairStylesDB.GetRecordByIndex(i);

    if (rec->m_RaceID <= MAX_PLAYER_RACE_ID && static_cast<UINT>(rec->m_SexID) < 2) {
      TSFixedArray<FACIALGEOSETS> &facialGeosets = s_characterVariations[rec->m_RaceID][rec->m_SexID].facialVariations.facialGeosets;
      FACIALGEOSETS               *facialGeoset;

      ASSERT(rec->m_VariationID < (int)facialGeosets.Count());

      facialGeoset = &facialGeosets[rec->m_VariationID];
      facialGeoset->beardGeoset = rec->m_BeardGeoset + 1;
      facialGeoset->sideBurnGeoset = rec->m_SideburnGeoset + 1;
      facialGeoset->moustacheGeoset = rec->m_MoustacheGeoset + 1;
    }
  }
}

void CharCustomizationInitialize() {
  UINT i;

  s_characterVariations.SetCount(g_chrRacesDB.GetMaxID() + 1);

  i = s_characterVariations.Count();
  for (; i; --i) {
    s_characterVariations[i - 1].SetCount(UNITSEX_LAST);
  }

  InitializeCameraFileNames();
  InitializeTextureFileNames();
  InitializeHairGeosets();
  InitializeFacialHairVariations();
  InitializeTextureHoldLayers();
}

void CharCustomizationShutdown() {
  s_characterVariations.Clear();
  s_raceTextureFileNames.Clear();
}

void CharCustomizationGetNumSkinTextures(UINT raceID, UINT sexID, int *pcVars, int *npcVars) {
  FATALASSERT(raceID != 0);
  FATALASSERT(raceID <= static_cast<UINT>(g_chrRacesDB.GetMaxID()));
  FATALASSERT(sexID < UNITSEX_LAST);

  if (pcVars) {
    *pcVars = 0;
  }
  if (npcVars) {
    *npcVars = 0;
  }

  CHARACTERSEXVARIATIONS &var = s_raceTextureFileNames[raceID].sex[sexID];
  FATALASSERT(var.NumVariations(CHARTEXTURESECTION_SKIN) > 0);

  if (pcVars) {
    *pcVars = var.GetNames(CHARTEXTURESECTION_SKIN, 0).GetColorCount();
  }
  if (var.NumVariations(CHARTEXTURESECTION_SKIN) == 2 && npcVars) {
    *npcVars = var.GetNames(CHARTEXTURESECTION_SKIN, 1).GetColorCount();
  }
}

HTEXTURE CharCustomizationLoadSkin(HMODEL characterModel, LPCSTR skinName, UINT raceID, UINT sexID, UINT textureNumber, int isNPC) {
  FATALASSERT(characterModel);
  FATALASSERT(skinName);
  FATALASSERT(raceID != 0);
  FATALASSERT(raceID <= static_cast<UINT>(g_chrRacesDB.GetMaxID()));
  FATALASSERT(sexID < UNITSEX_LAST);

  int numNPCVariations;
  CharCustomizationGetNumSkinTextures(raceID, sexID, &numNPCVariations, 0);
  FATALASSERT(numNPCVariations);

  int color = textureNumber;
  int variation = 0;
  if (isNPC && static_cast<int>(textureNumber) >= numNPCVariations) {
    color -= numNPCVariations;
    variation = 1;
  }

  CStatus                 status;
  CHARACTERSEXVARIATIONS &var = s_raceTextureFileNames[raceID].sex[sexID];
  LPCSTR                  extraSkin = var.GetNames(CHARTEXTURESECTION_SKINEXTRA, variation).GetColor(color).GetString();

  if (extraSkin && *extraSkin) {
    HTEXTURE extraTexture = TextureCreate(
        extraSkin, CGxTexFlags(CWorld::enables & CWorld::Enable_Trilinear ? GxTex_LinearMipLinear : GxTex_LinearMipNearest, 0, 0, 0, 0, 0, 1),
        &status, 0
    );
    if (extraTexture) {
      ModelReplaceTexture(characterModel, 8, extraTexture, 0);
      HandleClose(extraTexture);
    }
  }

  HTEXTURE texture = TextureCreate(
      skinName, CGxTexFlags(CWorld::enables & CWorld::Enable_Trilinear ? GxTex_LinearMipLinear : GxTex_LinearMipNearest, 0, 0, 0, 0, 0, 1), &status, 0
  );
  if (texture) {
    ModelReplaceTexture(characterModel, 1, texture, 0);
  }

  return texture;
}

HTEXTURE CharCustomizationSetSkin(HMODEL characterModel, UINT raceID, UINT sexID, UINT textureNumber, int isNPC) {
  FATALASSERT(characterModel);
  FATALASSERT(raceID != 0);
  FATALASSERT(raceID <= static_cast<UINT>(g_chrRacesDB.GetMaxID()));
  FATALASSERT(sexID < UNITSEX_LAST);

  int numNPCVariations;
  CharCustomizationGetNumSkinTextures(raceID, sexID, &numNPCVariations, 0);
  FATALASSERT(numNPCVariations);

  int color = textureNumber;
  int variation = 0;
  if (isNPC && static_cast<int>(textureNumber) >= numNPCVariations) {
    color -= numNPCVariations;
    variation = 1;
  }

  CHARACTERSEXVARIATIONS &var = s_raceTextureFileNames[raceID].sex[sexID];
  EGxTexFilter            filter = CWorld::enables & CWorld::Enable_Trilinear ? GxTex_LinearMipLinear : GxTex_LinearMipNearest;
  LPCSTR                  extraSkin = var.GetNames(CHARTEXTURESECTION_SKINEXTRA, variation).GetColor(color).GetString();

  if (extraSkin && *extraSkin) {
    CStatus  status;
    HTEXTURE extraTexture = TextureCreate(extraSkin, CGxTexFlags(filter, 0, 0, 0, 0, 0, 1), &status, 0);
    if (extraTexture) {
      ModelReplaceTexture(characterModel, 8, extraTexture, 0);
      HandleClose(extraTexture);
    }
  }

  LPCSTR skin = var.GetNames(CHARTEXTURESECTION_SKIN, variation).GetColor(color).GetString();
  if (!skin || !*skin) {
    return 0;
  }

  HTEXTURE texture = TextureCreate(skin, 256, 256, GxTex_Argb1555, CGxTexFlags(filter, 0, 0, 0, 0, 0, 1));
  if (texture) {
    ModelReplaceTexture(characterModel, 1, texture, 0);
  }

  return texture;
}

int CharCustomizationGetNakedSectionName(
    UINT  raceID,
    UINT  sexID,
    UINT  skinID,
    UINT  underwearSection,
    char *outBuffer,
    UINT  outBufferSize,
    int   isNPC
) {
  FATALASSERT(raceID != 0);
  FATALASSERT(raceID <= static_cast<UINT>(g_chrRacesDB.GetMaxID()));
  FATALASSERT(sexID < UNITSEX_LAST);
  FATALASSERT(underwearSection < NUM_UNDERWEARHIDESECTIONS);
  FATALASSERT(outBuffer);
  FATALASSERT(outBufferSize);

  int PCSkinVariations;
  if (isNPC) {
    CharCustomizationGetNumSkinTextures(raceID, sexID, 0, &PCSkinVariations);
    if (!PCSkinVariations) {
      CharCustomizationGetNumSkinTextures(raceID, sexID, &PCSkinVariations, 0);
      isNPC = 0;
    }
  } else {
    CharCustomizationGetNumSkinTextures(raceID, sexID, &PCSkinVariations, 0);
  }
  if (!PCSkinVariations) {
    return 0;
  }

  int                  color = skinID % PCSkinVariations;
  CHARTEXTURESECTIONID section = underwearSection ? CHARTEXTURESECTION_NAKEDSKINPELVIS : CHARTEXTURESECTION_NAKEDSKINTORSO;
  LPCSTR               name = s_raceTextureFileNames[raceID].sex[sexID].GetNames(section, isNPC).GetColor(color).GetString();
  if (!name) {
    return 0;
  }

  SStrPrintf(outBuffer, outBufferSize, "%s", name);
  return *name != 0;
}

void CharCustomizationNumFaces(UINT raceID, UINT sexID, int *pcVars, int *npcVars) {
  FATALASSERT(raceID != 0);
  FATALASSERT(raceID <= static_cast<UINT>(g_chrRacesDB.GetMaxID()));
  FATALASSERT(sexID < UNITSEX_LAST);

  CHARACTERSEXVARIATIONS &var = s_raceTextureFileNames[raceID].sex[sexID];
  FATALASSERT(var.NumVariations(CHARTEXTURESECTION_FACEUPPER) == var.NumVariations(CHARTEXTURESECTION_FACELOWER));
  var.GetNumVariations(CHARTEXTURESECTION_FACEUPPER, pcVars, npcVars);
}

void CharCustomizationSetFaceTexture(
    HMODEL        characterModel,
    HTEXCOMPONENT texComponent,
    UINT          raceID,
    UINT          sexID,
    UINT          varID,
    UINT          colorID,
    int           isNPC
) {
  FATALASSERT(characterModel);
  FATALASSERT(texComponent);
  FATALASSERT(raceID != 0);
  FATALASSERT(raceID <= static_cast<UINT>(g_chrRacesDB.GetMaxID()));
  FATALASSERT(sexID < UNITSEX_LAST);

  int pcFaceVars;
  int npcFaceVars;
  int NPCSkinVariations;
  CharCustomizationGetNumSkinTextures(raceID, sexID, &NPCSkinVariations, 0);
  CharCustomizationNumFaces(raceID, sexID, &pcFaceVars, &npcFaceVars);

  if (isNPC) {
    if (colorID >= 100) {
      colorID += NPCSkinVariations - 100;
    }
    if (varID >= 100) {
      varID += pcFaceVars - 100;
    }
    pcFaceVars += npcFaceVars;
  }

  if (!NPCSkinVariations || !pcFaceVars) {
    return;
  }

  CHARACTERSEXVARIATIONS &var = s_raceTextureFileNames[raceID].sex[sexID];
  LPCSTR                  upperTexture = var.GetNames(CHARTEXTURESECTION_FACEUPPER, varID).GetColor(colorID).GetString();
  TexComponentChangeCharacterHead(
      texComponent, upperTexture, var.GetNames(CHARTEXTURESECTION_FACELOWER, varID).GetColor(colorID).GetString(), TEXLAYER_SKIN
  );
}

UINT CharCustomizationNumHairColors(UINT raceID, UINT sexID) {
  FATALASSERT(raceID != 0);
  FATALASSERT(raceID <= static_cast<UINT>(g_chrRacesDB.GetMaxID()));
  FATALASSERT(sexID < UNITSEX_LAST);

  CHARACTERSEXVARIATIONS &var = s_raceTextureFileNames[raceID].sex[sexID];
  if (!var.NumVariations(CHARTEXTURESECTION_HAIR)) {
    return 0;
  }
  return var.GetNames(CHARTEXTURESECTION_HAIR, 0).GetColorCount();
}

void CharCustomizationSetHairTexture(HMODEL characterModel, HTEXCOMPONENT texComponent, UINT raceID, UINT sexID, UINT hairID, UINT colorID) {
  FATALASSERT(characterModel);
  FATALASSERT(raceID != 0);
  FATALASSERT(raceID <= static_cast<UINT>(g_chrRacesDB.GetMaxID()));
  FATALASSERT(sexID < UNITSEX_LAST);

  CHARACTERSEXVARIATIONS &var = s_raceTextureFileNames[raceID].sex[sexID];
  if (!var.NumVariations(CHARTEXTURESECTION_HAIR)) {
    return;
  }

  LPCSTR hairTexture = var.GetNames(CHARTEXTURESECTION_HAIR, hairID).GetColor(colorID).GetString();
  LPCSTR lowerTexture = var.GetNames(CHARTEXTURESECTION_SCALPLOWERHAIR, hairID).GetColor(colorID).GetString();
  LPCSTR upperTexture = var.GetNames(CHARTEXTURESECTION_SCALPUPPERHAIR, hairID).GetColor(colorID).GetString();

  if (texComponent) {
    TexComponentChangeCharacterHead(texComponent, upperTexture, lowerTexture, TEXLAYER_ARMOR);
  }

  if (hairTexture && *hairTexture) {
    CStatus  status;
    HTEXTURE texture = TextureCreate(hairTexture, CGxTexFlags(GxTex_LinearMipLinear, 0, 0, 0, 0, 0, 1), &status, 0);
    if (texture) {
      ModelReplaceTexture(characterModel, 6, texture, 0);
      HandleClose(texture);
    }
  }
}

UINT CharCustomizationNumHairStyles(UINT raceID, UINT sexID) {
  FATALASSERT(raceID <= static_cast<UINT>(g_chrRacesDB.GetMaxID()));
  FATALASSERT(sexID < s_characterVariations[raceID].Count());
  return s_characterVariations[raceID][sexID].hairGeosets.Count();
}

UINT CharCustomizationGetHairGeoset(UINT race, UINT sex, UINT hair) {
  FATALASSERT(race != 0);
  FATALASSERT(race <= static_cast<UINT>(g_chrRacesDB.GetMaxID()));
  FATALASSERT(sex < s_characterVariations[race].Count());

  TSGrowableArray<INTDATA> &hairGeosets = s_characterVariations[race][sex].hairGeosets;
  if (!hairGeosets.Count()) {
    return 0;
  }
  return abs(hairGeosets[hair % hairGeosets.Count()].theInt);
}

UINT CharCustomizationNumBeardStyles(UINT raceID, UINT sexID) {
  FATALASSERT(raceID != 0);
  FATALASSERT(raceID <= static_cast<UINT>(g_chrRacesDB.GetMaxID()));
  FATALASSERT(sexID < UNITSEX_LAST);
  return s_characterVariations[raceID][sexID].facialVariations.facialGeosets.Count();
}

int CharCustomizationGetBeardStyle(UINT raceID, UINT sexID, UINT facialHairID, BEARDSTYLEDATA *facialHairStyleData) {
  FATALASSERT(raceID != 0);
  FATALASSERT(raceID <= static_cast<UINT>(g_chrRacesDB.GetMaxID()));
  FATALASSERT(sexID < UNITSEX_LAST);
  FATALASSERT(facialHairStyleData);

  TSFixedArray<FACIALGEOSETS> &facialGeosets = s_characterVariations[raceID][sexID].facialVariations.facialGeosets;
  if (!facialGeosets.Count()) {
    return 0;
  }

  const FACIALGEOSETS &facial = facialGeosets[facialHairID % facialGeosets.Count()];
  facialHairStyleData->beardGeoset = facial.beardGeoset;
  facialHairStyleData->sideBurnGeoset = facial.sideBurnGeoset;
  facialHairStyleData->moustacheGeoset = facial.moustacheGeoset;
  return 1;
}

void CharCustomizationSetFacialTexture(HMODEL characterModel, HTEXCOMPONENT texComponent, UINT raceID, UINT sexID, UINT facialID, UINT colorID) {
  FATALASSERT(characterModel);
  FATALASSERT(texComponent);
  FATALASSERT(raceID != 0);
  FATALASSERT(raceID <= static_cast<UINT>(g_chrRacesDB.GetMaxID()));
  FATALASSERT(sexID < UNITSEX_LAST);

  CHARACTERSEXVARIATIONS &sexVar = s_raceTextureFileNames[raceID].sex[sexID];
  if (!sexVar.NumVariations(CHARTEXTURESECTION_FACIALUPPERHAIR)) {
    return;
  }

  LPCSTR lowerTexture = sexVar.GetNames(CHARTEXTURESECTION_FACIALLOWERHAIR, facialID).GetColor(colorID).GetString();
  LPCSTR upperTexture = sexVar.GetNames(CHARTEXTURESECTION_FACIALUPPERHAIR, facialID).GetColor(colorID).GetString();
  TexComponentChangeCharacterHead(texComponent, upperTexture, lowerTexture, TEXLAYER_CLOTH);
}

HCHARGEOSET CharCustomizationCreateGeosetHandle(HMODEL characterModel) {
  if (!characterModel) {
    return 0;
  }

  CCharGeoset *newObject = new (SMemAlloc(sizeof(CCharGeoset), "HCHARGEOSET", SERR_LINECODE_OBJECT, 0)) CCharGeoset;
  FATALASSERT(newObject);
  newObject->m_charModel = static_cast<HMODEL>(HandleDuplicate(characterModel));
  return reinterpret_cast<HCHARGEOSET>(HandleCreate(newObject, "HCHARGEOSET"));
}

void CharCustomizationSetPaperDollGeoset(HCHARGEOSET handle, HMODEL paperDollModel) {
  CCharGeoset *geoset = reinterpret_cast<CCharGeoset *>(handle);
  if (geoset) {
    if (geoset->m_paperDollModel) {
      HandleClose(geoset->m_paperDollModel);
    }
    geoset->m_paperDollModel = static_cast<HMODEL>(HandleDuplicate(paperDollModel));
  }
}

void CharCustomizationInitBaseCharacter(HCHARGEOSET geosetHandle, UINT beardGeoset, UINT sideBurnGeoset, UINT moustacheGeoset, UINT earGeoset) {
  CCharGeoset *geoset = reinterpret_cast<CCharGeoset *>(geosetHandle);
  if (!geoset) {
    return;
  }

  geoset->ShowGeosetSection(CHARGEOSET_BEARD, beardGeoset, 1);
  geoset->ShowGeosetSection(CHARGEOSET_SIDEBURN, sideBurnGeoset, 1);
  geoset->ShowGeosetSection(CHARGEOSET_MOUSTACHE, moustacheGeoset, 1);
  geoset->ShowGeosetSection(CHARGEOSET_EAR, earGeoset, 1);
  geoset->ShowGeosetSection(CHARGEOSET_GLOVE, s_baseGeosets[CHARGEOSET_GLOVE], 1);
  geoset->ShowGeosetSection(CHARGEOSET_BOOT, s_baseGeosets[CHARGEOSET_BOOT], 1);
  geoset->ShowGeosetSection(CHARGEOSET_SLEEVES, s_baseGeosets[CHARGEOSET_SLEEVES], 1);
  geoset->ShowGeosetSection(CHARGEOSET_PANTS, s_baseGeosets[CHARGEOSET_PANTS], 1);
  geoset->ShowGeosetSection(CHARGEOSET_DOUBLET, s_baseGeosets[CHARGEOSET_DOUBLET], 1);
  geoset->ShowGeosetSection(CHARGEOSET_PANTDOUBLET, s_baseGeosets[CHARGEOSET_PANTDOUBLET], 1);
  geoset->ShowGeosetSection(CHARGEOSET_TABARD, s_baseGeosets[CHARGEOSET_TABARD], 1);
  geoset->ShowGeosetSection(CHARGEOSET_ROBE, s_baseGeosets[CHARGEOSET_ROBE], 1);
  geoset->ShowGeosetSection(CHARGEOSET_LOINCLOTH, s_baseGeosets[CHARGEOSET_LOINCLOTH], 1);
  geoset->ClearGeosets();
}

void CharCustomizationResetHairGeoset(HCHARGEOSET geosetHandle, UINT race, UINT sex, UINT hairStyleID) {
  CCharGeoset *geoset = reinterpret_cast<CCharGeoset *>(geosetHandle);
  if (geoset) {
    geoset->EnableHairGeosets(race, sex, hairStyleID);
  }
}

void CharCustomizationCommitItemGeosets(HCHARGEOSET geosetHandle, int doNotCommitGeosets) {
  CCharGeoset *geoset = reinterpret_cast<CCharGeoset *>(geosetHandle);
  if (geoset) {
    geoset->CommitWorkingGeosetInfo();
    if (!doNotCommitGeosets) {
      geoset->Commit();
    }
  }
}

void CharCustomizationCommitItemGeosets(HCHARGEOSET geosetHandle, int doNotCommitGeosets, HMODEL paperDollModel) {
  CCharGeoset *geoset = reinterpret_cast<CCharGeoset *>(geosetHandle);
  if (geoset) {
    CharCustomizationCommitItemGeosets(geosetHandle, doNotCommitGeosets);
    if (paperDollModel) {
      geoset->CommitGeosets(paperDollModel);
    }
  }
}

void CharCustomizationClearItemGeosets(HCHARGEOSET geosetHandle) {
  CCharGeoset *geoset = reinterpret_cast<CCharGeoset *>(geosetHandle);
  if (geoset) {
    geoset->ClearGeosets();
  }
}

void CharCustomizationAddItemGeosets(
    HCHARGEOSET               geosetHandle,
    const ItemDisplayInfoRec *displayInfoRec,
    UINT                      itemInventoryType,
    HTEXCOMPONENT             component,
    UINT                      raceID,
    int                       doNotCommit
) {
  CCharGeoset *geoset = reinterpret_cast<CCharGeoset *>(geosetHandle);
  if (geoset) {
    FATALASSERT(displayInfoRec);
    FATALASSERT(itemInventoryType != INDEX_NON_EQUIP_TYPE);
    FATALASSERT(raceID != 0);
    FATALASSERT(raceID <= static_cast<UINT>(g_chrRacesDB.GetMaxID()));
    if ((1 << itemInventoryType) & 0x1805B0) {
      geoset->AddItemGeoset(displayInfoRec, itemInventoryType, component, raceID, doNotCommit);
    }
  }
}

void CharCustomizationRemoveItemGeosets(
    HCHARGEOSET               geosetHandle,
    const ItemDisplayInfoRec *displayInfoRec,
    UINT                      itemInventoryType,
    HTEXCOMPONENT             component
) {
  CCharGeoset *geoset = reinterpret_cast<CCharGeoset *>(geosetHandle);
  if (geoset) {
    FATALASSERT(displayInfoRec);
    FATALASSERT(itemInventoryType != INDEX_NON_EQUIP_TYPE);
    FATALASSERT(component);
    if ((1 << itemInventoryType) & 0x1805B0) {
      geoset->RemoveItemGeoset(displayInfoRec, itemInventoryType, component);
    }
  }
}

void CharCustomizationCommitGeosets(HCHARGEOSET handle) {
  CCharGeoset *geoset = reinterpret_cast<CCharGeoset *>(handle);
  if (geoset) {
    geoset->Commit();
  }
}

void CharCustomizationShowGeoset(HCHARGEOSET handle, CHARACTER_GEOSET_SECTIONS section, UINT geosetNumber) {
  CCharGeoset *geoset = reinterpret_cast<CCharGeoset *>(handle);
  if (geoset) {
    geoset->ShowGeosetSection(section, geosetNumber, 1);
  }
}

void CharCustomizationHideGeosetSection(HCHARGEOSET handle, CHARACTER_GEOSET_SECTIONS section) {
  CCharGeoset *geoset = reinterpret_cast<CCharGeoset *>(handle);
  if (geoset) {
    geoset->HideGeosetSection(section);
  }
}

void CharCustomizationGetTextureLayerHolds(UINT raceID, UINT sexID, UINT *textureLayerHolds, UINT numTextureLayerHolds) {
  FATALASSERT(raceID != 0);
  FATALASSERT(raceID <= static_cast<UINT>(g_chrRacesDB.GetMaxID()));
  FATALASSERT(sexID < UNITSEX_LAST);
  FATALASSERT(textureLayerHolds);
  FATALASSERT(numTextureLayerHolds == NUM_TEXLAYERS);

  UINT textureLayer;
  for (textureLayer = 0; textureLayer < numTextureLayerHolds; ++textureLayer) {
    textureLayerHolds[textureLayer] = s_characterVariations[raceID][sexID].textureHolds[textureLayer];
  }
}

void CHARACTERSEXVARIATIONS::GetNumVariations(CHARTEXTURESECTIONID section, int *pcVars, int *npcVars) {
  if (pcVars) {
    *pcVars = firstNPCVar[section] == INT_MAX ? names[section].Count() : firstNPCVar[section];
  }

  if (npcVars) {
    *npcVars = firstNPCVar[section] == INT_MAX ? 0 : lastNPCVar[section] - firstNPCVar[section] + 1;
  }
}
