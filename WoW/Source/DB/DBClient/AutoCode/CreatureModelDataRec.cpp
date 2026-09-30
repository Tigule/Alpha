#include "CreatureModelDataRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR CreatureModelDataRec::GetFilename() {
  return "DBFilesClient\\CreatureModelData.dbc";
}

CreatureModelDataRec::CreatureModelDataRec() {
}

CreatureModelDataRec::~CreatureModelDataRec() {
}

bool CreatureModelDataRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempModelNameIndices[1];

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_flags) ||
      !SFileReadTyped(f, &tempModelNameIndices[0]) ||
      !SFileReadTyped(f, &m_sizeClass) ||
      !SFileReadTyped(f, &m_modelScale) ||
      !SFileReadTyped(f, &m_bloodID) ||
      !SFileReadTyped(f, &m_footprintTextureID) ||
      !SFileReadTyped(f, &m_footprintTextureLength) ||
      !SFileReadTyped(f, &m_footprintTextureWidth) ||
      !SFileReadTyped(f, &m_footprintParticleScale) ||
      !SFileReadTyped(f, &m_foleyMaterialID) ||
      !SFileReadTyped(f, &m_footstepShakeSize) ||
      !SFileReadTyped(f, &m_deathThudShakeSize) ||
      !SFileReadTyped(f, &m_soundID)) {
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
