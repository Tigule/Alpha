#pragma once

#include <Tempest/c2ivector.h>
#include <TextureCacheCore.h>
#include <WowConst.h>
#include <ComponentCore/ComponentUtilCore.h>

DECLARE_DERIVED_HANDLE(HTEXCOMPONENT, HOBJECT);
DECLARE_DERIVED_HANDLE(HCHARGEOSET, HOBJECT);
struct HMODEL__;
typedef HMODEL__ *HMODEL;

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
