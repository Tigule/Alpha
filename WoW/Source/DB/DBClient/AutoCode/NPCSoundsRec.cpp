#include "NPCSoundsRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR NPCSoundsRec::GetFilename() {
  return "DBFilesClient\\NPCSounds.dbc";
}

NPCSoundsRec::NPCSoundsRec() {
}

NPCSoundsRec::~NPCSoundsRec() {
}

bool NPCSoundsRec::Read(SFile *f, LPCSTR stringBuffer) {

  if (!SFileReadTyped(f, &m_ID) ||
      !SFile::Read(f, &m_SoundID[0], sizeof(m_SoundID), 0, 0, 0)) {
    ConsoleWrite("Error reading NPCSoundsRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
