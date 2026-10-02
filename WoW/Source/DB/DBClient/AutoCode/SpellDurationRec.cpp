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
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_duration) == 0);
  error |= (SFileReadTyped(f, &m_durationPerLevel) == 0);
  error |= (SFileReadTyped(f, &m_maxDuration) == 0);

  if (error) {
    ConsoleWrite("Error reading SpellDurationRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
