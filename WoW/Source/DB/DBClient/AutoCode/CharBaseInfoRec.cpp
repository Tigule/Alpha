#include "CharBaseInfoRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

LPCSTR CharBaseInfoRec::GetFilename() {
  return "DBFilesClient\\CharBaseInfo.dbc";
}

CharBaseInfoRec::CharBaseInfoRec() {
}

CharBaseInfoRec::~CharBaseInfoRec() {
}

bool CharBaseInfoRec::Read(SFile *f, LPCSTR stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_raceID, sizeof(m_raceID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_classID, sizeof(m_classID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_proficiency, sizeof(m_proficiency), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading CharBaseInfoRec", DEFAULT_COLOR);
  }

  return result;
}
