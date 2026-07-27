#include "CreatureDisplayInfoRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *CreatureDisplayInfoRec::GetFilename() {
  return "DBFilesClient\\CreatureDisplayInfo.dbc";
}

CreatureDisplayInfoRec::CreatureDisplayInfoRec() {
}

CreatureDisplayInfoRec::~CreatureDisplayInfoRec() {
}

bool CreatureDisplayInfoRec::Read(SFile *f, const char *stringBuffer) {
  bool         result = true;
  unsigned int temptextureVariationIndices[3];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_modelID, sizeof(m_modelID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_soundID, sizeof(m_soundID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_extendedDisplayInfoID, sizeof(m_extendedDisplayInfoID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_creatureModelScale, sizeof(m_creatureModelScale), 0, 0, 0) && result;
  result = SFile::Read(f, &m_creatureModelAlpha, sizeof(m_creatureModelAlpha), 0, 0, 0) && result;
  result = SFile::Read(f, &temptextureVariationIndices[0], sizeof(temptextureVariationIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &temptextureVariationIndices[1], sizeof(temptextureVariationIndices[1]), 0, 0, 0) && result;
  result = SFile::Read(f, &temptextureVariationIndices[2], sizeof(temptextureVariationIndices[2]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_bloodID, sizeof(m_bloodID), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading CreatureDisplayInfoRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_textureVariation[0] = stringBuffer + temptextureVariationIndices[0];
    m_textureVariation[1] = stringBuffer + temptextureVariationIndices[1];
    m_textureVariation[2] = stringBuffer + temptextureVariationIndices[2];
  } else {
    m_textureVariation[0] = "";
    m_textureVariation[1] = "";
    m_textureVariation[2] = "";
  }

  return true;
}
