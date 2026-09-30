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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_minScale) ||
      !SFileReadTyped(f, &m_minScaleLevel) ||
      !SFileReadTyped(f, &m_maxScale) ||
      !SFileReadTyped(f, &m_maxScaleLevel) ||
      !SFile::Read(f, &m_skillLine[0], sizeof(m_skillLine), 0, 0, 0)) {
    ConsoleWrite("Error reading CreatureFamilyRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
