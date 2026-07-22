#include "ItemDisplayInfoRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall ItemDisplayInfoRec::GetFilename() {
  return "DBFilesClient\\ItemDisplayInfo.dbc";
}

ItemDisplayInfoRec::ItemDisplayInfoRec() {
}

ItemDisplayInfoRec::~ItemDisplayInfoRec() {
}

bool ItemDisplayInfoRec::Read(SFile *f, const char *stringBuffer) {
  bool         result = true;
  unsigned int tempgroundModelIndices[1];
  unsigned int temptextureIndices[8];
  unsigned int tempmodelNameIndices[2];
  unsigned int tempinventoryIconIndices[1];
  unsigned int tempmodelTextureIndices[2];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &tempmodelNameIndices[0], sizeof(tempmodelNameIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempmodelNameIndices[1], sizeof(tempmodelNameIndices[1]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempmodelTextureIndices[0], sizeof(tempmodelTextureIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempmodelTextureIndices[1], sizeof(tempmodelTextureIndices[1]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempinventoryIconIndices[0], sizeof(tempinventoryIconIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempgroundModelIndices[0], sizeof(tempgroundModelIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_geosetGroup[0], sizeof(m_geosetGroup), 0, 0, 0) && result;
  result = SFile::Read(f, &m_flags, sizeof(m_flags), 0, 0, 0) && result;
  result = SFile::Read(f, &m_spellVisualID, sizeof(m_spellVisualID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_groupSoundIndex, sizeof(m_groupSoundIndex), 0, 0, 0) && result;
  result = SFile::Read(f, &m_itemSize, sizeof(m_itemSize), 0, 0, 0) && result;
  result = SFile::Read(f, &m_helmetGeosetVisID, sizeof(m_helmetGeosetVisID), 0, 0, 0) && result;
  result = SFile::Read(f, &temptextureIndices[0], sizeof(temptextureIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &temptextureIndices[1], sizeof(temptextureIndices[1]), 0, 0, 0) && result;
  result = SFile::Read(f, &temptextureIndices[2], sizeof(temptextureIndices[2]), 0, 0, 0) && result;
  result = SFile::Read(f, &temptextureIndices[3], sizeof(temptextureIndices[3]), 0, 0, 0) && result;
  result = SFile::Read(f, &temptextureIndices[4], sizeof(temptextureIndices[4]), 0, 0, 0) && result;
  result = SFile::Read(f, &temptextureIndices[5], sizeof(temptextureIndices[5]), 0, 0, 0) && result;
  result = SFile::Read(f, &temptextureIndices[6], sizeof(temptextureIndices[6]), 0, 0, 0) && result;
  result = SFile::Read(f, &temptextureIndices[7], sizeof(temptextureIndices[7]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_itemVisual, sizeof(m_itemVisual), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading ItemDisplayInfoRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_modelName[0] = stringBuffer + tempmodelNameIndices[0];
    m_modelName[1] = stringBuffer + tempmodelNameIndices[1];
    m_modelTexture[0] = stringBuffer + tempmodelTextureIndices[0];
    m_modelTexture[1] = stringBuffer + tempmodelTextureIndices[1];
    m_inventoryIcon = stringBuffer + tempinventoryIconIndices[0];
    m_groundModel = stringBuffer + tempgroundModelIndices[0];
    m_texture[0] = stringBuffer + temptextureIndices[0];
    m_texture[1] = stringBuffer + temptextureIndices[1];
    m_texture[2] = stringBuffer + temptextureIndices[2];
    m_texture[3] = stringBuffer + temptextureIndices[3];
    m_texture[4] = stringBuffer + temptextureIndices[4];
    m_texture[5] = stringBuffer + temptextureIndices[5];
    m_texture[6] = stringBuffer + temptextureIndices[6];
    m_texture[7] = stringBuffer + temptextureIndices[7];
  } else {
    m_modelName[0] = "";
    m_modelName[1] = "";
    m_modelTexture[0] = "";
    m_modelTexture[1] = "";
    m_inventoryIcon = "";
    m_groundModel = "";
    m_texture[0] = "";
    m_texture[1] = "";
    m_texture[2] = "";
    m_texture[3] = "";
    m_texture[4] = "";
    m_texture[5] = "";
    m_texture[6] = "";
    m_texture[7] = "";
  }

  return true;
}
