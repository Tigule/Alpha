#include "SpellDurationRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR SpellDurationRec::GetFilename() {
  return "DBFilesClient\\SpellDuration.dbc";
}

SpellDurationRec::SpellDurationRec() {
}

SpellDurationRec::~SpellDurationRec() {
}

bool SpellDurationRec::Read(SFile *f, LPCSTR stringBuffer) {

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_duration) ||
      !SFileReadTyped(f, &m_durationPerLevel) ||
      !SFileReadTyped(f, &m_maxDuration)) {
    ConsoleWrite("Error reading SpellDurationRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
