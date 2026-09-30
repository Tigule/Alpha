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

  if (!SFileReadTyped(f, &m_RaceID) ||
      !SFileReadTyped(f, &m_SexID) ||
      !SFile::Read(f, &m_TextureHoldLayer[0], sizeof(m_TextureHoldLayer), 0, 0, 0)) {
    ConsoleWrite("Error reading CharVariationsRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
