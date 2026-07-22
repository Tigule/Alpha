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
  char fileName[6][MAX_PATH];
};

struct SUBCOMPONENTDESC {
  SUBCOMPONENTDESC() : modelName(0), textureName(0), attachmentPoint(0) {
  }
  ~SUBCOMPONENTDESC();

  char        *modelName;
  char        *textureName;
  unsigned int attachmentPoint;
};

struct LAYERIDS {
  TEXCOMPONENT_LAYERS layers[NUM_TEXCOMPONENT_SECTIONS];
};

struct SECTIONPRIORITIES {
  LAYERPRIORITY priorities[NUM_TEXCOMPONENT_SECTIONS];
};

extern const LAYERIDS          g_sectionLayers[INDEX_NUMSLOTS];
extern const SECTIONPRIORITIES g_sectionPriorities[INDEX_NUMSLOTS];

int __fastcall CompUtilGetSectionDimensions(unsigned int sectionIndex, unsigned int *width, unsigned int *height);
int __fastcall CompUtilGetSectionOffset(unsigned int sectionIndex, unsigned int *xCoord, unsigned int *yCoord);
int __fastcall CompUtilItemSectionInfo(INVENTORY_TYPES invType, TEXCOMPONENT_SECTIONS section, TEXCOMPONENT_LAYERS *layer, LAYERPRIORITY *priority);
int __fastcall CompUtilItemSectionInfo(
    const ItemDisplayInfoRec    *displayInfoRec,
    unsigned int                 inventoryType,
    unsigned int                *numTextureComponents,
    TEXCOMPONENT_SECTIONS *const sectionList,
    TEXCOMPONENT_LAYERS *const   layerList,
    LAYERPRIORITY *const         priorityList,
    CSectionFileNames           *fileNameList
);
const char *__fastcall  CompUtilGetTextureSectionName(const ItemDisplayInfoRec *displayInfoRec, unsigned int textureSection);
unsigned int __fastcall CompUtilGetObjComponents(
    const ItemDisplayInfoRec *displayInfoRec,
    int                       itemInventoryType,
    SUBCOMPONENTDESC         *subComponents,
    unsigned int              numSubComponents,
    int                       useAlternate
);
int __fastcall
GetObjComponentInfo(int race, int sex, int displayID, int inventoryType, bool isPlayer, bool useAlternate, HMODEL *models, int *attachmentPoints);
HMODEL __fastcall ObjComponentBuildAmmoModel(ItemDisplayInfoRec *displayInfoRec, unsigned int inventoryType, unsigned int &seqDuration);
void __fastcall
CompDecorateTexName(const char *string, TEXCOMPONENT_SECTIONS section, char *buffer, unsigned int size, unsigned int sex, int includeSex);
void __fastcall CompDecorateObjName(const char *string, char *buffer, unsigned int size, unsigned int race, unsigned int sex);
void __fastcall GetTabardBackgroundFileName(int section, int background, char *buffer, int size);
void __fastcall GetTabardEmblemFileName(int section, int emblem, int color, char *buffer, int size);
void __fastcall GetTabardBorderFileName(int section, int border, int color, char *buffer, int size);

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

  int IsOpaque() const {
    return m_textureInfo.opaque || !m_textureInfo.alphaBits;
  }

  int HasImage() const {
    return m_mippedTexture != 0;
  }

  int HasHolds() const {
    return m_holds != 0;
  }

  void SetHold(unsigned int hold) {
    m_holds |= 1 << hold;
  }

  void ClearHold(unsigned int hold) {
    m_holds &= ~(1 << hold);
  }

  int SetTexture(
      TEXCOMPONENT_SECTIONS section,
      TEXCOMPONENT_LAYERS   layer,
      LAYERPRIORITY         priority,
      CStatus              *status,
      int                   checkExistingTexture,
      const char           *fileName,
      unsigned int          expectedWidth,
      unsigned int          expectedHeight
  );
  void SetTexture(int checkExistingTexture, HTEXTURE texture);
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
  unsigned int   m_holds;

 protected:
  char m_fileName[MAX_PATH];
};

class CTextureLayer {
 public:
  CTextureLayer &operator=(const CTextureLayer &rhs);
  int            IsOpaque() const;
  void           AllocBlankTexture(
      TEXCOMPONENT_SECTIONS section,
      CStatus              *status,
      TEXCOMPONENT_LAYERS   layer,
      EGxTexFormat          format,
      unsigned int          width,
      unsigned int          height,
      int                   opaque
  );
  int SetTexture(
      TEXCOMPONENT_SECTIONS section,
      TEXCOMPONENT_LAYERS   layer,
      LAYERPRIORITY         priority,
      CStatus              *status,
      int                   checkExistingTexture,
      const char           *fileName,
      unsigned int          expectedWidth,
      unsigned int          expectedHeight
  );

  CTexturePiece m_priorities[4];
};

class CSection {
 public:
  CSection     &operator=(const CSection &rhs);
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

  CTexComponent &operator=(const CTexComponent &rhs);

  bool IsTabardSectionLayerAndPriority(TEXCOMPONENT_SECTIONS section, TEXCOMPONENT_LAYERS layer, LAYERPRIORITY priority) const;
  int  CheckPastingRules(TEXCOMPONENT_SECTIONS section, TEXCOMPONENT_LAYERS layer, LAYERPRIORITY priority);
  bool HasTabard() const;
  int  CheckSections(int bForce);
  void UpdateSections(CStatus *status, int bUpdate);
  int  CheckSection(TEXCOMPONENT_SECTIONS section, int bForce);
  void UpdateSection(CStatus *status, TEXCOMPONENT_SECTIONS section, int bUpdate);
  int  Paste(CStatus *status, TEXCOMPONENT_SECTIONS section, TEXCOMPONENT_LAYERS layer, int x, int y, int width, int height);
  void PasteTabardTexture(CStatus *status, TEXCOMPONENT_SECTIONS section);
  void BuildSkinPieces(CStatus *status, unsigned int *layerHoldSectionFlags);
  void BuildNakedPieces(CStatus *status, unsigned int race, unsigned int sex, unsigned int skinID, int isNPC);
  void HideUnderwear(unsigned int underwearSection);
  void ShowUnderwear(unsigned int underwearSection);
  void SetTexture(int checkExistingTexture, HTEXTURE texture);
  void SetTexture(
      CStatus              *status,
      int                   checkExistingTexture,
      const char           *fileName,
      TEXCOMPONENT_SECTIONS section,
      TEXCOMPONENT_LAYERS   layer,
      LAYERPRIORITY         priority,
      unsigned int          expectedWidth,
      unsigned int          expectedHeight
  );
  void UpdateUnderwearVisibility();
  void RemoveSections(const TEXCOMPONENT_SECTIONS *sectionPointers, const unsigned int *startLayerList, unsigned int size);
  void AddHold(INVENTORY_TYPES inventory, TEXCOMPONENT_SECTIONS section);
  void RemoveHold(INVENTORY_TYPES inventory, TEXCOMPONENT_SECTIONS section);
  void RemoveHolds();
  void IncUnderwearHideCount(int itemInventoryType, TEXCOMPONENT_SECTIONS sectionID);
  void SetUpperHeadTexture(const char *upperHead);
  void SetLowerHeadTexture(const char *lowerHead);

  HTEXTURE     m_texture;
  unsigned int m_dirtyFlags;
  CSection     m_sections[NUM_TEXCOMPONENT_SECTIONS];
  unsigned int m_underwearHideCounts[2];
  unsigned int m_flags;
  char         m_upperFaceTexture[MAX_PATH];
  char         m_lowerFaceTexture[MAX_PATH];
  int          m_emblemStyle;
  int          m_emblemColor;
  int          m_borderStyle;
  int          m_borderColor;
  int          m_background;
};

void __fastcall ComponentInitialize();
void __fastcall ComponentShutdown();
bool __fastcall ComponentApplyTabardTexture(HTEXCOMPONENT component, int eStyle, int eColor, int bStyle, int bColor, int b);
void __fastcall ComponentRemoveTabardTexture(int sex, HTEXCOMPONENT component, ItemDisplayInfoRec *displayInfo, int inventoryType);
void __fastcall ComponentForceTabardDraw(HTEXCOMPONENT component);
void __fastcall TexComponentCopy(HTEXCOMPONENT d, HTEXCOMPONENT s);
int __fastcall  TexComponentCommitSections(CStatus *status, HTEXCOMPONENT component, int bForce);
int __fastcall  TexComponentCheckSections(HTEXCOMPONENT component, int bForce);
void __fastcall TexComponentRemoveSections(
    HTEXCOMPONENT                component,
    const TEXCOMPONENT_SECTIONS *sectionPointers,
    const unsigned int          *startLayerList,
    unsigned int                 size
);
void __fastcall TexComponentRemoveAllHolds(HTEXCOMPONENT component);
void __fastcall TexComponentAddHold(HTEXCOMPONENT component, INVENTORY_TYPES inventory, TEXCOMPONENT_SECTIONS section);
void __fastcall TexComponentRemoveHold(HTEXCOMPONENT component, INVENTORY_TYPES inventory, TEXCOMPONENT_SECTIONS section);
HTEXCOMPONENT __fastcall
TexComponentCreate(HTEXTURE texture, unsigned int race, unsigned int sex, unsigned int skinID, int isNPC, int ignoreExistingTexture);
void __fastcall TexComponentAdd(
    CStatus                  *status,
    int                       playerSex,
    HTEXCOMPONENT             component,
    const ItemDisplayInfoRec *displayInfoRec,
    int                       itemInventoryType,
    int                       checkForExistingTexture
);
void __fastcall TexComponentChangeCharacterHead(HTEXCOMPONENT component, const char *upperHead, const char *lowerHead, unsigned int layer);
void __fastcall HeadGeosetHideCharGeosets(
    HCHARGEOSET               geosetHandle,
    const ItemDisplayInfoRec *displayInfoRec,
    unsigned int              raceID,
    const unsigned int       *preferredGeosets,
    unsigned int              numPreferredGeosets
);
typedef void(__fastcall *OBJCALLBACK)(void *param, unsigned int inventorySlot, HMODEL model, unsigned int unk, int loaded);
int __fastcall ObjComponentAdd(
    int                       unitSex,
    int                       unitRace,
    int                       unitPlayer,
    HMODEL                    model,
    const ItemDisplayInfoRec *displayInfoRec,
    int                       itemInventoryType,
    int                       useAlternateSlot,
    HMODEL                    existingModel,
    OBJCALLBACK               callback,
    void                     *param,
    unsigned int              inventorySlot
);
