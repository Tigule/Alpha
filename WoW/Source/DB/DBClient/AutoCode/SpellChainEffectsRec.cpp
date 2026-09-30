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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_AvgSegLen) ||
      !SFileReadTyped(f, &m_Width) ||
      !SFileReadTyped(f, &m_NoiseScale) ||
      !SFileReadTyped(f, &m_TexCoordScale) ||
      !SFileReadTyped(f, &m_SegDuration) ||
      !SFileReadTyped(f, &m_SegDelay) ||
      !SFileReadTyped(f, &tempTextureIndices[0])) {
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
