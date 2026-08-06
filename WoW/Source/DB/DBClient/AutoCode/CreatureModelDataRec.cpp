#include "CreatureModelDataRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

LPCSTR CreatureModelDataRec::GetFilename() {
  return "DBFilesClient\\CreatureModelData.dbc";
}

CreatureModelDataRec::CreatureModelDataRec() {
}

CreatureModelDataRec::~CreatureModelDataRec() {
}

bool CreatureModelDataRec::Read(SFile *f, LPCSTR stringBuffer) {
  bool result = true;
  UINT tempModelNameIndices[1];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_flags, sizeof(m_flags), 0, 0, 0) && result;
  result = SFile::Read(f, &tempModelNameIndices[0], sizeof(tempModelNameIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_sizeClass, sizeof(m_sizeClass), 0, 0, 0) && result;
  result = SFile::Read(f, &m_modelScale, sizeof(m_modelScale), 0, 0, 0) && result;
  result = SFile::Read(f, &m_bloodID, sizeof(m_bloodID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_footprintTextureID, sizeof(m_footprintTextureID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_footprintTextureLength, sizeof(m_footprintTextureLength), 0, 0, 0) && result;
  result = SFile::Read(f, &m_footprintTextureWidth, sizeof(m_footprintTextureWidth), 0, 0, 0) && result;
  result = SFile::Read(f, &m_footprintParticleScale, sizeof(m_footprintParticleScale), 0, 0, 0) && result;
  result = SFile::Read(f, &m_foleyMaterialID, sizeof(m_foleyMaterialID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_footstepShakeSize, sizeof(m_footstepShakeSize), 0, 0, 0) && result;
  result = SFile::Read(f, &m_deathThudShakeSize, sizeof(m_deathThudShakeSize), 0, 0, 0) && result;
  result = SFile::Read(f, &m_soundID, sizeof(m_soundID), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading CreatureModelDataRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_ModelName = stringBuffer + tempModelNameIndices[0];
  } else {
    m_ModelName = "";
  }

  return true;
}
