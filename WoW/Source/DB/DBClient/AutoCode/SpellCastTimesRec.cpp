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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_base) ||
      !SFileReadTyped(f, &m_perLevel) ||
      !SFileReadTyped(f, &m_minimum)) {
    ConsoleWrite("Error reading SpellCastTimesRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
