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
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_flags) == 0);
  error |= (SFileReadTyped(f, &tempModelNameIndices[0]) == 0);
  error |= (SFileReadTyped(f, &m_sizeClass) == 0);
  error |= (SFileReadTyped(f, &m_modelScale) == 0);
  error |= (SFileReadTyped(f, &m_bloodID) == 0);
  error |= (SFileReadTyped(f, &m_footprintTextureID) == 0);
  error |= (SFileReadTyped(f, &m_footprintTextureLength) == 0);
  error |= (SFileReadTyped(f, &m_footprintTextureWidth) == 0);
  error |= (SFileReadTyped(f, &m_footprintParticleScale) == 0);
  error |= (SFileReadTyped(f, &m_foleyMaterialID) == 0);
  error |= (SFileReadTyped(f, &m_footstepShakeSize) == 0);
  error |= (SFileReadTyped(f, &m_deathThudShakeSize) == 0);
  error |= (SFileReadTyped(f, &m_soundID) == 0);

  if (error) {
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
