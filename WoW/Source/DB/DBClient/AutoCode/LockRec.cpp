#include "LockRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR LockRec::GetFilename() {
  return "DBFilesClient\\Lock.dbc";
}

LockRec::LockRec() {
}

LockRec::~LockRec() {
}

bool LockRec::Read(SFile *f, LPCSTR stringBuffer) {
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_Type) == 0);
  error |= (SFileReadTyped(f, &m_Index) == 0);
  error |= (SFileReadTyped(f, &m_Skill) == 0);
  error |= (SFileReadTyped(f, &m_Action) == 0);

  if (error) {
    ConsoleWrite("Error reading LockRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
