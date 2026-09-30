#include "AttackAnimTypesRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR AttackAnimTypesRec::GetFilename() {
  return "DBFilesClient\\AttackAnimTypes.dbc";
}

AttackAnimTypesRec::AttackAnimTypesRec() {
}

AttackAnimTypesRec::~AttackAnimTypesRec() {
}

bool AttackAnimTypesRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempAnimNameIndices[1];

  if (!SFileReadTyped(f, &m_AnimID) ||
      !SFileReadTyped(f, &tempAnimNameIndices[0])) {
    ConsoleWrite("Error reading AttackAnimTypesRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_AnimName = stringBuffer + tempAnimNameIndices[0];
  } else {
    m_AnimName = "";
  }

  return true;
}
