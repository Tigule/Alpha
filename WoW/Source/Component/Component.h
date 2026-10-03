#pragma once

#include <Tempest/c2ivector.h>
#include <TextureCacheCore.h>
#include <WowConst.h>

DECLARE_DERIVED_HANDLE(HTEXCOMPONENT, HOBJECT);
DECLARE_DERIVED_HANDLE(HCHARGEOSET, HOBJECT);
struct HMODEL__;
typedef HMODEL__ *HMODEL;

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
    if (pathName) {
      SMemFree(pathName, __FILE__, __LINE__, 0);
    }
    if (textureName) {
      SMemFree(textureName, __FILE__, __LINE__, 0);
    }
    pathName = 0;
    textureName = 0;
  }

  SUBCOMPONENTDESC() : pathName(0), textureName(0), connectionPointIndex(0) {
  }

  void SetPathName(LPCSTR pathName) {
    if (pathName && *pathName) {
      if (this->pathName) {
        SMemFree(this->pathName, __FILE__, __LINE__, 0);
      }
      this->pathName = SStrDupA(pathName, __FILE__, __LINE__);
    }
  }

  void SetTextureName(LPCSTR textureName) {
    if (textureName && *textureName) {
      if (this->textureName) {
        SMemFree(this->textureName, __FILE__, __LINE__, 0);
      }
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
BOOL GetObjComponentInfo(
    int     race,
    int     sex,
    int     displayID,
    int     inventoryType,
    bool    isPlayer,
    bool    useAlternate,
    HMODEL *models,
    int    *attachmentPoints
);
HMODEL ObjComponentBuildAmmoModel(const ItemDisplayInfoRec *displayInfoRec, UINT inventoryType, UINT &seqDuration);
void   ComponentUtilAddItemVisual(HMODEL itemModel, int index, LPCSTR name);
HMODEL ComponentUtilGetChildModel(HMODEL parent, int index);
void   CompDecorateTexName(LPCSTR string, TEXCOMPONENT_SECTIONS section, char *buffer, UINT size, UINT sex, int includeSex);
void   CompDecorateObjName(LPCSTR string, char *buffer, UINT size, UINT race, UINT sex);
void   GetTabardBackgroundFileName(int section, int background, char *buffer, int size);
void   GetTabardEmblemFileName(int section, int emblem, int color, char *buffer, int size);
void   GetTabardBorderFileName(int section, int border, int color, char *buffer, int size);

void ComponentInitialize();
void ComponentShutdown();
bool ComponentApplyTabardTexture(HTEXCOMPONENT component, int eStyle, int eColor, int bStyle, int bColor, int b);
void ComponentRemoveTabardTexture(int sex, HTEXCOMPONENT component, const ItemDisplayInfoRec *displayInfo, int inventoryType);
void ComponentForceTabardDraw(HTEXCOMPONENT component);
void TexComponentCopy(HTEXCOMPONENT d, HTEXCOMPONENT s);
BOOL TexComponentCommitSections(CStatus *status, HTEXCOMPONENT component, BOOL bForce);
int  TexComponentCheckSections(HTEXCOMPONENT component, BOOL bForce);
void TexComponentRemoveSections(HTEXCOMPONENT component, const TEXCOMPONENT_SECTIONS *sectionPointers, const UINT *startLayerList, UINT size);
void TexComponentRemoveAllHolds(HTEXCOMPONENT component);
void TexComponentAddHold(HTEXCOMPONENT component, INVENTORY_TYPES inventory, TEXCOMPONENT_SECTIONS section);
void TexComponentRemoveHold(HTEXCOMPONENT component, INVENTORY_TYPES inventory, TEXCOMPONENT_SECTIONS section);
HTEXCOMPONENT
TexComponentCreate(HTEXTURE texture, UINT race, UINT sex, UINT skinID, BOOL isNPC, int ignoreExistingTexture);
void TexComponentAdd(
    CStatus                  *status,
    int                       playerSex,
    HTEXCOMPONENT             component,
    const ItemDisplayInfoRec *displayInfoRec,
    int                       itemInventoryType,
    int                       checkForExistingTexture
);
void TexComponentChangeCharacterHead(HTEXCOMPONENT component, LPCSTR upperHead, LPCSTR lowerHead, UINT layer);
void HeadGeosetHideCharGeosets(
    HCHARGEOSET               geosetHandle,
    const ItemDisplayInfoRec *displayInfoRec,
    UINT                      raceID,
    const UINT               *preferredGeosets,
    UINT                      numPreferredGeosets
);
void HeadGeosetUnhideCharGeosets(HCHARGEOSET geosetHandle, const UINT *preferredGeosets, UINT numPreferredGeosets);
typedef void (*OBJCALLBACK)(LPVOID param, UINT inventorySlot, HMODEL model, UINT unk, int loaded);
typedef HMODEL (*OBJREMOVECALLBACK)(LPVOID param, UINT inventorySlot, UINT componentLink);
int ObjComponentAdd(
    int                       unitSex,
    int                       unitRace,
    int                       unitPlayer,
    HMODEL                    model,
    const ItemDisplayInfoRec *displayInfoRec,
    int                       itemInventoryType,
    int                       useAlternateSlot,
    HMODEL                    existingModel,
    OBJCALLBACK               callback,
    LPVOID                    param,
    UINT                      inventorySlot
);
HMODEL ObjComponentCreate(UINT itemClass, UINT itemInventoryType, const ItemDisplayInfoRec *displayInfoRec);
void   ObjComponentRemove(HMODEL charModel, UINT inventoryType);
HMODEL ObjComponentRemove(
    HMODEL            charModel,
    UINT              unitRace,
    UINT              unitSex,
    UINT              slot,
    int               returnModelIfOnlyOneSubcomponent,
    OBJREMOVECALLBACK callback,
    LPVOID            callbackParam
);
void TexComponentRemove(HTEXCOMPONENT component, const ItemDisplayInfoRec *displayInfoRec, int itemInventoryType);
