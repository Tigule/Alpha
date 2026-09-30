#include "UnitBloodRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR UnitBloodRec::GetFilename() {
  return "DBFilesClient\\UnitBlood.dbc";
}

UnitBloodRec::UnitBloodRec() {
}

UnitBloodRec::~UnitBloodRec() {
}

bool UnitBloodRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempGroundBloodIndices[5];

  if (!SFileReadTyped(f, &m_ID) ||
      !SFile::Read(f, m_CombatBloodSpurtFront, sizeof(m_CombatBloodSpurtFront), 0, 0, 0) ||
      !SFile::Read(f, m_CombatBloodSpurtBack, sizeof(m_CombatBloodSpurtBack), 0, 0, 0) ||
      !SFileReadTyped(f, &tempGroundBloodIndices[0]) ||
      !SFileReadTyped(f, &tempGroundBloodIndices[1]) ||
      !SFileReadTyped(f, &tempGroundBloodIndices[2]) ||
      !SFileReadTyped(f, &tempGroundBloodIndices[3]) ||
      !SFileReadTyped(f, &tempGroundBloodIndices[4])) {
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
