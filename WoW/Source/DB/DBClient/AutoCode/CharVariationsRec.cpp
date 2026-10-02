#include "CharVariationsRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR CharVariationsRec::GetFilename() {
  return "DBFilesClient\\CharVariations.dbc";
}

CharVariationsRec::CharVariationsRec() {
}

CharVariationsRec::~CharVariationsRec() {
}

bool CharVariationsRec::Read(SFile *f, LPCSTR stringBuffer) {
  int error = 0;

  error |= (SFileReadTyped(f, &m_RaceID) == 0);
  error |= (SFileReadTyped(f, &m_SexID) == 0);
  error |= (SFileReadTyped(f, &m_TextureHoldLayer) == 0);

  if (error) {
    ConsoleWrite("Error reading CharVariationsRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
