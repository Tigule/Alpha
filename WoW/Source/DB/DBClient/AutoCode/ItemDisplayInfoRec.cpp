#include "ItemDisplayInfoRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR ItemDisplayInfoRec::GetFilename() {
  return "DBFilesClient\\ItemDisplayInfo.dbc";
}

ItemDisplayInfoRec::ItemDisplayInfoRec() {
}

ItemDisplayInfoRec::~ItemDisplayInfoRec() {
}

bool ItemDisplayInfoRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempgroundModelIndices[1];
  UINT temptextureIndices[8];
  UINT tempmodelNameIndices[2];
  UINT tempinventoryIconIndices[1];
  UINT tempmodelTextureIndices[2];
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &tempmodelNameIndices[0]) == 0);
  error |= (SFileReadTyped(f, &tempmodelNameIndices[1]) == 0);
  error |= (SFileReadTyped(f, &tempmodelTextureIndices[0]) == 0);
  error |= (SFileReadTyped(f, &tempmodelTextureIndices[1]) == 0);
  error |= (SFileReadTyped(f, &tempinventoryIconIndices[0]) == 0);
  error |= (SFileReadTyped(f, &tempgroundModelIndices[0]) == 0);
  error |= (SFileReadTyped(f, &m_geosetGroup) == 0);
  error |= (SFileReadTyped(f, &m_flags) == 0);
  error |= (SFileReadTyped(f, &m_spellVisualID) == 0);
  error |= (SFileReadTyped(f, &m_groupSoundIndex) == 0);
  error |= (SFileReadTyped(f, &m_itemSize) == 0);
  error |= (SFileReadTyped(f, &m_helmetGeosetVisID) == 0);
  error |= (SFileReadTyped(f, &temptextureIndices[0]) == 0);
  error |= (SFileReadTyped(f, &temptextureIndices[1]) == 0);
  error |= (SFileReadTyped(f, &temptextureIndices[2]) == 0);
  error |= (SFileReadTyped(f, &temptextureIndices[3]) == 0);
  error |= (SFileReadTyped(f, &temptextureIndices[4]) == 0);
  error |= (SFileReadTyped(f, &temptextureIndices[5]) == 0);
  error |= (SFileReadTyped(f, &temptextureIndices[6]) == 0);
  error |= (SFileReadTyped(f, &temptextureIndices[7]) == 0);
  error |= (SFileReadTyped(f, &m_itemVisual) == 0);

  if (error) {
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
