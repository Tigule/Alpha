#include "LockRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *LockRec::GetFilename() {
  return "DBFilesClient\\Lock.dbc";
}

LockRec::LockRec() {
}

LockRec::~LockRec() {
}

bool LockRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_Type[0], sizeof(m_Type), 0, 0, 0) && result;
  result = SFile::Read(f, &m_Index[0], sizeof(m_Index), 0, 0, 0) && result;
  result = SFile::Read(f, &m_Skill[0], sizeof(m_Skill), 0, 0, 0) && result;
  result = SFile::Read(f, &m_Action[0], sizeof(m_Action), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading LockRec", DEFAULT_COLOR);
  }

  return result;
}
