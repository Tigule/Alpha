#include "GroundEffectTextureRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

LPCSTR GroundEffectTextureRec::GetFilename() {
  return "DBFilesClient\\GroundEffectTexture.dbc";
}

GroundEffectTextureRec::GroundEffectTextureRec() {
}

GroundEffectTextureRec::~GroundEffectTextureRec() {
}

bool GroundEffectTextureRec::Read(SFile *f, LPCSTR stringBuffer) {
  bool result = true;
  UINT temptextureNameIndices[1];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_datestamp, sizeof(m_datestamp), 0, 0, 0) && result;
  result = SFile::Read(f, &m_continentId, sizeof(m_continentId), 0, 0, 0) && result;
  result = SFile::Read(f, &m_zoneId, sizeof(m_zoneId), 0, 0, 0) && result;
  result = SFile::Read(f, &m_textureId, sizeof(m_textureId), 0, 0, 0) && result;
  result = SFile::Read(f, &temptextureNameIndices[0], sizeof(temptextureNameIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_doodadId[0], sizeof(m_doodadId), 0, 0, 0) && result;
  result = SFile::Read(f, &m_density, sizeof(m_density), 0, 0, 0) && result;
  result = SFile::Read(f, &m_sound, sizeof(m_sound), 0, 0, 0) && result;

  if (!result) {
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
