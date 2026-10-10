#pragma once

enum TEXCOMPONENT_SECTIONS {
  TCS_UPPERARM = 0,
  TCS_LOWERARM = 1,
  TCS_HAND = 2,
  TCS_UPPERHEAD = 3,
  TCS_LOWERHEAD = 4,
  TCS_UPPERTORSO = 5,
  TCS_LOWERTORSO = 6,
  TCS_LEGUPPER = 7,
  TCS_LEGLOWER = 8,
  TCS_FEET = 9,
  NUM_TEXCOMPONENT_SECTIONS = 10,
  TCS_INVALIDSECTION = 11
};

enum TEXCOMPONENT_LAYERS {
  TEXLAYER_SKIN = 0,
  TEXLAYER_CLOTH = 1,
  TEXLAYER_ARMOR = 2,
  TEXLAYER_OVERLAY = 3,
  NUM_TEXLAYERS = 4,
  TEXLAYER_NONE = -1
};

enum LAYERPRIORITY {
  LAYERPRIORITY_0 = 0,
  LAYERPRIORITY_1 = 1,
  LAYERPRIORITY_2 = 2,
  LAYERPRIORITY_3 = 3,
  NUM_LAYERPRIORITIES = 4
};

class CStatus;
class ItemDisplayInfoRec;

struct CSectionFileNames {
  char path[6][MAX_PATH];
};

struct SUBCOMPONENTDESC {
  char *pathName;
  char *textureName;
  UINT  connectionPointIndex;

  void Cleanup() {
    FREEIFUSED(pathName);
    FREEIFUSED(textureName);
    pathName = 0;
    textureName = 0;
  }

  SUBCOMPONENTDESC() : pathName(0), textureName(0), connectionPointIndex(0) {
  }

  void SetPathName(LPCSTR pathName) {
    if (pathName && *pathName) {
      FREEIFUSED(this->pathName);
      this->pathName = SStrDupA(pathName, __FILE__, __LINE__);
    }
  }

  void SetTextureName(LPCSTR textureName) {
    if (textureName && *textureName) {
      FREEIFUSED(this->textureName);
      this->textureName = SStrDupA(textureName, __FILE__, __LINE__);
    }
  }
  ~SUBCOMPONENTDESC() {
    Cleanup();
  }
};

struct LAYERIDS {
  TEXCOMPONENT_LAYERS layers[NUM_TEXCOMPONENT_SECTIONS];
};

struct SECTIONPRIORITIES {
  LAYERPRIORITY priorities[NUM_TEXCOMPONENT_SECTIONS];
};

struct GEOCOMPONENTINFO {
  LONGLONG allowedSlots;
  int      itemLinks[2];
  int      altItemLinks[2];
};

extern GEOCOMPONENTINFO        g_geometryComponentLookups[INDEX_NUMSLOTS];
extern const LAYERIDS          g_sectionLayers[INDEX_NUMSLOTS];
extern const SECTIONPRIORITIES g_sectionPriorities[INDEX_NUMSLOTS];

BOOL CompUtilGetSectionDimensions(UINT sectionIndex, UINT *width, UINT *height);
BOOL CompUtilGetSectionOffset(UINT sectionIndex, UINT *xCoord, UINT *yCoord);
BOOL CompUtilItemSectionInfo(INVENTORY_TYPES invType, TEXCOMPONENT_SECTIONS section, TEXCOMPONENT_LAYERS *layer, LAYERPRIORITY *priority);
BOOL CompUtilItemSectionInfo(
    const ItemDisplayInfoRec *displayInfoRec,
    UINT                      inventoryType,
    UINT                     *numTextureComponents,
    TEXCOMPONENT_SECTIONS     sectionList[6],
    TEXCOMPONENT_LAYERS       layerList[6],
    LAYERPRIORITY             priorityList[6],
    CSectionFileNames        *fileNameList
);
LPCSTR CompUtilGetTextureSectionName(const ItemDisplayInfoRec *displayInfoRec, UINT textureSection);
UINT   CompUtilGetObjComponents(
    const ItemDisplayInfoRec *displayInfoRec,
    int                       itemInventoryType,
    SUBCOMPONENTDESC         *subComponents,
    UINT                      numSubComponents,
    int                       useAlternate
);
UINT CompUtilGetObjComponentSlotFlags(const ItemDisplayInfoRec *displayInfoRec, int itemInventoryType, int useAlternateSlot);
