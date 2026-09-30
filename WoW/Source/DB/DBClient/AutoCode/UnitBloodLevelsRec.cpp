#include "UnitBloodLevelsRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR UnitBloodLevelsRec::GetFilename() {
  return "DBFilesClient\\UnitBloodLevels.dbc";
}

UnitBloodLevelsRec::UnitBloodLevelsRec() {
}

UnitBloodLevelsRec::~UnitBloodLevelsRec() {
}

bool UnitBloodLevelsRec::Read(SFile *f, LPCSTR stringBuffer) {

  if (!SFileReadTyped(f, &m_ID) ||
      !SFile::Read(f, m_Violencelevel, sizeof(m_Violencelevel), 0, 0, 0)) {
    ConsoleWrite("Error reading UnitBloodLevelsRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
