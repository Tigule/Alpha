#include "AreaTriggerRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR AreaTriggerRec::GetFilename() {
  return "DBFilesClient\\AreaTrigger.dbc";
}

AreaTriggerRec::AreaTriggerRec() {
}

AreaTriggerRec::~AreaTriggerRec() {
}

bool AreaTriggerRec::Read(SFile *f, LPCSTR stringBuffer) {

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_ContinentID) ||
      !SFileReadTyped(f, &m_x) ||
      !SFileReadTyped(f, &m_y) ||
      !SFileReadTyped(f, &m_z) ||
      !SFileReadTyped(f, &m_radius)) {
    ConsoleWrite("Error reading AreaTriggerRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
