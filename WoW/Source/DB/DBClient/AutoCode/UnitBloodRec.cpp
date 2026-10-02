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
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_CombatBloodSpurtFront) == 0);
  error |= (SFileReadTyped(f, &m_CombatBloodSpurtBack) == 0);
  error |= (SFileReadTyped(f, &tempGroundBloodIndices[0]) == 0);
  error |= (SFileReadTyped(f, &tempGroundBloodIndices[1]) == 0);
  error |= (SFileReadTyped(f, &tempGroundBloodIndices[2]) == 0);
  error |= (SFileReadTyped(f, &tempGroundBloodIndices[3]) == 0);
  error |= (SFileReadTyped(f, &tempGroundBloodIndices[4]) == 0);

  if (error) {
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
