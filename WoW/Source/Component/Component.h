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
  SUBCOMPONENTDESC() : pathName(0), textureName(0), connectionPointIndex(0) {
  }
  ~SUBCOMPONENTDESC() {
    Cleanup();
  }

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

  void SetPathName(LPCSTR pathName) {
    if (this->pathName) {
      SMemFree(this->pathName, __FILE__, __LINE__, 0);
    }
    this->pathName = pathName ? SStrDupA(pathName, __FILE__, __LINE__) : 0;
  }

  void SetTextureName(LPCSTR textureName) {
    if (this->textureName) {
      SMemFree(this->textureName, __FILE__, __LINE__, 0);
    }
    this->textureName = textureName ? SStrDupA(textureName, __FILE__, __LINE__) : 0;
  }

  char *pathName;
  char *textureName;
  UINT  connectionPointIndex;
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

class CTexturePiece : public CHandleObject {
 public:
  CTexturePiece() : m_mippedTexture(0), m_holds(0) {
    m_fileName[0] = 0;
  }
  CTexturePiece(const CTexturePiece &source);
  CTexturePiece &operator=(const CTexturePiece &rhs);
  virtual ~CTexturePiece() {
    if (m_mippedTexture) {
      HandleClose(m_mippedTexture);
    }
  }

  BOOL IsOpaque() const {
    return m_textureInfo.opaque || !m_textureInfo.alphaBits;
  }

  BOOL HasImage() const {
    return m_mippedTexture != 0;
  }

  BOOL IsLoaded() const;

  BOOL HasHolds() const {
    return m_holds != 0;
  }

  BOOL HasHold(UINT hold) const;

  void SetHold(UINT hold) {
    m_holds |= 1 << hold;
  }

  void ClearHold(UINT hold) {
    m_holds &= ~(1 << hold);
  }

  BOOL SetTexture(
      TEXCOMPONENT_SECTIONS section,
      TEXCOMPONENT_LAYERS   layer,
      LAYERPRIORITY         priority,
      CStatus              *status,
      int                   checkExistingTexture,
      LPCSTR                fileName,
      UINT                  expectedWidth,
      UINT                  expectedHeight
  );
  void SetTexture(int checkExistingTexture, const CTexturePiece &source);
  void SetTexture(int checkExistingTexture, HTEXTURE texture);
  void AllocBlankTexture(EGxTexFormat format, UINT width, UINT height, int opaque);
  void SetOpaque(int opaque);
  int  UpdateInfo(int force);
  int  Paste(const CTexturePiece &source, int x, int y);
  int  Paste(const CTexturePiece &source, int x, int y, int width, int height);
  void PasteOpaque(const CTexturePiece &source, NTempest::C2iVector dstPos, NTempest::C2iVector srcPos, NTempest::C2iVector size);
  void PasteTransparentOneBit(const CTexturePiece &source, NTempest::C2iVector dstPos, NTempest::C2iVector srcPos, NTempest::C2iVector size);
  void PasteTransparentFull(const CTexturePiece &source, NTempest::C2iVector dstPos, NTempest::C2iVector srcPos, NTempest::C2iVector size);

  static MipBits    *m_destImage;
  static TEXTUREINFO m_destTextureInfo;

 private:
  friend class CTexComponent;

  TEXTUREINFO    m_textureInfo;
  HMIPPEDTEXTURE m_mippedTexture;
  UINT           m_holds;

 protected:
  char m_fileName[MAX_PATH];
};

class CTextureLayer {
 public:
  CTextureLayer &operator=(const CTextureLayer &rhs);
  BOOL           IsOpaque() const;
  void           SetTexture(int priority, int checkExistingTexture, HTEXTURE texture);
  void           SetTexture(int priority, int checkExistingTexture, const CTexturePiece &source);
  void           AllocBlankTexture(
      TEXCOMPONENT_SECTIONS section,
      CStatus              *status,
      TEXCOMPONENT_LAYERS   layer,
      EGxTexFormat          format,
      UINT                  width,
      UINT                  height,
      int                   opaque
  );
  int SetTexture(
      TEXCOMPONENT_SECTIONS section,
      TEXCOMPONENT_LAYERS   layer,
      LAYERPRIORITY         priority,
      CStatus              *status,
      int                   checkExistingTexture,
      LPCSTR                fileName,
      UINT                  expectedWidth,
      UINT                  expectedHeight
  );
  void
  PasteOpaque(const CTexturePiece &source, NTempest::C2iVector dstPos, NTempest::C2iVector srcPos, UINT width, UINT height, LAYERPRIORITY priority);
  void SetHold(int priority, UINT hold);
  void ClearHold(int priority, UINT hold);
  BOOL HasHold(int priority, UINT hold) const;
  BOOL HasHolds(int priority) const;
  BOOL HasImage(int priority) const;

  CTexturePiece m_priorities[4];
};

class CSection {
 public:
  CSection &operator=(const CSection &rhs);
  void      SetHold(int layer, int priority, UINT hold);
  void      ClearHold(int layer, int priority, UINT hold);
  BOOL      HasHold(int layer, int priority, UINT hold) const;
  BOOL      HasHolds(int layer, int priority) const;
  BOOL      HasImage(int layer, int priority) const;
  BOOL      IsLayerOpaque(UINT layer);
  void      SetTexture(int layer, int priority, int checkExistingTexture, HTEXTURE texture);
  void      SetTexture(int layer, int priority, int checkExistingTexture, const CTexturePiece &texture);
  int       SetTexture(
      CStatus              *status,
      TEXCOMPONENT_SECTIONS section,
      TEXCOMPONENT_LAYERS   layer,
      LAYERPRIORITY         priority,
      int                   checkExistingTexture,
      LPCSTR                fileName,
      UINT                  expectedWidth,
      UINT                  expectedHeight
  );
  void AllocBlankTexture(
      TEXCOMPONENT_SECTIONS section,
      CStatus              *status,
      TEXCOMPONENT_LAYERS   layer,
      EGxTexFormat          format,
      UINT                  width,
      UINT                  height,
      int                   opaque
  );
  void PasteOpaque(
      int                  layer,
      const CTexturePiece &source,
      NTempest::C2iVector  dstPos,
      NTempest::C2iVector  srcPos,
      UINT                 width,
      UINT                 height,
      LAYERPRIORITY        priority
  );

  CTextureLayer m_layers[4];
};

class CTexComponent : public CTexturePiece {
 public:
  CTexComponent()
      : m_texture(0), m_dirtyFlags(0), m_flags(0), m_emblemStyle(-1), m_emblemColor(-1), m_borderStyle(-1), m_borderColor(-1), m_background(-1) {
    m_upperFaceTexture[0] = 0;
    m_lowerFaceTexture[0] = 0;
    m_underwearHideCounts[0] = 0;
    m_underwearHideCounts[1] = 0;
  }

  virtual ~CTexComponent() {
    if (m_texture) {
      HandleClose(m_texture);
    }
  }

  CTexComponent &operator=(const CTexComponent &rhs);

  bool AnySectionsDirty() const {
    return m_dirtyFlags != 0;
  }

  void MarkSectionDirty(TEXCOMPONENT_SECTIONS section) {
    m_dirtyFlags |= 1 << section;
  }

  void MarkDirty() {
    for (UINT section = 0; section < NUM_TEXCOMPONENT_SECTIONS; ++section) {
      m_dirtyFlags |= 1 << section;
    }
  }

  void SetIgnoreExistingTexture(int ignore);
  bool IsTabardSectionLayerAndPriority(TEXCOMPONENT_SECTIONS section, TEXCOMPONENT_LAYERS layer, LAYERPRIORITY priority) const;
  BOOL CheckPastingRules(TEXCOMPONENT_SECTIONS section, TEXCOMPONENT_LAYERS layer, LAYERPRIORITY priority);
  bool HasTabard() const;
  BOOL CheckSections(BOOL bForce);
  void UpdateSections(CStatus *status, BOOL bUpdate);
  BOOL CheckSection(TEXCOMPONENT_SECTIONS section, BOOL bForce);
  void UpdateSection(CStatus *status, TEXCOMPONENT_SECTIONS section, BOOL bUpdate);
  int  Paste(CStatus *status, TEXCOMPONENT_SECTIONS section, TEXCOMPONENT_LAYERS layer, int x, int y, int width, int height);
  void PasteTabardTexture(CStatus *status, TEXCOMPONENT_SECTIONS section);
  void BuildSkinPieces(CStatus *status, UINT *layerHoldSectionFlags);
  void BuildNakedPieces(CStatus *status, UINT race, UINT sex, UINT skinID, BOOL isNPC);
  void HideUnderwear(UINT underwearSection);
  void ShowUnderwear(UINT underwearSection);
  void SetTexture(int checkExistingTexture, HTEXTURE texture);
  void SetTexture(
      CStatus              *status,
      int                   checkExistingTexture,
      LPCSTR                fileName,
      TEXCOMPONENT_SECTIONS section,
      TEXCOMPONENT_LAYERS   layer,
      LAYERPRIORITY         priority,
      UINT                  expectedWidth,
      UINT                  expectedHeight
  );
  void UpdateUnderwearVisibility();
  void RemoveSections(const TEXCOMPONENT_SECTIONS *sectionPointers, const UINT *startLayerList, UINT size);
  void AddHold(INVENTORY_TYPES inventory, TEXCOMPONENT_SECTIONS section);
  void RemoveHold(INVENTORY_TYPES inventory, TEXCOMPONENT_SECTIONS section);
  void RemoveHolds();
  void IncUnderwearHideCount(int itemInventoryType, TEXCOMPONENT_SECTIONS sectionID);
  void DecUnderwearHideCount(int itemInventoryType, TEXCOMPONENT_SECTIONS sectionID);
  void SetUpperHeadTexture(LPCSTR upperHead);
  void SetLowerHeadTexture(LPCSTR lowerHead);

  HTEXTURE m_texture;
  UINT     m_dirtyFlags;
  CSection m_sections[NUM_TEXCOMPONENT_SECTIONS];
  UINT     m_underwearHideCounts[2];
  UINT     m_flags;
  char     m_upperFaceTexture[MAX_PATH];
  char     m_lowerFaceTexture[MAX_PATH];
  int      m_emblemStyle;
  int      m_emblemColor;
  int      m_borderStyle;
  int      m_borderColor;
  int      m_background;
};

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
