#include "CreatureDisplayInfoRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR CreatureDisplayInfoRec::GetFilename() {
  return "DBFilesClient\\CreatureDisplayInfo.dbc";
}

CreatureDisplayInfoRec::CreatureDisplayInfoRec() {
}

CreatureDisplayInfoRec::~CreatureDisplayInfoRec() {
}

bool CreatureDisplayInfoRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT temptextureVariationIndices[3];

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_modelID) ||
      !SFileReadTyped(f, &m_soundID) ||
      !SFileReadTyped(f, &m_extendedDisplayInfoID) ||
      !SFileReadTyped(f, &m_creatureModelScale) ||
      !SFileReadTyped(f, &m_creatureModelAlpha) ||
      !SFileReadTyped(f, &temptextureVariationIndices[0]) ||
      !SFileReadTyped(f, &temptextureVariationIndices[1]) ||
      !SFileReadTyped(f, &temptextureVariationIndices[2]) ||
      !SFileReadTyped(f, &m_bloodID)) {
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
