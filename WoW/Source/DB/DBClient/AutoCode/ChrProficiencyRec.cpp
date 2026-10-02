#include "ChrProficiencyRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR ChrProficiencyRec::GetFilename() {
  return "DBFilesClient\\ChrProficiency.dbc";
}

ChrProficiencyRec::ChrProficiencyRec() {
}

ChrProficiencyRec::~ChrProficiencyRec() {
}

bool ChrProficiencyRec::Read(SFile *f, LPCSTR stringBuffer) {
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_proficiency_minLevel) == 0);
  error |= (SFileReadTyped(f, &m_proficiency_acquireMethod) == 0);
  error |= (SFileReadTyped(f, &m_proficiency_itemClass) == 0);
  error |= (SFileReadTyped(f, &m_proficiency_itemSubClassMask) == 0);

  if (error) {
    ConsoleWrite("Error reading ChrProficiencyRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
