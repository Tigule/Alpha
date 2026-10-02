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
  int error = 0;

  error |= (SFileReadTyped(f, &m_raceID) == 0);
  error |= (SFileReadTyped(f, &m_classID) == 0);
  error |= (SFileReadTyped(f, &m_proficiency) == 0);

  if (error) {
    ConsoleWrite("Error reading CharBaseInfoRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
