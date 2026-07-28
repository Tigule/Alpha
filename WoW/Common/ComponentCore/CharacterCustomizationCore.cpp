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
static const unsigned int                       NUM_UNDERWEARHIDESECTIONS = 2;
static const unsigned int                       s_defaultGeosets[NUM_CHARGEOSETS] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0};
unsigned int                                    g_defaultGeosetIDOffsets[NUM_CHARGEOSETS] = {1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0};
static const unsigned int                       s_baseGeosets[NUM_CHARGEOSETS] = {1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0};
static CHARACTER_GEOSET_SECTIONS s_clothingGeosetRanges[9] = {CHARGEOSET_GLOVE, CHARGEOSET_BOOT, CHARGEOSET_SLEEVES, CHARGEOSET_PANTS,     CHARGEOSET_DOUBLET,
                                                              CHARGEOSET_PANTDOUBLET, CHARGEOSET_TABARD,  CHARGEOSET_ROBE,   CHARGEOSET_LOINCLOTH};

static const unsigned int s_itemGeosetPriorities[INDEX_NUMSLOTS][9] = {
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

static const unsigned int s_inventoryAndGeosetDisables[INDEX_NUMSLOTS][9] = {
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

static unsigned int s_geosetGroupDisables[9] = {0, 0, 0, 0, 0, 0, 0, 2, 0};

struct HOLDINFO {
  CHARACTER_ITEM_GEOSETS geosetGroup;
  TEXCOMPONENT_SECTIONS holdSection;
};

struct INVHOLDINFO {
  INVHOLDINFO(
      CHARACTER_ITEM_GEOSETS geoset0,
      TEXCOMPONENT_SECTIONS section0,
      CHARACTER_ITEM_GEOSETS geoset1,
      TEXCOMPONENT_SECTIONS section1
  ) {
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

  void ShowInventoryTypeTextureHolds(HTEXCOMPONENT component, unsigned int inventoryType, int adding) {
    FATALASSERT(component);
    FATALASSERT(inventoryType != INDEX_NON_EQUIP_TYPE);
    unsigned int i;
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

  void
  UpdateGeosetDisplay(const ItemDisplayInfoRec *displayInfoRec, unsigned int itemInventoryType, HTEXCOMPONENT component, unsigned int playerRace) {
    if (!displayInfoRec || !playerRace || playerRace > static_cast<unsigned int>(g_chrRacesDB.GetMaxID())) {
      return;
    }

    const ChrRacesRec *raceRec = g_chrRacesDB.GetRecord(playerRace);
    if (!raceRec || ((raceRec->m_flags & 2) && itemInventoryType == INDEX_FEET_TYPE)) {
      return;
    }

    unsigned int i;
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

    const char *legLowerTexture = CompUtilGetTextureSectionName(displayInfoRec, TCS_LEGLOWER);
    if (itemInventoryType == INDEX_FEET_TYPE) {
      if (legLowerTexture && *legLowerTexture) {
        flags[3] |= 1;
        disabledByFlags[3] |= 1u << INDEX_FEET_TYPE;
      }
    } else if (itemInventoryType == INDEX_LEGS_TYPE && !flags[3] && legLowerTexture && *legLowerTexture) {
      flags[3] |= 1;
      disabledByFlags[3] |= 1u << INDEX_LEGS_TYPE;
    }

    unsigned int group;
    for (group = 0; group < 9; ++group) {
      if (s_geosetGroupDisables[group]) {
        unsigned int disabledGroup;
        for (disabledGroup = 0; disabledGroup < 9; ++disabledGroup) {
          if (currentGeosets[group] > 1 && (s_geosetGroupDisables[group] & (1u << disabledGroup))) {
            flags[disabledGroup] |= 2;
            disabledByFlags[disabledGroup] |= 1u << geosetCurrentlyUsedBy[group];
          }
        }
      }

      unsigned int disables = s_inventoryAndGeosetDisables[itemInventoryType][group];
      if (group >= 5 && group <= 7) {
        disables |= 1u << INDEX_FEET_TYPE;
      }
      if (currentGeosets[group] > 1 && disables) {
        unsigned int disabledGroup;
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

  void RemoveGeosetInfo(const ItemDisplayInfoRec *displayInfoRec, unsigned int inventoryType, HTEXCOMPONENT component) {
    FATALASSERT(inventoryType != INDEX_NON_EQUIP_TYPE);

    unsigned int oldGeosets[9];
    memcpy(oldGeosets, currentGeosets, sizeof(oldGeosets));

    unsigned int group;
    for (group = 0; group < 9; ++group) {
      inventoryTypeGeosets[group][inventoryType] = 0;
      geosetCurrentlyUsedBy[group] = INDEX_NON_EQUIP_TYPE;
      highestPriority[group] = -1;

      unsigned int bestInventoryType = INDEX_NON_EQUIP_TYPE;
      unsigned int candidateInventoryType;
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

      unsigned int hideFlags = s_geosetGroupDisables[group];
      if (hideFlags) {
        unsigned int disabledGroup;
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

      unsigned int thisGroupDisablesFlags = s_inventoryAndGeosetDisables[inventoryType][group];
      if (group >= 5 && group <= 7) {
        thisGroupDisablesFlags |= 1u << INDEX_FEET_TYPE;
      }
      if (thisGroupDisablesFlags) {
        unsigned int disabledGroup;
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
      const char *legLowerTexture = CompUtilGetTextureSectionName(displayInfoRec, TCS_LEGLOWER);
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

  int          highestPriority[9];
  unsigned int currentGeosets[9];
  unsigned int geosetCurrentlyUsedBy[9];
  unsigned int disabledByFlags[9];
  unsigned int flags[9];
  unsigned int inventoryTypeGeosets[9][INDEX_NUMSLOTS];
};

CharGeosetInfo::CharGeosetInfo() {
  Clear();
}

CharGeosetInfo::CharGeosetInfo(const CharGeosetInfo &rhs) {
  unsigned int group;
  for (group = 0; group < 9; ++group) {
    highestPriority[group] = rhs.highestPriority[group];
    currentGeosets[group] = rhs.currentGeosets[group];
    geosetCurrentlyUsedBy[group] = rhs.geosetCurrentlyUsedBy[group];
    disabledByFlags[group] = rhs.disabledByFlags[group];
    flags[group] = 0;

    unsigned int inventoryType;
    for (inventoryType = 0; inventoryType < INDEX_NUMSLOTS; ++inventoryType) {
      inventoryTypeGeosets[group][inventoryType] = rhs.inventoryTypeGeosets[group][inventoryType];
    }
  }
}

int CharGeosetInfo::ShowingSameGeosetsAs(const CharGeosetInfo &rhs) {
  unsigned int group;
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
  unsigned int group;
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

  void ShowGeosetSection(CHARACTER_GEOSET_SECTIONS section, unsigned int geosetNumber, int hideRemainder) {
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

  void ShowGeosetSection(
      HMODEL model, CHARACTER_GEOSET_SECTIONS section, unsigned int geosetNumber, int hideRemainder
  ) const {
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
      unsigned int group;
      for (group = 0; group < 9; ++group) {
        unsigned int workingFlags = m_workingGeosetInfo.flags[group];
        if (!(workingFlags & 1) || !(m_geosetInfo.flags[group] & 1)) {
          if (!(workingFlags & 2) || !(m_geosetInfo.flags[group] & 2)) {
            unsigned int geoset = m_workingGeosetInfo.currentGeosets[group];
            if (workingFlags & 1) {
              HideGeosetSection(s_clothingGeosetRanges[group]);
              m_currentGeosets[s_clothingGeosetRanges[group]] = 0;
            } else {
              if (workingFlags & 2) {
                geoset = 1;
              }
              ShowGeosetSection(s_clothingGeosetRanges[group], geoset, 0);
            }
          }
        }
      }
    }
    m_geosetInfo = m_workingGeosetInfo;
  }

  void AddItemGeoset(
      const ItemDisplayInfoRec *displayInfoRec,
      unsigned int              itemInventoryType,
      HTEXCOMPONENT             component,
      unsigned int              playerRace,
      int                       doNotCommit
  );

  void RemoveItemGeoset(const ItemDisplayInfoRec *displayInfoRec, unsigned int itemInventoryType, HTEXCOMPONENT component);

  void EnableHairGeosets(unsigned int race, unsigned int sex, unsigned int hairStyleID);

  HMODEL         m_charModel;
  HMODEL         m_paperDollModel;
  CharGeosetInfo m_geosetInfo;
  CharGeosetInfo m_workingGeosetInfo;
  int            m_flags;
  unsigned int   m_currentGeosets[NUM_CHARGEOSETS];
};

void CCharGeoset::ClearGeosets() {
  m_geosetInfo.Clear();
  m_workingGeosetInfo.Clear();
  unsigned int group;
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
  unsigned int section;
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
    unsigned int              itemInventoryType,
    HTEXCOMPONENT             component,
    unsigned int              playerRace,
    int                       doNotCommit
) {
  m_workingGeosetInfo.UpdateGeosetDisplay(displayInfoRec, itemInventoryType, component, playerRace);
  if (!doNotCommit) {
    CommitWorkingGeosetInfo();
  }
}

void CCharGeoset::RemoveItemGeoset(
    const ItemDisplayInfoRec *displayInfoRec, unsigned int itemInventoryType, HTEXCOMPONENT component
) {
  m_workingGeosetInfo.RemoveGeosetInfo(displayInfoRec, itemInventoryType, component);
}

void CCharGeoset::EnableHairGeosets(unsigned int race, unsigned int sex, unsigned int hairStyleID) {
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
  unsigned int i;

  s_cameraFileNames.SetCount(g_chrRacesDB.GetMaxID() + 1);

  for (i = 0; i < static_cast<unsigned int>(g_chrRacesDB.GetNumRecords()); ++i) {
    const ChrRacesRec *rec = g_chrRacesDB.GetRecordByIndex(i);
    unsigned int       sex;

    ASSERT(rec);

    for (sex = 0; sex < UNITSEX_LAST; ++sex) {
      const char *raceString;

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
        int variations = maxVars[variation];
        int sectionVariation;

        sexVar.GetSectionData(section).SetCount(variations);

        for (sectionVariation = 0; sectionVariation < variations; ++sectionVariation) {
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

  for (i = g_charTextureVariationsV2DB.GetNumRecords(); i; --i) {
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

    if (rec->m_TextureName && *rec->m_TextureName) {
      variation->GetColor(rec->m_ColorID).SetString("", rec->m_TextureName);
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

  for (i = g_charHairGeosetsDB.GetNumRecords(); i; --i) {
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

  for (i = g_charVariationsDB.GetNumRecords(); i; --i) {
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

  for (j = 0; j < MAX_PLAYER_RACE_ID * 2; ++j) {
    maxVariationID[j] = -1;
  }

  for (i = 0; i < records; ++i) {
    const CharacterFacialHairStylesRec *rec = g_characterFacialHairStylesDB.GetRecordByIndex(i);

    if (rec->m_RaceID <= MAX_PLAYER_RACE_ID && static_cast<unsigned int>(rec->m_SexID) < 2) {
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

    if (rec->m_RaceID <= MAX_PLAYER_RACE_ID && static_cast<unsigned int>(rec->m_SexID) < 2) {
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
  unsigned int i;

  s_characterVariations.SetCount(g_chrRacesDB.GetMaxID() + 1);

  for (i = s_characterVariations.Count(); i; --i) {
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

void CharCustomizationGetNumSkinTextures(unsigned int raceID, unsigned int sexID, int *pcVars, int *npcVars) {
  FATALASSERT(raceID != 0);
  FATALASSERT(raceID <= static_cast<unsigned int>(g_chrRacesDB.GetMaxID()));
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

HTEXTURE CharCustomizationLoadSkin(
    HMODEL       characterModel,
    const char  *skinName,
    unsigned int raceID,
    unsigned int sexID,
    unsigned int textureNumber,
    int          isNPC
) {
  FATALASSERT(characterModel);
  FATALASSERT(skinName);
  FATALASSERT(raceID != 0);
  FATALASSERT(raceID <= static_cast<unsigned int>(g_chrRacesDB.GetMaxID()));
  FATALASSERT(sexID < UNITSEX_LAST);

  int numPCVariations;
  int numNPCVariations;
  CharCustomizationGetNumSkinTextures(raceID, sexID, &numPCVariations, &numNPCVariations);
  FATALASSERT(numPCVariations + numNPCVariations);

  int color = textureNumber;
  int variation = 0;
  if (isNPC && static_cast<int>(textureNumber) >= numPCVariations) {
    color -= numPCVariations;
    variation = 1;
  }

  CStatus                 status;
  CHARACTERSEXVARIATIONS &var = s_raceTextureFileNames[raceID].sex[sexID];
  const char             *extraSkin = var.GetNames(CHARTEXTURESECTION_SKINEXTRA, variation).GetColor(color).GetString();

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

HTEXTURE CharCustomizationSetSkin(HMODEL characterModel, unsigned int raceID, unsigned int sexID, unsigned int textureNumber, int isNPC) {
  FATALASSERT(characterModel);
  FATALASSERT(raceID != 0);
  FATALASSERT(raceID <= static_cast<unsigned int>(g_chrRacesDB.GetMaxID()));
  FATALASSERT(sexID < UNITSEX_LAST);

  int numPCVariations;
  int numNPCVariations;
  CharCustomizationGetNumSkinTextures(raceID, sexID, &numPCVariations, &numNPCVariations);
  FATALASSERT(numPCVariations + numNPCVariations);

  int color = textureNumber;
  int variation = 0;
  if (isNPC && static_cast<int>(textureNumber) >= numPCVariations) {
    color -= numPCVariations;
    variation = 1;
  }

  CHARACTERSEXVARIATIONS &var = s_raceTextureFileNames[raceID].sex[sexID];
  EGxTexFilter            filter = CWorld::enables & CWorld::Enable_Trilinear ? GxTex_LinearMipLinear : GxTex_LinearMipNearest;
  const char             *extraSkin = var.GetNames(CHARTEXTURESECTION_SKINEXTRA, variation).GetColor(color).GetString();

  if (extraSkin && *extraSkin) {
    CStatus  status;
    HTEXTURE extraTexture = TextureCreate(extraSkin, CGxTexFlags(filter, 0, 0, 0, 0, 0, 1), &status, 0);
    if (extraTexture) {
      ModelReplaceTexture(characterModel, 8, extraTexture, 0);
      HandleClose(extraTexture);
    }
  }

  const char *skin = var.GetNames(CHARTEXTURESECTION_SKIN, variation).GetColor(color).GetString();
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
    unsigned int raceID,
    unsigned int sexID,
    unsigned int skinID,
    unsigned int underwearSection,
    char        *outBuffer,
    unsigned int outBufferSize,
    int          isNPC
) {
  FATALASSERT(raceID != 0);
  FATALASSERT(raceID <= static_cast<unsigned int>(g_chrRacesDB.GetMaxID()));
  FATALASSERT(sexID < UNITSEX_LAST);
  FATALASSERT(underwearSection < NUM_UNDERWEARHIDESECTIONS);
  FATALASSERT(outBuffer);
  FATALASSERT(outBufferSize);

  int PCSkinVariations;
  int NPCSkinVariations;
  CharCustomizationGetNumSkinTextures(raceID, sexID, &PCSkinVariations, &NPCSkinVariations);
  if (isNPC && NPCSkinVariations) {
    PCSkinVariations = NPCSkinVariations;
    isNPC = 1;
  } else {
    if (!PCSkinVariations) {
      return 0;
    }
    isNPC = 0;
  }

  int                  color = skinID % PCSkinVariations;
  CHARTEXTURESECTIONID section = underwearSection ? CHARTEXTURESECTION_NAKEDSKINPELVIS : CHARTEXTURESECTION_NAKEDSKINTORSO;
  const char          *name = s_raceTextureFileNames[raceID].sex[sexID].GetNames(section, isNPC).GetColor(color).GetString();
  if (!name) {
    return 0;
  }

  SStrPrintf(outBuffer, outBufferSize, "%s", name);
  return *name != 0;
}

void CharCustomizationNumFaces(unsigned int raceID, unsigned int sexID, int *pcVars, int *npcVars) {
  FATALASSERT(raceID != 0);
  FATALASSERT(raceID <= static_cast<unsigned int>(g_chrRacesDB.GetMaxID()));
  FATALASSERT(sexID < UNITSEX_LAST);

  CHARACTERSEXVARIATIONS &var = s_raceTextureFileNames[raceID].sex[sexID];
  FATALASSERT(var.NumVariations(CHARTEXTURESECTION_FACEUPPER) == var.NumVariations(CHARTEXTURESECTION_FACELOWER));
  var.GetNumVariations(CHARTEXTURESECTION_FACEUPPER, pcVars, npcVars);
}

void CharCustomizationSetFaceTexture(
    HMODEL        characterModel,
    HTEXCOMPONENT texComponent,
    unsigned int  raceID,
    unsigned int  sexID,
    unsigned int  varID,
    unsigned int  colorID,
    int           isNPC
) {
  FATALASSERT(characterModel);
  FATALASSERT(texComponent);
  FATALASSERT(raceID != 0);
  FATALASSERT(raceID <= static_cast<unsigned int>(g_chrRacesDB.GetMaxID()));
  FATALASSERT(sexID < UNITSEX_LAST);

  int pcFaceVars;
  int npcFaceVars;
  int PCSkinVariations;
  int NPCSkinVariations;
  CharCustomizationGetNumSkinTextures(raceID, sexID, &PCSkinVariations, &NPCSkinVariations);
  CharCustomizationNumFaces(raceID, sexID, &pcFaceVars, &npcFaceVars);

  if (isNPC) {
    if (colorID >= 100) {
      colorID += PCSkinVariations - 100;
    }
    PCSkinVariations += NPCSkinVariations;
    if (varID >= 100) {
      varID += pcFaceVars - 100;
    }
    pcFaceVars += npcFaceVars;
  }

  if (!PCSkinVariations || !pcFaceVars) {
    return;
  }

  CHARACTERSEXVARIATIONS &var = s_raceTextureFileNames[raceID].sex[sexID];
  const char             *upperTexture = var.GetNames(CHARTEXTURESECTION_FACEUPPER, varID).GetColor(colorID).GetString();
  TexComponentChangeCharacterHead(
      texComponent, upperTexture, var.GetNames(CHARTEXTURESECTION_FACELOWER, varID).GetColor(colorID).GetString(), TEXLAYER_SKIN
  );
}

unsigned int CharCustomizationNumHairColors(unsigned int raceID, unsigned int sexID) {
  FATALASSERT(raceID != 0);
  FATALASSERT(raceID <= static_cast<unsigned int>(g_chrRacesDB.GetMaxID()));
  FATALASSERT(sexID < UNITSEX_LAST);

  CHARACTERSEXVARIATIONS &var = s_raceTextureFileNames[raceID].sex[sexID];
  if (!var.NumVariations(CHARTEXTURESECTION_HAIR)) {
    return 0;
  }
  return var.GetNames(CHARTEXTURESECTION_HAIR, 0).GetColorCount();
}

void CharCustomizationSetHairTexture(
    HMODEL        characterModel,
    HTEXCOMPONENT texComponent,
    unsigned int  raceID,
    unsigned int  sexID,
    unsigned int  hairID,
    unsigned int  colorID
) {
  FATALASSERT(characterModel);
  FATALASSERT(raceID != 0);
  FATALASSERT(raceID <= static_cast<unsigned int>(g_chrRacesDB.GetMaxID()));
  FATALASSERT(sexID < UNITSEX_LAST);

  CHARACTERSEXVARIATIONS &var = s_raceTextureFileNames[raceID].sex[sexID];
  if (!var.NumVariations(CHARTEXTURESECTION_HAIR)) {
    return;
  }

  const char *hairTexture = var.GetNames(CHARTEXTURESECTION_HAIR, hairID).GetColor(colorID).GetString();
  const char *lowerTexture = var.GetNames(CHARTEXTURESECTION_SCALPLOWERHAIR, hairID).GetColor(colorID).GetString();
  const char *upperTexture = var.GetNames(CHARTEXTURESECTION_SCALPUPPERHAIR, hairID).GetColor(colorID).GetString();

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

unsigned int CharCustomizationNumHairStyles(unsigned int raceID, unsigned int sexID) {
  FATALASSERT(raceID <= static_cast<unsigned int>(g_chrRacesDB.GetMaxID()));
  FATALASSERT(sexID < s_characterVariations[raceID].Count());
  return s_characterVariations[raceID][sexID].hairGeosets.Count();
}

unsigned int CharCustomizationGetHairGeoset(unsigned int race, unsigned int sex, unsigned int hair) {
  FATALASSERT(race != 0);
  FATALASSERT(race <= static_cast<unsigned int>(g_chrRacesDB.GetMaxID()));
  FATALASSERT(sex < s_characterVariations[race].Count());

  TSGrowableArray<INTDATA> &hairGeosets = s_characterVariations[race][sex].hairGeosets;
  if (!hairGeosets.Count()) {
    return 0;
  }
  return abs(hairGeosets[hair % hairGeosets.Count()].theInt);
}

unsigned int CharCustomizationNumBeardStyles(unsigned int raceID, unsigned int sexID) {
  FATALASSERT(raceID != 0);
  FATALASSERT(raceID <= static_cast<unsigned int>(g_chrRacesDB.GetMaxID()));
  FATALASSERT(sexID < UNITSEX_LAST);
  return s_characterVariations[raceID][sexID].facialVariations.facialGeosets.Count();
}

int
CharCustomizationGetBeardStyle(unsigned int raceID, unsigned int sexID, unsigned int facialHairID, BEARDSTYLEDATA *facialHairStyleData) {
  FATALASSERT(raceID != 0);
  FATALASSERT(raceID <= static_cast<unsigned int>(g_chrRacesDB.GetMaxID()));
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

void CharCustomizationSetFacialTexture(
    HMODEL        characterModel,
    HTEXCOMPONENT texComponent,
    unsigned int  raceID,
    unsigned int  sexID,
    unsigned int  facialID,
    unsigned int  colorID
) {
  FATALASSERT(characterModel);
  FATALASSERT(texComponent);
  FATALASSERT(raceID != 0);
  FATALASSERT(raceID <= static_cast<unsigned int>(g_chrRacesDB.GetMaxID()));
  FATALASSERT(sexID < UNITSEX_LAST);

  CHARACTERSEXVARIATIONS &sexVar = s_raceTextureFileNames[raceID].sex[sexID];
  if (!sexVar.NumVariations(CHARTEXTURESECTION_FACIALUPPERHAIR)) {
    return;
  }

  const char *lowerTexture = sexVar.GetNames(CHARTEXTURESECTION_FACIALLOWERHAIR, facialID).GetColor(colorID).GetString();
  const char *upperTexture = sexVar.GetNames(CHARTEXTURESECTION_FACIALUPPERHAIR, facialID).GetColor(colorID).GetString();
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

void CharCustomizationInitBaseCharacter(
    HCHARGEOSET  geosetHandle,
    unsigned int beardGeoset,
    unsigned int sideBurnGeoset,
    unsigned int moustacheGeoset,
    unsigned int earGeoset
) {
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

void CharCustomizationResetHairGeoset(HCHARGEOSET geosetHandle, unsigned int race, unsigned int sex, unsigned int hairStyleID) {
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
    unsigned int              itemInventoryType,
    HTEXCOMPONENT             component,
    unsigned int              raceID,
    int                       doNotCommit
) {
  CCharGeoset *geoset = reinterpret_cast<CCharGeoset *>(geosetHandle);
  if (geoset) {
    FATALASSERT(displayInfoRec);
    FATALASSERT(itemInventoryType != INDEX_NON_EQUIP_TYPE);
    FATALASSERT(raceID != 0);
    FATALASSERT(raceID <= static_cast<unsigned int>(g_chrRacesDB.GetMaxID()));
    if ((1 << itemInventoryType) & 0x1805B0) {
      geoset->AddItemGeoset(displayInfoRec, itemInventoryType, component, raceID, doNotCommit);
    }
  }
}

void CharCustomizationRemoveItemGeosets(
    HCHARGEOSET               geosetHandle,
    const ItemDisplayInfoRec *displayInfoRec,
    unsigned int              itemInventoryType,
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

void CharCustomizationShowGeoset(HCHARGEOSET handle, CHARACTER_GEOSET_SECTIONS section, unsigned int geosetNumber) {
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

void
CharCustomizationGetTextureLayerHolds(unsigned int raceID, unsigned int sexID, unsigned int *textureLayerHolds, unsigned int numTextureLayerHolds) {
  FATALASSERT(raceID != 0);
  FATALASSERT(raceID <= static_cast<unsigned int>(g_chrRacesDB.GetMaxID()));
  FATALASSERT(sexID < UNITSEX_LAST);
  FATALASSERT(textureLayerHolds);
  FATALASSERT(numTextureLayerHolds == NUM_TEXLAYERS);

  unsigned int textureLayer;
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
