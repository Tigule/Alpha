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
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_SoundID) == 0);

  if (error) {
    ConsoleWrite("Error reading NPCSoundsRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
