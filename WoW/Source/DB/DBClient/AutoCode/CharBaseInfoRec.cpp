#include "CharBaseInfoRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR CharBaseInfoRec::GetFilename() {
  return "DBFilesClient\\CharBaseInfo.dbc";
}

CharBaseInfoRec::CharBaseInfoRec() {
}

CharBaseInfoRec::~CharBaseInfoRec() {
}

bool CharBaseInfoRec::Read(SFile *f, LPCSTR stringBuffer) {

  if (!SFile::Read(f, &m_raceID, sizeof(m_raceID), 0, 0, 0) ||
      !SFile::Read(f, &m_classID, sizeof(m_classID), 0, 0, 0) ||
      !SFileReadTyped(f, &m_proficiency)) {
    ConsoleWrite("Error reading CharBaseInfoRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
