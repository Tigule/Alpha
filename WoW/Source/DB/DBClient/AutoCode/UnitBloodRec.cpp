#include "UnitBloodRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall UnitBloodRec::GetFilename() {
  return "DBFilesClient\\UnitBlood.dbc";
}

UnitBloodRec::UnitBloodRec() {
}

UnitBloodRec::~UnitBloodRec() {
}

bool UnitBloodRec::Read(SFile *f, const char *stringBuffer) {
  bool         result = true;
  unsigned int tempGroundBloodIndices[5];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, m_CombatBloodSpurtFront, sizeof(m_CombatBloodSpurtFront), 0, 0, 0) && result;
  result = SFile::Read(f, m_CombatBloodSpurtBack, sizeof(m_CombatBloodSpurtBack), 0, 0, 0) && result;
  result = SFile::Read(f, &tempGroundBloodIndices[0], sizeof(tempGroundBloodIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempGroundBloodIndices[1], sizeof(tempGroundBloodIndices[1]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempGroundBloodIndices[2], sizeof(tempGroundBloodIndices[2]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempGroundBloodIndices[3], sizeof(tempGroundBloodIndices[3]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempGroundBloodIndices[4], sizeof(tempGroundBloodIndices[4]), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading UnitBloodRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_GroundBlood[0] = &stringBuffer[tempGroundBloodIndices[0]];
    m_GroundBlood[1] = &stringBuffer[tempGroundBloodIndices[1]];
    m_GroundBlood[2] = &stringBuffer[tempGroundBloodIndices[2]];
    m_GroundBlood[3] = &stringBuffer[tempGroundBloodIndices[3]];
    m_GroundBlood[4] = &stringBuffer[tempGroundBloodIndices[4]];
  } else {
    m_GroundBlood[0] = "";
    m_GroundBlood[1] = "";
    m_GroundBlood[2] = "";
    m_GroundBlood[3] = "";
    m_GroundBlood[4] = "";
  }

  return true;
}
