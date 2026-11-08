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
  CHARGEOSET_HAIR = 0,
  CHARGEOSET_BEARD = 1,
  CHARGEOSET_SIDEBURN = 2,
  CHARGEOSET_MOUSTACHE = 3,
  CHARGEOSET_GLOVE = 4,
  CHARGEOSET_BOOT = 5,
  CHARGEOSET_OBSOLETEDONTUSEME = 6,
  CHARGEOSET_EAR = 7,
  CHARGEOSET_SLEEVES = 8,
  CHARGEOSET_PANTS = 9,
  CHARGEOSET_DOUBLET = 10,
  CHARGEOSET_PANTDOUBLET = 11,
  CHARGEOSET_TABARD = 12,
  CHARGEOSET_ROBE = 13,
  CHARGEOSET_LOINCLOTH = 14,
  NUM_CHARGEOSETS = 15,
  CHARGEOSET_NONE = -1
};

enum CHARACTER_ITEM_GEOSETS {
  CHARITEMGEOSETS_GLOVES = 0,
  CHARITEMGEOSETS_BOOTS = 1,
  CHARITEMGEOSETS_SLEEVES = 2,
  CHARITEMGEOSETS_PANTS = 3,
  CHARITEMGEOSETS_DOUBLET = 4,
  CHARITEMGEOSETS_PANTDOUBLET = 5,
  CHARITEMGEOSETS_TABARD = 6,
  CHARITEMGEOSETS_ROBE = 7,
  CHARITEMGEOSETS_LOINCLOTH = 8,
  NUM_CHARITEMGEOSETS = 9,
  INVALID_CHARITEMGEOSET = -1
};

enum {
  CHAR_TEXID_HAIR = 0,
  CHAR_TEXID_SCALP_UPPER = 1,
  CHAR_TEXID_SCALP_LOWER = 2,
  CHAR_TEXID_FACIAL_HAIR = 3,
  CHAR_TEXID_FACIAL_UPPER = 4,
  CHAR_TEXID_FACIAL_LOWER = 5,
  NUM_CHAR_TEXIDS = 6
};

struct ITEMGEOSETGROUPS {
  CHARACTER_ITEM_GEOSETS geosetGroup[4];
};

extern const CHARTEXTUREVARIATIONS g_charTextureSectionMapping[CHARTEXTURESECTION_NUM];
extern LPCSTR const                g_sexString[UNITSEX_LAST];
extern const ITEMGEOSETGROUPS      g_geosetGroupsPerItem[];

extern UINT g_defaultGeosetIDOffsets[NUM_CHARGEOSETS];
extern int  g_charGeosetIsOverlay[NUM_CHARGEOSETS];

struct BEARDSTYLEDATA {
  UINT beardGeoset;
  UINT sideBurnGeoset;
  UINT moustacheGeoset;

  BEARDSTYLEDATA()
      : beardGeoset(g_defaultGeosetIDOffsets[1]), sideBurnGeoset(g_defaultGeosetIDOffsets[2]), moustacheGeoset(g_defaultGeosetIDOffsets[3]) {
  }
};

void     CharCustomizationInitialize();
void     CharCustomizationShutdown();
void     CharCustomizationGetNumSkinTextures(UINT raceID, UINT sexID, int *pcVars, int *npcVars);
HTEXTURE CharCustomizationLoadSkin(HMODEL characterModel, LPCSTR skinName, UINT raceID, UINT sexID, UINT textureNumber, BOOL isNPC);
HTEXTURE CharCustomizationSetSkin(HMODEL characterModel, UINT raceID, UINT sexID, UINT textureNumber, BOOL isNPC);
BOOL     CharCustomizationGetNakedSectionName(
    UINT  raceID,
    UINT  sexID,
    UINT  skinID,
    UINT  underwearSection,
    char *outBuffer,
    UINT  outBufferSize,
    BOOL  isNPC
);
void CharCustomizationNumFaces(UINT raceID, UINT sexID, int *pcVars, int *npcVars);
void CharCustomizationSetFaceTexture(
    HMODEL        characterModel,
    HTEXCOMPONENT texComponent,
    UINT          raceID,
    UINT          sexID,
    UINT          varID,
    UINT          colorID,
    BOOL          isNPC
);
void CharCustomizationSetHairTexture(HMODEL characterModel, HTEXCOMPONENT texComponent, UINT raceID, UINT sexID, UINT hairID, UINT colorID);
UINT CharCustomizationGetHairGeoset(UINT race, UINT sex, UINT hair);
UINT CharCustomizationNumHairStyles(UINT raceID, UINT sexID);
UINT CharCustomizationNumHairColors(UINT raceID, UINT sexID);
UINT CharCustomizationNumBeardStyles(UINT raceID, UINT sexID);
BOOL CharCustomizationGetBeardStyle(UINT raceID, UINT sexID, UINT facialHairID, BEARDSTYLEDATA *facialHairStyleData);
void CharCustomizationSetFacialTexture(HMODEL characterModel, HTEXCOMPONENT texComponent, UINT raceID, UINT sexID, UINT facialID, UINT colorID);
HCHARGEOSET CharCustomizationCreateGeosetHandle(HMODEL characterModel);
void        CharCustomizationSetPaperDollGeoset(HCHARGEOSET handle, HMODEL paperDollModel);
void        CharCustomizationInitBaseCharacter(HCHARGEOSET geosetHandle, UINT beardGeoset, UINT sideBurnGeoset, UINT moustacheGeoset, UINT earGeoset);
void        CharCustomizationResetHairGeoset(HCHARGEOSET geosetHandle, UINT race, UINT sex, UINT hairStyleID);
void        CharCustomizationAddItemGeosets(
    HCHARGEOSET               geosetHandle,
    const ItemDisplayInfoRec *displayInfoRec,
    UINT                      itemInventoryType,
    HTEXCOMPONENT             component,
    UINT                      raceID,
    int                       doNotCommit
);
void CharCustomizationRemoveItemGeosets(
    HCHARGEOSET               geosetHandle,
    const ItemDisplayInfoRec *displayInfoRec,
    UINT                      itemInventoryType,
    HTEXCOMPONENT             component
);
void CharCustomizationCommitItemGeosets(HCHARGEOSET geosetHandle, int doNotCommitGeosets);
void CharCustomizationCommitItemGeosets(HCHARGEOSET geosetHandle, int doNotCommitGeosets, HMODEL paperDollModel);
void CharCustomizationClearItemGeosets(HCHARGEOSET geosetHandle);
void CharCustomizationCommitGeosets(HCHARGEOSET handle);
void CharCustomizationShowGeoset(HCHARGEOSET handle, CHARACTER_GEOSET_SECTIONS section, UINT geosetNumber);
void CharCustomizationHideGeosetSection(HCHARGEOSET handle, CHARACTER_GEOSET_SECTIONS section);
void CharCustomizationGetTextureLayerHolds(UINT raceID, UINT sexID, UINT *textureLayerHolds, UINT numTextureLayerHolds);
