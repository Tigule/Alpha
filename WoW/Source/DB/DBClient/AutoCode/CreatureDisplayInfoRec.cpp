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
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_modelID) == 0);
  error |= (SFileReadTyped(f, &m_soundID) == 0);
  error |= (SFileReadTyped(f, &m_extendedDisplayInfoID) == 0);
  error |= (SFileReadTyped(f, &m_creatureModelScale) == 0);
  error |= (SFileReadTyped(f, &m_creatureModelAlpha) == 0);
  error |= (SFileReadTyped(f, &temptextureVariationIndices[0]) == 0);
  error |= (SFileReadTyped(f, &temptextureVariationIndices[1]) == 0);
  error |= (SFileReadTyped(f, &temptextureVariationIndices[2]) == 0);
  error |= (SFileReadTyped(f, &m_bloodID) == 0);

  if (error) {
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
