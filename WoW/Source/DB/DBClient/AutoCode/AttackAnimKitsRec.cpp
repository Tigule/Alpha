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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_ItemSubclassID) ||
      !SFileReadTyped(f, &m_AnimTypeID) ||
      !SFileReadTyped(f, &m_AnimFrequency) ||
      !SFileReadTyped(f, &m_WhichHand)) {
    ConsoleWrite("Error reading AttackAnimKitsRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
