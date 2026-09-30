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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFile::Read(f, &m_proficiency_minLevel[0], sizeof(m_proficiency_minLevel), 0, 0, 0) ||
      !SFile::Read(f, &m_proficiency_acquireMethod[0], sizeof(m_proficiency_acquireMethod), 0, 0, 0) ||
      !SFile::Read(f, &m_proficiency_itemClass[0], sizeof(m_proficiency_itemClass), 0, 0, 0) ||
      !SFile::Read(f, &m_proficiency_itemSubClassMask[0], sizeof(m_proficiency_itemSubClassMask), 0, 0, 0)) {
    ConsoleWrite("Error reading ChrProficiencyRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
