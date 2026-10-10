#include <Base/Base.h>
#include <Gx/Gx.h>
#include <WowConst.h>
#include <MapDefs.h>

#include <Component/Component.h>

#include "DB/DBClient/AutoCode/ItemDisplayInfoRec.h"

#include <Model/IModel.h>
#include <Services/Texture.h>

#include <string.h>

struct SECTIONDESC {
  char *m_columnName;
  UINT  x;
  UINT  y;
  UINT  width;
  UINT  height;
};

extern LPCSTR const s_sectionDirectorynames[NUM_TEXCOMPONENT_SECTIONS] = {"ArmUpperTexture",   "ArmLowerTexture",   "HandTexture",     "HeadUpperTexture",
                                                                           "HeadLowerTexture",  "TorsoUpperTexture", "TorsoLowerTexture", "LegUpperTexture",
                                                                           "LegLowerTexture",   "FootTexture"};

static const SECTIONDESC s_sectionTable[NUM_TEXCOMPONENT_SECTIONS] = {
    {  "ArmUpperTexture",   0,   0, 128, 64},
    {  "ArmLowerTexture",   0,  64, 128, 64},
    {      "HandTexture",   0, 128, 128, 32},
    { "HeadUpperTexture",   0, 160, 128, 32},
    { "HeadLowerTexture",   0, 192, 128, 64},
    {"TorsoUpperTexture", 128,   0, 128, 64},
    {"TorsoLowerTexture", 128,  64, 128, 32},
    {  "LegUpperTexture", 128,  96, 128, 64},
    {  "LegLowerTexture", 128, 160, 128, 64},
    {      "FootTexture", 128, 224, 128, 32}
};

static const UINT s_sectionFlags[INDEX_NUMSLOTS] = {0, 0, 0, 0, 0x0E3, 0x0E3, 0x080, 0x180, 0x300, 0x002, 0x006, 0, 0, 0,
                                                    0, 0, 0, 0, 0,     0x0E0, 0x1E3, 0,     0,     0,     0,     0, 0};

static const int s_textureSections[NUM_TEXCOMPONENT_SECTIONS] = {0, 1, 2, -1, -1, 3, 4, 5, 6, 7};

extern const LAYERIDS g_sectionLayers[INDEX_NUMSLOTS] = {
    {{TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE,
      TEXLAYER_NONE}},
    {{TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE,
      TEXLAYER_NONE}},
    {{TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE,
      TEXLAYER_NONE}},
    {{TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE,
      TEXLAYER_NONE}},
    {{TEXLAYER_CLOTH, TEXLAYER_CLOTH, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_CLOTH, TEXLAYER_CLOTH, TEXLAYER_ARMOR, TEXLAYER_NONE,
      TEXLAYER_NONE}},
    {{TEXLAYER_ARMOR, TEXLAYER_CLOTH, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_ARMOR, TEXLAYER_ARMOR, TEXLAYER_ARMOR, TEXLAYER_NONE,
      TEXLAYER_NONE}},
    {{TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_ARMOR, TEXLAYER_NONE,
      TEXLAYER_NONE}},
    {{TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_CLOTH, TEXLAYER_CLOTH,
      TEXLAYER_NONE}},
    {{TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_ARMOR,
      TEXLAYER_ARMOR}},
    {{TEXLAYER_NONE, TEXLAYER_CLOTH, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE,
      TEXLAYER_NONE}},
    {{TEXLAYER_NONE, TEXLAYER_ARMOR, TEXLAYER_ARMOR, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE,
      TEXLAYER_NONE}},
    {{TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE,
      TEXLAYER_NONE}},
    {{TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE,
      TEXLAYER_NONE}},
    {{TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE,
      TEXLAYER_NONE}},
    {{TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE,
      TEXLAYER_NONE}},
    {{TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE,
      TEXLAYER_NONE}},
    {{TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE,
      TEXLAYER_NONE}},
    {{TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE,
      TEXLAYER_NONE}},
    {{TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE,
      TEXLAYER_NONE}},
    {{TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_ARMOR, TEXLAYER_ARMOR, TEXLAYER_ARMOR, TEXLAYER_NONE,
      TEXLAYER_NONE}},
    {{TEXLAYER_ARMOR, TEXLAYER_CLOTH, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_ARMOR, TEXLAYER_ARMOR, TEXLAYER_CLOTH, TEXLAYER_ARMOR,
      TEXLAYER_NONE}},
    {{TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE,
      TEXLAYER_NONE}},
    {{TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE,
      TEXLAYER_NONE}},
    {{TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE,
      TEXLAYER_NONE}},
    {{TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE,
      TEXLAYER_NONE}},
    {{TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE,
      TEXLAYER_NONE}},
    {{TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE, TEXLAYER_NONE,
      TEXLAYER_NONE}}
};

extern const SECTIONPRIORITIES g_sectionPriorities[INDEX_NUMSLOTS] = {
    {{LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0,
      LAYERPRIORITY_0, LAYERPRIORITY_0}},
    {{LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0,
      LAYERPRIORITY_0, LAYERPRIORITY_0}},
    {{LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0,
      LAYERPRIORITY_0, LAYERPRIORITY_0}},
    {{LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0,
      LAYERPRIORITY_0, LAYERPRIORITY_0}},
    {{LAYERPRIORITY_0, LAYERPRIORITY_1, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0,
      LAYERPRIORITY_0, LAYERPRIORITY_0}},
    {{LAYERPRIORITY_0, LAYERPRIORITY_2, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_1,
      LAYERPRIORITY_2, LAYERPRIORITY_0}},
    {{LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_3,
      LAYERPRIORITY_0, LAYERPRIORITY_0}},
    {{LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0,
      LAYERPRIORITY_0, LAYERPRIORITY_0}},
    {{LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0,
      LAYERPRIORITY_0, LAYERPRIORITY_0}},
    {{LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0,
      LAYERPRIORITY_0, LAYERPRIORITY_0}},
    {{LAYERPRIORITY_0, LAYERPRIORITY_2, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0,
      LAYERPRIORITY_0, LAYERPRIORITY_0}},
    {{LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0,
      LAYERPRIORITY_0, LAYERPRIORITY_0}},
    {{LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0,
      LAYERPRIORITY_0, LAYERPRIORITY_0}},
    {{LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0,
      LAYERPRIORITY_0, LAYERPRIORITY_0}},
    {{LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0,
      LAYERPRIORITY_0, LAYERPRIORITY_0}},
    {{LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0,
      LAYERPRIORITY_0, LAYERPRIORITY_0}},
    {{LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0,
      LAYERPRIORITY_0, LAYERPRIORITY_0}},
    {{LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0,
      LAYERPRIORITY_0, LAYERPRIORITY_0}},
    {{LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0,
      LAYERPRIORITY_0, LAYERPRIORITY_0}},
    {{LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_2, LAYERPRIORITY_2, LAYERPRIORITY_2,
      LAYERPRIORITY_0, LAYERPRIORITY_0}},
    {{LAYERPRIORITY_1, LAYERPRIORITY_3, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_1, LAYERPRIORITY_1, LAYERPRIORITY_1,
      LAYERPRIORITY_2, LAYERPRIORITY_0}},
    {{LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0,
      LAYERPRIORITY_0, LAYERPRIORITY_0}},
    {{LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0,
      LAYERPRIORITY_0, LAYERPRIORITY_0}},
    {{LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0,
      LAYERPRIORITY_0, LAYERPRIORITY_0}},
    {{LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0,
      LAYERPRIORITY_0, LAYERPRIORITY_0}},
    {{LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0,
      LAYERPRIORITY_0, LAYERPRIORITY_0}},
    {{LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0, LAYERPRIORITY_0,
      LAYERPRIORITY_0, LAYERPRIORITY_0}}
};

BOOL CompUtilGetSectionDimensions(UINT sectionIndex, UINT *width, UINT *height) {
  VALIDATEBEGIN;
  VALIDATE(width);
  VALIDATE(height);
  VALIDATEEND;

  if (sectionIndex >= NUM_TEXCOMPONENT_SECTIONS) {
    return 0;
  }

  *width = s_sectionTable[sectionIndex].width;
  *height = s_sectionTable[sectionIndex].height;
  return 1;
}

BOOL CompUtilGetSectionOffset(UINT sectionIndex, UINT *xCoord, UINT *yCoord) {
  VALIDATEBEGIN;
  VALIDATE(xCoord);
  VALIDATE(yCoord);
  VALIDATEEND;

  if (sectionIndex >= NUM_TEXCOMPONENT_SECTIONS) {
    return 0;
  }

  *xCoord = s_sectionTable[sectionIndex].x;
  *yCoord = s_sectionTable[sectionIndex].y;
  return 1;
}

BOOL CompUtilItemSectionInfo(
    const ItemDisplayInfoRec *displayInfoRec,
    UINT                      inventoryType,
    UINT                     *numTextureComponents,
    TEXCOMPONENT_SECTIONS     sectionList[6],
    TEXCOMPONENT_LAYERS       layerList[6],
    LAYERPRIORITY             priorityList[6],
    CSectionFileNames        *fileNameList
) {
  ASSERT(inventoryType < INDEX_NUMSLOTS);
  ASSERT(numTextureComponents);

  UINT numSections = 0;
  UINT section;
  for (section = 0; section < NUM_TEXCOMPONENT_SECTIONS; ++section) {
    if (numSections >= 6) {
      break;
    }

    if (s_sectionFlags[inventoryType] & (1 << section)) {
      sectionList[numSections] = (TEXCOMPONENT_SECTIONS)section;
      layerList[numSections] = g_sectionLayers[inventoryType].layers[section];
      priorityList[numSections] = g_sectionPriorities[inventoryType].priorities[section];

      LPCSTR fileName = CompUtilGetTextureSectionName(displayInfoRec, section);
      if (fileNameList) {
        if (fileName && *fileName) {
          SStrCopy(fileNameList->path[numSections], fileName, MAX_PATH);
        } else {
          fileNameList->path[numSections][0] = 0;
        }
      }

      ++numSections;
    }
  }

  *numTextureComponents = numSections;
  return 1;
}

BOOL CompUtilItemSectionInfo(INVENTORY_TYPES invType, TEXCOMPONENT_SECTIONS section, TEXCOMPONENT_LAYERS *layer, LAYERPRIORITY *priority) {
  ASSERT(invType < INDEX_NUMSLOTS);
  ASSERT(section < NUM_TEXCOMPONENT_SECTIONS);
  ASSERT(layer);
  ASSERT(priority);

  *layer = g_sectionLayers[invType].layers[section];
  if (*layer == TEXLAYER_NONE) {
    return 0;
  }

  *priority = g_sectionPriorities[invType].priorities[section];
  return 1;
}

static BOOL ReadSubComponent(const ItemDisplayInfoRec *displayInfoRec, UINT whichComponent, UINT inventoryType, SUBCOMPONENTDESC *subComp) {
  ASSERT(inventoryType < INDEX_NUMSLOTS);
  ASSERT(displayInfoRec != 0);
  ASSERT(whichComponent < 2);

  if (subComp) {
    if (subComp->pathName) {
      SMemFree(subComp->pathName, __FILE__, __LINE__, 0);
    }
    if (subComp->textureName) {
      SMemFree(subComp->textureName, __FILE__, __LINE__, 0);
    }
    subComp->pathName = 0;
    subComp->textureName = 0;
  }

  if (!displayInfoRec) {
    return 0;
  }
  LPCSTR modelName = displayInfoRec->m_modelName[whichComponent];
  if (!*modelName) {
    return 0;
  }

  if (subComp) {
    subComp->pathName = SStrDupA(modelName, __FILE__, __LINE__);
    LPCSTR textureName = displayInfoRec->m_modelTexture[whichComponent];
    if (*textureName) {
      char        buffer[MAX_PATH];
      TEXFILETYPE fileType = TextureDiscoverFileType(textureName);
      if (fileType == TEXFILETYPE_TGA) {
        TexturePickAlternateFilename(textureName, fileType, buffer, sizeof(buffer));
      } else {
        SStrCopy(buffer, textureName, sizeof(buffer));
      }
      subComp->textureName = SStrDupA(buffer, __FILE__, __LINE__);
    } else {
      subComp->textureName = 0;
    }
  }
  return 1;
}

UINT CompUtilGetObjComponents(
    const ItemDisplayInfoRec *displayInfoRec,
    int                       itemInventoryType,
    SUBCOMPONENTDESC         *subComponents,
    UINT                      numSubComponents,
    int                       useAlternate
) {
  ASSERT(numSubComponents == 2);
  ASSERT(subComponents);
  if (!displayInfoRec) {
    return 0;
  }

  if (!g_geometryComponentLookups[itemInventoryType].allowedSlots) {
    return 0;
  }

  UINT i;
  for (i = 0; i < numSubComponents; ++i) {
    subComponents->Cleanup();
  }

  UINT count = 0;
  for (UINT componentLink = 0; componentLink < 36 && count < 2; ++componentLink) {
    if (!(g_geometryComponentLookups[itemInventoryType].allowedSlots & ((LONGLONG)1 << componentLink))) {
      continue;
    }
    if (ReadSubComponent(displayInfoRec, count, itemInventoryType, &subComponents[count])) {
      subComponents[count].connectionPointIndex = useAlternate ? g_geometryComponentLookups[itemInventoryType].altItemLinks[count]
                                                               : g_geometryComponentLookups[itemInventoryType].itemLinks[count];
      ++count;
    }
  }
  return count;
}

UINT CompUtilGetObjComponentSlotFlags(const ItemDisplayInfoRec *displayInfoRec, int itemInventoryType, int useAlternateSlot) {
  if (!displayInfoRec) {
    return 0;
  }

  if (!g_geometryComponentLookups[itemInventoryType].allowedSlots) {
    return 0;
  }

  UINT flags = 0;
  UINT componentIndex = 0;
  for (UINT componentLink = 0; componentLink < 36 && componentIndex < 2; ++componentLink) {
    if (!(g_geometryComponentLookups[itemInventoryType].allowedSlots & ((LONGLONG)1 << componentLink))) {
      continue;
    }
    if (ReadSubComponent(displayInfoRec, componentIndex, itemInventoryType, 0)) {
      flags |= 1 << (useAlternateSlot ? g_geometryComponentLookups[itemInventoryType].altItemLinks[componentIndex]
                                      : g_geometryComponentLookups[itemInventoryType].itemLinks[componentIndex]);
      ++componentIndex;
    }
  }
  return flags;
}

LPCSTR CompUtilGetTextureSectionName(const ItemDisplayInfoRec *displayInfoRec, UINT textureSection) {
  if (!displayInfoRec || textureSection >= NUM_TEXCOMPONENT_SECTIONS) {
    return 0;
  }

  int textureIndex = s_textureSections[textureSection];
  if (textureIndex == -1) {
    FATALERROR(("Error, texture section %d doesn't have a texture field in the ItemDisplayInfo table!"));
  }
  return displayInfoRec->m_texture[textureIndex];
}

static LPCSTR s_itemVisualAnimNames[1] = {"stand"};

void ComponentUtilAddItemVisual(HMODEL itemModel, int index, LPCSTR name) {
  if (ModelIsLoaded(itemModel, 1) && !ModelHasLinkPoint(itemModel, index)) {
    return;
  }

  CModelCreate createData;
  createData.flags = 0x2002;
  createData.sequenceNames = s_itemVisualAnimNames;
  createData.numSequences = 1;

  if (itemModel && name && *name) {
    HMODEL visualModel = ModelCreate(name, &createData, 0);
    ModelAddLink(itemModel, index, visualModel, 1.0f);
    HandleClose(visualModel);
  }
}

HMODEL ComponentUtilGetChildModel(HMODEL parent, int index) {
  if (!parent) {
    return 0;
  }

  HMODEL model = 0;
  UINT   max = 1;
  ModelGetLinkPoint(parent, index, &model, &max);
  return model;
}
