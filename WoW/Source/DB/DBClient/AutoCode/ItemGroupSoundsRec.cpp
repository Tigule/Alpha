#include "ItemGroupSoundsRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR ItemGroupSoundsRec::GetFilename() {
  return "DBFilesClient\\ItemGroupSounds.dbc";
}

ItemGroupSoundsRec::ItemGroupSoundsRec() {
}

ItemGroupSoundsRec::~ItemGroupSoundsRec() {
}

bool ItemGroupSoundsRec::Read(SFile *f, LPCSTR stringBuffer) {
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_sound) == 0);

  if (error) {
    ConsoleWrite("Error reading ItemGroupSoundsRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
