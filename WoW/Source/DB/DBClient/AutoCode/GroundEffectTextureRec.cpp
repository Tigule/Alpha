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
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_datestamp) == 0);
  error |= (SFileReadTyped(f, &m_continentId) == 0);
  error |= (SFileReadTyped(f, &m_zoneId) == 0);
  error |= (SFileReadTyped(f, &m_textureId) == 0);
  error |= (SFileReadTyped(f, &temptextureNameIndices[0]) == 0);
  error |= (SFileReadTyped(f, &m_doodadId) == 0);
  error |= (SFileReadTyped(f, &m_density) == 0);
  error |= (SFileReadTyped(f, &m_sound) == 0);

  if (error) {
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
