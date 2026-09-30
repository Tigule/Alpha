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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFile::Read(f, &m_Type[0], sizeof(m_Type), 0, 0, 0) ||
      !SFile::Read(f, &m_Index[0], sizeof(m_Index), 0, 0, 0) ||
      !SFile::Read(f, &m_Skill[0], sizeof(m_Skill), 0, 0, 0) ||
      !SFile::Read(f, &m_Action[0], sizeof(m_Action), 0, 0, 0)) {
    ConsoleWrite("Error reading LockRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
