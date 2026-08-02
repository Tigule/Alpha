#include <Component/Component.h>

#include "DB/DBClient/AutoCode/ItemDisplayInfoRec.h"

#include <Model/IModel.h>
#include <Services/Texture.h>

#include <string.h>

static const char *s_itemVisualAnimNames[1] = {"stand"};

void ComponentUtilAddItemVisual(HMODEL itemModel, int index, const char *name) {
  if (ModelIsLoaded(itemModel, 1) && !ModelHasLinkPoint(itemModel, index)) {
    return;
  }

  CModelCreate createData;
  memset(&createData.boneNames, 0, 4 * sizeof(createData.boneNames));
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

  HMODEL       model = 0;
  unsigned int max = 1;
  ModelGetLinkPoint(parent, index, &model, &max);
  return model;
}

struct SECTIONDESC {
  char        *m_columnName;
  unsigned int x;
  unsigned int y;
  unsigned int width;
  unsigned int height;
};

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

static const unsigned int s_sectionFlags[INDEX_NUMSLOTS] = {0, 0, 0, 0, 0x0E3, 0x0E3, 0x080, 0x180, 0x300, 0x002, 0x006, 0, 0, 0,
                                                            0, 0, 0, 0, 0,     0,     0x0E0, 0x1E3, 0,     0,     0,     0, 0};

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

int CompUtilGetSectionDimensions(unsigned int sectionIndex, unsigned int *width, unsigned int *height) {
  ASSERT(width);
  ASSERT(height);

  if (sectionIndex >= NUM_TEXCOMPONENT_SECTIONS) {
    return 0;
  }

  *width = s_sectionTable[sectionIndex].width;
  *height = s_sectionTable[sectionIndex].height;
  return 1;
}

int CompUtilGetSectionOffset(unsigned int sectionIndex, unsigned int *xCoord, unsigned int *yCoord) {
  ASSERT(xCoord);
  ASSERT(yCoord);

  if (sectionIndex >= NUM_TEXCOMPONENT_SECTIONS) {
    return 0;
  }

  *xCoord = s_sectionTable[sectionIndex].x;
  *yCoord = s_sectionTable[sectionIndex].y;
  return 1;
}

int CompUtilItemSectionInfo(
    const ItemDisplayInfoRec    *displayInfoRec,
    unsigned int                 inventoryType,
    unsigned int                *numTextureComponents,
    TEXCOMPONENT_SECTIONS        sectionList[6],
    TEXCOMPONENT_LAYERS          layerList[6],
    LAYERPRIORITY                priorityList[6],
    CSectionFileNames           *fileNameList
) {
  ASSERT(inventoryType < INDEX_NUMSLOTS);
  ASSERT(numTextureComponents);

  unsigned int numSections = 0;
  unsigned int section;
  for (section = 0; section < NUM_TEXCOMPONENT_SECTIONS; ++section) {
    if (!(s_sectionFlags[inventoryType] & (1 << section))) {
      continue;
    }

    sectionList[numSections] = static_cast<TEXCOMPONENT_SECTIONS>(section);
    layerList[numSections] = g_sectionLayers[inventoryType].layers[section];
    priorityList[numSections] = g_sectionPriorities[inventoryType].priorities[section];

    if (fileNameList) {
      const char *fileName = CompUtilGetTextureSectionName(displayInfoRec, section);
      if (fileName && *fileName) {
        SStrCopy(fileNameList->path[numSections], fileName, MAX_PATH);
      } else {
        fileNameList->path[numSections][0] = 0;
      }
    }

    ++numSections;
  }

  *numTextureComponents = numSections;
  return 1;
}

int CompUtilItemSectionInfo(INVENTORY_TYPES invType, TEXCOMPONENT_SECTIONS section, TEXCOMPONENT_LAYERS *layer, LAYERPRIORITY *priority) {
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

const char *CompUtilGetTextureSectionName(const ItemDisplayInfoRec *displayInfoRec, unsigned int textureSection) {
  if (!displayInfoRec || textureSection >= NUM_TEXCOMPONENT_SECTIONS) {
    return 0;
  }

  int textureIndex = s_textureSections[textureSection];
  FATALASSERT(textureIndex != -1);
  return displayInfoRec->m_texture[textureIndex];
}

static int
ReadSubComponent(const ItemDisplayInfoRec *displayInfoRec, unsigned int whichComponent, unsigned int inventoryType, SUBCOMPONENTDESC *subComp) {
  ASSERT(inventoryType < INDEX_NUMSLOTS);
  ASSERT(displayInfoRec);
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
  const char *modelName = displayInfoRec->m_modelName[whichComponent];
  if (!modelName || !*modelName) {
    return 0;
  }
  if (!subComp) {
    return 1;
  }

  subComp->pathName = SStrDupA(modelName, __FILE__, __LINE__);
  const char *textureName = displayInfoRec->m_modelTexture[whichComponent];
  if (textureName && *textureName) {
    char buffer[MAX_PATH];
    if (TextureDiscoverFileType(textureName) == TEXFILETYPE_TGA) {
      TexturePickAlternateFilename(textureName, TEXFILETYPE_TGA, buffer, sizeof(buffer));
      textureName = buffer;
    }
    subComp->textureName = SStrDupA(textureName, __FILE__, __LINE__);
  }
  return 1;
}

unsigned int CompUtilGetObjComponents(
    const ItemDisplayInfoRec *displayInfoRec,
    int                       itemInventoryType,
    SUBCOMPONENTDESC         *subComponents,
    unsigned int              numSubComponents,
    int                       useAlternate
) {
  ASSERT(numSubComponents == 2);
  ASSERT(subComponents);
  if (!displayInfoRec) {
    return 0;
  }

  unsigned int count = 0;
  unsigned int whichComponent;
  for (whichComponent = 0; whichComponent < 2 && count < numSubComponents; ++whichComponent) {
    if (ReadSubComponent(displayInfoRec, whichComponent, itemInventoryType, &subComponents[count])) {
      subComponents[count].connectionPointIndex =
          useAlternate ? g_geometryComponentLookups[itemInventoryType].altItemLinks[count]
                       : g_geometryComponentLookups[itemInventoryType].itemLinks[count];
      ++count;
    }
  }
  return count;
}

unsigned int CompUtilGetObjComponentSlotFlags(const ItemDisplayInfoRec *displayInfoRec, int itemInventoryType, int useAlternateSlot) {
  if (!displayInfoRec || !g_geometryComponentLookups[itemInventoryType].allowedSlots) {
    return 0;
  }

  unsigned int flags = 0;
  unsigned int componentIndex = 0;
  for (unsigned int componentLink = 0; componentLink < 36 && componentIndex < 2; ++componentLink) {
    if (!(g_geometryComponentLookups[itemInventoryType].allowedSlots & (static_cast<__int64>(1) << componentLink))) {
      continue;
    }
    if (ReadSubComponent(displayInfoRec, componentIndex, itemInventoryType, 0)) {
      int link = useAlternateSlot ? g_geometryComponentLookups[itemInventoryType].altItemLinks[componentIndex]
                                  : g_geometryComponentLookups[itemInventoryType].itemLinks[componentIndex];
      if (link >= 0) {
        flags |= 1 << link;
      }
      ++componentIndex;
    }
  }
  return flags;
}
