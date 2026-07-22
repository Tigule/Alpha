#include "AreaTriggerRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall AreaTriggerRec::GetFilename() {
  return "DBFilesClient\\AreaTrigger.dbc";
}

AreaTriggerRec::AreaTriggerRec() {
}

AreaTriggerRec::~AreaTriggerRec() {
}

bool AreaTriggerRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_ContinentID, sizeof(m_ContinentID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_x, sizeof(m_x), 0, 0, 0) && result;
  result = SFile::Read(f, &m_y, sizeof(m_y), 0, 0, 0) && result;
  result = SFile::Read(f, &m_z, sizeof(m_z), 0, 0, 0) && result;
  result = SFile::Read(f, &m_radius, sizeof(m_radius), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading AreaTriggerRec", DEFAULT_COLOR);
  }

  return result;
}
