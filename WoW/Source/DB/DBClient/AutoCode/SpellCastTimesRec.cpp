#include "SpellCastTimesRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR SpellCastTimesRec::GetFilename() {
  return "DBFilesClient\\SpellCastTimes.dbc";
}

SpellCastTimesRec::SpellCastTimesRec() {
}

SpellCastTimesRec::~SpellCastTimesRec() {
}

bool SpellCastTimesRec::Read(SFile *f, LPCSTR stringBuffer) {
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_base) == 0);
  error |= (SFileReadTyped(f, &m_perLevel) == 0);
  error |= (SFileReadTyped(f, &m_minimum) == 0);

  if (error) {
    ConsoleWrite("Error reading SpellCastTimesRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
