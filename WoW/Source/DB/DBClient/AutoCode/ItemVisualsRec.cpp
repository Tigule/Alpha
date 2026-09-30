#include "ItemVisualsRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR ItemVisualsRec::GetFilename() {
  return "DBFilesClient\\ItemVisuals.dbc";
}

ItemVisualsRec::ItemVisualsRec() {
}

ItemVisualsRec::~ItemVisualsRec() {
}

bool ItemVisualsRec::Read(SFile *f, LPCSTR stringBuffer) {

  if (!SFileReadTyped(f, &m_ID) ||
      !SFile::Read(f, m_Slot, sizeof(m_Slot), 0, 0, 0)) {
    ConsoleWrite("Error reading ItemVisualsRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
