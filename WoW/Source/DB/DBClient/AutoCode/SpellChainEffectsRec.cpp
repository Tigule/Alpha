#include "SpellChainEffectsRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR SpellChainEffectsRec::GetFilename() {
  return "DBFilesClient\\SpellChainEffects.dbc";
}

SpellChainEffectsRec::SpellChainEffectsRec() {
}

SpellChainEffectsRec::~SpellChainEffectsRec() {
}

bool SpellChainEffectsRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempTextureIndices[1];
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_AvgSegLen) == 0);
  error |= (SFileReadTyped(f, &m_Width) == 0);
  error |= (SFileReadTyped(f, &m_NoiseScale) == 0);
  error |= (SFileReadTyped(f, &m_TexCoordScale) == 0);
  error |= (SFileReadTyped(f, &m_SegDuration) == 0);
  error |= (SFileReadTyped(f, &m_SegDelay) == 0);
  error |= (SFileReadTyped(f, &tempTextureIndices[0]) == 0);

  if (error) {
    ConsoleWrite("Error reading SpellChainEffectsRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_Texture = &stringBuffer[tempTextureIndices[0]];
  } else {
    m_Texture = "";
  }

  return true;
}
