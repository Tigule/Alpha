#pragma once

#include <stpl.h>

struct HMODEL__;
typedef HMODEL__ *HMODEL;
struct HTEXTURE__;
typedef HTEXTURE__ *HTEXTURE;
struct HTEXCOMPONENT__;
typedef HTEXCOMPONENT__ *HTEXCOMPONENT;
struct HCHARGEOSET__;
typedef HCHARGEOSET__ *HCHARGEOSET;
class ItemDisplayInfoRec;

enum UNIT_SEX {
  UNITSEX_MALE = 0,
  UNITSEX_FEMALE = 1,
  UNITSEX_NONE = 2,
  UNITSEX_LAST = 3,
  UNITSEX_BOTH = 3
};

enum CHARTEXTURESECTIONID {
  CHARTEXTURESECTION_SKIN = 0,
  CHARTEXTURESECTION_NAKEDSKINPELVIS = 1,
  CHARTEXTURESECTION_NAKEDSKINTORSO = 2,
  CHARTEXTURESECTION_SKINEXTRA = 3,
  CHARTEXTURESECTION_FACELOWER = 4,
  CHARTEXTURESECTION_FACEUPPER = 5,
  CHARTEXTURESECTION_HAIR = 6,
  CHARTEXTURESECTION_SCALPLOWERHAIR = 7,
  CHARTEXTURESECTION_SCALPUPPERHAIR = 8,
  CHARTEXTURESECTION_FACIALLOWERHAIR = 9,
  CHARTEXTURESECTION_FACIALUPPERHAIR = 10,
  CHARTEXTURESECTION_NUM = 11
};

enum CHARTEXTUREVARIATIONS {
  CHARTEXTUREVAR_SKIN = 0,
  CHARTEXTUREVAR_FACE = 1,
  CHARTEXTUREVAR_HAIR = 2,
  CHARTEXTUREVAR_FACIALHAIR = 3,
  CHARTEXTUREVARIATIONS_NUM = 4
};

enum CHARACTER_GEOSET_SECTIONS {
  CGS_HAIR = 0,
  CGS_FACIAL_BEARD = 1,
  CGS_FACIAL_SIDEBURN = 2,
  CGS_FACIAL_MOUSTACHE = 3,
  CGS_GLOVES = 4,
  CGS_BOOTS = 5,
  CGS_SECTION_6 = 6,
  CGS_EARS = 7,
  CGS_SLEEVES = 8,
  CGS_PANTS = 9,
  CGS_CHEST = 10,
  CGS_TABARD = 11,
  CGS_ROBE = 12,
  CGS_CLOAK = 13,
  CGS_SECTION_14 = 14,
  NUM_CHARGEOSETS = 15
};

extern const CHARTEXTUREVARIATIONS g_charTextureSectionMapping[CHARTEXTURESECTION_NUM];
extern const char *const           g_sexString[UNITSEX_LAST];

struct CAMERAFILENAMES {
  CAMERAFILENAMES() {
    unsigned int sex;

    for (sex = 0; sex < UNITSEX_LAST; ++sex) {
      fileName[sex][0] = 0;
    }
  }

  char fileName[UNITSEX_LAST][MAX_PATH];
};

struct STRINGWANNABE {
  STRINGWANNABE() : string(0) {
  }

  void SetString(const char *prefix, const char *value) {
    char textureName[MAX_PATH];

    SStrPrintf(textureName, sizeof(textureName), "%s%s", prefix, value);
    string = value;
  }

  const char *GetString() {
    return string;
  }

 private:
  const char *string;
};

struct CHARACTERVARIATIONS {
  STRINGWANNABE &GetColor(int colorID) {
    return color[colorID % color.Count()];
  }

  const STRINGWANNABE &GetColor(int colorID) const {
    return color[colorID % color.Count()];
  }

  int GetColorCount() const {
    return color.Count();
  }

  void SetColorCount(int count) {
    color.SetCount(count);
  }

 private:
  TSGrowableArray<STRINGWANNABE> color;
};

struct CHARACTERSEXVARIATIONS {
  CHARACTERSEXVARIATIONS() {
    unsigned int section;

    for (section = 0; section < CHARTEXTURESECTION_NUM; ++section) {
      firstNPCVar[section] = INT_MAX;
      lastNPCVar[section] = -1;
    }
  }

  void GetNumVariations(CHARTEXTURESECTIONID section, int *pcVars, int *npcVars);

  CHARACTERVARIATIONS &GetNames(int section, int variation) {
    ASSERT(section < (sizeof(names) / sizeof(names[0])));
    return names[section][variation % names[section].Count()];
  }

  const CHARACTERVARIATIONS &GetNames(int section, int variation) const {
    ASSERT(section < (sizeof(names) / sizeof(names[0])));
    return names[section][variation % names[section].Count()];
  }

  int NumVariations(int section) const {
    ASSERT(section < (sizeof(names) / sizeof(names[0])));
    return names[section].Count();
  }

  TSGrowableArray<CHARACTERVARIATIONS> &GetSectionData(int section) {
    ASSERT(section < (sizeof(names) / sizeof(names[0])));
    return names[section];
  }

  const TSGrowableArray<CHARACTERVARIATIONS> &GetSectionData(int section) const {
    ASSERT(section < (sizeof(names) / sizeof(names[0])));
    return names[section];
  }

 private:
  TSGrowableArray<CHARACTERVARIATIONS> names[CHARTEXTURESECTION_NUM];

 public:
  int firstNPCVar[CHARTEXTURESECTION_NUM];
  int lastNPCVar[CHARTEXTURESECTION_NUM];
};

struct CHARACTERRACEVARIATIONS {
  CHARACTERSEXVARIATIONS sex[UNITSEX_LAST];
};

struct FACIALGEOSETS {
  unsigned int beardGeoset;
  unsigned int sideBurnGeoset;
  unsigned int moustacheGeoset;
};

struct BEARDSTYLEDATA {
  unsigned int beardGeoset;
  unsigned int sideBurnGeoset;
  unsigned int moustacheGeoset;
};

struct FACIALVARIATIONS {
  TSFixedArray<FACIALGEOSETS> facialGeosets;
};

struct INTDATA {
  operator int &() {
    return theInt;
  }

  int theInt;
};

struct VARIATIONS {
  unsigned int             textureHolds[CHARTEXTUREVARIATIONS_NUM];
  FACIALVARIATIONS         facialVariations;
  TSGrowableArray<INTDATA> hairGeosets;
};

void __fastcall     CharCustomizationInitialize();
void __fastcall     CharCustomizationShutdown();
void __fastcall     CharCustomizationGetNumSkinTextures(unsigned int raceID, unsigned int sexID, int *pcVars, int *npcVars);
HTEXTURE __fastcall CharCustomizationLoadSkin(
    HMODEL       characterModel,
    const char  *skinName,
    unsigned int raceID,
    unsigned int sexID,
    unsigned int textureNumber,
    int          isNPC
);
HTEXTURE __fastcall CharCustomizationSetSkin(HMODEL characterModel, unsigned int raceID, unsigned int sexID, unsigned int textureNumber, int isNPC);
int __fastcall      CharCustomizationGetNakedSectionName(
    unsigned int raceID,
    unsigned int sexID,
    unsigned int skinID,
    unsigned int underwearSection,
    char        *outBuffer,
    unsigned int outBufferSize,
    int          isNPC
);
void __fastcall CharCustomizationNumFaces(unsigned int raceID, unsigned int sexID, int *pcVars, int *npcVars);
void __fastcall CharCustomizationSetFaceTexture(
    HMODEL        characterModel,
    HTEXCOMPONENT texComponent,
    unsigned int  raceID,
    unsigned int  sexID,
    unsigned int  varID,
    unsigned int  colorID,
    int           isNPC
);
void __fastcall CharCustomizationSetHairTexture(
    HMODEL        characterModel,
    HTEXCOMPONENT texComponent,
    unsigned int  raceID,
    unsigned int  sexID,
    unsigned int  hairID,
    unsigned int  colorID
);
unsigned int __fastcall CharCustomizationGetHairGeoset(unsigned int race, unsigned int sex, unsigned int hair);
unsigned int __fastcall CharCustomizationNumHairStyles(unsigned int raceID, unsigned int sexID);
unsigned int __fastcall CharCustomizationNumHairColors(unsigned int raceID, unsigned int sexID);
unsigned int __fastcall CharCustomizationNumBeardStyles(unsigned int raceID, unsigned int sexID);
int __fastcall
CharCustomizationGetBeardStyle(unsigned int raceID, unsigned int sexID, unsigned int facialHairID, BEARDSTYLEDATA *facialHairStyleData);
void __fastcall CharCustomizationSetFacialTexture(
    HMODEL        characterModel,
    HTEXCOMPONENT texComponent,
    unsigned int  raceID,
    unsigned int  sexID,
    unsigned int  facialID,
    unsigned int  colorID
);
HCHARGEOSET __fastcall CharCustomizationCreateGeosetHandle(HMODEL characterModel);
void __fastcall        CharCustomizationSetPaperDollGeoset(HCHARGEOSET handle, HMODEL paperDollModel);
void __fastcall        CharCustomizationInitBaseCharacter(
    HCHARGEOSET  geosetHandle,
    unsigned int beardGeoset,
    unsigned int sideBurnGeoset,
    unsigned int moustacheGeoset,
    unsigned int earGeoset
);
void __fastcall CharCustomizationResetHairGeoset(HCHARGEOSET geosetHandle, unsigned int race, unsigned int sex, unsigned int hairStyleID);
void __fastcall CharCustomizationAddItemGeosets(
    HCHARGEOSET               geosetHandle,
    const ItemDisplayInfoRec *displayInfoRec,
    unsigned int              itemInventoryType,
    HTEXCOMPONENT             component,
    unsigned int              raceID,
    int                       doNotCommit
);
void __fastcall CharCustomizationRemoveItemGeosets(
    HCHARGEOSET               geosetHandle,
    const ItemDisplayInfoRec *displayInfoRec,
    unsigned int              itemInventoryType,
    HTEXCOMPONENT             component
);
void __fastcall CharCustomizationCommitItemGeosets(HCHARGEOSET geosetHandle, int doNotCommitGeosets);
void __fastcall CharCustomizationCommitItemGeosets(HCHARGEOSET geosetHandle, int doNotCommitGeosets, HMODEL paperDollModel);
void __fastcall CharCustomizationClearItemGeosets(HCHARGEOSET geosetHandle);
void __fastcall CharCustomizationCommitGeosets(HCHARGEOSET handle);
void __fastcall CharCustomizationShowGeoset(HCHARGEOSET handle, CHARACTER_GEOSET_SECTIONS section, unsigned int geosetNumber);
void __fastcall CharCustomizationHideGeosetSection(HCHARGEOSET handle, CHARACTER_GEOSET_SECTIONS section);
void __fastcall
CharCustomizationGetTextureLayerHolds(unsigned int raceID, unsigned int sexID, unsigned int *textureLayerHolds, unsigned int numTextureLayerHolds);
