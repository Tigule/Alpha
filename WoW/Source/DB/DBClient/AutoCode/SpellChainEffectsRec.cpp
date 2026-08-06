#include "SpellChainEffectsRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

LPCSTR SpellChainEffectsRec::GetFilename() {
  return "DBFilesClient\\SpellChainEffects.dbc";
}

SpellChainEffectsRec::SpellChainEffectsRec() {
}

SpellChainEffectsRec::~SpellChainEffectsRec() {
}

bool SpellChainEffectsRec::Read(SFile *f, LPCSTR stringBuffer) {
  bool result = true;
  UINT tempTextureIndices[1];

  result = SFileReadTyped(f, &m_ID) && result;
  result = SFileReadTyped(f, &m_AvgSegLen) && result;
  result = SFileReadTyped(f, &m_Width) && result;
  result = SFileReadTyped(f, &m_NoiseScale) && result;
  result = SFileReadTyped(f, &m_TexCoordScale) && result;
  result = SFileReadTyped(f, &m_SegDuration) && result;
  result = SFileReadTyped(f, &m_SegDelay) && result;
  result = SFileReadTyped(f, &tempTextureIndices[0]) && result;

  if (!result) {
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
