#include "CreatureFamilyRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR CreatureFamilyRec::GetFilename() {
  return "DBFilesClient\\CreatureFamily.dbc";
}

CreatureFamilyRec::CreatureFamilyRec() {
}

CreatureFamilyRec::~CreatureFamilyRec() {
}

bool CreatureFamilyRec::Read(SFile *f, LPCSTR stringBuffer) {
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_minScale) == 0);
  error |= (SFileReadTyped(f, &m_minScaleLevel) == 0);
  error |= (SFileReadTyped(f, &m_maxScale) == 0);
  error |= (SFileReadTyped(f, &m_maxScaleLevel) == 0);
  error |= (SFileReadTyped(f, &m_skillLine) == 0);

  if (error) {
    ConsoleWrite("Error reading CreatureFamilyRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
