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
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_ContinentID) == 0);
  error |= (SFileReadTyped(f, &m_x) == 0);
  error |= (SFileReadTyped(f, &m_y) == 0);
  error |= (SFileReadTyped(f, &m_z) == 0);
  error |= (SFileReadTyped(f, &m_radius) == 0);

  if (error) {
    ConsoleWrite("Error reading AreaTriggerRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
