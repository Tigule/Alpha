#include "ChrProficiencyRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *ChrProficiencyRec::GetFilename() {
  return "DBFilesClient\\ChrProficiency.dbc";
}

ChrProficiencyRec::ChrProficiencyRec() {
}

ChrProficiencyRec::~ChrProficiencyRec() {
}

bool ChrProficiencyRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_proficiency_minLevel[0], sizeof(m_proficiency_minLevel), 0, 0, 0) && result;
  result = SFile::Read(f, &m_proficiency_acquireMethod[0], sizeof(m_proficiency_acquireMethod), 0, 0, 0) && result;
  result = SFile::Read(f, &m_proficiency_itemClass[0], sizeof(m_proficiency_itemClass), 0, 0, 0) && result;
  result = SFile::Read(f, &m_proficiency_itemSubClassMask[0], sizeof(m_proficiency_itemSubClassMask), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading ChrProficiencyRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
