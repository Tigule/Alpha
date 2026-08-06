#include "AttackAnimTypesRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

LPCSTR AttackAnimTypesRec::GetFilename() {
  return "DBFilesClient\\AttackAnimTypes.dbc";
}

AttackAnimTypesRec::AttackAnimTypesRec() {
}

AttackAnimTypesRec::~AttackAnimTypesRec() {
}

bool AttackAnimTypesRec::Read(SFile *f, LPCSTR stringBuffer) {
  bool result = true;
  UINT tempAnimNameIndices[1];

  result = SFile::Read(f, &m_AnimID, sizeof(m_AnimID), 0, 0, 0) && result;
  result = SFile::Read(f, &tempAnimNameIndices[0], sizeof(tempAnimNameIndices[0]), 0, 0, 0) && result;

  if (!result) {
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
