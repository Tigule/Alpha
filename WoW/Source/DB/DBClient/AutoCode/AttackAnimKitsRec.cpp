#include "AttackAnimKitsRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR AttackAnimKitsRec::GetFilename() {
  return "DBFilesClient\\AttackAnimKits.dbc";
}

AttackAnimKitsRec::AttackAnimKitsRec() {
}

AttackAnimKitsRec::~AttackAnimKitsRec() {
}

bool AttackAnimKitsRec::Read(SFile *f, LPCSTR stringBuffer) {
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_ItemSubclassID) == 0);
  error |= (SFileReadTyped(f, &m_AnimTypeID) == 0);
  error |= (SFileReadTyped(f, &m_AnimFrequency) == 0);
  error |= (SFileReadTyped(f, &m_WhichHand) == 0);

  if (error) {
    ConsoleWrite("Error reading AttackAnimKitsRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
