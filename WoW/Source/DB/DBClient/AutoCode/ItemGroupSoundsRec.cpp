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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFile::Read(f, &m_sound[0], sizeof(m_sound), 0, 0, 0)) {
    ConsoleWrite("Error reading ItemGroupSoundsRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
