#include "AttackAnimKitsRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall AttackAnimKitsRec::GetFilename() {
  return "DBFilesClient\\AttackAnimKits.dbc";
}

AttackAnimKitsRec::AttackAnimKitsRec() {
}

AttackAnimKitsRec::~AttackAnimKitsRec() {
}

bool AttackAnimKitsRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_ItemSubclassID, sizeof(m_ItemSubclassID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_AnimTypeID, sizeof(m_AnimTypeID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_AnimFrequency, sizeof(m_AnimFrequency), 0, 0, 0) && result;
  result = SFile::Read(f, &m_WhichHand, sizeof(m_WhichHand), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading AttackAnimKitsRec", DEFAULT_COLOR);
  }

  return result;
}
