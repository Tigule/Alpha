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
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_Slot) == 0);

  if (error) {
    ConsoleWrite("Error reading ItemVisualsRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
