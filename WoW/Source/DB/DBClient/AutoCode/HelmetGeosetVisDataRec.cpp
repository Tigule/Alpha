#include "HelmetGeosetVisDataRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall HelmetGeosetVisDataRec::GetFilename() {
  return "DBFilesClient\\HelmetGeosetVisData.dbc";
}

HelmetGeosetVisDataRec::HelmetGeosetVisDataRec() {
}

HelmetGeosetVisDataRec::~HelmetGeosetVisDataRec() {
}

bool HelmetGeosetVisDataRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_DefaultFlags[0], sizeof(m_DefaultFlags), 0, 0, 0) && result;
  result = SFile::Read(f, &m_PreferredFlags[0], sizeof(m_PreferredFlags), 0, 0, 0) && result;
  result = SFile::Read(f, &m_HideFlags[0], sizeof(m_HideFlags), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading HelmetGeosetVisDataRec", DEFAULT_COLOR);
  }

  return result;
}
