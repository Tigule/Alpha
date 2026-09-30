#include "GroundEffectTextureRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR GroundEffectTextureRec::GetFilename() {
  return "DBFilesClient\\GroundEffectTexture.dbc";
}

GroundEffectTextureRec::GroundEffectTextureRec() {
}

GroundEffectTextureRec::~GroundEffectTextureRec() {
}

bool GroundEffectTextureRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT temptextureNameIndices[1];

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_datestamp) ||
      !SFileReadTyped(f, &m_continentId) ||
      !SFileReadTyped(f, &m_zoneId) ||
      !SFileReadTyped(f, &m_textureId) ||
      !SFileReadTyped(f, &temptextureNameIndices[0]) ||
      !SFile::Read(f, &m_doodadId[0], sizeof(m_doodadId), 0, 0, 0) ||
      !SFileReadTyped(f, &m_density) ||
      !SFileReadTyped(f, &m_sound)) {
    ConsoleWrite("Error reading GroundEffectTextureRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_textureName = stringBuffer + temptextureNameIndices[0];
  } else {
    m_textureName = "";
  }

  return true;
}
