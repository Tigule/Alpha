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
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_Violencelevel) == 0);

  if (error) {
    ConsoleWrite("Error reading UnitBloodLevelsRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
