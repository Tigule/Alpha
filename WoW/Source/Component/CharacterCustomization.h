#pragma once

#include "Object/Unit.h"

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
class CharacterFacialHairStylesRec;

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
  ~CHARACTERVARIATIONS() {
  }

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

  ~CHARACTERSEXVARIATIONS() {
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
  FACIALGEOSETS() {
  }

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
  ~FACIALVARIATIONS() {
  }

  void AddVariation(const CharacterFacialHairStylesRec *);
  unsigned int NumVariations();

  TSFixedArray<FACIALGEOSETS> facialGeosets;
};

struct INTDATA {
  INTDATA() {
  }

  operator int &() {
    return theInt;
  }

  int theInt;
};

struct VARIATIONS {
  ~VARIATIONS() {
  }

  unsigned int             textureHolds[CHARTEXTUREVARIATIONS_NUM];
  FACIALVARIATIONS         facialVariations;
  TSGrowableArray<INTDATA> hairGeosets;
};

void CharCustomizationInitialize();
void CharCustomizationShutdown();
void CharCustomizationGetNumSkinTextures(unsigned int raceID, unsigned int sexID, int *pcVars, int *npcVars);
HTEXTURE CharCustomizationLoadSkin(
    HMODEL       characterModel,
    const char  *skinName,
    unsigned int raceID,
    unsigned int sexID,
    unsigned int textureNumber,
    int          isNPC
);
HTEXTURE CharCustomizationSetSkin(HMODEL characterModel, unsigned int raceID, unsigned int sexID, unsigned int textureNumber, int isNPC);
int CharCustomizationGetNakedSectionName(
    unsigned int raceID,
    unsigned int sexID,
    unsigned int skinID,
    unsigned int underwearSection,
    char        *outBuffer,
    unsigned int outBufferSize,
    int          isNPC
);
void CharCustomizationNumFaces(unsigned int raceID, unsigned int sexID, int *pcVars, int *npcVars);
void CharCustomizationSetFaceTexture(
    HMODEL        characterModel,
    HTEXCOMPONENT texComponent,
    unsigned int  raceID,
    unsigned int  sexID,
    unsigned int  varID,
    unsigned int  colorID,
    int           isNPC
);
void CharCustomizationSetHairTexture(
    HMODEL        characterModel,
    HTEXCOMPONENT texComponent,
    unsigned int  raceID,
    unsigned int  sexID,
    unsigned int  hairID,
    unsigned int  colorID
);
unsigned int CharCustomizationGetHairGeoset(unsigned int race, unsigned int sex, unsigned int hair);
unsigned int CharCustomizationNumHairStyles(unsigned int raceID, unsigned int sexID);
unsigned int CharCustomizationNumHairColors(unsigned int raceID, unsigned int sexID);
unsigned int CharCustomizationNumBeardStyles(unsigned int raceID, unsigned int sexID);
int
CharCustomizationGetBeardStyle(unsigned int raceID, unsigned int sexID, unsigned int facialHairID, BEARDSTYLEDATA *facialHairStyleData);
void CharCustomizationSetFacialTexture(
    HMODEL        characterModel,
    HTEXCOMPONENT texComponent,
    unsigned int  raceID,
    unsigned int  sexID,
    unsigned int  facialID,
    unsigned int  colorID
);
HCHARGEOSET CharCustomizationCreateGeosetHandle(HMODEL characterModel);
void CharCustomizationSetPaperDollGeoset(HCHARGEOSET handle, HMODEL paperDollModel);
void CharCustomizationInitBaseCharacter(
    HCHARGEOSET  geosetHandle,
    unsigned int beardGeoset,
    unsigned int sideBurnGeoset,
    unsigned int moustacheGeoset,
    unsigned int earGeoset
);
void CharCustomizationResetHairGeoset(HCHARGEOSET geosetHandle, unsigned int race, unsigned int sex, unsigned int hairStyleID);
void CharCustomizationAddItemGeosets(
    HCHARGEOSET               geosetHandle,
    const ItemDisplayInfoRec *displayInfoRec,
    unsigned int              itemInventoryType,
    HTEXCOMPONENT             component,
    unsigned int              raceID,
    int                       doNotCommit
);
void CharCustomizationRemoveItemGeosets(
    HCHARGEOSET               geosetHandle,
    const ItemDisplayInfoRec *displayInfoRec,
    unsigned int              itemInventoryType,
    HTEXCOMPONENT             component
);
void CharCustomizationCommitItemGeosets(HCHARGEOSET geosetHandle, int doNotCommitGeosets);
void CharCustomizationCommitItemGeosets(HCHARGEOSET geosetHandle, int doNotCommitGeosets, HMODEL paperDollModel);
void CharCustomizationClearItemGeosets(HCHARGEOSET geosetHandle);
void CharCustomizationCommitGeosets(HCHARGEOSET handle);
void CharCustomizationShowGeoset(HCHARGEOSET handle, CHARACTER_GEOSET_SECTIONS section, unsigned int geosetNumber);
void CharCustomizationHideGeosetSection(HCHARGEOSET handle, CHARACTER_GEOSET_SECTIONS section);
void
CharCustomizationGetTextureLayerHolds(unsigned int raceID, unsigned int sexID, unsigned int *textureLayerHolds, unsigned int numTextureLayerHolds);
